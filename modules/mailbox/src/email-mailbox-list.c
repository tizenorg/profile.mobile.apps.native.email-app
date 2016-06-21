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

#define _GNU_SOURCE

#include <notification.h>
#include <system_settings.h>
#include <utils_i18n_ulocale.h>

#include "email-common-types.h"
#include "email-mailbox.h"
#include "email-mailbox-list.h"
#include "email-mailbox-list-extensions.h"
#include "email-mailbox-search.h"
#include "email-mailbox-sync.h"
#include "email-mailbox-toolbar.h"
#include "email-mailbox-noti-mgr.h"
#include "email-mailbox-request.h"
#include "email-mailbox-title.h"
#include "email-mailbox-module-util.h"
#include "email-mailbox-util.h"
#include "email-mailbox-more-menu.h"

/*
 * Definitions
 */

#define MAILBOX_STATUS_ICON_PADDING_SIZE 12
#define MAILBOX_LIST_LEFT_ICON_PADDING_SIZE 4
#define SELECT_ALL_LIST_ITEM_HEIGHT 121	/* 120 + 1 (margin) */

#define BLOCK_COUNT 10
#define FIRST_BLOCK_SIZE 9

/*
 * Structures
 */


/*
 * Globals
 */


/*
 * Declaration for static functions
 */

/* genlist callbacks */
static void _realized_item_cb(void *data, Evas_Object *obj, void *event_info);
static void _pressed_item_cb(void *data, Evas_Object *obj, void *event_info);
static void _mailbox_gl_free_cb(void *data, Evas *e, Evas_Object *o, void *info);

/* mail item */
static char *_mail_item_gl_text_get(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_mail_item_gl_content_get(void *data, Evas_Object *obj, const char *source);
static char *_mail_item_gl_subject_text_add(MailItemData *ld);
static char *_mail_item_gl_sender_text_add(MailItemData *ld);
static char *_mail_item_gl_time_stamp_text_add(MailItemData *ld);
static Evas_Object *_mail_item_gl_attach_icon_add(Evas_Object *parent, MailItemData *ld);
static Evas_Object *_mail_item_gl_status_icons_add(Evas_Object *parent, MailItemData *ld);
static Evas_Object *_mail_item_gl_star_checkbox_add(Evas_Object *parent, MailItemData *ld);
static Evas_Object *_mail_item_gl_select_checkbox_add(Evas_Object *parent, MailItemData *ld);
static Evas_Object *_mail_item_gl_account_colorbar_add(Evas_Object *parent, MailItemData *ld);
static void _mail_item_gl_text_style_set(MailItemData *ld);
static void _mail_item_gl_selected_cb(void *data, Evas_Object *obj, void *event_info);
static void _mail_item_check_changed_cb(void *data, Evas_Object *obj, void *event_info);
static void _mail_item_check_process_change(MailItemData *ld);
static void _mail_item_important_status_changed_cb(void *data, Evas_Object *obj, void *event_info);

static void _mail_item_data_insert_search_tag(char *dest, int dest_len, const char *src, const char *key);
static void _mail_item_data_list_free(GList **mail_item_data_list);

static void _mailbox_star_ly_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _mailbox_select_checkbox_ly_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

/* insert mail item data into list */
static void _insert_mail_item_to_list(EmailMailboxView *view, MailItemData *ld);
static void _insert_mail_item_to_list_from_noti(EmailMailboxView *view, MailItemData *ld, MailItemData *prev, MailItemData *next);
static gint _compare_mail_item_order(gconstpointer a, gconstpointer b);

/* get mail list */
static email_mail_list_item_t *_mailbox_get_mail_list(EmailMailboxView *view, const email_search_data_t *search_data, int *mail_count);
static email_mail_list_item_t *_mailbox_get_mail_list_by_mailbox_id(int account_id, int mailbox_id, int sort_type, int thread_id, const email_search_data_t *search_data, int *mail_count);
static email_mail_list_item_t *_mailbox_get_mail_list_by_mailbox_type(int account_id, int mailbox_type, int sort_type, int thread_id, const email_search_data_t *search_data, int *mail_count);
static email_mail_list_item_t *_mailbox_get_priority_sender_mail_list(int sort_type, int thread_id, const email_search_data_t *search_data, int *mail_count);
static email_mail_list_item_t *_mailbox_get_favourite_mail_list(int sort_type, int thread_id, const email_search_data_t *search_data, int *mail_count);
static email_mail_list_item_t *_mailbox_get_mail_list_for_search_all_folders(int account_id, int sort_type, const email_search_data_t *search_data, int *mail_count);

static int _get_filter_cnt_for_search_data(const email_search_data_t *search_data);
static void _add_search_data_into_filter_list(const email_search_data_t *search_data, email_list_filter_t *filter_list, int *current_index);
static void _make_sorting_rule_list(email_sort_type_e sort_type, int account_id, email_list_sorting_rule_t *sorting_rule_list);

/* mail list */
static void _mailbox_make_list(EmailMailboxView *view, const email_search_data_t *search_data);
static void _mailbox_make_remaining_items(EmailMailboxView *view, const email_search_data_t *search_data,
		email_mail_list_item_t *mail_list, int mail_count);
static void _mailbox_clear_list(EmailMailboxView *view);
static void _mailbox_free_req_data(AddRemainingMailReqData **req);
static email_search_data_t *_clone_search_data(const email_search_data_t *search_data);

/* system settings callbacks */
static void _mailbox_system_font_change_cb(system_settings_key_e key, void *data);
static char *_mailbox_get_ricipient_display_string(int mail_id);
static void _mailbox_get_recipient_display_information(const gchar *addr_list, char **recipient_str, int *recipent_cnt, bool count_only);

static void mailbox_exit_clicked_edit_mode(void *data, Evas_Object *obj, void *event_info);
static void mailbox_update_done_clicked_edit_mode(void *data, Evas_Object *obj, void *event_info);

/* check box cache */
static void _mailbox_check_cache_init(EmailMailboxCheckCache *cache, const char *obj_style);
static void _mailbox_check_cache_free(EmailMailboxCheckCache *cache);
static Evas_Object *_mailbox_check_cache_get_obj(EmailMailboxCheckCache *cache, Evas_Object *parent);
static void _mailbox_check_cache_release_obj(EmailMailboxCheckCache *cache, Evas_Object *obj);

/*Folders name cache for search in all folders mode*/
static char *_mailbox_get_cashed_folder_name(EmailMailboxView *view, email_mailbox_type_e mailbox_type, int mailbox_id);
static int _mailbox_folder_name_cashe_comparator_cb(const void *data1, const void *data2);

static void _mailbox_list_insert_n_mails(EmailMailboxView *view, email_mail_list_item_t* mail_list, int count, const email_search_data_t *search_data);

static const Elm_Genlist_Item_Class itc = {
	ELM_GEN_ITEM_CLASS_HEADER,
	.item_style = "email_mailbox",
	.func.text_get = _mail_item_gl_text_get,
	.func.content_get = _mail_item_gl_content_get
};

/*
 * Definition for static functions
 */
static void _mail_item_gl_text_style_set(MailItemData *ld)
{
	retm_if(!ld, "ld is NULL");

	if (ld->is_highlited) {
		if (ld->is_seen == true) {
			elm_object_item_signal_emit(ld->base.item, "highlight_read_mail", "mailbox");
		} else {
			elm_object_item_signal_emit(ld->base.item, "highlight_unread_mail", "mailbox");
		}
	} else {
		if (ld->is_seen == true) {
			elm_object_item_signal_emit(ld->base.item, "check_mail_as_read", "mailbox");
		} else {
			elm_object_item_signal_emit(ld->base.item, "check_mail_as_unread", "mailbox");
		}
	}
}

static void _realized_item_cb(void *data, Evas_Object *obj, void *event_info)
{
	retm_if(!event_info, "event_info is NULL");

	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	MailboxBaseItemData *base_data = (MailboxBaseItemData *)elm_object_item_data_get(item);
	retm_if(!base_data, "base_data is NULL");

	if (base_data->item_type == MAILBOX_LIST_ITEM_MAIL_BRIEF) {
		MailItemData *ld = (MailItemData *)elm_object_item_data_get(item);

		ld->is_highlited = EINA_FALSE;
		_mail_item_gl_text_style_set(ld);
	}
}

static void _pressed_item_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!event_info, "event_info is NULL");

	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	MailboxBaseItemData *base_data = (MailboxBaseItemData *)elm_object_item_data_get(item);
	retm_if(!base_data, "base_data is NULL");

	if (base_data->item_type == MAILBOX_LIST_ITEM_MAIL_BRIEF) {
		MailItemData *ld = (MailItemData *)elm_object_item_data_get(item);

		ld->is_highlited = EINA_TRUE;
		_mail_item_gl_text_style_set(ld);
	}

	debug_leave();
}

static void _mailbox_gl_free_cb(void *data, Evas *e, Evas_Object *o, void *info)
{
	EmailMailboxView *view = data;

	_mailbox_check_cache_free(&view->star_check_cache);
	_mailbox_check_cache_free(&view->select_check_cache);
}

static Evas_Object *_mail_item_gl_account_colorbar_add(Evas_Object *parent, MailItemData *ld)
{
	retvm_if(!ld, NULL, "ld is NULL");
	retvm_if(!parent, NULL, "item_ly is NULL");

	Evas_Object *account_colorbar = evas_object_rectangle_add(evas_object_evas_get(parent));
	evas_object_size_hint_fill_set(account_colorbar, EVAS_HINT_FILL, EVAS_HINT_FILL);

	int account_color = mailbox_account_color_list_get_account_color(ld->view, ld->account_id);
	int r = R_MASKING(account_color);
	int g = G_MASKING(account_color);
	int b = B_MASKING(account_color);
	int a = A_MASKING(account_color);

	evas_object_color_set(account_colorbar, r, g, b, a);

	return account_colorbar;
}

