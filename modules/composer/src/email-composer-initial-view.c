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
#include "email-composer-predictive-search.h"
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

static Eina_Bool _initial_view_exit_idler_cb(void *data);

static Evas_Object *_initial_view_create_root_layout(Evas_Object *parent, EmailComposerView *view);
static Evas_Object *_initial_view_create_composer_layout(Evas_Object *parent);
static void _initial_view_create_naviframe_buttons(Evas_Object *parent, Evas_Object *content, EmailComposerView *view);

static void _initial_view_update_rttb_position(EmailComposerView *view);
static void _initial_view_notify_ewk_visible_area(EmailComposerView *view);

static void _initial_view_cs_begin_scroll(EmailComposerView *view);
static void _initial_view_cs_end_scroll(EmailComposerView *view);

static void _initial_view_cs_stop_animator(EmailComposerView *view);
static void _initial_view_cs_stop_dragging(EmailComposerView *view);
static void _initial_view_cs_stop_all(EmailComposerView *view);
static int _initial_view_cs_fix_pos(EmailComposerView *view, int y);
static void _initial_view_cs_set_pos(EmailComposerView *view, int y);
static void _initial_view_cs_handle_event(EmailComposerView *view, int event_mask);
static void _initial_view_cs_update(EmailComposerView *view, int event_mask);
static void _initial_view_cs_sync_with_ewk(EmailComposerView *view);

static void _initial_view_cs_handle_caret_pos_change(EmailComposerView *view, int top, int bottom);
static void _initial_view_cs_ensure_ewk_on_top(EmailComposerView *view, bool force_bring_in);
static void _initial_view_cs_backup_scroll_pos(EmailComposerView *view);

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

	EmailComposerView *view = (EmailComposerView *)data;

	/* To prevent buttons on naviframe being processed while launching composer. this flag is set after executing all js via webkit. */
	retm_if(!view->is_load_finished, "_initial_view_back_button_clicked_cb() return; view->is_load_finished = false");

	/* To prevent B/S. If a user clicks hardware back key or 'X' button on the navi bar while launching other applications(like camera via attach files), it'll cause B/S.
	 * Because result callback for the launching request is called after the composer is destroyed.
	 */
	retm_if(view->base.module->is_launcher_busy, "is_launcher_busy = true");
	retm_if(view->is_back_btn_clicked || view->is_save_in_drafts_clicked || view->is_send_btn_clicked, "while destroying composer!");

	Ecore_IMF_Context *ctx = ecore_imf_context_add(ecore_imf_context_default_id_get());
	Ecore_IMF_Input_Panel_State imf_state = ecore_imf_context_input_panel_state_get(ctx);
	ecore_imf_context_del(ctx);

	debug_log("imf state ==> %d", imf_state);

	view->is_back_btn_clicked = EINA_TRUE;
	if (imf_state != ECORE_IMF_INPUT_PANEL_STATE_SHOW) {
		composer_exit_composer_get_contents(view);
	} else {
		DELETE_IDLER_OBJECT(view->idler_destroy_self);
		view->idler_destroy_self = ecore_idler_add(_initial_view_exit_idler_cb, view);
	}

	debug_leave();
}

static void _initial_view_send_button_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");
	retm_if(!obj, "Invalid parameter: obj is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	/* To prevent buttons on naviframe being processed while launching composer. this flag is set after executing all js via webkit. */
	retm_if(!view->is_load_finished, "_initial_view_send_button_clicked_cb() return; view->is_load_finished = false");

	/* To prevent B/S. If a user clicks 'send' button on the navi bar while launching other applications(like camera via attach files), it'll cause B/S.
	 * Because result callback for the launching request is called after the composer is destroyed.
	 */
	retm_if(view->base.module->is_launcher_busy, "is_launcher_busy = true");
	retm_if(view->is_back_btn_clicked || view->is_save_in_drafts_clicked || view->is_send_btn_clicked, "while destroying composer!");

	view->is_send_btn_clicked = EINA_TRUE;
	composer_exit_composer_get_contents(view);

	debug_leave();
}

