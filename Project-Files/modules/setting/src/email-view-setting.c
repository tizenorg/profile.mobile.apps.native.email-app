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

typedef struct view_data EmailSettingVD;

#define ACCOUNT_COLOR_LINE_WIDTH 10
#define ACCOUNT_COLOR_LINE_HEIGHT 64

static int _create(email_view_t *self);
static void _destroy(email_view_t *self);
static void _update(email_view_t *self, int flags);
static void _on_back_key(email_view_t *self);

static int _update_list(EmailSettingVD *vd);

static void _update_default_option_value(EmailSettingVD *vd);
static void _get_default_option_value(EmailSettingVD *vd);
static void _push_naviframe(EmailSettingVD *vd);
static void _create_list(EmailSettingVD *vd);
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
static void _create_toolbar_more_btn(EmailSettingVD *vd);
static void _toolbar_more_btn_cb(void *data, Evas_Object *obj, void *event_info);
static void _ctxpopup_dismissed_cb(void *data, Evas_Object *obj, void *event_info);
static void _move_more_ctxpopup(Evas_Object *ctxpopup, Evas_Object *win);
static void _delete_more_ctxpopup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _resize_more_ctxpopup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _add_account_cb(void *data, Evas_Object *obj, void *event_info);

static int _create_google_sync_view(EmailSettingVD *vd, int my_account_id);
static void _google_sync_view_result_cb(app_control_h request, app_control_h reply, app_control_result_e result, void *user_data);

static EmailSettingVD *g_vd = NULL; /* TODO get rid */

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
	EmailSettingVD *vd;
};

struct _AccountListItemData {
	int index;
	email_account_t *account_data;
	Evas_Object *color_bar;
	Elm_Object_Item *it;
	EmailSettingVD *vd;
};

void create_setting_view(EmailSettingUGD *ugd)
{
	debug_enter();

	retm_if(!ugd, "ug data is null");

	EmailSettingVD *vd = calloc(1, sizeof(EmailSettingVD));
	retm_if(!vd, "view data is null");

	vd->base.create = _create;
	vd->base.destroy = _destroy;
	vd->base.update = _update;
	vd->base.on_back_key = _on_back_key;

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

	setting_noti_subscribe(ugd, EMAIL_SETTING_ACCOUNT_ADD_NOTI, _account_added_deleted_cb, vd);
	setting_noti_subscribe(ugd, EMAIL_SETTING_ACCOUNT_UPDATE_NOTI, _account_updated_cb, vd);
	setting_noti_subscribe(ugd, EMAIL_SETTING_ACCOUNT_DELETE_NOTI, _account_added_deleted_cb, vd);

	_create_toolbar_more_btn(vd);

	_update_list(vd);

	return 0;
}

static void _create_toolbar_more_btn(EmailSettingVD *vd)
{
	debug_enter();
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;

	Evas_Object *btn = elm_button_add(ugd->base.navi);
	elm_object_style_set(btn, "naviframe/more/default");
	evas_object_smart_callback_add(btn, "clicked", _toolbar_more_btn_cb, vd);

	elm_object_item_part_content_set(vd->base.navi_item, "toolbar_more_btn", btn);
	ugd->more_btn = btn;
}

static void _toolbar_more_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailSettingVD *vd = data;
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;
	Elm_Object_Item *it = NULL;

	DELETE_EVAS_OBJECT(vd->ctx_popup);
	vd->ctx_popup = elm_ctxpopup_add(ugd->base.win);
	elm_ctxpopup_auto_hide_disabled_set(vd->ctx_popup, EINA_TRUE);
	/* TODO: Should be uncommented when "more/default" style will be supported. */
