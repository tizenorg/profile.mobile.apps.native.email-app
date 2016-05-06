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
gchar *composer_util_get_ellipsised_entry_name(EmailComposerView *view, Evas_Object *display_entry, const char *entry_string)
{
	debug_enter();

	gchar *ellipsised_string = NULL;
	Evas_Object *text_block = NULL;
	Evas_Coord width = 0, height = 0;
	Evas_Coord label_width = 0;
	int allowed_entry_width = 0;
	int win_w = 0;

	if ((view->base.orientation == APP_DEVICE_ORIENTATION_0) ||
		(view->base.orientation == APP_DEVICE_ORIENTATION_180)) {
		elm_win_screen_size_get(view->base.module->win, NULL, NULL, &win_w, NULL);
	} else {
		elm_win_screen_size_get(view->base.module->win, NULL, NULL, NULL, &win_w);
	}

	if (display_entry == view->recp_to_display_entry.entry && view->recp_to_label) {
		evas_object_geometry_get(view->recp_to_label, NULL, NULL, &label_width, NULL);
	} else if (display_entry == view->recp_cc_display_entry.entry && view->recp_cc_label_cc) {
		evas_object_geometry_get(view->recp_cc_label_cc, NULL, NULL, &label_width, NULL);
	} else if (display_entry == view->recp_bcc_display_entry.entry && view->recp_bcc_label) {
		evas_object_geometry_get(view->recp_bcc_label, NULL, NULL, &label_width, NULL);
	}

	allowed_entry_width = win_w - label_width - ENTRY_SIZE_ADJUSTMENT;

	Evas_Object *entry = elm_entry_add(view->composer_layout);
	composer_util_set_entry_text_style(entry);
	elm_entry_entry_set(entry, entry_string);
	text_block = elm_entry_textblock_get(entry);
	evas_object_textblock_size_native_get(text_block, &width, &height);

	debug_secure("width %d, allowed_entry_width %d, entry_string %s", width, allowed_entry_width, entry_string);
	if (width > allowed_entry_width) {
		gchar buff[EMAIL_BUFF_SIZE_1K] = { 0 };
		gchar tmp[EMAIL_BUFF_SIZE_1K] = { 0 };
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

		snprintf(buff, EMAIL_BUFF_SIZE_1K, "...");
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

			n = snprintf(tmp, EMAIL_BUFF_SIZE_1K, "%s...%s", (first_half ? first_half : ""), (second_half ? second_half : ""));
			if (n >= EMAIL_BUFF_SIZE_1K) {
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

Eina_Bool composer_util_recp_is_duplicated(EmailComposerView *view, Evas_Object *obj, const char *email_address)
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

	EmailComposerView *view = (EmailComposerView *)data;
	Eina_Bool ret = EINA_TRUE;

	if ((elm_multibuttonentry_items_get(view->recp_to_mbe) != NULL) || (elm_multibuttonentry_items_get(view->recp_cc_mbe)) || (elm_multibuttonentry_items_get(view->recp_bcc_mbe))) {
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

EmailRecpInfo *composer_util_recp_make_recipient_info(const char *email_address)
{
	debug_enter();
	retvm_if(!email_address, NULL, "Invalid parameter: email_address is NULL!");

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
		addr = strdup(email_address);
	}
	if (nick) {
		ri->display_name = nick;
	}
	snprintf(ai->address, sizeof(ai->address), "%s", addr);
	ri->email_list = eina_list_append(ri->email_list, ai);

	g_free(ri->display_name);
	email_contact_list_info_t *contact_info = email_contacts_get_contact_info_by_email_address(ai->address);
	if (contact_info) {
		ri->display_name = g_strdup(contact_info->display_name);
		ri->email_id = contact_info->email_id;
		email_contacts_delete_contact_info(&contact_info);
	} else {
		ri->display_name = g_strdup(ai->address);
		ri->email_id = 0;
	}

	/* nick is used on ri->display_name */
	free(addr);
	debug_leave();
	return ri;
}

EmailRecpInfo *composer_util_recp_make_recipient_info_with_display_name(char *email_address, char *display_name)
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

COMPOSER_ERROR_TYPE_E composer_util_recp_retrieve_recp_string(EmailComposerView *view, Evas_Object *obj, char **dest)
{
	debug_enter();

	const Eina_List *items = elm_multibuttonentry_items_get(obj);
	retv_if(!items, COMPOSER_ERROR_NONE);

	char *result = (char *)calloc(eina_list_count(items), EMAIL_BUFF_SIZE_1K);
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
				snprintf(result + strlen(result), EMAIL_BUFF_SIZE_1K, "\"%s\" <%s>;", processed_name, ai->address);
				g_free(processed_name);
			} else {
				snprintf(result + strlen(result), EMAIL_BUFF_SIZE_1K, "\"%s\" <%s>;", ri->display_name, ai->address);
			}
		} else {
			snprintf(result + strlen(result), EMAIL_BUFF_SIZE_1K, "<%s>;", ai->address);
		}
	}

	debug_secure("result = (%s)", result);

	*dest = result;

	debug_leave();
	return COMPOSER_ERROR_NONE;
}
