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
#include "email-mailbox-noti-mgr.h"
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


/*
 * Declaration for static functions
 */

static bool _mailbox_sync_single_mailbox(EmailMailboxView *view);
static bool _mailbox_sync_combined_mailbox(EmailMailboxView *view);

/*
 * Definition for static functions
 */

static bool _mailbox_sync_single_mailbox(EmailMailboxView *view)
{
	debug_enter();

	int handle = 0;
	int account_id = view->account_id;
	int mailbox_id = view->mailbox_id;
	email_account_t *account = NULL;
	char buf[MAX_STR_LEN] = {0};

	if (email_engine_get_account_data(account_id, (EMAIL_ACC_GET_OPT_DEFAULT | EMAIL_ACC_GET_OPT_OPTIONS), &account)) {
		if (account->sync_disabled) {
			debug_log("sync of this account(%d) was disabled.", account_id);
			snprintf(buf, sizeof(buf), email_get_email_string("IDS_EMAIL_TPOP_SYNC_EMAIL_DISABLED_FOR_PS"), account->user_email_address);
			int ret = notification_status_message_post(buf);
			if (ret != NOTIFICATION_ERROR_NONE) {
				debug_log("notification_status_message_post() failed: %d", ret);
			}
			email_engine_free_account_list(&account, 1);
			return false;
		}
		email_engine_free_account_list(&account, 1);
	} else {
		debug_warning("email_engine_get_account_data failed");
	}

	if (view->account_type == EMAIL_SERVER_TYPE_IMAP4) {
		debug_warning("syncing IMAP4 mailbox");
		email_engine_sync_imap_mailbox_list(account_id, &handle);
	}

	if (!email_engine_sync_folder(account_id, mailbox_id, &handle)) {
		debug_warning("email_engine_sync_folder(%d, %d)s failed.", account_id, mailbox_id);
		return false;
	}
	debug_log("handle: %d", handle);

	mailbox_req_handle_list_add_handle(handle);

	return true;
}

static bool _mailbox_sync_combined_mailbox(EmailMailboxView *view)
{
	debug_enter();

	email_account_t *account_list = NULL;
	int account_cnt = 0, i = 0;
	char buf[MAX_STR_LEN] = {0};
	bool ret = false;
	int mailbox_type = EMAIL_MAILBOX_TYPE_NONE;

	if (view->mailbox_type == EMAIL_MAILBOX_TYPE_NONE || view->mailbox_type == EMAIL_MAILBOX_TYPE_FLAGGED)
		mailbox_type = EMAIL_MAILBOX_TYPE_INBOX;
	else
		mailbox_type = view->mailbox_type;

	if (!email_engine_get_account_list(&account_cnt, &account_list)) {
		debug_warning("email_engine_get_account_list fail");
		return false;
	}

	if (account_cnt > 0) {
		int handle = 0;
		int err = 0;

		for (i = 0; i < account_cnt; ++i) {
			if (account_list[i].sync_disabled == true) {
				debug_log("sync of this account(%d) was disabled.", account_list[i].account_id);
				snprintf(buf, sizeof(buf), email_get_email_string("IDS_EMAIL_TPOP_SYNC_EMAIL_DISABLED_FOR_PS"), account_list[i].user_email_address);
				int ret = notification_status_message_post(buf);
				if (ret != NOTIFICATION_ERROR_NONE) {
					debug_log("notification_status_message_post() failed: %d", ret);
				}
			} else {
				if (account_list[i].incoming_server_type == EMAIL_SERVER_TYPE_IMAP4) {
					debug_warning("need to sync IMAP folders");
					email_engine_sync_imap_mailbox_list(account_list[i].account_id, &handle);
				}

				err = email_engine_sync_folder(account_list[i].account_id, GET_MAILBOX_ID(account_list[i].account_id, mailbox_type), &handle);
				if (err == false) {
					debug_warning("fail to sync header - account_id(%d)", account_list[i].account_id);
					email_engine_free_account_list(&account_list, account_cnt);
					return false;
				} else {
					debug_log("account_id(%d), handle(%d)", account_list[i].account_id, handle);
					mailbox_req_handle_list_add_handle(handle);
					ret = true;
				}
			}
		}
	}

	email_engine_free_account_list(&account_list, account_cnt);

	return ret;
}

