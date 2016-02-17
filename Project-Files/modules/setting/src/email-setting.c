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
#include <getopt.h>
#include <account.h>

#include "email-setting.h"
#include "email-setting-utils.h"
#include "email-setting-noti-mgr.h"
#include "email-view-setting.h"
#include "email-view-account-setup.h"
#include "email-view-account-details.h"
#include "email-view-signature-setting.h"
#include "email-view-notification.h"

static int _setting_create(email_module_t *self, email_params_h params);
static void _setting_resume(email_module_t *self);
static void _setting_destroy(email_module_t *self);
static void _setting_on_event(email_module_t *self, email_module_event_e event);

static int _account_init(EmailSettingUGD *ugd);
static int _account_finalize(EmailSettingUGD *ugd);
static ViewType _parse_option(EmailSettingUGD *ugd, email_params_h params);
static void _account_deleted_cb(int account_id, email_setting_response_data *response, void *user_data);

static void _filter_destroy_request_cb(void *data, email_module_h module);

static void _popup_ok_cb(void *data, Evas_Object *obj, void *event_info);

static void _create_launching_fail_popup(EmailSettingUGD *ugd, const email_string_t *content);

static email_string_t EMAIL_SETTING_STRING_OK = {PACKAGE, "IDS_EMAIL_BUTTON_OK"};
static email_string_t EMAIL_SETTING_STRING_FAILED_TO_START_EMAIL_APPLICATION = {PACKAGE, "IDS_EMAIL_POP_FAILED_TO_START_EMAIL_APPLICATION"};
static email_string_t EMAIL_SETTING_STRING_WARNING = {PACKAGE, "IDS_ST_HEADER_WARNING"};

#ifdef SHARED_MODULES_FEATURE
EMAIL_API email_module_t *email_module_alloc()
#else
email_module_t *setting_module_alloc()
#endif
{
	debug_enter();

	EmailSettingUGD *ugd = MEM_ALLOC(ugd, 1);
	if (!ugd) {
		return NULL;
	}

	ugd->base.create = _setting_create;
	ugd->base.destroy = _setting_destroy;
	ugd->base.resume = _setting_resume;
	ugd->base.on_event = _setting_on_event;

	debug_leave();
	return &ugd->base;
}

UG_MODULE_API int setting_plugin_reset(app_control_h service, void *priv)
{
	return 0;
}

static int _setting_create(email_module_t *self, email_params_h params)
{
	debug_enter();
	EmailSettingUGD *ugd;

	if (!self)
		return -1;

	ugd = (EmailSettingUGD *)self;

	bindtextdomain(PACKAGE, email_get_locale_dir());

	ugd->filter_listener.cb_data = ugd;
	ugd->filter_listener.destroy_request_cb = _filter_destroy_request_cb;

	/* DBUS */
	setting_noti_init(ugd);

	/* subscribe for account delete notification */
	setting_noti_subscribe(ugd, EMAIL_SETTING_ACCOUNT_DELETE_NOTI, _account_deleted_cb, ugd);

	/* ICU */
	setting_open_icu_pattern_generator();

	/* Init Email Service */
	if (!email_engine_initialize()) {
		_create_launching_fail_popup(ugd, &(EMAIL_SETTING_STRING_FAILED_TO_START_EMAIL_APPLICATION));
		return -1;
	}

	/* Init Emf Account */
	if (!_account_init(ugd)) {
		debug_warning("Failed to get account list");
	}

	/* register keypad callback */
	setting_register_keypad_rot_cb(ugd);

	/* creating view */

	switch (_parse_option(ugd, params)) {
	case VIEW_SETTING:
		create_setting_view(ugd);
		break;
	case VIEW_ACCOUNT_SETUP:
		create_account_setup_view(ugd);
		break;
	case VIEW_ACCOUNT_DETAILS:
		create_account_details_view(ugd);
		break;
	case VIEW_NOTIFICATION_SETTING:
		create_notification_setting_view(ugd);
		break;
	case VIEW_SIGNATURE_SETTING:
		create_signature_setting_view(ugd);
		break;
	default:
		/* invalid view type */
		_create_launching_fail_popup(ugd, &(EMAIL_SETTING_STRING_FAILED_TO_START_EMAIL_APPLICATION));
		break;
	}

	return 0;
}

static void _setting_on_event(email_module_t *self, email_module_event_e event)
{
	debug_enter();

	if (!self)
		return;

	EmailSettingUGD *ugd = (EmailSettingUGD *)self;

	switch (event) {
	case EM_EVENT_REGION_CHANGE:
	case EM_EVENT_LANG_CHANGE:
		setting_close_icu_pattern_generator();
		setting_open_icu_pattern_generator();
		break;
	default:
		break;
	}

	debug_log("is_keypad: %d", ugd->is_keypad);
	debug_log("event: %d", event);

	return;
}