static Eina_Bool _initial_view_exit_idler_cb(void *data)
{
	debug_enter();
	retvm_if(!data, ECORE_CALLBACK_CANCEL, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	view->idler_destroy_self = NULL;

	composer_exit_composer_get_contents(view);

	return ECORE_CALLBACK_CANCEL;
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

static Evas_Object *_initial_view_create_root_layout(Evas_Object *parent, EmailComposerView *view)
{
	debug_enter();

	retvm_if(!parent, NULL, "Invalid parameter: parent is NULL!");

	Evas_Object *layout = elm_layout_add(parent);
	retvm_if(!layout, NULL, "elm_layout_add() failed!");

	elm_layout_file_set(layout, email_get_composer_theme_path(), "ec/layout/root");
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(layout);

	if (view->theme) {
		elm_object_theme_set(layout, view->theme);
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

static void _initial_view_create_naviframe_buttons(Evas_Object *parent, Evas_Object *content, EmailComposerView *view)
{
	debug_enter();

	email_profiling_begin(_initial_view_create_naviframe_buttons);

	retm_if(!parent, "Invalid parameter: parent is NULL!");

	Elm_Object_Item *navi_item = email_module_view_push(&view->base, "IDS_EMAIL_HEADER_COMPOSE_ABB", 0);

	elm_object_item_domain_text_translatable_set(navi_item, PACKAGE, EINA_TRUE);

	Evas_Object *more_btn = elm_button_add(parent);
	elm_object_style_set(more_btn, "naviframe/more/default");
	elm_object_item_part_content_set(navi_item, "toolbar_more_btn", more_btn);
	evas_object_smart_callback_add(more_btn, "clicked", composer_more_menu_clicked_cb, view);

	Evas_Object *cancel_btn = elm_button_add(parent);
	elm_object_style_set(cancel_btn, "naviframe/title_left");
	elm_object_item_part_content_set(navi_item, "title_left_btn", cancel_btn);
	elm_object_domain_translatable_text_set(cancel_btn, PACKAGE, "IDS_TPLATFORM_ACBUTTON_CANCEL_ABB");
	evas_object_smart_callback_add(cancel_btn, "clicked", _initial_view_back_button_clicked_cb, view);

	Evas_Object *send_btn = elm_button_add(parent);
	elm_object_style_set(send_btn, "naviframe/title_right");
	elm_object_item_part_content_set(navi_item, "title_right_btn", send_btn);
	elm_object_domain_translatable_text_set(send_btn, PACKAGE, "IDS_TPLATFORM_ACBUTTON_SEND_ABB");
	evas_object_smart_callback_add(send_btn, "clicked", _initial_view_send_button_clicked_cb, view);
	elm_object_disabled_set(send_btn, EINA_TRUE);

	view->send_btn = send_btn;

	email_profiling_end(_initial_view_create_naviframe_buttons);

	debug_leave();
}

static void _initial_view_update_rttb_position(EmailComposerView *view)
{
	if (!view->rttb_placeholder && !view->richtext_toolbar) {
		return;
	}

	if (view->rttb_placeholder) {
		if (view->cs_main_scroll_pos < view->cs_edge_scroll_pos) {
			debug_log("Unpin from top.");

			DELETE_EVAS_OBJECT(view->rttb_placeholder);

			elm_object_part_content_unset(view->base.content, "ec.swallow.richtext");
			elm_box_pack_end(view->composer_box, view->richtext_toolbar);
		}
	} else {
		if (view->cs_main_scroll_pos >= view->cs_edge_scroll_pos) {
			debug_log("Pin to top.");

			Evas_Object *bg = NULL;
			int rttb_h = 0;

			evas_object_geometry_get(view->richtext_toolbar, NULL, NULL, NULL, &rttb_h);

			elm_box_unpack(view->composer_box, view->richtext_toolbar);
			elm_object_part_content_set(view->base.content, "ec.swallow.richtext", view->richtext_toolbar);

			bg = elm_bg_add(view->composer_box);
			elm_box_pack_end(view->composer_box, bg);
			evas_object_size_hint_min_set(bg, 0, rttb_h);
			evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, 0);
			evas_object_size_hint_align_set(bg, EVAS_HINT_FILL, 0);
			evas_object_show(bg);

			view->rttb_placeholder = bg;
		}
	}
}

static void _initial_view_notify_ewk_visible_area(EmailComposerView *view)
{
	Eina_Rectangle rect = { 0 };
	int min_y = view->cs_top + view->cs_rttb_height;

	rect.x = 0;
	rect.w = view->cs_width;

	rect.y = view->cs_top + view->cs_header_height - view->cs_scroll_pos;
	if (rect.y < min_y) {
		rect.y = min_y;
	}

	rect.h = view->cs_top + view->cs_height - rect.y;
	if (rect.h < 0) {
		rect.h = 0;
	}

	evas_object_smart_callback_call(view->ewk_view, "visible,content,changed", &rect);
}

static void _initial_view_cs_begin_scroll(EmailComposerView *view)
{
	if (view->cs_is_scrolling) {
		return;
	}
	view->cs_is_scrolling = true;

	debug_log("COMBINED_SCROLL BEGIN");
	evas_object_smart_callback_call(view->ewk_view, "custom,scroll,begin", NULL);
}

static void _initial_view_cs_end_scroll(EmailComposerView *view)
{
	if (!view->cs_is_scrolling) {
		return;
	}
	view->cs_is_scrolling = false;

	debug_log("COMBINED_SCROLL END");
	evas_object_smart_callback_call(view->ewk_view, "custom,scroll,end", NULL);
}

static void _initial_view_cs_stop_animator(EmailComposerView *view)
{
	debug_enter();

	if (view->cs_animator) {
		ecore_animator_del(view->cs_animator);
		view->cs_animator = NULL;
	}
}

static void _initial_view_cs_stop_dragging(EmailComposerView *view)
{
	view->cs_is_sliding = false;
	view->cs_is_dragging = false;
}

static void _initial_view_cs_stop_all(EmailComposerView *view)
{
	_initial_view_cs_stop_animator(view);
	_initial_view_cs_stop_dragging(view);
	_initial_view_cs_end_scroll(view);
}

static int _initial_view_cs_fix_pos(EmailComposerView *view, int y)
{
	int result = 0;

	if (y < 0) {
		result = 0;
	} else if (y > view->cs_max_scroll_pos) {
		result = view->cs_max_scroll_pos;
	} else {
		result = y;
	}

	return result;
}

static void _initial_view_cs_set_pos(EmailComposerView *view, int y)
{
	int ewk_h = 0;
	int edge_pos = 0;
	int ewk_scroll_pos = 0;

	if (!view->cs_ready) {
		return;
	}

	view->cs_scroll_pos = _initial_view_cs_fix_pos(view, y);

	evas_object_geometry_get(view->ewk_view, NULL, NULL, NULL, &ewk_h);

	edge_pos = (ewk_h - view->cs_height - view->cs_rttb_height) / 2 + view->cs_header_height;

	if (view->cs_scroll_pos > edge_pos) {
		ewk_scroll_pos = view->cs_scroll_pos - edge_pos;
		if (ewk_scroll_pos > view->cs_max_ewk_scroll_pos) {
			ewk_scroll_pos = view->cs_max_ewk_scroll_pos;
			view->cs_main_scroll_pos = view->cs_scroll_pos - ewk_scroll_pos;
		} else {
			view->cs_main_scroll_pos = edge_pos;
		}
	} else {
		view->cs_main_scroll_pos = view->cs_scroll_pos;
		ewk_scroll_pos = 0;
	}

	elm_scroller_region_show(view->main_scroller, 0, view->cs_main_scroll_pos, view->cs_width, view->cs_height);
	ewk_view_scroll_set(view->ewk_view, 0, ewk_scroll_pos);

	edje_object_part_drag_size_set(_EDJ(view->combined_scroller), "elm.dragable.vbar",
			1.0, 1.0 * view->cs_height / (view->cs_height + view->cs_max_scroll_pos));
	edje_object_part_drag_value_set(_EDJ(view->combined_scroller), "elm.dragable.vbar",
			0.0, 1.0 * view->cs_scroll_pos / view->cs_max_scroll_pos);
	edje_object_signal_emit(_EDJ(view->combined_scroller), "elm,action,scroll_v", "elm");

	_initial_view_update_rttb_position(view);
	_initial_view_notify_ewk_visible_area(view);
}

static void _initial_view_cs_handle_event(EmailComposerView *view, int event_mask)
{
	if (event_mask & COMPOSER_CSEF_IMMEDIATE_EVENTS) {
		if (view->cs_immediate_event_mask != 0) {
			debug_log("Recursion.");
			view->cs_immediate_event_mask |= event_mask;
			return;
		}
		view->cs_immediate_event_mask = event_mask;
		if (event_mask & (COMPOSER_CSEF_IMMEDIATE_EVENTS & ~COMPOSER_CSEF_DRAG_EVENTS)) {
			evas_smart_objects_calculate(evas_object_evas_get(view->composer_layout));
		}
		_initial_view_cs_update(view, view->cs_immediate_event_mask);
		view->cs_immediate_event_mask = 0;
	}

	if (event_mask & COMPOSER_CSEF_PENDING_EVENTS) {
		if (event_mask & COMPOSER_CSEF_EWK_CONTENT_RESIZE) {
			DELETE_TIMER_OBJECT(view->cs_events_timer1);
			view->cs_events_timer1 = ecore_timer_add(CS_UPDATE_TIMEOUT_SEC, _initial_view_cs_event_timer1_cb, view);
		}
		if (event_mask & COMPOSER_CSEF_INITIALIZE) {
			DELETE_TIMER_OBJECT(view->cs_events_timer2);
			view->cs_events_timer2 = ecore_timer_add(CS_INITIALIZE_TIMEOUT_SEC, _initial_view_cs_event_timer2_cb, view);
		}
		view->cs_pending_event_mask |= (event_mask & COMPOSER_CSEF_PENDING_EVENTS);
	}
}

static void _initial_view_cs_update(EmailComposerView *view, int event_mask)
{
	int new_scroll_pos = 0;
	bool need_set_pos = false;

	_initial_view_cs_sync_with_ewk(view);

	if (event_mask & COMPOSER_CSEF_INITIALIZE) {
		event_mask |= (COMPOSER_CSEF_MAIN_SCROLLER_RESIZE |
					   COMPOSER_CSEF_MAIN_CONTENT_RESIZE |
					   COMPOSER_CSEF_EWK_CONTENT_RESIZE);
		view->cs_ready = true;
		debug_log("Ready");
	}

	if (event_mask & COMPOSER_CSEF_DRAG_START) {
		view->cs_drag_content_y = view->cs_drag_start_y - view->cs_top + view->cs_scroll_pos;
	}

	if (event_mask & (COMPOSER_CSEF_MAIN_SCROLLER_RESIZE |
					  COMPOSER_CSEF_MAIN_CONTENT_RESIZE |
					  COMPOSER_CSEF_EWK_CONTENT_RESIZE)) {

		int main_content_h = 0;
		int ewk_h = 0;

		int old_edge_scroll_pos = 0;

		evas_object_geometry_get(view->composer_layout, NULL, NULL, NULL, &main_content_h);
		evas_object_geometry_get(view->ewk_view, NULL, NULL, NULL, &ewk_h);

		if (event_mask & COMPOSER_CSEF_MAIN_SCROLLER_RESIZE) {
			evas_object_geometry_get(view->main_scroller, NULL, &view->cs_top, &view->cs_width, &view->cs_height);
			view->cs_notify_caret_pos |= (view->cs_height < ewk_h);
		}

		if (event_mask & COMPOSER_CSEF_MAIN_CONTENT_RESIZE) {
			if (view->richtext_toolbar) {
				evas_object_geometry_get(view->richtext_toolbar, NULL, NULL, NULL, &view->cs_rttb_height);
			} else {
				view->cs_rttb_height = 0;
			}
		}

		if (event_mask & (COMPOSER_CSEF_EWK_CONTENT_RESIZE | COMPOSER_CSEF_MAIN_CONTENT_RESIZE)) {
			ewk_view_scroll_size_get(view->ewk_view, NULL, &view->cs_max_ewk_scroll_pos);
		}

		old_edge_scroll_pos = view->cs_edge_scroll_pos;
		view->cs_header_height = main_content_h - ewk_h;
		view->cs_edge_scroll_pos = view->cs_header_height - view->cs_rttb_height;
		view->cs_max_scroll_pos = main_content_h - view->cs_height + view->cs_max_ewk_scroll_pos;

		if (view->cs_is_dragging) {
			new_scroll_pos = view->cs_drag_content_y - view->cs_drag_cur_y + view->cs_top;
		} else if ((view->cs_scroll_pos > 0) && (view->cs_scroll_pos >= old_edge_scroll_pos)) {
			new_scroll_pos = view->cs_scroll_pos + view->cs_edge_scroll_pos - old_edge_scroll_pos;
		} else {
			new_scroll_pos = view->cs_scroll_pos;
		}
		need_set_pos = true;
	}

	if (event_mask & COMPOSER_CSEF_DRAGGING) {
		new_scroll_pos = view->cs_drag_content_y - view->cs_drag_cur_y + view->cs_top;
		need_set_pos = true;
	}

	if (need_set_pos) {
		if ((ecore_time_get() - view->cs_backup_pos_time) < CS_CARET_BACKUP_TIMEOUT_SEC) {
			new_scroll_pos = view->cs_backup_scroll_pos;
		}
		_initial_view_cs_set_pos(view, new_scroll_pos);
	}

	if (view->cs_pending_event_mask == 0) {
		if (view->cs_notify_caret_pos && elm_object_focus_get(view->ewk_btn)) {
			ewk_view_script_execute(view->ewk_view, EC_JS_NOTIFY_CARET_POS_CHANGE, NULL, NULL);
		}
		view->cs_notify_caret_pos = false;
	}

	if (view->ps_layout && (event_mask & COMPOSER_CSEF_MAIN_SCROLLER_RESIZE)) {
		composer_ps_change_layout_size(view);
	}
}

static void _initial_view_cs_sync_with_ewk(EmailComposerView *view)
{
	int ewk_pos = 0;
	int new_cs_pos = 0;

	ewk_view_scroll_pos_get(view->ewk_view, NULL, &ewk_pos);

	new_cs_pos = view->cs_main_scroll_pos + ewk_pos;

	if (new_cs_pos != view->cs_scroll_pos) {
		view->cs_scroll_pos = new_cs_pos;
		debug_log("Combined scroller position was changed due to ewk scroll change. %d", ewk_pos);
	}
}

static void _initial_view_cs_handle_caret_pos_change(EmailComposerView *view, int top, int bottom)
{
	debug_enter();

	int caret_content_top = 0;
	int caret_content_bottom = 0;

	int caret_scroller_top = 0;
	int caret_scroller_bottom = 0;

	if (view->cs_pending_event_mask != 0) {
		debug_log("There are pending events.");
		view->cs_notify_caret_pos = true;
		return;
	}

	caret_content_top = view->cs_header_height + top;
	caret_content_bottom = view->cs_header_height + bottom;

	_initial_view_cs_sync_with_ewk(view);

	caret_scroller_top = caret_content_top - view->cs_scroll_pos;
	caret_scroller_bottom = caret_content_bottom - view->cs_scroll_pos;

	if ((caret_scroller_top <= view->cs_rttb_height) || (caret_scroller_bottom >= view->cs_height)) {
		int new_scroll_pos = 0;

		if (caret_scroller_top <= view->cs_rttb_height) {
			new_scroll_pos = caret_content_top - view->cs_rttb_height - CS_CARET_PADDING_PX;
		} else {
			new_scroll_pos = caret_content_bottom - view->cs_height + CS_CARET_PADDING_PX;
		}

		if ((view->cs_scroll_pos >= view->cs_edge_scroll_pos) &&
			(new_scroll_pos < view->cs_edge_scroll_pos)) {
			new_scroll_pos = view->cs_edge_scroll_pos;
		}

		if (view->cs_has_selection || view->cs_has_magnifier) {
			composer_initial_view_cs_show(view, new_scroll_pos);
		} else {
			composer_initial_view_cs_bring_in(view, new_scroll_pos);
		}

	} else if (view->cs_bringin_to_ewk && (view->cs_scroll_pos < view->cs_edge_scroll_pos)) {

		composer_initial_view_cs_bring_in(view, view->cs_edge_scroll_pos);
	}

	view->cs_bringin_to_ewk = false;

	debug_leave();
}

static void _initial_view_cs_ensure_ewk_on_top(EmailComposerView *view, bool force_bring_in)
{
	debug_enter();

	if (view->cs_scroll_pos < view->cs_edge_scroll_pos) {
		if (force_bring_in) {
			int freeze_count = view->cs_freeze_count;
			view->cs_freeze_count = 0;
			composer_initial_view_cs_bring_in(view, view->cs_edge_scroll_pos);
			view->cs_freeze_count = freeze_count;
		} else {
			composer_initial_view_cs_show(view, view->cs_edge_scroll_pos);
		}
	}
}

static void _initial_view_cs_backup_scroll_pos(EmailComposerView *view)
{
	debug_enter();

	view->cs_backup_scroll_pos = view->cs_scroll_pos;
	view->cs_backup_pos_time = ecore_time_get();
}

static Eina_Bool _initial_view_cs_event_timer1_cb(void *data)
{
	EmailComposerView *view = data;

	view->cs_pending_event_mask &= ~COMPOSER_CSEF_EWK_CONTENT_RESIZE;
	_initial_view_cs_update(view, COMPOSER_CSEF_EWK_CONTENT_RESIZE);
	view->cs_events_timer1 = NULL;

	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _initial_view_cs_event_timer2_cb(void *data)
{
	EmailComposerView *view = data;

	view->cs_pending_event_mask &= ~COMPOSER_CSEF_INITIALIZE;
	_initial_view_cs_update(view, COMPOSER_CSEF_INITIALIZE);
	view->cs_events_timer2 = NULL;

	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _initial_view_cs_animator_cb(void *data, double pos)
{
	EmailComposerView *view = data;

	int new_pos = 0;
	float t = pos * view->cs_anim_duration;

	view->cs_anim_t = t;

	if (t <= view->cs_anim_t1) {
		new_pos = (int)(view->cs_anim_pos0 + view->cs_anim_v0 * t + 0.5f * view->cs_anim_a1 * t * t + 0.5f);
	} else {
		t -= view->cs_anim_t1;
		new_pos = (int)(view->cs_anim_pos1 + view->cs_anim_v1 * t + 0.5f * view->cs_anim_a2 * t * t + 0.5f);
	}

	_initial_view_cs_begin_scroll(view);

	_initial_view_cs_set_pos(view, new_pos);

	if (new_pos != view->cs_scroll_pos) {
		view->cs_animator = NULL;
		_initial_view_cs_end_scroll(view);
		debug_log("End reached.");
		return ECORE_CALLBACK_CANCEL;
	}

	if (pos == 1.0) {
		view->cs_animator = NULL;
		_initial_view_cs_end_scroll(view);
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
	EmailComposerView *view = data;

	_initial_view_cs_handle_event(view, COMPOSER_CSEF_MAIN_SCROLLER_RESIZE);
}

static void _initial_view_main_scroller_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailComposerView *view = data;
	Evas_Event_Mouse_Down *event = event_info;

	view->allow_click_events = EINA_TRUE;

	if (view->cs_ready && !view->cs_is_sliding && (view->cs_freeze_count == 0)) {
		view->cs_is_sliding = true;
		view->cs_drag_down_y = event->canvas.y;
		view->cs_drag_cur_y = event->canvas.y;
		view->cs_drag_cur_time = event->timestamp;

		_initial_view_cs_stop_animator(view);
		_initial_view_cs_begin_scroll(view);
	}
}

static void _initial_view_main_scroller_mouse_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	EmailComposerView *view = data;
	Evas_Event_Mouse_Move *event = event_info;

	int event_mask = 0;

	if (!view->cs_is_sliding) {
		return;
	}

	if (!view->cs_is_dragging) {
		if (abs(event->cur.canvas.y - view->cs_drag_down_y) > CS_DRAG_THRESHOLD_PX) {
			view->cs_is_dragging = true;
			view->cs_drag_start_y = view->cs_drag_cur_y;
			event_mask |= COMPOSER_CSEF_DRAG_START;
		} else {
			view->cs_drag_cur_y = event->cur.canvas.y;
			view->cs_drag_cur_time = event->timestamp;
			return;
		}
	}

	view->cs_anim_v0 = -1000.0f * (event->cur.canvas.y - view->cs_drag_cur_y) / (event->timestamp - view->cs_drag_cur_time);
	event_mask |= COMPOSER_CSEF_DRAGGING;

	view->cs_drag_cur_y = event->cur.canvas.y;
	view->cs_drag_cur_time = event->timestamp;

	_initial_view_cs_handle_event(view, event_mask);
}

static void _initial_view_main_scroller_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailComposerView *view = data;

	view->cs_is_sliding = false;

	if (view->cs_is_dragging) {
		view->allow_click_events = EINA_FALSE;
		view->cs_is_dragging = false;
		view->cs_anim_pos0 = (float)view->cs_scroll_pos;
		view->cs_anim_a1 = ((view->cs_anim_v0 > 0.0f) ? -CS_DRAG_ACCEL : CS_DRAG_ACCEL);
		view->cs_anim_t1 = -view->cs_anim_v0 / view->cs_anim_a1;
		view->cs_anim_duration = view->cs_anim_t1;
		view->cs_animator = ecore_animator_timeline_add(view->cs_anim_duration, _initial_view_cs_animator_cb, view);
	} else {
		_initial_view_cs_end_scroll(view);
	}
}

static void _initial_view_composer_layout_resize_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailComposerView *view = data;

	_initial_view_cs_handle_event(view, COMPOSER_CSEF_MAIN_CONTENT_RESIZE);
}

static void _initial_view_main_scroller_scroll_cb(void *data, Evas_Object *obj, void *event_info)
{
	EmailComposerView *view = data;

	int main_scroll_pos = 0;

	elm_scroller_region_get(obj, NULL, &main_scroll_pos, NULL, NULL);

	if (view->cs_main_scroll_pos != main_scroll_pos) {
		debug_log("Main scroller was changed! Prevent scroll change.");

		_initial_view_cs_sync_with_ewk(view);
		_initial_view_cs_set_pos(view, view->cs_scroll_pos);

		if (!view->cs_animator) {
			int new_cs_scroll_pos = 0;

			if (main_scroll_pos < view->cs_edge_scroll_pos) {
				new_cs_scroll_pos = main_scroll_pos;
			} else {
				int ewk_pos = 0;
				ewk_view_scroll_pos_get(view->ewk_view, NULL, &ewk_pos);
				new_cs_scroll_pos = main_scroll_pos + ewk_pos;
			}

			composer_initial_view_cs_bring_in(view, new_cs_scroll_pos);
		}
	}
}

static void _initial_view_magnifier_show_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailComposerView *view = data;

	view->cs_has_magnifier = true;

	composer_initial_view_cs_freeze_push(view);

	if (view->cs_has_selection) {
		_initial_view_cs_ensure_ewk_on_top(view, true);
	}
}

static void _initial_view_magnifier_hide_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailComposerView *view = data;

	view->cs_has_magnifier = false;

	composer_initial_view_cs_freeze_pop(view);
}

