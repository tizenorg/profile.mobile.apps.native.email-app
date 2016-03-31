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

#include <limits.h>
#include <account.h>
#include <metadata_extractor.h>
#include <system_settings.h>

#include "email-setting-utils.h"
#include "email-view-account-details.h"
#include "email-view-account-edit.h"
#include "email-view-signature-setting.h"
#include "email-setting-noti-mgr.h"
#include "email-view-notification.h"
#include "email-utils.h"

#define EMAIL_SIZE_NO_LIMIT INT_MAX

typedef struct view_data EmailSettingView;

static email_account_t *account_data = NULL;
static EmailSettingView *g_vd = NULL;

static int _create(email_view_t *self);
static void _update(email_view_t *self, int flags);
static void _destroy(email_view_t *self);
static void _on_back_cb(email_view_t *self);

static void _update_view(EmailSettingView *view);

static void _push_naviframe(EmailSettingView *view);
static void _create_list(EmailSettingView *view);
static Eina_Bool _update_account_info(void *data);
static Eina_Bool _validate_account(void *data);
static void _account_update_validate_cb(int account_id, email_setting_response_data *response, void *user_data);

static char *_gl_ef_text_get_cb(void *data, Evas_Object *obj, const char *part);
static char *_gl_sig_text_get_cb(void *data, Evas_Object *obj, const char *part);
static char *_gl_onoff_text_get_cb(void *data, Evas_Object *obj, const char *part);
static char *_gl_wifi_autodownload_onoff_text_get_cb(void *data, Evas_Object *obj, const char *part);
static void _gl_wifi_autodownload_text_size_change_cb(system_settings_key_e key, void *user_data);
static char *_gl_index_text_get_cb(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_gl_onoff_content_get_cb(void *data, Evas_Object *obj, const char *part);
static char *_gl_sync_onoff_text_get_cb(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_gl_sync_onoff_content_get_cb(void *data, Evas_Object *obj, const char *part);
static char *_gl_sync_text_get_cb(void *data, Evas_Object *obj, const char *part);
static char *_gl_roaming_text_get_cb(void *data, Evas_Object *obj, const char *part);
static char *_gl_ex_roaming_text_get_cb(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_gl_ex_roaming_content_get_cb(void *data, Evas_Object *obj, const char *part);
static char *_gl_ex_sync_text_get_cb(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_gl_ex_sync_content_get_cb(void *data, Evas_Object *obj, const char *part);
static char *_gl_size_text_get_cb(void *data, Evas_Object *obj, const char *part);
static char *_gl_ex_size_text_get_cb(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_gl_ex_size_content_get_cb(void *data, Evas_Object *obj, const char *part);

static void _create_toolbar_more_btn(EmailSettingView *view);
static void _gl_ex_size_sel_cb(void *data, Evas_Object *obj, void *event_info);
static void _gl_ex_size_radio_cb(void *data, Evas_Object *obj, void *event_info);

static void _show_finished_cb(void *data, Evas_Object *obj, void *event_info);
static void _popup_cancel_cb(void *data, Evas_Object *obj, void *event_info);
static void _popup_delete_ok_cb(void *data, Evas_Object *obj, void *event_info);
static Eina_Bool _after_delete_cb(void *data);

static void _popup_ok_cb(void *data, Evas_Object *obj, void *event_info);

/* Account popup cbs */
static void _account_popup_name_done_cb(void *data, Evas_Object *obj, void *event_info);
static void _account_popup_sender_name_done_cb(void *data, Evas_Object *obj, void *event_info);
static void _account_popup_password_done_cb(void *data, Evas_Object *obj, void *event_info);
static void _account_popup_entry_changed_cb(void *data, Evas_Object *obj, void *event_info);
static void _account_popup_cancel_cb(void *data, Evas_Object *obj, void *event_info);

static char *_population_password_str(int password_type);
static int _is_imap_push_supported(const email_account_t *account);
static void _delete_account_cb(void *data, Evas_Object *obj, void *event_info);
static void _gl_account_sel_cb(void *data, Evas_Object *obj, void *event_info);
static void _gl_sel_cb(void *data, Evas_Object *obj, void *event_info);
static void _onoff_cb(void *data, Evas_Object *obj, void *event_info);
static void _gl_ex_sel_cb(void *data, Evas_Object *obj, void *event_info);
static void _sync_onoff_cb(void *data, Evas_Object *obj, void *event_info);
static void _gl_ex_roaming_sel_cb(void *data, Evas_Object *obj, void *event_info);
static void _gl_ex_roaming_radio_cb(void *data, Evas_Object *obj, void *event_info);
static void _gl_ex_sync_sel_cb(void *data, Evas_Object *obj, void *event_info);
static void _gl_ex_sync_radio_cb(void *data, Evas_Object *obj, void *event_info);
static void _perform_sync(EmailSettingView *view);
static void _perform_cancel_sync(EmailSettingView *view);
static void _update_toolbar_sync_button_state(EmailSettingView *view);
static void _sync_start_cb(int account_id, email_setting_response_data *response, void *user_data);
static void _sync_end_cb(int account_id, email_setting_response_data *response, void *user_data);
static Eina_Bool _after_save_cb(void *data);

#define SYNC_SCHEDULE_ITEM_COUNT 8
static int sync_schedule[SYNC_SCHEDULE_ITEM_COUNT] = { 0, -1, 10, 30, 60, 180, 360, 1440 };

static email_string_t EMAIL_SETTING_STRING_EMAIL = {PACKAGE, "IDS_ST_HEADER_EMAIL"};
static email_string_t EMAIL_SETTING_STRING_ACCOUNT_NAME = {PACKAGE, "IDS_ST_TMBODY_ACCOUNT_NAME"};
static email_string_t EMAIL_SETTING_STRING_PWD = {PACKAGE, "IDS_ST_TMBODY_PASSWORD"};
static email_string_t EMAIL_SETTING_STRING_EMAIL_SETTINGS = {PACKAGE, "IDS_EMAIL_HEADER_EMAIL_SETTINGS"};
static email_string_t EMAIL_SETTING_STRING_SENDER_NAME = {PACKAGE, "IDS_ST_TMBODY_SENDER_NAME"};
static email_string_t EMAIL_SETTING_STRING_SIGNATURE = {PACKAGE, "IDS_EMAIL_MBODY_SIGNATURE"};
static email_string_t EMAIL_SETTING_STRING_DISPLAY_IMAGES = {PACKAGE, "IDS_EMAIL_MBODY_DISPLAY_IMAGES"};
static email_string_t EMAIL_SETTING_STRING_EMAIL_SIZE = {PACKAGE, "IDS_ST_HEADER_LIMIT_RETRIEVAL_SIZE"};
static email_string_t EMAIL_SETTING_STRING_WIFI_AUTODOWNLOAD = {PACKAGE, "IDS_EMAIL_OPT_AUTOMATICALLY_DOWNLOAD_ATTACHMENTS_WHEN_CONNECTED_TO_A_WI_FI_NETWORK"};
static email_string_t EMAIL_SETTING_STRING_SERVER_SETTINGS = {PACKAGE, "IDS_ST_MBODY_SERVER_SETTINGS"};
static email_string_t EMAIL_SETTING_STRING_NOTIFICATION_SETTINGS = {PACKAGE, "IDS_EMAIL_HEADER_NOTIFICATIONS_ABB"};
static email_string_t EMAIL_SETTING_STRING_SYNC_SETTINGS = {PACKAGE, "IDS_EMAIL_TMBODY_SYNC_SETTINGS"};
static email_string_t EMAIL_SETTING_STRING_EMAIL_SYNC = {PACKAGE, "IDS_ST_MBODY_SYNC_EMAILS"};
static email_string_t EMAIL_SETTING_STRING_NOT_SYNCED_YET = {PACKAGE, "IDS_ST_SBODY_NOT_SYNCED_YET_M_STATUS"};
static email_string_t EMAIL_SETTING_STRING_SYNCING_ONGOING = {PACKAGE, "IDS_EMAIL_BODY_SYNCING_ING"};
static email_string_t EMAIL_SETTING_STRING_SYNCING_FAILED = {PACKAGE, "IDS_EMAIL_BODY_SYNC_FAILED_TRY_LATER"};
static email_string_t EMAIL_SETTING_STRING_EMAIL_SYNC_SCHEDULE = {PACKAGE, "IDS_ST_MBODY_EMAIL_SYNC_SCHEDULE"};
static email_string_t EMAIL_SETTING_STRING_WHILE_ROAMING = {PACKAGE, "IDS_ST_TMBODY_SYNC_WHILE_ROAMING"};
static email_string_t EMAIL_SETTING_STRING_E_1H = {PACKAGE, "IDS_ST_OPT_EVERY_HOUR_ABB"};
static email_string_t EMAIL_SETTING_STRING_PUSH = {PACKAGE, "IDS_ST_OPT_PUSH"};
static email_string_t EMAIL_SETTING_STRING_EVERY_DAY = {PACKAGE, "IDS_ST_OPT_EVERY_DAY"};
static email_string_t EMAIL_SETTING_STRING_PD_H = {PACKAGE, "IDS_ST_OPT_EVERY_PD_HOURS_ABB2"};
static email_string_t EMAIL_SETTING_STRING_PD_MIN = {PACKAGE, "IDS_ST_OPT_EVERY_PD_MINUTES_ABB"};
static email_string_t EMAIL_SETTING_STRING_MANUAL = {PACKAGE, "IDS_ST_OPT_MANUAL"};
static email_string_t EMAIL_SETTING_STRING_USE_ABOVE_SETTINGS = {PACKAGE, "IDS_ST_OPT_KEEP_NON_ROAMING_SETTINGS_ABB"};

static email_string_t EMAIL_SETTING_STRING_REFRESH = {PACKAGE, "IDS_ST_TMBODY_SYNC"};
static email_string_t EMAIL_SETTING_STRING_DONE = {PACKAGE, "IDS_EMAIL_BUTTON_DONE"};
static email_string_t EMAIL_SETTING_STRING_VALIDATING_ACCOUNT_ING = {PACKAGE, "IDS_ST_TPOP_VALIDATING_ACCOUNT_ING_ABB"};
static email_string_t EMAIL_SETTING_STRING_DELETE_ACCOUNT = {PACKAGE, "IDS_ST_OPT_REMOVE"};
static email_string_t EMAIL_SETTING_STRING_GOOGLE_ACCOUNT_DELETED = {PACKAGE, "IDS_EMAIL_POP_DELETING_THIS_ACCOUNT_ALSO_DELETES_ALL_ACCOUNT_MESSAGES_CONTACTS_AND_OTHER_DATA_FROM_THE_DEVICE_TAP_DELETE_TO_CONTINUE"};
static email_string_t EMAIL_SETTING_STRING_DELETING = {PACKAGE, "IDS_EMAIL_POP_REMOVING_ACCOUNT_ING"};
static email_string_t EMAIL_SETTING_STRING_UNABLE_DELETE = {PACKAGE, "IDS_EMAIL_HEADER_FAILED_TO_DELETE_ACCOUNT_ABB"};
static email_string_t EMAIL_SETTING_STRING_CANCEL = {PACKAGE, "IDS_EMAIL_BUTTON_CANCEL"};
static email_string_t EMAIL_SETTING_STRING_OK = {PACKAGE, "IDS_EMAIL_BUTTON_OK"};
static email_string_t EMAIL_SETTING_STRING_DELETE = {PACKAGE, "IDS_ST_OPT_REMOVE"};
static email_string_t EMAIL_SETTING_STRING_ACCOUNT_DELETED = {PACKAGE, "IDS_ST_POP_ALL_DATA_RELATED_TO_THIS_ACCOUNT_WILL_BE_DELETED_FROM_THE_DEVICE"};
static email_string_t EMAIL_SETTING_STRING_HEADER_DELETE = {PACKAGE, "IDS_EMAIL_HEADER_REMOVE_ACCOUNT_ABB"};
static email_string_t EMAIL_SETTING_STRING_HEADER_ONLY = {PACKAGE, "IDS_ST_OPT_HEADER_ONLY"};
static email_string_t EMAIL_SETTING_STRING_ENTIRE_EMAIL = {PACKAGE, "IDS_ST_OPT_NO_LIMIT"};

enum {
	ACCOUNT_NAME_LIST_ITEM = 1,
	PASSWORD_LIST_ITEM,
	EMAIL_SETTING_TITLE_LIST_ITEM,
	DISPLAY_NAME_LIST_ITEM,
	SIGNATURE_LIST_ITEM,
	SHOW_IMAGE_LIST_ITEM,
	EMAIL_SIZE_LIST_ITEM,
	WIFI_AUTODOWNLOAD_LIST_ITEM,
	SERVER_SETTING_LIST_ITEM,
	NOTIFICATION_LIST_ITEM,
	SYNC_SETTING_TITLE_LIST_ITEM,
	SYNC_ONOFF_LIST_ITEM,
	SYNC_SCHEDULE_LIST_ITEM,
	WHILE_ROAMING_LIST_ITEM,
};

static int genlist_item_updated[12] = {0,};

struct view_data {
	email_view_t base;

	GSList *list_items;
	GSList *itc_list;

	Ecore_Timer *del_timer;
	Ecore_Timer *edit_vc_timer;
	Ecore_Timer *validate_timer;

	Evas_Object *genlist;
	Elm_Genlist_Item_Class *itc_account_details;
	Elm_Genlist_Item_Class *itc_sig_noti_serversetting;
	Elm_Genlist_Item_Class *itc_onoff;
	Elm_Genlist_Item_Class *itc_wifi_autodownload_onoff;
	Elm_Genlist_Item_Class *itc_sync_groupindex;
	Elm_Genlist_Item_Class *itc_sync_onoff;
	Elm_Genlist_Item_Class *itc_sync;
	Elm_Genlist_Item_Class *itc_roaming;
	Elm_Genlist_Item_Class *itc_ex_roaming;
	Elm_Genlist_Item_Class *itc_ex_sync;
	Elm_Genlist_Item_Class *itc_size;
	Elm_Genlist_Item_Class *itc_ex_size;

	Evas_Object *roaming_radio_grp;
	Evas_Object *sync_radio_grp;
	Evas_Object *size_radio_grp;

	Evas_Object *sync_btn;
	Evas_Object *remove_btn;
	Ecore_Idler *account_info_idler;

	int is_google_launched;
	int is_imap_push_supported;
	int handle;

	Eina_Bool syncing;
	Eina_Bool sync_status_failed;
	Eina_Bool sync_status_canceled;

	char *signature_backup;
};

typedef struct _ListItemData {
	int index;
	Elm_Object_Item *gl_it;
	Evas_Object *entry;
	char *entry_str;
	Evas_Object *check;
	Eina_Bool is_checked;
	Elm_Entry_Filter_Limit_Size *entry_limit;
	EmailSettingView *view;
} ListItemData;

static Evas_Object *_get_option_genlist(Evas_Object *parent, ListItemData *li);

void create_account_details_view(EmailSettingModule *module)
{
	debug_enter();
	retm_if(!module, "module is null");

	EmailSettingView *view = calloc(1, sizeof(EmailSettingView));
	retm_if(!view, "view data is null");

	view->base.create = _create;
	view->base.update = _update;
	view->base.destroy = _destroy;
	view->base.on_back_key = _on_back_cb;

	debug_log("view create result: %d", email_module_create_view(&module->base, &view->base));
}

static int _create(email_view_t *self)
{
	debug_enter();
	retvm_if(!self, -1, "self is null");

	EmailSettingView *view = (EmailSettingView *)self;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	if (!setting_get_acct_full_data(module->account_id, &account_data)) {
		debug_error("setting_get_acct_full_data failed");
		return FALSE;
	}

	EMAIL_SETTING_PRINT_ACCOUNT_INFO(account_data);
	view->base.content = setting_add_inner_layout(&view->base);
	_push_naviframe(view);

	g_vd = view;

	_create_toolbar_more_btn(view);

	if (account_data->incoming_server_type == EMAIL_SERVER_TYPE_IMAP4)
			view->is_imap_push_supported = _is_imap_push_supported(account_data);

	_create_list(view);

	setting_noti_subscribe(module, EMAIL_SETTING_SYNC_START_MAIL_NOTI, _sync_start_cb, view);
	setting_noti_subscribe(module, EMAIL_SETTING_SYNC_MAIL_NOTI, _sync_end_cb, view);
	setting_noti_subscribe(module, EMAIL_SETTING_ACCOUNT_VALIDATE_AND_UPDATE_NOTI, _account_update_validate_cb, view);

	_update_view(view);

	return 0;
}

static void _update(email_view_t *self, int flags)
{
	debug_enter();
	retm_if(!self, "self is null");

	EmailSettingView *view = (EmailSettingView *)self;

	if (flags & EVUF_LANGUAGE_CHANGED) {
		/* refreshing genlist. */
		elm_genlist_realized_items_update(view->genlist);
	}

	if (flags & (EVUF_WAS_PAUSED | EVUF_POPPING)) {
		_update_view(view);
	}
}

static void _update_view(EmailSettingView *view)
{
	debug_enter();
	retm_if(!view, "view data is null");

	EmailSettingModule *module = (EmailSettingModule *)view->base.module;
	account_user_data_t *user_data = account_data->user_data;

	/* read account info again. */
	if (account_data) {
		genlist_item_updated[SHOW_IMAGE_LIST_ITEM-1] = user_data->show_images;
		genlist_item_updated[WIFI_AUTODOWNLOAD_LIST_ITEM-1] = user_data->wifi_auto_download;
		genlist_item_updated[SYNC_ONOFF_LIST_ITEM-1] = !account_data->sync_disabled;
		email_engine_free_account_list(&account_data, 1);
		account_data = NULL;
	}

	if (!setting_get_acct_full_data(module->account_id, &account_data)) {
		debug_error("setting_get_acct_full_data failed");
		return;
	}

	EMAIL_SETTING_PRINT_ACCOUNT_INFO(account_data);

	if (!account_data->sync_disabled &&
			(account_data->sync_status & SYNC_STATUS_SYNCING)) {
		view->syncing = EINA_TRUE;
	} else {
		view->syncing = EINA_FALSE;
	}
	view->sync_status_failed = EINA_FALSE;
	view->sync_status_canceled = EINA_FALSE;
	_update_toolbar_sync_button_state(view);
	GSList *l = view->list_items;
	while (l) {
		ListItemData *_li = l->data;
		if (_li->check && _li->is_checked == genlist_item_updated[_li->index-1]) {
			debug_log("onoff variables %p,%d,%d,%d", _li->check, _li->is_checked, _li->index, genlist_item_updated[_li->index-1]);
		} else {
			elm_genlist_item_update(_li->gl_it);
		}
		l = g_slist_next(l);
	}
}

static void _destroy(email_view_t *self)
{
	debug_enter();

	EmailSettingView *view = (EmailSettingView *)self;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	retm_if(!view, "view data is null");

	setting_noti_unsubscribe(module, EMAIL_SETTING_SYNC_START_MAIL_NOTI, _sync_start_cb);
	setting_noti_unsubscribe(module, EMAIL_SETTING_SYNC_MAIL_NOTI, _sync_end_cb);
	setting_noti_unsubscribe(module, EMAIL_SETTING_ACCOUNT_VALIDATE_AND_UPDATE_NOTI, _account_update_validate_cb);
	DELETE_TIMER_OBJECT(view->del_timer);
	DELETE_IDLER_OBJECT(view->account_info_idler);
	DELETE_TIMER_OBJECT(view->edit_vc_timer);
	DELETE_TIMER_OBJECT(view->validate_timer);
	system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_FONT_SIZE);

	if (account_data) {
		email_engine_free_account_list(&account_data, 1);
		account_data = NULL;
	}

	GSList *l = view->list_items;
	while (l) {
		ListItemData *li = l->data;
		FREE(li->entry_str);
		FREE(li->entry_limit);
		FREE(li);
		l = g_slist_next(l);
	}
	g_slist_free(view->list_items);

	l = view->itc_list;
	while (l) {
		Elm_Genlist_Item_Class *itc = l->data;
		EMAIL_GENLIST_ITC_FREE(itc);
		l = g_slist_next(l);
	}
	g_slist_free(view->itc_list);

	free(view);
}

static void _push_naviframe(EmailSettingView *view)
{
	debug_enter();

	email_module_view_push(&view->base, EMAIL_SETTING_STRING_EMAIL.id, 0);
	elm_object_item_domain_text_translatable_set(view->base.navi_item, EMAIL_SETTING_STRING_EMAIL.domain, EINA_TRUE);

	if (account_data->sync_status & SYNC_STATUS_SYNCING) {
		view->syncing = EINA_TRUE;
	}
	evas_object_show(view->base.module->navi);
}

static void _create_list(EmailSettingView *view)
{
	debug_enter();

	retm_if(!view, "view data is null");

	Elm_Object_Item *git = NULL;
	ListItemData *li = NULL;
	Elm_Object_Item *item = NULL;
	account_user_data_t *user_data = account_data->user_data;

	view->genlist = elm_genlist_add(view->base.content);
	elm_scroller_policy_set(view->genlist, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
	elm_genlist_mode_set(view->genlist, ELM_LIST_COMPRESS);
	elm_genlist_homogeneous_set(view->genlist, EINA_TRUE);

	view->roaming_radio_grp = elm_radio_add(view->base.content);
	elm_radio_value_set(view->roaming_radio_grp, -1);
	evas_object_hide(view->roaming_radio_grp);

	view->sync_radio_grp = elm_radio_add(view->base.content);
	elm_radio_value_set(view->sync_radio_grp, -1);
	evas_object_hide(view->sync_radio_grp);

	view->size_radio_grp = elm_radio_add(view->base.content);
	elm_radio_value_set(view->size_radio_grp, -1);
	evas_object_hide(view->size_radio_grp);

	view->itc_account_details = setting_get_genlist_class_item("type1", _gl_ef_text_get_cb, NULL, NULL, NULL);
	view->itc_sig_noti_serversetting = setting_get_genlist_class_item("type1", _gl_sig_text_get_cb, NULL, NULL, NULL);
	view->itc_onoff = setting_get_genlist_class_item("type1", _gl_onoff_text_get_cb, _gl_onoff_content_get_cb, NULL, NULL);
	view->itc_sync_groupindex = setting_get_genlist_class_item("group_index", _gl_index_text_get_cb, NULL, NULL, NULL);
	view->itc_wifi_autodownload_onoff  = setting_get_genlist_class_item("multiline", _gl_wifi_autodownload_onoff_text_get_cb, _gl_onoff_content_get_cb, NULL, NULL);
	view->itc_sync_onoff = setting_get_genlist_class_item("type1", _gl_sync_onoff_text_get_cb, _gl_sync_onoff_content_get_cb, NULL, NULL);
	view->itc_sync = setting_get_genlist_class_item("type1", _gl_sync_text_get_cb, NULL, NULL, NULL);
	view->itc_roaming = setting_get_genlist_class_item("type1", _gl_roaming_text_get_cb, NULL, NULL, NULL);
	view->itc_ex_roaming = setting_get_genlist_class_item("type1", _gl_ex_roaming_text_get_cb, _gl_ex_roaming_content_get_cb, NULL, NULL);
	view->itc_ex_sync = setting_get_genlist_class_item("type1", _gl_ex_sync_text_get_cb, _gl_ex_sync_content_get_cb, NULL, NULL);
	view->itc_size = setting_get_genlist_class_item("type1", _gl_size_text_get_cb, NULL, NULL, NULL);
	view->itc_ex_size = setting_get_genlist_class_item("type1", _gl_ex_size_text_get_cb, _gl_ex_size_content_get_cb, NULL, NULL);

	view->itc_list = g_slist_append(view->itc_list, view->itc_account_details);
	view->itc_list = g_slist_append(view->itc_list, view->itc_sig_noti_serversetting);
	view->itc_list = g_slist_append(view->itc_list, view->itc_onoff);
	view->itc_list = g_slist_append(view->itc_list, view->itc_wifi_autodownload_onoff);
	view->itc_list = g_slist_append(view->itc_list, view->itc_sync_groupindex);
	view->itc_list = g_slist_append(view->itc_list, view->itc_sync_onoff);
	view->itc_list = g_slist_append(view->itc_list, view->itc_sync);
	view->itc_list = g_slist_append(view->itc_list, view->itc_ex_roaming);
	view->itc_list = g_slist_append(view->itc_list, view->itc_ex_sync);
	view->itc_list = g_slist_append(view->itc_list, view->itc_size);
	view->itc_list = g_slist_append(view->itc_list, view->itc_ex_size);

	/* account name */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = ACCOUNT_NAME_LIST_ITEM;
	li->view = view;
	li->entry_str = elm_entry_utf8_to_markup(account_data->account_name);
	li->gl_it = item = elm_genlist_item_append(view->genlist, view->itc_account_details, li, NULL,
			ELM_GENLIST_ITEM_NONE, _gl_account_sel_cb, li);
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_ALWAYS);
	view->list_items = g_slist_append(view->list_items, li);

	/* password */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = PASSWORD_LIST_ITEM;
	li->view = view;
	li->entry_str = _population_password_str(EMAIL_GET_INCOMING_PASSWORD_LENGTH);
	li->gl_it = item = elm_genlist_item_append(view->genlist, view->itc_sig_noti_serversetting, li, NULL,
			ELM_GENLIST_ITEM_NONE, _gl_account_sel_cb, li);
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_ALWAYS);
	view->list_items = g_slist_append(view->list_items, li);

	/* Email settings - group index */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = EMAIL_SETTING_TITLE_LIST_ITEM;
	li->view = view;
	li->gl_it = git = elm_genlist_item_append(view->genlist, view->itc_sync_groupindex, li, NULL,
			ELM_GENLIST_ITEM_GROUP, NULL, NULL);
	elm_genlist_item_select_mode_set(git, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
	view->list_items = g_slist_append(view->list_items, li);

	/* display name */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = DISPLAY_NAME_LIST_ITEM;
	li->view = view;
	li->entry_str = elm_entry_utf8_to_markup(account_data->user_display_name);
	li->gl_it = item = elm_genlist_item_append(view->genlist, view->itc_account_details, li, NULL,
			ELM_GENLIST_ITEM_NONE, _gl_account_sel_cb, li);
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_ALWAYS);
	view->list_items = g_slist_append(view->list_items, li);

	/*--signature--*/
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = SIGNATURE_LIST_ITEM;
	li->view = view;
	li->gl_it = elm_genlist_item_append(view->genlist, view->itc_sig_noti_serversetting, li,
			NULL, ELM_GENLIST_ITEM_NONE, _gl_sel_cb, li);
	view->list_items = g_slist_append(view->list_items, li);


	/*--show images--*/
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = SHOW_IMAGE_LIST_ITEM;
	li->is_checked = user_data->show_images ? EINA_TRUE : EINA_FALSE;
	li->view = view;
	li->gl_it = elm_genlist_item_append(view->genlist, view->itc_onoff, li,
			NULL, ELM_GENLIST_ITEM_NONE, _gl_sel_cb, li);
	view->list_items = g_slist_append(view->list_items, li);

	/*--Email size--*/
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = EMAIL_SIZE_LIST_ITEM;
	li->view = view;
	li->gl_it = elm_genlist_item_append(view->genlist, view->itc_size, li,
			NULL, ELM_GENLIST_ITEM_NONE, _gl_ex_sel_cb, li);
	view->list_items = g_slist_append(view->list_items, li);

	/*--Auto download attachment via WiFi--*/
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = WIFI_AUTODOWNLOAD_LIST_ITEM;
	li->is_checked = user_data->wifi_auto_download ? EINA_TRUE : EINA_FALSE;
	li->view = view;
	li->gl_it = elm_genlist_item_append(view->genlist, view->itc_wifi_autodownload_onoff, li,
			NULL, ELM_GENLIST_ITEM_NONE, _gl_sel_cb, li);
	system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_FONT_SIZE, _gl_wifi_autodownload_text_size_change_cb, li->gl_it);
	view->list_items = g_slist_append(view->list_items, li);

	/*--Server setting--*/
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = SERVER_SETTING_LIST_ITEM;
	li->view = view;
	li->gl_it = elm_genlist_item_append(view->genlist, view->itc_sig_noti_serversetting, li,
			NULL, ELM_GENLIST_ITEM_NONE, _gl_sel_cb, li);
	view->list_items = g_slist_append(view->list_items, li);

	/*--Notification--*/
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = NOTIFICATION_LIST_ITEM;
	li->view = view;
	li->gl_it = elm_genlist_item_append(view->genlist, view->itc_sig_noti_serversetting, li,
			NULL, ELM_GENLIST_ITEM_NONE, _gl_sel_cb, li);
	view->list_items = g_slist_append(view->list_items, li);

	/* Sync settings - group index */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = SYNC_SETTING_TITLE_LIST_ITEM;
	li->view = view;
	li->gl_it = git = elm_genlist_item_append(view->genlist, view->itc_sync_groupindex, li, NULL,
			ELM_GENLIST_ITEM_GROUP, NULL, NULL);
	elm_genlist_item_select_mode_set(git, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
	view->list_items = g_slist_append(view->list_items, li);

	/*sync on/off*/
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = SYNC_ONOFF_LIST_ITEM;
	li->view = view;
	li->gl_it = elm_genlist_item_append(view->genlist, view->itc_sync_onoff, li,
			NULL, ELM_GENLIST_ITEM_NONE, _gl_sel_cb, li);
	view->list_items = g_slist_append(view->list_items, li);

	/*sync schedule*/
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = SYNC_SCHEDULE_LIST_ITEM;
	li->view = view;
	li->gl_it = elm_genlist_item_append(view->genlist, view->itc_sync, li,
			NULL, ELM_GENLIST_ITEM_NONE, _gl_ex_sel_cb, li);
	view->list_items = g_slist_append(view->list_items, li);

	/*while roaming*/
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = WHILE_ROAMING_LIST_ITEM;
	li->view = view;
	li->gl_it = elm_genlist_item_append(view->genlist, view->itc_roaming, li,
			NULL, ELM_GENLIST_ITEM_NONE, _gl_ex_sel_cb, li);
	view->list_items = g_slist_append(view->list_items, li);

	elm_object_part_content_set(view->base.content, "elm.swallow.content", view->genlist);

}

static Eina_Bool _update_account_info(void *data)
{
	debug_enter();
	EmailSettingView *view = data;

	retvm_if(!view, ECORE_CALLBACK_CANCEL, "view data is null");

	view->account_info_idler = NULL;

	retvm_if(!account_data, ECORE_CALLBACK_CANCEL, "account data is null");

	if (email_engine_update_account(account_data->account_id, account_data) == TRUE)
		debug_log("Account updated successfully");

	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _validate_account(void *data)
{
	debug_enter();
	EmailSettingView *view = data;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;
	int account_id = module->account_id;

	retvm_if(!view, ECORE_CALLBACK_CANCEL, "view is NULL");

	gboolean ret;
	DELETE_TIMER_OBJECT(view->validate_timer);

	EMAIL_SETTING_PRINT_ACCOUNT_INFO(account_data);
	ret = email_engine_update_account_with_validation(account_id, account_data);

	DELETE_EVAS_OBJECT(module->popup);

	if (ret) {
		debug_log("Start Account Validation");
		module->popup = setting_get_pb_process_notify(&view->base,
				&(EMAIL_SETTING_STRING_VALIDATING_ACCOUNT_ING), 0,
				NULL, NULL,
				NULL, NULL,
				POPUP_BACK_TYPE_DESTROY_WITH_CANCEL_OP, NULL);
	} else {
		email_setting_response_data response;
		response.account_id = module->account_id;
		response.type = NOTI_VALIDATE_AND_UPDATE_ACCOUNT_FAIL;
		response.err = ret;
		_account_update_validate_cb(module->account_id, &response, view);
	}
	return ECORE_CALLBACK_CANCEL;
}

static void _account_update_validate_cb(int account_id, email_setting_response_data *response, void *user_data)
{
	debug_enter();
	EmailSettingView *view = user_data;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	retm_if(!response, "response data is NULL");
	retm_if(module->account_id != response->account_id, "account_id is different");

	debug_log("response err: %d", response->err);
	debug_log("response type: %d", response->type);

	if (response->err == EMAIL_ERROR_NONE ||
			response->err == EMAIL_ERROR_VALIDATE_ACCOUNT_OF_SMTP) {
		_update_account_info(view);
		DELETE_TIMER_OBJECT(view->edit_vc_timer);
		if (response->err == EMAIL_ERROR_VALIDATE_ACCOUNT_OF_SMTP)
			debug_warning("smtp validation failed but it can be ignored");

		view->edit_vc_timer = ecore_timer_add(0.5, _after_save_cb, view);
	} else {
		DELETE_EVAS_OBJECT(module->popup);

		if (response->type != NOTI_VALIDATE_AND_UPDATE_ACCOUNT_CANCEL) {
			const email_string_t *err_msg = setting_get_service_fail_type(response->err);
			const email_string_t *header = setting_get_service_fail_type_header(response->err);
			module->popup = setting_get_notify(&view->base, header, err_msg, 1,
					&(EMAIL_SETTING_STRING_OK),
					_popup_ok_cb, NULL, NULL);
		}
	}
}

static char *_population_password_str(int password_type)
{
	debug_enter();
	char password_buf[MAX_STR_LEN] = { 0, };
	int pass_len = 0;
	int i = 0;

	retvm_if(!email_engine_get_password_length_of_account(account_data->account_id, password_type, &pass_len),
			NULL, "email_engine_get_password_length_of_account failed");

	if (pass_len > 0 && pass_len < MAX_STR_LEN) {
		for (i = 0; i < pass_len; i++) {
			password_buf[i] = '*';
		}
	}

	return g_strdup(password_buf);
}

static int _is_imap_push_supported(const email_account_t *account)
{
	int ret = 0;

	if (account->retrieval_mode & EMAIL_IMAP4_IDLE_SUPPORTED)
		ret = 1;

	return ret;
}

static void _on_back_cb(email_view_t *self)
{
	debug_enter();
	retm_if(!self, "self is null");

	email_module_exit_view(self);
}

static void _gl_account_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	ListItemData *li = data;
	EmailSettingView *view = li->view;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;
	Elm_Object_Item *item = event_info;
	elm_genlist_item_selected_set(item, EINA_FALSE);
	char *str = NULL;

	if (li->index == ACCOUNT_NAME_LIST_ITEM) {
		str = g_strdup(account_data->account_name);
		module->popup = setting_get_entry_content_notify(&view->base, &(EMAIL_SETTING_STRING_ACCOUNT_NAME), str,
				2, &(EMAIL_SETTING_STRING_CANCEL), _account_popup_cancel_cb, &(EMAIL_SETTING_STRING_DONE), _account_popup_name_done_cb, POPUP_ENTRY_EDITFIELD);
		Evas_Object *entry_layout = elm_object_content_get(module->popup);
		Evas_Object *entry = elm_layout_content_get(entry_layout, "elm.swallow.content");
		li->entry_limit = setting_set_input_entry_limit(entry, EMAIL_LIMIT_ACCOUNT_NAME_LENGTH, 0);
		li->entry = entry;
		evas_object_smart_callback_add(li->entry, "activated", _account_popup_name_done_cb, view);
		evas_object_smart_callback_add(li->entry, "changed", _account_popup_entry_changed_cb, view);
		evas_object_smart_callback_add(li->entry, "preedit,changed", _account_popup_entry_changed_cb, view);
	} else if (li->index == DISPLAY_NAME_LIST_ITEM) {
		str = g_strdup(account_data->user_display_name);
		module->popup = setting_get_entry_content_notify(&view->base, &(EMAIL_SETTING_STRING_SENDER_NAME), str,
				2, &(EMAIL_SETTING_STRING_CANCEL), _account_popup_cancel_cb, &(EMAIL_SETTING_STRING_DONE), _account_popup_sender_name_done_cb, POPUP_ENTRY_EDITFIELD);
		Evas_Object *entry_layout = elm_object_content_get(module->popup);
		Evas_Object *entry = elm_layout_content_get(entry_layout, "elm.swallow.content");
		li->entry_limit = setting_set_input_entry_limit(entry, EMAIL_LIMIT_DISPLAY_NAME_LENGTH, 0);
		li->entry = entry;
		evas_object_smart_callback_add(li->entry, "activated", _account_popup_sender_name_done_cb, view);
		evas_object_smart_callback_add(li->entry, "changed", _account_popup_entry_changed_cb, view);
		evas_object_smart_callback_add(li->entry, "preedit,changed", _account_popup_entry_changed_cb, view);
	} else if (li->index == PASSWORD_LIST_ITEM) {
		module->popup = setting_get_entry_content_notify(&view->base, &(EMAIL_SETTING_STRING_PWD), NULL,
				2, &(EMAIL_SETTING_STRING_CANCEL), _account_popup_cancel_cb, &(EMAIL_SETTING_STRING_DONE), _account_popup_password_done_cb, POPUP_ENRTY_PASSWORD);
		Evas_Object *entry_layout = elm_object_content_get(module->popup);
		Evas_Object *entry = elm_layout_content_get(entry_layout, "elm.swallow.content");
		li->entry_limit = setting_set_input_entry_limit(entry, EMAIL_LIMIT_PASSWORD_LENGTH, 0);
		li->entry = entry;
		evas_object_smart_callback_add(li->entry, "activated", _account_popup_password_done_cb, view);
		evas_object_smart_callback_add(li->entry, "changed", _account_popup_entry_changed_cb, view);
		evas_object_smart_callback_add(li->entry, "preedit,changed", _account_popup_entry_changed_cb, view);
	}
	elm_object_focus_set(li->entry, EINA_TRUE);
	evas_object_show(module->popup);
	FREE(str);
}

static void _gl_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	ListItemData *li = data;
	EmailSettingView *view = li->view;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	Elm_Object_Item *item = event_info;
	elm_genlist_item_selected_set(item, EINA_FALSE);

	if (li->index == SIGNATURE_LIST_ITEM) {
		create_signature_setting_view(module);
		return;
	} else if (li->index == NOTIFICATION_LIST_ITEM) {
		create_notification_setting_view(module);
	} else if (li->index == SERVER_SETTING_LIST_ITEM) {
		create_account_edit_view(module);
	} else if (li->index == SYNC_ONOFF_LIST_ITEM) {
		Eina_Bool state = elm_check_state_get(li->check);
		elm_check_state_set(li->check, !state);
		_sync_onoff_cb(view, li->check, NULL);
	} else {
		Eina_Bool state = elm_check_state_get(li->check);
		elm_check_state_set(li->check, !state);
		_onoff_cb(li, li->check, NULL);
	}
	_update_account_info(view);
}

static void _gl_ex_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	ListItemData *li = data;
	EmailSettingView *view = li->view;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;
	Elm_Object_Item *item = event_info;
	elm_genlist_item_selected_set(item, EINA_FALSE);

	Evas_Object *genlist = NULL;
	if (li->index == WHILE_ROAMING_LIST_ITEM) {
		module->popup = setting_get_empty_content_notify(&view->base, &(EMAIL_SETTING_STRING_WHILE_ROAMING),
				0, NULL, NULL, NULL, NULL);
		genlist = _get_option_genlist(module->popup, li);
	} else if (li->index == SYNC_SCHEDULE_LIST_ITEM) {
		module->popup = setting_get_empty_content_notify(&view->base, &(EMAIL_SETTING_STRING_EMAIL_SYNC_SCHEDULE),
				0, NULL, NULL, NULL, NULL);
		genlist = _get_option_genlist(module->popup, li);
	} else if (li->index == EMAIL_SIZE_LIST_ITEM) {
		module->popup = setting_get_empty_content_notify(&view->base, &(EMAIL_SETTING_STRING_EMAIL_SIZE),
				0, NULL, NULL, NULL, NULL);
		genlist = _get_option_genlist(module->popup, li);
	}
	elm_object_content_set(module->popup, genlist);
	evas_object_show(module->popup);
	evas_object_smart_callback_add(module->popup, "show,finished", _show_finished_cb, li);
}

static void _gl_ex_roaming_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!account_data, "account_data is null");

	int index = (int)(ptrdiff_t)data;
	EmailSettingView *view = g_vd;

	Elm_Object_Item *item = event_info;
	elm_genlist_item_selected_set(item, EINA_FALSE);
	elm_radio_value_set(view->roaming_radio_grp, index);

	_gl_ex_roaming_radio_cb((void *)(ptrdiff_t)index, NULL, NULL);
}

