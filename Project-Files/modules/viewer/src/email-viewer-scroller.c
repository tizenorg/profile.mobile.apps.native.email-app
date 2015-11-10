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

static void _viewer_set_horizontal_scroller_view_sizes(EmailViewerUGD *ug_data);
static void _viewer_set_vertical_scroller_view_sizes(EmailViewerUGD *ug_data);
static void _viewer_adjust_header_scroll_pos(EmailViewerUGD *ug_data);
static void _viewer_scroll_down_cb(void *data, Evas_Object *obj, void *event_info);
static void _viewer_scroll_up_cb(void *data, Evas_Object *obj, void *event_info);
static void viewer_scroll_left_cb(void *data, Evas_Object *obj, void *event_info);
static void _viewer_scroll_right_cb(void *data, Evas_Object *obj, void *event_info);
static void _viewer_main_scroller_scroll(void *data, Eina_Bool is_up);
static void _viewer_main_scroller_scroll_up_cb(void *data, Evas_Object *obj, void *event_info);
static void _viewer_main_scroller_scroll_down_cb(void *data, Evas_Object *obj, void *event_info);
static Evas_Object *_viewer_create_combined_scroller_layout(Evas_Object *parent);
static void _set_horizontal_scroller_position(EmailViewerUGD *ug_data);


static int g_lock = 0;
static void _viewer_set_horizontal_scroller_view_sizes(EmailViewerUGD *ug_data)
{
	debug_enter_scroller();
	retm_if(ug_data == NULL, "Invalid parameter: ug_data[NULL]");
	retm_if(ug_data->webview == NULL, "ug_data->webview is NULL.");

	int scroll_pos_x = 0;

	ewk_view_scroll_pos_get(ug_data->webview, &scroll_pos_x, NULL);
	ug_data->webkit_scroll_x = scroll_pos_x;

	int webkit_w = 0;
	double scale = ewk_view_scale_get(ug_data->webview);
	ewk_view_contents_size_get(ug_data->webview, &webkit_w, NULL);

	double width = (double)webkit_w * scale;
	ug_data->total_width = (int)width;

	int y = 0, h = 0, w = 0;
	elm_scroller_region_get(ug_data->scroller, NULL, &y, &w, &h);
	ug_data->main_scroll_y = y;
	ug_data->main_scroll_h = h;
	ug_data->main_scroll_w = w;

	debug_leave_scroller();
}

static void _viewer_set_vertical_scroller_view_sizes(EmailViewerUGD *ug_data)
{
	debug_enter_scroller();
	retm_if(ug_data == NULL, "Invalid parameter: ug_data[NULL]");
	retm_if(ug_data->webview == NULL, "ug_data->webview is NULL.");

	int scroll_pos_y = 0;
	int subject_y = 0;
	int w_h = 0;
	int w_y = 0;
	int navi_h = 0;
	int navi_y = 0;

	ewk_view_scroll_pos_get(ug_data->webview, NULL, &scroll_pos_y);
	ug_data->webkit_scroll_y = scroll_pos_y;

	evas_object_geometry_get(ug_data->subject_ly, NULL, &subject_y, NULL, NULL);
	evas_object_geometry_get(ug_data->webview_ly, NULL, &w_y, NULL, &w_h);
	evas_object_geometry_get(ug_data->base.module->navi, NULL, &navi_y, NULL, &navi_h);

	int webkit_h = 0;
	double scale = ewk_view_scale_get(ug_data->webview);
	ewk_view_contents_size_get(ug_data->webview, NULL, &webkit_h);

	double height = (double)webkit_h * scale;
	int h_info = w_y;			/*height of part above webkit*/
	ug_data->trailer_height = (navi_y + navi_h) - (w_y + w_h);
	ug_data->total_height = (int)height + h_info;
	ug_data->header_height = w_y;

	debug_leave_scroller();
}

static void _set_horizontal_scroller_position(EmailViewerUGD *ug_data)
{
	debug_enter_scroller();
	retm_if(ug_data == NULL, "Invalid parameter: ug_data[NULL]");

	double pos_h = 0.0;
	_viewer_set_horizontal_scroller_view_sizes(ug_data);

	pos_h = ((double)ug_data->webkit_scroll_x) / ((double)ug_data->total_width - (double)ug_data->main_scroll_w);

	edje_object_part_drag_value_set(_EDJ(ug_data->combined_scroller), "elm.dragable.hbar", pos_h, 0.0);

	debug_leave_scroller();
}

void viewer_set_vertical_scroller_position(EmailViewerUGD *ug_data)
{
	debug_enter_scroller();
	retm_if(ug_data == NULL, "Invalid parameter: ug_data[NULL]");

	double pos_v = 0.0;
	_viewer_adjust_header_scroll_pos(ug_data);
	_viewer_set_vertical_scroller_view_sizes(ug_data);

	pos_v = ((double)ug_data->webkit_scroll_y + (double)ug_data->main_scroll_y) / ((double)ug_data->total_height - (double)ug_data->main_scroll_h);
	debug_log_scroller("ug_data->main_scroll_y:%d ug_data->main_scroll_h:%d", ug_data->main_scroll_y, ug_data->main_scroll_h);
	edje_object_part_drag_value_set(_EDJ(ug_data->combined_scroller), "elm.dragable.vbar", 0.0, pos_v);

	debug_leave_scroller();
}

