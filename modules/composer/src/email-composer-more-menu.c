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

#include "email-composer-more-menu.h"
#include "email-composer.h"
#include "email-composer-types.h"
#include "email-composer-util.h"
#include "email-composer-recipient.h"
#include "email-composer-attachment.h"
#include "email-composer-js.h"
#include "email-composer-webkit.h"
#include "email-composer-send-mail.h"
#include "email-composer-launcher.h"
#include "email-composer-initial-view.h"

/*
 * Declaration for static functions
 */

static Eina_Bool _more_menu_ctxpopup_show_timer_cb(void *data);
static void _more_menu_dismissed_cb(void *data, Evas_Object *obj, void *event_info);
static void _more_menu_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _more_menu_window_resized_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _more_menu_ctxpopup_show_cb(void *data, Evas_Object *obj, void *event_info);
static void _more_menu_clear_after_execution(void *data, Evas_Object *obj, void *event_info);
static void _more_menu_move_ctxpopup(Evas_Object *ctxpopup, Evas_Object *win);

static void _more_menu_send_to_myself_clicked(void *data, Evas_Object *obj, void *event_info);
static void _more_menu_richtext_toolbar_hide_clicked(void *data, Evas_Object *obj, void *event_info);
static void _more_menu_richtext_toolbar_show_clicked(void *data, Evas_Object *obj, void *event_info);
static void _more_menu_add_cc_bcc_clicked(void *data, Evas_Object *obj, void *event_info);
static void _more_menu_save_in_drafts_clicked(void *data, Evas_Object *obj, void *event_info);

static void _tomyself_append_myaddress(void *data, int index);
static void _tomyself_gl_sel(void *data, Evas_Object *obj, void *event_info);
static void _more_menu_ctxpopup_back_cb(void *data, Evas_Object *obj, void *event_info);

static email_string_t EMAIL_COMPOSER_STRING_NULL = { NULL, NULL };
static email_string_t EMAIL_COMPOSER_STRING_HEADER_SELECT_EMAIL_ADDRESS = { PACKAGE, "IDS_EMAIL_HEADER_SEND_TO_MYSELF_ABB" };


/*
 * Definition for static functions
 */

static Eina_Bool _more_menu_ctxpopup_show_timer_cb(void *data)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;
	view->timer_ctxpopup_show = NULL;
	Evas_Object *context_popup = view->context_popup;
	evas_object_show(context_popup);

	debug_leave();
	return ECORE_CALLBACK_CANCEL;
}

static void _more_menu_dismissed_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	DELETE_EVAS_OBJECT(view->context_popup);
	if (!view->composer_popup && !view->is_save_in_drafts_clicked) {
		composer_util_focus_set_focus_with_idler(view, view->selected_widget);
	}

	debug_leave();
}

static void _more_menu_ctxpopup_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	if (obj) {
		elm_ctxpopup_dismiss(obj);
	}
}

static void _more_menu_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	evas_object_smart_callback_del(view->context_popup, "dismissed", _more_menu_dismissed_cb);
	evas_object_event_callback_del_full(view->base.module->win, EVAS_CALLBACK_RESIZE, _more_menu_window_resized_cb, view);

	if (evas_object_freeze_events_get(view->ewk_view)) {
		evas_object_freeze_events_set(view->ewk_view, EINA_FALSE);
	}

	if (!view->is_save_in_drafts_clicked) {
		elm_object_tree_focus_allow_set(view->composer_layout, EINA_TRUE);
	}

	debug_leave();
}

static void _more_menu_window_resized_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	_more_menu_move_ctxpopup(view->context_popup, view->base.module->win);

	debug_leave();
}

static void _more_menu_ctxpopup_show_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	DELETE_TIMER_OBJECT(view->timer_ctxpopup_show);
	view->timer_ctxpopup_show = ecore_timer_add(0.3, _more_menu_ctxpopup_show_timer_cb, view);
	evas_object_smart_callback_del(obj, "virtualkeypad,state,off", _more_menu_ctxpopup_show_cb);

	debug_leave();
}

static void _more_menu_clear_after_execution(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	/* To dismiss context popup */
	_more_menu_dismissed_cb(data, obj, event_info);

	debug_leave();
}

static void _more_menu_move_ctxpopup(Evas_Object *ctxpopup, Evas_Object *win)
{
	debug_enter();

	Evas_Coord x, y, w, h;
	int pos = -1;

	elm_win_screen_size_get(win, &x, &y, &w, &h);
	pos = elm_win_rotation_get(win);

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
		default:
			break;
	}

	debug_leave();
}

static void _more_menu_send_to_myself_clicked(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	if (view->account_info->account_count == 1) {
		_tomyself_append_myaddress(data, 0);
		composer_util_focus_set_focus_with_idler(view, view->selected_widget);
	} else {
		composer_util_popup_create_account_list_popup(data, composer_util_popup_response_cb, _tomyself_gl_sel, "email.2text.2icon/popup", EMAIL_COMPOSER_STRING_HEADER_SELECT_EMAIL_ADDRESS, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);
	}
	_more_menu_clear_after_execution(data, obj, event_info);

	debug_leave();
}

