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

#include <stdio.h>
#include <stdlib.h>
#include <system_settings.h>
#include <app.h>
#include "email-mailbox.h"
#include "email-mailbox-toolbar.h"
#include "email-mailbox-list.h"
#include "email-mailbox-list-extensions.h"
#include "email-mailbox-search.h"
#include "email-mailbox-noti-mgr.h"
#include "email-mailbox-module-util.h"
#include "email-mailbox-util.h"
#include "email-mailbox-more-menu.h"
#include "email-mailbox-sync.h"
#include "email-mailbox-title.h"
#include "email-mailbox-request.h"
#include "email-utils.h"

/*
 * Definitions
 */

#define NANOS 1000000000
#define EMAIL_SERIVE_PING_TIMEOUT_SEC 0.1
#define EMAIL_SERIVE_PING_RETRY_DELAY_SEC 3

/*
 * Structures
 */


/*
 * Globals
 */


static pthread_mutex_t START_THREAD_MUTEX = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t START_THREAD_COND = PTHREAD_COND_INITIALIZER;

/*
 * Declaration for static functions
 */

static int _mailbox_module_create(email_module_t *self, email_params_h params);
static void _mailbox_module_destroy(email_module_t *self);
static void _mailbox_on_message(email_module_t *self, email_params_h msg);

static void _mailbox_email_service_ping_thread_heavy_cb(void *data, Ecore_Thread *thread);
static void _mailbox_email_service_ping_thread_notify_cb(void *data, Ecore_Thread *thread, void *msg_data);

static int _mailbox_module_create_view(EmailMailboxModule *module);
static int _mailbox_create_account_setting_module(EmailMailboxView *view);

static int _mailbox_create(email_view_t *self);
static void _mailbox_destroy(email_view_t *self);
static void _mailbox_activate(email_view_t *self, email_view_state prev_state);
static void _mailbox_deactivate(email_view_t *self);
static void _mailbox_update(email_view_t *self, int flags);
static void _mailbox_on_back_key(email_view_t *self);

static int _mailbox_initialize(EmailMailboxView *view);
static void _mailbox_finalize(EmailMailboxView *view);

static email_run_type_e _mailbox_params_get_run_type(email_params_h params);
static int _mailbox_params_get_account_id(email_params_h params);
static int _mailbox_params_get_mail_id(email_params_h params);

static int _mailbox_handle_launch_mailbox_bundle_val(EmailMailboxView *view, email_params_h msg);
static int _mailbox_handle_launch_viewer_bundle_val(EmailMailboxView *view, email_params_h msg);
static int _mailbox_handle_launch_composer_bundle_val(EmailMailboxView *view, email_params_h msg);

static void _mailbox_create_view(EmailMailboxView *view);
static void _mailbox_delete_evas_object(EmailMailboxView *view);
static void _mailbox_timezone_change_cb(system_settings_key_e key, void *data);
static void _mailbox_sys_settings_datetime_format_changed_cb(system_settings_key_e node, void *data);
static void _mailbox_vip_rule_value_changed_cb(const char *key, void *data);

static void _mailbox_title_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static void _mailbox_module_show_initialising_popup(EmailMailboxModule *module);
static void _mailbox_initialising_popup_back_cb(void *data, Evas_Object *obj, void *event_info);
static int _mailbox_push_base_view_layout(EmailMailboxView *view);

/*
 * Definition for static functions
 */

static int _mailbox_module_create(email_module_t *self, email_params_h params)
{
	debug_enter();
	retvm_if(!self, -1, "self is NULL");

	EmailMailboxModule *module = (EmailMailboxModule *)self;

	if (!email_params_clone(&module->view.run_params, params)) {
		debug_error("email_params_clone () failed!");
		return -1;
	}

	module->view.run_type = _mailbox_params_get_run_type(params);

	if (module->view.run_type == RUN_MAILBOX_FROM_NOTIFICATION) {
		module->view.account_id = _mailbox_params_get_account_id(params);
	} else if (module->view.run_type == RUN_VIEWER_FROM_NOTIFICATION) {
		module->view.start_mail_id = _mailbox_params_get_mail_id(params);
	}

	module->start_thread = ecore_thread_feedback_run(_mailbox_email_service_ping_thread_heavy_cb,
			_mailbox_email_service_ping_thread_notify_cb, NULL, NULL, module, EINA_TRUE);
	if (!module->start_thread) {
		debug_error("Start thread create failed!");
		return -1;
	}

	bool wait_timed_out = false;

	struct timespec wait_until;
	clock_gettime(CLOCK_REALTIME, &wait_until);
	wait_until.tv_nsec += EMAIL_SERIVE_PING_TIMEOUT_SEC * NANOS;
	wait_until.tv_sec += wait_until.tv_nsec / NANOS;
	wait_until.tv_nsec %= NANOS;

	pthread_mutex_lock(&START_THREAD_MUTEX);
	while (!module->start_thread_done) {
		int r = pthread_cond_timedwait(&START_THREAD_COND, &START_THREAD_MUTEX, &wait_until);
		if (r == ETIMEDOUT) {
			wait_timed_out = true;
			break;
		}
	}
	pthread_mutex_unlock(&START_THREAD_MUTEX);

	if (wait_timed_out) {
		_mailbox_module_show_initialising_popup(module);
		debug_leave();
		return 0;
	}

	_mailbox_email_service_ping_thread_notify_cb(module, NULL, NULL);

	debug_leave();
	return 0;
}

static void _mailbox_module_destroy(email_module_t *self)
{
	debug_enter();
	retm_if(!self, "self is NULL");

	EmailMailboxModule *module = (EmailMailboxModule *)self;

	email_params_free(&module->view.run_params);

	if (module->start_thread) {
		ecore_thread_cancel(module->start_thread);
		module->start_thread = NULL;
	}

	debug_leave();
}

static void _mailbox_email_service_ping_thread_heavy_cb(void *data, Ecore_Thread *thread)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailMailboxModule *module = data;

	gboolean first = true;
	gboolean ret = false;

	while (!ret && !ecore_thread_check(thread)) {

		if (!first) {
			debug_log("Waiting for %d seconds before retry...", EMAIL_SERIVE_PING_RETRY_DELAY_SEC);
			sleep(EMAIL_SERIVE_PING_RETRY_DELAY_SEC);
		}
		first = false;

		debug_log("Initializing email engine...");

		ret = email_engine_initialize_force();
		debug_log("email_service_begin() return: %s", ret ? "ok" : "fail");

		if (ret) {
			ecore_thread_feedback(thread, NULL);
			pthread_mutex_lock(&START_THREAD_MUTEX);
			module->start_thread_done = true;
			pthread_mutex_unlock(&START_THREAD_MUTEX);
			pthread_cond_signal(&START_THREAD_COND);
		}

		email_engine_finalize_force();
	}

	debug_leave();
}

