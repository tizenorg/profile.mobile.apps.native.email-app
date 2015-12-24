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

#include <notification.h>
#include <sys/vfs.h>
#include <malloc.h>
#include <storage.h>
#include <app.h>
#include <app_common.h>

#include "email-utils-contacts.h"
#include "email-composer.h"
#include "email-composer-util.h"
#include "email-composer-attachment.h"
#include "email-composer-recipient.h"
#include "email-composer-subject.h"
#include "email-composer-webkit.h"
#include "email-composer-predictive-search.h"
#include "email-composer-js.h"
#include "email-composer-more-menu.h"
#include "email-composer-send-mail.h"
#include "email-composer-initial-view.h"
#include "email-composer-initial-data.h"
#include "email-composer-launcher.h"

/*
 * Declaration for static functions
 */

static Eina_Bool _composer_destroy_timer_cb(void *data);
static Eina_Bool _composer_delayed_drawing_timer_cb(void *data);
static Eina_Bool _composer_update_data_after_adding_account_timer_cb(void *data);
static void _composer_setting_destroy_request(void *data, email_module_h module);
static void _composer_launch_settings_popup_response_cb(void *data, Evas_Object *obj, void *event_info);
static void _composer_storage_full_popup_response_cb(void *data, Evas_Object *obj, void *event_info);
static void _composer_create_delay_loading_popup(EmailComposerUGD *ugd);

static COMPOSER_ERROR_TYPE_E _composer_check_storage_full(unsigned int mb_size);
static COMPOSER_ERROR_TYPE_E _composer_load_account_list(void *data);
static COMPOSER_ERROR_TYPE_E _composer_initialize_mail_info(void *data);
static COMPOSER_ERROR_TYPE_E _composer_initialize_account_data(void *data);
static COMPOSER_ERROR_TYPE_E _composer_prepare_vcard(void *data);
static COMPOSER_ERROR_TYPE_E _composer_initialize_composer_data(void *data);
static COMPOSER_ERROR_TYPE_E _composer_create_temp_folder(const char *root_tmp_folder, const char *tmp_folder);
static COMPOSER_ERROR_TYPE_E _composer_create_temp_folders();
static COMPOSER_ERROR_TYPE_E _composer_make_composer_tmp_dir(const char *path);
static COMPOSER_ERROR_TYPE_E _composer_initialize_dbus(EmailComposerUGD *ugd);
static COMPOSER_ERROR_TYPE_E _composer_initialize_services(void *data);
static COMPOSER_ERROR_TYPE_E _composer_initialize_themes(void *data);
static COMPOSER_ERROR_TYPE_E _composer_construct_composer_data(void *data);

static void *_composer_vcard_save_worker(void *data);
static Eina_Bool _composer_vcard_save_timer_cb(void *data);

static void _composer_remove_temp_folders();
static void _composer_finalize_dbus(EmailComposerUGD *ugd);
static void _composer_finalize_services(void *data);
static void _composer_destroy_email_info(EmailComposerUGD *ugd);
static void _composer_destroy_evas_objects(EmailComposerUGD *ugd);
static void _composer_destroy_composer(void *data);

static void _composer_process_error_popup(EmailComposerUGD *ugd, COMPOSER_ERROR_TYPE_E error);

static void _composer_set_environment_variables();
static void _composer_add_main_callbacks(EmailComposerUGD *ugd);
static void _composer_del_main_callbacks(EmailComposerUGD *ugd);

static int _composer_module_create(email_module_t *self, app_control_h params);
static void _composer_on_message(email_module_t *self, app_control_h msg);
static void _composer_on_event(email_module_t *self, email_module_event_e event);

static int _composer_create(email_view_t *self);
static void _composer_destroy(email_view_t *self);
static void _composer_activate(email_view_t *self, email_view_state prev_state);
static void _composer_deactivate(email_view_t *self);
static void _composer_update(email_view_t *self, int flags);
static void _composer_on_back_key(email_view_t *self);

static void _composer_start(EmailComposerUGD *ugd);

static void _composer_orientation_change_update(EmailComposerUGD *ugd);
static void _composer_language_change_update(EmailComposerUGD *ugd);

static void _composer_virtualkeypad_state_on_cb(void *data, Evas_Object *obj, void *event_info);
static void _composer_virtualkeypad_state_off_cb(void *data, Evas_Object *obj, void *event_info);
static void _composer_virtualkeypad_size_changed_cb(void *data, Evas_Object *obj, void *event_info);
static void _composer_gdbus_signal_receiver_cb(GDBusConnection *connection, const gchar *sender_name, const gchar *object_path,
		const gchar *interface_name, const gchar *signal_name, GVariant *parameters, gpointer data);

static void _composer_set_charset_info(void *data);
static void _composer_contacts_update_recp_info_for_mbe(EmailComposerUGD *ugd, Evas_Object *mbe);
static void _composer_contacts_update_recp_info_for_recipients(EmailComposerUGD *ugd);

GDBusConnection *_g_composer_dbus_conn = NULL;
guint _g_composer_network_id = 0;

static email_string_t EMAIL_COMPOSER_STRING_NULL = { NULL, NULL };
static email_string_t EMAIL_COMPOSER_STRING_BUTTON_OK = { PACKAGE, "IDS_EMAIL_BUTTON_OK" };
static email_string_t EMAIL_COMPOSER_STRING_TPOP_LOADING_ING = { PACKAGE, "IDS_EMAIL_TPOP_LOADING_ING" };
static email_string_t EMAIL_COMPOSER_STRING_HEADER_UNABLE_TO_DOWNLOAD_ATTACHMENT_ABB = { PACKAGE, "IDS_EMAIL_HEADER_UNABLE_TO_DOWNLOAD_ATTACHMENT_ABB" };
static email_string_t EMAIL_COMPOSER_STRING_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED = { PACKAGE, "IDS_EMAIL_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED" };

static email_string_t EMAIL_COMPOSER_STRING_BUTTON_SETTINGS = { PACKAGE, "IDS_EMAIL_BUTTON_SETTINGS" };
static email_string_t EMAIL_COMPOSER_STRING_STORAGE_FULL_CONTENTS = { PACKAGE, "IDS_EMAIL_POP_THERE_IS_NOT_ENOUGH_SPACE_IN_YOUR_DEVICE_STORAGE_GO_TO_SETTINGS_POWER_AND_STORAGE_STORAGE_THEN_DELETE_SOME_FILES_AND_TRY_AGAIN" };
static email_string_t EMAIL_COMPOSER_STRING_VCARD_CREATE_FILED = { PACKAGE, "IDS_EMAIL_POP_THE_VCARD_HAS_NOT_BEEN_CREATED_AN_UNKNOWN_ERROR_HAS_OCCURRED" };
static email_string_t EMAIL_COMPOSER_STRING_UABLE_TO_COMPOSE_EMAIL_MESSAGE = { PACKAGE, "IDS_EMAIL_HEADER_UNABLE_TO_COMPOSE_EMAIL_ABB" };
static email_string_t EMAIL_COMPOSER_STRING_THERE_IS_NO_ACCOUNT_CREATE_A_NEW_ACCOUNT_FIRST = { PACKAGE, "IDS_EMAIL_POP_TO_COMPOSE_AN_EMAIL_CREATE_AN_EMAIL_ACCOUNT" };


/*
 * Definition for static functions
 */

static Eina_Bool _composer_destroy_timer_cb(void *data)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	ugd->timer_destory_composer = NULL;
	DELETE_EVAS_OBJECT(ugd->composer_popup);

	/* Restore the indicator mode. */
	composer_util_indicator_restore(ugd);

	email_module_make_destroy_request(ugd->base.module);

	debug_leave();
	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _composer_delayed_drawing_timer_cb(void *data)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	ugd->timer_lazy_loading = NULL;

	ugd->eComposerErrorType = _composer_construct_composer_data(ugd);

	gotom_if(ugd->eComposerErrorType != COMPOSER_ERROR_NONE, CATCH, "_composer_construct_composer_data() failed");

	composer_initial_view_draw_remaining_components(ugd);

	composer_initial_data_set_mail_info(ugd, true);
	composer_initial_data_post_parse_arguments(ugd, ugd->ec_app);
	composer_initial_data_make_initial_contents(ugd);

CATCH:
	if (ugd->eComposerErrorType != COMPOSER_ERROR_NONE) {
		_composer_process_error_popup(ugd, ugd->eComposerErrorType);
	}

	debug_leave();
	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _composer_update_data_after_adding_account_timer_cb(void *data)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	ugd->timer_adding_account = NULL;

	ugd->eComposerErrorType = _composer_load_account_list(ugd);
	gotom_if(ugd->eComposerErrorType != COMPOSER_ERROR_NONE, CATCH, "_composer_load_account_list() failed");

	ugd->eComposerErrorType = _composer_construct_composer_data(ugd);
	gotom_if(ugd->eComposerErrorType != COMPOSER_ERROR_NONE, CATCH, "_composer_construct_composer_data() failed");

	composer_initial_data_set_mail_info(ugd, true);
	composer_initial_data_post_parse_arguments(ugd, ugd->ec_app);
	composer_initial_data_make_initial_contents(ugd);

CATCH:
	if (ugd->eComposerErrorType != COMPOSER_ERROR_NONE) {
		_composer_process_error_popup(ugd, ugd->eComposerErrorType);
	}

	debug_leave();
	return ECORE_CALLBACK_CANCEL;
}

