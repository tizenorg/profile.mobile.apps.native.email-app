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

#ifndef __EMAIL_COMPOSER_H__
#define __EMAIL_COMPOSER_H__

#include <gio/gio.h>

#ifdef _TIZEN_2_4_BUILD_
#include <EWebKit.h>
#else
#include <ewk_chromium.h>
#endif

#include "email-locale.h"
#include "email-utils.h"
#include "email-common-types.h"
#include "email-engine.h"
#include "email-composer-types.h"

#undef LOG_TAG
#define LOG_TAG "EMAIL_COMPOSER"

#ifdef _TIZEN_2_4_BUILD_
/**
 * @brief Suppress "implicit declaration of function" warning
 */
typedef struct Ewk_Console_Message_Tag Ewk_Console_Message;

/**
 * @brief Forward declaration
 */
Eina_Bool ewk_view_command_execute(Evas_Object *, char *, char *);
void ewk_view_vertical_panning_hold_set(Evas_Object *, Eina_Bool);
void ewk_view_scroll_size_get(Evas_Object *, int *, int *);
void ewk_view_split_scroll_overflow_enabled_set(Evas_Object *, Eina_Bool);
void ewk_settings_select_word_automatically_set(Ewk_Settings *, Eina_Bool);
void ewk_settings_clear_text_selection_automatically_set(Ewk_Settings *, Eina_Bool);
Eina_Bool ewk_view_plain_text_get(Evas_Object *, void *, void *);
void ewk_settings_uses_keypad_without_user_action_set(Ewk_Settings *, Eina_Bool);
void ewk_view_draws_transparent_background_set(Evas_Object *, Eina_Bool);
void ewk_settings_extra_feature_set(Ewk_Settings *, const char *, Eina_Bool);
void ewk_context_cache_clear(Ewk_Context *);
void ewk_context_notify_low_memory(Ewk_Context *);
Eina_Bool ewk_view_text_selection_clear(Evas_Object *);

const char *ewk_console_message_text_get(Ewk_Console_Message *msg);
unsigned ewk_console_message_line_get(Ewk_Console_Message *msg);

#endif

typedef struct _view_data EmailComposerView;

/**
 * @brief Email composer data
 */
struct _view_data {
	email_view_t base;

	GDBusConnection *dbus_conn;
	guint dbus_network_id;

	/* Idlers */
	Ecore_Idler *idler_set_focus;
	Ecore_Idler *idler_show_ctx_popup;
	Ecore_Idler *idler_destroy_self;
	Ecore_Idler *idler_regionshow;
	Ecore_Idler *idler_regionbringin;
	Ecore_Idler *idler_move_recipient;

	/* Timers */
	Ecore_Timer *timer_ctxpopup_show;
	Ecore_Timer *timer_adding_account;
	Ecore_Timer *timer_lazy_loading;
	Ecore_Timer *timer_regionshow;
	Ecore_Timer *timer_regionbringin;
	Ecore_Timer *timer_resize_webkit;
	Ecore_Timer *timer_destroy_composer_popup;
	Ecore_Timer *timer_destory_composer;

	/* Related to base layout */
	Evas_Object *main_scroller;
	Evas_Object *composer_layout;
	Evas_Object *composer_box;
	Evas_Object *composer_popup;
	Evas_Object *context_popup;

	Evas_Object *selected_widget;
	Evas_Object *send_btn;

	/* Related to from field */
	Evas_Object *recp_from_layout;
	Evas_Object *recp_from_btn;
	Evas_Object *recp_from_label;
	Evas_Object *recp_from_ctxpopup;

	Eina_List *recp_from_item_list;

	/* Related to to field */
	Evas_Object *recp_to_layout;

	Evas_Object *recp_to_mbe_layout;
	Evas_Object *recp_to_mbe;

	Evas_Object *recp_to_box;
	Evas_Object *recp_to_label;
	email_editfield_t recp_to_entry;
	Evas_Object *recp_to_entry_layout;
	Evas_Object *recp_to_btn;
	email_editfield_t recp_to_display_entry;
	Evas_Object *recp_to_display_entry_layout;

	/* Related to cc field */
	Evas_Object *recp_cc_layout;

	Evas_Object *recp_cc_mbe_layout;
	Evas_Object *recp_cc_mbe;

