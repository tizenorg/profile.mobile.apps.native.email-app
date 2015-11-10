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

#define DEFAULT_BUTTON_H ELM_SCALE_SIZE(78)
#define EXPAND_BUTTON_H ELM_SCALE_SIZE(44)
#define EXPAND_BUTTON_TARGET_TEXT_SIZE 26
#define EXPAND_BUTTON_SCALE (1.0 * EXPAND_BUTTON_H / DEFAULT_BUTTON_H)
#define EXPAND_BUTTON_TEXT_SIZE ((int)(EXPAND_BUTTON_TARGET_TEXT_SIZE / EXPAND_BUTTON_SCALE + 0.5))

static Elm_Genlist_Item_Class reply_popup_itc;

/*
 * Declaration for static functions
 */

/*header layouts*/
static void _header_create_subject(EmailViewerUGD *ug_data);
static void _header_update_subject(EmailViewerUGD *ug_data, email_mail_data_t *mail_info);
static void _header_create_sender(EmailViewerUGD *ug_data);
static void _header_create_divider(EmailViewerUGD *ug_data);
static void _header_update_sender(EmailViewerUGD *ug_data);
static void _header_create_attachment_preview(void *data);
static void _header_update_attachments_preview(void *data);
static void _header_pack_attachment_preview(void *data);
static void _header_back_key_cb_clicked(void *data, Evas_Object *obj, void *event_info);

/*recipient button logic*/
static void _header_delete_recipient_idler(void *data);
static void _header_show_hide_details_text_set(void *data, Eina_Bool is_opened);
static void _header_expand_button_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static void _header_pack_recipient_ly(void *data, VIEWER_TO_CC_BCC_LY recipient);

/*reply, favrourite and priority icon logic*/
static void _header_popup_response_cb(void *data, Evas_Object *obj, void *event_info);
static void _header_favorite_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _header_set_reply_icon(EmailViewerUGD *ug_data, const char *part);
static void _header_reply_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source);
static char *_header_reply_popup_text_get(void *data, Evas_Object *obj, const char *part);


/*popup string definitions*/
static email_viewer_string_t EMAIL_VIEWER_STRING_NULL = { NULL, NULL };
static email_viewer_string_t EMAIL_VIEWER_STRING_OK = { PACKAGE, "IDS_EMAIL_BUTTON_OK" };
static email_viewer_string_t EMAIL_VIEWER_POP_ERROR = { PACKAGE, "IDS_EMAIL_HEADER_UNABLE_TO_PERFORM_ACTION_ABB" };
static email_viewer_string_t EMAIL_VIEWER_BUTTON_CANCEL = { PACKAGE, "IDS_EMAIL_BUTTON_CANCEL" };
static email_viewer_string_t EMAIL_VIEWER_STR_REPLY_OPTIONS = { PACKAGE, "IDS_EMAIL_HEADER_SELECT_ACTION_ABB" };
static email_viewer_string_t EMAIL_VIEWER_STR_FAILED_TO_SET_FAVOURITE = { NULL, N_("Failed to set favourite") };

/*
 * Definition for static functions
 */

static void _header_back_key_cb_clicked(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	if (!data) {
		debug_error("data is NULL");
		return;
	}

	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;
	viewer_back_key_press_handler(ug_data);
	debug_leave();
}

