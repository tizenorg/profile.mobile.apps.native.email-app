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

#ifndef __DEF_EMAIL_MAILBOX_TITLE_H_
#define __DEF_EMAIL_MAILBOX_TITLE_H_

/**
 * @brief Pack title layout to Mailbox view content
 * @param[in]	view		Email mailbox data
 */
void mailbox_view_title_pack(EmailMailboxView *view);

/**
 * @brief Unpack title from Mailbox view
 * @param[in]	view		Email mailbox data
 */
void mailbox_view_title_unpack(EmailMailboxView *view);

/**
 * @brief Updates title for Mailbox view
 * @param[in]	view		Email mailbox data
 */
void mailbox_view_title_update_all(EmailMailboxView *view);

/**
 * @brief Apply updated title to Mailbox view
 * @param[in]	view		Email mailbox data
 */
void mailbox_view_title_update_all_without_mailbox_change(EmailMailboxView *view);

/**
 * @brief Update account name for Mailbox view title
 * @param[in]	view		Email mailbox data
 */
void mailbox_view_title_update_account_name(EmailMailboxView *view);

/**
 * @brief Actual updates of mail count for Mailbox view title
 * @param[in]	view		Email mailbox data
 */
void mailbox_view_title_update_mail_count(EmailMailboxView *view);

/**
 * @brief Updates Mailbox view title for total and unread mails
 * @param[in]	total_count		Count of total mails
 * @param[in]	unread_count	Count of unread mails
 * @param[in]	view		Email mailbox data
 */
void mailbox_view_title_increase_mail_count(int total_count, int unread_count, EmailMailboxView *view);

#endif				/* __DEF_EMAIL_MAILBOX_TITLE_H_ */
