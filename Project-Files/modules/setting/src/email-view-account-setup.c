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
#include "email-setting-account-set.h"
#include "email-setting-noti-mgr.h"

#include "email-view-account-setup.h"
#include "email-view-manual-setup.h"
#include "email-view-account-details-setup.h"

typedef struct view_data EmailSettingVD;

static EmailSettingVD *g_vd = NULL;

static int _create(email_view_t *self);
static void _destroy(email_view_t *self);
static void _activate(email_view_t *self, email_view_state prev_state);
static void _update(email_view_t *self, int flags);
static void _on_back_key(email_view_t *self);

static void _push_naviframe(EmailSettingVD *vd);
static void _create_view(EmailSettingVD *vd);
static void _initialize_handle(EmailSettingVD *vd);
static void _create_list(EmailSettingVD *vd);
static void _create_validation_popup(EmailSettingVD *vd);
static void _validate_account(void *data);
static void _account_validate_cb(int account_id, email_setting_response_data *response, void *user_data);
static int _check_null_field(EmailSettingVD *vd);
static void _read_all_entries(EmailSettingVD *vd);
static void _set_domain_string(EmailSettingVD *vd);
static void _set_username_before_at(EmailSettingVD *vd);
static void _popup_list_select_cb(void *data, Evas_Object *obj, void *event_info);
static char *_popup_list_text_get_cb(void *data, Evas_Object *obj, const char *part);
static void _update_account_capability(EmailSettingVD *vd, const char *capability);
static void _update_account_smtp_mail_limit_size(EmailSettingVD *vd, const char *mail_limit_size);

