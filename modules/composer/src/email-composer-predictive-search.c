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

#define _GNU_SOURCE

#include <string.h>

#include "email-composer.h"
#include "email-composer-types.h"
#include "email-composer-util.h"
#include "email-composer-predictive-search.h"
#include "email-composer-initial-view.h"

/*
 * Declaration for static variables
 */

#define COMPOSER_PS_BOTTOM_PADDING ELM_SCALE_SIZE(16)
#define COMPOSER_PS_BORDER_SIZE ELM_SCALE_SIZE(2)
#define COMPOSER_PS_INITAL_WIDTH 256

Elm_Genlist_Item_Class __ps_itc;


/*
 * Declaration for static functions
 */

static Eina_Bool __composer_ps_mouse_down_cb(void *data, int type, void *event);
static void __composer_ps_back_cb(void *data, Evas_Object *obj, void *event_info);
static void __composer_ps_gl_sel(void *data, Evas_Object *obj, void *event_info);
static char *__composer_pslines_gl_text_get(void *data, Evas_Object *obj, const char *part);


static char *__composer_ps_insert_match_tag(char *input, char *matched_word);
Evas_Object *__composer_ps_create_genlist(Evas_Object *parent, EmailComposerView *view);
static void __composer_ps_create_view(EmailComposerView *view);
static void __composer_ps_destroy_view(EmailComposerView *view);

static void composer_ps_selected_recipient_editfield_get(EmailComposerView *view, email_editfield_t **editfield);

static void __composer_ps_append_result(EmailComposerView *view, Eina_List *predict_list, Elm_Object_Item *parent);
static Eina_List *__composer_ps_search_through_list(Eina_List *src, const char *search_word);

/*
 * Definition for static functions
 */

static Eina_Bool __composer_ps_mouse_down_cb(void *data, int type, void *event)
{
	debug_enter();

	retvm_if(!data, EINA_TRUE, "Invalid parameter: data is NULL!");
	retvm_if(!event, EINA_TRUE, "Invalid parameter: event is NULL!");

	EmailComposerView *view = (EmailComposerView *) data;
	Ecore_Event_Mouse_Button *ev = (Ecore_Event_Mouse_Button *)event;

	Evas_Coord x = 0, y = 0, w = 0, h = 0;
	Evas_Coord part_x = 0, part_y = 0, part_w = 0, part_h = 0;
	int mouse_x = 0, mouse_y = 0;

	/* Convert mouse position based on orientation.
	 * Because root point(R0) of mouse down is not changed according to rotation, but, root point of predictsearch layout is changed according to rotation.
	 * for COMPOSER_ROTATE_PORTRAIT and COMPOSER_ROTATE_PORTRAIT_UPSIDEDOWN different values are required if we were to implement reverse portrait mode
	 */

	Evas_Coord eWidth = 0, eHeight = 0;
	evas_object_geometry_get(view->base.module->win, NULL, NULL, &eWidth, &eHeight);
	debug_log("eWidth : %d, eHeight : %d", eWidth, eHeight);

	if (view->base.orientation == APP_DEVICE_ORIENTATION_0 || view->base.orientation ==  APP_DEVICE_ORIENTATION_180) {
		mouse_x = ev->x;
		mouse_y = ev->y;
	} else if (view->base.orientation == APP_DEVICE_ORIENTATION_270) {
		mouse_x = ev->y;
		mouse_y = eHeight - ev->x;
	} else if (view->base.orientation ==  APP_DEVICE_ORIENTATION_90) {
		mouse_x = eWidth - ev->y;
		mouse_y = ev->x;
	}

	debug_secure("Rotation: [%d] ", view->base.orientation);
	debug_secure("Final mouse down point: [x:%d,y:%d]", mouse_x, mouse_y);

	evas_object_geometry_get(view->ps_layout, &x, &y, &w, &h); /* it retrurns geometry position base on combination window and rotation */
	edje_object_part_geometry_get(_EDJ(view->ps_layout), "ec.swallow.content", &part_x, &part_y, &part_w, &part_h); /* it retrurns relative geometry position base on layout */

	int start_x = x + part_x;
	int end_x = start_x + part_w;
	int start_y = y + part_y;
	int end_y = start_y + part_h;

	debug_secure("layout position: [%d,%d] ~ [%d,%d]", start_x, start_y, end_x, end_y);

	if ((((mouse_x < start_x || mouse_x > end_x) || (mouse_y < start_y || mouse_y > end_y)) && view->ps_is_runnig)) {
		composer_ps_stop_search(view);
	}

	debug_leave();
	return EINA_TRUE;
}

static void __composer_ps_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	composer_ps_stop_search(data);

	debug_leave();
}

