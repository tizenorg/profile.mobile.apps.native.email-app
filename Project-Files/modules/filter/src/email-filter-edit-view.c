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
#include "email-filter-edit-view.h"
#include "email-common-contact-defines.h"

typedef struct _EmailFilterEditVD EmailFilterVD;

/* Internal functions */
static int _create(email_view_t *self);
static void _destroy(email_view_t *self);
static void _activate(email_view_t *self, email_view_state prev_state);
static void _update(email_view_t *self, int flags);
static void _on_back_key(email_view_t *self);

static void _done_cb(void *data, Evas_Object *obj, void *event_info);
static void _cancel_cb(void *data, Evas_Object *obj, void *event_info);
static void _return_key_cb(void *data, Evas_Object *obj, void *event_info);
static void _entry_changed_cb(void *data, Evas_Object *obj, void *event_info);
static void _contact_button_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static Evas_Object *_gl_content_get_cb(void *data, Evas_Object *obj, const char *part);
static void _contact_app_reply_cb(void *data, app_control_result_e result, app_control_h reply);
static int _checking_is_dup_rule(EmailFilterVD *vd, email_rule_t *filter_rule);

static Evas_Object *_create_list(EmailFilterVD *vd);
static void _launch_contact_app(EmailFilterVD *vd);
static char *_get_filter_address_by_id(int id);
static int _get_field_validation(EmailFilterVD *vd);
static void _done_key_disabled_set(EmailFilterVD *vd, Eina_Bool disabled);

static email_filter_string_t EMAIL_FILTER_STRING_ALREADY_ADDED = {PACKAGE, "IDS_EMAIL_TPOP_EMAIL_ADDRESS_ALREADY_ADDED_TO_PRIORITY_SENDERS"};
static email_filter_string_t EMAIL_FILTER_STRING_EMAIL_ADDRESS = {PACKAGE, "IDS_EMAIL_BODY_EMAIL_ADDRESS"};
static email_filter_string_t EMAIL_SETTING_STRING_ADD_PRIORITY_SENDER = {PACKAGE, "IDS_EMAIL_OPT_ADD"};
static email_filter_string_t EMAIL_SETTING_STRING_EDIT_PRIORITY_SENDER = {PACKAGE, "IDS_EMAIL_OPT_EDIT"};
static email_filter_string_t EMAIL_FILTER_STRING_DONE_TITLE_BTN = {PACKAGE, "IDS_TPLATFORM_ACBUTTON_DONE_ABB"};
static email_filter_string_t EMAIL_FILTER_STRING_CANCEL_TITLE_BTN = {PACKAGE, "IDS_TPLATFORM_ACBUTTON_CANCEL_ABB"};

/* Internal structure */
struct _EmailFilterEditVD {
	email_view_t base;
	filter_edit_view_mode mode;

	Evas_Object *done_btn;
	Evas_Object *genlist;
	GSList *list_items;

	email_rule_t *filter_rule;
	char *original_priority_addr;

	Elm_Genlist_Item_Class *itc2;
	Elm_Genlist_Item_Class *itc3;
};

enum {
	EMAIL_FILTER_ADD_SENDER = 1,
};

typedef struct _ListItemData {
	int index;
	char *entry_str;
	email_editfield_t editfield;
	Elm_Entry_Filter_Limit_Size *entry_limit;
	Elm_Object_Item *it;
	EmailFilterVD *vd;
} ListItemData;

void create_filter_edit_view(EmailFilterUGD *ugd, filter_edit_view_mode mode, int filter_id)
{
	debug_enter();
	retm_if(!ugd, "ug data is null");

	EmailFilterVD *vd = calloc(1, sizeof(EmailFilterVD));
	retm_if(!vd, "view data is null");

	if (mode == FILTER_EDIT_VIEW_MODE_EDIT_EXISTENT) {
		email_rule_t *filter_rule = NULL;
		email_get_rule(filter_id, &filter_rule);
		retvm_if(!filter_rule, free(vd), "filter_rule to be edited is null");
		vd->filter_rule = filter_rule;
	}

	vd->mode = mode;

	vd->base.create = _create;
	vd->base.destroy = _destroy;
	vd->base.activate = _activate;
	vd->base.update = _update;
	vd->base.on_back_key = _on_back_key;

	debug_log("view create result: %d", email_module_create_view(&ugd->base, &vd->base));
}

