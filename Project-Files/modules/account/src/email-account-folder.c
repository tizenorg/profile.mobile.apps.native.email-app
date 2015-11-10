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

#include "email-account-folder.h"
#include "email-account-folder-move.h"
#include "email-account-util.h"

#define EMAIL_SING_ACC_MANDAT_FOLDER_COUNT 8

static void _finish_folder_view(void *data, Evas_Object *obj, void *event_info);
static void _update_mail_count_on_folder_view(EmailAccountUGD *ug_data, int account_id);

static char *_gl_account_item_text_get(void *data, Evas_Object *obj, const char *part);
static void _gl_account_item_del(void *data, Evas_Object *obj);

static char *_gl_label_get_for_all_acc_box(void *data, Evas_Object *obj, const char *part);
static void _gl_sel_single(void *data, Evas_Object *obj, void *event_info);
static char *_gl_label_get_for_single_subitem(void *data, Evas_Object *obj, const char *part);

static char *_gl_group_text_get(void *data, Evas_Object *obj, const char *part);
static void _gl_grouptitle_del(void *data, Evas_Object *obj);
static void _gl_del(void *data, Evas_Object *obj);
static void _open_allacc_boxtype(void *data, Evas_Object *obj, void *event_info);
static void _popup_newfolder_cb(void *data, Evas_Object *obj, void *event_info);
static void _create_folder_popup_cancel_cb(void *data, Evas_Object *obj, void *event_info);
static void _create_folder_ok_cb(void *data, Evas_Object *obj, void *event_info);

static void _popup_delfolder_cb(void *data, Evas_Object *obj, void *event_info);
static void _delete_con_cb(void *data, Evas_Object *obj, void *event_info);

static void _popup_renamefolder_cb(void *data, Evas_Object *obj, void *event_info);
static void _rename_folder_cancel_cb(void *data, Evas_Object *obj, void *event_info);
static void _rename_folder_ok_cb(void *data, Evas_Object *obj, void *event_info);

static void _popup_destroy_cb(void *data, Evas_Object *obj, void *event_info);
static void _back_button_cb(void *data, Evas_Object *obj, void *event_info);
static void _popup_success_cb(void *data, Evas_Object *obj, void *event_info);
static void _popup_fail_cb(void *data, Evas_Object *obj, void *event_info, int err_code);
static void _popup_progress_cb(void *data, Evas_Object *obj, void *event_info);

static void _update_folder_view_after_folder_action(void *data);
static void _update_folder_list_after_folder_delete_action(void *data, int action_type, int mailbox_id, bool show_success_popup);
static void _update_folder_list_after_folder_action(void *data, int action_type, int mailbox_id, bool show_success_popup);

static void _mouseup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _folder_popup_mouseup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void _add_root_item_in_genlist(void *data);
static void _gl_root_item_del(void *data, Evas_Object *obj);
static char *_gl_label_get_for_root_item(void *data, Evas_Object *obj, const char *part);
static void _gl_root_item_sel(void *data, Evas_Object *obj, void *event_info);
static bool _check_folder_name_exists(EmailAccountUGD *ug_data, const char *mailbox_name);
/* If true returned, *out_mailbox_name and *out_mailbox_alias has to be freed manually via free() */
static bool _generate_email_name_and_alias(EmailAccountUGD *ug_data, char **out_mailbox_name, char **out_mailbox_alias);

static void _add_account_name_item_to_list(EmailAccountUGD *ug_data, char *account_name);
static Eina_Bool _try_add_group_item_to_list(EmailAccountUGD *ug_data);
static int _add_custom_folders_to_list(EmailAccountUGD *ug_data, int mailbox_type);
static int _add_default_folders_to_list(EmailAccountUGD *ug_data, int mailbox_type, email_mailbox_t *mailbox_list, int mailbox_list_count);
static char *_create_gl_item_text(Item_Data *item_data, const char *text);
static int _convert_acc_boxtype_to_email_boxtype(int boxtype);

void account_init_genlist_item_class_for_combined_folder_list(EmailAccountUGD *ug_data)
{
	debug_enter();
	RETURN_IF_FAIL(ug_data != NULL);

	Elm_Genlist_Item_Class *itc_combined = elm_genlist_item_class_new();
	retm_if(itc_combined == NULL, "itc_combined is NULL!");
	itc_combined->item_style = "type1";
	itc_combined->func.text_get = _gl_label_get_for_all_acc_box;
	itc_combined->func.content_get = NULL;
	itc_combined->func.state_get = NULL;
	itc_combined->func.del = _gl_del;
	ug_data->itc_combined = itc_combined;
}

void account_init_genlist_item_class_for_single_folder_list(EmailAccountUGD *ug_data)
{
	debug_enter();

	RETURN_IF_FAIL(ug_data != NULL);

	Elm_Genlist_Item_Class *itc_account_name = elm_genlist_item_class_new();
	retm_if(!itc_account_name, "itc_account_name is NULL!");
	itc_account_name->item_style = "group_index";
	itc_account_name->func.text_get = _gl_account_item_text_get;
	itc_account_name->func.content_get = NULL;
	itc_account_name->func.state_get = NULL;
	itc_account_name->func.del = _gl_account_item_del;
	ug_data->itc_account_name = itc_account_name;

	Elm_Genlist_Item_Class *itc_single = elm_genlist_item_class_new();
	if (!itc_single){
		debug_error("itc_single is NULL - allocation memory failed");
		EMAIL_GENLIST_ITC_FREE(ug_data->itc_account_name);
		return;
	}
	itc_single->item_style = "type1";
	itc_single->func.text_get = _gl_label_get_for_single_subitem;
	itc_single->func.content_get = NULL;
	itc_single->func.state_get = NULL;
	itc_single->func.del = _gl_del;
	ug_data->itc_single = itc_single;

	Elm_Genlist_Item_Class *itc_group = elm_genlist_item_class_new();
	if (!itc_group){
		debug_error("itc_group is NULL - allocation memory failed");
		EMAIL_GENLIST_ITC_FREE(ug_data->itc_account_name);
		EMAIL_GENLIST_ITC_FREE(ug_data->itc_single);
		return;
	}
	itc_group->item_style = "group_index";
	itc_group->func.text_get = _gl_group_text_get;
	itc_group->func.content_get = NULL;
	itc_group->func.state_get = NULL;
	itc_group->func.del = _gl_grouptitle_del;
	ug_data->itc_group = itc_group;

	Elm_Genlist_Item_Class *itc_root = elm_genlist_item_class_new();
	if (!itc_root){
		debug_error("itc_root is NULL - allocation memory failed");
		EMAIL_GENLIST_ITC_FREE(ug_data->itc_account_name);
		EMAIL_GENLIST_ITC_FREE(ug_data->itc_single);
		EMAIL_GENLIST_ITC_FREE(ug_data->itc_group);
		return;
	}
	itc_root->item_style = "type1";
	itc_root->func.text_get = _gl_label_get_for_root_item;
	itc_root->func.content_get = NULL;
	itc_root->func.state_get = NULL;
	itc_root->func.del = _gl_root_item_del;
	ug_data->itc_root = itc_root;

}

int account_create_combined_folder_list(EmailAccountUGD *ug_data)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(ug_data != NULL, 0);

	Evas_Object *gl = ug_data->gl;
	Item_Data *tree_item_data = NULL;
	int inserted_cnt = 0;
	int i = 0;
	int total_count = 0, unread_count = 0;

	memset((ug_data->all_acc_unread_count), 0, sizeof(ug_data->all_acc_unread_count));
	memset((ug_data->all_acc_total_count), 0, sizeof(ug_data->all_acc_unread_count));

	int index = ACC_MAILBOX_TYPE_INBOX;
	for (index = ACC_MAILBOX_TYPE_INBOX; index < ACC_MAILBOX_TYPE_MAX; index++) {
		switch (index) {
		case ACC_MAILBOX_TYPE_INBOX:
		{
			if (email_get_combined_mail_count_by_mailbox_type(EMAIL_MAILBOX_TYPE_INBOX, &total_count, &unread_count)) {
				ug_data->all_acc_total_count[ACC_MAILBOX_TYPE_INBOX] = total_count;
				ug_data->all_acc_unread_count[ACC_MAILBOX_TYPE_INBOX] = unread_count;
			}
		}
		break;

		case ACC_MAILBOX_TYPE_PRIORITY_INBOX:
		{
			if (email_get_priority_sender_mail_count_ex(&total_count, &unread_count)) {
				ug_data->all_acc_total_count[ACC_MAILBOX_TYPE_PRIORITY_INBOX] = total_count;
				ug_data->all_acc_unread_count[ACC_MAILBOX_TYPE_PRIORITY_INBOX] = unread_count;
			}
		}
		break;

		case ACC_MAILBOX_TYPE_FLAGGED:
		{
			if (email_get_favourite_mail_count_ex(&total_count, &unread_count)) {
				ug_data->all_acc_total_count[ACC_MAILBOX_TYPE_FLAGGED] = total_count;
				ug_data->all_acc_unread_count[ACC_MAILBOX_TYPE_FLAGGED] = unread_count;
			}
		}
		break;
		case ACC_MAILBOX_TYPE_DRAFT:
		{
			if (email_get_combined_mail_count_by_mailbox_type(EMAIL_MAILBOX_TYPE_DRAFT, &total_count, &unread_count)) {
				ug_data->all_acc_total_count[ACC_MAILBOX_TYPE_DRAFT] = total_count;
				ug_data->all_acc_unread_count[ACC_MAILBOX_TYPE_DRAFT] = unread_count;
			}
		}
		break;
		case ACC_MAILBOX_TYPE_OUTBOX:
		{
			if (email_get_combined_mail_count_by_mailbox_type(EMAIL_MAILBOX_TYPE_OUTBOX, &total_count, &unread_count)) {
				ug_data->all_acc_total_count[ACC_MAILBOX_TYPE_OUTBOX] = total_count;
				ug_data->all_acc_unread_count[ACC_MAILBOX_TYPE_OUTBOX] = unread_count;
			}
		}
		break;
		case ACC_MAILBOX_TYPE_SENTBOX:
		{
			if (email_get_combined_mail_count_by_mailbox_type(EMAIL_MAILBOX_TYPE_SENTBOX, &total_count, &unread_count)) {
				ug_data->all_acc_total_count[ACC_MAILBOX_TYPE_SENTBOX] = total_count;
				ug_data->all_acc_unread_count[ACC_MAILBOX_TYPE_SENTBOX] = unread_count;
			}
		}
		break;
		case ACC_MAILBOX_TYPE_SPAMBOX:
		{
			if (email_get_combined_mail_count_by_mailbox_type(EMAIL_MAILBOX_TYPE_SPAMBOX, &total_count, &unread_count)) {
				ug_data->all_acc_total_count[ACC_MAILBOX_TYPE_SPAMBOX] = total_count;
				ug_data->all_acc_unread_count[ACC_MAILBOX_TYPE_SPAMBOX] = unread_count;
			}
		}
		break;
		case ACC_MAILBOX_TYPE_TRASH:
		{
			if (email_get_combined_mail_count_by_mailbox_type(EMAIL_MAILBOX_TYPE_TRASH, &total_count, &unread_count)) {
				ug_data->all_acc_total_count[ACC_MAILBOX_TYPE_TRASH] = total_count;
				ug_data->all_acc_unread_count[ACC_MAILBOX_TYPE_TRASH] = unread_count;
			}
		}
		break;

		default:
			break;
		}
	}

	for (i = ACC_MAILBOX_TYPE_INBOX; i < ACC_MAILBOX_TYPE_MAX; i++) {
		if (i <= ACC_MAILBOX_TYPE_TRASH) {
			tree_item_data = calloc(1, sizeof(Item_Data));
			if (!tree_item_data) {
				debug_error("tree_item_data is NULL - allocation memory failed");
				return inserted_cnt;
			}
			tree_item_data->ug_data = ug_data;
			tree_item_data->i_boxtype = i;
			tree_item_data->mailbox_type = _convert_acc_boxtype_to_email_boxtype(i);
			tree_item_data->it = elm_genlist_item_append(gl, ug_data->itc_combined, tree_item_data, NULL,
					ELM_GENLIST_ITEM_NONE, _open_allacc_boxtype, (void *)(ptrdiff_t)tree_item_data->mailbox_type);
		}
		inserted_cnt++;
	}

	debug_leave();

	return inserted_cnt;
}

