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

#include <notification.h>

#include "email-composer.h"
#include "email-composer-util.h"
#include "email-composer-recipient.h"
#include "email-composer-attachment.h"
#include "email-composer-webkit.h"
#include "email-composer-more-menu.h"
#include "email-composer-subject.h"
#include "email-composer-js.h"
#include "email-composer-rich-text-toolbar.h"

/*
 * Definitions
 */

/* Declaration for static functions */

static void _subject_set_entry_guide_text(Evas_Object *entry);
static void _subject_create_entry(Evas_Object *parent, email_editfield_t *editfield);
static Evas_Object *_subject_create_add_attachment(Evas_Object *parent);
static void _subject_register_entry_callbacks(Evas_Object *obj, EmailComposerView *view);
static void _subject_entry_maxlength_reached_cb(void *data, Evas_Object *obj, void *event_info);
static void _subject_entry_focused_cb(void *data, Evas_Object *obj, void *event_info);
static void _subject_entry_activated_cb(void *data, Evas_Object *obj, void *event_info);
static void _subject_attach_files_clicked(void *data, Evas_Object *obj, void *event_info);
static void _entry_filter_accept_set(void *data, Evas_Object *entry, char **text);

static void _show_attach_panel(EmailComposerView *view);
static void _attach_panel_reply_cb(void *data, const char **path_array, int array_len);

static email_string_t EMAIL_COMPOSER_STRING_TPOP_MAXIMUM_NUMBER_OF_CHARACTERS_HPD_REACHED = { PACKAGE, "IDS_EMAIL_TPOP_MAXIMUM_NUMBER_OF_CHARACTERS_HPD_REACHED" };


/*
 * Definition for static functions
 */

static void _entry_filter_accept_set(void *data, Evas_Object *entry, char **text)
{
	debug_enter();

	if (strcmp(*text, "<br/>") == 0) {
		free(*text);
		*text = NULL;
	}

	debug_leave();
}

static void _subject_set_entry_guide_text(Evas_Object *entry)
{
	debug_enter();

	char guide_text[BUF_LEN_S] = { 0, };
	snprintf(guide_text, sizeof(guide_text), TEXT_STYLE_SUBJECT_GUIDE, FONT_SIZE_ENTRY, ENTRY_GUIDE_TEXT_FONT_COLOR , email_get_email_string("IDS_EMAIL_HEADER_SUBJECT"));
	elm_object_part_text_set(entry, "elm.guide", guide_text);

	debug_leave();
}

static void _subject_create_entry(Evas_Object *parent, email_editfield_t *editfield)
{
	email_profiling_begin(_subject_create_entry);
	debug_enter();
	email_common_util_editfield_create(parent, 0, editfield);
	elm_entry_markup_filter_append(editfield->entry, _entry_filter_accept_set, NULL);
	elm_entry_input_panel_layout_set(editfield->entry, ELM_INPUT_PANEL_LAYOUT_NORMAL);
	elm_entry_input_panel_return_key_type_set(editfield->entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_NEXT);
	_subject_set_entry_guide_text(editfield->entry);
	composer_util_set_entry_text_style(editfield->entry);

	Elm_Entry_Filter_Limit_Size limit_filter_data;
	limit_filter_data.max_char_count = EMAIL_LIMIT_COMPOSER_SUBJECT_LENGTH;
	limit_filter_data.max_byte_count = 0;
	elm_entry_markup_filter_append(editfield->entry, elm_entry_filter_limit_size, &limit_filter_data);

	evas_object_show(editfield->layout);

	debug_leave();
	email_profiling_end(_subject_create_entry);

}

static Evas_Object *_subject_create_add_attachment(Evas_Object *parent)
{
	debug_enter();

	Evas_Object *btn = elm_button_add(parent);
	elm_object_style_set(btn, "transparent");
	evas_object_show(btn);

	Evas_Object *icon = elm_layout_add(btn);
	elm_layout_file_set(icon, email_get_common_theme_path(), EMAIL_IMAGE_COMPOSER_ATTACH_ICON);
	elm_object_part_content_set(btn, "elm.swallow.content", icon);
	evas_object_show(icon);

	debug_leave();
	return btn;
}

static void _subject_register_entry_callbacks(Evas_Object *obj, EmailComposerView *view)
{
	debug_enter();

	evas_object_smart_callback_add(obj, "maxlength,reached", _subject_entry_maxlength_reached_cb, view);
	evas_object_smart_callback_add(obj, "focused", _subject_entry_focused_cb, view);
	evas_object_smart_callback_add(obj, "activated", _subject_entry_activated_cb, view);

	debug_leave();
}

static void _subject_entry_maxlength_reached_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	char buf[BUF_LEN_M] = { 0, };
	snprintf(buf, sizeof(buf), email_get_email_string(EMAIL_COMPOSER_STRING_TPOP_MAXIMUM_NUMBER_OF_CHARACTERS_HPD_REACHED.id), EMAIL_LIMIT_COMPOSER_SUBJECT_LENGTH);

	int noti_ret = notification_status_message_post(buf);
	debug_warning_if(noti_ret != NOTIFICATION_ERROR_NONE, "notification_status_message_post() failed! ret:(%d)", noti_ret);

	debug_leave();
}

