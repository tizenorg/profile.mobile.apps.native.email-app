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

#include "email-mailbox.h"
#include "email-mailbox-toolbar.h"
#include "email-mailbox-list.h"
#include "email-mailbox-list-other-items.h"
#include "email-mailbox-sync.h"
#include "email-mailbox-title.h"
#include "email-mailbox-ug-util.h"
#include "email-mailbox-util.h"
#include "email-mailbox-more-menu.h"


/*
 * Declaration for static functions
 */

static void _viewer_result_cb(void *data, email_module_h module, app_control_h result);
static void _account_result_cb(void *data, email_module_h module, app_control_h result);

/*
 * Definitions
 */

/*
 * Structures
 */

/*
 * Globals
 */


/*
 * Definition for static functions
 */

static void _viewer_result_cb(void *data, email_module_h module, app_control_h result)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailMailboxUGD *mailbox_ugd = data;
	char *msg_type = NULL;
	int ret = 0;

	ret = app_control_get_extra_data(result, EMAIL_BUNDLE_KEY_MSG, &msg_type);
	retm_if(ret != APP_CONTROL_ERROR_NONE, "app_control_get_extra_data() failed! ret: [%d]", ret);

	if (g_strcmp0(msg_type, EMAIL_BUNDLE_VAL_NEXT_MSG) == 0) {
		mailbox_handle_next_msg_bundle(mailbox_ugd, result);
	} else if (g_strcmp0(msg_type, EMAIL_BUNDLE_VAL_PREV_MSG) == 0) {
		mailbox_handle_prev_msg_bundle(mailbox_ugd, result);
	}

	debug_log("mailbox_ugd->opened_mail_id : %d", mailbox_ugd->opened_mail_id);

	G_FREE(msg_type);
}

