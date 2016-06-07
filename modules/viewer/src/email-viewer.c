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

/* Header Include */
#include <utils_i18n.h>
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
#include "email-viewer-reply-toolbar.h"
#include "email-viewer-recipient.h"
#include "email-viewer-initial-data.h"
#include "email-viewer-eml.h"
#include "email-viewer-scroller.h"
#include "email-viewer-js.h"
#include "email-viewer-ext-gesture.h"
#include "email-viewer-more-menu-callback.h"
#include "email-viewer-noti-mgr.h"


/* Global Val */
EmailViewerView *_g_md = NULL; /* TODO Need more investigation why we need this global variable. */

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
static email_string_t EMAIL_VIEWER_STRING_NULL = { NULL, NULL };

/* module */
static int _viewer_module_create(email_module_t *self, email_params_h params);
static void _viewer_module_on_message(email_module_t *self, email_params_h msg);
static void _viewer_module_on_event(email_module_t *self, email_module_event_e event);

/* view */
static int _viewer_create(email_view_t *self);
static void _viewer_destroy(email_view_t *self);
static void _viewer_activate(email_view_t *self, email_view_state prev_state);
static void _viewer_update(email_view_t *self, int flags);
static void _viewer_on_back_key(email_view_t *self);

/* viewer theme */
static void _viewer_initialize_theme(EmailViewerView *view);
static void _viewer_deinitialize_theme(EmailViewerView *view);

/*event update functions*/
static void _viewer_update_on_orientation_change(EmailViewerView *view, bool isLandscape);
static void _viewer_update_on_region_change(EmailViewerView *view);
static void _viewer_update_on_language_change(EmailViewerView *view);

static void __viewer_add_main_callbacks(EmailViewerView *view, void *data);
static void _viewer_add_main_callbacks(EmailViewerView *view);
static void _viewer_update_data_on_start(void *data);

/* View related */
static VIEWER_ERROR_TYPE_E _viewer_create_base_view(EmailViewerView *view);
static void _viewer_get_window_sizes(EmailViewerView *view);
static void _create_webview_layout(void *data);

static void _timezone_changed_cb(system_settings_key_e key, void *data);

static void _viewer_set_webview_size(EmailViewerView *view, int height);

/* callback functions */
static void _more_cb(void *data, Evas_Object *obj, void *event_info);
static void _download_msg_btn_clicked_cb(void *data, Evas_Object *obj, void *event_info);

static void _popup_response_cb(void *data, Evas_Object *obj, void *event_info);
static void _popup_response_delete_ok_cb(void *data, Evas_Object *obj, void *event_info);
static void _popup_response_to_destroy_cb(void *data, Evas_Object *obj, void *event_info);

static void _outter_scroller_bottom_hit_cb(void *data, Evas_Object *obj, void *event_info);
static void _outter_scroller_top_hit_cb(void *data, Evas_Object *obj, void *event_info);

static Eina_Bool _viewer_launch_email_application_cb(void *data);

static int _construct_viewer_data(EmailViewerView *view);
static void _destroy_viewer(EmailViewerView *view);

#ifdef SHARED_MODULES_FEATURE
EMAIL_API email_module_t *email_module_alloc()
#else
email_module_t *viewer_module_alloc()
#endif
{
	debug_enter();

	EmailViewerModule *module = MEM_ALLOC(module, 1);
	if (!module) {
		debug_error("module is NULL");
		return NULL;
	}

	_g_md = &module->view;

	module->base.create = _viewer_module_create;
	module->base.on_message = _viewer_module_on_message;
	module->base.on_event = _viewer_module_on_event;

	return &module->base;
}

static void _viewer_set_webview_size(EmailViewerView *view, int height)
{
	debug_enter();

	retm_if(view->loaded_module, "Viewer module in background");
	int min_height = 0, max_height = 0;

	int win_h = 0, win_w = 0;
	elm_win_screen_size_get(view->base.module->win, NULL, NULL, &win_w, &win_h);
	debug_log("==> main layout [w, h] = [%d, %d]", win_w, win_h);
	debug_log("view->header_height : %d", view->header_height);

	if (view->isLandscape) {
		min_height = win_w - view->header_height;
		max_height = win_w;
		view->webview_width = win_h;
	} else {
		min_height = win_h - view->header_height;
		max_height = win_h;
		view->webview_width = win_w;
	}

	if (height <= min_height) {
		debug_log("min");
		view->webview_height = min_height;
	} else {
		debug_log("max");
		view->webview_height = max_height;
	}

	debug_log("min_height: %d height :%d max_height :%d ", min_height, height, max_height);
	debug_log("view->webview_height: %d", view->webview_height);

	debug_leave();
}

void viewer_resize_webview(EmailViewerView *view, int height)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");
	_viewer_set_webview_size(view, height);

	evas_object_size_hint_min_set(view->webview, view->webview_width, view->webview_height);
	evas_object_size_hint_max_set(view->webview, view->webview_width, view->webview_height);
}

void viewer_back_key_press_handler(EmailViewerView *view)
{
	debug_enter();

	if (view->b_load_finished == EINA_FALSE) {
		debug_log("b_load_finished is not yet done");
		return;
	}

	if (view->webview) {
		if (ewk_view_text_selection_clear(view->webview)) {
			debug_log("ewk_view_text_selection_clear() failed");
			return;
		}
	}
	debug_log("email_module_make_destroy_request");
	view->need_pending_destroy = EINA_TRUE;
	evas_object_smart_callback_del(view->more_btn, "clicked", _more_cb);
	email_module_make_destroy_request(view->base.module);
	debug_leave();
}

static void __viewer_add_main_callbacks(EmailViewerView *view, void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");
	retm_if(!view, "Invalid parameter: view is NULL!");

	/* indicator */
	elm_win_indicator_mode_set(view->base.module->win, ELM_WIN_INDICATOR_SHOW);
	elm_win_indicator_opacity_set(view->base.module->win, ELM_WIN_INDICATOR_OPAQUE);

	viewer_open_pattern_generator();
	if (email_register_timezone_changed_callback(_timezone_changed_cb, data) == -1) {
		debug_log("Failed to register system settings callback for time changed");
		view->eViewerErrorType = VIEWER_ERROR_FAIL;
	}

	debug_leave();
}

static void _viewer_add_main_callbacks(EmailViewerView *view)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	__viewer_add_main_callbacks(view, (void *)view);

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

