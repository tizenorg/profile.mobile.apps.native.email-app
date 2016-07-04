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
#include "email-mailbox-list-extensions.h"
#include "email-mailbox-toolbar.h"
#include "email-mailbox-util.h"
#include "email-mailbox-search.h"
#include "email-mailbox-sync.h"
#include "email-mailbox-more-menu.h"
#include "email-mailbox-title.h"

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
static void _mailbox_change_search_layout_state(EmailMailboxView *view, bool show_search_layout);
static void _mailbox_exit_search_mode(EmailMailboxView *view);
static void _mailbox_show_search_result(EmailMailboxView *view);

/*Server search functionality*/
static Eina_Bool _mailbox_is_server_search_supported(EmailMailboxView *view);
static void _mailbox_setup_server_search_mode(EmailMailboxView *view);
static void _mailbox_exit_server_search_mode(EmailMailboxView *view);
static void _mailbox_search_init_sever_search_time_range(EmailMailboxView *view);
static void _mailbox_server_search_update_from_time_value(EmailMailboxView *view, time_t new_from_time);
static void _mailbox_server_search_update_to_time_value(EmailMailboxView *view, time_t new_to_time);

/*Server search button item*/
static void _mailbox_add_server_search_btn(EmailMailboxView *view);
static void _mailbox_remove_server_search_btn(EmailMailboxView *view);
static Evas_Object *_mailbox_server_search_btn_gl_content_get_cb(void *data, Evas_Object *obj, const char *source);
static void _mailbox_server_search_btn_gl_item_del_cb(void *data, Evas_Object *obj);
static void _mailbox_server_search_btn_click_cb(void *data, Evas_Object *obj, void *event_info);

/*Server search processing item*/
static void _mailbox_add_server_search_processing_item(EmailMailboxView *view);
static void _mailbox_remove_server_search_processing_item(EmailMailboxView *view);
static char *_mailbox_search_processing_gl_text_get_cb(void *data, Evas_Object *obj, const char *source);
static Evas_Object *_mailbox_search_processing_gl_content_get_cb(void *data, Evas_Object *obj, const char *source);
static void _mailbox_server_search_processing_item_del_cb(void *data, Evas_Object *obj);

/*Date range selection popup logic*/
static void _mailbox_search_create_date_range_popup(EmailMailboxView *view);
static void _mailbox_search_date_range_popup_cancel_btn_click_cb(void *data, Evas_Object *obj, void *event_info);
static void _mailbox_search_date_range_popup_done_btn_click_cb(void *data, Evas_Object *obj, void *event_info);
static void _mailbox_search_date_range_popup_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _mailbox_search_date_range_popup_from_btn_click_cb(void *data, Evas_Object *obj, void *event_info);
static void _mailbox_search_date_range_popup_to_btn_click_cb(void *data, Evas_Object *obj, void *event_info);
static void _mailbox_search_date_range_popup_set_date_to_date_btn(Evas_Object *button, time_t date);

/*Date picker popup*/
static void _mailbox_search_create_date_picker_popup(EmailMailboxView *view, MailboxDatePickerRangeType range_type);
static void _mailbox_search_date_picker_popup_set_btn_click_cb(void *data, Evas_Object *obj, void *event_info);
static void _mailbox_search_date_picker_popup_cancel_btn_click_cb(void *data, Evas_Object *obj, void *event_info);
static void _mailbox_search_date_picker_popup_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

static const Elm_Genlist_Item_Class itc_server_search_btn = {
	ELM_GEN_ITEM_CLASS_HEADER,
	.item_style = "full",
	.func.text_get = NULL,
	.func.content_get = _mailbox_server_search_btn_gl_content_get_cb,
	.func.del = _mailbox_server_search_btn_gl_item_del_cb,

};

static const Elm_Genlist_Item_Class itc_server_search_progress = {
	ELM_GEN_ITEM_CLASS_HEADER,
	.item_style = "type1",
	.func.text_get = _mailbox_search_processing_gl_text_get_cb,
	.func.content_get = _mailbox_search_processing_gl_content_get_cb,
	.func.del = _mailbox_server_search_processing_item_del_cb,
};

/*
 * Definition for static functions
 */

static void _mailbox_searchbar_back_key_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "data is NULL");
	EmailMailboxView *view = (EmailMailboxView *)data;

	email_module_view_dispatch_back_key_press(&view->base);
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

	debug_leave();
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

	debug_leave();
}