static void _header_update_subject(EmailViewerUGD *ug_data, email_mail_data_t *mail_info)
{
	debug_enter();
	retm_if(ug_data == NULL, "Invalid parameter: ug_data[NULL]");
	retm_if(!mail_info, "invalid data");

	char *subject_txt = elm_entry_utf8_to_markup(ug_data->subject);
	if (!g_strcmp0(subject_txt, "")) {
		g_free(subject_txt);
		subject_txt = g_strdup(email_get_email_string("IDS_EMAIL_SBODY_NO_SUBJECT_M_NOUN"));
	}
	char *appended_text = NULL;
	if (mail_info->flags_answered_field || mail_info->flags_forwarded_field) {
		const char *icon = mail_info->flags_answered_field ?
			EMAIL_IMAGE_ICON_LIST_REPLY : EMAIL_IMAGE_ICON_LIST_FORWARD;
		appended_text = g_strdup_printf(HEAD_SUBJ_TEXT_WITH_ICON_STYLE, HEAD_SUBJ_ICON_WIDTH, HEAD_SUBJ_ICON_HEIGHT,
				email_get_img_dir(), icon,
				HEAD_SUBJ_FONT_SIZE, VIEWER_SUBJECT_FONT_COLOR, subject_txt);
		elm_object_text_set(ug_data->en_subject, appended_text);

	} else {
		appended_text = g_strdup_printf(HEAD_SUBJ_TEXT_STYLE,
				HEAD_SUBJ_FONT_SIZE, VIEWER_SUBJECT_FONT_COLOR, subject_txt);
		elm_object_text_set(ug_data->en_subject, appended_text);
	}

	evas_object_show(ug_data->subject_ly);
	g_free(appended_text);
	g_free(subject_txt);

	debug_leave();
}

static void _header_update_sender(EmailViewerUGD *ug_data)
{
	debug_enter();
	retm_if(ug_data == NULL, "Invalid parameter: ug_data[NULL]");

	header_set_sender_name(ug_data);

	/* priority sender image */
	if (recipient_is_priority_email_address(ug_data->sender_address)) {
		Evas_Object *priority_image = elm_layout_add(ug_data->sender_ly);
		elm_layout_file_set(priority_image, email_get_common_theme_path(), EMAIL_IMAGE_ICON_PRIO_SENDER);
		elm_object_part_content_set(ug_data->sender_ly, "sender.priority.icon", priority_image);
		elm_layout_signal_emit(ug_data->sender_ly, "set.priority.sender", "elm");
		evas_object_show(priority_image);
		ug_data->priority_sender_image = priority_image;
	} else {
		elm_layout_signal_emit(ug_data->sender_ly, "remove.priority.sender", "elm");
	}

	debug_leave();
}

static void _header_create_subject(EmailViewerUGD *ug_data)
{
	debug_enter();
	retm_if(ug_data == NULL, "Invalid parameter: ug_data[NULL]");

	if (ug_data->subject_ly) {
		elm_object_part_content_unset(ug_data->base.content, "ev.swallow.subject");
		DELETE_EVAS_OBJECT(ug_data->subject_ly);
	}
	/*create subject layout*/
	ug_data->subject_ly = viewer_load_edj(ug_data->base.content, email_get_viewer_theme_path(), "ev/subject/base", EVAS_HINT_EXPAND, 0.0);
	evas_object_size_hint_align_set(ug_data->subject_ly, EVAS_HINT_FILL, EVAS_HINT_FILL);

	ug_data->en_subject = elm_label_add(ug_data->subject_ly);
	elm_label_line_wrap_set(ug_data->en_subject, ELM_WRAP_MIXED);
	elm_label_ellipsis_set(ug_data->en_subject, EINA_TRUE);
	elm_object_style_set(ug_data->en_subject, "subject_text");


	evas_object_show(ug_data->en_subject);
	elm_object_part_content_set(ug_data->subject_ly, "ev.swallow.subject_label", ug_data->en_subject);

	Evas_Object *back_btn = elm_button_add(ug_data->subject_ly);
	elm_object_style_set(back_btn, "naviframe/back_btn/default");
	evas_object_smart_callback_add(back_btn, "clicked", _header_back_key_cb_clicked, ug_data);

	elm_object_part_content_set(ug_data->subject_ly, "ev.swallow.back_button", back_btn);
	elm_object_part_content_set(ug_data->base.content, "ev.swallow.subject", ug_data->subject_ly);
	debug_leave();
}

static void _header_set_reply_icon(EmailViewerUGD *ug_data, const char *part)
{
	debug_enter();
	retm_if(ug_data == NULL, "Invalid parameter: ug_data[NULL]");

	Evas_Object *ly = elm_layout_add(ug_data->sender_ly);
	elm_layout_file_set(ly, email_get_viewer_theme_path(), "elm/layout/reply.icon/default");
	elm_object_part_content_set(ug_data->sender_ly, part, ly);
	edje_object_signal_callback_add(_EDJ(ly), "clicked", "edje", _header_reply_clicked_cb, ug_data);

	debug_leave();
}

