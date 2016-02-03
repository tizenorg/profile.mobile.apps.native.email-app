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

/* Header Include */
#include <utils_i18n.h>
#include <app_control.h>
#include <malloc.h>
#include <notification.h>

#include "email-common-types.h"
#include "email-debug.h"
#include "email-engine.h"
#include "email-utils.h"
#include "email-color-box.h"

#include "email-module.h"

#include "email-viewer.h"
#include "email-viewer-contents.h"
#include "email-viewer-util.h"
#include "email-viewer-attachment.h"
#include "email-viewer-more-menu.h"
#include "email-viewer-header.h"
#include "email-viewer-recipient.h"
#include "email-viewer-initial-data.h"
#include "email-viewer-eml.h"
#include "email-viewer-scroller.h"
#include "email-viewer-js.h"
#include "email-viewer-ext-gesture.h"
#include "email-viewer-more-menu-callback.h"
#include "email-viewer-noti-mgr.h"


/* Global Val */
EmailViewerUGD *_g_md = NULL; /* TODO Need more investigation why we need this global variable. */

static email_string_t EMAIL_VIEWER_POPUP_DOWNLOADING = { PACKAGE, "IDS_EMAIL_BUTTON_DOWNLOAD_FULL_EMAIL_ABB" };
static email_string_t EMAIL_VIEWER_STRING_OK = { PACKAGE, "IDS_EMAIL_BUTTON_OK" };
static email_string_t EMAIL_VIEWER_STRING_CANCEL = { PACKAGE, "IDS_EMAIL_BUTTON_CANCEL" };
static email_string_t EMAIL_VIEWER_HEADER_UNABLE_TO_PERFORM_ACTION = { PACKAGE, "IDS_EMAIL_HEADER_UNABLE_TO_PERFORM_ACTION_ABB" };
static email_string_t EMAIL_VIEWER_UNKNOWN_ERROR_HAS_OCCURED = { PACKAGE, "IDS_EMAIL_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED" };
static email_string_t EMAIL_VIEWER_HEADER_UNAVAILABLE = { PACKAGE, "IDS_ASEML_HEADER_UNABLE_TO_USE_FUNCTION_ABB" };
static email_string_t EMAIL_VIEWER_POP_SECURITY_POLICY_RESTRICTS_USE_OF_POP_IMAP_EMAIL = { PACKAGE, "IDS_ASEML_POP_THE_SECURITY_POLICY_PREVENTS_THE_USE_OF_POP_AND_IMAP_EMAIL_PROTOCOLS" };
static email_string_t EMAIL_VIEWER_POP_FAILED_TO_START_EMAIL_APPLICATION = { PACKAGE, "IDS_EMAIL_POP_FAILED_TO_START_EMAIL_APPLICATION" };
static email_string_t EMAIL_VIEWER_BODY_SELECTED_DATA_NOT_FOUND = { PACKAGE, "IDS_EMAIL_POP_THE_SELECTED_DATA_COULD_NOT_BE_FOUND" };
static email_string_t EMAIL_VIEWER_POP_THERE_IS_NO_ACCOUNT_CREATE_A_NEW_ACCOUNT_FIRST = { PACKAGE, "IDS_EMAIL_POP_YOU_HAVE_NOT_YET_CREATED_AN_EMAIL_ACCOUNT_CREATE_AN_EMAIL_ACCOUNT_AND_TRY_AGAIN" };
static email_string_t EMAIL_VIEWER_HEADER_DOWNLOAD_ENTIRE_EMAIL = { PACKAGE, "IDS_EMAIL_HEADER_DOWNLOAD_ENTIRE_EMAIL_ABB" };
static email_string_t EMAIL_VIEWER_POP_PARTIAL_BODY_DOWNLOADED_REST_LOST = { PACKAGE, "IDS_EMAIL_POP_ONLY_PART_OF_THE_MESSAGE_HAS_BEEN_DOWNLOADED_IF_YOU_CONTINUE_THE_REST_OF_THE_MESSAGE_MAY_BE_LOST" };
static email_string_t EMAIL_VIEWER_BUTTON_CONTINUE = { PACKAGE, "IDS_EMAIL_BUTTON_CONTINUE_ABB" };
static email_string_t EMAIL_VIEWER_STRING_NULL = { NULL, NULL };

/* module */
static int _viewer_module_create(email_module_t *self, app_control_h params);
static void _viewer_module_on_message(email_module_t *self, app_control_h msg);
static void _viewer_module_on_event(email_module_t *self, email_module_event_e event);

/* view */
static int _viewer_create(email_view_t *self);
static void _viewer_destroy(email_view_t *self);
static void _viewer_activate(email_view_t *self, email_view_state prev_state);
static void _viewer_show(email_view_t *self);
static void _viewer_hide(email_view_t *self);
static void _viewer_update(email_view_t *self, int flags);
static void _viewer_on_back_key(email_view_t *self);

/* viewer theme */
static void _viewer_initialize_theme(EmailViewerUGD *ug_data);
static void _viewer_deinitialize_theme(EmailViewerUGD *ug_data);

/*event update functions*/
static void _viewer_update_on_orientation_change(EmailViewerUGD *ug_data, bool isLandscape);
static void _viewer_update_on_region_change(EmailViewerUGD *ug_data);
static void _viewer_update_on_language_change(EmailViewerUGD *ug_data);

static void __viewer_add_main_callbacks(EmailViewerUGD *ug_data, void *data);
static void _viewer_add_main_callbacks(EmailViewerUGD *ug_data);
static void _viewer_update_data_on_start(void *data);

/* View related */
static VIEWER_ERROR_TYPE_E _viewer_create_base_view(EmailViewerUGD *ug_data);
static void _viewer_get_window_sizes(EmailViewerUGD *ug_data);
static void _create_webview_layout(void *data);

static void _timezone_changed_cb(system_settings_key_e key, void *data);

static void _viewer_set_webview_size(EmailViewerUGD *ug_data, int height);

/* callback functions */
static void _more_cb(void *data, Evas_Object *obj, void *event_info);
static void _download_msg_btn_clicked_cb(void *data, Evas_Object *obj, void *event_info);

static void _popup_response_cb(void *data, Evas_Object *obj, void *event_info);
static void _popup_response_delete_ok_cb(void *data, Evas_Object *obj, void *event_info);
static void _popup_response_continue_reply_cb(void *data, Evas_Object *obj, void *event_info);
static void _popup_response_continue_reply_all_cb(void *data, Evas_Object *obj, void *event_info);
static void _popup_response_continue_forward_cb(void *data, Evas_Object *obj, void *event_info);
static void _popup_response_to_destroy_cb(void *data, Evas_Object *obj, void *event_info);

static void _outter_scroller_bottom_hit_cb(void *data, Evas_Object *obj, void *event_info);
static void _outter_scroller_top_hit_cb(void *data, Evas_Object *obj, void *event_info);

static Eina_Bool _viewer_launch_email_application_cb(void *data);

static int _construct_viewer_data(EmailViewerUGD *ug_data);
static void _destroy_viewer(EmailViewerUGD *ug_data);

#ifdef SHARED_MODULES_FEATURE
EMAIL_API email_module_t *email_module_alloc()
#else
email_module_t *viewer_module_alloc()
#endif
{
	debug_enter();

	EmailViewerModule *md = MEM_ALLOC(md, 1);
	if (!md) {
		debug_error("md is NULL");
		return NULL;
	}

	_g_md = &md->view;

	md->base.create = _viewer_module_create;
	md->base.on_message = _viewer_module_on_message;
	md->base.on_event = _viewer_module_on_event;

	return &md->base;
}

static void _viewer_set_webview_size(EmailViewerUGD *ug_data, int height)
{
	debug_enter();

	retm_if(ug_data->loaded_module, "Viewer module in background");
	int min_height = 0, max_height = 0;

	int win_h = 0, win_w = 0;
	elm_win_screen_size_get(ug_data->base.module->win, NULL, NULL, &win_w, &win_h);
	debug_log("==> main layout [w, h] = [%d, %d]", win_w, win_h);
	debug_log("ug_data->header_height : %d", ug_data->header_height);

	if (ug_data->isLandscape) {
		min_height = win_w - ug_data->header_height;
		max_height = win_w;
		ug_data->webview_width = win_h;
	} else {
		min_height = win_h - ug_data->header_height;
		max_height = win_h;
		ug_data->webview_width = win_w;
	}

	if (height <= min_height) {
		debug_log("min");
		ug_data->webview_height = min_height;
	} else {
		debug_log("max");
		ug_data->webview_height = max_height;
	}

	debug_log("min_height: %d height :%d max_height :%d ", min_height, height, max_height);
	debug_log("ug_data->webview_height: %d", ug_data->webview_height);

	debug_leave();
}

void viewer_resize_webview(EmailViewerUGD *ug_data, int height)
{
	debug_enter();
	retm_if(ug_data == NULL, "Invalid parameter: ug_data[NULL]");
	_viewer_set_webview_size(ug_data, height);

	evas_object_size_hint_min_set(ug_data->webview, ug_data->webview_width, ug_data->webview_height);
	evas_object_size_hint_max_set(ug_data->webview, ug_data->webview_width, ug_data->webview_height);
}

