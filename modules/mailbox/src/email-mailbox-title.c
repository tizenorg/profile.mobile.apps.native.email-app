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

#include <system_settings.h>
#include "email-mailbox.h"
#include "email-mailbox-list.h"
#include "email-mailbox-search.h"
#include "email-mailbox-title.h"
#include "email-mailbox-toolbar.h"
#include "email-mailbox-util.h"

#define TITLE_TEXT_FORMAT "<align=left>%s</align>"
#define TITLE_TEXT_WITH_COUNT_FORMAT "<align=left>%s (%d)</align>"



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
 * Definition for static functions
 */


/*
 * Declaration for static functions
 */
static Eina_Bool _get_total_and_unread_count_from_curr_mailbox(EmailMailboxView *view, int *total_count, int *unread_count);

/*
 * Definition for exported functions
 */
void mailbox_view_title_unpack(EmailMailboxView *view)
{
	debug_enter();
	retm_if(!view, "view is NULL");

	Evas_Object *obj = elm_object_part_content_unset(view->content_layout, "top_bar");
	evas_object_hide(obj);
	debug_leave();
}

void mailbox_view_title_pack(EmailMailboxView *view)
{
	debug_enter();
	retm_if(!view, "view is NULL");

	evas_object_show(view->title_layout);
	elm_object_part_content_set(view->content_layout, "top_bar", view->title_layout);
	debug_leave();
}

