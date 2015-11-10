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

#ifndef __DEF_EMAIL_MAILBOX_SYNC_H_
#define __DEF_EMAIL_MAILBOX_SYNC_H_

/**
 * @brief Provides synchronization for current Mailbox with email service
 * @param[in]	mailbox_ugd		Email mailbox data
 * @return TRUE on success or FALSE if none or an error occurred
 */
bool mailbox_sync_current_mailbox(EmailMailboxUGD *mailbox_ugd);

/**
 * @brief Provides synchronization for more messages request from email service
 * @param[in]	mailbox_ugd		Email mailbox data
 * @return TRUE on success or FALSE if none or an error occurred
 */
bool mailbox_sync_more_messages(EmailMailboxUGD *mailbox_ugd);

/**
 * @brief Cancel all synchronization processes email service
 * @param[in]	mailbox_ugd		Email mailbox data
 */
void mailbox_sync_cancel_all(EmailMailboxUGD *mailbox_ugd);

/**
 * @brief Provides synchronization for current folder
 * @param[in]	account_id		Account ID
 * @param[in]	mailbox_id		Moilbox ID
 * @param[in]	data			User data (Email mailbox data)
 */
void mailbox_sync_folder_with_new_password(int account_id, int mailbox_id, void *data);

#endif				/* __DEF_EMAIL_MAILBOX_SYNC_H_ */
