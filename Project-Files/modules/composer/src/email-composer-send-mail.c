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

#define _GNU_SOURCE /* for strcasestr() */

#include <string.h>
#include <notification.h>
#include <email-api.h>

#include "email-engine.h"
#include "email-utils.h"

#include "email-composer.h"
#include "email-composer-util.h"
#include "email-composer-types.h"
#include "email-composer-send-mail.h"
#include "email-composer-predictive-search.h"
#include "email-composer-js.h"
#include "email-composer-recipient.h"
#include "email-composer-launcher.h"
#include "email-composer-attachment.h"

#define ATTACHMENT_SIZE_THRESHOLD (25 * 1024 * 1024) /* 25 MB */


/*
 * Declaration for static functions
 */

static MBE_VALIDATION_ERROR _check_vaildify_of_recipient_field(EmailComposerUGD *ugd, Evas_Object *obj);

static void _delete_image_control_layer_cb(Evas_Object *obj, const char *result, void *data);
static void _get_image_list_before_leaving_composer_cb(Evas_Object *obj, const char *result, void *data);
static void _get_final_plain_text_cb(Evas_Object *obj, const char *result, void *data);
static void _get_final_parent_content_cb(Evas_Object *obj, const char *result, void *data);
static void _get_final_new_message_content_cb(Evas_Object *obj, const char *result, void *data);
static void _get_final_body_content_cb(Evas_Object *obj, const char *result, void *data);

static void _save_in_drafts_thread_worker(void *data, Ecore_Thread *th);
static void _save_in_drafts_thread_notify(void *data, Ecore_Thread *th, void *msg_data);
static void _save_in_drafts_thread_finish(void *data, Ecore_Thread *th);

static void _exit_composer_cb(Evas_Object *obj, const char *result, void *data);
static void _exit_composer_with_sending_mail(void *data);
static void _exit_composer_process_sending_mail(void *data);
static void _exit_composer_without_sending_mail(void *data);

static COMPOSER_ERROR_TYPE_E _send_mail(EmailComposerUGD *ugd);
static COMPOSER_ERROR_TYPE_E _add_mail(EmailComposerUGD *ugd, email_mailbox_type_e mailbox_type);
static COMPOSER_ERROR_TYPE_E _make_mail(EmailComposerUGD *ugd, email_mailbox_type_e mailbox_type);
static void _save_in_drafts_process_result(EmailComposerUGD *ugd, int ret);
static void _save_in_drafts(EmailComposerUGD *ugd);
static void _save_in_drafts_in_thread(EmailComposerUGD *ugd);

static Eina_Bool _send_mail_finish_idler_cb(void *data);
static void _composer_send_mail_storage_full_popup_response_cb(void *data, Evas_Object *obj, void *event_info);
static void _save_popup_cb(void *data, Evas_Object *obj, void *event_info);
static void _unable_to_save_in_drafts_cb(void *data, Evas_Object *obj, void *event_info);
static void _download_attachment_popup_cb(void *data, Evas_Object *obj, void *event_info);

static COMPOSER_ERROR_TYPE_E _remove_old_body_file(EmailComposerUGD *ugd);
static COMPOSER_ERROR_TYPE_E _make_mail_set_from_address(EmailComposerUGD *ugd);
static COMPOSER_ERROR_TYPE_E _make_mail_set_mailbox_info(EmailComposerUGD *ugd, email_mailbox_type_e mailbox_type);
static void _make_mail_set_reference_id(EmailComposerUGD *ugd);

static void _make_mail_make_draft_html_content(EmailComposerUGD *ugd, char **html_content, email_mailbox_type_e mailbox_type);
static Eina_Bool _make_mail_change_image_paths_to_relative(char *src_html_content, char **dest_buffer, email_mailbox_type_e dest_mailbox);
static void _make_mail_make_attachment_list(EmailComposerUGD *ugd);
static void _check_forwarding_attachment_download(void *data);
static void _send_mail_failed_popup_cb(void *data, Evas_Object *obj, void *event_info);

static email_string_t EMAIL_COMPOSER_STRING_NULL = { NULL, NULL };
static email_string_t EMAIL_COMPOSER_STRING_BUTTON_OK = { PACKAGE, "IDS_EMAIL_BUTTON_OK" };
static email_string_t EMAIL_COMPOSER_STRING_BUTTON_CANCEL = { PACKAGE, "IDS_EMAIL_BUTTON_CANCEL" };
static email_string_t EMAIL_COMPOSER_STRING_BUTTON_SAVE_ABB = { PACKAGE, "IDS_EMAIL_BUTTON_SAVE_ABB" };
static email_string_t EMAIL_COMPOSER_STRING_BUTTON_DISCARD_ABB2 = { PACKAGE, "IDS_EMAIL_BUTTON_DISCARD_ABB2" };
static email_string_t EMAIL_COMPOSER_STRING_BUTTON_CONTINUE_ABB = { PACKAGE, "IDS_EMAIL_BUTTON_CONTINUE_ABB" };
static email_string_t EMAIL_COMPOSER_STRING_HEADER_STOP_COMPOSING_EMAIL_HEADER_ABB = { PACKAGE, "IDS_EMAIL_HEADER_STOP_COMPOSING_EMAIL_HEADER_ABB" };
static email_string_t EMAIL_COMPOSER_STRING_HEADER_DOWNLOAD_ATTACHMENTS_ABB = { PACKAGE, "IDS_EMAIL_HEADER_DOWNLOAD_ATTACHMENTS_ABB" };
static email_string_t EMAIL_COMPOSER_STRING_HEADER_UNABLE_TO_SEND_EMAIL_ABB = { PACKAGE, "IDS_EMAIL_HEADER_UNABLE_TO_SEND_EMAIL_ABB" };
static email_string_t EMAIL_COMPOSER_STRING_POP_SAVE_YOUR_CHANGES_OR_DISCARD_THEM_Q = { PACKAGE, "IDS_EMAIL_POP_SAVE_YOUR_CHANGES_OR_DISCARD_THEM_Q" };
static email_string_t EMAIL_COMPOSER_STRING_POP_ALL_ATTACHMENTS_IN_THIS_EMAIL_WILL_BE_DOWNLOADED_BEFORE_IT_IS_FORWARDED = { PACKAGE, "IDS_EMAIL_POP_ALL_ATTACHMENTS_IN_THIS_EMAIL_WILL_BE_DOWNLOADED_BEFORE_IT_IS_FORWARDED" };
static email_string_t EMAIL_COMPOSER_STRING_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED = { PACKAGE, "IDS_EMAIL_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED" };
static email_string_t EMAIL_COMPOSER_STRING_MF_HEADER_UNABLE_TO_SAVE_DATA_ABB = { PACKAGE, "IDS_MF_HEADER_UNABLE_TO_SAVE_DATA_ABB" };
static email_string_t EMAIL_COMPOSER_STRING_STORAGE_FULL_CONTENTS = { PACKAGE, "IDS_EMAIL_POP_THERE_IS_NOT_ENOUGH_SPACE_IN_YOUR_DEVICE_STORAGE_GO_TO_SETTINGS_POWER_AND_STORAGE_STORAGE_THEN_DELETE_SOME_FILES_AND_TRY_AGAIN" };
static email_string_t EMAIL_COMPOSER_STRING_BUTTON_SETTINGS = { PACKAGE, "IDS_EMAIL_BUTTON_SETTINGS" };
static email_string_t EMAIL_COMPOSER_STRING_HEADER_UNABLE_TO_PERFORM_ACTION_ABB = { PACKAGE, "IDS_EMAIL_HEADER_UNABLE_TO_PERFORM_ACTION_ABB" };
static email_string_t EMAIL_COMPOSER_STRING_POP_SAVING_EMAIL_ING = { PACKAGE, "IDS_EMAIL_POP_SAVING_EMAIL_ING" };


