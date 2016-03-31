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

#include "email-setting-utils.h"
#include "email-setting-noti-mgr.h"
#include "email-view-account-edit.h"

typedef struct view_data EmailSettingView;

static email_account_t *account_data = NULL;
static EmailSettingView *g_vd = NULL;

static int _create(email_view_t *self);
static void _destroy(email_view_t *self);
static void _update(email_view_t *self, int flags);
static void _on_back_cb(email_view_t *self);

static void _push_naviframe(EmailSettingView *view);
static void _create_list(EmailSettingView *view);
static int _check_null_field(EmailSettingView *view);
static void _read_all_entries(EmailSettingView *view);
static void _free_all_entries(EmailSettingView *view);
static void _update_account_info(EmailSettingView *view);
static void _validate_account(EmailSettingView *view, int account_id);
static void _account_update_validate_cb(int account_id, email_setting_response_data *response, void *user_data);
static char *_population_password_str(int password_type);

static void _show_finished_cb(void *data, Evas_Object *obj, void *event_info);
static void _save_cb(void *data, Evas_Object *obj, void *event_info);
static void _onoff_cb(void *data, Evas_Object *obj, void *event_info);
static void _cancel_cb(void *data, Evas_Object *obj, void *event_info);
static Eina_Bool _after_save_cb(void *data);

static void _popup_ok_cb(void *data, Evas_Object *obj, void *event_info);

static void _backup_input_cb(void *data, Evas_Object *obj, void *event_info);
static void _return_key_cb(void *data, Evas_Object *obj, void *event_info);

