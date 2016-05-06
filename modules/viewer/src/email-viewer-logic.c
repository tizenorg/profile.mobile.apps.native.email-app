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

#include "email-utils.h"
#include "email-debug.h"
#include "email-engine.h"

#include "email-viewer.h"
#include "email-viewer-util.h"
#include "email-viewer-logic.h"
#include "email-viewer-eml.h"

/*
 * Declaration for static functions
 */

static void _set_mail_flags(void *data);
static void _set_status(void *data);
static void _set_favorite(void *data);
static void _set_request_report(void *data);
static void _set_body_download(void *data);

static void _make_internal_sender(void *data);
static void _make_internal_date(void *data);
static void _make_internal_subject(void *data);
static void _make_internal_body(void *data);
static void _make_internal_html_body(void *data);
static void _make_internal_attachments(void *data);
static int _make_webview_data(void *data);

static void _remove_internal_sender(void *data);
static void _remove_internal_subject(void *data);
static void _remove_internal_body(void *data);
static void _remove_internal_attachments(void *data);

static int _get_valid_address_count(char *full_address);

gboolean viewer_get_internal_mail_info(void *data)
{
	debug_enter();
	retvm_if(data == NULL, EINA_FALSE, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *) data;

	email_engine_free_mail_data_list(&view->mail_info, 1);

	if (view->attachment_info) {
		debug_log("attachment_info freed p[%p] n[%d]", view->attachment_info, view->attachment_count);
		email_engine_free_attachment_data_list(&view->attachment_info, view->attachment_count);
		view->attachment_count = 0;
	}

	debug_secure("view->eml_file_path:%s", view->eml_file_path);
	if (view->eml_file_path == NULL) {
		if (!email_engine_get_mail_data(view->mail_id, &(view->mail_info))) {
			debug_log("fail to get mail data");
			return FALSE;
		}

		retvm_if(view->mail_info == NULL, FALSE, "mail_info is NULL");

		if (!email_engine_get_attachment_data_list(view->mail_id, &(view->attachment_info), &(view->attachment_count))) {
			debug_log("fail to get attachment data");
			return FALSE;
		}
	} else {
		if (email_get_file_size(view->eml_file_path) == 0) {
			debug_log("eml file size: zero");
			return FALSE;
		}
		if (!email_engine_parse_mime_file(view->eml_file_path, &(view->mail_info), &(view->attachment_info), &(view->attachment_count))) {
			debug_log("email_engine_parse_mime_file failed");
			return FALSE;
		}

		retvm_if(view->mail_info == NULL, FALSE, "mail_info is NULL");
	}

	debug_secure("sender address :%s", view->mail_info->email_address_sender);
	debug_log("view - ATTACH[%d], INLINE[%d]", view->mail_info->attachment_count, view->mail_info->inline_content_count);
	debug_log("Total attachment count of view->mail_info [%d]", view->mail_info->attachment_count + view->mail_info->inline_content_count);
	debug_log("view->attachment_info :%p", view->attachment_info);
	debug_log("view->attachment_count [%d]", view->attachment_count);
	if (view->attachment_info) {
		debug_log("view->attachment_info->attachment_size :%d", view->attachment_info->attachment_size);
	}

	debug_leave();
	return TRUE;
}

void viewer_make_internal_mail(void *data)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;
	email_mail_data_t *mail_info = view->mail_info;
	retm_if(mail_info == NULL, "mail_info is NULL");

	/* mail flags */
	_set_mail_flags(view);

	/* sender */
	_make_internal_sender(view);

	/* datetime */
	_make_internal_date(view);

	/* subject */
	_make_internal_subject(view);

	/* body */
	_make_internal_body(view);

	/* html body */
	_make_internal_html_body(view);

	/* attachment */
	_make_internal_attachments(view);
	debug_log("total_att_count(%d), normal_att_count(%d), has_attachment(%d)", view->total_att_count, view->normal_att_count, view->has_attachment);

	debug_leave();
}

