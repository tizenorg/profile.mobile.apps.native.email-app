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

#define _GNU_SOURCE

#include <string.h>

#include <ctype.h>
#include <time.h>
#include <net_connection.h>
#include <notification.h>
#include <system_settings.h>

#include "email-debug.h"
#include "email-utils.h"

#include "email-composer.h"
#include "email-composer-types.h"
#include "email-composer-util.h"
#include "email-composer-js.h"
#include "email-composer-initial-view.h"
#include "email-composer-predictive-search.h"
#include "email-composer-recipient.h"
#include "email-composer-launcher.h"

#define REPLACE_ARRAY_SIZE 3
#define RANDOM_BUFF_SIZE 33
#define RANDIOM_NUM_SIZE 4

#define EMAIL_BUNDLE_KEY_NAME_TOTAL_SIZE "http://tizen.org/appcontrol/data/total_size"

#define FILE_ACCESS_MODE 0644
#define VCARD_NAME_SUFFIX_LEN 16

static char *_composer_util_convert_dayformat(const char *format_str);
static Evas_Coord _composer_util_get_selected_widget_position(EmailComposerUGD *ugd);

static const char *_composer_util_file_get_unique_dirname(const char *root_dir, char *ec_dirname);

static void _composer_vcard_save_popup_cancel_cb(void *data, Evas_Object *obj, void *event_info);
static Eina_Bool _util_generate_random_string32(char *inString, int inSize);

EMAIL_DEFINE_GET_EDJ_PATH(email_get_composer_theme_path, "/email-composer-view.edj")
EMAIL_DEFINE_GET_SHARED_DATA_PATH(email_get_composer_tmp_dir, "/.email-composer-efl")
EMAIL_DEFINE_GET_SHARED_USER_PATH(email_get_composer_public_tmp_dir, "/.email-composer-efl")

static EmailCommonStringType EMAIL_COMPOSER_STRING_NULL = { NULL, NULL };
static EmailCommonStringType EMAIL_COMPOSER_STRING_BUTTON_OK = { PACKAGE, "IDS_EMAIL_BUTTON_OK" };

static EmailCommonStringType EMAIL_COMPOSER_STRING_UNABLE_TO_OPEN_FILE = { PACKAGE, "IDS_EMAIL_HEADER_UNABLE_TO_OPEN_FILE_ABB" };
static EmailCommonStringType EMAIL_COMPOSER_STRING_UNABLE_TO_DISPLAY_ATTACHMENT = { NULL, N_("Unable to display attachment.") };

static EmailCommonStringType EMAIL_COMPOSER_STRING_CREATING_VCARD = { PACKAGE, "IDS_EMAIL_POP_CREATING_VCARD_ING" };

static char *_composer_util_convert_dayformat(const char *format_str)
{
	retvm_if(!format_str, NULL, "Invalid parameter format_str: data is NULL!");
	const char *what[REPLACE_ARRAY_SIZE] = {"MM", "dd", "yy"};
	const char *on[REPLACE_ARRAY_SIZE] = {"%m", "%d", "%Y"};
	char *new_format_str = strdup(format_str);
	retvm_if(!new_format_str, NULL, "new_format_str is NULL!");
	int i = 0;

	for (; i < REPLACE_ARRAY_SIZE; i++) {
		char *new_str = NULL;
		char *old_str = NULL;
		char *head = NULL;
		char *tok = NULL;
		const char *substr = what[i];
		const char *replacement = on[i];

		new_str = new_format_str;
		head = new_str;

		while ((tok = strstr(head, substr))) {
			old_str = new_str;
			new_str = malloc(strlen(old_str) - strlen(substr) + strlen(replacement) + 1);

			if (new_str == NULL) {
				free(old_str);
				return NULL;
			}

			memcpy(new_str, old_str, tok - old_str);
			memcpy(new_str + (tok - old_str), replacement, strlen(replacement));
			memcpy(new_str + (tok - old_str) + strlen(replacement),
					tok + strlen(substr),
					strlen(old_str) - strlen(substr) - (tok - old_str));
			memset(new_str + strlen(old_str) - strlen(substr) + strlen(replacement), 0, 1);
			head = new_str + (tok - old_str) + strlen(replacement);
			free(old_str);
		}
		new_format_str = new_str;
	}
	return new_format_str;
}

char *composer_util_get_datetime_format()
{
	debug_enter();

	bool is_24hour = false;
	char format_buf[BUF_LEN_S] = {'\0'};
	char *format_date = NULL;
	char *format_time = NULL;
	char *country = NULL;

	char result[BUF_LEN_S] = {'\0'};
	int res = 0;

	res = system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_COUNTRY, &country);
	if (res != SYSTEM_SETTINGS_ERROR_NONE || country == NULL) {
		debug_error("failed to get system settings locale country data. "
				"res = %d, country = %s", res, country);
		return NULL;
	}

	res = email_generate_pattern_for_local(country, "yy-MM-dd", result, BUF_LEN_S);
	FREE(country);
	if (res != 0) {
		debug_error("failed to generate pattern for local");
		return NULL;
	}

	format_date = _composer_util_convert_dayformat(result);
	if (format_date == NULL) {
		debug_critical("failed to convert day format");
		return NULL;
	}

	res = system_settings_get_value_bool(SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR, &is_24hour);
	if (res != SYSTEM_SETTINGS_ERROR_NONE) {
		debug_error("failed to get system settings time format. res = %d", res);
		FREE(format_date);
		return NULL;
	}

	if (is_24hour) {
		format_time = "%H:%M:%S";
	} else {
		format_time = "%I:%M:%S %p";
	}

	snprintf(format_buf, sizeof(format_buf), "%s %s", format_date, format_time);
	debug_log("time:[%s], format:[%s]", (is_24hour ? "24h" : "12h"), format_buf);
	FREE(format_date);

	debug_leave();

	return g_strdup(format_buf);
}

