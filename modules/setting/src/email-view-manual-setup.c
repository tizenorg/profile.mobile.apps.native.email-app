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

#include "email-setting-account-set.h"
#include "email-setting-utils.h"
#include "email-setting-noti-mgr.h"
#include "email-view-manual-setup.h"
#include "email-view-account-details-setup.h"

typedef struct view_data EmailSettingView;

static EmailSettingView *g_vd = NULL;

static int _create(email_view_t *self);
static void _update(email_view_t *self, int flags);
static void _destroy(email_view_t *self);
static void _on_back_cb(email_view_t *self);

static void _initialize_handle(EmailSettingView *view);

static void _push_naviframe(EmailSettingView *view);
static void _create_list(EmailSettingView *view);
static void _validate_account(EmailSettingView *view);
static Eina_Bool _check_null_field(EmailSettingView *view);
static Eina_Bool _check_validation_field(EmailSettingView *view);
static void _set_username_before_at(EmailSettingView *view);
static void _set_username_with_email_address(EmailSettingView *view);
static void _read_all_entries(EmailSettingView *view);
static void _account_validate_cb(int account_id, email_setting_response_data *response, void *user_data);
static void _update_server_info(EmailSettingView *view, email_account_t *account, email_account_server_t server);
static void _update_account_capability(EmailSettingView *view, const char *capability);
static void _update_account_smtp_mail_limit_size(EmailSettingView *view, const char *mail_limit_size);

static void _perform_account_validation(EmailSettingView *view);
static void _show_finished_cb(void *data, Evas_Object *obj, void *event_info);
static void _next_btn_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static void _cancel_btn_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static Eina_Bool _after_validation_cb(void *data);
static void _popup_ok_cb(void *data, Evas_Object *obj, void *event_info);
static void _backup_input_cb(void *data, Evas_Object *obj, void *event_info);
static void _return_key_cb(void *data, Evas_Object *obj, void *event_info);