static void _gl_ex_sync_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!account_data, "account data is null");

	int index = (int)(ptrdiff_t)data;
	EmailSettingView *view = g_vd;

	Elm_Object_Item *item = event_info;
	elm_genlist_item_selected_set(item, EINA_FALSE);
	elm_radio_value_set(view->sync_radio_grp, sync_schedule[index]);

	_gl_ex_sync_radio_cb((void *)(ptrdiff_t)index, NULL, NULL);
}

static void _onoff_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	ListItemData *li = data;
	EmailSettingView *view = li->view;

	Eina_Bool state = elm_check_state_get(obj);

	if (li->index == SHOW_IMAGE_LIST_ITEM) {
		account_user_data_t *user_data = account_data->user_data;
		user_data->show_images = state;
		debug_secure("show images %d", user_data->show_images);
	} else if (li->index == WIFI_AUTODOWNLOAD_LIST_ITEM) {
		account_user_data_t *user_data = account_data->user_data;
		user_data->wifi_auto_download = state;
		debug_secure("wifi_auto_download %d", user_data->wifi_auto_download);
	}
	_update_account_info(view);
}

static void _sync_onoff_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailSettingView *view = data;
	Elm_Object_Item *sync_sche_it = NULL;
	Elm_Object_Item *sync_roaming_it = NULL;

	Eina_Bool state = elm_check_state_get(obj);

	GSList *l = view->list_items;
	while (l) {
		ListItemData *li = l->data;
		if (li->index == SYNC_SCHEDULE_LIST_ITEM) {
			sync_sche_it = li->gl_it;
		} else if (li->index == WHILE_ROAMING_LIST_ITEM) {
			sync_roaming_it = li->gl_it;
		}
		l = g_slist_next(l);
	}

	if (state) {
		account_data->sync_disabled = 0;

		if (sync_sche_it) {
			elm_genlist_item_update(sync_sche_it);
			elm_object_item_disabled_set(sync_sche_it, EINA_FALSE);
		}

		if (sync_roaming_it) {
			elm_genlist_item_update(sync_roaming_it);
			elm_object_item_disabled_set(sync_roaming_it, EINA_FALSE);
		}
	} else {
		account_data->sync_disabled = 1;

		if (sync_sche_it) {
			elm_genlist_item_update(sync_sche_it);
			elm_object_item_disabled_set(sync_sche_it, EINA_TRUE);
		}

		if (sync_roaming_it) {
			elm_genlist_item_update(sync_roaming_it);
			elm_object_item_disabled_set(sync_roaming_it, EINA_TRUE);
		}

		_perform_cancel_sync(view);
	}
	_update_toolbar_sync_button_state(view);
	_update_account_info(view);
	DELETE_IDLER_OBJECT(view->account_info_idler);
	view->account_info_idler = ecore_idler_add(_update_account_info, view);
}