char *composer_util_get_error_string(int type)
{
	debug_enter();

	char *ret = NULL;
	char str[BUF_LEN_L] = {'\0'};

	switch (type) {
		case EMAIL_ERROR_NETWORK_TOO_BUSY:
			ret = g_strdup(email_get_email_string("IDS_EMAIL_POP_NETWORK_BUSY"));
			break;
		case EMAIL_ERROR_LOGIN_ALLOWED_EVERY_15_MINS:
			ret = g_strdup(email_get_email_string("IDS_EMAIL_POP_YOU_CAN_ONLY_LOG_IN_ONCE_EVERY_15_MINUTES"));
			break;
		case EMAIL_ERROR_CONNECTION_FAILURE:
			ret = g_strdup(email_get_email_string("IDS_EMAIL_POP_CONNECTION_FAILED"));
			break;
		case EMAIL_ERROR_AUTHENTICATE:
			ret = g_strdup(email_get_email_string("IDS_EMAIL_POP_AUTHENTICATION_FAILED"));
			break;
		case EMAIL_ERROR_CANCELLED:
			ret = g_strdup(email_get_email_string("IDS_EMAIL_POP_CANCELLED"));
			break;
		case EMAIL_ERROR_MAIL_NOT_FOUND_ON_SERVER:
			ret = g_strdup(email_get_email_string("IDS_EMAIL_POP_THIS_EMAIL_HAS_BEEN_DELETED_FROM_THE_SERVER"));
			break;
		case EMAIL_ERROR_NO_SUCH_HOST:
			ret = g_strdup(email_get_email_string("IDS_EMAIL_POP_HOST_NOT_FOUND"));
			break;
		case EMAIL_ERROR_INVALID_SERVER:
			ret = g_strdup(email_get_email_string("IDS_EMAIL_POP_SERVER_NOT_AVAILABLE"));
			break;
		case EMAIL_ERROR_MAIL_MEMORY_FULL:
			ret = g_strdup(email_get_email_string("IDS_EMAIL_POP_THERE_IS_NOT_ENOUGH_SPACE_IN_YOUR_DEVICE_STORAGE"));
			break;
		case EMAIL_ERROR_FAILED_BY_SECURITY_POLICY:
			ret = g_strdup(email_get_email_string("IDS_EMAIL_POP_THE_CURRENT_EXCHANGE_SERVER_POLICY_PREVENTS_ATTACHMENTS_FROM_BEING_DOWNLOADED_TO_MOBILE_DEVICES"));
			break;
		case EMAIL_ERROR_ATTACHMENT_SIZE_EXCEED_POLICY_LIMIT:
			ret = g_strdup(email_get_email_string("IDS_EMAIL_POP_THE_MAXIMUM_ATTACHMENT_SIZE_ALLOWED_BY_THE_CURRENT_EXCHANGE_SERVER_POLICY_HAS_BEEN_EXCEEDED"));
			break;
		case EMAIL_ERROR_NETWORK_NOT_AVAILABLE:
		case EMAIL_ERROR_FLIGHT_MODE_ENABLE:
			ret = g_strdup(email_get_email_string("IDS_EMAIL_POP_FLIGHT_MODE_ENABLED_OR_NETWORK_NOT_AVAILABLE"));
			break;
		case EMAIL_ERROR_NO_RESPONSE:
			ret = g_strdup(email_get_email_string("IDS_EMAIL_POP_NO_RESPONSE_HAS_BEEN_RECEIVED_FROM_THE_SERVER_TRY_AGAIN_LATER"));
			break;
		default:
			snprintf(str, sizeof(str), "%s (%d)", email_get_email_string("IDS_EMAIL_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED"), type);
			ret = g_strdup(str);
			break;
	}
	debug_leave();
	return ret;
}

int composer_util_get_total_attachments_size(EmailComposerUGD *ugd, Eina_Bool with_inline_contents)
{
	retvm_if(!ugd, 0, "ugd is NULL!");

	int attach_size = 0;
	int inline_size = 0;

	struct stat file_info;
	Eina_List *l = NULL;
	email_attachment_data_t *att_data;

	ComposerAttachmentItemData *attachment_item_data = NULL;
	EINA_LIST_FOREACH(ugd->attachment_item_list, l, attachment_item_data) {
		email_attachment_data_t *att = attachment_item_data->attachment_data;
		if (att) {
			if (att->attachment_path && (stat(att->attachment_path, &file_info) != -1)) {
				/*debug_secure("size(%s): %d", att->attachment_path, file_info.st_size);*/
				attach_size += file_info.st_size;
			} else {
				debug_secure("stat(%s) failed! (%d): %s", att->attachment_path, errno, strerror(errno));
				attach_size += att->attachment_size;
			}
		}
	}

	if (with_inline_contents) {
		EINA_LIST_FOREACH(ugd->attachment_inline_item_list, l, att_data) {
			if (att_data) {
				if (att_data->attachment_path && (stat(att_data->attachment_path, &file_info) != -1)) {
					/*debug_secure("size(%s): %d", att_data->attachment_path, file_info.st_size);*/
					inline_size += file_info.st_size;
				} else {
					debug_secure("stat(%s) failed! (%d): %s", att_data->attachment_path, errno, strerror(errno));
				}
			}
		}
	}

	/*debug_log("Attachments size:(%d), Inline size:(%d)", attach_size, inline_size);*/
	return (attach_size + inline_size);
}

int composer_util_get_body_size(EmailComposerMail *mailinfo)
{
	retvm_if(!mailinfo, 0, "mailinfo is NULL!");

	int total_size = 0;
	if (mailinfo->mail_data) {
		total_size = email_get_file_size(mailinfo->mail_data->file_path_html);
		total_size += email_get_file_size(mailinfo->mail_data->file_path_plain);
		debug_log("total body size is %d", total_size);
	}

	return total_size;
}

Eina_Bool composer_util_is_max_sending_size_exceeded(void *data)
{
	debug_enter();

	retvm_if(!data, EINA_TRUE, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	Eina_Bool ret = EINA_FALSE;

	int mail_size = composer_util_get_total_attachments_size(ugd, EINA_TRUE); /* + composer_util_get_body_size(ugd->new_mail_info); */
	if (mail_size > ugd->account_info->max_sending_size) {
		ret = EINA_TRUE;
	}
	debug_log("Total size:(%d Byte), max:(%d Byte)", mail_size, ugd->account_info->max_sending_size);

	debug_leave();
	return ret;
}

void composer_util_modify_send_button(EmailComposerUGD *ugd)
{
	debug_enter();

	retm_if(!ugd, "Invalid parameter: ugd is NULL!");

	if (ugd->to_recipients_cnt == 0 && ugd->cc_recipients_cnt == 0 && ugd->bcc_recipients_cnt == 0) {
		elm_object_disabled_set(ugd->send_btn, EINA_TRUE);
	} else {
		if (composer_util_is_max_sending_size_exceeded(ugd)) {
			elm_object_disabled_set(ugd->send_btn, EINA_TRUE);
		} else {
			elm_object_disabled_set(ugd->send_btn, EINA_FALSE);
		}
	}

	debug_leave();
}

void composer_util_get_image_list_cb(Evas_Object *o, const char *result, void *data)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	EmailComposerAccount *account_info = ugd->account_info;
	retm_if(!account_info, "account_info is NULL!");

	int i = 0;
	Eina_List *l = NULL;
	email_attachment_data_t *att_data = NULL;

	EINA_LIST_FOREACH(ugd->attachment_inline_item_list, l, att_data) {
		if (att_data) {
			email_free_attachment_data(&att_data, 1);
		}
	}
	eina_list_free(ugd->attachment_inline_item_list);
	ugd->attachment_inline_item_list = NULL;

	if (result) {
		debug_secure("inline attachment list from webkit: [%s]", result);

		gchar **uris = g_strsplit(result, ";", -1);
		for (i = 0; i < g_strv_length(uris); i++) {
			if (g_str_has_prefix(uris[i], "file://") || g_str_has_prefix(uris[i], "/")) {
				email_attachment_data_t *new_att = (email_attachment_data_t *)calloc(1, sizeof(email_attachment_data_t));
				if (new_att) {
					new_att->attachment_id = 0;
					if (g_str_has_prefix(uris[i], "file://")) {
						new_att->attachment_path = g_uri_unescape_string(uris[i]+7, NULL);
					} else {
						new_att->attachment_path = g_uri_unescape_string(uris[i], NULL);
					}

					new_att->attachment_name = COMPOSER_STRDUP(email_file_file_get(new_att->attachment_path));
					new_att->mail_id = 0;
					new_att->account_id = account_info->selected_account->account_id;
					new_att->mailbox_id = 0;
					new_att->inline_content_status = 1;
					new_att->attachment_mime_type = email_get_mime_type_from_file(new_att->attachment_path);

					struct stat file_info;
					if (stat(new_att->attachment_path, &file_info) == -1) {
						debug_error("stat() failed! (%d): %s", errno, strerror(errno));
						new_att->attachment_size = 0;
						new_att->save_status = 0;
					} else {
						new_att->attachment_size = file_info.st_size;
						new_att->save_status = 1;
					}
					ugd->attachment_inline_item_list = eina_list_append(ugd->attachment_inline_item_list, new_att);
				}
			}
		}
		g_strfreev(uris);

		/* For logging */
		/*debug_log("total inline count:[%d]", eina_list_count(ugd->attachment_inline_item_list));
		EINA_LIST_FOREACH(ugd->attachment_inline_item_list, l, att_data) {
			debug_secure("Name:[%s], path:[%s], size:[%d], save[%d]", att_data->attachment_name, att_data->attachment_path, att_data->attachment_size, att_data->save_status);
		}*/
	}

	debug_leave();
}


