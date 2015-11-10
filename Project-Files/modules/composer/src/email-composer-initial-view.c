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

#include "email-composer.h"
#include "email-composer-util.h"
#include "email-composer-recipient.h"
#include "email-composer-subject.h"
#include "email-composer-webkit.h"
#include "email-composer-more-menu.h"
#include "email-composer-send-mail.h"
#include "email-composer-initial-view.h"
#include "email-composer-attachment.h"
#include "email-composer-rich-text-toolbar.h"
#include "email-composer-js.h"
#include "email-color-box.h"

#define CS_BRING_REL_ACCEL1 (ELM_SCALE_SIZE(20.0f))
#define CS_BRING_REL_ACCEL2 (ELM_SCALE_SIZE(10.0f))
#define CS_BRING_ADJUST_TIME 0.1f

#define CS_DRAG_ACCEL (ELM_SCALE_SIZE(5000.0f))
#define CS_DRAG_THRESHOLD_PX (ELM_SCALE_SIZE(15))

#define CS_CARET_PADDING_PX (ELM_SCALE_SIZE(15))

#define CS_CARET_BACKUP_TIMEOUT_SEC 0.5
#define CS_UPDATE_TIMEOUT_SEC 0.1
#define CS_INITIALIZE_TIMEOUT_SEC 0.8

/*
 * Declaration for static functions
 */

static void _initial_view_back_button_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static void _initial_view_send_button_clicked_cb(void *data, Evas_Object *obj, void *event_info);

static Evas_Object *_initial_view_create_root_layout(Evas_Object *parent, EmailComposerUGD *ugd);
static Evas_Object *_initial_view_create_composer_layout(Evas_Object *parent);
static void _initial_view_create_naviframe_buttons(Evas_Object *parent, Evas_Object *content, EmailComposerUGD *ugd);

static void _initial_view_update_rttb_position(EmailComposerUGD *ugd);
static void _initial_view_notify_ewk_visible_area(EmailComposerUGD *ugd);

static void _initial_view_cs_stop_animator(EmailComposerUGD *ugd);
static void _initial_view_cs_stop_dragging(EmailComposerUGD *ugd);
static int _initial_view_cs_fix_pos(EmailComposerUGD *ugd, int y);
static void _initial_view_cs_set_pos(EmailComposerUGD *ugd, int y);
static void _initial_view_cs_handle_event(EmailComposerUGD *ugd, int event_mask);
static void _initial_view_cs_update(EmailComposerUGD *ugd, int event_mask);
static void _initial_view_cs_sync_with_ewk(EmailComposerUGD *ugd);

static void _initial_view_cs_handle_caret_pos_change(EmailComposerUGD *ugd, int top, int bottom);
static void _initial_view_cs_ensure_ewk_on_top(EmailComposerUGD *ugd, bool force_bring_in);
static void _initial_view_cs_backup_scroll_pos(EmailComposerUGD *ugd);

static Eina_Bool _initial_view_cs_event_timer1_cb(void *data);
static Eina_Bool _initial_view_cs_event_timer2_cb(void *data);
static Eina_Bool _initial_view_cs_animator_cb(void *data, double pos);

static Evas_Object *_initial_view_create_combined_scroller_layout(Evas_Object *parent);

static void _initial_view_main_scroller_resize_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _initial_view_main_scroller_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _initial_view_main_scroller_mouse_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _initial_view_main_scroller_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void _initial_view_composer_layout_resize_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void _initial_view_main_scroller_scroll_cb(void *data, Evas_Object *obj, void *event_info);

static void _initial_view_magnifier_show_cb(void *data, Evas_Object *obj, void *event_info);
static void _initial_view_magnifier_hide_cb(void *data, Evas_Object *obj, void *event_info);
static void _initial_view_ewk_content_resize_cb(void *data, Evas_Object *obj, void *event_info);

/*
 * Definition for static functions
 */

static void _initial_view_back_button_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	/* To prevent buttons on naviframe being processed while launching composer. this flag is set after executing all js via webkit. */
	retm_if(!ugd->is_load_finished, "_initial_view_back_button_clicked_cb() return; ugd->is_load_finished = false");

	/* To prevent B/S. If a user clicks hardware back key or 'X' button on the navi bar while launching other applications(like camera via attach files), it'll cause B/S.
	 * Because result callback for the launching request is called after the composer is destroyed.
	 */
	retm_if(ugd->base.module->is_launcher_busy, "is_launcher_busy = true");
	retm_if(ugd->is_back_btn_clicked || ugd->is_save_in_drafts_clicked || ugd->is_send_btn_clicked, "while destroying composer!");

	Ecore_IMF_Context *ctx = ecore_imf_context_add(ecore_imf_context_default_id_get());
	Ecore_IMF_Input_Panel_State imf_state = ecore_imf_context_input_panel_state_get(ctx);
	ecore_imf_context_del(ctx);

	debug_log("imf state ==> %d", imf_state);

	ugd->is_back_btn_clicked = EINA_TRUE;
	if (imf_state != ECORE_IMF_INPUT_PANEL_STATE_SHOW) {
		composer_exit_composer_get_contents(ugd);
	}

	debug_leave();
}

