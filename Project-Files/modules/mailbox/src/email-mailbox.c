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

#ifndef UG_MODULE_API
#define UG_MODULE_API __attribute__ ((visibility("default")))
#endif

#include <stdio.h>
#include <stdlib.h>
#include <system_settings.h>
#include <app.h>
#include "email-mailbox.h"
#include "email-mailbox-toolbar.h"
#include "email-mailbox-list.h"
#include "email-mailbox-list-other-items.h"
#include "email-mailbox-search.h"
#include "email-mailbox-noti-mgr.h"
#include "email-mailbox-ug-util.h"
#include "email-mailbox-util.h"
#include "email-mailbox-more-menu.h"
#include "email-mailbox-sync.h"
#include "email-mailbox-title.h"
#include "email-mailbox-request.h"
#include "email-utils.h"

/*
 * Definitions
 */

#define EMAIL_SERIVE_PING_CHECK_INTERVAL_SEC 0.005
#define EMAIL_SERIVE_PING_CHECK_INTERVAL_USEC ((int)1000000 * EMAIL_SERIVE_PING_CHECK_INTERVAL_SEC + 0.5)
#define EMAIL_SERIVE_PING_TIMEOUT_SEC 0.1
#define EMAIL_SERIVE_PING_RETRY_DELAY_SEC 3

/*
 * Structures
 */


/*
 * Globals
 */


/*
 * Declaration for static functions
 */

static int _mailbox_module_create(email_module_t *self, app_control_h params);
static void _mailbox_module_destroy(email_module_t *self);
static void _mailbox_on_message(email_module_t *self, app_control_h msg);

static void _mailbox_email_service_ping_thread_heavy_cb(void *data, Ecore_Thread *thread);
static void _mailbox_email_service_ping_thread_notify_cb(void *data, Ecore_Thread *thread, void *msg_data);

static int _mailbox_module_create_view(EmailMailboxModule *md);
static int _mailbox_create_account_setting_module(EmailMailboxUGD *mailbox_ugd);

static int _mailbox_create(email_view_t *self);
static void _mailbox_destroy(email_view_t *self);
static void _mailbox_activate(email_view_t *self, email_view_state prev_state);
static void _mailbox_deactivate(email_view_t *self);
static void _mailbox_update(email_view_t *self, int flags);
static void _mailbox_on_back_key(email_view_t *self);

static int _mailbox_initialize(EmailMailboxUGD *mailbox_ugd);
static void _mailbox_finalize(EmailMailboxUGD *mailbox_ugd);

static email_run_type_e _mailbox_params_get_run_type(app_control_h params);
static int _mailbox_params_get_account_id(app_control_h params);
static int _mailbox_params_get_mail_id(app_control_h params);

static int _mailbox_handle_launch_mailbox_bundle_val(EmailMailboxUGD *mailbox_ugd, app_control_h msg);
static int _mailbox_handle_launch_viewer_bundle_val(EmailMailboxUGD *mailbox_ugd, app_control_h msg);

static void _mailbox_create_view(EmailMailboxUGD *mailbox_ugd);
static void _mailbox_delete_evas_object(EmailMailboxUGD *mailbox_ugd);
static void _mailbox_timezone_change_cb(system_settings_key_e key, void *data);
static void _mailbox_sys_settings_datetime_format_changed_cb(system_settings_key_e node, void *data);
static void _mailbox_vip_rule_value_changed_cb(const char *key, void *data);

static void _mailbox_title_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static void _mailbox_module_show_initialising_popup(EmailMailboxModule *md);
static void _mailbox_initialising_popup_back_cb(void *data, Evas_Object *obj, void *event_info);
static int _mailbox_push_base_view_layout(EmailMailboxUGD *mailbox_ugd);

/*
 * Definition for static functions
 */

static int _mailbox_module_create(email_module_t *self, app_control_h params)
{
	debug_enter();
	retvm_if(!self, -1, "self is NULL");

	EmailMailboxModule *md = (EmailMailboxModule *)self;

	md->view.run_type = _mailbox_params_get_run_type(params);

	if (md->view.run_type == RUN_MAILBOX_FROM_NOTIFICATION) {
		md->view.account_id = _mailbox_params_get_account_id(params);
	} else if (md->view.run_type == RUN_VIEWER_FROM_NOTIFICATION) {
		md->view.start_mail_id = _mailbox_params_get_mail_id(params);
	}

	md->start_thread = ecore_thread_feedback_run(_mailbox_email_service_ping_thread_heavy_cb,
			_mailbox_email_service_ping_thread_notify_cb, NULL, NULL, md, EINA_TRUE);
	if (!md->start_thread) {
		debug_error("Start thread create failed!");
		return -1;
	}

	double wait_time = 0.0;

	while (!md->start_thread_done) {
		usleep(EMAIL_SERIVE_PING_CHECK_INTERVAL_USEC);
		wait_time += EMAIL_SERIVE_PING_CHECK_INTERVAL_SEC;
		if (wait_time >= EMAIL_SERIVE_PING_TIMEOUT_SEC) {
			_mailbox_module_show_initialising_popup(md);
			debug_leave();
			return 0;
		}
	}

	_mailbox_email_service_ping_thread_notify_cb(md, NULL, NULL);

	debug_leave();
	return 0;
}

static void _mailbox_module_destroy(email_module_t *self)
{
	debug_enter();
	retm_if(!self, "self is NULL");

	EmailMailboxModule *md = (EmailMailboxModule *)self;

	if (md->start_thread) {
		ecore_thread_cancel(md->start_thread);
		md->start_thread = NULL;
	}

	debug_leave();
}

static void _mailbox_email_service_ping_thread_heavy_cb(void *data, Ecore_Thread *thread)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailMailboxModule *md = data;

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
			md->start_thread_done = true;
		}

		email_engine_finalize_force();
	}

	debug_leave();
}

static void _mailbox_email_service_ping_thread_notify_cb(void *data, Ecore_Thread *thread, void *msg_data)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailMailboxModule *md = data;

	if (md->start_thread) {
		md->start_thread = NULL;
		if (_mailbox_module_create_view(md) != 0) {
			debug_error("_mailbox_module_create_view() failed!");
			email_module_make_destroy_request(&md->base);
			return;
		}
		DELETE_EVAS_OBJECT(md->init_pupup);
	}

	debug_leave();
}

static int _mailbox_module_create_view(EmailMailboxModule *md)
{
	debug_enter();
	retvm_if(!md, -1, "md is NULL");

	int ret = 0;

	md->view.base.create = _mailbox_create;
	md->view.base.destroy = _mailbox_destroy;
	md->view.base.activate = _mailbox_activate;
	md->view.base.deactivate = _mailbox_deactivate;
	md->view.base.update = _mailbox_update;
	md->view.base.on_back_key = _mailbox_on_back_key;

	ret = email_module_create_view(&md->base, &md->view.base);
	if (ret != 0) {
		debug_error("email_module_create_view(): failed (%d)", ret);
		return -1;
	}

	debug_leave();
	return 0;
}