static int _create(email_view_t *self)
{
	debug_enter();
	retvm_if(!self, -1, "self is null");

	EmailFilterVD *vd = (EmailFilterVD *)self;
	EmailFilterUGD *ugd = (EmailFilterUGD *)vd->base.module;

	Elm_Object_Item *navi_it = NULL;
	Evas_Object *layout = NULL;
	Evas_Object *genlist = NULL;
	Evas_Object *done_btn = NULL;
	Evas_Object *cancel_btn = NULL;

	layout = elm_layout_add(ugd->base.navi);
	elm_layout_theme_set(layout, "layout", "application", "default");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	vd->base.content = layout;

	vd->genlist = genlist = _create_list(vd);
	elm_object_part_content_set(layout, "elm.swallow.content", genlist);

	email_filter_string_t *title = NULL;
	if (vd->mode == FILTER_EDIT_VIEW_MODE_ADD_NEW) {
		title = &EMAIL_SETTING_STRING_ADD_PRIORITY_SENDER;
	} else {
		title = &EMAIL_SETTING_STRING_EDIT_PRIORITY_SENDER;
	}
	navi_it = email_module_view_push(&vd->base, title->id, 0);
	elm_object_item_domain_text_translatable_set(navi_it, title->domain, EINA_TRUE);

	cancel_btn = elm_button_add(ugd->base.navi);
	elm_object_style_set(cancel_btn, "naviframe/title_left");
	elm_object_domain_translatable_text_set(cancel_btn, EMAIL_FILTER_STRING_CANCEL_TITLE_BTN.domain, EMAIL_FILTER_STRING_CANCEL_TITLE_BTN.id);
	evas_object_smart_callback_add(cancel_btn, "clicked", _cancel_cb, vd);
	elm_object_item_part_content_set(navi_it, "title_left_btn", cancel_btn);

	vd->done_btn = done_btn = elm_button_add(ugd->base.navi);
	elm_object_style_set(done_btn, "naviframe/title_right");
	elm_object_domain_translatable_text_set(done_btn, EMAIL_FILTER_STRING_DONE_TITLE_BTN.domain, EMAIL_FILTER_STRING_DONE_TITLE_BTN.id);
	evas_object_smart_callback_add(done_btn, "clicked", _done_cb, vd);
	elm_object_item_part_content_set(navi_it, "title_right_btn", done_btn);

	if (_get_field_validation(vd) == 0) {
		elm_object_disabled_set(done_btn, EINA_TRUE);
	}

	email_filter_add_conformant_callback(ugd);

	return 0;
}

static void _destroy(email_view_t *self)
{
	debug_enter();
	retm_if(!self, "self is NULL");

	EmailFilterVD *vd = (EmailFilterVD *)self;
	EmailFilterUGD *ugd = (EmailFilterUGD *)vd->base.module;

	email_filter_del_conformant_callback(ugd);

	EMAIL_GENLIST_ITC_FREE(vd->itc2);
	EMAIL_GENLIST_ITC_FREE(vd->itc3);

	FREE(vd->original_priority_addr);

	GSList *l = vd->list_items;
	while (l) {
		ListItemData *li = l->data;

		if (li->entry_str)
			free(li->entry_str);
		if (li->entry_limit)
			free(li->entry_limit);
		if (li->editfield.entry) {
			evas_object_smart_callback_del(li->editfield.entry, "preedit,changed", _entry_changed_cb);
			evas_object_smart_callback_del(li->editfield.entry, "changed", _entry_changed_cb);
			evas_object_smart_callback_del(li->editfield.entry, "activated", _return_key_cb);
		}
		free(l->data);
		l = g_slist_next(l);
	}
	g_slist_free(vd->list_items);

	free(vd);
}

static void _update(email_view_t *self, int flags)
{
	debug_enter();
	retm_if(!self, "self is NULL");

	EmailFilterVD *vd = (EmailFilterVD *)self;

	if (flags & EVUF_LANGUAGE_CHANGED) {
		elm_genlist_realized_items_update(vd->genlist);
	}
}

