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

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <notification.h>
#include <storage/storage.h>
#include <stdarg.h>
#include <media_content.h>

#include "email-common-types.h"
#include "email-utils.h"
#include "email-engine.h"
#include "email-debug.h"

#include "email-viewer.h"
#include "email-viewer-attachment.h"
#include "email-viewer-types.h"
#include "email-viewer-util.h"
#include "email-viewer-contents.h"
#include "email-viewer-logic.h"
#include "email-viewer-header.h"

static email_string_t EMAIL_VIEWER_POP_ERROR = { PACKAGE, "IDS_EMAIL_HEADER_UNABLE_TO_PERFORM_ACTION_ABB" };
static email_string_t EMAIL_VIEWER_STRING_OK = { PACKAGE, "IDS_EMAIL_BUTTON_OK" };
static email_string_t EMAIL_VIEWER_HEADER_UNABLE_TO_OPEN_FILE = { PACKAGE, "IDS_EMAIL_HEADER_UNABLE_TO_OPEN_FILE_ABB" };
static email_string_t EMAIL_VIEWER_POP_THIS_FILE_TYPE_IS_NOT_SUPPORTED = { PACKAGE, "IDS_EMAIL_POP_THIS_FILE_TYPE_IS_NOT_SUPPORTED_BY_ANY_APPLICATION_ON_YOUR_DEVICE" };
static email_string_t EMAIL_VIEWER_POP_FAILED_TO_DELETE = { PACKAGE, N_("IDS_EMAIL_POP_FAILED_TO_DELETE") };
static email_string_t EMAIL_VIEWER_POP_FAILED_TO_MOVE = { PACKAGE, N_("IDS_EMAIL_POP_FAILED_TO_MOVE") };
static email_string_t EMAIL_VIEWER_STRING_NULL = { NULL, NULL };


typedef enum {
	EMAIL_VIEWER_LINK_NONE,
	EMAIL_VIEWER_LINK_PHONE,
	EMAIL_VIEWER_LINK_EMAIL,
	EMAIL_VIEWER_LINK_RTSP,
	EMAIL_VIEWER_LINK_URL,
	EMAIL_VIEWER_LINK_MAX
} EmailViewerLinkType;

#define EMAIL_VIEWER_LIST_ITEM_LINE_COUNT_LANDSCAPE 4
#define EMAIL_VIEWER_LIST_ITEM_LINE_COUNT_PORTRAIT 7

static void _mouseup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _show_attachment(EV_attachment_data *aid);
static void  _viewer_storage_full_popup_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static int _get_filepath_from_path(const char *default_path, char *origin_path, char *new_path);

static void _mouseup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();
	Evas_Event_Mouse_Up *ev = event_info;
	if (ev->button == 3) { /* if mouse right button is up */
		DELETE_EVAS_OBJECT(obj);
	}
}

EMAIL_DEFINE_GET_EDJ_PATH(email_get_viewer_theme_path, "/email-viewer.edj")

EMAIL_DEFINE_GET_DATA_PATH(email_get_viewer_tmp_dir, "/.email-viewer-efl")

Evas_Object *util_notify_genlist_add(Evas_Object *parent)
{
	debug_enter();
	retvm_if(parent == NULL, NULL, "Invalid parameter: parent[NULL]");

	Evas_Object *genlist = elm_genlist_add(parent);
	elm_scroller_content_min_limit(genlist, EINA_FALSE, EINA_TRUE);
	elm_genlist_homogeneous_set(genlist, EINA_TRUE);
	elm_scroller_policy_set(genlist, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
	evas_object_show(genlist);

	return genlist;
}

static void _popup_response_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;
	DELETE_EVAS_OBJECT(view->notify);
	debug_leave();
}


#define LIST_POPUP_BUTTON1_CB "list_popup_button1_cb"
#define LIST_POPUP_BUTTON2_CB "list_popup_button2_cb"
#define LIST_POPUP_FOCUS_OUT_CB "list_popup_focus_out_cb"
#define LIST_POPUP_BLOCK_CLICKED_CB "list_popup_block_clicked_cb"
#define LIST_POPUP_BACK_CB "list_popup_back_cb"
static void _popup_content_remove(Evas_Object *popup)
{
	debug_enter();
	Evas_Object *content = NULL;
	Evas_Object *btn1 = NULL;
	Evas_Object *btn2 = NULL;
	Evas_Smart_Cb btn1_cb = NULL;
	Evas_Smart_Cb btn2_cb = NULL;
	Evas_Smart_Cb block_clicked_cb = NULL;

	content = elm_object_content_get(popup);
	if (content) {
		elm_object_content_unset(popup);
		DELETE_EVAS_OBJECT(content);
	}

	btn1_cb = (Evas_Smart_Cb)evas_object_data_get(popup, LIST_POPUP_BUTTON1_CB);
	if (btn1_cb) {
		btn1 = elm_object_part_content_get(popup, "button1");
		if (btn1) {
			elm_object_part_content_unset(popup, "button1");
			DELETE_EVAS_OBJECT(btn1);
		}
	}

	btn2_cb = (Evas_Smart_Cb)evas_object_data_get(popup, LIST_POPUP_BUTTON2_CB);
	if (btn2_cb) {
		btn2 = elm_object_part_content_get(popup, "button2");
		if (btn2) {
			elm_object_part_content_unset(popup, "button2");
			DELETE_EVAS_OBJECT(btn2);
		}
	}

	block_clicked_cb = (Evas_Smart_Cb)evas_object_data_get(popup, LIST_POPUP_BLOCK_CLICKED_CB);
	if (block_clicked_cb) {
		evas_object_smart_callback_del(popup, "block,clicked", block_clicked_cb);
	}

	Eext_Event_Cb back_cb = (Eext_Event_Cb)evas_object_data_get(popup, LIST_POPUP_BACK_CB);
	if (back_cb) {
		eext_object_event_callback_del(popup, EEXT_CALLBACK_BACK, back_cb);
	}

}