int _viewer_module_create(email_module_t *self, email_params_h params)
{
	debug_enter();
	retvm_if(!self, -1, "Invalid parameter: self is NULL!");

	EmailViewerModule *module = (EmailViewerModule *)self;
	EmailViewerView *view = &module->view;

	viewer_initialize_data(view);

	if (!params) {
		debug_log("params handle is NULL");
		view->eViewerErrorType = VIEWER_ERROR_INVALID_ARG;
	}

	int ret = viewer_initial_data_pre_parse_arguments(view, params);
	if (ret != VIEWER_ERROR_NONE) {
		debug_log("viewer_initial_data_pre_parse_arguments() failed! (error type: %d)", ret);
		view->eViewerErrorType = ret;
	}

	view->base.create = _viewer_create;
	view->base.destroy = _viewer_destroy;
	view->base.activate = _viewer_activate;
	view->base.update = _viewer_update;
	view->base.on_back_key = _viewer_on_back_key;

	ret = email_module_create_view(&module->base, &view->base);
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

	EmailViewerView *view = (EmailViewerView *)self;

	int ret = _construct_viewer_data(view);
	if (ret != VIEWER_ERROR_NONE) {
		debug_log("_construct_viewer_data() failed! (error type: %d)", ret);
		view->eViewerErrorType = ret;
	}

	ret = _initialize_services(view);
	if (ret != VIEWER_ERROR_NONE) {
		view->eViewerErrorType = ret;
	}

	view->create_contact_arg = CONTACTUI_REQ_ADD_EMAIL;
	_viewer_initialize_theme(view);
	_viewer_create_base_view(view);
	_create_webview_layout(view);
	viewer_get_webview(view);
	header_create_view(view);
	header_update_view(view);
	reply_toolbar_create_view(view);

	create_body_progress(view);
	view->b_viewer_hided = FALSE;
	_viewer_add_main_callbacks(view);

	debug_leave();
	return 0;
}

static VIEWER_ERROR_TYPE_E _viewer_process_error_popup(EmailViewerView *view)
{
	debug_enter();

	debug_log("eViewerErrorType:%d", view->eViewerErrorType);
	if (view->eViewerErrorType != VIEWER_ERROR_NONE) {
		switch (view->eViewerErrorType) {
		case VIEWER_ERROR_NOT_ALLOWED:
			debug_warning("Lauching viewer failed! VIEWER_ERROR_NOT_ALLOWED");
			util_create_notify(view, EMAIL_VIEWER_HEADER_UNAVAILABLE,
					EMAIL_VIEWER_POP_SECURITY_POLICY_RESTRICTS_USE_OF_POP_IMAP_EMAIL, 1,
					EMAIL_VIEWER_STRING_OK, _popup_response_to_destroy_cb, EMAIL_VIEWER_STRING_NULL, NULL, NULL);
			break;

		case VIEWER_ERROR_NO_DEFAULT_ACCOUNT:
		case VIEWER_ERROR_NO_ACCOUNT_LIST:
		case VIEWER_ERROR_NO_ACCOUNT:
			debug_warning("Lauching viewer failed! Failed to get account! [%d]", view->eViewerErrorType);
			util_create_notify(view, EMAIL_VIEWER_HEADER_UNABLE_TO_PERFORM_ACTION,
					EMAIL_VIEWER_POP_THERE_IS_NO_ACCOUNT_CREATE_A_NEW_ACCOUNT_FIRST, 0,
					EMAIL_VIEWER_STRING_NULL, NULL, EMAIL_VIEWER_STRING_NULL, NULL, NULL);
			DELETE_TIMER_OBJECT(view->launch_timer);
			view->launch_timer = ecore_timer_add(2.0f, _viewer_launch_email_application_cb, view);
			break;

		case VIEWER_ERROR_GET_DATA_FAIL:
			debug_warning("Lauching viewer failed! VIEWER_ERROR_GET_DATA_FAIL");
			util_create_notify(view, EMAIL_VIEWER_HEADER_UNABLE_TO_PERFORM_ACTION,
						EMAIL_VIEWER_BODY_SELECTED_DATA_NOT_FOUND, 1,
						EMAIL_VIEWER_STRING_OK, _popup_response_to_destroy_cb, EMAIL_VIEWER_STRING_NULL, NULL, NULL);
			break;

		case VIEWER_ERROR_FAIL:
		case VIEWER_ERROR_UNKOWN_TYPE:
		case VIEWER_ERROR_INVALID_ARG:
			debug_warning("Lauching viewer failed! Due to [%d]", view->eViewerErrorType);
			util_create_notify(view, EMAIL_VIEWER_HEADER_UNABLE_TO_PERFORM_ACTION,
						EMAIL_VIEWER_UNKNOWN_ERROR_HAS_OCCURED, 1,
						EMAIL_VIEWER_STRING_OK, _popup_response_to_destroy_cb, EMAIL_VIEWER_STRING_NULL, NULL, NULL);

			break;

		case VIEWER_ERROR_ENGINE_INIT_FAILED:
		case VIEWER_ERROR_DBUS_FAIL:
		case VIEWER_ERROR_SERVICE_INIT_FAIL:
		case VIEWER_ERROR_OUT_OF_MEMORY:
			debug_warning("Lauching viewer failed! Due to [%d]", view->eViewerErrorType);
			util_create_notify(view, EMAIL_VIEWER_HEADER_UNABLE_TO_PERFORM_ACTION,
						EMAIL_VIEWER_POP_FAILED_TO_START_EMAIL_APPLICATION, 1,
						EMAIL_VIEWER_STRING_OK, _popup_response_to_destroy_cb, EMAIL_VIEWER_STRING_NULL, NULL, NULL);
			break;

		case VIEWER_ERROR_STORAGE_FULL:
			debug_warning("Lauching viewer failed! VIEWER_ERROR_STORAGE_FULL");
			email_string_t content = { PACKAGE, "IDS_EMAIL_POP_THERE_IS_NOT_ENOUGH_SPACE_IN_YOUR_DEVICE_STORAGE_GO_TO_SETTINGS_POWER_AND_STORAGE_STORAGE_THEN_DELETE_SOME_FILES_AND_TRY_AGAIN" };
			email_string_t btn2 = { PACKAGE, "IDS_EMAIL_BUTTON_SETTINGS" };
			util_create_notify(view, EMAIL_VIEWER_HEADER_UNABLE_TO_PERFORM_ACTION, content, 2,
						EMAIL_VIEWER_STRING_OK, _popup_response_to_destroy_cb,
						btn2, viewer_storage_full_popup_response_cb, NULL);
			break;

		default:
			break;
		}

		debug_leave();
		return view->eViewerErrorType;
	}

	debug_leave();
	return VIEWER_ERROR_NONE;
}

void _viewer_activate(email_view_t *self, email_view_state prev_state)
{
	debug_enter();
	retm_if(!self, "Invalid parameter: self is NULL!");

	EmailViewerView *view = (EmailViewerView *)self;

	if (prev_state != EV_STATE_CREATED) {
		return;
	}

	VIEWER_ERROR_TYPE_E error = _viewer_process_error_popup(view);
	if (error != VIEWER_ERROR_NONE) {
		return;
	}
	_viewer_update_data_on_start(view);

	debug_leave();
}

