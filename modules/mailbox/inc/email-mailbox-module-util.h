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

#ifndef __DEF_EMAIL_MAILBOX_MODULE_UTIL_H_
#define __DEF_EMAIL_MAILBOX_MODULE_UTIL_H_

/**
 * @brief Creates Composer module from Mailbox view
 * @param[in]	data		User data (Email mailbox data)
 * @param[in]	type		Email Module type (EMAIL_MODULE_COMPOSER)
 * @param[in]	params		Email params handle
 * @return email_module_h handler that used as text filter
 */
email_module_h mailbox_composer_module_create(void *data, email_module_type_e type, email_params_h params);

/**
 * @brief Callback function to destroy Composer module
 * @param[in]	priv		User data (Email mailbox data)
 * @param[in]	module		Email Module handler
 */
void mailbox_composer_module_destroy(void *priv, email_module_h module);

/**
 * @brief Creates Viewer module from Mailbox view
 * @param[in]	data		User data (Email mailbox data)
 * @param[in]	type		Email Module type (EMAIL_MODULE_COMPOSER)
 * @param[in]	params		Email params handle
 * @return email_module_h handler that used as text filter
 */
email_module_h mailbox_viewer_module_create(void *data, email_module_type_e type, email_params_h params);

/**
 * @brief Callback function to destroy Viewer module
 * @param[in]	priv		User data (Email mailbox data)
 * @param[in]	module		Email Module handler
 */
void mailbox_viewer_module_destroy(void *priv, email_module_h module);

/**
 * @brief Creates Account module from Mailbox view
 * @param[in]	data		User data (Email mailbox data)
 * @param[in]	type		Email Module type (EMAIL_MODULE_COMPOSER)
 * @param[in]	params		Email params handle
 * @return email_module_h handler that used as text filter
 */
email_module_h mailbox_account_module_create(void *data, email_module_type_e type, email_params_h params);

/**
 * @brief Callback function to destroy Account module
 * @param[in]	priv		User data (Email mailbox data)
 * @param[in]	module		Email Module handler
 */
void mailbox_account_module_destroy(void *priv, email_module_h module);

/**
 * @brief Creates Setting module from Mailbox view
 * @param[in]	data		User data (Email mailbox data)
 * @param[in]	type		Email Module type (EMAIL_MODULE_COMPOSER)
 * @param[in]	params		Email params handle
 * @return email_module_h handler that used as text filter
 */
email_module_h mailbox_setting_module_create(void *data, email_module_type_e type, email_params_h params);

/**
 * @brief Callback function to destroy Setting module
 * @param[in]	priv		User data (Email mailbox data)
 * @param[in]	module		Email Module handler
 */
void mailbox_setting_module_destroy(void *priv, email_module_h module);

#endif /*__DEF_EMAIL_MAILBOX_MODULE_UTIL_H_*/