static int _mailbox_create_account_setting_module(EmailMailboxUGD *mailbox_ugd)
{
	debug_enter();
	retvm_if(!mailbox_ugd, -1, "mailbox_ugd is NULL");

	app_control_h service = NULL;
	int ret = APP_CONTROL_ERROR_NONE;

	ret = app_control_create(&service);
	retvm_if(ret != APP_CONTROL_ERROR_NONE, -1, "app_control_create() failed! ret: [%d]", ret);

	ret = app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_VIEW_TYPE, EMAIL_BUNDLE_VAL_VIEW_ACCOUNT_SETUP);
	if (ret != APP_CONTROL_ERROR_NONE) {
		debug_error("app_control_add_extra_data() failed! ret: [%d]", ret);
	} else {
		mailbox_ugd->setting = mailbox_setting_module_create(mailbox_ugd, EMAIL_MODULE_SETTING, service);
	}

	app_control_destroy(service);

	retvm_if(!mailbox_ugd->setting, -1, "mailbox_setting_module_create() failed!");

	debug_leave();
	return 0;
}

static email_run_type_e _mailbox_params_get_run_type(app_control_h params)
{
	debug_enter();

	email_run_type_e result = RUN_TYPE_UNKNOWN;

	char *operation = NULL;
	bool is_operation_default = false;

	if ((app_control_get_operation(params, &operation) != APP_CONTROL_ERROR_NONE) || !operation) {
		return result;
	}

	if (strcmp(operation, APP_CONTROL_OPERATION_DEFAULT) == 0) {
		is_operation_default = true;
	} else {
		debug_warning("Operation is not supported: %s", operation);
	}

	free(operation);

	if (is_operation_default) {
		if (email_params_get_int_opt(params, EMAIL_BUNDLE_KEY_RUN_TYPE, &result)) {
			if ((result != RUN_TYPE_UNKNOWN) && (result != RUN_VIEWER_FROM_NOTIFICATION) &&
				(result != RUN_MAILBOX_FROM_NOTIFICATION)) {
				debug_warning("Run type is not supported: %d", (int)result);
				result = RUN_TYPE_UNKNOWN;
			}
		} else {
			debug_error("email_params_get_int_opt() failed!");
		}
	}

	debug_leave();
	return result;
}

static int _mailbox_params_get_account_id(app_control_h params)
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

static int _mailbox_params_get_mail_id(app_control_h params)
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

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)self;

	int ret = 0;
	int default_account_id = 0;
	email_run_type_e run_type = mailbox_ugd->run_type;

	if (!email_engine_initialize()) {
		debug_error("Failed to initialize email engine");
		return -1;
	}

	ret = _mailbox_push_base_view_layout(mailbox_ugd);
	retvm_if(ret != 0, -1, "_mailbox_push_base_view_layout() failed! ret: [%d]", ret);

	if (!email_engine_get_default_account(&default_account_id)) {
		debug_log("No default account. Creating account setup setting view...");
		mailbox_ugd->account_id = 0;
		return _mailbox_create_account_setting_module(mailbox_ugd);
	}

	mailbox_ugd->run_type = RUN_TYPE_UNKNOWN;
	mailbox_ugd->b_ready_for_initialize = true;

	if (run_type == RUN_VIEWER_FROM_NOTIFICATION) {
		email_mail_list_item_t *mail = email_engine_get_mail_by_mailid(mailbox_ugd->start_mail_id);
		retvm_if(!mail, -1, "email_engine_get_mail_by_mailid() failed!");
		mailbox_ugd->account_id = mail->account_id;
		mailbox_ugd->mailbox_id = mail->mailbox_id;
		return mailbox_open_email_viewer(mailbox_ugd, mail->account_id, mail->mailbox_id, mailbox_ugd->start_mail_id);
	}

	debug_leave();
	return _mailbox_initialize(mailbox_ugd);
}

static int _mailbox_initialize(EmailMailboxUGD *mailbox_ugd)
{
	debug_enter();
	retvm_if(!mailbox_ugd, -1, "mailbox_ugd is NULL");
	retvm_if(!mailbox_ugd->b_ready_for_initialize, -1, "mailbox_ugd->b_ready_for_initialize is false");

	/* init members of mailbox */
	mailbox_ugd->initialized = true;
	mailbox_ugd->account_count = 0;
	mailbox_ugd->mailbox_type = EMAIL_MAILBOX_TYPE_NONE;
	mailbox_ugd->sort_type = EMAIL_SORT_DATE_RECENT;
	mailbox_ugd->sort_type_index = SORTBY_DATE_RECENT;
	mailbox_ugd->only_local = false;
	mailbox_ugd->gl = NULL;
	mailbox_ugd->mail_list = NULL;
	mailbox_ugd->b_editmode = false;
	mailbox_ugd->g_sending_mail_list = NULL;
	mailbox_ugd->is_send_all_run = false;
	mailbox_ugd->account_type = EMAIL_SERVER_TYPE_NONE;
	mailbox_ugd->b_format_24hour = false;

	/* DBUS */
	mailbox_setup_dbus_receiver(mailbox_ugd);

	system_settings_get_value_bool(
			SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR,
			&mailbox_ugd->b_format_24hour);


	mailbox_ugd->request_queue = email_request_queue_create();
	/* register thread callbacks for mailbox thread operation */
	mailbox_requests_cbs_register();

	/* create accouts list view */
	_mailbox_create_view(mailbox_ugd);

	int account_count = 0;
	email_account_t *account_list = NULL;

	if (email_engine_get_account_list(&account_count, &account_list))
		mailbox_ugd->account_count = account_count;
	else
		mailbox_ugd->account_count = 0;

	if (account_list && (mailbox_ugd->account_count > 0)) {
		int i;
		for (i = 0; i < account_count; i++) {
			mailbox_account_color_list_add(mailbox_ugd, account_list[i].account_id, account_list[i].color_label);
		}
	}

	if ((mailbox_ugd->account_id == 0 && mailbox_ugd->account_count > 1)
			|| (mailbox_ugd->account_count == 0)) {
		mailbox_ugd->account_id = 0;
		mailbox_ugd->mode = EMAIL_MAILBOX_MODE_ALL;
		mailbox_ugd->mailbox_type = EMAIL_MAILBOX_TYPE_INBOX;
	} else {
		int err = 0;
		email_mailbox_t *mailbox = NULL;

		mailbox_ugd->mode = EMAIL_MAILBOX_MODE_MAILBOX;
		if (mailbox_ugd->account_id == 0 && account_list) {
			mailbox_ugd->account_id = account_list[0].account_id; /* If one account is only registered, launch mailbox mode by using first account id. */
			mailbox_ugd->mailbox_id = 0;
		}

		if (mailbox_ugd->mailbox_id == 0) {
			err = email_get_mailbox_by_mailbox_type(mailbox_ugd->account_id, EMAIL_MAILBOX_TYPE_INBOX, &mailbox);
			if (err == EMAIL_ERROR_NONE && mailbox) {
				mailbox_ugd->mailbox_id = mailbox->mailbox_id;
				email_free_mailbox(&mailbox, 1);
			} else {
				mailbox_ugd->mailbox_id = 0;
				debug_warning("email_get_mailbox_by_mailbox_type : account_id(%d) type(INBOX) - err(%d) or mailbox is NULL(%p)", mailbox_ugd->account_id, err, mailbox);
			}
		}
	}

	if (account_list)
		email_engine_free_account_list(&account_list, account_count);

	mailbox_view_title_update_all(mailbox_ugd);
	mailbox_list_refresh(mailbox_ugd, NULL);

	/* register callbacks */
	system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR, _mailbox_sys_settings_datetime_format_changed_cb, mailbox_ugd);
	system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_LOCALE_COUNTRY, _mailbox_sys_settings_datetime_format_changed_cb, mailbox_ugd);

	mailbox_list_system_settings_callback_register(mailbox_ugd);

	email_register_timezone_changed_callback(_mailbox_timezone_change_cb, mailbox_ugd);

	return 0;
}

