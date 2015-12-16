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

#include <Elementary.h>
#include <contacts.h>

#include "email-debug.h"
#include "email-utils.h"
#include "email-utils-contacts.h"

#include "email-composer.h"
#include "email-composer-types.h"
#include "email-composer-util.h"

#define ENTRY_SIZE_ADJUSTMENT	ELM_SCALE_SIZE(120) /* Size of left and right paddings for entry layout and paddings for textblock inside entry */


/*
 * Declarations for static variables
 */

/*
 * Declarations for static functions
 */

static char *_util_recp_escaping_email_display_name(char *display_name, int len);

/*
 * Definitions for static & exported functions
 */
gchar *composer_util_get_ellipsised_entry_name(EmailComposerUGD *ugd, Evas_Object *display_entry, const char *entry_string)
{
	debug_enter();

	gchar *ellipsised_string = NULL;
	Evas_Object *text_block = NULL;
	Evas_Coord width = 0, height = 0;
	Evas_Coord label_width = 0;
	int allowed_entry_width = 0;
	int win_w = 0;

	if ((ugd->base.orientation == APP_DEVICE_ORIENTATION_0) ||
		(ugd->base.orientation == APP_DEVICE_ORIENTATION_180)) {
		elm_win_screen_size_get(ugd->base.module->win, NULL, NULL, &win_w, NULL);
	} else {
		elm_win_screen_size_get(ugd->base.module->win, NULL, NULL, NULL, &win_w);
	}

	if (display_entry == ugd->recp_to_display_entry.entry && ugd->recp_to_label) {
		evas_object_geometry_get(ugd->recp_to_label, NULL, NULL, &label_width, NULL);
	} else if (display_entry == ugd->recp_cc_display_entry.entry && ugd->recp_cc_label_cc) {
		evas_object_geometry_get(ugd->recp_cc_label_cc, NULL, NULL, &label_width, NULL);
	} else if (display_entry == ugd->recp_bcc_display_entry.entry && ugd->recp_bcc_label) {
		evas_object_geometry_get(ugd->recp_bcc_label, NULL, NULL, &label_width, NULL);
	}

	allowed_entry_width = win_w - label_width - ENTRY_SIZE_ADJUSTMENT;

	Evas_Object *entry = elm_entry_add(ugd->composer_layout);
	composer_util_set_entry_text_style(entry);
	elm_entry_entry_set(entry, entry_string);
	text_block = elm_entry_textblock_get(entry);
	evas_object_textblock_size_native_get(text_block, &width, &height);

	debug_secure("width %d, allowed_entry_width %d, entry_string %s", width, allowed_entry_width, entry_string);
	if (width > allowed_entry_width) {
		gchar buff[BUF_LEN_L] = { 0 };
		gchar tmp[BUF_LEN_L] = { 0 };
		gchar *first_half = NULL;
		gchar *second_half = NULL;
		int dots_width = 0;
		int first_width = 0;
		int second_width = 0;
		int i = 0;
		/*
		 * To get the length of characters, not bytes.
		 */
		int j = g_utf8_strlen(entry_string, -1);

		snprintf(buff, BUF_LEN_L, "...");
		elm_entry_entry_set(entry, buff);
		evas_object_textblock_size_native_get(text_block, &dots_width, &height);

		while (i < j) {
			int n = 0;

			if (first_width <= second_width) {
				i++;
				g_free(first_half);
				first_half = g_utf8_substring(entry_string, 0, i);
				elm_entry_entry_set(entry, first_half);
				evas_object_textblock_size_native_get(text_block, &first_width, &height);
			} else {
				j--;
				second_half = g_utf8_offset_to_pointer(entry_string, j);
				elm_entry_entry_set(entry, second_half);
				evas_object_textblock_size_native_get(text_block, &second_width, &height);
			}

			n = snprintf(tmp, BUF_LEN_L, "%s...%s", (first_half ? first_half : ""), (second_half ? second_half : ""));
			if (n >= BUF_LEN_L) {
				break;
			}

			if (first_width + dots_width + second_width > allowed_entry_width) {
				int total_width = 0;
				elm_entry_entry_set(entry, tmp);
				evas_object_textblock_size_native_get(text_block, &total_width, &height);
				if (total_width > allowed_entry_width) {
					break;
				}
			}

			memcpy(buff, tmp, n + 1);
		}

		g_free(first_half);

		ellipsised_string = g_strdup(buff);
	} else {
		ellipsised_string = g_strdup(entry_string);
	}

	evas_object_del(entry);

	debug_leave();
	return ellipsised_string;
}

