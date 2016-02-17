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

#include <account.h>

#include "email-popup-utils.h"
#include "email-composer.h"
#include "email-composer-util.h"
#include "email-composer-launcher.h"
#include "email-composer-types.h"
#include "email-composer-attachment.h"
#include "email-composer-js.h"
#include "email-composer-webkit.h"

#define FIRST_EMAIL_ADD	0

/*
 * Declaration for static functions
 */

static void _launch_app_recipient_contacts_reply_cb(void *data, app_control_result_e result, email_params_h reply);
static void _preview_attachment_close_cb(void *data);

static int _composer_add_update_contact_launch(EmailComposerUGD *ugd,
		const char *operation,
		const char *contact_email,
		const char *contact_name);

static email_string_t EMAIL_COMPOSER_STRING_NULL = { NULL, NULL };
static email_string_t EMAIL_COMPOSER_STRING_BUTTON_OK = { PACKAGE, "IDS_EMAIL_BUTTON_OK" };
static email_string_t EMAIL_COMPOSER_STRING_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED = { PACKAGE, "IDS_EMAIL_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED" };
static email_string_t EMAIL_COMPOSER_STRING_UNABLE_TO_OPEN_FILE = { PACKAGE, "IDS_EMAIL_HEADER_UNABLE_TO_OPEN_FILE_ABB" };
static email_string_t EMAIL_COMPOSER_STRING_FILE_NOT_SUPPORTED = { PACKAGE, "IDS_EMAIL_POP_THIS_FILE_TYPE_IS_NOT_SUPPORTED_BY_ANY_APPLICATION_ON_YOUR_DEVICE" };
static email_string_t EMAIL_COMPOSER_UNABLE_TO_LAUNCH_APPLICATION = { NULL, N_("Unable to launch application") };


/*
 * Definition for static functions
 */

static int _composer_add_update_contact_launch(EmailComposerUGD *ugd,
		const char *operation,
		const char *contact_email,
		const char *contact_name)
{
	debug_enter();

	int ret = -1;
	email_params_h params = NULL;

	if (email_params_create(&params) &&
		email_params_set_mime(params, EMAIL_CONTACT_MIME_SCHEME) &&
		email_params_set_operation(params, operation) &&
		email_params_add_str(params, EMAIL_CONTACT_EXT_DATA_EMAIL, contact_email) &&
		(!contact_name ||
		email_params_add_str(params, EMAIL_CONTACT_EXT_DATA_NAME, contact_name))) {

		ret = email_module_launch_app(ugd->base.module, EMAIL_LAUNCH_APP_AUTO, params, NULL);
	}

	email_params_free(&params);

	if (ret != 0) {
		debug_error("Launch failed! [%d]", ret);
		return COMPOSER_ERROR_FAILED_TO_LAUNCH_APPLICATION;
	}

	debug_leave();
	return COMPOSER_ERROR_NONE;
}

