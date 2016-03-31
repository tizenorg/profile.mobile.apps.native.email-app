#include "email-module-dev.h"

#include <attach_panel.h>

#include "email-debug.h"

#undef LOG_TAG
#define LOG_TAG "EMAIL_MODULE_LAUNCHER"

/**
 * Maximum timeout before considering transparent application as closed
 */
#define EMAIL_MODULE_APP_TIMER_TIMEOUT_SEC 3.0

/**
 * Array to convert application type to the corresponding application id
 */
static const char *EMAIL_LAUNCH_APP_NAMES[EMAIL_LAUNCH_APP_COUNT] = {
		NULL, /* auto */
		"setting-ringtone-efl",
		"setting-storage-efl"
};

/**
 * @brief Terminates any launched application or attach panel
 *
 * This method must be called as part of the finialization process of the launcher.
 *
 * @param[in]	module			pointer to the module
 * @param[in]	notify_close	true - need to notify listener on close, false - otherwise
 */
static void _email_module_terminate_any_launched_app(email_module_t *module, bool notify_close);

/**
 * @brief Resets internal state of the launcher after closing of the launched application
 *
 * After this method call new application may be launched.
 * Used inside _email_module_terminate_any_launched_app() method.
 *
 * @param[in]	module			pointer to the module
 * @param[in]	notify_close	true - need to notify listener, false - otherwise
 * @param[in]	reset_listener	true - need to reset listener, false - otherwise
 */
static void _email_module_reset_app_launch_state(email_module_t *module, bool notify_close, bool reset_listener);

/**
 * @brief Frees attach panel if it was created
 *
 * @param[in]	module			pointer to the module
 */
static void _email_module_free_attach_panel(email_module_t *module);

/**
 * @brief Frees all attach panel category extra data bundles
 *
 * @param[in]	module			pointer to the module
 */
static void _email_module_free_attach_panel_bundles(email_module_t *module);

/**
 * @brief Frees specified attach panel category extra data bundle
 *
 * @param[in]	module			pointer to the module
 * @param[in]	category_type	category of the bundle to free
 */
static void _email_module_free_attach_panel_bundle(email_module_t *module,
		email_attach_panel_category_type_e category_type);

/**
 * @brief Handler method of the launched application reply callback
 *
 * @param[in]	request		request that was send to the launched application
 * @param[in]	reply		reply that was send back from this application
 * @param[in]	result		result status of the request send back by the application
 * @param[in]	user_data	pointer to the callback user data
 */
static void _email_module_app_reply_cb(app_control_h request, app_control_h reply,
		app_control_result_e result, void *user_data);

/**
 * @brief Callback of the transparent application close wait timer
 *
 * @param[in]	data	pointer to the callback user data
 *
 * @return ECORE_CALLBACK_RENEW - if timer should continue ticking,
 * 		ECORE_CALLBACK_CANCEL - otherwise
 */
static Eina_Bool _email_module_app_timer_cb(void *data);

/**
 * @brief Creates attach panel with categories and registered callbacks
 *
 * @param[in]	module			pointer to the module
 *
 * @return 0 - on success,
 * 		negative value - on error
 */
static int _email_module_create_attach_panel(email_module_t *module);

/**
 * @brief Closes attach panel or resets its state after close by itself
 *
 * @param[in]	module			pointer to the module
 * @param[in]	notify_close	true - need to notify listener, false - otherwise
 */
static void _email_module_close_attach_panel(email_module_t *module, bool notify_close);

/**
 * @brief Handler method of the attach panel reply callback
 *
 * @param[in]	attach_panel		handle of the attach panel which sends this result callback
 * @param[in]	content_category	category for which result was send
 * @param[in]	reply				reply that was send back to the attach panel from the application
 * @param[in]	reply_res			result status of the request send back by the application
 * @param[in]	user_data			pointer to the callback user data
 */
static void _email_module_attach_panel_result_cb(attach_panel_h attach_panel,
		attach_panel_content_category_e content_category,
		app_control_h reply,
		app_control_result_e reply_res,
		void *user_data);

/**
 * @brief Handler method of the attach panel event callback
 *
 * @param[in]	attach_panel	handle of the attach panel which triggers this event
 * @param[in]	event			type of the event occurred
 * @param[in]	event_info		pointer of the event information. Depends on the type of the event.
 * 								See attach panel documentation for details.
 * @param[in]	user_data		pointer to the callback user data
 */
