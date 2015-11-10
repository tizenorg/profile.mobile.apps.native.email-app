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

#ifndef __DEF_EMAIL_MAILBOX_UTIL_H_
#define __DEF_EMAIL_MAILBOX_UTIL_H_


#define R_MASKING(val) (((val) & 0xff000000) >> 24)
#define G_MASKING(val) (((val) & 0x00ff0000) >> 16)
#define B_MASKING(val) (((val) & 0x0000ff00) >> 8)
#define A_MASKING(val) (((val) & 0x000000ff))

/* int account_id, email_folder_type_e folder_type */
#define GET_MAILBOX_ID(account_id, folder_type) \
		({\
			int _acct = account_id;\
			int _type = folder_type;\
			int _folder = 0;\
			email_mailbox_t *mbox = NULL;\
			int e = email_get_mailbox_by_mailbox_type(_acct, _type, &mbox);\
			if( e != EMAIL_ERROR_NONE || !mbox ) {\
				debug_warning("Error to get mailbox id acct(%d) type(%d) - err(%d) or mbox is NULL(%p)",\
								_acct, _type, e, mbox);\
			}\
			else _folder = mbox->mailbox_id;\
			if(mbox) email_free_mailbox(&mbox, 1);\
			_folder;\
		})

/* int mailbox_id */
#define GET_MAILBOX_TYPE(mailbox_id) \
		({\
			int _folder = mailbox_id;\
			email_mailbox_t *mbox = NULL;\
			int type = EMAIL_MAILBOX_TYPE_NONE;\
			int e = email_get_mailbox_by_mailbox_id(_folder, &mbox);\
			if( e != EMAIL_ERROR_NONE || !mbox )\
				debug_warning("email_get_mailbox_by_mailbox_id folder(%d) - err(%d) or mbox is NULL(%p)",\
				_folder, e, mbox);\
			else type = mbox->mailbox_type;\
			if(mbox) email_free_mailbox(&mbox, 1);\
			type;\
		})

#define GET_ACCOUNT_SERVER_TYPE(account_id) \
		({\
			email_account_t *email_account = NULL;\
			int server_type = 0;\
			int e = email_get_account(account_id, EMAIL_ACC_GET_OPT_DEFAULT, &email_account);\
			if (e != EMAIL_ERROR_NONE || !email_account) {\
				debug_warning("email_get_account acct(%d) - err(%d) or acct NULL(%p)",\
								account_id, e, email_account);\
			}\
			else server_type = email_account->incoming_server_type;\
			if(email_account) email_free_account(&email_account, 1);\
			server_type;\
		})

#define GET_ACCOUNT_EMAIL_SYNC_DISABLE_STATUS(account_id) \
		({\
			email_account_t *email_account = NULL;\
			int sync_disabled = 0;\
			int e = email_get_account(account_id, (EMAIL_ACC_GET_OPT_DEFAULT | EMAIL_ACC_GET_OPT_OPTIONS), &email_account);\
			if (e != EMAIL_ERROR_NONE || !email_account) {\
				debug_warning("email_get_account acct(%d) - err(%d) or acct NULL(%p)",\
								account_id, e, email_account);\
			}\
			else sync_disabled = email_account->sync_disabled;\
			if(email_account) email_free_account(&email_account, 1);\
			sync_disabled;\
		})

#define GET_INBOX_TO_GETMORE(account_id) \
		({\
			int _folder = GET_MAILBOX_ID(account_id, EMAIL_MAILBOX_TYPE_INBOX);\
			email_mailbox_t *mbox = NULL;\
			int i_getmore = 0;\
			int e = email_get_mailbox_by_mailbox_id(_folder, &mbox);\
			if( e != EMAIL_ERROR_NONE || !mbox )\
				debug_warning("email_get_mailbox_by_mailbox_id folder(%d) - err(%d) or mbox is NULL(%p)",\
				_folder, e, mbox);\
			else i_getmore = (mbox->total_mail_count_on_server-mbox->total_mail_count_on_local);\
			if(mbox) email_free_mailbox(&mbox, 1);\
			i_getmore;\
		})