static void _gl_ex_roaming_radio_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	int index = (int)(ptrdiff_t)data;
	EmailSettingView *view = g_vd;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	if (index == 0)
		account_data->roaming_option = EMAIL_ROAMING_OPTION_RESTRICTED_BACKGROUND_TASK;
	else if (index == 1)
		account_data->roaming_option = EMAIL_ROAMING_OPTION_UNRESTRICTED;

	_update_account_info(view);

	GSList *l = view->list_items;
	while (l) {
		ListItemData *li = l->data;
		if (li->index == WHILE_ROAMING_LIST_ITEM) {
			elm_genlist_item_update(li->gl_it);
		}
		l = g_slist_next(l);
	}
	DELETE_EVAS_OBJECT(module->popup);
}

static void _gl_ex_sync_radio_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	int index = (int)(ptrdiff_t)data;
	EmailSettingView *view = g_vd;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	account_data->check_interval = sync_schedule[index];

	_update_account_info(view);

	GSList *l = view->list_items;
	while (l) {
		ListItemData *li = l->data;
		if (li->index == SYNC_SCHEDULE_LIST_ITEM) {
			elm_genlist_item_update(li->gl_it);
			break;
		}
		l = g_slist_next(l);
	}

	DELETE_EVAS_OBJECT(module->popup);
}
static void _sync_toolbar_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailSettingView *view = data;
	retm_if(!view, "view data is null");
	if (view->syncing) {
		elm_object_disabled_set(obj, EINA_TRUE);
		_perform_cancel_sync(view);
	} else {
		_perform_sync(view);
	}

	debug_leave();

}
static void _perform_sync(EmailSettingView *view)
{
	debug_enter();
	retm_if(!view, "view data is null");

	int handle = 0;
	int account_id = account_data->account_id;
	view->syncing = EINA_TRUE;
	view->sync_status_canceled = EINA_FALSE;
	_update_toolbar_sync_button_state(view);
	/* Inbox sync */
	gboolean res = FALSE;
	email_mailbox_t *mailbox = NULL;
	if (!email_engine_get_mailbox_by_mailbox_type(account_id, EMAIL_MAILBOX_TYPE_INBOX, &mailbox)) {
		debug_error("email_engine_get_mailbox_by_mailbox_type failed");
		view->sync_status_failed = EINA_TRUE;
		return;
	}

	res = email_engine_sync_folder(account_id, mailbox->mailbox_id, &handle);
	debug_log("handle: %d, res: %d", handle, res);

	view->handle = handle;
	email_engine_free_mailbox_list(&mailbox, 1);

	GSList *l = view->list_items;
	while (l) {
		ListItemData *_li = l->data;
		if (_li->index == SYNC_ONOFF_LIST_ITEM) {
			elm_genlist_item_fields_update(_li->gl_it, "elm.text.sub", ELM_GENLIST_ITEM_FIELD_TEXT);
		}
		l = g_slist_next(l);
	}
}