static void _initial_view_ewk_content_resize_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailComposerView *view = data;

	_initial_view_cs_handle_event(view, COMPOSER_CSEF_EWK_CONTENT_RESIZE);
}

/*
 * Definition for exported functions
 */

void composer_initial_view_back_cb(EmailComposerView *view)
{
	debug_enter();
	retm_if(!view, "Invalid parameter: view is NULL!");

	Eina_Bool is_selection_removed = ewk_view_text_selection_clear(view->ewk_view);
	retm_if(is_selection_removed, "ewk_view_text_selection_clear() result TRUE");

	_initial_view_back_button_clicked_cb(view, NULL, NULL);

	debug_leave();
}

void composer_initial_view_draw_base_frame(EmailComposerView *view)
{
	email_profiling_begin(composer_initial_view_draw_base_frame);
	debug_enter();

	retm_if(!view, "Invalid parameter: view is NULL!");

	Evas_Object *layout = NULL;
	Evas_Object *scroller = NULL;

	view->base.content = _initial_view_create_root_layout(view->base.module->navi, view);
	scroller = composer_util_create_scroller(view->base.module->navi);
	layout = _initial_view_create_composer_layout(view->base.module->navi);

	elm_object_part_content_set(view->base.content, "ec.swallow.content", scroller);
	elm_object_content_set(scroller, layout);
	_initial_view_create_naviframe_buttons(view->base.module->navi, view->base.content, view);

	view->main_scroller = scroller;
	view->composer_layout = layout;

	debug_leave();
	email_profiling_end(composer_initial_view_draw_base_frame);
}