static void _mailbox_searchbar_enter_key_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	if (!data) {
		debug_error("data is NULL");
		return;
	}
	EmailMailboxView *view = (EmailMailboxView *)data;
	if (view->search_editfield.entry) {
		elm_entry_input_panel_hide(view->search_editfield.entry);
	}
}

static void _mailbox_searchbar_entry_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailMailboxView *view = (EmailMailboxView *)data;

	DELETE_TIMER_OBJECT(view->search_entry_changed_timer);
	G_FREE(view->current_entry_string);
	view->current_entry_string = (char *)elm_entry_markup_to_utf8(elm_object_text_get(view->search_editfield.entry));
	view->search_entry_changed_timer = ecore_timer_add(0.5, _mailbox_search_editfield_changed_timer_cb, (void *)view);

	debug_leave();
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
				_mailbox_show_search_result(view);
				if (entry_str_len == 0) {
					elm_object_focus_set(view->search_editfield.entry, EINA_TRUE);
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

	elm_entry_input_panel_hide(entry);
	debug_log("Hide SIP, mailbox list scroll starts.");

}

static Eina_Bool _mailbox_searchbar_entry_set_focus(void *data)
{
	EmailMailboxView *view = (EmailMailboxView *)data;
	view->search_entry_focus_idler = NULL;
	elm_object_focus_set(view->search_editfield.entry, EINA_TRUE);
	return ECORE_CALLBACK_CANCEL;
}

static void _mailbox_exit_search_mode(EmailMailboxView *view)
{
	debug_enter();

	DELETE_TIMER_OBJECT(view->search_entry_changed_timer);
	G_FREE(view->current_entry_string);

	evas_object_smart_callback_del(view->gl, "scroll,drag,stop", _mailbox_search_list_scroll_stop_cb);
	evas_object_smart_callback_del(view->search_editfield.entry, "changed", _mailbox_searchbar_entry_changed_cb);
	evas_object_smart_callback_del(view->search_editfield.entry, "preedit,changed", _mailbox_searchbar_entry_changed_cb);

	G_FREE(view->last_entry_string);

	view->b_searchmode = false;
	view->search_type = EMAIL_SEARCH_NONE;

	_mailbox_change_search_layout_state(view, false);

	mailbox_list_refresh(view, NULL);
	mailbox_naviframe_mailbox_button_add(view);
	mailbox_show_compose_btn(view);
	mailbox_folders_name_cache_clear(view);
	_mailbox_remove_server_search_btn(view);

	debug_leave();
}

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

	debug_leave();

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

	mailbox_sync_cancel_all(view);
	mailbox_remove_refresh_progress_bar(view);

	_mailbox_change_search_layout_state(view, true);
	elm_entry_context_menu_disabled_set(view->search_editfield.entry, EINA_FALSE);
	evas_object_smart_callback_add(view->search_editfield.entry, "changed", _mailbox_searchbar_entry_changed_cb, view);
	evas_object_smart_callback_add(view->search_editfield.entry, "preedit,changed", _mailbox_searchbar_entry_changed_cb, view);
	evas_object_smart_callback_add(view->gl, "scroll,drag,stop", _mailbox_search_list_scroll_stop_cb, view->search_editfield.entry);

	if (_mailbox_is_server_search_supported(view)) {
		_mailbox_add_server_search_btn(view);
	}

	debug_leave();
}

static void _mailbox_change_search_layout_state(EmailMailboxView *view, bool show_search_layout)
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

	debug_leave();
}

static Eina_Bool _mailbox_is_server_search_supported(EmailMailboxView *view)
{
	debug_enter();

	if (view->account_type == EMAIL_SERVER_TYPE_IMAP4 && view->mode == EMAIL_MAILBOX_MODE_MAILBOX
			&& view->search_type != EMAIL_SEARCH_IN_ALL_FOLDERS
			&& GET_ACCOUNT_EMAIL_SYNC_DISABLE_STATUS(view->account_id) == false) {
		return EINA_TRUE;
	}

	debug_leave();
	return EINA_FALSE;
}

static void _mailbox_setup_server_search_mode(EmailMailboxView *view)
{
	debug_enter();

	if (difftime(view->search_to_time, view->search_from_time) <= 0.0) {
		_mailbox_search_init_sever_search_time_range(view);
	}

	_mailbox_search_create_date_range_popup(view);
	debug_leave();
}