static void _initial_view_send_button_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");
	retm_if(!obj, "Invalid parameter: obj is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	/* To prevent buttons on naviframe being processed while launching composer. this flag is set after executing all js via webkit. */
	retm_if(!ugd->is_load_finished, "_initial_view_send_button_clicked_cb() return; ugd->is_load_finished = false");

	/* To prevent B/S. If a user clicks 'send' button on the navi bar while launching other applications(like camera via attach files), it'll cause B/S.
	 * Because result callback for the launching request is called after the composer is destroyed.
	 */
	retm_if(ugd->base.module->is_launcher_busy, "is_launcher_busy = true");

	/* if a user clicks 'send' button more than once. it may cause B/S */
	retm_if(ugd->is_send_btn_clicked, "_initial_view_send_button_clicked_cb() return; ugd->is_send_btn_clicked is already clicked");

	ugd->is_send_btn_clicked = EINA_TRUE;
	composer_exit_composer_get_contents(ugd);

	debug_leave();
}

static Evas_Object *_initial_view_create_composer_box(Evas_Object *parent)
{
	email_profiling_begin(_initial_view_create_composer_box);
	debug_enter();

	Evas_Object *box = email_color_box_add(parent);
	debug_leave();
	email_profiling_end(_initial_view_create_composer_box);
	return box;
}

static Evas_Object *_initial_view_create_root_layout(Evas_Object *parent, EmailComposerUGD *ugd)
{
	debug_enter();

	retvm_if(!parent, NULL, "Invalid parameter: parent is NULL!");

	Evas_Object *layout = elm_layout_add(parent);
	retvm_if(!layout, NULL, "elm_layout_add() failed!");

	elm_layout_file_set(layout, email_get_composer_theme_path(), "ec/layout/root");
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(layout);

	if (ugd->theme) {
		elm_object_theme_set(layout, ugd->theme);
	}

	debug_leave();
	return layout;
}

static Evas_Object *_initial_view_create_composer_layout(Evas_Object *parent)
{
	debug_enter();

	email_profiling_begin(_initial_view_create_composer_layout);

	retvm_if(!parent, NULL, "Invalid parameter: parent is NULL!");

	Evas_Object *layout = elm_layout_add(parent);
	retvm_if(!layout, NULL, "elm_layout_add() failed!");

	elm_layout_file_set(layout, email_get_composer_theme_path(), "ec/layout/base");
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(layout);

	email_profiling_end(_initial_view_create_composer_layout);

	debug_leave();
	return layout;
}

static void _initial_view_create_naviframe_buttons(Evas_Object *parent, Evas_Object *content, EmailComposerUGD *ugd)
{
	debug_enter();

	email_profiling_begin(_initial_view_create_naviframe_buttons);

	retm_if(!parent, "Invalid parameter: parent is NULL!");

	Elm_Object_Item *navi_item = email_module_view_push(&ugd->base, "IDS_EMAIL_HEADER_COMPOSE_ABB", 0);

	elm_object_item_domain_text_translatable_set(navi_item, PACKAGE, EINA_TRUE);

	Evas_Object *more_btn = elm_button_add(parent);
	elm_object_style_set(more_btn, "naviframe/more/default");
	elm_object_item_part_content_set(navi_item, "toolbar_more_btn", more_btn);
	evas_object_smart_callback_add(more_btn, "clicked", composer_more_menu_clicked_cb, ugd);

	Evas_Object *cancel_btn = elm_button_add(parent);
	elm_object_style_set(cancel_btn, "naviframe/title_left");
	elm_object_item_part_content_set(navi_item, "title_left_btn", cancel_btn);
	elm_object_domain_translatable_text_set(cancel_btn, PACKAGE, "IDS_TPLATFORM_ACBUTTON_CANCEL_ABB");
	evas_object_smart_callback_add(cancel_btn, "clicked", _initial_view_back_button_clicked_cb, ugd);

	Evas_Object *send_btn = elm_button_add(parent);
	elm_object_style_set(send_btn, "naviframe/title_right");
	elm_object_item_part_content_set(navi_item, "title_right_btn", send_btn);
	elm_object_domain_translatable_text_set(send_btn, PACKAGE, "IDS_TPLATFORM_ACBUTTON_SEND_ABB");
	evas_object_smart_callback_add(send_btn, "clicked", _initial_view_send_button_clicked_cb, ugd);
	elm_object_disabled_set(send_btn, EINA_TRUE);

	ugd->send_btn = send_btn;

	email_profiling_end(_initial_view_create_naviframe_buttons);

	debug_leave();
}

static void _initial_view_update_rttb_position(EmailComposerUGD *ugd)
{
	if (!ugd->rttb_placeholder && !ugd->richtext_toolbar) {
		return;
	}

	if (ugd->rttb_placeholder) {
		if (ugd->cs_main_scroll_pos < ugd->cs_edge_scroll_pos) {
			debug_log("Unpin from top.");

			DELETE_EVAS_OBJECT(ugd->rttb_placeholder);

			elm_object_part_content_unset(ugd->base.content, "ec.swallow.richtext");
			elm_box_pack_end(ugd->composer_box, ugd->richtext_toolbar);
		}
	} else {
		if (ugd->cs_main_scroll_pos >= ugd->cs_edge_scroll_pos) {
			debug_log("Pin to top.");

			Evas_Object *bg = NULL;
			int rttb_h = 0;

			evas_object_geometry_get(ugd->richtext_toolbar, NULL, NULL, NULL, &rttb_h);

			elm_box_unpack(ugd->composer_box, ugd->richtext_toolbar);
			elm_object_part_content_set(ugd->base.content, "ec.swallow.richtext", ugd->richtext_toolbar);

			bg = elm_bg_add(ugd->composer_box);
			elm_box_pack_end(ugd->composer_box, bg);
			evas_object_size_hint_min_set(bg, 0, rttb_h);
			evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, 0);
			evas_object_size_hint_align_set(bg, EVAS_HINT_FILL, 0);
			evas_object_show(bg);

			ugd->rttb_placeholder = bg;
		}
	}
}

