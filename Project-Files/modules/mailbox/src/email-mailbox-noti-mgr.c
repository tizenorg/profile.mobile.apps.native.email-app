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
#include <gio/gio.h>
#include "email-mailbox.h"
#include "email-mailbox-list.h"
#include "email-mailbox-list-other-items.h"
#include "email-mailbox-search.h"
#include "email-mailbox-sync.h"
#include "email-mailbox-noti-mgr.h"
#include "email-mailbox-request.h"
#include "email-mailbox-title.h"
#include "email-mailbox-module-util.h"
#include "email-mailbox-util.h"
#include "email-mailbox-toolbar.h"

/*
 * Declaration for static functions
 */

static void _gdbus_event_mailbox_receive(GDBusConnection *connection, const gchar *sender_name, const gchar *object_path,
		const gchar *interface_name, const gchar *signal_name, GVariant *parameters, gpointer data);
static void _parse_set_rule_buf(char *inbuf, GList **mail_list);
static void _parse_set_move_buf(char *inbuf, int *from_box_id, int *to_box_id, GList **mail_list);
static void _parse_set_flag_buf(char *inbuf, char **field_name, GList **mail_list);
static Eina_Bool _check_req_handle_list(int handle);
static void _sending_mail_list_add_mail_id(EmailMailboxView *view, int mail_id);
static Eina_Bool _check_sending_mail_list(EmailMailboxView *view, int mail_id);
static Eina_Bool _check_req_account_list(int account);
static void _mailbox_remove_deleted_flag_mail(int mail_id, void *data);

/*
 * Globals
 */

GDBusConnection *_g_mailbox_conn = NULL;
guint _network_id;
guint _storage_id;

static GList *g_req_handle_list = NULL;
static GList *g_req_account_list = NULL;


/*
 * Definition for static functions
 */