static void _launch_app_recipient_contacts_reply_cb(void *data, app_control_result_e result, email_params_h reply)
{
	debug_enter();

	retm_if(!data, "data is NULL!");
	retm_if(!reply, "reply is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	ugd->recipient_added_from_contacts = EINA_TRUE;

	int i = 0;
	const char **return_value = NULL;
	int len = 0;

	retm_if(!email_params_get_str_array(reply, EMAIL_CONTACT_EXT_DATA_SELECTED, &return_value, &len),
			"email_params_get_str_array() failed!");

	for (i = 0; i < len ; i++) {

		bool is_display_name_duplicate = false;
		int res_email_id = atoi(return_value[i]);

		email_contact_list_info_t *contact_data = email_contacts_get_contact_info_by_email_id(res_email_id);
		if (!contact_data) {
			debug_warning("There's no contact information for email_id:(%d)", res_email_id);
			continue;
		}

		EmailRecpInfo *ri = (EmailRecpInfo *)calloc(1, sizeof(EmailRecpInfo));
		if (!ri) {
			debug_error("Memory allocation failed!");
			continue;
		}
		ri->email_id = res_email_id;
		ri->display_name = g_strdup(contact_data->display_name);

		if ((eina_list_count(ri->email_list) == FIRST_EMAIL_ADD) && (!g_strcmp0(ri->display_name, contact_data->email_address))) {
			is_display_name_duplicate = true;

			char *validated_disp_name = NULL;
			char *validated_add = NULL;
			email_get_recipient_info(contact_data->email_address, &validated_disp_name, &validated_add);
			if (is_display_name_duplicate && validated_disp_name) {
				g_free(ri->display_name);
				ri->display_name = g_strdup(validated_disp_name);
			}
			FREE(contact_data->email_address);
			contact_data->email_address = strdup(validated_add);

			FREE(validated_disp_name);
			FREE(validated_add);
		}

		ri->selected_email_idx = eina_list_count(ri->email_list);
		debug_log("selected_email_idx:[%d]", ri->selected_email_idx);

		ri->selected_email_idx = eina_list_count(ri->email_list);
		debug_log("selected_email_idx:[%d]", ri->selected_email_idx);

		EmailAddressInfo *ai = (EmailAddressInfo *)calloc(1, sizeof(EmailAddressInfo));
		if (ai) {
			snprintf(ai->address, sizeof(ai->address), "%s", contact_data->email_address);
			ai->type = contact_data->email_type;
			ri->email_list = eina_list_append(ri->email_list, ai);

			debug_secure("[%d] address:(%s), type:(%d)", eina_list_count(ri->email_list), ai->address, ai->type);
		}
		email_contacts_delete_contact_info(&contact_data);

		char *markup_name = elm_entry_utf8_to_markup(ri->display_name);

		if (ugd->selected_entry == ugd->recp_to_entry.entry) {
			elm_multibuttonentry_item_append(ugd->recp_to_mbe, markup_name, NULL, ri);
		} else if (ugd->selected_entry == ugd->recp_cc_entry.entry) {
			elm_multibuttonentry_item_append(ugd->recp_cc_mbe, markup_name, NULL, ri);
		} else if (ugd->selected_entry == ugd->recp_bcc_entry.entry) {
			elm_multibuttonentry_item_append(ugd->recp_bcc_mbe, markup_name, NULL, ri);
		} else {
			debug_error("Not matched!!");
			g_free(ri->display_name);
			FREE(ri);
		}
		FREE(markup_name);
	}

	ugd->recipient_added_from_contacts = EINA_FALSE;
	debug_leave();
}

static void _preview_attachment_close_cb(void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	evas_object_freeze_events_set(ugd->ewk_view, EINA_FALSE);

	debug_leave();
}

/*
 * Definition for exported functions
 */

void composer_launcher_preview_attachment(EmailComposerUGD *ugd, const char *uri)
{
	debug_enter();

	retm_if(!ugd, "Invalid parameter: ugd is NULL!");
	retm_if(!uri, "Invalid parameter: uri is NULL!");

	debug_log("uri:%s", uri);

	email_launched_app_listener_t listener = { 0 };
	listener.cb_data = ugd;
	listener.close_cb = _preview_attachment_close_cb;

	int ret = email_preview_attachment_file(ugd->base.module, uri, &listener);
	if (ret != 0) {
		ugd->composer_popup = composer_util_popup_create(ugd, EMAIL_COMPOSER_STRING_UNABLE_TO_OPEN_FILE, EMAIL_COMPOSER_STRING_FILE_NOT_SUPPORTED,
				composer_util_popup_response_cb, EMAIL_COMPOSER_STRING_BUTTON_OK, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);
		debug_secure("email_preview_attachment_file() failed!");
		return;
	}
	evas_object_freeze_events_set(ugd->ewk_view, EINA_TRUE);

	debug_leave();
}

void composer_launcher_launch_storage_settings(EmailComposerUGD *ugd)
{
	debug_enter();
	retm_if(!ugd, "Invalid parameter: ugd is NULL!");

	int ret = -1;
	email_params_h params = NULL;

	if (email_params_create(&params)) {

		ret = email_module_launch_app(ugd->base.module, EMAIL_LAUNCH_APP_STORAGE_SETTINGS, params, NULL);
	}

	email_params_free(&params);

	if (ret != 0) {
		debug_error("Launch failed! [%d]", ret);
	}

	debug_leave();
}

void composer_launcher_pick_contacts(EmailComposerUGD *ugd)
{
	debug_enter();

	retm_if(!ugd, "Invalid parameter: ugd is NULL!");

	int ret = -1;
	email_params_h params = NULL;

	if (email_params_create(&params) &&
		email_params_set_mime(params, EMAIL_CONTACT_MIME_SCHEME) &&
		email_params_set_operation(params, APP_CONTROL_OPERATION_PICK) &&
		email_params_add_str(params, EMAIL_CONTACT_EXT_DATA_SELECTION_MODE, EMAIL_CONTACT_BUNDLE_SELECT_MULTIPLE) &&
		email_params_add_str(params, EMAIL_CONTACT_EXT_DATA_TYPE, EMAIL_CONTACT_BUNDLE_EMAIL) &&
		email_params_add_int(params, EMAIL_CONTACT_EXT_DATA_TOTAL_COUNT, MAX_RECIPIENT_COUNT)) {

		email_launched_app_listener_t listener = { 0 };
		listener.cb_data = ugd;
		listener.reply_cb = _launch_app_recipient_contacts_reply_cb;

		ret = email_module_launch_app(ugd->base.module, EMAIL_LAUNCH_APP_AUTO, params, &listener);
	}

	email_params_free(&params);

	if (ret != 0) {
		debug_error("Launch failed! [%d]", ret);

		ugd->composer_popup = composer_util_popup_create(ugd,
				EMAIL_COMPOSER_UNABLE_TO_LAUNCH_APPLICATION,
				EMAIL_COMPOSER_STRING_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED,
				composer_util_popup_response_cb,
				EMAIL_COMPOSER_STRING_BUTTON_OK,
				EMAIL_COMPOSER_STRING_NULL,
				EMAIL_COMPOSER_STRING_NULL);
	}

	debug_leave();
}

void composer_launcher_update_contact(EmailComposerUGD *ugd)
{
	debug_enter();

	retm_if(!ugd, "Invalid parameter: ugd is NULL!");

	while (true) {

		EmailRecpInfo *ri = (EmailRecpInfo *) elm_object_item_data_get(ugd->selected_mbe_item);
		if (!ri) {
			debug_error("email recipient info is NULL!");
			break;
		}

		EmailAddressInfo *ai = eina_list_nth(ri->email_list, ri->selected_email_idx);
		if (!ai) {
			debug_error("email address info is NULL!");
			break;
		}

		int ret = _composer_add_update_contact_launch(ugd, APP_CONTROL_OPERATION_EDIT, ai->address, NULL);
		if (ret != COMPOSER_ERROR_NONE) {
			debug_error("_composer_add_update_contact_launch failed![%d]", ret);
			break;
		}
		debug_leave();
		return;
	}

	ugd->composer_popup = composer_util_popup_create(ugd,
			EMAIL_COMPOSER_UNABLE_TO_LAUNCH_APPLICATION,
			EMAIL_COMPOSER_STRING_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED,
			composer_util_popup_response_cb,
			EMAIL_COMPOSER_STRING_BUTTON_OK,
			EMAIL_COMPOSER_STRING_NULL,
			EMAIL_COMPOSER_STRING_NULL);

	debug_leave();
}

void composer_launcher_add_contact(EmailComposerUGD *ugd)
{
	debug_enter();

	retm_if(!ugd, "Invalid parameter: ugd is NULL!");

	while (true) {

		EmailRecpInfo *ri = (EmailRecpInfo *) elm_object_item_data_get(ugd->selected_mbe_item);
		if (!ri) {
			debug_error("email recipient info is NULL!");
			break;
		}

		EmailAddressInfo *ai = eina_list_nth(ri->email_list, ri->selected_email_idx);
		if (!ai) {
			debug_error("email address info is NULL!");
			break;
		}

		int ret = _composer_add_update_contact_launch(ugd, APP_CONTROL_OPERATION_ADD, ai->address, ri->display_name);
		if (ret != COMPOSER_ERROR_NONE) {
			debug_error("_composer_add_update_contact_launch failed![%d]", ret);
			break;
		}
		debug_leave();
		return;
	}

	ugd->composer_popup = composer_util_popup_create(ugd,
			EMAIL_COMPOSER_UNABLE_TO_LAUNCH_APPLICATION,
			EMAIL_COMPOSER_STRING_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED,
			composer_util_popup_response_cb,
			EMAIL_COMPOSER_STRING_BUTTON_OK,
			EMAIL_COMPOSER_STRING_NULL,
			EMAIL_COMPOSER_STRING_NULL);

	debug_leave();
}