static void _mailbox_exit_server_search_mode(EmailMailboxView *view)
{
	debug_enter();

	DELETE_EVAS_OBJECT(view->datepicker_data->popup);
	DELETE_EVAS_OBJECT(view->date_range_popup);
	_mailbox_remove_server_search_processing_item(view);

	debug_leave();
}

static void _mailbox_add_server_search_btn(EmailMailboxView *view)
{
	debug_enter();
	retm_if(view->search_btn_item_data.base.item != NULL, "Server search button item is already exist!");

	Elm_Object_Item *item = elm_genlist_item_append(view->gl,
									&itc_server_search_btn,
									&view->search_btn_item_data,
									NULL,
									ELM_GENLIST_ITEM_NONE,
									NULL,
									NULL);

	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_NONE);
	view->search_btn_item_data.base.item = item;
	if (view->no_content_shown) {
		mailbox_hide_no_contents_view(view);
	}

	debug_leave();
}

static void  _mailbox_server_search_btn_gl_item_del_cb(void *data, Evas_Object *obj)
{
	debug_enter();

	ServerSearchBtnItemData *item_data = (ServerSearchBtnItemData *)data;
	item_data->base.item = NULL;

	debug_leave();
}

static void _mailbox_remove_server_search_btn(EmailMailboxView *view)
{
	debug_enter();

	DELETE_ELM_OBJECT_ITEM(view->search_btn_item_data.base.item);

	debug_leave();
}

static Evas_Object *_mailbox_server_search_btn_gl_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
	if (!strcmp(part, "elm.swallow.content")) {
		retvm_if(!data, NULL, "Invalid arguments!");

		ServerSearchBtnItemData *item_data = (ServerSearchBtnItemData *)data;
		Evas_Object *layout = elm_layout_add(obj);
		elm_layout_file_set(layout, email_get_mailbox_theme_path(), "email/layout/server_search_btn_item");

		Evas_Object *search_btn = elm_button_add(layout);
		elm_object_style_set(search_btn, "default");
		elm_object_domain_translatable_text_set(search_btn, PACKAGE, "IDS_EMAIL_BUTTON_SEARCH_SERVER_ABB");

		evas_object_smart_callback_add(search_btn, "clicked", _mailbox_server_search_btn_click_cb, item_data->view);
		elm_layout_content_set(layout, "swallow.server_search.btn", search_btn);

		return layout;
	}

	return NULL;
}

static void _mailbox_server_search_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "Invalid arguments!");
	EmailMailboxView *view = data;

	mailbox_set_search_mode(view, EMAIL_SEARCH_ON_SERVER);

	debug_leave();
}

static void _mailbox_server_search_update_from_time_value(EmailMailboxView *view, time_t new_from_time)
{
	debug_enter();
	retm_if(difftime(view->search_to_time, new_from_time) < 0.0, "Attempt to set from_time greater than to_time! Rejected");

	view->search_from_time = new_from_time;
	if (view->date_btn_from) {
		_mailbox_search_date_range_popup_set_date_to_date_btn(view->date_btn_from, new_from_time);
	}

	debug_leave();
}

static void _mailbox_server_search_update_to_time_value(EmailMailboxView *view, time_t new_to_time)
{
	debug_enter();
	retm_if(difftime(new_to_time, view->search_from_time) < 0.0, "Attempt to set to_time less than from_time! Rejected");

	view->search_to_time = new_to_time;
	if (view->date_btn_to) {
		_mailbox_search_date_range_popup_set_date_to_date_btn(view->date_btn_to, new_to_time);
	}

	debug_leave();
}

static void _mailbox_search_init_sever_search_time_range(EmailMailboxView *view)
{
	debug_enter();
	retm_if(!view, "Invalid argument!");

	time_t current_time = time(NULL);
	struct tm *tm_time = localtime(&current_time);

	/*Setup default value of TO Search time as current date*/
	tm_time->tm_hour = 23;
	tm_time->tm_min = 59;
	tm_time->tm_sec = 59;
	view->search_to_time = mktime(tm_time);

	/*Setup default value of FROM Search time as one week later*/
	view->search_from_time = view->search_to_time - 7*24*60*60;

	debug_leave();
}