static void _perform_cancel_sync(EmailSettingView *view)
{
	debug_enter();
	if (view == NULL) {
		debug_error("view is NULL");
		return;
	}

	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	setting_cancel_job_by_account_id(module->account_id);

	view->syncing = EINA_FALSE;
	view->sync_status_canceled = EINA_TRUE;
}

static void _sync_start_cb(int account_id, email_setting_response_data *response, void *user_data)
{
	debug_enter();

	EmailSettingView *view = user_data;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	retm_if(!response, "response data is NULL");
	retm_if(module->account_id != response->account_id, "account_id is different");

	if (!setting_get_acct_full_data(module->account_id, &account_data)) {
		debug_warning("failed to get account data");
	}

	EMAIL_SETTING_PRINT_ACCOUNT_INFO(account_data);
	view->syncing = EINA_TRUE;

}

static void _sync_end_cb(int account_id, email_setting_response_data *response, void *user_data)
{
	debug_enter();

	EmailSettingView *view = user_data;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	if (!response) {
		debug_error("response data is NULL");
		view->sync_status_failed = EINA_TRUE;
		return;
	}
	retm_if(module->account_id != response->account_id, "account_id is different");

	if (!setting_get_acct_full_data(module->account_id, &account_data)) {
		debug_warning("failed to get account data");
		view->sync_status_failed = EINA_TRUE;
		return;
	}

	EMAIL_SETTING_PRINT_ACCOUNT_INFO(account_data);

	if (response->err != 0) {
		view->sync_status_failed = EINA_TRUE;
	} else {
		view->sync_status_failed = EINA_FALSE;
	}

	view->syncing = EINA_FALSE;
	_update_toolbar_sync_button_state(view);
	GSList *l = view->list_items;
	while (l) {
		ListItemData *_li = l->data;
		if (_li->index == SYNC_ONOFF_LIST_ITEM) {
			elm_genlist_item_fields_update(_li->gl_it, "elm.text.sub", ELM_GENLIST_ITEM_FIELD_TEXT);
		}
		l = g_slist_next(l);
	}

}

