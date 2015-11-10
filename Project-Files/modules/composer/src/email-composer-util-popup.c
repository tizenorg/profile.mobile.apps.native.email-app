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

#include <string.h>
#include <Elementary.h>

#include "email-debug.h"
#include "email-utils.h"
#include "email-popup-utils.h"

#include "email-composer.h"
#include "email-composer-types.h"
#include "email-composer-util.h"


/*
 * Declarations for static variables
 */

static Elm_Genlist_Item_Class account_list_itc;
static EmailCommonStringType EMAIL_COMPOSER_STRING_NULL = { NULL, NULL };

/*
 * Definitions for static & exported functions
 */

Evas_Object *composer_util_popup_create(EmailComposerUGD *ugd, EmailCommonStringType t_title, EmailCommonStringType t_content, Evas_Smart_Cb response_cb,
                                        EmailCommonStringType t_btn1_lb, EmailCommonStringType t_btn2_lb, EmailCommonStringType t_btn3_lb)
{
	debug_enter();

	retvm_if(!ugd, NULL, "Invalid parameter: ugd is NULL!");
	/*retvm_if(!response_cb, NULL, "Invalid parameter: response_cb is NULL!");*/

	Evas_Object *popup = common_util_create_popup(ugd->base.module->win,
			t_title, response_cb, t_btn1_lb, response_cb, t_btn2_lb, response_cb, t_btn3_lb, response_cb, EINA_TRUE, ugd);

	if (t_content.id) {
		if (t_content.domain) {
			elm_object_domain_translatable_text_set(popup, t_content.domain, t_content.id);
		} else {
			elm_object_text_set(popup, t_content.id);
		}
	}

	debug_leave();
	return popup;
}

Evas_Object *composer_util_popup_create_with_progress_horizontal(EmailComposerUGD *ugd, EmailCommonStringType t_title, EmailCommonStringType t_content, Evas_Smart_Cb response_cb,
                                                                 EmailCommonStringType t_btn1_lb, EmailCommonStringType t_btn2_lb, EmailCommonStringType t_btn3_lb)
{
	debug_enter();

	retvm_if(!ugd, NULL, "Invalid parameter: ugd is NULL!");

	Evas_Object *popup = common_util_create_popup(ugd->base.module->win,
			t_title, response_cb, t_btn1_lb, response_cb, t_btn2_lb, response_cb, t_btn3_lb, response_cb, EINA_TRUE, ugd);

	Evas_Object *layout = elm_layout_add(popup);
	elm_layout_file_set(layout, email_get_composer_theme_path(), "ec/popup/processing_with_text_horizontal");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(layout);

	Evas_Object *progress = elm_progressbar_add(popup);
	elm_object_style_set(progress, "process_medium");
	elm_progressbar_pulse(progress, EINA_TRUE);
	evas_object_show(progress);

	elm_object_part_content_set(layout, "ec.swallow.content", progress);

	if (t_content.id) {
		if (t_content.domain) {
			elm_object_domain_translatable_text_set(layout, t_content.domain, t_content.id);
		} else {
			elm_object_text_set(layout, t_content.id);
		}
	}
	elm_object_content_set(popup, layout);

	debug_leave();
	return popup;
}

void composer_util_popup_response_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	DELETE_EVAS_OBJECT(ugd->composer_popup);

	Ewk_Settings *ewkSetting = ewk_view_settings_get(ugd->ewk_view);
	ewk_settings_clear_text_selection_automatically_set(ewkSetting, EINA_TRUE);

	if (evas_object_freeze_events_get(ugd->base.module->navi)) {
		evas_object_freeze_events_set(ugd->base.module->navi, EINA_FALSE);
	}

	if (evas_object_freeze_events_get(ugd->ewk_view)) {
		evas_object_freeze_events_set(ugd->ewk_view, EINA_FALSE);
	}

	if (ugd->selected_mbe_item) {
		elm_multibuttonentry_item_selected_set(ugd->selected_mbe_item, EINA_FALSE);
		ugd->selected_mbe_item = NULL;
	}

	elm_object_tree_focus_allow_set(ugd->composer_layout, EINA_TRUE);
	elm_object_focus_allow_set(ugd->selected_entry, EINA_TRUE);
	elm_object_focus_allow_set(ugd->subject_entry.entry, EINA_TRUE);
	elm_object_focus_allow_set(ugd->ewk_btn, EINA_TRUE);

	Evas_Object *target_entry = ugd->selected_entry;
	if (ugd->is_inline_contents_inserted) {
		ugd->is_inline_contents_inserted = EINA_FALSE;
		target_entry = ugd->ewk_view;
	}

	composer_util_focus_set_focus_with_idler(ugd, target_entry);

	debug_leave();
}

void composer_util_popup_resize_popup_for_rotation(Evas_Object *popup, Eina_Bool is_horizontal)
{
	debug_enter();

	int item_count = (int)(ptrdiff_t)evas_object_data_get(popup, COMPOSER_EVAS_DATA_NAME_POPUP_ITEM_COUNT);
	if (item_count) {
		int is_gengrid = (int)(ptrdiff_t)evas_object_data_get(popup, COMPOSER_EVAS_DATA_NAME_POPUP_IS_GENGRID);
		if (is_gengrid) {
			Evas_Object *layout = elm_object_content_get(popup);
			if (is_horizontal) {
				elm_layout_file_set(layout, email_get_composer_theme_path(), "ec/popup/gengrid/landscape");
			} else {
				if (item_count > 6) {
					elm_layout_file_set(layout, email_get_composer_theme_path(), "ec/popup/gengrid/portrait/3line");
				} else {
					elm_layout_file_set(layout, email_get_composer_theme_path(), "ec/popup/gengrid/portrait/2line");
				}
			}
			Evas_Object *gengrid = elm_object_part_content_get(layout, "elm.swallow.content");
			elm_gengrid_item_show(elm_gengrid_first_item_get(gengrid), ELM_GENGRID_ITEM_SCROLLTO_TOP);
		}
	}

	debug_leave();
}