//	elm_object_style_set(vd->ctx_popup, "more/default");
	eext_object_event_callback_add(vd->ctx_popup, EEXT_CALLBACK_BACK, _ctxpopup_dismissed_cb, vd);
	eext_object_event_callback_add(vd->ctx_popup, EEXT_CALLBACK_MORE, _ctxpopup_dismissed_cb, vd);
	evas_object_smart_callback_add(vd->ctx_popup, "dismissed", _ctxpopup_dismissed_cb, vd);
	evas_object_event_callback_add(vd->ctx_popup, EVAS_CALLBACK_DEL, _delete_more_ctxpopup_cb, vd);
	evas_object_event_callback_add(ugd->base.navi, EVAS_CALLBACK_RESIZE, _resize_more_ctxpopup_cb, vd);

	it = elm_ctxpopup_item_append(vd->ctx_popup, EMAIL_SETTING_STRING_ADD_ACCOUNT.id, NULL, _add_account_cb, vd);
	elm_object_item_domain_text_translatable_set(it, EMAIL_SETTING_STRING_ADD_ACCOUNT.domain, EINA_TRUE);

	elm_ctxpopup_direction_priority_set(vd->ctx_popup, ELM_CTXPOPUP_DIRECTION_UP,
			ELM_CTXPOPUP_DIRECTION_UNKNOWN, ELM_CTXPOPUP_DIRECTION_UNKNOWN, ELM_CTXPOPUP_DIRECTION_UNKNOWN);

	_move_more_ctxpopup(vd->ctx_popup, ugd->base.win);

	evas_object_show(vd->ctx_popup);
}

static void _ctxpopup_dismissed_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailSettingVD *vd = data;

	DELETE_EVAS_OBJECT(vd->ctx_popup);
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

	EmailSettingVD *vd = data;

	eext_object_event_callback_del(vd->ctx_popup, EEXT_CALLBACK_BACK, _ctxpopup_dismissed_cb);
	eext_object_event_callback_del(vd->ctx_popup, EEXT_CALLBACK_MORE, _ctxpopup_dismissed_cb);
	evas_object_smart_callback_del(vd->ctx_popup, "dismissed", _ctxpopup_dismissed_cb);
	evas_object_event_callback_del(vd->ctx_popup, EVAS_CALLBACK_DEL, _delete_more_ctxpopup_cb);
	evas_object_event_callback_del(vd->base.module->navi, EVAS_CALLBACK_RESIZE, _resize_more_ctxpopup_cb);

	debug_leave();
}

static void _resize_more_ctxpopup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailSettingVD *vd = data;

	_move_more_ctxpopup(vd->ctx_popup, vd->base.module->win);
}

static void _add_account_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailSettingVD *vd = data;
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;

	DELETE_EVAS_OBJECT(vd->ctx_popup);

	create_account_setup_view(ugd);
}

static int _update_list(EmailSettingVD *vd)
{
	debug_enter();

	retvm_if(!vd, -1, "view data is null");

	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;

	if (setting_update_acct_list(&vd->base) == FALSE) {
		ugd->account_count = 0;
		ugd->account_list = NULL;
	}

	_get_default_option_value(vd);

	if (vd->genlist) {
		elm_object_part_content_unset(vd->base.content, "elm.swallow.content");
		elm_genlist_clear(vd->genlist);
		evas_object_del(vd->genlist);
		vd->genlist = NULL;
	}

	_create_list(vd);

	return 0;
}

static void _destroy(email_view_t *self)
{
	debug_enter();
	retm_if(!self, "self is null");

	EmailSettingVD *vd = (EmailSettingVD *)self;
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;

	setting_noti_unsubscribe(ugd, EMAIL_SETTING_ACCOUNT_ADD_NOTI, _account_added_deleted_cb);
	setting_noti_unsubscribe(ugd, EMAIL_SETTING_ACCOUNT_UPDATE_NOTI, _account_updated_cb);
	setting_noti_unsubscribe(ugd, EMAIL_SETTING_ACCOUNT_DELETE_NOTI, _account_added_deleted_cb);

	GSList *l = vd->list_items;
	while (l) {
		free(l->data);
		l = g_slist_next(l);
	}
	g_slist_free(vd->list_items);
	vd->list_items = NULL;

	l = vd->account_list_items;
	while (l) {
		free(l->data);
		l = g_slist_next(l);
	}
	g_slist_free(vd->account_list_items);

	/* unref itc */
	l = vd->itc_list;
	while (l) {
		Elm_Genlist_Item_Class *itc = l->data;
		EMAIL_GENLIST_ITC_FREE(itc);
		l = g_slist_next(l);
	}
	g_slist_free(vd->itc_list);
	vd->itc_list = NULL;

	DELETE_EVAS_OBJECT(ugd->popup);

	free(vd);
}

