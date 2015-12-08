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

#include <contacts.h>
#include "email-filter-list-view.h"
#include "email-filter-edit-view.h"
#include "email-filter-delete-view.h"

typedef struct _EmailFilterListVD EmailFilterVD;

/* Internal functions */
static int _create(email_view_t *self);
static void _destroy(email_view_t *self);
static void _update(email_view_t *self, int flags);
static void _on_back_key(email_view_t *self);

static void _update_list(EmailFilterVD *vd);

static void _gl_sel_cb(void *data, Evas_Object *obj, void *event_info);
static char *_gl_text_get_cb(void *data, Evas_Object *obj, const char *part);
static char *_gl_text_get_cb2(void *data, Evas_Object *obj, const char *part);
static void _add_filter_cb(void *data, Evas_Object *obj, void *event_info);
static void _delete_filter_cb(void *data, Evas_Object *obj, void *event_info);
static Evas_Object *_create_rule_list_layout(EmailFilterVD *vd, email_rule_t *filter_rule_list, int filter_count);
static Evas_Object *_create_no_content_layout(EmailFilterVD *vd);
static int _get_filter_rule_list(EmailFilterVD *vd, email_rule_t **filter_rule_list, int *filter_rule_count);

static void _create_toolbar_more_btn(EmailFilterVD *vd);
static void _toolbar_more_btn_cb(void *data, Evas_Object *obj, void *event_info);
static void _ctxpopup_dismissed_cb(void *data, Evas_Object *obj, void *event_info);
static void _move_more_ctxpopup(Evas_Object *ctxpopup, Evas_Object *win);
static void _delete_more_ctxpopup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _resize_more_ctxpopup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

enum {
	EMAIL_PRIORITY_SENDERS_DEFAULT_FOLDER_LIST_ITEM = -1000,
	EMAIL_PRIORITY_SENDERS_NO_CONTENT_ITEM,
};

static email_string_t EMAIL_FILTER_STRING_NO_PRIORITY_SENDERS = {PACKAGE, "IDS_EMAIL_NPBODY_NO_PRIORITY_SENDERS"};
static email_string_t EMAIL_FILTER_STRING_ADD = {PACKAGE, "IDS_EMAIL_OPT_ADD"};
static email_string_t EMAIL_FILTER_STRING_REMOVE = {PACKAGE, "IDS_EMAIL_OPT_REMOVE"};
static email_string_t EMAIL_FILTER_STRING_NO_PRIORITY_SENDERS_HELP = {PACKAGE, "IDS_EMAIL_BODY_AFTER_YOU_ADD_PRIORITY_SENDERS_THEY_WILL_BE_SHOWN_HERE"};
static email_string_t EMAIL_FILTER_STRING_PRIORITY_SENDERS = {PACKAGE, "IDS_EMAIL_HEADER_PRIORITY_SENDERS_ABB"};

/* Internal structure */
struct _EmailFilterListVD {
	email_view_t base;

	Evas_Object *content_layout;
	Evas_Object *genlist;
	Evas_Object *add_btn;
	Evas_Object *ctx_popup;
	Evas_Object *more_btn;
	GSList *list_items;
	email_rule_t *filter_rule_list;

	int filter_rule_count;

	Elm_Genlist_Item_Class itc1;
	Elm_Genlist_Item_Class itc2;
};

typedef struct _ListItemData {
	int index;
	Elm_Object_Item *it;
	Evas_Object *check;
	Eina_Bool is_checked;
	EmailFilterVD *vd;
	email_rule_t *filter_rule;
	contacts_list_h ct_list;
	contacts_record_h ct_value;
} ListItemData;

