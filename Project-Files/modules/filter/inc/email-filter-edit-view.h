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

#include "email-filter.h"

#ifndef _email_filter_edit_H_
#define _email_filter_edit_H_

enum _filter_edit_view_mode {
	FILTER_EDIT_VIEW_MODE_ADD_NEW,
	FILTER_EDIT_VIEW_MODE_EDIT_EXISTENT
};
typedef enum _filter_edit_view_mode filter_edit_view_mode;

/**
 * @brief Create filter edit view
 *
 * @param[in]	module		Email Filter data
 * @param[in]	mode		Edit view mode
 * @param[in]	filter_id	Id of filter rule which should be modified if mode is edit
 *
 */
void create_filter_edit_view(EmailFilterModule *module, filter_edit_view_mode mode, int filter_id);

#endif