void mailbox_view_title_update_all(EmailMailboxView *view)
{
	debug_log("mode: [%d], account_id: [%d], mailbox_id: [%d], mailbox_type: [%d], b_searchmode[%d], b_editmode[%d]",
		view->mode, view->account_id, view->mailbox_id, view->mailbox_type, view->b_searchmode, view->b_editmode);

	int total_count = 0;
	int unread_count = 0;

	/* initialize account name and mailbox name */
	G_FREE(view->account_name);
	G_FREE(view->mailbox_alias);
	view->total_mail_count = 0;
	view->unread_mail_count = 0;

	if (view->mode == EMAIL_MAILBOX_MODE_ALL) {
		if (view->mailbox_type != EMAIL_MAILBOX_TYPE_FLAGGED) {
			int mailbox_type = (view->mailbox_type == EMAIL_MAILBOX_TYPE_NONE) ? EMAIL_MAILBOX_TYPE_INBOX : view->mailbox_type;
			if (!email_get_combined_mail_count_by_mailbox_type(mailbox_type, &total_count, &unread_count))
				debug_warning("email_get_combined_mail_count_by_mailbox_type() failed");
		}

		switch (view->mailbox_type) {
		case EMAIL_MAILBOX_TYPE_INBOX:
			view->mailbox_alias = g_strdup(_("IDS_EMAIL_HEADER_COMBINED_INBOX_ABB"));
			view->unread_mail_count = unread_count;
			view->only_local = false;
		break;
		case EMAIL_MAILBOX_TYPE_SENTBOX:
			view->mailbox_alias = g_strdup(_("IDS_EMAIL_HEADER_SENT_M_EMAIL"));
			view->unread_mail_count = unread_count;
			view->only_local = false;
		break;
		case EMAIL_MAILBOX_TYPE_TRASH:
			view->mailbox_alias = g_strdup(_("IDS_EMAIL_HEADER_RECYCLE_BIN_ABB"));
			view->unread_mail_count = unread_count;
			view->only_local = false;
		break;
		case EMAIL_MAILBOX_TYPE_DRAFT:
			view->mailbox_alias = g_strdup(_("IDS_EMAIL_HEADER_DRAFTS"));
			view->total_mail_count = total_count;
			view->only_local = false;
		break;
		case EMAIL_MAILBOX_TYPE_SPAMBOX:
			view->mailbox_alias = g_strdup(_("IDS_EMAIL_HEADER_SPAMBOX"));
			view->unread_mail_count = unread_count;
			view->only_local = false;
		break;
		case EMAIL_MAILBOX_TYPE_OUTBOX:
			view->mailbox_alias = g_strdup(_("IDS_EMAIL_HEADER_OUTBOX"));
			view->total_mail_count = total_count;
			view->only_local = true;
		break;
		case EMAIL_MAILBOX_TYPE_FLAGGED:
			view->mailbox_alias = g_strdup(_("IDS_EMAIL_HEADER_STARRED_EMAILS_ABB"));
			view->unread_mail_count = email_get_favourite_mail_count(true);
			view->only_local = false;
		break;

		default:
			view->mailbox_alias = g_strdup(_("IDS_EMAIL_HEADER_INBOX"));
			view->unread_mail_count = unread_count;
			view->only_local = false;
		break;
		}
		view->account_type = EMAIL_SERVER_TYPE_NONE;
	}
#ifndef _FEATURE_PRIORITY_SENDER_DISABLE_
	else if (view->mode == EMAIL_MAILBOX_MODE_PRIORITY_SENDER) {
		view->mailbox_type = EMAIL_MAILBOX_TYPE_INBOX;
		view->mailbox_alias = g_strdup(_("IDS_EMAIL_HEADER_PRIORITY_SENDERS_ABB"));
		view->unread_mail_count = email_get_priority_sender_mail_count(true);
		view->account_type = EMAIL_SERVER_TYPE_NONE;
		view->only_local = false;
	}
#endif
	else if (view->mode == EMAIL_MAILBOX_MODE_MAILBOX) {
		email_account_t *account = NULL;
		if (email_engine_get_account_data(view->account_id, GET_FULL_DATA_WITHOUT_PASSWORD, &account)) {
			view->account_name = g_strdup(account->user_email_address);
			view->account_type = account->incoming_server_type;
			email_engine_free_account_list(&account, 1);
		}

		if (view->mailbox_id > 0) {
			email_mailbox_t *mailbox = NULL;
			if (email_engine_get_mailbox_by_mailbox_id(view->mailbox_id, &mailbox)) {
				view->mailbox_type = mailbox->mailbox_type;

				switch (view->mailbox_type) {
				case EMAIL_MAILBOX_TYPE_INBOX:
					view->mailbox_alias = g_strdup(_("IDS_EMAIL_HEADER_INBOX"));
					view->unread_mail_count = mailbox->unread_count - email_get_deleted_flag_mail_count(view->account_id);
					break;
				case EMAIL_MAILBOX_TYPE_SENTBOX:
					view->mailbox_alias = g_strdup(_("IDS_EMAIL_HEADER_SENT_M_EMAIL"));
					view->unread_mail_count = mailbox->unread_count;
					break;
				case EMAIL_MAILBOX_TYPE_TRASH:
					view->mailbox_alias = g_strdup(_("IDS_EMAIL_HEADER_RECYCLE_BIN_ABB"));
					view->unread_mail_count = mailbox->unread_count;
					break;
				case EMAIL_MAILBOX_TYPE_DRAFT:
					view->mailbox_alias = g_strdup(_("IDS_EMAIL_HEADER_DRAFTS"));
					view->total_mail_count = mailbox->total_mail_count_on_local;
					break;
				case EMAIL_MAILBOX_TYPE_SPAMBOX:
					view->mailbox_alias = g_strdup(_("IDS_EMAIL_HEADER_SPAMBOX"));
					view->unread_mail_count = mailbox->unread_count;
					break;
				case EMAIL_MAILBOX_TYPE_OUTBOX:
					view->mailbox_alias = g_strdup(_("IDS_EMAIL_HEADER_OUTBOX"));
					view->total_mail_count = mailbox->total_mail_count_on_local - email_get_scheduled_outbox_mail_count_by_account_id(mailbox->account_id, false);
					break;
				default:
					view->mailbox_alias = g_strdup(mailbox->alias);
					view->unread_mail_count = mailbox->unread_count;
					break;
				}
				view->only_local = mailbox->local;

				mailbox_set_last_updated_time(mailbox->last_sync_time, view);
				email_engine_free_mailbox_list(&mailbox, 1);
			}
		} else if (view->mailbox_id == 0) {
			/* This is for the first entrance during creating an account. */
			view->mailbox_type = EMAIL_MAILBOX_TYPE_INBOX;
			view->mailbox_alias = g_strdup(_("IDS_EMAIL_HEADER_INBOX"));
			view->unread_mail_count = 0;
			view->only_local = false;
		} else {
			debug_warning("INVALID view->mailbox_id(%d)", view->mailbox_id);
			return;
		}
	} else {
		debug_warning("INVALID view->mode(%d)", view->mode);
		return;
	}

	debug_log("mailbox_id: [%d], mailbox_type: [%d]", view->mailbox_id, view->mailbox_type);
	mailbox_view_title_update_all_without_mailbox_change(view);

}