static Evas_Object *_mail_item_gl_select_checkbox_add(Evas_Object *parent, MailItemData *ld)
{
	retvm_if(!ld, NULL, "ld is NULL");
	retvm_if(!parent,  NULL, "parent is NULL");

	Evas_Object *select_checkbox_ly = elm_layout_add(parent);
	evas_object_size_hint_weight_set(select_checkbox_ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_layout_file_set(select_checkbox_ly, email_get_mailbox_theme_path(), "email/layout/select.icon");

	Evas_Object *select_checkbox = _mailbox_check_cache_get_obj(&ld->view->select_check_cache, parent);

	elm_check_state_set(select_checkbox, ld->checked);
	evas_object_smart_callback_add(select_checkbox, "changed", _mail_item_check_changed_cb, ld);
	ld->check_btn = select_checkbox;

	elm_layout_content_set(select_checkbox_ly, "elm.swallow.content", ld->check_btn);

	evas_object_event_callback_add(select_checkbox_ly, EVAS_CALLBACK_DEL, _mailbox_select_checkbox_ly_del_cb, ld);

	return select_checkbox_ly;

}

static Evas_Object *_mail_item_gl_star_checkbox_add(Evas_Object *parent, MailItemData *ld)
{
	retvm_if(!ld, NULL,  "ld is NULL");
	retvm_if(!parent, NULL,  "parent is NULL");

	Evas_Object *star_ly = elm_layout_add(parent);
	evas_object_size_hint_weight_set(star_ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_layout_file_set(star_ly, email_get_mailbox_theme_path(), "email/layout/star.icon");

	Evas_Object *star_checkbox = _mailbox_check_cache_get_obj(&ld->view->star_check_cache, parent);

	if (ld->flag_type == EMAIL_FLAG_FLAGED) {
		elm_check_state_set(star_checkbox, EINA_TRUE);
	} else {
		elm_check_state_set(star_checkbox, EINA_FALSE);
	}

	if (!ld->view->b_editmode) {
		evas_object_freeze_events_set(star_checkbox, EINA_FALSE);
		evas_object_smart_callback_add(star_checkbox, "changed", _mail_item_important_status_changed_cb, ld);
	} else {
		evas_object_freeze_events_set(star_checkbox, EINA_TRUE);
	}

	ld->check_favorite_btn = star_checkbox;
	elm_layout_content_set(star_ly, "elm.swallow.content", star_checkbox);

	evas_object_event_callback_add(star_ly, EVAS_CALLBACK_DEL, _mailbox_star_ly_del_cb, ld);

	return star_ly;
}

static Evas_Object *_mail_item_gl_status_icons_add(Evas_Object *parent, MailItemData *ld)
{
	retvm_if(!ld, NULL, "ld is NULL");
	retvm_if(!parent, NULL, "parent is NULL");

	Evas_Object *icon = NULL;
	Evas_Object *box = elm_box_add(parent);
	elm_box_horizontal_set(box, EINA_TRUE);

	if (ld->is_priority_sender_mail && (ld->reply_flag || ld->forward_flag)) {
		elm_box_padding_set(box, ELM_SCALE_SIZE(MAILBOX_LIST_LEFT_ICON_PADDING_SIZE), 0);
	}

	if (ld->is_priority_sender_mail) {
		icon = elm_layout_add(box);
		elm_layout_file_set(icon, email_get_common_theme_path(), EMAIL_IMAGE_ICON_PRIO_SENDER);
		evas_object_show(icon);
		elm_box_pack_end(box, icon);
	}

	if(ld->reply_flag && ld->forward_flag) {
		icon = elm_layout_add(box);
		elm_layout_file_set(icon, email_get_common_theme_path(), EMAIL_IMAGE_ICON_LIST_REPLY_FORWARD);
		evas_object_show(icon);
		elm_box_pack_end(box, icon);
	} else if (ld->reply_flag) {
		icon = elm_layout_add(box);
		elm_layout_file_set(icon, email_get_common_theme_path(), EMAIL_IMAGE_ICON_LIST_REPLY);
		evas_object_show(icon);
		elm_box_pack_end(box, icon);
	} else if (ld->forward_flag) {
		icon = elm_layout_add(box);
		elm_layout_file_set(icon, email_get_common_theme_path(), EMAIL_IMAGE_ICON_LIST_FORWARD);
		evas_object_show(icon);
		elm_box_pack_end(box, icon);
	}

	icon = evas_object_rectangle_add(evas_object_evas_get(box));
	evas_object_size_hint_min_set(icon, ELM_SCALE_SIZE(MAILBOX_STATUS_ICON_PADDING_SIZE), 0);
	evas_object_color_set(icon, TRANSPARENT);
	evas_object_show(icon);
	elm_box_pack_end(box, icon);

	return box;

}

static Evas_Object *_mail_item_gl_attach_icon_add(Evas_Object *parent, MailItemData *ld)
{
	retvm_if(!ld, NULL, "ld is NULL");
	retvm_if(!parent, NULL, "parent is NULL");

	Evas_Object *icon =  elm_layout_add(parent);
	elm_layout_file_set(icon, email_get_common_theme_path(), EMAIL_IMAGE_ICON_LIST_ATTACH);
	evas_object_show(icon);

	return icon;

}

static char *_mail_item_gl_time_stamp_text_add(MailItemData *ld)
{
	retvm_if(!ld, NULL, "ld is NULL");

	if (ld->timeordate) {
		return strdup(ld->timeordate);
	}

	return NULL;

}

static char *_mail_item_gl_sender_text_add(MailItemData *ld)
{
	retvm_if(!ld, NULL, "ld is NULL");

	char *address_info = NULL;
	if (ld->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX ||
			ld->mailbox_type == EMAIL_MAILBOX_TYPE_SENTBOX ||
			ld->mailbox_type == EMAIL_MAILBOX_TYPE_DRAFT) {
		address_info = email_util_strtrim(ld->recipient);
	} else {
		if (0 == strlen(email_util_strtrim(ld->alias)))
			address_info = email_util_strtrim(ld->recipient);
		else
			address_info = email_util_strtrim(ld->alias);
	}

	if (address_info) {
		return strdup(address_info);
	}

	return NULL;
}

static char *_mail_item_gl_subject_text_add(MailItemData *ld)
{
	retvm_if(!ld, NULL, "ld is NULL");

	char *title = ld->title;
	if ((ld->message_class == EMAIL_MESSAGE_CLASS_SMS) && ld->preview_body) {
		title = ld->preview_body;
	}

	if (!title) {
		title = _("IDS_EMAIL_SBODY_NO_SUBJECT_M_NOUN");
	}

	return strdup(title);
}

static char *_mail_item_gl_text_get(void *data, Evas_Object *obj, const char *part)
{
	retvm_if(!data, NULL, "data is NULL");

	MailItemData *ld = (MailItemData *)data;
	EmailMailboxView *view = ld->view;
	retvm_if(!view, NULL, "view is NULL");

	if (!strcmp(part, "email.data.text")) {
		return _mail_item_gl_time_stamp_text_add(ld);
	} else if (!strcmp(part, "email.sender.text")) {
		return _mail_item_gl_sender_text_add(ld);
	} else if (!strcmp(part, "email.subject.text")) {
		return _mail_item_gl_subject_text_add(ld);
	}
	return NULL;

}

static Evas_Object *_mail_item_gl_content_get(void *data, Evas_Object *obj, const char *part)
{

	retvm_if(!data, NULL, "data is NULL");

	MailItemData *ld = (MailItemData *)data;
	EmailMailboxView *view = ld->view;
	retvm_if(!view, NULL, "view is NULL");

	if (!strcmp(part, "select_checkbox_icon") && (view->b_editmode == true)) {
			return _mail_item_gl_select_checkbox_add(obj, ld);
	}

	if (!strcmp(part, "account_colorbar") && view && ld->account_id > 0 && view->account_count > 1 && (view->mode == EMAIL_MAILBOX_MODE_ALL
#ifndef _FEATURE_PRIORITY_SENDER_DISABLE_
					|| view->mode == EMAIL_MAILBOX_MODE_PRIORITY_SENDER
#endif
			)) {
		return _mail_item_gl_account_colorbar_add(obj, ld);
	}

	if (!strcmp(part, "email.star.icon") && (view->mailbox_type != EMAIL_MAILBOX_TYPE_OUTBOX && ld->view->mailbox_type != EMAIL_MAILBOX_TYPE_DRAFT)) {
		return _mail_item_gl_star_checkbox_add(obj, ld);
	}

	if (!strcmp(part, "email.status.icon") && (ld->reply_flag || ld->forward_flag ||  (ld->is_priority_sender_mail &&  (view->mode == EMAIL_MAILBOX_MODE_ALL
						|| view->mode == EMAIL_MAILBOX_MODE_MAILBOX
#ifndef _FEATURE_PRIORITY_SENDER_DISABLE_
						|| view->mode == EMAIL_MAILBOX_MODE_PRIORITY_SENDER
#endif
						)))) {
		return  _mail_item_gl_status_icons_add(obj, ld);
	}

	if (!strcmp(part, "email.attachment.icon") && ld->is_attachment) {
		return _mail_item_gl_attach_icon_add(obj, ld);
	}

	return NULL;
}

static void _mail_item_gl_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!event_info, "event_info is NULL");

	Elm_Object_Item *it = (Elm_Object_Item *)event_info;
	MailItemData *ld = elm_object_item_data_get(it);

	retm_if(!ld, "ld is NULL");
	retm_if(!ld->view, "ld->view == NULL");
	elm_genlist_item_selected_set((Elm_Object_Item *)it, EINA_FALSE);
	if (ld->view->b_editmode) {
		_mail_item_check_changed_cb(ld, obj, NULL);
		return;
	}
	DELETE_EVAS_OBJECT(ld->view->more_ctxpopup);

	mailbox_sync_cancel_all(ld->view);
	int id = ld->mail_id;
	email_mailbox_t *mbox = NULL;
	if (!email_engine_get_mailbox_by_mailbox_id(ld->mailbox_id, &mbox)) {
		debug_warning("email_engine_get_mailbox_by_mailbox_id mailbox_id(%d)", ld->mailbox_id);
		return;
	}

	if (mbox->mailbox_type == EMAIL_MAILBOX_TYPE_DRAFT && email_engine_check_body_download(ld->mail_id)) {
		if (ld->view->composer) {
			debug_log("Composer module is already launched");
			return;
		}

		email_params_h params = NULL;

		if (email_params_create(&params) &&
			email_params_add_int(params, EMAIL_BUNDLE_KEY_RUN_TYPE, RUN_COMPOSER_EDIT) &&
			email_params_add_int(params, EMAIL_BUNDLE_KEY_ACCOUNT_ID, ld->account_id) &&
			email_params_add_int(params, EMAIL_BUNDLE_KEY_MAILBOX, ld->mailbox_id) &&
			email_params_add_int(params, EMAIL_BUNDLE_KEY_MAIL_ID, id)) {

			ld->view->composer = mailbox_composer_module_create(ld->view, EMAIL_MODULE_COMPOSER, params);
		}

		email_params_free(&params);
	} else {
		mailbox_list_open_email_viewer(ld);
	}
	email_engine_free_mailbox_list(&mbox, 1);
}

static void _mail_item_check_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	MailItemData *ld = (MailItemData *)data;
	retm_if(!ld, "ld is NULL");
	retm_if(!ld->check_btn, "ld->check_btn is NULL");

	if (ld != NULL) {
		if (ld->mail_status == EMAIL_MAIL_STATUS_SENDING || ld->mail_status == EMAIL_MAIL_STATUS_SEND_WAIT) {
			debug_log("email being sent selected - unselect it(%d)", ld->mail_status);
			ld->checked = EINA_FALSE;
			elm_check_state_set(ld->check_btn, ld->checked);
			int ret = notification_status_message_post(email_get_email_string("IDS_EMAIL_TPOP_SENDING_EMAIL_ING"));
			if (ret != NOTIFICATION_ERROR_NONE) {
				debug_warning("notification_status_message_post() failed: %d", ret);
			}
			return;
		}

		ld->checked = !ld->checked;
		elm_check_state_set(ld->check_btn, ld->checked);

		_mail_item_check_process_change(ld);
	}
}

static void _mail_item_check_process_change(MailItemData *ld)
{
	debug_enter();

	EmailMailboxView *view = ld->view;

	if (ld->checked) {
		view->selected_mail_list = eina_list_append(view->selected_mail_list, ld);
	} else {
		view->selected_mail_list = eina_list_remove(view->selected_mail_list, ld);
	}

	int cnt = eina_list_count(view->selected_mail_list);
	debug_log("list count = %d", cnt);

	if (!view->b_editmode) {
		debug_log("Enter edit mode.[%d]", cnt);
		if (cnt == 1 && !view->b_editmode) {
			debug_log("view->selected_mail_list has something wrong.[%d]", cnt);
			view->selected_mail_list = eina_list_free(view->selected_mail_list);
			view->selected_mail_list = NULL;
			if (ld->checked) {
				view->selected_mail_list = eina_list_append(view->selected_mail_list, ld);
			}
		}

		view->b_editmode = true;

		mailbox_sync_cancel_all(view);
		if ((view->mode == EMAIL_MAILBOX_MODE_ALL
			|| view->mode == EMAIL_MAILBOX_MODE_MAILBOX)
			&& view->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX) {
				mailbox_send_all_btn_remove(view);
			}
		mailbox_change_search_layout_state(view, false);

		mailbox_create_select_info(view);
		mailbox_toolbar_create(view);
	} else if (true == view->b_editmode) {
		debug_log("Update edit mode");
		mailbox_update_select_info(view);
	}

	/* Update Select All Checkbox */
	int checked_count = eina_list_count(view->selected_mail_list);
	int total_count = elm_genlist_items_count(view->gl);
	if (checked_count == (total_count-1)) {
		view->select_all_item_data.is_checked = EINA_TRUE;
	} else {
		view->select_all_item_data.is_checked = EINA_FALSE;
	}

	elm_check_state_set(view->select_all_item_data.checkbox, view->select_all_item_data.is_checked);

}

static void _mail_item_important_status_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	MailItemData *ld = (MailItemData *)data;
	int count = 0;
	retm_if(!ld, "ld is NULL");
	retm_if(!ld->check_favorite_btn, "ld->check_favorite_btn is NULL");

	ld->view->important_list = g_list_prepend(ld->view->important_list,
			GINT_TO_POINTER(ld->mail_id));
	count = 1;

	if (ld->view->important_list) {
		int mail_ids[count];
		memset(mail_ids, 0, sizeof(mail_ids));
		int i = 0;
		GList *cur = g_list_first(ld->view->important_list);
		for ( ; i < count; ++i, cur = g_list_next(cur)) {
			mail_ids[i] = (int)(ptrdiff_t)g_list_nth_data(cur, 0);
		}

		email_engine_set_flags_field(ld->account_id, mail_ids, count, EMAIL_FLAGS_FLAGGED_FIELD, ld->flag_type ? 0 : 1, true);

		g_list_free(ld->view->important_list);
		ld->view->important_list = NULL;
	}
}

/* dest_len is max str len of dest (not including \0)*/
static void _mail_item_data_insert_search_tag(char *dest, int dest_len, const char *src, const char *key)
{
	if (!key || !dest || !src) return;
	int src_len = STR_LEN(src);
	int key_len = STR_LEN(key);
	if (src_len <= 0 || key_len <= 0) {
		debug_warning("src(%d) or key(%d) 0 long", src_len, key_len);
		return;
	}

	char *sub = (char *)strcasestr(src, key);
	if (!sub) {
		STR_NCPY(dest, src, dest_len);
		return;
	}

	int sub_len = STR_LEN(sub);
	int pre_len = src_len - sub_len;

	char fmt[EMAIL_BUFF_SIZE_MID];
	snprintf(fmt, sizeof(fmt), "%%.%ds<match>%%.%ds</match>%%.%ds",
			pre_len, key_len, sub_len - key_len);

	snprintf(dest, dest_len, fmt, src, src + pre_len, src + pre_len + key_len);
}

static void _mail_item_data_list_free(GList **mail_item_data_list)
{
	retm_if(!mail_item_data_list, "mail_item_data_list is NULL");

	if (!*mail_item_data_list) {
		return;
	}

	int i = 0;
	GList *list = (GList *)(*mail_item_data_list);
	MailItemData *ld = NULL;
	debug_log("g_list_length(list): %d", g_list_length(list));

	for (i = 0; i < g_list_length(list); i++) {
		ld = (MailItemData *)g_list_nth_data(list, i);
		mailbox_list_free_mail_item_data(ld);
	}

	if (list) {
		g_list_free(list);
		list = NULL;
	}

	*mail_item_data_list = NULL;
}

static void _mailbox_star_ly_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	MailItemData *ld = data;

	evas_object_smart_callback_del(ld->check_favorite_btn, "changed", _mail_item_important_status_changed_cb);

	_mailbox_check_cache_release_obj(&ld->view->star_check_cache, ld->check_favorite_btn);

	ld->check_favorite_btn = NULL;
}

static void _mailbox_select_checkbox_ly_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	MailItemData *ld = data;

	evas_object_smart_callback_del(ld->check_btn, "changed", _mail_item_check_changed_cb);

	_mailbox_check_cache_release_obj(&ld->view->select_check_cache, ld->check_btn);

	ld->check_btn = NULL;
}

