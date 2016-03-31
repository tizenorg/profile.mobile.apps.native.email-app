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

#include "email-setting-utils.h"
#include "email-setting-noti-mgr.h"
#include "email-view-setting.h"
#include "email-view-account-details.h"
#include "email-view-account-setup.h"

typedef struct view_data EmailSettingView;

#define ACCOUNT_COLOR_LINE_WIDTH 10
#define ACCOUNT_COLOR_LINE_HEIGHT 64

static int _create(email_view_t *self);
static void _destroy(email_view_t *self);
static void _update(email_view_t *self, int flags);
static void _on_back_key(email_view_t *self);

static int _update_list(EmailSettingView *view);

static void _update_default_option_value(EmailSettingView *view);
static void _get_default_option_value(EmailSettingView *view);
static void _push_naviframe(EmailSettingView *view);
static void _create_list(EmailSettingView *view);
static void _popup_ok_cb(void *data, Evas_Object *obj, void *event_info);
static void _show_finished_cb(void *data, Evas_Object *obj, void *event_info);

static Evas_Object *_create_account_color_bar(Evas_Object *parent, unsigned int color_label);
static Evas_Object *_gl_account_content_get_cb2(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_gl_account_content_get_cb(void *data, Evas_Object *obj, const char *part);
static char *_gl_text_get_cb(void *data, Evas_Object *obj, const char *part);
static char *_gl_text_get_cb2(void *data, Evas_Object *obj, const char *part);
static char *_gl_account_text_get_cb(void *data, Evas_Object *obj, const char *part);
static char *_gl_account_text_get_cb2(void *data, Evas_Object *obj, const char *part);
static void _gl_account_sel_cb(void *data, Evas_Object *obj, void *event_info);
static void _account_added_deleted_cb(int account_id, email_setting_response_data *response, void *user_data);
static void _account_updated_cb(int account_id, email_setting_response_data *response, void *user_data);
static void _priority_senders_email_cb(void *data, Evas_Object *obj, void *event_info);
static char *_gl_ex_text_get_cb(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_gl_ex_content_get_cb(void *data, Evas_Object *obj, const char *part);
static void _gl_default_sel_cb(void *data, Evas_Object *obj, void *event_info);
static void _gl_ex_sel_cb(void *data, Evas_Object *obj, void *event_info);
static void _gl_default_account_ex_radio_cb(void *data, Evas_Object *obj, void *event_info);
static void _create_toolbar_more_btn(EmailSettingView *view);
static void _toolbar_more_btn_cb(void *data, Evas_Object *obj, void *event_info);
static void _ctxpopup_dismissed_cb(void *data, Evas_Object *obj, void *event_info);
static void _move_more_ctxpopup(Evas_Object *ctxpopup, Evas_Object *win);
static void _delete_more_ctxpopup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _resize_more_ctxpopup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _add_account_cb(void *data, Evas_Object *obj, void *event_info);

static int _create_google_sync_view(EmailSettingView *view, int my_account_id);
static void _google_sync_view_result_cb(app_control_h request, app_control_h reply, app_control_result_e result, void *user_data);

static EmailSettingView *g_vd = NULL; /* TODO get rid */

static email_string_t EMAIL_SETTING_STRING_DEFAULT_SENDING_ACCOUNT = {PACKAGE, "IDS_EMAIL_TMBODY_DEFAULT_SENDING_ACCOUNT"};
static email_string_t EMAIL_SETTING_STRING_ACCOUNT = {PACKAGE, "IDS_EMAIL_HEADER_ACCOUNTS"};
static email_string_t EMAIL_SETTING_STRING_EMAIL_SETTINGS = {PACKAGE, "IDS_EMAIL_HEADER_SETTINGS"};
static email_string_t EMAIL_SETTING_STRING_GENERAL_SETTINGS = {PACKAGE, "IDS_EMAIL_HEADER_GENERAL"};
static email_string_t EMAIL_SETTING_STRING_PRIORITY_SENDERS = {PACKAGE, "IDS_EMAIL_MBODY_PRIORITY_SENDERS"};
static email_string_t EMAIL_SETTING_STRING_ADD_ACCOUNT = {PACKAGE, "IDS_EMAIL_OPT_ADD_ACCOUNT"};
static email_string_t EMAIL_SETTING_STRING_OK = {PACKAGE, "IDS_EMAIL_BUTTON_OK"};
static email_string_t EMAIL_SETTING_STRING_WARNING = {PACKAGE, "IDS_ST_HEADER_WARNING"};
static email_string_t EMAIL_SETTING_STRING_NO_ACCOUNT = {PACKAGE, "IDS_EMAIL_POP_YOU_HAVE_NOT_YET_CREATED_AN_EMAIL_ACCOUNT_CREATE_AN_EMAIL_ACCOUNT_AND_TRY_AGAIN"};

enum {
	GENERAL_SETTINGS_TITLE_LIST_ITEM,
	DEFAULT_SENDING_ACCOUNT_LIST_ITEM,
	PRIORITY_SENDER_LIST_ITEM,
	ACCOUNTS_LIST_TITLE_LIST_ITEM,
	ACCOUNTS_LIST_ITEM
};

typedef struct _AccountListItemData AccountListItemData;
typedef struct _ListItemData ListItemData;

static Evas_Object *_get_option_genlist(Evas_Object *parent, ListItemData *li);

struct view_data {
	email_view_t base;

	int def_acct_id;
	char *def_acct_addr;

	GSList *list_items;
	GSList *account_list_items;
	GSList *itc_list;

	Evas_Object *radio_grp;
	Evas_Object *ctx_popup;
	Evas_Object *more_btn;

	Evas_Object *genlist;
	Elm_Genlist_Item_Class *itc;
	Elm_Genlist_Item_Class *itc2;
	Elm_Genlist_Item_Class *itc3;
	Elm_Genlist_Item_Class *itc4;
	Elm_Genlist_Item_Class *itc5;
	Elm_Genlist_Item_Class *itc6;
	Elm_Genlist_Item_Class *itc7;

	Elm_Object_Item *first_account_item;
	Elm_Object_Item *last_account_item;

	AccountListItemData *selected_account_item;
};

struct _ListItemData {
	int index;
	Elm_Object_Item *gl_it;
	EmailSettingView *view;
};

struct _AccountListItemData {
	int index;
	email_account_t *account_data;
	Evas_Object *color_bar;
	Elm_Object_Item *it;
	EmailSettingView *view;
};

void create_setting_view(EmailSettingModule *module)
{
	debug_enter();

	retm_if(!module, "module is null");

	EmailSettingView *view = calloc(1, sizeof(EmailSettingView));
	retm_if(!view, "view data is null");

	view->base.create = _create;
	view->base.destroy = _destroy;
	view->base.update = _update;
	view->base.on_back_key = _on_back_key;

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

	g_vd = view;

	setting_noti_subscribe(module, EMAIL_SETTING_ACCOUNT_ADD_NOTI, _account_added_deleted_cb, view);
	setting_noti_subscribe(module, EMAIL_SETTING_ACCOUNT_UPDATE_NOTI, _account_updated_cb, view);
	setting_noti_subscribe(module, EMAIL_SETTING_ACCOUNT_DELETE_NOTI, _account_added_deleted_cb, view);

	_create_toolbar_more_btn(view);

	_update_list(view);

	return 0;
}

static void _create_toolbar_more_btn(EmailSettingView *view)
{
	debug_enter();
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	Evas_Object *btn = elm_button_add(module->base.navi);
	elm_object_style_set(btn, "naviframe/more/default");
	evas_object_smart_callback_add(btn, "clicked", _toolbar_more_btn_cb, view);

	elm_object_item_part_content_set(view->base.navi_item, "toolbar_more_btn", btn);
	module->more_btn = btn;
}

static void _toolbar_more_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailSettingView *view = data;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;
	Elm_Object_Item *it = NULL;

	DELETE_EVAS_OBJECT(view->ctx_popup);
	view->ctx_popup = elm_ctxpopup_add(module->base.win);
	elm_ctxpopup_auto_hide_disabled_set(view->ctx_popup, EINA_TRUE);
	elm_object_style_set(view->ctx_popup, "more/default");
	eext_object_event_callback_add(view->ctx_popup, EEXT_CALLBACK_BACK, _ctxpopup_dismissed_cb, view);
	eext_object_event_callback_add(view->ctx_popup, EEXT_CALLBACK_MORE, _ctxpopup_dismissed_cb, view);
	evas_object_smart_callback_add(view->ctx_popup, "dismissed", _ctxpopup_dismissed_cb, view);
	evas_object_event_callback_add(view->ctx_popup, EVAS_CALLBACK_DEL, _delete_more_ctxpopup_cb, view);
	evas_object_event_callback_add(module->base.navi, EVAS_CALLBACK_RESIZE, _resize_more_ctxpopup_cb, view);

	it = elm_ctxpopup_item_append(view->ctx_popup, EMAIL_SETTING_STRING_ADD_ACCOUNT.id, NULL, _add_account_cb, view);
	elm_object_item_domain_text_translatable_set(it, EMAIL_SETTING_STRING_ADD_ACCOUNT.domain, EINA_TRUE);

	elm_ctxpopup_direction_priority_set(view->ctx_popup, ELM_CTXPOPUP_DIRECTION_UP,
			ELM_CTXPOPUP_DIRECTION_UNKNOWN, ELM_CTXPOPUP_DIRECTION_UNKNOWN, ELM_CTXPOPUP_DIRECTION_UNKNOWN);

	_move_more_ctxpopup(view->ctx_popup, module->base.win);

	evas_object_show(view->ctx_popup);
}

static void _ctxpopup_dismissed_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailSettingView *view = data;

	DELETE_EVAS_OBJECT(view->ctx_popup);
}