void mailbox_view_title_update_all_without_mailbox_change(EmailMailboxView *view)
{
	debug_log("mode: [%d], account_id: [%d], mailbox_id: [%d], mailbox_type: [%d], b_searchmode[%d], b_editmode[%d]",
		view->mode, view->account_id, view->mailbox_id, view->mailbox_type, view->b_searchmode, view->b_editmode);

	char mailbox_name_buf[MAX_STR_LEN] = { 0, };
	char mailbox_alias_buf[MAX_STR_LEN] = { 0, };
	char *markup_title = NULL;
	char *markup_subtitle = NULL;

	char *lang = NULL;
	system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_LANGUAGE, &lang);
	if (view->mode == EMAIL_MAILBOX_MODE_MAILBOX) {
		/* two line title */
		if (view->mailbox_type == EMAIL_MAILBOX_TYPE_DRAFT || view->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX) {
			if (view->total_mail_count == 0)
				snprintf(mailbox_name_buf, sizeof(mailbox_name_buf), TITLE_TEXT_FORMAT, view->mailbox_alias);
			else
				snprintf(mailbox_name_buf, sizeof(mailbox_name_buf), TITLE_TEXT_WITH_COUNT_FORMAT, view->mailbox_alias, view->total_mail_count);
		} else {
			if (view->unread_mail_count == 0)
				snprintf(mailbox_name_buf, sizeof(mailbox_name_buf), TITLE_TEXT_FORMAT, view->mailbox_alias);
			else
				snprintf(mailbox_name_buf, sizeof(mailbox_name_buf), TITLE_TEXT_WITH_COUNT_FORMAT, view->mailbox_alias, view->unread_mail_count);
		}

		markup_subtitle = elm_entry_utf8_to_markup(view->account_name);
		snprintf(mailbox_alias_buf, sizeof(mailbox_alias_buf), TITLE_TEXT_FORMAT, markup_subtitle);

		elm_layout_signal_emit(view->title_layout, "two_lines_mode", "mailbox");
		elm_object_part_text_set(view->title_layout, "title.text", mailbox_name_buf);
		elm_object_part_text_set(view->title_layout, "title.text.sub", mailbox_alias_buf);


	} else {
		/*
		 * There's possibility that the flag status of all selected starred mails are changed on edit mode.
		 * So, If selected box is starred, update mail count when exit edit mode.
		 */
		/* one line title */
		if (view->mode == EMAIL_MAILBOX_MODE_ALL && view->mailbox_type == EMAIL_MAILBOX_TYPE_FLAGGED) {
			view->unread_mail_count = email_get_favourite_mail_count(true);
			if (view->unread_mail_count == 0)
				snprintf(mailbox_name_buf, sizeof(mailbox_name_buf), TITLE_TEXT_FORMAT, view->mailbox_alias);
			else
				snprintf(mailbox_name_buf, sizeof(mailbox_name_buf), TITLE_TEXT_WITH_COUNT_FORMAT, view->mailbox_alias, view->unread_mail_count);
		} else if (view->mailbox_type == EMAIL_MAILBOX_TYPE_DRAFT || view->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX) {
			if (view->total_mail_count == 0)
				snprintf(mailbox_name_buf, sizeof(mailbox_name_buf), TITLE_TEXT_FORMAT, view->mailbox_alias);
			else
				snprintf(mailbox_name_buf, sizeof(mailbox_name_buf), TITLE_TEXT_WITH_COUNT_FORMAT, view->mailbox_alias, view->total_mail_count);
		} else {
			if (view->unread_mail_count == 0)
				snprintf(mailbox_name_buf, sizeof(mailbox_name_buf), TITLE_TEXT_FORMAT, view->mailbox_alias);
			else
				snprintf(mailbox_name_buf, sizeof(mailbox_name_buf), TITLE_TEXT_WITH_COUNT_FORMAT, view->mailbox_alias, view->unread_mail_count);
		}

		elm_layout_signal_emit(view->title_layout, "single_line_mode", "mailbox");
		elm_object_part_text_set(view->title_layout, "title.text.sub", NULL);
		elm_object_part_text_set(view->title_layout, "title.text", mailbox_name_buf);
	}
	FREE(lang);
	FREE(markup_title);
	FREE(markup_subtitle);
	mailbox_toolbar_create(view);
	mailbox_naviframe_mailbox_button_add(view);
}

