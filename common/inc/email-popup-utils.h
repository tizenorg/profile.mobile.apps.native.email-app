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

#ifndef __EMAIL_POPUP_UTILS_H__
#define __EMAIL_POPUP_UTILS_H__

#include "email-common-types.h"

#define COMMON_POPUP_ITEM_1_LINE_HEIGHT 96
#define COMMON_POPUP_ITEM_2_LINE_HEIGHT 96
#define COMMON_POPUP_ITEM_1_LINE_COUNT_LANDSCAPE 4
#define COMMON_POPUP_ITEM_1_LINE_COUNT_PORTRAIT 7
#define COMMON_POPUP_ITEM_2_LINE_COUNT_LANDSCAPE 3
#define COMMON_POPUP_ITEM_2_LINE_COUNT_PORTRAIT 6
#define COMMON_EVAS_DATA_NAME_POPUP_ITEM_COUNT "popup_item_count"
#define COMMON_EVAS_DATA_NAME_POPUP_ITEM_HEIGHT "popup_item_height"

EMAIL_API Evas_Object *common_util_create_popup(Evas_Object *parent, email_string_t title,
		Evas_Smart_Cb btn1_click_cb, email_string_t btn1_text,
		Evas_Smart_Cb btn2_click_cb, email_string_t btn2_text,
		Evas_Smart_Cb btn3_click_cb, email_string_t btn3_text,
		Evas_Smart_Cb cancel_cb, Eina_Bool need_ime_hide, void *cb_data);

EMAIL_API void common_util_popup_display_genlist(Evas_Object *popup, Evas_Object *genlist, Eina_Bool is_horizontal, int item_height, int item_count);

#endif/* __EMAIL_POPUP_UTILS_H__ */