static void _account_result_cb(void *data, email_module_h module, app_control_h result)
{
	debug_enter();
	retm_if(!module, "module is NULL");
	retm_if(!data, "data is NULL");

	char *mode = NULL;
	char *account_id = NULL;
	char *mailbox_id = NULL;
	char *mailbox_type = NULL;
	int ret;
	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;

	if (mailbox_ugd->account)
		mailbox_account_module_destroy(mailbox_ugd, mailbox_ugd->account);

	/* handle result from move to folder ug */
	char *temp = NULL;
	int is_move_mail_ug = 0;

	app_control_get_extra_data(result, EMAIL_BUNDLE_KEY_IS_MAILBOX_MOVE_UG, &temp);
	is_move_mail_ug = atoi(temp);
	G_FREE(temp);

	if (is_move_mail_ug) {
		int b_edit_mod = 0;
		app_control_get_extra_data(result, EMAIL_BUNDLE_KEY_IS_MAILBOX_EDIT_MODE, &temp);
		b_edit_mod = atoi(temp);
		G_FREE(temp);
		app_control_get_extra_data(result, EMAIL_BUNDLE_KEY_MAILBOX, &temp);
		mailbox_ugd->mailbox_id = (int)atoi(temp);
		G_FREE(temp);
		app_control_get_extra_data(result, EMAIL_BUNDLE_KEY_MAILBOX_MOVE_STATUS, &temp);
		mailbox_ugd->move_status = (int)atoi(temp);
		G_FREE(temp);
		app_control_get_extra_data(result, EMAIL_BUNDLE_KEY_MAILBOX_MOVED_MAILBOX_NAME, &temp);
		G_FREE(mailbox_ugd->moved_mailbox_name);
		mailbox_ugd->moved_mailbox_name = g_strdup(temp);
		G_FREE(temp);

		if (b_edit_mod) {
			mailbox_exit_edit_mode(mailbox_ugd);
		}
		return;
	}


	ret = app_control_get_extra_data(result, EMAIL_BUNDLE_KEY_ACCOUNT_TYPE, &mode);
	debug_log("app_control_get_extra_data: %d", ret);

	mailbox_clear_prev_mailbox_info(mailbox_ugd);

	if (g_strcmp0(mode, EMAIL_BUNDLE_VAL_ALL_ACCOUNT) == 0) {
		debug_log("EMAIL_BUNDLE_VAL_ALL_ACCOUNT");

		mailbox_ugd->mode = EMAIL_MAILBOX_MODE_ALL;
		mailbox_ugd->account_id = 0;

		ret = app_control_get_extra_data(result, EMAIL_BUNDLE_KEY_MAILBOX_TYPE, &(mailbox_type));
		debug_log("app_control_get_extra_data: %d", ret);
		mailbox_ugd->mailbox_type = (gint) atoi(mailbox_type);

		debug_log("account_id(%d), mailbox_type(%d)", mailbox_ugd->account_id, mailbox_ugd->mailbox_type);

	} else if (g_strcmp0(mode, EMAIL_BUNDLE_VAL_SINGLE_ACCOUNT) == 0) {
		debug_log("EMAIL_BUNDLE_VAL_SINGLE_ACCOUNT");

		mailbox_ugd->mode = EMAIL_MAILBOX_MODE_MAILBOX;

		ret = app_control_get_extra_data(result, EMAIL_BUNDLE_KEY_ACCOUNT_ID, &account_id);
		debug_log("app_control_get_extra_data: %d", ret);
		mailbox_ugd->account_id = (gint) atoi(account_id);

		ret = app_control_get_extra_data(result, EMAIL_BUNDLE_KEY_MAILBOX, &mailbox_id);
		debug_log("app_control_get_extra_data: %d", ret);
		if (mailbox_id) {
			mailbox_ugd->mailbox_id = (int)atoi(mailbox_id);
			G_FREE(mailbox_id);
		} else {
			debug_error("failure on getting mailbox_id");
			G_FREE(mode);
			G_FREE(account_id);
			return;
		}

		debug_log("account_id[%d], mailbox_id[%d]", mailbox_ugd->account_id, mailbox_ugd->mailbox_id);

	} else if (g_strcmp0(mode, EMAIL_BUNDLE_VAL_PRIORITY_SENDER) == 0) {
		debug_log("EMAIL_BUNDLE_VAL_PRIORITY_SENDER");

		mailbox_ugd->account_id = 0;
		mailbox_ugd->mailbox_type = EMAIL_MAILBOX_TYPE_INBOX;
		mailbox_ugd->mode = EMAIL_MAILBOX_MODE_PRIORITY_SENDER;
	} else {
		G_FREE(mode);
		G_FREE(account_id);
		G_FREE(mailbox_type);
		return;
	}

	mailbox_view_title_update_all(mailbox_ugd);
	mailbox_check_sort_type_validation(mailbox_ugd);
	mailbox_list_refresh(mailbox_ugd, NULL);

	mailbox_sync_current_mailbox(mailbox_ugd);
	/*if (mailbox_sync_current_mailbox(mailbox_ugd))
		mailbox_refreshing_progress_add(mailbox_ugd);
	else
		mailbox_refreshing_progress_remove(mailbox_ugd);*/

	G_FREE(mode);
	G_FREE(account_id);
	G_FREE(mailbox_type);
}

/*
 * Definition for exported functions
 */

email_module_h mailbox_composer_module_create(void *data, email_module_type_e type, app_control_h service)
{
	debug_enter();

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;

	email_module_listener_t listener = { 0 };
	listener.cb_data = mailbox_ugd;
	listener.destroy_request_cb = mailbox_composer_module_destroy;

	email_module_h module = email_module_create_child(mailbox_ugd->base.module, type, service, &listener);
	if (mailbox_ugd->content_layout)
		elm_object_tree_focus_allow_set(mailbox_ugd->content_layout, EINA_FALSE);

	debug_leave();

	return module;
}

void mailbox_composer_module_destroy(void *priv, email_module_h module)
{
	debug_enter();
	retm_if(!module, "module is NULL");
	retm_if(!priv, "priv is NULL");

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)priv;

	GList *opened_mail = g_list_find_custom(mailbox_ugd->mail_list, GINT_TO_POINTER(mailbox_ugd->opened_mail_id), mailbox_compare_mailid_in_list);
	if (opened_mail) {
		MailItemData *ld = (MailItemData *)g_list_nth_data(opened_mail, 0);
		if (ld) {
			if (elm_genlist_item_selected_get(ld->item) == EINA_TRUE)
				elm_genlist_item_selected_set(ld->item, EINA_FALSE);
		}
	}
	if (mailbox_ugd->content_layout)
		elm_object_tree_focus_allow_set(mailbox_ugd->content_layout, EINA_TRUE);

	if (mailbox_ugd->composer) {
		email_module_destroy(mailbox_ugd->composer);
		mailbox_ugd->composer = NULL;
	}
	mailbox_ugd->is_module_launching = false;

	if (mailbox_ugd->run_type == RUN_VIEWER_FROM_NOTIFICATION) {
		mailbox_ugd->run_type = RUN_TYPE_UNKNOWN;
		mailbox_open_email_viewer(mailbox_ugd, mailbox_ugd->account_id, mailbox_ugd->mailbox_id, mailbox_ugd->start_mail_id);
	}
}

