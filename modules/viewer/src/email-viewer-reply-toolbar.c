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

#include "email-engine.h"
#include "email-debug.h"
#include "email-viewer.h"
#include "email-viewer-util.h"

static Elm_Genlist_Item_Class reply_popup_itc;
static Elm_Genlist_Item_Class fwd_popup_itc;

/*reply toolbar logic*/
static void _reply_toolbar_reply_btn_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static void _reply_toolbar_fwd_btn_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static void _reply_toolbar_do_reply(EmailViewerView *view);
static void _reply_toolbar_do_fwd(EmailViewerView *view);
void _reply_toolbar_launch_composer(EmailViewerView *view, int type, Eina_Bool is_attach_included);

/*reply options popup*/
static void _reply_toolbar_create_reply_options_popup(EmailViewerView *view);
static char *_reply_toolbar_reply_options_popup_gl_text_get(void *data, Evas_Object *obj, const char *part);
static void _reply_toolbar_reply_options_popup_gl_select_cb(void *data, Evas_Object *obj, void *event_info);
static void _reply_toolbar_reply_option_popup_cancel_cb(void *data, Evas_Object *obj, void *event_info);

/*fwd options popup*/
static void _reply_toolbar_create_fwd_options_popup(EmailViewerView *view);
static char *_reply_toolbar_fwd_options_popup_gl_text_get(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_reply_toolbar_fwd_options_popup_gl_content_get(void *data, Evas_Object *obj, const char *part);
static void _reply_toolbar_fwd_options_popup_gl_select_cb(void *data, Evas_Object *obj, void *event_info);
static void _reply_toolbar_fwd_options_popup_checkbox_change_cb(void *data, Evas_Object *obj, void *event_info);
static void _reply_toolbar_fwd_option_popup_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _reply_toolbar_fwd_option_popup_ok_cb(void *data, Evas_Object *obj, void *event_info);
static void _reply_toolbar_fwd_option_popup_cancel_cb(void *data, Evas_Object *obj, void *event_info);

/*Warning popup*/
static Eina_Bool _reply_toolbar_is_mail_fully_loaded(EmailViewerView *view);
static void _reply_toolbar_warning_popup_cancel_cb(void *data, Evas_Object *obj, void *event_info);
static void _reply_toolbar_warning_popup_continue_fwd_cb(void *data, Evas_Object *obj, void *event_info);
static void _reply_toolbar_warning_popup_continue_reply_cb(void *data, Evas_Object *obj, void *event_info);



/*Reply and Forward popup*/
static email_string_t EMAIL_VIEWER_STRING_NULL = { NULL, NULL };
static email_string_t EMAIL_VIEWER_BUTTON_CANCEL = { PACKAGE, "IDS_EMAIL_BUTTON_CANCEL" };
static email_string_t EMAIL_VIEWER_BUTTON_CONTINUE = { PACKAGE, "IDS_EMAIL_BUTTON_CONTINUE_ABB" };
static email_string_t EMAIL_VIEWER_BUTTON_OK = { PACKAGE, "IDS_EMAIL_BUTTON_OK" };
static email_string_t EMAIL_VIEWER_HEADER_DOWNLOAD_ENTIRE_EMAIL = { PACKAGE, "IDS_EMAIL_HEADER_DOWNLOAD_ENTIRE_EMAIL_ABB" };
static email_string_t EMAIL_VIEWER_POP_PARTIAL_BODY_DOWNLOADED_REST_LOST = { PACKAGE, "IDS_EMAIL_POP_ONLY_PART_OF_THE_MESSAGE_HAS_BEEN_DOWNLOADED_IF_YOU_CONTINUE_THE_REST_OF_THE_MESSAGE_MAY_BE_LOST" };
static email_string_t EMAIL_VIEWER_HEADER_SELECT_ACTION = { PACKAGE, "IDS_EMAIL_HEADER_SELECT_ACTION_ABB" };
static email_string_t EMAIL_VIEWER_HEADER_FWD_EMAIL = { PACKAGE, "IDS_EMAIL_HEADER_FORWARD_EMAIL_ABB" };

/*Reply and Forward buttons*/
static email_string_t EMAIL_VIEWER_BUTTON_REPLY = { PACKAGE, "IDS_EMAIL_OPT_REPLY" };
static email_string_t EMAIL_VIEWER_BUTTON_FORWARD = { PACKAGE, "IDS_EMAIL_OPT_FORWARD" };

enum {
	REPLY_SINGLE_OPTION_INDEX = 1,
	REPLY_ALL_OPTION_INDEX,
};

enum {
	FWD_NEW_RECIPIENT_INDEX = 1,
	FWD_ALL_RECIPIENT_INDEX,
	FWD_WITH_ATTACHMENT_INDEX
};

void reply_toolbar_create_view(EmailViewerView *view)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	if (view->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX) {
		debug_log("Reply options for current mail not supported!");
		return;
	}

	view->reply_toolbar_ly = viewer_load_edj(view->base.content, email_get_common_theme_path(), "two_bottom_btn", EVAS_HINT_EXPAND, 0.0);

	Evas_Object *reply_btn = elm_button_add(view->reply_toolbar_ly);
	elm_object_style_set(reply_btn, "bottom");
	evas_object_smart_callback_add(reply_btn, "clicked", _reply_toolbar_reply_btn_clicked_cb, view);
	elm_object_domain_translatable_text_set(reply_btn, EMAIL_VIEWER_BUTTON_REPLY.domain, EMAIL_VIEWER_BUTTON_REPLY.id);
	elm_layout_content_set(view->reply_toolbar_ly, "btn1.swallow", reply_btn);

	Evas_Object *fwd_btn = elm_button_add(view->reply_toolbar_ly);
	elm_object_style_set(fwd_btn, "bottom");
	evas_object_smart_callback_add(fwd_btn, "clicked", _reply_toolbar_fwd_btn_clicked_cb, view);
	elm_object_domain_translatable_text_set(fwd_btn, EMAIL_VIEWER_BUTTON_FORWARD.domain, EMAIL_VIEWER_BUTTON_FORWARD.id);
	elm_layout_content_set(view->reply_toolbar_ly, "btn2.swallow", fwd_btn);

	elm_object_item_part_content_set(view->base.navi_item, "toolbar", view->reply_toolbar_ly);
	debug_leave();
}

