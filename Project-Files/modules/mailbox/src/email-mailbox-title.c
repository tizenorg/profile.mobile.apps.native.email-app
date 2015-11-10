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
static Eina_Bool _get_total_and_unread_count_from_curr_mailbox(EmailMailboxUGD *mailbox_ugd, int *total_count, int *unread_count);

/*
 * Definition for exported functions
 */
void mailbox_view_title_unpack(EmailMailboxUGD *mailbox_ugd)
{
	debug_enter();
	retm_if(!mailbox_ugd, "mailbox_ugd is NULL");

	Evas_Object *obj = elm_object_part_content_unset(mailbox_ugd->content_layout, "top_bar");
	evas_object_hide(obj);
	debug_leave();
}

void mailbox_view_title_pack(EmailMailboxUGD *mailbox_ugd)
{
	debug_enter();
	retm_if(!mailbox_ugd, "mailbox_ugd is NULL");

	evas_object_show(mailbox_ugd->title_layout);
	elm_object_part_content_set(mailbox_ugd->content_layout, "top_bar", mailbox_ugd->title_layout);
	debug_leave();
}

void mailbox_view_title_update_all(EmailMailboxUGD *mailbox_ugd)
{
	debug_log("mode: [%d], account_id: [%d], mailbox_id: [%d], mailbox_type: [%d], b_searchmode[%d], b_editmode[%d]",
		mailbox_ugd->mode, mailbox_ugd->account_id, mailbox_ugd->mailbox_id, mailbox_ugd->mailbox_type, mailbox_ugd->b_searchmode, mailbox_ugd->b_editmode);

	int total_count = 0;
	int unread_count = 0;
	int ret;

	/* initialize account name and mailbox name */
	G_FREE(mailbox_ugd->account_name);
	G_FREE(mailbox_ugd->mailbox_alias);
	mailbox_ugd->total_mail_count = 0;
	mailbox_ugd->unread_mail_count = 0;

	if (mailbox_ugd->mode == EMAIL_MAILBOX_MODE_ALL) {
		if (mailbox_ugd->mailbox_type != EMAIL_MAILBOX_TYPE_FLAGGED) {
			int mailbox_type = (mailbox_ugd->mailbox_type == EMAIL_MAILBOX_TYPE_NONE) ? EMAIL_MAILBOX_TYPE_INBOX : mailbox_ugd->mailbox_type;
			if (!email_get_combined_mail_count_by_mailbox_type(mailbox_type, &total_count, &unread_count))
				debug_warning("email_get_combined_mail_count_by_mailbox_type() failed");
		}

		switch (mailbox_ugd->mailbox_type) {
		case EMAIL_MAILBOX_TYPE_INBOX:
			mailbox_ugd->mailbox_alias = g_strdup(_("IDS_EMAIL_HEADER_COMBINED_INBOX_ABB"));
			mailbox_ugd->unread_mail_count = unread_count;
			mailbox_ugd->only_local = false;
		break;
		case EMAIL_MAILBOX_TYPE_SENTBOX:
			mailbox_ugd->mailbox_alias = g_strdup(_("IDS_EMAIL_HEADER_SENT_M_EMAIL"));
			mailbox_ugd->unread_mail_count = unread_count;
			mailbox_ugd->only_local = false;
		break;
		case EMAIL_MAILBOX_TYPE_TRASH:
			mailbox_ugd->mailbox_alias = g_strdup(_("IDS_EMAIL_HEADER_RECYCLE_BIN_ABB"));
			mailbox_ugd->unread_mail_count = unread_count;
			mailbox_ugd->only_local = false;
		break;
		case EMAIL_MAILBOX_TYPE_DRAFT:
			mailbox_ugd->mailbox_alias = g_strdup(_("IDS_EMAIL_HEADER_DRAFTS"));
			mailbox_ugd->total_mail_count = total_count;
			mailbox_ugd->only_local = false;
		break;
		case EMAIL_MAILBOX_TYPE_SPAMBOX:
			mailbox_ugd->mailbox_alias = g_strdup(_("IDS_EMAIL_HEADER_SPAMBOX"));
			mailbox_ugd->unread_mail_count = unread_count;
			mailbox_ugd->only_local = false;
		break;
		case EMAIL_MAILBOX_TYPE_OUTBOX:
			mailbox_ugd->mailbox_alias = g_strdup(_("IDS_EMAIL_HEADER_OUTBOX"));
			mailbox_ugd->total_mail_count = total_count;
			mailbox_ugd->only_local = true;
		break;
		case EMAIL_MAILBOX_TYPE_FLAGGED:
			mailbox_ugd->mailbox_alias = g_strdup(_("IDS_EMAIL_HEADER_STARRED_EMAILS_ABB"));
			mailbox_ugd->unread_mail_count = email_get_favourite_mail_count(true);
			mailbox_ugd->only_local = false;
		break;

		default:
			mailbox_ugd->mailbox_alias = g_strdup(_("IDS_EMAIL_HEADER_INBOX"));
			mailbox_ugd->unread_mail_count = unread_count;
			mailbox_ugd->only_local = false;
		break;
		}
		mailbox_ugd->account_type = EMAIL_SERVER_TYPE_NONE;
	}
#ifndef _FEATURE_PRIORITY_SENDER_DISABLE_
	else if (mailbox_ugd->mode == EMAIL_MAILBOX_MODE_PRIORITY_SENDER) {
		mailbox_ugd->mailbox_type = EMAIL_MAILBOX_TYPE_INBOX;
		mailbox_ugd->mailbox_alias = g_strdup(_("IDS_EMAIL_HEADER_PRIORITY_SENDERS_ABB"));
		mailbox_ugd->unread_mail_count = email_get_priority_sender_mail_count(true);
		mailbox_ugd->account_type = EMAIL_SERVER_TYPE_NONE;
		mailbox_ugd->only_local = false;
	}
#endif
	else if (mailbox_ugd->mode == EMAIL_MAILBOX_MODE_MAILBOX) {
		email_account_t *account = NULL;
		ret = email_get_account(mailbox_ugd->account_id, GET_FULL_DATA_WITHOUT_PASSWORD, &account);
		if (ret == EMAIL_ERROR_NONE && account) {
			mailbox_ugd->account_name = g_strdup(account->user_email_address);
			mailbox_ugd->account_type = account->incoming_server_type;
		}
		if (account)
			email_free_account(&account, 1);

		if (mailbox_ugd->mailbox_id > 0) {
			email_mailbox_t *mailbox = NULL;
			ret = email_get_mailbox_by_mailbox_id(mailbox_ugd->mailbox_id, &mailbox);
			if (ret == EMAIL_ERROR_NONE && mailbox) {
				mailbox_ugd->mailbox_type = mailbox->mailbox_type;

				switch (mailbox_ugd->mailbox_type) {
				case EMAIL_MAILBOX_TYPE_INBOX:
					mailbox_ugd->mailbox_alias = g_strdup(_("IDS_EMAIL_HEADER_INBOX"));
					mailbox_ugd->unread_mail_count = mailbox->unread_count - email_get_deleted_flag_mail_count(mailbox_ugd->account_id);
					break;
				case EMAIL_MAILBOX_TYPE_SENTBOX:
					mailbox_ugd->mailbox_alias = g_strdup(_("IDS_EMAIL_HEADER_SENT_M_EMAIL"));
					mailbox_ugd->unread_mail_count = mailbox->unread_count;
					break;
				case EMAIL_MAILBOX_TYPE_TRASH:
					mailbox_ugd->mailbox_alias = g_strdup(_("IDS_EMAIL_HEADER_RECYCLE_BIN_ABB"));
					mailbox_ugd->unread_mail_count = mailbox->unread_count;
					break;
				case EMAIL_MAILBOX_TYPE_DRAFT:
					mailbox_ugd->mailbox_alias = g_strdup(_("IDS_EMAIL_HEADER_DRAFTS"));
					mailbox_ugd->total_mail_count = mailbox->total_mail_count_on_local;
					break;
				case EMAIL_MAILBOX_TYPE_SPAMBOX:
					mailbox_ugd->mailbox_alias = g_strdup(_("IDS_EMAIL_HEADER_SPAMBOX"));
					mailbox_ugd->unread_mail_count = mailbox->unread_count;
					break;
				case EMAIL_MAILBOX_TYPE_OUTBOX:
					mailbox_ugd->mailbox_alias = g_strdup(_("IDS_EMAIL_HEADER_OUTBOX"));
					mailbox_ugd->total_mail_count = mailbox->total_mail_count_on_local - email_get_scheduled_outbox_mail_count_by_account_id(mailbox->account_id, false);
					break;
				default:
					mailbox_ugd->mailbox_alias = g_strdup(mailbox->alias);
					mailbox_ugd->unread_mail_count = mailbox->unread_count;
					break;
				}
				mailbox_ugd->only_local = mailbox->local;

				mailbox_set_last_updated_time(mailbox->last_sync_time, mailbox_ugd);
			}
			email_free_mailbox(&mailbox, 1);
		} else if (mailbox_ugd->mailbox_id == 0) {
			/* This is for the first entrance during creating an account. */
			mailbox_ugd->mailbox_type = EMAIL_MAILBOX_TYPE_INBOX;
			mailbox_ugd->mailbox_alias = g_strdup(_("IDS_EMAIL_HEADER_INBOX"));
			mailbox_ugd->unread_mail_count = 0;
			mailbox_ugd->only_local = false;
		} else {
			debug_warning("INVALID mailbox_ugd->mailbox_id(%d)", mailbox_ugd->mailbox_id);
			return;
		}
	} else {
		debug_warning("INVALID mailbox_ugd->mode(%d)", mailbox_ugd->mode);
		return;
	}

	debug_log("mailbox_id: [%d], mailbox_type: [%d]", mailbox_ugd->mailbox_id, mailbox_ugd->mailbox_type);
	mailbox_view_title_update_all_without_mailbox_change(mailbox_ugd);

}