static void _email_module_attach_panel_event_cb(attach_panel_h attach_panel,
		attach_panel_event_e event,
		void *event_info,
		void *user_data);

int email_module_launch_app(email_module_t *module, email_launch_app_type_e app_type,
		email_params_h params, email_launched_app_listener_t *listener)
{
	debug_enter();
	/* Check state and input arguments */
	retvm_if(!module, -1, "module is NULL");
	retvm_if(!params, -1, "params is NULL");
	retvm_if(module->is_launcher_busy, -1, "Launcher is busy");

	/* Check if valid application type */
	if ((app_type <= EMAIL_LAUNCH_APP_NONE) || (app_type >= EMAIL_LAUNCH_APP_COUNT)) {
		debug_error("Invalid app type: %d", app_type);
		return -1;
	}

	/* Reset previous listener */
	memset(&module->_app_listener, 0, sizeof(module->_app_listener));

	/* Dummy loop to handle exceptions and cleanup */
	while (1) {
		/* Compile-time warning is preferred over undefined behavior in run-time. */
		int r;

		if (!email_params_to_app_control(params, &module->_launched_app)) {
			debug_error("Failed to convert parameters.");
			break;
		}

		/* Many applications need this in order to work properly.
		 * Otherwise group mode applications will be not closed automatically on parent terminate */
		r = app_control_set_launch_mode(module->_launched_app, APP_CONTROL_LAUNCH_MODE_GROUP);
		if (r != APP_CONTROL_ERROR_NONE) {
			debug_error("Failed to set launch mode: %d", r);
			break;
		}

		debug_log("App ID: %s", EMAIL_LAUNCH_APP_NAMES[app_type - 1]);

		/* If application id is not implicit */
		if (app_type != EMAIL_LAUNCH_APP_AUTO) {
			/* Set application id of the terget application */
			r = app_control_set_app_id(module->_launched_app, EMAIL_LAUNCH_APP_NAMES[app_type - 1]);
			if (r != APP_CONTROL_ERROR_NONE) {
				debug_error("Failed to set App ID: %d", r);
				break;
			}
		}

		/* We need to know when the application was actually started by AUL */
		r = app_control_enable_app_started_result_event(module->_launched_app);
		if (r != APP_CONTROL_ERROR_NONE) {
			debug_error("Failed to enable app started result event: %d", r);
			break;
		}

		/* Perform launch request */
		r = app_control_send_launch_request(module->_launched_app,
				_email_module_app_reply_cb, module);
		if (r != APP_CONTROL_ERROR_NONE) {
			debug_error("Failed to send launch request: %d", r);
			break;
		}

		/* Copy listener if it was specified */
		if (listener) {
			memcpy(&module->_app_listener, listener, sizeof(*listener));
		}

		/* Initialzie application launch state */
		module->launched_app_type = app_type;
		module->is_launcher_busy = true;

		/* Return from function with success */
		debug_leave();
		return 0;
	}

	/* Exception handling and cleanup */

	/* Destroy application control structure */
	if (module->_launched_app) {
		app_control_destroy(module->_launched_app);
		module->_launched_app = NULL;
	}

	return -1;
}

int email_module_launch_attach_panel(email_module_t *module)
{
	debug_enter();
	/* Check state and input arguments */
	retvm_if(!module, -1, "module is NULL");
	retvm_if(module->is_attach_panel_launched, -1, "Attach panel is running");
	retvm_if(module->is_launcher_busy, -1, "Launcher is busy");

	/* We should create attach panel if not created.
	 * Or recreate if bundles changes */
	if (!module->_attach_panel || module->_attach_panel_bundles_changed) {
		int r = 0;

		/* Free attach panel in case of recreate */
		if (module->_attach_panel) {
			debug_log("Freeing attach panel...");
			_email_module_free_attach_panel(module);
		}
		/* Reset changed state of bundles */
		module->_attach_panel_bundles_changed = false;

		/* Creating new attach panel */
		debug_log("Creating attach panel...");
		r = _email_module_create_attach_panel(module);
		retvm_if(r != 0, -1, "_email_module_create_attach_panel() failed");
	}

	/* Launching attach panel */
	attach_panel_show(module->_attach_panel);

	/* Initialzie attach panel launch state */
	module->is_attach_panel_launched = true;

	debug_leave();
	return 0;
}