static char *_gl_text_get_cb(void *data, Evas_Object *obj, const char *part);
static Evas_Object * _gl_ef_account_info_get_content_cb(void *data, Evas_Object *obj, const char *part);
static Evas_Object * _gl_ef_server_settings_get_content_cb(void *data, Evas_Object *obj, const char *part);
static char *_gl_ex_secure_text_get_cb(void *data, Evas_Object *obj, const char *part);
static char *_gl_sp_text_get_cb(void *data, Evas_Object *obj, const char *part);
static char *_gl_ex_incoming_type_text_get_cb(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_gl_ex_sending_secure_content_get_cb(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_gl_ex_incoming_type_content_get_cb(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_gl_ex_incoming_secure_content_get_cb(void *data, Evas_Object *obj, const char *part);
static void _gl_ex_sel_cb(void *data, Evas_Object *obj, void *event_info);
static void _gl_ex_sending_secure_sel_cb(void *data, Evas_Object *obj, void *event_info);
static void _gl_ex_incoming_type_sel_cb(void *data, Evas_Object *obj, void *event_info);
static void _gl_ex_incoming_secure_sel_cb(void *data, Evas_Object *obj, void *event_info);
static void _gl_ex_sending_secure_radio_cb(void *data, Evas_Object *obj, void *event_info);
static void _gl_ex_incoming_type_radio_cb(void *data, Evas_Object *obj, void *event_info);
static void _gl_ex_incoming_secure_radio_cb(void *data, Evas_Object *obj, void *event_info);

static email_string_t EMAIL_SETTING_STRING_EMAIL = {PACKAGE, "IDS_ST_HEADER_EMAIL"};
static email_string_t EMAIL_SETTING_STRING_IMAP4 = {PACKAGE, "IDS_EMAIL_BODY_IMAP4"};
static email_string_t EMAIL_SETTING_STRING_INCOMING_MAIL_SERVER_TYPE = {PACKAGE, "IDS_ST_HEADER_SERVER_TYPE_ABB"};
static email_string_t EMAIL_SETTING_STRING_INCOMING_PORT = {PACKAGE, "IDS_ST_BODY_PORT"};
static email_string_t EMAIL_SETTING_STRING_INCOMING_SERVER = {PACKAGE, "IDS_ST_TMBODY_SERVER_ADDRESS"};
static email_string_t EMAIL_SETTING_STRING_INCOMING_SETTINGS = {PACKAGE, "IDS_ST_HEADER_INCOMING_SERVER_SETTINGS_ABB"};
static email_string_t EMAIL_SETTING_STRING_OK = {PACKAGE, "IDS_EMAIL_BUTTON_OK"};
static email_string_t EMAIL_SETTING_STRING_OUTGOING_PORT = {PACKAGE, "IDS_ST_BODY_PORT"};
static email_string_t EMAIL_SETTING_STRING_OUTGOING_SERVER = {PACKAGE, "IDS_ST_TMBODY_SERVER_ADDRESS"};
static email_string_t EMAIL_SETTING_STRING_OUTGOING_SETTINGS = {PACKAGE, "IDS_ST_HEADER_OUTGOING_SERVER_SETTINGS_ABB"};
static email_string_t EMAIL_SETTING_STRING_POP3 = {PACKAGE, "IDS_ST_SBODY_POP3"};
static email_string_t EMAIL_SETTING_STRING_PWD = {PACKAGE, "IDS_ST_TMBODY_PASSWORD"};
static email_string_t EMAIL_SETTING_STRING_SECURE_CONNECTION = {PACKAGE, "IDS_ST_HEADER_SECURITY_TYPE_ABB"};
static email_string_t EMAIL_SETTING_STRING_SSL = {PACKAGE, "IDS_ST_SBODY_SSL"};
static email_string_t EMAIL_SETTING_STRING_TLS = {PACKAGE, "IDS_ST_SBODY_TLS"};
static email_string_t EMAIL_SETTING_STRING_USER_NAME = {PACKAGE, "IDS_ST_TMBODY_USERNAME"};
static email_string_t EMAIL_SETTING_STRING_OFF = {PACKAGE, "IDS_ST_OPT_NONE"};
static email_string_t EMAIL_SETTING_STRING_NEXT = {PACKAGE, "IDS_ST_BUTTON_NEXT"};
static email_string_t EMAIL_SETTING_STRING_CANCEL = {PACKAGE, "IDS_EMAIL_BUTTON_CANCEL"};
static email_string_t EMAIL_SETTING_STRING_WARNING = {PACKAGE, "IDS_ST_HEADER_WARNING"};
static email_string_t EMAIL_SETTING_STRING_FILL_MANDATORY_FIELDS = {PACKAGE, "IDS_EMAIL_POP_PLEASE_FILL_ALL_THE_MANDATORY_FIELDS"};
static email_string_t EMAIL_SETTING_STRING_ACCOUNT_ALREADY_EXISTS = {PACKAGE, "IDS_ST_POP_THIS_ACCOUNT_HAS_ALREADY_BEEN_ADDED"};
static email_string_t EMAIL_SETTING_STRING_UNABLE_TO_ADD_ACCOUNT = {PACKAGE, "IDS_ST_HEADER_UNABLE_TO_ADD_ACCOUNT_ABB"};

enum {
	USERNAME_LIST_ITEM = 1,
	PASSWORD_LIST_ITEM,
	OUTGOING_SETTING_TITLE_LIST_ITEM,
	OUTGOING_SERVER_LIST_ITEM,
	OUTGOING_PORT_LIST_ITEM,
	OUTGOING_SECURE_CONN_LIST_ITEM,
	INCOMING_SETTING_TITLE_LIST_ITEM,
	INCOMING_SERVER_TYPE_LIST_ITEM,
	INCOMING_SERVER_LIST_ITEM,
	INCOMING_PORT_LIST_ITEM,
	INCOMING_SECURE_CONN_LIST_ITEM,
};

struct view_data {
	email_view_t base;

	int handle;
	int is_retry_validate_with_username;

	GSList *list_items;
	GSList *itc_list;

	Evas_Object *sending_secure_radio_grp;
	Evas_Object *incoming_type_radio_grp;
	Evas_Object *incoming_secure_radio_grp;

	Ecore_Timer *other_vc_timer;
	Ecore_Idler *focus_idler;

	Evas_Object *next_btn;
	Evas_Object *genlist;

	email_account_t *account;

	Elm_Genlist_Item_Class *itc1;
	Elm_Genlist_Item_Class *itc2;
	Elm_Genlist_Item_Class *itc3;
	Elm_Genlist_Item_Class *itc4;
	Elm_Genlist_Item_Class *itc5;
	Elm_Genlist_Item_Class *itc6;
	Elm_Genlist_Item_Class *itc_title;
};

typedef struct _ListItemData {
	int index;
	Elm_Object_Item *gl_it;
	email_editfield_t editfield;
	char *entry_str;
	Elm_Entry_Filter_Limit_Size *entry_limit;
	EmailSettingView *view;
} ListItemData;

static Evas_Object *_get_option_genlist(Evas_Object *parent, ListItemData *li);

void create_manual_setup_view(EmailSettingModule *module)
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

	view->account = module->new_account;
	view->base.content = setting_add_inner_layout(&view->base);
	_push_naviframe(view);

	g_vd = view;

	_create_list(view);

	setting_noti_subscribe(module, EMAIL_SETTING_ACCOUNT_VALIDATE_NOTI, _account_validate_cb, view);

	_initialize_handle(view);

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

	if (flags & EVUF_POPPING) {
		_initialize_handle(view);
	}
}

static void _initialize_handle(EmailSettingView *view)
{
	debug_enter();
	retm_if(!view, "view is null");

	/* initialize handle to distinguish with manual setup's callback */
	view->handle = EMAIL_OP_HANDLE_INITIALIZER;
}

static void _destroy(email_view_t *self)
{
	debug_enter();
	retm_if(!self, "self is null");

	EmailSettingView *view = (EmailSettingView *)self;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	setting_noti_unsubscribe(module, EMAIL_SETTING_ACCOUNT_VALIDATE_NOTI, _account_validate_cb);

	DELETE_EVAS_OBJECT(module->popup);
	DELETE_TIMER_OBJECT(view->other_vc_timer);
	DELETE_IDLER_OBJECT(view->focus_idler);

	GSList *l = view->list_items;
	while (l) {
		ListItemData *li = l->data;
		FREE(li->entry_limit);
		FREE(li->entry_str);
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

	Elm_Object_Item *navi_it = email_module_view_push(&view->base, EMAIL_SETTING_STRING_EMAIL.id, 0);
	elm_object_item_domain_text_translatable_set(navi_it, EMAIL_SETTING_STRING_EMAIL.domain, EINA_TRUE);

	Evas_Object *btn_ly = elm_layout_add(view->base.content);
	elm_layout_file_set(btn_ly, email_get_common_theme_path(), "two_bottom_btn");

	Evas_Object *cancel_btn = elm_button_add(view->base.module->navi);
	elm_object_style_set(cancel_btn, "bottom");
	elm_object_domain_translatable_text_set(cancel_btn, EMAIL_SETTING_STRING_CANCEL.domain, EMAIL_SETTING_STRING_CANCEL.id);
	evas_object_smart_callback_add(cancel_btn, "clicked", _cancel_btn_clicked_cb, view);
	elm_layout_content_set(btn_ly, "btn1.swallow", cancel_btn);

	view->next_btn = elm_button_add(view->base.module->navi);
	elm_object_style_set(view->next_btn, "bottom");
	elm_object_domain_translatable_text_set(view->next_btn, EMAIL_SETTING_STRING_NEXT.domain, EMAIL_SETTING_STRING_NEXT.id);
	evas_object_smart_callback_add(view->next_btn, "clicked", _next_btn_clicked_cb, view);
	elm_layout_content_set(btn_ly, "btn2.swallow", view->next_btn);
	elm_object_disabled_set(view->next_btn, !_check_null_field(view));

	elm_object_item_part_content_set(navi_it, "toolbar", btn_ly);
	evas_object_show(view->base.content);
}

static void _create_list(EmailSettingView *view)
{
	debug_enter();

	retm_if(!view, "view data is null");

	ListItemData *li = NULL;
	char buf[10] = { 0, };

	Elm_Object_Item *item = NULL;
	Elm_Object_Item *git = NULL;

	view->genlist = elm_genlist_add(view->base.content);
	elm_genlist_mode_set(view->genlist, ELM_LIST_COMPRESS);
	elm_genlist_homogeneous_set(view->genlist, EINA_FALSE);
	elm_scroller_policy_set(view->genlist, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);

	view->sending_secure_radio_grp = elm_radio_add(view->genlist);
	elm_radio_value_set(view->sending_secure_radio_grp, -1);
	evas_object_hide(view->sending_secure_radio_grp);

	view->incoming_secure_radio_grp = elm_radio_add(view->genlist);
	elm_radio_value_set(view->incoming_secure_radio_grp, -1);
	evas_object_hide(view->incoming_secure_radio_grp);

	view->incoming_type_radio_grp = elm_radio_add(view->genlist);
	elm_radio_value_set(view->incoming_type_radio_grp, -1);
	evas_object_hide(view->incoming_type_radio_grp);

	view->itc_title = setting_get_genlist_class_item("group_index", _gl_sp_text_get_cb, NULL, NULL, NULL);
	view->itc1 = setting_get_genlist_class_item("full", NULL, _gl_ef_account_info_get_content_cb, NULL, NULL);
	view->itc2 = setting_get_genlist_class_item("full", NULL, _gl_ef_server_settings_get_content_cb, NULL, NULL);
	view->itc3 = setting_get_genlist_class_item("type1", _gl_text_get_cb, NULL, NULL, NULL);

	view->itc4 = setting_get_genlist_class_item("type1", _gl_ex_secure_text_get_cb, _gl_ex_sending_secure_content_get_cb, NULL, NULL);
	view->itc5 = setting_get_genlist_class_item("type1", _gl_ex_incoming_type_text_get_cb, _gl_ex_incoming_type_content_get_cb, NULL, NULL);
	view->itc6 = setting_get_genlist_class_item("type1", _gl_ex_secure_text_get_cb, _gl_ex_incoming_secure_content_get_cb, NULL, NULL);

	view->itc_list = g_slist_append(view->itc_list, view->itc1);
	view->itc_list = g_slist_append(view->itc_list, view->itc2);
	view->itc_list = g_slist_append(view->itc_list, view->itc3);
	view->itc_list = g_slist_append(view->itc_list, view->itc4);
	view->itc_list = g_slist_append(view->itc_list, view->itc5);
	view->itc_list = g_slist_append(view->itc_list, view->itc6);
	view->itc_list = g_slist_append(view->itc_list, view->itc_title);

	/* User name */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = USERNAME_LIST_ITEM;
	li->view = view;
	li->entry_str = g_strdup(view->account->user_display_name);
	li->gl_it = item = elm_genlist_item_append(view->genlist, view->itc1, li, NULL,
			ELM_GENLIST_ITEM_NONE, NULL, NULL);
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_NONE);
	view->list_items = g_slist_append(view->list_items, li);

	/* Password */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = PASSWORD_LIST_ITEM;
	li->view = view;
	li->entry_str = g_strdup(view->account->incoming_server_password);
	li->gl_it = item = elm_genlist_item_append(view->genlist, view->itc1, li, NULL,
			ELM_GENLIST_ITEM_NONE, NULL, NULL);
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_NONE);
	view->list_items = g_slist_append(view->list_items, li);

	/* Incoming settings title */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = INCOMING_SETTING_TITLE_LIST_ITEM;
	li->view = view;
	li->gl_it = git = elm_genlist_item_append(view->genlist, view->itc_title, li, NULL,
			ELM_GENLIST_ITEM_GROUP, NULL, NULL);
	elm_genlist_item_select_mode_set(git, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
	view->list_items = g_slist_append(view->list_items, li);

	/* incoming server type */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = INCOMING_SERVER_TYPE_LIST_ITEM;
	li->view = view;
	li->gl_it = item = elm_genlist_item_append(view->genlist, view->itc3, li,
			NULL, ELM_GENLIST_ITEM_TREE, _gl_ex_sel_cb, li);
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_NONE);
	view->list_items = g_slist_append(view->list_items, li);

	/* incoming server */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = INCOMING_SERVER_LIST_ITEM;
	li->view = view;
	li->entry_str = g_strdup(view->account->incoming_server_address);
	li->gl_it = item = elm_genlist_item_append(view->genlist, view->itc2, li, NULL,
			ELM_GENLIST_ITEM_NONE, NULL, NULL);
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_NONE);
	view->list_items = g_slist_append(view->list_items, li);

	/* incoming port */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = INCOMING_PORT_LIST_ITEM;
	li->view = view;
	if (view->account->incoming_server_port_number > 0) {
		snprintf(buf, sizeof(buf), "%d", view->account->incoming_server_port_number);
		li->entry_str = g_strdup(buf);
	}
	li->gl_it = item = elm_genlist_item_append(view->genlist, view->itc2, li, NULL,
			ELM_GENLIST_ITEM_NONE, NULL, NULL);
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_NONE);
	view->list_items = g_slist_append(view->list_items, li);

	/* secure connection */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = INCOMING_SECURE_CONN_LIST_ITEM;
	li->view = view;
	li->gl_it = item = elm_genlist_item_append(view->genlist, view->itc3, li,
			NULL, ELM_GENLIST_ITEM_TREE, _gl_ex_sel_cb, li);
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_ALWAYS);
	view->list_items = g_slist_append(view->list_items, li);

	/* Outgoing setting title */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = OUTGOING_SETTING_TITLE_LIST_ITEM;
	li->view = view;
	li->gl_it = git = elm_genlist_item_append(view->genlist, view->itc_title, li, NULL,
			ELM_GENLIST_ITEM_GROUP, NULL, NULL);
	elm_genlist_item_select_mode_set(git, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
	view->list_items = g_slist_append(view->list_items, li);

	/* smtp server */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = OUTGOING_SERVER_LIST_ITEM;
	li->view = view;
	li->entry_str = g_strdup(view->account->outgoing_server_address);
	li->gl_it = item = elm_genlist_item_append(view->genlist, view->itc2, li, NULL,
			ELM_GENLIST_ITEM_NONE, NULL, NULL);
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_NONE);
	view->list_items = g_slist_append(view->list_items, li);

	/* smtp port */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = OUTGOING_PORT_LIST_ITEM;
	li->view = view;
	if (view->account->outgoing_server_port_number > 0) {
		snprintf(buf, sizeof(buf), "%d", view->account->outgoing_server_port_number);
		li->entry_str = g_strdup(buf);
	}
	li->gl_it = item = elm_genlist_item_append(view->genlist, view->itc2, li, NULL,
			ELM_GENLIST_ITEM_NONE, NULL, NULL);
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_NONE);
	view->list_items = g_slist_append(view->list_items, li);

	/* sending security */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = OUTGOING_SECURE_CONN_LIST_ITEM;
	li->view = view;
	li->gl_it = item = elm_genlist_item_append(view->genlist, view->itc3, li,
			NULL, ELM_GENLIST_ITEM_NONE, _gl_ex_sel_cb, li);
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_ALWAYS);
	view->list_items = g_slist_append(view->list_items, li);

	elm_object_part_content_set(view->base.content, "elm.swallow.content", view->genlist);
}

