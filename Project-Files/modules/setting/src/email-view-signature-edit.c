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

typedef struct view_data EmailSettingVD;

static int _create(email_view_t *self);
static void _destroy(email_view_t *self);
static void _on_back_cb(email_view_t *self);

static void _push_naviframe(EmailSettingVD *vd);
static void _create_view_content(EmailSettingVD *vd);
static void _update_account_info(EmailSettingVD *vd);

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

void create_signature_edit_view(EmailSettingUGD *ugd)
{
	debug_enter();
	retm_if(!ugd, "ug data is null");

	EmailSettingVD *vd = (EmailSettingVD *)calloc(1, sizeof(EmailSettingVD));
	retm_if(!vd, "view data is null");

	vd->base.create = _create;
	vd->base.destroy = _destroy;
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
	Evas_Object *bg = elm_bg_add(vd->base.content);
	evas_object_color_set(bg, COLOR_WHITE);
	elm_object_part_content_set(vd->base.content, "elm.swallow.bg", bg);
	_push_naviframe(vd);

	_create_view_content(vd);

	DELETE_TIMER_OBJECT(vd->focus_timer);
	vd->focus_timer = ecore_timer_add(0.5, _startup_focus_cb, vd);

	return 0;
}

static void _destroy(email_view_t *self)
{
	debug_enter();
	retm_if(!self, "self is null");

	EmailSettingVD *vd = (EmailSettingVD *)self;

	DELETE_TIMER_OBJECT(vd->focus_timer);
	FREE(vd->entry_str);
	FREE(vd->entry_limit);

	free(vd);
}

static void _push_naviframe(EmailSettingVD *vd)
{
	debug_enter();
	Elm_Object_Item *navi_it = NULL;
	Evas_Object *done_btn = NULL;
	Evas_Object *cancel_btn = NULL;

	navi_it = email_module_view_push(&vd->base, EMAIL_SETTING_STRING_EDIT_SIGNATURE.id, 0);
	elm_object_item_domain_text_translatable_set(navi_it, EMAIL_SETTING_STRING_EDIT_SIGNATURE.domain, EINA_TRUE);

	cancel_btn = elm_button_add(vd->base.module->navi);
	elm_object_style_set(cancel_btn, "naviframe/title_left");
	elm_object_domain_translatable_text_set(cancel_btn, EMAIL_SETTING_STRING_CANCEL_TITLE_BTN.domain, EMAIL_SETTING_STRING_CANCEL_TITLE_BTN.id);
	evas_object_smart_callback_add(cancel_btn, "clicked", _cancel_cb, vd);
	elm_object_item_part_content_set(navi_it, "title_left_btn", cancel_btn);

	done_btn = elm_button_add(vd->base.module->navi);
	elm_object_style_set(done_btn, "naviframe/title_right");
	elm_object_domain_translatable_text_set(done_btn, EMAIL_SETTING_STRING_DONE_TITLE_BTN.domain, EMAIL_SETTING_STRING_DONE_TITLE_BTN.id);
	evas_object_smart_callback_add(done_btn, "clicked", _done_cb, vd);
	elm_object_item_part_content_set(navi_it, "title_right_btn", done_btn);

	evas_object_show(vd->base.content);
}

static void _create_view_content(EmailSettingVD *vd)
{
	debug_enter();

	retm_if(!vd, "view data is null");

	Evas_Object *scroller = elm_scroller_add(vd->base.content);
	elm_scroller_bounce_set(scroller, EINA_FALSE, EINA_TRUE);
	elm_scroller_policy_set(scroller, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
	evas_object_show(scroller);
	vd->scroller = scroller;

	Evas_Object *box = elm_box_add(vd->scroller);
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, 0);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, 0.0);
	evas_object_show(box);
	elm_object_content_set(scroller, box);
	vd->box = box;

	if (vd->account_data->options.signature)
		vd->entry_str = g_strdup(dgettext(PACKAGE, vd->account_data->options.signature));

	email_common_util_editfield_create(box, EF_MULTILINE, &vd->editfield);
	elm_object_domain_translatable_part_text_set(vd->editfield.entry, "elm.guide", EMAIL_SETTING_STRING_SIGNATURE.domain, EMAIL_SETTING_STRING_SIGNATURE.id);
	setting_set_entry_str(vd->editfield.entry, vd->entry_str);
	elm_entry_cursor_end_set(vd->editfield.entry);
	vd->entry_limit = setting_set_input_entry_limit(vd->editfield.entry, EMAIL_LIMIT_SIGNATURE_LENGTH, 0);

	elm_box_pack_end(box, vd->editfield.layout);
	evas_object_show(vd->editfield.layout);

	evas_object_smart_callback_add(vd->editfield.entry, "preedit,changed", _backup_input_cb, vd);
	evas_object_smart_callback_add(vd->editfield.entry, "changed", _backup_input_cb, vd);

	elm_object_part_content_set(vd->base.content, "elm.swallow.content", vd->scroller);
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

	EmailSettingVD *vd = data;

	/* save signature */
	notification_status_message_post(email_setting_gettext(EMAIL_SETTING_STRING_SIGNATURE_SAVED));
	FREE(vd->account_data->options.signature);
	vd->account_data->options.signature = g_strdup(vd->entry_str);
	debug_secure("set signature as : %s", vd->entry_str);
	_update_account_info(vd);

	email_module_exit_view(&vd->base);
}

static void _cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is null");

	EmailSettingVD *vd = data;

	email_module_exit_view(&vd->base);
}

static void _backup_input_cb(void *data, Evas_Object *obj, void *event_info)
{
	retm_if(!data, "data is null");

	EmailSettingVD *vd = data;

	FREE(vd->entry_str);
	vd->entry_str = setting_get_entry_str(obj);
}

static Eina_Bool _startup_focus_cb(void *data)
{
	debug_enter();
	retvm_if(!data, ECORE_CALLBACK_CANCEL, "data is null");

	EmailSettingVD *vd = data;

	DELETE_TIMER_OBJECT(vd->focus_timer);

	elm_object_focus_set(vd->editfield.entry, EINA_TRUE);
	elm_entry_cursor_end_set(vd->editfield.entry);

	return ECORE_CALLBACK_CANCEL;
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