static Eina_Bool _after_save_cb(void *data)
{
	debug_enter();

	EmailSettingView *view = data;
	retv_if(view == NULL, -1);
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	GSList *l = view->list_items;
	while (l) {
		ListItemData *li = l->data;
		if (li->index == PASSWORD_LIST_ITEM) {
			li->entry_str = _population_password_str(EMAIL_GET_INCOMING_PASSWORD_LENGTH);
			elm_genlist_item_update(li->gl_it);
			break;
		}
		l = g_slist_next(l);
	}

	DELETE_EVAS_OBJECT(module->popup);
	DELETE_TIMER_OBJECT(view->edit_vc_timer);

	_update_account_info(view);
	return ECORE_CALLBACK_CANCEL;
}

static char *_gl_ef_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	ListItemData *li = data;

	if (li) {
		if (!strcmp("elm.text", part)) {
			if (li->index == ACCOUNT_NAME_LIST_ITEM)
				return strdup(email_setting_gettext(EMAIL_SETTING_STRING_ACCOUNT_NAME));
			else if (li->index == DISPLAY_NAME_LIST_ITEM)
				return strdup(email_setting_gettext(EMAIL_SETTING_STRING_SENDER_NAME));
		} else if (!strcmp(part, "elm.text.sub")) {
			if (li->index == ACCOUNT_NAME_LIST_ITEM)
				return elm_entry_utf8_to_markup(account_data->account_name);
			else if (li->index == DISPLAY_NAME_LIST_ITEM)
				return elm_entry_utf8_to_markup(account_data->user_display_name);
		}
	}
	return NULL;
}

static char *_gl_sig_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	ListItemData *li = data;

	if (li) {
		if (li->index == SIGNATURE_LIST_ITEM) {
			if (!strcmp(part, "elm.text"))
				return strdup(email_setting_gettext(EMAIL_SETTING_STRING_SIGNATURE));
		} else if (li->index == NOTIFICATION_LIST_ITEM) {
			if (!strcmp(part, "elm.text"))
				return strdup(email_setting_gettext(EMAIL_SETTING_STRING_NOTIFICATION_SETTINGS));
		} else if (li->index == SERVER_SETTING_LIST_ITEM) {
			if (!strcmp(part, "elm.text"))
				return strdup(email_setting_gettext(EMAIL_SETTING_STRING_SERVER_SETTINGS));
		} else if (li->index == PASSWORD_LIST_ITEM) {
			if (!strcmp(part, "elm.text"))
				return strdup(email_setting_gettext(EMAIL_SETTING_STRING_PWD));
		}
	}
	return NULL;
}

static Evas_Object *_gl_onoff_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
	ListItemData *li = data;

	if (!strcmp(part, "elm.swallow.end")) {
		Evas_Object *check = elm_check_add(obj);
		elm_object_style_set(check, "on&off");
		elm_check_state_pointer_set(check, &(li->is_checked));
		evas_object_propagate_events_set(check, EINA_FALSE);
		li->check = check;
		evas_object_smart_callback_add(check, "changed", _onoff_cb, li);
		return check;
	}

	return NULL;
}