static void _move_more_ctxpopup(Evas_Object *ctxpopup, Evas_Object *win)
{
	Evas_Coord w, h;
	int pos = -1;

	elm_win_screen_size_get(win, NULL, NULL, &w, &h);
	pos = elm_win_rotation_get(win);
	debug_log("pos: [%d], w[%d], h[%d]", pos, w, h);

	if (ctxpopup) {
		switch (pos) {
		case 0:
		case 180:
			evas_object_move(ctxpopup, (w / 2), h);
			break;
		case 90:
			evas_object_move(ctxpopup, (h / 2), w);
			break;
		case 270:
			evas_object_move(ctxpopup, (h / 2), w);
			break;
		}
	}
}

static void _delete_more_ctxpopup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailSettingView *view = data;

	eext_object_event_callback_del(view->ctx_popup, EEXT_CALLBACK_BACK, _ctxpopup_dismissed_cb);
	eext_object_event_callback_del(view->ctx_popup, EEXT_CALLBACK_MORE, _ctxpopup_dismissed_cb);
	evas_object_smart_callback_del(view->ctx_popup, "dismissed", _ctxpopup_dismissed_cb);
	evas_object_event_callback_del(view->ctx_popup, EVAS_CALLBACK_DEL, _delete_more_ctxpopup_cb);
	evas_object_event_callback_del(view->base.module->navi, EVAS_CALLBACK_RESIZE, _resize_more_ctxpopup_cb);

	debug_leave();
}