static void _more_menu_richtext_toolbar_show_clicked(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	composer_initial_view_draw_richtext_components(view);

	email_set_richtext_toolbar_status(EINA_TRUE);

	DELETE_EVAS_OBJECT(view->context_popup);

	composer_util_focus_set_focus_with_idler(view, view->selected_widget);

	debug_leave();
}

static void _more_menu_richtext_toolbar_hide_clicked(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	retm_if(NULL == view->richtext_toolbar, "Richtext toolbar is not initialized!");

	DELETE_EVAS_OBJECT(view->richtext_toolbar);
	DELETE_EVAS_OBJECT(view->rttb_placeholder);

	email_set_richtext_toolbar_status(EINA_FALSE);

	DELETE_EVAS_OBJECT(view->context_popup);

	composer_util_focus_set_focus_with_idler(view, view->selected_widget);

	debug_leave();
}

static void _more_menu_add_cc_bcc_clicked(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	if (!view->recp_cc_box) {
		composer_recipient_update_cc_detail(view, view->recp_cc_layout);
		composer_recipient_update_bcc_detail(view, view->recp_bcc_layout);
	}

	if (view->cc_added) {
		composer_recipient_show_hide_cc_field(view, EINA_FALSE);
		composer_recipient_show_hide_bcc_field(view, EINA_FALSE);

		composer_util_recp_clear_mbe(view->recp_cc_mbe);
		composer_util_recp_clear_mbe(view->recp_bcc_mbe);

		composer_util_focus_set_focus_with_idler(view, view->recp_to_entry.entry);
	} else {
		composer_recipient_show_hide_cc_field(view, EINA_TRUE);
		composer_recipient_show_hide_bcc_field(view, EINA_TRUE);

		composer_util_focus_set_focus_with_idler(view, view->recp_cc_entry.entry);
	}
	DELETE_EVAS_OBJECT(view->context_popup);
}

static void _more_menu_save_in_drafts_clicked(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	view->is_save_in_drafts_clicked = EINA_TRUE;
	composer_exit_composer_get_contents(view);
	_more_menu_clear_after_execution(view, obj, event_info);

	debug_leave();
}

static void _tomyself_append_myaddress(void *data, int index)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;
	Evas_Object *dest_mbe = NULL;

	if (view->selected_widget == view->recp_cc_entry.entry) {
		if (view->cc_recipients_cnt > 0) {
			composer_recipient_reset_entry_with_mbe(view->composer_box, view->recp_cc_mbe_layout, view->recp_cc_mbe, view->recp_cc_layout, view->recp_cc_box, view->bcc_added ? view->recp_cc_label_cc : view->recp_cc_label_cc_bcc);
		}
		composer_recipient_change_entry(EINA_TRUE, view->recp_cc_box, view->recp_cc_entry_layout, view->recp_cc_display_entry_layout);
		dest_mbe = view->recp_cc_mbe;
	} else if (view->selected_widget == view->recp_bcc_entry.entry) {
		if (view->bcc_recipients_cnt > 0) {
			composer_recipient_reset_entry_with_mbe(view->composer_box, view->recp_bcc_mbe_layout, view->recp_bcc_mbe, view->recp_bcc_layout, view->recp_bcc_box, view->recp_bcc_label);
		}
		composer_recipient_change_entry(EINA_TRUE, view->recp_bcc_box, view->recp_bcc_entry_layout, view->recp_bcc_display_entry_layout);
		dest_mbe = view->recp_bcc_mbe;
	} else {
		if (view->to_recipients_cnt > 0) {
			composer_recipient_reset_entry_with_mbe(view->composer_box, view->recp_to_mbe_layout, view->recp_to_mbe, view->recp_to_layout, view->recp_to_box, view->recp_to_label);
		}
		composer_recipient_change_entry(EINA_TRUE, view->recp_to_box, view->recp_to_entry_layout, view->recp_to_display_entry_layout);
		dest_mbe = view->recp_to_mbe;
		view->selected_widget = view->recp_to_entry.entry; /* To set focus to 'to' mbe */
	}

	EmailRecpInfo *ri = composer_util_recp_make_recipient_info_with_display_name(view->account_info->account_list[index].user_email_address, view->account_info->account_list[index].user_display_name);
	retm_if(!ri, "Memory allocation failed!");

	char *markup_name = elm_entry_utf8_to_markup(ri->display_name);
	elm_multibuttonentry_item_append(dest_mbe, markup_name, NULL, ri);
	FREE(markup_name);

	debug_leave();
}

static void _tomyself_gl_sel(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;

	int index = (int)(ptrdiff_t)elm_object_item_data_get(item);

	debug_log("selected index:[%d]", index);

	_tomyself_append_myaddress(view, index);
	composer_util_popup_response_cb(data, obj, event_info);

	debug_leave();
}

static Elm_Object_Item *_add_ctxpopup_item(EmailComposerView *view, const char *str, Evas_Object *icon, Evas_Smart_Cb cb)
{
	Elm_Object_Item *ctx_menu_item = NULL;

	ctx_menu_item = elm_ctxpopup_item_append(view->context_popup, str, icon, cb, view);
	elm_object_item_domain_text_translatable_set(ctx_menu_item, PACKAGE, EINA_TRUE);
	return ctx_menu_item;
}


