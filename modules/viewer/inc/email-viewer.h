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

#ifndef __EMAIL_VIEWER_H__
#define __EMAIL_VIEWER_H__

#include <Elementary.h>
#include <libintl.h>
#include <app.h>
#include <Evas.h>

#include <Edje.h>
#include <Eina.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

#ifdef _TIZEN_2_4_BUILD_
#include <EWebKit.h>
#else
#include <ewk_chromium.h>
#endif

#include "email-utils.h"
#include "email-utils-contacts.h"
#include "email-locale.h"
#include "email-viewer-logic.h"
#include "email-viewer-types.h"
#include "email-module-dev.h"

#undef LOG_TAG
#define LOG_TAG "EMAIL_VIEWER"

#define DEBUG_SCROLLER
#ifdef DEBUG_SCROLLER
#define debug_enter_scroller()
#define debug_leave_scroller()
#define debug_log_scroller(fmt, args...)
#else
#define debug_enter_scroller()					do { debug_trace(" * Enter *"); } while (0)
#define debug_leave_scroller()					do { debug_trace(" * Leave *"); } while (0)
#define debug_log_scroller(fmt, args...)			do { LOGI(fmt, ##args); } while (0)
#endif

#ifdef _TIZEN_2_4_BUILD_
/* Suppress "implicit declaration of function" warning */
char *ewk_hit_test_link_uri_get(void *hit_test);
char *ewk_hit_test_link_title_get(void *hit_test);
char *ewk_hit_test_link_label_get(void *hit_test);
char *ewk_hit_test_image_uri_get(void *hit_test);
char *ewk_hit_test_media_uri_get(void *hit_test);
char *ewk_hit_test_tag_name_get(void *hit_test);
char *ewk_hit_test_node_value_get(void *hit_test);
char *ewk_hit_test_image_buffer_get(void *hit_test);
unsigned int ewk_hit_test_image_buffer_length_get(void *hit_test);
char *ewk_hit_test_image_file_name_extension_get(void *hit_test);
void ewk_hit_test_free(void *hit_test);
const char *ewk_view_text_selection_text_get(Evas_Object *view);

void ewk_view_vertical_panning_hold_set(Evas_Object *view, Eina_Bool value);
void ewk_view_scroll_size_get(Evas_Object *view, Evas_Coord *w, Evas_Coord *h);
void ewk_view_orientation_send(Evas_Object *, int);
Eina_Bool ewk_view_text_selection_clear(Evas_Object *);
Eina_Bool ewk_view_visibility_set(Evas_Object *, Eina_Bool);

void ewk_settings_webkit_text_size_adjust_enabled_set(Ewk_Settings *settings, Eina_Bool value);
void ewk_settings_extra_feature_set(Ewk_Settings *, const char *, Eina_Bool);

void ewk_context_did_start_download_callback_set(Ewk_Context *, void (*)(const char *, void *), void *);
void ewk_context_cache_clear(Ewk_Context *);
void ewk_context_notify_low_memory(Ewk_Context *);
#endif

typedef struct _ev_attachment_data_tag EV_attachment_data;

typedef struct _view_data EmailViewerView;

typedef struct _fwd_option_popup_data FwdOptionPopupData;
/**
 * @brief Email viewer data
 */

struct _view_data {
	email_view_t base;

	/* base */
	Evas_Object *more_btn;
	Evas_Object *scroller;
	Evas_Object *base_ly;
	Evas_Object *main_bx;

	/* module */
	email_module_h loaded_module;
	int create_contact_arg;
	bool is_composer_module_launched;

	/* util */
	int main_w;
	int main_h;
	VIEWER_ERROR_TYPE_E eViewerErrorType;
	VIEWER_TYPE_E viewer_type;

	/*combined scroller*/
	Evas_Object *combined_scroller;
	int webkit_scroll_y;
	int webkit_scroll_x;
	int webkit_old_x;
	int header_height;
	int trailer_height;
	int total_height;
	int total_width;
	int main_scroll_y;
	int main_scroll_h;
	int main_scroll_w;

	/*dbus*/
	GDBusConnection *viewer_dbus_conn;
	guint viewer_network_id;
	guint viewer_storage_id;

	/*recipient layout*/
	Evas_Object *to_mbe;
	Evas_Object *to_ly;
	Evas_Object *cc_mbe;
	Evas_Object *cc_ly;
	Evas_Object *bcc_mbe;
	Evas_Object *bcc_ly;
	int to_recipients_cnt;
	int cc_recipients_cnt;
	int bcc_recipients_cnt;
	Ecore_Idler *to_recipient_idler;
	Ecore_Idler *cc_recipient_idler;
	Ecore_Idler *bcc_recipient_idler;
	char *recipient_address;
	char *recipient_name;
	char *selected_address;
	char *selected_name;
	email_contact_list_info_t *recipient_contact_list_item;

	/* arguments */
	int account_id;
	int account_type;
	int mail_id;
	int mailbox_id;
	char *account_email_address;
	char *eml_file_path;
	char *temp_folder_path;		/* freed when the folder is removed */
	char *temp_viewer_html_file_path;	/* freed when the temp file is removed */
	/* for preview */
	char *temp_preview_folder_path;		/* freed when the folder is removed */

