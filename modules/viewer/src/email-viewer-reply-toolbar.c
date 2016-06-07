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

/*reply logic*/
static void _reply_toolbar_reply_btn_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static void _reply_toolbar_forward_btn_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static Eina_Bool _reply_toolbar_is_mail_fully_loaded(EmailViewerView *view);
static void _reply_toolbar_do_reply(EmailViewerView *view);
static void _reply_toolbar_do_reply_all(EmailViewerView *view);
static void _reply_toolbar_do_forward(EmailViewerView *view);
static void _reply_toolbar_popup_response_cb(void *data, Evas_Object *obj, void *event_info);

/*reply options popup*/
static void _reply_toolbar_create_reply_options_popup(EmailViewerView *view);
static char *_reply_toolbar_reply_options_gl_text_get(void *data, Evas_Object *obj, const char *part);
static void _reply_toolbar_reply_option_cb(void *data, Evas_Object *obj, void *event_info);
static void _reply_toolbar_reply_all_option_cb(void *data, Evas_Object *obj, void *event_info);
static void _reply_toolbar_popup_response_continue_reply_cb(void *data, Evas_Object *obj, void *event_info);
static void _reply_toolbar_popup_response_continue_reply_all_cb(void *data, Evas_Object *obj, void *event_info);

/*forward options popup*/
static void _reply_toolbar_popup_response_continue_forward_cb(void *data, Evas_Object *obj, void *event_info);

/*Reply and Forward popup*/
static email_string_t EMAIL_VIEWER_STRING_NULL = { NULL, NULL };
static email_string_t EMAIL_VIEWER_BUTTON_CANCEL = { PACKAGE, "IDS_EMAIL_BUTTON_CANCEL" };
static email_string_t EMAIL_VIEWER_BUTTON_CONTINUE = { PACKAGE, "IDS_EMAIL_BUTTON_CONTINUE_ABB" };
static email_string_t EMAIL_VIEWER_HEADER_DOWNLOAD_ENTIRE_EMAIL = { PACKAGE, "IDS_EMAIL_HEADER_DOWNLOAD_ENTIRE_EMAIL_ABB" };
static email_string_t EMAIL_VIEWER_POP_PARTIAL_BODY_DOWNLOADED_REST_LOST = { PACKAGE, "IDS_EMAIL_POP_ONLY_PART_OF_THE_MESSAGE_HAS_BEEN_DOWNLOADED_IF_YOU_CONTINUE_THE_REST_OF_THE_MESSAGE_MAY_BE_LOST" };
static email_string_t EMAIL_VIEWER_STR_REPLY_OPTIONS = { PACKAGE, "IDS_EMAIL_HEADER_SELECT_ACTION_ABB" };

/*Reply and Forward buttons*/
static email_string_t EMAIL_VIEWER_BUTTON_REPLY = { PACKAGE, "IDS_EMAIL_OPT_REPLY" };
static email_string_t EMAIL_VIEWER_BUTTON_FORWARD = { PACKAGE, "IDS_EMAIL_OPT_FORWARD" };

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

	Evas_Object *forward_btn = elm_button_add(view->reply_toolbar_ly);
	elm_object_style_set(forward_btn, "bottom");
	evas_object_smart_callback_add(forward_btn, "clicked", _reply_toolbar_forward_btn_clicked_cb, view);
	elm_object_domain_translatable_text_set(forward_btn, EMAIL_VIEWER_BUTTON_FORWARD.domain, EMAIL_VIEWER_BUTTON_FORWARD.id);
	elm_layout_content_set(view->reply_toolbar_ly, "btn2.swallow", forward_btn);

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
		_reply_toolbar_do_reply(view);
	}

	debug_leave();
}

static void _reply_toolbar_forward_btn_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;
	//TODO Functionality of create forward options popup will be added later
	_reply_toolbar_do_forward(view);
}

static void _reply_toolbar_create_reply_options_popup(EmailViewerView *view)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	DELETE_EVAS_OBJECT(view->con_popup);

	view->notify = util_create_notify_with_list(view, EMAIL_VIEWER_STR_REPLY_OPTIONS, EMAIL_VIEWER_STRING_NULL, 1, EMAIL_VIEWER_BUTTON_CANCEL,
				_reply_toolbar_popup_response_cb, EMAIL_VIEWER_STRING_NULL, NULL, NULL);

	reply_popup_itc.item_style = "type1";
	reply_popup_itc.func.text_get = _reply_toolbar_reply_options_gl_text_get;
	reply_popup_itc.func.content_get = NULL;
	reply_popup_itc.func.state_get = NULL;
	reply_popup_itc.func.del = NULL;

	Evas_Object *reply_popup_genlist = util_notify_genlist_add(view->notify);

	elm_genlist_item_append(reply_popup_genlist,
			&reply_popup_itc,
			"IDS_EMAIL_OPT_REPLY",
			NULL,
			ELM_GENLIST_ITEM_NONE,
			_reply_toolbar_reply_option_cb,
			view);

	elm_genlist_item_append(reply_popup_genlist,
			&reply_popup_itc,
			"IDS_EMAIL_OPT_REPLY_ALL",
			NULL,
			ELM_GENLIST_ITEM_NONE,
			_reply_toolbar_reply_all_option_cb,
			view);

	elm_object_content_set(view->notify, reply_popup_genlist);
	evas_object_show(view->notify);

	debug_leave();
}