/*
 * Definition for static functions
 */

static MBE_VALIDATION_ERROR _check_vaildify_of_recipient_field(EmailComposerUGD *ugd, Evas_Object *obj)
{
	debug_enter();

	MBE_VALIDATION_ERROR err = MBE_VALIDATION_ERROR_NONE;
	Evas_Object *entry = NULL;

	if (obj == ugd->recp_to_mbe) {
		entry = ugd->recp_to_entry.entry;
	} else if (obj == ugd->recp_cc_mbe) {
		entry = ugd->recp_cc_entry.entry;
	} else if (obj == ugd->recp_bcc_mbe) {
		entry = ugd->recp_bcc_entry.entry;
	}

	char *recp = elm_entry_markup_to_utf8(elm_entry_entry_get(entry));
	if (STR_LEN(recp)) {
		char *invalid_list = NULL;
		char *mod_recp = g_strconcat(recp, ";", NULL);
		composer_recipient_mbe_validate_email_address_list(ugd, obj, mod_recp, &invalid_list, &err);
		FREE(mod_recp);

		if (invalid_list) {
			elm_entry_entry_set(entry, g_strstrip(invalid_list));
			elm_entry_cursor_end_set(entry);
			free(invalid_list);
		} else {
			elm_entry_entry_set(entry, NULL);
		}
	}

	FREE(recp);

	debug_leave();
	return err;
}

static void _delete_image_control_layer_cb(Evas_Object *obj, const char *result, void *data)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	if (!ugd->is_checkbox_clicked) {
		if (!ewk_view_script_execute(ugd->ewk_view, EC_JS_GET_IMAGE_LIST_FROM_NEW_MESSAGE, _get_image_list_before_leaving_composer_cb, (void *)ugd)) {
			debug_error("EC_JS_GET_IMAGE_LIST_FROM_NEW_MESSAGE failed.");
		}
	} else {
		if (!ewk_view_script_execute(ugd->ewk_view, EC_JS_GET_IMAGE_LIST, _get_image_list_before_leaving_composer_cb, (void *)ugd)) {
			debug_error("EC_JS_GET_IMAGE_LIST failed.");
		}
	}

	debug_leave();
}

static void _get_image_list_before_leaving_composer_cb(Evas_Object *obj, const char *result, void *data)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	composer_util_get_image_list_cb(obj, result, ugd);

	/* This ewk call invokes some successive ewk calls to get html contents. */
	if (!ewk_view_plain_text_get(ugd->ewk_view, _get_final_plain_text_cb, (void *)ugd)) {
		debug_error("ewk_view_plain_text_get() failed!");
	}

	debug_leave();
}

static void _get_final_plain_text_cb(Evas_Object *obj, const char *result, void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	FREE(ugd->plain_content);
	if (result) {
		ugd->plain_content = g_strdup(result);
	}

	if (ugd->with_original_message) {
		if (!ewk_view_script_execute(ugd->ewk_view, EC_JS_GET_CONTENTS_FROM_ORG_MESSAGE, _get_final_parent_content_cb, (void *)ugd)) {
			debug_error("EC_JS_GET_CONTENTS_FROM_ORG_MESSAGE failed!.");
		}
	} else {
		if (!ewk_view_script_execute(ugd->ewk_view, EC_JS_GET_CONTENTS_FROM_BODY, _get_final_body_content_cb, (void *)ugd)) {
			debug_error("EC_JS_GET_CONTENTS_FROM_BODY failed.");
		}
	}

	debug_leave();
}

/* DO NOT use this function alone. this function is successively called when you call _get_final_plain_text_cb(). */
static void _get_final_parent_content_cb(Evas_Object *obj, const char *result, void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	FREE(ugd->final_parent_content);
	if (result) {
		ugd->final_parent_content = g_strdup(result);
		/*debug_secure("FINAL PARENT CONTENT: (%s)", ugd->final_parent_content);*/
	}

	if (!ewk_view_script_execute(ugd->ewk_view, EC_JS_GET_CONTENTS_FROM_NEW_MESSAGE, _get_final_new_message_content_cb, (void *)ugd)) {
		debug_error("EC_JS_GET_CONTENTS_FROM_NEW_MESSAGE failed!.");
	}

	debug_leave();
}

/* DO NOT use this function alone. this function is successively called when you call _get_final_plain_text_cb(). */
static void _get_final_new_message_content_cb(Evas_Object *obj, const char *result, void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	FREE(ugd->final_new_message_content);
	if (result) {
		ugd->final_new_message_content = g_strdup(result);
	}

	if (!ewk_view_script_execute(ugd->ewk_view, EC_JS_GET_CONTENTS_FROM_BODY, _get_final_body_content_cb, (void *)ugd)) {
		debug_error("EC_JS_GET_CONTENTS_FROM_BODY failed!.");
	}

	debug_leave();
}

static void _get_final_body_content_cb(Evas_Object *obj, const char *result, void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	FREE(ugd->final_body_content);
	if (result) {
		ugd->final_body_content = g_strdup(result);
	}

	_exit_composer_cb(NULL, NULL, ugd);

	debug_leave();
}

static void _save_in_drafts_thread_worker(void *data, Ecore_Thread *th)
{
	debug_enter();

	int ret = _add_mail(data, EMAIL_MAILBOX_TYPE_DRAFT);
	ecore_thread_feedback(th, (void *)(ptrdiff_t)ret);

	debug_leave();
}

static void _save_in_drafts_thread_notify(void *data, Ecore_Thread *th, void *msg_data)
{
	debug_enter();

	_save_in_drafts_process_result(data, (int)(ptrdiff_t)msg_data);

	debug_leave();
}

static void _save_in_drafts_thread_finish(void *data, Ecore_Thread *th)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	ugd->thread_saving_email = NULL;

	debug_leave();
}

static void _exit_composer_cb(Evas_Object *obj, const char *result, void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	if (ugd->is_save_in_drafts_clicked || ugd->is_back_btn_clicked) {
		_exit_composer_without_sending_mail(ugd);
	} else {
		_exit_composer_with_sending_mail(ugd);
	}

	debug_leave();
}

