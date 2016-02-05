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

#include <app.h>
#include <system_settings.h>
#include <Elementary.h>
#include <utils_i18n.h>
#include <account-types.h>

#include "email-module.h"
#include "email-utils.h"
#include "email-locale.h"
#include "email-engine.h"
#include "email-debug.h"

#undef LOG_TAG
#define LOG_TAG "EMAIL_APP"

#define APP_BASE_SCALE 2.6
#define APP_WIN_NAME PKGNAME

#define APP_HANDLERS_COUNT 4

#ifndef SHARED_MODULES_FEATURE
#define APP_USE_SIG
#endif
#ifdef APP_USE_SIG
int g_email_sig_to_handle[] = {SIGILL, SIGABRT, SIGFPE, SIGSEGV};
#define APP_SIG_COUNT ((int)(sizeof(g_email_sig_to_handle) / sizeof(g_email_sig_to_handle[0])))
#endif

typedef struct _app_data {
	app_event_handler_h handlers[APP_HANDLERS_COUNT];

	Evas_Object *win;
	Evas_Object *conform;

	app_control_h launch_params;
	email_module_h module;

	bool module_mgr_init_ok;

} app_data_t;

static bool _app_create_cb(void *user_data);
static void _app_terminate_cb(void *user_data);
static void _app_control_cb(app_control_h app_control, void *user_data);
static void _app_resume_cb(void *user_data);
static void _app_pause_cb(void *user_data);

static void _app_low_battery_cb(app_event_info_h event_info, void *user_data);
static void _app_low_memory_cb(app_event_info_h event_info, void *user_data);
static void _app_lang_changed_cb(app_event_info_h event_info, void *user_data);
static void _app_region_fmt_changed_cb(app_event_info_h event_info, void *user_data);

static void _app_timezone_change_cb(system_settings_key_e key, void *data);
static void _app_win_rotation_changed_cb(void *data, Evas_Object *obj, void *event_info);
static void _app_module_result_cb(void *data, email_module_h module, app_control_h result);
static void _app_module_destroy_request_cb(void *data, email_module_h module);

#ifdef APP_USE_SIG
static void _app_signal_handler(int signum, siginfo_t *info, void *context);
#endif

static bool _app_init(app_data_t *ad);
static bool _app_init_main_layouts(app_data_t *ad);
static bool _app_init_module_mgr(app_data_t *ad);
static bool _app_register_callbacks(app_data_t *ad);

static void _app_finalize(app_data_t *ad);

static email_module_type_e _app_get_module_type(app_control_h params);
static bool _app_create_module(app_data_t *ad, app_control_h params);
static bool _app_recreate_module(app_data_t *ad);

bool _app_create_cb(void *user_data)
{
	debug_enter();
	app_data_t *ad = user_data;

	if (!_app_init(ad)) {
		debug_error("_app_init(): failed");
		_app_finalize(ad);
		return false;
	}

	debug_leave();
	return true;
}

void _app_terminate_cb(void *user_data)
{
	debug_enter();
	app_data_t *ad = user_data;

	_app_finalize(ad);

	debug_leave();
}

void _app_control_cb(app_control_h app_control, void *user_data)
{
	debug_enter();
	app_data_t *ad = user_data;

	bool first_launch = (ad->launch_params == NULL);

	if (first_launch) {
		int r = app_control_clone(&ad->launch_params, app_control);
		if (r != APP_CONTROL_ERROR_NONE) {
			debug_error("app_control_clone(): failed (%d). Shutting down...", r);
			ui_app_exit();
			return;
		}

		if (!_app_create_module(ad, app_control)) {
			debug_error("_app_create_module(): failed (%d). Shutting down...", r);
			ui_app_exit();
			return;
		}
	} else if (!email_module_mgr_is_in_transiton()) {
		debug_log("Sending message to the module...");
		email_module_send_message(ad->module, app_control);
	} else {
		debug_warning("Module manager in transition state. Ignoring message...");
	}

	debug_leave();
}