static void _viewer_update_data_on_start(void *data)
{
	debug_enter();

	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	viewer_check_body_download(view);

	debug_leave();
}

void _viewer_destroy(email_view_t *self)
{
	debug_enter();
	retm_if(!self, "Invalid parameter: self is NULL!");

	EmailViewerView *view = (EmailViewerView *)self;

	_destroy_viewer(view);

	debug_leave();
}

void _viewer_module_on_message(email_module_t *self, email_params_h msg)
{
	debug_enter();
	retm_if(!self, "Invalid parameter: self is NULL!");

	EmailViewerModule *module = (EmailViewerModule *)self;
	EmailViewerView *view = &module->view;

	bool wait_for_composer = false;

	const char *msg_type = NULL;
	email_params_get_str_opt(msg, EMAIL_BUNDLE_KEY_MSG, &msg_type);

	if (view->loaded_module && view->is_composer_module_launched) {
		email_params_h params = NULL;

		if (email_params_create(&params) &&
			email_params_add_str(params, EMAIL_BUNDLE_KEY_MSG, EMAIL_BUNDLE_VAL_EMAIL_COMPOSER_SAVE_DRAFT)) {

			email_module_send_message(view->loaded_module, params);
		}

		email_params_free(&params);
		wait_for_composer = true;
	}

	if (g_strcmp0(msg_type, EMAIL_BUNDLE_VAL_VIEWER_DESTROY_VIEW) == 0) {
		debug_log("Email viewer need to be closed");
		if (wait_for_composer) {
			view->need_pending_destroy = EINA_TRUE;
		} else {
			email_module_make_destroy_request(view->base.module);
		}
		return;
	}

	if (view->loaded_module && !view->is_composer_module_launched) {
		email_module_destroy(view->loaded_module);
		view->loaded_module = NULL;
	}

	if (g_strcmp0(msg_type, EMAIL_BUNDLE_VAL_VIEWER_RESTORE_VIEW) == 0) {
		debug_log("Restore Email viewer");
		_reset_view(view, EINA_TRUE);
		return;
	}

	debug_log("Hide previous mail data");
	_hide_view(view);

	_g_md = view;

	int ret = viewer_initial_data_pre_parse_arguments(view, msg);
	if (ret != VIEWER_ERROR_NONE) {
		debug_log("viewer_initial_data_pre_parse_arguments() failed! (error type: %d)", ret);
		view->eViewerErrorType = ret;
	}

	/* The viewer layout is not destroyed. Therefore, it runs with EINA_TRUE. */
	ret = _construct_viewer_data(view);
	if (ret != VIEWER_ERROR_NONE) {
		debug_log("_construct_viewer_data() failed! (error type: %d)", ret);
		view->eViewerErrorType = ret;
	}

	if (noti_mgr_dbus_receiver_setup(view) == EINA_FALSE) {
		debug_log("Failed to initialize dbus");
		view->eViewerErrorType = VIEWER_ERROR_DBUS_FAIL;
	}

	if (view->webview != NULL) {
		Ewk_Settings *ewk_settings = ewk_view_settings_get(view->webview);
		debug_log("b_show_remote_images is %d", view->b_show_remote_images);
		if (ewk_settings_loads_images_automatically_set(ewk_settings, view->b_show_remote_images) == EINA_FALSE) { /* ewk_settings_load_remote_images_set for remote image */
			debug_log("SET show images is FAILED!");
		}
	}

	edje_object_signal_emit(_EDJ(view->base.content), "hide_button", "elm.event.btn");

	int angle = elm_win_rotation_get(view->base.module->win);
	view->isLandscape = ((angle == APP_DEVICE_ORIENTATION_90) || (angle == APP_DEVICE_ORIENTATION_270)) ? true : false;
	debug_log("isLandscapte : %d", view->isLandscape);

	_reset_view(view, EINA_FALSE);

	create_body_progress(view);

	view->b_viewer_hided = FALSE;

	VIEWER_ERROR_TYPE_E error = _viewer_process_error_popup(view);
	if (error != VIEWER_ERROR_NONE) {
		return;
	}

	debug_leave();
}

