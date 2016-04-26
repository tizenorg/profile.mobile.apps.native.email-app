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

#include <metadata_extractor.h>
#include "email-setting-utils.h"
#include "email-view-notification.h"

typedef struct view_data EmailSettingView;

static int _create(email_view_t *self);
static void _update(email_view_t *self, int flags);
static void _destroy(email_view_t *self);
static void _on_back_cb(email_view_t *self);

static void _update_list(EmailSettingView *view);

static void _push_naviframe(EmailSettingView *view);
static void _create_list(EmailSettingView *view);
static void _update_account_info(EmailSettingView *view);

static char *_gl_vibro_onoff_text_get_cb(void *data, Evas_Object *obj, const char *part);
static char *_gl_noti_onoff_text_get_cb(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_gl_onoff_content_get_cb(void *data, Evas_Object *obj, const char *part);
static char *_gl_ringtone_text_get_cb(void *data, Evas_Object *obj, const char *part);
static void _onoff_cb(void *data, Evas_Object *obj, void *event_info);
static void _update_notification_options_status(EmailSettingView *view);
static void _alert_ringtone_setup_cb(void *data, Evas_Object *obj, void *event_info);
static void _ringtone_app_reply_cb(void *data, app_control_result_e result, email_params_h reply);

static void _gl_sel_cb(void *data, Evas_Object *obj, void *event_info);
static char *_get_alert_title(const char *path);

static email_string_t EMAIL_SETTING_STRING_EMAIL_NOTI = {PACKAGE, "IDS_EMAIL_MBODY_EMAIL_NOTIFICATIONS"};
static email_string_t EMAIL_SETTING_STRING_EMAIL_NOTI_HELP = {PACKAGE, "IDS_EMAIL_SBODY_RECEIVE_STATUS_BAR_NOTIFICATIONS_WHEN_EMAILS_ARRIVE"};
static email_string_t EMAIL_SETTING_STRING_VIBRATE = {PACKAGE, "IDS_EMAIL_MBODY_VIBRATION"};
static email_string_t EMAIL_SETTING_STRING_NOTI_SOUND = {PACKAGE, "IDS_EMAIL_MBODY_NOTIFICATION_SOUND"};
static email_string_t EMAIL_SETTING_STRING_DEFAULT_RINGTONE = {PACKAGE, "IDS_ST_OPT_DEFAULT_RINGTONE"};
static email_string_t EMAIL_SETTING_STRING_SILENT_RINGTONE = {PACKAGE, "IDS_ST_BODY_SILENT"};
static email_string_t EMAIL_SETTING_STRING_NOTIFICATION_SETTINGS = {PACKAGE, "IDS_EMAIL_HEADER_NOTIFICATIONS_ABB"};

enum {
	LIST_ITEM = 1,
	NOTIFICATION_LIST_ITEM,
	VIBRATE_LIST_ITEM,
	ALERT_RINGTONE_LIST_ITEM,
};

struct view_data {
	email_view_t base;

	GSList *list_items;
	email_account_t *account_data;
	Evas_Object *genlist;
	Evas_Object *check;
	Elm_Genlist_Item_Class *itc_vibro_onoff;
	Elm_Genlist_Item_Class *itc_sound;
	Elm_Genlist_Item_Class *itc_noti_onoff;
};

typedef struct _ListItemData {
	int index;
	Elm_Object_Item *it;
	Evas_Object *check;
	Eina_Bool is_checked;
	EmailSettingView *view;
} ListItemData;

#define SETTING_DEFAULT_ALERT_PATH		"/opt/usr/share/settings/Alerts"
#define MYFILE_DEFAULT_RINGTON_VALUE	"default"
#define MYFILE_SILENT_RINGTON_VALUE		"silent"

void create_notification_setting_view(EmailSettingModule *module)
{
	debug_enter();
	retm_if(!module, "module is null");

	EmailSettingView *view = calloc(1, sizeof(EmailSettingView));
	retm_if(!view, "view data is null");

	view->base.create = _create;
	view->base.update = _update;
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
	_push_naviframe(view);

	_update_list(view);

	return 0;
}

static void _update(email_view_t *self, int flags)
{
	debug_enter();
	retm_if(!self, "self is null");

	EmailSettingView *view = (EmailSettingView *)self;

	if (flags & EVUF_POPPING) {
		_update_list(view);
	}
}

static void _update_list(EmailSettingView *view)
{
	debug_enter();
	retm_if(!view, "view is null");

	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	if (view->genlist) {
		elm_genlist_clear(view->genlist);
		evas_object_del(view->genlist);
		view->genlist = NULL;
	}

	/* read account info again. */
	if (view->account_data) {
		email_engine_free_account_list(&view->account_data, 1);
		view->account_data = NULL;
	}

	if (!setting_get_acct_full_data(module->account_id, &view->account_data)) {
		debug_error("setting_get_acct_full_data failed");
		return;
	}

	EMAIL_SETTING_PRINT_ACCOUNT_INFO(view->account_data);

	_create_list(view);
}

static void _destroy(email_view_t *self)
{
	debug_enter();
	retm_if(!self, "self is null");

	EmailSettingView *view = (EmailSettingView *)self;

	GSList *l = view->list_items;
	while (l) {
		free(l->data);
		l = g_slist_next(l);
	}
	g_slist_free(view->list_items);

	free(view);
}

static void _push_naviframe(EmailSettingView *view)
{
	debug_enter();
	Elm_Object_Item *navi_it = NULL;

	navi_it = email_module_view_push(&view->base, EMAIL_SETTING_STRING_NOTIFICATION_SETTINGS.id, 0);
	elm_object_item_domain_text_translatable_set(navi_it, EMAIL_SETTING_STRING_NOTIFICATION_SETTINGS.domain, EINA_TRUE);

	evas_object_show(view->base.content);
}

static void _create_list(EmailSettingView *view)
{
	debug_enter();

	Evas_Object *genlist = NULL;
	ListItemData *li = NULL;

	genlist = elm_genlist_add(view->base.content);
	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
	elm_scroller_policy_set(genlist, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
	view->genlist = genlist;

	view->itc_vibro_onoff = setting_get_genlist_class_item("type1", _gl_vibro_onoff_text_get_cb, _gl_onoff_content_get_cb, NULL, NULL);
	view->itc_sound = setting_get_genlist_class_item("type1", _gl_ringtone_text_get_cb, NULL, NULL, NULL);
	view->itc_noti_onoff = setting_get_genlist_class_item("multiline", _gl_noti_onoff_text_get_cb, _gl_onoff_content_get_cb, NULL, NULL);

	/*--Notification--*/
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");
	li->index = NOTIFICATION_LIST_ITEM;
	li->is_checked = view->account_data->options.notification_status ? EINA_TRUE : EINA_FALSE;
	li->view = view;
	li->it = elm_genlist_item_append(view->genlist, view->itc_noti_onoff, li,
			NULL, ELM_GENLIST_ITEM_NONE, _gl_sel_cb, li);
	view->list_items = g_slist_append(view->list_items, li);

	/*--Alert ringtone--*/
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = ALERT_RINGTONE_LIST_ITEM;
	li->view = view;
	li->it = elm_genlist_item_append(view->genlist, view->itc_sound, li,
			NULL, ELM_GENLIST_ITEM_NONE, _alert_ringtone_setup_cb, li);
	elm_object_item_disabled_set(li->it, view->account_data->options.notification_status ? EINA_FALSE : EINA_TRUE);
	view->list_items = g_slist_append(view->list_items, li);

	/*--vibrate--*/
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = VIBRATE_LIST_ITEM;
	li->is_checked = view->account_data->options.vibrate_status ? EINA_TRUE : EINA_FALSE;
	li->view = view;
	li->it = elm_genlist_item_append(view->genlist, view->itc_vibro_onoff, li,
			NULL, ELM_GENLIST_ITEM_NONE, _gl_sel_cb, li);
	elm_object_item_disabled_set(li->it, view->account_data->options.notification_status ? EINA_FALSE : EINA_TRUE);
	view->list_items = g_slist_append(view->list_items, li);

	elm_object_part_content_set(view->base.content, "elm.swallow.content", genlist);
	evas_object_show(genlist);
}

static void _on_back_cb(email_view_t *self)
{
	debug_enter();
	retm_if(!self, "self is NULL");

	EmailSettingView *view = (EmailSettingView *)self;

	_update_account_info(view);
	email_module_exit_view(&view->base);
}

static void _update_notification_options_status(EmailSettingView *view)
{
	debug_enter();
	retm_if(!view, "view is NULL");

	GSList *l = view->list_items;
	Eina_Bool general_noti = EINA_FALSE;

	/* general noti and general badge noti on&off operation */
	while (l) {
		ListItemData *li = l->data;
		if (li->index == NOTIFICATION_LIST_ITEM)
			general_noti = li->is_checked;
		l = g_slist_next(l);
	}

	l = view->list_items;

	while (l) {
		ListItemData *li = l->data;
		if (li->index == VIBRATE_LIST_ITEM ||
				li->index == ALERT_RINGTONE_LIST_ITEM)
			elm_object_item_disabled_set(li->it, !general_noti);
		l = g_slist_next(l);
	}
}

static void _onoff_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	ListItemData *li = data;
	EmailSettingView *view = li->view;

	Eina_Bool state = elm_check_state_get(obj);

	if (li->index == NOTIFICATION_LIST_ITEM) {
		view->account_data->options.notification_status = state;
		debug_secure("notification status %d", state);
		_update_notification_options_status(view);
	} else if (li->index == VIBRATE_LIST_ITEM) {
		view->account_data->options.vibrate_status = state;
		debug_secure("vibrate status %d", state);
	}
	_update_account_info(view);
}

static void _alert_ringtone_setup_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	ListItemData *li = data;

	EmailSettingView *view = li->view;
	EmailSettingModule *module = (EmailSettingModule *)view->base.module;

	Elm_Object_Item *it = event_info;
	elm_genlist_item_selected_set(it, EINA_FALSE);

	const char *marked_mode = MYFILE_DEFAULT_RINGTON_VALUE;
	if (!view->account_data->options.default_ringtone_status) {
		marked_mode = view->account_data->options.alert_ringtone_path;
		if (!STR_VALID(marked_mode)) {
			marked_mode = MYFILE_SILENT_RINGTON_VALUE;
		}
	}

	email_params_h params = NULL;

	if (email_params_create(&params) &&
		email_params_add_str(params, "marked_mode", marked_mode) &&
		email_params_add_str(params, "path", SETTING_DEFAULT_ALERT_PATH) &&
		email_params_add_str(params, "select_type", "SINGLE_FILE") &&
		email_params_add_str(params, "file_type", "SOUND") &&
		email_params_add_str(params, "title", "IDS_EMAIL_BODY_ALERT_RINGTONE_ABB") &&
		email_params_add_str(params, "domain", PACKAGE) &&
		email_params_add_str(params, "default", "default show") &&
		email_params_add_str(params, "silent", "silent show") &&
		email_params_add_str(params, "landscape", "support landscape")) {

		email_launched_app_listener_t listener = { 0 };
		listener.cb_data = li;
		listener.reply_cb = _ringtone_app_reply_cb;

		email_module_launch_app(&module->base, EMAIL_LAUNCH_APP_RINGTONE, params, &listener);
	}

	email_params_free(&params);
}

