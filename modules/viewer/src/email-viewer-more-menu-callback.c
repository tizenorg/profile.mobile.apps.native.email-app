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
#include <system_settings.h>
#include "email-debug.h"
#include "email-engine.h"
#include "email-viewer.h"
#include "email-viewer-js.h"
#include "email-viewer-util.h"
#include "email-viewer-recipient.h"
#include "email-viewer-contents.h"
#include "email-viewer-more-menu.h"
#include "email-viewer-more-menu-callback.h"

#define	EMAIL_VIEWER_STRING_LENGTH	255

/*font size for viewer body*/
#define	EMAIL_VIEWER_TEXT_SCALE_SMALL	0.6f /* 1.3f */		/** small size */
#define	EMAIL_VIEWER_TEXT_SCALE_MEDIUM	1.2f /* 2.3f */		/** medium size */
#define	EMAIL_VIEWER_TEXT_SCALE_LARGE	1.8f /* 4.3f */		/** large size */
#define	EMAIL_VIEWER_TEXT_SCALE_EXTRA_LARGE	2.4f /* 6.3f */		/** extra large size */
#define	EMAIL_VIEWER_TEXT_SCALE_HUGE	3.0f /* 8.0f */		/** huge size */

typedef struct {
	char *key_font_name;
	int font_size;
} EmailViewerFontSizeTable;

typedef enum {
	SYSTEM_FONT_SIZE_INDEX_SMALL = 0,
	SYSTEM_FONT_SIZE_INDEX_MEDIUM,
	SYSTEM_FONT_SIZE_INDEX_LARGE,
	SYSTEM_FONT_SIZE_INDEX_EXTRA_LARGE,
	SYSTEM_FONT_SIZE_INDEX_HUGE,
	SYSTEM_FONT_SIZE_INDEX_MAX
} system_font_size_index;


/*
 * Declaration for static functions
 */

extern EmailViewerView *_g_md;


/*
 * Definition for static functions
 */

/*
 * Definition for exported functions
 */

void viewer_more_menu_window_resized_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailViewerView *view = (EmailViewerView *)data;
	viewer_more_menu_move_ctxpopup(view->con_popup, view->base.module->win);

	debug_leave();
}

void viewer_more_menu_ctxpopup_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailViewerView *view = (EmailViewerView *)data;

	evas_object_smart_callback_del(view->con_popup, "dismissed", viewer_more_ctxpopup_dismissed_cb);
	evas_object_event_callback_del_full(view->base.module->navi, EVAS_CALLBACK_RESIZE, viewer_more_menu_window_resized_cb, view);

	debug_leave();
}

void viewer_ctxpopup_add_contact_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;

	DELETE_EVAS_OBJECT(view->con_popup);

	view->create_contact_arg = CONTACTUI_REQ_ADD_EMAIL;
	recipient_add_to_contact_selection_popup(view, view->sender_address, EMAIL_SENDER_NAME);
	debug_leave();
}

void viewer_ctxpopup_detail_contact_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	retm_if(_g_md == NULL, "Invalid parameter: _g_md[NULL]");

	EmailViewerView *view = _g_md;

	DELETE_EVAS_OBJECT(view->con_popup);
	debug_secure("data(%s)", data);
	viewer_view_contact_detail(view, (intptr_t)data);
	debug_leave();
}

void viewer_ctxpopup_add_vip_rule_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	retm_if(_g_md == NULL, "Invalid parameter: _g_md[NULL]");

	EmailViewerView *view = _g_md;
	char *addr = (char *)data;

	debug_secure("VIP address: %s", addr);
	DELETE_EVAS_OBJECT(view->con_popup);

	int add_ok = 0;
	int apply_ok = 0;
	email_rule_t *rule = NULL;

	/* add filtering rule */
	rule = MEM_ALLOC(rule, 1);
	rule->account_id = 0;
	rule->type = EMAIL_PRIORITY_SENDER;
	rule->value2 = g_strdup(addr);
	rule->faction = EMAIL_FILTER_MOVE;
	rule->target_mailbox_id = 0;
	rule->flag1 = 1;
	rule->flag2 = RULE_TYPE_EXACTLY;

	add_ok = email_engine_add_rule(rule);
	if (!add_ok) {
		debug_warning("email_engine_add_rule failed");
	} else {
		debug_secure("email_engine_add_rule success: %s", rule->filter_name);
		apply_ok = email_engine_apply_rule(rule->filter_id);
		if (!apply_ok)
			debug_warning("email_engine_apply_rule failed");
		else
			debug_log("email_engine_apply_rule success");
		email_engine_free_rule_list(&rule, 1);
	}

	if (!add_ok || !apply_ok) {
		notification_status_message_post(_("IDS_EMAIL_HEADER_UNABLE_TO_PERFORM_ACTION_ABB"));
	} else {
		int ret = email_update_vip_rule_value();
		if (ret != 0) {
			debug_log("email_update_viprule_value failed. err=%d", ret);
			notification_status_message_post(_("IDS_EMAIL_HEADER_UNABLE_TO_PERFORM_ACTION_ABB"));
		} else {
			notification_status_message_post(_("IDS_EMAIL_TPOP_EMAIL_ADDRESS_ADDED_TO_PRIORITY_SENDERS"));

			if (g_strcmp0(view->sender_address, addr) == 0) {
				if (view->priority_sender_image) {
					elm_object_part_content_unset(view->sender_ly, "sender.priority.icon");
					DELETE_EVAS_OBJECT(view->priority_sender_image);
				}
				Evas_Object *priority_image = elm_layout_add(view->sender_ly);
				elm_layout_file_set(priority_image, email_get_common_theme_path(), EMAIL_IMAGE_ICON_PRIO_SENDER);
				elm_object_part_content_set(view->sender_ly, "sender.priority.icon", priority_image);
				elm_layout_signal_emit(view->sender_ly, "set.priority.sender", "elm");
				evas_object_show(priority_image);
				view->priority_sender_image = priority_image;
			}
		}
	}
	debug_leave();
}