static void _header_create_sender(EmailViewerUGD *ug_data)
{
	debug_enter();
	retm_if(ug_data == NULL, "Invalid parameter: ug_data[NULL]");

	/* append sender */
	ug_data->sender_ly = viewer_load_edj(ug_data->base.content, email_get_viewer_theme_path(), "ev/sender/base", EVAS_HINT_EXPAND, 0.0);
	evas_object_size_hint_align_set(ug_data->sender_ly, EVAS_HINT_FILL, 0.0);
	evas_object_show(ug_data->sender_ly);

	if ((ug_data->mailbox_type != EMAIL_MAILBOX_TYPE_OUTBOX) && (ug_data->viewer_type == EMAIL_VIEWER)) {
		header_set_favorite_icon(ug_data);
		bool reply_allowed = true;
		if (reply_allowed) {
			_header_set_reply_icon(ug_data, "sender.rep.event");
		}
	}
	if (ug_data->viewer_type == EML_VIEWER) {
		edje_object_signal_emit(_EDJ(ug_data->sender_ly), "set.eml.viewer", "");
		_header_set_reply_icon(ug_data, "sender.fav.event");
	}

	if (ug_data->to_recipients_cnt + ug_data->cc_recipients_cnt + ug_data->bcc_recipients_cnt > 0) {
		_header_show_hide_details_text_set(ug_data, EINA_FALSE);
	}
	debug_leave();
}

static void _header_create_divider(EmailViewerUGD *ug_data)
{
	debug_enter();

	retm_if(ug_data == NULL, "Invalid parameter: ug_data[NULL]");

	ug_data->header_divider = evas_object_rectangle_add(ug_data->sender_ly);
	evas_object_size_hint_fill_set(ug_data->header_divider, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_color_set(ug_data->header_divider, AO005_ON_WHITE);
	evas_object_size_hint_min_set(ug_data->header_divider, 0, VIEWER_HEADER_DIVIDER_HEIGHT);
	evas_object_show(ug_data->header_divider);

	debug_leave();
}

static void _header_delete_recipient_idler(void *data)
{
	debug_enter();

	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	DELETE_IDLER_OBJECT(ug_data->to_recipient_idler);
	DELETE_IDLER_OBJECT(ug_data->cc_recipient_idler);
	DELETE_IDLER_OBJECT(ug_data->bcc_recipient_idler);

	debug_leave();
}

static void _header_show_hide_details_text_set(void *data, Eina_Bool is_opened)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	if (ug_data->details_button == NULL) {
		Evas_Object *details_btn = elm_button_add(ug_data->sender_ly);
		elm_object_style_set(details_btn, "default");
		elm_object_scale_set(details_btn, EXPAND_BUTTON_SCALE);
		evas_object_smart_callback_add(details_btn, "clicked", _header_expand_button_clicked_cb, ug_data);
		elm_object_part_content_set(ug_data->sender_ly, "sender.expand.btn", details_btn);
		evas_object_show(details_btn);
		ug_data->details_button = details_btn;
	}

	char *btn_text = NULL;
	if (is_opened) {
		btn_text = email_get_email_string("IDS_EMAIL_BUTTON_HIDE_ABB3");
	} else {
		btn_text = email_get_email_string("IDS_EMAIL_BUTTON_SHOW_ABB");
	}

	char buff[BUF_LEN_M] = { 0 };
	snprintf(buff, BUF_LEN_M, "<font_size=%d>%s</font_size>", EXPAND_BUTTON_TEXT_SIZE, btn_text);
	elm_object_text_set(ug_data->details_button, buff);
	debug_leave();
}