void _app_resume_cb(void *user_data)
{
	debug_enter();

	email_module_mgr_resume();

	debug_leave();
}

void _app_pause_cb(void *user_data)
{
	debug_enter();

	email_module_mgr_pause();

	debug_leave();
}

void _app_low_battery_cb(app_event_info_h event_info, void *user_data)
{
	debug_enter();

	email_module_mgr_send_event(EM_EVENT_LOW_BATTERY);

	debug_leave();
}

void _app_low_memory_cb(app_event_info_h event_info, void *user_data)
{
	debug_enter();

	app_event_low_memory_status_e low_memory_status;
	int error = app_event_get_low_memory_status(event_info, &low_memory_status);
	debug_log("low mem app event type:%d", low_memory_status);
	if (error != APP_ERROR_NONE) {
		debug_error("app_event_get_low_memory_status failed, error:(%d)", error);
		debug_error("apply event low_memory_hard");
		email_module_mgr_send_event(EM_EVENT_LOW_MEMORY_HARD);
		return;
	}

	if (low_memory_status == APP_EVENT_LOW_MEMORY_SOFT_WARNING) {
		email_module_mgr_send_event(EM_EVENT_LOW_MEMORY_SOFT);
	} else {
		email_module_mgr_send_event(EM_EVENT_LOW_MEMORY_HARD);
	}

	debug_leave();
}

void _app_lang_changed_cb(app_event_info_h event_info, void *user_data)
{
	debug_enter();

	char *language;
	int r = system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_LANGUAGE, &language);
	if (r == SYSTEM_SETTINGS_ERROR_NONE) {
		debug_log("language: %s", language);
		elm_language_set(language);
		free(language);
	}

	email_module_mgr_send_event(EM_EVENT_LANG_CHANGE);

	debug_leave();
}

void _app_region_fmt_changed_cb(app_event_info_h event_info, void *user_data)
{
	debug_enter();

	email_mutex_lock();
	email_close_pattern_generator();
	email_open_pattern_generator();
	email_mutex_unlock();

	email_module_mgr_send_event(EM_EVENT_REGION_CHANGE);

	debug_leave();
}

void _app_timezone_change_cb(system_settings_key_e key, void *data)
{
	debug_enter();

	email_mutex_lock();
	email_close_pattern_generator();
	email_open_pattern_generator();
	email_mutex_unlock();

	debug_leave();
}

void _app_win_rotation_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	app_data_t *ad = data;

	email_module_event_e event = EM_EVENT_ROTATE_PORTRAIT;
	int rot = elm_win_rotation_get(ad->win);

	switch (rot) {
	case 0:
		event = EM_EVENT_ROTATE_PORTRAIT;
		break;
	case 180:
		event = EM_EVENT_ROTATE_PORTRAIT_UPSIDEDOWN;
		break;
	case 270:
		event = EM_EVENT_ROTATE_LANDSCAPE;
		break;
	case 90:
		event = EM_EVENT_ROTATE_LANDSCAPE_UPSIDEDOWN;
		break;
	default:
		debug_warning("Unexpected rotation: %d", rot);
		break;
	}

	email_module_mgr_send_event(event);

	debug_leave();
}

void _app_module_result_cb(void *data, email_module_h module, app_control_h result)
{
	debug_enter();
	app_data_t *ad = data;

	int r = app_control_reply_to_launch_request(result, ad->launch_params, APP_CONTROL_RESULT_SUCCEEDED);
	if (r != APP_CONTROL_ERROR_NONE) {
		debug_error("app_control_reply_to_launch_request(): failed (%d).", r);
		return;
	}

	debug_leave();
}

void _app_module_destroy_request_cb(void *data, email_module_h module)
{
	debug_enter();
	app_data_t *ad = data;

	if (email_get_need_restart_flag()) {
		email_set_need_restart_flag(false);
		debug_log("Need to restart the application.");
		if (_app_recreate_module(ad)) {
			debug_log("Module was recreated.");
			return;
		}
		debug_error("Failed to recreate the modle!");
	}

	debug_log("Shutting down...");
	ui_app_exit();

	debug_leave();
}