void composer_initial_view_draw_richtext_components(void *data)
{
	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	view->richtext_toolbar = composer_rich_text_create_toolbar(view);
	composer_rich_text_disable_set(view, EINA_TRUE);
}

void composer_initial_view_draw_header_components(void *data)
{
	email_profiling_begin(composer_initial_view_draw_header_components);
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	Evas_Object *box = _initial_view_create_composer_box(view->main_scroller);
	elm_object_part_content_set(view->composer_layout, "ec.swallow.box", box);

	if (view->account_info && (view->account_info->account_count > 1) && (view->eComposerErrorType == COMPOSER_ERROR_NONE)) {
		Evas_Object *from_layout = composer_recipient_create_layout(view, box);
		elm_box_pack_end(box, from_layout);
		view->recp_from_layout = from_layout;
	}

	Evas_Object *to_layout = composer_recipient_create_layout(view, box);
	Evas_Object *cc_layout = composer_recipient_create_layout(view, box);
	Evas_Object *bcc_layout = composer_recipient_create_layout(view, box);
	Evas_Object *subject_layout = composer_subject_create_layout(box);

	elm_box_pack_end(box, to_layout);
	elm_box_pack_end(box, subject_layout);

	evas_object_hide(cc_layout);
	evas_object_hide(bcc_layout);

	evas_object_show(box);

	view->composer_box = box;
	view->recp_to_layout = to_layout;
	view->recp_cc_layout = cc_layout;
	view->recp_bcc_layout = bcc_layout;
	view->subject_layout = subject_layout;

	debug_leave();
	email_profiling_end(composer_initial_view_draw_header_components);
}