static char *_gl_secure_text_get_cb(void *data, Evas_Object *obj, const char *part);
static char *_gl_ex_secure_text_get_cb(void *data, Evas_Object *obj, const char *part);
static char *_gl_server_type_text_get_cb(void *data, Evas_Object *obj, const char *part);
static char *_gl_onoff_text_get_cb(void *data, Evas_Object *obj, const char *part);
static char *_gl_sp_text_get_cb(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_gl_ef_content_get_cb(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_gl_ex_sending_secure_content_get_cb(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_gl_ex_incoming_secure_content_get_cb(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_gl_onoff_content_get_cb(void *data, Evas_Object *obj, const char *part);
static void _gl_secure_sel_cb(void *data, Evas_Object *obj, void *event_info);

static void _gl_ex_sending_secure_sel_cb(void *data, Evas_Object *obj, void *event_info);
static void _gl_ex_incoming_secure_sel_cb(void *data, Evas_Object *obj, void *event_info);
static void _gl_ex_sending_secure_radio_cb(void *data, Evas_Object *obj, void *event_info);
static void _gl_ex_incoming_secure_radio_cb(void *data, Evas_Object *obj, void *event_info);

static void _gl_onoff_sel_cb(void *data, Evas_Object *obj, void *event_info);

static email_string_t EMAIL_SETTING_STRING_SERVER_SETTINGS = {PACKAGE, "IDS_ST_MBODY_SERVER_SETTINGS"};
static email_string_t EMAIL_SETTING_STRING_OK = {PACKAGE, "IDS_EMAIL_BUTTON_OK"};
static email_string_t EMAIL_SETTING_STRING_PWD = {PACKAGE, "IDS_ST_TMBODY_PASSWORD"};
static email_string_t EMAIL_SETTING_STRING_USER_NAME = {PACKAGE, "IDS_ST_TMBODY_USERNAME"};
static email_string_t EMAIL_SETTING_STRING_IMAP4 = {PACKAGE, "IDS_EMAIL_BODY_IMAP4"};
static email_string_t EMAIL_SETTING_STRING_IMAP4_PORT = {PACKAGE, "IDS_ST_BODY_PORT"};
static email_string_t EMAIL_SETTING_STRING_IMAP4_SERVER = {PACKAGE, "IDS_ST_TMBODY_SERVER_ADDRESS"};
static email_string_t EMAIL_SETTING_STRING_INCOMING_MAIL_SERVER_TYPE = {PACKAGE, "IDS_ST_HEADER_SERVER_TYPE_ABB"};
static email_string_t EMAIL_SETTING_STRING_INCOMING_PORT = {PACKAGE, "IDS_ST_BODY_PORT"};
static email_string_t EMAIL_SETTING_STRING_INCOMING_SERVER = {PACKAGE, "IDS_ST_TMBODY_SERVER_ADDRESS"};
static email_string_t EMAIL_SETTING_STRING_INCOMING_SETTINGS = {PACKAGE, "IDS_ST_HEADER_INCOMING_SERVER_SETTINGS_ABB"};
static email_string_t EMAIL_SETTING_STRING_OUTGOING_PORT = {PACKAGE, "IDS_ST_BODY_PORT"};
static email_string_t EMAIL_SETTING_STRING_OUTGOING_SERVER = {PACKAGE, "IDS_ST_TMBODY_SERVER_ADDRESS"};
static email_string_t EMAIL_SETTING_STRING_OUTGOING_SETTINGS = {PACKAGE, "IDS_ST_HEADER_OUTGOING_SERVER_SETTINGS_ABB"};
static email_string_t EMAIL_SETTING_STRING_POP3 = {PACKAGE, "IDS_ST_SBODY_POP3"};
static email_string_t EMAIL_SETTING_STRING_POP3_BEFORE_SMTP = {PACKAGE, "IDS_ST_MBODY_POP3_BEFORE_SMTP"};
static email_string_t EMAIL_SETTING_STRING_POP3_PORT = {PACKAGE, "IDS_ST_BODY_PORT"};
static email_string_t EMAIL_SETTING_STRING_POP3_SERVER = {PACKAGE, "IDS_ST_TMBODY_SERVER_ADDRESS"};
static email_string_t EMAIL_SETTING_STRING_SAME_AS_POP3_IMAP4 = {PACKAGE, "IDS_ST_MBODY_SAME_AS_POP3_IMAP4"};
static email_string_t EMAIL_SETTING_STRING_SECURE_CONNECTION = {PACKAGE, "IDS_ST_HEADER_SECURITY_TYPE_ABB"};
static email_string_t EMAIL_SETTING_STRING_SMTP_AUTHENTICATION = {PACKAGE, "IDS_ST_MBODY_SMTP_AUTHENTICATION"};
static email_string_t EMAIL_SETTING_STRING_SSL = {PACKAGE, "IDS_ST_SBODY_SSL"};
static email_string_t EMAIL_SETTING_STRING_TLS = {PACKAGE, "IDS_ST_SBODY_TLS"};
static email_string_t EMAIL_SETTING_STRING_VALIDATING_ACCOUNT_ING = {PACKAGE, "IDS_ST_TPOP_VALIDATING_ACCOUNT_ING_ABB"};
static email_string_t EMAIL_SETTING_STRING_WARNING = {PACKAGE, "IDS_ST_HEADER_WARNING"};
static email_string_t EMAIL_SETTING_STRING_FILL_MANDATORY_FIELDS = {PACKAGE, "IDS_EMAIL_POP_PLEASE_FILL_ALL_THE_MANDATORY_FIELDS"};
static email_string_t EMAIL_SETTING_STRING_OFF = {PACKAGE, "IDS_ST_OPT_NONE"};
static email_string_t EMAIL_SETTING_STRING_DONE_TITLE_BTN = {PACKAGE, "IDS_TPLATFORM_ACBUTTON_DONE_ABB"};
static email_string_t EMAIL_SETTING_STRING_CANCEL_TITLE_BTN = {PACKAGE, "IDS_TPLATFORM_ACBUTTON_CANCEL_ABB"};

enum {
	OUTGOING_SETTING_TITLE_LIST_ITEM = 1,
	OUTGOING_SERVER_LIST_ITEM,
	OUTGOING_PORT_LIST_ITEM,
	OUTGOING_SECURE_CONN_LIST_ITEM,
	INCOMING_SETTING_TITLE_LIST_ITEM = 5,
	INCOMING_SERVER_TYPE_LIST_ITEM,
	POP3_SERVER_LIST_ITEM,
	POP3_PORT_LIST_ITEM,
	POP3_SECURE_CONN_LIST_ITEM,
	POP3_BEFORE_SMTP_LIST_ITEM = 10,
	SMTP_AUTH_LIST_ITEM,
	SAME_POP3_IMAP4_LIST_ITEM,
	SMTP_USERNAME_LIST_ITEM,
	SMTP_PASSWORD_LIST_ITEM
};

struct view_data {
	email_view_t base;

	GSList *list_items;
	GSList *itc_list;

	Evas_Object *sending_secure_radio_grp;
	Evas_Object *incoming_secure_radio_grp;
	Evas_Object *genlist;
	Evas_Object *save_btn;

	char *pwd_default;
	char *smtp_pwd_default;

	Ecore_Timer *edit_vc_timer;

	Elm_Genlist_Item_Class *itc_ef;
	Elm_Genlist_Item_Class *itc_secure;
	Elm_Genlist_Item_Class *itc_ex_sending;
	Elm_Genlist_Item_Class *itc_ex_incoming;
	Elm_Genlist_Item_Class *itc_type;
	Elm_Genlist_Item_Class *itc_onoff;
	Elm_Genlist_Item_Class *itc_title;
};

typedef struct _ListItemData {
	int index;
	Elm_Object_Item *gl_it;
	email_editfield_t editfield;
	char *entry_str;
	Elm_Entry_Filter_Limit_Size *entry_limit;
	Evas_Object *check;
	Eina_Bool is_checked;
	EmailSettingView *view;
} ListItemData;

static Evas_Object *_get_option_genlist(Evas_Object *parent, ListItemData *li);

void create_account_edit_view(EmailSettingModule *module)
{
	debug_enter();
	retm_if(!module, "module is null");

	EmailSettingView *view = calloc(1, sizeof(EmailSettingView));
	retm_if(!view, "memory allocation fail");

	view->base.create = _create;
	view->base.destroy = _destroy;
	view->base.update = _update;
	view->base.on_back_key = _on_back_cb;

	debug_log("view create result: %d", email_module_create_view(&module->base, &view->base));
}

static int _create(email_view_t *self)
{
	debug_enter();
	retvm_if(!self, -1, "self is null");

	EmailSettingView *view = (EmailSettingView *)self;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	if (account_data) {
		email_engine_free_account_list(&account_data, 1);
		account_data = NULL;
	}

	if (!setting_get_acct_full_data(module->account_id, &account_data)) {
		debug_error("setting_get_acct_full_data failed");
		return -1;
	}

	EMAIL_SETTING_PRINT_ACCOUNT_INFO(account_data);

	view->base.content = setting_add_inner_layout(&view->base);
	_push_naviframe(view);

	g_vd = view;

	_create_list(view);

	setting_noti_subscribe(module, EMAIL_SETTING_ACCOUNT_VALIDATE_AND_UPDATE_NOTI, _account_update_validate_cb, view);

	return 0;
}

static void _update(email_view_t *self, int flags)
{
	debug_enter();
	retm_if(!self, "self is null");

	EmailSettingView *view = (EmailSettingView *)self;

	if (flags & EVUF_LANGUAGE_CHANGED) {
		elm_genlist_realized_items_update(view->genlist);
	}
}

static void _destroy(email_view_t *self)
{
	debug_enter();
	retm_if(!self, "self is null");

	EmailSettingView *view = (EmailSettingView *)self;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	setting_noti_unsubscribe(module, EMAIL_SETTING_ACCOUNT_VALIDATE_AND_UPDATE_NOTI, _account_update_validate_cb);

	DELETE_EVAS_OBJECT(module->popup);
	DELETE_TIMER_OBJECT(view->edit_vc_timer);

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

	FREE(view->pwd_default);
	FREE(view->smtp_pwd_default);

	free(view);
}

static void _push_naviframe(EmailSettingView *view)
{
	debug_enter();
	Elm_Object_Item *navi_it = NULL;
	Evas_Object *save_btn = NULL;
	Evas_Object *cancel_btn = NULL;

	navi_it = email_module_view_push(&view->base, email_setting_gettext(EMAIL_SETTING_STRING_SERVER_SETTINGS), 0);
	elm_object_item_domain_text_translatable_set(navi_it, PACKAGE, EINA_TRUE);

	cancel_btn = elm_button_add(view->base.module->navi);
	elm_object_style_set(cancel_btn, "naviframe/title_left");
	elm_object_domain_translatable_text_set(cancel_btn, EMAIL_SETTING_STRING_CANCEL_TITLE_BTN.domain, EMAIL_SETTING_STRING_CANCEL_TITLE_BTN.id);
	evas_object_smart_callback_add(cancel_btn, "clicked", _cancel_cb, view);
	elm_object_item_part_content_set(navi_it, "title_left_btn", cancel_btn);

	view->save_btn = save_btn = elm_button_add(view->base.module->navi);
	elm_object_style_set(save_btn, "naviframe/title_right");
	elm_object_domain_translatable_text_set(save_btn, EMAIL_SETTING_STRING_DONE_TITLE_BTN.domain, EMAIL_SETTING_STRING_DONE_TITLE_BTN.id);
	evas_object_smart_callback_add(save_btn, "clicked", _save_cb, view);
	elm_object_item_part_content_set(navi_it, "title_right_btn", save_btn);

	evas_object_show(view->base.content);
}

static void _create_list(EmailSettingView *view)
{
	debug_enter();
	ListItemData *li = NULL;
	char buf[10] = {'\0', };

	retm_if(!view, "view is NULL");

	Elm_Object_Item *item = NULL;
	Elm_Object_Item *git = NULL;

	view->genlist = elm_genlist_add(view->base.content);
	elm_genlist_mode_set(view->genlist, ELM_LIST_COMPRESS);
	elm_genlist_homogeneous_set(view->genlist, EINA_TRUE);
	elm_scroller_policy_set(view->genlist, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);

	view->sending_secure_radio_grp = elm_radio_add(view->genlist);
	elm_radio_value_set(view->sending_secure_radio_grp, -1);
	evas_object_hide(view->sending_secure_radio_grp);

	view->incoming_secure_radio_grp = elm_radio_add(view->genlist);
	elm_radio_value_set(view->incoming_secure_radio_grp, -1);
	evas_object_hide(view->incoming_secure_radio_grp);

	view->itc_ef = setting_get_genlist_class_item("full", NULL, _gl_ef_content_get_cb, NULL, NULL);
	view->itc_secure = setting_get_genlist_class_item("type1", _gl_secure_text_get_cb, NULL, NULL, NULL);
	view->itc_ex_sending = setting_get_genlist_class_item("type1", _gl_ex_secure_text_get_cb, _gl_ex_sending_secure_content_get_cb, NULL, NULL);
	view->itc_ex_incoming = setting_get_genlist_class_item("type1", _gl_ex_secure_text_get_cb, _gl_ex_incoming_secure_content_get_cb, NULL, NULL);
	view->itc_type = setting_get_genlist_class_item("type1", _gl_server_type_text_get_cb, NULL, NULL, NULL);
	view->itc_onoff = setting_get_genlist_class_item("type1", _gl_onoff_text_get_cb, _gl_onoff_content_get_cb, NULL, NULL);
	view->itc_title = setting_get_genlist_class_item("group_index", _gl_sp_text_get_cb, NULL, NULL, NULL);

	view->itc_list = g_slist_append(view->itc_list, view->itc_ef);
	view->itc_list = g_slist_append(view->itc_list, view->itc_secure);
	view->itc_list = g_slist_append(view->itc_list, view->itc_ex_sending);
	view->itc_list = g_slist_append(view->itc_list, view->itc_ex_incoming);
	view->itc_list = g_slist_append(view->itc_list, view->itc_type);
	view->itc_list = g_slist_append(view->itc_list, view->itc_onoff);
	view->itc_list = g_slist_append(view->itc_list, view->itc_title);


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
	li->gl_it = item = elm_genlist_item_append(view->genlist, view->itc_type, li, NULL,
			ELM_GENLIST_ITEM_NONE, NULL, NULL);
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_NONE);
	elm_object_item_disabled_set(item, EINA_TRUE);
	view->list_items = g_slist_append(view->list_items, li);

	/* incoming server */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = POP3_SERVER_LIST_ITEM;
	li->view = view;
	li->entry_str = g_strdup(account_data->incoming_server_address);
	li->gl_it = item = elm_genlist_item_append(view->genlist, view->itc_ef, li, NULL,
			ELM_GENLIST_ITEM_NONE, NULL, NULL);
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_NONE);
	view->list_items = g_slist_append(view->list_items, li);

	/* incoming port */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = POP3_PORT_LIST_ITEM;
	li->view = view;
	snprintf(buf, sizeof(buf), "%d", account_data->incoming_server_port_number);
	li->entry_str = g_strdup(buf);
	li->gl_it = item = elm_genlist_item_append(view->genlist, view->itc_ef, li, NULL,
			ELM_GENLIST_ITEM_NONE, NULL, NULL);
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_NONE);
	view->list_items = g_slist_append(view->list_items, li);

	/* secure connection */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = POP3_SECURE_CONN_LIST_ITEM;
	li->view = view;
	li->gl_it = item = elm_genlist_item_append(view->genlist, view->itc_secure, li,
			NULL, ELM_GENLIST_ITEM_NONE, _gl_secure_sel_cb, li);
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
	li->entry_str = g_strdup(account_data->outgoing_server_address);
	li->gl_it = item = elm_genlist_item_append(view->genlist, view->itc_ef, li, NULL,
			ELM_GENLIST_ITEM_NONE, NULL, NULL);
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_NONE);
	view->list_items = g_slist_append(view->list_items, li);

	/* smtp port */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = OUTGOING_PORT_LIST_ITEM;
	li->view = view;
	snprintf(buf, sizeof(buf), "%d", account_data->outgoing_server_port_number);
	li->entry_str = g_strdup(buf);
	li->gl_it = item = elm_genlist_item_append(view->genlist, view->itc_ef, li, NULL,
			ELM_GENLIST_ITEM_NONE, NULL, NULL);
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_NONE);
	view->list_items = g_slist_append(view->list_items, li);

	/* sending security */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = OUTGOING_SECURE_CONN_LIST_ITEM;
	li->view = view;
	li->gl_it = item = elm_genlist_item_append(view->genlist, view->itc_secure, li,
			NULL, ELM_GENLIST_ITEM_NONE, _gl_secure_sel_cb, li);
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_ALWAYS);
	view->list_items = g_slist_append(view->list_items, li);

	/* SMTP Authentication */
	/* POP before SMTP */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = POP3_BEFORE_SMTP_LIST_ITEM;
	li->view = view;
	li->gl_it = item = elm_genlist_item_append(view->genlist, view->itc_onoff, li, NULL,
			ELM_GENLIST_ITEM_NONE, _gl_onoff_sel_cb, li);
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_ALWAYS);
	view->list_items = g_slist_append(view->list_items, li);

	/* SMTP Auth */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = SMTP_AUTH_LIST_ITEM;
	li->view = view;
	li->gl_it = item = elm_genlist_item_append(view->genlist, view->itc_onoff, li, NULL,
			ELM_GENLIST_ITEM_NONE, _gl_onoff_sel_cb, li);
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_ALWAYS);
	view->list_items = g_slist_append(view->list_items, li);

	/* Same as POP3/IMAP4 */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = SAME_POP3_IMAP4_LIST_ITEM;
	li->view = view;
	li->gl_it = item = elm_genlist_item_append(view->genlist, view->itc_onoff, li, NULL,
			ELM_GENLIST_ITEM_NONE, _gl_onoff_sel_cb, li);
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_ALWAYS);
	view->list_items = g_slist_append(view->list_items, li);

	/* user name */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = SMTP_USERNAME_LIST_ITEM;
	li->view = view;
	li->entry_str = account_data->outgoing_server_use_same_authenticator ?
		g_strdup(account_data->incoming_server_user_name) :
		g_strdup(account_data->outgoing_server_user_name);
	li->gl_it = item = elm_genlist_item_append(view->genlist, view->itc_ef, li, NULL,
			ELM_GENLIST_ITEM_NONE, NULL, NULL);
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_NONE);
	view->list_items = g_slist_append(view->list_items, li);

	/* password */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = SMTP_PASSWORD_LIST_ITEM;
	li->view = view;
	li->entry_str = _population_password_str(EMAIL_GET_OUTGOING_PASSWORD_LENGTH);
	if (view->smtp_pwd_default)
		free(view->smtp_pwd_default);
	view->smtp_pwd_default = g_strdup(li->entry_str);
	li->gl_it = item = elm_genlist_item_append(view->genlist, view->itc_ef, li, NULL,
			ELM_GENLIST_ITEM_NONE, NULL, NULL);
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_NONE);
	view->list_items = g_slist_append(view->list_items, li);

	if (account_data->outgoing_server_use_same_authenticator) {
		GSList *l = view->list_items;
		while (l) {
			li = l->data;
			if (li->index == SMTP_USERNAME_LIST_ITEM || li->index == SMTP_PASSWORD_LIST_ITEM) {
				elm_object_item_disabled_set(li->gl_it, EINA_TRUE);
				elm_genlist_item_update(li->gl_it);
			}
			l = g_slist_next(l);
		}
	}

	elm_object_part_content_set(view->base.content, "elm.swallow.content", view->genlist);
}

