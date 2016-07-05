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
#include "email-account-util.h"

#define MAX_NAME_SIZE 1024
#define EMAIL_UTF7_REVERSE_SLASH_CODE "+AFw-"
#define EMAIL_UTF7_REVERSE_TILDE_CODE "+AH4-"

struct _folder_type_position {
	int mailbox_position;
	int mailbox_type;
};

typedef struct _EmailAccountColor {
	int account_id;
	int account_color;
} EmailAccountColor;

static void _entry_maxlength_reached_cb(void *data, Evas_Object *obj, void *event_info);
static void _entry_changed_cb(void *data, Evas_Object *obj, void *event_info);
static void _entry_clicked_cb(void *data, Evas_Object *obj, void *event_info);

static void _register_entry_popup_rot_callback(Evas_Object *popup, EmailAccountView *view);
static void _unregister_entry_popup_rot_callback(Evas_Object *popup, EmailAccountView *view);

static void _entry_popup_keypad_down_cb(void *data, Evas_Object *obj, void *event_info);
static void _entry_popup_keypad_up_cb(void *data, Evas_Object *obj, void *event_info);
static void _entry_popup_rot_cb(void *data, Evas_Object *obj, void *event_info);
static void _entry_popup_del_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static Eina_Bool _entry_popup_idler_cb(void *data);

EMAIL_DEFINE_GET_EDJ_PATH(email_get_account_theme_path, "/email-account.edj")

char *account_get_ellipsised_folder_name(EmailAccountView *view, char *org_filename)
{
	debug_enter();
	char *after_utf8_markup = NULL;
	char *filename = NULL;

	debug_secure("org_filename[%s]", org_filename);
	after_utf8_markup = elm_entry_utf8_to_markup(org_filename);
	filename = g_strconcat("<ellipsis=0.5>", after_utf8_markup, "</ellipsis>", NULL);
	if (!filename) {
		filename = g_strdup(org_filename);
	}

	FREE(after_utf8_markup);
	debug_leave();
	return filename;
}

/* from email-service char *emcore_convert_mutf7_to_utf8(char *mailbox_name) */
char *account_util_convert_mutf7_to_utf8(char *mailbox_name)
{
	char *result_mailbox_name = NULL;
	char *cursor = NULL;
	int is_base64 = 0;
	char buff[MAX_NAME_SIZE] = { 0 };
	int offset = 0;

	if (mailbox_name == NULL) {
		debug_error("EMAIL_ERROR_INVALID_PARAM");
		return NULL;
	}

	debug_secure("mailbox_name[%s]", mailbox_name);

	cursor = mailbox_name;

	if (cursor == NULL) {
		debug_error("EMAIL_ERROR_OUT_OF_MEMORY");
		return NULL;
	}

	for (; *cursor; ++cursor) {
		char temp = '\0';
		switch (*cursor) {
		case '+':
			if (!is_base64) {
				temp = '&';
			} else {
				temp = *cursor;
			}
			break;
		case '&':
			is_base64 = 1;
			temp = '+';
			break;
		case '-':
			is_base64 = 0;
			temp = '-';
			break;
		case ',':
			if (is_base64) {
				temp = '/';
			} else {
				temp = *cursor;
			}
			break;
		case '\\':
			offset += snprintf(buff + offset, MAX_NAME_SIZE - offset, EMAIL_UTF7_REVERSE_SLASH_CODE);
			break;
		case '~':
			offset += snprintf(buff + offset, MAX_NAME_SIZE - offset, EMAIL_UTF7_REVERSE_TILDE_CODE);
			break;
		default:
			temp = *cursor;
			break;
		}
		if (temp && offset < MAX_NAME_SIZE - 1) {
			buff[offset++] = temp;
			buff[offset] = '\0';
		}
	}

	result_mailbox_name = eina_str_convert("UTF-7", "UTF-8", buff);
	debug_secure("result_mailbox_name [%s]", result_mailbox_name);

	if (result_mailbox_name == NULL) {
		debug_error("convert utf7 to utf8 failed");
		return NULL;
	}

	cursor = result_mailbox_name;
	for (; *cursor; ++cursor) {
		switch (*cursor) {
			case '&':
				*cursor = '+';
				break;
			case '+':
				*cursor = '&';
				break;
		}
	}

	debug_secure("result_mailbox_name[%p] [%s]", result_mailbox_name, result_mailbox_name);

	return result_mailbox_name;
}