static void _resize_more_ctxpopup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailSettingView *view = data;

	_move_more_ctxpopup(view->ctx_popup, view->base.module->win);
}

static void _add_account_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailSettingView *view = data;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	DELETE_EVAS_OBJECT(view->ctx_popup);

	create_account_setup_view(module);
}

static int _update_list(EmailSettingView *view)
{
	debug_enter();

	retvm_if(!view, -1, "view data is null");

	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	if (setting_update_acct_list(&view->base) == FALSE) {
		module->account_count = 0;
		module->account_list = NULL;
	}

	_get_default_option_value(view);

	if (view->genlist) {
		elm_object_part_content_unset(view->base.content, "elm.swallow.content");
		elm_genlist_clear(view->genlist);
		evas_object_del(view->genlist);
		view->genlist = NULL;
	}

	_create_list(view);

	return 0;
}

static void _destroy(email_view_t *self)
{
	debug_enter();
	retm_if(!self, "self is null");

	EmailSettingView *view = (EmailSettingView *)self;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	setting_noti_unsubscribe(module, EMAIL_SETTING_ACCOUNT_ADD_NOTI, _account_added_deleted_cb);
	setting_noti_unsubscribe(module, EMAIL_SETTING_ACCOUNT_UPDATE_NOTI, _account_updated_cb);
	setting_noti_unsubscribe(module, EMAIL_SETTING_ACCOUNT_DELETE_NOTI, _account_added_deleted_cb);

	GSList *l = view->list_items;
	while (l) {
		free(l->data);
		l = g_slist_next(l);
	}
	g_slist_free(view->list_items);
	view->list_items = NULL;

	l = view->account_list_items;
	while (l) {
		free(l->data);
		l = g_slist_next(l);
	}
	g_slist_free(view->account_list_items);

	/* unref itc */
	l = view->itc_list;
	while (l) {
		Elm_Genlist_Item_Class *itc = l->data;
		EMAIL_GENLIST_ITC_FREE(itc);
		l = g_slist_next(l);
	}
	g_slist_free(view->itc_list);
	view->itc_list = NULL;

	DELETE_EVAS_OBJECT(module->popup);

	free(view);
}

