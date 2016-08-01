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

static MBE_VALIDATION_ERROR _check_vaildify_of_recipient_field(EmailComposerView *view, Evas_Object *obj);

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

static COMPOSER_ERROR_TYPE_E _send_mail(EmailComposerView *view);
static COMPOSER_ERROR_TYPE_E _add_mail(EmailComposerView *view, email_mailbox_type_e mailbox_type);
static COMPOSER_ERROR_TYPE_E _make_mail(EmailComposerView *view, email_mailbox_type_e mailbox_type);
static void _save_in_drafts_process_result(EmailComposerView *view, int ret);
static void _save_in_drafts(EmailComposerView *view);
static void _save_in_drafts_in_thread(EmailComposerView *view);

static Eina_Bool _send_mail_finish_idler_cb(void *data);
static void _composer_send_mail_storage_full_popup_response_cb(void *data, Evas_Object *obj, void *event_info);
static void _save_popup_cb(void *data, Evas_Object *obj, void *event_info);
static void _unable_to_save_in_drafts_cb(void *data, Evas_Object *obj, void *event_info);
static void _download_attachment_popup_cb(void *data, Evas_Object *obj, void *event_info);

static COMPOSER_ERROR_TYPE_E _remove_old_body_file(EmailComposerView *view);
static COMPOSER_ERROR_TYPE_E _make_mail_set_from_address(EmailComposerView *view);
static COMPOSER_ERROR_TYPE_E _make_mail_set_mailbox_info(EmailComposerView *view, email_mailbox_type_e mailbox_type);
static void _make_mail_set_reference_id(EmailComposerView *view);

static void _make_mail_make_draft_html_content(EmailComposerView *view, char **html_content, email_mailbox_type_e mailbox_type);
static Eina_Bool _make_mail_change_image_paths_to_relative(char *src_html_content, char **dest_buffer, email_mailbox_type_e dest_mailbox);
static void _make_mail_make_attachment_list(EmailComposerView *view);
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

static MBE_VALIDATION_ERROR _check_vaildify_of_recipient_field(EmailComposerView *view, Evas_Object *obj)
{
	debug_enter();

	MBE_VALIDATION_ERROR err = MBE_VALIDATION_ERROR_NONE;
	Evas_Object *entry = NULL;

	if (obj == view->recp_to_mbe) {
		entry = view->recp_to_entry.entry;
	} else if (obj == view->recp_cc_mbe) {
		entry = view->recp_cc_entry.entry;
	} else if (obj == view->recp_bcc_mbe) {
		entry = view->recp_bcc_entry.entry;
	}

	char *recp = elm_entry_markup_to_utf8(elm_entry_entry_get(entry));
	if (STR_LEN(recp)) {
		char *invalid_list = NULL;
		char *mod_recp = g_strconcat(recp, ";", NULL);
		composer_recipient_mbe_validate_email_address_list(view, obj, mod_recp, &invalid_list, &err);
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

	EmailComposerView *view = (EmailComposerView *)data;

	if (!view->is_checkbox_clicked) {
		if (!ewk_view_script_execute(view->ewk_view, EC_JS_GET_IMAGE_LIST_FROM_NEW_MESSAGE, _get_image_list_before_leaving_composer_cb, (void *)view)) {
			debug_error("EC_JS_GET_IMAGE_LIST_FROM_NEW_MESSAGE failed.");
		}
	} else {
		if (!ewk_view_script_execute(view->ewk_view, EC_JS_GET_IMAGE_LIST, _get_image_list_before_leaving_composer_cb, (void *)view)) {
			debug_error("EC_JS_GET_IMAGE_LIST failed.");
		}
	}

	debug_leave();
}

static void _get_image_list_before_leaving_composer_cb(Evas_Object *obj, const char *result, void *data)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	composer_util_update_inline_image_list(view, result);

	/* This ewk call invokes some successive ewk calls to get html contents. */
	if (!ewk_view_plain_text_get(view->ewk_view, _get_final_plain_text_cb, (void *)view)) {
		debug_error("ewk_view_plain_text_get() failed!");
	}

	debug_leave();
}