static void _exit_composer_with_sending_mail(void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	_check_forwarding_attachment_download(ugd);

	debug_leave();
}

static void _exit_composer_process_sending_mail(void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	COMPOSER_ERROR_TYPE_E ret = _send_mail(ugd);
	if (ret == COMPOSER_ERROR_NONE) {
		/* If we don't use idler to destroy composer which is launched by appcontrol, a black screen appears when ug is destroyed.
		 * It's only happened on this exit. (No one knows the exact reason why a black screen appears.)
		 */
		composer_util_network_state_noti_post();
		DELETE_IDLER_OBJECT(ugd->idler_destroy_self);
		ugd->idler_destroy_self = ecore_idler_add(_send_mail_finish_idler_cb, ugd);
	} else if (ret == COMPOSER_ERROR_MAIL_SIZE_EXCEEDED) {
		composer_attachment_launch_attachment_error_popup(COMPOSER_ERROR_ATTACHMENT_MAX_SIZE_EXCEEDED, _send_mail_failed_popup_cb, ugd);
	} else if (ret == COMPOSER_ERROR_MAIL_STORAGE_FULL) {
		ugd->composer_popup = composer_util_popup_create(ugd, EMAIL_COMPOSER_STRING_MF_HEADER_UNABLE_TO_SAVE_DATA_ABB, EMAIL_COMPOSER_STRING_STORAGE_FULL_CONTENTS, _composer_send_mail_storage_full_popup_response_cb,
		                                                EMAIL_COMPOSER_STRING_BUTTON_CANCEL, EMAIL_COMPOSER_STRING_BUTTON_SETTINGS, EMAIL_COMPOSER_STRING_NULL);
	} else {
		ugd->composer_popup = composer_util_popup_create(ugd, EMAIL_COMPOSER_STRING_HEADER_UNABLE_TO_SEND_EMAIL_ABB, EMAIL_COMPOSER_STRING_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED,
								_send_mail_failed_popup_cb, EMAIL_COMPOSER_STRING_BUTTON_OK, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);
	}

	debug_leave();
}

static void _exit_composer_without_sending_mail(void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	if (ugd->is_save_in_drafts_clicked) {
		_save_in_drafts_in_thread(ugd);
	} else if (ugd->is_back_btn_clicked) {
		if (!composer_util_is_mail_modified(ugd)) {
			DELETE_EVAS_OBJECT(ugd->composer_popup); /* To delete mdm noti popup */

			/* Restore the indicator mode. */
			composer_util_indicator_restore(ugd);

			email_module_make_destroy_request(ugd->base.module);
		} else {
			ugd->composer_popup = composer_util_popup_create(ugd, EMAIL_COMPOSER_STRING_HEADER_STOP_COMPOSING_EMAIL_HEADER_ABB, EMAIL_COMPOSER_STRING_POP_SAVE_YOUR_CHANGES_OR_DISCARD_THEM_Q, _save_popup_cb,
					EMAIL_COMPOSER_STRING_BUTTON_CANCEL, EMAIL_COMPOSER_STRING_BUTTON_DISCARD_ABB2, EMAIL_COMPOSER_STRING_BUTTON_SAVE_ABB);
			if (composer_util_is_max_sending_size_exceeded(ugd)) {
				Evas_Object *save_button = elm_object_part_content_get(ugd->composer_popup, "button3");
				elm_object_disabled_set(save_button, EINA_TRUE);
			}
		}
	}

	debug_leave();
}

static COMPOSER_ERROR_TYPE_E _send_mail(EmailComposerUGD *ugd)
{
	debug_enter();

	retvm_if(!ugd, COMPOSER_ERROR_SEND_FAIL, "ugd is NULL!");
	retvm_if(!ugd->account_info, COMPOSER_ERROR_SEND_FAIL, "account_info is NULL!");
	retvm_if(!ugd->account_info->selected_account, COMPOSER_ERROR_SEND_FAIL, "selected_account is NULL!");

	COMPOSER_ERROR_TYPE_E ret = COMPOSER_ERROR_NONE;
	int handle = 0;
	int err = 0;

	ret = _add_mail(ugd, EMAIL_MAILBOX_TYPE_OUTBOX);
	retvm_if(ret != COMPOSER_ERROR_NONE, ret, "_add_mail() failed: %d", ret);

	if (((ugd->composer_type == RUN_COMPOSER_FORWARD) || (ugd->composer_type == RUN_COMPOSER_EDIT)) && ugd->need_download && ugd->new_mail_info->mail_data->reference_mail_id) {
		debug_log(">> email_send_mail_with_downloading_attachment_of_original_mail()");
		err = email_send_mail_with_downloading_attachment_of_original_mail(ugd->new_mail_info->mail_data->mail_id, &handle);
		debug_warning_if(err != EMAIL_ERROR_NONE, "email_send_mail_with_downloading_attachment_of_original_mail() failed! err:[%d]", err);
	} else {
		debug_log(">> email_send_mail()");
		err = email_send_mail(ugd->new_mail_info->mail_data->mail_id, &handle);
	}

	if (err == EMAIL_ERROR_NONE) {
		debug_log("Sending email succeeded!");

		/* Update Reply/Forward state and time */
		email_mail_attribute_value_t sending_time;
		sending_time.datetime_type_value = time(NULL);

		if (ugd->composer_type == RUN_COMPOSER_REPLY || ugd->composer_type == RUN_COMPOSER_REPLY_ALL) {
			if (ugd->org_mail_info->mail_data->flags_forwarded_field) {
				email_set_flags_field(ugd->account_info->original_account->account_id, &ugd->original_mail_id, 1, EMAIL_FLAGS_FORWARDED_FIELD, 0, 1);
			}

			err = email_update_mail_attribute(ugd->org_mail_info->mail_data->account_id, &ugd->org_mail_info->mail_data->mail_id, 1, EMAIL_MAIL_ATTRIBUTE_REPLIED_TIME, sending_time);
			debug_error_if(err != EMAIL_ERROR_NONE, "email_update_mail_attribute() failed! err:[%d]", err);

			err = email_set_flags_field(ugd->account_info->original_account->account_id, &ugd->original_mail_id, 1, EMAIL_FLAGS_ANSWERED_FIELD, 1, 1);
			debug_error_if(err != EMAIL_ERROR_NONE, "email_set_flags_field() failed! err:[%d]", err);
		} else if (ugd->composer_type == RUN_COMPOSER_FORWARD) {
			if (ugd->org_mail_info->mail_data->flags_answered_field) {
				email_set_flags_field(ugd->account_info->original_account->account_id, &ugd->original_mail_id, 1, EMAIL_FLAGS_ANSWERED_FIELD, 0, 1);
			}

			err = email_update_mail_attribute(ugd->org_mail_info->mail_data->account_id, &ugd->org_mail_info->mail_data->mail_id, 1, EMAIL_MAIL_ATTRIBUTE_FORWARDED_TIME, sending_time);
			debug_error_if(err != EMAIL_ERROR_NONE, "email_update_mail_attribute() failed! err:[%d]", err);

			err = email_set_flags_field(ugd->account_info->original_account->account_id, &ugd->original_mail_id, 1, EMAIL_FLAGS_FORWARDED_FIELD, 1, 1);
			debug_error_if(err != EMAIL_ERROR_NONE, "email_set_flags_field() failed! err:[%d]", err);
		}
	} else {
		debug_error("Sending email failed: [%d]", err);
		return COMPOSER_ERROR_SEND_FAIL;
	}

	debug_leave();
	return COMPOSER_ERROR_NONE;
}

