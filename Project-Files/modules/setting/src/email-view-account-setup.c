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

typedef struct view_data EmailSettingView;

static int _create(email_view_t *self);
static void _destroy(email_view_t *self);
static void _activate(email_view_t *self, email_view_state prev_state);
static void _update(email_view_t *self, int flags);
static void _on_back_key(email_view_t *self);

static void _push_naviframe(EmailSettingView *view);
static void _create_view(EmailSettingView *view);
static void _initialize_handle(EmailSettingView *view);
static void _create_list(EmailSettingView *view);
static void _validate_account(EmailSettingView *view);
static void _account_validate_cb(int account_id, email_setting_response_data *response, void *user_data);
static int _check_null_field(EmailSettingView *view);
static void _read_all_entries(EmailSettingView *view);
static void _set_username_before_at(EmailSettingView *view);
static void _popup_list_select_cb(void *data, Evas_Object *obj, void *event_info);
static char *_popup_list_text_get_cb(void *data, Evas_Object *obj, const char *part);
static void _update_account_capability(EmailSettingView *view, const char *capability);
static void _update_account_smtp_mail_limit_size(EmailSettingView *view, const char *mail_limit_size);

static char *_gl_text_get_cb(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_gl_content_get_cb(void *data, Evas_Object *obj, const char *part);

static void _perform_account_validation(EmailSettingView *view);
static void _switch_to_manual_setup(EmailSettingView *view);
static void _next_btn_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static void _manual_btn_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static void _check_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static Eina_Bool _after_validation_cb(void *data);
static Eina_Bool _startup_focus_cb(void *data);
static void _popup_ok_cb(void *data, Evas_Object *obj, void *event_info);
static void _backup_input_cb(void *data, Evas_Object *obj, void *event_info);
static void _return_key_cb(void *data, Evas_Object *obj, void *event_info);
static void _gl_sel_cb(void *data, Evas_Object *obj, void *event_info);

static void _create_account_type_chooser_popup(EmailSettingView *view);

static email_string_t EMAIL_SETTING_STRING_EMAIL_ADDRESS = {PACKAGE, "IDS_EMAIL_BODY_EMAIL_ADDRESS"};
static email_string_t EMAIL_SETTING_STRING_NEXT = {PACKAGE, "IDS_ST_BUTTON_NEXT"};
static email_string_t EMAIL_SETTING_STRING_OK = {PACKAGE, "IDS_EMAIL_BUTTON_OK"};
static email_string_t EMAIL_SETTING_STRING_EMAIL = {PACKAGE, "IDS_ST_HEADER_EMAIL"};
static email_string_t EMAIL_SETTING_STRING_IMAP = {PACKAGE, "IDS_ST_SBODY_IMAP"};
static email_string_t EMAIL_SETTING_STRING_MANUAL_SETUP = {PACKAGE, "IDS_ST_BUTTON_MANUAL_SETUP_ABB"};
static email_string_t EMAIL_SETTING_STRING_POP3 = {PACKAGE, "IDS_ST_SBODY_POP3"};
static email_string_t EMAIL_SETTING_STRING_SELECT_TYPE_OF_ACCOUNT = {PACKAGE, "IDS_ST_HEADER_SELECT_ACCOUNT_TYPE_ABB"};
static email_string_t EMAIL_SETTING_STRING_SEND_EMAIL_FROM_THIS_ACCOUNT_BY_DEFAULT = {PACKAGE, "IDS_ST_SBODY_SET_AS_DEFAULT_ACCOUNT_ABB"};
static email_string_t EMAIL_SETTING_STRING_SHOW_PASSWORD = {PACKAGE, "IDS_ST_SBODY_SHOW_PASSWORD_ABB"};
static email_string_t EMAIL_SETTING_STRING_ENTER_PASS = {PACKAGE, "IDS_ST_TMBODY_PASSWORD"};
static email_string_t EMAIL_SETTING_STRING_WARNING = {PACKAGE, "IDS_ST_HEADER_WARNING"};
static email_string_t EMAIL_SETTING_STRING_SK_ADD_ACCOUNT = {PACKAGE, "IDS_EMAIL_OPT_ADD_ACCOUNT"};
static email_string_t EMAIL_SETTING_STRING_INVALID_EMAIL_ADDRESS = {PACKAGE, "IDS_EMAIL_TPOP_INVALID_EMAIL_ADDRESS_ENTERED"};
static email_string_t EMAIL_SETTING_STRING_UNABLE_TO_ADD_ACCOUNT = {PACKAGE, "IDS_ST_HEADER_UNABLE_TO_ADD_ACCOUNT_ABB"};
static email_string_t EMAIL_SETTING_STRING_FILL_MANDATORY_FIELDS = {PACKAGE, "IDS_EMAIL_POP_PLEASE_FILL_ALL_THE_MANDATORY_FIELDS"};
static email_string_t EMAIL_SETTING_STRING_SERVER_QUERY_FAIL = {PACKAGE, "IDS_EMAIL_POP_SERVER_INFORMATION_QUERY_FAILED_ENTER_SERVER_INFORMATION_MANUALLY"};
static email_string_t EMAIL_SETTING_STRING_ACCOUNT_ALREADY_EXISTS = {PACKAGE, "IDS_ST_POP_THIS_ACCOUNT_HAS_ALREADY_BEEN_ADDED"};

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

	Evas_Object *next_btn;
	Evas_Object *manual_btn;

	Ecore_Timer *preset_vc_timer;
	Ecore_Timer *focus_timer;

	Elm_Genlist_Item_Class *itc1;
	Elm_Genlist_Item_Class *itc2;
};