static void _validate_account(EmailSettingView *view)
{
	debug_enter();

	retm_if(!view, "view data is null");

	EmailSettingModule *module = (EmailSettingModule *)view->base.module;
	int error_code = 0;

	view->handle = EMAIL_OP_HANDLE_INITIALIZER;

	if (email_engine_validate_account(module->new_account, &(view->handle), &error_code)) {
		debug_log("Validate account");
		setting_create_account_validation_popup(&view->base, &(view->handle));
	} else {
		if (error_code == EMAIL_ERROR_ALREADY_EXISTS) {
			module->popup = setting_get_notify(&view->base,
					&(EMAIL_SETTING_STRING_WARNING),
					&(EMAIL_SETTING_STRING_ACCOUNT_ALREADY_EXISTS), 1,
					&(EMAIL_SETTING_STRING_OK), _popup_ok_cb, NULL, NULL);
		} else {
			module->popup = setting_get_notify(&view->base,
					&(EMAIL_SETTING_STRING_WARNING),
					&(EMAIL_SETTING_STRING_UNABLE_TO_ADD_ACCOUNT), 1,
					&(EMAIL_SETTING_STRING_OK), _popup_ok_cb, NULL, NULL);
		}
	}
}

static Eina_Bool _check_validation_field(EmailSettingView *view)
{
	debug_enter();
	GSList *l = view->list_items;

	Eina_Bool ret = EINA_TRUE;
	if (!l) {
		ret = EINA_FALSE;
	}

	while (l) {
		ListItemData *li = l->data;
		if (li->index != OUTGOING_SETTING_TITLE_LIST_ITEM &&
				li->index != OUTGOING_SECURE_CONN_LIST_ITEM &&
				li->index != INCOMING_SERVER_TYPE_LIST_ITEM &&
				li->index != INCOMING_SETTING_TITLE_LIST_ITEM &&
				li->index != INCOMING_SECURE_CONN_LIST_ITEM) {
			if (li->entry_str == NULL || !g_strcmp0(li->entry_str, "")) {
				ret = EINA_FALSE;
				break;
			}
		}
		l = g_slist_next(l);
	}

	return ret;
}