static void _reply_toolbar_reply_btn_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	if (view->to_recipients_cnt + view->cc_recipients_cnt + view->bcc_recipients_cnt > 1) {
		debug_log("Reply all is possible");
		_reply_toolbar_create_reply_options_popup(view);
	} else {
		debug_log("Reply all is not possible");

		view->selected_reply_type = EMAIL_VIEWER_REPLY_SINGLE;
		if (!_reply_toolbar_is_mail_fully_loaded(view)) {
			util_create_notify(view, EMAIL_VIEWER_HEADER_DOWNLOAD_ENTIRE_EMAIL, EMAIL_VIEWER_POP_PARTIAL_BODY_DOWNLOADED_REST_LOST, 2,
					EMAIL_VIEWER_BUTTON_CANCEL, _reply_toolbar_warning_popup_cancel_cb,
					EMAIL_VIEWER_BUTTON_CONTINUE, _reply_toolbar_warning_popup_continue_reply_cb, NULL);
		} else {
			_reply_toolbar_do_reply(view);
		}

	}

	debug_leave();
}

static void _reply_toolbar_fwd_btn_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;
	_reply_toolbar_create_fwd_options_popup(view);
}

static void _reply_toolbar_create_reply_options_popup(EmailViewerView *view)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	DELETE_EVAS_OBJECT(view->con_popup);
	DELETE_EVAS_OBJECT(view->notify);

	view->reply_option_popup = util_create_notify_with_list(view, EMAIL_VIEWER_HEADER_SELECT_ACTION, EMAIL_VIEWER_STRING_NULL, 1, EMAIL_VIEWER_BUTTON_CANCEL,
			_reply_toolbar_reply_option_popup_cancel_cb, EMAIL_VIEWER_STRING_NULL, NULL, NULL);

	reply_popup_itc.item_style = "type1";
	reply_popup_itc.func.text_get = _reply_toolbar_reply_options_popup_gl_text_get;
	reply_popup_itc.func.content_get = NULL;
	reply_popup_itc.func.state_get = NULL;
	reply_popup_itc.func.del = NULL;

	Evas_Object *reply_popup_genlist = util_notify_genlist_add(view->reply_option_popup);
	evas_object_data_set(reply_popup_genlist, VIEWER_EVAS_DATA_NAME, view);

	elm_genlist_item_append(reply_popup_genlist,
			&reply_popup_itc,
			(void *)(ptrdiff_t)REPLY_SINGLE_OPTION_INDEX,
			NULL,
			ELM_GENLIST_ITEM_NONE,
			_reply_toolbar_reply_options_popup_gl_select_cb,
			(void *)(ptrdiff_t)REPLY_SINGLE_OPTION_INDEX);

	elm_genlist_item_append(reply_popup_genlist,
			&reply_popup_itc,
			(void *)(ptrdiff_t)REPLY_ALL_OPTION_INDEX,
			NULL,
			ELM_GENLIST_ITEM_NONE,
			_reply_toolbar_reply_options_popup_gl_select_cb,
			(void *)(ptrdiff_t)REPLY_ALL_OPTION_INDEX);

	elm_object_content_set(view->reply_option_popup, reply_popup_genlist);
	evas_object_show(view->reply_option_popup);

	debug_leave();
}