static void _mailbox_email_service_ping_thread_notify_cb(void *data, Ecore_Thread *thread, void *msg_data)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailMailboxModule *module = data;

	if (module->start_thread) {
		module->start_thread = NULL;
		if (_mailbox_module_create_view(module) != 0) {
			debug_error("_mailbox_module_create_view() failed!");
			email_module_make_destroy_request(&module->base);
			return;
		}
		DELETE_EVAS_OBJECT(module->init_pupup);
	}

	debug_leave();
}

static int _mailbox_module_create_view(EmailMailboxModule *module)
{
	debug_enter();
	retvm_if(!module, -1, "module is NULL");

	int ret = 0;

	module->view.base.create = _mailbox_create;
	module->view.base.destroy = _mailbox_destroy;
	module->view.base.activate = _mailbox_activate;
	module->view.base.deactivate = _mailbox_deactivate;
	module->view.base.update = _mailbox_update;
	module->view.base.on_back_key = _mailbox_on_back_key;

	ret = email_module_create_view(&module->base, &module->view.base);
	if (ret != 0) {
		debug_error("email_module_create_view(): failed (%d)", ret);
		return -1;
	}

	debug_leave();
	return 0;
}

static int _mailbox_create_account_setting_module(EmailMailboxView *view)
{
	debug_enter();
	retvm_if(!view, -1, "view is NULL");

	email_params_h params = NULL;

	if (email_params_create(&params) &&
		email_params_add_str(params, EMAIL_BUNDLE_KEY_VIEW_TYPE, EMAIL_BUNDLE_VAL_VIEW_ACCOUNT_SETUP)) {

		view->setting = mailbox_setting_module_create(view, EMAIL_MODULE_SETTING, params);
	}

	email_params_free(&params);

	retvm_if(!view->setting, -1, "mailbox_setting_module_create() failed!");

	debug_leave();
	return 0;
}

static email_run_type_e _mailbox_params_get_run_type(email_params_h params)
{
	debug_enter();

	email_run_type_e result = RUN_TYPE_UNKNOWN;

	const char *operation = NULL;

	if (!email_params_get_operation(params, &operation)) {
		return result;
	}

	if (strcmp(operation, APP_CONTROL_OPERATION_DEFAULT) == 0) {
		if (email_params_get_int_opt(params, EMAIL_BUNDLE_KEY_RUN_TYPE, &result)) {
			if ((result != RUN_TYPE_UNKNOWN) && (result != RUN_VIEWER_FROM_NOTIFICATION) &&
				(result != RUN_MAILBOX_FROM_NOTIFICATION)) {
				debug_warning("Run type is not supported: %d", (int)result);
				result = RUN_TYPE_UNKNOWN;
			}
		} else {
			debug_error("email_params_get_int_opt() failed!");
		}
	} else if (strcmp(operation, APP_CONTROL_OPERATION_COMPOSE) == 0
			|| strcmp(operation, APP_CONTROL_OPERATION_SHARE) == 0
			|| strcmp(operation, APP_CONTROL_OPERATION_MULTI_SHARE) == 0
			|| strcmp(operation, APP_CONTROL_OPERATION_SHARE_TEXT) == 0) {
		result = RUN_COMPOSER_EXTERNAL;
	} else {
		debug_warning("Operation is not supported: %s", operation);
	}

	debug_leave();
	return result;
}

static int _mailbox_params_get_account_id(email_params_h params)
{
	debug_enter();

	int result = 0;

	if (!email_params_get_int(params, EMAIL_BUNDLE_KEY_ACCOUNT_ID, &result)) {
		debug_error("email_params_get_int() failed!");
	}

	debug_log("result: %d", result);

	debug_leave();
	return result;
}

static int _mailbox_params_get_mail_id(email_params_h params)
{
	debug_enter();

	int result = 0;

	if (!email_params_get_int(params, EMAIL_BUNDLE_KEY_MAIL_ID, &result)) {
		debug_error("email_params_get_int() failed!");
	}

	debug_log("result: %d", result);

	debug_leave();
	return result;
}

static int _mailbox_create(email_view_t *self)
{
	debug_enter();
	retvm_if(!self, -1, "self is NULL");

	EmailMailboxView *view = (EmailMailboxView *)self;

	int ret = 0;
	int default_account_id = 0;
	email_run_type_e run_type = view->run_type;

	if (!email_engine_initialize()) {
		debug_error("Failed to initialize email engine");
		return -1;
	}

	ret = _mailbox_push_base_view_layout(view);
	retvm_if(ret != 0, -1, "_mailbox_push_base_view_layout() failed! ret: [%d]", ret);

	if (!email_engine_get_default_account(&default_account_id)) {
		debug_log("No default account. Creating account setup setting view...");
		view->account_id = 0;
		return _mailbox_create_account_setting_module(view);
	}

	view->run_type = RUN_TYPE_UNKNOWN;
	view->b_ready_for_initialize = true;

	if (run_type == RUN_VIEWER_FROM_NOTIFICATION) {
		email_mail_list_item_t *mail = email_engine_get_mail_by_mailid(view->start_mail_id);
		retvm_if(!mail, -1, "email_engine_get_mail_by_mailid() failed!");
		view->account_id = mail->account_id;
		view->mailbox_id = mail->mailbox_id;
		return mailbox_open_email_viewer(view, mail->account_id, mail->mailbox_id, view->start_mail_id);
	} else if (run_type == RUN_COMPOSER_EXTERNAL) {
		view->composer = mailbox_composer_module_create(view, EMAIL_MODULE_COMPOSER, view->run_params);
		return (view->composer ? 0 : -1);
	}

	debug_leave();
	return _mailbox_initialize(view);
}