	/* flags */
	int b_internal;
	int b_viewer_hided;
	int b_partial_body;
	int scroller_locked;
	Eina_Bool b_load_finished;
	Eina_Bool b_show_remote_images;
	Eina_Bool is_long_pressed;
	Eina_Bool is_main_scroller_scrolling;
	Eina_Bool is_recipient_ly_shown;
	Eina_Bool is_download_message_btn_clicked;
	Eina_Bool is_cancel_sending_btn_clicked;
	Eina_Bool need_pending_destroy;
	Eina_Bool is_storage_full_popup_shown;
	bool can_destroy_on_msg_delete;
	Eina_Bool is_webview_text_selected;

	/* Webview resize */
	float scale_factor;
	int webview_width;
	int webview_height;

	/* rotation */
	int isLandscape;

	/* mailbox list */
	email_mailbox_type_e mailbox_type;
	GList *folder_list;
	email_mailbox_t *move_mailbox_list;
	int move_mailbox_count;
	int move_status;

	/*Header Evas Object */
	Evas_Object *header_ly;
	Evas_Object *subject_entry;
	Evas_Object *favourite_btn;
	Evas_Object *attachment_ly;
	Evas_Object *header_divider;
	Evas_Object *details_button;

	/*Reply toolbar*/
	Evas_Object *reply_toolbar_ly;
	Evas_Object *reply_option_popup;
	Evas_Object *fwd_option_popup;
	FwdOptionPopupData *fwd_popup_data;
	ViewerReplyType selected_reply_type;


	Evas_Object *webview_ly;
	Evas_Object *webview_button;
	Evas_Object *webview;

	/*attachment view*/
	email_view_t attachment_view;
	pthread_t attachment_save_thread;
	pthread_mutex_t attachment_save_mutex;
	pthread_cond_t attachment_save_cond;
	Ecore_Job *attachment_update_job;
	Ecore_Timer *attachment_update_timer;
	Eina_List *attachment_data_list;
	Eina_List *attachment_data_list_to_save;
	EV_attachment_data *preview_aid;
	bool show_cancel_all_btn;
	Evas_Object *attachment_genlist;
	Evas_Object *save_all_btn;
	Elm_Object_Item *attachment_group_item;
	Elm_Genlist_Item_Class *attachment_itc;
	Elm_Genlist_Item_Class *attachment_group_itc;

	/* download button and popup */
	Evas_Object *list_progressbar;
	Evas_Object *con_popup;
	Evas_Object *notify;
	Evas_Object *pb_notify;
	Evas_Object *pb_notify_lb;
	Evas_Object *dn_btn;

	/* more context popup */
	Elm_Object_Item *cancel_sending_ctx_item;

	/*email data*/
	char *file_id;
	int file_size;
	int email_handle;
	email_mail_data_t *mail_info;
	email_attachment_data_t *attachment_info;
	int attachment_count;
	int mail_type;
	time_t mktime;
	email_mail_priority_t priority;
	email_mail_status_t save_status;
	email_body_download_status_t body_download_status;
	EmailViewerWebview *webview_data;
	char *sender_address;
	char *sender_display_name;
	char *subject;
	char *charset;
	char *body;
	char *body_uri;
	GList *attachment_info_list;
	int total_att_count;
	int normal_att_count;
	gint64 total_att_size;
	gboolean has_html;
	gboolean has_attachment;
	gboolean has_inline_image;
	gboolean favorite;
	gboolean request_report;
	bool is_smil_mail;
	email_account_t *account;

	/* idler */
	Ecore_Idler *idler_regionbringin;

	/* timer */
	Ecore_Timer *rcpt_scroll_corr;
	Ecore_Timer *launch_timer;

	/*popup translation*/
	int popup_element_type;
	char *translation_string_id1;
	char *translation_string_id2;
	char *str_format1;
	char *str_format2;
	int package_type1;
	int package_type2;
	int extra_variable_type;
	int extra_variable_integer;
	char *extra_variable_string;

	/* popup */
	double loading_progress;
	Evas_Object *passwd_popup;

	/* theme */
	Elm_Theme *theme;
};

/**
 * @brief Email viewer module data
 */
typedef struct {
	email_module_t base;
	EmailViewerView view;
} EmailViewerModule;

/**
 * @brief Email viewer attachment state enum
 */
typedef enum {
	EV_ATT_STATE_IDLE,
	EV_ATT_STATE_PENDING,
	EV_ATT_STATE_IN_PROGRESS
} EV_attachment_state;

/**
 * @brief Email viewer all attachment state enum
 */
typedef enum {
	EV_ALL_ATT_STATE_IDLE,
	EV_ALL_ATT_STATE_MIXED,
	EV_ALL_ATT_STATE_BUSY,
	EV_ALL_ATT_STATE_INVALID
} EV_all_attachment_state;

/**
 * @brief Email viewer attachment tag data
 */
struct _ev_attachment_data_tag {

	/* Main fields */
	EmailViewerView *view;
	Elm_Object_Item *it;
	EmailAttachmentType *attachment_info;