/*
 * Definition for exported functions
 */

bool mailbox_sync_current_mailbox(EmailMailboxView *view)
{
	debug_log("view->mode(%d)", view->mode);

	if (view->account_count == 0) {
		mailbox_create_timeout_popup(view, _("IDS_ST_HEADER_WARNING"),
				_("IDS_EMAIL_POP_YOU_HAVE_NOT_YET_CREATED_AN_EMAIL_ACCOUNT_CREATE_AN_EMAIL_ACCOUNT_AND_TRY_AGAIN"), 3.0);
		return false;
	}

	if (view->only_local == true) {
		debug_log("this is local folder");
		return false;
	}

	if ((view->mode == EMAIL_MAILBOX_MODE_ALL)
#ifndef _FEATURE_PRIORITY_SENDER_DISABLE_
			|| view->mode == EMAIL_MAILBOX_MODE_PRIORITY_SENDER
#endif
			) {
		mailbox_sync_cancel_all(view);

		return _mailbox_sync_combined_mailbox(view);
	} else if (view->mode == EMAIL_MAILBOX_MODE_MAILBOX) {
		email_account_server_t account_type = view->account_type;
		if (account_type == EMAIL_SERVER_TYPE_POP3 || account_type == EMAIL_SERVER_TYPE_IMAP4) {
			mailbox_sync_cancel_all(view);
		}
		mailbox_req_handle_list_free();

		return _mailbox_sync_single_mailbox(view);
	} else {
		debug_log("not need to sync for view->mode(%d), type(%d)", view->mode, view->mailbox_type);
		return false;
	}

	return false;
}

bool mailbox_sync_more_messages(EmailMailboxView *view)
{
	debug_enter();

	int new_slot_size = 0;
	bool ret = false;
	int handle = 0;

	if (view->mode == EMAIL_MAILBOX_MODE_ALL) {
		/* loop: All account */
		email_account_t *account_list = NULL;
		int account_count = 0;
		int i = 0;
		if (!email_engine_get_account_list(&account_count, &account_list)) {
			debug_log("email_engine_get_account_list() failed");
			return false;
		}

		for (i = 0; i < account_count; ++i) {
			if (account_list[i].sync_disabled == false
				&& (account_list[i].incoming_server_type == EMAIL_SERVER_TYPE_POP3 || account_list[i].incoming_server_type == EMAIL_SERVER_TYPE_IMAP4)) {
				int mailbox_id = 0;
				email_mailbox_t *mailbox = NULL;
				if (email_engine_get_mailbox_by_mailbox_type(account_list[i].account_id, view->mailbox_type, &mailbox)) {
					new_slot_size = mailbox->mail_slot_size + 25;
					mailbox_id = mailbox->mailbox_id;
					email_engine_free_mailbox_list(&mailbox, 1);
				} else {
					debug_error("mailbox is NULL");
					email_engine_free_account_list(&account_list, account_count);
					return false;
				}

				debug_log("account_id: [%d], mailbox_id: [%d], slot_size: [%d]", account_list[i].account_id, mailbox_id, new_slot_size);
				if (!email_engine_set_mail_slot_size(account_list[i].account_id, mailbox_id, new_slot_size)) {
					debug_error("email_engine_set_mail_slot_size() failed");
					email_engine_free_account_list(&account_list, account_count);
					return false;
				}

				ret = email_engine_sync_folder(account_list[i].account_id, mailbox_id, &handle);
				if (ret == false) {
					debug_warning("fail to sync header - account_id(%d)", account_list[i].account_id);
					email_engine_free_account_list(&account_list, account_count);
					return false;
				} else {
					mailbox_req_handle_list_add_handle(handle);
				}
			} else {
				debug_log("skip this account. account_id(%d), sync_disabled(%d), incoming_server_type(%d)",
					account_list[i].account_id, account_list[i].sync_disabled, account_list[i].incoming_server_type);
			}
		}

		email_engine_free_account_list(&account_list, account_count);
	} else if (view->mode == EMAIL_MAILBOX_MODE_MAILBOX) {
		/* mailbox mode */
		email_account_t *account = NULL;
		if (!email_engine_get_account_data(view->account_id, EMAIL_ACC_GET_OPT_OPTIONS, &account)) {
			debug_log("email_engine_get_account_data() failed");
			return false;
		}
		bool sync_disabled = account->sync_disabled;
		email_engine_free_account_list(&account, 1);
		if (sync_disabled) {
			debug_log("sync of this account(%d) was disabled.", view->account_id);
			return false;
		}

		email_mailbox_t *mailbox = NULL;
		retvm_if(!email_engine_get_mailbox_by_mailbox_id(view->mailbox_id, &mailbox),
				false, "email_engine_get_mailbox_by_mailbox_id() failed");
		new_slot_size = mailbox->mail_slot_size + 25;
		email_engine_free_mailbox_list(&mailbox, 1);

		retvm_if(!email_engine_set_mail_slot_size(view->account_id, view->mailbox_id, new_slot_size),
				false, "email_engine_set_mail_slot_size() failed");
		debug_log("account_id: [%d], mailbox_id: [%d], slot_size: [%d]", view->account_id, view->mailbox_id, new_slot_size);

		ret = email_engine_sync_folder(view->account_id, view->mailbox_id, &handle);
		retvm_if(ret == false, false, "email_engine_sync_folder() failed");

		mailbox_req_handle_list_add_handle(handle);
	} else {
		debug_warning("invalid view->mode(%d)", view->mode);
	}

	return ret;
}

