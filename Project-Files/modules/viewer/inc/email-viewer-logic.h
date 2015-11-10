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

#ifndef __EMAIL_VIEWER_LOGIC_H__
#define __EMAIL_VIEWER_LOGIC_H__

#include <Elementary.h>
#include <glib.h>
#include <sys/time.h>
#include <email-api.h>
#include <email-types.h>
#include "email-viewer-types.h"

/**
 * @brief Get internal mail info
 *
 * @param[in]	data			User data (Email viewer data)
 *
 * @return TRUE on success or FALSE if none or an error occurred
 */
gboolean viewer_get_internal_mail_info(void *data);

/**
 * @brief Make internal mail
 *
 * @param[in]	data			User data (Email viewer data)
 *
 */
void viewer_make_internal_mail(void *data);

/**
 * @brief Remove internal mail
 *
 * @param[in]	data			User data (Email viewer data)
 *
 */
void viewer_remove_internal_mail(void *data);

/**
 * @brief Set mail as seen
 *
 * @param[in]	data			User data (Email viewer data)
 *
 */
void viewer_set_mail_seen(void *data);

/**
 * @brief Creates hard links to all inline images
 *
 * @remark Image Viewer does not support the symbolic link
 *
 * @param[in]	data			User data (Email viewer data)
 * @param[in]	path			Target destination directory where hard link will be created
 *
 */
void viewer_make_hard_link_for_inline_images(void *data, const char *path);

/**
 * @brief Remove hard links to all the inline images
 *
 * @param[in]	data			User data (Email viewer data)
 *
 */
void viewer_remove_hard_link_for_inline_images(void *data);

/**
 * @brief Check if is normal attachment that mean not inline
 *
 * @param[in]	data			User data (Email viewer data)
 *
 * @return TRUE on success or FALSE if none or an error occurred
 */
Eina_Bool viewer_is_normal_attachment(EmailAttachmentType *info);

/**
 * @brief Set internal data (internal mail)
 *
 * @param[in]	data			User data (Email viewer data)
 * @param[in]	b_remove		Flag specifies that internal mail must be deleted and after that requested from email service once more
 *
 * @return VIEWER_ERROR_NONE on success, otherwise VIEWER_ERROR_TYPE_E when an error occurred
 */
int viewer_set_internal_data(void *data, Eina_Bool b_remove);

/**
 * @brief Remove quots from string
 *
 * @param[in]	src			User data (Email viewer data)
 *
 * @return The string without quots on success, otherwise NULL. It should be freed.
 */
char *viewer_str_removing_quots(char *src);

/**
 * @brief Get count of To/Cc/Bcc from address info
 *
 * @param[in]	data			User data (Email viewer data)
 * @param[out]	to_count		Number of recipients from field "To"
 * @param[out]	cc_count		Number of recipients from field "Cc"
 * @param[out]	bcc_count		Number of recipients from field "Bcc"
 *
 */
void viewer_get_address_info_count(email_mail_data_t *mail_info, int *to_count, int *cc_count, int *bcc_count);

#endif	/* __EMAIL_VIEWER_LOGIC_H__ */

/* EOF */
