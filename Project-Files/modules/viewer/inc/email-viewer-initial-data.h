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

#ifndef __EMAIL_VIEWER_INITIAL_DATA_H__
#define __EMAIL_VIEWER_INITIAL_DATA_H__

/**
 * @brief Provides initial data pre parse arguments
 *
 * @param[in]	data			User data (Email viewer data)
 * @param[in]	svc_handle		App control handler to get operation and data to perform
 *
 * @return VIEWER_ERROR_NONE on success, otherwise VIEWER_ERROR_TYPE_E when an error occurred
 */
int viewer_initial_data_pre_parse_arguments(void *data, app_control_h svc_handle);

/**
 * @brief Free viewer data
 *
 * @param[in]	data			User data (Email viewer data)
 *
 * @return TRUE on success or FALSE if none or an error occurred
 */
Eina_Bool viewer_free_viewer_data(void *data);

#endif /* __EMAIL_VIEWER_INITIAL_DATA_H__ */