Evas_Object *util_create_notify_with_list(EmailViewerView *view, email_string_t t_header, email_string_t t_content,
						int btn_num, email_string_t t_btn1_lb, popup_cb resp_cb1,
						email_string_t t_btn2_lb, popup_cb resp_cb2, popup_cb resp_block_cb)
{
	debug_enter();
	retvm_if(view == NULL, NULL, "Invalid parameter: view[NULL]");

	Evas_Object *notify = NULL;

	if (view->notify) {
		_popup_content_remove(view->notify);
		notify = view->notify;
	} else {
		notify = elm_popup_add(view->base.module->win);
		elm_popup_align_set(notify, ELM_NOTIFY_ALIGN_FILL, 1.0);
		retvm_if(notify == NULL, NULL, "elm_popup_add returns NULL.");
		debug_log("notify: %p, win_main: %p", notify, view->base.module->win);
	}

	if (t_header.id) {
		if (t_header.domain) {
			elm_object_domain_translatable_part_text_set(notify, "title,text", t_header.domain, t_header.id);
		} else {
			elm_object_part_text_set(notify, "title,text", t_header.id);
		}
	}

	if (t_content.id) {
		if (t_content.domain) {
			elm_object_domain_translatable_text_set(notify, t_content.domain, t_content.id);
		} else {
			elm_object_text_set(notify, t_content.id);
		}
	}

	if (btn_num == 0 && resp_block_cb) {
		elm_popup_timeout_set(notify, 2.0);
		evas_object_smart_callback_add(notify, "timeout", resp_block_cb, view);
		evas_object_smart_callback_add(notify, "block,clicked", resp_block_cb, view);
		evas_object_data_set(notify, LIST_POPUP_BLOCK_CLICKED_CB, (void *)resp_block_cb);
		eext_object_event_callback_add(notify, EEXT_CALLBACK_BACK, resp_block_cb, view);
		evas_object_data_set(notify, LIST_POPUP_BACK_CB, (void *)resp_block_cb);
	}

	if (btn_num == 1) {
		if (g_strcmp0(t_btn1_lb.id, "IDS_EMAIL_BUTTON_CANCEL") != 0) { /* show btn1 if it is not cancel button */
			Evas_Object *btn1 = elm_button_add(notify);
			elm_object_style_set(btn1, "popup");
			if (t_btn1_lb.id) {
				if (t_btn1_lb.domain) {
					elm_object_domain_translatable_text_set(btn1, t_btn1_lb.domain, t_btn1_lb.id);
				} else {
					elm_object_text_set(btn1, t_btn1_lb.id);
				}
			}
			elm_object_part_content_set(notify, "button1", btn1);
			evas_object_smart_callback_add(btn1, "clicked", resp_cb1, view);
			evas_object_data_set(notify, LIST_POPUP_BUTTON1_CB, (void *)resp_cb1);
		} else {
			evas_object_smart_callback_add(notify, "block,clicked", resp_cb1, view);
			evas_object_data_set(notify, LIST_POPUP_BLOCK_CLICKED_CB, (void *)resp_cb1);
		}
		eext_object_event_callback_add(notify, EEXT_CALLBACK_BACK, resp_cb1, view);
		evas_object_data_set(notify, LIST_POPUP_BACK_CB, (void *)resp_cb1);
	}
	if (btn_num == 2) {
		Evas_Object *btn1 = elm_button_add(notify);
		elm_object_style_set(btn1, "popup");
		if (t_btn1_lb.id) {
			if (t_btn1_lb.domain) {
				elm_object_domain_translatable_text_set(btn1, t_btn1_lb.domain, t_btn1_lb.id);
			} else {
				elm_object_text_set(btn1, t_btn1_lb.id);
			}
		}
		elm_object_part_content_set(notify, "button1", btn1);
		evas_object_smart_callback_add(btn1, "clicked", resp_cb1, view);
		evas_object_data_set(notify, LIST_POPUP_BUTTON1_CB, (void *)resp_cb1);
		eext_object_event_callback_add(notify, EEXT_CALLBACK_BACK, resp_cb1, view);
		evas_object_data_set(notify, LIST_POPUP_BACK_CB, (void *)resp_cb1);

		Evas_Object *btn2 = elm_button_add(notify);
		elm_object_style_set(btn2, "popup");
		if (t_btn2_lb.id) {
			if (t_btn2_lb.domain) {
				elm_object_domain_translatable_text_set(btn2, t_btn2_lb.domain, t_btn2_lb.id);
			} else {
				elm_object_text_set(btn2, t_btn2_lb.id);
			}
		}
		elm_object_part_content_set(notify, "button2", btn2);
		evas_object_smart_callback_add(btn2, "clicked", resp_cb2, view);
		evas_object_data_set(notify, LIST_POPUP_BUTTON2_CB, (void *)resp_cb2);
	}

	evas_object_event_callback_add(notify, EVAS_CALLBACK_MOUSE_UP, _mouseup_cb, view);
	evas_object_show(notify);

	debug_leave();
	return notify;
}

