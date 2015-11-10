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

#include <stdio.h>

#include "email-debug.h"
#include "email-utils-contacts.h"
#include "email-locale.h"
#include "email-utils.h"


/*
 * Function implementation
 */

/* Contact service2 */
EMAIL_API int email_get_contacts_list_int(contacts_match_int_flag_e match, contacts_list_h *list, int search_num)
{
	debug_enter();
	int ct_ret = CONTACTS_ERROR_NONE;

	contacts_query_h query = NULL;
	contacts_filter_h filter = NULL;

	ct_ret = contacts_query_create(_contacts_contact_email._uri, &query);
	ct_ret = contacts_filter_create(_contacts_contact_email._uri, &filter);
	ct_ret = contacts_filter_add_int(filter, _contacts_contact_email.contact_id, CONTACTS_MATCH_EQUAL, search_num);
	ct_ret = contacts_query_set_filter(query, filter);

	ct_ret = contacts_db_get_records_with_query(query, 0, 0, list);
	ct_ret = contacts_filter_destroy(filter);
	ct_ret = contacts_query_destroy(query);

	if (ct_ret != CONTACTS_ERROR_NONE) {
		debug_log("contacts_query_destroy is failed. error_code = %d", ct_ret);
	}
	return ct_ret;
}

EMAIL_API int email_get_contacts_list(contacts_match_str_flag_e match, contacts_list_h *list, const char *search_word, EmailContactSearchType search_type)
{
	debug_enter();
	int ct_ret = CONTACTS_ERROR_NONE;

	contacts_query_h query = NULL;
	contacts_filter_h filter = NULL;
	debug_secure("search_word = %s", search_word);

	/* get number list first */
	ct_ret = contacts_query_create(_contacts_contact_email._uri, &query);
	ct_ret = contacts_filter_create(_contacts_contact_email._uri, &filter);
	if (search_type == EMAIL_SEARCH_CONTACT_BY_NAME)
		ct_ret = contacts_filter_add_str(filter, _contacts_contact_email.display_name, match, search_word);
	else if (search_type == EMAIL_SEARCH_CONTACT_BY_EMAIL)
		ct_ret = contacts_filter_add_str(filter, _contacts_contact_email.email, match, search_word);
	ct_ret = contacts_query_set_filter(query, filter);

	ct_ret = contacts_db_get_records_with_query(query, 0, 0, list);
	ct_ret = contacts_filter_destroy(filter);
	ct_ret = contacts_query_destroy(query);

	if (ct_ret != CONTACTS_ERROR_NONE) {
		debug_log("contacts_query_destroy is failed. error_code = %d", ct_ret);
	}
	return ct_ret;
}

EMAIL_API int email_get_last_contact_in_contact_list(contacts_list_h list, contacts_record_h *last_contact)
{
	debug_enter();
	int ct_ret = CONTACTS_ERROR_NONE;
	contacts_record_h ct_value = NULL;

	ct_ret = contacts_list_last(list);
	if (ct_ret == CONTACTS_ERROR_NONE)
		ct_ret = contacts_list_get_current_record_p(list, &ct_value);
	*last_contact = ct_value;

	return ct_ret;
}

EMAIL_API int email_get_contacts_index(contacts_record_h record, int *index)
{
	debug_enter();
	int ct_ret = CONTACTS_ERROR_NONE;
	ct_ret = contacts_record_get_int(record, _contacts_contact_email.contact_id, index);
	return ct_ret;
}

EMAIL_API int email_get_contacts_display_name(contacts_record_h record, char **display_name)
{
	debug_enter();
	int ct_ret = CONTACTS_ERROR_NONE;
	ct_ret = contacts_record_get_str_p(record, _contacts_contact_email.display_name, display_name);
	return ct_ret;
}

EMAIL_API int email_get_contacts_email_address(contacts_record_h record, char **email_addr)
{
	debug_enter();
	int ct_ret = CONTACTS_ERROR_NONE;
	ct_ret = contacts_record_get_str_p(record, _contacts_contact_email.email, email_addr);
	return ct_ret;
}

EMAIL_API int email_get_contacts_first_name(contacts_record_h record, char **first_name)
{
	debug_enter();
	int ct_ret = CONTACTS_ERROR_NONE;
	ct_ret = contacts_record_get_str_p(record, _contacts_name.first, first_name);
	return ct_ret;
}

EMAIL_API int email_get_contacts_last_name(contacts_record_h record, char **last_name)
{
	debug_enter();
	int ct_ret = CONTACTS_ERROR_NONE;
	ct_ret = contacts_record_get_str_p(record, _contacts_name.last, last_name);
	return ct_ret;
}

