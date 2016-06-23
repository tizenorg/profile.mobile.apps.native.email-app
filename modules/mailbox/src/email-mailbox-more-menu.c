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
#include "email-mailbox-module-util.h"
#include "email-mailbox-util.h"
#include "email-mailbox-more-menu.h"
#include "email-mailbox-noti-mgr.h"
#include "email-mailbox-sync.h"
#include "email-mailbox-search.h"
#include "email-mailbox-list-other-items.h"

/*
 * Declaration for static functions
 */

/* More menu logic */
static void _more_ctxpopup_dismissed_cb(void *data, Evas_Object *obj, void *event_info);
static void _more_ctxpopup_back_cb(void *data, Evas_Object *obj, void *event_info);
static void _move_more_ctxpopup(Evas_Object *ctxpopup, Evas_Object *win);
static void _resize_more_ctxpopup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _delete_more_ctxpopup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _more_toolbar_clicked_cb(void *data, Evas_Object *obj, void *event_info);

/*More menu option items callbacks*/
static void _more_refresh_cb(void *data, Evas_Object *obj, void *event_info);
static void _more_search_cb(void *data, Evas_Object *obj, void *event_info);
static void _more_search_in_all_folders_cb(void *data, Evas_Object *obj, void *event_info);
static void _more_edit_unread_mode_cb(void *data, Evas_Object *obj, void *event_info);
static void _more_edit_read_mode_cb(void *data, Evas_Object *obj, void *event_info);
static void _more_edit_remove_spam_mode_cb(void *data, Evas_Object *obj, void *event_info);
static void _more_edit_spam_mode_cb(void *data, Evas_Object *obj, void *event_info);
static void _more_edit_move_mode_cb(void *data, Evas_Object *obj, void *event_info);
static void _more_edit_delete_mode_cb(void *data, Evas_Object *obj, void *event_info);
static void _more_settings_cb(void *data, Evas_Object *obj, void *event_info);


/*Floating compose button*/
static void _compose_toolbar_clicked_cb(void *data, Evas_Object *obj, void *event_info);

/*Edit mode view setup*/
static void _change_view_for_selection_mode(EmailMailboxView *view);
static void _setup_edit_mode(EmailMailboxView *view, mailbox_edit_type_e edit_type);

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
	EmailMailboxView *view;
} RadioItemData;


/*
 * Globals
 */

/*
 * Definition for static functions
 */

static void _setup_edit_mode(EmailMailboxView *view, mailbox_edit_type_e edit_type)
{
	DELETE_EVAS_OBJECT(view->more_ctxpopup);
	if (!view->b_editmode) {
		view->editmode_type = edit_type;
		view->selected_mail_list = eina_list_free(view->selected_mail_list);
		view->selected_mail_list = NULL;
		debug_log("Enter edit mode. type:(%d)", view->editmode_type);

		view->b_editmode = true;
		mailbox_sync_cancel_all(view);
		_change_view_for_selection_mode(view);
		mailbox_toolbar_create(view);
		mailbox_create_select_info(view);
	} else {
		debug_warning("error status: already edit mode");
	}

}

static void _more_edit_delete_mode_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailMailboxView *view = (EmailMailboxView *)data;
	_setup_edit_mode(view, MAILBOX_EDIT_TYPE_DELETE);
}

static void _more_edit_move_mode_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailMailboxView *view = (EmailMailboxView *)data;
	_setup_edit_mode(view, MAILBOX_EDIT_TYPE_MOVE);

}

static void _more_edit_spam_mode_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailMailboxView *view = (EmailMailboxView *)data;
	_setup_edit_mode(view, MAILBOX_EDIT_TYPE_ADD_SPAM);
}

static void _more_edit_remove_spam_mode_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailMailboxView *view = (EmailMailboxView *)data;
	_setup_edit_mode(view, MAILBOX_EDIT_TYPE_REMOVE_SPAM);

}

static void _more_edit_read_mode_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailMailboxView *view = (EmailMailboxView *)data;
	_setup_edit_mode(view, MAILBOX_EDIT_TYPE_MARK_READ);

}

