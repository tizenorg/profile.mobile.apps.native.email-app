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

#include "email-common-types.h"
#include "email-mailbox.h"
#include "email-mailbox-request.h"
#include "email-mailbox-list.h"
#include "email-mailbox-list-other-items.h"
#include "email-mailbox-noti-mgr.h"
#include "email-mailbox-search.h"
#include "email-mailbox-title.h"
#include "email-mailbox-util.h"
#include "email-mailbox-sync.h"


/*
 * Declaration for static functions
 */

static void _parse_move_mail_buf(char *inbuf, char **src_folder, char **dst_folder, GList **mail_list);
static void _parse_delete_mail_buf(char *inbuf, GList **mail_list);
static void _mailbox_add_remaining_mail_req_cb(email_request_h request);
static void _mailbox_add_remaining_mail_req_add_item_cb(email_request_h request, void *msg_data);
static void _mailbox_add_remaining_mail_req_end_cb(email_request_h request);
static void _mailbox_move_mail_req_cb(email_request_h request);
static void _mailbox_move_mail_req_move_item_cb(email_request_h request, void *msg_data);
static void _mailbox_move_mail_req_end_cb(email_request_h request);
static void _mailbox_del_mail_req_cb(email_request_h request);
static void _mailbox_del_mail_req_del_item_cb(email_request_h request, void *msg_data);
static void _mailbox_del_mail_req_end_cb(email_request_h request);
static void _mailbox_add_mail_req_cb(email_request_h request);
static void _mailbox_add_mail_req_add_seed_mail_cb(email_request_h request, void *msg_data);
static void _mailbox_add_mail_req_end_cb(email_request_h request);

/*
 * Definitions
 */

#define WORKER_SLEEP_PERIOD	2
#define CHECK_THREAD_BUSY() \
	({\
		while (is_main_thread_busy) { \
			sleep(WORKER_SLEEP_PERIOD); \
		} \
	})


/*
 * Globals
 */

static int is_main_thread_busy = false;


/*
 * Definition for static functions
 */

static void _parse_move_mail_buf(char *inbuf, char **src_folder, char **dst_folder, GList **mail_list)
{
	debug_enter();

	if (!STR_VALID(inbuf)) {
		return;
	}

	/* notification format: <src_folder><0x01><dst_folder><0x01><<mail_id><,><mail_id>>*/
	gchar **outer_tok;
	char delimiter[2] = {0x01, 0x00};
	outer_tok = g_strsplit_set(inbuf, delimiter, -1);
	if (outer_tok == NULL) {
		debug_warning("outer_tok == NULL");
		return;
	}
	if (outer_tok[0] && strlen(outer_tok[0]) > 0) {
		*src_folder = strdup(outer_tok[0]);
	}
	if (outer_tok[1] && strlen(outer_tok[1]) > 0) {
		*dst_folder = strdup(outer_tok[1]);
	}
	if (outer_tok[2] && strlen(outer_tok[2]) > 0) {
		gchar **inner_tok;
		int i = 0;
		inner_tok = g_strsplit_set(outer_tok[2], ",", -1);
		for (i = 0; i < g_strv_length(inner_tok) - 1; ++i) {
			debug_log("%s", inner_tok[i]);
			if (!strcmp(inner_tok[i], "\"")) {/* skip the empty token */
				continue;
			} else {
				int *mail_id = (int *)calloc(1, sizeof(int));
				IF_NULL_FREE_2GSTRA_AND_RETURN(mail_id, outer_tok, inner_tok);
				*mail_id = atoi(inner_tok[i]);
				*mail_list = g_list_append(*mail_list, mail_id);
			}
		}
		g_strfreev(inner_tok);
	}
	g_strfreev(outer_tok);
	debug_secure("src_folder: [%s], dst_folder: [%s]", *src_folder, *dst_folder);
}

