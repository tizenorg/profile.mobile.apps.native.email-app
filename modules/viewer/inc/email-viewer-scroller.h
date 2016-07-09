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

#ifndef __EMAIL_VIEWER_SCROLLER_H__
#define __EMAIL_VIEWER_SCROLLER_H__

#include "email-viewer.h"

/**
 * @brief Set vertical scroller position
 *
 * @param[in]	view				Email viewer data
 *
 */
void viewer_set_vertical_scroller_position(EmailViewerView *view);

/**
 * @brief Set horizontal scroller size
 *
 * @param[in]	view				Email viewer data
 *
 */
void viewer_set_horizontal_scroller_size(EmailViewerView *view);

/**
 * @brief Set vertical scroller size
 *
 * @param[in]	view				Email viewer data
 *
 */
void viewer_set_vertical_scroller_size(EmailViewerView *view);

/**
 * @brief Stop main scroller and start webkit scroller
 *
 * @param[in]	data				User data (Email viewer data)
 *
 */
void viewer_stop_elm_scroller_start_webkit_scroller(void *data);

/**
 * @brief Stop webkit scroller and start main scroller
 *
 * @param[in]	data				User data (Email viewer data)
 *
 */
void viewer_stop_webkit_scroller_start_elm_scroller(void *data);

/**
 * @brief Create combined custom scroller
 *
 * @param[in]	data				User data (Email viewer data)
 *
 */
void viewer_create_combined_scroller(void *data);

void _refresh_webview_geometry(void *data);

#endif	/* __EMAIL_VIEWER_SCROLLER_H__ */
