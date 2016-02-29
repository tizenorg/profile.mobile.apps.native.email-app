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
#include "email-mailbox-module-util.h"
#include "email-mailbox-util.h"
#include "email-mailbox-more-menu.h"


/*
 * Declaration for static functions
 */

static void _viewer_result_cb(void *data, email_module_h module, email_params_h result);
static void _account_result_cb(void *data, email_module_h module, email_params_h result);

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

static void _viewer_result_cb(void *data, email_module_h module, email_params_h result)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailMailboxView *view = data;
	const char *msg_type = NULL;

	retm_if(!email_params_get_str(result, EMAIL_BUNDLE_KEY_MSG, &msg_type),
			"email_params_get_str() failed!");

	if (g_strcmp0(msg_type, EMAIL_BUNDLE_VAL_NEXT_MSG) == 0) {
		mailbox_handle_next_msg_bundle(view, result);
	} else if (g_strcmp0(msg_type, EMAIL_BUNDLE_VAL_PREV_MSG) == 0) {
		mailbox_handle_prev_msg_bundle(view, result);
	}

	debug_log("view->opened_mail_id : %d", view->opened_mail_id);
}

static void _account_result_cb(void *data, email_module_h module, email_params_h result)
{
	debug_enter();
	retm_if(!module, "module is NULL");
	retm_if(!data, "data is NULL");

	const char *mode = NULL;
	EmailMailboxView *view = (EmailMailboxView *)data;

	if (view->account)
		mailbox_account_module_destroy(view, view->account);

	/* handle result from move to folder mode */

	int is_move_mail_mode = 0;
	email_params_get_int_opt(result, EMAIL_BUNDLE_KEY_IS_MAILBOX_MOVE_MODE, &is_move_mail_mode);

	if (is_move_mail_mode) {
		int b_edit_mod = 0;
		const char *moved_mailbox_name = NULL;

		email_params_get_int_opt(result, EMAIL_BUNDLE_KEY_IS_MAILBOX_EDIT_MODE, &b_edit_mod);
		email_params_get_int_opt(result, EMAIL_BUNDLE_KEY_MAILBOX, &view->mailbox_id);
		email_params_get_int_opt(result, EMAIL_BUNDLE_KEY_MAILBOX_MOVE_STATUS, &view->move_status);
		email_params_get_str_opt(result, EMAIL_BUNDLE_KEY_MAILBOX_MOVED_MAILBOX_NAME, &moved_mailbox_name);

		G_FREE(view->moved_mailbox_name);
		view->moved_mailbox_name = g_strdup(moved_mailbox_name);

		if (b_edit_mod) {
			mailbox_exit_edit_mode(view);
		}
		return;
	}


	email_params_get_str_opt(result, EMAIL_BUNDLE_KEY_ACCOUNT_TYPE, &mode);

	mailbox_clear_prev_mailbox_info(view);

	if (g_strcmp0(mode, EMAIL_BUNDLE_VAL_ALL_ACCOUNT) == 0) {
		debug_log("EMAIL_BUNDLE_VAL_ALL_ACCOUNT");

		view->mode = EMAIL_MAILBOX_MODE_ALL;
		view->account_id = 0;

		int mailbox_type = 0;
		email_params_get_int_opt(result, EMAIL_BUNDLE_KEY_MAILBOX_TYPE, &mailbox_type);
		view->mailbox_type = mailbox_type;

		debug_log("account_id(%d), mailbox_type(%d)", view->account_id, view->mailbox_type);

	} else if (g_strcmp0(mode, EMAIL_BUNDLE_VAL_SINGLE_ACCOUNT) == 0) {
		debug_log("EMAIL_BUNDLE_VAL_SINGLE_ACCOUNT");

		view->mode = EMAIL_MAILBOX_MODE_MAILBOX;

		email_params_get_int_opt(result, EMAIL_BUNDLE_KEY_ACCOUNT_ID, &view->account_id);

		if (!email_params_get_int(result, EMAIL_BUNDLE_KEY_MAILBOX, &view->mailbox_id)) {
			debug_error("failure on getting mailbox_id");
			return;
		}

		debug_log("account_id[%d], mailbox_id[%d]", view->account_id, view->mailbox_id);

	} else if (g_strcmp0(mode, EMAIL_BUNDLE_VAL_PRIORITY_SENDER) == 0) {
		debug_log("EMAIL_BUNDLE_VAL_PRIORITY_SENDER");

		view->account_id = 0;
		view->mailbox_type = EMAIL_MAILBOX_TYPE_INBOX;
		view->mode = EMAIL_MAILBOX_MODE_PRIORITY_SENDER;
	} else {
		return;
	}

	mailbox_view_title_update_all(view);
	mailbox_check_sort_type_validation(view);
	mailbox_list_refresh(view, NULL);

	mailbox_sync_current_mailbox(view);
	/*if (mailbox_sync_current_mailbox(view))
		mailbox_refreshing_progress_add(view);
	else
		mailbox_refreshing_progress_remove(view);*/
}

/*
 * Definition for exported functions
 */

