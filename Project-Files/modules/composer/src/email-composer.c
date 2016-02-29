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
static void _composer_create_delay_loading_popup(EmailComposerView *view);

static COMPOSER_ERROR_TYPE_E _composer_check_storage_full(unsigned int mb_size);
static COMPOSER_ERROR_TYPE_E _composer_load_account_list(void *data);
static COMPOSER_ERROR_TYPE_E _composer_initialize_mail_info(void *data);
static COMPOSER_ERROR_TYPE_E _composer_initialize_account_data(void *data);
static COMPOSER_ERROR_TYPE_E _composer_prepare_vcard(void *data);
static COMPOSER_ERROR_TYPE_E _composer_initialize_composer_data(void *data);
static COMPOSER_ERROR_TYPE_E _composer_create_temp_folder(const char *root_tmp_folder, const char *tmp_folder);
static COMPOSER_ERROR_TYPE_E _composer_create_temp_folders();
static COMPOSER_ERROR_TYPE_E _composer_make_composer_tmp_dir(const char *path);
static COMPOSER_ERROR_TYPE_E _composer_initialize_dbus(EmailComposerView *view);
static COMPOSER_ERROR_TYPE_E _composer_initialize_services(void *data);
static COMPOSER_ERROR_TYPE_E _composer_initialize_themes(void *data);
static COMPOSER_ERROR_TYPE_E _composer_construct_composer_data(void *data);

static void *_composer_vcard_save_worker(void *data);
static Eina_Bool _composer_vcard_save_timer_cb(void *data);

static void _composer_remove_temp_folders();
static void _composer_finalize_dbus(EmailComposerView *view);
static void _composer_finalize_services(void *data);
static void _composer_destroy_email_info(EmailComposerView *view);
static void _composer_destroy_evas_objects(EmailComposerView *view);
static void _composer_destroy_composer(void *data);

static void _composer_process_error_popup(EmailComposerView *view, COMPOSER_ERROR_TYPE_E error);

static void _composer_set_environment_variables();
static void _composer_add_main_callbacks(EmailComposerView *view);
static void _composer_del_main_callbacks(EmailComposerView *view);

static int _composer_module_create(email_module_t *self, email_params_h params);
static void _composer_on_message(email_module_t *self, email_params_h msg);
static void _composer_on_event(email_module_t *self, email_module_event_e event);

static int _composer_create(email_view_t *self);
static void _composer_destroy(email_view_t *self);
static void _composer_activate(email_view_t *self, email_view_state prev_state);
static void _composer_deactivate(email_view_t *self);
static void _composer_update(email_view_t *self, int flags);
static void _composer_on_back_key(email_view_t *self);

static void _composer_start(EmailComposerView *view);

static void _composer_orientation_change_update(EmailComposerView *view);
static void _composer_language_change_update(EmailComposerView *view);

static void _composer_virtualkeypad_state_on_cb(void *data, Evas_Object *obj, void *event_info);
static void _composer_virtualkeypad_state_off_cb(void *data, Evas_Object *obj, void *event_info);
static void _composer_virtualkeypad_size_changed_cb(void *data, Evas_Object *obj, void *event_info);
static void _composer_gdbus_signal_receiver_cb(GDBusConnection *connection, const gchar *sender_name, const gchar *object_path,
		const gchar *interface_name, const gchar *signal_name, GVariant *parameters, gpointer data);

static void _composer_set_charset_info(void *data);
static void _composer_contacts_update_recp_info_for_mbe(EmailComposerView *view, Evas_Object *mbe);
static void _composer_contacts_update_recp_info_for_recipients(EmailComposerView *view);

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

	EmailComposerView *view = (EmailComposerView *)data;

	view->timer_destory_composer = NULL;
	DELETE_EVAS_OBJECT(view->composer_popup);

	/* Restore the indicator mode. */
	composer_util_indicator_restore(view);

	email_module_make_destroy_request(view->base.module);

	debug_leave();
	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _composer_delayed_drawing_timer_cb(void *data)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	view->timer_lazy_loading = NULL;

	view->eComposerErrorType = _composer_construct_composer_data(view);

	gotom_if(view->eComposerErrorType != COMPOSER_ERROR_NONE, CATCH, "_composer_construct_composer_data() failed");

	composer_initial_view_draw_remaining_components(view);

	composer_initial_data_set_mail_info(view, true);
	composer_initial_data_post_parse_arguments(view, view->launch_params);
	composer_initial_data_make_initial_contents(view);

CATCH:
	if (view->eComposerErrorType != COMPOSER_ERROR_NONE) {
		_composer_process_error_popup(view, view->eComposerErrorType);
	}

	debug_leave();
	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _composer_update_data_after_adding_account_timer_cb(void *data)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	view->timer_adding_account = NULL;

	view->eComposerErrorType = _composer_load_account_list(view);
	gotom_if(view->eComposerErrorType != COMPOSER_ERROR_NONE, CATCH, "_composer_load_account_list() failed");

	view->eComposerErrorType = _composer_construct_composer_data(view);
	gotom_if(view->eComposerErrorType != COMPOSER_ERROR_NONE, CATCH, "_composer_construct_composer_data() failed");

	composer_initial_data_set_mail_info(view, true);
	composer_initial_data_post_parse_arguments(view, view->launch_params);
	composer_initial_data_make_initial_contents(view);

CATCH:
	if (view->eComposerErrorType != COMPOSER_ERROR_NONE) {
		_composer_process_error_popup(view, view->eComposerErrorType);
	}

	debug_leave();
	return ECORE_CALLBACK_CANCEL;
}

static void _composer_setting_destroy_request(void *data, email_module_h module)
{
	debug_enter();

	EmailComposerView *view = data;
	int account_id = 0;

	if (!email_engine_get_default_account(&account_id)) {
		debug_log("No account was registered. Terminating...");
		composer_util_indicator_restore(view);
		email_module_make_destroy_request(view->base.module);
	} else {
		debug_log("added account id: [%d]", account_id);
		view->is_adding_account_requested = EINA_TRUE;
		view->selected_entry = view->recp_to_entry.entry;
		email_module_destroy(module);
	}

	debug_leave();
}

static void _composer_launch_settings_popup_response_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerView *view = data;
	email_params_h params = NULL;
	email_module_h module = NULL;

	if ((obj != view->composer_popup) &&
		email_params_create(&params) &&
		email_params_add_str(params, EMAIL_BUNDLE_KEY_VIEW_TYPE, EMAIL_BUNDLE_VAL_VIEW_ACCOUNT_SETUP)) {

		email_module_listener_t listener = { 0 };
		listener.cb_data = view;
		listener.destroy_request_cb = _composer_setting_destroy_request;

		module = email_module_create_child(view->base.module, EMAIL_MODULE_SETTING, params, &listener);
	}

	email_params_free(&params);

	if (!module) {
		debug_error("Terminating...");
		composer_util_indicator_restore(view);
		email_module_make_destroy_request(view->base.module);
	} else {
		DELETE_EVAS_OBJECT(view->composer_popup);
	}

	debug_leave();
}

