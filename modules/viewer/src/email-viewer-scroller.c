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

#include "email-engine.h"
#include "email-viewer-scroller.h"
#include "email-viewer-util.h"

static void _viewer_set_horizontal_scroller_view_sizes(EmailViewerView *view);
static void _viewer_set_vertical_scroller_view_sizes(EmailViewerView *view);
static void _viewer_adjust_header_scroll_pos(EmailViewerView *view);
static void _viewer_scroll_down_cb(void *data, Evas_Object *obj, void *event_info);
static void _viewer_scroll_up_cb(void *data, Evas_Object *obj, void *event_info);
static void viewer_scroll_left_cb(void *data, Evas_Object *obj, void *event_info);
static void _viewer_scroll_right_cb(void *data, Evas_Object *obj, void *event_info);
static void _viewer_main_scroller_scroll(EmailViewerView *view);
static Evas_Object *_viewer_create_combined_scroller_layout(Evas_Object *parent);
static void _set_horizontal_scroller_position(EmailViewerView *view);

static void _viewer_set_horizontal_scroller_view_sizes(EmailViewerView *view)
{
	debug_enter_scroller();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");
	retm_if(view->webview == NULL, "view->webview is NULL.");

	int scroll_pos_x = 0;

	ewk_view_scroll_pos_get(view->webview, &scroll_pos_x, NULL);
	view->webkit_scroll_x = scroll_pos_x;

	int webkit_w = 0;
	double scale = ewk_view_scale_get(view->webview);
	ewk_view_contents_size_get(view->webview, &webkit_w, NULL);

	double width = (double)webkit_w * scale;
	view->total_width = (int)width;

	int y = 0, h = 0, w = 0;
	elm_scroller_region_get(view->scroller, NULL, &y, &w, &h);
	view->main_scroll_y = y;
	view->main_scroll_h = h;
	view->main_scroll_w = w;

	debug_leave_scroller();
}

static void _viewer_set_vertical_scroller_view_sizes(EmailViewerView *view)
{
	debug_enter_scroller();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");
	retm_if(view->webview == NULL, "view->webview is NULL.");

	int scroll_pos_y = 0;
	int w_h = 0;
	int w_y = 0;
	int navi_h = 0;
	int navi_y = 0;

	ewk_view_scroll_pos_get(view->webview, NULL, &scroll_pos_y);
	view->webkit_scroll_y = scroll_pos_y;

	evas_object_geometry_get(view->webview_ly, NULL, &w_y, NULL, &w_h);
	evas_object_geometry_get(view->base.module->navi, NULL, &navi_y, NULL, &navi_h);

	int webkit_h = 0;
	double scale = ewk_view_scale_get(view->webview);
	ewk_view_contents_size_get(view->webview, NULL, &webkit_h);

	double height = (double)webkit_h * scale;
	int h_info = w_y;			/*height of part above webkit*/
	view->trailer_height = (navi_y + navi_h) - (w_y + w_h);
	view->total_height = (int)height + h_info;
	view->header_height = w_y;

	debug_leave_scroller();
}

static void _set_horizontal_scroller_position(EmailViewerView *view)
{
	debug_enter_scroller();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	double pos_h = 0.0;
	_viewer_set_horizontal_scroller_view_sizes(view);

	pos_h = ((double)view->webkit_scroll_x) / ((double)view->total_width - (double)view->main_scroll_w);

	edje_object_part_drag_value_set(_EDJ(view->combined_scroller), "elm.dragable.hbar", pos_h, 0.0);

	debug_leave_scroller();
}

void viewer_set_vertical_scroller_position(EmailViewerView *view)
{
	debug_enter_scroller();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	double pos_v = 0.0;
	_viewer_adjust_header_scroll_pos(view);
	_viewer_set_vertical_scroller_view_sizes(view);

	pos_v = ((double)view->webkit_scroll_y + (double)view->main_scroll_y) / ((double)view->total_height - (double)view->main_scroll_h);
	debug_log_scroller("view->main_scroll_y:%d view->main_scroll_h:%d", view->main_scroll_y, view->main_scroll_h);
	edje_object_part_drag_value_set(_EDJ(view->combined_scroller), "elm.dragable.vbar", 0.0, pos_v);

	debug_leave_scroller();
}

