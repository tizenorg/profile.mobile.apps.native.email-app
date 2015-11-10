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

#ifndef _EMAIL_MODULE_H_
#define _EMAIL_MODULE_H_

#include <app_control.h>
#include <Evas.h>

#include "email-common-types.h"

/**
 * Opaque handle of the email module
 */
typedef struct {} *email_module_h;

/**
 * Enumeration of the email module types
 */
typedef enum _email_module_type
{
	/* Represents absence of module */
	EMAIL_MODULE_NONE = 0,

	/* Mail box module to display list of email by different folders */
	EMAIL_MODULE_MAILBOX,
	/* Viewer module to read emails */
	EMAIL_MODULE_VIEWER,
	/* Composer module to compose and send new emails */
	EMAIL_MODULE_COMPOSER,
	/* Account module to manage account folders */
	EMAIL_MODULE_ACCOUNT,
	/* Setting module to configure account settings */
	EMAIL_MODULE_SETTING,
	/* Filter module to configure priority senders */
	EMAIL_MODULE_FILTER,

	/* Count of enumeration values */
	EMAIL_MODULE_COUNT
} email_module_type_e;

/**
 * Enumeration of the email module events
 */
typedef enum _email_module_event
{
	/* System is almost out of memory */
	EM_EVENT_LOW_MEMORY_SOFT,
	/* System is out of memory */
	EM_EVENT_LOW_MEMORY_HARD,
	/* Device battery level is low */
	EM_EVENT_LOW_BATTERY,
	/* Device locale language was changed */
	EM_EVENT_LANG_CHANGE,
	/* Device orientation change events */
	EM_EVENT_ROTATE_PORTRAIT,
	EM_EVENT_ROTATE_PORTRAIT_UPSIDEDOWN,
	EM_EVENT_ROTATE_LANDSCAPE,
	EM_EVENT_ROTATE_LANDSCAPE_UPSIDEDOWN,
	/* Region format information was changed */
	EM_EVENT_REGION_CHANGE

} email_module_event_e;

/**
 * Listener of the email module events
 */
typedef struct _email_module_listener
{
	/* User data to pass to each callback function */
	void *cb_data;

	/**
	 * @brief Callback which is used to send result messages to the listener
	 *
	 * @param[in]	data		callback user data
	 * @param[in]	module		sender module
	 * @param[in]	result		result message send back to the listener
	 */
	void (*result_cb) (void *data, email_module_h module, app_control_h result);

	/**
	 * @brief Callback which is used to send destroy request to the listener
	 *
	 * Module can't destroy itself. It must make a destroy request.
	 * Because only the creator of the module can destroy it.
	 *
	 * @param[in]	data		callback user data
	 * @param[in]	module		sender module
	 */
	void (*destroy_request_cb) (void *data, email_module_h module);

	/**
	 * @brief Callback  which is called when module is actually destroyed
	 *
	 * @param[in]	data		callback user data
	 * @param[in]	module		sender module
	 */
	void (*destroy_cb) (void *data, email_module_h module);

} email_module_listener_t;

/**
 * email_module_mgr
 */

/**
 * @brief Initializes module manager global object
 *
 * This function must be called before any other function related to email modules.
 *
 * @param[in]	win			application main window widget
 * @param[in]	conform		application conformant widget
 *
 * @return 0 - on success,
 * 		negative value - on error
 */
EMAIL_API int email_module_mgr_init(Evas_Object *win, Evas_Object *conform);

/**
 * @brief Immediately finalazes module manager and all remaining modules
 *
 * Use this function when application terminates.
 * If you simply need to restart the application with different root module
 * you may simply destroy root module and create new one again.
 *
 * @return 0 - on success,
 * 		negative value - on error
 */
EMAIL_API int email_module_mgr_finalize();

/**
 * @brief Pauses email module manager and modules
 *
 * Call this function in application pause callback
 *
 * @return 0 - on success,
 * 		negative value - on error
 */
EMAIL_API int email_module_mgr_pause();

/**
 * @brief Resumes email module manager and modules
 *
 * Call this function in application resume callback
 *
 * @return 0 - on success,
 * 		negative value - on error
 */
EMAIL_API int email_module_mgr_resume();

/**
 * @brief Sends event to module manager and modules
 *
 * Call this function in application event callbacks
 *
 * @param[in]	event	event type to send
 *
 * @return 0 - on success,
 * 		negative value - on error
 */
EMAIL_API int email_module_mgr_send_event(email_module_event_e event);

/**
 * @brief Creates new root module
 *
 * To create child module you must use method of the parent module.
 * See "email-module-core.h" for details.
 *
 * @param[in]	module_type		type of the module to create
 * @param[in]	params			module create parameters (depends on concrete module)
 * @param[in]	listener		listener of the new module
 *
 * @return handle to the new module,
 * 		NULL - on error
 */
EMAIL_API email_module_h email_module_mgr_create_root_module(email_module_type_e module_type,
		app_control_h params, email_module_listener_t *listener);

/**
 * @brief Checks weather the module manager is in the middle of switching views
 *
 * @return true - switching of views is currently in progress,
 * 		false - otherwise
 */
EMAIL_API bool email_module_mgr_is_in_transiton();

/**
 * @brief Checks weather the application is in compressed mode (e.g. keyboard is shown)
 *
 * @return true - if in compressed mode
 * 		false - otherwise
 */
EMAIL_API bool email_module_mgr_is_in_compressed_mode();

/**
 * email_module
 */

/**
 * @brief Destroys the module
 *
 * Destruction of the module is asynchronous process.
 * To know the actual time of the destruction use destroy_cb().
 * Root module always get destroyed immediately.
 *
 * @param[in]	module		handle of the module to destroy
 *
 * @return 0 - on success,
 * 		negative value - on error
 */
EMAIL_API int email_module_destroy(email_module_h module);

/**
 * @brief Same as email_module_destroy() but no using naviframe pop
 *
 * When module manager is in transition pop will e used anyway for non root modules.
 * For the root module this function has the same effect as email_module_destroy().
 *
 * @param[in]	module		handle of the module to destroy
 *
 * @return 0 - on success,
 * 		negative value - on error
 */
EMAIL_API int email_module_destroy_no_pop(email_module_h module);

/**
 * @brief Sends a message to the spceified module
 *
 * @param[in]	module		handle of the module to send message
 * @param[in]	msg			message to send
 *
 * @return 0 - on success,
 * 		negative value - on error
 */
EMAIL_API int email_module_send_message(email_module_h module, app_control_h msg);

#endif /* _EMAIL_MODULE_H_ */
