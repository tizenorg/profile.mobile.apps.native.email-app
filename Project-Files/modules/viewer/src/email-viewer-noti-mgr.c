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

#include <notification.h>

#include "email-engine.h"
#include "email-viewer.h"
#include "email-viewer-noti-mgr.h"
#include "email-viewer-util.h"
#include "email-viewer-attachment.h"
#include "email-viewer-header.h"
#include "email-viewer-contents.h"

static void _noti_mgr_popup_response_cb(void *data, Evas_Object *obj, void *event_info);
static void _noti_mgr_set_value_down_progress(void *data, double val);
static char *_noti_mgr_get_service_fail_type(int type, EmailViewerUGD *ug_data);
static void _noti_mgr_parse_mail_move_finish_data_params(char *buf, int *src_mailbox_id, int *dst_mailbox_id, int *mail_id);
static void _noti_mgr_parse_mail_field_update_data_params(char *inbuf, char **field_name, GList **mail_list);
static void _noti_mgr_on_gdbus_event_receive(GDBusConnection *connection,
		const gchar *sender_name,
		const gchar *object_path,
		const gchar *interface_name,
		const gchar *signal_name,
		GVariant *parameters,
		gpointer data);

static void _noti_mgr_popup_response_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;
	DELETE_EVAS_OBJECT(ug_data->notify);

	FREE(ug_data->translation_string_id1);
	FREE(ug_data->translation_string_id2);
	FREE(ug_data->str_format1);
	FREE(ug_data->str_format2);
	FREE(ug_data->extra_variable_string);

	debug_leave();
}

static void _noti_mgr_set_value_down_progress(void *data, double val)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;
	Evas_Object *pb = ug_data->pb_notify_lb;
	if (val < 0.0) {
		debug_log("val(%f) is MINUS", val);
		return;
	}
	retm_if(pb == NULL, "progressbar is NULL");
	char *style = NULL;
	style = (char *)elm_object_style_get(pb);

	if (STR_INVALID(style))
		return;

	if (g_strcmp0(style, "pending") == 0) {
		debug_log("changing theme to list progress");
		elm_object_style_set(pb, "default");
	}
	elm_progressbar_value_set(pb, val);

	char percent[MAX_STR_LEN] = { 0, };
	g_snprintf(percent, sizeof(percent), "%d%%", (int)(val * 100));

	char size_string[MAX_STR_LEN] = { 0, };
	char *file_size = email_get_file_size_string((guint64)ug_data->file_size);
	char *downloaded_size = email_get_file_size_string((guint64)ug_data->file_size * val);
	g_snprintf(size_string, sizeof(size_string), "%s/%s", downloaded_size, file_size);

	if ((int)elm_progressbar_value_get(pb) == 100) {
		elm_object_part_text_set(pb, "elm.text.bottom.left", file_size);
		elm_object_part_text_set(pb, "elm.text.bottom.right", "100%");
	} else {
		elm_object_part_text_set(pb, "elm.text.bottom.left", size_string);
		elm_object_part_text_set(pb, "elm.text.bottom.right", percent);
	}

	FREE(file_size);
	FREE(downloaded_size);

	debug_leave();
}