static char *_reply_toolbar_reply_options_popup_gl_text_get(void *data, Evas_Object *obj, const char *part)
{
	retvm_if(!data, NULL, "Callback data is NULL!");
	retvm_if(!obj, NULL, "Callback data is NULL!");

	EmailViewerView *view = evas_object_data_get(obj, VIEWER_EVAS_DATA_NAME);
	retvm_if(!view, NULL, "Callback data is NULL!");

	if (!strcmp(part, "elm.text")) {
		retvm_if(!data, NULL, "Callback data is NULL!");

		int option_index = (int)(ptrdiff_t)data;
		switch (option_index) {
		case REPLY_SINGLE_OPTION_INDEX:
			return strdup(_("IDS_EMAIL_OPT_REPLY"));
		case REPLY_ALL_OPTION_INDEX:
			return strdup(_("IDS_EMAIL_OPT_REPLY_ALL"));
		default:
			return NULL;
		}
	}
	return NULL;
}

static void _reply_toolbar_reply_options_popup_gl_select_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data || !obj || !event_info, "Callback data is NULL!");
	Elm_Object_Item *item = event_info;
	elm_genlist_item_selected_set(item, EINA_FALSE);

	EmailViewerView *view = evas_object_data_get(obj, VIEWER_EVAS_DATA_NAME);
	retm_if(!view, "Callback data is NULL!");

	int option_index = (int)(ptrdiff_t)data;

	if (option_index == REPLY_SINGLE_OPTION_INDEX) {
		view->selected_reply_type = EMAIL_VIEWER_REPLY_SINGLE;
	} else if (option_index == REPLY_ALL_OPTION_INDEX) {
		view->selected_reply_type = EMAIL_VIEWER_REPLY_ALL;
	} else {
		debug_error("Unsupported option!");
		return;
	}

	if (!_reply_toolbar_is_mail_fully_loaded(view)) {
		util_create_notify(view, EMAIL_VIEWER_HEADER_DOWNLOAD_ENTIRE_EMAIL, EMAIL_VIEWER_POP_PARTIAL_BODY_DOWNLOADED_REST_LOST, 2,
				EMAIL_VIEWER_BUTTON_CANCEL, _reply_toolbar_warning_popup_cancel_cb,
				EMAIL_VIEWER_BUTTON_CONTINUE, _reply_toolbar_warning_popup_continue_reply_cb, NULL);
	}

	_reply_toolbar_do_reply(data);

	debug_leave();
}

static void _reply_toolbar_do_reply(EmailViewerView *view)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	if (view->viewer_type == EML_VIEWER) {
		if (view->selected_reply_type == EMAIL_VIEWER_REPLY_SINGLE) {
			_reply_toolbar_launch_composer(view, RUN_EML_REPLY, EINA_FALSE);
		} else {
			_reply_toolbar_launch_composer(view, RUN_EML_REPLY_ALL, EINA_FALSE);
		}
	} else {
		if (view->selected_reply_type == EMAIL_VIEWER_REPLY_SINGLE) {
			_reply_toolbar_launch_composer(view, RUN_COMPOSER_REPLY, EINA_FALSE);
		} else {
			_reply_toolbar_launch_composer(view, RUN_COMPOSER_REPLY_ALL, EINA_FALSE);
		}
	}

	debug_leave();
}