void viewer_remove_internal_mail(void *data)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	/* sender */
	_remove_internal_sender(view);

	/* subject */
	_remove_internal_subject(view);

	/* body & html body */
	_remove_internal_body(view);

	/* attachment */
	_remove_internal_attachments(view);

	debug_leave();
}

void viewer_set_mail_seen(void *data)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;
	email_mail_data_t *mail_info = view->mail_info;
	retm_if(mail_info == NULL, "view->mail_info is NULL");

	if (!mail_info->flags_seen_field) {
		debug_log("newly arrived mail");

		int mail_id = mail_info->mail_id;
		email_engine_set_flags_field(mail_info->account_id, &mail_id, 1, EMAIL_FLAGS_SEEN_FIELD, 1, 1);
	}
	debug_leave();
}

static void _set_mail_flags(void *data)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	_set_status(view);
	_set_favorite(view);
	_set_request_report(view);
	_set_body_download(view);
	debug_leave();
}

static void _set_status(void *data)
{
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;
	email_mail_data_t *mail_info = view->mail_info;
	retm_if(mail_info == NULL, "mail_info is NULL");

	if (mail_info->flags_answered_field) {
		view->mail_type |= EMAIL_STATUS_REPLY;
	}

	if (mail_info->flags_forwarded_field) {
		view->mail_type |= EMAIL_STATUS_FORWARD;
	}

	view->save_status = mail_info->save_status;
	debug_log("EMAIL_STATUS_REPLY(%d), EMAIL_STATUS_FORWARD(%d), save_status(%d)", (view->mail_type & EMAIL_STATUS_REPLY), (view->mail_type & EMAIL_STATUS_FORWARD), view->save_status);
}

static void _set_favorite(void *data)
{
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;
	email_mail_data_t *mail_info = view->mail_info;
	retm_if(mail_info == NULL, "mail_info is NULL");

	view->favorite = mail_info->flags_flagged_field;
	debug_log("favorite (%d)", view->favorite);
}

static void _set_request_report(void *data)
{
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;
	email_mail_data_t *mail_info = view->mail_info;
	retm_if(mail_info == NULL, "mail_info is NULL");

	view->request_report = (mail_info->report_status & EMAIL_MAIL_REQUEST_MDN) ? TRUE : FALSE;
	debug_log("request_report (%d)", view->request_report);
}

static void _set_body_download(void *data)
{
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;
	email_mail_data_t *mail_info = view->mail_info;
	retm_if(mail_info == NULL, "mail_info is NULL");

	view->body_download_status = mail_info->body_download_status;
	debug_log("body_download_status (%d)", view->body_download_status);
}

static void _make_internal_sender(void *data)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;
	email_mail_data_t *mail_info = view->mail_info;
	retm_if(mail_info == NULL, "mail_info is NULL");

	if (STR_VALID(mail_info->full_address_from)) {
		if (STR_VALID(view->sender_address)) {
			FREE(view->sender_address);
		}
		debug_secure("head from (%s)", mail_info->full_address_from);
		gchar *nick = NULL;
		gchar *addr = NULL;

		if (!email_get_recipient_info(mail_info->full_address_from, &nick, &addr)) {
			debug_log("email_get_recipient_info failed");
			STR_LCPY(nick, mail_info->full_address_from, MAX_STR_LEN);
			STR_LCPY(addr, mail_info->full_address_from, MAX_STR_LEN);
		}
		debug_secure("nick [%s], addr [%s]", nick, addr);

		view->sender_display_name = g_strdup(nick);
		view->sender_address = g_strdup(addr);

		g_free(addr);
		g_free(nick);

		debug_secure("mail_data: alias_sender (%s), email_address_sender (%s)", mail_info->alias_sender, mail_info->email_address_sender);
		debug_secure("view: sender name (%s), sender address (%s)", view->sender_display_name, view->sender_address);
	}
	debug_secure("sender_address (%s)", view->sender_address ? view->sender_address : "@niL");
	debug_leave();
}