void composer_initial_view_draw_remaining_components(void *data)
{
	email_profiling_begin(composer_initial_view_draw_remaining_components);
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	if (view->composer_box) {
		if (view->recp_from_layout) {
			composer_recipient_update_from_detail(view, view->recp_from_layout);
		}
		composer_recipient_update_to_detail(view, view->recp_to_layout);
		if (view->composer_type != RUN_COMPOSER_NEW) {
			composer_recipient_update_cc_detail(view, view->recp_cc_layout);
			composer_recipient_update_bcc_detail(view, view->recp_bcc_layout);
		}

#ifdef _ALWAYS_CC_MYSELF
		if ((view->composer_type != RUN_COMPOSER_EDIT) && view->account_info && view->account_info->original_account &&
			(view->account_info->original_account->options.add_my_address_to_bcc != EMAIL_ADD_MY_ADDRESS_OPTION_DO_NOT_ADD)) {
			if (!view->bcc_added) {
				composer_recipient_show_hide_bcc_field(view, EINA_TRUE);
			}
			composer_recipient_append_myaddress(view, view->account_info->original_account);
		}
#endif
		composer_subject_update_detail(view, view->subject_layout);
	}

	composer_webkit_create_body_field(view->composer_layout, view);

	debug_leave();
	email_profiling_end(composer_initial_view_draw_remaining_components);
}

