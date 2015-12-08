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
#include <account-types.h>
#include <metadata_extractor.h>
#include <system_settings.h>

#include "email-setting-utils.h"
#include "email-view-account-details-setup.h"
#include "email-view-account-edit.h"
#include "email-setting-noti-mgr.h"
#include "email-view-manual-setup.h"
#include "email-utils.h"

typedef struct view_data EmailSettingVD;

static email_account_t *account_data = NULL;
static EmailSettingVD *g_vd = NULL;

static int _create(email_view_t *self);
static void _destroy(email_view_t *self);
static void _update(email_view_t *self, int flags);
static void _on_back_cb(email_view_t *self);

static void _update_list(EmailSettingVD *vd);

static void _push_naviframe(EmailSettingVD *vd);
static void _create_list(EmailSettingVD *vd);

static void _cancel_cb(void *data, Evas_Object *obj, void *event_info);
static void _save_cb(void *data, Evas_Object *obj, void *event_info);

static char *_gl_ef_text_get_cb(void *data, Evas_Object *obj, const char *part);
static char *_gl_index_text_get_cb(void *data, Evas_Object *obj, const char *part);
static char *_gl_sync_onoff_text_get_cb(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_gl_sync_onoff_content_get_cb(void *data, Evas_Object *obj, const char *part);
static char *_gl_sync_text_get_cb(void *data, Evas_Object *obj, const char *part);
static char *_gl_roaming_text_get_cb(void *data, Evas_Object *obj, const char *part);
static char *_gl_ex_roaming_text_get_cb(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_gl_ex_roaming_content_get_cb(void *data, Evas_Object *obj, const char *part);
static char *_gl_ex_sync_text_get_cb(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_gl_ex_sync_content_get_cb(void *data, Evas_Object *obj, const char *part);

static void _popup_ok_cb(void *data, Evas_Object *obj, void *event_info);
static void _show_finished_cb(void *data, Evas_Object *obj, void *event_info);

/* Account popup cbs */
static void _account_popup_name_done_cb(void *data, Evas_Object *obj, void *event_info);
static void _account_popup_sender_name_done_cb(void *data, Evas_Object *obj, void *event_info);
static void _account_popup_entry_changed_cb(void *data, Evas_Object *obj, void *event_info);
static void _account_popup_cancel_cb(void *data, Evas_Object *obj, void *event_info);

static void _create_processing_popup(EmailSettingVD *vd);
static Eina_Bool _after_add_account_cb(void *data);
static void _mailbox_sync_end_cb(int account_id, email_setting_response_data *response, void *user_data);
static int _is_imap_push_supported(const email_account_t *account);
static void _gl_account_sel_cb(void *data, Evas_Object *obj, void *event_info);
static void _gl_sel_cb(void *data, Evas_Object *obj, void *event_info);
static void _gl_ex_sel_cb(void *data, Evas_Object *obj, void *event_info);
static void _sync_onoff_cb(void *data, Evas_Object *obj, void *event_info);
static void _gl_ex_roaming_sel_cb(void *data, Evas_Object *obj, void *event_info);
static void _gl_ex_roaming_radio_cb(void *data, Evas_Object *obj, void *event_info);
static void _gl_ex_sync_sel_cb(void *data, Evas_Object *obj, void *event_info);
static void _gl_ex_sync_radio_cb(void *data, Evas_Object *obj, void *event_info);

#define SYNC_TIMEOUT_SEC 3.0
#define SYNC_SCHEDULE_ITEM_COUNT 8
static int sync_schedule[SYNC_SCHEDULE_ITEM_COUNT] = { 0, -1, 10, 30, 60, 180, 360, 1440 };

static email_string_t EMAIL_SETTING_STRING_SYNC_SETTINGS = {PACKAGE, "IDS_EMAIL_TMBODY_SYNC_SETTINGS"};
static email_string_t EMAIL_SETTING_STRING_CANCEL = {PACKAGE, "IDS_EMAIL_BUTTON_CANCEL"};
static email_string_t EMAIL_SETTING_STRING_OK = {PACKAGE, "IDS_EMAIL_BUTTON_OK"};

static email_string_t EMAIL_SETTING_STRING_EMAIL = {PACKAGE, "IDS_ST_HEADER_EMAIL"};
static email_string_t EMAIL_SETTING_STRING_ACCOUNT_NAME = {PACKAGE, "IDS_ST_TMBODY_ACCOUNT_NAME"};
static email_string_t EMAIL_SETTING_STRING_SENDER_NAME = {PACKAGE, "IDS_ST_TMBODY_SENDER_NAME"};
static email_string_t EMAIL_SETTING_STRING_DONE = {PACKAGE, "IDS_EMAIL_BUTTON_DONE"};
static email_string_t EMAIL_SETTING_STRING_EMAIL_SYNC = {PACKAGE, "IDS_ST_MBODY_SYNC_EMAILS"};
static email_string_t EMAIL_SETTING_STRING_NOT_SYNCED_YET = {PACKAGE, "IDS_ST_SBODY_NOT_SYNCED_YET_M_STATUS"};
static email_string_t EMAIL_SETTING_STRING_WHILE_ROAMING = {PACKAGE, "IDS_ST_TMBODY_SYNC_WHILE_ROAMING"};
static email_string_t EMAIL_SETTING_STRING_EMAIL_SYNC_SCHEDULE = {PACKAGE, "IDS_ST_MBODY_EMAIL_SYNC_SCHEDULE"};
static email_string_t EMAIL_SETTING_STRING_E_1H = {PACKAGE, "IDS_ST_OPT_EVERY_HOUR_ABB"};
static email_string_t EMAIL_SETTING_STRING_PUSH = {PACKAGE, "IDS_ST_OPT_PUSH"};
static email_string_t EMAIL_SETTING_STRING_EVERY_DAY = {PACKAGE, "IDS_ST_OPT_EVERY_DAY"};
static email_string_t EMAIL_SETTING_STRING_PD_H = {PACKAGE, "IDS_ST_OPT_EVERY_PD_HOURS_ABB2"};
static email_string_t EMAIL_SETTING_STRING_PD_MIN = {PACKAGE, "IDS_ST_OPT_EVERY_PD_MINUTES_ABB"};
static email_string_t EMAIL_SETTING_STRING_MANUAL = {PACKAGE, "IDS_ST_OPT_MANUAL"};
static email_string_t EMAIL_SETTING_STRING_USE_ABOVE_SETTINGS = {PACKAGE, "IDS_ST_OPT_KEEP_NON_ROAMING_SETTINGS_ABB"};
static email_string_t EMAIL_SETTING_STRING_WARNING = {PACKAGE, "IDS_ST_HEADER_WARNING"};
static email_string_t EMAIL_SETTING_STRING_ALREADY_EXIST = {PACKAGE, "IDS_ST_POP_THIS_ACCOUNT_HAS_ALREADY_BEEN_ADDED"};
static email_string_t EMAIL_SETTING_STRING_UNABLE_TO_ADD_ACCOUNT = {PACKAGE, "IDS_ST_HEADER_UNABLE_TO_ADD_ACCOUNT_ABB"};
static email_string_t EMAIL_SETTING_STRING_PROCESSING = {PACKAGE, "IDS_ST_SBODY_PROCESSING_PLEASE_WAIT_ING_ABB"};

enum {
	ACCOUNT_NAME_LIST_ITEM = 1,
	DISPLAY_NAME_LIST_ITEM,
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

	Evas_Object *account_icon;

	Ecore_Timer *processing_timer;

	Evas_Object *genlist;
	Elm_Genlist_Item_Class *itc_account_details;
	Elm_Genlist_Item_Class *itc_sync_groupindex;
	Elm_Genlist_Item_Class *itc_sync_onoff;
	Elm_Genlist_Item_Class *itc_sync;
	Elm_Genlist_Item_Class *itc_roaming;
	Elm_Genlist_Item_Class *itc_ex_roaming;
	Elm_Genlist_Item_Class *itc_ex_sync;

	Evas_Object *roaming_radio_grp;
	Evas_Object *sync_radio_grp;

	int is_google_launched;
	int account_deleted;
	int is_imap_push_supported;
	int handle;
	int syncing;
	int folder_sync_handle;
	int is_add_account_finished;

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
	EmailSettingVD *vd;
} ListItemData;

static Evas_Object *_get_option_genlist(Evas_Object *parent, ListItemData *li);

void create_account_details_setup_view(EmailSettingUGD *ugd)
{
	debug_enter();
	retm_if(!ugd, "ug data is null");

	EmailSettingVD *vd = calloc(1, sizeof(EmailSettingVD));
	retm_if(!vd, "view data is null");

	vd->base.create = _create;
	vd->base.destroy = _destroy;
	vd->base.update = _update;
	vd->base.on_back_key = _on_back_cb;

	debug_log("view create result: %d", email_module_create_view(&ugd->base, &vd->base));
}

static int _create(email_view_t *self)
{
	debug_enter();
	retvm_if(!self, -1, "self is null");

	EmailSettingVD *vd = (EmailSettingVD *)self;
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;

	account_data = ugd->new_account;

	EMAIL_SETTING_PRINT_ACCOUNT_INFO(account_data);
	vd->base.content = setting_add_inner_layout(&vd->base);
	_push_naviframe(vd);

	g_vd = vd;

	if (account_data->incoming_server_type == EMAIL_SERVER_TYPE_IMAP4)
		vd->is_imap_push_supported = _is_imap_push_supported(account_data);

	_create_list(vd);

	setting_noti_subscribe(ugd, EMAIL_SETTING_SYNC_FOLDER_NOTI, _mailbox_sync_end_cb, vd);

	_update_list(vd);

	return 0;
}

static void _update(email_view_t *self, int flags)
{
	debug_enter();
	retm_if(!self, "self is null");

	EmailSettingVD *vd = (EmailSettingVD *)self;

	if (flags & EVUF_LANGUAGE_CHANGED) {
		/* refreshing genlist. */
		elm_genlist_realized_items_update(vd->genlist);
	}

	if (flags & (EVUF_WAS_PAUSED | EVUF_POPPING)) {
		_update_list(vd);
	}
}

static void _update_list(EmailSettingVD *vd)
{
	debug_enter();
	retm_if(!vd, "view data is null");

	/* read account info again. */
	if (account_data) {
		genlist_item_updated[SYNC_ONOFF_LIST_ITEM-1] = !account_data->sync_disabled;

		EMAIL_SETTING_PRINT_ACCOUNT_INFO(account_data);

		if (!account_data->sync_disabled &&
			(account_data->sync_status & SYNC_STATUS_SYNCING)) {
			vd->syncing = 1;
		} else {
			vd->syncing = 0;
		}
	}

	GSList *l = vd->list_items;
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
	retm_if(!self, "self is null");

	EmailSettingVD *vd = (EmailSettingVD *)self;
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;

	setting_noti_unsubscribe(ugd, EMAIL_SETTING_SYNC_FOLDER_NOTI, _mailbox_sync_end_cb);
	DELETE_TIMER_OBJECT(vd->processing_timer);

	GSList *l = vd->list_items;
	while (l) {
		ListItemData *li = l->data;
		FREE(li->entry_str);
		FREE(li->entry_limit);
		FREE(li);
		l = g_slist_next(l);
	}
	g_slist_free(vd->list_items);

	l = vd->itc_list;
	while (l) {
		Elm_Genlist_Item_Class *itc = l->data;
		EMAIL_GENLIST_ITC_FREE(itc);
		l = g_slist_next(l);
	}
	g_slist_free(vd->itc_list);

	free(vd);
}

static void _push_naviframe(EmailSettingVD *vd)
{
	debug_enter();
	Evas_Object *next_btn = NULL;
	Evas_Object *cancel_btn = NULL;
	email_module_view_push(&vd->base, EMAIL_SETTING_STRING_EMAIL.id, 0);
	elm_object_item_domain_text_translatable_set(vd->base.navi_item, EMAIL_SETTING_STRING_EMAIL.domain, EINA_TRUE);

	if (account_data->sync_status & SYNC_STATUS_SYNCING) {
		vd->syncing = 1;
	}
	evas_object_show(vd->base.module->navi);

	Evas_Object *btn_ly = NULL;
	btn_ly = elm_layout_add(vd->base.content);
	elm_layout_file_set(btn_ly, email_get_setting_theme_path(), "two_bottom_btn");

	cancel_btn = elm_button_add(vd->base.module->navi);
	elm_object_style_set(cancel_btn, "bottom");
	elm_object_domain_translatable_text_set(cancel_btn, EMAIL_SETTING_STRING_CANCEL.domain, EMAIL_SETTING_STRING_CANCEL.id);
	evas_object_smart_callback_add(cancel_btn, "clicked", _cancel_cb, vd);
	elm_layout_content_set(btn_ly, "btn1.swallow", cancel_btn);

	next_btn = elm_button_add(vd->base.module->navi);
	elm_object_style_set(next_btn, "bottom");
	elm_object_domain_translatable_text_set(next_btn, EMAIL_SETTING_STRING_DONE.domain, EMAIL_SETTING_STRING_DONE.id);
	evas_object_smart_callback_add(next_btn, "clicked", _save_cb, vd);
	elm_layout_content_set(btn_ly, "btn2.swallow", next_btn);

	elm_object_item_part_content_set(vd->base.navi_item, "toolbar", btn_ly);
}

static void _create_list(EmailSettingVD *vd)
{
	debug_enter();

	retm_if(!vd, "view data is null");

	Elm_Object_Item *git = NULL;
	ListItemData *li = NULL;
	Elm_Object_Item *item = NULL;

	vd->genlist = elm_genlist_add(vd->base.content);
	elm_scroller_policy_set(vd->genlist, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
	elm_object_style_set(vd->genlist, "dialogue");
	elm_genlist_mode_set(vd->genlist, ELM_LIST_COMPRESS);

	vd->roaming_radio_grp = elm_radio_add(vd->base.content);
	elm_radio_value_set(vd->roaming_radio_grp, -1);
	evas_object_hide(vd->roaming_radio_grp);

	vd->sync_radio_grp = elm_radio_add(vd->base.content);
	elm_radio_value_set(vd->sync_radio_grp, -1);
	evas_object_hide(vd->sync_radio_grp);


	vd->itc_account_details = setting_get_genlist_class_item("type1", _gl_ef_text_get_cb, NULL, NULL, NULL);
	vd->itc_sync_groupindex = setting_get_genlist_class_item("group_index", _gl_index_text_get_cb, NULL, NULL, NULL);
	vd->itc_sync_onoff = setting_get_genlist_class_item("type1", _gl_sync_onoff_text_get_cb, _gl_sync_onoff_content_get_cb, NULL, NULL);
	vd->itc_sync = setting_get_genlist_class_item("type1", _gl_sync_text_get_cb, NULL, NULL, NULL);
	vd->itc_roaming = setting_get_genlist_class_item("type1", _gl_roaming_text_get_cb, NULL, NULL, NULL);
	vd->itc_ex_roaming = setting_get_genlist_class_item("type1", _gl_ex_roaming_text_get_cb, _gl_ex_roaming_content_get_cb, NULL, NULL);
	vd->itc_ex_sync = setting_get_genlist_class_item("type1", _gl_ex_sync_text_get_cb, _gl_ex_sync_content_get_cb, NULL, NULL);

	vd->itc_list = g_slist_append(vd->itc_list, vd->itc_account_details);
	vd->itc_list = g_slist_append(vd->itc_list, vd->itc_sync_groupindex);
	vd->itc_list = g_slist_append(vd->itc_list, vd->itc_sync_onoff);
	vd->itc_list = g_slist_append(vd->itc_list, vd->itc_sync);
	vd->itc_list = g_slist_append(vd->itc_list, vd->itc_ex_roaming);
	vd->itc_list = g_slist_append(vd->itc_list, vd->itc_ex_sync);

	/* account name */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = ACCOUNT_NAME_LIST_ITEM;
	li->vd = vd;
	li->entry_str = elm_entry_utf8_to_markup(account_data->account_name);
	li->gl_it = item = elm_genlist_item_append(vd->genlist, vd->itc_account_details, li, NULL,
			ELM_GENLIST_ITEM_NONE, _gl_account_sel_cb, li);
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_ALWAYS);
	vd->list_items = g_slist_append(vd->list_items, li);

	/* display name */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = DISPLAY_NAME_LIST_ITEM;
	li->vd = vd;
	li->entry_str = elm_entry_utf8_to_markup(account_data->user_display_name);
	li->gl_it = item = elm_genlist_item_append(vd->genlist, vd->itc_account_details, li, NULL,
			ELM_GENLIST_ITEM_NONE, _gl_account_sel_cb, li);
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_ALWAYS);
	vd->list_items = g_slist_append(vd->list_items, li);

	/* Sync settings - group index */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = SYNC_SETTING_TITLE_LIST_ITEM;
	li->vd = vd;
	li->gl_it = git = elm_genlist_item_append(vd->genlist, vd->itc_sync_groupindex, li, NULL,
			ELM_GENLIST_ITEM_GROUP, NULL, NULL);
	elm_genlist_item_select_mode_set(git, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
	vd->list_items = g_slist_append(vd->list_items, li);

	/*sync on/off*/
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = SYNC_ONOFF_LIST_ITEM;
	li->vd = vd;
	li->gl_it = elm_genlist_item_append(vd->genlist, vd->itc_sync_onoff, li,
			NULL, ELM_GENLIST_ITEM_NONE, _gl_sel_cb, li);
	vd->list_items = g_slist_append(vd->list_items, li);

	/*sync schedule*/
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = SYNC_SCHEDULE_LIST_ITEM;
	li->vd = vd;
	li->gl_it = elm_genlist_item_append(vd->genlist, vd->itc_sync, li,
			NULL, ELM_GENLIST_ITEM_NONE, _gl_ex_sel_cb, li);
	vd->list_items = g_slist_append(vd->list_items, li);

	/*while roaming*/
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = WHILE_ROAMING_LIST_ITEM;
	li->vd = vd;
	li->gl_it = elm_genlist_item_append(vd->genlist, vd->itc_roaming, li,
			NULL, ELM_GENLIST_ITEM_NONE, _gl_ex_sel_cb, li);
	vd->list_items = g_slist_append(vd->list_items, li);

	elm_object_part_content_set(vd->base.content, "elm.swallow.content", vd->genlist);

}

static int _is_imap_push_supported(const email_account_t *account)
{
	int ret = 0;

	if (account->retrieval_mode & EMAIL_IMAP4_IDLE_SUPPORTED)
		ret = 1;

	return ret;
}

static void _save_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailSettingVD *vd = data;

	retm_if(!vd, "vd is NULL");

	int account_id = -1;
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;
	int error_code;

	DELETE_EVAS_OBJECT(ugd->popup);

	/* handle initialize */
	vd->folder_sync_handle = EMAIL_OP_HANDLE_INITIALIZER;

	/* add to account_svc */
	error_code = setting_add_account_to_account_svc(ugd->new_account);
	if (error_code == ACCOUNT_ERROR_NONE) {
		debug_log("succeed to adding account to account-svc");
		/* add to email_svc */
		if (email_engine_add_account(ugd->new_account, &account_id, &error_code)) {
			debug_log("succeed to adding account to email-service");
			ugd->account_id = account_id;
			if (ugd->is_set_default_account) {
				debug_log("this account will be default account: %d", account_id);
				email_engine_set_default_account(account_id);
			}
			/* account color set */
			ugd->new_account->color_label = setting_get_available_color(account_id);
			if (!email_engine_update_account(account_id, ugd->new_account)) {
				debug_log("Failed email_engine_update_account");
			}

			/* start folder sync */
			email_engine_sync_imap_mailbox_list(ugd->account_id, &(vd->folder_sync_handle));
			_create_processing_popup(vd);
		} else {
			debug_error("failed to adding account to email-service");
			if (error_code != EMAIL_ERROR_NONE) {
				const email_string_t *type = setting_get_service_fail_type(error_code);
				const email_string_t *header = setting_get_service_fail_type_header(error_code);
				ugd->popup = setting_get_notify(&vd->base,
						header,
						type,
						1,
						&(EMAIL_SETTING_STRING_OK),
						_popup_ok_cb, NULL, NULL);
			}
			error_code = setting_delete_account_from_account_svc(ugd->new_account->account_svc_id);
			debug_warning_if(error_code != ACCOUNT_ERROR_NONE, "setting_delete_account_from_account_svc failed: %d", error_code);
		}
	} else {
		debug_error("failed to adding account to account-svc");
		if (error_code == ACCOUNT_ERROR_DUPLICATED) {
			ugd->popup = setting_get_notify(&vd->base,
					&(EMAIL_SETTING_STRING_WARNING),
					&(EMAIL_SETTING_STRING_ALREADY_EXIST), 1,
					&(EMAIL_SETTING_STRING_OK),
					_popup_ok_cb, NULL, NULL);
		} else {
			ugd->popup = setting_get_notify(&vd->base,
					&(EMAIL_SETTING_STRING_WARNING),
					&(EMAIL_SETTING_STRING_UNABLE_TO_ADD_ACCOUNT), 1,
					&(EMAIL_SETTING_STRING_OK),
					_popup_ok_cb, NULL, NULL);
		}
	}
}

static void _create_processing_popup(EmailSettingVD *vd)
{
	debug_enter();

	retm_if(!vd, "view data is null");

	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;
	email_account_t *account = ugd->new_account;

	debug_secure("account name:%s", account->account_name);
	debug_secure("email address:%s", account->user_email_address);

	DELETE_EVAS_OBJECT(ugd->popup);

	debug_log("Start Add Account");
	ugd->popup = setting_get_pb_process_notify(&vd->base,
			&(EMAIL_SETTING_STRING_PROCESSING), 0,
			NULL, NULL,
			NULL, NULL,
			POPUP_BACK_TYPE_NO_OP, NULL);

	DELETE_TIMER_OBJECT(vd->processing_timer);
	vd->processing_timer = ecore_timer_add(SYNC_TIMEOUT_SEC, _after_add_account_cb, vd);
}

static Eina_Bool _after_add_account_cb(void *data)
{
	retvm_if(!data, ECORE_CALLBACK_CANCEL, "vd is NULL");

	EmailSettingVD *vd = data;
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;

	if (!vd->is_add_account_finished)
		return ECORE_CALLBACK_RENEW;

	DELETE_TIMER_OBJECT(vd->processing_timer);

	email_module_make_destroy_request(&ugd->base);

	return ECORE_CALLBACK_CANCEL;
}

static void _mailbox_sync_end_cb(int account_id, email_setting_response_data *response, void *user_data)
{
	debug_enter();

	EmailSettingVD *vd = user_data;
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;
	int handle;

	retm_if(!response, "response data is NULL");
	retm_if(ugd->account_id != response->account_id, "account_id is different");
	retm_if(vd->folder_sync_handle != response->handle, "handle is different");

	/* handle initialize */
	vd->folder_sync_handle = EMAIL_OP_HANDLE_INITIALIZER;

	/* Inbox sync */
	if (!account_data->sync_disabled) {
		debug_log("initial sync start");
		int ret = 0;
		email_mailbox_t *mailbox = NULL;
		ret = email_get_mailbox_by_mailbox_type(account_id, EMAIL_MAILBOX_TYPE_INBOX, &mailbox);
		if (ret != EMAIL_ERROR_NONE || mailbox == NULL) {
			debug_error("email_get_mailbox_by_mailbox_type failed");
		} else {
			email_engine_sync_folder(account_id, mailbox->mailbox_id, &handle);
			email_free_mailbox(&mailbox, 1);
		}
	} else {
		debug_log("initial sync was disabled!");
	}

	vd->is_add_account_finished = 1;
}

static void _cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailSettingVD *vd = data;

	email_module_exit_view(&vd->base);
}

static void _on_back_cb(email_view_t *self)
{
	debug_enter();
	retm_if(!self, "self is NULL");

	email_module_exit_view(self);
}

static void _gl_account_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	ListItemData *li = data;
	EmailSettingVD *vd = li->vd;
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;
	Elm_Object_Item *item = event_info;
	elm_genlist_item_selected_set(item, EINA_FALSE);
	char *str = NULL;

	if (li->index == ACCOUNT_NAME_LIST_ITEM) {
		str = g_strdup(li->entry_str);
		ugd->popup = setting_get_entry_content_notify(&vd->base, &(EMAIL_SETTING_STRING_ACCOUNT_NAME), str,
				2, &(EMAIL_SETTING_STRING_CANCEL), _account_popup_cancel_cb, &(EMAIL_SETTING_STRING_DONE), _account_popup_name_done_cb, POPUP_ENTRY_EDITFIELD);
		Evas_Object *entry_layout = elm_object_content_get(ugd->popup);
		Evas_Object *entry = elm_layout_content_get(entry_layout, "elm.swallow.content");
		li->entry_limit = setting_set_input_entry_limit(entry, EMAIL_LIMIT_ACCOUNT_NAME_LENGTH, 0);
		li->entry = entry;
		elm_object_focus_set(entry, EINA_TRUE);
		evas_object_smart_callback_add(li->entry, "activated", _account_popup_name_done_cb, vd);
		evas_object_smart_callback_add(li->entry, "changed", _account_popup_entry_changed_cb, vd);
		evas_object_smart_callback_add(li->entry, "preedit,changed", _account_popup_entry_changed_cb, vd);
	} else if (li->index == DISPLAY_NAME_LIST_ITEM) {
		str = g_strdup(li->entry_str);
		ugd->popup = setting_get_entry_content_notify(&vd->base, &(EMAIL_SETTING_STRING_SENDER_NAME), str,
				2, &(EMAIL_SETTING_STRING_CANCEL), _account_popup_cancel_cb, &(EMAIL_SETTING_STRING_DONE), _account_popup_sender_name_done_cb, POPUP_ENTRY_EDITFIELD);
		Evas_Object *entry_layout = elm_object_content_get(ugd->popup);
		Evas_Object *entry = elm_layout_content_get(entry_layout, "elm.swallow.content");
		li->entry_limit = setting_set_input_entry_limit(entry, EMAIL_LIMIT_DISPLAY_NAME_LENGTH, 0);
		li->entry = entry;
		elm_object_focus_set(entry, EINA_TRUE);
		evas_object_smart_callback_add(li->entry, "activated", _account_popup_sender_name_done_cb, vd);
		evas_object_smart_callback_add(li->entry, "changed", _account_popup_entry_changed_cb, vd);
		evas_object_smart_callback_add(li->entry, "preedit,changed", _account_popup_entry_changed_cb, vd);
	}
	evas_object_show(ugd->popup);
	FREE(str);
}