static void _initial_view_notify_ewk_visible_area(EmailComposerUGD *ugd)
{
	Eina_Rectangle rect = { 0 };
	int min_y = ugd->cs_top + ugd->cs_rttb_height;

	rect.x = 0;
	rect.w = ugd->cs_width;

	rect.y = ugd->cs_top + ugd->cs_header_height - ugd->cs_scroll_pos;
	if (rect.y < min_y) {
		rect.y = min_y;
	}

	rect.h = ugd->cs_top + ugd->cs_height - rect.y;
	if (rect.h < 0) {
		rect.h = 0;
	}

	evas_object_smart_callback_call(ugd->ewk_view, "visible,content,changed", &rect);
}

static void _initial_view_cs_stop_animator(EmailComposerUGD *ugd)
{
	debug_enter();

	if (ugd->cs_animator) {
		ecore_animator_del(ugd->cs_animator);
		ugd->cs_animator = NULL;
	}
}

static void _initial_view_cs_stop_dragging(EmailComposerUGD *ugd)
{
	ugd->cs_is_sliding = false;
	ugd->cs_is_dragging = false;
}

static int _initial_view_cs_fix_pos(EmailComposerUGD *ugd, int y)
{
	int result = 0;

	if (y < 0) {
		result = 0;
	} else if (y > ugd->cs_max_scroll_pos) {
		result = ugd->cs_max_scroll_pos;
	} else {
		result = y;
	}

	return result;
}

static void _initial_view_cs_set_pos(EmailComposerUGD *ugd, int y)
{
	int ewk_h = 0;
	int edge_pos = 0;
	int ewk_scroll_pos = 0;

	if (!ugd->cs_ready) {
		return;
	}

	ugd->cs_scroll_pos = _initial_view_cs_fix_pos(ugd, y);

	evas_object_geometry_get(ugd->ewk_view, NULL, NULL, NULL, &ewk_h);

	edge_pos = (ewk_h - ugd->cs_height - ugd->cs_rttb_height) / 2 + ugd->cs_header_height;

	if (ugd->cs_scroll_pos > edge_pos) {
		ewk_scroll_pos = ugd->cs_scroll_pos - edge_pos;
		if (ewk_scroll_pos > ugd->cs_max_ewk_scroll_pos) {
			ewk_scroll_pos = ugd->cs_max_ewk_scroll_pos;
			ugd->cs_main_scroll_pos = ugd->cs_scroll_pos - ewk_scroll_pos;
		} else {
			ugd->cs_main_scroll_pos = edge_pos;
		}
	} else {
		ugd->cs_main_scroll_pos = ugd->cs_scroll_pos;
		ewk_scroll_pos = 0;
	}

	elm_scroller_region_show(ugd->main_scroller, 0, ugd->cs_main_scroll_pos, ugd->cs_width, ugd->cs_height);
	ewk_view_scroll_set(ugd->ewk_view, 0, ewk_scroll_pos);

	edje_object_part_drag_size_set(_EDJ(ugd->combined_scroller), "elm.dragable.vbar",
			1.0, 1.0 * ugd->cs_height / (ugd->cs_height + ugd->cs_max_scroll_pos));
	edje_object_part_drag_value_set(_EDJ(ugd->combined_scroller), "elm.dragable.vbar",
			0.0, 1.0 * ugd->cs_scroll_pos / ugd->cs_max_scroll_pos);
	edje_object_signal_emit(_EDJ(ugd->combined_scroller), "elm,action,scroll_v", "elm");

	_initial_view_update_rttb_position(ugd);
	_initial_view_notify_ewk_visible_area(ugd);
}

static void _initial_view_cs_handle_event(EmailComposerUGD *ugd, int event_mask)
{
	if (event_mask & COMPOSER_CSEF_IMMEDIATE_EVENTS) {
		if (ugd->cs_immediate_event_mask != 0) {
			debug_log("Recursion.");
			ugd->cs_immediate_event_mask |= event_mask;
			return;
		}
		ugd->cs_immediate_event_mask = event_mask;
		if (event_mask & (COMPOSER_CSEF_IMMEDIATE_EVENTS & ~COMPOSER_CSEF_DRAG_EVENTS)) {
			evas_smart_objects_calculate(evas_object_evas_get(ugd->composer_layout));
		}
		_initial_view_cs_update(ugd, ugd->cs_immediate_event_mask);
		ugd->cs_immediate_event_mask = 0;
	}

	if (event_mask & COMPOSER_CSEF_PENDING_EVENTS) {
		if (event_mask & COMPOSER_CSEF_EWK_CONTENT_RESIZE) {
			DELETE_TIMER_OBJECT(ugd->cs_events_timer1);
			ugd->cs_events_timer1 = ecore_timer_add(CS_UPDATE_TIMEOUT_SEC, _initial_view_cs_event_timer1_cb, ugd);
		}
		if (event_mask & COMPOSER_CSEF_INITIALIZE) {
			DELETE_TIMER_OBJECT(ugd->cs_events_timer2);
			ugd->cs_events_timer2 = ecore_timer_add(CS_INITIALIZE_TIMEOUT_SEC, _initial_view_cs_event_timer2_cb, ugd);
		}
		ugd->cs_pending_event_mask |= (event_mask & COMPOSER_CSEF_PENDING_EVENTS);
	}
}

