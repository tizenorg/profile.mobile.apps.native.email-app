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

#include "email-html-converter.h"
#include "email-composer.h"
#include "email-composer-util.h"
#include "email-composer-attachment.h"
#include "email-composer-recipient.h"
#include "email-composer-initial-view.h"
#include "email-composer-initial-data.h"
#include "email-composer-js.h"
#include "email-composer-types.h"

/*
 * Declaration for static functions
 */

static void _initial_data_download_contents(EmailComposerView *view);
static void _initial_data_destroy_download_contents_popup_cb(void *data, Evas_Object *obj, void *event_info);

static Eina_Bool _initial_data_mbe_append_recipients(EmailComposerView *view, Evas_Object *mbe, const char *to_be_added_recipients);

static char *_initial_data_body_make_recipient_string(const char *original_list, const char *delim);
static char *_initial_data_body_get_original_message_bar_tags(EmailComposerView *view);
static char *_initial_data_body_get_parent_content(EmailComposerView *view);

static void _initial_data_parse_params_by_internal(email_params_h params, const char **argv, int run_type);
static void _initial_data_parse_params_from_parent_module(email_params_h params, const char **argv);
static void _initial_data_parse_params_by_appcontrol(EmailComposerView *view,
		email_params_h params, const char **argv, const char *operation);

static char *_initial_data_parse_mailto_recipients(char *recipients_list);
static void _initial_data_parse_mailto_uri(EmailComposerView *view, const char *mailto_uri);
static void _initial_data_parse_contacts_sharing(EmailComposerView *view, email_params_h params, const char *operation);
static void _initial_data_parse_attachments(EmailComposerView *view, Eina_List *attachment_uri_list);

static COMPOSER_ERROR_TYPE_E _initial_data_process_params_by_internal(EmailComposerView *view, const char **argv);
static COMPOSER_ERROR_TYPE_E _initial_data_process_params_by_external(EmailComposerView *view, const char **argv);

static void _initial_data_set_mail_to_recipients(EmailComposerView *view);
static void _initial_data_set_mail_cc_recipients(EmailComposerView *view);
static void _initial_data_set_mail_bcc_recipients(EmailComposerView *view);
static void _initial_data_set_mail_subject(EmailComposerView *view);
static char *_initial_data_get_mail_body(EmailComposerView *view);
static void _initial_data_set_mail_attachment(EmailComposerView *view);
static void _initial_data_set_selected_entry(EmailComposerView *view);

static Eina_List *_initial_data_make_initial_recipients_list(Evas_Object *mbe);

static void _recp_preppend_string(gchar **dst, gchar *src);
static void _recp_append_string(gchar **dst, gchar *src);

static void _recp_append_extra_data_array(gchar **dst, email_params_h params, const char *data_key);

static email_string_t EMAIL_COMPOSER_STRING_NULL = { NULL, NULL };
static email_string_t EMAIL_COMPOSER_STRING_BUTTON_CANCEL = { PACKAGE, "IDS_EMAIL_BUTTON_CANCEL" };
static email_string_t EMAIL_COMPOSER_STRING_HEADER_OPEN_EMAIL = { PACKAGE, "IDS_EMAIL_HEADER_OPEN_EMAIL" };
static email_string_t EMAIL_COMPOSER_STRING_TPOP_LOADING_ING = { PACKAGE, "IDS_EMAIL_TPOP_LOADING_ING" };


/*
 * Definition for static functions
 */
static void _initial_data_download_contents(EmailComposerView *view)
{
	debug_enter();

	retm_if(!view, "view is NULL!");
	retm_if(!view->org_mail_info->mail_data, "view->org_mail_info->mail_data is NULL!");

	Eina_Bool ret = (Eina_Bool)email_engine_body_download(view->org_mail_info->mail_data->account_id, view->org_mail_info->mail_data->mail_id, EINA_TRUE, &view->draft_sync_handle);
	if (ret == EINA_TRUE) {
		debug_log("email_engine_body_download(handle:%d) is in progress...", view->draft_sync_handle);
		view->composer_popup = composer_util_popup_create_with_progress_horizontal(view, EMAIL_COMPOSER_STRING_HEADER_OPEN_EMAIL, EMAIL_COMPOSER_STRING_TPOP_LOADING_ING, _initial_data_destroy_download_contents_popup_cb,
		                                                                          EMAIL_COMPOSER_STRING_BUTTON_CANCEL, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);
	}

	debug_leave();
}

static void _initial_data_destroy_download_contents_popup_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	email_engine_stop_working(view->org_mail_info->mail_data->account_id, view->draft_sync_handle);
	composer_initial_data_destroy_download_contents_popup(view);
	composer_initial_data_set_mail_info(view, false);

	debug_leave();
}

static Eina_Bool _initial_data_mbe_append_recipients(EmailComposerView *view, Evas_Object *mbe, const char *to_be_added_recipients)
{
	debug_enter();

	Eina_Bool ret = EINA_FALSE;

	if (to_be_added_recipients) {
		debug_secure("list to be appended: (%s)", to_be_added_recipients);

		gchar **recipients_list = g_strsplit(to_be_added_recipients, ";", -1);
		int count;
		for (count = 0; count < g_strv_length(recipients_list); count++) {
			if (!recipients_list[count] || strlen(recipients_list[count]) == 0)
				continue;

			/* In the case of reply all, don't include my address in recipient fields. */
			if ((view->composer_type == RUN_COMPOSER_REPLY_ALL) && (g_strstr_len(recipients_list[count], strlen(recipients_list[count]), view->account_info->selected_account->user_email_address)))
				continue;

			EmailRecpInfo *ri = composer_util_recp_make_recipient_info(recipients_list[count]);
			if (ri) {
				char *markup_name = elm_entry_utf8_to_markup(ri->display_name);
				elm_multibuttonentry_item_append(mbe, markup_name, NULL, ri);
				FREE(markup_name);
				ret = EINA_TRUE;
			} else {
				debug_error("ri is NULL!");
			}
		}
		g_strfreev(recipients_list);
	}

	debug_leave();
	return ret;
}

static char *_initial_data_body_make_recipient_string(const char *original_list, const char *delim)
{
	debug_enter();

	retvm_if(!original_list, NULL, "Invalid parameter: original_list is NULL!");
	retvm_if(!delim, NULL, "Invalid parameter: delim is NULL!");

	char **recipients_list = g_strsplit(original_list, delim, -1);
	int len = g_strv_length(recipients_list);

	int count;
	for (count = 0; count < len; count++) {
		if (recipients_list[count] && (strlen(recipients_list[count]) == 0))
			continue;

		char *temp = composer_util_strip_quotation_marks_in_email_address(recipients_list[count]);
		if (temp) {
			free(recipients_list[count]);
			recipients_list[count] = temp;
		}
	}

	char *recp_string = g_strjoinv("; ", recipients_list);
	g_strfreev(recipients_list);

	debug_leave();
	return recp_string;
}

static char *_initial_data_body_get_original_message_bar_tags(EmailComposerView *view)
{
	debug_enter();

	char *original_message_bar = g_strdup(EC_TAG_ORIGINAL_MESSAGE_BAR);

	debug_leave();
	return original_message_bar;
}

static char *_initial_data_body_get_parent_content(EmailComposerView *view)
{
	debug_enter();

	char *tmp_text = NULL;

	if (view->org_mail_info->mail_data->file_path_html) {
		debug_log("[HTML body]");
		tmp_text = (char *)email_get_buff_from_file(view->org_mail_info->mail_data->file_path_html, 0);
	} else if (view->org_mail_info->mail_data->file_path_plain) {
		debug_log("[TEXT body]");
		char *temp_body_plain = (char *)email_get_buff_from_file(view->org_mail_info->mail_data->file_path_plain, 0);

		/* Convert plain-text to html content */
		tmp_text = (char *)email_get_parse_plain_text(temp_body_plain, STR_LEN(temp_body_plain));
		free(temp_body_plain);
	} else {
		tmp_text = g_strdup("\n");
	}

	debug_leave();
	return tmp_text;
}