static Eina_Bool _check_null_field(EmailSettingView *view)
{
	debug_enter();
	email_account_t *account = view->account;

	debug_secure("account name:%s", account->account_name);
	debug_secure("user name:%s", account->incoming_server_user_name);
	debug_secure("recv server:%s", account->incoming_server_address);
	debug_secure("recv port:%d", account->incoming_server_port_number);
	debug_secure("sending server:%s", account->outgoing_server_address);
	debug_secure("sending port:%d", account->outgoing_server_port_number);
	debug_secure("sending user:%s", account->outgoing_server_user_name);

	if (!STR_VALID(account->incoming_server_user_name) ||
			!STR_VALID(account->incoming_server_password) ||
			!STR_VALID(account->incoming_server_address) ||
			!STR_VALID(account->outgoing_server_address) ||
			account->incoming_server_port_number == 0 ||
			account->outgoing_server_port_number == 0) {
		return EINA_FALSE;
	} else {
		return EINA_TRUE;
	}
}

static void _read_all_entries(EmailSettingView *view)
{
	debug_enter();
	GSList *l = NULL;
	email_account_t *account = NULL;

	account = view->account;
	l = view->list_items;

	FREE(account->incoming_server_user_name);
	FREE(account->incoming_server_password);
	FREE(account->incoming_server_address);
	FREE(account->outgoing_server_address);
	FREE(account->outgoing_server_user_name);
	FREE(account->user_display_name);

	while (l) {
		ListItemData *li = l->data;
		if (li->index == USERNAME_LIST_ITEM) {
			account->incoming_server_user_name = g_strdup(li->entry_str);
			account->outgoing_server_user_name = g_strdup(li->entry_str);
			account->user_display_name = g_strdup(li->entry_str);
		} else if (li->index == PASSWORD_LIST_ITEM) {
			account->incoming_server_password = g_strdup(li->entry_str);
		} else if (li->index == OUTGOING_SERVER_LIST_ITEM) {
			account->outgoing_server_address = g_strdup(li->entry_str);
		} else if (li->index == OUTGOING_PORT_LIST_ITEM) {
			account->outgoing_server_port_number = atoi(li->entry_str);
		} else if (li->index == INCOMING_SERVER_LIST_ITEM) {
			account->incoming_server_address = g_strdup(li->entry_str);
		} else if (li->index == INCOMING_PORT_LIST_ITEM) {
			account->incoming_server_port_number = atoi(li->entry_str);
		}
		l = g_slist_next(l);
	}
}

static void _perform_account_validation(EmailSettingView *view)
{
	email_account_t *account = view->account;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	/* Save the data */
	_read_all_entries(view);

	/* check Null field */
	if (!_check_null_field(view)) {
		module->popup = setting_get_notify(&(view->base),
				&(EMAIL_SETTING_STRING_WARNING),
				&(EMAIL_SETTING_STRING_FILL_MANDATORY_FIELDS), 1,
				&(EMAIL_SETTING_STRING_OK), _popup_ok_cb, NULL, NULL);
		return;
	}

	/* check duplication account */
	if (setting_is_duplicate_account(account->user_email_address) < 0) {
		module->popup = setting_get_notify(&(view->base),
				&(EMAIL_SETTING_STRING_WARNING),
				&(EMAIL_SETTING_STRING_ACCOUNT_ALREADY_EXISTS), 1,
				&(EMAIL_SETTING_STRING_OK), _popup_ok_cb, NULL, NULL);
		return;
	}

	_validate_account(view);
}

static void _next_btn_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	_perform_account_validation(data);
}

static void _cancel_btn_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailSettingView *view = data;

	email_module_exit_view(&view->base);
}

static void _on_back_cb(email_view_t *self)
{
	debug_enter();
	retm_if(!self, "self is NULL");

	email_module_exit_view(self);
}

static Eina_Bool _after_validation_cb(void *data)
{
	debug_enter();

	EmailSettingView *view = data;

	retvm_if(!view, ECORE_CALLBACK_CANCEL, "view is NULL");

	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	DELETE_TIMER_OBJECT(view->other_vc_timer);
	DELETE_EVAS_OBJECT(module->popup);

	create_account_details_setup_view(module);

	return ECORE_CALLBACK_CANCEL;
}

static void _popup_ok_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "data is NULL");

	EmailSettingView *view = data;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	DELETE_EVAS_OBJECT(module->popup);
}

static void _backup_input_cb(void *data, Evas_Object *obj, void *event_info)
{
	retm_if(!data, "data is NULL");

	ListItemData *li = data;

	FREE(li->entry_str);
	li->entry_str = setting_get_entry_str(obj);

	Eina_Bool is_input_valid = _check_validation_field(li->view);

	elm_object_disabled_set(li->view->next_btn, !is_input_valid);

	if (li->index == OUTGOING_PORT_LIST_ITEM) {
		elm_entry_input_panel_return_key_disabled_set(li->editfield.entry, !is_input_valid);
	}
}

