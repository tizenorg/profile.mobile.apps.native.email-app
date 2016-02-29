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

#ifndef __EMAIL_ACCOUNT_H__
#define __EMAIL_ACCOUNT_H__

#undef LOG_TAG
#define LOG_TAG "EMAIL_ACCOUNT"

#include <Edje.h>
#include <Elementary.h>
#include <libintl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <glib.h>
#include <glib-object.h>
#include <Ecore_IMF.h>
#include <glib/gprintf.h>
#include <gio/gio.h>
#include <notification.h>

#include "email-api.h"
#include "email-types.h"

#include "email-utils.h"
#include "email-engine.h"
#include "email-debug.h"
#include "email-common-types.h"
#include "email-locale.h"
#include "email-module-dev.h"

#define ACCOUNT_PACKAGE "email-account"

#define MAX_FOLDER_NAME_LEN	1024
#define MAX_FOLDER_NAME_BUF_LEN	4096
#define NUM_STR_LEN	20

#define EMAIL_HANDLE_INVALID 0
#define SEARCH_TAG_LEN		50

typedef enum {
	ACC_FOLDER_VIEW_MODE_UNKNOWN = -1,
	ACC_FOLDER_COMBINED_VIEW_MODE,
	ACC_FOLDER_COMBINED_SINGLE_VIEW_MODE,
	ACC_FOLDER_SINGLE_VIEW_MODE,
	ACC_FOLDER_MOVE_MAIL_VIEW_MODE,
	ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE,
	ACC_FOLDER_VIEW_MODE_MAX,
} account_folder_view_mode;

typedef enum
{
	ACC_MAILBOX_TYPE_ALL_ACCOUNT = -2,
	ACC_MAILBOX_TYPE_SINGLE_ACCOUNT = -1,
	ACC_MAILBOX_TYPE_INBOX = 0,			/**< Specified inbox type*/
	ACC_MAILBOX_TYPE_PRIORITY_INBOX,		/**< Specified trash type*/
	ACC_MAILBOX_TYPE_FLAGGED,			/**< Specified flagged mailbox type on gmail */
	ACC_MAILBOX_TYPE_DRAFT,				/**< Specified draft box type*/
	ACC_MAILBOX_TYPE_OUTBOX,			/**< Specified outbox type*/
	ACC_MAILBOX_TYPE_SENTBOX,			/**< Specified sent box type*/
	ACC_MAILBOX_TYPE_SPAMBOX,			/**< Specified spam box type*/
	ACC_MAILBOX_TYPE_TRASH,				/**< Specified trash type*/
	ACC_MAILBOX_TYPE_MAX,				/**< Specified all emails type of gmail*/
}account_mailbox_type_e;

typedef enum
{
	ACC_FOLDER_VIEW_INBOX = 0,			/**< Specified inbox type*/
	ACC_FOLDER_VIEW_DRAFT,				/**< Specified draft box type*/
	ACC_FOLDER_VIEW_OUTBOX,				/**< Specified outbox type*/
	ACC_FOLDER_VIEW_SENTBOX,			/**< Specified sent box type*/
	ACC_FOLDER_VIEW_SPAMBOX,			/**< Specified spam box type*/
	ACC_FOLDER_VIEW_TRASH,				/**< Specified trash type*/
	ACC_FOLDER_VIEW_MAX,
}account_folder_view_type; //folder position is decided by this value.

typedef enum
{
	ACC_FOLDER_ACTION_NONE		= 0,
	ACC_FOLDER_ACTION_CREATE,
	ACC_FOLDER_ACTION_DELETE,
	ACC_FOLDER_ACTION_RENAME,
}folder_action_type;

typedef enum
{
	ACC_FOLDER_INVALID		= -1,
	ACC_FOLDER_NONE,
	ACC_FOLDER_CREATE,
	ACC_FOLDER_DELETE,
	ACC_FOLDER_RENAME,
}folder_view_type_e;

typedef enum
{
	EMAIL_MAILBOX_TYPE_PRIORITY_SENDERS = EMAIL_MAILBOX_TYPE_USER_DEFINED + 1
}email_mailbox_type_e_ext;