typedef struct _ListItemData {
	int index;
	Elm_Entry_Filter_Limit_Size *entry_limit;
	EmailSettingView *view;
} ListItemData;

void create_account_setup_view(EmailSettingModule *module)
{
	debug_enter();

	retm_if(!module, "module is null");

	EmailSettingView *view = calloc(1, sizeof(EmailSettingView));
	retm_if(!view, "view data is null");

	view->base.create = _create;
	view->base.destroy = _destroy;
	view->base.activate = _activate;
	view->base.update = _update;
	view->base.on_back_key = _on_back_key;

	module->is_set_default_account = EINA_FALSE;

	debug_log("view create result: %d", email_module_create_view(&module->base, &view->base));
}

static int _create(email_view_t *self)
{
	debug_enter();
	retvm_if(!self, -1, "self is null");

	EmailSettingView *view = (EmailSettingView *)self;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	view->base.content = setting_add_inner_layout(&view->base);
	_push_naviframe(view);

	if (module->new_account) {
		setting_new_acct_final(&view->base);
	}
	setting_new_acct_init(&view->base);

	module->account_id = 0;

	DELETE_TIMER_OBJECT(view->focus_timer);
	view->focus_timer = ecore_timer_add(0.5, _startup_focus_cb, view);

	_create_view(view);

	setting_noti_subscribe(module, EMAIL_SETTING_ACCOUNT_VALIDATE_NOTI, _account_validate_cb, view);

	_initialize_handle(view);

	return 0;
}

