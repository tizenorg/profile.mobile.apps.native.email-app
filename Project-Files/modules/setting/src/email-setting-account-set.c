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

#include "email-setting.h"
#include "email-view-setting.h"
#include "email-setting-account-set.h"
#include "email-setting-utils.h"

#include <tzplatform_config.h>

static email_setting_string_t EMAIL_SETTING_STRING_SENT_FROM_SAMSUNG_MOBILE = {PACKAGE, "IDS_EMAIL_SBODY_SENT_FROM_MY_SAMSUNG_DEVICE"};

#define ACCOUNT_ICON_PATH tzplatform_mkpath(TZ_SYS_RO_ICONS, "org.tizen.email.png")
#define DEFAULT_SIGNATURE EMAIL_SETTING_STRING_SENT_FROM_SAMSUNG_MOBILE.id
#define DEFAULT_EMAIL_SIZE 1024*50

static void _account_info_print(email_account_t *account);
static void _set_display_name_with_email_addr(char *email_addr, char **display_name);
static char *_get_default_server_address(char *email_addr, int account_type, int server_type);
static void _set_default_setting_to_account(email_view_t *vd, email_account_t *account);
static void _set_setting_account_provider_info_to_email_account_t(email_account_t *account, setting_account_provider_info_t *info);
static void _set_setting_account_server_info_to_email_account_t(email_account_t *account, setting_account_server_info_t *info);

int setting_is_in_default_provider_list(email_view_t *vd, const char *email_addr)
{
	debug_enter();

	int ret = FALSE;
	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->module;
	GSList *l = NULL;

	l = ugd->default_provider_list;
	while (l) {
		setting_account_provider_info_t *info = l->data;
		if (g_str_has_suffix(email_addr, info->provider_domain)) {
			debug_secure("provider: %s - %s", info->provider_id, info->provider_domain);
			ret = TRUE;
			break;
		}
		l = g_slist_next(l);
	}
	return ret;
}

void setting_set_default_provider_info_to_account(email_view_t *vd, email_account_t *account)
{
	debug_enter();

	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->module;
	char *email_addr = g_strdup(account->user_email_address);
	GSList *l = NULL;

	l = ugd->default_provider_list;
	while (l) {
		setting_account_provider_info_t *info = l->data;
		if (g_str_has_suffix(email_addr, info->provider_domain)) {
			_set_setting_account_provider_info_to_email_account_t(account, info);
			break;
		}
		l = g_slist_next(l);
	}

	FREE(email_addr);

	/* set default common setting */
	_set_default_setting_to_account(vd, account);

	_account_info_print(account);
}

void setting_set_others_account(email_view_t *vd)
{
	debug_enter();
	retm_if(!vd, "vd is NULL");

	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->module;
	email_account_t *account = ugd->new_account;

	/* set server configuration if default provider. */
	setting_set_default_provider_info_to_account(vd, account);
}

void setting_set_others_account_server_default_type(email_account_t *account, int account_type,
		int incoming_protocol, int outgoing_protocol)
{
	debug_enter();
	retm_if(!account, "invalid parameter");

	if (incoming_protocol < 0) {
		debug_log("bypass incoming_protocol");
	} else {
		if (account_type == 0)
			account->incoming_server_type = EMAIL_SERVER_TYPE_IMAP4;
		else
			account->incoming_server_type = EMAIL_SERVER_TYPE_POP3;
		account->incoming_server_address = _get_default_server_address(
				account->user_email_address,
				account_type,
				0);
		if (incoming_protocol == 0) { /* NONE */
			if (account_type == 0) /* IMAP */
				account->incoming_server_port_number = 143;
			else /* POP3 */
				account->incoming_server_port_number = 110;
			account->incoming_server_secure_connection = 0;
		} else if (incoming_protocol == 1) { /* SSL */
			if (account_type == 0) /* IMAP */
				account->incoming_server_port_number = 993;
			else /* POP3 */
				account->incoming_server_port_number = 995;
			account->incoming_server_secure_connection = 1;
		} else { /* TLS */
			account->incoming_server_port_number = 143;
			account->incoming_server_secure_connection = 2;
		}
	}

	if (outgoing_protocol < 0) {
		debug_log("bypass outgoing_protocol");
	} else {
		account->outgoing_server_address = _get_default_server_address(
				account->user_email_address,
				account_type,
				1);
		if (outgoing_protocol == 0) { /* NONE */
			account->outgoing_server_port_number = 25;
			account->outgoing_server_secure_connection = 0;
		} else if (outgoing_protocol == 1) { /* SSL */
			account->outgoing_server_port_number = 465;
			account->outgoing_server_secure_connection = 1;
		} else { /* TLS */
			account->outgoing_server_port_number = 25;
			account->outgoing_server_secure_connection = 2;
		}
	}
}

