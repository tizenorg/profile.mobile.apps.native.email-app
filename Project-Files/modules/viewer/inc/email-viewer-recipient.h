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

#ifndef __EMAIL_VIEWER_RECIPIENT_H__
#define __EMAIL_VIEWER_RECIPIENT_H__

/**
 * @brief Create layout to accessory recipient
 *
 * @param[in]	data				User data (Email viewer data)
 * @param[in]	addrs_info_list		List with addresses info
 * @param[in]	recipient			Accessory for recipient
 *
 * @return @return Evas_Object with suitable layout, otherwise NULL
 */
Evas_Object *recipient_create_address(void *data, email_address_info_list_t *addrs_info_list, VIEWER_TO_CC_BCC_LY recipient);

/**
 * @brief Check email address if is one from priority
 *
 * @param[in]	email_address		Email address name
 *
 * @return TRUE when email address from priority list, otherwise FALSE or an error occurred
 */
bool recipient_is_priority_email_address(char *email_address);

/**
 * @brief Check email address if is one from blocked list
 *
 * @param[in]	email_address		Email address name
 *
 * @return TRUE when email address from blocked list, otherwise FALSE or an error occurred
 */
bool recipient_is_blocked_email_address(char *email_address);

/**
 * @brief Create popup list with selection and adding contact belong to sender/recipient
 *
 * @param[in]	data				User data (Email viewer data)
 * @param[in]	contact_data		Contact data to be displayed
 * @param[in]	name_type			Type of data belongs sender/recipient
 *
 */
void recipient_add_to_contact_selection_popup(void *data, char *contact_data, VIEWER_DISPLAY_NAME name_type);

/**
 * @brief Clear multibutton entry from any data
 *
 * @param[in]	obj		Multibutton entry
 *
 */
void recipient_clear_multibuttonentry_data(Evas_Object *obj);

/**
 * @brief Delete registered callbacks from recipient fields layouts
 *
 * @param[in]	data		User data (Email viewer data)
 *
 */
void recipient_delete_callbacks(void *data);

#endif /* __EMAIL_VIEWER_RECIPIENT_H__ */
