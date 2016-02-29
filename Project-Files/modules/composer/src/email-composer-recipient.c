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

#include <string.h>
#include <notification.h>

#include "email-utils.h"
#include "email-composer.h"
#include "email-composer-recipient.h"
#include "email-composer-attachment.h"
#include "email-composer-util.h"
#include "email-composer-predictive-search.h"

/*
 * Declaration for static functions
 */
#define RECIPIENT_PADDING_MIN_MAX_WIDTH 15

static void _recipient_entry_layout_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source);

static void _recipient_get_header_string(COMPOSER_RECIPIENT_TYPE_E recp_type, char *buf, int buf_len);
static void _recipient_notify_invalid_recipient(EmailComposerView *view, MBE_VALIDATION_ERROR err);
static MBE_VALIDATION_ERROR _recipient_mbe_validate_email_address(EmailComposerView *view, Evas_Object *obj, char *email_address);

static void _recipient_from_button_update_text(EmailComposerView *view, email_account_t *account);
static void _recipient_from_ctxpopup_move_list(EmailComposerView *view);
static void _recipient_from_ctxpopup_dismissed_cb(void *data, Evas_Object *obj, void *event_info);
static Eina_Bool _recipient_from_ctxpopup_show(void *data);
static void _recipient_from_ctxpopup_resized_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _recipient_from_ctxpopup_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _recipient_from_ctxpopup_item_select_cb(void *data, Evas_Object *obj, void *event_info);
static void _recipient_from_button_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static void _recipient_account_change_size_exceeded_cb(void *data, Evas_Object *obj, void *event_info);

static Evas_Object *_recipient_create_contact_button(EmailComposerView *view, Evas_Object *parent);
static Evas_Object *_recipient_create_mbe(Evas_Object *parent, COMPOSER_RECIPIENT_TYPE_E recp_type, EmailComposerView *view);
static void _recipient_create_editfield(Evas_Object *parent, EmailComposerView *view, Eina_Bool is_editable, email_editfield_t *editfield);
static Evas_Object *_recipient_create_entry_label(Evas_Object *parent, COMPOSER_RECIPIENT_TYPE_E recp_type, EmailComposerView *view);
static Evas_Object *_recipient_create_layout(Evas_Object *parent, const char *group);
static void _recipient_ctxpopup_back_cb(void *data, Evas_Object *obj, void *event_info);

static Eina_Bool _recipient_timer_duplicate_toast_cb(void *data);
static Evas_Object *_recipient_get_mbe_with_entry(void *data, Evas_Object *entry);

typedef struct _fromListItemData {
	EmailComposerView *view;
	Elm_Object_Item *item;
	int index;
} fromListItemData;


/*
 * Definition for static functions
 */

static void _recipient_entry_layout_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;
	ret_if(!view->is_load_finished);

	if (!view->allow_click_events) {
		debug_log("Click was blocked.");
		return;
	}

	if ((obj == view->recp_to_layout) && (view->selected_entry != view->recp_to_entry.entry)) {
		composer_recipient_change_entry(EINA_TRUE, view->recp_to_box, &view->recp_to_entry, &view->recp_to_display_entry, view->recp_to_entry_layout, view->recp_to_display_entry_layout);
		composer_util_focus_set_focus(view, view->recp_to_entry.entry);
	} else if ((obj == view->recp_cc_layout) && (view->selected_entry != view->recp_cc_entry.entry)) {
		composer_recipient_change_entry(EINA_TRUE, view->recp_cc_box, &view->recp_cc_entry, &view->recp_cc_display_entry, view->recp_cc_entry_layout, view->recp_cc_display_entry_layout);
		composer_util_focus_set_focus(view, view->recp_cc_entry.entry);
	} else if ((obj == view->recp_bcc_layout) && (view->selected_entry != view->recp_bcc_entry.entry)) {
		composer_recipient_change_entry(EINA_TRUE, view->recp_bcc_box, &view->recp_bcc_entry, &view->recp_bcc_display_entry, view->recp_bcc_entry_layout, view->recp_bcc_display_entry_layout);
		composer_util_focus_set_focus(view, view->recp_bcc_entry.entry);
	}

	debug_leave();
}

static void _recipient_get_header_string(COMPOSER_RECIPIENT_TYPE_E recp_type, char *buf, int buf_len)
{
	debug_enter();

	char *header = NULL;
	if (recp_type == COMPOSER_RECIPIENT_TYPE_FROM) {
		header = email_get_email_string("IDS_EMAIL_BODY_FROM_MSENDER");
		snprintf(buf, buf_len, TEXT_STYLE_ENTRY_FROM_ADDRESS, COLOR_BLUE, header);
		return;
	} else if (recp_type == COMPOSER_RECIPIENT_TYPE_TO) {
		header = email_get_email_string("IDS_EMAIL_BODY_TO_M_RECIPIENT");
	} else if (recp_type == COMPOSER_RECIPIENT_TYPE_CC) {
		header = email_get_email_string("IDS_EMAIL_BODY_CC");
	} else if (recp_type == COMPOSER_RECIPIENT_TYPE_BCC) {
		header = email_get_email_string("IDS_EMAIL_BODY_BCC");
	} else if (recp_type == COMPOSER_RECIPIENT_TYPE_CC_BCC) {
		header = email_get_email_string("IDS_EMAIL_BODY_CC_BCC");
	}

	snprintf(buf, buf_len, "%s", header);

	debug_leave();
}

static void _recipient_notify_invalid_recipient(EmailComposerView *view, MBE_VALIDATION_ERROR err)
{
	debug_enter();

	if (!view->is_back_btn_clicked && !view->is_send_btn_clicked && !view->is_save_in_drafts_clicked) {
		int noti_ret = NOTIFICATION_ERROR_NONE;
		switch (err) {
			case MBE_VALIDATION_ERROR_NO_ADDRESS:
				noti_ret = notification_status_message_post(email_get_email_string("IDS_EMAIL_TPOP_ENTER_AT_LEAST_ONE_VALID_RECIPIENT"));
				debug_warning_if(noti_ret != NOTIFICATION_ERROR_NONE, "notification_status_message_post() failed! ret:(%d)", noti_ret);
				break;
			case MBE_VALIDATION_ERROR_DUPLICATE_ADDRESS:
				noti_ret = notification_status_message_post(email_get_email_string("IDS_EMAIL_TPOP_RECIPIENT_ALREADY_EXISTS"));
				debug_warning_if(noti_ret != NOTIFICATION_ERROR_NONE, "notification_status_message_post() failed! ret:(%d)", noti_ret);
				break;
			case MBE_VALIDATION_ERROR_MAX_NUMBER_REACHED:
			{
				char buf[BUF_LEN_L] = { 0, };
				snprintf(buf, sizeof(buf), email_get_email_string("IDS_EMAIL_TPOP_MAXIMUM_NUMBER_OF_RECIPIENTS_HPD_REACHED"), MAX_RECIPIENT_COUNT);

				noti_ret = notification_status_message_post(buf);
				debug_warning_if(noti_ret != NOTIFICATION_ERROR_NONE, "notification_status_message_post() failed! ret:(%d)", noti_ret);
				break;
			}
			case MBE_VALIDATION_ERROR_INVALID_ADDRESS:
				noti_ret = notification_status_message_post(email_get_email_string("IDS_EMAIL_TPOP_INVALID_EMAIL_ADDRESS_ENTERED"));
				debug_warning_if(noti_ret != NOTIFICATION_ERROR_NONE, "notification_status_message_post() failed! ret:(%d)", noti_ret);
				break;
			default:
				debug_error("NEVER enter here!!");
				break;
		}
	}

	composer_util_focus_set_focus_with_idler(view, view->selected_entry);

	debug_leave();
}