void util_create_notify(EmailViewerView *view, email_string_t t_header, email_string_t t_content,
						int btn_num, email_string_t t_btn1_lb, popup_cb resp_cb1,
						email_string_t t_btn2_lb, popup_cb resp_cb2, popup_cb resp_block_cb)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");
	retm_if(view->base.module->win == NULL, "Invalid parameter: view->base.module->win[NULL]");
	Evas_Object *notify = NULL;

	DELETE_EVAS_OBJECT(view->notify);
	notify = elm_popup_add(view->base.module->win);
	elm_popup_align_set(notify, ELM_NOTIFY_ALIGN_FILL, 1.0);
	retm_if(notify == NULL, "elm_popup_add returns NULL.");
	debug_log("notify: %p, win_main: %p", notify, view->base.module->win);
	evas_object_size_hint_weight_set(notify, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	view->notify = notify;

	if (t_header.id) {
		if (t_header.domain) {
			elm_object_domain_translatable_part_text_set(notify, "title,text", t_header.domain, t_header.id);
		} else {
			elm_object_part_text_set(notify, "title,text", t_header.id);
		}
	}

	if (t_content.id) {
		if (t_content.domain) {
			elm_object_domain_translatable_text_set(notify, t_content.domain, t_content.id);
		} else {
			elm_object_text_set(notify, t_content.id);
		}
	}

	if (btn_num == 0 && resp_block_cb) {
		elm_popup_timeout_set(notify, 2.0);
		evas_object_smart_callback_add(notify, "timeout", resp_block_cb, view);
		evas_object_smart_callback_add(notify, "block,clicked", resp_block_cb, view);
		eext_object_event_callback_add(notify, EEXT_CALLBACK_BACK, resp_block_cb, view);
	}

	if (btn_num == 1) {
		Evas_Object *btn1 = elm_button_add(notify);
		elm_object_style_set(btn1, "popup");
		if (t_btn1_lb.id) {
			if (t_btn1_lb.domain) {
				elm_object_domain_translatable_text_set(btn1, t_btn1_lb.domain, t_btn1_lb.id);
			} else {
				elm_object_text_set(btn1, t_btn1_lb.id);
			}
		}
		elm_object_part_content_set(notify, "button1", btn1);
		evas_object_smart_callback_add(btn1, "clicked", resp_cb1, view);
		eext_object_event_callback_add(notify, EEXT_CALLBACK_BACK, resp_cb1, view);
	} else if (btn_num == 2) {
		Evas_Object *btn1 = elm_button_add(notify);
		elm_object_style_set(btn1, "popup");
		if (t_btn1_lb.id) {
			if (t_btn1_lb.domain) {
				elm_object_domain_translatable_text_set(btn1, t_btn1_lb.domain, t_btn1_lb.id);
			} else {
				elm_object_text_set(btn1, t_btn1_lb.id);
			}
		}
		elm_object_part_content_set(notify, "button1", btn1);
		evas_object_smart_callback_add(btn1, "clicked", resp_cb1, view);
		eext_object_event_callback_add(notify, EEXT_CALLBACK_BACK, resp_cb1, view);

		Evas_Object *btn2 = elm_button_add(notify);
		elm_object_style_set(btn2, "popup");
		if (t_btn2_lb.id) {
			if (t_btn2_lb.domain) {
				elm_object_domain_translatable_text_set(btn2, t_btn2_lb.domain, t_btn2_lb.id);
			} else {
				elm_object_text_set(btn2, t_btn2_lb.id);
			}
		}
		elm_object_part_content_set(notify, "button2", btn2);
		evas_object_smart_callback_add(btn2, "clicked", resp_cb2, view);
	}

	evas_object_event_callback_add(notify, EVAS_CALLBACK_MOUSE_UP, _mouseup_cb, view);

	evas_object_show(notify);
	debug_leave();
}

int _find_folder_id_using_folder_type(EmailViewerView *view, email_mailbox_type_e mailbox_type)
{
	debug_enter();
	retvm_if(view == NULL, -1, "Invalid parameter: view[NULL]");
	retvm_if(view->folder_list == NULL, -1, "Invalid parameter: view->folder_list[NULL]");

	int i = 0;
	debug_log("mailbox_type:%d", mailbox_type);

	LIST_ITER_START(i, view->folder_list) {
		email_mailbox_name_and_alias_t *nameandalias = (email_mailbox_name_and_alias_t *) LIST_ITER_GET_DATA(i, view->folder_list);
		int folder_id = nameandalias->mailbox_id;
		if (mailbox_type == nameandalias->mailbox_type) {
			debug_secure("folder_id: %d, name: %s, alias: %s", folder_id, nameandalias->name, nameandalias->alias);
			debug_leave();
			return folder_id;
		}
	}
	return -1;
}

int _find_folder_type_using_folder_id(EmailViewerView *view, int mailbox_id)
{
	debug_enter();
	retvm_if(view == NULL, -1, "Invalid parameter: view[NULL]");
	retvm_if(view->folder_list == NULL, -1, "Invalid parameter: view->folder_list[NULL]");

	int i = 0;
	debug_log("mailbox_id:%d", mailbox_id);

	LIST_ITER_START(i, view->folder_list) {
		email_mailbox_name_and_alias_t *nameandalias = (email_mailbox_name_and_alias_t *) LIST_ITER_GET_DATA(i, view->folder_list);
		int folder_type = nameandalias->mailbox_type;
		if (mailbox_id == nameandalias->mailbox_id) {
			debug_secure("folder_type: %d, name: %s, alias: %s", folder_type, nameandalias->name, nameandalias->alias);
			debug_leave();
			return folder_type;
		}
	}
	return -1;
}

int viewer_move_email(EmailViewerView *view, int dest_folder_id, gboolean is_delete, gboolean is_block)
{
	debug_enter();
	retvm_if(view == NULL, 0, "Invalid parameter: view[NULL]");

	if (!email_engine_move_mail(dest_folder_id, view->mail_id)) {
		debug_log("Moving email is failed.");

		if (is_delete)
			util_create_notify(view, EMAIL_VIEWER_POP_ERROR,
					EMAIL_VIEWER_POP_FAILED_TO_DELETE, 1,
					EMAIL_VIEWER_STRING_OK, _popup_response_cb, EMAIL_VIEWER_STRING_NULL, NULL, NULL);
		else
			util_create_notify(view, EMAIL_VIEWER_POP_ERROR,
					EMAIL_VIEWER_POP_FAILED_TO_MOVE, 1,
					EMAIL_VIEWER_STRING_OK, _popup_response_cb, EMAIL_VIEWER_STRING_NULL, NULL, NULL);

		return -1;
	} else {
		debug_log("account_id: %d, moveto folder id: %d, mail_id: %d", view->account_id, dest_folder_id, view->mail_id);
		debug_leave();
		return 1;
	}
}

int viewer_delete_email(EmailViewerView *view)
{
	debug_enter();
	retvm_if(view == NULL, 0, "Invalid parameter: view[NULL]");

	int sync = EMAIL_DELETE_LOCAL_AND_SERVER;
	if (view->account_type == EMAIL_SERVER_TYPE_POP3) {
		email_account_t *account_data = NULL;
		if (email_engine_get_account_full_data(view->account_id, &account_data)) {
			debug_log("email_engine_get_account");
			if (account_data) {
				account_user_data_t *user_data = (account_user_data_t *)account_data->user_data;
				if (user_data != NULL) {
					debug_log("pop3_deleting_option:%d", user_data->pop3_deleting_option);
					if (user_data->pop3_deleting_option == 0) {
						sync = EMAIL_DELETE_LOCALLY;
					} else if (user_data->pop3_deleting_option == 1) {
						sync = EMAIL_DELETE_LOCAL_AND_SERVER;
					}
				}
			}
		}

		email_engine_free_account_list(&account_data, 1);
	}

	if (!email_engine_delete_mail(view->mailbox_id, view->mail_id, sync)) {
		debug_log("Deleting email is failed.");
		util_create_notify(view, EMAIL_VIEWER_POP_ERROR,
				EMAIL_VIEWER_POP_FAILED_TO_DELETE, 1,
				EMAIL_VIEWER_STRING_OK, _popup_response_cb, EMAIL_VIEWER_STRING_NULL, NULL, NULL);
		return 0;
	}

	debug_log("account_id: %d, mailbox_id: %d, mail_id: %d", view->account_id, view->mailbox_id, view->mail_id);
	return 1;
}