static COMPOSER_ERROR_TYPE_E _add_mail(EmailComposerUGD *ugd, email_mailbox_type_e mailbox_type)
{
	debug_enter();

	EmailComposerAccount *account_info = ugd->account_info;
	retvm_if(!account_info, COMPOSER_ERROR_NULL_POINTER, "account_info is NULL!");

	int ret = EMAIL_ERROR_NONE;

	ret = _make_mail(ugd, mailbox_type);
	retv_if(ret != COMPOSER_ERROR_NONE, ret);

	ret = email_add_mail(ugd->new_mail_info->mail_data, ugd->new_mail_info->attachment_list, ugd->new_mail_info->total_attachment_count, NULL, 0);
	if (ret != EMAIL_ERROR_NONE) {
		debug_error("email_add_mail() failed! ret:[%d]", ret);
		return (ret == EMAIL_ERROR_MAIL_MEMORY_FULL) ? COMPOSER_ERROR_MAIL_STORAGE_FULL : COMPOSER_ERROR_FAIL;
	}

	if (ugd->composer_type == RUN_COMPOSER_EDIT) {
		int sync = EMAIL_DELETE_LOCAL_AND_SERVER;

		/* Retrieve mailbox type from email-service to identify Outbox */
		if (EMAIL_MAILBOX_TYPE_OUTBOX == ugd->org_mail_info->mail_data->mailbox_type)
			sync = EMAIL_DELETE_LOCALLY;

		debug_secure("Original account_id:[%d], mailbox_id:[%d], mail_id:[%d]", account_info->original_account->account_id, ugd->org_mail_info->mail_data->mailbox_id, ugd->original_mail_id);

		if (!email_engine_delete_mail(account_info->original_account->account_id, ugd->org_mail_info->mail_data->mailbox_id, ugd->original_mail_id, sync)) {
			debug_error("email_engine_delete_mail() failed!");
			return COMPOSER_ERROR_SEND_FAIL;
		}
	}

	debug_leave();
	return COMPOSER_ERROR_NONE;
}

static COMPOSER_ERROR_TYPE_E _make_mail(EmailComposerUGD *ugd, email_mailbox_type_e mailbox_type)
{
	debug_enter();

	int err = COMPOSER_ERROR_NONE;

	debug_log("account_id = %d", ugd->account_info->selected_account->account_id);
	ugd->new_mail_info->mail_data->account_id = ugd->account_info->selected_account->account_id;

	err = _make_mail_set_from_address(ugd);
	retv_if(err != COMPOSER_ERROR_NONE, err);

	err = _make_mail_set_mailbox_info(ugd, mailbox_type);
	retv_if(err != COMPOSER_ERROR_NONE, err);

	/* --> Set other properties */
	ugd->new_mail_info->mail_data->report_status = EMAIL_MAIL_REPORT_NONE;
	ugd->new_mail_info->mail_data->priority = ugd->priority_option;

	if (ugd->composer_type == RUN_COMPOSER_EDIT) {
		ugd->new_mail_info->mail_data->body_download_status = EMAIL_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED;
	}
	ugd->new_mail_info->mail_data->flags_draft_field = 1;
	ugd->new_mail_info->mail_data->remaining_resend_times = (ugd->account_info->selected_account->auto_resend_times == EMAIL_NO_LIMITED_RETRY_COUNT) ? 10 : ugd->account_info->selected_account->auto_resend_times;

	_make_mail_set_reference_id(ugd);
	/* <-- Set other properties */

	/* --> Make recipient list from MBE */
	FREE(ugd->new_mail_info->mail_data->full_address_to);
	FREE(ugd->new_mail_info->mail_data->full_address_cc);
	FREE(ugd->new_mail_info->mail_data->full_address_bcc);

	err = composer_util_recp_retrieve_recp_string(ugd, ugd->recp_to_mbe, &(ugd->new_mail_info->mail_data->full_address_to));
	retv_if(err != COMPOSER_ERROR_NONE, err);

	err = composer_util_recp_retrieve_recp_string(ugd, ugd->recp_cc_mbe, &(ugd->new_mail_info->mail_data->full_address_cc));
	retv_if(err != COMPOSER_ERROR_NONE, err);

	err = composer_util_recp_retrieve_recp_string(ugd, ugd->recp_bcc_mbe, &(ugd->new_mail_info->mail_data->full_address_bcc));
	retv_if(err != COMPOSER_ERROR_NONE, err);
	/* <-- Make recipient list from MBE */

	ugd->new_mail_info->mail_data->subject = elm_entry_markup_to_utf8(elm_entry_entry_get(ugd->subject_entry.entry));

	/* --> Make body html */
	err = _remove_old_body_file(ugd);
	retv_if(err != COMPOSER_ERROR_NONE, err);

	char *html_content = NULL;
	char *html_content_processed = NULL;
	char *plain_text_content = NULL;

	_make_mail_make_draft_html_content(ugd, &html_content, mailbox_type);
	retvm_if(!html_content, COMPOSER_ERROR_MAKE_MAIL_FAIL, "html_content is NULL!");

	/*debug_secure("html_content: (%s)", html_content);*/
	/* <-- Make body html */

	/* --> Save html body as file */
	if (ugd->attachment_inline_item_list == NULL) {
		debug_log("No images found or ERROR");
		html_content_processed = html_content;
		html_content = NULL;
	} else {
		if (!_make_mail_change_image_paths_to_relative(html_content, &html_content_processed, mailbox_type)) {
			debug_error("email_composer_change_image_paths_to_relative() failed!");
			FREE(html_content);
			return COMPOSER_ERROR_MAKE_MAIL_FAIL;
		}
		debug_secure("html_source_processed: {%s}", html_content_processed);

		FREE(html_content);
	}

	if (!email_save_file(ugd->saved_html_path, html_content_processed, STR_LEN(html_content_processed))) {
		debug_secure("email_save_file() failed! for (%s)", ugd->saved_html_path);
		FREE(html_content_processed);
		return COMPOSER_ERROR_MAKE_MAIL_FAIL;
	}
	FREE(html_content_processed);
	/* <-- Save html body as file */

	/* --> Save plain text body as file */
	if (!ugd->plain_content || (ugd->plain_content && STR_LEN(ugd->plain_content) == 0)) {
		plain_text_content = STR_DUP("\n");
	} else {
		plain_text_content = STR_DUP(ugd->plain_content);
	}
	retvm_if(!plain_text_content, COMPOSER_ERROR_MAKE_MAIL_FAIL, "plain_text_content is NULL!");

	if (!email_save_file(ugd->saved_text_path, plain_text_content, strlen(plain_text_content))) {
		debug_secure("email_save_file() failed! for (%s)", ugd->saved_text_path);
		FREE(plain_text_content);
		return COMPOSER_ERROR_MAKE_MAIL_FAIL;
	}
	FREE(plain_text_content);
	/* <-- Save plain text body as file */

	ugd->new_mail_info->mail_data->file_path_html = STR_DUP(ugd->saved_html_path);
	ugd->new_mail_info->mail_data->file_path_plain = STR_DUP(ugd->saved_text_path);

	_make_mail_make_attachment_list(ugd);

	/* This logic checks if mail size exceeds when attachments and text entered both are taken into account.
	 * - This is defensive code for normal case because "send button" and "save in drafts menu" were already disabled if mail size exceeds.
	 * - When we press a new mail noti while composing a mail, viewer is getting opened and the mail is being composed is saved in drafts.
	 *   But for a mail whose size exceeds max, the mail is be discarded.
	 */
	retvm_if(composer_util_is_max_sending_size_exceeded(ugd), COMPOSER_ERROR_MAIL_SIZE_EXCEEDED, "Mail size exceeded!");

	debug_leave();
	return COMPOSER_ERROR_NONE;
}

