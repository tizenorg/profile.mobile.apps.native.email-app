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
#include "email-mailbox-item.h"
#include "email-mailbox-list.h"
#include "email-mailbox-list-other-items.h"
#include "email-mailbox-ug-util.h"
#include "email-mailbox-util.h"
#include "email-mailbox-more-menu.h"
#include "email-mailbox-sync.h"
#include "email-mailbox-search.h"


/*
 * Definitions
 */

typedef enum {
	POPUP__LIST_TYPE_UNKNOWN = -1,
	MARK_AS_READ,
	MARK_AS_UNREAD,
	SET_AS_FAVOURITE,
	SET_AS_UNFAVOURITE,
	SET_AS_FLAG,
	SET_AS_COMPLETE,
	SET_AS_UNFLAG,
	POPUP_LIST_TYPE_MAX,
} MailboxPopupListIndex;

typedef enum {
	MARK_READ_UNREAD_POPUP = MARK_AS_READ,
	SET_FAVOURITE_UNFAVOURITE_POPUP = SET_AS_FAVOURITE,
	SET_FLAG_AS_POPUP = SET_AS_FLAG,
	POPUP_TYPE_MAX,
} MailboxPopupType;

/*
 * Structures
 */

/*
 * Globals
 */

/*
 * Declaration for static functions
 */
static void _create_control_option(EmailMailboxUGD *ad);
static void _create_edit_control_option(EmailMailboxUGD *ad);

static void _popup_response_delete_ok_cb(void *data, Evas_Object *obj, void *event_info);

/*
 * Definition for static functions
 */

static void _error_popup_destroy_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;
	DELETE_EVAS_OBJECT(mailbox_ugd->error_popup);
}

static void _error_popup_mouseup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Event_Mouse_Up *ev = event_info;
	if (ev->button == 3) { /* if mouse right button is up */
		_error_popup_destroy_cb(data, obj, NULL);
	}
}

static void _create_control_option(EmailMailboxUGD *mailbox_ugd)
{
	debug_enter();

	if (!mailbox_ugd->controlbar_more_btn) {
		mailbox_ugd->controlbar_more_btn = mailbox_create_toolbar_more_btn(mailbox_ugd);
		elm_object_item_part_content_set(mailbox_ugd->base.navi_item, "toolbar_more_btn", mailbox_ugd->controlbar_more_btn);
	}
	return;
}

static void _create_edit_control_option(EmailMailboxUGD *mailbox_ugd)
{
	debug_enter();

	int i = 0;
	bool use_ReadUnread_popup = false;
	bool use_StarUnstar_popup = false;
	int checked_count = eina_list_count(mailbox_ugd->selected_mail_list);
	Eina_List *first_list = NULL;
	MailItemData *first_ld = NULL;
	int first_read_status = 0;
	int first_favourite_status = 0;

	if (checked_count <= 0) {
		debug_log("selected count %d", checked_count);
		checked_count = 0;
	} else {
		first_list = eina_list_nth_list(mailbox_ugd->selected_mail_list, 0);
		first_ld = eina_list_data_get(first_list);
		first_read_status = first_ld->is_seen;
		first_favourite_status = first_ld->flag_type;
	}

	for (i = 1; checked_count > 1 && i < checked_count; i++) {
		Eina_List *nth_list = eina_list_nth_list(mailbox_ugd->selected_mail_list, i);
		MailItemData *ld = eina_list_data_get(nth_list);

		if (ld->is_seen != first_read_status && false == use_ReadUnread_popup) {
			use_ReadUnread_popup = true;
		}
		if (first_favourite_status < EMAIL_FLAG_TASK_STATUS_CLEAR && ld->flag_type != first_favourite_status && false == use_StarUnstar_popup) {
			use_StarUnstar_popup = true;
		}
	}

	debug_log("checked_count[%d], use_ReadUnread_popup[%d], use_StarUnstar_popup[%d]", checked_count, use_ReadUnread_popup, use_StarUnstar_popup);
	return;
}

/*
 * Definition for exported functions
 */