static int _get_filepath_from_path(const char *default_path, char *origin_path, char *new_path)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(STR_VALID(origin_path), EMAIL_EXT_SAVE_ERR_UNKNOWN);

	int err = EMAIL_EXT_SAVE_ERR_NONE;
	gchar tmp_path[MAX_PATH_LEN] = { 0, };
	gchar new_filename[MAX_PATH_LEN] = { 0, };
	gchar prefix[MAX_PATH_LEN] = { 0, };
	gint max_length = MAX_PATH_LEN;

	snprintf(prefix, sizeof(prefix), "%s", default_path);

	debug_secure("prefix:%s", prefix);

	memset(new_path, 0, sizeof(MAX_PATH_LEN));

	if (STR_LEN(tmp_path) == 0) {
		g_snprintf(tmp_path, sizeof(tmp_path), "%s", origin_path);
	}

	gchar *file_name = NULL;
	gchar *file_ext = NULL;
	gchar *file_path = email_parse_get_filepath_from_path(tmp_path);
	email_parse_get_filename_n_ext_from_path(tmp_path, &file_name, &file_ext);

	debug_secure("file_name:%s, file_ext:%s", file_name, file_ext);

	if (file_ext != NULL) {
		char *save_ptr = NULL;
		char *new_file_ext = strtok_r(file_ext, "?", &save_ptr);
		if (new_file_ext != file_ext) {
			new_file_ext = g_strdup(new_file_ext ? new_file_ext : "");
			g_free(file_ext);
			file_ext = new_file_ext;
		}
	} else {
		file_ext = g_strdup("");
	}

	debug_secure("file_ext:%s", file_ext);

	if (file_name != NULL) {
		debug_secure("file_name:%s", file_name);
		int num = 1;
		if (strlen(file_name) + strlen(file_ext) > max_length - STR_LEN(prefix)) {
			gint available_len = max_length - STR_LEN(prefix);

			if (strlen(file_ext) > 0) {
				available_len -= strlen(file_ext);
			}

			gchar *new_name = email_cut_text_by_byte_len(file_name, available_len);

			if (STR_VALID(new_name)) {
				g_snprintf(new_path, MAX_PATH_LEN, "%s/%s%s", prefix, new_name, file_ext);
				g_snprintf(new_filename, sizeof(new_filename), "%s%s", new_name, file_ext);
				g_free(new_name);	/* MUST BE. */
			}
		} else {
			g_snprintf(new_path, MAX_PATH_LEN, "%s/%s%s", prefix, file_name, file_ext);
		}

		do {
			if (access(new_path, F_OK) != -1) {
				debug_log("file existed");
				memset(new_path, 0, sizeof(MAX_PATH_LEN));
				gint available_len = max_length - STR_LEN(prefix);
				if (strlen(file_ext) > 0) {
					available_len -= strlen(file_ext);
				}
				gchar *new_name = email_cut_text_by_byte_len(file_name, available_len);

				if (STR_VALID(new_name)) {
					g_snprintf(new_path, MAX_PATH_LEN, "%s/%s (%d)%s", prefix, new_name, ++num, file_ext);
					g_free(new_name);
				}
			} else {
				debug_log("file path is valid.");
				break;
			}
		} while (1);
	}

	if (STR_VALID(file_path)) {
		g_free(file_path);	/* MUST BE. */
	}

	if (STR_VALID(file_name)) {
		g_free(file_name);	/* MUST BE. */
	}

	if (STR_VALID(file_ext)) {
		g_free(file_ext);	/* MUST BE. */
	}

	debug_secure("new_path:%s", new_path);
	debug_leave();
	return err;
}

static void _module_destroy_request_cb(void *data, email_module_h module)
{
	debug_enter();
	retm_if(module == NULL, "Invalid parameter: module[NULL]");
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;

	if (view->base.content) {
		elm_object_tree_focus_allow_set(view->base.content, EINA_TRUE);
	}

	debug_log("need_pending_destroy: %d", view->need_pending_destroy);

	if (view->need_pending_destroy) {
		debug_log("email_module_make_destroy_request");
		email_module_make_destroy_request(view->base.module);
	} else {
		email_module_destroy(module);
	}

	view->loaded_module = NULL;

	if (view->is_composer_module_launched) {
		debug_log("composer module destroyed");
		view->is_composer_module_launched = FALSE;
	}

	debug_leave();
}

static void _scheduled_composer_destroy_request_cb(void *data, email_module_h module)
{
	debug_enter();
	retm_if(module == NULL, "Invalid parameter: module[NULL]");
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;

	if (view->base.content) {
		elm_object_tree_focus_allow_set(view->base.content, EINA_TRUE);
	}

	if (view->need_pending_destroy || (view->save_status == EMAIL_MAIL_STATUS_SEND_SCHEDULED)) {
		debug_log("email_module_make_destroy_request");
		email_module_make_destroy_request(view->base.module);
	} else {
		debug_log("email_module_destroy");
		email_module_destroy(module);
	}

	view->loaded_module = NULL;

	debug_leave();
}

static void _account_destroy_request_cb(void *data, email_module_h module)
{
	debug_enter();
	retm_if(module == NULL, "Invalid parameter: module[NULL]");
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;

	if (view->base.content) {
		elm_object_tree_focus_allow_set(view->base.content, EINA_TRUE);
	}

	if (view->move_status) {
		debug_log("email_module_make_destroy_request");
		email_module_make_destroy_request(view->base.module);
	} else {
		email_module_destroy(module);
	}

	view->loaded_module = NULL;

	debug_leave();
}

