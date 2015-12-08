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

typedef struct view_data EmailSettingVD;

static int _create(email_view_t *self);
static void _update(email_view_t *self, int flags);
static void _destroy(email_view_t *self);
static void _on_back_cb(email_view_t *self);

static void _update_list(EmailSettingVD *vd);

static void _push_naviframe(EmailSettingVD *vd);
static void _create_list(EmailSettingVD *vd);
static void _update_account_info(EmailSettingVD *vd);

static char *_gl_vibro_onoff_text_get_cb(void *data, Evas_Object *obj, const char *part);
static char *_gl_noti_onoff_text_get_cb(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_gl_onoff_content_get_cb(void *data, Evas_Object *obj, const char *part);
static char *_gl_ringtone_text_get_cb(void *data, Evas_Object *obj, const char *part);
static void _onoff_cb(void *data, Evas_Object *obj, void *event_info);
static void _update_notification_options_status(EmailSettingVD *vd);
static void _alert_ringtone_setup_cb(void *data, Evas_Object *obj, void *event_info);
static void _ringtone_app_reply_cb(void *data, app_control_result_e result, app_control_h reply);

static void _gl_sel_cb(void *data, Evas_Object *obj, void *event_info);
static char *_get_alert_title(char *path);

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
	EmailSettingVD *vd;
} ListItemData;

#define SETTING_DEFAULT_ALERT_PATH		"/opt/usr/share/settings/Alerts"
#define MYFILE_DEFAULT_RINGTON_VALUE	"default"
#define MYFILE_SILENT_RINGTON_VALUE		"silent"

void create_notification_setting_view(EmailSettingUGD *ugd)
{
	debug_enter();
	retm_if(!ugd, "ug data is null");

	EmailSettingVD *vd = calloc(1, sizeof(EmailSettingVD));
	retm_if(!vd, "view data is null");

	vd->base.create = _create;
	vd->base.update = _update;
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
	retm_if(!vd, "vd is null");

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

	free(vd);
}

static void _push_naviframe(EmailSettingVD *vd)
{
	debug_enter();
	Elm_Object_Item *navi_it = NULL;

	navi_it = email_module_view_push(&vd->base, EMAIL_SETTING_STRING_NOTIFICATION_SETTINGS.id, 0);
	elm_object_item_domain_text_translatable_set(navi_it, EMAIL_SETTING_STRING_NOTIFICATION_SETTINGS.domain, EINA_TRUE);

	evas_object_show(vd->base.content);
}

static void _create_list(EmailSettingVD *vd)
{
	debug_enter();

	Evas_Object *genlist = NULL;
	ListItemData *li = NULL;

	genlist = elm_genlist_add(vd->base.content);
	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
	elm_scroller_policy_set(genlist, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
	vd->genlist = genlist;

	vd->itc_vibro_onoff = setting_get_genlist_class_item("type1", _gl_vibro_onoff_text_get_cb, _gl_onoff_content_get_cb, NULL, NULL);
	vd->itc_sound = setting_get_genlist_class_item("type1", _gl_ringtone_text_get_cb, NULL, NULL, NULL);
	vd->itc_noti_onoff = setting_get_genlist_class_item("multiline", _gl_noti_onoff_text_get_cb, _gl_onoff_content_get_cb, NULL, NULL);

	/*--Notification--*/
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");
	li->index = NOTIFICATION_LIST_ITEM;
	li->is_checked = vd->account_data->options.notification_status ? EINA_TRUE : EINA_FALSE;
	li->vd = vd;
	li->it = elm_genlist_item_append(vd->genlist, vd->itc_noti_onoff, li,
			NULL, ELM_GENLIST_ITEM_NONE, _gl_sel_cb, li);
	vd->list_items = g_slist_append(vd->list_items, li);

	/*--Alert ringtone--*/
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = ALERT_RINGTONE_LIST_ITEM;
	li->vd = vd;
	li->it = elm_genlist_item_append(vd->genlist, vd->itc_sound, li,
			NULL, ELM_GENLIST_ITEM_NONE, _alert_ringtone_setup_cb, li);
	elm_object_item_disabled_set(li->it, vd->account_data->options.notification_status ? EINA_FALSE : EINA_TRUE);
	vd->list_items = g_slist_append(vd->list_items, li);

	/*--vibrate--*/
	li = calloc(1, sizeof(ListItemData));
	retm_if(!li, "memory allocation failed");

	li->index = VIBRATE_LIST_ITEM;
	li->is_checked = vd->account_data->options.vibrate_status ? EINA_TRUE : EINA_FALSE;
	li->vd = vd;
	li->it = elm_genlist_item_append(vd->genlist, vd->itc_vibro_onoff, li,
			NULL, ELM_GENLIST_ITEM_NONE, _gl_sel_cb, li);
	elm_object_item_disabled_set(li->it, vd->account_data->options.notification_status ? EINA_FALSE : EINA_TRUE);
	vd->list_items = g_slist_append(vd->list_items, li);

	elm_object_part_content_set(vd->base.content, "elm.swallow.content", genlist);
	evas_object_show(genlist);
}

static void _on_back_cb(email_view_t *self)
{
	debug_enter();
	retm_if(!self, "self is NULL");

	EmailSettingVD *vd = (EmailSettingVD *)self;

	_update_account_info(vd);
	email_module_exit_view(&vd->base);
}

static void _update_notification_options_status(EmailSettingVD *vd)
{
	debug_enter();
	retm_if(!vd, "vd is NULL");

	GSList *l = vd->list_items;
	Eina_Bool general_noti = EINA_FALSE;

	/* general noti and general badge noti on&off operation */
	while (l) {
		ListItemData *li = l->data;
		if (li->index == NOTIFICATION_LIST_ITEM)
			general_noti = li->is_checked;
		l = g_slist_next(l);
	}

	l = vd->list_items;

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
	EmailSettingVD *vd = li->vd;

	Eina_Bool state = elm_check_state_get(obj);

	if (li->index == NOTIFICATION_LIST_ITEM) {
		vd->account_data->options.notification_status = state;
		debug_secure("notification status %d", state);
		_update_notification_options_status(vd);
	} else if (li->index == VIBRATE_LIST_ITEM) {
		vd->account_data->options.vibrate_status = state;
		debug_secure("vibrate status %d", state);
	}
	_update_account_info(vd);
}

static void _alert_ringtone_setup_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	ListItemData *li = data;

	EmailSettingVD *vd = li->vd;
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->base.module;

	Elm_Object_Item *it = event_info;
	elm_genlist_item_selected_set(it, EINA_FALSE);

	app_control_h service = NULL;
	app_control_create(&service);
	if (service) {
		char *pa_cur_ringtone = NULL;
		char *dir_path = NULL;
		int value = -1;

		if (li->index == ALERT_RINGTONE_LIST_ITEM) {
			value = vd->account_data->options.default_ringtone_status;
			pa_cur_ringtone = g_strdup(vd->account_data->options.alert_ringtone_path);
		}

		if (value) {
			FREE(pa_cur_ringtone);
			pa_cur_ringtone = g_strdup("default");
		} else if (!value && !STR_VALID(pa_cur_ringtone)) {
			FREE(pa_cur_ringtone);
			pa_cur_ringtone = g_strdup(MYFILE_SILENT_RINGTON_VALUE);
		}

		debug_secure("pa_cur_ringtone : %s", pa_cur_ringtone);
		dir_path = g_strdup(SETTING_DEFAULT_ALERT_PATH);
		app_control_add_extra_data(service, "marked_mode", pa_cur_ringtone);
		app_control_add_extra_data(service, "path", dir_path);
		app_control_add_extra_data(service, "select_type", "SINGLE_FILE");
		app_control_add_extra_data(service, "file_type", "SOUND");
		app_control_add_extra_data(service, "title", "IDS_EMAIL_BODY_ALERT_RINGTONE_ABB");
		app_control_add_extra_data(service, "domain", PACKAGE);
		app_control_add_extra_data(service, "default", "default show");
		app_control_add_extra_data(service, "silent", "silent show");
		app_control_add_extra_data(service, "landscape", "support landscape");

		email_launched_app_listener_t listener = { 0 };
		listener.cb_data = li;
		listener.reply_cb = _ringtone_app_reply_cb;

		email_module_launch_app(&ugd->base, EMAIL_LAUNCH_APP_RINGTONE, service, &listener);

		app_control_destroy(service);
		FREE(pa_cur_ringtone);
		FREE(dir_path);
	}
}