void mailbox_toolbar_create(EmailMailboxUGD *mailbox_ugd)
{
	debug_enter();
	retm_if(!mailbox_ugd, "mailbox_ugd is NULL.");

	if (mailbox_ugd->b_editmode) {
		_create_edit_control_option(mailbox_ugd);
	} else {
		_create_control_option(mailbox_ugd);
	}
}

static void _popup_response_delete_ok_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;

	edit_req_t *req = MEM_ALLOC(req, 1);
	retm_if(!req, "memory allocation failed.");

	Eina_List *cur = NULL;
	MailItemData *ld = NULL;
	EINA_LIST_FOREACH(mailbox_ugd->selected_mail_list, cur, ld) {
		DeleteRequestedMail *requested_mail = MEM_ALLOC(requested_mail, 1);
		requested_mail->account_id = ld->account_id;
		requested_mail->mail_id = ld->mail_id;
		requested_mail->thread_id = ld->thread_id;
		requested_mail->mailbox_type = ld->mailbox_type;
		requested_mail->mail_status = ld->mail_status;
		req->requested_mail_list = eina_list_append(req->requested_mail_list, requested_mail);
	}
	req->mailbox_ugd = mailbox_ugd;
	ecore_thread_feedback_run(mailbox_process_delete_mail, NULL, NULL, NULL, req, EINA_TRUE);

	mailbox_exit_edit_mode(mailbox_ugd);
}

void mailbox_move_mail(void *data)
{
	debug_enter();

	if (data == NULL) {
		debug_log("data == NULL");
		return;
	}

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;

	if (mailbox_ugd->mode == EMAIL_MAILBOX_MODE_ALL || mailbox_ugd->mode == EMAIL_MAILBOX_MODE_PRIORITY_SENDER) {
		MailItemData *ld = eina_list_data_get(mailbox_ugd->selected_mail_list);
		int first_account_id = ld->account_id;

		Eina_List *cur = NULL;
		EINA_LIST_FOREACH(mailbox_ugd->selected_mail_list, cur, ld) {
			if (first_account_id != ld->account_id) {
				debug_log("first_account_id : %d, account_id : %d", first_account_id, ld->account_id);
				DELETE_EVAS_OBJECT(mailbox_ugd->error_popup);
				mailbox_ugd->error_popup = mailbox_create_popup(mailbox_ugd, _("IDS_EMAIL_HEADER_UNABLE_TO_MOVE_EMAILS_ABB"), _("IDS_EMAIL_POP_UNABLE_TO_MOVE_EMAILS_FROM_MULTIPLE_ACCOUNTS_AT_ONCE"), _error_popup_destroy_cb,
				_error_popup_destroy_cb, "IDS_EMAIL_BUTTON_OK", NULL, NULL);
				evas_object_event_callback_add(mailbox_ugd->error_popup, EVAS_CALLBACK_MOUSE_UP, _error_popup_mouseup_cb, mailbox_ugd);
				return;
			}
		}
	}

	mailbox_process_move_mail(mailbox_ugd);
}