Eina_Bool composer_util_recp_is_duplicated(EmailComposerUGD *ugd, Evas_Object *obj, const char *email_address)
{
	debug_enter();

	Eina_Bool is_duplicated = EINA_FALSE;

	Elm_Object_Item *mbe_item = elm_multibuttonentry_first_item_get(obj);
	while (mbe_item) {
		EmailRecpInfo *ri = elm_object_item_data_get(mbe_item);
		if (ri) {
			EmailAddressInfo *ai = (EmailAddressInfo *)eina_list_nth(ri->email_list, ri->selected_email_idx);
			if (ai && !g_strcmp0(ai->address, email_address)) {
				is_duplicated = EINA_TRUE;
				break;
			}
		} else {
			char *email = g_strstrip((char *)elm_object_item_text_get(mbe_item));
			if (email && !g_strcmp0(email, email_address)) {
				is_duplicated = EINA_TRUE;
				break;
			}
		}
		mbe_item = elm_multibuttonentry_item_next_get(mbe_item);
	}

	debug_leave();
	return is_duplicated;
}

Eina_Bool composer_util_recp_is_mbe_empty(void *data)
{
	debug_enter();

	retvm_if(!data, EINA_TRUE, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	Eina_Bool ret = EINA_TRUE;

	if ((elm_multibuttonentry_items_get(ugd->recp_to_mbe) != NULL) || (elm_multibuttonentry_items_get(ugd->recp_cc_mbe)) || (elm_multibuttonentry_items_get(ugd->recp_bcc_mbe))) {
		ret = EINA_FALSE;
	}

	debug_leave();
	return ret;
}

void composer_util_recp_clear_mbe(Evas_Object *obj)
{
	debug_enter();

	Elm_Object_Item *mbe_item;

	mbe_item = elm_multibuttonentry_first_item_get(obj);
	while (mbe_item) {
		Elm_Object_Item *del_item = mbe_item;
		mbe_item = elm_multibuttonentry_item_next_get(mbe_item);

		elm_object_item_del(del_item);
	}

	debug_leave();
}

void composer_util_recp_delete_contact_item(email_contact_list_info_t *contact_item)
{
	if (contact_item) {
		g_free(contact_item->display);
		g_free(contact_item->email_address);
		g_free(contact_item->image_path);
		g_free(contact_item);
	}
}

void composer_util_recp_delete_contacts_list(Eina_List *list)
{
	debug_enter();

	retm_if(!list, "Invalid parameter: list is NULL!");

	Eina_List *l = NULL;
	email_contact_list_info_t *item = NULL;

	EINA_LIST_FOREACH(list, l, item) {
		composer_util_recp_delete_contact_item(item);
	}
	DELETE_LIST_OBJECT(list);

	debug_leave();
}

void *composer_util_recp_search_contact_by_id(EmailComposerUGD *ugd, int search_id)
{
	debug_enter();

	int ret = CONTACTS_ERROR_NONE;
	email_contact_list_info_t *contacts_list_item = NULL;
	contacts_list_h list = NULL;

	ret = email_get_contacts_list_int(CONTACTS_MATCH_EQUAL, &list, search_id);
	retvm_if(!list, NULL, "list is NULL!");

	contacts_list_item = (email_contact_list_info_t *)calloc(1, sizeof(email_contact_list_info_t));
	if (!contacts_list_item) {
		debug_error("memory allocation failed");
		contacts_list_destroy(list, EINA_TRUE);
		return NULL;
	}
	contacts_list_item->ugd = (void *)ugd;

	ret = email_get_contacts_list_info(list, contacts_list_item);
	if (ret != CONTACTS_ERROR_NONE) {
		composer_util_recp_delete_contact_item(contacts_list_item);
		contacts_list_destroy(list, EINA_TRUE);
		return NULL;
	}

	debug_log("email_get_contacts_list_info() - ret:[%d], contacts_list_item:[%p]", ret, contacts_list_item);

	contacts_list_destroy(list, EINA_TRUE);

	debug_leave();
	return contacts_list_item;
}

void *composer_util_recp_search_contact_by_email(EmailComposerUGD *ugd, const char *search_word)
{
	debug_enter();

	int ret = CONTACTS_ERROR_NONE;
	email_contact_list_info_t *contacts_list_item = NULL;
	contacts_list_h list = NULL;

	ret = email_get_contacts_list(CONTACTS_MATCH_FULLSTRING, &list, search_word, EMAIL_SEARCH_CONTACT_BY_EMAIL);
	retvm_if(!list, NULL, "list is NULL!");

	contacts_list_item = (email_contact_list_info_t *)calloc(1, sizeof(email_contact_list_info_t));
	if (!contacts_list_item) {
		debug_error("memory allocation failed");
		contacts_list_destroy(list, EINA_TRUE);
		return NULL;
	}
	contacts_list_item->ugd = (void *)ugd;

	ret = email_get_contacts_list_info(list, contacts_list_item);
	if (ret != CONTACTS_ERROR_NONE) {
		composer_util_recp_delete_contact_item(contacts_list_item);
		contacts_list_destroy(list, EINA_TRUE);
		return NULL;
	}

	debug_log("email_get_contacts_list_info() - ret:[%d], contacts_list_item:[%p]", ret, contacts_list_item);

	contacts_list_destroy(list, EINA_TRUE);

	debug_leave();
	return contacts_list_item;
}

static char *_util_recp_escaping_email_display_name(char *display_name, int len)
{
	debug_enter();

	retvm_if(!display_name, NULL, "Invalid parameter: display_name is NULL!");
	retvm_if(len <= 0, NULL, "Invalid parameter: len: [%d]!", len);

	char *name = NULL;

	if (g_utf8_strchr(display_name, len, '\"')) {
		gchar **sp_str = g_strsplit_set(display_name, "\"", -1);
		int i;
		for (i = 0; i < (g_strv_length(sp_str) - 1); i++) {
			char *replace_str = "\\\"";
			if (!g_strcmp0(g_utf8_offset_to_pointer(sp_str[i], g_utf8_strlen(sp_str[i], -1) - 1), "\\")) {
				replace_str = "\"";
			}

			if (name) {
				char *tmp = name;
				name = g_strconcat(name, sp_str[i], replace_str, NULL);
				g_free(tmp);
			} else {
				if (sp_str[i] && (strlen(sp_str[i]) > 0)) {
					name = g_strconcat(sp_str[i], replace_str, NULL);
				} else {
					name = g_strdup(replace_str);
				}
			}
		}

		if (sp_str[i] && (strlen(sp_str[i]) > 0)) {
			char *tmp = name;
			name = g_strconcat(name, sp_str[i], NULL);
			g_free(tmp);
		}
	}

	debug_leave();
	return name;
}

EmailRecpInfo *composer_util_recp_make_recipient_info(void *data, const char *email_address)
{
	debug_enter();

	retvm_if(!data, NULL, "Invalid parameter: data is NULL!");
	retvm_if(!email_address, NULL, "Invalid parameter: email_address is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	char *nick = NULL;
	char *addr = NULL;

	EmailRecpInfo *ri = (EmailRecpInfo *)calloc(1, sizeof(EmailRecpInfo));
	retvm_if(!ri, NULL, "Failed to allocate memory for ri!");

	EmailAddressInfo *ai = (EmailAddressInfo *)calloc(1, sizeof(EmailAddressInfo));
	if (!ai) {
		debug_error("Memory allocation failed!");
		free(ri);
		return NULL;
	}

	email_get_recipient_info(email_address, &nick, &addr);
	if (!nick && !addr) {
		debug_error("Both nick and addr are NULL!");
		free(ri);
		free(ai);
		return NULL;
	}

	if (addr && !email_get_address_validation(addr)) {
		free(addr);
		addr = g_strdup(email_address);
	}
	if (nick) {
		ri->display_name = nick;
	}
	snprintf(ai->address, sizeof(ai->address), "%s", addr);
	ri->email_list = eina_list_append(ri->email_list, ai);

	email_contact_list_info_t *contact_list_item = (email_contact_list_info_t *)composer_util_recp_search_contact_by_email(ugd, ai->address);
	if (contact_list_item) {
		if (!ri->display_name) {
			ri->display_name = g_strdup(contact_list_item->display_name);
		}
		ri->person_id = contact_list_item->person_id;
		composer_util_recp_delete_contact_item(contact_list_item);
	} else {
		if (!ri->display_name) {
			ri->display_name = g_strdup(ai->address);
		}
		ri->person_id = 0;
	}

	/* nick is used on ri->display_name */
	free(addr);

	debug_leave();
	return ri;
}

EmailRecpInfo *composer_util_recp_make_recipient_info_with_from_address(char *email_address, char *display_name)
{
	debug_enter();

	retvm_if(!email_address, NULL, "Invalid parameter: email_address is NULL!");

	EmailRecpInfo *ri = (EmailRecpInfo *)calloc(1, sizeof(EmailRecpInfo));
	retvm_if(!ri, NULL, "Memory allocation failed!");

	EmailAddressInfo *ai = (EmailAddressInfo *)calloc(1, sizeof(EmailAddressInfo));
	if (!ai) {
		debug_error("Memory allocation failed!");
		free(ri);
		return NULL;
	}
	snprintf(ai->address, sizeof(ai->address), "%s", email_address);
	ri->email_list = eina_list_append(ri->email_list, ai);

	/* Don't need to search contact name for this kind of address. */
	if (display_name) {
		ri->display_name = g_strdup(display_name);
	} else {
		ri->display_name = g_strdup(ai->address);
	}

	debug_leave();
	return ri;
}

COMPOSER_ERROR_TYPE_E composer_util_recp_retrieve_recp_string(EmailComposerUGD *ugd, Evas_Object *obj, char **dest)
{
	debug_enter();

	const Eina_List *items = elm_multibuttonentry_items_get(obj);
	retv_if(!items, COMPOSER_ERROR_NONE);

	char *result = (char *)calloc(eina_list_count(items), BUF_LEN_L);
	retvm_if(!result, COMPOSER_ERROR_OUT_OF_MEMORY, "Memory allocation failed!");

	const Eina_List *l = NULL;
	Elm_Object_Item *mbe_item = NULL;

	EINA_LIST_FOREACH(items, l, mbe_item) {
		EmailRecpInfo *ri = (EmailRecpInfo *)elm_object_item_data_get(mbe_item);
		if (!ri) {
			debug_error("ri is NULL!");
			continue;
		}

		EmailAddressInfo *ai = (EmailAddressInfo *)eina_list_nth(ri->email_list, ri->selected_email_idx);
		if (!ai) {
			debug_error("ai is NULL!");
			continue;
		}

		debug_secure("display name: (%s)", ri->display_name);
		debug_secure("email address: (%s)", ai->address);

		if (ri->display_name) {
			char *processed_name = _util_recp_escaping_email_display_name(ri->display_name, strlen(ri->display_name));
			if (processed_name) {
				snprintf(result + strlen(result), BUF_LEN_L, "\"%s\" <%s>;", processed_name, ai->address);
				g_free(processed_name);
			} else {
				snprintf(result + strlen(result), BUF_LEN_L, "\"%s\" <%s>;", ri->display_name, ai->address);
			}
		} else {
			snprintf(result + strlen(result), BUF_LEN_L, "<%s>;", ai->address);
		}
	}

	debug_secure("result = (%s)", result);

	*dest = result;

	debug_leave();
	return COMPOSER_ERROR_NONE;
}