static void _account_result_cb(void *data, email_module_h module, email_params_h result)
{
	debug_enter();
	retm_if(module == NULL, "Invalid parameter: module[NULL]");
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;

	int is_move_mail_mode = 0;
	email_params_get_int_opt(result, EMAIL_BUNDLE_KEY_IS_MAILBOX_MOVE_MODE, &is_move_mail_mode);

	if (is_move_mail_mode) {
		int move_status = 0;
		email_params_get_int_opt(result, EMAIL_BUNDLE_KEY_MAILBOX_MOVE_STATUS, &move_status);

		if (move_status == EMAIL_ERROR_NONE) {
			char str[BUF_LEN_L] = { 0, };
			const char *mailbox_name = NULL;
			if (email_params_get_str(result, EMAIL_BUNDLE_KEY_MAILBOX_MOVED_MAILBOX_NAME, &mailbox_name)) {
				snprintf(str, sizeof(str), email_get_email_string("IDS_EMAIL_TPOP_1_EMAIL_MOVED_TO_PS"), mailbox_name);
			} else {
				snprintf(str, sizeof(str), "%s", email_get_email_string("IDS_EMAIL_TPOP_1_EMAIL_MOVED_TO_SELECTED_FOLDER"));
			}
			int ret = notification_status_message_post(str);
			debug_warning_if(ret != NOTIFICATION_ERROR_NONE, "notification_status_message_post() failed! ret:(%d)", ret);
			view->move_status = move_status;
		}
	}
	if (view->loaded_module) {
		_account_destroy_request_cb(view, view->loaded_module);
	}
}

email_module_h viewer_create_module(void *data, email_module_type_e module_type, email_params_h params)
{
	debug_enter();
	retvm_if(data == NULL, NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	email_module_listener_t listener = { 0 };
	listener.cb_data = view;
	if (module_type == EMAIL_MODULE_ACCOUNT) {
		listener.result_cb = _account_result_cb;
		listener.destroy_request_cb = _account_destroy_request_cb;
	} else {
		listener.destroy_request_cb = _module_destroy_request_cb;
	}

	if (module_type == EMAIL_MODULE_COMPOSER) {
		debug_log("composer module launched");
		view->is_composer_module_launched = TRUE;
	}

	email_module_h module = email_module_create_child(view->base.module, module_type, params, &listener);
	if (view->base.content)
		elm_object_tree_focus_allow_set(view->base.content, EINA_FALSE);

	return module;
}

email_module_h viewer_create_scheduled_composer_module(void *data, email_params_h params)
{
	debug_enter();
	retvm_if(data == NULL, NULL, "Invalid parameter: data[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;

	email_module_listener_t listener = { 0 };
	listener.cb_data = view;
	listener.destroy_request_cb = _scheduled_composer_destroy_request_cb;

	email_module_h module = email_module_create_child(view->base.module,
			EMAIL_MODULE_COMPOSER, params, &listener);
	if (view->base.content)
		elm_object_tree_focus_allow_set(view->base.content, EINA_FALSE);

	return module;
}

Evas_Object *viewer_load_edj(Evas_Object *parent, const char *file, const char *group, double hintx, double hinty)
{
	retvm_if(parent == NULL, NULL, "Invalid parameter: parent[NULL]");
	Evas_Object *eo;
	int r;

	eo = elm_layout_add(parent);
	if (eo) {
		r = elm_layout_file_set(eo, file, group);
		if (!r) {
			DELETE_EVAS_OBJECT(eo);
			return NULL;
		}

		evas_object_size_hint_weight_set(eo, hintx, hinty);
	}
	return eo;
}

void viewer_launch_browser(EmailViewerView *view, const char *link_url)
{
	debug_enter();
	retm_if(link_url == NULL, "Invalid parameter: link_url[NULL]");
	debug_secure("link_url (%s)", link_url);

	int ret = APP_CONTROL_ERROR_NONE;
	app_control_h app_control = NULL;
	ret = app_control_create(&app_control);
	retm_if(ret != APP_CONTROL_ERROR_NONE, "app_control create failed: app_control[NULL]");

	ret = app_control_set_launch_mode(app_control, APP_CONTROL_LAUNCH_MODE_GROUP);
	debug_log("app_control_set_launch_mode: %d", ret);
	ret = app_control_set_operation(app_control, APP_CONTROL_OPERATION_VIEW);
	debug_log("app_control_set_operation: %d", ret);
	ret = app_control_set_uri(app_control, link_url);
	debug_log("app_control_set_uri: %d", ret);
	ret = app_control_send_launch_request(app_control, NULL, NULL);
	debug_log("app_control_send_launch_request: %d", ret);
	if (ret != APP_CONTROL_ERROR_NONE) {
		notification_status_message_post(_("IDS_EMAIL_HEADER_UNABLE_TO_PERFORM_ACTION_ABB"));
	}
	ret = app_control_destroy(app_control);
	debug_log("app_control_destroy: %d", ret);

	debug_leave();
}

void viewer_create_email(EmailViewerView *view, const char *email_address)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");
	retm_if(email_address == NULL, "Invalid parameter: email_address[NULL]");
	debug_secure("email_address (%s)", email_address);
	int scheme_length = strlen(URI_SCHEME_MAILTO);
	email_params_h params = NULL;

	if (email_params_create(&params) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_RUN_TYPE, RUN_COMPOSER_EXTERNAL) &&
		email_params_add_str(params, EMAIL_BUNDLE_KEY_TO, &email_address[scheme_length])) {

		view->loaded_module = viewer_create_module(view, EMAIL_MODULE_COMPOSER, params);
	}

	email_params_free(&params);

	debug_leave();
}