static void _composer_setting_destroy_request(void *data, email_module_h module)
{
	debug_enter();

	EmailComposerUGD *ugd = data;
	int account_id = 0;

	if (!email_engine_get_default_account(&account_id)) {
		debug_log("No account was registered. Terminating...");
		composer_util_indicator_restore(ugd);
		email_module_make_destroy_request(ugd->base.module);
	} else {
		debug_log("added account id: [%d]", account_id);
		ugd->is_adding_account_requested = EINA_TRUE;
		ugd->selected_entry = ugd->recp_to_entry.entry;
		email_module_destroy(module);
	}

	debug_leave();
}

static void _composer_launch_settings_popup_response_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerUGD *ugd = data;
	app_control_h params = NULL;
	email_module_h module = NULL;

	if ((obj != ugd->composer_popup) &&
		email_params_create(&params) &&
		email_params_add_str(params, EMAIL_BUNDLE_KEY_VIEW_TYPE, EMAIL_BUNDLE_VAL_VIEW_ACCOUNT_SETUP)) {

		email_module_listener_t listener = { 0 };
		listener.cb_data = ugd;
		listener.destroy_request_cb = _composer_setting_destroy_request;

		module = email_module_create_child(ugd->base.module, EMAIL_MODULE_SETTING, params, &listener);
	}

	email_params_free(&params);

	if (!module) {
		debug_error("Terminating...");
		composer_util_indicator_restore(ugd);
		email_module_make_destroy_request(ugd->base.module);
	} else {
		DELETE_EVAS_OBJECT(ugd->composer_popup);
	}

	debug_leave();
}

static void _composer_create_delay_loading_popup(EmailComposerUGD *ugd)
{
	debug_enter();

	/* TODO: The loading progress couldn't be displayed smoothly. So we cannot use progress popup here.
	 * If we have to use progress popup, we need to refactoring the routine which draws elm widgets.
	 */
	ugd->composer_popup = composer_util_popup_create_with_progress_horizontal(ugd, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_TPOP_LOADING_ING, NULL,
											EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);
	ugd->is_loading_popup = EINA_TRUE;

	debug_leave();
}

static void _composer_storage_full_popup_response_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	char *button_text = NULL;

	/* To distinguish soft button and hardware back button */
	if (obj != ugd->composer_popup) {
		button_text = (char *)elm_object_text_get(obj);
	}

	if (!g_strcmp0(button_text, email_get_email_string(EMAIL_COMPOSER_STRING_BUTTON_SETTINGS.id))) {
		composer_launcher_launch_storage_settings(ugd);
		ugd->is_need_close_composer_on_low_memory = EINA_TRUE;
	} else {
		DELETE_EVAS_OBJECT(ugd->composer_popup);

		/* Restore the indicator mode. */
		composer_util_indicator_restore(ugd);

		email_module_make_destroy_request(ugd->base.module);
	}

	debug_leave();
}

static COMPOSER_ERROR_TYPE_E _composer_check_storage_full(unsigned int mb_size)
{
	debug_enter();

	COMPOSER_ERROR_TYPE_E ret = COMPOSER_ERROR_NONE;
	unsigned long int need_size = mb_size * 1024 * 1024;
	struct statvfs s;

	int r = storage_get_internal_memory_size(&s);
	if (r < 0) {
		debug_error("storage_get_internal_memory_size() failed! ret:[%d]", r);
		return COMPOSER_ERROR_STORAGE_FULL;
	}

	if ((need_size / s.f_bsize) > s.f_bavail) {
		debug_secure_error("Not enough free space! [%lf]", (double)s.f_bsize * s.f_bavail);
		ret = COMPOSER_ERROR_STORAGE_FULL;
	}

	debug_leave();
	return ret;
}

static COMPOSER_ERROR_TYPE_E _composer_initialize_mail_info(void *data)
{
	debug_enter();

	retvm_if(!data, COMPOSER_ERROR_INVALID_ARG, "Invalid parameter: data is NULL!");

	int err = 0;
	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	/* Get mail data */
	if (ugd->composer_type == RUN_COMPOSER_EDIT || ugd->composer_type == RUN_COMPOSER_REPLY || ugd->composer_type == RUN_COMPOSER_REPLY_ALL || ugd->composer_type == RUN_COMPOSER_FORWARD) {
		err = email_get_mail_data(ugd->original_mail_id, &ugd->org_mail_info->mail_data);
		retvm_if(err != EMAIL_ERROR_NONE, COMPOSER_ERROR_SERVICE_INIT_FAIL, "email_get_mail_data() failed! err:[%d]", err);

		err = email_get_attachment_data_list(ugd->original_mail_id, &ugd->org_mail_info->attachment_list, &ugd->org_mail_info->total_attachment_count);
		retvm_if(err != EMAIL_ERROR_NONE, COMPOSER_ERROR_SERVICE_INIT_FAIL, "email_get_attachment_data_list() failed! err:[%d]", err);

		debug_warning_if(ugd->org_mail_info->mail_data->attachment_count+ugd->org_mail_info->mail_data->inline_content_count != ugd->org_mail_info->total_attachment_count,
				"ATTACHMENT count is different! [%d != %d]", ugd->org_mail_info->mail_data->attachment_count+ugd->org_mail_info->mail_data->inline_content_count,
				ugd->org_mail_info->total_attachment_count);

		/* Parse charset of original mail. */
		_composer_set_charset_info(ugd);
		debug_secure("Original charset: (%s)", ugd->original_charset);

		if (ugd->composer_type == RUN_COMPOSER_EDIT) {
			/* Need to update seen flag for original mail in draft folder */
			err = email_set_flags_field(ugd->account_info->original_account_id, &ugd->original_mail_id, 1, EMAIL_FLAGS_SEEN_FIELD, 1, 1);
			debug_warning_if(err != EMAIL_ERROR_NONE, "email_set_flags_field() failed! ret:[%d]", err);

			/* Set priority information */
			ugd->priority_option = ugd->org_mail_info->mail_data->priority;
		}
	} else if ((ugd->composer_type == RUN_EML_REPLY) || (ugd->composer_type == RUN_EML_REPLY_ALL) || (ugd->composer_type == RUN_EML_FORWARD)) {
		err = email_parse_mime_file(ugd->eml_file_path, &ugd->org_mail_info->mail_data, &ugd->org_mail_info->attachment_list, &ugd->org_mail_info->total_attachment_count);
		retvm_if(err != EMAIL_ERROR_NONE, COMPOSER_ERROR_SERVICE_INIT_FAIL, "email_parse_mime_file() failed! err:[%d]", err);

		debug_secure("full_address_from :%s", ugd->org_mail_info->mail_data->full_address_from);
		debug_secure("full_address_reply :%s", ugd->org_mail_info->mail_data->full_address_reply);
	}

	/* Create temp file */
	ugd->saved_html_path = g_strconcat(composer_util_file_get_public_temp_dirname(), "/", DEFAULT_CHARSET, ".htm", NULL);
	ugd->saved_text_path = g_strconcat(composer_util_file_get_public_temp_dirname(), "/", DEFAULT_CHARSET, NULL);

	debug_leave();
	return COMPOSER_ERROR_NONE;
}