static void _parse_delete_mail_buf(char *inbuf, GList **mail_list)
{
	debug_enter();

	if (!STR_VALID(inbuf)) {
		return;
	}

	/* notification format: <<mail_id><,><mail_id>>*/
	gchar **inner_tok;
	int i = 0;
	inner_tok = g_strsplit_set(inbuf, ",", -1);
	for (i = 0; i < g_strv_length(inner_tok) - 1; ++i) {
		debug_log("%s", inner_tok[i]);
		if (!strcmp(inner_tok[i], "\"")) { /* skip the empty token */
			continue;
		} else {
			int *mail_id = (int *)calloc(1, sizeof(int));
			IF_NULL_FREE_2GSTRA_AND_RETURN(mail_id, inner_tok, NULL);
			*mail_id = atoi(inner_tok[i]);
			*mail_list = g_list_append(*mail_list, mail_id);
		}
	}
	g_strfreev(inner_tok);

}

void mailbox_set_main_thread_busy_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *o EINA_UNUSED, void *info EINA_UNUSED)
{
	debug_enter();

	is_main_thread_busy = true;
}

void mailbox_reset_main_thread_busy_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *oEINA_UNUSED, void *info EINA_UNUSED)
{
	debug_enter();

	is_main_thread_busy = false;
}

void mailbox_requests_cbs_register()
{
	debug_enter();

	email_register_request_cbs(EMAIL_REQUEST_TYPE_ADD_REMAINING_MAIL,
							_mailbox_add_remaining_mail_req_cb,
							_mailbox_add_remaining_mail_req_add_item_cb,
							_mailbox_add_remaining_mail_req_end_cb);

	email_register_request_cbs(EMAIL_REQUEST_TYPE_MOVE_MAIL,
							_mailbox_move_mail_req_cb,
							_mailbox_move_mail_req_move_item_cb,
							_mailbox_move_mail_req_end_cb);

	email_register_request_cbs(EMAIL_REQUEST_TYPE_DELETE_MAIL,
							_mailbox_del_mail_req_cb,
							_mailbox_del_mail_req_del_item_cb,
							_mailbox_del_mail_req_end_cb);

	email_register_request_cbs(EMAIL_REQUEST_TYPE_ADD_MAIL,
							_mailbox_add_mail_req_cb,
							_mailbox_add_mail_req_add_seed_mail_cb,
							_mailbox_add_mail_req_end_cb);

	debug_leave();
}

void mailbox_requests_cbs_unregister()
{
	debug_enter();

	email_unregister_request_cbs(EMAIL_REQUEST_TYPE_ADD_REMAINING_MAIL);
	email_unregister_request_cbs(EMAIL_REQUEST_TYPE_MOVE_MAIL);
	email_unregister_request_cbs(EMAIL_REQUEST_TYPE_DELETE_MAIL);
	email_unregister_request_cbs(EMAIL_REQUEST_TYPE_ADD_MAIL);

	debug_leave();
}

void mailbox_cancel_all_requests(EmailMailboxView *view)
{
	debug_enter();

	if (view->request_queue) {
		email_request_queue_cancel_all_requests(view->request_queue);
	}

	debug_leave();
}

static void _mailbox_add_remaining_mail_req_cb(email_request_h request)
{
	debug_enter();

	retm_if(!request, "param request is NULL");

	AddRemainingMailReqData *req_data = (AddRemainingMailReqData *)email_request_get_data(request);
	retm_if(!req_data, "request data is NULL");

	GList *ld_list = NULL;
	email_mail_list_item_t *mail_list = req_data->mail_list;
	int count = req_data->count;

	int i =req_data->start;
	for (; i < count; i++) {
		MailItemData *ld = mailbox_list_make_mail_item_data(mail_list+i, req_data->search_data, req_data->view);
		if (!ld) {
			continue;
		}

		ld_list = g_list_append(ld_list, ld);

		if (email_request_check(request)) {
			debug_log("request is cancelled");
			g_list_foreach(ld_list, (GFunc)mailbox_list_free_mail_item_data, (void *)NULL);
			g_list_free(ld_list);
			goto CLEANUP;
		}

		if (!(i%50)) {
			debug_log("feeding to main(%d)", i);
			email_request_send_feedback(request, ld_list);
			ld_list = NULL;
		}
	}

	if ((i-1)%50) {
		debug_log("feeding to main(%d)", i);
		email_request_send_feedback(request, ld_list);
		ld_list = NULL;
	}

CLEANUP:
	FREE(req_data->mail_list);
	debug_leave();
}

