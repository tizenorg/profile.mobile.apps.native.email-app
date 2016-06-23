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
void _mailbox_create_searchbar(EmailMailboxView *view);
static void _mailbox_searchbar_back_key_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static void _mailbox_searchbar_back_key_pressed_cb(void *data, Evas_Object *obj, void *event_info);
static void _mailbox_searchbar_back_key_unpressed_cb(void *data, Evas_Object *obj, void *event_info);
static void _mailbox_searchbar_enter_key_cb(void *data, Evas_Object *obj, void *event_info);
static void _mailbox_searchbar_entry_changed_cb(void *data, Evas_Object *obj, void *event_info);
static void _mailbox_search_list_scroll_stop_cb(void *data, Evas_Object * obj, void *event_info);
static Eina_Bool _mailbox_searchbar_entry_set_focus(void *data);
static Eina_Bool _mailbox_search_editfield_changed_timer_cb(void *data);

static void _mailbox_change_view_to_search_mode(EmailMailboxView *view);


/*
 * Definition for static functions
 */

static void _mailbox_searchbar_back_key_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "data is NULL");
	EmailMailboxView *view = (EmailMailboxView *)data;

	mailbox_back_key_click_handle(view);

	debug_leave();
}

static void _mailbox_searchbar_back_key_pressed_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	if (!data) {
		debug_error("data is NULL");
		return;
	}
	EmailMailboxView *view = (EmailMailboxView *)data;
	edje_object_signal_emit(_EDJ(view->searchbar_ly), "btn.pressed", "elm");
}

static void _mailbox_searchbar_back_key_unpressed_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	if (!data) {
		debug_error("data is NULL");
		return;
	}
	EmailMailboxView *view = (EmailMailboxView *)data;
	edje_object_signal_emit(_EDJ(view->searchbar_ly), "btn.unpressed", "elm");

}

static void _mailbox_searchbar_enter_key_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	if (!data) {
		debug_error("data is NULL");
		return;
	}
	EmailMailboxView *view = (EmailMailboxView *)data;
	if (view->search_editfield.entry)
		ecore_imf_context_input_panel_hide((Ecore_IMF_Context *)elm_entry_imf_context_get(view->search_editfield.entry));
}

static void _mailbox_searchbar_entry_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailMailboxView *view = (EmailMailboxView *)data;

	DELETE_TIMER_OBJECT(view->search_entry_changed_timer);
	G_FREE(view->current_entry_string);
	view->current_entry_string = (char *)elm_entry_markup_to_utf8(elm_object_text_get(view->search_editfield.entry));
	view->search_entry_changed_timer = ecore_timer_add(0.5, _mailbox_search_editfield_changed_timer_cb, (void *)view);

}

