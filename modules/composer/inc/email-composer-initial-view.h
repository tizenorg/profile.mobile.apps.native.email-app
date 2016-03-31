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

#ifndef __EMAIL_COMPOSER_INITIAL_VIEW_H__
#define __EMAIL_COMPOSER_INITIAL_VIEW_H__

/**
 * @brief Draw base frame
 * @param[in]	view»			Email composer data
 */
void composer_initial_view_draw_base_frame(EmailComposerView *view);

/**
 * @brief Draw header components
 * @param[in]	data»		Used data (Email composer data)
 */
void composer_initial_view_draw_header_components(void *data);

/**
 * @brief Draw remaining mails components
 * @param[in]	data»		Used data (Email composer data)
 */
void composer_initial_view_draw_remaining_components(void *data);

/**
 * @brief Draw richtext component
 * @param[in]	data»		Used data (Email composer data)
 */
void composer_initial_view_draw_richtext_components(void *data);

/**
 * @brief Creates combined scroller
 * @param[in]	data»		Used data (Email composer data)
 */
void composer_initial_view_create_combined_scroller(void *data);

/**
 * @brief Set combined scroller rotation mode
 * @param[in]	data»		Used data (Email composer data)
 */
void composer_initial_view_set_combined_scroller_rotation_mode(void *data);

/**
 * @brief Back button callback, provides operation when clicked
 * @param[in]	view»			Email composer data
 */
void composer_initial_view_back_cb(EmailComposerView *view);

/**
 * @brief Brings combined scroller with animation to appropriate position
 * @param[in]	view»			Email composer data
 * @param[in]	pos»			Position
 */
void composer_initial_view_cs_bring_in(EmailComposerView *view, int pos);

/**
 * @brief Immediately brings combined scroller to appropriate position
 * @param[in]	view»			Email composer data
 * @param[in]	pos»			Position
 */
void composer_initial_view_cs_show(EmailComposerView *view, int pos);

/**
 * @brief Freeze combined scroller
 * @param[in]	view»			Email composer data
 */
void composer_initial_view_cs_freeze_push(EmailComposerView *view);

/**
 * @brief Decrease freeze count
 * @param[in]	view»			Email composer data
 */
void composer_initial_view_cs_freeze_pop(EmailComposerView *view);

/**
 * @brief Activate selection mode
 * @param[in]	view»			Email composer data
 */
void composer_initial_view_activate_selection_mode(EmailComposerView *view);

/**
 * @brief Callback when coret position chenged
 * @param[in]	view»			Email composer data
 * @param[in]	top»			Top position value
 * @param[in]	bottom»			Bottom position value
 * @param[in]	isCollapsed»	Is colapsed (hiden)
 */
void composer_initial_view_caret_pos_changed_cb(EmailComposerView *view, int top, int bottom, bool isCollapsed);

#endif /* __EMAIL_COMPOSER_INITIAL_VIEW_H__ */
