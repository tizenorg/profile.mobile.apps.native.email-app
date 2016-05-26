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

#include "email-account.h"
#include "email-account-util.h"
#include "email-account-folder-move.h"

static char *_gl_label_get(void *data, Evas_Object *obj, const char *part);
static char *_gl_label_get_for_subitem(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_gl_folder_icon_get_for_subitem(void *data, Evas_Object *obj, const char *part);

static void _gl_group_sel(void *data, Evas_Object *obj, void *event_info);
static void _gl_group_del(void *data, Evas_Object *obj);

static void _gl_sel(void *data, Evas_Object *obj, void *event_info);
static void _send_result_to_mailbox_module_for_move(EmailAccountView *view);
static Evas_Object *_gl_icon_get(void *data, Evas_Object *obj, const char *part);
static void _account_list_for_mail_move(EmailAccountView *view, int account_index, bool hidden);

typedef struct _List_Item_Data {
	EmailAccountView *view;
	email_move_list *move_folder;

	Elm_Object_Item *it; /* Genlist Item pointer */

	char *mailbox_name;
	email_mailbox_type_e mailbox_type;
	char *alias_name;
	int mailbox_id;
	Eina_Bool expanded;
	int account_id;
	Eina_List *sub_items;
} List_Item_Data;

static void _gl_group_sel(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(event_info == NULL, "Invalid parameter: event_info[NULL]");

	Elm_Object_Item *it = (Elm_Object_Item *)event_info;
	elm_genlist_item_selected_set(it, EINA_FALSE);

	List_Item_Data *group_item_data = (List_Item_Data *)elm_object_item_data_get(it);
	List_Item_Data *sub_item_data = NULL;
	EmailAccountView *view = (EmailAccountView *)group_item_data->view;

	if (group_item_data->expanded) {
		elm_genlist_item_subitems_clear(group_item_data->it);
	} else {
		Eina_List *l = NULL;
		EINA_LIST_FOREACH(group_item_data->sub_items, l, sub_item_data) {
			sub_item_data->it = elm_genlist_item_append(view->gl, view->itc_move, sub_item_data, group_item_data->it, ELM_GENLIST_ITEM_NONE, _gl_sel, sub_item_data);
		}
		elm_genlist_item_bring_in(group_item_data->it, ELM_GENLIST_ITEM_SCROLLTO_TOP);
	}

	group_item_data->expanded = !group_item_data->expanded;
	elm_genlist_item_fields_update(group_item_data->it, "elm.swallow.end", ELM_GENLIST_ITEM_FIELD_CONTENT);

	debug_leave();
}

static void _account_list_for_mail_move(EmailAccountView *view, int account_index, bool expanded)
{
	debug_enter();
	RETURN_IF_FAIL(view != NULL);

	Evas_Object *gl = view->gl;
	List_Item_Data *tree_item_data = NULL, *group_tree_item_data = NULL;
	int mailbox_list_count = 0;
	int j = 0;
	email_mailbox_t *mailbox_list = NULL;
	int i = account_index;

	mailbox_list_count = email_engine_get_mailbox_list_with_mail_count(
			view->account_list[i].account_id, &mailbox_list);
	if (mailbox_list_count <= 0) {
		debug_critical("email_engine_get_mailbox_list_with_mail_count return error");
	}

	view->move_list[i].account_info = &(view->account_list[i]);
	view->move_list[i].mailbox_list = mailbox_list;
	view->move_list[i].mailbox_cnt = mailbox_list_count;
	view->move_list[i].move_view_mode = view->move_mode;

	tree_item_data = calloc(1, sizeof(List_Item_Data));
	if (!tree_item_data) {
		email_engine_free_mailbox_list(&mailbox_list, mailbox_list_count);
		debug_error("tree_item_data is NULL - allocation memory failed");
		return;
	}
	group_tree_item_data = tree_item_data;
	group_tree_item_data->view = view;
	group_tree_item_data->move_folder = &view->move_list[i];
	group_tree_item_data->expanded = expanded;
	group_tree_item_data->it = elm_genlist_item_append(gl, view->itc_group_move, group_tree_item_data, NULL, ELM_GENLIST_ITEM_NONE, _gl_group_sel, group_tree_item_data);

	for (j = 0; j < mailbox_list_count; j++) {
		if (g_strcmp0(mailbox_list[j].mailbox_name, "[Gmail]") && (0 == mailbox_list[j].no_select)) {
			if (mailbox_list[j].mailbox_type != EMAIL_MAILBOX_TYPE_OUTBOX
				&& mailbox_list[j].mailbox_type != EMAIL_MAILBOX_TYPE_SENTBOX
				&& mailbox_list[j].mailbox_type != EMAIL_MAILBOX_TYPE_DRAFT
				&& mailbox_list[j].mailbox_type != EMAIL_MAILBOX_TYPE_ALL_EMAILS
				&& mailbox_list[j].mailbox_type != EMAIL_MAILBOX_TYPE_SEARCH_RESULT
				&& view->folder_id != mailbox_list[j].mailbox_id) {
				tree_item_data = calloc(1, sizeof(List_Item_Data));
				if (!tree_item_data) {
					email_engine_free_mailbox_list(&mailbox_list, mailbox_list_count);
					debug_error("tree_item_data is NULL - allocation memory failed");
					return;
				}
				tree_item_data->view = view;
				tree_item_data->mailbox_name = g_strdup(mailbox_list[j].mailbox_name);
				tree_item_data->mailbox_type = mailbox_list[j].mailbox_type;
				tree_item_data->alias_name = g_strdup(mailbox_list[j].alias);
				tree_item_data->mailbox_id = mailbox_list[j].mailbox_id;
				tree_item_data->account_id = mailbox_list[j].account_id;
				tree_item_data->move_folder = &view->move_list[i];
				if (expanded) {
					tree_item_data->it = elm_genlist_item_append(gl, view->itc_move, tree_item_data, group_tree_item_data->it, ELM_GENLIST_ITEM_NONE, _gl_sel, tree_item_data);
				}
				group_tree_item_data->sub_items = eina_list_append(group_tree_item_data->sub_items, tree_item_data);
			}
		}
	}

	debug_leave();
}

int account_create_folder_list_for_mail_move(EmailAccountView *view)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(view != NULL, 0);

	int i = 0;

	email_move_list *move_list = (email_move_list *)calloc(view->account_count, sizeof(email_move_list));
	retvm_if(move_list == NULL, 0, "Invalid parameter: move_list[NULL]");

	view->move_list = move_list;

	int inserted_account_id[view->account_count];
	memset(inserted_account_id, 0, sizeof(inserted_account_id));

	if (EMAIL_SERVER_TYPE_IMAP4 == GET_ACCOUNT_SERVER_TYPE(view->account_id)) {
		for (i = 0; i < view->account_count; i++) {
			if (view->account_list[i].account_id == view->account_id) {
				inserted_account_id[i] = 1;
				_account_list_for_mail_move(view, i, EINA_TRUE);
				view->inserted_account_cnt++;
			}
		}

		for (i = 0; i < view->account_count; i++) {
			if (inserted_account_id[i] != 1 && EMAIL_SERVER_TYPE_IMAP4 == view->account_list[i].incoming_server_type) {
				inserted_account_id[i] = 1;
				_account_list_for_mail_move(view, i, EINA_FALSE);
				view->inserted_account_cnt++;
			}
		}
	} else {
		for (i = 0; i < view->account_count; i++) {
			if (view->account_list[i].account_id == view->account_id) {
				inserted_account_id[i] = 1;
				_account_list_for_mail_move(view, i, EINA_TRUE);
				view->inserted_account_cnt++;
			}
		}
	}
	debug_log("view->inserted_account_cnt : %d", view->inserted_account_cnt);
	return 0;
}