static void _setting_destroy(email_module_t *self)
{
	debug_enter();
	EmailSettingUGD *ugd;

	if (!self)
		return;

	ugd = (EmailSettingUGD *)self;

	setting_noti_unsubscribe(ugd, EMAIL_SETTING_ACCOUNT_DELETE_NOTI, _account_deleted_cb);
	setting_noti_deinit(ugd);
	setting_deregister_keypad_rot_cb(ugd);

	DELETE_EVAS_OBJECT(ugd->popup);

	/* destroying cancel op */
	GSList *cancel_op_list = ugd->cancel_op_list;
	if (g_slist_length(ugd->cancel_op_list) > 0) {
		while (cancel_op_list) {
			free(cancel_op_list->data);
			cancel_op_list = g_slist_next(cancel_op_list);
		}
		g_slist_free(ugd->cancel_op_list);
		ugd->cancel_op_list = NULL;
	}

	if (ugd->app_control_google_eas) {
		app_control_send_terminate_request(ugd->app_control_google_eas);
		app_control_destroy(ugd->app_control_google_eas);
		ugd->app_control_google_eas = NULL;
	}

	/* ICU */
	setting_close_icu_pattern_generator();

	setting_free_provider_list(&(ugd->default_provider_list));
	email_engine_finalize();

	_account_finalize(ugd);
}

static void _setting_resume(email_module_t *self)
{
	debug_enter();

	EmailSettingUGD *ugd;

	if (!self)
		return;

	ugd = (EmailSettingUGD *)self;

	if (ugd->app_control_google_eas) {
		app_control_destroy(ugd->app_control_google_eas);
		ugd->app_control_google_eas = NULL;
	}

	return;
}

static int _account_init(EmailSettingUGD *ugd)
{
	debug_enter();
	int i = 0;

	/* Get Email Account List Info */
	ugd->account_list = NULL;
	if (!email_engine_get_account_list(&ugd->account_count, &ugd->account_list)) {
		return FALSE;
	}

	for (i = 0; i < ugd->account_count; i++) {
		EMAIL_SETTING_PRINT_ACCOUNT_INFO((&(ugd->account_list[i])));
	}

	return TRUE;
}

static int _account_finalize(EmailSettingUGD *ugd)
{
	debug_enter();
	int r;

	/* Free Account List Info */
	if (ugd->account_list != NULL) {
		r = email_engine_free_account_list(&(ugd->account_list), ugd->account_count);
		retv_if(r == FALSE, -1);
	}
	ugd->account_list = NULL;

	return TRUE;
}

