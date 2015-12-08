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

#ifndef __DEF_EMAIL_MAILBOX_H_
#define __DEF_EMAIL_MAILBOX_H_

#include <Elementary.h>
#include <Ecore_IMF.h>
#include <libintl.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gprintf.h>

#include "email-types.h"
#include "email-debug.h"
#include "email-common-types.h"
#include "email-engine.h"
#include "email-utils.h"
#include "email-locale.h"
#include "email-module-dev.h"
#include "email-request-queue.h"

#undef LOG_TAG
#define LOG_TAG "EMAIL_MBOX"

#define _EDJ(o)			elm_layout_edje_get(o)

#define LAST_UPDATED_TIME 512
#define MAIL_ID_SIZE	30
#define MAIL_ID_COUNT	1000

#define EMAIL_MAILBOX_PREVIEW_TEXT_DEFAULT_CHAR_COUNT 75
#define EMAIL_MAILBOX_TITLE_DEFAULT_CHAR_COUNT 55

#define EMAIL_MAILBOX_CHECK_CACHE_SIZE 42

typedef enum {
	EMAIL_MAILBOX_MODE_UNKOWN = -1,
	EMAIL_MAILBOX_MODE_ALL,
	EMAIL_MAILBOX_MODE_MAILBOX,
#ifndef _FEATURE_PRIORITY_SENDER_DISABLE_
	EMAIL_MAILBOX_MODE_PRIORITY_SENDER,
#endif
	EMAIL_MAILBOX_MODE_MAX,
} EmailMailboxMode;

typedef enum {
	SORTBY_UNKNOWN = -1,
	SORTBY_DATE_RECENT,
	SORTBY_DATE_OLD,
	SORTBY_SENDER_ATOZ,
	SORTBY_SENDER_ZTOA,
	SORTBY_UNREAD,
	SORTBY_IMPORTANT,
	SORTBY_ATTACHMENT,
	SORTBY_PRIORITY,
	SORTBY_SUBJECT_ATOZ,
	SORTBY_SUBJECT_ZTOA,
	SORTBY_MAX,
} EmailMailboxSortby;

typedef enum {
	MAILBOX_EDIT_TYPE_NONE = 0,
	MAILBOX_EDIT_TYPE_DELETE = 1,
	MAILBOX_EDIT_TYPE_MOVE = 2,
	MAILBOX_EDIT_TYPE_ADD_SPAM = 3,
	MAILBOX_EDIT_TYPE_REMOVE_SPAM = 4,
	MAILBOX_EDIT_TYPE_MARK_READ = 5,
	MAILBOX_EDIT_TYPE_MARK_UNREAD = 6,
} mailbox_edit_type_e;

typedef struct add_remaining_mail_req_data AddRemainingMailReqData;

/**
 * @brief Email mailbox check cache data
 */
typedef struct {
	const char *obj_style;
	Evas_Object *items[EMAIL_MAILBOX_CHECK_CACHE_SIZE];
	int free_index;
	int used_index;
} EmailMailboxCheckCache;

typedef struct ug_data EmailMailboxUGD;

/**
 * @brief Email mailbox data
 */
struct ug_data {
	email_view_t base;

	email_run_type_e run_type;
	int start_mail_id;
	bool initialized;
	bool started;

	email_module_h viewer;
	email_module_h composer;
	email_module_h account;
	email_module_h setting;

	Eina_Bool b_ready_for_initialize;
	Elm_Theme *theme;

	/* navi title */
	Evas_Object *title_layout;
	Evas_Object *btn_mailbox;
	Evas_Object *cancel_btn;
	Evas_Object *save_btn;

	/* list */
	Elm_Genlist_Item_Class itc;
	Evas_Object *gl;
	Evas_Object *no_content_thread;
	Elm_Object_Item *last_updated_time_item;
	Elm_Object_Item *get_more_progress_item;
	Elm_Object_Item *load_more_messages_item;
	Elm_Object_Item *no_more_emails_item;
	bool no_content_shown;
	bool b_edge_bottom;
	bool b_enable_get_more;