static void _mailbox_add_remaining_mail_req_add_item_cb(email_request_h request, void *msg_data)
{
	debug_enter();

	retm_if(!msg_data, "param msg_data is NULL", msg_data);
	gotom_if(!request, CLEANUP, "param request is NULL");

	AddRemainingMailReqData *req_data = (AddRemainingMailReqData *)email_request_get_data(request);
	gotom_if(!req_data, CLEANUP, "request data is NULL");
	gotom_if(email_request_check(request), CLEANUP, "request is cancelled");

	EmailMailboxView *view = req_data->view;
	GList *ld_list = (GList *)msg_data;
	g_list_foreach(ld_list, (GFunc)mailbox_list_insert_mail_item, (void *)view);
	g_list_free(ld_list);
	if (view->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX) {
		mailbox_send_all_btn_remove(view);
		mailbox_send_all_btn_add(view);
	}
	return;

CLEANUP:
	if (msg_data) {
		GList *ld_list = (GList *)msg_data;
		g_list_foreach(ld_list, (GFunc)mailbox_list_free_mail_item_data, (void *)NULL);
		g_list_free(ld_list);
		debug_leave();
	}
}

static void _mailbox_add_remaining_mail_req_end_cb(email_request_h request)
{
	debug_enter();

	retm_if(!request, "param request is NULL");

	AddRemainingMailReqData *req_data = (AddRemainingMailReqData *)email_request_get_data(request);
	retm_if(!req_data, "request data is NULL");

	if (req_data->search_data) {
		mailbox_free_mailbox_search_data(req_data->search_data);
	}

	FREE(req_data);
	debug_leave();
}

static void _mailbox_move_mail_req_cb(email_request_h request)
{
	debug_enter();

	retm_if(!request, "param request is NULL");
	MoveMailReqData *req_data = (MoveMailReqData *)email_request_get_data(request);
	retm_if(!req_data, "request data is NULL");

	CHECK_THREAD_BUSY();
	debug_log("wake up now");

	EmailMailboxView *view = req_data->view;
	char *src_folder = NULL;
	char *dst_folder = NULL;
	int src_mailbox_id = 0, dst_mailbox_id = 0;
	GList *mail_list = NULL;
	MailItemData *ld = NULL;

	GList *cur = NULL;
	int *idx = NULL;

	_parse_move_mail_buf(req_data->msg_buf, &src_folder, &dst_folder, &mail_list);

	if (!STR_VALID(src_folder) || !STR_VALID(dst_folder)) {
		goto CLEANUP;
	}

	src_mailbox_id = atoi(src_folder);
	dst_mailbox_id = atoi(dst_folder);

	if (src_mailbox_id == dst_mailbox_id) {
		goto CLEANUP;
	}

	int dst_mailbox_type = GET_MAILBOX_TYPE(dst_mailbox_id);

	debug_log("b_searchmode : %d, b_editmode : %d", view->b_searchmode, view->b_editmode);

	G_LIST_FOREACH(mail_list, cur, idx) {
		ld = mailbox_list_get_mail_item_data_by_mailid(*idx, view->mail_list);
		if (ld) {
			MoveMailReqReturnData *move_ret_data = MEM_ALLOC(move_ret_data, 1);
			retm_if(!move_ret_data, "memory allocation failed.");
			*move_ret_data = (MoveMailReqReturnData) {*idx, ld, true};

			if (email_request_check(request)) {
				debug_log("request is cancelled");
				FREE(move_ret_data);
				break;
			}

			CHECK_THREAD_BUSY();
			debug_log("feeding to main");
			email_request_send_feedback(request, move_ret_data);
			debug_log("Mail(%d) found in this view", *idx);
		} else if ((view->mode == EMAIL_MAILBOX_MODE_ALL || view->account_id == req_data->account_id)
			&& view->mailbox_type == EMAIL_MAILBOX_TYPE_SENTBOX
			&& dst_mailbox_type == EMAIL_MAILBOX_TYPE_SENTBOX) {
			debug_log("add sent email in Sentbox");

			gotom_if(!email_engine_open_db(), CLEANUP, "fail to open db");

			email_mail_list_item_t *mail_info = email_engine_get_mail_by_mailid(*idx);
			gotom_if(!mail_info, CLEANUP, "no email exits(%d)", *idx);

			debug_warning_if(!email_engine_close_db(), "fail to close db");

			MailItemData *ld = mailbox_list_make_mail_item_data(mail_info, NULL, view);
			gotom_if(!ld, CLEANUP, "mailbox_list_make_mail_item_data() failed.");

			MoveMailReqReturnData *move_ret_data = MEM_ALLOC(move_ret_data, 1);
			if (!move_ret_data) {
				debug_error("memory allocation failed.");
				FREE(ld);
				goto CLEANUP;
			}
			*move_ret_data = (MoveMailReqReturnData) {*idx, ld, false};

			if (email_request_check(request)) {
				debug_log("request is cancelled");
				FREE(move_ret_data);
				break;
			}

			CHECK_THREAD_BUSY();
			debug_log("feeding to main");
			email_request_send_feedback(request, move_ret_data);
		}
	}

CLEANUP:
	if (mail_list) {
		G_LIST_FOREACH(mail_list, cur, idx) {
			FREE(idx);
		}
		g_list_free(mail_list);
		mail_list = NULL;
	}
	FREE(src_folder);
	FREE(dst_folder);

	debug_leave();
	return;
}