static char *_gl_text_get_cb(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_gl_content_get_cb(void *data, Evas_Object *obj, const char *part);
static void _set_cb(void *data, Evas_Object *obj, void *event_info);
static void _next_cb(void *data, Evas_Object *obj, void *event_info);
static void _save_cb(void *data, Evas_Object *obj, void *event_info);
static void _login_cb(void *data, Evas_Object *obj, void *event_info);
static void _manual_cb(void *data, Evas_Object *obj, void *event_info);
static void _check_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static Eina_Bool _after_validation_cb(void *data);
static Eina_Bool _startup_focus_cb(void *data);
static void _popup_ok_cb(void *data, Evas_Object *obj, void *event_info);
static void _backup_input_cb(void *data, Evas_Object *obj, void *event_info);
static void _return_key_cb(void *data, Evas_Object *obj, void *event_info);
static void _gl_sel_cb(void *data, Evas_Object *obj, void *event_info);

static email_setting_string_t EMAIL_SETTING_STRING_ALREADY_EXIST = {PACKAGE, "IDS_ST_POP_THIS_ACCOUNT_HAS_ALREADY_BEEN_ADDED"};
static email_setting_string_t EMAIL_SETTING_STRING_EMAIL_ADDRESS = {PACKAGE, "IDS_EMAIL_BODY_EMAIL_ADDRESS"};
static email_setting_string_t EMAIL_SETTING_STRING_NEXT = {PACKAGE, "IDS_ST_BUTTON_NEXT"};
static email_setting_string_t EMAIL_SETTING_STRING_OK = {PACKAGE, "IDS_EMAIL_BUTTON_OK"};
static email_setting_string_t EMAIL_SETTING_STRING_OTHERS = {PACKAGE, "IDS_ST_MBODY_OTHER"};
static email_setting_string_t EMAIL_SETTING_STRING_SK_ADD_ACCOUNT = {PACKAGE, "IDS_EMAIL_OPT_ADD_ACCOUNT"};
static email_setting_string_t EMAIL_SETTING_STRING_ACCOUNT_ALREADY_EXISTS = {PACKAGE, "IDS_ST_POP_THIS_ACCOUNT_HAS_ALREADY_BEEN_ADDED"};
static email_setting_string_t EMAIL_SETTING_STRING_EMAIL = {PACKAGE, "IDS_ST_HEADER_EMAIL"};
static email_setting_string_t EMAIL_SETTING_STRING_IMAP = {PACKAGE, "IDS_ST_SBODY_IMAP"};
static email_setting_string_t EMAIL_SETTING_STRING_INVALID_EMAIL_ADDRESS = {PACKAGE, "IDS_EMAIL_TPOP_INVALID_EMAIL_ADDRESS_ENTERED"};
static email_setting_string_t EMAIL_SETTING_STRING_MANUAL_SETUP = {PACKAGE, "IDS_ST_BUTTON_MANUAL_SETUP_ABB"};
static email_setting_string_t EMAIL_SETTING_STRING_POP3 = {PACKAGE, "IDS_ST_SBODY_POP3"};
static email_setting_string_t EMAIL_SETTING_STRING_SELECT_TYPE_OF_ACCOUNT = {PACKAGE, "IDS_ST_HEADER_SELECT_ACCOUNT_TYPE_ABB"};
static email_setting_string_t EMAIL_SETTING_STRING_SEND_EMAIL_FROM_THIS_ACCOUNT_BY_DEFAULT = {PACKAGE, "IDS_ST_SBODY_SET_AS_DEFAULT_ACCOUNT_ABB"};
static email_setting_string_t EMAIL_SETTING_STRING_SHOW_PASSWORD = {PACKAGE, "IDS_ST_SBODY_SHOW_PASSWORD_ABB"};
static email_setting_string_t EMAIL_SETTING_STRING_UNABLE_TO_ADD_ACCOUNT = {PACKAGE, "IDS_ST_HEADER_UNABLE_TO_ADD_ACCOUNT_ABB"};
static email_setting_string_t EMAIL_SETTING_STRING_VALIDATING_ACCOUNT_ING = {PACKAGE, "IDS_ST_TPOP_VALIDATING_ACCOUNT_ING_ABB"};
static email_setting_string_t EMAIL_SETTING_STRING_ENTER_PASS = {PACKAGE, "IDS_ST_TMBODY_PASSWORD"};
static email_setting_string_t EMAIL_SETTING_STRING_FILL_MANDATORY_FIELDS = {PACKAGE, "IDS_EMAIL_POP_PLEASE_FILL_ALL_THE_MANDATORY_FIELDS"};
static email_setting_string_t EMAIL_SETTING_STRING_SERVER_QUERY_FAIL = {PACKAGE, "IDS_EMAIL_POP_SERVER_INFORMATION_QUERY_FAILED_ENTER_SERVER_INFORMATION_MANUALLY"};
static email_setting_string_t EMAIL_SETTING_STRING_WARNING = {PACKAGE, "IDS_ST_HEADER_WARNING"};

enum {
	EMAIL_ADDRESS_LIST_ITEM = 1,
	PASSWORD_LIST_ITEM,
	DEFAULT_ACCOUNT_LIST_ITEM,
	SHOW_PASSWORD_LIST_ITEM,
};

struct view_data {
	email_view_t base;

	int handle;
	int is_retry_validate_with_username;

	GSList *list_items;
	GSList *itc_list;

	char *str_email_address;
	char *str_password;
	char *str_password_before;

	Evas_Object *genlist;
	email_editfield_t entry_email_address;
	email_editfield_t entry_password;

	Evas_Object *default_account_check;
	Evas_Object *show_passwd_check;
	Eina_Bool is_show_passwd_check;

	Evas_Object *login_c_btn;
	Evas_Object *manual_c_btn;

	Ecore_Timer *preset_vc_timer;
	Ecore_Timer *focus_timer;

	Elm_Genlist_Item_Class *itc1;
	Elm_Genlist_Item_Class *itc2;
};

typedef struct _ListItemData {
	int index;
	Elm_Entry_Filter_Limit_Size *entry_limit;
	EmailSettingVD *vd;
} ListItemData;

void create_account_setup_view(EmailSettingUGD *ugd, email_add_account_e account_type, char *sp_name, char *sp_icon_path)
{
	debug_enter();

	retm_if(!ugd, "ug data is null");

	EmailSettingVD *vd = calloc(1, sizeof(EmailSettingVD));
	retm_if(!vd, "view data is null");

	vd->base.create = _create;
	vd->base.destroy = _destroy;
	vd->base.activate = _activate;
	vd->base.update = _update;
	vd->base.on_back_key = _on_back_key;

	FREE(ugd->email_sp);
	FREE(ugd->email_sp_icon_path);
	ugd->email_sp = g_strdup(sp_name);
	ugd->email_sp_icon_path = g_strdup(sp_icon_path);
	ugd->add_account_type = account_type;
	ugd->is_set_default_account = EINA_FALSE;

	debug_log("view create result: %d", email_module_create_view(&ugd->base, &vd->base));
}

static int _create(email_view_t *self)
{
	debug_enter();
	retvm_if(!self, -1, "self is null");

	EmailSettingVD *vd = (EmailSettingVD *)self;
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;

	vd->base.content = setting_add_inner_layout(&vd->base);
	_push_naviframe(vd);

	g_vd = vd;
	if (ugd->new_account)
		setting_new_acct_final(&vd->base);
	setting_new_acct_init(&vd->base);

	if (ugd->add_account_type == EMAIL_ADD_ACCOUNT_TYPE_CSC) {
		setting_set_csc_account(&vd->base);
		_set_domain_string(vd);
	}

	ugd->account_id = 0;

	DELETE_TIMER_OBJECT(vd->focus_timer);
	vd->focus_timer = ecore_timer_add(0.5, _startup_focus_cb, vd);

	_create_view(vd);

	setting_noti_subscribe(ugd, EMAIL_SETTING_ACCOUNT_VALIDATE_NOTI, _account_validate_cb, vd);

	_initialize_handle(vd);

	return 0;
}

static void _update(email_view_t *self, int flags)
{
	debug_enter();
	retm_if(!self, "self is null");

	EmailSettingVD *vd = (EmailSettingVD *)self;

	if (flags & EVUF_POPPING) {
		_initialize_handle(vd);
	}
}

static void _initialize_handle(EmailSettingVD *vd)
{
	debug_enter();
	retm_if(!vd, "vd is null");

	/* initialize handle to distinguish with manual setup's callback */
	vd->handle = EMAIL_OP_HANDLE_INITIALIZER;
}

static void _destroy(email_view_t *self)
{
	debug_enter();
	retm_if(!self, "self is null");

	EmailSettingVD *vd = (EmailSettingVD *)self;
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;

	setting_noti_unsubscribe(ugd, EMAIL_SETTING_ACCOUNT_VALIDATE_NOTI, _account_validate_cb);
	DELETE_EVAS_OBJECT(ugd->popup);
	DELETE_TIMER_OBJECT(vd->focus_timer);
	DELETE_TIMER_OBJECT(vd->preset_vc_timer);
	FREE(vd->str_password);
	FREE(vd->str_password_before);


	GSList *l = vd->list_items;
	while (l) {
		ListItemData *li = l->data;
		free(li->entry_limit);
		free(li);
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

	setting_new_acct_final(&vd->base);

	free(vd);
}

static void _activate(email_view_t *self, email_view_state prev_state)
{
	debug_enter();
	retm_if(!self, "self is null");

	EmailSettingVD *vd = (EmailSettingVD *)self;
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;

	if (prev_state != EV_STATE_CREATED) {
		return;
	}

	setting_load_provider_list(&(ugd->default_provider_list), NULL, EMAIL_SETTING_DEFAULT_PROVIDER_XML_FILENAME);
}

static void _push_naviframe(EmailSettingVD *vd)
{
	debug_enter();
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;
	Elm_Object_Item *navi_it = NULL;

	int push_flags = ((ugd->base.views_count == 1) ? EVPF_NO_TRANSITION : 0);

	if (ugd->add_account_type == EMAIL_ADD_ACCOUNT_TYPE_CSC) {
		navi_it = email_module_view_push(&vd->base, ugd->email_sp, push_flags);
	} else if (ugd->add_account_type == EMAIL_ADD_ACCOUNT_TYPE_EMAIL) {
		navi_it = email_module_view_push(&vd->base, EMAIL_SETTING_STRING_EMAIL.id, push_flags);
		elm_object_item_domain_text_translatable_set(navi_it, EMAIL_SETTING_STRING_EMAIL.domain, EINA_TRUE);
	} else {
		navi_it = email_module_view_push(&vd->base, EMAIL_SETTING_STRING_OTHERS.id, push_flags);
		elm_object_item_domain_text_translatable_set(navi_it, EMAIL_SETTING_STRING_OTHERS.domain, EINA_TRUE);
	}

	Evas_Object *btn = NULL;
	Evas_Object *btn_ly = NULL;
	btn_ly = elm_layout_add(vd->base.content);
	elm_layout_file_set(btn_ly, email_get_setting_theme_path(), "two_bottom_btn");

	btn = elm_button_add(btn_ly);
	elm_object_style_set(btn, "bottom");
	elm_object_domain_translatable_text_set(btn, EMAIL_SETTING_STRING_MANUAL_SETUP.domain, EMAIL_SETTING_STRING_MANUAL_SETUP.id);
	evas_object_smart_callback_add(btn, "clicked", _manual_cb, vd);
	vd->manual_c_btn = btn;
	elm_object_disabled_set(btn, EINA_TRUE);
	elm_layout_content_set(btn_ly, "btn1.swallow", btn);

	btn = elm_button_add(btn_ly);
	elm_object_style_set(btn, "bottom");
	elm_object_domain_translatable_text_set(btn, EMAIL_SETTING_STRING_NEXT.domain, EMAIL_SETTING_STRING_NEXT.id);
	evas_object_smart_callback_add(btn, "clicked", _set_cb, vd);
	vd->login_c_btn = btn;
	elm_object_disabled_set(btn, EINA_TRUE);
	elm_layout_content_set(btn_ly, "btn2.swallow", btn);

	elm_object_item_part_content_set(navi_it, "toolbar", btn_ly);

	evas_object_show(vd->base.content);
}

static void _create_view(EmailSettingVD *vd)
{
	debug_enter();

	retm_if(!vd, "view data is null");

	_create_list(vd);

	elm_object_part_content_set(vd->base.content, "elm.swallow.content", vd->genlist);
}

static void _create_list(EmailSettingVD *vd)
{
	debug_enter();

	retm_if(!vd, "view data is null");

	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;

	vd->genlist = elm_genlist_add(vd->base.content);
	elm_genlist_mode_set(vd->genlist, ELM_LIST_COMPRESS);
	elm_scroller_policy_set(vd->genlist, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);

	vd->itc1 = setting_get_genlist_class_item("full", NULL, _gl_content_get_cb, NULL, NULL);
	vd->itc2 = setting_get_genlist_class_item("type1", _gl_text_get_cb, _gl_content_get_cb, NULL, NULL);

	vd->itc_list = g_slist_append(vd->itc_list, vd->itc1);
	vd->itc_list = g_slist_append(vd->itc_list, vd->itc2);

	ListItemData *li = NULL;
	Elm_Object_Item *item = NULL;

	/*email address*/
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = EMAIL_ADDRESS_LIST_ITEM;
	li->vd = vd;
	item = elm_genlist_item_append(vd->genlist, vd->itc1, li, NULL,
			ELM_GENLIST_ITEM_NONE, NULL, NULL);
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_NONE);
	vd->list_items = g_slist_append(vd->list_items, li);

	/*password*/
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = PASSWORD_LIST_ITEM;
	li->vd = vd;
	item = elm_genlist_item_append(vd->genlist, vd->itc1, li, NULL,
			ELM_GENLIST_ITEM_NONE, NULL, NULL);
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_NONE);
	vd->list_items = g_slist_append(vd->list_items, li);

	/*show password*/
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = SHOW_PASSWORD_LIST_ITEM;
	li->vd = vd;
	item = elm_genlist_item_append(vd->genlist, vd->itc2, li, NULL,
			ELM_GENLIST_ITEM_NONE, _gl_sel_cb, li);
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_ALWAYS);
	vd->list_items = g_slist_append(vd->list_items, li);

	if (!(ugd->account_list == NULL || ugd->account_count == 0)) {
		/*set as default account*/
		li = calloc(1, sizeof(ListItemData));
		retm_if(!li, "memory allocation failed");

		li->index = DEFAULT_ACCOUNT_LIST_ITEM;
		li->vd = vd;
		item = elm_genlist_item_append(vd->genlist, vd->itc2, li, NULL,
				ELM_GENLIST_ITEM_NONE, _gl_sel_cb, li);
		elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_ALWAYS);
		vd->list_items = g_slist_append(vd->list_items, li);
	}
}