static void __composer_ps_gl_sel(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");
	retm_if(!event_info, "Invalid parameter: event_info is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	email_contact_list_info_t *contact_info = (email_contact_list_info_t *)elm_object_item_data_get(item);
	Evas_Object *mbe = NULL;

	elm_genlist_item_selected_set(item, EINA_FALSE);

	if (view->selected_widget == view->recp_to_entry.entry) {
		mbe = view->recp_to_mbe;
	} else if (view->selected_widget == view->recp_cc_entry.entry) {
		mbe = view->recp_cc_mbe;
	} else if (view->selected_widget == view->recp_bcc_entry.entry) {
		mbe = view->recp_bcc_mbe;
	}
	retm_if(!mbe, "Invalid entry is selected!");

	EmailRecpInfo *ri = NULL;
	if(contact_info->contact_origin == EMAIL_SEARCH_CONTACT_ORIGIN_CONTACTS) {
		ri = composer_util_recp_make_recipient_info_with_display_name(contact_info->email_address, contact_info->display_name);
		if (ri) {
			ri->email_id = contact_info->email_id;
		}
	} else {
		ri = composer_util_recp_make_recipient_info(contact_info->email_address);
		if (ri) {
			ri->email_id = 0;
		}
	}

	if (ri) {
		elm_entry_entry_set(view->selected_widget, "");
		char *markup_name = elm_entry_utf8_to_markup(ri->display_name);
		elm_multibuttonentry_item_append(mbe, markup_name, NULL, ri);
		FREE(markup_name);
	} else {
		debug_error("ri is NULL!");
	}

	if (view->ps_is_runnig) {
		composer_ps_stop_search(view);
	}

	debug_leave();
}

static char *__composer_pslines_gl_text_get(void *data, Evas_Object *obj, const char *part)
{
	email_contact_list_info_t *contact_info = (email_contact_list_info_t *)data;
	EmailComposerView *view = (EmailComposerView *)contact_info->view;
	retvm_if(!view, NULL, "Failed to get view");

	char *ret = NULL;
	if (contact_info->contact_origin == EMAIL_SEARCH_CONTACT_ORIGIN_CONTACTS) {
		if (!g_strcmp0(part, "elm.text")) {
				ret = __composer_ps_insert_match_tag(contact_info->display_name, view->ps_keyword);
				if (!ret) {
					ret = elm_entry_utf8_to_markup(contact_info->display_name);
				}
			} else if (!g_strcmp0(part, "elm.text.sub")) {
				ret = __composer_ps_insert_match_tag(contact_info->email_address, view->ps_keyword);
				if (!ret) {
					ret = elm_entry_utf8_to_markup(contact_info->email_address);
				}
			}
	} else if (contact_info->contact_origin == EMAIL_SEARCH_CONTACT_ORIGIN_RECENT) {
		if (!g_strcmp0(part, "elm.text")) {
				ret = __composer_ps_insert_match_tag(contact_info->email_address, view->ps_keyword);
				if (!ret) {
					ret = elm_entry_utf8_to_markup(contact_info->email_address);
				}
			}
	}

	return ret;
}

static char *__composer_ps_insert_match_tag(char *input, char *matched_word)
{
	retvm_if(!input, NULL, "Invalid parameter: input is NULL!");
	retvm_if(!matched_word, NULL, "Invalid parameter: matched_word is NULL!");

	char *found = (char *)strcasestr(input, matched_word);
	retv_if(!found, NULL);

	char *string = NULL;
	char *first = NULL, *matched = NULL, *last = NULL;

	char *temp = strndup(input, found - input);
	first = elm_entry_utf8_to_markup(temp);
	matched = strndup(found, strlen(matched_word));
	last = elm_entry_utf8_to_markup(input + strlen(matched_word) + (temp ? strlen(temp) : 0));
	g_free(temp);

	string = g_strconcat(first, COMPOSER_TAG_MATCH_PREFIX, matched, COMPOSER_TAG_MATCH_SUFFIX, last, NULL);

	FREE(first);
	FREE(matched);
	FREE(last);

	return string;
}

static void __composer_ps_genlist_size_hint_changed(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailComposerView *view = data;

	int genlist_min_height = 0;
	evas_object_size_hint_min_get(view->ps_genlist, NULL, &genlist_min_height);
	if (genlist_min_height != view->ps_genlist_min_height) {
		view->ps_genlist_min_height = genlist_min_height;
		composer_ps_change_layout_size(view);
	}
}

Evas_Object *__composer_ps_create_genlist(Evas_Object *parent, EmailComposerView *view)
{
	debug_enter();

	__ps_itc.item_style = "type1";
	__ps_itc.func.text_get = __composer_pslines_gl_text_get;
	__ps_itc.func.content_get = NULL;
	__ps_itc.func.state_get = NULL;
	__ps_itc.func.del = NULL;

	Evas_Object *layout = elm_layout_add(parent);
	elm_layout_file_set(layout, email_get_composer_theme_path(), "ec/predictive_search/base");
	evas_object_hide(layout);

	Evas_Object *grid = elm_grid_add(layout);
	evas_object_show(grid);

	Evas_Object *genlist = elm_genlist_add(grid);
	elm_object_focus_allow_set(genlist, EINA_FALSE);
	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
	elm_genlist_homogeneous_set(genlist, EINA_FALSE);
	elm_scroller_content_min_limit(genlist, EINA_FALSE, EINA_TRUE);
	evas_object_show(genlist);

	elm_grid_pack(grid, genlist, 0, 0, 100, 100);
	elm_object_part_content_set(layout, "ec.swallow.content", grid);
	eext_object_event_callback_add(layout, EEXT_CALLBACK_BACK, __composer_ps_back_cb, view);
	view->ps_genlist = genlist;

	view->ps_genlist_min_height = 0;

	debug_leave();
	return layout;
}

static void __composer_ps_create_view(EmailComposerView *view)
{
	debug_enter();

	retm_if(!view, "Invalid parameter: view is NULL!");

	/* Create Predictive search field */
	view->ps_layout = __composer_ps_create_genlist(view->composer_layout, view);

	if (!view->ps_mouse_down_handler) {
		view->ps_mouse_down_handler = ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN, __composer_ps_mouse_down_cb, view);
	}

	evas_object_event_callback_add(view->ps_genlist, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
			__composer_ps_genlist_size_hint_changed, view);

	evas_object_resize(view->ps_layout, COMPOSER_PS_INITAL_WIDTH, 0); /* To force size calculations */

	composer_initial_view_cs_freeze_push(view);

	view->ps_is_runnig = EINA_TRUE;

	debug_leave();
}

