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

#ifndef _EMAIL_MAILBOX_REQUEST_H_
#define _EMAIL_MAILBOX_REQUEST_H_

#include "email-request-queue.h"

/**
 * @brief Mail delete request data
 */
typedef struct {
	int account_id;
	int mailbox_type;
	char *msg_buf;
	EmailMailboxView *view;
} DeleteMailReqData;

/**
 * @brief Mail move request data
 */
typedef struct {
	int account_id;
	int mailbox_type;
	char *msg_buf;
	EmailMailboxView *view;
} MoveMailReqData;

/**
 * @brief Request to add mail data
 */
typedef struct {
	int account_id;
	int thread_id;
	int mail_id;
	int mailbox_id;
	email_mailbox_type_e mailbox_type;
	bool need_increase_mail_count;
	EmailMailboxMode mode;
	EmailMailboxView *view;
} AddMailReqData;

/**
 * @brief Request to add remaining mail data
 */
typedef struct add_remaining_mail_req_data {
	email_mail_list_item_t *mail_list;
	int start;
	int count;
	email_search_data_t *search_data;
	EmailMailboxView *view;
} AddRemainingMailReqData;

/**
 * @brief Request to delete a mail data to work with with feedback
 */
typedef struct {
	MailItemData *ld;
	int mail_id;
} DeleteMailReqReturnData;

/**
 * @brief Request to move a mail data with feedback
 */
typedef struct {
	int mail_id;
	MailItemData *ld;
	bool is_delete;
} MoveMailReqReturnData;

/**
 * @brief Callback that set main thread busy flag
 */
void mailbox_set_main_thread_busy_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *o EINA_UNUSED, void *info EINA_UNUSED);

/**
 * @brief Callback that reset main thread busy flag
 */
void mailbox_reset_main_thread_busy_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *o EINA_UNUSED, void *info EINA_UNUSED);

/**
 * @brief Provides registration for Mailbox requests
 */
void mailbox_requests_cbs_register();

/**
 * @brief Unregister Mailbox requests
 */
void mailbox_requests_cbs_unregister();

/**
 * @brief If in Mailbox exist request queue it cancle all requests
 * @param[in]	view		Email mailbox data
 */
void mailbox_cancel_all_requests(EmailMailboxView *view);

#endif /*_EMAIL_MAILBOX_REQUEST_H_*/
