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

#ifndef __EMAIL_VIEWER_EML_H__
#define __EMAIL_VIEWER_EML_H__

#include <glib.h>
#include "email-utils.h"
#include "email-viewer.h"

/**
 * @brief Create address info list
 *
 * @param[in]	view			Email viewer data
 *
 * @return Email addres info list on success or NULL if an error occurred
 */
email_address_info_list_t *viewer_create_address_info_list(EmailViewerView *view);

/**
 * @brief Free address info list from Email address info list data
 *
 * @param[in]	address_info_list			Email address info list data
 *
 */
void viewer_free_address_info_list(email_address_info_list_t *address_info_list);

/**
 * @brief Copy recipient list
 *
 * @param[in]	src			Source list
 *
 * @return List on success or NULL if an error occurred
 */
GList *viewer_copy_recipient_list(GList *src);

/**
 * @brief Free recipient list
 *
 * @param[in]	address_info			Address info list
 *
 */
void viewer_free_recipient_list(GList *address_info);

/**
 * @brief Copy address info
 *
 * @param[in]	src			Source Email address info data
 *
 * @return Email address info data on success or NULL if an error occurred
 */
email_address_info_t *viewer_copy_address_info(const email_address_info_t *src);

/**
 * @brief Free data from Email address info data
 *
 * @param[in]	src			Source email address info data
 *
 */
void viewer_free_address_info(email_address_info_t *src);

#endif	/* __EMAIL_VIEWER_EML_H__ */
