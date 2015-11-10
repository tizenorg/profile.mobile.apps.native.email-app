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


#include <glib.h>
#include <gio/gio.h>
#include "email-setting.h"
#include "email-setting-noti-mgr.h"

void _event_receiver(GDBusConnection *connection, const gchar *sender_name, const gchar *object_path,
		const gchar *interface_name, const gchar *signal_name, GVariant *parameters, gpointer data);
static void _call_operation_callback(EmailSettingUGD *ugd, email_setting_operation_type_e op_type, email_setting_response_data *data);
static email_setting_operation_type_e _get_email_setting_storage_noti_type(int type);
static email_setting_operation_type_e _get_email_setting_network_noti_type(int type);

int setting_noti_init(EmailSettingUGD *ugd)
{
	debug_enter();

	GError *error = NULL;
	if (ugd->dbus_conn == NULL) {
		ugd->dbus_conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
		if (error) {
			debug_error("g_bus_get_sync() failed (%s)", error->message);
			g_error_free(error);
			return -1;
		}

		ugd->storage_id = g_dbus_connection_signal_subscribe(ugd->dbus_conn, NULL, "User.Email.StorageChange", "email", "/User/Email/StorageChange",
													NULL, G_DBUS_SIGNAL_FLAGS_NONE, _event_receiver, ugd, NULL);

		if (ugd->storage_id == GDBUS_SIGNAL_SUBSCRIBE_FAILURE) {
			debug_log("Failed to g_dbus_connection_signal_subscribe()");
			return -1;
		}
		ugd->network_id = g_dbus_connection_signal_subscribe(ugd->dbus_conn, NULL, "User.Email.NetworkStatus", "email", "/User/Email/NetworkStatus",
													NULL, G_DBUS_SIGNAL_FLAGS_NONE, _event_receiver, ugd, NULL);
		if (ugd->network_id == GDBUS_SIGNAL_SUBSCRIBE_FAILURE) {
			debug_critical("Failed to g_dbus_connection_signal_subscribe()");
			return -1;
		}
	}

	return 0;
}

int setting_noti_deinit(EmailSettingUGD *ugd)
{
	debug_enter();

	g_dbus_connection_signal_unsubscribe(ugd->dbus_conn, ugd->storage_id);
	g_dbus_connection_signal_unsubscribe(ugd->dbus_conn, ugd->network_id);
	g_object_unref(ugd->dbus_conn);
	ugd->storage_id = 0;
	ugd->network_id = 0;
	ugd->dbus_conn = NULL;

	GSList *l = NULL;
	email_setting_ipc_data *ipc_data = NULL;
	l = ugd->noti_list;
	while (l) {
		ipc_data = l->data;
		if (ipc_data)
			free(ipc_data);
		l = g_slist_next(l);
	}
	g_slist_free(ugd->noti_list);
	ugd->noti_list = NULL;

	return 0;
}

int setting_noti_subscribe(EmailSettingUGD *ugd, email_setting_operation_type_e op_type, email_setting_ipc_cb cb, void *user_data)
{
	debug_enter();
	email_setting_ipc_data *ipc_data = NULL;

	ipc_data = calloc(1, sizeof(email_setting_ipc_data));
	retvm_if(!ipc_data, -1, "memory allocation fail");

	ipc_data->op_type = op_type;
	ipc_data->cb = cb;
	ipc_data->user_data = user_data;

	debug_log("add operation: %d", op_type);
	ugd->noti_list = g_slist_append(ugd->noti_list, ipc_data);

	return 0;
}

int setting_noti_unsubscribe(EmailSettingUGD *ugd, email_setting_operation_type_e op_type, email_setting_ipc_cb cb)
{
	debug_enter();
	GSList *l = NULL;
	email_setting_ipc_data *ipc_data = NULL;

	l = ugd->noti_list;
	while (l) {
		ipc_data = l->data;
		if (ipc_data && ipc_data->op_type == op_type && ipc_data->cb == cb) {
			debug_log("remove operation: %d", op_type);
			ugd->noti_list = g_slist_remove(ugd->noti_list, ipc_data);
			l = ugd->noti_list;
		} else {
			l = g_slist_next(l);
		}
	}
	return 0;
}

void _event_receiver(GDBusConnection *connection,
											const gchar *sender_name,
											const gchar *object_path,
											const gchar *interface_name,
											const gchar *signal_name,
											GVariant *parameters,
											gpointer data)
{
	debug_enter();
	EmailSettingUGD *ugd = data;
	debug_secure("Object path=%s, interface name=%s, signal name=%s", object_path, interface_name, signal_name);

	retm_if(ugd->dbus_conn == NULL, "dbus_conn is NULL");

	if (!(g_strcmp0(object_path, "/User/Email/StorageChange")) && !(g_strcmp0(signal_name, "email"))) {
		debug_log("receive storage event");
		int subtype = 0;
		int data1 = 0;
		int data2 = 0;
		char *data3 = NULL;
		int data4 = 0;
		email_setting_response_data *data = NULL;

		g_variant_get(parameters, "(iiisi)", &subtype, &data1, &data2, &data3, &data4);
		debug_log("subtype: %d, data1: %d, data2: %d, data3: %s, data4: %d",
				subtype, data1, data2, data3, data4);

		data = calloc(1, sizeof(email_setting_response_data));
		retm_if(!data, "memory allocation fail");

		/* storage event info */
		data->account_id = data1;
		data->type = subtype;
		data->handle = -1;
		data->err = data4;
		data->data = data3;

		_call_operation_callback(ugd, _get_email_setting_storage_noti_type(subtype), data);
	} else if (!(g_strcmp0(object_path, "/User/Email/NetworkStatus")) && !(g_strcmp0(signal_name, "email"))) {
		debug_log("receive network event");
		int subtype = 0;
		int data1 = 0;
		char *data2 = NULL;
		int data3 = 0;
		int data4 = 0;
		email_setting_response_data *data = NULL;

		g_variant_get(parameters, "(iisii)", &subtype, &data1, &data2, &data3, &data4);
		debug_log("subtype: %d, data1: %d, data2: %s, data3: %d, data4: %d",
				subtype, data1, data2, data3, data4);

		data = calloc(1, sizeof(email_setting_response_data));
		retm_if(!data, "memory allocation fail");

		/* network event info */
		data->account_id = data1;
		data->type = subtype;
		data->handle = data3;
		data->err = data4;
		data->data = data2;

		_call_operation_callback(ugd, _get_email_setting_network_noti_type(subtype), data);
	} else {
		debug_warning("uninterested dbus message");
	}
}

