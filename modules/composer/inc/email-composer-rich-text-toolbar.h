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

#ifndef __EMAIL_COMMON_EXT_RICH_TEXT_TOOLBAR_H__
#define __EMAIL_COMMON_EXT_RICH_TEXT_TOOLBAR_H__

#include "email-composer.h"

/**
 * @brief Font style params data
 */
typedef struct {
	int font_size;
	Eina_Bool is_bold;
	Eina_Bool is_italic;
	Eina_Bool is_underline;
	Eina_Bool is_ordered_list;
	Eina_Bool is_unordered_list;
	email_rgba_t font_color;
	email_rgba_t bg_color;
} FontStyleParams;

/**
 * @brief Creates rich text toolbar for composer
 *
 * @param[in]	view		Email composer data
 *
 * @return Evas_Object instance with suitable richtext, otherwise NULL
 */
EMAIL_API Evas_Object *composer_rich_text_create_toolbar(EmailComposerView *view);

/**
 * @brief Set parameters for rich text toolbar in composer
 *
 * @param[in]	view			Email composer data
 * @param[in]	params		Font style params data
 *
 */
EMAIL_API void composer_rich_text_font_style_params_set(EmailComposerView *view, FontStyleParams *params);

/**
 * @brief Disable rich text toolbar in composer
 *
 * @param[in]	view				Email composer data
 * @param[in]	is_disable		Flag if set disable richtext toolbar
 *
 */
EMAIL_API void composer_rich_text_disable_set(EmailComposerView *view, Eina_Bool is_disable);

/**
 * @brief Get disable flag from rich text toolbar in composer
 *
 * @param[in]	view		Email composer data
 *
 * @return EINA_TRUE when disabled, otherwise EINA_FALSE or an error occurred
 */
EMAIL_API Eina_Bool composer_rich_text_disable_get(EmailComposerView *view);

#endif /* __EMAIL_COMMON_EXT_RICH_TEXT_TOOLBAR_H__ */
