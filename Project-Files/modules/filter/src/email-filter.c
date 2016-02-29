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

#include "email-utils-contacts.h"
#include "email-filter.h"
#include "email-filter-list-view.h"
#include "email-filter-edit-view.h"
#include "email-utils.h"

EMAIL_DEFINE_GET_EDJ_PATH(email_get_filter_theme_path, "/email-filter.edj")

/* internal functions */
static void _keypad_down_cb(void *data, Evas_Object *obj, void *event_info);
static void _keypad_up_cb(void *data, Evas_Object *obj, void *event_info);
static void _entry_maxlength_reached_cb(void *data, Evas_Object *obj, void *event_info);
static void _title_show(EmailFilterModule *module);
static void _title_hide(EmailFilterModule *module);
static EmailFilterViewType _parse_option(EmailFilterModule *module, email_params_h params);

static int _filter_create(email_module_t *self, email_params_h params)
{
	debug_enter();
	EmailFilterModule *module = NULL;

	if (!self)
		return -1;

	module = (EmailFilterModule *)self;

	bindtextdomain(PACKAGE, email_get_locale_dir());

	int err = email_contacts_init_service();
	debug_warning_if(err != CONTACTS_ERROR_NONE, "email_contacts_init_service failed: %d", err);

	switch (_parse_option(module, params)) {
		case EMAIL_FILTER_VIEW_FILTER_LIST:
			create_filter_list_view(module);
			break;
		case EMAIL_FILTER_VIEW_ADD_FILTER:
			create_filter_edit_view(module, FILTER_EDIT_VIEW_MODE_ADD_NEW, 0);
			break;
		case EMAIL_FILTER_VIEW_DELETE_FILTER:
		case EMAIL_FILTER_VIEW_EDIT_FILTER:
		default:
			break;
	}

	debug_leave();
	return 0;
}

static void _filter_destroy(email_module_t *self)
{
	debug_enter();
	if (!self)
		return;

	int err = email_contacts_finalize_service();
	debug_warning_if(err != CONTACTS_ERROR_NONE, "email_contacts_finalize_service failed: %d", err);

}

static void _filter_on_event(email_module_t *self, email_module_event_e event)
{
	debug_enter();
	EmailFilterModule *module = (EmailFilterModule *)self;

	if (module->is_conformant) {
		if (module->is_keypad) {
			if (event == EM_EVENT_ROTATE_LANDSCAPE ||
					event == EM_EVENT_ROTATE_LANDSCAPE_UPSIDEDOWN) {
				_title_hide(module);
			} else if (event == EM_EVENT_ROTATE_PORTRAIT ||
					event == EM_EVENT_ROTATE_PORTRAIT_UPSIDEDOWN) {
				_title_show(module);
			}
		} else {
			_title_show(module);
		}
	}
}

static EmailFilterViewType _parse_option(EmailFilterModule *module, email_params_h params)
{
	debug_enter();
	const char *operation_mode = NULL;
	const char *filter_op = NULL;

	retvm_if(!params, EMAIL_FILTER_VIEW_INVALID, "invalid parameter!");

	if (email_params_get_str(params, EMAIL_BUNDLE_KEY_FILTER_OPERATION, &filter_op)) {
		if (!strcmp(filter_op, EMAIL_BUNDLE_VAL_FILTER_OPERATION_PS))
			module->op_type = EMAIL_FILTER_OPERATION_PRIORITY_SERNDER;
		else if (!strcmp(filter_op, EMAIL_BUNDLE_VAL_FILTER_OPERATION_BLOCK))
			module->op_type = EMAIL_FILTER_OPERATION_BLOCK;
		else
			module->op_type = EMAIL_FILTER_OPERATION_FILTER;
	} else {
		module->op_type = EMAIL_FILTER_OPERATION_FILTER;
	}

	retvm_if(!email_params_get_str(params, EMAIL_BUNDLE_KEY_FILTER_MODE, &operation_mode),
			EMAIL_FILTER_VIEW_INVALID, "invalid parameter!");

	if (!strcmp(operation_mode, EMAIL_BUNDLE_VAL_FILTER_LIST)) {
		debug_log("filter list view start");
		return EMAIL_FILTER_VIEW_FILTER_LIST;
	} else if (!strcmp(operation_mode, EMAIL_BUNDLE_VAL_FILTER_ADD)) {
		email_params_get_str_opt(params, EMAIL_BUNDLE_KEY_FILTER_ADDR, &module->param_filter_addr);
		debug_secure("filter add view start: address[%s]", module->param_filter_addr);
		return EMAIL_FILTER_VIEW_ADD_FILTER;
	}

	debug_error("invalid parameter!");
	return EMAIL_FILTER_VIEW_INVALID;
}

