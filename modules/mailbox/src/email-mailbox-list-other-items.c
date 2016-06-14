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

#include "email-mailbox.h"
#include "email-mailbox-toolbar.h"
#include "email-mailbox-list.h"
#include "email-mailbox-list-other-items.h"
#include "email-mailbox-sync.h"
#include "email-mailbox-util.h"


/*
 * Definitions
 */

/*
 * Structures
 */


/*
 * Globals
 */

#define TIME_OUT	7.0
#define GROUP_INDEX_LIST_ITEM_HEIGHT 52 /* 51 + 1 (margin) */

/*
 * Declaration for static functions
 */

static char *_last_updated_time_gl_text_get(void *data, Evas_Object *obj, const char *part);
static void _edge_bottom_cb(void *data, Evas_Object *obj, void *event_info);
static char *_get_more_progress_gl_text_get(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_get_more_progress_gl_content_get(void *data, Evas_Object *obj, const char *source);
static char *_load_more_messages_gl_text_get(void *data, Evas_Object *obj, const char *part);
static Evas_Event_Flags _mailbox_gesture_flicks_abort_cb(void *data, void *event_info);
static Evas_Event_Flags _mailbox_gesture_flicks_end_cb(void *data, void *event_info);
static void _mailbox_gesture_flick_to_load_more(EmailMailboxView *view);


/* Select All item */
static char *_select_all_item_gl_text_get(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_select_all_item_gl_content_get(void *data, Evas_Object *obj, const char *source);
static void _select_all_item_check_changed_cb(void *data, Evas_Object *obj, void *event_info);
static void _select_all_item_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static char *_no_more_emails_gl_text_get(void *data, Evas_Object *obj, const char *part);

/* Send All item */
static void _mailbox_send_outgoing_messages_thread_worker(void *data, Ecore_Thread *th);
static void _mailbox_send_outgoing_messages_thread_finish(void *data, Ecore_Thread *th);
static GList *_mailbox_make_send_all_data_list(EmailMailboxView *view);
static void _mailbox_free_send_all_data_list(GList *send_all_list);

static const Elm_Genlist_Item_Class itc_last_updated_time = {
	ELM_GEN_ITEM_CLASS_HEADER,
	.item_style = "group_index",
	.func.text_get = _last_updated_time_gl_text_get
};

static const Elm_Genlist_Item_Class itc_get_more_progress = {
	ELM_GEN_ITEM_CLASS_HEADER,
	.item_style = "type1",
	.func.text_get = _get_more_progress_gl_text_get,
	.func.content_get = _get_more_progress_gl_content_get
};

static const Elm_Genlist_Item_Class itc_load_more_messages = {
	ELM_GEN_ITEM_CLASS_HEADER,
	.item_style = "type1",
	.func.text_get = _load_more_messages_gl_text_get
};

static const Elm_Genlist_Item_Class itc_no_more_emails = {
	ELM_GEN_ITEM_CLASS_HEADER,
	.item_style = "type1",
	.func.text_get = _no_more_emails_gl_text_get
};

static const Elm_Genlist_Item_Class itc_select_all = {
	ELM_GEN_ITEM_CLASS_HEADER,
	.item_style = "group_index",
	.func.text_get = _select_all_item_gl_text_get,
	.func.content_get = _select_all_item_gl_content_get
};

/*
 * Definition for static functions
 */

static char *_last_updated_time_gl_text_get(void *data, Evas_Object *obj, const char *part)
{
	UpdateTimeItemData *item_data = (UpdateTimeItemData *)data;
	if(item_data->time && !strcmp(part, "elm.text")) {
		return strdup(item_data->time);
	} else
		return NULL;
}

static void _edge_bottom_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailMailboxView *view = (EmailMailboxView *)data;

	/* should know the current mode, account id, and mailbox name. */
	/* The view type is always DATE view. */
	debug_log("view->mode(%d), view->mailbox_type(%d) b_edge_bottom(%d)", view->mode, view->mailbox_type, view->b_edge_bottom);

	if (view->b_edge_bottom == false) {
		view->b_edge_bottom = true;
		debug_log("set b_edge_bottom as TRUE %d", view->b_edge_bottom);
	}

	debug_leave();
}

static char *_get_more_progress_gl_text_get(void *data, Evas_Object *obj, const char *part)
{
	debug_enter();

	if (!strcmp(part, "elm.text")) {
		return strdup(_("IDS_EMAIL_BODY_LOADING_MORE_ING_ABB"));
	}
	return NULL;
}

static Evas_Object *_get_more_progress_gl_content_get(void *data, Evas_Object *obj, const char *source)
{
	debug_enter();

	if (!strcmp(source, "elm.swallow.icon")) {
		Evas_Object *progressbar = elm_progressbar_add(obj);
		elm_object_style_set(progressbar, "process_medium");
		evas_object_size_hint_align_set(progressbar, EVAS_HINT_FILL, EVAS_HINT_FILL);
		evas_object_size_hint_weight_set(progressbar, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_show(progressbar);
		elm_progressbar_pulse(progressbar, EINA_TRUE);
		return progressbar;
	}

	return NULL;
}

static char *_load_more_messages_gl_text_get(void *data, Evas_Object *obj, const char *part)
{
	debug_enter();

	if (!strcmp(part, "elm.text")) {
		return strdup(_("IDS_EMAIL_BODY_FLICK_UPWARDS_TO_LOAD_MORE_M_NOUN_ABB"));
	}
	return NULL;
}

static char *_select_all_item_gl_text_get(void *data, Evas_Object *obj, const char *part)
{
	if (!strcmp(part, "elm.text")) {
		return strdup(_("IDS_EMAIL_HEADER_SELECT_ALL_ABB"));
	} else
		return NULL;
}

static Evas_Object *_select_all_item_gl_content_get(void *data, Evas_Object *obj, const char *source)
{
	if (data == NULL) {
		debug_warning("data is NULL");
		return NULL;
	}

	SelectAllItemData *item_data = (SelectAllItemData *)data;
	if (!strcmp(source, "elm.swallow.end")) {
		item_data->checkbox = elm_check_add(obj);
		evas_object_propagate_events_set(item_data->checkbox, EINA_FALSE);
		elm_check_state_pointer_set(item_data->checkbox, &item_data->is_checked);
		evas_object_smart_callback_add(item_data->checkbox, "changed", _select_all_item_check_changed_cb, item_data->view);
		return item_data->checkbox;
	}

	return NULL;
}

static void _select_all_item_check_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailMailboxView *view = (EmailMailboxView *) data;
	Eina_Bool group_checked = EINA_FALSE;
	MailItemData *child_ld;

	if (obj == NULL) {
		group_checked = view->select_all_item_data.is_checked;
	} else {
		group_checked = elm_check_state_get(obj);
		view->select_all_item_data.is_checked = group_checked;
	}
	debug_log("group_checked = %d", group_checked);

	Elm_Object_Item *next_item = elm_genlist_item_next_get(elm_genlist_first_item_get(view->gl));
	while (next_item) {
		child_ld = (MailItemData *)elm_object_item_data_get(next_item);
		debug_log("executed elm_object_item_data_get()");
		if (NULL == child_ld) {
			debug_warning("next_item does not have data associated with it");
			break;
		}
		if (child_ld->checked != group_checked) {
			child_ld->checked = group_checked;
			debug_log("executing elm_check_state_set()");
			debug_log("child_ld->checked = %d", child_ld->checked);
			if (child_ld->mail_status == EMAIL_MAIL_STATUS_SENDING || child_ld->mail_status == EMAIL_MAIL_STATUS_SEND_WAIT) {
				debug_log("email being sent selected - unselect it(%d)", child_ld->mail_status);
				child_ld->checked = EINA_FALSE;
				view->select_all_item_data.is_checked = EINA_FALSE;
				elm_check_state_set(view->select_all_item_data.checkbox, view->select_all_item_data.is_checked);
				int ret = notification_status_message_post(email_get_email_string("IDS_EMAIL_TPOP_SENDING_EMAIL_ING"));
				if (ret != NOTIFICATION_ERROR_NONE) {
					debug_warning("notification_status_message_post() failed: %d", ret);
				}
				break;
			}

			if (child_ld->checked) {
				view->selected_mail_list = eina_list_append(view->selected_mail_list, child_ld);
			} else {
				view->selected_mail_list = eina_list_remove(view->selected_mail_list, child_ld);
			}
		}
		elm_check_state_set(view->select_all_item_data.checkbox, view->select_all_item_data.is_checked);
		next_item = elm_genlist_item_next_get(next_item);

		/* ETC genlist items should be removed before entering edit mode. If normal case, below error case is not occurred. */
		if ((next_item == view->load_more_messages_item) || (next_item == view->no_more_emails_item)
				|| (next_item == view->get_more_progress_item)
				|| (next_item == view->update_time_item_data.base.item)) {
			debug_error("ETC genlist item is inserted. It should not be included!!!!");
			break;
		}
	}

	Eina_List *l;
	Elm_Object_Item *it;
	l = elm_genlist_realized_items_get(view->gl);
	EINA_LIST_FREE(l, it) {
		child_ld = (MailItemData *)elm_object_item_data_get(it);
		elm_check_state_set(child_ld->check_btn, child_ld->checked);
	}
	mailbox_create_select_info(view);
}

/*
 * Definition for exported functions
 */

void mailbox_last_updated_time_item_add(EmailMailboxView *view, bool show_directly)
{
	debug_enter();

	mailbox_last_updated_time_item_remove(view);
	Elm_Object_Item *item = elm_genlist_item_prepend(view->gl,
													&itc_last_updated_time,
													&view->update_time_item_data,
													NULL,
													ELM_GENLIST_ITEM_NONE,
													NULL,
													NULL);
	view->update_time_item_data.base.item = item;
	elm_genlist_item_select_mode_set(item, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

	if (show_directly) {
		elm_genlist_item_show(item, ELM_GENLIST_ITEM_SCROLLTO_TOP);
	}

	debug_leave();
}

void mailbox_last_updated_time_item_update(int mailbox_id, EmailMailboxView *view)
{
	debug_enter();

	email_mailbox_t *mailbox = NULL;

	if (email_engine_get_mailbox_by_mailbox_id(mailbox_id, &mailbox)) {
		mailbox_set_last_updated_time(mailbox->last_sync_time, view);
		email_engine_free_mailbox_list(&mailbox, 1);
	}

	if (view->update_time_item_data.base.item)
		elm_genlist_item_update(view->update_time_item_data.base.item);
	else {
		mailbox_last_updated_time_item_add(view, true);
	}

	return;
}

void mailbox_last_updated_time_item_remove(EmailMailboxView *view)
{
	debug_enter();

	if (view && view->update_time_item_data.base.item) {
		elm_object_item_del(view->update_time_item_data.base.item);
		view->update_time_item_data.base.item = NULL;
	}
}

void mailbox_refresh_flicks_cb_register(EmailMailboxView *view)
{
	debug_enter();

	evas_object_smart_callback_add(view->gl, "edge,bottom", _edge_bottom_cb, view);
	debug_log("_edge_bottom_cb() is registered in view->gl(%p)", view->gl);

	view->gesture_ly = elm_gesture_layer_add(view->gl);
	if (view->gesture_ly) {
		elm_gesture_layer_attach(view->gesture_ly, view->gl);
		elm_gesture_layer_cb_set(view->gesture_ly, ELM_GESTURE_N_FLICKS, ELM_GESTURE_STATE_END, _mailbox_gesture_flicks_end_cb, view);
		elm_gesture_layer_cb_set(view->gesture_ly, ELM_GESTURE_N_FLICKS, ELM_GESTURE_STATE_ABORT, _mailbox_gesture_flicks_abort_cb, view);
	} else {
		debug_warning("gesture layout INVALID!! gesture_ly(%p)", view->gesture_ly);
	}
}

static Evas_Event_Flags _mailbox_gesture_flicks_end_cb(void *data, void *event_info)
{
	debug_enter();
	retvm_if(!data, EVAS_EVENT_FLAG_NONE, "data == NULL");
	retvm_if(!event_info, EVAS_EVENT_FLAG_NONE, "event_info == NULL");

	EmailMailboxView *view = (EmailMailboxView *)data;
	retvm_if(!view, EVAS_EVENT_FLAG_NONE, "view == NULL");
	Elm_Gesture_Line_Info *p = (Elm_Gesture_Line_Info *) event_info;
	if (!view->b_editmode && !view->b_searchmode && !view->only_local) {
		if ((p->angle > 20.0f && p->angle < 160.0f) || (p->angle > 200.0f && p->angle < 340.0f)) {
			return EVAS_EVENT_FLAG_NONE;

		} else if (p->angle > 340.0f || p->angle < 20.0f) {
			_mailbox_gesture_flick_to_load_more(view);

		}
	}
	return EVAS_EVENT_FLAG_NONE;

}

static void _mailbox_gesture_flick_to_load_more(EmailMailboxView *view)
{
	debug_enter();
	int ncnt = g_list_length(view->mail_list);
	debug_log("view->mode(%d), view->mailbox_type(%d) b_edge_bottom(%d) sort_type(%d)", view->mode, view->mailbox_type, view->b_edge_bottom, view->sort_type);
	if ((NULL != view->load_more_messages_item) && (view->b_edge_bottom == true || ncnt < 6) && (view->b_enable_get_more == true)) {
		
		if (view->mode == EMAIL_MAILBOX_MODE_ALL &&
			(view->mailbox_type == EMAIL_MAILBOX_TYPE_NONE ||
				view->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX ||
				view->mailbox_type == EMAIL_MAILBOX_TYPE_SEARCH_RESULT ||
				view->mailbox_type == EMAIL_MAILBOX_TYPE_FLAGGED))
			return;

		if (view->mode == EMAIL_MAILBOX_MODE_MAILBOX &&
			(view->mailbox_type == EMAIL_MAILBOX_TYPE_NONE ||
				view->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX ||
				view->mailbox_type == EMAIL_MAILBOX_TYPE_SEARCH_RESULT ||
				view->mailbox_type == EMAIL_MAILBOX_TYPE_FLAGGED))
			return;

		if (view->get_more_progress_item || view->sort_type != EMAIL_SORT_DATE_RECENT) {
			return;
		}

		mailbox_sync_cancel_all(view);
		if (mailbox_sync_more_messages(view)) {
			mailbox_load_more_messages_item_remove(view);
			mailbox_no_more_emails_item_remove(view);
			mailbox_get_more_progress_item_add(view);
		}
	}
}

static Evas_Event_Flags _mailbox_gesture_flicks_abort_cb(void *data, void *event_info)
{
	debug_enter();
	retvm_if(!data, EVAS_EVENT_FLAG_NONE, "data == NULL");
	retvm_if(!event_info, EVAS_EVENT_FLAG_NONE, "event_info == NULL");

	EmailMailboxView *view = (EmailMailboxView *)data;
	retvm_if(!view, EVAS_EVENT_FLAG_NONE, "view == NULL");
	Elm_Gesture_Line_Info *p = (Elm_Gesture_Line_Info *) event_info;

	if (!view->b_editmode && !view->b_searchmode && !view->only_local) {
		if ((p->angle > 20.0f && p->angle < 160.0f) || (p->angle > 200.0f && p->angle < 340.0f)) {
			return EVAS_EVENT_FLAG_NONE;
		}

		if (view->b_edge_bottom == true && (p->angle > 160.0f && p->angle < 200.0f)) {
			view->b_edge_bottom = false;
		}

	}

	return EVAS_EVENT_FLAG_NONE;
}

void mailbox_get_more_progress_item_add(EmailMailboxView *view)
{
	debug_enter();

	int i;
	int total_mail_count = 0;
	int unread_mail_count = 0;
	int account_count = 0;
	email_account_t *account_list = NULL;
	email_mailbox_t *mailbox_list = NULL;

	if (view->get_more_progress_item) {
		debug_log("more_progress is already created(%p)", view->get_more_progress_item);
		return;
	}

	if (view->b_enable_get_more) {
		view->b_enable_get_more = false;
		debug_log("_edge_bottom_cb() is unregistered in view->gl(%p)", view->gl);
	}

	view->total_mail_count = 0;
	view->unread_mail_count = 0;

	if (view->mode == EMAIL_MAILBOX_MODE_ALL) {
		if (email_engine_get_account_list(&account_count, &account_list)) {
			for (i = 0; i < account_count; i++) {
				debug_log("account_id(%d), mailbox_type(%d)", account_list->account_id, view->mailbox_type);
				if (email_engine_get_mailbox_by_mailbox_type(account_list->account_id, view->mailbox_type, &mailbox_list)) {
					total_mail_count += mailbox_list->total_mail_count_on_local;
					unread_mail_count += mailbox_list->unread_count;
					debug_log("unread_mail_count(%d), total_mail_count(%d)", unread_mail_count, total_mail_count);
					email_engine_free_mailbox_list(&mailbox_list, 1);
				}
			}
			email_engine_free_account_list(&account_list, account_count);
		}
	} else if (view->mode == EMAIL_MAILBOX_MODE_MAILBOX) {
		debug_log("account_id(%d), mailbox_type(%d)", view->account_id, view->mailbox_type);
		if (email_engine_get_mailbox_by_mailbox_id(view->mailbox_id, &mailbox_list)) {
			total_mail_count = mailbox_list->total_mail_count_on_local;
			unread_mail_count = mailbox_list->unread_count;
			debug_log("unread_mail_count(%d), total_mail_count(%d)", unread_mail_count, total_mail_count);
			email_engine_free_mailbox_list(&mailbox_list, 1);
		}
	}

	view->total_mail_count = total_mail_count;
	view->unread_mail_count = unread_mail_count;

	view->get_more_progress_item = elm_genlist_item_append(view->gl,
									&itc_get_more_progress,
									NULL,
									NULL,
									ELM_GENLIST_ITEM_NONE,
									NULL,
									NULL);

	elm_genlist_item_select_mode_set(view->get_more_progress_item, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
	elm_genlist_item_bring_in(view->get_more_progress_item, ELM_GENLIST_ITEM_SCROLLTO_IN);


	if (view->b_enable_get_more == false) {
		view->b_enable_get_more = true;
		debug_log("_edge_bottom_cb() is registered in view->gl(%p)", view->gl);
	}

	debug_leave();
}

void mailbox_get_more_progress_item_remove(EmailMailboxView *view)
{
	debug_enter();

	if (view->get_more_progress_item) {
		view->b_edge_bottom = false;
		elm_object_item_del(view->get_more_progress_item);
		view->get_more_progress_item = NULL;
	}
}

void mailbox_load_more_messages_item_add(EmailMailboxView *view)
{
	debug_enter();

	if (view->load_more_messages_item) {
		debug_log("get_more_messages item already exist.");
		return;
	}

	view->load_more_messages_item = elm_genlist_item_append(view->gl,
									&itc_load_more_messages,
									NULL,
									NULL,
									ELM_GENLIST_ITEM_NONE,
									NULL,
									NULL);

	elm_genlist_item_select_mode_set(view->load_more_messages_item, ELM_OBJECT_SELECT_MODE_NONE);
	view->b_edge_bottom = false;
	debug_leave();
}

void mailbox_load_more_messages_item_remove(EmailMailboxView *view)
{
	debug_enter();

	if (view->load_more_messages_item) {
		elm_object_item_del(view->load_more_messages_item);
		view->load_more_messages_item = NULL;
		view->b_edge_bottom = false;
	}
}

static void _send_outgoing_messages_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "data is NULL");

	EmailMailboxView *view = (EmailMailboxView *)data;
	MailOutgoingListData *send_thread_data = MEM_ALLOC(send_thread_data, 1);
	retm_if(!send_thread_data, "Memory allocation failed");

	Eina_Bool is_disabled = elm_object_disabled_get(view->outbox_send_all_btn);
	if (!is_disabled) {
		elm_object_disabled_set(view->outbox_send_all_btn, !is_disabled);
	}

	mailbox_send_all_btn_remove(view);

	int ret = notification_status_message_post(email_get_email_string("IDS_EMAIL_TPOP_SENDING_EMAILS_ING"));
	if (ret != NOTIFICATION_ERROR_NONE) {
		debug_log("notification_status_message_post() failed: %d", ret);
	}

	send_thread_data->send_all_runned = &(view->is_send_all_run);
	send_thread_data->send_all_list =  _mailbox_make_send_all_data_list(view);
	Ecore_Thread *th = ecore_thread_run(_mailbox_send_outgoing_messages_thread_worker, _mailbox_send_outgoing_messages_thread_finish, NULL, send_thread_data);
	if (!th) {
		debug_log("failed to start thread for sending outgoing mail");
	}

	debug_leave();
}

static GList *_mailbox_make_send_all_data_list(EmailMailboxView *view)
{
	debug_enter();

	if (!view) {
		debug_warning("view is NULL");
		return NULL;
	}

	GList *send_all_list = NULL;
	GList *cur = g_list_first(view->mail_list);
	MailItemData *ld = NULL;
	for (; cur; cur = g_list_next(cur)) {
		ld = (MailItemData *)g_list_nth_data(cur, 0);

		if (ld) {
			MailOutgoingItem *new_item = MEM_ALLOC(new_item,  1);
			retvm_if(!new_item, NULL, "memory allocarion failed.");

			new_item->mail_id = ld->mail_id;
			new_item->account_id = ld->account_id;
			send_all_list = g_list_append(send_all_list, new_item);
		}
	}

	debug_leave();
	return send_all_list;
}

static void _mailbox_free_send_all_data_list(GList *send_all_list)
{
	GList *cur = g_list_first(send_all_list);
	for (; cur; cur = g_list_next(cur)) {
		MailItemData *ld = (MailItemData *) g_list_nth_data(cur, 0);
		free(ld);
	}
	g_list_free(send_all_list);
}

static void _mailbox_send_outgoing_messages_thread_worker(void *data, Ecore_Thread *th)
{
	debug_enter();
	email_account_t *account = NULL;
	MailOutgoingListData *send_thread_data = NULL;
	int handle = 0;
	MailOutgoingItem *ld = NULL;
	if (!data) {
		debug_warning("data is NULL");
		return;
	}

	send_thread_data = (MailOutgoingListData *)data;
	*(send_thread_data->send_all_runned) = false;
	/* find the list node having same mailid */
	GList *cur = g_list_first(send_thread_data->send_all_list);
	for (; cur; cur = g_list_next(cur)) {
		ld = (MailOutgoingItem *) g_list_nth_data(cur, 0);

		if (ld) {
			debug_log("ld->mail_id: %d", ld->mail_id);
			if (email_engine_get_account_data(ld->account_id, EMAIL_ACC_GET_OPT_DEFAULT, &account)) {
				email_mail_attribute_value_t attribute_value = {0};
				attribute_value.integer_type_value = account->auto_resend_times;
				debug_log("ld->mail_id : %d, attribute_value->integer_type_value : %d", ld->mail_id, attribute_value.integer_type_value);
				email_engine_update_mail_attribute(ld->account_id, &(ld->mail_id), 1, EMAIL_MAIL_ATTRIBUTE_REMAINING_RESEND_TIMES, attribute_value);
				email_engine_free_account_list(&account, 1);
			}

			/* Send email again */
			if (!email_engine_send_mail(ld->mail_id, &handle)) {
				debug_warning("email_engine_send_mail acct(%d)", ld->account_id);
				*(send_thread_data->send_all_runned) = false;
				return;
			}
		}
	}
	debug_leave();
}

static void _mailbox_send_outgoing_messages_thread_finish(void *data, Ecore_Thread *th)
{
	debug_enter();
	MailOutgoingListData *send_thread_data = NULL;
	if (!data) {
		debug_warning("data is NULL");
		return;
	}

	send_thread_data = (MailOutgoingListData *)data;
	_mailbox_free_send_all_data_list(send_thread_data->send_all_list);
	*(send_thread_data->send_all_runned) = false;
	free(send_thread_data);

	debug_leave();
}

Evas_Object *mailbox_create_toolbar_btn(Evas_Object *parent, const char *text, const char *domain)
{
	debug_enter();

	retvm_if(!parent, NULL, "Invalid parameter: parent is NULL!");

	Evas_Object *btn = elm_button_add(parent);
	elm_object_style_set(btn, "default");
	elm_object_domain_translatable_text_set(btn, domain, text);
	evas_object_show(btn);

	debug_leave();
	return btn;
}

void mailbox_send_all_btn_add(EmailMailboxView *view)
{
	debug_enter();

	int mail_count = g_list_length(view->g_sending_mail_list);
	if (view->is_send_all_run || mail_count != 0) {
		debug_log("Send all is currently runned now");
		return;
	}

	if (view->b_searchmode || view->b_editmode) {
		debug_log("Send all not needed: b_editmode [%d],  b_searchmode [%d] ", view->b_editmode, view->b_searchmode);
		return;
	}

	if (!view->outbox_send_all_bar) {
		view->outbox_send_all_bar = elm_layout_add(view->base.module->navi);
		evas_object_show(view->outbox_send_all_bar);
		view->outbox_send_all_btn = mailbox_create_toolbar_btn(view->outbox_send_all_bar, _("IDS_EMAIL_BUTTON_SEND_ALL"), PACKAGE);
		evas_object_smart_callback_add(view->outbox_send_all_btn, "clicked", _send_outgoing_messages_clicked_cb, (void *)view);

		Eina_Bool is_disabled = elm_object_disabled_get(view->outbox_send_all_btn);
		if (is_disabled)
			elm_object_disabled_set(view->outbox_send_all_btn, !is_disabled);

		elm_layout_file_set(view->outbox_send_all_bar, email_get_mailbox_theme_path(), "email/layout/send_all_bar");
		elm_layout_content_set(view->outbox_send_all_bar, "send_all_bar_button", view->outbox_send_all_btn);
		elm_object_part_content_set(view->content_layout, "send_all_bar", view->outbox_send_all_bar);
	}

	debug_leave();
	return;
}

void mailbox_send_all_btn_remove(EmailMailboxView *view)
{
	debug_enter();
	if (view->outbox_send_all_bar) {
		elm_object_part_content_unset(view->outbox_send_all_bar, "send_all_bar_button");
		DELETE_EVAS_OBJECT(view->outbox_send_all_btn);
		elm_object_part_content_unset(view->content_layout, "send_all_bar");
		DELETE_EVAS_OBJECT(view->outbox_send_all_bar);
		view->outbox_send_all_bar = NULL;
	}

	debug_leave();
}

static void _select_all_item_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	SelectAllItemData *item_data = (SelectAllItemData *)data;
	Elm_Object_Item *it = (Elm_Object_Item *)event_info;

	if (!event_info) {
		debug_warning("event_info is NULL");
		return;
	}
	elm_genlist_item_selected_set((Elm_Object_Item *)it, EINA_FALSE);

	if (item_data->is_checked) {
		item_data->is_checked = EINA_FALSE;
	} else {
		item_data->is_checked = EINA_TRUE;
	}
	_select_all_item_check_changed_cb(item_data->view, NULL, event_info);
}