static void __composer_ps_destroy_view(EmailComposerView *view)
{
	debug_enter();

	retm_if(!view, "Invalid parameter: view is NULL!");

	if (view->ps_mouse_down_handler) {
		ecore_event_handler_del(view->ps_mouse_down_handler);
		view->ps_mouse_down_handler = NULL;
	}

	eext_object_event_callback_del(view->ps_layout, EEXT_CALLBACK_BACK, __composer_ps_back_cb);
	evas_object_event_callback_del_full(view->ps_genlist, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
			__composer_ps_genlist_size_hint_changed, view);
	DELETE_EVAS_OBJECT(view->ps_layout);
	view->ps_genlist = NULL;

	composer_initial_view_cs_freeze_pop(view);

	view->ps_is_runnig = EINA_FALSE;

	debug_leave();
}

static void __composer_ps_append_result(EmailComposerView *view, Eina_List *predict_list, Elm_Object_Item *parent)
{
	debug_enter();

	retm_if(!view, "Invalid parameter: view is NULL!");
	retm_if(!view->ps_genlist, "Invalid parameter: view is NULL!");
	retm_if(!predict_list, "Invalid parameter: view is NULL!");

	Eina_List *l = NULL;
	email_contact_list_info_t *item = NULL;
	EINA_LIST_FOREACH(predict_list, l, item) {
		item->view = view;
		elm_genlist_item_append(view->ps_genlist, &__ps_itc, item, NULL, ELM_GENLIST_ITEM_NONE, __composer_ps_gl_sel, view);
	}

	debug_leave();
}

static Eina_List *__composer_ps_search_through_list(Eina_List *src, const char *search_word)
{
	debug_enter();

	retvm_if(!src, NULL, "Invalid parameter: src is NULL!");
	retvm_if(!search_word, NULL, "Invalid parameter: search_word is NULL!");

	Eina_List *result = NULL;
	Eina_List *l = NULL;
	email_contact_list_info_t *item = NULL;

	EINA_LIST_FOREACH(src, l, item) {
		if (item && (strcasestr(item->display_name, search_word) || (item->email_address && (strcasestr(item->email_address, search_word))))) {
			result = eina_list_append(result, item);
		}
	}

	debug_leave();
	return result;
}

static void composer_ps_selected_recipient_editfield_get(EmailComposerView *view, email_editfield_t **editfield)
{
	debug_enter();
	retm_if(!view, "Invalid parameter: view is NULL!");
	retm_if(!editfield, "Invalid parameter: editfield is NULL!");

	*editfield = NULL;

	if (view->selected_widget == view->recp_to_entry.entry) {
		*editfield = &view->recp_to_entry;
	} else if (view->selected_widget == view->recp_cc_entry.entry) {
		*editfield = &view->recp_cc_entry;
	} else if (view->selected_widget == view->recp_bcc_entry.entry) {
		*editfield = &view->recp_bcc_entry;
	}
}