static void _insert_mail_item_to_list(EmailMailboxView *view, MailItemData *ld)
{
	debug_enter();

	/* insert normal item */
	if (view->get_more_progress_item != NULL) {
		ld->base.item = elm_genlist_item_insert_before(view->gl,
						&itc,
						ld,
						NULL,
						view->get_more_progress_item,
						ELM_GENLIST_ITEM_NONE,
						_mail_item_gl_selected_cb,
						NULL);
	} else if (view->load_more_messages_item != NULL) {
		ld->base.item = elm_genlist_item_insert_before(view->gl,
						&itc,
						ld,
						NULL,
						view->load_more_messages_item,
						ELM_GENLIST_ITEM_NONE,
						_mail_item_gl_selected_cb,
						NULL);
	} else if (view->no_more_emails_item != NULL) {
		ld->base.item = elm_genlist_item_insert_before(view->gl,
						&itc,
						ld,
						NULL,
						view->no_more_emails_item,
						ELM_GENLIST_ITEM_NONE,
						_mail_item_gl_selected_cb,
						NULL);
	} else {
		ld->base.item = elm_genlist_item_append(view->gl,
						&itc,
						ld,
						NULL,
						ELM_GENLIST_ITEM_NONE,
						_mail_item_gl_selected_cb,
						NULL);
	}
	debug_leave();
}

static void _insert_mail_item_to_list_from_noti(EmailMailboxView *view, MailItemData *ld, MailItemData *prev, MailItemData *next)
{
	debug_enter();
	debug_secure_trace("prev->title: [%s], next->title: [%s]", prev ? prev->title : "NULL", next ? next->title : "NULL");

	Elm_Object_Item *select_all_item = view->select_all_item_data.base.item;
	Elm_Object_Item *update_time_item = view->update_time_item_data.base.item;

	if (!prev || !prev->base.item) {
		if (update_time_item) {
			ld->base.item = elm_genlist_item_insert_after(view->gl,
								&itc,
								ld,
								NULL,
								update_time_item,
								ELM_GENLIST_ITEM_NONE,
								_mail_item_gl_selected_cb, NULL);
		} else if (select_all_item) {
			ld->base.item = elm_genlist_item_insert_after(view->gl,
								&itc,
								ld,
								NULL,
								select_all_item,
								ELM_GENLIST_ITEM_NONE,
								_mail_item_gl_selected_cb, NULL);
		} else {
			ld->base.item = elm_genlist_item_prepend(view->gl,
								&itc,
								ld,
								NULL,
								ELM_GENLIST_ITEM_NONE,
								_mail_item_gl_selected_cb,
								NULL);

			elm_genlist_item_show(ld->base.item, ELM_GENLIST_ITEM_SCROLLTO_TOP);
		}
	} else {
		ld->base.item = elm_genlist_item_insert_after(view->gl,
								&itc,
								ld,
								NULL,
								prev->base.item,
								ELM_GENLIST_ITEM_NONE,
								_mail_item_gl_selected_cb, NULL);
	}
	debug_leave();
}

gint _compare_mail_item_order(gconstpointer a, gconstpointer b)
{
	int ret = 0;
	if (a == NULL || b == NULL)
		return 0;
	MailItemData *first_item = (MailItemData *)a;
	MailItemData *second_item = (MailItemData *)b;
	EmailMailboxView *view = first_item->view;

	int sort_type = view->sort_type;

	switch (sort_type) {
		case EMAIL_SORT_DATE_RECENT:
		case EMAIL_SORT_DATE_OLDEST:
			if (first_item->absolute_time > second_item->absolute_time)
				ret = (sort_type == EMAIL_SORT_DATE_RECENT) ? -1 : 1;
			else if (first_item->absolute_time == second_item->absolute_time)
				ret = 1;
			else
				ret = (sort_type == EMAIL_SORT_DATE_RECENT) ? 1 : -1;
			break;

		case EMAIL_SORT_UNREAD:
			if (first_item->is_seen > second_item->is_seen) {
				ret = 1;
			} else if (first_item->is_seen == second_item->is_seen) {
				if (first_item->absolute_time > second_item->absolute_time)
					ret = -1;
				else
					ret = 1;
			} else
				ret = -1; /* Unread item is added first */
			break;

		case EMAIL_SORT_IMPORTANT:
			if (first_item->flag_type > second_item->flag_type) {
				ret = -1; /* Important item is added first */
			} else if (first_item->flag_type == second_item->flag_type) {
				if (first_item->absolute_time > second_item->absolute_time)
					ret = -1;
				else
					ret = 1;
			} else
				ret = 1;
			break;

		case EMAIL_SORT_PRIORITY:
			if (first_item->priority > second_item->priority) {
				ret = 1; /* high priority item is added first */
			} else if (first_item->priority == second_item->priority) {
				if (first_item->absolute_time > second_item->absolute_time)
					ret = -1;
				else
					ret = 1;
			} else
				ret = -1;
			break;

		case EMAIL_SORT_ATTACHMENTS:
			if (first_item->is_attachment > second_item->is_attachment) {
				ret = -1; /* if attachment is contained, added first */
			} else if (first_item->is_attachment == second_item->is_attachment) {
				if (first_item->absolute_time > second_item->absolute_time)
					ret = -1;
				else
					ret = 1;
			} else
				ret = 1;
			break;
		case EMAIL_SORT_TO_CC_BCC:
			if (first_item->is_to_address_mail > second_item->is_to_address_mail) {
				ret = 1;
			} else if (first_item->is_to_address_mail == second_item->is_to_address_mail) {
				if (first_item->absolute_time > second_item->absolute_time)
					ret = -1;
				else
					ret = 1;
			} else
				ret = -1;
			break;

		case EMAIL_SORT_SENDER_ATOZ:
		case EMAIL_SORT_SENDER_ZTOA:
		case EMAIL_SORT_RCPT_ATOZ:
		case EMAIL_SORT_RCPT_ZTOA:
			if (!first_item->group_title && !second_item->group_title) {
				if (first_item->absolute_time > second_item->absolute_time)
					ret = -1;
				else
					ret = 1;
			} else if (!first_item->group_title && second_item->group_title) {
				ret = (sort_type == EMAIL_SORT_SENDER_ATOZ || sort_type == EMAIL_SORT_RCPT_ATOZ) ? -1 : 1;
			} else if (first_item->group_title && !second_item->group_title) {
				ret = (sort_type == EMAIL_SORT_SENDER_ATOZ || sort_type == EMAIL_SORT_RCPT_ATOZ) ? 1 : -1;
			} else {
				ret = g_strcmp0(first_item->group_title, second_item->group_title);
				if (!ret) {
					if (first_item->absolute_time > second_item->absolute_time)
						ret = -1;
					else
						ret = 1;
				} else {
					ret = (sort_type == EMAIL_SORT_SENDER_ATOZ || sort_type == EMAIL_SORT_RCPT_ATOZ) ? ret : ret * (-1);
				}
			}

			debug_secure("####### %d, %s, %s, %d", sort_type, first_item->group_title, second_item->group_title, ret);

			break;

		case EMAIL_SORT_SUBJECT_ATOZ:
		case EMAIL_SORT_SUBJECT_ZTOA:
			if (!first_item->title && !second_item->title) {
				if (first_item->absolute_time > second_item->absolute_time)
					ret = -1;
				else
					ret = 1;
			} else if (!first_item->title && second_item->title) {
				ret = (sort_type == EMAIL_SORT_SUBJECT_ATOZ) ? -1 : 1;
			} else if (first_item->title && !second_item->title) {
				ret = (sort_type == EMAIL_SORT_SUBJECT_ATOZ) ? 1 : -1;
			} else {
				ret = g_strcmp0(first_item->title, second_item->title);
				if (!ret) {
					if (first_item->absolute_time > second_item->absolute_time)
						ret = -1;
					else
						ret = 1;
				} else {
					ret = (sort_type == EMAIL_SORT_SUBJECT_ATOZ) ? ret : ret * (-1);
				}
			}

			debug_secure("####### %d, %s, %s, %d", sort_type, first_item->title, second_item->title, ret);

			break;
		case EMAIL_SORT_SIZE_SMALLEST:
			if (first_item->mail_size > second_item->mail_size) {
				ret = 1;
			} else if (first_item->is_seen == second_item->is_seen) {
				if (first_item->absolute_time > second_item->absolute_time)
					ret = -1;
				else
					ret = 1;
			} else
				ret = -1;
			break;

		case EMAIL_SORT_SIZE_LARGEST:
			if (first_item->mail_size > second_item->mail_size) {
				ret = -1;
			} else if (first_item->is_seen == second_item->is_seen) {
				if (first_item->absolute_time > second_item->absolute_time)
					ret = -1;
				else
					ret = 1;
			} else
				ret = 1;
			break;
	}
	return ret;
}

static email_mail_list_item_t *_mailbox_get_mail_list(EmailMailboxView *view, const email_search_data_t *search_data, int *mail_count)
{
	debug_enter();

	email_mail_list_item_t *mail_data = NULL;
	int mailbox_type = EMAIL_MAILBOX_TYPE_NONE;

	if (view->mailbox_type == EMAIL_MAILBOX_TYPE_NONE) {
		mailbox_type = EMAIL_MAILBOX_TYPE_INBOX;
	} else {
		mailbox_type = view->mailbox_type;
	}

	if (view->b_searchmode) {
		retvm_if(!search_data, NULL, "Search data is NULL!");
		if (search_data->search_type == EMAIL_SEARCH_IN_SINGLE_FOLDER) {
			if (view->mode == EMAIL_MAILBOX_MODE_ALL) {
				if (mailbox_type == EMAIL_MAILBOX_TYPE_FLAGGED) {
					mail_data = _mailbox_get_favourite_mail_list(view->sort_type, EMAIL_GET_MAIL_NORMAL, search_data, mail_count);
				} else {
					mail_data = _mailbox_get_mail_list_by_mailbox_type(0, mailbox_type, EMAIL_SORT_DATE_RECENT, EMAIL_GET_MAIL_NORMAL, search_data, mail_count);
				}
			}
#ifndef _FEATURE_PRIORITY_SENDER_DISABLE_
			else if (view->mode == EMAIL_MAILBOX_MODE_PRIORITY_SENDER) {
				mail_data = _mailbox_get_priority_sender_mail_list(EMAIL_SORT_DATE_RECENT, EMAIL_GET_MAIL_NORMAL, search_data, mail_count);
			}
#endif
			else {
				mail_data = _mailbox_get_mail_list_by_mailbox_id(view->account_id, view->mailbox_id, EMAIL_SORT_DATE_RECENT, EMAIL_GET_MAIL_NORMAL, search_data, mail_count);
			}
		} else if (search_data->search_type == EMAIL_SEARCH_IN_ALL_FOLDERS) {
			mail_data = _mailbox_get_mail_list_for_search_all_folders(view->account_id, EMAIL_SORT_DATE_RECENT, search_data, mail_count);
		}
	} else {
		if (view->mode == EMAIL_MAILBOX_MODE_ALL) {
			if (mailbox_type == EMAIL_MAILBOX_TYPE_FLAGGED)
				mail_data = _mailbox_get_favourite_mail_list(view->sort_type, EMAIL_GET_MAIL_NORMAL, NULL, mail_count);
			else
				mail_data = _mailbox_get_mail_list_by_mailbox_type(0, mailbox_type, view->sort_type, EMAIL_GET_MAIL_NORMAL, NULL, mail_count);
		}
#ifndef _FEATURE_PRIORITY_SENDER_DISABLE_
		else if (view->mode == EMAIL_MAILBOX_MODE_PRIORITY_SENDER) {
			mail_data = _mailbox_get_priority_sender_mail_list(view->sort_type, EMAIL_GET_MAIL_NORMAL, NULL, mail_count);
		}
#endif
		else {
			mail_data = _mailbox_get_mail_list_by_mailbox_id(view->account_id, view->mailbox_id, view->sort_type, EMAIL_GET_MAIL_NORMAL, NULL, mail_count);
		}
	}

	debug_leave();
	return mail_data;
}