static int _check_null_field(EmailSettingView *view)
{
	debug_enter();
	int ret = TRUE;

	GSList *l = view->list_items;
	while (l) {
		ListItemData *li = l->data;
		if (li->index == SMTP_USERNAME_LIST_ITEM ||
				li->index == SMTP_PASSWORD_LIST_ITEM) {
			if (account_data->outgoing_server_use_same_authenticator == 1) {
				l = g_slist_next(l);
				continue;
			}
		}

		if (li->entry_str) {
			if (li->entry_str == NULL ||
					strlen(li->entry_str) <= 0)
				ret = FALSE;
		}
		l = g_slist_next(l);
	}

	return ret;
}

static void _read_all_entries(EmailSettingView *view)
{
	debug_enter();
	char *username = g_strdup(account_data->incoming_server_user_name);
	char *pass = g_strdup(account_data->incoming_server_password);

	_free_all_entries(view);

	GSList *l = view->list_items;
	while (l) {
		ListItemData *li = l->data;
		if (li->index == OUTGOING_SERVER_LIST_ITEM) {
			account_data->outgoing_server_address = g_strdup(li->entry_str);
			debug_secure("outgoing server - %s", account_data->outgoing_server_address);
		} else if (li->index == OUTGOING_PORT_LIST_ITEM) {
			account_data->outgoing_server_port_number = 0;
			if (li->entry_str && strlen(li->entry_str) > 0)
				account_data->outgoing_server_port_number = atoi(li->entry_str);
			debug_secure("outgoing port - %d", account_data->outgoing_server_port_number);
		} else if (li->index == POP3_SERVER_LIST_ITEM) {
			account_data->incoming_server_address = g_strdup(li->entry_str);
			debug_secure("incoming server - %s", account_data->incoming_server_address);
		} else if (li->index == POP3_PORT_LIST_ITEM) {
			account_data->incoming_server_port_number = 0;
			if (li->entry_str && strlen(li->entry_str) > 0)
				account_data->incoming_server_port_number = atoi(li->entry_str);
			debug_secure("incoming port - %d", account_data->incoming_server_port_number);
		} else if (li->index == SMTP_USERNAME_LIST_ITEM) {
			account_data->outgoing_server_user_name = account_data->outgoing_server_use_same_authenticator ?
				g_strdup(username) :
				g_strdup(li->entry_str);
			debug_secure("sending user name - %s", account_data->outgoing_server_user_name);
		} else if (li->index == SMTP_PASSWORD_LIST_ITEM) {
			if (account_data->outgoing_server_use_same_authenticator) {
				account_data->outgoing_server_password = g_strdup(pass);
			} else {
				if (!g_strcmp0(view->smtp_pwd_default, li->entry_str))
					account_data->outgoing_server_password = NULL;
				else
					account_data->outgoing_server_password = g_strdup(li->entry_str);
			}
		}
		l = g_slist_next(l);
	}

	FREE(username);
	FREE(pass);
}

