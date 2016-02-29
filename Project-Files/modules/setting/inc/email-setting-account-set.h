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

#ifndef __EMAIL_SETTING_ACCOUNT_SET_H__
#define __EMAIL_SETTING_ACCOUNT_SET_H__

#include "email-setting.h"
#include "email-setting-utils.h"

/**
 * @brief Set up an account setting
 * @remark Operation occurs with default provider.
 * @param[in]	view	View data
 */
void setting_set_others_account(email_view_t *view);

/**
 * @brief Set up an account provider server setting
 * @param[out]	account				Account instance
 * @param[in]	account_type		Type of account
 * @param[in]	incoming_protocol	Type of incoming protocol
 * @param[in]	outgoing_protocol	Type of outgoing protocol
 */
void setting_set_others_account_server_default_type(email_account_t *account, int account_type,
		int incoming_protocol, int outgoing_protocol);


/**
 * @brief Check if email address domain is from default providers list
 * @param[in]	view			View data
 * @param[in]	email_addr	The email address name
 * @return TRUE if email address domain is from default providers list, otherwise FALSE
 */
int setting_is_in_default_provider_list(email_view_t *view, const char *email_addr);

/**
 * @brief Set default provider info to account.
 * @remark Operation occurs by server configuration with default provider.
 * @param[in]	view		View data
 * @param[in]	account	Account data
 */
void setting_set_default_provider_info_to_account(email_view_t *view, email_account_t *account);

#endif				/* __EMAIL_SETTING_ACCOUNT_SET_H__ */

/* EOF */