void viewer_back_key_press_handler(EmailViewerUGD *ug_data)
{
	debug_enter();
	int ret = APP_CONTROL_ERROR_NONE;

	if (ug_data->service_handle) {
		ret = app_control_send_terminate_request(ug_data->service_handle);
		debug_warning_if(ret != APP_CONTROL_ERROR_NONE, "app_control_send_terminate_request() failed! ret:[%d]", ret);

		ret = app_control_destroy(ug_data->service_handle);
		debug_warning_if(ret != APP_CONTROL_ERROR_NONE, "app_control_destroy() failed! ret:[%d]", ret);
		ug_data->service_handle = NULL;
	}

	if (ug_data->b_load_finished == EINA_FALSE) {
		debug_log("b_load_finished is not yet done");
		return;
	}

	if (ug_data->webview) {
		if (ewk_view_text_selection_clear(ug_data->webview)) {
			debug_log("ewk_view_text_selection_clear() failed");
			return;
		}
	}
	debug_log("ug_destroy_me");
	ug_data->need_pending_destroy = EINA_TRUE;
	evas_object_smart_callback_del(ug_data->more_btn, "clicked", _more_cb);
	email_module_make_destroy_request(ug_data->base.module);
	debug_leave();
}

static void __viewer_add_main_callbacks(EmailViewerUGD *ug_data, void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");
	retm_if(!ug_data, "Invalid parameter: ug_data is NULL!");

	/* indicator */
	elm_win_indicator_mode_set(ug_data->base.module->win, ELM_WIN_INDICATOR_SHOW);
	elm_win_indicator_opacity_set(ug_data->base.module->win, ELM_WIN_INDICATOR_OPAQUE);

	viewer_open_pattern_generator();
	if (email_register_timezone_changed_callback(_timezone_changed_cb, data) == -1) {
		debug_log("Failed to register system settings callback for time changed");
		ug_data->eViewerErrorType = VIEWER_ERROR_FAIL;
	}

	debug_leave();
}

static void _viewer_add_main_callbacks(EmailViewerUGD *ug_data)
{
	debug_enter();
	retm_if(ug_data == NULL, "Invalid parameter: ug_data[NULL]");

	__viewer_add_main_callbacks(ug_data, (void *)ug_data);

	debug_leave();
}

static VIEWER_ERROR_TYPE_E _initialize_services(void *data)
{
	debug_enter();
	retvm_if(!data, VIEWER_ERROR_FAIL, "data is NULL");

	/* DBUS */
	if (noti_mgr_dbus_receiver_setup(data) == EINA_FALSE) {
		debug_log("Failed to initialize dbus");
		return VIEWER_ERROR_DBUS_FAIL;
	}

	/* email engine initialize */
	if (email_engine_initialize() == FALSE) {
		debug_log("Failed to initialize email engine");
		return VIEWER_ERROR_ENGINE_INIT_FAILED;
	}

	if (email_contacts_init_service() != CONTACTS_ERROR_NONE) {
		debug_error("Fail of email_contacts_init_service !!!");
		return VIEWER_ERROR_SERVICE_INIT_FAIL;
	}

	debug_leave();
	return VIEWER_ERROR_NONE;
}

int _viewer_module_create(email_module_t *self, app_control_h params)
{
	debug_enter();
	retvm_if(!self, -1, "Invalid parameter: self is NULL!");

	EmailViewerModule *md = (EmailViewerModule *)self;
	EmailViewerUGD *ug_data = &md->view;

	viewer_initialize_data(ug_data);

	if (!params) {
		debug_log("service handle is NULL");
		ug_data->eViewerErrorType = VIEWER_ERROR_INVALID_ARG;
	}

	int ret = viewer_initial_data_pre_parse_arguments(ug_data, params);
	if (ret != VIEWER_ERROR_NONE) {
		debug_log("viewer_initial_data_pre_parse_arguments() failed! (error type: %d)", ret);
		ug_data->eViewerErrorType = ret;
	}

	ug_data->base.create = _viewer_create;
	ug_data->base.destroy = _viewer_destroy;
	ug_data->base.activate = _viewer_activate;
	ug_data->base.show = _viewer_show;
	ug_data->base.hide = _viewer_hide;
	ug_data->base.update = _viewer_update;
	ug_data->base.on_back_key = _viewer_on_back_key;

	ret = email_module_create_view(&md->base, &ug_data->base);
	if (ret != 0) {
		debug_error("email_module_create_view(): failed (%d)", ret);
		return -1;
	}

	debug_leave();
	return 0;
}

int _viewer_create(email_view_t *self)
{
	debug_enter();
	retvm_if(!self, -1, "Invalid parameter: self is NULL!");

	EmailViewerUGD *ug_data = (EmailViewerUGD *)self;

	int ret = _construct_viewer_data(ug_data);
	if (ret != VIEWER_ERROR_NONE) {
		debug_log("_construct_viewer_data() failed! (error type: %d)", ret);
		ug_data->eViewerErrorType = ret;
	}

	ret = _initialize_services(ug_data);
	if (ret != VIEWER_ERROR_NONE) {
		ug_data->eViewerErrorType = ret;
	}

	ug_data->create_contact_arg = CONTACTUI_REQ_ADD_EMAIL;
	_viewer_initialize_theme(ug_data);
	_viewer_create_base_view(ug_data);
	_create_webview_layout(ug_data);
	viewer_get_webview(ug_data);
	header_create_view(ug_data);
	header_update_view(ug_data);

	create_body_progress(ug_data);
	ug_data->b_viewer_hided = FALSE;
	_viewer_add_main_callbacks(ug_data);

	debug_leave();
	return 0;
}

static VIEWER_ERROR_TYPE_E _viewer_process_error_popup(EmailViewerUGD *ug_data)
{
	debug_enter();

	debug_log("eViewerErrorType:%d", ug_data->eViewerErrorType);
	if (ug_data->eViewerErrorType != VIEWER_ERROR_NONE) {
		switch (ug_data->eViewerErrorType) {
		case VIEWER_ERROR_NOT_ALLOWED:
			debug_warning("Lauching viewer failed! VIEWER_ERROR_NOT_ALLOWED");
			util_create_notify(ug_data, EMAIL_VIEWER_HEADER_UNAVAILABLE,
					EMAIL_VIEWER_POP_SECURITY_POLICY_RESTRICTS_USE_OF_POP_IMAP_EMAIL, 1,
					EMAIL_VIEWER_STRING_OK, _popup_response_to_destroy_cb, EMAIL_VIEWER_STRING_NULL, NULL, NULL);
			break;

		case VIEWER_ERROR_NO_DEFAULT_ACCOUNT:
		case VIEWER_ERROR_NO_ACCOUNT_LIST:
		case VIEWER_ERROR_NO_ACCOUNT:
			debug_warning("Lauching viewer failed! Failed to get account! [%d]", ug_data->eViewerErrorType);
			util_create_notify(ug_data, EMAIL_VIEWER_HEADER_UNABLE_TO_PERFORM_ACTION,
					EMAIL_VIEWER_POP_THERE_IS_NO_ACCOUNT_CREATE_A_NEW_ACCOUNT_FIRST, 0,
					EMAIL_VIEWER_STRING_NULL, NULL, EMAIL_VIEWER_STRING_NULL, NULL, NULL);
			DELETE_TIMER_OBJECT(ug_data->launch_timer);
			ug_data->launch_timer = ecore_timer_add(2.0f, _viewer_launch_email_application_cb, ug_data);
			break;

		case VIEWER_ERROR_GET_DATA_FAIL:
			debug_warning("Lauching viewer failed! VIEWER_ERROR_GET_DATA_FAIL");
			util_create_notify(ug_data, EMAIL_VIEWER_HEADER_UNABLE_TO_PERFORM_ACTION,
						EMAIL_VIEWER_BODY_SELECTED_DATA_NOT_FOUND, 1,
						EMAIL_VIEWER_STRING_OK, _popup_response_to_destroy_cb, EMAIL_VIEWER_STRING_NULL, NULL, NULL);
			break;

		case VIEWER_ERROR_FAIL:
		case VIEWER_ERROR_UNKOWN_TYPE:
		case VIEWER_ERROR_INVALID_ARG:
			debug_warning("Lauching viewer failed! Due to [%d]", ug_data->eViewerErrorType);
			util_create_notify(ug_data, EMAIL_VIEWER_HEADER_UNABLE_TO_PERFORM_ACTION,
						EMAIL_VIEWER_UNKNOWN_ERROR_HAS_OCCURED, 1,
						EMAIL_VIEWER_STRING_OK, _popup_response_to_destroy_cb, EMAIL_VIEWER_STRING_NULL, NULL, NULL);

			break;

		case VIEWER_ERROR_ENGINE_INIT_FAILED:
		case VIEWER_ERROR_DBUS_FAIL:
		case VIEWER_ERROR_SERVICE_INIT_FAIL:
		case VIEWER_ERROR_OUT_OF_MEMORY:
			debug_warning("Lauching viewer failed! Due to [%d]", ug_data->eViewerErrorType);
			util_create_notify(ug_data, EMAIL_VIEWER_HEADER_UNABLE_TO_PERFORM_ACTION,
						EMAIL_VIEWER_POP_FAILED_TO_START_EMAIL_APPLICATION, 1,
						EMAIL_VIEWER_STRING_OK, _popup_response_to_destroy_cb, EMAIL_VIEWER_STRING_NULL, NULL, NULL);
			break;

		case VIEWER_ERROR_STORAGE_FULL:
			debug_warning("Lauching viewer failed! VIEWER_ERROR_STORAGE_FULL");
			email_string_t content = { PACKAGE, "IDS_EMAIL_POP_THERE_IS_NOT_ENOUGH_SPACE_IN_YOUR_DEVICE_STORAGE_GO_TO_SETTINGS_POWER_AND_STORAGE_STORAGE_THEN_DELETE_SOME_FILES_AND_TRY_AGAIN" };
			email_string_t btn2 = { PACKAGE, "IDS_EMAIL_BUTTON_SETTINGS" };
			util_create_notify(ug_data, EMAIL_VIEWER_HEADER_UNABLE_TO_PERFORM_ACTION, content, 2,
						EMAIL_VIEWER_STRING_OK, _popup_response_to_destroy_cb,
						btn2, viewer_storage_full_popup_response_cb, NULL);
			break;

		default:
			break;
		}

		debug_leave();
		return ug_data->eViewerErrorType;
	}

	debug_leave();
	return VIEWER_ERROR_NONE;
}

