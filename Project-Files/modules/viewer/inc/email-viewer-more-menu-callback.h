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

#ifndef __EMAIL_VIEWER_MORE_MENU_CALLBACK_H__
#define __EMAIL_VIEWER_MORE_MENU_CALLBACK_H__

/**
 * @brief Callback to add contact from ctxpopup
 *
 * @param[in]	data		User data (Email viewer data)
 * @param[in]	obj			Unused
 * @param[in]	event_info	Unused
 *
 */
void viewer_ctxpopup_add_contact_cb(void *data, Evas_Object *obj, void *event_info);

/**
 * @brief Callback to view contact details
 *
 * @param[in]	data		User data (Email viewer data)
 * @param[in]	obj			Unused
 * @param[in]	event_info	Unused
 *
 */
void viewer_ctxpopup_detail_contact_cb(void *data, Evas_Object *obj, void *event_info);

/**
 * @brief Callback to add priority sender view
 *
 * @param[in]	data		User data (Email viewer data)
 * @param[in]	obj			Unused
 * @param[in]	event_info	Unused
 *
 */
void viewer_ctxpopup_add_vip_rule_cb(void *data, Evas_Object *obj, void *event_info);

/**
 * @brief Callback to remove priority sender
 *
 * @param[in]	data		User data (Email viewer data)
 * @param[in]	obj			Unused
 * @param[in]	event_info	Unused
 *
 */
void viewer_ctxpopup_remove_vip_rule_cb(void *data, Evas_Object *obj, void *event_info);

/**
 * @brief Callback to add mail to spam box
 *
 * @param[in]	data		User data (Email viewer data)
 * @param[in]	obj			Unused
 * @param[in]	event_info	Unused
 *
 */
void viewer_ctxpopup_add_to_spambox_cb(void *data, Evas_Object *obj, void *event_info);

/**
 * @brief Callback to remove mail from spam box
 *
 * @param[in]	data		User data (Email viewer data)
 * @param[in]	obj			Unused
 * @param[in]	event_info	Unused
 *
 */
void viewer_ctxpopup_remove_from_spambox_cb(void *data, Evas_Object *obj, void *event_info);

/**
 * @brief Callback, called when ctxpopup have to be dismissed
 *
 * @param[in]	data		User data (Email viewer data)
 * @param[in]	obj			Unused
 * @param[in]	event_info	Unused
 *
 */
void viewer_more_ctxpopup_dismissed_cb(void *data, Evas_Object *obj, void *event_info);

/**
 * @brief Callback to resize content of viewer naviframe
 *
 * @param[in]	data		User data (Email viewer data)
 * @param[in]	obj			Unused
 * @param[in]	event_info	Unused
 *
 */
void viewer_more_menu_window_resized_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

/**
 * @brief Callback, called when ctxpopup have to be deleted
 *
 * @param[in]	data		User data (Email viewer data)
 * @param[in]	obj			Unused
 * @param[in]	event_info	Unused
 *
 */
void viewer_more_menu_ctxpopup_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

/**
 * @brief Callback to select message as unread
 *
 * @param[in]	data		User data (Email viewer data)
 * @param[in]	obj			Unused
 * @param[in]	event_info	Unused
 *
 */
void viewer_mark_as_unread_cb(void *data, Evas_Object *obj, void *event_info);

/**
 * @brief Callback to enter to edit the mail
 *
 * @param[in]	data		User data (Email viewer data)
 * @param[in]	obj			Unused
 * @param[in]	event_info	Unused
 *
 */
void viewer_edit_email_cb(void *data, Evas_Object *obj, void *event_info);

/**
 * @brief Callback to enter to edit the mail with the cancellation of sending mail
 *
 * @param[in]	data		User data (Email viewer data)
 * @param[in]	obj			Unused
 * @param[in]	event_info	Unused
 *
 */
void viewer_edit_scheduled_email_cb(void *data, Evas_Object *obj, void *event_info);

/**
 * @brief Callback to cancel send mail
 *
 * @param[in]	data		User data (Email viewer data)
 * @param[in]	obj			Unused
 * @param[in]	event_info	Unused
 *
 */
void viewer_cancel_send_cb(void *data, Evas_Object *obj, void *event_info);

/**
 * @brief Callback to send email again
 *
 * @param[in]	data		User data (Email viewer data)
 * @param[in]	obj			Unused
 * @param[in]	event_info	Unused
 *
 */
void viewer_resend_cb(void *data, Evas_Object *obj, void *event_info);

#endif /* __EMAIL_VIEWER_MORE_MENU_CALLBACK_H__ */