email_ext_save_err_type_e viewer_save_attachment_in_downloads(EV_attachment_data *aid, gboolean(*copy_file_cb) (void *data, float percentage))
{
	debug_enter();
	retvm_if(aid == NULL, EMAIL_EXT_SAVE_ERR_UNKNOWN, "Invalid parameter: aid[NULL]");
	EmailAttachmentType *info = aid->attachment_info;

	debug_secure("attachment file path = %s", info->path);

	email_ext_save_err_type_e ret_val = EMAIL_EXT_SAVE_ERR_NONE;
	int mc_ret = 0;
	gchar new_path[MAX_PATH_LEN] = {'\0'};

	ret_val = _get_filepath_from_path(email_get_phone_downloads_dir(), info->path, new_path);
	if (ret_val != EMAIL_EXT_SAVE_ERR_NONE) {
		debug_error("email_get_filepath_from_path() failed!");
		return ret_val;
	}

	debug_secure("new_path:%s", new_path);

	mc_ret = media_content_connect();
	if (mc_ret == MEDIA_CONTENT_ERROR_NONE) {

		gboolean saved = email_copy_file_feedback(info->path, new_path, copy_file_cb, aid);
		ret_val = saved ? EMAIL_EXT_SAVE_ERR_NONE : EMAIL_EXT_SAVE_ERR_UNKNOWN;
		debug_log("saved(%d), err(%d), errno(%d)", saved, ret_val, errno);
		if (errno == ENOSPC) {
			ret_val = EMAIL_EXT_SAVE_ERR_NO_FREE_SPACE;
		}

		mc_ret = media_content_scan_file(new_path);
		debug_log("media_content_scan_file: %d", mc_ret);

		mc_ret = media_content_disconnect();
		if (mc_ret != MEDIA_CONTENT_ERROR_NONE) {
			debug_log("media_content_disconnect() is failed!");
		}
	} else {
		debug_log("media_content_connect() is failed!");
	}

	debug_log("return value = %d", ret_val);
	return ret_val;
}

email_ext_save_err_type_e viewer_save_attachment_for_preview(EV_attachment_data *aid, gboolean(*copy_file_cb) (void *data, float percentage))
{
	debug_enter();
	retvm_if(aid == NULL, EMAIL_EXT_SAVE_ERR_UNKNOWN, "Invalid parameter: aid[NULL]");
	EmailAttachmentType *info = aid->attachment_info;

	debug_secure("attachment file path = %s", info->path);

	email_ext_save_err_type_e ret_val = EMAIL_EXT_SAVE_ERR_NONE;
	gboolean saved = FALSE;

	if (!aid->preview_path) {
		debug_error("aid->preview_path is NULL!");
		return EMAIL_EXT_SAVE_ERR_UNKNOWN;
	}

	debug_secure("preview_path: %s", aid->preview_path);

	saved = email_copy_file_feedback(info->path, aid->preview_path, copy_file_cb, aid);
	ret_val = saved ? EMAIL_EXT_SAVE_ERR_NONE : EMAIL_EXT_SAVE_ERR_UNKNOWN;
	debug_log("saved(%d), err(%d), errno(%d)", saved, ret_val, errno);
	if (errno == ENOSPC) {
		ret_val = EMAIL_EXT_SAVE_ERR_NO_FREE_SPACE;
	}



	debug_log("return value = %d", ret_val);
	return ret_val;
}

void viewer_show_attachment_preview(EV_attachment_data *aid)
{
	debug_enter();
	retm_if(aid == NULL, "Invalid parameter: aid[NULL]");
	EmailViewerView *view = aid->view;

	if (aid != view->preview_aid) {
		debug_error("Not ready for preview!");
		return;
	}
	view->preview_aid = NULL;

	_show_attachment(aid);

	debug_leave();
}

static void _show_attachment(EV_attachment_data *aid)
{
	debug_enter();
	EmailViewerView *view = aid->view;

	int ret;
	ret = email_preview_attachment_file(view->base.module, aid->preview_path, NULL);
	if (ret != 0) {
		util_create_notify(view, EMAIL_VIEWER_HEADER_UNABLE_TO_OPEN_FILE,
				EMAIL_VIEWER_POP_THIS_FILE_TYPE_IS_NOT_SUPPORTED, 1,
				EMAIL_VIEWER_STRING_OK, _popup_response_cb, EMAIL_VIEWER_STRING_NULL, NULL, NULL);
	}

	debug_leave();
}

int viewer_check_storage_full(unsigned int mb_size)
{
	debug_enter();

	bool ret = false;
	unsigned long int need_size = mb_size * 1024 * 1024;
	struct statvfs s;

	int r = storage_get_internal_memory_size(&s);
	if (r < 0) {
		debug_error("storage_get_internal_memory_size() failed! ret:[%d]", r);
		return true;
	}

	if ((need_size / s.f_bsize) > s.f_bavail) {
		debug_secure_error("Not enough free space! [%lf]", (double)s.f_bsize * s.f_bavail);
		ret = true;
	}

	debug_leave();
	return ret;
}

void viewer_show_storage_full_popup(EmailViewerView *view)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	if (view->is_storage_full_popup_shown) {
		debug_log("Popup is already dispalyed. No need to show another one");
		return;
	}
	view->is_storage_full_popup_shown = EINA_TRUE;
	email_string_t content = { PACKAGE, "IDS_EMAIL_POP_THERE_IS_NOT_ENOUGH_SPACE_IN_YOUR_DEVICE_STORAGE_GO_TO_SETTINGS_POWER_AND_STORAGE_STORAGE_THEN_DELETE_SOME_FILES_AND_TRY_AGAIN" };
	email_string_t btn2 = { PACKAGE, "IDS_EMAIL_BUTTON_SETTINGS" };
	util_create_notify(view, EMAIL_VIEWER_POP_ERROR, content, 2,
			EMAIL_VIEWER_STRING_OK, _popup_response_cb,
			btn2, viewer_storage_full_popup_response_cb, NULL);

	evas_object_event_callback_add(view->notify, EVAS_CALLBACK_DEL, _viewer_storage_full_popup_del_cb, view);
	debug_leave();
}

static void  _viewer_storage_full_popup_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	view->is_storage_full_popup_shown = EINA_FALSE;
	debug_leave();

}