static void _gl_sel_single(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	if (!data) {
		debug_log("data is NULL");
		return;
	}

	Elm_Object_Item *it = event_info;
	elm_genlist_item_selected_set(it, EINA_FALSE);

	Item_Data *tree_item_data = (Item_Data *)data;
	EmailAccountUGD *ug_data = tree_item_data->ug_data;

	if (ACC_FOLDER_CREATE == ug_data->folder_mode) {
		ug_data->it = it;
		_popup_newfolder_cb(ug_data, obj, event_info);

	} else if (ACC_FOLDER_DELETE == ug_data->folder_mode) {
		ug_data->it = it;
		_popup_delfolder_cb(ug_data, obj, event_info);

	} else if (ACC_FOLDER_RENAME == ug_data->folder_mode) {
		ug_data->it = it;
		_popup_renamefolder_cb(ug_data, obj, event_info);

	} else if (ACC_FOLDER_NONE == ug_data->folder_mode) {
		app_control_h service;
		if (APP_CONTROL_ERROR_NONE != app_control_create(&service)) {
			debug_log("creating service handle failed");
			return;
		}

		char id[NUM_STR_LEN] = { 0 };
		snprintf(id, sizeof(id), "%d", ug_data->account_id);

		char is_mail_move_ug[NUM_STR_LEN] = { 0 };
		snprintf(is_mail_move_ug, sizeof(is_mail_move_ug), "%d", 0);

		app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_IS_MAILBOX_MOVE_UG, is_mail_move_ug);

		if (tree_item_data->b_scheduled_outbox > 0) {
			app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_ACCOUNT_ID, id);
			app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_ACCOUNT_TYPE, EMAIL_BUNDLE_VAL_SCHEDULED_OUTBOX);
			debug_log("scheduled outbox is selected");
		} else {

			int i_mailbox_id = tree_item_data->mailbox_id;
			int mb_type = tree_item_data->mailbox_type;
			if (tree_item_data->mailbox_type == EMAIL_MAILBOX_TYPE_PRIORITY_SENDERS) {
				mb_type = EMAIL_MAILBOX_TYPE_INBOX;
				i_mailbox_id = 0;
			} else if (tree_item_data->mailbox_type == EMAIL_MAILBOX_TYPE_FLAGGED) {
				mb_type = tree_item_data->mailbox_type;
				i_mailbox_id = 0;
			}

			char mailbox_id[NUM_STR_LEN] = { 0 };
			snprintf(mailbox_id, sizeof(mailbox_id), "%d", i_mailbox_id);

			char mailbox_type[NUM_STR_LEN] = { 0 };
			snprintf(mailbox_type, sizeof(mailbox_type), "%d", mb_type);

			if (tree_item_data->mailbox_type == EMAIL_MAILBOX_TYPE_FLAGGED) {
				app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_ACCOUNT_TYPE, EMAIL_BUNDLE_VAL_ALL_ACCOUNT);
			} else if (tree_item_data->mailbox_type == EMAIL_MAILBOX_TYPE_PRIORITY_SENDERS) {
				app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_ACCOUNT_TYPE, EMAIL_BUNDLE_VAL_PRIORITY_SENDER);
			} else {
				app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_ACCOUNT_TYPE, EMAIL_BUNDLE_VAL_SINGLE_ACCOUNT);
			}

			app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_ACCOUNT_ID, id);
			app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_MAILBOX, mailbox_id);
			app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_MAILBOX_TYPE, mailbox_type);
			app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_IS_MAILBOX_MOVE_UG, is_mail_move_ug);
		}

		email_module_send_result(ug_data->base.module, service);

		app_control_destroy(service);
	}
	debug_leave();
}

static bool _check_folder_name_exists(EmailAccountUGD *ug_data, const char *mailbox_name)
{
	retv_if(!ug_data, false);
	retv_if(!mailbox_name, false);
	Item_Data *tree_item_data;
	Elm_Object_Item *gl_item = elm_genlist_first_item_get(ug_data->gl);
	while (gl_item) {
		tree_item_data = elm_object_item_data_get(gl_item);
		if (tree_item_data && tree_item_data->mailbox_type == EMAIL_MAILBOX_TYPE_USER_DEFINED
				&& tree_item_data->mailbox_name && !strcmp(mailbox_name, tree_item_data->mailbox_name)) {
			return true;
		}
		gl_item = elm_genlist_item_next_get(gl_item);
	}
	return false;
}

static bool _generate_email_name_and_alias(EmailAccountUGD *ug_data, char **out_mailbox_name, char **out_mailbox_alias)
{
	debug_enter();
	*out_mailbox_name = NULL;
	*out_mailbox_alias = NULL;

	Elm_Object_Item *it = ug_data->it;
	retv_if(!it, false);
	Item_Data *tree_item_data = elm_object_item_data_get(it);
	retv_if(!tree_item_data, false);

	email_mailbox_t *mlist = (email_mailbox_t *)tree_item_data->mailbox_list;
	retv_if(!mlist && (ug_data->it != ug_data->root_item), false);

	retv_if(!elm_object_text_get(ug_data->entry), false);
	char *mailbox_alias = elm_entry_markup_to_utf8(elm_object_text_get(ug_data->entry));
	retv_if(!mailbox_alias, false);
	email_util_strrtrim(mailbox_alias);

	debug_log("root folder is selected");

	*out_mailbox_name = strdup(mailbox_alias);
	*out_mailbox_alias = mailbox_alias;

	debug_leave();
	return true;
}