static void _gdbus_event_mailbox_receive(GDBusConnection *connection,
											const gchar *sender_name,
											const gchar *object_path,
											const gchar *interface_name,
											const gchar *signal_name,
											GVariant *parameters,
											gpointer data)
{
	debug_enter();

	debug_secure("Object path=%s, interface name=%s, signal name=%s", object_path, interface_name, signal_name);
	EmailMailboxView *view = (EmailMailboxView *)data;

	if (view == NULL) {
		debug_warning("data is NULL");
		return;
	}

	if (!(g_strcmp0(object_path, "/User/Email/StorageChange")) && !(g_strcmp0(signal_name, "email"))) {
		debug_trace("User.Email.StorageChange");

		int subtype = 0;
		int data1 = 0;
		int data2 = 0;
		char *data3 = NULL;
		int data4 = 0;

		g_variant_get(parameters, "(iiisi)", &subtype, &data1, &data2, &data3, &data4);
		debug_log("STORE_ENABLE: subtype: %d, data1: %d, data2: %d, data3: %s, data4: %d", subtype, data1, data2, data3, data4);

		int account_id = 0;
		int account_count = 0;
		int thread_id = 0;
		int mailid = 0;
		int mailbox_id = 0;
		int err = 0;
		email_sort_type_e sort_type = view->sort_type;
		email_mailbox_type_e mailbox_type = EMAIL_MAILBOX_TYPE_NONE;
		email_mailbox_t *mailbox = NULL;
		email_account_t *account = NULL;
		email_account_t *account_list = NULL;
		char *msg_buf = NULL;

		switch (subtype) {

		/* This notification could be called in pause state when an account is added or deleted in setting app */
		case NOTI_ACCOUNT_ADD:
			debug_trace("NOTI_ACCOUNT_ADD");

			if (view->b_restoring) {
				debug_trace("email account is being restored now.");
				break;
			}

			account_id = data1;

			if (email_engine_get_account_list(&account_count, &account_list))
				view->account_count = account_count;
			else
				view->account_count = 0;

			if (account_list)
				email_engine_free_account_list(&account_list, account_count);

			/* destroy all top modules and refresh all emails */
			if (view->account) {
				mailbox_account_module_destroy(view, view->account);
			}
			if (view->composer) {
				mailbox_composer_module_destroy(view, view->composer);
			}

			err = email_get_account(account_id, EMAIL_ACC_GET_OPT_DEFAULT, &account);
			if (err != EMAIL_ERROR_NONE || !account) {
				if (account)
					email_free_account(&account, 1);
				debug_error("email_get_account : account_id(%d) failed", account_id);
				break;
			}

			mailbox_hide_no_contents_view(view);

			view->account_id = account_id;
			view->account_type = account->incoming_server_type;
			view->mode = EMAIL_MAILBOX_MODE_MAILBOX;
			view->mailbox_type = EMAIL_MAILBOX_TYPE_INBOX;

			mailbox_account_color_list_add(view, account_id, account->color_label);

			G_FREE(view->account_name);
			G_FREE(view->mailbox_alias);

			view->account_name = g_strdup(account->user_email_address);

			email_free_account(&account, 1);

			err = email_get_mailbox_by_mailbox_type(view->account_id, EMAIL_MAILBOX_TYPE_INBOX, &mailbox);
			if (err == EMAIL_ERROR_NONE && mailbox) {
				view->mailbox_id = mailbox->mailbox_id;
				email_free_mailbox(&mailbox, 1);
			} else {
				view->mailbox_id = 0;
				debug_warning("email_get_mailbox_by_mailbox_type : account_id(%d) type(INBOX) - err(%d) or mailbox is NULL(%p)", view->account_id, err, mailbox);
			}

			if (view->b_editmode) {
				mailbox_exit_edit_mode(view);
			}

			mailbox_view_title_update_all(view);
			mailbox_update_notifications_status(view);
			mailbox_list_refresh(view, NULL);
			mailbox_load_more_messages_item_remove(view);
			mailbox_no_more_emails_item_remove(view);

			mailbox_req_handle_list_free();

			break;

		case NOTI_ACCOUNT_DELETE:
			debug_log("NOTI_ACCOUNT_DELETE");

			if (view->b_restoring) {
				debug_trace("email account is being restored now.");
				break;
			}

			account_id = data1;

			view->account_count--;
			mailbox_account_color_list_remove(view, account_id);

			/* destroy all top modules and refresh all emails */
			if (view->account) {
				mailbox_account_module_destroy(view, view->account);
			}
			if (view->composer) {
				mailbox_composer_module_destroy(view, view->composer);
			}

			if (view->account_count <= 0) {
				debug_log("no account exists");
				email_set_need_restart_flag(true);
				email_module_make_destroy_request(view->base.module);
				return;
			}

			if (view->account_id == 0 || view->account_id == account_id) {
				if (view->account_count == 1) {
					int err = 0;
					email_mailbox_t *mailbox = NULL;
					int default_account_id = 0;

					if (email_engine_get_default_account(&default_account_id)) {
						view->account_id = default_account_id;

						err = email_get_mailbox_by_mailbox_type(view->account_id, EMAIL_MAILBOX_TYPE_INBOX, &mailbox);
						if (err == EMAIL_ERROR_NONE && mailbox) {
							view->mailbox_id = mailbox->mailbox_id;
						} else {
							view->mailbox_id = 0;
							debug_warning("email_get_mailbox_by_mailbox_type : account_id(%d) type(INBOX) - err(%d) or mailbox is NULL(%p)", view->account_id, err, mailbox);
						}
						if (mailbox)
							email_free_mailbox(&mailbox, 1);

						view->mode = EMAIL_MAILBOX_MODE_MAILBOX;
						view->mailbox_type = EMAIL_MAILBOX_TYPE_INBOX;
					} else {
						debug_log("email_engine_get_default_account failed");
					}
				} else {
					view->account_id = 0;
					view->mode = EMAIL_MAILBOX_MODE_ALL;
					view->mailbox_type = EMAIL_MAILBOX_TYPE_INBOX;
					view->mailbox_id = 0;
					G_FREE(view->account_name);
					G_FREE(view->mailbox_alias);
				}

				if (view->b_editmode)
					mailbox_exit_edit_mode(view);

				if (view->b_searchmode)
					mailbox_finish_search_mode(view);

				mailbox_view_title_update_all(view);
				mailbox_update_notifications_status(view);
				mailbox_list_refresh(view, NULL);
			}

			break;

		case NOTI_ACCOUNT_UPDATE:
			debug_log("NOTI_ACCOUNT_UPDATE");
			account_id = data1;
			email_account_t *account = NULL;

			if (!email_engine_get_account_full_data(account_id, &account)) {
				debug_log("Failed email_engine_get_account_full_data");
				return;
			}
			if (account) {
				mailbox_account_color_list_update(view, account_id, account->color_label);
				email_engine_free_account_list(&account, 1);
			}

			if (account_id == view->account_id)
				mailbox_view_title_update_account_name(view);

			break;

		case NOTI_ACCOUNT_UPDATE_SYNC_STATUS:
			debug_log("NOTI_ACCOUNT_UPDATE_SYNC_STATUS");
			account_id = data1;
			if (data2 == SYNC_STATUS_FINISHED && (view->account_id == 0 || view->account_id == account_id)) {
				mailbox_view_title_update_mail_count(view);
			}
			if (data2 == SYNC_STATUS_HAVE_NEW_MAILS
				&& view->base.state == EV_STATE_ACTIVE
				&& view->mailbox_type == EMAIL_MAILBOX_TYPE_INBOX
				&& view->mode == EMAIL_MAILBOX_MODE_MAILBOX
				&& view->account_id != account_id) {
				debug_log("update notification bar(%d)", account_id);
				email_update_notification_bar(account_id, 0, 0, 0);
			}
			break;

		case NOTI_MAILBOX_ADD:
			debug_log("NOTI_MAILBOX_ADD");
			account_id = data1;
			mailbox_id = data2;
			mailbox_type = data4;

			if (view->account_id == account_id
				&& view->mode == EMAIL_MAILBOX_MODE_MAILBOX
				&& view->mailbox_type == mailbox_type
				&& view->mailbox_id == 0) {
				view->mailbox_id = mailbox_id;
				debug_log("view->mailbox_id(%d) is set", view->mailbox_id);
			}

			break;

		case NOTI_MAILBOX_DELETE:
			debug_log("NOTI_MAILBOX_DELETE");
			account_id = data1;
			mailbox_id = data2;

			if (view->mailbox_id == data2 && view->mode == EMAIL_MAILBOX_MODE_MAILBOX) {
				view->mailbox_id = GET_MAILBOX_ID(account_id, EMAIL_MAILBOX_TYPE_INBOX);
				debug_log("current folder view is destroyed.(mailbox_id : %d -> %d)", data2, view->mailbox_id);
				if (view->b_editmode) {
					mailbox_exit_edit_mode(view);
				} else if (view->b_searchmode) {
					mailbox_finish_search_mode(view);
				}
				mailbox_view_title_update_all(view);
				mailbox_check_sort_type_validation(view);
				mailbox_list_refresh(view, NULL);
			}

			break;
		case NOTI_THREAD_ID_CHANGED_BY_MOVE_OR_DELETE:
		{
			debug_log("NOTI_THREAD_ID_CHANGED_BY_MOVE_OR_DELETE, mail_id = %d, old thread_id = %d, new thread_id = %d", data4, data1, data2);
			break;
		}

		case NOTI_THREAD_ID_CHANGED_BY_ADD:
		{
			debug_log("NOTI_THREAD_ID_CHANGED_BY_ADD, mail_id = %d, old thread_id = %d, new thread_id = %d", data4, data1, data2);
			break;
		}

		case NOTI_MAIL_MOVE:
		{
			debug_log("NOTI_MAIL_MOVE");
			account_id = data1;
			msg_buf = data3;
			int moved_count = 0;
			GList *mail_list = NULL;
			GList *cur = NULL;
			int *idx = NULL;
			int from_box_id = 0, to_box_id = 0;
			int ret = 1;

			_parse_set_move_buf(msg_buf, &from_box_id, &to_box_id, &mail_list);
			moved_count = g_list_length(mail_list)-1;
			if (mail_list) {
				G_LIST_FOREACH(mail_list, cur, idx) {
					FREE(idx);
				}
				g_list_free(mail_list);
				mail_list = NULL;
			}
			debug_log("account_id : %d, from_box_id= %d, to_box_id =%d, size of moved_count : %d", account_id, from_box_id, to_box_id, moved_count);

			if (view->move_status == EMAIL_ERROR_NONE) {
				view->move_status = -1;
				if (view->moved_mailbox_name) {
					if (moved_count == 1) {
						char str[MAX_STR_LEN] = { 0, };
						snprintf(str, sizeof(str), email_get_email_string("IDS_EMAIL_TPOP_1_EMAIL_MOVED_TO_PS"), view->moved_mailbox_name);
						ret = notification_status_message_post(str);
					} else {
						char str[MAX_STR_LEN] = { 0, };
						snprintf(str, sizeof(str), email_get_email_string("IDS_EMAIL_TPOP_P1SD_EMAILS_MOVED_TO_P2SS"), moved_count, view->moved_mailbox_name);
						ret = notification_status_message_post(str);
					}
					G_FREE(view->moved_mailbox_name);
				} else {
					if (moved_count == 1) {
						ret = notification_status_message_post(email_get_email_string("IDS_EMAIL_TPOP_1_EMAIL_MOVED_TO_SELECTED_FOLDER"));
					} else {
						char str[MAX_STR_LEN] = { 0, };
						snprintf(str, sizeof(str), email_get_email_string("IDS_EMAIL_TPOP_PD_EMAILS_MOVED_TO_SELECTED_FOLDER"), moved_count);
						ret = notification_status_message_post(str);
					}
				}
				if (ret != NOTIFICATION_ERROR_NONE) {
					debug_log("notification_status_message_post() failed: %d", ret);
				}
			} else if (view->need_deleted_noti) {
				view->need_deleted_noti = false;
			}

			if (sort_type == EMAIL_SORT_SENDER_ATOZ || sort_type == EMAIL_SORT_SENDER_ZTOA
					|| sort_type == EMAIL_SORT_RCPT_ATOZ || sort_type == EMAIL_SORT_RCPT_ZTOA
					|| sort_type == EMAIL_SORT_SUBJECT_ATOZ || sort_type == EMAIL_SORT_SUBJECT_ZTOA) {
				mailbox_list_refresh(view, NULL);
			} else {
				MoveMailReqData *req = MEM_ALLOC(req, 1);
				retm_if(!req, "req is NULL");
				*req = (MoveMailReqData) {account_id, 0, g_strdup(msg_buf), view};

				if (!email_request_queue_add_request(view->request_queue, EMAIL_REQUEST_TYPE_MOVE_MAIL, req)) {
					debug_error("failed to add request EMAIL_REQUEST_TYPE_MOVE_WORKER");
					free(req);
				}

				mailbox_view_title_update_mail_count(view);
			}

			break;
		}

		case NOTI_MAIL_MOVE_FINISH:
			debug_log("NOTI_MAIL_MOVE_FINISH: account_id( %d )", data1);
			account_id = data1;

			if (_check_req_account_list(account_id)) {
				/* free list */
				debug_log("Delete_all finished when account_id = 0(All inbox case)");
				mailbox_req_account_list_free();
			}
			break;

		case NOTI_MAIL_MOVE_FAIL:
			debug_log("NOTI_MAIL_MOVE_FAIL");
			debug_log("move status is %d, %d", view->move_status, EMAIL_ERROR_NONE);
			int ret = 1;
			if (view->move_status != EMAIL_ERROR_NONE) {
				ret = notification_status_message_post(email_get_email_string("IDS_EMAIL_POP_FAILED_TO_MOVE"));
			} else if (view->need_deleted_noti) {
				ret = notification_status_message_post(email_get_email_string("IDS_EMAIL_POP_FAILED_TO_MOVE"));
				view->need_deleted_noti = false;
			}
			if (ret != NOTIFICATION_ERROR_NONE) {
				debug_log("notification_status_message_post() failed: %d", ret);
			}
			break;

		case NOTI_MAIL_DELETE:
			debug_log("NOTI_MAIL_DELETE");
			account_id = data1;
			msg_buf = data3;

			if (view->need_deleted_noti) {
				/*if(view->mode == EMAIL_MAILBOX_MODE_SCHEDULED_OUTBOX)
					notification_status_message_post(_("IDS_EMAIL_BODY_SCHEDULED_EMAIL_DELETED"));
				else
					notification_status_message_post(dgettext(PACKAGE, "IDS_EMAIL_BUTTON_DELETE_ABB4"));
				*/
				view->need_deleted_noti = false;
			}

			DeleteMailReqData *req = MEM_ALLOC(req, 1);
			retm_if(!req, "req is NULL");
			/* this dynamic var should be freed in NotiCB */
			*req = (DeleteMailReqData) {account_id, 0, g_strdup(msg_buf), view};

			if (!email_request_queue_add_request(view->request_queue, EMAIL_REQUEST_TYPE_DELETE_MAIL, req)) {
				debug_error("failed to add request EMAIL_REQUEST_TYPE_DELETE_WORKER");
				free(req);
			}

			mailbox_view_title_update_mail_count(view);
			break;

		case NOTI_MAIL_DELETE_FAIL:
			debug_log("NOTI_MAIL_DELETE_FAIL");
			if (view->need_deleted_noti) {
				int ret = notification_status_message_post(email_get_email_string("IDS_EMAIL_POP_FAILED_TO_DELETE"));
				if (ret != NOTIFICATION_ERROR_NONE) {
					debug_log("notification_status_message_post() failed: %d", ret);
				}
				view->need_deleted_noti = false;
			}
			break;

		case NOTI_MAIL_ADD: /* Seed mail was received */
			debug_log("NOTI_MAIL_ADD");
			account_id = data1;
			thread_id = data4;
			mailid = data2;
			mailbox_id = atoi(data3);

			debug_log("view->mode(%d), view->mailbox_type(%d), view->mailbox_id(%d)",
					view->mode, view->mailbox_type, view->mailbox_id);

			if ((view->mode == EMAIL_MAILBOX_MODE_ALL && view->mailbox_type == GET_MAILBOX_TYPE(mailbox_id))
					|| (view->mode == EMAIL_MAILBOX_MODE_MAILBOX && view->mailbox_id == mailbox_id)) {
				AddMailReqData *req = MEM_ALLOC(req, 1);
				retm_if(!req, "req is NULL");
				/* this dynamic variable(req) should be freed in func_notify */
				*req = (AddMailReqData) {account_id, thread_id, mailid, mailbox_id, view->mailbox_type, true,
					view->mode, view};

				if (!email_request_queue_add_request(view->request_queue, EMAIL_REQUEST_TYPE_ADD_MAIL, req)) {
					debug_error("failed to add request EMAIL_REQUEST_TYPE_ADD_MAIL_WORKER");
					free(req);
				}
			}
#ifndef _FEATURE_PRIORITY_SENDER_DISABLE_
			else if (view->mode == EMAIL_MAILBOX_MODE_PRIORITY_SENDER) {
				email_mail_list_item_t *mail_info = NULL;
				mail_info = email_engine_get_mail_by_mailid(mailid);
				if (!mail_info) break;

				if (mail_info->tag_id == PRIORITY_SENDER_TAG_ID && mail_info->mailbox_type == EMAIL_MAILBOX_TYPE_INBOX) {
					debug_secure("priority sender email has been added(%s)", mail_info->email_address_sender);
					AddMailReqData *req = MEM_ALLOC(req, 1);
					retm_if(!req, "req is NULL");
					/* this dynamic variable(req) should be freed in func_notify */
					*req = (AddMailReqData) {account_id, thread_id, mailid, mailbox_id, view->mailbox_type, true,
						view->mode, view};

					if (!email_request_queue_add_request(view->request_queue, EMAIL_REQUEST_TYPE_ADD_MAIL, req)) {
						debug_error("failed to add request EMAIL_REQUEST_TYPE_ADD_MAIL_WORKER");
						free(req);
					}
				}
				FREE(mail_info);
			}
#endif
			break;

		case NOTI_MAIL_UPDATE:
			debug_log("NOTI_MAIL_UPDATE");
			account_id = data1;
			mailid = data2;
			int type = data4;

			if (type == UPDATE_PARTIAL_BODY_DOWNLOAD || type == APPEND_BODY) {
				MailItemData *ld = mailbox_list_get_mail_item_data_by_mailid(mailid, view->mail_list);
				if (ld) {
					email_mail_list_item_t *mail_info = NULL;
					mail_info = email_engine_get_mail_by_mailid(mailid);
					if (!mail_info) break;
					ld->is_attachment = mail_info->attachment_count;
					if (ld->mailbox_type == EMAIL_MAILBOX_TYPE_DRAFT && STR_VALID(mail_info->subject)) {
						gchar buf[MAX_STR_LEN] = { 0, };

						FREE(ld->title);
						UTF8_VALIDATE(mail_info->subject, SUBJECT_LEN);
						g_utf8_strncpy(buf, mail_info->subject, EMAIL_MAILBOX_TITLE_DEFAULT_CHAR_COUNT);
						char *_subject = elm_entry_utf8_to_markup(buf);
						int title_len = STR_LEN(_subject);
						ld->title = MEM_ALLOC_STR(title_len + TAG_LEN + 1);
						STR_NCPY(ld->title, _subject, title_len + TAG_LEN);
						FREE(_subject);
					}
					MAILBOX_UPDATE_GENLIST_ITEM(view->gl, ld->item);
					FREE(mail_info);
				}
			} else if (type == UPDATE_EXTRA_FLAG) {
				if (view->mode == EMAIL_MAILBOX_MODE_MAILBOX && view->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX) {
					MailItemData *ld = mailbox_list_get_mail_item_data_by_mailid(mailid, view->mail_list);
					if (ld) {
						email_mail_list_item_t *mail_info = NULL;
						mail_info = email_engine_get_mail_by_mailid(mailid);
						if (!mail_info) break;
						if (mail_info->save_status == EMAIL_MAIL_STATUS_SEND_SCHEDULED) {
							mailbox_list_remove_mail_item(view, ld);
							mailbox_view_title_update_mail_count(view);
						} else {
							ld->mail_status = mail_info->save_status;
							MAILBOX_UPDATE_GENLIST_ITEM(view->gl, ld->item);
						}
						FREE(mail_info);
					}
				}
			} else if (type == UPDATE_MAIL || type == UPDATE_MEETING) {
				MailItemData *ld = mailbox_list_get_mail_item_data_by_mailid(mailid, view->mail_list);
				if (ld) {
					email_mail_list_item_t *mail_info = NULL;
					mail_info = email_engine_get_mail_by_mailid(mailid);
					if (!mail_info) break;
					if ((sort_type == EMAIL_SORT_PRIORITY && ld->priority != mail_info->priority) ||
							((sort_type == EMAIL_SORT_SUBJECT_ATOZ || sort_type == EMAIL_SORT_SUBJECT_ZTOA) &&
									g_strcmp0(ld->title, mail_info->subject)) ||
							(sort_type == EMAIL_SORT_ATTACHMENTS &&
									((ld->is_attachment && mail_info->attachment_count == 0) ||
									(!ld->is_attachment && mail_info->attachment_count > 0))) ||
							(sort_type == EMAIL_SORT_UNREAD && ld->is_seen != mail_info->flags_seen_field) ||
							(sort_type == EMAIL_SORT_IMPORTANT && ld->flag_type != mail_info->flags_flagged_field)) {
						int mailbox_id = ld->mailbox_id;
						mailbox_list_remove_mail_item(view, ld);
						FREE(mail_info);

						AddMailReqData *req = MEM_ALLOC(req, 1);
						retm_if(!req, "req is NULL");
						/* this dynamic variable(req) should be freed in func_notify */
						*req = (AddMailReqData) {account_id, 0, mailid, mailbox_id, view->mailbox_type, false,
							view->mode, view};

						if (!email_request_queue_add_request(view->request_queue, EMAIL_REQUEST_TYPE_ADD_MAIL, req)) {
							debug_error("failed to add request EMAIL_REQUEST_TYPE_ADD_MAIL_WORKER");
							free(req);
						}
					} else {
						ld->is_attachment = mail_info->attachment_count;
						if (view->mailbox_type == EMAIL_MAILBOX_TYPE_DRAFT) {
							email_address_info_list_t *addrs_info_list = NULL;
							email_address_info_t *addrs_info = NULL;

							FREE(ld->recipient);
							email_get_address_info_list(mailid, &addrs_info_list);
							if (addrs_info_list) {
								if (addrs_info_list->to) {
									addrs_info = g_list_nth_data(addrs_info_list->to, 0);
								}

								if (addrs_info) {
									if (addrs_info->display_name) {
										ld->recipient = STRNDUP(addrs_info->display_name, RECIPIENT_LEN - 1);
									} else if (addrs_info->address) {
										ld->recipient = STRNDUP(addrs_info->address, RECIPIENT_LEN - 1);
									} else {
										ld->recipient = STRNDUP(email_get_email_string("IDS_EMAIL_SBODY_NO_RECIPIENTS_M_NOUN"), RECIPIENT_LEN - 1);
									}
								} else {
									ld->recipient = STRNDUP(email_get_email_string("IDS_EMAIL_SBODY_NO_RECIPIENTS_M_NOUN"), RECIPIENT_LEN - 1);
								}
								email_free_address_info_list(&addrs_info_list);
							} else {
								ld->recipient = STRNDUP(email_get_email_string("IDS_EMAIL_SBODY_NO_RECIPIENTS_M_NOUN"), RECIPIENT_LEN - 1);
							}
							debug_secure("ld->recipient : %s", ld->recipient);

							if (STR_VALID(mail_info->subject)) {
								gchar buf[MAX_STR_LEN] = { 0, };

								FREE(ld->title);
								UTF8_VALIDATE(mail_info->subject, SUBJECT_LEN);
								g_utf8_strncpy(buf, mail_info->subject, EMAIL_MAILBOX_TITLE_DEFAULT_CHAR_COUNT);
								char *_subject = elm_entry_utf8_to_markup(buf);
								int title_len = STR_LEN(_subject);
								ld->title = MEM_ALLOC_STR(title_len + TAG_LEN + 1);
								STR_NCPY(ld->title, _subject, title_len + TAG_LEN);
								FREE(_subject);
							}

							ld->priority = mail_info->priority;
						}
						if (ld->is_seen != mail_info->flags_seen_field) {
							debug_log("read status has been changed.(%d, %d)", ld->is_seen, mail_info->flags_seen_field);
							ld->is_seen = mail_info->flags_seen_field;
							MAILBOX_UPDATE_GENLIST_ITEM_READ_STATE(view->gl, ld->item, ld->is_seen);
						}
						ld->flag_type = mail_info->flags_flagged_field;
						ld->reply_flag = mail_info->flags_answered_field;
						ld->forward_flag = mail_info->flags_forwarded_field;
						MAILBOX_UPDATE_GENLIST_ITEM(view->gl, ld->item);
						FREE(mail_info);
					}
				} else {
					if ((view->mode == EMAIL_MAILBOX_MODE_ALL || view->mode == EMAIL_MAILBOX_MODE_MAILBOX)
						&& view->mailbox_type == EMAIL_MAILBOX_TYPE_INBOX) {
						email_mail_list_item_t *mail_info = NULL;
						mail_info = email_engine_get_mail_by_mailid(mailid);
						if (!mail_info) break;

						if (mail_info->save_status == EMAIL_MAIL_STATUS_RECEIVED) {
							AddMailReqData *req = MEM_ALLOC(req, 1);
							retm_if(!req, "req is NULL");
							/* this dynamic variable(req) should be freed in func_notify */
							*req = (AddMailReqData) {account_id, mail_info->thread_id, mailid, mail_info->mailbox_id, mail_info->mailbox_type, true,
												view->mode, view};

							if (!email_request_queue_add_request(view->request_queue, EMAIL_REQUEST_TYPE_ADD_MAIL, req)) {
								debug_error("failed to add request EMAIL_REQUEST_TYPE_ADD_MAIL_WORKER");
								free(req);
							}
						}
						FREE(mail_info);
					}
#ifndef _FEATURE_PRIORITY_SENDER_DISABLE_
					else if (view->mode == EMAIL_MAILBOX_MODE_PRIORITY_SENDER) {
						email_mail_list_item_t *mail_info = NULL;
						mail_info = email_engine_get_mail_by_mailid(mailid);
						if (!mail_info) break;

						if (mail_info->tag_id == PRIORITY_SENDER_TAG_ID && mail_info->mailbox_type == EMAIL_MAILBOX_TYPE_INBOX) {
							debug_secure("priority sender email has been added(%s)", mail_info->email_address_sender);
							AddMailReqData *req = MEM_ALLOC(req, 1);
							retm_if(!req, "req is NULL");
							/* this dynamic variable(req) should be freed in func_notify */
							*req = (AddMailReqData) {account_id, thread_id, mailid, mailbox_id, view->mailbox_type, true,
								view->mode, view};

							if (!email_request_queue_add_request(view->request_queue, EMAIL_REQUEST_TYPE_ADD_MAIL, req)) {
								debug_error("failed to add request EMAIL_REQUEST_TYPE_ADD_MAIL_WORKER");
								free(req);
							}
						}
						FREE(mail_info);
					}
#endif
				}
			}
			break;
		case NOTI_MAIL_FIELD_UPDATE:
			debug_log("NOTI_MAIL_FIELD_UPDATE");
			account_id = data1;
			int update_type = data2;
			int value = data4;
			char *field_name = NULL; /* Use data2 param instead of field_name */
			GList *mail_list = NULL;
			GList *cur = NULL;
			int *idx = NULL;

			_parse_set_flag_buf(data3, &field_name, &mail_list);
			debug_log("update_type : %d, value : %d, size of mail_list : %d", update_type, value, g_list_length(mail_list));

			switch (update_type) {
			case EMAIL_MAIL_ATTRIBUTE_FLAGS_DELETED_FIELD:
				debug_log("DELETED FILED UPDATE");
				if (view->move_status == EMAIL_ERROR_NONE) {
					view->move_status = -1;
					int ret = 1;
					if (view->moved_mailbox_name) {
						char str[MAX_STR_LEN] = { 0, };
						snprintf(str, sizeof(str), email_get_email_string("IDS_EMAIL_TPOP_1_EMAIL_MOVED_TO_PS"), view->moved_mailbox_name);
						ret = notification_status_message_post(str);
						G_FREE(view->moved_mailbox_name);
					} else {
						ret = notification_status_message_post(email_get_email_string("IDS_EMAIL_TPOP_1_EMAIL_MOVED_TO_SELECTED_FOLDER"));
					}
					if (ret != NOTIFICATION_ERROR_NONE) {
						debug_log("notification_status_message_post() failed: %d", ret);
					}
				}
				if (value) {
					G_LIST_FOREACH(mail_list, cur, idx) {
						debug_log("idx: [%d]", *idx);
						_mailbox_remove_deleted_flag_mail(*idx, view);
					}
				}
				mailbox_view_title_update_mail_count(view);
				break;
			case EMAIL_MAIL_ATTRIBUTE_FLAGS_SEEN_FIELD:
				G_LIST_FOREACH(mail_list, cur, idx) {
					debug_log("idx: [%d]", *idx);
					MailItemData *ld = mailbox_list_get_mail_item_data_by_mailid(*idx, view->mail_list);
					if (ld) {
						if (sort_type == EMAIL_SORT_UNREAD) {
							int mailbox_id = ld->mailbox_id;
							mailbox_list_remove_mail_item(view, ld);

							AddMailReqData *req = MEM_ALLOC(req, 1);
							if (!req) {
								debug_log("req is NULL");
								FREE(field_name);
								return;
							}
							/* this dynamic variable(req) should be freed in func_notify */
							*req = (AddMailReqData) {account_id, 0, *idx, mailbox_id, view->mailbox_type, false,
												view->mode, view};

							if (!email_request_queue_add_request(view->request_queue, EMAIL_REQUEST_TYPE_ADD_MAIL, req)) {
								debug_error("failed to add request EMAIL_REQUEST_TYPE_ADD_MAIL_WORKER");
								free(req);
							}
						} else {
							ld->is_seen = value;
							MAILBOX_UPDATE_GENLIST_ITEM_READ_STATE(view->gl, ld->item, ld->is_seen);
						}
					}
				}
				mailbox_view_title_update_mail_count(view);
				break;
			case EMAIL_MAIL_ATTRIBUTE_FLAGS_FLAGGED_FIELD:
				G_LIST_FOREACH(mail_list, cur, idx) {
					debug_log("idx: [%d]", *idx);
					MailItemData *ld = mailbox_list_get_mail_item_data_by_mailid(*idx, view->mail_list);

					if (ld) {
						if ((value == EMAIL_FLAG_NONE || value == EMAIL_FLAG_TASK_STATUS_CLEAR || value == EMAIL_FLAG_TASK_STATUS_COMPLETE)
							&& view->mode == EMAIL_MAILBOX_MODE_ALL && view->mailbox_type == EMAIL_MAILBOX_TYPE_FLAGGED) {
							mailbox_list_remove_mail_item(view, ld);
							mailbox_view_title_update_mail_count(view);
							if (!g_list_length(view->mail_list))
								mailbox_show_no_contents_view(view);
						} else if (sort_type == EMAIL_SORT_IMPORTANT) {
							int mailbox_id = ld->mailbox_id;
							mailbox_list_remove_mail_item(view, ld);
							mailbox_view_title_update_mail_count(view);

							AddMailReqData *req = MEM_ALLOC(req, 1);
							if (!req) {
								debug_log("req is NULL");
								FREE(field_name);
								return;
							}
							/* this dynamic variable(req) should be freed in func_notify */
							*req = (AddMailReqData) {account_id, 0, *idx, mailbox_id, view->mailbox_type, false,
												view->mode, view};

							if (!email_request_queue_add_request(view->request_queue, EMAIL_REQUEST_TYPE_ADD_MAIL, req)) {
								debug_error("failed to add request EMAIL_REQUEST_TYPE_ADD_MAIL_WORKER");
								free(req);
							}

						} else {
							ld->flag_type = value;
							MAILBOX_UPDATE_GENLIST_ITEM_FLAG_STATE(view->gl, ld->item, ld->flag_type);
						}

						if (true == view->b_editmode) {
							mailbox_update_select_info(view);
						}
					} else {
						if ((value == EMAIL_FLAG_FLAGED || value == EMAIL_FLAG_TASK_STATUS_ACTIVE)
							&& view->mode == EMAIL_MAILBOX_MODE_ALL && view->mailbox_type == EMAIL_MAILBOX_TYPE_FLAGGED) {
							AddMailReqData *req = MEM_ALLOC(req, 1);
							if (!req) {
								debug_log("req is NULL");
								FREE(field_name);
								return;
							}
							/* this dynamic variable(req) should be freed in func_notify */
							*req = (AddMailReqData) {account_id, 0, *idx, mailbox_id, view->mailbox_type, false,
												view->mode, view};

							if (!email_request_queue_add_request(view->request_queue, EMAIL_REQUEST_TYPE_ADD_MAIL, req)) {
								debug_error("failed to add request EMAIL_REQUEST_TYPE_ADD_MAIL_WORKER");
								free(req);
							}
						}
					}
				}
				break;
			case EMAIL_MAIL_ATTRIBUTE_FLAGS_ANSWERED_FIELD:
				G_LIST_FOREACH(mail_list, cur, idx) {
					debug_log("idx: [%d]", *idx);
					MailItemData *ld = mailbox_list_get_mail_item_data_by_mailid(*idx, view->mail_list);
					if (ld) {
						ld->reply_flag = value;
						MAILBOX_UPDATE_GENLIST_ITEM(view->gl, ld->item);
					}
				}
				break;
			case EMAIL_MAIL_ATTRIBUTE_FLAGS_FORWARDED_FIELD:
				G_LIST_FOREACH(mail_list, cur, idx) {
					debug_log("idx: [%d]", *idx);
					MailItemData *ld = mailbox_list_get_mail_item_data_by_mailid(*idx, view->mail_list);
					if (ld) {
						ld->forward_flag = value;
						MAILBOX_UPDATE_GENLIST_ITEM(view->gl, ld->item);
					}
				}
				break;
			case EMAIL_MAIL_ATTRIBUTE_SAVE_STATUS:
				G_LIST_FOREACH(mail_list, cur, idx) {

					debug_log("idx: [%d]", *idx);
					MailItemData *ld = mailbox_list_get_mail_item_data_by_mailid(*idx, view->mail_list);
					if (ld) {
						debug_log("view->mailbox_type: [%d], ld->mailbox_type : %d", view->mailbox_type, ld->mailbox_type);
						{
							ld->mail_status = value;
							MAILBOX_UPDATE_GENLIST_ITEM(view->gl, ld->item);
						}
					}
				}
				break;
			}

			if (mail_list) {
				G_LIST_FOREACH(mail_list, cur, idx) {
					FREE(idx);
				}
				g_list_free(mail_list);
				mail_list = NULL;
			}
			FREE(field_name);
			break;

		case NOTI_MAIL_DELETE_FINISH:
			debug_log("NOTI_MAIL_DELETE_FINISH");
			account_id = data1;

			mailbox_view_title_update_mail_count(view);
			break;

		case NOTI_RULE_APPLY:
		{
			debug_log("NOTI_RULE_APPLY");
			account_id = data1;
			int update_type = data2;
			int value = data4;
			GList *mail_list = NULL;
			GList *cur = NULL;
			int *idx = NULL;

			_parse_set_rule_buf(data3, &mail_list);
			debug_log("update_type : %d, value : %d, size of mail_list : %d", update_type, value, g_list_length(mail_list));

			G_LIST_FOREACH(mail_list, cur, idx) {
				debug_log("idx: [%d]", *idx);
				MailItemData *ld = mailbox_list_get_mail_item_data_by_mailid(*idx, view->mail_list);
				if (ld) {
					ld->is_priority_sender_mail = true;
					MAILBOX_UPDATE_GENLIST_ITEM(view->gl, ld->item);
				}
			}

			if (mail_list) {
				G_LIST_FOREACH(mail_list, cur, idx) {
					FREE(idx);
				}
				g_list_free(mail_list);
				mail_list = NULL;
			}
		}
		break;

		case NOTI_RULE_DELETE:
		{
			debug_log("NOTI_RULE_DELETE");
			account_id = data1;
			int update_type = data2;
			int value = data4;
			GList *mail_list = NULL;
			GList *cur = NULL;
			int *idx = NULL;

			_parse_set_rule_buf(data3, &mail_list);
			debug_log("update_type : %d, value : %d, size of mail_list : %d", update_type, value, g_list_length(mail_list));

			G_LIST_FOREACH(mail_list, cur, idx) {
				debug_log("idx: [%d]", *idx);
				MailItemData *ld = mailbox_list_get_mail_item_data_by_mailid(*idx, view->mail_list);
				if (ld) {
					ld->is_priority_sender_mail = false;
					MAILBOX_UPDATE_GENLIST_ITEM(view->gl, ld->item);
				}
			}

			if (mail_list) {
				G_LIST_FOREACH(mail_list, cur, idx) {
					FREE(idx);
				}
				g_list_free(mail_list);
				mail_list = NULL;
			}
		}
		break;

		case NOTI_RULE_UPDATE:
		{
			debug_log("NOTI_RULE_UPDATE");
			account_id = data1;
			int update_type = data2;
			int value = data4;
			GList *mail_list = NULL;
			GList *cur = NULL;
			int *idx = NULL;
			email_rule_t *rule_list = NULL;
			int count, i;

			_parse_set_rule_buf(data3, &mail_list);
			debug_log("update_type : %d, value : %d, size of mail_list : %d", update_type, value, g_list_length(mail_list));

			if (email_get_rule_list(&rule_list, &count) < 0) {
				debug_log("email_get_rule_list failed");
			} else {
				G_LIST_FOREACH(mail_list, cur, idx) {
					debug_log("idx: [%d]", *idx);
					MailItemData *ld = mailbox_list_get_mail_item_data_by_mailid(*idx, view->mail_list);
					if (ld) {
						for (i = 0; i < count; i++) {
							if (rule_list[i].type == EMAIL_PRIORITY_SENDER) {
								/*debug_secure("vip address %s ld->sender %s", rule_list[i].value2, ld->sender);*/

								if (!g_strcmp0(rule_list[i].value2, ld->sender)) {
									debug_secure("[%s] already set", rule_list[i].value2);
									ld->is_priority_sender_mail = true;
									break;
								} else {
									ld->is_priority_sender_mail = false;
								}
							} else {
								ld->is_priority_sender_mail = false;
							}
						}
						MAILBOX_UPDATE_GENLIST_ITEM(view->gl, ld->item);
					}
				}
				email_free_rule(&rule_list, count);
			}

			if (mail_list) {
				G_LIST_FOREACH(mail_list, cur, idx) {
					FREE(idx);
				}
				g_list_free(mail_list);
				mail_list = NULL;
			}
		}
		break;

		default:
			debug_trace("Uninterested notification");
			break;
		}
		g_free(data3);
	} else if (!(g_strcmp0(object_path, "/User/Email/NetworkStatus")) && !(g_strcmp0(signal_name, "email"))) {
		debug_log("User.Email.NetworkStatus");

		int subtype = 0;
		int data1 = 0;
		char *data2 = NULL;
		int data3 = 0;
		int data4 = 0;

		g_variant_get(parameters, "(iisii)", &subtype, &data1, &data2, &data3, &data4);
		debug_secure("subtype: %d, data1: %d, data2: %s, data3: %d, data4: %d", subtype, data1, data2, data3, data4);

		int account_id = 0;
		int mailbox_id = 0;
		int mail_id = 0;

		switch (subtype) {
		case NOTI_DOWNLOAD_START:
			debug_log("NOTI_DOWNLOAD_START");
			break;

		case NOTI_DOWNLOAD_FINISH:
			debug_log("NOTI_DOWNLOAD_FINISH");
			account_id = data1;
			mailbox_id = data2 ? atoi(data2) : 0;
			if (view->mode == EMAIL_MAILBOX_MODE_MAILBOX
				&& view->only_local == FALSE
				&& !view->b_searchmode && !view->b_editmode
				&& (mailbox_id == view->mailbox_id || (account_id == view->account_id && mailbox_id == 0))
				&& g_list_length(view->mail_list)) {
				mailbox_last_updated_time_item_update(view->mailbox_id, view);

			}

			if (_check_req_handle_list(data3) || data1 == 0) {
				if (data1 == 0) {
					/* free list */
					debug_log("Sync finished when account_id = 0(All inbox case)");
					mailbox_req_handle_list_free();
				}

				mailbox_get_more_progress_item_remove(view);
				if (0 < g_list_length(view->mail_list)) {
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
			} else {
				debug_log("Invalid account_id(%d)", data1);
			}
			break;

		case NOTI_DOWNLOAD_FAIL:
			debug_log("NOTI_DOWNLOAD_FAIL");
			if (_check_req_handle_list(data3) || data1 == 0) {
				if (data1 == 0) {
					/* free list */
					debug_log("Sync finished when account_id = 0(All inbox case)");
					mailbox_req_handle_list_free();
				}

				mailbox_get_more_progress_item_remove(view);
				if (0 < g_list_length(view->mail_list)) {
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

				if (data4 != EMAIL_ERROR_NONE && view->composer == NULL
					&& view->setting == NULL
					&& view->viewer == NULL && view->account == NULL) {
					if (subtype == NOTI_DOWNLOAD_FAIL && data4 == EMAIL_ERROR_AUTHENTICATE) {
						email_authentication_method_t auth_method = EMAIL_AUTHENTICATION_METHOD_NO_AUTH;
						email_account_t *account = NULL;
						int ret = email_get_account(data1, EMAIL_ACC_GET_OPT_DEFAULT, &account);
						if (ret == EMAIL_ERROR_NONE && account) {
							auth_method = account->incoming_server_authentication_method;
						}
						if (account)
							email_free_account(&account, 1);

						if (auth_method == EMAIL_AUTHENTICATION_METHOD_XOAUTH2) {
							mailbox_create_error_popup(data4, data1, view);
						} else {
							int mailbox_id = 0;
							if (data2)
								mailbox_id = atoi(data2);

							debug_log("error : %d, account_id = %d, mailbox_id = %d", data4, data1, mailbox_id);
							view->sync_needed_mailbox_id = mailbox_id;
							mailbox_create_password_changed_popup(view, data1);
						}
					} else if (subtype == NOTI_DOWNLOAD_FAIL && data4 == EMAIL_ERROR_LOGIN_FAILURE) {
						int mailbox_id = 0;
						if (data2)
							mailbox_id = atoi(data2);

						debug_log("error : %d, account_id = %d, mailbox_id = %d", data4, data1, mailbox_id);
						view->sync_needed_mailbox_id = mailbox_id;
						mailbox_create_password_changed_popup(view, data1);
					} else {
						mailbox_create_error_popup(data4, data1, view);
					}
				}
			} else {
				debug_log("Invalid account_id(%d)", data1);
			}
			break;

		case NOTI_DOWNLOAD_BODY_FINISH:
			debug_log("NOTI_DOWNLOAD_BODY_FINISH");
			mail_id = data1;

			MailItemData *ld = mailbox_list_get_mail_item_data_by_mailid(mail_id, view->mail_list);
			if (ld) {
				email_mail_list_item_t *mail_info = NULL;
				mail_info = email_engine_get_mail_by_mailid(mail_id);
				if (!mail_info) break;
				ld->is_attachment = mail_info->attachment_count;
				MAILBOX_UPDATE_GENLIST_ITEM(view->gl, ld->item);
				FREE(mail_info);
			}
			break;

		case NOTI_SEND_START:
			debug_log("NOTI_SEND_START");
			mail_id = data3;
			if ((view->mode == EMAIL_MAILBOX_MODE_ALL
					|| view->mode == EMAIL_MAILBOX_MODE_MAILBOX)
					&& view->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX) {
				if (data4 >= 0) {
					_sending_mail_list_add_mail_id(view, mail_id);
					mailbox_send_all_btn_remove(view);
				}
			}
			break;

		case NOTI_SEND_FINISH:
			debug_log("NOTI_SEND_FINISH");
			mail_id = data3;
			if (!view->b_searchmode && !view->b_editmode
					&& view->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX
					&& (view->mode == EMAIL_MAILBOX_MODE_ALL
					|| view->mode == EMAIL_MAILBOX_MODE_MAILBOX)) {
				if (_check_sending_mail_list(view, mail_id)) {
					if (g_list_first(view->mail_list)) {
						mailbox_send_all_btn_add(view);
					}
				}
			}
			break;

		case NOTI_SEND_FAIL:
			account_id = data1;
			debug_log("NOTI_SEND_FAIL: arg_accountid: %d, view->account_id: %d", account_id, view->account_id);
			mail_id = data3;
			if (!view->b_searchmode && !view->b_editmode
					&& view->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX
					&& (view->mode == EMAIL_MAILBOX_MODE_ALL
					|| view->mode == EMAIL_MAILBOX_MODE_MAILBOX)) {
				if (_check_sending_mail_list(view, mail_id)) {
					if (g_list_first(view->mail_list)) {
						mailbox_send_all_btn_add(view);
					}
				}
			}
			break;

		case NOTI_SEND_CANCEL:
			account_id = data1;
			debug_log("NOTI_SEND_CANCEL: arg_accountid: %d, view->account_id: %d", account_id, view->account_id);
			mail_id = data3;
			if (!view->b_searchmode && !view->b_editmode
					&& view->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX
					&& (view->mode == EMAIL_MAILBOX_MODE_ALL
					|| view->mode == EMAIL_MAILBOX_MODE_MAILBOX)) {
				if (_check_sending_mail_list(view, mail_id)) {
					if (g_list_first(view->mail_list)) {
						mailbox_send_all_btn_add(view);
					}
				}
			}
			break;

		case NOTI_SEARCH_ON_SERVER_START:

			break;

		case NOTI_SEARCH_ON_SERVER_FINISH:
			break;

		case NOTI_SEARCH_ON_SERVER_FAIL:
		case NOTI_SEARCH_ON_SERVER_CANCEL:
			break;

		case NOTI_VALIDATE_AND_CREATE_ACCOUNT_FINISH:
			break;

		default:
			debug_log("Uninterested notification");
			break;
		}
		g_free(data2);
	} else {
		debug_warning("We receive dbus message, but we can't do anything");
	}
	debug_leave();
}

static void _parse_set_flag_buf(char *inbuf, char **field_name, GList **mail_list)
{
	debug_enter();

	if (!STR_VALID(inbuf)) {
		return;
	}
	debug_log("inbuf = %s", inbuf);

	/* notification format: <field_name><0x01><<mail_id><,><mail_id>>*/
	gchar **outer_tok;
	char delimiter[2] = {0x01, 0x00};
	outer_tok = g_strsplit_set(inbuf, delimiter, -1);
	if (outer_tok == NULL) {
		debug_log("outer_tok == NULL");
		return;
	}
	if (outer_tok[0] && strlen(outer_tok[0]) > 0)
		*field_name = strdup(outer_tok[0]);
	if (outer_tok[1] && strlen(outer_tok[1]) > 0) {
		gchar **inner_tok;
		int i = 0;
		inner_tok = g_strsplit_set(outer_tok[1], ",", -1);
		if (g_strv_length(inner_tok) == 1) { /* only one mail_id exists without "," */
			debug_log("outer_tok[1] : %s", outer_tok[1]);
			int *mail_id = (int *)calloc(1, sizeof(int));
			IF_NULL_FREE_2GSTRA_AND_RETURN(mail_id, outer_tok, inner_tok);
			*mail_id = atoi(outer_tok[1]);
			*mail_list = g_list_append(*mail_list, mail_id);
		} else {
			for (i = 0; i < g_strv_length(inner_tok); ++i) {
				debug_log("inner_tok[i] : %s", inner_tok[i]);
				if (!strcmp(inner_tok[i], "\"")) /* skip the empty token */
					continue;
				else {
					int *mail_id = (int *)calloc(1, sizeof(int));
					IF_NULL_FREE_2GSTRA_AND_RETURN(mail_id, outer_tok, inner_tok);
					*mail_id = atoi(inner_tok[i]);
					*mail_list = g_list_append(*mail_list, mail_id);
				}
			}
		}
		g_strfreev(inner_tok);
	}
	g_strfreev(outer_tok);

}

static void _parse_set_move_buf(char *inbuf, int *from_box_id, int *to_box_id, GList **mail_list)
{
	debug_enter();

	if (!STR_VALID(inbuf)) {
		return;
	}
	debug_log("inbuf = %s", inbuf);

	/* notification format: <from_box_id><0x01><to_box_id><0x01><<mail_id><,><mail_id>>*/
	gchar **outer_tok;
	char delimiter[2] = {0x01, 0x00};
	outer_tok = g_strsplit_set(inbuf, delimiter, -1);
	if (outer_tok == NULL) {
		debug_log("outer_tok == NULL");
		return;
	}
	if (outer_tok[0] && strlen(outer_tok[0]) > 0)
		*from_box_id = atoi(outer_tok[0]);
	if (outer_tok[1] && strlen(outer_tok[1]) > 0)
		*to_box_id = atoi(outer_tok[1]);
	if (outer_tok[2] && strlen(outer_tok[2]) > 0) {
		gchar **inner_tok;
		int i = 0;
		inner_tok = g_strsplit_set(outer_tok[2], ",", -1);
		if (g_strv_length(inner_tok) == 1) { /* only one mail_id exists without "," */
			/*debug_log("outer_tok[2] : %s", outer_tok[2]);*/
			int *mail_id = (int *)calloc(1, sizeof(int));
			IF_NULL_FREE_2GSTRA_AND_RETURN(mail_id, outer_tok, inner_tok);
			*mail_id = atoi(outer_tok[2]);
			*mail_list = g_list_append(*mail_list, mail_id);
		} else {
			for (i = 0; i < g_strv_length(inner_tok); ++i) {
				/*debug_log("inner_tok[i] : %s", inner_tok[i]);*/
				if (!strcmp(inner_tok[i], "\"")) /* skip the empty token */
					continue;
				else {
					int *mail_id = (int *)calloc(1, sizeof(int));
					IF_NULL_FREE_2GSTRA_AND_RETURN(mail_id, outer_tok, inner_tok);
					*mail_id = atoi(inner_tok[i]);
					*mail_list = g_list_append(*mail_list, mail_id);
				}
			}
		}
		g_strfreev(inner_tok);
	}
	g_strfreev(outer_tok);

}

static void _parse_set_rule_buf(char *inbuf, GList **mail_list)
{
	debug_enter();

	if (!STR_VALID(inbuf)) {
		return;
	}
	debug_log("inbuf = %s", inbuf);

	/* notification format: <<mail_id><,><mail_id>>*/
	gchar **outer_tok;
	char delimiter[2] = {0x01, 0x00};
	outer_tok = g_strsplit_set(inbuf, delimiter, -1);
	if (outer_tok == NULL) {
		debug_log("outer_tok == NULL");
		return;
	}
	if (outer_tok[0] && strlen(outer_tok[0]) > 0) {
		gchar **inner_tok;
		int i = 0;
		inner_tok = g_strsplit_set(outer_tok[0], ",", -1);
		if (g_strv_length(inner_tok) == 1) { /* only one mail_id exists without "," */
			/*debug_log("outer_tok[0] : %s", outer_tok[0]);*/
			int *mail_id = (int *)calloc(1, sizeof(int));
			IF_NULL_FREE_2GSTRA_AND_RETURN(mail_id, outer_tok, inner_tok);
			*mail_id = atoi(outer_tok[0]);
			*mail_list = g_list_append(*mail_list, mail_id);
		} else {
			for (i = 0; i < g_strv_length(inner_tok); ++i) {
				/*debug_log("inner_tok[i] : %s", inner_tok[i]);*/
				if (!strcmp(inner_tok[i], "\"")) /* skip the empty token */
					continue;
				else {
					int *mail_id = (int *)calloc(1, sizeof(int));
					IF_NULL_FREE_2GSTRA_AND_RETURN(mail_id, outer_tok, inner_tok);
					*mail_id = atoi(inner_tok[i]);
					*mail_list = g_list_append(*mail_list, mail_id);
				}
			}
		}
		g_strfreev(inner_tok);
	}
	g_strfreev(outer_tok);

}

static Eina_Bool _check_req_handle_list(int handle)
{
	debug_log("handle(%d), g_req_handle_list(%p)", handle, g_req_handle_list);

	GList *it = NULL;
	int *_handle = NULL;

	int handle_count = g_list_length(g_req_handle_list);
	debug_log("handle_count(%d)", handle_count);

	G_LIST_FOREACH(g_req_handle_list, it, _handle) {
		if (GPOINTER_TO_INT(_handle) == handle) {
			g_req_handle_list = g_list_remove(g_req_handle_list, _handle);
			debug_log("handle(%d) was removed", _handle);
		}
	}

	handle_count = g_list_length(g_req_handle_list);
	debug_log("handle_count(%d)", handle_count);

	if (handle_count)
		return FALSE;
	else
		return TRUE;
}

static void _sending_mail_list_add_mail_id(EmailMailboxView *view, int mail_id)
{
	debug_log("mail_id(%d), g_sending_mail_list(%p)", mail_id, view->g_sending_mail_list);

	GList *cur = g_list_find(view->g_sending_mail_list, GINT_TO_POINTER(mail_id));

	if (!cur) {
		view->g_sending_mail_list = g_list_append(view->g_sending_mail_list, GINT_TO_POINTER(mail_id));
	} else {
		debug_log("already exist.");
	}

	debug_log("mail_count(%d)", g_list_length(view->g_sending_mail_list));
}

static Eina_Bool _check_sending_mail_list(EmailMailboxView *view, int mail_id)
{
	debug_log("mail_id(%d), g_sending_mail_list(%p)", mail_id, view->g_sending_mail_list);

	GList *it = NULL;
	int *_mail_id = NULL;

	int mail_count = g_list_length(view->g_sending_mail_list);
	debug_log("mail_count(%d)", mail_count);

	G_LIST_FOREACH(view->g_sending_mail_list, it, _mail_id) {
		if (GPOINTER_TO_INT(_mail_id) == mail_id) {
			view->g_sending_mail_list = g_list_remove(view->g_sending_mail_list, _mail_id);
			debug_log("mail_id(%d) was removed", _mail_id);
		}
	}

	mail_count = g_list_length(view->g_sending_mail_list);
	debug_log("mail_count(%d)", mail_count);

	if (mail_count)
		return FALSE;
	else
		return TRUE;
}

static Eina_Bool _check_req_account_list(int acount)
{
	debug_log("account(%d), g_req_account_list(%p)", acount, g_req_account_list);

	GList *it = NULL;
	int *_handle = NULL;

	if (NULL == g_req_account_list) {
		debug_log("g_req_account_list is NULL. return FALSE");
		return FALSE;
	}

	int account_count = g_list_length(g_req_account_list);
	debug_log("account_count(%d)", account_count);

	G_LIST_FOREACH(g_req_account_list, it, _handle) {
		if (GPOINTER_TO_INT(_handle) == acount) {
			g_req_account_list = g_list_remove(g_req_account_list, _handle);
			debug_log("handle(%d) was removed", _handle);
		}
	}

	account_count = g_list_length(g_req_account_list);
	debug_log("account_count(%d)", account_count);

	if (account_count)
		return FALSE;
	else
		return TRUE;
}

static void _mailbox_remove_deleted_flag_mail(int mail_id, void *data)
{
	debug_enter();

	EmailMailboxView *view = (EmailMailboxView *)data;

	if (mail_id <= 0) {
		debug_log("Invalid parameter(mail_id)");
		return;
	}

	MailItemData *ld = mailbox_list_get_mail_item_data_by_mailid(mail_id, view->mail_list);
	if (ld) {
		if ((view->mode == EMAIL_MAILBOX_MODE_ALL && view->mailbox_type == ld->mailbox_type)
				|| (view->mode == EMAIL_MAILBOX_MODE_MAILBOX && ld->mailbox_id == view->mailbox_id)) {
			debug_log("Remove mail from list mail_id : %d", ld->mail_id);

			if (view->b_editmode) {
				debug_log("edit list update first");
				mailbox_update_edit_list_view(ld, view);
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
		}
	}
}


/*
 * Definition for exported functions
 */

void mailbox_setup_dbus_receiver(EmailMailboxView *view)
{
	debug_enter();

	GError *error = NULL;
	if (_g_mailbox_conn == NULL) {
		_g_mailbox_conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
		if (error) {
			debug_error("g_bus_get_sync() failed (%s)", error->message);
			g_error_free(error);
			return;
		}

		_storage_id = g_dbus_connection_signal_subscribe(_g_mailbox_conn, NULL, "User.Email.StorageChange", "email", "/User/Email/StorageChange",
													NULL, G_DBUS_SIGNAL_FLAGS_NONE, _gdbus_event_mailbox_receive, (void *)view, NULL);

		if (_storage_id == GDBUS_SIGNAL_SUBSCRIBE_FAILURE) {
			debug_log("Failed to g_dbus_connection_signal_subscribe()");
			return;
		}
		_network_id = g_dbus_connection_signal_subscribe(_g_mailbox_conn, NULL, "User.Email.NetworkStatus", "email", "/User/Email/NetworkStatus",
													NULL, G_DBUS_SIGNAL_FLAGS_NONE, _gdbus_event_mailbox_receive, (void *)view, NULL);
		if (_network_id == GDBUS_SIGNAL_SUBSCRIBE_FAILURE) {
			debug_critical("Failed to g_dbus_connection_signal_subscribe()");
			return;
		}
	}
}

void mailbox_remove_dbus_receiver()
{
	debug_enter();

	g_dbus_connection_signal_unsubscribe(_g_mailbox_conn, _storage_id);
	g_dbus_connection_signal_unsubscribe(_g_mailbox_conn, _network_id);
	g_object_unref(_g_mailbox_conn);
	_g_mailbox_conn = NULL;
	_storage_id = 0;
	_network_id = 0;
}

void mailbox_req_handle_list_add_handle(int handle)
{
	debug_log("handle(%d)", handle);
	debug_log("g_req_handle_list(%p)", g_req_handle_list);

	g_req_handle_list = g_list_append(g_req_handle_list, GINT_TO_POINTER(handle));
}

void mailbox_req_handle_list_free()
{
	debug_log("g_req_handle_list(%p)", g_req_handle_list);

	if (g_req_handle_list) {
		g_list_free(g_req_handle_list);
		g_req_handle_list = NULL;
	}
}

void mailbox_req_account_list_free()
{
	debug_log("g_req_account_list(%p)", g_req_account_list);

	if (g_req_account_list) {
		g_list_free(g_req_account_list);
		g_req_account_list = NULL;
	}
}

void mailbox_sending_mail_list_free(EmailMailboxView *view)
{
	debug_log("g_sending_mail_list(%p)", view->g_sending_mail_list);

	if (view->g_sending_mail_list) {
		g_list_free(view->g_sending_mail_list);
		view->g_sending_mail_list = NULL;
	}
}