static void _get_final_plain_text_cb(Evas_Object *obj, const char *result, void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	FREE(view->plain_content);
	if (result) {
		view->plain_content = g_strdup(result);
	}

	if (view->with_original_message) {
		if (!ewk_view_script_execute(view->ewk_view, EC_JS_GET_CONTENTS_FROM_ORG_MESSAGE, _get_final_parent_content_cb, (void *)view)) {
			debug_error("EC_JS_GET_CONTENTS_FROM_ORG_MESSAGE failed!.");
		}
	} else {
		if (!ewk_view_script_execute(view->ewk_view, EC_JS_GET_CONTENTS_FROM_BODY, _get_final_body_content_cb, (void *)view)) {
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

	EmailComposerView *view = (EmailComposerView *)data;

	FREE(view->final_parent_content);
	if (result) {
		view->final_parent_content = g_strdup(result);
		/*debug_secure("FINAL PARENT CONTENT: (%s)", view->final_parent_content);*/
	}

	if (!ewk_view_script_execute(view->ewk_view, EC_JS_GET_CONTENTS_FROM_NEW_MESSAGE, _get_final_new_message_content_cb, (void *)view)) {
		debug_error("EC_JS_GET_CONTENTS_FROM_NEW_MESSAGE failed!.");
	}

	debug_leave();
}

/* DO NOT use this function alone. this function is successively called when you call _get_final_plain_text_cb(). */
static void _get_final_new_message_content_cb(Evas_Object *obj, const char *result, void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	FREE(view->final_new_message_content);
	if (result) {
		view->final_new_message_content = g_strdup(result);
	}

	if (!ewk_view_script_execute(view->ewk_view, EC_JS_GET_CONTENTS_FROM_BODY, _get_final_body_content_cb, (void *)view)) {
		debug_error("EC_JS_GET_CONTENTS_FROM_BODY failed!.");
	}

	debug_leave();
}

static void _get_final_body_content_cb(Evas_Object *obj, const char *result, void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	FREE(view->final_body_content);
	if (result) {
		view->final_body_content = g_strdup(result);
	}

	_exit_composer_cb(NULL, NULL, view);

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

	EmailComposerView *view = (EmailComposerView *)data;
	view->thread_saving_email = NULL;

	debug_leave();
}

static void _exit_composer_cb(Evas_Object *obj, const char *result, void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	if (view->is_send_btn_clicked) {
		_exit_composer_with_sending_mail(view);
	} else {
		_exit_composer_without_sending_mail(view);
	}

	debug_leave();
}

static void _exit_composer_with_sending_mail(void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	_check_forwarding_attachment_download(view);

	debug_leave();
}

static void _exit_composer_process_sending_mail(void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	COMPOSER_ERROR_TYPE_E ret = _send_mail(view);
	if (ret == COMPOSER_ERROR_NONE) {
		/* If we don't use idler to destroy composer which is launched by appcontrol, a black screen appears when module is destroyed.
		 * It's only happened on this exit. (No one knows the exact reason why a black screen appears.)
		 */
		composer_util_network_state_noti_post();
		DELETE_IDLER_OBJECT(view->idler_destroy_self);
		view->idler_destroy_self = ecore_idler_add(_send_mail_finish_idler_cb, view);
	} else if (ret == COMPOSER_ERROR_MAIL_SIZE_EXCEEDED) {
		composer_attachment_launch_attachment_error_popup(COMPOSER_ERROR_ATTACHMENT_MAX_SIZE_EXCEEDED, _send_mail_failed_popup_cb, view);
	} else if (ret == COMPOSER_ERROR_MAIL_STORAGE_FULL) {
		view->composer_popup = composer_util_popup_create(view, EMAIL_COMPOSER_STRING_MF_HEADER_UNABLE_TO_SAVE_DATA_ABB, EMAIL_COMPOSER_STRING_STORAGE_FULL_CONTENTS, _composer_send_mail_storage_full_popup_response_cb,
		                                                EMAIL_COMPOSER_STRING_BUTTON_CANCEL, EMAIL_COMPOSER_STRING_BUTTON_SETTINGS, EMAIL_COMPOSER_STRING_NULL);
	} else {
		view->composer_popup = composer_util_popup_create(view, EMAIL_COMPOSER_STRING_HEADER_UNABLE_TO_SEND_EMAIL_ABB, EMAIL_COMPOSER_STRING_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED,
								_send_mail_failed_popup_cb, EMAIL_COMPOSER_STRING_BUTTON_OK, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);
	}

	debug_leave();
}

static void _exit_composer_without_sending_mail(void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	if (view->is_save_in_drafts_clicked) {
		_save_in_drafts_in_thread(view);
	} else if (view->is_back_btn_clicked) {
		if (!composer_util_is_mail_modified(view)) {
			DELETE_EVAS_OBJECT(view->composer_popup); /* To delete mdm noti popup */

			/* Restore the indicator mode. */
			composer_util_indicator_restore(view);

			email_module_make_destroy_request(view->base.module);
		} else {
			view->composer_popup = composer_util_popup_create(view, EMAIL_COMPOSER_STRING_HEADER_STOP_COMPOSING_EMAIL_HEADER_ABB, EMAIL_COMPOSER_STRING_POP_SAVE_YOUR_CHANGES_OR_DISCARD_THEM_Q, _save_popup_cb,
					EMAIL_COMPOSER_STRING_BUTTON_CANCEL, EMAIL_COMPOSER_STRING_BUTTON_DISCARD_ABB2, EMAIL_COMPOSER_STRING_BUTTON_SAVE_ABB);
			if (composer_util_is_max_sending_size_exceeded(view)) {
				Evas_Object *save_button = elm_object_part_content_get(view->composer_popup, "button3");
				elm_object_disabled_set(save_button, EINA_TRUE);
			}
		}
	}

	debug_leave();
}

static COMPOSER_ERROR_TYPE_E _send_mail(EmailComposerView *view)
{
	debug_enter();

	retvm_if(!view, COMPOSER_ERROR_SEND_FAIL, "view is NULL!");
	retvm_if(!view->account_info, COMPOSER_ERROR_SEND_FAIL, "account_info is NULL!");
	retvm_if(!view->account_info->selected_account, COMPOSER_ERROR_SEND_FAIL, "selected_account is NULL!");

	COMPOSER_ERROR_TYPE_E ret = COMPOSER_ERROR_NONE;
	int handle = 0;
	gboolean ok = FALSE;

	ret = _add_mail(view, EMAIL_MAILBOX_TYPE_OUTBOX);
	retvm_if(ret != COMPOSER_ERROR_NONE, ret, "_add_mail() failed: %d", ret);

	if (((view->composer_type == RUN_COMPOSER_FORWARD) || (view->composer_type == RUN_COMPOSER_EDIT)) && view->need_download && view->new_mail_info->mail_data->reference_mail_id) {
		debug_log(">> email_engine_send_mail_with_downloading_attachment_of_original_mail()");
		ok = email_engine_send_mail_with_downloading_attachment_of_original_mail(view->new_mail_info->mail_data->mail_id, &handle);
	} else {
		debug_log(">> email_engine_send_mail()");
		ok = email_engine_send_mail(view->new_mail_info->mail_data->mail_id, &handle);
	}

	if (ok) {
		debug_log("Sending email succeeded!");

		/* Update Reply/Forward state and time */
		email_mail_attribute_value_t sending_time;
		sending_time.datetime_type_value = time(NULL);

		if (view->composer_type == RUN_COMPOSER_REPLY || view->composer_type == RUN_COMPOSER_REPLY_ALL) {
			email_engine_update_mail_attribute(view->org_mail_info->mail_data->account_id,
					&view->org_mail_info->mail_data->mail_id, 1, EMAIL_MAIL_ATTRIBUTE_REPLIED_TIME, sending_time);
			email_engine_set_flags_field(view->account_info->original_account->account_id,
					&view->original_mail_id, 1, EMAIL_FLAGS_ANSWERED_FIELD, 1, 1);
		} else if (view->composer_type == RUN_COMPOSER_FORWARD || view->composer_type == RUN_COMPOSER_FORWARD_ALL) {
			email_engine_update_mail_attribute(view->org_mail_info->mail_data->account_id,
					&view->org_mail_info->mail_data->mail_id, 1, EMAIL_MAIL_ATTRIBUTE_FORWARDED_TIME, sending_time);
			email_engine_set_flags_field(view->account_info->original_account->account_id,
					&view->original_mail_id, 1, EMAIL_FLAGS_FORWARDED_FIELD, 1, 1);
		}
	} else {
		debug_error("Sending email failed");
		return COMPOSER_ERROR_SEND_FAIL;
	}

	debug_leave();
	return COMPOSER_ERROR_NONE;
}

static COMPOSER_ERROR_TYPE_E _add_mail(EmailComposerView *view, email_mailbox_type_e mailbox_type)
{
	debug_enter();

	EmailComposerAccount *account_info = view->account_info;
	retvm_if(!account_info, COMPOSER_ERROR_NULL_POINTER, "account_info is NULL!");

	int ret = EMAIL_ERROR_NONE;

	ret = _make_mail(view, mailbox_type);
	retv_if(ret != COMPOSER_ERROR_NONE, ret);

	ret = email_engine_add_mail(view->new_mail_info->mail_data, view->new_mail_info->attachment_list, view->new_mail_info->total_attachment_count, NULL, 0);
	if (ret != EMAIL_ERROR_NONE) {
		debug_error("email_engine_add_mail() failed! ret:[%d]", ret);
		return (ret == EMAIL_ERROR_MAIL_MEMORY_FULL) ? COMPOSER_ERROR_MAIL_STORAGE_FULL : COMPOSER_ERROR_FAIL;
	}

	if (view->composer_type == RUN_COMPOSER_EDIT) {
		int sync = EMAIL_DELETE_LOCAL_AND_SERVER;

		/* Retrieve mailbox type from email-service to identify Outbox */
		if (EMAIL_MAILBOX_TYPE_OUTBOX == view->org_mail_info->mail_data->mailbox_type)
			sync = EMAIL_DELETE_LOCALLY;

		debug_secure("Original account_id:[%d], mailbox_id:[%d], mail_id:[%d]", account_info->original_account->account_id, view->org_mail_info->mail_data->mailbox_id, view->original_mail_id);

		if (!email_engine_delete_mail(view->org_mail_info->mail_data->mailbox_id, view->original_mail_id, sync)) {
			debug_error("email_engine_delete_mail() failed!");
			return COMPOSER_ERROR_SEND_FAIL;
		}
	}

	debug_leave();
	return COMPOSER_ERROR_NONE;
}

static COMPOSER_ERROR_TYPE_E _make_mail(EmailComposerView *view, email_mailbox_type_e mailbox_type)
{
	debug_enter();

	int err = COMPOSER_ERROR_NONE;

	debug_log("account_id = %d", view->account_info->selected_account->account_id);
	view->new_mail_info->mail_data->account_id = view->account_info->selected_account->account_id;

	err = _make_mail_set_from_address(view);
	retv_if(err != COMPOSER_ERROR_NONE, err);

	err = _make_mail_set_mailbox_info(view, mailbox_type);
	retv_if(err != COMPOSER_ERROR_NONE, err);

	/* --> Set other properties */
	view->new_mail_info->mail_data->report_status = EMAIL_MAIL_REPORT_NONE;
	view->new_mail_info->mail_data->priority = view->priority_option;

	if (view->composer_type == RUN_COMPOSER_EDIT) {
		view->new_mail_info->mail_data->body_download_status = EMAIL_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED;
	}
	view->new_mail_info->mail_data->flags_draft_field = 1;
	view->new_mail_info->mail_data->remaining_resend_times = (view->account_info->selected_account->auto_resend_times == EMAIL_NO_LIMITED_RETRY_COUNT) ? 10 : view->account_info->selected_account->auto_resend_times;

	_make_mail_set_reference_id(view);
	/* <-- Set other properties */

	/* --> Make recipient list from MBE */
	FREE(view->new_mail_info->mail_data->full_address_to);
	FREE(view->new_mail_info->mail_data->full_address_cc);
	FREE(view->new_mail_info->mail_data->full_address_bcc);

	err = composer_util_recp_retrieve_recp_string(view, view->recp_to_mbe, &(view->new_mail_info->mail_data->full_address_to));
	retv_if(err != COMPOSER_ERROR_NONE, err);

	err = composer_util_recp_retrieve_recp_string(view, view->recp_cc_mbe, &(view->new_mail_info->mail_data->full_address_cc));
	retv_if(err != COMPOSER_ERROR_NONE, err);

	err = composer_util_recp_retrieve_recp_string(view, view->recp_bcc_mbe, &(view->new_mail_info->mail_data->full_address_bcc));
	retv_if(err != COMPOSER_ERROR_NONE, err);
	/* <-- Make recipient list from MBE */

	view->new_mail_info->mail_data->subject = elm_entry_markup_to_utf8(elm_entry_entry_get(view->subject_entry.entry));

	/* --> Make body html */
	err = _remove_old_body_file(view);
	retv_if(err != COMPOSER_ERROR_NONE, err);

	char *html_content = NULL;
	char *html_content_processed = NULL;
	char *plain_text_content = NULL;

	_make_mail_make_draft_html_content(view, &html_content, mailbox_type);
	retvm_if(!html_content, COMPOSER_ERROR_MAKE_MAIL_FAIL, "html_content is NULL!");

	/*debug_secure("html_content: (%s)", html_content);*/
	/* <-- Make body html */

	/* --> Save html body as file */
	if (view->attachment_inline_item_list == NULL) {
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

	if (!email_save_file(view->saved_html_path, html_content_processed, STR_LEN(html_content_processed))) {
		debug_secure("email_save_file() failed! for (%s)", view->saved_html_path);
		FREE(html_content_processed);
		return COMPOSER_ERROR_MAKE_MAIL_FAIL;
	}
	FREE(html_content_processed);
	/* <-- Save html body as file */

	/* --> Save plain text body as file */
	if (!view->plain_content || (view->plain_content && STR_LEN(view->plain_content) == 0)) {
		plain_text_content = STR_DUP("\n");
	} else {
		plain_text_content = STR_DUP(view->plain_content);
	}
	retvm_if(!plain_text_content, COMPOSER_ERROR_MAKE_MAIL_FAIL, "plain_text_content is NULL!");

	if (!email_save_file(view->saved_text_path, plain_text_content, strlen(plain_text_content))) {
		debug_secure("email_save_file() failed! for (%s)", view->saved_text_path);
		FREE(plain_text_content);
		return COMPOSER_ERROR_MAKE_MAIL_FAIL;
	}
	FREE(plain_text_content);
	/* <-- Save plain text body as file */

	view->new_mail_info->mail_data->file_path_html = STR_DUP(view->saved_html_path);
	view->new_mail_info->mail_data->file_path_plain = STR_DUP(view->saved_text_path);

	_make_mail_make_attachment_list(view);

	/* This logic checks if mail size exceeds when attachments and text entered both are taken into account.
	 * - This is defensive code for normal case because "send button" and "save in drafts menu" were already disabled if mail size exceeds.
	 * - When we press a new mail noti while composing a mail, viewer is getting opened and the mail is being composed is saved in drafts.
	 *   But for a mail whose size exceeds max, the mail is be discarded.
	 */
	retvm_if(composer_util_is_max_sending_size_exceeded(view), COMPOSER_ERROR_MAIL_SIZE_EXCEEDED, "Mail size exceeded!");

	debug_leave();
	return COMPOSER_ERROR_NONE;
}

static void _save_in_drafts_process_result(EmailComposerView *view, int ret)
{
	debug_enter();

	if ((ret == COMPOSER_ERROR_NONE) || view->is_force_destroy) {
		DELETE_EVAS_OBJECT(view->composer_popup);

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
		composer_util_indicator_restore(view);

		email_module_make_destroy_request(view->base.module);
	} else if (ret == COMPOSER_ERROR_MAIL_SIZE_EXCEEDED) {
		composer_attachment_launch_attachment_error_popup(COMPOSER_ERROR_ATTACHMENT_MAX_SIZE_EXCEEDED, _unable_to_save_in_drafts_cb, view);
	} else if (ret == COMPOSER_ERROR_MAIL_STORAGE_FULL) {
		view->composer_popup = composer_util_popup_create(view, EMAIL_COMPOSER_STRING_MF_HEADER_UNABLE_TO_SAVE_DATA_ABB, EMAIL_COMPOSER_STRING_STORAGE_FULL_CONTENTS, _composer_send_mail_storage_full_popup_response_cb,
		                                                EMAIL_COMPOSER_STRING_BUTTON_CANCEL, EMAIL_COMPOSER_STRING_BUTTON_SETTINGS, EMAIL_COMPOSER_STRING_NULL);
	} else {
		debug_error("_add_mail() failed: %d", ret);
		view->composer_popup = composer_util_popup_create(view, EMAIL_COMPOSER_STRING_HEADER_UNABLE_TO_PERFORM_ACTION_ABB, EMAIL_COMPOSER_STRING_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED,
		                                                _unable_to_save_in_drafts_cb, EMAIL_COMPOSER_STRING_BUTTON_OK, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);
	}

	debug_leave();
}

static void _save_in_drafts(EmailComposerView *view)
{
	debug_enter();

	retm_if(!view, "Invalid parameter: data is NULL!");

	int ret = _add_mail(view, EMAIL_MAILBOX_TYPE_DRAFT);
	_save_in_drafts_process_result(view, ret);

	debug_leave();
}

static void _save_in_drafts_in_thread(EmailComposerView *view)
{
	debug_enter();

	/* It takes too long time to save files to disk on kiran device that has small storage and poor disk controller. (Around 6MB/s)
	 * When we save large size of attachment, email_engine_add_mail() takes too long time because of the above problem. So we handle this API in thread.
	 *
	 * We should show progress popup while saving a mail because saving email in thread takes too long time if attachment size exceeds threshold.
	 * In case of force destroy, we do not show progress popup while saving a mail because composer is not shown on the top of the screen.
	 */
	if (!view->is_force_destroy) {
		int attachment_size = composer_util_get_total_attachments_size(view, EINA_TRUE);
		if (attachment_size >= ATTACHMENT_SIZE_THRESHOLD) {
			view->composer_popup = composer_util_popup_create_with_progress_horizontal(view, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_POP_SAVING_EMAIL_ING, NULL,
			                                                                          EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);

			view->thread_saving_email = ecore_thread_feedback_run(_save_in_drafts_thread_worker, _save_in_drafts_thread_notify,
			                                                     _save_in_drafts_thread_finish, NULL, view, EINA_TRUE);
		} else {
			view->thread_saving_email = NULL;
		}
	}

	if (!view->thread_saving_email) {
		_save_in_drafts(view);
	}

	debug_leave();
}

static Eina_Bool _send_mail_finish_idler_cb(void *data)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	view->idler_destroy_self = NULL;

	/* Display "Downloading..." toast popup when downloading attachments will be displayed after destroying composer. */
	if (view->is_forward_download_processed) {
		int noti_ret = notification_status_message_post(email_get_email_string("IDS_EMAIL_POP_DOWNLOADING_ING"));
		debug_warning_if(noti_ret != NOTIFICATION_ERROR_NONE, "notification_status_message_post() failed! ret:(%d)", noti_ret);
	}

	/* Restore the indicator mode. */
	composer_util_indicator_restore(view);

	email_module_make_destroy_request(view->base.module);

	debug_leave();
	return ECORE_CALLBACK_CANCEL;
}

static void _composer_send_mail_storage_full_popup_response_cb(void *data, Evas_Object *obj, void *event_info)
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

		view->is_send_btn_clicked = EINA_FALSE;
		view->is_back_btn_clicked = EINA_FALSE;
		view->is_save_in_drafts_clicked = EINA_FALSE;
	} else {
		DELETE_EVAS_OBJECT(view->composer_popup);
		composer_util_return_composer_view(view);
	}

	debug_leave();
}