static MBE_VALIDATION_ERROR _recipient_mbe_validate_email_address(EmailComposerView *view, Evas_Object *obj, char *email_address)
{
	debug_enter();

	MBE_VALIDATION_ERROR ret = MBE_VALIDATION_ERROR_NONE;
	if (email_get_address_validation(email_address)) {
		if (composer_util_recp_is_duplicated(view, obj, email_address)) {
			ret = MBE_VALIDATION_ERROR_DUPLICATE_ADDRESS;
		} else if (((obj == view->recp_to_mbe) && (view->to_recipients_cnt >= MAX_RECIPIENT_COUNT)) ||
			((obj == view->recp_cc_mbe) && (view->cc_recipients_cnt >= MAX_RECIPIENT_COUNT)) ||
			((obj == view->recp_bcc_mbe) && (view->bcc_recipients_cnt >= MAX_RECIPIENT_COUNT))) {
			ret = MBE_VALIDATION_ERROR_MAX_NUMBER_REACHED;
		}
	} else {
		char *nick = NULL;
		char *addr = NULL;

		email_get_recipient_info(email_address, &nick, &addr);

		if (email_get_address_validation(addr)) {
			if (composer_util_recp_is_duplicated(view, obj, addr)) {
				ret = MBE_VALIDATION_ERROR_DUPLICATE_ADDRESS;
			} else if (((obj == view->recp_to_mbe) && (view->to_recipients_cnt >= MAX_RECIPIENT_COUNT)) ||
				((obj == view->recp_cc_mbe) && (view->cc_recipients_cnt >= MAX_RECIPIENT_COUNT)) ||
				((obj == view->recp_bcc_mbe) && (view->bcc_recipients_cnt >= MAX_RECIPIENT_COUNT))) {
				ret = MBE_VALIDATION_ERROR_MAX_NUMBER_REACHED;
			}
		} else {
			ret = MBE_VALIDATION_ERROR_INVALID_ADDRESS;
		}

		/* If there's quatation mark in the email address, a user can see abnormal animation while adding recipients to mbe. so we need to re-add the address.
		 * e.g.> "ABCD" <hello@ok.com> ==> ABCD<hello@ok.com>
		 */
		if (ret == MBE_VALIDATION_ERROR_NONE) {
			char *single_quotation_mark = g_utf8_strchr(email_address, strlen(email_address), '\'');
			if (single_quotation_mark) {
				ret = MBE_VALIDATION_ERROR_QUOTATIONS;
			} else {
				char *double_quotation_mark = g_utf8_strchr(email_address, strlen(email_address), '\"');
				if (double_quotation_mark) {
					ret = MBE_VALIDATION_ERROR_QUOTATIONS;
				}
			}
		}

		g_free(addr);
		g_free(nick);
	}

	debug_leave();
	return ret;
}


static void _recipient_from_button_update_text(EmailComposerView *view, email_account_t *account)
{
	debug_enter();

	retm_if(!view, "Invalid parameter: data is NULL!");
	retm_if(!account, "account is NULL!");

	char from_display_name[BUF_LEN_L] = { 0, };
	if (account->account_name && (g_strcmp0(account->account_name, account->user_email_address) != 0)) {
		snprintf(from_display_name, sizeof(from_display_name), "%s <%s>", account->account_name, account->user_email_address);
	} else {
		snprintf(from_display_name, sizeof(from_display_name), "%s", account->user_email_address);
	}

	EmailRecpInfo *ri = composer_util_recp_make_recipient_info_with_display_name(account->user_email_address, from_display_name);
	if (ri) {
		char *markup_name = elm_entry_utf8_to_markup(ri->display_name);
		elm_object_part_text_set(view->recp_from_btn, "elm.text", markup_name);
		FREE(markup_name);

		EmailAddressInfo *ai = NULL;
		EINA_LIST_FREE(ri->email_list, ai) {
			FREE(ai);
		}
		g_free(ri->display_name);
		free(ri);
	}

	debug_leave();
}

static void _recipient_from_ctxpopup_move_list(EmailComposerView *view)
{
	debug_enter();

	int pos = -1;
	Evas_Coord x = 0, y = 0, w = 0, h = 0;
	Evas_Coord root_win_width = 0, root_win_height = 0;

	pos = elm_win_rotation_get(view->base.module->win);
	evas_object_geometry_get(view->recp_from_btn, &x, &y, &w, &h);
	elm_win_screen_size_get(view->base.module->win, NULL, NULL, &root_win_width, &root_win_height);

	switch (pos) {
		case 0:
		case 180:
			w = root_win_width - x;
			evas_object_move(view->recp_from_ctxpopup, (x / 2) + (w / 2), y + h);
			break;
		case 90:
		case 270:
			w = root_win_height - x;
			evas_object_move(view->recp_from_ctxpopup, x + (w / 2), y + h);
			break;
		default:
			break;
	}

	DELETE_IDLER_OBJECT(view->idler_show_ctx_popup);
	view->idler_show_ctx_popup = ecore_idler_add(_recipient_from_ctxpopup_show, view);
	debug_leave();
}

static Eina_Bool _recipient_from_ctxpopup_show(void *data)
{

	debug_enter();
	EmailComposerView *view = (EmailComposerView *)data;

	view->idler_show_ctx_popup = NULL;

	evas_object_show(view->recp_from_ctxpopup);

	debug_leave();
	return ECORE_CALLBACK_CANCEL;
}

static void _recipient_from_ctxpopup_dismissed_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	evas_object_del(view->recp_from_ctxpopup);
	view->recp_from_ctxpopup = NULL;
	composer_recipient_from_ctxpopup_item_delete(view);
	composer_util_focus_set_focus_with_idler(view, view->selected_entry);

	debug_leave();
}

static void _recipient_from_ctxpopup_resized_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	_recipient_from_ctxpopup_move_list(view);

	debug_leave();
}

static void _recipient_from_ctxpopup_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	/* XXX; check this. deletion for dismissed can be deleted. */
	evas_object_smart_callback_del(view->recp_from_ctxpopup, "dismissed", _recipient_from_ctxpopup_dismissed_cb);
	evas_object_event_callback_del_full(view->base.module->win, EVAS_CALLBACK_RESIZE, _recipient_from_ctxpopup_resized_cb, view);

	debug_leave();
}