static void _free_all_entries(EmailSettingView *view)
{
	debug_enter();

	FREE(account_data->incoming_server_address);
	FREE(account_data->outgoing_server_address);
	FREE(account_data->outgoing_server_user_name);
	FREE(account_data->outgoing_server_password);
}

static void _update_account_info(EmailSettingView *view)
{
	debug_enter();

	retm_if(!view, "view is NULL");

	retm_if(!account_data, "account_data is NULL");

	if (email_engine_update_account(account_data->account_id, account_data) == TRUE)
		debug_log("Account updated successfully");
}

static void _validate_account(EmailSettingView *view, int account_id)
{
	debug_enter();

	retm_if(!view, "view is NULL");

	gboolean ret;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

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
}

static void _save_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailSettingView *view = data;
	EmailSettingModule *module = NULL;

	retm_if(!view, "view is NULL");

	module = (EmailSettingModule *)view->base.module;
	module->account_id = account_data->account_id;

	_read_all_entries(view);

	DELETE_EVAS_OBJECT(module->popup);

	if (!_check_null_field(view)) {
		debug_log("Please dont leave empty fields");
		module->popup = setting_get_notify(&view->base, &(EMAIL_SETTING_STRING_WARNING),
												&(EMAIL_SETTING_STRING_FILL_MANDATORY_FIELDS), 1,
												&(EMAIL_SETTING_STRING_OK),
												_popup_ok_cb, NULL, NULL);
		return;
	}

	_validate_account(view, module->account_id);

	return;
}

