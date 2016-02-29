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

#ifndef __EMAIL_VIEWER_MORE_MENU_H__
#define __EMAIL_VIEWER_MORE_MENU_H__

/**
 * @brief Add contact by setting contact address and name
 *
 * @param[in]	data			User data (Email viewer data)
 * @param[in]	contact_address			Contact address string
 * @param[in]	contact_name			Contact name string
 *
 */
void viewer_add_contact(void *data, char *contact_address, char *contact_name);

/**
 * @brief Update contact by contact data
 *
 * @param[in]	data			User data (Email viewer data)
 * @param[in]	contact_data	Contact data string
 *
 */
void viewer_update_contact(void *data, char *contact_data);

/**
 * @brief Create more ctxpopup
 *
 * @param[in]	view			Email viewer data
 *
 */
void viewer_create_more_ctxpopup(EmailViewerView *view);

/**
 * @brief Upadte geometry position after rotation for ctxpopup
 *
 * @param[in]	ctxpopup		More button ctxpopup
 * @param[in]	win				Parent object to which ctxpopup was attached
 *
 */
void viewer_more_menu_move_ctxpopup(Evas_Object *ctxpopup, Evas_Object *win);

#endif /* __EMAIL_VIEWER_MORE_MENU_H__ */