static email_setting_operation_type_e _get_email_setting_storage_noti_type(int type)
{
	debug_enter();
	email_setting_operation_type_e ret_op_type = EMAIL_SETTING_NO_OP_NOTI;

	switch (type) {
		debug_log("receiving event: %d", type);
		case NOTI_ACCOUNT_ADD:
			ret_op_type = EMAIL_SETTING_ACCOUNT_ADD_NOTI;
			break;
		case NOTI_ACCOUNT_UPDATE:
			ret_op_type = EMAIL_SETTING_ACCOUNT_UPDATE_NOTI;
			break;
		case NOTI_ACCOUNT_DELETE:
			ret_op_type = EMAIL_SETTING_ACCOUNT_DELETE_NOTI;
			break;
		default:
			debug_warning("unknown type");
			ret_op_type = EMAIL_SETTING_INVALID_NOTI;
			break;
	}

	return ret_op_type;
}

static email_setting_operation_type_e _get_email_setting_network_noti_type(int type)
{
	debug_enter();
	email_setting_operation_type_e ret_op_type = EMAIL_SETTING_NO_OP_NOTI;

	debug_log("receiving event: %d", type);
	switch (type) {
	case NOTI_VALIDATE_ACCOUNT_FINISH:
	case NOTI_VALIDATE_ACCOUNT_FAIL:
	case NOTI_VALIDATE_ACCOUNT_CANCEL:
		ret_op_type = EMAIL_SETTING_ACCOUNT_VALIDATE_NOTI;
		break;
	case NOTI_VALIDATE_AND_UPDATE_ACCOUNT_FINISH:
	case NOTI_VALIDATE_AND_UPDATE_ACCOUNT_FAIL:
	case NOTI_VALIDATE_AND_UPDATE_ACCOUNT_CANCEL:
		ret_op_type = EMAIL_SETTING_ACCOUNT_VALIDATE_AND_UPDATE_NOTI;
		break;
	case NOTI_VALIDATE_AND_CREATE_ACCOUNT_FINISH:
	case NOTI_VALIDATE_AND_CREATE_ACCOUNT_FAIL:
	case NOTI_VALIDATE_AND_CREATE_ACCOUNT_CANCEL:
		ret_op_type = EMAIL_SETTING_ACCOUNT_VALIDATE_AND_CREATE_NOTI;
		break;
	case NOTI_DOWNLOAD_START:
		ret_op_type = EMAIL_SETTING_SYNC_START_MAIL_NOTI;
		break;
	case NOTI_DOWNLOAD_FINISH:
	case NOTI_DOWNLOAD_FAIL:
	case NOTI_DOWNLOAD_CANCEL:
		ret_op_type = EMAIL_SETTING_SYNC_MAIL_NOTI;
		break;
	case NOTI_SYNC_IMAP_MAILBOX_LIST_START:
		ret_op_type = EMAIL_SETTING_SYNC_START_FOLDER_NOTI;
		break;
	case NOTI_SYNC_IMAP_MAILBOX_LIST_FINISH:
	case NOTI_SYNC_IMAP_MAILBOX_LIST_FAIL:
	case NOTI_SYNC_IMAP_MAILBOX_LIST_CANCEL:
		ret_op_type = EMAIL_SETTING_SYNC_FOLDER_NOTI;
		break;
	case NOTI_QUERY_SMTP_MAIL_SIZE_LIMIT_FINISH:
	case NOTI_QUERY_SMTP_MAIL_SIZE_LIMIT_FAIL:
		ret_op_type = EMAIL_SETTING_SMTP_MAIL_SIZE_NOTI;
		break;
	default:
		debug_warning("unknown type");
		ret_op_type = EMAIL_SETTING_INVALID_NOTI;
		break;
	}

	return ret_op_type;
}

static void _call_operation_callback(EmailSettingUGD *ugd, email_setting_operation_type_e op_type, email_setting_response_data *data)
{
	debug_enter();
	GSList *l = NULL;
	email_setting_ipc_data *ipc_data = NULL;

	retm_if(!data, "response data is NULL");

	l = ugd->noti_list;
	while (l) {
		ipc_data = l->data;
		if (ipc_data && ipc_data->cb && ipc_data->op_type == op_type) {
			ipc_data->cb(data->account_id, data, ipc_data->user_data);
		}

		l = g_slist_next(l);
	}
}