static void _recipient_from_ctxpopup_item_select_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	fromListItemData *it = (fromListItemData *)data;
	retm_if(!it, "Invalid parameter: item data is NULL!");

	EmailComposerView *view = (EmailComposerView *)it->view;
	Eina_Bool is_popup_displayed = EINA_FALSE;

	if (view->account_info->selected_account->account_id != view->account_info->account_list[it->index].account_id) {
		char *previously_selected_email = view->account_info->selected_account->user_email_address;
		email_account_t *selected_account = &view->account_info->account_list[it->index];

		int max_sending_size = 0;
		if (selected_account->outgoing_server_size_limit == 0 || selected_account->outgoing_server_size_limit == -1) {
			max_sending_size = MAX_ATTACHMENT_SIZE;
		} else {
			max_sending_size = selected_account->outgoing_server_size_limit / 1.5;
		}
		debug_log("outgoing_server_size_limit:%d, max_sending_size:%d", selected_account->outgoing_server_size_limit, max_sending_size);

		view->account_info->selected_account = selected_account;
		view->account_info->account_type = selected_account->incoming_server_type;
		view->account_info->max_sending_size = max_sending_size;

		/* Only display notification popup to user and allowing the account switch and disabling send button */
		if (composer_util_is_max_sending_size_exceeded(view)) {
			composer_attachment_launch_attachment_error_popup(COMPOSER_ERROR_ATTACHMENT_MAX_SIZE_EXCEEDED, _recipient_account_change_size_exceeded_cb, view);
			is_popup_displayed = EINA_TRUE;
		}
		composer_util_modify_send_button(view);

		composer_util_update_attach_panel_bundles(view);

		debug_log("changed account_id:[%d]", view->account_info->selected_account->account_id);

#ifdef _ALWAYS_CC_MYSELF
		debug_secure("removing cc/bcc address (%s)", previously_selected_email);
		/* Remove previously added item for from address by always_cc/bcc myself */
		if (previously_selected_email) {
			composer_recipient_remove_myaddress(view->recp_cc_mbe, previously_selected_email); /* Remove cc myself if it is */
			composer_recipient_remove_myaddress(view->recp_bcc_mbe, previously_selected_email); /* Remove bcc myself if it is */
		}

		/* Add item to cc or bcc field if always cc/bcc is enabled. */
		if (view->account_info->selected_account->options.add_my_address_to_bcc != EMAIL_ADD_MY_ADDRESS_OPTION_DO_NOT_ADD) {
			composer_recipient_append_myaddress(view, view->account_info->selected_account);
		}
#endif

		/* Change from text */
		_recipient_from_button_update_text(view, view->account_info->selected_account);
		composer_attachment_ui_group_update_text(view);
	} else {
		debug_log("No need to change from address! selected account is the same as previous one!");
	}

	if (!is_popup_displayed)
		_recipient_from_ctxpopup_dismissed_cb(view, NULL, NULL);

	debug_leave();
}

static void _recipient_account_change_size_exceeded_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	_recipient_from_ctxpopup_dismissed_cb(view, NULL, NULL);
	composer_util_popup_response_cb(view, NULL, NULL);

	debug_leave();

}

static void _recipient_ctxpopup_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	if (obj) {
		elm_ctxpopup_dismiss(obj);
	}
}

static void _recipient_from_button_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	if (!view->allow_click_events) {
		debug_log("Click was blocked.");
		return;
	}

	if (view->recp_from_ctxpopup) {
		_recipient_from_ctxpopup_dismissed_cb(view, NULL, NULL);
		return;
	}

	Evas_Object *ctxpopup = elm_ctxpopup_add(view->base.module->navi);
	// TODO: Remove comment when "dropdown/list" style will be available
	//elm_object_style_set(ctxpopup, "dropdown/list");
	elm_ctxpopup_auto_hide_disabled_set(ctxpopup, EINA_TRUE);
	elm_ctxpopup_direction_priority_set(ctxpopup, ELM_CTXPOPUP_DIRECTION_DOWN, ELM_CTXPOPUP_DIRECTION_UNKNOWN, ELM_CTXPOPUP_DIRECTION_UNKNOWN, ELM_CTXPOPUP_DIRECTION_UNKNOWN);

	Eina_List *item_list = NULL;
	int index;
	for (index = 0; index < view->account_info->account_count; index++) {
		fromListItemData *it = (fromListItemData *)malloc(sizeof(fromListItemData));
		if (!it) {
			DELETE_EVAS_OBJECT(ctxpopup);
			debug_error("it is NULL - allocation memory failed");
			return;
		}
		it->index = index;
		it->view = view;
		it->item = elm_ctxpopup_item_append(ctxpopup, view->account_info->account_list[index].user_email_address, NULL, _recipient_from_ctxpopup_item_select_cb, it);
		item_list = eina_list_append(item_list, it);
	}

	evas_object_smart_callback_add(ctxpopup, "dismissed", _recipient_from_ctxpopup_dismissed_cb, view);
	evas_object_event_callback_add(ctxpopup, EVAS_CALLBACK_DEL, _recipient_from_ctxpopup_del_cb, view);
	eext_object_event_callback_add(ctxpopup, EEXT_CALLBACK_BACK, _recipient_ctxpopup_back_cb, NULL);
	eext_object_event_callback_add(ctxpopup, EEXT_CALLBACK_MORE, _recipient_ctxpopup_back_cb, NULL);
	evas_object_event_callback_add(view->base.module->win, EVAS_CALLBACK_RESIZE, _recipient_from_ctxpopup_resized_cb, view);

	view->recp_from_item_list = item_list;
	view->recp_from_ctxpopup = ctxpopup;

	_recipient_from_ctxpopup_move_list(view);

	debug_leave();
}

static Evas_Object *_recipient_create_contact_button(EmailComposerView *view, Evas_Object *parent)
{
	email_profiling_begin(_recipient_create_contact_button);
	debug_enter();

	Evas_Object *btn;
	Evas_Object *icon;

	btn = elm_button_add(parent);
	elm_object_style_set(btn, "transparent");

	icon = elm_layout_add(btn);
	elm_layout_file_set(icon, email_get_common_theme_path(), EMAIL_IMAGE_COMPOSER_CONTACT);
	evas_object_show(icon);

	elm_object_part_content_set(btn, "elm.swallow.content", icon);

	evas_object_smart_callback_add(btn, "clicked", _recipient_contact_button_clicked_cb, view);

	debug_leave();
	email_profiling_end(_recipient_create_contact_button);
	return btn;
}