static char *_gl_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	ListItemData *li = data;
	if (li) {
		if (li->index == DEFAULT_ACCOUNT_LIST_ITEM && !strcmp("elm.text", part))
			return g_strdup(email_setting_gettext(EMAIL_SETTING_STRING_SEND_EMAIL_FROM_THIS_ACCOUNT_BY_DEFAULT));
		else if (li->index == SHOW_PASSWORD_LIST_ITEM && !strcmp("elm.text", part))
			return g_strdup(email_setting_gettext(EMAIL_SETTING_STRING_SHOW_PASSWORD));
	}
	return NULL;
}

static Evas_Object *_gl_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
	ListItemData *li = data;
	if (li) {
		EmailSettingVD *vd = li->vd;
		EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;
		Evas_Object *check = NULL;
		if ((li->index == EMAIL_ADDRESS_LIST_ITEM || li->index == PASSWORD_LIST_ITEM) && !strcmp("elm.swallow.content", part)) {
			Elm_Entry_Filter_Limit_Size *entry_limit = NULL;
			if (li->index == EMAIL_ADDRESS_LIST_ITEM) {
				email_common_util_editfield_create(obj, 0, &vd->entry_email_address);
				elm_entry_input_panel_layout_set(vd->entry_email_address.entry, ELM_INPUT_PANEL_LAYOUT_EMAIL);
				elm_entry_input_panel_return_key_type_set(vd->entry_email_address.entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_NEXT);
				elm_object_domain_translatable_part_text_set(vd->entry_email_address.entry, "elm.guide",
						EMAIL_SETTING_STRING_EMAIL_ADDRESS.domain, EMAIL_SETTING_STRING_EMAIL_ADDRESS.id);
				entry_limit = setting_set_input_entry_limit(vd->entry_email_address.entry, 0, EMAIL_LIMIT_EMAIL_ADDRESS_LENGTH);
				setting_set_entry_str(vd->entry_email_address.entry, vd->str_email_address);
				evas_object_smart_callback_add(vd->entry_email_address.entry, "changed", _backup_input_cb, li);
				evas_object_smart_callback_add(vd->entry_email_address.entry, "preedit,changed", _backup_input_cb, li);
				evas_object_smart_callback_add(vd->entry_email_address.entry, "activated", _return_key_cb, li);
				li->entry_limit = entry_limit;
				return vd->entry_email_address.layout;

			} else if (li->index == PASSWORD_LIST_ITEM && !strcmp("elm.swallow.content", part)) {
				email_common_util_editfield_create(obj, EF_PASSWORD, &vd->entry_password);
				elm_entry_input_panel_return_key_type_set(vd->entry_password.entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
				elm_entry_input_panel_return_key_disabled_set(vd->entry_password.entry, EINA_TRUE);
				elm_object_domain_translatable_part_text_set(vd->entry_password.entry, "elm.guide",
						EMAIL_SETTING_STRING_ENTER_PASS.domain, EMAIL_SETTING_STRING_ENTER_PASS.id);
				entry_limit = setting_set_input_entry_limit(vd->entry_password.entry, 0, EMAIL_LIMIT_PASSWORD_LENGTH);
				elm_entry_password_set(vd->entry_password.entry, !vd->is_show_passwd_check);
				setting_set_entry_str(vd->entry_password.entry, vd->str_password);
				evas_object_smart_callback_add(vd->entry_password.entry, "changed", _backup_input_cb, li);
				evas_object_smart_callback_add(vd->entry_password.entry, "preedit,changed", _backup_input_cb, li);
				evas_object_smart_callback_add(vd->entry_password.entry, "activated", _return_key_cb, li);
				li->entry_limit = entry_limit;
				return vd->entry_password.layout;
			}

		} else if (li->index == DEFAULT_ACCOUNT_LIST_ITEM && !strcmp("elm.swallow.end", part)) {
			check = elm_check_add(obj);
			elm_check_state_pointer_set(check, &(ugd->is_set_default_account));
			elm_object_focus_allow_set(check, EINA_FALSE);
			evas_object_propagate_events_set(check, EINA_FALSE);
			vd->default_account_check = check;
			evas_object_smart_callback_add(check, "changed", _check_clicked_cb, li);
			return check;
		} else if (li->index == SHOW_PASSWORD_LIST_ITEM && !strcmp("elm.swallow.end", part)) {
			check = elm_check_add(obj);
			elm_check_state_pointer_set(check, &(vd->is_show_passwd_check));
			elm_object_focus_allow_set(check, EINA_FALSE);
			evas_object_propagate_events_set(check, EINA_FALSE);
			vd->show_passwd_check = check;
			evas_object_smart_callback_add(check, "changed", _check_clicked_cb, li);
			return check;
		}
	}
	return NULL;
}