void mailbox_sync_cancel_all(EmailMailboxView *view)
{
	debug_enter();

	email_task_information_t *cur_task_info = NULL;
	int task_info_cnt = 0;
	int i = 0;
	gint acct_id = 0;

	gboolean b_default_account_exist = email_engine_get_default_account(&acct_id);
	if (b_default_account_exist) {
		email_engine_get_task_information(&cur_task_info, &task_info_cnt);
		if (cur_task_info) {
			for (i = 0; i < task_info_cnt; i++) {
				debug_log("account_id(%d), handle(%d), type(%d)", cur_task_info[i].account_id, cur_task_info[i].handle, cur_task_info[i].type);
				if (cur_task_info[i].type != EMAIL_EVENT_MOVE_MAIL
					&& cur_task_info[i].type != EMAIL_EVENT_DELETE_MAIL
					&& cur_task_info[i].type != EMAIL_EVENT_DELETE_MAIL_ALL
					&& cur_task_info[i].type != EMAIL_EVENT_SEND_MAIL
					&& cur_task_info[i].type != EMAIL_EVENT_SYNC_FLAGS_FIELD_TO_SERVER
					&& cur_task_info[i].type != EMAIL_EVENT_SYNC_MAIL_FLAG_TO_SERVER) {
					email_engine_cancel_job(cur_task_info[i].account_id, cur_task_info[i].handle, EMAIL_CANCELED_BY_USER);
				}
			}

			FREE(cur_task_info);
		}
	}
	mailbox_req_handle_list_free();
}

void mailbox_sync_folder_with_new_password(int account_id, int mailbox_id, void *data)
{
	debug_enter();

	EmailMailboxView *view = (EmailMailboxView *)data;
	int handle = 0;
	gboolean res = FALSE;

	mailbox_sync_cancel_all(view);

	res = email_engine_sync_folder(account_id, mailbox_id, &handle);
	debug_log("handle: %d, res: %d", handle, res);

	mailbox_req_handle_list_add_handle(handle);
}
