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

#ifndef __EMAIL_SETTING_NOTI_MGR_H__
#define __EMAIL_SETTING_NOTI_MGR_H__

typedef enum _email_setting_operation_type_e {
	EMAIL_SETTING_NO_OP_NOTI = 1000,
	EMAIL_SETTING_ACCOUNT_VALIDATE_NOTI,
	EMAIL_SETTING_ACCOUNT_VALIDATE_AND_UPDATE_NOTI,
	EMAIL_SETTING_ACCOUNT_VALIDATE_AND_CREATE_NOTI,
	EMAIL_SETTING_ACCOUNT_ADD_NOTI,
	EMAIL_SETTING_ACCOUNT_UPDATE_NOTI,
	EMAIL_SETTING_ACCOUNT_DELETE_NOTI,
	EMAIL_SETTING_SYNC_START_MAIL_NOTI,
	EMAIL_SETTING_SYNC_MAIL_NOTI,
	EMAIL_SETTING_SYNC_START_FOLDER_NOTI,
	EMAIL_SETTING_SYNC_FOLDER_NOTI,
	EMAIL_SETTING_SMTP_MAIL_SIZE_NOTI,
	EMAIL_SETTING_INVALID_NOTI
} email_setting_operation_type_e;

/**
 * @brief Email setting response data
 */
typedef struct _email_setting_response_data {
	int account_id;		/**< account id */
	int handle;			/**< handle */
	int type;			/**< type */
	int err;			/**< error code */
	void *data;			/**< response data */
} email_setting_response_data;

/**
 * @brief Prototype of email setting IPC callback function
 */
typedef void (*email_setting_ipc_cb)(int account_id, email_setting_response_data *response, void *user_data);

/**
 * @brief Email setting IPC data
 */
typedef struct _email_setting_ipc_data {
	email_setting_operation_type_e op_type;		/**< operation type */
	email_setting_ipc_cb cb;					/**< IPC callback function */
	void *user_data;							/**< user data */
} email_setting_ipc_data;

/**
 * @brief Initialize settings module notification system
 * @param[in]	module	Email settings data
 * @@return 0 on success, otherwise a negative error value
 */
int setting_noti_init(EmailSettingModule *module);

/**
 * @brief Destroys the dbus connection and releases all its resources.
 * @param[in]	module	Email settings data
 * @return 0 on success, otherwise a negative error value
 */
int setting_noti_deinit(EmailSettingModule *module);

/**
 * @brief Subscribe to dbus signals
 * @param[in]	module		Email settings data
 * @param[in]	op_type		Email settings operation type enum
 * @param[in]	cb			IPC callback function
 * @param[in]	user_data	user data pointer
 * @return 0 on success, otherwise a negative error value
 */
int setting_noti_subscribe(EmailSettingModule *module, email_setting_operation_type_e op_type, email_setting_ipc_cb cb, void *user_data);
/**
 * @brief Unsubscribe from dbus signals
 * @param[in]	module		Email settings data
 * @param[in]	op_type		Email settings operation type enum
 * @param[in]	cb			IPC callback function
 * @return 0 on success, otherwise a negative error value
 */
int setting_noti_unsubscribe(EmailSettingModule *module, email_setting_operation_type_e op_type, email_setting_ipc_cb cb);

#endif				/* __EMAIL_SETTING_NOTI_MGR_H__ */
