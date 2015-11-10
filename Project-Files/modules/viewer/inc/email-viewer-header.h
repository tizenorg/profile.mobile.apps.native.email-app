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

#ifndef __EMAIL_VIEWER_HEADER_H__
#define __EMAIL_VIEWER_HEADER_H__

/**
 * @brief Update attachment summary info
 *
 * @param[in]	ug_data		Email viewer data
 *
 */
void header_update_attachment_summary_info(EmailViewerUGD *ug_data);

/**
 * @brief Delete viewer header attachment preview
 *
 * @param[in]	data		User data (Email viewer data)
 *
 */
void _header_delete_attachment_preview(void *data);

/**
 * @brief Create viewer header view
 *
 * @param[in]	data		User data (Email viewer data)
 *
 */
void header_create_view(void *data);

/**
 * @brief Update viewer header view
 *
 * @param[in]	data		User data (Email viewer data)
 *
 */
void header_update_view(void *data);

/**
 * @brief Update viewer header date
 *
 * @param[in]	data		User data (Email viewer data)
 *
 */
void header_update_date(EmailViewerUGD *ug_data);

/**
 * @brief Update viewer header response icon REPLY/FORWARD
 *
 * @param[in]	ug_data			Email viewer data
 * @param[in]	mail_info		Email mail data
 *
 */
void header_update_response_icon(EmailViewerUGD *ug_data, email_mail_data_t *mail_info);

/**
 * @brief Update viewer header favorite icon
 *
 * @param[in]	ug_data		Email viewer data
 *
 */
void header_update_favorite_icon(EmailViewerUGD *ug_data);

/**
 * @brief Set viewer header favorite icon
 *
 * @param[in]	ug_data		Email viewer data
 *
 */
void header_set_favorite_icon(EmailViewerUGD *ug_data);

/**
 * @brief Set viewer header sender name
 *
 * @param[in]	ug_data		Email viewer data
 *
 */
void header_set_sender_name(EmailViewerUGD *ug_data);

/**
 * @brief Unpack header layout from box
 *
 * @param[in]	data		User data (Email viewer data)
 *
 */
void header_layout_unpack(void *data);

/**
 * @brief Pack header layout from box
 *
 * @param[in]	data		User data (Email viewer data)
 *
 */
void header_layout_pack(void *data);

/**
 * @brief Open pattern generator
 *
 * @return 0 on success, otherwise a negative error value
 */
int viewer_open_pattern_generator(void);

/**
 * @brief Close pattern generator
 *
 * @return 0 on success, otherwise a negative error value
 */
int viewer_close_pattern_generator(void);
#endif /* __EMAIL_VIEWER_HEADER_H__ */
