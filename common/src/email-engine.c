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

#include <ctype.h>
#include <string.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <email-api.h>
#include "email-debug.h"
#include "email-utils.h"
#include "email-engine.h"

#define QUERY_SIZE 1024

#define ACCOUNT_MIN						-1


static guint _g_edb_ref_count = 0;

/* Internal.
 */

/* Exported.
 */
EMAIL_API gboolean email_engine_initialize(void)
{
	debug_enter();
	++_g_edb_ref_count;

	if (_g_edb_ref_count > 1) {
		debug_log("already opened - EDB ref_count(%d)", _g_edb_ref_count);
		return TRUE;
	}

	return email_engine_initialize_force();
}

EMAIL_API gboolean email_engine_initialize_force(void)
{
	debug_enter();

	int err = 0;

	debug_log("email_service_begin");

	err = email_service_begin();
	if (err != EMAIL_ERROR_NONE) {
		debug_critical("fail to email_service_begin - err(%d)", err);
		return FALSE;
	}

	err = email_open_db();
	if (err != EMAIL_ERROR_NONE) {
		debug_critical("fail to open db - err(%d)", err);
		return FALSE;
	}

	return TRUE;
}

EMAIL_API void email_engine_finalize(void)
{
	debug_enter();
	--_g_edb_ref_count;

	if (_g_edb_ref_count > 0) {
		debug_log("remain EDB ref_count(%d)", _g_edb_ref_count);
		return;
	}

	email_engine_finalize_force();
}

EMAIL_API void email_engine_finalize_force(void)
{
	debug_enter();

	int err = 0;

	err = email_close_db();
	if (err != EMAIL_ERROR_NONE) {
		debug_critical("fail to close db - err(%d)", err);
	}

	debug_log("email_service_end");

	err = email_service_end();
	if (err != EMAIL_ERROR_NONE) {
		debug_critical("fail to email_service_end - err(%d)", err);
	}
}

EMAIL_API gboolean email_engine_validate_account(email_account_t *_account, int *handle, int *error_code)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(_account != NULL, FALSE);

	int err = 0;

	err = email_validate_account_ex(_account, handle);
	if (error_code != NULL)
		*error_code = err;
	if (err != EMAIL_ERROR_NONE) {
		debug_critical("Fail to Validate Account");
		debug_critical("Error code(%d)", err);
		return FALSE;
	}

	return TRUE;
}

EMAIL_API gboolean email_engine_add_account(email_account_t *_account, int *account_id, int *error_code)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(_account != NULL, FALSE);

	int err = 0;

	err = email_add_account(_account);
	if (error_code != NULL)
		*error_code = err;
	if (err != EMAIL_ERROR_NONE) {
		debug_critical("Fail to Create Account");
		debug_critical("Error code(%d)", err);
		return FALSE;
	}
	debug_log("Succeed in adding account");
	*account_id = _account->account_id;
	debug_log("account id is %d", _account->account_id);

	return TRUE;
}

EMAIL_API gboolean email_engine_sync_imap_mailbox_list(gint account_id, int *handle)
{
	debug_log("account_id(%d)", account_id);

	RETURN_VAL_IF_FAIL(account_id != 0, FALSE);

	int err = 0;

	err = email_sync_imap_mailbox_list(account_id, handle);
	if (err != EMAIL_ERROR_NONE) {
		debug_critical("email_sync_imap_mailbox_list() failed(%d)", err);
		return FALSE;
	} else {
		debug_log("email_sync_imap_mailbox_list success");
	}

	return TRUE;
}

EMAIL_API gboolean email_engine_get_smtp_mail_size(gint account_id, int *handle)
{
	debug_log("account_id(%d)", account_id);

	RETURN_VAL_IF_FAIL(account_id != 0, FALSE);

	int err = 0;

	err = email_query_smtp_mail_size_limit(account_id, handle);
	if (err != EMAIL_ERROR_NONE) {
		debug_critical("email_query_smtp_mail_size_limit failed(%d)", err);
		return FALSE;
	}

	return TRUE;
}

EMAIL_API gboolean email_engine_add_account_with_validation(email_account_t *_account, int *account_id, int *handle, int *error_code)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(_account != NULL, FALSE);

	int err = 0;

	err = email_add_account_with_validation(_account, handle);
	if (error_code != NULL)
		*error_code = err;
	if (err != EMAIL_ERROR_NONE) {
		debug_critical("Fail to Create Account with validation");
		debug_critical("Error code(%d)", err);
		return FALSE;
	}

	debug_log("Succeed in adding account with validation");
	*account_id = _account->account_id;
	debug_log("account id is %d", _account->account_id);

	return TRUE;
}

EMAIL_API gboolean email_engine_update_account(gint account_id, email_account_t *_account)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(account_id != 0, FALSE);
	RETURN_VAL_IF_FAIL(_account != NULL, FALSE);

	int err = 0;

	err = email_update_account(account_id, _account);
	if (err == EMAIL_ERROR_NONE) {
		debug_log("Suceeded in email_update_account");
		return TRUE;
	} else {
		debug_critical("Failed to update account Err(%d)", err);
		return FALSE;
	}
}

EMAIL_API gboolean email_engine_update_account_with_validation(gint account_id, email_account_t *_account)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(account_id != 0, FALSE);
	RETURN_VAL_IF_FAIL(_account != NULL, FALSE);

	int err = 0;

	err = email_update_account_with_validation(account_id, _account);
	if (err == EMAIL_ERROR_NONE) {
		debug_log("Suceeded in email_update_account_with_validation");
		return TRUE;
	} else {
		debug_critical("Failed to update account with validation Err(%d)", err);
		return FALSE;
	}
}

EMAIL_API gboolean email_engine_delete_account(gint account_id)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(account_id != 0, FALSE);

	int err = 0;

	err = email_delete_account(account_id);
	if (err == EMAIL_ERROR_NONE) {
		debug_log("Account is Successfully deleted");
		return TRUE;
	} else {
		debug_critical("Failed to delete account Err(%d)", err);
		return FALSE;
	}

}

EMAIL_API gboolean email_engine_get_account_list(int *count, email_account_t **_account_list)
{
	debug_enter();
	int i, err = 0;

	err = email_get_account_list(_account_list, count);
	if ((err != EMAIL_ERROR_NONE) || (*_account_list && (*count <= 0))) {
		*count = 0;
		*_account_list = NULL;
		if (err != EMAIL_ERROR_ACCOUNT_NOT_FOUND) {
			debug_warning("email_get_account_list error Err(%d)", err);
			return FALSE;
		}
		debug_log("No accounts found");
	}

	if (!*_account_list) {
		*count = 0;
	}

	debug_log("valid account count :(%d)", *count);

	for (i = 0; i < *count; i++) {
		debug_secure("%2d) %-15s %-30s\n", (*_account_list)[i].account_id, (*_account_list)[i].account_name, (*_account_list)[i].user_email_address);
	}
	debug_log("Get All Account List");

	return TRUE;
}

EMAIL_API gboolean email_engine_free_account_list(email_account_t **_account_list, int count)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(_account_list != NULL, FALSE);
	RETURN_VAL_IF_FAIL(count > 0, FALSE);

	if (!*_account_list) {
		return TRUE;
	}

	int err = 0;

	err = email_free_account(_account_list, count);
	*_account_list = NULL;
	if (err != EMAIL_ERROR_NONE) {
		debug_critical("Fail to free account list Err(%d)", err);
		return FALSE;
	}

	debug_log("Succeed in freeing account list");
	return TRUE;
}

EMAIL_API gboolean email_engine_get_account_data(int acctid, int options, email_account_t **account)
{
	debug_enter();
	debug_log("acctid: %d", acctid);
	RETURN_VAL_IF_FAIL(acctid > ACCOUNT_MIN, FALSE);
	int err = 0;

	err = email_get_account(acctid, options, account);
	if (err != EMAIL_ERROR_NONE) {
		debug_critical("email_get_account() failed(%d)", err);

		/* email-service API can return error value even though account is allocated. */
		if (*account) {
			if (!email_engine_free_account_list(account, 1)) {
				debug_warning("email_engine_free_account_list() failed! it'll cause memory leak!");
			}
			*account = NULL;
		}
		return FALSE;
	}

	if (!*account) {
		debug_critical("account is NULL");
		return FALSE;
	}

	return TRUE;
}

