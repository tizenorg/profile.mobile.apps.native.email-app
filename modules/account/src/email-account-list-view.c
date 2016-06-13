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
#include "email-account-list-view.h"
#include "email-account-folder.h"

typedef struct {
	EmailAccountView *view;
	int item_type;
	int account_id;
	int mailbox_id;
	int unread_cnt;
	int total_cnt;
	Eina_Bool expanded;
	int group_order;
	char *account_name;
	Elm_Object_Item *it;
	Eina_List *sub_items;
} Account_Item_Data;

/*
 * Below account list item type order is very important.
 * If below account list item type order is changed, all account list item creating/updating routine should be checked.
 * Do not change enum(account list item type) sorting.
 */
typedef enum {
	ACCOUNT_LIST_NONE = 0,
	ACCOUNT_LIST_COMBINED_GROUP = 1,
	ACCOUNT_LIST_COMBINED_INBOX,
#ifndef _FEATURE_PRIORITY_SENDER_DISABLE_
	ACCOUNT_LIST_PRIORITY_INBOX,
#endif
	ACCOUNT_LIST_STARRED,
	ACCOUNT_LIST_COMBINED_DRAFTS,
	ACCOUNT_LIST_COMBINED_OUTBOX,
	ACCOUNT_LIST_COMBINED_SENT,
	ACCOUNT_LIST_COMBINED_SHOW_ALL_FOLDERS,
	ACCOUNT_LIST_COMBINED_MAX,

	ACCOUNT_LIST_SINGLE_GROUP, /* 12 */
	ACCOUNT_LIST_SINGLE_INBOX,
	ACCOUNT_LIST_SINGLE_DRAFTS,
	ACCOUNT_LIST_SINGLE_OUTBOX,
	ACCOUNT_LIST_SINGLE_SENT,
	ACCOUNT_LIST_SINGLE_SHOW_ALL_FOLDERS,
	ACCOUNT_LIST_SINGLE_MAX,
} account_list_item_type;

typedef enum {
	ACCOUNT_LIST_SOME_GROUP_EXPANEDED = 0,
	ACCOUNT_LIST_ALL_GROUP_EXPANDED,
	ACCOUNT_LIST_ALL_GROUP_CONTRACTED,
} account_list_group_state;

static int _create_account_all_account_list(EmailAccountView *view);
static void _create_account_single_accout_list(EmailAccountView *view, int account_id, int group_order);

static char *_gl_label_get_for_account_list_group_item(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_gl_icon_get_for_account_list_group_item(void *data, Evas_Object *obj, const char *part);
static void _gl_account_list_item_del(void *data, Evas_Object *obj);
static void _gl_account_list_item_sel(void *data, Evas_Object *obj, void *event_info);
static void _gl_account_group_list_item_sel(void *data, Evas_Object *obj, void *event_info);
static Evas_Object *_gl_account_content_get_for_single_subitem(void *data, Evas_Object *obj, const char *part);
static char *_get_account_item_text_for_single_subitem(Account_Item_Data *item_data);

static void _check_account_list_zoom_state(EmailAccountView *view);
static int _convert_account_list_item_type(EmailAccountView *view, int account_id, int mailbox_type);
static char *_create_account_list_item_text(Account_Item_Data *item_data, const char *text);
static Evas_Object *_create_account_subitem_color_bar(Evas_Object *parent, unsigned int color);
static Evas_Object *_create_account_subitem_folder_icon(Evas_Object *parent, account_list_item_type item_type);

int account_create_account_list_view(EmailAccountView *view)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(view != NULL, 0);
	int inserted_cnt = 0;

	G_LIST_FREE(view->account_group_list);

	if (!email_engine_get_default_account(&(view->default_account_id))) {
		debug_log("email_engine_get_default_account failed");
	}

	inserted_cnt = _create_account_all_account_list(view);

	debug_log("account_create_account_list_view %d", inserted_cnt);
	return inserted_cnt;
}