static Evas_Object *_gl_sync_onoff_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
	retvm_if(!account_data, NULL, "account_data is null");

	ListItemData *li = data;
	EmailSettingView *view = li->view;

	if (!strcmp(part, "elm.swallow.end")) {
		Evas_Object *check = elm_check_add(obj);
		elm_object_style_set(check, "on&off");
		evas_object_smart_callback_add(check, "changed", _sync_onoff_cb, view);
		evas_object_propagate_events_set(check, EINA_FALSE);
		elm_check_state_set(check, account_data->sync_disabled ? EINA_FALSE : EINA_TRUE);

		GSList *l = view->list_items;
		while (l) {
			ListItemData *_li = l->data;
			if (_li->index == SYNC_SCHEDULE_LIST_ITEM || _li->index == WHILE_ROAMING_LIST_ITEM) {
				elm_object_item_disabled_set(_li->gl_it,
						account_data->sync_disabled ? EINA_TRUE : EINA_FALSE);
			}
			l = g_slist_next(l);
		}
		li->check = check;
		return check;
	}

	return NULL;
}

static Evas_Object *_gl_ex_roaming_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
	retvm_if(!account_data, NULL, "account_data is null");

	int index = (int)(ptrdiff_t)data;
	EmailSettingView *view = g_vd;

	if (!strcmp(part, "elm.swallow.end")) {
		Evas_Object *radio = elm_radio_add(obj);
		elm_radio_group_add(radio, view->roaming_radio_grp);
		elm_radio_state_value_set(radio, index);

		if (account_data->roaming_option == EMAIL_ROAMING_OPTION_RESTRICTED_BACKGROUND_TASK)
			elm_radio_value_set(view->roaming_radio_grp, 0);
		else if (account_data->roaming_option == EMAIL_ROAMING_OPTION_UNRESTRICTED)
			elm_radio_value_set(view->roaming_radio_grp, 1);
		evas_object_propagate_events_set(radio, EINA_FALSE);
		evas_object_smart_callback_add(radio, "changed", _gl_ex_roaming_radio_cb, (void *)(ptrdiff_t)index);
		return radio;
	}

	return NULL;
}

static Evas_Object *_gl_ex_sync_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
	retvm_if(!account_data, NULL, "account data is null");

	int index = (int)(ptrdiff_t)data;
	EmailSettingView *view = g_vd;

	if (!strcmp(part, "elm.swallow.end")) {
		Evas_Object *radio = elm_radio_add(obj);
		if (index < SYNC_SCHEDULE_ITEM_COUNT) { /* sync schedule */
			elm_radio_group_add(radio, view->sync_radio_grp);
			elm_radio_state_value_set(radio, sync_schedule[index]);

			if (sync_schedule[index] == account_data->check_interval) {
				elm_radio_value_set(view->sync_radio_grp, sync_schedule[index]);
			}
		}
		evas_object_propagate_events_set(radio, EINA_FALSE);
		evas_object_smart_callback_add(radio, "changed", _gl_ex_sync_radio_cb, (void *)(ptrdiff_t)index);
		return radio;
	}
	return NULL;
}

static char *_gl_onoff_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	ListItemData *li = data;

	if (!strcmp(part, "elm.text")) {
		char buf[MAX_STR_LEN] = { 0, };

		if (li->index == SHOW_IMAGE_LIST_ITEM) {
			snprintf(buf, sizeof(buf), "%s", email_setting_gettext(EMAIL_SETTING_STRING_DISPLAY_IMAGES));
		}
		return strdup(buf);
	}

	return NULL;
}

static char *_gl_wifi_autodownload_onoff_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	ListItemData *li = data;

	if (!strcmp(part, "elm.text.multiline")) {
		char buf[MAX_STR_LEN] = { 0, };
		int font_size = 0;
		Eina_Bool res = edje_text_class_get("list_item", NULL, &font_size);
		if (!res) {
			debug_log("Failed to get text size from text_class");
			font_size = TEXT_ITEM_DEFAULT_SIZE;
		} else if (font_size < 0) {
			font_size = -font_size * TEXT_ITEM_DEFAULT_SIZE / 100;
		}

		if (li->index == WIFI_AUTODOWNLOAD_LIST_ITEM) {
			snprintf(buf, sizeof(buf), WIFI_AUTODOWNLOAD_TEXT_STYLE, font_size, email_setting_gettext(EMAIL_SETTING_STRING_WIFI_AUTODOWNLOAD));
		}
		return strdup(buf);
	}

	return NULL;
}

static void _gl_wifi_autodownload_text_size_change_cb(system_settings_key_e key, void *user_data)
{
	retm_if(!user_data, "user_data is null");
	Elm_Object_Item *gl_item = (Elm_Object_Item *) user_data;
	elm_genlist_item_update(gl_item);
}

static char *_gl_index_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	ListItemData *li = data;

	if (!li)
		return NULL;

	if (li->index == EMAIL_SETTING_TITLE_LIST_ITEM && !g_strcmp0(part, "elm.text")) {
		return g_strdup(email_setting_gettext(EMAIL_SETTING_STRING_EMAIL_SETTINGS));
	} else if (li->index == SYNC_SETTING_TITLE_LIST_ITEM && !g_strcmp0(part, "elm.text")) {
		return g_strdup(email_setting_gettext(EMAIL_SETTING_STRING_SYNC_SETTINGS));
	}

	return NULL;
}

static char *_gl_sync_onoff_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	retvm_if(!account_data, NULL, "account data is null");
	if (!strcmp(part, "elm.text")) {
		return strdup(email_setting_gettext(EMAIL_SETTING_STRING_EMAIL_SYNC));
	} else if (!strcmp(part, "elm.text.sub")) {
		email_mailbox_t *mailbox = NULL;
		EmailSettingView *view = g_vd;
		retvm_if(!view, NULL, "view data is null");
		if(view->syncing) {
			return strdup(email_setting_gettext(EMAIL_SETTING_STRING_SYNCING_ONGOING));
		} else if(view->sync_status_failed && !view->sync_status_canceled) {
			char sync_failed_text[MAX_STR_LEN] = {0,};
			snprintf(sync_failed_text, sizeof(sync_failed_text), SYNC_FAILED_TEXT_STYLE, email_setting_gettext(EMAIL_SETTING_STRING_SYNCING_FAILED));
			return strdup(sync_failed_text);
		} else {
			if (email_engine_get_mailbox_by_mailbox_type(account_data->account_id, EMAIL_MAILBOX_TYPE_INBOX, &mailbox)) {
				bool is_24hour = false;
				const char *skeleton = NULL;
				system_settings_get_value_bool(SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR, &is_24hour);
				skeleton = ((!is_24hour) ?
					EMAIL_SETTING_DATETIME_FORMAT_12 : EMAIL_SETTING_DATETIME_FORMAT_24);
				char *date_string = setting_get_datetime_format_text(EMAIL_SETTING_DATETIME_FORMAT, &(mailbox->last_sync_time));
				char *time_string = setting_get_datetime_format_text(skeleton, &(mailbox->last_sync_time));

				char datetime_string[MAX_STR_LEN] = {0,};
				snprintf(datetime_string, sizeof(datetime_string), "%s %s", date_string, time_string);
				email_engine_free_mailbox_list(&mailbox, 1);
				if (STR_VALID(date_string) || STR_VALID(time_string)) {
					FREE(date_string);
					FREE(time_string);
					return strdup(datetime_string);
				} else {
					FREE(date_string);
					FREE(time_string);
					return strdup(email_setting_gettext(EMAIL_SETTING_STRING_NOT_SYNCED_YET));
				}
			} else {
				debug_warning("email_engine_get_mailbox_by_mailbox_type failed");
				return g_strdup(email_setting_gettext(EMAIL_SETTING_STRING_NOT_SYNCED_YET));
			}
		}
	}

	return NULL;
}

static char *_gl_sync_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	retvm_if(!account_data, NULL, "account_data is null");

	if (!strcmp("elm.text", part)) {
		return strdup(email_setting_gettext(EMAIL_SETTING_STRING_EMAIL_SYNC_SCHEDULE));
	} else if (!strcmp("elm.text.sub", part)) {
		char buf[MAX_STR_LEN] = { 0, };
		int period = account_data->check_interval;
		if (period == 0) {
			snprintf(buf, sizeof(buf), "%s", email_setting_gettext(EMAIL_SETTING_STRING_PUSH));
		} else if (period <= -1) {
			snprintf(buf, sizeof(buf), "%s", email_setting_gettext(EMAIL_SETTING_STRING_MANUAL));
		} else if (sync_schedule[2] <= period && period <= sync_schedule[3]) {
			snprintf(buf, sizeof(buf), email_setting_gettext(EMAIL_SETTING_STRING_PD_MIN), period);
		} else if (period == sync_schedule[4]) {
			snprintf(buf, sizeof(buf), "%s", email_setting_gettext(EMAIL_SETTING_STRING_E_1H));
		} else if (period == sync_schedule[5]) {
			snprintf(buf, sizeof(buf), email_setting_gettext(EMAIL_SETTING_STRING_PD_H), 3);
		} else if (period == sync_schedule[6]) {
			snprintf(buf, sizeof(buf), email_setting_gettext(EMAIL_SETTING_STRING_PD_H), 6);
		} else if (period == sync_schedule[7]) {
			snprintf(buf, sizeof(buf), "%s", email_setting_gettext(EMAIL_SETTING_STRING_EVERY_DAY));
		}
		return strdup(buf);
	}
	return NULL;
}

static char *_gl_roaming_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	debug_enter();

	retvm_if(!account_data, NULL, "account_data is null");

	if (!strcmp(part, "elm.text")) {
		return strdup(email_setting_gettext(EMAIL_SETTING_STRING_WHILE_ROAMING));
	} else if (!strcmp(part, "elm.text.sub")) {
		if (account_data->roaming_option == EMAIL_ROAMING_OPTION_RESTRICTED_BACKGROUND_TASK)
			return strdup(email_setting_gettext(EMAIL_SETTING_STRING_MANUAL));
		else if (account_data->roaming_option == EMAIL_ROAMING_OPTION_UNRESTRICTED)
			return strdup(email_setting_gettext(EMAIL_SETTING_STRING_USE_ABOVE_SETTINGS));
	}

	return NULL;
}