	/* Logic fields */
	EV_attachment_state state;
	int download_handle;
	char *preview_path;
	email_ext_save_err_type_e save_result;
	bool is_saving;
	bool save_for_preview;
	bool saving_was_started;
	bool saving_was_finished;
	bool saving_was_canceled;
	bool is_busy;

	/* Item appearance */
	int progress_val;	/* 1/100 % */
	bool need_update;
	bool is_saved;
	bool is_progress_packed;

	/* GUI objects */
	Evas_Object *content_box;
	Evas_Object *progressbar;
	Evas_Object *download_button;
	Evas_Object *filename_label;
	Evas_Object *progress_info_box;
	Evas_Object *progress_label_right;
	Evas_Object *progress_label_left;
};

enum {
	POPUP_ELEMENT_TYPE_TITLE = 1,
	POPUP_ELEMENT_TYPE_CONTENT,
	POPUP_ELEMENT_TYPE_BTN1,
	POPUP_ELEMENT_TYPE_BTN2,
	POPUP_ELEMENT_TYPE_BTN3,
};

struct _fwd_option_popup_data {
	Evas_Object *radio_group;
	Eina_Bool is_add_attach_checked;
	Evas_Object *add_attach_checkbox;
	ViewerFwdType selectedType;
};

enum {
	PACKAGE_TYPE_NOT_AVAILABLE = 0,
	PACKAGE_TYPE_PACKAGE,
	PACKAGE_TYPE_SYS_STRING,
};

enum {
	VARIABLE_TYPE_NONE = 0,
	VARIABLE_TYPE_INT,
	VARIABLE_TYPE_STRING,
};

/**
 * @brief Reset viewer view
 *
 * @param[in]	view			Email viewer data
 * @param[in]	update_body		Flag specifies that need to update header body
 *
 */
void _reset_view(EmailViewerView *view, Eina_Bool update_body);

/**
 * @brief Hide viewer view
 *
 * @param[in]	view			Email viewer data
 *
 */
void _hide_view(EmailViewerView *view);

/**
 * @brief Callback to delete
 *
 * @param[in]	data			User data (Email viewer data)
 * @param[in]	obj				Unused
 * @param[in]	event_info		Unused
 *
 */
void _delete_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED);

/**
 * @brief Callback to move message
 *
 * @param[in]	data			User data (Email viewer data)
 * @param[in]	obj				Unused
 * @param[in]	event_info		Unused
 *
 */
void _move_cb(void *data, Evas_Object *obj, void *event_info);

/**
 * @brief Provides webviewe body creation
 *
 * @param[in]	view			Email viewer data
 *
 */
void _create_body(EmailViewerView *view);

/**
 * @brief Provides download webviewe body
 *
 * @param[in]	data			User data (Email viewer data)
 *
 */
void _download_body(void *data);

/**
 * @brief Check webviewe body download
 *
 * @param[in]	data			User data (Email viewer data)
 *
 * @return TRUE on success or FALSE if none or an error occurred
 */
Eina_Bool viewer_check_body_download(void *data);

/**
 * @brief Creates body progress and list process animation when body downloading
 *
 * @param[in]	view			Email viewer data
 *
 */
void create_body_progress(EmailViewerView *view);

/**
 * @brief Provides resizing for webview
 *
 * @param[in]	view			Email viewer data
 * @param[in]	height			Heigh value to be setted
 *
 */
void viewer_resize_webview(EmailViewerView *view, int height);

/**
 * @brief Initialize viewer's private data
 *
 * @param[in]	view			Email viewer data
 *
 * @return TRUE on success or FALSE if none or an error occurred
 *
 */
Eina_Bool viewer_initialize_data(EmailViewerView *view);

/**
 * @brief Delete all viewer internal timers and idlers
 *
 * @param[in]	view			Email viewer data
 *
 */
void viewer_stop_ecore_running_apis(EmailViewerView *view);

/**
 * @brief Reset all data belongs to mail
 *
 * @param[in]	view			Email viewer data
 *
 * @return Code from VIEWER_ERROR_TYPE_E enum about occurred error
 */
int viewer_reset_mail_data(EmailViewerView *view);

/**
 * @brief Deletes viewer's private data
 *
 * @param[in]	view			Email viewer data
 * @param[in]	isHide			Evas_Bool value
 *
 */
void viewer_delete_evas_objects(EmailViewerView *view, Eina_Bool isHide);

/**
 * @brief Unregister viewer from callbacks
 *
 * @param[in]	view			Email viewer data
 *
 */
void viewer_remove_callback(EmailViewerView *view);

/**
 * @brief Creates account data
 *
 * @param[in]	view			Email viewer data
 *
 */
void viewer_create_account_data(EmailViewerView *view);

/**
 * @brief Deletes account data
 *
 * @param[in]	view			Email viewer data
 *
 */
void viewer_delete_account_data(EmailViewerView *view);

/**
 * @brief Back key press handler
 *
 * @param[in]	view			Email viewer data
 *
 */
void viewer_back_key_press_handler(EmailViewerView *view);

#endif	/* __EMAIL_VIEWER_H__ */