void composer_initial_view_create_combined_scroller(void *data)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	Evas_Object *combined_sc_ly = _initial_view_create_combined_scroller_layout(view->base.module->navi);
	elm_object_part_content_set(view->base.content, "ec.swallow.scroller", combined_sc_ly);
	view->combined_scroller = combined_sc_ly;
	evas_object_raise(combined_sc_ly);

	/* set callbacks for scrolling */

	evas_object_event_callback_add(view->main_scroller, EVAS_CALLBACK_RESIZE, _initial_view_main_scroller_resize_cb, view);
	evas_object_event_callback_add(view->main_scroller, EVAS_CALLBACK_MOUSE_DOWN, _initial_view_main_scroller_down_cb, view);
	evas_object_event_callback_add(view->main_scroller, EVAS_CALLBACK_MOUSE_MOVE, _initial_view_main_scroller_mouse_move_cb, view);
	evas_object_event_callback_add(view->main_scroller, EVAS_CALLBACK_MOUSE_UP, _initial_view_main_scroller_up_cb, view);

	evas_object_event_callback_add(view->composer_layout, EVAS_CALLBACK_RESIZE, _initial_view_composer_layout_resize_cb, view);

	evas_object_smart_callback_add(view->main_scroller, "scroll", _initial_view_main_scroller_scroll_cb, view);

	evas_object_smart_callback_add(view->ewk_view, "magnifier,show", _initial_view_magnifier_show_cb, view);
	evas_object_smart_callback_add(view->ewk_view, "magnifier,hide", _initial_view_magnifier_hide_cb, view);
	evas_object_smart_callback_add(view->ewk_view, "contents,size,changed", _initial_view_ewk_content_resize_cb, view);

	_initial_view_cs_handle_event(view, COMPOSER_CSEF_INITIALIZE);

	debug_leave();
	return;
}