static void _ringtone_app_reply_cb(void *data, app_control_result_e result, app_control_h reply)
{
	debug_enter();

	if (!data)
		return;

	ListItemData *li = data;

	EmailSettingVD *vd = li->vd;

	char *ringtone_file = NULL;
	char *ringtone_path = NULL;

	app_control_get_extra_data(reply, "result", &ringtone_path);
	if (!ringtone_path) {
		debug_warning("ringtone path result is NULL");
		return;
	}

	debug_secure("ringtone_path: %s", ringtone_path);
	if (!g_strcmp0(MYFILE_DEFAULT_RINGTON_VALUE, ringtone_path)) {
		if (li->index == ALERT_RINGTONE_LIST_ITEM) {
			vd->account_data->options.default_ringtone_status = 1;
			FREE(vd->account_data->options.alert_ringtone_path);
			vd->account_data->options.alert_ringtone_path = strdup(DEFAULT_EMAIL_RINGTONE_PATH);
		}
	} else if (!g_strcmp0(MYFILE_SILENT_RINGTON_VALUE, ringtone_path)) {
		if (li->index == ALERT_RINGTONE_LIST_ITEM) {
			vd->account_data->options.default_ringtone_status = 0;
			FREE(vd->account_data->options.alert_ringtone_path);
			vd->account_data->options.alert_ringtone_path = strdup("");
		}
	} else {
		ringtone_file = _get_alert_title(ringtone_path);
		debug_secure("ringtone_file:%s", ringtone_file);

		if (li->index == ALERT_RINGTONE_LIST_ITEM) {
			vd->account_data->options.default_ringtone_status = 0;
			FREE(vd->account_data->options.alert_ringtone_path);
			vd->account_data->options.alert_ringtone_path = strdup(ringtone_path);
		}
	}

	elm_genlist_item_update(li->it);
	FREE(ringtone_path);
	FREE(ringtone_file);

	_update_account_info(vd);
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
	EmailSettingVD *vd = li->vd;

	if (!strcmp(part, "elm.text")) {
		return strdup(email_setting_gettext(EMAIL_SETTING_STRING_NOTI_SOUND));
	} else if (!strcmp(part, "elm.text.sub")) {
		char *ringtone = NULL;
		char *ringtone_file = NULL;
		int value = -1;

		if (li->index == ALERT_RINGTONE_LIST_ITEM) {
			value = vd->account_data->options.default_ringtone_status;
			ringtone = strdup(vd->account_data->options.alert_ringtone_path);
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

static char *_get_alert_title(char *path)
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

static void _update_account_info(EmailSettingVD *vd)
{
	debug_enter();

	retm_if(!vd, "view data is null");

	retm_if(!(vd->account_data), "account_data is null");

	if (email_engine_update_account(vd->account_data->account_id, vd->account_data) == TRUE)
		debug_log("Account updated successfully");
}
/* EOF */