void create_filter_list_view(EmailFilterUGD *ugd)
{
	debug_enter();
	retm_if(!ugd, "ug data is null");

	EmailFilterVD *vd = calloc(1, sizeof(EmailFilterVD));
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
	retvm_if(!self, -1, "self is NULL");

	EmailFilterVD *vd = (EmailFilterVD *)self;

	Elm_Object_Item *navi_it = NULL;

	Evas_Object *layout = elm_layout_add(vd->base.module->navi);
	elm_layout_theme_set(layout, "layout", "application", "default");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	vd->base.content = layout;

	navi_it = email_module_view_push(&vd->base, EMAIL_FILTER_STRING_PRIORITY_SENDERS.id, 0);
	elm_object_item_domain_text_translatable_set(navi_it, EMAIL_FILTER_STRING_PRIORITY_SENDERS.domain, EINA_TRUE);

	_create_toolbar_more_btn(vd);

	_update_list(vd);

	return 0;
}

static void _update(email_view_t *self, int flags)
{
	debug_enter();
	retm_if(!self, "self is NULL");

	EmailFilterVD *vd = (EmailFilterVD *)self;

	if (flags & EVUF_POPPING) {
		_update_list(vd);
		return;
	}

	if (flags & EVUF_LANGUAGE_CHANGED) {
		elm_genlist_realized_items_update(vd->genlist);
	}
}

static void _update_list(EmailFilterVD *vd)
{
	int ret = 0;

	GSList *l = vd->list_items;
	while (l) {
		ListItemData *li = l->data;
		if (li->ct_list)
			contacts_list_destroy(li->ct_list, 1);
		li->ct_list = NULL;
		li->ct_value = NULL;
		FREE(li);
		l = g_slist_next(l);
	}

	if (vd->list_items) {
		g_slist_free(vd->list_items);
		vd->list_items = NULL;
	}

	if (vd->genlist) {
		elm_genlist_clear(vd->genlist);
		DELETE_EVAS_OBJECT(vd->genlist);
	}

	if (vd->content_layout) {
		elm_layout_content_unset(vd->base.content, "elm.swallow.content");
		DELETE_EVAS_OBJECT(vd->content_layout);
	}

	if (vd->filter_rule_list) {
		email_free_rule(&vd->filter_rule_list, vd->filter_rule_count);
	}

	ret = _get_filter_rule_list(vd, &vd->filter_rule_list, &vd->filter_rule_count);
	if (ret < 0)
		debug_warning("_get_filter_rule_list failed");

	debug_log("p_vd->filter_rule_count: %d", vd->filter_rule_count);
	if (vd->filter_rule_count > 0) {
		vd->content_layout = _create_rule_list_layout(vd, vd->filter_rule_list, vd->filter_rule_count);
	} else {
		vd->content_layout = _create_no_content_layout(vd);
	}

	elm_layout_content_set(vd->base.content, "elm.swallow.content", vd->content_layout);
}

static void _destroy(email_view_t *self)
{
	debug_enter();
	retm_if(!self, "self is NULL");

	EmailFilterVD *vd = (EmailFilterVD *)self;

	GSList *l = vd->list_items;
	while (l) {
		ListItemData *li = l->data;
		if (li->ct_list)
			contacts_list_destroy(li->ct_list, 1);
		li->ct_list = NULL;
		li->ct_value = NULL;
		FREE(li);
		l = g_slist_next(l);
	}

	if (vd->list_items) {
		g_slist_free(vd->list_items);
		vd->list_items = NULL;
	}

	if (vd->filter_rule_list) {
		email_free_rule(&vd->filter_rule_list, vd->filter_rule_count);
	}

	if (vd->content_layout) {
		elm_layout_content_unset(vd->base.content, "elm.swallow.content");
		DELETE_EVAS_OBJECT(vd->content_layout);
	}

	free(vd);
}

static void _add_filter_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailFilterVD *vd = data;
	EmailFilterUGD *ugd = (EmailFilterUGD *)vd->base.module;

	DELETE_EVAS_OBJECT(vd->ctx_popup);

	create_filter_edit_view(ugd, FILTER_EDIT_VIEW_MODE_ADD_NEW, 0);
}