static Evas_Object *_gl_icon_get(void *data, Evas_Object *obj, const char *part)
{
	if (!data) {
		debug_log("data is NULL");
		return NULL;
	}
	List_Item_Data *tree_item_data = (List_Item_Data *)data;

	if (!g_strcmp0(part, "elm.swallow.end")) {
		if (tree_item_data->view && tree_item_data->view->inserted_account_cnt > 1) {

			Evas_Object *expand_icon = elm_layout_add(obj);
			if (tree_item_data->expanded) {
				elm_layout_file_set(expand_icon, email_get_common_theme_path(), EMAIL_IMAGE_CORE_EXPAND_OPENED);
			} else {
				elm_layout_file_set(expand_icon, email_get_common_theme_path(), EMAIL_IMAGE_CORE_EXPAND_CLOSED);
			}
			evas_object_show(expand_icon);
			return expand_icon;
		}
	}

	return NULL;
}

void account_init_genlist_item_class_for_mail_move(EmailAccountView *view)
{
	debug_enter();
	RETURN_IF_FAIL(view != NULL);

	Elm_Genlist_Item_Class *itc_group_move = elm_genlist_item_class_new();
	retm_if(itc_group_move == NULL, "itc_group_move is NULL!");
	itc_group_move->item_style = "group_index";
	itc_group_move->func.text_get = _gl_label_get;
	itc_group_move->func.content_get = _gl_icon_get;
	itc_group_move->func.state_get = NULL;
	itc_group_move->func.del = _gl_group_del;
	view->itc_group_move = itc_group_move;

	Elm_Genlist_Item_Class *itc_move = elm_genlist_item_class_new();
	if (!itc_move){
		debug_error("itc_move is NULL - allocation memory failed");
		EMAIL_GENLIST_ITC_FREE(view->itc_group_move);
		return;
	}
	itc_move->item_style = "type1";
	itc_move->func.text_get = _gl_label_get_for_subitem;
	itc_move->func.content_get = _gl_folder_icon_get_for_subitem;
	itc_move->func.state_get = NULL;
	itc_move->func.del = NULL;
	view->itc_move = itc_move;
}

