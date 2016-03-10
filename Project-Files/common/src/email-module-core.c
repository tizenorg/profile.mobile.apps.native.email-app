/*
 * Copyright (c) 2009-2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "email-module-dev.h"

#include <dlfcn.h>
#include <efl_extension.h>

#include "email-debug.h"

#undef LOG_TAG
#define LOG_TAG "EMAIL_MODULE_CORE"

/**
 * Step increment to use when pushing new children to a parent module
 */
#define EMAIL_MODULE_CHILDREN_ARRAY_STEP 4

/**
 * Name of the entry point function to every module's .so library
 */
#define EMAIL_MODULE_ALLOC_FUNC_NAME "email_module_alloc"

/**
 * @brief Prototype of the entry point function
 *
 * @return pointer to the allocated module object, or NULL on error
 */
typedef email_module_t *(*alloc_func_t)();

/**
 * @brief Callback prototype which is used to iterate on through the tree of modules
 *
 * @param[in]	module		pointer to the module
 * @param[in]	cb_data		pointer to the callback data
 */
typedef void (*email_module_each_cb_t)(email_module_t *module, void *cb_data);

/**
 * Callback data structure to use in eina_array_foreach() function when iterate through tree of modules
 */
typedef struct {
	email_module_each_cb_t cb;
	void *cb_data;

} each_module_cb_data_t;

/**
 * Utils
 */

/**
 * @brief Gets module .so file name from module type enumeration
 *
 * @param[in]	module_type		type of the module
 *
 * @return name of the .so file or NULL on error
 */
#ifdef SHARED_MODULES_FEATURE
static const char *_get_module_lib_name(email_module_type_e module_type);
#endif

/**
 * email_module_mgr
 */

/**
 * Global data structure of the module manager.
 * This is the root object for all modules of the application.
 */
static struct _email_module_mgr
{
	/* Root window element of the application */
	Evas_Object *win;

	/* Root layout of the application.
	 * Handles layout resize on keypad show/hide.
	 * Displays indicator bar. */
	Evas_Object *conform;
	/* Main widget to hold module's views.
	 * Handles view navigation.
	 * Provides parts for title and tool bar buttons. */
	Evas_Object *navi;

	/* Pointer to the root module.
	 * This module must be created first.
	 * Only one root module can be created.
	 * Root module can be recreated during program execution. */
	email_module_t *root_module;
	/* Last created module. Only this module can be parent to new modules and create views. */
	email_module_t *top_module;

	/* Ecore job which is used to perform popping of views.
	 * Job is used to allow popping through multiple views and even modules. */
	Ecore_Job *pop_job;
	/* Current item to which we need to pop in pop-job callback */
	Elm_Object_Item *pop_to_item;
	/* Item that was previously popped but was not deleted yet. */
	Elm_Object_Item *popped_item;
	 /* Old top item that was pushed in by new top item */
	Elm_Object_Item *pushed_in_item;

	/* Indicate whether the naviframe is currently in transition state (push/pop animation) */
	bool navi_in_transition;
	/* Used to handle special case when transition finish
	 * callback in the destroy callback of the last popped item */
	bool is_delayed_transition;

	/* Indicates whether the module manager is in the paused state */
	bool paused;
	/* Indicate whether the module manager was successfully initialized */
	bool initialized;

} MODULE_MGR;

/**
 * @brief Allocates module's object from shared library
 *
 * @param[in]	module_type		type of the module
 *
 * @return pointer to the allocated module or NULL on error
 */
static email_module_t *_email_module_mgr_alloc_module(email_module_type_e module_type);

/**
 * @brief Creates and initializes an email module
 *
 * @param[in]	module_type		type of the module
 * @param[in]	params			create parameters
 * @param[in]	listener		module listener
 * @param[in]	parent			parent module or NULL
 *
 * @return handle to the module or NULL on error
 */
static email_module_h _email_module_mgr_create_module(email_module_type_e module_type, email_params_h params,
		email_module_listener_t *listener, email_module_t *parent);

/**
 * @brief Adds specified update flag to all views of the application
 *
 * @param[in]	flags		update flags
 */
static void _email_module_mgr_add_view_ufs(int flags);

/**
 * @brief Activate new top view after previous top view was destroyed
 *
 * @return 0 on success,
 * 		negative value on error
 */
static int _email_module_mgr_activate_new_top_view();

/**
 * @brief Performs the necessary logic to exit from the specified view
 *
 * @param[in]	view_item		pointer to the view item
 *
 * @return 0 on success,
 * 		negative value on error
 */
static int _email_module_mgr_exit_view(Elm_Object_Item *view_item);

/**
 * @brief Schedules pop-job to a specified view item
 *
 * @param[in]	view_item		pointer to the view item
 *
 * @return 0 on success,
 * 		negative value on error
 */
static int _email_module_mgr_pop_view_to(Elm_Object_Item *view_item);

/**
 * @brief Callback which is called but the naviframe when transition has finished
 *
 * @param[in]	data		pointer to the user data
 * @param[in]	obj			pointer to evas object
 * @param[in]	event_info	pointer to event enfo
 */
static void _email_module_mgr_transition_finished_cb(void *data, Evas_Object *obj, void *event_info);

/**
 * @brief Callback to handle hardware back key press
 *
 * @param[in]	data		pointer to the user data
 * @param[in]	obj			pointer to evas object
 * @param[in]	event_info	pointer to event enfo
 */
static void _email_module_mgr_navi_back_cb(void *data, Evas_Object *obj, void *event_info);

/**
 * @brief Handler of the pop-job callback
 *
 * @param[in]	data		pointer to the user data
 */
static void _email_module_mgr_pop_job_cb(void *data);

/**
 * email_module
 */

/**
 * @brief Initializes allocated module object
 *
 * @param[in]	module				pointer to the module
 * @param[in]	params				create parameters
 * @param[in]	listener			module listener
 * @param[in]	parent				parent module or NULL
 * @param[in]	parent_navi_item	pointer to parent naviframe item or NULL
 *
 * @return 0 on success,
 * 		negative value on error
 */
static int _email_module_init(email_module_t *module, email_params_h params,
		email_module_listener_t *listener, email_module_t *parent, Elm_Object_Item *parent_navi_item);

/**
 * @brief Adds child module to the module
 *
 * @param[in]	module		pointer to the parent module
 * @param[in]	child		pointer to the child module
 *
 * @return 0 on success,
 * 		negative value on error
 */
static int _email_module_add_child(email_module_t *module, email_module_t *child);

/**
 * @brief Finalizes allocated module.
 *
 * Module may be not completely initialized.
 * It is possible to skip view cleanup if module.
 * is get finalized by the deletion of the last view.
 *
 * @param[in]	module				pointer to the module
 * @param[in]	skip_views_cleanup	true, if no need to destroy views, otherwise - false
 */
static void _email_module_finalize(email_module_t *module, bool skip_views_cleanup);

/**
 * @brief Deletes all views of the specified module
 *
 * @param[in]	module				pointer to the module
 */
static void _email_module_cleanup_views(email_module_t *module);

/**
 * @brief Destroys initialized module.
 *
 * This function allows to skip navifram pop animation.
 *
 * @param[in]	module		pointer to the module
 * @param[in]	use_pop		true, if use popping, otherwise - false
 *
 * @return 0 on success,
 * 		negative value on error
 */
static int _email_module_destroy(email_module_t *module, bool use_pop);

/**
 * @brief Used to remove a specified module from parent's module children array
 *
 * @param[in]	module		pointer to the module
 * @param[in]	use_pop		true, if use popping, otherwise - false
 *
 * @return Eina_True if object should be deleted,
 * 		Eina_False - otherwise
 */