static void _mailbox_search_create_date_range_popup(EmailMailboxView *view)
{
	debug_enter();
	retm_if(!view, "Invalid argument!");

	Evas_Object *popup = mailbox_create_popup(view,
			"IDS_EMAIL_HEADER_SET_DATE_RANGE_ABB",
			NULL,
			_mailbox_search_date_range_popup_cancel_btn_click_cb,
			_mailbox_search_date_range_popup_cancel_btn_click_cb,
			"IDS_EMAIL_BUTTON_CANCEL",
			_mailbox_search_date_range_popup_done_btn_click_cb,
			"IDS_EMAIL_BUTTON_DONE");
	evas_object_event_callback_add(popup, EVAS_CALLBACK_DEL, _mailbox_search_date_range_popup_del_cb, view);

	Evas_Object *content_layout = elm_layout_add(popup);
	elm_layout_file_set(content_layout, email_get_mailbox_theme_path(), "email/layout/date_range_popup");

	elm_object_domain_translatable_part_text_set(content_layout, "text_label.from", PACKAGE, "IDS_EMAIL_BODY_FROM_M_DATE");
	Evas_Object *from_btn = elm_button_add(content_layout);
	elm_layout_content_set(content_layout, "swallow.date_button.from", from_btn);
	evas_object_smart_callback_add(from_btn, "clicked", _mailbox_search_date_range_popup_from_btn_click_cb, view);

	elm_object_domain_translatable_part_text_set(content_layout, "text_label.to", PACKAGE, "IDS_EMAIL_BODY_TO_M_DATE");
	Evas_Object *to_btn = elm_button_add(content_layout);
	elm_layout_content_set(content_layout, "swallow.date_button.to", to_btn);
	evas_object_smart_callback_add(to_btn, "clicked", _mailbox_search_date_range_popup_to_btn_click_cb, view);

	elm_object_content_set(popup, content_layout);
	view->date_btn_from = from_btn;
	view->date_btn_to = to_btn;
	view->date_range_popup = popup;

	_mailbox_search_date_range_popup_set_date_to_date_btn(from_btn, view->search_from_time);
	_mailbox_search_date_range_popup_set_date_to_date_btn(to_btn, view->search_to_time);

	evas_object_show(popup);
	debug_leave();
}

static void _mailbox_search_date_range_popup_set_date_to_date_btn(Evas_Object *date_button, time_t date)
{
	debug_enter();
	retm_if(!date_button, "Invalid argument");

	char *from_formatted_text = mailbox_get_formatted_date(date);
	elm_object_text_set(date_button, from_formatted_text);
	FREE(from_formatted_text);

	debug_leave();
}

static void _mailbox_search_date_range_popup_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "Invalid arguments");
	EmailMailboxView *view = data;

	view->date_btn_from = NULL;
	view->date_btn_to = NULL;
	view->date_range_popup = NULL;

	debug_leave();
}
static void _mailbox_search_date_range_popup_cancel_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "Invalid arguments");
	EmailMailboxView *view = data;

	DELETE_EVAS_OBJECT(view->date_range_popup);
	mailbox_set_search_mode(view, EMAIL_SEARCH_IN_SINGLE_FOLDER);

	debug_leave();
}

static void _mailbox_search_date_range_popup_done_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "Invalid arguments");
	EmailMailboxView *view = data;

	DELETE_EVAS_OBJECT(view->date_range_popup);
	//TODO Logic for running server search will be added here
	debug_leave();
}

static void _mailbox_search_date_range_popup_from_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "Invalid arguments");
	EmailMailboxView *view = data;
	_mailbox_search_create_date_picker_popup(view, DATE_PICKER_RANGE_TYPE_FROM_DATE);

	debug_leave();
}

static void _mailbox_search_date_range_popup_to_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "Invalid arguments");
	EmailMailboxView *view = data;
	_mailbox_search_create_date_picker_popup(view, DATE_PICKER_RANGE_TYPE_TO_DATE);
	debug_leave();
}

