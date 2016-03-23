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

#ifndef _EMAIL_ENGINE_H_
#define _EMAIL_ENGINE_H_

#include <glib.h>
#include <email-api.h>
#include "email-common-types.h"

G_BEGIN_DECLS

/* Array length definition */

#define TAG_LEN		50 /*tag for hightlight keyword <color=#ff0000>key</>*/
#define FROM_LEN		100
#define SUBJECT_LEN	100
#define ADDR_LEN		320
#define RECIPIENT_LEN	100

typedef struct {
	gint uid;
	gint account_id;
	gchar from[FROM_LEN + TAG_LEN + 1];
	gchar sender[ADDR_LEN + 1];
	guint from_count;		// will be removed
	char recipient[RECIPIENT_LEN + 1];
	gchar subject[SUBJECT_LEN + TAG_LEN + 1];
	gint mailbox_id;
	time_t datetime;
	gint priority;
	gint flags;		/* See email_mail_status_type_e in email-types.h. */
	gboolean seen : 1;
	gboolean attachments : 1;
	gboolean lock : 1;
	gboolean body_download : 1;
	gboolean sending : 1;
	gboolean checked : 1;
	gboolean reserved_1 : 1;
	gboolean reserved_2 : 1;
	gint thread_id;
	gint thread_count;
	gint save_status;
} email_mailbox_info_t;

typedef struct {
	gchar *name;
	gchar *alias;
	gchar *account_name;
	gchar *mail_addr;
	gchar *thumb_path;
	gboolean has_archived;
	gint unread_count;
	GList *sub_list;
} email_mailbox_list_info_t;

typedef struct {
	gchar *account_name;
	gchar *email_address;
	gchar *user_name;
	gchar *password;
	gchar *receiving_address;
	gchar *smtp_address;
	gchar *smtp_user_name;
	gchar *smtp_password;
	gint smtp_auth;
	gint smtp_port;
	gint same_as;
	gint receiving_port;
	gint receiving_type;
	gint smtp_ssl;
	gint receiving_ssl;
	gint download_mode;
} email_account_info_t;

typedef struct {
	gchar *name;
	gchar *mail_addr;
	gchar *thumb_path;
	gint account_id;
} email_account_list_info_t;

#define GET_ALIAS_FROM_FULL_ADDRESS(full_address, address, len) \
		({\
			char *_alias = NULL;\
			if (STR_VALID(full_address)) {\
				gchar **token = NULL;\
				token = g_strsplit_set(full_address, "\"", -1);\
				if (token && STR_VALID(token[1]) && FALSE == STR_CMP0(token[1], " ")) {\
					_alias = STRNDUP(token[1], len);\
				} else { \
					debug_secure("***FROM == [%s], email_address_sender : [%s]", full_address, address);\
					if (STR_VALID(address)) {\
						_alias = STRNDUP(address, len);\
					} \
				}\
				g_strfreev(token);\
			} \
			_alias;\
		})

#define GET_RECIPIENT_FROM_FULL_ADDRESS(full_address, len) \
		({\
			char *_recipient = NULL;\
			if (STR_VALID(full_address)) {\
				gchar **token = NULL;\
				token = g_strsplit_set(full_address, "\"<>", -1);\
				int token_len = g_strv_length(token);\
				if (token && token_len > 1 && STR_VALID(token[1])) {\
					_recipient = STRNDUP(token[1], len);\
				} else if (token && token_len > 2 && STR_VALID(token[2])) {\
					_recipient = STRNDUP(token[2], len);\
				} else { \
					debug_secure("full_address [%s]", full_address);\
					_recipient = STRNDUP(email_get_email_string("IDS_EMAIL_SBODY_NO_RECIPIENTS_M_NOUN"), len);\
				}\
				g_strfreev(token);\
			} else\
				_recipient = STRNDUP(email_get_email_string("IDS_EMAIL_SBODY_NO_RECIPIENTS_M_NOUN"), len);\
			_recipient;\
		})