static void _create_validation_popup(EmailSettingVD *vd)
{
	debug_enter();

	retm_if(!vd, "view data is null");

	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;
	email_account_t *account = ugd->new_account;

	debug_secure("account name:%s", account->account_name);
	debug_secure("email address:%s", account->user_email_address);

	debug_log("Start Account Validation");
	ugd->popup = setting_get_pb_process_notify(&vd->base,
			&(EMAIL_SETTING_STRING_VALIDATING_ACCOUNT_ING), 0,
			NULL, NULL,
			NULL, NULL,
			POPUP_BACK_TYPE_DESTROY_WITH_CANCEL_OP, &(vd->handle));
}

static void _validate_account(void *data)
{
	debug_enter();

	EmailSettingVD *vd = data;

	retm_if(!vd, "view data is null");

	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;
	int error_code = 0;

	vd->handle = EMAIL_OP_HANDLE_INITIALIZER;

#ifdef _TESTING_WITH_STATIC_EMAIL_ACC_

	debug_warning("*** _TESTING_WITH_STATIC_EMAIL_ACC_ ***");

	char *address_string = strdup(ugd->new_account->account_name);
	char *id_string = strdup(ugd->new_account->account_name);
	char *password_string = strdup(ugd->new_account->incoming_server_password);

	void *user_data = ugd->new_account->user_data;
	int length = ugd->new_account->user_data_length;

	// free current account
	free(ugd->new_account);
	email_account_t *account = malloc(sizeof(email_account_t));
	memset(account, 0x00, sizeof(email_account_t));

	/* Common Options */
	account->retrieval_mode                          = EMAIL_IMAP4_RETRIEVAL_MODE_ALL;
	account->incoming_server_secure_connection	     = 1;
	account->outgoing_server_type                    = EMAIL_SERVER_TYPE_SMTP;
	account->auto_download_size			             = 2;
	account->outgoing_server_use_same_authenticator  = 1;
	account->auto_resend_times                       = 3;
	account->pop_before_smtp                         = 0;
	account->incoming_server_requires_apop           = 0;
	account->incoming_server_authentication_method   = 0;
	account->logo_icon_path                          = NULL;
	account->color_label                             = (128 << 16) | (128 << 8) | (128);

	account->user_data                               = user_data;
	account->user_data_length                        = length;

	account->options.priority                        = 3;
	account->options.keep_local_copy                 = 1;
	account->options.req_delivery_receipt            = 0;
	account->options.req_read_receipt                = 0;
	account->options.download_limit                  = 0;
	account->options.block_address                   = 0;
	account->options.block_subject                   = 0;
	account->options.display_name_from               = NULL;
	account->options.reply_with_body                 = 0;
	account->options.forward_with_files              = 0;
	account->options.add_myname_card                 = 0;
	account->options.add_signature                   = 0;
	account->options.signature                       = NULL;
	account->options.add_my_address_to_bcc           = 0;
	account->check_interval                          = 0;
	account->keep_mails_on_pop_server_after_download = 1;
	account->default_mail_slot_size                  = 20;
	account->roaming_option                          = EMAIL_ROAMING_OPTION_RESTRICTED_BACKGROUND_TASK;
	account->peak_interval                           = 30;
	account->peak_days                               = EMAIL_PEAK_DAYS_MONDAY | EMAIL_PEAK_DAYS_TUEDAY | EMAIL_PEAK_DAYS_THUDAY | EMAIL_PEAK_DAYS_FRIDAY;
	account->peak_start_time                         = 830;
	account->peak_end_time                           = 1920;

	account->account_name                            = strdup(address_string);
	account->user_display_name                       = strdup(id_string);
	account->user_email_address                      = strdup(address_string);
	account->reply_to_address                        = strdup(address_string);
	account->return_address                          = strdup(address_string);

	account->incoming_server_user_name               = strdup(id_string);
	account->incoming_server_password                = strdup(password_string);
	account->outgoing_server_user_name               = strdup(id_string);
	account->outgoing_server_password	             = strdup(password_string);

	account->incoming_server_type  = EMAIL_SERVER_TYPE_IMAP4;
	account->incoming_server_address = strdup("imap.gmail.com");
	account->incoming_server_port_number = 993;
	account->incoming_server_secure_connection	= 1;
	account->outgoing_server_address = strdup("smtp.gmail.com");
	account->outgoing_server_port_number = 465;
	account->outgoing_server_secure_connection = 1;
	account->outgoing_server_need_authentication   = EMAIL_AUTHENTICATION_METHOD_DEFAULT;
	account->incoming_server_authentication_method = EMAIL_AUTHENTICATION_METHOD_NO_AUTH;
	account->account_svc_id = rand()%100;

	ugd->new_account = account;

	free(address_string);
	free(id_string);
	free(password_string);

#endif

	if (email_engine_validate_account(ugd->new_account, &(vd->handle), &error_code)) {
		debug_log("Validate account");
		_create_validation_popup(vd);
	} else {
		debug_error("Fail to make account");

		if (error_code == EMAIL_ERROR_ALREADY_EXISTS)
			ugd->popup = setting_get_notify(&vd->base,
					&(EMAIL_SETTING_STRING_WARNING),
					&(EMAIL_SETTING_STRING_ALREADY_EXIST), 1,
					&(EMAIL_SETTING_STRING_OK),
					_popup_ok_cb, NULL, NULL);
		else
			ugd->popup = setting_get_notify(&vd->base,
					&(EMAIL_SETTING_STRING_WARNING),
					&(EMAIL_SETTING_STRING_UNABLE_TO_ADD_ACCOUNT), 1,
					&(EMAIL_SETTING_STRING_OK),
					_popup_ok_cb, NULL, NULL);
	}
}