static char *_noti_mgr_get_service_fail_type(int type, EmailViewerUGD *ug_data)
{
	debug_enter();

	char *ret = NULL;
	debug_log("service error type: %d", type);

	debug_leave();
	ug_data->package_type2 = PACKAGE_TYPE_PACKAGE;
	if (type == EMAIL_ERROR_CANCELLED) {
		ug_data->translation_string_id2 = strdup(N_("IDS_EMAIL_POP_DOWNLOAD_CANCELLED"));
		ret = email_get_email_string(N_("IDS_EMAIL_POP_DOWNLOAD_CANCELLED"));
		return g_strdup(ret);
	} else if (type == EMAIL_ERROR_MAIL_NOT_FOUND_ON_SERVER) {
		ug_data->translation_string_id2 = strdup("IDS_EMAIL_POP_THIS_EMAIL_HAS_BEEN_DELETED_FROM_THE_SERVER");
		ret = email_get_email_string("IDS_EMAIL_POP_THIS_EMAIL_HAS_BEEN_DELETED_FROM_THE_SERVER");
		return g_strdup(ret);
	} else if (type == EMAIL_ERROR_NO_SUCH_HOST) {
		ug_data->translation_string_id2 = strdup(N_("IDS_EMAIL_POP_HOST_NOT_FOUND"));
		ret = email_get_email_string(N_("IDS_EMAIL_POP_HOST_NOT_FOUND"));
		return g_strdup(ret);
	} else if (type == EMAIL_ERROR_INVALID_SERVER) {
		ug_data->translation_string_id2 = strdup(N_("IDS_EMAIL_POP_SERVER_NOT_AVAILABLE"));
		ret = email_get_email_string(N_("IDS_EMAIL_POP_SERVER_NOT_AVAILABLE"));
		return g_strdup(ret);
	} else if (type == EMAIL_ERROR_MAIL_MEMORY_FULL) {
		ug_data->translation_string_id2 = strdup("IDS_EMAIL_POP_THERE_IS_NOT_ENOUGH_SPACE_IN_YOUR_DEVICE_STORAGE_GO_TO_SETTINGS_POWER_AND_STORAGE_STORAGE_THEN_DELETE_SOME_FILES_AND_TRY_AGAIN");
		ret = _("IDS_EMAIL_POP_THERE_IS_NOT_ENOUGH_SPACE_IN_YOUR_DEVICE_STORAGE_GO_TO_SETTINGS_POWER_AND_STORAGE_STORAGE_THEN_DELETE_SOME_FILES_AND_TRY_AGAIN");
		return g_strdup(ret);
	} else if (type == EMAIL_ERROR_FAILED_BY_SECURITY_POLICY) {
		ug_data->translation_string_id2 = strdup("IDS_EMAIL_POP_THE_CURRENT_EXCHANGE_SERVER_POLICY_PREVENTS_ATTACHMENTS_FROM_BEING_DOWNLOADED_TO_MOBILE_DEVICES");
		ret = email_get_email_string("IDS_EMAIL_POP_THE_CURRENT_EXCHANGE_SERVER_POLICY_PREVENTS_ATTACHMENTS_FROM_BEING_DOWNLOADED_TO_MOBILE_DEVICES");
		return g_strdup(ret);
	} else if (type == EMAIL_ERROR_ATTACHMENT_SIZE_EXCEED_POLICY_LIMIT) {
		ug_data->translation_string_id2 = strdup("IDS_EMAIL_POP_THE_MAXIMUM_ATTACHMENT_SIZE_ALLOWED_BY_THE_CURRENT_EXCHANGE_SERVER_POLICY_HAS_BEEN_EXCEEDED");
		ret = email_get_email_string("IDS_EMAIL_POP_THE_MAXIMUM_ATTACHMENT_SIZE_ALLOWED_BY_THE_CURRENT_EXCHANGE_SERVER_POLICY_HAS_BEEN_EXCEEDED");
		return g_strdup(ret);
	} else if (type == EMAIL_ERROR_NO_RESPONSE) {
		ug_data->translation_string_id2 = strdup(N_("IDS_EMAIL_POP_NO_RESPONSE_HAS_BEEN_RECEIVED_FROM_THE_SERVER_TRY_AGAIN_LATER"));
		ret = email_get_email_string(N_("IDS_EMAIL_POP_NO_RESPONSE_HAS_BEEN_RECEIVED_FROM_THE_SERVER_TRY_AGAIN_LATER"));
		return g_strdup(ret);
	} else if (type == EMAIL_ERROR_CERTIFICATE_FAILURE) {
		ug_data->translation_string_id2 = strdup(N_("IDS_EMAIL_POP_INVALID_OR_INACCESSIBLE_CERTIFICATE"));
		ret = email_get_email_string(N_("IDS_EMAIL_POP_INVALID_OR_INACCESSIBLE_CERTIFICATE"));
		return g_strdup(ret);
	} else if (type == EMAIL_ERROR_TLS_NOT_SUPPORTED) {
		ug_data->translation_string_id2 = strdup(N_("IDS_EMAIL_BODY_SERVER_DOES_NOT_SUPPORT_TLS"));
		ret = email_get_email_string(N_("IDS_EMAIL_BODY_SERVER_DOES_NOT_SUPPORT_TLS"));
		return g_strdup(ret);
	} else if (type == EMAIL_ERROR_TLS_SSL_FAILURE) {
		ug_data->translation_string_id2 = strdup(N_("IDS_EMAIL_BODY_UNABLE_TO_OPEN_CONNECTION_TO_SERVER_SECURITY_ERROR_OCCURRED"));
		ret = email_get_email_string(N_("IDS_EMAIL_BODY_UNABLE_TO_OPEN_CONNECTION_TO_SERVER_SECURITY_ERROR_OCCURRED"));
		return g_strdup(ret);
	} else {
		ug_data->translation_string_id2 = strdup("IDS_EMAIL_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED");
		ret = _("IDS_EMAIL_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED");
		return g_strdup(ret);
	}
}

static void _noti_mgr_parse_mail_move_finish_data_params(char *buf, int *src_mailbox_id, int *dst_mailbox_id, int *mail_id)
{
	debug_enter();

	retm_if(!STR_VALID(buf), "Invalid parameter: buf[NULL]");
	debug_log("inbuf = %s", buf);

	/* notification format: <src_folder><0x01><dst_folder><0x01><<mail_id><,><mail_id>>*/
	gchar **token;
	char delimiter[2] = { 0x01, 0x00 };
	token = g_strsplit_set(buf, delimiter, -1);
	if (token == NULL) {
		debug_log("tokken == NULL");
		return;
	}
	if (token[0] && strlen(token[0]) > 0) {
		*src_mailbox_id = atoi(token[0]);
	}
	if (token[1] && strlen(token[1]) > 0) {
		*dst_mailbox_id = atoi(token[1]);
	}
	if (token[2] && strlen(token[2]) > 0) {
		*mail_id = atoi(token[2]);
	}
	g_strfreev(token);
}