void mailbox_view_title_update_all_without_mailbox_change(EmailMailboxUGD *mailbox_ugd)
{
	debug_log("mode: [%d], account_id: [%d], mailbox_id: [%d], mailbox_type: [%d], b_searchmode[%d], b_editmode[%d]",
		mailbox_ugd->mode, mailbox_ugd->account_id, mailbox_ugd->mailbox_id, mailbox_ugd->mailbox_type, mailbox_ugd->b_searchmode, mailbox_ugd->b_editmode);

	char mailbox_name_buf[MAX_STR_LEN] = { 0, };
	char mailbox_alias_buf[MAX_STR_LEN] = { 0, };
	char *markup_title = NULL;
	char *markup_subtitle = NULL;

	char *lang = NULL;
	system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_LANGUAGE, &lang);
	if (mailbox_ugd->mode == EMAIL_MAILBOX_MODE_MAILBOX) {
		/* two line title */
		if (mailbox_ugd->mailbox_type == EMAIL_MAILBOX_TYPE_DRAFT || mailbox_ugd->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX) {
			if (mailbox_ugd->total_mail_count == 0)
				snprintf(mailbox_name_buf, sizeof(mailbox_name_buf), TITLE_TEXT_FORMAT, mailbox_ugd->mailbox_alias);
			else
				snprintf(mailbox_name_buf, sizeof(mailbox_name_buf), TITLE_TEXT_WITH_COUNT_FORMAT, mailbox_ugd->mailbox_alias, mailbox_ugd->total_mail_count);
		} else {
			if (mailbox_ugd->unread_mail_count == 0)
				snprintf(mailbox_name_buf, sizeof(mailbox_name_buf), TITLE_TEXT_FORMAT, mailbox_ugd->mailbox_alias);
			else
				snprintf(mailbox_name_buf, sizeof(mailbox_name_buf), TITLE_TEXT_WITH_COUNT_FORMAT, mailbox_ugd->mailbox_alias, mailbox_ugd->unread_mail_count);
		}

		markup_subtitle = elm_entry_utf8_to_markup(mailbox_ugd->account_name);
		snprintf(mailbox_alias_buf, sizeof(mailbox_alias_buf), TITLE_TEXT_FORMAT, markup_subtitle);

		elm_layout_signal_emit(mailbox_ugd->title_layout, "two_lines_mode", "mailbox");
		elm_object_part_text_set(mailbox_ugd->title_layout, "title.text", mailbox_name_buf);
		elm_object_part_text_set(mailbox_ugd->title_layout, "title.text.sub", mailbox_alias_buf);


	} else {
		/*
		 * There's possibility that the flag status of all selected starred mails are changed on edit mode.
		 * So, If selected box is starred, update mail count when exit edit mode.
		 */
		/* one line title */
		if (mailbox_ugd->mode == EMAIL_MAILBOX_MODE_ALL && mailbox_ugd->mailbox_type == EMAIL_MAILBOX_TYPE_FLAGGED) {
			mailbox_ugd->unread_mail_count = email_get_favourite_mail_count(true);
			if (mailbox_ugd->unread_mail_count == 0)
				snprintf(mailbox_name_buf, sizeof(mailbox_name_buf), TITLE_TEXT_FORMAT, mailbox_ugd->mailbox_alias);
			else
				snprintf(mailbox_name_buf, sizeof(mailbox_name_buf), TITLE_TEXT_WITH_COUNT_FORMAT, mailbox_ugd->mailbox_alias, mailbox_ugd->unread_mail_count);
		} else if (mailbox_ugd->mailbox_type == EMAIL_MAILBOX_TYPE_DRAFT || mailbox_ugd->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX) {
			if (mailbox_ugd->total_mail_count == 0)
				snprintf(mailbox_name_buf, sizeof(mailbox_name_buf), TITLE_TEXT_FORMAT, mailbox_ugd->mailbox_alias);
			else
				snprintf(mailbox_name_buf, sizeof(mailbox_name_buf), TITLE_TEXT_WITH_COUNT_FORMAT, mailbox_ugd->mailbox_alias, mailbox_ugd->total_mail_count);
		} else {
			if (mailbox_ugd->unread_mail_count == 0)
				snprintf(mailbox_name_buf, sizeof(mailbox_name_buf), TITLE_TEXT_FORMAT, mailbox_ugd->mailbox_alias);
			else
				snprintf(mailbox_name_buf, sizeof(mailbox_name_buf), TITLE_TEXT_WITH_COUNT_FORMAT, mailbox_ugd->mailbox_alias, mailbox_ugd->unread_mail_count);
		}

		elm_layout_signal_emit(mailbox_ugd->title_layout, "single_line_mode", "mailbox");
		elm_object_part_text_set(mailbox_ugd->title_layout, "title.text.sub", NULL);
		elm_object_part_text_set(mailbox_ugd->title_layout, "title.text", mailbox_name_buf);
	}
	FREE(lang);
	FREE(markup_title);
	FREE(markup_subtitle);
	mailbox_toolbar_create(mailbox_ugd);
	mailbox_naviframe_mailbox_button_add(mailbox_ugd);
	if (!mailbox_ugd->b_editmode && mailbox_ugd->b_searchmode) {
		mailbox_change_search_layout_state(mailbox_ugd, true);
	}
}