static void _viewer_module_on_event(email_module_t *self, email_module_event_e event)
{
	debug_enter();
	retm_if(!self, "Invalid parameter: self is NULL!");

	EmailViewerModule *module = (EmailViewerModule *)self;
	EmailViewerView *view = &module->view;
	debug_log("event = %d", event);

	switch (event) {
	case EM_EVENT_LOW_MEMORY_SOFT:
		debug_log("Low memory: %d", event);
		viewer_webview_handle_mem_warning(view, false);
		break;
	case EM_EVENT_LOW_MEMORY_HARD:
		debug_log("Low memory: %d", event);
		viewer_webview_handle_mem_warning(view, true);
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

	EmailViewerView *view = (EmailViewerView *)self;
	debug_log("flags = %d", flags);

	if (flags & EVUF_LANGUAGE_CHANGED) {
		_viewer_update_on_language_change(view);
	}

	if (flags & EVUF_ORIENTATION_CHANGED) {
		switch (view->base.orientation) {
		case APP_DEVICE_ORIENTATION_0:
		case APP_DEVICE_ORIENTATION_180:
			if (view->webview) {
				ewk_view_orientation_send(view->webview,
						view->base.orientation == APP_DEVICE_ORIENTATION_0 ? 0 : 180);
			}
			if (view->isLandscape == true) {
				_viewer_update_on_orientation_change(view, false);
			}
			break;

		case APP_DEVICE_ORIENTATION_270:
		case APP_DEVICE_ORIENTATION_90:
			if (view->webview) {
				ewk_view_orientation_send(view->webview,
						view->base.orientation == APP_DEVICE_ORIENTATION_270 ? 90 : -90);
			}
			if (view->isLandscape == false) {
				_viewer_update_on_orientation_change(view, true);
			}
			break;
		}
	}

	if (flags & EVUF_REGION_FMT_CHANGED) {
		_viewer_update_on_region_change(view);
	}

	debug_leave();
}

static void _viewer_update_on_orientation_change(EmailViewerView *view, bool isLandscape)
{
	debug_enter();
	retm_if(!view, "Invalid parameter: view is NULL!");

	view->isLandscape = isLandscape;
	viewer_set_vertical_scroller_position(view);
	/* resize webview */
	double scale = ewk_view_scale_get(view->webview);
	int content_height = 0;
	ewk_view_contents_size_get(view->webview, NULL, &content_height);
	if (content_height) {
		viewer_resize_webview(view, content_height * scale);
	}

	debug_log("content_height :%d scale :%f", content_height, scale);

	debug_leave();
}

static void _viewer_update_on_region_change(EmailViewerView *view)
{
	debug_enter();
	retm_if(!view, "Invalid parameter: view is NULL!");

	header_update_date(view);

	debug_leave();
}

static void _viewer_update_on_language_change(EmailViewerView *view)
{
	debug_enter();
	retm_if(!view, "Invalid parameter: view is NULL!");

	header_update_subject_text(view);

	if (view->to_mbe) {
		elm_object_text_set(view->to_mbe, email_get_email_string("IDS_EMAIL_BODY_TO_M_RECIPIENT"));
	}
	if (view->cc_mbe) {
		elm_object_text_set(view->cc_mbe, email_get_email_string("IDS_EMAIL_BODY_CC"));
	}
	if (view->bcc_mbe) {
		elm_object_text_set(view->bcc_mbe, email_get_email_string("IDS_EMAIL_BODY_BCC"));
	}

	header_update_attachment_summary_info(view);

	/* This code is specific for popups where string translation is not handled by EFL */
	if (view->notify && view->popup_element_type) {
		char str[MAX_STR_LEN] = { 0, };
		char first_str[MAX_STR_LEN] = { 0, };
		switch (view->popup_element_type) {
		case POPUP_ELEMENT_TYPE_TITLE:
			break;
		case POPUP_ELEMENT_TYPE_CONTENT:
			if (view->extra_variable_type == VARIABLE_TYPE_INT) {
				if (view->str_format2) {
					if (view->package_type2 == PACKAGE_TYPE_SYS_STRING) {
						snprintf(str, sizeof(str), view->str_format2, email_get_system_string(view->translation_string_id2), view->extra_variable_integer);
					} else if (view->package_type2 == PACKAGE_TYPE_PACKAGE) {
						snprintf(str, sizeof(str), view->str_format2, email_get_email_string(view->translation_string_id2), view->extra_variable_integer);
					}
				} else {
					if (view->package_type2 == PACKAGE_TYPE_SYS_STRING) {
						snprintf(str, sizeof(str), email_get_system_string(view->translation_string_id2), view->extra_variable_integer);
					} else if (view->package_type2 == PACKAGE_TYPE_PACKAGE) {
						snprintf(str, sizeof(str), email_get_email_string(view->translation_string_id2), view->extra_variable_integer);
					}
				}
			} else if (view->extra_variable_type == VARIABLE_TYPE_STRING) {
				if (view->str_format2) {
					if (view->package_type2 == PACKAGE_TYPE_SYS_STRING) {
						snprintf(str, sizeof(str), view->str_format2, email_get_system_string(view->translation_string_id2), view->extra_variable_string);
					} else if (view->package_type2 == PACKAGE_TYPE_PACKAGE) {
						snprintf(str, sizeof(str), view->str_format2, email_get_email_string(view->translation_string_id2), view->extra_variable_string);
					}
				} else {
					if (view->package_type2 == PACKAGE_TYPE_SYS_STRING) {
						snprintf(str, sizeof(str), email_get_system_string(view->translation_string_id2), view->extra_variable_string);
					} else if (view->package_type2 == PACKAGE_TYPE_PACKAGE) {
						snprintf(str, sizeof(str), email_get_email_string(view->translation_string_id2), view->extra_variable_string);
					}
				}
			} else {
				if (view->package_type2 == PACKAGE_TYPE_SYS_STRING) {
					snprintf(str, sizeof(str), "%s", email_get_system_string(view->translation_string_id2));
				} else if (view->package_type2 == PACKAGE_TYPE_PACKAGE) {
					snprintf(str, sizeof(str), "%s", email_get_email_string(view->translation_string_id2));
				} else {
					snprintf(str, sizeof(str), "%s", view->translation_string_id2);
				}
			}
			if (view->str_format1) {
				if (view->package_type1 == PACKAGE_TYPE_SYS_STRING) {
					snprintf(first_str, sizeof(first_str), view->str_format1, email_get_system_string(view->translation_string_id1), str);
				} else if (view->package_type1 == PACKAGE_TYPE_PACKAGE) {
					snprintf(first_str, sizeof(first_str), view->str_format1, email_get_email_string(view->translation_string_id1), str);
				} else {
					snprintf(first_str, sizeof(first_str), view->str_format1, view->translation_string_id1, str);
				}
			} else {
				snprintf(first_str, sizeof(first_str), "%s", view->translation_string_id1);
			}
			elm_object_text_set(view->notify, first_str);
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
	EmailViewerView *view = (EmailViewerView *)data;

	app_control_h app_control = NULL;
	int ret = APP_CONTROL_ERROR_NONE;

	ret = app_control_create(&app_control);
	retvm_if((ret != APP_CONTROL_ERROR_NONE || !app_control), ECORE_CALLBACK_CANCEL, "app_control_create() failed! ret:[%d]", ret);

	ret = app_control_add_extra_data(app_control, EMAIL_BUNDLE_KEY_RUN_TYPE, "9"); /* RUN_SETTING_ACCOUNT_ADD */
	debug_warning_if(ret != APP_CONTROL_ERROR_NONE, "app_control_add_extra_data() failed! ret:[%d]", ret);

	ret = app_control_set_app_id(app_control, PKGNAME);
	debug_warning_if(ret != APP_CONTROL_ERROR_NONE, "app_control_set_app_id() failed! ret:[%d]", ret);

	/* Launch application */
	ret = app_control_send_launch_request(app_control, NULL, NULL);
	debug_warning_if(ret != APP_CONTROL_ERROR_NONE, "app_control_send_launch_request() failed! ret:[%d]", ret);

	ret = app_control_destroy(app_control);
	debug_warning_if(ret != APP_CONTROL_ERROR_NONE, "app_control_destroy() failed! ret:[%d]", ret);

	view->launch_timer = NULL;
	_popup_response_to_destroy_cb(view, view->notify, NULL);

	debug_leave();
	return ECORE_CALLBACK_CANCEL;
}

static int _construct_viewer_data(EmailViewerView *view)
{
	debug_enter();
	retvm_if(view == NULL, VIEWER_ERROR_FAIL, "Invalid parameter: view[NULL]");

	debug_log("pointer %p account_id [%d], mail_id [%d], mailbox_id [%d]", view, view->account_id, view->mail_id, view->mailbox_id);
	if (((view->account_id <= 0 || (view->mail_id <= 0) || (view->mailbox_id <= 0)) && (view->viewer_type != EML_VIEWER))) {
		debug_log("Required parameters are missing!");
		return VIEWER_ERROR_INVALID_ARG;
	}

	if (viewer_create_temp_folder(view) < 0) {
		debug_log("creating email viewer temp folder is failed");
		return VIEWER_ERROR_FAIL;
	}

	G_FREE(view->temp_viewer_html_file_path);
	view->temp_viewer_html_file_path = g_strdup_printf("%s/%s", view->temp_folder_path, EMAIL_VIEWER_TMP_HTML_FILE);

	int ret = viewer_reset_mail_data(view);
	if (ret != VIEWER_ERROR_NONE) {
		debug_log("viewer_reset_mail_data() failed! (error type: %d)", ret);
		return ret;
	}

	viewer_create_account_data(view);

	if (view->viewer_type == EMAIL_VIEWER) {
		retvm_if(view->mail_info == NULL, VIEWER_ERROR_GET_DATA_FAIL, "view->mail_info is NULL.");

		email_mail_data_t *mail_data = view->mail_info;
		view->mailbox_type = mail_data->mailbox_type;
		debug_log("mailbox_type:%d", view->mailbox_type);
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

static void _viewer_get_window_sizes(EmailViewerView *view)
{
	debug_enter();

	Evas_Coord width, height;
	evas_object_geometry_get(view->base.module->win, NULL, NULL, &width, &height);
	view->main_w = width;
	view->main_h = height;
	view->scale_factor = elm_app_base_scale_get() / elm_config_scale_get();

	debug_log("WINDOW W[%d] H[%d]", width, height);
	debug_log("ELM Scale[%f]", view->scale_factor);

}

static VIEWER_ERROR_TYPE_E _viewer_create_base_view(EmailViewerView *view)
{
	debug_enter();
	retvm_if(view == NULL, VIEWER_ERROR_FAIL, "Invalid parameter: view[NULL]");

	_viewer_get_window_sizes(view);

	/* add container layout */
	view->base.content = viewer_load_edj(view->base.module->navi, email_get_viewer_theme_path(), "ev/viewer/base", EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	if (view->theme) {
		elm_object_theme_set(view->base.content, view->theme);
	}
	retvm_if(view->base.content == NULL, VIEWER_ERROR_FAIL, "Cannot load edj!");
	evas_object_show(view->base.content);

	/* create scroller */
	view->scroller = _create_scroller(view->base.content);
	retvm_if(view->scroller == NULL, VIEWER_ERROR_FAIL, "scroller is NULL!");
	elm_object_part_content_set(view->base.content, "ev.swallow.content", view->scroller);
	evas_object_smart_callback_add(view->scroller, "edge,bottom", _outter_scroller_bottom_hit_cb, view);
	evas_object_smart_callback_add(view->scroller, "edge,top", _outter_scroller_top_hit_cb, view);
	evas_object_show(view->scroller);

	/* create flick detection layer */
	viewer_set_flick_layer(view);

	/* create base layout */
	view->base_ly = viewer_load_edj(view->scroller, email_get_viewer_theme_path(), "ev/layout/base", EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	retvm_if(view->base_ly == NULL, VIEWER_ERROR_FAIL, "Cannot load edj!");
	elm_object_content_set(view->scroller, view->base_ly);
	evas_object_show(view->base_ly);

	/* create main box */
	view->main_bx = _create_box(view->scroller);
	retvm_if(view->main_bx == NULL, VIEWER_ERROR_FAIL, "main_bx is NULL!");
	elm_object_part_content_set(view->base_ly, "ev.swallow.box", view->main_bx);
	evas_object_show(view->main_bx);

	/* push navigation bar */
	email_module_view_push(&view->base, NULL, 0);

	/* This button is set for devices which doesn't have H/W more key. */
	view->more_btn = elm_button_add(view->base.module->navi);
	elm_object_style_set(view->more_btn, "naviframe/more/default");
	evas_object_smart_callback_add(view->more_btn, "clicked", _more_cb, view);
	elm_object_item_part_content_set(view->base.navi_item, "toolbar_more_btn", view->more_btn);

	debug_leave();
	return VIEWER_ERROR_NONE;
}

void _reset_view(EmailViewerView *view, Eina_Bool update_body)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	if (view->base.module->views_count > 1) {
		email_module_exit_view(&view->attachment_view);
	}

	debug_log("@@@@@@@@@@ isLandscape = %d", view->isLandscape);
	if (view->isLandscape == true) {
		debug_log("In Split view - Hiding navi bar");
		/* Set scroller to start - height made 480 to consider viewer in split view */
		elm_scroller_region_show(view->scroller, 0, 0, (int)((double)view->main_h * 0.6), view->main_w);
	} else {
		debug_log("In Full view - Showing navi bar");
		/* Set scroller to start */
		elm_scroller_region_show(view->scroller, 0, 0, view->main_w, view->main_h);
	}

	if (elm_object_scroll_freeze_get(view->scroller) > 0)
		elm_object_scroll_freeze_pop(view->scroller);

	viewer_check_body_download(view);

	if (update_body) {
		header_update_view(view);
	} else {
		view->is_recipient_ly_shown = EINA_FALSE;
		header_layout_unpack(view);
		header_create_view(view);
		header_update_view(view);
		reply_toolbar_create_view(view);
	}

	debug_leave();
}

void _hide_view(EmailViewerView *view)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	view->b_viewer_hided = TRUE;

	viewer_webkit_del_callbacks(view);
	viewer_delete_account_data(view);
	viewer_stop_ecore_running_apis(view);
	viewer_delete_evas_objects(view, EINA_TRUE);
	viewer_remove_hard_link_for_inline_images(view);

	viewer_remove_temp_files(view);
	viewer_remove_temp_folders(view);
	viewer_free_viewer_data(view);
	noti_mgr_dbus_receiver_remove(view);

	viewer_initialize_data(view);
	_g_md = NULL;
	debug_leave();
}

/**
 * initialize viewer's private data
 */
Eina_Bool viewer_initialize_data(EmailViewerView *view)
{
	debug_enter();
	retvm_if(view == NULL, EINA_FALSE, "Invalid parameter: view[NULL]");

	view->loaded_module = NULL;

	view->main_w = 0;
	view->main_h = 0;
	view->eViewerErrorType = VIEWER_ERROR_NONE;

	/*dbus*/
	view->viewer_dbus_conn = NULL;
	view->viewer_network_id = 0;
	view->viewer_storage_id = 0;

	/*combined scroller*/
	view->combined_scroller = NULL;
	view->webkit_scroll_y = 0;
	view->webkit_scroll_x = 0;
	view->header_height = 0;
	view->trailer_height = 0;
	view->total_height = 0;
	view->total_width = 0;
	view->main_scroll_y = 0;
	view->main_scroll_h = 0;
	view->main_scroll_w = 0;

	/* arguments */
	view->account_id = 0;
	view->account_type = 0;
	view->mail_id = 0;
	view->mailbox_id = 0;
	view->account_email_address = NULL;
	view->temp_folder_path = NULL;
	view->temp_viewer_html_file_path = NULL;
	/* for preview */
	view->temp_preview_folder_path = NULL;

	/* flags */
	view->b_viewer_hided = 0;
	view->b_partial_body = 0;
	view->scroller_locked = 0;
	view->is_composer_module_launched = FALSE;

	view->can_destroy_on_msg_delete = false;
	view->b_load_finished = EINA_FALSE;
	view->b_show_remote_images = EINA_FALSE;
	view->is_long_pressed = EINA_FALSE;
	view->is_webview_scrolling = EINA_FALSE;
	view->is_main_scroller_scrolling = EINA_TRUE;
	view->is_outer_scroll_bot_hit = EINA_FALSE;
	view->is_magnifier_opened = EINA_FALSE;
	view->is_recipient_ly_shown = EINA_FALSE;
	view->is_download_message_btn_clicked = EINA_FALSE;
	view->is_cancel_sending_btn_clicked = EINA_FALSE;
	view->is_storage_full_popup_shown = EINA_FALSE;
	view->need_pending_destroy = EINA_FALSE;
	view->is_top_webview_reached = EINA_FALSE;
	view->is_bottom_webview_reached = EINA_FALSE;
	view->is_scrolling_down = EINA_FALSE;
	view->is_scrolling_up = EINA_FALSE;
	view->is_webview_text_selected = EINA_FALSE;

	/*eml viewer*/
	view->viewer_type = EMAIL_VIEWER;
	view->eml_file_path = NULL;

	/*popup translation*/
	view->popup_element_type = 0;
	view->package_type1 = 0;
	view->package_type2 = 0;
	view->translation_string_id1 = NULL;
	view->translation_string_id2 = NULL;
	view->extra_variable_type = 0;
	view->extra_variable_string = NULL;
	view->extra_variable_integer = 0;
	view->str_format1 = NULL;
	view->str_format2 = NULL;

	/* for module create */
	view->create_contact_arg = 0;

	/* Scalable UI */
	view->scale_factor = 0.0;
	view->webview_width = 0;
	view->webview_height = 0;

	/* rotation */
	view->isLandscape = 0;

	/* mailbox list */
	view->mailbox_type = EMAIL_MAILBOX_TYPE_NONE;
	view->folder_list = NULL;
	view->move_mailbox_list = NULL;
	view->move_mailbox_count = 0;
	view->move_status = 0;

	/* Evas Object */
	view->header_ly = NULL;
	view->reply_toolbar_ly = NULL;
	view->favourite_btn = NULL;
	view->subject_entry = NULL;
	view->attachment_ly = NULL;
	view->webview_ly = NULL;
	view->webview_button = NULL;
	view->webview = NULL;
	view->list_progressbar = NULL;
	view->con_popup = NULL;
	view->notify = NULL;
	view->pb_notify = NULL;
	view->pb_notify_lb = NULL;
	view->dn_btn = NULL;
	view->to_mbe = NULL;
	view->to_ly = NULL;
	view->cc_mbe = NULL;
	view->cc_ly = NULL;
	view->bcc_mbe = NULL;
	view->bcc_ly = NULL;
	view->to_recipients_cnt = 0;
	view->cc_recipients_cnt = 0;
	view->bcc_recipients_cnt = 0;
	view->to_recipient_idler = NULL;
	view->cc_recipient_idler = NULL;
	view->bcc_recipient_idler = NULL;
	view->idler_regionbringin = NULL;

	view->attachment_save_thread = 0;
	view->attachment_update_job = NULL;
	view->attachment_update_timer = NULL;
	view->attachment_data_list = NULL;
	view->attachment_data_list_to_save = NULL;
	view->preview_aid = NULL;
	view->show_cancel_all_btn = false;
	view->attachment_genlist = NULL;
	view->save_all_btn = NULL;
	view->attachment_group_item = NULL;
	view->attachment_itc = NULL;
	view->attachment_group_itc = NULL;

	view->cancel_sending_ctx_item = NULL;

	view->webview_data = NULL;

	/* Email Viewer Private variables */
	view->file_id = NULL;
	view->file_size = 0;
	view->email_handle = 0;
	view->mail_info = NULL;
	view->attachment_info = NULL;
	view->attachment_count = 0;

	/*Email Viewer Property Variables*/
	view->mail_type = 0;
	view->mktime = 0;
	view->sender_address = NULL;
	view->sender_display_name = NULL;
	view->subject = NULL;
	view->charset = NULL;
	view->body = NULL;
	view->body_uri = NULL;
	view->attachment_info_list = NULL;
	view->total_att_count = 0;
	view->normal_att_count = 0;
	view->total_att_size = 0;
	view->has_html = FALSE;
	view->has_attachment = FALSE;
	view->has_inline_image = FALSE;
	view->is_smil_mail = FALSE;
	view->favorite = FALSE;
	view->request_report = FALSE;

	view->account = NULL;

	view->launch_timer = NULL;
	view->rcpt_scroll_corr = NULL;

	view->recipient_address = NULL;
	view->recipient_name = NULL;
	view->selected_address = NULL;
	view->selected_name = NULL;
	view->recipient_contact_list_item = NULL;

	view->passwd_popup = NULL;
	return EINA_TRUE;
}

static void _timezone_changed_cb(system_settings_key_e key, void *data)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	header_update_date(view);
	debug_leave();
}

void create_body_progress(EmailViewerView *view)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");
	retm_if(view->list_progressbar != NULL, "progressbar already present");
	/*unset webview to show progressbar*/
	/*webview will be set on "load,finished" callback or after setting search highlight*/
	if (view->webview_ly && view->webview) {
		elm_object_part_content_unset(view->webview_ly, "elm.swallow.content");
		evas_object_hide(view->webview);
	}

	/*create list process animation*/
	debug_log("Creating list progress");
	view->list_progressbar = elm_progressbar_add(view->base_ly);
	elm_object_style_set(view->list_progressbar, "process_large");
	evas_object_size_hint_align_set(view->list_progressbar, EVAS_HINT_FILL, 0.5);
	evas_object_size_hint_weight_set(view->list_progressbar, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(view->list_progressbar);
	elm_progressbar_pulse(view->list_progressbar, EINA_TRUE);
	elm_object_part_content_set(view->base_ly, "ev.swallow.progress", view->list_progressbar);
	debug_leave();
}

void _create_body(EmailViewerView *view)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	/* create webview */
	if (view->webview == NULL) {
		viewer_get_webview(view);
	}

	viewer_set_webview_content(view);
	viewer_set_mail_seen(view);

	debug_leave();
}

void viewer_stop_ecore_running_apis(EmailViewerView *view)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	DELETE_TIMER_OBJECT(view->launch_timer);
	DELETE_TIMER_OBJECT(view->rcpt_scroll_corr);
	DELETE_IDLER_OBJECT(view->idler_regionbringin);
	DELETE_IDLER_OBJECT(view->to_recipient_idler);
	DELETE_IDLER_OBJECT(view->cc_recipient_idler);
	DELETE_IDLER_OBJECT(view->bcc_recipient_idler);
}

void viewer_delete_evas_objects(EmailViewerView *view, Eina_Bool isHide)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	if (view->base.module->views_count > 1) {
		email_module_exit_view(&view->attachment_view);
	}

	FREE(view->eml_file_path);

	/* folder list */
	if (NULL != view->folder_list) {
		int i = 0;
		LIST_ITER_START(i, view->folder_list) {
			email_mailbox_name_and_alias_t *nameandalias = (email_mailbox_name_and_alias_t *)LIST_ITER_GET_DATA(i, view->folder_list);
			FREE(nameandalias->name);
			FREE(nameandalias->alias);
			FREE(nameandalias);
		}
		g_list_free(view->folder_list);
		view->folder_list = NULL;
	}

	/* mailbox list */
	email_engine_free_mailbox_list(&view->move_mailbox_list, view->move_mailbox_count);
	view->move_mailbox_count = 0;

	/* Evas Object */
	DELETE_EVAS_OBJECT(view->passwd_popup);
	DELETE_EVAS_OBJECT(view->con_popup);
	DELETE_EVAS_OBJECT(view->pb_notify);
	DELETE_EVAS_OBJECT(view->notify);

	if (isHide) {
		DELETE_EVAS_OBJECT(view->combined_scroller);
		DELETE_EVAS_OBJECT(view->header_ly);
		DELETE_EVAS_OBJECT(view->reply_toolbar_ly);
		DELETE_EVAS_OBJECT(view->to_ly);
		DELETE_EVAS_OBJECT(view->cc_ly);
		DELETE_EVAS_OBJECT(view->bcc_ly);
		DELETE_EVAS_OBJECT(view->header_divider);
		DELETE_EVAS_OBJECT(view->details_button);
		DELETE_EVAS_OBJECT(view->favourite_btn);
		DELETE_EVAS_OBJECT(view->attachment_ly);
		DELETE_EVAS_OBJECT(view->dn_btn);
		DELETE_EVAS_OBJECT(view->webview_ly);
		DELETE_EVAS_OBJECT(view->webview_button);
		DELETE_EVAS_OBJECT(view->list_progressbar);
	}

	FREE(view->selected_address);
	FREE(view->selected_name);
	email_contacts_delete_contact_info(&view->recipient_contact_list_item);

	debug_leave();
}

static void _viewer_on_back_key(email_view_t *self)
{
	debug_enter();
	retm_if(self == NULL, "Invalid parameter: self is NULL");

	EmailViewerView *view = (EmailViewerView *)self;
	viewer_back_key_press_handler(view);

	debug_leave();
}

void _delete_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	DELETE_EVAS_OBJECT(view->con_popup);

	email_string_t title = { PACKAGE, "IDS_EMAIL_HEADER_DELETE" };
	email_string_t content = { PACKAGE, "IDS_EMAIL_POP_THIS_EMAIL_WILL_BE_DELETED" };
	email_string_t btn2 = { PACKAGE, "IDS_EMAIL_BUTTON_DELETE_ABB4" };
	util_create_notify(view, title, content, 2,
			EMAIL_VIEWER_STRING_CANCEL, _popup_response_cb,
			btn2, _popup_response_delete_ok_cb, NULL);

	debug_leave();
}