int email_module_set_attach_panel_listener(email_module_t *module,
		email_attach_panel_listener_t *listener)
{
	debug_enter();
	retvm_if(!module, -1, "module is NULL");

	if (listener) {
		memcpy(&module->_attach_panel_listener, listener, sizeof(*listener));
	} else {
		memset(&module->_attach_panel_listener, 0, sizeof(module->_attach_panel_listener));
	}

	debug_leave();
	return 0;
}

int email_module_set_attach_panel_category_bundle(email_module_t *module,
		email_attach_panel_category_type_e category_type, bundle *b)
{
	debug_enter();
	/* Check input arguments */
	retvm_if(!module, -1, "module is NULL");
	retvm_if((category_type < 0) || (category_type >= EMAIL_APCT_COUNT), -1,
			"Invalid category type: %d", category_type);

	bundle *new_b = NULL;

	/* Copy source bundle if not NULl */
	if (b) {
		new_b = bundle_dup(b);
		if (!new_b) {
			debug_error("Failed to duplicate a bundle");
			return -1;
		}
	}

	/* Update changed state of the bundle */
	if (b || module->_attach_panel_bundles[category_type]) {
		module->_attach_panel_bundles_changed = true;
	}

	/* Free existing bundle if set */
	_email_module_free_attach_panel_bundle(module, category_type);

	/* Save new bundle */
	module->_attach_panel_bundles[category_type] = new_b;

	debug_leave();
	return 0;
}

void email_module_terminate_any_launched_app(email_module_t *module, bool notify_close)
{
	debug_enter();
	/* Check input arguments */
	retm_if(!module, "module is NULL");

	/* Implement using private function */
	_email_module_terminate_any_launched_app(module, notify_close);

	debug_leave();
}

void _email_module_handle_launcher_pause(email_module_t *module)
{
	debug_enter();

	/* Remove timer on application pause.
	 * We will wait for resume instead. */
	if (module->_app_timer) {
		debug_log("Email was paused after app start reply. Delete timer...");

		ecore_timer_del(module->_app_timer);
		module->_app_timer = NULL;
	}

	debug_leave();
}

void _email_module_handle_launcher_resume(email_module_t *module)
{
	debug_enter();

	/* If resume is when application was launched - assume launched application close */
	if (module->_app_was_started) {
		debug_log("Email was resumed while there was launched app. Reset app state...");

		/* Handle application close with notify callback */
		_email_module_reset_app_launch_state(module, true, false);
	}

	debug_leave();
}

void _email_module_handle_launcher_initialize(email_module_t *module)
{
	/* Emty implementation for future use */
}

void _email_module_handle_launcher_finalize(email_module_t *module)
{
	debug_enter();

	/* Terminating launched application and attach panel if any */
	_email_module_terminate_any_launched_app(module, false);

	/* Free attach panel category bundles */
	_email_module_free_attach_panel_bundles(module);

	debug_leave();
}

void _email_module_terminate_any_launched_app(email_module_t *module, bool notify_close)
{
	/* If application was launched */
	if (module->_launched_app) {
		debug_log("Terminating launched app");

		/* Sending a terminate request to this application */
		int r = app_control_send_terminate_request(module->_launched_app);
		if (r != APP_CONTROL_ERROR_NONE) {
			debug_warning("Failed to send terminate request: %d", r);
		}

		/* Handling application close immediately */
		_email_module_reset_app_launch_state(module, notify_close, true);

	}

	/* If attach panel was launched */
	if (module->is_attach_panel_launched) {
		debug_log("Closing attach panel");

		/* Close it before freeing */
		_email_module_close_attach_panel(module, notify_close);
	}

	/* Free the attach panel in any case */
	_email_module_free_attach_panel(module);
}

void _email_module_reset_app_launch_state(email_module_t *module, bool notify_close, bool reset_listener)
{
	/* Destroy launch request */
	app_control_destroy(module->_launched_app);
	module->_launched_app = NULL;

	/* Remove timer */
	if (module->_app_timer) {
		ecore_timer_del(module->_app_timer);
		module->_app_timer = NULL;
	}

	/* Reset state variables */

	module->_app_was_started = false;

	module->is_launcher_busy = false;

	/* Notify listener about close if needed */
	if (module->_app_listener.close_cb) {
		if (notify_close) {
			module->_app_listener.close_cb(module->_app_listener.cb_data);
		}
		module->_app_listener.close_cb = NULL;
	}

	/* Inside close callback a new application may be launched.
	 * Reset rest of the state if this not happened. */
	if (!module->_launched_app) {
		if (reset_listener) {
			memset(&module->_app_listener, 0, sizeof(module->_app_listener));
		}

		module->launched_app_type = EMAIL_LAUNCH_APP_NONE;
	}
}