static int _create_account_all_account_list(EmailAccountView *view)
{
	debug_enter();
	retvm_if(!view, 0, "EmailAccountView is NULL");

	int i = 0;
	int total_count = 0, unread_count = 0;
	int inserted_cnt = 0;

	int all_acc_total_count[ACCOUNT_LIST_COMBINED_MAX];
	int all_acc_unread_count[ACCOUNT_LIST_COMBINED_MAX];
	bool all_acc_is_inserted[ACCOUNT_LIST_COMBINED_MAX];

	memset(all_acc_unread_count, 0, sizeof(all_acc_unread_count));
	memset(all_acc_total_count, 0, sizeof(all_acc_total_count));
	memset(all_acc_is_inserted, 0, sizeof(all_acc_is_inserted));

	Evas_Object *gl = view->gl;
	Account_Item_Data *item_data = NULL, *group_item_data = NULL;

	if (!view->account_list) {
			debug_warning("Email account list is NULL");
	} else {

		int index = ACCOUNT_LIST_COMBINED_INBOX;

		for (index = ACCOUNT_LIST_COMBINED_INBOX; index < ACCOUNT_LIST_COMBINED_MAX; index++) {
			switch (index) {
			case ACCOUNT_LIST_COMBINED_INBOX:
			{
				if (email_get_combined_mail_count_by_mailbox_type(EMAIL_MAILBOX_TYPE_INBOX, &total_count, &unread_count)) {
					all_acc_total_count[ACCOUNT_LIST_COMBINED_INBOX] = total_count;
					all_acc_unread_count[ACCOUNT_LIST_COMBINED_INBOX] = unread_count;
				}
				all_acc_is_inserted[ACCOUNT_LIST_COMBINED_INBOX] = true;
			}
			break;
	#ifndef _FEATURE_PRIORITY_SENDER_DISABLE_
			case ACCOUNT_LIST_PRIORITY_INBOX:
			{
				if (email_get_priority_sender_mail_count_ex(&total_count, &unread_count)) {
					all_acc_total_count[ACCOUNT_LIST_PRIORITY_INBOX] = total_count;
					all_acc_unread_count[ACCOUNT_LIST_PRIORITY_INBOX] = unread_count;
				}
				all_acc_is_inserted[ACCOUNT_LIST_PRIORITY_INBOX] = true;
			}
			break;
	#endif

			case ACCOUNT_LIST_STARRED:
			{
				if (email_get_favourite_mail_count_ex(&total_count, &unread_count)) {
					all_acc_total_count[ACCOUNT_LIST_STARRED] = total_count;
					all_acc_unread_count[ACCOUNT_LIST_STARRED] = unread_count;
					if (total_count > 0)
						all_acc_is_inserted[ACCOUNT_LIST_STARRED] = true;
				}
			}
			break;

			case ACCOUNT_LIST_COMBINED_DRAFTS:
			{
				if (email_get_combined_mail_count_by_mailbox_type(EMAIL_MAILBOX_TYPE_DRAFT, &total_count, &unread_count)) {
					all_acc_total_count[ACCOUNT_LIST_COMBINED_DRAFTS] = total_count;
					all_acc_unread_count[ACCOUNT_LIST_COMBINED_DRAFTS] = unread_count;
					if (total_count > 0)
						all_acc_is_inserted[ACCOUNT_LIST_COMBINED_DRAFTS] = true;
				}
			}
			break;

			case ACCOUNT_LIST_COMBINED_OUTBOX:
			{
				if (email_get_combined_mail_count_by_mailbox_type(EMAIL_MAILBOX_TYPE_OUTBOX, &total_count, &unread_count)) {
					all_acc_total_count[ACCOUNT_LIST_COMBINED_OUTBOX] = total_count;
					all_acc_unread_count[ACCOUNT_LIST_COMBINED_OUTBOX] = unread_count;
					if (total_count > 0)
						all_acc_is_inserted[ACCOUNT_LIST_COMBINED_OUTBOX] = true;
				}
			}
			break;

			case ACCOUNT_LIST_COMBINED_SENT:
			{
				if (email_get_combined_mail_count_by_mailbox_type(EMAIL_MAILBOX_TYPE_SENTBOX, &total_count, &unread_count)) {
					all_acc_total_count[ACCOUNT_LIST_COMBINED_SENT] = total_count;
					all_acc_unread_count[ACCOUNT_LIST_COMBINED_SENT] = unread_count;
				}
				all_acc_is_inserted[ACCOUNT_LIST_COMBINED_SENT] = true;
			}
			break;

			case ACCOUNT_LIST_COMBINED_SHOW_ALL_FOLDERS:
				all_acc_is_inserted[ACCOUNT_LIST_COMBINED_SHOW_ALL_FOLDERS] = true;
				break;

			}
		}
	}

	item_data = calloc(1, sizeof(Account_Item_Data));
	if (!item_data) {
		debug_error("tree_item_data is NULL - allocation memory failed");
		return 0;
	}
	group_item_data = item_data;
	group_item_data->view = view;
	group_item_data->item_type = ACCOUNT_LIST_COMBINED_GROUP;
	group_item_data->unread_cnt = all_acc_unread_count[ACCOUNT_LIST_COMBINED_INBOX];
	group_item_data->total_cnt = all_acc_total_count[ACCOUNT_LIST_COMBINED_INBOX];
	group_item_data->expanded = EINA_TRUE;
	group_item_data->group_order = 0;

	group_item_data->it = elm_genlist_item_append(gl, view->itc_account_group, group_item_data, NULL, ELM_GENLIST_ITEM_NONE, _gl_account_group_list_item_sel, group_item_data);
	view->account_group_list = g_list_append(view->account_group_list, group_item_data);

	for (i = ACCOUNT_LIST_COMBINED_INBOX; i < ACCOUNT_LIST_COMBINED_MAX; i++) {
		if (!all_acc_is_inserted[i])
			continue;

		item_data = calloc(1, sizeof(Account_Item_Data));
		if (!item_data) {
			debug_error("item_data is NULL - allocation memory failed");
			return inserted_cnt;
		}
		item_data->view = view;
		item_data->item_type = i;
		item_data->unread_cnt = all_acc_unread_count[i];
		item_data->total_cnt = all_acc_total_count[i];

		item_data->it = elm_genlist_item_append(gl, view->itc_account_item, item_data, group_item_data->it, ELM_GENLIST_ITEM_NONE, _gl_account_list_item_sel, item_data);
		debug_log("it : %p", item_data->it);

		group_item_data->sub_items = eina_list_append(group_item_data->sub_items, item_data);
		++inserted_cnt;
	}

	/* Add sing account list */
	if (view->account_list) {
		for (i = 0; i < view->account_count; i++) {
			_create_account_single_accout_list(view, view->account_list[i].account_id, i+1);
		}
	}

	debug_log("inserted_cnt : %d", inserted_cnt);
	return inserted_cnt;
}