void _move_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;

	DELETE_EVAS_OBJECT(view->con_popup);

	char s_mail_id[EMAIL_BUFF_SIZE_SML] = { 0 };
	const char *sa_mail_id = s_mail_id;
	snprintf(s_mail_id, EMAIL_BUFF_SIZE_SML, "%d", view->mail_id);

	email_params_h params = NULL;

	if (email_params_create(&params) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_ACCOUNT_ID, view->account_id) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_MAILBOX, view->mailbox_id) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_MOVE_SRC_MAILBOX_ID, view->mailbox_id) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_IS_MAILBOX_MOVE_MODE, 1) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_MAILBOX_MOVE_MODE, 0) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_IS_MAILBOX_EDIT_MODE, 0) &&
		email_params_add_str_array(params, EMAIL_BUNDLE_KEY_ARRAY_SELECTED_MAIL_IDS, &sa_mail_id, 1)) {

		view->loaded_module = viewer_create_module(view, EMAIL_MODULE_ACCOUNT, params);
	}

	email_params_free(&params);

	debug_leave();
}

static void _more_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	viewer_create_more_ctxpopup(view);
	debug_leave();
}

void _download_body(void *data)
{
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	int handle;
	gboolean ret = 0;
	EmailViewerView *view = (EmailViewerView *)data;

	if (viewer_check_storage_full(MIN_FREE_SPACE)) {
		viewer_show_storage_full_popup(view);
		return;
	}
	ret = email_engine_body_download(view->account_id, view->mail_id, FALSE, &handle);

	if (ret == TRUE) {
		view->email_handle = handle;
		debug_log("mail_id(%d) account_id(%d) handle(%d)",
				view->mail_id, view->account_id, view->email_handle);
		viewer_create_down_progress(view, EMAIL_VIEWER_POPUP_DOWNLOADING, viewer_destroy_down_progress_cb);
	} else {
		debug_log("unable to download mail body");
		notification_status_message_post(_("IDS_EMAIL_POP_DOWNLOAD_FAILED"));
	}
}

