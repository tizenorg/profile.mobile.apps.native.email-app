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

#ifndef _EMAIL_MODULE_CORE_H_
#define _EMAIL_MODULE_CORE_H_

/**
 * Forward declaration of the email module structure
 */
typedef struct _email_module email_module_t;

/**
 * Forward declaration of the email view structure
 */
typedef struct _email_view email_view_t;

/**
 * @brief Sends a result from the module back to listener
 *
 * @param[in]	module		handle of the module from which to send a message
 * @param[in]	result		result message to send
 *
 * @return 0 - on success,
 * 		negative value - on error
 */
EMAIL_API int email_module_send_result(email_module_t *module, app_control_h result);

/**
 * @brief Makes destroy request to listener
 *
 * @param[in]	module		handle of the module from which to make a request
 *
 * @return 0 - on success,
 * 		negative value - on error
 */
EMAIL_API int email_module_make_destroy_request(email_module_t *module);

/**
 * @brief Creates child module for the specified module
 *
 * Current implementation supports only single child.
 * But new child may be created when the old has not been destroyed yet.
 *
 * @param[in]	module			parent module
 * @param[in]	module_type		type of the module to create
 * @param[in]	params			module create parameters (depends on concrete module)
 * @param[in]	listener		listener of the new module
 *
 * @return handle to the new module,
 * 		NULL - on error
 */
EMAIL_API email_module_h email_module_create_child(email_module_t *module, email_module_type_e module_type,
		app_control_h params, email_module_listener_t *listener);

/**
 * @brief Creates view for specified module
 *
 * View object must be allocated externally before using this function.
 *
 * @param[in]	module		parent module
 * @param[in]	view		pointer to the allocated view
 *
 * @return 0 - on success,
 * 		negative value - on error
 */
EMAIL_API int email_module_create_view(email_module_t *module, email_view_t *view);

/**
 * @brief Pushes view to the naviframe and completes view creation
 *
 * This method must be called inside create() method of the concrete view
 *
 * @param[in]	view			view to push
 * @param[in]	title_label		initial title label of the view
 * @param[in]	flags			push flags
 *
 * @return 0 - on success,
 * 		negative value - on error
 */
EMAIL_API Elm_Object_Item *email_module_view_push(email_view_t *view, const char *title_label, int flags);

/**
 * @brief Exits from the caller view
 *
 * @param[in]	view		view pointer
 *
 * @return 0 - on success,
 * 		negative value - on error
 */
EMAIL_API int email_module_exit_view(email_view_t *view);

#endif /* _EMAIL_MODULE_CORE_H_ */
