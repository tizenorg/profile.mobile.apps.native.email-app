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
#include "email-view-signature-setting.h"
#include "email-view-signature-edit.h"

typedef struct view_data EmailSettingVD;

static int _create(email_view_t *self);
static void _update(email_view_t *self, int flags);
static void _destroy(email_view_t *self);
static void _on_back_cb(email_view_t *self);

static void _update_list(EmailSettingVD *vd);

static void _push_naviframe(EmailSettingVD *vd);
static void _create_list(EmailSettingVD *vd);
static void _update_account_info(EmailSettingVD *vd);

static void _onoff_cb(void *data, Evas_Object *obj, void *event_info);
static char *_gl_text_get_cb(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_gl_content_get_cb(void *data, Evas_Object *obj, const char *part);
static void _gl_sel_cb(void *data, Evas_Object *obj, void *event_info);

static email_string_t EMAIL_SETTING_STRING_SHOW_SIGNATURE = {PACKAGE, "IDS_EMAIL_MBODY_DISPLAY_SIGNATURE"};
static email_string_t EMAIL_SETTING_STRING_EDIT_SIGNATURE = {PACKAGE, "IDS_EMAIL_MBODY_EDIT_SIGNATURE"};
static email_string_t EMAIL_SETTING_STRING_SIGNATURE = {PACKAGE, "IDS_EMAIL_MBODY_SIGNATURE"};

enum {
	EMAIL_SETTING_SHOW_SIGNATURE = -1000,
	EMAIL_SETTING_EDIT_SIGNATURE
};

struct view_data {
	email_view_t base;

	GSList *list_items;
	email_account_t *account_data;
	Evas_Object *genlist;
	Evas_Object *check;
	Elm_Genlist_Item_Class *itc1;
	Elm_Genlist_Item_Class *itc5;
};

typedef struct _ListItemData {
	int index;
	Elm_Object_Item *it;
	Evas_Object *check;
	Eina_Bool is_checked;
	EmailSettingVD *vd;
} ListItemData;

void create_signature_setting_view(EmailSettingUGD *ugd)
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

	if (!setting_get_acct_full_data(ugd->account_id, &(vd->account_data))) {
		debug_error("setting_get_acct_full_data failed");
		return -1;
	}

	EMAIL_SETTING_PRINT_ACCOUNT_INFO(vd->account_data);
	vd->base.content = setting_add_inner_layout(&vd->base);
	_push_naviframe(vd);

	_update_list(vd);

	return 0;
}

static void _update(email_view_t *self, int flags)
{
	debug_enter();
	retm_if(!self, "self is null");

	EmailSettingVD *vd = (EmailSettingVD *)self;

	if (flags & EVUF_POPPING) {
		_update_list(vd);
	}
}

static void _update_list(EmailSettingVD *vd)
{
	debug_enter();
	retm_if(!vd, "view data is null");

	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;

	if (vd->genlist) {
		elm_genlist_clear(vd->genlist);
		evas_object_del(vd->genlist);
		vd->genlist = NULL;
	}

	/* read account info again. */

	if (vd->account_data) {
		email_engine_free_account_list(&vd->account_data, 1);
		vd->account_data = NULL;
	}

	if (!setting_get_acct_full_data(ugd->account_id, &vd->account_data)) {
		debug_error("setting_get_acct_full_data failed");
		return;
	}

	EMAIL_SETTING_PRINT_ACCOUNT_INFO(vd->account_data);

	_create_list(vd);
}

static void _destroy(email_view_t *self)
{
	debug_enter();
	retm_if(!self, "self is null");

	EmailSettingVD *vd = (EmailSettingVD *)self;

	GSList *l = vd->list_items;
	while (l) {
		free(l->data);
		l = g_slist_next(l);
	}
	g_slist_free(vd->list_items);

	EMAIL_GENLIST_ITC_FREE(vd->itc1);
	EMAIL_GENLIST_ITC_FREE(vd->itc5);

	free(vd);
}

static void _push_naviframe(EmailSettingVD *vd)
{
	debug_enter();
	Elm_Object_Item *navi_it = NULL;

	navi_it = email_module_view_push(&vd->base, EMAIL_SETTING_STRING_SIGNATURE.id, 0);
	elm_object_item_domain_text_translatable_set(navi_it, EMAIL_SETTING_STRING_SIGNATURE.domain, EINA_TRUE);

	evas_object_show(vd->base.content);
}