#define GET_MAILBOX_TO_GETMORE(mailbox_id) \
		({\
			int _folder = mailbox_id;\
			email_mailbox_t *mbox = NULL;\
			int i_getmore = 0;\
			int e = email_get_mailbox_by_mailbox_id(_folder, &mbox);\
			if( e != EMAIL_ERROR_NONE || !mbox )\
				debug_warning("email_get_mailbox_by_mailbox_id folder(%d) - err(%d) or mbox is NULL(%p)",\
				_folder, e, mbox);\
			else i_getmore = (mbox->total_mail_count_on_server-mbox->total_mail_count_on_local);\
			if(mbox) email_free_mailbox(&mbox, 1);\
			i_getmore;\
		})

#define MAILBOX_UPDATE_GENLIST_ITEM_READ_STATE(gl, item, read) \
		({\
			Eina_List *realized_list = elm_genlist_realized_items_get(gl);\
			Elm_Object_Item *it = NULL;\
			Eina_List *l = NULL;\
			EINA_LIST_FOREACH(realized_list, l, it) {\
				if(item == it) {\
					if(read) {\
						elm_object_item_signal_emit(it, "check_mail_as_read", "mailbox");\
					} else{\
						elm_object_item_signal_emit(it, "check_mail_as_unread", "mailbox");\
					}\
					break;\
				}\
			}\
			eina_list_free(realized_list);\
		})

#define MAILBOX_UPDATE_GENLIST_ITEM_FLAG_STATE(gl, item, flag_type) \
		({\
			Eina_List *realized_list = elm_genlist_realized_items_get(gl);\
			Elm_Object_Item *it = NULL;\
			Eina_List *l = NULL;\
			EINA_LIST_FOREACH(realized_list, l, it) {\
				if(item == it) {\
					MailItemData *ld = (MailItemData *)elm_object_item_data_get(it);\
					if(ld) {\
						switch(flag_type) { \
							case EMAIL_FLAG_NONE:\
							elm_check_state_set(ld->check_favorite_btn, EINA_FALSE); \
							break;\
							case EMAIL_FLAG_FLAGED:\
							elm_check_state_set(ld->check_favorite_btn, EINA_TRUE); \
							break;\
						} \
						break;\
					}\
				}\
			}\
			eina_list_free(realized_list);\
		})

#define MAILBOX_UPDATE_REALIZED_GENLIST_ITEM_FLAG_STATE(item) \
		({\
			if(item) {\
				MailItemData *ld = (MailItemData *)elm_object_item_data_get(it);\
				if(ld) {\
					switch(ld->flag_type) { \
						case EMAIL_FLAG_NONE:\
						elm_check_state_set(ld->check_favorite_btn, EINA_FALSE); \
						break;\
						case EMAIL_FLAG_FLAGED:\
						elm_check_state_set(ld->check_favorite_btn, EINA_TRUE); \
						break;\
					} \
				}\
			}\
		})

#define MAILBOX_UPDATE_GENLIST_ITEM(gl, item) \
		({\
			Eina_List *realized_list = elm_genlist_realized_items_get(gl);\
			Elm_Object_Item *it = NULL;\
			Eina_List *l = NULL;\
			EINA_LIST_FOREACH(realized_list, l, it) {\
				if(item == it) {\
					elm_genlist_item_update(item);\
					break;\
				}\
			}\
			eina_list_free(realized_list);\
		})

#define MAILBOX_CEHCK_ITEM_IS_REALIZED_OR_NOT(gl, item) \
		({\
			Eina_List *realized_list = elm_genlist_realized_items_get(gl);\
			Elm_Object_Item *it = NULL;\
			Eina_List *l = NULL;\
			bool realized = 0;\
			EINA_LIST_FOREACH(realized_list, l, it) {\
				if(item == it) {\
					realized = 1;\
					break;\
				}\
			}\
			eina_list_free(realized_list);\
			realized;\
		})