static void _create_account_single_accout_list(EmailAccountView *view, int account_id, int group_order)
{
	debug_enter();
	retm_if(!view, "view is NULL");

	int i = 0;

	email_account_t *email_account = NULL;
	Account_Item_Data *item_data = NULL, *group_item_data = NULL;
	email_mailbox_t *mailbox = NULL;
	int scheduled_mail_cnt = 0;

	Evas_Object *gl = view->gl;

	if (!email_engine_get_account_data(account_id, EMAIL_ACC_GET_OPT_DEFAULT, &email_account)) {
		debug_error("email_engine_get_account_data() failed, account_name is NULL");
		return;
	}

	if (!email_engine_get_mailbox_by_mailbox_type(account_id, EMAIL_MAILBOX_TYPE_INBOX, &mailbox)) {

		item_data = calloc(1, sizeof(Account_Item_Data));
		group_item_data = item_data;
		group_item_data->view = view;
		group_item_data->item_type = ACCOUNT_LIST_SINGLE_GROUP;
		group_item_data->account_id = account_id;
		group_item_data->account_name = email_account->user_email_address ? g_strdup(email_account->user_email_address) : NULL;
		group_item_data->expanded = EINA_TRUE;
		group_item_data->group_order = group_order;

		group_item_data->it = elm_genlist_item_append(gl, view->itc_account_group, group_item_data, NULL, ELM_GENLIST_ITEM_NONE, _gl_account_list_item_sel, group_item_data);
		view->account_group_list = g_list_append(view->account_group_list, item_data);

		/* Insert Inbox item first. */
		item_data = calloc(1, sizeof(Account_Item_Data));
		item_data->view = view;
		item_data->account_id = account_id;
		item_data->item_type = ACCOUNT_LIST_SINGLE_INBOX;
		item_data->it = elm_genlist_item_append(gl, view->itc_account_item, item_data, group_item_data->it,
				ELM_GENLIST_ITEM_NONE, _gl_account_list_item_sel, item_data);
		group_item_data->sub_items = eina_list_append(group_item_data->sub_items, item_data);
		debug_log("it : %p", item_data->it);

		/* Insert Show all folders item */
		item_data = calloc(1, sizeof(Account_Item_Data));
		item_data->view = view;
		item_data->account_id = account_id;
		item_data->item_type = ACCOUNT_LIST_SINGLE_SHOW_ALL_FOLDERS;
		item_data->it = elm_genlist_item_append(gl, view->itc_account_item, item_data, group_item_data->it,
				ELM_GENLIST_ITEM_NONE, _gl_account_list_item_sel, item_data);
		group_item_data->sub_items = eina_list_append(group_item_data->sub_items, item_data);

		debug_log("it : %p", item_data->it);

		email_engine_free_account_list(&email_account, 1);
		return;
	}

	item_data = calloc(1, sizeof(Account_Item_Data));
	group_item_data = item_data;
	group_item_data->view = view;
	group_item_data->item_type = ACCOUNT_LIST_SINGLE_GROUP;
	group_item_data->unread_cnt = mailbox->unread_count;
	group_item_data->total_cnt = mailbox->total_mail_count_on_local;
	group_item_data->account_id = account_id;
	group_item_data->account_name = email_account->user_email_address ? g_strdup(email_account->user_email_address) : NULL;
	group_item_data->expanded = EINA_TRUE;
	group_item_data->group_order = group_order;

	group_item_data->it = elm_genlist_item_append(gl, view->itc_account_group, item_data, group_item_data->it, ELM_GENLIST_ITEM_NONE, _gl_account_group_list_item_sel, item_data);
	view->account_group_list = g_list_append(view->account_group_list, item_data);

	/* Insert Inbox item first. */
	item_data = calloc(1, sizeof(Account_Item_Data));
	item_data->view = view;
	item_data->account_id = account_id;
	item_data->mailbox_id = mailbox->mailbox_id;
	item_data->item_type = ACCOUNT_LIST_SINGLE_INBOX;
	item_data->unread_cnt = mailbox->unread_count;
	item_data->total_cnt = mailbox->total_mail_count_on_local;
	item_data->it = elm_genlist_item_append(gl, view->itc_account_item, item_data, group_item_data->it,
			ELM_GENLIST_ITEM_NONE, _gl_account_list_item_sel, item_data);
	group_item_data->sub_items = eina_list_append(group_item_data->sub_items, item_data);
	debug_log("it : %p", item_data->it);

	email_engine_free_mailbox_list(&mailbox, 1);

	debug_log("it : %p", item_data->it);

	for (i = ACCOUNT_LIST_SINGLE_DRAFTS; i <= ACCOUNT_LIST_SINGLE_SENT; i++) {
		int mailbox_type = EMAIL_MAILBOX_TYPE_NONE;
		int bCountCheck = true;

		switch (i) {
		case ACCOUNT_LIST_SINGLE_DRAFTS:
			mailbox_type = EMAIL_MAILBOX_TYPE_DRAFT;
			break;
		case ACCOUNT_LIST_SINGLE_OUTBOX:
			mailbox_type = EMAIL_MAILBOX_TYPE_OUTBOX;
			break;
		case ACCOUNT_LIST_SINGLE_SENT:
			mailbox_type = EMAIL_MAILBOX_TYPE_SENTBOX;
			bCountCheck = false;
			break;
		}

		if (!email_engine_get_mailbox_by_mailbox_type(account_id, mailbox_type, &mailbox)) {
			email_engine_free_account_list(&email_account, 1);
			return;
		}

		if (mailbox->total_mail_count_on_local > 0) {
			if (mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX) {
				scheduled_mail_cnt = email_get_scheduled_outbox_mail_count_by_account_id(account_id, false);
				if (mailbox->total_mail_count_on_local - scheduled_mail_cnt > 0) {
					bCountCheck = false;
				}
			} else {
				bCountCheck = false;
			}
		}
		if (!bCountCheck) {
			item_data = calloc(1, sizeof(Account_Item_Data));
			item_data->view = view;
			item_data->account_id = account_id;
			item_data->mailbox_id = mailbox->mailbox_id;
			item_data->item_type = i;
			item_data->unread_cnt = mailbox->unread_count;
			if (mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX) {
				/* debug_log("mailbox->total_mail_count_on_local : %d, scheduled_mail_cnt : %d", mailbox->total_mail_count_on_local, scheduled_mail_cnt); */
				item_data->total_cnt = mailbox->total_mail_count_on_local - scheduled_mail_cnt;
			} else {
				item_data->total_cnt = mailbox->total_mail_count_on_local;
			}

			item_data->it = elm_genlist_item_append(gl, view->itc_account_item, item_data, group_item_data->it,
					ELM_GENLIST_ITEM_NONE, _gl_account_list_item_sel, item_data);
			group_item_data->sub_items = eina_list_append(group_item_data->sub_items, item_data);
			debug_log("it : %p", item_data->it);
		}
		email_engine_free_mailbox_list(&mailbox, 1);
	}

	/* Insert Show all folders item */
	item_data = calloc(1, sizeof(Account_Item_Data));
	item_data->view = view;
	item_data->account_id = account_id;
	item_data->item_type = ACCOUNT_LIST_SINGLE_SHOW_ALL_FOLDERS;
	item_data->it = elm_genlist_item_append(gl, view->itc_account_item, item_data, group_item_data->it,
			ELM_GENLIST_ITEM_NONE, _gl_account_list_item_sel, item_data);
	group_item_data->sub_items = eina_list_append(group_item_data->sub_items, item_data);

	email_engine_free_account_list(&email_account, 1);
}

