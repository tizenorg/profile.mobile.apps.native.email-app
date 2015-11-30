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

#include <app_control.h>
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

static void _launch_app_recipient_contacts_reply_cb(void *data, app_control_result_e result, app_control_h reply);
static void _preview_attachment_close_cb(void *data);

static contacts_list_h _get_person_email_list_by_person_id(int person_id);
static contacts_list_h _get_person_email_list_by_email_id(int person_id);

static int _composer_pick_contacts_set_params(EmailComposerUGD *ugd, app_control_h svc_handle);
static int _composer_add_update_contact_launch(EmailComposerUGD *ugd,
		const char *operation,
		const char *contact_email,
		const char *contact_name);
static int _composer_add_update_contact_set_params(EmailComposerUGD *ugd,
		app_control_h service,
		const char *operation,
		const char *contact_email,
		const char *contact_name);

static EmailCommonStringType EMAIL_COMPOSER_STRING_NULL = { NULL, NULL };
static EmailCommonStringType EMAIL_COMPOSER_STRING_BUTTON_OK = { PACKAGE, "IDS_EMAIL_BUTTON_OK" };
static EmailCommonStringType EMAIL_COMPOSER_STRING_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED = { PACKAGE, "IDS_EMAIL_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED" };
static EmailCommonStringType EMAIL_COMPOSER_STRING_UNABLE_TO_OPEN_FILE = { PACKAGE, "IDS_EMAIL_HEADER_UNABLE_TO_OPEN_FILE_ABB" };
static EmailCommonStringType EMAIL_COMPOSER_STRING_FILE_NOT_SUPPORTED = { PACKAGE, "IDS_EMAIL_POP_THIS_FILE_TYPE_IS_NOT_SUPPORTED_BY_ANY_APPLICATION_ON_YOUR_DEVICE" };
static EmailCommonStringType EMAIL_COMPOSER_UNABLE_TO_LAUNCH_APPLICATION = { NULL, N_("Unable to launch application") };


/*
 * Definition for static functions
 */

static int _composer_add_update_contact_launch(EmailComposerUGD *ugd,
		const char *operation,
		const char *contact_email,
		const char *contact_name)
{
	debug_enter();

	app_control_h svc_handle = NULL;
	int ret = app_control_create(&svc_handle);
	retvm_if((ret != APP_CONTROL_ERROR_NONE || !svc_handle),
			COMPOSER_ERROR_FAILED_TO_LAUNCH_APPLICATION,
			"app_control_create() failed![%d]", ret);

	ret = app_control_set_mime(svc_handle, EMAIL_CONTACT_MIME_SCHEME);
	if (ret != APP_CONTROL_ERROR_NONE) {
		debug_error("app_control_set_mime failed![%d]", ret);
		app_control_destroy(svc_handle);
		return COMPOSER_ERROR_FAILED_TO_LAUNCH_APPLICATION;
	}

	if (_composer_add_update_contact_set_params(ugd, svc_handle,
			operation, contact_email, contact_name) != APP_CONTROL_ERROR_NONE) {
		debug_error("_composer_add_update_contact_set_params failed![%d]", ret);
		app_control_destroy(svc_handle);
		return COMPOSER_ERROR_FAILED_TO_LAUNCH_APPLICATION;
	}

	ret = email_module_launch_app(ugd->base.module, EMAIL_LAUNCH_APP_AUTO, svc_handle, NULL);
	if (ret != 0) {
		debug_error("email_module_launch_app() failed![%d]", ret);
		app_control_destroy(svc_handle);
		return COMPOSER_ERROR_FAILED_TO_LAUNCH_APPLICATION;
	}

	app_control_destroy(svc_handle);
	debug_leave();

	return COMPOSER_ERROR_NONE;
}

static int _composer_pick_contacts_set_params(EmailComposerUGD *ugd, app_control_h svc_handle)
{
	int ret = app_control_set_operation(svc_handle, APP_CONTROL_OPERATION_PICK);
	retvm_if(ret != APP_CONTROL_ERROR_NONE, COMPOSER_ERROR_FAILED_TO_LAUNCH_APPLICATION, "app_control_set_operation() failed! ret:[%d]", ret);

	ret = app_control_add_extra_data(svc_handle, EMAIL_CONTACT_EXT_DATA_SELECTION_MODE, EMAIL_CONTACT_BUNDLE_SELECT_MULTIPLE);
	retvm_if(ret != APP_CONTROL_ERROR_NONE, COMPOSER_ERROR_FAILED_TO_LAUNCH_APPLICATION, "app_control_add_extra_data() failed! ret:[%d]", ret);

	ret = app_control_add_extra_data(svc_handle, EMAIL_CONTACT_EXT_DATA_TYPE, EMAIL_CONTACT_BUNDLE_EMAIL);
	retvm_if(ret != APP_CONTROL_ERROR_NONE, COMPOSER_ERROR_FAILED_TO_LAUNCH_APPLICATION, "app_control_add_extra_data() failed! ret:[%d]", ret);

	char max_count[BUF_LEN_T] = { 0 };
	snprintf(max_count, sizeof(max_count), "%d", MAX_RECIPIENT_COUNT);
	ret = app_control_add_extra_data(svc_handle, EMAIL_CONTACT_EXT_DATA_TOTAL_COUNT, max_count);
	retvm_if(ret != APP_CONTROL_ERROR_NONE, COMPOSER_ERROR_FAILED_TO_LAUNCH_APPLICATION, "app_control_add_extra_data() failed! ret:[%d]", ret);

	return COMPOSER_ERROR_NONE;
}

