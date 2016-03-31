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

#include "email-debug.h"
#include "email-utils.h"
#include "email-editfield-utils.h"
#include "email-locale.h"

#define SEARCH_TITLE_COLOR_GUIDE_TEXT_FOCUSED "<font_size=40><color=#EAEAEA91>"
#define SEARCH_TITLE_COLOR_GUIDE_TEXT_UNFOCUSED "<font_size=40><color=#EAEAEA6D>"
#define SEARCH_TITLE_COLOR_GUIDE_TEXT_END_TEXT_TAG "</color></font_size>"
#define SEARCH_ENTRY_GUIDE_TEXT_SIZE 512

static void _editfield_focused_cb(void *data, Evas_Object *obj, void *event_info);
static void _editfield_unfocused_cb(void *data, Evas_Object *obj, void *event_info);
static void _editfield_with_clear_btn_focused_cb(void *data, Evas_Object *obj, void *event_info);
static void _editfield_with_clear_btn_unfocused_cb(void *data, Evas_Object *obj, void *event_info);
static void _editfield_search_title_focused_cb(void *data, Evas_Object *obj, void *event_info);
static void _editfield_search_title_unfocused_cb(void *data, Evas_Object *obj, void *event_info);
static void _editfield_with_clear_btn_changed_cb(void *data, Evas_Object *obj, void *event_info);
static void _clear_button_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static void _editfield_search_title_entry_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _on_system_settings_change_cb(system_settings_key_e key, void *user_data);

void email_common_util_editfield_create(Evas_Object *parent, int flags, email_editfield_t *editfield)
{
	debug_enter();
	retm_if(!parent, "Invalid parameter: parent is NULL!");
	retm_if(!editfield, "Invalid parameter: editfield is NULL!");

	editfield->layout = NULL;
	editfield->entry = NULL;

	Evas_Object *editfield_ly = elm_layout_add(parent);
	if (!editfield_ly) {
		debug_error("Failed to create layout");
		return;
	}

	if (flags & EF_TITLE_SEARCH) {
		elm_layout_file_set(editfield_ly, email_get_common_theme_path(), "title_search_editfield");
	} else if (flags & EF_MULTILINE) {
		elm_layout_theme_set(editfield_ly, "layout", "editfield", "multiline");
	} else {
		elm_layout_theme_set(editfield_ly, "layout", "editfield", "singleline");
	}

	evas_object_size_hint_align_set(editfield_ly, EVAS_HINT_FILL, 0.0);
	evas_object_size_hint_weight_set(editfield_ly, EVAS_HINT_EXPAND, 0.0);

	Evas_Object *entry = elm_entry_add(editfield_ly);
	if (!entry) {
		debug_error("Failed to create entry");
		DELETE_EVAS_OBJECT(editfield_ly);
		return;
	}

	elm_entry_cnp_mode_set(entry, ELM_CNP_MODE_PLAINTEXT);
	eext_entry_selection_back_event_allow_set(entry, EINA_TRUE);

	if (flags & EF_MULTILINE) {
		elm_entry_single_line_set(entry, EINA_FALSE);
		elm_entry_autocapital_type_set(entry, ELM_AUTOCAPITAL_TYPE_SENTENCE);
	} else {
		elm_entry_single_line_set(entry, EINA_TRUE);
		elm_entry_scrollable_set(entry, EINA_TRUE);
		elm_entry_autocapital_type_set(entry, ELM_AUTOCAPITAL_TYPE_NONE);
	}

	if (flags & EF_TITLE_SEARCH) {

		/* default entry text color */
		elm_entry_text_style_user_push(entry, "DEFAULT='color=#FAFAFAFF font_size=40'");

		/* register callback on language change for updating guide text */
		email_register_language_changed_callback(_on_system_settings_change_cb, editfield);

		/* register callback on font size change for updating guide text */
		email_register_accessibility_font_size_changed_callback(_on_system_settings_change_cb, editfield);

		/* needed for unregistering on language change callback when entry is deleted */
		evas_object_event_callback_add(entry, EVAS_CALLBACK_DEL, _editfield_search_title_entry_del_cb, editfield);

		/* additional callback for changing guide text color on focus event */
		evas_object_smart_callback_add(entry, "focused", _editfield_search_title_focused_cb, editfield);

		/* additional callback for changing guide text color on unfocus event */
		evas_object_smart_callback_add(entry, "unfocused", _editfield_search_title_unfocused_cb, editfield);
	}

	if (flags & EF_PASSWORD) {
		elm_entry_prediction_allow_set(entry, EINA_FALSE);
		elm_entry_input_panel_layout_set(entry, ELM_INPUT_PANEL_LAYOUT_PASSWORD);
		elm_entry_password_set(entry, EINA_TRUE);
	} else {
		elm_entry_prediction_allow_set(entry, EINA_TRUE);
		elm_entry_input_panel_layout_set(entry, ELM_INPUT_PANEL_LAYOUT_NORMAL);
	}

	if (flags & EF_CLEAR_BTN) {
		Evas_Object *button = elm_button_add(editfield_ly);
		if (!button) {
			debug_error("Failed to create clear button!");
			DELETE_EVAS_OBJECT(editfield_ly);
			DELETE_EVAS_OBJECT(entry);
			return;
		}

		if (flags & EF_TITLE_SEARCH) {
			elm_object_style_set(button, "transparent");
			Evas_Object *icon = elm_layout_add(button);
			elm_layout_file_set(icon, email_get_common_theme_path(), EMAIL_IMAGE_CORE_CLEAR);
			elm_object_part_content_set(button, "elm.swallow.content", icon);
			evas_object_show(icon);
		} else {
			elm_object_style_set(button, "editfield_clear");
		}
		evas_object_smart_callback_add(button, "clicked", _clear_button_clicked_cb, entry);
		evas_object_smart_callback_add(entry, "focused", _editfield_with_clear_btn_focused_cb, editfield_ly);
		evas_object_smart_callback_add(entry, "unfocused", _editfield_with_clear_btn_unfocused_cb, editfield_ly);
		evas_object_smart_callback_add(entry, "changed", _editfield_with_clear_btn_changed_cb, editfield_ly);
		evas_object_smart_callback_add(entry, "preedit,changed", _editfield_with_clear_btn_changed_cb, editfield_ly);
		elm_object_part_content_set(editfield_ly, "elm.swallow.button", button);
	} else {
		evas_object_smart_callback_add(entry, "focused", _editfield_focused_cb, editfield_ly);
		evas_object_smart_callback_add(entry, "unfocused", _editfield_unfocused_cb, editfield_ly);
	}

	elm_object_part_content_set(editfield_ly, "elm.swallow.content", entry);

	editfield->layout = editfield_ly;
	editfield->entry = entry;

	debug_leave();
}