typedef struct _view_data EmailAccountView;

typedef struct _Item_Data Item_Data;
struct _Item_Data {
	EmailAccountView *view;
	Elm_Object_Item *it; // Genlist Item pointer
	int i_boxtype; //for all account view
	int b_scheduled_outbox; //If scheduled outbox, assign account id.

	char *mailbox_name;
	int mailbox_type;
	char *alias_name;
	int unread_count;
	int total_mail_count_on_local;
	int mailbox_id;
	email_mailbox_t *mailbox_list;
	char *account_name;
};

typedef struct _email_move_list {
	int move_view_mode;
	int mailbox_cnt;
	email_account_t *account_info;
	email_mailbox_t *mailbox_list;
} email_move_list;

struct _view_data
{
	email_view_t base;

	Evas_Object *gl;

	Evas_Object *more_btn;
	Evas_Object *more_ctxpopup;
	bool editmode;
	email_module_h setting;

	Evas_Object *entry;
	Evas_Object *popup;
	Evas_Object *popup_ok_btn;

	Elm_Object_Item *group_title_item;
	Elm_Object_Item *it;
	Elm_Object_Item *move_from_item;

	Elm_Genlist_Item_Class *itc_combined;
	Elm_Genlist_Item_Class *itc_account_name;
	Elm_Genlist_Item_Class *itc_single;
	Elm_Genlist_Item_Class *itc_group;

	Elm_Genlist_Item_Class *itc_move;
	Elm_Genlist_Item_Class *itc_group_move;

	Elm_Genlist_Item_Class *itc_account_group;
	Elm_Genlist_Item_Class *itc_account_item;

	char *original_folder_name;
	char *user_email_address;

	gchar *moved_mailbox_name; //for notification popup
	int account_count;
	int account_id;
	int folder_id;
	int mailbox_type;
	int folder_mode;
	int folder_view_mode;
	int emf_handle;
	int target_mailbox_id;
	int need_refresh;
	bool selection_disabled;

	/*move mail mode*/
	GList *selected_mail_list_to_move;			/* revised, used to move the mails selected in previous view */
	email_move_list *move_list;
	int move_src_mailbox_id;
	int move_mode;
	int b_editmode;
	int move_status;
	int inserted_account_cnt;

	/*account list mode*/
	GList *account_group_list;
	gint default_account_id;	//Display default account(Drawer view)
	int account_group_state;
	GList *account_color_list;
	gint mailbox_id;
	account_mailbox_type_e mailbox_mode;

	int all_acc_total_count[ACC_MAILBOX_TYPE_MAX];
	int all_acc_unread_count[ACC_MAILBOX_TYPE_MAX];
	int block_item_click;
	int main_w;
	int main_h;
	bool isRotate;
	int is_keypad;

	email_account_t *account_list;

	GDBusConnection *dbus_conn;
	guint signal_handler_network;
	guint signal_handler_storage;
};

typedef struct {
	email_module_t base;
	EmailAccountView view;
} EmailAccountModule;

#define GET_ACCOUNT_SERVER_TYPE(account_id) \
		({\
			email_account_t *email_account = NULL;\
			int server_type = 0;\
			int e = email_get_account(account_id, EMAIL_ACC_GET_OPT_DEFAULT, &email_account);\
			if (e != EMAIL_ERROR_NONE || !email_account) {\
				debug_warning("email_get_account acct(%d) - err(%d) or acct NULL(%p)",\
								account_id, e, email_account);\
			}\
			else server_type = email_account->incoming_server_type;\
			if(email_account) email_free_account(&email_account, 1);\
			server_type;\
		})


int account_create_list(EmailAccountView *view);
Evas_Object *account_add_empty_list(EmailAccountView *view);
void account_destroy_view(EmailAccountView *view);
void account_gdbus_event_account_receive(GDBusConnection *connection,
											const gchar *sender_name,
											const gchar *object_path,
											const gchar *interface_name,
											const gchar *signal_name,
											GVariant *parameters,
											gpointer data);

void account_show_all_folder(EmailAccountView *view);

#endif	/* __EMAIL_ACCOUNT_H__ */