/*
 * Definition for exported functions
 */

void composer_ps_start_search(void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;
	Eina_List *ps_list = NULL;

	/* Retrieve contacts list from contacts db & phonelog db. If there's the list already searched, use it instead. */
	if (!view->ps_contacts_list) {

		view->ps_contacts_list = email_contacts_search_contacts_by_keyword(view->ps_keyword); /* contacts_list is cached for future use. */
		ps_list = eina_list_clone(view->ps_contacts_list);
	} else {
		ps_list = __composer_ps_search_through_list(view->ps_contacts_list, view->ps_keyword); /* cached contacts_list is used here. */
	}

	/* If there're some contacts found, append it to the genlist. If not, destroy the layout. */
	if (!ps_list) {
		if (view->ps_is_runnig) {
			composer_ps_stop_search(view);
		}
	} else {
		if (!view->ps_is_runnig) {
			__composer_ps_create_view(view);
		} else if (view->ps_genlist) { /* Clear existing items to append new list. */
			elm_genlist_clear(view->ps_genlist);
		}

		__composer_ps_append_result(view, ps_list, NULL);
		DELETE_LIST_OBJECT(ps_list); /* DO NOT remove the item on the list. they are removed on composer_ps_stop_search() */
	}

	debug_leave();
}

void composer_ps_stop_search(void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	if (view->ps_is_runnig) {
		__composer_ps_destroy_view(view);
	}

	if (view->ps_contacts_list) {
		email_contacts_delete_contact_info_list(&view->ps_contacts_list);
	}

	debug_leave();
}

void composer_ps_change_layout_size(EmailComposerView *view)
{
	debug_enter();
	retm_if(!view, "Invalid parameter: view is NULL!");

	if (view->ps_genlist_min_height == 0) {
		debug_log("Genlist min height is 0");
		evas_object_hide(view->ps_layout);
		return;
	}

	email_editfield_t *editfield = NULL;
	int scroller_y = 0;
	int scroller_h = 0;

	composer_ps_selected_recipient_editfield_get(view, &editfield);
	retm_if(!editfield, "Failed to get selected recipient editfield");

	evas_object_geometry_get(view->main_scroller, NULL, &scroller_y, NULL, &scroller_h);

	const int target_h = (view->ps_genlist_min_height + COMPOSER_PS_BORDER_SIZE * 2);

	debug_log("scroller_y: %d; scroller_h: %d; target_h: %d", scroller_y, scroller_h, target_h);

	int i = 0;
	for (i = 0; i < 2; ++i) {
		int composer_layout_y = 0;
		int editfield_layout_x = 0;
		int editfield_layout_y = 0;
		int editfield_layout_w = 0;
		int editfield_layout_h = 0;

		evas_object_geometry_get(view->composer_layout, NULL, &composer_layout_y, NULL, NULL);
		evas_object_geometry_get(editfield->layout, &editfield_layout_x, &editfield_layout_y,
				&editfield_layout_w, &editfield_layout_h);

		debug_log("composer_layout_y: %d; editfield_layout_y: %d; editfield_layout_h: %d",
				composer_layout_y, editfield_layout_y, editfield_layout_h);

		const int optimal_scroll_pos = (editfield_layout_y - composer_layout_y);
		const int extra_h = (editfield_layout_y - scroller_y);

		debug_log("optimal_scroll_pos: %d; extra_h: %d", optimal_scroll_pos, extra_h);

		if ((i == 0) && (extra_h < 0)) {
			debug_log("extra_h < 0 - scroll to optimal position!");
			composer_initial_view_cs_show(view, optimal_scroll_pos);
			continue;
		}

		const int avail_top = (editfield_layout_y + editfield_layout_h);
		const int avail_bottom = (scroller_y + scroller_h - COMPOSER_PS_BOTTOM_PADDING);
		const int avail_h = (avail_bottom - avail_top);
		const int max_h = (avail_h + extra_h);

		debug_log("avail_top: %d; avail_bottom: %d; avail_h: %d; max_h: %d",
			avail_top, avail_bottom, avail_h, max_h);

		if ((i == 0) && (target_h > avail_h) && (avail_h < max_h)) {
			debug_log("avail_h < max_h - adjusting scroll position...");
			composer_initial_view_cs_show(view, optimal_scroll_pos - MAX(0, max_h - target_h));
			continue;
		}

		const int result_h = MIN(avail_h, target_h);

		debug_log("result_h: %d", result_h);

		evas_object_move(view->ps_layout, editfield_layout_x, avail_top);
		evas_object_resize(view->ps_layout, editfield_layout_w, result_h);
		evas_object_show(view->ps_layout);

		break;
	}

	debug_leave();
}