static void _update(email_view_t *self, int flags)
{
	debug_enter();
	retm_if(!self, "self is null");

	EmailSettingView *view = (EmailSettingView *)self;

	if (flags & EVUF_LANGUAGE_CHANGED) {

		_get_default_option_value(view);

		/* refreshing genlist. */
		elm_genlist_realized_items_update(view->genlist);
	}

	if (flags & EVUF_POPPING) {
		_update_list(view);
	}
}

static void _push_naviframe(EmailSettingView *view)
{
	debug_enter();

	email_module_view_push(&view->base, EMAIL_SETTING_STRING_EMAIL_SETTINGS.id, 0);
	elm_object_item_domain_text_translatable_set(view->base.navi_item, EMAIL_SETTING_STRING_EMAIL_SETTINGS.domain, EINA_TRUE);

	evas_object_show(view->base.content);
}

static void _create_list(EmailSettingView *view)
{
	debug_enter();

	retm_if(!view, "view data is null");

	EmailSettingModule *module;
	email_account_t *account_data = NULL;
	int i = 0;
	ListItemData *li = NULL;
	AccountListItemData *ali = NULL;
	Elm_Object_Item *git = NULL;

	module = (EmailSettingModule *)view->base.module;

	GSList *l = view->list_items;
	while (l) {
		free(l->data);
		l = g_slist_next(l);
	}
	g_slist_free(view->list_items);
	view->list_items = NULL;

	l = view->account_list_items;
	while (l) {
		free(l->data);
		l = g_slist_next(l);
	}
	g_slist_free(view->account_list_items);
	view->account_list_items = NULL;

	view->radio_grp = elm_radio_add(view->base.content);
	elm_radio_value_set(view->radio_grp, -1);
	evas_object_hide(view->radio_grp);

	view->genlist = elm_genlist_add(view->base.content);
	elm_scroller_policy_set(view->genlist, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
	elm_genlist_mode_set(view->genlist, ELM_LIST_COMPRESS);
	elm_genlist_homogeneous_set(view->genlist, EINA_TRUE);

	view->itc = setting_get_genlist_class_item("type1", _gl_text_get_cb2, NULL, NULL, NULL);
	view->itc2 = setting_get_genlist_class_item("type1", _gl_ex_text_get_cb, _gl_ex_content_get_cb, NULL, NULL);
	view->itc3 = setting_get_genlist_class_item("type1", _gl_text_get_cb, NULL, NULL, NULL);
	if (module->account_count > 1) {
		view->itc4 = setting_get_genlist_class_item("full", NULL, _gl_account_content_get_cb, NULL, NULL);
		view->itc7 = setting_get_genlist_class_item("full", NULL, _gl_account_content_get_cb2, NULL, NULL);
	} else {
		view->itc4 = setting_get_genlist_class_item("type1", _gl_account_text_get_cb, NULL, NULL, NULL);
		view->itc7 = setting_get_genlist_class_item("type1", _gl_account_text_get_cb2, NULL, NULL, NULL);
	}
	view->itc5 = setting_get_genlist_class_item("group_index", _gl_text_get_cb, NULL, NULL, NULL);
	view->itc6 = setting_get_genlist_class_item("type1", _gl_text_get_cb2, NULL, NULL, NULL);

	view->itc_list = g_slist_append(view->itc_list, view->itc);
	view->itc_list = g_slist_append(view->itc_list, view->itc2);
	view->itc_list = g_slist_append(view->itc_list, view->itc3);
	view->itc_list = g_slist_append(view->itc_list, view->itc4);
	view->itc_list = g_slist_append(view->itc_list, view->itc5);
	view->itc_list = g_slist_append(view->itc_list, view->itc6);
	view->itc_list = g_slist_append(view->itc_list, view->itc7);

	/* title */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = GENERAL_SETTINGS_TITLE_LIST_ITEM;
	li->view = view;
	li->gl_it = git = elm_genlist_item_append(view->genlist, view->itc5, li, NULL,
			ELM_GENLIST_ITEM_GROUP, NULL, NULL);
	elm_genlist_item_select_mode_set(git, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
	view->list_items = g_slist_append(view->list_items, li);

	/* default sending account */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = DEFAULT_SENDING_ACCOUNT_LIST_ITEM;
	li->view = view;
	if (module->account_count > 0)
		li->gl_it = elm_genlist_item_append(view->genlist, view->itc6, li, NULL,
				ELM_GENLIST_ITEM_NONE, _gl_default_sel_cb, li);
	else {
		li->gl_it = git = elm_genlist_item_append(view->genlist, view->itc6, li, NULL,
				ELM_GENLIST_ITEM_NONE, NULL, NULL);
		elm_object_item_disabled_set(git, EINA_TRUE);
	}
	view->list_items = g_slist_append(view->list_items, li);

	/* priority senders */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = PRIORITY_SENDER_LIST_ITEM;
	li->view = view;
	li->gl_it = git = elm_genlist_item_append(view->genlist, view->itc3, li, NULL,
			ELM_GENLIST_ITEM_NONE, _priority_senders_email_cb, li);
	elm_genlist_item_select_mode_set(git, ELM_OBJECT_SELECT_MODE_ALWAYS);
	view->list_items = g_slist_append(view->list_items, li);

	/* title */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = ACCOUNTS_LIST_TITLE_LIST_ITEM;
	li->view = view;
	li->gl_it = git = elm_genlist_item_append(view->genlist, view->itc5, li, NULL,
			ELM_GENLIST_ITEM_GROUP, NULL, NULL);
	elm_genlist_item_select_mode_set(git, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
	view->list_items = g_slist_append(view->list_items, li);

	/* accounts */
	int is_first_account = 0;
	for (i = 0; i < module->account_count; i++) {
		account_data = &(module->account_list[i]);

		if (account_data) {
			ali = calloc(1, sizeof(AccountListItemData));
			if (!ali) {
				free(li);
				debug_error("ali: is NULL");
				return;
			}
			ali->index = i;
			ali->view = view;
			ali->account_data = account_data;
			if (!g_strcmp0(account_data->account_name, account_data->user_email_address)) {
				ali->it = elm_genlist_item_append(view->genlist, view->itc7,
						ali, NULL, ELM_GENLIST_ITEM_NONE,
						_gl_account_sel_cb, ali);
			} else {
				ali->it = elm_genlist_item_append(view->genlist, view->itc4,
						ali, NULL, ELM_GENLIST_ITEM_NONE,
						_gl_account_sel_cb, ali);
			}
			if (!is_first_account) {
				view->first_account_item = ali->it;
				is_first_account = 1;
			}
			view->account_list_items = g_slist_append(view->account_list_items, ali);
		}
	}

	if (ali)
		view->last_account_item = ali->it;

	elm_object_part_content_set(view->base.content, "elm.swallow.content", view->genlist);
}

static void _get_default_option_value(EmailSettingView *view)
{
	debug_enter();

	retm_if(!view, "view is null");

	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	int account_id = 0;
	int err = 0;
	int i = 0;

	/* getting default account */
	if (module->account_count > 0) {
		err = email_engine_get_default_account(&account_id);
		if (err == FALSE) {
			debug_error("failed to get default account");
			return;
		}

		debug_log("default account id (%d)", account_id);
		view->def_acct_id = account_id;

		if (view->def_acct_addr) {
			g_free(view->def_acct_addr);
			view->def_acct_addr = NULL;
		}

		for (i = 0; i < module->account_count; i++) {
			if (module->account_list[i].account_id == view->def_acct_id) {
				if (module->account_list[i].user_email_address != NULL) {
					debug_secure("Default account Addr : %s", module->account_list[i].user_email_address);
					view->def_acct_addr = g_strdup(module->account_list[i].user_email_address);
				}
			}
		}
	}
}

static void _update_default_option_value(EmailSettingView *view)
{
	debug_enter();

	retm_if(!view, "view is null");

	_get_default_option_value(view);

	elm_genlist_realized_items_update(view->genlist);
}

static void _on_back_key(email_view_t *self)
{
	debug_enter();

	EmailSettingView *view = (EmailSettingView *)self;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	evas_object_smart_callback_del(module->more_btn, "clicked", _toolbar_more_btn_cb);

	email_module_exit_view(&view->base);
}

static void _popup_ok_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "data is NULL");

	EmailSettingView *view = data;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	DELETE_EVAS_OBJECT(module->popup);
}

static char *_gl_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	ListItemData *li = data;

	if (!li)
		return NULL;
	if(!strcmp(part, "elm.text")) {
		if (li->index == ACCOUNTS_LIST_TITLE_LIST_ITEM) {
			return strdup(email_setting_gettext(EMAIL_SETTING_STRING_ACCOUNT));
		} else if (li->index == GENERAL_SETTINGS_TITLE_LIST_ITEM) {
			return g_strdup(email_setting_gettext(EMAIL_SETTING_STRING_GENERAL_SETTINGS));
		} else if (li->index == PRIORITY_SENDER_LIST_ITEM) {
			return strdup(email_setting_gettext(EMAIL_SETTING_STRING_PRIORITY_SENDERS));
		}
	}
	return NULL;
}