static int _composer_add_update_contact_set_params(EmailComposerUGD *ugd,
		app_control_h service,
		const char *operation,
		const char *contact_email,
		const char *contact_name)
{
	int ret = app_control_set_operation(service, operation);
	retvm_if(ret != APP_CONTROL_ERROR_NONE, COMPOSER_ERROR_FAILED_TO_LAUNCH_APPLICATION, "app_control_set_operation failed: %d", ret);

	ret = app_control_add_extra_data(service, EMAIL_CONTACT_EXT_DATA_EMAIL, contact_email);
	retvm_if(ret != APP_CONTROL_ERROR_NONE, COMPOSER_ERROR_FAILED_TO_LAUNCH_APPLICATION, "app_control_add_extra_data failed: %d", ret);

	if (contact_name) {
		ret = app_control_add_extra_data(service, EMAIL_CONTACT_EXT_DATA_NAME, contact_name);
		retvm_if(ret != APP_CONTROL_ERROR_NONE, COMPOSER_ERROR_FAILED_TO_LAUNCH_APPLICATION, "app_control_add_extra_data failed: %d", ret);
	}

	return COMPOSER_ERROR_NONE;
}

static contacts_list_h _get_person_email_list_by_person_id(int person_id)
{
	debug_enter();

	int ret = CONTACTS_ERROR_NONE;
	contacts_list_h list = NULL;
	contacts_query_h query = NULL;
	contacts_filter_h filter = NULL;

	ret = contacts_query_create(_contacts_person_email._uri, &query);
	retvm_if(ret != CONTACTS_ERROR_NONE, NULL, "contacts_query_create() failed! ret:[%d]");

	ret = contacts_filter_create(_contacts_person_email._uri, &filter);
	gotom_if(ret != CONTACTS_ERROR_NONE, FUNC_EXIT, "contacts_filter_create() failed! ret:[%d]");

	ret = contacts_filter_add_int(filter, _contacts_person_email.person_id, CONTACTS_MATCH_EQUAL, person_id);
	gotom_if(ret != CONTACTS_ERROR_NONE, FUNC_EXIT, "contacts_filter_add_int() failed! ret:[%d]");

	ret = contacts_query_set_filter(query, filter);
	gotom_if(ret != CONTACTS_ERROR_NONE, FUNC_EXIT, "contacts_query_set_filter() failed! ret:[%d]");

	ret = contacts_db_get_records_with_query(query, 0, 0, &list);
	gotom_if(ret != CONTACTS_ERROR_NONE, FUNC_EXIT, "contacts_db_get_records_with_query() failed! ret:[%d]");

FUNC_EXIT:
	if (filter) {
		ret = contacts_filter_destroy(filter);
		debug_warning_if(ret != CONTACTS_ERROR_NONE, "contacts_filter_destroy() failed! ret:[%d]");
	}

	if (query) {
		ret = contacts_query_destroy(query);
		debug_warning_if(ret != CONTACTS_ERROR_NONE, "contacts_query_destroy() failed! ret:[%d]");
	}

	debug_leave();
	return list;
}