Evas_Object *composer_util_create_layout_with_noindicator(Evas_Object *parent)
{
	debug_enter();

	retvm_if(!parent, NULL, "Invalid parameter: parent is NULL!");

	Evas_Object *layout = elm_layout_add(parent);
	retvm_if(!layout, NULL, "elm_layout_add() failed!");

	elm_layout_theme_set(layout, "layout", "application", "noindicator");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(layout);

	debug_leave();
	return layout;
}

Evas_Object *composer_util_create_scroller(Evas_Object *parent)
{
	debug_enter();

	email_profiling_begin(composer_util_create_scroller);

	retvm_if(!parent, NULL, "Invalid parameter: parent is NULL!");

	Evas_Object *scroller = elm_scroller_add(parent);
	retvm_if(!scroller, NULL, "elm_scroller_add() failed!");

	elm_scroller_bounce_set(scroller, EINA_FALSE, EINA_FALSE);
	elm_scroller_policy_set(scroller, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
	elm_object_focus_allow_set(scroller, EINA_FALSE);
	elm_object_scroll_freeze_push(scroller);
	evas_object_show(scroller);

	email_profiling_end(composer_util_create_scroller);

	debug_leave();
	return scroller;
}

Evas_Object *composer_util_create_box(Evas_Object *parent)
{
	debug_enter();

	retvm_if(!parent, NULL, "Invalid parameter: parent is NULL!");

	Evas_Object *box = elm_box_add(parent);
	retvm_if(!box, NULL, "elm_box_add() failed!");

	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, 0.0);
	evas_object_show(box);

	debug_leave();
	return box;
}