static void _composer_create_delay_loading_popup(EmailComposerView *view)
{
	debug_enter();

	/* TODO: The loading progress couldn't be displayed smoothly. So we cannot use progress popup here.
	 * If we have to use progress popup, we need to refactoring the routine which draws elm widgets.
	 */
	view->composer_popup = composer_util_popup_create_with_progress_horizontal(view, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_TPOP_LOADING_ING, NULL,
											EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);
	view->is_loading_popup = EINA_TRUE;

	debug_leave();
}

static void _composer_storage_full_popup_response_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;
	char *button_text = NULL;

	/* To distinguish soft button and hardware back button */
	if (obj != view->composer_popup) {
		button_text = (char *)elm_object_text_get(obj);
	}

	if (!g_strcmp0(button_text, email_get_email_string(EMAIL_COMPOSER_STRING_BUTTON_SETTINGS.id))) {
		composer_launcher_launch_storage_settings(view);
		view->is_need_close_composer_on_low_memory = EINA_TRUE;
	} else {
		DELETE_EVAS_OBJECT(view->composer_popup);

		/* Restore the indicator mode. */
		composer_util_indicator_restore(view);

		email_module_make_destroy_request(view->base.module);
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
	EmailComposerView *view = (EmailComposerView *)data;

	/* Get mail data */
	if (view->composer_type == RUN_COMPOSER_EDIT || view->composer_type == RUN_COMPOSER_REPLY || view->composer_type == RUN_COMPOSER_REPLY_ALL || view->composer_type == RUN_COMPOSER_FORWARD) {
		err = email_get_mail_data(view->original_mail_id, &view->org_mail_info->mail_data);
		retvm_if(err != EMAIL_ERROR_NONE, COMPOSER_ERROR_SERVICE_INIT_FAIL, "email_get_mail_data() failed! err:[%d]", err);

		err = email_get_attachment_data_list(view->original_mail_id, &view->org_mail_info->attachment_list, &view->org_mail_info->total_attachment_count);
		retvm_if(err != EMAIL_ERROR_NONE, COMPOSER_ERROR_SERVICE_INIT_FAIL, "email_get_attachment_data_list() failed! err:[%d]", err);

		debug_warning_if(view->org_mail_info->mail_data->attachment_count+view->org_mail_info->mail_data->inline_content_count != view->org_mail_info->total_attachment_count,
				"ATTACHMENT count is different! [%d != %d]", view->org_mail_info->mail_data->attachment_count+view->org_mail_info->mail_data->inline_content_count,
				view->org_mail_info->total_attachment_count);

		/* Parse charset of original mail. */
		_composer_set_charset_info(view);
		debug_secure("Original charset: (%s)", view->original_charset);

		if (view->composer_type == RUN_COMPOSER_EDIT) {
			/* Need to update seen flag for original mail in draft folder */
			err = email_set_flags_field(view->account_info->original_account_id, &view->original_mail_id, 1, EMAIL_FLAGS_SEEN_FIELD, 1, 1);
			debug_warning_if(err != EMAIL_ERROR_NONE, "email_set_flags_field() failed! ret:[%d]", err);

			/* Set priority information */
			view->priority_option = view->org_mail_info->mail_data->priority;
		}
	} else if ((view->composer_type == RUN_EML_REPLY) || (view->composer_type == RUN_EML_REPLY_ALL) || (view->composer_type == RUN_EML_FORWARD)) {
		err = email_parse_mime_file(view->eml_file_path, &view->org_mail_info->mail_data, &view->org_mail_info->attachment_list, &view->org_mail_info->total_attachment_count);
		retvm_if(err != EMAIL_ERROR_NONE, COMPOSER_ERROR_SERVICE_INIT_FAIL, "email_parse_mime_file() failed! err:[%d]", err);

		debug_secure("full_address_from :%s", view->org_mail_info->mail_data->full_address_from);
		debug_secure("full_address_reply :%s", view->org_mail_info->mail_data->full_address_reply);
	}

	/* Create temp file */
	view->saved_html_path = g_strconcat(composer_util_file_get_temp_dirname(), "/", DEFAULT_CHARSET, ".htm", NULL);
	view->saved_text_path = g_strconcat(composer_util_file_get_temp_dirname(), "/", DEFAULT_CHARSET, NULL);

	debug_leave();
	return COMPOSER_ERROR_NONE;
}

