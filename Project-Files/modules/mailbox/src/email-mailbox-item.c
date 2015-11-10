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
#include "email-mailbox-item.h"
#include "email-mailbox-list.h"
#include "email-mailbox-sync.h"
#include "email-mailbox-request.h"
#include "email-mailbox-ug-util.h"
#include "email-mailbox-util.h"

void mailbox_process_move_mail(EmailMailboxUGD *mailbox_ugd)
{
	debug_enter();

	int checked_count = eina_list_count(mailbox_ugd->selected_mail_list);
	int account_id = 0;
	int mail_list_length = 0;
	app_control_h service;

	debug_log("checked_count: [%d]", checked_count);
	if (checked_count <= 0) return;

	if (APP_CONTROL_ERROR_NONE != app_control_create(&service)) {
		debug_log("creating service handle failed");
		return;
	}

	int i = 0;
	int temp_count = 0;
	temp_count = checked_count;
	debug_log("temp_count %d", temp_count);
	char **temp = MEM_ALLOC(temp, temp_count);
	for (i = 0; i < temp_count; i++) {
		temp[i] = MEM_ALLOC(temp[i], MAIL_ID_SIZE);
	}

	for (i = 0; i < checked_count; i++) {
		Eina_List *nth_list = eina_list_nth_list(mailbox_ugd->selected_mail_list, i);
		MailItemData *ld = eina_list_data_get(nth_list);

		if (account_id < 1) {
			account_id = ld->account_id;
			mailbox_ugd->move_src_mailbox_id = ld->mailbox_id;
		}

		/* save the mail-id to be moved */
		debug_log("not Thread...");
		int mail_id = ld->mail_id;
		snprintf(temp[mail_list_length++], MAIL_ID_SIZE, "%d", mail_id);
	}

	debug_log("mail list length %d", mail_list_length);
	char **selected_mail_list = (char **)malloc(mail_list_length * sizeof(char *));
	if (!selected_mail_list) {
		app_control_destroy(service);
		for (i = 0; i < temp_count; i++) {
			FREE(temp[i]);
		}
		FREE(temp);
		debug_error("allocation memory failed");
		return;
	}
	for (i = 0; i < mail_list_length; i++) {
		selected_mail_list[i] = (char *)malloc(MAIL_ID_SIZE * sizeof(char));
		/* In case of failure free all previous elements service & temp & selected_mail_list[j] */
		if (!selected_mail_list[i]) {
			int j = i;
			app_control_destroy(service);
			for (i = 0; i < temp_count; i++) {
				FREE(temp[i]);
			}
			FREE(temp);
			for (--j; j >= 0; --j) {
				free(selected_mail_list[j]);
			}
			free(selected_mail_list);
			debug_error("allocation memory failed");
			return;
		}
		snprintf(selected_mail_list[i], MAIL_ID_SIZE, "%s", temp[i]);
	}

	for (i = 0; i < temp_count; i++) {
		FREE(temp[i]);
	}
	FREE(temp);

	mailbox_sync_cancel_all(mailbox_ugd);

	char acctid[30] = { 0, };
	snprintf(acctid, sizeof(acctid), "%d", account_id);
	char mailboxid[30] = { 0, };
	snprintf(mailboxid, sizeof(mailboxid), "%d", mailbox_ugd->move_src_mailbox_id);
	char move_ug[30] = { 0, };
	snprintf(move_ug, sizeof(move_ug), "%d", 1);
	char move_mode[30] = { 0, };
	snprintf(move_mode, sizeof(move_mode), "%d", EMAIL_MOVE_VIEW_NORMAL);
	debug_log("setting move mode %s", move_mode);
	char mailbox_edit_mode[30] = { 0, };
	snprintf(mailbox_edit_mode, sizeof(mailbox_edit_mode), "%d", mailbox_ugd->b_editmode);
	char move_src_mailbox_id[30] = { 0, };
	snprintf(move_src_mailbox_id, sizeof(move_src_mailbox_id), "%d", mailbox_ugd->move_src_mailbox_id);
	app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_ACCOUNT_ID, acctid);
	app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_MAILBOX, mailboxid);
	app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_IS_MAILBOX_MOVE_UG, move_ug);
	app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_MAILBOX_MOVE_MODE, move_mode);
	app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_IS_MAILBOX_EDIT_MODE, mailbox_edit_mode);
	app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_MOVE_SRC_MAILBOX_ID, move_src_mailbox_id);
	app_control_add_extra_data_array(service, EMAIL_BUNDLE_KEY_ARRAY_SELECTED_MAIL_IDS, (const char **)selected_mail_list, mail_list_length);
	debug_log("account_id: %s, mailbox_id: %s", acctid, mailboxid);

	mailbox_ugd->account = mailbox_account_module_create(mailbox_ugd, EMAIL_MODULE_ACCOUNT, service);
	app_control_destroy(service);

	for (i = 0; i < mail_list_length; i++) {
		free(selected_mail_list[i]);
	}
	free(selected_mail_list);

}

