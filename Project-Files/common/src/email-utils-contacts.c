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

#define VCARD_FILE_ACCESS_MODE 0644
#define VCARD_NAME_SUFFIX_LEN 16

static int _contacts_db_ref_count = 0;
/*
 * STATIC FUNCTIONS
 */

static int _email_contacts_get_contacts_list(contacts_match_str_flag_e match, contacts_list_h *list, const char *search_word, email_contact_search_type_e search_type);
static int _email_contacts_get_contacts_list_info(contacts_list_h list, email_contact_list_info_t *ct_list_info);
static int _email_contacts_get_contacts_list_by_email_id(int email_id, contacts_list_h *list);
static int _email_contacts_get_log_list(contacts_match_str_flag_e match, contacts_list_h *list, const char *search_word);
static int _email_contacts_get_log_list_info(contacts_list_h list, email_contact_list_info_t *ct_list_info);
static Eina_List * _email_contacts_logs_db_search(const char *search_word, Eina_Hash *hash);
static Eina_List * _email_contacts_contacts_db_search(const char *search_word, Eina_Hash *hash);
static bool _email_contacts_is_valid_vcard_file_name_char(char c);
static char *_email_contacts_make_vcard_file_path(const char* display_name, const char *vcard_dir_path);
static bool _email_contacts_write_vcard_to_file(int fd, contacts_record_h record, Eina_Bool my_profile);
static bool _email_contacts_write_vcard_to_file_from_id(int fd, int person_id);

static int _email_contacts_get_contacts_list(contacts_match_str_flag_e match, contacts_list_h *list, const char *search_word, email_contact_search_type_e search_type)
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