	Evas_Object *recp_cc_box;
	Evas_Object *recp_cc_label_cc;
	Evas_Object *recp_cc_label_cc_bcc;
	email_editfield_t recp_cc_entry;
	Evas_Object *recp_cc_entry_layout;
	Evas_Object *recp_cc_btn;
	email_editfield_t recp_cc_display_entry;
	Evas_Object *recp_cc_display_entry_layout;

	/* Related to bcc field */
	Evas_Object *recp_bcc_layout;

	Evas_Object *recp_bcc_mbe_layout;
	Evas_Object *recp_bcc_mbe;

	Evas_Object *recp_bcc_box;
	Evas_Object *recp_bcc_label;
	email_editfield_t recp_bcc_entry;
	Evas_Object *recp_bcc_entry_layout;
	Evas_Object *recp_bcc_btn;
	email_editfield_t recp_bcc_display_entry;
	Evas_Object *recp_bcc_display_entry_layout;

	/* To handle the behaviour of recipient entry */
	Elm_Object_Item *selected_mbe_item;

	int to_recipients_cnt;
	int cc_recipients_cnt;
	int bcc_recipients_cnt;

	Eina_Bool cc_added;
	Eina_Bool bcc_added;
	Eina_Bool recipient_added_from_contacts;

	Eina_Bool is_commit_pressed;
	Eina_Bool is_next_pressed;
	Eina_Bool is_backspace_pressed;
	Eina_Bool is_real_empty_entry;

	Eina_Bool is_last_item_selected;

	Eina_Bool allow_click_events;

	/* For avoiding duplicate toast messages */
	int prev_toast_error;
	Ecore_Timer *timer_duplicate_toast_in_reciepients;

	/* Related to subject field */
	Evas_Object *subject_layout;
	email_editfield_t subject_entry;

	/*Add attachment button in subject field*/
	Evas_Object *attachment_btn;

	/* Related to webview */
	Evas_Object *ewk_view_layout;
	Evas_Object *ewk_view;
	Evas_Object *ewk_btn;
	Ecore_Timer *ewk_focus_set_timer;
	Ecore_Timer *ewk_entry_sip_show_timer;
	Eina_Bool is_ewk_focused;

	double webview_load_progress;
	Eina_Bool with_original_message;
	Eina_Bool is_checkbox_clicked;
	Eina_Bool is_focus_on_new_message_div;
	Eina_Bool need_to_set_focus_with_js;
	Eina_Bool is_low_memory_handled;

	/* Related to attachment */
	Evas_Object *attachment_group_layout;
	Evas_Object *attachment_group_icon;

	Eina_List *attachment_item_list;
	Eina_List *attachment_inline_item_list;
	Eina_List *attachment_list_to_be_processed;

	Eina_Bool is_attachment_list_expanded;

	/* For resizing images */
	Ecore_Thread *thread_resize_image;
	Eina_Bool resize_thread_cancel;
	Eina_List *resize_image_list;
	int resize_image_quality;

	/* For downloading attachment */
	int handle_for_downloading_attachment;
	int downloading_attachment_index;
	email_attachment_data_t *downloading_attachment;

	/* Related to predictive search */
	char ps_keyword[EMAIL_LIMIT_EMAIL_ADDRESS_LENGTH + 1];
	Eina_Bool ps_is_runnig;
	Eina_List *ps_contacts_list;

	Evas_Object *ps_layout;
	Evas_Object *ps_genlist;
	int ps_genlist_min_height;

	Ecore_Event_Handler *ps_mouse_down_handler;

	/* Related to combined scroller */
	Evas_Object *combined_scroller;

	bool cs_ready;

	bool cs_bringin_to_ewk;
	bool cs_notify_caret_pos;

	bool cs_in_selection_mode;
	bool cs_has_selection;
	bool cs_has_magnifier;

	int cs_top;
	int cs_width;
	int cs_height;
	int cs_header_height;
	int cs_rttb_height;
	int cs_edge_scroll_pos;
	int cs_max_ewk_scroll_pos;
	int cs_max_scroll_pos;
	int cs_scroll_pos;
	int cs_main_scroll_pos;

	int cs_backup_scroll_pos;
	double cs_backup_pos_time;

	int cs_freeze_count;
	int cs_drag_down_y;
	int cs_drag_start_y;
	int cs_drag_cur_y;
	unsigned int cs_drag_cur_time;
	int cs_drag_content_y;
	bool cs_is_sliding;
	bool cs_is_dragging;

