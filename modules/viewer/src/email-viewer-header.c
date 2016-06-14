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

#include <utils_i18n.h>
#include <system_settings.h>

#include "email-engine.h"
#include "email-debug.h"
#include "email-viewer.h"
#include "email-viewer-util.h"
#include "email-viewer-more-menu.h"
#include "email-viewer-more-menu-callback.h"
#include "email-viewer-attachment.h"
#include "email-viewer-recipient.h"
#include "email-viewer-header.h"
#include "email-viewer-contents.h"
#include "email-viewer-eml.h"
#include "email-viewer-scroller.h"

i18n_udate_format_h viewer_date_formatter = NULL;
static i18n_udatepg_h viewer_pattern_generator = NULL;
#define CUSTOM_I18N_UDATE_IGNORE -2 /* Uses temporarily since there is no substistute of UDATE_IGNORE in base-utils */
#define VIEWER_HEADER_DIVIDER_HEIGHT 1

#define VIEWER_HEADER_PRIO_SENDER_TITLE_FMT		"<font=BreezeSansFallback><color=#FFB200>&#xF898;</color> %s</font>"
#define VIEWER_SUBJECT_ENTRY_TEXT_STYLE			"DEFAULT='font_size=40 color=#000000'"

#define VIEWER_ATTACH_INFO_FONT_STYLE		"Tizen:style=Regular"
#define VIEWER_ATTACH_INFO_FONT_SIZE		36
#define VIEWER_ATTACH_INFO_TEXT_PADDINGS	122

/*Expand button parameters*/
#define DEFAULT_BUTTON_H ELM_SCALE_SIZE(78)
#define EXPAND_BUTTON_H ELM_SCALE_SIZE(50)
#define EXPAND_BUTTON_TARGET_TEXT_SIZE 32
#define EXPAND_BUTTON_SCALE (1.0 * EXPAND_BUTTON_H / DEFAULT_BUTTON_H)
#define EXPAND_BUTTON_TEXT_SIZE ((int)(EXPAND_BUTTON_TARGET_TEXT_SIZE / EXPAND_BUTTON_SCALE + 0.5))

/*
 * Declaration for static functions
 */

/*header layouts*/
static void _header_create_main_layout(EmailViewerView *view);
static void _header_create_subject_entry(EmailViewerView *view);
static void _header_create_divider(EmailViewerView *view);
static void _header_create_attachment_preview(void *data);
static void _header_delete_attachment_preview(void *data);
static void _header_update_attachments_preview(void *data);
static char *_header_format_attachment_summary_text(int total_file_count, const char *first_file_name, guint64 total_size);
static void _header_pack_attachment_preview(void *data);

/*recipient details button logic*/
static void _header_delete_recipient_idler(void *data);
static void _header_show_hide_details_text_set(void *data, Eina_Bool is_opened);
static void _header_details_button_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static void _header_pack_recipient_ly(void *data, VIEWER_TO_CC_BCC_LY recipient);

/*favorite icon logic*/
static void _header_create_favorite_btn(EmailViewerView *view);
static void _header_favorite_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static void _header_popup_response_cb(void *data, Evas_Object *obj, void *event_info);

/*popup string definitions*/
static email_string_t EMAIL_VIEWER_STRING_NULL = { NULL, NULL };
static email_string_t EMAIL_VIEWER_STRING_OK = { PACKAGE, "IDS_EMAIL_BUTTON_OK" };
static email_string_t EMAIL_VIEWER_POP_ERROR = { PACKAGE, "IDS_EMAIL_HEADER_UNABLE_TO_PERFORM_ACTION_ABB" };
static email_string_t EMAIL_VIEWER_STR_FAILED_TO_SET_FAVOURITE = { NULL, N_("Failed to set favourite") };

/*
 * Definition for static functions
 */