static char *_gl_ex_roaming_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	int index = (int)(ptrdiff_t)data;

	if (!strcmp(part, "elm.text")) {
		if (index == 0) {
			return strdup(email_setting_gettext(EMAIL_SETTING_STRING_MANUAL));
		} else if (index == 1) {
			return strdup(email_setting_gettext(EMAIL_SETTING_STRING_USE_ABOVE_SETTINGS));
		}

		return NULL;
	}

	return NULL;
}

static char *_gl_ex_sync_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	int index = (int)(ptrdiff_t)data;

	if (!strcmp(part, "elm.text")) {
		char buf[MAX_STR_LEN] = { 0, };

		if (index == 0) {
			snprintf(buf, sizeof(buf), "%s", email_setting_gettext(EMAIL_SETTING_STRING_PUSH));
		} else if (index == 1) {
			snprintf(buf, sizeof(buf), "%s", email_setting_gettext(EMAIL_SETTING_STRING_MANUAL));
		} else if (2 <= index && index <= 3) {
			snprintf(buf, sizeof(buf), email_setting_gettext(EMAIL_SETTING_STRING_PD_MIN), sync_schedule[index]);
		} else if (index == 4) {
			snprintf(buf, sizeof(buf), "%s", email_setting_gettext(EMAIL_SETTING_STRING_E_1H));
		} else if (index == 5) {
			snprintf(buf, sizeof(buf), email_setting_gettext(EMAIL_SETTING_STRING_PD_H), 3);
		} else if (index == 6) {
			snprintf(buf, sizeof(buf), email_setting_gettext(EMAIL_SETTING_STRING_PD_H), 6);
		} else if (index == 7) {
			snprintf(buf, sizeof(buf), "%s", email_setting_gettext(EMAIL_SETTING_STRING_EVERY_DAY));
		}

		return strdup(buf);
	}

	return NULL;
}

static Evas_Object *_get_option_genlist(Evas_Object *parent, ListItemData *li)
{
	debug_enter();
	Evas_Object *genlist = NULL;
	int i = 0;

	if (li) {
		EmailSettingView *view = li->view;
		genlist = elm_genlist_add(parent);
		elm_genlist_homogeneous_set(genlist, EINA_TRUE);
		elm_scroller_policy_set(genlist, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
		elm_scroller_content_min_limit(genlist, EINA_FALSE, EINA_TRUE);

		if (li->index == WHILE_ROAMING_LIST_ITEM) {
			for (i = 0; i < 2; i++) {
				elm_genlist_item_append(genlist, view->itc_ex_roaming, (void *)(ptrdiff_t)i,
						NULL, ELM_GENLIST_ITEM_NONE, _gl_ex_roaming_sel_cb, (void *)(ptrdiff_t)i);
			}
		} else if (li->index == SYNC_SCHEDULE_LIST_ITEM) {
			for (i = 0; i < SYNC_SCHEDULE_ITEM_COUNT; i++) {
				if (i == 0 && account_data->incoming_server_type == EMAIL_SERVER_TYPE_POP3)
					continue;
				if (i == 0 && !view->is_imap_push_supported)
					continue;
				elm_genlist_item_append(genlist, view->itc_ex_sync, (void *)(ptrdiff_t)i,
						NULL, ELM_GENLIST_ITEM_NONE, _gl_ex_sync_sel_cb, (void *)(ptrdiff_t)i);
			}
		} else if (li->index == EMAIL_SIZE_LIST_ITEM) {
			for (i = 0; i < 10; i++) {
				elm_genlist_item_append(genlist, view->itc_ex_size, (void *)(ptrdiff_t)i,
						NULL, ELM_GENLIST_ITEM_NONE, _gl_ex_size_sel_cb, (void *)(ptrdiff_t)i);
			}
		}
		evas_object_show(genlist);

		return genlist;
	}
	return NULL;
}

static void _create_toolbar_more_btn(EmailSettingView *view)
{
	debug_enter();

	Evas_Object *sync_btn = NULL;
	Evas_Object *remove_btn = NULL;
	Evas_Object *btn_ly = NULL;
	btn_ly = elm_layout_add(view->base.content);
	elm_layout_file_set(btn_ly, email_get_setting_theme_path(), "two_bottom_btn");

	sync_btn = elm_button_add(btn_ly);
	elm_object_style_set(sync_btn, "bottom");

	evas_object_smart_callback_add(sync_btn, "clicked", _sync_toolbar_btn_cb, view);
	elm_layout_content_set(btn_ly, "btn1.swallow", sync_btn);
	view->sync_btn = sync_btn;
	_update_toolbar_sync_button_state(view);

	remove_btn = elm_button_add(btn_ly);
	elm_object_style_set(remove_btn, "bottom");
	elm_object_domain_translatable_text_set(remove_btn, EMAIL_SETTING_STRING_DELETE_ACCOUNT.domain, EMAIL_SETTING_STRING_DELETE_ACCOUNT.id);
	evas_object_smart_callback_add(remove_btn, "clicked", _delete_account_cb, view);
	elm_layout_content_set(btn_ly, "btn2.swallow", remove_btn);
	view->remove_btn = remove_btn;

	elm_object_item_part_content_set(view->base.navi_item, "toolbar", btn_ly);
	evas_object_show(view->base.content);

	debug_leave();
}

static void _update_toolbar_sync_button_state(EmailSettingView *view)
{
	retm_if(!view->sync_btn,  "Sync button is null");
	debug_enter();
	if (!view->syncing) {
		elm_object_domain_translatable_text_set(view->sync_btn, EMAIL_SETTING_STRING_REFRESH.domain, EMAIL_SETTING_STRING_REFRESH.id);
		elm_object_disabled_set(view->sync_btn, account_data->sync_disabled ? EINA_TRUE : EINA_FALSE);
	} else {
		elm_object_domain_translatable_text_set(view->sync_btn, EMAIL_SETTING_STRING_CANCEL.domain, EMAIL_SETTING_STRING_CANCEL.id);
	}

	debug_leave();
}

static void _delete_account_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailSettingView *view = data;
	retm_if(!view, "view data is null");

	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	module->account_id = account_data->account_id;

	DELETE_EVAS_OBJECT(module->popup);
	email_string_t temp;
	if (account_data->incoming_server_authentication_method == EMAIL_AUTHENTICATION_METHOD_XOAUTH2) {
		temp = EMAIL_SETTING_STRING_GOOGLE_ACCOUNT_DELETED;
	} else {
		temp = EMAIL_SETTING_STRING_ACCOUNT_DELETED;
	}
	module->popup = setting_get_notify(&view->base, &(EMAIL_SETTING_STRING_HEADER_DELETE),
			&(temp), 2,
			&(EMAIL_SETTING_STRING_CANCEL),
			_popup_cancel_cb,
			&(EMAIL_SETTING_STRING_DELETE),
			_popup_delete_ok_cb);
}

static void _popup_cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "data is null");

	EmailSettingView *view = data;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	DELETE_EVAS_OBJECT(module->popup);
}

static void _popup_delete_ok_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "data is null");

	EmailSettingView *view = data;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	int account_id = module->account_id;
	int account_svc_id = account_data->account_svc_id;

	DELETE_EVAS_OBJECT(module->popup);

	/* delete account from account_svc */
	int ret = setting_delete_account_from_account_svc(account_svc_id);
	if (ret == ACCOUNT_ERROR_NONE) {
		/* delete account from email_svc */
		module->is_account_deleted_on_this = EINA_TRUE;
		ret = email_engine_delete_account(account_id);
		if (ret) {
			module->popup = setting_get_pb_process_notify(&view->base,
					&(EMAIL_SETTING_STRING_DELETING), 0,
					NULL, NULL,
					NULL, NULL,
					POPUP_BACK_TYPE_NO_OP, NULL);
			DELETE_TIMER_OBJECT(view->del_timer);
			view->del_timer = ecore_timer_add(3.0, _after_delete_cb, view);
			debug_log("delete success");
		} else {
			module->is_account_deleted_on_this = EINA_FALSE;
			module->popup = setting_get_notify(&view->base, &(EMAIL_SETTING_STRING_HEADER_DELETE),
					&(EMAIL_SETTING_STRING_UNABLE_DELETE), 1,
					&(EMAIL_SETTING_STRING_OK),
					_popup_cancel_cb, NULL, NULL);
			debug_log("delete failed");
		}
	} else {
		debug_error("setting_delete_account_from_account_svc failed: %d", ret);
		module->popup = setting_get_notify(&view->base, &(EMAIL_SETTING_STRING_HEADER_DELETE),
				&(EMAIL_SETTING_STRING_UNABLE_DELETE), 1,
				&(EMAIL_SETTING_STRING_OK),
				_popup_cancel_cb, NULL, NULL);
		debug_log("delete failed");
	}
}

static Eina_Bool _after_delete_cb(void *data)
{
	debug_enter();

	EmailSettingView *view = data;

	retvm_if(!view, ECORE_CALLBACK_CANCEL, "view data is null");

	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	DELETE_TIMER_OBJECT(view->del_timer);
	DELETE_EVAS_OBJECT(module->popup);

	if (module->base.views_count > 1) {
		setting_update_acct_list(&view->base);
	}

	email_module_exit_view(&view->base);

	return ECORE_CALLBACK_CANCEL;
}

static void _popup_ok_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	if (!data) {
		debug_error("data is NULL");
		return;
	}

	EmailSettingView *view = data;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	DELETE_EVAS_OBJECT(module->popup);
}

static void _account_popup_name_done_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "data is null");

	EmailSettingView *view = data;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	Evas_Object *entry_layout = elm_object_content_get(module->popup);
	Evas_Object *entry = elm_layout_content_get(entry_layout, "elm.swallow.content");

	char *val = setting_get_entry_str(entry);
	if (val && strlen(val) > 0) {
		FREE(account_data->account_name);
		if (!(account_data->account_name = strdup(val))) {
			debug_error("strdup() to account_data->account_name - failed error ");
			free(val);
			return;
		}

		_update_account_info(view);
		GSList *l = view->list_items;
		while (l) {
			ListItemData *li = l->data;
			if (li->index == ACCOUNT_NAME_LIST_ITEM) {
				li->entry_str = elm_entry_utf8_to_markup(account_data->account_name);
				elm_genlist_item_update(li->gl_it);
				li->entry = NULL;
				break;
			}
			l = g_slist_next(l);
		}
		DELETE_EVAS_OBJECT(module->popup);
	}
	free(val);
}