static void _make_internal_date(void *data)
{
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;
	email_mail_data_t *mail_info = view->mail_info;
	retm_if(mail_info == NULL, "mail_info is NULL");
	if (mail_info->save_status == EMAIL_MAIL_STATUS_SEND_SCHEDULED)
		view->mktime = mail_info->scheduled_sending_time;
	else
		view->mktime = mail_info->date_time;
	debug_secure("mail time = %s", ctime(&view->mktime));
}

static void _make_internal_subject(void *data)
{
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;
	email_mail_data_t *mail_info = view->mail_info;
	retm_if(mail_info == NULL, "mail_info is NULL");

	if (!STR_VALID(mail_info->subject)) {
		debug_log("mail_info->head->subject is not valid");
		return;
	}

	if (STR_VALID(view->subject)) {
		FREE(view->subject);
	}
	view->subject = g_strdup(mail_info->subject);
	debug_secure("subject (%s)", view->subject);
}

static void _make_internal_body(void *data)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;
	email_mail_data_t *mail_info = view->mail_info;
	retm_if(mail_info == NULL, "mail_info is NULL");

	if (STR_VALID(mail_info->file_path_plain)) {
		if (STR_VALID(view->body_uri)) {
			FREE(view->body_uri);
		}
		view->body_uri = g_strdup(mail_info->file_path_plain);
	}

	char *plain_charset = email_parse_get_filename_from_path(mail_info->file_path_plain);

	if (STR_VALID(plain_charset)) {
		if (STR_VALID(view->charset)) {
			FREE(view->charset);
		}
		debug_secure("plain_charset:%s", plain_charset);
		if (!g_ascii_strcasecmp(plain_charset, UNKNOWN_CHARSET_PLAIN_TEXT_FILE)) {
			g_free(plain_charset);
			view->charset = g_strdup(DEFAULT_CHARSET);
		} else {
			view->charset = plain_charset;
		}
		debug_secure("charset: %s", view->charset);
	}

	if (STR_VALID(mail_info->file_path_plain)) {
		if (STR_VALID(view->body)) {
			FREE(view->body);
		}
		view->body = email_get_buff_from_file(mail_info->file_path_plain, 0);
	}
	debug_leave();
}

static void _make_internal_html_body(void *data)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	view->has_html = FALSE;

	email_mail_data_t *mail_info = view->mail_info;
	retm_if(mail_info == NULL, "mail_info is NULL");

	char *html = mail_info->file_path_html;

	if (STR_VALID(html)) {
		debug_secure("html body path (%s)", html);

		if (STR_VALID(view->body_uri)) {
			FREE(view->body_uri);
		}

		view->body_uri = g_strdup(mail_info->file_path_html);
		view->has_html = TRUE;

		if (view->body_uri) {
			if (STR_VALID(view->charset)) {
				FREE(view->charset);
			}

			view->charset = email_parse_get_filename_from_path(view->body_uri);
			debug_secure("charset: %s", view->charset);
			if (!g_ascii_strcasecmp(view->charset, UNKNOWN_CHARSET_PLAIN_TEXT_FILE)) {
				g_free(view->charset);
				view->charset = g_strdup(DEFAULT_CHARSET);
			}
		}
	}
	debug_leave();
}