void _viewer_activate(email_view_t *self, email_view_state prev_state)
{
	debug_enter();
	retm_if(!self, "Invalid parameter: self is NULL!");

	EmailViewerUGD *ug_data = (EmailViewerUGD *)self;

	if (prev_state != EV_STATE_CREATED) {
		return;
	}

	VIEWER_ERROR_TYPE_E error = _viewer_process_error_popup(ug_data);
	if (error != VIEWER_ERROR_NONE) {
		return;
	}
	_viewer_update_data_on_start(ug_data);

	debug_leave();
}

static void _viewer_update_data_on_start(void *data)
{
	debug_enter();

	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	viewer_check_body_download(ug_data);

	debug_leave();
}

void _viewer_hide(email_view_t *self)
{
	debug_enter();
	retm_if(self == NULL, "Invalid parameter: self is NULL!");

	EmailViewerUGD *ug_data = (EmailViewerUGD *)self;

	viewer_hide_webview(ug_data);
	elm_object_focus_allow_set(ug_data->en_subject, EINA_FALSE);

	debug_leave();
}

void _viewer_show(email_view_t *self)
{
	debug_enter();
	retm_if(!self, "Invalid parameter: self is NULL!");

	EmailViewerUGD *ug_data = (EmailViewerUGD *)self;

	viewer_show_webview(ug_data);
	elm_object_focus_allow_set(ug_data->en_subject, EINA_TRUE);

	debug_leave();
}

void _viewer_destroy(email_view_t *self)
{
	debug_enter();
	retm_if(!self, "Invalid parameter: self is NULL!");

	EmailViewerUGD *ug_data = (EmailViewerUGD *)self;

	_destroy_viewer(ug_data);

	debug_leave();
}

void _viewer_module_on_message(email_module_t *self, app_control_h msg)
{
	debug_enter();
	retm_if(!self, "Invalid parameter: self is NULL!");

	EmailViewerModule *md = (EmailViewerModule *)self;
	EmailViewerUGD *ug_data = &md->view;

	bool wait_for_composer = false;

	char *msg_type = NULL;
	app_control_get_extra_data(msg, EMAIL_BUNDLE_KEY_MSG, &(msg_type));

	if (ug_data->loaded_module && ug_data->is_composer_module_launched) {
		int ret;
		app_control_h service = NULL;

		ret = app_control_create(&service);
		debug_log("app_control_create: %d", ret);
		retm_if(service == NULL, "service create failed: service[NULL]");

		ret = app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_MSG, EMAIL_BUNDLE_VAL_EMAIL_COMPOSER_SAVE_DRAFT);
		debug_log("app_control_add_extra_data: %d", ret);

		email_module_send_message(ug_data->loaded_module, service);

		ret = app_control_destroy(service);
		debug_log("app_control_destroy: %d", ret);

		wait_for_composer = true;
	}

	if (g_strcmp0(msg_type, EMAIL_BUNDLE_VAL_VIEWER_DESTROY_VIEW) == 0) {
		debug_log("Email viewer need to be closed");
		FREE(msg_type);
		if (wait_for_composer) {
			ug_data->need_pending_destroy = EINA_TRUE;
		} else {
			email_module_make_destroy_request(ug_data->base.module);
		}
		return;
	}

	if (ug_data->loaded_module && !ug_data->is_composer_module_launched) {
		email_module_destroy(ug_data->loaded_module);
		ug_data->loaded_module = NULL;
	}

	if (g_strcmp0(msg_type, EMAIL_BUNDLE_VAL_VIEWER_RESTORE_VIEW) == 0) {
		debug_log("Restore Email viewer");
		_reset_view(ug_data, EINA_TRUE);
		FREE(msg_type);
		return;
	}

	FREE(msg_type);

	debug_log("Hide previous mail data");
	_hide_view(ug_data);

	_g_md = ug_data;

	int ret = viewer_initial_data_pre_parse_arguments(ug_data, msg);
	if (ret != VIEWER_ERROR_NONE) {
		debug_log("viewer_initial_data_pre_parse_arguments() failed! (error type: %d)", ret);
		ug_data->eViewerErrorType = ret;
	}

	/* The viewer layout is not destroyed. Therefore, it runs with EINA_TRUE. */
	ret = _construct_viewer_data(ug_data);
	if (ret != VIEWER_ERROR_NONE) {
		debug_log("_construct_viewer_data() failed! (error type: %d)", ret);
		ug_data->eViewerErrorType = ret;
	}

	if (noti_mgr_dbus_receiver_setup(ug_data) == EINA_FALSE) {
		debug_log("Failed to initialize dbus");
		ug_data->eViewerErrorType = VIEWER_ERROR_DBUS_FAIL;
	}

	if (ug_data->webview != NULL) {
		Ewk_Settings *ewk_settings = ewk_view_settings_get(ug_data->webview);
		debug_log("b_show_remote_images is %d", ug_data->b_show_remote_images);
		if (ewk_settings_loads_images_automatically_set(ewk_settings, ug_data->b_show_remote_images) == EINA_FALSE) { /* ewk_settings_load_remote_images_set for remote image */
			debug_log("SET show images is FAILED!");
		}
	}

	edje_object_signal_emit(_EDJ(ug_data->base.content), "hide_button", "elm.event.btn");

	int angle = elm_win_rotation_get(ug_data->base.module->win);
	ug_data->isLandscape = ((angle == APP_DEVICE_ORIENTATION_90) || (angle == APP_DEVICE_ORIENTATION_270)) ? true : false;
	debug_log("isLandscapte : %d", ug_data->isLandscape);

	_reset_view(ug_data, EINA_FALSE);

	create_body_progress(ug_data);

	ug_data->b_viewer_hided = FALSE;

	VIEWER_ERROR_TYPE_E error = _viewer_process_error_popup(ug_data);
	if (error != VIEWER_ERROR_NONE) {
		return;
	}

	debug_leave();
}

static void _viewer_module_on_event(email_module_t *self, email_module_event_e event)
{
	debug_enter();
	retm_if(!self, "Invalid parameter: self is NULL!");

	EmailViewerModule *md = (EmailViewerModule *)self;
	EmailViewerUGD *ug_data = &md->view;
	debug_log("event = %d", event);

	switch (event) {
	case EM_EVENT_LOW_MEMORY_SOFT:
		debug_log("Low memory: %d", event);
		viewer_webview_handle_mem_warning(ug_data, false);
		break;
	case EM_EVENT_LOW_MEMORY_HARD:
		debug_log("Low memory: %d", event);
		viewer_webview_handle_mem_warning(ug_data, true);
		break;

	default:
		debug_log("other event: %d", event);
		break;
	}

	debug_leave();
}

static void _viewer_update(email_view_t *self, int flags)
{
	debug_enter();
	retm_if(!self, "Invalid parameter: self is NULL!");

	EmailViewerUGD *ug_data = (EmailViewerUGD *)self;
	debug_log("flags = %d", flags);

	if (flags & EVUF_LANGUAGE_CHANGED) {
		_viewer_update_on_language_change(ug_data);
	}

	if (flags & EVUF_ORIENTATION_CHANGED) {
		switch (ug_data->base.orientation) {
		case APP_DEVICE_ORIENTATION_0:
		case APP_DEVICE_ORIENTATION_180:
			if (ug_data->webview) {
				ewk_view_orientation_send(ug_data->webview,
						ug_data->base.orientation == APP_DEVICE_ORIENTATION_0 ? 0 : 180);
			}
			if (ug_data->isLandscape == true) {
				_viewer_update_on_orientation_change(ug_data, false);
			}
			break;

		case APP_DEVICE_ORIENTATION_270:
		case APP_DEVICE_ORIENTATION_90:
			if (ug_data->webview) {
				ewk_view_orientation_send(ug_data->webview,
						ug_data->base.orientation == APP_DEVICE_ORIENTATION_270 ? 90 : -90);
			}
			if (ug_data->isLandscape == false) {
				_viewer_update_on_orientation_change(ug_data, true);
			}
			break;
		}
	}

	if (flags & EVUF_REGION_FMT_CHANGED) {
		_viewer_update_on_region_change(ug_data);
	}

	debug_leave();
}

static void _viewer_update_on_orientation_change(EmailViewerUGD *ug_data, bool isLandscape)
{
	debug_enter();
	retm_if(!ug_data, "Invalid parameter: ug_data is NULL!");

	ug_data->isLandscape = isLandscape;
	viewer_set_vertical_scroller_position(ug_data);
	/* resize webview */
	double scale = ewk_view_scale_get(ug_data->webview);
	int content_height = 0;
	ewk_view_contents_size_get(ug_data->webview, NULL, &content_height);
	if (content_height) {
		viewer_resize_webview(ug_data, content_height * scale);
	}

	debug_log("content_height :%d scale :%f", content_height, scale);

	debug_leave();
}

static void _viewer_update_on_region_change(EmailViewerUGD *ug_data)
{
	debug_enter();
	retm_if(!ug_data, "Invalid parameter: ug_data is NULL!");

	header_update_date(ug_data);

	debug_leave();
}