void mailbox_process_delete_mail(void *data, Ecore_Thread *thd)
{
	debug_enter();
	retm_if(!data, "PARAM NULL: data(%p)", data);

	int i = 0;
	int max_account_id = email_engine_get_max_account_id();
	int result = EMAIL_ERROR_UNKNOWN;
	int is_trash = false;
	Eina_List *cur = NULL;
	DeleteRequestedMail *requested_mail = NULL;

	edit_req_t *req = (edit_req_t *)data;
	EmailMailboxUGD *mailbox_ugd = req->mailbox_ugd;
	Eina_List *list = req->requested_mail_list;

	/*gotom_if(max_account_id < 0, CATCH_ERROR, "Invalid max_account_id(%d) was returned.", max_account_id);*/
	GList * mail_list[max_account_id];
	memset(mail_list, 0, max_account_id*sizeof(GList *));

	int checked_count = eina_list_count(list);
	debug_log("checked_count: %d", checked_count);
	if (checked_count <= 0) goto CATCH_ERROR;

	/* if single mail view (not threaded view), */
	/* get the first item and check if item is in trash box. if so, all other items are in trash */
	requested_mail = eina_list_data_get(list);
	is_trash = (requested_mail->mailbox_type == EMAIL_MAILBOX_TYPE_TRASH) ? 1 : 0;

	if (requested_mail->mail_status == EMAIL_MAIL_STATUS_SEND_SCHEDULED) {
		debug_log("Delete scheduled mail");
		is_trash = 1;
	}

	EINA_LIST_FOREACH(list, cur, requested_mail) {
		mail_list[requested_mail->account_id-1] = g_list_prepend(mail_list[requested_mail->account_id-1], GINT_TO_POINTER(requested_mail->mail_id));
	}

	/* move or delete mails rutine.*/
	int acct = 0;
	for (acct = 0; acct < max_account_id; ++acct) {
		if (!mail_list[acct]) continue;
		else {
			/* convert GList to int array */
			int count = g_list_length(mail_list[acct]);
			int mail_ids[count]; memset(mail_ids, 0, sizeof(mail_ids));
			int i = 0;
			GList *cur = g_list_first(mail_list[acct]);
			for (; i < count; i++, cur = g_list_next(cur))
				mail_ids[i] = (int)(ptrdiff_t)g_list_nth_data(cur, 0);

			debug_log("account_id : %d, count : %d", acct+1, count);

			if (is_trash) {
				email_delete_option_t delete_option = EMAIL_DELETE_LOCAL_AND_SERVER;

				if (GET_ACCOUNT_SERVER_TYPE(acct+1) == EMAIL_SERVER_TYPE_POP3) {
					debug_log("EMAIL_SERVER_TYPE_POP3..");
					email_account_t *account_data = NULL;
					if (email_engine_get_account_full_data(acct+1, &account_data)) {
						if (account_data) {
							account_user_data_t *user_data = (account_user_data_t *)account_data->user_data;
							if (user_data != NULL) {
								debug_log("pop3_deleting_option:%d", user_data->pop3_deleting_option);
								if (user_data->pop3_deleting_option == 0) {
									delete_option = EMAIL_DELETE_LOCALLY;
								} else if (user_data->pop3_deleting_option == 1) {
									delete_option = EMAIL_DELETE_LOCAL_AND_SERVER;
								}
							}
							email_free_account(&account_data, 1);
							account_data = NULL;
						}
					}
				}

				int trashbox_id = GET_MAILBOX_ID(acct+1, EMAIL_MAILBOX_TYPE_TRASH);

				result = email_delete_mail(trashbox_id, mail_ids, count, delete_option);
				if (result != EMAIL_ERROR_NONE) {
					debug_warning("email_delete_message mailbox_id(%d) count(%d)- err (%d)",
												mailbox_ugd->mailbox_id, count, result);
				}
			} else {
				/* making dest folder - trash */
				int trashbox_id = GET_MAILBOX_ID(acct+1, EMAIL_MAILBOX_TYPE_TRASH);

				result = email_move_mail_to_mailbox(mail_ids, count, trashbox_id);
				if (result != EMAIL_ERROR_NONE) {
					debug_warning("email_move_mail_to_mailbox num(%d) folder_id(%d) - err (%d)",
									count, trashbox_id, result);
				}
			}
		}
	}

	if (result == EMAIL_ERROR_NONE) {
		mailbox_ugd->need_deleted_noti = true;
	} else {
		mailbox_ugd->need_deleted_noti = false;
		int ret = notification_status_message_post(email_get_email_string("IDS_EMAIL_POP_FAILED_TO_DELETE"));
		if (ret != NOTIFICATION_ERROR_NONE) {
			debug_log("fail to notification_status_message_post() : %d\n", ret);
		}
	}

	for (i = 0; i < max_account_id; ++i) {
		if (mail_list[i] != NULL) {
			g_list_free(mail_list[i]);
			mail_list[i] = NULL;
		}
	}

CATCH_ERROR:
	EINA_LIST_FOREACH(list, cur, requested_mail) {
		FREE(requested_mail);
	}
	eina_list_free(list);
	FREE(req);
	debug_leave();
}