static char *_gl_account_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	AccountListItemData *ali = data;
	email_account_t *account_data = ali->account_data;

	if (!strcmp(part, "elm.text")) {
		return elm_entry_utf8_to_markup(account_data->account_name);
	} else if (!strcmp(part, "elm.text.sub")) {
		if(account_data->user_email_address) {
			return strdup(account_data->user_email_address);
		}
	}

	return NULL;
}

static char *_gl_account_text_get_cb2(void *data, Evas_Object *obj, const char *part)
{
	AccountListItemData *ali = data;
	email_account_t *account_data = ali->account_data;

	if (!strcmp(part, "elm.text")) {
		return elm_entry_utf8_to_markup(account_data->account_name);
	}

	return NULL;
}

static Evas_Object *_create_account_color_bar(Evas_Object *parent, unsigned int color_label)
{
	Evas_Object *color_bar = evas_object_rectangle_add(evas_object_evas_get(parent));
	int r = R_MASKING(color_label);
	int g = G_MASKING(color_label);
	int b = B_MASKING(color_label);
	int a = A_MASKING(color_label);

	evas_object_size_hint_fill_set(color_bar, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_color_set(color_bar, r, g, b, a);
	return color_bar;
}

static Evas_Object *_gl_account_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
	AccountListItemData *ali = data;
	if (ali && !strcmp(part, "elm.swallow.content")) {
		Evas_Object *full_item_ly = elm_layout_add(obj);
		elm_layout_file_set(full_item_ly, email_get_setting_theme_path(), "gl_accounts_2lines_item");
		evas_object_size_hint_weight_set(full_item_ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

		Evas_Object *color_bar = _create_account_color_bar(full_item_ly, ali->account_data->color_label);
		elm_object_part_content_set(full_item_ly, "elm.swallow.icon", color_bar);
		elm_object_part_text_set(full_item_ly, "elm.text", elm_entry_utf8_to_markup(ali->account_data->account_name));
		elm_object_part_text_set(full_item_ly, "elm.text.sub", strdup(ali->account_data->user_email_address));

		return full_item_ly;
	}
	return NULL;
}

static Evas_Object *_gl_account_content_get_cb2(void *data, Evas_Object *obj, const char *part)
{
	AccountListItemData *ali = data;
	if (ali && !strcmp(part, "elm.swallow.content")) {
		Evas_Object *full_item_ly = elm_layout_add(obj);
		elm_layout_file_set(full_item_ly, email_get_setting_theme_path(), "gl_accounts_1line_item");
		evas_object_size_hint_weight_set(full_item_ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

		Evas_Object *color_bar = _create_account_color_bar(full_item_ly, ali->account_data->color_label);
		elm_object_part_content_set(full_item_ly, "elm.swallow.icon", color_bar);
		elm_object_part_text_set(full_item_ly, "elm.text", elm_entry_utf8_to_markup(ali->account_data->account_name));

		return full_item_ly;
	}
	return NULL;
}

static void _gl_account_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	AccountListItemData *ail = data;
	email_account_t *account_data = ail->account_data;
	EmailSettingView *view = ail->view;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	Elm_Object_Item *item = event_info;
	elm_genlist_item_selected_set(item, EINA_FALSE);

	DELETE_EVAS_OBJECT(module->popup);

	if (module->account_count == 0) {
		module->popup = setting_get_notify(&view->base, &(EMAIL_SETTING_STRING_WARNING),
												&(EMAIL_SETTING_STRING_NO_ACCOUNT), 1,
												&(EMAIL_SETTING_STRING_OK),
												_popup_ok_cb, NULL, NULL);
	} else {
		module->account_id = -1;
		if (account_data->incoming_server_authentication_method == EMAIL_AUTHENTICATION_METHOD_XOAUTH2) {
			debug_log("select GOOGLE account");
			_create_google_sync_view(view, account_data->account_svc_id);
		} else {
			module->account_id = account_data->account_id;
			debug_log("selected account id %d", module->account_id);
			debug_log("select EMAIL account");
			create_account_details_view(module);
		}
	}

	return;
}

static void _account_added_deleted_cb(int account_id, email_setting_response_data *response, void *user_data)
{
	debug_enter();
	retm_if(!response, "response data is NULL");
	EmailSettingView *view = user_data;

	debug_log("account is added or deleted: %d", response->account_id);

	_update_list(view);
}

static void _account_updated_cb(int account_id, email_setting_response_data *response, void *user_data)
{
	debug_enter();
	retm_if(!response, "response data is NULL");
	EmailSettingView *view = user_data;

	debug_log("account is updated: %d", response->account_id);

	_update_default_option_value(view);
}

static void _priority_senders_email_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	ListItemData *li = data;
	EmailSettingView *view = li->view;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;
	Elm_Object_Item *item = event_info;

	elm_genlist_item_selected_set(item, EINA_FALSE);

	DELETE_EVAS_OBJECT(module->popup);

	if (module->account_count == 0) {
		module->popup = setting_get_notify(&view->base, &(EMAIL_SETTING_STRING_WARNING),
												&(EMAIL_SETTING_STRING_NO_ACCOUNT), 1,
												&(EMAIL_SETTING_STRING_OK),
												_popup_ok_cb, NULL, NULL);
	} else {
		email_params_h params = NULL;
		if (email_params_create(&params) &&
			email_params_add_str(params, EMAIL_BUNDLE_KEY_FILTER_OPERATION, EMAIL_BUNDLE_VAL_FILTER_OPERATION_PS) &&
			email_params_add_str(params, EMAIL_BUNDLE_KEY_FILTER_MODE, EMAIL_BUNDLE_VAL_FILTER_LIST)) {

			module->filter = email_module_create_child(&module->base, EMAIL_MODULE_FILTER,
					params, &module->filter_listener);
		}
		email_params_free(&params);
	}

	return;
}