static void _initial_view_cs_update(EmailComposerUGD *ugd, int event_mask)
{
	int new_scroll_pos = 0;
	bool need_set_pos = false;

	_initial_view_cs_sync_with_ewk(ugd);

	if (event_mask & COMPOSER_CSEF_INITIALIZE) {
		event_mask |= (COMPOSER_CSEF_MAIN_SCROLLER_RESIZE |
					   COMPOSER_CSEF_MAIN_CONTENT_RESIZE |
					   COMPOSER_CSEF_EWK_CONTENT_RESIZE);
		ugd->cs_ready = true;
		debug_log("Ready");
	}

	if (event_mask & COMPOSER_CSEF_DRAG_START) {
		ugd->cs_drag_content_y = ugd->cs_drag_start_y - ugd->cs_top + ugd->cs_scroll_pos;
	}

	if (event_mask & (COMPOSER_CSEF_MAIN_SCROLLER_RESIZE |
					  COMPOSER_CSEF_MAIN_CONTENT_RESIZE |
					  COMPOSER_CSEF_EWK_CONTENT_RESIZE)) {

		int main_content_h = 0;
		int ewk_h = 0;

		int old_edge_scroll_pos = 0;

		evas_object_geometry_get(ugd->composer_layout, NULL, NULL, NULL, &main_content_h);
		evas_object_geometry_get(ugd->ewk_view, NULL, NULL, NULL, &ewk_h);

		if (event_mask & COMPOSER_CSEF_MAIN_SCROLLER_RESIZE) {
			evas_object_geometry_get(ugd->main_scroller, NULL, &ugd->cs_top, &ugd->cs_width, &ugd->cs_height);
			ugd->cs_notify_caret_pos |= (ugd->cs_height < ewk_h);
		}

		if (event_mask & COMPOSER_CSEF_MAIN_CONTENT_RESIZE) {
			if (ugd->richtext_toolbar) {
				evas_object_geometry_get(ugd->richtext_toolbar, NULL, NULL, NULL, &ugd->cs_rttb_height);
			} else {
				ugd->cs_rttb_height = 0;
			}
		}

		if (event_mask & (COMPOSER_CSEF_EWK_CONTENT_RESIZE | COMPOSER_CSEF_MAIN_CONTENT_RESIZE)) {
			ewk_view_scroll_size_get(ugd->ewk_view, NULL, &ugd->cs_max_ewk_scroll_pos);
		}

		old_edge_scroll_pos = ugd->cs_edge_scroll_pos;
		ugd->cs_header_height = main_content_h - ewk_h;
		ugd->cs_edge_scroll_pos = ugd->cs_header_height - ugd->cs_rttb_height;
		ugd->cs_max_scroll_pos = main_content_h - ugd->cs_height + ugd->cs_max_ewk_scroll_pos;

		if (ugd->cs_is_dragging) {
			new_scroll_pos = ugd->cs_drag_content_y - ugd->cs_drag_cur_y + ugd->cs_top;
		} else if ((ugd->cs_scroll_pos > 0) && (ugd->cs_scroll_pos >= old_edge_scroll_pos)) {
			new_scroll_pos = ugd->cs_scroll_pos + ugd->cs_edge_scroll_pos - old_edge_scroll_pos;
		} else {
			new_scroll_pos = ugd->cs_scroll_pos;
		}
		need_set_pos = true;
	}

	if (event_mask & COMPOSER_CSEF_DRAGGING) {
		new_scroll_pos = ugd->cs_drag_content_y - ugd->cs_drag_cur_y + ugd->cs_top;
		need_set_pos = true;
	}

	if (need_set_pos) {
		if ((ecore_time_get() - ugd->cs_backup_pos_time) < CS_CARET_BACKUP_TIMEOUT_SEC) {
			_initial_view_cs_stop_animator(ugd);
			new_scroll_pos = ugd->cs_backup_scroll_pos;
		}
		_initial_view_cs_set_pos(ugd, new_scroll_pos);
	}

	if (ugd->cs_pending_event_mask == 0) {
		if (ugd->cs_notify_caret_pos && elm_object_focus_get(ugd->ewk_btn)) {
			ewk_view_script_execute(ugd->ewk_view, EC_JS_NOTIFY_CARET_POS_CHANGE, NULL, NULL);
		}
		ugd->cs_notify_caret_pos = false;
	}
}

static void _initial_view_cs_sync_with_ewk(EmailComposerUGD *ugd)
{
	int ewk_pos = 0;
	int new_cs_pos = 0;

	ewk_view_scroll_pos_get(ugd->ewk_view, NULL, &ewk_pos);

	new_cs_pos = ugd->cs_main_scroll_pos + ewk_pos;

	if (new_cs_pos != ugd->cs_scroll_pos) {
		ugd->cs_scroll_pos = new_cs_pos;
		debug_log("Combined scroller position was changed due to ewk scroll change. %d", ewk_pos);
	}
}