static void _mailbox_activate(email_view_t *self, email_view_state prev_state)
{
	debug_enter();
	retm_if(!self, "self is NULL");

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)self;

	if (mailbox_ugd->initialized) {
		if (!mailbox_ugd->started) {
			mailbox_ugd->started = true;

			int res = email_update_vip_rule_value();
			if (res != 0) {
				debug_error("email_update_vip_rule_value failed. err = %d", res);
			}

			res = email_get_vip_rule_value(&mailbox_ugd->vip_rule_value);
			if (res != 0) {
				debug_error("email_get_vip_rule_value failed. err = %d", res);
			}

			res = email_set_vip_rule_change_callback(_mailbox_vip_rule_value_changed_cb, mailbox_ugd);
			if (res != 0) {
				debug_error("email_set_vip_rule_change_callback failed. err = %d", res);
			}

			/* download new mails */
			if (mailbox_sync_current_mailbox(mailbox_ugd))
			{
				debug_log("Start sync");
			}
		}

		if (mailbox_ugd->remaining_req) {
			mailbox_list_make_remaining_items_in_thread(mailbox_ugd, mailbox_ugd->remaining_req);
			mailbox_ugd->remaining_req = NULL;
		}

		mailbox_update_notifications_status(mailbox_ugd);
	}

	debug_leave();
}

static void _mailbox_deactivate(email_view_t *self)
{
	debug_enter();
	retm_if(!self, "self is NULL");

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)self;

	if (mailbox_ugd->initialized) {
		mailbox_update_notifications_status(mailbox_ugd);
	}

	debug_leave();
}

static void _mailbox_update(email_view_t *self, int flags)
{
	debug_enter();
	retm_if(!self, "self is NULL");

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)self;

	debug_log("flags: %d", flags);

	bool need_update = false;
	bool update_title = false;

	if (!mailbox_ugd->initialized) {
		if (flags & EVUF_POPPING) {
			int default_account_id = 0;
			if (!email_engine_get_default_account(&default_account_id)) {
				debug_log("No default account. Terminating...");
				email_module_make_destroy_request(mailbox_ugd->base.module);
			} else {
				mailbox_ugd->b_ready_for_initialize = true;
				if (_mailbox_initialize(mailbox_ugd) != 0) {
					debug_error("Application initialization failed. Terminating...");
					email_module_make_destroy_request(mailbox_ugd->base.module);
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
			if (mailbox_ugd->vip_rule_value != viprule_val &&
					mailbox_ugd->mode == EMAIL_MAILBOX_MODE_PRIORITY_SENDER) {
				debug_log("Mailbox list should be refreshed because vip setting is changed, priority sender view");
				need_update = true;
				update_title = true;
			}
#endif
			mailbox_ugd->vip_rule_value = viprule_val;
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
		mailbox_refresh_fullview(mailbox_ugd, update_title);
	}
}

static void _mailbox_destroy(email_view_t *self)
{
	debug_enter();
	retm_if(!self, "self is NULL");

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)self;

	_mailbox_finalize(mailbox_ugd);

	email_engine_finalize();

	debug_leave();
}

static void _mailbox_finalize(EmailMailboxUGD *mailbox_ugd)
{
	debug_enter();
	retm_if(!mailbox_ugd, "mailbox_ugd is NULL");

	if (!mailbox_ugd->initialized) {
		return;
	}
	mailbox_ugd->initialized = false;

	if(mailbox_ugd->search_entry_focus_idler) {
		ecore_idler_del(mailbox_ugd->search_entry_focus_idler);
		mailbox_ugd->search_entry_focus_idler = NULL;
	}

	mailbox_sync_cancel_all(mailbox_ugd);

	_mailbox_delete_evas_object(mailbox_ugd);

	mailbox_list_free_all_item_class_data(mailbox_ugd);

	if (mailbox_ugd->request_queue) {
		email_request_queue_destroy(mailbox_ugd->request_queue);
		mailbox_ugd->request_queue = NULL;
	}
	mailbox_requests_cbs_unregister();

	mailbox_list_free_all_item_data(mailbox_ugd);

	mailbox_sending_mail_list_free(mailbox_ugd);
	mailbox_account_color_list_free(mailbox_ugd);

	G_FREE(mailbox_ugd->mailbox_alias);
	G_FREE(mailbox_ugd->account_name);
	FREE(mailbox_ugd->last_updated_time);
	G_FREE(mailbox_ugd->moved_mailbox_name);

	email_unregister_timezone_changed_callback(_mailbox_timezone_change_cb);

	system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR);
	system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_LOCALE_COUNTRY);

	int res = email_unset_vip_rule_change_callback();
	if (res != 0) {
		debug_error("email_unset_vip_rule_change_callback failed. err = %d", res);
	}

	elm_theme_extension_del(mailbox_ugd->theme, email_get_mailbox_theme_path());
	elm_theme_free(mailbox_ugd->theme);

	mailbox_list_system_settings_callback_unregister();

	mailbox_remove_dbus_receiver();

}

static void _mailbox_on_message(email_module_t *self, app_control_h msg)
{
	debug_enter();
	retm_if(!self, "self is NULL");

	EmailMailboxModule *md = (EmailMailboxModule *)self;
	EmailMailboxUGD *mailbox_ugd = &md->view;
	email_run_type_e run_type = _mailbox_params_get_run_type(msg);

	if (!mailbox_ugd->initialized && (_mailbox_initialize(mailbox_ugd) != 0)) {
		debug_error("Initialize failed!");
		return;
	}

	if (run_type == RUN_MAILBOX_FROM_NOTIFICATION) {
		_mailbox_handle_launch_mailbox_bundle_val(mailbox_ugd, msg);
	} else if (run_type == RUN_VIEWER_FROM_NOTIFICATION) {
		_mailbox_handle_launch_viewer_bundle_val(mailbox_ugd, msg);
	} else {
		debug_log("Unknown msg type");
	}

	debug_leave();
	return;
}