void mailbox_from_spam_mail(void *data)
{
	debug_enter();

	if (data == NULL) {
		debug_log("data == NULL");
		return;
	}

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;

	DELETE_EVAS_OBJECT(mailbox_ugd->more_ctxpopup);

	int i = 0, unblock_count = 0, j = 0;

	int max_account_id = email_engine_get_max_account_id();
	debug_log("max_account_id %d", max_account_id);
	if (max_account_id < 0) {
		debug_error("Invalid max_account_id(%d) was returned.", max_account_id);
		return;
	}

	int checked_count = eina_list_count(mailbox_ugd->selected_mail_list);
	if (checked_count <= 0) return;

	GList *mail_list[max_account_id];
	memset(mail_list, 0, max_account_id*sizeof(GList *));

	int ret = 0;
	for (j = 0; j < checked_count; j++) {
		Eina_List *nth_list = eina_list_nth_list(mailbox_ugd->selected_mail_list, j);

		MailItemData *ld = eina_list_data_get(nth_list);
		mail_list[ld->account_id-1] = g_list_prepend(mail_list[ld->account_id-1], GINT_TO_POINTER(ld->mail_id));
		unblock_count++;
	}

	if (unblock_count > 0) {
		if (unblock_count == 1) {
			char str[255] = {0, };
			snprintf(str, sizeof(str), email_get_email_string("IDS_EMAIL_TPOP_1_EMAIL_MOVED_TO_PS"), email_get_email_string("IDS_EMAIL_HEADER_INBOX"));
			ret = notification_status_message_post(str);
		} else {
			char str[255] = {0, };
			snprintf(str, sizeof(str), email_get_email_string("IDS_EMAIL_TPOP_P1SD_EMAILS_MOVED_TO_P2SS"), unblock_count, email_get_email_string("IDS_EMAIL_HEADER_INBOX"));
			ret = notification_status_message_post(str);
		}
		if (ret != NOTIFICATION_ERROR_NONE) {
			debug_log("notification_status_message_post() failed: %d", ret);
		}

		for (i = 0; i < max_account_id; i++) {
			if (!mail_list[i]) continue;
			else {
				int count = g_list_length(mail_list[i]);
				int mail_ids[count];
				int err = 0;
				memset(mail_ids, 0, sizeof(mail_ids));

				int j = 0;
				GList *cur = g_list_first(mail_list[i]);
				for (j = 0; j < count; j++, cur = g_list_next(cur))
					mail_ids[j] = (int)(ptrdiff_t)g_list_nth_data(cur, 0);

				debug_log("account_id : %d, count : %d", i+1, count);

				email_mailbox_t *inbox = NULL;
				err = email_get_mailbox_by_mailbox_type(i+1, EMAIL_MAILBOX_TYPE_INBOX, &inbox);
				if (err == EMAIL_ERROR_NONE && inbox) {
					err = email_move_mail_to_mailbox(mail_ids, count, inbox->mailbox_id);
					if (err != EMAIL_ERROR_NONE) {
						debug_warning("email_move_mail_to_mailbox acct(%d) mailbox_id(%d) num(%d) - err (%d)",
								i+1, inbox->mailbox_id, count, err);
					}
					email_free_mailbox(&inbox, 1);
				} else {
					debug_warning("email_get_mailbox_by_mailbox_type : account_id(%d) type(INBOX) - err(%d) or mailbox is NULL(%p)", mailbox_ugd->account_id, err, inbox);
				}
			}
		}
	}

	if (unblock_count > 0) {
		for (i = 0; i < max_account_id; ++i) {
			if (mail_list[i] != NULL) {
				g_list_free(mail_list[i]);
				mail_list[i] = NULL;
			}
		}
	}

	mailbox_exit_edit_mode(mailbox_ugd);
	debug_leave();
}