static void _viewer_update_on_language_change(EmailViewerUGD *ug_data)
{
	debug_enter();
	retm_if(!ug_data, "Invalid parameter: ug_data is NULL!");

	char *_subject = elm_entry_utf8_to_markup(ug_data->subject);
	if (!g_strcmp0(_subject, "")) {
		g_free(_subject);
		_subject = g_strdup(email_get_email_string("IDS_EMAIL_SBODY_NO_SUBJECT_M_NOUN"));
		elm_entry_entry_set(ug_data->en_subject, _subject);
	}
	g_free(_subject);

	if (ug_data->to_mbe) {
		elm_object_text_set(ug_data->to_mbe, email_get_email_string("IDS_EMAIL_BODY_TO_M_RECIPIENT"));
	}
	if (ug_data->cc_mbe) {
		elm_object_text_set(ug_data->cc_mbe, email_get_email_string("IDS_EMAIL_BODY_CC"));
	}
	if (ug_data->bcc_mbe) {
		elm_object_text_set(ug_data->bcc_mbe, email_get_email_string("IDS_EMAIL_BODY_BCC"));
	}

	header_update_attachment_summary_info(ug_data);

	/* This code is specific for popups where string translation is not handled by EFL */
	if (ug_data->notify && ug_data->popup_element_type) {
		char str[MAX_STR_LEN] = { 0, };
		char first_str[MAX_STR_LEN] = { 0, };
		switch (ug_data->popup_element_type) {
		case POPUP_ELEMENT_TYPE_TITLE:
			break;
		case POPUP_ELEMENT_TYPE_CONTENT:
			if (ug_data->extra_variable_type == VARIABLE_TYPE_INT) {
				if (ug_data->str_format2) {
					if (ug_data->package_type2 == PACKAGE_TYPE_SYS_STRING) {
						snprintf(str, sizeof(str), ug_data->str_format2, email_get_system_string(ug_data->translation_string_id2), ug_data->extra_variable_integer);
					} else if (ug_data->package_type2 == PACKAGE_TYPE_PACKAGE) {
						snprintf(str, sizeof(str), ug_data->str_format2, email_get_email_string(ug_data->translation_string_id2), ug_data->extra_variable_integer);
					}
				} else {
					if (ug_data->package_type2 == PACKAGE_TYPE_SYS_STRING) {
						snprintf(str, sizeof(str), email_get_system_string(ug_data->translation_string_id2), ug_data->extra_variable_integer);
					} else if (ug_data->package_type2 == PACKAGE_TYPE_PACKAGE) {
						snprintf(str, sizeof(str), email_get_email_string(ug_data->translation_string_id2), ug_data->extra_variable_integer);
					}
				}
			} else if (ug_data->extra_variable_type == VARIABLE_TYPE_STRING) {
				if (ug_data->str_format2) {
					if (ug_data->package_type2 == PACKAGE_TYPE_SYS_STRING) {
						snprintf(str, sizeof(str), ug_data->str_format2, email_get_system_string(ug_data->translation_string_id2), ug_data->extra_variable_string);
					} else if (ug_data->package_type2 == PACKAGE_TYPE_PACKAGE) {
						snprintf(str, sizeof(str), ug_data->str_format2, email_get_email_string(ug_data->translation_string_id2), ug_data->extra_variable_string);
					}
				} else {
					if (ug_data->package_type2 == PACKAGE_TYPE_SYS_STRING) {
						snprintf(str, sizeof(str), email_get_system_string(ug_data->translation_string_id2), ug_data->extra_variable_string);
					} else if (ug_data->package_type2 == PACKAGE_TYPE_PACKAGE) {
						snprintf(str, sizeof(str), email_get_email_string(ug_data->translation_string_id2), ug_data->extra_variable_string);
					}
				}
			} else {
				if (ug_data->package_type2 == PACKAGE_TYPE_SYS_STRING) {
					snprintf(str, sizeof(str), "%s", email_get_system_string(ug_data->translation_string_id2));
				} else if (ug_data->package_type2 == PACKAGE_TYPE_PACKAGE) {
					snprintf(str, sizeof(str), "%s", email_get_email_string(ug_data->translation_string_id2));
				} else {
					snprintf(str, sizeof(str), "%s", ug_data->translation_string_id2);
				}
			}
			if (ug_data->str_format1) {
				if (ug_data->package_type1 == PACKAGE_TYPE_SYS_STRING) {
					snprintf(first_str, sizeof(first_str), ug_data->str_format1, email_get_system_string(ug_data->translation_string_id1), str);
				} else if (ug_data->package_type1 == PACKAGE_TYPE_PACKAGE) {
					snprintf(first_str, sizeof(first_str), ug_data->str_format1, email_get_email_string(ug_data->translation_string_id1), str);
				} else {
					snprintf(first_str, sizeof(first_str), ug_data->str_format1, ug_data->translation_string_id1, str);
				}
			} else {
				snprintf(first_str, sizeof(first_str), "%s", ug_data->translation_string_id1);
			}
			elm_object_text_set(ug_data->notify, first_str);
			break;
		case POPUP_ELEMENT_TYPE_BTN1:
			break;
		case POPUP_ELEMENT_TYPE_BTN2:
			break;
		case POPUP_ELEMENT_TYPE_BTN3:
			break;

		}
	}
	debug_leave();
}

static Eina_Bool _viewer_launch_email_application_cb(void *data)
{
	debug_enter();
	retvm_if(data == NULL, ECORE_CALLBACK_CANCEL, "Invalid parameter: data[NULL]");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	app_control_h svc_handle = NULL;
	int ret = APP_CONTROL_ERROR_NONE;

	ret = app_control_create(&svc_handle);
	retvm_if((ret != APP_CONTROL_ERROR_NONE || !svc_handle), ECORE_CALLBACK_CANCEL, "app_control_create() failed! ret:[%d]", ret);

	ret = app_control_add_extra_data(svc_handle, EMAIL_BUNDLE_KEY_RUN_TYPE, "9"); /* RUN_SETTING_ACCOUNT_ADD */
	debug_warning_if(ret != APP_CONTROL_ERROR_NONE, "app_control_add_extra_data() failed! ret:[%d]", ret);

	ret = app_control_set_app_id(svc_handle, PKGNAME);
	debug_warning_if(ret != APP_CONTROL_ERROR_NONE, "app_control_set_app_id() failed! ret:[%d]", ret);

	/* Launch application */
	ret = app_control_send_launch_request(svc_handle, NULL, NULL);
	debug_warning_if(ret != APP_CONTROL_ERROR_NONE, "app_control_send_launch_request() failed! ret:[%d]", ret);

	ret = app_control_destroy(svc_handle);
	debug_warning_if(ret != APP_CONTROL_ERROR_NONE, "app_control_destroy() failed! ret:[%d]", ret);

	ug_data->launch_timer = NULL;
	_popup_response_to_destroy_cb(ug_data, ug_data->notify, NULL);

	debug_leave();
	return ECORE_CALLBACK_CANCEL;
}

static int _construct_viewer_data(EmailViewerUGD *ug_data)
{
	debug_enter();
	retvm_if(ug_data == NULL, VIEWER_ERROR_FAIL, "Invalid parameter: ug_data[NULL]");

	debug_log("pointer %p account_id [%d], mail_id [%d], mailbox_id [%d]", ug_data, ug_data->account_id, ug_data->mail_id, ug_data->mailbox_id);
	if (((ug_data->account_id <= 0 || (ug_data->mail_id <= 0) || (ug_data->mailbox_id <= 0)) && (ug_data->viewer_type != EML_VIEWER))) {
		debug_log("Required parameters are missing!");
		return VIEWER_ERROR_INVALID_ARG;
	}

	if (viewer_create_temp_folder(ug_data) < 0) {
		debug_log("creating email viewer temp folder is failed");
		return VIEWER_ERROR_FAIL;
	}

	G_FREE(ug_data->temp_viewer_html_file_path);
	ug_data->temp_viewer_html_file_path = g_strdup_printf("%s/%s", ug_data->temp_folder_path, EMAIL_VIEWER_TMP_HTML_FILE);

	int ret = viewer_reset_mail_data(ug_data);
	if (ret != VIEWER_ERROR_NONE) {
		debug_log("viewer_reset_mail_data() failed! (error type: %d)", ret);
		return ret;
	}

	viewer_create_account_data(ug_data);

	if (ug_data->viewer_type == EMAIL_VIEWER) {
		retvm_if(ug_data->mail_info == NULL, VIEWER_ERROR_GET_DATA_FAIL, "ug_data->mail_info is NULL.");

		email_mail_data_t *mail_data = ug_data->mail_info;
		ug_data->mailbox_type = mail_data->mailbox_type;
		debug_log("mailbox_type:%d", ug_data->mailbox_type);
		debug_secure("server_mailbox_name:%s", mail_data->server_mailbox_name);
	}
	return VIEWER_ERROR_NONE;
}

static Evas_Object *_create_fullview(Evas_Object *parent)
{
	debug_enter();
	retvm_if(parent == NULL, NULL, "Invalid parameter: parent[NULL]");
	Evas_Object *layout = elm_layout_add(parent);
	elm_layout_theme_set(layout, "layout", "application", "default");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(layout);
	debug_leave();
	return layout;
}