int mailbox_handle_next_msg_bundle(EmailMailboxUGD *mailbox_ugd, app_control_h msg)
{
	debug_enter();
	retvm_if(!mailbox_ugd, -1, "Error: mailbox_ugd is NULL");

	debug_log("EMAIL_BUNDLE_VAL_NEXT_MSG");

	if (!mailbox_ugd->initialized && (_mailbox_initialize(mailbox_ugd) != 0)) {
		debug_error("Initialize failed!");
		return -1;
	}

	MailItemData *ld = NULL;
	GList *first = g_list_first(mailbox_ugd->mail_list);
	GList *last = g_list_last(mailbox_ugd->mail_list);
	int ret;
	char *s_current_mail_index = NULL;
	int current_mail_index = -1;

	int cnt = g_list_length(mailbox_ugd->mail_list);
	debug_log("first %p, last %p, cnt: %d", first, last, cnt);

	ret = app_control_get_extra_data(msg, EMAIL_BUNDLE_KEY_MAIL_INDEX, &s_current_mail_index);
	debug_log("app_control_get_extra_data: %d, %s", ret, s_current_mail_index);
	if (s_current_mail_index) {
		current_mail_index = atoi(s_current_mail_index);
		g_free(s_current_mail_index);
	}

	GList *opened_mail = g_list_find_custom(mailbox_ugd->mail_list, GINT_TO_POINTER(mailbox_ugd->opened_mail_id), mailbox_compare_mailid_in_list);
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

int mailbox_handle_prev_msg_bundle(EmailMailboxUGD *mailbox_ugd, app_control_h msg)
{
	debug_enter();
	retvm_if(!mailbox_ugd, -1, "Error: mailbox_ugd is NULL");

	debug_log("EMAIL_BUNDLE_VAL_PREV_MSG");

	if (!mailbox_ugd->initialized && (_mailbox_initialize(mailbox_ugd) != 0)) {
		debug_error("Initialize failed!");
		return -1;
	}

	MailItemData *ld = NULL;
	GList *first = g_list_first(mailbox_ugd->mail_list);
	GList *last = g_list_last(mailbox_ugd->mail_list);
	int ret;
	char *s_current_mail_index = NULL;
	int current_mail_index = -1;

	int cnt = g_list_length(mailbox_ugd->mail_list);
	debug_log("first %p, last %p, cnt: %d", first, last, cnt);

	ret = app_control_get_extra_data(msg, EMAIL_BUNDLE_KEY_MAIL_INDEX, &s_current_mail_index);
	debug_log("app_control_get_extra_data: %d, %s", ret, s_current_mail_index);
	if (s_current_mail_index) {
		current_mail_index = atoi(s_current_mail_index);
		g_free(s_current_mail_index);
	}

	GList *opened_mail = g_list_find_custom(mailbox_ugd->mail_list, GINT_TO_POINTER(mailbox_ugd->opened_mail_id), mailbox_compare_mailid_in_list);
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

static int _mailbox_destroy_child_modules(EmailMailboxUGD *mailbox_ugd, bool keep_viewer)
{
	debug_enter();
	retvm_if(!mailbox_ugd, -1,  "Error: mailbox_ugd is NULL");

	app_control_h service = NULL;
	int ret = -1;

	if (mailbox_ugd->account) {
		mailbox_account_module_destroy(mailbox_ugd, mailbox_ugd->account);
	}

	if (mailbox_ugd->setting) {
		mailbox_setting_module_destroy(mailbox_ugd, mailbox_ugd->setting);
	}

	if (mailbox_ugd->viewer && !keep_viewer) {
		debug_log("Viewer is running. Need to destroy all viewer child modules.");

		if (email_params_create(&service) &&
			email_params_add_str(service, EMAIL_BUNDLE_KEY_MSG, EMAIL_BUNDLE_VAL_VIEWER_DESTROY_VIEW)) {
			ret = email_module_send_message(mailbox_ugd->viewer, service);
		}

		if (ret != 0) {
			debug_warning("Failed to send message to viewer. Forcing destroy.");
			mailbox_viewer_module_destroy(mailbox_ugd, mailbox_ugd->viewer);
		}
	}

	if (mailbox_ugd->composer) {
		debug_log("Asking composer to save in drafts.");

		if (email_params_create(&service) &&
			email_params_add_str(service, EMAIL_BUNDLE_KEY_MSG, EMAIL_BUNDLE_VAL_EMAIL_COMPOSER_SAVE_DRAFT)) {
			ret = email_module_send_message(mailbox_ugd->viewer, service);
		}

		if (ret != 0) {
			debug_log("Failed to send message to composer. Forcing destroy.");
			mailbox_composer_module_destroy(mailbox_ugd, mailbox_ugd->composer);
		}
	}

	email_params_free(&service);

	debug_leave();
	return 0;
}

static int _mailbox_update_mailbox(EmailMailboxUGD *mailbox_ugd, int account_id, int mailbox_id)
{
	debug_enter();
	bool need_update = true;

	if (account_id == mailbox_ugd->account_id) {
		if (mailbox_id == 0) {
			if (mailbox_ugd->mailbox_type == EMAIL_MAILBOX_TYPE_INBOX &&
				mailbox_ugd->mode != EMAIL_MAILBOX_MODE_PRIORITY_SENDER) {
				need_update = false;
			}
		} else {
			if (mailbox_ugd->mailbox_id == mailbox_id &&
				mailbox_ugd->mode == EMAIL_MAILBOX_MODE_MAILBOX) {
				need_update = false;
			}
		}
	}

	if (mailbox_ugd->b_editmode) {
		mailbox_exit_edit_mode(mailbox_ugd);
	}

	if (!need_update) {
		debug_log("the proper mailbox is shown now.");
		if (mailbox_ugd->b_searchmode) {
			mailbox_finish_search_mode(mailbox_ugd);
		}
		return 0;
	}

	if (account_id == 0) {
		mailbox_ugd->account_id = account_id;
		mailbox_ugd->mode = EMAIL_MAILBOX_MODE_ALL;
		mailbox_ugd->mailbox_type = EMAIL_MAILBOX_TYPE_INBOX;
		mailbox_ugd->only_local = FALSE;
	} else {
		int err = 0;
		email_mailbox_t *mailbox = NULL;

		mailbox_ugd->mode = EMAIL_MAILBOX_MODE_MAILBOX;
		mailbox_ugd->account_id = account_id;
		mailbox_ugd->mailbox_id = mailbox_id;

		if (mailbox_id == 0) {
			err = email_get_mailbox_by_mailbox_type(mailbox_ugd->account_id, EMAIL_MAILBOX_TYPE_INBOX, &mailbox);
			if ((err == EMAIL_ERROR_NONE) && mailbox) {
				mailbox_ugd->mailbox_id = mailbox->mailbox_id;
				email_free_mailbox(&mailbox, 1);
			} else {
				debug_warning("email_get_mailbox_by_mailbox_type : account_id(%d) type(INBOX) - err(%d) or mailbox is NULL(%p)", mailbox_ugd->account_id, err, mailbox);
				mailbox_ugd->mailbox_id = 0;
			}
		}
	}

	mailbox_view_title_update_all(mailbox_ugd);
	mailbox_update_notifications_status(mailbox_ugd);
	mailbox_check_sort_type_validation(mailbox_ugd);

	if (mailbox_ugd->b_searchmode) {
		mailbox_finish_search_mode(mailbox_ugd);
	} else {
		mailbox_list_refresh(mailbox_ugd, NULL);
	}

	debug_leave();
	return 0;
}

static int _mailbox_handle_launch_mailbox_bundle_val(EmailMailboxUGD *mailbox_ugd, app_control_h msg)
{
	debug_enter();
	retvm_if(!mailbox_ugd, -1,  "Error: mailbox_ugd is NULL");

	int account_id = 0;
	int ret = 0;

	account_id = _mailbox_params_get_account_id(msg);
	retvm_if(account_id <= 0, -1,  "(account_id <= 0) is not allowed");

	ret = _mailbox_destroy_child_modules(mailbox_ugd, false);
	retvm_if(ret != 0, -1,  "_mailbox_destroy_child_modules() failed!");

	ret = _mailbox_update_mailbox(mailbox_ugd, account_id, 0);
	retvm_if(ret != 0, -1,  "_mailbox_update_mailbox() failed!");

	debug_leave();
	return 0;
}

static int _mailbox_handle_launch_viewer_bundle_val(EmailMailboxUGD *mailbox_ugd, app_control_h msg)
{
	debug_enter();
	retvm_if(!mailbox_ugd, -1, "Error: mailbox_ugd is NULL");

	int mail_id = 0;
	email_mail_list_item_t *mail = NULL;
	int ret = -1;

	mail_id = _mailbox_params_get_mail_id(msg);
	retvm_if(mail_id <= 0, -1,  "(mail_id <= 0) is not allowed");

	if (mailbox_ugd->viewer && (mail_id == mailbox_ugd->opened_mail_id)) {
		app_control_h params = NULL;

		if (email_params_create(&params) &&
			email_params_add_str(params, EMAIL_BUNDLE_KEY_MSG, EMAIL_BUNDLE_VAL_VIEWER_RESTORE_VIEW)) {
			ret = email_module_send_message(mailbox_ugd->viewer, params);
		} else {
			ret = -1;
		}

		email_params_free(&params);

		retvm_if(ret != 0, -1,  "Failed to send message to viewer!");
		return 0;
	}

	mail = email_engine_get_mail_by_mailid(mail_id);
	retvm_if(!mail, -1,  "email_engine_get_mail_by_mailid() failed!");

	ret = _mailbox_destroy_child_modules(mailbox_ugd, true);
	retvm_if(ret != 0, -1,  "_mailbox_update_mailbox() failed!");

	ret = _mailbox_update_mailbox(mailbox_ugd, mail->account_id, mail->mailbox_id);
	retvm_if(ret != 0, -1,  "_mailbox_update_mailbox() failed!");

	if (!mailbox_ugd->composer) {
		ret = mailbox_open_email_viewer(mailbox_ugd, mail->account_id, mail->mailbox_id, mail_id);
		retvm_if(ret != 0, -1,  "mailbox_open_email_viewer() failed!");
	} else {
		mailbox_ugd->run_type = RUN_VIEWER_FROM_NOTIFICATION;
		mailbox_ugd->start_mail_id = mail_id;
	}

	debug_leave();
	return 0;
}

static void _mailbox_create_view(EmailMailboxUGD *mailbox_ugd)
{
	debug_enter();

	/*load theme*/
	mailbox_ugd->theme = elm_theme_new();
	elm_theme_ref_set(mailbox_ugd->theme, NULL); /* refer to default theme(NULL) */
	elm_theme_extension_add(mailbox_ugd->theme, email_get_mailbox_theme_path()); /* add extension to the new theme */

	/*base content*/
	mailbox_ugd->content_layout = elm_layout_add(mailbox_ugd->base.content);
	evas_object_size_hint_weight_set(mailbox_ugd->content_layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_layout_file_set(mailbox_ugd->content_layout, email_get_mailbox_theme_path(), "layout.email.mailbox.hd");
	elm_object_part_content_set(mailbox_ugd->base.content, "elm.swallow.content", mailbox_ugd->content_layout);

	/*title*/
	mailbox_ugd->title_layout = elm_layout_add(mailbox_ugd->base.content);
	evas_object_size_hint_weight_set(mailbox_ugd->title_layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_layout_file_set(mailbox_ugd->title_layout, email_get_mailbox_theme_path(), "email/layout/mailbox_title");
	elm_object_part_content_set(mailbox_ugd->content_layout, "top_bar", mailbox_ugd->title_layout);


	/* create genlist */
	mailbox_list_create_view(mailbox_ugd);

	/* create nocontents layout */
	mailbox_create_no_contents_view(mailbox_ugd, false);

	elm_object_part_content_set(mailbox_ugd->content_layout, "list", mailbox_ugd->gl);
	mailbox_ugd->sub_layout_search = mailbox_ugd->content_layout;

	mailbox_naviframe_mailbox_button_add(mailbox_ugd);

	/*Compose floatting button*/
	mailbox_create_compose_btn(mailbox_ugd);
}

static void _mailbox_module_show_initialising_popup(EmailMailboxModule *md)
{
	debug_enter();
	retm_if(!md, "md == NULL");

	Evas_Object *popup = elm_popup_add(md->base.win);
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

	eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, _mailbox_initialising_popup_back_cb, md);

	evas_object_show(popup);

	md->init_pupup = popup;
}

static void _mailbox_initialising_popup_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data == NULL");

	EmailMailboxModule *md = data;

	email_module_make_destroy_request(&md->base);
}

static int _mailbox_push_base_view_layout(EmailMailboxUGD *mailbox_ugd)
{
	debug_enter();
	retvm_if(!mailbox_ugd, -1, "mailbox_ugd == NULL");

	mailbox_ugd->base.content = elm_layout_add(mailbox_ugd->base.module->navi);
	elm_layout_theme_set(mailbox_ugd->base.content, "layout", "application", "default");
	evas_object_size_hint_weight_set(mailbox_ugd->base.content, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(mailbox_ugd->base.content);
	email_module_view_push(&mailbox_ugd->base, NULL, EVPF_NO_BACK_BUTTON | EVPF_NO_TITLE);

	debug_leave();
	return 0;
}

static void _mailbox_delete_evas_object(EmailMailboxUGD *mailbox_ugd)
{
	debug_enter();
	retm_if(!mailbox_ugd, "mailbox_ugd == NULL");

	DELETE_EVAS_OBJECT(mailbox_ugd->more_ctxpopup);
	DELETE_EVAS_OBJECT(mailbox_ugd->error_popup);
	DELETE_EVAS_OBJECT(mailbox_ugd->searchbar_ly);
	DELETE_EVAS_OBJECT(mailbox_ugd->title_layout);
	DELETE_EVAS_OBJECT(mailbox_ugd->compose_btn);
	DELETE_EVAS_OBJECT(mailbox_ugd->gl);
}

static void _mailbox_timezone_change_cb(system_settings_key_e key, void *data)
{
	debug_enter();
	retm_if(!data, "data == NULL");

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;

	mailbox_exit_edit_mode(mailbox_ugd);

	if (mailbox_ugd->last_updated_time_item)
		mailbox_last_updated_time_item_update(mailbox_ugd->mailbox_id, mailbox_ugd);

	mailbox_list_refresh(mailbox_ugd, NULL);
}

static void _mailbox_sys_settings_datetime_format_changed_cb(system_settings_key_e node, void *data)
{
	debug_enter();
	retm_if(!data, "data == NULL");

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;

	char *dt_fmt = email_get_datetime_format();

	/* This values is used on mailbox_last_updated_time_item_update function. */
	system_settings_get_value_bool(
			SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR,
			&mailbox_ugd->b_format_24hour);

	if (mailbox_ugd->last_updated_time_item)
		mailbox_last_updated_time_item_update(mailbox_ugd->mailbox_id, mailbox_ugd);

	FREE(dt_fmt);

}

static void _mailbox_vip_rule_value_changed_cb(const char *key, void *data)
{
	debug_enter();
	retm_if(!data, "data == NULL");

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;

	int res = email_get_vip_rule_value(&mailbox_ugd->vip_rule_value);
	if (res != 0) {
		debug_error("email_update_viprule_value failed. err = %d", res);
	}

#ifndef _FEATURE_PRIORITY_SENDER_DISABLE_
	if (mailbox_ugd->mode == EMAIL_MAILBOX_MODE_PRIORITY_SENDER) {
		debug_log("Mailbox list should be refreshed because vip setting is changed, priority sender view");
		mailbox_view_title_update_all(mailbox_ugd);
		mailbox_list_refresh(mailbox_ugd, NULL);
	}
#endif
}

static void _mailbox_title_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailMailboxUGD *mailbox_ugd = data;
	retm_if(mailbox_ugd->is_module_launching, "is_module_launching is true;");
	mailbox_ugd->is_module_launching = true;

	debug_log("account_id(%d), mode(%d)", mailbox_ugd->account_id, mailbox_ugd->mode);

	DELETE_EVAS_OBJECT(mailbox_ugd->more_ctxpopup);

	if (!mailbox_ugd->b_editmode && !mailbox_ugd->b_searchmode) {
		app_control_h service;
		if (APP_CONTROL_ERROR_NONE != app_control_create(&service)) {
			debug_log("creating service handle failed");
			mailbox_ugd->is_module_launching = false;
			return;
		}

		char acctid[30] = { 0, };
		snprintf(acctid, sizeof(acctid), "%d", mailbox_ugd->account_id);
		char is_account_ug[30] = { 0, };
		snprintf(is_account_ug, sizeof(is_account_ug), "%d", 1);
		char boxtype[30] = { 0, };
		snprintf(boxtype, sizeof(boxtype), "%d", mailbox_ugd->mailbox_type);
		char boxid[30] = { 0, };
		snprintf(boxid, sizeof(boxid), "%d", mailbox_ugd->mailbox_id);
		app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_ACCOUNT_ID, acctid);
		app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_IS_MAILBOX_MOVE_UG, 0);
		app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_IS_MAILBOX_ACCOUNT_UG, is_account_ug);
		app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_MAILBOX_TYPE, boxtype);
		app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_MAILBOX, boxid);

		if (mailbox_ugd->mode == EMAIL_MAILBOX_MODE_PRIORITY_SENDER) {
			app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_ACCOUNT_TYPE, EMAIL_BUNDLE_VAL_PRIORITY_SENDER);
		} else if (mailbox_ugd->mode == EMAIL_MAILBOX_MODE_ALL && mailbox_ugd->account_id == 0 && EMAIL_MAILBOX_TYPE_FLAGGED == mailbox_ugd->mailbox_type) {
			app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_ACCOUNT_TYPE, EMAIL_BUNDLE_VAL_FILTER_INBOX);
		} else if (mailbox_ugd->account_id == 0) {
			app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_ACCOUNT_TYPE, EMAIL_BUNDLE_VAL_ALL_ACCOUNT);
		} else {
			app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_ACCOUNT_TYPE, EMAIL_BUNDLE_VAL_SINGLE_ACCOUNT);
		}

		mailbox_ugd->account = mailbox_account_module_create(mailbox_ugd, EMAIL_MODULE_ACCOUNT, service);

		app_control_destroy(service);
	} else {
		debug_log("account couldn't open. Edit/Search mode");
	}

}