static void _editfield_with_clear_btn_focused_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "Invalid parameter: data is NULL!");

	Evas_Object *editfield_ly = (Evas_Object *)data;

	elm_object_signal_emit(editfield_ly, "elm,state,focused", "");
	if (!elm_entry_is_empty(obj)) {
		elm_object_signal_emit(editfield_ly, "elm,action,show,button", "");
	}
	debug_leave();
}

static void _editfield_with_clear_btn_unfocused_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "Invalid parameter: data is NULL!");

	Evas_Object *editfield_ly = (Evas_Object *)data;
	elm_object_signal_emit(editfield_ly, "elm,state,unfocused", "");
	elm_object_signal_emit(editfield_ly, "elm,action,hide,button", "");
	debug_leave();
}

static void _editfield_search_title_focused_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "Invalid parameter: data is NULL!");

	email_editfield_t *editfield = (email_editfield_t *)data;
	char temp[SEARCH_ENTRY_GUIDE_TEXT_SIZE] = { 0 };
	snprintf(temp, sizeof(temp), "%s%s%s", SEARCH_TITLE_COLOR_GUIDE_TEXT_FOCUSED, _("IDS_EMAIL_OPT_SEARCH"), SEARCH_TITLE_COLOR_GUIDE_TEXT_END_TEXT_TAG);
	elm_object_part_text_set(editfield->entry, "elm.guide", temp);

	debug_leave();
}

static void _editfield_search_title_unfocused_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "Invalid parameter: data is NULL!");

	email_editfield_t *editfield = (email_editfield_t *)data;
	char temp[SEARCH_ENTRY_GUIDE_TEXT_SIZE] = { 0 };
	snprintf(temp, sizeof(temp), "%s%s%s", SEARCH_TITLE_COLOR_GUIDE_TEXT_UNFOCUSED, _("IDS_EMAIL_OPT_SEARCH"), SEARCH_TITLE_COLOR_GUIDE_TEXT_END_TEXT_TAG);
	elm_object_part_text_set(editfield->entry, "elm.guide", temp);

	debug_leave();
}

static void _editfield_with_clear_btn_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "Invalid parameter: data is NULL!");

	Evas_Object *editfield_ly = (Evas_Object *)data;
	if (!elm_entry_is_empty(obj) && elm_object_focus_get(obj)) {
		elm_object_signal_emit(editfield_ly, "elm,action,show,button", "");
	} else {
		elm_object_signal_emit(editfield_ly, "elm,action,hide,button", "");
	}
	debug_leave();
}

static void _clear_button_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "Invalid parameter: data is NULL!");

	if (!strcmp(elm_object_style_get(obj), "transparent")) {
		email_feedback_play_tap_sound();
	}

	Evas_Object *entry = (Evas_Object *)data;

	elm_entry_entry_set(entry, "");
	debug_leave();
}

static void _editfield_focused_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "Invalid parameter: data is NULL!");

	Evas_Object *editfield_ly = (Evas_Object *)data;
	elm_object_signal_emit(editfield_ly, "elm,state,focused", "");
	debug_leave();

}

static void _editfield_unfocused_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "Invalid parameter: data is NULL!");

	Evas_Object *editfield_ly = (Evas_Object *)data;
	elm_object_signal_emit(editfield_ly, "elm,state,unfocused", "");
	debug_leave();
}

static void _editfield_search_title_entry_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	email_editfield_t *editfield = (email_editfield_t *)data;
	editfield->entry = NULL;

	email_unregister_accessibility_font_size_changed_callback(_on_system_settings_change_cb);
	email_unregister_language_changed_callback(_on_system_settings_change_cb);
}

static void _on_system_settings_change_cb(system_settings_key_e key, void *data)
{
	email_editfield_t *editfield = (email_editfield_t *)data;

	if (!editfield->entry) {
		return;
	}

	char temp[SEARCH_ENTRY_GUIDE_TEXT_SIZE] = { 0 };
	const char *start_tag_string = NULL;

	if (elm_object_focus_get(editfield->entry)) {
		start_tag_string = SEARCH_TITLE_COLOR_GUIDE_TEXT_FOCUSED;
	} else {
		start_tag_string = SEARCH_TITLE_COLOR_GUIDE_TEXT_UNFOCUSED;
	}
	snprintf(temp, sizeof(temp), "%s%s%s", start_tag_string, _("IDS_EMAIL_OPT_SEARCH"), SEARCH_TITLE_COLOR_GUIDE_TEXT_END_TEXT_TAG);
	elm_object_part_text_set(editfield->entry, "elm.guide", temp);
}