static char *_gl_label_get(void *data, Evas_Object *obj, const char *part)
{
	List_Item_Data *tree_item_data = (List_Item_Data *)data;

	if (!tree_item_data) {
		debug_log("tree_item_data is NULL");
		return NULL;
	}
	email_move_list *move_folder = tree_item_data->move_folder;

	if (move_folder == NULL) {
		debug_log("move_folder is NULL !!");
		return NULL;
	}

	if (!strcmp(part, "elm.text")) {
		if (move_folder->account_info && move_folder->account_info->user_email_address) {
			return g_strdup(move_folder->account_info->user_email_address);
		}
	}

	return NULL;
}

static char *_gl_label_get_for_subitem(void *data, Evas_Object *obj, const char *part)
{
	if (!strcmp(part, "elm.text")) {

		List_Item_Data *tree_item_data = (List_Item_Data *)data;

		if (!tree_item_data) {
			debug_log("tree_item_data is NULL");
			return NULL;
		}
		EmailAccountView *view = tree_item_data->view;

		char *mailbox_alias = NULL;
		switch (tree_item_data->mailbox_type) {
		case EMAIL_MAILBOX_TYPE_INBOX:
			mailbox_alias = _("IDS_EMAIL_HEADER_INBOX");
			break;
		case EMAIL_MAILBOX_TYPE_DRAFT:
			mailbox_alias = _("IDS_EMAIL_HEADER_DRAFTS");
			break;
		case EMAIL_MAILBOX_TYPE_OUTBOX:
			mailbox_alias = _("IDS_EMAIL_HEADER_OUTBOX");
			break;
		case EMAIL_MAILBOX_TYPE_SENTBOX:
			mailbox_alias = _("IDS_EMAIL_HEADER_SENT_M_EMAIL");
			break;
		case EMAIL_MAILBOX_TYPE_SPAMBOX:
			mailbox_alias = _("IDS_EMAIL_HEADER_SPAMBOX");
			break;
		case EMAIL_MAILBOX_TYPE_TRASH:
			mailbox_alias = _("IDS_EMAIL_HEADER_RECYCLE_BIN_ABB");
			break;

		default:
			if (EMAIL_SERVER_TYPE_ACTIVE_SYNC == GET_ACCOUNT_SERVER_TYPE(tree_item_data->account_id)) {
				mailbox_alias = g_strdup(tree_item_data->mailbox_name);
			} else {
				mailbox_alias = account_util_convert_mutf7_to_utf8(tree_item_data->mailbox_name);
			}
			char *filename = account_get_ellipsised_folder_name(view, mailbox_alias);
			G_FREE(mailbox_alias);
			return filename;
			break;
		}
		return elm_entry_utf8_to_markup(mailbox_alias);
	}
	return NULL;
}

Evas_Object *_gl_folder_icon_get_for_subitem(void *data, Evas_Object *obj, const char *part)
{
	if (!strcmp(part, "elm.swallow.icon")) {
		List_Item_Data *tree_item_data = (List_Item_Data *)data;

		if (!tree_item_data) {
			debug_log("tree_item_data is NULL");
			return NULL;
		}

		int mailbox_type = tree_item_data->mailbox_type;
		char *folder_icon_name = account_get_folder_icon_name_by_mailbox_type(mailbox_type);
		return account_create_folder_icon(obj, folder_icon_name);
	}

	return NULL;
}