static void _initial_view_cs_handle_caret_pos_change(EmailComposerUGD *ugd, int top, int bottom)
{
	debug_enter();

	int caret_content_top = 0;
	int caret_content_bottom = 0;

	int caret_scroller_top = 0;
	int caret_scroller_bottom = 0;

	if (ugd->cs_pending_event_mask != 0) {
		debug_log("There are pending events.");
		ugd->cs_notify_caret_pos = true;
		return;
	}

	caret_content_top = ugd->cs_header_height + top;
	caret_content_bottom = ugd->cs_header_height + bottom;

	_initial_view_cs_sync_with_ewk(ugd);

	caret_scroller_top = caret_content_top - ugd->cs_scroll_pos;
	caret_scroller_bottom = caret_content_bottom - ugd->cs_scroll_pos;

	if ((caret_scroller_top <= ugd->cs_rttb_height) || (caret_scroller_bottom >= ugd->cs_height)) {
		int new_scroll_pos = 0;

		if (caret_scroller_top <= ugd->cs_rttb_height) {
			new_scroll_pos = caret_content_top - ugd->cs_rttb_height - CS_CARET_PADDING_PX;
		} else {
			new_scroll_pos = caret_content_bottom - ugd->cs_height + CS_CARET_PADDING_PX;
		}

		if ((ugd->cs_scroll_pos >= ugd->cs_edge_scroll_pos) &&
			(new_scroll_pos < ugd->cs_edge_scroll_pos)) {
			new_scroll_pos = ugd->cs_edge_scroll_pos;
		}

		if (ugd->cs_has_selection || ugd->cs_has_magnifier) {
			composer_initial_view_cs_show(ugd, new_scroll_pos);
		} else {
			composer_initial_view_cs_bring_in(ugd, new_scroll_pos);
		}

	} else if (ugd->cs_bringin_to_ewk && (ugd->cs_scroll_pos < ugd->cs_edge_scroll_pos)) {

		composer_initial_view_cs_bring_in(ugd, ugd->cs_edge_scroll_pos);
	}

	ugd->cs_bringin_to_ewk = false;

	debug_leave();
}

static void _initial_view_cs_ensure_ewk_on_top(EmailComposerUGD *ugd, bool force_bring_in)
{
	debug_enter();

	if (ugd->cs_scroll_pos < ugd->cs_edge_scroll_pos) {
		if (force_bring_in) {
			int freeze_count = ugd->cs_freeze_count;
			ugd->cs_freeze_count = 0;
			composer_initial_view_cs_bring_in(ugd, ugd->cs_edge_scroll_pos);
			ugd->cs_freeze_count = freeze_count;
		} else {
			composer_initial_view_cs_show(ugd, ugd->cs_edge_scroll_pos);
		}
	}
}

static void _initial_view_cs_backup_scroll_pos(EmailComposerUGD *ugd)
{
	debug_enter();

	ugd->cs_backup_scroll_pos = ugd->cs_scroll_pos;
	ugd->cs_backup_pos_time = ecore_time_get();
}

static Eina_Bool _initial_view_cs_event_timer1_cb(void *data)
{
	EmailComposerUGD *ugd = data;

	ugd->cs_pending_event_mask &= ~COMPOSER_CSEF_EWK_CONTENT_RESIZE;
	_initial_view_cs_update(ugd, COMPOSER_CSEF_EWK_CONTENT_RESIZE);
	ugd->cs_events_timer1 = NULL;

	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _initial_view_cs_event_timer2_cb(void *data)
{
	EmailComposerUGD *ugd = data;

	ugd->cs_pending_event_mask &= ~COMPOSER_CSEF_INITIALIZE;
	_initial_view_cs_update(ugd, COMPOSER_CSEF_INITIALIZE);
	ugd->cs_events_timer2 = NULL;

	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _initial_view_cs_animator_cb(void *data, double pos)
{
	EmailComposerUGD *ugd = data;

	int new_pos = 0;
	float t = pos * ugd->cs_anim_duration;

	ugd->cs_anim_t = t;

	if (t <= ugd->cs_anim_t1) {
		new_pos = (int)(ugd->cs_anim_pos0 + ugd->cs_anim_v0 * t + 0.5f * ugd->cs_anim_a1 * t * t + 0.5f);
	} else {
		t -= ugd->cs_anim_t1;
		new_pos = (int)(ugd->cs_anim_pos1 + ugd->cs_anim_v1 * t + 0.5f * ugd->cs_anim_a2 * t * t + 0.5f);
	}

	_initial_view_cs_set_pos(ugd, new_pos);

	if (new_pos != ugd->cs_scroll_pos) {
		ugd->cs_animator = NULL;
		debug_log("End reached.");
		return ECORE_CALLBACK_CANCEL;
	}

	if (pos == 1.0) {
		ugd->cs_animator = NULL;
		debug_log("Animator finished.");
		return ECORE_CALLBACK_CANCEL;
	}

	return ECORE_CALLBACK_RENEW;
}

static Evas_Object *_initial_view_create_combined_scroller_layout(Evas_Object *parent)
{
	debug_enter();

	Evas_Object *layout = elm_layout_add(parent);
	elm_layout_file_set(layout, email_get_composer_theme_path(), "ec/combined_scroller/base");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	edje_object_signal_emit(layout, "load", ""); /* initialize scroller */
	evas_object_show(layout);

	debug_leave();
	return layout;
}

static void _initial_view_main_scroller_resize_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailComposerUGD *ugd = data;

	_initial_view_cs_handle_event(ugd, COMPOSER_CSEF_MAIN_SCROLLER_RESIZE);
}

static void _initial_view_main_scroller_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailComposerUGD *ugd = data;
	Evas_Event_Mouse_Down *event = event_info;

	ugd->allow_click_events = EINA_TRUE;

	if (ugd->cs_ready && !ugd->cs_is_sliding && (ugd->cs_freeze_count == 0)) {
		ugd->cs_is_sliding = true;
		ugd->cs_drag_down_y = event->canvas.y;
		ugd->cs_drag_cur_y = event->canvas.y;
		ugd->cs_drag_cur_time = event->timestamp;

		_initial_view_cs_stop_animator(ugd);
	}
}