void viewer_ctxpopup_remove_vip_rule_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	retm_if(_g_md == NULL, "Invalid parameter: _g_md[NULL]");

	EmailViewerView *view = _g_md;
	char *addr = (char *)data;

	debug_secure("VIP address: %s", addr);
	DELETE_EVAS_OBJECT(view->con_popup);

	int count, i;
	gboolean ok = FALSE;
	email_rule_t *rule_list = NULL;

	/* get the rule list from service */
	ok = email_engine_get_rule_list(&rule_list, &count);
	if (!ok) {
		debug_log("email_engine_get_rule_list failed");
	} else {
		if (count > 0) {
			for (i = 0; i < count; i++) {
				if (rule_list[i].type == EMAIL_PRIORITY_SENDER) {
					/*debug_secure("vip address %s", rule_list[i].value2);*/
					if (g_strcmp0(rule_list[i].value2, addr) == 0) {
						debug_secure("[%s] already set", rule_list[i].value2);
						ok = email_engine_delete_rule(rule_list[i].filter_id) && ok;
					}
				}
			}
		}
		/* free email rule_list */
		email_engine_free_rule_list(&rule_list, count);
	}

	if (!ok) {
		debug_log("operation failed!");
		notification_status_message_post(_("IDS_EMAIL_HEADER_UNABLE_TO_PERFORM_ACTION_ABB"));
	} else {
		int ret = email_update_vip_rule_value();
		if (ret != 0) {
			debug_log("preference_set_int failed. err=%d", ret);
			notification_status_message_post(_("IDS_EMAIL_HEADER_UNABLE_TO_PERFORM_ACTION_ABB"));
		} else {
			notification_status_message_post(_("IDS_EMAIL_TPOP_EMAIL_ADDRESS_REMOVED_FROM_PRIORITY_SENDERS"));

			if (g_strcmp0(view->sender_address, addr) == 0) {
				elm_layout_signal_emit(view->sender_ly, "remove.priority.sender", "elm");
				elm_object_part_content_unset(view->sender_ly, "sender.priority.icon");
				DELETE_EVAS_OBJECT(view->priority_sender_image);
			}
		}
	}
	debug_leave();
}

void viewer_ctxpopup_add_to_spambox_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;

	DELETE_EVAS_OBJECT(view->con_popup);

	if (view->viewer_type == EML_VIEWER) {
		debug_log("eml viewer. return here");
		return;
	}

	debug_log("mailbox_id(%d), mailbox_type(%d)", view->mailbox_id, view->mailbox_type);
	if (view->mailbox_type == EMAIL_MAILBOX_TYPE_SPAMBOX) {
		debug_log("original folder is already spambox.");
	} else {
		int ret = EMAIL_ERROR_NONE;
		ret = viewer_move_email(view, _find_folder_id_using_folder_type(view, EMAIL_MAILBOX_TYPE_SPAMBOX), FALSE, TRUE);

		if (ret == 1) {
			char str[EMAIL_VIEWER_STRING_LENGTH] = {0, };
			snprintf(str, sizeof(str), email_get_email_string("IDS_EMAIL_TPOP_1_EMAIL_MOVED_TO_PS"), email_get_email_string("IDS_EMAIL_HEADER_SPAMBOX"));
			notification_status_message_post(str);
			debug_log("email_module_make_destroy_request");
			email_module_make_destroy_request(view->base.module);
		} else {
			notification_status_message_post(_("IDS_EMAIL_HEADER_UNABLE_TO_PERFORM_ACTION_ABB"));
			debug_log("move failed. ret[%d]", ret);
		}
	}
	debug_leave();
}