static int _email_contacts_get_contacts_list_info(contacts_list_h list, email_contact_list_info_t *ct_list_info)
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
				int email_id = -1;

				ct_list_info->index = index;

				if ((ct_ret = contacts_record_get_int(ct_value, _contacts_contact_email.person_id, &person_id)) == CONTACTS_ERROR_NONE) {
					debug_log("person_id(%d)", person_id);
					if (person_id != -1) {
						ct_list_info->person_id = person_id;
					}
				} else {
					debug_log("contacts_record_get_int() failed! error_code = %d", ct_ret);
				}

				if ((ct_ret = contacts_record_get_int(ct_value, _contacts_contact_email.email_id, &email_id)) == CONTACTS_ERROR_NONE) {
					debug_log("email_id(%d)", email_id);
					if (email_id != -1) {
						ct_list_info->email_id = email_id;
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

static int _email_contacts_get_log_list(contacts_match_str_flag_e match, contacts_list_h *list, const char *search_word)
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

static int _email_contacts_get_log_list_info(contacts_list_h list, email_contact_list_info_t *ct_list_info)
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

static Eina_List * _email_contacts_logs_db_search(const char *search_word, Eina_Hash *hash)
{
	debug_enter();

	retvm_if(!search_word, NULL, "Invalid parameter: search_word is NULL!");
	retvm_if(!hash, NULL, "Invalid parameter: hash is NULL!");

	int ret = CONTACTS_ERROR_NONE;
	Eina_List *contacts_list = NULL;
	contacts_list_h list = NULL;
	char buf[EMAIL_LIMIT_EMAIL_ADDRESS_LENGTH + 1] = { 0, };

	ret = _email_contacts_get_log_list(CONTACTS_MATCH_CONTAINS, &list, search_word);
	if (ret != CONTACTS_ERROR_NONE) {
		if (list) {
			contacts_list_destroy(list, EINA_TRUE);
		}
		return NULL;
	} else if (!list) {
		return NULL;
	}

	int list_count = 0;
	ret = contacts_list_get_count(list, &list_count);
	if(ret != CONTACTS_ERROR_NONE) {
		debug_error("contacts_list_get_count() failed! ret:(%d)", ret);
		contacts_list_destroy(list, EINA_TRUE);
		return NULL;
	} else if (list_count == 0) {
		debug_error("list count is 0!");
		contacts_list_destroy(list, EINA_TRUE);
		return NULL;
	}

	while (ret == CONTACTS_ERROR_NONE) {
		email_contact_list_info_t *contact_info = (email_contact_list_info_t *)calloc(1, sizeof(email_contact_list_info_t));
		if (!contact_info) {
			debug_error("Failed to allocate memory for contact_info!");
			ret = contacts_list_next(list);
			continue;
		}

		ret = _email_contacts_get_log_list_info(list, contact_info);
		if (ret != CONTACTS_ERROR_NONE) {
			debug_error("_email_contacts_get_log_list_info() failed! ret:[%d]", ret);
			email_contacts_delete_contact_info(contact_info);
			ret = contacts_list_next(list);
			continue;
		}

		if (contact_info->email_address) {
			contact_info->contact_origin = EMAIL_SEARCH_CONTACT_ORIGIN_RECENT;

			char *tmp = contact_info->email_address;
			contact_info->email_address = g_ascii_strdown(tmp, -1); /* Need to use with case insensitive. */
			g_free(tmp);

			/* Note: for email address in recent, it should be omitted if there's the same email_address in the list */
			memset(buf, 0x0, sizeof(buf));
			strncpy(buf, contact_info->email_address, EMAIL_LIMIT_EMAIL_ADDRESS_LENGTH);

			if (eina_hash_find(hash, buf) != NULL) {
				debug_log("There's a duplicate entry! Omitting!");
				email_contacts_delete_contact_info(contact_info);
			} else {
				eina_hash_add(hash, buf, contact_info);
				contacts_list = eina_list_append(contacts_list, contact_info);
			}
		} else {
			debug_log("There's no email addresses on the contact! Freeing!");
			email_contacts_delete_contact_info(contact_info);
		}

		ret = contacts_list_next(list);
	}

	ret = contacts_list_destroy(list, EINA_TRUE);
	debug_warning_if(ret != CONTACTS_ERROR_NONE, "contacts_list_destroy() failed! ret:(%d)", ret);

	debug_leave();
	return contacts_list;
}

static Eina_List * _email_contacts_contacts_db_search(const char *search_word, Eina_Hash *hash)
{
	debug_enter();

	retvm_if(!search_word, NULL, "Invalid parameter: search_word is NULL!");
	retvm_if(!hash, NULL, "Invalid parameter: hash is NULL!");

	int ret = CONTACTS_ERROR_NONE;
	Eina_List *contacts_list = NULL;
	char buf[2 * EMAIL_LIMIT_EMAIL_ADDRESS_LENGTH + 1] = { 0, };
	int index = 0;

	for (index = EMAIL_SEARCH_CONTACT_MIN; index < EMAIL_SEARCH_CONTACT_MAX; index++) {
		contacts_list_h list = NULL;

		ret = _email_contacts_get_contacts_list(CONTACTS_MATCH_CONTAINS, &list, search_word, index);
		if (!list) {
			debug_warning("list is NULL! type:[%d]", index);
			continue;
		}

		int list_count = 0;
		ret = contacts_list_get_count(list, &list_count);
		if ((list_count == 0) || (ret != CONTACTS_ERROR_NONE)) {
			contacts_list_destroy(list, EINA_TRUE);
			continue;
		}

		while (ret == CONTACTS_ERROR_NONE) {
			email_contact_list_info_t *contact_info = (email_contact_list_info_t *)calloc(1, sizeof(email_contact_list_info_t));
			if (!contact_info) {
				debug_error("Failed to allocate memory for contact_info!");
				ret = contacts_list_next(list);
				continue;
			}

			ret = _email_contacts_get_contacts_list_info(list, contact_info);
			if (ret != CONTACTS_ERROR_NONE) {
				debug_error("_email_contacts_get_contacts_list_info() failed! ret:[%d]", ret);
				email_contacts_delete_contact_info(contact_info);
				ret = contacts_list_next(list);
				continue;
			}

			if (contact_info->email_address) {
				contact_info->contact_origin = EMAIL_SEARCH_CONTACT_ORIGIN_CONTACTS;

				/* Note: for email address in contacts app, it should be omitted if display_name and email_address is the same. */
				memset(buf, 0x0, sizeof(buf));
				strncpy(buf, contact_info->email_address, EMAIL_LIMIT_EMAIL_ADDRESS_LENGTH);
				strncat(buf, contact_info->display_name, EMAIL_LIMIT_EMAIL_ADDRESS_LENGTH);

				if (eina_hash_find(hash, buf) != NULL) {
					debug_log("There's a duplicate entry! Omitting!");
					email_contacts_delete_contact_info(contact_info);
				} else {
					eina_hash_add(hash, buf, contact_info);
					contacts_list = eina_list_append(contacts_list, contact_info);

					memset(buf, 0x0, sizeof(buf));
					snprintf(buf, sizeof(buf), "%s", contact_info->email_address);
					if (eina_hash_find(hash, buf) == NULL) {
						eina_hash_add(hash, buf, contact_info);
					}
				}
			} else {
				debug_log("There's no email addresses on the contact! Freeing!");
				email_contacts_delete_contact_info(contact_info);
			}

			ret = contacts_list_next(list);
		}

		ret = contacts_list_destroy(list, EINA_TRUE);
		debug_warning_if(ret != CONTACTS_ERROR_NONE, "contacts_list_destroy() failed! ret:(%d)", ret);
	}

	debug_leave();
	return contacts_list;
}

static int _email_contacts_get_contacts_list_by_email_id(int email_id, contacts_list_h *list)
{
	debug_enter();

	int ret = CONTACTS_ERROR_NONE;
	contacts_query_h query = NULL;
	contacts_filter_h filter = NULL;

	ret = contacts_query_create(_contacts_contact_email._uri, &query);
	retvm_if(ret != CONTACTS_ERROR_NONE, ret, "contacts_query_create() failed! ret:[%d]");

	ret = contacts_filter_create(_contacts_contact_email._uri, &filter);
	gotom_if(ret != CONTACTS_ERROR_NONE, FUNC_EXIT, "contacts_filter_create() failed! ret:[%d]");

	ret = contacts_filter_add_int(filter, _contacts_contact_email.email_id, CONTACTS_MATCH_EQUAL, email_id);
	gotom_if(ret != CONTACTS_ERROR_NONE, FUNC_EXIT, "contacts_filter_add_int() failed! ret:[%d]");

	ret = contacts_query_set_filter(query, filter);
	gotom_if(ret != CONTACTS_ERROR_NONE, FUNC_EXIT, "contacts_query_set_filter() failed! ret:[%d]");

	ret = contacts_db_get_records_with_query(query, 0, 0, list);
	gotom_if(ret != CONTACTS_ERROR_NONE, FUNC_EXIT, "contacts_db_get_records_with_query() failed! ret:[%d]");

FUNC_EXIT:
	if (filter) {
		ret = contacts_filter_destroy(filter);
		debug_warning_if(ret != CONTACTS_ERROR_NONE, "contacts_filter_destroy() failed! ret:[%d]");
	}

	if (query) {
		ret = contacts_query_destroy(query);
		debug_warning_if(ret != CONTACTS_ERROR_NONE, "contacts_query_destroy() failed! ret:[%d]");
	}

	debug_leave();
	return ret;
}

static bool _email_contacts_is_valid_vcard_file_name_char(char c)
{
	switch (c) {
	case '\\':
	case '/':
	case ':':
	case '*':
	case '?':
	case '\"':
	case '<':
	case '>':
	case '|':
	case ';':
	case ' ':
		return false;
	default:
		return true;
	}
}

static char *_email_contacts_make_vcard_file_path(const char* display_name, const char *vcard_dir_path)
{
	debug_log();
	retvm_if(!display_name, NULL, "display_name is NULL");

	char *display_name_copy = NULL;
	char *file_name = NULL;
	char file_path[PATH_MAX] = { 0 };

	display_name_copy = strdup(display_name);
	retvm_if(!display_name_copy, NULL, "display_name_copy is NULL");

	file_name = email_util_strtrim(display_name_copy);
	if (!STR_VALID(file_name)) {
		snprintf(file_path, sizeof(file_path), "%s/Unknown.vcf", vcard_dir_path);

	} else {
		int i = 0;
		int length = 0;

		length = strlen(file_name);
		if (length > NAME_MAX - VCARD_NAME_SUFFIX_LEN) {
			debug_warning("Display name is too long. It will be cut.");
			length = NAME_MAX - VCARD_NAME_SUFFIX_LEN;
			file_name[length] = '\0';
		}

		for (i = 0; i < length; ++i) {
			if (!_email_contacts_is_valid_vcard_file_name_char(file_name[i])) {
				file_name[i] = '_';
			}
		}

		snprintf(file_path, sizeof(file_path), "%s/%s.vcf", vcard_dir_path, file_name);
	}

	debug_log("file_path = %s", file_path);

	free(display_name_copy);

	return strdup(file_path);
}

static bool _email_contacts_write_vcard_to_file(int fd, contacts_record_h record, Eina_Bool my_profile)
{
	debug_log();

	char *vcard_buff = NULL;
	bool ok = false;

	do {
		int size_left = 0;

		if (my_profile) {
			contacts_vcard_make_from_my_profile(record, &vcard_buff);
		} else {
			contacts_vcard_make_from_person(record, &vcard_buff);
		}

		if (!vcard_buff) {
			debug_error("vcard_buff is NULL");
			break;
		}

		size_left = strlen(vcard_buff);
		while (size_left) {
			int written = write(fd, vcard_buff, size_left);
			if (written == -1) {
				debug_error("write() failed: %s", strerror(errno));
				break;
			}
			size_left -= written;
		}

		ok = (size_left == 0);
	} while (false);

	free(vcard_buff);

	return ok;
}

static bool _email_contacts_write_vcard_to_file_from_id(int fd, int person_id)
{
	debug_enter();

	contacts_record_h record = NULL;
	bool ok = false;

	do {
		int ret = contacts_db_get_record(_contacts_person._uri, person_id, &record);
		if (ret != CONTACTS_ERROR_NONE) {
			debug_error("contacts_db_get_record() failed: %d", ret);
			record = NULL;
			break;
		}

		if (!_email_contacts_write_vcard_to_file(fd, record, false)) {
			debug_error("_email_contacts_write_vcard_to_file() failed");
			break;
		}

		ok = true;
	} while (false);

	if (record) {
		contacts_record_destroy(record, true);
	}

	return ok;
}

/* PUBLIC FUNCTIONS */
EMAIL_API int email_contacts_init_service()
{
	debug_enter();
	++_contacts_db_ref_count;

	if (_contacts_db_ref_count > 1) {
		debug_log("already opened - EDB ref_count(%d)", _contacts_db_ref_count);
		return CONTACTS_ERROR_NONE;
	}

	debug_leave();
	return contacts_connect();
}

EMAIL_API int email_contacts_finalize_service()
{
	debug_enter();

	--_contacts_db_ref_count;
	if (_contacts_db_ref_count > 0) {
		debug_log("remain EDB ref_count(%d)", _contacts_db_ref_count);
		return CONTACTS_ERROR_NONE;
	}

	debug_leave();
	return contacts_disconnect();
}

EMAIL_API email_contact_list_info_t *email_contacts_get_contact_info_by_email_id(int email_id)
{
	debug_enter();

	int ret = CONTACTS_ERROR_NONE;
	contacts_list_h person_email_list = NULL;

	ret = _email_contacts_get_contacts_list_by_email_id(email_id, &person_email_list);
	retvm_if(ret != CONTACTS_ERROR_NONE, NULL, "Failed to get contact list");

	email_contact_list_info_t *contact_info = MEM_ALLOC(contact_info, 1);

	ret = _email_contacts_get_contacts_list_info(person_email_list, contact_info);
	if (ret != CONTACTS_ERROR_NONE) {
		debug_warning("contacts_list_get_current_record_p() failed (%d)", ret);
		contacts_list_destroy(person_email_list, true);
		return NULL;
	}
	contact_info->email_id = email_id;
	contacts_list_destroy(person_email_list, true);
	debug_leave();
	return contact_info;
}

EMAIL_API email_contact_list_info_t *email_contacts_get_contact_info_by_email_address(const char *email_address)
{
	debug_enter();

	retvm_if(email_address == NULL, NULL, "Invalid parameter: ug_data[NULL]");
	int ct_ret = CONTACTS_ERROR_NONE;
	email_contact_list_info_t *contacts_list_item = NULL;
	contacts_list_h list = NULL;

	ct_ret = _email_contacts_get_contacts_list(CONTACTS_MATCH_FULLSTRING, &list, email_address, EMAIL_SEARCH_CONTACT_BY_EMAIL);
	debug_log("email_get_contacts_record:%d, list : %p", ct_ret, list);

	if (!list) {
		debug_log("list is NULL");
		contacts_list_destroy(list, true);
		return NULL;
	}

	contacts_list_item = MEM_ALLOC(contacts_list_item, 1);

	ct_ret = _email_contacts_get_contacts_list_info(list, contacts_list_item);
	if (ct_ret != CONTACTS_ERROR_NONE) {
		debug_log("_email_contacts_get_contacts_list_info: ct_ret : %d", ct_ret);
		email_contacts_delete_contact_info(contacts_list_item);
		contacts_list_destroy(list, true);
		return NULL;
	}
	debug_log("_email_contacts_get_contacts_list_info:%d, contacts_list_item : %p", ct_ret, contacts_list_item);

	contacts_list_destroy(list, true);
	debug_leave();
	return contacts_list_item;
}

EMAIL_API void email_contacts_delete_contact_info(email_contact_list_info_t *contact_info)
{
	debug_enter();
	retm_if(!contact_info, "Invalid parameter: contact_info[NULL]");

	FREE(contact_info->display);
	FREE(contact_info->email_address);
	FREE(contact_info->image_path);
	FREE(contact_info);
	debug_leave();
}

EMAIL_API Eina_List *email_contacts_search_contacts_by_keyword(const char *search_word)
{
	debug_enter();

	Eina_Hash *hash = eina_hash_string_superfast_new(NULL); /* Hash for omitting duplicate entries. */
	retvm_if(!hash, NULL, "ugd->ps_hash is NULL!");

	Eina_List *contacts_list = _email_contacts_contacts_db_search(search_word, hash); /* Retrieve contacts list from contacts db */
	Eina_List *recents_list = _email_contacts_logs_db_search(search_word, hash); /* Retrieve contacts list from phonelog db */

	Eina_List *search_result = eina_list_merge(contacts_list, recents_list); /* contacts_list is cached for future use. */
	eina_hash_free(hash);
	debug_leave();
	return search_result;
}

EMAIL_API int email_contacts_get_email_address_by_contact_id(int contact_id, char **email_address)
{
	debug_enter();

	int ret = CONTACTS_ERROR_NONE;
	contacts_record_h record = NULL;

	ret = contacts_db_get_record(_contacts_email._uri, contact_id, &record);
	if (ret != CONTACTS_ERROR_NONE) {
		debug_error("contacts_db_get_record failed: %d", ret);
		return ret;
	}

	ret = contacts_record_get_str(record, _contacts_email.email, email_address);
	if (ret != CONTACTS_ERROR_NONE) {
		debug_error("contacts_record_get_str failed: %d", ret);
	}

	contacts_record_destroy(record, true);
	debug_leave();
	return ret;
}

EMAIL_API int email_contacts_get_contact_name_by_email_address(char *email_address, char **contact_name)
{
	retvm_if(!email_address, CONTACTS_ERROR_INVALID_PARAMETER, "Email address is NULL");

	int ret = CONTACTS_ERROR_NONE;

	contacts_list_h ct_list = NULL;
	ret = _email_contacts_get_contacts_list(CONTACTS_MATCH_FULLSTRING, &ct_list, email_address, EMAIL_SEARCH_CONTACT_BY_EMAIL);
	if (ret != CONTACTS_ERROR_NONE ) {
		debug_error("Failed to get contacts list :%d", ret);
		contacts_list_destroy(ct_list, 1);
		return ret;
	}

	email_contact_list_info_t *contact_info = (email_contact_list_info_t *)calloc(1, sizeof(email_contact_list_info_t));
	ret = _email_contacts_get_contacts_list_info(ct_list, contact_info);
	if (ret != CONTACTS_ERROR_NONE ) {
			debug_error("Failed to get contact record from contacts list :%d", ret);
			contacts_list_destroy(ct_list, 1);
			return ret;
	}

	if (contact_info->display_name) {
		*contact_name = strdup(contact_info->display_name);
	}

	email_contacts_delete_contact_info(contact_info);

	debug_leave();
	return ret;
}

EMAIL_API char *email_contacts_create_vcard_from_id_list(const int *id_list, int count, const char *vcard_dir_path, volatile Eina_Bool *cancel)
{
	debug_enter();
	retvm_if(!id_list, NULL, "id_list is NULL");
	retvm_if(count <= 0, NULL, "count <= 0");

	char file_path[PATH_MAX] = { 0 };
	char *vcard_path = NULL;
	int fd = -1;
	bool ok = false;

	if (cancel && *cancel) {
		return NULL;
	}

	do {
		int i = 0;

		snprintf(file_path, sizeof(file_path), "%s/Contacts.vcf", vcard_dir_path);

		vcard_path = strdup(file_path);
		if (!vcard_path) {
			debug_error("strdup() failed");
			break;
		}

		fd = open(vcard_path, O_WRONLY | O_CREAT | O_TRUNC, VCARD_FILE_ACCESS_MODE);
		if (fd == -1) {
			debug_error("open(%s) Failed(%s)", vcard_path, strerror(errno));
			break;
		}

		for (i = 0; (!cancel || !(*cancel)) && (i < count); ++i) {
			if (!_email_contacts_write_vcard_to_file_from_id(fd, id_list[i])) {
				debug_error("_email_contacts_write_vcard_to_file() failed");
				break;
			}
		}

		ok = (i == count);
	} while (false);

	if (fd != -1) {
		close(fd);
		if (!ok) {
			email_file_remove(vcard_path);
		}
	}

	if (!ok) {
		FREE(vcard_path);
	}

	debug_leave();
	return vcard_path;
}

EMAIL_API char* email_contacts_create_vcard_from_id(int id, const char *vcard_dir_path, Eina_Bool my_profile)
{
	debug_enter();

	contacts_record_h record = NULL;
	char *vcard_path = NULL;
	int fd = -1;
	bool ok = false;

	do {
		char *display_name = NULL;

		int ret = contacts_db_get_record((my_profile ?
				_contacts_my_profile._uri :
				_contacts_person._uri), id, &record);
		if (ret != CONTACTS_ERROR_NONE) {
			debug_error("contacts_db_get_record() failed: %d", ret);
			record = NULL;
			break;
		}

		if (my_profile) {
			contacts_record_get_str_p(record, _contacts_my_profile.display_name, &display_name);
		} else {
			contacts_record_get_str_p(record, _contacts_person.display_name, &display_name);
		}

		vcard_path = _email_contacts_make_vcard_file_path(display_name, vcard_dir_path);
		if (!vcard_path) {
			debug_error("_email_contacts_make_vcard_file_path() failed");
			break;
		}

		fd = open(vcard_path, O_WRONLY | O_CREAT | O_TRUNC, VCARD_FILE_ACCESS_MODE);
		if (fd == -1) {
			debug_error("open(%s) Failed(%s)", vcard_path, strerror(errno));
			break;
		}

		if (!_email_contacts_write_vcard_to_file(fd, record, my_profile)) {
			debug_error("_email_contacts_write_vcard_to_file() failed");
			break;
		}

		ok = true;
	} while (false);

	if (record) {
		contacts_record_destroy(record, true);
	}

	if (fd != -1) {
		close(fd);
		if (!ok) {
			email_file_remove(vcard_path);
		}
	}

	if (!ok) {
		FREE(vcard_path);
	}

	debug_leave();
	return vcard_path;
}
/* EOF */