static Eina_Bool _email_module_keep_all_except_cb(void *cur_module, void *except_module);


/**
 * @brief Used in _email_module_foreach_module() to mark all modules for deletion
 *
 * @param[in]	module		pointer to the module
 * @param[in]	cb_data		pointer to the callback data
 */
static void _email_module_mark_for_destroy_cb(email_module_t *module, void *cb_data);

/**
 * @brief Handles module manager pause event
 *
 * @param[in]	module		pointer to the module
 */
static void _email_module_do_pause(email_module_t *module);

/**
 * @brief Handles module manager resume event
 *
 * @param[in]	module		pointer to the module
 */
static void _email_module_do_resume(email_module_t *module);

/**
 * @brief Handles module manager event callback
 *
 * @param[in]	module		pointer to the module
 * @param[in]	event		event to send
 */
static void _email_module_do_send_event(email_module_t *module, email_module_event_e event);

/**
 * @brief Used in _email_module_do_pause() to pass the event to all children modules
 *
 * @param[in]	module		pointer to the module
 * @param[in]	cb_data		pointer to the callback data
 */
static void _email_module_pause_cb(email_module_t *module, void *cb_data);

/**
 * @brief Used in _email_module_do_resume() to pass the event to all children modules
 *
 * @param[in]	module		pointer to the module
 * @param[in]	cb_data		pointer to the callback data
 */
static void _email_module_resume_cb(email_module_t *module, void *cb_data);

/**
 * @brief Used in _email_module_do_send_event() to pass the event to all children modules
 *
 * @param[in]	module		pointer to the module
 * @param[in]	cb_data		pointer to the callback data
 */
static void _email_module_send_event_cb(email_module_t *module, void *cb_data);

/**
 * @brief Sends destroy request to the parent module.
 *
 * Because only parent module can destroy a child module.
 *
 * @param[in]	module		pointer to the module
 *
 * @return 0 on success,
 * 		negative value on error
 */
static int _email_module_make_destroy_request(email_module_t *module);

/**
 * @brief Iterates over each module in the module tree
 *
 * @param[in]	module		pointer to the base module of the tree
 * @param[in]	cb			pointer to the callback function to call for each module in the tree
 * @param[in]	cb_data		pointer to the callback data
 */
static void _email_module_foreach_module(email_module_t *module, email_module_each_cb_t cb, void *cb_data);

/**
 * @brief Recursive implementation of _email_module_foreach_module() function
 *
 * @param[in]	module		pointer to the current module
 * @param[in]	each_data	each data to pass across recursive calls
 */
static void _email_module_foreach_module_impl(email_module_t *module, each_module_cb_data_t *each_data);

/**
 * @brief Used in _email_module_foreach_module_impl() to iterate over a module's children
 *
 * @param[in]	container	pointer to the containre object (Eina_Array in our case)
 * @param[in]	data		pointer to the current element
 * @param[in]	fdata		user data to pass to each call
 *
 * @return Eina_True - to continue iteration,
 * 		Eina_False - otherwise
 */
static Eina_Bool _email_module_each_module_cb(const void *container, void *data, void *fdata);

/**
 * email_view
 */

/**
 * @brief Initializes email view
 *
 * @param[in]	view		pointer to the new view
 * @param[in]	module		pointer to the module of the view
 *
 * @return 0 on success,
 * 		negative value on error
 */
static int _email_view_init(email_view_t *view, email_module_t *module);

/**
 * @brief Finalizes email view
 *
 * @param[in]	view		pointer to the view
 */
static void _email_view_finalize(email_view_t *view);

/**
 * @brief Activates email view
 *
 * @param[in]	view		pointer to the view
 */
static void _email_view_activate(email_view_t *view);

/**
 * @brief Deactivates email view
 *
 * @param[in]	view		pointer to the view
 * @param[in]	destroy		true - to swith to destroying state,
 * 							false - to not ective state
 */
static void _email_view_deactivate(email_view_t *view, bool destroy);

/**
 * @brief Handles show event of the email view
 *
 * @param[in]	view		pointer to the view
 */
static void _email_view_show(email_view_t *view);

/**
 * @brief Handles hide event of the email view
 *
 * @param[in]	view		pointer to the view
 */
static void _email_view_hide(email_view_t *view);

/**
 * @brief Updates email view
 *
 * @param[in]	view		pointer to the view
 */
static void _email_view_update(email_view_t *view);

/**
 * @brief Handles back key press event of the email view
 *
 * @param[in]	view		pointer to the view
 */
static void _email_view_on_back_key(email_view_t *view);

/**
 * @brief Pop callback handler of the email view
 *
 * @param[in]	data	pointer to the view item data
 * @param[in]	it		pointer to the view item
 *
 * @return Eina_True - allow pop of the view,
 * 		Eina_False - otherwise
 */

static Eina_Bool _email_view_pop_cb(void *data, Elm_Object_Item *it);

/**
 * @brief Email view item delete callback handler
 *
 * @param[in]	data		pointer to the user data
 * @param[in]	obj			pointer to evas object
 * @param[in]	event_info	pointer to event enfo
 */
static void _email_view_item_del_cb(void *data, Evas_Object *obj, void *event_info);

/**
 * @brief Naviframe back button click event handler
 *
 * @param[in]	data		pointer to the user data
 * @param[in]	obj			pointer to evas object
 * @param[in]	event_info	pointer to event enfo
 */
static void _email_view_back_btn_cb(void *data, Evas_Object *obj, void *event_info);

/**
 * utils
 */

#ifdef SHARED_MODULES_FEATURE
const char *_get_module_lib_name(email_module_type_e module_type)
{
	switch (module_type) {
	case EMAIL_MODULE_MAILBOX:	return "libemail-mailbox-module.so";
	case EMAIL_MODULE_VIEWER:	return "libemail-viewer-module.so";
	case EMAIL_MODULE_COMPOSER:	return "libemail-composer-module.so";
	case EMAIL_MODULE_ACCOUNT:	return "libemail-account-module.so";
	case EMAIL_MODULE_SETTING:	return "libemail-setting-module.so";
	case EMAIL_MODULE_FILTER:	return "libemail-filter-module.so";
	default:
		return NULL;
	}
}
#endif

/**
 * email_module_mgr
 */