static void _ringtone_app_reply_cb(void *data, app_control_result_e result, email_params_h reply)
{
	debug_enter();

	if (!data)
		return;

	ListItemData *li = data;

	EmailSettingView *view = li->view;

	char *ringtone_file = NULL;
	const char *ringtone_path = NULL;

	if (!email_params_get_str(reply, "result", &ringtone_path)) {
		debug_warning("ringtone path result is NULL");
		return;
	}

	debug_secure("ringtone_path: %s", ringtone_path);
	if (!g_strcmp0(MYFILE_DEFAULT_RINGTON_VALUE, ringtone_path)) {
		if (li->index == ALERT_RINGTONE_LIST_ITEM) {
			view->account_data->options.default_ringtone_status = 1;
			FREE(view->account_data->options.alert_ringtone_path);
			view->account_data->options.alert_ringtone_path = strdup(DEFAULT_EMAIL_RINGTONE_PATH);
		}
	} else if (!g_strcmp0(MYFILE_SILENT_RINGTON_VALUE, ringtone_path)) {
		if (li->index == ALERT_RINGTONE_LIST_ITEM) {
			view->account_data->options.default_ringtone_status = 0;
			FREE(view->account_data->options.alert_ringtone_path);
			view->account_data->options.alert_ringtone_path = strdup("");
		}
	} else {
		ringtone_file = _get_alert_title(ringtone_path);
		debug_secure("ringtone_file:%s", ringtone_file);

		if (li->index == ALERT_RINGTONE_LIST_ITEM) {
			view->account_data->options.default_ringtone_status = 0;
			FREE(view->account_data->options.alert_ringtone_path);
			view->account_data->options.alert_ringtone_path = strdup(ringtone_path);
		}
	}

	elm_genlist_item_update(li->it);
	FREE(ringtone_file);

	_update_account_info(view);
}