static void _mailbox_on_back_key(email_view_t *self)
{
	debug_enter();
	retm_if(!self, "self == NULL");

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)self;
	retm_if(!mailbox_ugd->initialized, "Not initialized!");

	int account_count = 0;
	email_account_t *account_list = NULL;

	if (mailbox_ugd->b_editmode) {
		mailbox_exit_edit_mode(mailbox_ugd);
	} else if (mailbox_ugd->b_searchmode) {
		mailbox_finish_search_mode(mailbox_ugd);
	} else {
		if ((mailbox_ugd->mode == EMAIL_MAILBOX_MODE_ALL || mailbox_ugd->mode == EMAIL_MAILBOX_MODE_MAILBOX)
			&& mailbox_ugd->mailbox_type == EMAIL_MAILBOX_TYPE_INBOX) {
			elm_win_lower(mailbox_ugd->base.module->win);
		} else {
			mailbox_sync_cancel_all(mailbox_ugd);

			if (mailbox_ugd->mode == EMAIL_MAILBOX_MODE_MAILBOX) {
				int ret = 0;
				email_mailbox_t *mailbox = NULL;
				ret = email_get_mailbox_by_mailbox_type(mailbox_ugd->account_id, EMAIL_MAILBOX_TYPE_INBOX, &mailbox);
				if (ret == EMAIL_ERROR_NONE && mailbox) {
					mailbox_ugd->mailbox_id = mailbox->mailbox_id;
					mailbox_ugd->mailbox_type = EMAIL_MAILBOX_TYPE_INBOX;
					email_free_mailbox(&mailbox, 1);
				} else {
					debug_log("email_get_mailbox_by_mailbox_type failed : %d", ret);
					mailbox_ugd->mailbox_id = 0;
					mailbox_ugd->mailbox_type = EMAIL_MAILBOX_TYPE_INBOX;
				}
			} else {
				if (FALSE == email_engine_get_account_list(&account_count, &account_list)) {
					debug_warning("email_engine_get_account_list return fail. %d", account_count);
				}
				debug_log("account_count:%d", account_count);

				if (account_count == 1 && account_list) {
					int ret = 0;
					email_mailbox_t *mailbox = NULL;
					mailbox_ugd->account_id = account_list[0].account_id;
					ret = email_get_mailbox_by_mailbox_type(mailbox_ugd->account_id, EMAIL_MAILBOX_TYPE_INBOX, &mailbox);
					if (ret == EMAIL_ERROR_NONE && mailbox) {
						debug_log("account_id:%d, account_count:%d, mailbox_id:%d", mailbox_ugd->account_id, account_count, mailbox->mailbox_id);
						mailbox_ugd->mailbox_id = mailbox->mailbox_id;
						mailbox_ugd->mode = EMAIL_MAILBOX_MODE_MAILBOX;
						mailbox_ugd->mailbox_type = EMAIL_MAILBOX_TYPE_INBOX;
						email_free_mailbox(&mailbox, 1);
					} else {
						debug_log("email_get_mailbox_by_mailbox_type failed : %d", ret);
						mailbox_ugd->mailbox_id = 0;
						mailbox_ugd->mode = EMAIL_MAILBOX_MODE_MAILBOX;
						mailbox_ugd->mailbox_type = EMAIL_MAILBOX_TYPE_INBOX;
					}
				} else {
					mailbox_ugd->mode = EMAIL_MAILBOX_MODE_ALL;
					mailbox_ugd->mailbox_type = EMAIL_MAILBOX_TYPE_INBOX;
				}

				if (account_list)
					email_engine_free_account_list(&account_list, account_count);
			}

			mailbox_view_title_update_all(mailbox_ugd);
			mailbox_update_notifications_status(mailbox_ugd);
			mailbox_list_refresh(mailbox_ugd, NULL);
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

	EmailMailboxModule *md = MEM_ALLOC(md, 1);
	if (!md) {
		debug_error("md is NULL");
		return NULL;
	}

	md->base.create = _mailbox_module_create;
	md->base.destroy = _mailbox_module_destroy;
	md->base.on_message = _mailbox_on_message;

	debug_leave();
	return &md->base;
}

void mailbox_create_no_contents_view(EmailMailboxUGD *mailbox_ugd, bool search_mode)
{
	debug_log("search_mode : %d", search_mode);

	if (!mailbox_ugd) {
		debug_warning("mailbox_ugd is NULL");
		return;
	}

	Evas_Object *no_content_scroller = elm_object_part_content_get(mailbox_ugd->content_layout, "noc");
	if (no_content_scroller) {
		debug_log("remove no content view");
		elm_object_part_content_unset(mailbox_ugd->content_layout, "noc");
		evas_object_del(no_content_scroller);
		no_content_scroller = NULL;
	}

	Evas_Object *no_content = NULL;
	no_content = elm_layout_add(mailbox_ugd->content_layout);

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
	no_content_scroller = elm_scroller_add(mailbox_ugd->content_layout);
	elm_scroller_policy_set(no_content_scroller, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
	elm_object_focus_allow_set(no_content_scroller, EINA_FALSE);
	evas_object_show(no_content_scroller);
	elm_object_content_set(no_content_scroller, no_content);

	elm_layout_signal_emit(no_content, "text,disabled", "");
	elm_layout_signal_emit(no_content, "align.center", "elm");

	elm_object_part_content_set(mailbox_ugd->content_layout, "noc", no_content_scroller);

	if (mailbox_ugd->no_content_shown && !search_mode) {
		mailbox_hide_no_contents_view(mailbox_ugd);
	}
}

void mailbox_show_no_contents_view(EmailMailboxUGD *mailbox_ugd)
{
	debug_enter();

	if (evas_object_visible_get(mailbox_ugd->gl))
		evas_object_hide(mailbox_ugd->gl);

	edje_object_signal_emit(_EDJ(mailbox_ugd->sub_layout_search), "hide_list", "elm");
	edje_object_signal_emit(_EDJ(mailbox_ugd->sub_layout_search), "show_noc", "elm");

	mailbox_send_all_btn_remove(mailbox_ugd);

	Evas_Object *no_content_scroller = elm_object_part_content_get(mailbox_ugd->content_layout, "noc");
	Evas_Object *no_content = elm_object_content_get(no_content_scroller);

	if (mailbox_ugd->b_searchmode == true) {
		elm_layout_theme_set(no_content, "layout", "nocontents", "search");
		elm_object_domain_translatable_part_text_set(no_content, "elm.text", PACKAGE, "IDS_EMAIL_NPBODY_NO_RESULTS_FOUND");
		elm_object_domain_translatable_part_text_set(no_content, "elm.help.text", PACKAGE, "");
	} else {
		elm_layout_theme_set(no_content, "layout", "nocontents", "default");
		elm_object_domain_translatable_part_text_set(no_content, "elm.text", PACKAGE, "IDS_EMAIL_NPBODY_NO_EMAILS");

		if (mailbox_ugd->mode == EMAIL_MAILBOX_MODE_PRIORITY_SENDER) {
			elm_object_domain_translatable_part_text_set(no_content, "elm.help.text", PACKAGE, "IDS_EMAIL_BODY_AFTER_YOU_ADD_PRIORITY_SENDERS_EMAILS_FROM_THEM_WILL_BE_SHOWN_HERE");
		} else if (mailbox_ugd->mode == EMAIL_MAILBOX_MODE_ALL && mailbox_ugd->mailbox_type == EMAIL_MAILBOX_TYPE_INBOX) {
			elm_object_domain_translatable_part_text_set(no_content, "elm.help.text", PACKAGE, "IDS_EMAIL_BODY_AFTER_YOU_RECEIVE_EMAILS_THEY_WILL_BE_SHOWN_HERE");
		}  else if (mailbox_ugd->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX) {
			elm_object_domain_translatable_part_text_set(no_content, "elm.help.text", PACKAGE, "IDS_EMAIL_BODY_AFTER_YOU_HAVE_TRIED_TO_SEND_EMAILS_BUT_THEY_FAILED_TO_BE_SENT_THEY_WILL_BE_SHOWN_HERE");
		} else if (mailbox_ugd->mailbox_type == EMAIL_MAILBOX_TYPE_SENTBOX) {
			elm_object_domain_translatable_part_text_set(no_content, "elm.help.text", PACKAGE, "IDS_EMAIL_BODY_AFTER_YOU_SEND_EMAILS_THEY_WILL_BE_SHOWN_HERE");
		} else if (mailbox_ugd->mailbox_type == EMAIL_MAILBOX_TYPE_TRASH) {
			elm_object_domain_translatable_part_text_set(no_content, "elm.help.text", PACKAGE, "IDS_EMAIL_BODY_AFTER_YOU_DELETE_EMAILS_THEY_WILL_BE_SHOWN_HERE");
		} else if (mailbox_ugd->mailbox_type == EMAIL_MAILBOX_TYPE_FLAGGED) {
			elm_object_domain_translatable_part_text_set(no_content, "elm.help.text", PACKAGE, "IDS_EMAIL_BODY_AFTER_YOU_ADD_FAVOURITE_EMAILS_BY_TAPPING_THE_STAR_ICON_THEY_WILL_BE_SHOWN_HERE");
		} else if (mailbox_ugd->mailbox_type == EMAIL_MAILBOX_TYPE_INBOX) {
			elm_object_domain_translatable_part_text_set(no_content, "elm.help.text", PACKAGE, "IDS_EMAIL_BODY_AFTER_YOU_RECEIVE_EMAILS_THEY_WILL_BE_SHOWN_HERE");
		} else if (mailbox_ugd->mailbox_type == EMAIL_MAILBOX_TYPE_DRAFT) {
			elm_object_domain_translatable_part_text_set(no_content, "elm.help.text", PACKAGE, "IDS_EMAIL_BODY_AFTER_YOU_SAVE_EMAILS_IN_DRAFTS_THEY_WILL_BE_SHOWN_HERE");
		} else if (mailbox_ugd->mailbox_type == EMAIL_MAILBOX_TYPE_SPAMBOX) {
			elm_object_domain_translatable_part_text_set(no_content, "elm.help.text", PACKAGE, "IDS_EMAIL_BODY_AFTER_YOU_ADD_EMAILS_TO_SPAM_THEY_WILL_BE_SHOWN_HERE");
		} else {
			elm_object_domain_translatable_part_text_set(no_content, "elm.help.text", PACKAGE, "IDS_EMAIL_BODY_AFTER_YOU_MOVE_EMAILS_TO_THIS_FOLDER_THEY_WILL_BE_SHOWN_HERE");
		}
	}
	elm_layout_signal_emit(no_content, "text,disabled", "");
	elm_layout_signal_emit(no_content, "align.center", "elm");

	mailbox_ugd->no_content_shown = true;
}

void mailbox_hide_no_contents_view(EmailMailboxUGD *mailbox_ugd)
{
	debug_enter();

	if (!evas_object_visible_get(mailbox_ugd->gl))
		evas_object_show(mailbox_ugd->gl);

	edje_object_signal_emit(_EDJ(mailbox_ugd->sub_layout_search), "hide_noc", "elm");
	edje_object_signal_emit(_EDJ(mailbox_ugd->sub_layout_search), "show_list", "elm");
	mailbox_ugd->no_content_shown = false;
}

void mailbox_refresh_fullview(EmailMailboxUGD *mailbox_ugd, bool update_title)
{
	debug_enter();
	retm_if(!mailbox_ugd, "mailbox_ugd == NULL");

	DELETE_EVAS_OBJECT(mailbox_ugd->more_ctxpopup);

	if (mailbox_ugd->b_editmode)
		mailbox_exit_edit_mode(mailbox_ugd);

	if (update_title)
		mailbox_view_title_update_all(mailbox_ugd);

	if (mailbox_ugd->b_searchmode)
		mailbox_show_search_result(mailbox_ugd);
	else
		mailbox_list_refresh(mailbox_ugd, NULL);

}

void mailbox_naviframe_mailbox_button_remove(EmailMailboxUGD *mailbox_ugd)
{

	elm_object_part_content_unset(mailbox_ugd->title_layout, "mailbox_button");
	evas_object_hide(mailbox_ugd->btn_mailbox);

	return;
}

void mailbox_naviframe_mailbox_button_add(EmailMailboxUGD *mailbox_ugd)
{
	if (!mailbox_ugd->btn_mailbox) {
		mailbox_ugd->btn_mailbox = elm_button_add(mailbox_ugd->title_layout);
		elm_object_style_set(mailbox_ugd->btn_mailbox, "naviframe/title_right");
		evas_object_smart_callback_add(mailbox_ugd->btn_mailbox, "clicked", _mailbox_title_clicked_cb, mailbox_ugd);
	}
	elm_object_domain_translatable_text_set(mailbox_ugd->btn_mailbox, PACKAGE, "IDS_EMAIL_ACBUTTON_MAILBOX_ABB");
	elm_object_part_content_set(mailbox_ugd->title_layout, "mailbox_button", mailbox_ugd->btn_mailbox);
	evas_object_show(mailbox_ugd->btn_mailbox);

	return;
}

void mailbox_update_notifications_status(EmailMailboxUGD *mailbox_ugd)
{
	debug_enter();
	retm_if(!mailbox_ugd, "mailbox_ugd == NULL");

	if ((mailbox_ugd->base.state == EV_STATE_ACTIVE) &&
		(mailbox_ugd->mailbox_type == EMAIL_MAILBOX_TYPE_INBOX) &&
		(mailbox_ugd->mode != EMAIL_MAILBOX_MODE_PRIORITY_SENDER)) {
		email_set_is_inbox_active(true);
		if (mailbox_ugd->mode == EMAIL_MAILBOX_MODE_MAILBOX) {
			email_clear_notification_bar(mailbox_ugd->account_id);
		} else {
			email_clear_notification_bar(ALL_ACCOUNT);
		}
	} else {
		email_set_is_inbox_active(false);
	}

	debug_leave();
}

int mailbox_open_email_viewer(EmailMailboxUGD *mailbox_ugd, int account_id, int mailbox_id, int mail_id)
{
	debug_enter();
	retvm_if(!mailbox_ugd, -1, "mailbox_ugd is NULL");
	debug_log("account_id: %d, mailbox_id: %d, mail_id: %d", account_id, mailbox_id, mail_id);

	app_control_h service = NULL;

	if (email_params_create(&service) &&
		email_params_add_int(service, EMAIL_BUNDLE_KEY_ACCOUNT_ID, account_id) &&
		email_params_add_int(service, EMAIL_BUNDLE_KEY_MAILBOX, mailbox_id) &&
		email_params_add_int(service, EMAIL_BUNDLE_KEY_MAIL_ID, mail_id)) {

		mailbox_ugd->opened_mail_id = mail_id;
		mailbox_ugd->viewer = mailbox_viewer_module_create(mailbox_ugd, EMAIL_MODULE_VIEWER, service);
	}

	email_params_free(&service);

	debug_leave();
	return 0;
}