	/*Main layout*/
	Evas_Object *content_layout;

	/*Sellect ALL*/
	Elm_Object_Item *select_all_item;
	Eina_Bool selectAll_chksel;
	Evas_Object *selectAll_check;
	gboolean is_send_all_run;

	/* toolbar */
	Evas_Object *toolbar;
	Evas_Object *controlbar_more_btn;
	Evas_Object *more_ctxpopup;

	/*Compose button*/
	Evas_Object *compose_btn;

	/* search */
	Evas_Object *searchbar_ly;
	Evas_Object *sub_layout_search;
	email_editfield_t search_editfield;
	gchar *current_entry_string;
	Ecore_Timer *search_entry_changed_timer;
	gchar *last_entry_string;
	gboolean b_advanced_search_view;
	gboolean editable_search;
	Ecore_Idler *search_entry_focus_idler;

	/* other popup */
	Evas_Object *error_popup;
	Evas_Object *passwd_popup;
	Evas_Object *outbox_send_all_bar;
	Evas_Object *outbox_send_all_btn;

	bool b_hide_checkbox;
	bool is_module_launching;
	bool b_editmode;
	mailbox_edit_type_e editmode_type;
	bool b_searchmode;
	bool b_restoring;

	Ecore_Thread *add_remaining_thread;
	Ecore_Thread *move_thread;
	Ecore_Thread *delete_thread;

	email_request_queue_h request_queue;

	Eina_List *add_thread_list;
	bool b_format_24hour;

	EmailMailboxMode mode;
	email_sort_type_e sort_type;
	EmailMailboxSortby sort_type_index;

	int move_status;
	/* account */
	email_account_server_t account_type;
	gint account_id;
	gint account_count;
	gchar *account_name;

	/*mailbox */
	gint mailbox_id;
	email_mailbox_type_e mailbox_type;
	gchar *mailbox_alias;
	bool only_local;
	char *last_updated_time;
	int total_mail_count;
	int unread_mail_count;

	int opened_mail_id;

	/* list */
	GList *mail_list;

	Eina_List *selected_mail_list;
	Eina_List *selected_group_list;

	GList *important_list;
	GList *account_color_list;
	GList *g_sending_mail_list;

	/* move */
	gchar *moved_mailbox_name;
	int move_src_mailbox_id;
	bool need_deleted_noti;

	gint sync_needed_mailbox_id;

	Evas_Object *gesture_ly;
	double vip_rule_value;

	AddRemainingMailReqData *remaining_req;

	EmailMailboxCheckCache star_check_cache;
	EmailMailboxCheckCache select_check_cache;
};

/**
 * @brief Email mailbox module data
 */
typedef struct {
	email_module_t base;
	EmailMailboxUGD view;

	Ecore_Thread *start_thread;
	volatile bool start_thread_done;

	Evas_Object *init_pupup;

} EmailMailboxModule;

/**
 * @brief Email group item data
 */
typedef struct {
	EmailMailboxUGD *mailbox_ugd;

	gchar *group_title;
	int mail_count;
	Eina_Bool group_chksel;
	Evas_Object *group_check;
	Elm_Object_Item *group_item;
	Eina_Bool hidden;
} GroupItemData;

/**
 * @brief Email mailbox mail item data
 */