static char *_gl_text_get_cb2(void *data, Evas_Object *obj, const char *part)
{
	ListItemData *li = data;
	retvm_if(!li, NULL, "li is null");

	if (!strcmp(part, "elm.text")) {
		if (li->index == DEFAULT_SENDING_ACCOUNT_LIST_ITEM)
			return strdup(email_setting_gettext(EMAIL_SETTING_STRING_DEFAULT_SENDING_ACCOUNT));
	} else if (!strcmp(part, "elm.text.sub")) {
		if (li->index == DEFAULT_SENDING_ACCOUNT_LIST_ITEM) {
			if(li->view->def_acct_addr) {
				return strdup(li->view->def_acct_addr);
			}

		}
	}

	return NULL;
}

static char *_gl_ex_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	int index = (int)(ptrdiff_t)data;
	EmailSettingView *view = g_vd;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	email_account_t *account_data = &(module->account_list[index]);

	if (!strcmp(part, "elm.text")) {
		return g_strdup(account_data->user_email_address);
	}

	return NULL;
}

static Evas_Object *_gl_ex_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
	int index = (int)(ptrdiff_t)data;
	EmailSettingView *view = g_vd;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;
	email_account_t *account_data = &(module->account_list[index]);

	if (!strcmp(part, "elm.swallow.end")) {
		Evas_Object *radio = elm_radio_add(obj);
		elm_radio_group_add(radio, view->radio_grp);
		elm_radio_state_value_set(radio, index);

		if (view->def_acct_id == account_data->account_id) {
			elm_radio_value_set(view->radio_grp, index);
		}
		evas_object_propagate_events_set(radio, EINA_FALSE);
		evas_object_smart_callback_add(radio, "changed", _gl_default_account_ex_radio_cb, (void *)(ptrdiff_t)index);
		return radio;
	}

	return NULL;
}