static void _gl_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	ListItemData *li = data;
	EmailSettingVD *vd = li->vd;

	Elm_Object_Item *item = event_info;
	elm_genlist_item_selected_set(item, EINA_FALSE);

	if (li->index == SYNC_ONOFF_LIST_ITEM) {
		Eina_Bool state = elm_check_state_get(li->check);
		elm_check_state_set(li->check, !state);
		_sync_onoff_cb(vd, li->check, NULL);
	}
}

static void _gl_ex_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	ListItemData *li = data;
	EmailSettingVD *vd = li->vd;
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;
	Elm_Object_Item *item = event_info;
	elm_genlist_item_selected_set(item, EINA_FALSE);

	Evas_Object *genlist = NULL;
	if (li->index == WHILE_ROAMING_LIST_ITEM) {
		ugd->popup = setting_get_empty_content_notify(&vd->base, &(EMAIL_SETTING_STRING_WHILE_ROAMING),
				0, NULL, NULL, NULL, NULL);
		genlist = _get_option_genlist(ugd->popup, li);
	} else if (li->index == SYNC_SCHEDULE_LIST_ITEM) {
		ugd->popup = setting_get_empty_content_notify(&vd->base, &(EMAIL_SETTING_STRING_EMAIL_SYNC_SCHEDULE),
				0, NULL, NULL, NULL, NULL);
		genlist = _get_option_genlist(ugd->popup, li);
	}
	elm_object_content_set(ugd->popup, genlist);
	evas_object_show(ugd->popup);
	evas_object_smart_callback_add(ugd->popup, "show,finished", _show_finished_cb, li);
}