Evas_Object *composer_util_create_entry_layout(Evas_Object *parent)
{
	debug_enter();

	retvm_if(!parent, NULL, "Invalid parameter: parent is NULL!");

	Evas_Object *entry_layout = elm_layout_add(parent);
	retvm_if(!entry_layout, NULL, "elm_layout_add() failed!");

	elm_layout_file_set(entry_layout, email_get_composer_theme_path(), "ec/recipient/entry_layout");
	evas_object_show(entry_layout);
	evas_object_size_hint_weight_set(entry_layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(entry_layout, EVAS_HINT_FILL, EVAS_HINT_FILL);

	return entry_layout;
}

Eina_Bool composer_util_file_copy_temp_file(const char *src_file_path, char *dst_file_path, int size_dst_file_path)
{
	debug_enter();

	if (!composer_util_file_get_temp_filename(src_file_path, dst_file_path, size_dst_file_path, NULL)) {
		debug_error("composer_get_temp_filename() failed!");
		return EINA_FALSE;
	}

	if (!email_file_cp(src_file_path, dst_file_path)) {
		debug_error("email_file_cp() failed!");
		memset(dst_file_path, 0x0, size_dst_file_path);
		return EINA_FALSE;
	}

	debug_leave();
	return EINA_TRUE;
}

static void _composer_util_file_get_filename_and_ext_from_path(const char *src_file_path, char **filename, char **file_ext)
{
	debug_enter();

	retm_if(!src_file_path, "src_file_path is NULL!");

	char *name = NULL, *ext = NULL;

	const char *file = email_file_file_get(src_file_path);
	retm_if(!file, "email_file_file_get() failed!");

	debug_secure("file: (%s)", file);

	name = email_file_strip_ext(file);
	if (g_strcmp0(file, name) != 0) {
		char *p = strrchr(file, '.');
		if (p) {
			ext = g_strdup(p);
		}
	}

	*filename = name;
	*file_ext = ext;

	debug_leave();
}

Eina_Bool composer_util_file_get_temp_filename(const char *src_file_path, char *dst_file_path, int size_dst_file_path, const char *suffix)
{
	debug_enter();

	retvm_if(!src_file_path, EINA_FALSE, "src_file_path is NULL!");
	retvm_if(!dst_file_path, EINA_FALSE, "dst_file_path is NULL!");

	char temp_file_path[EMAIL_FILEPATH_MAX] = {'\0'};

	debug_secure("src_file_path: (%s)", src_file_path);

	char *name = NULL, *ext = NULL;
	_composer_util_file_get_filename_and_ext_from_path(src_file_path, &name, &ext);

	debug_secure("name: (%s), ext:(%s)", name, ext);

	snprintf(temp_file_path, EMAIL_FILEPATH_MAX, "%s/%s%s%s", composer_util_file_get_temp_dirname(), name, suffix ? suffix : "", ext ? ext : "");
	debug_secure("file path: (%s)", temp_file_path);

	int i = 0;
	while (email_file_exists(temp_file_path)) {
		bzero(temp_file_path, EMAIL_FILEPATH_MAX);
		snprintf(temp_file_path, EMAIL_FILEPATH_MAX, "%s/%s%s_%d%s", composer_util_file_get_temp_dirname(), name, suffix ? suffix : "", i++, ext ? ext : "");
		debug_secure("file path: (%s)", temp_file_path);
	}

	if (size_dst_file_path < strlen(temp_file_path)) {
		debug_error("Buffer size(%d) is less than the size of dest file(%d)", size_dst_file_path, strlen(temp_file_path));
		free(name);
		return EINA_FALSE;
	}

	snprintf(dst_file_path, size_dst_file_path, "%s", temp_file_path);
	free(name);

	debug_leave();

	return EINA_TRUE;
}

static const char *_composer_util_file_get_unique_dirname(const char *root_dir, char *ec_dirname)
{
	debug_enter();

	debug_secure("ec_dirname: (%s), (%p)", ec_dirname, ec_dirname);

	if (!ec_dirname[0]) {
		pid_t pid = getpid();
		char random[RANDOM_BUFF_SIZE] = {'\0'};
		_util_generate_random_string32(random, RANDOM_BUFF_SIZE);
		snprintf(ec_dirname, BUF_LEN_L, "%s/%d_%s", root_dir, pid, random);
	}

	debug_leave();
	return ec_dirname;
}

const char *composer_util_file_get_temp_dirname()
{
	debug_enter();
	static char ec_dirname[BUF_LEN_L] = {'\0'};
	return _composer_util_file_get_unique_dirname(email_get_composer_tmp_dir(), ec_dirname);
}

const char *composer_util_file_get_public_temp_dirname()
{
	debug_enter();
	static char ec_dirname[BUF_LEN_L] = {'\0'};
	return _composer_util_file_get_unique_dirname(email_get_composer_public_tmp_dir(), ec_dirname);
}

Eina_List *composer_util_make_array_to_list(const char **path_array, int path_len)
{
	debug_enter();

	retvm_if(!path_array, NULL, "path_array is NULL!");
	retvm_if(path_len <= 0, NULL, "Invalid parameter: len is [%d]!", path_len);

	Eina_List *list = NULL;

	int i;
	for (i = 0; i < path_len; i++) {
		char *p = (char *)path_array[i];
		if (p && (strlen(p) > 0)) {
			list = eina_list_append(list, g_strdup(p));
		}
	}

	debug_leave();
	return list;
}

Eina_List *composer_util_make_string_to_list(const char *path_string, const char *delim)
{
	debug_enter();

	retvm_if(!path_string, NULL, "path_string is NULL!");
	retvm_if(!delim, NULL, "delim is NULL!");

	Eina_List *list = NULL;
	int count = 0;
	gchar **split_path = g_strsplit_set(path_string, delim, -1);

	for (count = 0; count < g_strv_length(split_path); count++) {
		char *p = split_path[count];
		if (p && (strlen(p) > 0)) {
			list = eina_list_append(list, g_strdup(p));
		}
	}

	g_strfreev(split_path);

	debug_leave();
	return list;
}

static Eina_Bool __composer_util_is_mbe_modified(Evas_Object *mbe, Eina_List **initial_recipients_list)
{
	debug_enter();

	retvm_if(!mbe, EINA_FALSE, "Invalid parameter: mbe is NULL!");
	retvm_if(!initial_recipients_list, EINA_FALSE, "Invalid initial_recipients_list: data is NULL!");

	const Eina_List *current_list = NULL;
	Eina_List *mbe_initial_list = *initial_recipients_list;
	Eina_Bool is_modified = EINA_FALSE;

	current_list = elm_multibuttonentry_items_get(mbe);

	if ((!mbe_initial_list && current_list) || (mbe_initial_list && !current_list)) {
		is_modified = EINA_TRUE;
	} else if (mbe_initial_list && current_list) {
		if (current_list->accounting->count != mbe_initial_list->accounting->count) {
			is_modified = EINA_TRUE;
		} else {
			Eina_List *l1 = mbe_initial_list;
			const Eina_List *l2 = current_list;
			int index;

			for (index = 0; index < current_list->accounting->count; index++) {
				if (eina_list_data_get(l1) != eina_list_data_get(l2)) {
					is_modified = EINA_TRUE;
					break;
				}
				l1 = eina_list_next(l1);
				l2 = eina_list_next(l2);
			}
		}
	}

	debug_leave();
	return is_modified;
}

void composer_util_focus_set_focus(void *data, Evas_Object *target)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	/*debug_log("Flag values are %d,%d,%d,%d,%d",ugd->is_launching_ug, ugd->is_hided, ugd->is_back_btn_clicked, ugd->is_save_in_drafts_clicked, ugd->is_send_btn_clicked);*/
	if (ugd->is_hided) {
		ugd->need_to_set_focus_on_resume = EINA_TRUE;
	}

	/* If ewk_view in focus some elm object may be in invalid focus state.
	 * So unfocus any elm object to validate focus state.*/
	if (evas_object_focus_get(ugd->ewk_view)) {
		evas_object_focus_set(ugd->ewk_view, EINA_FALSE);
		Evas_Object *elm_object_in_focus = elm_object_focused_object_get(ugd->base.content);
		if (elm_object_in_focus) {
			elm_object_focus_set(elm_object_in_focus, EINA_FALSE);
		}
	}

	retm_if(ugd->base.module->is_launcher_busy || ugd->is_hided || ugd->composer_popup || ugd->context_popup, "should not set focus");
	retm_if(ugd->is_back_btn_clicked || ugd->is_save_in_drafts_clicked || ugd->is_send_btn_clicked, "while exiting composer");

	/* XXX; check this. when is this routine needed? */
	if (ugd->recp_from_ctxpopup) {
		evas_object_del(ugd->recp_from_ctxpopup);
		ugd->recp_from_ctxpopup = NULL;
		composer_recipient_from_ctxpopup_item_delete(ugd);
	}

	if (!target) {
		debug_log("Focus set [None]");
	} else if (target == ugd->recp_to_entry.entry) {
		debug_log("Focus set [to entry]");
		elm_object_focus_set(ugd->recp_to_entry.entry, EINA_TRUE);
	} else if (target == ugd->recp_cc_entry.entry) {
		debug_log("Focus set [cc entry]");
		elm_object_focus_set(ugd->recp_cc_entry.entry, EINA_TRUE);
	} else if (target == ugd->recp_bcc_entry.entry) {
		debug_log("Focus set [bcc entry]");
		elm_object_focus_set(ugd->recp_bcc_entry.entry, EINA_TRUE);
	} else if (target == ugd->subject_entry.entry) {
		debug_log("Focus set [subject_entry]");
		elm_object_focus_set(ugd->subject_entry.entry, EINA_TRUE);
	} else if (target == ugd->ewk_view) {
		debug_log("Focus set [ewk_view]");
		elm_object_focus_set(ugd->ewk_btn, EINA_TRUE);
	} else {
		debug_log("Focus set [NO! subject_entry]");
		elm_object_focus_set(ugd->subject_entry.entry, EINA_TRUE);
	}
	debug_leave();
}

static Eina_Bool __composer_util_focus_set_focus_idler_cb(void *data)
{
	debug_enter();

	COMPOSER_GET_TIMER_DATA(tdata, ugd, data);
	Evas_Object *entry = (Evas_Object *)tdata->data;

	ugd->idler_set_focus = NULL;

#ifdef ATTACH_PANEL_FEATURE
	if (!entry && !ugd->base.module->is_attach_panel_launched && (ugd->selected_entry != ugd->ewk_view)) {
#else
	if (!entry && (ugd->selected_entry != ugd->ewk_view)) {
#endif
		entry = ugd->selected_entry;
	}

	composer_util_focus_set_focus(ugd, entry);

	FREE(tdata);

	debug_leave();
	return ECORE_CALLBACK_CANCEL;
}

void composer_util_focus_set_focus_with_idler(void *data, Evas_Object *target)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	EmailCommonTimerData *tdata = (EmailCommonTimerData *)calloc(1, sizeof(EmailCommonTimerData));
	retm_if(!tdata, "tdata is NULL");
	tdata->ug_data = (void *)ugd;
	tdata->data = (void *)target;

	DELETE_IDLER_OBJECT(ugd->idler_set_focus);
	ugd->idler_set_focus = ecore_idler_add(__composer_util_focus_set_focus_idler_cb, tdata);

	debug_leave();
}