static int _mailbox_initialize(EmailMailboxView *view)
{
	debug_enter();
	retvm_if(!view, -1, "view is NULL");
	retvm_if(!view->b_ready_for_initialize, -1, "view->b_ready_for_initialize is false");

	/* init members of mailbox */
	view->initialized = true;
	view->account_count = 0;
	view->mailbox_type = EMAIL_MAILBOX_TYPE_NONE;
	view->sort_type = EMAIL_SORT_DATE_RECENT;
	view->sort_type_index = SORTBY_DATE_RECENT;
	view->only_local = false;
	view->gl = NULL;
	view->mail_list = NULL;
	view->b_editmode = false;
	view->g_sending_mail_list = NULL;
	view->is_send_all_run = false;
	view->account_type = EMAIL_SERVER_TYPE_NONE;
	view->b_format_24hour = false;

	/*Search mode*/
	view->b_searchmode = false;
	view->search_type = EMAIL_SEARCH_NONE;

	/*SelectAll Item*/
	view->select_all_item_data.base.item_type = MAILBOX_LIST_ITEM_SELECT_ALL;
	view->select_all_item_data.base.item = NULL;
	view->select_all_item_data.checkbox = NULL;
	view->select_all_item_data.is_checked = EINA_FALSE;
	view->select_all_item_data.view = view;

	/*Last Update Time Item*/
	view->update_time_item_data.base.item_type = MAILBOX_LIST_ITEM_UPDATE_TIME;
	view->update_time_item_data.base.item = NULL;
	view->update_time_item_data.time = NULL;

	/* DBUS */
	mailbox_setup_dbus_receiver(view);

	system_settings_get_value_bool(
			SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR,
			&view->b_format_24hour);


	view->request_queue = email_request_queue_create();
	/* register thread callbacks for mailbox thread operation */
	mailbox_requests_cbs_register();

	/* create accouts list view */
	_mailbox_create_view(view);

	int account_count = 0;
	email_account_t *account_list = NULL;

	if (email_engine_get_account_list(&account_count, &account_list))
		view->account_count = account_count;
	else
		view->account_count = 0;

	if (account_list && (view->account_count > 0)) {
		int i;
		for (i = 0; i < account_count; i++) {
			mailbox_account_color_list_add(view, account_list[i].account_id, account_list[i].color_label);
		}
	}

	if ((view->account_id == 0 && view->account_count > 1)
			|| (view->account_count == 0)) {
		view->account_id = 0;
		view->mode = EMAIL_MAILBOX_MODE_ALL;
		view->mailbox_type = EMAIL_MAILBOX_TYPE_INBOX;
	} else {
		email_mailbox_t *mailbox = NULL;

		view->mode = EMAIL_MAILBOX_MODE_MAILBOX;
		if (view->account_id == 0 && account_list) {
			view->account_id = account_list[0].account_id; /* If one account is only registered, launch mailbox mode by using first account id. */
			view->mailbox_id = 0;
		}

		if (view->mailbox_id == 0) {
			if (email_engine_get_mailbox_by_mailbox_type(view->account_id, EMAIL_MAILBOX_TYPE_INBOX, &mailbox)) {
				view->mailbox_id = mailbox->mailbox_id;
				email_engine_free_mailbox_list(&mailbox, 1);
			} else {
				view->mailbox_id = 0;
				debug_warning("email_engine_get_mailbox_by_mailbox_type : account_id(%d) type(INBOX)", view->account_id);
			}
		}
	}

	if (account_list)
		email_engine_free_account_list(&account_list, account_count);

	mailbox_view_title_update_all(view);
	mailbox_list_refresh(view, NULL);

	/* register callbacks */
	system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR, _mailbox_sys_settings_datetime_format_changed_cb, view);
	system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_LOCALE_COUNTRY, _mailbox_sys_settings_datetime_format_changed_cb, view);

	mailbox_list_system_settings_callback_register(view);

	email_register_timezone_changed_callback(_mailbox_timezone_change_cb, view);

	return 0;
}

static void _mailbox_activate(email_view_t *self, email_view_state prev_state)
{
	debug_enter();
	retm_if(!self, "self is NULL");

	EmailMailboxView *view = (EmailMailboxView *)self;

	if (view->initialized) {
		if (!view->started) {
			view->started = true;

			int res = email_update_vip_rule_value();
			if (res != 0) {
				debug_error("email_update_vip_rule_value failed. err = %d", res);
			}

			res = email_get_vip_rule_value(&view->vip_rule_value);
			if (res != 0) {
				debug_error("email_get_vip_rule_value failed. err = %d", res);
			}

			res = email_set_vip_rule_change_callback(_mailbox_vip_rule_value_changed_cb, view);
			if (res != 0) {
				debug_error("email_set_vip_rule_change_callback failed. err = %d", res);
			}

			/* download new mails */
			if (mailbox_sync_current_mailbox(view))
			{
				debug_log("Start sync");
			}
		}

		if (view->remaining_req) {
			mailbox_list_make_remaining_items_in_thread(view, view->remaining_req);
			view->remaining_req = NULL;
		}

		mailbox_update_notifications_status(view);
	}

	debug_leave();
}

static void _mailbox_deactivate(email_view_t *self)
{
	debug_enter();
	retm_if(!self, "self is NULL");

	EmailMailboxView *view = (EmailMailboxView *)self;

	if (view->initialized) {
		mailbox_update_notifications_status(view);
	}

	debug_leave();
}

static void _mailbox_update(email_view_t *self, int flags)
{
	debug_enter();
	retm_if(!self, "self is NULL");

	EmailMailboxView *view = (EmailMailboxView *)self;

	debug_log("flags: %d", flags);

	bool need_update = false;
	bool update_title = false;

	if (!view->initialized) {
		if (flags & EVUF_POPPING) {
			int default_account_id = 0;
			if (!email_engine_get_default_account(&default_account_id)) {
				debug_log("No default account. Terminating...");
				email_module_make_destroy_request(view->base.module);
			} else {
				view->b_ready_for_initialize = true;
				if (_mailbox_initialize(view) != 0) {
					debug_error("Application initialization failed. Terminating...");
					email_module_make_destroy_request(view->base.module);
				}
			}
		}
		return;
	}

	if (flags & EVUF_WAS_PAUSED) {
		double viprule_val;
		int res = email_get_vip_rule_value(&viprule_val);
		if (res != 0) {
			debug_error("email_update_viprule_value failed. err = %d", res);
		} else {
#ifndef _FEATURE_PRIORITY_SENDER_DISABLE_
			if (view->vip_rule_value != viprule_val &&
					view->mode == EMAIL_MAILBOX_MODE_PRIORITY_SENDER) {
				debug_log("Mailbox list should be refreshed because vip setting is changed, priority sender view");
				need_update = true;
				update_title = true;
			}
#endif
			view->vip_rule_value = viprule_val;
		}
	}

	if (flags & EVUF_LANGUAGE_CHANGED) {
		need_update = true;
		update_title = true;
	}

	if (flags & EVUF_REGION_FMT_CHANGED) {
		need_update = true;
	}

	if (need_update) {
		mailbox_refresh_fullview(view, update_title);
	}
}

static void _mailbox_destroy(email_view_t *self)
{
	debug_enter();
	retm_if(!self, "self is NULL");

	EmailMailboxView *view = (EmailMailboxView *)self;

	_mailbox_finalize(view);

	email_engine_finalize();

	debug_leave();
}

static void _mailbox_finalize(EmailMailboxView *view)
{
	debug_enter();
	retm_if(!view, "view is NULL");

	if (!view->initialized) {
		return;
	}
	view->initialized = false;

	if(view->search_entry_focus_idler) {
		ecore_idler_del(view->search_entry_focus_idler);
		view->search_entry_focus_idler = NULL;
	}

	mailbox_sync_cancel_all(view);

	_mailbox_delete_evas_object(view);

	if (view->request_queue) {
		email_request_queue_destroy(view->request_queue);
		view->request_queue = NULL;
	}
	mailbox_requests_cbs_unregister();

	mailbox_list_free_all_item_data(view);

	mailbox_sending_mail_list_free(view);
	mailbox_account_color_list_free(view);
	mailbox_folders_name_cache_clear(view);

	G_FREE(view->mailbox_alias);
	G_FREE(view->account_name);
	FREE(view->update_time_item_data.time);
	G_FREE(view->moved_mailbox_name);

	email_unregister_timezone_changed_callback(_mailbox_timezone_change_cb);

	system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR);
	system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_LOCALE_COUNTRY);

	int res = email_unset_vip_rule_change_callback();
	if (res != 0) {
		debug_error("email_unset_vip_rule_change_callback failed. err = %d", res);
	}

	elm_theme_extension_del(view->theme, email_get_mailbox_theme_path());
	elm_theme_free(view->theme);

	mailbox_list_system_settings_callback_unregister();

	mailbox_remove_dbus_receiver();

}

