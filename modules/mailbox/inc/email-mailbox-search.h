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

#ifndef __DEF_EMAIL_MAILBOX_SEARCH_H_
#define __DEF_EMAIL_MAILBOX_SEARCH_H_

/**
 * @brief Cancel search mode in Mailbox and return to normal view
 * @param[in]	view		Email mailbox data
 */
void mailbox_finish_search_mode(EmailMailboxView *view);

/**
 * @brief Free Email search data
 * @param[in]	search_data		Email search data
 */
void mailbox_free_mailbox_search_data(email_search_data_t *search_data);

/**
 * @brief Callback that provides click on search button
 * @param[in]	data		User data (Email mailbox data)
 */
void _search_button_clicked_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED);

/**
 * @brief Callback that provides click on search button
 * @param[in]	view				Email mailbox data
 * @param[in]	show_search_layout		Bool value to define the search state
 */
void mailbox_change_search_layout_state(EmailMailboxView *view, bool show_search_layout);

/**
 * @brief Create and initialize Email search data
 * @param[in]	data				User data (Email mailbox data)
 */
email_search_data_t *mailbox_make_search_data(EmailMailboxView *view);

/**
 * @brief Show search result
 * @param[in]	view			Email mailbox data
 */
int mailbox_show_search_result(EmailMailboxView *view);

#endif	/* __DEF_EMAIL_MAILBOX_SEARCH_H_ */