static void _header_create_main_layout(EmailViewerView *view)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	/* append sender */
	view->header_ly = viewer_load_edj(view->base.content, email_get_viewer_theme_path(), "ev/header/base", EVAS_HINT_EXPAND, 0.0);
	evas_object_size_hint_align_set(view->header_ly, EVAS_HINT_FILL, 0.0);
	evas_object_show(view->header_ly);

	_header_create_subject_entry(view);

	if ((view->mailbox_type != EMAIL_MAILBOX_TYPE_OUTBOX) && (view->viewer_type == EMAIL_VIEWER)) {
		_header_create_favorite_btn(view);
	}

	if (view->to_recipients_cnt + view->cc_recipients_cnt + view->bcc_recipients_cnt > 0) {
		_header_show_hide_details_text_set(view, EINA_FALSE);
	}

	debug_leave();
}

static void _header_create_subject_entry(EmailViewerView *view)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	view->subject_entry = elm_entry_add(view->header_ly);
	elm_object_part_content_set(view->header_ly, "header.subject.swallow", view->subject_entry);
	elm_entry_text_style_user_push(view->subject_entry, VIEWER_SUBJECT_ENTRY_TEXT_STYLE);
	elm_entry_editable_set(view->subject_entry, EINA_FALSE);
	evas_object_freeze_events_set(view->subject_entry, EINA_TRUE);
}

static void _header_create_favorite_btn(EmailViewerView *view)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	if (!view->favourite_btn) {
		view->favourite_btn = elm_button_add(view->header_ly);
		elm_object_style_set(view->favourite_btn, "transparent");
		evas_object_smart_callback_add(view->favourite_btn, "clicked", _header_favorite_clicked_cb, view);
		elm_object_part_content_set(view->header_ly, "header.favourite.btn", view->favourite_btn);
	}

	debug_leave();
}

static void _header_create_divider(EmailViewerView *view)
{
	debug_enter();

	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	view->header_divider = evas_object_rectangle_add(view->header_ly);
	evas_object_size_hint_fill_set(view->header_divider, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_color_set(view->header_divider, AO005_ON_WHITE);
	evas_object_size_hint_min_set(view->header_divider, 0, VIEWER_HEADER_DIVIDER_HEIGHT);
	evas_object_show(view->header_divider);

	debug_leave();
}

static void _header_delete_recipient_idler(void *data)
{
	debug_enter();

	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	DELETE_IDLER_OBJECT(view->to_recipient_idler);
	DELETE_IDLER_OBJECT(view->cc_recipient_idler);
	DELETE_IDLER_OBJECT(view->bcc_recipient_idler);

	debug_leave();
}

static void _header_show_hide_details_text_set(void *data, Eina_Bool is_opened)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	if (view->details_button == NULL) {
		Evas_Object *details_btn = elm_button_add(view->header_ly);
		elm_object_style_set(details_btn, "default");
		elm_object_scale_set(details_btn, EXPAND_BUTTON_SCALE);
		evas_object_smart_callback_add(details_btn, "clicked", _header_details_button_clicked_cb, view);
		elm_object_part_content_set(view->header_ly, "header.details.btn", details_btn);
		evas_object_show(details_btn);
		view->details_button = details_btn;
	}

	char *btn_text = NULL;
	if (is_opened) {
		btn_text = email_get_email_string("IDS_EMAIL_BUTTON_HIDE_ABB3");
	} else {
		btn_text = email_get_email_string("IDS_EMAIL_BUTTON_SHOW_ABB");
	}

	char buff[EMAIL_BUFF_SIZE_HUG] = { 0 };
	snprintf(buff, EMAIL_BUFF_SIZE_HUG, "<font_size=%d>%s</font_size>", EXPAND_BUTTON_TEXT_SIZE, btn_text);
	elm_object_text_set(view->details_button, buff);
	debug_leave();
}

static void _header_pack_recipient_ly(void *data, VIEWER_TO_CC_BCC_LY recipient)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	switch (recipient) {
		case EMAIL_VIEWER_TO_LAYOUT:
			elm_box_pack_after(view->main_bx, view->to_ly, view->header_ly);
			break;
		case EMAIL_VIEWER_CC_LAYOUT:
			if (view->to_ly)
				elm_box_pack_after(view->main_bx, view->cc_ly, view->to_ly);
			else
				elm_box_pack_after(view->main_bx, view->cc_ly, view->header_ly);
			break;
		case EMAIL_VIEWER_BCC_LAYOUT:
			if (view->cc_ly)
				elm_box_pack_after(view->main_bx, view->bcc_ly, view->cc_ly);
			else if (view->to_ly)
				elm_box_pack_after(view->main_bx, view->bcc_ly, view->to_ly);
			else
				elm_box_pack_after(view->main_bx, view->bcc_ly, view->header_ly);
			break;
		default:
			break;
	}
}