static void _save_popup_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;
	char *button_text = NULL;

	/* To distinguish soft button and hareware back button */
	if (obj != view->composer_popup) {
		button_text = (char *)elm_object_text_get(obj);
	}
	DELETE_EVAS_OBJECT(view->composer_popup);

	if (!button_text || !g_strcmp0(button_text, email_get_email_string(EMAIL_COMPOSER_STRING_BUTTON_CANCEL.id))) {
		composer_util_return_composer_view(view);
	} else if (!g_strcmp0(button_text, email_get_email_string(EMAIL_COMPOSER_STRING_BUTTON_DISCARD_ABB2.id))) {
		/* Restore the indicator mode. */
		composer_util_indicator_restore(view);

		email_module_make_destroy_request(view->base.module);
	} else if (!g_strcmp0(button_text, email_get_email_string(EMAIL_COMPOSER_STRING_BUTTON_SAVE_ABB.id))) {
		_save_in_drafts_in_thread(view);
	}

	debug_leave();
}

static void _unable_to_save_in_drafts_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	DELETE_EVAS_OBJECT(view->composer_popup);
	composer_util_return_composer_view(view);

	debug_leave();
}

static void _download_attachment_popup_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;
	char *button_text = NULL;

	/* To distinguish soft button and hareware back button */
	if (obj != view->composer_popup) {
		button_text = (char *)elm_object_text_get(obj);
	}

	if (!g_strcmp0(button_text, email_get_email_string(EMAIL_COMPOSER_STRING_BUTTON_CONTINUE_ABB.id))) {
		view->is_forward_download_processed = EINA_TRUE;
		_exit_composer_process_sending_mail(view);
		DELETE_EVAS_OBJECT(view->composer_popup);
	} else {
		DELETE_EVAS_OBJECT(view->composer_popup);
		composer_util_return_composer_view(view);
	}

	debug_leave();
}