#ifdef APP_USE_SIG
void _app_signal_handler(int signum, siginfo_t *info, void *context)
{
	debug_enter();

	debug_error("Received [%d] signal.", signum);
	email_set_is_inbox_active(false);

	debug_leave();
	raise(signum);
}
#endif

bool _app_init(app_data_t *ad)
{
	email_mutex_init();

	email_feedback_init();

	if (!_app_init_main_layouts(ad)) {
		return false;
	}

	if (email_open_pattern_generator() != 0) {
		return false;
	}

	if (!_app_init_module_mgr(ad)) {
		return false;
	}

	if (!_app_register_callbacks(ad)) {
		return false;
	}

	return true;
}

bool _app_init_main_layouts(app_data_t *ad)
{
	elm_app_base_scale_set(APP_BASE_SCALE);

	ad->win = elm_win_util_standard_add(APP_WIN_NAME, APP_WIN_NAME);
	if (!ad->win) {
		debug_error("elm_win_add(): failed");
		return false;
	}

	elm_win_indicator_mode_set(ad->win, ELM_WIN_INDICATOR_SHOW);
	elm_win_indicator_opacity_set(ad->win, ELM_WIN_INDICATOR_OPAQUE);
	elm_win_conformant_set(ad->win, EINA_TRUE);

	ad->conform = elm_conformant_add(ad->win);
	if (!ad->conform) {
		debug_error("elm_conformant_add(): failed");
		return false;
	}

	evas_object_size_hint_weight_set(ad->conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(ad->win, ad->conform);
	evas_object_show(ad->conform);

	evas_object_show(ad->win);

	return true;
}

bool _app_init_module_mgr(app_data_t *ad)
{
	int r = email_module_mgr_init(ad->win, ad->conform);
	if (r != 0) {
		debug_error("email_module_mgr_init(): failed");
		return false;
	}

	ad->module_mgr_init_ok = true;

	return true;
}

bool _app_register_callbacks(app_data_t *ad)
{
	app_event_type_e events[APP_HANDLERS_COUNT] = {
			APP_EVENT_LOW_BATTERY,
			APP_EVENT_LOW_MEMORY,
			APP_EVENT_LANGUAGE_CHANGED,
			APP_EVENT_REGION_FORMAT_CHANGED
	};

	app_event_cb cbs[APP_HANDLERS_COUNT] = {
			_app_low_battery_cb,
			_app_low_memory_cb,
			_app_lang_changed_cb,
			_app_region_fmt_changed_cb
	};

	int i = 0;

#ifdef APP_USE_SIG
	struct sigaction act;
	act.sa_sigaction = _app_signal_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
	act.sa_flags |= SA_RESETHAND;
	for (i = 0; i < APP_SIG_COUNT; ++i) {
		if (sigaction(g_email_sig_to_handle[i], &act, NULL) < 0) {
			debug_error("sigaction() failed!");
		}
	}
#endif

	for (i = 0; i < APP_HANDLERS_COUNT; ++i) {
		int r = ui_app_add_event_handler(&ad->handlers[i], events[i], cbs[i], &ad);
		if (r != APP_ERROR_NONE) {
			debug_warning("ui_app_add_event_handler(%d): failed (%d)", events[i], r);
			ad->handlers[i] = NULL;
		}
	}

	if (elm_win_wm_rotation_supported_get(ad->win)) {
		int rots[4] = { 0, 90, 180, 270 };
		elm_win_wm_rotation_available_rotations_set(ad->win, rots, 4);
		evas_object_smart_callback_add(ad->win, "wm,rotation,changed", _app_win_rotation_changed_cb, ad);
	}

	if (email_register_timezone_changed_callback(_app_timezone_change_cb, NULL) != 0) {
		return false;
	}

	return true;
}

void _app_finalize(app_data_t *ad)
{
	int i;
	for (i = 0; i < APP_HANDLERS_COUNT; ++i) {
		if (ad->handlers[i]) {
			ui_app_remove_event_handler(ad->handlers[i]);
		}
	}

	email_unregister_timezone_changed_callback(_app_timezone_change_cb);

	if (ad->module_mgr_init_ok) {
		email_module_mgr_finalize();
	}

	email_close_pattern_generator();

	evas_object_del(ad->win);

	email_feedback_deinit();

	email_mutex_destroy();

	email_engine_finalize_force();
}

static email_module_type_e _app_get_module_type(app_control_h params)
{
#ifdef SHARED_MODULES_FEATURE
	return APP_MODULE_TYPE;
#endif

	email_module_type_e module_type = EMAIL_MODULE_MAILBOX;

	app_control_launch_mode_e launch_mode = APP_CONTROL_LAUNCH_MODE_SINGLE;
	int r = app_control_get_launch_mode(params, &launch_mode);
	if (r != APP_CONTROL_ERROR_NONE) {
		debug_log("app_control_get_launch_mode(): failed (%d)", r);
		return module_type;
	}

	if (launch_mode != APP_CONTROL_LAUNCH_MODE_SINGLE) {
		char *operation = NULL;
		r = app_control_get_operation(params, &operation);
		if (r != APP_CONTROL_ERROR_NONE) {
			debug_error("app_control_get_operation(): failed (%d)", r);
			return module_type;
		}
		if (operation) {
			if ((strcmp(operation, APP_CONTROL_OPERATION_COMPOSE) == 0) ||
				(strcmp(operation, APP_CONTROL_OPERATION_SHARE) == 0) ||
				(strcmp(operation, APP_CONTROL_OPERATION_MULTI_SHARE) == 0) ||
				(strcmp(operation, APP_CONTROL_OPERATION_SHARE_TEXT) == 0)) {
				module_type = EMAIL_MODULE_COMPOSER;
			} if ((strcmp(operation, ACCOUNT_OPERATION_SIGNIN) == 0) ||
				  (strcmp(operation, ACCOUNT_OPERATION_VIEW) == 0)) {
				module_type = EMAIL_MODULE_SETTING;
			}
			FREE(operation);
		}
	}

	debug_log("module_type: %d", (int)module_type);

	return module_type;
}

static bool _app_create_module(app_data_t *ad, app_control_h params)
{
	debug_enter();
	retvm_if(ad->module, false, "Root module already created!");

	email_module_listener_t listener = { 0 };
	listener.cb_data = ad;
	listener.result_cb = _app_module_result_cb;
	listener.destroy_request_cb = _app_module_destroy_request_cb;

	email_module_h module = email_module_mgr_create_root_module(
			_app_get_module_type(params), params, &listener);
	if (!module) {
		debug_error("Module creation failed.");
		return false;
	}
	ad->module = module;

	debug_leave();
	return true;
}

static bool _app_recreate_module(app_data_t *ad)
{
	debug_enter();
	retvm_if(!ad->module, false, "Root module is not created!");

	bool result = true;
	app_control_h params = NULL;

	if (email_params_create(&params)) {

		email_module_destroy(ad->module);
		ad->module = NULL;

		if (!_app_create_module(ad, params)) {
			debug_error("_app_create_module(): failed.");
			result = false;
		}
	}

	email_params_free(&params);

	debug_leave();
	return result;
}

EXPORT_API int main(int argc, char **argv)
{
	debug_enter();

	app_data_t ad;
	memset(&ad, 0, sizeof(app_data_t));

	ui_app_lifecycle_callback_s callback = { 0 };

	callback.create = _app_create_cb;
	callback.terminate = _app_terminate_cb;
	callback.app_control = _app_control_cb;
	callback.resume = _app_resume_cb;
	callback.pause = _app_pause_cb;

	int r = ui_app_main(argc, argv, &callback, &ad);
	debug_log("ui_app_main(): result = %X", r);

	debug_leave();
	return r;
}
