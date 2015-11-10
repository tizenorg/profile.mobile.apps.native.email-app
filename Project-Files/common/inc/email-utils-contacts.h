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

#ifndef _EMAIL_UTILS_CONTACTS_H_
#define _EMAIL_UTILS_CONTACTS_H_

#include <contacts.h>

#include "email-common-types.h"

G_BEGIN_DECLS

/*
 * Contact service2
 */
typedef struct _EMAIL_CONTACT_LIST_INFO_S {
	int index;
	int person_id;
	char *display;
	char *email_address;
	char *image_path;
	char display_name[EMAIL_LIMIT_EMAIL_ADDRESS_LENGTH + 1];
	void *ugd;
	int email_type; // Type of email address
	EmailContactSearchOriginType contact_origin;
} EMAIL_CONTACT_LIST_INFO_S;

/*
 * Exported fuctions.
 */
EMAIL_API char *get_email_type_str(int type);

EMAIL_API int email_get_contacts_list_int(contacts_match_int_flag_e match, contacts_list_h *list, int search_num);
EMAIL_API int email_get_contacts_list(contacts_match_str_flag_e match, contacts_list_h *list, const char *search_word, EmailContactSearchType search_type);
EMAIL_API int email_get_contacts_index(contacts_record_h record, int *index);
EMAIL_API int email_get_contacts_display_name(contacts_record_h record, char **display_name);
EMAIL_API int email_get_contacts_email_address(contacts_record_h record, char **email_addr);
EMAIL_API int email_get_contacts_first_name(contacts_record_h record, char **first_name);
EMAIL_API int email_get_contacts_last_name(contacts_record_h record, char **last_name);
EMAIL_API int email_get_contacts_list_info(contacts_list_h list, EMAIL_CONTACT_LIST_INFO_S *contact_list_info);
EMAIL_API int email_get_phone_log(contacts_match_str_flag_e match, contacts_list_h *list, const char *search_word);
EMAIL_API int email_get_phone_log_info(contacts_list_h list, EMAIL_CONTACT_LIST_INFO_S *ct_list_info);
EMAIL_API int email_get_last_contact_in_contact_list(contacts_list_h list, contacts_record_h *last_contact);
EMAIL_API int email_num_id_get_contacts_record(int num_id, contacts_record_h *out_record);
EMAIL_API void *email_contact_search_by_email(void *ug_data, const char *search_word);
EMAIL_API void email_delete_contacts_list(EMAIL_CONTACT_LIST_INFO_S **contacts_list_item);

G_END_DECLS
#endif	/* _EMAIL_UTILS_CONTACTS_H_ */

/* EOF */