static Eina_Bool _mailbox_search_editfield_changed_timer_cb(void *data)
{
	debug_enter();
	EmailMailboxView *view = (EmailMailboxView *)data;
	debug_secure("search entry text: %s", view->current_entry_string);

	view->search_entry_changed_timer = NULL;

	if (view->current_entry_string) {
		int entry_str_len = strlen(view->current_entry_string);
		if ((entry_str_len == 0 && NULL != view->last_entry_string) || (entry_str_len > 0)) {
			if (g_strcmp0(view->current_entry_string, view->last_entry_string)) {
				G_FREE(view->last_entry_string);
				view->last_entry_string = g_strdup(view->current_entry_string);
				mailbox_show_search_result(view);
				if (entry_str_len == 0) {
					Ecore_IMF_Context *imf_context = (Ecore_IMF_Context *)elm_entry_imf_context_get(view->search_editfield.entry);
					if (imf_context)
						ecore_imf_context_input_panel_show(imf_context);
				}
			}
		}
		G_FREE(view->current_entry_string);
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
	EmailMailboxView *view = (EmailMailboxView *)data;
	view->search_entry_focus_idler = NULL;
	elm_object_focus_set(view->search_editfield.entry, EINA_TRUE);
	return ECORE_CALLBACK_CANCEL;
}

/*
 * Definition for exported functions
 */

void _mailbox_create_searchbar(EmailMailboxView *view)
{
	debug_enter();
	view->searchbar_ly = elm_layout_add(view->base.module->navi);
	elm_layout_file_set(view->searchbar_ly, email_get_mailbox_theme_path(), "email/layout/searchbar");

	Evas_Object *back_button = elm_button_add(view->searchbar_ly);
	elm_object_style_set(back_button, "naviframe/back_btn/default");
	evas_object_smart_callback_add(back_button, "clicked", _mailbox_searchbar_back_key_clicked_cb, view);
	evas_object_smart_callback_add(back_button, "pressed", _mailbox_searchbar_back_key_pressed_cb, view);
	evas_object_smart_callback_add(back_button, "unpressed", _mailbox_searchbar_back_key_unpressed_cb, view);
	elm_object_part_content_set(view->searchbar_ly, "elm.swallow.button", back_button);

	email_common_util_editfield_create(view->searchbar_ly, EF_TITLE_SEARCH | EF_CLEAR_BTN, &view->search_editfield);
	elm_entry_input_panel_layout_set(view->search_editfield.entry, ELM_INPUT_PANEL_LAYOUT_NORMAL);
	elm_entry_input_panel_return_key_type_set(view->search_editfield.entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_SEARCH);
	elm_entry_autocapital_type_set(view->search_editfield.entry, ELM_AUTOCAPITAL_TYPE_NONE);
	evas_object_smart_callback_add(view->search_editfield.entry, "activated", _mailbox_searchbar_enter_key_cb, view);
	elm_object_part_content_set(view->searchbar_ly, "elm.swallow.content", view->search_editfield.layout);

}

static void _mailbox_change_view_to_search_mode(EmailMailboxView *view)
{
	debug_enter();

	DELETE_TIMER_OBJECT(view->search_entry_changed_timer);
	G_FREE(view->current_entry_string);
	mailbox_last_updated_time_item_remove(view);
	if (view->btn_mailbox) {
		mailbox_naviframe_mailbox_button_remove(view);
	}

	mailbox_hide_compose_btn(view);
	mailbox_get_more_progress_item_remove(view);
	mailbox_load_more_messages_item_remove(view);
	mailbox_no_more_emails_item_remove(view);
	mailbox_send_all_btn_remove(view);

	mailbox_create_no_contents_view(view, true);
	mailbox_sync_cancel_all(view);
	mailbox_remove_refresh_progress_bar(view);

	mailbox_change_search_layout_state(view, true);
	elm_entry_context_menu_disabled_set(view->search_editfield.entry, EINA_FALSE);
	evas_object_smart_callback_add(view->search_editfield.entry, "changed", _mailbox_searchbar_entry_changed_cb, view);
	evas_object_smart_callback_add(view->search_editfield.entry, "preedit,changed", _mailbox_searchbar_entry_changed_cb, view);
	evas_object_smart_callback_add(view->gl, "scroll,drag,stop", _mailbox_search_list_scroll_stop_cb, view->search_editfield.entry);

	debug_leave();
}

/*
 * Definition for exported functions
 */

void mailbox_actiate_search_mode(EmailMailboxView *view, email_search_type_e search_type)
{
	debug_enter();

	retm_if(!view, "view == NULL");
	retm_if(view->b_searchmode && view->search_type == search_type, "Requested search mode is already set");

	if (!view->b_searchmode) {
		_mailbox_change_view_to_search_mode(view);
		view->b_searchmode = true;
	}

	view->search_type = search_type;

	DELETE_TIMER_OBJECT(view->search_entry_changed_timer);
	mailbox_show_search_result(view);

}

void mailbox_finish_search_mode(EmailMailboxView *view)
{
	debug_log("[Search Bar] Canceled Callback Called");

	DELETE_TIMER_OBJECT(view->search_entry_changed_timer);
	G_FREE(view->current_entry_string);

	evas_object_smart_callback_del(view->gl, "scroll,drag,stop", _mailbox_search_list_scroll_stop_cb);
	evas_object_smart_callback_del(view->search_editfield.entry, "changed", _mailbox_searchbar_entry_changed_cb);
	evas_object_smart_callback_del(view->search_editfield.entry, "preedit,changed", _mailbox_searchbar_entry_changed_cb);

	G_FREE(view->last_entry_string);

	view->b_searchmode = false;
	view->search_type = EMAIL_SEARCH_NONE;
	mailbox_create_no_contents_view(view, false);

	mailbox_change_search_layout_state(view, false);

	mailbox_list_refresh(view, NULL);
	mailbox_naviframe_mailbox_button_add(view);
	mailbox_show_compose_btn(view);
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

void mailbox_change_search_layout_state(EmailMailboxView *view, bool show_search_layout)
{
	debug_enter();
	if (!view) {
		debug_warning("view is NULL");
		return;
	}

	if(show_search_layout) {
		if (!view->search_editfield.layout) {
			debug_log("search_bar is not created, create search bar");
			_mailbox_create_searchbar(view);
		}
		debug_log("title is visible, hide thread title");
		mailbox_view_title_unpack(view);

		if (!evas_object_visible_get(view->searchbar_ly)) {
			debug_log("search_bar is not visible, show search bar");
			elm_object_part_content_set(view->sub_layout_search, "top_bar", view->searchbar_ly);
			if (view->last_entry_string) {
				elm_entry_entry_set(view->search_editfield.entry, view->last_entry_string);
				elm_entry_cursor_end_set(view->search_editfield.entry);
			} else {
				elm_entry_entry_set(view->search_editfield.entry, "");
			}
			view->search_entry_focus_idler = ecore_idler_add(_mailbox_searchbar_entry_set_focus, view);
		}
	} else {
		debug_log("title is not visible, show thread title.");
		elm_object_part_content_unset(view->content_layout, "top_bar");
		evas_object_hide(view->searchbar_ly);
		if (view->searchbar_ly &&
			evas_object_visible_get(view->searchbar_ly)) {
			if (evas_object_visible_get(view->search_editfield.layout)) {
				debug_log("search entry is visible, hide search entry");
				if (!view->b_searchmode)
					elm_entry_entry_set(view->search_editfield.entry, "");
				elm_object_focus_set(view->search_editfield.entry, EINA_FALSE);
				evas_object_hide(view->search_editfield.layout);
			}
			debug_log("search_bar is visible, hide search bar");
			elm_object_part_content_unset(view->sub_layout_search, "top_bar");

		}
		mailbox_view_title_pack(view);
	}

}

email_search_data_t *mailbox_make_search_data(EmailMailboxView *view)
{
	debug_enter();
	retvm_if(!view, NULL, "view is NULL");

	email_search_data_t *search_data = calloc(1, sizeof(email_search_data_t));
	retvm_if(!search_data, NULL, "search_data memory alloc failed");

	search_data->subject = (char *)elm_entry_markup_to_utf8(elm_object_text_get(view->search_editfield.entry));
	search_data->body_text = (char *)elm_entry_markup_to_utf8(elm_object_text_get(view->search_editfield.entry));
	search_data->attach_text = (char *)elm_entry_markup_to_utf8(elm_object_text_get(view->search_editfield.entry));
	if ((view->mailbox_type == EMAIL_MAILBOX_TYPE_SENTBOX)
			|| (view->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX)
			|| (view->mailbox_type == EMAIL_MAILBOX_TYPE_DRAFT)) {
		search_data->recipient = (char *)elm_entry_markup_to_utf8(elm_object_text_get(view->search_editfield.entry));
	} else {
		search_data->sender = (char *)elm_entry_markup_to_utf8(elm_object_text_get(view->search_editfield.entry));
	}
	debug_secure("[EMAIL_SEARCH_ALL] %s", search_data->subject);

	return search_data;
}

int mailbox_show_search_result(EmailMailboxView *view)
{
	debug_enter();
	retvm_if(!view, 0, "Invalid arguments");

	email_search_data_t *search_data = mailbox_make_search_data(view);

	mailbox_list_refresh(view, search_data);

	mailbox_free_mailbox_search_data(search_data);

	return 0;
}

void mailbox_back_key_click_handle(EmailMailboxView *view)
{
	debug_enter();

	if (view->search_type == EMAIL_SEARCH_IN_ALL_FOLDERS) {
		mailbox_actiate_search_mode(view, EMAIL_SEARCH_IN_SINGLE_FOLDER);
	} else if (view->search_type == EMAIL_SEARCH_IN_SINGLE_FOLDER) {
		mailbox_finish_search_mode(view);
	}

	debug_leave();
}