static void _header_details_button_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;

	if (!(view->is_recipient_ly_shown)) {
		(view->is_recipient_ly_shown)++;

		email_address_info_list_t *addrs_info_list = NULL;
		addrs_info_list = viewer_create_address_info_list(view);
		if (addrs_info_list) {
			view->to_recipients_cnt = g_list_length(addrs_info_list->to);
			view->cc_recipients_cnt = g_list_length(addrs_info_list->cc);
			view->bcc_recipients_cnt = g_list_length(addrs_info_list->bcc);
			debug_log("bcc_recipients_cnt [%d]", view->bcc_recipients_cnt);
		}
		if (!view->to_ly && view->to_recipients_cnt) {
			view->to_ly = recipient_create_address(view, addrs_info_list, EMAIL_VIEWER_TO_LAYOUT);
			_header_pack_recipient_ly(view, EMAIL_VIEWER_TO_LAYOUT);
		}
		if (!view->cc_ly && view->cc_recipients_cnt) {
			view->cc_ly = recipient_create_address(view, addrs_info_list, EMAIL_VIEWER_CC_LAYOUT);
			_header_pack_recipient_ly(view, EMAIL_VIEWER_CC_LAYOUT);
		}
		if (!view->bcc_ly && view->bcc_recipients_cnt) {
			view->bcc_ly = recipient_create_address(view, addrs_info_list, EMAIL_VIEWER_BCC_LAYOUT);
			_header_pack_recipient_ly(view, EMAIL_VIEWER_BCC_LAYOUT);
		}

		viewer_free_address_info_list(addrs_info_list);
		addrs_info_list = NULL;

		_header_show_hide_details_text_set(view, EINA_TRUE);
	} else {
		(view->is_recipient_ly_shown)--;

		_header_delete_recipient_idler(view);

		_header_show_hide_details_text_set(view, EINA_FALSE);
		recipient_clear_multibuttonentry_data(view->to_mbe);
		recipient_clear_multibuttonentry_data(view->cc_mbe);
		recipient_clear_multibuttonentry_data(view->bcc_mbe);
		DELETE_EVAS_OBJECT(view->to_mbe);
		DELETE_EVAS_OBJECT(view->cc_mbe);
		DELETE_EVAS_OBJECT(view->bcc_mbe);
		if (view->to_ly) {
			elm_box_unpack(view->main_bx, view->to_ly);
			DELETE_EVAS_OBJECT(view->to_ly);
		}
		if (view->cc_ly) {
			elm_box_unpack(view->main_bx, view->cc_ly);
			DELETE_EVAS_OBJECT(view->cc_ly);
		}
		if (view->bcc_ly) {
			elm_box_unpack(view->main_bx, view->bcc_ly);
			DELETE_EVAS_OBJECT(view->bcc_ly);
		}
	}
	debug_leave();
}

static void _header_favorite_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;

	DELETE_EVAS_OBJECT(view->con_popup);

	debug_log("Before view->favorite: %d", view->favorite);
	switch (view->favorite) {
		case EMAIL_FLAG_NONE:
			view->favorite = EMAIL_FLAG_FLAGED;
			break;
		case EMAIL_FLAG_FLAGED:
			view->favorite = EMAIL_FLAG_NONE;
			break;
		default:
			debug_log("Never here");
	}
	debug_log("After view->favorite: %d", view->favorite);

	if (!email_engine_set_flags_field(view->account_id, &view->mail_id, 1, EMAIL_FLAGS_FLAGGED_FIELD, view->favorite, 1)) {
		if ((view->favorite == EMAIL_FLAG_NONE) || (view->favorite == EMAIL_FLAG_FLAGED)) {
			util_create_notify(view, EMAIL_VIEWER_POP_ERROR,
					EMAIL_VIEWER_STR_FAILED_TO_SET_FAVOURITE, 1,
					EMAIL_VIEWER_STRING_OK, _header_popup_response_cb, EMAIL_VIEWER_STRING_NULL, NULL, NULL);
		} else {
			debug_log("Never here");
		}
		return;
	}

	header_update_favorite_icon(view);

	debug_leave();
}

