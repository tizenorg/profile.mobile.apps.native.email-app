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

#ifndef __EMAIL_SETTING_H__
#define __EMAIL_SETTING_H__

#undef LOG_TAG
#define LOG_TAG "EMAIL_SETTING"

#include <Elementary.h>
#include <Ecore_IMF.h>

#include <libintl.h>
#include <bundle.h>

#include <notification.h>
#include <glib.h>
#include <gio/gio.h>

#include "email-types.h"
#include "email-api.h"

#include "email-engine.h"
#include "email-debug.h"
#include "email-locale.h"
#include "email-utils.h"
#include "email-common-types.h"
#include "email-setting-string.h"
#include "email-module-dev.h"

#define VIEW_ST_MAX 20		/* Maximum View Stack */

#define _EDJ(o) elm_layout_edje_get(o)

#define DEFAULT_EMAIL_RINGTONE_PATH "/opt/usr/share/settings/Alerts/Postman(Default_Email).ogg"

typedef enum _ViewType ViewType;
enum _ViewType {
	VIEW_SETTING = 1000,
	VIEW_ACCOUNT_SETUP,
	VIEW_MANUAL_SETUP,
	VIEW_ACCOUNT_EDIT,
	VIEW_ACCOUNT_DETAILS,
	VIEW_ACCOUNT_DETAILS_SETUP,
	VIEW_NOTIFICATION_SETTING,
	VIEW_SIGNATURE_SETTING,
	VIEW_SIGNATURE_EDIT,

	/* error case */
	VIEW_INVALID,
};

typedef struct sp_desc EmailSpDesc;
struct sp_desc {
	int index;
	char *id;
	char *csc_code;
	char *name;
	char *name_str_id;
	char *icon_path;
	char *sending_address;
	int sending_port;
	int sending_security;
	char *receiving_server_type;
	char *receiving_address;
	int receiving_port;
	int receiving_security;
	int rgb;
};

enum {
	PRIORITY_SENDER_MODE_ADD = 1,
	PRIORITY_SENDER_MODE_EDIT,
	PRIORITY_SENDER_MODE_MAX
};

typedef enum _email_add_account_e {
	EMAIL_ADD_ACCOUNT_TYPE_EMAIL = 1,		/**<  CSC_FEATURE_DEF_BOOL_EMAIL_PROVIDERS_DISPLAY_ENABLE 0 */
	EMAIL_ADD_ACCOUNT_TYPE_OTHERS,			/**<  when "Others" is selected */
	EMAIL_ADD_ACCOUNT_TYPE_CSC,				/**<  CSC_FEATURE_DEF_BOOL_EMAIL_PROVIDERS_DISPLAY_ENABLE 1 */
} email_add_account_e;

typedef struct ug_data EmailSettingUGD;

struct ug_data
{
	email_module_t base;

	email_module_h filter;
	app_control_h app_control_google_eas;

	email_module_listener_t filter_listener;

	Evas_Object *popup;
	Evas_Object *more_btn;

	/* for adding account */
	GSList *default_provider_list;
	char *email_sp;
	char *email_sp_icon_path;
	email_add_account_e add_account_type;
	email_account_t *new_account;
	email_account_t *account_list;
	email_protocol_config_t *imap_conf;
	email_protocol_config_t *pop_conf;
	email_protocol_config_t *smtp_conf;

	/* view specified variables */
	int account_count;
	int account_id;
	int add_account;
	int setup_mailbox;
	int is_account_deleted_on_this;
	Eina_Bool is_set_default_account;

	/* for noti mgr */
	GSList *noti_list;
	GDBusConnection *dbus_conn;
	guint network_id;
	guint storage_id;

	/* for cancel op */
	GSList *cancel_op_list;
	Ecore_Thread *query_thread;

	/* for conformant */
	int is_keypad;
	int is_clipboard;
};

#endif				/* __EMAIL_SETTING_H__ */

/* EOF */