static void _save_in_drafts_process_result(EmailComposerUGD *ugd, int ret)
{
	debug_enter();

	if ((ret == COMPOSER_ERROR_NONE) || ugd->is_force_destroy) {
		DELETE_EVAS_OBJECT(ugd->composer_popup);

		int noti_ret = NOTIFICATION_ERROR_NONE;
		if (ret == COMPOSER_ERROR_NONE) {
			debug_log("Succeeded in saving in Drafts");
			noti_ret = notification_status_message_post(email_get_email_string("IDS_EMAIL_TPOP_EMAIL_SAVED_IN_DRAFTS"));
		} else {
			debug_warning("Failed in saving in Drafts");
			noti_ret = notification_status_message_post(email_get_email_string("IDS_EMAIL_TPOP_FAILED_TO_SAVE_EMAIL"));
		}
		debug_warning_if(noti_ret != NOTIFICATION_ERROR_NONE, "notification_status_message_post() failed! ret:(%d)", noti_ret);

		/* Restore the indicator mode. */
		composer_util_indicator_restore(ugd);

		email_module_make_destroy_request(ugd->base.module);
	} else if (ret == COMPOSER_ERROR_MAIL_SIZE_EXCEEDED) {
		composer_attachment_launch_attachment_error_popup(COMPOSER_ERROR_ATTACHMENT_MAX_SIZE_EXCEEDED, _unable_to_save_in_drafts_cb, ugd);
	} else if (ret == COMPOSER_ERROR_MAIL_STORAGE_FULL) {
		ugd->composer_popup = composer_util_popup_create(ugd, EMAIL_COMPOSER_STRING_MF_HEADER_UNABLE_TO_SAVE_DATA_ABB, EMAIL_COMPOSER_STRING_STORAGE_FULL_CONTENTS, _composer_send_mail_storage_full_popup_response_cb,
		                                                EMAIL_COMPOSER_STRING_BUTTON_CANCEL, EMAIL_COMPOSER_STRING_BUTTON_SETTINGS, EMAIL_COMPOSER_STRING_NULL);
	} else {
		debug_error("_add_mail() failed: %d", ret);
		ugd->composer_popup = composer_util_popup_create(ugd, EMAIL_COMPOSER_STRING_HEADER_UNABLE_TO_PERFORM_ACTION_ABB, EMAIL_COMPOSER_STRING_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED,
		                                                _unable_to_save_in_drafts_cb, EMAIL_COMPOSER_STRING_BUTTON_OK, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);
	}

	debug_leave();
}

static void _save_in_drafts(EmailComposerUGD *ugd)
{
	debug_enter();

	retm_if(!ugd, "Invalid parameter: data is NULL!");

	int ret = _add_mail(ugd, EMAIL_MAILBOX_TYPE_DRAFT);
	_save_in_drafts_process_result(ugd, ret);

	debug_leave();
}

static void _save_in_drafts_in_thread(EmailComposerUGD *ugd)
{
	debug_enter();

	/* It takes too long time to save files to disk on kiran device that has small storage and poor disk controller. (Around 6MB/s)
	 * When we save large size of attachment, email_add_mail() takes too long time because of the above problem. So we handle this API in thread.
	 *
	 * We should show progress popup while saving a mail because saving email in thread takes too long time if attachment size exceeds threshold.
	 * In case of force destroy, we do not show progress popup while saving a mail because composer is not shown on the top of the screen.
	 */
	if (!ugd->is_force_destroy) {
		int attachment_size = composer_util_get_total_attachments_size(ugd, EINA_TRUE);
		if (attachment_size >= ATTACHMENT_SIZE_THRESHOLD) {
			ugd->composer_popup = composer_util_popup_create_with_progress_horizontal(ugd, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_POP_SAVING_EMAIL_ING, NULL,
			                                                                          EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);

			ugd->thread_saving_email = ecore_thread_feedback_run(_save_in_drafts_thread_worker, _save_in_drafts_thread_notify,
			                                                     _save_in_drafts_thread_finish, NULL, ugd, EINA_TRUE);
		} else {
			ugd->thread_saving_email = NULL;
		}
	}

	if (!ugd->thread_saving_email) {
		_save_in_drafts(ugd);
	}

	debug_leave();
}

static Eina_Bool _send_mail_finish_idler_cb(void *data)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	ugd->idler_destroy_self = NULL;

	/* Display "Downloading..." toast popup when downloading attachments will be displayed after destroying composer. */
	if (ugd->is_forward_download_processed) {
		int noti_ret = notification_status_message_post(email_get_email_string("IDS_EMAIL_POP_DOWNLOADING_ING"));
		debug_warning_if(noti_ret != NOTIFICATION_ERROR_NONE, "notification_status_message_post() failed! ret:(%d)", noti_ret);
	}

	/* Restore the indicator mode. */
	composer_util_indicator_restore(ugd);

	email_module_make_destroy_request(ugd->base.module);

	debug_leave();
	return ECORE_CALLBACK_CANCEL;
}

static void _composer_send_mail_storage_full_popup_response_cb(void *data, Evas_Object *obj, void *event_info)
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

		ugd->is_send_btn_clicked = EINA_FALSE;
		ugd->is_back_btn_clicked = EINA_FALSE;
		ugd->is_save_in_drafts_clicked = EINA_FALSE;

		elm_object_tree_focus_allow_set(ugd->composer_layout, EINA_TRUE);
		elm_object_focus_allow_set(ugd->ewk_btn, EINA_TRUE);
	} else {
		DELETE_EVAS_OBJECT(ugd->composer_popup);
		composer_util_return_composer_view(ugd);
	}

	debug_leave();
}