static void _update(email_view_t *self, int flags)
{
	debug_enter();
	retm_if(!self, "self is null");

	EmailSettingView *view = (EmailSettingView *)self;

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
	DELETE_TIMER_OBJECT(view->focus_timer);
	DELETE_TIMER_OBJECT(view->preset_vc_timer);
	FREE(view->str_password);
	FREE(view->str_password_before);

	GSList *l = view->list_items;
	while (l) {
		ListItemData *li = l->data;
		free(li->entry_limit);
		free(li);
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

	setting_new_acct_final(&view->base);

	free(view);
}

static void _activate(email_view_t *self, email_view_state prev_state)
{
	debug_enter();
	retm_if(!self, "self is null");

	EmailSettingView *view = (EmailSettingView *)self;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	if (prev_state != EV_STATE_CREATED) {
		return;
	}

	setting_load_provider_list(&(module->default_provider_list), NULL, EMAIL_SETTING_DEFAULT_PROVIDER_XML_FILENAME);
}

static void _push_naviframe(EmailSettingView *view)
{
	debug_enter();
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	int push_flags = ((module->base.views_count == 1) ? EVPF_NO_TRANSITION : 0);

	Elm_Object_Item *navi_it = email_module_view_push(&view->base, EMAIL_SETTING_STRING_EMAIL.id, push_flags);
	elm_object_item_domain_text_translatable_set(navi_it, EMAIL_SETTING_STRING_EMAIL.domain, EINA_TRUE);

	Evas_Object *btn = NULL;
	Evas_Object *btn_ly = NULL;
	btn_ly = elm_layout_add(view->base.content);
	elm_layout_file_set(btn_ly, email_get_setting_theme_path(), "two_bottom_btn");

	btn = elm_button_add(btn_ly);
	elm_object_style_set(btn, "bottom");
	elm_object_domain_translatable_text_set(btn, EMAIL_SETTING_STRING_MANUAL_SETUP.domain, EMAIL_SETTING_STRING_MANUAL_SETUP.id);
	evas_object_smart_callback_add(btn, "clicked", _manual_btn_clicked_cb, view);
	view->manual_btn = btn;
	elm_object_disabled_set(btn, EINA_TRUE);
	elm_layout_content_set(btn_ly, "btn1.swallow", btn);

	btn = elm_button_add(btn_ly);
	elm_object_style_set(btn, "bottom");
	elm_object_domain_translatable_text_set(btn, EMAIL_SETTING_STRING_NEXT.domain, EMAIL_SETTING_STRING_NEXT.id);
	evas_object_smart_callback_add(btn, "clicked", _next_btn_clicked_cb, view);
	view->next_btn = btn;
	elm_object_disabled_set(btn, EINA_TRUE);
	elm_layout_content_set(btn_ly, "btn2.swallow", btn);

	elm_object_item_part_content_set(navi_it, "toolbar", btn_ly);

	evas_object_show(view->base.content);
}

static void _create_view(EmailSettingView *view)
{
	debug_enter();

	retm_if(!view, "view data is null");

	_create_list(view);

	elm_object_part_content_set(view->base.content, "elm.swallow.content", view->genlist);
}

static void _create_list(EmailSettingView *view)
{
	debug_enter();

	retm_if(!view, "view data is null");

	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	view->genlist = elm_genlist_add(view->base.content);
	elm_genlist_mode_set(view->genlist, ELM_LIST_COMPRESS);
	elm_genlist_homogeneous_set(view->genlist, EINA_TRUE);
	elm_scroller_policy_set(view->genlist, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);

	view->itc1 = setting_get_genlist_class_item("full", NULL, _gl_content_get_cb, NULL, NULL);
	view->itc2 = setting_get_genlist_class_item("type1", _gl_text_get_cb, _gl_content_get_cb, NULL, NULL);

	view->itc_list = g_slist_append(view->itc_list, view->itc1);
	view->itc_list = g_slist_append(view->itc_list, view->itc2);

	ListItemData *li = NULL;
	Elm_Object_Item *item = NULL;

	/*email address*/
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = EMAIL_ADDRESS_LIST_ITEM;
	li->view = view;
	item = elm_genlist_item_append(view->genlist, view->itc1, li, NULL,
			ELM_GENLIST_ITEM_NONE, NULL, NULL);
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_NONE);
	view->list_items = g_slist_append(view->list_items, li);

	/*password*/
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = PASSWORD_LIST_ITEM;
	li->view = view;
	item = elm_genlist_item_append(view->genlist, view->itc1, li, NULL,
			ELM_GENLIST_ITEM_NONE, NULL, NULL);
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_NONE);
	view->list_items = g_slist_append(view->list_items, li);

	/*show password*/
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = SHOW_PASSWORD_LIST_ITEM;
	li->view = view;
	item = elm_genlist_item_append(view->genlist, view->itc2, li, NULL,
			ELM_GENLIST_ITEM_NONE, _gl_sel_cb, li);
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_ALWAYS);
	view->list_items = g_slist_append(view->list_items, li);

	if (!(module->account_list == NULL || module->account_count == 0)) {
		/*set as default account*/
		li = calloc(1, sizeof(ListItemData));
		retm_if(!li, "memory allocation failed");

		li->index = DEFAULT_ACCOUNT_LIST_ITEM;
		li->view = view;
		item = elm_genlist_item_append(view->genlist, view->itc2, li, NULL, ELM_GENLIST_ITEM_NONE, _gl_sel_cb, li);
		elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_ALWAYS);
		view->list_items = g_slist_append(view->list_items, li);
	}
}