static void _gl_default_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	ListItemData *li = data;
	EmailSettingView *view = li->view;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;
	Elm_Object_Item *item = event_info;
	elm_genlist_item_selected_set(item, EINA_FALSE);

	Evas_Object *genlist = NULL;
	if (li->index == DEFAULT_SENDING_ACCOUNT_LIST_ITEM) {
		module->popup = setting_get_empty_content_notify(&view->base, &(EMAIL_SETTING_STRING_DEFAULT_SENDING_ACCOUNT),
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
	int acct_cnt = 0;
	int i = 0;
	email_account_t *account_data = NULL;

	if (li) {
		EmailSettingView *view = li->view;
		EmailSettingModule *module = (EmailSettingModule *)view->base.module;

		if (module->account_count > 0) {
			acct_cnt = module->account_count;
		}

		genlist = elm_genlist_add(parent);
		elm_genlist_homogeneous_set(genlist, EINA_TRUE);
		elm_scroller_policy_set(genlist, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
		elm_scroller_content_min_limit(genlist, EINA_FALSE, EINA_TRUE);

		if (li->index == DEFAULT_SENDING_ACCOUNT_LIST_ITEM) {
			for (i = 0; i < acct_cnt; i++) {
				account_data = &(module->account_list[i]);
				if (account_data) {
					elm_genlist_item_append(genlist, view->itc2, (void *)(ptrdiff_t)i,
							NULL, ELM_GENLIST_ITEM_NONE, _gl_ex_sel_cb, (void *)(ptrdiff_t)i);
				}
			}
		}

		evas_object_show(genlist);

		return genlist;
	}
	return NULL;
}


static void _gl_ex_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	int index = (int)(ptrdiff_t)data;
	EmailSettingView *view = g_vd;

	Elm_Object_Item *item = event_info;
	elm_genlist_item_selected_set(item, EINA_FALSE);
	elm_radio_value_set(view->radio_grp, index);

	_gl_default_account_ex_radio_cb((void *)(ptrdiff_t)index, NULL, NULL);
}

static void _gl_default_account_ex_radio_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	int index = (int)(ptrdiff_t)data;
	EmailSettingView *view = g_vd;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;
	email_account_t *account_data = &(module->account_list[index]);

	debug_log("Changed default account ID [%d]", account_data->account_id);

	if (email_engine_set_default_account(account_data->account_id)) {
		debug_log("new default account is %d", account_data->account_id);
		_get_default_option_value(view);
	}

	GSList *l = view->list_items;
	while (l) {
		ListItemData *li = l->data;
		if (li->index == DEFAULT_SENDING_ACCOUNT_LIST_ITEM) {
			elm_genlist_item_update(li->gl_it);
			elm_genlist_item_expanded_set(li->gl_it, EINA_FALSE);
			break;
		}
		l = g_slist_next(l);
	}
	DELETE_EVAS_OBJECT(module->popup);
}