void viewer_ctxpopup_remove_from_spambox_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;

	DELETE_EVAS_OBJECT(view->con_popup);

	if (view->viewer_type == EML_VIEWER) {
		debug_log("eml viewer. return here");
		return;
	}

	if (view->mailbox_type != EMAIL_MAILBOX_TYPE_SPAMBOX) {
		debug_log("original folder is already spambox.");
	} else {
		int ret = EMAIL_ERROR_NONE;
		ret = viewer_move_email(view, _find_folder_id_using_folder_type(view, EMAIL_MAILBOX_TYPE_INBOX), FALSE, TRUE);

		if (ret == 1) {
			char str[EMAIL_VIEWER_STRING_LENGTH] = {0, };
			snprintf(str, sizeof(str), email_get_email_string("IDS_EMAIL_TPOP_1_EMAIL_MOVED_TO_PS"), email_get_email_string("IDS_EMAIL_HEADER_INBOX"));
			notification_status_message_post(str);
			debug_log("email_module_make_destroy_request");
			email_module_make_destroy_request(view->base.module);
		} else {
			notification_status_message_post(_("IDS_EMAIL_HEADER_UNABLE_TO_PERFORM_ACTION_ABB"));
			debug_log("move failed. ret[%d]", ret);
		}
	}
	debug_leave();
}

void viewer_more_ctxpopup_dismissed_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;
	DELETE_EVAS_OBJECT(view->con_popup);

	debug_leave();
}

void viewer_mark_as_unread_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;

	DELETE_EVAS_OBJECT(view->con_popup);

	debug_log("view->mail_id(%d), mail_info->mail_id(%d)", view->mail_id, view->mail_info->mail_id);

	if (!email_engine_set_flags_field(view->account_id, &view->mail_id, 1, EMAIL_FLAGS_SEEN_FIELD, 0, 1)) {
		notification_status_message_post(_("IDS_EMAIL_HEADER_UNABLE_TO_PERFORM_ACTION_ABB"));
	} else {
		notification_status_message_post(_("IDS_EMAIL_TPOP_EMAIL_MARKED_AS_UNREAD"));
		debug_log("email_module_make_destroy_request");
		email_module_make_destroy_request(view->base.module);
	}
	debug_leave();
}

void viewer_edit_email_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;

	DELETE_EVAS_OBJECT(view->con_popup);

	email_params_h params = NULL;

	if (email_params_create(&params) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_RUN_TYPE, RUN_COMPOSER_EDIT) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_ACCOUNT_ID, view->account_id) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_MAILBOX, view->mailbox_id) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_MAIL_ID, view->mail_id)) {

		view->loaded_module = viewer_create_module(view, EMAIL_MODULE_COMPOSER, params);
	}

	email_params_free(&params);

	debug_leave();
}

void viewer_edit_scheduled_email_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;

	debug_log("view->mail_id:%d", view->mail_id);

	email_engine_cancel_sending_mail(view->mail_id);

	DELETE_EVAS_OBJECT(view->con_popup);

	email_params_h params = NULL;

	if (email_params_create(&params) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_RUN_TYPE, RUN_COMPOSER_EDIT) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_ACCOUNT_ID, view->account_id) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_MAILBOX, view->mailbox_id) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_MAIL_ID, view->mail_id)) {

		view->loaded_module = viewer_create_scheduled_composer_module(view, params);
	}

	email_params_free(&params);

	debug_leave();
}

void viewer_cancel_send_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;
	DELETE_EVAS_OBJECT(view->con_popup);

	debug_log("view->account_id(%d), mail_info->account_id(%d)", view->account_id, view->mail_info->account_id);
	debug_log("view->mail_id(%d), mail_info->mail_id(%d)", view->mail_id, view->mail_info->mail_id);

	email_engine_cancel_sending_mail(view->mail_id);
	view->is_cancel_sending_btn_clicked = EINA_TRUE;

	debug_leave();
}

void viewer_resend_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;
	DELETE_EVAS_OBJECT(view->con_popup);

	/* Send email again */
	debug_log("view->account_id(%d), mail_info->account_id(%d)", view->account_id, view->mail_info->account_id);
	debug_log("view->mail_id(%d), mail_info->mail_id(%d)", view->mail_id, view->mail_info->mail_id);

	email_account_t *account = NULL;
	if (email_engine_get_account_data(view->account_id, EMAIL_ACC_GET_OPT_DEFAULT, &account)) {
		email_mail_attribute_value_t mail_attr;

		mail_attr.integer_type_value = (account->auto_resend_times == EMAIL_NO_LIMITED_RETRY_COUNT) ? 10 : account->auto_resend_times;
		email_engine_update_mail_attribute(view->account_id, &view->mail_id, 1, EMAIL_MAIL_ATTRIBUTE_REMAINING_RESEND_TIMES, mail_attr);
		debug_log("mail_id(%d) resend tme(%d)", view->mail_id, mail_attr.integer_type_value);
	} else {
		debug_warning("email_engine_get_account failed! account_id(%d)", view->account_id);
	}

	if (account) {
		email_engine_free_account_list(&account, 1);
		account = NULL;
	}

	int handle = 0;
	email_engine_send_mail(view->mail_id, &handle);
	debug_log("sending handle(%d)", handle);
	debug_leave();
}
