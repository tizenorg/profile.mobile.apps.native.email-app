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
#include "email-debug.h"
#include "email-viewer.h"
#include "email-viewer-util.h"
#include "email-engine.h"
#include "email-viewer-recipient.h"
#include "email-viewer-more-menu-callback.h"
#include "email-viewer-more-menu.h"

/*
 * Declaration for static functions
 */

static void _viewer_more_menu_ctxpopup_back_cb(void *data, Evas_Object *obj, void *event_info);
static int _viewer_launch_contacts_for_add_update(
		EmailViewerView *view,
		const char *operation,
		const char *contact_data,
		const char *contact_name);

/*
 * Definition for exported functions
 */

static int _viewer_launch_contacts_for_add_update(
		EmailViewerView *view,
		const char *operation,
		const char *contact_data,
		const char *contact_name)
{
	const char *ext_data_key = NULL;

	switch (view->create_contact_arg) {
	case CONTACTUI_REQ_ADD_PHONE_NUMBER:
		ext_data_key = EMAIL_CONTACT_EXT_DATA_PHONE;
		break;
	case CONTACTUI_REQ_ADD_EMAIL:
		ext_data_key = EMAIL_CONTACT_EXT_DATA_EMAIL;
		break;
	case CONTACTUI_REQ_ADD_URL:
		ext_data_key = EMAIL_CONTACT_EXT_DATA_URL;
		break;
	default:
		debug_error("not needed value %d", view->create_contact_arg);
		break;
	}

	int ret = -1;
	email_params_h params = NULL;

	if (email_params_create(&params) &&
		email_params_set_operation(params, operation) &&
		email_params_set_mime(params, EMAIL_CONTACT_MIME_SCHEME) &&
		(!ext_data_key ||
		email_params_add_str(params, ext_data_key, contact_data)) &&
		(!contact_name ||
		email_params_add_str(params, EMAIL_CONTACT_EXT_DATA_NAME, contact_name))) {

		ret = email_module_launch_app(view->base.module, EMAIL_LAUNCH_APP_AUTO, params, NULL);
	}

	email_params_free(&params);

	if (ret != 0) {
		debug_error("email_module_launch_app failed: %d", ret);
		return -1;
	}
	return 0;
}

void viewer_add_contact(void *data, char *contact_data, char *contact_name)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	retm_if(contact_data == NULL, "Invalid parameter: contact_address[NULL]");
	debug_secure("contact_address (%s)", contact_data);

	EmailViewerView *view = (EmailViewerView *)data;

	int ret = _viewer_launch_contacts_for_add_update(
			view,
			APP_CONTROL_OPERATION_ADD,
			contact_data,
			contact_name);
	if (ret != 0) {
		debug_error("_viewer_launch_contacts_for_add_update failed: %d", ret);
	}

	debug_leave();
}

void viewer_update_contact(void *data, char *contact_data)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	retm_if(contact_data == NULL, "Invalid parameter: contact_data[NULL]");
	debug_secure("contact_data (%s)", contact_data);

	EmailViewerView *view = (EmailViewerView *)data;

	int ret = _viewer_launch_contacts_for_add_update(
			view,
			APP_CONTROL_OPERATION_EDIT,
			contact_data,
			NULL);
	if (ret != 0) {
		debug_error("_viewer_launch_contacts_for_add_update failed: %d", ret);
	}

	debug_leave();
}

static Elm_Object_Item *_add_ctxpopup_menu_item(EmailViewerView *view, const char *str, Evas_Object *icon, Evas_Smart_Cb cb)
{
	Elm_Object_Item *ctx_menu_item = NULL;

	ctx_menu_item = elm_ctxpopup_item_append(view->con_popup, str, icon, cb, view);
	elm_object_item_domain_text_translatable_set(ctx_menu_item, PACKAGE, EINA_TRUE);
	return ctx_menu_item;
}

static Elm_Object_Item *_viewer_more_menu_add_item(Evas_Object *obj, const char *file, const char *label, Evas_Smart_Cb cb, void *data)
{
	Elm_Object_Item *ctx_menu_item = NULL;

	Evas_Object *icon = NULL;
	ctx_menu_item = _add_ctxpopup_menu_item(data, label, icon, cb);

	return ctx_menu_item;
}