static void _viewer_launch_storage_settings(EmailViewerView *view)
{
	debug_enter();
	int ret = APP_CONTROL_ERROR_NONE;
	app_control_h app_control = NULL;

	ret = app_control_create(&app_control);
	debug_warning_if((ret != APP_CONTROL_ERROR_NONE || !app_control), "app_control_create() failed! ret:[%d]", ret);

	/* Do make prerequisites. */
	ret = app_control_set_launch_mode(app_control, APP_CONTROL_LAUNCH_MODE_GROUP);
	debug_warning_if(ret != APP_CONTROL_ERROR_NONE, "app_control_set_launch_mode() failed! ret:[%d]", ret);

	/* Set some data for launching application. */
	ret = app_control_set_app_id(app_control, "setting-storage-efl");
	debug_warning_if(ret != APP_CONTROL_ERROR_NONE, "app_control_set_app_id() failed! ret:[%d]", ret);

	/* Launch application */
	ret = app_control_send_launch_request(app_control, NULL, view);
	debug_warning_if(ret != APP_CONTROL_ERROR_NONE, "app_control_send_launch_request() failed! ret:[%d]", ret);
	if (ret != APP_CONTROL_ERROR_NONE) {
		notification_status_message_post(_("IDS_EMAIL_HEADER_UNABLE_TO_PERFORM_ACTION_ABB"));
	}

	ret = app_control_destroy(app_control);
	debug_warning_if(ret != APP_CONTROL_ERROR_NONE, "app_control_destroy() failed! ret:[%d]", ret);

	debug_leave();
}

void viewer_storage_full_popup_response_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	DELETE_EVAS_OBJECT(view->notify);

	_viewer_launch_storage_settings(view);

	debug_leave();
}

void viewer_create_down_progress(void *data, email_string_t t, popup_cb resp_cb)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;
	Evas_Object *popup, *pb;
	view->is_download_message_btn_clicked = EINA_TRUE;
	popup = elm_popup_add(view->base.module->win);
	elm_popup_align_set(popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
	retm_if(popup == NULL, "elm_popup_add returns NULL.");
	debug_log("popup: %p, win_main: %p", popup, view->base.module->win);
	view->pb_notify = popup;

	elm_object_domain_translatable_part_text_set(popup, "title,text", t.domain, t.id);

	Evas_Object *layout = viewer_load_edj(popup, email_get_viewer_theme_path(), "popup_text_progressbar_view_layout", EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, 0.5);
	evas_object_show(layout);
	elm_object_part_text_set(layout, "elm.text.description", email_get_email_string("IDS_EMAIL_POP_DOWNLOADING_ING"));

	pb = elm_progressbar_add(layout);

	elm_object_style_set(pb, "pending");
	elm_progressbar_pulse(pb, EINA_TRUE);
	elm_progressbar_horizontal_set(pb, EINA_TRUE);
	evas_object_size_hint_align_set(pb, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(pb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(pb);
	elm_progressbar_value_set(pb, 0.0);
	elm_object_part_text_set(pb, "elm.text.bottom.left", "0.0/0.0");
	elm_object_part_text_set(pb, "elm.text.bottom.right", "0%");
	view->pb_notify_lb = pb;
	elm_object_part_content_set(layout, "progressbar", pb);
	elm_object_content_set(popup, layout);

	Evas_Object *btn = elm_button_add(popup);
	elm_object_style_set(btn, "popup");
	elm_object_domain_translatable_text_set(btn, PACKAGE, "IDS_EMAIL_BUTTON_CANCEL");
	elm_object_part_content_set(popup, "button1", btn);
	evas_object_smart_callback_add(btn, "clicked", resp_cb, view);
	debug_log("Cancel btn1 for downloading body: %p", btn);

	evas_object_event_callback_add(popup, EVAS_CALLBACK_MOUSE_UP, _mouseup_cb, view);
	eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, resp_cb, view);
	evas_object_show(popup);
	debug_leave();
}

void viewer_destroy_down_progress_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;
	view->is_download_message_btn_clicked = EINA_FALSE;

	email_engine_stop_working(view->account_id, view->email_handle);
	view->email_handle = 0;

	DELETE_EVAS_OBJECT(view->pb_notify);
	DELETE_EVAS_OBJECT(view->pb_notify_lb);
	debug_leave();
}

void viewer_send_email(void *data, char *email_address)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	retm_if(email_address == NULL, "Invalid parameter: email_address[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;

	email_params_h params = NULL;

	if (email_params_create(&params) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_RUN_TYPE, RUN_COMPOSER_EXTERNAL) &&
		email_params_add_str(params, EMAIL_BUNDLE_KEY_TO, email_address)) {

		view->loaded_module = viewer_create_module(view, EMAIL_MODULE_COMPOSER, params);
	}

	email_params_free(&params);

	debug_leave();
}

void viewer_view_contact_detail(void *data, int index)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	debug_secure("index (%d)", index);

	EmailViewerView *view = (EmailViewerView *)data;
	email_params_h params = NULL;
	int ret = -1;

	if (email_params_create(&params) &&
		email_params_set_operation(params, APP_CONTROL_OPERATION_VIEW) &&
		email_params_set_mime(params, EMAIL_CONTACT_MIME_SCHEME) &&
		email_params_add_int(params, EMAIL_CONTACT_EXT_DATA_ID, index)) {

		ret = email_module_launch_app(view->base.module, EMAIL_LAUNCH_APP_AUTO, params, NULL);
	}

	email_params_free(&params);

	if (ret != 0) {
		notification_status_message_post(_("IDS_EMAIL_HEADER_UNABLE_TO_PERFORM_ACTION_ABB"));
	}

	debug_leave();
}

void viewer_launch_composer(void *data, int type)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;
	retm_if(view->mail_info == NULL, "Invalid parameter: mail_info[NULL]");

	email_params_h params = NULL;

	if (email_params_create(&params) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_RUN_TYPE, type) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_ACCOUNT_ID, view->account_id) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_MAILBOX, view->mailbox_id) &&
		email_params_add_int(params, EMAIL_BUNDLE_KEY_MAIL_ID, view->mail_id) &&
		((type != RUN_EML_REPLY && type != RUN_EML_REPLY_ALL && type != RUN_EML_FORWARD) ||
		email_params_add_str(params, EMAIL_BUNDLE_KEY_MYFILE_PATH, view->eml_file_path))) {

		view->loaded_module = viewer_create_module(view, EMAIL_MODULE_COMPOSER, params);
	}

	email_params_free(&params);

	debug_leave();
}

void viewer_remove_folder(char *path)
{
	debug_enter();
	retm_if(path == NULL, "Invalid parameter: path[NULL]");

	debug_secure("filepath:(%s)", path);

	if (email_file_recursive_rm(path) == EINA_FALSE) {
		char err_buff[EMAIL_BUFF_SIZE_HUG] = { 0, };
		debug_error("email_file_recursive_rm failed! (%d): %s", errno,
				strerror_r(errno, err_buff, sizeof(err_buff)));
	}
	debug_leave();
}