static int _check_null_field(EmailSettingVD *vd)
{
	debug_enter();

	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;
	email_account_t *account = ugd->new_account;

	retvm_if(!account, FALSE, "account is null");

	if (!STR_VALID(account->user_email_address) ||
			!STR_VALID(account->incoming_server_password))
		return FALSE;
	else
		return TRUE;
}

static void _read_all_entries(EmailSettingVD *vd)
{
	debug_enter();

	retm_if(!vd, "vd is null");

	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;
	email_account_t *account = ugd->new_account;

	FREE(account->user_email_address);
	FREE(account->incoming_server_password);

	account->user_email_address = g_strdup(vd->str_email_address);
	account->incoming_server_password = g_strdup(vd->str_password);

	if (ugd->add_account_type != EMAIL_ADD_ACCOUNT_TYPE_CSC) {
		FREE(account->account_name);
		account->account_name = g_strdup(vd->str_email_address);
	}
}

static void _set_domain_string(EmailSettingVD *vd)
{
	debug_enter();
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;

	if (ugd->add_account_type == EMAIL_ADD_ACCOUNT_TYPE_CSC) {
		vd->str_email_address = g_strdup(ugd->new_account->user_email_address);
	}
}

static void _next_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailSettingVD *vd = data;

	retm_if(!vd, "vd is null");

	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;
	email_account_t *account = ugd->new_account;
	char *buf = NULL;
	int ret = 0;

	/* Save the Account Data */
	_read_all_entries(vd);

	debug_secure("accountStructure Info");
	debug_secure("Account Name: %s", account->account_name);
	debug_secure("Display Name: %s", account->user_display_name);
	debug_secure("Email Addr: %s", account->user_email_address);
	debug_secure("User Name: %s", account->incoming_server_user_name);

	/* check Null field */
	ret = _check_null_field(vd);
	if (!ret) {
		ugd->popup = setting_get_notify(&vd->base,
				&(EMAIL_SETTING_STRING_WARNING),
				&(EMAIL_SETTING_STRING_FILL_MANDATORY_FIELDS), 1,
				&(EMAIL_SETTING_STRING_OK),
				_popup_ok_cb, NULL, NULL);
		return;
	}

	/* check character validation */
	buf = g_strdup(account->user_email_address);
	ret = email_get_address_validation(buf);
	if (!ret) {
		ugd->popup = setting_get_notify(&vd->base,
				&(EMAIL_SETTING_STRING_WARNING),
				&(EMAIL_SETTING_STRING_INVALID_EMAIL_ADDRESS), 1,
				&(EMAIL_SETTING_STRING_OK),
				_popup_ok_cb, NULL, NULL);
		g_free(buf);
		return;
	}

	/* check duplication account */
	ret = setting_is_duplicate_account(buf);
	if (ret < 0) {
		ugd->popup = setting_get_notify(&vd->base,
				&(EMAIL_SETTING_STRING_WARNING),
				&(EMAIL_SETTING_STRING_ACCOUNT_ALREADY_EXISTS), 1,
				&(EMAIL_SETTING_STRING_OK),
				_popup_ok_cb, NULL, NULL);
		g_free(buf);
		return;
	}

	/* account info setting */
	ret = setting_is_in_default_provider_list(&vd->base, buf);
	if (ret) {
		setting_set_default_provider_info_to_account(&vd->base, account);
		_validate_account(vd);
	} else {
		/* fail for _get_login_account_info_cb */
		DELETE_EVAS_OBJECT(ugd->popup);
		ugd->popup = setting_get_notify(&vd->base,
				&(EMAIL_SETTING_STRING_SK_ADD_ACCOUNT),
				&(EMAIL_SETTING_STRING_SERVER_QUERY_FAIL),
				1, &(EMAIL_SETTING_STRING_OK),
				_popup_ok_cb, NULL, NULL);
	}

	g_free(buf);
}

