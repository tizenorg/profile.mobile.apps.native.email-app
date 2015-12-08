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
#include <glib.h>
#include <glib/gprintf.h>

#include "email-debug.h"
#include "email-common-types.h"
#include "email-mailbox.h"
#include "email-mailbox-list.h"
#include "email-mailbox-list-other-items.h"
#include "email-mailbox-toolbar.h"
#include "email-mailbox-util.h"
#include "email-mailbox-search.h"
#include "email-mailbox-sync.h"
#include "email-mailbox-more-menu.h"
#include "email-mailbox-title.h"

/*
 * Definitions
 */

/*
 * Globals
 */

/*
 * enum
 */

/*
 * Structures
 */

/*
 * Declaration for static functions
 */
void _mailbox_create_searchbar(EmailMailboxUGD *mailbox_ugd);
static void _searchbar_back_key_cb_clicked(void *data, Evas_Object *obj, void *event_info);
static void _searchbar_back_key_cb_pressed(void *data, Evas_Object *obj, void *event_info);
static void _searchbar_back_key_cb_unpressed(void *data, Evas_Object *obj, void *event_info);
static void _searchbar_enter_key_cb(void *data, Evas_Object *obj, void *event_info);
static void _searchbar_entry_changed_cb(void *data, Evas_Object *obj, void *event_info);
static void _mailbox_search_list_scroll_stop_cb(void *data, Evas_Object * obj, void *event_info);
static Eina_Bool _mailbox_searchbar_entry_set_focus(void *data);
static Eina_Bool _search_editfield_changed_timer_cb(void *data);

/*
 * Definition for static functions
 */

static void _searchbar_back_key_cb_clicked(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	if (!data) {
		debug_error("data is NULL");
		return;
	}
	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;
	mailbox_finish_search_mode(mailbox_ugd);

}

static void _searchbar_back_key_cb_pressed(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	if (!data) {
		debug_error("data is NULL");
		return;
	}
	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;
	edje_object_signal_emit(_EDJ(mailbox_ugd->searchbar_ly), "btn.pressed", "elm");
}

static void _searchbar_back_key_cb_unpressed(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	if (!data) {
		debug_error("data is NULL");
		return;
	}
	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;
	edje_object_signal_emit(_EDJ(mailbox_ugd->searchbar_ly), "btn.unpressed", "elm");

}

static void _searchbar_enter_key_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	if (!data) {
		debug_error("data is NULL");
		return;
	}
	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;
	if (mailbox_ugd->search_editfield.entry)
		ecore_imf_context_input_panel_hide((Ecore_IMF_Context *)elm_entry_imf_context_get(mailbox_ugd->search_editfield.entry));
}

static void _searchbar_entry_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;

	DELETE_TIMER_OBJECT(mailbox_ugd->search_entry_changed_timer);
	G_FREE(mailbox_ugd->current_entry_string);
	mailbox_ugd->current_entry_string = (char *)elm_entry_markup_to_utf8(elm_object_text_get(mailbox_ugd->search_editfield.entry));
	mailbox_ugd->search_entry_changed_timer = ecore_timer_add(0.5, _search_editfield_changed_timer_cb, (void *)mailbox_ugd);

}

static Eina_Bool _search_editfield_changed_timer_cb(void *data)
{
	debug_enter();
	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;
	debug_secure("search entry text: %s", mailbox_ugd->current_entry_string);

	mailbox_ugd->search_entry_changed_timer = NULL;

	if (mailbox_ugd->current_entry_string) {
		int entry_str_len = strlen(mailbox_ugd->current_entry_string);
		if ((entry_str_len == 0 && NULL != mailbox_ugd->last_entry_string) || (entry_str_len > 0)) {
			if (g_strcmp0(mailbox_ugd->current_entry_string, mailbox_ugd->last_entry_string)) {
				G_FREE(mailbox_ugd->last_entry_string);
				mailbox_ugd->last_entry_string = g_strdup(mailbox_ugd->current_entry_string);
				mailbox_show_search_result(mailbox_ugd);
				if (entry_str_len == 0) {
					Ecore_IMF_Context *imf_context = (Ecore_IMF_Context *)elm_entry_imf_context_get(mailbox_ugd->search_editfield.entry);
					if (imf_context)
						ecore_imf_context_input_panel_show(imf_context);
				}
			}
		}
		G_FREE(mailbox_ugd->current_entry_string);
	}
	return ECORE_CALLBACK_CANCEL;
}

void _mailbox_search_list_scroll_stop_cb(void *data, Evas_Object * obj, void *event_info)
{
	Evas_Object *entry = (Evas_Object *)data;
	if (!entry) {
		debug_warning("entry is NULL");
		return;
	}
	Ecore_IMF_Context *imf_context = (Ecore_IMF_Context *)elm_entry_imf_context_get(entry);

	if (imf_context) {
		ecore_imf_context_input_panel_hide(imf_context);
		debug_log("Hide SIP, mailbox list scroll starts.");
	}
}