static void _return_key_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "data is NULL");
	retm_if(!obj, "obj is NULL");

	ListItemData *li = data;
	EmailSettingView *view = li->view;
	Evas_Object *entry_password = NULL;
	Evas_Object *entry_smtp_server = NULL;
	Evas_Object *entry_smtp_port = NULL;
	Evas_Object *entry_incoming_server = NULL;
	Evas_Object *entry_incoming_port = NULL;

	GSList *l = view->list_items;
	while (l) {
		ListItemData *_li = l->data;
		if (_li->index == PASSWORD_LIST_ITEM)
			entry_password = _li->editfield.entry;
		else if (_li->index == INCOMING_SERVER_LIST_ITEM)
			entry_incoming_server = _li->editfield.entry;
		else if (_li->index == OUTGOING_SERVER_LIST_ITEM)
			entry_smtp_server = _li->editfield.entry;
		else if (_li->index == INCOMING_PORT_LIST_ITEM) {
			entry_incoming_port = _li->editfield.entry;
		} else if (_li->index == OUTGOING_PORT_LIST_ITEM) {
			entry_smtp_port = _li->editfield.entry;
		}
		l = g_slist_next(l);
	}

	if (li->index != OUTGOING_PORT_LIST_ITEM) {
		Evas_Object *next_entry = NULL;
		if (li->index == USERNAME_LIST_ITEM)
			next_entry = entry_password;
		else if (li->index == PASSWORD_LIST_ITEM)
			next_entry = entry_incoming_server;
		else if (li->index == INCOMING_SERVER_LIST_ITEM)
			next_entry = entry_incoming_port;
		else if (li->index == INCOMING_PORT_LIST_ITEM)
			next_entry = entry_smtp_server;
		else if (li->index == OUTGOING_SERVER_LIST_ITEM)
			next_entry = entry_smtp_port;
		elm_object_focus_set(next_entry, EINA_TRUE);
		elm_entry_cursor_end_set(next_entry);
	} else {
		_perform_account_validation(view);
	}
}

static char *_gl_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	ListItemData *li = data;
	EmailSettingView *view = li->view;
	email_account_t *account = view->account;

	if (!strcmp(part, "elm.text")) {
		if (li->index == OUTGOING_SECURE_CONN_LIST_ITEM) {
			return strdup(email_setting_gettext(EMAIL_SETTING_STRING_SECURE_CONNECTION));
		} else if (li->index == INCOMING_SERVER_TYPE_LIST_ITEM) {
			return strdup(email_setting_gettext(EMAIL_SETTING_STRING_INCOMING_MAIL_SERVER_TYPE));
		} else if (li->index == INCOMING_SECURE_CONN_LIST_ITEM) {
			return strdup(email_setting_gettext(EMAIL_SETTING_STRING_SECURE_CONNECTION));
		}
	} else if (!strcmp(part, "elm.text.sub")) {
		if (li->index == OUTGOING_SECURE_CONN_LIST_ITEM) {
			if (account->outgoing_server_secure_connection == 0) {
				return strdup(email_setting_gettext(EMAIL_SETTING_STRING_OFF));
			} else if (account->outgoing_server_secure_connection == 1) {
				return strdup(email_setting_gettext(EMAIL_SETTING_STRING_SSL));
			} else if (account->outgoing_server_secure_connection == 2) {
				return strdup(email_setting_gettext(EMAIL_SETTING_STRING_TLS));
			}
		} else if (li->index == INCOMING_SERVER_TYPE_LIST_ITEM) {
			if (account->incoming_server_type == EMAIL_SERVER_TYPE_POP3) {
				return strdup(email_setting_gettext(EMAIL_SETTING_STRING_POP3));
			} else if (account->incoming_server_type == EMAIL_SERVER_TYPE_IMAP4) {
				return strdup(email_setting_gettext(EMAIL_SETTING_STRING_IMAP4));
			}
		} else if (li->index == INCOMING_SECURE_CONN_LIST_ITEM) {
			if (account->incoming_server_secure_connection == 0) {
				return strdup(email_setting_gettext(EMAIL_SETTING_STRING_OFF));
			} else if (account->incoming_server_secure_connection == 1) {
				return strdup(email_setting_gettext(EMAIL_SETTING_STRING_SSL));
			} else if (account->incoming_server_secure_connection == 2) {
				return strdup(email_setting_gettext(EMAIL_SETTING_STRING_TLS));
			}
		}
	}

	return NULL;
}
static Evas_Object *_gl_ef_account_info_get_content_cb(void *data, Evas_Object *obj, const char *part)
{
	ListItemData *li = data;
	if (li != NULL && !strcmp(part, "elm.swallow.content")) {
		Evas_Object *full_item_ly = elm_layout_add(obj);
		elm_layout_file_set(full_item_ly, email_get_setting_theme_path(), "gl_entry_text_item");

		if (li->index == PASSWORD_LIST_ITEM){
			email_common_util_editfield_create(full_item_ly, EF_PASSWORD, &li->editfield);
			li->entry_limit = setting_set_input_entry_limit(li->editfield.entry,
					EMAIL_LIMIT_PASSWORD_LENGTH, 0);

			elm_object_part_text_set(full_item_ly, "elm.text", email_setting_gettext(EMAIL_SETTING_STRING_PWD));

		} else if (li->index == USERNAME_LIST_ITEM) {
			email_common_util_editfield_create(full_item_ly, 0, &li->editfield);
			li->entry_limit = setting_set_input_entry_limit(li->editfield.entry,
					0, EMAIL_LIMIT_USER_NAME_LENGTH);

			elm_object_part_text_set(full_item_ly, "elm.text", email_setting_gettext(EMAIL_SETTING_STRING_USER_NAME));
		}
		setting_set_entry_str(li->editfield.entry, li->entry_str);
		elm_entry_input_panel_return_key_type_set(li->editfield.entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_NEXT);
		evas_object_propagate_events_set(li->editfield.entry, EINA_TRUE);
		evas_object_smart_callback_add(li->editfield.entry, "changed", _backup_input_cb, li);
		evas_object_smart_callback_add(li->editfield.entry, "preedit,changed", _backup_input_cb, li);
		evas_object_smart_callback_add(li->editfield.entry, "activated", _return_key_cb, li);

		evas_object_smart_calculate(li->editfield.layout);
		elm_object_part_content_set(full_item_ly, "elm.swallow.entry", li->editfield.layout);
		elm_layout_sizing_eval(full_item_ly);
		return full_item_ly;
	}
	return NULL;
}