EMAIL_API gboolean email_engine_get_account_full_data(int acctid, email_account_t **account)
{
	debug_enter();

	if (!email_engine_get_account_data(acctid, GET_FULL_DATA_WITHOUT_PASSWORD, account)) {
		debug_error("email_engine_get_account_data() failed");
		return FALSE;
	}

	debug_secure("Account name: %s", (*account)->account_name);
	if ((*account)->options.signature) {
		debug_secure("Signature: %s", (*account)->options.signature);
	}

	return TRUE;
}

EMAIL_API gboolean email_engine_set_default_account(gint account_id)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(account_id > ACCOUNT_MIN, FALSE);
	int err = 0;

	err = email_save_default_account_id(account_id);
	debug_log("email_save_default_account_id returns %d.", err);
	if (err != EMAIL_ERROR_NONE) {
		debug_critical("email_save_default_account_id: Err(%d)", err);
		return FALSE;
	} else {
		debug_log("default account is set as account_id %d.", account_id);
		return TRUE;
	}
}

EMAIL_API gboolean email_engine_get_default_account(gint *account_id)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(account_id != NULL, FALSE);

	email_account_t *_account = NULL;
	email_account_t *_account_list = NULL;
	int count = 0;
	int err = 0;

	err = email_load_default_account_id(account_id);
	debug_log("email_load_default_account_id returns %d.", err);

	/* if account_id is default account, then check account_id whether it is valid or not */
	if (err == EMAIL_ERROR_NONE) {
		debug_log("default account id is %d.", *account_id);
		if ((email_get_account(*account_id, EMAIL_ACC_GET_OPT_DEFAULT, &_account) == EMAIL_ERROR_NONE) &&
			(_account != NULL)) {
			email_free_account(&_account, 1);
			return TRUE;
		}
	}

	if (_account) {
		email_free_account(&_account, 1);
	}

	/* if slp_ret have no value or account id is not valid */
	err = email_get_account_list(&_account_list, &count);
	if (err != EMAIL_ERROR_NONE) {
		debug_warning("fail to get account list - err(%d)", err);
		email_free_account(&_account_list, count);
		return FALSE;
	}

	/* no account */
	if (_account_list == NULL) {
		debug_log("account info is NULL");
		return FALSE;
	}

	if (count > 0) {
		*account_id = _account_list[0].account_id;
		debug_log("account id (%d)", *account_id);
	} else {
		*account_id = 0;
		debug_warning("count: %d", count);
	}

	err = email_free_account(&_account_list, count);
	if (err != EMAIL_ERROR_NONE) {
		debug_critical("fail to free account - err(%d)", err);
		return FALSE;
	}

	RETURN_VAL_IF_FAIL((*account_id) > 0, FALSE);
	email_engine_set_default_account(*account_id);

	return TRUE;
}

EMAIL_API gboolean email_engine_get_account_validation(email_account_t *_account, int *handle)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(_account != NULL, FALSE);

	int err = 0;

	err = email_validate_account_ex(_account, handle);

	if (err != EMAIL_ERROR_NONE) {
		debug_log("Validation Failed");
		return FALSE;
	} else {
		debug_log("Validation Succeeded");
		return TRUE;
	}
}

EMAIL_API gboolean email_engine_get_folder_list(gint account_id, int *handle)
{
	debug_enter();
	debug_log("account id:%d", account_id);
	RETURN_VAL_IF_FAIL(account_id > ACCOUNT_MIN, FALSE);
	int err = 0;

	err = email_sync_imap_mailbox_list(account_id, handle);
	if (err != EMAIL_ERROR_NONE) {
		debug_log("handle valuse : %d", *handle);
		return TRUE;
	} else {
		debug_log("Error");
		return FALSE;
	}
}

/* the _list should be free after use */
EMAIL_API email_mail_list_item_t *email_engine_get_mail_by_mailid(int mail_id)
{
	debug_enter();

	char _where[256] = {0};
	snprintf(_where, 255, "where mail_id=%d", mail_id);

	int count = 0;
	email_mail_list_item_t *mail_info = NULL;

	int _e = email_query_mail_list(_where, &mail_info, &count);
	if (_e != EMAIL_ERROR_NONE || !mail_info || count != 1) {
		debug_warning("get mail info - err (%d) or list NULL(%p) or _list != 1 (%d)",
							_e, mail_info, count);
		goto CLEANUP;
	}

	return mail_info;

CLEANUP:
	FREE(mail_info);
	return NULL;
}

EMAIL_API gchar *email_engine_get_mailbox_service_name(const gchar *folder_name)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(STR_VALID(folder_name), NULL);

	if (!strstr(folder_name, "@"))
		return NULL;

	gchar *folder = NULL;

	folder = g_strdup(folder_name);

	RETURN_VAL_IF_FAIL(folder != NULL, NULL);

	gchar **token_list = g_strsplit_set(folder, "/", -1);

	g_free(folder);	/* MUST BE */

	RETURN_VAL_IF_FAIL(token_list != NULL, NULL);

	gchar *name = g_strdup(token_list[0]);

	g_strfreev(token_list);	/* MUST BE */

	return name;
}

EMAIL_API email_mailbox_list_info_t *email_engine_get_mailbox_info(gint account_id, const gchar *folder_name)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(account_id > ACCOUNT_MIN, NULL);
	RETURN_VAL_IF_FAIL(STR_VALID(folder_name), NULL);

	email_mailbox_list_info_t *info = NULL;
	email_mailbox_t *mailbox_list = NULL;
	int err = 0;
	int count = 0;
	int i = 0;

	err = email_get_mailbox_list(account_id, EMAIL_MAILBOX_ALL, &mailbox_list, &count);

	if (err != EMAIL_ERROR_NONE) {
		debug_warning("fail to mailbox list - err(%d)", err);
		if (mailbox_list != NULL) {
			goto error;
		}
		return NULL;
	}

	if (mailbox_list == NULL) {
		debug_critical("mailbox_list is @niL");
		return NULL;
	}

	for (i = 0; i < count; ++i) {

		if (STR_CMP(mailbox_list[i].mailbox_name, folder_name)) {

			info = g_new0(email_mailbox_list_info_t, 1);

			if (info != NULL) {
				if (STR_VALID(mailbox_list[i].mailbox_name)) {
					info->name = g_strdup(mailbox_list[i].mailbox_name);
				}
				if (STR_VALID(mailbox_list[i].alias)) {
					info->alias = g_strdup(mailbox_list[i].alias);
				}

				int err = 0;
				email_account_t *account = NULL;
				if ((err = email_get_account(mailbox_list[i].account_id, EMAIL_ACC_GET_OPT_ACCOUNT_NAME, &account)) != EMAIL_ERROR_NONE) {
					debug_critical("fail to get mail - err (%d)", err);
				}

				if (account) {
					debug_log("email_free_account is called");

					if (STR_VALID(account->account_name)) {
						info->account_name = g_strdup(account->account_name);
					}

					err = email_free_account(&account, 1);
					account = NULL;

					if (err != EMAIL_ERROR_NONE) {
						debug_critical("failed to free account info - err (%d)", err);
					}
				}
				info->mail_addr = email_engine_get_mailbox_service_name(mailbox_list[i].mailbox_name);

				info->unread_count = mailbox_list[i].unread_count;
			}
			break;
		}
	}

error:
	err = email_free_mailbox(&mailbox_list, count);

	if (err != EMAIL_ERROR_NONE) {
		debug_warning("fail to free mailbox");
	}

	return info;
}

EMAIL_API void email_engine_free_mailbox_info(email_mailbox_list_info_t **info)
{
	debug_enter();
	RETURN_IF_FAIL(*info != NULL);

	email_mailbox_list_info_t *d = (*info);

	if (d != NULL) {
		if (d->name) {
			g_free(d->name);
		}
		if (d->alias) {
			g_free(d->alias);
		}
		if (d->account_name) {
			g_free(d->account_name);
		}
		if (d->mail_addr) {
			g_free(d->mail_addr);
		}
		if (d->thumb_path) {
			g_free(d->thumb_path);
		}
		g_free(d);
		*info = NULL;
	}
}