static void _set_display_name_with_email_addr(char *email_addr, char **display_name)
{
	debug_enter();

	retm_if(!email_addr, "invalid parameter");

	char *buf;

	buf = g_strdup(email_addr);
	*display_name = g_strdup(strtok(buf, "@"));
	debug_secure("email address : %s", email_addr);
	debug_secure("buf email address :%s", buf);
	debug_secure("display_name :%s", *display_name);
	free(buf);
}

static char *_get_default_server_address(char *email_addr, int account_type, int server_type)
{
	debug_enter();
	retvm_if(!email_addr, NULL, "invalid parameter");

	char ret[256] = {'\0',};
	char *buf = NULL;
	char **split = NULL;

	buf = g_strdup(email_addr);
	split = g_strsplit(buf, "@", 2);

	debug_secure("domain: %s", split[1]);
	if (server_type == 0) { /* incoming server */
		if (account_type == 0) /* imap */
			g_snprintf(ret, 256, "imap.%s", split[1]);
		else /* pop3 */
			g_snprintf(ret, 256, "pop3.%s", split[1]);
	} else { /* outgoing server */
		g_snprintf(ret, 256, "smtp.%s", split[1]);
	}

	g_free(buf);
	g_strfreev(split);

	return g_strdup(ret);
}

static void _account_info_print(email_account_t *account)
{
	debug_enter();

	retm_if(!account, "account is NULL");

	debug_secure("account name: %s", account->account_name);
	debug_secure("incoming server user name: %s", account->incoming_server_user_name);
	debug_secure("user display name: %s", account->user_display_name);
	debug_secure("user email addr: %s", account->user_email_address);

	debug_secure("incoming server address: %s", account->incoming_server_address);
	debug_secure("outgoing server address: %s", account->outgoing_server_address);

	debug_secure("incoming server type: %d", account->incoming_server_type);
	debug_secure("incoming server port :%d", account->incoming_server_port_number);
	debug_secure("incoming server ssl : %d", account->incoming_server_secure_connection);

	debug_secure("outgoing server port: %d", account->outgoing_server_port_number);
	debug_secure("outgoing server ssl: %d", account->outgoing_server_secure_connection);
	debug_secure("outgoing server user name: %s", account->outgoing_server_user_name);
}