void account_init_genlist_item_class_for_account_view_list(EmailAccountView *view)
{
	debug_enter();
	RETURN_IF_FAIL(view != NULL);

	Elm_Genlist_Item_Class *itc_account_group = elm_genlist_item_class_new();
	retm_if(!itc_account_group, "itc_account_group is NULL!");
	itc_account_group->item_style = "group_index";
	itc_account_group->func.text_get = _gl_label_get_for_account_list_group_item;
	itc_account_group->func.content_get = _gl_icon_get_for_account_list_group_item;
	itc_account_group->func.state_get = NULL;
	itc_account_group->func.del = _gl_account_list_item_del;
	view->itc_account_group = itc_account_group;

	Elm_Genlist_Item_Class *itc_account_item = elm_genlist_item_class_new();
	if (!itc_account_item){
		debug_error("itc_account_item is NULL - allocation memory failed");
		EMAIL_GENLIST_ITC_FREE(view->itc_account_group);
		return;
	}
	itc_account_item->item_style = "full";
	itc_account_item->func.text_get = NULL;
	itc_account_item->func.content_get = _gl_account_content_get_for_single_subitem;
	itc_account_item->func.state_get = NULL;
	itc_account_item->func.del = NULL;
	view->itc_account_item = itc_account_item;
}

static char *_gl_label_get_for_account_list_group_item(void *data, Evas_Object *obj, const char *part)
{
	if (!data) {
		debug_log("data is NULL");
		return NULL;
	}

	Account_Item_Data *item_data = (Account_Item_Data *)data;
	int item_type = item_data->item_type;
	char tmp[MAX_STR_LEN] = { 0, };

	if (!strcmp(part, "elm.text")) {
		switch (item_type) {
		case ACCOUNT_LIST_COMBINED_GROUP:
			return strdup(_("IDS_EMAIL_HEADER_COMBINED_ACCOUNTS_ABB"));
			break;
		case ACCOUNT_LIST_SINGLE_GROUP:
			if (!item_data->account_name) {
				return NULL;
			} else {
				char *account_name = elm_entry_utf8_to_markup(item_data->account_name);
				if (item_data->view->default_account_id == item_data->account_id) {
					snprintf(tmp, sizeof(tmp), "%s %s", account_name,  _("IDS_EMAIL_HEADER_HDEFAULT_LC_ABB"));
				} else {
					snprintf(tmp, sizeof(tmp), "%s", account_name);
				}
				free(account_name);
				return strdup(tmp);
			}
			break;
		default:
			debug_log("invalid item type :%d", item_type);
			break;
		}
	}

	return NULL;
}