static COMPOSER_ERROR_TYPE_E _composer_load_account_list(void *data)
{
	debug_enter();

	retvm_if(!data, COMPOSER_ERROR_INVALID_ARG, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	int err = email_get_account_list(&view->account_info->account_list, &view->account_info->account_count);
	retvm_if(err != EMAIL_ERROR_NONE, COMPOSER_ERROR_NO_ACCOUNT_LIST, "email_get_account_list() failed! err:[%d]", err);

	/* If there's no account id, set account_id as default account id); */
	if (!view->account_info->original_account_id && (email_engine_get_default_account(&view->account_info->original_account_id) == false)) {
		return COMPOSER_ERROR_NO_DEFAULT_ACCOUNT;
	}

	debug_leave();
	return COMPOSER_ERROR_NONE;
}

static COMPOSER_ERROR_TYPE_E _composer_initialize_account_data(void *data)
{
	debug_enter();

	retvm_if(!data, COMPOSER_ERROR_INVALID_ARG, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;
	EmailComposerAccount *account_info = view->account_info;
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

	composer_util_update_attach_panel_bundles(view);

	debug_log("outgoing_server_size_limit:%d, max_sending_size:%d", account_info->original_account->outgoing_server_size_limit, account_info->max_sending_size);

	debug_leave();
	return COMPOSER_ERROR_NONE;
}

static COMPOSER_ERROR_TYPE_E _composer_prepare_vcard(void *data)
{
	debug_enter();

	retvm_if(!data, COMPOSER_ERROR_INVALID_ARG, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;
	char *file_path = NULL;

	if (!view->is_sharing_contacts) {
		debug_log("Not sharing contacts");
		return COMPOSER_ERROR_NONE;
	}

	debug_log("view->is_sharing_contacts = %s", view->is_sharing_contacts ? "true" : "false");
	debug_log("view->is_sharing_my_profile = %s", view->is_sharing_my_profile ? "true" : "false");
	debug_log("view->shared_contacts_count = %d", view->shared_contacts_count);
	debug_log("view->shared_contact_id = %d", view->shared_contact_id);
	debug_log("view->shared_contact_id_list = %p", view->shared_contact_id_list);

	if (view->shared_contacts_count == 1) {
		int id = (view->shared_contact_id_list ? view->shared_contact_id_list[0] : view->shared_contact_id);
		file_path = email_contacts_create_vcard_from_id(id, composer_util_file_get_temp_dirname(), view->is_sharing_my_profile);
	} else {
		view->vcard_save_timer = ecore_timer_add(COMPOSER_VCARD_SAVE_POPUP_SHOW_MIN_TIME_SEC,
				_composer_vcard_save_timer_cb, view);
		if (view->vcard_save_timer) {
			int ret = 0;
			ecore_timer_freeze(view->vcard_save_timer);
			view->vcard_save_cancel = EINA_FALSE;
			ret = pthread_create(&view->vcard_save_thread, NULL, _composer_vcard_save_worker, view);
			if (ret == 0) {
				debug_log("vCard save thread was created.");
				return COMPOSER_ERROR_NONE;
			}
			debug_error("pthread_create() failed: %d", ret);
			view->vcard_save_thread = 0;
			DELETE_TIMER_OBJECT(view->vcard_save_timer);
		} else {
			debug_error("ecore_timer_add() failed");
		}

		debug_log("Generating in main thread...");

		file_path = email_contacts_create_vcard_from_id_list(view->shared_contact_id_list, view->shared_contacts_count,
				composer_util_file_get_temp_dirname(), NULL);
	}

	if (!file_path) {
		debug_error("vCard generation failed!");
		return COMPOSER_ERROR_VCARD_CREATE_FAILED;
	}

	view->vcard_file_path = file_path;

	debug_leave();
	return COMPOSER_ERROR_NONE;
}

static void *_composer_vcard_save_worker(void *data)
{
	debug_enter();

	retvm_if(!data, NULL, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	view->vcard_file_path = email_contacts_create_vcard_from_id_list(
			view->shared_contact_id_list, view->shared_contacts_count, composer_util_file_get_temp_dirname(), &view->vcard_save_cancel);
	if (!view->vcard_file_path) {
		debug_error("Failed to generate vCard file!");
		view->is_vcard_create_error = EINA_TRUE;
	} else {
		debug_log("Generation of vCard is complete.");
	}

	return NULL;
}

static Eina_Bool _composer_vcard_save_timer_cb(void *data)
{
	retvm_if(!data, ECORE_CALLBACK_CANCEL, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	if (view->vcard_file_path || view->is_vcard_create_error) {
		DELETE_EVAS_OBJECT(view->composer_popup);

		pthread_join(view->vcard_save_thread, NULL);
		view->vcard_save_thread = 0;

		view->vcard_save_timer = NULL;

		if (!view->is_vcard_create_error) {

			composer_attachment_process_attachments_with_string(view,
					view->vcard_file_path, "", ATTACH_BY_USE_ORG_FILE, EINA_FALSE);

			if (view->is_ewk_ready) {
				view->is_load_finished = EINA_TRUE;
				if (view->need_to_destroy_after_initializing) {
					view->is_save_in_drafts_clicked = EINA_TRUE;
					composer_exit_composer_get_contents(view);
				}
			}

		} else if (view->eComposerErrorType == COMPOSER_ERROR_NONE) {
			debug_log("Updating composer error status.");
			view->eComposerErrorType = COMPOSER_ERROR_VCARD_CREATE_FAILED;
			if (view->base.state != EV_STATE_CREATED) {
				debug_log("Composer already started. Creating error popup.");
				_composer_process_error_popup(view, view->eComposerErrorType);
			}
		} else {
			debug_log("Another composer error is already set: %d", view->eComposerErrorType);
		}

		return ECORE_CALLBACK_CANCEL;
	}

	ecore_timer_interval_set(view->vcard_save_timer, COMPOSER_VCARD_SAVE_STATUS_CHECK_INTERVAL_SEC);

	return ECORE_CALLBACK_RENEW;
}

static COMPOSER_ERROR_TYPE_E _composer_initialize_composer_data(void *data)
{
	debug_enter();

	retvm_if(!data, COMPOSER_ERROR_INVALID_ARG, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	view->eComposerErrorType = COMPOSER_ERROR_NONE;
	view->priority_option = EMAIL_MAIL_PRIORITY_NORMAL;

	view->need_to_set_focus_with_js = EINA_TRUE;

	view->with_original_message = EINA_FALSE;
	view->is_focus_on_new_message_div = EINA_TRUE;
	view->is_checkbox_clicked = EINA_TRUE;

	/* To make scrolling speed slow */
	view->scroll_friction = elm_config_scroll_bring_in_scroll_friction_get();
	elm_config_scroll_bring_in_scroll_friction_set(0.6);
	debug_log("==> original scroll_friction: (%f)", view->scroll_friction);

	view->org_mail_info = (EmailComposerMail *)calloc(1, sizeof(EmailComposerMail));
	retvm_if(!view->org_mail_info, COMPOSER_ERROR_OUT_OF_MEMORY, "Failed to allocate memory for view->org_mail_info");

	view->new_mail_info = (EmailComposerMail *)calloc(1, sizeof(EmailComposerMail));
	retvm_if(!view->new_mail_info, COMPOSER_ERROR_OUT_OF_MEMORY, "Failed to allocate memory for view->new_mail_info");

	view->new_mail_info->mail_data = (email_mail_data_t *)calloc(1, sizeof(email_mail_data_t));
	retvm_if(!view->new_mail_info->mail_data, COMPOSER_ERROR_OUT_OF_MEMORY, "Failed to allocate memory for view->new_mail_info->mail_data");

	view->account_info = (EmailComposerAccount *)calloc(1, sizeof(EmailComposerAccount));
	retvm_if(!view->account_info, COMPOSER_ERROR_OUT_OF_MEMORY, "Failed to allocate memory for view->account_info");

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

	debug_leave();
	return COMPOSER_ERROR_NONE;
}

static COMPOSER_ERROR_TYPE_E _composer_initialize_dbus(EmailComposerView *view)
{
	debug_enter();

	GError *error = NULL;
	if (view->dbus_conn == NULL) {
		view->dbus_conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
		if (error) {
			debug_error("g_bus_get_sync() failed (%s)", error->message);
			g_error_free(error);
			return COMPOSER_ERROR_DBUS_FAIL;
		}

		view->dbus_network_id = g_dbus_connection_signal_subscribe(view->dbus_conn, NULL, "User.Email.NetworkStatus", "email", "/User/Email/NetworkStatus",
													   NULL, G_DBUS_SIGNAL_FLAGS_NONE, _composer_gdbus_signal_receiver_cb, (void *)view, NULL);
		retvm_if(view->dbus_network_id == GDBUS_SIGNAL_SUBSCRIBE_FAILURE, COMPOSER_ERROR_DBUS_FAIL, "g_dbus_connection_signal_subscribe() failed!");
	}

	debug_leave();
	return COMPOSER_ERROR_NONE;
}

static COMPOSER_ERROR_TYPE_E _composer_initialize_services(void *data)
{
	debug_enter();

	retvm_if(!data, COMPOSER_ERROR_INVALID_ARG, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;
	COMPOSER_ERROR_TYPE_E ret = COMPOSER_ERROR_NONE;
	int ct_ret = CONTACTS_ERROR_NONE;

	if (!email_engine_initialize()) {
		debug_error("email_engine_initialize() failed!");
		return COMPOSER_ERROR_ENGINE_INIT_FAILED;
	}

	ct_ret = email_contacts_init_service();
	retvm_if(ct_ret != CONTACTS_ERROR_NONE, COMPOSER_ERROR_SERVICE_INIT_FAIL, "email_contacts_init_service() failed! ct_ret:[%d]", ct_ret);

	ret = _composer_initialize_dbus(view);
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

	EmailComposerView *view = (EmailComposerView *)data;

	view->theme = elm_theme_new();
	elm_theme_ref_set(view->theme, NULL); /* refer to default theme(NULL) */
	elm_theme_extension_add(view->theme, email_get_composer_theme_path()); /* add extension to the new theme */

	debug_leave();
	return COMPOSER_ERROR_NONE;
}

static COMPOSER_ERROR_TYPE_E _composer_construct_composer_data(void *data)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;
	COMPOSER_ERROR_TYPE_E ret = COMPOSER_ERROR_NONE;

	ret = _composer_initialize_services(view);
	gotom_if(ret != COMPOSER_ERROR_NONE, CATCH, "_composer_initialize_services() failed");

	ret = composer_initial_data_pre_parse_arguments(view, view->launch_params);
	gotom_if(ret != COMPOSER_ERROR_NONE, CATCH, "composer_initial_data_pre_parse_arguments() failed");

	ret = _composer_prepare_vcard(view);
	gotom_if(ret != COMPOSER_ERROR_NONE, CATCH, "_composer_construct_mail_info() failed");

	ret = _composer_initialize_account_data(view);
	gotom_if(ret != COMPOSER_ERROR_NONE, CATCH, "_composer_construct_mail_info() failed");

	ret = _composer_initialize_mail_info(view);
	gotom_if(ret != COMPOSER_ERROR_NONE, CATCH, "_composer_construct_mail_info() failed");

	_composer_add_main_callbacks(view);

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

	debug_leave();
}

static void _composer_finalize_dbus(EmailComposerView *view)
{
	debug_enter();

	g_dbus_connection_signal_unsubscribe(view->dbus_conn, view->dbus_network_id);
	g_object_unref(view->dbus_conn);
	view->dbus_conn = NULL;
	view->dbus_network_id = 0;

	debug_leave();
}

static void _composer_finalize_services(void *data)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;
	int ct_ret = CONTACTS_ERROR_NONE;

	_composer_finalize_dbus(view);

	ct_ret = email_contacts_finalize_service();
	debug_warning_if(ct_ret != CONTACTS_ERROR_NONE, "email_contacts_finalize_service() failed! ct_ret:[%d]", ct_ret);

	email_engine_finalize();

	debug_leave();
}

static void _composer_destroy_email_info(EmailComposerView *view)
{
	debug_enter();

	if (view->account_info) {
		if (view->account_info->account_list) {
			email_free_account(&view->account_info->account_list, view->account_info->account_count);
			view->account_info->account_list = NULL;
		}
		free(view->account_info);
		view->account_info = NULL;
	}

	if (view->org_mail_info) {
		if (view->org_mail_info->attachment_list) {
			email_free_attachment_data(&view->org_mail_info->attachment_list, view->org_mail_info->total_attachment_count);
			view->org_mail_info->attachment_list = NULL;
		}

		if (view->org_mail_info->mail_data) {
			email_free_mail_data(&view->org_mail_info->mail_data, 1);
			view->org_mail_info->mail_data = NULL;
		}

		free(view->org_mail_info);
		view->org_mail_info = NULL;
	}

	if (view->new_mail_info) {
		if (view->new_mail_info->attachment_list) {
			email_free_attachment_data(&view->new_mail_info->attachment_list, view->new_mail_info->total_attachment_count);
			view->new_mail_info->attachment_list = NULL;
		}

		if (view->new_mail_info->mail_data) {
			email_free_mail_data(&view->new_mail_info->mail_data, 1);
			view->new_mail_info->mail_data = NULL;
		}

		free(view->new_mail_info);
		view->new_mail_info = NULL;
	}

	g_free(view->original_charset);

	debug_leave();
}

static void _composer_destroy_evas_objects(EmailComposerView *view)
{
	debug_enter();

	composer_webkit_del_callbacks(view->ewk_view, view);

	if (view->thread_saving_email) {
		ecore_thread_cancel(view->thread_saving_email);
		view->thread_saving_email = NULL;
	}

	if (view->thread_resize_image) {
		ecore_thread_cancel(view->thread_resize_image);
		view->thread_resize_image = NULL;
	}

	DELETE_IDLER_OBJECT(view->idler_set_focus);
	DELETE_IDLER_OBJECT(view->idler_show_ctx_popup);
	DELETE_IDLER_OBJECT(view->idler_destroy_self);
	DELETE_IDLER_OBJECT(view->idler_regionshow);
	DELETE_IDLER_OBJECT(view->idler_regionbringin);
	DELETE_IDLER_OBJECT(view->idler_move_recipient);

	DELETE_TIMER_OBJECT(view->timer_ctxpopup_show);
	DELETE_TIMER_OBJECT(view->timer_adding_account);
	DELETE_TIMER_OBJECT(view->timer_lazy_loading);
	DELETE_TIMER_OBJECT(view->timer_regionshow);
	DELETE_TIMER_OBJECT(view->timer_regionbringin);
	DELETE_TIMER_OBJECT(view->timer_resize_webkit);
	DELETE_TIMER_OBJECT(view->timer_destroy_composer_popup);
	DELETE_TIMER_OBJECT(view->timer_destory_composer);
	DELETE_TIMER_OBJECT(view->timer_duplicate_toast_in_reciepients);
	DELETE_TIMER_OBJECT(view->cs_events_timer1);
	DELETE_TIMER_OBJECT(view->cs_events_timer2);
	DELETE_TIMER_OBJECT(view->vcard_save_timer);

	DELETE_ANIMATOR_OBJECT(view->cs_animator);

	/* parent of context_popup is base.win. so it has to be removed here. */
	DELETE_EVAS_OBJECT(view->context_popup);

	composer_util_recp_clear_mbe(view->recp_to_mbe);
	composer_util_recp_clear_mbe(view->recp_cc_mbe);
	composer_util_recp_clear_mbe(view->recp_bcc_mbe);

	/* These evas objects regarding recipient entries should be removed here. (they may not be in parent because it is hided when there's no contents)
	 * RelationShip
	 * view->recp_to_layout
	 * view->recp_to_box
	 * view->recp_to_entry_ly, view->recp_to_mbe
	 * view->recp_to_entry_box, view->recp_to_entry_btn
	 * view->recp_to_entry_label, view->recp_to_entry, view->recp_to_display_entry
	 * Deletion should happen in the reverse order
	 */
	DELETE_EVAS_OBJECT(view->recp_to_btn);
	DELETE_EVAS_OBJECT(view->recp_cc_btn);
	DELETE_EVAS_OBJECT(view->recp_bcc_btn);

	DELETE_EVAS_OBJECT(view->recp_to_label);
	DELETE_EVAS_OBJECT(view->recp_cc_label_cc);
	DELETE_EVAS_OBJECT(view->recp_cc_label_cc_bcc);
	DELETE_EVAS_OBJECT(view->recp_bcc_label);

	DELETE_EVAS_OBJECT(view->recp_to_entry.layout);
	DELETE_EVAS_OBJECT(view->recp_cc_entry.layout);
	DELETE_EVAS_OBJECT(view->recp_bcc_entry.layout);

	DELETE_EVAS_OBJECT(view->recp_to_entry_layout);
	DELETE_EVAS_OBJECT(view->recp_cc_entry_layout);
	DELETE_EVAS_OBJECT(view->recp_bcc_entry_layout);

	DELETE_EVAS_OBJECT(view->recp_to_display_entry.layout);
	DELETE_EVAS_OBJECT(view->recp_cc_display_entry.layout);
	DELETE_EVAS_OBJECT(view->recp_bcc_display_entry.layout);

	DELETE_EVAS_OBJECT(view->recp_to_display_entry_layout);
	DELETE_EVAS_OBJECT(view->recp_cc_display_entry_layout);
	DELETE_EVAS_OBJECT(view->recp_bcc_display_entry_layout);

	DELETE_EVAS_OBJECT(view->recp_to_box);
	DELETE_EVAS_OBJECT(view->recp_cc_box);
	DELETE_EVAS_OBJECT(view->recp_bcc_box);

	DELETE_EVAS_OBJECT(view->recp_to_mbe);
	DELETE_EVAS_OBJECT(view->recp_cc_mbe);
	DELETE_EVAS_OBJECT(view->recp_bcc_mbe);

	DELETE_EVAS_OBJECT(view->recp_to_mbe_layout);
	DELETE_EVAS_OBJECT(view->recp_cc_mbe_layout);
	DELETE_EVAS_OBJECT(view->recp_bcc_mbe_layout);

	DELETE_EVAS_OBJECT(view->recp_to_layout);
	DELETE_EVAS_OBJECT(view->recp_cc_layout);
	DELETE_EVAS_OBJECT(view->recp_bcc_layout);

	DELETE_EVAS_OBJECT(view->main_scroller);

	DELETE_EVAS_OBJECT(view->composer_popup);

	debug_leave();
}

/* TODO:: the routines deleting resources should be separated into each view */
static void _composer_destroy_composer(void *data)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	if (view->vcard_save_thread) {
		view->vcard_save_cancel = EINA_TRUE;
		pthread_join(view->vcard_save_thread, NULL);
		view->vcard_save_thread = 0;
	}

	/* Restore scroll friction to original one */
	elm_config_scroll_bring_in_scroll_friction_set(view->scroll_friction);

	/* Restore the indicator mode. // Other module such like mailbox can destroy composer. */
	composer_util_indicator_restore(view);

	composer_attachment_reset_attachment(view);
	composer_util_popup_translate_free_variables(view);
	_composer_destroy_email_info(view);

	elm_theme_extension_del(view->theme, email_get_composer_theme_path());
	elm_theme_free(view->theme);

	_composer_destroy_evas_objects(view);

	/* deinitialize ewk */
	debug_secure("composer_type(%d), saved_html_path(%s), saved_text_path(%s)", view->composer_type, view->saved_html_path, view->saved_text_path);

	FREE(view->saved_text_path);
	FREE(view->saved_html_path);
	FREE(view->plain_content);
	FREE(view->final_body_content);
	FREE(view->final_parent_content);
	FREE(view->final_new_message_content);
	FREE(view->initial_body_content);
	FREE(view->initial_parent_content);
	FREE(view->initial_new_message_content);

	FREE(view->eml_file_path);
	FREE(view->vcard_file_path);
	FREE(view->shared_contact_id_list);

	composer_initial_data_free_initial_contents(view);
	_composer_remove_temp_folders();
	_composer_finalize_services(view);

	debug_leave();
}

static void _composer_process_error_popup(EmailComposerView *view, COMPOSER_ERROR_TYPE_E error)
{
	debug_enter();

	switch (error) {
		case COMPOSER_ERROR_NO_DEFAULT_ACCOUNT:
		case COMPOSER_ERROR_NO_ACCOUNT_LIST:
			debug_error("Lauching composer failed! Failed to get account! [%d]", error);
			view->composer_popup = composer_util_popup_create(view, EMAIL_COMPOSER_STRING_UABLE_TO_COMPOSE_EMAIL_MESSAGE, EMAIL_COMPOSER_STRING_THERE_IS_NO_ACCOUNT_CREATE_A_NEW_ACCOUNT_FIRST,
									_composer_launch_settings_popup_response_cb, EMAIL_COMPOSER_STRING_BUTTON_OK, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);
			break;

		case COMPOSER_ERROR_STORAGE_FULL:
			debug_error("Lauching composer failed! COMPOSER_ERROR_STORAGE_FULL");
			view->composer_popup = composer_util_popup_create(view, EMAIL_COMPOSER_STRING_UABLE_TO_COMPOSE_EMAIL_MESSAGE, EMAIL_COMPOSER_STRING_STORAGE_FULL_CONTENTS,
									_composer_storage_full_popup_response_cb, EMAIL_COMPOSER_STRING_BUTTON_OK, EMAIL_COMPOSER_STRING_BUTTON_SETTINGS, EMAIL_COMPOSER_STRING_NULL);
			break;

		case COMPOSER_ERROR_VCARD_CREATE_FAILED:
			debug_error("Lauching composer failed! COMPOSER_ERROR_VCARD_CREATE_FAILED");
			view->composer_popup = composer_util_popup_create(view, EMAIL_COMPOSER_STRING_UABLE_TO_COMPOSE_EMAIL_MESSAGE, EMAIL_COMPOSER_STRING_VCARD_CREATE_FILED,
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
			view->composer_popup = composer_util_popup_create(view, EMAIL_COMPOSER_STRING_UABLE_TO_COMPOSE_EMAIL_MESSAGE, EMAIL_COMPOSER_STRING_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED,
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

static void _composer_add_main_callbacks(EmailComposerView *view)
{
	debug_enter();

	evas_object_smart_callback_add(view->base.module->conform, "virtualkeypad,state,on", _composer_virtualkeypad_state_on_cb, (void *)view);
	evas_object_smart_callback_add(view->base.module->conform, "virtualkeypad,state,off", _composer_virtualkeypad_state_off_cb, (void *)view);
	evas_object_smart_callback_add(view->base.module->conform, "virtualkeypad,size,changed", _composer_virtualkeypad_size_changed_cb, (void *)view);
	debug_leave();
}

static void _composer_del_main_callbacks(EmailComposerView *view)
{
	debug_enter();

	evas_object_smart_callback_del(view->base.module->conform, "virtualkeypad,state,on", _composer_virtualkeypad_state_on_cb);
	evas_object_smart_callback_del(view->base.module->conform, "virtualkeypad,state,off", _composer_virtualkeypad_state_off_cb);
	evas_object_smart_callback_del(view->base.module->conform, "virtualkeypad,size,changed", _composer_virtualkeypad_size_changed_cb);

	debug_leave();
}

static void _composer_initial_view_draw_richtext_components_if_enabled(EmailComposerView *view)
{
	Eina_Bool richtext_toolbar_enabled = EINA_TRUE;
	if (email_get_richtext_toolbar_status(&richtext_toolbar_enabled) != 0) { /* It does not exist */
		richtext_toolbar_enabled = EINA_FALSE;
		email_set_richtext_toolbar_status(richtext_toolbar_enabled);
	}
	if (true == richtext_toolbar_enabled) {
		composer_initial_view_draw_richtext_components(view);
	}
}

static int _composer_module_create(email_module_t *self, email_params_h params)
{
	debug_enter();
	retvm_if(!self, -1, "Invalid parameter: self is NULL!");

	EmailComposerModule *module = (EmailComposerModule *)self;

	email_params_clone(&module->view.launch_params, params);

	module->view.eComposerErrorType = composer_initial_data_parse_composer_run_type(&module->view, params);
	retvm_if(module->view.eComposerErrorType != COMPOSER_ERROR_NONE, -1, "composer_initial_data_parse_composer_run_type() failed");

	module->view.base.create = _composer_create;
	module->view.base.destroy = _composer_destroy;
	module->view.base.activate = _composer_activate;
	module->view.base.deactivate = _composer_deactivate;
	module->view.base.update = _composer_update;
	module->view.base.on_back_key = _composer_on_back_key;

	int ret = email_module_create_view(&module->base, &module->view.base);
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

	EmailComposerView *view = (EmailComposerView *)self;

	bindtextdomain(PACKAGE, email_get_locale_dir());

	view->eComposerErrorType = _composer_initialize_themes(view);
	gotom_if(view->eComposerErrorType != COMPOSER_ERROR_NONE, CATCH, "_composer_initialize_themes() failed");

	view->eComposerErrorType = _composer_initialize_composer_data(view);
	gotom_if(view->eComposerErrorType != COMPOSER_ERROR_NONE, CATCH, "_composer_initialize_composer_data() failed");

	view->eComposerErrorType = _composer_load_account_list(view);
	gotom_if(view->eComposerErrorType != COMPOSER_ERROR_NONE, CATCH, "_composer_load_account_list() failed");

	/* NOTE: If you need lazy loading concepts, uncomment these lines. */
	/*if (view->composer_type == RUN_COMPOSER_EXTERNAL) {
		view->need_lazy_loading = EINA_TRUE;
	}*/

	if (!view->need_lazy_loading) {
		view->eComposerErrorType = _composer_construct_composer_data(view);
		gotom_if(view->eComposerErrorType != COMPOSER_ERROR_NONE, CATCH, "_composer_construct_composer_data() failed");
	}

CATCH:
	composer_initial_view_draw_base_frame(view);
	composer_initial_view_draw_header_components(view);
	if (!view->need_lazy_loading) {
		composer_initial_view_draw_remaining_components(view);

		/* 1. Not to show IME immediately. (to mbe entry gets the focus when on_create() finishes.)
		 * 2. Highlight focus moves from attachment icon to to field while entering composer.
		 * - Solution: Makes 4 buttons on naviframe unfocusable. In _webkit_js_execute_set_focus_cb(), they will be changed as focusable button.
		 */
		elm_object_tree_focus_allow_set(view->composer_layout, EINA_FALSE);
	}

	composer_util_indicator_show(view);

	_composer_initial_view_draw_richtext_components_if_enabled(view);

	debug_leave();
	return 0;
}

static void _composer_start(EmailComposerView *view)
{
	debug_enter();

	retm_if(!view, "Invalid parameter: view is NULL!");

	if (view->eComposerErrorType != COMPOSER_ERROR_NONE) {
		_composer_process_error_popup(view, view->eComposerErrorType);
		debug_leave();
		return;
	}

	if (view->need_lazy_loading) {
		_composer_create_delay_loading_popup(view);

		/* If we use idler here, sometimes the popup couldn't be displayed properly. */
		view->timer_lazy_loading = ecore_timer_add(0.2f, _composer_delayed_drawing_timer_cb, view);
	} else {
		composer_initial_data_set_mail_info(view, true);
		composer_initial_data_post_parse_arguments(view, view->launch_params);
		composer_initial_data_make_initial_contents(view);
	}

	debug_leave();
}

static void _composer_deactivate(email_view_t *self)
{
	debug_enter();

	retm_if(!self, "Invalid parameter: self is NULL!");
	EmailComposerView *view = (EmailComposerView *)self;

	if (view->is_need_close_composer_on_low_memory) {
		view->timer_destory_composer = ecore_timer_add(0.1f, _composer_destroy_timer_cb, view);

	}
	if (view->selected_entry == view->ewk_view) {
		evas_object_focus_set(view->ewk_view, EINA_FALSE);
	}

	view->is_hided = EINA_TRUE;

	debug_leave();
}

static void _composer_activate(email_view_t *self, email_view_state prev_state)
{
	debug_enter();

	retm_if(!self, "Invalid parameter: self is NULL!");
	EmailComposerView *view = (EmailComposerView *)self;

	if (prev_state == EV_STATE_CREATED) {
		_composer_start(view);
		return;
	}

	ret_if(!(view->is_load_finished || view->is_adding_account_requested));

	view->is_hided = EINA_FALSE;
	_composer_contacts_update_recp_info_for_recipients(view);
	if (view->need_to_set_focus_on_resume) {
		composer_util_focus_set_focus(view, view->selected_entry);
		view->need_to_set_focus_on_resume = EINA_FALSE;
	}

	if (evas_object_freeze_events_get(view->ewk_view)) {
		evas_object_freeze_events_set(view->ewk_view, EINA_FALSE);
	}

	if (view->is_adding_account_requested) {
		view->is_adding_account_requested = EINA_FALSE;
		DELETE_TIMER_OBJECT(view->timer_adding_account);
		view->timer_adding_account = ecore_timer_add(0.01f, _composer_update_data_after_adding_account_timer_cb, view);
	}

	if (!view->composer_popup) {
		composer_util_focus_set_focus_with_idler(view, view->selected_entry);
	}

	debug_leave();
}

static void _composer_destroy(email_view_t *self)
{
	debug_enter();

	retm_if(!self, "Invalid parameter: self is NULL!");

	EmailComposerView *view = (EmailComposerView *)self;
	email_params_h reply = NULL;

	if (email_params_create(&reply)) {
		int ret = email_module_send_result(view->base.module, reply);
		debug_warning_if(ret != 0, "email_module_send_result() failed!");
	} else {
		debug_warning("email_params_create() failed!");
	}

	email_params_free(&reply);

	email_params_free(&view->launch_params);

	view->is_composer_getting_destroyed = EINA_TRUE;
	_composer_del_main_callbacks(view);
	_composer_destroy_composer(view);

	debug_leave();
}

static void _composer_on_back_key(email_view_t *self)
{
	debug_enter();

	retm_if(!self, "self is NULL");
	EmailComposerView *view = (EmailComposerView *)self;

	composer_initial_view_back_cb(view);

	debug_leave();
	return;
}

static void _composer_on_message(email_module_t *self, email_params_h msg)
{
	debug_enter();

	retm_if(!self, "self is NULL");

	EmailComposerModule *module = (EmailComposerModule *)self;
	EmailComposerView *view = &module->view;
	const char *msg_type = NULL;

	debug_warning_if(!email_params_get_str(msg, EMAIL_BUNDLE_KEY_MSG,  &msg_type),
			"email_params_get_str(%s) failed!", EMAIL_BUNDLE_KEY_MSG);

	if (g_strcmp0(msg_type, EMAIL_BUNDLE_VAL_EMAIL_COMPOSER_SAVE_DRAFT) == 0) {

		email_module_terminate_any_launched_app(view->base.module, false);

		view->is_force_destroy = EINA_TRUE;
		if (view->is_load_finished) {
			view->is_save_in_drafts_clicked = EINA_TRUE; /* The action should be just like clicking save in drafts. */
			composer_exit_composer_get_contents(view); /* Exit out of composer without any pop-up. */
		} else {
			view->need_to_destroy_after_initializing = EINA_TRUE;
		}
	}

	debug_leave();
}

static void _composer_on_event(email_module_t *self, email_module_event_e event)
{
	debug_enter();

	EmailComposerModule *module = (EmailComposerModule *)self;
	EmailComposerView *view = &module->view;

	switch (event) {
	case EM_EVENT_LOW_MEMORY_SOFT:
		debug_log("Low memory: %d", event);
		composer_webkit_handle_mem_warning(view, EINA_FALSE);
		break;
	case EM_EVENT_LOW_MEMORY_HARD:
		debug_log("Low memory: %d", event);
		composer_webkit_handle_mem_warning(view, EINA_TRUE);
		break;
	default:
		break;
	}

	debug_leave();
}

static void _composer_language_change_update(EmailComposerView *view)
{
	debug_enter();
	retm_if(!view, "Invalid parameter: self is NULL!");

	if (view->recp_from_label) { composer_recipient_label_text_set(view->recp_from_label, COMPOSER_RECIPIENT_TYPE_FROM); }
	if (view->recp_to_label) { composer_recipient_label_text_set(view->recp_to_label, COMPOSER_RECIPIENT_TYPE_TO); }
	if (view->recp_cc_label_cc) { composer_recipient_label_text_set(view->recp_cc_label_cc, COMPOSER_RECIPIENT_TYPE_CC); }
	if (view->recp_cc_label_cc_bcc) { composer_recipient_label_text_set(view->recp_cc_label_cc_bcc, COMPOSER_RECIPIENT_TYPE_CC_BCC); }
	if (view->recp_bcc_label) { composer_recipient_label_text_set(view->recp_bcc_label, COMPOSER_RECIPIENT_TYPE_BCC); }

	if (view->recp_to_mbe) { composer_recipient_label_text_set(view->recp_to_mbe, COMPOSER_RECIPIENT_TYPE_TO); }
	if (view->recp_cc_mbe) {
		if (view->bcc_added) {
			composer_recipient_label_text_set(view->recp_cc_mbe, COMPOSER_RECIPIENT_TYPE_CC);
		} else {
			composer_recipient_label_text_set(view->recp_cc_mbe, COMPOSER_RECIPIENT_TYPE_CC_BCC);
		}
	}
	if (view->recp_bcc_mbe) { composer_recipient_label_text_set(view->recp_bcc_mbe, COMPOSER_RECIPIENT_TYPE_BCC); }
	if (view->attachment_group_layout) { composer_attachment_ui_group_update_text(view); }

	if (view->with_original_message) {
		char *to_be_updated_original_message_bar = g_strdup_printf(EC_JS_UPDATE_TEXT_TO_ORIGINAL_MESSAGE_BAR, email_get_email_string("IDS_EMAIL_BODY_INCLUDE_ORIGINAL_MESSAGE"));
		if (!ewk_view_script_execute(view->ewk_view, to_be_updated_original_message_bar, NULL, NULL)) {
			debug_error("EC_JS_UPDATE_TEXT_TO_ORIGINAL_MESSAGE_BAR failed.");
		}
		g_free(to_be_updated_original_message_bar);
	}

	if (view->subject_entry.entry) {
		composer_subject_update_guide_text(view);
	}

	/* This code is specific for popups where string translation is not handled by EFL */
	if (view->composer_popup && view->pt_element_type) { composer_util_popup_translate_do(view); }

	debug_leave();
}

static void _composer_orientation_change_update(EmailComposerView *view)
{
	debug_enter();
	retm_if(!view, "Invalid parameter: self is NULL!");

	switch (view->base.orientation) {
	case APP_DEVICE_ORIENTATION_0:
		view->is_horizontal = EINA_FALSE;
		break;
	case APP_DEVICE_ORIENTATION_90:
		view->is_horizontal = EINA_TRUE;
		break;
	case APP_DEVICE_ORIENTATION_180:
		view->is_horizontal = EINA_FALSE;
		break;
	case APP_DEVICE_ORIENTATION_270:
		view->is_horizontal = EINA_TRUE;
		break;
	}

	if (view->recp_from_ctxpopup) {
		DELETE_EVAS_OBJECT(view->recp_from_ctxpopup);
	}

	composer_recipient_update_display_string(view, view->recp_to_mbe, view->recp_to_entry.entry, view->recp_to_display_entry.entry, view->to_recipients_cnt);
	composer_recipient_update_display_string(view, view->recp_cc_mbe, view->recp_cc_entry.entry, view->recp_cc_display_entry.entry, view->cc_recipients_cnt);
	composer_recipient_update_display_string(view, view->recp_bcc_mbe, view->recp_bcc_entry.entry, view->recp_bcc_display_entry.entry, view->bcc_recipients_cnt);
	ret_if(!view->is_load_finished);

	composer_initial_view_set_combined_scroller_rotation_mode(view);

	/* Timer should be here to bringin the selected entry.
	 * If we don't use timer here, bringin function doesn't work properly.
	 */
	DELETE_TIMER_OBJECT(view->timer_regionshow);
	if (view->selected_entry != view->ewk_view) {
		view->timer_regionshow = ecore_timer_add(0.2f, composer_util_scroll_region_show_timer, view);
	}

	/* If there's a popup which has list or genlist, we should resize it to fit to the desired size. */
	if (view->composer_popup) {
		composer_util_popup_resize_popup_for_rotation(view->composer_popup, view->is_horizontal);
	}

	debug_leave();
}

static void _composer_update(email_view_t *self, int flags)
{
	debug_enter();

	retm_if(!self, "Invalid parameter: self is NULL!");
	EmailComposerView *view = (EmailComposerView *)self;

	if (flags & EVUF_ORIENTATION_CHANGED) {
		_composer_orientation_change_update(view);
	}
	if (flags & EVUF_LANGUAGE_CHANGED) {
		_composer_language_change_update(view);
	}

	debug_leave();
}

static void _composer_virtualkeypad_state_on_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	if (view->ps_box) {
		composer_ps_change_layout_size(view);
	}

	debug_leave();
}

static void _composer_virtualkeypad_state_off_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	/* When user rotate device with "Stop composing email" popup, this callback is called even though the popup has already shown. */
	if (view->is_back_btn_clicked && !view->composer_popup) {
		composer_exit_composer_get_contents(view);
	}

	if (view->ps_box) {
		composer_ps_change_layout_size(view);
	}

	debug_leave();
}

static void _composer_virtualkeypad_size_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;
	Evas_Coord_Rectangle *rect = (Evas_Coord_Rectangle *)event_info;

	debug_log("[x,y = %d,%d] - [w,h = %d,%d]", rect->x, rect->y, rect->w, rect->h);

	composer_util_resize_min_height_for_new_message(view, rect->h);

	if (view->ps_box) {
		composer_ps_change_layout_size(view);
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

	EmailComposerView *view = (EmailComposerView *)data;

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
				if (view->draft_sync_handle == data3 && view->org_mail_info && view->org_mail_info->mail_data && view->org_mail_info->mail_data->mail_id == data1) {
					composer_initial_data_destroy_download_contents_popup(view); /* delete progress popup */
					/* Below function should also get called after download is finished
					 * First data should be initialized after download, then composer_initial_data_set_mail_info should be called
					 * This function will also get mail data and attachment list
					 * If we do not do this parsing function for smime attachment is not called and details are not displayed for the
					 * first time after draft mail is downloaded from server, even though data is updated
					 */
					_composer_initialize_mail_info(view);
					composer_initial_data_set_mail_info(view, false);
				}
				break;

			case NOTI_DOWNLOAD_BODY_FAIL:
				/* DATA1[mail_id] DATA2[NULL] DATA3[handle_id] */
				debug_log("receive noti, DOWNLOAD_BODY_FAIL");
				if (view->draft_sync_handle == data3 && view->org_mail_info && view->org_mail_info->mail_data && view->org_mail_info->mail_data->mail_id == data1) {
					composer_initial_data_destroy_download_contents_popup(view); /* delete progress popup */

					char *err_msg = composer_util_get_error_string(data4);
					int noti_ret = notification_status_message_post(err_msg);
					debug_warning_if(noti_ret != NOTIFICATION_ERROR_NONE, "notification_status_message_post() failed! ret:(%d)", noti_ret);
					g_free(err_msg);

					composer_initial_data_set_mail_info(view, false);
				}
				break;

			case NOTI_DOWNLOAD_ATTACH_FINISH:
				/* DATA1[mail_id] DATA2[NULL] DATA3[index] */
				debug_log("receive noti, DOWNLOAD_ATTACH_FINISH");
				if (view->downloading_attachment && (view->downloading_attachment->mail_id == data1) && (view->downloading_attachment_index == data3)) {
					email_attachment_data_t *new_attachment_list = NULL;
					int new_attachment_count = 0;
					int ret = EMAIL_ERROR_NONE;

					/* 1-1. Update attachment info for existing mail */
					if (view->downloading_attachment->mail_id == view->org_mail_info->mail_data->mail_id) {
						ret = email_get_attachment_data_list(view->org_mail_info->mail_data->mail_id, &new_attachment_list, &new_attachment_count);
					/* 1-2. Update attachment info for reference mail */
					} else if (view->downloading_attachment->mail_id == view->org_mail_info->mail_data->reference_mail_id) {
						ret = email_get_attachment_data_list(view->org_mail_info->mail_data->reference_mail_id, &new_attachment_list, &new_attachment_count);
					}

					if (ret == EMAIL_ERROR_NONE) {
						/* 2. Update attachment info for new mail. */
						int i = 0;
						email_attachment_data_t *curr_att = NULL;
						for (i = 0; i < new_attachment_count; i++) {
							curr_att = new_attachment_list + i;
							if (curr_att->attachment_id == view->downloading_attachment->attachment_id) {
								FREE(view->downloading_attachment->attachment_path);
								FREE(view->downloading_attachment->attachment_mime_type);

								view->downloading_attachment->attachment_path = COMPOSER_STRDUP(curr_att->attachment_path);
								view->downloading_attachment->attachment_mime_type = COMPOSER_STRDUP(curr_att->attachment_mime_type);
								view->downloading_attachment->attachment_size = curr_att->attachment_size;
								view->downloading_attachment->save_status = curr_att->save_status;
								break;
							}
						}

						if (view->downloading_attachment->mail_id == view->org_mail_info->mail_data->mail_id) {
							ret = email_free_attachment_data(&view->org_mail_info->attachment_list, view->org_mail_info->total_attachment_count);
							debug_warning_if(ret != EMAIL_ERROR_NONE, "email_free_attachment_data() failed! It'll cause a memory leak!");

							view->org_mail_info->attachment_list = new_attachment_list;
							view->org_mail_info->total_attachment_count = new_attachment_count;
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

						EINA_LIST_FOREACH(view->attachment_item_list, list, attachment_item_data) {
							email_attachment_data_t *att = attachment_item_data->attachment_data;
							if (att->attachment_id == view->downloading_attachment->attachment_id) {
								if (view->composer_popup) {
									composer_util_popup_response_cb(view, view->composer_popup, NULL);
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
						view->need_download = is_needed_to_download; /* this attachment is from existing mail, but it hasn't been downloaded yet. */
					} else {
						debug_error("email_get_attachment_data_list() failed! ret:[%d]", ret);
						view->composer_popup = composer_util_popup_create(view, EMAIL_COMPOSER_STRING_HEADER_UNABLE_TO_DOWNLOAD_ATTACHMENT_ABB, EMAIL_COMPOSER_STRING_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED,
													composer_util_popup_response_cb, EMAIL_COMPOSER_STRING_BUTTON_OK, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);
					}

					composer_attachment_download_attachment_clear_flags(view);
				}
				break;

			case NOTI_DOWNLOAD_ATTACH_FAIL:
				/* DATA1[mail_id] DATA2[NULL] DATA3[attachment_index] */
				debug_log("receive noti, DOWNLOAD_ATTACH_FAIL");
				if (view->downloading_attachment && (view->downloading_attachment->mail_id == data1) && (view->downloading_attachment_index == data3)) {
					composer_attachment_download_attachment_clear_flags(view);

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

	EmailComposerView *view = (EmailComposerView *)data;
	retm_if(!view->org_mail_info || !view->org_mail_info->mail_data, "Invalid parameter: data is NULL!");

	if (view->org_mail_info->mail_data->file_path_html) {
		debug_log("[HTML body]");
		view->original_charset = email_parse_get_filename_from_path(view->org_mail_info->mail_data->file_path_html);
	} else if (view->org_mail_info->mail_data->file_path_plain) {
		debug_log("[TEXT body]");
		view->original_charset = email_parse_get_filename_from_path(view->org_mail_info->mail_data->file_path_plain);
	}
}

static void _composer_contacts_update_recp_info_for_mbe(EmailComposerView *view, Evas_Object *mbe)
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

			email_contacts_delete_contact_info(&contact_info);
		}

		elm_object_item_data_set(item, ri);
		item = elm_multibuttonentry_item_next_get(item);

	}

	debug_leave();
}

static void _composer_contacts_update_recp_info_for_recipients(EmailComposerView *view)
{
	debug_enter();

	if (view->recp_to_mbe) {
		_composer_contacts_update_recp_info_for_mbe(view, view->recp_to_mbe);
	}
	if (view->recp_cc_mbe) {
		_composer_contacts_update_recp_info_for_mbe(view, view->recp_cc_mbe);
	}
	if (view->recp_bcc_mbe) {
		_composer_contacts_update_recp_info_for_mbe(view, view->recp_bcc_mbe);
	}

	debug_leave();
}

/*
 * Definition for exported functions
 */
#ifdef SHARED_MODULES_FEATURE
EMAIL_API email_module_t *email_module_alloc()
#else
email_module_t *composer_module_alloc()
#endif
{
	debug_enter();

	_composer_set_environment_variables();

	EmailComposerModule *module = MEM_ALLOC(module, 1);
	if (!module) {
		debug_error("module is NULL");
		return NULL;
	}

	module->base.create = _composer_module_create;
	module->base.on_message = _composer_on_message;
	module->base.on_event = _composer_on_event;

	debug_leave();
	return &module->base;
}