static void _add_new_rule(EmailFilterVD *vd)
{
	debug_enter();

	EmailFilterUGD *ugd = (EmailFilterUGD *)vd->base.module;
	email_rule_t *filter_rule = NULL;
	int ret = EMAIL_ERROR_NONE;
	Evas_Object *done_btn = vd->done_btn;

	evas_object_smart_callback_del(done_btn, "clicked", _done_cb);

	filter_rule = calloc(1, sizeof(email_rule_t));
	retm_if(!filter_rule, "memory allocation failed");

	filter_rule->account_id = 0;
	filter_rule->faction = EMAIL_FILTER_MOVE;
	filter_rule->target_mailbox_id = 0;
	filter_rule->flag1 = 1;
	filter_rule->flag2 = RULE_TYPE_EXACTLY;
	filter_rule->type = EMAIL_PRIORITY_SENDER;

	GSList *l = vd->list_items;
	while (l) {
		ListItemData *li = l->data;
		if (li->index == EMAIL_FILTER_ADD_SENDER) {
			filter_rule->value2 = g_strdup(li->entry_str);
		}
		l = g_slist_next(l);
	}

	if (_checking_is_dup_rule(vd, filter_rule)) {
		debug_warning("this rule already exist!");
		ret = notification_status_message_post(email_get_email_string(EMAIL_FILTER_STRING_ALREADY_ADDED.id));
		if (ret != NOTIFICATION_ERROR_NONE) {
			debug_log("fail to notification_status_message_post() : %d\n", ret);
		}
		email_free_rule(&filter_rule, 1);
		evas_object_smart_callback_add(done_btn, "clicked", _done_cb, vd);
		return;
	}

	ret = email_add_rule(filter_rule);
	if (ret != EMAIL_ERROR_NONE) {
		debug_warning("email_add_rule failed: %d", ret);
	} else {
		debug_secure("email_add_rule success: %s", filter_rule->filter_name);
		ret = email_apply_rule(filter_rule->filter_id);
		if (ret != EMAIL_ERROR_NONE) {
			debug_warning("email_apply_rule failed: %d", ret);
		} else {
			debug_log("email_apply_rule success");
			email_filter_publish_changed_noti(ugd);
		}
	}
	email_free_rule(&filter_rule, 1);

	email_module_exit_view(&vd->base);

	debug_leave();
}

static void _apply_rule_changes(EmailFilterVD *vd)
{
	debug_enter();

	EmailFilterUGD *ugd = (EmailFilterUGD *)vd->base.module;
	email_rule_t *filter_rule = vd->filter_rule;
	int ret = EMAIL_ERROR_NONE;
	Evas_Object *done_btn = vd->done_btn;

	evas_object_smart_callback_del(done_btn, "clicked", _done_cb);

	GSList *l = vd->list_items;
	while (l) {
		ListItemData *li = l->data;
		if (li->index == EMAIL_FILTER_ADD_SENDER) {
			G_FREE(filter_rule->value2);
			filter_rule->value2 = g_strdup(li->entry_str);
		}
		l = g_slist_next(l);
	}

	if (_checking_is_dup_rule(vd, filter_rule)) {
		debug_warning("this rule already exist!");
		ret = notification_status_message_post(email_get_email_string(EMAIL_FILTER_STRING_ALREADY_ADDED.id));
		if (ret != NOTIFICATION_ERROR_NONE) {
			debug_log("fail to notification_status_message_post() : %d\n", ret);
		}
		evas_object_smart_callback_add(done_btn, "clicked", _done_cb, vd);
		return;
	}

	ret = email_update_rule(filter_rule->filter_id, filter_rule);
	if (ret != EMAIL_ERROR_NONE) {
		debug_warning("email_update_rule failed: %d", ret);
	} else {
		debug_secure("email_update_rule success: %s", filter_rule->filter_name);
		ret = email_apply_rule(filter_rule->filter_id);
		if (ret != EMAIL_ERROR_NONE) {
			debug_warning("email_apply_rule failed: %d", ret);
		} else {
			debug_log("email_apply_rule success");
			email_filter_publish_changed_noti(ugd);
		}
	}

	email_module_exit_view(&vd->base);

	debug_leave();
}

static void _done_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailFilterVD *vd = data;

	if (vd->mode == FILTER_EDIT_VIEW_MODE_ADD_NEW) {
		return _add_new_rule(vd);
	} else {
		return _apply_rule_changes(vd);
	}
}