static void _reply_toolbar_create_fwd_options_popup(EmailViewerView *view)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	DELETE_EVAS_OBJECT(view->con_popup);
	DELETE_EVAS_OBJECT(view->notify);

	fwd_popup_itc.item_style = "type1";
	fwd_popup_itc.func.text_get = _reply_toolbar_fwd_options_popup_gl_text_get;
	fwd_popup_itc.func.content_get = _reply_toolbar_fwd_options_popup_gl_content_get;
	fwd_popup_itc.func.state_get = NULL;
	fwd_popup_itc.func.del = NULL;

	view->fwd_option_popup = util_create_notify_with_list(view, EMAIL_VIEWER_HEADER_FWD_EMAIL, EMAIL_VIEWER_STRING_NULL, 2, EMAIL_VIEWER_BUTTON_CANCEL,
			_reply_toolbar_fwd_option_popup_cancel_cb, EMAIL_VIEWER_BUTTON_OK, _reply_toolbar_fwd_option_popup_ok_cb, NULL);

	evas_object_event_callback_add(view->fwd_option_popup, EVAS_CALLBACK_DEL,
				_reply_toolbar_fwd_option_popup_del_cb, view);

	Evas_Object *fwd_popup_genlist = util_notify_genlist_add(view->fwd_option_popup);
	evas_object_data_set(fwd_popup_genlist, VIEWER_EVAS_DATA_NAME, view);

	view->fwd_popup_data = MEM_ALLOC(view->fwd_popup_data, 1);
	view->fwd_popup_data->radio_group = elm_radio_add(fwd_popup_genlist);
	elm_radio_value_set(view->fwd_popup_data->radio_group, FWD_NEW_RECIPIENT_INDEX);

	elm_genlist_item_append(fwd_popup_genlist,
			&fwd_popup_itc,
			(void *)(ptrdiff_t)FWD_NEW_RECIPIENT_INDEX,
			NULL,
			ELM_GENLIST_ITEM_NONE,
			_reply_toolbar_fwd_options_popup_gl_select_cb,
			(void *)(ptrdiff_t)FWD_NEW_RECIPIENT_INDEX);

	elm_genlist_item_append(fwd_popup_genlist,
			&fwd_popup_itc,
			(void *)(ptrdiff_t)FWD_ALL_RECIPIENT_INDEX,
			NULL,
			ELM_GENLIST_ITEM_NONE,
			_reply_toolbar_fwd_options_popup_gl_select_cb,
			(void *)(ptrdiff_t)FWD_ALL_RECIPIENT_INDEX);

	if (view->has_attachment) {
		view->fwd_popup_data->is_add_attach_checked = EINA_TRUE;
		elm_genlist_item_append(fwd_popup_genlist,
				&fwd_popup_itc,
				(void *)(ptrdiff_t)FWD_WITH_ATTACHMENT_INDEX,
				NULL,
				ELM_GENLIST_ITEM_NONE,
				_reply_toolbar_fwd_options_popup_gl_select_cb,
				(void *)(ptrdiff_t)FWD_WITH_ATTACHMENT_INDEX);
	}

	elm_object_content_set(view->fwd_option_popup, fwd_popup_genlist);
	evas_object_show(view->fwd_option_popup);

	debug_leave();
}

static void _reply_toolbar_fwd_option_popup_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "Callback data is NULL!");

	EmailViewerView *view = (EmailViewerView *)data;
	FREE(view->fwd_popup_data);

	debug_leave();
}

static void _reply_toolbar_fwd_options_popup_gl_select_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data || !obj || !event_info, "Callback data is NULL!");

	Elm_Object_Item *item = event_info;
	elm_genlist_item_selected_set(item, EINA_FALSE);

	EmailViewerView *view = evas_object_data_get(obj, VIEWER_EVAS_DATA_NAME);
	retm_if(!view, "Callback data is NULL!");

	FwdOptionPopupData *fwd_popup_data = view->fwd_popup_data;
	int option_index = (int)(ptrdiff_t)data;

	if (option_index == FWD_NEW_RECIPIENT_INDEX || option_index == FWD_ALL_RECIPIENT_INDEX) {
		elm_radio_value_set(fwd_popup_data->radio_group, option_index);
	} else if (option_index == FWD_WITH_ATTACHMENT_INDEX) {
		fwd_popup_data->is_add_attach_checked = !elm_check_state_get(fwd_popup_data->add_attach_checkbox);
		elm_check_state_set(fwd_popup_data->add_attach_checkbox, fwd_popup_data->is_add_attach_checked);
	}

	debug_leave();
}