static void _noti_mgr_parse_mail_field_update_data_params(char *inbuf, char **field_name, GList **mail_list)
{
	debug_enter();

	retm_if(!STR_VALID(inbuf), "Invalid parameter: inbuf[NULL]");
	debug_log("inbuf = %s", inbuf);

	/* notification format: <field_name><0x01><<mail_id><,><mail_id>> */
	gchar **outer_tok;
	char delimiter[2] = { 0x01, 0x00 };
	outer_tok = g_strsplit_set(inbuf, delimiter, -1);
	if (outer_tok == NULL) {
		debug_log("outer_tok == NULL");
		return;
	}
	if (outer_tok[0] && strlen(outer_tok[0]) > 0)
		*field_name = strdup(outer_tok[0]);
	if (outer_tok[1] && strlen(outer_tok[1]) > 0) {
		gchar **inner_tok;
		int i = 0;
		inner_tok = g_strsplit_set(outer_tok[1], ",", -1);
		if (g_strv_length(inner_tok) == 1) { /* only one mail_id exists without "," */
			debug_log("outer_tok[1] : %s", outer_tok[1]);
			int *mail_id = (int *)calloc(1, sizeof(int));
			IF_NULL_FREE_2GSTRA_AND_RETURN(mail_id, inner_tok, outer_tok);
			*mail_id = atoi(outer_tok[1]);
			*mail_list = g_list_append(*mail_list, mail_id);
		} else {
			for (i = 0; i < g_strv_length(inner_tok); ++i) {
				debug_log("inner_tok[i] : %s", inner_tok[i]);
				if (!strcmp(inner_tok[i], "\"")) {	/* skip the empty token */
					continue;
				} else {
					int *mail_id = (int *)calloc(1, sizeof(int));
					IF_NULL_FREE_2GSTRA_AND_RETURN(mail_id, inner_tok, outer_tok);
					*mail_id = atoi(inner_tok[i]);
					*mail_list = g_list_append(*mail_list, mail_id);
				}
			}
		}
		g_strfreev(inner_tok);
	}
	g_strfreev(outer_tok);
}

Eina_Bool noti_mgr_dbus_receiver_setup(void *data)
{
	debug_enter();

	retvm_if(!data, EINA_FALSE, "data is NULL");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	GError *error = NULL;
	if (ug_data->viewer_dbus_conn == NULL) {
		ug_data->viewer_dbus_conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
		if (error) {
			debug_error("g_bus_get_sync() failed (%s)", error->message);
			g_error_free(error);
			return EINA_FALSE;
		}

		ug_data->viewer_network_id = g_dbus_connection_signal_subscribe(ug_data->viewer_dbus_conn, NULL, "User.Email.StorageChange", "email", "/User/Email/StorageChange",
													NULL, G_DBUS_SIGNAL_FLAGS_NONE, _noti_mgr_on_gdbus_event_receive, (void *)ug_data, NULL);
		retvm_if(ug_data->viewer_network_id == GDBUS_SIGNAL_SUBSCRIBE_FAILURE, EINA_FALSE, "Failed to g_dbus_connection_signal_subscribe()");

		ug_data->viewer_storage_id = g_dbus_connection_signal_subscribe(ug_data->viewer_dbus_conn, NULL, "User.Email.NetworkStatus", "email", "/User/Email/NetworkStatus",
													NULL, G_DBUS_SIGNAL_FLAGS_NONE, _noti_mgr_on_gdbus_event_receive, (void *)ug_data, NULL);
		retvm_if(ug_data->viewer_storage_id == GDBUS_SIGNAL_SUBSCRIBE_FAILURE, EINA_FALSE, "Failed to g_dbus_connection_signal_subscribe()");
	}

	debug_leave();
	return EINA_TRUE;
}

void noti_mgr_dbus_receiver_remove(EmailViewerUGD *ug_data)
{
	debug_enter();
	retm_if(ug_data == NULL, "Invalid parameter: ug_data[NULL]");

	g_dbus_connection_signal_unsubscribe(ug_data->viewer_dbus_conn, ug_data->viewer_network_id);
	g_dbus_connection_signal_unsubscribe(ug_data->viewer_dbus_conn, ug_data->viewer_storage_id);
	g_object_unref(ug_data->viewer_dbus_conn);
	ug_data->viewer_dbus_conn = NULL;
	ug_data->viewer_network_id = 0;
	ug_data->viewer_storage_id = 0;

	debug_leave();
}

static void _passwd_popup_cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;
	DELETE_EVAS_OBJECT(ug_data->passwd_popup);
}

static void _continue_download_body(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;
	DELETE_EVAS_OBJECT(ug_data->passwd_popup);
	_download_body(ug_data);
}

