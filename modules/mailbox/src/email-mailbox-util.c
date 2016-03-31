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
#include <utils_i18n_ulocale.h>

#include "email-mailbox.h"
#include "email-mailbox-util.h"
#include "email-mailbox-sync.h"

/*
 * Declaration for static functions
 */

static void _mailbox_password_popup_ok_cb(void *data, Evas_Object *obj, void *event_info);
static void _mailbox_password_popup_canceled_cb(void *data, Evas_Object *obj, void *event_info);
static int _is_today(const time_t req_time);
static int _is_yesterday(const time_t req_time);

/*
 * Definitions
 */

EMAIL_DEFINE_GET_EDJ_PATH(email_get_mailbox_theme_path, "/email-mailbox.edj")

/*
 * Structures
 */

typedef struct _EmailMailboxIdlerData {
	void *view;
	void *data;
} EmailMailboxIdlerData;

typedef struct _MailboxAccountColor {
	int account_id;
	int account_color;
} MailboxAccountColor;


/*
 * Globals
 */

/*
 * Definition for static functions
 */

static void _timeout_popup_destroy_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	DELETE_EVAS_OBJECT(obj);
}

static void _mailbox_password_popup_ok_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailMailboxView *view = (EmailMailboxView *)data;

	mailbox_sync_folder_with_new_password((int)(ptrdiff_t)event_info, view->sync_needed_mailbox_id, data);
	view->sync_needed_mailbox_id = -1;

	DELETE_EVAS_OBJECT(view->passwd_popup);
}

static void _mailbox_password_popup_canceled_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailMailboxView *view = (EmailMailboxView *)data;
	DELETE_EVAS_OBJECT(view->passwd_popup);
	view->sync_needed_mailbox_id = -1;
}



static void _entry_maxlength_reached_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	if (!data || !obj) {
		debug_warning("data is NULL");
		return;
	}

	/* display warning popup */
	debug_log("entry length is max now");
	int ret = notification_status_message_post(email_get_email_string("IDS_EMAIL_TPOP_MAXIMUM_NUMBER_OF_CHARACTERS_REACHED"));
	if (ret != NOTIFICATION_ERROR_NONE) {
		debug_log("fail to notification_status_message_post() : %d\n", ret);
	}
}

static int _is_today(const time_t req_time)
{
	time_t now_time = time(NULL);

	struct tm tm_buff = { 0, };

	struct tm *dummy = localtime_r(&now_time, &tm_buff);
	retvm_if(!dummy, 0, "localtime_r() - failed");
	struct tm now_tm;
	memcpy(&now_tm, dummy, sizeof(struct tm));

	dummy = localtime_r(&req_time, &tm_buff);
	retvm_if(!dummy, 0, "localtime_r() - failed");
	struct tm req_tm;
	memcpy(&req_tm, dummy, sizeof(struct tm));

	if (req_tm.tm_year == now_tm.tm_year && req_tm.tm_yday == now_tm.tm_yday)
		return 1;
	else
		return 0;
}

static int _is_yesterday(const time_t req_time)
{
	time_t now_time = time(NULL);

	struct tm tm_buff = { 0, };

	struct tm *dummy = localtime_r(&now_time, &tm_buff);
	retvm_if(!dummy, 0, "localtime_r() - failed");
	struct tm now_tm;
	memcpy(&now_tm, dummy, sizeof(struct tm));

	dummy = localtime_r(&req_time, &tm_buff);
	retvm_if(!dummy, 0, "localtime_r() - failed");
	struct tm req_tm;
	memcpy(&req_tm, dummy, sizeof(struct tm));

	if (now_tm.tm_yday == 0) { /* It is the first day of year */
		if (req_tm.tm_year == now_tm.tm_year - 1 && req_tm.tm_mon == 12 && req_tm.tm_mday == 31)
			return 1;
		else
			return 0;
	} else {
		if (req_tm.tm_year == now_tm.tm_year && req_tm.tm_yday == now_tm.tm_yday - 1)
			return 1;
		else
			return 0;
	}
}

/*
 * Definition for exported functions
 */

void mailbox_clear_prev_mailbox_info(EmailMailboxView *view)
{
	debug_enter();

	G_FREE(view->account_name);
	G_FREE(view->mailbox_alias);
	view->account_id = 0;
	view->mode = EMAIL_MAILBOX_MODE_UNKOWN;
	view->mailbox_type = EMAIL_MAILBOX_TYPE_NONE;
	view->mailbox_id = 0;
}

