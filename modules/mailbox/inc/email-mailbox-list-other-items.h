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

#ifndef __DEF_EMAIL_MAILBOX_LIST_OTHER_ITEMS_H_
#define __DEF_EMAIL_MAILBOX_LIST_OTHER_ITEMS_H_

/**
 * @brief Provides adding the last updated time item to Mailbox viewer
 * @param[in]	view			Email mailbox data
 * @param[in]	show_directly»		If TRUE immediately bring scroller to the item
 */
void mailbox_last_updated_time_item_add(EmailMailboxView *view, bool show_directly);

/**
 * @brief Provides request from email service for last updated time with specified Mailbox ID and shows result in Mailbox viewer item
 * @param[in]	mailbox_id»		The Mailbox ID from email service
 * @param[in]	view		Email mailbox data
 */
void mailbox_last_updated_time_item_update(int mailbox_id, EmailMailboxView *view);

/**
 * @brief Free and release last updated time item from Mailbox viewer
 * @param[in]	view		Email mailbox data
 */
void mailbox_last_updated_time_item_remove(EmailMailboxView *view);

/**
 * @brief Provides registration of callbacks for refresh operation in Mailbox viewer
 * @remark To genlist "edge,bottom" callback is added and gesture layout with flicks callbacks is added as well
 * @param[in]	view		Email mailbox data
 */
void mailbox_refresh_flicks_cb_register(EmailMailboxView *view);

/**
 * @brief When waiting for refresh for a new messages in Mailbox viewer the function provides appearance of progress item in Mailbox viewer
 * @param[in]	view		Email mailbox data
 */
void mailbox_get_more_progress_item_add(EmailMailboxView *view);

/**
 * @brief Free and release progress time item from Mailbox viewer
 * @param[in]	view		Email mailbox data
 */
void mailbox_get_more_progress_item_remove(EmailMailboxView *view);

/**
 * @brief Provides adding the load more messages item to Mailbox viewer
 * @param[in]	view		Email mailbox data
 */
void mailbox_load_more_messages_item_add(EmailMailboxView *view);

/**
 * @brief Free and release load more messages item from Mailbox viewer
 * @param[in]	view		Email mailbox data
 */
void mailbox_load_more_messages_item_remove(EmailMailboxView *view);

/**
 * @brief Provides functionality for send all messages from outbox in Mailbox viewer
 * @param[in]	view		Email mailbox data
 */
void mailbox_send_all_btn_add(EmailMailboxView *view);

/**
 * @brief Free and release functionality of send all messages from Mailbox viewer
 * @param[in]	view		Email mailbox data
 */
void mailbox_send_all_btn_remove(EmailMailboxView *view);

/**
 * @brief Provides functionality for select all messages to Mailbox viewer
 * @param[in]	view		Email mailbox data
 */
void mailbox_select_all_item_add(EmailMailboxView *view);

/**
 * @brief Free and release functionality for select all messages from Mailbox viewer
 * @param[in]	view		Email mailbox data
 */
void mailbox_select_all_item_remove(EmailMailboxView *view);

/**
 * @brief Provides adding the "no more emails" item to Mailbox viewer
 * @param[in]	view		Email mailbox data
 */
void mailbox_no_more_emails_item_add(EmailMailboxView *view);

/**
 * @brief Free and release for "no more emails" from Mailbox viewer
 * @param[in]	view		Email mailbox data
 */
void mailbox_no_more_emails_item_remove(EmailMailboxView *view);

/**
 * @brief Provides adding the "circle refresh progress bar" item to Mailbox list in case of mailbox refreshing mode
 * @param[in]	view		Email mailbox data
 */
void mailbox_add_refresh_progress_bar(EmailMailboxView *view);

/**
 * @brief Free and release for "circle refresh progress bar" from Mailbox viewer
 * @param[in]	view		Email mailbox data
 */
void mailbox_remove_refresh_progress_bar(EmailMailboxView *view);

#endif	/* __DEF_EMAIL_MAILBOX_LIST_OTHER_ITEMS_H_ */
