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

#ifndef _email_filter_H_
#define _email_filter_H_

#undef LOG_TAG
#define LOG_TAG "EMAIL_FILTER"

#include <Elementary.h>
#include <glib.h>

#include "email-locale.h"
#include "email-common-types.h"
#include "email-engine.h"
#include "email-utils.h"
#include "email-utils-contacts.h"
#include "email-debug.h"
#include "email-module-dev.h"

#define _EDJ(o) elm_layout_edje_get(o)

/**
 * @brief Email Filter supported operations enumeration
 *
 */
typedef enum _FilterOperationType {
	EMAIL_FILTER_OPERATION_FILTER = 1001,
	EMAIL_FILTER_OPERATION_BLOCK,
	EMAIL_FILTER_OPERATION_PRIORITY_SERNDER
} FilterOperationType;

/**
 * @brief Email Filter View supported types enumeration
 *
 */
typedef enum _EmailFilterViewType {
	EMAIL_FILTER_VIEW_FILTER_LIST = 1001,
	EMAIL_FILTER_VIEW_ADD_FILTER,
	EMAIL_FILTER_VIEW_DELETE_FILTER,
	EMAIL_FILTER_VIEW_EDIT_FILTER,
	EMAIL_FILTER_VIEW_INVALID
} EmailFilterViewType;

/**
 * @brief Email Filter view data
 *
 */
typedef struct ug_data
{
	email_module_t base;

	/*Filter operation type*/
	FilterOperationType op_type;

	const char *param_filter_addr;

	/* base layout variable */
	Evas_Object *more_btn;

	/* keypad management variables*/
	Eina_Bool is_keypad;
	Eina_Bool is_conformant;

} EmailFilterUGD;

/**
 * @brief elm util function
 *
 * @return path to the edc file where theme is described
 *
 */
EMAIL_API const char *email_get_filter_theme_path();

/**
 * @brief Provide functionality to notify email service about filter rule changes
 * @param[in]	ugd		email filter module data
 *
 */
void email_filter_publish_changed_noti(EmailFilterUGD *ugd);

/**
 * @brief util function which set callbacks on conformant object about keyboard and clipboard events
 * @param[in]	ugd		email filter module data
 *
 */
void email_filter_add_conformant_callback(EmailFilterUGD *ugd);

/**
 * @brief util function which unset callbacks on conformant object about keyboard and clipboard events
 * @param[in]	ugd		email filter module data
 *
 */
void email_filter_del_conformant_callback(EmailFilterUGD *ugd);

/**
 * @brief function which set limit on amount for inputted characters into entry
 * @param[in]	vd					email filter view data
 * @param[in]	entry				Entry object
 * @param[in]	max_char_count		limits by character count (disabled if 0)
 * @param[in]	max_byte_count		limits by bytes count (disabled if 0)
 *
 * @return	pointer to Elm_Entry_Filter_Limit_Size structure which was setted as entry filter limit
 *
 */
Elm_Entry_Filter_Limit_Size *email_filter_set_input_entry_limit(email_view_t *vd, Evas_Object *entry, int max_char_count, int max_byte_count);

#endif				/* _EDF_email_filter_H__ */