static void _more_edit_unread_mode_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailMailboxView *view = (EmailMailboxView *)data;
	_setup_edit_mode(view, MAILBOX_EDIT_TYPE_MARK_UNREAD);

}

static void _more_settings_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailMailboxView *view = (EmailMailboxView *)data;
	email_params_h params = NULL;

	DELETE_EVAS_OBJECT(view->more_ctxpopup);

	if (email_params_create(&params) &&
		email_params_add_str(params, EMAIL_BUNDLE_KEY_VIEW_TYPE, EMAIL_BUNDLE_VAL_VIEW_SETTING)) {

		view->setting = mailbox_setting_module_create(view, EMAIL_MODULE_SETTING, params);
	}

	email_params_free(&params);
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

	EmailMailboxView *view = (EmailMailboxView *)data;
	DELETE_EVAS_OBJECT(view->more_ctxpopup);
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

	EmailMailboxView *view = (EmailMailboxView *)data;
	_move_more_ctxpopup(view->more_ctxpopup, view->base.module->win);
}

static void _delete_more_ctxpopup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailMailboxView *view = (EmailMailboxView *)data;

	evas_object_event_callback_del(view->base.module->navi, EVAS_CALLBACK_RESIZE, _resize_more_ctxpopup_cb);
	eext_object_event_callback_del(view->more_ctxpopup, EEXT_CALLBACK_BACK, _more_ctxpopup_back_cb);
	eext_object_event_callback_del(view->more_ctxpopup, EEXT_CALLBACK_MORE, _more_ctxpopup_back_cb);
	evas_object_smart_callback_del(view->more_ctxpopup, "dismissed", _more_ctxpopup_dismissed_cb);
	evas_object_event_callback_del(view->more_ctxpopup, EVAS_CALLBACK_DEL, _delete_more_ctxpopup_cb);
}

static void _more_refresh_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailMailboxView *view = (EmailMailboxView *)data;
	mailbox_do_refresh(view);

	DELETE_EVAS_OBJECT(view->more_ctxpopup);
	debug_leave();
}

static void _more_search_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailMailboxView *view = (EmailMailboxView *)data;
	mailbox_actiate_search_mode(view, EMAIL_SEARCH_IN_SINGLE_FOLDER);

	DELETE_EVAS_OBJECT(view->more_ctxpopup);
	debug_leave();
}

static void _more_search_in_all_folders_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailMailboxView *view = (EmailMailboxView *)data;
	mailbox_actiate_search_mode(view, EMAIL_SEARCH_IN_ALL_FOLDERS);

	DELETE_EVAS_OBJECT(view->more_ctxpopup);
	debug_leave();
}
static void _compose_toolbar_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailMailboxView *view = (EmailMailboxView *)data;
	RETURN_IF_FAIL(view != NULL);

	retm_if(view->is_module_launching, "is_module_launching is true;");
	view->is_module_launching = true;

	int account_id = 0;

	DELETE_EVAS_OBJECT(view->more_ctxpopup);
	DELETE_EVAS_OBJECT(view->error_popup);

	if (view->account_count == 0) {
		mailbox_create_timeout_popup(view, _("IDS_ST_HEADER_WARNING"),
				_("IDS_EMAIL_POP_YOU_HAVE_NOT_YET_CREATED_AN_EMAIL_ACCOUNT_CREATE_AN_EMAIL_ACCOUNT_AND_TRY_AGAIN"), 3.0);
		view->is_module_launching = false;
		return;
	}

	if (view->composer) {
		debug_log("Composer module is already launched");
		view->is_module_launching = false;
		return;
	}

	mailbox_sync_cancel_all(view);

	gint composer_type = RUN_COMPOSER_NEW;
	account_id = view->account_id;
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
		view->is_module_launching = false;
		return;
	}

	debug_log("composer type: %d", composer_type);
	debug_log("account id: %d", account_id);

	email_params_h params = NULL;

	if (email_params_create(&params) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_RUN_TYPE, composer_type) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_ACCOUNT_ID, account_id)) {

		view->composer = mailbox_composer_module_create(view, EMAIL_MODULE_COMPOSER, params);
	} else {
		view->is_module_launching = false;
	}

	email_params_free(&params);
	debug_leave();
}