EMAIL_API gchar *email_engine_convert_from_folder_to_srcbox(gint account_id, email_mailbox_type_e mailbox_type)
{
	debug_enter();
	debug_log("account_id: %d", account_id);

	int err = 0;
	email_mailbox_t *mailbox_list = NULL;
	gchar *mailbox_name = NULL;

	err = email_get_mailbox_by_mailbox_type(account_id, mailbox_type, &mailbox_list);
	if (err != EMAIL_ERROR_NONE) {
		debug_log("err: %d", err);
		return g_strdup("");
	}

	if (mailbox_list) {

		debug_secure("mailbox_list: %s", mailbox_list->mailbox_name);
		debug_secure("mailbox_alias: %s", mailbox_list->alias);
		mailbox_name = g_strdup(mailbox_list->mailbox_name);

		email_free_mailbox(&mailbox_list, 1);
		mailbox_list = NULL;
	} else {
		mailbox_name = g_strdup("");
	}

	return mailbox_name;
}

EMAIL_API gboolean email_engine_get_mail_list(email_list_filter_t *filter_list, int filter_count,
		email_list_sorting_rule_t *sorting_rule_list, int sorting_rule_count,
		int start_index, int limit_count, email_mail_list_item_t **mail_list, int *mail_count)
{
	debug_enter();
	retvm_if(!mail_list, FALSE, "mail_list is NULL");
	retvm_if(!mail_count, FALSE, "mail_count is NULL");

	*mail_list = NULL;
	*mail_count = 0;

	int err = email_get_mail_list_ex(filter_list, filter_count, sorting_rule_list, sorting_rule_count,
			start_index, limit_count, mail_list, mail_count);
	if ((err != EMAIL_ERROR_NONE) || (*mail_list && (*mail_count <= 0))) {
		*mail_list = NULL;
		*mail_count = 0;
		debug_error("email_get_mail_list_ex failed: %d;", err);
		return FALSE;
	}

	if (!*mail_list) {
		*mail_count = 0;
	}

	return TRUE;
}

EMAIL_API gboolean email_engine_get_mail_count(email_list_filter_t *filter_list, int filter_count,
		int *total_mail_count, int *unseen_mail_count)
{
	debug_enter();
	retvm_if(!total_mail_count, FALSE, "total_mail_count is NULL");
	retvm_if(!unseen_mail_count, FALSE, "unseen_mail_count is NULL");

	*total_mail_count = 0;
	*unseen_mail_count = 0;

	int err = email_count_mail(filter_list, filter_count, total_mail_count, unseen_mail_count);
	if ((err != EMAIL_ERROR_NONE) || (*total_mail_count < *unseen_mail_count)) {
		*total_mail_count = 0;
		*unseen_mail_count = 0;
		debug_error("email_count_mail failed: %d;", err);
		return FALSE;
	}

	return TRUE;
}

EMAIL_API gboolean email_engine_sync_folder(gint account_id, int mailbox_id, int *handle)
{
	debug_enter();
	debug_log("account_id(%d), mailbox_id(%d)", account_id, mailbox_id);

	gboolean res = FALSE;
	int email_handle = 0;
	int err = 0;

	err = email_sync_header(account_id, mailbox_id, &email_handle);
	debug_log("email_handle: %d", email_handle);

	if (err != EMAIL_ERROR_NONE) {
		debug_critical("fail to sync current folder - err (%d)", err);
		res = FALSE;
	} else {
		res = TRUE;
	}

	if (handle != NULL) {
		debug_log("email_handle for folder sync: %d", email_handle);
		*handle = email_handle;
	}

	return res;
}

EMAIL_API gboolean email_engine_sync_all_folder(gint account_id, int *handle1, int *handle2)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(account_id > ACCOUNT_MIN, FALSE);

	debug_log("account id (%d)", account_id);

	int email_handle1 = 0;
	int email_handle2 = 0;
	int err = 0;

	/* 2nd parameter must be set empty string.
	If you set NULL, email_get_imap_mailbox_list function doesn't work. */
	err = email_sync_imap_mailbox_list(account_id, &email_handle1);

	if (err != EMAIL_ERROR_NONE) {
		debug_critical("fail to get imap folder list - err(%d)", err);
		return FALSE;
	}

	err = email_sync_header(account_id, 0, &email_handle2);	/* 0 : all folders. */

	if (err != EMAIL_ERROR_NONE) {
		debug_critical("fail to sync all folders - err (%d)", err);
		return FALSE;
	}

	if (handle1 != NULL) {
		*handle1 = email_handle1;
	}

	if (handle2 != NULL) {
		*handle2 = email_handle2;
	}

	return TRUE;
}

EMAIL_API void email_engine_stop_working(gint account_id, int handle)
{
	debug_enter();
	debug_log("account_id:%d, handle:%d", account_id, handle);

	RETURN_IF_FAIL(handle != 0);

	int err = 0;

	debug_log("handle (%d)", handle);

	err = email_cancel_job(account_id, handle, EMAIL_CANCELED_BY_USER);

	if (err != EMAIL_ERROR_NONE) {
		debug_warning("fail to cancel job");
	}
}

EMAIL_API void email_engine_free_mail_sender_list(GList **list)
{
	debug_enter();
	if (!list || !*list) return;

	GList *cur = g_list_first(*list);
	for (; cur; cur = g_list_next(cur)) {
		gchar *d = (gchar *)g_list_nth_data(cur, 0);
		if (d) {
			g_free(d);
		}
	}
	g_list_free(*list);
	*list = NULL;
}

EMAIL_API gboolean email_engine_check_seen_mail(gint account_id, gint mail_id)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(account_id > ACCOUNT_MIN, FALSE);
	RETURN_VAL_IF_FAIL(mail_id > 0, FALSE);

	int res = 0;
	int err = 0;
	email_mail_data_t *mail_info = NULL;

	if ((err = email_get_mail_data(mail_id, &mail_info)) != EMAIL_ERROR_NONE) {
		debug_log("fail to get mail data - err (%d)", err);
		email_free_mail_data(&mail_info, 1);
		return 0;
	}

	if (mail_info == NULL) {
		debug_critical("mail_info is @niL");
		return 0;
	}

	res = mail_info->flags_seen_field;
	debug_log("flags_seen_field: %d", res);

	err = email_free_mail_data(&mail_info, 1);

	if (err != EMAIL_ERROR_NONE) {
		debug_critical("fail to free mail info - err (%d)", err);
	}

	return res;
}

EMAIL_API gboolean email_engine_check_draft_mail(gint account_id, gint mail_id)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(account_id > ACCOUNT_MIN, FALSE);
	RETURN_VAL_IF_FAIL(mail_id > 0, FALSE);

	int res = 0;
	int err = 0;
	email_mail_data_t *mail_info = NULL;

	if ((err = email_get_mail_data(mail_id, &mail_info)) != EMAIL_ERROR_NONE) {
		debug_log("fail to get mail data - err (%d)", err);
		email_free_mail_data(&mail_info, 1);
		return 0;
	}

	if (mail_info == NULL) {
		debug_critical("mail_info is @niL");
		return 0;
	}

	res = mail_info->flags_draft_field;
	debug_log("flags_draft_field: %d", res);

	err = email_free_mail_data(&mail_info, 1);

	if (err != EMAIL_ERROR_NONE) {
		debug_critical("fail to free mail info - err (%d)", err);
	}

	return res;
}

EMAIL_API int email_engine_check_body_download(int mail_id)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(mail_id > 0, FALSE);

	int res = 0;
	int err = 0;
	email_mail_data_t *mail_info = NULL;

	if ((err = email_get_mail_data(mail_id, &mail_info)) != EMAIL_ERROR_NONE) {
		debug_log("fail to get mail data - err (%d)", err);
		email_free_mail_data(&mail_info, 1);
		return 0;
	}

	if (mail_info == NULL) {
		debug_critical("mail_info is @niL");
		return 0;
	}

	res = mail_info->body_download_status;
	debug_log("body_download_yn: %d", res);

	err = email_free_mail_data(&mail_info, 1);

	if (err != EMAIL_ERROR_NONE) {
		debug_critical("fail to free mail info - err (%d)", err);
	}

	return res;
}