void viewer_set_horizontal_scroller_size(EmailViewerUGD *ug_data)
{
	debug_enter_scroller();
	retm_if(ug_data == NULL, "Invalid parameter: ug_data[NULL]");

	double size = 0.0;
	size = (double)ug_data->main_scroll_w / ug_data->total_width;

	edje_object_part_drag_size_set(_EDJ(ug_data->combined_scroller), "elm.dragable.hbar", size, 1.0);

	debug_leave_scroller();
}

void viewer_set_vertical_scroller_size(EmailViewerUGD *ug_data)
{
	debug_enter_scroller();
	retm_if(ug_data == NULL, "Invalid parameter: ug_data[NULL]");

	double size = (double)ug_data->main_scroll_h / ug_data->total_height;
	edje_object_part_drag_size_set(_EDJ(ug_data->combined_scroller), "elm.dragable.vbar", 1.0, size);

	debug_leave_scroller();
}

static void _viewer_adjust_header_scroll_pos(EmailViewerUGD *ug_data)
{
	debug_enter();
	retm_if(!ug_data, "Invalid parameter: ug_data is NULL!");

	if (ug_data->webkit_scroll_y) {
		Evas_Coord ms_x, ms_y, ms_w, ms_h, web_y;
		elm_scroller_region_get(ug_data->scroller, &ms_x, &ms_y, &ms_w, &ms_h);
		evas_object_geometry_get(ug_data->webview, NULL, &web_y, NULL, NULL);

		Evas_Coord bottom_coord = ms_y + web_y;
		elm_scroller_region_show(ug_data->scroller, ms_x, bottom_coord, ms_w, ms_h);
	}
}

static void _viewer_scroll_down_cb(void *data, Evas_Object *obj, void *event_info)
{

	debug_enter_scroller();
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	ug_data->is_scrolling_down = EINA_TRUE;
	ug_data->is_scrolling_up = EINA_FALSE;

	viewer_set_vertical_scroller_position(ug_data);
	viewer_set_vertical_scroller_size(ug_data);

	edje_object_signal_emit(_EDJ(ug_data->combined_scroller), "elm,action,scroll_v", "elm");

	debug_leave_scroller();
}

static void _viewer_scroll_up_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter_scroller();

	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	ug_data->is_scrolling_up = EINA_TRUE;
	ug_data->is_scrolling_down = EINA_FALSE;

	viewer_set_vertical_scroller_position(ug_data);
	viewer_set_vertical_scroller_size(ug_data);

	edje_object_signal_emit(_EDJ(ug_data->combined_scroller), "elm,action,scroll_v", "elm");

	debug_leave_scroller();
}

static void viewer_scroll_left_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter_scroller();

	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	_set_horizontal_scroller_position(ug_data);
	viewer_set_horizontal_scroller_size(ug_data);

	edje_object_signal_emit(_EDJ(ug_data->combined_scroller), "elm,action,scroll_h", "elm");

	debug_leave_scroller();
}

static void _viewer_scroll_right_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter_scroller();

	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	_set_horizontal_scroller_position(ug_data);
	viewer_set_horizontal_scroller_size(ug_data);

	edje_object_signal_emit(_EDJ(ug_data->combined_scroller), "elm,action,scroll_h", "elm");

	debug_leave_scroller();
}

static void _viewer_main_scroller_scroll(void *data, Eina_Bool is_up)
{
	debug_enter_scroller();

	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	_viewer_set_vertical_scroller_view_sizes(ug_data);

	int x = 0, y = 0, h = 0, w = 0;
	elm_scroller_region_get(ug_data->scroller, &x, &y, &w, &h);
	ug_data->main_scroll_y = y;
	ug_data->main_scroll_h = h;
	ug_data->main_scroll_w = w;
	edje_object_signal_emit(_EDJ(ug_data->combined_scroller), "elm,action,scroll_v", "elm");

	double pos = ((double)ug_data->webkit_scroll_y + (double)ug_data->main_scroll_y) / ((double)ug_data->total_height - (double)ug_data->main_scroll_h);

	edje_object_part_drag_value_set(_EDJ(ug_data->combined_scroller), "elm.dragable.vbar", 0.0, pos);
	debug_log_scroller("pos: %f", pos);
	viewer_set_vertical_scroller_size(ug_data);

	debug_leave_scroller();
}