static void _account_popup_sender_name_done_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "data is null");

	EmailSettingView *view = data;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	Evas_Object *entry_layout = elm_object_content_get(module->popup);
	Evas_Object *entry = elm_layout_content_get(entry_layout, "elm.swallow.content");

	char *val = setting_get_entry_str(entry);
	if (val && strlen(val) > 0) {
		FREE(account_data->user_display_name);
		if (!(account_data->user_display_name = strdup(val))) {
			debug_error("strdup() to account_data->user_display_name - failed error ");
			free(val);
			return;
		}

		_update_account_info(view);
		GSList *l = view->list_items;
		while (l) {
			ListItemData *li = l->data;
			if (li->index == DISPLAY_NAME_LIST_ITEM) {
				li->entry_str = elm_entry_utf8_to_markup(account_data->user_display_name);
				elm_genlist_item_update(li->gl_it);
				li->entry = NULL;
				break;
			}
			l = g_slist_next(l);
		}
		DELETE_EVAS_OBJECT(module->popup);
	}
	free(val);
}

static void _account_popup_entry_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "data is null");

	EmailSettingView *view = data;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	Evas_Object *entry_layout = elm_object_content_get(module->popup);
	Evas_Object *entry = elm_layout_content_get(entry_layout, "elm.swallow.content");

	char *val = setting_get_entry_str(entry);
	if (val && strlen(val) > 0) {
		elm_entry_input_panel_return_key_disabled_set(obj, EINA_FALSE);
		elm_object_disabled_set(evas_object_data_get(obj, EMAIL_SETTING_POPUP_BUTTON), EINA_FALSE);
	} else {
		elm_entry_input_panel_return_key_disabled_set(obj, EINA_TRUE);
		elm_object_disabled_set(evas_object_data_get(obj, EMAIL_SETTING_POPUP_BUTTON), EINA_TRUE);
	}
	free(val);
	debug_leave();
}

static void _account_popup_password_done_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "data is null");

	EmailSettingView *view = data;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	Evas_Object *entry_layout = elm_object_content_get(module->popup);
	Evas_Object *entry = elm_layout_content_get(entry_layout, "elm.swallow.content");

	char *val = setting_get_entry_str(entry);
	if (val && strlen(val) > 0) {
		FREE(account_data->incoming_server_password);
		if (!(account_data->incoming_server_password = strdup(val))) {
			debug_error("strdup() to account_data->incoming_server_password - failed error ");
			free(val);
			return;
		}

		GSList *l = view->list_items;
		while (l) {
			ListItemData *li = l->data;
			if (li->index == PASSWORD_LIST_ITEM) {
				li->entry = NULL;
				break;
			}
			l = g_slist_next(l);
		}
		DELETE_TIMER_OBJECT(view->validate_timer);
		view->validate_timer = ecore_timer_add(0.5, _validate_account, view);
		DELETE_EVAS_OBJECT(module->popup);
	}
	free(val);
	debug_leave();
}

static void _account_popup_cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "data is null");

	EmailSettingView *view = data;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	GSList *l = view->list_items;
	while (l) {
		ListItemData *li = l->data;
		if (li->entry) {
			li->entry = NULL;
			break;
		}
		l = g_slist_next(l);
	}

	DELETE_EVAS_OBJECT(module->popup);
}

static char *_gl_size_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	if (!strcmp(part, "elm.text")) {
		return strdup(email_setting_gettext(EMAIL_SETTING_STRING_EMAIL_SIZE));
	} else if (!strcmp(part, "elm.text.sub")) {
		char buf[MAX_STR_LEN] = { 0, };
		if (account_data->auto_download_size == 0) {
			return strdup(email_setting_gettext(EMAIL_SETTING_STRING_HEADER_ONLY));
		} else if (account_data->auto_download_size == EMAIL_SIZE_NO_LIMIT) {
			snprintf(buf, sizeof(buf), "%s", email_setting_gettext(EMAIL_SETTING_STRING_ENTIRE_EMAIL));
		} else if (account_data->auto_download_size == 1024/2) {
			snprintf(buf, sizeof(buf), "0.5 KB");
		} else {
			snprintf(buf, sizeof(buf), "%d KB", account_data->auto_download_size/1024);
		}
		return strdup(buf);
	}
	return NULL;
}

static char *_gl_ex_size_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	int index = (int)(ptrdiff_t)data;

	if (!strcmp(part, "elm.text")) {
		char buf[MAX_STR_LEN] = { 0, };

		if (index == 0) {
			snprintf(buf, sizeof(buf), "%s", email_setting_gettext(EMAIL_SETTING_STRING_HEADER_ONLY));
		} else if (index == 1) {
			snprintf(buf, sizeof(buf), "0.5 KB");
		} else if (index == 2) {
			snprintf(buf, sizeof(buf), "1 KB");
		} else if (index == 3) {
			snprintf(buf, sizeof(buf), "2 KB");
		} else if (index == 4) {
			snprintf(buf, sizeof(buf), "5 KB");
		} else if (index == 5) {
			snprintf(buf, sizeof(buf), "10 KB");
		} else if (index == 6) {
			snprintf(buf, sizeof(buf), "20 KB");
		} else if (index == 7) {
			snprintf(buf, sizeof(buf), "50 KB");
		} else if (index == 8) {
			snprintf(buf, sizeof(buf), "100 KB");
		} else if (index == 9) {
			snprintf(buf, sizeof(buf), "%s", email_setting_gettext(EMAIL_SETTING_STRING_ENTIRE_EMAIL));
		}

		return strdup(buf);
	}

	return NULL;
}

static Evas_Object *_gl_ex_size_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
	int index = (int)(ptrdiff_t)data;
	EmailSettingView *view = g_vd;

	if (!strcmp(part, "elm.swallow.end")) {
		Evas_Object *radio = elm_radio_add(obj);
		elm_radio_group_add(radio, view->size_radio_grp);
		elm_radio_state_value_set(radio, index);

		if (index == 0) {
			if (account_data->auto_download_size == 0) {
				elm_radio_value_set(view->size_radio_grp, 0);
			} else if (account_data->auto_download_size == 1024/2) {
				elm_radio_value_set(view->size_radio_grp, 1);
			} else if (account_data->auto_download_size == 1024) {
				elm_radio_value_set(view->size_radio_grp, 2);
			} else if (account_data->auto_download_size == 1024*2) {
				elm_radio_value_set(view->size_radio_grp, 3);
			} else if (account_data->auto_download_size == 1024*5) {
				elm_radio_value_set(view->size_radio_grp, 4);
			} else if (account_data->auto_download_size == 1024*10) {
				elm_radio_value_set(view->size_radio_grp, 5);
			} else if (account_data->auto_download_size == 1024*20) {
				elm_radio_value_set(view->size_radio_grp, 6);
			} else if (account_data->auto_download_size == 1024*50) {
				elm_radio_value_set(view->size_radio_grp, 7);
			} else if (account_data->auto_download_size == 1024*100) {
				elm_radio_value_set(view->size_radio_grp, 8);
			} else if (account_data->auto_download_size == EMAIL_SIZE_NO_LIMIT) {
				elm_radio_value_set(view->size_radio_grp, 9);
			}
		}
		evas_object_propagate_events_set(radio, EINA_FALSE);
		evas_object_smart_callback_add(radio, "changed", _gl_ex_size_radio_cb, (void *)(ptrdiff_t)index);
		return radio;
	}

	return NULL;
}

static void _gl_ex_size_radio_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	int index = (int)(ptrdiff_t)data;
	EmailSettingView *view = g_vd;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;


	switch (index) {
		case 0:
			account_data->auto_download_size = 0;
			break;
		case 1:
			account_data->auto_download_size = 1024/2;
			break;
		case 2:
			account_data->auto_download_size = 1024;
			break;
		case 3:
			account_data->auto_download_size = 1024*2;
			break;
		case 4:
			account_data->auto_download_size = 1024*5;
			break;
		case 5:
			account_data->auto_download_size = 1024*10;
			break;
		case 6:
			account_data->auto_download_size = 1024*20;
			break;
		case 7:
			account_data->auto_download_size = 1024*50;
			break;
		case 8:
			account_data->auto_download_size = 1024*100;
			break;
		case 9:
			account_data->auto_download_size = EMAIL_SIZE_NO_LIMIT;
			break;
		default:
			debug_log("Unknown size value: %d", index);
			break;
	}

	_update_account_info(view);

	GSList *l = view->list_items;
	while (l) {
		ListItemData *li = l->data;
		if (li->index == EMAIL_SIZE_LIST_ITEM) {
			elm_genlist_item_update(li->gl_it);
			break;
		}
		l = g_slist_next(l);
	}
	DELETE_EVAS_OBJECT(module->popup);
}

static void _gl_ex_size_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	int index = (int)(ptrdiff_t)data;
	EmailSettingView *view = g_vd;


	Elm_Object_Item *item = event_info;
	elm_genlist_item_selected_set(item, EINA_FALSE);
	elm_radio_value_set(view->size_radio_grp, index);

	_gl_ex_size_radio_cb((void *)(ptrdiff_t)index, NULL, NULL);
}

static void _show_finished_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	ListItemData *li = data;
	int selected_index = 0;
	EmailSettingView *view = li->view;

	if (li->index == WHILE_ROAMING_LIST_ITEM) {
		selected_index = elm_radio_value_get(view->roaming_radio_grp);
	} else if (li->index == SYNC_SCHEDULE_LIST_ITEM) {
		selected_index = elm_radio_value_get(view->sync_radio_grp);
		int i;
		for (i = 0; i < SYNC_SCHEDULE_ITEM_COUNT; i++) {
			if (selected_index == sync_schedule[i]) {
				selected_index = i;
				break;
			}
		}
	} else if (li->index == EMAIL_SIZE_LIST_ITEM) {
		selected_index = elm_radio_value_get(view->size_radio_grp);
	}

	Evas_Object *genlist = elm_object_content_get(obj);
	Elm_Object_Item *it = elm_genlist_first_item_get(genlist);
	while (it) {
		int index = (int)(ptrdiff_t)elm_object_item_data_get(it);
		if (index == selected_index) {
			elm_genlist_item_show(it, ELM_GENLIST_ITEM_SCROLLTO_MIDDLE);
			break;
		}
		it = elm_genlist_item_next_get(it);
	}
}