static Evas_Object *_create_scroller(Evas_Object *parent)
{
	debug_enter();
	retvm_if(parent == NULL, NULL, "Invalid parameter: parent[NULL]");
	Evas_Object *scroller = elm_scroller_add(parent);
	evas_object_propagate_events_set(scroller, EINA_FALSE);
	elm_scroller_bounce_set(scroller, EINA_FALSE, EINA_TRUE);
	elm_scroller_policy_set(scroller, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
	evas_object_show(scroller);
	debug_leave();
	return scroller;
}

static Evas_Object *_create_box(Evas_Object *parent)
{
	debug_enter();
	retvm_if(parent == NULL, NULL, "Invalid parameter: parent[NULL]");
	Evas_Object *box = email_color_box_add(parent);
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_box_align_set(box, EVAS_HINT_FILL, 0.0);
	evas_object_show(box);
	debug_leave();
	return box;
}

static void _viewer_get_window_sizes(EmailViewerUGD *ug_data)
{
	debug_enter();

	Evas_Coord width, height;
	evas_object_geometry_get(ug_data->base.module->win, NULL, NULL, &width, &height);
	ug_data->main_w = width;
	ug_data->main_h = height;
	ug_data->scale_factor = elm_app_base_scale_get() / elm_config_scale_get();

	debug_log("WINDOW W[%d] H[%d]", width, height);
	debug_log("ELM Scale[%f]", ug_data->scale_factor);

}

static VIEWER_ERROR_TYPE_E _viewer_create_base_view(EmailViewerUGD *ug_data)
{
	debug_enter();
	retvm_if(ug_data == NULL, VIEWER_ERROR_FAIL, "Invalid parameter: ug_data[NULL]");

	_viewer_get_window_sizes(ug_data);

	/* add container layout */
	ug_data->base.content = viewer_load_edj(ug_data->base.module->navi, email_get_viewer_theme_path(), "ev/viewer/base", EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	if (ug_data->theme) {
		elm_object_theme_set(ug_data->base.content, ug_data->theme);
	}
	retvm_if(ug_data->base.content == NULL, VIEWER_ERROR_FAIL, "Cannot load edj!");
	evas_object_show(ug_data->base.content);

	/* create scroller */
	ug_data->scroller = _create_scroller(ug_data->base.content);
	retvm_if(ug_data->scroller == NULL, VIEWER_ERROR_FAIL, "scroller is NULL!");
	elm_object_part_content_set(ug_data->base.content, "ev.swallow.content", ug_data->scroller);
	evas_object_smart_callback_add(ug_data->scroller, "edge,bottom", _outter_scroller_bottom_hit_cb, ug_data);
	evas_object_smart_callback_add(ug_data->scroller, "edge,top", _outter_scroller_top_hit_cb, ug_data);
	evas_object_show(ug_data->scroller);

	/* create flick detection layer */
	viewer_set_flick_layer(ug_data);

	/* create base layout */
	ug_data->base_ly = viewer_load_edj(ug_data->scroller, email_get_viewer_theme_path(), "ev/layout/base", EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	retvm_if(ug_data->base_ly == NULL, VIEWER_ERROR_FAIL, "Cannot load edj!");
	elm_object_content_set(ug_data->scroller, ug_data->base_ly);
	evas_object_show(ug_data->base_ly);

	/* create main box */
	ug_data->main_bx = _create_box(ug_data->scroller);
	retvm_if(ug_data->main_bx == NULL, VIEWER_ERROR_FAIL, "main_bx is NULL!");
	elm_object_part_content_set(ug_data->base_ly, "ev.swallow.box", ug_data->main_bx);
	evas_object_show(ug_data->main_bx);

	/* push navigation bar */
	email_module_view_push(&ug_data->base, NULL, EVPF_NO_BACK_BUTTON | EVPF_NO_TITLE);

	/* This button is set for devices which doesn't have H/W more key. */
	ug_data->more_btn = elm_button_add(ug_data->base.module->navi);
	elm_object_style_set(ug_data->more_btn, "naviframe/more/default");
	evas_object_smart_callback_add(ug_data->more_btn, "clicked", _more_cb, ug_data);
	elm_object_item_part_content_set(ug_data->base.navi_item, "toolbar_more_btn", ug_data->more_btn);

	debug_leave();
	return VIEWER_ERROR_NONE;
}

void _reset_view(EmailViewerUGD *ug_data, Eina_Bool update_body)
{
	debug_enter();
	retm_if(ug_data == NULL, "Invalid parameter: ug_data[NULL]");

	if (ug_data->base.module->views_count > 1) {
		email_module_exit_view(&ug_data->attachment_view);
	}

	debug_log("@@@@@@@@@@ isLandscape = %d", ug_data->isLandscape);
	if (ug_data->isLandscape == true) {
		debug_log("In Split view - Hiding navi bar");
		/* Set scroller to start - height made 480 to consider viewer in split view */
		elm_scroller_region_show(ug_data->scroller, 0, 0, (int)((double)ug_data->main_h * 0.6), ug_data->main_w);
	} else {
		debug_log("In Full view - Showing navi bar");
		/* Set scroller to start */
		elm_scroller_region_show(ug_data->scroller, 0, 0, ug_data->main_w, ug_data->main_h);
	}

	if (elm_object_scroll_freeze_get(ug_data->scroller) > 0)
		elm_object_scroll_freeze_pop(ug_data->scroller);

	viewer_check_body_download(ug_data);

	if (update_body) {
		header_update_view(ug_data);
	} else {
		ug_data->is_recipient_ly_shown = EINA_FALSE;
		header_layout_unpack(ug_data);
		header_create_view(ug_data);
		header_update_view(ug_data);
	}

	debug_leave();
}

void _hide_view(EmailViewerUGD *ug_data)
{
	debug_enter();
	retm_if(ug_data == NULL, "Invalid parameter: ug_data[NULL]");

	ug_data->b_viewer_hided = TRUE;

	viewer_webkit_del_callbacks(ug_data);
	viewer_delete_account_data(ug_data);
	viewer_stop_ecore_running_apis(ug_data);
	viewer_delete_evas_objects(ug_data, EINA_TRUE);
	viewer_remove_hard_link_for_inline_images(ug_data);

	viewer_remove_temp_files(ug_data);
	viewer_remove_temp_folders(ug_data);
	viewer_free_viewer_data(ug_data);
	noti_mgr_dbus_receiver_remove(ug_data);

	viewer_initialize_data(ug_data);
	_g_md = NULL;
	debug_leave();
}

/**
 * initialize viewer's private data
 */
Eina_Bool viewer_initialize_data(EmailViewerUGD *ug_data)
{
	debug_enter();
	retvm_if(ug_data == NULL, EINA_FALSE, "Invalid parameter: ug_data[NULL]");

	ug_data->loaded_module = NULL;
	ug_data->service_handle = NULL;

	ug_data->main_w = 0;
	ug_data->main_h = 0;
	ug_data->eViewerErrorType = VIEWER_ERROR_NONE;

	/*dbus*/
	ug_data->viewer_dbus_conn = NULL;
	ug_data->viewer_network_id = 0;
	ug_data->viewer_storage_id = 0;

	/*combined scroller*/
	ug_data->combined_scroller = NULL;
	ug_data->webkit_scroll_y = 0;
	ug_data->webkit_scroll_x = 0;
	ug_data->header_height = 0;
	ug_data->trailer_height = 0;
	ug_data->total_height = 0;
	ug_data->total_width = 0;
	ug_data->main_scroll_y = 0;
	ug_data->main_scroll_h = 0;
	ug_data->main_scroll_w = 0;

	/* arguments */
	ug_data->account_id = 0;
	ug_data->account_type = 0;
	ug_data->mail_id = 0;
	ug_data->mailbox_id = 0;
	ug_data->account_email_address = NULL;
	ug_data->temp_folder_path = NULL;
	ug_data->temp_viewer_html_file_path = NULL;
	/* for preview */
	ug_data->temp_preview_folder_path = NULL;

	/* flags */
	ug_data->b_viewer_hided = 0;
	ug_data->b_partial_body = 0;
	ug_data->scroller_locked = 0;
	ug_data->is_composer_module_launched = FALSE;

	ug_data->can_destroy_on_msg_delete = false;
	ug_data->b_load_finished = EINA_FALSE;
	ug_data->b_show_remote_images = EINA_FALSE;
	ug_data->is_long_pressed = EINA_FALSE;
	ug_data->is_webview_scrolling = EINA_FALSE;
	ug_data->is_main_scroller_scrolling = EINA_TRUE;
	ug_data->is_outer_scroll_bot_hit = EINA_FALSE;
	ug_data->is_magnifier_opened = EINA_FALSE;
	ug_data->is_recipient_ly_shown = EINA_FALSE;
	ug_data->is_download_message_btn_clicked = EINA_FALSE;
	ug_data->is_cancel_sending_btn_clicked = EINA_FALSE;
	ug_data->is_storage_full_popup_shown = EINA_FALSE;
	ug_data->need_pending_destroy = EINA_FALSE;
	ug_data->is_top_webview_reached = EINA_FALSE;
	ug_data->is_bottom_webview_reached = EINA_FALSE;
	ug_data->is_scrolling_down = EINA_FALSE;
	ug_data->is_scrolling_up = EINA_FALSE;
	ug_data->is_webview_text_selected = EINA_FALSE;

	/*eml viewer*/
	ug_data->viewer_type = EMAIL_VIEWER;
	ug_data->eml_file_path = NULL;

	/*popup translation*/
	ug_data->popup_element_type = 0;
	ug_data->package_type1 = 0;
	ug_data->package_type2 = 0;
	ug_data->translation_string_id1 = NULL;
	ug_data->translation_string_id2 = NULL;
	ug_data->extra_variable_type = 0;
	ug_data->extra_variable_string = NULL;
	ug_data->extra_variable_integer = 0;
	ug_data->str_format1 = NULL;
	ug_data->str_format2 = NULL;

	/* for ug create */
	ug_data->create_contact_arg = 0;

	/* Scalable UI */
	ug_data->scale_factor = 0.0;
	ug_data->webview_width = 0;
	ug_data->webview_height = 0;

	/* rotation */
	ug_data->isLandscape = 0;

	/* mailbox list */
	ug_data->mailbox_type = EMAIL_MAILBOX_TYPE_NONE;
	ug_data->folder_list = NULL;
	ug_data->move_mailbox_list = NULL;
	ug_data->move_mailbox_count = 0;
	ug_data->move_status = 0;

	/* Evas Object */
	ug_data->en_subject = NULL;
	ug_data->subject_ly = NULL;
	ug_data->sender_ly = NULL;
	ug_data->attachment_ly = NULL;
	ug_data->webview_ly = NULL;
	ug_data->webview_button = NULL;
	ug_data->webview = NULL;
	ug_data->list_progressbar = NULL;
	ug_data->con_popup = NULL;
	ug_data->notify = NULL;
	ug_data->pb_notify = NULL;
	ug_data->pb_notify_lb = NULL;
	ug_data->dn_btn = NULL;
	ug_data->to_mbe = NULL;
	ug_data->to_ly = NULL;
	ug_data->cc_mbe = NULL;
	ug_data->cc_ly = NULL;
	ug_data->bcc_mbe = NULL;
	ug_data->bcc_ly = NULL;
	ug_data->to_recipients_cnt = 0;
	ug_data->cc_recipients_cnt = 0;
	ug_data->bcc_recipients_cnt = 0;
	ug_data->to_recipient_idler = NULL;
	ug_data->cc_recipient_idler = NULL;
	ug_data->bcc_recipient_idler = NULL;
	ug_data->idler_regionbringin = NULL;

	ug_data->attachment_save_thread = 0;
	ug_data->attachment_update_job = NULL;
	ug_data->attachment_update_timer = NULL;
	ug_data->attachment_data_list = NULL;
	ug_data->attachment_data_list_to_save = NULL;
	ug_data->preview_aid = NULL;
	ug_data->show_cancel_all_btn = false;
	ug_data->attachment_genlist = NULL;
	ug_data->save_all_btn = NULL;
	ug_data->attachment_group_item = NULL;
	ug_data->attachment_itc = NULL;
	ug_data->attachment_group_itc = NULL;

	ug_data->cancel_sending_ctx_item = NULL;

	ug_data->webview_data = NULL;

	/* Email Viewer Private variables */
	ug_data->file_id = NULL;
	ug_data->file_size = 0;
	ug_data->email_handle = 0;
	ug_data->mail_info = NULL;
	ug_data->attachment_info = NULL;
	ug_data->attachment_count = 0;

	/*Email Viewer Property Variables*/
	ug_data->mail_type = 0;
	ug_data->mktime = 0;
	ug_data->sender_address = NULL;
	ug_data->sender_display_name = NULL;
	ug_data->subject = NULL;
	ug_data->charset = NULL;
	ug_data->body = NULL;
	ug_data->body_uri = NULL;
	ug_data->attachment_info_list = NULL;
	ug_data->total_att_count = 0;
	ug_data->normal_att_count = 0;
	ug_data->total_att_size = 0;
	ug_data->has_html = FALSE;
	ug_data->has_attachment = FALSE;
	ug_data->has_inline_image = FALSE;
	ug_data->is_smil_mail = FALSE;
	ug_data->favorite = FALSE;
	ug_data->request_report = FALSE;

	ug_data->account = NULL;

	ug_data->launch_timer = NULL;
	ug_data->rcpt_scroll_corr = NULL;

	ug_data->recipient_address = NULL;
	ug_data->recipient_name = NULL;
	ug_data->selected_address = NULL;
	ug_data->selected_name = NULL;
	ug_data->recipient_contact_list_item = NULL;

	ug_data->passwd_popup = NULL;
	return EINA_TRUE;
}

static void _timezone_changed_cb(system_settings_key_e key, void *data)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	char *timezone_id = NULL;
	i18n_uchar utimezone_id[512] = { 0, };
	i18n_error_code_e status = I18N_ERROR_NONE;

	timezone_id = email_get_timezone_str();
	retm_if(timezone_id == NULL, "timezone_id is NULL.");

	i18n_ustring_copy_ua(utimezone_id, timezone_id);

	status = i18n_ucalendar_set_default_timezone(utimezone_id);

	if (status != I18N_ERROR_NONE) {
		debug_critical("i18n_ucalendar_set_default_timezone() failed: %d", status);
	}

	FREE(timezone_id);

	header_update_date(ug_data);
	debug_leave();
}

void create_body_progress(EmailViewerUGD *ug_data)
{
	debug_enter();
	retm_if(ug_data == NULL, "Invalid parameter: ug_data[NULL]");
	retm_if(ug_data->list_progressbar != NULL, "progressbar already present");
	/*unset webview to show progressbar*/
	/*webview will be set on "load,finished" callback or after setting search highlight*/
	if (ug_data->webview_ly && ug_data->webview) {
		elm_object_part_content_unset(ug_data->webview_ly, "elm.swallow.content");
		evas_object_hide(ug_data->webview);
	}

	/*create list process animation*/
	debug_log("Creating list progress");
	ug_data->list_progressbar = elm_progressbar_add(ug_data->base.content);
	elm_object_style_set(ug_data->list_progressbar, "process_large");
	evas_object_size_hint_align_set(ug_data->list_progressbar, EVAS_HINT_FILL, 0.5);
	evas_object_size_hint_weight_set(ug_data->list_progressbar, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(ug_data->list_progressbar);
	elm_progressbar_pulse(ug_data->list_progressbar, EINA_TRUE);
	elm_object_part_content_set(ug_data->base.content, "ev.swallow.progress", ug_data->list_progressbar);
	debug_leave();
}

void _create_body(EmailViewerUGD *ug_data)
{
	debug_enter();
	retm_if(ug_data == NULL, "Invalid parameter: ug_data[NULL]");

	/* create webview */
	if (ug_data->webview == NULL) {
		viewer_get_webview(ug_data);
	}

	viewer_set_webview_content(ug_data);
	viewer_set_mail_seen(ug_data);

	debug_leave();
}

void viewer_stop_ecore_running_apis(EmailViewerUGD *ug_data)
{
	debug_enter();
	retm_if(ug_data == NULL, "Invalid parameter: ug_data[NULL]");

	DELETE_TIMER_OBJECT(ug_data->launch_timer);
	DELETE_TIMER_OBJECT(ug_data->rcpt_scroll_corr);
	DELETE_IDLER_OBJECT(ug_data->idler_regionbringin);
	DELETE_IDLER_OBJECT(ug_data->to_recipient_idler);
	DELETE_IDLER_OBJECT(ug_data->cc_recipient_idler);
	DELETE_IDLER_OBJECT(ug_data->bcc_recipient_idler);
}

void viewer_delete_evas_objects(EmailViewerUGD *ug_data, Eina_Bool isHide)
{
	debug_enter();
	retm_if(ug_data == NULL, "Invalid parameter: ug_data[NULL]");

	if (ug_data->base.module->views_count > 1) {
		email_module_exit_view(&ug_data->attachment_view);
	}

	FREE(ug_data->eml_file_path);

	/* folder list */
	if (NULL != ug_data->folder_list) {
		int i = 0;
		LIST_ITER_START(i, ug_data->folder_list) {
			email_mailbox_name_and_alias_t *nameandalias = (email_mailbox_name_and_alias_t *)LIST_ITER_GET_DATA(i, ug_data->folder_list);
			FREE(nameandalias->name);
			FREE(nameandalias->alias);
			FREE(nameandalias);
		}
		g_list_free(ug_data->folder_list);
		ug_data->folder_list = NULL;
	}

	/* mailbox list */
	if (ug_data->move_mailbox_list) {
		email_free_mailbox(&ug_data->move_mailbox_list, ug_data->move_mailbox_count);
		ug_data->move_mailbox_list = NULL;
		ug_data->move_mailbox_count = 0;
	}

	/* Evas Object */
	DELETE_EVAS_OBJECT(ug_data->passwd_popup);
	DELETE_EVAS_OBJECT(ug_data->con_popup);
	DELETE_EVAS_OBJECT(ug_data->pb_notify);
	DELETE_EVAS_OBJECT(ug_data->notify);

	if (isHide) {
		DELETE_EVAS_OBJECT(ug_data->combined_scroller);
		DELETE_EVAS_OBJECT(ug_data->subject_ly);
		DELETE_EVAS_OBJECT(ug_data->en_subject);
		DELETE_EVAS_OBJECT(ug_data->sender_ly);
		DELETE_EVAS_OBJECT(ug_data->to_ly);
		DELETE_EVAS_OBJECT(ug_data->cc_ly);
		DELETE_EVAS_OBJECT(ug_data->bcc_ly);
		DELETE_EVAS_OBJECT(ug_data->header_divider);
		DELETE_EVAS_OBJECT(ug_data->priority_sender_image);
		DELETE_EVAS_OBJECT(ug_data->details_button);
		DELETE_EVAS_OBJECT(ug_data->attachment_ly);
		DELETE_EVAS_OBJECT(ug_data->dn_btn);
		DELETE_EVAS_OBJECT(ug_data->webview_ly);
		DELETE_EVAS_OBJECT(ug_data->webview_button);
		DELETE_EVAS_OBJECT(ug_data->list_progressbar);
	}

	FREE(ug_data->selected_address);
	FREE(ug_data->selected_name);
	email_contacts_delete_contact_info(&ug_data->recipient_contact_list_item);

	debug_leave();
}

static void _viewer_on_back_key(email_view_t *self)
{
	debug_enter();
	retm_if(self == NULL, "Invalid parameter: self is NULL");

	EmailViewerUGD *ug_data = (EmailViewerUGD *)self;
	viewer_back_key_press_handler(ug_data);

	debug_leave();
}

void _reply_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	DELETE_EVAS_OBJECT(ug_data->con_popup);
	DELETE_EVAS_OBJECT(ug_data->notify);

	if (ug_data->viewer_type == EML_VIEWER) {
		viewer_launch_composer(ug_data, RUN_EML_REPLY);
	} else {
		bool is_body_fully_downloaded = viewer_check_body_download_status(ug_data->body_download_status, EMAIL_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED);

		email_attachment_data_t *attach_data = ug_data->attachment_info;
		int attach_count = ug_data->attachment_count;

		int i = 0;
		bool is_inline_fully_downloaded = false;
		for (i = 0; i < attach_count; i++) {
			if (attach_data[i].inline_content_status && attach_data[i].save_status) {
				is_inline_fully_downloaded = true;
			}
		}

		debug_log("is_body_fully_downloaded [%d], account_type [%d], has_inline_image [%d], is_inline_fully_downloaded [%d]", is_body_fully_downloaded, ug_data->account_type, ug_data->has_inline_image, is_inline_fully_downloaded);
		if (!is_body_fully_downloaded && !(ug_data->has_inline_image && is_inline_fully_downloaded)) {
			util_create_notify(ug_data, EMAIL_VIEWER_HEADER_DOWNLOAD_ENTIRE_EMAIL, EMAIL_VIEWER_POP_PARTIAL_BODY_DOWNLOADED_REST_LOST, 2,
					EMAIL_VIEWER_STRING_CANCEL, _popup_response_cb,
					EMAIL_VIEWER_BUTTON_CONTINUE, _popup_response_continue_reply_cb, NULL);
		} else {
			viewer_launch_composer(ug_data, RUN_COMPOSER_REPLY);
		}
	}
	debug_leave();
}

void _reply_all_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	DELETE_EVAS_OBJECT(ug_data->con_popup);
	DELETE_EVAS_OBJECT(ug_data->notify);

	if (ug_data->viewer_type == EML_VIEWER) {
		viewer_launch_composer(ug_data, RUN_EML_REPLY_ALL);
	} else {
		bool is_body_fully_downloaded = viewer_check_body_download_status(ug_data->body_download_status, EMAIL_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED);

		email_attachment_data_t *attach_data = ug_data->attachment_info;
		int attach_count = ug_data->attachment_count;

		int i = 0;
		bool is_inline_fully_downloaded = false;
		for (i = 0; i < attach_count; i++) {
			if (attach_data[i].inline_content_status && attach_data[i].save_status) {
				is_inline_fully_downloaded = true;
			}
		}

		debug_log("is_body_fully_downloaded [%d], account_type [%d], has_inline_image [%d], is_inline_fully_downloaded [%d]", is_body_fully_downloaded, ug_data->account_type, ug_data->has_inline_image, is_inline_fully_downloaded);
		if (!is_body_fully_downloaded && !(ug_data->has_inline_image && is_inline_fully_downloaded)) {
			util_create_notify(ug_data, EMAIL_VIEWER_HEADER_DOWNLOAD_ENTIRE_EMAIL, EMAIL_VIEWER_POP_PARTIAL_BODY_DOWNLOADED_REST_LOST, 2,
					EMAIL_VIEWER_STRING_CANCEL, _popup_response_cb,
					EMAIL_VIEWER_BUTTON_CONTINUE, _popup_response_continue_reply_all_cb, NULL);
		} else {
			viewer_launch_composer(ug_data, RUN_COMPOSER_REPLY_ALL);
		}
	}
	debug_leave();
}

void _forward_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	DELETE_EVAS_OBJECT(ug_data->con_popup);
	DELETE_EVAS_OBJECT(ug_data->notify);

	if (ug_data->viewer_type == EML_VIEWER) {
		viewer_launch_composer(ug_data, RUN_EML_FORWARD);
	} else {
		bool is_body_fully_downloaded = viewer_check_body_download_status(ug_data->body_download_status, EMAIL_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED);

		email_attachment_data_t *attach_data = ug_data->attachment_info;
		int attach_count = ug_data->attachment_count;

		int i = 0;
		bool is_inline_fully_downloaded = false;
		for (i = 0; i < attach_count; i++) {
			if (attach_data[i].inline_content_status && attach_data[i].save_status) {
				is_inline_fully_downloaded = true;
			}
		}

		debug_log("is_body_fully_downloaded [%d], account_type [%d], has_inline_image [%d], is_inline_fully_downloaded [%d]", is_body_fully_downloaded, ug_data->account_type, ug_data->has_inline_image, is_inline_fully_downloaded);
		if (!is_body_fully_downloaded && !(ug_data->has_inline_image && is_inline_fully_downloaded)) {
			util_create_notify(ug_data, EMAIL_VIEWER_HEADER_DOWNLOAD_ENTIRE_EMAIL, EMAIL_VIEWER_POP_PARTIAL_BODY_DOWNLOADED_REST_LOST, 2,
					EMAIL_VIEWER_STRING_CANCEL, _popup_response_cb,
					EMAIL_VIEWER_BUTTON_CONTINUE, _popup_response_continue_forward_cb, NULL);
		} else {
			viewer_launch_composer(ug_data, RUN_COMPOSER_FORWARD);
		}
	}
	debug_leave();
}