static void _download_msg_btn_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;
	retm_if(view->is_download_message_btn_clicked, "Already clicked!!");

	/* check already downloaded */
	int body_download_status = view->body_download_status;

	if (viewer_check_body_download_status(body_download_status, EMAIL_BODY_DOWNLOAD_STATUS_NONE)) {
		debug_log("only header downloaded -> need body download");
	} else if (viewer_check_body_download_status(body_download_status, EMAIL_BODY_DOWNLOAD_STATUS_PARTIALLY_DOWNLOADED)) {
		debug_log("body downloaded partially -> need full body download");
	}

	_download_body(view);

	debug_leave();
}

static void _popup_response_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	DELETE_EVAS_OBJECT(view->notify);
	debug_leave();
}

static void _popup_response_delete_ok_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;
	int ret = 0;

	DELETE_EVAS_OBJECT(view->notify);

	if (view->mailbox_type == EMAIL_MAILBOX_TYPE_TRASH || view->save_status == EMAIL_MAIL_STATUS_SEND_SCHEDULED) {
		/*cancel scheduled email before deleting the mail*/
		if (view->save_status == EMAIL_MAIL_STATUS_SEND_SCHEDULED) {
			email_engine_cancel_sending_mail(view->mail_id);
		}
		ret = viewer_delete_email(view);
	} else if (view->viewer_type == EML_VIEWER) {
		if (remove(view->eml_file_path)) {
			debug_log("failed to delete eml file");
		} else {
			debug_log("succeed to delete eml file");
			ret = 1;
		}
	} else {
		ret = viewer_move_email(view, _find_folder_id_using_folder_type(view, EMAIL_MAILBOX_TYPE_TRASH), TRUE, FALSE);
	}

	if (ret == 1) {
		debug_log("email_module_make_destroy_request");
		email_module_make_destroy_request(view->base.module);
	}
	debug_leave();
}