static void _delete_filter_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailFilterVD *vd = data;
	EmailFilterUGD *ugd = (EmailFilterUGD *)vd->base.module;

	DELETE_EVAS_OBJECT(vd->ctx_popup);

	create_filter_delete_view(ugd);
}

static void _on_back_key(email_view_t *self)
{
	debug_enter();
	retm_if(!self, "self is NULL");

	email_module_exit_view(self);
}

static Evas_Object *_create_rule_list_layout(EmailFilterVD *vd, email_rule_t *filter_rule_list, int filter_count)
{
	debug_enter();
	EmailFilterUGD *ugd = (EmailFilterUGD *)vd->base.module;
	Evas_Object *content_ly = NULL;
	Evas_Object *genlist = NULL;
	Elm_Object_Item *git = NULL;
	ListItemData *li = NULL;
	int i = 0;

	content_ly = elm_layout_add(vd->base.content);
	elm_layout_theme_set(content_ly, "layout", "application", "default");
	evas_object_size_hint_weight_set(content_ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(content_ly, EVAS_HINT_FILL, EVAS_HINT_FILL);

	genlist = elm_genlist_add(content_ly);
	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
	elm_scroller_policy_set(genlist, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
	vd->genlist = genlist;

	vd->itc1.item_style = "type1";
	vd->itc1.func.text_get = _gl_text_get_cb;
	vd->itc1.func.content_get = NULL;
	vd->itc1.func.state_get = NULL;
	vd->itc1.func.del = NULL;

	vd->itc2.item_style = "type1";
	vd->itc2.func.text_get = _gl_text_get_cb2;
	vd->itc2.func.content_get = NULL;
	vd->itc2.func.state_get = NULL;
	vd->itc2.func.del = NULL;

	for (i = 0; i < filter_count; i++) {
		li = calloc(1, sizeof(ListItemData));
		retvm_if(!li, NULL, "memory allocation failed");

		li->index = i;
		li->vd = vd;
		li->filter_rule = filter_rule_list + i;
		if (ugd->op_type == EMAIL_FILTER_OPERATION_PRIORITY_SERNDER &&
				STR_VALID(li->filter_rule->value2)) {
			contacts_list_h ct_list = NULL;
			contacts_record_h ct_val = NULL;
			email_get_contacts_list(CONTACTS_MATCH_FULLSTRING, &ct_list, li->filter_rule->value2, EMAIL_SEARCH_CONTACT_BY_EMAIL);
			li->ct_list = ct_list;
			email_get_last_contact_in_contact_list(ct_list, &ct_val);
			li->ct_value = ct_val;
		}
		li->it = git = elm_genlist_item_append(genlist,
				li->ct_value ? &(vd->itc2) : &(vd->itc1), li, NULL,
				ELM_GENLIST_ITEM_NONE, _gl_sel_cb, li);
		elm_genlist_item_select_mode_set(git, ELM_OBJECT_SELECT_MODE_ALWAYS);
		vd->list_items = g_slist_append(vd->list_items, li);
	}

	elm_object_part_content_set(content_ly, "elm.swallow.content", genlist);
	evas_object_show(content_ly);
	evas_object_show(genlist);

	return content_ly;
}

static Evas_Object *_create_no_content_layout(EmailFilterVD *vd)
{
	debug_enter();
	Evas_Object *no_ly = NULL;

	no_ly = elm_layout_add(vd->base.module->navi);
	elm_layout_theme_set(no_ly, "layout", "nocontents", "default");

	elm_object_domain_translatable_part_text_set(no_ly, "elm.text", EMAIL_FILTER_STRING_NO_PRIORITY_SENDERS.domain, EMAIL_FILTER_STRING_NO_PRIORITY_SENDERS.id);
	elm_object_domain_translatable_part_text_set(no_ly, "elm.help.text", EMAIL_FILTER_STRING_NO_PRIORITY_SENDERS_HELP.domain, EMAIL_FILTER_STRING_NO_PRIORITY_SENDERS_HELP.id);
	elm_layout_signal_emit(no_ly, "text,disabled", "");
	elm_layout_signal_emit(no_ly, "align.center", "elm");

	return no_ly;
}

static int _get_filter_rule_list(EmailFilterVD *vd, email_rule_t **filter_rule_list, int *filter_rule_count)
{
	debug_enter();
	EmailFilterUGD *ugd = (EmailFilterUGD *)vd->base.module;
	email_rule_t *rule_list = NULL;
	int count;
	int ret = 0;
	int i = 0;
	int ret_count = 0;
	int op_type = 0;

	ret = email_get_rule_list(&rule_list, &count);
	retvm_if(ret != EMAIL_ERROR_NONE, -1, "email_get_rule_list failed: %d", ret);

	/* get count */
	for (i = 0; i < count; i++) {
		if (ugd->op_type == EMAIL_FILTER_OPERATION_FILTER) {
			op_type = (!(rule_list[i].type & EMAIL_PRIORITY_SENDER))
				&& (rule_list[i].faction == EMAIL_FILTER_MOVE);
		} else {
			op_type = rule_list[i].type == EMAIL_PRIORITY_SENDER;
		}

		if (op_type) {
			ret_count++;
		}
	}

	/* get filter rule list */
	*filter_rule_list = NULL;
	*filter_rule_list = calloc(ret_count, sizeof(email_rule_t));
	ret_count = 0;
	for (i = 0; i < count; i++) {
		if (ugd->op_type == EMAIL_FILTER_OPERATION_FILTER) {
			op_type = (!(rule_list[i].type & EMAIL_PRIORITY_SENDER))
				&& (rule_list[i].faction == EMAIL_FILTER_MOVE);
		} else {
			op_type = rule_list[i].type == EMAIL_PRIORITY_SENDER;
		}

		if (op_type) {
			(*filter_rule_list)[ret_count].account_id = rule_list[i].account_id;
			(*filter_rule_list)[ret_count].filter_id = rule_list[i].filter_id;
			(*filter_rule_list)[ret_count].filter_name = g_strdup(rule_list[i].filter_name);
			(*filter_rule_list)[ret_count].type = rule_list[i].type;
			(*filter_rule_list)[ret_count].value = g_strdup(rule_list[i].value);
			(*filter_rule_list)[ret_count].value2 = g_strdup(rule_list[i].value2);
			(*filter_rule_list)[ret_count].faction = rule_list[i].faction;
			(*filter_rule_list)[ret_count].target_mailbox_id = rule_list[i].target_mailbox_id;
			(*filter_rule_list)[ret_count].flag1 = rule_list[i].flag1;
			(*filter_rule_list)[ret_count].flag2 = rule_list[i].flag2;
			ret_count++;
		}
	}

	*filter_rule_count = ret_count;

	return 0;
}

static char *_gl_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	ListItemData *li = data;

	if (li && li->filter_rule && !strcmp(part, "elm.text")) {
		email_rule_t *filter_rule = li->filter_rule;
		return elm_entry_utf8_to_markup(filter_rule->value2);
	}
	return NULL;
}

static char *_gl_text_get_cb2(void *data, Evas_Object *obj, const char *part)
{
	ListItemData *li = data;
	if (li) {
		email_rule_t *filter_rule = li->filter_rule;
		if (!strcmp(part, "elm.text") && filter_rule) {
			char *name = NULL;
			email_get_contacts_display_name(li->ct_value, &name);
			return elm_entry_utf8_to_markup(name);
		} else if (!strcmp(part, "elm.text.sub") && filter_rule) {
			return elm_entry_utf8_to_markup(filter_rule->value2);
		}
	}
	return NULL;
}

static void _gl_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	ListItemData *li = data;
	EmailFilterVD *vd = li->vd;
	EmailFilterUGD *ugd = (EmailFilterUGD *)vd->base.module;
	Elm_Object_Item *item = event_info;

	elm_genlist_item_selected_set(item, EINA_FALSE);

	create_filter_edit_view(ugd,
			FILTER_EDIT_VIEW_MODE_EDIT_EXISTENT,
			li->filter_rule->filter_id);
}