void _delete_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	DELETE_EVAS_OBJECT(ug_data->con_popup);

	email_string_t title = { PACKAGE, "IDS_EMAIL_HEADER_DELETE" };
	email_string_t content = { PACKAGE, "IDS_EMAIL_POP_THIS_EMAIL_WILL_BE_DELETED" };
	email_string_t btn2 = { PACKAGE, "IDS_EMAIL_BUTTON_DELETE_ABB4" };
	util_create_notify(ug_data, title, content, 2,
			EMAIL_VIEWER_STRING_CANCEL, _popup_response_cb,
			btn2, _popup_response_delete_ok_cb, NULL);

	debug_leave();
}

void _move_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	DELETE_EVAS_OBJECT(ug_data->con_popup);

	int mail_list_length = 1;
	char **selected_mail_list = (char **)malloc(mail_list_length * sizeof(char *));
	retm_if(!selected_mail_list, "selected_mail_list is NULL!");

	const int BUF_LEN = 30;
	selected_mail_list[0] = (char *)malloc(BUF_LEN * sizeof(char));
	if (!selected_mail_list[0]) {
		debug_error("selected_mail_list[0] is NULL - allocation memory failed");
		free(selected_mail_list);
		return;
	}
	snprintf(selected_mail_list[0], BUF_LEN, "%d", ug_data->mail_id);

	app_control_h service;
	if (APP_CONTROL_ERROR_NONE != app_control_create(&service)) {
		debug_log("creating service handle failed");
		free(selected_mail_list[0]);
		free(selected_mail_list);
		return;
	}

	char acctid[30] = { 0, };
	snprintf(acctid, sizeof(acctid), "%d", ug_data->account_id);
	char mailboxid[30] = { 0, };
	snprintf(mailboxid, sizeof(mailboxid), "%d", ug_data->mailbox_id);
	char move_ug[30] = { 0, };
	snprintf(move_ug, sizeof(move_ug), "%d", 1);
	char move_mode[30] = { 0, };
	snprintf(move_mode, sizeof(move_mode), "%d", 0);
	debug_log("setting move mode %s", move_mode);
	char mailbox_edit_mode[30] = { 0, };
	snprintf(mailbox_edit_mode, sizeof(mailbox_edit_mode), "%d", 0);
	char move_src_mailbox_id[30] = { 0, };
	snprintf(move_src_mailbox_id, sizeof(move_src_mailbox_id), "%d", ug_data->mailbox_id);
	app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_ACCOUNT_ID, acctid);
	app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_MAILBOX, mailboxid);
	app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_IS_MAILBOX_MOVE_UG, move_ug);
	app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_MAILBOX_MOVE_MODE, move_mode);
	app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_IS_MAILBOX_EDIT_MODE, mailbox_edit_mode);
	app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_MOVE_SRC_MAILBOX_ID, move_src_mailbox_id);
	app_control_add_extra_data_array(service, EMAIL_BUNDLE_KEY_ARRAY_SELECTED_MAIL_IDS, (const char **)selected_mail_list, mail_list_length);
	debug_log("account_id: %s, mailbox_id: %s, mail_id: %s", acctid, mailboxid, selected_mail_list[0]);

	ug_data->loaded_module = viewer_create_module(ug_data, EMAIL_MODULE_ACCOUNT, service, false);
	app_control_destroy(service);

	free(selected_mail_list[0]);
	free(selected_mail_list);
	debug_leave();
}

