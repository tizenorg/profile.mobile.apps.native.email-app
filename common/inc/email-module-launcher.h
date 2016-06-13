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

#ifndef _EMAIL_MODULE_LAUNCHER_H_
#define _EMAIL_MODULE_LAUNCHER_H_

/**
 * Enumeration of the application type
 */
typedef enum _email_launch_app_type
{
	/* Application is not specified */
	EMAIL_LAUNCH_APP_NONE = 0,

	/* Used in implicit request */
	EMAIL_LAUNCH_APP_AUTO,

	/* Launch ringtone application */
	EMAIL_LAUNCH_APP_RINGTONE,
	/* Launch settings storage application */
	EMAIL_LAUNCH_APP_STORAGE_SETTINGS,

	/* Count of enumeration values */
	EMAIL_LAUNCH_APP_COUNT
} email_launch_app_type_e;

/**
 * Listener of the launched application events
 */
typedef struct _email_launch_app_listener
{
	/* User data to pass to each callback function */
	void *cb_data;

	/**
	 * @brief Callback which is used to send result messages to the listener
	 *
	 * @param[in]	data		callback user data
	 * @param[in]	result		result status of the reply
	 * @param[in]	reply		reply message send back from the application
	 */
	void (*reply_cb) (void *data, app_control_result_e result, email_params_h reply);

	/**
	 * @brief Callback which is used to signal close of the application
	 *
	 * @param[in]	data		callback user data
	 */
	void (*close_cb) (void *data);

} email_launched_app_listener_t;

/**
 * Enumeration of the attach panel mode type
 */
typedef enum _email_attach_panel_mode_type
{
	/* General attach panal mode to use when attaching files */
	EMAIL_APMT_GENERAL,
	/* Attach panel mode to attach only image files or taking pictures */
	EMAIL_APMT_IMAGES,

	/* Count of enumeration values */
	EMAIL_APMT_COUNT
} email_attach_panel_mode_type_e;

/**
 * Enumeration of the attach panel category type
 */
typedef enum _email_attach_panel_category_type
{
	/* Image files */
	EMAIL_APCT_IMAGE,
	/* Take picture with camera */
	EMAIL_APCT_CAMERA,
	/* Record voice with voice recorder */
	EMAIL_APCT_VOICE,
	/* Video files */
	EMAIL_APCT_VIDEO,
	/* Audio files */
	EMAIL_APCT_AUDIO,
	/* Calendar events */
	EMAIL_APCT_CALENDAR,
	/* Contact vCard files */
	EMAIL_APCT_CONTACT,
	/* Any file from device */
	EMAIL_APCT_MYFILES,
	/* Record video with camera */
	EMAIL_APCT_VIDEO_RECORDER,

	/* Count of enumeration values */
	EMAIL_APCT_COUNT
} email_attach_panel_category_type_e;

/**
 * Listener of the attach panel events events
 */
typedef struct _email_attach_panel_listener
{
	/* User data to pass to each callback function */
	void *cb_data;

	/**
	 * @brief Callback which is used to send reply from attach panel to the listener
	 *
	 * @param[in]	data			callback user data
	 * @param[in]	path_array		array with file paths
	 * @param[in]	array_len		length of the array
	 */
	void (*reply_cb) (void *data, email_attach_panel_mode_type_e mode,
			const char **path_array, int array_len);
	void (*close_cb) (void *data);

} email_attach_panel_listener_t;

/**
 * Forward declaration of the email module structure
 */
typedef struct _email_module email_module_t;

/**
 * @brief Launches the application
 *
 * @param[in]	module		pointer to the module
 * @param[in]	app_type	type of the application to launch
 * @param[in]	params		application launch extra data parameters
 * @param[in]	listener	application listener
 *
 * @return 0 - on success,
 * 		negative value - on error
 */
EMAIL_API int email_module_launch_app(email_module_t *module, email_launch_app_type_e app_type,
		email_params_h params, email_launched_app_listener_t *listener);

/**
 * @brief Launches the attach panel
 *
 * Attach panel can't be launched if an application is already was launched
 *
 * @param[in]	module		pointer to the module
 * @param[in]	mode		attach panel mode
 *
 * @return 0 - on success,
 * 		negative value - on error
 */
EMAIL_API int email_module_launch_attach_panel(email_module_t *module,
		email_attach_panel_mode_type_e mode);

/**
 * @brief Registers the attach panel listener
 *
 * @param[in]	module		pointer to the module
 * @param[in]	listener	attach panel listener (or NULL to unset)
 *
 * @return 0 - on success,
 * 		negative value - on error
 */
EMAIL_API int email_module_set_attach_panel_listener(email_module_t *module,
		email_attach_panel_listener_t *listener);

/**
 * @brief Attaches extra data category bundle
 *
 * Attach panel can't be launched if an application is already was launched
 *
 * @param[in]	module		pointer to the module
 * @param[in]	listener	attach panel listener
 *
 * @return 0 - on success,
 * 		negative value - on error
 */
EMAIL_API int email_module_set_attach_panel_category_bundle(email_module_t *module,
		email_attach_panel_category_type_e category_type, bundle *b);

/**
 * @brief Terminates launched application and attach panel if any
 *
 * @param[in]	module			pointer to the module
 * @param[in]	notify_close	true - notify listener on close, false - otherwise
 */
EMAIL_API void email_module_terminate_any_launched_app(email_module_t *module, bool notify_close);

/**
 * PRIVATE FUNCTIONALITY
 */

/**
 * @brief Initializes launcher sub object
 *
 * This is private function. Do not call this function directly.
 *
 * @param[in]	module		pointer to the module
 */
void _email_module_handle_launcher_initialize(email_module_t *module);

/**
 * @brief Signals about module pause to the launcher sub object
 *
 * This is private function. Do not call this function directly.
 *
 * @param[in]	module		pointer to the module
 */
void _email_module_handle_launcher_pause(email_module_t *module);

/**
 * @brief Signals about module resume to the launcher sub object
 *
 * This is private function. Do not call this function directly.
 *
 * @param[in]	module		pointer to the module
 */
void _email_module_handle_launcher_resume(email_module_t *module);

/**
 * @brief Finalizes launcher sub object
 *
 * This is private function. Do not call this function directly.
 *
 * @param[in]	module		pointer to the module
 */
void _email_module_handle_launcher_finalize(email_module_t *module);

#endif /* _EMAIL_MODULE_LAUNCHER_H_ */