static void _onoff_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	ListItemData *li = data;
	EmailSettingView *view = li->view;

	Eina_Bool state = elm_check_state_get(obj);

	if (li->index == POP3_BEFORE_SMTP_LIST_ITEM) {
		account_data->pop_before_smtp = state;
		debug_secure("pop before smtp %d", account_data->pop_before_smtp);
	} else if (li->index == SMTP_AUTH_LIST_ITEM) {
		account_data->outgoing_server_need_authentication = state;
		debug_secure("smtp auth %d", account_data->outgoing_server_need_authentication);
	} else if (li->index == SAME_POP3_IMAP4_LIST_ITEM) {
		account_data->outgoing_server_use_same_authenticator = state;
		debug_secure("same as pop3/imap4 %d", account_data->outgoing_server_use_same_authenticator);

		GSList *l = view->list_items;
		while (l) {
			ListItemData *_li = l->data;
			if (_li->index == SMTP_USERNAME_LIST_ITEM || _li->index == SMTP_PASSWORD_LIST_ITEM) {
				elm_object_item_disabled_set(_li->gl_it,
						account_data->outgoing_server_use_same_authenticator ? EINA_TRUE : EINA_FALSE);
				elm_genlist_item_update(_li->gl_it);
			} else if (_li->index == OUTGOING_PORT_LIST_ITEM) {
				elm_entry_input_panel_return_key_type_set(elm_object_item_part_content_get(_li->gl_it, "elm.icon.entry"),
						account_data->outgoing_server_use_same_authenticator ?
						ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE : ELM_INPUT_PANEL_RETURN_KEY_TYPE_NEXT);
			}
			l = g_slist_next(l);
		}
	}
}