static void _update(email_view_t *self, int flags)
{
	debug_enter();
	retm_if(!self, "self is null");

	EmailSettingVD *vd = (EmailSettingVD *)self;

	if (flags & EVUF_LANGUAGE_CHANGED) {

		_get_default_option_value(vd);

		/* refreshing genlist. */
		elm_genlist_realized_items_update(vd->genlist);
	}

	if (flags & EVUF_POPPING) {
		_update_list(vd);
	}
}

static void _push_naviframe(EmailSettingVD *vd)
{
	debug_enter();

	email_module_view_push(&vd->base, EMAIL_SETTING_STRING_EMAIL_SETTINGS.id, 0);
	elm_object_item_domain_text_translatable_set(vd->base.navi_item, EMAIL_SETTING_STRING_EMAIL_SETTINGS.domain, EINA_TRUE);

	evas_object_show(vd->base.content);
}

static void _create_list(EmailSettingVD *vd)
{
	debug_enter();

	retm_if(!vd, "view data is null");

	EmailSettingUGD *ugd;
	email_account_t *account_data = NULL;
	int i = 0;
	ListItemData *li = NULL;
	AccountListItemData *ali = NULL;
	Elm_Object_Item *git = NULL;

	ugd = (EmailSettingUGD *)vd->base.module;

	GSList *l = vd->list_items;
	while (l) {
		free(l->data);
		l = g_slist_next(l);
	}
	g_slist_free(vd->list_items);
	vd->list_items = NULL;

	l = vd->account_list_items;
	while (l) {
		free(l->data);
		l = g_slist_next(l);
	}
	g_slist_free(vd->account_list_items);
	vd->account_list_items = NULL;

	vd->radio_grp = elm_radio_add(vd->base.content);
	elm_radio_value_set(vd->radio_grp, -1);
	evas_object_hide(vd->radio_grp);

	vd->genlist = elm_genlist_add(vd->base.content);
	elm_scroller_policy_set(vd->genlist, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
	elm_genlist_mode_set(vd->genlist, ELM_LIST_COMPRESS);
	elm_genlist_homogeneous_set(vd->genlist, EINA_TRUE);

	vd->itc = setting_get_genlist_class_item("type1", _gl_text_get_cb2, NULL, NULL, NULL);
	vd->itc2 = setting_get_genlist_class_item("type1", _gl_ex_text_get_cb, _gl_ex_content_get_cb, NULL, NULL);
	vd->itc3 = setting_get_genlist_class_item("type1", _gl_text_get_cb, NULL, NULL, NULL);
	if (ugd->account_count > 1) {
		vd->itc4 = setting_get_genlist_class_item("full", NULL, _gl_account_content_get_cb, NULL, NULL);
		vd->itc7 = setting_get_genlist_class_item("full", NULL, _gl_account_content_get_cb2, NULL, NULL);
	} else {
		vd->itc4 = setting_get_genlist_class_item("type1", _gl_account_text_get_cb, NULL, NULL, NULL);
		vd->itc7 = setting_get_genlist_class_item("type1", _gl_account_text_get_cb2, NULL, NULL, NULL);
	}
	vd->itc5 = setting_get_genlist_class_item("group_index", _gl_text_get_cb, NULL, NULL, NULL);
	vd->itc6 = setting_get_genlist_class_item("type1", _gl_text_get_cb2, NULL, NULL, NULL);

	vd->itc_list = g_slist_append(vd->itc_list, vd->itc);
	vd->itc_list = g_slist_append(vd->itc_list, vd->itc2);
	vd->itc_list = g_slist_append(vd->itc_list, vd->itc3);
	vd->itc_list = g_slist_append(vd->itc_list, vd->itc4);
	vd->itc_list = g_slist_append(vd->itc_list, vd->itc5);
	vd->itc_list = g_slist_append(vd->itc_list, vd->itc6);
	vd->itc_list = g_slist_append(vd->itc_list, vd->itc7);

	/* title */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = GENERAL_SETTINGS_TITLE_LIST_ITEM;
	li->vd = vd;
	li->gl_it = git = elm_genlist_item_append(vd->genlist, vd->itc5, li, NULL,
			ELM_GENLIST_ITEM_GROUP, NULL, NULL);
	elm_genlist_item_select_mode_set(git, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
	vd->list_items = g_slist_append(vd->list_items, li);

	/* default sending account */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = DEFAULT_SENDING_ACCOUNT_LIST_ITEM;
	li->vd = vd;
	if (ugd->account_count > 0)
		li->gl_it = elm_genlist_item_append(vd->genlist, vd->itc6, li, NULL,
				ELM_GENLIST_ITEM_NONE, _gl_default_sel_cb, li);
	else {
		li->gl_it = git = elm_genlist_item_append(vd->genlist, vd->itc6, li, NULL,
				ELM_GENLIST_ITEM_NONE, NULL, NULL);
		elm_object_item_disabled_set(git, EINA_TRUE);
	}
	vd->list_items = g_slist_append(vd->list_items, li);

	/* priority senders */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = PRIORITY_SENDER_LIST_ITEM;
	li->vd = vd;
	li->gl_it = git = elm_genlist_item_append(vd->genlist, vd->itc3, li, NULL,
			ELM_GENLIST_ITEM_NONE, _priority_senders_email_cb, li);
	elm_genlist_item_select_mode_set(git, ELM_OBJECT_SELECT_MODE_ALWAYS);
	vd->list_items = g_slist_append(vd->list_items, li);

	/* title */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = ACCOUNTS_LIST_TITLE_LIST_ITEM;
	li->vd = vd;
	li->gl_it = git = elm_genlist_item_append(vd->genlist, vd->itc5, li, NULL,
			ELM_GENLIST_ITEM_GROUP, NULL, NULL);
	elm_genlist_item_select_mode_set(git, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
	vd->list_items = g_slist_append(vd->list_items, li);

	/* accounts */
	int is_first_account = 0;
	for (i = 0; i < ugd->account_count; i++) {
		account_data = &(ugd->account_list[i]);

		if (account_data) {
			ali = calloc(1, sizeof(AccountListItemData));
			if (!ali) {
				free(li);
				debug_error("ali: is NULL");
				return;
			}
			ali->index = i;
			ali->vd = vd;
			ali->account_data = account_data;
			if (!g_strcmp0(account_data->account_name, account_data->user_email_address)) {
				ali->it = elm_genlist_item_append(vd->genlist, vd->itc7,
						ali, NULL, ELM_GENLIST_ITEM_NONE,
						_gl_account_sel_cb, ali);
			} else {
				ali->it = elm_genlist_item_append(vd->genlist, vd->itc4,
						ali, NULL, ELM_GENLIST_ITEM_NONE,
						_gl_account_sel_cb, ali);
			}
			if (!is_first_account) {
				vd->first_account_item = ali->it;
				is_first_account = 1;
			}
			vd->account_list_items = g_slist_append(vd->account_list_items, ali);
		}
	}

	if (ali)
		vd->last_account_item = ali->it;

	elm_object_part_content_set(vd->base.content, "elm.swallow.content", vd->genlist);
}

static void _get_default_option_value(EmailSettingVD *vd)
{
	debug_enter();

	retm_if(!vd, "vd is null");

	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;

	int account_id = 0;
	int err = 0;
	int i = 0;

	/* getting default account */
	if (ugd->account_count > 0) {
		err = email_engine_get_default_account(&account_id);
		if (err == FALSE) {
			debug_error("failed to get default account");
			return;
		}

		debug_log("default account id (%d)", account_id);
		vd->def_acct_id = account_id;

		if (vd->def_acct_addr) {
			g_free(vd->def_acct_addr);
			vd->def_acct_addr = NULL;
		}

		for (i = 0; i < ugd->account_count; i++) {
			if (ugd->account_list[i].account_id == vd->def_acct_id) {
				if (ugd->account_list[i].user_email_address != NULL) {
					debug_secure("Default account Addr : %s", ugd->account_list[i].user_email_address);
					vd->def_acct_addr = g_strdup(ugd->account_list[i].user_email_address);
				}
			}
		}
	}
}

static void _update_default_option_value(EmailSettingVD *vd)
{
	debug_enter();

	retm_if(!vd, "vd is null");

	_get_default_option_value(vd);

	elm_genlist_realized_items_update(vd->genlist);
}

static void _on_back_key(email_view_t *self)
{
	debug_enter();

	EmailSettingVD *vd = (EmailSettingVD *)self;
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;

	evas_object_smart_callback_del(ugd->more_btn, "clicked", _toolbar_more_btn_cb);

	email_module_exit_view(&vd->base);
}

static void _popup_ok_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "data is NULL");

	EmailSettingVD *vd = data;
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;

	DELETE_EVAS_OBJECT(ugd->popup);
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
	EmailSettingVD *vd = ail->vd;
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;

	Elm_Object_Item *item = event_info;
	elm_genlist_item_selected_set(item, EINA_FALSE);

	DELETE_EVAS_OBJECT(ugd->popup);

	if (ugd->account_count == 0) {
		ugd->popup = setting_get_notify(&vd->base, &(EMAIL_SETTING_STRING_WARNING),
												&(EMAIL_SETTING_STRING_NO_ACCOUNT), 1,
												&(EMAIL_SETTING_STRING_OK),
												_popup_ok_cb, NULL, NULL);
	} else {
		ugd->account_id = -1;
		if (account_data->incoming_server_authentication_method == EMAIL_AUTHENTICATION_METHOD_XOAUTH2) {
			debug_log("select GOOGLE account");
			_create_google_sync_view(vd, account_data->account_svc_id);
		} else {
			ugd->account_id = account_data->account_id;
			debug_log("selected account id %d", ugd->account_id);
			debug_log("select EMAIL account");
			create_account_details_view(ugd);
		}
	}

	return;
}

static void _account_added_deleted_cb(int account_id, email_setting_response_data *response, void *user_data)
{
	debug_enter();
	retm_if(!response, "response data is NULL");
	EmailSettingVD *vd = user_data;

	debug_log("account is added or deleted: %d", response->account_id);

	_update_list(vd);
}

static void _account_updated_cb(int account_id, email_setting_response_data *response, void *user_data)
{
	debug_enter();
	retm_if(!response, "response data is NULL");
	EmailSettingVD *vd = user_data;

	debug_log("account is updated: %d", response->account_id);

	_update_default_option_value(vd);
}

static void _priority_senders_email_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	ListItemData *li = data;
	EmailSettingVD *vd = li->vd;
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;
	Elm_Object_Item *item = event_info;

	elm_genlist_item_selected_set(item, EINA_FALSE);

	DELETE_EVAS_OBJECT(ugd->popup);

	if (ugd->account_count == 0) {
		ugd->popup = setting_get_notify(&vd->base, &(EMAIL_SETTING_STRING_WARNING),
												&(EMAIL_SETTING_STRING_NO_ACCOUNT), 1,
												&(EMAIL_SETTING_STRING_OK),
												_popup_ok_cb, NULL, NULL);
	} else {
		app_control_h service = NULL;
		app_control_create(&service);
		if (service) {
			app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_FILTER_OPERATION,
					EMAIL_BUNDLE_VAL_FILTER_OPERATION_PS);
			app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_FILTER_MODE, EMAIL_BUNDLE_VAL_FILTER_LIST);

			ugd->filter = email_module_create_child(&ugd->base, EMAIL_MODULE_FILTER,
					service, &ugd->filter_listener);
			app_control_destroy(service);
		}
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
			if(li->vd->def_acct_addr) {
				return strdup(li->vd->def_acct_addr);
			}

		}
	}

	return NULL;
}

