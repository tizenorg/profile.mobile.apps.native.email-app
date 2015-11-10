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

#include "email-popup-utils.h"

#include "email-utils.h"
#include "email-debug.h"

#define COMMON_POPUP_SHOW_TIMEOUT_SEC 0.4

static struct info {
	Evas_Object *popup;
	Ecore_Timer *popup_show_timer;

} s_info = {
	.popup = NULL,
	.popup_show_timer = NULL
};

static void _common_util_create_popup_button(Evas_Object *popup, const char *part,
		EmailCommonStringType btn_text, Evas_Smart_Cb click_cb, void *cb_data);

static void _common_util_show_popup_with_timer();
static Eina_Bool _common_util_show_popup_timer_cb(void *data);
static void _common_util_popup_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void _common_util_popup_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();

	s_info.popup = NULL;

	DELETE_TIMER_OBJECT(s_info.popup_show_timer);
}

static Eina_Bool _common_util_show_popup_timer_cb(void *data)
{
	debug_enter();

	evas_object_show(s_info.popup);

	s_info.popup_show_timer = NULL;

	return ECORE_CALLBACK_CANCEL;
}

static void _common_util_show_popup_with_timer()
{
	debug_enter();

	DELETE_IDLER_OBJECT(s_info.popup_show_timer);
	s_info.popup_show_timer = ecore_timer_add(COMMON_POPUP_SHOW_TIMEOUT_SEC, _common_util_show_popup_timer_cb, NULL);
}

static void _common_util_create_popup_button(Evas_Object *popup, const char *part,
		EmailCommonStringType btn_text, Evas_Smart_Cb click_cb, void *cb_data)
{
	Evas_Object *btn = elm_button_add(popup);
	elm_object_style_set(btn, "bottom");
	evas_object_show(btn);

	if (btn_text.domain) {
		elm_object_domain_translatable_text_set(btn, btn_text.domain, btn_text.id);
	} else {
		elm_object_text_set(btn, btn_text.id);
	}

	elm_object_part_content_set(popup, part, btn);
	evas_object_smart_callback_add(btn, "clicked", click_cb, cb_data);
}

Evas_Object *common_util_create_popup(Evas_Object *parent, EmailCommonStringType title,
		Evas_Smart_Cb btn1_click_cb, EmailCommonStringType btn1_text,
		Evas_Smart_Cb btn2_click_cb, EmailCommonStringType btn2_text,
		Evas_Smart_Cb btn3_click_cb, EmailCommonStringType btn3_text,
		Evas_Smart_Cb cancel_cb, Eina_Bool need_ime_hide, void *cb_data)
{
	debug_enter();
	retvm_if(!parent, NULL, "parent is NULL");

	DELETE_EVAS_OBJECT(s_info.popup);

	Evas_Object *popup = elm_popup_add(parent);
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_popup_align_set(popup, ELM_NOTIFY_ALIGN_FILL, 1.0);

	evas_object_event_callback_add(popup, EVAS_CALLBACK_DEL, _common_util_popup_del_cb, NULL);

	s_info.popup = popup;

	if (title.id) {
		if (title.domain) {
			elm_object_domain_translatable_part_text_set(popup, "title,text", title.domain, title.id);
		} else {
			elm_object_part_text_set(popup, "title,text", title.id);
		}
	}

	bool has_btn = false;

	if (btn1_click_cb && btn1_text.id) {
		_common_util_create_popup_button(popup, "button1", btn1_text, btn1_click_cb, cb_data);
		has_btn = true;
	}
	if (btn2_click_cb && btn2_text.id) {
		_common_util_create_popup_button(popup, "button2", btn2_text, btn2_click_cb, cb_data);
		has_btn = true;
	}
	if (btn3_click_cb && btn3_text.id) {
		_common_util_create_popup_button(popup, "button3", btn3_text, btn3_click_cb, cb_data);
		has_btn = true;
	}

	if (cancel_cb && !has_btn) {
		evas_object_smart_callback_add(popup, "block,clicked", cancel_cb, cb_data);
	}

	if (cancel_cb) {
		eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, cancel_cb, cb_data);
	}

	if (need_ime_hide && email_module_mgr_is_in_compressed_mode()) {
		ecore_imf_input_panel_hide();
		_common_util_show_popup_with_timer();
	} else {
		evas_object_show(popup);
	}

	debug_leave();
	return popup;
}

static int __common_util_popup_calculate_item_count(int item_height, int item_count, Eina_Bool is_horizontal)
{
	debug_enter();

	int calculated_count = item_count;

	if (is_horizontal) {
		if ((item_height == COMMON_POPUP_ITEM_1_LINE_HEIGHT) && (item_count > COMMON_POPUP_ITEM_1_LINE_COUNT_LANDSCAPE)) {
			calculated_count = COMMON_POPUP_ITEM_1_LINE_COUNT_LANDSCAPE;
		} else if ((item_height == COMMON_POPUP_ITEM_2_LINE_HEIGHT) && (item_count > COMMON_POPUP_ITEM_2_LINE_COUNT_LANDSCAPE)) {
			calculated_count = COMMON_POPUP_ITEM_2_LINE_COUNT_LANDSCAPE;
		}
	} else {
		if ((item_height == COMMON_POPUP_ITEM_1_LINE_HEIGHT) && (item_count > COMMON_POPUP_ITEM_1_LINE_COUNT_PORTRAIT)) {
			calculated_count = COMMON_POPUP_ITEM_1_LINE_COUNT_PORTRAIT;
		} else if ((item_height == COMMON_POPUP_ITEM_2_LINE_HEIGHT) && (item_count > COMMON_POPUP_ITEM_2_LINE_COUNT_PORTRAIT)) {
			calculated_count = COMMON_POPUP_ITEM_2_LINE_COUNT_PORTRAIT;
		}
	}

	debug_leave();
	return calculated_count;
}

void common_util_popup_display_genlist(Evas_Object *popup, Evas_Object *genlist, Eina_Bool is_horizontal, int item_height, int item_count)
{
	debug_enter();

	int calculated_count = __common_util_popup_calculate_item_count(item_height, item_count, is_horizontal);

	/* The height of "min_menustyle" popup is 0. We need to set the height of contents.
	 * So we use box to set the height because genlist has no height.
	 */
	Evas_Object *box = elm_box_add(popup);
	evas_object_size_hint_min_set(box, 0, ELM_SCALE_SIZE(item_height * calculated_count));

	debug_log("test_ height=%d", item_height * calculated_count);

	elm_scroller_policy_set(genlist, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);

	/* No need to set data when item_count is too small to resize. */
	if (((item_height == COMMON_POPUP_ITEM_1_LINE_HEIGHT) && ((calculated_count >= COMMON_POPUP_ITEM_1_LINE_COUNT_LANDSCAPE))) ||
		((item_height == COMMON_POPUP_ITEM_2_LINE_HEIGHT) && (calculated_count >= COMMON_POPUP_ITEM_2_LINE_COUNT_LANDSCAPE))) {
		evas_object_data_set(popup, COMMON_EVAS_DATA_NAME_POPUP_ITEM_COUNT, (void *)(ptrdiff_t)item_count);
		evas_object_data_set(popup, COMMON_EVAS_DATA_NAME_POPUP_ITEM_HEIGHT, (void *)(ptrdiff_t)item_height);
	}
	elm_box_pack_end(box, genlist);
	elm_object_content_set(popup, box);
	evas_object_show(genlist);

	debug_leave();
}

/* EOF */