char *account_convert_folder_alias_by_mailbox(email_mailbox_t *mlist)
{
	if (!mlist) {
		debug_log("mlist is NULL");
		return NULL;
	}
	char *mailbox_alias = NULL;

	if (mlist) {
		switch (mlist->mailbox_type) {
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
			mailbox_alias = mlist->alias;
			break;
		}
	}

	if (mailbox_alias)
		return g_strdup(mailbox_alias);
	else
		return NULL;
}

char *account_convert_folder_alias_by_mailbox_type(email_mailbox_type_e mailbox_type)
{
	char *mailbox_alias = NULL;

	switch (mailbox_type) {
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
		break;
	}

	if (mailbox_alias)
		return g_strdup(mailbox_alias);
	else
		return NULL;
}

char *account_get_folder_icon_name_by_mailbox_type(int mailbox_type)
{
	char *folder_image_name = NULL;

		switch (mailbox_type) {
		case EMAIL_MAILBOX_TYPE_INBOX:
			folder_image_name = EMAIL_IMAGE_ACCOUNT_INBOX_FOLDER_ICON;
			break;
		case EMAIL_MAILBOX_TYPE_FLAGGED:
			folder_image_name = EMAIL_IMAGE_ACCOUNT_STARRED_FOLDER_ICON;
			break;
		case EMAIL_MAILBOX_TYPE_DRAFT:
			folder_image_name = EMAIL_IMAGE_ACCOUNT_DRAFT_FOLDER_ICON;
			break;
		case EMAIL_MAILBOX_TYPE_OUTBOX:
			folder_image_name = EMAIL_IMAGE_ACCOUNT_OUTBOX_FOLDER_ICON;
			break;
		case EMAIL_MAILBOX_TYPE_SENTBOX:
			folder_image_name = EMAIL_IMAGE_ACCOUNT_SENT_FOLDER_ICON;
			break;
		case EMAIL_MAILBOX_TYPE_SPAMBOX:
			folder_image_name = EMAIL_IMAGE_ACCOUNT_SPAM_FOLDER_ICON;
			break;
		case EMAIL_MAILBOX_TYPE_TRASH:
			folder_image_name = EMAIL_IMAGE_ACCOUNT_TRASH_FOLDER_ICON;
			break;
		case EMAIL_MAILBOX_TYPE_USER_DEFINED:
			folder_image_name = EMAIL_IMAGE_ACCOUNT_CUSTOM_FOLDER_ICON;
			break;
		/* This mailbox type is extension of email_mailbox_type_e. It is defined in Email to handle Priority senders folder */
		case EMAIL_MAILBOX_TYPE_PRIORITY_SENDERS:
			folder_image_name = EMAIL_IMAGE_ACCOUNT_PRIO_SENDER_FOLDER_ICON;
			break;
		default:
			debug_error("Item type is not supported!");
			return NULL;
		}

		return folder_image_name;
}

Evas_Object *account_create_folder_icon(Evas_Object *parent, const char *folder_image_name)
{
	if (!parent || !folder_image_name) {
		debug_error("Invalid arguments!");
		return NULL;
	}

	Evas_Object *folder_icon =  elm_layout_add(parent);
	elm_layout_file_set(folder_icon, email_get_common_theme_path(), folder_image_name);
	evas_object_show(folder_icon);
	return folder_icon;
}