void _email_module_free_attach_panel(email_module_t *module)
{
	debug_enter();

	/* Destroy attach panel if crated */
	if (module->_attach_panel) {
		attach_panel_destroy(module->_attach_panel);
		module->_attach_panel = NULL;
	}

	debug_leave();
}

void _email_module_free_attach_panel_bundles(email_module_t *module)
{
	/* Free all attach panel category bundle ony by one */
	int i = 0;
	for (i = 0; i < EMAIL_APCT_COUNT; ++i) {
		_email_module_free_attach_panel_bundle(module,
				(email_attach_panel_category_type_e)i);
	}
}

void _email_module_free_attach_panel_bundle(email_module_t *module,
		email_attach_panel_category_type_e category_type)
{
	/* Free specified attach panel category bundle if set */
	if (module->_attach_panel_bundles[category_type]) {
		bundle_free(module->_attach_panel_bundles[category_type]);
		module->_attach_panel_bundles[category_type] = NULL;
	}
}

void _email_module_app_reply_cb(app_control_h request, app_control_h reply,
		app_control_result_e result, void *user_data)
{
	email_module_t *module = user_data;

	/* Ignore all unexpected reply callbacks */
	if (!module->_launched_app) {
		debug_warning("Unexpected reply from an application!");
	}

	/* Handle special result cases */
	switch (result) {
	/* Application was actually started by AUL */
	case APP_CONTROL_RESULT_APP_STARTED:
		debug_log("App was started.");
		/* So we ignore resume until this event */
		module->_app_was_started = true;
		/* Creating timer only if not in pause.
		 * Because after pause will be resume instead of timer */
		if (module->_state == EM_STATE_RUNNING) {
			debug_log("Email is running. Creating timer...");
			/* This should not happend */
			if (module->_app_timer) {
				debug_warning("Timer was already created.");
				ecore_timer_del(module->_app_timer);
			}
			/* Creating timer to reset application state if launching transparent application*/
			module->_app_timer = ecore_timer_add(EMAIL_MODULE_APP_TIMER_TIMEOUT_SEC,
					_email_module_app_timer_cb, module);
			if (!module->_app_timer) {
				debug_error("Failed to start app timer.");
			}
		}
		break;
	/* Launching of the application was cancelled by system */
	case APP_CONTROL_RESULT_CANCELED:
		/* Reset state immediately */
		debug_log("App launch was cancelled by framework.");
		_email_module_reset_app_launch_state(module, true, true);
		break;
	default:
		/* In other cases - forward reply to listener */
		if (module->_app_listener.reply_cb) {
			email_params_h params = NULL;
			if (email_params_from_app_control(&params, reply)) {
				module->_app_listener.reply_cb(module->_app_listener.cb_data, result, params);
				email_params_free(&params);
			} else {
				debug_error("Reply convert error!");
			}
			module->_app_listener.reply_cb = NULL;
		}
	}
}

Eina_Bool _email_module_app_timer_cb(void *data)
{
	email_module_t *module = data;

	debug_log("Timer expired. Reset app state...");

	/* We will cancel this timer in this function */
	module->_app_timer = NULL;

	/* Assuming termination of the transparent application */
	_email_module_reset_app_launch_state(module, true, false);

	return ECORE_CALLBACK_CANCEL;
}