static Elm_Object_Item *_add_ctx_menu_item(EmailMailboxView *view, const char *str, Evas_Object *icon, Evas_Smart_Cb cb)
{
	Elm_Object_Item *ctx_menu_item = NULL;

	ctx_menu_item = elm_ctxpopup_item_append(view->more_ctxpopup, str, icon, cb, view);
	elm_object_item_domain_text_translatable_set(ctx_menu_item, PACKAGE, EINA_TRUE);
	return ctx_menu_item;
}

static void _more_toolbar_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailMailboxView *view = (EmailMailboxView *)data;

	retm_if(view->composer, "view->composer(%p) is being shown.", view->composer);
	retm_if(view->viewer, "view->viewer(%p) is being shown.", view->viewer);
	retm_if(view->account, "view->account(%p) is being shown.", view->account);
	retm_if(view->b_editmode, "view->b_editmode(%d) Edit mode is ON.", view->b_editmode);

	if (view->b_searchmode && view->search_type != EMAIL_SEARCH_IN_SINGLE_FOLDER) {
		debug_log("Search mode is ON, Search type: %d", view->search_type);
		return;
	}

	view->more_ctxpopup = elm_ctxpopup_add(view->base.module->win);

	elm_ctxpopup_auto_hide_disabled_set(view->more_ctxpopup, EINA_TRUE);
	elm_object_style_set(view->more_ctxpopup, "more/default");
	eext_object_event_callback_add(view->more_ctxpopup, EEXT_CALLBACK_BACK, _more_ctxpopup_back_cb, view);
	eext_object_event_callback_add(view->more_ctxpopup, EEXT_CALLBACK_MORE, _more_ctxpopup_back_cb, view);
	evas_object_smart_callback_add(view->more_ctxpopup, "dismissed", _more_ctxpopup_dismissed_cb, view);

	evas_object_event_callback_add(view->more_ctxpopup, EVAS_CALLBACK_DEL, _delete_more_ctxpopup_cb, view);
	evas_object_event_callback_add(view->base.module->navi, EVAS_CALLBACK_RESIZE, _resize_more_ctxpopup_cb, view);

	if (view->b_searchmode) {
		//TODO Need to change with translatable IDS when it will be given form UX
		_add_ctx_menu_item(view, "Search all folders", NULL, _more_search_in_all_folders_cb);
		if (view->search_editfield.entry) {
			Ecore_IMF_Context *imf_context = elm_entry_imf_context_get(view->search_editfield.entry);
			ecore_imf_context_input_panel_hide(imf_context);
		}
	} else {
		_add_ctx_menu_item(view, "IDS_EMAIL_OPT_REFRESH", NULL, _more_refresh_cb);
		_add_ctx_menu_item(view, "IDS_EMAIL_OPT_SEARCH", NULL, _more_search_cb);

		if (g_list_length(view->mail_list) > 0) {
			_add_ctx_menu_item(view, "IDS_EMAIL_OPT_DELETE", NULL, _more_edit_delete_mode_cb);
			if (view->mailbox_type == EMAIL_MAILBOX_TYPE_INBOX
					|| view->mailbox_type == EMAIL_MAILBOX_TYPE_TRASH
					|| view->mailbox_type == EMAIL_MAILBOX_TYPE_SPAMBOX
					|| view->mailbox_type == EMAIL_MAILBOX_TYPE_USER_DEFINED) {
				_add_ctx_menu_item(view, "IDS_EMAIL_OPT_MOVE", NULL, _more_edit_move_mode_cb);
				if (view->mailbox_type != EMAIL_MAILBOX_TYPE_SPAMBOX) {
					_add_ctx_menu_item(view, "IDS_EMAIL_OPT_MOVE_TO_SPAMBOX", NULL, _more_edit_spam_mode_cb);
				} else {
					_add_ctx_menu_item(view, "IDS_EMAIL_OPT_REMOVE_FROM_SPAM_ABB", NULL, _more_edit_remove_spam_mode_cb);
				}
			}
			if (view->mailbox_type != EMAIL_MAILBOX_TYPE_OUTBOX && view->mailbox_type != EMAIL_MAILBOX_TYPE_DRAFT
					&& view->mailbox_type != EMAIL_MAILBOX_TYPE_TRASH && view->mailbox_type != EMAIL_MAILBOX_TYPE_SPAMBOX) {
				_add_ctx_menu_item(view, "IDS_EMAIL_OPT_MARK_AS_READ_ABB", NULL, _more_edit_read_mode_cb);
				_add_ctx_menu_item(view, "IDS_EMAIL_OPT_MARK_AS_UNREAD_ABB", NULL, _more_edit_unread_mode_cb);
			}
		}
		_add_ctx_menu_item(view, "IDS_EMAIL_OPT_SETTINGS", NULL, _more_settings_cb);
	}

	elm_ctxpopup_direction_priority_set(view->more_ctxpopup, ELM_CTXPOPUP_DIRECTION_UP, ELM_CTXPOPUP_DIRECTION_UNKNOWN, ELM_CTXPOPUP_DIRECTION_UNKNOWN, ELM_CTXPOPUP_DIRECTION_UNKNOWN);
	_move_more_ctxpopup(view->more_ctxpopup, view->base.module->win);

	evas_object_show(view->more_ctxpopup);
}

