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

#define COMPOSER_PREDICTIVE_LAYOUT_BOTTOM_MASKING_ADJ ELM_SCALE_SIZE(16) /* It is to take care of the bottom bg & effect of the layout */

Elm_Genlist_Item_Class __ps_itc;


/*
 * Declaration for static functions
 */

static Eina_Bool __composer_ps_mouse_down_cb(void *data, int type, void *event);
static void __composer_ps_back_cb(void *data, Evas_Object *obj, void *event_info);
static void __composer_ps_gl_sel(void *data, Evas_Object *obj, void *event_info);
static char *__composer_pslines_gl_text_get(void *data, Evas_Object *obj, const char *part);


static char *__composer_ps_insert_match_tag(char *input, char *matched_word);
Evas_Object *__composer_ps_create_genlist(Evas_Object *parent, EmailComposerUGD *ugd);
static void __composer_ps_create_view(EmailComposerUGD *ugd);
static void __composer_ps_destroy_view(EmailComposerUGD *ugd);

static Eina_Bool _composer_correct_ps_layout_size_timer_cb(void *data);
static void composer_ps_selected_recipient_content_get(EmailComposerUGD *ugd, Evas_Object **recipient_layout, Evas_Object **editfield_layout);

static void __composer_ps_append_result(EmailComposerUGD *ugd, Eina_List *predict_list, Elm_Object_Item *parent);
static Eina_List *__composer_ps_search_through_list(Eina_List *src, const char *search_word);

/*
 * Definition for static functions
 */

