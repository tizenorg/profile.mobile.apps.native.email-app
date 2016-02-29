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

#include <notification.h>
#include "email-filter-delete-view.h"

typedef struct _EmailFilterDeleteView EmailFilterView;

/* Internal functions */
static int _create(email_view_t *self);
static void _destroy(email_view_t *self);
static void _update(email_view_t *self, int flags);
static void _on_back_key(email_view_t *self);

static void _update_list(EmailFilterView *view);
static void _refresh_list(EmailFilterView *view);

static void _gl_sel_cb(void *data, Evas_Object *obj, void *event_info);
static char *_gl_text_get_cb0(void *data, Evas_Object *obj, const char *part);
static char *_gl_text_get_cb(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_gl_content_get_cb0(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_gl_content_get_cb(void *data, Evas_Object *obj, const char *part);
static char *_gl_text_get_cb2(void *data, Evas_Object *obj, const char *part);
static void _delete_filter_cb(void *data, Evas_Object *obj, void *event_info);
static void _select_all_check_changed_cb(void *data, Evas_Object *obj, void *event_info);
static Evas_Object *_create_rule_list_layout(EmailFilterView *view, email_rule_t *filter_rule_list, int filter_count);
static Evas_Object *_create_no_content_layout(EmailFilterView *view);
static int _get_filter_rule_list(EmailFilterView *view, email_rule_t **filter_rule_list, int *filter_rule_count);
static Eina_Bool _is_checked_item(EmailFilterView *view);
static void _check_changed_cb(void *data, Evas_Object *obj, void *event_info);
static void _show_selected_item_count(EmailFilterView *view);
static void _cancel_cb(void *data, Evas_Object *obj, void *event_info);

static email_string_t EMAIL_FILTER_STRING_NO_PRIORITY_SENDERS = {PACKAGE, "IDS_EMAIL_NPBODY_NO_PRIORITY_SENDERS"};
static email_string_t EMAIL_FILTER_STRING_REMOVED_ONE_PRIORITY_SENDERS = {PACKAGE, "IDS_EMAIL_TPOP_1_EMAIL_ADDRESS_REMOVED_FROM_PRIORITY_SENDERS"};
static email_string_t EMAIL_FILTER_STRING_REMOVED_MORE_PRIORITY_SENDERS = {PACKAGE, "IDS_EMAIL_TPOP_PD_EMAIL_ADDRESSES_REMOVED_FROM_PRIORITY_SENDERS"};
static email_string_t EMAIL_FILTER_STRING_SELECT_ALL = {PACKAGE, "IDS_EMAIL_HEADER_SELECT_ALL_ABB"};
static email_string_t EMAIL_FILTER_STRING_SELECTED = {PACKAGE, "IDS_EMAIL_HEADER_PD_SELECTED_ABB2"};
static email_string_t EMAIL_FILTER_STRING_DONE_TITLE_BTN = {PACKAGE, "IDS_TPLATFORM_ACBUTTON_DONE_ABB"};
static email_string_t EMAIL_FILTER_STRING_CANCEL_TITLE_BTN = {PACKAGE, "IDS_TPLATFORM_ACBUTTON_CANCEL_ABB"};

/* Internal structure */
struct _EmailFilterDeleteView {
	email_view_t base;

	Evas_Object *content_layout;
	Evas_Object *genlist;
	Evas_Object *done_btn;
	Evas_Object *all_select_btn;
	GSList *list_items;
	email_rule_t *filter_rule_list;

	int filter_rule_count;

	Elm_Genlist_Item_Class *itc0;
	Elm_Genlist_Item_Class *itc1;
	Elm_Genlist_Item_Class *itc2;
};

typedef struct _ListItemData {
	int index;
	Evas_Object *check;
	Elm_Object_Item *it;
	EmailFilterView *view;
	email_rule_t *filter_rule;
	Eina_Bool is_delete;
	char *contact_name;
} ListItemData;

void create_filter_delete_view(EmailFilterModule *module)
{
	debug_enter();
	retm_if(!module, "module is null");

	EmailFilterView *view = calloc(1, sizeof(EmailFilterView));
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

	EmailFilterView *view = (EmailFilterView *)self;

	Evas_Object *done_btn = NULL;
	Evas_Object *cancel_btn = NULL;
	Elm_Object_Item *navi_it = NULL;
	char str[255] = {0, };

	Evas_Object *layout = elm_layout_add(view->base.module->navi);
	elm_layout_theme_set(layout, "layout", "application", "default");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	view->base.content = layout;

	snprintf(str, sizeof(str), dgettext(EMAIL_FILTER_STRING_SELECTED.domain, EMAIL_FILTER_STRING_SELECTED.id), 0);
	navi_it = email_module_view_push(&view->base, str, 0);

	cancel_btn = elm_button_add(view->base.module->navi);
	elm_object_style_set(cancel_btn, "naviframe/title_left");
	elm_object_domain_translatable_text_set(cancel_btn, EMAIL_FILTER_STRING_CANCEL_TITLE_BTN.domain, EMAIL_FILTER_STRING_CANCEL_TITLE_BTN.id);
	evas_object_smart_callback_add(cancel_btn, "clicked", _cancel_cb, view);
	elm_object_item_part_content_set(navi_it, "title_left_btn", cancel_btn);

	view->done_btn = done_btn = elm_button_add(view->base.module->navi);
	elm_object_style_set(done_btn, "naviframe/title_right");
	elm_object_domain_translatable_text_set(done_btn, EMAIL_FILTER_STRING_DONE_TITLE_BTN.domain, EMAIL_FILTER_STRING_DONE_TITLE_BTN.id);
	evas_object_smart_callback_add(done_btn, "clicked", _delete_filter_cb, view);
	elm_object_item_part_content_set(navi_it, "title_right_btn", done_btn);
	elm_object_disabled_set(done_btn, EINA_TRUE);

	_update_list(view);

	return 0;
}

static void _update(email_view_t *self, int flags)
{
	debug_enter();
	retm_if(!self, "self is NULL");

	EmailFilterView *view = (EmailFilterView *)self;

	if (flags & EVUF_POPPING) {
		_update_list(view);
		return;
	}

	if (flags & EVUF_WAS_PAUSED) {
		_refresh_list(view);
	}

	if (flags & EVUF_LANGUAGE_CHANGED) {
		elm_genlist_realized_items_update(view->genlist);
	}
}

static void _update_list(EmailFilterView *view)
{
	debug_enter();

	int ret = 0;

	GSList *l = view->list_items;
	while (l) {
		ListItemData *li = l->data;
		FREE(li->contact_name);
		FREE(li);
		l = g_slist_next(l);
	}

	if (view->list_items) {
		g_slist_free(view->list_items);
		view->list_items = NULL;
	}

	if (view->genlist) {
		elm_genlist_clear(view->genlist);
		DELETE_EVAS_OBJECT(view->genlist);
	}

	if (view->content_layout) {
		elm_layout_content_unset(view->base.content, "elm.swallow.content");
		DELETE_EVAS_OBJECT(view->content_layout);
	}

	if (view->filter_rule_list) {
		email_free_rule(&view->filter_rule_list, view->filter_rule_count);
	}
	ret = _get_filter_rule_list(view, &view->filter_rule_list, &view->filter_rule_count);

	if (ret < 0)
		debug_warning("_get_filter_rule_list failed");

	if (view->filter_rule_count > 0) {
		view->content_layout = _create_rule_list_layout(view, view->filter_rule_list, view->filter_rule_count);
	} else {
		view->content_layout = _create_no_content_layout(view);
	}

	elm_layout_content_set(view->base.content, "elm.swallow.content", view->content_layout);
}

static void _destroy(email_view_t *self)
{
	debug_enter();
	retm_if(!self, "self is NULL");

	EmailFilterView *view = (EmailFilterView *)self;

	EMAIL_GENLIST_ITC_FREE(view->itc1);
	EMAIL_GENLIST_ITC_FREE(view->itc2);

	GSList *l = view->list_items;
	while (l) {
		ListItemData *li = l->data;
		FREE(li->contact_name);
		FREE(li);
		l = g_slist_next(l);
	}

	if (view->list_items) {
		g_slist_free(view->list_items);
		view->list_items = NULL;
	}
	if (view->filter_rule_list) {
		email_free_rule(&view->filter_rule_list, view->filter_rule_count);
	}

	if (view->content_layout) {
		elm_layout_content_unset(view->base.content, "elm.swallow.content");
		DELETE_EVAS_OBJECT(view->content_layout);
	}

	free(view);
}

static void _refresh_list(EmailFilterView *view)
{
	debug_enter();

	retm_if(!view, "viewdata is NULL");

	EmailFilterModule *module = (EmailFilterModule *)view->base.module;
	if (module->op_type == EMAIL_FILTER_OPERATION_PRIORITY_SERNDER) {
		GSList *l = view->list_items;
		while (l) {
			ListItemData *li = l->data;
			if (li->index >= 0 && STR_VALID(li->filter_rule->value2)) {
				char *contact_name = NULL;
				int ret = email_contacts_get_contact_name_by_email_address(li->filter_rule->value2, &contact_name);
				FREE(li->contact_name);
				if (ret == CONTACTS_ERROR_NONE) {
					li->contact_name = contact_name;
				} else {
					li->contact_name = NULL;
				}

				elm_genlist_item_item_class_update(li->it, li->contact_name ? view->itc2 : view->itc1);
			}
			l = g_slist_next(l);
		}
	}
}

static void _delete_filter_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailFilterView *view = data;
	EmailFilterModule *module = (EmailFilterModule *)view->base.module;
	int ret = 0;
	int count = 0;

	GSList *l = view->list_items;
	while (l) {
		ListItemData *li = l->data;
		if (li->index >= 0 && li->is_delete == EINA_TRUE) {
			email_rule_t *filter_rule = li->filter_rule;
			ret = email_delete_rule(filter_rule->filter_id);
			if (ret != EMAIL_ERROR_NONE)
				debug_warning("email_delete_rule failed: %d", ret);
			else {
				debug_log("email_delete_rule success");
				count++;
			}
		}
		l = g_slist_next(l);
	}

	if (count > 1) {
		char str[255] = {0, };
		snprintf(str, sizeof(str), dgettext(EMAIL_FILTER_STRING_REMOVED_MORE_PRIORITY_SENDERS.domain, EMAIL_FILTER_STRING_REMOVED_MORE_PRIORITY_SENDERS.id), count);
		email_string_t EMAIL_FILTER_STRING_REMOVED_PRIORITY_SENDERS = {PACKAGE, str};
		ret = notification_status_message_post(email_get_email_string(EMAIL_FILTER_STRING_REMOVED_PRIORITY_SENDERS.id));
	} else {
		ret = notification_status_message_post(email_get_email_string(EMAIL_FILTER_STRING_REMOVED_ONE_PRIORITY_SENDERS.id));
	}

	if (ret != NOTIFICATION_ERROR_NONE) {
		debug_log("fail to notification_status_message_post() : %d\n", ret);
	}

	email_filter_publish_changed_noti(module);
	email_module_exit_view(&view->base);
}

static void _on_back_key(email_view_t *self)
{
	debug_enter();
	retm_if(!self, "self is NULL");

	email_module_exit_view(self);
}

static void _cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailFilterView *view = data;

	email_module_exit_view(&view->base);
}