static void _gl_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	ListItemData *li = data;

	Elm_Object_Item *item = event_info;
	elm_genlist_item_selected_set(item, EINA_FALSE);

	Eina_Bool state = elm_check_state_get(li->check);
	elm_check_state_set(li->check, !state);
	_onoff_cb(li, li->check, NULL);
}

static Evas_Object *_gl_onoff_content_get_cb(void *data, Evas_Object *obj, const char *part)
{
	ListItemData *li = data;

	if (!strcmp(part, "elm.swallow.end")) {
		Evas_Object *check = elm_check_add(obj);
		elm_object_style_set(check, "on&off");
		elm_check_state_pointer_set(check, &(li->is_checked));
		evas_object_propagate_events_set(check, EINA_FALSE);
		li->check = check;
		evas_object_smart_callback_add(check, "changed", _onoff_cb, li);
		return check;
	}

	return NULL;
}

static char *_gl_vibro_onoff_text_get_cb(void *data, Evas_Object *obj, const char *part)
{

	ListItemData *li = data;

	if (!strcmp(part, "elm.text") && (li->index == VIBRATE_LIST_ITEM)) {
		return strdup(email_setting_gettext(EMAIL_SETTING_STRING_VIBRATE));
	}

	return NULL;
}

static char *_gl_noti_onoff_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	ListItemData *li = data;
	if (li->index == NOTIFICATION_LIST_ITEM) {
		if (!strcmp(part, "elm.text")) {
			return strdup(email_setting_gettext(EMAIL_SETTING_STRING_EMAIL_NOTI));
		} else if (!strcmp(part, "elm.text.multiline")) {
			return strdup(email_setting_gettext(EMAIL_SETTING_STRING_EMAIL_NOTI_HELP));
		}
	}
	return NULL;
}