email_module_h mailbox_composer_module_create(void *data, email_module_type_e type, email_params_h params)
{
	debug_enter();

	EmailMailboxView *view = (EmailMailboxView *)data;

	email_module_listener_t listener = { 0 };
	listener.cb_data = view;
	listener.destroy_request_cb = mailbox_composer_module_destroy;

	email_module_h module = email_module_create_child(view->base.module, type, params, &listener);
	if (view->content_layout)
		elm_object_tree_focus_allow_set(view->content_layout, EINA_FALSE);

	debug_leave();

	return module;
}

void mailbox_composer_module_destroy(void *priv, email_module_h module)
{
	debug_enter();
	retm_if(!module, "module is NULL");
	retm_if(!priv, "priv is NULL");

	EmailMailboxView *view = (EmailMailboxView *)priv;

	GList *opened_mail = g_list_find_custom(view->mail_list, GINT_TO_POINTER(view->opened_mail_id), mailbox_compare_mailid_in_list);
	if (opened_mail) {
		MailItemData *ld = (MailItemData *)g_list_nth_data(opened_mail, 0);
		if (ld) {
			if (elm_genlist_item_selected_get(ld->item) == EINA_TRUE)
				elm_genlist_item_selected_set(ld->item, EINA_FALSE);
		}
	}
	if (view->content_layout)
		elm_object_tree_focus_allow_set(view->content_layout, EINA_TRUE);

	if (view->composer) {
		email_module_destroy(view->composer);
		view->composer = NULL;
	}
	view->is_module_launching = false;

	if (view->run_type == RUN_VIEWER_FROM_NOTIFICATION) {
		view->run_type = RUN_TYPE_UNKNOWN;
		mailbox_open_email_viewer(view, view->account_id, view->mailbox_id, view->start_mail_id);
	}
}

email_module_h mailbox_viewer_module_create(void *data, email_module_type_e type, email_params_h params)
{
	debug_enter();

	EmailMailboxView *view = (EmailMailboxView *)data;

	email_module_listener_t listener = { 0 };
	listener.cb_data = data;
	listener.result_cb = _viewer_result_cb;
	listener.destroy_request_cb = mailbox_viewer_module_destroy;

	if (!view->viewer) {
		view->viewer = email_module_create_child(view->base.module, type, params, &listener);
		if (view->content_layout) {
			elm_object_tree_focus_allow_set(view->content_layout, EINA_FALSE);
		}
	} else {
		email_module_send_message(view->viewer, params);
	}

	return view->viewer;
}

void mailbox_viewer_module_destroy(void *priv, email_module_h module)
{
	debug_enter();
	retm_if(!module, "module is NULL");
	retm_if(!priv, "priv is NULL");

	EmailMailboxView *view = (EmailMailboxView *)priv;
	if (view) {

		if (view->content_layout)
			elm_object_tree_focus_allow_set(view->content_layout, EINA_TRUE);
	}

	email_module_destroy(module);
	view->viewer = NULL;
	view->opened_mail_id = 0;
}

email_module_h mailbox_account_module_create(void *data, email_module_type_e type, email_params_h params)
{
	debug_enter();

	EmailMailboxView *view = (EmailMailboxView *)data;

	email_module_listener_t listener = { 0 };
	listener.cb_data = view;
	listener.result_cb = _account_result_cb;
	listener.destroy_request_cb = mailbox_account_module_destroy;

	email_module_h module = email_module_create_child(view->base.module, type, params, &listener);
	if (view->content_layout)
		elm_object_tree_focus_allow_set(view->content_layout, EINA_FALSE);

	return module;
}

void mailbox_account_module_destroy(void *priv, email_module_h module)
{
	debug_enter();
	retm_if(!module, "module is NULL");
	retm_if(!priv, "priv is NULL");

	EmailMailboxView *view = (EmailMailboxView *)priv;

	if (view->content_layout)
		elm_object_tree_focus_allow_set(view->content_layout, EINA_TRUE);

	if (view->account) {
		email_module_destroy(view->account);
		view->account = NULL;
	}
	view->is_module_launching = false;
}

email_module_h mailbox_setting_module_create(void *data, email_module_type_e type, email_params_h params)
{
	debug_enter();

	EmailMailboxView *view = (EmailMailboxView *)data;

	email_module_listener_t listener = { 0 };
	listener.cb_data = view;
	listener.destroy_request_cb = mailbox_setting_module_destroy;

	email_module_h module = email_module_create_child(view->base.module, type, params, &listener);
	if (view->content_layout)
		elm_object_tree_focus_allow_set(view->content_layout, EINA_FALSE);

	return module;
}

void mailbox_setting_module_destroy(void *priv, email_module_h module)
{
	debug_enter();
	retm_if(!module, "module is NULL");
	retm_if(!priv, "priv is NULL");

	EmailMailboxView *view = (EmailMailboxView *)priv;

	if (view->content_layout)
		elm_object_tree_focus_allow_set(view->content_layout, EINA_TRUE);

	if (module == view->setting) {
		email_module_destroy(view->setting);
		view->setting = NULL;
	}
}