static void _header_pack_recipient_ly(void *data, VIEWER_TO_CC_BCC_LY recipient)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	switch (recipient) {
		case EMAIL_VIEWER_TO_LAYOUT:
			elm_box_pack_after(ug_data->main_bx, ug_data->to_ly, ug_data->sender_ly);
			break;
		case EMAIL_VIEWER_CC_LAYOUT:
			if (ug_data->to_ly)
				elm_box_pack_after(ug_data->main_bx, ug_data->cc_ly, ug_data->to_ly);
			else
				elm_box_pack_after(ug_data->main_bx, ug_data->cc_ly, ug_data->sender_ly);
			break;
		case EMAIL_VIEWER_BCC_LAYOUT:
			if (ug_data->cc_ly)
				elm_box_pack_after(ug_data->main_bx, ug_data->bcc_ly, ug_data->cc_ly);
			else if (ug_data->to_ly)
				elm_box_pack_after(ug_data->main_bx, ug_data->bcc_ly, ug_data->to_ly);
			else
				elm_box_pack_after(ug_data->main_bx, ug_data->bcc_ly, ug_data->sender_ly);
			break;
		default:
			break;
	}
}

static void _header_expand_button_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	if (!(ug_data->is_recipient_ly_shown)) {
		(ug_data->is_recipient_ly_shown)++;

		email_address_info_list_t *addrs_info_list = NULL;
		addrs_info_list = viewer_create_address_info_list(ug_data);
		if (addrs_info_list) {
			ug_data->to_recipients_cnt = g_list_length(addrs_info_list->to);
			ug_data->cc_recipients_cnt = g_list_length(addrs_info_list->cc);
			ug_data->bcc_recipients_cnt = g_list_length(addrs_info_list->bcc);
			debug_log("bcc_recipients_cnt [%d]", ug_data->bcc_recipients_cnt);
		}
		if (!ug_data->to_ly && ug_data->to_recipients_cnt) {
			ug_data->to_ly = recipient_create_address(ug_data, addrs_info_list, EMAIL_VIEWER_TO_LAYOUT);
			_header_pack_recipient_ly(ug_data, EMAIL_VIEWER_TO_LAYOUT);
		}
		if (!ug_data->cc_ly && ug_data->cc_recipients_cnt) {
			ug_data->cc_ly = recipient_create_address(ug_data, addrs_info_list, EMAIL_VIEWER_CC_LAYOUT);
			_header_pack_recipient_ly(ug_data, EMAIL_VIEWER_CC_LAYOUT);
		}
		if (!ug_data->bcc_ly && ug_data->bcc_recipients_cnt) {
			ug_data->bcc_ly = recipient_create_address(ug_data, addrs_info_list, EMAIL_VIEWER_BCC_LAYOUT);
			_header_pack_recipient_ly(ug_data, EMAIL_VIEWER_BCC_LAYOUT);
		}

		viewer_free_address_info_list(addrs_info_list);
		addrs_info_list = NULL;

		_header_show_hide_details_text_set(ug_data, EINA_TRUE);
	} else {
		(ug_data->is_recipient_ly_shown)--;

		_header_delete_recipient_idler(ug_data);

		_header_show_hide_details_text_set(ug_data, EINA_FALSE);
		recipient_clear_multibuttonentry_data(ug_data->to_mbe);
		recipient_clear_multibuttonentry_data(ug_data->cc_mbe);
		recipient_clear_multibuttonentry_data(ug_data->bcc_mbe);
		DELETE_EVAS_OBJECT(ug_data->to_mbe);
		DELETE_EVAS_OBJECT(ug_data->cc_mbe);
		DELETE_EVAS_OBJECT(ug_data->bcc_mbe);
		if (ug_data->to_ly) {
			elm_box_unpack(ug_data->main_bx, ug_data->to_ly);
			DELETE_EVAS_OBJECT(ug_data->to_ly);
		}
		if (ug_data->cc_ly) {
			elm_box_unpack(ug_data->main_bx, ug_data->cc_ly);
			DELETE_EVAS_OBJECT(ug_data->cc_ly);
		}
		if (ug_data->bcc_ly) {
			elm_box_unpack(ug_data->main_bx, ug_data->bcc_ly);
			DELETE_EVAS_OBJECT(ug_data->bcc_ly);
		}
	}
	debug_leave();
}