void mailbox_view_title_update_account_name(EmailMailboxView *view)
{
	debug_log("mode: [%d], account_id: [%d], b_searchmode[%d], b_editmode[%d]",
		view->mode, view->account_id, view->b_searchmode, view->b_editmode);

	email_account_t *account = NULL;
	char *markup_subtitle = NULL;
	char mailbox_alias_buf[MAX_STR_LEN] = { 0, };

	if (email_engine_get_account_data(view->account_id, EMAIL_ACC_GET_OPT_DEFAULT, &account)) {
		G_FREE(view->account_name);
		view->account_name = g_strdup(account->user_email_address);
		email_engine_free_account_list(&account, 1);
	}

	if (view->b_editmode)
		return;

	if (view->mode == EMAIL_MAILBOX_MODE_MAILBOX) {
		/* two line title */
		markup_subtitle = elm_entry_utf8_to_markup(view->account_name);
		snprintf(mailbox_alias_buf, sizeof(mailbox_alias_buf), TITLE_TEXT_FORMAT, markup_subtitle);
		elm_layout_signal_emit(view->title_layout, "two_lines_mode", "mailbox");
		elm_object_part_text_set(view->title_layout, "title.text.sub", mailbox_alias_buf);
	}

	FREE(markup_subtitle);
}