static void _make_internal_attachments(void *data)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;
	email_mail_data_t *mail_info = view->mail_info;
	retm_if(mail_info == NULL, "mail_info is NULL");

	view->has_attachment = FALSE;

	email_attachment_data_t *attach_data = view->attachment_info;
	int attach_count = view->attachment_count;
	retm_if(attach_data == NULL, "Invalid parameter: attach_data[NULL]");
	retm_if(attach_count <= 0, "Invalid parameter: attach_count <= 0");

	int index = 0;
	_remove_internal_attachments(view);

	int i = 0;
	for (i = 0; i < attach_count; i++) {
		email_attachment_data_t *attachment = attach_data + i;
		debug_log("attachment id (%d)", attachment->attachment_id);
		debug_secure("attachment name (%s)", attachment->attachment_name ? attachment->attachment_name : "@niL");
		debug_secure("attachment path (%s)", attachment->attachment_path ? attachment->attachment_path : "@niL");
		debug_secure("attachment content id (%s)", attachment->content_id ? attachment->content_id : "@niL");
		debug_secure("attachment mime type (%s)", attachment->attachment_mime_type ? attachment->attachment_mime_type : "@niL");
		debug_log("attachment size (%d)", attachment->attachment_size);
		debug_log("attachment download (%d)", attachment->save_status);
		debug_log("attachment inline? (%d)", attachment->inline_content_status);
		EmailAttachmentType *attachment_info = g_new0(EmailAttachmentType, 1);
		if (attachment_info) {
			/* id */
			attachment_info->attach_id = attachment->attachment_id;

			/* index */
			attachment_info->index = ++index;

			/* name */
			if (STR_VALID(attachment->attachment_name)) {
				attachment_info->name = g_strdup(attachment->attachment_name);
			}
			/* path */
			if (STR_VALID(attachment->attachment_path)) {
				if (STR_INVALID(attachment_info->name)) {
					g_free(attachment_info->name);
					attachment_info->name = email_parse_get_filename_from_path(attachment->attachment_path);
				}
				attachment_info->path = g_strdup(attachment->attachment_path);
			}
			/* content id */
			if (STR_VALID(attachment->content_id)) {
				attachment_info->content_id = g_strdup(attachment->content_id);
			}
			/* size */
			if (attachment->attachment_size <= 0) {
				attachment_info->size = email_get_file_size(attachment->attachment_path);
			} else {
				attachment_info->size = (guint64) attachment->attachment_size;
				debug_log("attachment_info size (%d)", (gint) attachment_info->size);
			}

			/* downloaded. */
			attachment_info->is_downloaded = attachment->save_status ? TRUE : FALSE;

			/* inline */
			attachment_info->inline_content = attachment->inline_content_status ? TRUE : FALSE;

			debug_log("inline_content(%d), inline_content_status(%d) ", attachment_info->inline_content, attachment->inline_content_status);
			if (attachment_info->inline_content) {
				view->has_inline_image = TRUE;
			}
			if (!g_strcmp0(attachment->attachment_mime_type, "application/smil") ||
					!g_strcmp0(attachment->attachment_mime_type, "application/smil+xml")) {
				view->is_smil_mail = TRUE;
			}

			if (viewer_is_normal_attachment(attachment_info) || (view->is_smil_mail)) {
				(view->normal_att_count)++;
				view->total_att_size += (guint64) attachment_info->size;
			}

			(view->total_att_count)++;

			view->attachment_info_list = g_list_append(view->attachment_info_list, attachment_info);
		}
	}

	if (view->normal_att_count > 0) {
		view->has_attachment = TRUE;
	}
	if (view->is_smil_mail) {
		view->has_inline_image = FALSE;
	}

	debug_log("total_att_count(%d), normal_att_count(%d), has_attachment(%d)", view->total_att_count, view->normal_att_count, view->has_attachment);
	debug_leave();
}

static void _remove_internal_sender(void *data)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;
	FREE(view->sender_address);
	FREE(view->sender_display_name);
	debug_leave();
}

static void _remove_internal_subject(void *data)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;
	FREE(view->subject);
	debug_leave();
}

static void _remove_internal_body(void *data)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;
	FREE(view->body_uri);
	FREE(view->charset);
	FREE(view->body);
	debug_leave();
}