EMAIL_API int email_get_contacts_list_info(contacts_list_h list, EMAIL_CONTACT_LIST_INFO_S *ct_list_info)
{
	debug_enter();
	int index = 0;
	int ct_ret = CONTACTS_ERROR_NONE;

	while (CONTACTS_ERROR_NONE == ct_ret) {
		contacts_record_h ct_value = NULL;
		ct_ret = contacts_list_get_current_record_p(list, &ct_value);
		debug_log("ct_ret = %d, ct_value = %p (%d)", ct_ret, ct_value, (ct_value == NULL));
		if (ct_value) {
			ct_ret = contacts_record_get_int(ct_value, _contacts_contact_email.contact_id, &index);
			debug_log("contact_id(%d)", index);
			if (index > 0) {
				char *display_name = NULL;
				char *image_path = NULL;
				char *email_addr = NULL;
				int type = -1;
				int person_id = -1;

				ct_list_info->index = index;

				if ((ct_ret = contacts_record_get_int(ct_value, _contacts_contact_email.person_id, &person_id)) == CONTACTS_ERROR_NONE) {
					debug_log("person_id(%d)", person_id);
					if (person_id != -1) {
						ct_list_info->person_id = person_id;
					}
				} else {
					debug_log("contacts_record_get_int() failed! error_code = %d", ct_ret);
				}

				if ((ct_ret = contacts_record_get_str_p(ct_value, _contacts_contact_email.display_name, &display_name)) == CONTACTS_ERROR_NONE) {
					debug_secure("display_name(%s)", display_name);
					if (display_name != NULL) {
						ct_list_info->display = g_strdup(display_name);
					}
				} else {
					debug_log("email_get_contacts_display_name is failed. error_code = %d", ct_ret);
				}

				if ((ct_ret = contacts_record_get_str_p(ct_value, _contacts_contact_email.image_thumbnail_path, &image_path)) == CONTACTS_ERROR_NONE) {
					debug_secure("image_path(%s)", image_path);
					if (image_path != NULL) {
						ct_list_info->image_path = g_strdup(image_path);
					}
				} else {
					debug_log("email_get_contacts_image_thumbnail_path is failed. error_code = %d", ct_ret);
				}

				if ((ct_ret = contacts_record_get_str_p(ct_value, _contacts_contact_email.email, &email_addr)) == CONTACTS_ERROR_NONE) {
					debug_secure("email_addr(%s)", email_addr);
					if (email_addr != NULL) {
						ct_list_info->email_address = g_strdup(email_addr);
					}
				} else {
					debug_log("email_get_contacts_email_address is failed. error_code = %d", ct_ret);
				}

				if ((ct_ret = contacts_record_get_int(ct_value, _contacts_contact_email.type, &type)) == CONTACTS_ERROR_NONE) {
					debug_log("contact type = %d", type);
					if (type != -1) {
						debug_log("contacts email type: %d", type);
						ct_list_info->email_type = type;
					}
				} else {
					debug_log("contacts_record_get_int() failed! error_code = %d", ct_ret);
				}

				if (!ct_list_info->display && email_addr) {
					strncpy(ct_list_info->display_name, email_addr, sizeof(ct_list_info->display_name) - 1);
				} else {
					if (ct_list_info->display && strlen(ct_list_info->display) > 0) {
						snprintf(ct_list_info->display_name, sizeof(ct_list_info->display_name), "%s", ct_list_info->display);
					}
				}

				debug_secure("index(%d), display(%s), image_path(%s), email(%s), display_name(%s)",
					index, ct_list_info->display, ct_list_info->image_path, ct_list_info->email_address, ct_list_info->display_name);

				return ct_ret;
			}
		}
		ct_ret = contacts_list_next(list);
	}
	return ct_ret;
}