static char *_gl_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	ListItemData *li = data;
	if (li) {
		if (li->index == DEFAULT_ACCOUNT_LIST_ITEM && !strcmp("elm.text", part)) {
			return g_strdup(email_setting_gettext(EMAIL_SETTING_STRING_SEND_EMAIL_FROM_THIS_ACCOUNT_BY_DEFAULT));
		} else if (li->index == SHOW_PASSWORD_LIST_ITEM && !strcmp("elm.text", part)) {
			return g_strdup(email_setting_gettext(EMAIL_SETTING_STRING_SHOW_PASSWORD));
		}
	}
	return NULL;
}

static Evas_Object *_gl_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
	ListItemData *li = data;
	if (li) {
		EmailSettingView *view = li->view;
		EmailSettingModule *module = (EmailSettingModule *)view->base.module;
		Evas_Object *check = NULL;
		if ((li->index == EMAIL_ADDRESS_LIST_ITEM || li->index == PASSWORD_LIST_ITEM) && !strcmp("elm.swallow.content", part)) {
			Elm_Entry_Filter_Limit_Size *entry_limit = NULL;
			if (li->index == EMAIL_ADDRESS_LIST_ITEM) {
				email_common_util_editfield_create(obj, 0, &view->entry_email_address);
				elm_entry_input_panel_layout_set(view->entry_email_address.entry, ELM_INPUT_PANEL_LAYOUT_EMAIL);
				elm_entry_input_panel_return_key_type_set(view->entry_email_address.entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_NEXT);
				elm_object_domain_translatable_part_text_set(view->entry_email_address.entry, "elm.guide",
						EMAIL_SETTING_STRING_EMAIL_ADDRESS.domain, EMAIL_SETTING_STRING_EMAIL_ADDRESS.id);
				entry_limit = setting_set_input_entry_limit(view->entry_email_address.entry, 0, EMAIL_LIMIT_EMAIL_ADDRESS_LENGTH);
				setting_set_entry_str(view->entry_email_address.entry, view->str_email_address);
				evas_object_smart_callback_add(view->entry_email_address.entry, "changed", _backup_input_cb, li);
				evas_object_smart_callback_add(view->entry_email_address.entry, "preedit,changed", _backup_input_cb, li);
				evas_object_smart_callback_add(view->entry_email_address.entry, "activated", _return_key_cb, li);
				li->entry_limit = entry_limit;
				return view->entry_email_address.layout;

			} else if (li->index == PASSWORD_LIST_ITEM && !strcmp("elm.swallow.content", part)) {
				email_common_util_editfield_create(obj, EF_PASSWORD, &view->entry_password);
				elm_entry_input_panel_return_key_type_set(view->entry_password.entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
				elm_entry_input_panel_return_key_disabled_set(view->entry_password.entry, EINA_TRUE);
				elm_object_domain_translatable_part_text_set(view->entry_password.entry, "elm.guide",
						EMAIL_SETTING_STRING_ENTER_PASS.domain, EMAIL_SETTING_STRING_ENTER_PASS.id);
				entry_limit = setting_set_input_entry_limit(view->entry_password.entry, 0, EMAIL_LIMIT_PASSWORD_LENGTH);
				elm_entry_password_set(view->entry_password.entry, !view->is_show_passwd_check);
				setting_set_entry_str(view->entry_password.entry, view->str_password);
				evas_object_smart_callback_add(view->entry_password.entry, "changed", _backup_input_cb, li);
				evas_object_smart_callback_add(view->entry_password.entry, "preedit,changed", _backup_input_cb, li);
				evas_object_smart_callback_add(view->entry_password.entry, "activated", _return_key_cb, li);
				li->entry_limit = entry_limit;
				return view->entry_password.layout;
			}

		} else if (li->index == DEFAULT_ACCOUNT_LIST_ITEM && !strcmp("elm.swallow.end", part)) {
			check = elm_check_add(obj);
			elm_check_state_pointer_set(check, &(module->is_set_default_account));
			elm_object_focus_allow_set(check, EINA_FALSE);
			evas_object_propagate_events_set(check, EINA_FALSE);
			view->default_account_check = check;
			evas_object_smart_callback_add(check, "changed", _check_clicked_cb, li);
			return check;
		} else if (li->index == SHOW_PASSWORD_LIST_ITEM && !strcmp("elm.swallow.end", part)) {
			check = elm_check_add(obj);
			elm_check_state_pointer_set(check, &(view->is_show_passwd_check));
			elm_object_focus_allow_set(check, EINA_FALSE);
			evas_object_propagate_events_set(check, EINA_FALSE);
			view->show_passwd_check = check;
			evas_object_smart_callback_add(check, "changed", _check_clicked_cb, li);
			return check;
		}
	}
	return NULL;
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

static void _create_account_type_chooser_popup(EmailSettingView *view)
{
	debug_enter();

	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	retm_if(!module, "module is NULL");

	if (module->popup) {
		DELETE_EVAS_OBJECT(module->popup);
	}

	module->popup = setting_get_empty_content_notify(&view->base,
			&(EMAIL_SETTING_STRING_SELECT_TYPE_OF_ACCOUNT),
			0, NULL, NULL, NULL, NULL);

	Elm_Genlist_Item_Class *itc = setting_get_genlist_class_item("type1", _popup_list_text_get_cb, NULL, NULL, NULL);

	Evas_Object *genlist = elm_genlist_add(module->popup);
	elm_genlist_homogeneous_set(genlist, EINA_TRUE);
	elm_scroller_policy_set(genlist, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
	elm_scroller_content_min_limit(genlist, EINA_FALSE, EINA_TRUE);

	int index = 0;
	for (; index < 2; index++) {
		elm_genlist_item_append(genlist, itc,
				(void *)(ptrdiff_t)index,
				NULL,
				ELM_GENLIST_ITEM_NONE,
				_popup_list_select_cb,
				view);
	}

	elm_genlist_item_class_free(itc);
	evas_object_show(genlist);

	elm_object_content_set(module->popup, genlist);

	evas_object_show(module->popup);
}

static int _check_null_field(EmailSettingView *view)
{
	debug_enter();

	EmailSettingModule *module = (EmailSettingModule *)view->base.module;
	email_account_t *account = module->new_account;

	retvm_if(!account, FALSE, "account is null");

	if (!STR_VALID(account->user_email_address) ||
			!STR_VALID(account->incoming_server_password)) {
		return FALSE;
	} else {
		return TRUE;
	}
}

static void _read_all_entries(EmailSettingView *view)
{
	debug_enter();

	EmailSettingModule *module = (EmailSettingModule *)view->base.module;
	email_account_t *account = module->new_account;

	FREE(account->user_email_address);
	FREE(account->incoming_server_password);

	account->user_email_address = g_strdup(view->str_email_address);
	account->account_name = g_strdup(view->str_email_address);
	account->incoming_server_password = g_strdup(view->str_password);
}

static bool _check_possibility_to_perform_action(EmailSettingView *view)
{
	debug_enter();
	retvm_if(setting_get_network_failure_notify(&view->base), false, "display network failure notify");

	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	retvm_if(module->popup, false, "invalid case..");

	/* Save the Account Data */
	_read_all_entries(view);

	email_account_t *account = module->new_account;

	debug_secure("accountStructure Info");
	debug_secure("Account Name: %s", account->account_name);
	debug_secure("Display Name: %s", account->user_display_name);
	debug_secure("Email Addr: %s", account->user_email_address);
	debug_secure("User Name: %s", account->incoming_server_user_name);

	/* check Null field */
	if (!_check_null_field(view)) {
		module->popup = setting_get_notify(&view->base,
				&(EMAIL_SETTING_STRING_WARNING),
				&(EMAIL_SETTING_STRING_FILL_MANDATORY_FIELDS), 1,
				&(EMAIL_SETTING_STRING_OK), _popup_ok_cb, NULL, NULL);
		return false;
	}

	/* check character validation */
	if (!email_get_address_validation(account->user_email_address)) {
		module->popup = setting_get_notify(&view->base,
				&(EMAIL_SETTING_STRING_WARNING),
				&(EMAIL_SETTING_STRING_INVALID_EMAIL_ADDRESS), 1,
				&(EMAIL_SETTING_STRING_OK), _popup_ok_cb, NULL, NULL);
		return false;
	}
	return true;
}

static void _perform_account_validation(EmailSettingView *view)
{
	debug_enter();
	retm_if(!_check_possibility_to_perform_action(view), "_check_possibility_to_perform_action failed");

	EmailSettingModule *module = (EmailSettingModule *)view->base.module;
	email_account_t *account = module->new_account;

	/* check duplication account */
	if (setting_is_duplicate_account(account->user_email_address) < 0) {
		module->popup = setting_get_notify(&view->base,
				&(EMAIL_SETTING_STRING_WARNING),
				&(EMAIL_SETTING_STRING_ACCOUNT_ALREADY_EXISTS), 1,
				&(EMAIL_SETTING_STRING_OK), _popup_ok_cb, NULL, NULL);
		return;
	}

	if (setting_is_in_default_provider_list(&view->base, account->user_email_address)) {
		setting_set_default_provider_info_to_account(&view->base, account);
		_validate_account(view);
	} else {
		module->popup = setting_get_notify(&view->base,
				&(EMAIL_SETTING_STRING_SK_ADD_ACCOUNT),
				&(EMAIL_SETTING_STRING_SERVER_QUERY_FAIL), 1,
				&(EMAIL_SETTING_STRING_OK), _popup_ok_cb, NULL, NULL);
		return;
	}
}

static void _switch_to_manual_setup(EmailSettingView *view)
{
	debug_enter();
	retm_if(!_check_possibility_to_perform_action(view), "_check_possibility_to_perform_action failed");

	EmailSettingModule *module = (EmailSettingModule *)view->base.module;
	email_account_t *account = module->new_account;

	if (setting_is_in_default_provider_list(&view->base, account->user_email_address)) {
		setting_set_default_provider_info_to_account(&view->base, account);
		create_manual_setup_view(module);
	} else {
		debug_log("show popup to select IMAP/POP3");
		_create_account_type_chooser_popup(view);
	}
}

static void _next_btn_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	_perform_account_validation((EmailSettingView *)data);
}

static void _manual_btn_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	_switch_to_manual_setup((EmailSettingView *)data);
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

	EmailSettingView *view = li->view;
	if (li->index == SHOW_PASSWORD_LIST_ITEM) {
		elm_entry_password_set(view->entry_password.entry, !view->is_show_passwd_check);
		elm_entry_cursor_line_end_set(view->entry_password.entry);
	}
}