static void _mailbox_move_mail_req_move_item_cb(email_request_h request, void *msg_data)
{
	debug_enter();

	retm_if(!msg_data, "param msg_data is NULL");
	gotom_if(!request, CLEANUP, "param request is NULL");

	MoveMailReqData *req_data = (MoveMailReqData *)email_request_get_data(request);
	gotom_if(!req_data, CLEANUP, "request data is NULL");
	gotom_if(email_request_check(request), CLEANUP, "request is cancelled");

	MoveMailReqReturnData *move_ret_data = (MoveMailReqReturnData *)msg_data;
	MailItemData *ld = move_ret_data->ld;
	EmailMailboxView *view = req_data->view;

	if (view->b_editmode) {
		debug_log("edit list update first");
		mailbox_update_edit_list_view(ld, view);
	}

	if (move_ret_data->is_delete) {
		if (!mailbox_list_get_mail_item_data_by_mailid(move_ret_data->mail_id, view->mail_list)) {
			debug_log("mailbox_list_get_mail_item_data_by_mailid(%d) failed.", move_ret_data->mail_id);
			goto CLEANUP;
		}

		mailbox_list_remove_mail_item(view, ld);

		if (g_list_length(view->mail_list) > 0) {
			if (!view->b_searchmode && !view->b_editmode
				&& ((view->mode == EMAIL_MAILBOX_MODE_MAILBOX && view->only_local == false
					&& view->mailbox_type != EMAIL_MAILBOX_TYPE_OUTBOX
					&& view->account_type == EMAIL_SERVER_TYPE_IMAP4)
					|| (view->mode == EMAIL_MAILBOX_MODE_MAILBOX && view->only_local == false
					&& view->mailbox_type == EMAIL_MAILBOX_TYPE_INBOX
					&& view->account_type == EMAIL_SERVER_TYPE_POP3))) {
				mailbox_load_more_messages_item_add(view);
			} else if (!view->b_searchmode && !view->b_editmode
					&& view->mailbox_type != EMAIL_MAILBOX_TYPE_OUTBOX) {
				mailbox_no_more_emails_item_add(view);
			}
		} else {
			mailbox_show_no_contents_view(view);
		}
	} else {
		mailbox_load_more_messages_item_remove(view);
		mailbox_no_more_emails_item_remove(view);
		mailbox_hide_no_contents_view(view);
		mailbox_list_insert_mail_item_from_noti(&ld, view);
	}

CLEANUP:
	FREE(msg_data);
	debug_leave();
}

