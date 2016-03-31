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

#ifndef __EMAIL_VIEWER_ATTACHMENT_H__
#define __EMAIL_VIEWER_ATTACHMENT_H__

/**
 * @brief Provides attachment view
 *
 * @param[in]	view			Email viewer data
 *
 */
void viewer_create_attachment_view(EmailViewerView *view);

/**
 * @brief Provides process for downloading and preview, depends from current state
 *
 * @param[in]	aid			Viewer attachment data
 *
 * @return Code from VIEWER_ERROR_TYPE_E enum about occurred error
 */
VIEWER_ERROR_TYPE_E viewer_download_and_preview_save_attachment(EV_attachment_data *aid);

/**
 * @brief Set attachment state like IDLE/PENDING/PROCCES
 *
 * @param[in]	aid				Viewer attachment data
 * @param[in]	new_state		Attachment state value
 *
 */
void viewer_set_attachment_state(EV_attachment_data *aid, EV_attachment_state new_state);

/**
 * @brief Get attachment data
 *
 * @param[in]	view			Email viewer data
 * @param[in]	info_index		Attachment ID
 *
 * @return Viewer attachment data on success or NULL if none or an error occurred
 */
EV_attachment_data *viewer_get_attachment_data(EmailViewerView *view, int info_index);

/**
 * @brief Get attachment state for all attachments
 *
 * @param[in]	view			Email viewer data
 *
 * @return  Email viewer mutually attachment state enum
 */
EV_all_attachment_state viewer_get_all_attachments_state(EmailViewerView *view);

/**
 * @brief Update attachment item info
 *
 * @param[in]	data		user data (Viewer attachment data)
 *
 */
void viewer_update_attachment_item_info(void *data);

#endif /* __EMAIL_VIEWER_ATTACHMENT_H__ */