static void _mailbox_search_create_date_picker_popup(EmailMailboxView *view, MailboxDatePickerRangeType range_type)
{
	debug_enter();
	retm_if(!view, "Invalid arguments");

	MailboxDatePickerData *datepicker_data = MEM_ALLOC(datepicker_data, 1);
	retm_if(!datepicker_data, "Memory allocation failed!");

	view->datepicker_data = datepicker_data;
	datepicker_data->range_type = range_type;

	Evas_Object *popup = mailbox_create_popup(view,
			"IDS_EMAIL_HEADER_SET_DATE",
			NULL,
			_mailbox_search_date_picker_popup_cancel_btn_click_cb,
			_mailbox_search_date_picker_popup_cancel_btn_click_cb,
			"IDS_EMAIL_BUTTON_CANCEL",
			_mailbox_search_date_picker_popup_set_btn_click_cb,
			"IDS_EMAIL_BUTTON_SET");

	evas_object_event_callback_add(datepicker_data->popup, EVAS_CALLBACK_DEL, _mailbox_search_date_picker_popup_del_cb, datepicker_data);

	Evas_Object *datetime = elm_datetime_add(popup);

	Elm_Datetime_Time from_time;
	memset(&from_time, 0, sizeof(Elm_Datetime_Time));
	localtime_r(&view->search_from_time, &from_time);

	Elm_Datetime_Time to_time;
	memset(&to_time, 0, sizeof(Elm_Datetime_Time));
	localtime_r(&view->search_to_time, &to_time);

	if (range_type == DATE_PICKER_RANGE_TYPE_FROM_DATE) {
		elm_datetime_value_set(datetime, &from_time);
		elm_datetime_value_max_set(datetime, &to_time);
	} else if (range_type == DATE_PICKER_RANGE_TYPE_TO_DATE) {
		elm_datetime_value_set(datetime, &to_time);
		elm_datetime_value_min_set(datetime, &from_time);
	}

	datepicker_data->popup = popup;
	datepicker_data->datetime_obj = datetime;

	elm_object_content_set(popup, datetime);
	evas_object_show(popup);

	debug_leave();
}

static void _mailbox_search_date_picker_popup_set_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "Invalid arguments");
	EmailMailboxView *view = data;
	MailboxDatePickerData *popup_data = view->datepicker_data;

	Elm_Datetime_Time raw_time;
	memset(&raw_time, 0, sizeof(Elm_Datetime_Time));

	Eina_Bool res = elm_datetime_value_get(popup_data->datetime_obj, &raw_time);
	retm_if(!res, "Failed to obtain selected time!");

	time_t converted_time = mktime(&raw_time);

	if (popup_data->range_type == DATE_PICKER_RANGE_TYPE_FROM_DATE) {
		_mailbox_server_search_update_from_time_value(view, converted_time);
	} else if (popup_data->range_type == DATE_PICKER_RANGE_TYPE_TO_DATE) {
		_mailbox_server_search_update_to_time_value(view, converted_time);
	}

	DELETE_EVAS_OBJECT(popup_data->popup);
	debug_leave();
}

static void _mailbox_search_date_picker_popup_cancel_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "Invalid arguments");
	EmailMailboxView *view = data;

	DELETE_EVAS_OBJECT(view->datepicker_data->popup);
	debug_leave();
}

static void _mailbox_search_date_picker_popup_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "Invalid arguments");
	EmailMailboxView *view = data;

	FREE(view->datepicker_data);
	debug_leave();
}

//TODO will be used when server search logic added
static void _mailbox_add_server_search_processing_item(EmailMailboxView *view)
{
	debug_enter();
	retm_if(view->search_progress_item_data.item, "Server search processing item is already exist!");

	Elm_Object_Item *item = elm_genlist_item_append(view->gl,
									&itc_server_search_progress,
									&view->search_progress_item_data,
									NULL,
									ELM_GENLIST_ITEM_NONE,
									NULL,
									NULL);

	retm_if(!item, "Failed to create server search button item!");

	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_NONE);
	view->search_progress_item_data.item = item;

	debug_leave();
}

//TODO will be used when server search logic added
static void _mailbox_remove_server_search_processing_item(EmailMailboxView *view)
{
	debug_enter();

	if (view->search_progress_item_data.item) {
		elm_object_item_del(view->search_progress_item_data.item);
		view->search_progress_item_data.item = NULL;
	}

	debug_leave();
}

static char *_mailbox_search_processing_gl_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	if (!strcmp(part, "elm.text")) {
		return strdup(_("IDS_EMAIL_BODY_SEARCHING_ING"));
	}

	return NULL;
}