static Evas_Object *_recipient_create_mbe(Evas_Object *parent, COMPOSER_RECIPIENT_TYPE_E recp_type, EmailComposerView *view)
{
	email_profiling_begin(_recipient_create_mbe);
	debug_enter();

	Evas_Object *mbe = elm_multibuttonentry_add(parent);
	evas_object_size_hint_weight_set(mbe, EVAS_HINT_EXPAND, 0);
	evas_object_size_hint_align_set(mbe, EVAS_HINT_FILL, 0);
	evas_object_show(mbe);
	elm_multibuttonentry_editable_set(mbe, EINA_FALSE);
	elm_multibuttonentry_expanded_set(mbe, EINA_TRUE);

	composer_recipient_label_text_set(mbe, recp_type);
	elm_multibuttonentry_item_filter_append(mbe, _recipient_mbe_filter_cb, view);
	evas_object_smart_callback_add(mbe, "item,added", _recipient_mbe_added_cb, view);
	evas_object_smart_callback_add(mbe, "item,deleted", _recipient_mbe_deleted_cb, view);
	evas_object_smart_callback_add(mbe, "item,clicked", _recipient_mbe_selected_cb, view);
	evas_object_smart_callback_add(mbe, "focused", _recipient_mbe_focused_cb, view);

	debug_leave();
	email_profiling_end(_recipient_create_mbe);
	return mbe;
}

static void _recipient_create_editfield(Evas_Object *parent, EmailComposerView *view, Eina_Bool is_editable, email_editfield_t *editfield)
{
	email_profiling_begin(_recipient_create_entry);
	debug_enter();

	email_common_util_editfield_create(parent, 0, editfield);
	if (is_editable) {
		elm_entry_cnp_mode_set(editfield->entry, ELM_CNP_MODE_PLAINTEXT);
		elm_entry_input_panel_layout_set(editfield->entry, ELM_INPUT_PANEL_LAYOUT_EMAIL);
		elm_entry_input_panel_return_key_type_set(editfield->entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_NEXT);
		elm_entry_prediction_allow_set(editfield->entry, EINA_TRUE);
		elm_entry_autocapital_type_set(editfield->entry, ELM_AUTOCAPITAL_TYPE_NONE);
		eext_entry_selection_back_event_allow_set(editfield->entry, EINA_TRUE);
		evas_object_smart_callback_add(editfield->entry, "preedit,changed", _recipient_entry_changed_cb, view);
		evas_object_smart_callback_add(editfield->entry, "changed", _recipient_entry_changed_cb, view);
		evas_object_smart_callback_add(editfield->entry, "focused", _recipient_entry_focused_cb, view);
		evas_object_smart_callback_add(editfield->entry, "unfocused", _recipient_entry_unfocused_cb, view);
		evas_object_event_callback_add(editfield->entry, EVAS_CALLBACK_KEY_DOWN, _recipient_entry_keydown_cb, view);
		evas_object_event_callback_add(editfield->entry, EVAS_CALLBACK_KEY_UP, _recipient_entry_keyup_cb, view);
	} else {
		elm_entry_editable_set(editfield->entry, EINA_FALSE);
		elm_object_focus_allow_set(editfield->entry, EINA_FALSE);
	}

	composer_util_set_entry_text_style(editfield->entry);
	evas_object_show(editfield->entry);

	debug_leave();
	email_profiling_end(_recipient_create_entry);
}

static Evas_Object *_recipient_create_entry_label(Evas_Object *parent, COMPOSER_RECIPIENT_TYPE_E recp_type, EmailComposerView *view)
{
	email_profiling_begin(_recipient_create_entry_label);
	debug_enter();

	Evas_Object *label = elm_layout_add(parent);
	elm_layout_theme_set(label, "multibuttonentry", "label", "default");
	evas_object_show(label);

	composer_recipient_label_text_set(label, recp_type);

	debug_leave();
	email_profiling_end(_recipient_create_entry_label);
	return label;
}

static Evas_Object *_recipient_create_layout(Evas_Object *parent, const char *group)
{
	email_profiling_begin(_recipient_create_layout);
	debug_enter();

	Evas_Object *layout = elm_layout_add(parent);
	elm_layout_file_set(layout, email_get_composer_theme_path(), group);
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(layout);

	debug_leave();
	email_profiling_end(_recipient_create_layout);
	return layout;
}


/*
 * Definition for exported functions
 */

Evas_Object *composer_recipient_create_layout(void *data, Evas_Object *parent)
{
	email_profiling_begin(composer_recipient_create_layout);
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;
	Evas_Object *layout = _recipient_create_layout(parent, "ec/recipient/entry/base");

	elm_layout_signal_callback_add(layout, "ec,action,clicked", "", _recipient_entry_layout_clicked_cb, view);

	debug_leave();
	email_profiling_end(composer_recipient_create_layout);
	return layout;
}

void composer_recipient_update_to_detail(void *data, Evas_Object *parent)
{
	email_profiling_begin(composer_recipient_update_to_detail);
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	email_editfield_t editfield;
	email_editfield_t dispaly_editfield;
	Evas_Object *box = composer_util_create_box(parent);
	Evas_Object *entry_layout = composer_util_create_entry_layout(box);
	Evas_Object *label = _recipient_create_entry_label(box, COMPOSER_RECIPIENT_TYPE_TO, view);
	Evas_Object *btn = _recipient_create_contact_button(view, parent);
	Evas_Object *display_entry_layout = composer_util_create_entry_layout(box);

	_recipient_create_editfield(entry_layout, view, EINA_TRUE, &editfield);
	_recipient_create_editfield(display_entry_layout, view, EINA_FALSE, &dispaly_editfield);

	elm_object_part_content_set(entry_layout, "ec.swallow.content", editfield.layout);
	elm_object_part_content_set(display_entry_layout, "ec.swallow.content", dispaly_editfield.layout);

	elm_box_horizontal_set(box, EINA_TRUE);
	elm_box_pack_end(box, label);
	elm_box_pack_end(box, entry_layout);
	elm_object_part_content_set(parent, "ec.swallow.content", box);

	evas_object_hide(btn);
	evas_object_hide(dispaly_editfield.layout);
	evas_object_hide(display_entry_layout);

	view->recp_to_box = box;
	view->recp_to_label = label;
	view->recp_to_entry = editfield;
	view->recp_to_entry_layout = entry_layout;
	view->recp_to_btn = btn;
	view->recp_to_display_entry = dispaly_editfield;
	view->recp_to_display_entry_layout = display_entry_layout;

	Evas_Object *mbe_layout = _recipient_create_layout(parent, "ec/recipient/mbe/base");
	Evas_Object *mbe = _recipient_create_mbe(mbe_layout, COMPOSER_RECIPIENT_TYPE_TO, view);
	elm_object_part_content_set(mbe_layout, "ec.swallow.content", mbe);
	evas_object_hide(mbe_layout);

	view->recp_to_mbe_layout = mbe_layout;
	view->recp_to_mbe = mbe;

	debug_leave();
	email_profiling_end(composer_recipient_update_to_detail);
}