/*
 * Declaration for exported functions
 */

void composer_more_menu_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");
	retm_if(!obj, "Invalid parameter: obj is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	/* To prevent showing more menu before launching composer finishes. */
	retm_if(!view->is_load_finished, "Composer hasn't been launched completely!");

	/* To prevent B/S. If a user clicks hardware more key while destroying composer, it'll cause B/S sometimes. */
	retm_if(view->is_back_btn_clicked || view->is_save_in_drafts_clicked || view->is_send_btn_clicked, "Composer is being destroyed!");

	/* To prevent showing more menu while launching other application.
	 * If a user clicks hardware menu key while launching other applications(like camera via attach files), it'll cause strange behaviour.
	 */
	ret_if(view->base.module->is_launcher_busy);

	/* while launching menu popup, user can click webview.
	 * -> 1. Focus is on the subject entry with IME.
	 *	2. Click menu hard key.
	 *	3. Click webview field immediately.
	 *	4. Menu popup launches. but IME is also shown because webview gets the focus.
	 * This freezing event code is needed to prevent this scenario.
	 */
	evas_object_freeze_events_set(view->ewk_view, EINA_TRUE);
	elm_object_focus_set(view->ewk_btn, EINA_FALSE);
	ecore_imf_input_panel_hide();
	DELETE_EVAS_OBJECT(view->context_popup);

	view->context_popup = elm_ctxpopup_add(view->base.module->win);
	elm_object_style_set(view->context_popup, "more/default");
	elm_ctxpopup_direction_priority_set(view->context_popup, ELM_CTXPOPUP_DIRECTION_UP, ELM_CTXPOPUP_DIRECTION_UNKNOWN, ELM_CTXPOPUP_DIRECTION_UNKNOWN, ELM_CTXPOPUP_DIRECTION_UNKNOWN);
	elm_ctxpopup_auto_hide_disabled_set(view->context_popup, EINA_TRUE);

	/* To prevent setting focus to last selected widget IME when device is rotated. (when ec_window is resized) */
	elm_object_tree_focus_allow_set(view->composer_layout, EINA_FALSE);

	evas_object_smart_callback_add(view->context_popup, "dismissed", _more_menu_dismissed_cb, view);
	evas_object_event_callback_add(view->context_popup, EVAS_CALLBACK_DEL, _more_menu_del_cb, view);
	eext_object_event_callback_add(view->context_popup, EEXT_CALLBACK_BACK, _more_menu_ctxpopup_back_cb, view);
	eext_object_event_callback_add(view->context_popup, EEXT_CALLBACK_MORE, _more_menu_ctxpopup_back_cb, view);
	evas_object_event_callback_add(view->base.module->win, EVAS_CALLBACK_RESIZE, _more_menu_window_resized_cb, view);

	if (!view->cc_added) {
		_add_ctxpopup_item(view, "IDS_EMAIL_OPT_ADD_CC_BCC", NULL, _more_menu_add_cc_bcc_clicked);
	}

	Elm_Object_Item *save_in_drafts_it = elm_ctxpopup_item_append(view->context_popup, "IDS_EMAIL_OPT_SAVE_IN_DRAFTS_ABB", NULL, _more_menu_save_in_drafts_clicked, data);
	elm_object_item_domain_text_translatable_set(save_in_drafts_it, PACKAGE, EINA_TRUE);
	if (composer_util_is_max_sending_size_exceeded(view)) {
		elm_object_item_disabled_set(save_in_drafts_it, EINA_TRUE);
	}

	_add_ctxpopup_item(view, "IDS_EMAIL_OPT_SEND_TO_MYSELF_ABB", NULL, _more_menu_send_to_myself_clicked);

	Eina_Bool richtext_toolbar_enabled = EINA_TRUE;
	if (email_get_richtext_toolbar_status(&richtext_toolbar_enabled) != 0) { /*It does not exist */
		richtext_toolbar_enabled = EINA_FALSE;
		email_set_richtext_toolbar_status(richtext_toolbar_enabled);
	}
	if (true == richtext_toolbar_enabled) {
		_add_ctxpopup_item(view, "IDS_EMAIL_OPT_DISABLE_RICH_TEXT_ABB2", NULL, _more_menu_richtext_toolbar_hide_clicked);
	} else {
		_add_ctxpopup_item(view, "IDS_EMAIL_OPT_ENABLE_RICH_TEXT_ABB2", NULL, _more_menu_richtext_toolbar_show_clicked);
	}

	/* Move context popup to proper position. */
	_more_menu_move_ctxpopup(view->context_popup, view->base.module->win);
	if (evas_object_size_hint_display_mode_get(view->base.module->navi) == EVAS_DISPLAY_MODE_COMPRESS) {
		evas_object_smart_callback_add(view->base.module->conform, "virtualkeypad,state,off", _more_menu_ctxpopup_show_cb, view);
	} else {
		evas_object_show(view->context_popup);
	}

	debug_leave();
}

/* EOF */