static void _header_popup_response_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;
	DELETE_EVAS_OBJECT(view->notify);
	debug_leave();
}

static void _header_pack_attachment_preview(void *data)
{
	debug_enter();

	EmailViewerView *view = (EmailViewerView *)data;
	Evas_Object *bottom_ly = view->header_divider;
	if (!view->is_recipient_ly_shown) {
		elm_box_pack_after(view->main_bx, view->attachment_ly, bottom_ly);
	} else {
		if (view->bcc_ly) {
			elm_box_pack_after(view->main_bx, view->attachment_ly, view->bcc_ly);
		} else if (view->cc_ly) {
			elm_box_pack_after(view->main_bx, view->attachment_ly, view->cc_ly);
		} else if (view->to_ly) {
			elm_box_pack_after(view->main_bx, view->attachment_ly, view->to_ly);
		} else {
			elm_box_pack_after(view->main_bx, view->attachment_ly, bottom_ly);
		}
	}
	debug_leave();
}

static void _header_attachment_clicked_edje_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;
	retm_if(view->attachment_genlist != NULL, "Attachment view already created");

	viewer_create_attachment_view(view);

	debug_leave();
}

static void _header_delete_attachment_preview(void *data)
{
	debug_enter();

	EmailViewerView *view = (EmailViewerView *)data;

	elm_box_unpack(view->main_bx, view->attachment_ly);
	evas_object_hide(view->attachment_ly);
	DELETE_EVAS_OBJECT(view->attachment_ly);
	debug_leave();
}

static void _header_create_attachment_preview(void *data)
{
	debug_enter();

	EmailViewerView *view = (EmailViewerView *)data;

	Evas_Object *attachment_ly = viewer_load_edj(view->base.content, email_get_viewer_theme_path(), "ev/attachment/base", EVAS_HINT_EXPAND, 0.0);
	evas_object_size_hint_align_set(attachment_ly, EVAS_HINT_FILL, 0.0);
	evas_object_show(attachment_ly);
	view->attachment_ly = attachment_ly;

	_header_pack_attachment_preview(view);
	header_update_attachment_summary_info(view);

	edje_object_signal_callback_add(_EDJ(attachment_ly), "attachment.clicked", "edje", _header_attachment_clicked_edje_cb, view);

	debug_leave();
}

static void _header_update_attachments_preview(void *data)
{
	debug_enter();

	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	_header_delete_attachment_preview(view);
	if (view->has_attachment) {
		_header_create_attachment_preview(view);
	}

	debug_leave();
}

/**
 * Definition for exported functions
 *
 */

void header_create_view(void *data)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;
	_header_create_main_layout(view);
	_header_create_divider(view);

	header_layout_pack(view);
	debug_leave();
}

void header_update_view(void *data)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;
	retm_if(view->eViewerErrorType != VIEWER_ERROR_NONE, "Some Error occurred");


	header_update_sender_name_and_prio_status(view);
	header_update_sender_address(view);
	header_update_subject_text(view);
	header_update_favorite_icon(view);
	header_update_date(view);

	if (!view->is_recipient_ly_shown) {
		_header_show_hide_details_text_set(view, EINA_FALSE);
	} else {
		_header_show_hide_details_text_set(view, EINA_TRUE);
	}

	_header_update_attachments_preview(view);

	debug_leave();
}

void header_layout_unpack(void *data)
{
	debug_enter();

	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	elm_box_unpack(view->main_bx, view->header_ly);

	if (view->has_attachment && view->attachment_ly) {
		elm_box_unpack(view->main_bx, view->attachment_ly);
	}

	if (view->is_recipient_ly_shown) {
		if (view->to_ly) {
			elm_box_unpack(view->main_bx, view->to_ly);
		}
		if (view->cc_ly) {
			elm_box_unpack(view->main_bx, view->cc_ly);
		}
		if (view->bcc_ly) {
			elm_box_unpack(view->main_bx, view->bcc_ly);
		}
	}

	elm_box_unpack(view->main_bx, view->header_divider);

	debug_leave();
}

