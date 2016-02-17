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

int viewer_initial_data_pre_parse_arguments(void *data, email_params_h params)
{
	retvm_if(!data, VIEWER_ERROR_INVALID_ARG, "Invalid parameter: data is NULL!");
	retvm_if(!params, VIEWER_ERROR_INVALID_ARG, "Invalid parameter: params is NULL!");

	EmailViewerUGD *temp_data = (EmailViewerUGD *)data;

	if (email_params_get_int(params, EMAIL_BUNDLE_KEY_ACCOUNT_ID, &temp_data->account_id)) {
		viewer_create_account_data(temp_data);
		temp_data->folder_list = email_engine_get_ca_mailbox_list_using_glist(temp_data->account_id);
	}
	debug_secure("account_id:%d", temp_data->account_id);

	email_params_get_int_opt(params, EMAIL_BUNDLE_KEY_MAIL_ID, &temp_data->mail_id);
	debug_secure("mail_id:%d", temp_data->mail_id);
	email_params_get_int_opt(params, EMAIL_BUNDLE_KEY_MAILBOX, &temp_data->mailbox_id);
	debug_secure("mailbox:%d", temp_data->mailbox_id);
	email_params_get_int_opt(params, EMAIL_BUNDLE_KEY_FIRST_LANDSCAPE, &temp_data->isLandscape);
	debug_log("landscape:%d", temp_data->isLandscape);

	const char *file_path = NULL;
	const char *uri = NULL;

	if (email_params_get_str(params, EMAIL_BUNDLE_KEY_MYFILE_PATH, &file_path)) {
		debug_log("ug called by app-control request");
		temp_data->eml_file_path = g_strdup(file_path);
		temp_data->viewer_type = EML_VIEWER;
		debug_secure("eml_file_path: %s", temp_data->eml_file_path);
	} else if (email_params_get_uri(params, &uri)) {
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