static void _add_account_name_item_to_list(EmailAccountUGD *ug_data, char *account_name)
{
	debug_enter();

	if (account_name == NULL) {
		debug_error("add account name item failed. account name is empty");
		return;
	}

	Item_Data *tree_item_data = calloc(1, sizeof(Item_Data));
	retm_if(!tree_item_data, "tree_item_data is NULL!");
	tree_item_data->account_name = strdup(account_name);
	tree_item_data->it = elm_genlist_item_append(ug_data->gl, ug_data->itc_account_name, tree_item_data, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
	elm_genlist_item_select_mode_set(tree_item_data->it, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
}

static Eina_Bool _try_add_group_item_to_list(EmailAccountUGD *ug_data)
{
	debug_enter();

	Item_Data *tree_item_data = NULL;
	email_mailbox_t *mailbox = NULL;

	int res = email_get_mailbox_by_mailbox_type(ug_data->account_id, EMAIL_MAILBOX_TYPE_USER_DEFINED, &mailbox);
	if (res == EMAIL_ERROR_NONE && mailbox != NULL) {
		tree_item_data = calloc(1, sizeof(Item_Data));
		if (!tree_item_data) {
			email_free_mailbox(&mailbox, 1);
			debug_error("sorting_rule_list is NULL - allocation memory failed");
			return EINA_FALSE;
		}
		tree_item_data->ug_data = ug_data;

		tree_item_data->it = elm_genlist_item_append(ug_data->gl, ug_data->itc_group, tree_item_data, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
		tree_item_data->ug_data->group_title_item = tree_item_data->it;
		elm_genlist_item_select_mode_set(tree_item_data->it, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
		email_free_mailbox(&mailbox, 1);
		return EINA_TRUE;
	}
	email_free_mailbox(&mailbox, 1);

	return EINA_FALSE;
}

static int _add_custom_folders_to_list(EmailAccountUGD *ug_data, int mailbox_type)
{
	debug_enter();

	int total_count = 0;
	int unread_count = 0;
	char tmp[MAX_STR_LEN] = { 0 };

	switch (mailbox_type) {
	case EMAIL_MAILBOX_TYPE_PRIORITY_SENDERS:
		if (email_get_priority_sender_mail_count_ex(&total_count, &unread_count) == 0) {
			debug_warning("get priority sender mail count failed");
		}
		snprintf(tmp, sizeof(tmp), "%s", _("IDS_EMAIL_HEADER_PRIORITY_SENDERS_ABB"));
		break;

	case EMAIL_MAILBOX_TYPE_FLAGGED:
		if (email_get_favourite_mail_count_ex(&total_count, &unread_count) == 0) {
			debug_warning("get favourite mail count failed");
		}
		snprintf(tmp, sizeof(tmp), "%s", _("IDS_EMAIL_HEADER_STARRED_EMAILS_ABB"));
		break;

	default:
		debug_error("Undefined account mailbox type");
		break;
	}

	Item_Data *tree_item_data = calloc(1, sizeof(Item_Data));
	retvm_if(!tree_item_data, 0, "tree_item_data is NULL!");
	tree_item_data->ug_data = ug_data;
	tree_item_data->mailbox_type = mailbox_type;
	tree_item_data->mailbox_name = strdup(tmp);
	tree_item_data->alias_name = strdup(tmp);
	tree_item_data->unread_count = unread_count;
	tree_item_data->total_mail_count_on_local = total_count;

	tree_item_data->it = elm_genlist_item_append(ug_data->gl, ug_data->itc_single, tree_item_data, NULL, ELM_GENLIST_ITEM_NONE, _gl_sel_single, tree_item_data);

	if (tree_item_data->ug_data->folder_mode != ACC_FOLDER_NONE) {
		elm_object_item_disabled_set(tree_item_data->it, EINA_TRUE);
	}
	return 1;
}

static int _add_default_folders_to_list(EmailAccountUGD *ug_data,
		int mailbox_type,
		email_mailbox_t *mailbox_list,
		int mailbox_list_count)
{
	debug_enter();

	Item_Data *tree_item_data = NULL;
	int j = 0;

	for (; j < mailbox_list_count; j++) {
		if (g_strcmp0(mailbox_list[j].mailbox_name, "[Gmail]") && (mailbox_list[j].mailbox_type == mailbox_type)) {
			tree_item_data = calloc(1, sizeof(Item_Data));
			retvm_if(!tree_item_data, 0, "tree_item_data is NULL!");
			tree_item_data->ug_data = ug_data;
			tree_item_data->mailbox_name = strdup(mailbox_list[j].mailbox_name);
			tree_item_data->mailbox_type = mailbox_list[j].mailbox_type;
			tree_item_data->unread_count = mailbox_list[j].unread_count;
			tree_item_data->total_mail_count_on_local = mailbox_list[j].total_mail_count_on_local;
			tree_item_data->mailbox_id = mailbox_list[j].mailbox_id;

			if (tree_item_data->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX
					&& (email_get_scheduled_outbox_mail_count_by_account_id(mailbox_list[j].account_id, false) > 0)) {
				tree_item_data->b_scheduled_outbox = 1;
			} else {
				tree_item_data->b_scheduled_outbox = 0;
			}
			tree_item_data->alias_name = strdup(mailbox_list[j].alias);

			tree_item_data->it = elm_genlist_item_append(ug_data->gl, ug_data->itc_single,
					tree_item_data,
					NULL,
					ELM_GENLIST_ITEM_NONE,
					_gl_sel_single,
					tree_item_data);

			if (tree_item_data->ug_data->folder_mode != ACC_FOLDER_NONE) {
				elm_object_item_disabled_set(tree_item_data->it, EINA_TRUE);
			}
			break;
		}
	}
	return 1;
}

static void _add_user_defined_folders_to_list(EmailAccountUGD *ug_data,
		email_mailbox_t *mailbox_list,
		int mailbox_list_count, int *inserted_count)
{
	debug_enter();

	Item_Data *tree_item_data = NULL;
	int inserted_mailbox_id[mailbox_list_count];
	memset(inserted_mailbox_id, 0, sizeof(inserted_mailbox_id));
	int j = 0;

	for (; j < mailbox_list_count; j++) {
		if ((inserted_mailbox_id[j] == 0) && (g_strcmp0(mailbox_list[j].mailbox_name, "[Gmail]")
			&& (mailbox_list[j].mailbox_type == EMAIL_MAILBOX_TYPE_USER_DEFINED)
			&& (mailbox_list[j].no_select == 0))) {
				tree_item_data = calloc(1, sizeof(Item_Data));
				retm_if(!tree_item_data, "tree_item_data is NULL!");
				tree_item_data->ug_data = ug_data;
				tree_item_data->mailbox_name = strdup(mailbox_list[j].mailbox_name);
				tree_item_data->mailbox_type = mailbox_list[j].mailbox_type;
				tree_item_data->unread_count = mailbox_list[j].unread_count;
				tree_item_data->total_mail_count_on_local = mailbox_list[j].total_mail_count_on_local;
				tree_item_data->mailbox_id = mailbox_list[j].mailbox_id;
				tree_item_data->mailbox_list = mailbox_list;
				inserted_mailbox_id[j] = mailbox_list[j].mailbox_id;
				tree_item_data->b_scheduled_outbox = 0;
				tree_item_data->alias_name = strdup(mailbox_list[j].alias);

				tree_item_data->it = elm_genlist_item_append(ug_data->gl, ug_data->itc_single, tree_item_data, tree_item_data->ug_data->group_title_item, ELM_GENLIST_ITEM_NONE, _gl_sel_single, tree_item_data);
				if (tree_item_data->ug_data->folder_mode != ACC_FOLDER_NONE) {
					elm_object_item_disabled_set(tree_item_data->it, EINA_TRUE);
				}
				(*inserted_count)++;
		}
	}
}

int account_create_single_account_folder_list(EmailAccountUGD *ug_data)
{
	debug_enter();

	retv_if(!ug_data, -1);

	int res = 0;
	int i = 0;
	int inserted_count = 0;
	int mailbox_list_count = 0;
	email_mailbox_t *mailbox_list = NULL;

	if (ug_data->account_id == 0 && ug_data->account_count == 1) {

		if (!email_engine_get_default_account(&(ug_data->default_account_id))) {
			debug_log("email_engine_get_default_account failed");
			return -1;
		}
		ug_data->account_id = ug_data->default_account_id;
	}

	if (ug_data->folder_view_mode == ACC_FOLDER_SINGLE_VIEW_MODE) {

		if (ug_data->user_email_address == NULL) {
			ug_data->user_email_address = account_get_user_email_address(ug_data->account_id);
		}
		_add_account_name_item_to_list(ug_data, ug_data->user_email_address);
	}

	int order_mailbox_type[EMAIL_SING_ACC_MANDAT_FOLDER_COUNT] = {
			EMAIL_MAILBOX_TYPE_INBOX,
			EMAIL_MAILBOX_TYPE_PRIORITY_SENDERS,
			EMAIL_MAILBOX_TYPE_FLAGGED,
			EMAIL_MAILBOX_TYPE_DRAFT,
			EMAIL_MAILBOX_TYPE_OUTBOX,
			EMAIL_MAILBOX_TYPE_SENTBOX,
			EMAIL_MAILBOX_TYPE_SPAMBOX,
			EMAIL_MAILBOX_TYPE_TRASH
	};

	res = email_get_mailbox_list_ex(ug_data->account_id, -1, 1, &mailbox_list, &mailbox_list_count);
	if (res != EMAIL_ERROR_NONE) {
		debug_critical("email_get_mailbox_list return error, err : %d", res);
		return -1;
	}

	for (; i < EMAIL_SING_ACC_MANDAT_FOLDER_COUNT; i++) {
		if ((order_mailbox_type[i] == EMAIL_MAILBOX_TYPE_PRIORITY_SENDERS
				|| order_mailbox_type[i] == EMAIL_MAILBOX_TYPE_FLAGGED)
				&& ug_data->folder_view_mode == ACC_FOLDER_SINGLE_VIEW_MODE) {
			res = _add_custom_folders_to_list(ug_data, order_mailbox_type[i]);
		} else {
			res = _add_default_folders_to_list(ug_data, order_mailbox_type[i], mailbox_list, mailbox_list_count);
		}
		if (res == -1) {
			debug_critical("Failed to add item to list");
			return res;
		}
		inserted_count += res;
	}

	if (_try_add_group_item_to_list(ug_data)) {
		_add_user_defined_folders_to_list(ug_data, mailbox_list, mailbox_list_count, &inserted_count);
	}

	res = email_free_mailbox(&mailbox_list, mailbox_list_count);
	if (res != EMAIL_ERROR_NONE) {
		debug_critical("fail to free mailbox list - err(%d)", res);
		return -1;
	}

	debug_leave();
	return inserted_count;
}

void account_create_folder_create_view(void *data)
{
	debug_enter();
	RETURN_IF_FAIL(data != NULL);

	EmailAccountUGD *ug_data = (EmailAccountUGD *)data;

	if (ug_data->folder_mode == ACC_FOLDER_NONE) {
		ug_data->folder_mode = ACC_FOLDER_CREATE;

		_add_root_item_in_genlist(data);
		account_update_folder_item_dim_state(data);

	} else if (ug_data->folder_mode == ACC_FOLDER_CREATE) {
		ug_data->folder_mode = ACC_FOLDER_NONE;

		if (ug_data->root_item) {
			elm_object_item_del(ug_data->root_item);
			ug_data->root_item = NULL;
		}
		account_update_folder_item_dim_state(data);
		account_update_view_title(data);
	}
	debug_leave();

}

void account_delete_folder_view(void *data)
{
	debug_enter();
	RETURN_IF_FAIL(data != NULL);

	EmailAccountUGD *ug_data = (EmailAccountUGD *)data;

	if (ug_data->folder_mode == ACC_FOLDER_NONE) {
		ug_data->folder_mode = ACC_FOLDER_DELETE;
		account_update_folder_item_dim_state(ug_data);
		elm_object_item_domain_translatable_text_set(ug_data->base.navi_item, PACKAGE, _("IDS_EMAIL_HEADER_SELECT_FOLDER_ABB2"));

	} else if (ug_data->folder_mode == ACC_FOLDER_DELETE) {
		ug_data->folder_mode = ACC_FOLDER_NONE;

		account_update_folder_item_dim_state(ug_data);
		account_update_view_title(ug_data);
	}
	debug_leave();
}

void account_rename_folder_view(void *data)
{
	debug_enter();
	RETURN_IF_FAIL(data != NULL);

	EmailAccountUGD *ug_data = (EmailAccountUGD *)data;

	if (ug_data->folder_mode == ACC_FOLDER_NONE) {
		ug_data->folder_mode = ACC_FOLDER_RENAME;
		account_update_folder_item_dim_state(ug_data);
		elm_object_item_domain_translatable_text_set(ug_data->base.navi_item, PACKAGE, _("IDS_EMAIL_HEADER_SELECT_FOLDER_ABB2"));

	} else if (ug_data->folder_mode == ACC_FOLDER_RENAME) {
		ug_data->folder_mode = ACC_FOLDER_NONE;

		account_update_folder_item_dim_state(ug_data);
		account_update_view_title(ug_data);
	}
	debug_leave();
}

void account_gdbus_event_account_receive(GDBusConnection *connection,
											const gchar *sender_name,
											const gchar *object_path,
											const gchar *interface_name,
											const gchar *signal_name,
											GVariant *parameters,
											gpointer data)
{
	debug_enter();


	if (get_app_terminated()) {
		debug_log("App is in terminating");
		return;
	}
	debug_secure("Object path=%s, interface name=%s, signal name=%s", object_path, interface_name, signal_name);
	EmailAccountUGD *ug_data = (EmailAccountUGD *)data;

	if (ug_data == NULL) {
		debug_log("data is NULL");
		return;
	}

	debug_log("folder_mode[%d]", ug_data->folder_mode);

	if (!(g_strcmp0(object_path, "/User/Email/StorageChange")) && !(g_strcmp0(signal_name, "email"))) {
		debug_log("User.Email.StorageChange");

		int subtype = 0;
		int data1 = 0;
		int data2 = 0;
		char *data3 = NULL;
		int data4 = 0;

		g_variant_get(parameters, "(iiisi)", &subtype, &data1, &data2, &data3, &data4);
		debug_secure("STORE_ENABLE: subtype: %d, data1: %d, data2: %d, data3: %s, data4: %d", subtype, data1, data2, data3, data4);

		int account_id = 0;
		switch (subtype) {
		/* folder list updating is here in case of native account
		 * folder list updating and popup handling is here in case of EAS account */
		case NOTI_MAILBOX_ADD:
			account_id = data1;

			debug_log("NOTI_MAILBOX_ADD, account_id [%d], folder_view_mode[%d]", account_id, ug_data->folder_view_mode);
			if (ug_data->folder_view_mode == ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE
				|| ug_data->folder_view_mode == ACC_FOLDER_MOVE_MAIL_VIEW_MODE) {
				G_FREE(data3);
				return;
			}

			if (ug_data->emf_handle != EMAIL_HANDLE_INVALID) {
				debug_log("Native account folder added.");
				_update_folder_list_after_folder_action(ug_data, ACC_FOLDER_ACTION_CREATE, data2, false);
			} else {
				if (ug_data->account_id == account_id) {
					int account_type = GET_ACCOUNT_SERVER_TYPE(account_id);
					if (account_type == EMAIL_SERVER_TYPE_IMAP4) {
						debug_log("folder update(add) is occurred regardless of user's intention.");
						ug_data->need_refresh++;
					} else if (account_type == EMAIL_SERVER_TYPE_POP3) {
						debug_log("POP account folder added.");
						_update_folder_list_after_folder_action(ug_data, ACC_FOLDER_ACTION_CREATE, data2, true);
					} else {
						debug_log("folder update(add) is occurred regardless of user's intention.");
						_finish_folder_view(ug_data, NULL, NULL);
					}
				}
			}
			break;

		case NOTI_MAILBOX_DELETE:
			account_id = data1;

			debug_log("NOTI_MAILBOX_DELETE, account_id [%d], folder_view_mode[%d]", account_id, ug_data->folder_view_mode);
			if (ug_data->folder_view_mode == ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE
				|| ug_data->folder_view_mode == ACC_FOLDER_MOVE_MAIL_VIEW_MODE) {
				G_FREE(data3);
				return;
			}

			if (ug_data->emf_handle != EMAIL_HANDLE_INVALID) {
				if (ug_data->target_mailbox_id == data2) {
					debug_log("NOTI_MAILBOX_DELETE");
					_update_folder_list_after_folder_delete_action(ug_data, ACC_FOLDER_ACTION_DELETE, data2, false);
				} else {
					debug_log("Mailbox is deleted.(sub-folder is deleted)");
				}
			} else {
				if (ug_data->account_id == account_id) {
					int account_type = GET_ACCOUNT_SERVER_TYPE(account_id);
					if (account_type == EMAIL_SERVER_TYPE_IMAP4) {
						debug_log("folder update(delete) is occurred regardless of user's intention.");
						ug_data->need_refresh++;
					} else if (account_type == EMAIL_SERVER_TYPE_POP3) {
						debug_log("POP account folder deleted.");
						_update_folder_list_after_folder_delete_action(ug_data, ACC_FOLDER_ACTION_DELETE, data2, true);
					} else {
						debug_log("folder update(delete) is occurred regardless of user's intention.");
						_finish_folder_view(ug_data, NULL, NULL);
					}
				}
			}
			break;

		case NOTI_MAILBOX_UPDATE:
			account_id = data1;

			debug_log("NOTI_MAILBOX_UPDATE, account_id [%d], folder_view_mode[%d]", account_id, ug_data->folder_view_mode);
			if (ug_data->folder_view_mode == ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE
				|| ug_data->folder_view_mode == ACC_FOLDER_MOVE_MAIL_VIEW_MODE) {
				G_FREE(data3);
				return;
			}

			if (EMAIL_SERVER_TYPE_ACTIVE_SYNC != GET_ACCOUNT_SERVER_TYPE(account_id)) {
				debug_log("NOTI_MAILBOX_UPDATE");
				_finish_folder_view(ug_data, NULL, NULL);
			} else {
				debug_log("EAS account folder moved");
			}
			break;
		case NOTI_MAILBOX_RENAME:
			account_id = data1;

			debug_log("NOTI_MAILBOX_RENAME, account_id [%d], folder_view_mode[%d]", account_id, ug_data->folder_view_mode);
			if (ug_data->folder_view_mode == ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE
				|| ug_data->folder_view_mode == ACC_FOLDER_MOVE_MAIL_VIEW_MODE) {
				G_FREE(data3);
				return;
			}

			if (ug_data->emf_handle != EMAIL_HANDLE_INVALID) {
				debug_log("NOTI_MAILBOX_RENAME");
				_finish_folder_view(ug_data, NULL, NULL);
			} else if (ug_data->account_id == account_id) {
				int account_type = GET_ACCOUNT_SERVER_TYPE(account_id);
				if (account_type == EMAIL_SERVER_TYPE_IMAP4) {
					debug_log("folder update(rename) is occurred regardless of user's intention.");
					ug_data->need_refresh++;
				} else if (account_type == EMAIL_SERVER_TYPE_POP3) {
					debug_log("POP account folder renamed.");
					_finish_folder_view(ug_data, NULL, NULL);
				} else {
					debug_log("folder update(rename) is occurred regardless of user's intention.");
					_finish_folder_view(ug_data, NULL, NULL);
				}
			}
			break;

		case NOTI_MAILBOX_RENAME_FAIL:
			account_id = data1;

			debug_log("NOTI_MAILBOX_RENAME_FAIL, account_id [%d], folder_view_mode[%d]", account_id, ug_data->folder_view_mode);
			if (ug_data->folder_view_mode == ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE
				|| ug_data->folder_view_mode == ACC_FOLDER_MOVE_MAIL_VIEW_MODE) {
				G_FREE(data3);
				return;
			}

			if (EMAIL_SERVER_TYPE_ACTIVE_SYNC != GET_ACCOUNT_SERVER_TYPE(account_id)) {
				debug_log("NOTI_MAILBOX_RENAME_FAIL");
				_update_folder_view_after_folder_action(ug_data);
			}
			break;

		case NOTI_ACCOUNT_UPDATE_SYNC_STATUS:
			account_id = data1;

			debug_log("NOTI_ACCOUNT_UPDATE_SYNC_STATUS, account_id [%d], folder_view_mode[%d]", account_id, ug_data->folder_view_mode);
			if (ug_data->folder_view_mode == ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE
				|| ug_data->folder_view_mode == ACC_FOLDER_MOVE_MAIL_VIEW_MODE) {
				G_FREE(data3);
				return;
			}

			if (data2 == SYNC_STATUS_FINISHED && (ug_data->account_id == data1 || ug_data->account_id == 0)) {
				_update_mail_count_on_folder_view(ug_data, data1);
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
				account_color_list_update(ug_data, account_id, account->color_label);
				email_engine_free_account_list(&account, 1);
			}

			break;

		case NOTI_ACCOUNT_DELETE:
			debug_log("NOTI_ACCOUNT_DELETE");
			account_id = data1;

			ug_data->account_count--;
			account_color_list_remove(ug_data, account_id);
			break;

		default:
			debug_log("Uninterested notification");
			break;
		}
		G_FREE(data3);
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
		int email_handle = 0;
		int err_msg = 0;
		account_id = data1;

		switch (subtype) {
		/* popup handling is here. In case of EAS account, network noti should be handled only. */
			email_handle = data3;
			err_msg = data4;
			debug_log("email_handle: %d, err_msg: %d", email_handle, err_msg);

		case NOTI_ADD_MAILBOX_START:
			debug_log("NOTI_ADD_MAILBOX_START");
			break;

		case NOTI_ADD_MAILBOX_CANCEL:
			debug_log("NOTI_ADD_MAILBOX_CANCEL");
			break;

		case NOTI_ADD_MAILBOX_FINISH:
			debug_log("NOTI_ADD_MAILBOX_FINISH, account_id [%d], folder_view_mode[%d]", account_id, ug_data->folder_view_mode);
			if (ug_data->folder_view_mode == ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE
				|| ug_data->folder_view_mode == ACC_FOLDER_MOVE_MAIL_VIEW_MODE) {
				G_FREE(data2);
				return;
			}
			debug_log("NOTI_ADD_MAILBOX_FINISH");
			if (ug_data->emf_handle != EMAIL_HANDLE_INVALID) {
				_popup_success_cb(ug_data, NULL, NULL);
			}
			break;

		case NOTI_ADD_MAILBOX_FAIL:
			debug_log("NOTI_ADD_MAILBOX_FAIL, account_id [%d], folder_view_mode[%d]", account_id, ug_data->folder_view_mode);
			if (ug_data->folder_view_mode == ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE
				|| ug_data->folder_view_mode == ACC_FOLDER_MOVE_MAIL_VIEW_MODE) {
				G_FREE(data2);
				return;
			}

			_popup_fail_cb(ug_data, NULL, NULL, data4);
			break;

		case NOTI_DELETE_MAILBOX_START:
			debug_log("NOTI_DELETE_MAILBOX_START");
			break;

		case NOTI_DELETE_MAILBOX_CANCEL:
			debug_log("NOTI_DELETE_MAILBOX_CANCEL");
			break;

		case NOTI_DELETE_MAILBOX_FINISH:
			debug_log("NOTI_DELETE_MAILBOX_FINISH, account_id [%d], folder_view_mode[%d]", account_id, ug_data->folder_view_mode);
			if (ug_data->folder_view_mode == ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE
					|| ug_data->folder_view_mode == ACC_FOLDER_MOVE_MAIL_VIEW_MODE) {
				G_FREE(data2);
				return;
			}
			debug_log("NOTI_DELETE_MAILBOX_FINISH");
			if (ug_data->target_mailbox_id == data1) {
				_popup_success_cb(ug_data, NULL, NULL);
			}
			break;

		case NOTI_DELETE_MAILBOX_FAIL:
			debug_log("NOTI_DELETE_MAILBOX_FAIL, account_id [%d], folder_view_mode[%d]", account_id, ug_data->folder_view_mode);
			if (ug_data->folder_view_mode == ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE
				|| ug_data->folder_view_mode == ACC_FOLDER_MOVE_MAIL_VIEW_MODE) {
				G_FREE(data2);
				return;
			}

			_popup_fail_cb(ug_data, NULL, NULL, data4);
			break;

		case NOTI_RENAME_MAILBOX_START:
			debug_log("NOTI_RENAME_MAILBOX_START");
			break;

		case NOTI_RENAME_MAILBOX_FINISH:
			debug_log("NOTI_RENAME_MAILBOX_FINISH, account_id [%d], folder_view_mode[%d]", account_id, ug_data->folder_view_mode);
			if (ug_data->folder_view_mode == ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE
				|| ug_data->folder_view_mode == ACC_FOLDER_MOVE_MAIL_VIEW_MODE) {
				G_FREE(data2);
				return;
			}
			debug_log("NOTI_RENAME_MAILBOX_FINISH");
			if (ug_data->target_mailbox_id == data1) {
				_popup_success_cb(ug_data, NULL, NULL);
			}
			break;

		case NOTI_RENAME_MAILBOX_FAIL:
			debug_log("NOTI_RENAME_MAILBOX_FAIL, account_id [%d], folder_view_mode[%d]", account_id, ug_data->folder_view_mode);
			if (ug_data->folder_view_mode == ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE
				|| ug_data->folder_view_mode == ACC_FOLDER_MOVE_MAIL_VIEW_MODE) {
				G_FREE(data2);
				return;
			}

			_popup_fail_cb(ug_data, NULL, NULL, data4);
			break;

		case NOTI_RENAME_MAILBOX_CANCEL:
			debug_log("NOTI_RENAME_MAILBOX_CANCEL");
			break;

		case NOTI_SYNC_IMAP_MAILBOX_LIST_FINISH:
			debug_log("NOTI_SYNC_IMAP_MAILBOX_LIST_FINISH, account_id [%d], folder_view_mode[%d]", account_id, ug_data->folder_view_mode);
			if (ug_data->folder_view_mode == ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE
				|| ug_data->folder_view_mode == ACC_FOLDER_MOVE_MAIL_VIEW_MODE) {
				G_FREE(data2);
				return;
			}

			if (ug_data->account_id == account_id && ug_data->need_refresh > 0) {
				ug_data->need_refresh = 0;
				debug_log("folder view is refreshed");
				_finish_folder_view(ug_data, NULL, NULL);
			}
			break;

		case NOTI_DOWNLOAD_FINISH:
			debug_log("NOTI_DOWNLOAD_FINISH, account_id [%d], folder_view_mode[%d]", account_id, ug_data->folder_view_mode);
			if (ug_data->folder_view_mode == ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE
				|| ug_data->folder_view_mode == ACC_FOLDER_MOVE_MAIL_VIEW_MODE) {
				G_FREE(data2);
				return;
			}

			if (ug_data->account_id == data1 || ug_data->account_id == 0) {
				_update_mail_count_on_folder_view(ug_data, data1);
			}
			break;


		default:
			debug_warning("unknown type");
			break;
		}
		G_FREE(data2);
	} else {
		debug_warning("We receive dbus message, but we can't do anything");
	}
	debug_leave();
}

static char *_create_gl_item_text(Item_Data *item_data, const char *text)
{
	EmailAccountUGD *ug_data = item_data->ug_data;

	char buff[MAX_STR_LEN] = { 0 };
	if ((item_data->mailbox_id == ug_data->mailbox_id
			&& item_data->mailbox_type == ug_data->mailbox_type)
			&& (ug_data->account_id != 0 || (ug_data->account_id == 0
					&& ug_data->folder_view_mode == ACC_FOLDER_COMBINED_VIEW_MODE))) {
		snprintf(buff, sizeof(buff), "<font=System:style=Bold><color=#%02x%02x%02x%02x>%s</color></font>", ACCOUNT_CURRENT_FOLDER_COLOR, text);
	} else {
		snprintf(buff, sizeof(buff), "<font=System:style=Regular>%s</font>", text);
	}
	return strdup(buff);
}

static char *_gl_label_get_for_single_subitem(void *data, Evas_Object *obj, const char *part)
{
	Item_Data *tree_item_data = (Item_Data *)data;
	EmailAccountUGD *ug_data = tree_item_data->ug_data;
	char tmp[MAX_STR_LEN] = { 0 };
	int mail_count = 0;
	char *converted_name = NULL;
	char *filename = NULL;

	if (strcmp(part, "elm.text")) {
		return NULL;
	}
	if (tree_item_data->b_scheduled_outbox > 0) {
		mail_count = email_get_scheduled_outbox_mail_count_by_account_id(tree_item_data->b_scheduled_outbox, false);
		if (mail_count == 0) {
			return strdup(_("IDS_EMAIL_MBODY_SCHEDULED_OUTBOX"));
		} else {
			snprintf(tmp, sizeof(tmp), "%s (%d)", _("IDS_EMAIL_MBODY_SCHEDULED_OUTBOX"), mail_count);
			return _create_gl_item_text(tree_item_data, tmp);
		}
	} else {
		if (tree_item_data->alias_name != NULL) {
			if (tree_item_data->mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX) {
				int scheduled_mail_total = email_get_scheduled_outbox_mail_count_by_account_id(tree_item_data->ug_data->account_id, false);
				if (tree_item_data->total_mail_count_on_local - scheduled_mail_total == 0)
					snprintf(tmp, sizeof(tmp), "%s", _("IDS_EMAIL_HEADER_OUTBOX"));
				else
					snprintf(tmp, sizeof(tmp), "%s (%d)", _("IDS_EMAIL_HEADER_OUTBOX"), (tree_item_data->total_mail_count_on_local - scheduled_mail_total));
			} else if (tree_item_data->mailbox_type == EMAIL_MAILBOX_TYPE_DRAFT) {
				if (tree_item_data->total_mail_count_on_local == 0)
					snprintf(tmp, sizeof(tmp), "%s", _("IDS_EMAIL_HEADER_DRAFTS"));
				else
					snprintf(tmp, sizeof(tmp), "%s (%d)", _("IDS_EMAIL_HEADER_DRAFTS"), tree_item_data->total_mail_count_on_local);
			} else if (tree_item_data->mailbox_type == EMAIL_MAILBOX_TYPE_INBOX) {
				if (tree_item_data->unread_count == 0)
					snprintf(tmp, sizeof(tmp), "%s", _("IDS_EMAIL_HEADER_INBOX"));
				else
					snprintf(tmp, sizeof(tmp), "%s (%d)", _("IDS_EMAIL_HEADER_INBOX"), tree_item_data->unread_count);
			} else if (tree_item_data->mailbox_type == EMAIL_MAILBOX_TYPE_SENTBOX) {
				if (tree_item_data->unread_count == 0)
					snprintf(tmp, sizeof(tmp), "%s", _("IDS_EMAIL_HEADER_SENT_M_EMAIL"));
				else
					snprintf(tmp, sizeof(tmp), "%s (%d)", _("IDS_EMAIL_HEADER_SENT_M_EMAIL"), tree_item_data->unread_count);
			} else if (tree_item_data->mailbox_type == EMAIL_MAILBOX_TYPE_SPAMBOX) {
				if (tree_item_data->unread_count == 0)
					snprintf(tmp, sizeof(tmp), "%s", _("IDS_EMAIL_HEADER_SPAMBOX"));
				else
					snprintf(tmp, sizeof(tmp), "%s (%d)", _("IDS_EMAIL_HEADER_SPAMBOX"), tree_item_data->unread_count);
			} else if (tree_item_data->mailbox_type == EMAIL_MAILBOX_TYPE_TRASH) {
				if (tree_item_data->unread_count == 0)
					snprintf(tmp, sizeof(tmp), "%s", _("IDS_EMAIL_HEADER_RECYCLE_BIN_ABB"));
				else
					snprintf(tmp, sizeof(tmp), "%s (%d)", _("IDS_EMAIL_HEADER_RECYCLE_BIN_ABB"), tree_item_data->unread_count);
#ifndef _FEATURE_PRIORITY_SENDER_DISABLE_
			} else if (tree_item_data->mailbox_type == EMAIL_MAILBOX_TYPE_PRIORITY_SENDERS) {
				if (tree_item_data->unread_count == 0)
					snprintf(tmp, sizeof(tmp), "%s", _("IDS_EMAIL_HEADER_PRIORITY_SENDERS_ABB"));
				else
					snprintf(tmp, sizeof(tmp), "%s (%d)", _("IDS_EMAIL_HEADER_PRIORITY_SENDERS_ABB"), tree_item_data->unread_count);
#endif
			} else if (tree_item_data->mailbox_type == EMAIL_MAILBOX_TYPE_FLAGGED) {
				if (tree_item_data->unread_count == 0)
					snprintf(tmp, sizeof(tmp), "%s", _("IDS_EMAIL_HEADER_STARRED_EMAILS_ABB"));
				else
					snprintf(tmp, sizeof(tmp), "%s (%d)", _("IDS_EMAIL_HEADER_STARRED_EMAILS_ABB"), tree_item_data->unread_count);
			} else {
				if (tree_item_data->unread_count == 0)
					snprintf(tmp, sizeof(tmp), "%s", tree_item_data->mailbox_name);
				else
					snprintf(tmp, sizeof(tmp), "%s (%d)", tree_item_data->mailbox_name, tree_item_data->unread_count);
				if (EMAIL_SERVER_TYPE_ACTIVE_SYNC == GET_ACCOUNT_SERVER_TYPE(ug_data->account_id)) {
					filename = account_get_ellipsised_folder_name(ug_data, tmp);
				} else {
					converted_name = account_util_convert_mutf7_to_utf8(tmp);
					filename = account_get_ellipsised_folder_name(ug_data, converted_name);
				}
				debug_secure("tmp(%s) converted_name(%s) filename(%s) mailbox_name(%s) alias_name(%s)",
					tmp, converted_name, filename, tree_item_data->mailbox_name, tree_item_data->alias_name);

				free(converted_name);
				return _create_gl_item_text(tree_item_data, filename);;
			}
			return _create_gl_item_text(tree_item_data, tmp);
		} else {
			if (tree_item_data->unread_count == 0)
				snprintf(tmp, sizeof(tmp), "%s", tree_item_data->mailbox_name);
			else
				snprintf(tmp, sizeof(tmp), "%s (%d)", tree_item_data->mailbox_name, tree_item_data->unread_count);

			if (EMAIL_SERVER_TYPE_ACTIVE_SYNC == GET_ACCOUNT_SERVER_TYPE(ug_data->account_id)) {
				filename = account_get_ellipsised_folder_name(ug_data, tmp);
			} else {
				converted_name = account_util_convert_mutf7_to_utf8(tmp);
				filename = account_get_ellipsised_folder_name(ug_data, converted_name);
			}

			debug_secure("tmp(%s) converted_name(%s) filename(%s) mailbox_name(%s) alias_name(%s)",
				tmp, converted_name, filename, tree_item_data->mailbox_name, tree_item_data->alias_name);

			free(converted_name);
			return _create_gl_item_text(tree_item_data, filename);;
		}
	}

	return NULL;
}

static char *_gl_account_item_text_get(void *data, Evas_Object *obj, const char *part)
{
	debug_enter();

	if (!g_strcmp0(part, "elm.text")) {
		Item_Data *item_data = (Item_Data *)data;
		if (!item_data) {
			debug_log("item_data is NULL");
			return NULL;
		}
		return strdup(item_data->account_name);
	}
	return NULL;
}

static void _gl_account_item_del(void *data, Evas_Object *obj)
{
	debug_enter();

	Item_Data *item_data = (Item_Data *)data;
	if (!item_data) {
		debug_log("item_data is NULL");
		return;
	}

	FREE(item_data->account_name);
	FREE(item_data);
}

static void _gl_del(void *data, Evas_Object *obj)
{
	Item_Data *item_data = (Item_Data *)data;
	if (!item_data) {
		debug_log("item_data is NULL");
		return;
	}
	EmailAccountUGD *ug_data = item_data->ug_data;
	if (ug_data && ug_data->move_from_item && (ug_data->move_from_item == item_data->it)) {
		ug_data->move_from_item = NULL;
		debug_log("Reset move_from_item");
	}
	G_FREE(item_data->mailbox_name);
	G_FREE(item_data->alias_name);
	FREE(item_data);
}

static char *_gl_group_text_get(void *data, Evas_Object *obj, const char *part)
{
	if (!g_strcmp0(part, "elm.text"))
		return strdup(_("IDS_EMAIL_HEADER_FOLDERS_FOR_THIS_ACCOUNT_ABB2"));

	return NULL;
}

static void _gl_grouptitle_del(void *data, Evas_Object *obj)
{
	Item_Data *item_data = (Item_Data *)data;

	if (item_data && item_data->ug_data && item_data->ug_data->group_title_item)
		item_data->ug_data->group_title_item = NULL;

	FREE(item_data);
}

static void _update_folder_list_after_folder_delete_action(void *data, int action_type, int mailbox_id, bool show_success_popup)
{
	debug_enter();
	EmailAccountUGD *ug_data = (EmailAccountUGD *) data;
	if (ug_data == NULL) {
		debug_log("data is NULL");
		return;
	}

	if (ug_data->popup) {
		evas_object_del(ug_data->popup);
		ug_data->popup = NULL;
	}

	if (!(ug_data->it)) {
		debug_log("ug_data->it is NULL");
		return;
	}

	if (show_success_popup) {
		_popup_success_cb(ug_data, NULL, NULL);
	}

	Item_Data *item_data = (Item_Data *)elm_object_item_data_get((const Elm_Object_Item *)ug_data->it);
	if (!item_data) {
		debug_log("item_data is NULL");
		return;
	}

	Eina_List *item_list = NULL;
	Eina_List *list = NULL;
	int length = strlen(item_data->mailbox_name);
	ug_data->editmode = false;

	Elm_Object_Item *it = NULL;
	if (ug_data->gl) {
		it = elm_genlist_first_item_get(ug_data->gl);
		if (it == NULL) {
			debug_log("elm_genlist_first_item_get is failed");
			return;
		}
		Item_Data *tree_item_data = NULL;
		while (it) {
			Elm_Genlist_Item_Class *group_item_itc = (Elm_Genlist_Item_Class *)elm_genlist_item_item_class_get(it);
			if (group_item_itc) {
				debug_log("group_item_itc->item_style %s", group_item_itc->item_style);
				if (g_strcmp0(group_item_itc->item_style, "group_index")) {
					tree_item_data = (Item_Data *)elm_object_item_data_get((const Elm_Object_Item *)it);
					if (tree_item_data) {
						if (!strncmp(item_data->mailbox_name, tree_item_data->mailbox_name, length)) {
							item_list = eina_list_append(item_list, it);
						}
					}
				}
			}
			it = elm_genlist_item_next_get(it);
		}
	}

	if (ug_data->group_title_item) {
		int deleted_folder_cnt = eina_list_count(item_list);
		int user_folder_cnt = 0;

		Elm_Object_Item *group_item = elm_genlist_item_next_get(ug_data->group_title_item);
		while (group_item) {
			group_item = elm_genlist_item_next_get(group_item);
			user_folder_cnt++;
			if (user_folder_cnt > deleted_folder_cnt) {
				break;
			}
		}

		if (user_folder_cnt <= deleted_folder_cnt) {
			elm_object_item_del(ug_data->group_title_item);
			debug_log("Remove group item(user_folder_cnt : %d, deleted_folder_cnt : %d)", user_folder_cnt, deleted_folder_cnt);
		}
	}

	EINA_LIST_FOREACH(item_list, list, it) {
		elm_object_item_del(it);
	}

	if (item_list) {
		eina_list_free(item_list);
	}

	debug_log("emf_handle [%d], folder_mode[%d]", ug_data->emf_handle, ug_data->folder_mode);
	ug_data->emf_handle = EMAIL_HANDLE_INVALID;

	_update_folder_view_after_folder_action(ug_data);
	ug_data->target_mailbox_id = -1;
	ug_data->it = NULL;

}

static void _update_folder_list_after_folder_action(void *data, int action_type, int mailbox_id, bool show_success_popup)
{
	debug_enter();
	EmailAccountUGD *ug_data = (EmailAccountUGD *) data;
	if (ug_data == NULL) {
		debug_log("data is NULL");
		return;
	}

	if (ug_data->popup) {
		evas_object_del(ug_data->popup);
		ug_data->popup = NULL;
	}

	if (!(ug_data->it)) {
		debug_log("ug_data->it is NULL");
		return;
	}

	Item_Data *tree_item_data = NULL;
	email_mailbox_t *selected_mailbox = NULL;
	bool item_inserted = false;

	if (show_success_popup) {
		_popup_success_cb(ug_data, NULL, NULL);
	}

	debug_log("emf_handle [%d], folder_mode[%d]", ug_data->emf_handle, ug_data->folder_mode);
	ug_data->emf_handle = EMAIL_HANDLE_INVALID;

	int e = email_get_mailbox_by_mailbox_id(mailbox_id, &selected_mailbox);
	if (e != EMAIL_ERROR_NONE || !selected_mailbox) {
		debug_warning("email_get_mailbox_by_mailbox_id mailbox_id(%d)- err[%d]", mailbox_id, e);
		goto FINISH;
	}

	if (ug_data->it == ug_data->root_item) {
		Elm_Object_Item *it = ug_data->group_title_item;
		Item_Data *next_item_data = NULL;
		tree_item_data = calloc(1, sizeof(Item_Data));
		if (!tree_item_data) {
			debug_error("tree_item_data is NULL - allocation memory failed");
			goto FINISH;
		}
		tree_item_data->ug_data = ug_data;
		tree_item_data->mailbox_type = selected_mailbox->mailbox_type;
		tree_item_data->unread_count = selected_mailbox->unread_count;
		tree_item_data->total_mail_count_on_local = selected_mailbox->total_mail_count_on_local;
		tree_item_data->mailbox_id = mailbox_id;
		tree_item_data->mailbox_list = selected_mailbox;

		if (selected_mailbox->mailbox_name) {
			tree_item_data->mailbox_name = strdup(selected_mailbox->mailbox_name);
		}

		if (selected_mailbox->alias) {
			tree_item_data->alias_name = strdup(selected_mailbox->alias);
		}

		tree_item_data->b_scheduled_outbox = 0;

		while (it) {
			it = elm_genlist_item_next_get(it);
			next_item_data = (Item_Data *)elm_object_item_data_get((const Elm_Object_Item *)it);
			if (next_item_data) {
				/* debug_secure("next_item_data: %s, selected_mailbox: %s", next_item_data->mailbox_name, selected_mailbox->mailbox_name); */

				if (next_item_data->mailbox_name && g_strcmp0(next_item_data->mailbox_name, selected_mailbox->mailbox_name) > 0
						&& (next_item_data->mailbox_type > EMAIL_MAILBOX_TYPE_OUTBOX)) {
					tree_item_data->it = elm_genlist_item_insert_before(ug_data->gl, ug_data->itc_single,
							tree_item_data, NULL, it, ELM_GENLIST_ITEM_NONE, _gl_sel_single, tree_item_data);
					/* debug_log("add item, tree_item_data->it : %p", tree_item_data->it); */
					item_inserted = true;
					break;
				}
			}
		}

		if (!item_inserted && ug_data->group_title_item) {
			tree_item_data->it = elm_genlist_item_insert_after(ug_data->gl, ug_data->itc_single, tree_item_data, NULL, ug_data->group_title_item, ELM_GENLIST_ITEM_NONE, _gl_sel_single, tree_item_data); /*ug_data->it */
		} else if (!item_inserted) {
			if (!ug_data->group_title_item) {
				Item_Data *group_item_data = calloc(1, sizeof(Item_Data));
				if (!group_item_data) {
					debug_error("group_item_data is NULL - allocation memory failed");
					free(tree_item_data->mailbox_name);
					free(tree_item_data->alias_name);
					free(tree_item_data);
					goto FINISH;
				}
				group_item_data->ug_data = ug_data;
				group_item_data->it = elm_genlist_item_append(ug_data->gl, ug_data->itc_group, group_item_data, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
				ug_data->group_title_item = group_item_data->it;
				elm_genlist_item_select_mode_set(group_item_data->it, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
				debug_log("group item is not existed, Add group_item_data->it : %p", group_item_data->it);
			}
			tree_item_data->it = elm_genlist_item_append(ug_data->gl, ug_data->itc_single, tree_item_data, NULL, ELM_GENLIST_ITEM_NONE, _gl_sel_single, tree_item_data); /* ug_data->it */
		}
	}

FINISH:
	ug_data->it = NULL;
	ug_data->move_from_item = NULL;
	ug_data->target_mailbox_id = -1;

	if (ug_data->root_item) {
		elm_object_item_del(ug_data->root_item);
		ug_data->root_item = NULL;
	}

	_update_folder_view_after_folder_action(data);

	email_free_mailbox(&selected_mailbox, 1);

	return;
}

static void _update_folder_view_after_folder_action(void *data)
{
	debug_enter();
	EmailAccountUGD *ug_data = (EmailAccountUGD *) data;

	if (ug_data == NULL) {
		debug_log("data is NULL");
		return;
	}

	if (ug_data->gl == NULL) {
		debug_log("genlist is NULL");
		return;
	}

	ug_data->folder_mode = ACC_FOLDER_NONE;

	account_update_folder_item_dim_state(data);

	/* Set the navigation bar title */
	if (ug_data->folder_view_mode == ACC_FOLDER_COMBINED_VIEW_MODE) {
		elm_object_item_part_text_set(ug_data->base.navi_item, "elm.text.title", _("IDS_EMAIL_HEADER_COMBINED_ACCOUNTS_ABB"));
	} else {
		elm_object_item_part_text_set(ug_data->base.navi_item, "subtitle", NULL);
		account_update_view_title(ug_data);
	}

	return;
}

static void _update_mail_count_on_folder_view(EmailAccountUGD *ug_data, int account_id)
{
	debug_log("sync finished account_id : %d", account_id);

	if (ug_data == NULL) {
		debug_log("data is NULL");
		return;
	}

	int updated = 0;
	if (ug_data->gl) {
		if (ug_data->folder_view_mode == ACC_FOLDER_COMBINED_VIEW_MODE) {
			int total_count = 0;
			int unread_count = 0;
			int index = ACC_MAILBOX_TYPE_INBOX;

			for (index = ACC_MAILBOX_TYPE_INBOX; index < ACC_MAILBOX_TYPE_MAX; index++) {
				switch (index) {
				case ACC_MAILBOX_TYPE_INBOX:
				{
					if (email_get_combined_mail_count_by_mailbox_type(EMAIL_MAILBOX_TYPE_INBOX, &total_count, &unread_count)) {
						if (unread_count != ug_data->all_acc_unread_count[ACC_MAILBOX_TYPE_INBOX]) {
							ug_data->all_acc_total_count[ACC_MAILBOX_TYPE_INBOX] = total_count;
							ug_data->all_acc_unread_count[ACC_MAILBOX_TYPE_INBOX] = unread_count;
							updated++;
						}
					}
				}
				break;

				case ACC_MAILBOX_TYPE_PRIORITY_INBOX:
				{
					if (email_get_priority_sender_mail_count_ex(&total_count, &unread_count)) {
						if (unread_count != ug_data->all_acc_unread_count[ACC_MAILBOX_TYPE_PRIORITY_INBOX]) {
							ug_data->all_acc_total_count[ACC_MAILBOX_TYPE_PRIORITY_INBOX] = total_count;
							ug_data->all_acc_unread_count[ACC_MAILBOX_TYPE_PRIORITY_INBOX] = unread_count;
							updated++;
						}
					}
				}
				break;

				case ACC_MAILBOX_TYPE_FLAGGED:
				{
					if (email_get_favourite_mail_count_ex(&total_count, &unread_count)) {
						if (unread_count != ug_data->all_acc_unread_count[ACC_MAILBOX_TYPE_FLAGGED]) {
							ug_data->all_acc_total_count[ACC_MAILBOX_TYPE_FLAGGED] = total_count;
							ug_data->all_acc_unread_count[ACC_MAILBOX_TYPE_FLAGGED] = unread_count;
							updated++;
						}
					}
				}
				break;
				case ACC_MAILBOX_TYPE_DRAFT:
				{
					if (email_get_combined_mail_count_by_mailbox_type(EMAIL_MAILBOX_TYPE_DRAFT, &total_count, &unread_count)) {
						if (total_count != ug_data->all_acc_total_count[ACC_MAILBOX_TYPE_DRAFT]) {
							ug_data->all_acc_total_count[ACC_MAILBOX_TYPE_DRAFT] = total_count;
							ug_data->all_acc_unread_count[ACC_MAILBOX_TYPE_DRAFT] = unread_count;
							updated++;
						}
					}
				}
				break;
				case ACC_MAILBOX_TYPE_OUTBOX:
				{
					if (email_get_combined_mail_count_by_mailbox_type(EMAIL_MAILBOX_TYPE_OUTBOX, &total_count, &unread_count)) {
						if (total_count != ug_data->all_acc_total_count[ACC_MAILBOX_TYPE_OUTBOX]) {
							ug_data->all_acc_total_count[ACC_MAILBOX_TYPE_OUTBOX] = total_count;
							ug_data->all_acc_unread_count[ACC_MAILBOX_TYPE_OUTBOX] = unread_count;
							updated++;
						}
					}
				}
				break;
				case ACC_MAILBOX_TYPE_SENTBOX:
				{
					if (email_get_combined_mail_count_by_mailbox_type(EMAIL_MAILBOX_TYPE_SENTBOX, &total_count, &unread_count)) {
						if (unread_count != ug_data->all_acc_unread_count[ACC_MAILBOX_TYPE_SENTBOX]) {
							ug_data->all_acc_total_count[ACC_MAILBOX_TYPE_SENTBOX] = total_count;
							ug_data->all_acc_unread_count[ACC_MAILBOX_TYPE_SENTBOX] = unread_count;
							updated++;
						}
					}
				}
				break;
				case ACC_MAILBOX_TYPE_SPAMBOX:
				{
					if (email_get_combined_mail_count_by_mailbox_type(EMAIL_MAILBOX_TYPE_SPAMBOX, &total_count, &unread_count)) {
						if (unread_count != ug_data->all_acc_unread_count[ACC_MAILBOX_TYPE_SPAMBOX]) {
							ug_data->all_acc_total_count[ACC_MAILBOX_TYPE_SPAMBOX] = total_count;
							ug_data->all_acc_unread_count[ACC_MAILBOX_TYPE_SPAMBOX] = unread_count;
							updated++;
						}
					}
				}
				break;
				case ACC_MAILBOX_TYPE_TRASH:
				{
					if (email_get_combined_mail_count_by_mailbox_type(EMAIL_MAILBOX_TYPE_TRASH, &total_count, &unread_count)) {
						if (unread_count != ug_data->all_acc_unread_count[ACC_MAILBOX_TYPE_TRASH]) {
							ug_data->all_acc_total_count[ACC_MAILBOX_TYPE_TRASH] = total_count;
							ug_data->all_acc_unread_count[ACC_MAILBOX_TYPE_TRASH] = unread_count;
							updated++;
						}
					}
				}
				break;

				default:
					break;
				}
			}
		}

		if (updated > 0) {
			debug_log("Updage genlist. mail count is changed.");
			elm_genlist_realized_items_update(ug_data->gl);
		}
	}

}

static void _finish_folder_view(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailAccountUGD *ug_data = (EmailAccountUGD *) data;

	if (ug_data == NULL) {
		debug_log("data is NULL");
		return;
	}

	if (ug_data->gl == NULL) {
		debug_log("genlist is NULL");
		return;
	}

	account_destroy_view(ug_data);

	ug_data->folder_mode = ACC_FOLDER_NONE;

	account_create_list(ug_data);
	elm_object_part_content_set(ug_data->base.content, "elm.swallow.content", ug_data->gl);

	/* Set the navigation bar title */

	if (ug_data->folder_view_mode == ACC_FOLDER_COMBINED_VIEW_MODE) {
		elm_object_item_part_text_set(ug_data->base.navi_item, "elm.text.title", _("IDS_EMAIL_HEADER_COMBINED_ACCOUNTS_ABB"));
	} else if (ug_data->folder_view_mode == ACC_FOLDER_COMBINED_SINGLE_VIEW_MODE) {
		elm_object_item_part_text_set(ug_data->base.navi_item, "subtitle", NULL);
		if (ug_data->user_email_address) {
			elm_object_item_text_set(ug_data->base.navi_item, ug_data->user_email_address);
		} else {
			elm_object_item_domain_translatable_text_set(ug_data->base.navi_item, PACKAGE, "IDS_EMAIL_HEADER_FOLDERS");
		}
	} else if (ug_data->folder_view_mode == ACC_FOLDER_SINGLE_VIEW_MODE) {
		elm_object_item_domain_translatable_text_set(ug_data->base.navi_item, PACKAGE, "IDS_EMAIL_HEADER_MAILBOX_ABB");
	} else if (ug_data->folder_view_mode == ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE) {
		elm_object_item_part_text_set(ug_data->base.navi_item, "elm.text.title", _("IDS_EMAIL_HEADER_COMBINED_ACCOUNTS_ABB"));
	}

	return;
}

static char *_gl_label_get_for_all_acc_box(void *data, Evas_Object *obj, const char *part)
{
	if (!data) {
		debug_log("data is NULL");
		return NULL;
	}

	Item_Data *tree_item_data = (Item_Data *) data;
	EmailAccountUGD *ug_data = tree_item_data->ug_data;
	int i_boxtype = tree_item_data->i_boxtype;
	char tmp[MAX_STR_LEN] = { 0 };

	if (!strcmp(part, "elm.text")) {
		if (ug_data) {
			switch (i_boxtype) {
			case ACC_MAILBOX_TYPE_INBOX:
				if (ug_data->all_acc_unread_count[ACC_MAILBOX_TYPE_INBOX] == 0) {
					snprintf(tmp, sizeof(tmp), "%s", _("IDS_EMAIL_HEADER_COMBINED_INBOX_ABB"));
				} else {
					snprintf(tmp, sizeof(tmp), "%s (%d)", _("IDS_EMAIL_HEADER_COMBINED_INBOX_ABB"), ug_data->all_acc_unread_count[ACC_MAILBOX_TYPE_INBOX]);
				}
				break;
			case ACC_MAILBOX_TYPE_FLAGGED:
				if (ug_data->all_acc_unread_count[ACC_MAILBOX_TYPE_FLAGGED] == 0) {
					snprintf(tmp, sizeof(tmp), "%s", _("IDS_EMAIL_HEADER_STARRED_EMAILS_ABB"));
				} else {
					snprintf(tmp, sizeof(tmp), "%s (%d)", _("IDS_EMAIL_HEADER_STARRED_EMAILS_ABB"), ug_data->all_acc_unread_count[ACC_MAILBOX_TYPE_FLAGGED]);
				}
				break;
			case ACC_MAILBOX_TYPE_DRAFT:
				if (ug_data->all_acc_total_count[ACC_MAILBOX_TYPE_DRAFT] == 0) {
					snprintf(tmp, sizeof(tmp), "%s", _("IDS_EMAIL_HEADER_DRAFTS"));
				} else {
					snprintf(tmp, sizeof(tmp), "%s (%d)", _("IDS_EMAIL_HEADER_DRAFTS"), ug_data->all_acc_total_count[ACC_MAILBOX_TYPE_DRAFT]);
				}
				break;
			case ACC_MAILBOX_TYPE_OUTBOX:
				if (ug_data->all_acc_total_count[ACC_MAILBOX_TYPE_OUTBOX] == 0) {
					snprintf(tmp, sizeof(tmp), "%s", _("IDS_EMAIL_HEADER_OUTBOX"));
				} else {
					snprintf(tmp, sizeof(tmp), "%s (%d)", _("IDS_EMAIL_HEADER_OUTBOX"), ug_data->all_acc_total_count[ACC_MAILBOX_TYPE_OUTBOX]);
				}
				break;
			case ACC_MAILBOX_TYPE_SENTBOX:
				if (ug_data->all_acc_unread_count[ACC_MAILBOX_TYPE_SENTBOX] == 0) {
					snprintf(tmp, sizeof(tmp), "%s", _("IDS_EMAIL_HEADER_SENT_M_EMAIL"));
				} else {
					snprintf(tmp, sizeof(tmp), "%s (%d)", _("IDS_EMAIL_HEADER_SENT_M_EMAIL"), ug_data->all_acc_unread_count[ACC_MAILBOX_TYPE_SENTBOX]);
				}
				break;
			case ACC_MAILBOX_TYPE_SPAMBOX:
				if (ug_data->all_acc_unread_count[ACC_MAILBOX_TYPE_SPAMBOX] == 0) {
					snprintf(tmp, sizeof(tmp), "%s", _("IDS_EMAIL_HEADER_SPAMBOX"));
				} else {
					snprintf(tmp, sizeof(tmp), "%s (%d)", _("IDS_EMAIL_HEADER_SPAMBOX"), ug_data->all_acc_unread_count[ACC_MAILBOX_TYPE_SPAMBOX]);
				}
				break;
			case ACC_MAILBOX_TYPE_TRASH:
				if (ug_data->all_acc_unread_count[ACC_MAILBOX_TYPE_TRASH] == 0) {
					snprintf(tmp, sizeof(tmp), "%s", _("IDS_EMAIL_HEADER_RECYCLE_BIN_ABB"));
				} else {
					snprintf(tmp, sizeof(tmp), "%s (%d)", _("IDS_EMAIL_HEADER_RECYCLE_BIN_ABB"), ug_data->all_acc_unread_count[ACC_MAILBOX_TYPE_TRASH]);
				}
				break;
			case ACC_MAILBOX_TYPE_PRIORITY_INBOX:
				if (ug_data->all_acc_unread_count[ACC_MAILBOX_TYPE_PRIORITY_INBOX] == 0) {
					snprintf(tmp, sizeof(tmp), "%s", _("IDS_EMAIL_HEADER_PRIORITY_SENDERS_ABB"));
				} else {
					snprintf(tmp, sizeof(tmp), "%s (%d)", _("IDS_EMAIL_HEADER_PRIORITY_SENDERS_ABB"), ug_data->all_acc_unread_count[ACC_MAILBOX_TYPE_PRIORITY_INBOX]);
				}
				break;
			default:
				debug_log("invalid box type :%d", i_boxtype);
				return NULL;
			}
		}
		return _create_gl_item_text(tree_item_data, tmp);
	}
	return NULL;
}

static int _convert_acc_boxtype_to_email_boxtype(int box_type)
{
	int mailbox_type = EMAIL_MAILBOX_TYPE_NONE;

	switch (box_type) {
	case ACC_MAILBOX_TYPE_INBOX:
		mailbox_type = EMAIL_MAILBOX_TYPE_INBOX;
		break;
	case ACC_MAILBOX_TYPE_FLAGGED:
		mailbox_type = EMAIL_MAILBOX_TYPE_FLAGGED;
		break;
	case ACC_MAILBOX_TYPE_DRAFT:
		mailbox_type = EMAIL_MAILBOX_TYPE_DRAFT;
		break;
	case ACC_MAILBOX_TYPE_OUTBOX:
		mailbox_type = EMAIL_MAILBOX_TYPE_OUTBOX;
		break;
	case ACC_MAILBOX_TYPE_SENTBOX:
		mailbox_type = EMAIL_MAILBOX_TYPE_SENTBOX;
		break;
	case ACC_MAILBOX_TYPE_SPAMBOX:
		mailbox_type = EMAIL_MAILBOX_TYPE_SPAMBOX;
		break;
	case ACC_MAILBOX_TYPE_TRASH:
		mailbox_type = EMAIL_MAILBOX_TYPE_TRASH;
		break;
	case ACC_MAILBOX_TYPE_PRIORITY_INBOX:
		mailbox_type = EMAIL_MAILBOX_TYPE_PRIORITY_SENDERS;
		break;
	default:
		debug_log("account mailbox type not defined");
		break;
	}

	return mailbox_type;
}

static void _open_allacc_boxtype(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	int mailbox_type = (int)(ptrdiff_t)data;
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	Item_Data *item_data = elm_object_item_data_get((const Elm_Object_Item *)item);
	RETURN_IF_FAIL(item_data != NULL);
	EmailAccountUGD *ug_data = (EmailAccountUGD *) item_data->ug_data;

	elm_genlist_item_selected_set(item, 0);

	if (ug_data->block_item_click) {
		return;
	}
	elm_genlist_item_update(item);

	app_control_h service;
	if (APP_CONTROL_ERROR_NONE != app_control_create(&service)) {
		debug_log("creating service handle failed");
		return;
	}
	debug_log("mailbox_type: [%d]", mailbox_type);

	char id[NUM_STR_LEN] = {0,};
	snprintf(id, sizeof(id), "%d", 0);

	char ch_boxtype[NUM_STR_LEN] = { 0 };

	if (mailbox_type != EMAIL_MAILBOX_TYPE_PRIORITY_SENDERS) {
		snprintf(ch_boxtype, sizeof(ch_boxtype), "%d", mailbox_type);
	} else {
		snprintf(ch_boxtype, sizeof(ch_boxtype), "%d", EMAIL_MAILBOX_TYPE_INBOX);
	}

	app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_ACCOUNT_TYPE, EMAIL_BUNDLE_VAL_ALL_ACCOUNT);
	app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_ACCOUNT_ID, id);
	app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_MAILBOX_TYPE, ch_boxtype);

	if (mailbox_type == EMAIL_MAILBOX_TYPE_PRIORITY_SENDERS) {
		app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_ACCOUNT_TYPE, EMAIL_BUNDLE_VAL_PRIORITY_SENDER);
	}

	char is_mail_move_ug[NUM_STR_LEN] = {0,};
	snprintf(is_mail_move_ug, sizeof(is_mail_move_ug), "%d", 0);
	app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_IS_MAILBOX_MOVE_UG, is_mail_move_ug);

	email_module_send_result(ug_data->base.module, service);

	ug_data->block_item_click = 1;

	app_control_destroy(service);
}

static void _delete_con_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	RETURN_IF_FAIL(data != NULL);

	EmailAccountUGD *ug_data = (EmailAccountUGD *) data;
	Elm_Object_Item *it = ug_data->it;
	Item_Data *item_data = elm_object_item_data_get((const Elm_Object_Item *)it);
	if (item_data == NULL) {
		debug_log("item_data is NULL");
		return;
	}

	account_sync_cancel_all(ug_data);


	int err_code = EMAIL_ERROR_NONE;
	int handle;
	int on_server = (EMAIL_SERVER_TYPE_POP3 == GET_ACCOUNT_SERVER_TYPE(ug_data->account_id) ? 0 : 1);

	debug_secure("delfolder name[%s], account_id[%d], on_server[%d], mbox.mailbox_id[%d]", item_data->mailbox_name, ug_data->account_id, on_server, item_data->mailbox_id);

	err_code = email_delete_mailbox(item_data->mailbox_id, on_server, &handle);

	if (err_code < 0) {
		debug_log("\n email_delete_mailbox failed");
		_popup_fail_cb(ug_data, obj, event_info, err_code);
	} else {
		debug_log("\n email_delete_mailbox succeed : handle[%d]\n", handle);
		ug_data->emf_handle = handle;
		ug_data->target_mailbox_id = item_data->mailbox_id;
		_popup_progress_cb(ug_data, obj, event_info);
	}
}