static Eina_Bool _after_validation_cb(void *data)
{
	debug_enter();
	retvm_if(!data, ECORE_CALLBACK_CANCEL, "data is null");

	EmailSettingView *view = data;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	DELETE_TIMER_OBJECT(view->preset_vc_timer);
	DELETE_EVAS_OBJECT(module->popup);

	create_account_details_setup_view(module);

	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _startup_focus_cb(void *data)
{
	debug_enter();
	retvm_if(!data, ECORE_CALLBACK_CANCEL, "data is null");

	EmailSettingView *view = data;

	DELETE_TIMER_OBJECT(view->focus_timer);

	elm_object_focus_set(view->entry_email_address.entry, EINA_TRUE);
	elm_entry_cursor_begin_set(view->entry_email_address.entry);

	return ECORE_CALLBACK_CANCEL;
}

static void _popup_ok_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is null");

	EmailSettingView *view = data;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	DELETE_EVAS_OBJECT(module->popup);
}

static void _backup_input_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	ListItemData *li = data;
	EmailSettingView *view = li->view;

	if (li->index == EMAIL_ADDRESS_LIST_ITEM) {
		FREE(view->str_email_address);
		char *val = setting_get_entry_str(obj);
		view->str_email_address = strdup(g_strstrip(val));
		free(val);
	} else if (li->index == PASSWORD_LIST_ITEM) {
		if (view->str_password) {
			FREE(view->str_password_before);
			view->str_password_before = strdup(view->str_password);
			FREE(view->str_password);
		}
		view->str_password = setting_get_entry_str(obj);
	}

	if (view->str_email_address == NULL || view->str_password == NULL || !email_get_address_validation(view->str_email_address)) {
		elm_object_disabled_set(view->manual_btn, EINA_TRUE);
		elm_object_disabled_set(view->next_btn, EINA_TRUE);
		elm_entry_input_panel_return_key_disabled_set(view->entry_password.entry, EINA_TRUE);
		return;
	}

	if (g_strcmp0(view->str_email_address, "") == 0 || g_strcmp0(view->str_password, "") == 0) {
		elm_object_disabled_set(view->manual_btn, EINA_TRUE);
		elm_object_disabled_set(view->next_btn, EINA_TRUE);
		elm_entry_input_panel_return_key_disabled_set(view->entry_password.entry, EINA_TRUE);
	} else {
		elm_object_disabled_set(view->manual_btn, EINA_FALSE);
		elm_object_disabled_set(view->next_btn, EINA_FALSE);
		elm_entry_input_panel_return_key_disabled_set(view->entry_password.entry, EINA_FALSE);
	}
}