static Eina_Bool _get_total_and_unread_count_from_curr_mailbox(EmailMailboxView *view, int *total_count, int *unread_count)
{
	debug_enter();
	*total_count = 0;
	*unread_count = 0;

	if (view->mode == EMAIL_MAILBOX_MODE_ALL) {
		if (view->mailbox_type != EMAIL_MAILBOX_TYPE_FLAGGED) {
			email_account_t *account_list = NULL;
			int account_count = 0, i = 0;

			if (email_engine_get_account_list(&account_count, &account_list)) {
				for (i = 0; i < account_count; i++) {
					email_mailbox_t *mailbox = NULL;
					if (email_engine_get_mailbox_by_mailbox_type(account_list[i].account_id,
							(view->mailbox_type == EMAIL_MAILBOX_TYPE_NONE) ?
									EMAIL_MAILBOX_TYPE_INBOX : view->mailbox_type, &mailbox)) {
						*total_count = *total_count + mailbox->total_mail_count_on_local;
						*unread_count = *unread_count + mailbox->unread_count;
						email_engine_free_mailbox_list(&mailbox, 1);
					}
				}
				if (view->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX) {
					*total_count = *total_count - email_get_scheduled_outbox_mail_count_by_account_id(0, false);
				}
				email_engine_free_account_list(&account_list, account_count);
			}
		}
	}
#ifndef _FEATURE_PRIORITY_SENDER_DISABLE_
	else if (view->mode == EMAIL_MAILBOX_MODE_PRIORITY_SENDER) {
		*total_count = email_get_priority_sender_mail_count(false);
		*unread_count = email_get_priority_sender_mail_count(true);
	}
#endif
	else if (view->mode == EMAIL_MAILBOX_MODE_MAILBOX) {
		email_mailbox_t *mailbox = NULL;
		if (email_engine_get_mailbox_by_mailbox_id(view->mailbox_id, &mailbox)) {
			if (view->mailbox_type == EMAIL_MAILBOX_TYPE_DRAFT)
				*total_count = mailbox->total_mail_count_on_local;
			else if (view->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX)
				*total_count = mailbox->total_mail_count_on_local - email_get_scheduled_outbox_mail_count_by_account_id(view->account_id, false);
			else
				*unread_count = mailbox->unread_count;
			email_engine_free_mailbox_list(&mailbox, 1);
		}
	} else {
		debug_warning("INVALID view->mode(%d)", view->mode);
		return EINA_FALSE;
	}
	debug_log("mailbox total count [%d]", *total_count);
	debug_log("mailbox unread count [%d]", *unread_count);

	return EINA_TRUE;
}

void mailbox_view_title_update_mail_count(EmailMailboxView *view)
{
	debug_enter();

	int total_mail_count = 0;
	int unread_mail_count = 0;

	if(_get_total_and_unread_count_from_curr_mailbox(view, &total_mail_count, &unread_mail_count)) {
		view->total_mail_count = total_mail_count;
		view->unread_mail_count = unread_mail_count;
		debug_log("mailbox total mail count = %d", view->total_mail_count);
		debug_log("mailbox unread mail count = %d", view->unread_mail_count);
	} else {
		debug_error("_get_total_and_unread_count_from_curr_mailbox() failed");
	}

	if (view->b_editmode) {
		mailbox_update_select_info(view);
		debug_log("In edit mode, updated count should not be set in Navi title because selection info is shown there.");
		return;
	}

	mailbox_view_title_update_all_without_mailbox_change(view);

	debug_leave();
}

void mailbox_view_title_increase_mail_count(int total_count_incr, int unread_count_incr, EmailMailboxView *view)
{
	debug_log("total param [%d], unread param [%d]", total_count_incr, unread_count_incr);
	debug_log("current total count [%d]", view->total_mail_count);
	debug_log("current unread count [%d]", view->unread_mail_count);

	int serv_total_count = 0;
	int serv_unread_count = 0;
	if (_get_total_and_unread_count_from_curr_mailbox(view, &serv_total_count, &serv_unread_count)) {
		if (view->total_mail_count == serv_total_count) {
			debug_log("Total message count is the same. No need to update");
		} else {
			debug_log("Total message need to be updated by %d", total_count_incr);
			view->total_mail_count = view->total_mail_count + total_count_incr;
		}

		if (unread_count_incr != 0) {
			if (view->unread_mail_count == serv_unread_count) {
				debug_log("Unread message count is the same. No need to update");
			} else {
				debug_log("Unread message count need to be updated by %d", unread_count_incr);
				view->unread_mail_count = view->unread_mail_count + unread_count_incr;
			}
		}
	} else {
		debug_error("_get_total_and_unread_count_from_curr_mailbox() failed!");
		view->total_mail_count = view->total_mail_count + total_count_incr;
		view->unread_mail_count = view->unread_mail_count + unread_count_incr;
	}

	if (view->b_editmode) {
		debug_log("edit mode. just return here.");
		return;
	}

	if (view->unread_mail_count < 0)
		view->unread_mail_count = 0;
	if (view->total_mail_count < 0)
		view->total_mail_count = 0;

	mailbox_view_title_update_all_without_mailbox_change(view);
}
