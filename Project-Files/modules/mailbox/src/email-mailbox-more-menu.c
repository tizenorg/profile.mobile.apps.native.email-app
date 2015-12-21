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

#include <account-types.h>

#include "email-mailbox.h"
#include "email-mailbox-toolbar.h"
#include "email-mailbox-item.h"
#include "email-mailbox-list.h"
#include "email-mailbox-ug-util.h"
#include "email-mailbox-util.h"
#include "email-mailbox-more-menu.h"
#include "email-mailbox-noti-mgr.h"
#include "email-mailbox-sync.h"
#include "email-mailbox-search.h"
#include "email-mailbox-list-other-items.h"

/*
 * Declaration for static functions
 */

/* others */
static void _settings_cb(void *data, Evas_Object *obj, void *event_info);
static void _more_ctxpopup_dismissed_cb(void *data, Evas_Object *obj, void *event_info);
static void _more_ctxpopup_back_cb(void *data, Evas_Object *obj, void *event_info);
static void _move_more_ctxpopup(Evas_Object *ctxpopup, Evas_Object *win);
static void _resize_more_ctxpopup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _delete_more_ctxpopup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _more_toolbar_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static void _compose_toolbar_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static void _change_view_for_selection_mode(EmailMailboxUGD *mailbox_ugd);
static void _setup_edit_mode(EmailMailboxUGD *mailbox_ugd, mailbox_edit_type_e edit_type);

/*
 * Definitions
 */

#define MAX_LEN 100

/*
 * Structures
 */

typedef struct {
	int index;
	Evas_Object *radio;
	EmailMailboxUGD *mailbox_ugd;
} RadioItemData;


/*
 * Globals
 */

/*
 * Definition for static functions
 */

static void _setup_edit_mode(EmailMailboxUGD *mailbox_ugd, mailbox_edit_type_e edit_type)
{
	DELETE_EVAS_OBJECT(mailbox_ugd->more_ctxpopup);
	if (!mailbox_ugd->b_editmode) {
		mailbox_ugd->editmode_type = edit_type;
		mailbox_ugd->selected_mail_list = eina_list_free(mailbox_ugd->selected_mail_list);
		mailbox_ugd->selected_mail_list = NULL;
		debug_log("Enter edit mode. type:(%d)", mailbox_ugd->editmode_type);

		mailbox_ugd->b_editmode = true;
		_change_view_for_selection_mode(mailbox_ugd);
		mailbox_sync_cancel_all(mailbox_ugd);
		mailbox_toolbar_create(mailbox_ugd);
		mailbox_create_select_info(mailbox_ugd);
	} else {
		debug_warning("error status: already edit mode");
	}

}
static void _more_edit_delete_mode_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;
	_setup_edit_mode(mailbox_ugd, MAILBOX_EDIT_TYPE_DELETE);
}

static void _more_edit_move_mode_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;
	_setup_edit_mode(mailbox_ugd, MAILBOX_EDIT_TYPE_MOVE);

}

static void _more_edit_spam_mode_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;
	_setup_edit_mode(mailbox_ugd, MAILBOX_EDIT_TYPE_ADD_SPAM);
}

static void _more_edit_remove_spam_mode_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;
	_setup_edit_mode(mailbox_ugd, MAILBOX_EDIT_TYPE_REMOVE_SPAM);

}

static void _more_edit_read_mode_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;
	_setup_edit_mode(mailbox_ugd, MAILBOX_EDIT_TYPE_MARK_READ);

}

static void _more_edit_unread_mode_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;
	_setup_edit_mode(mailbox_ugd, MAILBOX_EDIT_TYPE_MARK_UNREAD);

}

static void _settings_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;
	app_control_h service;

	DELETE_EVAS_OBJECT(mailbox_ugd->more_ctxpopup);

	if (APP_CONTROL_ERROR_NONE != app_control_create(&service)) {
		debug_log("creating service handle failed");
		return;
	}

	app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_VIEW_TYPE, EMAIL_BUNDLE_VAL_VIEW_SETTING);
	mailbox_ugd->setting = mailbox_setting_module_create(mailbox_ugd, EMAIL_MODULE_SETTING, service);
	app_control_destroy(service);
}

static void _more_ctxpopup_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	if (obj) {
		elm_ctxpopup_dismiss(obj);
	}

	debug_leave();
}

static void _more_ctxpopup_dismissed_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;
	DELETE_EVAS_OBJECT(mailbox_ugd->more_ctxpopup);
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

