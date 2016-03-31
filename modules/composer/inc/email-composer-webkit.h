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

#ifndef __EMAIL_COMPOSER_WEBKIT_H__
#define __EMAIL_COMPOSER_WEBKIT_H__

/**
 * @brief Creates body field for webkit
 *
 * @param[in]	parent		Evas_Object parent
 * @param[in]	view			Email composer data
 *
 */
void composer_webkit_create_body_field(Evas_Object *parent, EmailComposerView *view);

/**
 * @brief Registers callbacks for webkit
 *
 * @param[in]	ewk_view	Evas_Object with ewk view
 * @param[in]	data		User data (Email composer data)
 *
 */
void composer_webkit_add_callbacks(Evas_Object *ewk_view, void *data);

/**
 * @brief Unregisters callbacks from webkit
 *
 * @param[in]	ewk_view		Evas_Object with ewk view
 * @param[in]	data			User data (Email composer data)
 *
 */
void composer_webkit_del_callbacks(Evas_Object *ewk_view, void *data);

/**
 * @brief Update focus on webkit elements
 *
 * @param[in]	data			User data (Email composer data)
 *
 */
void composer_webkit_blur_webkit_focus(void *data);

/**
 * @brief Provides memory worning notification
 *
 * @param[in]	data			User data (Email composer data)
 * @param[in]	is_hard			Eina_Bool
 *
 */
void composer_webkit_handle_mem_warning(void *data, Eina_Bool is_hard);

/**
 * @brief Set focus to webview by dint of java script
 *
 * @param[in]	data			User data (Email composer data)
 *
 */
void composer_webkit_set_focus_to_webview_with_js(void *data);

#endif /* __EMAIL_COMPOSER_WEBKIT_H__ */