static email_mail_list_item_t *_mailbox_get_mail_list_by_mailbox_id(int account_id, int mailbox_id, int sort_type, int thread_id, const email_search_data_t *search_data, int *mail_count)
{
	debug_log("account_id: %d, mailbox_id: %d, sort_type: %d, thread_id : %d", account_id, mailbox_id, sort_type, thread_id);

	email_mail_list_item_t *mail_list = NULL;
	email_list_filter_t *filter_list = NULL;
	email_list_sorting_rule_t *sorting_rule_list = NULL;
	int cnt_filter_list = 0;
	int cnt_soring_rule = 0;
	int i = 0;
	bool isOutbox = false;

	if (GET_MAILBOX_TYPE(mailbox_id) == EMAIL_MAILBOX_TYPE_OUTBOX) {
		isOutbox = true;
		cnt_filter_list += 2;
	}

	if (account_id == 0)
		cnt_filter_list += 3;
	else if (account_id > 0)
		cnt_filter_list += 5;
	else {
		debug_warning("account_id SHOULD be greater than or equal to 0.");
		return NULL;
	}

	if (thread_id > 0 || thread_id == EMAIL_GET_MAIL_THREAD)
		cnt_filter_list += 2;

	if (search_data)
		cnt_filter_list += _get_filter_cnt_for_search_data(search_data);

	debug_log("cnt_filter_list: %d", cnt_filter_list);

	filter_list = calloc(cnt_filter_list, sizeof(email_list_filter_t));
	retvm_if(!filter_list, NULL, "filter_list is NULL!");

	if (account_id == 0) {
		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
		filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_MAILBOX_ID;
		filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_EQUAL;
		filter_list[i].list_filter_item.rule.key_value.integer_type_value = mailbox_id;
		i++;
	} else if (account_id > 0) {
		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
		filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_ACCOUNT_ID;
		filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_EQUAL;
		filter_list[i].list_filter_item.rule.key_value.integer_type_value = account_id;
		i++;

		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
		filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
		i++;

		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
		filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_MAILBOX_ID;
		filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_EQUAL;
		filter_list[i].list_filter_item.rule.key_value.integer_type_value = mailbox_id;
		i++;
	}

	if (thread_id > 0) {
		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
		filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
		i++;

		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
		filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_THREAD_ID;
		filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_EQUAL;
		filter_list[i].list_filter_item.rule.key_value.integer_type_value = thread_id;
		i++;
	} else if (thread_id == EMAIL_GET_MAIL_THREAD) {
		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
		filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
		i++;

		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
		filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_THREAD_ITEM_COUNT;
		filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_GREATER_THAN;
		filter_list[i].list_filter_item.rule.key_value.integer_type_value = 0;
		i++;
	}

	if (isOutbox) {
		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
		filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
		i++;

		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
		filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_SAVE_STATUS;
		filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_NOT_EQUAL;
		filter_list[i].list_filter_item.rule.key_value.integer_type_value = EMAIL_MAIL_STATUS_SEND_SCHEDULED;
		i++;
	}

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
	i++;

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_FLAGS_DELETED_FIELD;
	filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_NOT_EQUAL;
	filter_list[i].list_filter_item.rule.key_value.integer_type_value = 1;
	i++;

	if (search_data)
		_add_search_data_into_filter_list(search_data, filter_list, &i);

	debug_log("filled filter count: %d", i + 1);

	if (sort_type != EMAIL_SORT_DATE_RECENT && sort_type != EMAIL_SORT_DATE_OLDEST)
		cnt_soring_rule = 2;
	else
		cnt_soring_rule = 1;

	sorting_rule_list = calloc(cnt_soring_rule, sizeof(email_list_sorting_rule_t));
	if (!sorting_rule_list) {
		email_engine_free_list_filter(&filter_list, cnt_filter_list);
		debug_error("sorting_rule_list is NULL - allocation memory failed");
		return NULL;
	}

	_make_sorting_rule_list(sort_type, account_id, sorting_rule_list);

	email_engine_get_mail_list(filter_list, cnt_filter_list, sorting_rule_list, cnt_soring_rule, -1, -1, &mail_list, mail_count);

	FREE(sorting_rule_list);
	email_engine_free_list_filter(&filter_list, cnt_filter_list);

	debug_log("mail_count(%d)", *mail_count);

	debug_leave();

	return mail_list;
}

static email_mail_list_item_t *_mailbox_get_mail_list_by_mailbox_type(int account_id, int mailbox_type, int sort_type, int thread_id, const email_search_data_t *search_data, int *mail_count)
{
	debug_log("account_id: %d, mailbox_type: %d, sort_type: %d, thread_id : %d", account_id, mailbox_type, sort_type, thread_id);

	email_mail_list_item_t *mail_list = NULL;
	email_list_filter_t *filter_list = NULL;
	email_list_sorting_rule_t *sorting_rule_list = NULL;
	int cnt_filter_list = 0;
	int cnt_soring_rule = 0;
	int i = 0;
	bool isOutbox = (mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX) ? true : false;

	if (isOutbox)
		cnt_filter_list += 2;

	if (account_id == 0)
		cnt_filter_list += 3;
	else if (account_id > 0)
		cnt_filter_list += 5;
	else {
		debug_warning("account_id SHOULD be greater than or equal to 0.");
		return NULL;
	}

	if (thread_id > 0 || thread_id == EMAIL_GET_MAIL_THREAD)
		cnt_filter_list += 2;

	if (search_data)
		cnt_filter_list += _get_filter_cnt_for_search_data(search_data);

	debug_log("cnt_filter_list: %d", cnt_filter_list);

	filter_list = calloc(cnt_filter_list, sizeof(email_list_filter_t));
	retvm_if(!filter_list, NULL, "filter_list is NULL!");

	if (account_id == 0) {
		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
		filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_MAILBOX_TYPE;
		filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_EQUAL;
		filter_list[i].list_filter_item.rule.key_value.integer_type_value = mailbox_type;
		i++;
	} else if (account_id > 0) {
		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
		filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_ACCOUNT_ID;
		filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_EQUAL;
		filter_list[i].list_filter_item.rule.key_value.integer_type_value = account_id;
		i++;

		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
		filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
		i++;

		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
		filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_MAILBOX_TYPE;
		filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_EQUAL;
		filter_list[i].list_filter_item.rule.key_value.integer_type_value = mailbox_type;
		i++;
	}

	if (thread_id > 0) {
		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
		filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
		i++;

		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
		filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_THREAD_ID;
		filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_EQUAL;
		filter_list[i].list_filter_item.rule.key_value.integer_type_value = thread_id;
		i++;
	} else if (thread_id == EMAIL_GET_MAIL_THREAD) {
		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
		filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
		i++;

		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
		filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_THREAD_ITEM_COUNT;
		filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_GREATER_THAN;
		filter_list[i].list_filter_item.rule.key_value.integer_type_value = 0;
		i++;
	}

	if (isOutbox) {
		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
		filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
		i++;

		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
		filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_SAVE_STATUS;
		filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_NOT_EQUAL;
		filter_list[i].list_filter_item.rule.key_value.integer_type_value = EMAIL_MAIL_STATUS_SEND_SCHEDULED;
		i++;
	}

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
	i++;

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_FLAGS_DELETED_FIELD;
	filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_NOT_EQUAL;
	filter_list[i].list_filter_item.rule.key_value.integer_type_value = 1;
	i++;

	if (search_data)
		_add_search_data_into_filter_list(search_data, filter_list, &i);

	debug_log("filled filter count: %d", i);

	if (sort_type != EMAIL_SORT_DATE_RECENT && sort_type != EMAIL_SORT_DATE_OLDEST)
		cnt_soring_rule = 2;
	else
		cnt_soring_rule = 1;

	sorting_rule_list = calloc(cnt_soring_rule, sizeof(email_list_sorting_rule_t));
	if (!sorting_rule_list) {
		email_engine_free_list_filter(&filter_list, cnt_filter_list);
		debug_error("sorting_rule_list is NULL - allocation memory failed");
		return NULL;
	}

	_make_sorting_rule_list(sort_type, account_id, sorting_rule_list);

	email_engine_get_mail_list(filter_list, cnt_filter_list, sorting_rule_list, cnt_soring_rule, -1, -1, &mail_list, mail_count);

	FREE(sorting_rule_list);
	email_engine_free_list_filter(&filter_list, cnt_filter_list);

	debug_log("mail_count(%d)", *mail_count);

	return mail_list;
}

static email_mail_list_item_t *_mailbox_get_priority_sender_mail_list(int sort_type, int thread_id, const email_search_data_t *search_data, int *mail_count)
{
	debug_log("sort_type: %d", sort_type);

	int j = 0;
	email_mail_list_item_t *mail_list = NULL;
	email_list_filter_t *filter_list = NULL;
	email_list_sorting_rule_t *sorting_rule_list = NULL;
	int cnt_filter_list = 5;
	int cnt_soring_rule = 0;

	if (thread_id > 0 || thread_id == EMAIL_GET_MAIL_THREAD)
		cnt_filter_list += 2;

	if (search_data)
		cnt_filter_list += _get_filter_cnt_for_search_data(search_data);

	filter_list = calloc(cnt_filter_list, sizeof(email_list_filter_t));
	retvm_if(!filter_list, NULL, "filter_list is NULL!");

	filter_list[j].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[j].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_TAG_ID;
	filter_list[j].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[j].list_filter_item.rule.key_value.integer_type_value = PRIORITY_SENDER_TAG_ID;
	j++;

	filter_list[j].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[j].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
	j++;

	filter_list[j].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[j].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_MAILBOX_TYPE;
	filter_list[j].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[j].list_filter_item.rule.key_value.integer_type_value = EMAIL_MAILBOX_TYPE_INBOX;
	j++;

	if (thread_id > 0) {
		filter_list[j].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
		filter_list[j].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
		j++;

		filter_list[j].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
		filter_list[j].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_THREAD_ID;
		filter_list[j].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_EQUAL;
		filter_list[j].list_filter_item.rule.key_value.integer_type_value = thread_id;
		j++;
	} else if (thread_id == EMAIL_GET_MAIL_THREAD) {
		filter_list[j].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
		filter_list[j].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
		j++;

		filter_list[j].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
		filter_list[j].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_THREAD_ITEM_COUNT;
		filter_list[j].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_GREATER_THAN;
		filter_list[j].list_filter_item.rule.key_value.integer_type_value = 0;
		j++;
	}

	filter_list[j].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[j].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
	j++;

	filter_list[j].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[j].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_FLAGS_DELETED_FIELD;
	filter_list[j].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_NOT_EQUAL;
	filter_list[j].list_filter_item.rule.key_value.integer_type_value = 1;
	j++;

	if (search_data)
		_add_search_data_into_filter_list(search_data, filter_list, &j);

	debug_log("filled filter count: %d", j);

	if (sort_type != EMAIL_SORT_DATE_RECENT && sort_type != EMAIL_SORT_DATE_OLDEST)
		cnt_soring_rule = 2;
	else
		cnt_soring_rule = 1;

	sorting_rule_list = calloc(cnt_soring_rule, sizeof(email_list_sorting_rule_t));
	if (!sorting_rule_list) {
		email_engine_free_list_filter(&filter_list, cnt_filter_list);
		debug_error("sorting_rule_list is NULL - allocation memory failed");
		return NULL;
	}

	_make_sorting_rule_list(sort_type, 0, sorting_rule_list);

	email_engine_get_mail_list(filter_list, j, sorting_rule_list, cnt_soring_rule, -1, -1, &mail_list, mail_count);

	FREE(sorting_rule_list);
	email_engine_free_list_filter(&filter_list, cnt_filter_list);

	debug_log("mail_count(%d)", *mail_count);

	return mail_list;
}

static email_mail_list_item_t *_mailbox_get_favourite_mail_list(int sort_type, int thread_id, const email_search_data_t *search_data, int *mail_count)
{
	debug_log("sort_type: %d, thread_id: %d", sort_type, thread_id);

	email_mail_list_item_t *mail_list = NULL;
	email_list_filter_t *filter_list = NULL;
	email_list_sorting_rule_t *sorting_rule_list = NULL;
	int cnt_filter_list = 0;
	int cnt_soring_rule = 0;
	int i = 0;

	cnt_filter_list += 9;

	if (thread_id > 0 || thread_id == EMAIL_GET_MAIL_THREAD)
		cnt_filter_list += 2;

	if (search_data)
		cnt_filter_list += _get_filter_cnt_for_search_data(search_data);

	debug_log("cnt_filter_list: %d", cnt_filter_list);

	filter_list = malloc(sizeof(email_list_filter_t)*cnt_filter_list);
	retvm_if(!filter_list, NULL, "filter_list memory alloc failed");
	memset(filter_list, 0, sizeof(email_list_filter_t)*cnt_filter_list);

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_LEFT_PARENTHESIS;
	i++;

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_FLAGS_FLAGGED_FIELD;
	filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[i].list_filter_item.rule.key_value.integer_type_value = EMAIL_FLAG_TASK_STATUS_ACTIVE;
	i++;

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_OR;
	i++;

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_FLAGS_FLAGGED_FIELD;
	filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[i].list_filter_item.rule.key_value.integer_type_value = EMAIL_FLAG_FLAGED;
	i++;

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_RIGHT_PARENTHESIS;
	i++;

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
	i++;

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_FLAGS_DELETED_FIELD;
	filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_NOT_EQUAL;
	filter_list[i].list_filter_item.rule.key_value.integer_type_value = 1;
	i++;

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
	i++;

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_MAILBOX_TYPE;
	filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_NOT_EQUAL;
	filter_list[i].list_filter_item.rule.key_value.integer_type_value = EMAIL_MAILBOX_TYPE_TRASH;
	i++;

	if (thread_id > 0) {
		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
		filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
		i++;

		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
		filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_THREAD_ID;
		filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_EQUAL;
		filter_list[i].list_filter_item.rule.key_value.integer_type_value = thread_id;
		i++;
	} else if (thread_id == EMAIL_GET_MAIL_THREAD) {
		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
		filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
		i++;

		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
		filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_THREAD_ITEM_COUNT;
		filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_GREATER_THAN;
		filter_list[i].list_filter_item.rule.key_value.integer_type_value = 0;
		i++;
	}

	if (search_data)
		_add_search_data_into_filter_list(search_data, filter_list, &i);

	debug_log("filled filter count: %d", i);

	if (sort_type != EMAIL_SORT_DATE_RECENT && sort_type != EMAIL_SORT_DATE_OLDEST)
		cnt_soring_rule = 2;
	else
		cnt_soring_rule = 1;
	sorting_rule_list = malloc(sizeof(email_list_sorting_rule_t)*cnt_soring_rule);
	if (!sorting_rule_list) {
		email_engine_free_list_filter(&filter_list, cnt_filter_list);
		debug_error("sorting_rule_list is NULL - allocation memory failed");
		return NULL;
	}
	memset(sorting_rule_list, 0, sizeof(email_list_sorting_rule_t)*cnt_soring_rule);

	_make_sorting_rule_list(sort_type, 0, sorting_rule_list);

	email_engine_get_mail_list(filter_list, cnt_filter_list, sorting_rule_list, cnt_soring_rule, -1, -1, &mail_list, mail_count);

	FREE(sorting_rule_list);
	email_engine_free_list_filter(&filter_list, cnt_filter_list);

	debug_log("mail_count(%d)", *mail_count);

	return mail_list;
}