static COMPOSER_ERROR_TYPE_E _remove_old_body_file(EmailComposerView *view)
{
	debug_enter();

	if (email_check_file_exist(view->saved_html_path)) {
		int ret = remove(view->saved_html_path);
		retvm_if(ret == -1, COMPOSER_ERROR_FAIL, "remove() failed!");
	}

	if (email_check_file_exist(view->saved_text_path)) {
		int ret = remove(view->saved_text_path);
		retvm_if(ret == -1, COMPOSER_ERROR_FAIL, "remove() failed!");
	}

	debug_leave();
	return COMPOSER_ERROR_NONE;
}

static COMPOSER_ERROR_TYPE_E _make_mail_set_from_address(EmailComposerView *view)
{
	debug_enter();

	EmailComposerAccount *account_info = view->account_info;
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

	view->new_mail_info->mail_data->full_address_from = g_strdup_printf("\"%s\" <%s>", processed_display_name ? processed_display_name : account_info->selected_account->user_display_name, view->account_info->selected_account->user_email_address);
	debug_secure("from_address: (%s)", view->new_mail_info->mail_data->full_address_from);

	FREE(processed_display_name);

	debug_leave();
	return COMPOSER_ERROR_NONE;
}

static COMPOSER_ERROR_TYPE_E _make_mail_set_mailbox_info(EmailComposerView *view, email_mailbox_type_e mailbox_type)
{
	debug_enter();

	email_mailbox_t *mailbox = NULL;
	EmailComposerAccount *account_info = view->account_info;
	retvm_if(!account_info, COMPOSER_ERROR_NULL_POINTER, "account_info is NULL!");

	if (!email_engine_get_mailbox_by_mailbox_type(account_info->selected_account->account_id, mailbox_type, &mailbox)) {
		debug_log("email_engine_get_mailbox_by_mailbox_type() failed!");
		return COMPOSER_ERROR_FAIL;
	}
	debug_log("mailbox_id = %d", mailbox->mailbox_id);

	view->new_mail_info->mail_data->mailbox_id = mailbox->mailbox_id;
	view->new_mail_info->mail_data->mailbox_type = mailbox->mailbox_type;

	email_engine_free_mailbox_list(&mailbox, 1);

	debug_leave();
	return COMPOSER_ERROR_NONE;
}