static void _initial_data_parse_params_by_internal(email_params_h params, const char **argv, int run_type)
{
	debug_enter();

	retm_if(!params, "params is NULL!");
	retm_if(!argv, "argv is NULL!");

	debug_warning_if(!email_params_get_str(params, EMAIL_BUNDLE_KEY_ACCOUNT_ID, &argv[0]),
			"email_params_get_str(%s) failed!", EMAIL_BUNDLE_KEY_ACCOUNT_ID);

	debug_secure("account_id: (%s)", argv[0]);

	debug_warning_if(!email_params_get_str(params, EMAIL_BUNDLE_KEY_MAIL_ID, &argv[1]),
			"email_params_get_str(%s) failed!", EMAIL_BUNDLE_KEY_MAIL_ID);

	debug_log("mail_id: (%s)", argv[1]);

	if ((run_type == RUN_EML_FORWARD) || (run_type == RUN_EML_REPLY) || (run_type == RUN_EML_REPLY_ALL)) {
		debug_warning_if(!email_params_get_str(params, EMAIL_BUNDLE_KEY_MYFILE_PATH, &argv[3]),
				"email_params_get_str(%s) failed!", EMAIL_BUNDLE_KEY_MYFILE_PATH);

		debug_secure("eml_file_path: (%s)", argv[3]);
	}

	debug_leave();
}

static void _initial_data_parse_params_from_parent_module(email_params_h params, const char **argv)
{
	debug_enter();

	retm_if(!params, "Invalid parameter: params is NULL!");
	retm_if(!argv, "Invalid parameter: argv is NULL!");

	debug_warning_if(!email_params_get_str(params, EMAIL_BUNDLE_KEY_TO, &argv[0]),
			"email_params_get_str(%s) failed!", EMAIL_BUNDLE_KEY_TO);

	debug_warning_if(!email_params_get_str(params, EMAIL_BUNDLE_KEY_CC, &argv[1]),
			"email_params_get_str(%s) failed!", EMAIL_BUNDLE_KEY_CC);

	debug_warning_if(!email_params_get_str(params, EMAIL_BUNDLE_KEY_BCC, &argv[2]),
			"email_params_get_str(%s) failed!", EMAIL_BUNDLE_KEY_BCC);

	debug_warning_if(!email_params_get_str(params, EMAIL_BUNDLE_KEY_SUBJECT, &argv[3]),
			"email_params_get_str(%s) failed!", EMAIL_BUNDLE_KEY_SUBJECT);

	debug_warning_if(!email_params_get_str(params, EMAIL_BUNDLE_KEY_BODY, &argv[4]),
			"email_params_get_str(%s) failed!", EMAIL_BUNDLE_KEY_BODY);

	debug_warning_if(!email_params_get_str(params, EMAIL_BUNDLE_KEY_ATTACHMENT, &argv[5]),
			"email_params_get_str(%s) failed!", EMAIL_BUNDLE_KEY_ATTACHMENT);

	debug_secure("to: (%s)", argv[0]);
	debug_secure("cc: (%s)", argv[1]);
	debug_secure("bcc: (%s)", argv[2]);
	debug_secure("subject: (%s)", argv[3]);
	debug_secure("body: (%s)", argv[4]);
	debug_secure("attachment: (%s)", argv[5]);

	debug_leave();
}

static void _initial_data_parse_params_by_appcontrol(EmailComposerView *view,
		email_params_h params, const char **argv, const char *operation)
{
	debug_enter();

	retm_if(!params, "Invalid parameter: params is NULL!");
	retm_if(!argv, "Invalid parameter: argv is NULL!");
	retm_if(!operation, "Invalid parameter: operation is NULL!");

	if (!g_strcmp0(operation, APP_CONTROL_OPERATION_COMPOSE)) {

		_recp_append_extra_data_array(&view->new_mail_info->mail_data->full_address_to, params, APP_CONTROL_DATA_TO);

		_recp_append_extra_data_array(&view->new_mail_info->mail_data->full_address_cc, params, APP_CONTROL_DATA_CC);

		_recp_append_extra_data_array(&view->new_mail_info->mail_data->full_address_bcc, params, APP_CONTROL_DATA_BCC);
	}

	if (!g_strcmp0(operation, APP_CONTROL_OPERATION_COMPOSE) ||
		!g_strcmp0(operation, APP_CONTROL_OPERATION_SHARE_TEXT)) {

		debug_warning_if(!email_params_get_str(params, APP_CONTROL_DATA_SUBJECT, &argv[3]),
				"email_params_get_str(%s) failed!", APP_CONTROL_DATA_SUBJECT);

		debug_warning_if(!email_params_get_str(params, APP_CONTROL_DATA_TEXT, &argv[4]),
				"email_params_get_str(%s) failed!", APP_CONTROL_DATA_TEXT);

		debug_secure("subject: (%s)", argv[3]);
		debug_secure("body: (%s)", argv[4]);
	}

	debug_leave();
}

static char *_initial_data_parse_mailto_recipients(char *recipients_list)
{
	debug_enter();

	char *new_recipients_list = NULL;
	gchar **vector = g_strsplit_set(recipients_list, ",", -1);
	retvm_if(!vector, NULL, "g_strsplit_set() failed!");

	int i;
	for (i = 0; i < g_strv_length(vector); i++) {
		if (vector[i] && strlen(vector[i]) > 0) {
			char *tmp = g_uri_unescape_string(vector[i], NULL);
			if (tmp) {
				g_free(vector[i]);
				vector[i] = tmp;
			}
		}
	}

	new_recipients_list = g_strjoinv(";", vector);

	g_strfreev(vector);

	debug_leave();
	return new_recipients_list;
}

static void _initial_data_parse_mailto_uri(EmailComposerView *view, const char *mailto_uri)
{
	debug_enter();

	/*
	 * Please refer RFC document(RFC 2368:The mailto URL scheme) to understand this algorithm.
	 * Brief example:
	 *  mailto:aa@b.com?cc=cc@d.com&bcc=ee@f.com&subject=hello&body=nice&attachment=/opt/a.jpg
	 *  mailto:?to=aa@b.com&cc=cc@d.com&bcc=ee@f.com&subject=hello&body=nice&attachment=/opt/a.jpg
	 *  mailto:aa@b.com?to=bb@c.com&cc=cc@d.com&bcc=ee@f.com&subject=hello&body=nice&attachment=/opt/a.jpg
	 *  mailto:aa@b.com,hh@j.com&cc=cc@d.com&bcc=ee@f.com&subject=hello&body=nice&attachment=/opt/a.jpg
	 */

	gchar **split_mailto = g_strsplit_set(mailto_uri, "?", -1);
	if (split_mailto) {
		gchar *first_uri = split_mailto[0] + strlen(COMPOSER_SCHEME_MAILTO);
		if (STR_VALID(first_uri)) {
			view->new_mail_info->mail_data->full_address_to = _initial_data_parse_mailto_recipients(first_uri);
		}

		/*gchar **split_contents = g_strsplit_set(split_mailto[1], "&", -1);
		if (split_contents) {
			int i;
			for (i = 0; i < g_strv_length(split_contents); i++) {
				if (g_str_has_prefix(split_contents[i], COMPOSER_MAILTO_TO_PREFIX)) {
					_recp_append_string(&view->new_mail_info->mail_data->full_address_to,
							_initial_data_parse_mailto_recipients(split_contents[i] + strlen(COMPOSER_MAILTO_TO_PREFIX)));
				} else if (g_str_has_prefix(split_contents[i], COMPOSER_MAILTO_CC_PREFIX)) {
					_recp_append_string(&view->new_mail_info->mail_data->full_address_cc,
							_initial_data_parse_mailto_recipients(split_contents[i] + strlen(COMPOSER_MAILTO_CC_PREFIX)));
				} else if (g_str_has_prefix(split_contents[i], COMPOSER_MAILTO_BCC_PREFIX)) {
					_recp_append_string(&view->new_mail_info->mail_data->full_address_bcc,
							_initial_data_parse_mailto_recipients(split_contents[i] + strlen(COMPOSER_MAILTO_BCC_PREFIX)));
				} else if (g_str_has_prefix(split_contents[i], COMPOSER_MAILTO_SUBJECT_PREFIX)) {
					g_free(view->new_mail_info->mail_data->subject);
					view->new_mail_info->mail_data->subject = g_uri_unescape_string(split_contents[i] + strlen(COMPOSER_MAILTO_SUBJECT_PREFIX), NULL);
				} else if (g_str_has_prefix(split_contents[i], COMPOSER_MAILTO_BODY_PREFIX)) {
					g_free(view->new_mail_info->mail_data->file_path_plain);
					view->new_mail_info->mail_data->file_path_plain = g_uri_unescape_string(split_contents[i] + strlen(COMPOSER_MAILTO_BODY_PREFIX), NULL);
				}
			}

			g_strfreev(split_contents);
		}*/
		g_strfreev(split_mailto);
	}

	debug_leave();
}