static void _return_key_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "data is null");
	retm_if(!obj, "obj is null");

	ListItemData *li = data;
	EmailSettingView *view = li->view;

	if (li->index == EMAIL_ADDRESS_LIST_ITEM) {
		elm_object_focus_set(view->entry_password.entry, EINA_TRUE);
	} else if (li->index == PASSWORD_LIST_ITEM) {
		_perform_account_validation(view);
	}
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

	if (response->err == EMAIL_ERROR_NONE || response->err == EMAIL_ERROR_VALIDATE_ACCOUNT_OF_SMTP) {
		view->is_retry_validate_with_username = 0;
		DELETE_TIMER_OBJECT(view->preset_vc_timer);

		if (response->err == EMAIL_ERROR_VALIDATE_ACCOUNT_OF_SMTP) {
			debug_warning("smtp validation failed but it can be ignored");
		}
		_update_account_capability(view, (const char *)(response->data));
		_update_account_smtp_mail_limit_size(view, (const char *)(response->data));
		view->preset_vc_timer = ecore_timer_add(0.5, _after_validation_cb, view);
	} else if (!(view->is_retry_validate_with_username) && (response->err != EMAIL_ERROR_CANCELLED)) {
		view->is_retry_validate_with_username = 1;
		_set_username_before_at(view);
		_validate_account(view);
	} else {
		view->is_retry_validate_with_username = 0;
		if (response->err != EMAIL_ERROR_CANCELLED) {
			const email_string_t *type = setting_get_service_fail_type(response->err);
			const email_string_t *header = setting_get_service_fail_type_header(response->err);
			module->popup = setting_get_notify(&view->base,
					header,
					type, 1, &(EMAIL_SETTING_STRING_OK),
					_popup_ok_cb, NULL, NULL);
		}
	}
}