static void _gl_ex_roaming_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!account_data, "account_data is null");

	int index = (int)(ptrdiff_t)data;
	EmailSettingVD *vd = g_vd;

	Elm_Object_Item *item = event_info;
	elm_genlist_item_selected_set(item, EINA_FALSE);
	elm_radio_value_set(vd->roaming_radio_grp, index);

	_gl_ex_roaming_radio_cb((void *)(ptrdiff_t)index, NULL, NULL);
}

static void _gl_ex_sync_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!account_data, "account data is null");

	int index = (int)(ptrdiff_t)data;
	EmailSettingVD *vd = g_vd;

	Elm_Object_Item *item = event_info;
	elm_genlist_item_selected_set(item, EINA_FALSE);
	elm_radio_value_set(vd->sync_radio_grp, sync_schedule[index]);

	_gl_ex_sync_radio_cb((void *)(ptrdiff_t)index, NULL, NULL);
}

static void _sync_onoff_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailSettingVD *vd = data;
	Elm_Object_Item *sync_sche_it = NULL;
	Elm_Object_Item *sync_roaming_it = NULL;

	Eina_Bool state = elm_check_state_get(obj);

	GSList *l = vd->list_items;
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

	}
}

static void _gl_ex_roaming_radio_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	int index = (int)(ptrdiff_t)data;
	EmailSettingVD *vd = g_vd;
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;

	if (index == 0)
		account_data->roaming_option = EMAIL_ROAMING_OPTION_RESTRICTED_BACKGROUND_TASK;
	else if (index == 1)
		account_data->roaming_option = EMAIL_ROAMING_OPTION_UNRESTRICTED;

	GSList *l = vd->list_items;
	while (l) {
		ListItemData *li = l->data;
		if (li->index == WHILE_ROAMING_LIST_ITEM) {
			elm_genlist_item_update(li->gl_it);
		}
		l = g_slist_next(l);
	}
	DELETE_EVAS_OBJECT(ugd->popup);
}

