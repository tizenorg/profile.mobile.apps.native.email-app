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

#include "email-engine.h"
#include "email-viewer.h"
#include "email-viewer-logic.h"
#include "email-viewer-initial-data.h"

int viewer_initial_data_pre_parse_arguments(void *data, app_control_h svc_handle)
{
	retvm_if(!data, VIEWER_ERROR_INVALID_ARG, "Invalid parameter: data is NULL!");
	retvm_if(!svc_handle, VIEWER_ERROR_INVALID_ARG, "Invalid parameter: svc_handle is NULL!");

	EmailViewerUGD *temp_data = (EmailViewerUGD *)data;

	int ret;
	char *account_id = NULL;
	char *mail_id = NULL;
	char *mailbox_id = NULL;
	char *landscape = NULL;
	char *operation = NULL;
	char *file_path = NULL;
	char *uri = NULL;

	ret = app_control_get_operation(svc_handle, &operation);
	debug_log_if(ret != 0, "app_control_get_operation: %d", ret);
	debug_secure("operation = %s", operation);

	if (operation == NULL) { /* ug called by ug_create */
		debug_log("operation: NULL");
	} else { /* ug called by appcontrol request */
		if (!g_strcmp0(operation, APP_CONTROL_OPERATION_VIEW)) {
			debug_log("operation: APP_CONTROL_OPERATION_VIEW");
		}
		G_FREE(operation);
	}
	ret = app_control_get_extra_data(svc_handle, EMAIL_BUNDLE_KEY_ACCOUNT_ID, (char **)&account_id);
	debug_log_if(ret != 0, "app_control_get_extra_data: %d", ret);
	debug_secure("account_id:%s", account_id);
	ret = app_control_get_extra_data(svc_handle, EMAIL_BUNDLE_KEY_MAIL_ID, (char **)&mail_id);
	debug_log_if(ret != 0, "app_control_get_extra_data: %d", ret);
	debug_secure("mail_id:%s", mail_id);
	ret = app_control_get_extra_data(svc_handle, EMAIL_BUNDLE_KEY_MAILBOX, (char **)&mailbox_id);
	debug_log_if(ret != 0, "app_control_get_extra_data: %d", ret);
	debug_secure("mailbox:%s", mailbox_id);
	ret = app_control_get_extra_data(svc_handle, EMAIL_BUNDLE_KEY_FIRST_LANDSCAPE, (char **)&landscape);
	debug_log_if(ret != 0, "app_control_get_extra_data: %d", ret);
	debug_log("landscape:%s", landscape);
	ret = app_control_get_extra_data(svc_handle, EMAIL_BUNDLE_KEY_MYFILE_PATH, (char **)&file_path);
	debug_log_if(ret != 0, "app_control_get_extra_data: %d", ret);
	debug_secure("file_path:%s", file_path);

	ret = app_control_get_uri(svc_handle, &uri);
	debug_log_if(ret != 0, "app_control_get_uri: %d", ret);
	debug_secure("uri:%s", uri);

	if (landscape) {
		temp_data->isLandscape = atoi(landscape);
		G_FREE(landscape);
	}

	if (account_id) {
		temp_data->account_id = atoi(account_id);
		viewer_create_account_data(temp_data);
		temp_data->folder_list = email_engine_get_ca_mailbox_list_using_glist(temp_data->account_id);
		G_FREE(account_id);
	}

	if (mail_id) {
		temp_data->mail_id = atoi(mail_id);
		g_free(mail_id);
	}

	if (mailbox_id) {
		temp_data->mailbox_id = atoi(mailbox_id);
		G_FREE(mailbox_id);
	}

	if (file_path) {
		debug_log("ug called by app-service request");
		temp_data->eml_file_path = g_strdup(file_path);
		G_FREE(file_path);
		temp_data->viewer_type = EML_VIEWER;
		debug_secure("eml_file_path: %s", temp_data->eml_file_path);
	} else if (uri) {
		debug_log("ug called by app-control request");
		if (g_str_has_prefix(uri, URI_SCHEME_FILE)) {
			temp_data->eml_file_path = g_strndup(uri + strlen(URI_SCHEME_FILE), strlen(uri) - strlen(URI_SCHEME_FILE));
		} else {
			temp_data->eml_file_path = g_strdup(uri);
		}
		temp_data->viewer_type = EML_VIEWER;
		debug_secure("eml_file_path: %s", temp_data->eml_file_path);
	} else {
		temp_data->viewer_type = EMAIL_VIEWER;
	}

	G_FREE(uri);

	debug_leave();
	return VIEWER_ERROR_NONE;
}

Eina_Bool viewer_free_viewer_data(void *data)
{
	retvm_if(!data, EINA_FALSE, "Invalid parameter: data is NULL!");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;
	Eina_Bool ret = EINA_TRUE;

	/* stop engine */
	if (ug_data->email_handle != 0) {
		email_engine_stop_working(ug_data->account_id, ug_data->email_handle);
		ug_data->email_handle = 0;
	}

	debug_log("free mail_info");
	if (ug_data->mail_info) {
		email_free_mail_data(&(ug_data->mail_info), 1);
		ug_data->mail_info = NULL;
	}

	if (ug_data->attachment_info && ug_data->attachment_count > 0) {
		email_free_attachment_data(&(ug_data->attachment_info), ug_data->attachment_count);
		ug_data->attachment_info = NULL;
		ug_data->attachment_count = 0;
	}

	ug_data->eml_file_path = NULL;
	FREE(ug_data->file_id);

	viewer_remove_internal_mail(ug_data);

	FREE(ug_data->webview_data);

	FREE(ug_data->account_email_address);

	return ret;
}