static void _mailbox_move_mail_req_end_cb(email_request_h request)
{
	debug_enter();

	retm_if(!request, "param request is NULL");
	MoveMailReqData *req_data = (MoveMailReqData *)email_request_get_data(request);
	retm_if(!req_data, "request data is NULL");

	FREE(req_data->msg_buf);
	FREE(req_data);
	debug_leave();
}

static void _mailbox_del_mail_req_cb(email_request_h request)
{
	debug_enter();

	retm_if(!request, "param request is NULL");
	DeleteMailReqData *req_data = (DeleteMailReqData *)email_request_get_data(request);
	retm_if(!req_data, "request data is NULL");

	CHECK_THREAD_BUSY();
	debug_log("wake up now");

	EmailMailboxView *view = req_data->view;
	GList *mail_list = NULL;
	MailItemData *ld = NULL;

	GList *cur = NULL;
	int *idx = NULL;

	_parse_delete_mail_buf(req_data->msg_buf, &mail_list);

	debug_log("b_searchmode : %d, b_editmode : %d", view->b_searchmode, view->b_editmode);

	G_LIST_FOREACH(mail_list, cur, idx) {
		debug_log("idx: [%d]", *idx);
		ld = mailbox_list_get_mail_item_data_by_mailid(*idx, view->mail_list);
		if (ld) {
			CHECK_THREAD_BUSY();
			DeleteMailReqReturnData *delete_ret_data = MEM_ALLOC(delete_ret_data, 1);
			if (!delete_ret_data) {
				return;
			}
			*delete_ret_data = (DeleteMailReqReturnData) {ld, *idx};

			if (email_request_check(request)) {
				debug_log("request is cancelled");
				FREE(delete_ret_data);
				break;
			}

			debug_log("feeding to main");
			email_request_send_feedback(request, delete_ret_data);
		}
	}

	if (mail_list) {
		G_LIST_FOREACH(mail_list, cur, idx) {
			FREE(idx);
		}
		g_list_free(mail_list);
		mail_list = NULL;
	}

	debug_leave();
}

static void _mailbox_del_mail_req_del_item_cb(email_request_h request, void *msg_data)
{
	debug_enter();

	retm_if(!msg_data, "param msg_data is NULL");
	gotom_if(!request, CLEANUP, "param request is NULL");

	DeleteMailReqData *req_data = (DeleteMailReqData *)email_request_get_data(request);
	gotom_if(!req_data, CLEANUP, "request data is NULL");
	gotom_if(email_request_check(request), CLEANUP, "request is cancelled");

	DeleteMailReqReturnData *delete_ret_data = (DeleteMailReqReturnData *)msg_data;
	MailItemData *ld = delete_ret_data->ld;
	EmailMailboxView *view = req_data->view;

	if (!mailbox_list_get_mail_item_data_by_mailid(delete_ret_data->mail_id, view->mail_list)) {
		goto CLEANUP;
	}

	if (view->b_editmode) {
		debug_log("edit list update first");
		mailbox_update_edit_list_view(ld, view);
	}

	if (!(view->mailbox_type == EMAIL_MAILBOX_TYPE_DRAFT
		|| view->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX)) {
		if (ld->is_seen) {
			mailbox_view_title_increase_mail_count(-1, 0, view);
		} else {
			mailbox_view_title_increase_mail_count(-1, -1, view);
		}
	}

	mailbox_list_remove_mail_item(view, ld);

	if (g_list_length(view->mail_list) > 0) {
		if (!view->b_searchmode && !view->b_editmode
			&& ((view->mode == EMAIL_MAILBOX_MODE_MAILBOX && view->only_local == false
					&& view->mailbox_type != EMAIL_MAILBOX_TYPE_OUTBOX
					&& view->account_type == EMAIL_SERVER_TYPE_IMAP4)
					|| (view->mode == EMAIL_MAILBOX_MODE_MAILBOX && view->only_local == false
					&& view->mailbox_type == EMAIL_MAILBOX_TYPE_INBOX
					&& view->account_type == EMAIL_SERVER_TYPE_POP3))) {
			mailbox_load_more_messages_item_add(view);
		} else if (!view->b_searchmode && !view->b_editmode
				&& view->mailbox_type != EMAIL_MAILBOX_TYPE_OUTBOX) {
			mailbox_no_more_emails_item_add(view);
		}
	} else {
		mailbox_show_no_contents_view(view);
	}

CLEANUP:
	FREE(msg_data);
	debug_leave();
}