void account_update_folder_item_dim_state(EmailAccountView *view)
{
	debug_enter();

	if (view == NULL) {
		debug_log("data is NULL");
		return;
	}
	if (view->gl == NULL) {
		debug_log("genlist is NULL");
		return;
	}

	Elm_Object_Item *it = NULL;
	if (view->gl) {
		it = elm_genlist_first_item_get(view->gl);
		if (it == NULL) {
			debug_log("elm_genlist_first_item_get is failed");
			return;
		}
		Item_Data *tree_item_data = NULL;
		while (it) {
			tree_item_data = (Item_Data *)elm_object_item_data_get((const Elm_Object_Item *)it);
			if (tree_item_data) {
				Eina_Bool value_to_set;
				switch (tree_item_data->mailbox_type) {
				case EMAIL_MAILBOX_TYPE_USER_DEFINED:
					value_to_set = EINA_FALSE;
					break;
				case EMAIL_MAILBOX_TYPE_OUTBOX:
					value_to_set = view->folder_mode != ACC_FOLDER_NONE;
					break;
				default:
					value_to_set = (view->folder_mode == ACC_FOLDER_DELETE ||
							view->folder_mode == ACC_FOLDER_RENAME);
				}
				elm_object_item_disabled_set(tree_item_data->it, value_to_set);
			}
			it = elm_genlist_item_next_get(it);
		}
	}
	return;
}

void account_update_view_title(EmailAccountView *view)
{
	if (view == NULL) {
		debug_log("data is NULL");
		return;
	}

	if (view->folder_view_mode == ACC_FOLDER_COMBINED_SINGLE_VIEW_MODE) {
		if (view->user_email_address) {
			elm_object_item_text_set(view->base.navi_item, view->user_email_address);
		} else {
			elm_object_item_domain_translatable_text_set(view->base.navi_item, PACKAGE, "IDS_EMAIL_HEADER_FOLDERS");
		}
	} else {
		elm_object_item_domain_translatable_text_set(view->base.navi_item, PACKAGE, "IDS_EMAIL_HEADER_MAILBOX_ABB");
	}
}

char *account_get_user_email_address(int account_id)
{
	email_account_t *email_account = NULL;
	char *address = NULL;

	if (!email_engine_get_account_data(account_id, EMAIL_ACC_GET_OPT_DEFAULT, &email_account)) {
		debug_log("email_engine_get_account_data failed, account_name is NULL");
		return NULL;
	}
	debug_log("%s", email_account->user_email_address);

	address = strdup(email_account->user_email_address);

	email_engine_free_account_list(&email_account, 1);

	return address;
}

static Eina_Bool _entry_popup_idler_cb(void *data)
{
	EmailAccountView *view = data;

	elm_object_focus_set(view->entry, EINA_TRUE);

	view->popup_focus_idler = NULL;

	return ECORE_CALLBACK_DONE;
}