void composer_recipient_update_cc_detail(void *data, Evas_Object *parent)
{
	email_profiling_begin(composer_recipient_update_cc_detail);
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	email_editfield_t editfield;
	email_editfield_t dispaly_editfield;
	Evas_Object *box = composer_util_create_box(parent);
	Evas_Object *entry_layout = composer_util_create_entry_layout(box);
	Evas_Object *label_cc = _recipient_create_entry_label(box, COMPOSER_RECIPIENT_TYPE_CC, view);
	Evas_Object *label_cc_bcc = _recipient_create_entry_label(box, COMPOSER_RECIPIENT_TYPE_CC_BCC, view);
	Evas_Object *btn = _recipient_create_contact_button(view, parent);
	Evas_Object *display_entry_layout = composer_util_create_entry_layout(box);

	_recipient_create_editfield(entry_layout, view, EINA_TRUE, &editfield);
	_recipient_create_editfield(display_entry_layout, view, EINA_FALSE, &dispaly_editfield);

	elm_object_part_content_set(entry_layout, "ec.swallow.content", editfield.layout);
	elm_object_part_content_set(display_entry_layout, "ec.swallow.content", dispaly_editfield.layout);

	elm_box_horizontal_set(box, EINA_TRUE);
	elm_box_pack_end(box, label_cc_bcc);
	elm_box_pack_end(box, entry_layout);
	elm_object_part_content_set(parent, "ec.swallow.content", box);

	evas_object_hide(label_cc);
	evas_object_hide(btn);
	evas_object_hide(dispaly_editfield.layout);
	evas_object_hide(display_entry_layout);

	view->recp_cc_box = box;
	view->recp_cc_label_cc = label_cc;
	view->recp_cc_label_cc_bcc = label_cc_bcc;
	view->recp_cc_entry = editfield;
	view->recp_cc_entry_layout = entry_layout;
	view->recp_cc_btn = btn;
	view->recp_cc_display_entry = dispaly_editfield;
	view->recp_cc_display_entry_layout = display_entry_layout;

	Evas_Object *mbe_layout = _recipient_create_layout(parent, "ec/recipient/mbe/base");
	Evas_Object *mbe = _recipient_create_mbe(box, COMPOSER_RECIPIENT_TYPE_CC, view);
	elm_object_part_content_set(mbe_layout, "ec.swallow.content", mbe);
	evas_object_hide(mbe_layout);

	view->recp_cc_mbe_layout = mbe_layout;
	view->recp_cc_mbe = mbe;

	debug_leave();
	email_profiling_end(composer_recipient_update_cc_detail);
}

void composer_recipient_update_bcc_detail(void *data, Evas_Object *parent)
{
	email_profiling_begin(composer_recipient_update_bcc_detail);
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;
	email_editfield_t editfield;
	email_editfield_t dispaly_editfield;
	Evas_Object *box = composer_util_create_box(parent);
	Evas_Object *entry_layout = composer_util_create_entry_layout(box);
	Evas_Object *label = _recipient_create_entry_label(box, COMPOSER_RECIPIENT_TYPE_BCC, view);
	Evas_Object *btn = _recipient_create_contact_button(view, parent);
	Evas_Object *display_entry_layout = composer_util_create_entry_layout(box);

	_recipient_create_editfield(entry_layout, view, EINA_TRUE, &editfield);
	_recipient_create_editfield(display_entry_layout, view, EINA_FALSE, &dispaly_editfield);

	elm_object_part_content_set(entry_layout, "ec.swallow.content", editfield.layout);
	elm_object_part_content_set(display_entry_layout, "ec.swallow.content", dispaly_editfield.layout);

	elm_box_horizontal_set(box, EINA_TRUE);
	elm_box_pack_end(box, label);
	elm_box_pack_end(box, entry_layout);
	elm_object_part_content_set(parent, "ec.swallow.content", box);

	evas_object_hide(btn);
	evas_object_hide(dispaly_editfield.layout);
	evas_object_hide(display_entry_layout);

	view->recp_bcc_box = box;
	view->recp_bcc_label = label;
	view->recp_bcc_entry = editfield;
	view->recp_bcc_entry_layout = entry_layout;
	view->recp_bcc_btn = btn;
	view->recp_bcc_display_entry = dispaly_editfield;
	view->recp_bcc_display_entry_layout = display_entry_layout;

	Evas_Object *mbe_layout = _recipient_create_layout(parent, "ec/recipient/mbe/base");
	Evas_Object *mbe = _recipient_create_mbe(box, COMPOSER_RECIPIENT_TYPE_BCC, view);
	elm_object_part_content_set(mbe_layout, "ec.swallow.content", mbe);
	evas_object_hide(mbe_layout);

	view->recp_bcc_mbe_layout = mbe_layout;
	view->recp_bcc_mbe = mbe;

	debug_leave();
	email_profiling_end(composer_recipient_update_bcc_detail);
}

void composer_recipient_update_from_detail(void *data, Evas_Object *parent)
{
	email_profiling_begin(composer_recipient_update_from_detail);
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	Evas_Object *box = composer_util_create_box(parent);
	Evas_Object *label = _recipient_create_entry_label(box, COMPOSER_RECIPIENT_TYPE_FROM, view);

	Evas_Object *layout = elm_layout_add(box);
	elm_layout_file_set(layout, email_get_composer_theme_path(), "ec/recipient/layout/from");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(layout);

	Evas_Object *btn = elm_button_add(layout);
	elm_object_style_set(btn, "dropdown");
	evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_smart_callback_add(btn, "clicked", _recipient_from_button_clicked_cb, view);
	evas_object_show(btn);

	elm_object_part_content_set(layout, "ec.swallow.content", btn);

	elm_box_horizontal_set(box, EINA_TRUE);
	elm_box_pack_end(box, label);
	elm_box_pack_end(box, layout);
	elm_object_part_content_set(parent, "ec.swallow.content", box);

	view->recp_from_btn = btn;
	view->recp_from_label = label;

	_recipient_from_button_update_text(view, view->account_info->original_account);

	debug_leave();
	email_profiling_end(composer_recipient_update_from_detail);
}

void composer_recipient_from_ctxpopup_item_delete(EmailComposerView *view)
{
	debug_enter();

	Eina_List *list = view->recp_from_item_list;
	fromListItemData *item_data = NULL;

	EINA_LIST_FREE(list, item_data) {
		FREE(item_data);
	}
	view->recp_from_item_list = NULL;

	debug_leave();
}

void composer_recipient_show_hide_cc_field(EmailComposerView *view, Eina_Bool to_be_showed)
{
	debug_enter();

	if (to_be_showed) {
		if (!composer_util_is_object_packed_in(view->composer_box, view->recp_cc_layout)) {
			evas_object_show(view->recp_cc_layout);
			elm_box_pack_after(view->composer_box, view->recp_cc_layout, view->recp_to_layout);
		}
		view->cc_added = EINA_TRUE;
	} else {
		if (composer_util_is_object_packed_in(view->composer_box, view->recp_cc_layout)) {
			elm_box_unpack(view->composer_box, view->recp_cc_layout);
			evas_object_hide(view->recp_cc_layout);
		}
		view->cc_added = EINA_FALSE;
	}

	debug_leave();
}