static int _create_google_sync_view(EmailSettingView *view, int my_account_id)
{
	debug_enter();
	char my_account_id_str[30] = { 0, };
	app_control_h app_control = NULL;
	int ret;

	retvm_if(!view, 0, "view is NULL");

	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	ret = app_control_create(&app_control);
	retvm_if(!app_control, 0, "app_control_create failed: %d", ret);

	snprintf(my_account_id_str, 30, "%d", my_account_id);
	app_control_add_extra_data(app_control, ACCOUNT_DATA_ID, my_account_id_str);

	app_control_set_launch_mode(app_control, APP_CONTROL_LAUNCH_MODE_GROUP);

	app_control_set_app_id(app_control, "org.tizen.google-service-account-lite");
	app_control_set_operation(app_control, ACCOUNT_OPERATION_VIEW);

	ret = app_control_send_launch_request(app_control, _google_sync_view_result_cb, view);
	if (ret != APP_CONTROL_ERROR_NONE) {
		debug_error("app_control_send_launch_request failed: %d", ret);
		return 0;
	}

	module->app_control_google_eas = app_control;

	return 1;
}

static void _google_sync_view_result_cb(app_control_h request, app_control_h reply, app_control_result_e result, void *user_data)
{
	debug_enter();

	EmailSettingView *view = user_data;

	retm_if(!view, "view is NULL");

	/* google service will send result to me just before it is destroyed. */
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;
	if (module->app_control_google_eas) {
		app_control_destroy(module->app_control_google_eas);
		module->app_control_google_eas = NULL;
	}
}

static void _show_finished_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	ListItemData *li = data;
	EmailSettingView *view = li->view;
	int selected_index = 0;

	if (li->index == DEFAULT_SENDING_ACCOUNT_LIST_ITEM) {
		selected_index = elm_radio_value_get(view->radio_grp);
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