static Eina_Bool _mailbox_searchbar_entry_set_focus(void *data)
{
	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;
	mailbox_ugd->search_entry_focus_idler = NULL;
	elm_object_focus_set(mailbox_ugd->search_editfield.entry, EINA_TRUE);
	return ECORE_CALLBACK_CANCEL;
}

/*
 * Definition for exported functions
 */

void _mailbox_create_searchbar(EmailMailboxUGD *mailbox_ugd)
{
	debug_enter();
	mailbox_ugd->searchbar_ly = elm_layout_add(mailbox_ugd->base.module->navi);
	elm_layout_file_set(mailbox_ugd->searchbar_ly, email_get_mailbox_theme_path(), "email/layout/searchbar");

	Evas_Object *back_button = elm_button_add(mailbox_ugd->searchbar_ly);
	elm_object_style_set(back_button, "naviframe/back_btn/default");
	evas_object_smart_callback_add(back_button, "clicked", _searchbar_back_key_cb_clicked, mailbox_ugd);
	evas_object_smart_callback_add(back_button, "pressed", _searchbar_back_key_cb_pressed, mailbox_ugd);
	evas_object_smart_callback_add(back_button, "unpressed", _searchbar_back_key_cb_unpressed, mailbox_ugd);
	elm_object_part_content_set(mailbox_ugd->searchbar_ly, "elm.swallow.button", back_button);

	email_common_util_editfield_create(mailbox_ugd->searchbar_ly, EF_TITLE_SEARCH | EF_CLEAR_BTN, &mailbox_ugd->search_editfield);
	elm_entry_input_panel_layout_set(mailbox_ugd->search_editfield.entry, ELM_INPUT_PANEL_LAYOUT_NORMAL);
	elm_entry_input_panel_return_key_type_set(mailbox_ugd->search_editfield.entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_SEARCH);
	elm_entry_autocapital_type_set(mailbox_ugd->search_editfield.entry, ELM_AUTOCAPITAL_TYPE_NONE);
	evas_object_smart_callback_add(mailbox_ugd->search_editfield.entry, "activated", _searchbar_enter_key_cb, mailbox_ugd);
	elm_object_part_content_set(mailbox_ugd->searchbar_ly, "elm.swallow.content", mailbox_ugd->search_editfield.layout);

}

/*
 * Definition for exported functions
 */

void mailbox_finish_search_mode(EmailMailboxUGD *mailbox_ugd)
{
	debug_log("[Search Bar] Canceled Callback Called");

	DELETE_TIMER_OBJECT(mailbox_ugd->search_entry_changed_timer);
	G_FREE(mailbox_ugd->current_entry_string);

	evas_object_smart_callback_del(mailbox_ugd->gl, "scroll,drag,stop", _mailbox_search_list_scroll_stop_cb);
	evas_object_smart_callback_del(mailbox_ugd->search_editfield.entry, "changed", _searchbar_entry_changed_cb);
	evas_object_smart_callback_del(mailbox_ugd->search_editfield.entry, "preedit,changed", _searchbar_entry_changed_cb);

	G_FREE(mailbox_ugd->last_entry_string);

	mailbox_ugd->b_searchmode = false;
	mailbox_create_no_contents_view(mailbox_ugd, false);

	mailbox_change_search_layout_state(mailbox_ugd, false);

	mailbox_list_refresh(mailbox_ugd, NULL);
	mailbox_naviframe_mailbox_button_add(mailbox_ugd);
	mailbox_show_compose_btn(mailbox_ugd);
}

void mailbox_free_mailbox_search_data(email_search_data_t *search_data)
{
	debug_enter();
	if (!search_data) {
		return;
	}

	FREE(search_data->body_text);
	FREE(search_data->subject);
	FREE(search_data->sender);
	FREE(search_data->recipient);
	FREE(search_data->attach_text);
	FREE(search_data);
}

void _search_button_clicked_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
	debug_enter();
	retm_if(!data, "data == NULL");

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;

	if (mailbox_ugd->b_searchmode) {
		debug_log("already in search mode, return");
		return;
	}
	mailbox_ugd->b_searchmode = true;

	DELETE_EVAS_OBJECT(mailbox_ugd->more_ctxpopup);

	DELETE_TIMER_OBJECT(mailbox_ugd->search_entry_changed_timer);
	G_FREE(mailbox_ugd->current_entry_string);
	mailbox_last_updated_time_item_remove(mailbox_ugd);
	if (mailbox_ugd->btn_mailbox)
		mailbox_naviframe_mailbox_button_remove(mailbox_ugd);
	mailbox_hide_compose_btn(mailbox_ugd);
	mailbox_get_more_progress_item_remove(mailbox_ugd);
	mailbox_load_more_messages_item_remove(mailbox_ugd);
	mailbox_no_more_emails_item_remove(mailbox_ugd);
	mailbox_send_all_btn_remove(mailbox_ugd);

	mailbox_create_no_contents_view(mailbox_ugd, true);

	mailbox_sync_cancel_all(mailbox_ugd);

	mailbox_change_search_layout_state(mailbox_ugd, true);
	elm_entry_context_menu_disabled_set(mailbox_ugd->search_editfield.entry, EINA_FALSE);
	evas_object_smart_callback_add(mailbox_ugd->search_editfield.entry, "changed", _searchbar_entry_changed_cb, mailbox_ugd);
	evas_object_smart_callback_add(mailbox_ugd->search_editfield.entry, "preedit,changed", _searchbar_entry_changed_cb, mailbox_ugd);
	evas_object_smart_callback_add(mailbox_ugd->gl, "scroll,drag,stop", _mailbox_search_list_scroll_stop_cb, mailbox_ugd->search_editfield.entry);
}