static void _mailbox_on_message(email_module_t *self, email_params_h msg)
{
	debug_enter();
	retm_if(!self, "self is NULL");

	EmailMailboxModule *module = (EmailMailboxModule *)self;
	EmailMailboxView *view = &module->view;
	email_run_type_e run_type = _mailbox_params_get_run_type(msg);

	if (!view->initialized && (_mailbox_initialize(view) != 0)) {
		debug_error("Initialize failed!");
		return;
	}

	if (run_type == RUN_MAILBOX_FROM_NOTIFICATION) {
		_mailbox_handle_launch_mailbox_bundle_val(view, msg);
	} else if (run_type == RUN_VIEWER_FROM_NOTIFICATION) {
		_mailbox_handle_launch_viewer_bundle_val(view, msg);
	} else if (run_type == RUN_COMPOSER_EXTERNAL) {
		_mailbox_handle_launch_composer_bundle_val(view, msg);
	} else {
		debug_log("Unknown msg type: %d", run_type);
	}

	debug_leave();
	return;
}

int mailbox_handle_next_msg_bundle(EmailMailboxView *view, email_params_h msg)
{
	debug_enter();
	retvm_if(!view, -1, "Error: view is NULL");

	debug_log("EMAIL_BUNDLE_VAL_NEXT_MSG");

	if (!view->initialized && (_mailbox_initialize(view) != 0)) {
		debug_error("Initialize failed!");
		return -1;
	}

	MailItemData *ld = NULL;
	GList *first = g_list_first(view->mail_list);
	GList *last = g_list_last(view->mail_list);
	int current_mail_index = -1;

	int cnt = g_list_length(view->mail_list);
	debug_log("first %p, last %p, cnt: %d", first, last, cnt);

	email_params_get_int_opt(msg, EMAIL_BUNDLE_KEY_MAIL_INDEX, &current_mail_index);
	debug_log("current_mail_index: %d", current_mail_index);

	GList *opened_mail = g_list_find_custom(view->mail_list, GINT_TO_POINTER(view->opened_mail_id), mailbox_compare_mailid_in_list);
	if (!opened_mail && current_mail_index > -1 && current_mail_index < cnt) {
		opened_mail = g_list_nth(first, current_mail_index);
	} else if (!opened_mail && current_mail_index > -1 && current_mail_index == cnt) {
		opened_mail = g_list_nth(first, (current_mail_index - 1));
	}

	if (!opened_mail) {
		debug_error("cannot find the opened mail");
		return -1;
	}

	if (opened_mail == last) {
		debug_log("get the first mail data");
		ld = (MailItemData *)g_list_nth_data(first, 0);
	} else {
		debug_log("get the next mail data");
		ld = (MailItemData *)g_list_nth_data(opened_mail, 1);
	}

	if (ld) {
		mailbox_list_open_email_viewer(ld);
	} else {
		debug_error("cannot find the next mail");
	}

	debug_leave();
	return 0;

}

int mailbox_handle_prev_msg_bundle(EmailMailboxView *view, email_params_h msg)
{
	debug_enter();
	retvm_if(!view, -1, "Error: view is NULL");

	debug_log("EMAIL_BUNDLE_VAL_PREV_MSG");

	if (!view->initialized && (_mailbox_initialize(view) != 0)) {
		debug_error("Initialize failed!");
		return -1;
	}

	MailItemData *ld = NULL;
	GList *first = g_list_first(view->mail_list);
	GList *last = g_list_last(view->mail_list);
	int current_mail_index = -1;

	int cnt = g_list_length(view->mail_list);
	debug_log("first %p, last %p, cnt: %d", first, last, cnt);

	email_params_get_int_opt(msg, EMAIL_BUNDLE_KEY_MAIL_INDEX, &current_mail_index);
	debug_log("current_mail_index: %d", current_mail_index);

	GList *opened_mail = g_list_find_custom(view->mail_list, GINT_TO_POINTER(view->opened_mail_id), mailbox_compare_mailid_in_list);
	if (!opened_mail && current_mail_index > -1 && current_mail_index < cnt) {
		opened_mail = g_list_nth(first, current_mail_index);
	} else if (!opened_mail && current_mail_index > -1 && current_mail_index == cnt) {
		opened_mail = g_list_nth(first, (current_mail_index-1));
	}

	if (!opened_mail) {
		debug_error("cannot find the opened mail");
		return -1;
	}

	if (opened_mail == first) {
		debug_log("get the first mail data");
		ld = (MailItemData *)g_list_nth_data(last, 0);

	} else {
		debug_log("get the previous mail data");
		GList *prev_mail = g_list_nth_prev(opened_mail, 1);
		ld = (MailItemData *)g_list_nth_data(prev_mail, 0);
	}

	if (ld) {
		mailbox_list_open_email_viewer(ld);
	} else {
		debug_error("cannot find the previous mail");
	}

	debug_leave();
	return 0;
}

static int _mailbox_destroy_child_modules(EmailMailboxView *view, bool keep_viewer)
{
	debug_enter();
	retvm_if(!view, -1,  "Error: view is NULL");

	email_params_h params = NULL;
	int ret = -1;

	if (view->account) {
		mailbox_account_module_destroy(view, view->account);
	}

	if (view->setting) {
		mailbox_setting_module_destroy(view, view->setting);
	}

	if (view->viewer && !keep_viewer) {
		debug_log("Viewer is running. Need to destroy all viewer child modules.");

		if (email_params_create(&params) &&
			email_params_add_str(params, EMAIL_BUNDLE_KEY_MSG, EMAIL_BUNDLE_VAL_VIEWER_DESTROY_VIEW)) {
			ret = email_module_send_message(view->viewer, params);
		}

		if (ret != 0) {
			debug_warning("Failed to send message to viewer. Forcing destroy.");
			mailbox_viewer_module_destroy(view, view->viewer);
		}
	}

	if (view->composer) {
		debug_log("Asking composer to save in drafts.");

		if (email_params_create(&params) &&
			email_params_add_str(params, EMAIL_BUNDLE_KEY_MSG, EMAIL_BUNDLE_VAL_EMAIL_COMPOSER_SAVE_DRAFT)) {
			ret = email_module_send_message(view->composer, params);
		}

		if (ret != 0) {
			debug_log("Failed to send message to composer. Forcing destroy.");
			mailbox_composer_module_destroy(view, view->composer);
		}
	}

	email_params_free(&params);

	debug_leave();
	return 0;
}