static void _gl_ex_sync_radio_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	int index = (int)(ptrdiff_t)data;
	EmailSettingVD *vd = g_vd;
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;

	account_data->check_interval = sync_schedule[index];

	GSList *l = vd->list_items;
	while (l) {
		ListItemData *li = l->data;
		if (li->index == SYNC_SCHEDULE_LIST_ITEM) {
			elm_genlist_item_update(li->gl_it);
			break;
		}
		l = g_slist_next(l);
	}

	DELETE_EVAS_OBJECT(ugd->popup);
}

static char *_gl_ef_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	ListItemData *li = data;

	if (li) {
		if (!strcmp("elm.text", part)) {
			if (li->index == ACCOUNT_NAME_LIST_ITEM)
				return strdup(email_setting_gettext(EMAIL_SETTING_STRING_ACCOUNT_NAME));
			else if (li->index == DISPLAY_NAME_LIST_ITEM)
				return g_strdup(email_setting_gettext(EMAIL_SETTING_STRING_SENDER_NAME));
		} else if (!strcmp(part, "elm.text.sub")) {
			if (li->index == ACCOUNT_NAME_LIST_ITEM)
				return elm_entry_utf8_to_markup(account_data->account_name);
			else if (li->index == DISPLAY_NAME_LIST_ITEM)
				return elm_entry_utf8_to_markup(account_data->user_display_name);
		}
	}
	return NULL;
}