static Evas_Object *_gl_ef_server_settings_get_content_cb(void *data, Evas_Object *obj, const char *part)
{
	ListItemData *li = data;
	if (li != NULL && !strcmp(part, "elm.swallow.content")) {

		email_common_util_editfield_create(obj, 0, &li->editfield);

		if (li->index == OUTGOING_PORT_LIST_ITEM ||
				li->index == INCOMING_PORT_LIST_ITEM) {
			elm_entry_input_panel_layout_set(li->editfield.entry, ELM_INPUT_PANEL_LAYOUT_DATETIME);
			elm_entry_context_menu_disabled_set(li->editfield.entry, EINA_TRUE);
		}

		if (li->index == OUTGOING_SERVER_LIST_ITEM) {
			li->entry_limit = setting_set_input_entry_limit(li->editfield.entry,
					0, EMAIL_LIMIT_OUTGOING_SERVER_LENGTH);
		} else if (li->index == OUTGOING_PORT_LIST_ITEM) {
			li->entry_limit = setting_set_input_entry_limit(li->editfield.entry,
					0, EMAIL_LIMIT_OUTGOING_PORT_LENGTH);
		} else if (li->index == INCOMING_SERVER_LIST_ITEM) {
			li->entry_limit = setting_set_input_entry_limit(li->editfield.entry,
					0, EMAIL_LIMIT_INCOMING_SERVER_LENGTH);
		} else if (li->index == INCOMING_PORT_LIST_ITEM) {
			li->entry_limit = setting_set_input_entry_limit(li->editfield.entry,
					0, EMAIL_LIMIT_INCOMING_PORT_LENGTH);
		}

		elm_entry_cnp_mode_set(li->editfield.entry, ELM_CNP_MODE_PLAINTEXT);
		elm_entry_editable_set(li->editfield.entry, TRUE);

		if (li->index == OUTGOING_PORT_LIST_ITEM) {
			elm_entry_input_panel_return_key_type_set(li->editfield.entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
			elm_entry_input_panel_return_key_disabled_set(li->editfield.entry, EINA_TRUE);
		} else {
			elm_entry_input_panel_return_key_type_set(li->editfield.entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_NEXT);
		}
		setting_set_entry_str(li->editfield.entry, li->entry_str);

		evas_object_propagate_events_set(li->editfield.entry, EINA_TRUE);
		evas_object_smart_callback_add(li->editfield.entry, "changed", _backup_input_cb, li);
		evas_object_smart_callback_add(li->editfield.entry, "preedit,changed", _backup_input_cb, li);
		evas_object_smart_callback_add(li->editfield.entry, "activated", _return_key_cb, li);

		if (li->index == OUTGOING_SERVER_LIST_ITEM) {
			elm_object_domain_translatable_part_text_set(li->editfield.entry, "elm.guide",
					EMAIL_SETTING_STRING_OUTGOING_SERVER.domain, EMAIL_SETTING_STRING_OUTGOING_SERVER.id);
		} else if (li->index == OUTGOING_PORT_LIST_ITEM) {
			elm_object_domain_translatable_part_text_set(li->editfield.entry, "elm.guide",
					EMAIL_SETTING_STRING_OUTGOING_PORT.domain, EMAIL_SETTING_STRING_OUTGOING_PORT.id);
		} else if (li->index == INCOMING_SERVER_LIST_ITEM) {
			elm_object_domain_translatable_part_text_set(li->editfield.entry, "elm.guide",
					EMAIL_SETTING_STRING_INCOMING_SERVER.domain, EMAIL_SETTING_STRING_INCOMING_SERVER.id);
		} else if (li->index == INCOMING_PORT_LIST_ITEM) {
			elm_object_domain_translatable_part_text_set(li->editfield.entry, "elm.guide",
					EMAIL_SETTING_STRING_INCOMING_PORT.domain, EMAIL_SETTING_STRING_INCOMING_PORT.id);
		}
		return li->editfield.layout;
	}

	return NULL;
}

static char *_gl_ex_secure_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	int index = (int)(ptrdiff_t)data;

	if (!strcmp(part, "elm.text")) {
		if (index == 0) {
			return strdup(email_setting_gettext(EMAIL_SETTING_STRING_OFF));
		} else if (index == 1) {
			return strdup(email_setting_gettext(EMAIL_SETTING_STRING_SSL));
		} else if (index == 2) {
			return strdup(email_setting_gettext(EMAIL_SETTING_STRING_TLS));
		}
	}
	return NULL;
}

static char *_gl_sp_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	ListItemData *li = data;

	if (!strcmp(part, "elm.text")) {
		if (li->index == INCOMING_SETTING_TITLE_LIST_ITEM) {
			return strdup(email_setting_gettext(EMAIL_SETTING_STRING_INCOMING_SETTINGS));
		} else if (li->index == OUTGOING_SETTING_TITLE_LIST_ITEM) {
			return strdup(email_setting_gettext(EMAIL_SETTING_STRING_OUTGOING_SETTINGS));
		}
	}

	return NULL;
}

static Evas_Object *_gl_ex_sending_secure_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
	int index = (int)(ptrdiff_t)data;
	EmailSettingView *view = g_vd;
	email_account_t *account = view->account;

	if (!strcmp(part, "elm.swallow.end")) {
		Evas_Object *radio = elm_radio_add(view->genlist);
		elm_radio_group_add(radio, view->sending_secure_radio_grp);
		elm_radio_state_value_set(radio, index);

		if (index == 0) {
			elm_radio_value_set(view->sending_secure_radio_grp, account->outgoing_server_secure_connection);
		}
		evas_object_propagate_events_set(radio, EINA_FALSE);
		evas_object_smart_callback_add(radio, "changed", _gl_ex_sending_secure_radio_cb, (void *)(ptrdiff_t)index);
		return radio;
	}

	return NULL;
}

static char *_gl_ex_incoming_type_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	int index = (int)(ptrdiff_t)data;

	if (!strcmp(part, "elm.text")) {
		if (index == 0) {
			return strdup(email_setting_gettext(EMAIL_SETTING_STRING_POP3));
		} else if (index == 1) {
			return strdup(email_setting_gettext(EMAIL_SETTING_STRING_IMAP4));
		}
	}

	return NULL;
}

static Evas_Object *_gl_ex_incoming_type_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
	int index = (int)(ptrdiff_t)data;
	EmailSettingView *view = g_vd;
	email_account_t *account = view->account;

	if (!strcmp(part, "elm.swallow.end")) {
		Evas_Object *radio = elm_radio_add(view->genlist);
		elm_radio_group_add(radio, view->incoming_type_radio_grp);
		elm_radio_state_value_set(radio, index);

		if (index == 0) {
			if (account->incoming_server_type == EMAIL_SERVER_TYPE_POP3)
				elm_radio_value_set(view->incoming_type_radio_grp, 0);

			if (account->incoming_server_type == EMAIL_SERVER_TYPE_IMAP4)
				elm_radio_value_set(view->incoming_type_radio_grp, 1);
		}
		evas_object_propagate_events_set(radio, EINA_FALSE);
		evas_object_smart_callback_add(radio, "changed", _gl_ex_incoming_type_radio_cb, (void *)(ptrdiff_t)index);
		return radio;
	}

	return NULL;
}

