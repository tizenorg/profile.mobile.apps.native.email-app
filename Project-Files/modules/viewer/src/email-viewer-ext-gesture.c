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
#include "email-viewer-ext-gesture.h"

#define GESTURE_MOMENT_MIN_X 600
#define GESTURE_ANGLE_MAX_DELTA 25.0

/*
 * Declaration for static functions
 */

static Evas_Event_Flags _onGestureStart(void *data, void *eventInfo);
static Evas_Event_Flags _onGestureEnd(void *data, void *eventInfo);

/*
 * Definition for static functions
 */

void __email_viewer_got_next_prev_message(void *data, gboolean next)
{
	if (!data) {
		debug_log("data is NULL");
		return;
	}

	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	_hide_view(ug_data);

	app_control_h service = NULL;
	app_control_create(&service);
	if (!service) {
		debug_log("service create failed");
		return;
	}

	if (next == true) {
		app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_MSG, EMAIL_BUNDLE_VAL_PREV_MSG);
	} else {
		app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_MSG, EMAIL_BUNDLE_VAL_NEXT_MSG);
	}
	char idx[BUFSIZ] = { '\0' };
	snprintf(idx, sizeof(idx), "%d", ug_data->mail_id);
	app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_MAIL_INDEX, idx);

	email_module_send_result(ug_data->base.module, service);

	app_control_destroy(service);

	debug_leave();
}

static Evas_Event_Flags _onGestureStart(void *data, void *eventInfo)
{
	debug_enter();
	EmailViewerUGD *ug_data = (EmailViewerUGD *) data;

	int ewk_sc_x = 0;
	int ewk_sc_y = 0;

	ewk_view_scroll_pos_get(ug_data->webview, &ewk_sc_x, &ewk_sc_y);
	ug_data->webkit_old_x = ewk_sc_x;

	return EVAS_EVENT_FLAG_NONE;
}

static Evas_Event_Flags _onGestureEnd(void *data, void *eventInfo)
{
	debug_enter();
	EmailViewerUGD *ug_data = (EmailViewerUGD *) data;
	Elm_Gesture_Line_Info *event = (Elm_Gesture_Line_Info *) eventInfo;

	Evas_Coord ewk_ct_w = 0;
	Evas_Coord ewk_ct_h = 0;

	if ((abs(event->momentum.mx) < GESTURE_MOMENT_MIN_X) ||
		(fabs(fabs(event->angle - 180.0) - 90.0) > GESTURE_ANGLE_MAX_DELTA)) {
		debug_log("Skip");
		return EVAS_EVENT_FLAG_NONE;
	}

	ewk_view_scroll_size_get(ug_data->webview, &ewk_ct_w, &ewk_ct_h);

	if (event->momentum.mx > 0) { /* right//next */
		if (ug_data->webkit_old_x == 0) {
			__email_viewer_got_next_prev_message(data, true);
		}
	} else { /* left//prevous */
		if (ug_data->webkit_old_x == ewk_ct_w) {
			__email_viewer_got_next_prev_message(data, false);
		}
	}

	debug_leave();
	return EVAS_EVENT_FLAG_NONE;
}


/*
 * Definition for exported functions
 */

void viewer_set_flick_layer(void *data)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerUGD *ug_data = (EmailViewerUGD *) data;

	Evas_Object *gesture_obj = elm_gesture_layer_add(ug_data->scroller);
	retm_if(gesture_obj == NULL, "elm_gesture_layer_add() failed!");

	elm_gesture_layer_attach(gesture_obj, ug_data->scroller);
	elm_gesture_layer_continues_enable_set(gesture_obj, EINA_FALSE);

	elm_gesture_layer_cb_set(gesture_obj, ELM_GESTURE_N_FLICKS, ELM_GESTURE_STATE_START, _onGestureStart, ug_data);
	elm_gesture_layer_cb_set(gesture_obj, ELM_GESTURE_N_FLICKS, ELM_GESTURE_STATE_END, _onGestureEnd, ug_data);

	debug_leave();
}