static email_mail_list_item_t *_mailbox_get_mail_list_for_search_all_folders(int account_id, int sort_type, const email_search_data_t *search_data, int *mail_count)
{
	debug_enter();
	debug_log("account_id: %d, sort_type: %d", account_id, sort_type);

	email_mail_list_item_t *mail_list = NULL;
	email_list_filter_t *filter_list = NULL;
	email_list_sorting_rule_t *sorting_rule_list = NULL;
	int cnt_soring_rule = 0;
	int cnt_filter_list = 7;
	int i = 0;

	if (search_data ) {
		cnt_filter_list += _get_filter_cnt_for_search_data(search_data);
	}

	debug_log("cnt_filter_list: %d", cnt_filter_list);

	filter_list = malloc(sizeof(email_list_filter_t) * cnt_filter_list);
	memset(filter_list, 0, sizeof(email_list_filter_t) * cnt_filter_list);

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_ACCOUNT_ID;
	filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[i].list_filter_item.rule.key_value.integer_type_value = account_id;
	i++;

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
	i++;

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_MAILBOX_ID;
	filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_NOT_EQUAL;
	filter_list[i].list_filter_item.rule.key_value.integer_type_value = -1;
	i++;

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
	i++;

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_MAILBOX_TYPE;
	filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_NOT_EQUAL;
	filter_list[i].list_filter_item.rule.key_value.integer_type_value = EMAIL_MAILBOX_TYPE_SEARCH_RESULT;
	i++;

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
	i++;

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_FLAGS_DELETED_FIELD;
	filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_NOT_EQUAL;
	filter_list[i].list_filter_item.rule.key_value.integer_type_value = 1;
	i++;

	if (search_data) {
		_add_search_data_into_filter_list(search_data, filter_list, &i);
	}

	if (sort_type != EMAIL_SORT_DATE_RECENT && sort_type != EMAIL_SORT_DATE_OLDEST) {
		cnt_soring_rule = 2;
	} else {
		cnt_soring_rule = 1;
	}

	sorting_rule_list = malloc(sizeof(email_list_sorting_rule_t) * cnt_soring_rule);
	memset(sorting_rule_list, 0, sizeof(email_list_sorting_rule_t) * cnt_soring_rule);

	_make_sorting_rule_list(EMAIL_SORT_DATE_RECENT, account_id, sorting_rule_list);

	email_engine_get_mail_list(filter_list, cnt_filter_list, sorting_rule_list, cnt_soring_rule, -1, -1, &mail_list, mail_count);

	FREE(sorting_rule_list);
	email_engine_free_list_filter(&filter_list, cnt_filter_list);

	debug_log("mail_count(%d)", *mail_count);

	debug_leave();
	return mail_list;
}

static int _get_filter_cnt_for_search_data(const email_search_data_t *search_data)
{
	debug_enter();

	int search_filter_cnt = 0;
	if (!search_data) {
		debug_log("search_data is null");
		return 0;
	}

	if (search_data->subject) {
		search_filter_cnt += 2;
	}
	if (search_data->sender) {
		search_filter_cnt += 2;
	}
	if (search_data->recipient) {
		search_filter_cnt += 2;
	}
	if (search_data->body_text) {
		search_filter_cnt += 2;
	}
	if (search_data->attach_text) {
		search_filter_cnt += 2;
	}
	if (search_filter_cnt > 0) {
		search_filter_cnt += 2;
	}

	debug_log("search_filter_cnt: %d", search_filter_cnt);

	return search_filter_cnt;

}

static void _add_search_data_into_filter_list(const email_search_data_t *search_data, email_list_filter_t *filter_list, int *current_index)
{
	debug_enter();

	int i = *current_index;

	if (!search_data) {
		debug_warning("search_data is NULL");
		return;
	}

	int filter_rule_cnt = 0;
	if (search_data->subject || search_data->sender || search_data->recipient || search_data->body_text || search_data->attach_text) {
		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
		filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
		i++;

		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
		filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_LEFT_PARENTHESIS;
		filter_list[i].list_filter_item.rule.case_sensitivity = EMAIL_CASE_INSENSITIVE;
		i++;
	}
	if (search_data->subject) {
		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
		filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_SUBJECT;
		filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_INCLUDE;
		filter_list[i].list_filter_item.rule.case_sensitivity = EMAIL_CASE_INSENSITIVE;
		filter_list[i].list_filter_item.rule.key_value.string_type_value = strdup(search_data->subject);
		i++;
		filter_rule_cnt++;
	}
	if (search_data->sender) {
		if (filter_rule_cnt > 0) {
			filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
			filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_OR;
			i++;
		}

		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
		filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_FROM;
		filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_INCLUDE;
		filter_list[i].list_filter_item.rule.case_sensitivity = EMAIL_CASE_INSENSITIVE;
		filter_list[i].list_filter_item.rule.key_value.string_type_value = strdup(search_data->sender);
		i++;
		filter_rule_cnt++;
	}
	if (search_data->recipient) {
		if (filter_rule_cnt > 0) {
			filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
			filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_OR;
			i++;
		}

		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
		filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_TO;
		filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_INCLUDE;
		filter_list[i].list_filter_item.rule.case_sensitivity = EMAIL_CASE_INSENSITIVE;
		filter_list[i].list_filter_item.rule.key_value.string_type_value = strdup(search_data->recipient);
		i++;
		filter_rule_cnt++;
	}
	if (search_data->body_text) {
		if (filter_rule_cnt > 0) {
			filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
			filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_OR;
			i++;
		}

		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE_FTS;
		filter_list[i].list_filter_item.rule_fts.target_attribute = EMAIL_MAIL_TEXT_ATTRIBUTE_FULL_TEXT;
		filter_list[i].list_filter_item.rule_fts.rule_type = EMAIL_LIST_FILTER_RULE_INCLUDE;
		filter_list[i].list_filter_item.rule_fts.key_value.string_type_value = strdup(search_data->body_text);
		i++;
		filter_rule_cnt++;
	}

	if (search_data->attach_text) {
		if (filter_rule_cnt > 0) {
			filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
			filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_OR;
			i++;
		}

		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE_ATTACH;
		filter_list[i].list_filter_item.rule_attach.target_attribute = EMAIL_MAIL_ATTACH_ATTRIBUTE_ATTACHMENT_NAME;
		filter_list[i].list_filter_item.rule_attach.rule_type =  EMAIL_LIST_FILTER_RULE_INCLUDE;
		filter_list[i].list_filter_item.rule_attach.case_sensitivity = EMAIL_CASE_INSENSITIVE;
		filter_list[i].list_filter_item.rule_attach.key_value.string_type_value = strdup(search_data->attach_text);
		i++;
		filter_rule_cnt++;
	}

	if (filter_rule_cnt > 0) {
		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
		filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_RIGHT_PARENTHESIS;
		i++;
	}

	*current_index = i;

	debug_leave();
}

static void _make_sorting_rule_list(email_sort_type_e sort_type, int account_id, email_list_sorting_rule_t *sorting_rule_list)
{
	debug_enter();

	switch (sort_type) {
	case EMAIL_SORT_DATE_RECENT:
		sorting_rule_list[0].target_attribute = EMAIL_MAIL_ATTRIBUTE_DATE_TIME;
		sorting_rule_list[0].sort_order = EMAIL_SORT_ORDER_DESCEND;
		sorting_rule_list[0].force_boolean_check = false;
		break;
	case EMAIL_SORT_DATE_OLDEST:
		sorting_rule_list[0].target_attribute = EMAIL_MAIL_ATTRIBUTE_DATE_TIME;
		sorting_rule_list[0].sort_order = EMAIL_SORT_ORDER_ASCEND;
		sorting_rule_list[0].force_boolean_check = false;
		break;
	case EMAIL_SORT_SENDER_ATOZ:
		sorting_rule_list[0].target_attribute = EMAIL_MAIL_ATTRIBUTE_FROM;
		sorting_rule_list[0].sort_order = EMAIL_SORT_ORDER_LOCALIZE_ASCEND;
		sorting_rule_list[0].force_boolean_check = false;
		break;
	case EMAIL_SORT_SENDER_ZTOA:
		sorting_rule_list[0].target_attribute = EMAIL_MAIL_ATTRIBUTE_FROM;
		sorting_rule_list[0].sort_order = EMAIL_SORT_ORDER_LOCALIZE_DESCEND;
		sorting_rule_list[0].force_boolean_check = false;
		break;
	case EMAIL_SORT_RCPT_ATOZ:
		sorting_rule_list[0].target_attribute = EMAIL_MAIL_ATTRIBUTE_TO;
		sorting_rule_list[0].sort_order = EMAIL_SORT_ORDER_LOCALIZE_ASCEND;
		sorting_rule_list[0].force_boolean_check = false;
		break;
	case EMAIL_SORT_RCPT_ZTOA:
		sorting_rule_list[0].target_attribute = EMAIL_MAIL_ATTRIBUTE_TO;
		sorting_rule_list[0].sort_order = EMAIL_SORT_ORDER_LOCALIZE_DESCEND;
		sorting_rule_list[0].force_boolean_check = false;
		break;
	case EMAIL_SORT_UNREAD:
		sorting_rule_list[0].target_attribute = EMAIL_MAIL_ATTRIBUTE_FLAGS_SEEN_FIELD;
		sorting_rule_list[0].sort_order = EMAIL_SORT_ORDER_ASCEND;
		sorting_rule_list[0].force_boolean_check = false;
		break;
	case EMAIL_SORT_IMPORTANT:
		sorting_rule_list[0].target_attribute = EMAIL_MAIL_ATTRIBUTE_FLAGS_FLAGGED_FIELD;
		sorting_rule_list[0].sort_order = EMAIL_SORT_ORDER_DESCEND;
		sorting_rule_list[0].force_boolean_check = false;
		break;
	case EMAIL_SORT_PRIORITY:
		sorting_rule_list[0].target_attribute = EMAIL_MAIL_ATTRIBUTE_PRIORITY;
		sorting_rule_list[0].sort_order = EMAIL_SORT_ORDER_ASCEND;
		sorting_rule_list[0].force_boolean_check = false;
		break;
	case EMAIL_SORT_ATTACHMENTS:
		sorting_rule_list[0].target_attribute = EMAIL_MAIL_ATTRIBUTE_ATTACHMENT_COUNT;
		sorting_rule_list[0].sort_order = EMAIL_SORT_ORDER_ASCEND;
		sorting_rule_list[0].force_boolean_check = true;
		break;

	case EMAIL_SORT_TO_CC_BCC:
		{
			email_account_t *account = NULL;

			sorting_rule_list[0].target_attribute = EMAIL_MAIL_ATTRIBUTE_RECIPIENT_ADDRESS;

			if (account_id == 0) {
				sorting_rule_list[0].sort_order = EMAIL_SORT_ORDER_TO_CCBCC_ALL;
			} else {
				if (email_engine_get_account_data(account_id, EMAIL_ACC_GET_OPT_DEFAULT, &account)) {
					if (account->user_email_address)
						sorting_rule_list[0].key_value.string_type_value = g_strdup(account->user_email_address);
					email_engine_free_account_list(&account, 1);
				}

				sorting_rule_list[0].sort_order = EMAIL_SORT_ORDER_TO_CCBCC;
			}
		}
		break;
	case EMAIL_SORT_SUBJECT_ATOZ:
		sorting_rule_list[0].target_attribute = EMAIL_MAIL_ATTRIBUTE_SUBJECT;
		sorting_rule_list[0].sort_order = EMAIL_SORT_ORDER_LOCALIZE_ASCEND;
		sorting_rule_list[0].force_boolean_check = false;
		break;
	case EMAIL_SORT_SUBJECT_ZTOA:
		sorting_rule_list[0].target_attribute = EMAIL_MAIL_ATTRIBUTE_SUBJECT;
		sorting_rule_list[0].sort_order = EMAIL_SORT_ORDER_LOCALIZE_DESCEND;
		sorting_rule_list[0].force_boolean_check = false;
		break;
	case EMAIL_SORT_SIZE_SMALLEST:
		sorting_rule_list[0].target_attribute = EMAIL_MAIL_ATTRIBUTE_MAIL_SIZE;
		sorting_rule_list[0].sort_order = EMAIL_SORT_ORDER_ASCEND;
		sorting_rule_list[0].force_boolean_check = false;
		break;

	case EMAIL_SORT_SIZE_LARGEST:
		sorting_rule_list[0].target_attribute = EMAIL_MAIL_ATTRIBUTE_MAIL_SIZE;
		sorting_rule_list[0].sort_order = EMAIL_SORT_ORDER_DESCEND;
		sorting_rule_list[0].force_boolean_check = false;
		break;


	default:
		debug_warning("INVALID sort_type.");
		return;
	}

	if (sort_type != EMAIL_SORT_DATE_RECENT && sort_type != EMAIL_SORT_DATE_OLDEST) {
		sorting_rule_list[1].target_attribute = EMAIL_MAIL_ATTRIBUTE_DATE_TIME;
		sorting_rule_list[1].sort_order = EMAIL_SORT_ORDER_DESCEND;
		sorting_rule_list[1].force_boolean_check = false;
	}
}