void viewer_set_horizontal_scroller_size(EmailViewerView *view)
{
	debug_enter_scroller();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	double size = 0.0;
	size = (double)view->main_scroll_w / view->total_width;

	edje_object_part_drag_size_set(_EDJ(view->combined_scroller), "elm.dragable.hbar", size, 1.0);

	debug_leave_scroller();
}

void viewer_set_vertical_scroller_size(EmailViewerView *view)
{
	debug_enter_scroller();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	double size = (double)view->main_scroll_h / view->total_height;
	edje_object_part_drag_size_set(_EDJ(view->combined_scroller), "elm.dragable.vbar", 1.0, size);

	debug_leave_scroller();
}

static void _viewer_adjust_header_scroll_pos(EmailViewerView *view)
{
	debug_enter();
	retm_if(!view, "Invalid parameter: view is NULL!");

	if (view->webkit_scroll_y) {
		Evas_Coord ms_x, ms_y, ms_w, ms_h, web_y;
		elm_scroller_region_get(view->scroller, &ms_x, &ms_y, &ms_w, &ms_h);
		evas_object_geometry_get(view->webview, NULL, &web_y, NULL, NULL);

		Evas_Coord bottom_coord = ms_y + web_y;
		elm_scroller_region_show(view->scroller, ms_x, bottom_coord, ms_w, ms_h);
	}
}

static void _viewer_scroll_down_cb(void *data, Evas_Object *obj, void *event_info)
{

	debug_enter_scroller();
	EmailViewerView *view = (EmailViewerView *)data;

	viewer_set_vertical_scroller_position(view);
	viewer_set_vertical_scroller_size(view);

	edje_object_signal_emit(_EDJ(view->combined_scroller), "elm,action,scroll_v", "elm");

	debug_leave_scroller();
}

static void _viewer_scroll_up_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter_scroller();

	EmailViewerView *view = (EmailViewerView *)data;

	viewer_set_vertical_scroller_position(view);
	viewer_set_vertical_scroller_size(view);

	edje_object_signal_emit(_EDJ(view->combined_scroller), "elm,action,scroll_v", "elm");

	debug_leave_scroller();
}

static void viewer_scroll_left_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter_scroller();

	EmailViewerView *view = (EmailViewerView *)data;

	_set_horizontal_scroller_position(view);
	viewer_set_horizontal_scroller_size(view);

	edje_object_signal_emit(_EDJ(view->combined_scroller), "elm,action,scroll_h", "elm");

	debug_leave_scroller();
}

static void _viewer_scroll_right_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter_scroller();

	EmailViewerView *view = (EmailViewerView *)data;

	_set_horizontal_scroller_position(view);
	viewer_set_horizontal_scroller_size(view);

	edje_object_signal_emit(_EDJ(view->combined_scroller), "elm,action,scroll_h", "elm");

	debug_leave_scroller();
}

static void _viewer_main_scroller_scroll(EmailViewerView *view)
{
	debug_enter_scroller();

	_viewer_set_vertical_scroller_view_sizes(view);

	int x = 0, y = 0, h = 0, w = 0;
	elm_scroller_region_get(view->scroller, &x, &y, &w, &h);
	view->main_scroll_y = y;
	view->main_scroll_h = h;
	view->main_scroll_w = w;
	edje_object_signal_emit(_EDJ(view->combined_scroller), "elm,action,scroll_v", "elm");

	double pos = ((double)view->webkit_scroll_y + (double)view->main_scroll_y) / ((double)view->total_height - (double)view->main_scroll_h);

	edje_object_part_drag_value_set(_EDJ(view->combined_scroller), "elm.dragable.vbar", 0.0, pos);
	debug_log_scroller("pos: %f", pos);
	viewer_set_vertical_scroller_size(view);

	debug_leave_scroller();
}