static char *__composer_util_popup_account_gl_text_get(void *data, Evas_Object *obj, const char *part)
{
	EmailComposerUGD *ugd = (EmailComposerUGD *)evas_object_data_get(obj, COMPOSER_EVAS_DATA_NAME);
	retvm_if(!ugd, NULL, "ugd is NULL!");
	int index = (int)(ptrdiff_t)data;

	if (!strcmp(part, "elm.text")) {
		debug_secure("Account [%d] %s", ugd->account_info->account_list[index].account_id, ugd->account_info->account_list[index].user_email_address);
		if (ugd->account_info->account_list[index].user_email_address) {
			return strdup(ugd->account_info->account_list[index].user_email_address);
		}
	}

	return NULL;
}

void composer_util_popup_create_account_list_popup(void *data, Evas_Smart_Cb response_cb, Evas_Smart_Cb selected_cb, const char *style,
                                                   EmailCommonStringType t_title, EmailCommonStringType t_btn1_lb, EmailCommonStringType t_btn2_lb)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	ugd->composer_popup = common_util_create_popup(ugd->base.module->win,
			t_title, response_cb, t_btn1_lb, response_cb, t_btn2_lb, response_cb, EMAIL_COMPOSER_STRING_NULL, response_cb, EINA_TRUE, ugd);

	account_list_itc.item_style = "type1";
	account_list_itc.func.text_get = __composer_util_popup_account_gl_text_get;
	account_list_itc.func.content_get = NULL;
	account_list_itc.func.state_get = NULL;
	account_list_itc.func.del = NULL;

	Evas_Object *genlist = elm_genlist_add(ugd->composer_popup);
	evas_object_data_set(genlist, COMPOSER_EVAS_DATA_NAME, ugd);
	elm_genlist_homogeneous_set(genlist, EINA_TRUE);
	elm_scroller_policy_set(genlist, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
	elm_scroller_content_min_limit(genlist, EINA_FALSE, EINA_TRUE);

	int index;
	for (index = 0; index < ugd->account_info->account_count; index++) {
		elm_genlist_item_append(genlist, &account_list_itc, (void *)(ptrdiff_t)index, NULL, ELM_GENLIST_ITEM_NONE, selected_cb, (void *)ugd);
	}

	elm_object_content_set(ugd->composer_popup, genlist);
	evas_object_show(genlist);

	debug_leave();
}

void composer_util_popup_translate_do(void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	char buf[BUF_LEN_H * 10] = { 0, };
	char *(*func_get_string)(const char *);

	if (ugd->pt_package_type == PACKAGE_TYPE_SYS_STRING) {
		func_get_string = email_get_system_string;
	} else {
		func_get_string = email_get_email_string;
	}

	if (ugd->pt_string_format) {
		if (ugd->pt_data1) {
			snprintf(buf, sizeof(buf), ugd->pt_string_format, func_get_string(ugd->pt_string_id), ugd->pt_data1);
		} else {
			snprintf(buf, sizeof(buf), ugd->pt_string_format, func_get_string(ugd->pt_string_id), ugd->pt_data2);
		}
	} else {
		if (ugd->pt_data1) {
			snprintf(buf, sizeof(buf), func_get_string(ugd->pt_string_id), ugd->pt_data1);
		} else {
			snprintf(buf, sizeof(buf), func_get_string(ugd->pt_string_id), ugd->pt_data2);
		}
	}
	retm_if(!strlen(buf), "translated string is 0!");

	debug_secure("==> buf(%s)", buf);

	switch (ugd->pt_element_type) {
		case POPUP_ELEMENT_TYPE_TITLE:
			elm_object_part_text_set(ugd->composer_popup, "title,text", buf);
			break;
		case POPUP_ELEMENT_TYPE_CONTENT:
			elm_object_text_set(ugd->composer_popup, buf);
			break;
		default:
			break;

	}

	debug_leave();
}

void composer_util_popup_translate_set_variables(void *data, EmailPopupElementType element_type, EmailPopupStringType string_type, const char *string_format, const char *string_id, const char *data1, int data2)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");
	retm_if(!string_id, "Invalid parameter: string_id is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	/*
	 * free privous variables allocated.
	 */
	composer_util_popup_translate_free_variables(ugd);

	ugd->pt_element_type = element_type;
	ugd->pt_package_type = string_type;
	/*
	 * if string_format is NULL, use pt_string_id as format.
	 */
	ugd->pt_string_format = string_format ? strdup(string_format) : NULL;
	ugd->pt_string_id = strdup(string_id);

	/*
	 * If there's data1(string), use it. otherwise use data2(int)
	 */
	if (data1) {
		ugd->pt_data1 = strdup(data1);
	} else {
		ugd->pt_data2 = data2;
	}

	debug_leave();
}

void composer_util_popup_translate_free_variables(void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	FREE(ugd->pt_string_format);
	FREE(ugd->pt_string_id);
	FREE(ugd->pt_data1);

	debug_leave();
}