EMAIL_API gboolean email_engine_body_download(gint account_id, gint mail_id, gboolean remove_rights, int *handle)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(account_id > ACCOUNT_MIN, FALSE);
	/*
	RETURN_VAL_IF_FAIL(STR_VALID(folder_name), FALSE);
	*/
	RETURN_VAL_IF_FAIL(mail_id > 0, FALSE);

	int err = 0;
	int email_handle = 0;
	gboolean res = FALSE;

	err = email_download_body(mail_id, remove_rights, &email_handle);

	if (err != EMAIL_ERROR_NONE) {
		debug_warning("fail to download body - err (%d)", err);
		goto error;
	}

	if (handle != NULL) {
		debug_log("email_handle for body download: %d", email_handle);
		*handle = email_handle;
	}

	res = TRUE;

error:
	return res;
}

EMAIL_API gboolean email_engine_attachment_download(gint mail_id, gint index, int *handle)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(mail_id > 0, FALSE);
	RETURN_VAL_IF_FAIL(index > 0, FALSE);

	int err = 0;
	int email_handle = 0;
	gboolean res = FALSE;

	err = email_download_attachment(mail_id, index, &email_handle);

	if (err != EMAIL_ERROR_NONE) {
		debug_warning("fail to download attachment - err (%d)", err);
		goto error;
	}

	if (handle != NULL) {
		debug_log("email_handle for attachment download: %d", email_handle);
		*handle = email_handle;
	}

	res = TRUE;

error:
	return res;
}

EMAIL_API gboolean email_engine_delete_mail(int mailbox_id, gint mail_id, int sync)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(mailbox_id > 0, FALSE);
	RETURN_VAL_IF_FAIL(mail_id > 0, FALSE);

	debug_log("mailbox_id: %d", mailbox_id);
	debug_log("mail_id: %d", mail_id);
	debug_log("sync: %d", sync);

	int err = 0;
	gboolean res = TRUE;	/* MUST BE initialized TRUE. */
	int mail_ids[1] = { 0 };

	mail_ids[0] = mail_id;

	debug_log("mail_ids[0] : %d", mail_ids[0]);
	err = email_delete_mail(mailbox_id, mail_ids, 1, sync);

	if (err != EMAIL_ERROR_NONE) {
		debug_warning("failed to delete message - err (%d)", err);
		res = FALSE;
	}

	return res;
}

EMAIL_API gboolean email_engine_delete_mail_list(int mailbox_id, GList *id_list, int sync)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(mailbox_id > 0, FALSE);
	RETURN_VAL_IF_FAIL(id_list != NULL, FALSE);

	debug_log("mailbox_id: %d", mailbox_id);
	debug_log("sync: %d", sync);

	guint size = g_list_length(id_list);
	RETURN_VAL_IF_FAIL(size > 0, FALSE);
	debug_log("size (%d)", size);

	int err = 0;
	gboolean res = TRUE;	/* MUST BE initialized TRUE. */
	int mail_ids[size];
	guint i = 0;
	GList *cur = g_list_first(id_list);

	for (i = 0; i < size; ++i, cur = g_list_next(cur)) {
		mail_ids[i] = (int)(intptr_t)cur->data;
	}

	err = email_delete_mail(mailbox_id, mail_ids, (int)size, sync);

	if (err != EMAIL_ERROR_NONE) {
		debug_warning("email_delete_message mailbox_id(%d) - err (%d)", mailbox_id, err);
		res = FALSE;
	}

	return res;
}

EMAIL_API gboolean email_engine_delete_all_mail(int mailbox_id, int sync)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(mailbox_id > 0, FALSE);
	/*
	RETURN_VAL_IF_FAIL(STR_VALID(folder_name), FALSE);
	*/
	debug_log("mailbox_id: %d", mailbox_id);
	debug_log("sync: %d", sync);

	int err = 0;
	gboolean res = TRUE;	/* MUST BE initialized TRUE. */

	err = email_delete_all_mails_in_mailbox(mailbox_id, sync);

	if (err != EMAIL_ERROR_NONE) {
		debug_warning("failed to delete all message - err (%d)", err);
		res = FALSE;
	}

	return res;
}

EMAIL_API gboolean email_engine_move_mail(int mailbox_id, gint mail_id)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(mailbox_id > 0, FALSE);
	RETURN_VAL_IF_FAIL(mail_id > 0, FALSE);

	debug_log("mailbox_id: %d", mailbox_id);
	debug_log("mail_id: %d", mail_id);

	int err = 0;
	gboolean res = TRUE;	/* MUST BE initialized TRUE. */
	int mail_ids[1] = { 0 };
	mail_ids[0] = mail_id;

	debug_log("mail_ids[0] : %d", mail_ids[0]);
	err = email_move_mail_to_mailbox(mail_ids, 1, mailbox_id);

	if (err != EMAIL_ERROR_NONE) {
		debug_warning("failed to move message - err (%d)", err);
		res = FALSE;
	}

	return res;
}

EMAIL_API int email_engine_move_mail_list(int dst_mailbox_id, GList *id_list, int src_mailbox_id)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(dst_mailbox_id > 0, FALSE);
	RETURN_VAL_IF_FAIL(src_mailbox_id >= 0, FALSE);
	RETURN_VAL_IF_FAIL(id_list != NULL, FALSE);

	debug_log("dst_mailbox_id: %d", dst_mailbox_id);
	debug_log("src_mailbox_id: %d", src_mailbox_id);

	guint size = g_list_length(id_list);
	RETURN_VAL_IF_FAIL(size > 0, FALSE);
	debug_log("size (%d)", size);

	int err = 0;
	int mail_ids[size];
	int task_id = 0;

	guint i = 0;
	GList *cur = g_list_first(id_list);

	for (i = 0; i < size; ++i, cur = g_list_next(cur))
		mail_ids[i] = (int)(intptr_t)cur->data;

	if (src_mailbox_id != 0) {
		err = email_move_mails_to_mailbox_of_another_account(
				src_mailbox_id, mail_ids, (int)size, dst_mailbox_id, &task_id);
	} else {
		err = email_move_mail_to_mailbox(mail_ids, (int)size, dst_mailbox_id);
	}

	if (err != EMAIL_ERROR_NONE) {
		debug_warning("email_move_mail_to_mailbox_*** - err (%d)", err);
	}

	return err;
}

EMAIL_API gboolean email_engine_move_all_mail(int old_mailbox_id, int new_mailbox_id)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(old_mailbox_id > 0, FALSE);
	RETURN_VAL_IF_FAIL(new_mailbox_id > 0, FALSE);

	debug_log("old_mailbox_id: %d", old_mailbox_id);
	debug_log("new_mailbox_id: %d", new_mailbox_id);

	int err = 0;
	gboolean res = TRUE;	/* MUST BE initialized TRUE. */

	err = email_move_all_mails_to_mailbox(old_mailbox_id, new_mailbox_id);

	if (err != EMAIL_ERROR_NONE) {
		debug_warning("failed to move all message - err (%d)", err);
		res = FALSE;
	}

	return res;
}

EMAIL_API gint email_engine_get_unread_mail_count(gint account_id, const gchar *folder_name)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(account_id > ACCOUNT_MIN, 0);
	RETURN_VAL_IF_FAIL(STR_VALID(folder_name), 0);

	email_mailbox_t mailbox;
	int total_count = 0;
	int unread_count = 0;
	gchar *src_box = g_strdup(folder_name);

	memset(&mailbox, 0, sizeof(email_mailbox_t));
	mailbox.mailbox_name = src_box;
	mailbox.account_id = account_id;

	email_list_filter_t *filter_list = NULL;
	int filter_count = 1;

	filter_list = malloc(sizeof(email_list_filter_t)*filter_count);
	if (filter_list != NULL) {
		memset(filter_list, 0, sizeof(email_list_filter_t)*filter_count);

		filter_list[0].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
		filter_list[0].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_EQUAL;
		filter_list[0].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_MAILBOX_NAME;
		filter_list[0].list_filter_item.rule.key_value.string_type_value = src_box;
		filter_list[0].list_filter_item.rule.case_sensitivity = EMAIL_CASE_INSENSITIVE;

		email_count_mail(filter_list, filter_count, &total_count, &unread_count);

		debug_log("total_count (%d) unread_count (%d)", total_count, unread_count);

		email_free_list_filter(&filter_list, filter_count);
		filter_list = NULL;
	}

	if (src_box) {
		g_free(src_box);	/* MUST BE. */
	}

	return (gint)unread_count;
}