static char *_reply_toolbar_fwd_options_popup_gl_text_get(void *data, Evas_Object *obj, const char *part)
{
	retvm_if(!data, NULL, "Callback data is NULL!");
	retvm_if(!obj, NULL, "Callback data is NULL!");

	EmailViewerView *view = evas_object_data_get(obj, VIEWER_EVAS_DATA_NAME);
	retvm_if(!view, NULL, "Callback data is NULL!");

	if (!strcmp(part, "elm.text")) {
		retvm_if(!data, NULL, "Callback data is NULL!");

		int option_index = (int)(ptrdiff_t)data;
		switch (option_index) {
		case FWD_NEW_RECIPIENT_INDEX:
			return strdup(_("IDS_EMAIL_OPT_NEW_RECIPIENTS_ONLY_ABB2"));
		case FWD_ALL_RECIPIENT_INDEX:
			return strdup(_("IDS_EMAIL_OPT_INCLUDE_PREVIOUS_RECIPIENTS_ABB"));
		case FWD_WITH_ATTACHMENT_INDEX:
			return strdup(_("IDS_EMAIL_OPT_INCLUDE_ATTACHMENTS_ABB2"));
		default:
			return NULL;
		}
	}
	return NULL;
}

static Evas_Object *_reply_toolbar_fwd_options_popup_gl_content_get(void *data, Evas_Object *obj, const char *part)
{
	retvm_if(!data, NULL, "Callback data is NULL!");
	retvm_if(!obj, NULL, "Callback data is NULL!");

	EmailViewerView *view = evas_object_data_get(obj, VIEWER_EVAS_DATA_NAME);
	retvm_if(!view, NULL, "Callback data is NULL!");

	FwdOptionPopupData *fwd_popup_data = view->fwd_popup_data;
	int option_index = (int)(ptrdiff_t)data;

	if (option_index == FWD_NEW_RECIPIENT_INDEX || option_index == FWD_ALL_RECIPIENT_INDEX) {
		if (!strcmp(part, "elm.swallow.end")) {
			Evas_Object *radio = elm_radio_add(obj);
			elm_radio_group_add(radio, fwd_popup_data->radio_group);
			elm_radio_state_value_set(radio, option_index);
			evas_object_propagate_events_set(radio, EINA_FALSE);
			return radio;
		}
	} else if (option_index == FWD_WITH_ATTACHMENT_INDEX) {
		if (!strcmp(part, "elm.swallow.icon")) {
			fwd_popup_data->add_attach_checkbox = elm_check_add(obj);
			evas_object_propagate_events_set(fwd_popup_data->add_attach_checkbox, EINA_FALSE);
			elm_check_state_set(fwd_popup_data->add_attach_checkbox, fwd_popup_data->is_add_attach_checked);
			evas_object_smart_callback_add(fwd_popup_data->add_attach_checkbox, "changed", _reply_toolbar_fwd_options_popup_checkbox_change_cb, fwd_popup_data);
			return fwd_popup_data->add_attach_checkbox;
		}
	}
	return NULL;
}

static void _reply_toolbar_fwd_options_popup_checkbox_change_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	FwdOptionPopupData *fwd_popup_data = (FwdOptionPopupData *)data;
	fwd_popup_data->is_add_attach_checked = elm_check_state_get(obj);

	debug_leave();
}

static void _reply_toolbar_fwd_option_popup_ok_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "Callback data is NULL!");
	EmailViewerView *view = (EmailViewerView *)data;

	if (!_reply_toolbar_is_mail_fully_loaded(view)) {
		util_create_notify(view, EMAIL_VIEWER_HEADER_DOWNLOAD_ENTIRE_EMAIL, EMAIL_VIEWER_POP_PARTIAL_BODY_DOWNLOADED_REST_LOST, 2,
				EMAIL_VIEWER_BUTTON_CANCEL, _reply_toolbar_warning_popup_cancel_cb,
				EMAIL_VIEWER_BUTTON_CONTINUE, _reply_toolbar_warning_popup_continue_fwd_cb, NULL);
	} else {
		_reply_toolbar_do_fwd(data);
	}

	debug_leave();
}