static void _initial_data_parse_contacts_sharing(EmailComposerView *view, email_params_h params, const char *operation)
{
	debug_enter();

	if (!g_strcmp0(operation, APP_CONTROL_OPERATION_SHARE)) {
		const char *data_type = "person";

		if (email_params_get_str_opt(params, APP_CONTROL_DATA_TYPE, &data_type) &&
			email_params_get_int(params, APP_CONTROL_DATA_ID, &view->shared_contact_id)) {

			view->shared_contacts_count = 1;

			if (strcmp(data_type, "person")) {
				if (!strcmp(data_type, "my_profile")) {
					view->is_sharing_my_profile = EINA_TRUE;
				} else {
					debug_warning("Invalid type: %s. Assume: person", data_type);
				}
			}
		} else {
			debug_error("Parsing failed!");
			return;
		}

	} else {
		const char **array = NULL;
		int i = 0;

		retm_if(!email_params_get_str_array(params, APP_CONTROL_DATA_ID, &array, &view->shared_contacts_count),
				"email_params_get_str_array(%s) failed!", APP_CONTROL_DATA_ID);

		if (view->shared_contacts_count > 0) {
			view->shared_contact_id_list = calloc(view->shared_contacts_count, sizeof(int));
			retm_if(!view->shared_contact_id_list, "calloc() failed");

			for (i = 0; i < view->shared_contacts_count; ++i) {
				view->shared_contact_id_list[i] = atoi(array[i]);
			}
		} else {
			debug_error("Array is empty");
		}
	}

	view->is_sharing_contacts = (view->shared_contacts_count > 0) ? EINA_TRUE : EINA_FALSE;
}

static void _initial_data_parse_attachments(EmailComposerView *view, Eina_List *attachment_uri_list)
{
	debug_enter();

	retm_if(!view, "view is NULL!");
	retm_if(!attachment_uri_list, "attachment_uri_list is NULL!");

	Eina_List *list = NULL;
	char *uri = NULL;
	EINA_LIST_FOREACH(attachment_uri_list, list, uri) {
		/* Remove "file://" scheme. */
		if (g_str_has_prefix(uri, COMPOSER_SCHEME_FILE)) {
			char *p = g_strdup(uri + strlen(COMPOSER_SCHEME_FILE));
			list->data = p;
			g_free(uri);
			uri = p;
		}

		/* TODO: Check. is it needed?
		 * Copy vCalendar file to composer temp directory.
		 */
		char *file_ext = email_parse_get_ext_from_filename(uri);
		if (!g_ascii_strncasecmp(file_ext, "vcs", strlen("vcs"))) {
			char tmp_file_path[EMAIL_FILEPATH_MAX] = { 0, };
			if (composer_util_file_copy_temp_file(uri, tmp_file_path, EMAIL_FILEPATH_MAX)) {
				char *p = g_strdup(tmp_file_path);
				list->data = p;
				g_free(uri);
				uri = p;
			}
		}
		g_free(file_ext);
	}

	composer_attachment_process_attachments_with_list(view, attachment_uri_list, ATTACH_BY_USE_ORG_FILE, EINA_FALSE);

	debug_leave();
}

static COMPOSER_ERROR_TYPE_E _initial_data_process_params_by_internal(EmailComposerView *view, const char **argv)
{
	debug_enter();

	if (argv[0]) {
		view->account_info->original_account_id = atoi(argv[0]);
	}

	switch (view->composer_type) {
		case RUN_EML_REPLY:
		case RUN_EML_FORWARD:
		case RUN_EML_REPLY_ALL:
			view->eml_file_path = g_strdup(argv[3]);
			break;
		case RUN_COMPOSER_NEW:
			break;
		case RUN_COMPOSER_REPLY:
		case RUN_COMPOSER_REPLY_ALL:
		case RUN_COMPOSER_EDIT:
		case RUN_COMPOSER_FORWARD:
			if (argv[1]) {
				view->original_mail_id = atoi(argv[1]);
			}
			break;

		default:
			debug_error("MUST NOT reach here!! type:[%d]", view->composer_type);
			return COMPOSER_ERROR_UNKOWN_TYPE;
	}

	debug_leave();
	return COMPOSER_ERROR_NONE;
}

static COMPOSER_ERROR_TYPE_E _initial_data_process_params_by_external(EmailComposerView *view, const char **argv)
{
	debug_enter();

	if (STR_VALID(argv[0])) {
		_recp_preppend_string(&view->new_mail_info->mail_data->full_address_to, g_strdup(argv[0]));
	}

	if (STR_VALID(argv[1])) {
		_recp_preppend_string(&view->new_mail_info->mail_data->full_address_cc, g_strdup(argv[1]));
	}

	if (STR_VALID(argv[2])) {
		_recp_preppend_string(&view->new_mail_info->mail_data->full_address_bcc, g_strdup(argv[2]));
	}

	if (STR_VALID(argv[3])) {
		g_free(view->new_mail_info->mail_data->subject);
		view->new_mail_info->mail_data->subject = g_strdup(argv[3]);
	}

	if (STR_VALID(argv[4])) {
		g_free(view->new_mail_info->mail_data->file_path_plain);
		view->new_mail_info->mail_data->file_path_plain = g_strdup(argv[4]);
	}

	debug_leave();
	return COMPOSER_ERROR_NONE;
}

static void _initial_data_set_mail_to_recipients(EmailComposerView *view)
{
	debug_enter();

	char *to_be_added_recipients = NULL;

	if (view->org_mail_info && view->org_mail_info->mail_data && view->org_mail_info->mail_data->full_address_from) {
		debug_secure("   full_address_from: (%s)", view->org_mail_info->mail_data->full_address_from);
		debug_secure("email_address_sender: (%s)", view->org_mail_info->mail_data->email_address_sender);
		debug_secure("  user_email_address: (%s)", view->account_info->selected_account->user_email_address);

		char *to_be_added_sender_address = NULL;

		/* In the case of "Reply all", we exclude my email address if it is a sender email address. */
		if ((view->composer_type == RUN_COMPOSER_REPLY_ALL) || (view->composer_type == RUN_EML_REPLY_ALL)) {
			if (!g_strstr_len(view->org_mail_info->mail_data->full_address_from, strlen(view->org_mail_info->mail_data->full_address_from), view->account_info->selected_account->user_email_address)) {
				to_be_added_sender_address = view->org_mail_info->mail_data->full_address_from;
			}
		} else if ((view->composer_type == RUN_COMPOSER_REPLY) || (view->composer_type == RUN_EML_REPLY)) {
			to_be_added_sender_address = view->org_mail_info->mail_data->full_address_from;
		}

		if (to_be_added_sender_address) {
			EmailRecpInfo *ri = composer_util_recp_make_recipient_info(to_be_added_sender_address);
			if (ri) {
				char *markup_name = elm_entry_utf8_to_markup(ri->display_name);
				elm_multibuttonentry_item_append(view->recp_to_mbe, markup_name, NULL, ri);
				FREE(markup_name);
			} else {
				debug_error("From address is invalid! ri is NULL!");
			}
		}
	}

	if ((view->composer_type == RUN_COMPOSER_REPLY_ALL) || (view->composer_type == RUN_COMPOSER_EDIT) || (view->composer_type == RUN_EML_REPLY_ALL)) {
		if (view->org_mail_info && view->org_mail_info->mail_data && view->org_mail_info->mail_data->full_address_to) {
			debug_secure("view->org_mail_info->mail_data->full_address_to: (%s)", view->org_mail_info->mail_data->full_address_to);
			to_be_added_recipients = view->org_mail_info->mail_data->full_address_to;
		}
	} else if (view->composer_type == RUN_COMPOSER_EXTERNAL) {
		debug_secure("view->new_mail_info->mail_data->full_address_to: (%s)", view->new_mail_info->mail_data->full_address_to);
		to_be_added_recipients = view->new_mail_info->mail_data->full_address_to;
	}

	_initial_data_mbe_append_recipients(view, view->recp_to_mbe, to_be_added_recipients);
	elm_multibuttonentry_expanded_set(view->recp_to_mbe, EINA_FALSE);

	debug_leave();
}