static void _change_view_for_selection_mode(EmailMailboxView *view)
{
	debug_enter();
	retm_if(!view, "view is NULL");
	elm_genlist_realized_items_update(view->gl);

	if ((view->mode == EMAIL_MAILBOX_MODE_ALL
	|| view->mode == EMAIL_MAILBOX_MODE_MAILBOX)
	&& view->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX) {
		mailbox_send_all_btn_remove(view);
	}

	/* Last updated item should be removed after select all item is added. */
	mailbox_select_all_item_add(view);
	mailbox_remove_unnecessary_list_item_for_edit_mode(view);

	mailbox_hide_compose_btn(view);
	if (view->btn_mailbox)
		mailbox_naviframe_mailbox_button_remove(view);

}

/*
 * Definition for exported functions
 */

Evas_Object *mailbox_create_toolbar_more_btn(EmailMailboxView *view)
{
	debug_enter();

	Evas_Object *btn = elm_button_add(view->base.module->navi);
	retvm_if(!btn, NULL, "btn is NULL");

	elm_object_style_set(btn, "naviframe/more/default");
	evas_object_smart_callback_add(btn, "clicked", _more_toolbar_clicked_cb, view);
	return btn;
}

void mailbox_create_compose_btn(EmailMailboxView *view)
{
	debug_enter();
	retm_if(!view, "view is NULL");

	Evas_Object *floating_btn = eext_floatingbutton_add(view->base.content);
	elm_object_part_content_set(view->base.content, "elm.swallow.floatingbutton", floating_btn);
	evas_object_repeat_events_set(floating_btn, EINA_FALSE);

	Evas_Object *btn = elm_button_add(floating_btn);
	elm_object_part_content_set(floating_btn, "button1", btn);
	evas_object_smart_callback_add(btn, "clicked", _compose_toolbar_clicked_cb, view);

	Evas_Object *img = elm_layout_add(btn);
	elm_layout_file_set(img, email_get_common_theme_path(), EMAIL_IMAGE_COMPOSE_BUTTON);
	elm_object_part_content_set(btn, "elm.swallow.content", img);

	evas_object_show(img);
	evas_object_show(floating_btn);
	view->compose_btn = floating_btn;

	debug_leave();
}

void mailbox_show_compose_btn(EmailMailboxView *view)
{
	debug_enter();
	retm_if(!view, "view is NULL");

	if (view->compose_btn && !view->b_editmode && !view->b_searchmode) {
		elm_object_part_content_set(view->base.content, "elm.swallow.floatingbutton", view->compose_btn);
		evas_object_repeat_events_set(view->compose_btn, EINA_FALSE);
		evas_object_show(view->compose_btn);
	}

	debug_leave();
}

void mailbox_hide_compose_btn(EmailMailboxView *view)
{
	debug_enter();
	retm_if(!view, "view is NULL");

	if (view->compose_btn) {
		elm_object_part_content_unset(view->base.content, "elm.swallow.floatingbutton");
		evas_object_hide(view->compose_btn);
	}
	debug_leave();
}