static Evas_Object *_gl_sync_onoff_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
	retvm_if(!account_data, NULL, "account_data is null");

	ListItemData *li = data;
	EmailSettingVD *vd = li->vd;

	if (!strcmp(part, "elm.swallow.end")) {
		Evas_Object *check = elm_check_add(obj);
		elm_object_style_set(check, "on&off");
		evas_object_smart_callback_add(check, "changed", _sync_onoff_cb, vd);
		evas_object_propagate_events_set(check, EINA_FALSE);
		elm_check_state_set(check, account_data->sync_disabled ? EINA_FALSE : EINA_TRUE);

		GSList *l = vd->list_items;
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
	EmailSettingVD *vd = g_vd;

	if (!strcmp(part, "elm.swallow.end")) {
		Evas_Object *radio = elm_radio_add(obj);
		elm_radio_group_add(radio, vd->roaming_radio_grp);
		elm_radio_state_value_set(radio, index);

		if (account_data->roaming_option == EMAIL_ROAMING_OPTION_RESTRICTED_BACKGROUND_TASK)
			elm_radio_value_set(vd->roaming_radio_grp, 0);
		else if (account_data->roaming_option == EMAIL_ROAMING_OPTION_UNRESTRICTED)
			elm_radio_value_set(vd->roaming_radio_grp, 1);
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
	EmailSettingVD *vd = g_vd;

	if (!strcmp(part, "elm.swallow.end")) {
		Evas_Object *radio = elm_radio_add(obj);
		if (index < SYNC_SCHEDULE_ITEM_COUNT) { /* sync schedule */
			elm_radio_group_add(radio, vd->sync_radio_grp);
			elm_radio_state_value_set(radio, sync_schedule[index]);

			if (sync_schedule[index] == account_data->check_interval) {
				elm_radio_value_set(vd->sync_radio_grp, sync_schedule[index]);
			}
		}
		evas_object_propagate_events_set(radio, EINA_FALSE);
		evas_object_smart_callback_add(radio, "changed", _gl_ex_sync_radio_cb, (void *)(ptrdiff_t)index);
		return radio;
	}
	return NULL;
}

static char *_gl_index_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	ListItemData *li = data;

	if (!li)
		return NULL;

	if (li->index == SYNC_SETTING_TITLE_LIST_ITEM && !g_strcmp0(part, "elm.text")) {
		return strdup(email_setting_gettext(EMAIL_SETTING_STRING_SYNC_SETTINGS));
	}

	return NULL;
}