static void _popup_delfolder_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailAccountUGD *ug_data = (EmailAccountUGD *)data;
	RETURN_IF_FAIL(ug_data != NULL);

	Elm_Object_Item *it = (Elm_Object_Item *)ug_data->it;
	Item_Data *tree_item_data = elm_object_item_data_get(it);

	if (!tree_item_data) {
		debug_log("tree_item_data is NULL");
		return;
	}

	if (ug_data->it == NULL) {
		Evas_Object *popup = NULL, *btn1 = NULL;
		popup = elm_popup_add(ug_data->base.module->navi);
		elm_popup_align_set(popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
		eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, _popup_destroy_cb, ug_data);
		evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		elm_object_domain_translatable_part_text_set(popup, "title,text", PACKAGE, "IDS_EMAIL_HEADER_DELETE");
		elm_object_domain_translatable_text_set(popup, PACKAGE, "IDS_EMAIL_POP_THE_SELECTED_DATA_COULD_NOT_BE_FOUND");

		btn1 = elm_button_add(popup);
		elm_object_style_set(btn1, "popup");
		elm_object_domain_translatable_text_set(btn1, PACKAGE, "IDS_EMAIL_BUTTON_OK");
		elm_object_part_content_set(popup, "button1", btn1);
		evas_object_smart_callback_add(btn1, "clicked", _popup_destroy_cb, ug_data);
		ug_data->popup = popup;
		evas_object_event_callback_add(popup, EVAS_CALLBACK_MOUSE_UP, _mouseup_cb, ug_data);
		evas_object_show(popup);
	} else {
		ug_data->folder_mode = ACC_FOLDER_DELETE;

		Evas_Object *notify = elm_popup_add(ug_data->base.module->navi);
		elm_popup_align_set(notify, ELM_NOTIFY_ALIGN_FILL, 1.0);
		eext_object_event_callback_add(notify, EEXT_CALLBACK_BACK, _back_button_cb, ug_data);
		evas_object_size_hint_weight_set(notify, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		elm_object_domain_translatable_part_text_set(notify, "title,text", PACKAGE, "IDS_EMAIL_HEADER_DELETE");
		elm_object_domain_translatable_text_set(notify, PACKAGE, "IDS_EMAIL_BODY_THIS_FOLDER_AND_ALL_ITS_CONTENT_WILL_BE_DELETED");
		/* elm_object_domain_translatable_text_set(notify, PACKAGE, "IDS_EMAIL_POP_DELETE_THIS_EMAIL_Q"); */

		Evas_Object *btn1 = elm_button_add(notify);
		elm_object_style_set(btn1, "popup");
		elm_object_domain_translatable_text_set(btn1, PACKAGE, "IDS_EMAIL_BUTTON_CANCEL");
		elm_object_part_content_set(notify, "button1", btn1);
		evas_object_smart_callback_add(btn1, "clicked", _back_button_cb, ug_data);

		Evas_Object *btn2 = elm_button_add(notify);
		elm_object_style_set(btn2, "popup");
		elm_object_domain_translatable_text_set(btn2, PACKAGE, "IDS_EMAIL_BUTTON_DELETE_ABB4");
		elm_object_part_content_set(notify, "button2", btn2);
		evas_object_smart_callback_add(btn2, "clicked", _delete_con_cb, ug_data);

		ug_data->popup = notify;
		evas_object_event_callback_add(notify, EVAS_CALLBACK_MOUSE_UP, _folder_popup_mouseup_cb, ug_data);

		evas_object_show(notify);
	}
}