static void _mailbox_del_mail_req_end_cb(email_request_h request)
{
	debug_enter();

	retm_if(!request, "param request is NULL");
	DeleteMailReqData *req_data = (DeleteMailReqData *)email_request_get_data(request);
	retm_if(!req_data, "request data is NULL");

	FREE(req_data->msg_buf);
	FREE(req_data);
	debug_leave();
}

static void _mailbox_add_mail_req_cb(email_request_h request)
{
	debug_enter();

	retm_if(!request, "param request is NULL");
	AddMailReqData *req_data = (AddMailReqData *)email_request_get_data(request);
	retm_if(!req_data, "request data is NULL");

	CHECK_THREAD_BUSY();

	EmailMailboxView *view = req_data->view;
	email_mail_list_item_t *mail_info = NULL;

	gotom_if(!email_engine_open_db(), CLEANUP, "fail to open db");

	mail_info = email_engine_get_mail_by_mailid(req_data->mail_id);
	gotom_if(!mail_info, CLEANUP, "mail_info is NULL");

	debug_warning_if(!email_engine_close_db(), "fail to close db");

	if (mail_info->message_class == EMAIL_MESSAGE_CLASS_SMS && mail_info->save_status == EMAIL_MAIL_STATUS_SAVED_OFFLINE) {
		debug_log("This is EAS SMS message. It will be added on the next sync operation");
		goto CLEANUP;
	}
	MailItemData *ld = mailbox_list_make_mail_item_data(mail_info, NULL, view);
	gotom_if(!ld, CLEANUP, "ld is NULL");

	if (ld->mailbox_type == EMAIL_MAILBOX_TYPE_SEARCH_RESULT) {
		debug_log("This is Server Search result. (%d)", ld->mail_id);
		mailbox_list_free_mail_item_data(ld);
		goto CLEANUP;
	}

	ld->view = view;

	CHECK_THREAD_BUSY();

	email_request_send_feedback(request, ld);

 CLEANUP:

	FREE(mail_info);
	debug_leave();
}