static void _initial_data_set_mail_cc_recipients(EmailComposerView *view)
{
	debug_enter();

	char *to_be_added_recipients = NULL;

	if ((view->composer_type == RUN_COMPOSER_REPLY_ALL) || (view->composer_type == RUN_COMPOSER_EDIT) || (view->composer_type == RUN_EML_REPLY_ALL)) {
		debug_secure("view->org_mail_info->mail_data->full_address_cc: (%s)", view->org_mail_info->mail_data->full_address_cc);
		to_be_added_recipients = view->org_mail_info->mail_data->full_address_cc;
	} else if (view->composer_type == RUN_COMPOSER_EXTERNAL) {
		debug_secure("view->new_mail_info->mail_data->full_address_cc: (%s)", view->new_mail_info->mail_data->full_address_cc);
		to_be_added_recipients = view->new_mail_info->mail_data->full_address_cc;
	}

	Eina_Bool ret = _initial_data_mbe_append_recipients(view, view->recp_cc_mbe, to_be_added_recipients);
	if (ret && !view->cc_added) {
		composer_recipient_show_hide_cc_field(view, EINA_TRUE);
		composer_recipient_show_hide_bcc_field(view, EINA_TRUE);
	}
	elm_multibuttonentry_expanded_set(view->recp_cc_mbe, EINA_FALSE);

	debug_leave();
}

static void _initial_data_set_mail_bcc_recipients(EmailComposerView *view)
{
	debug_enter();

	char *to_be_added_recipients = NULL;

	if ((view->composer_type == RUN_COMPOSER_REPLY_ALL) || (view->composer_type == RUN_COMPOSER_EDIT) || (view->composer_type == RUN_EML_REPLY_ALL)) {
		debug_secure("view->org_mail_info->mail_data->full_address_bcc: (%s)", view->org_mail_info->mail_data->full_address_bcc);
		to_be_added_recipients = view->org_mail_info->mail_data->full_address_bcc;
	} else if (view->composer_type == RUN_COMPOSER_EXTERNAL) {
		debug_secure("view->new_mail_info->mail_data->full_address_bcc: (%s)", view->new_mail_info->mail_data->full_address_bcc);
		to_be_added_recipients = view->new_mail_info->mail_data->full_address_bcc;
	}

	Eina_Bool ret = _initial_data_mbe_append_recipients(view, view->recp_bcc_mbe, to_be_added_recipients);
	if (ret && !view->bcc_added) {
		composer_recipient_show_hide_cc_field(view, EINA_TRUE);
		composer_recipient_show_hide_bcc_field(view, EINA_TRUE);
	}
	elm_multibuttonentry_expanded_set(view->recp_bcc_mbe, EINA_FALSE);

	debug_leave();
}

static void _initial_data_set_mail_subject(EmailComposerView *view)
{
	debug_enter();

	char temp_subject[EMAIL_BUFF_SIZE_1K] = { 0, };
	char *to_be_added_subject = NULL;

	if (view->org_mail_info->mail_data) {
		if (view->composer_type == RUN_COMPOSER_EDIT) {
			if (view->org_mail_info->mail_data->subject) {
				to_be_added_subject = view->org_mail_info->mail_data->subject;
			}
		} else if ((view->composer_type == RUN_COMPOSER_FORWARD) || (view->composer_type == RUN_EML_FORWARD)) {
			if (view->org_mail_info->mail_data->subject) {
				if ((strncasecmp(view->org_mail_info->mail_data->subject, "fw:", 3) == 0) ||
					(strncasecmp(view->org_mail_info->mail_data->subject, "fwd:", 4) == 0)) {
					char *p = strchr(view->org_mail_info->mail_data->subject, ':');
					snprintf(temp_subject, sizeof(temp_subject), "%s%s", email_get_email_string("IDS_EMAIL_BODY_FWD_C"), p + 1);
				} else {
					snprintf(temp_subject, sizeof(temp_subject), "%s %s", email_get_email_string("IDS_EMAIL_BODY_FWD_C"), view->org_mail_info->mail_data->subject);
				}
			} else {
				snprintf(temp_subject, sizeof(temp_subject), "%s ", email_get_email_string("IDS_EMAIL_BODY_FWD_C"));
			}
			to_be_added_subject = temp_subject;
		} else if ((view->composer_type == RUN_COMPOSER_REPLY) || (view->composer_type == RUN_COMPOSER_REPLY_ALL) ||
				(view->composer_type == RUN_EML_REPLY) || (view->composer_type == RUN_EML_REPLY_ALL)) {
			char *resp_text = email_get_email_string("IDS_EMAIL_HEADER_RE_C_M_READ_REPORT");

			if (view->org_mail_info->mail_data->subject) {
				if (strncasecmp(view->org_mail_info->mail_data->subject, "re:", 3) == 0) {
					char *p = strchr(view->org_mail_info->mail_data->subject, ':');
					snprintf(temp_subject, sizeof(temp_subject), "%s%s", resp_text, p + 1);
				} else {
					snprintf(temp_subject, sizeof(temp_subject), "%s %s", resp_text, view->org_mail_info->mail_data->subject);
				}
			} else {
				snprintf(temp_subject, sizeof(temp_subject), "%s ", resp_text);
			}
			to_be_added_subject = temp_subject;
		}
	} else if (view->new_mail_info->mail_data && view->new_mail_info->mail_data->subject) {
		if (view->composer_type == RUN_COMPOSER_EXTERNAL) {
			snprintf(temp_subject, sizeof(temp_subject), "%s ", view->new_mail_info->mail_data->subject);
			to_be_added_subject = temp_subject;
		}
	}

	if (to_be_added_subject) {
		char *temp = elm_entry_utf8_to_markup(to_be_added_subject);
		elm_entry_entry_set(view->subject_entry.entry, temp);
		FREE(temp);

		elm_entry_cursor_begin_set(view->subject_entry.entry);
	}

	if (view->new_mail_info->mail_data) {
		FREE(view->new_mail_info->mail_data->subject);
	}

	debug_leave();
}