static Evas_Object *_gl_ex_incoming_secure_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
	int index = (int)(ptrdiff_t)data;
	EmailSettingView *view = g_vd;
	email_account_t *account = view->account;

	if (!strcmp(part, "elm.swallow.end")) {
		Evas_Object *radio = elm_radio_add(obj);
		elm_radio_group_add(radio, view->incoming_secure_radio_grp);
		elm_radio_state_value_set(radio, index);

		if (index == 0) {
			elm_radio_value_set(view->incoming_secure_radio_grp, account->incoming_server_secure_connection);
		}

		evas_object_propagate_events_set(radio, EINA_FALSE);
		evas_object_smart_callback_add(radio, "changed", _gl_ex_incoming_secure_radio_cb, (void *)(ptrdiff_t)index);
		return radio;
	}

	return NULL;
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
	if (li->index == INCOMING_SERVER_TYPE_LIST_ITEM) {
		module->popup = setting_get_empty_content_notify(&view->base, &(EMAIL_SETTING_STRING_INCOMING_MAIL_SERVER_TYPE),
				0, NULL, NULL, NULL, NULL);
		genlist = _get_option_genlist(module->popup, li);
	} else if (li->index == INCOMING_SECURE_CONN_LIST_ITEM) {
		module->popup = setting_get_empty_content_notify(&view->base, &(EMAIL_SETTING_STRING_SECURE_CONNECTION),
				0, NULL, NULL, NULL, NULL);
		genlist = _get_option_genlist(module->popup, li);
	} else if (li->index == OUTGOING_SECURE_CONN_LIST_ITEM) {
		module->popup = setting_get_empty_content_notify(&view->base, &(EMAIL_SETTING_STRING_SECURE_CONNECTION),
				0, NULL, NULL, NULL, NULL);
		genlist = _get_option_genlist(module->popup, li);
	}
	elm_object_content_set(module->popup, genlist);
	evas_object_show(module->popup);
	evas_object_smart_callback_add(module->popup, "show,finished", _show_finished_cb, li);
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

		if (li->index == OUTGOING_SECURE_CONN_LIST_ITEM) {
			for (i = 0; i < 3; i++) {
				elm_genlist_item_append(genlist, view->itc4, (void *)(ptrdiff_t)i,
						NULL, ELM_GENLIST_ITEM_NONE, _gl_ex_sending_secure_sel_cb, (void *)(ptrdiff_t)i);
			}
		} else if (li->index == INCOMING_SERVER_TYPE_LIST_ITEM) {
			for (i = 0; i < 2; i++) {
				elm_genlist_item_append(genlist, view->itc5, (void *)(ptrdiff_t)i,
						NULL, ELM_GENLIST_ITEM_NONE, _gl_ex_incoming_type_sel_cb, (void *)(ptrdiff_t)i);
			}
		} else if (li->index == INCOMING_SECURE_CONN_LIST_ITEM) {
			for (i = 0; i < 3; i++) {
				elm_genlist_item_append(genlist, view->itc6, (void *)(ptrdiff_t)i,
						NULL, ELM_GENLIST_ITEM_NONE, _gl_ex_incoming_secure_sel_cb, (void *)(ptrdiff_t)i);
			}
		}

		evas_object_show(genlist);

		return genlist;
	}
	return NULL;
}

static void _gl_ex_sending_secure_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	int index = (int)(ptrdiff_t)data;
	EmailSettingView *view = g_vd;

	Elm_Object_Item *item = event_info;
	elm_genlist_item_selected_set(item, EINA_FALSE);
	elm_radio_value_set(view->sending_secure_radio_grp, index);

	_gl_ex_sending_secure_radio_cb((void *)(ptrdiff_t)index, NULL, NULL);
}

static void _gl_ex_sending_secure_radio_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	int index = (int)(ptrdiff_t)data;
	EmailSettingView *view = g_vd;
	email_account_t *account = view->account;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	switch (index) {
	case 0:
		{
			account->outgoing_server_secure_connection = 0;	/* Off */
			break;
		}
	case 1:
		{
			account->outgoing_server_secure_connection = 1;	/* SSL */
			break;
		}
	case 2:
		{
			account->outgoing_server_secure_connection = 2;	/* TLS */
			break;
		}
	}

	GSList *l = view->list_items;
	while (l) {
		ListItemData *li = l->data;
		if (li->index == OUTGOING_SECURE_CONN_LIST_ITEM) {
			elm_genlist_item_update(li->gl_it);
			elm_genlist_item_expanded_set(li->gl_it, EINA_FALSE);
			break;
		}
		l = g_slist_next(l);
	}
	DELETE_EVAS_OBJECT(module->popup);
}

static void _gl_ex_incoming_type_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	int index = (int)(ptrdiff_t)data;
	EmailSettingView *view = g_vd;

	Elm_Object_Item *item = event_info;
	elm_genlist_item_selected_set(item, EINA_FALSE);
	elm_radio_value_set(view->incoming_type_radio_grp, index);

	_gl_ex_incoming_type_radio_cb((void *)(ptrdiff_t)index, NULL, NULL);
}

static void _gl_ex_incoming_type_radio_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	int index = (int)(ptrdiff_t)data;
	EmailSettingView *view = g_vd;
	email_account_t *account = view->account;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	switch (index) {
	case 0:
		{
			_update_server_info(view, account, EMAIL_SERVER_TYPE_POP3);
			break;
		}
	case 1:
		{
			_update_server_info(view, account, EMAIL_SERVER_TYPE_IMAP4);
			break;
		}
	}

	GSList *l = view->list_items;
	while (l) {
		ListItemData *li = l->data;
		if (li->index == INCOMING_SERVER_TYPE_LIST_ITEM) {
			elm_genlist_item_update(li->gl_it);
			elm_genlist_item_expanded_set(li->gl_it, EINA_FALSE);
			break;
		}
		l = g_slist_next(l);
	}

	l = view->list_items;
	while (l) {
		ListItemData *li = l->data;
		if (li->index == INCOMING_SERVER_LIST_ITEM
				|| li->index == INCOMING_PORT_LIST_ITEM
				|| li->index == INCOMING_SECURE_CONN_LIST_ITEM) {
			elm_genlist_item_update(li->gl_it);
		}
		l = g_slist_next(l);
	}
	DELETE_EVAS_OBJECT(module->popup);
}

static void _gl_ex_incoming_secure_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	int index = (int)(ptrdiff_t)data;
	EmailSettingView *view = g_vd;

	Elm_Object_Item *item = event_info;
	elm_genlist_item_selected_set(item, EINA_FALSE);
	elm_radio_value_set(view->incoming_secure_radio_grp, index);

	_gl_ex_incoming_secure_radio_cb((void *)(ptrdiff_t)index, NULL, NULL);
}

static void _gl_ex_incoming_secure_radio_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	int index = (int)(ptrdiff_t)data;
	EmailSettingView *view = g_vd;
	email_account_t *account = view->account;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	switch (index) {
	case 0:
		{
			account->incoming_server_secure_connection = 0; /* Off */
			break;
		}
	case 1:
		{
			account->incoming_server_secure_connection = 1; /* SSL */
			break;
		}
	case 2:
		{
			account->incoming_server_secure_connection = 2; /* TLS */
			break;
		}
	}

	GSList *l = view->list_items;
	while (l) {
		ListItemData *li = l->data;
		if (li->index == INCOMING_SECURE_CONN_LIST_ITEM) {
			elm_genlist_item_update(li->gl_it);
			elm_genlist_item_expanded_set(li->gl_it, EINA_FALSE);
			break;
		}
		l = g_slist_next(l);
	}
	DELETE_EVAS_OBJECT(module->popup);
}