static void _cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailFilterVD *vd = data;

	email_module_exit_view(&vd->base);
}

static void _activate(email_view_t *self, email_view_state prev_state)
{
	debug_enter();

	retm_if(!self, "self is NULL");

	if (prev_state != EV_STATE_CREATED) {
		return;
	}

	EmailFilterVD *vd = (EmailFilterVD *)self;
	ListItemData *li = vd->list_items->data;
	retm_if(!li, "li is NULL");

	elm_object_focus_set(li->editfield.entry, EINA_TRUE);

	debug_leave();
}

static void _on_back_key(email_view_t *self)
{
	debug_enter();
	retm_if(!self, "self is NULL");

	email_module_exit_view(self);
}

static Evas_Object *_create_list(EmailFilterVD *vd)
{
	debug_enter();

	ListItemData *li = NULL;
	Evas_Object *genlist = NULL;
	Elm_Object_Item *git = NULL;

	genlist = elm_genlist_add(vd->base.content);
	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
	elm_scroller_policy_set(genlist, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
	elm_genlist_select_mode_set(genlist, ELM_OBJECT_SELECT_MODE_NONE);
	elm_genlist_highlight_mode_set(genlist, EINA_FALSE);

	vd->itc3 = email_util_get_genlist_item_class("full", NULL, _gl_content_get_cb, NULL, NULL);

	/* Sender */
	li = calloc(1, sizeof(ListItemData));
	retvm_if(!li, NULL, "memory allocation failed");

	li->index = EMAIL_FILTER_ADD_SENDER;
	li->vd = vd;

	if (vd->mode == FILTER_EDIT_VIEW_MODE_ADD_NEW) {
		EmailFilterUGD *ugd = (EmailFilterUGD *)vd->base.module;
		li->entry_str = g_strdup(ugd->param_filter_addr);
	} else {
		email_rule_t *filter_rule = vd->filter_rule;
		li->entry_str = g_strdup(filter_rule->value2);
		vd->original_priority_addr = g_strdup(filter_rule->value2);
	}

	li->it = git = elm_genlist_item_append(genlist, vd->itc3, li, NULL,
			ELM_GENLIST_ITEM_NONE, NULL, NULL);
	elm_genlist_item_select_mode_set(git, ELM_OBJECT_SELECT_MODE_ALWAYS);
	vd->list_items = g_slist_append(vd->list_items, li);

	return genlist;
}

static Evas_Object *_gl_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
	ListItemData *li = data;

	if (li) {
		if (!strcmp(part, "elm.swallow.content")) {
			Evas_Object *layout = elm_layout_add(obj);
			elm_layout_file_set(layout, email_get_filter_theme_path(), "email.filter.address_edit_view");

			EmailFilterVD *vd = li->vd;
			email_common_util_editfield_create(layout, 0, &li->editfield);
			elm_entry_input_panel_layout_set(li->editfield.entry, ELM_INPUT_PANEL_LAYOUT_EMAIL);

			elm_object_domain_translatable_part_text_set(li->editfield.entry, "elm.guide",
					EMAIL_FILTER_STRING_EMAIL_ADDRESS.domain, EMAIL_FILTER_STRING_EMAIL_ADDRESS.id);
			li->entry_limit = email_filter_set_input_entry_limit(&vd->base, li->editfield.entry, 0, EMAIL_LIMIT_EMAIL_ADDRESS_LENGTH);

			if (li->entry_str) {
				char *str = elm_entry_utf8_to_markup(li->entry_str);
				elm_entry_entry_set(li->editfield.entry, str);
				elm_entry_cursor_line_end_set(li->editfield.entry);
				FREE(str);
			}

			elm_entry_input_panel_return_key_type_set(li->editfield.entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
			if (_get_field_validation(vd) == 0)
				elm_entry_input_panel_return_key_disabled_set(li->editfield.entry, EINA_TRUE);

			evas_object_smart_callback_add(li->editfield.entry, "preedit,changed", _entry_changed_cb, li);
			evas_object_smart_callback_add(li->editfield.entry, "changed", _entry_changed_cb, li);
			evas_object_smart_callback_add(li->editfield.entry, "activated", _return_key_cb, li);

			elm_object_part_content_set(layout, "email.filter.editfield_ly", li->editfield.layout);

			Evas_Object *ic = elm_layout_add(layout);
			elm_layout_file_set(ic, email_get_common_theme_path(), EMAIL_IMAGE_COMPOSER_CONTACT);
			evas_object_show(ic);

			Evas_Object *ct_btn = elm_button_add(obj);
			elm_object_style_set(ct_btn, "transparent");
			elm_object_part_content_set(ct_btn, "elm.swallow.content", ic);
			evas_object_propagate_events_set(ct_btn, EINA_FALSE);
			evas_object_smart_callback_add(ct_btn, "clicked", _contact_button_clicked_cb, li->vd);
			elm_object_part_content_set(layout, "email.filter.contact_btn", ct_btn);
			return layout;
		}
	}

	return NULL;
}

static void _entry_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "data is NULL");

	ListItemData *li = data;
	EmailFilterVD *vd = li->vd;

	if (li->entry_str) {
		free(li->entry_str);
		li->entry_str = NULL;
	}

	li->entry_str = elm_entry_markup_to_utf8(elm_entry_entry_get(obj));

	if (_get_field_validation(vd) == 0) {
		_done_key_disabled_set(vd, EINA_TRUE);
	} else {
		_done_key_disabled_set(vd, EINA_FALSE);
	}

	debug_leave();
}