void composer_recipient_show_bcc_field(EmailComposerView *view)
{
	debug_enter();

	/* XXX; need to update this? */
	if (view->recp_cc_mbe) {
		composer_recipient_label_text_set(view->recp_cc_mbe, COMPOSER_RECIPIENT_TYPE_CC);
	}

	if (composer_util_is_object_packed_in(view->recp_cc_box, view->recp_cc_label_cc_bcc)) {
		elm_box_unpack(view->recp_cc_box, view->recp_cc_label_cc_bcc);
		evas_object_hide(view->recp_cc_label_cc_bcc);
		evas_object_show(view->recp_cc_label_cc);
		elm_box_pack_start(view->recp_cc_box, view->recp_cc_label_cc);
	}

	if (!composer_util_is_object_packed_in(view->composer_box, view->recp_bcc_layout)) {
		evas_object_show(view->recp_bcc_layout);
		elm_box_pack_after(view->composer_box, view->recp_bcc_layout, view->recp_cc_layout);
	}

	debug_leave();
}

void composer_recipient_hide_bcc_field(EmailComposerView *view)
{
	debug_enter();

	if (view->recp_cc_mbe) {
		composer_recipient_label_text_set(view->recp_cc_mbe, COMPOSER_RECIPIENT_TYPE_CC_BCC);
	}

	if (composer_util_is_object_packed_in(view->recp_cc_box, view->recp_cc_label_cc)) {
		elm_box_unpack(view->recp_cc_box, view->recp_cc_label_cc);
		evas_object_hide(view->recp_cc_label_cc);
		evas_object_show(view->recp_cc_label_cc_bcc);
		elm_box_pack_start(view->recp_cc_box, view->recp_cc_label_cc_bcc);
	}

	if (composer_util_is_object_packed_in(view->composer_box, view->recp_bcc_layout)) {
		elm_box_unpack(view->composer_box, view->recp_bcc_layout);
		evas_object_hide(view->recp_bcc_layout);
	}

	debug_leave();
}

void composer_recipient_show_hide_bcc_field(EmailComposerView *view, Eina_Bool to_be_showed)
{
	debug_enter();

	if (to_be_showed) {
		composer_recipient_show_bcc_field(view);
	} else {
		composer_recipient_hide_bcc_field(view);
	}
	view->bcc_added = to_be_showed;

	debug_leave();
}

void composer_recipient_show_contact_button(void *data, Evas_Object *recp_layout, Evas_Object *btn)
{
	debug_enter();

	evas_object_show(btn);
	elm_object_part_content_set(recp_layout, "ec.swallow.button", btn);
	elm_layout_signal_emit(recp_layout, "ec,state,button,show", "");

	debug_leave();
}

void composer_recipient_hide_contact_button(void *data, Evas_Object *entry)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;
	Evas_Object *recp_layout = NULL;
	Evas_Object *btn = NULL;

	if (view->recp_to_entry.entry == entry) {
		recp_layout = view->recp_to_layout;
		btn = view->recp_to_btn;
	} else if (view->recp_cc_entry.entry == entry) {
		recp_layout = view->recp_cc_layout;
		btn = view->recp_cc_btn;
	} else if (view->recp_bcc_entry.entry == entry) {
		recp_layout = view->recp_bcc_layout;
		btn = view->recp_bcc_btn;
	}

	if (recp_layout) {
		elm_object_part_content_unset(recp_layout, "ec.swallow.button");
		evas_object_hide(btn);
		elm_layout_signal_emit(recp_layout, "ec,state,button,hide", "");
	}

	debug_leave();
}

void composer_recipient_unfocus_entry(void *data, Evas_Object *entry)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	if (view->recp_to_entry.entry == entry) {
		composer_recipient_reset_entry_without_mbe(view->composer_box, view->recp_to_mbe_layout, view->recp_to_layout, view->recp_to_box, view->recp_to_label);
		composer_recipient_update_display_string(view, view->recp_to_mbe, view->recp_to_entry.entry , view->recp_to_display_entry.entry , view->to_recipients_cnt);
		composer_recipient_change_entry(EINA_FALSE, view->recp_to_box, &view->recp_to_entry, &view->recp_to_display_entry, view->recp_to_entry_layout, view->recp_to_display_entry_layout);
	} else if (view->recp_cc_entry.entry == entry) {
		composer_recipient_reset_entry_without_mbe(view->composer_box, view->recp_cc_mbe_layout, view->recp_cc_layout, view->recp_cc_box, view->bcc_added ? view->recp_cc_label_cc : view->recp_cc_label_cc_bcc);
		composer_recipient_update_display_string(view, view->recp_cc_mbe, view->recp_cc_entry.entry, view->recp_cc_display_entry.entry, view->cc_recipients_cnt);
		composer_recipient_change_entry(EINA_FALSE, view->recp_cc_box, &view->recp_cc_entry, &view->recp_cc_display_entry, view->recp_cc_entry_layout, view->recp_cc_display_entry_layout);
	} else if (view->recp_bcc_entry.entry == entry) {
		composer_recipient_reset_entry_without_mbe(view->composer_box, view->recp_bcc_mbe_layout, view->recp_bcc_layout, view->recp_bcc_box, view->recp_bcc_label);
		composer_recipient_update_display_string(view, view->recp_bcc_mbe, view->recp_bcc_entry.entry , view->recp_bcc_display_entry.entry , view->bcc_recipients_cnt);
		composer_recipient_change_entry(EINA_FALSE, view->recp_bcc_box, &view->recp_bcc_entry, &view->recp_bcc_display_entry, view->recp_bcc_entry_layout, view->recp_bcc_display_entry_layout);
	}
	composer_recipient_hide_contact_button(view, entry);

	debug_leave();
}