static void _cancel_cb(void *data, Evas_Object *obj, void *event_info)
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

static Eina_Bool _after_save_cb(void *data)
{
	debug_enter();
	retvm_if(!data, ECORE_CALLBACK_CANCEL, "data is NULL");

	EmailSettingView *view = data;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	DELETE_EVAS_OBJECT(module->popup);
	DELETE_TIMER_OBJECT(view->edit_vc_timer);

	email_module_exit_view(&view->base);

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
}

static char *_gl_secure_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	ListItemData *li = data;
	email_account_t *account = account_data;

	if (!strcmp(part, "elm.text")) {
		if (li->index == OUTGOING_SECURE_CONN_LIST_ITEM) {
			return strdup(email_setting_gettext(EMAIL_SETTING_STRING_SECURE_CONNECTION));
		} else if (li->index == POP3_SECURE_CONN_LIST_ITEM) {
			return strdup(email_setting_gettext(EMAIL_SETTING_STRING_SECURE_CONNECTION));
		}
		return NULL;
	} else if (!strcmp(part, "elm.text.sub")) {
		if (li->index == OUTGOING_SECURE_CONN_LIST_ITEM) {
			if (account->outgoing_server_secure_connection == 0)
				return strdup(email_setting_gettext(EMAIL_SETTING_STRING_OFF));
			else if (account->outgoing_server_secure_connection == 1)
				return strdup(email_setting_gettext(EMAIL_SETTING_STRING_SSL));
			else if (account->outgoing_server_secure_connection == 2)
				return strdup(email_setting_gettext(EMAIL_SETTING_STRING_TLS));
			return NULL;
		} else if (li->index == POP3_SECURE_CONN_LIST_ITEM) {
			if (account->incoming_server_secure_connection == 0)
				return strdup(email_setting_gettext(EMAIL_SETTING_STRING_OFF));
			else if (account->incoming_server_secure_connection == 1)
				return strdup(email_setting_gettext(EMAIL_SETTING_STRING_SSL));
			else if (account->incoming_server_secure_connection == 2)
				return strdup(email_setting_gettext(EMAIL_SETTING_STRING_TLS));
			return NULL;
		}
		return NULL;
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
		return NULL;
	}
	return NULL;
}

static char *_gl_server_type_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	email_account_t *account = account_data;

	if (!strcmp(part, "elm.text")) {
		return strdup(email_setting_gettext(EMAIL_SETTING_STRING_INCOMING_MAIL_SERVER_TYPE));
	} else if (!strcmp(part, "elm.text.sub")) {
		if (account->incoming_server_type == EMAIL_SERVER_TYPE_POP3)
			return strdup(email_setting_gettext(EMAIL_SETTING_STRING_POP3));
		else if (account->incoming_server_type == EMAIL_SERVER_TYPE_IMAP4)
			return strdup(email_setting_gettext(EMAIL_SETTING_STRING_IMAP4));
		return NULL;
	}
	return NULL;
}

static char *_gl_onoff_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	ListItemData *li = data;

	if (!strcmp(part, "elm.text")) {
		if (li->index == POP3_BEFORE_SMTP_LIST_ITEM)
			return strdup(email_setting_gettext(EMAIL_SETTING_STRING_POP3_BEFORE_SMTP));
		else if (li->index == SMTP_AUTH_LIST_ITEM)
			return strdup(email_setting_gettext(EMAIL_SETTING_STRING_SMTP_AUTHENTICATION));
		else if (li->index == SAME_POP3_IMAP4_LIST_ITEM)
			return strdup(email_setting_gettext(EMAIL_SETTING_STRING_SAME_AS_POP3_IMAP4));
		return NULL;
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
		return NULL;
	}
	return NULL;
}