static int _mailbox_update_mailbox(EmailMailboxView *view, int account_id, int mailbox_id)
{
	debug_enter();
	bool need_update = true;

	if (account_id == view->account_id) {
		if (mailbox_id == 0) {
			if (view->mailbox_type == EMAIL_MAILBOX_TYPE_INBOX &&
				view->mode != EMAIL_MAILBOX_MODE_PRIORITY_SENDER) {
				need_update = false;
			}
		} else {
			if (view->mailbox_id == mailbox_id &&
				view->mode == EMAIL_MAILBOX_MODE_MAILBOX) {
				need_update = false;
			}
		}
	}

	if (view->b_editmode) {
		mailbox_exit_edit_mode(view);
	}

	if (!need_update) {
		debug_log("the proper mailbox is shown now.");
		if (view->b_searchmode) {
			mailbox_set_search_mode(view, EMAIL_SEARCH_NONE);
		}
		return 0;
	}

	if (account_id == 0) {
		view->account_id = account_id;
		view->mode = EMAIL_MAILBOX_MODE_ALL;
		view->mailbox_type = EMAIL_MAILBOX_TYPE_INBOX;
		view->only_local = FALSE;
	} else {
		email_mailbox_t *mailbox = NULL;

		view->mode = EMAIL_MAILBOX_MODE_MAILBOX;
		view->account_id = account_id;
		view->mailbox_id = mailbox_id;

		if (mailbox_id == 0) {
			if (email_engine_get_mailbox_by_mailbox_type(view->account_id, EMAIL_MAILBOX_TYPE_INBOX, &mailbox)) {
				view->mailbox_id = mailbox->mailbox_id;
				email_engine_free_mailbox_list(&mailbox, 1);
			} else {
				debug_warning("email_engine_get_mailbox_by_mailbox_type : account_id(%d) type(INBOX)", view->account_id);
				view->mailbox_id = 0;
			}
		}
	}

	mailbox_view_title_update_all(view);
	mailbox_update_notifications_status(view);
	mailbox_check_sort_type_validation(view);

	if (view->b_searchmode) {
		mailbox_set_search_mode(view, EMAIL_SEARCH_NONE);
	} else {
		mailbox_list_refresh(view, NULL);
	}

	debug_leave();
	return 0;
}

static int _mailbox_handle_launch_mailbox_bundle_val(EmailMailboxView *view, email_params_h msg)
{
	debug_enter();
	retvm_if(!view, -1,  "Error: view is NULL");

	int account_id = 0;
	int ret = 0;

	account_id = _mailbox_params_get_account_id(msg);
	retvm_if(account_id <= 0, -1,  "(account_id <= 0) is not allowed");

	ret = _mailbox_destroy_child_modules(view, false);
	retvm_if(ret != 0, -1,  "_mailbox_destroy_child_modules() failed!");

	ret = _mailbox_update_mailbox(view, account_id, 0);
	retvm_if(ret != 0, -1,  "_mailbox_update_mailbox() failed!");

	debug_leave();
	return 0;
}

static int _mailbox_handle_launch_viewer_bundle_val(EmailMailboxView *view, email_params_h msg)
{
	debug_enter();
	retvm_if(!view, -1, "Error: view is NULL");

	int mail_id = 0;
	email_mail_list_item_t *mail = NULL;
	int ret = -1;

	mail_id = _mailbox_params_get_mail_id(msg);
	retvm_if(mail_id <= 0, -1,  "(mail_id <= 0) is not allowed");

	if (view->viewer && (mail_id == view->opened_mail_id)) {
		email_params_h params = NULL;

		if (email_params_create(&params) &&
			email_params_add_str(params, EMAIL_BUNDLE_KEY_MSG, EMAIL_BUNDLE_VAL_VIEWER_RESTORE_VIEW)) {
			ret = email_module_send_message(view->viewer, params);
		} else {
			ret = -1;
		}

		email_params_free(&params);

		retvm_if(ret != 0, -1,  "Failed to send message to viewer!");
		return 0;
	}

	mail = email_engine_get_mail_by_mailid(mail_id);
	retvm_if(!mail, -1,  "email_engine_get_mail_by_mailid() failed!");

	ret = _mailbox_destroy_child_modules(view, true);
	retvm_if(ret != 0, -1,  "_mailbox_update_mailbox() failed!");

	ret = _mailbox_update_mailbox(view, mail->account_id, mail->mailbox_id);
	retvm_if(ret != 0, -1,  "_mailbox_update_mailbox() failed!");

	if (!view->composer) {
		ret = mailbox_open_email_viewer(view, mail->account_id, mail->mailbox_id, mail_id);
		retvm_if(ret != 0, -1,  "mailbox_open_email_viewer() failed!");
	} else {
		view->run_type = RUN_VIEWER_FROM_NOTIFICATION;
		view->start_mail_id = mail_id;
	}

	debug_leave();
	return 0;
}

int _mailbox_handle_launch_composer_bundle_val(EmailMailboxView *view, email_params_h msg)
{
	debug_enter();
	retvm_if(!view, -1, "Error: view is NULL");

	int ret = -1;

	ret = _mailbox_destroy_child_modules(view, false);
	retvm_if(ret != 0, -1,  "_mailbox_update_mailbox() failed!");

	if (!view->composer) {
		view->composer = mailbox_composer_module_create(view, EMAIL_MODULE_COMPOSER, msg);
	} else {
		email_params_free(&view->run_params);
		if (!email_params_clone(&view->run_params, msg)) {
			debug_error("email_params_clone () failed!");
			return -1;
		}
		view->run_type = RUN_COMPOSER_EXTERNAL;
	}

	debug_leave();
	return 0;
}