EMAIL_API gchar *email_engine_get_attachment_path(gint attach_id)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(attach_id > 0, FALSE);

	int err = 0;
	email_attachment_data_t *attachments = NULL;
	gchar *attachment_path = NULL;

	err = email_get_attachment_data(attach_id, &attachments);

	if (err != EMAIL_ERROR_NONE) {
		debug_critical("fail to get attachment info - err (%d)", err);
		goto error;
	}

	if (attachments == NULL) {
		debug_critical("attachments is @niL");
		goto error;
	}

	if (STR_VALID(attachments->attachment_path)) {
		debug_secure("attachment path (%s)", attachments->attachment_path);
		attachment_path = g_strdup(attachments->attachment_path);
	}

	err = email_free_attachment_data(&attachments, 1);

	if (err != EMAIL_ERROR_NONE) {
		debug_warning("fail to free attachment info - err(%d)", err);
	}

error:
	return attachment_path;
}

EMAIL_API gboolean email_engine_get_account_info(gint account_id, email_account_info_t **account_info)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(account_id > ACCOUNT_MIN, FALSE);

	(*account_info) = NULL;

	email_account_info_t *info = (email_account_info_t *) calloc(1, sizeof(email_account_info_t));

	if (info == NULL) {
		debug_critical("failed to memory allocation");
		return FALSE;
	}

	email_account_t *account = NULL;
	int err = 0;

	err = email_get_account(account_id, GET_FULL_DATA_WITHOUT_PASSWORD, &account); /* EMAIL_ACC_GET_OPT_FULL_DATA */

	if (err != EMAIL_ERROR_NONE) {
		debug_critical("failed to get account info - err (%d)", err);
		goto error;
	}

	if (STR_VALID(account->account_name)) {
		info->account_name = strdup(account->account_name);
	}

	if (STR_VALID(account->user_email_address)) {
		info->email_address = strdup(account->user_email_address);
	}

	if (STR_VALID(account->user_display_name)) {
		info->user_name = strdup(account->user_display_name);
	}

	if (STR_VALID(account->incoming_server_password)) {
		info->password = strdup(account->incoming_server_password);
	}

	if (STR_VALID(account->outgoing_server_address)) {
		info->smtp_address = strdup(account->outgoing_server_address);
	}

	if (STR_VALID(account->outgoing_server_user_name)) {
		info->smtp_user_name = strdup(account->outgoing_server_user_name);
	}

	if (STR_VALID(account->outgoing_server_password)) {
		info->smtp_password = strdup(account->outgoing_server_password);
	}

	if (STR_VALID(account->incoming_server_address)) {
		info->receiving_address = strdup(account->incoming_server_address);
	}

	info->smtp_auth = account->outgoing_server_need_authentication;
	info->smtp_port = account->outgoing_server_port_number;
	info->receiving_port = account->incoming_server_port_number;
	info->receiving_type = account->incoming_server_type;
	info->same_as = account->outgoing_server_use_same_authenticator;
	info->smtp_ssl = account->outgoing_server_secure_connection;
	info->receiving_ssl = account->incoming_server_secure_connection;
	info->download_mode = account->auto_download_size;

	err = email_free_account(&account, 1);

	if (err != EMAIL_ERROR_NONE) {
		debug_critical("failed to free account info - err (%d)", err);
	}

	(*account_info) = info;

	return TRUE;

error:
	if (info) {
		free(info);
	}
	if (account) {
		err = email_free_account(&account, 1);
		if (err != EMAIL_ERROR_NONE) {
			debug_critical("failed to free account info - err (%d)", err);
		}
	}
	return FALSE;
}

EMAIL_API gboolean email_engine_set_account_info(gint account_id, email_account_info_t *account_info)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(account_id > ACCOUNT_MIN, FALSE);
	RETURN_VAL_IF_FAIL(account_info != NULL, FALSE);

	email_account_t *account = NULL;
	email_account_t update_account;
	int err = 0;

	err = email_get_account(account_id, EMAIL_ACC_GET_OPT_DEFAULT, &account);

	if (err != EMAIL_ERROR_NONE) {
		debug_critical("failed to get account info - err (%d)", err);
		email_free_account(&account, 1);
		return FALSE;
	}

	memset(&update_account, 0, sizeof(update_account));

	/* update_account.account_bind_type = account->account_bind_type; */
	update_account.account_name = account_info->account_name;
	update_account.incoming_server_type = account_info->receiving_type;
	update_account.incoming_server_address = account_info->receiving_address;
	update_account.user_email_address = account_info->email_address;
	update_account.user_display_name = account_info->user_name;
	update_account.incoming_server_password = account_info->password;
	update_account.retrieval_mode = account->retrieval_mode;
	update_account.incoming_server_port_number = account_info->receiving_port;
	update_account.incoming_server_secure_connection = account_info->receiving_ssl;
	update_account.outgoing_server_type = account->outgoing_server_type;
	update_account.outgoing_server_address = account_info->smtp_address;
	update_account.outgoing_server_port_number = account_info->smtp_port;
	update_account.outgoing_server_need_authentication = account_info->smtp_auth;
	update_account.outgoing_server_secure_connection = account_info->smtp_ssl;
	update_account.outgoing_server_user_name = account_info->smtp_user_name;
	update_account.outgoing_server_password = account_info->smtp_password;
	update_account.user_display_name = account->user_display_name;
	update_account.reply_to_address = account_info->email_address;
	update_account.return_address = account_info->email_address;
	update_account.account_id = account->account_id;
	update_account.keep_mails_on_pop_server_after_download = account->keep_mails_on_pop_server_after_download;
	update_account.auto_download_size = account_info->download_mode;
	update_account.outgoing_server_use_same_authenticator = account_info->same_as;
	update_account.pop_before_smtp = account->pop_before_smtp;
	update_account.incoming_server_requires_apop = account->incoming_server_requires_apop;

	/* debug_log("update_account.account_bind_type (%d)", update_account.account_bind_type); */
	debug_secure("update_account.account_name (%s)", update_account.account_name ? update_account.account_name : "@niL");
	debug_secure("update_account.receiving_server_type (%d)", update_account.incoming_server_type);
	debug_secure("update_account.receiving_server_addr (%s)", update_account.incoming_server_address ? update_account.incoming_server_address : "@niL");
	debug_secure("update_account.email_addr (%s)", update_account.user_email_address ? update_account.user_email_address : "@niL");
	debug_secure("update_account.user_name (%s)", update_account.user_display_name ? update_account.user_display_name : "@niL");
	debug_secure("update_account.retrieval_mode (%d)", update_account.retrieval_mode);
	debug_secure("update_account.port_num (%d)", update_account.incoming_server_port_number);
	debug_secure("update_account.use_security (%d)", update_account.incoming_server_secure_connection);
	debug_secure("update_account.sending_server_type (%d)", update_account.outgoing_server_type);
	debug_secure("update_account.sending_server_addr (%s)", update_account.outgoing_server_address ? update_account.outgoing_server_address : "@niL");
	debug_secure("update_account.sending_port_num (%d)", update_account.outgoing_server_port_number);
	debug_secure("update_account.sending_auth (%d)", update_account.outgoing_server_need_authentication);
	debug_secure("update_account.sending_security (%d)", update_account.outgoing_server_secure_connection);
	debug_secure("update_account.sending_user (%s)", update_account.outgoing_server_user_name ? update_account.outgoing_server_user_name : "@niL");
	debug_secure("update_account.outgoing_server_password (%s)", update_account.outgoing_server_password ? update_account.outgoing_server_password : "@niL");
	debug_secure("update_account.display_name (%s)", update_account.user_display_name ? update_account.user_display_name : "@niL");
	debug_secure("update_account.reply_to_addr (%s)", update_account.reply_to_address ? update_account.reply_to_address : "@niL");
	debug_secure("update_account.return_addr (%s)", update_account.return_address ? update_account.return_address : "@niL");
	debug_secure("update_account.account_id (%d)", update_account.account_id);
	debug_secure("update_account.keep_on_server (%d)", update_account.keep_mails_on_pop_server_after_download);
	debug_secure("update_account.flag1 (%d)", update_account.auto_download_size);
	debug_secure("update_account.flag2 (%d)", update_account.outgoing_server_use_same_authenticator);
	debug_secure("update_account.pop_before_smtp (%d)", update_account.pop_before_smtp);
	debug_secure("update_account.apop (%d)", update_account.incoming_server_requires_apop);

	err = email_update_account(account_id, &update_account);

	if (err != EMAIL_ERROR_NONE) {
		debug_critical("failed to update account info - err (%d)", err);
	}

	err = email_free_account(&account, 1);

	if (err != EMAIL_ERROR_NONE) {
		debug_critical("failed to free account info - err (%d)", err);
	}

	return TRUE;
}