static char *_gl_sync_onoff_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	retvm_if(!account_data, NULL, "account data is null");

	if (!strcmp(part, "elm.text")) {
		return strdup(email_setting_gettext(EMAIL_SETTING_STRING_EMAIL_SYNC));
	} else if (!strcmp(part, "elm.text.sub")) {
		return strdup(email_setting_gettext(EMAIL_SETTING_STRING_NOT_SYNCED_YET));
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
		EmailSettingVD *vd = li->vd;
		genlist = elm_genlist_add(parent);
		elm_object_style_set(genlist, "popup");

		elm_genlist_homogeneous_set(genlist, EINA_TRUE);
		elm_scroller_policy_set(genlist, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
		elm_scroller_content_min_limit(genlist, EINA_FALSE, EINA_TRUE);

		if (li->index == WHILE_ROAMING_LIST_ITEM) {
			for (i = 0; i < 2; i++) {
				elm_genlist_item_append(genlist, vd->itc_ex_roaming, (void *)(ptrdiff_t)i,
						NULL, ELM_GENLIST_ITEM_NONE, _gl_ex_roaming_sel_cb, (void *)(ptrdiff_t)i);
			}
		} else if (li->index == SYNC_SCHEDULE_LIST_ITEM) {
			for (i = 0; i < SYNC_SCHEDULE_ITEM_COUNT; i++) {
				if (i == 0 && account_data->incoming_server_type == EMAIL_SERVER_TYPE_POP3)
					continue;
				if (i == 0 && !vd->is_imap_push_supported)
					continue;
				elm_genlist_item_append(genlist, vd->itc_ex_sync, (void *)(ptrdiff_t)i,
						NULL, ELM_GENLIST_ITEM_NONE, _gl_ex_sync_sel_cb, (void *)(ptrdiff_t)i);
			}
		}

		evas_object_show(genlist);

		return genlist;
	}
	return NULL;
}