static char *_initial_data_get_mail_body(EmailComposerView *view)
{
	debug_enter();

	char *html_document = NULL;
	char *body_document = NULL;
	char *current_content = NULL;
	char *parent_content = NULL;
	char *original_message_bar = NULL;

	if (view->composer_type == RUN_COMPOSER_EDIT) {
		if (view->org_mail_info->mail_data->body_download_status != EMAIL_BODY_DOWNLOAD_STATUS_NONE) {
			if (view->org_mail_info->mail_data->file_path_html) {
				body_document = (char *)email_get_buff_from_file(view->org_mail_info->mail_data->file_path_html, 0);
			} else if (view->org_mail_info->mail_data->file_path_plain) {
				char *temp_body_plain = (char *)email_get_buff_from_file(view->org_mail_info->mail_data->file_path_plain, 0);

				/* Convert plain-text to html content */
				body_document = (char *)email_get_parse_plain_text(temp_body_plain, STR_LEN(temp_body_plain));
				g_free(temp_body_plain);
			} else {
				body_document = (char *)email_get_buff_from_file(email_get_default_html_path(), 0);
			}

			/* To insert css style when the mail is in outbox because there's no css style on an outgoing mail. (Outbox > Mail > Menu > Edit) */
			if (view->org_mail_info->mail_data->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX) {
				char *temp_body = body_document;
				body_document = g_strconcat(EC_TAG_BODY_CSS_START, temp_body, EC_TAG_BODY_END, NULL);
				FREE(temp_body);
			}
		}
	} else {
		if (view->composer_type == RUN_COMPOSER_EXTERNAL) {
			if (view->new_mail_info->mail_data->file_path_plain) {
				current_content = (char *)email_get_parse_plain_text(view->new_mail_info->mail_data->file_path_plain, STR_LEN(view->new_mail_info->mail_data->file_path_plain));
				if (current_content) {
					body_document = g_strconcat(EC_TAG_BODY_CSS_START, current_content, EC_TAG_BODY_END, NULL);
				}
				free(view->new_mail_info->mail_data->file_path_plain);
				view->new_mail_info->mail_data->file_path_plain = NULL;
			}
		} else if (((view->composer_type == RUN_COMPOSER_REPLY || view->composer_type == RUN_COMPOSER_REPLY_ALL) && view->account_info->original_account->options.reply_with_body) ||
			(view->composer_type == RUN_COMPOSER_FORWARD)) {
			parent_content = _initial_data_body_get_parent_content(view);
			original_message_bar = _initial_data_body_get_original_message_bar_tags(view);

			if (parent_content) {
				body_document = g_strconcat(EC_TAG_BODY_ORIGINAL_MESSAGE_START, EC_TAG_NEW_MSG_START, EC_TAG_NEW_MSG_END, original_message_bar, EC_TAG_ORG_MSG_START, parent_content, EC_TAG_ORG_MSG_END, EC_TAG_BODY_END, NULL);
			}
		}
	}

	if (!body_document) {
		/* html_document will be freed on the place that this functions returned. */
		body_document = g_strconcat(EC_TAG_BODY_CSS_START, EC_TAG_BODY_END, NULL);
	}

	char html_meta_info[EC_HTML_META_INFO_SIZE];
	snprintf(html_meta_info, EC_HTML_META_INFO_SIZE, EC_HTML_META_INFO_FMT,
			email_get_misc_dir(), email_get_misc_dir());

	html_document = g_strconcat(html_meta_info, body_document, EC_TAG_HTML_END, NULL);

	g_free(body_document);
	g_free(current_content);
	g_free(parent_content);
	g_free(original_message_bar);

	debug_leave();
	return html_document;
}

static void _initial_data_set_mail_attachment(EmailComposerView *view)
{
	debug_enter();

	EmailComposerAccount *account_info = view->account_info;
	retm_if(!account_info, "account_info is NULL!");

	int i, j;
	int inline_count = 0, attach_count = 0;
	int total_attachment_size = 0;

	Eina_Bool ret = EINA_TRUE;

	char err_buff[EMAIL_BUFF_SIZE_HUG] = { 0, };

	if (view->composer_type == RUN_COMPOSER_EDIT || view->composer_type == RUN_COMPOSER_REPLY || view->composer_type == RUN_COMPOSER_REPLY_ALL || view->composer_type == RUN_COMPOSER_FORWARD) {
		/* Check the size of the inline images first */
		for (i = 0; i < view->org_mail_info->total_attachment_count; i++) {
			email_attachment_data_t *att_data = view->org_mail_info->attachment_list + i;
			if (att_data && att_data->inline_content_status) {
				total_attachment_size += att_data->attachment_size;
				inline_count++;

				if (inline_count > MAX_ATTACHMENT_ITEM) {
					composer_attachment_launch_attachment_error_popup(COMPOSER_ERROR_ATTACHMENT_MAX_NUMBER_EXCEEDED, composer_util_popup_response_cb, view);
					ret = EINA_FALSE;
					break;
				}

				if (ret && (total_attachment_size > account_info->max_sending_size)) {
					composer_attachment_launch_attachment_error_popup(COMPOSER_ERROR_ATTACHMENT_MAX_SIZE_EXCEEDED, composer_util_popup_response_cb, view);
					ret = EINA_FALSE;
				}

				if (att_data->save_status) {
					char src[PATH_MAX] = { 0, };
					if (realpath(att_data->attachment_path, src)) {
						free(att_data->attachment_path);
						att_data->attachment_path = strdup(src);

						char dest[PATH_MAX] = { 0, };
						snprintf(dest, sizeof(dest), "%s/%s", composer_util_file_get_temp_dirname(), att_data->attachment_name);

						debug_secure("inline_attach_path: [%s]", att_data->attachment_path);
						debug_secure("symbolic_link_path: [%s]", dest);

						int result = symlink(att_data->attachment_path, dest);
						if (result == -1) {
							debug_log("symlink() failed! (%d): %s", result, strerror_r(errno, err_buff, sizeof(err_buff)));
						}
					} else {
						debug_log("realpath() failed!: %s", strerror_r(errno, err_buff, sizeof(err_buff)));
					}
				}
			}
		}

		if (view->composer_type == RUN_COMPOSER_EDIT || (view->composer_type == RUN_COMPOSER_FORWARD && account_info->original_account->options.forward_with_files)) {
			if (view->org_mail_info->mail_data->attachment_count > 0) {
				int reference_attachment_count = 0;
				email_attachment_data_t *reference_attachment_list = NULL;

				if ((view->composer_type == RUN_COMPOSER_EDIT) && (view->org_mail_info->mail_data->reference_mail_id != 0) && view->org_mail_info->total_attachment_count) {
					email_engine_get_attachment_data_list(view->org_mail_info->mail_data->reference_mail_id, &reference_attachment_list, &reference_attachment_count);
				}

				for (j = 0; j < view->org_mail_info->total_attachment_count; j++) {
					email_attachment_data_t *att_data = view->org_mail_info->attachment_list + j;

					/* Check the mime type of the attachment if it is for s/mime. */
					if ((view->org_mail_info->mail_data->smime_type != EMAIL_SMIME_NONE) && email_is_smime_cert_attachment(att_data->attachment_mime_type)) {
						debug_secure("(%s) - mime_type:(%s) - skip it!", att_data->attachment_path, att_data->attachment_mime_type);
						continue;
					}

					debug_log("existing att size:[%d]", att_data->attachment_size);

					struct stat sb;
					if (stat(att_data->attachment_path, &sb) == -1) {
						debug_log("stat() failed! (%d): %s", errno, strerror_r(errno, err_buff, sizeof(err_buff)));

						int m = 0;
						for (m = 0; m < reference_attachment_count; m++) {
							if (!g_strcmp0(att_data->attachment_name, reference_attachment_list[m].attachment_name) && (att_data->attachment_size == reference_attachment_list[m].attachment_size)) {
								debug_secure("==> replace mail_id:[%d], att_id:[%d] ==> ref_id:[%d], att_id:[%d]", view->org_mail_info->mail_data->mail_id, att_data->attachment_id,
								             view->org_mail_info->mail_data->reference_mail_id, (reference_attachment_list + m)->attachment_id);
								att_data = reference_attachment_list + m;
							}
						}
					}

					if (!att_data->inline_content_status) {
						total_attachment_size += att_data->attachment_size;
						attach_count++;

						if (inline_count + attach_count > MAX_ATTACHMENT_ITEM) {
							composer_attachment_launch_attachment_error_popup(COMPOSER_ERROR_ATTACHMENT_MAX_NUMBER_EXCEEDED, composer_util_popup_response_cb, view);
							break;
						}

						if (ret && (total_attachment_size > account_info->max_sending_size)) {
							composer_attachment_launch_attachment_error_popup(COMPOSER_ERROR_ATTACHMENT_MAX_SIZE_EXCEEDED, composer_util_popup_response_cb, view);
							ret = EINA_FALSE;
						}

						email_attachment_data_t *attachment_data = (email_attachment_data_t *)calloc(1, sizeof(email_attachment_data_t));
						if (!attachment_data) {
							debug_error("Failed to allocate memory for attachment_data!");
							composer_attachment_launch_attachment_error_popup(COMPOSER_ERROR_OUT_OF_MEMORY, composer_util_popup_response_cb, view);
							break;
						}

						attachment_data->attachment_id = att_data->attachment_id;
						attachment_data->attachment_name = COMPOSER_STRDUP(att_data->attachment_name);
						attachment_data->attachment_path = COMPOSER_STRDUP(att_data->attachment_path);
						attachment_data->attachment_size = att_data->attachment_size;
						attachment_data->mail_id = att_data->mail_id;
						attachment_data->account_id = att_data->account_id;
						attachment_data->mailbox_id = att_data->mailbox_id;
						attachment_data->save_status = att_data->save_status;
						attachment_data->inline_content_status = att_data->inline_content_status;

						if (!attachment_data->save_status) {
							view->need_download = EINA_TRUE;
						}

						composer_attachment_ui_create_item_layout(view, attachment_data, attach_count > 1, EINA_FALSE);
					}
				}

				email_engine_free_attachment_data_list(&reference_attachment_list, reference_attachment_count);
			}
		}
	} else if ((view->composer_type == RUN_EML_REPLY) || (view->composer_type == RUN_EML_REPLY_ALL) || (view->composer_type == RUN_EML_FORWARD)) {
		do {
			total_attachment_size = (int)email_get_file_size(view->org_mail_info->mail_data->file_path_html);
			total_attachment_size += (int)email_get_file_size(view->org_mail_info->mail_data->file_path_plain);

			struct stat file_info;
			if (stat(view->eml_file_path, &file_info) == -1) {
				debug_error("stat() failed! (%d): %s", errno, strerror_r(errno, err_buff, sizeof(err_buff)));
				composer_attachment_launch_attachment_error_popup(COMPOSER_ERROR_ATTACHMENT_NOT_EXIST, composer_util_popup_response_cb, view);
				break;
			}

			total_attachment_size += file_info.st_size;
			if (total_attachment_size > view->account_info->max_sending_size) {
				composer_attachment_launch_attachment_error_popup(COMPOSER_ERROR_ATTACHMENT_MAX_SIZE_EXCEEDED, composer_util_popup_response_cb, view);
				break;
			}

			email_attachment_data_t *attachment_data = (email_attachment_data_t *)calloc(1, sizeof(email_attachment_data_t));
			if (!attachment_data) {
				debug_error("Failed to allocate memory for attachment_data!");
				composer_attachment_launch_attachment_error_popup(COMPOSER_ERROR_OUT_OF_MEMORY, composer_util_popup_response_cb, view);
				break;
			}

			gchar **tokens = g_strsplit(view->eml_file_path, "/", -1);
			attachment_data->attachment_name = COMPOSER_STRDUP(tokens[g_strv_length(tokens) - 1]);
			attachment_data->attachment_path = COMPOSER_STRDUP(view->eml_file_path);
			attachment_data->save_status = 1;
			attachment_data->attachment_size = file_info.st_size;
			g_strfreev(tokens);

			debug_secure("attachment_data->attachment_name:%s", attachment_data->attachment_name);
			debug_secure("attachment_data->attachment_size:%d", attachment_data->attachment_size);

			composer_attachment_ui_create_item_layout(view, attachment_data, EINA_FALSE, EINA_FALSE);
		} while (0);
	}

	composer_util_modify_send_button(view);

	debug_leave();
}