static Evas_Coord _composer_util_get_selected_widget_position(EmailComposerUGD *ugd)
{
	debug_enter();

	if (ugd->recp_from_ctxpopup) {
		debug_log("selected_layout = from layout");
		return 0;
	}

	Evas_Coord size_to_be_scrolled = 0;

	Evas_Coord composer_layout_y = 0;
	Evas_Coord entry_layout_y = 0;

	evas_object_geometry_get(ugd->composer_layout, NULL, &composer_layout_y, NULL, NULL);

	if (ugd->selected_entry == ugd->recp_to_entry.entry) {
		evas_object_geometry_get(ugd->recp_to_layout, NULL, &entry_layout_y, NULL, NULL);
	} else if (ugd->selected_entry == ugd->recp_cc_entry.entry) {
		evas_object_geometry_get(ugd->recp_cc_layout, NULL, &entry_layout_y, NULL, NULL);
	} else if (ugd->selected_entry == ugd->recp_bcc_entry.entry) {
		evas_object_geometry_get(ugd->recp_bcc_layout, NULL, &entry_layout_y, NULL, NULL);
	} else if (ugd->selected_entry == ugd->subject_entry.entry) {
		evas_object_geometry_get(ugd->subject_layout, NULL, &entry_layout_y, NULL, NULL);
	} else {
		debug_log("No selected entry!");
		return -1;
	}

	size_to_be_scrolled = entry_layout_y - composer_layout_y;

	debug_leave();
	return size_to_be_scrolled;
}

void composer_util_scroll_region_show(void *data)
{
	debug_enter();
	EmailComposerUGD *ugd = data;

	Evas_Coord size_to_be_scrolled = _composer_util_get_selected_widget_position(ugd);

	if (size_to_be_scrolled >= 0) {
		composer_initial_view_cs_show(ugd, size_to_be_scrolled);
	}

	debug_leave();
}

Eina_Bool composer_util_scroll_region_show_idler(void *data)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	ugd->idler_regionshow = NULL;
	composer_util_scroll_region_show(ugd);

	debug_leave();
	return ECORE_CALLBACK_CANCEL;
}

Eina_Bool composer_util_scroll_region_show_timer(void *data)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	ugd->timer_regionshow = NULL;
	composer_util_scroll_region_show(ugd);

	/* To resize predictive search layout after rotating device.
	 * (P140630-03288) Steps.
	 *  1. Focus to CC or BCC
	 *  2. Enter a char to make predictive search list (list should have more than 5 recp.)
	 *  3. Rotate device landscape.
	 *  4. Rotate device portrait.
	 *  5. There's an empty space below the box. (It's because we moves the scroller to the focused entry.)
	 */
	if (ugd->ps_box) {
		composer_ps_change_layout_size(ugd);
	}

	debug_leave();
	return ECORE_CALLBACK_CANCEL;
}

void composer_util_scroll_region_bringin(void *data)
{
	debug_enter();
	EmailComposerUGD *ugd = data;

	Evas_Coord size_to_be_scrolled = _composer_util_get_selected_widget_position(ugd);

	if (size_to_be_scrolled >= 0) {
		composer_initial_view_cs_bring_in(ugd, size_to_be_scrolled);
	}

	debug_leave();
}

Eina_Bool composer_util_scroll_region_bringin_idler(void *data)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	ugd->idler_regionbringin = NULL;
	composer_util_scroll_region_bringin(ugd);

	debug_leave();
	return ECORE_CALLBACK_CANCEL;
}

Eina_Bool composer_util_scroll_region_bringin_timer(void *data)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	ugd->timer_regionbringin = NULL;
	composer_util_scroll_region_bringin(ugd);

	debug_leave();
	return ECORE_CALLBACK_CANCEL;
}

void composer_util_ug_destroy_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	/* Restore the indicator mode. */
	composer_util_indicator_restore(ugd);

	DELETE_EVAS_OBJECT(ugd->composer_popup);
	email_module_make_destroy_request(ugd->base.module);

	debug_leave();
}