	float cs_anim_pos0;
	float cs_anim_pos1;
	float cs_anim_pos2;
	float cs_anim_v0;
	float cs_anim_v1;
	float cs_anim_a1;
	float cs_anim_a2;
	float cs_anim_t1;
	float cs_anim_duration;
	float cs_anim_t;
	Ecore_Animator *cs_animator;

	int cs_immediate_event_mask;
	int cs_pending_event_mask;
	Ecore_Timer *cs_events_timer1;
	Ecore_Timer *cs_events_timer2;

	/*
	 * Related to composer data
	 * Launch params handle for composer
	 */
	email_params_h launch_params;

	/* Previous setting values. These should be restored when exiting out of composer. */
	double scroll_friction; // friction value(scrolling speed) for scroller.
	Elm_Win_Indicator_Mode indicator_mode;
	Elm_Win_Indicator_Opacity_Mode opacity_mode;

	/* Themes */
	Elm_Theme *theme;

	/* Account list */
	EmailComposerAccount *account_info;

	/* Original & New mail info */
	EmailComposerMail *new_mail_info;
	EmailComposerMail *org_mail_info;

	/* To launch composer fast with loading popup */
	Eina_Bool need_lazy_loading;
	Eina_Bool is_loading_popup;

	COMPOSER_ERROR_TYPE_E eComposerErrorType;
	int composer_type;
	int draft_sync_handle;
	int original_mail_id;
	int is_original_attach_included;
	char *original_charset;
	char *saved_text_path;
	char *saved_html_path;
	char *eml_file_path;

	/* Related to initial contents to identify modification */
	Eina_List *initial_contents_to_list;
	Eina_List *initial_contents_cc_list;
	Eina_List *initial_contents_bcc_list;
	Eina_List *initial_contents_attachment_list;
	char *initial_contents_subject;
	char *initial_body_content;
	char *initial_parent_content;
	char *initial_new_message_content;

	/* Related to sending/saving mail */
	Ecore_Thread *thread_saving_email;
	Eina_Bool need_download;
	char *plain_content;
	char *final_body_content;
	char *final_parent_content;
	char *final_new_message_content;

	/* For priority option */
	int priority_option;

	/* Miscellaneous */
	Eina_Bool is_horizontal;
	Eina_Bool is_load_finished;
	Eina_Bool is_ewk_ready;
	Eina_Bool is_hided;

	Eina_Bool is_back_btn_clicked;
	Eina_Bool is_save_in_drafts_clicked;
	Eina_Bool is_send_btn_clicked;
	Eina_Bool is_composer_getting_destroyed;

	Eina_Bool is_from_mbe_moved;
	Eina_Bool is_adding_account_requested;
	Eina_Bool is_need_close_composer_on_low_memory;
	Eina_Bool is_forward_download_processed;
	Eina_Bool is_inline_contents_inserted;
	Eina_Bool need_to_destroy_after_initializing;
	Eina_Bool need_to_display_max_size_popup;
	Eina_Bool is_force_destroy;
	Eina_Bool is_mbe_edit_mode;

	/* Related to popup translation */
	int pt_element_type;
	int pt_package_type;
	char *pt_string_format;
	char *pt_string_id;
	char *pt_data1;
	int pt_data2;

	/* Related to richtex toolbar */
	Evas_Object *richtext_toolbar;
	Evas_Object *rttb_placeholder;

	Evas_Object *richtext_font_size_radio_group;
	int richtext_font_size_pixels;
	email_rgba_t richtext_font_color;
	email_rgba_t richtext_bg_color;
	Evas_Object *richtext_colorselector;
	RichButtonState richtext_button_list[RICH_TEXT_TYPE_COUNT];
	Eina_Bool richtext_is_disable;

	/* Related to contacts sharing */
	int shared_contacts_count;
	int shared_contact_id;
	int *shared_contact_id_list;
	char *vcard_file_path;
	Ecore_Timer *vcard_save_timer;
	pthread_t vcard_save_thread;
	Eina_Bool vcard_save_cancel;
	Eina_Bool is_sharing_contacts;
	Eina_Bool is_sharing_my_profile;
	Eina_Bool is_vcard_create_error;

	attach_panel_h attach_panel;
};

/**
 * @brief Email composer module data
 */
typedef struct {
	email_module_t base;
	EmailComposerView view;
} EmailComposerModule;

#endif /* __EMAIL_COMPOSER_H__ */