Evas_Object *account_create_entry_popup(EmailAccountView *view, email_string_t t_title,
		const char *entry_text, const char *entry_selection_text,
		Evas_Smart_Cb _back_response_cb, Evas_Smart_Cb _done_key_cb,
		Evas_Smart_Cb btn1_response_cb, const char *btn1_text, Evas_Smart_Cb btn2_response_cb, const char *btn2_text)
{
	debug_enter();

	if (!view) {
		debug_log("view is NULL");
		return NULL;
	}

	Evas_Object *popup, *btn1, *btn2;
	email_editfield_t editfield;
	Elm_Entry_Filter_Limit_Size limit_filter_data;
	Eina_Bool disable_btn = EINA_FALSE;

	if (view->popup) {
		evas_object_del(view->popup);
		view->popup = NULL;
	}

	if (!_back_response_cb || !_done_key_cb) {
		debug_log("_response_cb or _done_key_cb is NULL");
		return NULL;
	}

	if (!btn1_response_cb || !btn1_text) {
		debug_log("btn1_response_cb or btn1_text is NULL");
		return NULL;
	}

	if (!btn2_response_cb || !btn2_text) {
		debug_log("btn2_response_cb or btn2_text is NULL");
		return NULL;
	}

	if (!t_title.id) {
		debug_log("title is NULL");
		return NULL;
	}

	popup = elm_popup_add(view->base.module->navi);
	elm_popup_align_set(popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
	eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, _back_response_cb, view);
	evas_object_event_callback_add(popup, EVAS_CALLBACK_DEL, _entry_popup_del_cb, view);
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	if (t_title.id) {
		if (t_title.domain) {
			elm_object_domain_translatable_part_text_set(popup, "title,text", t_title.domain, t_title.id);
		} else {
			elm_object_part_text_set(popup, "title,text", t_title.id);
		}
	}

	evas_object_show(popup);
	email_common_util_editfield_create(popup, 0, &editfield);
	elm_object_content_set(popup, editfield.layout);

	btn1 = elm_button_add(popup);
	elm_object_style_set(btn1, "popup");
	elm_object_domain_translatable_text_set(btn1, PACKAGE, btn1_text);
	elm_object_part_content_set(popup, "button1", btn1);
	evas_object_smart_callback_add(btn1, "clicked", btn1_response_cb, view);

	btn2 = elm_button_add(popup);
	elm_object_style_set(btn2, "popup");
	elm_object_domain_translatable_text_set(btn2, PACKAGE, btn2_text);
	elm_object_part_content_set(popup, "button2", btn2);
	evas_object_smart_callback_add(btn2, "clicked", btn2_response_cb, view);

	limit_filter_data.max_byte_count = 0;
	limit_filter_data.max_char_count = MAX_FOLDER_NAME_LEN;
	elm_entry_markup_filter_append(editfield.entry, elm_entry_filter_limit_size, &limit_filter_data);
	evas_object_smart_callback_add(editfield.entry, "maxlength,reached", _entry_maxlength_reached_cb, view);

	FREE(view->original_folder_name);
	if (entry_text) {
		elm_entry_entry_set(editfield.entry, entry_text); /* set current folder name */
		elm_entry_cursor_end_set(editfield.entry);
		view->original_folder_name = g_strdup(entry_text);
	}

	if (entry_selection_text) {
		elm_entry_context_menu_disabled_set(editfield.entry, EINA_TRUE);
		elm_entry_entry_set(editfield.entry, entry_selection_text);
		elm_entry_select_all(editfield.entry);
		evas_object_smart_callback_add(editfield.entry, "clicked", _entry_clicked_cb, view);
		view->selection_disabled = true;
	} else {
		view->selection_disabled = false;
	}

	if (elm_entry_is_empty(editfield.entry)) {
		disable_btn = EINA_TRUE;
	} else {
		gchar *final_entry_text = g_strdup(elm_object_text_get(editfield.entry));
		g_strstrip(final_entry_text);

		if (strlen(final_entry_text) == 0) {
			debug_log("entry text includes only whitespace");
			disable_btn = EINA_TRUE;
		} else if (view->original_folder_name) {
			if (!g_strcmp0(final_entry_text, view->original_folder_name)) {
				disable_btn = EINA_TRUE;
			}
		}
		FREE(final_entry_text);
	}

	elm_object_disabled_set(btn2, disable_btn);
	elm_entry_input_panel_return_key_disabled_set(editfield.entry, disable_btn);

	evas_object_smart_callback_add(editfield.entry, "activated", _done_key_cb, view);
	evas_object_smart_callback_add(editfield.entry, "changed", _entry_changed_cb, view);
	evas_object_smart_callback_add(editfield.entry, "preedit,changed", _entry_changed_cb, view);
	email_string_t EMAIL_ACCOUNT_FOLDER_NAME = { PACKAGE, "IDS_EMAIL_BODY_FOLDER_NAME"};
	elm_object_domain_translatable_part_text_set(editfield.entry, "elm.guide", EMAIL_ACCOUNT_FOLDER_NAME.domain, EMAIL_ACCOUNT_FOLDER_NAME.id);

	view->entry = editfield.entry;
	view->popup = popup;
	view->popup_ok_btn = btn2;

	evas_object_data_del(popup, "_header");
	evas_object_data_set(popup, "_header", t_title.id);
	_register_entry_popup_rot_callback(popup, view);

	view->popup_focus_idler = ecore_idler_add(_entry_popup_idler_cb, view);

	return popup;
}

static void _entry_maxlength_reached_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	if (!data || !obj) {
		debug_log("data is NULL");
		return;
	}
	char buf[MAX_STR_LEN] = {0,};

	/* display warning popup */
	debug_log("entry length is max now");
	snprintf(buf, sizeof(buf), email_get_email_string("IDS_EMAIL_TPOP_MAXIMUM_NUMBER_OF_CHARACTERS_HPD_REACHED"), MAX_FOLDER_NAME_LEN);
	int ret = notification_status_message_post(buf);
	if (ret != NOTIFICATION_ERROR_NONE) {
		debug_log("fail to notification_status_message_post() : %d\n", ret);
	}
	debug_leave();
}