Eina_Bool composer_util_is_mail_modified(void *data)
{
	debug_enter();

	retvm_if(!data, EINA_TRUE, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	Eina_Bool is_modified = EINA_TRUE;

	if ((ugd->composer_type == RUN_COMPOSER_EXTERNAL) ||
		(ugd->recp_to_mbe && __composer_util_is_mbe_modified(ugd->recp_to_mbe, &ugd->initial_contents_to_list)) ||
		(ugd->recp_cc_mbe && __composer_util_is_mbe_modified(ugd->recp_cc_mbe, &ugd->initial_contents_cc_list)) ||
		(ugd->recp_bcc_mbe && __composer_util_is_mbe_modified(ugd->recp_bcc_mbe, &ugd->initial_contents_bcc_list)) ||
		(g_strcmp0(elm_entry_entry_get(ugd->subject_entry.entry), ugd->initial_contents_subject) != 0)) {
		goto FINISH_OFF;
	}

	if ((ugd->initial_contents_attachment_list && !ugd->attachment_item_list) || (!ugd->initial_contents_attachment_list && ugd->attachment_item_list)) {
		goto FINISH_OFF;
	} else if (ugd->initial_contents_attachment_list && ugd->attachment_item_list) {
		int nInitialListCount = eina_list_count(ugd->initial_contents_attachment_list);
		int nAttachmentCount = eina_list_count(ugd->attachment_item_list);

		if (nInitialListCount != nAttachmentCount) {
			goto FINISH_OFF;
		} else {
			int i = 0;
			int nInitialListCount = eina_list_count(ugd->initial_contents_attachment_list);

			for (i = 0; i < nInitialListCount; i++) {
				if (eina_list_nth(ugd->initial_contents_attachment_list, i) != eina_list_nth(ugd->attachment_item_list, i)) {
					goto FINISH_OFF;
					break;
				}
			}
		}
	}

	if (ugd->with_original_message) {
		if (g_strcmp0(ugd->initial_parent_content, ugd->final_parent_content) != 0) {
			debug_log("final_parent_content is differ from initial_parent_content!");
			goto FINISH_OFF;
		}

		if (g_strcmp0(ugd->initial_new_message_content, ugd->final_new_message_content) != 0) {
			debug_log("final_new_message_content is differ from initial_new_message_content!");
			goto FINISH_OFF;
		}
	} else {
		if (g_strcmp0(ugd->initial_body_content, ugd->final_body_content) != 0) {
			debug_log("final_new_message_content is differ from initial_new_message_content!");
			goto FINISH_OFF;
		}
	}

	is_modified = EINA_FALSE;

FINISH_OFF:
	return is_modified;
}

static Eina_Bool _util_generate_random_string32(char *inString, int inSize)
{
	debug_enter();

	retvm_if(inString == NULL, EINA_FALSE, "Invalid parameter: inString[NULL]");
	retvm_if(inSize < RANDOM_BUFF_SIZE, EINA_FALSE, "Invalid parameter: inSize[%d]", inSize);

	int randomNumber[RANDIOM_NUM_SIZE] = {'\0'};
	struct timespec timeValue;
	if (clock_gettime(CLOCK_REALTIME, &timeValue) == -1) {
		return EINA_FALSE;
	}
	unsigned int curClock = timeValue.tv_nsec;
	unsigned int seed = 0;

	memset(inString, 0x0, inSize);
	int i = 0;
	for (; i < RANDIOM_NUM_SIZE; i++) {
		seed = rand_r(&curClock);
		randomNumber[i] = rand_r(&seed) * rand_r(&seed);
	}

	snprintf(inString, inSize, "%08x%08x%08x%08x", randomNumber[0], randomNumber[1], randomNumber[2], randomNumber[3]);

	debug_log("generated String: (%s)", inString);

	debug_leave();
	return EINA_TRUE;
}

void composer_util_resize_webview_height(EmailComposerUGD *ugd)
{
	debug_enter();

	int ewk_width = 0;
	int ewk_height = 0;
	int win_w = 0;
	int win_h = 0;

	elm_win_screen_size_get(ugd->base.module->win, NULL, NULL, &win_w, &win_h);

	if ((ugd->base.orientation == APP_DEVICE_ORIENTATION_0) ||
		(ugd->base.orientation == APP_DEVICE_ORIENTATION_180)) {
		ewk_width = win_w;
		ewk_height = win_h;
	} else {
		ewk_width = win_h;
		ewk_height = win_w;
	}
	debug_log("ewk_height: %d", ewk_height);

	evas_object_size_hint_max_set(ugd->ewk_view, ewk_width, ewk_height);
	evas_object_size_hint_min_set(ugd->ewk_view, ewk_width, ewk_height);

	debug_leave();
}

void composer_util_resize_min_height_for_new_message(EmailComposerUGD *ugd, int ime_height)
{
	debug_enter();

	if (ugd->with_original_message) {
		int min_size = 0;
		char buf[BUF_LEN_L] = {'\0'};

		Evas_Coord nWidth = 0, nHeight = 0;
		elm_win_screen_size_get(ugd->base.module->win, NULL, NULL, &nWidth, &nHeight);
		int rot = elm_win_rotation_get(ugd->base.module->win);
		double ewk_scale = ewk_view_scale_get(ugd->ewk_view);

		/*debug_log("==> window [w, h, rot] = [%d, %d, %d]", nWidth, nHeight, rot);*/
		if ((rot == 0) || (rot == 180)) {
			min_size = (double)(nHeight - COMPOSER_NAVI_HEIGHT - COMPOSER_MESSAGEBAR_HEIGHT - COMPOSER_DEFAULT_WEBVIEW_MARGIN - ime_height) / ewk_scale;
		} else {
			min_size = (double)(nWidth - COMPOSER_NAVI_LAND_HEIGHT - COMPOSER_MESSAGEBAR_HEIGHT - COMPOSER_DEFAULT_WEBVIEW_MARGIN - ime_height) / ewk_scale;
		}

		if (ugd->is_checkbox_clicked) {
			/* When original message area is visible, the area should have min height. */
			snprintf(buf, sizeof(buf), EC_JS_UPDATE_MIN_HEIGHT_OF_ORG_MESSAGE, min_size);
			if (!ewk_view_script_execute(ugd->ewk_view, buf, NULL, NULL)) {
				debug_error("EC_JS_UPDATE_MIN_HEIGHT_OF_ORG_MESSAGE failed!");
			}

			min_size = 0; /* No need to have min height for new message area in this case. */
		}

		/* Update min height for new message area */
		snprintf(buf, sizeof(buf), EC_JS_UPDATE_MIN_HEIGHT_OF_NEW_MESSAGE, min_size);
		if (!ewk_view_script_execute(ugd->ewk_view, buf, NULL, NULL)) {
			debug_error("EC_JS_UPDATE_MIN_HEIGHT_OF_NEW_MESSAGE failed!");
		}
	}

	debug_leave();
}

void composer_util_display_position(void *data)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	Evas_Coord x = 0, y = 0, w = 0, h = 0;

	evas_object_geometry_get(ugd->base.module->win, &x, &y, &w, &h);
	debug_log("window     = [x:%d, y:%d, w:%d, h:%d]", x, y, w, h);
	x = y = w = h = 0;

	evas_object_geometry_get(ugd->base.module->conform, &x, &y, &w, &h);
	debug_log("conformant = [x:%d, y:%d, w:%d, h:%d]", x, y, w, h);
	x = y = w = h = 0;

	evas_object_geometry_get(ugd->composer_layout, &x, &y, &w, &h);
	debug_log("composer_layout = [x:%d, y:%d, w:%d, h:%d]", x, y, w, h);
	x = y = w = h = 0;

	evas_object_geometry_get(ugd->base.module->navi, &x, &y, &w, &h);
	debug_log("naviframe = [x:%d, y:%d, w:%d, h:%d]", x, y, w, h);
	x = y = w = h = 0;

	elm_scroller_region_get(ugd->main_scroller, &x, &y, &w, &h);
	debug_log("scroller   = [x:%d, y:%d, w:%d, h:%d]", x, y, w, h);
	x = y = w = h = 0;

	if (ugd->recp_from_layout) {
		evas_object_geometry_get(ugd->recp_from_layout, &x, &y, &w, &h);
		debug_log("from       = [x:%d, y:%d, w:%d, h:%d]", x, y, w, h);
		x = y = w = h = 0;
	}

	evas_object_geometry_get(ugd->recp_to_layout, &x, &y, &w, &h);
	debug_log("to         = [x:%d, y:%d, w:%d, h:%d]", x, y, w, h);
	x = y = w = h = 0;

	if (ugd->recp_cc_layout) {
		evas_object_geometry_get(ugd->recp_cc_layout, &x, &y, &w, &h);
		debug_log("cc         = [x:%d, y:%d, w:%d, h:%d]", x, y, w, h);
		x = y = w = h = 0;
	}

	if (ugd->recp_bcc_layout) {
		evas_object_geometry_get(ugd->recp_bcc_layout, &x, &y, &w, &h);
		debug_log("bcc        = [x:%d, y:%d, w:%d, h:%d]", x, y, w, h);
		x = y = w = h = 0;
	}

	evas_object_geometry_get(ugd->subject_layout, &x, &y, &w, &h);
	debug_log("subject    = [x:%d, y:%d, w:%d, h:%d]", x, y, w, h);
	x = y = w = h = 0;

	evas_object_geometry_get(ugd->ewk_view, &x, &y, &w, &h);
	debug_log("webview    = [x:%d, y:%d, w:%d, h:%d]", x, y, w, h);
	x = y = w = h = 0;

	debug_leave();
}

void composer_util_return_composer_view(void *data)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	/* Reset exit flags. */
	ugd->is_send_btn_clicked = EINA_FALSE;
	ugd->is_back_btn_clicked = EINA_FALSE;
	ugd->is_save_in_drafts_clicked = EINA_FALSE;

	elm_object_tree_focus_allow_set(ugd->composer_layout, EINA_TRUE);
	elm_object_focus_allow_set(ugd->ewk_btn, EINA_TRUE);

	composer_util_focus_set_focus_with_idler(ugd, ugd->selected_entry);

	debug_leave();
}

void composer_util_indicator_show(void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	ugd->indicator_mode = elm_win_indicator_mode_get(ugd->base.module->win);
	ugd->opacity_mode = elm_win_indicator_opacity_get(ugd->base.module->win);

	debug_log("Indicator mode:[%d], Opacity mode:[%d]", ugd->indicator_mode, ugd->opacity_mode);

	elm_win_indicator_mode_set(ugd->base.module->win, ELM_WIN_INDICATOR_SHOW);
	elm_win_indicator_opacity_set(ugd->base.module->win, ELM_WIN_INDICATOR_OPAQUE);

	debug_leave();
}