static void _save_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailSettingVD *vd = data;

	retm_if(!vd, "vd is null");

	EmailSettingUGD *ugd = NULL;
	char *buf = NULL;
	int ret = 0;

	ugd = (EmailSettingUGD *)vd->base.module;

	/* Save the data */
	_read_all_entries(vd);
	email_account_t *account = ugd->new_account;

	debug_secure("accountStructure Info");
	debug_secure("Display Name:%s", account->user_display_name);
	debug_secure("Email Addr:%s", account->user_email_address);

	/* check Null field */
	ret = _check_null_field(vd);
	if (!ret) {
		debug_error("empty space in account info");
		ugd->popup = setting_get_notify(&vd->base,
				&(EMAIL_SETTING_STRING_WARNING),
				&(EMAIL_SETTING_STRING_FILL_MANDATORY_FIELDS),
				1, &(EMAIL_SETTING_STRING_OK),
				_popup_ok_cb, NULL, NULL);
		return;
	}

	/* check character validation */
	buf = g_strdup(account->user_email_address);

	ret = email_get_address_validation(buf);
	if (!ret) {
		debug_error("Wrong character in email address");
		ugd->popup = setting_get_notify(&vd->base,
				&(EMAIL_SETTING_STRING_WARNING),
				&(EMAIL_SETTING_STRING_INVALID_EMAIL_ADDRESS),
				1, &(EMAIL_SETTING_STRING_OK),
				_popup_ok_cb, NULL, NULL);
		g_free(buf);
		return;
	}

	/* check duplication account */
	ret = setting_is_duplicate_account(buf);
	if (ret < 0) {
		ugd->popup = setting_get_notify(&vd->base,
				&(EMAIL_SETTING_STRING_WARNING),
				&(EMAIL_SETTING_STRING_ACCOUNT_ALREADY_EXISTS), 1,
				&(EMAIL_SETTING_STRING_OK),
				_popup_ok_cb, NULL, NULL);

		g_free(buf);
		return;
	}
	g_free(buf);

	/* account info setting */
	setting_set_csc_account(&vd->base);

	_validate_account(vd);
}

static void _login_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailSettingVD *vd = data;

	retm_if(!vd, "vd is NULL");

	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;

	_read_all_entries(vd);

	ugd->email_sp = NULL;
	_next_cb(vd, NULL, NULL);
}

static void _set_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailSettingVD *vd = data;
	retm_if(!vd, "vd is NULL");

	if (setting_get_network_failure_notify(&vd->base)) {
		debug_log("display network failure notify");
		return;
	}

	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;

	debug_log("add account mode: %d", ugd->add_account_type);

	FREE(ugd->imap_conf);
	FREE(ugd->pop_conf);
	FREE(ugd->smtp_conf);

	if (ugd->popup) {
		debug_log("invalid case..");
		return;
	}

	if (ugd->add_account_type == EMAIL_ADD_ACCOUNT_TYPE_OTHERS) {
		_next_cb(vd, NULL, NULL);
	} else if (ugd->add_account_type == EMAIL_ADD_ACCOUNT_TYPE_CSC) {
		_save_cb(vd, NULL, NULL);
	} else {
		_login_cb(vd, NULL, NULL);
	}
}