static void _entry_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	if (!data || !obj) {
		debug_log("data is NULL");
		return;
	}
	EmailAccountView *view = (EmailAccountView *)data;

	if (view->selection_disabled) {
		elm_entry_context_menu_disabled_set(obj, EINA_FALSE);
		elm_object_signal_emit(obj, "app,selection,handler,enable", "app");
		view->selection_disabled = false;
		evas_object_smart_callback_del(obj, "clicked", _entry_clicked_cb);
	}
}

static void _entry_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	if (!data || !obj) {
		debug_log("data is NULL");
		return;
	}
	EmailAccountView *view = (EmailAccountView *)data;
	Eina_Bool disable_btn = EINA_FALSE;

	if (view->selection_disabled) {
		elm_entry_context_menu_disabled_set(obj, EINA_FALSE);
		elm_object_signal_emit(obj, "app,selection,handler,enable", "app");
		view->selection_disabled = false;
	}

	if (elm_entry_is_empty(obj)) {
		disable_btn = EINA_TRUE;
	} else {
		gchar *entry_text = g_strdup(elm_object_text_get(obj));
		g_strstrip(entry_text);

		if (strlen(entry_text) == 0) {
			debug_log("entry text includes only whitespace");
			disable_btn = EINA_TRUE;
		} else if (view->original_folder_name) {
			if (!g_strcmp0(entry_text, view->original_folder_name)) {
				disable_btn = EINA_TRUE;
			}
		}
		FREE(entry_text);
	}

	elm_object_disabled_set(view->popup_ok_btn, disable_btn);
	elm_entry_input_panel_return_key_disabled_set(obj, disable_btn);
}

void account_stop_emf_job(EmailAccountView *view, int handle)
{
	debug_enter();
	RETURN_IF_FAIL(view != NULL);

	gint account_id = view->account_id;
	debug_log("stop job - handle (%d)", handle);
	email_engine_stop_working(account_id, handle);
	view->emf_handle = EMAIL_HANDLE_INVALID;
}

void account_sync_cancel_all(EmailAccountView *view)
{
	debug_enter();
	RETURN_IF_FAIL(view != NULL);

	email_task_information_t *cur_task_info = NULL;
	int task_info_cnt = 0;
	int i = 0;
	gint acct_id = 0;

	gboolean b_default_account_exist = email_engine_get_default_account(&acct_id);
	if (b_default_account_exist) {
		if (view->emf_handle == EMAIL_HANDLE_INVALID) {
			email_engine_get_task_information(&cur_task_info, &task_info_cnt);
			if (cur_task_info) {
				for (i = 0; i < task_info_cnt; i++) {
					debug_log("account_id(%d), handle(%d), type(%d)", cur_task_info[i].account_id, cur_task_info[i].handle, cur_task_info[i].type);
					if (cur_task_info[i].type != EMAIL_EVENT_MOVE_MAIL
							&& cur_task_info[i].type != EMAIL_EVENT_DELETE_MAIL
							&& cur_task_info[i].type != EMAIL_EVENT_DELETE_MAIL_ALL
							&& cur_task_info[i].type != EMAIL_EVENT_SEND_MAIL) {
						email_engine_cancel_job(cur_task_info[i].account_id, cur_task_info[i].handle, EMAIL_CANCELED_BY_USER);
					}
				}

				FREE(cur_task_info);
			}
		}
	}
}

static void _register_entry_popup_rot_callback(Evas_Object *popup, EmailAccountView *view)
{
	debug_enter();

	evas_object_smart_callback_add(view->base.module->conform, "virtualkeypad,state,on", _entry_popup_keypad_up_cb, view);
	evas_object_smart_callback_add(view->base.module->conform, "virtualkeypad,state,off", _entry_popup_keypad_down_cb, view);
	evas_object_smart_callback_add(view->base.module->win, "wm,rotation,changed", _entry_popup_rot_cb, view);
}