#define SNPRINTF_OFFSET(base_buf, offset, base_size, format, args...) \
			({\
				int _offset = offset;\
				snprintf(base_buf + _offset, base_size - _offset - 1, format, ##args);\
			})

/**
 * @brief elm util function
 */
const char *email_get_mailbox_theme_path();

/**
 * @brief Clear Mailbox info data
 * @param[in]	mailbox_ugd		Email mailbox data
 */
void mailbox_clear_prev_mailbox_info(EmailMailboxUGD *mailbox_ugd);

/**
 * @brief Copies a byte string encoded to time format for last updated string
 * @param[in]	last_update_time	Structure represented time
 * @param[in]	mailbox_ugd			Email mailbox data
 */
void mailbox_set_last_updated_time(time_t last_update_time, EmailMailboxUGD *mailbox_ugd);

/**
 * @brief Compare received Mail ID if exist in Mail item data
 * @param[in]	a	Mail item data
 * @param[in]	b	Mail ID
 * @return If Mail ID exist return 0 if not 1
 */
int mailbox_compare_mailid_in_list(gconstpointer a, gconstpointer b);

/**
 * @brief Creates popup with duration
 * @param[in]	mailbox_ugd			Email mailbox data
 * @param[in]	title_text			Title text
 * @param[in]	contents_text		Content body text
 * @param[in]	timeout				time duration
 * @return Evas_Object with suitable popup notification, otherwise NULL
 */
Evas_Object *mailbox_create_timeout_popup(EmailMailboxUGD *mailbox_ugd, const char *title_text, const char *contents_text, double timeout);

/**
 * @brief Creates popup
 * @param[in]	mailbox_ugd			Email mailbox data
 * @param[in]	title				Title text
 * @param[in]	content				Content body text
 * @param[in]	_response_cb		Evas_Smart callback function for back button
 * @param[in]	btn1_response_cb	Evas_Smart callback function for button #1
 * @param[in]	btn1_text			String data for button #1
 * @param[in]	btn2_response_cb	Evas_Smart callback function for button #2
 * @param[in]	btn2_text			String data for button #2
 * @return Evas_Object with suitable popup notification, otherwise NULL
 */
Evas_Object *mailbox_create_popup(EmailMailboxUGD *mailbox_ugd, const char *title, const char *content, Evas_Smart_Cb _response_cb,
		Evas_Smart_Cb btn1_response_cb, const char *btn1_text, Evas_Smart_Cb btn2_response_cb, const char *btn2_text);
/**
 * @brief Creates error popup
 * @param[in]	error_type			Error type
 * @param[in]	account_id			Account ID
 * @param[in]	mailbox_ugd			Email mailbox data
 */
void mailbox_create_error_popup(int error_type, int account_id, EmailMailboxUGD *mailbox_ugd);

/**
 * @brief Creates password change popup
 * @param[in]	error_type			Error type
 * @param[in]	account_id			Account ID
 */
void mailbox_create_password_changed_popup(void *data, int account_id);

/**
 * @brief Check sort type validation
 * @param[in]	mailbox_ugd		Email mailbox data
 */
void mailbox_check_sort_type_validation(EmailMailboxUGD *mailbox_ugd);

/**
 * @brief Add account color to Email settings data Mailbox Account list
 * @param[in]	mailbox_ugd			Email mailbox data
 * @param[in]	account_id			Account ID
 * @param[in]	account_color		Account color
 */
void mailbox_account_color_list_add(EmailMailboxUGD *mailbox_ugd, int account_id, int account_color);

/**
 * @brief Pass the elements of the Mailbox Account Color list and update color to appropriate account ID
 * @param[in]	mailbox_ugd			Email mailbox data
 * @param[in]	account_id			Account ID
 * @param[in]	update_color		Account color RGB type
 */
void mailbox_account_color_list_update(EmailMailboxUGD *mailbox_ugd, int account_id, int update_color);

/**
 * @brief Pass the elements of the Mailbox Account Color list and delete color from appropriate account ID
 * @param[in]	mailbox_ugd		Email mailbox data
 * @param[in]	account_id		Account ID
 */
void mailbox_account_color_list_remove(EmailMailboxUGD *mailbox_ugd, int account_id);

/**
 * @brief Pass the elements of the Mailbox Account Color list and return color for appropriate account ID
 * @param[in]	mailbox_ugd			Email mailbox data
 * @param[in]	account_id			Account ID
 * @return color for appropriate account ID, otherwise 0
 */
int mailbox_account_color_list_get_account_color(EmailMailboxUGD *mailbox_ugd, int account_id);

/**
 * @brief Free all elements from Mailbox Account Color list
 * @param[in]	mailbox_ugd		Email mailbox data
 */
void mailbox_account_color_list_free(EmailMailboxUGD *mailbox_ugd);

/**
 * @brief Markup filter function for text inserted in the entry
 * @param[in]	entry				Entry object
 * @param[in]	max_char_count		limits by character count (disabled if 0)
 * @param[in]	max_byte_count		limits by bytes count (disabled if 0)
 */
void mailbox_set_input_entry_limit(Evas_Object *entry, int max_char_count, int max_byte_count);

#endif /*__DEF_EMAIL_MAILBOX_UTIL_H_*/