void mailbox_set_last_updated_time(time_t last_update_time, EmailMailboxView *view)
{
	debug_enter();

	char updatetime[LAST_UPDATED_TIME] = { 0, };
	const char *icu_locale = NULL;
	char *formatted_text = NULL;
	char *skeleton = NULL;

	int ret = i18n_ulocale_get_default(&icu_locale);
	retm_if(ret != I18N_ERROR_NONE, "i18n_ulocale_get_default() failed! ret:[%d]");

	if (_is_today(last_update_time)) {
		skeleton = (!view->b_format_24hour) ?
				EMAIL_TIME_FORMAT_12 : EMAIL_TIME_FORMAT_24;
		formatted_text = email_get_date_text(icu_locale, skeleton, &last_update_time);
		snprintf(updatetime, sizeof(updatetime), "%s : %s %s", _("IDS_EMAIL_BODY_LAST_UPDATED"), _("IDS_EMAIL_BODY_TODAY"), formatted_text);
	} else if (_is_yesterday(last_update_time)) {
		skeleton = (!view->b_format_24hour) ?
				EMAIL_TIME_FORMAT_12 : EMAIL_TIME_FORMAT_24;
		formatted_text = email_get_date_text(icu_locale, skeleton, &last_update_time);
		snprintf(updatetime, sizeof(updatetime), "%s : %s %s", _("IDS_EMAIL_BODY_LAST_UPDATED"), _("IDS_EMAIL_BODY_YESTERDAY"), formatted_text);
	} else {
		skeleton = (!view->b_format_24hour) ?
			EMAIL_DATETIME_FORMAT_12 : EMAIL_DATETIME_FORMAT_24;
		formatted_text = email_get_date_text(icu_locale, skeleton, &last_update_time);
		snprintf(updatetime, sizeof(updatetime), "%s : %s", _("IDS_EMAIL_BODY_LAST_UPDATED"), formatted_text);
	}

	debug_secure("date&time: %d [%s]", last_update_time, updatetime);

	FREE(formatted_text);
	FREE(view->last_updated_time);
	view->last_updated_time = calloc(1, LAST_UPDATED_TIME);
	retm_if(!view->last_updated_time, "view->last_updated_time is NULL!");
	memcpy(view->last_updated_time, updatetime, LAST_UPDATED_TIME);

}

int mailbox_compare_mailid_in_list(gconstpointer a, gconstpointer b)
{
	MailItemData *ld = (MailItemData *)a;
	if (ld->mail_id == GPOINTER_TO_INT(b))
		return 0;
	else
		return 1;
}

Evas_Object *mailbox_create_timeout_popup(EmailMailboxView *view, const char *title_text, const char *contents_text, double timeout)
{
	debug_enter();
	retvm_if(!view, NULL, "view is NULL");
	retvm_if(!contents_text, NULL, "contents_text is NULL");

	Evas_Object *popup = elm_popup_add(view->base.module->navi);

	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_popup_align_set(popup, ELM_NOTIFY_ALIGN_FILL, 1.0);

	if (title_text) {
		elm_object_part_text_set(popup, "title,text", title_text);
	}
	elm_object_text_set(popup, contents_text);

	if (timeout > 0.0) {
		elm_popup_timeout_set(popup, timeout);
		evas_object_smart_callback_add(popup, "timeout", _timeout_popup_destroy_cb, NULL);
		evas_object_smart_callback_add(popup, "block,clicked", _timeout_popup_destroy_cb, NULL);
	}
	evas_object_show(popup);

	return popup;
}