static void _gl_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	ListItemData *li = data;
	EmailSettingVD *vd = li->vd;
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;
	Elm_Object_Item *item = event_info;

	elm_genlist_item_selected_set(item, EINA_FALSE);

	if (li->index == EMAIL_SETTING_SHOW_SIGNATURE) {
		_onoff_cb(li, li->check, NULL);
		elm_genlist_item_update(item);
	} else {
		create_signature_edit_view(ugd);
	}
}

static Evas_Object *_gl_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
	retvm_if(!data, NULL, "data is NULL");
	ListItemData *li = data;
	EmailSettingVD *vd = li->vd;

	if (!strcmp(part, "elm.swallow.end")) {
		if (li->index == EMAIL_SETTING_SHOW_SIGNATURE) {
			Evas_Object *check = NULL;
			check = elm_check_add(obj);
			elm_object_style_set(check, "on&off");
			evas_object_smart_callback_add(check, "changed", _onoff_cb, li);
			evas_object_propagate_events_set(check, EINA_FALSE);
			li->is_checked = vd->account_data->options.add_signature;
			elm_check_state_set(check, li->is_checked);
			vd->check = check;
			return check;
		}
	}
	return NULL;
}

static char *_gl_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	retvm_if(!data, NULL, "data is NULL");
	ListItemData *li = data;
	EmailSettingVD *vd = li->vd;

	if (li->index == EMAIL_SETTING_SHOW_SIGNATURE) {
		if (!strcmp(part, "elm.text")) {
			return strdup(email_setting_gettext(EMAIL_SETTING_STRING_SHOW_SIGNATURE));
		}
	} else if (li->index == EMAIL_SETTING_EDIT_SIGNATURE) {
		if (!strcmp(part, "elm.text")) {
			return strdup(email_setting_gettext(EMAIL_SETTING_STRING_EDIT_SIGNATURE));
		} else if (!strcmp(part, "elm.text.sub")) {
			return elm_entry_utf8_to_markup(dgettext(PACKAGE, vd->account_data->options.signature));
		}
	}
	return NULL;
}

static void _create_list(EmailSettingVD *vd)
{
	debug_enter();

	Evas_Object *genlist = NULL;
	Elm_Object_Item *git = NULL;
	ListItemData *li = NULL;

	genlist = elm_genlist_add(vd->base.content);
	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
	elm_scroller_policy_set(genlist, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
	vd->genlist = genlist;

	vd->itc1 = setting_get_genlist_class_item("type1", _gl_text_get_cb, _gl_content_get_cb, NULL, NULL);
	vd->itc5 = setting_get_genlist_class_item("type1", _gl_text_get_cb, NULL, NULL, NULL);

	/* show signature */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = EMAIL_SETTING_SHOW_SIGNATURE;
	li->vd = vd;
	li->it = git = elm_genlist_item_append(genlist, vd->itc1, li, NULL,
			ELM_GENLIST_ITEM_NONE, _gl_sel_cb, li);
	elm_genlist_item_select_mode_set(git, ELM_OBJECT_SELECT_MODE_ALWAYS);
	vd->list_items = g_slist_append(vd->list_items, li);

	/* edit signature */
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = EMAIL_SETTING_EDIT_SIGNATURE;
	li->vd = vd;
	li->it = git = elm_genlist_item_append(genlist, vd->itc5, li, NULL,
			ELM_GENLIST_ITEM_NONE, _gl_sel_cb, li);
	elm_genlist_item_select_mode_set(git, ELM_OBJECT_SELECT_MODE_ALWAYS);
	vd->list_items = g_slist_append(vd->list_items, li);

	elm_object_part_content_set(vd->base.content, "elm.swallow.content", genlist);
	evas_object_show(genlist);
}

static void _on_back_cb(email_view_t *self)
{
	debug_enter();
	retm_if(!self, "self is NULL");

	EmailSettingVD *vd = (EmailSettingVD *)self;

	/* save on/off state */
	debug_secure("set signature state as : %d", vd->account_data->options.add_signature);
	_update_account_info(vd);
	email_module_exit_view(&vd->base);
}

static void _onoff_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	ListItemData *li = data;
	EmailSettingVD *vd = li->vd;
	li->is_checked = !li->is_checked;
	elm_check_state_set(obj, li->is_checked);

	vd->account_data->options.add_signature = li->is_checked;
	_update_account_info(vd);
}

static void _update_account_info(EmailSettingVD *vd)
{
	debug_enter();

	retm_if(!vd, "view data is null");

	retm_if(!(vd->account_data), "account_data is null");

	if (email_engine_update_account(vd->account_data->account_id, vd->account_data) == TRUE)
		debug_log("Account updated successfully");
}
/* EOF */