static void _make_mail_set_reference_id(EmailComposerView *view)
{
	debug_enter();

	view->new_mail_info->mail_data->reference_mail_id = 0; /* Set reference_mail_id as 0 by default. */

	if (view->account_info && (view->account_info->original_account->account_id == view->account_info->selected_account->account_id)) {
		if (view->composer_type == RUN_COMPOSER_FORWARD) {
			view->new_mail_info->mail_data->reference_mail_id = view->org_mail_info->mail_data->mail_id;
		} else if (view->composer_type == RUN_COMPOSER_EDIT) {
			view->new_mail_info->mail_data->reference_mail_id = view->org_mail_info->mail_data->reference_mail_id;
		}
	}

	debug_log("new mail - reference_mail_id:[%d]", view->new_mail_info->mail_data->reference_mail_id);

	debug_leave();
}

static void _make_mail_make_draft_html_content(EmailComposerView *view, char **html_content, email_mailbox_type_e mailbox_type)
{
	debug_enter();

	retm_if(!view, "Invalid parameter: view is NULL!");
	retm_if(!html_content, "Invalid parameter: html_content is NULL!");

	if (view->is_send_btn_clicked) {
		if (view->with_original_message) {
			if (view->is_checkbox_clicked) {
				*html_content = g_strconcat(EC_TAG_HTML_START, EC_TAG_BODY_START, view->final_new_message_content, EC_TAG_DIV_DIR""EC_TAG_DIV_DIR, view->final_parent_content, EC_TAG_BODY_END, EC_TAG_HTML_END, NULL);
			} else {
				*html_content = g_strconcat(EC_TAG_HTML_START, EC_TAG_BODY_START, view->final_new_message_content, EC_TAG_BODY_END, EC_TAG_HTML_END, NULL);
			}
		} else {
			*html_content = g_strconcat(EC_TAG_HTML_START, EC_TAG_BODY_START, view->final_body_content, EC_TAG_BODY_END, EC_TAG_HTML_END, NULL);
		}
	} else {
		if (view->with_original_message) {
			*html_content = g_strconcat(EC_TAG_HTML_START, EC_TAG_BODY_ORIGINAL_MESSAGE_START, view->final_body_content, EC_TAG_BODY_END, EC_TAG_HTML_END, NULL);
		} else {
			*html_content = g_strconcat(EC_TAG_HTML_START, EC_TAG_BODY_CSS_START, view->final_body_content, EC_TAG_BODY_END, EC_TAG_HTML_END, NULL);
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

static void _make_mail_make_attachment_list(EmailComposerView *view)
{
	debug_enter();

	int attach_count = eina_list_count(view->attachment_item_list);
	int inline_count = eina_list_count(view->attachment_inline_item_list);

	debug_log("1. attach_count: [%d]", attach_count);
	debug_log("1. inline_count: [%d]", inline_count);

	Eina_List *list = NULL;
	email_attachment_data_t *inline_att = NULL;
	ComposerAttachmentItemData *attachment_item_data = NULL;

	char err_buff[EMAIL_BUFF_SIZE_HUG] = { 0, };

	/* If attachment hasn't been downloaded yet, it shouldn't be included in sending mail.
	 * But to support email_engine_send_mail_with_downloading_attachment_of_original_mail() API, forward case is excluded.
	 */
	EINA_LIST_FOREACH(view->attachment_item_list, list, attachment_item_data) {
		email_attachment_data_t *att = attachment_item_data->attachment_data;
		struct stat sb;
		if (!att->attachment_path || (stat(att->attachment_path, &sb) == -1)) {
			if (att->attachment_path) {
				debug_log("stat() failed! (%d): %s", errno, strerror_r(errno, err_buff, sizeof(err_buff)));
			}

			bool is_changed = true;
			if (view->new_mail_info->mail_data->reference_mail_id != 0) {
				unsigned int i;
				for (i = 0; i < view->org_mail_info->total_attachment_count; i++) {
					email_attachment_data_t *org_att = view->org_mail_info->attachment_list + i;
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

	EINA_LIST_FOREACH(view->attachment_inline_item_list, list, inline_att) {
		struct stat sb;
		if (!inline_att->attachment_path || (stat(inline_att->attachment_path, &sb) == -1)) {
			if (inline_att->attachment_path) {
				debug_log("stat() failed! (%d): %s", errno, strerror_r(errno, err_buff, sizeof(err_buff)));
			}

			if (view->new_mail_info->mail_data->reference_mail_id == 0) {
				inline_att->attachment_id = -1;
				inline_count--;
			}
		}
	}

	debug_log("3. attach_count: [%d]", attach_count);
	debug_log("3. inline_count: [%d]", inline_count);

	/* For logging */
	EINA_LIST_FOREACH(view->attachment_inline_item_list, list, inline_att) {
		debug_secure("[%d] name:%s, path:%s, is_inline:[%d], save:[%d]",
			inline_att->attachment_id, inline_att->attachment_name, inline_att->attachment_path, inline_att->inline_content_status, inline_att->save_status);
	}
#ifdef _DEBUG
	EINA_LIST_FOREACH(view->attachment_item_list, list, attachment_item_data) {
		email_attachment_data_t *att = attachment_item_data->attachment_data;
		debug_secure("[%d] name:%s, path:%s, is_inline:[%d], save:[%d]", att->attachment_id, att->attachment_name, att->attachment_path, att->inline_content_status, att->save_status);
	}
#endif

	if (attach_count + inline_count > 0) {
		view->new_mail_info->total_attachment_count = attach_count + inline_count;
		view->new_mail_info->mail_data->attachment_count = attach_count;
		view->new_mail_info->mail_data->inline_content_count = inline_count;
		view->new_mail_info->attachment_list = (email_attachment_data_t *)calloc(view->new_mail_info->total_attachment_count, sizeof(email_attachment_data_t));
		retm_if(!view->new_mail_info->attachment_list, "Failed to allocate memory for attachment_list!");

		email_attachment_data_t *new_att = view->new_mail_info->attachment_list;

		EINA_LIST_FOREACH(view->attachment_item_list, list, attachment_item_data) {
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

		EINA_LIST_FOREACH(view->attachment_inline_item_list, list, inline_att) {
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

#ifdef _DEBUG
	/* For logging */
	int i;
	debug_log("----------------------------------------------------------------------------------------------");
	for (i = 0; i < view->new_mail_info->total_attachment_count; i++) {
		email_attachment_data_t *att = view->new_mail_info->attachment_list + i;
		debug_secure("[%d] name:%s, path:%s, is_inline:[%d], save:[%d]", att->attachment_id, att->attachment_name, att->attachment_path, att->inline_content_status, att->save_status);
	}
#endif

	debug_leave();
}


/*
 * Definition for exported functions
 */

void composer_exit_composer_get_contents(void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	if (view->is_send_btn_clicked) {
		MBE_VALIDATION_ERROR err = MBE_VALIDATION_ERROR_NONE;
		MBE_VALIDATION_ERROR prev_err = MBE_VALIDATION_ERROR_NONE;
		Evas_Object *error_entry = NULL;

		prev_err = err = _check_vaildify_of_recipient_field(view, view->recp_to_mbe);
		if (err == MBE_VALIDATION_ERROR_INVALID_ADDRESS) {
			error_entry = view->recp_to_entry.entry;
		}

		err = _check_vaildify_of_recipient_field(view, view->recp_cc_mbe);
		if (err == MBE_VALIDATION_ERROR_INVALID_ADDRESS) {
			if (!error_entry) {
				error_entry = view->recp_cc_entry.entry;
			}
		}

		if (prev_err < err) {
			prev_err = err;
		}

		err = _check_vaildify_of_recipient_field(view, view->recp_bcc_mbe);
		if (err == MBE_VALIDATION_ERROR_INVALID_ADDRESS) {
			if (!error_entry) {
				error_entry = view->recp_bcc_entry.entry;
			}
		}

		if (prev_err < err) {
			prev_err = err;
		}

		if (prev_err > MBE_VALIDATION_ERROR_DUPLICATE_ADDRESS) {
			view->selected_widget = error_entry;

			view->is_send_btn_clicked = EINA_FALSE;
			composer_recipient_display_error_string(view, prev_err);

			debug_leave();
			return;
		}

		if (composer_util_recp_is_mbe_empty(view)) {
			view->selected_widget = view->recp_to_entry.entry;

			view->is_send_btn_clicked = EINA_FALSE;
			composer_recipient_display_error_string(view, MBE_VALIDATION_ERROR_NO_ADDRESS);

			debug_leave();
			return;
		}

		/* To hide IME when send button is clicked. (send button isn't a focusable object) */
		ecore_imf_input_panel_hide();
	}

#ifdef FEATRUE_PREDICTIVE_SEARCH
	if (view->ps_is_runnig) {
		composer_ps_stop_search(view);
	}
#endif

	/* To prevent showing ime while sending email. */
	elm_object_tree_focus_allow_set(view->composer_layout, EINA_FALSE);

	if (!ewk_view_script_execute(view->ewk_view, EC_JS_REMOVE_IMAGE_CONTROL_LAYER, _delete_image_control_layer_cb, (void *)view)) {
		debug_error("EC_JS_REMOVE_IMAGE_CONTROL_LAYER failed.");
	}

	debug_leave();
}

static void _check_forwarding_attachment_download(void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	view->is_forward_download_processed = EINA_FALSE;

	if ((view->account_info->original_account->account_id == view->account_info->selected_account->account_id) &&
		((view->composer_type == RUN_COMPOSER_FORWARD) || (view->composer_type == RUN_COMPOSER_EDIT)) && view->need_download) {
		view->composer_popup = composer_util_popup_create(view, EMAIL_COMPOSER_STRING_HEADER_DOWNLOAD_ATTACHMENTS_ABB, EMAIL_COMPOSER_STRING_POP_ALL_ATTACHMENTS_IN_THIS_EMAIL_WILL_BE_DOWNLOADED_BEFORE_IT_IS_FORWARDED,
		                                                _download_attachment_popup_cb, EMAIL_COMPOSER_STRING_BUTTON_CANCEL, EMAIL_COMPOSER_STRING_BUTTON_CONTINUE_ABB, EMAIL_COMPOSER_STRING_NULL);
	} else {
		_exit_composer_process_sending_mail(view);
	}

	debug_leave();
}

static void _send_mail_failed_popup_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	DELETE_EVAS_OBJECT(view->composer_popup);
	composer_util_return_composer_view(view);

	debug_leave();
}

/* EOF */