static void _remove_internal_attachments(void *data)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;
	if (view->attachment_info_list) {
		debug_log("Free the existing attachments..");
		int i = 0;
		LIST_ITER_START(i, view->attachment_info_list) {
			EmailAttachmentType *info = (EmailAttachmentType *)LIST_ITER_GET_DATA(i, view->attachment_info_list);
			FREE(info->name);
			FREE(info->path);
			FREE(info->content_id);
			FREE(info);
		}
		g_list_free(view->attachment_info_list);
		view->attachment_info_list = NULL;
		view->normal_att_count = 0;
		view->total_att_size = 0;
		view->total_att_count = 0;
	}
	debug_leave();
}

char *viewer_str_removing_quots(char *src)
{
	retvm_if(src == NULL, NULL, "Invalid parameter: src[NULL]");

	int i = 0, j = 0;
	int len = strlen(src);
	char *ret;

	ret = (char *)calloc((len + 1), sizeof(char));
	retvm_if(!ret, NULL, "ret is NULL!");

	for (i = 0; i < len; i++) {
		if (src[i] != '"') {
			ret[j++] = src[i];
		}
	}
	return ret;
}

/* Creates hard links to all the inline images */
/* Image Viewer does not support the symbolic link. */
void viewer_make_hard_link_for_inline_images(void *data, const char *path)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;
	email_attachment_data_t *attach_data = view->attachment_info;
	int attach_count = view->attachment_count;
	retm_if(attach_data == NULL, "Invalid parameter: attach_data[NULL]");
	retm_if(attach_count <= 0, "Invalid parameter: attach_count <= 0");

	int i = 0;
	char dest[EMAIL_BUFF_SIZE_1K] = { 0, };
	char err_buff[EMAIL_BUFF_SIZE_HUG] = { 0, };
	int result = -1;

	debug_log("view->body_download_status : %d", view->body_download_status);
	for (i = 0; i < attach_count; i++) {
		if (attach_data[i].inline_content_status && attach_data[i].save_status) {
			snprintf(dest, sizeof(dest), "%s/%s", path, attach_data[i].attachment_name);
			debug_secure("inline_attach_path: [%s]", attach_data[i].attachment_path);
			debug_secure("hard_link_path: [%s]", dest);
			if (email_file_exists(dest)) {
				result = email_file_remove(dest);
				if (result == -1) {
					debug_error("remove(dest) failed! (%d): %s", result,
							strerror_r(errno, err_buff, sizeof(err_buff)));
				}
			}

			if (!email_file_cp(attach_data[i].attachment_path, dest)) {
				debug_error("link() failed! (%d): %s", result,
						strerror_r(errno, err_buff, sizeof(err_buff)));
			}
		}
	}
	debug_leave();
}

/* Remove hard links to all the inline images */
void viewer_remove_hard_link_for_inline_images(void *data)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;
	email_attachment_data_t *attach_data = view->attachment_info;
	int attach_count = view->attachment_count;

	int i = 0;
	char dest[EMAIL_BUFF_SIZE_1K] = { 0, };
	char err_buff[EMAIL_BUFF_SIZE_HUG] = { 0, };
	int result = -1;

	debug_log("view->body_download_status : %d", view->body_download_status);
	for (i = 0; i < attach_count; i++) {
		if (attach_data != NULL) {
			if (attach_data[i].inline_content_status && attach_data[i].save_status) {
				snprintf(dest, sizeof(dest), "%s/%s", view->temp_folder_path, attach_data[i].attachment_name);
				/*debug_secure("inline_attach_path: [%s]", attach_data[i].attachment_path);*/
				/*debug_secure("hard_link_path: [%s]", dest);*/
				if (email_file_exists(dest)) {
					result = email_file_remove(dest);
					if (result == -1) {
						debug_error("remove(dest) failed! (%d): %s", result,
								strerror_r(errno, err_buff, sizeof(err_buff)));
					}
				}
			}
		}
	}
	debug_leave();
}

Eina_Bool viewer_is_normal_attachment(EmailAttachmentType *info)
{
	if ((info && !info->inline_content)) {
		return EINA_TRUE;
	}
	return EINA_FALSE;
}

