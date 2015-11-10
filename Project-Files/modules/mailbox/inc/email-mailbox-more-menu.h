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

#ifndef __DEF_EMAIL_MAILBOX_MORE_MENU_H_
#define __DEF_EMAIL_MAILBOX_MORE_MENU_H_

/**
 * @brief Creates the toolbar more button for Mailbox viewer
 * @param[in]	mailbox_ugd			Email mailbox data
 * @return Evas_Object instance with suitable more button, otherwise NULL
 */
Evas_Object *mailbox_create_toolbar_more_btn(EmailMailboxUGD *mailbox_ugd);

/**
 * @brief Delete emails from selected mail list in Email mailbox, at the end shows popup that deletion operation is done
 * @param[in]	data			Email mailbox data
 */
void mailbox_delete_mail(void *data);

/**
 * @brief Provides move emails from selected mail list in Email mailbox, at the end shows popup that move operation is done
 * @param[in]	data			Email mailbox data
 */
void mailbox_move_mail(void *data);

/**
 * @brief Move emails to spam from selected mail list in Email mailbox, at the end shows popup that move to spam operation is done
 * @param[in]	data			Email mailbox data
 */
void mailbox_to_spam_mail(void *data);

/**
 * @brief Move emails out of spam from selected mail list in Email mailbox, at the end shows popup that move to spam operation is done
 * @param[in]	data			Email mailbox data
 */
void mailbox_from_spam_mail(void *data);

/**
 * @brief Mark unread messages
 * @param[in]	data			Email mailbox data
 */
void mailbox_markunread_mail(void *data);

/**
 * @brief Mark read messages
 * @param[in]	data			Email mailbox data
 */
void mailbox_markread_mail(void *data);

/**
 * @brief Creates floating button on Mailbox viewer to provide compose of new message
 * @param[in]	mailbox_ugd			Email mailbox data
 */
void mailbox_create_compose_btn(EmailMailboxUGD *mailbox_ugd);

/**
 * @brief Show on Mailbox viewer previously created compose new message floating button
 * @param[in]	mailbox_ugd			Email mailbox data
 */
void mailbox_show_compose_btn(EmailMailboxUGD *mailbox_ugd);

/**
 * @brief Hide compose new message floating button
 * @param[in]	mailbox_ugd			Email mailbox data
 */
void mailbox_hide_compose_btn(EmailMailboxUGD *mailbox_ugd);


#endif	/* __DEF_EMAIL_MAILBOX_MORE_MENU_H_ */