static void _handle_auth_error(EmailViewerUGD *ug_data, int error, Evas_Smart_Cb continue_cb)
{
	email_authentication_method_t auth_method = EMAIL_AUTHENTICATION_METHOD_NO_AUTH;
	email_account_t *account = NULL;
	int ret = email_get_account(ug_data->account_id, EMAIL_ACC_GET_OPT_DEFAULT, &account);
	if (ret == EMAIL_ERROR_NONE && account) {
		auth_method = account->incoming_server_authentication_method;
	}
	if (account) email_free_account(&account, 1);

	bool show_passwd_popup =
		error == EMAIL_ERROR_AUTH_REQUIRED ||
		error == EMAIL_ERROR_AUTHENTICATE ||
		error == EMAIL_ERROR_LOGIN_FAILURE;

	if (!show_passwd_popup || !continue_cb ||
			auth_method == EMAIL_AUTHENTICATION_METHOD_XOAUTH2) {
		/* in case of Google account, password changed popup should not created. */
		char *s = email_util_get_login_failure_string(ug_data->account_id);
		if (s) {
			notification_status_message_post(s);
			free(s);
		}
	} else {
		debug_log("error[%d], account_id[%d]", error, ug_data->account_id);
		DELETE_EVAS_OBJECT(ug_data->passwd_popup);
		ug_data->passwd_popup = email_util_create_password_changed_popup(
				ug_data->base.module->navi, ug_data->account_id, continue_cb,
				_passwd_popup_cancel_cb, ug_data);
	}
}

static bool _is_auth_error(int error)
{
	return error == EMAIL_ERROR_AUTH_REQUIRED ||
		error == EMAIL_ERROR_AUTHENTICATE ||
		error == EMAIL_ERROR_LOGIN_FAILURE ||
		error == EMAIL_ERROR_AUTH_NOT_SUPPORTED ||
		error == EMAIL_ERROR_LOGIN_ALLOWED_EVERY_15_MINS ||
		error == EMAIL_ERROR_TOO_MANY_LOGIN_FAILURE;
}

static bool _is_connection_error(int error)
{
	return error == EMAIL_ERROR_CONNECTION_BROKEN ||
		error == EMAIL_ERROR_CONNECTION_FAILURE ||
		error == EMAIL_ERROR_FLIGHT_MODE_ENABLE ||
		error == EMAIL_ERROR_NETWORK_NOT_AVAILABLE ||
		error == EMAIL_ERROR_NETWORK_TOO_BUSY ||
		error == EMAIL_ERROR_NO_SIM_INSERTED;
}

static bool _is_storage_full_error(int error)
{
	return error == EMAIL_ERROR_MAIL_MEMORY_FULL ||
		error ==  EMAIL_ERROR_OUT_OF_MEMORY ||
		error == EMAIL_ERROR_SERVER_STORAGE_FULL;
}