static void _popup_renamefolder_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	RETURN_IF_FAIL(data != NULL);

	EmailAccountUGD *ug_data = (EmailAccountUGD *)data;
	Item_Data *item_data = NULL;
	char *prev_folder_name = NULL;
	if (ug_data->it) {
		item_data = elm_object_item_data_get(ug_data->it);
		prev_folder_name = account_util_convert_mutf7_to_utf8(item_data->mailbox_name);
	}

	email_account_string_t EMAIL_ACCOUNT_HEADER_RENAME_FOLDER = { PACKAGE, "IDS_EMAIL_OPT_RENAME_FOLDER"};
	account_create_entry_popup(ug_data, EMAIL_ACCOUNT_HEADER_RENAME_FOLDER, prev_folder_name, NULL,
			_rename_folder_cancel_cb, _mouseup_cb, _rename_folder_ok_cb,
			_rename_folder_cancel_cb, "IDS_EMAIL_BUTTON_CANCEL",
			_rename_folder_ok_cb, "IDS_EMAIL_BUTTON_RENAME_ABB");
	FREE(prev_folder_name);
	debug_leave();
	return;
}

static void _rename_folder_cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	RETURN_IF_FAIL(data != NULL);

	EmailAccountUGD *ug_data = (EmailAccountUGD *) data;

	if (ug_data->popup) {
		evas_object_del(ug_data->popup);
		ug_data->popup = NULL;
	}
	ug_data->it = NULL;
	ug_data->editmode = false;

	account_rename_folder_view(ug_data);
}