static contacts_list_h _get_person_email_list_by_email_id(int person_id)
{
	debug_enter();

	debug_enter();

	int ret = CONTACTS_ERROR_NONE;
	contacts_list_h list = NULL;
	contacts_query_h query = NULL;
	contacts_filter_h filter = NULL;

	ret = contacts_query_create(_contacts_person_email._uri, &query);
	retvm_if(ret != CONTACTS_ERROR_NONE, NULL, "contacts_query_create() failed! ret:[%d]");

	ret = contacts_filter_create(_contacts_person_email._uri, &filter);
	gotom_if(ret != CONTACTS_ERROR_NONE, FUNC_EXIT, "contacts_filter_create() failed! ret:[%d]");

	ret = contacts_filter_add_int(filter, _contacts_person_email.email_id, CONTACTS_MATCH_EQUAL, person_id);
	gotom_if(ret != CONTACTS_ERROR_NONE, FUNC_EXIT, "contacts_filter_add_int() failed! ret:[%d]");

	ret = contacts_query_set_filter(query, filter);
	gotom_if(ret != CONTACTS_ERROR_NONE, FUNC_EXIT, "contacts_query_set_filter() failed! ret:[%d]");

	ret = contacts_db_get_records_with_query(query, 0, 0, &list);
	gotom_if(ret != CONTACTS_ERROR_NONE, FUNC_EXIT, "contacts_db_get_records_with_query() failed! ret:[%d]");

FUNC_EXIT:
	if (filter) {
		ret = contacts_filter_destroy(filter);
		debug_warning_if(ret != CONTACTS_ERROR_NONE, "contacts_filter_destroy() failed! ret:[%d]");
	}

	if (query) {
		ret = contacts_query_destroy(query);
		debug_warning_if(ret != CONTACTS_ERROR_NONE, "contacts_query_destroy() failed! ret:[%d]");
	}

	debug_leave();
	return list;
}