static int _make_webview_data(void *data)
{
	debug_enter();
	retvm_if(data == NULL, -1, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *) data;
	EmailViewerWebview *wvd;
	if (view->webview_data == NULL) {
		wvd = (EmailViewerWebview *)malloc(sizeof(EmailViewerWebview));
		retvm_if(!wvd, -1, "wvd is NULL!");
		memset(wvd, 0x0, sizeof(EmailViewerWebview));
		view->webview_data = wvd;
	}

	if (STR_VALID(view->charset)) {
		view->webview_data->charset = view->charset;
	} else {
		view->webview_data->charset = DEFAULT_CHARSET;
	}
	debug_secure("charset:%s", view->webview_data->charset);

	if (view->has_html) {
		view->webview_data->body_type_prev = BODY_TYPE_HTML;
		view->webview_data->body_type = BODY_TYPE_HTML;
		view->webview_data->uri = view->body_uri;
	} else {
		view->webview_data->body_type_prev = BODY_TYPE_TEXT;
		view->webview_data->body_type = BODY_TYPE_TEXT;
		view->webview_data->text_content = view->body;
	}
	return 0;
}

int viewer_set_internal_data(void *data, Eina_Bool b_remove)
{
	debug_enter();
	retvm_if(data == NULL, VIEWER_ERROR_INVALID_ARG, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	if (b_remove == EINA_TRUE) {
		viewer_remove_internal_mail(view);
		if (viewer_get_internal_mail_info(view)) {
			viewer_make_internal_mail(view);
			if (view->total_att_count > 0) {
				viewer_make_hard_link_for_inline_images(view, view->temp_folder_path);
			}
			_make_webview_data(view);
		} else {
			debug_log("viewer_get_internal_mail_info() failed!");
			return VIEWER_ERROR_GET_DATA_FAIL;
		}
	} else {
		viewer_remove_internal_mail(view);
		viewer_make_internal_mail(view);
		if (view->total_att_count > 0) {
			viewer_make_hard_link_for_inline_images(view, view->temp_folder_path);
		}
		_make_webview_data(view);
	}
	debug_leave();
	return VIEWER_ERROR_NONE;
}

static int _get_valid_address_count(char *full_address)
{
	debug_enter();
	retvm_if(full_address == NULL, 0, "full_address is NULL");

	guint index = 0;
	gchar *recipient = g_strdup(full_address);
	recipient = g_strstrip(recipient);
	debug_secure("recipient (%s)", recipient);
	gchar **recipient_list = g_strsplit_set(recipient, ";", -1);
	G_FREE(recipient);

	while ((recipient_list[index] != NULL) && (strlen(recipient_list[index]) > 0)) {
		/*debug_secure("recipient_list[%s], index:%d", recipient_list[index], index);*/
		recipient_list[index] = g_strstrip(recipient_list[index]);
		/*debug_secure("recipient_list[%s]", recipient_list[index]);*/
		gchar *nick = NULL;
		gchar *addr = NULL;
		if (!email_get_recipient_info(recipient_list[index], &nick, &addr)) {
			debug_log("email_get_recipient_info failed");
			addr = g_strdup(recipient_list[index]);
		}

		if (!email_get_address_validation(addr)) {
			++index;
			G_FREE(nick);
			G_FREE(addr);
			continue;
		}

		G_FREE(nick);
		G_FREE(addr);
		if (index > 1) {
			break;
		}
		index++;
	}
	g_strfreev(recipient_list);

	return index;
}

void viewer_get_address_info_count(email_mail_data_t *mail_info, int *to_count, int *cc_count, int *bcc_count)
{
	debug_enter();
	retm_if(mail_info == NULL, "mail_info is NULL");

	*to_count = _get_valid_address_count(mail_info->full_address_to);
	*cc_count = _get_valid_address_count(mail_info->full_address_cc);
	*bcc_count = _get_valid_address_count(mail_info->full_address_bcc);
}
/* EOF */