static void _mailbox_create_view(EmailMailboxView *view)
{
	debug_enter();

	/*load theme*/
	view->theme = elm_theme_new();
	elm_theme_ref_set(view->theme, NULL); /* refer to default theme(NULL) */
	elm_theme_extension_add(view->theme, email_get_mailbox_theme_path()); /* add extension to the new theme */

	/*base content*/
	view->content_layout = elm_layout_add(view->base.content);
	evas_object_size_hint_weight_set(view->content_layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_layout_file_set(view->content_layout, email_get_mailbox_theme_path(), "layout.email.mailbox.hd");
	elm_object_part_content_set(view->base.content, "elm.swallow.content", view->content_layout);

	/*title*/
	view->title_layout = elm_layout_add(view->base.content);
	evas_object_size_hint_weight_set(view->title_layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_layout_file_set(view->title_layout, email_get_mailbox_theme_path(), "email/layout/mailbox_title");
	elm_object_part_content_set(view->content_layout, "top_bar", view->title_layout);


	/* create genlist */
	mailbox_list_create_view(view);

	/* create nocontents layout */
	mailbox_create_no_contents_view(view, false);

	elm_object_part_content_set(view->content_layout, "list", view->gl);
	view->sub_layout_search = view->content_layout;

	mailbox_naviframe_mailbox_button_add(view);

	/*Compose floatting button*/
	mailbox_create_compose_btn(view);
}

static void _mailbox_module_show_initialising_popup(EmailMailboxModule *module)
{
	debug_enter();
	retm_if(!module, "module == NULL");

	Evas_Object *popup = elm_popup_add(module->base.win);
	elm_popup_align_set(popup, ELM_NOTIFY_ALIGN_FILL, 1.0);

	Evas_Object *layout = elm_layout_add(popup);
	elm_layout_file_set(layout, email_get_common_theme_path(), "processing_view");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_object_domain_translatable_part_text_set(layout, "elm.text", PACKAGE, "IDS_EMAIL_POP_INITIALISING_APPLICATION_ING");

	Evas_Object *progress = elm_progressbar_add(popup);
	elm_object_style_set(progress, "process_medium");
	evas_object_size_hint_align_set(progress, EVAS_HINT_FILL, 0.5);
	evas_object_size_hint_weight_set(progress, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_progressbar_pulse_set(progress, EINA_TRUE);
	elm_progressbar_pulse(progress, EINA_TRUE);
	evas_object_show(progress);
	elm_object_part_content_set(layout, "processing", progress);
	elm_object_content_set(popup, layout);

	eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, _mailbox_initialising_popup_back_cb, module);

	evas_object_show(popup);

	module->init_pupup = popup;
}

static void _mailbox_initialising_popup_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data == NULL");

	EmailMailboxModule *module = data;

	email_module_make_destroy_request(&module->base);
}

static int _mailbox_push_base_view_layout(EmailMailboxView *view)
{
	debug_enter();
	retvm_if(!view, -1, "view == NULL");

	view->base.content = elm_layout_add(view->base.module->navi);
	elm_layout_theme_set(view->base.content, "layout", "application", "default");
	evas_object_size_hint_weight_set(view->base.content, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(view->base.content);
	email_module_view_push(&view->base, NULL, EVPF_NO_BACK_BUTTON | EVPF_NO_TITLE);

	debug_leave();
	return 0;
}

static void _mailbox_delete_evas_object(EmailMailboxView *view)
{
	debug_enter();
	retm_if(!view, "view == NULL");

	DELETE_EVAS_OBJECT(view->more_ctxpopup);
	DELETE_EVAS_OBJECT(view->error_popup);
	DELETE_EVAS_OBJECT(view->searchbar_ly);
	DELETE_EVAS_OBJECT(view->title_layout);
	DELETE_EVAS_OBJECT(view->compose_btn);
	DELETE_EVAS_OBJECT(view->gl);
}

static void _mailbox_timezone_change_cb(system_settings_key_e key, void *data)
{
	debug_enter();
	retm_if(!data, "data == NULL");

	EmailMailboxView *view = (EmailMailboxView *)data;

	mailbox_exit_edit_mode(view);

	if (view->update_time_item_data.base.item) {
		mailbox_last_updated_time_item_update(view->mailbox_id, view);
	}

	mailbox_list_refresh(view, NULL);
}

static void _mailbox_sys_settings_datetime_format_changed_cb(system_settings_key_e node, void *data)
{
	debug_enter();
	retm_if(!data, "data == NULL");

	EmailMailboxView *view = (EmailMailboxView *)data;

	char *dt_fmt = email_get_datetime_format();

	/* This values is used on mailbox_last_updated_time_item_update function. */
	system_settings_get_value_bool(
			SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR,
			&view->b_format_24hour);

	if (view->update_time_item_data.base.item) {
		mailbox_last_updated_time_item_update(view->mailbox_id, view);
	}

	FREE(dt_fmt);

}

static void _mailbox_vip_rule_value_changed_cb(const char *key, void *data)
{
	debug_enter();
	retm_if(!data, "data == NULL");

	EmailMailboxView *view = (EmailMailboxView *)data;

	int res = email_get_vip_rule_value(&view->vip_rule_value);
	if (res != 0) {
		debug_error("email_update_viprule_value failed. err = %d", res);
	}

#ifndef _FEATURE_PRIORITY_SENDER_DISABLE_
	if (view->mode == EMAIL_MAILBOX_MODE_PRIORITY_SENDER) {
		debug_log("Mailbox list should be refreshed because vip setting is changed, priority sender view");
		mailbox_view_title_update_all(view);
		mailbox_list_refresh(view, NULL);
	}
#endif
}

static void _mailbox_title_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailMailboxView *view = data;
	retm_if(view->is_module_launching, "is_module_launching is true;");
	view->is_module_launching = true;

	debug_log("account_id(%d), mode(%d)", view->account_id, view->mode);

	DELETE_EVAS_OBJECT(view->more_ctxpopup);

	if (!view->b_editmode && !view->b_searchmode) {

		const char *account_type = EMAIL_BUNDLE_VAL_SINGLE_ACCOUNT;
		if (view->mode == EMAIL_MAILBOX_MODE_PRIORITY_SENDER) {
			account_type = EMAIL_BUNDLE_VAL_PRIORITY_SENDER;
		} else if (view->mode == EMAIL_MAILBOX_MODE_ALL && view->account_id == 0 && EMAIL_MAILBOX_TYPE_FLAGGED == view->mailbox_type) {
			account_type = EMAIL_BUNDLE_VAL_FILTER_INBOX;
		} else if (view->account_id == 0) {
			account_type = EMAIL_BUNDLE_VAL_ALL_ACCOUNT;
		}

		email_params_h params = NULL;

		if (email_params_create(&params) &&
			email_params_add_int(params, EMAIL_BUNDLE_KEY_ACCOUNT_ID, view->account_id) &&
			email_params_add_int(params, EMAIL_BUNDLE_KEY_IS_MAILBOX_MOVE_MODE, 0) &&
			email_params_add_int(params, EMAIL_BUNDLE_KEY_IS_MAILBOX_ACCOUNT_MODE, 1) &&
			email_params_add_int(params, EMAIL_BUNDLE_KEY_MAILBOX_TYPE, view->mailbox_type) &&
			email_params_add_int(params, EMAIL_BUNDLE_KEY_MAILBOX, view->mailbox_id) &&
			email_params_add_str(params, EMAIL_BUNDLE_KEY_ACCOUNT_TYPE, account_type)) {

			view->account = mailbox_account_module_create(view, EMAIL_MODULE_ACCOUNT, params);
		}

		email_params_free(&params);
	} else {
		debug_log("account couldn't open. Edit/Search mode");
	}

}

static void _mailbox_on_back_key(email_view_t *self)
{
	debug_enter();
	retm_if(!self, "self == NULL");

	EmailMailboxView *view = (EmailMailboxView *)self;
	retm_if(!view->initialized, "Not initialized!");

	int account_count = 0;
	email_account_t *account_list = NULL;

	if (view->b_editmode) {
		mailbox_exit_edit_mode(view);
	} else if (view->b_searchmode) {
		if (view->search_type == EMAIL_SEARCH_IN_ALL_FOLDERS) {
			mailbox_set_search_mode(view, EMAIL_SEARCH_IN_SINGLE_FOLDER);
		} else {
			mailbox_set_search_mode(view, EMAIL_SEARCH_NONE);
		}
	} else {
		if ((view->mode == EMAIL_MAILBOX_MODE_ALL || view->mode == EMAIL_MAILBOX_MODE_MAILBOX)
			&& view->mailbox_type == EMAIL_MAILBOX_TYPE_INBOX) {
			elm_win_lower(view->base.module->win);
		} else {
			mailbox_sync_cancel_all(view);

			if (view->mode == EMAIL_MAILBOX_MODE_MAILBOX) {
				email_mailbox_t *mailbox = NULL;
				if (email_engine_get_mailbox_by_mailbox_type(view->account_id, EMAIL_MAILBOX_TYPE_INBOX, &mailbox)) {
					view->mailbox_id = mailbox->mailbox_id;
					view->mailbox_type = EMAIL_MAILBOX_TYPE_INBOX;
					email_engine_free_mailbox_list(&mailbox, 1);
				} else {
					debug_warning("email_engine_get_mailbox_by_mailbox_type failed");
					view->mailbox_id = 0;
					view->mailbox_type = EMAIL_MAILBOX_TYPE_INBOX;
				}
			} else {
				if (FALSE == email_engine_get_account_list(&account_count, &account_list)) {
					debug_warning("email_engine_get_account_list return fail. %d", account_count);
				}
				debug_log("account_count:%d", account_count);

				if (account_count == 1 && account_list) {
					email_mailbox_t *mailbox = NULL;
					view->account_id = account_list[0].account_id;
					if (email_engine_get_mailbox_by_mailbox_type(view->account_id, EMAIL_MAILBOX_TYPE_INBOX, &mailbox)) {
						debug_log("account_id:%d, account_count:%d, mailbox_id:%d", view->account_id, account_count, mailbox->mailbox_id);
						view->mailbox_id = mailbox->mailbox_id;
						view->mode = EMAIL_MAILBOX_MODE_MAILBOX;
						view->mailbox_type = EMAIL_MAILBOX_TYPE_INBOX;
						email_engine_free_mailbox_list(&mailbox, 1);
					} else {
						debug_warning("email_engine_get_mailbox_by_mailbox_type failed");
						view->mailbox_id = 0;
						view->mode = EMAIL_MAILBOX_MODE_MAILBOX;
						view->mailbox_type = EMAIL_MAILBOX_TYPE_INBOX;
					}
				} else {
					view->mode = EMAIL_MAILBOX_MODE_ALL;
					view->mailbox_type = EMAIL_MAILBOX_TYPE_INBOX;
				}

				if (account_list)
					email_engine_free_account_list(&account_list, account_count);
			}

			mailbox_view_title_update_all(view);
			mailbox_update_notifications_status(view);
			mailbox_list_refresh(view, NULL);
		}
	}
}


/*
 * Definition for exported functions
 */

#ifdef SHARED_MODULES_FEATURE
EMAIL_API email_module_t *email_module_alloc()
#else
email_module_t *mailbox_module_alloc()
#endif
{
	debug_enter();

	EmailMailboxModule *module = MEM_ALLOC(module, 1);
	if (!module) {
		debug_error("module is NULL");
		return NULL;
	}

	module->base.create = _mailbox_module_create;
	module->base.destroy = _mailbox_module_destroy;
	module->base.on_message = _mailbox_on_message;

	debug_leave();
	return &module->base;
}

void mailbox_create_no_contents_view(EmailMailboxView *view, bool search_mode)
{
	debug_log("search_mode : %d", search_mode);

	if (!view) {
		debug_warning("view is NULL");
		return;
	}

	Evas_Object *no_content_scroller = elm_object_part_content_get(view->content_layout, "noc");
	if (no_content_scroller) {
		debug_log("remove no content view");
		elm_object_part_content_unset(view->content_layout, "noc");
		evas_object_del(no_content_scroller);
		no_content_scroller = NULL;
	}

	Evas_Object *no_content = NULL;
	no_content = elm_layout_add(view->content_layout);

	if (search_mode) {
		elm_layout_theme_set(no_content, "layout", "nocontents", "search");
		elm_object_domain_translatable_part_text_set(no_content, "elm.text", PACKAGE, "IDS_EMAIL_NPBODY_NO_RESULTS_FOUND");
	} else {
		elm_layout_theme_set(no_content, "layout", "nocontents", "default");
		elm_object_domain_translatable_part_text_set(no_content, "elm.text", PACKAGE, "IDS_EMAIL_BODY_NO_EMAILS");
	}
	evas_object_size_hint_align_set(no_content, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(no_content, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	/* Added Scrollar to fix the searchbar not showing in landscape mode completely*/
	no_content_scroller = elm_scroller_add(view->content_layout);
	elm_scroller_policy_set(no_content_scroller, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
	elm_object_focus_allow_set(no_content_scroller, EINA_FALSE);
	evas_object_show(no_content_scroller);
	elm_object_content_set(no_content_scroller, no_content);

	elm_layout_signal_emit(no_content, "text,disabled", "");
	elm_layout_signal_emit(no_content, "align.center", "elm");

	elm_object_part_content_set(view->content_layout, "noc", no_content_scroller);

	if (view->no_content_shown && !search_mode) {
		mailbox_hide_no_contents_view(view);
	}
}

void mailbox_show_no_contents_view(EmailMailboxView *view)
{
	debug_enter();

	if (evas_object_visible_get(view->gl))
		evas_object_hide(view->gl);

	edje_object_signal_emit(_EDJ(view->sub_layout_search), "hide_list", "elm");
	edje_object_signal_emit(_EDJ(view->sub_layout_search), "show_noc", "elm");

	mailbox_send_all_btn_remove(view);

	Evas_Object *no_content_scroller = elm_object_part_content_get(view->content_layout, "noc");
	Evas_Object *no_content = elm_object_content_get(no_content_scroller);

	if (view->b_searchmode == true) {
		elm_layout_theme_set(no_content, "layout", "nocontents", "search");
		elm_object_domain_translatable_part_text_set(no_content, "elm.text", PACKAGE, "IDS_EMAIL_NPBODY_NO_RESULTS_FOUND");
		elm_object_domain_translatable_part_text_set(no_content, "elm.help.text", PACKAGE, "");
	} else {
		elm_layout_theme_set(no_content, "layout", "nocontents", "default");
		elm_object_domain_translatable_part_text_set(no_content, "elm.text", PACKAGE, "IDS_EMAIL_NPBODY_NO_EMAILS");

		if (view->mode == EMAIL_MAILBOX_MODE_PRIORITY_SENDER) {
			elm_object_domain_translatable_part_text_set(no_content, "elm.help.text", PACKAGE, "IDS_EMAIL_BODY_AFTER_YOU_ADD_PRIORITY_SENDERS_EMAILS_FROM_THEM_WILL_BE_SHOWN_HERE");
		} else if (view->mode == EMAIL_MAILBOX_MODE_ALL && view->mailbox_type == EMAIL_MAILBOX_TYPE_INBOX) {
			elm_object_domain_translatable_part_text_set(no_content, "elm.help.text", PACKAGE, "IDS_EMAIL_BODY_AFTER_YOU_RECEIVE_EMAILS_THEY_WILL_BE_SHOWN_HERE");
		}  else if (view->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX) {
			elm_object_domain_translatable_part_text_set(no_content, "elm.help.text", PACKAGE, "IDS_EMAIL_BODY_AFTER_YOU_HAVE_TRIED_TO_SEND_EMAILS_BUT_THEY_FAILED_TO_BE_SENT_THEY_WILL_BE_SHOWN_HERE");
		} else if (view->mailbox_type == EMAIL_MAILBOX_TYPE_SENTBOX) {
			elm_object_domain_translatable_part_text_set(no_content, "elm.help.text", PACKAGE, "IDS_EMAIL_BODY_AFTER_YOU_SEND_EMAILS_THEY_WILL_BE_SHOWN_HERE");
		} else if (view->mailbox_type == EMAIL_MAILBOX_TYPE_TRASH) {
			elm_object_domain_translatable_part_text_set(no_content, "elm.help.text", PACKAGE, "IDS_EMAIL_BODY_AFTER_YOU_DELETE_EMAILS_THEY_WILL_BE_SHOWN_HERE");
		} else if (view->mailbox_type == EMAIL_MAILBOX_TYPE_FLAGGED) {
			elm_object_domain_translatable_part_text_set(no_content, "elm.help.text", PACKAGE, "IDS_EMAIL_BODY_AFTER_YOU_ADD_FAVOURITE_EMAILS_BY_TAPPING_THE_STAR_ICON_THEY_WILL_BE_SHOWN_HERE");
		} else if (view->mailbox_type == EMAIL_MAILBOX_TYPE_INBOX) {
			elm_object_domain_translatable_part_text_set(no_content, "elm.help.text", PACKAGE, "IDS_EMAIL_BODY_AFTER_YOU_RECEIVE_EMAILS_THEY_WILL_BE_SHOWN_HERE");
		} else if (view->mailbox_type == EMAIL_MAILBOX_TYPE_DRAFT) {
			elm_object_domain_translatable_part_text_set(no_content, "elm.help.text", PACKAGE, "IDS_EMAIL_BODY_AFTER_YOU_SAVE_EMAILS_IN_DRAFTS_THEY_WILL_BE_SHOWN_HERE");
		} else if (view->mailbox_type == EMAIL_MAILBOX_TYPE_SPAMBOX) {
			elm_object_domain_translatable_part_text_set(no_content, "elm.help.text", PACKAGE, "IDS_EMAIL_BODY_AFTER_YOU_ADD_EMAILS_TO_SPAM_THEY_WILL_BE_SHOWN_HERE");
		} else {
			elm_object_domain_translatable_part_text_set(no_content, "elm.help.text", PACKAGE, "IDS_EMAIL_BODY_AFTER_YOU_MOVE_EMAILS_TO_THIS_FOLDER_THEY_WILL_BE_SHOWN_HERE");
		}
	}
	elm_layout_signal_emit(no_content, "text,disabled", "");
	elm_layout_signal_emit(no_content, "align.center", "elm");

	view->no_content_shown = true;
}

void mailbox_hide_no_contents_view(EmailMailboxView *view)
{
	debug_enter();

	if (!evas_object_visible_get(view->gl))
		evas_object_show(view->gl);

	edje_object_signal_emit(_EDJ(view->sub_layout_search), "hide_noc", "elm");
	edje_object_signal_emit(_EDJ(view->sub_layout_search), "show_list", "elm");
	view->no_content_shown = false;
}

void mailbox_refresh_fullview(EmailMailboxView *view, bool update_title)
{
	debug_enter();
	retm_if(!view, "view == NULL");

	DELETE_EVAS_OBJECT(view->more_ctxpopup);

	if (view->b_editmode)
		mailbox_exit_edit_mode(view);

	if (update_title)
		mailbox_view_title_update_all(view);

	if (view->b_searchmode)
		mailbox_show_search_result(view);
	else
		mailbox_list_refresh(view, NULL);

}

void mailbox_naviframe_mailbox_button_remove(EmailMailboxView *view)
{

	elm_object_part_content_unset(view->title_layout, "mailbox_button");
	evas_object_hide(view->btn_mailbox);

	return;
}

void mailbox_naviframe_mailbox_button_add(EmailMailboxView *view)
{
	if (!view->btn_mailbox) {
		view->btn_mailbox = elm_button_add(view->title_layout);
		elm_object_style_set(view->btn_mailbox, "naviframe/title_right");
		evas_object_smart_callback_add(view->btn_mailbox, "clicked", _mailbox_title_clicked_cb, view);
	}
	elm_object_domain_translatable_text_set(view->btn_mailbox, PACKAGE, "IDS_EMAIL_ACBUTTON_MAILBOX_ABB");
	elm_object_part_content_set(view->title_layout, "mailbox_button", view->btn_mailbox);
	evas_object_show(view->btn_mailbox);

	return;
}

void mailbox_update_notifications_status(EmailMailboxView *view)
{
	debug_enter();
	retm_if(!view, "view == NULL");

	if ((view->base.state == EV_STATE_ACTIVE) &&
		(view->mailbox_type == EMAIL_MAILBOX_TYPE_INBOX) &&
		(view->mode != EMAIL_MAILBOX_MODE_PRIORITY_SENDER)) {
		email_set_is_inbox_active(true);
		if (view->mode == EMAIL_MAILBOX_MODE_MAILBOX) {
			email_engine_clear_notification_bar(view->account_id);
		} else {
			email_engine_clear_notification_bar(ALL_ACCOUNT);
		}
	} else {
		email_set_is_inbox_active(false);
	}

	debug_leave();
}

int mailbox_open_email_viewer(EmailMailboxView *view, int account_id, int mailbox_id, int mail_id)
{
	debug_enter();
	retvm_if(!view, -1, "view is NULL");
	debug_log("account_id: %d, mailbox_id: %d, mail_id: %d", account_id, mailbox_id, mail_id);

	email_params_h params = NULL;

	if (email_params_create(&params) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_ACCOUNT_ID, account_id) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_MAILBOX, mailbox_id) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_MAIL_ID, mail_id)) {

		view->opened_mail_id = mail_id;
		view->viewer = mailbox_viewer_module_create(view, EMAIL_MODULE_VIEWER, params);
	}

	email_params_free(&params);

	debug_leave();
	return 0;
}