static COMPOSER_ERROR_TYPE_E _composer_load_account_list(void *data)
{
	debug_enter();

	retvm_if(!data, COMPOSER_ERROR_INVALID_ARG, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	int err = email_get_account_list(&ugd->account_info->account_list, &ugd->account_info->account_count);
	retvm_if(err != EMAIL_ERROR_NONE, COMPOSER_ERROR_NO_ACCOUNT_LIST, "email_get_account_list() failed! err:[%d]", err);

	/* If there's no account id, set account_id as default account id); */
	if (!ugd->account_info->original_account_id && (email_engine_get_default_account(&ugd->account_info->original_account_id) == false)) {
		return COMPOSER_ERROR_NO_DEFAULT_ACCOUNT;
	}

	debug_leave();
	return COMPOSER_ERROR_NONE;
}

static COMPOSER_ERROR_TYPE_E _composer_initialize_account_data(void *data)
{
	debug_enter();

	retvm_if(!data, COMPOSER_ERROR_INVALID_ARG, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	EmailComposerAccount *account_info = ugd->account_info;
	retvm_if(!account_info, COMPOSER_ERROR_NULL_POINTER, "account_info is NULL!");

	int i;
	for (i = 0; i < account_info->account_count; i++) {
		if (account_info->original_account_id == account_info->account_list[i].account_id) {
			account_info->original_account = &account_info->account_list[i];
			account_info->selected_account = &account_info->account_list[i];
		}
		debug_secure("[%d] %s", account_info->account_list[i].account_id, account_info->account_list[i].user_email_address);
	}
	account_info->account_type = account_info->original_account->incoming_server_type;
	if (account_info->original_account->outgoing_server_size_limit == 0 || account_info->original_account->outgoing_server_size_limit == -1) {
		account_info->max_sending_size = MAX_ATTACHMENT_SIZE;
	} else {
		account_info->max_sending_size = account_info->original_account->outgoing_server_size_limit / 1.5;
	}

#ifdef ATTACH_PANEL_FEATURE
	composer_util_update_attach_panel_bundles(ugd);
#endif

	debug_log("outgoing_server_size_limit:%d, max_sending_size:%d", account_info->original_account->outgoing_server_size_limit, account_info->max_sending_size);

	debug_leave();
	return COMPOSER_ERROR_NONE;
}

static COMPOSER_ERROR_TYPE_E _composer_prepare_vcard(void *data)
{
	debug_enter();

	retvm_if(!data, COMPOSER_ERROR_INVALID_ARG, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	char *file_path = NULL;

	if (!ugd->is_sharing_contacts) {
		debug_log("Not sharing contacts");
		return COMPOSER_ERROR_NONE;
	}

	debug_log("ugd->is_sharing_contacts = %s", ugd->is_sharing_contacts ? "true" : "false");
	debug_log("ugd->is_sharing_my_profile = %s", ugd->is_sharing_my_profile ? "true" : "false");
	debug_log("ugd->shared_contacts_count = %d", ugd->shared_contacts_count);
	debug_log("ugd->shared_contact_id = %d", ugd->shared_contact_id);
	debug_log("ugd->shared_contact_id_list = %p", ugd->shared_contact_id_list);

	if (ugd->shared_contacts_count == 1) {
		int id = (ugd->shared_contact_id_list ? ugd->shared_contact_id_list[0] : ugd->shared_contact_id);
		file_path = email_contacts_create_vcard_from_id(id, composer_util_file_get_temp_dirname(), ugd->is_sharing_my_profile);
	} else {
		ugd->vcard_save_timer = ecore_timer_add(COMPOSER_VCARD_SAVE_POPUP_SHOW_MIN_TIME_SEC,
				_composer_vcard_save_timer_cb, ugd);
		if (ugd->vcard_save_timer) {
			int ret = 0;
			ecore_timer_freeze(ugd->vcard_save_timer);
			ugd->vcard_save_cancel = EINA_FALSE;
			ret = pthread_create(&ugd->vcard_save_thread, NULL, _composer_vcard_save_worker, ugd);
			if (ret == 0) {
				debug_log("vCard save thread was created.");
				return COMPOSER_ERROR_NONE;
			}
			debug_error("pthread_create() failed: %d", ret);
			ugd->vcard_save_thread = 0;
			DELETE_TIMER_OBJECT(ugd->vcard_save_timer);
		} else {
			debug_error("ecore_timer_add() failed");
		}

		debug_log("Generating in main thread...");

		file_path = email_contacts_create_vcard_from_id_list(ugd->shared_contact_id_list, ugd->shared_contacts_count,
				composer_util_file_get_temp_dirname(), NULL);
	}

	if (!file_path) {
		debug_error("vCard generation failed!");
		return COMPOSER_ERROR_VCARD_CREATE_FAILED;
	}

	ugd->vcard_file_path = file_path;

	debug_leave();
	return COMPOSER_ERROR_NONE;
}

static void *_composer_vcard_save_worker(void *data)
{
	debug_enter();

	retvm_if(!data, NULL, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	ugd->vcard_file_path = email_contacts_create_vcard_from_id_list(
			ugd->shared_contact_id_list, ugd->shared_contacts_count, composer_util_file_get_temp_dirname(), &ugd->vcard_save_cancel);
	if (!ugd->vcard_file_path) {
		debug_error("Failed to generate vCard file!");
		ugd->is_vcard_create_error = EINA_TRUE;
	} else {
		debug_log("Generation of vCard is complete.");
	}

	return NULL;
}

static Eina_Bool _composer_vcard_save_timer_cb(void *data)
{
	retvm_if(!data, ECORE_CALLBACK_CANCEL, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	if (ugd->vcard_file_path || ugd->is_vcard_create_error) {
		DELETE_EVAS_OBJECT(ugd->composer_popup);

		pthread_join(ugd->vcard_save_thread, NULL);
		ugd->vcard_save_thread = 0;

		ugd->vcard_save_timer = NULL;

		if (!ugd->is_vcard_create_error) {

			composer_attachment_process_attachments_with_string(ugd,
					ugd->vcard_file_path, "", ATTACH_BY_USE_ORG_FILE, EINA_FALSE);

			if (ugd->is_ewk_ready) {
				ugd->is_load_finished = EINA_TRUE;
				if (ugd->need_to_destroy_after_initializing) {
					ugd->is_save_in_drafts_clicked = EINA_TRUE;
					composer_exit_composer_get_contents(ugd);
				}
			}

		} else if (ugd->eComposerErrorType == COMPOSER_ERROR_NONE) {
			debug_log("Updating composer error status.");
			ugd->eComposerErrorType = COMPOSER_ERROR_VCARD_CREATE_FAILED;
			if (ugd->base.state != EV_STATE_CREATED) {
				debug_log("Composer already started. Creating error popup.");
				_composer_process_error_popup(ugd, ugd->eComposerErrorType);
			}
		} else {
			debug_log("Another composer error is already set: %d", ugd->eComposerErrorType);
		}

		return ECORE_CALLBACK_CANCEL;
	}

	ecore_timer_interval_set(ugd->vcard_save_timer, COMPOSER_VCARD_SAVE_STATUS_CHECK_INTERVAL_SEC);

	return ECORE_CALLBACK_RENEW;
}

static COMPOSER_ERROR_TYPE_E _composer_initialize_composer_data(void *data)
{
	debug_enter();

	retvm_if(!data, COMPOSER_ERROR_INVALID_ARG, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	ugd->eComposerErrorType = COMPOSER_ERROR_NONE;
	ugd->priority_option = EMAIL_MAIL_PRIORITY_NORMAL;

	ugd->need_to_set_focus_with_js = EINA_TRUE;

	ugd->with_original_message = EINA_FALSE;
	ugd->is_focus_on_new_message_div = EINA_TRUE;
	ugd->is_checkbox_clicked = EINA_TRUE;

	/* To make scrolling speed slow */
	ugd->scroll_friction = elm_config_scroll_bring_in_scroll_friction_get();
	elm_config_scroll_bring_in_scroll_friction_set(0.6);
	debug_log("==> original scroll_friction: (%f)", ugd->scroll_friction);

	ugd->org_mail_info = (EmailComposerMail *)calloc(1, sizeof(EmailComposerMail));
	retvm_if(!ugd->org_mail_info, COMPOSER_ERROR_OUT_OF_MEMORY, "Failed to allocate memory for ugd->org_mail_info");

	ugd->new_mail_info = (EmailComposerMail *)calloc(1, sizeof(EmailComposerMail));
	retvm_if(!ugd->new_mail_info, COMPOSER_ERROR_OUT_OF_MEMORY, "Failed to allocate memory for ugd->new_mail_info");

	ugd->new_mail_info->mail_data = (email_mail_data_t *)calloc(1, sizeof(email_mail_data_t));
	retvm_if(!ugd->new_mail_info->mail_data, COMPOSER_ERROR_OUT_OF_MEMORY, "Failed to allocate memory for ugd->new_mail_info->mail_data");

	ugd->account_info = (EmailComposerAccount *)calloc(1, sizeof(EmailComposerAccount));
	retvm_if(!ugd->account_info, COMPOSER_ERROR_OUT_OF_MEMORY, "Failed to allocate memory for ugd->account_info");

	debug_leave();
	return COMPOSER_ERROR_NONE;
}

static COMPOSER_ERROR_TYPE_E _composer_make_composer_tmp_dir(const char *path)
{
	debug_enter();
	int curent_umask = umask(0);
	int result = COMPOSER_ERROR_NONE;
	if (mkdir(path, EMAIL_COMPOSER_TMP_FOLDER_PERMISSION) == -1) {
		debug_error("mkdir() for composer folder failed! (%d): %s", errno, strerror(errno));
		result = COMPOSER_ERROR_FAIL;
	}
	umask(curent_umask);
	return result;
}

static COMPOSER_ERROR_TYPE_E _composer_create_temp_folder(const char *root_tmp_folder, const char *tmp_folder)
{
	debug_enter();

	retvm_if(!root_tmp_folder, COMPOSER_ERROR_INVALID_ARG, "root_tmp_folder is NULL");
	retvm_if(!tmp_folder, COMPOSER_ERROR_INVALID_ARG, "tmp_folder is NULL");

	if (!email_check_dir_exist(root_tmp_folder)) {
		int res = _composer_make_composer_tmp_dir(root_tmp_folder);
		retvm_if(res != COMPOSER_ERROR_NONE, res, "_composer_make_composer_tmp_dir() for '%s' failed", root_tmp_folder);
	}

	if (!email_check_dir_exist(tmp_folder)) {
		int res = _composer_make_composer_tmp_dir(tmp_folder);
		retvm_if(res != COMPOSER_ERROR_NONE, res, "_composer_make_composer_tmp_dir() for '%s' failed", tmp_folder);
	} else {
		debug_log("Email temp folder '%s' already exists!", tmp_folder);
	}

	debug_leave();
	return COMPOSER_ERROR_NONE;
}

static COMPOSER_ERROR_TYPE_E _composer_create_temp_folders()
{
	debug_enter();

	COMPOSER_ERROR_TYPE_E ret = COMPOSER_ERROR_NONE;

	ret = _composer_create_temp_folder(email_get_composer_tmp_dir(),
			composer_util_file_get_temp_dirname());
	retvm_if(ret != COMPOSER_ERROR_NONE, ret, "_composer_create_temp_folder() failed! ret:[%d]", ret);

#ifndef _SAVE_IN_USER_SHARE_DIR_
	ret = _composer_create_temp_folder(email_get_composer_public_tmp_dir(),
			composer_util_file_get_public_temp_dirname());
	retvm_if(ret != COMPOSER_ERROR_NONE, ret, "_composer_create_temp_folder() failed! ret:[%d]", ret);
#endif

	debug_leave();
	return COMPOSER_ERROR_NONE;
}

static COMPOSER_ERROR_TYPE_E _composer_initialize_dbus(EmailComposerUGD *ugd)
{
	debug_enter();

	GError *error = NULL;
	if (_g_composer_dbus_conn == NULL) {
		_g_composer_dbus_conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
		if (error) {
			debug_error("g_bus_get_sync() failed (%s)", error->message);
			g_error_free(error);
			return COMPOSER_ERROR_DBUS_FAIL;
		}

		_g_composer_network_id = g_dbus_connection_signal_subscribe(_g_composer_dbus_conn, NULL, "User.Email.NetworkStatus", "email", "/User/Email/NetworkStatus",
													   NULL, G_DBUS_SIGNAL_FLAGS_NONE, _composer_gdbus_signal_receiver_cb, (void *)ugd, NULL);
		retvm_if(_g_composer_network_id == GDBUS_SIGNAL_SUBSCRIBE_FAILURE, COMPOSER_ERROR_DBUS_FAIL, "g_dbus_connection_signal_subscribe() failed!");
	}

	debug_leave();
	return COMPOSER_ERROR_NONE;
}

static COMPOSER_ERROR_TYPE_E _composer_initialize_services(void *data)
{
	debug_enter();

	retvm_if(!data, COMPOSER_ERROR_INVALID_ARG, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	COMPOSER_ERROR_TYPE_E ret = COMPOSER_ERROR_NONE;
	int ct_ret = CONTACTS_ERROR_NONE;

	if (!email_engine_initialize()) {
		debug_error("email_engine_initialize() failed!");
		return COMPOSER_ERROR_ENGINE_INIT_FAILED;
	}

	ct_ret = email_contacts_init_service();
	retvm_if(ct_ret != CONTACTS_ERROR_NONE, COMPOSER_ERROR_SERVICE_INIT_FAIL, "email_contacts_init_service() failed! ct_ret:[%d]", ct_ret);

	ret = _composer_initialize_dbus(ugd);
	retvm_if(ret != COMPOSER_ERROR_NONE, ret, "_composer_initialize_dbus() failed! ret:[%d]", ret);

	ret = _composer_check_storage_full(MIN_FREE_SPACE);
	retvm_if(ret != COMPOSER_ERROR_NONE, ret, "_composer_check_storage_full() failed! ret:[%d]", ret);

	ret = _composer_create_temp_folders();
	retvm_if(ret != COMPOSER_ERROR_NONE, ret, "_composer_create_temp_folders() failed! ret:[%d]", ret);

	debug_leave();
	return ret;
}

static COMPOSER_ERROR_TYPE_E _composer_initialize_themes(void *data)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	ugd->theme = elm_theme_new();
	elm_theme_ref_set(ugd->theme, NULL); /* refer to default theme(NULL) */
	elm_theme_extension_add(ugd->theme, email_get_composer_theme_path()); /* add extension to the new theme */

	debug_leave();
	return COMPOSER_ERROR_NONE;
}

static COMPOSER_ERROR_TYPE_E _composer_construct_composer_data(void *data)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	COMPOSER_ERROR_TYPE_E ret = COMPOSER_ERROR_NONE;

	ret = _composer_initialize_services(ugd);
	gotom_if(ret != COMPOSER_ERROR_NONE, CATCH, "_composer_initialize_services() failed");

	ret = composer_initial_data_pre_parse_arguments(ugd, ugd->ec_app);
	gotom_if(ret != COMPOSER_ERROR_NONE, CATCH, "composer_initial_data_pre_parse_arguments() failed");

	ret = _composer_prepare_vcard(ugd);
	gotom_if(ret != COMPOSER_ERROR_NONE, CATCH, "_composer_construct_mail_info() failed");

	ret = _composer_initialize_account_data(ugd);
	gotom_if(ret != COMPOSER_ERROR_NONE, CATCH, "_composer_construct_mail_info() failed");

	ret = _composer_initialize_mail_info(ugd);
	gotom_if(ret != COMPOSER_ERROR_NONE, CATCH, "_composer_construct_mail_info() failed");

	_composer_add_main_callbacks(ugd);

CATCH:
	debug_leave();
	return ret;
}

static void _composer_remove_temp_folders()
{
	debug_enter();

	if (!email_file_recursive_rm(composer_util_file_get_temp_dirname())) {
		debug_warning("email_file_recursive_rm() failed!");
	}
#ifndef _SAVE_IN_USER_SHARE_DIR_
	if (!email_file_recursive_rm(composer_util_file_get_public_temp_dirname())) {
		debug_warning("email_file_recursive_rm() failed!");
	}
#endif
	debug_leave();
}

static void _composer_finalize_dbus(EmailComposerUGD *ugd)
{
	debug_enter();

	g_dbus_connection_signal_unsubscribe(_g_composer_dbus_conn, _g_composer_network_id);
	g_object_unref(_g_composer_dbus_conn);
	_g_composer_dbus_conn = NULL;
	_g_composer_network_id = 0;

	debug_leave();
}

static void _composer_finalize_services(void *data)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	int ct_ret = CONTACTS_ERROR_NONE;

	_composer_finalize_dbus(ugd);

	ct_ret = email_contacts_finalize_service();
	debug_warning_if(ct_ret != CONTACTS_ERROR_NONE, "email_contacts_finalize_service() failed! ct_ret:[%d]", ct_ret);

	email_engine_finalize();

	debug_leave();
}

static void _composer_destroy_email_info(EmailComposerUGD *ugd)
{
	debug_enter();

	if (ugd->account_info) {
		if (ugd->account_info->account_list) {
			email_free_account(&ugd->account_info->account_list, ugd->account_info->account_count);
			ugd->account_info->account_list = NULL;
		}
		free(ugd->account_info);
		ugd->account_info = NULL;
	}

	if (ugd->org_mail_info) {
		if (ugd->org_mail_info->attachment_list) {
			email_free_attachment_data(&ugd->org_mail_info->attachment_list, ugd->org_mail_info->total_attachment_count);
			ugd->org_mail_info->attachment_list = NULL;
		}

		if (ugd->org_mail_info->mail_data) {
			email_free_mail_data(&ugd->org_mail_info->mail_data, 1);
			ugd->org_mail_info->mail_data = NULL;
		}

		free(ugd->org_mail_info);
		ugd->org_mail_info = NULL;
	}

	if (ugd->new_mail_info) {
		if (ugd->new_mail_info->attachment_list) {
			email_free_attachment_data(&ugd->new_mail_info->attachment_list, ugd->new_mail_info->total_attachment_count);
			ugd->new_mail_info->attachment_list = NULL;
		}

		if (ugd->new_mail_info->mail_data) {
			email_free_mail_data(&ugd->new_mail_info->mail_data, 1);
			ugd->new_mail_info->mail_data = NULL;
		}

		free(ugd->new_mail_info);
		ugd->new_mail_info = NULL;
	}

	g_free(ugd->original_charset);

	debug_leave();
}

static void _composer_destroy_evas_objects(EmailComposerUGD *ugd)
{
	debug_enter();

	composer_webkit_del_callbacks(ugd->ewk_view, ugd);

	if (ugd->thread_saving_email) {
		ecore_thread_cancel(ugd->thread_saving_email);
		ugd->thread_saving_email = NULL;
	}

	if (ugd->thread_resize_image) {
		ecore_thread_cancel(ugd->thread_resize_image);
		ugd->thread_resize_image = NULL;
	}

	DELETE_IDLER_OBJECT(ugd->idler_set_focus);
	DELETE_IDLER_OBJECT(ugd->idler_show_ctx_popup);
	DELETE_IDLER_OBJECT(ugd->idler_destroy_self);
	DELETE_IDLER_OBJECT(ugd->idler_regionshow);
	DELETE_IDLER_OBJECT(ugd->idler_regionbringin);
	DELETE_IDLER_OBJECT(ugd->idler_move_recipient);

	DELETE_TIMER_OBJECT(ugd->timer_ctxpopup_show);
	DELETE_TIMER_OBJECT(ugd->timer_adding_account);
	DELETE_TIMER_OBJECT(ugd->timer_lazy_loading);
	DELETE_TIMER_OBJECT(ugd->timer_regionshow);
	DELETE_TIMER_OBJECT(ugd->timer_regionbringin);
	DELETE_TIMER_OBJECT(ugd->timer_resize_webkit);
	DELETE_TIMER_OBJECT(ugd->timer_destroy_composer_popup);
	DELETE_TIMER_OBJECT(ugd->timer_destory_composer);
	DELETE_TIMER_OBJECT(ugd->timer_duplicate_toast_in_reciepients);
	DELETE_TIMER_OBJECT(ugd->cs_events_timer1);
	DELETE_TIMER_OBJECT(ugd->cs_events_timer2);
	DELETE_TIMER_OBJECT(ugd->vcard_save_timer);

	DELETE_ANIMATOR_OBJECT(ugd->cs_animator);

	/* parent of context_popup is base.win. so it has to be removed here. */
	DELETE_EVAS_OBJECT(ugd->context_popup);

	composer_util_recp_clear_mbe(ugd->recp_to_mbe);
	composer_util_recp_clear_mbe(ugd->recp_cc_mbe);
	composer_util_recp_clear_mbe(ugd->recp_bcc_mbe);

	/* These evas objects regarding recipient entries should be removed here. (they may not be in parent because it is hided when there's no contents)
	 * RelationShip
	 * ugd->recp_to_layout
	 * ugd->recp_to_box
	 * ugd->recp_to_entry_ly, ugd->recp_to_mbe
	 * ugd->recp_to_entry_box, ugd->recp_to_entry_btn
	 * ugd->recp_to_entry_label, ugd->recp_to_entry, ugd->recp_to_display_entry
	 * Deletion should happen in the reverse order
	 */
	DELETE_EVAS_OBJECT(ugd->recp_to_btn);
	DELETE_EVAS_OBJECT(ugd->recp_cc_btn);
	DELETE_EVAS_OBJECT(ugd->recp_bcc_btn);

	DELETE_EVAS_OBJECT(ugd->recp_to_label);
	DELETE_EVAS_OBJECT(ugd->recp_cc_label_cc);
	DELETE_EVAS_OBJECT(ugd->recp_cc_label_cc_bcc);
	DELETE_EVAS_OBJECT(ugd->recp_bcc_label);

	DELETE_EVAS_OBJECT(ugd->recp_to_entry.layout);
	DELETE_EVAS_OBJECT(ugd->recp_cc_entry.layout);
	DELETE_EVAS_OBJECT(ugd->recp_bcc_entry.layout);

	DELETE_EVAS_OBJECT(ugd->recp_to_entry_layout);
	DELETE_EVAS_OBJECT(ugd->recp_cc_entry_layout);
	DELETE_EVAS_OBJECT(ugd->recp_bcc_entry_layout);

	DELETE_EVAS_OBJECT(ugd->recp_to_display_entry.layout);
	DELETE_EVAS_OBJECT(ugd->recp_cc_display_entry.layout);
	DELETE_EVAS_OBJECT(ugd->recp_bcc_display_entry.layout);

	DELETE_EVAS_OBJECT(ugd->recp_to_display_entry_layout);
	DELETE_EVAS_OBJECT(ugd->recp_cc_display_entry_layout);
	DELETE_EVAS_OBJECT(ugd->recp_bcc_display_entry_layout);

	DELETE_EVAS_OBJECT(ugd->recp_to_box);
	DELETE_EVAS_OBJECT(ugd->recp_cc_box);
	DELETE_EVAS_OBJECT(ugd->recp_bcc_box);

	DELETE_EVAS_OBJECT(ugd->recp_to_mbe);
	DELETE_EVAS_OBJECT(ugd->recp_cc_mbe);
	DELETE_EVAS_OBJECT(ugd->recp_bcc_mbe);

	DELETE_EVAS_OBJECT(ugd->recp_to_mbe_layout);
	DELETE_EVAS_OBJECT(ugd->recp_cc_mbe_layout);
	DELETE_EVAS_OBJECT(ugd->recp_bcc_mbe_layout);

	DELETE_EVAS_OBJECT(ugd->recp_to_layout);
	DELETE_EVAS_OBJECT(ugd->recp_cc_layout);
	DELETE_EVAS_OBJECT(ugd->recp_bcc_layout);

	DELETE_EVAS_OBJECT(ugd->main_scroller);

	DELETE_EVAS_OBJECT(ugd->composer_popup);

	debug_leave();
}

/* TODO:: the routines deleting resources should be separated into each view */
static void _composer_destroy_composer(void *data)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	if (ugd->vcard_save_thread) {
		ugd->vcard_save_cancel = EINA_TRUE;
		pthread_join(ugd->vcard_save_thread, NULL);
		ugd->vcard_save_thread = 0;
	}

	/* Restore scroll friction to original one */
	elm_config_scroll_bring_in_scroll_friction_set(ugd->scroll_friction);

	/* Restore the indicator mode. // Other module such like mailbox can destroy composer. */
	composer_util_indicator_restore(ugd);

	composer_attachment_reset_attachment(ugd);
	composer_util_popup_translate_free_variables(ugd);
	_composer_destroy_email_info(ugd);

	elm_theme_extension_del(ugd->theme, email_get_composer_theme_path());
	elm_theme_free(ugd->theme);

	_composer_destroy_evas_objects(ugd);

	/* deinitialize ewk */
	debug_secure("composer_type(%d), saved_html_path(%s), saved_text_path(%s)", ugd->composer_type, ugd->saved_html_path, ugd->saved_text_path);

	FREE(ugd->saved_text_path);
	FREE(ugd->saved_html_path);
	FREE(ugd->plain_content);
	FREE(ugd->final_body_content);
	FREE(ugd->final_parent_content);
	FREE(ugd->final_new_message_content);
	FREE(ugd->initial_body_content);
	FREE(ugd->initial_parent_content);
	FREE(ugd->initial_new_message_content);

	FREE(ugd->eml_file_path);
	FREE(ugd->vcard_file_path);
	FREE(ugd->shared_contact_id_list);

	composer_initial_data_free_initial_contents(ugd);
	_composer_remove_temp_folders();
	_composer_finalize_services(ugd);

	debug_leave();
}

static void _composer_process_error_popup(EmailComposerUGD *ugd, COMPOSER_ERROR_TYPE_E error)
{
	debug_enter();

	switch (error) {
		case COMPOSER_ERROR_NO_DEFAULT_ACCOUNT:
		case COMPOSER_ERROR_NO_ACCOUNT_LIST:
			debug_error("Lauching composer failed! Failed to get account! [%d]", error);
			ugd->composer_popup = composer_util_popup_create(ugd, EMAIL_COMPOSER_STRING_UABLE_TO_COMPOSE_EMAIL_MESSAGE, EMAIL_COMPOSER_STRING_THERE_IS_NO_ACCOUNT_CREATE_A_NEW_ACCOUNT_FIRST,
									_composer_launch_settings_popup_response_cb, EMAIL_COMPOSER_STRING_BUTTON_OK, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);
			break;

		case COMPOSER_ERROR_STORAGE_FULL:
			debug_error("Lauching composer failed! COMPOSER_ERROR_STORAGE_FULL");
			ugd->composer_popup = composer_util_popup_create(ugd, EMAIL_COMPOSER_STRING_UABLE_TO_COMPOSE_EMAIL_MESSAGE, EMAIL_COMPOSER_STRING_STORAGE_FULL_CONTENTS,
									_composer_storage_full_popup_response_cb, EMAIL_COMPOSER_STRING_BUTTON_OK, EMAIL_COMPOSER_STRING_BUTTON_SETTINGS, EMAIL_COMPOSER_STRING_NULL);
			break;

		case COMPOSER_ERROR_VCARD_CREATE_FAILED:
			debug_error("Lauching composer failed! COMPOSER_ERROR_VCARD_CREATE_FAILED");
			ugd->composer_popup = composer_util_popup_create(ugd, EMAIL_COMPOSER_STRING_UABLE_TO_COMPOSE_EMAIL_MESSAGE, EMAIL_COMPOSER_STRING_VCARD_CREATE_FILED,
									composer_util_ug_destroy_cb, EMAIL_COMPOSER_STRING_BUTTON_OK, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);
			break;

		case COMPOSER_ERROR_ENGINE_INIT_FAILED:
		case COMPOSER_ERROR_DBUS_FAIL:
		case COMPOSER_ERROR_SERVICE_INIT_FAIL:
		case COMPOSER_ERROR_FAIL:
		case COMPOSER_ERROR_UNKOWN_TYPE:
		case COMPOSER_ERROR_INVALID_ARG:
		case COMPOSER_ERROR_OUT_OF_MEMORY:
			debug_error("Lauching composer failed! Due to [%d]", error);
			ugd->composer_popup = composer_util_popup_create(ugd, EMAIL_COMPOSER_STRING_UABLE_TO_COMPOSE_EMAIL_MESSAGE, EMAIL_COMPOSER_STRING_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED,
									composer_util_ug_destroy_cb, EMAIL_COMPOSER_STRING_BUTTON_OK, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);
			break;

		default:
			break;
	}

	debug_leave();
}

static void _composer_set_environment_variables()
{
	debug_enter();

	/* To set the cursor size in webkit as email specific size (current size = 2, 2013-06-28) */
	setenv("TIZEN_WEBKIT_CARET_SET", "EMAIL", true);

	debug_leave();
}

static void _composer_add_main_callbacks(EmailComposerUGD *ugd)
{
	debug_enter();

	evas_object_smart_callback_add(ugd->base.module->conform, "virtualkeypad,state,on", _composer_virtualkeypad_state_on_cb, (void *)ugd);
	evas_object_smart_callback_add(ugd->base.module->conform, "virtualkeypad,state,off", _composer_virtualkeypad_state_off_cb, (void *)ugd);
	evas_object_smart_callback_add(ugd->base.module->conform, "virtualkeypad,size,changed", _composer_virtualkeypad_size_changed_cb, (void *)ugd);
	debug_leave();
}

static void _composer_del_main_callbacks(EmailComposerUGD *ugd)
{
	debug_enter();

	evas_object_smart_callback_del(ugd->base.module->conform, "virtualkeypad,state,on", _composer_virtualkeypad_state_on_cb);
	evas_object_smart_callback_del(ugd->base.module->conform, "virtualkeypad,state,off", _composer_virtualkeypad_state_off_cb);
	evas_object_smart_callback_del(ugd->base.module->conform, "virtualkeypad,size,changed", _composer_virtualkeypad_size_changed_cb);

	debug_leave();
}

static void _composer_initial_view_draw_richtext_components_if_enabled(EmailComposerUGD *ugd)
{
	Eina_Bool richtext_toolbar_enabled = EINA_TRUE;
	if (email_get_richtext_toolbar_status(&richtext_toolbar_enabled) != 0) { /* It does not exist */
		richtext_toolbar_enabled = EINA_FALSE;
		email_set_richtext_toolbar_status(richtext_toolbar_enabled);
	}
	if (true == richtext_toolbar_enabled) {
		composer_initial_view_draw_richtext_components(ugd);
	}
}

static int _composer_module_create(email_module_t *self, app_control_h params)
{
	debug_enter();
	retvm_if(!self, -1, "Invalid parameter: self is NULL!");

	EmailComposerModule *md = (EmailComposerModule *)self;

	app_control_clone(&md->view.ec_app, params);

	md->view.eComposerErrorType = composer_initial_data_parse_composer_run_type(&md->view, params);
	retvm_if(md->view.eComposerErrorType != COMPOSER_ERROR_NONE, -1, "composer_initial_data_parse_composer_run_type() failed");

	md->view.base.create = _composer_create;
	md->view.base.destroy = _composer_destroy;
	md->view.base.activate = _composer_activate;
	md->view.base.deactivate = _composer_deactivate;
	md->view.base.update = _composer_update;
	md->view.base.on_back_key = _composer_on_back_key;

	int ret = email_module_create_view(&md->base, &md->view.base);
	if (ret != 0) {
		debug_error("email_module_create_view(): failed (%d)", ret);
		return -1;
	}

	debug_leave();
	return 0;
}

static int _composer_create(email_view_t *self)
{
	debug_enter();
	retvm_if(!self, -1, "self is NULL");

	EmailComposerUGD *ugd = (EmailComposerUGD *)self;

	bindtextdomain(PACKAGE, email_get_locale_dir());

	ugd->eComposerErrorType = _composer_initialize_themes(ugd);
	gotom_if(ugd->eComposerErrorType != COMPOSER_ERROR_NONE, CATCH, "_composer_initialize_themes() failed");

	ugd->eComposerErrorType = _composer_initialize_composer_data(ugd);
	gotom_if(ugd->eComposerErrorType != COMPOSER_ERROR_NONE, CATCH, "_composer_initialize_composer_data() failed");

	ugd->eComposerErrorType = _composer_load_account_list(ugd);
	gotom_if(ugd->eComposerErrorType != COMPOSER_ERROR_NONE, CATCH, "_composer_load_account_list() failed");

	/* NOTE: If you need lazy loading concepts, uncomment these lines. */
	/*if (ugd->composer_type == RUN_COMPOSER_EXTERNAL) {
		ugd->need_lazy_loading = EINA_TRUE;
	}*/

	if (!ugd->need_lazy_loading) {
		ugd->eComposerErrorType = _composer_construct_composer_data(ugd);
		gotom_if(ugd->eComposerErrorType != COMPOSER_ERROR_NONE, CATCH, "_composer_construct_composer_data() failed");
	}

CATCH:
	composer_initial_view_draw_base_frame(ugd);
	composer_initial_view_draw_header_components(ugd);
	if (!ugd->need_lazy_loading) {
		composer_initial_view_draw_remaining_components(ugd);

		/* 1. Not to show IME immediately. (to mbe entry gets the focus when on_create() finishes.)
		 * 2. Highlight focus moves from attachment icon to to field while entering composer.
		 * - Solution: Makes 4 buttons on naviframe unfocusable. In _webkit_js_execute_set_focus_cb(), they will be changed as focusable button.
		 */
		elm_object_tree_focus_allow_set(ugd->composer_layout, EINA_FALSE);
	}

	composer_util_indicator_show(ugd);

	_composer_initial_view_draw_richtext_components_if_enabled(ugd);

	debug_leave();
	return 0;
}

static void _composer_start(EmailComposerUGD *ugd)
{
	debug_enter();

	retm_if(!ugd, "Invalid parameter: ugd is NULL!");

	if (ugd->eComposerErrorType != COMPOSER_ERROR_NONE) {
		_composer_process_error_popup(ugd, ugd->eComposerErrorType);
		debug_leave();
		return;
	}

	if (ugd->need_lazy_loading) {
		_composer_create_delay_loading_popup(ugd);

		/* If we use idler here, sometimes the popup couldn't be displayed properly. */
		ugd->timer_lazy_loading = ecore_timer_add(0.2f, _composer_delayed_drawing_timer_cb, ugd);
	} else {
		composer_initial_data_set_mail_info(ugd, true);
		composer_initial_data_post_parse_arguments(ugd, ugd->ec_app);
		composer_initial_data_make_initial_contents(ugd);
	}

	debug_leave();
}

static void _composer_deactivate(email_view_t *self)
{
	debug_enter();

	retm_if(!self, "Invalid parameter: self is NULL!");
	EmailComposerUGD *ugd = (EmailComposerUGD *)self;

	if (ugd->is_need_close_composer_on_low_memory) {
		ugd->timer_destory_composer = ecore_timer_add(0.1f, _composer_destroy_timer_cb, ugd);

	}
	if (ugd->selected_entry == ugd->ewk_view) {
		evas_object_focus_set(ugd->ewk_view, EINA_FALSE);
	}

	ugd->is_hided = EINA_TRUE;

	debug_leave();
}

static void _composer_activate(email_view_t *self, email_view_state prev_state)
{
	debug_enter();

	retm_if(!self, "Invalid parameter: self is NULL!");
	EmailComposerUGD *ugd = (EmailComposerUGD *)self;

	if (prev_state == EV_STATE_CREATED) {
		_composer_start(ugd);
		return;
	}

	ret_if(!(ugd->is_load_finished || ugd->is_adding_account_requested));

	ugd->is_hided = EINA_FALSE;
	_composer_contacts_update_recp_info_for_recipients(ugd);
	if (ugd->need_to_set_focus_on_resume) {
		composer_util_focus_set_focus(ugd, ugd->selected_entry);
		ugd->need_to_set_focus_on_resume = EINA_FALSE;
	}

	if (evas_object_freeze_events_get(ugd->ewk_view)) {
		evas_object_freeze_events_set(ugd->ewk_view, EINA_FALSE);
	}

	if (ugd->is_adding_account_requested) {
		ugd->is_adding_account_requested = EINA_FALSE;
		DELETE_TIMER_OBJECT(ugd->timer_adding_account);
		ugd->timer_adding_account = ecore_timer_add(0.01f, _composer_update_data_after_adding_account_timer_cb, ugd);
	}

	if (!ugd->composer_popup) {
		composer_util_focus_set_focus_with_idler(ugd, ugd->selected_entry);
	}

	debug_leave();
}

static void _composer_destroy(email_view_t *self)
{
	debug_enter();

	retm_if(!self, "Invalid parameter: self is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)self;
	app_control_h reply_service;

	int ret = app_control_create(&reply_service);
	if (ret == APP_CONTROL_ERROR_NONE) {
		ret = email_module_send_result(ugd->base.module, reply_service);
		debug_warning_if(ret != 0, "email_module_send_result() failed!");

		ret = app_control_destroy(reply_service);
		debug_warning_if(ret != APP_CONTROL_ERROR_NONE, "app_control_destroy() failed!");
	} else {
		debug_warning("app_control_create() failed!");
	}

	app_control_destroy(ugd->ec_app);

	ugd->is_composer_getting_destroyed = EINA_TRUE;
	_composer_del_main_callbacks(ugd);
	_composer_destroy_composer(ugd);

	debug_leave();
}

static void _composer_on_back_key(email_view_t *self)
{
	debug_enter();

	retm_if(!self, "self is NULL");
	EmailComposerUGD *ugd = (EmailComposerUGD *)self;

	composer_initial_view_back_cb(ugd);

	debug_leave();
	return;
}

static void _composer_on_message(email_module_t *self, app_control_h msg)
{
	debug_enter();

	retm_if(!self, "self is NULL");

	EmailComposerModule *md = (EmailComposerModule *)self;
	EmailComposerUGD *ugd = &md->view;
	char *msg_type = NULL;

	int ret = app_control_get_extra_data(msg, EMAIL_BUNDLE_KEY_MSG, &msg_type);
	debug_warning_if(ret != APP_CONTROL_ERROR_NONE, "app_control_get_extra_data(%s) failed! (%d)", EMAIL_BUNDLE_KEY_MSG, ret);

	if (g_strcmp0(msg_type, EMAIL_BUNDLE_VAL_EMAIL_COMPOSER_SAVE_DRAFT) == 0) {

		email_module_terminate_any_launched_app(ugd->base.module, false);

		ugd->is_force_destroy = EINA_TRUE;
		if (ugd->is_load_finished) {
			ugd->is_save_in_drafts_clicked = EINA_TRUE; /* The action should be just like clicking save in drafts. */
			composer_exit_composer_get_contents(ugd); /* Exit out of composer without any pop-up. */
		} else {
			ugd->need_to_destroy_after_initializing = EINA_TRUE;
		}
	}
	FREE(msg_type);

	debug_leave();
}

static void _composer_on_event(email_module_t *self, email_module_event_e event)
{
	debug_enter();

	EmailComposerModule *md = (EmailComposerModule *)self;
	EmailComposerUGD *ugd = &md->view;

	switch (event) {
	case EM_EVENT_LOW_MEMORY_SOFT:
		debug_log("Low memory: %d", event);
		composer_webkit_handle_mem_warning(ugd, EINA_FALSE);
		break;
	case EM_EVENT_LOW_MEMORY_HARD:
		debug_log("Low memory: %d", event);
		composer_webkit_handle_mem_warning(ugd, EINA_TRUE);
		break;
	default:
		break;
	}

	debug_leave();
}

static void _composer_language_change_update(EmailComposerUGD *ugd)
{
	debug_enter();
	retm_if(!ugd, "Invalid parameter: self is NULL!");

	if (ugd->recp_from_label) { composer_recipient_label_text_set(ugd->recp_from_label, COMPOSER_RECIPIENT_TYPE_FROM); }
	if (ugd->recp_to_label) { composer_recipient_label_text_set(ugd->recp_to_label, COMPOSER_RECIPIENT_TYPE_TO); }
	if (ugd->recp_cc_label_cc) { composer_recipient_label_text_set(ugd->recp_cc_label_cc, COMPOSER_RECIPIENT_TYPE_CC); }
	if (ugd->recp_cc_label_cc_bcc) { composer_recipient_label_text_set(ugd->recp_cc_label_cc_bcc, COMPOSER_RECIPIENT_TYPE_CC_BCC); }
	if (ugd->recp_bcc_label) { composer_recipient_label_text_set(ugd->recp_bcc_label, COMPOSER_RECIPIENT_TYPE_BCC); }

	if (ugd->recp_to_mbe) { composer_recipient_label_text_set(ugd->recp_to_mbe, COMPOSER_RECIPIENT_TYPE_TO); }
	if (ugd->recp_cc_mbe) {
		if (ugd->bcc_added) {
			composer_recipient_label_text_set(ugd->recp_cc_mbe, COMPOSER_RECIPIENT_TYPE_CC);
		} else {
			composer_recipient_label_text_set(ugd->recp_cc_mbe, COMPOSER_RECIPIENT_TYPE_CC_BCC);
		}
	}
	if (ugd->recp_bcc_mbe) { composer_recipient_label_text_set(ugd->recp_bcc_mbe, COMPOSER_RECIPIENT_TYPE_BCC); }
	if (ugd->attachment_group_layout) { composer_attachment_ui_group_update_text(ugd); }

	if (ugd->with_original_message) {
		char *to_be_updated_original_message_bar = g_strdup_printf(EC_JS_UPDATE_TEXT_TO_ORIGINAL_MESSAGE_BAR, email_get_email_string("IDS_EMAIL_BODY_INCLUDE_ORIGINAL_MESSAGE"));
		if (!ewk_view_script_execute(ugd->ewk_view, to_be_updated_original_message_bar, NULL, NULL)) {
			debug_error("EC_JS_UPDATE_TEXT_TO_ORIGINAL_MESSAGE_BAR failed.");
		}
		g_free(to_be_updated_original_message_bar);
	}

	if (ugd->subject_entry.entry) {
		composer_subject_update_guide_text(ugd);
	}

	/* This code is specific for popups where string translation is not handled by EFL */
	if (ugd->composer_popup && ugd->pt_element_type) { composer_util_popup_translate_do(ugd); }

	debug_leave();
}

static void _composer_orientation_change_update(EmailComposerUGD *ugd)
{
	debug_enter();
	retm_if(!ugd, "Invalid parameter: self is NULL!");

	switch (ugd->base.orientation) {
	case APP_DEVICE_ORIENTATION_0:
		ugd->is_horizontal = EINA_FALSE;
		break;
	case APP_DEVICE_ORIENTATION_90:
		ugd->is_horizontal = EINA_TRUE;
		break;
	case APP_DEVICE_ORIENTATION_180:
		ugd->is_horizontal = EINA_FALSE;
		break;
	case APP_DEVICE_ORIENTATION_270:
		ugd->is_horizontal = EINA_TRUE;
		break;
	}

	if (ugd->recp_from_ctxpopup) {
		DELETE_EVAS_OBJECT(ugd->recp_from_ctxpopup);
	}

	composer_recipient_update_display_string(ugd, ugd->recp_to_mbe, ugd->recp_to_entry.entry, ugd->recp_to_display_entry.entry, ugd->to_recipients_cnt);
	composer_recipient_update_display_string(ugd, ugd->recp_cc_mbe, ugd->recp_cc_entry.entry, ugd->recp_cc_display_entry.entry, ugd->cc_recipients_cnt);
	composer_recipient_update_display_string(ugd, ugd->recp_bcc_mbe, ugd->recp_bcc_entry.entry, ugd->recp_bcc_display_entry.entry, ugd->bcc_recipients_cnt);
	ret_if(!ugd->is_load_finished);

	composer_initial_view_set_combined_scroller_rotation_mode(ugd);

	/* Timer should be here to bringin the selected entry.
	 * If we don't use timer here, bringin function doesn't work properly.
	 */
	DELETE_TIMER_OBJECT(ugd->timer_regionshow);
	if (ugd->selected_entry != ugd->ewk_view) {
		ugd->timer_regionshow = ecore_timer_add(0.2f, composer_util_scroll_region_show_timer, ugd);
	}

	/* If there's a popup which has list or genlist, we should resize it to fit to the desired size. */
	if (ugd->composer_popup) {
		composer_util_popup_resize_popup_for_rotation(ugd->composer_popup, ugd->is_horizontal);
	}

	debug_leave();
}

static void _composer_update(email_view_t *self, int flags)
{
	debug_enter();

	retm_if(!self, "Invalid parameter: self is NULL!");
	EmailComposerUGD *ugd = (EmailComposerUGD *)self;

	if (flags & EVUF_ORIENTATION_CHANGED) {
		_composer_orientation_change_update(ugd);
	}
	if (flags & EVUF_LANGUAGE_CHANGED) {
		_composer_language_change_update(ugd);
	}

	debug_leave();
}

static void _composer_virtualkeypad_state_on_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	if (ugd->ps_box) {
		composer_ps_change_layout_size(ugd);
	}

	debug_leave();
}

static void _composer_virtualkeypad_state_off_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	/* When user rotate device with "Stop composing email" popup, this callback is called even though the popup has already shown. */
	if (ugd->is_back_btn_clicked && !ugd->composer_popup) {
		composer_exit_composer_get_contents(ugd);
	}

	if (ugd->ps_box) {
		composer_ps_change_layout_size(ugd);
	}

	debug_leave();
}

static void _composer_virtualkeypad_size_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	Evas_Coord_Rectangle *rect = (Evas_Coord_Rectangle *)event_info;

	debug_log("[x,y = %d,%d] - [w,h = %d,%d]", rect->x, rect->y, rect->w, rect->h);

	composer_util_resize_min_height_for_new_message(ugd, rect->h);

	if (ugd->ps_box) {
		composer_ps_change_layout_size(ugd);
	}

	debug_leave();
}

static void _composer_gdbus_signal_receiver_cb(GDBusConnection *connection,
											const gchar *sender_name,
											const gchar *object_path,
											const gchar *interface_name,
											const gchar *signal_name,
											GVariant *parameters,
											gpointer data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");
	debug_secure("Object path=%s, interface name=%s, signal name = %s", object_path, interface_name, signal_name);

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	if (!(g_strcmp0(object_path, "/User/Email/NetworkStatus")) && !(g_strcmp0(signal_name, "email"))) {
		int subtype = 0;
		int data1 = 0;
		char *data2 = NULL;
		int data3 = 0;
		int data4 = 0;

		g_variant_get(parameters, "(iisii)", &subtype, &data1, &data2, &data3, &data4);
		debug_log("NETWORK : subtype:[%d], data1:[%d], data2:(%s), data3:[%d], data4:[%d]", subtype, data1, data2, data3, data4);

		switch (subtype) {
			case NOTI_DOWNLOAD_BODY_FINISH:
				/* DATA1[mail_id] DATA2[NULL] DATA3[handle_id] */
				debug_log("receive noti, DOWNLOAD_BODY_FINISH");
				if (ugd->draft_sync_handle == data3 && ugd->org_mail_info && ugd->org_mail_info->mail_data && ugd->org_mail_info->mail_data->mail_id == data1) {
					composer_initial_data_destroy_download_contents_popup(ugd); /* delete progress popup */
					/* Below function should also get called after download is finished
					 * First data should be initialized after download, then composer_initial_data_set_mail_info should be called
					 * This function will also get mail data and attachment list
					 * If we do not do this parsing function for smime attachment is not called and details are not displayed for the
					 * first time after draft mail is downloaded from server, even though data is updated
					 */
					_composer_initialize_mail_info(ugd);
					composer_initial_data_set_mail_info(ugd, false);
				}
				break;

			case NOTI_DOWNLOAD_BODY_FAIL:
				/* DATA1[mail_id] DATA2[NULL] DATA3[handle_id] */
				debug_log("receive noti, DOWNLOAD_BODY_FAIL");
				if (ugd->draft_sync_handle == data3 && ugd->org_mail_info && ugd->org_mail_info->mail_data && ugd->org_mail_info->mail_data->mail_id == data1) {
					composer_initial_data_destroy_download_contents_popup(ugd); /* delete progress popup */

					char *err_msg = composer_util_get_error_string(data4);
					int noti_ret = notification_status_message_post(err_msg);
					debug_warning_if(noti_ret != NOTIFICATION_ERROR_NONE, "notification_status_message_post() failed! ret:(%d)", noti_ret);
					g_free(err_msg);

					composer_initial_data_set_mail_info(ugd, false);
				}
				break;

			case NOTI_DOWNLOAD_ATTACH_FINISH:
				/* DATA1[mail_id] DATA2[NULL] DATA3[index] */
				debug_log("receive noti, DOWNLOAD_ATTACH_FINISH");
				if (ugd->downloading_attachment && (ugd->downloading_attachment->mail_id == data1) && (ugd->downloading_attachment_index == data3)) {
					email_attachment_data_t *new_attachment_list = NULL;
					int new_attachment_count = 0;
					int ret = EMAIL_ERROR_NONE;

					/* 1-1. Update attachment info for existing mail */
					if (ugd->downloading_attachment->mail_id == ugd->org_mail_info->mail_data->mail_id) {
						ret = email_get_attachment_data_list(ugd->org_mail_info->mail_data->mail_id, &new_attachment_list, &new_attachment_count);
					/* 1-2. Update attachment info for reference mail */
					} else if (ugd->downloading_attachment->mail_id == ugd->org_mail_info->mail_data->reference_mail_id) {
						ret = email_get_attachment_data_list(ugd->org_mail_info->mail_data->reference_mail_id, &new_attachment_list, &new_attachment_count);
					}

					if (ret == EMAIL_ERROR_NONE) {
						/* 2. Update attachment info for new mail. */
						int i = 0;
						email_attachment_data_t *curr_att = NULL;
						for (i = 0; i < new_attachment_count; i++) {
							curr_att = new_attachment_list + i;
							if (curr_att->attachment_id == ugd->downloading_attachment->attachment_id) {
								FREE(ugd->downloading_attachment->attachment_path);
								FREE(ugd->downloading_attachment->attachment_mime_type);

								ugd->downloading_attachment->attachment_path = COMPOSER_STRDUP(curr_att->attachment_path);
								ugd->downloading_attachment->attachment_mime_type = COMPOSER_STRDUP(curr_att->attachment_mime_type);
								ugd->downloading_attachment->attachment_size = curr_att->attachment_size;
								ugd->downloading_attachment->save_status = curr_att->save_status;
								break;
							}
						}

						if (ugd->downloading_attachment->mail_id == ugd->org_mail_info->mail_data->mail_id) {
							ret = email_free_attachment_data(&ugd->org_mail_info->attachment_list, ugd->org_mail_info->total_attachment_count);
							debug_warning_if(ret != EMAIL_ERROR_NONE, "email_free_attachment_data() failed! It'll cause a memory leak!");

							ugd->org_mail_info->attachment_list = new_attachment_list;
							ugd->org_mail_info->total_attachment_count = new_attachment_count;
						} else {
							ret = email_free_attachment_data(&new_attachment_list, new_attachment_count);
							debug_warning_if(ret != EMAIL_ERROR_NONE, "email_free_attachment_data() failed! It'll cause a memory leak!");
						}
						new_attachment_list = NULL;
						new_attachment_count = 0;

						/* 3. Update thumbnail and flag */
						Eina_List *list = NULL;
						ComposerAttachmentItemData *attachment_item_data = NULL;
						Eina_Bool is_needed_to_download = EINA_FALSE;

						EINA_LIST_FOREACH(ugd->attachment_item_list, list, attachment_item_data) {
							email_attachment_data_t *att = attachment_item_data->attachment_data;
							if (att->attachment_id == ugd->downloading_attachment->attachment_id) {
								if (ugd->composer_popup) {
									composer_util_popup_response_cb(ugd, ugd->composer_popup, NULL);
								}
								/* Update thumbnail for the attachment */
								composer_attachment_ui_update_item(attachment_item_data);
								composer_util_show_preview(attachment_item_data);
								att->attachment_id = 0;
							} else if ((att->attachment_id > 0) && (att->save_status == 0)) {
								is_needed_to_download = EINA_TRUE;
							}
						}

						/* Update need_download flag. */
						ugd->need_download = is_needed_to_download; /* this attachment is from existing mail, but it hasn't been downloaded yet. */
					} else {
						debug_error("email_get_attachment_data_list() failed! ret:[%d]", ret);
						ugd->composer_popup = composer_util_popup_create(ugd, EMAIL_COMPOSER_STRING_HEADER_UNABLE_TO_DOWNLOAD_ATTACHMENT_ABB, EMAIL_COMPOSER_STRING_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED,
													composer_util_popup_response_cb, EMAIL_COMPOSER_STRING_BUTTON_OK, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);
					}

					composer_attachment_download_attachment_clear_flags(ugd);
				}
				break;

			case NOTI_DOWNLOAD_ATTACH_FAIL:
				/* DATA1[mail_id] DATA2[NULL] DATA3[attachment_index] */
				debug_log("receive noti, DOWNLOAD_ATTACH_FAIL");
				if (ugd->downloading_attachment && (ugd->downloading_attachment->mail_id == data1) && (ugd->downloading_attachment_index == data3)) {
					composer_attachment_download_attachment_clear_flags(ugd);

					char *err_msg = composer_util_get_error_string(data4);
					int noti_ret = notification_status_message_post(err_msg);
					debug_warning_if(noti_ret != NOTIFICATION_ERROR_NONE, "notification_status_message_post() failed! ret:(%d)", noti_ret);
					g_free(err_msg);
				}
				break;

			default:
				debug_log("unsupported noti type: [%d]", subtype);
				break;
		}
		g_free(data2);
	} else {
		debug_error("The signal received has WRONG TYPE!");
	}

	debug_leave();
}

static void _composer_set_charset_info(void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	retm_if(!ugd->org_mail_info || !ugd->org_mail_info->mail_data, "Invalid parameter: data is NULL!");

	if (ugd->org_mail_info->mail_data->file_path_html) {
		debug_log("[HTML body]");
		ugd->original_charset = email_parse_get_filename_from_path(ugd->org_mail_info->mail_data->file_path_html);
	} else if (ugd->org_mail_info->mail_data->file_path_plain) {
		debug_log("[TEXT body]");
		ugd->original_charset = email_parse_get_filename_from_path(ugd->org_mail_info->mail_data->file_path_plain);
	}
}

static void _composer_contacts_update_recp_info_for_mbe(EmailComposerUGD *ugd, Evas_Object *mbe)
{
	debug_enter();

	Elm_Object_Item *item = elm_multibuttonentry_first_item_get(mbe);
	while (item) {
		EmailRecpInfo *ri = (EmailRecpInfo *)elm_object_item_data_get(item);
		if (!ri) {
			debug_warning("elm_object_item_data_get() failed! ri is NULL!");
			continue;
		}

		EmailAddressInfo *selected_ai = (EmailAddressInfo *)eina_list_nth(ri->email_list, ri->selected_email_idx);
		if (!selected_ai) {
			debug_warning("selected_ai is NULL!");
			continue;
		}
		email_contact_list_info_t *contact_info = NULL;
		contact_info = email_contacts_get_contact_info_by_email_id(ri->email_id);
		if(!contact_info) {
			debug_log("Contact no longer listed in db");
			ri->email_id = 0;
			char *address = strdup(selected_ai->address);
			EmailAddressInfo *ai = NULL;
			EINA_LIST_FREE(ri->email_list, ai) {
				FREE(ai);
			}
			g_free(ri->display_name);
			ri = composer_util_recp_make_recipient_info(address);
			elm_object_item_text_set(item, ri->display_name);
			free(address);
		} else {
			g_free(ri->display_name);
			if (!contact_info->display_name[0]) {
				elm_object_item_text_set(item, selected_ai->address);
				ri->display_name = g_strdup(selected_ai->address);
			} else {
				char *temp = elm_entry_utf8_to_markup(contact_info->display_name);
				ri->display_name = g_strdup(contact_info->display_name);
				elm_object_item_text_set(item, temp);
				free(temp);
			}

			email_contacts_delete_contact_info(contact_info);
		}

		elm_object_item_data_set(item, ri);
		item = elm_multibuttonentry_item_next_get(item);

	}

	debug_leave();
}

static void _composer_contacts_update_recp_info_for_recipients(EmailComposerUGD *ugd)
{
	debug_enter();

	if (ugd->recp_to_mbe) {
		_composer_contacts_update_recp_info_for_mbe(ugd, ugd->recp_to_mbe);
	}
	if (ugd->recp_cc_mbe) {
		_composer_contacts_update_recp_info_for_mbe(ugd, ugd->recp_cc_mbe);
	}
	if (ugd->recp_bcc_mbe) {
		_composer_contacts_update_recp_info_for_mbe(ugd, ugd->recp_bcc_mbe);
	}

	debug_leave();
}

/*
 * Definition for exported functions
 */
EMAIL_API email_module_t *email_module_alloc()
{
	debug_enter();

	_composer_set_environment_variables();

	EmailComposerModule *md = MEM_ALLOC(md, 1);
	if (!md) {
		debug_error("md is NULL");
		return NULL;
	}

	md->base.create = _composer_module_create;
	md->base.on_message = _composer_on_message;
	md->base.on_event = _composer_on_event;

	debug_leave();
	return &md->base;
}