static void _save_popup_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	char *button_text = NULL;

	/* To distinguish soft button and hareware back button */
	if (obj != ugd->composer_popup) {
		button_text = (char *)elm_object_text_get(obj);
	}
	DELETE_EVAS_OBJECT(ugd->composer_popup);

	if (!button_text || !g_strcmp0(button_text, email_get_email_string(EMAIL_COMPOSER_STRING_BUTTON_CANCEL.id))) {
		composer_util_return_composer_view(ugd);
	} else if (!g_strcmp0(button_text, email_get_email_string(EMAIL_COMPOSER_STRING_BUTTON_DISCARD_ABB2.id))) {
		/* Restore the indicator mode. */
		composer_util_indicator_restore(ugd);

		email_module_make_destroy_request(ugd->base.module);
	} else if (!g_strcmp0(button_text, email_get_email_string(EMAIL_COMPOSER_STRING_BUTTON_SAVE_ABB.id))) {
		_save_in_drafts_in_thread(ugd);
	}

	debug_leave();
}

static void _unable_to_save_in_drafts_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	DELETE_EVAS_OBJECT(ugd->composer_popup);
	composer_util_return_composer_view(ugd);

	debug_leave();
}

static void _download_attachment_popup_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	char *button_text = NULL;

	/* To distinguish soft button and hareware back button */
	if (obj != ugd->composer_popup) {
		button_text = (char *)elm_object_text_get(obj);
	}

	if (!g_strcmp0(button_text, email_get_email_string(EMAIL_COMPOSER_STRING_BUTTON_CONTINUE_ABB.id))) {
		ugd->is_forward_download_processed = EINA_TRUE;
		_exit_composer_process_sending_mail(ugd);
		DELETE_EVAS_OBJECT(ugd->composer_popup);
	} else {
		DELETE_EVAS_OBJECT(ugd->composer_popup);
		composer_util_return_composer_view(ugd);
	}

	debug_leave();
}

static COMPOSER_ERROR_TYPE_E _remove_old_body_file(EmailComposerUGD *ugd)
{
	debug_enter();

	if (email_check_file_exist(ugd->saved_html_path)) {
		int ret = remove(ugd->saved_html_path);
		retvm_if(ret == -1, COMPOSER_ERROR_FAIL, "remove() failed!");
	}

	if (email_check_file_exist(ugd->saved_text_path)) {
		int ret = remove(ugd->saved_text_path);
		retvm_if(ret == -1, COMPOSER_ERROR_FAIL, "remove() failed!");
	}

	debug_leave();
	return COMPOSER_ERROR_NONE;
}

static COMPOSER_ERROR_TYPE_E _make_mail_set_from_address(EmailComposerUGD *ugd)
{
	debug_enter();

	EmailComposerAccount *account_info = ugd->account_info;
	retvm_if(!account_info, COMPOSER_ERROR_NULL_POINTER, "account_info is NULL!");

	char *processed_display_name = NULL;

	debug_secure("user_name: (%s)", account_info->selected_account->user_display_name);
	debug_secure("email_address: (%s)", account_info->selected_account->user_email_address);

	if (account_info->selected_account->user_display_name && g_strstr_len(account_info->selected_account->user_display_name, strlen(account_info->selected_account->user_display_name), "\"")) {
		debug_secure("double quotation mark is there!");

		char **sp_name = g_strsplit(account_info->selected_account->user_display_name, "\"", -1);
		processed_display_name = g_strjoinv("\\\"", sp_name);
		g_strfreev(sp_name);
	}

	ugd->new_mail_info->mail_data->full_address_from = g_strdup_printf("\"%s\" <%s>", processed_display_name ? processed_display_name : account_info->selected_account->user_display_name, ugd->account_info->selected_account->user_email_address);
	debug_secure("from_address: (%s)", ugd->new_mail_info->mail_data->full_address_from);

	FREE(processed_display_name);

	debug_leave();
	return COMPOSER_ERROR_NONE;
}

static COMPOSER_ERROR_TYPE_E _make_mail_set_mailbox_info(EmailComposerUGD *ugd, email_mailbox_type_e mailbox_type)
{
	debug_enter();

	int err = EMAIL_ERROR_NONE;
	email_mailbox_t *mailbox = NULL;
	EmailComposerAccount *account_info = ugd->account_info;
	retvm_if(!account_info, COMPOSER_ERROR_NULL_POINTER, "account_info is NULL!");

	err = email_get_mailbox_by_mailbox_type(account_info->selected_account->account_id, mailbox_type, &mailbox);
	if (err != EMAIL_ERROR_NONE) {
		debug_log("email_get_mailbox_by_mailbox_type() failed! %d", err);
		return err;
	}
	debug_log("mailbox_id = %d", mailbox->mailbox_id);

	ugd->new_mail_info->mail_data->mailbox_id = mailbox->mailbox_id;
	ugd->new_mail_info->mail_data->mailbox_type = mailbox->mailbox_type;

	err = email_free_mailbox(&mailbox, 1);
	if (err != EMAIL_ERROR_NONE) {
		debug_warning("email_free_mailbox() failed: [%d], it will cause memory leak", err);
	}

	debug_leave();
	return COMPOSER_ERROR_NONE;
}

static void _make_mail_set_reference_id(EmailComposerUGD *ugd)
{
	debug_enter();

	ugd->new_mail_info->mail_data->reference_mail_id = 0; /* Set reference_mail_id as 0 by default. */

	if (ugd->account_info && (ugd->account_info->original_account->account_id == ugd->account_info->selected_account->account_id)) {
		if (ugd->composer_type == RUN_COMPOSER_FORWARD) {
			ugd->new_mail_info->mail_data->reference_mail_id = ugd->org_mail_info->mail_data->mail_id;
		} else if (ugd->composer_type == RUN_COMPOSER_EDIT) {
			ugd->new_mail_info->mail_data->reference_mail_id = ugd->org_mail_info->mail_data->reference_mail_id;
		}
	}

	debug_log("new mail - reference_mail_id:[%d]", ugd->new_mail_info->mail_data->reference_mail_id);

	debug_leave();
}