static void _manual_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailSettingVD *vd = data;
	retm_if(!vd, "vd is null");

	if (setting_get_network_failure_notify(&vd->base)) {
		debug_log("display network failure notify");
		return;
	}

	EmailSettingUGD *ugd = NULL;
	char *buf = NULL;
	int ret = 0;

	ugd = (EmailSettingUGD *)vd->base.module;

	if (ugd->popup) {
		debug_log("invalid case..");
		return;
	}

	/* Save the Account Data */
	_read_all_entries(vd);
	email_account_t *account = ugd->new_account;

	debug_secure("accountStructure Info");
	debug_secure("Account Name:%s", account->account_name);
	debug_secure("Display Name:%s", account->user_display_name);
	debug_secure("Email Addr:%s", account->user_email_address);
	debug_secure("User Name:%s", account->incoming_server_user_name);

	/* check Null field */
	ret = _check_null_field(vd);
	if (!ret) {
		ugd->popup = setting_get_notify(&vd->base,
				&(EMAIL_SETTING_STRING_WARNING),
				&(EMAIL_SETTING_STRING_FILL_MANDATORY_FIELDS), 1,
				&(EMAIL_SETTING_STRING_OK),
				_popup_ok_cb, NULL, NULL);
		return;
	}

	/* check character validation */
	buf = g_strdup(account->user_email_address);

	ret = email_get_address_validation(buf);
	if (!ret) {
		ugd->popup = setting_get_notify(&vd->base,
				&(EMAIL_SETTING_STRING_WARNING),
				&(EMAIL_SETTING_STRING_INVALID_EMAIL_ADDRESS), 1,
				&(EMAIL_SETTING_STRING_OK),
				_popup_ok_cb, NULL, NULL);

		g_free(buf);
		return;
	}

	g_free(buf);

	/* account info setting */
	int is_need_query = 0;
	FREE(ugd->imap_conf);
	FREE(ugd->pop_conf);
	FREE(ugd->smtp_conf);
	if (ugd->add_account_type == EMAIL_ADD_ACCOUNT_TYPE_CSC) {
		setting_set_csc_account(&vd->base);
	} else if (ugd->add_account_type == EMAIL_ADD_ACCOUNT_TYPE_EMAIL) {

		ugd->email_sp = NULL;
		int is_default_provider = setting_is_in_default_provider_list(&vd->base, account->user_email_address);
		if (is_default_provider)
			setting_set_default_provider_info_to_account(&vd->base, account);
		is_need_query = !is_default_provider;

	} else if (ugd->add_account_type == EMAIL_ADD_ACCOUNT_TYPE_OTHERS) {
		ugd->email_sp = NULL;
		int is_default_provider = setting_is_in_default_provider_list(&vd->base, account->user_email_address);
		if (is_default_provider)
			setting_set_default_provider_info_to_account(&vd->base, account);
		else
			setting_set_others_account(&vd->base);
		is_need_query = !is_default_provider;
	}

	if (is_need_query) {
		DELETE_EVAS_OBJECT(ugd->popup);

		debug_log("show popup to select IMAP/POP3");
		ugd->popup = setting_get_empty_content_notify(&vd->base, &(EMAIL_SETTING_STRING_SELECT_TYPE_OF_ACCOUNT),
				0, NULL, NULL, NULL, NULL);

		Evas_Object *genlist = NULL;
		Elm_Genlist_Item_Class *itc = setting_get_genlist_class_item("type1", _popup_list_text_get_cb, NULL, NULL, NULL);

		genlist = elm_genlist_add(ugd->popup);
		elm_object_style_set(genlist, "popup");

		elm_genlist_homogeneous_set(genlist, EINA_TRUE);
		elm_scroller_policy_set(genlist, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
		elm_scroller_content_min_limit(genlist, EINA_FALSE, EINA_TRUE);

		int index = 0;
		for (index = 0; index < 2; index++) {
			elm_genlist_item_append(genlist, itc, (void *)(ptrdiff_t)index, NULL, ELM_GENLIST_ITEM_NONE, _popup_list_select_cb, (void *)(ptrdiff_t)index);
		}

		evas_object_show(genlist);
		elm_object_content_set(ugd->popup, genlist);

		evas_object_show(ugd->popup);

	} else {
		DELETE_EVAS_OBJECT(ugd->popup);

		debug_log("add account mode: %d", ugd->add_account_type);
		create_manual_setup_view(ugd);
	}
}

static void _on_back_key(email_view_t *self)
{
	debug_enter();
	retm_if(!self, "self is null");

	email_module_exit_view(self);
}

static void _check_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	ListItemData *li = data;

	retm_if(!li, "li is null");

	EmailSettingVD *vd = li->vd;
	if (li->index == SHOW_PASSWORD_LIST_ITEM) {
		elm_entry_password_set(vd->entry_password.entry, !vd->is_show_passwd_check);
		elm_entry_cursor_line_end_set(vd->entry_password.entry);
	}
}

static Eina_Bool _after_validation_cb(void *data)
{
	debug_enter();

	EmailSettingVD *vd = data;

	retvm_if(!vd, ECORE_CALLBACK_CANCEL, "vd is null");

	EmailSettingUGD *ugd = NULL;

	ugd = (EmailSettingUGD *)vd->base.module;

	DELETE_TIMER_OBJECT(vd->preset_vc_timer);
	DELETE_EVAS_OBJECT(ugd->popup);

	create_account_details_setup_view(ugd);

	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _startup_focus_cb(void *data)
{
	debug_enter();

	EmailSettingVD *vd = data;

	retvm_if(!vd, ECORE_CALLBACK_CANCEL, "vd is null");

	DELETE_TIMER_OBJECT(vd->focus_timer);

	elm_object_focus_set(vd->entry_email_address.entry, EINA_TRUE);
	elm_entry_cursor_begin_set(vd->entry_email_address.entry);

	return ECORE_CALLBACK_CANCEL;
}

static void _popup_ok_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "data is null");

	EmailSettingVD *vd = data;
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;

	DELETE_EVAS_OBJECT(ugd->popup);
}

static void _backup_input_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	ListItemData *li = data;
	EmailSettingVD *vd = li->vd;

	if (li->index == EMAIL_ADDRESS_LIST_ITEM) {
		FREE(vd->str_email_address);
		char *val = setting_get_entry_str(obj);
		vd->str_email_address = strdup(g_strstrip(val));
		free(val);
	} else if (li->index == PASSWORD_LIST_ITEM) {
		if (vd->str_password) {
			FREE(vd->str_password_before);
			vd->str_password_before = strdup(vd->str_password);
			FREE(vd->str_password);
		}
		vd->str_password = setting_get_entry_str(obj);
	}

	if (vd->str_email_address == NULL || vd->str_password == NULL || !email_get_address_validation(vd->str_email_address)) {
		elm_object_disabled_set(vd->manual_c_btn, EINA_TRUE);
		elm_object_disabled_set(vd->login_c_btn, EINA_TRUE);
		elm_entry_input_panel_return_key_disabled_set(vd->entry_password.entry, EINA_TRUE);
		return;
	}

	if (g_strcmp0(vd->str_email_address, "") == 0 || g_strcmp0(vd->str_password, "") == 0) {
		elm_object_disabled_set(vd->manual_c_btn, EINA_TRUE);
		elm_object_disabled_set(vd->login_c_btn, EINA_TRUE);
		elm_entry_input_panel_return_key_disabled_set(vd->entry_password.entry, EINA_TRUE);
	} else {
		elm_object_disabled_set(vd->manual_c_btn, EINA_FALSE);
		elm_object_disabled_set(vd->login_c_btn, EINA_FALSE);
		elm_entry_input_panel_return_key_disabled_set(vd->entry_password.entry, EINA_FALSE);
	}
}