static ViewType _parse_option(EmailSettingUGD *ugd, email_params_h params)
{
	debug_enter();
	const char *operation = NULL;
	ViewType ret_type = VIEW_INVALID;

	retvm_if(!params, VIEW_INVALID, "invalid parameter!");

	email_params_get_operation_opt(params, &operation);
	debug_log("operation = %s", operation);

	if (operation) {
		if (!g_strcmp0(operation, ACCOUNT_OPERATION_SIGNIN) ||
				!g_strcmp0(operation, APP_CONTROL_OPERATION_DEFAULT)) {
			debug_log("Operation ACCOUNT_OPERATION_SIGNIN");

			debug_log("VIEW_TYPE - VIEW_ACCOUNT_SETUP");
			ret_type = VIEW_ACCOUNT_SETUP;

			goto FINISH;
		} else if (!g_strcmp0(operation, ACCOUNT_OPERATION_VIEW)) {
			debug_log("Operation ACCOUNT_OPERATION_VIEW");

			int my_account_id = 0;
			int i = 0;

			gotom_if(!email_params_get_int(params, ACCOUNT_DATA_ID, &my_account_id),
					FINISH, "%s is invalid", ACCOUNT_DATA_ID);

			debug_log("my-account id - %d", my_account_id);

			ugd->account_id = -1;
			for (i = 0; i < ugd->account_count; i++) {
				if (ugd->account_list[i].account_svc_id == my_account_id) {
					ugd->account_id = ugd->account_list[i].account_id;
					debug_log("email-service id - %d", ugd->account_id);
				}
			}

			if (ugd->account_id != -1) {
				debug_log("VIEW_TYPE - VIEW_SYNC_OPTION");
				ret_type = VIEW_ACCOUNT_DETAILS;
			} else {
				debug_log("VIEW_TYPE - VIEW_INVALID");
				ret_type = VIEW_INVALID;
			}

			goto FINISH;
		} else if (!g_strcmp0(operation, EMAIL_URI_NOTIFICATION_SETTING)) {
			debug_log("Operation EMAIL_URI_NOTIFICATION_SETTING");
			int account_id = -1;

			gotom_if(!email_params_get_int(params, EMAIL_BUNDLE_KEY_ACCOUNT_ID, &account_id),
					FINISH, "%s is invalid", EMAIL_BUNDLE_KEY_ACCOUNT_ID);

			debug_log("EMAIL_URI_NOTIFICATION_SETTING account id: %d", account_id);

			ugd->account_id = account_id;
			if (ugd->account_id != -1) {
				debug_log("VIEW_TYPE - NOTIFICATION_SETTING");
				ret_type = VIEW_NOTIFICATION_SETTING;
			} else {
				debug_log("VIEW_TYPE - VIEW_INVALID");
				ret_type = VIEW_INVALID;
			}
		} else if (!g_strcmp0(operation, EMAIL_URI_SIGNATURE_SETTING)) {
			debug_log("Operation EMAIL_URI_SIGNATURE_SETTING");
			int account_id = -1;

			gotom_if(!email_params_get_int(params, EMAIL_BUNDLE_KEY_ACCOUNT_ID, &account_id),
					FINISH, "%s is invalid", EMAIL_BUNDLE_KEY_ACCOUNT_ID);

			debug_log("EMAIL_URI_NOTIFICATION_SETTING account id: %d", account_id);

			ugd->account_id = account_id;
			if (ugd->account_id != -1) {
				debug_log("VIEW_TYPE - SIGNATURE_SETTING");
				ret_type = VIEW_SIGNATURE_SETTING;
			} else {
				debug_log("VIEW_TYPE - VIEW_INVALID");
				ret_type = VIEW_INVALID;
			}
		}
	} else {
		/* from ug_create */
		const char *start_view_type = NULL;
		retvm_if(!email_params_get_str(params, EMAIL_BUNDLE_KEY_VIEW_TYPE, &start_view_type),
				VIEW_INVALID, "%s is invalid", EMAIL_BUNDLE_KEY_VIEW_TYPE);

		debug_log("VIEW_TYPE - %s", start_view_type);
		if (!g_strcmp0(start_view_type, EMAIL_BUNDLE_VAL_VIEW_SETTING)) {
			ret_type = VIEW_SETTING;
		} else if (!g_strcmp0(start_view_type, EMAIL_BUNDLE_VAL_VIEW_ACCOUNT_SETUP)) {
			ret_type = VIEW_ACCOUNT_SETUP;
		} else {
			ret_type = VIEW_INVALID;
		}
	}

FINISH:
	debug_log("start view is %d", ret_type);
	return ret_type;
}

static void _filter_destroy_request_cb(void *data, email_module_h module)
{
	debug_enter();
	EmailSettingUGD *ugd;

	if (!data)
		return;

	ugd = data;

	if (module == ugd->filter) {
		email_module_destroy(ugd->filter);
		ugd->filter = NULL;
	}

	return;
}

static void _popup_ok_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	if (!data) {
		debug_error("data is NULL");
		return;
	}

	if (obj)
		evas_object_del(obj);

	EmailSettingUGD *ugd = data;
	email_module_make_destroy_request(&ugd->base);
}

static void _account_deleted_cb(int account_id, email_setting_response_data *response, void *user_data)
{
	debug_enter();
	EmailSettingUGD *ugd = user_data;

	retm_if(!response, "response data is NULL");
	retm_if(ugd->account_id != response->account_id, "account_id is different");
	retm_if(ugd->is_account_deleted_on_this, "account is deleted on this ug");

	debug_log("account is deleted. Let's destroyed");
	email_module_make_destroy_request(&ugd->base);
}

static void _create_launching_fail_popup(EmailSettingUGD *ugd, const email_string_t *content)
{
	debug_enter();
	Evas_Object *popup = elm_popup_add(ugd->base.win);
	elm_popup_align_set(popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
	elm_object_domain_translatable_part_text_set(popup, "title,text", EMAIL_SETTING_STRING_WARNING.domain, EMAIL_SETTING_STRING_WARNING.id);
	elm_object_domain_translatable_text_set(popup, content->domain, content->id);
	eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, _popup_ok_cb, ugd);
	Evas_Object *btn1 = elm_button_add(popup);
	elm_object_style_set(btn1, "popup");
	elm_object_domain_translatable_text_set(btn1, EMAIL_SETTING_STRING_OK.domain, EMAIL_SETTING_STRING_OK.id);
	elm_object_part_content_set(popup, "button1", btn1);
	evas_object_smart_callback_add(btn1, "clicked", _popup_ok_cb, ugd);
	evas_object_show(popup);
}

/* EOF */