static void _resize_more_ctxpopup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;
	_move_more_ctxpopup(mailbox_ugd->more_ctxpopup, mailbox_ugd->base.module->win);
}

static void _delete_more_ctxpopup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;

	evas_object_event_callback_del(mailbox_ugd->base.module->navi, EVAS_CALLBACK_RESIZE, _resize_more_ctxpopup_cb);
	eext_object_event_callback_del(mailbox_ugd->more_ctxpopup, EEXT_CALLBACK_BACK, _more_ctxpopup_back_cb);
	eext_object_event_callback_del(mailbox_ugd->more_ctxpopup, EEXT_CALLBACK_MORE, _more_ctxpopup_back_cb);
	evas_object_smart_callback_del(mailbox_ugd->more_ctxpopup, "dismissed", _more_ctxpopup_dismissed_cb);
	evas_object_event_callback_del(mailbox_ugd->more_ctxpopup, EVAS_CALLBACK_DEL, _delete_more_ctxpopup_cb);
}

void _sync_toolbar_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;

	DELETE_EVAS_OBJECT(mailbox_ugd->more_ctxpopup);

	mailbox_sync_current_mailbox(mailbox_ugd);
}

static void _compose_toolbar_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;
	RETURN_IF_FAIL(mailbox_ugd != NULL);

	retm_if(mailbox_ugd->is_module_launching, "is_module_launching is true;");
	mailbox_ugd->is_module_launching = true;

	int account_id = 0;

	DELETE_EVAS_OBJECT(mailbox_ugd->more_ctxpopup);
	DELETE_EVAS_OBJECT(mailbox_ugd->error_popup);

	if (mailbox_ugd->account_count == 0) {
		mailbox_create_timeout_popup(mailbox_ugd, _("IDS_ST_HEADER_WARNING"),
				_("IDS_EMAIL_POP_YOU_HAVE_NOT_YET_CREATED_AN_EMAIL_ACCOUNT_CREATE_AN_EMAIL_ACCOUNT_AND_TRY_AGAIN"), 3.0);
		mailbox_ugd->is_module_launching = false;
		return;
	}

	if (mailbox_ugd->composer) {
		debug_log("Composer UG is already launched");
		mailbox_ugd->is_module_launching = false;
		return;
	}

	mailbox_sync_cancel_all(mailbox_ugd);

	gint composer_type = RUN_COMPOSER_NEW;
	account_id = mailbox_ugd->account_id;
	if (account_id == 0) {	/* If user execute all emails view, we will use default account id */
		int default_account_id = 0;
		if (!email_engine_get_default_account(&default_account_id)) {
			debug_log("email_engine_get_default_account failed");
		} else {
			account_id = default_account_id;
		}
	}

	if (account_id <= 0) {
		debug_log("No account");
		mailbox_ugd->is_module_launching = false;
		return;
	}

	debug_log("composer type: %d", composer_type);
	debug_log("account id: %d", account_id);

	char s_composer_type[14] = { 0, };
	char s_account_id[14] = { 0, };
	snprintf(s_composer_type, sizeof(s_composer_type), "%d", composer_type);
	snprintf(s_account_id, sizeof(s_account_id), "%d", account_id);

	app_control_h service;
	if (APP_CONTROL_ERROR_NONE != app_control_create(&service)) {
		debug_log("creating service handle failed");
		mailbox_ugd->is_module_launching = false;
		return;
	}

	app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_RUN_TYPE, s_composer_type);
	app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_ACCOUNT_ID, s_account_id);
	mailbox_ugd->composer = mailbox_composer_module_create(mailbox_ugd, EMAIL_MODULE_COMPOSER, service);

	app_control_destroy(service);
	debug_leave();
}

static Elm_Object_Item *_add_ctx_menu_item(EmailMailboxUGD *mailbox_ugd, const char *str, Evas_Object *icon, Evas_Smart_Cb cb)
{
	Elm_Object_Item *ctx_menu_item = NULL;

	ctx_menu_item = elm_ctxpopup_item_append(mailbox_ugd->more_ctxpopup, str, icon, cb, mailbox_ugd);
	elm_object_item_domain_text_translatable_set(ctx_menu_item, PACKAGE, EINA_TRUE);
	return ctx_menu_item;
}