static void _unregister_entry_popup_rot_callback(Evas_Object *popup, EmailAccountView *view)
{
	debug_enter();

	evas_object_smart_callback_del_full(view->base.module->conform, "virtualkeypad,state,on", _entry_popup_keypad_up_cb, view);
	evas_object_smart_callback_del_full(view->base.module->conform, "virtualkeypad,state,off", _entry_popup_keypad_down_cb, view);
	evas_object_smart_callback_del_full(view->base.module->win, "wm,rotation,changed", _entry_popup_rot_cb, view);
}

static void _entry_popup_keypad_down_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailAccountView *view = (EmailAccountView *)data;
	int rot = -1;

	rot = elm_win_rotation_get(view->base.module->win);
	if (rot == 90 || rot == 270) {
		const char *header = (const char *)evas_object_data_get(view->popup, "_header");
		elm_object_domain_translatable_part_text_set(view->popup, "title,text", PACKAGE, header);
	}
	view->is_keypad = 0;
}

static void _entry_popup_keypad_up_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailAccountView *view = (EmailAccountView *)data;
	int rot = -1;

	rot = elm_win_rotation_get(view->base.module->win);
	if (rot == 90 || rot == 270) {
		elm_object_part_text_set(view->popup, "title,text", NULL);
	}
	view->is_keypad = 1;
}

static void _entry_popup_rot_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailAccountView *view = (EmailAccountView *)data;
	int rot = -1;
	const char *header = (const char *)evas_object_data_get(view->popup, "_header");

	rot = elm_win_rotation_get(obj);
	if (rot == 90 || rot == 270) {
		if (view->is_keypad) {
			elm_object_domain_translatable_part_text_set(view->popup, "title,text", PACKAGE, header);
		} else {
			elm_object_part_text_set(view->popup, "title,text", NULL);
		}
	} else {
		if (view->is_keypad) {
			elm_object_domain_translatable_part_text_set(view->popup, "title,text", PACKAGE, header);
		}
	}
}

static void _entry_popup_del_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailAccountView *view = data;

	if (view->popup_focus_idler) {
		ecore_idler_del(view->popup_focus_idler);
		view->popup_focus_idler = NULL;
	}
	view->entry = NULL;
	view->popup_ok_btn = NULL;

	_unregister_entry_popup_rot_callback(obj, view);
}

int account_color_list_get_account_color(EmailAccountView *view, int account_id)
{
	if (account_id <= 0 || !view) {
		debug_log("invalid parameter, account_id : %d", account_id);
		return 0;
	}

	if (view->account_color_list) {
		GList *cur = NULL;
		EmailAccountColor *account_color_data = NULL;
		G_LIST_FOREACH(view->account_color_list, cur, account_color_data) {
			if (account_color_data->account_id == account_id) {
				return account_color_data->account_color;
			}
		}
	}
	return 0;
}

void account_color_list_free(EmailAccountView *view)
{
	retm_if(view == NULL, "EmailAccountView[NULL]");

	debug_log("view->account_color_list(%p)", view->account_color_list);

	if (view->account_color_list) {
		GList *cur = NULL;
		EmailAccountColor *account_color_data = NULL;
		G_LIST_FOREACH(view->account_color_list, cur, account_color_data) {
			FREE(account_color_data);
		}
		g_list_free(view->account_color_list);
		view->account_color_list = NULL;
	}
}

void account_color_list_add(EmailAccountView *view, int account_id, int account_color)
{
	debug_enter();
	retm_if(!view, "invalid parameter, view is NULL");
	debug_log("account_color : %d", account_color);

	EmailAccountColor *account_color_data = MEM_ALLOC(account_color_data, 1);
	retm_if(!account_color_data, "Internal error! Failed to allocate memory!");

	account_color_data->account_id = account_id;
	account_color_data->account_color = account_color;
	view->account_color_list = g_list_append(view->account_color_list, account_color_data);
}

void account_color_list_update(EmailAccountView *view, int account_id, int update_color)
{
	debug_enter();
	retm_if(account_id <= 0 || !view, "invalid parameter, account_id : %d", account_id);

	if (view->account_color_list) {
		GList *cur = NULL;
		EmailAccountColor *account_color_data = NULL;
		G_LIST_FOREACH(view->account_color_list, cur, account_color_data) {
			if (account_color_data->account_id == account_id) {
				account_color_data->account_color = update_color;
				break;
			}
		}
	}
}