static Eina_List *_initial_data_make_initial_recipients_list(Evas_Object *mbe)
{
	debug_enter();

	retvm_if(!mbe, NULL, "Invalid parameter: mbe is NULL!");

	Eina_List *l = NULL;
	Eina_List *initial_list = NULL;
	Elm_Object_Item *item = NULL;

	const Eina_List *items_list = elm_multibuttonentry_items_get(mbe);
	if (items_list) {
		EINA_LIST_FOREACH((Eina_List *)items_list, l, item) {
			initial_list = eina_list_append(initial_list, item);
		}
	}

	debug_leave();
	return initial_list;
}


/*
 * Definition for exported functions
 */

char *composer_initial_data_body_make_parent_mail_info(EmailComposerView *view)
{
	debug_enter();

	char *text_for_original_info = NULL;
	char sent_time[EMAIL_BUFF_SIZE_BIG] = { 0, };
	char *sent_time_format = NULL;
	struct tm sent_tm;

	char buf[EMAIL_BUFF_SIZE_BIG] = { 0, };
	char text_for_orig[EMAIL_BUFF_SIZE_BIG] = { 0, };
	char text_for_from[EMAIL_BUFF_SIZE_1K] = { 0, };
	char text_for_sent[EMAIL_BUFF_SIZE_1K] = { 0, };
	char *text_for_to = NULL;
	char *text_for_cc = NULL;
	char *text_for_subject = NULL;

	if (view->org_mail_info && view->org_mail_info->mail_data) {
		/* Show "---- Original message ----" */
		snprintf(text_for_orig, sizeof(text_for_orig), EC_TAG_DIV_DIR_S"---- %s ----"EC_TAG_DIV_F, email_get_email_string("IDS_EMAIL_BODY_ORIGINAL_MESSAGE"));

		char *stripped_from_address = composer_util_strip_quotation_marks_in_email_address(view->org_mail_info->mail_data->full_address_from);
		if (stripped_from_address) {
			snprintf(text_for_from, sizeof(text_for_from), EC_TAG_DIV_DIR_S"%s: %s"EC_TAG_DIV_F, email_get_email_string("IDS_EMAIL_BODY_FROM_MSENDER"), stripped_from_address);
			free(stripped_from_address);
		}

		sent_time_format = composer_util_get_datetime_format();
		if (sent_time_format) {
			localtime_r(&view->org_mail_info->mail_data->date_time, &sent_tm);

			strftime(sent_time, sizeof(sent_time), sent_time_format, &sent_tm);
			snprintf(text_for_sent, sizeof(text_for_sent), EC_TAG_DIV_DIR_S"%s %s"EC_TAG_DIV_F, email_get_email_string("IDS_EMAIL_BODY_SENT_C_M_DATE"), sent_time);
			g_free(sent_time_format);
		}

		char *to_string = _initial_data_body_make_recipient_string(view->org_mail_info->mail_data->full_address_to, ";");
		if (!to_string) {
			to_string = g_strdup(view->org_mail_info->mail_data->full_address_to);
		}
		snprintf(buf, sizeof(buf), EC_TAG_DIV_DIR_S"%s: ", email_get_email_string("IDS_EMAIL_BODY_TO_M_RECIPIENT"));
		text_for_to = g_strconcat(buf, to_string, EC_TAG_DIV_F, NULL);
		FREE(to_string);

		if (view->org_mail_info->mail_data->full_address_cc) {
			char *cc_string = _initial_data_body_make_recipient_string(view->org_mail_info->mail_data->full_address_cc, ";");
			if (!cc_string) {
				cc_string = g_strdup(view->org_mail_info->mail_data->full_address_cc);
			}
			memset(buf, 0x0, sizeof(buf));
			snprintf(buf, sizeof(buf), EC_TAG_DIV_DIR_S"%s: ", email_get_email_string("IDS_EMAIL_BODY_CC"));
			text_for_cc = g_strconcat(buf, cc_string, EC_TAG_DIV_F, NULL);
			FREE(cc_string);
		}

		if (view->org_mail_info->mail_data->subject) {
			char *subject_markup = elm_entry_utf8_to_markup(view->org_mail_info->mail_data->subject);
			text_for_subject = g_strdup_printf(EC_TAG_DIV_DIR_S"%s: %s"EC_TAG_DIV_F, email_get_email_string("IDS_EMAIL_HEADER_SUBJECT"), subject_markup);
			g_free(subject_markup);
		} else {
			text_for_subject = g_strdup(EC_TAG_DIV_DIR);
		}

		if (text_for_cc && (strlen(text_for_cc) > 0)) {
			text_for_original_info = g_strconcat(text_for_orig, text_for_from, text_for_sent, text_for_to, text_for_cc, text_for_subject, EC_TAG_DIV_DIR, NULL);
		} else {
			text_for_original_info = g_strconcat(text_for_orig, text_for_from, text_for_sent, text_for_to, text_for_subject, EC_TAG_DIV_DIR, NULL);
		}

		g_free(text_for_subject);
		g_free(text_for_cc);
		g_free(text_for_to);
	}

	debug_leave();
	return text_for_original_info;
}