static void _rename_folder_ok_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	RETURN_IF_FAIL(data != NULL);
	EmailAccountUGD *ug_data = (EmailAccountUGD *)data;
	ug_data->editmode = false;
	Elm_Object_Item *it = ug_data->it;
	Item_Data *item_data = elm_object_item_data_get((const Elm_Object_Item *)it);
	int mailbox_id = item_data->mailbox_id;
	char *mailbox_name;
	char *mailbox_alias;

	bool generate_result = _generate_email_name_and_alias(ug_data, &mailbox_name, &mailbox_alias);
	if (ug_data->popup) {
		evas_object_del(ug_data->popup);
		ug_data->popup = NULL;
	}
	if (!generate_result) {
		debug_log("generating email and alias error occured");
		return;
	}

	if (_check_folder_name_exists(ug_data, mailbox_name)) {
		free(mailbox_alias);
		free(mailbox_name);
		debug_log("email mailbox with such name already exists");
		_popup_fail_cb(ug_data, NULL, NULL, EMAIL_ERROR_ALREADY_EXISTS);
		return;
	}

	account_sync_cancel_all(ug_data);
	int handle;
	int on_server = (EMAIL_SERVER_TYPE_POP3 == GET_ACCOUNT_SERVER_TYPE(ug_data->account_id) ? 0 : 1);
	debug_secure("name[%s], alias[%s], on_server[%d]", mailbox_name, mailbox_alias, on_server);

	int err_code = email_rename_mailbox(mailbox_id, mailbox_name, mailbox_alias, on_server, &handle);
	if (err_code < 0) {
		debug_log("email_rename_mailbox failed");
		_popup_fail_cb(ug_data, obj, event_info, err_code);
	} else {
		debug_log("email_rename_mailbox succeed : handle[%d]", handle);
		ug_data->emf_handle = handle;
		ug_data->target_mailbox_id = mailbox_id;
		_popup_progress_cb(ug_data, obj, event_info);
	}
	free(mailbox_alias);
	free(mailbox_name);
}