int email_module_mgr_init(Evas_Object *win, Evas_Object *conform)
{
	debug_enter();
	/* Check state and input arguments */
	retvm_if(MODULE_MGR.initialized, -1, "Already initialized");
	retvm_if(!win, -1, "win is NULL");
	retvm_if(!conform, -1, "conform is NULL");

	/* Initialize window and conformant */
	MODULE_MGR.win = win;
	MODULE_MGR.conform = conform;

	/* Creating naviframe */
	Evas_Object *navi = elm_naviframe_add(conform);
	if (!navi) {
		debug_error("elm_naviframe_add(): failed");
		return -1;
	}
	MODULE_MGR.navi = navi;

	/* Set naviframe properties */
	evas_object_size_hint_weight_set(navi, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(navi, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_object_part_content_set(conform, "elm.swallow.content", navi);
	evas_object_show(navi);

	/* We do not need automatically pushed software back button*/
	elm_naviframe_prev_btn_auto_pushed_set(navi, EINA_FALSE);

	/* Registering naviframe callback listeners */

	evas_object_smart_callback_add(navi, "transition,finished",
			_email_module_mgr_transition_finished_cb, NULL);

	eext_object_event_callback_add(navi, EEXT_CALLBACK_BACK,
			_email_module_mgr_navi_back_cb, NULL);

	eext_object_event_callback_add(navi, EEXT_CALLBACK_MORE,
			eext_naviframe_more_cb, NULL);

	/* Initialize rest members */
	MODULE_MGR.paused = true;
	MODULE_MGR.initialized = true;

	debug_leave();
	return 0;
}

int email_module_mgr_finalize()
{
	debug_enter();
	/* Check state */
	retvm_if(!MODULE_MGR.initialized, -1, "Not initialized");

	/* Destroy root module if any */
	if (MODULE_MGR.root_module) {
		_email_module_destroy(MODULE_MGR.root_module, false);
	}

	/* Destroy naviframe */
	evas_object_del(MODULE_MGR.navi);

	/* Initialize module manager object to 0 */
	memset(&MODULE_MGR, 0, sizeof(MODULE_MGR));

	debug_leave();
	return 0;
}

int email_module_mgr_pause()
{
	debug_enter();
	/* Check state */
	retvm_if(!MODULE_MGR.initialized, -1, "Not initialized");

	/* Skip handling of event if already paused */
	if (!MODULE_MGR.paused) {
		/* Update paused state */
		MODULE_MGR.paused = true;

		/* Add EVUF_WAS_PAUSED to all views of the application */
		_email_module_mgr_add_view_ufs(EVUF_WAS_PAUSED);

		/* Deactivate and hide topmost view of the application */
		Elm_Object_Item *top_navi_item = elm_naviframe_top_item_get(MODULE_MGR.navi);
		if (top_navi_item) {
			email_view_t *view = elm_object_item_data_get(top_navi_item);
			if (view) {
				_email_view_deactivate(view, false);
				_email_view_hide(view);
			} else {
				debug_warning("view is NULL");
			}
		}

		/* Handle pause event by root module (if any) */
		if (MODULE_MGR.root_module) {
			_email_module_do_pause(MODULE_MGR.root_module);
		}
	}

	debug_leave();
	return 0;
}

int email_module_mgr_resume()
{
	debug_enter();
	/* Check state */
	retvm_if(!MODULE_MGR.initialized, -1, "Not initialized");

	/* Skip handling of event if not paused */
	if (MODULE_MGR.paused) {
		/* Update paused state */
		MODULE_MGR.paused = false;

		/* Handle resume event by root module (if any) */
		if (MODULE_MGR.root_module) {
			_email_module_do_resume(MODULE_MGR.root_module);
		}

		/* Activating topmost view of the application */
		Elm_Object_Item *top_navi_item = elm_naviframe_top_item_get(MODULE_MGR.navi);
		if (top_navi_item) {
			email_view_t *view = elm_object_item_data_get(top_navi_item);
			if (view) {
				/* We need to show and update the view befor activayion */
				_email_view_show(view);
				_email_view_update(view);
				/* If naviframe in transition skip actual activation
				 * (will be activate in transition finish callback)*/
				if (!MODULE_MGR.navi_in_transition) {
					_email_view_activate(view);
				}
			} else {
				debug_warning("view is NULL");
			}
		}
	}

	debug_leave();
	return 0;
}

int email_module_mgr_send_event(email_module_event_e event)
{
	debug_enter();
	/* Check state */
	retvm_if(!MODULE_MGR.initialized, -1, "Not initialized");

	/* Handle event by root module (if any) */
	if (MODULE_MGR.root_module) {
		_email_module_do_send_event(MODULE_MGR.root_module, event);
	}

	int update_flag = 0;

	/* Convert event to view update flag */
	switch (event) {
	case EM_EVENT_ROTATE_PORTRAIT:
	case EM_EVENT_ROTATE_PORTRAIT_UPSIDEDOWN:
	case EM_EVENT_ROTATE_LANDSCAPE:
	case EM_EVENT_ROTATE_LANDSCAPE_UPSIDEDOWN:
		update_flag = EVUF_ORIENTATION_CHANGED;
		break;
	case EM_EVENT_LANG_CHANGE:
		update_flag = EVUF_LANGUAGE_CHANGED;
		break;
	case EM_EVENT_REGION_CHANGE:
		update_flag = EVUF_REGION_FMT_CHANGED;
		break;
	default:
		update_flag = 0;
		break;
	}

	/* If update flag is not 0 */
	if (update_flag != 0) {
		/* Add this flag to all views of the application */
		_email_module_mgr_add_view_ufs(update_flag);

		/* Update topmost view immediately */
		Elm_Object_Item *top_navi_item = elm_naviframe_top_item_get(MODULE_MGR.navi);
		if (top_navi_item) {
			email_view_t *view = elm_object_item_data_get(top_navi_item);
			if (view) {
				_email_view_update(view);
			} else {
				debug_warning("view is NULL");
			}
		}
	}

	debug_leave();
	return 0;
}

email_module_h email_module_mgr_create_root_module(email_module_type_e module_type,
		email_params_h params, email_module_listener_t *listener)
{
	debug_enter();

	/* Creating root module by specifying parent with NULL */
	email_module_h module = _email_module_mgr_create_module(module_type, params, listener, NULL);

	debug_leave();
	return module;
}

bool email_module_mgr_is_in_transiton()
{
	debug_enter();
	return MODULE_MGR.navi_in_transition;
}

bool email_module_mgr_is_in_compressed_mode()
{
	debug_enter();
	/* Check state */
	retvm_if(!MODULE_MGR.initialized, false, "Not initialized");
	/* Return true if naviframe in compressed mode */
	return (evas_object_size_hint_display_mode_get(MODULE_MGR.navi) == EVAS_DISPLAY_MODE_COMPRESS);
}

#ifdef SHARED_MODULES_FEATURE
email_module_t *_email_module_mgr_alloc_module(email_module_type_e module_type)
{
	debug_enter();

	/* Get name of the module's shared library */
	const char *lib_name = _get_module_lib_name(module_type);
	if (!lib_name) {
		debug_error("Unknown module type: %d", module_type);
		return NULL;
	}

	/* We will call unload only if module was already loaded.
	 * So this logic will always keep 1 reference to the library. */
	bool unload = true;

	/* Try to obtain library handle without loading */
	void *lib_handle = dlopen(lib_name, RTLD_LAZY | RTLD_NOLOAD);
	if (!lib_handle) {
		/* Load library if previous operation fails*/
		lib_handle = dlopen(lib_name, RTLD_LAZY);
		if (!lib_handle) {
			debug_error("Failed to load module: %s (%s)", lib_name, dlerror());
			return NULL;
		}
		debug_log("Module loaded successfully: %s", lib_name);
		/* We must keep this handle opened. */
		unload = false;
	} else {
		debug_log("Module was already loaded: %s", lib_name);
	}

	/* Obtain pointer to the module's entry point function */
	alloc_func_t alloc_func = dlsym(lib_handle, EMAIL_MODULE_ALLOC_FUNC_NAME);

	/* Close library if needed */
	if (unload) {
		dlclose(lib_handle);
	}

	/* Check function pointer */
	if (!alloc_func) {
		debug_error("Module was not loaded: %d", module_type);
		return NULL;
	}

	/* Finally. Allocating module object using shared function */
	email_module_t *module = alloc_func();
	if (!module) {
		debug_error("Module allocation failed: %d", module_type);
		return NULL;
	}

	debug_leave();
	return module;
}
#else
email_module_t *mailbox_module_alloc();
email_module_t *viewer_module_alloc();
email_module_t *composer_module_alloc();
email_module_t *account_module_alloc();
email_module_t *setting_module_alloc();
email_module_t *filter_module_alloc();

email_module_t *_email_module_mgr_alloc_module(email_module_type_e module_type)
{
	switch (module_type) {
	case EMAIL_MODULE_MAILBOX:	return mailbox_module_alloc();
	case EMAIL_MODULE_VIEWER:	return viewer_module_alloc();
	case EMAIL_MODULE_COMPOSER:	return composer_module_alloc();
	case EMAIL_MODULE_ACCOUNT:	return account_module_alloc();
	case EMAIL_MODULE_SETTING:	return setting_module_alloc();
	case EMAIL_MODULE_FILTER:	return filter_module_alloc();
	default:
		debug_error("Unknown module type: %d", module_type);
		return NULL;
	}
}
#endif

email_module_h _email_module_mgr_create_module(email_module_type_e module_type, email_params_h params,
		email_module_listener_t *listener, email_module_t *parent)
{
	debug_enter();
	/* Check state and input arguments */
	retvm_if(!MODULE_MGR.initialized, NULL, "Not initialized");
	retvm_if(!params, NULL, "params is NULL");
	retvm_if(!listener, NULL, "listener is NULL");

	/* Staring with no parent */
	Elm_Object_Item *parent_navi_item = NULL;

	/* If parent specified - creating child module */
	if (parent) {
		/* Only topmost module can be a parent to a newly created modules */
		if (parent != MODULE_MGR.top_module) {
			debug_error("Parent must be topmost!");
			return NULL;
		}
		/* Can't create module on already destroying parent */
		if (parent->_state == EM_STATE_DESTROYING) {
			debug_error("Parent is in destroying state");
			return NULL;
		}
		/* Can't create a module on a parent without a view */
		if (parent->views_count == 0) {
			debug_error("Parent module must have a view!");
			return NULL;
		}
		/* Check if parent module already have a child.
		 * Current implementation does not allow multiple children. */
		if (parent->_children) {
			int c = eina_array_count(parent->_children);
			int i = 0;
			while (i < c) {
				email_module_t *sibling = eina_array_data_get(parent->_children, i);
				/* Only child in destroying state may coexist with the new module */
				if (sibling->_state != EM_STATE_DESTROYING) {
					debug_error("Sibling not in destroying state");
					return NULL;
				}
				/* Inherit parent naviframe item from a destroying sibling */
				parent_navi_item = sibling->_parent_navi_item;
				++i;
			}
		}
		/* If no naviframe parent item was inherited */
		if (!parent_navi_item) {
			/* Top item will be a parent */
			parent_navi_item = elm_naviframe_top_item_get(MODULE_MGR.navi);
		}
	/* Else - creating root module */
	} else if (MODULE_MGR.root_module) {
		/* Only 1 root module allowed */
		debug_error("Root module already created");
		return NULL;
	}

	/* Allocate a module object */
	email_module_t *module = _email_module_mgr_alloc_module(module_type);
	if (!module) {
		debug_error("Module (%d) allocation failed!", module_type);
		return NULL;
	}

	/* New module will become topmost */
	MODULE_MGR.top_module = module;

	/* Initialize allocated module */
	if (_email_module_init(module, params, listener, parent, parent_navi_item) != 0) {
		debug_error("Module (%d) initialization failed!", module_type);
		/* Reset parent and finalize invalid module */
		MODULE_MGR.top_module = parent;
		_email_module_finalize(module, false);
		return NULL;
	}

	debug_leave();
	return (email_module_h)module;
}

void _email_module_mgr_add_view_ufs(int flags)
{
	/* Iterate over each naviframe item */
	Eina_List *item = elm_naviframe_items_get(MODULE_MGR.navi);
	while (item) {
		email_view_t *view = elm_object_item_data_get(
				(Elm_Object_Item *)eina_list_data_get(item));
		if (view) {
			/* Add specified flag to the view's update flags */
			view->_update_flags |= flags;
		} else {
			debug_warning("view is NULL");
		}
		/* Iterate to the next item */
		item = eina_list_next(item);
	}
}

int _email_module_mgr_activate_new_top_view()
{
	debug_enter();

	/* Variable declarations */
	Elm_Object_Item *top_item = NULL;
	email_view_t *view = NULL;

	/* Get top naviframe item */
	top_item = elm_naviframe_top_item_get(MODULE_MGR.navi);
	if (!top_item) {
		debug_error("top_item is NULL");
		return -1;
	}

	/* Get item's data */
	view = elm_object_item_data_get(top_item);
	if (!view) {
		debug_error("prev_view is NULL");
		return -1;
	}

	/* We need to show the view before updating */
	_email_view_show(view);
	/* Set necessary update flag and updating the view */
	view->_update_flags |= EVUF_POPPING;
	_email_view_update(view);
	/* Activating the view */
	_email_view_activate(view);

	debug_leave();
	return 0;
}

int _email_module_mgr_exit_view(Elm_Object_Item *view_item)
{
	debug_enter();

	/* Variable declarations */
	Elm_Object_Item *top_item = NULL;
	Eina_List *top_list_item = NULL;

	/* Get top naviframe item */
	top_item = elm_naviframe_top_item_get(MODULE_MGR.navi);
	if (!top_item) {
		debug_error("top_item is NULL");
		return -1;
	}

	/* If specified view is not topmost or
	 * application is paused without a transition */
	if ((view_item != top_item) || (MODULE_MGR.paused && !MODULE_MGR.navi_in_transition)) {
		debug_log("View can be destroyed immediately.");

		/* Delete naviframe item without any transition effect */
		elm_object_item_del(view_item);

		/* If specified view is topmost */
		if (view_item == top_item) {
			/* Activating new topmost view immediately */
			int r = _email_module_mgr_activate_new_top_view();
			if (r != 0) {
				debug_warning("_email_module_mgr_activate_top_view() failed: %d", r);
				return 0;
			}
		}

		/* If we deleting view to which we are currently going to pop */
		if (view_item == MODULE_MGR.pop_to_item) {
			/* We should pop to the previous view after this */
			top_list_item = eina_list_data_find_list(elm_naviframe_items_get(MODULE_MGR.navi), view_item);
		}

	/* Else - we pop the view */
	} else {
		/* Get top list item so that we can get to the previous item */
		top_list_item = eina_list_last(elm_naviframe_items_get(MODULE_MGR.navi));
	}

	/* If we need to perform pop to */
	if (top_list_item) {
		int r = 0;
		/* Get previous item after top item */
		Elm_Object_Item *prev_item = eina_list_data_get(eina_list_prev(top_list_item));
		if (!prev_item) {
			debug_error("prev_item is NULL");
			return -1;
		}

		/* Scheduling pop to the previous item */
		r = _email_module_mgr_pop_view_to(prev_item);
		if (r != 0) {
			debug_error("_email_module_mgr_pop_view() failed: %d", r);
			return -1;
		}
	}

	/* Rest pushed_in_item if it was deleted/popped */
	if (view_item == MODULE_MGR.pushed_in_item) {
		MODULE_MGR.pushed_in_item = NULL;
	}

	debug_leave();
	return 0;
}

int _email_module_mgr_pop_view_to(Elm_Object_Item *view_item)
{
	debug_enter();

	/* Create pop-job if not created and naviframe not in transition.
	 * If naviframe in transition job will be created later in transition finish callback */
	if (!MODULE_MGR.pop_job && !MODULE_MGR.navi_in_transition) {
		MODULE_MGR.pop_job = ecore_job_add(_email_module_mgr_pop_job_cb, NULL);
		if (!MODULE_MGR.pop_job) {
			debug_error("Failed to create pop_job!");
			return -1;
		}
	}

	/* Store traget item. */
	MODULE_MGR.pop_to_item = view_item;

	debug_leave();
	return 0;
}

void _email_module_mgr_transition_finished_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	/* Do not handle this callback after popping to item.
	 * Wait for destroy callback of the popped item. */
	if (MODULE_MGR.popped_item) {
		debug_log("Previously popped item is still there. Wait for item deletion..");
		MODULE_MGR.is_delayed_transition = true;
		return;
	}

	/* Reset transition flag */
	MODULE_MGR.navi_in_transition = false;

	/* Start pop-job if there are pending pop_to_item */
	if (MODULE_MGR.pop_to_item) {
		if (!MODULE_MGR.pop_job) {
			debug_log("pop_to_item is not NULL. Starting Pop job...");
			MODULE_MGR.pop_job = ecore_job_add(_email_module_mgr_pop_job_cb, NULL);
		} else {
			debug_warning("Pop job should not be created here!");
		}
		/* Just reset state. We can skip hiding of this view because
		 * it will be shown/destroyed in the next pop-job callback */
		MODULE_MGR.pushed_in_item = NULL;
		return;
	}

	/* Hide the item that was pushed in by newly pushed view */
	if (MODULE_MGR.pushed_in_item) {
		email_view_t *view = elm_object_item_data_get(MODULE_MGR.pushed_in_item);
		if (view) {
			_email_view_hide(view);
		} else {
			debug_warning("view is NULL");
		}
		MODULE_MGR.pushed_in_item = NULL;
	}

	/* Get current topmost view */
	Elm_Object_Item *top_navi_item = elm_naviframe_top_item_get(MODULE_MGR.navi);
	if (!top_navi_item) {
		debug_warning("Naviframe is empty");
		return;
	}

	/* Activating topmost view */
	email_view_t *view = elm_object_item_data_get(top_navi_item);
	if (view) {
		_email_view_activate(view);
	} else {
		debug_warning("view is NULL");
	}

	debug_leave();
}

void _email_module_mgr_navi_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	/* Skip hardware back key presses if in transition */
	if (MODULE_MGR.navi_in_transition) {
		debug_log("Transition in progress - skip");
		return;
	}

	/* Get top naviframe item */
	Elm_Object_Item *top_navi_item = elm_naviframe_top_item_get(MODULE_MGR.navi);
	if (!top_navi_item) {
		debug_warning("Naviframe is empty");
		return;
	}

	/* Get view data for the item */
	email_view_t *view = elm_object_item_data_get(top_navi_item);
	if (view) {
		/* Handling back key press by the topmost view */
		_email_view_on_back_key(view);
	} else {
		debug_warning("view is NULL");
	}

	debug_leave();
}