void composer_recipient_update_display_string(EmailComposerView *view, Evas_Object *mbe, Evas_Object *entry, Evas_Object *display_entry, int count)
{
	debug_enter();

	char recp_string[BUF_LEN_L] = { 0, };
	char final_string[BUF_LEN_L] = { 0, };
	char *recp_suffix = NULL;
	char *entry_ellipsised_string = NULL;

	Elm_Object_Item *item = elm_multibuttonentry_first_item_get(mbe);
	if (item) {
		if (count == 1) {
			snprintf(recp_string, sizeof(recp_string), "%s", elm_object_item_text_get(item));
		} else {
			recp_suffix = g_strdup_printf("+%d", count - 1);
			snprintf(recp_string, sizeof(recp_string), "%1$s %2$s", elm_object_item_text_get(item), recp_suffix);
		}
	}

	const char *entry_string = elm_entry_entry_get(entry);
	if (entry_string && (strlen(entry_string) > 0)) {
		if (strlen(recp_string) > 0) {
			elm_entry_entry_set(display_entry, recp_string);
			elm_entry_entry_append(display_entry, ", ");
			elm_entry_entry_append(display_entry, entry_string);
			entry_ellipsised_string = composer_util_get_ellipsised_entry_name(view, display_entry, elm_entry_entry_get(display_entry));
		} else {
			entry_ellipsised_string = composer_util_get_ellipsised_entry_name(view, display_entry, entry_string);
		}
	} else if (strlen(recp_string) > 0) {
		entry_ellipsised_string = composer_util_get_ellipsised_entry_name(view, display_entry, recp_string);
	}

	if (entry_ellipsised_string) {
		char *found = NULL;
		if (recp_suffix) {
			found = g_strstr_len(entry_ellipsised_string, strlen(entry_ellipsised_string), recp_suffix);
		}

		if (found) {
			strncpy(final_string, entry_ellipsised_string, strlen(entry_ellipsised_string) - strlen(recp_suffix));
			snprintf(final_string + strlen(final_string), sizeof(final_string) - strlen(final_string), "<color=#%02x%02x%02x%02x>%s</color>", COLOR_BLACK, recp_suffix);
			elm_entry_entry_set(display_entry, final_string);
		} else {
			elm_entry_entry_set(display_entry, entry_ellipsised_string);
		}
		g_free(entry_ellipsised_string);
	} else {
		elm_entry_entry_set(display_entry, "");
	}

	FREE(recp_suffix);

	debug_leave();
}

void composer_recipient_change_entry(Eina_Bool to_editable, Evas_Object *recp_box, email_editfield_t *editfield,
								email_editfield_t *display_editfield, Evas_Object *entry_layout, Evas_Object *display_entry_layout)
{
	debug_enter();

	if (to_editable) {
		if (composer_util_is_object_packed_in(recp_box, display_entry_layout)) {
			evas_object_hide(display_entry_layout);
			evas_object_hide(display_editfield->layout);
			elm_object_part_content_unset(entry_layout, "ec.swallow.content");
			elm_box_unpack(recp_box, display_entry_layout);
		}
		if (!composer_util_is_object_packed_in(recp_box, entry_layout)) {
			elm_object_part_content_set(entry_layout, "ec.swallow.content", editfield->layout);
			elm_box_pack_end(recp_box, entry_layout);
			evas_object_show(editfield->layout);
			evas_object_show(entry_layout);
		}
	} else {
		if (composer_util_is_object_packed_in(recp_box, entry_layout)) {
			evas_object_hide(entry_layout);
			evas_object_hide(editfield->layout);
			elm_object_part_content_unset(entry_layout, "ec.swallow.content");
			elm_box_unpack(recp_box, entry_layout);
		}
		if (!composer_util_is_object_packed_in(recp_box, display_entry_layout)) {
			elm_object_part_content_set(display_entry_layout, "ec.swallow.content", display_editfield->layout);
			elm_box_pack_end(recp_box, display_entry_layout);
			evas_object_show(display_editfield->layout);
			evas_object_show(display_entry_layout);
		}
	}

	debug_leave();
}

void composer_recipient_reset_entry_with_mbe(Evas_Object *composer_box, Evas_Object *mbe_layout, Evas_Object *mbe, Evas_Object *recp_layout, Evas_Object *recp_box, Evas_Object *label)
{
	debug_enter();

	if (composer_util_is_object_packed_in(recp_box, label)) {
		elm_box_unpack(recp_box, label);
		evas_object_hide(label);
		elm_layout_signal_emit(recp_layout, "ec,state,left_padding,hide", "");
		edje_object_message_signal_process(_EDJ(recp_layout));
	}
	if (!composer_util_is_object_packed_in(composer_box, mbe_layout)) {
		evas_object_show(mbe_layout);
		elm_box_pack_before(composer_box, mbe_layout, recp_layout);
	}
	elm_multibuttonentry_expanded_set(mbe, EINA_TRUE);

	debug_leave();
}

void composer_recipient_reset_entry_without_mbe(Evas_Object *composer_box, Evas_Object *mbe_layout, Evas_Object *recp_layout, Evas_Object *recp_box, Evas_Object *label)
{
	debug_enter();

	if (composer_util_is_object_packed_in(composer_box, mbe_layout)) {
		elm_box_unpack(composer_box, mbe_layout);
		evas_object_hide(mbe_layout);
	}
	if (!composer_util_is_object_packed_in(recp_box, label)) {
		evas_object_show(label);
		elm_box_pack_start(recp_box, label);
		elm_layout_signal_emit(recp_layout, "ec,state,left_padding,show", "");
		edje_object_message_signal_process(_EDJ(recp_layout));
	}

	debug_leave();
}

static Eina_Bool _recipient_timer_duplicate_toast_cb(void *data)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	view->timer_duplicate_toast_in_reciepients = NULL;
	view->prev_toast_error = 0;

	debug_leave();
	return ECORE_CALLBACK_CANCEL;
}

void composer_recipient_display_error_string(void *data, MBE_VALIDATION_ERROR err)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	if (!view->timer_duplicate_toast_in_reciepients || (view->prev_toast_error != (int)err)) {
		DELETE_TIMER_OBJECT(view->timer_duplicate_toast_in_reciepients);
		view->prev_toast_error = (int)err;
		view->timer_duplicate_toast_in_reciepients = ecore_timer_add(3.0f, _recipient_timer_duplicate_toast_cb, view);

		_recipient_notify_invalid_recipient(view, err);
	}

	debug_leave();
}