Evas_Object *mailbox_create_popup(EmailMailboxView *view, const char *title, const char *content, Evas_Smart_Cb back_button_cb,
		Evas_Smart_Cb btn1_response_cb, const char *btn1_text, Evas_Smart_Cb btn2_response_cb, const char *btn2_text)
{
	debug_enter();
	retvm_if(!view, NULL, "view is NULL");
	retvm_if(!content, NULL, "content is NULL");
	retvm_if(!back_button_cb, NULL, "response_cb is NULL");
	retvm_if(!btn1_response_cb || !btn1_text, NULL, "btn1_response_cb or btn1_text is NULL");

	Evas_Object *popup = elm_popup_add(view->base.module->navi);
	eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, back_button_cb, view);
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_popup_align_set(popup, ELM_NOTIFY_ALIGN_FILL, 1.0);

	if (title) {
		elm_object_domain_translatable_part_text_set(popup, "title,text", PACKAGE, title);
	}
	if (content) {
		elm_object_domain_translatable_text_set(popup, PACKAGE, content);
	}

	if (btn2_text) {
		Evas_Object *btn1 = elm_button_add(popup);
		elm_object_style_set(btn1, "popup");
		elm_object_domain_translatable_text_set(btn1, PACKAGE, btn1_text);
		elm_object_part_content_set(popup, "button1", btn1);
		evas_object_smart_callback_add(btn1, "clicked", btn1_response_cb, view);
		elm_object_focus_set(btn1, EINA_TRUE);

		Evas_Object *btn2 = elm_button_add(popup);
		elm_object_style_set(btn2, "popup");
		elm_object_domain_translatable_text_set(btn2, PACKAGE, btn2_text);
		elm_object_part_content_set(popup, "button2", btn2);
		evas_object_smart_callback_add(btn2, "clicked", btn2_response_cb, view);

	} else {
		Evas_Object *btn1 = elm_button_add(popup);
		elm_object_style_set(btn1, "popup");
		elm_object_domain_translatable_text_set(btn1, PACKAGE, btn1_text);
		elm_object_part_content_set(popup, "button1", btn1);
		evas_object_smart_callback_add(btn1, "clicked", btn1_response_cb, view);
		elm_object_focus_set(btn1, EINA_TRUE);
	}

	evas_object_show(popup);

	return popup;
}

void mailbox_create_error_popup(int error_type, int account_id, EmailMailboxView *view)
{
	debug_enter();

	char *popup_text = NULL;
	bool need_free = false;

	switch (error_type) {
	case EMAIL_ERROR_MAXIMUM_DEVICES_LIMIT_REACHED:
		popup_text = _("IDS_EMAIL_TPOP_MAXIMUM_NUMBER_OF_REGISTERED_DEVICES_REACHED_FOR_THIS_ACCOUNT_DEREGISTER_SOME_DEVICES_AND_TRY_AGAIN");
		break;
	case EMAIL_ERROR_ACCOUNT_IS_BLOCKED:
		popup_text = _("IDS_EMAIL_POP_UNABLE_TO_CONNECT_TO_SERVER");
		break;
	case EMAIL_ERROR_NETWORK_NOT_AVAILABLE:
	case EMAIL_ERROR_CONNECTION_FAILURE:
		popup_text = _("IDS_EMAIL_TPOP_FAILED_TO_CONNECT_TO_NETWORK");
		break;
	case EMAIL_ERROR_MAIL_MEMORY_FULL:
		popup_text = _("IDS_EMAIL_POP_THERE_IS_NOT_ENOUGH_SPACE_IN_YOUR_DEVICE_STORAGE");
		break;
	case EMAIL_ERROR_AUTHENTICATE:
		popup_text = _("IDS_EMAIL_POP_AUTHENTICATION_FAILED");
		break;
	case EMAIL_ERROR_LOGIN_ALLOWED_EVERY_15_MINS:
		popup_text = _("IDS_EMAIL_POP_YOU_CAN_ONLY_LOG_IN_ONCE_EVERY_15_MINUTES");
		break;
	case EMAIL_ERROR_NO_RESPONSE:
		popup_text = _("IDS_EMAIL_POP_NO_RESPONSE_HAS_BEEN_RECEIVED_FROM_THE_SERVER_TRY_AGAIN_LATER");
		break;
	case EMAIL_ERROR_LOGIN_FAILURE:
		popup_text = email_util_get_login_failure_string(account_id);
		need_free = true;
		break;
	default:
		debug_log("Can't find error string. error_type : %d", error_type);
		break;
	}

	if (popup_text) {
		int ret = notification_status_message_post(popup_text);
		if (ret != NOTIFICATION_ERROR_NONE) {
			debug_log("notification_status_message_post() failed: %d", ret);
		}
		debug_log("error_type : %d", error_type);
		if (need_free) free(popup_text);
	}
}

void mailbox_create_password_changed_popup(void *data, int account_id)
{
	debug_enter();

	EmailMailboxView *view = (EmailMailboxView *)data;

	DELETE_EVAS_OBJECT(view->passwd_popup);
	view->passwd_popup = email_util_create_password_changed_popup(
			view->base.module->navi, account_id, _mailbox_password_popup_ok_cb,
			_mailbox_password_popup_canceled_cb, data);
}