void mailbox_view_title_update_account_name(EmailMailboxUGD *mailbox_ugd)
{
	debug_log("mode: [%d], account_id: [%d], b_searchmode[%d], b_editmode[%d]",
		mailbox_ugd->mode, mailbox_ugd->account_id, mailbox_ugd->b_searchmode, mailbox_ugd->b_editmode);

	int ret = 0;
	email_account_t *account = NULL;
	char *markup_subtitle = NULL;
	char mailbox_alias_buf[MAX_STR_LEN] = { 0, };

	ret = email_get_account(mailbox_ugd->account_id, EMAIL_ACC_GET_OPT_DEFAULT, &account);
	if (ret == EMAIL_ERROR_NONE && account) {
		G_FREE(mailbox_ugd->account_name);
		mailbox_ugd->account_name = g_strdup(account->user_email_address);
	}
	if (account)
		email_free_account(&account, 1);

	if (mailbox_ugd->b_editmode)
		return;

	if (mailbox_ugd->mode == EMAIL_MAILBOX_MODE_MAILBOX) {
		/* two line title */
		markup_subtitle = elm_entry_utf8_to_markup(mailbox_ugd->account_name);
		snprintf(mailbox_alias_buf, sizeof(mailbox_alias_buf), TITLE_TEXT_FORMAT, markup_subtitle);
		elm_layout_signal_emit(mailbox_ugd->title_layout, "two_lines_mode", "mailbox");
		elm_object_part_text_set(mailbox_ugd->title_layout, "title.text.sub", mailbox_alias_buf);
	}

	FREE(markup_subtitle);
}