static Evas_Object *_viewer_create_combined_scroller_layout(Evas_Object *parent)
{
	debug_enter_scroller();

	Evas_Object *layout = viewer_load_edj(parent, email_get_viewer_theme_path(), "combined_scroller", EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	edje_object_signal_emit(layout, "load", ""); /*initialize scroller*/
	evas_object_show(layout);

	debug_leave_scroller();

	return layout;
}

void viewer_stop_elm_scroller_start_webkit_scroller(void *data)
{
	debug_enter_scroller();

	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	int h = 0;
	int res = email_engine_check_body_download(view->mail_id);

	debug_log_scroller("recipient idler present: %d", (view->to_recipient_idler || view->cc_recipient_idler));

	debug_log_scroller("is_main_scroller_scrolling: %d, to_recipient_idler: %d, cc_recipient_idler: %d",
			view->is_main_scroller_scrolling, view->to_recipient_idler, view->cc_recipient_idler);

	if ((view->is_main_scroller_scrolling) &&
			((((res != 0) || (view->viewer_type == EML_VIEWER))) &&
				!(view->to_recipient_idler || view->cc_recipient_idler || view->bcc_recipient_idler))) {
		ewk_view_scroll_size_get(view->webview, NULL, &h);
		if (h > 0) {
			debug_log_scroller("Main scroller hold push");
			if (!elm_object_scroll_freeze_get(view->scroller)) {
				elm_object_scroll_freeze_push(view->scroller); /* stop */
			}
			ewk_view_vertical_panning_hold_set(view->webview, EINA_FALSE); /* restart */
			view->is_main_scroller_scrolling = EINA_FALSE;
		}
	}

	debug_leave_scroller();
}

void viewer_stop_webkit_scroller_start_elm_scroller(void *data)
{
	debug_enter_scroller();

	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;
	retm_if(view->webview == NULL, "webview is NULL");

	if (!view->is_main_scroller_scrolling) {
		debug_log_scroller("main scroller start");
		ewk_view_vertical_panning_hold_set(view->webview, EINA_TRUE); /* stop */

		if (elm_object_scroll_freeze_get(view->scroller) > 0)
			elm_object_scroll_freeze_pop(view->scroller); /* restart */

		int x = 0, y = 0, w = 0, h = 0;
		elm_scroller_region_get(view->scroller, &x, &y, &w, &h);
		debug_log_scroller("main_scroller(x:%d, y:%d, w:%d, h:%d)", x, y, w, h);

		elm_scroller_region_bring_in(view->scroller, x, 0, w, h);
		view->is_main_scroller_scrolling = EINA_TRUE;
	}

	debug_leave_scroller();
}

static void _viewer_main_scroller_scroll_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter_scroller();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;
	viewer_refresh_webview_visible_geometry(data);
	_viewer_main_scroller_scroll(view);

	debug_leave_scroller();
}

void viewer_create_combined_scroller(void *data)
{
	debug_enter_scroller();

	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	DELETE_EVAS_OBJECT(view->combined_scroller);
	Evas_Object *combined_sc_ly = _viewer_create_combined_scroller_layout(view->base.content);
	elm_object_part_content_set(view->base.content, "ev.swallow.scroller", combined_sc_ly);
	view->combined_scroller = combined_sc_ly;

	/*set scroller size*/
	_viewer_set_vertical_scroller_view_sizes(view);
	_viewer_set_horizontal_scroller_view_sizes(view);
	viewer_set_vertical_scroller_size(view);
	viewer_set_horizontal_scroller_size(view);

	/*set callbacks for scroll*/
	evas_object_smart_callback_add(view->scroller, "scroll", _viewer_main_scroller_scroll_cb, view);
	evas_object_smart_callback_add(view->webview, "scroll,up", _viewer_scroll_up_cb, view);
	evas_object_smart_callback_add(view->webview, "scroll,down", _viewer_scroll_down_cb, view);
	evas_object_smart_callback_add(view->webview, "scroll,left", viewer_scroll_left_cb, view);
	evas_object_smart_callback_add(view->webview, "scroll,right", _viewer_scroll_right_cb, view);

	debug_leave_scroller();
}