EMAIL_API gboolean email_engine_initialize(void);
EMAIL_API gboolean email_engine_initialize_force(void);
EMAIL_API void email_engine_finalize(void);
EMAIL_API void email_engine_finalize_force(void);
EMAIL_API gboolean email_engine_add_account_with_validation(email_account_t *_account, int *account_id, int *handle, int *error_code);
EMAIL_API gboolean email_engine_validate_account(email_account_t *_account, int *handle, int *error_code);
EMAIL_API gboolean email_engine_add_account(email_account_t *_account, int *account_id, int *error_code);
EMAIL_API gboolean email_engine_sync_imap_mailbox_list(gint account_id, int *handle);
EMAIL_API gboolean email_engine_update_account_with_validation(gint account_id, email_account_t *_account);
EMAIL_API gboolean email_engine_update_account(gint account_id, email_account_t *_account);
EMAIL_API gboolean email_engine_delete_account(gint account_id);
EMAIL_API gboolean email_engine_get_account_list(int *count, email_account_t **_account_list);
EMAIL_API gboolean email_engine_free_account_list(email_account_t **_account_list, int count);
EMAIL_API gboolean email_engine_get_account_data(int acctid, int options, email_account_t **account);
EMAIL_API gboolean email_engine_get_account_full_data(int acctid, email_account_t **account);
EMAIL_API gboolean email_engine_get_account_validation(email_account_t *_account, int *handle);
EMAIL_API gboolean email_engine_get_folder_list(gint account_id, int *handle);
EMAIL_API gboolean email_engine_get_default_account(gint *account_id);
EMAIL_API gboolean email_engine_set_default_account(gint account_id);

EMAIL_API email_mail_list_item_t *email_engine_get_mail_by_mailid(int mail_id);
EMAIL_API gchar *email_engine_get_mailbox_service_name(const gchar *folder_name);	/* g_free(). */

/* See EmailMailboxListInfo. */
EMAIL_API email_mailbox_list_info_t *email_engine_get_mailbox_info(gint account_id, const gchar *folder_name);
EMAIL_API void email_engine_free_mailbox_info(email_mailbox_list_info_t **info);

/* See EmailMailboxInfo. */
EMAIL_API void email_engine_free_mail_list(GList **list);
EMAIL_API guint email_engine_get_mail_list(int account_id, int mailbox_id, email_mail_list_item_t **mail_list, int *mail_count);
EMAIL_API guint email_engine_get_mail_list_count(gint account_id, const gchar *folder_name);

EMAIL_API gboolean email_engine_sync_folder(gint account_id, int mailbox_id, int *handle);
EMAIL_API gboolean email_engine_sync_all_folder(gint account_id, int *handle1, int *handle2);
EMAIL_API void email_engine_stop_working(gint account_id, int handle);

EMAIL_API void email_engine_free_mail_sender_list(GList **list);

EMAIL_API gboolean email_engine_check_seen_mail(gint account_id, gint mail_id);
EMAIL_API gboolean email_engine_check_draft_mail(gint account_id, gint mail_id);
EMAIL_API int email_engine_check_body_download(int mail_id);
EMAIL_API gboolean email_engine_body_download(gint account_id, gint mail_id, gboolean remove_rights, int *handle);
EMAIL_API gboolean email_engine_attachment_download(gint mail_id, gint index, int *handle);

EMAIL_API gboolean email_engine_delete_mail(gint account_id, int mailbox_id, gint mail_id, int sync);
EMAIL_API gboolean email_engine_delete_mail_list(gint account_id, int mailbox_id, GList *id_list, int sync);
EMAIL_API gboolean email_engine_delete_all_mail(gint account_id, int mailbox_id, int sync);
EMAIL_API gboolean email_engine_move_mail(int mailbox_id, gint mail_id);
EMAIL_API int email_engine_move_mail_list(int dst_mailbox_id, GList *id_list, int src_mailbox_id);
EMAIL_API gboolean email_engine_move_all_mail(gint account_id, int old_mailbox_id, int new_mailbox_id);
EMAIL_API gint email_engine_get_unread_mail_count(gint account_id, const gchar *folder_name);

EMAIL_API gchar *email_engine_get_attachment_path(gint attach_id);	/* g_free(). */