email_module_h mailbox_viewer_module_create(void *data, email_module_type_e type, app_control_h service)
{
	debug_enter();

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;

	email_module_listener_t listener = { 0 };
	listener.cb_data = data;
	listener.result_cb = _viewer_result_cb;
	listener.destroy_request_cb = mailbox_viewer_module_destroy;

	if (!mailbox_ugd->viewer) {
		mailbox_ugd->viewer = email_module_create_child(mailbox_ugd->base.module, type, service, &listener);
		if (mailbox_ugd->content_layout) {
			elm_object_tree_focus_allow_set(mailbox_ugd->content_layout, EINA_FALSE);
		}
	} else {
		email_module_send_message(mailbox_ugd->viewer, service);
	}

	return mailbox_ugd->viewer;
}

void mailbox_viewer_module_destroy(void *priv, email_module_h module)
{
	debug_enter();
	retm_if(!module, "module is NULL");
	retm_if(!priv, "priv is NULL");

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)priv;
	if (mailbox_ugd) {

		if (mailbox_ugd->content_layout)
			elm_object_tree_focus_allow_set(mailbox_ugd->content_layout, EINA_TRUE);
	}

	email_module_destroy(module);
	mailbox_ugd->viewer = NULL;
	mailbox_ugd->opened_mail_id = 0;
}

email_module_h mailbox_account_module_create(void *data, email_module_type_e type, app_control_h service)
{
	debug_enter();

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;

	email_module_listener_t listener = { 0 };
	listener.cb_data = mailbox_ugd;
	listener.result_cb = _account_result_cb;
	listener.destroy_request_cb = mailbox_account_module_destroy;

	email_module_h module = email_module_create_child(mailbox_ugd->base.module, type, service, &listener);
	if (mailbox_ugd->content_layout)
		elm_object_tree_focus_allow_set(mailbox_ugd->content_layout, EINA_FALSE);

	return module;
}

void mailbox_account_module_destroy(void *priv, email_module_h module)
{
	debug_enter();
	retm_if(!module, "module is NULL");
	retm_if(!priv, "priv is NULL");

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)priv;

	if (mailbox_ugd->content_layout)
		elm_object_tree_focus_allow_set(mailbox_ugd->content_layout, EINA_TRUE);

	if (mailbox_ugd->account) {
		email_module_destroy(mailbox_ugd->account);
		mailbox_ugd->account = NULL;
	}
	mailbox_ugd->is_module_launching = false;
}

email_module_h mailbox_setting_module_create(void *data, email_module_type_e type, app_control_h service)
{
	debug_enter();

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;

	email_module_listener_t listener = { 0 };
	listener.cb_data = mailbox_ugd;
	listener.destroy_request_cb = mailbox_setting_module_destroy;

	email_module_h module = email_module_create_child(mailbox_ugd->base.module, type, service, &listener);
	if (mailbox_ugd->content_layout)
		elm_object_tree_focus_allow_set(mailbox_ugd->content_layout, EINA_FALSE);

	return module;
}

void mailbox_setting_module_destroy(void *priv, email_module_h module)
{
	debug_enter();
	retm_if(!module, "module is NULL");
	retm_if(!priv, "priv is NULL");

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)priv;

	if (mailbox_ugd->content_layout)
		elm_object_tree_focus_allow_set(mailbox_ugd->content_layout, EINA_TRUE);

	if (module == mailbox_ugd->setting) {
		email_module_destroy(mailbox_ugd->setting);
		mailbox_ugd->setting = NULL;
	}
}