static void _set_default_setting_to_account(email_view_t *vd, email_account_t *account)
{
	retm_if(!vd, "vd is NULL");
	retm_if(!account, "account is NULL");

	EmailSettingUGD *ugd = (EmailSettingUGD *)vd->module;

	FREE(account->incoming_server_user_name);
	account->incoming_server_user_name = g_strdup(account->user_email_address);
	_set_display_name_with_email_addr(account->user_email_address, &(account->user_display_name));

	account->reply_to_address = g_strdup(account->user_email_address);
	account->return_address = g_strdup(account->user_email_address);
	account->retrieval_mode = EMAIL_IMAP4_RETRIEVAL_MODE_ALL;
	account->auto_resend_times = 0;
	account->auto_download_size = DEFAULT_EMAIL_SIZE;
	account->outgoing_server_use_same_authenticator = 1;
	account->default_mail_slot_size = 50;
	account->pop_before_smtp = 0;
	account->incoming_server_requires_apop = 0;
	account->options.priority = 3;
	account->options.keep_local_copy = 1;
	account->options.req_delivery_receipt = 0;
	account->options.req_read_receipt = 0;
	account->options.download_limit = 0;
	account->options.block_address = 1;
	account->options.block_subject = 1;
	account->options.display_name_from = NULL;
	account->options.reply_with_body = 1;
	account->options.forward_with_files = 1;
	account->options.add_myname_card = 0;
	account->options.add_signature = 1;
	account->options.signature = strdup(DEFAULT_SIGNATURE);
	account->logo_icon_path = strdup(ACCOUNT_ICON_PATH);
	account->outgoing_server_type = EMAIL_SERVER_TYPE_SMTP;
	account->outgoing_server_need_authentication = EMAIL_AUTHENTICATION_METHOD_DEFAULT;
	account->incoming_server_authentication_method = EMAIL_AUTHENTICATION_METHOD_NO_AUTH;
	account->outgoing_server_password = g_strdup(account->incoming_server_password);
	account->outgoing_server_user_name = g_strdup(account->incoming_server_user_name);
	account->peak_interval = account->check_interval = -1;
	account->peak_days = EMAIL_PEAK_DAYS_MONDAY | EMAIL_PEAK_DAYS_TUEDAY | EMAIL_PEAK_DAYS_WEDDAY |
		EMAIL_PEAK_DAYS_THUDAY | EMAIL_PEAK_DAYS_FRIDAY;
	account->peak_days = -abs(account->peak_days);
	account->peak_start_time = 800;
	account->peak_end_time = 1700;
	account->wifi_auto_download = 0;

	account->options.notification_status = 1;
	account->options.vibrate_status = 1;
	account->options.display_content_status = 1;
	account->options.default_ringtone_status = 1;
	account->options.alert_ringtone_path = g_strdup(DEFAULT_EMAIL_RINGTONE_PATH);

	/*user data set*/
	account_user_data_t data;
	memset(&data, 0x00, sizeof(account_user_data_t));
	data.show_images = 0;
	data.send_read_report = 0;
	data.pop3_deleting_option = 0;
	snprintf(data.service_provider_name, 128, "%s", setting_get_provider_name(ugd));

	int data_len = sizeof(account_user_data_t);
	account->user_data = calloc(1, data_len);
	retm_if(!account->user_data, "account->user_data is NULL!");
	memcpy(account->user_data, &data, data_len);
	account->user_data_length = data_len;

}

void _set_setting_account_provider_info_to_email_account_t(email_account_t *account, setting_account_provider_info_t *info)
{
	debug_enter();
	retm_if(!account, "account is NULL");
	retm_if(!info, "info is NULL");

	setting_account_server_info_t *server_info = NULL;

	GSList *incoming_server_list = info->incoming_server_list;
	while (incoming_server_list) {
		server_info = incoming_server_list->data;
		_set_setting_account_server_info_to_email_account_t(account, server_info);
		incoming_server_list = g_slist_next(incoming_server_list);
	}

	GSList *outgoing_server_list = info->outgoing_server_list;
	while (outgoing_server_list) {
		server_info = outgoing_server_list->data;
		_set_setting_account_server_info_to_email_account_t(account, server_info);
		outgoing_server_list = g_slist_next(outgoing_server_list);
	}
}

void _set_setting_account_server_info_to_email_account_t(email_account_t *account, setting_account_server_info_t *info)
{
	debug_enter();
	retm_if(!account, "account is NULL");
	retm_if(!info, "info is NULL");

	if (info->provider_server_type == 3) {
		account->outgoing_server_type = info->provider_server_type;
		FREE(account->outgoing_server_address);
		account->outgoing_server_address = g_strdup(info->provider_server_addr);
		account->outgoing_server_port_number = info->provider_server_port;
		account->outgoing_server_secure_connection = info->provider_server_security_type;
	} else {
		account->incoming_server_type = info->provider_server_type;
		FREE(account->incoming_server_address);
		account->incoming_server_address = g_strdup(info->provider_server_addr);
		account->incoming_server_port_number = info->provider_server_port;
		account->incoming_server_secure_connection = info->provider_server_security_type;
	}
}

/* EOF */