static void _more_toolbar_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;

	retm_if(mailbox_ugd->composer, "mailbox_ugd->composer(%p) is being shown.", mailbox_ugd->composer);
	retm_if(mailbox_ugd->viewer, "mailbox_ugd->viewer(%p) is being shown.", mailbox_ugd->viewer);
	retm_if(mailbox_ugd->account, "mailbox_ugd->account(%p) is being shown.", mailbox_ugd->account);
	debug_log("b_searchmode: %d, b_editmode: %d", mailbox_ugd->b_searchmode, mailbox_ugd->b_editmode);
	retm_if(mailbox_ugd->b_searchmode, "mailbox_ugd->b_searchmode(%d) Search mode is ON.", mailbox_ugd->b_searchmode);
	retm_if(mailbox_ugd->b_editmode, "mailbox_ugd->b_editmode(%d) Edit mode is ON.", mailbox_ugd->b_editmode);

	mailbox_ugd->more_ctxpopup = elm_ctxpopup_add(mailbox_ugd->base.module->win);

	elm_ctxpopup_auto_hide_disabled_set(mailbox_ugd->more_ctxpopup, EINA_TRUE);

	/* TODO: Should be uncommented when "more/default" style will be supported. */
//	elm_object_style_set(mailbox_ugd->more_ctxpopup, "more/default");

	eext_object_event_callback_add(mailbox_ugd->more_ctxpopup, EEXT_CALLBACK_BACK, _more_ctxpopup_back_cb, mailbox_ugd);
	eext_object_event_callback_add(mailbox_ugd->more_ctxpopup, EEXT_CALLBACK_MORE, _more_ctxpopup_back_cb, mailbox_ugd);
	evas_object_smart_callback_add(mailbox_ugd->more_ctxpopup, "dismissed", _more_ctxpopup_dismissed_cb, mailbox_ugd);

	evas_object_event_callback_add(mailbox_ugd->more_ctxpopup, EVAS_CALLBACK_DEL, _delete_more_ctxpopup_cb, mailbox_ugd);
	evas_object_event_callback_add(mailbox_ugd->base.module->navi, EVAS_CALLBACK_RESIZE, _resize_more_ctxpopup_cb, mailbox_ugd);

	_add_ctx_menu_item(mailbox_ugd, "IDS_EMAIL_OPT_REFRESH", NULL, _sync_toolbar_clicked_cb);
	_add_ctx_menu_item(mailbox_ugd, "IDS_EMAIL_OPT_SEARCH", NULL, _search_button_clicked_cb);

	if (g_list_length(mailbox_ugd->mail_list) > 0) {
		_add_ctx_menu_item(mailbox_ugd, "IDS_EMAIL_OPT_DELETE", NULL, _more_edit_delete_mode_cb);
		if (mailbox_ugd->mailbox_type == EMAIL_MAILBOX_TYPE_INBOX
				|| mailbox_ugd->mailbox_type == EMAIL_MAILBOX_TYPE_TRASH
				|| mailbox_ugd->mailbox_type == EMAIL_MAILBOX_TYPE_SPAMBOX
				|| mailbox_ugd->mailbox_type == EMAIL_MAILBOX_TYPE_USER_DEFINED) {
			_add_ctx_menu_item(mailbox_ugd, "IDS_EMAIL_OPT_MOVE", NULL, _more_edit_move_mode_cb);
			if (mailbox_ugd->mailbox_type != EMAIL_MAILBOX_TYPE_SPAMBOX) {
				_add_ctx_menu_item(mailbox_ugd, "IDS_EMAIL_OPT_MOVE_TO_SPAMBOX", NULL, _more_edit_spam_mode_cb);
			} else {
				_add_ctx_menu_item(mailbox_ugd, "IDS_EMAIL_OPT_REMOVE_FROM_SPAM_ABB", NULL, _more_edit_remove_spam_mode_cb);
			}
		}
		if (mailbox_ugd->mailbox_type != EMAIL_MAILBOX_TYPE_OUTBOX && mailbox_ugd->mailbox_type != EMAIL_MAILBOX_TYPE_DRAFT
				&& mailbox_ugd->mailbox_type != EMAIL_MAILBOX_TYPE_TRASH && mailbox_ugd->mailbox_type != EMAIL_MAILBOX_TYPE_SPAMBOX) {
			_add_ctx_menu_item(mailbox_ugd, "IDS_EMAIL_OPT_MARK_AS_READ_ABB", NULL, _more_edit_read_mode_cb);
			_add_ctx_menu_item(mailbox_ugd, "IDS_EMAIL_OPT_MARK_AS_UNREAD_ABB", NULL, _more_edit_unread_mode_cb);
		}
	}
	_add_ctx_menu_item(mailbox_ugd, "IDS_EMAIL_OPT_SETTINGS", NULL, _settings_cb);

	elm_ctxpopup_direction_priority_set(mailbox_ugd->more_ctxpopup, ELM_CTXPOPUP_DIRECTION_UP, ELM_CTXPOPUP_DIRECTION_UNKNOWN, ELM_CTXPOPUP_DIRECTION_UNKNOWN, ELM_CTXPOPUP_DIRECTION_UNKNOWN);
	_move_more_ctxpopup(mailbox_ugd->more_ctxpopup, mailbox_ugd->base.module->win);

	evas_object_show(mailbox_ugd->more_ctxpopup);
}