static void _make_mail_make_draft_html_content(EmailComposerUGD *ugd, char **html_content, email_mailbox_type_e mailbox_type)
{
	debug_enter();

	retm_if(!ugd, "Invalid parameter: ugd is NULL!");
	retm_if(!html_content, "Invalid parameter: html_content is NULL!");

	if (ugd->is_send_btn_clicked) {
		if (ugd->with_original_message) {
			if (ugd->is_checkbox_clicked) {
				*html_content = g_strconcat(EC_TAG_HTML_START, EC_TAG_BODY_START, ugd->final_new_message_content, EC_TAG_DIV_DIR""EC_TAG_DIV_DIR, ugd->final_parent_content, EC_TAG_BODY_END, EC_TAG_HTML_END, NULL);
			} else {
				*html_content = g_strconcat(EC_TAG_HTML_START, EC_TAG_BODY_START, ugd->final_new_message_content, EC_TAG_BODY_END, EC_TAG_HTML_END, NULL);
			}
		} else {
			*html_content = g_strconcat(EC_TAG_HTML_START, EC_TAG_BODY_START, ugd->final_body_content, EC_TAG_BODY_END, EC_TAG_HTML_END, NULL);
		}
	} else {
		if (ugd->with_original_message) {
			*html_content = g_strconcat(EC_TAG_HTML_START, EC_TAG_BODY_ORIGINAL_MESSAGE_START, ugd->final_body_content, EC_TAG_BODY_END, EC_TAG_HTML_END, NULL);
		} else {
			*html_content = g_strconcat(EC_TAG_HTML_START, EC_TAG_BODY_CSS_START, ugd->final_body_content, EC_TAG_BODY_END, EC_TAG_HTML_END, NULL);
		}
	}

	debug_leave();
}

static Eina_Bool _make_mail_change_image_paths_to_relative(char *src_html_content, char **dest_buffer, email_mailbox_type_e dest_mailbox)
{
	debug_enter();

	retvm_if(!src_html_content, EINA_FALSE, "Invalid parameter: src_html_content is NULL!");
	retvm_if(!dest_buffer, EINA_FALSE, "Invalid parameter: dest_buffer is NULL!");

	*dest_buffer = NULL;

	char *src_str = src_html_content;
	char *buf = (char *)calloc(1, strlen(src_str) + 1);
	retvm_if(!buf, EINA_FALSE, "Failed to allocate memory for buf!");

	char *point1 = NULL;
	char *point2 = NULL;
	char *temp = NULL;
	Eina_Bool success_flag = EINA_TRUE;

	/* Ex. <img src="/opt/media/Images/image5.jpg" width=342 height="192" id="/opt/media/Images/image5.jpg"> */
	while ((point1 = strcasestr(src_str, "<img"))) {
		temp = strcasestr(point1, "src=");
		if (!temp) {
			debug_warning("1. No src=");
			success_flag = EINA_FALSE;
			break;
		}

		point1 = temp + 5;
		strncat(buf, src_str, point1 - src_str);

		debug_log("point1[0] = %c", point1[0]);

		if (point1[0] == '/') {
			point2 = strstr(point1, "\"");
			if (!point2) {
				debug_warning("2. No end quotation");
				success_flag = EINA_FALSE;
				break;
			}

			/* Allocate img src uri */
			char *image_path = g_strndup(point1, point2 - point1);
			char *file_path = NULL;

			debug_secure("image_path : (%s)", image_path);

			if (dest_mailbox == EMAIL_MAILBOX_TYPE_OUTBOX) {
				file_path = g_uri_unescape_string(email_file_file_get(image_path), NULL);
			} else {
				file_path = g_strdup(email_file_file_get(image_path));
			}
			free(image_path);

			debug_secure("file_path: (%s)", file_path);
			if (!file_path) {
				debug_warning("3. Could not parse inline image path");
				success_flag = EINA_FALSE;
				break;
			}

			strncat(buf, file_path, strlen(file_path));
			free(file_path);

			point1 = point2;
		}
		src_str = point1;
	}

	if (success_flag == EINA_FALSE) {
		free(buf);
		return EINA_FALSE;
	}

	strncat(buf, src_str, strlen(src_str));
	*dest_buffer = buf;

	return EINA_TRUE;
}

static void _make_mail_make_attachment_list(EmailComposerUGD *ugd)
{
	debug_enter();

	int attach_count = eina_list_count(ugd->attachment_item_list);
	int inline_count = eina_list_count(ugd->attachment_inline_item_list);

	debug_log("1. attach_count: [%d]", attach_count);
	debug_log("1. inline_count: [%d]", inline_count);

	Eina_List *list = NULL;
	email_attachment_data_t *inline_att = NULL;
	ComposerAttachmentItemData *attachment_item_data = NULL;

	/* If attachment hasn't been downloaded yet, it shouldn't be included in sending mail.
	 * But to support email_send_mail_with_downloading_attachment_of_original_mail() API, forward case is excluded.
	 */
	EINA_LIST_FOREACH(ugd->attachment_item_list, list, attachment_item_data) {
		email_attachment_data_t *att = attachment_item_data->attachment_data;
		struct stat sb;
		if (!att->attachment_path || (stat(att->attachment_path, &sb) == -1)) {
			if (att->attachment_path) {
				debug_log("stat() failed! (%d): %s", errno, strerror(errno));
			}

			bool is_changed = true;
			if (ugd->new_mail_info->mail_data->reference_mail_id != 0) {
				unsigned int i;
				for (i = 0; i < ugd->org_mail_info->total_attachment_count; i++) {
					email_attachment_data_t *org_att = ugd->org_mail_info->attachment_list + i;
					if ((org_att && org_att->attachment_id == att->attachment_id) ||
						/* The second option is for the email in drafts from forwarding a mail. */
						(!g_strcmp0(org_att->attachment_name, att->attachment_name) && (org_att->attachment_size == att->attachment_size))) {
						is_changed = false;
						break;
					}
				}
			}

			if (is_changed) {
				att->attachment_id = -1;
				attach_count--;
			}
		}
	}

	debug_log("2. attach_count: [%d]", attach_count);
	debug_log("2. inline_count: [%d]", inline_count);

	EINA_LIST_FOREACH(ugd->attachment_inline_item_list, list, inline_att) {
		struct stat sb;
		if (!inline_att->attachment_path || (stat(inline_att->attachment_path, &sb) == -1)) {
			if (inline_att->attachment_path) {
				debug_log("stat() failed! (%d): %s", errno, strerror(errno));
			}

			if (ugd->new_mail_info->mail_data->reference_mail_id == 0) {
				inline_att->attachment_id = -1;
				inline_count--;
			}
		}
	}

	debug_log("3. attach_count: [%d]", attach_count);
	debug_log("3. inline_count: [%d]", inline_count);

	/* For logging */
	EINA_LIST_FOREACH(ugd->attachment_inline_item_list, list, inline_att) {
		debug_secure("[%d] name:%s, path:%s, is_inline:[%d], save:[%d]",
			inline_att->attachment_id, inline_att->attachment_name, inline_att->attachment_path, inline_att->inline_content_status, inline_att->save_status);
	}
	EINA_LIST_FOREACH(ugd->attachment_item_list, list, attachment_item_data) {
		email_attachment_data_t *att = attachment_item_data->attachment_data;
		debug_secure("[%d] name:%s, path:%s, is_inline:[%d], save:[%d]", att->attachment_id, att->attachment_name, att->attachment_path, att->inline_content_status, att->save_status);
	}

	if (attach_count + inline_count > 0) {
		ugd->new_mail_info->total_attachment_count = attach_count + inline_count;
		ugd->new_mail_info->mail_data->attachment_count = attach_count;
		ugd->new_mail_info->mail_data->inline_content_count = inline_count;
		ugd->new_mail_info->attachment_list = (email_attachment_data_t *)calloc(ugd->new_mail_info->total_attachment_count, sizeof(email_attachment_data_t));
		retm_if(!ugd->new_mail_info->attachment_list, "Failed to allocate memory for attachment_list!");

		email_attachment_data_t *new_att = ugd->new_mail_info->attachment_list;

		EINA_LIST_FOREACH(ugd->attachment_item_list, list, attachment_item_data) {
			email_attachment_data_t *attachment_data = attachment_item_data->attachment_data;
			if (attachment_data && attachment_data->attachment_id != -1) {
				new_att->attachment_id = attachment_data->attachment_id;
				new_att->attachment_name = COMPOSER_STRDUP(attachment_data->attachment_name);
				new_att->attachment_path = COMPOSER_STRDUP(attachment_data->attachment_path);
				new_att->attachment_size = attachment_data->attachment_size;
				new_att->mail_id = attachment_data->mail_id;
				new_att->account_id = attachment_data->account_id;
				new_att->mailbox_id = attachment_data->mailbox_id;
				new_att->save_status = attachment_data->save_status;
				new_att->inline_content_status = attachment_data->inline_content_status;
				new_att++;
			}
		}

		EINA_LIST_FOREACH(ugd->attachment_inline_item_list, list, inline_att) {
			if (inline_att && inline_att->attachment_id != -1) {
				new_att->attachment_id = inline_att->attachment_id;
				new_att->attachment_name = COMPOSER_STRDUP(inline_att->attachment_name);
				new_att->attachment_path = COMPOSER_STRDUP(inline_att->attachment_path);
				new_att->attachment_size = inline_att->attachment_size;
				new_att->mail_id = inline_att->mail_id;
				new_att->account_id = inline_att->account_id;
				new_att->mailbox_id = inline_att->mailbox_id;
				new_att->save_status = inline_att->save_status;
				new_att->inline_content_status = inline_att->inline_content_status;
				new_att->attachment_mime_type = COMPOSER_STRDUP(inline_att->attachment_mime_type);
				new_att++;
			}
		}
	}

	/* For logging */
	int i;
	debug_log("----------------------------------------------------------------------------------------------");
	for (i = 0; i < ugd->new_mail_info->total_attachment_count; i++) {
		email_attachment_data_t *att = ugd->new_mail_info->attachment_list + i;
		debug_secure("[%d] name:%s, path:%s, is_inline:[%d], save:[%d]", att->attachment_id, att->attachment_name, att->attachment_path, att->inline_content_status, att->save_status);
	}

	debug_leave();
}


