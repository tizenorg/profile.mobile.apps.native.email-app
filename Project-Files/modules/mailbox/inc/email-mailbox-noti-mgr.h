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

#ifndef __DEF_EMAIL_MAILBOX_NOTI_MGR_H_
#define __DEF_EMAIL_MAILBOX_NOTI_MGR_H_

/**
 * @brief Notification data from network communication
 */
typedef struct _noti_event {
	int operation;
	int data1;
	int data2;
	char* data3;
	int data4;
} noti_event;

/**
 * @brief Initialize module notification system
 * @param[in]	mailbox_ugd		Email mailbox data
 */
void mailbox_setup_dbus_receiver(EmailMailboxUGD *mailbox_ugd);

/**
 * @brief Remove module notification system
 */
void mailbox_remove_dbus_receiver();

/**
 * @brief Add request handle to request handle list
 * @param[in]	handle		Email engine handler request
 */
void mailbox_req_handle_list_add_handle(int handle);

/**
 * @brief Remove and free request handle list
 */
void mailbox_req_handle_list_free();

/**
 * @brief Remove and free sending mail list
 * @param[in]	mailbox_ugd		Email mailbox data
 */
void mailbox_sending_mail_list_free(EmailMailboxUGD *mailbox_ugd);

/**
 * @brief Remove and free request account list
 */
void mailbox_req_account_list_free();

#endif	/* __DEF_EMAIL_MAILBOX_NOTI_MGR_H_ */