static void _header_popup_response_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;
	DELETE_EVAS_OBJECT(ug_data->notify);
	debug_leave();
}

static void _header_reply_clicked_cb(void *data, Evas_Object *obj,
		const char *emission, const char *source)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	DELETE_EVAS_OBJECT(ug_data->con_popup);

	ug_data->notify = util_create_notify_with_list(ug_data, EMAIL_VIEWER_STR_REPLY_OPTIONS, EMAIL_VIEWER_STRING_NULL, 1, EMAIL_VIEWER_BUTTON_CANCEL,
			_header_popup_response_cb, EMAIL_VIEWER_STRING_NULL, NULL, NULL);

	reply_popup_itc.item_style = "type1";
	reply_popup_itc.func.text_get = _header_reply_popup_text_get;
	reply_popup_itc.func.content_get = NULL;
	reply_popup_itc.func.state_get = NULL;
	reply_popup_itc.func.del = NULL;

	Evas_Object *reply_popup_genlist = util_notify_genlist_add(ug_data->notify);
	evas_object_data_set(reply_popup_genlist, VIEWER_EVAS_DATA_NAME, ug_data);

	bool reply_allowed = (ug_data->mailbox_type != EMAIL_MAILBOX_TYPE_OUTBOX);
	if (reply_allowed) {
		elm_genlist_item_append(reply_popup_genlist, &reply_popup_itc,
				"IDS_EMAIL_OPT_REPLY", NULL, ELM_GENLIST_ITEM_NONE, _reply_cb,
				(void *)ug_data);
	}

	if (ug_data->to_recipients_cnt + ug_data->cc_recipients_cnt + ug_data->bcc_recipients_cnt > 1) {
		bool reply_all_allowed = (ug_data->mailbox_type != EMAIL_MAILBOX_TYPE_OUTBOX);
		if (reply_all_allowed) {
			elm_genlist_item_append(reply_popup_genlist, &reply_popup_itc,
					"IDS_EMAIL_OPT_REPLY_ALL", NULL, ELM_GENLIST_ITEM_NONE,
					_reply_all_cb, (void *)ug_data);
		}
	}

	bool fwd_allowed = (ug_data->mailbox_type != EMAIL_MAILBOX_TYPE_OUTBOX);
	if (fwd_allowed) {
		elm_genlist_item_append(reply_popup_genlist, &reply_popup_itc,
				"IDS_EMAIL_OPT_FORWARD", NULL, ELM_GENLIST_ITEM_NONE, _forward_cb,
				(void *)ug_data);
	}

	elm_object_content_set(ug_data->notify, reply_popup_genlist);
	evas_object_show(ug_data->notify);
	debug_leave();
}

static void _header_favorite_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;
	int err = EMAIL_ERROR_NONE;

	DELETE_EVAS_OBJECT(ug_data->con_popup);

	debug_log("Before ug_data->favorite: %d", ug_data->favorite);
	switch (ug_data->favorite) {
		case EMAIL_FLAG_NONE:
			ug_data->favorite = EMAIL_FLAG_FLAGED;
			break;
		case EMAIL_FLAG_FLAGED:
			ug_data->favorite = EMAIL_FLAG_NONE;
			break;
		default:
			debug_log("Never here");
	}
	debug_log("After ug_data->favorite: %d", ug_data->favorite);

	err = email_set_flags_field(ug_data->account_id, &ug_data->mail_id, 1, EMAIL_FLAGS_FLAGGED_FIELD, ug_data->favorite, 1);

	if (err != EMAIL_ERROR_NONE) {
		if ((ug_data->favorite == EMAIL_FLAG_NONE) || (ug_data->favorite == EMAIL_FLAG_FLAGED)) {
			util_create_notify(ug_data, EMAIL_VIEWER_POP_ERROR,
					EMAIL_VIEWER_STR_FAILED_TO_SET_FAVOURITE, 1,
					EMAIL_VIEWER_STRING_OK, _header_popup_response_cb, EMAIL_VIEWER_STRING_NULL, NULL, NULL);
		} else {
			debug_log("Never here");
		}
		return;
	}

	header_update_favorite_icon(ug_data);

	debug_leave();
}

