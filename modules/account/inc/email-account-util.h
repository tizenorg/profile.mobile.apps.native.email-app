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

#ifndef __EMAIL_ACCOUNT_UTIL_H__
#define __EMAIL_ACCOUNT_UTIL_H__

#include "email-account.h"

#define R_MASKING(val) (((val) & 0xff000000) >> 24)
#define G_MASKING(val) (((val) & 0x00ff0000) >> 16)
#define B_MASKING(val) (((val) & 0x0000ff00) >> 8)
#define A_MASKING(val) (((val) & 0x000000ff))


void account_stop_emf_job(EmailAccountView *view, int handle);
void account_sync_cancel_all(EmailAccountView *view);

char *account_convert_folder_alias_by_mailbox(email_mailbox_t *mlist);
char *account_convert_folder_alias_by_mailbox_type(email_mailbox_type_e mailbox_type);

void account_update_folder_item_dim_state(EmailAccountView *view);
void account_update_view_title(EmailAccountView *view);
char *account_get_user_email_address(int account_id);

Evas_Object *account_create_entry_popup(EmailAccountView *view, email_string_t t_title,
		const char *entry_text, const char *entry_selection_text,
		Evas_Smart_Cb _back_response_cb, Evas_Smart_Cb _done_key_cb,
		Evas_Smart_Cb btn1_response_cb, const char *btn1_text, Evas_Smart_Cb btn2_response_cb, const char *btn2_text);

char *account_get_ellipsised_folder_name(EmailAccountView *view, char *org_filename);
char *account_util_convert_mutf7_to_utf8(char *mailbox_name);
int account_color_list_get_account_color(EmailAccountView *view, int account_id);
void account_color_list_free(EmailAccountView *view);
void account_color_list_add(EmailAccountView *view, int account_id, int account_color);
void account_color_list_update(EmailAccountView *view, int account_id, int update_color);

char *account_get_folder_icon_name_by_mailbox_type(int mailbox_type);
Evas_Object *account_create_folder_icon(Evas_Object *parent, const char *folder_image_name);

const char *email_get_account_theme_path();

#endif	/* __EMAIL_ACCOUNT_UTIL_H__ */