void mailbox_check_sort_type_validation(EmailMailboxView *view)
{
	debug_enter();

	if (view->sort_type == EMAIL_SORT_IMPORTANT) {
		if (!(view->mode == EMAIL_MAILBOX_MODE_MAILBOX &&
			view->mailbox_type != EMAIL_MAILBOX_TYPE_OUTBOX && view->mailbox_type != EMAIL_MAILBOX_TYPE_DRAFT)) {
			debug_log("Account options. sort_type is changed into EMAIL_SORT_DATE_RECENT");
			view->sort_type_index = SORTBY_DATE_RECENT;
			view->sort_type = EMAIL_SORT_DATE_RECENT;
		}
	}

	if (view->sort_type == EMAIL_SORT_TO_CC_BCC) {
		if (view->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX ||
			view->mailbox_type == EMAIL_MAILBOX_TYPE_DRAFT ||
			view->mailbox_type == EMAIL_MAILBOX_TYPE_SENTBOX) {
			debug_log("Not outbox options. sort_type is changed into EMAIL_SORT_DATE_RECENT");
			view->sort_type_index = SORTBY_DATE_RECENT;
			view->sort_type = EMAIL_SORT_DATE_RECENT;
		}
	}
}

void mailbox_account_color_list_add(EmailMailboxView *view, int account_id, int account_color)
{
	debug_enter();
	retm_if(!view, "invalid parameter, view is NULL");
	debug_log("account_color : %d", account_color);

	MailboxAccountColor *account_color_data = MEM_ALLOC(account_color_data, 1);
	account_color_data->account_id = account_id;
	account_color_data->account_color = account_color;
	view->account_color_list = g_list_append(view->account_color_list, account_color_data);
}

void mailbox_account_color_list_update(EmailMailboxView *view, int account_id, int update_color)
{
	debug_enter();
	retm_if(account_id <= 0 || !view, "invalid parameter, account_id : %d", account_id);

	if (view->account_color_list) {
		GList *cur = NULL;
		MailboxAccountColor *account_color_data = NULL;
		G_LIST_FOREACH(view->account_color_list, cur, account_color_data) {
			if (account_color_data->account_id == account_id) {
				account_color_data->account_color = update_color;
				break;
			}
		}
	}
}

void mailbox_account_color_list_remove(EmailMailboxView *view, int account_id)
{
	debug_enter();
	retm_if(account_id <= 0 || !view, "invalid parameter, account_id : %d", account_id);
	debug_log("account_id : %d", account_id);

	if (view->account_color_list) {
		GList *cur = NULL;
		MailboxAccountColor *account_color_data = NULL;
		G_LIST_FOREACH(view->account_color_list, cur, account_color_data) {
			if (account_color_data->account_id == account_id) {
				view->account_color_list = g_list_remove(view->account_color_list, account_color_data);
				FREE(account_color_data);
				break;
			}
		}
	}
}

int mailbox_account_color_list_get_account_color(EmailMailboxView *view, int account_id)
{
	if (account_id <= 0 || !view) {
		debug_log("invalid parameter, account_id : %d", account_id);
		return 0;
	}

	if (view->account_color_list) {
		GList *cur = NULL;
		MailboxAccountColor *account_color_data = NULL;
		G_LIST_FOREACH(view->account_color_list, cur, account_color_data) {
			if (account_color_data->account_id == account_id) {
				return account_color_data->account_color;
			}
		}
	}
	return 0;
}

void mailbox_account_color_list_free(EmailMailboxView *view)
{
	retm_if(view == NULL, "view[NULL]");

	debug_log("view->account_color_list(%p)", view->account_color_list);

	if (view->account_color_list) {
		GList *cur = NULL;
		MailboxAccountColor *account_color_data = NULL;
		G_LIST_FOREACH(view->account_color_list, cur, account_color_data) {
			FREE(account_color_data);
		}
		g_list_free(view->account_color_list);
		view->account_color_list = NULL;
	}
}

void mailbox_set_input_entry_limit(Evas_Object *entry, int max_char_count, int max_byte_count)
{
	Elm_Entry_Filter_Limit_Size limit_filter_data;

	limit_filter_data.max_char_count = max_char_count;
	limit_filter_data.max_byte_count = max_byte_count;

	elm_entry_markup_filter_append(entry, elm_entry_filter_limit_size, &limit_filter_data);
	evas_object_smart_callback_add(entry, "maxlength,reached", _entry_maxlength_reached_cb, NULL);

	return;
}