static void _popup_response_to_destroy_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	DELETE_EVAS_OBJECT(view->notify);
	debug_log("email_module_make_destroy_request");
	email_module_make_destroy_request(view->base.module);
	debug_leave();
}

/* Double_Scroller */
static void _outter_scroller_bottom_hit_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	viewer_stop_elm_scroller_start_webkit_scroller(view);

	debug_leave();
}

static void _outter_scroller_top_hit_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;
	view->is_outer_scroll_bot_hit = EINA_FALSE;
	edje_object_part_drag_value_set(_EDJ(view->combined_scroller), "elm.dragable.vbar", 0.0, 0.0);
	debug_leave();
}

void viewer_remove_callback(EmailViewerView *view)
{
	email_unregister_timezone_changed_callback(_timezone_changed_cb);
	viewer_close_pattern_generator();
}

int viewer_reset_mail_data(EmailViewerView *view)
{
	debug_enter();
	retvm_if(view == NULL, VIEWER_ERROR_INVALID_ARG, "Invalid parameter: view[NULL]");

	/* Create Email Viewer Private variables */
	email_engine_free_mail_data_list(&view->mail_info, 1);

	if (view->attachment_info) {
		debug_log("attachment_info freed p[%p] n[%d]", view->attachment_info, view->attachment_count);
		email_engine_free_attachment_data_list(&view->attachment_info, view->attachment_count);
		view->attachment_count = 0;
	}

	/* Get mail_info from service, and fill property */
	int ret = viewer_set_internal_data(view, EINA_TRUE);
	if (ret != VIEWER_ERROR_NONE) {
		debug_log("viewer_set_internal_data() failed! (error type: %d)", ret);
		return ret;
	}

	viewer_get_address_info_count(view->mail_info, &view->to_recipients_cnt, &view->cc_recipients_cnt, &view->bcc_recipients_cnt);

	debug_leave();
	return VIEWER_ERROR_NONE;
}