Eina_Bool composer_recipient_mbe_validate_email_address_list(void *data, Evas_Object *obj, char *address, char **invalid_list, MBE_VALIDATION_ERROR *ret_err)
{
	debug_enter();

	retvm_if(!address, MBE_VALIDATION_ERROR_INVALID_ADDRESS, "Invalid parameter: address is NULL!");
	retvm_if(!invalid_list, MBE_VALIDATION_ERROR_INVALID_ADDRESS, "Invalid parameter: invalid_list is NULL!");
	retvm_if(!ret_err, MBE_VALIDATION_ERROR_INVALID_ADDRESS, "Invalid parameter: ret_err is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;
	Eina_Bool ret = EINA_TRUE;
	char *invalid_addr = NULL;
	gchar **split_email = g_strsplit_set(address, ",;", -1);
	guint len = g_strv_length(split_email);
	guint i;

	if (len > 1) {
		for (i = 0; i < len; i++) {
			if (split_email[i] && (strlen(split_email[i]) == 0))
				continue;

			MBE_VALIDATION_ERROR err = _recipient_mbe_validate_email_address(view, obj, split_email[i]);
			if (err == MBE_VALIDATION_ERROR_NONE) {
				EmailRecpInfo *ri = composer_util_recp_make_recipient_info(split_email[i]);
				if (ri) {
					char *markup_name = elm_entry_utf8_to_markup(ri->display_name);
					elm_multibuttonentry_item_append(obj, markup_name, NULL, ri);
					FREE(markup_name);
				} else {
					debug_secure_error("ri is NULL! email_address:(%s)", split_email[i]);
				}
			} else {
				if (err == MBE_VALIDATION_ERROR_QUOTATIONS) {
					if (view->recipient_added_from_contacts) {
						char *nick = NULL;
						char *addr = NULL;
						char *email_addr = NULL;

						email_get_recipient_info(address, &nick, &addr);

						if (nick)
							email_addr = g_strconcat(nick, "<", addr, ">", NULL);
						else
							email_addr = g_strconcat(addr, NULL);
						char *markup_name = elm_entry_utf8_to_markup(email_addr);
						elm_multibuttonentry_item_append(obj, markup_name, NULL, NULL);
						FREE(markup_name);
						g_free(email_addr);
						g_free(nick);
						g_free(addr);
					}
				} else if (err > MBE_VALIDATION_ERROR_DUPLICATE_ADDRESS) {
					if (invalid_addr) {
						char *temp = g_strconcat(invalid_addr, ";", split_email[i], NULL);
						g_free(invalid_addr);
						invalid_addr = temp;
					} else {
						invalid_addr = g_strdup(split_email[i]);
					}
				}

				if (err > *ret_err) {
					*ret_err = err;
				}
			}
		}

		ret = EINA_FALSE;
	} else {
		MBE_VALIDATION_ERROR err = _recipient_mbe_validate_email_address(view, obj, address);
		if (err != MBE_VALIDATION_ERROR_NONE) {
			if (err == MBE_VALIDATION_ERROR_QUOTATIONS) {
				if (view->recipient_added_from_contacts) {
					char *nick = NULL;
					char *addr = NULL;
					char *email_addr = NULL;

					email_get_recipient_info(address, &nick, &addr);

					if (nick)
						email_addr = g_strconcat(nick, "<", addr, ">", NULL);
					else
						email_addr = g_strconcat(addr, NULL);
					char *markup_name = elm_entry_utf8_to_markup(email_addr);
					elm_multibuttonentry_item_append(obj, markup_name, NULL, NULL);
					FREE(markup_name);
					g_free(email_addr);
					g_free(nick);
					g_free(addr);
					ret = EINA_FALSE;
				}
			} else if (err > MBE_VALIDATION_ERROR_DUPLICATE_ADDRESS) {
				invalid_addr = g_strdup(address);
				ret = EINA_FALSE;
			} else {
				ret = EINA_FALSE;
			}
			*ret_err = err;
		}
	}
	g_strfreev(split_email);

	if (invalid_addr) {
		*invalid_list = invalid_addr;
	}

	debug_leave();
	return ret;
}

Eina_Bool composer_recipient_commit_recipient_on_entry(void *data, Evas_Object *entry)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;
	Eina_Bool ret = EINA_TRUE;

	const char *str = elm_entry_entry_get(entry);
	if (str && (strlen(str) > 0)) {
		Evas_Object *mbe = _recipient_get_mbe_with_entry(view, entry);
		Elm_Object_Item *it = elm_multibuttonentry_item_append(mbe, str, NULL, NULL);
		if (it) {
			elm_entry_entry_set(entry, "");
		} else {
			ret = EINA_FALSE;
		}
	}

	debug_leave();
	return ret;
}

Eina_Bool composer_recipient_is_recipient_entry(void *data, Evas_Object *entry)
{
	retvm_if(!data, EINA_FALSE, "Invalid parameter: data is NULL!");
	retvm_if(!entry, EINA_FALSE, "Invalid parameter: entry is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;
	Eina_Bool ret = EINA_FALSE;

	if ((entry == view->recp_to_entry.entry) || (entry == view->recp_cc_entry.entry) || (entry == view->recp_bcc_entry.entry)) {
		ret = EINA_TRUE;
	}

	return ret;
}

static Evas_Object *_recipient_get_mbe_with_entry(void *data, Evas_Object *entry)
{
	retvm_if(!data, NULL, "Invalid parameter: data is NULL!");
	retvm_if(!entry, NULL, "Invalid parameter: entry is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;
	Evas_Object *mbe = NULL;

	if (entry == view->recp_to_entry.entry) {
		mbe = view->recp_to_mbe;
	} else if (entry == view->recp_cc_entry.entry) {
		mbe = view->recp_cc_mbe;
	} else if (entry == view->recp_bcc_entry.entry) {
		mbe = view->recp_bcc_mbe;
	}

	return mbe;
}

#ifdef _ALWAYS_CC_MYSELF
void composer_recipient_append_myaddress(void *data, email_account_t *account)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	Evas_Object *dest_mbe = NULL;
	EmailRecpInfo *ri = composer_util_recp_make_recipient_info_with_display_name(account->user_email_address, account->account_name);
	retm_if(!ri, "ri is NULL!");

	ri->is_always_bcc = true;

	if (account->options.add_my_address_to_bcc == EMAIL_ADD_MY_ADDRESS_OPTION_ALWAYS_ADD_TO_CC) {
		dest_mbe = view->recp_cc_mbe;
	} else {
		dest_mbe = view->recp_bcc_mbe;
	}

	Elm_Object_Item *mbe_item = elm_multibuttonentry_first_item_get(dest_mbe);
	char *markup_name = elm_entry_utf8_to_markup(ri->display_name);
	if (mbe_item) {
		elm_multibuttonentry_item_insert_before(dest_mbe, mbe_item, markup_name, NULL, ri);
	} else {
		elm_multibuttonentry_item_append(dest_mbe, markup_name, NULL, ri);
	}
	FREE(markup_name);

	debug_leave();
}

void composer_recipient_remove_myaddress(Evas_Object *mbe, const char *myaddress)
{
	debug_enter();

	retm_if(!mbe, "Invalid parameter: mbe is NULL!");
	retm_if(!myaddress, "Invalid parameter: myaddress is NULL!");

	Elm_Object_Item *item = elm_multibuttonentry_first_item_get(mbe);
	while (item) {
		EmailRecpInfo *ri = (EmailRecpInfo *)elm_object_item_data_get(item);
		if (!ri) {
			debug_error("ri is NULL!");
			continue;
		}
		EmailAddressInfo *ai = (EmailAddressInfo *)eina_list_nth(ri->email_list, ri->selected_email_idx);
		if (!ai) {
			debug_error("ri is NULL!");
			continue;
		}

		if ((ri->is_always_bcc == EINA_TRUE) && (g_strcmp0(myaddress, ai->address) == 0)) {
			elm_object_item_del(item);
			break;
		}
		item = elm_multibuttonentry_item_next_get(item);
	}

	debug_leave();
}
#endif

void composer_recipient_label_text_set(Evas_Object *obj, COMPOSER_RECIPIENT_TYPE_E recp_type) {

	char recp_string[BUF_LEN_M] = { 0, };
	_recipient_get_header_string(recp_type, recp_string, sizeof(recp_string));
	elm_object_text_set(obj, recp_string);
}