static void _account_validate_cb(int account_id, email_setting_response_data *response, void *user_data)
{
	debug_enter();
	EmailSettingView *view = user_data;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	retm_if(!response, "response data is NULL");
	retm_if(view->handle != response->handle, "handle is different");

	debug_log("response err: %d", response->err);
	debug_log("response type: %d", response->type);
	debug_log("local handle: %d, reponse handle: %d", view->handle, response->handle);

	/* initialize handle */
	view->handle = EMAIL_OP_HANDLE_INITIALIZER;

	if (response->err == EMAIL_ERROR_NONE ||
			response->err == EMAIL_ERROR_VALIDATE_ACCOUNT_OF_SMTP) {
		view->is_retry_validate_with_username = 0;
		DELETE_TIMER_OBJECT(view->other_vc_timer);
		if (response->err == EMAIL_ERROR_VALIDATE_ACCOUNT_OF_SMTP)
			debug_warning("smtp validation failed but it can be ignored");
		_update_account_capability(view, (const char *)(response->data));
		_update_account_smtp_mail_limit_size(view, (const char *)(response->data));
		view->other_vc_timer = ecore_timer_add(0.5, _after_validation_cb, view);
	} else if ((view->is_retry_validate_with_username <= 2) &&
			(response->err != EMAIL_ERROR_CANCELLED)) {
		view->is_retry_validate_with_username++;
		/* first retry is before @ */
		if (view->is_retry_validate_with_username == 1)
			_set_username_before_at(view);
		/* second retry is full address */
		else if (view->is_retry_validate_with_username == 2)
			_set_username_with_email_address(view);
		_validate_account(view);
	} else {
		view->is_retry_validate_with_username = 0;
		if (response->err != EMAIL_ERROR_CANCELLED) {
			const email_string_t *err_msg = setting_get_service_fail_type(response->err);
			const email_string_t *header = setting_get_service_fail_type_header(response->err);
			module->popup = setting_get_notify(&view->base,
					header, err_msg, 1,
					&(EMAIL_SETTING_STRING_OK),
					_popup_ok_cb, NULL, NULL);
		}
	}
}

static void _update_account_capability(EmailSettingView *view, const char *capability)
{
	debug_enter();
	retm_if(!view, "view is NULL");
	retm_if(!capability, "capability is NULL");

	EmailSettingModule *module = (EmailSettingModule *)view->base.module;
	email_account_t *account = module->new_account;

	debug_secure("capability: %s", capability);
	if (g_strrstr(capability, "IDLE")) {
		account->retrieval_mode = EMAIL_IMAP4_RETRIEVAL_MODE_ALL | EMAIL_IMAP4_IDLE_SUPPORTED;
	}
}

static void _update_account_smtp_mail_limit_size(EmailSettingView *view, const char *mail_limit_size)
{
	debug_enter();
	retm_if(!view, "view is NULL");
	retm_if(!mail_limit_size, "capability is NULL");

	EmailSettingModule *module = (EmailSettingModule *)view->base.module;
	email_account_t *account = module->new_account;

	debug_secure("mail limit size: %s", mail_limit_size);

	char *tmp = NULL;
	if ((tmp = g_strrstr(mail_limit_size, "SMTP_MAIL_SIZE_LIMIT"))) {
		int value = 0;
		int i = 0;
		for (i = 0; i < strlen(tmp); i++) {
			if (tmp[i] == ' ')
				break;
			if (tmp[i] >= '0' && tmp[i] <= '9')
				value = (value * 10) + (tmp[i] - '0');
		}
		account->outgoing_server_size_limit = value;
		debug_secure("mail limit size: %d", value);
	}
}

static void _update_server_info(EmailSettingView *view, email_account_t *account, email_account_server_t server)
{
	debug_enter();

	retm_if(!view, "view is NULL");
	retm_if(!account, "account is NULL");

	if (server == EMAIL_SERVER_TYPE_IMAP4) {
		setting_set_others_account_server_default_type(account, 0, 1, -1);
	} else {
		setting_set_others_account_server_default_type(account, 1, 1, -1);
	}

	GSList *l = view->list_items;
	while (l) {
		ListItemData *li = l->data;
		if (li->index == INCOMING_SERVER_LIST_ITEM) {
			FREE(li->entry_str);
			li->entry_str = g_strdup(account->incoming_server_address);
		} else if (li->index == INCOMING_PORT_LIST_ITEM) {
			if (account->incoming_server_port_number > 0) {
				char buf[10] = { 0, };
				snprintf(buf, sizeof(buf), "%d", account->incoming_server_port_number);
				FREE(li->entry_str);
				li->entry_str = g_strdup(buf);
			}
		}
		l = g_slist_next(l);
	}
}

static void _set_username_before_at(EmailSettingView *view)
{
	debug_enter();

	retm_if(!view, "view is NULL");

	EmailSettingModule *module = (EmailSettingModule *)view->base.module;
	email_account_t *account = module->new_account;

	if (account->incoming_server_user_name){
		free(account->incoming_server_user_name);
	}
	if (account->outgoing_server_user_name) {
		free(account->outgoing_server_user_name);
	}

	char *buf = g_strdup(account->user_email_address);
	char *save_ptr = NULL;
	account->incoming_server_user_name = g_strdup(strtok_r(buf, "@", &save_ptr));
	account->outgoing_server_user_name = g_strdup(account->incoming_server_user_name);
	g_free(buf);

	debug_secure("retry to validate with user name: %s", account->incoming_server_user_name);
}

static void _set_username_with_email_address(EmailSettingView *view)
{
	debug_enter();

	retm_if(!view, "view is NULL");

	EmailSettingModule *module = (EmailSettingModule *)view->base.module;
	email_account_t *account = module->new_account;

	if (account->incoming_server_user_name) {
		FREE(account->incoming_server_user_name);
	}
	if (account->outgoing_server_user_name) {
		FREE(account->outgoing_server_user_name);
	}

	if (account->user_email_address) {
		account->incoming_server_user_name = strdup(account->user_email_address);
		account->outgoing_server_user_name = strdup(account->user_email_address);
	}

	debug_secure("retry to validate with user name: %s", account->incoming_server_user_name);
}

static void _show_finished_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	ListItemData *li = data;
	int selected_index = 0;

	EmailSettingView *view = li->view;

	if (li->index == INCOMING_SERVER_TYPE_LIST_ITEM) {
		selected_index = elm_radio_value_get(view->incoming_type_radio_grp);
	} else if (li->index == INCOMING_SECURE_CONN_LIST_ITEM) {
		selected_index = elm_radio_value_get(view->incoming_secure_radio_grp);
	} else if (li->index == OUTGOING_SECURE_CONN_LIST_ITEM) {
		selected_index = elm_radio_value_get(view->sending_secure_radio_grp);
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
/* EOF */