void composer_util_indicator_restore(void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	elm_win_indicator_mode_set(ugd->base.module->win, ugd->indicator_mode);

	if (ugd->opacity_mode == ELM_WIN_INDICATOR_OPACITY_UNKNOWN) {
		ugd->opacity_mode = ELM_WIN_INDICATOR_OPAQUE;
	}
	elm_win_indicator_opacity_set(ugd->base.module->win, ugd->opacity_mode);

	debug_leave();
}

Eina_Bool composer_util_is_object_packed_in(Evas_Object *box, Evas_Object *obj)
{
	retvm_if(!box, EINA_FALSE, "Invalid parameter: box is NULL!");
	retvm_if(!obj, EINA_FALSE, "Invalid parameter: obj is NULL!");

	Eina_Bool ret = EINA_FALSE;
	Eina_List *children = elm_box_children_get(box);
	void *data = eina_list_data_find(children, obj);
	if (data) {
		ret = EINA_TRUE;
	}
	eina_list_free(children);

	return ret;
}

char *composer_util_strip_quotation_marks_in_email_address(const char *email_address)
{
	retvm_if(!email_address, NULL, "Invalid parameter: email_address is NULL!");

	/* Replace from email address with new one.
	 *  Case 1. "hello" <abc@def.com> ==> hello <abc@def.com>
	 *  Case 2. <abc@def.com>         ==> abc@def.com
	 */

	char buf[BUF_LEN_L] = { 0, };
	char *nick = NULL;
	char *addr = NULL;

	email_get_recipient_info(email_address, &nick, &addr);
	if (nick) {
		snprintf(buf, sizeof(buf), "%s <%s>", nick, addr);
	} else if (addr) {
		snprintf(buf, sizeof(buf), "%s", addr);
	}
	FREE(nick);
	FREE(addr);

	return elm_entry_utf8_to_markup(buf); /* markup string will be returnd. */
}

EmailExtSaveErrType _composer_util_save_attachment_for_preview(ComposerAttachmentItemData *attachment_item_data)
{
	debug_enter();
	retvm_if(attachment_item_data == NULL, EMAIL_EXT_SAVE_ERR_UNKNOWN, "Invalid parameter: aid[NULL]");
	EmailComposerUGD *ugd = (EmailComposerUGD *)attachment_item_data->ugd;
	email_attachment_data_t *attachment = attachment_item_data->attachment_data;

	EmailExtSaveErrType ret = email_prepare_temp_file_path(attachment->attachment_id, composer_util_file_get_temp_dirname(), attachment->attachment_path, &attachment_item_data->preview_path);
	if (ret == EMAIL_EXT_SAVE_ERR_ALREADY_EXIST) {
		return EMAIL_EXT_SAVE_ERR_NONE;
	}
	if (ret == EMAIL_EXT_SAVE_ERR_UNKNOWN || !(email_file_cp(attachment->attachment_path, attachment_item_data->preview_path))) {
		ugd->composer_popup = composer_util_popup_create(ugd, EMAIL_COMPOSER_STRING_UNABLE_TO_OPEN_FILE, EMAIL_COMPOSER_STRING_UNABLE_TO_DISPLAY_ATTACHMENT,
								composer_util_popup_response_cb, EMAIL_COMPOSER_STRING_BUTTON_OK, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);
		debug_error("save attachment failed error");
		return EMAIL_EXT_SAVE_ERR_UNKNOWN;
	}

	debug_leave();
	return EMAIL_EXT_SAVE_ERR_NONE;
}

void composer_util_show_preview(ComposerAttachmentItemData *attach_item_data)
{
	debug_enter();
	EmailComposerUGD *ugd = (EmailComposerUGD *)attach_item_data->ugd;
	email_attachment_data_t *attachment = attach_item_data->attachment_data;

	const char *path = attachment->attachment_path;

	if (!attach_item_data->from_user) {
		if (_composer_util_save_attachment_for_preview(attach_item_data) != EMAIL_EXT_SAVE_ERR_NONE) {
			debug_error("composer show preview - failed error ");
			return;
		}
		path = attach_item_data->preview_path;
	}

	composer_launcher_preview_attachment(ugd, path);

	debug_leave();
	return;
}

#ifdef ATTACH_PANEL_FEATURE
void composer_util_update_attach_panel_bundles(EmailComposerUGD *ugd)
{
	debug_enter();

	bundle *b = NULL;
	int r = 0;
	char buff[BUF_LEN_T] = { 0 };

	snprintf(buff, BUF_LEN_T, "%d", (int)ugd->account_info->max_sending_size);

	b = bundle_create();
	retm_if(!b, "bundle_create() failed!");

	r = bundle_add_str(b, EMAIL_BUNDLE_KEY_NAME_TOTAL_SIZE, buff);
	if (r == BUNDLE_ERROR_NONE) {
		r = email_module_set_attach_panel_category_bundle(ugd->base.module, EMAIL_APCT_VIDEO_RECORDER, b);
		if (r != 0) {
			debug_error("email_module_set_attach_panel_category_bundle() failed");
		}
	} else {
		debug_error("bundle_add() failed: %d", r);
	}

	bundle_free(b);

	debug_leave();
}
#endif

void composer_util_set_entry_text_style(Evas_Object *entry)
{
	debug_enter();
	char text_style[BUF_LEN_S] = { 0, };
	snprintf(text_style, sizeof(text_style), TEXT_STYLE_ENTRY_STRING, FONT_SIZE_ENTRY, COLOR_BLACK);
	elm_entry_text_style_user_push(entry, text_style);
	debug_leave();
}

void composer_util_network_state_noti_post()
{
	debug_enter();

	connection_h connection = NULL;
	connection_type_e network_state = CONNECTION_TYPE_DISCONNECTED;
	connection_cellular_state_e cellular_state = CONNECTION_CELLULAR_STATE_OUT_OF_SERVICE;

	int err = connection_create(&connection);
	retm_if(err != CONNECTION_ERROR_NONE, "connection_create() failed! err:[%d]", err);

	err = connection_get_type(connection, &network_state);
	debug_error_if(err != CONNECTION_ERROR_NONE, "connection_get_type() failed! err:[%d]", err);

	if (network_state == CONNECTION_TYPE_DISCONNECTED) {
		err = connection_get_cellular_state(connection, &cellular_state);
		debug_error_if(err != CONNECTION_ERROR_NONE, "connection_get_cellular_state() failed! err:[%d]", err);

		if (cellular_state == CONNECTION_CELLULAR_STATE_FLIGHT_MODE) {
			err = notification_status_message_post(email_get_email_string("IDS_EMAIL_POP_FLIGHT_MODE_ENABLED_THIS_MESSAGE_WILL_BE_SENT_WHEN_YOU_ARE_CONNECTED_TO_THE_NETWORK"));
		} else {
			err = notification_status_message_post(email_get_email_string("IDS_EMAIL_POP_NETWORK_NOT_AVAILABLE_THIS_MESSAGE_WILL_BE_SENT_WHEN_YOU_ARE_CONNECTED_TO_THE_NETWORK"));
		}

		debug_error_if(err != NOTIFICATION_ERROR_NONE, "notification_status_message_post() failed: %d", err);

	}

	err = connection_destroy(connection);
	debug_warning_if(err != CONNECTION_ERROR_NONE, "connection_destroy() failed! err:[%d]", err);
	debug_leave();
}