void _email_module_mgr_pop_job_cb(void *data)
{
	debug_enter();

	/* Inside the loop pop_to_item may change,
	 * so we will iterate until it will no change */
	while (MODULE_MGR.pop_to_item) {
		/* Save current pop item upon loop entry */
		Elm_Object_Item *cur_item = MODULE_MGR.pop_to_item;
		email_view_t *cur_view = elm_object_item_data_get(cur_item);
		if (cur_view) {
			/* Handling view show event */
			_email_view_show(cur_view);
			/* Skip current loop if new pop was performed inside the handler */
			if (cur_item != MODULE_MGR.pop_to_item) {
				continue;
			}
			/* Updating view */
			cur_view->_update_flags |= EVUF_POPPING;
			_email_view_update(cur_view);
			/* Skip if changed */
			if (cur_item != MODULE_MGR.pop_to_item) {
				continue;
			}
		} else {
			debug_warning("view is NULL");
		}
		/* Pop to item was not changed after show and update*/
		break;
	}

	/* If we still need to pop */
	if (MODULE_MGR.pop_to_item) {
		/* Get top item so we can deactivate its view */
		Elm_Object_Item *top_item = elm_naviframe_top_item_get(MODULE_MGR.navi);
		if (top_item) {

			email_view_t *top_view = elm_object_item_data_get(top_item);
			if (top_view) {
				_email_view_deactivate(top_view, true);
				/* Remove pop-block guard */
				top_view->_allow_pop = true;
			} else {
				debug_warning("top_view is NULL");
			}

			/* Perform actual */
			elm_naviframe_item_pop_to(MODULE_MGR.pop_to_item);
			/* Save popped item so we can control its life time */
			MODULE_MGR.popped_item = top_item;
			/* Update navifram transition state */
			MODULE_MGR.navi_in_transition = true;
		} else {
			debug_error("top_item is NULL");
		}
		/* No need to pop */
		MODULE_MGR.pop_to_item = NULL;
	}

	/* Job is complete */
	MODULE_MGR.pop_job = NULL;

	debug_leave();
}