static void _initial_view_main_scroller_mouse_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	EmailComposerUGD *ugd = data;
	Evas_Event_Mouse_Move *event = event_info;

	int event_mask = 0;

	if (!ugd->cs_is_sliding) {
		return;
	}

	if (!ugd->cs_is_dragging) {
		if (abs(event->cur.canvas.y - ugd->cs_drag_down_y) > CS_DRAG_THRESHOLD_PX) {
			ugd->cs_is_dragging = true;
			ugd->cs_drag_start_y = ugd->cs_drag_cur_y;
			event_mask |= COMPOSER_CSEF_DRAG_START;
		} else {
			ugd->cs_drag_cur_y = event->cur.canvas.y;
			ugd->cs_drag_cur_time = event->timestamp;
			return;
		}
	}

	ugd->cs_anim_v0 = -1000.0f * (event->cur.canvas.y - ugd->cs_drag_cur_y) / (event->timestamp - ugd->cs_drag_cur_time);
	event_mask |= COMPOSER_CSEF_DRAGGING;

	ugd->cs_drag_cur_y = event->cur.canvas.y;
	ugd->cs_drag_cur_time = event->timestamp;

	_initial_view_cs_handle_event(ugd, event_mask);
}

static void _initial_view_main_scroller_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailComposerUGD *ugd = data;

	ugd->cs_is_sliding = false;

	if (ugd->cs_is_dragging) {
		ugd->allow_click_events = EINA_FALSE;
		ugd->cs_is_dragging = false;
		ugd->cs_anim_pos0 = (float)ugd->cs_scroll_pos;
		ugd->cs_anim_a1 = ((ugd->cs_anim_v0 > 0.0f) ? -CS_DRAG_ACCEL : CS_DRAG_ACCEL);
		ugd->cs_anim_t1 = -ugd->cs_anim_v0 / ugd->cs_anim_a1;
		ugd->cs_anim_duration = ugd->cs_anim_t1;
		ugd->cs_animator = ecore_animator_timeline_add(ugd->cs_anim_duration, _initial_view_cs_animator_cb, ugd);
	}
}

static void _initial_view_composer_layout_resize_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailComposerUGD *ugd = data;

	_initial_view_cs_handle_event(ugd, COMPOSER_CSEF_MAIN_CONTENT_RESIZE);
}

static void _initial_view_main_scroller_scroll_cb(void *data, Evas_Object *obj, void *event_info)
{
	EmailComposerUGD *ugd = data;

	int main_scroll_pos = 0;

	elm_scroller_region_get(obj, NULL, &main_scroll_pos, NULL, NULL);

	if (ugd->cs_main_scroll_pos != main_scroll_pos) {
		debug_log("Main scroller was changed! Prevent scroll change.");

		_initial_view_cs_sync_with_ewk(ugd);
		_initial_view_cs_set_pos(ugd, ugd->cs_scroll_pos);

		if (!ugd->cs_animator) {
			int new_cs_scroll_pos = 0;

			if (main_scroll_pos < ugd->cs_edge_scroll_pos) {
				new_cs_scroll_pos = main_scroll_pos;
			} else {
				int ewk_pos = 0;
				ewk_view_scroll_pos_get(ugd->ewk_view, NULL, &ewk_pos);
				new_cs_scroll_pos = main_scroll_pos + ewk_pos;
			}

			composer_initial_view_cs_bring_in(ugd, new_cs_scroll_pos);
		}
	}
}

static void _initial_view_magnifier_show_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailComposerUGD *ugd = data;

	ugd->cs_has_magnifier = true;

	composer_initial_view_cs_freeze_push(ugd);

	if (ugd->cs_has_selection) {
		_initial_view_cs_ensure_ewk_on_top(ugd, true);
	}
}

static void _initial_view_magnifier_hide_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailComposerUGD *ugd = data;

	ugd->cs_has_magnifier = false;

	composer_initial_view_cs_freeze_pop(ugd);
}

static void _initial_view_ewk_content_resize_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailComposerUGD *ugd = data;

	_initial_view_cs_handle_event(ugd, COMPOSER_CSEF_EWK_CONTENT_RESIZE);
}

/*
 * Definition for exported functions
 */

void composer_initial_view_back_cb(EmailComposerUGD *ugd)
{
	debug_enter();
	retm_if(!ugd, "Invalid parameter: ugd is NULL!");

	Eina_Bool is_selection_removed = ewk_view_text_selection_clear(ugd->ewk_view);
	retm_if(is_selection_removed, "ewk_view_text_selection_clear() result TRUE");

	_initial_view_back_button_clicked_cb(ugd, NULL, NULL);

	debug_leave();
}

void composer_initial_view_draw_base_frame(EmailComposerUGD *ugd)
{
	email_profiling_begin(composer_initial_view_draw_base_frame);
	debug_enter();

	retm_if(!ugd, "Invalid parameter: ugd is NULL!");

	Evas_Object *layout = NULL;
	Evas_Object *scroller = NULL;

	ugd->base.content = _initial_view_create_root_layout(ugd->base.module->navi, ugd);
	scroller = composer_util_create_scroller(ugd->base.module->navi);
	layout = _initial_view_create_composer_layout(ugd->base.module->navi);

	elm_object_part_content_set(ugd->base.content, "ec.swallow.content", scroller);
	elm_object_content_set(scroller, layout);
	_initial_view_create_naviframe_buttons(ugd->base.module->navi, ugd->base.content, ugd);

	ugd->main_scroller = scroller;
	ugd->composer_layout = layout;

	debug_leave();
	email_profiling_end(composer_initial_view_draw_base_frame);
}