static char *_header_reply_popup_text_get(void *data, Evas_Object *obj, const char *part)
{
	debug_enter();
	retvm_if(obj == NULL, NULL, "Invalid parameter: obj[NULL]");

	EmailViewerUGD *ug_data = (EmailViewerUGD *)evas_object_data_get(obj, VIEWER_EVAS_DATA_NAME);
	retvm_if(ug_data == NULL, NULL, "Invalid parameter: ug_data[NULL]");

	if (!strcmp(part, "elm.text")) {
		const char *text = data ? _(data) : NULL;
		return text && *text ? strdup(text) : NULL;
	}
	debug_leave();
	return NULL;
}

static void _header_pack_attachment_preview(void *data)
{
	debug_enter();

	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;
	Evas_Object *bottom_ly = ug_data->header_divider;
	if (!ug_data->is_recipient_ly_shown) {
		elm_box_pack_after(ug_data->main_bx, ug_data->attachment_ly, bottom_ly);
	} else {
		if (ug_data->bcc_ly) {
			elm_box_pack_after(ug_data->main_bx, ug_data->attachment_ly, ug_data->bcc_ly);
		} else if (ug_data->cc_ly) {
			elm_box_pack_after(ug_data->main_bx, ug_data->attachment_ly, ug_data->cc_ly);
		} else if (ug_data->to_ly) {
			elm_box_pack_after(ug_data->main_bx, ug_data->attachment_ly, ug_data->to_ly);
		} else {
			elm_box_pack_after(ug_data->main_bx, ug_data->attachment_ly, bottom_ly);
		}
	}
	debug_leave();
}

static void _header_attachment_clicked_edje_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;
	retm_if(ug_data->attachment_genlist != NULL, "Attachment view already created");

	viewer_create_attachment_view(ug_data);

	debug_leave();
}

void header_update_response_icon(EmailViewerUGD *ug_data, email_mail_data_t *mail_info)
{
	debug_enter();
	retm_if(ug_data == NULL, "Invalid parameter: ug_data[NULL]");
	retm_if(mail_info == NULL, "Invalid parameter: mail_info[NULL]");
	retm_if(ug_data->viewer_type != EMAIL_VIEWER, "Not needed in this view");

	if (mail_info->flags_answered_field || mail_info->flags_forwarded_field) {
		_header_update_subject(ug_data, mail_info);
	}

	debug_leave();
}

void header_update_attachment_summary_info(EmailViewerUGD *ug_data)
{
	debug_enter();

	/*set file count*/
	char file_count[BUF_LEN_S];
	if (ug_data->normal_att_count == 1) {
		snprintf(file_count, sizeof(file_count), "%s", email_get_email_string("IDS_EMAIL_BODY_1_FILE_ABB"));
	} else {
		snprintf(file_count, sizeof(file_count), email_get_email_string("IDS_EMAIL_BODY_PD_FILES_ABB"), ug_data->normal_att_count);
	}

	/*set file size*/
	char *_total_file_size = NULL;
	_total_file_size = email_get_file_size_string(ug_data->total_att_size);

	char attachment_info[MAX_STR_LEN];
	snprintf(attachment_info, sizeof(attachment_info), "%s (%s)", file_count, _total_file_size);

	elm_object_part_text_set(ug_data->attachment_ly, "attachment.info.text", attachment_info);

	g_free(_total_file_size);

	debug_leave();
}

void _header_delete_attachment_preview(void *data)
{
	debug_enter();

	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	elm_box_unpack(ug_data->main_bx, ug_data->attachment_ly);
	evas_object_hide(ug_data->attachment_ly);
	DELETE_EVAS_OBJECT(ug_data->attachment_ly);
	debug_leave();
}