static bool _composer_is_valid_file_name_char(char c)
{
	switch (c) {
	case '\\':
	case '/':
	case ':':
	case '*':
	case '?':
	case '\"':
	case '<':
	case '>':
	case '|':
	case ';':
	case ' ':
		return false;
	default:
		return true;
	}
}

static char *_composer_make_vcard_file_path(const char* display_name)
{
	debug_log();
	retvm_if(!display_name, NULL, "display_name is NULL");

	char *display_name_copy = NULL;
	char *file_name = NULL;
	char file_path[PATH_MAX] = { 0 };

	display_name_copy = strdup(display_name);
	retvm_if(!display_name_copy, NULL, "display_name_copy is NULL");

	file_name = email_util_strtrim(display_name_copy);
	if (!STR_VALID(file_name)) {
		snprintf(file_path, sizeof(file_path), "%s/Unknown.vcf",
				composer_util_file_get_temp_dirname());

	} else {
		int i = 0;
		int length = 0;

		length = strlen(file_name);
		if (length > NAME_MAX - VCARD_NAME_SUFFIX_LEN) {
			debug_warning("Display name is too long. It will be cut.");
			length = NAME_MAX - VCARD_NAME_SUFFIX_LEN;
			file_name[length] = '\0';
		}

		for (i = 0; i < length; ++i) {
			if (!_composer_is_valid_file_name_char(file_name[i])) {
				file_name[i] = '_';
			}
		}

		snprintf(file_path, sizeof(file_path), "%s/%s.vcf",
				composer_util_file_get_temp_dirname(), file_name);
	}

	debug_log("file_path = %s", file_path);

	free(display_name_copy);

	return strdup(file_path);
}

static bool _composer_write_vcard_to_file(int fd, contacts_record_h record, Eina_Bool my_profile)
{
	debug_log();

	char *vcard_buff = NULL;
	bool ok = false;

	do {
		int size_left = 0;

		if (my_profile) {
			contacts_vcard_make_from_my_profile(record, &vcard_buff);
		} else {
			contacts_vcard_make_from_person(record, &vcard_buff);
		}

		if (!vcard_buff) {
			debug_error("vcard_buff is NULL");
			break;
		}

		size_left = strlen(vcard_buff);
		while (size_left) {
			int written = write(fd, vcard_buff, size_left);
			if (written == -1) {
				debug_error("write() failed: %s", strerror(errno));
				break;
			}
			size_left -= written;
		}

		ok = (size_left == 0);
	} while (false);

	free(vcard_buff);

	return ok;
}

static bool _composer_write_vcard_to_file_from_id(int fd, int person_id)
{
	debug_enter();

	contacts_record_h record = NULL;
	bool ok = false;

	do {
		int ret = contacts_db_get_record(_contacts_person._uri, person_id, &record);
		if (ret != CONTACTS_ERROR_NONE) {
			debug_error("contacts_db_get_record() failed: %d", ret);
			record = NULL;
			break;
		}

		if (!_composer_write_vcard_to_file(fd, record, false)) {
			debug_error("_composer_write_vcard_to_file() failed");
			break;
		}

		ok = true;
	} while (false);

	if (record) {
		contacts_record_destroy(record, true);
	}

	return ok;
}

char *composer_create_vcard_from_id(int id, Eina_Bool my_profile)
{
	debug_enter();

	contacts_record_h record = NULL;
	char *vcard_path = NULL;
	int fd = -1;
	bool ok = false;

	do {
		char *display_name = NULL;

		int ret = contacts_db_get_record((my_profile ?
				_contacts_my_profile._uri :
				_contacts_person._uri), id, &record);
		if (ret != CONTACTS_ERROR_NONE) {
			debug_error("contacts_db_get_record() failed: %d", ret);
			record = NULL;
			break;
		}

		if (my_profile) {
			contacts_record_get_str_p(record, _contacts_my_profile.display_name, &display_name);
		} else {
			contacts_record_get_str_p(record, _contacts_person.display_name, &display_name);
		}

		vcard_path = _composer_make_vcard_file_path(display_name);
		if (!vcard_path) {
			debug_error("_composer_make_vcard_file_path() failed");
			break;
		}

		fd = open(vcard_path, O_WRONLY | O_CREAT | O_TRUNC, FILE_ACCESS_MODE);
		if (fd == -1) {
			debug_error("open(%s) Failed(%s)", vcard_path, strerror(errno));
			break;
		}

		if (!_composer_write_vcard_to_file(fd, record, my_profile)) {
			debug_error("_composer_write_vcard_to_file() failed");
			break;
		}

		ok = true;
	} while (false);

	if (record) {
		contacts_record_destroy(record, true);
	}

	if (fd != -1) {
		close(fd);
		if (!ok) {
			email_file_remove(vcard_path);
		}
	}

	if (!ok) {
		FREE(vcard_path);
	}

	debug_leave();
	return vcard_path;
}

char *composer_create_vcard_from_id_list(const int *id_list, int count, volatile Eina_Bool *cancel)
{
	debug_enter();
	retvm_if(!id_list, NULL, "id_list is NULL");
	retvm_if(count <= 0, NULL, "count <= 0");

	char file_path[PATH_MAX] = { 0 };
	char *vcard_path = NULL;
	int fd = -1;
	bool ok = false;

	if (cancel && *cancel) {
		return NULL;
	}

	do {
		int i = 0;

		snprintf(file_path, sizeof(file_path), "%s/Contacts.vcf",
				composer_util_file_get_temp_dirname());

		vcard_path = strdup(file_path);
		if (!vcard_path) {
			debug_error("strdup() failed");
			break;
		}

		fd = open(vcard_path, O_WRONLY | O_CREAT | O_TRUNC, FILE_ACCESS_MODE);
		if (fd == -1) {
			debug_error("open(%s) Failed(%s)", vcard_path, strerror(errno));
			break;
		}

		for (i = 0; (!cancel || !(*cancel)) && (i < count); ++i) {
			if (!_composer_write_vcard_to_file_from_id(fd, id_list[i])) {
				debug_error("_composer_write_vcard_to_file() failed");
				break;
			}
		}

		ok = (i == count);
	} while (false);

	if (fd != -1) {
		close(fd);
		if (!ok) {
			email_file_remove(vcard_path);
		}
	}

	if (!ok) {
		FREE(vcard_path);
	}

	debug_leave();
	return vcard_path;
}

void composer_create_vcard_create_popup(EmailComposerUGD *ugd)
{
	DELETE_EVAS_OBJECT(ugd->composer_popup);
	ugd->composer_popup = composer_util_popup_create_with_progress_horizontal(ugd,
			EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_CREATING_VCARD, _composer_vcard_save_popup_cancel_cb,
			EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);
}

static void _composer_vcard_save_popup_cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "Invalid parameter: data is NULL!");
	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	email_module_make_destroy_request(ugd->base.module);
}