static void _more_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	viewer_create_more_ctxpopup(ug_data);
	debug_leave();
}

void _download_body(void *data)
{
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	int handle;
	gboolean ret = 0;
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	if (viewer_check_storage_full(MIN_FREE_SPACE)) {
		viewer_show_storage_full_popup(ug_data);
		return;
	}
	ret = email_engine_body_download(ug_data->account_id, ug_data->mail_id, FALSE, &handle);

	if (ret == TRUE) {
		ug_data->email_handle = handle;
		debug_log("mail_id(%d) account_id(%d) handle(%d)",
				ug_data->mail_id, ug_data->account_id, ug_data->email_handle);
		viewer_create_down_progress(ug_data, EMAIL_VIEWER_POPUP_DOWNLOADING, viewer_destroy_down_progress_cb);
	} else {
		debug_log("unable to download mail body");
		notification_status_message_post(_("IDS_EMAIL_POP_DOWNLOAD_FAILED"));
	}
}

static void _download_msg_btn_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;
	retm_if(ug_data->is_download_message_btn_clicked, "Already clicked!!");

	/* check already downloaded */
	int body_download_status = ug_data->body_download_status;

	if (viewer_check_body_download_status(body_download_status, EMAIL_BODY_DOWNLOAD_STATUS_NONE)) {
		debug_log("only header downloaded -> need body download");
	} else if (viewer_check_body_download_status(body_download_status, EMAIL_BODY_DOWNLOAD_STATUS_PARTIALLY_DOWNLOADED)) {
		debug_log("body downloaded partially -> need full body download");
	}

	_download_body(ug_data);

	debug_leave();
}