static Evas_Object *_gl_account_content_get_for_single_subitem(void *data, Evas_Object *obj, const char *part)
{
	if (!data) {
		debug_log("data is NULL");
		return NULL;
	}

	Account_Item_Data *item_data = (Account_Item_Data *)data;
	if (!item_data->view) {
		debug_log("item_data->view is NULL");
		return NULL;
	}

	if (!strcmp(part, "elm.swallow.content")) {
		Evas_Object *full_item_ly = elm_layout_add(obj);
		elm_layout_file_set(full_item_ly, email_get_account_theme_path(), "gl_accounts_1line_item");
		evas_object_size_hint_weight_set(full_item_ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(full_item_ly, EVAS_HINT_FILL, EVAS_HINT_FILL);

		int account_color = account_color_list_get_account_color(item_data->view, item_data->account_id);
		Evas_Object *color_bar = _create_account_subitem_color_bar(full_item_ly, account_color);
		Evas_Object *folder_icon = _create_account_subitem_folder_icon(full_item_ly, item_data->item_type);

		elm_object_part_content_set(full_item_ly, "elm.swallow.color_label", color_bar);
		elm_object_part_content_set(full_item_ly, "elm.swallow.icon", folder_icon);

		elm_object_part_text_set(full_item_ly, "elm.text", _get_account_item_text_for_single_subitem(item_data));
		evas_object_smart_calculate(folder_icon);

		return full_item_ly;
	}
	return NULL;
}

static Evas_Object *_gl_icon_get_for_account_list_group_item(void *data, Evas_Object *obj, const char *part)
{
	retvm_if(!data,  NULL, "Invalid parameter: data[NULL]");

	Account_Item_Data *item_data = (Account_Item_Data *)data;
	retvm_if(!item_data->view,  NULL, "Invalid parameter: item_data->view is NULL");
	retvm_if(!item_data->it,  NULL, "Invalid parameter: item_data->it is NULL");

	if (!strcmp(part, "elm.swallow.end")) {
		if (item_data->view->account_count > 1) {
			Evas_Object *expand_icon = elm_layout_add(obj);
			if (item_data->expanded) {
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

static void  _gl_account_group_list_item_sel(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(event_info == NULL, "Invalid parameter: data[NULL]");

	Elm_Object_Item *it = (Elm_Object_Item *)event_info;
	elm_genlist_item_selected_set(it, EINA_FALSE);

	Account_Item_Data *group_item_data = (Account_Item_Data *)elm_object_item_data_get(it);
	Account_Item_Data *sub_item_data = NULL;
	EmailAccountView *view = (EmailAccountView *)group_item_data->view;
	Eina_List *l = NULL;

	if (group_item_data->expanded) {
		elm_genlist_item_subitems_clear(group_item_data->it);
	} else {
		EINA_LIST_FOREACH(group_item_data->sub_items, l, sub_item_data) {
			sub_item_data->it = elm_genlist_item_append(view->gl, view->itc_account_item, sub_item_data, group_item_data->it, ELM_GENLIST_ITEM_NONE, _gl_account_list_item_sel, sub_item_data);
			elm_genlist_item_fields_update(sub_item_data->it, "elm.icon.1", ELM_GENLIST_ITEM_FIELD_CONTENT);
		}
		elm_genlist_item_bring_in(group_item_data->it, ELM_GENLIST_ITEM_SCROLLTO_TOP);
	}

	group_item_data->expanded = !group_item_data->expanded;

	_check_account_list_zoom_state(view);
	elm_genlist_item_fields_update(group_item_data->it, "elm.swallow.end", ELM_GENLIST_ITEM_FIELD_CONTENT);

	debug_leave();
}

static Evas_Object *_create_account_subitem_color_bar(Evas_Object *parent, unsigned int color)
{
	Evas_Object *color_bar = evas_object_rectangle_add(evas_object_evas_get(parent));
	int r = R_MASKING(color);
	int g = G_MASKING(color);
	int b = B_MASKING(color);
	int a = A_MASKING(color);

	evas_object_color_set(color_bar, r, g, b, a);
	return color_bar;
}

static Evas_Object *_create_account_subitem_folder_icon(Evas_Object *parent, account_list_item_type item_type)
{
	char *folder_image_name = NULL;

	switch (item_type) {
	case ACCOUNT_LIST_COMBINED_INBOX:
		folder_image_name = EMAIL_IMAGE_ACCOUNT_COMBINED_INBOX_FOLDER_ICON;
		break;
	case ACCOUNT_LIST_SINGLE_INBOX:
		folder_image_name = EMAIL_IMAGE_ACCOUNT_INBOX_FOLDER_ICON;
		break;
	case ACCOUNT_LIST_PRIORITY_INBOX:
		folder_image_name = EMAIL_IMAGE_ACCOUNT_PRIO_SENDER_FOLDER_ICON;
		break;
	case ACCOUNT_LIST_STARRED:
		folder_image_name = EMAIL_IMAGE_ACCOUNT_STARRED_FOLDER_ICON;
		break;
	case ACCOUNT_LIST_COMBINED_DRAFTS:
	case ACCOUNT_LIST_SINGLE_DRAFTS:
		folder_image_name = EMAIL_IMAGE_ACCOUNT_DRAFT_FOLDER_ICON;
		break;
	case ACCOUNT_LIST_COMBINED_OUTBOX:
	case ACCOUNT_LIST_SINGLE_OUTBOX:
		folder_image_name = EMAIL_IMAGE_ACCOUNT_OUTBOX_FOLDER_ICON;
		break;
	case ACCOUNT_LIST_COMBINED_SENT:
	case ACCOUNT_LIST_SINGLE_SENT:
		folder_image_name = EMAIL_IMAGE_ACCOUNT_SENT_FOLDER_ICON;
		break;
	case ACCOUNT_LIST_COMBINED_SHOW_ALL_FOLDERS:
	case ACCOUNT_LIST_SINGLE_SHOW_ALL_FOLDERS:
		folder_image_name = EMAIL_IMAGE_ACCOUNT_SHOW_ALL_FOLDERS_ICON;
		break;
	default:
		debug_error("Item type is not supported!");
		return NULL;
	}

	return account_create_folder_icon(parent, folder_image_name);
}

static char *_create_account_list_item_text(Account_Item_Data *item_data, const char *text)
{
	int account_item_type = _convert_account_list_item_type(item_data->view,
			item_data->view->account_id,
			item_data->view->mailbox_type);

	char buff[MAX_STR_LEN] = { 0 };
	if (item_data->item_type > ACCOUNT_LIST_COMBINED_GROUP &&
			item_data->item_type <= ACCOUNT_LIST_COMBINED_SENT) {

		if ((account_item_type == item_data->item_type) &&
				(item_data->view->account_count == 1
				|| item_data->view->account_id == item_data->account_id)) {
			snprintf(buff, sizeof(buff), "<font=System:style=Bold><color=#%02x%02x%02x%02x>%s</color></font>", ACCOUNT_CURRENT_FOLDER_COLOR, text);
		} else {
			snprintf(buff, sizeof(buff), "<font=System:style=Regular>%s</font>", text);
		}
	} else if (item_data->item_type > ACCOUNT_LIST_SINGLE_GROUP &&
			item_data->item_type < ACCOUNT_LIST_SINGLE_SHOW_ALL_FOLDERS) {

		if (item_data->view->account_id == item_data->account_id &&
			item_data->view->mailbox_id == item_data->mailbox_id &&
			account_item_type == item_data->item_type) {
			snprintf(buff, sizeof(buff), "<font=System:style=Bold><color=#%02x%02x%02x%02x>%s</color></font>", ACCOUNT_CURRENT_FOLDER_COLOR, text);
		} else {
			snprintf(buff, sizeof(buff), "<font=System:style=Regular>%s</font>", text);
		}
	} else {
		return NULL;
	}
	return strdup(buff);
}

static char *_get_account_item_text_for_single_subitem(Account_Item_Data *item_data)
{
	int item_type = item_data->item_type;
	char tmp[MAX_STR_LEN] = { 0 };

	switch (item_type) {
	case ACCOUNT_LIST_COMBINED_INBOX:
		if (item_data->unread_cnt == 0) {
			snprintf(tmp, sizeof(tmp), "%s", _("IDS_EMAIL_HEADER_COMBINED_INBOX_ABB"));
		} else {
			snprintf(tmp, sizeof(tmp), "%s (%d)", _("IDS_EMAIL_HEADER_COMBINED_INBOX_ABB"), item_data->unread_cnt);
		}
		break;
#ifndef _FEATURE_PRIORITY_SENDER_DISABLE_
	case ACCOUNT_LIST_PRIORITY_INBOX:
		if (item_data->unread_cnt == 0) {
			snprintf(tmp, sizeof(tmp), "%s", _("IDS_EMAIL_HEADER_PRIORITY_SENDERS_ABB"));
		} else {
			snprintf(tmp, sizeof(tmp), "%s (%d)", _("IDS_EMAIL_HEADER_PRIORITY_SENDERS_ABB"), item_data->unread_cnt);
		}
		break;
#endif
	case ACCOUNT_LIST_STARRED:
		if (item_data->unread_cnt == 0) {
			snprintf(tmp, sizeof(tmp), "%s", _("IDS_EMAIL_HEADER_STARRED_EMAILS_ABB"));
		} else {
			snprintf(tmp, sizeof(tmp), "%s (%d)", _("IDS_EMAIL_HEADER_STARRED_EMAILS_ABB"), item_data->unread_cnt);
		}
		break;
	case ACCOUNT_LIST_COMBINED_DRAFTS:
	case ACCOUNT_LIST_SINGLE_DRAFTS:
		if (item_data->total_cnt == 0) {
			snprintf(tmp, sizeof(tmp), "%s", _("IDS_EMAIL_HEADER_DRAFTS"));
		} else {
			snprintf(tmp, sizeof(tmp), "%s (%d)", _("IDS_EMAIL_HEADER_DRAFTS"), item_data->total_cnt);
		}
		break;
	case ACCOUNT_LIST_COMBINED_OUTBOX:
	case ACCOUNT_LIST_SINGLE_OUTBOX:
		if (item_data->total_cnt == 0) {
			snprintf(tmp, sizeof(tmp), "%s", _("IDS_EMAIL_HEADER_OUTBOX"));
		} else {
			snprintf(tmp, sizeof(tmp), "%s (%d)", _("IDS_EMAIL_HEADER_OUTBOX"), item_data->total_cnt);
		}
		break;
	case ACCOUNT_LIST_COMBINED_SENT:
	case ACCOUNT_LIST_SINGLE_SENT:
		if (item_data->unread_cnt == 0) {
			snprintf(tmp, sizeof(tmp), "%s", _("IDS_EMAIL_HEADER_SENT_M_EMAIL"));
		} else {
			snprintf(tmp, sizeof(tmp), "%s (%d)", _("IDS_EMAIL_HEADER_SENT_M_EMAIL"), item_data->unread_cnt);
		}
		break;
	case ACCOUNT_LIST_COMBINED_SHOW_ALL_FOLDERS:
	case ACCOUNT_LIST_SINGLE_SHOW_ALL_FOLDERS:
		return strdup(_("IDS_EMAIL_HEADER_SHOW_ALL_FOLDERS_ABB"));
	case ACCOUNT_LIST_SINGLE_INBOX:
		if (item_data->unread_cnt == 0) {
			snprintf(tmp, sizeof(tmp), "%s", _("IDS_EMAIL_HEADER_INBOX"));
		} else {
			snprintf(tmp, sizeof(tmp), "%s (%d)", _("IDS_EMAIL_HEADER_INBOX"), item_data->unread_cnt);
		}
		break;
	default:
		debug_log("invalid item type :%d", item_type);
		return NULL;
	}
	return _create_account_list_item_text(item_data, tmp);
}

static void account_item_del(Account_Item_Data *item_data)
{
	if (item_data) {
		EmailAccountView *view = item_data->view;
		if (view)
			view->account_group_list = g_list_remove(view->account_group_list, item_data);
		FREE(item_data->account_name);

		if (item_data->sub_items) {
			Eina_List *l = NULL;
			Account_Item_Data *sub_item_data = NULL;
			EINA_LIST_FOREACH(item_data->sub_items, l, sub_item_data) {
				account_item_del(sub_item_data);
			}
			eina_list_free(item_data->sub_items);
		}

		FREE(item_data);
	}
}

static void _gl_account_list_item_del(void *data, Evas_Object *obj)
{
	account_item_del((Account_Item_Data *)data);
}

static void _gl_account_list_item_sel(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	if (!data) {
		debug_log("data is NULL");
		return;
	}

	Account_Item_Data *item_data = (Account_Item_Data *)data;
	EmailAccountView *view = item_data->view;
	int mailbox_id = 0;
	retm_if(!view, "view is NULL");

	email_mailbox_type_e mailbox_type = EMAIL_MAILBOX_TYPE_INBOX;
	int box_type = ACC_MAILBOX_TYPE_INBOX;


	elm_genlist_item_selected_set(item_data->it, EINA_FALSE);

	switch (item_data->item_type) {
	case ACCOUNT_LIST_COMBINED_GROUP:
	case ACCOUNT_LIST_SINGLE_GROUP:
	{
		return;
	}
	case ACCOUNT_LIST_COMBINED_INBOX:
		mailbox_type = EMAIL_MAILBOX_TYPE_INBOX;
		box_type = ACC_MAILBOX_TYPE_INBOX;
		view->account_id = 0;
		break;
#ifndef _FEATURE_PRIORITY_SENDER_DISABLE_
	case ACCOUNT_LIST_PRIORITY_INBOX:
		mailbox_type = EMAIL_MAILBOX_TYPE_INBOX;
		box_type = ACC_MAILBOX_TYPE_PRIORITY_INBOX;
		view->account_id = 0;
		break;
#endif
	case ACCOUNT_LIST_STARRED:
		mailbox_type = EMAIL_MAILBOX_TYPE_FLAGGED;
		box_type = ACC_MAILBOX_TYPE_FLAGGED;
		view->account_id = 0;
		break;

	case ACCOUNT_LIST_COMBINED_DRAFTS:
		mailbox_type = EMAIL_MAILBOX_TYPE_DRAFT;
		box_type = ACC_MAILBOX_TYPE_DRAFT;
		view->account_id = 0;
		break;

	case ACCOUNT_LIST_COMBINED_OUTBOX:
		mailbox_type = EMAIL_MAILBOX_TYPE_OUTBOX;
		box_type = ACC_MAILBOX_TYPE_OUTBOX;
		view->account_id = 0;
		break;

	case ACCOUNT_LIST_COMBINED_SENT:
		mailbox_type = EMAIL_MAILBOX_TYPE_SENTBOX;
		box_type = ACC_MAILBOX_TYPE_SENTBOX;
		view->account_id = 0;
		break;

	case ACCOUNT_LIST_COMBINED_SHOW_ALL_FOLDERS:
		view->account_id = 0;
		view->folder_view_mode = ACC_FOLDER_COMBINED_VIEW_MODE;
		view->folder_mode = ACC_FOLDER_NONE;
		account_show_all_folder(view);
		return;

	case ACCOUNT_LIST_SINGLE_INBOX:
		mailbox_type = EMAIL_MAILBOX_TYPE_INBOX;
		box_type = ACC_MAILBOX_TYPE_INBOX;
		view->account_id = item_data->account_id;
		mailbox_id = item_data->mailbox_id;
		break;

	case ACCOUNT_LIST_SINGLE_DRAFTS:
		mailbox_type = EMAIL_MAILBOX_TYPE_DRAFT;
		box_type = ACC_MAILBOX_TYPE_DRAFT;
		view->account_id = item_data->account_id;
		mailbox_id = item_data->mailbox_id;
		break;

	case ACCOUNT_LIST_SINGLE_OUTBOX:
		mailbox_type = EMAIL_MAILBOX_TYPE_OUTBOX;
		box_type = ACC_MAILBOX_TYPE_OUTBOX;
		view->account_id = item_data->account_id;
		mailbox_id = item_data->mailbox_id;
		break;

	case ACCOUNT_LIST_SINGLE_SENT:
		mailbox_type = EMAIL_MAILBOX_TYPE_SENTBOX;
		box_type = ACC_MAILBOX_TYPE_SENTBOX;
		view->account_id = item_data->account_id;
		mailbox_id = item_data->mailbox_id;
		break;

	case ACCOUNT_LIST_SINGLE_SHOW_ALL_FOLDERS:
		view->account_id = item_data->account_id;
		view->folder_view_mode = ACC_FOLDER_COMBINED_SINGLE_VIEW_MODE;
		view->folder_mode = ACC_FOLDER_NONE;
		account_show_all_folder(view);
		return;

	default:
		debug_log("invalid item type :%d", item_data->item_type);
		break;
	}

	debug_log("account_id: [%d], box_type: [%d], mailbox_type: [%d]", view->account_id, box_type, mailbox_type);

	const char *account_type = EMAIL_BUNDLE_VAL_SINGLE_ACCOUNT;
	if (box_type == ACC_MAILBOX_TYPE_PRIORITY_INBOX) {
		account_type = EMAIL_BUNDLE_VAL_PRIORITY_SENDER;
	} else if (view->account_id == 0) {
		account_type = EMAIL_BUNDLE_VAL_ALL_ACCOUNT;
	}

	email_params_h params = NULL;

	if (email_params_create(&params) &&
		email_params_add_str(params, EMAIL_BUNDLE_KEY_ACCOUNT_TYPE, account_type) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_ACCOUNT_ID, view->account_id) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_MAILBOX_TYPE, mailbox_type) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_MAILBOX, mailbox_id) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_IS_MAILBOX_MOVE_MODE, 0)) {

		email_module_send_result(view->base.module, params);
	}

	email_params_free(&params);

	view->block_item_click = 1;
}

static void _check_account_list_zoom_state(EmailAccountView *view)
{
	debug_enter();

	if (view && view->account_group_list) {
		GList *first_element = g_list_first(view->account_group_list);
		Account_Item_Data *first_item_data = (Account_Item_Data *)g_list_nth_data(first_element, 0);
		bool same_state = true;
		GList *cur = g_list_next(first_element);

		while (cur) {
			Account_Item_Data *item_data = (Account_Item_Data *)g_list_nth_data(cur, 0);
			if (item_data->expanded != first_item_data->expanded) {
				same_state = false;
				break;
			}
			cur = g_list_next(cur);
		}
		if (same_state) {
			view->account_group_state =
					first_item_data->expanded ? ACCOUNT_LIST_ALL_GROUP_EXPANDED : ACCOUNT_LIST_ALL_GROUP_CONTRACTED;
		} else {
			view->account_group_state = ACCOUNT_LIST_SOME_GROUP_EXPANEDED;
		}
		debug_log("account_group_state : %d", view->account_group_state);
	}
}

static int _convert_account_list_item_type(EmailAccountView *view, int account_id, int mailbox_type)
{
	int account_item_type = ACCOUNT_LIST_NONE;

	if (view) {

		if (view->mailbox_mode == ACC_MAILBOX_TYPE_PRIORITY_INBOX) {
			account_item_type = ACCOUNT_LIST_PRIORITY_INBOX;
		} else {
			if (account_id == 0) {
				switch (mailbox_type) {
				case EMAIL_MAILBOX_TYPE_INBOX:
					account_item_type = ACCOUNT_LIST_COMBINED_INBOX;
					break;
				case EMAIL_MAILBOX_TYPE_SENTBOX:
					account_item_type = ACCOUNT_LIST_COMBINED_SENT;
					break;
				case EMAIL_MAILBOX_TYPE_DRAFT:
					account_item_type = ACCOUNT_LIST_COMBINED_DRAFTS;
					break;
				case EMAIL_MAILBOX_TYPE_OUTBOX:
					account_item_type = ACCOUNT_LIST_COMBINED_OUTBOX;
					break;
				case EMAIL_MAILBOX_TYPE_FLAGGED:
					account_item_type = ACCOUNT_LIST_STARRED;
					break;
				}

			} else {
				switch (mailbox_type) {
				case EMAIL_MAILBOX_TYPE_INBOX:
					account_item_type = ACCOUNT_LIST_SINGLE_INBOX;
					break;
				case EMAIL_MAILBOX_TYPE_SENTBOX:
					account_item_type = ACCOUNT_LIST_SINGLE_SENT;
					break;
				case EMAIL_MAILBOX_TYPE_DRAFT:
					account_item_type = ACCOUNT_LIST_SINGLE_DRAFTS;
					break;
				case EMAIL_MAILBOX_TYPE_OUTBOX:
					account_item_type = ACCOUNT_LIST_SINGLE_OUTBOX;
					break;
				}
			}
		}
	}
	return account_item_type;
}