void header_layout_pack(void *data)
{
	debug_enter();

	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	/* 1. sender */
	elm_box_pack_start(view->main_bx, view->header_ly);
	Evas_Object *bottom_ly = view->header_ly;
	/* 2-2. recipient */
	if (view->is_recipient_ly_shown) {
		if (view->to_ly) {
			elm_box_pack_before(view->main_bx, view->to_ly, view->header_ly);
			bottom_ly = view->to_ly;
		}

		if (view->cc_ly) {
			if (view->to_ly && view->cc_ly) {
				elm_box_pack_after(view->main_bx, view->cc_ly, view->to_ly);
			} else if (view->cc_ly) {
				elm_box_pack_after(view->main_bx, view->cc_ly, view->header_ly);
			}
			bottom_ly = view->cc_ly;
		}
		if (view->bcc_ly) {
			if (view->cc_ly) {
				elm_box_pack_after(view->main_bx, view->bcc_ly, view->cc_ly);
			}
			if (view->to_ly && view->cc_ly == NULL) {
				elm_box_pack_after(view->main_bx, view->bcc_ly, view->to_ly);
			}
			if (view->to_ly == NULL && view->cc_ly == NULL) {
				elm_box_pack_after(view->main_bx, view->bcc_ly, view->header_ly);
			}
			bottom_ly = view->bcc_ly;
		}
	}

	elm_box_pack_after(view->main_bx, view->header_divider, bottom_ly);
	bottom_ly = view->header_divider;

	if (view->has_attachment && view->attachment_ly) {
		elm_box_pack_after(view->main_bx, view->attachment_ly, bottom_ly);
	}

	debug_leave();
}

static char *_header_format_attachment_summary_text(int total_file_count, const char *first_file_name, guint64 total_size)
{
	debug_enter();

	char attach_sum[EMAIL_BUFF_SIZE_HUG] = {0,};

	/*format file size string*/
	char *_total_file_size = NULL;
	_total_file_size = email_get_file_size_string(total_size);

	if (total_file_count > 1) {
		char files_info_text[EMAIL_BUFF_SIZE_BIG] = {0,};
		const char *files_info_fmt = _("IDS_EMAIL_MBODY_P1SS_AND_P2SD_MORE");

		snprintf(files_info_text, sizeof(files_info_text), files_info_fmt, first_file_name, (total_file_count - 1));
		snprintf(attach_sum, sizeof(attach_sum), "%s (%s)", files_info_text, _total_file_size);
	} else {
		snprintf(attach_sum, sizeof(attach_sum), "%s (%s)", first_file_name, _total_file_size);
	}

	g_free(_total_file_size);

	debug_leave();
	return strdup(attach_sum);
}

void header_update_attachment_summary_info(EmailViewerView *view)
{
	debug_enter();

	/*set file name*/
	char first_attach_file_name[EMAIL_BUFF_SIZE_MID] = {0,};

	int i;
	for (i = 0; i < view->attachment_count; i++) {
		if (!view->attachment_info[i].inline_content_status) {
			snprintf(first_attach_file_name, sizeof(first_attach_file_name), "%s", view->attachment_info[i].attachment_name);
			break;
		}
	}

	Evas_Object *text_obj = evas_object_text_add(evas_object_evas_get(view->attachment_ly));
	evas_object_text_font_set(text_obj, VIEWER_ATTACH_INFO_FONT_STYLE, VIEWER_ATTACH_INFO_FONT_SIZE);
	evas_object_text_style_set(text_obj, EVAS_TEXT_STYLE_PLAIN);

	char *non_ellipsised_text = _header_format_attachment_summary_text(view->attachment_count, "", view->total_att_size);
	evas_object_text_text_set(text_obj, non_ellipsised_text);
	free(non_ellipsised_text);

	int layout_width = 0;
	int win_h = 0, win_w = 0;
	elm_win_screen_size_get(view->base.module->win, NULL, NULL, &win_w, &win_h);
	if (view->base.orientation == APP_DEVICE_ORIENTATION_0 || view->base.orientation == APP_DEVICE_ORIENTATION_180) {
		layout_width = win_w;
	} else {
		layout_width = win_h;
	}

	int text_avail_width = layout_width - ELM_SCALE_SIZE(VIEWER_ATTACH_INFO_TEXT_PADDINGS);
	int max_filename_width = text_avail_width - evas_object_text_horiz_width_without_ellipsis_get(text_obj);

	email_set_ellipsised_text(text_obj, first_attach_file_name, max_filename_width);
	char *formatted_attach_text = _header_format_attachment_summary_text(view->attachment_count, evas_object_text_text_get(text_obj), view->total_att_size);

	elm_object_part_text_set(view->attachment_ly, "attachment.info.text", formatted_attach_text);
	evas_object_smart_calculate(view->attachment_ly);

	free(formatted_attach_text);
	evas_object_del(text_obj);

	debug_leave();
}