static void _mailbox_add_mail_req_add_seed_mail_cb(email_request_h request, void *msg_data)
{
	debug_enter();

	gotom_if(!request, CLEANUP, "param request is NULL");
	retm_if(!msg_data, "param msg_data is NULL", msg_data);

	AddMailReqData *req_data = (AddMailReqData *)email_request_get_data(request);
	gotom_if(!req_data, CLEANUP, "request data is NULL");
	gotom_if(email_request_check(request), CLEANUP, "request is cancelled");

	EmailMailboxView *view = req_data->view;
	MailItemData *ld = (MailItemData *) msg_data;

	debug_log("thread_id(%d) mail_id(%d) info(%p)", req_data->thread_id, req_data->mail_id, ld);

	mailbox_get_more_progress_item_remove(view);

	debug_log("(req, view) mode(%d, %d), mailbox_type(%d, %d), mailbox_id(%d, %d)",
		req_data->mode, view->mode, req_data->mailbox_type, view->mailbox_type, req_data->mailbox_id, view->mailbox_id);

	if (req_data->mail_id == req_data->thread_id) {
		if ((req_data->mode == EMAIL_MAILBOX_MODE_ALL && view->mode == EMAIL_MAILBOX_MODE_ALL && req_data->mailbox_type == view->mailbox_type) ||
#ifndef _FEATURE_PRIORITY_SENDER_DISABLE_
			(req_data->mode == EMAIL_MAILBOX_MODE_PRIORITY_SENDER && view->mode == EMAIL_MAILBOX_MODE_PRIORITY_SENDER && req_data->mailbox_type == view->mailbox_type) ||
#endif
			(req_data->mode == EMAIL_MAILBOX_MODE_MAILBOX && view->mode == EMAIL_MAILBOX_MODE_MAILBOX && req_data->mailbox_id == view->mailbox_id)/* ||
			(req->mode == EMAIL_MAILBOX_MODE_SCHEDULED_OUTBOX && view->mode == EMAIL_MAILBOX_MODE_SCHEDULED_OUTBOX)*/) {
			mailbox_list_insert_mail_item_from_noti(&ld, view);
			if (req_data->need_increase_mail_count && ld) {
				if (ld->is_seen) {
					mailbox_view_title_increase_mail_count(1, 0, view);
				} else {
					mailbox_view_title_increase_mail_count(1, 1, view);
				}
			}
		} else {
			debug_log("list has been changed");
		}
	} else {
		if ((req_data->mode == EMAIL_MAILBOX_MODE_ALL && view->mode == EMAIL_MAILBOX_MODE_ALL && req_data->mailbox_type == view->mailbox_type) ||
#ifndef _FEATURE_PRIORITY_SENDER_DISABLE_
			(req_data->mode == EMAIL_MAILBOX_MODE_PRIORITY_SENDER && view->mode == EMAIL_MAILBOX_MODE_PRIORITY_SENDER && req_data->mailbox_type == view->mailbox_type) ||
#endif
			/*(req->mode == EMAIL_MAILBOX_MODE_SCHEDULED_OUTBOX && view->mode == EMAIL_MAILBOX_MODE_SCHEDULED_OUTBOX) ||*/
			(req_data->mode == EMAIL_MAILBOX_MODE_MAILBOX && view->mode == EMAIL_MAILBOX_MODE_MAILBOX && req_data->mailbox_id == view->mailbox_id)) {
			mailbox_list_insert_mail_item_from_noti(&ld, view);
			if (req_data->need_increase_mail_count && ld) {
				if (ld->is_seen)
					mailbox_view_title_increase_mail_count(1, 0, view);
				else
					mailbox_view_title_increase_mail_count(1, 1, view);
			}
		} else {
			debug_log("list has been changed");
		}
	}

	if (view->no_content_shown) {
		if (g_list_length(view->mail_list) > 0) {
			mailbox_hide_no_contents_view(view);
			if (!view->b_searchmode && !view->b_editmode
				&& ((view->mode == EMAIL_MAILBOX_MODE_MAILBOX && view->only_local == false
				&& view->mailbox_type != EMAIL_MAILBOX_TYPE_OUTBOX
				&& view->account_type == EMAIL_SERVER_TYPE_IMAP4)
			|| (view->mode == EMAIL_MAILBOX_MODE_MAILBOX && view->only_local == false
				&& view->mailbox_type == EMAIL_MAILBOX_TYPE_INBOX
				&& view->account_type == EMAIL_SERVER_TYPE_POP3))) {
				mailbox_load_more_messages_item_add(view);
			} else if (!view->b_searchmode && !view->b_editmode
				&& view->mailbox_type != EMAIL_MAILBOX_TYPE_OUTBOX) {
				mailbox_no_more_emails_item_add(view);
			}
		}
	}

	debug_leave();
	return;

 CLEANUP:
	if (msg_data) {
		mailbox_list_free_mail_item_data((MailItemData *) msg_data);
	}
	return;
}

static void _mailbox_add_mail_req_end_cb(email_request_h request)
{
	debug_enter();

	retm_if(!request, "param request is NULL");
	AddMailReqData *req_data = (AddMailReqData *)email_request_get_data(request);
	retm_if(!req_data, "request data is NULL");

	FREE(req_data);

	debug_leave();
}
