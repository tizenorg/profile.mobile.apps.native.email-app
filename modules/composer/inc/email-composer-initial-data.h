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

#ifndef __EMAIL_COMPOSER_INITIAL_DATA_H__
#define __EMAIL_COMPOSER_INITIAL_DATA_H__

/**
 * @brief Creates body mail info
 * @param[in]	view»			Email composer data
 * @return The body main info string on success, otherwise NULL. It should be freed.
 */
char *composer_initial_data_body_make_parent_mail_info(EmailComposerView *view);

/**
 * @brief Creates signature
 * @param[in]	view»			Email composer data
 * @return The body signature string on success, otherwise NULL. It should be freed.
 */
char *composer_initial_data_body_make_signature_markup(EmailComposerView *view);

/**
 * @brief Delete progress popup
 * @param[in]	data»			User data (Email composer data)
 */
void composer_initial_data_destroy_download_contents_popup(void *data);

/**
 * @brief Data set to ewk view
 * @param[in]	view»						Email composer data
 * @param[in]	is_draft_sync_requested»	Is draft sync reqested, bool value
 */
void composer_initial_data_set_mail_info(EmailComposerView *view, bool is_draft_sync_requested);

/**
 * @brief Parse composer run type
 * @param[in]	view»			Email composer data
 * @param[in]	params			Email params handle
 * @return Composer error type
 */
COMPOSER_ERROR_TYPE_E composer_initial_data_parse_composer_run_type(EmailComposerView *view, email_params_h params);

/**
 * @brief Pre parse arguments
 * @param[in]	view»			Email composer data
 * @param[in]	params			Email params handle
 * @return Composer error type
 */
COMPOSER_ERROR_TYPE_E composer_initial_data_pre_parse_arguments(EmailComposerView *view, email_params_h params);

/**
 * @brief Post parse arguments
 * @param[in]	view»			Email composer data
 * @param[in]	params			Email params handle
 * @return Composer error type
 */
COMPOSER_ERROR_TYPE_E composer_initial_data_post_parse_arguments(EmailComposerView *view, email_params_h params);

/**
 * @brief Composer make initial content
 * @param[in]	view»			Email composer data
 */
void composer_initial_data_make_initial_contents(EmailComposerView *view);

/**
 * @brief Composer free initial content
 * @param[in]	view»			Email composer data
 */
void composer_initial_data_free_initial_contents(EmailComposerView *view);

#endif /* __EMAIL_COMPOSER_INITIAL_DATA_H__ */