static void account_item_del(List_Item_Data *item_data)
{
	if (item_data) {

		G_FREE(item_data->mailbox_name);
		G_FREE(item_data->alias_name);

		if (item_data->sub_items) {
			Eina_List *l = NULL;
			List_Item_Data *sub_item_data = NULL;
			EINA_LIST_FOREACH(item_data->sub_items, l, sub_item_data) {
				account_item_del(sub_item_data);
			}
			eina_list_free(item_data->sub_items);
		}

		FREE(item_data);
	}
}

static void _gl_group_del(void *data, Evas_Object *obj)
{
	account_item_del((List_Item_Data *)data);
}

static void _send_result_to_mailbox_module_for_move(EmailAccountView *view)
{
	debug_enter();

	RETURN_IF_FAIL(view != NULL);

	email_params_h params = NULL;

	if (email_params_create(&params) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_IS_MAILBOX_EDIT_MODE, view->b_editmode) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_MAILBOX, view->folder_id) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_MAILBOX_MOVE_STATUS, view->move_status) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_IS_MAILBOX_MOVE_MODE, 1) &&
		(!view->moved_mailbox_name ||
		email_params_add_str(params, EMAIL_BUNDLE_KEY_MAILBOX_MOVED_MAILBOX_NAME, view->moved_mailbox_name))) {

		email_module_send_result(view->base.module, params);
	}

	email_params_free(&params);
}

static void _gl_sel(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	if (data == NULL) {
		debug_log("data is NULL");
		return;
	}
	if (event_info == NULL) {
		debug_log("event_info is NULL");
		return;
	}
	int ret = 1;

	List_Item_Data *item_data = (List_Item_Data *)data;
	email_move_list *move_list = item_data->move_folder;
	Elm_Object_Item *it = event_info;

	EmailAccountView *view = item_data->view;
	int err = 0;
	email_mail_list_item_t *mail_info = NULL;

	GList *first = g_list_first(view->selected_mail_list_to_move);

	elm_genlist_item_selected_set(it, EINA_FALSE);
	debug_log("selected account_id : %d, selected mailbox_id : %d", move_list->account_info->account_id, item_data->mailbox_id);

	if (!first) {
		debug_warning("There's no selected mail.");
		goto FINISH;
	}

	if (move_list->move_view_mode == EMAIL_MOVE_VIEW_NORMAL) {
		int src_mailbox_id = 0;

		mail_info = email_engine_get_mail_by_mailid((int)(intptr_t)first->data);
		if (!mail_info) {
			debug_log("email_engine_get_mail_by_mailid is failed(mail_id : %d)", (int)(intptr_t)first->data);
			goto FINISH;
		}

		if (mail_info->account_id != move_list->account_info->account_id) {
			src_mailbox_id = mail_info->mailbox_id;
		}

		err = email_engine_move_mail_list(item_data->mailbox_id, view->selected_mail_list_to_move, src_mailbox_id);
		if (err != EMAIL_ERROR_NONE) {
			debug_warning("email_engine_move_mail_list src mailbox(%d), dest acc(%d) mailbox_id(%d) - err (%d)",
					src_mailbox_id, move_list->account_info->account_id, item_data->mailbox_id, err);
			ret = notification_status_message_post(email_get_email_string("IDS_EMAIL_POP_FAILED_TO_MOVE"));
		}

		if (ret != NOTIFICATION_ERROR_NONE) {
			debug_log("fail to notification_status_message_post() : %d\n", ret);
		}

		FREE(mail_info);
	}

	debug_log("move status is %d, %d", err, EMAIL_ERROR_NONE);
	view->move_status = err;

	G_FREE(view->moved_mailbox_name);

	if (item_data->alias_name) {
		char *alias = account_convert_folder_alias_by_mailbox_type(item_data->mailbox_type);
		if (alias) {
			view->moved_mailbox_name = g_strdup(alias);
			FREE(alias);
		} else {
			view->moved_mailbox_name = g_strdup(item_data->alias_name);
		}
	}

	FINISH:
	_send_result_to_mailbox_module_for_move(view);

}