/*
 * Definition for exported functions
 */

void composer_exit_composer_get_contents(void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	if (ugd->is_send_btn_clicked) {
		MBE_VALIDATION_ERROR err = MBE_VALIDATION_ERROR_NONE;
		MBE_VALIDATION_ERROR prev_err = MBE_VALIDATION_ERROR_NONE;
		Evas_Object *error_entry = NULL;

		prev_err = err = _check_vaildify_of_recipient_field(ugd, ugd->recp_to_mbe);
		if (err == MBE_VALIDATION_ERROR_INVALID_ADDRESS) {
			error_entry = ugd->recp_to_entry.entry;
		}

		err = _check_vaildify_of_recipient_field(ugd, ugd->recp_cc_mbe);
		if (err == MBE_VALIDATION_ERROR_INVALID_ADDRESS) {
			if (!error_entry) {
				error_entry = ugd->recp_cc_entry.entry;
			}
		}

		if (prev_err < err) {
			prev_err = err;
		}

		err = _check_vaildify_of_recipient_field(ugd, ugd->recp_bcc_mbe);
		if (err == MBE_VALIDATION_ERROR_INVALID_ADDRESS) {
			if (!error_entry) {
				error_entry = ugd->recp_bcc_entry.entry;
			}
		}

		if (prev_err < err) {
			prev_err = err;
		}

		if (prev_err > MBE_VALIDATION_ERROR_DUPLICATE_ADDRESS) {
			ugd->selected_entry = error_entry;

			ugd->is_send_btn_clicked = EINA_FALSE;
			composer_recipient_display_error_string(ugd, prev_err);

			debug_leave();
			return;
		}

		if (composer_util_recp_is_mbe_empty(ugd)) {
			ugd->selected_entry = ugd->recp_to_entry.entry;

			ugd->is_send_btn_clicked = EINA_FALSE;
			composer_recipient_display_error_string(ugd, MBE_VALIDATION_ERROR_NO_ADDRESS);

			debug_leave();
			return;
		}

		/* To hide IME when send button is clicked. (send button isn't a focusable object) */
		ecore_imf_input_panel_hide();
	}

#ifdef FEATRUE_PREDICTIVE_SEARCH
	if (ugd->ps_is_runnig) {
		composer_ps_stop_search(ugd);
	}
#endif

	/* To prevent showing IME when the focus was on the webkit. ewk_btn isn't a child of composer_layout. so we need to control the focus of it as well. */
	elm_object_focus_allow_set(ugd->ewk_btn, EINA_FALSE);
	/* To prevent showing ime while sending email. */
	elm_object_tree_focus_allow_set(ugd->composer_layout, EINA_FALSE);

	if (!ewk_view_script_execute(ugd->ewk_view, EC_JS_REMOVE_IMAGE_CONTROL_LAYER, _delete_image_control_layer_cb, (void *)ugd)) {
		debug_error("EC_JS_REMOVE_IMAGE_CONTROL_LAYER failed.");
	}

	debug_leave();
}

static void _check_forwarding_attachment_download(void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	ugd->is_forward_download_processed = EINA_FALSE;

	if ((ugd->account_info->original_account->account_id == ugd->account_info->selected_account->account_id) &&
		((ugd->composer_type == RUN_COMPOSER_FORWARD) || (ugd->composer_type == RUN_COMPOSER_EDIT)) && ugd->need_download) {
		ugd->composer_popup = composer_util_popup_create(ugd, EMAIL_COMPOSER_STRING_HEADER_DOWNLOAD_ATTACHMENTS_ABB, EMAIL_COMPOSER_STRING_POP_ALL_ATTACHMENTS_IN_THIS_EMAIL_WILL_BE_DOWNLOADED_BEFORE_IT_IS_FORWARDED,
		                                                _download_attachment_popup_cb, EMAIL_COMPOSER_STRING_BUTTON_CANCEL, EMAIL_COMPOSER_STRING_BUTTON_CONTINUE_ABB, EMAIL_COMPOSER_STRING_NULL);
	} else {
		_exit_composer_process_sending_mail(ugd);
	}

	debug_leave();
}

static void _send_mail_failed_popup_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	DELETE_EVAS_OBJECT(ugd->composer_popup);
	composer_util_return_composer_view(ugd);

	debug_leave();
}

/* EOF */