void mailbox_change_search_layout_state(EmailMailboxUGD *mailbox_ugd, bool show_search_layout)
{
	debug_enter();
	if (!mailbox_ugd) {
		debug_warning("mailbox_ugd is NULL");
		return;
	}

	if(show_search_layout) {
		if (!mailbox_ugd->search_editfield.layout) {
			debug_log("search_bar is not created, create search bar");
			_mailbox_create_searchbar(mailbox_ugd);
		}
		debug_log("title is visible, hide thread title");
		mailbox_view_title_unpack(mailbox_ugd);

		if (!evas_object_visible_get(mailbox_ugd->searchbar_ly)) {
			debug_log("search_bar is not visible, show search bar");
			elm_object_part_content_set(mailbox_ugd->sub_layout_search, "top_bar", mailbox_ugd->searchbar_ly);
			if (mailbox_ugd->last_entry_string) {
				elm_entry_entry_set(mailbox_ugd->search_editfield.entry, mailbox_ugd->last_entry_string);
				elm_entry_cursor_end_set(mailbox_ugd->search_editfield.entry);
			} else {
				elm_entry_entry_set(mailbox_ugd->search_editfield.entry, "");
			}
			mailbox_ugd->search_entry_focus_idler = ecore_idler_add(_mailbox_searchbar_entry_set_focus, mailbox_ugd);
		}
	} else {
		debug_log("title is not visible, show thread title.");
		elm_object_part_content_unset(mailbox_ugd->content_layout, "top_bar");
		evas_object_hide(mailbox_ugd->searchbar_ly);
		if (mailbox_ugd->searchbar_ly &&
			evas_object_visible_get(mailbox_ugd->searchbar_ly)) {
			if (evas_object_visible_get(mailbox_ugd->search_editfield.layout)) {
				debug_log("search entry is visible, hide search entry");
				if (!mailbox_ugd->b_searchmode)
					elm_entry_entry_set(mailbox_ugd->search_editfield.entry, "");
				elm_object_focus_set(mailbox_ugd->search_editfield.entry, EINA_FALSE);
				evas_object_hide(mailbox_ugd->search_editfield.layout);
			}
			debug_log("search_bar is visible, hide search bar");
			elm_object_part_content_unset(mailbox_ugd->sub_layout_search, "top_bar");

		}
		mailbox_view_title_pack(mailbox_ugd);
	}

}

email_search_data_t *mailbox_make_search_data(EmailMailboxUGD *mailbox_ugd)
{
	debug_enter();
	retvm_if(!mailbox_ugd, NULL, "mailbox_ugd is NULL");

	email_search_data_t *search_data = calloc(1, sizeof(email_search_data_t));
	retvm_if(!search_data, NULL, "search_data memory alloc failed");

	search_data->subject = (char *)elm_entry_markup_to_utf8(elm_object_text_get(mailbox_ugd->search_editfield.entry));
	search_data->body_text = (char *)elm_entry_markup_to_utf8(elm_object_text_get(mailbox_ugd->search_editfield.entry));
	search_data->attach_text = (char *)elm_entry_markup_to_utf8(elm_object_text_get(mailbox_ugd->search_editfield.entry));
	if ((mailbox_ugd->mailbox_type == EMAIL_MAILBOX_TYPE_SENTBOX)
			|| (mailbox_ugd->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX)
			|| (mailbox_ugd->mailbox_type == EMAIL_MAILBOX_TYPE_DRAFT)) {
		search_data->recipient = (char *)elm_entry_markup_to_utf8(elm_object_text_get(mailbox_ugd->search_editfield.entry));
	} else {
		search_data->sender = (char *)elm_entry_markup_to_utf8(elm_object_text_get(mailbox_ugd->search_editfield.entry));
	}
	debug_secure("[EMAIL_SEARCH_ALL] %s", search_data->subject);

	return search_data;
}

int mailbox_show_search_result(EmailMailboxUGD *mailbox_ugd)
{
	debug_enter();

	if (!mailbox_ugd)
		return 0;

	email_search_data_t *search_data = mailbox_make_search_data(mailbox_ugd);

	mailbox_list_refresh(mailbox_ugd, search_data);

	mailbox_free_mailbox_search_data(search_data);

	return 0;
}