static void _create_folder_ok_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	RETURN_IF_FAIL(data != NULL);
	EmailAccountUGD *ug_data = (EmailAccountUGD *)data;

	ug_data->editmode = false;
	char *mailbox_name;
	char *mailbox_alias;
	bool generate_result = _generate_email_name_and_alias(ug_data, &mailbox_name, &mailbox_alias);
	if (ug_data->popup) {
		evas_object_del(ug_data->popup);
		ug_data->popup = NULL;
	}
	if (!generate_result) {
		debug_log("generating email and alias error occured");
		return;
	}

	if (_check_folder_name_exists(ug_data, mailbox_name)) {
		free(mailbox_alias);
		free(mailbox_name);
		debug_log("email mailbox with such name already exists");
		_popup_fail_cb(ug_data, NULL, NULL, EMAIL_ERROR_ALREADY_EXISTS);
		return;
	}

	account_sync_cancel_all(ug_data);
	int handle;
	int on_server = (EMAIL_SERVER_TYPE_POP3 == GET_ACCOUNT_SERVER_TYPE(ug_data->account_id) ? 0 : 1);
	debug_secure("name[%s], alias[%s], on_server[%d]", mailbox_name, mailbox_alias, on_server);

	email_mailbox_t mbox;
	memset(&mbox, 0x00, sizeof(email_mailbox_t));
	mbox.mailbox_name = mailbox_name;
	mbox.alias = mailbox_alias;
	mbox.local = 0;
	mbox.mailbox_type = EMAIL_MAILBOX_TYPE_USER_DEFINED;
	mbox.account_id = ug_data->account_id;
	int err_code = email_add_mailbox(&mbox, on_server, &handle);

	if (err_code < 0) {
		debug_log("email_add_mailbox failed");
		_popup_fail_cb(ug_data, obj, event_info, err_code);
	} else {
		debug_log("email_add_mailbox succeed : handle[%d]\n", handle);
		ug_data->emf_handle = handle;
		_popup_progress_cb(ug_data, obj, event_info);
	}
	free(mailbox_alias);
	free(mailbox_name);
}