#ifdef SHARED_MODULES_FEATURE
EMAIL_API email_module_t *email_module_alloc()
#else
email_module_t *filter_module_alloc()
#endif
{
	debug_enter();

	EmailFilterModule *module = MEM_ALLOC(module, 1);
	if (!module) {
		return NULL;
	}

	module->base.create = _filter_create;
	module->base.destroy = _filter_destroy;
	module->base.on_event = _filter_on_event;

	debug_leave();
	return &module->base;
}

void email_filter_add_conformant_callback(EmailFilterModule *module)
{
	debug_enter();

	module->is_conformant = EINA_TRUE;

	evas_object_smart_callback_add(module->base.conform, "virtualkeypad,state,on", _keypad_up_cb, module);
	evas_object_smart_callback_add(module->base.conform, "virtualkeypad,state,off", _keypad_down_cb, module);
	evas_object_smart_callback_add(module->base.conform, "clipboard,state,on", _keypad_up_cb, module);
	evas_object_smart_callback_add(module->base.conform, "clipboard,state,off", _keypad_down_cb, module);
}

void email_filter_del_conformant_callback(EmailFilterModule *module)
{
	debug_enter();

	module->is_conformant = EINA_FALSE;

	evas_object_smart_callback_del(module->base.conform, "virtualkeypad,state,on", _keypad_up_cb);
	evas_object_smart_callback_del(module->base.conform, "virtualkeypad,state,off", _keypad_down_cb);
	evas_object_smart_callback_del(module->base.conform, "clipboard,state,on", _keypad_up_cb);
	evas_object_smart_callback_del(module->base.conform, "clipboard,state,off", _keypad_down_cb);
}

static void _title_hide(EmailFilterModule *module)
{
	debug_enter();
	Elm_Object_Item *navi_it = NULL;

	navi_it = elm_naviframe_top_item_get(module->base.navi);
	elm_naviframe_item_title_enabled_set(navi_it, EINA_FALSE, EINA_TRUE);
}

static void _title_show(EmailFilterModule *module)
{
	debug_enter();
	Elm_Object_Item *navi_it = NULL;

	navi_it = elm_naviframe_top_item_get(module->base.navi);
	elm_naviframe_item_title_enabled_set(navi_it, EINA_TRUE, EINA_TRUE);
}

static void _keypad_down_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailFilterModule *module = data;

	module->is_keypad = EINA_FALSE;

	_title_show(module);
}

static void _keypad_up_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailFilterModule *module = data;

	int rot = elm_win_rotation_get(module->base.win);

	module->is_keypad = EINA_TRUE;

	if (rot == EM_EVENT_ROTATE_LANDSCAPE ||
		rot == EM_EVENT_ROTATE_LANDSCAPE_UPSIDEDOWN) {
		_title_hide(module);
	}
}

Elm_Entry_Filter_Limit_Size *email_filter_set_input_entry_limit(email_view_t *view, Evas_Object *entry, int max_char_count, int max_byte_count)
{
	Elm_Entry_Filter_Limit_Size *limit_filter_data = NULL;

	retvm_if(!view, NULL, "view is NULL");

	limit_filter_data = calloc(1, sizeof(Elm_Entry_Filter_Limit_Size));
	retvm_if(!limit_filter_data, NULL, "memory allocation fail");

	limit_filter_data->max_char_count = max_char_count;
	limit_filter_data->max_byte_count = max_byte_count;

	elm_entry_markup_filter_append(entry, elm_entry_filter_limit_size, limit_filter_data);
	evas_object_smart_callback_add(entry, "maxlength,reached", _entry_maxlength_reached_cb, view);

	return limit_filter_data;
}

static void _entry_maxlength_reached_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	/* display warning popup */
	debug_log("entry length is max now");
	notification_status_message_post(dgettext(PACKAGE, "IDS_EMAIL_TPOP_MAXIMUM_NUMBER_OF_CHARACTERS_REACHED"));
}

void email_filter_publish_changed_noti(EmailFilterModule *module)
{
	debug_enter();

	retm_if(!module, "module is null");

	debug_log("changed: %d", module->op_type);
	if (module->op_type != EMAIL_FILTER_OPERATION_FILTER) {
		int ret = email_update_vip_rule_value();
		debug_log("rule changed noti ret: %d", ret);
	}
}

