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
#include "email-view-signature-edit.h"

typedef struct view_data EmailSettingView;

static int _create(email_view_t *self);
static void _destroy(email_view_t *self);
static void _on_back_cb(email_view_t *self);

static void _push_naviframe(EmailSettingView *view);
static void _create_view_content(EmailSettingView *view);
static void _update_account_info(EmailSettingView *view);

static void _cancel_cb(void *data, Evas_Object *obj, void *event_info);
static void _done_cb(void *data, Evas_Object *obj, void *event_info);

static void _backup_input_cb(void *data, Evas_Object *obj, void *event_info);
static Eina_Bool _startup_focus_cb(void *data);

static email_string_t EMAIL_SETTING_STRING_SIGNATURE = {PACKAGE, "IDS_EMAIL_MBODY_SIGNATURE"};
static email_string_t EMAIL_SETTING_STRING_EDIT_SIGNATURE = {PACKAGE, "IDS_EMAIL_MBODY_EDIT_SIGNATURE"};
static email_string_t EMAIL_SETTING_STRING_SIGNATURE_SAVED = {PACKAGE, "IDS_EMAIL_TPOP_SIGNATURE_SAVED"};
static email_string_t EMAIL_SETTING_STRING_DONE_TITLE_BTN = {PACKAGE, "IDS_TPLATFORM_ACBUTTON_DONE_ABB"};
static email_string_t EMAIL_SETTING_STRING_CANCEL_TITLE_BTN = {PACKAGE, "IDS_TPLATFORM_ACBUTTON_CANCEL_ABB"};

struct view_data {
	email_view_t base;

	email_account_t *account_data;

	Evas_Object *scroller;
	Evas_Object *box;
	email_editfield_t editfield;

	char *entry_str;
	Elm_Entry_Filter_Limit_Size *entry_limit;
	Ecore_Timer *focus_timer;
};

void create_signature_edit_view(EmailSettingModule *module)
{
	debug_enter();
	retm_if(!module, "module is null");

	EmailSettingView *view = (EmailSettingView *)calloc(1, sizeof(EmailSettingView));
	retm_if(!view, "view data is null");

	view->base.create = _create;
	view->base.destroy = _destroy;
	view->base.on_back_key = _on_back_cb;

	debug_log("view create result: %d", email_module_create_view(&module->base, &view->base));
}

static int _create(email_view_t *self)
{
	debug_enter();
	retvm_if(!self, -1, "self is null");

	EmailSettingView *view = (EmailSettingView *)self;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	if (!setting_get_acct_full_data(module->account_id, &(view->account_data))) {
		debug_error("setting_get_acct_full_data failed");
		return -1;
	}

	EMAIL_SETTING_PRINT_ACCOUNT_INFO(view->account_data);

	view->base.content = setting_add_inner_layout(&view->base);
	Evas_Object *bg = elm_bg_add(view->base.content);
	evas_object_color_set(bg, COLOR_WHITE);
	elm_object_part_content_set(view->base.content, "elm.swallow.bg", bg);
	_push_naviframe(view);

	_create_view_content(view);

	DELETE_TIMER_OBJECT(view->focus_timer);
	view->focus_timer = ecore_timer_add(0.5, _startup_focus_cb, view);

	return 0;
}

static void _destroy(email_view_t *self)
{
	debug_enter();
	retm_if(!self, "self is null");

	EmailSettingView *view = (EmailSettingView *)self;

	DELETE_TIMER_OBJECT(view->focus_timer);
	FREE(view->entry_str);
	FREE(view->entry_limit);

	free(view);
}