EMAIL_API void email_engine_free_account_info(email_account_info_t **account_info)
{
	debug_enter();
	RETURN_IF_FAIL(account_info != NULL);

	if (!*account_info) {
		return;
	}

	email_account_info_t *info = *account_info;

	if (STR_VALID(info->account_name)) {
		free(info->account_name);
		info->account_name = NULL;
	}

	if (STR_VALID(info->email_address)) {
		free(info->email_address);
		info->email_address = NULL;
	}

	if (STR_VALID(info->user_name)) {
		free(info->user_name);
		info->user_name = NULL;
	}

	if (STR_VALID(info->password)) {
		free(info->password);
		info->password = NULL;
	}

	if (STR_VALID(info->receiving_address)) {
		free(info->receiving_address);
		info->receiving_address = NULL;
	}

	if (STR_VALID(info->smtp_address)) {
		free(info->smtp_address);
		info->smtp_address = NULL;
	}

	if (STR_VALID(info->smtp_user_name)) {
		free(info->smtp_user_name);
		info->smtp_user_name = NULL;
	}

	if (STR_VALID(info->smtp_password)) {
		free(info->smtp_password);
		info->smtp_password = NULL;
	}

	free(info);
	*account_info = NULL;
}

EMAIL_API GList *email_engine_get_ca_mailbox_list_using_glist(int account_id)
{
	debug_enter();
	int i, count = 0;
	email_mailbox_t *mailbox_list = NULL;
	GList *ret = NULL;
	int err = 0;
	/*
	debug_log("account_id: %d", account_id);
	debug_log("&mailbox_list: 0x%x", &mailbox_list);
	*/

	err = email_get_mailbox_list_ex(account_id, -1, 1, &mailbox_list, &count);
	if (err != EMAIL_ERROR_NONE) {
		debug_critical("email_get_mailbox_list return error");
		goto finally;
	}
	/*
	debug_log("Executing email_get_mailbox_list is ended.");
	debug_log("count: %d", count);
	*/

	for (i = 0; i < count; i++) {
		email_mailbox_name_and_alias_t *nameandalias = calloc(1, sizeof(email_mailbox_name_and_alias_t));
		if (mailbox_list[i].mailbox_name == NULL) {
			debug_critical("mailbox_list[%d].name is null", i);
			free(nameandalias);
			continue;
		} else {
			/* debug_secure("mailbox_list[].name is %s", mailbox_list[i].mailbox_name); */
		}
		nameandalias->name = g_strdup(mailbox_list[i].mailbox_name);
		nameandalias->mailbox_id = mailbox_list[i].mailbox_id;

		if (mailbox_list[i].alias == NULL) {
			debug_critical("alias is NULL");
			nameandalias->alias = g_strdup(mailbox_list[i].mailbox_name);
		} else {
			/* debug_secure("mailbox_list[].alias is %s", mailbox_list[i].alias); */
			nameandalias->alias = g_strdup(mailbox_list[i].alias);
		}

		nameandalias->mailbox_type = mailbox_list[i].mailbox_type;
		nameandalias->unread_count = mailbox_list[i].unread_count;
		nameandalias->total_mail_count_on_local = mailbox_list[i].total_mail_count_on_local;
		nameandalias->total_mail_count_on_server = mailbox_list[i].total_mail_count_on_server;

		ret = g_list_append(ret, (gpointer)nameandalias);

		/* debug_secure("mailbox name: %s", mailbox_list[i].mailbox_name); */
	}

 finally:

	email_free_mailbox(&mailbox_list, count);
	return ret;
}

EMAIL_API void email_engine_free_ca_mailbox_list_using_glist(GList **mailbox_list)
{
	debug_enter();
	RETURN_IF_FAIL(mailbox_list != NULL);
	RETURN_IF_FAIL(*mailbox_list != NULL);

	GList *list = (GList *)(*mailbox_list);
	int list_cnt = g_list_length(list);
	int i = 0;

	for (i = 0; i < list_cnt; i++) {
		email_mailbox_name_and_alias_t *nameandalias = (email_mailbox_name_and_alias_t *) g_list_nth_data(list, i);
		if (nameandalias == NULL) {
			debug_warning("nameandalias is NULL");
		} else {
			g_free(nameandalias->name);
			g_free(nameandalias->alias);
		}
	}
	g_list_free(list);
	*mailbox_list = NULL;
}

EMAIL_API int email_engine_get_mailbox_list(int account_id, email_mailbox_t **mailbox_list)
{
	debug_enter();
	retvm_if(!mailbox_list, 0, "mailbox_list is NULL");

	int count = 0;

	*mailbox_list = NULL;

	int err = email_get_mailbox_list(account_id, EMAIL_MAILBOX_ALL, mailbox_list, &count);
	if ((err != EMAIL_ERROR_NONE) || (*mailbox_list && (count <= 0))) {
		*mailbox_list = NULL;
		debug_critical("email_get_mailbox_list return error: %d", err);
		return 0;
	}

	if (!*mailbox_list) {
		count = 0;
	}

	return count;
}

EMAIL_API int email_engine_get_mailbox_list_with_mail_count(int account_id, email_mailbox_t **mailbox_list)
{
	debug_enter();
	retvm_if(!mailbox_list, 0, "mailbox_list is NULL");

	int count = 0;

	*mailbox_list = NULL;

	int err = email_get_mailbox_list_ex(account_id, EMAIL_MAILBOX_ALL, 1, mailbox_list, &count);
	if ((err != EMAIL_ERROR_NONE) || (*mailbox_list && (count <= 0))) {
		*mailbox_list = NULL;
		debug_critical("email_get_mailbox_list_ex return error: %d", err);
		return 0;
	}

	if (!*mailbox_list) {
		count = 0;
	}

	return count;
}

EMAIL_API void email_engine_free_mailbox_list(email_mailbox_t **mailbox_list, int count)
{
	debug_enter();
	retm_if(!mailbox_list, "mailbox_list is NULL");

	if (*mailbox_list) {
		int err = email_free_mailbox(mailbox_list, count);
		if (err != EMAIL_ERROR_NONE) {
			debug_critical("email_free_mailbox return error: %d", err);
		}
	}

	*mailbox_list = NULL;
}

EMAIL_API int email_engine_get_max_account_id(void)
{
	debug_enter();

	email_account_t *account_list = NULL;
	int count = 0;
	int e = email_get_account_list(&account_list, &count);
	if (e != EMAIL_ERROR_NONE) {
		debug_critical("email_get_account_list - err(%d)", e);
		email_free_account(&account_list, count);
		return -1;
	}
	debug_secure("-- total account count : %d", count);

	int max_account_id = 0;
	int i = 0;
	for (i = 0; i < count; i++) {
		max_account_id = (account_list[i].account_id > max_account_id) ?
									account_list[i].account_id : max_account_id;
		debug_secure("%2d) %-15s %-30s\n", account_list[i].account_id, account_list[i].account_name,
										account_list[i].user_email_address);
	}
	email_free_account(&account_list, count);

	debug_leave();
	return max_account_id;
}

EMAIL_API int email_engine_get_count_account(void)
{
	debug_enter();

	email_account_t *account_list = NULL;
	int count = 0;
	int e = email_get_account_list(&account_list, &count);
	if (e != EMAIL_ERROR_NONE) {
		debug_critical("email_get_account_list - err(%d)", e);
		email_free_account(&account_list, count);
		return -1;
	}
	debug_secure("-- total account count : %d", count);

	email_free_account(&account_list, count);

	debug_leave();
	return count;
}