static void _popup_newfolder_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	RETURN_IF_FAIL(data != NULL);

	EmailAccountUGD *ug_data = (EmailAccountUGD *)data;

	email_account_string_t EMAIL_ACCOUNT_HEADER_CREATE_FOLDER = { PACKAGE, "IDS_EMAIL_OPT_CREATE_FOLDER_ABB2"};
	account_create_entry_popup(ug_data, EMAIL_ACCOUNT_HEADER_CREATE_FOLDER, NULL, NULL,
			_create_folder_popup_cancel_cb, _mouseup_cb, _create_folder_ok_cb,
			_create_folder_popup_cancel_cb, "IDS_EMAIL_BUTTON_CANCEL",
			_create_folder_ok_cb, "IDS_EMAIL_BUTTON_CREATE_ABB2");
}

static void _create_folder_popup_cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	RETURN_IF_FAIL(data != NULL);

	EmailAccountUGD *ug_data = (EmailAccountUGD *) data;

	if (ug_data->popup) {
		evas_object_del(ug_data->popup);
		ug_data->popup = NULL;
	}
	ug_data->it = NULL;
	ug_data->target_mailbox_id = -1;
	ug_data->editmode = false;

	account_create_folder_create_view(ug_data);
}

static void _back_button_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	RETURN_IF_FAIL(data != NULL);

	EmailAccountUGD *ug_data = (EmailAccountUGD *) data;

	if (ug_data->popup) {
		evas_object_del(ug_data->popup);
		ug_data->popup = NULL;
	}
	FREE(ug_data->original_folder_name);
	ug_data->editmode = false;

	switch (ug_data->folder_mode) {
	case ACC_FOLDER_CREATE:
		account_create_folder_create_view(ug_data);
		break;
	case ACC_FOLDER_DELETE:
		account_delete_folder_view(ug_data);
		ug_data->folder_mode = ACC_FOLDER_NONE;
		break;
	case ACC_FOLDER_RENAME:
		account_rename_folder_view(ug_data);
		ug_data->folder_mode = ACC_FOLDER_NONE;
		break;
	default:
		debug_log("Warning...");
		break;
	}

}

static void _popup_destroy_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	RETURN_IF_FAIL(data != NULL);

	EmailAccountUGD *ug_data = (EmailAccountUGD *) data;

	if (ug_data->popup) {
		evas_object_del(ug_data->popup);
		ug_data->popup = NULL;
	}
}

static void _popup_success_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	RETURN_IF_FAIL(data != NULL);

	EmailAccountUGD *ug_data = (EmailAccountUGD *)data;
	int ret = 1;

	DELETE_EVAS_OBJECT(ug_data->popup);

	if (ug_data->folder_mode == ACC_FOLDER_NONE) {
		debug_log("exit func without popup");
		if (EMAIL_SERVER_TYPE_ACTIVE_SYNC == GET_ACCOUNT_SERVER_TYPE(ug_data->account_id)) {
			debug_log("Success notification is arrived after cancelling");
			_finish_folder_view(data, NULL, NULL);
		}
		return;
	}
	switch (ug_data->folder_mode) {
		case ACC_FOLDER_CREATE:
			debug_log("Folder created.");
#ifdef _SHOW_CONFIRM_GUIDE_POPUP_
			ret = notification_status_message_post(email_get_email_string("IDS_EMAIL_TPOP_FOLDER_CREATED")); /* Not show this toast popup for confirm guide according to popup simplification_2014.10.31 */
#endif
			break;
		case ACC_FOLDER_DELETE:
			debug_log("Folder deleted.");
#ifdef _SHOW_CONFIRM_GUIDE_POPUP_
			ret = notification_status_message_post(email_get_email_string("IDS_EMAIL_TPOP_FOLDER_DELETED_ABB")); /* Not show this confirm toast popup for confirm guide according to popup simplification_2014.10.31 */
#endif
			break;
		case ACC_FOLDER_RENAME:
			debug_log("Folder Renamed.");
#ifdef _SHOW_CONFIRM_GUIDE_POPUP_

			ret = notification_status_message_post(email_get_email_string("IDS_EMAIL_TPOP_FOLDER_RENAMED"));
#endif
		break;

		default:
			debug_log("Warning...");
			break;
	}
	debug_warning_if(ret != NOTIFICATION_ERROR_NONE, "notification_status_message_post() failed! ret:(%d)", ret);
}

static void _popup_fail_cb(void *data, Evas_Object *obj, void *event_info, int err_code)
{
	debug_enter();
	RETURN_IF_FAIL(data != NULL);

	EmailAccountUGD *ug_data = (EmailAccountUGD *)data;
	int ret = 1;

	if (ug_data->popup) {
		evas_object_del(ug_data->popup);
		ug_data->popup = NULL;
	}

	debug_log("emf_handle [%d], folder_mode[%d]", ug_data->emf_handle, ug_data->folder_mode);
	ug_data->emf_handle = EMAIL_HANDLE_INVALID;

	if (ug_data->folder_mode == ACC_FOLDER_NONE) {
		debug_log("exit func without popup");
		return;
	}
	/* If we take care of many different kinds error cases, then this code should be improved. */
	if (err_code == EMAIL_ERROR_SERVER_NOT_SUPPORT_FUNCTION) {
		ret = notification_status_message_post(email_get_email_string("IDS_EMAIL_TPOP_SERVER_ERROR_OCCURRED_ABB"));
	} else if (err_code == EMAIL_ERROR_ALREADY_EXISTS) {
		ret = notification_status_message_post(email_get_email_string("IDS_EMAIL_TPOP_FOLDER_NAME_ALREADY_IN_USE_ABB"));
	} else {
		switch (ug_data->folder_mode) {
		case ACC_FOLDER_CREATE:
			ret = notification_status_message_post(email_get_email_string("IDS_EMAIL_TPOP_SERVER_ERROR_OCCURRED_ABB"));
			break;
		case ACC_FOLDER_DELETE:
			ret = notification_status_message_post(email_get_email_string("IDS_EMAIL_TPOP_SERVER_ERROR_OCCURRED_ABB"));
			break;
		case ACC_FOLDER_RENAME:
			ret = notification_status_message_post(email_get_email_string("IDS_EMAIL_TPOP_SERVER_ERROR_OCCURRED_ABB"));
			break;
		default:
			debug_log("Warning...");
			break;
		}
	}
	if (ret != NOTIFICATION_ERROR_NONE) {
		debug_log("fail to notification_status_message_post() : %d\n", ret);
	}
	_back_button_cb(ug_data, NULL, NULL);
}

static void _popup_progress_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	RETURN_IF_FAIL(data != NULL);

	Evas_Object *popup, *layout;
	Evas_Object *progressbar;
	EmailAccountUGD *ug_data = (EmailAccountUGD *)data;

	if (ug_data->popup) {
		evas_object_del(ug_data->popup);
		ug_data->popup = NULL;
	}

	popup = elm_popup_add(ug_data->base.module->navi);
	elm_popup_align_set(popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	layout = elm_layout_add(popup);
	elm_layout_file_set(layout, email_get_common_theme_path(), "processing_view");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);

	switch (ug_data->folder_mode) {
		case ACC_FOLDER_CREATE:
			elm_object_domain_translatable_part_text_set(layout, "elm.text", PACKAGE, "IDS_EMAIL_TPOP_CREATING_FOLDER_ING");
			break;
		case ACC_FOLDER_DELETE:
			elm_object_domain_translatable_part_text_set(layout, "elm.text", PACKAGE, "IDS_EMAIL_TPOP_DELETING_FOLDER_ING");
			break;
		case ACC_FOLDER_RENAME:
			elm_object_domain_translatable_part_text_set(layout, "elm.text", PACKAGE, "IDS_EMAIL_POP_RENAMING_FOLDER_ING");
			break;
		default:
			elm_object_domain_translatable_part_text_set(layout, "elm.text", PACKAGE, email_get_email_string("IDS_ST_HEADER_WARNING"));
			break;
	}

	progressbar = elm_progressbar_add(popup);
	elm_object_style_set(progressbar, "process_medium");
	evas_object_size_hint_align_set(progressbar, EVAS_HINT_FILL, 0.5);
	evas_object_size_hint_weight_set(progressbar, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(progressbar);
	elm_progressbar_pulse(progressbar, EINA_TRUE);
	elm_object_part_content_set(layout, "processing", progressbar);
	elm_object_content_set(popup, layout);

	ug_data->popup = popup;
	evas_object_show(popup);
}

static void _mouseup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Event_Mouse_Up *ev = event_info;
	if (ev->button == 3) { /* if mouse right button is up */
		_popup_destroy_cb(data, obj, NULL);
	}
}

static void _folder_popup_mouseup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Event_Mouse_Up *ev = event_info;
	if (ev->button == 3) { /* if mouse right button is up */
		_back_button_cb(data, obj, NULL);
	}
}

/*
static void _folder_progress_popup_mouseup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Event_Mouse_Up *ev = event_info;
	if (ev->button == 3) // if mouse right button is up
	{
		_cancel_button_cb(data, obj, NULL);
	}
}
*/

static void _add_root_item_in_genlist(void *data)
{
	debug_enter();
	RETURN_IF_FAIL(data != NULL);

	EmailAccountUGD *ug_data = (EmailAccountUGD *)data;

	Item_Data *next_data = (Item_Data *)elm_object_item_data_get(elm_genlist_first_item_get(ug_data->gl));
	if (next_data == NULL) {
		debug_log("next_data is NULL");
		return;
	}
	Item_Data *item_data = calloc(1, sizeof(Item_Data));
	retm_if(!item_data, "item_data is NULL!");
	item_data->ug_data = ug_data;
	item_data->it = elm_genlist_item_prepend(ug_data->gl, ug_data->itc_root,
			item_data, NULL, ELM_GENLIST_ITEM_NONE, _gl_root_item_sel, item_data);
	ug_data->root_item = item_data->it;
	debug_leave();
}

static void _gl_root_item_del(void *data, Evas_Object *obj)
{
	Item_Data *item_data = (Item_Data *)data;
	if (!item_data) {
		debug_log("item_data is NULL");
		return;
	}
	EmailAccountUGD *ug_data = item_data->ug_data;
	if (ug_data) {
		ug_data->root_item = NULL;
		debug_log("Reset root_item");
	}
	FREE(item_data);
}

static char *_gl_label_get_for_root_item(void *data, Evas_Object *obj, const char *part)
{
	if (!strcmp(part, "elm.text")) {
		return strdup(N_("Root"));
	}
	return NULL;
}


static void _gl_root_item_sel(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	RETURN_IF_FAIL(data != NULL);

	Item_Data *item_data = (Item_Data *)data;
	EmailAccountUGD *ug_data = item_data->ug_data;

	elm_genlist_item_selected_set(item_data->it, 0);

	if (ACC_FOLDER_CREATE == ug_data->folder_mode) {
		ug_data->it = item_data->it;
		_popup_newfolder_cb(ug_data, obj, event_info);
	}
}

void account_folder_newfolder(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	RETURN_IF_FAIL(data != NULL);

	EmailAccountUGD *ug_data = (EmailAccountUGD *)data;

	email_account_string_t EMAIL_ACCOUNT_HEADER_CREATE_FOLDER = { PACKAGE, "IDS_EMAIL_OPT_CREATE_FOLDER_ABB2"};
	account_create_entry_popup(ug_data, EMAIL_ACCOUNT_HEADER_CREATE_FOLDER, NULL, NULL,
			_create_folder_popup_cancel_cb, _mouseup_cb, _create_folder_ok_cb,
			_create_folder_popup_cancel_cb, "IDS_EMAIL_BUTTON_CANCEL",
			_create_folder_ok_cb, "IDS_EMAIL_BUTTON_CREATE_ABB2");
}

/* EOF */