EMAIL_API gboolean email_engine_get_account_info(gint account_id, email_account_info_t **account_info);
EMAIL_API gboolean email_engine_set_account_info(gint account_id, email_account_info_t *account_info);
EMAIL_API void email_engine_free_account_info(email_account_info_t **account_info);

EMAIL_API int email_engine_get_mailbox_list(int account_id, email_mailbox_t **mailbox_list);
EMAIL_API int email_engine_get_mailbox_list_with_mail_count(int account_id, email_mailbox_t **mailbox_list);
EMAIL_API void email_engine_free_mailbox_list(email_mailbox_t **mailbox_list, int count);

EMAIL_API gchar *email_engine_convert_from_folder_to_srcbox(gint account_id, email_mailbox_type_e mailbox_type);
EMAIL_API GList *email_engine_get_ca_mailbox_list_using_glist(int account_id);
EMAIL_API void email_engine_free_ca_mailbox_list_using_glist(GList **mailbox_list);
EMAIL_API gboolean email_engine_get_smtp_mail_size(gint account_id, int *handle);

EMAIL_API int email_engine_get_max_account_id(void);
EMAIL_API int email_engine_get_count_account(void);

EMAIL_API gboolean email_engine_get_mailbox_by_mailbox_type(int account_id, email_mailbox_type_e mailbox_type, email_mailbox_t **mailbox);
EMAIL_API gboolean email_engine_get_mailbox_by_mailbox_id(int mailbox_id, email_mailbox_t **mailbox);

EMAIL_API int email_engine_delete_mailbox(int mailbox_id, int on_server, int *handle);
EMAIL_API int email_engine_rename_mailbox(int mailbox_id, char *mailbox_name, char *mailbox_alias, int on_server, int *handle);
EMAIL_API int email_engine_add_mailbox(email_mailbox_t *new_mailbox, int on_server, int *handle);

EMAIL_API int email_engine_get_task_information(email_task_information_t **task_information, int *task_information_count);

EMAIL_API gboolean email_engine_get_rule_list(email_rule_t **filtering_set, int *count);
EMAIL_API gboolean email_engine_add_rule(email_rule_t *filtering_set);
EMAIL_API gboolean email_engine_update_rule(int filter_id, email_rule_t *new_set);
EMAIL_API gboolean email_engine_delete_rule(int filter_id);
EMAIL_API gboolean email_engine_apply_rule(int filter_id);
EMAIL_API gboolean email_engine_free_rule(email_rule_t **filtering_set, int count);

EMAIL_API gboolean email_engine_get_password_length_of_account(int account_id,
		email_get_password_length_type password_type, int *password_length);

EMAIL_API gboolean email_engine_get_mail_data(int mail_id, email_mail_data_t **mail_data);
EMAIL_API gboolean email_engine_get_attachment_data_list(int mail_id,
		email_attachment_data_t **attachment_data, int *attachment_count);
EMAIL_API gboolean email_engine_parse_mime_file(char *eml_file_path, email_mail_data_t **mail_data,
		email_attachment_data_t **attachment_data, int *attachment_count);

EMAIL_API void email_engine_free_mail_data_list(email_mail_data_t** mail_list, int count);
EMAIL_API void email_engine_free_attachment_data_list(
		email_attachment_data_t **attachment_data_list, int attachment_count);

EMAIL_API int email_engine_add_mail(email_mail_data_t *mail_data, email_attachment_data_t *attachment_data_list,
		int attachment_count, email_meeting_request_t* meeting_request, int from_eas);
EMAIL_API gboolean email_engine_send_mail(int mail_id, int *handle);
EMAIL_API gboolean email_engine_send_mail_with_downloading_attachment_of_original_mail(int mail_id, int *handle);
EMAIL_API gboolean email_engine_set_flags_field(int account_id, int *mail_ids, int num,
		email_flags_field_type field_type, int value, int onserver);
EMAIL_API gboolean email_engine_update_mail_attribute(int account_id, int *mail_ids, int num,
		email_mail_attribute_type attribute_type, email_mail_attribute_value_t value);

G_END_DECLS
#endif	/* _EMAIL_ENGINE_H_ */

/* EOF */