EMAIL_API gboolean email_engine_get_mailbox_by_mailbox_type(int account_id, email_mailbox_type_e mailbox_type, email_mailbox_t **mailbox)
{
	debug_enter();
	retvm_if(!mailbox, FALSE, "mailbox is NULL");

	*mailbox = NULL;

	int err = email_get_mailbox_by_mailbox_type(account_id, mailbox_type, mailbox);
	if ((err != EMAIL_ERROR_NONE) || !*mailbox) {
		*mailbox = NULL;
		debug_error("email_get_mailbox_by_mailbox_type failed: %d; account_id: %d; mailbox_type: %d",
				err, account_id, mailbox_type);
		return FALSE;
	}

	return TRUE;
}

EMAIL_API gboolean email_engine_get_mailbox_by_mailbox_id(int mailbox_id, email_mailbox_t **mailbox)
{
	debug_enter();
	retvm_if(!mailbox, FALSE, "mailbox is NULL");

	*mailbox = NULL;

	int err = email_get_mailbox_by_mailbox_id(mailbox_id, mailbox);
	if ((err != EMAIL_ERROR_NONE) || !*mailbox) {
		*mailbox = NULL;
		debug_error("email_get_mailbox_by_mailbox_id failed: %d; mailbox_id: %d", err, mailbox_id);
		return FALSE;
	}

	return TRUE;
}

EMAIL_API int email_engine_delete_mailbox(int mailbox_id, int on_server, int *handle)
{
	debug_enter();

	int err = email_delete_mailbox(mailbox_id, on_server, handle);
	if (err != EMAIL_ERROR_NONE) {
		debug_error("email_delete_mailbox failed: %d; mailbox_id: %d; on_server: %d",
				err, mailbox_id, on_server);
	}

	return err;
}

EMAIL_API int email_engine_rename_mailbox(int mailbox_id, char *mailbox_name, char *mailbox_alias, int on_server, int *handle)
{
	debug_enter();

	int err = email_rename_mailbox(mailbox_id, mailbox_name, mailbox_alias, on_server, handle);
	if (err != EMAIL_ERROR_NONE) {
		debug_error("email_rename_mailbox failed: %d; "
				"mailbox_name: %s; mailbox_alias: %s; mailbox_id: %d; on_server: %d",
				err, mailbox_name, mailbox_alias, mailbox_id, on_server);
	}

	return err;
}

EMAIL_API int email_engine_add_mailbox(email_mailbox_t *new_mailbox, int on_server, int *handle)
{
	debug_enter();

	int err = email_add_mailbox(new_mailbox, on_server, handle);
	if (err != EMAIL_ERROR_NONE) {
		debug_error("email_engine_add_mailbox failed: %d; on_server: %d", err, on_server);
	}

	return err;
}

EMAIL_API int email_engine_get_task_information(email_task_information_t **task_information, int *task_information_count)
{
	debug_enter();

	int err = email_get_task_information(task_information, task_information_count);
	if (err != EMAIL_ERROR_NONE) {
		debug_error("email_get_task_information failed: %d", err);
	}

	return err;
}

EMAIL_API gboolean email_engine_get_rule_list(email_rule_t **filtering_set, int *count)
{
	debug_enter();
	retvm_if(!filtering_set, FALSE, "filtering_set is NULL");
	retvm_if(!count, FALSE, "count is NULL");

	*filtering_set = NULL;
	*count = 0;

	int err = email_get_rule_list(filtering_set, count);
	if ((err != EMAIL_ERROR_NONE) || (*filtering_set && (*count <= 0))) {
		debug_error("email_get_rule_list failed: %d", err);
		*filtering_set = NULL;
		*count = 0;
		return FALSE;
	}

	if (!*filtering_set) {
		count = 0;
	}

	return TRUE;
}

EMAIL_API gboolean email_engine_get_rule(int filter_id, email_rule_t** filtering_set)
{
	debug_enter();
	retvm_if(!filtering_set, FALSE, "filtering_set is NULL");

	*filtering_set = NULL;

	int err = email_get_rule(filter_id, filtering_set);
	if ((err != EMAIL_ERROR_NONE) || !*filtering_set) {
		debug_error("email_get_rule failed: %d; filter_id: %d", err, filter_id);
		*filtering_set = NULL;
		return FALSE;
	}

	return TRUE;
}

EMAIL_API gboolean email_engine_add_rule(email_rule_t *filtering_set)
{
	debug_enter();

	int err = email_add_rule(filtering_set);
	if (err != EMAIL_ERROR_NONE) {
		debug_error("email_add_rule failed: %d", err);
		return FALSE;
	}

	return TRUE;
}

EMAIL_API gboolean email_engine_update_rule(int filter_id, email_rule_t *new_set)
{
	debug_enter();

	int err = email_update_rule(filter_id, new_set);
	if (err != EMAIL_ERROR_NONE) {
		debug_error("email_update_rule failed: %d; filter_id: %d", err, filter_id);
		return FALSE;
	}

	return TRUE;
}

EMAIL_API gboolean email_engine_delete_rule(int filter_id)
{
	debug_enter();

	int err = email_delete_rule(filter_id);
	if (err != EMAIL_ERROR_NONE) {
		debug_error("email_delete_rule failed: %d; filter_id: %d", err, filter_id);
		return FALSE;
	}

	return TRUE;
}

EMAIL_API gboolean email_engine_apply_rule(int filter_id)
{
	debug_enter();

	int err = email_apply_rule(filter_id);
	if (err != EMAIL_ERROR_NONE) {
		debug_error("email_apply_rule failed: %d; filter_id: %d", err, filter_id);
		return FALSE;
	}

	return TRUE;
}

EMAIL_API void email_engine_free_rule_list(email_rule_t **filtering_set, int count)
{
	debug_enter();
	retm_if(!filtering_set, "filtering_set is NULL");

	if (*filtering_set) {
		int err = email_free_rule(filtering_set, count);
		if (err != EMAIL_ERROR_NONE) {
			debug_error("email_free_rule failed: %d", err);
		}
	}

	*filtering_set = NULL;
}

EMAIL_API gboolean email_engine_get_password_length_of_account(int account_id,
		email_get_password_length_type password_type, int *password_length)
{
	debug_enter();

	int err = email_get_password_length_of_account(account_id, password_type, password_length);
	if (err != EMAIL_ERROR_NONE) {
		*password_length = 0;
		debug_error("email_get_password_length_of_account failed: %d; "
				"account_id: %d; password_type: %d", err, account_id, (int)password_type);
		return FALSE;
	}

	return TRUE;
}

EMAIL_API gboolean email_engine_get_mail_data(int mail_id, email_mail_data_t **mail_data)
{
	debug_enter();
	retvm_if(!mail_data, FALSE, "mail_data is NULL");

	*mail_data = NULL;

	int err = email_get_mail_data(mail_id, mail_data);
	if ((err != EMAIL_ERROR_NONE) || !*mail_data) {
		*mail_data = NULL;
		debug_error("email_get_mail_data failed: %d; mail_id: %d", err, mail_id);
		return FALSE;
	}

	return TRUE;
}

EMAIL_API gboolean email_engine_get_attachment_data_list(int mail_id,
		email_attachment_data_t **attachment_data, int *attachment_count)
{
	debug_enter();
	retvm_if(!attachment_data, FALSE, "attachment_data is NULL");
	retvm_if(!attachment_count, FALSE, "attachment_count is NULL");

	*attachment_data = NULL;
	*attachment_count = 0;

	int err = email_get_attachment_data_list(mail_id, attachment_data, attachment_count);
	if ((err != EMAIL_ERROR_NONE) || (*attachment_data && (*attachment_count <= 0))) {
		*attachment_data = NULL;
		*attachment_count = 0;
		debug_error("email_get_attachment_data_list failed: %d; mail_id: %d", err, mail_id);
		return FALSE;
	}

	if (!*attachment_data) {
		*attachment_count = 0;
	}

	return TRUE;
}

