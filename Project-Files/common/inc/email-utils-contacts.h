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
typedef struct _email_contact_list_info {
	int index;
	int person_id;
	int email_id;
	char *display;
	char *email_address;
	char *image_path;
	char display_name[EMAIL_LIMIT_EMAIL_ADDRESS_LENGTH + 1];
	void *ugd;
	int email_type; // Type of email address
	email_contact_search_origin_type_e contact_origin;
} email_contact_list_info_t;

/*
 * Exported fuctions.
 */
EMAIL_API int email_contacts_init_service();
EMAIL_API int email_contacts_finalize_service();

EMAIL_API email_contact_list_info_t *email_contacts_get_contact_info_by_email_id(int email_id);
EMAIL_API email_contact_list_info_t *email_contacts_get_contact_info_by_email_address(const char *email_address);
EMAIL_API void email_contacts_delete_contact_info(email_contact_list_info_t **contacts_list_item);
EMAIL_API Eina_List *email_contacts_search_contacts_by_keyword(const char *search_word);

EMAIL_API int email_contacts_get_email_address_by_contact_id(int contact_id, char **email_address);
EMAIL_API int email_contacts_get_contact_name_by_email_address(char *email_address, char **contact_name);

EMAIL_API char *email_contacts_create_vcard_from_id(int id, const char *vcard_dir_path, Eina_Bool my_profile);
EMAIL_API char *email_contacts_create_vcard_from_id_list(const int *id_list, int count, const char *vcard_dir_path, volatile Eina_Bool *cancel);

G_END_DECLS
#endif	/* _EMAIL_UTILS_CONTACTS_H_ */

/* EOF */