void mailbox_select_all_item_add(EmailMailboxView *view)
{
	debug_enter();

	mailbox_select_all_item_remove(view);

	Evas_Coord scroll_x = 0, scroll_y = 0, scroll_w = 0, scroll_h = 0;
	elm_scroller_region_get(view->gl, &scroll_x, &scroll_y, &scroll_w, &scroll_h);
	debug_log("scroll_x : %d scroll_y : %d scroll_w : %d scroll_h : %d", scroll_x, scroll_y, scroll_w, scroll_h);

	view->select_all_item_data.base.item = elm_genlist_item_prepend(view->gl,
															&itc_select_all,
															&view->select_all_item_data,
															NULL,
															ELM_GENLIST_ITEM_NONE,
															_select_all_item_clicked_cb,
															&view->select_all_item_data);

	if (scroll_y <= 0 || (view->update_time_item_data.base.item && scroll_y <= GROUP_INDEX_LIST_ITEM_HEIGHT)) {
			elm_genlist_item_show(view->select_all_item_data.base.item, ELM_GENLIST_ITEM_SCROLLTO_TOP);
	}

	debug_leave();
}

void mailbox_select_all_item_remove(EmailMailboxView *view)
{
	debug_enter();

	if (view && view->select_all_item_data.base.item) {
		elm_object_item_del(view->select_all_item_data.base.item);
		view->select_all_item_data.base.item = NULL;
		view->select_all_item_data.checkbox = NULL;
		view->select_all_item_data.is_checked = EINA_FALSE;
		debug_log("select_all_item is removed");
	}
}

static char *_no_more_emails_gl_text_get(void *data, Evas_Object *obj, const char *part)
{
	debug_enter();

	if (!strcmp(part, "elm.text")) {
		return strdup(_("IDS_EMAIL_BODY_NO_MORE_EMAILS_M_STATUS"));
	}
	return NULL;
}

void mailbox_no_more_emails_item_add(EmailMailboxView *view)
{
	debug_enter();

	if (view->no_more_emails_item) {
		debug_log("no_more_emails_item item already exist.");
		return;
	}

	if (view->b_editmode) {
		debug_log("In case of edit mode, no_more_emails_item item is not needed.");
		return;
	}

	view->no_more_emails_item = elm_genlist_item_append(view->gl,
									&itc_no_more_emails,
									NULL,
									NULL,
									ELM_GENLIST_ITEM_NONE,
									NULL,
									NULL);

	elm_genlist_item_select_mode_set(view->no_more_emails_item, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

	debug_leave();
}

void mailbox_no_more_emails_item_remove(EmailMailboxView *view)
{
	debug_enter();

	if (view->no_more_emails_item) {
		elm_object_item_del(view->no_more_emails_item);
		view->no_more_emails_item = NULL;
	}
}