static void _change_view_for_selection_mode(EmailMailboxUGD *mailbox_ugd)
{
	debug_enter();
	retm_if(!mailbox_ugd, "mailbox_ugd is NULL");
	elm_genlist_realized_items_update(mailbox_ugd->gl);

	if ((mailbox_ugd->mode == EMAIL_MAILBOX_MODE_ALL
	|| mailbox_ugd->mode == EMAIL_MAILBOX_MODE_MAILBOX)
	&& mailbox_ugd->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX) {
		mailbox_send_all_btn_remove(mailbox_ugd);
	}

	/* Last updated item should be removed after select all item is added. */
	mailbox_select_all_item_add(mailbox_ugd);
	mailbox_remove_unnecessary_list_item_for_edit_mode(mailbox_ugd);

	mailbox_hide_compose_btn(mailbox_ugd);
	if (mailbox_ugd->btn_mailbox)
		mailbox_naviframe_mailbox_button_remove(mailbox_ugd);

}

/*
 * Definition for exported functions
 */

Evas_Object *mailbox_create_toolbar_more_btn(EmailMailboxUGD *mailbox_ugd)
{
	debug_enter();

	Evas_Object *btn = elm_button_add(mailbox_ugd->base.module->navi);
	if (!btn) return NULL;
	elm_object_style_set(btn, "naviframe/more/default");
	evas_object_smart_callback_add(btn, "clicked", _more_toolbar_clicked_cb, mailbox_ugd);
	return btn;
}

void mailbox_create_compose_btn(EmailMailboxUGD *mailbox_ugd)
{
	debug_enter();
	retm_if(!mailbox_ugd, "mailbox_ugd is NULL");

	Evas_Object *floating_btn = eext_floatingbutton_add(mailbox_ugd->base.content);
	elm_object_part_content_set(mailbox_ugd->base.content, "elm.swallow.floatingbutton", floating_btn);
	evas_object_repeat_events_set(floating_btn, EINA_FALSE);

	Evas_Object *btn = elm_button_add(floating_btn);
	elm_object_part_content_set(floating_btn, "button1", btn);
	evas_object_smart_callback_add(btn, "clicked", _compose_toolbar_clicked_cb, mailbox_ugd);

	Evas_Object *img = elm_layout_add(btn);
	elm_layout_file_set(img, email_get_common_theme_path(), EMAIL_IMAGE_COMPOSE_BUTTON);
	elm_object_part_content_set(btn, "elm.swallow.content", img);

	evas_object_show(img);
	evas_object_show(floating_btn);
	mailbox_ugd->compose_btn = floating_btn;

	debug_leave();
}

void mailbox_show_compose_btn(EmailMailboxUGD *mailbox_ugd)
{
	debug_enter();
	retm_if(!mailbox_ugd, "mailbox_ugd is NULL");

	if (mailbox_ugd->compose_btn && !mailbox_ugd->b_editmode && !mailbox_ugd->b_searchmode) {
		elm_object_part_content_set(mailbox_ugd->base.content, "elm.swallow.floatingbutton", mailbox_ugd->compose_btn);
		evas_object_repeat_events_set(mailbox_ugd->compose_btn, EINA_FALSE);
		evas_object_show(mailbox_ugd->compose_btn);
	}

	debug_leave();
}

void mailbox_hide_compose_btn(EmailMailboxUGD *mailbox_ugd)
{
	debug_enter();
	retm_if(!mailbox_ugd, "mailbox_ugd is NULL");

	if (mailbox_ugd->compose_btn) {
		elm_object_part_content_unset(mailbox_ugd->base.content, "elm.swallow.floatingbutton");
		evas_object_hide(mailbox_ugd->compose_btn);
	}
	debug_leave();
}