static void _launch_app_recipient_contacts_reply_cb(void *data, app_control_result_e result, app_control_h reply)
{
	debug_enter();

	retm_if(!data, "data is NULL!");
	retm_if(!reply, "reply is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	ugd->recipient_added_from_contacts = EINA_TRUE;

	int i = 0;
	char **return_value = NULL;
	int len = 0;

	int ret = app_control_get_extra_data_array(reply, EMAIL_CONTACT_EXT_DATA_SELECTED, &return_value, &len);
	retm_if(ret != APP_CONTROL_ERROR_NONE, "app_control_get_extra_data_array() failed! ret:[%d]", ret);

	for (i = 0; i < len ; i++) {

		int person_id = 0;
		char *display_name = NULL;
		contacts_list_h person_email_list = NULL;
		contacts_record_h h_person_email = NULL;
		bool is_display_name_duplicate = false;
		int res_email_id = atoi(return_value[i]);

		person_email_list = _get_person_email_list_by_email_id(res_email_id);
		if (!person_email_list) {
			debug_warning("There's no person for email_id:(%d)", res_email_id);
			continue;
		}

		ret = contacts_list_get_current_record_p(person_email_list, &h_person_email);
		if (ret != CONTACTS_ERROR_NONE) {
			debug_warning("contacts_list_get_current_record_p() failed (%d)", ret);
			contacts_list_destroy(person_email_list, true);
			continue;
		}

		ret = contacts_record_get_str(h_person_email, _contacts_person_email.display_name, &display_name);
		if (ret != CONTACTS_ERROR_NONE) {
			debug_error("contacts_record_get_str failed: %d", ret);
		}

		ret = contacts_record_get_int(h_person_email, _contacts_person_email.person_id, &person_id);
		if (ret != CONTACTS_ERROR_NONE) {
			debug_error("contacts_record_get_int failed: %d", ret);
		}

		contacts_list_destroy(person_email_list, true);
		person_email_list = NULL;

		EmailRecpInfo *ri = (EmailRecpInfo *)calloc(1, sizeof(EmailRecpInfo));
		if (!ri) {
			debug_error("Memory allocation failed!");
			continue;
		}
		ri->person_id = person_id;
		ri->display_name = display_name;

		person_email_list = _get_person_email_list_by_person_id(person_id);
		if (!person_email_list) {
			debug_warning("There's no person for person_id:(%d)", person_id);
			free(ri);
			continue;
		}

		do {
			contacts_record_h h_record = NULL;
			contacts_list_get_current_record_p(person_email_list, &h_record);

			if (h_record) {

				char *email_address = NULL;
				bool b_free_email_address = FALSE;
				int email_type = 0;
				int email_id = 0;

				contacts_record_get_str_p(h_record, _contacts_person_email.email, &email_address);
				contacts_record_get_int(h_record, _contacts_person_email.type, &email_type);
				contacts_record_get_int(h_record, _contacts_person_email.email_id, &email_id);

				if ((eina_list_count(ri->email_list) == FIRST_EMAIL_ADD) && (!g_strcmp0(ri->display_name, email_address))) {
					is_display_name_duplicate = true;

					char *validated_disp_name = NULL;
					char *validated_add = NULL;
					email_get_recipient_info(email_address, &validated_disp_name, &validated_add);
					if (is_display_name_duplicate && validated_disp_name) {
						FREE(display_name);
						ri->display_name = g_strdup(validated_disp_name);
					}
					email_address = g_strdup(validated_add);
					b_free_email_address = TRUE;
					FREE(validated_disp_name);
					FREE(validated_add);
				}

				if (res_email_id == email_id) {
					ri->selected_email_idx = eina_list_count(ri->email_list);
					debug_log("selected_email_idx:[%d]", ri->selected_email_idx);
				}

				EmailAddressInfo *ai = (EmailAddressInfo *)calloc(1, sizeof(EmailAddressInfo));
				if (ai) {
					snprintf(ai->address, sizeof(ai->address), "%s", email_address);
					ai->type = email_type;
					ri->email_list = eina_list_append(ri->email_list, ai);

					debug_secure("[%d] address:(%s), type:(%d)", eina_list_count(ri->email_list), ai->address, ai->type);
				}
				if (b_free_email_address) {
					FREE(email_address);
				}
			}
			ret = contacts_list_next(person_email_list);
		} while (ret == CONTACTS_ERROR_NONE);

		ret = contacts_list_destroy(person_email_list, true);
		debug_warning_if(ret != CONTACTS_ERROR_NONE, "contacts_list_destroy() is failed (%d)", ret);

		char *markup_name = elm_entry_utf8_to_markup(ri->display_name);
		if (ugd->selected_entry == ugd->recp_to_entry.entry) {
			elm_multibuttonentry_item_append(ugd->recp_to_mbe, markup_name, NULL, ri);
		} else if (ugd->selected_entry == ugd->recp_cc_entry.entry) {
			elm_multibuttonentry_item_append(ugd->recp_cc_mbe, markup_name, NULL, ri);
		} else if (ugd->selected_entry == ugd->recp_bcc_entry.entry) {
			elm_multibuttonentry_item_append(ugd->recp_bcc_mbe, markup_name, NULL, ri);
		} else {
			debug_error("Not matched!!");

			if (ri) {
				if (ri->display_name)
					free(ri->display_name);
				free(ri);
			}
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
		debug_secure("app_control_send_launch_request() failed!");
		return;
	}
	evas_object_freeze_events_set(ugd->ewk_view, EINA_TRUE);

	debug_leave();
}

void composer_launcher_launch_storage_settings(EmailComposerUGD *ugd)
{
	debug_enter();
	retm_if(!ugd, "Invalid parameter: ugd is NULL!");

	app_control_h svc_handle = NULL;
	int ret = app_control_create(&svc_handle);
	retm_if(ret != APP_CONTROL_ERROR_NONE, "app_control_create() failed![%d]", ret);

	ret = email_module_launch_app(ugd->base.module, EMAIL_LAUNCH_APP_STORAGE_SETTINGS, svc_handle, NULL);
	if (ret != 0) {
		debug_error("email_module_launch_app failed!");
	}

	app_control_destroy(svc_handle);

	debug_leave();
}

void composer_launcher_pick_contacts(EmailComposerUGD *ugd)
{
	debug_enter();

	retm_if(!ugd, "Invalid parameter: ugd is NULL!");

	app_control_h svc_handle = NULL;

	while (1) {

		int ret = app_control_create(&svc_handle);
		if (ret != APP_CONTROL_ERROR_NONE || !svc_handle) {
			debug_error("app_control_create() failed![%d]", ret);
			break;
		}

		ret = app_control_set_mime(svc_handle, EMAIL_CONTACT_MIME_SCHEME);
		if (ret != APP_CONTROL_ERROR_NONE) {
			debug_error("app_control_set_mime failed![%d]", ret);
			break;
		}

		ret = _composer_pick_contacts_set_params(ugd, svc_handle);
		if (ret != COMPOSER_ERROR_NONE) {
			debug_error("_composer_pick_contacts_set_params failed![%d]", ret);
			break;
		}

		email_launched_app_listener_t listener = { 0 };
		listener.cb_data = ugd;
		listener.reply_cb = _launch_app_recipient_contacts_reply_cb;

		ret = email_module_launch_app(ugd->base.module, EMAIL_LAUNCH_APP_AUTO, svc_handle, &listener);
		if (ret != 0) {
			debug_error("email_module_launch_app() failed![%d]", ret);
			break;
		}

		app_control_destroy(svc_handle);

		debug_leave();
		return;
	}

	app_control_destroy(svc_handle);
	ugd->composer_popup = composer_util_popup_create(ugd,
			EMAIL_COMPOSER_UNABLE_TO_LAUNCH_APPLICATION,
			EMAIL_COMPOSER_STRING_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED,
			composer_util_popup_response_cb,
			EMAIL_COMPOSER_STRING_BUTTON_OK,
			EMAIL_COMPOSER_STRING_NULL,
			EMAIL_COMPOSER_STRING_NULL);

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