static void _viewer_add_to_spam_menu_add(EmailViewerView *view)
{
	bool add_spam = (view->mailbox_type != EMAIL_MAILBOX_TYPE_SPAMBOX);

	const char *str = NULL;
	Evas_Smart_Cb cb = NULL;
	if (add_spam) {
		str = "IDS_EMAIL_OPT_MOVE_TO_SPAMBOX";
		cb = viewer_ctxpopup_add_to_spambox_cb;
	} else {
		str = "IDS_EMAIL_OPT_REMOVE_FROM_SPAMBOX_ABB";
		cb = viewer_ctxpopup_remove_from_spambox_cb;
	}

	Elm_Object_Item *ctx_menu_item = NULL;
	ctx_menu_item = elm_ctxpopup_item_append(view->con_popup, str, NULL, cb, view);
	elm_object_item_domain_text_translatable_set(ctx_menu_item, PACKAGE, EINA_TRUE);
}

void viewer_more_menu_move_ctxpopup(Evas_Object *ctxpopup, Evas_Object *win)
{
	debug_enter();

	Evas_Coord x, y, w, h;
	int pos = -1;

	elm_win_screen_size_get(win, &x, &y, &w, &h);
	pos = elm_win_rotation_get(win);

	switch (pos) {
	case 0:
	case 180:
		evas_object_move(ctxpopup, (w / 2), h);
		break;
	case 90:
		evas_object_move(ctxpopup, (h / 2), w);
		break;
	case 270:
		evas_object_move(ctxpopup, (h / 2), w);
		break;
	default:
		break;
	}

	debug_leave();
}