char *composer_initial_data_body_make_signature_markup(EmailComposerView *view)
{
	debug_enter();

	char *signature = NULL;

	if ((view->account_info->original_account->options).add_signature && (view->account_info->original_account->options).signature) {
		char *markup_signature = elm_entry_utf8_to_markup((view->account_info->original_account->options).signature);
		signature = g_strdup_printf(EC_TAG_SIGNATURE_TEXT, ATO014, markup_signature);
		g_free(markup_signature);
	} else {
		signature = g_strdup(EC_TAG_SIGNATURE_DEFAULT);
	}

	debug_leave();
	return signature;
}

void composer_initial_data_destroy_download_contents_popup(void *data)
{
	debug_enter();

	retm_if(!data, "data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	view->draft_sync_handle = 0;
	view->is_load_finished = EINA_FALSE;

	DELETE_EVAS_OBJECT(view->composer_popup);

	debug_leave();
}

void composer_initial_data_set_mail_info(EmailComposerView *view, bool is_draft_sync_requested)
{
	email_profiling_begin(composer_initial_data_set_mail_info);
	debug_enter();

	char *html_body = NULL;
	char file_path[EMAIL_FILEPATH_MAX] = { 0, };
	email_mail_data_t *mail_data = view->org_mail_info->mail_data;

	/* Check if attachment would be downloaded or not.
	 * 1. If the mail is in draft folder and the body and attachments hasn't been downloaded yet, those should be downloaded first!
	 * 2. If the mail is made on local (by forwarding -> save to drafts), reference_mail_id is set. In this case, downloading isn't needed.
	 */
	if (is_draft_sync_requested && (view->composer_type == RUN_COMPOSER_EDIT) && (view->org_mail_info->mail_data->reference_mail_id == 0)) {
		bool is_draft_sync_needed = false;
		if (mail_data->body_download_status != EMAIL_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED) {
			is_draft_sync_needed = true;
		} else {
			int i = 0;
			for (i = 0; i < view->org_mail_info->total_attachment_count; i++) {
				email_attachment_data_t *attachment_list = view->org_mail_info->attachment_list + i;
				if (attachment_list && !attachment_list->save_status && !attachment_list->inline_content_status) {
					is_draft_sync_needed = true;
					break;
				}
			}
		}

		if (is_draft_sync_needed) {
			_initial_data_download_contents(view);
			return;
		}
	}

	_initial_data_set_mail_to_recipients(view);
	_initial_data_set_mail_cc_recipients(view);
	_initial_data_set_mail_bcc_recipients(view);

	_initial_data_set_mail_subject(view);
	html_body = _initial_data_get_mail_body(view);
	retm_if(!html_body, "html_body: is [NULL]");
	_initial_data_set_mail_attachment(view);

	snprintf(file_path, sizeof(file_path), "file://%s", view->saved_html_path);
	debug_secure("file_path: (%s)", file_path);

	/* If encoding(5th parameter) is missing, "UTF-8" is used to display contents. */
	ewk_view_contents_set(view->ewk_view, html_body, strlen(html_body), NULL, view->original_charset, file_path);

	g_free(html_body);

	_initial_data_set_selected_entry(view);

	debug_leave();
	email_profiling_end(composer_initial_data_set_mail_info);
}

COMPOSER_ERROR_TYPE_E composer_initial_data_parse_composer_run_type(EmailComposerView *view, email_params_h params)
{
	email_profiling_begin(composer_initial_data_parse_composer_run_type);
	debug_enter();

	const char *operation = NULL;

	view->composer_type = RUN_TYPE_UNKNOWN;

	debug_warning_if(!email_params_get_operation_opt(params, &operation),
			"email_params_get_operation_opt() failed!");

	if (operation && (g_strcmp0(operation, APP_CONTROL_OPERATION_DEFAULT) != 0)) {
		debug_secure("operation type: (%s)", operation);
		view->composer_type = RUN_COMPOSER_EXTERNAL;
	} else {
		debug_warning_if(!email_params_get_int(params, EMAIL_BUNDLE_KEY_RUN_TYPE, &view->composer_type),
				"email_params_get_int(%s) failed!", EMAIL_BUNDLE_KEY_RUN_TYPE);
	}

	debug_leave();
	email_profiling_end(composer_initial_data_parse_composer_run_type);
	return COMPOSER_ERROR_NONE;
}

COMPOSER_ERROR_TYPE_E composer_initial_data_pre_parse_arguments(EmailComposerView *view, email_params_h params)
{
	email_profiling_begin(composer_initial_data_pre_parse_arguments);
	debug_enter();

	retvm_if(!view, COMPOSER_ERROR_INVALID_ARG, "Invalid parameter: view is NULL!");
	retvm_if(!params, COMPOSER_ERROR_INVALID_ARG, "Invalid parameter: params is NULL!");

	COMPOSER_ERROR_TYPE_E return_val = COMPOSER_ERROR_NONE;
	const char *operation = NULL;
	const char *uri = NULL;
	const char *mime = NULL;
	const char *argv[6] = { 0, };

	debug_warning_if(!email_params_get_operation_opt(params, &operation),
			"email_params_get_operation_opt() failed!");

	if (view->composer_type != RUN_COMPOSER_EXTERNAL) {
		debug_log("Composer launched internally! type:[%d]", view->composer_type);

		_initial_data_parse_params_by_internal(params, argv, view->composer_type);

		return_val = _initial_data_process_params_by_internal(view, argv);
		gotom_if(return_val != COMPOSER_ERROR_NONE, EXIT_FUNC, "_initial_data_process_params_by_internal() failed!");
	} else {
		debug_log("Composer launched externally!");

		if (operation == NULL) { /* module created by email_module_create_child */
			_initial_data_parse_params_from_parent_module(params, argv);
		} else { /* module created by appcontrol request */

			if (!g_strcmp0(operation, APP_CONTROL_OPERATION_COMPOSE)) {

				if (email_params_get_uri(params, &uri) &&
						g_str_has_prefix(uri, COMPOSER_SCHEME_MAILTO)) {
					_initial_data_parse_mailto_uri(view, uri);
				}

			} else if (!g_strcmp0(operation, APP_CONTROL_OPERATION_SHARE) ||
						!g_strcmp0(operation, APP_CONTROL_OPERATION_MULTI_SHARE)) {

				if (email_params_get_mime(params, &mime) &&
						!g_strcmp0(mime, COMPOSER_MIME_CONTACT_SHARE)) {
					_initial_data_parse_contacts_sharing(view, params, operation);
				}
			}

			_initial_data_parse_params_by_appcontrol(view, params, argv, operation);
		}

		return_val = _initial_data_process_params_by_external(view, argv);
		gotom_if(return_val != COMPOSER_ERROR_NONE, EXIT_FUNC, "_initial_data_process_params_by_external() failed!");
	}

EXIT_FUNC:
	debug_leave();
	email_profiling_end(composer_initial_data_pre_parse_arguments);
	return return_val;
}

COMPOSER_ERROR_TYPE_E composer_initial_data_post_parse_arguments(EmailComposerView *view, email_params_h params)
{
	email_profiling_begin(composer_initial_data_post_parse_arguments);
	debug_enter();

	retvm_if(!view, COMPOSER_ERROR_INVALID_ARG, "Invalid parameter: view is NULL!");
	retvm_if(!params, COMPOSER_ERROR_INVALID_ARG, "Invalid parameter: params is NULL!");

	if (view->composer_type == RUN_COMPOSER_EXTERNAL) {
		const char *operation = NULL;
		Eina_List *attachment_uri_list = NULL;

		debug_warning_if(!email_params_get_operation_opt(params, &operation),
				"email_params_get_operation_opt() failed!");

		/* TODO: the first case is needed? it seems like dead case. */
		if (operation == NULL) { /* module created by email_module_create_child */
			const char *attachment_uri = NULL;
			if (email_params_get_str(params, EMAIL_BUNDLE_KEY_ATTACHMENT, &attachment_uri)) {
				gchar **split_att = g_strsplit_set(attachment_uri, ";\n", -1);
				int i;
				for (i = 0; i < g_strv_length(split_att); i++) {
					if (split_att[i] && (strlen(split_att[i]) > 0)) {
						attachment_uri_list = eina_list_append(attachment_uri_list, g_strdup(split_att[i]));
					}
				}
				g_strfreev(split_att);
			}

		/* sharing contacts */
		} else if (view->is_sharing_contacts) {

			if (view->vcard_file_path) {

				if (view->vcard_save_thread) {
					pthread_join(view->vcard_save_thread, NULL);
					view->vcard_save_thread = 0;
					DELETE_TIMER_OBJECT(view->vcard_save_timer);
				}

				attachment_uri_list = eina_list_append(attachment_uri_list, g_strdup(view->vcard_file_path));

			} else if (view->vcard_save_thread) {
				ecore_timer_thaw(view->vcard_save_timer);
				composer_create_vcard_create_popup(view);
			}

		/* module created by appcontrol request */
		} else if (!g_strcmp0(operation, APP_CONTROL_OPERATION_COMPOSE) ||
					!g_strcmp0(operation, APP_CONTROL_OPERATION_SHARE) ||
					!g_strcmp0(operation, APP_CONTROL_OPERATION_MULTI_SHARE) ||
					!g_strcmp0(operation, APP_CONTROL_OPERATION_SHARE_TEXT)) {

			bool parse_data_path = true;

			/* 1. parse uri */
			if (!g_strcmp0(operation, APP_CONTROL_OPERATION_SHARE)) {
				const char *uri = NULL;
				if (email_params_get_uri(params, &uri)) {
					if (STR_VALID(uri) && !g_str_has_prefix(uri, COMPOSER_SCHEME_MAILTO)) {
						attachment_uri_list = eina_list_append(attachment_uri_list, g_strdup(uri));
						parse_data_path = false;
					}
				}
			}

			/* 2. parse value as "http://tizen.org/appcontrol/data/path" */
			if (parse_data_path) {
				const char *path = NULL;
				const char **path_array = NULL;
				int path_array_len = 0;
				bool is_array = true;
				bool ok = false;

				if (!email_params_get_is_array(params, APP_CONTROL_DATA_PATH, &is_array)) {
					debug_error("email_params_get_is_array() failed!");
				}

				debug_log("is_array = %d", is_array);
				if (is_array) {
					ok = email_params_get_str_array(params, APP_CONTROL_DATA_PATH, &path_array, &path_array_len);
				} else {
					ok = email_params_get_str(params, APP_CONTROL_DATA_PATH, &path);
				}

				if (!ok) {
					if (g_strcmp0(operation, APP_CONTROL_OPERATION_SHARE_TEXT)) {
						debug_warning("email_params_get_str***(%s) failed!", APP_CONTROL_DATA_PATH);
					}
				} else if (path_array) {
					int i = 0;
					for (i = 0; i < path_array_len; i++) {
						if (STR_VALID(path_array[i])) {
							attachment_uri_list = eina_list_append(attachment_uri_list, g_strdup(path_array[i]));
						}
					}
				} else {
					if (STR_VALID(path)) {
						attachment_uri_list = eina_list_append(attachment_uri_list, g_strdup(path));
					}
				}
			}
		}

		/* attachment_uri_list will be freed automatically after attaching files. */
		if (attachment_uri_list) {
			/* For logging. */
			Eina_List *l = NULL;
			char *p = NULL;
			EINA_LIST_FOREACH(attachment_uri_list, l, p) {
				debug_secure("[Attachment] (%s)", p);
			}

			_initial_data_parse_attachments(view, attachment_uri_list);
		}
	}

	debug_leave();
	email_profiling_end(composer_initial_data_post_parse_arguments);
	return COMPOSER_ERROR_NONE;
}

void composer_initial_data_make_initial_contents(EmailComposerView *view)
{
	email_profiling_begin(composer_initial_data_make_initial_contents);
	debug_enter();

	retm_if(!view, "Invalid parameter: view is NULL!");

	Eina_List *initial_list = NULL;
	Eina_List *list = NULL;
	ComposerAttachmentItemData *attach_item_data = NULL;

	if (view->recp_to_mbe) {
		view->initial_contents_to_list = _initial_data_make_initial_recipients_list(view->recp_to_mbe);
	}
	if (view->recp_cc_mbe) {
		view->initial_contents_cc_list = _initial_data_make_initial_recipients_list(view->recp_cc_mbe);
	}
	if (view->recp_bcc_mbe) {
		view->initial_contents_bcc_list = _initial_data_make_initial_recipients_list(view->recp_bcc_mbe);
	}

	view->initial_contents_subject = g_strdup(elm_entry_entry_get(view->subject_entry.entry));

	EINA_LIST_FOREACH(view->attachment_item_list, list, attach_item_data) {
		initial_list = eina_list_append(initial_list, (void *)attach_item_data);
	}
	view->initial_contents_attachment_list = initial_list;

	debug_leave();
	email_profiling_end(composer_initial_data_make_initial_contents);
}

void composer_initial_data_free_initial_contents(EmailComposerView *view)
{
	debug_enter();

	retm_if(!view, "Invalid parameter: view is NULL!");

	DELETE_LIST_OBJECT(view->initial_contents_to_list);
	DELETE_LIST_OBJECT(view->initial_contents_cc_list);
	DELETE_LIST_OBJECT(view->initial_contents_bcc_list);
	DELETE_LIST_OBJECT(view->initial_contents_attachment_list);

	if (view->initial_contents_subject) {
		g_free(view->initial_contents_subject);
		view->initial_contents_subject = NULL;
	}

	debug_leave();
}

static void _initial_data_set_selected_entry(EmailComposerView *view)
{
	debug_enter();

	if (elm_multibuttonentry_first_item_get(view->recp_to_mbe) != NULL) {
		if (g_strcmp0(elm_entry_entry_get(view->subject_entry.entry), "") == 0) {
			debug_log("To field is not empty, setting focus to subject_entry field.");
			view->selected_entry = view->subject_entry.entry;
		} else if (view->ewk_view) {
			debug_log("To field is not empty, setting focus to ewk_view field.");
			view->selected_entry = view->ewk_view;
			view->cs_bringin_to_ewk = true;
		}
	} else {
		debug_log("To field is empty, setting focus to To field.");
		view->selected_entry = view->recp_to_entry.entry;
	}

	debug_leave();
}

static void _recp_preppend_string(gchar **dst, gchar *src)
{
	retm_if(!dst, "dst is NULL");
	retm_if(!src, "src is NULL");
	gchar *old_str = *dst;
	if (old_str) {
		*dst = g_strconcat(src, ";", old_str, NULL);
		if (*dst) {
			g_free(old_str);
		} else {
			*dst = old_str;
		}
		g_free(src);
	} else {
		*dst = src;
	}
}

static void _recp_append_string(gchar **dst, gchar *src)
{
	retm_if(!dst, "dst is NULL");
	retm_if(!src, "src is NULL");
	gchar *old_str = *dst;
	if (old_str) {
		*dst = g_strconcat(old_str, ";", src, NULL);
		if (*dst) {
			g_free(old_str);
		} else {
			*dst = old_str;
		}
		g_free(src);
	} else {
		*dst = src;
	}
}

static void _recp_append_extra_data_array(gchar **dst, email_params_h params, const char *data_key)
{
	retm_if(!dst, "dst is NULL");
	retm_if(!params, "params is NULL");
	retm_if(!data_key, "data_key is NULL");

	const char **array = NULL;
	int array_length = 0;
	int i = 0;

	retm_if(!email_params_get_str_array(params, data_key, &array, &array_length),
			"email_params_get_str_array(%s) failed!", data_key);

	while (i < array_length) {
		_recp_append_string(dst, g_strdup(array[i]));
		++i;
	}
}