static void _header_create_attachment_preview(void *data)
{
	debug_enter();

	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	Evas_Object *attachment_ly = viewer_load_edj(ug_data->base.content, email_get_viewer_theme_path(), "ev/attachment/base", EVAS_HINT_EXPAND, 0.0);
	evas_object_size_hint_align_set(attachment_ly, EVAS_HINT_FILL, 0.0);
	evas_object_show(attachment_ly);
	ug_data->attachment_ly = attachment_ly;

	_header_pack_attachment_preview(ug_data);
	header_update_attachment_summary_info(ug_data);

	edje_object_signal_callback_add(_EDJ(attachment_ly), "attachment.clicked", "edje", _header_attachment_clicked_edje_cb, ug_data);

	debug_leave();
}

void header_set_sender_name(EmailViewerUGD *ug_data)
{
	debug_enter();
	retm_if(ug_data == NULL, "Invalid parameter: ug_data[NULL]");

	/* sender name */
	if (STR_VALID(ug_data->sender_display_name)) {
		/* 1. Show display name obtained form email server */
		char *markup_name = elm_entry_utf8_to_markup(ug_data->sender_display_name);
		elm_object_part_text_set(ug_data->sender_ly, "sender.name.text", markup_name);
		FREE(markup_name);

	} else if (STR_VALID(ug_data->sender_address)) {
		/* 2. If there is no sender name show email address */
		elm_object_part_text_set(ug_data->sender_ly, "sender.name.text", ug_data->sender_address);
	}

	/* sender email */
	if (STR_VALID(ug_data->sender_address)) {
		elm_object_part_text_set(ug_data->sender_ly, "sender.email.text", ug_data->sender_address);
	}

	if (!STR_VALID(ug_data->sender_display_name) && !STR_VALID(ug_data->sender_address)) {
		/* 3. Show text for no address */
		elm_object_domain_translatable_part_text_set(ug_data->sender_ly, "sender.name.text", PACKAGE, "IDS_EMAIL_MBODY_NO_EMAIL_ADDRESS_OR_NAME");
	}

	evas_object_show(ug_data->sender_ly);
	debug_leave();
}

void header_create_view(void *data)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;
	_header_create_subject(ug_data);
	_header_create_sender(ug_data);
	_header_create_divider(ug_data);

	header_layout_pack(ug_data);
	debug_leave();
}

void header_update_view(void *data)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;
	retm_if(ug_data->eViewerErrorType != VIEWER_ERROR_NONE, "Some Error occurred");

	_header_update_subject(ug_data, ug_data->mail_info);
	_header_update_sender(ug_data);
	header_update_date(ug_data);

	if (!ug_data->is_recipient_ly_shown) {
		_header_show_hide_details_text_set(ug_data, EINA_FALSE);
	} else {
		_header_show_hide_details_text_set(ug_data, EINA_TRUE);
	}

	_header_update_attachments_preview(ug_data);

	debug_leave();
}

void header_layout_unpack(void *data)
{
	debug_enter();

	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	elm_box_unpack(ug_data->main_bx, ug_data->sender_ly);

	if (ug_data->has_attachment && ug_data->attachment_ly) {
		elm_box_unpack(ug_data->main_bx, ug_data->attachment_ly);
	}

	if (ug_data->is_recipient_ly_shown) {
		if (ug_data->to_ly) {
			elm_box_unpack(ug_data->main_bx, ug_data->to_ly);
		}
		if (ug_data->cc_ly) {
			elm_box_unpack(ug_data->main_bx, ug_data->cc_ly);
		}
		if (ug_data->bcc_ly) {
			elm_box_unpack(ug_data->main_bx, ug_data->bcc_ly);
		}
	}

	elm_box_unpack(ug_data->main_bx, ug_data->header_divider);

	debug_leave();
}