void header_update_sender_name_and_prio_status(EmailViewerView *view)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	char *sender_name = NULL;

	/* sender name */
	if (!STR_VALID(view->sender_display_name) && !STR_VALID(view->sender_address)) {
		/* If no info about sender name and address show warning text */
		sender_name = strdup(_("IDS_EMAIL_MBODY_NO_EMAIL_ADDRESS_OR_NAME"));
	} else if (STR_VALID(view->sender_display_name)) {
		/* 1. Show display name obtained form email server */
		sender_name = elm_entry_utf8_to_markup(view->sender_display_name);
	} else if (STR_VALID(view->sender_address)) {
		/* 2. If there is no sender name show email address */
		sender_name = strdup(view->sender_address);
	}

	/* priority sender image */
	if (recipient_is_priority_email_address(view->sender_address)) {
		char prio_sender_name[EMAIL_BUFF_SIZE_HUG] = { 0 };
		snprintf(prio_sender_name, sizeof(prio_sender_name), VIEWER_HEADER_PRIO_SENDER_TITLE_FMT, sender_name);
		elm_object_item_part_text_set(view->base.navi_item, "elm.text.title", prio_sender_name);
	} else {
		elm_object_item_part_text_set(view->base.navi_item, "elm.text.title", sender_name);
	}

	FREE(sender_name);
	debug_leave();
}

void header_update_sender_address(EmailViewerView *view)
{
	/* sender email */
	if (STR_VALID(view->sender_address)) {
		elm_object_item_part_text_set(view->base.navi_item, "subtitle", view->sender_address);
	} else {
		elm_object_item_part_text_set(view->base.navi_item, "subtitle", NULL);
	}
}

void header_update_date(EmailViewerView *view)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	viewer_close_pattern_generator();
	viewer_open_pattern_generator();

	char *formatted_date = NULL;
	char date_time[MAX_STR_LEN] = { 0, };

	formatted_date = email_get_date_text_with_formatter(viewer_date_formatter, &(view->mktime));
	if (formatted_date != NULL) {
		snprintf(date_time, MAX_STR_LEN, "%s", formatted_date);
		FREE(formatted_date);
	}

	elm_object_part_text_set(view->header_ly, "header.date.text", date_time);

	debug_leave();
}

void header_update_favorite_icon(EmailViewerView *view)
{
	debug_enter();
	retm_if(!view, "Invalid parameter: view[NULL]");
	retm_if(!view->favourite_btn, "Invalid parameter: favourite_btn[NULL]");

	Evas_Object *start_icon = elm_layout_add(view->favourite_btn);
	debug_log("favorite: %d", view->favorite);
	switch (view->favorite) {
		case EMAIL_FLAG_NONE:
			elm_layout_file_set(start_icon, email_get_viewer_theme_path(), "elm/layout/flag.none.icon/default");
			break;
		case EMAIL_FLAG_FLAGED:
			elm_layout_file_set(start_icon, email_get_viewer_theme_path(), "elm/layout/flag.star.icon/default");
			break;
	}
	elm_object_content_set(view->favourite_btn, start_icon);
}