static Eina_Bool _get_total_and_unread_count_from_curr_mailbox(EmailMailboxUGD *mailbox_ugd, int *total_count, int *unread_count)
{
	debug_enter();
	int ret;
	*total_count = 0;
	*unread_count = 0;

	if (mailbox_ugd->mode == EMAIL_MAILBOX_MODE_ALL) {
		if (mailbox_ugd->mailbox_type != EMAIL_MAILBOX_TYPE_FLAGGED) {
			email_account_t *account_list = NULL;
			int account_count = 0, i = 0;

			ret = email_get_account_list(&account_list, &account_count);
			if (ret == EMAIL_ERROR_NONE && account_list) {
				for (i = 0; i < account_count; i++) {
					email_mailbox_t *mailbox = NULL;
					ret = email_get_mailbox_by_mailbox_type(account_list[i].account_id,
							(mailbox_ugd->mailbox_type == EMAIL_MAILBOX_TYPE_NONE) ? EMAIL_MAILBOX_TYPE_INBOX : mailbox_ugd->mailbox_type, &mailbox);
					if (ret == EMAIL_ERROR_NONE && mailbox) {
						*total_count = *total_count + mailbox->total_mail_count_on_local;
						*unread_count = *unread_count + mailbox->unread_count;
						email_free_mailbox(&mailbox, 1);
					}
				}
				if (mailbox_ugd->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX) {
					*total_count = *total_count - email_get_scheduled_outbox_mail_count_by_account_id(0, false);
				}
			}
			if (account_list)
				email_free_account(&account_list, account_count);
		}
	}
#ifndef _FEATURE_PRIORITY_SENDER_DISABLE_
	else if (mailbox_ugd->mode == EMAIL_MAILBOX_MODE_PRIORITY_SENDER) {
		*total_count = email_get_priority_sender_mail_count(false);
		*unread_count = email_get_priority_sender_mail_count(true);
	}
#endif
	else if (mailbox_ugd->mode == EMAIL_MAILBOX_MODE_MAILBOX) {
		email_mailbox_t *mailbox = NULL;
		ret = email_get_mailbox_by_mailbox_id(mailbox_ugd->mailbox_id, &mailbox);
		if (ret == EMAIL_ERROR_NONE && mailbox) {
			if (mailbox_ugd->mailbox_type == EMAIL_MAILBOX_TYPE_DRAFT)
				*total_count = mailbox->total_mail_count_on_local;
			else if (mailbox_ugd->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX)
				*total_count = mailbox->total_mail_count_on_local - email_get_scheduled_outbox_mail_count_by_account_id(mailbox_ugd->account_id, false);
			else
				*unread_count = mailbox->unread_count;
		}
		email_free_mailbox(&mailbox, 1);
	} else {
		debug_warning("INVALID mailbox_ugd->mode(%d)", mailbox_ugd->mode);
		return EINA_FALSE;
	}
	debug_log("mailbox total count [%d]", *total_count);
	debug_log("mailbox unread count [%d]", *unread_count);

	return EINA_TRUE;
}