static void _create_toolbar_more_btn(EmailFilterVD *vd)
{
	debug_enter();

	Evas_Object *btn = elm_button_add(vd->base.module->navi);
	elm_object_style_set(btn, "naviframe/more/default");
	evas_object_smart_callback_add(btn, "clicked", _toolbar_more_btn_cb, vd);

	elm_object_item_part_content_set(vd->base.navi_item, "toolbar_more_btn", btn);
	vd->more_btn = btn;
}

static void _resize_more_ctxpopup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailFilterVD *vd = data;

	_move_more_ctxpopup(vd->ctx_popup, vd->base.module->win);
}

static void _delete_more_ctxpopup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailFilterVD *vd = data;

	eext_object_event_callback_del(vd->ctx_popup, EEXT_CALLBACK_BACK, _ctxpopup_dismissed_cb);
	eext_object_event_callback_del(vd->ctx_popup, EEXT_CALLBACK_MORE, _ctxpopup_dismissed_cb);
	evas_object_smart_callback_del(vd->ctx_popup, "dismissed", _ctxpopup_dismissed_cb);
	evas_object_event_callback_del(vd->ctx_popup, EVAS_CALLBACK_DEL, _delete_more_ctxpopup_cb);
	evas_object_event_callback_del(vd->base.module->navi, EVAS_CALLBACK_RESIZE, _resize_more_ctxpopup_cb);

	debug_leave();
}