/**
 * email_module
 */

int _email_module_init(email_module_t *module, email_params_h params,
		email_module_listener_t *listener, email_module_t *parent, Elm_Object_Item *parent_navi_item)
{
	debug_enter();
	/* Check state */
	retvm_if(!module->create, -1, "module->create is NULL");

	/* Initialize members */

	module->win = MODULE_MGR.win;
	module->conform = MODULE_MGR.conform;
	module->navi = MODULE_MGR.navi;

	module->_listener = *listener;
	module->_parent = parent;

	module->_state = EM_STATE_RUNNING;

	module->_parent_navi_item = parent_navi_item;

	/* Initialize launcher sub object */
	_email_module_handle_launcher_initialize(module);

	/* Initialize concrete module */
	int r = module->create(module, params);
	if (r != 0) {
		debug_error("module->create(): failed (%d)", r);
		return -1;
	}

	/* If module has not parent is is becomes a root module */
	if (!parent) {
		MODULE_MGR.root_module = module;
	/* Else - adding module as child to the parent */
	} else if (_email_module_add_child(parent, module) != 0) {
		debug_error("Failed to register module in the perant's array");
		return -1;
	}

	debug_leave();
	return 0;
}

int _email_module_add_child(email_module_t *module, email_module_t *child)
{
	/* lazy initialization of module's children array */
	if (!module->_children) {
		module->_children = eina_array_new(EMAIL_MODULE_CHILDREN_ARRAY_STEP);
		if (!module->_children) {
			debug_error("Failed to create children array");
			return -1;
		}
	}
	/* Pushing child module to the array */
	if (!eina_array_push(module->_children, child)) {
		debug_error("Failed to add child module");
		return -1;
	}
	return 0;
}

Eina_Bool _email_module_keep_all_except_cb(void *cur_module, void *except_module)
{
	/* We will return false for all module except "except_module" */
	return (cur_module != except_module);
}

void _email_module_finalize(email_module_t *module, bool skip_views_cleanup)
{
	debug_enter();

	/* Finalizing launcher sub object */
	_email_module_handle_launcher_finalize(module);

	/* Cleanup view if no need to skip */
	if (!skip_views_cleanup) {
		_email_module_cleanup_views(module);
	}

	if (module->_parent) {
		/* Remove current module from parent if it is a child module */
		eina_array_remove(module->_parent->_children,
				_email_module_keep_all_except_cb, module);
	} else {
		/* Or NULL-out root_module if it is a root module */
		MODULE_MGR.root_module = NULL;
	}

	/* Break connections with children */
	if (module->_children) {
		int c = eina_array_count(module->_children);
		int i = 0;
		while (i < c) {
			email_module_t *child = eina_array_data_get(module->_children, i);
			child->_parent = NULL;
			++i;
		}
		eina_array_free(module->_children);
	}

	/* Destroy module with virtual function*/
	if (module->destroy) {
		module->destroy(module);
	}

	/* Notify listener */
	if ((module->_state == EM_STATE_DESTROYING) && (module->_listener.destroy_cb)) {
		module->_listener.destroy_cb(module->_listener.cb_data, (email_module_h)module);
	}

	free(module);

	debug_leave();
}