static void _contact_button_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailFilterVD *vd = data;

	_launch_contact_app(vd);
}

static void _launch_contact_app(EmailFilterVD *vd)
{
	debug_enter();

	app_control_h service = NULL;
	email_launched_app_listener_t listener = { 0 };
	int ret = 0;
	listener.cb_data = vd;
	listener.reply_cb = _contact_app_reply_cb;

	app_control_create(&service);
	retm_if(!service, "app_control_create failed");

	ret = app_control_set_operation(service, APP_CONTROL_OPERATION_PICK);
	if (ret != APP_CONTROL_ERROR_NONE) {
		debug_log("app_control_set_operation() failed! ret:[%d]", ret);
		goto FINISH;
	}

	ret = app_control_set_mime(service, EMAIL_CONTACT_MIME_SCHEME);
	if (ret != APP_CONTROL_ERROR_NONE) {
		debug_log("app_control_set_mime() failed! ret:[%d]", ret);
		goto FINISH;
	}

	ret = app_control_add_extra_data(service, EMAIL_CONTACT_EXT_DATA_TYPE, EMAIL_CONTACT_BUNDLE_EMAIL);
	if (ret != APP_CONTROL_ERROR_NONE) {
		debug_log("app_control_add_extra_data() failed! ret:[%d]", ret);
		goto FINISH;
	}

	ret = app_control_add_extra_data(service, EMAIL_CONTACT_EXT_DATA_SELECTION_MODE, EMAIL_CONTACT_BUNDLE_SELECT_SINGLE);
	if (ret != APP_CONTROL_ERROR_NONE) {
		debug_log("app_control_add_extra_data() failed! ret:[%d]", ret);
		goto FINISH;
	}

	email_module_launch_app(vd->base.module, EMAIL_LAUNCH_APP_AUTO, service, &listener);

FINISH:
	app_control_destroy(service);
}

static void _contact_app_reply_cb(void *data, app_control_result_e result, app_control_h reply)
{
	debug_enter();

	EmailFilterVD *vd = data;
	char *filter_addr = NULL;
	char **filter_addr_id_arr = NULL;
	int id = -1;
	int len = 0;

	int ret = app_control_get_extra_data_array(reply, EMAIL_CONTACT_EXT_DATA_SELECTED, &filter_addr_id_arr, &len);
	retm_if(ret != APP_CONTROL_ERROR_NONE, "app_control_get_extra_data_array() failed! ret:[%d]", ret);

	if (len <= 0) {
		free(filter_addr_id_arr);
		return;
	}

	id = atoi(filter_addr_id_arr[0]);
	debug_log("contact id: %d", id);

	int i = 0;
	for (; i < len; i++) {
		free(filter_addr_id_arr[i]);
	}
	free(filter_addr_id_arr);

	filter_addr = _get_filter_address_by_id(id);
	if (!filter_addr) {
		debug_error("filter address is NULL");
		return;
	}
	debug_secure("filter address: %s", filter_addr);

	GSList *l = vd->list_items;
	while (l) {
		ListItemData *li = l->data;
		if (li->index == EMAIL_FILTER_ADD_SENDER) {
			if (li->entry_str) {
				free(li->entry_str);
				li->entry_str = NULL;
			}
			li->entry_str = g_strdup(filter_addr);
			elm_entry_entry_set(li->editfield.entry, li->entry_str);
			break;
		}
		l = g_slist_next(l);
	}

	FREE(filter_addr);
}