static void _noti_mgr_on_gdbus_event_receive(GDBusConnection *connection,
		const gchar *sender_name,
		const gchar *object_path,
		const gchar *interface_name,
		const gchar *signal_name,
		GVariant *parameters,
		gpointer data)
{
	debug_enter();

	retm_if(!data, "data is NULL");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	debug_secure("Object path=%s, interface name=%s, signal name=%s", object_path, interface_name, signal_name);
	if (ug_data->b_viewer_hided == TRUE) {
		debug_log("viewer hided -> no need to get dbus noti");
		return;
	}

	if (!(g_strcmp0(object_path, "/User/Email/StorageChange")) && !(g_strcmp0(signal_name, "email"))) {
		debug_log("User.Email.StorageChange");

		int subtype = 0;
		int data1 = 0;
		int data2 = 0;
		char *data3 = NULL;
		int data4 = 0;

		g_variant_get(parameters, "(iiisi)", &subtype, &data1, &data2, &data3, &data4);
		debug_log("STORAGE: subtype: %d, data1: %d, data2: %d, data3: %s, data4: %d", subtype, data1, data2, data3, data4);

		switch (subtype) {
		case NOTI_ACCOUNT_ADD:
			/* DATA1[account_id] */
			debug_log("NOTI_ACCOUNT_ADD (account_id:%d)", data1);
			break;

		case NOTI_ACCOUNT_DELETE:
			/* DATA1[account_id] */
			debug_log("NOTI_ACCOUNT_DELETE (account_id:%d)", data1);
				if (ug_data->account_id == data1) {
					debug_log("email_module_make_destroy_request");
					email_module_make_destroy_request(ug_data->base.module);
				}
			break;

		case NOTI_ACCOUNT_UPDATE:
			/* DATA1[account_id] */
			debug_log("NOTI_ACCOUNT_UPDATE (account_id:%d)", data1);
			break;

		case NOTI_MAIL_UPDATE:
			debug_log("NOTI_MAIL_UPDATE");
			/* DATA1[account_id] DATA2[mail_id] DATA3[mailbox_id] DATA4[email_mail_change_type_t] */
			int mail_id = data2;
			int type = data4;
			debug_log("account_id : %d, mail_id : %d, mailbox_id : %d, type : %d", data1, mail_id, atoi(data3), data4);
			if (type == UPDATE_PARTIAL_BODY_DOWNLOAD) {
				debug_log("ug_data->mail_id : %d, mail_id : %d", ug_data->mail_id, mail_id);
				if (ug_data->mail_id == mail_id) {
					if (viewer_create_temp_folder(ug_data) < 0) {
						debug_log("creating viewer temp folder is failed");
						ug_data->eViewerErrorType = VIEWER_ERROR_FAIL;
					}
					G_FREE(ug_data->temp_viewer_html_file_path);
					ug_data->temp_viewer_html_file_path = g_strdup_printf("%s/%s", ug_data->temp_folder_path, EMAIL_VIEWER_TMP_HTML_FILE);

					viewer_set_internal_data(ug_data, EINA_TRUE);
					_reset_view(ug_data, EINA_TRUE);
				}
			} else if (type == UPDATE_MAIL) {
				debug_log("UPDATE_MAIL!");
			} else if (type == APPEND_BODY) {
				debug_log("APPEND_BODY!");
			}
			break;

		case NOTI_MAIL_DELETE_FINISH:
			/* DATA1[account_id] DATA2[email_delete_type] DATA3[mail_id,mail_id] */
			debug_log("NOTI_MAIL_DELETE_FINISH");
			int deleted_mail_id = atoi(data3);
			debug_log("account_id : %d, delete_type : %d, mail_id : %d", data1, data2, deleted_mail_id);
			if (ug_data->mail_id == deleted_mail_id) {
				if (ug_data->mailbox_type != EMAIL_MAILBOX_TYPE_OUTBOX) {
					int ret = notification_status_message_post(email_get_email_string("IDS_EMAIL_POP_THIS_EMAIL_HAS_BEEN_DELETED_FROM_THE_SERVER"));
					debug_warning_if(ret != NOTIFICATION_ERROR_NONE, "notification_status_message_post() failed! ret:(%d)", ret);
				}

				if (ug_data->loaded_module != NULL) {
					debug_log("destroying the viewer is pending....");
					ug_data->need_pending_destroy = EINA_TRUE;
				} else {
					if (ug_data->can_destroy_on_msg_delete) {
						debug_log("email_module_make_destroy_request");
						email_module_make_destroy_request(ug_data->base.module);
					} else {
						debug_log("pending destroy");
						ug_data->need_pending_destroy = EINA_TRUE;
					}
				}
			}
			break;

		case NOTI_MAIL_MOVE:
			debug_log("NOTI_MAIL_MOVE");
			break;

		case NOTI_MAIL_MOVE_FINISH:
			debug_log("NOTI_MAIL_MOVE_FINISH");
			/* DATA1[account_id] DATA2[move_type] DATA3[mailbox_id0x01updated_value0x01mail_id] DATA4[??] */
			if (ug_data->account_id == data1) {
				int src_mailbox_id = -1, dst_mailbox_id = -1, mail_id = -1;
				/* notification format: <src_folder><0x01><dst_folder><0x01><<mail_id><,><mail_id>>*/
				_noti_mgr_parse_mail_move_finish_data_params(data3, &src_mailbox_id, &dst_mailbox_id, &mail_id);
				debug_log("src_mailbox_id : %d, dst_mailbox_id : %d, mail_id : %d", src_mailbox_id, dst_mailbox_id, mail_id);

				if (ug_data->mail_id == mail_id) {
					debug_log("email_module_make_destroy_request");
					email_module_make_destroy_request(ug_data->base.module);
				}
			}
			break;

		case NOTI_MAIL_FIELD_UPDATE:
			/* DATA1[account_id] DATA2[email_mail_attribute_type] DATA3[updated_field_name0x01mail_id,mail_id] DATA4[updated_value] */
			debug_log("NOTI_MAIL_FIELD_UPDATE");
			debug_log("ug_data->mail_id : %d", ug_data->mail_id);
			int i = 0;
			int update_type = data2;
			int value = data4;
			char *field_name = NULL;
			GList *mail_list = NULL;
			int *idx = NULL;

			_noti_mgr_parse_mail_field_update_data_params(data3, &field_name, &mail_list);
			debug_log("update_type: %d, value: %d, mail_list count: %d, account_id : %d", update_type, value, g_list_length(mail_list), data1);

			for (i = 0; i < g_list_length(mail_list); i++) {
				idx = (typeof(idx)) g_list_nth_data(mail_list, i);
				if (idx && (ug_data->mail_id == *idx)) {
					debug_log("idx: [%d]", *idx);
					break;
				}
			}
			if (idx == NULL) {
				debug_log("idx is NULL");
				FREE(field_name);
				return;
			}
			switch (update_type) {
				case EMAIL_MAIL_ATTRIBUTE_FLAGS_FLAGGED_FIELD:
					debug_log("EMAIL_MAIL_ATTRIBUTE_FLAGS_FLAGGED_FIELD");
					if (ug_data->mail_id == *idx) {
						int err = 0;
						email_mail_data_t *mail_data = NULL;
						err = email_get_mail_data(ug_data->mail_id, &mail_data);
						if (err == EMAIL_ERROR_NONE && mail_data != NULL) {
							ug_data->favorite = mail_data->flags_flagged_field;
							debug_log("favorite (%d)", ug_data->favorite);

							header_update_favorite_icon(ug_data);
						}

						if (mail_data != NULL) {
							debug_log("free mail_data");
							email_free_mail_data(&mail_data, 1);
							mail_data = NULL;
						}
					}
					break;

				case EMAIL_MAIL_ATTRIBUTE_FLAGS_FORWARDED_FIELD:
					debug_log("EMAIL_MAIL_ATTRIBUTE_FLAGS_FORWARDED_FIELD");
					/* no break */
				case EMAIL_MAIL_ATTRIBUTE_FLAGS_ANSWERED_FIELD:
					debug_log("EMAIL_MAIL_ATTRIBUTE_FLAGS_ANSWERED_FIELD");
					if (ug_data->mail_id == *idx) {
						int err = 0;
						email_mail_data_t *mail_data = NULL;
						err = email_get_mail_data(ug_data->mail_id, &mail_data);
						if (err == EMAIL_ERROR_NONE && mail_data != NULL) {
							if (ug_data->subject_ly) {
								header_update_response_icon(ug_data, mail_data);
							}
						}

						if (mail_data != NULL) {
							debug_log("free mail_data");
							email_free_mail_data(&mail_data, 1);
							mail_data = NULL;
						}
					}
					break;

				case EMAIL_MAIL_ATTRIBUTE_FLAGS_DELETED_FIELD:
					debug_log("EMAIL_MAIL_ATTRIBUTE_FLAGS_DELETED_FIELD");
					if (ug_data->mail_id == *idx) {
						if ((ug_data->mailbox_type != EMAIL_MAILBOX_TYPE_OUTBOX) && (ug_data->move_status != EMAIL_ERROR_NONE)) {
							debug_log("This email has been deleted from the server. (mailbox_type:%d, move_status:%d)", ug_data->mailbox_type, ug_data->move_status);
						}
						debug_log("email_module_make_destroy_request");
						email_module_make_destroy_request(ug_data->base.module);
					}
					break;

				default:
					debug_log("Uninterested update type %d", update_type);
					break;
			}

			if (mail_list) {
				for (i = 0; i < g_list_length(mail_list); i++) {
					idx = (typeof(idx)) g_list_nth_data(mail_list, i);
					if (idx && (ug_data->mail_id == *idx)) {
						debug_log("idx: [%d]", *idx);
						FREE(idx);
						break;
					}
				}
				g_list_free(mail_list);
				mail_list = NULL;
			}
			FREE(field_name);
			break;

		default:
			debug_warning("unknown type");
			break;
		}
		g_free(data3);
	} else if (!(g_strcmp0(object_path, "/User/Email/NetworkStatus")) && !(g_strcmp0(signal_name, "email"))) {
		debug_log("User.Email.NetworkStatus");

		int subtype = 0;
		int data1 = 0;
		char *data2 = NULL;
		int data3 = 0;
		int data4 = 0;

		g_variant_get(parameters, "(iisii)", &subtype, &data1, &data2, &data3, &data4);
		debug_secure("NETWORK : subtype: %d, data1: %d, data2: %s, data3: %d, data4: %d", subtype, data1, data2, data3, data4);

		switch (subtype) {
		case NOTI_DOWNLOAD_BODY_START:
			/* DATA1[mail_id] DATA2[file_id] DATA3[body_size] DATA4[received size] */
			debug_log("receive noti, DOWNLOAD_BODY");
			if (ug_data->mail_id == data1) {
				ug_data->file_id = g_strdup(data2);
				ug_data->file_size = data3;

				if (data4 == 0) {
					_noti_mgr_set_value_down_progress(ug_data, 0.0);
				} else {
					double per = (double)data4 / (double)data3;
					_noti_mgr_set_value_down_progress(ug_data, per);
				}
			} else {
				debug_log("mail_id is different");
				debug_log("expected mail_id [%d], received mail_id [%d]", ug_data->mail_id, data1);
			}
			break;

		case NOTI_DOWNLOAD_MULTIPART_BODY:
			/* DATA1[mail_id] DATA2[file_id] DATA3[body_size] DATA4[received size] */
			debug_log("receive noti, DOWNLOAD_MULTIPART_BODY");
			if (ug_data->mail_id == data1) {
				ug_data->file_id = g_strdup(data2);
				ug_data->file_size = data3;

				if (data4 == 0) {
					_noti_mgr_set_value_down_progress(ug_data, 0.0);
				} else {
					double per = (double)data4 / (double)data3;
					_noti_mgr_set_value_down_progress(ug_data, per);
				}
			} else {
				debug_log("mail_id is different");
				debug_log("expected mail_id [%d], received mail_id [%d]", ug_data->mail_id, data1);
			}
			break;

		case NOTI_DOWNLOAD_BODY_FINISH:
			/* DATA1[mail_id] DATA2[NULL] DATA3[handle_id] */
			debug_log("receive noti, DOWNLOAD_BODY_FINISH");
			if (ug_data->mail_id == data1) {
				/* delete progress */
				if (ug_data->email_handle == data3) {
					if (ug_data->pb_notify) {
						_noti_mgr_set_value_down_progress(ug_data, 1.0);
						viewer_destroy_down_progress_cb(ug_data, NULL, NULL);
						viewer_delete_body_button(ug_data);
					}
					ug_data->email_handle = 0;
				} else {
					debug_log("email_handle is different");
					debug_log("expected email_handle [%d], received email_handle [%d]", ug_data->email_handle, data3);
					break;
				}
				/* set property */
				viewer_set_internal_data(ug_data, EINA_TRUE);
				/* create att and body */
				if (ug_data->b_partial_body) {
					int scr_x = 0;
					int scr_y = 0;
					int scr_w = 0;
					int scr_h = 0;
					elm_scroller_region_get(ug_data->scroller, &scr_x, &scr_y, &scr_w, &scr_h);
					debug_log("scroller region> x[%d] y[%d] w[%d] h[%d]", scr_x, scr_y, scr_w, scr_h);

					if (ug_data->has_html) {
						ug_data->webview_data->body_type = BODY_TYPE_HTML;
						ug_data->webview_data->uri = ug_data->body_uri;
					} else {
						ug_data->webview_data->body_type = BODY_TYPE_TEXT;
						ug_data->webview_data->text_content = ug_data->body;
					}

					viewer_set_webview_content(ug_data);

					elm_object_focus_set(ug_data->sender_ly, EINA_TRUE);
					elm_scroller_region_show(ug_data->scroller, scr_x, scr_y, scr_w, scr_h);
					header_update_view(ug_data);
				} else {
					debug_log("NOT b_partial_body");
					viewer_check_body_download(ug_data);
					header_update_view(ug_data);
				}
			} else {
				debug_log("email_handle or mail_id is different");
				debug_log("expected mail_id [%d], received mail_id [%d]", ug_data->mail_id, data1);
				debug_log("expected email_handle [%d], received email_handle [%d]", ug_data->email_handle, data3);
			}

			break;

		case NOTI_DOWNLOAD_BODY_FAIL:
			/* DATA1[mail_id] DATA2[NULL] DATA3[handle_id] DATA4[error_code] */
			debug_log("receive noti, DOWNLOAD_BODY_FAIL err[%d]", data4);
			if (ug_data->email_handle == data3 && ug_data->mail_id == data1) {
				ug_data->email_handle = 0;

				/* delete progress */
				if (ug_data->pb_notify) {
					viewer_destroy_down_progress_cb(ug_data, NULL, NULL);
				}

				if (_is_auth_error(data4)) {
					_handle_auth_error(ug_data, data4, _continue_download_body);
				} else if (_is_connection_error(data4)) {
					notification_status_message_post(_("IDS_EMAIL_TPOP_FAILED_TO_CONNECT_TO_NETWORK"));
				} else {
					notification_status_message_post(_("IDS_EMAIL_POP_DOWNLOAD_FAILED"));
				}
			} else {
				debug_log("email_handle or mail_id is different");
				debug_log("expected mail_id [%d], received mail_id [%d]", ug_data->mail_id, data1);
				debug_log("expected email_handle [%d], received email_handle [%d]", ug_data->email_handle, data3);
			}
			break;

		case NOTI_DOWNLOAD_ATTACH_START:
			/* DATA1[mail_id] DATA2[file_name] DATA3[info_index] DATA4[percentage] */
			debug_log("receive noti, NOTI_DOWNLOAD_ATTACH");
			if (ug_data->mail_id == data1) {

				EV_attachment_data *aid = viewer_get_attachment_data(ug_data, data3);
				retm_if(aid == NULL, "aid is NULL.");
				retm_if(aid->download_handle == 0, "aid->download_handle is 0.");

				if (data4 == 0) {
					debug_log("download has started");
				} else if (data4 > 0) {
					debug_log("download is in progress");
					aid->progress_val = data4 * 100;
					if (aid->state != EV_ATT_STATE_IN_PROGRESS) {
						viewer_set_attachment_state(aid, EV_ATT_STATE_IN_PROGRESS);
					}
				}

			} else {
				debug_log("mail_id is different");
				debug_log("expected mail_id [%d], received mail_id [%d]", ug_data->mail_id, data1);
			}
			break;

		case NOTI_DOWNLOAD_ATTACH_FINISH:
			{
				/* DATA1[mail_id] DATA2[NULL] DATA3[attachment_id] */
				debug_log("receive noti, DOWNLOAD_ATTACH_FINISH");
				if (ug_data->mail_id == data1) {

					EV_attachment_data *aid = viewer_get_attachment_data(ug_data, data3);
					retm_if(aid == NULL, "aid is NULL.");
					retm_if(aid->download_handle == 0, "aid->download_handle is 0.");

					aid->download_handle = 0;
					aid->is_busy = false;

					viewer_update_attachment_item_info(aid);
					header_update_attachment_summary_info(ug_data);

					viewer_download_and_preview_save_attachment(aid);

				} else {
					debug_log("mail_id is different");
					debug_log("expected mail_id [%d], received mail_id [%d]", ug_data->mail_id, data1);
				}
			}
			break;

		case NOTI_DOWNLOAD_ATTACH_FAIL:
		case NOTI_DOWNLOAD_ATTACH_CANCEL:
			if (subtype == NOTI_DOWNLOAD_ATTACH_FAIL) {
				/* DATA1[mail_id] DATA2[NULL] DATA3[attachment_id] DATA4[error_code] */
				debug_log("receive noti, DOWNLOAD_ATTACH_FAIL err[%d]", data4);
			} else {
				/* DATA1[mail_id] DATA2[NULL] DATA3[attachment_id] DATA4[??] */
				debug_log("receive noti, NOTI_DOWNLOAD_ATTACH_CANCEL");
			}

			if (ug_data->mail_id == data1) {

				EV_attachment_data *aid = viewer_get_attachment_data(ug_data, data3);
				retm_if(aid == NULL, "aid is NULL.");

				aid->is_busy = false;

				if ((subtype == NOTI_DOWNLOAD_ATTACH_CANCEL) || (data4 == EMAIL_ERROR_CANCELLED)) {
					debug_log("Download was cancelled.");
					aid->download_handle = 0;
					if (aid->state == EV_ATT_STATE_PENDING) {
						debug_log("New download is pending. Start downloading...");
						viewer_download_and_preview_save_attachment(aid);
					} else {
						viewer_set_attachment_state(aid, EV_ATT_STATE_IDLE);
					}
					break;
				}

				retm_if(aid->download_handle == 0, "aid->download_handle is 0.");

				aid->download_handle = 0;
				viewer_set_attachment_state(aid, EV_ATT_STATE_IDLE);

				retm_if(ug_data->notify, "Popup is already shown, no need to show one more");
				if (_is_auth_error(data4)) {
					_handle_auth_error(ug_data, data4, NULL);
				} else if (_is_storage_full_error(data4)) {
					viewer_show_storage_full_popup(ug_data);
				} else {
					char *err_msg = NULL;
					if (_is_connection_error(data4)) {
						ug_data->package_type2 = PACKAGE_TYPE_NOT_AVAILABLE;
						ug_data->translation_string_id2 = strdup("IDS_EMAIL_POP_FAILED_TO_DOWNLOAD_ATTACHMENT_CHECK_YOUR_NETWORK_CONNECTION_AND_TRY_AGAIN");
						err_msg = g_strdup(email_get_email_string("IDS_EMAIL_POP_FAILED_TO_DOWNLOAD_ATTACHMENT_CHECK_YOUR_NETWORK_CONNECTION_AND_TRY_AGAIN"));
					} else {
						err_msg = _noti_mgr_get_service_fail_type(data4, ug_data);
					}

					email_string_t EMAIL_VIEWER_FAIL_MSG = { NULL, err_msg};
					ug_data->translation_string_id1 = strdup(err_msg);
					ug_data->str_format1 = strdup("%s");
					ug_data->package_type1 = PACKAGE_TYPE_NOT_AVAILABLE;
					ug_data->popup_element_type = POPUP_ELEMENT_TYPE_CONTENT;

					email_string_t title = { PACKAGE, "IDS_EMAIL_HEADER_UNABLE_TO_DOWNLOAD_ATTACHMENT_ABB" };
					email_string_t btn = { PACKAGE, "IDS_EMAIL_BUTTON_OK" };
					email_string_t null_str = { NULL, NULL };

					util_create_notify(ug_data, title, EMAIL_VIEWER_FAIL_MSG, 1,
							btn, _noti_mgr_popup_response_cb, null_str, NULL, NULL);
					g_free(err_msg);
				}

			} else {
				debug_log("mail_id is different");
				debug_log("expected mail_id [%d], received mail_id [%d]", ug_data->mail_id, data1);
			}
			break;

		case NOTI_SEND_FINISH:
			/* DATA1[account_id] DATA2[NULL] DATA3[mail_id] DATA4[??] */
			debug_log("receive noti, NOTI_SEND_FINISH");
			if ((ug_data->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX) &&
				(ug_data->account_id == data1) && (ug_data->mail_id == data3)) {
				debug_log("email_module_make_destroy_request");
				email_module_make_destroy_request(ug_data->base.module);
			}
			break;

		case NOTI_SEND_FAIL:
			/* fall through */
		case NOTI_SEND_CANCEL:
			/* DATA1[account_id] DATA2[NULL] DATA3[mail_id] DATA4[??] */
			debug_log("receive noti, NOTI_SEND_FAIL or NOTI_SEND_CANCEL");
			if ((ug_data->account_id == data1) && (ug_data->mail_id == data3)) {
				if (ug_data->cancel_sending_ctx_item) {
					elm_object_item_disabled_set(ug_data->cancel_sending_ctx_item, EINA_FALSE);
				}
				ug_data->is_cancel_sending_btn_clicked = EINA_FALSE;
			}
			break;

		default:
			debug_log("unknown type");
			break;
		}
		g_free(data2);
		return;
	}
	debug_leave();
	return;
}