static Evas_Object *_gl_ef_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
	ListItemData *li = data;

	if (li != NULL && !strcmp(part, "elm.swallow.content")) {
		if (li->index == SMTP_PASSWORD_LIST_ITEM){
			email_common_util_editfield_create(obj, EF_PASSWORD, &li->editfield);
		} else{
			email_common_util_editfield_create(obj, 0, &li->editfield);
		}
		setting_set_entry_str(li->editfield.entry, li->entry_str);

		if (li->index == SMTP_USERNAME_LIST_ITEM) {
			elm_entry_input_panel_layout_set(li->editfield.entry, ELM_INPUT_PANEL_LAYOUT_EMAIL);
			li->entry_limit = setting_set_input_entry_limit(li->editfield.entry, 0, EMAIL_LIMIT_USER_NAME_LENGTH);
		} else if (li->index == SMTP_PASSWORD_LIST_ITEM) {
			li->entry_limit = setting_set_input_entry_limit(li->editfield.entry, EMAIL_LIMIT_PASSWORD_LENGTH, 0);
		} else if (li->index == OUTGOING_SERVER_LIST_ITEM ||
				li->index == POP3_SERVER_LIST_ITEM) {
			elm_entry_input_panel_layout_set(li->editfield.entry, ELM_INPUT_PANEL_LAYOUT_EMAIL);
			if (li->index == OUTGOING_SERVER_LIST_ITEM)
				li->entry_limit = setting_set_input_entry_limit(li->editfield.entry, 0, EMAIL_LIMIT_OUTGOING_SERVER_LENGTH);
			else if (li->index == POP3_SERVER_LIST_ITEM)
				li->entry_limit = setting_set_input_entry_limit(li->editfield.entry, 0, EMAIL_LIMIT_INCOMING_SERVER_LENGTH);
		} else if (li->index == OUTGOING_PORT_LIST_ITEM ||
				li->index == POP3_PORT_LIST_ITEM) {
			elm_entry_input_panel_layout_set(li->editfield.entry, ELM_INPUT_PANEL_LAYOUT_DATETIME);
			if (li->index == OUTGOING_PORT_LIST_ITEM)
				li->entry_limit = setting_set_input_entry_limit(li->editfield.entry, 0, EMAIL_LIMIT_OUTGOING_PORT_LENGTH);
			else if (li->index == POP3_PORT_LIST_ITEM)
				li->entry_limit = setting_set_input_entry_limit(li->editfield.entry, 0, EMAIL_LIMIT_INCOMING_PORT_LENGTH);
		}

		elm_entry_input_panel_return_key_disabled_set(li->editfield.entry, EINA_FALSE);
		if ((account_data->outgoing_server_use_same_authenticator && li->index == OUTGOING_PORT_LIST_ITEM) ||
				li->index == SMTP_PASSWORD_LIST_ITEM)
			elm_entry_input_panel_return_key_type_set(li->editfield.entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
		else
			elm_entry_input_panel_return_key_type_set(li->editfield.entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_NEXT);

		evas_object_smart_callback_add(li->editfield.entry, "changed", _backup_input_cb, li);
		evas_object_smart_callback_add(li->editfield.entry, "preedit,changed", _backup_input_cb, li);
		evas_object_smart_callback_add(li->editfield.entry, "activated", _return_key_cb, li);

		email_account_t *account = account_data;
		if (li->index == OUTGOING_SERVER_LIST_ITEM) {
			elm_object_domain_translatable_part_text_set(li->editfield.entry, "elm.guide",
					EMAIL_SETTING_STRING_OUTGOING_SERVER.domain, EMAIL_SETTING_STRING_OUTGOING_SERVER.id);
		} else if (li->index == OUTGOING_PORT_LIST_ITEM) {
			elm_object_domain_translatable_part_text_set(li->editfield.entry, "elm.guide",
					EMAIL_SETTING_STRING_OUTGOING_PORT.domain, EMAIL_SETTING_STRING_OUTGOING_PORT.id );
		} else if (li->index == POP3_SERVER_LIST_ITEM) {
			if (account->incoming_server_type == EMAIL_SERVER_TYPE_IMAP4) {
				elm_object_domain_translatable_part_text_set(li->editfield.entry, "elm.guide",
						EMAIL_SETTING_STRING_IMAP4_SERVER.domain, EMAIL_SETTING_STRING_IMAP4_SERVER.id);
			} else if (account->incoming_server_type == EMAIL_SERVER_TYPE_POP3) {
				elm_object_domain_translatable_part_text_set(li->editfield.entry, "elm.guide",
						EMAIL_SETTING_STRING_POP3_SERVER.domain, EMAIL_SETTING_STRING_POP3_SERVER.id);
			} else {
				elm_object_domain_translatable_part_text_set(li->editfield.entry, "elm.guide",
						EMAIL_SETTING_STRING_INCOMING_SERVER.domain, EMAIL_SETTING_STRING_INCOMING_SERVER.id);
			}
		} else if (li->index == POP3_PORT_LIST_ITEM) {
			if (account->incoming_server_type == EMAIL_SERVER_TYPE_IMAP4) {
				elm_object_domain_translatable_part_text_set(li->editfield.entry, "elm.guide",
						EMAIL_SETTING_STRING_IMAP4_PORT.domain, EMAIL_SETTING_STRING_IMAP4_PORT.id);
			} else if (account->incoming_server_type == EMAIL_SERVER_TYPE_POP3) {
				elm_object_domain_translatable_part_text_set(li->editfield.entry, "elm.guide",
						EMAIL_SETTING_STRING_POP3_PORT.domain, EMAIL_SETTING_STRING_POP3_PORT.id);
			} else {
				elm_object_domain_translatable_part_text_set(li->editfield.entry, "elm.guide",
						EMAIL_SETTING_STRING_INCOMING_PORT.domain, EMAIL_SETTING_STRING_INCOMING_PORT.id);
			}
		} else if (li->index == SMTP_USERNAME_LIST_ITEM) {
			elm_object_domain_translatable_part_text_set(li->editfield.entry, "elm.guide",
					EMAIL_SETTING_STRING_USER_NAME.domain, EMAIL_SETTING_STRING_USER_NAME.id);
		} else if (li->index == SMTP_PASSWORD_LIST_ITEM) {
			elm_object_domain_translatable_part_text_set(li->editfield.entry, "elm.guide",
					EMAIL_SETTING_STRING_PWD.domain, EMAIL_SETTING_STRING_PWD.id);
		}

		return li->editfield.layout;
	}

	return NULL;
}

static Evas_Object *_gl_ex_sending_secure_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
	debug_enter();
	int index = (int)(ptrdiff_t)data;
	EmailSettingView *view = g_vd;
	email_account_t *account = account_data;

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