static void _create_webview_layout(void *data)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;
	if (view->webview_ly == NULL) {
		Evas_Object *ewk_view_ly = _create_fullview(view->base.content);
		view->webview_ly = ewk_view_ly;

		Evas_Object *ewk_btn = elm_button_add(view->base.content);
		elm_object_style_set(ewk_btn, "transparent");
		evas_object_size_hint_weight_set(ewk_btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(ewk_btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
		elm_object_part_content_set(view->base.content, "ev.swallow.webview_button", ewk_btn);
		evas_object_show(ewk_btn);
		view->webview_button = ewk_btn;
	} else {
		evas_object_show(view->webview_ly);
		evas_object_show(view->webview_button);
	}
	elm_object_part_content_set(view->base_ly, "ev.swallow.webview", view->webview_ly);

	debug_leave();
}

Eina_Bool viewer_check_body_download(void *data)
{
	debug_enter();
	retvm_if(data == NULL, EINA_FALSE, "Invalid parameter: data[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;
	_create_webview_layout(view);
	_create_body(view);

	if (viewer_check_body_download_status(view->body_download_status, EMAIL_BODY_DOWNLOAD_STATUS_PARTIALLY_DOWNLOADED)) {
		view->b_partial_body = 1;
	} else {
		view->b_partial_body = 0;
	}

	if (!viewer_check_body_download_status(view->body_download_status, EMAIL_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED)) {
		viewer_create_body_button(view, "IDS_EMAIL_BUTTON_DOWNLOAD_FULL_EMAIL_ABB", _download_msg_btn_clicked_cb);
	}
	debug_leave();
	return EINA_FALSE;
}

void viewer_create_account_data(EmailViewerView *view)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	email_account_t *account = NULL;
	if (email_engine_get_account_full_data(view->account_id, &account)) {
		if (account) {
			view->account_type = account->incoming_server_type;
			FREE(view->account_email_address);
			view->account_email_address = strdup(account->user_email_address);
			account_user_data_t *ud = (account_user_data_t *)account->user_data;
			if (ud != NULL) {
				debug_log("show_images is %d", ud->show_images);
				view->b_show_remote_images = ud->show_images;
			}
		}
		viewer_delete_account_data(view);
		view->account = account;
	}

	debug_leave();
}

void viewer_delete_account_data(EmailViewerView *view)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	email_engine_free_account_list(&view->account, 1);

	debug_leave();
}

static void _viewer_initialize_theme(EmailViewerView *view)
{
	debug_enter();
	retm_if(!view, "Invalid parameter, view is NULL");

	view->theme = elm_theme_new();
	elm_theme_ref_set(view->theme, NULL);
	elm_theme_extension_add(view->theme, email_get_viewer_theme_path());

	debug_leave();

}

static void _viewer_deinitialize_theme(EmailViewerView *view)
{
	debug_enter();
	retm_if(!view, "Invalid parameter, view is NULL");

	elm_theme_extension_del(view->theme, email_get_viewer_theme_path());
	elm_theme_free(view->theme);
	view->theme = NULL;

	debug_leave();

}

static void _destroy_viewer(EmailViewerView *view)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	view->b_viewer_hided = TRUE;
	viewer_webkit_del_callbacks(view);
	viewer_delete_account_data(view);
	viewer_stop_ecore_running_apis(view);
	recipient_delete_callbacks(view);
	viewer_delete_evas_objects(view, EINA_FALSE);
	viewer_remove_hard_link_for_inline_images(view);
	viewer_remove_temp_files(view);
	viewer_remove_temp_folders(view);
	viewer_free_viewer_data(view);
	viewer_remove_callback(view);
	noti_mgr_dbus_receiver_remove(view);

	email_engine_finalize();
	int ct_ret = email_contacts_finalize_service();
	debug_warning_if(ct_ret != CONTACTS_ERROR_NONE, "email_contacts_finalize_service() failed! ct_ret:[%d]", ct_ret);
	_viewer_deinitialize_theme(view);
	_g_md = NULL;

	debug_leave();
}

/* EOF */