void _email_module_cleanup_views(email_module_t *module)
{
	debug_enter();

	/* Start from top item */
	Elm_Object_Item *it = elm_naviframe_top_item_get(MODULE_MGR.navi);

	/* Destroy item that s currently popping */
	if (MODULE_MGR.popped_item) {
		Elm_Object_Item *popped_item = MODULE_MGR.popped_item;
		MODULE_MGR.popped_item = NULL;
		elm_object_item_del(popped_item);
	}

	/* If there are views to destroy */
	if (it != module->_parent_navi_item) {
		debug_log("Cleaning up views...");

		/* Cancelling pop-job if any */
		if (MODULE_MGR.pop_job) {
			ecore_job_del(MODULE_MGR.pop_job);
			MODULE_MGR.pop_job = NULL;
		}

		/* Reset state */
		MODULE_MGR.pop_to_item = NULL;
		MODULE_MGR.pushed_in_item = NULL;
		MODULE_MGR.navi_in_transition = false;

		/* Reset number of views */
		module->views_count = 0;

		/* While there are views to destroy */
		while (it != module->_parent_navi_item) {
			/* Destroy view */
			elm_object_item_del(it);
			it = elm_naviframe_top_item_get(MODULE_MGR.navi);
		}

		/* Activate new top view */
		if (module->_parent_navi_item) {
			_email_module_mgr_activate_new_top_view();
		}
	}

	debug_leave();
}

void _email_module_mark_for_destroy_cb(email_module_t *module, void *cb_data)
{
	/* Set destroying state */
	module->_state = EM_STATE_DESTROYING;
	/* Unregister listeners from all children modules except currently destroying module */
	if (module != cb_data) {
		module->_listener.destroy_cb = NULL;
		module->_listener.result_cb = NULL;
	}
}

int _email_module_destroy(email_module_t *module, bool use_pop)
{
	debug_enter();

	/* Skip destroy if already destroying */
	if (module->_state == EM_STATE_DESTROYING) {
		debug_error("Module is already destroying.");
		return -1;
	}

	/* Mark module tree for destroying and unregister listeners */
	_email_module_foreach_module(module, _email_module_mark_for_destroy_cb, module);

	/* Parent is a new topmost module */
	MODULE_MGR.top_module = module->_parent;

	/* If root module of not need to use pop while not in transition */
	if (!module->_parent || (!use_pop && !MODULE_MGR.navi_in_transition)) {
		/* Finalize module immediately */
		_email_module_finalize(module, false);
	/* If there is parent naviframe item */
	} else if (module->_parent_navi_item) {
		/* Pop to this item */
		_email_module_mgr_pop_view_to(module->_parent_navi_item);
	} else {
		/* This case should not happen */
		debug_warning("module->_parent_navi_item is NULL");
	}

	debug_leave();
	return 0;
}

int email_module_destroy(email_module_h module_h)
{
	debug_enter();
	/* Check input arguments */
	retvm_if(!module_h, -1, "module_h is NULL");
	/* Destroy the module using pop if not in pause state */
	return _email_module_destroy((email_module_t *)module_h, !MODULE_MGR.paused);
}

int email_module_destroy_no_pop(email_module_h module_h)
{
	debug_enter();
	/* Check input arguments */
	retvm_if(!module_h, -1, "module_h is NULL");
	/* Destroy the module without pop */
	return _email_module_destroy((email_module_t *)module_h, false);
}

void _email_module_pause_cb(email_module_t *module, void *cb_data)
{
	/* If module in running state */
	if (module->_state == EM_STATE_RUNNING) {
		/* Switch it to the paused state */
		module->_state = EM_STATE_PAUSED;

		/* Handle pause event by launcher sub object */
		_email_module_handle_launcher_pause(module);

		/* Handle pause event by concrete implementation of the module */
		if (module->pause) {
			module->pause(module);
		}
	}
}

void _email_module_do_pause(email_module_t *module)
{
	/* Skip if module not running */
	if (module->_state == EM_STATE_RUNNING) {
		/* Handle pause event by each module in the tree */
		_email_module_foreach_module(module, _email_module_pause_cb, NULL);
	}
}

void _email_module_resume_cb(email_module_t *module, void *cb_data)
{
	/* If module in paused state */
	if (module->_state == EM_STATE_PAUSED) {
		/* Switch it to the running state */
		module->_state = EM_STATE_RUNNING;

		/* Handle resume event by launcher sub object */
		_email_module_handle_launcher_resume(module);

		/* Handle resume event by concrete implementation of the module */
		if (module->resume) {
			module->resume(module);
		}
	}
}

void _email_module_do_resume(email_module_t *module)
{
	/* Skip if module not paused */
	if (module->_state == EM_STATE_PAUSED) {
		/* Handle resume event by each module in the tree */
		_email_module_foreach_module(module, _email_module_resume_cb, NULL);
	}
}

void _email_module_send_event_cb(email_module_t *module, void *cb_data)
{
	/* If module is not destroying */
	if (module->_state != EM_STATE_DESTROYING) {
		/* Handle event by concrete implementation of the module */
		if (module->on_event) {
			module->on_event(module, (email_module_event_e)cb_data);
		}
	}
}

void _email_module_do_send_event(email_module_t *module, email_module_event_e event)
{
	/* If module is not destroying */
	if (module->_state != EM_STATE_DESTROYING) {
		/* Handle event by each module in the tree */
		_email_module_foreach_module(module, _email_module_send_event_cb, (void *)event);
	}
}

int email_module_send_message(email_module_h module_h, email_params_h msg)
{
	debug_enter();
	/* Check input arguments */
	retvm_if(!module_h, -1, "module_h is NULL");
	retvm_if(!msg, -1, "msg is NULL");

	/* Convert handle to structure */
	email_module_t *module = (email_module_t *)module_h;

	/* Generate error if in destroying state */
	if (module->_state == EM_STATE_DESTROYING) {
		debug_error("Module is in destroying state.");
		return -1;
	}
	/* Generate error if module does not implement this functionality */
	if (!module->on_message) {
		debug_error("on_message is NULL.");
		return -1;
	}
	/* Invoke module message handler */
	module->on_message(module, msg);

	debug_leave();
	return 0;
}

int email_module_send_result(email_module_t *module, email_params_h result)
{
	debug_enter();
	/* Check input arguments */
	retvm_if(!module, -1, "module is NULL");
	retvm_if(!result, -1, "result is NULL");

	/* Skip this function if listener not subscribed for this event */
	if (!module->_listener.result_cb) {
		debug_log("result_cb is NULL.");
		return 0;
	}
	/* Invoke callback listener */
	module->_listener.result_cb(module->_listener.cb_data, (email_module_h)module, result);

	debug_leave();
	return 0;
}

int email_module_make_destroy_request(email_module_t *module)
{
	debug_enter();
	/* Check input arguments */
	retvm_if(!module, -1, "module is NULL");

	/* Generate error if in destroying state */
	if (module->_state == EM_STATE_DESTROYING) {
		debug_error("Module is in destroying state.");
		return -1;
	}

	/* Send destroy request using private function */
	int r = _email_module_make_destroy_request(module);

	debug_leave();
	return r;
}

int _email_module_make_destroy_request(email_module_t *module)
{
	debug_enter();

	/* Generate error if listener not subscribed for this event */
	if (!module->_listener.destroy_request_cb) {
		debug_error("destroy_request_cb is NULL.");
		return -1;
	}
	/* Invoke callback listener */
	module->_listener.destroy_request_cb(module->_listener.cb_data, (email_module_h)module);

	debug_leave();
	return 0;
}