static void _reply_toolbar_do_fwd(EmailViewerView *view)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	if (view->viewer_type == EML_VIEWER) {
		_reply_toolbar_launch_composer(view, RUN_EML_FORWARD, EINA_TRUE);
	} else {
		int option_type = elm_radio_value_get(view->fwd_popup_data->radio_group);
		Eina_Bool is_attach_included = view->fwd_popup_data->is_add_attach_checked;

		if (option_type == FWD_NEW_RECIPIENT_INDEX) {
			_reply_toolbar_launch_composer(view, RUN_COMPOSER_FORWARD, is_attach_included);
		} else if (option_type == FWD_ALL_RECIPIENT_INDEX) {
			_reply_toolbar_launch_composer(view, RUN_COMPOSER_FORWARD_ALL, is_attach_included);
		} else {
			DELETE_EVAS_OBJECT(view->fwd_option_popup);
			debug_error("Unsupported option!");
		}
	}

	debug_leave();
}

static void _reply_toolbar_warning_popup_continue_reply_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	DELETE_EVAS_OBJECT(view->notify);
	_reply_toolbar_do_reply(view);
	debug_leave();
}

static void _reply_toolbar_warning_popup_continue_fwd_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	DELETE_EVAS_OBJECT(view->notify);
	_reply_toolbar_do_fwd(data);
	debug_leave();
}

static Eina_Bool _reply_toolbar_is_mail_fully_loaded(EmailViewerView *view)
{
	bool is_body_fully_downloaded = viewer_check_body_download_status(view->body_download_status, EMAIL_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED);

	if (!is_body_fully_downloaded) {
		debug_log("Email body not fully loaded!");
		return EINA_FALSE;
	} else if (view->has_inline_image) {
		email_attachment_data_t *attach_data = view->attachment_info;
		int attach_count = view->attachment_count;

		int i;
		for (i = 0; i < attach_count; i++) {
			if (attach_data[i].inline_content_status && !attach_data[i].save_status) {
				debug_log("Inline attachment is not loaded!");
				return EINA_FALSE;
			}
		}
	}

	return EINA_TRUE;
}

static void _reply_toolbar_warning_popup_cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;
	DELETE_EVAS_OBJECT(view->notify);
	DELETE_EVAS_OBJECT(view->fwd_option_popup);
	DELETE_EVAS_OBJECT(view->reply_option_popup);

	debug_leave();
}

static void _reply_toolbar_reply_option_popup_cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;
	DELETE_EVAS_OBJECT(view->reply_option_popup);

	debug_leave();
}

static void _reply_toolbar_fwd_option_popup_cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;
	DELETE_EVAS_OBJECT(view->fwd_option_popup);

	debug_leave();
}

void _reply_toolbar_launch_composer(EmailViewerView *view, int type, Eina_Bool is_attach_included)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");
	retm_if(view->mail_info == NULL, "Invalid parameter: mail_info[NULL]");

	DELETE_EVAS_OBJECT(view->fwd_option_popup);
	DELETE_EVAS_OBJECT(view->reply_option_popup);

	email_params_h params = NULL;
	int send_with_attach = 0;
	if (is_attach_included) {
		send_with_attach = 1;
	} else {
		send_with_attach = 0;
	}
	if (email_params_create(&params) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_RUN_TYPE, type) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_ACCOUNT_ID, view->account_id) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_MAILBOX, view->mailbox_id) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_MAIL_ID, view->mail_id) &&
		((type != RUN_EML_REPLY && type != RUN_EML_REPLY_ALL && type != RUN_EML_FORWARD) ||
		email_params_add_str(params, EMAIL_BUNDLE_KEY_MYFILE_PATH, view->eml_file_path))) {
		email_params_add_int(params, EMAIL_BUNDLE_KEY_IS_ATTACHMENT_INCLUDE_TO_FORWARD, send_with_attach);
		view->loaded_module = viewer_create_module(view, EMAIL_MODULE_COMPOSER, params);
	}

	email_params_free(&params);

	debug_leave();
}