/**
 * @return		allocated string to be freed after use.
 */
char *viewer_escape_filename(const char *filename)
{
	debug_enter();

	const char *p;
	char *q;
	char buf[PATH_MAX];

	if (!filename) return NULL;

	p = filename;
	q = buf;
	while (*p) {
		if ((q - buf) > (PATH_MAX - 6)) return NULL;
		if ((*p == ' ') || (*p == '\\') || (*p == '\'') ||
				(*p == '\"') || (*p == ';') || (*p == '!') ||
				(*p == '#') || (*p == '$') || (*p == '%') ||
				(*p == '&') || (*p == '*') || (*p == '(') ||
				(*p == ')') || (*p == '[') || (*p == ']') ||
				(*p == '{') || (*p == '}') || (*p == '|') ||
				(*p == '<') || (*p == '>') || (*p == '?') ||
				(*p == '\t') || (*p == '\n') || (*p == '/') ||
				(*p == ':'))
		{
			*q = '_';
		} else {
			*q = *p;
		}
		q++;
		p++;
	}
	*q = 0;

	debug_leave();
	return strdup(buf);
}

/**
 * @return		allocated string to be freed after use.
 */
char *viewer_get_datetime_string(void)
{
	debug_enter();

	tzset(); /* MUST BE. */

	time_t now_time = time(NULL);

	struct tm tm_buff = { 0, };
	struct tm *dummy = localtime_r(&now_time, &tm_buff);
	retvm_if(!dummy, NULL, "localtime_r() - failed");
	struct tm now_tm;
	memcpy(&now_tm, dummy, sizeof(struct tm));

	char date[9] = { 0, };
	snprintf(date, sizeof(date), "%04d%02d%02d",
			now_tm.tm_year + 1900, now_tm.tm_mon + 1, now_tm.tm_mday);

	char time[7] = { 0, };
	snprintf(time, sizeof(time), "%02d%02d%02d",
			now_tm.tm_hour, now_tm.tm_min, now_tm.tm_sec);

	debug_leave();
	return g_strconcat(date, "_", time, NULL);
}

/**
 * @return		allocated string to be freed after use.
 */
char *viewer_make_filepath(const char *dir, const char *name, const char *suffix)
{
	debug_enter();

	if (!dir || !name) {
		debug_log("invalid param: %p %p", dir, name);
		return NULL;
	}

	char path[PATH_MAX] = { 0, };

	char *fname = viewer_escape_filename(name);

	if (fname) {
		int dirlen = strlen(dir);
		bool add_slash = (dirlen > 0 && dir[dirlen - 1] != '/');

		int maxlen = NAME_MAX - (dirlen + (suffix ? strlen(suffix) : 0) + 3);
		if (strlen(fname) > maxlen) fname[maxlen] = '\0';

		snprintf(path, sizeof(path), "%s%s%s%s",
				dir, add_slash ? "/" : "", fname, suffix ? suffix : "");

		free(fname);
	}

	debug_leave();
	return strdup(path);
}

Eina_Bool viewer_check_body_download_status(int body_download_status, int status)
{
	debug_log("body_download_status(%d) & status(%d): %d", body_download_status, status, body_download_status & status);
	if (status == EMAIL_BODY_DOWNLOAD_STATUS_NONE && body_download_status == status)
		return true;
	return ((body_download_status & status) == status);
}

/* Temporary folder /tmp/email is created when viewer is launched. */
int viewer_create_temp_folder(EmailViewerView *view)
{
	debug_enter();
	retvm_if(view == NULL, -1, "Invalid parameter: view[NULL]");
	pid_t pid = getpid();

	/* for temp viewer files. */
	int nErr = email_create_folder(email_get_viewer_tmp_dir());
	if (nErr == -1) {
		debug_error("email_create_folder(EMAIL_VIEWER_TMP_FOLDER) failed!");
		return -1;
	}

	view->temp_folder_path = g_strdup_printf("%s/%d_%d_%d", email_get_viewer_tmp_dir(),
			pid, view->account_id, view->mail_id);
	nErr = email_create_folder(view->temp_folder_path);
	if (nErr == -1) {
		debug_error("email_create_folder(temp_folder_path) failed!");
		return -1;
	}
	debug_leave();
	return 0;
}

/* Temporary folder '/tmp/email' and its contents are deleted when composer is destroyed. */
void viewer_remove_temp_folders(EmailViewerView *view)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	viewer_remove_folder(view->temp_folder_path);
	FREE(view->temp_folder_path);
	viewer_remove_folder(view->temp_preview_folder_path);
	FREE(view->temp_preview_folder_path);

	debug_leave();
}

void viewer_remove_temp_files(EmailViewerView *view)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	if (email_file_exists(view->temp_viewer_html_file_path)) {
		if (!email_file_remove(view->temp_viewer_html_file_path)) {
			char err_buff[EMAIL_BUFF_SIZE_HUG] = { 0, };
			debug_error("email_file_remove(temp_viewer_html_file_path) failed! : %s", strerror_r(errno, err_buff, sizeof(err_buff)));
		}
		G_FREE(view->temp_viewer_html_file_path);
	}

	debug_leave();
}

void viewer_create_body_button(EmailViewerView *view, const char *button_title, Evas_Smart_Cb cb)
{
	debug_enter();

	retm_if(!cb, "Callback is NULL");
	retm_if(!button_title, "Button text is button");
	viewer_delete_body_button(view);

	view->dn_btn = elm_button_add(view->base.content);
	elm_object_style_set(view->dn_btn, "body_button");
	evas_object_size_hint_align_set(view->dn_btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(view->dn_btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_object_domain_translatable_text_set(view->dn_btn, PACKAGE, button_title);
	evas_object_show(view->dn_btn);

	if (view->dn_btn) {
		elm_box_pack_end(view->main_bx, view->dn_btn);
	}

	evas_object_smart_callback_add(view->dn_btn, "clicked", cb, (void *)view);
}

void viewer_delete_body_button(EmailViewerView *view)
{
	debug_enter();
	elm_box_unpack(view->main_bx, view->dn_btn);
	DELETE_EVAS_OBJECT(view->dn_btn);
}
/* EOF */