static Evas_Object *_create_rule_list_layout(EmailFilterView *view, email_rule_t *filter_rule_list, int filter_count)
{
	debug_enter();
	EmailFilterModule *module = (EmailFilterModule *)view->base.module;
	Evas_Object *content_ly = NULL;
	Evas_Object *genlist = NULL;
	ListItemData *li = NULL;
	int i = 0;

	content_ly = elm_layout_add(view->base.content);
	elm_layout_theme_set(content_ly, "layout", "application", "default");
	evas_object_size_hint_weight_set(content_ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	genlist = elm_genlist_add(content_ly);
	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
	elm_genlist_homogeneous_set(genlist, EINA_TRUE);
	elm_scroller_policy_set(genlist, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
	view->genlist = genlist;

	view->itc0 = email_util_get_genlist_item_class("group_index", _gl_text_get_cb0, _gl_content_get_cb0, NULL, NULL);
	view->itc1 = email_util_get_genlist_item_class("type1", _gl_text_get_cb, _gl_content_get_cb, NULL, NULL);
	view->itc2 = email_util_get_genlist_item_class("type1", _gl_text_get_cb2, _gl_content_get_cb, NULL, NULL);

	/* all select */
	li = calloc(1, sizeof(ListItemData));
	retvm_if(!li, NULL, "memory allocation failed");

	li->index = -1;
	li->view = view;
	li->it = elm_genlist_item_append(genlist, view->itc0, li,
			NULL, ELM_GENLIST_ITEM_NONE, _gl_sel_cb, li);
	elm_genlist_item_select_mode_set(li->it, ELM_OBJECT_SELECT_MODE_ALWAYS);
	view->list_items = g_slist_append(view->list_items, li);

	for (i = 0; i < filter_count; i++) {
		li = calloc(1, sizeof(ListItemData));
		retvm_if(!li, NULL, "memory allocation failed");

		li->index = i;
		li->view = view;
		li->filter_rule = filter_rule_list + i;
		if (module->op_type == EMAIL_FILTER_OPERATION_PRIORITY_SERNDER &&
				STR_VALID(li->filter_rule->value2)) {
			char *contact_name = NULL;
			int ret = email_contacts_get_contact_name_by_email_address(li->filter_rule->value2, &contact_name);
			FREE(li->contact_name);
			if (ret == CONTACTS_ERROR_NONE) {
				li->contact_name = contact_name;
			} else {
				li->contact_name = NULL;
			}
		}
		li->it = elm_genlist_item_append(genlist,
				li->contact_name ? view->itc2 : view->itc1, li, NULL,
				ELM_GENLIST_ITEM_NONE, _gl_sel_cb, li);
		elm_genlist_item_select_mode_set(li->it, ELM_OBJECT_SELECT_MODE_ALWAYS);
		view->list_items = g_slist_append(view->list_items, li);
	}

	elm_object_part_content_set(content_ly, "elm.swallow.content", genlist);

	return content_ly;
}

static Evas_Object *_create_no_content_layout(EmailFilterView *view)
{
	debug_enter();
	Evas_Object *no_ly = NULL;

	no_ly = elm_layout_add(view->base.module->navi);
	elm_layout_theme_set(no_ly, "layout", "nocontents", "text");

	elm_object_domain_translatable_part_text_set(no_ly, "elm.text", EMAIL_FILTER_STRING_NO_PRIORITY_SENDERS.domain, EMAIL_FILTER_STRING_NO_PRIORITY_SENDERS.id);

	return no_ly;
}

static int _get_filter_rule_list(EmailFilterView *view, email_rule_t **filter_rule_list, int *filter_rule_count)
{
	debug_enter();
	email_rule_t *rule_list = NULL;
	int count;
	int ret = 0;
	int i = 0;
	int ret_count = 0;

	ret = email_get_rule_list(&rule_list, &count);
	retvm_if(ret != EMAIL_ERROR_NONE, -1, "email_get_rule_list failed: %d", ret);

	/* get count */
	for (i = 0; i < count; i++) {
		if (rule_list[i].type == EMAIL_PRIORITY_SENDER) {
			ret_count++;
		}
	}

	/* get filter rule list */
	*filter_rule_list = NULL;
	*filter_rule_list = calloc(ret_count, sizeof(email_rule_t));
	ret_count = 0;
	for (i = 0; i < count; i++) {
		if (rule_list[i].type == EMAIL_PRIORITY_SENDER) {
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

static char *_gl_text_get_cb0(void *data, Evas_Object *obj, const char *part)
{
	ListItemData *li = data;
	if (li && li->index < 0 && !strcmp(part, "elm.text")) {
		return strdup(dgettext(EMAIL_FILTER_STRING_SELECT_ALL.domain, EMAIL_FILTER_STRING_SELECT_ALL.id));
	}
	return NULL;
}

static char *_gl_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	ListItemData *li = data;
	if (li) {
		email_rule_t *filter_rule = li->filter_rule;
		if (!strcmp(part, "elm.text")) {
			if (filter_rule->value2) {
				return strdup(filter_rule->value2);
			}

		}
	}
	return NULL;
}

static Evas_Object *_gl_content_get_cb0(void *data, Evas_Object *obj, const char *part)
{
	ListItemData *li = data;

	if (!li)
		return NULL;
	if (!strcmp(part, "elm.swallow.end")) {
		Evas_Object *check = elm_check_add(obj);
		evas_object_propagate_events_set(check, EINA_FALSE);

		EmailFilterView *view = li->view;
		elm_check_state_set(check, li->is_delete);
		view->all_select_btn = check;
		evas_object_smart_callback_add(check, "changed", _select_all_check_changed_cb, view);
		li->check = check;
		return check;
	}
	return NULL;

}

static Evas_Object *_gl_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
	ListItemData *li = data;

	if (!li)
		return NULL;

	if (!strcmp(part, "elm.swallow.end")) {
		Evas_Object *check = elm_check_add(obj);
		evas_object_propagate_events_set(check, EINA_FALSE);
		elm_check_state_set(check, li->is_delete);
		evas_object_smart_callback_add(check, "changed", _check_changed_cb, li);
		li->check = check;
		return check;
	}
	return NULL;
}

static char *_gl_text_get_cb2(void *data, Evas_Object *obj, const char *part)
{
	ListItemData *li = data;
	if (li) {
		email_rule_t *filter_rule = li->filter_rule;
		if (!strcmp(part, "elm.text")) {
			if (li->contact_name) {
				return strdup(li->contact_name);
			}
		} else if (!strcmp(part, "elm.text.sub") && filter_rule) {
			if (filter_rule->value2) {
				return strdup(filter_rule->value2);
			}
		}
	}
	return NULL;
}

static void _gl_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	ListItemData *li = data;
	Elm_Object_Item *item = event_info;

	elm_genlist_item_selected_set(item, EINA_FALSE);

	if (li->index < 0)
		_select_all_check_changed_cb(li->view, li->check, NULL);
	else
		_check_changed_cb(li, li->check, NULL);
}

static void _select_all_check_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailFilterView *view = data;
	Eina_Bool is_all_to_be_checked = EINA_FALSE;

	/* Updating selectAll check box state */
	GSList *l = view->list_items;
	while (l) {
		ListItemData *li = l->data;
		if (li->index < 0) {
			li->is_delete = !li->is_delete;
			is_all_to_be_checked = li->is_delete;
			elm_check_state_set(li->check, li->is_delete);
			break;
		}
		l = g_slist_next(l);
	}

	/* Updating state of all check boxes based on selectAll check box */
	l = view->list_items;
	while (l) {
		ListItemData *li = l->data;
		if (li->index >= 0) {
			li->is_delete = is_all_to_be_checked;
			elm_check_state_set(li->check, li->is_delete);
		}
		l = g_slist_next(l);
	}

	elm_object_disabled_set(view->done_btn, !is_all_to_be_checked);
	elm_genlist_realized_items_update(view->genlist);
	_show_selected_item_count(view);
}

static Eina_Bool _is_checked_item(EmailFilterView *view)
{
	debug_enter();
	Eina_Bool ret = EINA_FALSE;
	GSList *l = view->list_items;

	while (l) {
		ListItemData *li = l->data;
		if (li->index >= 0 && li->is_delete == EINA_TRUE) {
			ret = EINA_TRUE;
			break;
		}
		l = g_slist_next(l);
	}

	return ret;
}

static void _check_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	ListItemData *li = data;
	if (li) {
		EmailFilterView *view = li->view;
		li->is_delete = !li->is_delete;
		elm_check_state_set(li->check, li->is_delete);

		/* Checking whether any check box unchecked */
		GSList *l = view->list_items;
		Eina_Bool is_selectAll_to_be_checked = EINA_TRUE;
		while (l) {
			ListItemData *li_item = l->data;
			if (li_item->index >= 0) {
				if (!li_item->is_delete) {
					is_selectAll_to_be_checked = EINA_FALSE;
					break;
				}
			}
			l = g_slist_next(l);
		}

		/* Updating selectAll check box state if required */
		l = view->list_items;
		while (l) {
			li = l->data;
			if (li->index < 0) {
				li->is_delete = is_selectAll_to_be_checked;
				elm_check_state_set(li->check, li->is_delete);
				break;
			}
			l = g_slist_next(l);
		}

		elm_object_disabled_set(view->done_btn, !_is_checked_item(view));
		_show_selected_item_count(view);
	}
}

static void _show_selected_item_count(EmailFilterView *view)
{
	debug_enter();
	GSList *l = view->list_items;
	int count = 0;

	while (l) {
		ListItemData *li = l->data;
		if (li->index >= 0 && li->is_delete == EINA_TRUE) {
			count++;
		}
		l = g_slist_next(l);
	}

	char str[255] = {0, };
	snprintf(str, sizeof(str), dgettext(EMAIL_FILTER_STRING_SELECTED.domain, EMAIL_FILTER_STRING_SELECTED.id), count);
	elm_object_item_part_text_set(view->base.navi_item, "elm.text.title", str);
}