static Eina_Bool __composer_ps_mouse_down_cb(void *data, int type, void *event)
{
	debug_enter();

	retvm_if(!data, EINA_TRUE, "Invalid parameter: data is NULL!");
	retvm_if(!event, EINA_TRUE, "Invalid parameter: event is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *) data;
	Ecore_Event_Mouse_Button *ev = (Ecore_Event_Mouse_Button *)event;

	Evas_Coord x = 0, y = 0, w = 0, h = 0;
	Evas_Coord part_x = 0, part_y = 0, part_w = 0, part_h = 0;
	int mouse_x = 0, mouse_y = 0;

	/* Convert mouse position based on orientation.
	 * Because root point(R0) of mouse down is not changed according to rotation, but, root point of predictsearch layout is changed according to rotation.
	 * for COMPOSER_ROTATE_PORTRAIT and COMPOSER_ROTATE_PORTRAIT_UPSIDEDOWN different values are required if we were to implement reverse portrait mode
	 */

	Evas_Coord eWidth = 0, eHeight = 0;
	evas_object_geometry_get(ugd->base.module->win, NULL, NULL, &eWidth, &eHeight);
	debug_log("eWidth : %d, eHeight : %d", eWidth, eHeight);

	if (ugd->base.orientation == APP_DEVICE_ORIENTATION_0 || ugd->base.orientation ==  APP_DEVICE_ORIENTATION_180) {
		mouse_x = ev->x;
		mouse_y = ev->y;
	} else if (ugd->base.orientation == APP_DEVICE_ORIENTATION_270) {
		mouse_x = ev->y;
		mouse_y = eHeight - ev->x;
	} else if (ugd->base.orientation ==  APP_DEVICE_ORIENTATION_90) {
		mouse_x = eWidth - ev->y;
		mouse_y = ev->x;
	}

	debug_secure("Rotation: [%d] ", ugd->base.orientation);
	debug_secure("Final mouse down point: [x:%d,y:%d]", mouse_x, mouse_y);

	evas_object_geometry_get(ugd->ps_layout, &x, &y, &w, &h); /* it retrurns geometry position base on combination window and rotation */
	edje_object_part_geometry_get(_EDJ(ugd->ps_layout), "ec.swallow.content", &part_x, &part_y, &part_w, &part_h); /* it retrurns relative geometry position base on layout */

	int start_x = x + part_x;
	int end_x = start_x + part_w;
	int start_y = y + part_y;
	int end_y = start_y + part_h;

	debug_secure("layout position: [%d,%d] ~ [%d,%d]", start_x, start_y, end_x, end_y);

	if ((((mouse_x < start_x || mouse_x > end_x) || (mouse_y < start_y || mouse_y > end_y)) && ugd->ps_is_runnig)) {
		composer_ps_stop_search(ugd);
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

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	email_contact_list_info_t *contact_info = (email_contact_list_info_t *)elm_object_item_data_get(item);
	Evas_Object *mbe = NULL;

	elm_genlist_item_selected_set(item, EINA_FALSE);

	if (ugd->selected_entry == ugd->recp_to_entry.entry) {
		mbe = ugd->recp_to_mbe;
	} else if (ugd->selected_entry == ugd->recp_cc_entry.entry) {
		mbe = ugd->recp_cc_mbe;
	} else if (ugd->selected_entry == ugd->recp_bcc_entry.entry) {
		mbe = ugd->recp_bcc_mbe;
	}
	retm_if(!mbe, "Invalid entry is selected!");

	EmailRecpInfo *ri = NULL;
	if(contact_info->contact_origin == EMAIL_SEARCH_CONTACT_ORIGIN_CONTACTS && contact_info->display_name) {
		ri = composer_util_recp_make_recipient_info_with_display_name(contact_info->email_address, contact_info->display_name);
		ri->email_id = contact_info->email_id;
	} else {
		ri = composer_util_recp_make_recipient_info(contact_info->email_address);
		ri->email_id = 0;
	}

	if (ri) {
		elm_entry_entry_set(ugd->selected_entry, "");
		char *markup_name = elm_entry_utf8_to_markup(ri->display_name);
		elm_multibuttonentry_item_append(mbe, markup_name, NULL, ri);
		FREE(markup_name);
	} else {
		debug_error("ri is NULL!");
	}

	if (ugd->ps_is_runnig) {
		composer_ps_stop_search(ugd);
	}

	debug_leave();
}

static char *__composer_pslines_gl_text_get(void *data, Evas_Object *obj, const char *part)
{
	email_contact_list_info_t *contact_info = (email_contact_list_info_t *)data;
	EmailComposerUGD *ugd = (EmailComposerUGD *)contact_info->ugd;
	retvm_if(!ugd, NULL, "Failed to get ugd");

	char *ret = NULL;
	if (contact_info->contact_origin == EMAIL_SEARCH_CONTACT_ORIGIN_CONTACTS) {
		if (!g_strcmp0(part, "elm.text")) {
				ret = __composer_ps_insert_match_tag(contact_info->display_name, ugd->ps_keyword);
				if (!ret) {
					ret = elm_entry_utf8_to_markup(contact_info->display_name);
				}
			} else if (!g_strcmp0(part, "elm.text.sub")) {
				ret = __composer_ps_insert_match_tag(contact_info->email_address, ugd->ps_keyword);
				if (!ret) {
					ret = elm_entry_utf8_to_markup(contact_info->email_address);
				}
			}
	} else if (contact_info->contact_origin == EMAIL_SEARCH_CONTACT_ORIGIN_RECENT) {
		if (!g_strcmp0(part, "elm.text")) {
				ret = __composer_ps_insert_match_tag(contact_info->email_address, ugd->ps_keyword);
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

Evas_Object *__composer_ps_create_genlist(Evas_Object *parent, EmailComposerUGD *ugd)
{
	debug_enter();

	__ps_itc.item_style = "type1";
	__ps_itc.func.text_get = __composer_pslines_gl_text_get;
	__ps_itc.func.content_get = NULL;
	__ps_itc.func.state_get = NULL;
	__ps_itc.func.del = NULL;

	Evas_Object *layout = elm_layout_add(parent);
	elm_layout_file_set(layout, email_get_composer_theme_path(), "ec/predictive_search/base");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(layout);

	Evas_Object *box = elm_box_add(layout);
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(box);

	Evas_Object *genlist = elm_genlist_add(box);
	evas_object_size_hint_weight_set(genlist, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(genlist, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_object_focus_allow_set(genlist, EINA_FALSE);
	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
	elm_genlist_homogeneous_set(genlist, EINA_TRUE);
	evas_object_show(genlist);

	elm_box_pack_end(box, genlist);
	elm_object_part_content_set(layout, "ec.swallow.content", box);
	eext_object_event_callback_add(layout, EEXT_CALLBACK_BACK, __composer_ps_back_cb, ugd);
	ugd->ps_genlist = genlist;
	ugd->ps_box = box;

	debug_leave();
	return layout;
}

static void __composer_ps_create_view(EmailComposerUGD *ugd)
{
	debug_enter();

	retm_if(!ugd, "Invalid parameter: ugd is NULL!");

	/* Create Predictive search field */
	ugd->ps_layout = __composer_ps_create_genlist(ugd->composer_layout, ugd);

	if (!ugd->ps_mouse_down_handler) {
		ugd->ps_mouse_down_handler = ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN, __composer_ps_mouse_down_cb, ugd);
	}

	composer_initial_view_cs_freeze_push(ugd);

	ugd->ps_is_runnig = EINA_TRUE;

	debug_leave();
}

static void __composer_ps_destroy_view(EmailComposerUGD *ugd)
{
	debug_enter();

	retm_if(!ugd, "Invalid parameter: ugd is NULL!");

	if (ugd->ps_mouse_down_handler) {
		ecore_event_handler_del(ugd->ps_mouse_down_handler);
		ugd->ps_mouse_down_handler = NULL;
	}

	eext_object_event_callback_del(ugd->ps_layout, EEXT_CALLBACK_BACK, __composer_ps_back_cb);
	DELETE_EVAS_OBJECT(ugd->ps_layout);
	ugd->ps_genlist = NULL;
	ugd->ps_box = NULL;

	composer_initial_view_cs_freeze_pop(ugd);

	ugd->ps_is_runnig = EINA_FALSE;

	debug_leave();
}

static void __composer_ps_append_result(EmailComposerUGD *ugd, Eina_List *predict_list, Elm_Object_Item *parent)
{
	debug_enter();

	retm_if(!ugd, "Invalid parameter: ugd is NULL!");
	retm_if(!ugd->ps_genlist, "Invalid parameter: ugd is NULL!");
	retm_if(!predict_list, "Invalid parameter: ugd is NULL!");

	Eina_List *l = NULL;
	email_contact_list_info_t *item = NULL;
	EINA_LIST_FOREACH(predict_list, l, item) {
		item->ugd = ugd;
		elm_genlist_item_append(ugd->ps_genlist, &__ps_itc, item, NULL, ELM_GENLIST_ITEM_NONE, __composer_ps_gl_sel, ugd);
	}

	composer_ps_change_layout_size(ugd);

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

static int _composer_ps_genlist_item_max_height_calculate(EmailComposerUGD  *ugd)
{
	debug_enter();
	retvm_if(!ugd, 0, "Invalid parameter: ugd is NULL!");

	int item_height = 0;
	Elm_Object_Item *gl_item = NULL;

	Eina_List *relized_items = elm_genlist_realized_items_get(ugd->ps_genlist);
	retvm_if(!relized_items, item_height, "Error: relized_items is NULL!");

	gl_item = eina_list_data_get(relized_items);
	Evas_Object *track_rect = elm_object_item_track(gl_item);
	retvm_if(!track_rect, item_height, "Error: track_rect is NULL!");

	evas_object_geometry_get(track_rect, NULL, NULL, NULL, &item_height);
	elm_object_item_untrack(gl_item);
	EINA_LIST_FREE(relized_items, gl_item);

	debug_leave();
	return item_height;
}

static Eina_Bool _composer_correct_ps_layout_size_timer_cb(void *data)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	Evas_Object *recipient_layout = NULL, *editfield_layout = NULL;
	Evas_Coord recipient_layout_y = 0, recipient_layout_h = 0;
	Evas_Coord editfield_layout_width = 0, editfield_layout_x = 0;

	double total_height = 0, available_height = 0, item_height = 0, minimum_height = 0;
	double box_height = 0;
	int count = 0;

	Ecore_IMF_Context *ctx = ecore_imf_context_add(ecore_imf_context_default_id_get());
	Evas_Coord cx = 0, cy = 0, cw = 0, ch = 0;
	ecore_imf_context_input_panel_geometry_get(ctx, &cx, &cy, &cw, &ch);
	ecore_imf_context_del(ctx);

	Evas_Coord nHeight = 0;
	evas_object_geometry_get(ugd->base.module->win, NULL, NULL, NULL, &nHeight);
	composer_ps_selected_recipient_content_get(ugd, &recipient_layout, &editfield_layout);
	retvm_if((!recipient_layout || !editfield_layout), ECORE_CALLBACK_CANCEL, "Invalid mbe entry is selected!");

	evas_object_geometry_get(recipient_layout, NULL, &recipient_layout_y, NULL, &recipient_layout_h);
	evas_object_geometry_get(editfield_layout, &editfield_layout_x, NULL, &editfield_layout_width, NULL);

	if (recipient_layout_y < ugd->cs_top) {
		composer_initial_view_cs_show(ugd, ugd->cs_scroll_pos - (ugd->cs_top - recipient_layout_y));
		recipient_layout_y = ugd->cs_top;
	}
	item_height = _composer_ps_genlist_item_max_height_calculate(ugd);
	retvm_if(item_height == 0, ECORE_CALLBACK_CANCEL, "Failed to calculate item height!");

	count = elm_genlist_items_count(ugd->ps_genlist);
	total_height = count * item_height;
	available_height = nHeight - (recipient_layout_y + recipient_layout_h + COMPOSER_PREDICTIVE_LAYOUT_BOTTOM_MASKING_ADJ + ch);
	int display_item_count = (int) (available_height / item_height);
	debug_log("display_item_count: [%d]", display_item_count);

	int rot = elm_win_rotation_get(ugd->base.module->win);
	if ((rot == 0) || (rot == 180)) { /* Portrait */
		if (count > display_item_count) {
			minimum_height = item_height * display_item_count;
		} else {
			minimum_height = total_height;
		}
	} else { /* Landscape */
		minimum_height = item_height;
	}

	debug_log("item:[%f], avail:[%f], minimum:[%f], total:[%f]", item_height, available_height, minimum_height, total_height);
	composer_util_display_position(ugd);

	/* Move scroller to show the list. (Landscape: at least 1, Portrait: at least 3) */
	if (available_height < minimum_height) {
		int delta = minimum_height - available_height;
		int new_scroll_pos = ugd->cs_scroll_pos + delta;

		recipient_layout_y -= delta;

		debug_log("ugd->cs_scroll_pos:[%d]", ugd->cs_scroll_pos);
		debug_log("new_scroll_pos:[%d]", new_scroll_pos);

		composer_initial_view_cs_show(ugd, new_scroll_pos);
	}

	/* Calculate box height. */
	if (total_height < available_height) {
		box_height = total_height;
	} else if (minimum_height < available_height) {
		box_height = available_height;
	} else {
		box_height = minimum_height;
	}

	debug_log("==> box_height: (%f)", box_height);

	evas_object_size_hint_min_set(ugd->ps_box, 0, box_height);
	evas_object_size_hint_max_set(ugd->ps_box, -1, box_height);

	evas_object_resize(ugd->ps_layout, editfield_layout_width, box_height + ELM_SCALE_SIZE(4)); /* 4 means top line and bottom line on the layout */
	evas_object_move(ugd->ps_layout, editfield_layout_x, recipient_layout_y + recipient_layout_h);

	debug_leave();
	return ECORE_CALLBACK_CANCEL;

}

static void composer_ps_selected_recipient_content_get(EmailComposerUGD *ugd, Evas_Object **recipient_layout, Evas_Object **editfield_layout)
{
	debug_enter();
	*recipient_layout = NULL;
	*editfield_layout = NULL;
	retm_if(!ugd, "Invalid parameter: ugd is NULL!");

	if (ugd->selected_entry == ugd->recp_to_entry.entry) {
		*recipient_layout = ugd->recp_to_layout;
		*editfield_layout = ugd->recp_to_entry.layout;
	} else if (ugd->selected_entry == ugd->recp_cc_entry.entry) {
		*recipient_layout = ugd->recp_cc_layout;
		*editfield_layout = ugd->recp_cc_entry.layout;
	} else if (ugd->selected_entry == ugd->recp_bcc_entry.entry) {
		*recipient_layout = ugd->recp_bcc_layout;
		*editfield_layout = ugd->recp_bcc_entry.layout;
	}

	debug_leave();

}

/*
 * Definition for exported functions
 */

void composer_ps_start_search(void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	Eina_List *ps_list = NULL;

	/* Retrieve contacts list from contacts db & phonelog db. If there's the list already searched, use it instead. */
	if (!ugd->ps_contacts_list) {

		ugd->ps_contacts_list = email_contacts_search_contacts_by_keyword(ugd->ps_keyword); /* contacts_list is cached for future use. */
		ps_list = eina_list_clone(ugd->ps_contacts_list);
	} else {
		ps_list = __composer_ps_search_through_list(ugd->ps_contacts_list, ugd->ps_keyword); /* cached contacts_list is used here. */
	}

	/* If there're some contacts found, append it to the genlist. If not, destroy the layout. */
	if (!ps_list) {
		if (ugd->ps_is_runnig) {
			composer_ps_stop_search(ugd);
		}
	} else {
		if (!ugd->ps_is_runnig) {
			__composer_ps_create_view(ugd);
		} else if (ugd->ps_genlist) { /* Clear existing items to append new list. */
			elm_genlist_clear(ugd->ps_genlist);
		}

		__composer_ps_append_result(ugd, ps_list, NULL);
		DELETE_LIST_OBJECT(ps_list); /* DO NOT remove the item on the list. they are removed on composer_ps_stop_search() */
	}

	debug_leave();
}

void composer_ps_stop_search(void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	if (ugd->ps_is_runnig) {
		__composer_ps_destroy_view(ugd);
	}

	if (ugd->ps_contacts_list) {
		composer_util_recp_delete_contacts_list(ugd->ps_contacts_list);
		ugd->ps_contacts_list = NULL;
	}

	debug_leave();
}

void composer_ps_change_layout_size(EmailComposerUGD *ugd)
{
	debug_enter();

	Evas_Object *recipient_layout = NULL, *editfield_layout = NULL;
	composer_ps_selected_recipient_content_get(ugd, &recipient_layout, &editfield_layout);
	retm_if(!recipient_layout || !editfield_layout, "Failed to get recipient content");

	Evas_Coord recipient_layout_y = 0, recipient_layout_h = 0;
	Evas_Coord editfield_layout_width = 0, editfield_layout_x = 0;


	evas_object_geometry_get(recipient_layout, NULL, &recipient_layout_y, NULL, &recipient_layout_h);
	evas_object_geometry_get(editfield_layout, &editfield_layout_x, NULL, &editfield_layout_width, NULL);

	evas_object_resize(ugd->ps_layout, editfield_layout_width, COMPOSER_PREDICTIVE_SEARCH_ITEM_DEFAULT_HEIGHT); /* Initial resize to calculate genlist item height */
	evas_object_move(ugd->ps_layout, editfield_layout_x, recipient_layout_y + recipient_layout_h);

	ecore_timer_add(0.1f, _composer_correct_ps_layout_size_timer_cb, ugd);
	debug_leave();
}