void mailbox_to_spam_mail(void *data)
{
	debug_enter();

	if (data == NULL) {
		debug_log("data == NULL");
		return;
	}

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;
	int i = 0, block_count = 0;
	int max_account_id = email_engine_get_max_account_id();
	int ret = 1;

	if (max_account_id < 0) {
		debug_error("Invalid max_account_id(%d) was returned.", max_account_id);
		return;
	}

	int checked_count = eina_list_count(mailbox_ugd->selected_mail_list);

	DELETE_EVAS_OBJECT(mailbox_ugd->more_ctxpopup);

	if (checked_count <= 0) return;

	GList *mail_list[max_account_id];
	memset(mail_list, 0, max_account_id * sizeof(GList *));

	for (i = 0; i < checked_count; i++) {
		Eina_List *nth_list = eina_list_nth_list(mailbox_ugd->selected_mail_list, i);

		MailItemData *ld = eina_list_data_get(nth_list);
		mail_list[ld->account_id-1] = g_list_prepend(mail_list[ld->account_id-1], GINT_TO_POINTER(ld->mail_id));
		block_count++;
	}

	if (block_count > 0) {
		if (block_count == 1) {
			char str[255] = {0, };
			snprintf(str, sizeof(str), email_get_email_string("IDS_EMAIL_TPOP_1_EMAIL_MOVED_TO_PS"), email_get_email_string("IDS_EMAIL_HEADER_SPAMBOX"));
			ret = notification_status_message_post(str);
			if (ret != NOTIFICATION_ERROR_NONE) {
			debug_log("notification_status_message_post() failed: %d", ret);
			}
		} else {
			char str[255] = {0, };
			snprintf(str, sizeof(str), email_get_email_string("IDS_EMAIL_TPOP_P1SD_EMAILS_MOVED_TO_P2SS"), block_count, email_get_email_string("IDS_EMAIL_HEADER_SPAMBOX"));
			ret = notification_status_message_post(str);
		}
		if (ret != NOTIFICATION_ERROR_NONE) {
			debug_log("notification_status_message_post() failed: %d", ret);
		}

		mailbox_ugd->need_deleted_noti = false;

		for (i = 0; i < max_account_id; i++) {
			if (!mail_list[i]) continue;
			else {
				int count = g_list_length(mail_list[i]);
				int mail_ids[count];
				memset(mail_ids, 0, sizeof(mail_ids));

				int j = 0;
				GList *cur = g_list_first(mail_list[i]);
				for (j = 0; j < count; j++, cur = g_list_next(cur))
					mail_ids[j] = (int)(ptrdiff_t)g_list_nth_data(cur, 0);

				debug_log("account_id : %d, count : %d", i+1, count);

				email_mailbox_t *spambox = NULL;
				email_get_mailbox_by_mailbox_type(i+1, EMAIL_MAILBOX_TYPE_SPAMBOX, &spambox);
				if (spambox) {
					int err = email_move_mail_to_mailbox(mail_ids, count, spambox->mailbox_id);
					if (err != EMAIL_ERROR_NONE) {
						debug_warning("email_move_mail_to_mailbox acct(%d) mailbox_id(%d) num(%d) - err (%d)",
								i+1, spambox->mailbox_id, count, err);
					}
					email_free_mailbox(&spambox, 1);
				}
			}
		}
	}

	if (block_count > 0) {
		for (i = 0; i < max_account_id; ++i) {
			if (mail_list[i] != NULL) {
				g_list_free(mail_list[i]);
				mail_list[i] = NULL;
			}
		}
	}
	mailbox_exit_edit_mode(mailbox_ugd);
}

void mailbox_markunread_mail(void *data)
{
	debug_enter();

	if (data == NULL) {
		debug_log("data == NULL");
		return;
	}

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;
	int i = 0;
	int max_account_id = email_engine_get_max_account_id();
	int ret = 0;

	if (max_account_id < 0) {
		debug_error("Invalid max_account_id(%d) was returned.", max_account_id);
		return;
	}

	int checked_count = eina_list_count(mailbox_ugd->selected_mail_list);

	DELETE_EVAS_OBJECT(mailbox_ugd->more_ctxpopup);

	if (checked_count <= 0) return;

	GList *mail_list[max_account_id];
	memset(mail_list, 0, max_account_id * sizeof(GList *));

	for (i = 0; i < checked_count; i++) {
		Eina_List *nth_list = eina_list_nth_list(mailbox_ugd->selected_mail_list, i);
		MailItemData *ld = eina_list_data_get(nth_list);
		mail_list[ld->account_id-1] = g_list_prepend(mail_list[ld->account_id-1], GINT_TO_POINTER(ld->mail_id));
	}

	for (i = 0; i < max_account_id; i++) {
		if (!mail_list[i]) continue;
		else {
			int count = g_list_length(mail_list[i]);
			int mail_ids[count];
			memset(mail_ids, 0, sizeof(mail_ids));

			int j = 0;
			GList *cur = g_list_first(mail_list[i]);
			for (j = 0; j < count; j++, cur = g_list_next(cur))
				mail_ids[j] = (int)(ptrdiff_t)g_list_nth_data(cur, 0);

			debug_log("account_id : %d, count : %d", i+1, count);

			int err = email_set_flags_field(i+1, mail_ids, count, EMAIL_FLAGS_SEEN_FIELD, 0, 1);
			if (err != EMAIL_ERROR_NONE)
				debug_log("email_set_flags_field - err(%d)", err);

		}
	}

	if (1 == checked_count) {
		ret = notification_status_message_post(email_get_email_string("IDS_EMAIL_TPOP_1_EMAIL_MARKED_AS_UNREAD"));
	} else {
		char str[255] = {0, };
		snprintf(str, sizeof(str), email_get_email_string("IDS_EMAIL_TPOP_PD_EMAILS_MARKED_AS_UNREAD"), checked_count);
		ret = notification_status_message_post(str);
	}
	if (ret != NOTIFICATION_ERROR_NONE) {
		debug_log("notification_status_message_post() failed: %d", ret);
	}

	mailbox_exit_edit_mode(mailbox_ugd);

	for (i = 0; i < max_account_id; ++i) {
		if (mail_list[i] != NULL) {
			g_list_free(mail_list[i]);
			mail_list[i] = NULL;
		}
	}
}