static void _mailbox_make_list(EmailMailboxView *view, const email_search_data_t *search_data)
{
	debug_enter();

	email_mail_list_item_t *mail_list = NULL;
	int mail_count = 0;

	debug_log("mode: %d, mailbox_type: %d", view->mode, view->mailbox_type);

	mail_list = _mailbox_get_mail_list(view, search_data, &mail_count);

	if (mail_list) {
		/* Display last updated time for individual accounts */
		if (view->mode == EMAIL_MAILBOX_MODE_MAILBOX
				&& view->only_local == FALSE && !view->b_searchmode) {
			mailbox_last_updated_time_item_add(view, false);
		}

		_mailbox_list_insert_n_mails(view, mail_list, MIN(mail_count, FIRST_BLOCK_SIZE), search_data);
		debug_log("COUNT: %d", mail_count);

		/* add remaining items */
		_mailbox_make_remaining_items(view, search_data, mail_list, mail_count);

		if (view->b_searchmode) {
			mailbox_hide_no_contents_view(view);
		} else {
			if ((view->mode == EMAIL_MAILBOX_MODE_ALL || view->mode == EMAIL_MAILBOX_MODE_MAILBOX)
				&& view->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX && mail_count > 0) {
				bool sending_mail_exist = false;
				GList *cur = NULL;
				MailItemData *ld = NULL;
				G_LIST_FOREACH(view->mail_list, cur, ld) {
					if (ld->mail_status == EMAIL_MAIL_STATUS_SENDING) {
						sending_mail_exist = true;
						break;
					}
				}

				if (!sending_mail_exist) {
					mailbox_send_all_btn_add(view);
				}
			}

			if ((view->mode == EMAIL_MAILBOX_MODE_MAILBOX && view->only_local == false
					&& view->mailbox_type != EMAIL_MAILBOX_TYPE_OUTBOX
					&& view->account_type == EMAIL_SERVER_TYPE_IMAP4)
				|| (view->mode == EMAIL_MAILBOX_MODE_MAILBOX && view->only_local == false
					&& view->mailbox_type == EMAIL_MAILBOX_TYPE_INBOX
					&& view->account_type == EMAIL_SERVER_TYPE_POP3)) {
				/* since there is a delay in receiving noti from email-service,creating more progress item directly. */
				mailbox_load_more_messages_item_add(view);
			} else if (view->mailbox_type != EMAIL_MAILBOX_TYPE_OUTBOX) {
				mailbox_no_more_emails_item_add(view);
			}

			mailbox_hide_no_contents_view(view);
		}
	} else {
		debug_log("no email exists.");

		if (view->b_searchmode) {
			mailbox_show_no_contents_view(view);
		} else {
			if (view->b_editmode)
				mailbox_exit_edit_mode(view);
			mailbox_show_no_contents_view(view);
		}
	}

	debug_leave();
}

static void _mailbox_make_remaining_items(EmailMailboxView *view, const email_search_data_t *search_data,
		email_mail_list_item_t *mail_list, int mail_count)
{
	debug_enter();

	email_search_data_t *search_data_clone = NULL;
	AddRemainingMailReqData *req = NULL;

	while (true) {
		if (mail_count <= FIRST_BLOCK_SIZE) {
			break;
		}

		if (view->b_searchmode && search_data)
		{
			search_data_clone = _clone_search_data(search_data);
			if (!search_data_clone) {
				debug_error("failure in memory allocation.");
				break;
			}
		}

		req = MEM_ALLOC(req, 1);
		if (!req) {
			debug_error("failure in memory allocation.");
			break;
		}

		*req = (AddRemainingMailReqData) {mail_list, FIRST_BLOCK_SIZE, mail_count,
			search_data_clone, view};

		if (view->base.state != EV_STATE_ACTIVE) {
			debug_log("wait for active state...");
			view->remaining_req = req;
		} else {
			mailbox_list_make_remaining_items_in_thread(view, req);
		}

		return;
	}

	mailbox_free_mailbox_search_data(search_data_clone);
	free(mail_list);
	free(req);

	debug_leave();
}

static void _mailbox_clear_list(EmailMailboxView *view)
{
	debug_enter();

	if (view == NULL) {
		debug_warning("view is NULL");
		return;
	}

	debug_log("view->mail_list[%p]", view->mail_list);

	mailbox_cancel_all_requests(view);

	if (view->b_editmode)
		mailbox_exit_edit_mode(view);

	mailbox_get_more_progress_item_remove(view);
	mailbox_load_more_messages_item_remove(view);
	mailbox_no_more_emails_item_remove(view);
	mailbox_send_all_btn_remove(view);
	mailbox_select_all_item_remove(view);
	mailbox_last_updated_time_item_remove(view);
	mailbox_remove_refresh_progress_bar(view);

	debug_log("view->gl: 0x%x", view->gl);
	if (view->gl) {
		elm_genlist_clear(view->gl);
	} else {
		debug_warning("view->gl is null");
		return;
	}

	mailbox_list_free_all_item_data(view);

	debug_leave();
}

static void _mailbox_free_req_data(AddRemainingMailReqData **req)
{
	if (req && *req) {
		mailbox_free_mailbox_search_data((*req)->search_data);
		free((*req)->mail_list);
		free(*req);
		*req = NULL;
	}
}

static email_search_data_t *_clone_search_data(const email_search_data_t *search_data)
{
	debug_enter();
	retvm_if(!search_data, NULL, "search_data is NULL");

	email_search_data_t *search_data_clone = calloc(1, sizeof(email_search_data_t));
	retvm_if(!search_data_clone, NULL, "search_data_clone memory alloc failed");

	search_data_clone->search_type = search_data->search_type;
	if (search_data->body_text)
		search_data_clone->body_text = g_strdup(search_data->body_text);
	if (search_data->subject)
		search_data_clone->subject = g_strdup(search_data->subject);
	if (search_data->sender)
		search_data_clone->sender = g_strdup(search_data->sender);
	if (search_data->recipient)
		search_data_clone->recipient = g_strdup(search_data->recipient);

	search_data_clone->from_time = search_data->from_time;
	search_data_clone->to_time = search_data->to_time;

	return search_data_clone;
}

static void _mailbox_system_font_change_cb(system_settings_key_e key, void *data)
{
	debug_enter();
	retm_if(!data, "data si NULL");

	EmailMailboxView *view = data;

	mailbox_refresh_fullview(view, true);
}

static char *_mailbox_get_ricipient_display_string(int mail_id)
{
	debug_enter();
	email_mail_data_t *mail_data = NULL;
	char *recipient_str = NULL;
	char *display_string = NULL;
	int total_cnt = 0;

	if (email_engine_get_mail_data(mail_id, &mail_data)) {

		if (STR_VALID(mail_data->full_address_to)) {
			_mailbox_get_recipient_display_information(mail_data->full_address_to, &recipient_str, &total_cnt, false);
		}

		if (STR_VALID(mail_data->full_address_cc)) {
			int cc_cnt = 0;
			if (recipient_str) {
				_mailbox_get_recipient_display_information(mail_data->full_address_cc, NULL, &cc_cnt, true);
			} else {
				_mailbox_get_recipient_display_information(mail_data->full_address_cc, &recipient_str, &cc_cnt, false);
			}
			total_cnt = total_cnt + cc_cnt;
		}

		if (STR_VALID(mail_data->full_address_bcc)) {
			int bcc_cnt = 0;
			if (recipient_str) {
				_mailbox_get_recipient_display_information(mail_data->full_address_bcc, NULL, &bcc_cnt, true);
			} else {
				_mailbox_get_recipient_display_information(mail_data->full_address_bcc, &recipient_str, &bcc_cnt, false);
			}
			total_cnt = total_cnt + bcc_cnt;
		}

		if (recipient_str) {
			if (total_cnt > 1) {
				char *temp = g_strdup_printf("%s + %d", recipient_str, total_cnt-1);
				display_string = STRNDUP(temp, RECIPIENT_LEN - 1);
				G_FREE(temp);
			} else {
				display_string = STRNDUP(recipient_str, RECIPIENT_LEN - 1);
			}
			FREE(recipient_str);
		} else if (!STR_VALID(recipient_str) && STR_VALID(mail_data->alias_recipient) && mail_data->message_class == EMAIL_MESSAGE_CLASS_SMS) {
			display_string = STRNDUP(mail_data->alias_recipient, RECIPIENT_LEN - 1);
			debug_secure("alias_recipient :%s, %s", mail_data->alias_recipient, mail_data->full_address_to);
		}

		email_engine_free_mail_data_list(&mail_data, 1);
	}

	return display_string;

}

static void _mailbox_get_recipient_display_information(const gchar *addr_list, char **recipient_str, int *recipent_cnt, bool count_only)
{
	debug_enter();
	RETURN_IF_FAIL(STR_VALID(addr_list));

	debug_secure("addr list (%s)", addr_list);

	int count = 0;
	char *display_address = NULL;

	guint index = 0;
	gchar *recipient = g_strdup(addr_list);
	recipient = g_strstrip(recipient);
	gchar **recipient_list = g_strsplit_set(recipient, ";", -1);
	G_FREE(recipient);

	while ((recipient_list[index] != NULL) && (strlen(recipient_list[index]) > 0)) {
		recipient_list[index] = g_strstrip(recipient_list[index]);
		gchar *nick = NULL;
		gchar *addr = NULL;
		if (!email_get_recipient_info(recipient_list[index], &nick, &addr)) {
			debug_log("email_get_recipient_info failed");
			addr = g_strdup(recipient_list[index]);
		}

		if (!email_get_address_validation(addr)) {
			++index;
			G_FREE(nick);
			G_FREE(addr);
			continue;
		}

		if (!count_only && !display_address) {
			if (nick) {
				display_address = g_strdup(nick);
			} else if (addr) {
				display_address = g_strdup(addr);
			}
		}
		count++;

		++index;
		G_FREE(nick);
		G_FREE(addr);
	}

	g_strfreev(recipient_list);

	*recipent_cnt = count;
	if (!count_only && recipient_str) {
		*recipient_str = display_address;
	}
	debug_secure("addr display_address : %s, recipent_cnt : %d, count_only : %d", display_address, count, count_only);

}

static void _mailbox_list_insert_n_mails(EmailMailboxView *view, email_mail_list_item_t *mail_list, int count, const email_search_data_t *search_data)
{
	debug_enter();

	retm_if(!view, "view is NULL");
	retm_if(!mail_list, "mail_list is NULL");

	int i = 0;
	for (; i < count; i++) {
		MailItemData *ld = mailbox_list_make_mail_item_data(mail_list + i, search_data, view);
		if (!ld) {
			continue;
		}

		mailbox_list_insert_mail_item(ld, view);
	}

	debug_leave();
}

static void _mailbox_check_cache_init(EmailMailboxCheckCache *cache, const char *obj_style)
{
	cache->obj_style = obj_style;
	cache->free_index = 0;
	cache->used_index = EMAIL_MAILBOX_CHECK_CACHE_SIZE;
}

static void _mailbox_check_cache_free(EmailMailboxCheckCache *cache)
{
	int i = 0;
	int free_index = cache->free_index;
	int used_index = cache->used_index;

	for (i = 0; i < free_index; ++i) {
		evas_object_unref(cache->items[i]);
		evas_object_del(cache->items[i]);
	}

	if (used_index < EMAIL_MAILBOX_CHECK_CACHE_SIZE) {
		debug_warning("used_index: %d", used_index);
		for (i = used_index; i < EMAIL_MAILBOX_CHECK_CACHE_SIZE; ++i) {
			evas_object_unref(cache->items[i]);
		}
	}

	cache->free_index = 0;
	cache->used_index = EMAIL_MAILBOX_CHECK_CACHE_SIZE;
}

static Evas_Object *_mailbox_check_cache_get_obj(EmailMailboxCheckCache *cache, Evas_Object *parent)
{
	Evas_Object *result = NULL;

	int free_index = cache->free_index;
	int used_index = cache->used_index;

	if (free_index > 0) {
		--free_index;
		--used_index;
		result = cache->items[free_index];
		cache->items[used_index] = result;
		cache->free_index = free_index;
		cache->used_index = used_index;
	} else {

		result = elm_check_add(parent);
		elm_object_style_set(result, cache->obj_style);
		evas_object_repeat_events_set(result, EINA_FALSE);
		evas_object_propagate_events_set(result, EINA_FALSE);

		debug_log("New object was spawned: %p", result);

		if (used_index > 0) {
			--used_index;
			cache->items[used_index] = result;
			cache->used_index = used_index;

			evas_object_ref(result);
		} else {
			debug_warning("Cache is full!");
		}
	}

	evas_object_show(result);

	return result;
}

static void _mailbox_check_cache_release_obj(EmailMailboxCheckCache *cache, Evas_Object *obj)
{
	int i = cache->used_index;

	for (; i < EMAIL_MAILBOX_CHECK_CACHE_SIZE; ++i) {
		if (cache->items[i] == obj) {
			int free_index = cache->free_index;
			int used_index = cache->used_index;

			cache->items[free_index] = obj;
			cache->items[i] = cache->items[used_index];
			++free_index;
			++used_index;
			cache->free_index = free_index;
			cache->used_index = used_index;

			evas_object_hide(obj);
			return;
		}
	}

	debug_warning("Object not found in cache!");
}

static char *_mailbox_get_cashed_folder_name(EmailMailboxView *view, email_mailbox_type_e mailbox_type, int mailbox_id)
{
	debug_enter();
	char *mailbox_alias = mailbox_get_mailbox_folder_search_name_by_mailbox_type(mailbox_type);

	if (!mailbox_alias) {
		mailbox_alias = mailbox_folders_name_cashe_get_name(view, mailbox_id);
		if (!mailbox_alias) {
			mailbox_alias = mailbox_get_mailbox_folder_search_name_by_mailbox_id(mailbox_id);
			mailbox_folders_name_cashe_add_name(view, mailbox_id, mailbox_alias);
		}
	}

	return mailbox_alias;
}

static int _mailbox_folder_name_cashe_comparator_cb(const void *data1, const void *data2)
{
	const EmailFolderNameCashItem *item1 = data1;
	const EmailFolderNameCashItem *item2 = data2;

	if (item1->mailbox_id > item2->mailbox_id) {
		return 1;
	} else if (item1->mailbox_id < item2->mailbox_id) {
		return -1;
	} else {
		return 0;
	}
}

/*
 * Definition for exported functions
 */