void composer_initial_view_draw_richtext_components(void *data)
{
	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	ugd->richtext_toolbar = composer_rich_text_create_toolbar(ugd);
	composer_rich_text_disable_set(ugd, EINA_TRUE);
}

void composer_initial_view_draw_header_components(void *data)
{
	email_profiling_begin(composer_initial_view_draw_header_components);
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	Evas_Object *box = _initial_view_create_composer_box(ugd->main_scroller);
	elm_object_part_content_set(ugd->composer_layout, "ec.swallow.box", box);

	if (ugd->account_info && (ugd->account_info->account_count > 1) && (ugd->eComposerErrorType == COMPOSER_ERROR_NONE)) {
		Evas_Object *from_layout = composer_recipient_create_layout(ugd, box);
		elm_box_pack_end(box, from_layout);
		ugd->recp_from_layout = from_layout;
	}

	Evas_Object *to_layout = composer_recipient_create_layout(ugd, box);
	Evas_Object *cc_layout = composer_recipient_create_layout(ugd, box);
	Evas_Object *bcc_layout = composer_recipient_create_layout(ugd, box);
	Evas_Object *subject_layout = composer_subject_create_layout(box);

	elm_box_pack_end(box, to_layout);
	elm_box_pack_end(box, subject_layout);

	evas_object_hide(cc_layout);
	evas_object_hide(bcc_layout);

	evas_object_show(box);

	ugd->composer_box = box;
	ugd->recp_to_layout = to_layout;
	ugd->recp_cc_layout = cc_layout;
	ugd->recp_bcc_layout = bcc_layout;
	ugd->subject_layout = subject_layout;

	debug_leave();
	email_profiling_end(composer_initial_view_draw_header_components);
}

void composer_initial_view_draw_remaining_components(void *data)
{
	email_profiling_begin(composer_initial_view_draw_remaining_components);
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	if (ugd->composer_box) {
		if (ugd->recp_from_layout) {
			composer_recipient_update_from_detail(ugd, ugd->recp_from_layout);
		}
		composer_recipient_update_to_detail(ugd, ugd->recp_to_layout);
		if (ugd->composer_type != RUN_COMPOSER_NEW) {
			composer_recipient_update_cc_detail(ugd, ugd->recp_cc_layout);
			composer_recipient_update_bcc_detail(ugd, ugd->recp_bcc_layout);
		}

#ifdef _ALWAYS_CC_MYSELF
		if ((ugd->composer_type != RUN_COMPOSER_EDIT) && ugd->account_info && ugd->account_info->original_account &&
			(ugd->account_info->original_account->options.add_my_address_to_bcc != EMAIL_ADD_MY_ADDRESS_OPTION_DO_NOT_ADD)) {
			if (!ugd->bcc_added) {
				composer_recipient_show_hide_bcc_field(ugd, EINA_TRUE);
			}
			composer_recipient_append_myaddress(ugd, ugd->account_info->original_account);
		}
#endif
		composer_subject_update_detail(ugd, ugd->subject_layout);
	}

	composer_webkit_create_body_field(ugd->composer_layout, ugd);

	debug_leave();
	email_profiling_end(composer_initial_view_draw_remaining_components);
}

void composer_initial_view_create_combined_scroller(void *data)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	Evas_Object *combined_sc_ly = _initial_view_create_combined_scroller_layout(ugd->base.module->navi);
	elm_object_part_content_set(ugd->base.content, "ec.swallow.scroller", combined_sc_ly);
	ugd->combined_scroller = combined_sc_ly;
	evas_object_raise(combined_sc_ly);

	/* set callbacks for scrolling */

	evas_object_event_callback_add(ugd->main_scroller, EVAS_CALLBACK_RESIZE, _initial_view_main_scroller_resize_cb, ugd);
	evas_object_event_callback_add(ugd->main_scroller, EVAS_CALLBACK_MOUSE_DOWN, _initial_view_main_scroller_down_cb, ugd);
	evas_object_event_callback_add(ugd->main_scroller, EVAS_CALLBACK_MOUSE_MOVE, _initial_view_main_scroller_mouse_move_cb, ugd);
	evas_object_event_callback_add(ugd->main_scroller, EVAS_CALLBACK_MOUSE_UP, _initial_view_main_scroller_up_cb, ugd);

	evas_object_event_callback_add(ugd->composer_layout, EVAS_CALLBACK_RESIZE, _initial_view_composer_layout_resize_cb, ugd);

	evas_object_smart_callback_add(ugd->main_scroller, "scroll", _initial_view_main_scroller_scroll_cb, ugd);

	evas_object_smart_callback_add(ugd->ewk_view, "magnifier,show", _initial_view_magnifier_show_cb, ugd);
	evas_object_smart_callback_add(ugd->ewk_view, "magnifier,hide", _initial_view_magnifier_hide_cb, ugd);
	evas_object_smart_callback_add(ugd->ewk_view, "contents,size,changed", _initial_view_ewk_content_resize_cb, ugd);

	_initial_view_cs_handle_event(ugd, COMPOSER_CSEF_INITIALIZE);

	debug_leave();
	return;
}

void composer_initial_view_set_combined_scroller_rotation_mode(void *data)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	if (ugd->selected_entry == ugd->ewk_view) {
		_initial_view_cs_ensure_ewk_on_top(ugd, false);
		_initial_view_cs_backup_scroll_pos(ugd);
	}

	if (ugd->is_horizontal) {
		edje_object_signal_emit(_EDJ(ugd->combined_scroller), "scroller_landscape", "");
	} else {
		edje_object_signal_emit(_EDJ(ugd->combined_scroller), "scroller_portrait", "");
	}

	composer_util_resize_webview_height(ugd);

	debug_leave();
}