void mailbox_markread_mail(void *data)
{
	debug_enter();

	if (data == NULL) {
		debug_log("data == NULL");
		return;
	}

	EmailMailboxUGD *mailbox_ugd = (EmailMailboxUGD *)data;
	int i = 0;
	int max_account_id = email_engine_get_max_account_id();
	int ret = 0;

	if (max_account_id < 0) {
		debug_error("Invalid max_account_id(%d) was returned.", max_account_id);
		return;
	}

	int checked_count = eina_list_count(mailbox_ugd->selected_mail_list);

	DELETE_EVAS_OBJECT(mailbox_ugd->more_ctxpopup);

	if (checked_count <= 0) return;

	GList *mail_list[max_account_id];
	memset(mail_list, 0, max_account_id * sizeof(GList *));

	for (i = 0; i < checked_count; i++) {
		Eina_List *nth_list = eina_list_nth_list(mailbox_ugd->selected_mail_list, i);
		MailItemData *ld = eina_list_data_get(nth_list);
		mail_list[ld->account_id-1] = g_list_prepend(mail_list[ld->account_id-1], GINT_TO_POINTER(ld->mail_id));
	}

	for (i = 0; i < max_account_id; i++) {
		if (!mail_list[i]) continue;
		else {
			int count = g_list_length(mail_list[i]);
			int mail_ids[count];
			memset(mail_ids, 0, sizeof(mail_ids));

			int j = 0;
			GList *cur = g_list_first(mail_list[i]);
			for (j = 0; j < count; j++, cur = g_list_next(cur))
				mail_ids[j] = (int)(ptrdiff_t)g_list_nth_data(cur, 0);

			debug_log("account_id : %d, count : %d", i+1, count);

			int err = email_set_flags_field(i+1, mail_ids, count, EMAIL_FLAGS_SEEN_FIELD, 1, 1);
			if (err != EMAIL_ERROR_NONE)
				debug_log("email_set_flags_field - err(%d)", err);

		}
	}

	if (1 == checked_count) {
		ret = notification_status_message_post(email_get_email_string("IDS_EMAIL_TPOP_1_EMAIL_MARKED_AS_READ"));
	} else {
		char str[255] = {0, };
		snprintf(str, sizeof(str), email_get_email_string("IDS_EMAIL_TPOP_PD_EMAILS_MARKED_AS_READ"), checked_count);
		ret = notification_status_message_post(str);
	}
	if (ret != NOTIFICATION_ERROR_NONE) {
		debug_log("notification_status_message_post() failed: %d", ret);
	}

	mailbox_exit_edit_mode(mailbox_ugd);

	for (i = 0; i < max_account_id; ++i) {
		if (mail_list[i] != NULL) {
			g_list_free(mail_list[i]);
			mail_list[i] = NULL;
		}
	}
}

void mailbox_delete_mail(void *data)
{
	debug_enter();
	if (data == NULL) {
		debug_log("data == NULL");
		return;
	}
	_popup_response_delete_ok_cb(data, NULL, NULL);

}