void mailbox_view_title_update_mail_count(EmailMailboxUGD *mailbox_ugd)
{
	debug_enter();

	int total_mail_count = 0;
	int unread_mail_count = 0;

	if(_get_total_and_unread_count_from_curr_mailbox(mailbox_ugd, &total_mail_count, &unread_mail_count)) {
		mailbox_ugd->total_mail_count = total_mail_count;
		mailbox_ugd->unread_mail_count = unread_mail_count;
		debug_log("mailbox total mail count = %d", mailbox_ugd->total_mail_count);
		debug_log("mailbox unread mail count = %d", mailbox_ugd->unread_mail_count);
	} else {
		debug_error("_get_total_and_unread_count_from_curr_mailbox() failed");
	}

	if (mailbox_ugd->b_editmode) {
		mailbox_update_select_info(mailbox_ugd);
		debug_log("In edit mode, updated count should not be set in Navi title because selection info is shown there.");
		return;
	}

	mailbox_view_title_update_all_without_mailbox_change(mailbox_ugd);

	debug_leave();
}

void mailbox_view_title_increase_mail_count(int total_count_incr, int unread_count_incr, EmailMailboxUGD *mailbox_ugd)
{
	debug_log("total param [%d], unread param [%d]", total_count_incr, unread_count_incr);
	debug_log("current total count [%d]", mailbox_ugd->total_mail_count);
	debug_log("current unread count [%d]", mailbox_ugd->unread_mail_count);

	int serv_total_count = 0;
	int serv_unread_count = 0;
	if (_get_total_and_unread_count_from_curr_mailbox(mailbox_ugd, &serv_total_count, &serv_unread_count)) {
		if (mailbox_ugd->total_mail_count == serv_total_count) {
			debug_log("Total message count is the same. No need to update");
		} else {
			debug_log("Total message need to be updated by %d", total_count_incr);
			mailbox_ugd->total_mail_count = mailbox_ugd->total_mail_count + total_count_incr;
		}

		if (unread_count_incr != 0) {
			if (mailbox_ugd->unread_mail_count == serv_unread_count) {
				debug_log("Unread message count is the same. No need to update");
			} else {
				debug_log("Unread message count need to be updated by %d", unread_count_incr);
				mailbox_ugd->unread_mail_count = mailbox_ugd->unread_mail_count + unread_count_incr;
			}
		}
	} else {
		debug_error("_get_total_and_unread_count_from_curr_mailbox() failed!");
		mailbox_ugd->total_mail_count = mailbox_ugd->total_mail_count + total_count_incr;
		mailbox_ugd->unread_mail_count = mailbox_ugd->unread_mail_count + unread_count_incr;
	}

	if (mailbox_ugd->b_editmode) {
		debug_log("edit mode. just return here.");
		return;
	}

	if (mailbox_ugd->unread_mail_count < 0)
		mailbox_ugd->unread_mail_count = 0;
	if (mailbox_ugd->total_mail_count < 0)
		mailbox_ugd->total_mail_count = 0;

	mailbox_view_title_update_all_without_mailbox_change(mailbox_ugd);
}
