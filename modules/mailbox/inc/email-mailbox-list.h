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

#ifndef __DEF_EMAIL_MAILBOX_LIST_H_
#define __DEF_EMAIL_MAILBOX_LIST_H_

/**
 * @brief Create content for Mailbox view
 * @param[in]	view		Email mailbox data
 */
void mailbox_list_create_view(EmailMailboxView *view);

/**
 * @brief Show content of mail
 * @param[in]	ld		Mail item data
 */
void mailbox_list_open_email_viewer(MailItemData *ld);

/**
 * @brief System settings callback register
 * @remark Operation registers system accessibility for font size changes
 * @param[in]	view		Email mailbox data
 */
void mailbox_list_system_settings_callback_register(EmailMailboxView *view);

/**
 * @brief System settings callback unregister
 * @remark Operation unregisters system accessibility for font size changes
 */
void mailbox_list_system_settings_callback_unregister();

/**
 * @brief Refresh content of mail list in Mailbox
 * @remark Optionally search tag can be added
 * @param[in]	view		Email mailbox data
 * @param[in]	search_data		Email search data
 */
void mailbox_list_refresh(EmailMailboxView *view, const email_search_data_t *search_data);

/**
 * @brief Insert item to Mailbox genlist
 * @param[in]	ld				Mail item data to attach
 * @param[in]	view		Email mailbox data
 */
void mailbox_list_insert_mail_item(MailItemData *ld, EmailMailboxView *view);

/**
 * @brief Insert item to Mailbox genlist used when needed check email if already consistent in Mailbox
 * @param[in]	temp_ld			Mail item data to attach
 * @param[in]	view		Email mailbox data
 */
void mailbox_list_insert_mail_item_from_noti(MailItemData **temp_ld, EmailMailboxView *view);

/**
 * @brief Remove email item from Mailbox genlist content
 * @param[in]	view		Email mailbox data
 * @param[in]	ld				Mail item data to be removed
 */
void mailbox_list_remove_mail_item(EmailMailboxView * view, MailItemData * ld);

/**
 * @brief Provides creation of Mail Item data from email service received data
 * @param[in]	mail_info		Email mail list item data
 * @param[in]	search_data		Email Search data
 * @param[in]	view		Email Mailbox data
 * @return MailItemData structure or NULL if none or an error occurred
 */
MailItemData *mailbox_list_make_mail_item_data(email_mail_list_item_t* mail_info, const email_search_data_t *search_data, EmailMailboxView *view);

/**
 * @brief Get Mail Item data structure from Mail List by Mail ID
 * @param[in]	mailid			Mail ID
 * @param[in]	mail_list		Mail list
 * @return MailItemData structure or NULL if none or an error occurred
 */
MailItemData *mailbox_list_get_mail_item_data_by_mailid(int mailid, GList *mail_list);

/**
 * @brief Get Mail Item data structure from Mail List by Thread ID
 * @param[in]	thread_id		Thread ID
 * @param[in]	mail_list		Mail list
 * @return MailItemData structure or NULL if none or an error occurred
 */
MailItemData *mailbox_list_get_mail_item_data_by_threadid(int thread_id, GList *mail_list);

/**
 * @brief Free Mail Item data structure
 * @param[in]	ld		Mail item data to be freed
 */
void mailbox_list_free_mail_item_data(MailItemData *ld);

/**
 * @brief Free all Mail Item data structures from Email Mailbox
 * @param[in]	view		Email Mailbox data
 */
void mailbox_list_free_all_item_data(EmailMailboxView *view);

/**
 * @brief Provides selection functionality for content of Mailbox
 * @param[in]	view		Email Mailbox data
 */
void mailbox_create_select_info(EmailMailboxView *view);

/**
 * @brief Updates select info
 * @param[in]	view		Email Mailbox data
 */
void mailbox_update_select_info(EmailMailboxView *view);

/**
 * @brief Provides exit from edit mode
 * @param[in]	data		User data (Email Mailbox data)
 */
void mailbox_exit_edit_mode(void *data);

/**
 * @brief Clear edit mode content data
 * @param[in]	view		Email Mailbox data
 */
void mailbox_clear_edit_mode_list(EmailMailboxView *view);

/**
 * @brief The main purpose of this function is to remove from selected items list recieved Mail Item data and update recent of the list
 * @param[in]	ld				Mail Item data
 * @param[in]	view		Email Mailbox data
 */
void mailbox_update_edit_list_view(MailItemData *ld, EmailMailboxView * view);

/**
 * @brief Remove all unnecessary items (prompts) from genlist content
 * @remark Next itmes will be removed 1) last updated time 2) load more emails 3) no more emails 4) get more emails
 * @param[in]	data		User data (Email Mailbox data)
 */
void mailbox_remove_unnecessary_list_item_for_edit_mode(void *data);

/**
 * @brief Provides filter by Email ID
 * @param[in]	mail_id		Mail ID
 * @param[in]	data		User data (Email Mailbox data)
 * @return TRUE on success or FALSE if none or an error occurred
 */
bool mailbox_check_searched_mail(int mail_id, void *data);

/**
 * @brief Add thread queue job request to make remaining items from AddRemainingMailReq data
 * @param[in]	view		Email Mailbox data
 * @param[in]	req				Email AddRemainingMailReq data
 */
void mailbox_list_make_remaining_items_in_thread(EmailMailboxView *view, AddRemainingMailReqData *req);

/**
 * @brief Release mailbox folders name cache
 * @param[in]	view			Email Mailbox data
 */
void mailbox_folders_name_cache_free(EmailMailboxView *view);

/**
 * @brief Add new item into folders name cache
 * @param[in]	view			Email Mailbox data
 * @param[in]	mailbox_id		Email Mailbox id which is unique for each mailbox
 * @param[in]	folder_name		Email folder name which is related to mailbox id
 */
void mailbox_folders_name_cashe_add_name(EmailMailboxView *view, int mailbox_id, const char *folder_name);

/**
 * @brief Add new item into folders name cache
 * @param[in]	view			Email Mailbox data
 * @param[in]	mailbox_id		Email Mailbox id which is unique for each mailbox
 * @return	folder_name of current mailbox id if ot os listed in cache, othervise NULL
 */
char *mailbox_folders_name_cashe_get_name(EmailMailboxView *view, int mailbox_id);

#endif	/* __DEF_EMAIL_MAILBOX_LIST_H_ */