static void _viewer_main_scroller_scroll_up_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter_scroller();

	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	retm_if(g_lock == 1, "this function is already running");
	g_lock = 1;	/*lock this function*/

	ug_data->is_bottom_webview_reached = EINA_FALSE;
	ug_data->is_scrolling_up = EINA_TRUE;
	ug_data->is_scrolling_down = EINA_FALSE;

	_viewer_main_scroller_scroll(ug_data, EINA_TRUE);

	g_lock = 0;	/*unlock this function*/

	debug_leave_scroller();
}

static void _viewer_main_scroller_scroll_down_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter_scroller();

	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	retm_if(g_lock == 1, "this function is already running");
	g_lock = 1;	/*lock this function*/

	ug_data->is_top_webview_reached = EINA_FALSE;
	ug_data->is_scrolling_down = EINA_TRUE;
	ug_data->is_scrolling_up = EINA_FALSE;

	_viewer_main_scroller_scroll(ug_data, EINA_FALSE);

	g_lock = 0;	/*unlock this function*/

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
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	int h = 0;
	int res = email_engine_check_body_download(ug_data->mail_id);

	debug_log_scroller("recipient idler present: %d", (ug_data->to_recipient_idler || ug_data->cc_recipient_idler));

	debug_log_scroller("is_main_scroller_scrolling: %d, to_recipient_idler: %d, cc_recipient_idler: %d",
			ug_data->is_main_scroller_scrolling, ug_data->to_recipient_idler, ug_data->cc_recipient_idler);

	if ((ug_data->is_main_scroller_scrolling) &&
			((((res != 0) || (ug_data->viewer_type == EML_VIEWER))) &&
				!(ug_data->to_recipient_idler || ug_data->cc_recipient_idler || ug_data->bcc_recipient_idler))) {
		ewk_view_scroll_size_get(ug_data->webview, NULL, &h);
		if (h > 0) {
			debug_log_scroller("Main scroller hold push");
			if (!elm_object_scroll_freeze_get(ug_data->scroller)) {
				elm_object_scroll_freeze_push(ug_data->scroller); /* stop */
			}
			ewk_view_vertical_panning_hold_set(ug_data->webview, EINA_FALSE); /* restart */
			ug_data->is_main_scroller_scrolling = EINA_FALSE;
			ug_data->is_webview_scrolling = EINA_TRUE;
		}
	}

	debug_leave_scroller();
}

void viewer_stop_webkit_scroller_start_elm_scroller(void *data)
{
	debug_enter_scroller();

	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;
	retm_if(ug_data->webview == NULL, "webview is NULL");

	if (ug_data->is_webview_scrolling) {
		debug_log_scroller("main scroller start");
		ewk_view_vertical_panning_hold_set(ug_data->webview, EINA_TRUE); /* stop */

		if (elm_object_scroll_freeze_get(ug_data->scroller) > 0)
			elm_object_scroll_freeze_pop(ug_data->scroller); /* restart */

		int x = 0, y = 0, w = 0, h = 0;
		elm_scroller_region_get(ug_data->scroller, &x, &y, &w, &h);
		debug_log_scroller("main_scroller(x:%d, y:%d, w:%d, h:%d)", x, y, w, h);

		elm_scroller_region_bring_in(ug_data->scroller, x, 0, w, h);
		ug_data->is_main_scroller_scrolling = EINA_TRUE;
		ug_data->is_webview_scrolling = EINA_FALSE;
	}

	debug_leave_scroller();
}

void viewer_create_combined_scroller(void *data)
{
	debug_enter_scroller();

	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	DELETE_EVAS_OBJECT(ug_data->combined_scroller);
	Evas_Object *combined_sc_ly = _viewer_create_combined_scroller_layout(ug_data->base.content);
	elm_object_part_content_set(ug_data->base.content, "ev.swallow.scroller", combined_sc_ly);
	ug_data->combined_scroller = combined_sc_ly;

	/*set scroller size*/
	_viewer_set_vertical_scroller_view_sizes(ug_data);
	_viewer_set_horizontal_scroller_view_sizes(ug_data);
	viewer_set_vertical_scroller_size(ug_data);
	viewer_set_horizontal_scroller_size(ug_data);

	/*set callbacks for scroll*/
	evas_object_smart_callback_add(ug_data->scroller, "scroll,up", _viewer_main_scroller_scroll_up_cb, ug_data);
	evas_object_smart_callback_add(ug_data->scroller, "scroll,down", _viewer_main_scroller_scroll_down_cb, ug_data);
	evas_object_smart_callback_add(ug_data->webview, "scroll,up", _viewer_scroll_up_cb, ug_data);
	evas_object_smart_callback_add(ug_data->webview, "scroll,down", _viewer_scroll_down_cb, ug_data);
	evas_object_smart_callback_add(ug_data->webview, "scroll,left", viewer_scroll_left_cb, ug_data);
	evas_object_smart_callback_add(ug_data->webview, "scroll,right", _viewer_scroll_right_cb, ug_data);

	debug_leave_scroller();
	return;
}