static void _return_key_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "data is null");
	retm_if(!obj, "obj is null");

	ListItemData *li = data;
	EmailSettingVD *vd = li->vd;

	if (li->index == EMAIL_ADDRESS_LIST_ITEM) {
		elm_object_focus_set(vd->entry_password.entry, EINA_TRUE);
	} else if (li->index == PASSWORD_LIST_ITEM) {
		_set_cb(vd, NULL, NULL);
	}
}

static void _account_validate_cb(int account_id, email_setting_response_data *response, void *user_data)
{
	debug_enter();

	EmailSettingVD *vd = user_data;
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;

	retm_if(!response, "response data is NULL");
	retm_if(vd->handle != response->handle, "handle is different");

	debug_log("response err: %d", response->err);
	debug_log("response type: %d", response->type);
	debug_log("local handle: %d, reponse handle: %d", vd->handle, response->handle);

	/* initialize handle */
	vd->handle = EMAIL_OP_HANDLE_INITIALIZER;

	if (response->err == EMAIL_ERROR_NONE ||
			response->err == EMAIL_ERROR_VALIDATE_ACCOUNT_OF_SMTP) {
		vd->is_retry_validate_with_username = 0;
		DELETE_TIMER_OBJECT(vd->preset_vc_timer);
		if (response->err == EMAIL_ERROR_VALIDATE_ACCOUNT_OF_SMTP)
			debug_warning("smtp validation failed but it can be ignored");
		_update_account_capability(vd, (const char *)(response->data));
		_update_account_smtp_mail_limit_size(vd, (const char *)(response->data));
		vd->preset_vc_timer = ecore_timer_add(0.5, _after_validation_cb, vd);
	} else if (!(vd->is_retry_validate_with_username) &&
			(response->err != EMAIL_ERROR_CANCELLED)) {
		vd->is_retry_validate_with_username = 1;
		_set_username_before_at(vd);
		_validate_account(vd);
	} else {
		vd->is_retry_validate_with_username = 0;
		if (response->err != EMAIL_ERROR_CANCELLED) {
			const email_setting_string_t *type = setting_get_service_fail_type(response->err);
			const email_setting_string_t *header = setting_get_service_fail_type_header(response->err);
			ugd->popup = setting_get_notify(&vd->base,
					header,
					type, 1, &(EMAIL_SETTING_STRING_OK),
					_popup_ok_cb, NULL, NULL);
		}
	}
}

static void _set_username_before_at(EmailSettingVD *vd)
{
	debug_enter();

	retm_if(!vd, "vd is NULL");

	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;
	email_account_t *account = ugd->new_account;
	char *buf = NULL;

	if (account->incoming_server_user_name)
		free(account->incoming_server_user_name);
	if (account->outgoing_server_user_name)
		free(account->outgoing_server_user_name);

	buf = g_strdup(account->user_email_address);
	account->incoming_server_user_name = g_strdup(strtok(buf, "@"));
	account->outgoing_server_user_name = strdup(account->incoming_server_user_name);

	debug_secure("retry to validate with user name: %s", account->incoming_server_user_name);

	g_free(buf);
}

static char *_popup_list_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	int index = (int)(ptrdiff_t)data;
	if (!g_strcmp0("elm.text", part)) {
		if (index == 0) {
			return strdup(email_setting_gettext(EMAIL_SETTING_STRING_IMAP));
		} else {
			return strdup(email_setting_gettext(EMAIL_SETTING_STRING_POP3));
		}
	}
	return NULL;
}

static void _popup_list_select_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	int index = (int)(ptrdiff_t)data;
	EmailSettingVD *vd = NULL;
	EmailSettingUGD *ugd = NULL;

	vd = g_vd;
	ugd = (EmailSettingUGD *)vd->base.module;

	DELETE_EVAS_OBJECT(ugd->popup);

	setting_set_others_account(&vd->base);
	setting_set_others_account_server_default_type(ugd->new_account, index, 1, 1);

	create_manual_setup_view(ugd);
}

static void _update_account_capability(EmailSettingVD *vd, const char *capability)
{
	debug_enter();
	retm_if(!vd, "vd is NULL");
	retm_if(!capability, "capability is NULL");

	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;
	email_account_t *account = ugd->new_account;

	debug_secure("capability: %s", capability);
	if (g_strrstr(capability, "IDLE")) {
		account->retrieval_mode = EMAIL_IMAP4_RETRIEVAL_MODE_ALL | EMAIL_IMAP4_IDLE_SUPPORTED;
	}
}

static void _update_account_smtp_mail_limit_size(EmailSettingVD *vd, const char *mail_limit_size)
{
	debug_enter();
	retm_if(!vd, "vd is NULL");
	retm_if(!mail_limit_size, "capability is NULL");

	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;
	email_account_t *account = ugd->new_account;

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

static void _gl_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	ListItemData *li = data;
	Elm_Object_Item *item = event_info;
	EmailSettingVD *vd = li->vd;

	elm_genlist_item_selected_set(item, EINA_FALSE);

	if (li->index == DEFAULT_ACCOUNT_LIST_ITEM) {
		elm_check_state_set(vd->default_account_check, !elm_check_state_get(vd->default_account_check));
		_check_clicked_cb(li, vd->default_account_check, NULL);
	} else if (li->index == SHOW_PASSWORD_LIST_ITEM) {
		elm_check_state_set(vd->show_passwd_check, !elm_check_state_get(vd->show_passwd_check));
		_check_clicked_cb(li, vd->show_passwd_check, NULL);
	}
}
/* EOF */