static void _popup_ok_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	if (!data) {
		debug_error("data is NULL");
		return;
	}

	EmailSettingVD *vd = data;
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;

	DELETE_EVAS_OBJECT(ugd->popup);
}

static void _account_popup_name_done_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "data is null");

	EmailSettingVD *vd = data;
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;

	Evas_Object *entry_layout = elm_object_content_get(ugd->popup);
	Evas_Object *entry = elm_layout_content_get(entry_layout, "elm.swallow.content");

	char *val = setting_get_entry_str(entry);
	if (val && strlen(val) > 0) {
		FREE(account_data->account_name);
		if (!(account_data->account_name = strdup(val))) {
			debug_error("strdup() to account_data->account_name - failed error ");
			free(val);
			return;
		}

		GSList *l = vd->list_items;
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
		DELETE_EVAS_OBJECT(ugd->popup);
	}
	free(val);
}

static void _account_popup_sender_name_done_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "data is null");

	EmailSettingVD *vd = data;
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;

	Evas_Object *entry_layout = elm_object_content_get(ugd->popup);
	Evas_Object *entry = elm_layout_content_get(entry_layout, "elm.swallow.content");

	char *val = setting_get_entry_str(entry);
	if (val && strlen(val) > 0) {
		FREE(account_data->user_display_name);
		if (!(account_data->user_display_name = strdup(val))) {
			debug_error("strdup() to account_data->user_display_name - failed error ");
			free(val);
			return;
		}

		GSList *l = vd->list_items;
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
		DELETE_EVAS_OBJECT(ugd->popup);
	}
	free(val);
}

static void _account_popup_entry_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "data is null");

	EmailSettingVD *vd = data;
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;

	Evas_Object *entry_layout = elm_object_content_get(ugd->popup);
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

static void _account_popup_cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "data is null");

	EmailSettingVD *vd = data;
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;

	GSList *l = vd->list_items;
	while (l) {
		ListItemData *li = l->data;
		if (li->entry) {
			li->entry = NULL;
			break;
		}
		l = g_slist_next(l);
	}

	DELETE_EVAS_OBJECT(ugd->popup);
}

static void _show_finished_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	ListItemData *li = data;
	int selected_index = 0;
	EmailSettingVD *vd = (EmailSettingVD *)li->vd;

	if (li->index == WHILE_ROAMING_LIST_ITEM) {
		selected_index = elm_radio_value_get(vd->roaming_radio_grp);
	} else if (li->index == SYNC_SCHEDULE_LIST_ITEM) {
		selected_index = elm_radio_value_get(vd->sync_radio_grp);
		int i;
		for (i = 0; i < SYNC_SCHEDULE_ITEM_COUNT; i++) {
			if (selected_index == sync_schedule[i]) {
				selected_index = i;
				break;
			}
		}
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