void composer_initial_view_caret_pos_changed_cb(EmailComposerUGD *ugd, int top, int bottom, bool isCollapsed)
{
	if (isCollapsed) {
		ugd->cs_has_selection = false;
		if (!ugd->cs_has_magnifier) {
			ugd->cs_in_selection_mode = false;
		}
	} else {
		ugd->cs_has_selection = true;
	}

	_initial_view_cs_handle_caret_pos_change(ugd, top, bottom);
}

void composer_initial_view_cs_bring_in(EmailComposerUGD *ugd, int pos)
{
	debug_enter();
	retm_if(!ugd->cs_ready, "Not ready!");

	// (a1 - a1^2/a2)*t1^2 + (2*V0 - 2*V0*a1/a2)*t1 + (2*x0 - V0^2/a2 - 2*x) = 0
	// t2 = -V0/a2 - a1*t1/a2

	float a1 = 0.0f;
	float a2 = 0.0f;

	float t1 = 0.0f;
	float v1 = 0.0f;
	float duration = 0.0f;

	float pos0 = 0.0f;
	float pos2 = 0.0f;
	float delta = 0.0f;
	float v0 = 0.0f;
	float adjust = 0.0f;
	float a_scale = 0.0f;

	if (ugd->cs_freeze_count > 0) {
		debug_warning("Freezed!");
		return;
	}

	if (ugd->cs_is_sliding) {
		debug_log("User is sliding over!");
		return;
	}

	pos2 = (float)_initial_view_cs_fix_pos(ugd, pos);

	if (ugd->cs_animator) {
		if (pos2 == ugd->cs_anim_pos2) {
			debug_log("No need to bring in.");
			return;
		}

		v0 = ((ugd->cs_anim_t <= ugd->cs_anim_t1) ?
				(ugd->cs_anim_v0 + ugd->cs_anim_a1 * ugd->cs_anim_t) :
				(ugd->cs_anim_v1 + ugd->cs_anim_a2 * (ugd->cs_anim_t - ugd->cs_anim_t1)));

		adjust = fabsf(v0 * CS_BRING_ADJUST_TIME);

		_initial_view_cs_stop_animator(ugd);
	}

	_initial_view_cs_sync_with_ewk(ugd);

	pos0 = (float)ugd->cs_scroll_pos;
	delta = pos2 - pos0;

	a_scale = delta + (delta > 0.0f ? adjust : -adjust);
	if (a_scale == 0.0f) {
		debug_log("a_scale is 0.");
		return;
	}

	a1 = CS_BRING_REL_ACCEL1 * a_scale;
	a2 = -CS_BRING_REL_ACCEL2 * a_scale;

	{
		float tmp = 0.0f;
		float a2_inv = 1.0f / a2;

		int i = 0;
		for (; i < 2; ++i) {
			float k = 1.0f - a1 * a2_inv;
			float a = a1 * k;
			float b = 2.0f * v0 * k;
			float c = -2.0f * delta - v0 * v0 * a2_inv;
			float d = b * b - 4.0f * a * c;

			if (a1 > 0.0) {
				t1 = (-b + sqrtf(d)) / (2.0f * a);
			} else {
				t1 = (-b - sqrtf(d)) / (2.0f * a);
			}

			if (t1 >= 0.0f) {
				break;
			}

			tmp = a1;
			a1 = a2;
			a2 = tmp;
			a2_inv = 1.0f / a2;
		}

		v1 = v0 + a1 * t1;
		duration = t1 - v1 * a2_inv;
	}

	ugd->cs_anim_pos0 = pos0;
	ugd->cs_anim_pos1 = pos0 + v0 * t1 + 0.5f * a1 * t1 * t1;
	ugd->cs_anim_pos2 = pos2;
	ugd->cs_anim_v0 = v0;
	ugd->cs_anim_v1 = v1;
	ugd->cs_anim_a1 = a1;
	ugd->cs_anim_a2 = a2;
	ugd->cs_anim_t1 = t1;
	ugd->cs_anim_duration = duration;
	ugd->cs_anim_t = 0.0f;

	ugd->cs_animator = ecore_animator_timeline_add(duration, _initial_view_cs_animator_cb, ugd);
}

void composer_initial_view_cs_show(EmailComposerUGD *ugd, int pos)
{
	debug_enter();
	retm_if(!ugd->cs_ready, "Not ready!");

	_initial_view_cs_stop_animator(ugd);
	_initial_view_cs_set_pos(ugd, pos);
}

void composer_initial_view_cs_freeze_push(EmailComposerUGD *ugd)
{
	debug_enter();
	retm_if(!ugd->cs_ready, "Not ready!");

	++ugd->cs_freeze_count;

	_initial_view_cs_stop_dragging(ugd);
	_initial_view_cs_stop_animator(ugd);
}

void composer_initial_view_cs_freeze_pop(EmailComposerUGD *ugd)
{
	debug_enter();
	retm_if(!ugd->cs_ready, "Not ready!");

	if (ugd->cs_freeze_count > 0) {
		--ugd->cs_freeze_count;
	}
}

void composer_initial_view_activate_selection_mode(EmailComposerUGD *ugd)
{
	debug_enter();
	retm_if(!ugd->cs_ready, "Not ready!");

	if (!ugd->cs_in_selection_mode) {
		ugd->cs_in_selection_mode = true;
		_initial_view_cs_ensure_ewk_on_top(ugd, true);
	}
}