void header_update_subject_text(EmailViewerView *view)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	char *subject_txt = elm_entry_utf8_to_markup(view->subject);
	if (subject_txt && strcmp(subject_txt, "") != 0) {
		elm_entry_entry_set(view->subject_entry, subject_txt);
	} else {
		elm_entry_entry_set(view->subject_entry, _("IDS_EMAIL_SBODY_NO_SUBJECT_M_NOUN"));
	}

	free(subject_txt);
	debug_leave();
}

int viewer_open_pattern_generator(void)
{
	debug_enter();
	if (viewer_pattern_generator && viewer_date_formatter) {
		debug_log("already created");
		return 0;
	}
	viewer_close_pattern_generator();

	/* ICU API to set default locale */
	i18n_error_code_e status = I18N_ERROR_NONE;
	status = i18n_ulocale_set_default(getenv("LC_TIME"));
	retvm_if(status != I18N_ERROR_NONE, -1, "i18n_ulocale_set_default() failed! ret:[%d]", status);

	/* API to get default locale */
	status = I18N_ERROR_NONE;
	const char *locale = NULL;
	status = i18n_ulocale_get_default(&locale);
	debug_secure("locale: %s", locale); /* ex: en_GB.UTF-8 */
	retvm_if(status != I18N_ERROR_NONE, -1, "i18n_ulocale_get_default() failed! ret:[%d]", status);

	char *viewer_locale = NULL;
	viewer_locale = email_parse_get_title_from_filename(locale);
	debug_secure("real locale: %s", viewer_locale);

	bool is_24_hr_format;
	int retcode = system_settings_get_value_bool(SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR, &is_24_hr_format);
	if (retcode != SYSTEM_SETTINGS_ERROR_NONE) {
		debug_log("system_settings_get_value_bool failed");
	}

	status = i18n_udatepg_create(viewer_locale, &viewer_pattern_generator);
	if (!viewer_pattern_generator || status != I18N_ERROR_NONE) {
		debug_secure("i18n_udatepg_create() failed: %d", status);
		return -1;
	}

	char *skeleton = NULL;
	if (is_24_hr_format == false) {
		skeleton = "hh:mm a, y MMM d";
	} else {
		skeleton = "HH:mm, y MMM d";
	}
	i18n_uchar customSkeleton[64] = { 0, };
	int skeletonLength = strlen(skeleton);
	int len;

	i18n_ustring_copy_ua_n(customSkeleton, skeleton, skeletonLength);
	len = i18n_ustring_get_length(customSkeleton);
	i18n_uchar bestPattern[64] = { 0, };
	int32_t bestPatternCapacity;
	int32_t bestPatternLength;
	status = I18N_ERROR_NONE;
	bestPatternCapacity = (int32_t) (sizeof(bestPattern) / sizeof(bestPattern[0]));
	status = i18n_udatepg_get_best_pattern(viewer_pattern_generator, customSkeleton, len, bestPattern, bestPatternCapacity, &bestPatternLength);
	debug_log("bestPatternLength: %d", bestPatternLength);
	retvm_if(status != I18N_ERROR_NONE, -1, "i18n_udatepg_get_best_pattern() failed! ret:[%d]", status);

	status = I18N_ERROR_NONE;
	status = i18n_udate_create(CUSTOM_I18N_UDATE_IGNORE, CUSTOM_I18N_UDATE_IGNORE, viewer_locale, NULL, -1, bestPattern, -1, &viewer_date_formatter);
	g_free(viewer_locale);
	if (status > I18N_ERROR_NONE) { /* from the definition of U_FAILURE */
		debug_critical("i18n_udate_create() failed: %d", status);
		return -1;
	}

	return 0;
}

int viewer_close_pattern_generator(void)
{
	debug_enter();

	if (viewer_pattern_generator) {
		i18n_udatepg_destroy(viewer_pattern_generator);
		viewer_pattern_generator = NULL;
	}

	if (viewer_date_formatter) {
		i18n_udate_destroy(viewer_date_formatter);
		viewer_date_formatter = NULL;
	}

	return 0;
}