void mailbox_list_create_view(EmailMailboxView *view)
{
	debug_enter();
	retm_if(!view, "view is NULL");
	debug_log("mailbox_type[%d]", view->mailbox_type);

	if (view->gl) {
		elm_genlist_clear(view->gl);
		evas_object_del(view->gl);
		view->gl = NULL;
	}

	Evas_Object *gl = elm_genlist_add(view->content_layout);
	if (view->theme) {
		elm_object_theme_set(gl, view->theme);
	}

	debug_log("genlist = %p", gl);
	evas_object_smart_callback_add(gl, "realized", _realized_item_cb, view);
	evas_object_smart_callback_add(gl, "unhighlighted", _realized_item_cb, view);
	evas_object_smart_callback_add(gl, "highlighted", _pressed_item_cb, view);

	elm_scroller_policy_set(gl, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
	elm_genlist_block_count_set(gl, BLOCK_COUNT);
	elm_genlist_homogeneous_set(gl, EINA_TRUE);
	elm_genlist_mode_set(gl, ELM_LIST_COMPRESS);

	evas_object_size_hint_align_set(gl, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(gl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	evas_object_show(gl);
	view->gl = gl;

	view->selected_mail_list = eina_list_free(view->selected_mail_list);
	view->selected_group_list = eina_list_free(view->selected_group_list);
	mailbox_refresh_flicks_cb_register(view);

	_mailbox_check_cache_init(&view->star_check_cache, "favorite");
	_mailbox_check_cache_init(&view->select_check_cache, "default");

	evas_object_event_callback_add(gl, EVAS_CALLBACK_MOUSE_DOWN, mailbox_set_main_thread_busy_cb, NULL);
	evas_object_event_callback_add(gl, EVAS_CALLBACK_MOUSE_UP, mailbox_reset_main_thread_busy_cb, NULL);
	evas_object_event_callback_add(gl, EVAS_CALLBACK_FREE, _mailbox_gl_free_cb, view);

	return;
}

void mailbox_list_open_email_viewer(MailItemData *ld)
{
	debug_enter();
	retm_if(!ld, "ld == NULL");
	retm_if(ld->mail_id <= 0, "mail_id(%d) <= 0", ld->mail_id);

	EmailMailboxView *view = ld->view;
	retm_if(!view, "view == NULL");

	mailbox_open_email_viewer(view, ld->account_id, ld->mailbox_id, ld->mail_id);
}

void mailbox_list_system_settings_callback_register(EmailMailboxView *view)
{
	debug_enter();

	int ret = -1;
	ret = email_register_accessibility_font_size_changed_callback(_mailbox_system_font_change_cb, view);
	if (ret == -1)
		debug_error("email_register_accessibility_font_size_changed_callback failed");

}

void mailbox_list_system_settings_callback_unregister()
{
	debug_enter();

	int ret = -1;
	ret = email_unregister_accessibility_font_size_changed_callback(_mailbox_system_font_change_cb);
	if (ret == -1)
		debug_error("email_unregister_accessibility_font_size_changed_callback failed");
}

void mailbox_list_refresh(EmailMailboxView *view, const email_search_data_t *search_data)
{
	debug_enter();
	_mailbox_clear_list(view);
	_mailbox_make_list(view, search_data);

	debug_leave();
}

void mailbox_list_insert_mail_item(MailItemData *ld, EmailMailboxView *view)
{
	debug_enter();

	view->mail_list = g_list_append(view->mail_list, ld);

	_insert_mail_item_to_list(view, ld);
}

void mailbox_list_insert_mail_item_from_noti(MailItemData **ld_ptr, EmailMailboxView *view)
{
	debug_enter();
	if (*ld_ptr == NULL)
		return;
	MailItemData *ld = *ld_ptr; /* ld variable used for better readability. */

	if (mailbox_list_get_mail_item_data_by_mailid(ld->mail_id, view->mail_list)) {
		debug_log("this mail(%d) is already inserted in the current list.", ld->mail_id);
		mailbox_list_free_mail_item_data(ld);
		*ld_ptr = NULL;		/* *ld_ptr should be assigned to NULL as mailbox_list_free_mail_item_data frees the memory that *ld_ptr holds*/
		return;
	}

	if (view->b_searchmode) {
		if (mailbox_check_searched_mail(ld->mail_id, view)) {
			debug_log("While searching, mail is inserted.");
		} else {
			mailbox_list_free_mail_item_data(ld);
			*ld_ptr = NULL;		/* *ld_ptr should be assigned to NULL as mailbox_list_free_mail_item_data frees the memory that *ld_ptr holds */
			debug_log("This mail is not matched with current search condition!!");
			return;
		}
	}

	view->mail_list = g_list_insert_sorted(view->mail_list, ld, _compare_mail_item_order);

	GList *cur = g_list_find(view->mail_list, ld);
	retm_if(!cur, "No such ld");

	GList *prev = g_list_previous(cur);
	MailItemData *prev_data = prev ? g_list_nth_data(prev, 0) : NULL;
	GList *next = g_list_next(cur);
	MailItemData *next_data = next ? g_list_nth_data(next, 0) : NULL;

	_insert_mail_item_to_list_from_noti(view, ld, prev_data, next_data);

}

void mailbox_list_remove_mail_item(EmailMailboxView *view, MailItemData *ld)
{
	debug_enter();
	if (ld == NULL || ld->base.item == NULL) {
		debug_warning("ld == NULL or ld->item == NULL");
		return;
	}

	elm_object_item_del(ld->base.item);
	view->mail_list = g_list_remove(view->mail_list, ld);

	if (view->selected_mail_list) {
		view->selected_mail_list = eina_list_remove(view->selected_mail_list, ld);
		debug_log("Remove from view->selected_mail_list, ld->mail_id : %d", ld->mail_id);
	}

	mailbox_list_free_mail_item_data(ld);
}

MailItemData *mailbox_list_make_mail_item_data(email_mail_list_item_t *mail_info, const email_search_data_t *search_data, EmailMailboxView *view)
{
	retvm_if(!mail_info, NULL, "mail_info is NULL");

	MailItemData *ld = MEM_ALLOC(ld, 1);
	retvm_if(!ld, NULL, "memory allocarion failed.");

	gchar buf[MAX_STR_LEN] = { 0, };
	int preview_len = EMAIL_MAILBOX_PREVIEW_TEXT_DEFAULT_CHAR_COUNT;
	debug_log("account_id(%d), mail_id(%d), preview_len(%d)", mail_info->account_id, mail_info->mail_id, preview_len);

	ld->base.item_type = MAILBOX_LIST_ITEM_MAIL_BRIEF;
	ld->base.item = NULL;
	/* info field */
	ld->mail_id = mail_info->mail_id;
	ld->mail_size = mail_info->mail_size;
	ld->thread_id = mail_info->thread_id;
	ld->thread_count = mail_info->thread_item_count;
	ld->is_seen = mail_info->flags_seen_field;
	ld->is_attachment = mail_info->attachment_count;
	ld->priority = mail_info->priority;
	ld->is_body_download = mail_info->body_download_status;
	ld->account_id = mail_info->account_id;
	ld->flag_type = mail_info->flags_flagged_field;
	ld->mail_status = mail_info->save_status;
	ld->mailbox_id = mail_info->mailbox_id;
	ld->mailbox_type = mail_info->mailbox_type;
	ld->reply_flag = mail_info->flags_answered_field;
	ld->forward_flag = mail_info->flags_forwarded_field;
	ld->view = view;
	ld->checked = EINA_FALSE;
	ld->is_highlited = EINA_FALSE;

	/*Folder name for search mode*/
	if (view->b_searchmode && search_data && search_data->search_type == EMAIL_SEARCH_IN_ALL_FOLDERS) {
		ld->folder_name = _mailbox_get_cashed_folder_name(view, mail_info->mailbox_type, mail_info->mailbox_id);
	} else {
		ld->folder_name = NULL;
	}

#ifndef _FEATURE_PRIORITY_SENDER_DISABLE_
	if (mail_info->tag_id == PRIORITY_SENDER_TAG_ID) {
		debug_log("this is priority sender email");
		ld->is_priority_sender_mail = true;
	}
#endif

	/* subject */
	if (STR_VALID(mail_info->subject)) {
		UTF8_VALIDATE(mail_info->subject, SUBJECT_LEN);
		memset(buf, 0, sizeof(buf));
		g_utf8_strncpy(buf, mail_info->subject, EMAIL_MAILBOX_TITLE_DEFAULT_CHAR_COUNT);
		char *_subject = elm_entry_utf8_to_markup(buf);
		int title_len = STR_LEN(_subject);
		ld->title = MEM_ALLOC_STR(title_len + TAG_LEN + 1);
		if (view->b_searchmode && search_data && STR_VALID(search_data->subject)) {
			char *_search_subject = elm_entry_utf8_to_markup(search_data->subject);
			_mail_item_data_insert_search_tag(ld->title, title_len + TAG_LEN, (const char *) _subject, _search_subject);
			FREE(_search_subject);
		} else {
			STR_NCPY(ld->title, _subject, title_len + TAG_LEN);
		}
		FREE(_subject);
	} else {
		ld->title = NULL;
	}

	/* sender */
	ld->sender = (STR_VALID(mail_info->email_address_sender)) ?
					STRNDUP(mail_info->email_address_sender, ADDR_LEN - 1) :
					STRNDUP(email_get_email_string("IDS_EMAIL_MBODY_NO_EMAIL_ADDRESS_OR_NAME"), ADDR_LEN - 1);

	UTF8_VALIDATE(mail_info->full_address_from, FROM_LEN - 1);
	char *alias = GET_ALIAS_FROM_FULL_ADDRESS(mail_info->full_address_from, mail_info->email_address_sender, FROM_LEN - 1);
	if (alias) {
		char *_alias = elm_entry_utf8_to_markup(alias);
		int alias_len = STR_LEN(_alias);
		ld->alias = MEM_ALLOC_STR(alias_len + TAG_LEN + 1);
		if (view->b_searchmode && search_data && STR_VALID(search_data->sender)) {
			char *_search_sender = elm_entry_utf8_to_markup(search_data->sender);
			_mail_item_data_insert_search_tag(ld->alias, alias_len + TAG_LEN, (const char *) _alias, _search_sender);
			FREE(_search_sender);
		} else {
			STR_NCPY(ld->alias, _alias, alias_len + TAG_LEN);
		}
		FREE(_alias);
	} else {
		ld->alias = STRNDUP(email_get_email_string("IDS_EMAIL_MBODY_NO_EMAIL_ADDRESS_OR_NAME"), ADDR_LEN - 1);
	}

	FREE(alias);

	if (ld->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX ||
		ld->mailbox_type == EMAIL_MAILBOX_TYPE_SENTBOX ||
		ld->mailbox_type == EMAIL_MAILBOX_TYPE_DRAFT) {

		char *recipient = _mailbox_get_ricipient_display_string(mail_info->mail_id);
		if (recipient) {
			char *_recipient = elm_entry_utf8_to_markup(recipient);
			int recipient_len = STR_LEN(_recipient);
			ld->recipient = MEM_ALLOC_STR(recipient_len + TAG_LEN + 1);
			if (view->b_searchmode && search_data && STR_VALID(search_data->recipient)) {
				char *_search_recipient = elm_entry_utf8_to_markup(search_data->recipient);
				_mail_item_data_insert_search_tag(ld->recipient, recipient_len + TAG_LEN, (const char *) _recipient, _search_recipient);
				FREE(_search_recipient);
			} else {
				STR_NCPY(ld->recipient, _recipient, recipient_len + TAG_LEN);
			}
			FREE(_recipient);
			FREE(recipient);
		} else {
			ld->recipient = STRNDUP(email_get_email_string("IDS_EMAIL_SBODY_NO_RECIPIENTS_M_NOUN"), RECIPIENT_LEN - 1);
		}
	}

	/* date & time */
	ld->absolute_time = mail_info->date_time;
	ld->timeordate = email_get_str_datetime(ld->absolute_time);

	if (view->sort_type == EMAIL_SORT_TO_CC_BCC) {
		int i;
		int account_count = 0;
		email_account_t *account_list = NULL;

		ld->is_to_address_mail = false;

		if (email_engine_get_account_list(&account_count, &account_list)) {
			for (i = 0; i < account_count; i++) {
				if (view->account_id == 0 || view->account_id == account_list[i].account_id) {
					if (g_strstr_len(mail_info->email_address_recipient, -1, account_list[i].user_email_address)) {
						ld->is_to_address_mail = true;
						break;
					}
				}
			}
			email_engine_free_account_list(&account_list, account_count);
		}
	}
	return ld;
}

MailItemData *mailbox_list_get_mail_item_data_by_mailid(int mailid, GList *mail_list)
{
	/*debug_enter();*/

	MailItemData *ld = NULL;

	/* find the list node having same mailid */
	GList *cur = g_list_first(mail_list);
	for (; cur; cur = g_list_next(cur)) {
		ld = (MailItemData *)g_list_nth_data(cur, 0);
		if (ld->mail_id == mailid) return ld;
	}

	return NULL;
}

MailItemData *mailbox_list_get_mail_item_data_by_threadid(int thread_id, GList *mail_list)
{
	/*debug_enter();*/

	MailItemData *ld = NULL;

	GList *cur = g_list_first(mail_list);
	for (; cur; cur = g_list_next(cur)) {
		ld = (MailItemData *) g_list_nth_data(cur, 0);
		/*debug_log("COMP: ld->thread_id: %d, ld->mail_id: %d vs thdid: %d", ld->thread_id, ld->mail_id, thread_id);*/
		if (ld->thread_id == thread_id) {
			debug_log("found mail for thread_id(%d)", thread_id);
			return ld;
		}
	}

	return NULL;
}

void mailbox_list_free_all_item_data(EmailMailboxView *view)
{
	debug_enter();
	retm_if(!view, "view is NULL");

	_mail_item_data_list_free(&(view->mail_list));

	_mailbox_free_req_data(&view->remaining_req);
}

void mailbox_list_free_mail_item_data(MailItemData *ld)
{
	retm_if(!ld, "ld is NULL");

	G_FREE(ld->alias);
	G_FREE(ld->sender);
	G_FREE(ld->preview_body);
	G_FREE(ld->group_title);
	FREE(ld->folder_name);
	FREE(ld->title);
	FREE(ld->recipient);
	FREE(ld->timeordate);
	ld->base.item = NULL;
	FREE(ld);
}

void mailbox_create_select_info(EmailMailboxView *view)
{
	debug_enter();

	int cnt = 0;
	char text[128];

	cnt = eina_list_count(view->selected_mail_list);
	mailbox_view_title_unpack(view);
	elm_naviframe_item_title_enabled_set(view->base.navi_item, EINA_TRUE, EINA_TRUE);
	elm_naviframe_item_style_set(view->base.navi_item, NULL);

	snprintf(text, sizeof(text), email_get_email_string("IDS_EMAIL_HEADER_PD_SELECTED_ABB2"), cnt);
	elm_object_item_text_set(view->base.navi_item, text);
	elm_object_item_part_text_set(view->base.navi_item, "subtitle", NULL);

	/* Title Cancel Button */
	if (!view->cancel_btn) {
		view->cancel_btn = elm_button_add(view->base.module->navi);
		elm_object_style_set(view->cancel_btn, "naviframe/title_left");
		elm_object_domain_translatable_text_set(view->cancel_btn, PACKAGE, "IDS_TPLATFORM_ACBUTTON_CANCEL_ABB");
		evas_object_smart_callback_add(view->cancel_btn, "clicked", mailbox_exit_clicked_edit_mode, view);
	}
	elm_object_item_part_content_set(view->base.navi_item, "title_left_btn", view->cancel_btn);

	/* Title Done Button */
	if (!view->save_btn) {
		view->save_btn = elm_button_add(view->base.module->navi);
		elm_object_style_set(view->save_btn, "naviframe/title_right");
		if (view->editmode_type == MAILBOX_EDIT_TYPE_DELETE) {
			elm_object_domain_translatable_text_set(view->save_btn, PACKAGE, "IDS_TPLATFORM_ACBUTTON_DELETE_ABB");
		} else {
			elm_object_domain_translatable_text_set(view->save_btn, PACKAGE, "IDS_TPLATFORM_ACBUTTON_DONE_ABB");
		}
		evas_object_smart_callback_add(view->save_btn, "clicked", mailbox_update_done_clicked_edit_mode, view);
	}
	elm_object_item_part_content_set(view->base.navi_item, "title_right_btn", view->save_btn);

	if (cnt == 0) {
		elm_object_disabled_set(view->save_btn, EINA_TRUE);
	} else {
		elm_object_disabled_set(view->save_btn, EINA_FALSE);
	}

	debug_leave();
}

void mailbox_update_select_info(EmailMailboxView *view)
{
	debug_enter();

	int cnt = 0;
	char text[128];

	cnt = eina_list_count(view->selected_mail_list);
	int cnt_mail_list = g_list_length(view->mail_list);

	if (cnt_mail_list == 0) {
		debug_log("no email remain. b_editmode [%d], list count = %d", view->b_editmode, cnt_mail_list);
		mailbox_exit_edit_mode(view);
		return;
	}

	snprintf(text, sizeof(text), email_get_email_string("IDS_EMAIL_HEADER_PD_SELECTED_ABB2"), cnt);

	elm_object_item_text_set(view->base.navi_item, text);
	elm_object_item_part_text_set(view->base.navi_item, "subtitle", NULL);

	if (cnt == 0) {
		elm_object_disabled_set(view->save_btn, EINA_TRUE);
	} else {
		elm_object_disabled_set(view->save_btn, EINA_FALSE);
	}

	debug_leave();
}

static void mailbox_update_done_clicked_edit_mode(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailMailboxView *view = (EmailMailboxView *)data;
	debug_log("view->b_editmode %d", view->b_editmode);

	int cnt = 0;
	cnt = eina_list_count(view->selected_mail_list);
	if (cnt == 0) {
		int ret = notification_status_message_post(email_get_email_string("IDS_EMAIL_HEADER_SELECT_EMAILS"));
		if (ret != NOTIFICATION_ERROR_NONE) {
			debug_log("notification_status_message_post() failed: %d", ret);
		}
		return;
	}

	if (view->editmode_type == MAILBOX_EDIT_TYPE_DELETE) {
		mailbox_delete_mail(view);
	} else if (view->editmode_type == MAILBOX_EDIT_TYPE_MOVE) {
		mailbox_move_mail(view);
	} else if (view->editmode_type == MAILBOX_EDIT_TYPE_ADD_SPAM) {
		mailbox_to_spam_mail(view);
	} else if (view->editmode_type == MAILBOX_EDIT_TYPE_REMOVE_SPAM) {
		mailbox_from_spam_mail(view);
	} else if (view->editmode_type == MAILBOX_EDIT_TYPE_MARK_READ) {
		mailbox_markread_mail(view);
	} else if (view->editmode_type == MAILBOX_EDIT_TYPE_MARK_UNREAD) {
		mailbox_markunread_mail(view);
	}

	debug_leave();
}

static void mailbox_exit_clicked_edit_mode(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailMailboxView *view = (EmailMailboxView *)data;

	mailbox_exit_edit_mode(view);
}

void mailbox_exit_edit_mode(void *data)
{
	debug_enter();
	EmailMailboxView *view = (EmailMailboxView *)data;

	DELETE_EVAS_OBJECT(view->more_ctxpopup);

	/*clear checked status of mail item*/
	mailbox_clear_edit_mode_list(view);

	elm_naviframe_item_title_enabled_set(view->base.navi_item, EINA_FALSE, EINA_TRUE);
	mailbox_view_title_pack(view);
	mailbox_view_title_update_all_without_mailbox_change(view);

	if ((view->mode == EMAIL_MAILBOX_MODE_ALL || view->mode == EMAIL_MAILBOX_MODE_MAILBOX)
		&& view->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX) {
		mailbox_send_all_btn_add(view);
	}

	debug_log("b_editmode [%d]", view->b_editmode);
	debug_leave();
}

void mailbox_clear_edit_mode_list(EmailMailboxView *view)
{
	debug_enter();

	int i = 0;
	int checked_count = eina_list_count(view->selected_mail_list);
	debug_log("checked_count [%d]", checked_count);
	view->b_editmode = false;

	/*Remove Cancel and Done buttons*/
	elm_object_item_part_content_unset(view->base.navi_item, "title_left_btn");
	DELETE_EVAS_OBJECT(view->cancel_btn);
	elm_object_item_part_content_unset(view->base.navi_item, "title_right_btn");
	DELETE_EVAS_OBJECT(view->save_btn);

	if (0 < g_list_length(view->mail_list)) {
		if (!view->b_searchmode
			&& ((view->mode == EMAIL_MAILBOX_MODE_MAILBOX && view->only_local == false
					&& view->mailbox_type != EMAIL_MAILBOX_TYPE_OUTBOX
					&& view->account_type == EMAIL_SERVER_TYPE_IMAP4)
				|| (view->mode == EMAIL_MAILBOX_MODE_MAILBOX && view->only_local == false
					&& view->mailbox_type == EMAIL_MAILBOX_TYPE_INBOX
					&& view->account_type == EMAIL_SERVER_TYPE_POP3))) {
			mailbox_load_more_messages_item_add(view);
		} else if (!view->b_searchmode
				&& view->mailbox_type != EMAIL_MAILBOX_TYPE_OUTBOX) {
			mailbox_no_more_emails_item_add(view);
		}
	}

	for (i = 0; i < checked_count; i++) {
		Eina_List *nth_list = eina_list_nth_list(view->selected_mail_list, i);
		MailItemData *ld = (MailItemData *)eina_list_data_get(nth_list);
		if (ld) {
			if (ld->checked == EINA_TRUE) {
				ld->checked = EINA_FALSE;
			}
		}
	}

	view->selected_mail_list = eina_list_free(view->selected_mail_list);
	view->selected_group_list = eina_list_free(view->selected_group_list);

	view->select_all_item_data.is_checked = EINA_FALSE;
	elm_genlist_realized_items_update(view->gl);

	mailbox_select_all_item_remove(view);
	mailbox_show_compose_btn(view);
	if (view->mode == EMAIL_MAILBOX_MODE_MAILBOX
		&& view->only_local == FALSE && !view->b_searchmode) {

		Evas_Coord scroll_x = 0, scroll_y = 0, scroll_w = 0, scroll_h = 0;
		elm_scroller_region_get(view->gl, &scroll_x, &scroll_y, &scroll_w, &scroll_h);

		if (scroll_y <= 0 || (scroll_y <= SELECT_ALL_LIST_ITEM_HEIGHT)) {
			mailbox_last_updated_time_item_add(view, true);
		} else {
			mailbox_last_updated_time_item_add(view, false);
		}
	}

	debug_leave();
}

void mailbox_update_edit_list_view(MailItemData *ld, EmailMailboxView *view)
{
	debug_enter();
	retm_if(!ld, "ld is NULL");
	retm_if(!ld->base.item, "ld->base.item is NULL");

	view->selected_mail_list = eina_list_remove(view->selected_mail_list, ld);

	/*int cnt = eina_list_count(view->selected_mail_list);*/
	int cnt_mail_list = g_list_length(view->mail_list);
	debug_log("b_editmode [%d], list count = %d", view->b_editmode, cnt_mail_list);

	if (cnt_mail_list <= 0 && view->b_editmode == true) {
		debug_log("Exit edit mode due to empty list");
		/* update controlbar to mailbox menu */
		mailbox_exit_edit_mode(view);
	} else {
		mailbox_update_select_info(view);
	}
}

void mailbox_remove_unnecessary_list_item_for_edit_mode(void *data)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailMailboxView *view = (EmailMailboxView *)data;

	mailbox_last_updated_time_item_remove(view);
	mailbox_load_more_messages_item_remove(view);
	mailbox_no_more_emails_item_remove(view);
	mailbox_get_more_progress_item_remove(view);
	mailbox_remove_refresh_progress_bar(view);
}

bool mailbox_check_searched_mail(int mail_id, void *data)
{
	retvm_if(!data, false, "data is NULL");
	email_list_filter_t *filter_list = NULL;
	int cnt_filter_list = 3;
	int i = 0;
	int total_count = 0, unread_count = 0;

	EmailMailboxView *view = (EmailMailboxView *)data;

	email_search_data_t *search_data = mailbox_make_search_data(view);

	if (search_data)
		cnt_filter_list += _get_filter_cnt_for_search_data(search_data);

	filter_list = malloc(sizeof(email_list_filter_t)*cnt_filter_list);
	if (!filter_list) {
		mailbox_free_mailbox_search_data(search_data);
		debug_error("filter_list is NULL - allocation memory failed");
		return NULL;
	}
	memset(filter_list, 0, sizeof(email_list_filter_t)*cnt_filter_list);

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_MAIL_ID;
	filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[i].list_filter_item.rule.key_value.integer_type_value = mail_id;
	i++;

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
	i++;

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_FLAGS_DELETED_FIELD;
	filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_NOT_EQUAL;
	filter_list[i].list_filter_item.rule.key_value.integer_type_value = 1;
	i++;

	if (search_data)
		_add_search_data_into_filter_list(search_data, filter_list, &i);

	email_engine_get_mail_count(filter_list, cnt_filter_list, &total_count, &unread_count);

	email_engine_free_list_filter(&filter_list, cnt_filter_list);

	debug_log("total_count(%d)", total_count);

	mailbox_free_mailbox_search_data(search_data);

	if (total_count > 0)
		return true;
	else
		return false;
}

void mailbox_list_make_remaining_items_in_thread(EmailMailboxView *view, AddRemainingMailReqData *req)
{
	debug_enter();
	retm_if(!view, "view is NULL");
	retm_if(!req, "req is NULL");
	if (email_request_queue_add_request(view->request_queue, EMAIL_REQUEST_TYPE_ADD_REMAINING_MAIL, req)) {
		debug_log("Request added");
	} else {
		debug_error("Added request failed");
		_mailbox_free_req_data(&req);
	}

	debug_leave();
}

void mailbox_folders_name_cache_free(EmailMailboxView *view)
{
	debug_enter();

	EmailFolderNameCashItem *item = NULL;
	EINA_LIST_FREE(view->folders_names_cashe, view->folders_names_cashe) {
		G_FREE(item->mailbox_name);
		FREE(item);
	}

	view->folders_names_cashe = NULL;

	debug_leave();
}

void mailbox_folders_name_cashe_add_name(EmailMailboxView *view, int mailbox_id, const char *folder_name)
{
	debug_enter();
	retm_if(!view || !folder_name, "Invalid arguments!");

	EmailFolderNameCashItem *cashe_item = MEM_ALLOC(cashe_item, 1);
	cashe_item->mailbox_id = mailbox_id;
	cashe_item->mailbox_name = strdup(folder_name);

	view->folders_names_cashe = eina_list_sorted_insert(view->folders_names_cashe, _mailbox_folder_name_cashe_comparator_cb, cashe_item);
	debug_leave();
}

char *mailbox_folders_name_cashe_get_name(EmailMailboxView *view, int mailbox_id)
{
	retvm_if(!view, NULL, "Invalid arguments!");

	EmailFolderNameCashItem *search_item = MEM_ALLOC(search_item, 1);
	search_item->mailbox_id = mailbox_id;
	search_item->mailbox_name = NULL;

	EmailFolderNameCashItem *search_result = eina_list_search_sorted(view->folders_names_cashe, _mailbox_folder_name_cashe_comparator_cb, search_item);
	FREE(search_item);

	if (!search_result) {
		debug_log("Item not found in cash!");
		return NULL;
	} else {
		debug_log("Item found in cash! Mailbox name: %s", search_result->mailbox_name);
		return strdup(search_result->mailbox_name);
	}
}
