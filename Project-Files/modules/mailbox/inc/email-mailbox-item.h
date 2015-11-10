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

#ifndef __DEF_EMAIL_MAILBOX_ITEM_H_
#define __DEF_EMAIL_MAILBOX_ITEM_H_

/**
 * @brief Mailbox delete request mail data
 */
typedef struct _MailboxSelectedList {
	int account_id;
	int mail_id;
	int thread_id;
	int mailbox_type;
	email_mail_status_t mail_status;
} DeleteRequestedMail;

/**
 * @brief Mailbox requested mail list data
 */
typedef struct {
	Eina_List *requested_mail_list;
	EmailMailboxUGD *mailbox_ugd;
} edit_req_t;

/**
 * @brief Provides process of mail move accordingly to Mailbox data source and destination
 * @param[in]	mailbox_ugd		Email mailbox data
 */
void mailbox_process_move_mail(EmailMailboxUGD *mailbox_ugd);

/**
 * @brief Ecore thread heavy callback that provides delete mails routine
 * @param[in]	data	User data (Email mailbox data)
 * @param[in]	thd		The thread context the data belongs to.
 */
void mailbox_process_delete_mail(void *data, Ecore_Thread *thd);

#endif	/* __DEF_EMAIL_MAILBOX_ITEM_H_ */