static char *_gl_ex_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	int index = (int)(ptrdiff_t)data;
	EmailSettingVD *vd = g_vd;
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;

	email_account_t *account_data = &(ugd->account_list[index]);

	if (!strcmp(part, "elm.text")) {
		return g_strdup(account_data->user_email_address);
	}

	return NULL;
}

static Evas_Object *_gl_ex_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
	int index = (int)(ptrdiff_t)data;
	EmailSettingVD *vd = g_vd;
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;
	email_account_t *account_data = &(ugd->account_list[index]);

	if (!strcmp(part, "elm.swallow.end")) {
		Evas_Object *radio = elm_radio_add(obj);
		elm_radio_group_add(radio, vd->radio_grp);
		elm_radio_state_value_set(radio, index);

		if (vd->def_acct_id == account_data->account_id) {
			elm_radio_value_set(vd->radio_grp, index);
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
	EmailSettingVD *vd = li->vd;
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;
	Elm_Object_Item *item = event_info;
	elm_genlist_item_selected_set(item, EINA_FALSE);

	Evas_Object *genlist = NULL;
	if (li->index == DEFAULT_SENDING_ACCOUNT_LIST_ITEM) {
		ugd->popup = setting_get_empty_content_notify(&vd->base, &(EMAIL_SETTING_STRING_DEFAULT_SENDING_ACCOUNT),
				0, NULL, NULL, NULL, NULL);
		genlist = _get_option_genlist(ugd->popup, li);
	}
	elm_object_content_set(ugd->popup, genlist);
	evas_object_show(ugd->popup);
	evas_object_smart_callback_add(ugd->popup, "show,finished", _show_finished_cb, li);
}

static Evas_Object *_get_option_genlist(Evas_Object *parent, ListItemData *li)
{
	debug_enter();
	Evas_Object *genlist = NULL;
	int acct_cnt = 0;
	int i = 0;
	email_account_t *account_data = NULL;

	if (li) {
		EmailSettingVD *vd = li->vd;
		EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;

		if (ugd->account_count > 0) {
			acct_cnt = ugd->account_count;
		}

		genlist = elm_genlist_add(parent);
		elm_genlist_homogeneous_set(genlist, EINA_TRUE);
		elm_scroller_policy_set(genlist, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
		elm_scroller_content_min_limit(genlist, EINA_FALSE, EINA_TRUE);

		if (li->index == DEFAULT_SENDING_ACCOUNT_LIST_ITEM) {
			for (i = 0; i < acct_cnt; i++) {
				account_data = &(ugd->account_list[i]);
				if (account_data) {
					elm_genlist_item_append(genlist, vd->itc2, (void *)(ptrdiff_t)i,
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
	EmailSettingVD *vd = g_vd;

	Elm_Object_Item *item = event_info;
	elm_genlist_item_selected_set(item, EINA_FALSE);
	elm_radio_value_set(vd->radio_grp, index);

	_gl_default_account_ex_radio_cb((void *)(ptrdiff_t)index, NULL, NULL);
}

static void _gl_default_account_ex_radio_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	int index = (int)(ptrdiff_t)data;
	EmailSettingVD *vd = g_vd;
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;
	email_account_t *account_data = &(ugd->account_list[index]);

	debug_log("Changed default account ID [%d]", account_data->account_id);

	if (email_engine_set_default_account(account_data->account_id)) {
		debug_log("new default account is %d", account_data->account_id);
		_get_default_option_value(vd);
	}

	GSList *l = vd->list_items;
	while (l) {
		ListItemData *li = l->data;
		if (li->index == DEFAULT_SENDING_ACCOUNT_LIST_ITEM) {
			elm_genlist_item_update(li->gl_it);
			elm_genlist_item_expanded_set(li->gl_it, EINA_FALSE);
			break;
		}
		l = g_slist_next(l);
	}
	DELETE_EVAS_OBJECT(ugd->popup);
}


static int _create_google_sync_view(EmailSettingVD *vd, int my_account_id)
{
	debug_enter();
	char my_account_id_str[30] = { 0, };
	app_control_h service = NULL;
	int ret;

	retvm_if(!vd, 0, "vd is NULL");

	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;

	ret = app_control_create(&service);
	retvm_if(!service, 0, "app_control_create failed: %d", ret);

	snprintf(my_account_id_str, 30, "%d", my_account_id);
	app_control_add_extra_data(service, ACCOUNT_DATA_ID, my_account_id_str);

	app_control_set_launch_mode(service, APP_CONTROL_LAUNCH_MODE_GROUP);

	app_control_set_app_id(service, "org.tizen.google-service-account-lite");
	app_control_set_operation(service, ACCOUNT_OPERATION_VIEW);

	ret = app_control_send_launch_request(service, _google_sync_view_result_cb, vd);
	if (ret != APP_CONTROL_ERROR_NONE) {
		debug_error("app_control_send_launch_request failed: %d", ret);
		return 0;
	}

	ugd->app_control_google_eas = service;

	return 1;
}

static void _google_sync_view_result_cb(app_control_h request, app_control_h reply, app_control_result_e result, void *user_data)
{
	debug_enter();

	EmailSettingVD *vd = user_data;

	retm_if(!vd, "vd is NULL");

	/* google service will send result to me just before it is destroyed. */
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;
	if (ugd->app_control_google_eas) {
		app_control_destroy(ugd->app_control_google_eas);
		ugd->app_control_google_eas = NULL;
	}
}

static void _show_finished_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	ListItemData *li = data;
	EmailSettingVD *vd = li->vd;
	int selected_index = 0;

	if (li->index == DEFAULT_SENDING_ACCOUNT_LIST_ITEM) {
		selected_index = elm_radio_value_get(vd->radio_grp);
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