static char *_get_filter_address_by_id(int id)
{
	debug_enter();

	int ret = -1;
	contacts_record_h record = NULL;
	char *filter_addr = NULL;

	ret = contacts_connect();
	retvm_if(ret != CONTACTS_ERROR_NONE, NULL, "contacts_connect failed :%d", ret);

	ret = contacts_db_get_record(_contacts_email._uri, id, &record);
	if (ret != CONTACTS_ERROR_NONE) {
		debug_error("contacts_db_get_record failed: %d", ret);
		goto CATCH;
	}

	ret = contacts_record_get_str(record, _contacts_email.email, &filter_addr);
	if (ret != CONTACTS_ERROR_NONE) {
		debug_error("contacts_record_get_str failed: %d", ret);
		goto CATCH;
	}

	contacts_record_destroy(record, true);
	contacts_disconnect();
	return filter_addr;
CATCH:
	contacts_record_destroy(record, true);
	contacts_disconnect();
	return NULL;
}

static int _get_field_validation(EmailFilterVD *vd)
{
	debug_enter();

	int is_valid_sender = 1;
	int is_sender = 1;

	GSList *l = vd->list_items;
	while (l) {
		ListItemData *li = l->data;
		if (li->index == EMAIL_FILTER_ADD_SENDER) {
			char *tmp = g_strstrip(g_strdup(li->entry_str));
			if (!STR_VALID(tmp))
				is_sender = 0;
			FREE(tmp);
			if (!li->entry_str || strpbrk(li->entry_str, ";,\n") || !email_get_address_validation(li->entry_str))
				is_valid_sender = 0;
		}
		l = g_slist_next(l);
	}

	return (is_sender && is_valid_sender);
}

static void _return_key_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	ListItemData *li = data;
	retm_if(!li, "ListItemData is NULL");

	EmailFilterVD *vd = li->vd;

	_done_cb(vd, NULL, NULL);
}

static void _done_key_disabled_set(EmailFilterVD *vd, Eina_Bool disabled)
{
	EmailFilterUGD *ugd = (EmailFilterUGD *)vd->base.module;
	GSList *l = vd->list_items;

	elm_object_disabled_set(vd->done_btn, disabled);
	while (l) {
		ListItemData *_li = l->data;
		if (ugd->op_type == EMAIL_FILTER_OPERATION_PRIORITY_SERNDER &&
				_li->index == EMAIL_FILTER_ADD_SENDER) {
			elm_entry_input_panel_return_key_disabled_set(_li->editfield.entry, disabled);
		}
		l = g_slist_next(l);
	}
}

static int _checking_is_dup_rule(EmailFilterVD *vd, email_rule_t *filter_rule)
{
	debug_enter();

	email_rule_t *rule_list = NULL;
	int ret = 0;
	int count;
	char *input_str = NULL;
	char *compare_str = NULL;
	int i = 0;

	retvm_if(!vd, 0, "vd is NULL");
	retvm_if(!filter_rule, 0, "filter_rule is NULL");

	input_str = filter_rule->value2;

	ret = email_get_rule_list(&rule_list, &count);
	retvm_if(ret != EMAIL_ERROR_NONE, 0, "email_get_rule_list failed: %d", ret);

	ret = 0;
	for (i = 0; i < count; i++) {
		if (rule_list[i].type == EMAIL_PRIORITY_SENDER) {
			compare_str = rule_list[i].value2;
		}
		if (!g_strcmp0(input_str, compare_str) &&
				((vd->mode == FILTER_EDIT_VIEW_MODE_EDIT_EXISTENT && g_strcmp0(vd->original_priority_addr, compare_str))
				|| vd->mode == FILTER_EDIT_VIEW_MODE_ADD_NEW)) {
			ret = 1;
			break;
		}
	}

	email_free_rule(&rule_list, count);
	return ret;
}