static void _subject_entry_focused_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	if (composer_recipient_is_recipient_entry(view, view->selected_entry)) {
		if (!composer_recipient_commit_recipient_on_entry(view, view->selected_entry)) {
			return;
		}
	}
	composer_recipient_unfocus_entry(view, view->selected_entry);
	/* Force calculate the size on the evas.
	 * Case - 1. There're many contacts in mbe and the focus is on the mbe.
	 *        2. Click subject entry to set the focus on it.
	 *        3. MBE collapse its layout and the focus is being moved to subject entry.
	 *        4. The subject entry gets the focus and should be displayed within the screen.
	 *           But the entry is displayed out of the screen because the size wasn't changed.
	 */
	evas_smart_objects_calculate(evas_object_evas_get(view->composer_layout));

	if (view->bcc_added && !view->cc_recipients_cnt && !view->bcc_recipients_cnt) {
		composer_recipient_show_hide_bcc_field(view, EINA_FALSE);
	}

	DELETE_EVAS_OBJECT(view->context_popup);

	composer_webkit_blur_webkit_focus(view);
	if (view->richtext_toolbar) {
		composer_rich_text_disable_set(view, EINA_TRUE);
	}

	Evas_Object *current_entry = view->subject_entry.entry;
	if (view->selected_entry != current_entry) {
		composer_attachment_ui_contract_attachment_list(view);
		view->selected_entry = current_entry;
	}

	debug_leave();
}

static void _subject_entry_activated_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	composer_util_focus_set_focus(view, view->ewk_view);

	debug_leave();
}

static void _show_attach_panel(EmailComposerView *view)
{
	debug_enter();

	email_attach_panel_listener_t listener = { 0 };

	listener.cb_data = view;
	listener.reply_cb = _attach_panel_reply_cb;

	if (email_module_launch_attach_panel(view->base.module, &listener) == 0) {
		// To update inline image list to calculate total attachment size
		if (ewk_view_script_execute(view->ewk_view, EC_JS_GET_IMAGE_LIST,
				composer_util_get_image_list_cb, (void *)view) == EINA_FALSE) {
			debug_error("EC_JS_GET_IMAGE_LIST failed.");
		}
		evas_object_focus_set(view->ewk_view, EINA_FALSE);
		composer_webkit_blur_webkit_focus(view);
		if (view->richtext_toolbar) {
			composer_rich_text_disable_set(view, EINA_TRUE);
		}
	}

	debug_leave();
}

static void _attach_panel_reply_cb(void *data, const char **path_array, int array_len)
{
	debug_enter();
	EmailComposerView *view = data;

	composer_attachment_process_attachments_with_array(view,
			path_array, array_len, ATTACH_BY_USE_ORG_FILE, EINA_FALSE);

	debug_leave();
}

static void _subject_attach_files_clicked(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");
	EmailComposerView *view = (EmailComposerView *)data;
	retm_if(!view->is_load_finished, "Composer hasn't been launched completely!");

	if (!view->allow_click_events) {
		debug_log("Click was blocked.");
		return;
	}

	email_feedback_play_tap_sound();

	ecore_imf_input_panel_hide();
	int total_attachment_count = eina_list_count(view->attachment_item_list) + eina_list_count(view->attachment_inline_item_list);
	if (total_attachment_count >= MAX_ATTACHMENT_ITEM) {
		composer_attachment_launch_attachment_error_popup(COMPOSER_ERROR_ATTACHMENT_MAX_NUMBER_EXCEEDED, composer_util_popup_response_cb, view);
	} else {
		_show_attach_panel(view);
	}

	debug_leave();
}

/*
 * Definition for exported functions
 */

Evas_Object *composer_subject_create_layout(Evas_Object *parent)
{
	email_profiling_begin(composer_subject_create_layout);
	debug_enter();

	Evas_Object *layout = elm_layout_add(parent);
	elm_layout_file_set(layout, email_get_composer_theme_path(), "ec/subject/base");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(layout);

	debug_leave();
	email_profiling_end(composer_subject_create_layout);
	return layout;
}

void composer_subject_update_detail(void *data, Evas_Object *parent)
{
	email_profiling_begin(composer_subject_update_detail);
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;
	email_editfield_t editfield;
	editfield.layout = NULL;
	editfield.entry = NULL;
	_subject_create_entry(parent, &editfield);
	Evas_Object *btn = _subject_create_add_attachment(parent);

	_subject_register_entry_callbacks(editfield.entry, view);
	evas_object_smart_callback_add(btn, "clicked", _subject_attach_files_clicked, view);

	elm_object_part_content_set(parent, "ec.swallow.content", editfield.layout);
	elm_object_part_content_set(parent, "ec.swallow.add_attachment", btn);
	view->subject_entry = editfield;

	debug_leave();
	email_profiling_end(composer_subject_update_detail);
}

void composer_subject_update_guide_text(EmailComposerView *view)
{
	debug_enter();
	retm_if(!view, "Invalid parameter: view is NULL!");
	_subject_set_entry_guide_text(view->subject_entry.entry);
	debug_leave();
}