int _email_module_create_attach_panel(email_module_t *module)
{
	debug_enter();

	/* Creating attach panel */
	int r = attach_panel_create(module->conform, &module->_attach_panel);
	if (r != ATTACH_PANEL_ERROR_NONE) {
		debug_error("attach_panel_create() failed: %d", r);
		module->_attach_panel = NULL;
		return -1;
	}

	/* Adding necessary categories to the attach panel */

	attach_panel_add_content_category(module->_attach_panel,
			ATTACH_PANEL_CONTENT_CATEGORY_IMAGE,
			module->_attach_panel_bundles[EMAIL_APCT_IMAGE]);

	attach_panel_add_content_category(module->_attach_panel,
			ATTACH_PANEL_CONTENT_CATEGORY_CAMERA,
			module->_attach_panel_bundles[EMAIL_APCT_CAMERA]);

	attach_panel_add_content_category(module->_attach_panel,
			ATTACH_PANEL_CONTENT_CATEGORY_VOICE,
			module->_attach_panel_bundles[EMAIL_APCT_VOICE]);

	attach_panel_add_content_category(module->_attach_panel,
			ATTACH_PANEL_CONTENT_CATEGORY_VIDEO,
			module->_attach_panel_bundles[EMAIL_APCT_VIDEO]);

	attach_panel_add_content_category(module->_attach_panel,
			ATTACH_PANEL_CONTENT_CATEGORY_AUDIO,
			module->_attach_panel_bundles[EMAIL_APCT_AUDIO]);

	attach_panel_add_content_category(module->_attach_panel,
			ATTACH_PANEL_CONTENT_CATEGORY_CALENDAR,
			module->_attach_panel_bundles[EMAIL_APCT_CALENDAR]);

	attach_panel_add_content_category(module->_attach_panel,
			ATTACH_PANEL_CONTENT_CATEGORY_CONTACT,
			module->_attach_panel_bundles[EMAIL_APCT_CONTACT]);

	attach_panel_add_content_category(module->_attach_panel,
			ATTACH_PANEL_CONTENT_CATEGORY_MYFILES,
			module->_attach_panel_bundles[EMAIL_APCT_MYFILES]);

	attach_panel_add_content_category(module->_attach_panel,
			ATTACH_PANEL_CONTENT_CATEGORY_VIDEO_RECORDER,
			module->_attach_panel_bundles[EMAIL_APCT_VIDEO_RECORDER]);

	/* Registering necessary attach panel callbacks */

	attach_panel_set_result_cb(module->_attach_panel,
			_email_module_attach_panel_result_cb, module);

	attach_panel_set_event_cb(module->_attach_panel,
			_email_module_attach_panel_event_cb, module);

	debug_leave();
	return 0;
}

void _email_module_close_attach_panel(email_module_t *module, bool notify_close)
{
	debug_enter();

	/* Hide attach panel if launched */
	if (module->is_attach_panel_launched) {
		module->is_attach_panel_launched = false;

		attach_panel_hide(module->_attach_panel);
		debug_log("attach panel hide called");
	}

	/* Notify listener */
	if (module->_attach_panel_listener.close_cb) {
		if (notify_close) {
			module->_attach_panel_listener.close_cb(module->_attach_panel_listener.cb_data);
		}
	}

	debug_leave();
}

void _email_module_attach_panel_result_cb(attach_panel_h attach_panel,
		attach_panel_content_category_e content_category,
		app_control_h reply,
		app_control_result_e reply_res,
		void *user_data)
{
	debug_enter();
	email_module_t *module = user_data;

	/* Declare local variables */

	int r = APP_CONTROL_ERROR_NONE;
	int i = 0;

	char **path_array = NULL;
	int array_len = 0;

	/* Parse result data */
	if (reply) {
		r = app_control_get_extra_data_array(reply, APP_CONTROL_DATA_SELECTED, &path_array, &array_len);
		if (r != APP_CONTROL_ERROR_NONE) {
			debug_error("app_control_get_extra_data_array() failed: %d", r);
			path_array = NULL;
			array_len = 0;
		}
	} else {
		debug_error("reply is NULL");
	}

	/* TODO: add error about wrong data */

	/* If listener assigned */
	if (module->_attach_panel_listener.reply_cb) {
		/* If success */
		if (r == APP_CONTROL_ERROR_NONE) {
			/* This callback may be called in thread so we must sync with GUI thread */
			ecore_thread_main_loop_begin();

			/* Invoke reply callback */
			module->_attach_panel_listener.reply_cb(
					module->_attach_panel_listener.cb_data, (const char **)path_array, array_len);

			/* We must unlock GUI thread */
			ecore_thread_main_loop_end();
		}
	}

	/* Free path array and items */
	for (i = 0; i < array_len; i++) {
		free(path_array[i]);
	}
	free(path_array);

	debug_leave();
}

void _email_module_attach_panel_event_cb(attach_panel_h attach_panel,
		attach_panel_event_e event,
		void *event_info,
		void *user_data)
{
	email_module_t *module = user_data;

	switch (event)
	{
	/* Handle only attach panel hide event */
	case ATTACH_PANEL_EVENT_HIDE_FINISH:
		debug_log("Attach panel - HIDE FINISH");
		/* Ignore if was not launched */
		if (module->is_attach_panel_launched) {
			/* Reset state so we do not hide attach panel twice (it was already hidden) */
			module->is_attach_panel_launched = false;
			_email_module_close_attach_panel(module, true);
		} else {
			debug_log("event ignored");
		}
		break;
	default:
		/* Not interested in other events*/
		break;
	}
}