void viewer_create_more_ctxpopup(EmailViewerView *view)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	Evas_Object *icon = NULL;
	Elm_Object_Item *ctx_menu_item = NULL;

	DELETE_EVAS_OBJECT(view->con_popup);
	view->con_popup = elm_ctxpopup_add(view->base.module->win);
	retm_if(view->con_popup == NULL, "cannot create context popup: view->con_popup[NULL]");

	elm_ctxpopup_auto_hide_disabled_set(view->con_popup, EINA_TRUE);
	elm_object_style_set(view->con_popup, "more/default");
	elm_ctxpopup_direction_priority_set(view->con_popup,
							ELM_CTXPOPUP_DIRECTION_UP,
							ELM_CTXPOPUP_DIRECTION_UNKNOWN,
							ELM_CTXPOPUP_DIRECTION_UNKNOWN,
							ELM_CTXPOPUP_DIRECTION_UNKNOWN);

	evas_object_smart_callback_add(view->con_popup, "dismissed", viewer_more_ctxpopup_dismissed_cb, view);
	evas_object_event_callback_add(view->con_popup, EVAS_CALLBACK_DEL, viewer_more_menu_ctxpopup_del_cb, view);
	evas_object_event_callback_add(view->base.module->navi, EVAS_CALLBACK_RESIZE, viewer_more_menu_window_resized_cb, view);
	eext_object_event_callback_add(view->con_popup, EEXT_CALLBACK_BACK, _viewer_more_menu_ctxpopup_back_cb, NULL);
	eext_object_event_callback_add(view->con_popup, EEXT_CALLBACK_MORE, _viewer_more_menu_ctxpopup_back_cb, NULL);
	debug_log("mailbox_type:%d", view->mailbox_type);

	/* get save_status of mail_data */
	email_mail_data_t *mail_info = NULL;
	email_engine_get_mail_data(view->mail_id, &mail_info);

	if (view->viewer_type == EMAIL_VIEWER) {
		if (mail_info != NULL) {
			if (view->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX) {
				if (mail_info->save_status == EMAIL_MAIL_STATUS_SEND_CANCELED || mail_info->save_status == EMAIL_MAIL_STATUS_SEND_FAILURE) {
					_add_ctxpopup_menu_item(view, "IDS_EMAIL_OPT_SEND", icon, viewer_resend_cb);

					_add_ctxpopup_menu_item(view, "IDS_EMAIL_OPT_EDIT", icon, viewer_edit_email_cb);
				} else if (mail_info->save_status == EMAIL_MAIL_STATUS_SEND_SCHEDULED) {
					_add_ctxpopup_menu_item(view, "IDS_EMAIL_OPT_EDIT", icon, viewer_edit_scheduled_email_cb);
				} else if (mail_info->save_status == EMAIL_MAIL_STATUS_SENDING || mail_info->save_status == EMAIL_MAIL_STATUS_SEND_WAIT) {
					view->cancel_sending_ctx_item = _add_ctxpopup_menu_item(view, "IDS_EMAIL_OPT_CANCEL_SENDING_ABB", icon, viewer_cancel_send_cb);
					if (view->is_cancel_sending_btn_clicked == EINA_TRUE) {
						elm_object_item_disabled_set(view->cancel_sending_ctx_item, EINA_TRUE);
					}
				}
			}

			debug_log("view->save_status:%d, mail_info->save_status:%d", view->save_status, mail_info->save_status);
			if ((mail_info->save_status != EMAIL_MAIL_STATUS_SENDING && mail_info->save_status != EMAIL_MAIL_STATUS_SEND_WAIT
				&& mail_info->save_status != EMAIL_MAIL_STATUS_SEND_SCHEDULED && mail_info->save_status != EMAIL_MAIL_STATUS_SEND_DELAYED)) {
				_viewer_more_menu_add_item(view->con_popup, NULL, "IDS_EMAIL_OPT_DELETE", _delete_cb, (void *)view);
			}
		} else {
			debug_log("fail to get mail data");
		}

		if (view->mailbox_type != EMAIL_MAILBOX_TYPE_OUTBOX) {
			_add_ctxpopup_menu_item(view, "IDS_EMAIL_OPT_MARK_AS_UNREAD_ABB", icon, viewer_mark_as_unread_cb);
		}

		debug_log("mailbox_type:%d, save_status:%d", view->mailbox_type, view->save_status);
		if ((view->mailbox_type != EMAIL_MAILBOX_TYPE_SENTBOX) && !(view->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX)) {
			_viewer_more_menu_add_item(view->con_popup, NULL, "IDS_EMAIL_OPT_MOVE", _move_cb, (void *)view);
		}
	}

	if (mail_info != NULL && mail_info->save_status != EMAIL_MAIL_STATUS_SENDING && mail_info->save_status != EMAIL_MAIL_STATUS_SEND_WAIT) {
		email_contact_list_info_t *contact_list_item = email_contacts_get_contact_info_by_email_address(view->sender_address);
			if (contact_list_item) {
				debug_log("Sender address is listed in contacts DB, person ID:%d", contact_list_item->person_id);
				ctx_menu_item = elm_ctxpopup_item_append(view->con_popup, "IDS_EMAIL_OPT_VIEW_CONTACT_DETAILS_ABB",
						icon, viewer_ctxpopup_detail_contact_cb, (void *)(ptrdiff_t)contact_list_item->person_id);
			} else {
				debug_log("Sender address is not listed in contacts DB");
				ctx_menu_item = elm_ctxpopup_item_append(view->con_popup, "IDS_EMAIL_OPT_ADD_TO_CONTACTS_ABB2", icon, viewer_ctxpopup_add_contact_cb, view);
			}
			email_contacts_delete_contact_info(&contact_list_item);
	}

	elm_object_item_domain_text_translatable_set(ctx_menu_item, PACKAGE, EINA_TRUE);

	if (view->mailbox_type != EMAIL_MAILBOX_TYPE_OUTBOX) {
		_viewer_add_to_spam_menu_add(view);
	}

	if (view->mailbox_type != EMAIL_MAILBOX_TYPE_OUTBOX) {
		if (!recipient_is_priority_email_address(view->sender_address)) {
			ctx_menu_item = elm_ctxpopup_item_append(view->con_popup, "IDS_EMAIL_OPT_ADD_TO_PRIORITY_SENDERS_ABB", icon, viewer_ctxpopup_add_vip_rule_cb, view->sender_address);
		} else {
			ctx_menu_item = elm_ctxpopup_item_append(view->con_popup, "IDS_EMAIL_OPT_REMOVE_FROM_PRIORITY_SENDERS", icon, viewer_ctxpopup_remove_vip_rule_cb, view->sender_address);
		}
		elm_object_item_domain_text_translatable_set(ctx_menu_item, PACKAGE, EINA_TRUE);
	}

	email_engine_free_mail_data_list(&mail_info, 1);

	viewer_more_menu_move_ctxpopup(view->con_popup, view->base.module->win);

	evas_object_show(view->con_popup);
	debug_leave();
}

static void _viewer_more_menu_ctxpopup_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	if (obj) {
		elm_ctxpopup_dismiss(obj);
	}

}