email_module_h email_module_create_child(email_module_t *module, email_module_type_e module_type,
		email_params_h params, email_module_listener_t *listener)
{
	debug_enter();
	/* Check input arguments */
	retvm_if(!module, NULL, "module is NULL");

	/* Creating child module by specifying parent with the caller module */
	email_module_h child = _email_module_mgr_create_module(module_type, params, listener, module);

	debug_leave();
	return child;
}

int email_module_create_view(email_module_t *module, email_view_t *view)
{
	debug_enter();
	/* Check input arguments */
	retvm_if(!module, -1, "module is NULL");
	retvm_if(!view, -1, "view is NULL");

	/* Only topmost module may create a view */
	if (module != MODULE_MGR.top_module) {
		debug_error("Module must be topmost!");
		return -1;
	}

	/* Destroying module can't create new views */
	if (module->_state == EM_STATE_DESTROYING) {
		debug_error("Module is in destroying state.");
		return -1;
	}

	/* Get top naviframe item before initializing the view */
	Elm_Object_Item *top_navi_item = (MODULE_MGR.pop_to_item ? NULL :
			elm_naviframe_top_item_get(MODULE_MGR.navi));

	/* Initializing the view */
	int r = _email_view_init(view, module);
	if (r != 0) {
		debug_error("View initialization failed!");
		/* Finalize invalid view in case of an error */
		_email_view_finalize(view);
		return -1;
	}

	/* If there is no item to pop to */
	if (!MODULE_MGR.pop_to_item) {
		/* If there is was top view before this one */
		if (top_navi_item) {
			/* Deactivating this view */
			email_view_t *top_view = elm_object_item_data_get(top_navi_item);
			if (top_view) {
				_email_view_deactivate(top_view, false);
			} else {
				debug_warning("top_view is NULL");
			}

			/* If new view was pushed without no transition flag */
			if ((view->_push_flags & EVPF_NO_TRANSITION) == 0) {
				/* If there is already a pushed in item */
				if (MODULE_MGR.pushed_in_item) {
					/* Hide it immediately */
					email_view_t *view = elm_object_item_data_get(MODULE_MGR.pushed_in_item);
					if (view) {
						_email_view_hide(view);
					} else {
						debug_warning("view is NULL");
					}
				}
				/* Save pushed item so we can hide it in transition finish calback */
				MODULE_MGR.pushed_in_item = top_navi_item;
				MODULE_MGR.navi_in_transition = true;
			}
		}

		/* If naviframe not in transition and not paused activate view immediately */
		if (!MODULE_MGR.navi_in_transition && !MODULE_MGR.paused) {
			_email_view_activate(view);
		}
	}

	debug_leave();
	return 0;
}

int email_module_exit_view(email_view_t *view)
{
	debug_enter();
	/* Check input arguments */
	retvm_if(!view, -1, "view is NULL");

	int r = 0;

	/* Can't exit from destroying view */
	if (view->state == EV_STATE_DESTROYING) {
		debug_error("View is in destroying state.");
		return -1;
	}

	/* Can't exit from view in destroying module */
	if (view->module->_state == EM_STATE_DESTROYING) {
		debug_error("Module is in destroying state.");
		return -1;
	}

	/* All views already destroyed. */
	if (view->module->views_count == 0) {
		debug_error("Module's views count is 0");
		return -1;
	}

	/* If last view exits - make implicit destroy request */
	if (view->module->views_count == 1) {
		debug_log("It is a last view. Making destroy request...");
		r = _email_module_make_destroy_request(view->module);
	/* Otherwise perform regular procedure */
	} else {
		debug_log("Exit the view...");
		r = _email_module_mgr_exit_view(view->navi_item);
	}

	if (r != 0) {
		debug_error("Operation fails: %d", r);
		return -1;
	}

	/* Switch of the view to destroying */
	view->state = EV_STATE_DESTROYING;

	debug_leave();
	return 0;
}

Elm_Object_Item *email_module_view_push(email_view_t *view, const char *title_label, int flags)
{
	debug_enter();
	/* Check input arguments */
	retvm_if(!view, NULL, "view is NULL");

	Elm_Object_Item *after_item = NULL;

	/* Can't push a view in destroying module */
	if (view->module->_state == EM_STATE_DESTROYING) {
		debug_error("Module is in destroying state.");
		return NULL;
	}

	/* View must be in created state to be able to push itself */
	if (view->state != EV_STATE_CREATED) {
		debug_error("View not in created state.");
		return NULL;
	}

	/* Content of the view must be created before push */
	if (!view->content) {
		debug_error("view->content is NULL.");
		return NULL;
	}

	/* Not allow to push more than 1 view */
	if (view->navi_item) {
		debug_error("view->navi_item is not NULL.");
		return NULL;
	}

	/* If there is pop to item we will insert new view after it */
	if (MODULE_MGR.pop_to_item) {
		after_item = MODULE_MGR.pop_to_item;
	/* If no transition and no transition flag set or in pause*/
	} else if (!MODULE_MGR.navi_in_transition && (MODULE_MGR.paused || ((flags & EVPF_NO_TRANSITION) != 0))) {
		/* Use insert after top to disble transition. */
		after_item = elm_naviframe_top_item_get(view->module->navi);
	}

	/* Initialize actual push flags */
	view->_push_flags = flags;

	/* If need to use insert instead of push*/
	if (after_item) {
		/* Add no transition flag to view push flags */
		view->_push_flags |= EVPF_NO_TRANSITION;
		/* Insert view to the naviframe */
		view->navi_item = elm_naviframe_item_insert_after(
				view->module->navi, after_item, title_label, NULL, NULL, view->content, NULL);
	} else {
		/* Remove no transition flag to view push flags */
		view->_push_flags &= ~EVPF_NO_TRANSITION;
		/* Pop view to the naviframe */
		view->navi_item = elm_naviframe_item_push(
				view->module->navi, title_label, NULL, NULL, view->content, NULL);
	}

	/* Handle error case */
	if (!view->navi_item) {
		debug_error("Failed to push/insert naviframe item!");
		return NULL;
	}

	/* If there are was pop to item niew view bacomes new pop to item */
	if (MODULE_MGR.pop_to_item) {
		MODULE_MGR.pop_to_item = view->navi_item;
	}

	/* If software back button requested */
	if ((flags & EVPF_NO_BACK_BUTTON) == 0) {
		/* Create back button */
		Evas_Object *back_btn = elm_button_add(view->module->navi);
		if (back_btn) {
			elm_object_style_set(back_btn, "naviframe/back_btn/default");
			elm_object_item_part_content_set(view->navi_item, "prev_btn", back_btn);
			evas_object_show(back_btn);

			/* Register software back button click callback */
			evas_object_smart_callback_add(back_btn, "clicked", _email_view_back_btn_cb, view);
		} else {
			debug_warning("Back button create failed!");
		}
	}

	/* Enable title if requested */
	elm_naviframe_item_title_enabled_set(view->navi_item, ((flags & EVPF_NO_TITLE) == 0), EINA_FALSE);

	/* Set item data and register item callbacks */
	elm_object_item_data_set(view->navi_item, view);
	elm_naviframe_item_pop_cb_set(view->navi_item, _email_view_pop_cb, view);
	elm_object_item_del_cb_set(view->navi_item, _email_view_item_del_cb);

	debug_leave();
	return view->navi_item;
}

Eina_Bool _email_module_each_module_cb(const void *container, void *data, void *fdata)
{
	email_module_t *module = data;
	each_module_cb_data_t *each_data = fdata;

	/* Recursively calling function to iterate over module's children */
	_email_module_foreach_module_impl(module, each_data);

	return EINA_TRUE;
}