static void _toolbar_more_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailFilterVD *vd = data;
	Elm_Object_Item *it = NULL;

	DELETE_EVAS_OBJECT(vd->ctx_popup);
	vd->ctx_popup = elm_ctxpopup_add(vd->base.module->win);
	elm_ctxpopup_auto_hide_disabled_set(vd->ctx_popup, EINA_TRUE);
	elm_object_style_set(vd->ctx_popup, "more/default");
	eext_object_event_callback_add(vd->ctx_popup, EEXT_CALLBACK_BACK, _ctxpopup_dismissed_cb, vd);
	eext_object_event_callback_add(vd->ctx_popup, EEXT_CALLBACK_MORE, _ctxpopup_dismissed_cb, vd);
	evas_object_smart_callback_add(vd->ctx_popup, "dismissed", _ctxpopup_dismissed_cb, vd);
	evas_object_event_callback_add(vd->ctx_popup, EVAS_CALLBACK_DEL, _delete_more_ctxpopup_cb, vd);
	evas_object_event_callback_add(vd->base.module->navi, EVAS_CALLBACK_RESIZE, _resize_more_ctxpopup_cb, vd);

	it = elm_ctxpopup_item_append(vd->ctx_popup, EMAIL_FILTER_STRING_ADD.id, NULL, _add_filter_cb, vd);
	elm_object_item_domain_text_translatable_set(it, EMAIL_FILTER_STRING_ADD.domain, EINA_TRUE);

	if (vd->filter_rule_count) {
		it = elm_ctxpopup_item_append(vd->ctx_popup, EMAIL_FILTER_STRING_REMOVE.id, NULL, _delete_filter_cb, vd);
		elm_object_item_domain_text_translatable_set(it, EMAIL_FILTER_STRING_REMOVE.domain, EINA_TRUE);
	}

	elm_ctxpopup_direction_priority_set(vd->ctx_popup, ELM_CTXPOPUP_DIRECTION_UP,
			ELM_CTXPOPUP_DIRECTION_UNKNOWN, ELM_CTXPOPUP_DIRECTION_UNKNOWN, ELM_CTXPOPUP_DIRECTION_UNKNOWN);

	_move_more_ctxpopup(vd->ctx_popup, vd->base.module->win);

	evas_object_show(vd->ctx_popup);
}

static void _ctxpopup_dismissed_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailFilterVD *vd = data;

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