static void _set_username_before_at(EmailSettingView *view)
{
	debug_enter();

	retm_if(!view, "view is NULL");

	EmailSettingModule *module = (EmailSettingModule *)view->base.module;
	email_account_t *account = module->new_account;

	if (account->incoming_server_user_name) {
		free(account->incoming_server_user_name);
	}
	if (account->outgoing_server_user_name) {
		free(account->outgoing_server_user_name);
	}

	char *buf = g_strdup(account->user_email_address);
	account->incoming_server_user_name = g_strdup(strtok(buf, "@"));
	account->outgoing_server_user_name = g_strdup(account->incoming_server_user_name);
	g_free(buf);

	debug_secure("retry to validate with user name: %s", account->incoming_server_user_name);
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
	retm_if(!data, "data is NULL");

	Elm_Object_Item *item = event_info;
	int index = (int)(ptrdiff_t)elm_object_item_data_get(item);

	EmailSettingView *view = data;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	DELETE_EVAS_OBJECT(module->popup);

	setting_set_others_account(&view->base);
	setting_set_others_account_server_default_type(module->new_account, index, 1, 1);

	create_manual_setup_view(module);
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

static void _gl_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	ListItemData *li = data;
	Elm_Object_Item *item = event_info;
	EmailSettingView *view = li->view;

	elm_genlist_item_selected_set(item, EINA_FALSE);

	if (li->index == DEFAULT_ACCOUNT_LIST_ITEM) {
		elm_check_state_set(view->default_account_check, !elm_check_state_get(view->default_account_check));
		_check_clicked_cb(li, view->default_account_check, NULL);
	} else if (li->index == SHOW_PASSWORD_LIST_ITEM) {
		elm_check_state_set(view->show_passwd_check, !elm_check_state_get(view->show_passwd_check));
		_check_clicked_cb(li, view->show_passwd_check, NULL);
	}
}
/* EOF */