void composer_initial_view_set_combined_scroller_rotation_mode(void *data)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	_initial_view_cs_stop_all(view);

	if (view->selected_widget == view->ewk_view) {
		_initial_view_cs_ensure_ewk_on_top(view, false);
		_initial_view_cs_backup_scroll_pos(view);
	}

	if (view->is_horizontal) {
		edje_object_signal_emit(_EDJ(view->combined_scroller), "scroller_landscape", "");
	} else {
		edje_object_signal_emit(_EDJ(view->combined_scroller), "scroller_portrait", "");
	}

	composer_util_resize_webview_height(view);

	debug_leave();
}

void composer_initial_view_caret_pos_changed_cb(EmailComposerView *view, int top, int bottom, bool isCollapsed)
{
	if (isCollapsed) {
		view->cs_has_selection = false;
		if (!view->cs_has_magnifier) {
			view->cs_in_selection_mode = false;
		}
	} else {
		view->cs_has_selection = true;
	}

	_initial_view_cs_handle_caret_pos_change(view, top, bottom);
}

void composer_initial_view_cs_bring_in(EmailComposerView *view, int pos)
{
	debug_enter();
	retm_if(!view->cs_ready, "Not ready!");

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

	if (view->cs_freeze_count > 0) {
		debug_warning("Freezed!");
		return;
	}

	if (view->cs_is_sliding) {
		debug_log("User is sliding over!");
		return;
	}

	pos2 = (float)_initial_view_cs_fix_pos(view, pos);

	if (view->cs_animator) {
		if (pos2 == view->cs_anim_pos2) {
			debug_log("No need to bring in.");
			return;
		}

		v0 = ((view->cs_anim_t <= view->cs_anim_t1) ?
				(view->cs_anim_v0 + view->cs_anim_a1 * view->cs_anim_t) :
				(view->cs_anim_v1 + view->cs_anim_a2 * (view->cs_anim_t - view->cs_anim_t1)));

		adjust = fabsf(v0 * CS_BRING_ADJUST_TIME);

		_initial_view_cs_stop_animator(view);
	}

	_initial_view_cs_sync_with_ewk(view);

	pos0 = (float)view->cs_scroll_pos;
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

	view->cs_anim_pos0 = pos0;
	view->cs_anim_pos1 = pos0 + v0 * t1 + 0.5f * a1 * t1 * t1;
	view->cs_anim_pos2 = pos2;
	view->cs_anim_v0 = v0;
	view->cs_anim_v1 = v1;
	view->cs_anim_a1 = a1;
	view->cs_anim_a2 = a2;
	view->cs_anim_t1 = t1;
	view->cs_anim_duration = duration;
	view->cs_anim_t = 0.0f;

	view->cs_animator = ecore_animator_timeline_add(duration, _initial_view_cs_animator_cb, view);
}

void composer_initial_view_cs_show(EmailComposerView *view, int pos)
{
	debug_enter();
	retm_if(!view->cs_ready, "Not ready!");

	_initial_view_cs_stop_animator(view);
	_initial_view_cs_begin_scroll(view);
	_initial_view_cs_set_pos(view, pos);
	_initial_view_cs_end_scroll(view);
}

void composer_initial_view_cs_freeze_push(EmailComposerView *view)
{
	debug_enter();
	retm_if(!view->cs_ready, "Not ready!");

	++view->cs_freeze_count;

	_initial_view_cs_stop_all(view);
}

void composer_initial_view_cs_freeze_pop(EmailComposerView *view)
{
	debug_enter();
	retm_if(!view->cs_ready, "Not ready!");

	if (view->cs_freeze_count > 0) {
		--view->cs_freeze_count;
	}
}

void composer_initial_view_activate_selection_mode(EmailComposerView *view)
{
	debug_enter();
	retm_if(!view->cs_ready, "Not ready!");

	if (!view->cs_in_selection_mode) {
		view->cs_in_selection_mode = true;
		_initial_view_cs_ensure_ewk_on_top(view, true);
	}
}