static Evas_Object *_gl_ex_incoming_secure_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
	debug_enter();
	int index = (int)(ptrdiff_t)data;
	EmailSettingView *view = g_vd;
	email_account_t *account = account_data;

	if (!strcmp(part, "elm.swallow.end")) {
		Evas_Object *radio = elm_radio_add(view->genlist);
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

static Evas_Object *_gl_onoff_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
	debug_enter();
	ListItemData *li = data;

	if (!strcmp(part, "elm.swallow.end")) {
		li->check = elm_check_add(obj);
		elm_object_style_set(li->check, "on&off");
		evas_object_propagate_events_set(li->check, EINA_FALSE);

		if (li->index == POP3_BEFORE_SMTP_LIST_ITEM) {
			li->is_checked = account_data->pop_before_smtp;
		} else if (li->index == SMTP_AUTH_LIST_ITEM) {
			li->is_checked = account_data->outgoing_server_need_authentication;
		} else if (li->index == SAME_POP3_IMAP4_LIST_ITEM) {
			li->is_checked = account_data->outgoing_server_use_same_authenticator;
		}

		elm_check_state_set(li->check, li->is_checked);
		elm_check_state_pointer_set(li->check, &(li->is_checked));
		evas_object_smart_callback_add(li->check, "changed", _onoff_cb, li);
		return li->check;
	}

	return NULL;
}

static void _gl_secure_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	ListItemData *li = data;
	EmailSettingView *view = li->view;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;
	Elm_Object_Item *item = event_info;
	elm_genlist_item_selected_set(item, EINA_FALSE);

	Evas_Object *genlist = NULL;
	if (li->index == OUTGOING_SECURE_CONN_LIST_ITEM) {
		module->popup = setting_get_empty_content_notify(&view->base, &(EMAIL_SETTING_STRING_SECURE_CONNECTION),
				0, NULL, NULL, NULL, NULL);
		genlist = _get_option_genlist(module->popup, li);
	} else if (li->index == POP3_SECURE_CONN_LIST_ITEM) {
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

		/* sending security */
		if (li->index == OUTGOING_SECURE_CONN_LIST_ITEM) {
			for (i = 0; i < 3; i++) {
				elm_genlist_item_append(genlist, view->itc_ex_sending, (void *)(ptrdiff_t)i,
						NULL, ELM_GENLIST_ITEM_NONE, _gl_ex_sending_secure_sel_cb, (void *)(ptrdiff_t)i);
			}
		}
		/* incoming security */
		if (li->index == POP3_SECURE_CONN_LIST_ITEM) {
			for (i = 0; i < 3; i++) {
				elm_genlist_item_append(genlist, view->itc_ex_incoming, (void *)(ptrdiff_t)i,
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
	email_account_t *account = account_data;
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
	email_account_t *account = account_data;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	switch (index) {
		case 0:
			account->incoming_server_secure_connection = 0;	/* Off */
			break;
		case 1:
			account->incoming_server_secure_connection = 1;	/* SSL */
			break;
		case 2:
			account->incoming_server_secure_connection = 2;	/* TLS */
			break;
	}

	GSList *l = view->list_items;
	while (l) {
		ListItemData *li = l->data;
		if (li->index == POP3_SECURE_CONN_LIST_ITEM) {
			elm_genlist_item_update(li->gl_it);
			elm_genlist_item_expanded_set(li->gl_it, EINA_FALSE);
			break;
		}
		l = g_slist_next(l);
	}
	DELETE_EVAS_OBJECT(module->popup);
}

static void _gl_onoff_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	ListItemData *li = data;

	Elm_Object_Item *item = event_info;
	elm_genlist_item_selected_set(item, EINA_FALSE);

	elm_check_state_set(li->check, !elm_check_state_get(li->check));
	_onoff_cb(li, li->check, NULL);
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

static void _return_key_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	ListItemData *li = data;
	retm_if(!li, "ListItemData is NULL");
	retm_if(!obj, "Evas object is NULL");

	EmailSettingView *view = li->view;
	int dest_index = 0;
	Evas_Object *dest_entry = NULL;
	GSList *l = view->list_items;

	switch (li->index) {
	case POP3_SERVER_LIST_ITEM:
		dest_index = POP3_PORT_LIST_ITEM;
		break;
	case POP3_PORT_LIST_ITEM:
		dest_index = OUTGOING_SERVER_LIST_ITEM;
		break;
	case OUTGOING_SERVER_LIST_ITEM:
		dest_index = OUTGOING_PORT_LIST_ITEM;
		break;
	case OUTGOING_PORT_LIST_ITEM:
		dest_index = 0;
		if (account_data->outgoing_server_use_same_authenticator) {
			elm_entry_input_panel_hide(obj);
			_save_cb(view, NULL, NULL);
		} else
			dest_index = SMTP_USERNAME_LIST_ITEM;
		break;
	case SMTP_USERNAME_LIST_ITEM:
		dest_index = SMTP_PASSWORD_LIST_ITEM;
		break;
	case SMTP_PASSWORD_LIST_ITEM:
		dest_index = 0;
		elm_entry_input_panel_hide(obj);
		_save_cb(view, NULL, NULL);
		break;
	}


	while (dest_index && l) {
		ListItemData *_li = l->data;
		if (_li->index == dest_index) {
			dest_entry = elm_object_item_part_content_get(_li->gl_it, "elm.icon.entry");
			elm_object_focus_set(dest_entry, EINA_TRUE);
			elm_genlist_item_bring_in(_li->gl_it, ELM_GENLIST_ITEM_SCROLLTO_IN);
			elm_entry_cursor_end_set(dest_entry);
			dest_index = 0;
		}
		l = g_slist_next(l);
	}
}

static void _show_finished_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	ListItemData *li = data;
	EmailSettingView *view = li->view;
	int selected_index = 0;

	if (li->index == POP3_SECURE_CONN_LIST_ITEM) {
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