static char *_reply_toolbar_reply_options_gl_text_get(void *data, Evas_Object *obj, const char *part)
{
	debug_enter();
	retvm_if(obj == NULL, NULL, "Invalid parameter: obj[NULL]");

	if (!strcmp(part, "elm.text")) {
		const char *text = data ? _(data) : NULL;
		return text && *text ? strdup(text) : NULL;
	}

	debug_leave();
	return NULL;
}
static void _reply_toolbar_reply_option_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: view[NULL]");

	_reply_toolbar_do_reply(data);
	debug_leave();

}

static void _reply_toolbar_reply_all_option_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: view[NULL]");

	_reply_toolbar_do_reply_all(data);
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

static void _reply_toolbar_do_reply(EmailViewerView *view)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	DELETE_EVAS_OBJECT(view->con_popup);
	DELETE_EVAS_OBJECT(view->notify);

	if (view->viewer_type == EML_VIEWER) {
		viewer_launch_composer(view, RUN_EML_REPLY);
	} else if (!_reply_toolbar_is_mail_fully_loaded(view)) {
		util_create_notify(view, EMAIL_VIEWER_HEADER_DOWNLOAD_ENTIRE_EMAIL, EMAIL_VIEWER_POP_PARTIAL_BODY_DOWNLOADED_REST_LOST, 2,
				EMAIL_VIEWER_BUTTON_CANCEL, _reply_toolbar_popup_response_cb,
				EMAIL_VIEWER_BUTTON_CONTINUE, _reply_toolbar_popup_response_continue_reply_cb, NULL);
	} else {
		viewer_launch_composer(view, RUN_COMPOSER_REPLY);
	}

	debug_leave();
}

static void _reply_toolbar_do_reply_all(EmailViewerView *view)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	DELETE_EVAS_OBJECT(view->con_popup);
	DELETE_EVAS_OBJECT(view->notify);

	if (view->viewer_type == EML_VIEWER) {
		viewer_launch_composer(view, RUN_EML_REPLY_ALL);
	} else if (!_reply_toolbar_is_mail_fully_loaded(view)) {
		util_create_notify(view, EMAIL_VIEWER_HEADER_DOWNLOAD_ENTIRE_EMAIL, EMAIL_VIEWER_POP_PARTIAL_BODY_DOWNLOADED_REST_LOST, 2,
				EMAIL_VIEWER_BUTTON_CANCEL, _reply_toolbar_popup_response_cb,
				EMAIL_VIEWER_BUTTON_CONTINUE, _reply_toolbar_popup_response_continue_reply_all_cb, NULL);
	} else {
		viewer_launch_composer(view, RUN_COMPOSER_REPLY_ALL);
	}
	debug_leave();
}

static void _reply_toolbar_do_forward(EmailViewerView *view)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	DELETE_EVAS_OBJECT(view->con_popup);
	DELETE_EVAS_OBJECT(view->notify);

	if (view->viewer_type == EML_VIEWER) {
		viewer_launch_composer(view, RUN_EML_FORWARD);
	} else if (!_reply_toolbar_is_mail_fully_loaded(view)) {
		util_create_notify(view, EMAIL_VIEWER_HEADER_DOWNLOAD_ENTIRE_EMAIL, EMAIL_VIEWER_POP_PARTIAL_BODY_DOWNLOADED_REST_LOST, 2,
				EMAIL_VIEWER_BUTTON_CANCEL, _reply_toolbar_popup_response_cb,
				EMAIL_VIEWER_BUTTON_CONTINUE, _reply_toolbar_popup_response_continue_forward_cb, NULL);
	} else {
		viewer_launch_composer(view, RUN_COMPOSER_FORWARD);
	}

	debug_leave();
}

static void _reply_toolbar_popup_response_continue_reply_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	DELETE_EVAS_OBJECT(view->notify);

	viewer_launch_composer(view, RUN_COMPOSER_REPLY);
	debug_leave();
}

static void _reply_toolbar_popup_response_continue_reply_all_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	DELETE_EVAS_OBJECT(view->notify);

	viewer_launch_composer(view, RUN_COMPOSER_REPLY_ALL);
	debug_leave();
}

static void _reply_toolbar_popup_response_continue_forward_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	DELETE_EVAS_OBJECT(view->notify);

	viewer_launch_composer(view, RUN_COMPOSER_FORWARD);
	debug_leave();
}

static void _reply_toolbar_popup_response_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;
	DELETE_EVAS_OBJECT(view->notify);
	debug_leave();
}