void header_layout_pack(void *data)
{
	debug_enter();

	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	/* 1. sender */
	elm_box_pack_start(ug_data->main_bx, ug_data->sender_ly);
	Evas_Object *bottom_ly = ug_data->sender_ly;
	/* 2-2. recipient */
	if (ug_data->is_recipient_ly_shown) {
		if (ug_data->to_ly) {
			elm_box_pack_before(ug_data->main_bx, ug_data->to_ly, ug_data->sender_ly);
			bottom_ly = ug_data->to_ly;
		}

		if (ug_data->cc_ly) {
			if (ug_data->to_ly && ug_data->cc_ly) {
				elm_box_pack_after(ug_data->main_bx, ug_data->cc_ly, ug_data->to_ly);
			} else if (ug_data->cc_ly) {
				elm_box_pack_after(ug_data->main_bx, ug_data->cc_ly, ug_data->sender_ly);
			}
			bottom_ly = ug_data->cc_ly;
		}
		if (ug_data->bcc_ly) {
			if (ug_data->cc_ly) {
				elm_box_pack_after(ug_data->main_bx, ug_data->bcc_ly, ug_data->cc_ly);
			}
			if (ug_data->to_ly && ug_data->cc_ly == NULL) {
				elm_box_pack_after(ug_data->main_bx, ug_data->bcc_ly, ug_data->to_ly);
			}
			if (ug_data->to_ly == NULL && ug_data->cc_ly == NULL) {
				elm_box_pack_after(ug_data->main_bx, ug_data->bcc_ly, ug_data->sender_ly);
			}
			bottom_ly = ug_data->bcc_ly;
		}
	}

	elm_box_pack_after(ug_data->main_bx, ug_data->header_divider, bottom_ly);
	bottom_ly = ug_data->header_divider;

	if (ug_data->has_attachment && ug_data->attachment_ly) {
		elm_box_pack_after(ug_data->main_bx, ug_data->attachment_ly, bottom_ly);
	}

	debug_leave();
}

static void _header_update_attachments_preview(void *data)
{
	debug_enter();

	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	if (ug_data->has_attachment) {
		_header_delete_attachment_preview(ug_data);
		_header_create_attachment_preview(ug_data);
	} else {
		_header_delete_attachment_preview(ug_data);
	}

	debug_leave();
}

void header_update_date(EmailViewerUGD *ug_data)
{
	debug_enter();
	retm_if(ug_data == NULL, "Invalid parameter: ug_data[NULL]");

	viewer_close_pattern_generator();
	viewer_open_pattern_generator();

	char *formatted_date = NULL;
	char date_time[MAX_STR_LEN] = { 0, };

	formatted_date = email_get_date_text_with_formatter(viewer_date_formatter, &(ug_data->mktime));
	if (formatted_date != NULL) {
		snprintf(date_time, MAX_STR_LEN, "%s", formatted_date);
		FREE(formatted_date);
	}

	elm_object_part_text_set(ug_data->sender_ly, "sender.date.text", date_time);

	if ((ug_data->mailbox_type != EMAIL_MAILBOX_TYPE_OUTBOX) && (ug_data->viewer_type == EMAIL_VIEWER)) {
		header_set_favorite_icon(ug_data);
	}

	debug_leave();
}

void header_update_favorite_icon(EmailViewerUGD *ug_data)
{
	header_set_favorite_icon(ug_data);
}

void header_set_favorite_icon(EmailViewerUGD *ug_data)
{
	debug_enter();
	retm_if(ug_data == NULL, "Invalid parameter: ug_data[NULL]");

	Evas_Object *ly = elm_layout_add(ug_data->sender_ly);

	debug_log("favorite: %d", ug_data->favorite);

	switch (ug_data->favorite) {
		case EMAIL_FLAG_NONE:
			elm_layout_file_set(ly, email_get_viewer_theme_path(), "elm/layout/flag.none.icon/default");
			break;
		case EMAIL_FLAG_FLAGED:
			elm_layout_file_set(ly, email_get_viewer_theme_path(), "elm/layout/flag.star.icon/default");
			break;
	}
	elm_object_part_content_set(ug_data->sender_ly, "sender.fav.event", ly);
	edje_object_signal_callback_add(_EDJ(ly), "clicked", "edje", _header_favorite_clicked_cb, ug_data);

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