typedef struct {
	EmailMailboxUGD *mailbox_ugd;

	gchar *alias;
	gchar *sender;
	gchar *preview_body;
	char *title;
	char *recipient;
	gchar *timeordate;
	gchar *fastscroll_date;
	gchar *group_title;
	gint mailbox_id;
	int mailbox_type;
	int mail_size;
	gboolean is_attachment;
	gboolean is_seen;
	email_mail_status_t mail_status;
	gboolean is_body_download;
	gint mail_id;
	gint account_id;
	gint thread_id;
	gint thread_count;
	Eina_Bool checked;
	gint flag_type;
	gint priority;
	gint reply_flag;
	gint forward_flag;
	gint message_class;
	gboolean is_to_address_mail;
	gboolean is_priority_sender_mail;
	time_t absolute_time;

	Evas_Coord mouse_down_point_x;
	Evas_Coord mouse_down_point_y;

	Evas_Object *check_btn;
	Evas_Object *flag_btn;
	Evas_Object *check_favorite_btn;
	Eina_Bool is_highlited;

	Elm_Object_Item *item;
} MailItemData;

/**
 * @brief Email outgoing list data
 */
typedef struct {
	GList *send_all_list;
	gboolean *send_all_runned;
} MailOutgoingListData;

/**
 * @brief Email outgoing item data
 */
typedef struct {
	gint mail_id;
	gint account_id;
} MailOutgoingItem;

/**
 * @brief Request from email service update notification status for all accounts or to specified account ID
 * @param[in]	mailbox_ugd		Email mailbox data
 */
void mailbox_update_notifications_status(EmailMailboxUGD *mailbox_ugd);

/**
 * @brief Create no content view with search mode or not.
 * @param[in]	mailbox_ugd		Email mailbox data
 * @param[in]	search_mode		If TRUE "search" layout will be used, if FALSE view will be hidden with "default" layout
 */
void mailbox_create_no_contents_view(EmailMailboxUGD *mailbox_ugd, bool search_mode);

/**
 * @brief Function provides show appearance to the no content view
 * @param[in]	mailbox_ugd		Email mailbox data
 */
void mailbox_show_no_contents_view(EmailMailboxUGD *mailbox_ugd);

/**
 * @brief Function hides no content view
 * @param[in]	mailbox_ugd		Email mailbox data
 */
void mailbox_hide_no_contents_view(EmailMailboxUGD *mailbox_ugd);

/**
 * @brief Provides refresh for Mailbox including title or not
 * @param[in]	mailbox_ugd		Email mailbox data
 * @param[in]	update_title	If TRUE "title" will be updated, otherwise not
 */
void mailbox_refresh_fullview(EmailMailboxUGD *mailbox_ugd, bool update_title);

/**
 * @brief Provides removing the Mailbox button from naviframe
 * @param[in]	mailbox_ugd		Email mailbox data
 */
void mailbox_naviframe_mailbox_button_remove(EmailMailboxUGD *mailbox_ugd);

/**
 * @brief Provides adding the Mailbox button to naviframe
 * @param[in]	mailbox_ugd		Email mailbox data
 */
void mailbox_naviframe_mailbox_button_add(EmailMailboxUGD *mailbox_ugd);

/**
 * @brief Handles message from Viewer to open the next mail
 * @param[in]	mailbox_ugd		Email mailbox data
 * @return 0 on success, otherwise a negative error value
 */
int mailbox_handle_next_msg_bundle(EmailMailboxUGD *mailbox_ugd, app_control_h msg);

/**
 * @brief Handles message from Viewer to open the previous mail
 * @param[in]	mailbox_ugd		Email mailbox data
 * @return 0 on success, otherwise a negative error value
 */
int mailbox_handle_prev_msg_bundle(EmailMailboxUGD *mailbox_ugd, app_control_h msg);

/**
 * @brief Provides functionality to open the Email viewer
 * @param[in]	mailbox_ugd		Email mailbox data
 * @param[in]	account_id		Account ID
 * @param[in]	mailbox_id		Mailbox ID
 * @param[in]	mail_id			Mail ID
 * @return 0 on success, otherwise a negative error value
 */
int mailbox_open_email_viewer(EmailMailboxUGD *mailbox_ugd, int account_id, int mailbox_id, int mail_id);

#endif				/* __DEF_EMAIL_MAILBOX_H_ */
