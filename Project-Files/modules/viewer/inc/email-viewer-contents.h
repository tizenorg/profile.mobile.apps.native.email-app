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

#ifndef __EMAIL_VIEWER_CONTENTS_H__
#define __EMAIL_VIEWER_CONTENTS_H__

#include "email-viewer.h"

/**
 * @brief Register webkit callbacks
 *
 * @param[in]	data			User data (Email viewer data)
 *
 */
void viewer_webkit_add_callbacks(void *data);

/**
 * @brief Unregister webkit callbacks
 *
 * @param[in]	data			User data (Email viewer data)
 *
 */
void viewer_webkit_del_callbacks(void *data);

/**
 * @brief Creats webview and set initial setings to it
 *
 * @param[in]	ug_data			Email viewer data
 *
 * @return Evas_Object webview instance on success or NULL if an error occurred
 */
Evas_Object *viewer_get_webview(EmailViewerUGD *ug_data);

/**
 * @brief Set content to webview
 *
 * @param[in]	ug_data			Email viewer data
 *
 */
void viewer_set_webview_content(EmailViewerUGD *ug_data);

/**
 * @brief Set initial webview height
 *
 * @param[in]	data			User data (Email viewer data)
 *
 */
void viewer_set_initial_webview_height(void *data);

/**
 * @brief Hides webview
 *
 * @param[in]	data			User data (Email viewer data)
 *
 */
void viewer_hide_webview(void *data);

/**
 * @brief Shows webview
 *
 * @param[in]	data			User data (Email viewer data)
 *
 */
void viewer_show_webview(void *data);

/**
 * @brief Provides memory warning notification
 *
 * @param[in]	ug_data			Email viewer data
 *
 */
void viewer_webview_handle_mem_warning(EmailViewerUGD *ug_data, bool hard);

#endif	/* __EMAIL_VIEWER_CONTENTS_H__ */

/* EOF */
