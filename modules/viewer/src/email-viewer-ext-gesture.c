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

#include "email-debug.h"
#include "email-engine.h"
#include "email-viewer.h"
#include "email-viewer-util.h"
#include "email-viewer-scroller.h"
#include "email-viewer-ext-gesture.h"

#define EMAIL_GESTURE_MOMENT_MIN_X		600
#define EMAIL_GESTURE_ANGLE_MAX_DELTA	25.0

#define EMAIL_START_EWKVIEW_SCROLL_THRESHOLD_H 1

/*
 * Declaration for static functions
 */

static Evas_Event_Flags _gesture_flicks_start_cb(void *data, void *eventInfo);
static Evas_Event_Flags _gesture_flicks_end_cb(void *data, void *eventInfo);
static Evas_Event_Flags _gesture_momentum_move_cb(void *data, void *eventInfo);

/*
 * Definition for static functions
 */

void __email_viewer_got_next_prev_message(void *data, gboolean next)
{
	if (!data) {
		debug_log("data is NULL");
		return;
	}

	EmailViewerView *view = (EmailViewerView *)data;

	_hide_view(view);

	email_params_h params = NULL;

	if (email_params_create(&params) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_MAIL_INDEX, view->mail_id) &&
		email_params_add_str(params, EMAIL_BUNDLE_KEY_MSG,
				(next ? EMAIL_BUNDLE_VAL_PREV_MSG : EMAIL_BUNDLE_VAL_NEXT_MSG))) {

		email_module_send_result(view->base.module, params);
	}

	email_params_free(&params);

	debug_leave();
}

static Evas_Event_Flags _gesture_flicks_start_cb(void *data, void *eventInfo)
{
	debug_enter();
	EmailViewerView *view = (EmailViewerView *) data;

	int ewk_sc_x = 0;
	int ewk_sc_y = 0;

	ewk_view_scroll_pos_get(view->webview, &ewk_sc_x, &ewk_sc_y);
	view->webkit_old_x = ewk_sc_x;

	return EVAS_EVENT_FLAG_NONE;
}

static Evas_Event_Flags _gesture_flicks_end_cb(void *data, void *eventInfo)
{
	debug_enter();
	EmailViewerView *view = (EmailViewerView *) data;
	Elm_Gesture_Line_Info *event = (Elm_Gesture_Line_Info *) eventInfo;

	Evas_Coord ewk_ct_w = 0;
	Evas_Coord ewk_ct_h = 0;

	if ((abs(event->momentum.mx) < EMAIL_GESTURE_MOMENT_MIN_X) ||
		(fabs(fabs(event->angle - 180.0) - 90.0) > EMAIL_GESTURE_ANGLE_MAX_DELTA)) {
		debug_log("Skip");
		return EVAS_EVENT_FLAG_NONE;
	}

	ewk_view_scroll_size_get(view->webview, &ewk_ct_w, &ewk_ct_h);

	if (event->momentum.mx > 0) { /* right//next */
		if (view->webkit_old_x == 0) {
			__email_viewer_got_next_prev_message(data, true);
		}
	} else { /* left//prevous */
		if (view->webkit_old_x == ewk_ct_w) {
			__email_viewer_got_next_prev_message(data, false);
		}
	}

	debug_leave();
	return EVAS_EVENT_FLAG_NONE;
}

static Evas_Event_Flags _gesture_momentum_move_cb(void *data, void *eventInfo)
{
	Elm_Gesture_Momentum_Info *event = eventInfo;
	EmailViewerView *view = data;

	if (event->my > 0) {
		if (!view->is_main_scroller_scrolling) {
			int ewk_pos = 0;
			ewk_view_scroll_pos_get(view->webview, NULL, &ewk_pos);
			if (ewk_pos == 0) {
				viewer_stop_webkit_scroller_start_elm_scroller(view);
			}
		}
	} else if (event->my < 0) {
		if (view->is_main_scroller_scrolling) {
			Evas_Coord ewk_scroll_h;
			ewk_view_scroll_size_get(view->webview, NULL, &ewk_scroll_h);
			if (ewk_scroll_h > EMAIL_START_EWKVIEW_SCROLL_THRESHOLD_H) {
				Evas_Coord reg_x, reg_y, reg_w, reg_h, child_w, child_h;
				elm_scroller_region_get(view->scroller, &reg_x, &reg_y, &reg_w, &reg_h);
				elm_scroller_child_size_get(view->scroller, &child_w, &child_h);
				if (reg_y == (child_h - reg_h)) {
					viewer_stop_elm_scroller_start_webkit_scroller(view);
				}
			}
		}
	}

	return EVAS_EVENT_FLAG_NONE;
}

/*
 * Definition for exported functions
 */

void viewer_set_flick_layer(void *data)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerView *view = (EmailViewerView *) data;

	Evas_Object *gesture_obj = elm_gesture_layer_add(view->scroller);
	retm_if(gesture_obj == NULL, "elm_gesture_layer_add() failed!");

	elm_gesture_layer_attach(gesture_obj, view->scroller);
	elm_gesture_layer_continues_enable_set(gesture_obj, EINA_FALSE);

	elm_gesture_layer_cb_set(gesture_obj, ELM_GESTURE_N_FLICKS, ELM_GESTURE_STATE_START, _gesture_flicks_start_cb, view);
	elm_gesture_layer_cb_set(gesture_obj, ELM_GESTURE_N_FLICKS, ELM_GESTURE_STATE_END, _gesture_flicks_end_cb, view);
	elm_gesture_layer_cb_set(gesture_obj, ELM_GESTURE_MOMENTUM, ELM_GESTURE_STATE_MOVE, _gesture_momentum_move_cb, view);

	debug_leave();
}