static void _push_naviframe(EmailSettingView *view)
{
	debug_enter();
	Elm_Object_Item *navi_it = NULL;
	Evas_Object *done_btn = NULL;
	Evas_Object *cancel_btn = NULL;

	navi_it = email_module_view_push(&view->base, EMAIL_SETTING_STRING_EDIT_SIGNATURE.id, 0);
	elm_object_item_domain_text_translatable_set(navi_it, EMAIL_SETTING_STRING_EDIT_SIGNATURE.domain, EINA_TRUE);

	cancel_btn = elm_button_add(view->base.module->navi);
	elm_object_style_set(cancel_btn, "naviframe/title_left");
	elm_object_domain_translatable_text_set(cancel_btn, EMAIL_SETTING_STRING_CANCEL_TITLE_BTN.domain, EMAIL_SETTING_STRING_CANCEL_TITLE_BTN.id);
	evas_object_smart_callback_add(cancel_btn, "clicked", _cancel_cb, view);
	elm_object_item_part_content_set(navi_it, "title_left_btn", cancel_btn);

	done_btn = elm_button_add(view->base.module->navi);
	elm_object_style_set(done_btn, "naviframe/title_right");
	elm_object_domain_translatable_text_set(done_btn, EMAIL_SETTING_STRING_DONE_TITLE_BTN.domain, EMAIL_SETTING_STRING_DONE_TITLE_BTN.id);
	evas_object_smart_callback_add(done_btn, "clicked", _done_cb, view);
	elm_object_item_part_content_set(navi_it, "title_right_btn", done_btn);

	evas_object_show(view->base.content);
}

static void _create_view_content(EmailSettingView *view)
{
	debug_enter();

	retm_if(!view, "view data is null");

	Evas_Object *scroller = elm_scroller_add(view->base.content);
	elm_scroller_bounce_set(scroller, EINA_FALSE, EINA_TRUE);
	elm_scroller_policy_set(scroller, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
	evas_object_show(scroller);
	view->scroller = scroller;

	Evas_Object *box = elm_box_add(view->scroller);
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, 0);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, 0.0);
	evas_object_show(box);
	elm_object_content_set(scroller, box);
	view->box = box;

	if (view->account_data->options.signature)
		view->entry_str = g_strdup(dgettext(PACKAGE, view->account_data->options.signature));

	email_common_util_editfield_create(box, EF_MULTILINE, &view->editfield);
	elm_object_domain_translatable_part_text_set(view->editfield.entry, "elm.guide", EMAIL_SETTING_STRING_SIGNATURE.domain, EMAIL_SETTING_STRING_SIGNATURE.id);
	setting_set_entry_str(view->editfield.entry, view->entry_str);
	elm_entry_cursor_end_set(view->editfield.entry);
	view->entry_limit = setting_set_input_entry_limit(view->editfield.entry, EMAIL_LIMIT_SIGNATURE_LENGTH, 0);

	elm_box_pack_end(box, view->editfield.layout);
	evas_object_show(view->editfield.layout);

	evas_object_smart_callback_add(view->editfield.entry, "preedit,changed", _backup_input_cb, view);
	evas_object_smart_callback_add(view->editfield.entry, "changed", _backup_input_cb, view);

	elm_object_part_content_set(view->base.content, "elm.swallow.content", view->scroller);
}

static void _on_back_cb(email_view_t *self)
{
	debug_enter();
	retm_if(!self, "self is null");

	email_module_exit_view(self);
}

static void _done_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is null");

	EmailSettingView *view = data;

	/* save signature */
	notification_status_message_post(email_setting_gettext(EMAIL_SETTING_STRING_SIGNATURE_SAVED));
	FREE(view->account_data->options.signature);
	view->account_data->options.signature = g_strdup(view->entry_str);
	debug_secure("set signature as : %s", view->entry_str);
	_update_account_info(view);

	email_module_exit_view(&view->base);
}

static void _cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is null");

	EmailSettingView *view = data;

	email_module_exit_view(&view->base);
}

static void _backup_input_cb(void *data, Evas_Object *obj, void *event_info)
{
	retm_if(!data, "data is null");

	EmailSettingView *view = data;

	FREE(view->entry_str);
	view->entry_str = setting_get_entry_str(obj);
}

static Eina_Bool _startup_focus_cb(void *data)
{
	debug_enter();
	retvm_if(!data, ECORE_CALLBACK_CANCEL, "data is null");

	EmailSettingView *view = data;

	DELETE_TIMER_OBJECT(view->focus_timer);

	elm_object_focus_set(view->editfield.entry, EINA_TRUE);
	elm_entry_cursor_end_set(view->editfield.entry);

	return ECORE_CALLBACK_CANCEL;
}

static void _update_account_info(EmailSettingView *view)
{
	debug_enter();
	retm_if(!view, "view data is null");
	retm_if(!(view->account_data), "account_data is null");

	if (email_engine_update_account(view->account_data->account_id, view->account_data) == TRUE)
		debug_log("Account updated successfully");
}
/* EOF */
