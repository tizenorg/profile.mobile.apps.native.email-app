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

#include "email-color-box.h"
#include "email-utils.h"

#define EMAIL_COLOR_BOX_BG_PART_NAME "email.swallow.bg"

#define EMAIL_COLOR_BOX_TIMER_INTERVAL_SEC 0.067

#define EMAIL_COLOR_BOX_INITIAL_ALPHA 1.0
#define EMAIL_COLOR_BOX_ALPHA_STEP -0.04

typedef struct _email_color_box_data
{
	Evas_Object *box;
	Evas_Object *scroller;
	Ecore_Timer *timer;
	Eina_Bool update_on_timer;
	Eina_Bool calculating;

} email_color_box_t;

static void _email_color_box_update_colors_with_calculate(email_color_box_t *cbd);
static void _email_color_box_update_colors_with_timer(email_color_box_t *cbd);
static void _email_color_box_update_colors(email_color_box_t *cbd);

static Eina_Bool _email_color_box_evas_calculate(email_color_box_t *cbd);

static Eina_Bool _email_color_box_update_colors_timer_cb(void *data);

static void _email_color_box_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _email_color_box_resize_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _email_color_box_free_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void _email_color_box_scroller_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _email_color_box_scroller_resize_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _email_color_box_scroller_free_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

Evas_Object *email_color_box_add(Evas_Object *scroller)
{
	debug_enter();
	retvm_if(!scroller, NULL, "scroller is NULL");

	Evas_Object *box = NULL;
	email_color_box_t *cbd = NULL;

	box = elm_box_add(scroller);
	if (!box) {
		debug_error("elm_box_add() failed!");
		return NULL;
	}

	cbd = calloc(1, sizeof(email_color_box_t));
	if (!cbd) {
		debug_error("Memory allocation failed failed!");
		evas_object_del(box);
		return NULL;
	}

	cbd->box = box;
	cbd->scroller = scroller;

	evas_object_event_callback_add(box, EVAS_CALLBACK_MOVE, _email_color_box_move_cb, cbd);
	evas_object_event_callback_add(box, EVAS_CALLBACK_RESIZE, _email_color_box_resize_cb, cbd);
	evas_object_event_callback_add(box, EVAS_CALLBACK_FREE, _email_color_box_free_cb, cbd);

	evas_object_event_callback_add(scroller, EVAS_CALLBACK_MOVE, _email_color_box_scroller_move_cb, cbd);
	evas_object_event_callback_add(scroller, EVAS_CALLBACK_RESIZE, _email_color_box_scroller_resize_cb, cbd);
	evas_object_event_callback_add(scroller, EVAS_CALLBACK_FREE, _email_color_box_scroller_free_cb, cbd);

	return box;
}

void _email_color_box_update_colors_with_calculate(email_color_box_t *cbd)
{
	debug_enter();

	if (cbd->scroller && _email_color_box_evas_calculate(cbd)) {
		_email_color_box_update_colors(cbd);
	}
}

void _email_color_box_update_colors_with_timer(email_color_box_t *cbd)
{
	if (!cbd->scroller) {
		return;
	}

	if (!cbd->timer) {
		debug_log("Starting update timer");
		cbd->timer = ecore_timer_add(EMAIL_COLOR_BOX_TIMER_INTERVAL_SEC,
				_email_color_box_update_colors_timer_cb, cbd);
	}

	cbd->update_on_timer = EINA_TRUE;
}

void _email_color_box_update_colors(email_color_box_t *cbd)
{
	int scroller_y = 0;
	int scroller_h = 0;

	Eina_List *children = NULL;
	Eina_List *l = NULL;
	Evas_Object *subobj = NULL;

	double alpha = EMAIL_COLOR_BOX_INITIAL_ALPHA;

	evas_object_geometry_get(cbd->scroller, NULL, &scroller_y, NULL, &scroller_h);

	children = elm_box_children_get(cbd->box);
	EINA_LIST_FOREACH(children, l, subobj) {
		if (elm_object_widget_check(subobj)) {
			Evas_Object *edje = elm_layout_edje_get(subobj);
			if (edje_object_part_exists(edje, EMAIL_COLOR_BOX_BG_PART_NAME)) {

				int subobj_y = 0;
				int subobj_h = 0;
				int c = (int)(alpha * B0211_C + 0.5);
				int a = (int)(alpha * FULL_ALPHA + 0.5);
				Evas_Object *bg = elm_object_part_content_get(subobj, EMAIL_COLOR_BOX_BG_PART_NAME);

				if (!bg) {
					bg = evas_object_rectangle_add(evas_object_evas_get(subobj));
					elm_object_part_content_set(subobj, EMAIL_COLOR_BOX_BG_PART_NAME, bg);
					evas_object_show(bg);
				}
				evas_object_color_set(bg, c, c, c, a);

				evas_object_geometry_get(subobj, NULL, &subobj_y, NULL, &subobj_h);
				if ((subobj_y + subobj_h > scroller_y) && (subobj_y < scroller_y + scroller_h)) {
					alpha += EMAIL_COLOR_BOX_ALPHA_STEP;
				}
			}
		}
	}
	eina_list_free(children);
}

Eina_Bool _email_color_box_evas_calculate(email_color_box_t *cbd)
{
	if (cbd->calculating) {
		return EINA_FALSE;
	}

	cbd->calculating = EINA_TRUE;
	evas_smart_objects_calculate(evas_object_evas_get(cbd->box));
	cbd->calculating = EINA_FALSE;

	return EINA_TRUE;
}

Eina_Bool _email_color_box_update_colors_timer_cb(void *data)
{
	email_color_box_t *cbd = data;

	if (cbd->update_on_timer) {
		_email_color_box_update_colors(cbd);
		cbd->update_on_timer = EINA_FALSE;
		return ECORE_CALLBACK_RENEW;
	}

	cbd->timer = NULL;

	return ECORE_CALLBACK_CANCEL;
}

void _email_color_box_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	email_color_box_t *cbd = data;

	_email_color_box_update_colors_with_timer(cbd);
}

void _email_color_box_resize_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	email_color_box_t *cbd = data;

	_email_color_box_update_colors_with_calculate(cbd);
}

void _email_color_box_free_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();
	email_color_box_t *cbd = data;

	if (cbd->scroller) {
		evas_object_event_callback_del(cbd->scroller, EVAS_CALLBACK_MOVE, _email_color_box_move_cb);
		evas_object_event_callback_del(cbd->scroller, EVAS_CALLBACK_RESIZE, _email_color_box_resize_cb);
		evas_object_event_callback_del(cbd->scroller, EVAS_CALLBACK_FREE, _email_color_box_scroller_free_cb);
	}

	DELETE_TIMER_OBJECT(cbd->timer);

	free(cbd);
}

void _email_color_box_scroller_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	email_color_box_t *cbd = data;

	_email_color_box_update_colors_with_timer(cbd);
}

void _email_color_box_scroller_resize_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	email_color_box_t *cbd = data;

	_email_color_box_update_colors(cbd);
}

void _email_color_box_scroller_free_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();
	email_color_box_t *cbd = data;

	cbd->scroller = NULL;

	DELETE_TIMER_OBJECT(cbd->timer);
}