void _email_module_foreach_module_impl(email_module_t *module, each_module_cb_data_t *each_data)
{
	/* Iterate over children of the module if there is such */
	if (module->_children) {
		eina_array_foreach(module->_children, _email_module_each_module_cb, each_data);
	}
	/* Invoke use callback for current item */
	each_data->cb(module, each_data->cb_data);
}

void _email_module_foreach_module(email_module_t *module, email_module_each_cb_t cb, void *cb_data)
{
	/* Fill structure */
	each_module_cb_data_t each_data;
	each_data.cb = cb;
	each_data.cb_data = cb_data;
	/* Initial invocation of the recursive function */
	_email_module_foreach_module_impl(module, &each_data);
}

/**
 * email_view
 */

int _email_view_init(email_view_t *view, email_module_t *module)
{
	debug_enter();
	/* Check view state */
	retvm_if(!view->create, -1, "view->create is NULL");

	/* Increment view count of the module */
	++module->views_count;

	/* Initialize view members */

	view->module = module;

	view->state = EV_STATE_CREATED;
	view->visible = true;

	view->orientation = (app_device_orientation_e)elm_win_rotation_get(module->win);

	/* Create concrete view */
	int r = view->create(view);
	if (r != 0) {
		debug_error("view->create(): failed.");
		return -1;
	}

	/* Content must be create inside create() method */
	if (!view->content) {
		debug_error("view->content is NULL after view->create().");
		return -1;
	}

	/* This will prevent content from deletion by naviframe item */
	evas_object_ref(view->content);

	/* Naviframe item must be pushed inside create() method */
	if (!view->navi_item) {
		debug_error("Frame view is not supported. You must push a naviframe item");
		return -1;
	}

	debug_leave();
	return 0;
}

void _email_view_finalize(email_view_t *view)
{
	debug_enter();

	/* We must save this members to local variables because of view destroy */
	Evas_Object *content = view->content;
	email_module_t *module = view->module;

	/* Destroy naviframe item */
	if (view->navi_item) {
		/* Unset destroy callback*/
		elm_object_item_del_cb_set(view->navi_item, NULL);
		elm_object_item_del(view->navi_item);
		view->navi_item = NULL;
	}

	/* Destroy view data (may be freed inside) */
	if (view->destroy) {
		view->destroy(view);
	}

	/* Destroy content of the view */
	if (content) {
		evas_object_del(content);
		evas_object_unref(content);
	}

	/* Decrement view count */
	if (module->views_count > 0) {
		--module->views_count;
		/*If it is a last view - finalize module */
		if (module->views_count == 0) {
			_email_module_finalize(module, true);
		}
	}

	debug_leave();
}

void _email_view_activate(email_view_t *view)
{
	debug_enter();

	/* Only created or not active view may be activated */
	if ((view->state == EV_STATE_CREATED) || (view->state == EV_STATE_NOT_ACTIVE)) {
		/* Save old state */
		email_view_state prev_state = view->state;
		/* Set new state */
		view->state = EV_STATE_ACTIVE;

		/* Handle activate event if implemented */
		if (view->activate) {
			debug_log("Activating");
			view->activate(view, prev_state);
		}
	}

	debug_leave();
}

void _email_view_deactivate(email_view_t *view, bool destroy)
{
	debug_enter();

	/* Only active view may be deactivated */
	if (view->state == EV_STATE_ACTIVE) {
		/* Set new state */
		view->state = (destroy ? EV_STATE_DESTROYING : EV_STATE_NOT_ACTIVE);

		/* If new state is destroying - finalize launcher */
		if (destroy) {
			_email_module_handle_launcher_finalize(view->module);
		}

		/* Handle deactivate event if implemented */
		if (view->deactivate) {
			debug_log("Deactivating");
			view->deactivate(view);
		}
	}

	debug_leave();
}

void _email_view_show(email_view_t *view)
{
	debug_enter();

	/* Only not visible and not destroying view can be shown */
	if (!view->visible && (view->state != EV_STATE_DESTROYING)) {
		view->visible = true;

		/* Handle show event if implemented */
		if (view->show) {
			debug_log("Showing");
			view->show(view);
		}
	}

	debug_leave();
}

void _email_view_hide(email_view_t *view)
{
	debug_enter();

	/* Only visible and not destroying view can be hidden */
	if (view->visible && (view->state != EV_STATE_DESTROYING)) {
		view->visible = false;

		/* Handle show event if implemented */
		if (view->hide) {
			debug_log("Hiding");
			view->hide(view);
		}
	}

	debug_leave();
}

void _email_view_update(email_view_t *view)
{
	debug_enter();

	/* Only visible with update flags and not destroying view can be updated */
	if (view->visible && (view->_update_flags != 0) && (view->state != EV_STATE_DESTROYING)) {
		/* Update orientation member if there is orientation change flag */
		if (view->_update_flags & EVUF_ORIENTATION_CHANGED) {
			app_device_orientation_e orientation =
					(app_device_orientation_e)elm_win_rotation_get(view->module->win);
			if (orientation != view->orientation) {
				view->orientation = orientation;
			} else {
				/* Remove flag if orientation was not changed */
				view->_update_flags &= ~EVUF_ORIENTATION_CHANGED;
			}
		}

		/* If update flags is still not 0 */
		if (view->_update_flags != 0) {
			/* Perform update if implemented */
			if (view->update) {
				debug_log("Updating: flags = %d", view->_update_flags);
				view->update(view, view->_update_flags);
			}

			/* Reset update flags */
			view->_update_flags = 0;
		}
	}

	debug_leave();
}

void _email_view_on_back_key(email_view_t *view)
{
	debug_enter();

	/* Only active view can handle back key press event */
	if (view->state == EV_STATE_ACTIVE) {
		/* Handle back key press event if implemented */
		if (view->on_back_key) {
			debug_log("Handling back key press");
			view->on_back_key(view);
		}
	}

	debug_leave();
}

Eina_Bool _email_view_pop_cb(void *data, Elm_Object_Item *it)
{
	debug_enter();
	retvm_if(!data, EINA_FALSE, "data is NULL");
	email_view_t *view = data;

	/* Block undesired pop event.
	 * Only pop from this code will work. */
	if (!view->_allow_pop) {
		debug_error("Popping this view is not allowed!");
		return EINA_FALSE;
	}

	debug_leave();
	return EINA_TRUE;
}

void _email_view_item_del_cb(void *data, Evas_Object *obj, void *event_info)
{
	/* TODO On Tizen 2.4 data argument is NULL when event_info(item)
	 * is not NULL and it's data is also not NULL
	 * Need to assign an issue to the UIFW and after the fix remove this code
	 */

	debug_enter();
	retm_if(!data && !event_info, "data and event_info are NULL");
	debug_warning_if(!data, "data is NULL");

	/* If deleted item is popped item */
	if (MODULE_MGR.popped_item == event_info) {
		MODULE_MGR.popped_item = NULL;
		/* If transition finish was before this callback */
		if (MODULE_MGR.is_delayed_transition) {
			MODULE_MGR.is_delayed_transition = false;
			/* Handle this event here. */
			_email_module_mgr_transition_finished_cb(NULL, NULL, NULL);
		}
	}

	/* Get view data */
	email_view_t *view = data ? data : elm_object_item_data_get(event_info);
	retm_if(!view, "view is NULL");

	/* NULL naviframe item to prevent double deletion */
	view->navi_item = NULL;

	/* Finalize the view */
	_email_view_finalize(view);

	debug_leave();
}

void _email_view_back_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");
	email_view_t *view = data ;

	/* Handle software back key press */
	_email_view_on_back_key(view);

	debug_leave();
}