EMAIL_API gboolean email_engine_parse_mime_file(char *eml_file_path, email_mail_data_t **mail_data,
		email_attachment_data_t **attachment_data, int *attachment_count)
{
	debug_enter();
	retvm_if(!mail_data, FALSE, "mail_data is NULL");
	retvm_if(!attachment_data, FALSE, "attachment_data is NULL");
	retvm_if(!attachment_count, FALSE, "attachment_count is NULL");

	*mail_data = NULL;
	*attachment_data = NULL;
	*attachment_count = 0;

	int err = email_parse_mime_file(eml_file_path, mail_data, attachment_data, attachment_count);
	if ((err != EMAIL_ERROR_NONE) || !*mail_data || (*attachment_data && (*attachment_count <= 0))) {
		*mail_data = NULL;
		*attachment_data = NULL;
		*attachment_count = 0;
		debug_error("email_parse_mime_file failed: %d; eml_file_path: %s", err, eml_file_path);
		return FALSE;
	}

	if (!*attachment_data) {
		*attachment_count = 0;
	}

	return TRUE;
}

EMAIL_API void email_engine_free_mail_data_list(email_mail_data_t** mail_list, int count)
{
	debug_enter();
	retm_if(!mail_list, "mail_list is NULL");

	if (*mail_list) {
		int err = email_free_mail_data(mail_list, count);
		if (err != EMAIL_ERROR_NONE) {
			debug_error("email_free_mail_data_list failed: %d", err);
		}
	}

	*mail_list = NULL;
}

EMAIL_API void email_engine_free_attachment_data_list(
		email_attachment_data_t **attachment_data_list, int attachment_count)
{
	debug_enter();
	retm_if(!attachment_data_list, "attachment_data_list is NULL");

	if (*attachment_data_list) {
		int err = email_free_attachment_data(attachment_data_list, attachment_count);
		if (err != EMAIL_ERROR_NONE) {
			debug_error("email_free_attachment_data failed: %d", err);
		}
	}

	*attachment_data_list = NULL;
}

EMAIL_API int email_engine_add_mail(email_mail_data_t *mail_data, email_attachment_data_t *attachment_data_list,
		int attachment_count, email_meeting_request_t* meeting_request, int from_eas)
{
	debug_enter();

	int err = email_add_mail(mail_data, attachment_data_list, attachment_count, meeting_request, from_eas);
	if (err != EMAIL_ERROR_NONE) {
		debug_error("email_add_mail failed: %d; from_eas: %d", err, from_eas);
	}

	return err;
}

EMAIL_API gboolean email_engine_send_mail(int mail_id, int *handle)
{
	debug_enter();

	int err = email_send_mail(mail_id, handle);
	if (err != EMAIL_ERROR_NONE) {
		*handle = 0;
		debug_error("email_send_mail failed: %d; mail_id: %d", err, mail_id);
		return FALSE;
	}

	return TRUE;
}

EMAIL_API gboolean email_engine_send_mail_with_downloading_attachment_of_original_mail(int mail_id, int *handle)
{
	debug_enter();

	int err = email_send_mail_with_downloading_attachment_of_original_mail(mail_id, handle);
	if (err != EMAIL_ERROR_NONE) {
		*handle = 0;
		debug_error("email_send_mail_with_downloading_attachment_of_original_mail failed: %d; mail_id: %d", err, mail_id);
		return FALSE;
	}

	return TRUE;
}

EMAIL_API gboolean email_engine_cancel_sending_mail(int mail_id)
{
	debug_enter();

	int err = email_cancel_sending_mail(mail_id);
	if (err != EMAIL_ERROR_NONE) {
		debug_error("email_cancel_sending_mail failed: %d; mail_id: %d", err, mail_id);
		return FALSE;
	}

	return TRUE;
}

EMAIL_API gboolean email_engine_set_flags_field(int account_id, int *mail_ids, int num,
		email_flags_field_type field_type, int value, int onserver)
{
	debug_enter();

	int err = email_set_flags_field(account_id, mail_ids, num, field_type, value, onserver);
	if (err != EMAIL_ERROR_NONE) {
		debug_error("email_set_flags_field failed: %d; account_id: %d; field_type: %d; "
				"value: %d; onserver: %d", err, account_id, (int)field_type, value, onserver);
		return FALSE;
	}

	return TRUE;
}

EMAIL_API gboolean email_engine_update_mail_attribute(int account_id, int *mail_ids, int num,
		email_mail_attribute_type attribute_type, email_mail_attribute_value_t value)
{
	debug_enter();

	int err = email_update_mail_attribute(account_id, mail_ids, num, attribute_type, value);
	if (err != EMAIL_ERROR_NONE) {
		debug_error("email_update_mail_attribute failed: %d; account_id: %d; field_type: %d; "
				"value: %d", err, account_id, (int)attribute_type, value);
		return FALSE;
	}

	return TRUE;
}

EMAIL_API void email_engine_free_list_filter(email_list_filter_t **filter_list, int count)
{
	debug_enter();
	retm_if(!filter_list, "filter_list is NULL");

	if (*filter_list) {
		int err = email_free_list_filter(filter_list, count);
		if (err != EMAIL_ERROR_NONE) {
			debug_error("email_free_list_filter failed: %d", err);
		}
	}

	*filter_list = NULL;
}

EMAIL_API gboolean email_engine_get_address_info_list(int mail_id, email_address_info_list_t** address_info_list)
{
	debug_enter();
	retvm_if(!address_info_list, FALSE, "address_info_list is NULL");

	*address_info_list = NULL;

	int err = email_get_address_info_list(mail_id, address_info_list);
	if ((err != EMAIL_ERROR_NONE) || !*address_info_list) {
		*address_info_list = NULL;
		debug_error("email_get_address_info_list failed: %d; mail_id: %d", err, mail_id);
		return FALSE;
	}

	return TRUE;
}

EMAIL_API void email_engine_free_address_info_list(email_address_info_list_t** address_info_list)
{
	debug_enter();
	retm_if(!address_info_list, "address_info_list is NULL");

	if (*address_info_list) {
		int err = email_free_address_info_list(address_info_list);
		if (err != EMAIL_ERROR_NONE) {
			debug_error("email_free_address_info_list failed: %d", err);
		}
	}

	*address_info_list = NULL;
}

EMAIL_API gboolean email_engine_set_mail_slot_size(int account_id, int mailbox_id, int new_slot_size)
{
	debug_enter();

	int err = email_set_mail_slot_size(account_id, mailbox_id, new_slot_size);
	if (err != EMAIL_ERROR_NONE) {
		debug_error("email_set_mail_slot_size failed: %d; account_id: %d; "
				"mailbox_id: %d; new_slot_size: %d", err, account_id, mailbox_id, new_slot_size);
		return FALSE;
	}

	return TRUE;
}

EMAIL_API gboolean email_engine_update_notification_bar(int account_id)
{
	debug_enter();

	int err = email_update_notification_bar(account_id, 0, 0, 0);
	if (err != EMAIL_ERROR_NONE) {
		debug_error("email_update_notification_bar failed: %d; account_id: %d", err, account_id);
		return FALSE;
	}

	return TRUE;
}

EMAIL_API gboolean email_engine_clear_notification_bar(int account_id)
{
	debug_enter();

	int err = email_clear_notification_bar(account_id);
	if (err != EMAIL_ERROR_NONE) {
		debug_error("email_clear_notification_bar failed: %d; account_id: %d", err, account_id);
		return FALSE;
	}

	return TRUE;
}

EMAIL_API gboolean email_engine_open_db(void)
{
	debug_enter();

	int err = email_open_db();
	if (err != EMAIL_ERROR_NONE) {
		debug_error("email_open_db failed: %d", err);
		return FALSE;
	}

	return TRUE;
}

EMAIL_API gboolean email_engine_close_db(void)
{
	debug_enter();

	int err = email_close_db();
	if (err != EMAIL_ERROR_NONE) {
		debug_error("email_close_db failed: %d", err);
		return FALSE;
	}

	return TRUE;
}

EMAIL_API gboolean email_engine_cancel_job(int account_id, int handle, email_cancelation_type cancel_type)
{
	debug_enter();

	int err = email_cancel_job(account_id, handle, cancel_type);
	if (err != EMAIL_ERROR_NONE) {
		debug_error("email_cancel_job failed: %d; account_id: %d; cancel_type: %d",
				err, account_id, (int)cancel_type);
		return FALSE;
	}

	return TRUE;
}

/* EOF */