static void _popup_response_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	DELETE_EVAS_OBJECT(ug_data->notify);
	debug_leave();
}

static void _popup_response_delete_ok_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;
	int ret = 0;

	DELETE_EVAS_OBJECT(ug_data->notify);

	if (ug_data->mailbox_type == EMAIL_MAILBOX_TYPE_TRASH || ug_data->save_status == EMAIL_MAIL_STATUS_SEND_SCHEDULED) {
		/*cancel scheduled email before deleting the mail*/
		if (ug_data->save_status == EMAIL_MAIL_STATUS_SEND_SCHEDULED) {
			int err = email_cancel_sending_mail(ug_data->mail_id);
			if (err != EMAIL_ERROR_NONE)
				debug_log("email_cancel_sending_mail() failed: [%d]", err);
		}
		ret = viewer_delete_email(ug_data);
	} else if (ug_data->viewer_type == EML_VIEWER) {
		if (remove(ug_data->eml_file_path)) {
			debug_log("failed to delete eml file");
		} else {
			debug_log("succeed to delete eml file");
			ret = 1;
		}
	} else {
		ret = viewer_move_email(ug_data, _find_folder_id_using_folder_type(ug_data, EMAIL_MAILBOX_TYPE_TRASH), TRUE, FALSE);
	}

	if (ret == 1) {
		debug_log("ug_destroy_me");
		email_module_make_destroy_request(ug_data->base.module);
	}
	debug_leave();
}

static void _popup_response_continue_reply_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	DELETE_EVAS_OBJECT(ug_data->notify);

	viewer_launch_composer(ug_data, RUN_COMPOSER_REPLY);
	debug_leave();
}

static void _popup_response_continue_reply_all_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	DELETE_EVAS_OBJECT(ug_data->notify);

	viewer_launch_composer(ug_data, RUN_COMPOSER_REPLY_ALL);
	debug_leave();
}

static void _popup_response_continue_forward_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	DELETE_EVAS_OBJECT(ug_data->notify);

	viewer_launch_composer(ug_data, RUN_COMPOSER_FORWARD);
	debug_leave();
}

static void _popup_response_to_destroy_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	DELETE_EVAS_OBJECT(ug_data->notify);
	debug_log("email_module_make_destroy_request");
	email_module_make_destroy_request(ug_data->base.module);
	debug_leave();
}

/* Double_Scroller */
static void _outter_scroller_bottom_hit_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	viewer_stop_elm_scroller_start_webkit_scroller(ug_data);

	debug_leave();
}

static void _outter_scroller_top_hit_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;
	ug_data->is_outer_scroll_bot_hit = EINA_FALSE;
	edje_object_part_drag_value_set(_EDJ(ug_data->combined_scroller), "elm.dragable.vbar", 0.0, 0.0);
	debug_leave();
}

void viewer_remove_callback(EmailViewerUGD *ug_data)
{
	email_unregister_timezone_changed_callback(_timezone_changed_cb);
	viewer_close_pattern_generator();
}

int viewer_reset_mail_data(EmailViewerUGD *ug_data)
{
	debug_enter();
	retvm_if(ug_data == NULL, VIEWER_ERROR_INVALID_ARG, "Invalid parameter: ug_data[NULL]");

	/* Create Email Viewer Private variables */
	if (ug_data->mail_info) {
		debug_log("mail_info freed");
		email_free_mail_data(&(ug_data->mail_info), 1);
		ug_data->mail_info = NULL;
	}

	if (ug_data->attachment_info && ug_data->attachment_count > 0) {
		debug_log("attachment_info freed p[%p] n[%d]", ug_data->attachment_info, ug_data->attachment_count);
		email_free_attachment_data(&(ug_data->attachment_info), ug_data->attachment_count);
		ug_data->attachment_info = NULL;
		ug_data->attachment_count = 0;
	}

	/* Get mail_info from service, and fill property */
	int ret = viewer_set_internal_data(ug_data, EINA_TRUE);
	if (ret != VIEWER_ERROR_NONE) {
		debug_log("viewer_set_internal_data() failed! (error type: %d)", ret);
		return ret;
	}

	viewer_get_address_info_count(ug_data->mail_info, &ug_data->to_recipients_cnt, &ug_data->cc_recipients_cnt, &ug_data->bcc_recipients_cnt);

	debug_leave();
	return VIEWER_ERROR_NONE;
}

static void _create_webview_layout(void *data)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;
	if (ug_data->webview_ly == NULL) {
		Evas_Object *ewk_view_ly = _create_fullview(ug_data->base.content);
		ug_data->webview_ly = ewk_view_ly;

		Evas_Object *ewk_btn = elm_button_add(ug_data->base.content);
		elm_object_style_set(ewk_btn, "transparent");
		evas_object_size_hint_weight_set(ewk_btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(ewk_btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
		elm_object_part_content_set(ug_data->base.content, "ev.swallow.webview_button", ewk_btn);
		evas_object_show(ewk_btn);
		ug_data->webview_button = ewk_btn;
	} else {
		evas_object_show(ug_data->webview_ly);
		evas_object_show(ug_data->webview_button);
	}
	elm_object_part_content_set(ug_data->base_ly, "ev.swallow.webview", ug_data->webview_ly);

	debug_leave();
}

Eina_Bool viewer_check_body_download(void *data)
{
	debug_enter();
	retvm_if(data == NULL, EINA_FALSE, "Invalid parameter: data[NULL]");

	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;
	_create_webview_layout(ug_data);
	_create_body(ug_data);

	if (viewer_check_body_download_status(ug_data->body_download_status, EMAIL_BODY_DOWNLOAD_STATUS_PARTIALLY_DOWNLOADED)) {
		ug_data->b_partial_body = 1;
	} else {
		ug_data->b_partial_body = 0;
	}

	if (!viewer_check_body_download_status(ug_data->body_download_status, EMAIL_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED)) {
		viewer_create_body_button(ug_data, "IDS_EMAIL_BUTTON_DOWNLOAD_FULL_EMAIL_ABB", _download_msg_btn_clicked_cb);
	}
	debug_leave();
	return EINA_FALSE;
}

void viewer_create_account_data(EmailViewerUGD *ug_data)
{
	debug_enter();
	retm_if(ug_data == NULL, "Invalid parameter: ug_data[NULL]");

	email_account_t *account = NULL;
	if (email_engine_get_account_full_data(ug_data->account_id, &account)) {
		if (account) {
			ug_data->account_type = account->incoming_server_type;
			FREE(ug_data->account_email_address);
			ug_data->account_email_address = strdup(account->user_email_address);
			account_user_data_t *ud = (account_user_data_t *)account->user_data;
			if (ud != NULL) {
				debug_log("show_images is %d", ud->show_images);
				ug_data->b_show_remote_images = ud->show_images;
			}
		}
		viewer_delete_account_data(ug_data);
		ug_data->account = account;
	}

	debug_leave();
}

void viewer_delete_account_data(EmailViewerUGD *ug_data)
{
	debug_enter();
	retm_if(ug_data == NULL, "Invalid parameter: ug_data[NULL]");

	if (ug_data->account) {
		email_free_account(&ug_data->account, 1);
		ug_data->account = NULL;
	}

	debug_leave();
}

static void _viewer_initialize_theme(EmailViewerUGD *ug_data)
{
	debug_enter();
	retm_if(!ug_data, "Invalid parameter, ug_data is NULL");

	ug_data->theme = elm_theme_new();
	elm_theme_ref_set(ug_data->theme, NULL);
	elm_theme_extension_add(ug_data->theme, email_get_viewer_theme_path());

	debug_leave();

}

static void _viewer_deinitialize_theme(EmailViewerUGD *ug_data)
{
	debug_enter();
	retm_if(!ug_data, "Invalid parameter, ug_data is NULL");

	elm_theme_extension_del(ug_data->theme, email_get_viewer_theme_path());
	elm_theme_free(ug_data->theme);
	ug_data->theme = NULL;

	debug_leave();

}

static void _destroy_viewer(EmailViewerUGD *ug_data)
{
	debug_enter();
	retm_if(ug_data == NULL, "Invalid parameter: ug_data[NULL]");

	ug_data->b_viewer_hided = TRUE;
	viewer_webkit_del_callbacks(ug_data);
	viewer_delete_account_data(ug_data);
	viewer_stop_ecore_running_apis(ug_data);
	recipient_delete_callbacks(ug_data);
	viewer_delete_evas_objects(ug_data, EINA_FALSE);
	viewer_remove_hard_link_for_inline_images(ug_data);
	viewer_remove_temp_files(ug_data);
	viewer_remove_temp_folders(ug_data);
	viewer_free_viewer_data(ug_data);
	viewer_remove_callback(ug_data);
	noti_mgr_dbus_receiver_remove(ug_data);

	email_engine_finalize();
	int ct_ret = email_contacts_finalize_service();
	debug_warning_if(ct_ret != CONTACTS_ERROR_NONE, "email_contacts_finalize_service() failed! ct_ret:[%d]", ct_ret);
	_viewer_deinitialize_theme(ug_data);
	_g_md = NULL;

	debug_leave();
}

/* EOF */