EMAIL_API int email_get_phone_log(contacts_match_str_flag_e match, contacts_list_h *list, const char *search_word)
{
	debug_enter();
	int ct_ret = CONTACTS_ERROR_NONE;

	contacts_query_h query = NULL;
	contacts_filter_h filter = NULL;
	contacts_filter_h type_filter = NULL;
	debug_secure("search_word = %s", search_word);
	unsigned int projection[] = {
		_contacts_person_phone_log.address,
	};

	/* received/sent email type log */
	ct_ret = contacts_filter_create(_contacts_person_phone_log._uri, &type_filter);
	ct_ret = contacts_filter_add_int(type_filter, _contacts_person_phone_log.log_type, CONTACTS_MATCH_EQUAL, CONTACTS_PLOG_TYPE_EMAIL_RECEIVED);
	ct_ret = contacts_filter_add_operator(type_filter, CONTACTS_FILTER_OPERATOR_OR);
	ct_ret = contacts_filter_add_int(type_filter, _contacts_person_phone_log.log_type, CONTACTS_MATCH_EQUAL, CONTACTS_PLOG_TYPE_EMAIL_SENT);

	/* (log which email address is "email_address" in) AND (log which is not saved in contact) AND (email type log) */
	ct_ret = contacts_filter_create(_contacts_person_phone_log._uri, &filter);
	ct_ret = contacts_filter_add_str(filter, _contacts_person_phone_log.address, match, search_word);
	ct_ret = contacts_filter_add_operator(filter, CONTACTS_FILTER_OPERATOR_AND);
	/* log which is not saved as email of contact, duplicated log which was found in contact name and contact email address is removed */
	ct_ret = contacts_filter_add_int(filter, _contacts_person_phone_log.person_id, CONTACTS_MATCH_NONE, 0);
	ct_ret = contacts_filter_add_operator(filter, CONTACTS_FILTER_OPERATOR_AND);
	ct_ret = contacts_filter_add_filter(filter, type_filter);

	ct_ret = contacts_query_create(_contacts_person_phone_log._uri, &query);
	ct_ret = contacts_query_set_filter(query, filter);
	ct_ret = contacts_query_set_projection(query, projection, sizeof(projection) / sizeof(unsigned int));
	ct_ret = contacts_query_set_distinct(query, true);
	if (ct_ret != CONTACTS_ERROR_NONE)
		debug_log("contacts_query_set_distinct is failed (error_code = %d)", ct_ret);
	ct_ret = contacts_db_get_records_with_query(query, 0, 0, list);
	if (ct_ret != CONTACTS_ERROR_NONE)
		debug_log("contacts_db_get_records_with_query is failed (error_code = %d)", ct_ret);

	ct_ret = contacts_filter_destroy(filter);
	ct_ret = contacts_filter_destroy(type_filter);
	ct_ret = contacts_query_destroy(query);

	return ct_ret;
}

EMAIL_API int email_get_phone_log_info(contacts_list_h list, EMAIL_CONTACT_LIST_INFO_S *ct_list_info)
{
	debug_enter();
	int ct_ret = CONTACTS_ERROR_NONE;

	contacts_record_h ct_value = NULL;
	ct_ret = contacts_list_get_current_record_p(list, &ct_value);
	debug_log("ct_ret = %d, ct_value = %p (%d)", ct_ret, ct_value, (ct_value == NULL));
	char *email_address = NULL;
	if (ct_value) {
		if ((ct_ret = contacts_record_get_str_p(ct_value, _contacts_person_phone_log.address, &email_address)) == CONTACTS_ERROR_NONE) {
			if (email_address != NULL) {
				ct_list_info->email_address = g_strdup(email_address);
			}
		} else {
			debug_log("email_get_contacts_email_address is failed. error_code = %d", ct_ret);
		}
	}

	return ct_ret;
}

EMAIL_API int email_num_id_get_contacts_record(int num_id, contacts_record_h *out_record)
{
	debug_enter();
	int ct_ret = CONTACTS_ERROR_NONE;

	/* get record email */
	ct_ret = contacts_db_get_record(_contacts_contact_email._uri, num_id, out_record);
	if (ct_ret != CONTACTS_ERROR_NONE) {
		debug_log("_contacts_number, db_get_record is failed : ct_ret = [%d]", ct_ret);
		return ct_ret;
	}
	return ct_ret;
}

EMAIL_API void *email_contact_search_by_email(void *ug_data, const char *search_word)
{
	debug_enter();
	retvm_if(ug_data == NULL, NULL, "Invalid parameter: ug_data[NULL]");
	int ct_ret = CONTACTS_ERROR_NONE;
	EMAIL_CONTACT_LIST_INFO_S *contacts_list_item = NULL;
	contacts_list_h list = NULL;

	ct_ret = email_get_contacts_list(CONTACTS_MATCH_FULLSTRING, &list, search_word, EMAIL_SEARCH_CONTACT_BY_EMAIL);
	debug_log("email_get_contacts_record:%d, list : %p", ct_ret, list);

	if (!list) {
		debug_log("list is NULL");
		contacts_list_destroy(list, true);
		return NULL;
	}

	contacts_list_item = MEM_ALLOC(contacts_list_item, 1);
	contacts_list_item->ugd = ug_data;

	ct_ret = email_get_contacts_list_info(list, contacts_list_item);
	if (ct_ret != CONTACTS_ERROR_NONE) {
		debug_log("email_get_contacts_list_info: ct_ret : %d", ct_ret);
		email_delete_contacts_list(&contacts_list_item);
		contacts_list_destroy(list, true);
		return NULL;
	}
	debug_log("email_get_contacts_list_info:%d, contacts_list_item : %p", ct_ret, contacts_list_item);

	contacts_list_destroy(list, true);
	debug_leave();
	return contacts_list_item;
}

EMAIL_API void email_delete_contacts_list(EMAIL_CONTACT_LIST_INFO_S **contacts_list_item)
{
	debug_enter();
	retm_if(!contacts_list_item || !*contacts_list_item, "Invalid parameter: contacts_list_item[NULL]");

	FREE((*contacts_list_item)->display);
	FREE((*contacts_list_item)->email_address);
	FREE((*contacts_list_item)->image_path);
	FREE(*contacts_list_item);
	debug_leave();
}

/* EOF */