static char *_gl_ringtone_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	ListItemData *li = data;
	EmailSettingView *view = li->view;

	if (!strcmp(part, "elm.text")) {
		return strdup(email_setting_gettext(EMAIL_SETTING_STRING_NOTI_SOUND));
	} else if (!strcmp(part, "elm.text.sub")) {
		char *ringtone = NULL;
		char *ringtone_file = NULL;
		int value = -1;

		if (li->index == ALERT_RINGTONE_LIST_ITEM) {
			value = view->account_data->options.default_ringtone_status;
			ringtone = strdup(view->account_data->options.alert_ringtone_path);
		} else {
			return NULL;
		}

		if (value) {
			FREE(ringtone);
			ringtone_file = strdup(email_setting_gettext(EMAIL_SETTING_STRING_DEFAULT_RINGTONE));
		} else if (!value && !STR_VALID(ringtone)) {
			FREE(ringtone);
			ringtone_file = strdup(email_setting_gettext(EMAIL_SETTING_STRING_SILENT_RINGTONE));
		} else {
			ringtone_file = _get_alert_title(ringtone);
			debug_secure("ringtone file: %s", ringtone_file);
			FREE(ringtone);
		}
		return ringtone_file;
	}
	return NULL;
}

static char *_get_alert_title(const char *path)
{
	debug_enter();
	char *alert_title = NULL;
	int ret = METADATA_EXTRACTOR_ERROR_NONE;
	metadata_extractor_h metadata = NULL;

	ret = metadata_extractor_create(&metadata);
	gotom_if(ret != METADATA_EXTRACTOR_ERROR_NONE, CATCH, "metadata_extractor_create failed: %d", ret);

	ret = metadata_extractor_set_path(metadata, path);
	gotom_if(ret != METADATA_EXTRACTOR_ERROR_NONE, CATCH, "metadata_extractor_set_path failed: %d", ret);

	ret = metadata_extractor_get_metadata(metadata, METADATA_TITLE, &alert_title);
	gotom_if(ret != METADATA_EXTRACTOR_ERROR_NONE, CATCH, "metadata_extractor_get_metadata failed: %d", ret);

CATCH:
	if (metadata) {
		ret = metadata_extractor_destroy(metadata);
		debug_warning_if(ret != METADATA_EXTRACTOR_ERROR_NONE, "metadata_extractor_destroy() failed! ret:[%d]", ret);
	}

	if (!alert_title) {
		char *name = strrchr(path, '/');
		name = (name && (name + 1)) ? name + 1 : NULL;
		if (name == NULL) {
			return NULL;
		}
		return g_strdup(name);
	}

	return alert_title;
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