static Evas_Object *_mailbox_search_processing_gl_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
	if (!strcmp(part, "elm.swallow.icon")) {
		Evas_Object *progressbar = elm_progressbar_add(obj);
		elm_object_style_set(progressbar, "process_medium");
		elm_progressbar_pulse(progressbar, EINA_TRUE);
		return progressbar;
	}

	return NULL;
}

static void _mailbox_server_search_processing_item_del_cb(void *data, Evas_Object *obj)
{
	debug_enter();

	retm_if(!data, "Invalid argumets!");

	MailboxBaseItemData *item_data = data;
	item_data->item = NULL;

	debug_leave();
}

static void _mailbox_show_search_result(EmailMailboxView *view)
{
	debug_enter();

	retm_if(!view, "Invalid arguments");

	email_search_data_t *search_data = mailbox_make_search_data(view);
	retm_if(!search_data, "Failed to create search data");

	mailbox_list_refresh(view, search_data);
	mailbox_free_mailbox_search_data(search_data);

	if (_mailbox_is_server_search_supported(view)) {
		_mailbox_add_server_search_btn(view);
	} else if (!g_list_length(view->mail_list)) {
		mailbox_show_no_contents_view(view);
	}

	debug_leave();
}

/*
 * Definition for exported functions
 */

void mailbox_set_search_mode(EmailMailboxView *view, email_search_type_e search_type)
{
	debug_enter();

	retm_if(!view, "view == NULL");
	retm_if(view->search_type == search_type, "Requested search mode is already set");

	if (search_type == EMAIL_SEARCH_NONE) {
		debug_log("Exiting search mode...");
		_mailbox_exit_search_mode(view);
		return;
	}

	if (!view->b_searchmode) {
		_mailbox_change_view_to_search_mode(view);
		mailbox_folders_name_cache_clear(view);
		view->b_searchmode = true;
	}

	DELETE_TIMER_OBJECT(view->search_entry_changed_timer);
	if (view->search_type == EMAIL_SEARCH_ON_SERVER) {
		_mailbox_exit_server_search_mode(view);
	}

	view->search_type = search_type;
	if (view->search_type == EMAIL_SEARCH_ON_SERVER) {
		_mailbox_setup_server_search_mode(view);
	} else {
		_mailbox_show_search_result(view);
	}

	debug_leave();
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

	debug_leave();
}

email_search_data_t *mailbox_make_search_data(EmailMailboxView *view)
{
	debug_enter();
	retvm_if(!view, NULL, "view is NULL");

	email_search_data_t *search_data = calloc(1, sizeof(email_search_data_t));
	retvm_if(!search_data, NULL, "search_data memory alloc failed");

	search_data->subject = elm_entry_markup_to_utf8(elm_object_text_get(view->search_editfield.entry));
	search_data->body_text = elm_entry_markup_to_utf8(elm_object_text_get(view->search_editfield.entry));
	search_data->attach_text = elm_entry_markup_to_utf8(elm_object_text_get(view->search_editfield.entry));
	if (view->search_type == EMAIL_SEARCH_IN_ALL_FOLDERS) {
		search_data->recipient = elm_entry_markup_to_utf8(elm_object_text_get(view->search_editfield.entry));
		search_data->sender = elm_entry_markup_to_utf8(elm_object_text_get(view->search_editfield.entry));
	} else if ((view->mailbox_type == EMAIL_MAILBOX_TYPE_SENTBOX)
			|| (view->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX)
			|| (view->mailbox_type == EMAIL_MAILBOX_TYPE_DRAFT)) {
		search_data->recipient = elm_entry_markup_to_utf8(elm_object_text_get(view->search_editfield.entry));
		search_data->sender = NULL;
	} else {
		search_data->recipient = NULL;
		search_data->sender = elm_entry_markup_to_utf8(elm_object_text_get(view->search_editfield.entry));
	}

	debug_secure("[EMAIL_SEARCH_ALL] %s", search_data->subject);
	debug_leave();
	return search_data;
}

void mailbox_update_search_view(EmailMailboxView *view)
{
	debug_enter();
	retm_if(!view, "view is NULL");

	if (view->search_type == EMAIL_SEARCH_ON_SERVER && view->date_range_popup) {
		_mailbox_search_date_range_popup_set_date_to_date_btn(view->date_btn_from, view->search_from_time);
		_mailbox_search_date_range_popup_set_date_to_date_btn(view->date_btn_to, view->search_to_time);
	}

	_mailbox_show_search_result(view);
	debug_leave();
}
