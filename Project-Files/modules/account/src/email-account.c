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

#ifndef UG_MODULE_API
#define UG_MODULE_API __attribute__ ((visibility("default")))
#endif

#include <notification.h>

#include "email-account.h"
#include "email-account-folder.h"
#include "email-account-folder-move.h"
#include "email-account-util.h"
#include "email-account-list-view.h"

#define EMAIL_GENLIST_MAX_BLOCK_ITEMS_COUNT 36

static int _account_module_create(email_module_t *self, app_control_h params);

static int _account_create(email_view_t *self);
static void _account_destroy(email_view_t *self);
static void _account_activate(email_view_t *self, email_view_state prev_state);
static void _account_update(email_view_t *self, int flags);
static void _account_on_back_key(email_view_t *self);

int _create_fullview(EmailAccountUGD *ug_data);
static Evas_Object *_create_title_btn(Evas_Object *parent, char *image_name, Evas_Smart_Cb func, void *data);

static void _folder_create_btn_click_cb(void *data, Evas_Object *obj, void *event_info);

static void _dbus_receiver_setup(EmailAccountUGD *ug_data);
static void _remove_dbus_receiver(EmailAccountUGD *ug_data);

static void _init_genlist_item_class(EmailAccountUGD *ug_data);
static void _clear_all_genlist_item_class(EmailAccountUGD *ug_data);

static void _refresh_account_ug(EmailAccountUGD *ug_data);

static void _account_more_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static void _account_delete_more_ctxpopup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _account_more_ctxpopup_dismissed_cb(void *data, Evas_Object *obj, void *event_info);
static void _account_resize_more_ctxpopup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _account_settings_cb(void *data, Evas_Object *obj, void *event_info);
static email_module_h account_setting_module_create(void *data, email_module_type_e module_type, app_control_h service);
static void _account_setting_module_destroy(void *priv, email_module_h module);
static void _folder_delete_cb(void *data, Evas_Object *obj, void *event_info);
static void _folder_rename_cb(void *data, Evas_Object *obj, void *event_info);
static int _get_accounts_data(int *account_count, email_account_t **account_list);

static int _account_module_create(email_module_t *self, app_control_h params)
{
	debug_enter();
	EmailAccountModule *md = (EmailAccountModule *)self;
	EmailAccountUGD *ug_data = &md->view;
	char *argv[5] = { 0 };
	int account_count = 0;
	email_account_t *account_list = NULL;
	int ret = APP_CONTROL_ERROR_NONE;

	if (!self) {
		return -1;
	}

	if (!md->base.win) {
		return -1;
	}

	if (!email_engine_initialize()) {
		debug_warning("Failed to initialize email engine");
	}

	int w, h;
	elm_win_screen_size_get(md->base.win, NULL, NULL, &w, &h);
	debug_log("WINDOW W[%d] H[%d]", w, h);
	ug_data->main_w = w;
	ug_data->main_h = h;
	int is_move_mail_ug = 0;
	int is_account_list_ug = 0;
	int changed_ang = elm_win_rotation_get(md->base.win);
	if (changed_ang == 270 || changed_ang == 90) {
		ug_data->isRotate = true;
	} else {
		ug_data->isRotate = false;
	}

	/* parsing bundle data */
	app_control_get_extra_data(params, EMAIL_BUNDLE_KEY_ACCOUNT_ID, &(argv[0]));
	app_control_get_extra_data(params, EMAIL_BUNDLE_KEY_MAILBOX, &(argv[1]));
	app_control_get_extra_data(params, EMAIL_BUNDLE_KEY_IS_MAILBOX_MOVE_UG, &(argv[2]));
	app_control_get_extra_data(params, EMAIL_BUNDLE_KEY_IS_MAILBOX_ACCOUNT_UG, &(argv[3]));
	app_control_get_extra_data(params, EMAIL_BUNDLE_KEY_ACCOUNT_TYPE, &(argv[4]));

	if (argv[0]) {
		ug_data->account_id = atoi(argv[0]);
		g_free(argv[0]);
	}

	if (argv[1]) {
		ug_data->folder_id = atoi(argv[1]);
		g_free(argv[1]);
	}

	if (argv[2]) {
		is_move_mail_ug = atoi(argv[2]);
		g_free(argv[2]);

		if (is_move_mail_ug) {
			ug_data->folder_view_mode = ACC_FOLDER_MOVE_MAIL_VIEW_MODE;
		}
	}

	if (argv[3]) {
		is_account_list_ug = atoi(argv[3]);
		g_free(argv[3]);

		if (is_account_list_ug) {

			if (argv[4]) {

				debug_log("mailbox_mode:%s", argv[4]);

				if (_get_accounts_data(&account_count, &account_list) != 0) {
					debug_critical("get accounts data failed");
					return -1;
				}

				if (g_strcmp0(argv[4], EMAIL_BUNDLE_VAL_PRIORITY_SENDER) == 0) {
					ug_data->mailbox_mode = ACC_MAILBOX_TYPE_PRIORITY_INBOX;
					ug_data->mailbox_type = EMAIL_MAILBOX_TYPE_PRIORITY_SENDERS;
					if (account_count == 1) {
						ug_data->folder_view_mode = ACC_FOLDER_SINGLE_VIEW_MODE;
					} else {
						ug_data->folder_view_mode = ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE;
					}
				} else if (g_strcmp0(argv[4], EMAIL_BUNDLE_VAL_FILTER_INBOX) == 0) {
					ug_data->mailbox_mode = ACC_MAILBOX_TYPE_FLAGGED;
					ug_data->mailbox_type = EMAIL_MAILBOX_TYPE_FLAGGED;
					if (account_count == 1) {
						ug_data->folder_view_mode = ACC_FOLDER_SINGLE_VIEW_MODE;
					} else {
						ug_data->folder_view_mode = ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE;
					}
				} else if (g_strcmp0(argv[4], EMAIL_BUNDLE_VAL_ALL_ACCOUNT) == 0 || account_count > 1) {
					ug_data->mailbox_mode = ACC_MAILBOX_TYPE_ALL_ACCOUNT;
					ug_data->folder_view_mode = ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE;
				} else {
					ug_data->mailbox_mode = ACC_MAILBOX_TYPE_SINGLE_ACCOUNT;
					ug_data->folder_view_mode = ACC_FOLDER_SINGLE_VIEW_MODE;
				}
				g_free(argv[4]);
			} else {
				ug_data->mailbox_mode = ACC_MAILBOX_TYPE_ALL_ACCOUNT;
				ug_data->folder_view_mode = ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE;
			}
		}
	}
	debug_log("account_id [%d], folder_id [%d], is_move_mail_ug [%d], is_account_list_ug [%d]", ug_data->account_id, ug_data->folder_id, is_move_mail_ug, is_account_list_ug);
	debug_log("folder_view_mode [%d]", ug_data->folder_view_mode);

	/* contents */
	if (ug_data->folder_view_mode == ACC_FOLDER_MOVE_MAIL_VIEW_MODE) {
		char *move_argv[5] = { 0 };
		int selected_mail_list_legth = 0;
		char **selected_mail_list = NULL;

		app_control_get_extra_data(params, EMAIL_BUNDLE_KEY_MAILBOX_MOVE_MODE, &(move_argv[0]));
		if (move_argv[0]) {
			ug_data->move_mode = atoi(move_argv[0]);
			g_free(move_argv[0]);
		}
		app_control_get_extra_data(params, EMAIL_BUNDLE_KEY_IS_MAILBOX_EDIT_MODE, &(move_argv[2]));
		if (move_argv[2]) {
			ug_data->b_editmode = atoi(move_argv[2]);
			g_free(move_argv[2]);
		}
		app_control_get_extra_data(params, EMAIL_BUNDLE_KEY_MOVE_SRC_MAILBOX_ID, &(move_argv[1]));
		if (move_argv[1]) {
			ug_data->move_src_mailbox_id = atoi(move_argv[1]);
			g_free(move_argv[1]);
		}
		ret = app_control_get_extra_data_array(params, EMAIL_BUNDLE_KEY_ARRAY_SELECTED_MAIL_IDS, &selected_mail_list, &(selected_mail_list_legth));
		debug_warning_if(ret != APP_CONTROL_ERROR_NONE, "app_control_get_extra_data_array(item_id) failed! ret:[%d]", ret);
		int i;
		for (i = 0; i < selected_mail_list_legth; i++) {
			debug_log("mail list to move %d", atoi(selected_mail_list[i]));
			ug_data->selected_mail_list_to_move = g_list_prepend(ug_data->selected_mail_list_to_move,
											GINT_TO_POINTER(atoi(selected_mail_list[i])));
		}

		for (i = 0; i < selected_mail_list_legth; i++) {
			g_free(selected_mail_list[i]);
		}
		g_free(selected_mail_list);

	} else if (ug_data->folder_view_mode == ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE ||
			ug_data->folder_view_mode == ACC_FOLDER_SINGLE_VIEW_MODE) {

		char *mailbox_argv[3] = { 0 };

		if (ug_data->mailbox_type == 0) {
			app_control_get_extra_data(params, EMAIL_BUNDLE_KEY_MAILBOX_TYPE, &(mailbox_argv[0]));
			if (mailbox_argv[0]) {
				ug_data->mailbox_type = atoi(mailbox_argv[0]);
				g_free(mailbox_argv[0]);
			}
		}

		if (ug_data->mailbox_id == 0) {
			app_control_get_extra_data(params, EMAIL_BUNDLE_KEY_MAILBOX, &(mailbox_argv[1]));
			if (mailbox_argv[1]) {
				ug_data->mailbox_id = atoi(mailbox_argv[1]);
				g_free(mailbox_argv[1]);
			}
		}

		if (ug_data->folder_view_mode == ACC_FOLDER_SINGLE_VIEW_MODE) {
			ug_data->folder_mode = ACC_FOLDER_NONE;
			_dbus_receiver_setup(ug_data);
		} else {
			ug_data->folder_mode = ACC_FOLDER_INVALID;
		}
		debug_log("mailbox_type:%d, mailbox_id: %d, mailbox_mode:%d", ug_data->mailbox_type, ug_data->mailbox_id, ug_data->mailbox_mode);

	} else {
		/* DBUS */
		_dbus_receiver_setup(ug_data);

		ug_data->folder_mode = ACC_FOLDER_NONE;

		if (ug_data->account_id > 0) {
			ug_data->folder_view_mode = ACC_FOLDER_SINGLE_VIEW_MODE;
		} else if (ug_data->account_id == 0) {
			if (account_count == 1) {
				ug_data->folder_view_mode = ACC_FOLDER_SINGLE_VIEW_MODE;
				ug_data->account_id = account_list[0].account_id;
			} else {
				ug_data->folder_view_mode = ACC_FOLDER_COMBINED_VIEW_MODE;
			}
		}
	}

	if (account_list) {
		email_engine_free_account_list(&account_list, account_count);
	}

	md->view.base.create = _account_create;
	md->view.base.destroy = _account_destroy;
	md->view.base.activate = _account_activate;
	md->view.base.update = _account_update;
	md->view.base.on_back_key = _account_on_back_key;

	ret = email_module_create_view(&md->base, &md->view.base);
	if (ret != 0) {
		debug_error("email_module_create_view(): failed (%d)", ret);
		return -1;
	}

	debug_leave();
	return 0;
}

static int _account_create(email_view_t *self)
{
	debug_enter();

	EmailAccountUGD *vd = (EmailAccountUGD *)self;

	int ret = _create_fullview(vd);
	if (ret != 0) {
		debug_error("_create_fullview(): failed (%d)", ret);
		return -1;
	}

	debug_leave();
	return 0;
}

static void _account_activate(email_view_t *self, email_view_state prev_state)
{
	debug_enter();

	retm_if(!self, "Invalid parameter: self is NULL!");

	if (prev_state != EV_STATE_CREATED) {
		return;
	}

	EmailAccountUGD *ug_data = (EmailAccountUGD *)self;
	ug_data->folder_mode = ACC_FOLDER_NONE;

	debug_leave();
}

static void _account_destroy(email_view_t *self)
{
	debug_enter();
	EmailAccountUGD *ug_data;

	if (!self) {
		return;
	}

	ug_data = (EmailAccountUGD *)self;

	if (ug_data->emf_handle != EMAIL_HANDLE_INVALID) {
		account_stop_emf_job(ug_data, ug_data->emf_handle);
	}

	if (ug_data->folder_view_mode != ACC_FOLDER_MOVE_MAIL_VIEW_MODE && ug_data->folder_view_mode != ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE) {
		/* DBUS */
		_remove_dbus_receiver(ug_data);
	}

	account_color_list_free(ug_data);
	account_destroy_view(ug_data);
	_clear_all_genlist_item_class(ug_data);
	email_engine_finalize();

	FREE(ug_data->original_folder_name);
	FREE(ug_data->user_email_address);
	FREE(ug_data->moved_mailbox_name);

	if (ug_data->selected_mail_list_to_move) {
		g_list_free(ug_data->selected_mail_list_to_move);
		ug_data->selected_mail_list_to_move = NULL;
	}

	if (ug_data->popup) {
		evas_object_del(ug_data->popup);
		ug_data->popup = NULL;
	}
}

static void _account_update(email_view_t *self, int flags)
{
	debug_enter();
	EmailAccountUGD *ug_data = (EmailAccountUGD *)self;

	if (flags & EVUF_LANGUAGE_CHANGED) {
		_refresh_account_ug(ug_data);
	}

	if (flags & EVUF_ORIENTATION_CHANGED) {
		ug_data->isRotate =
				((ug_data->base.orientation == APP_DEVICE_ORIENTATION_90) ||
				 (ug_data->base.orientation == APP_DEVICE_ORIENTATION_270));
		if (ug_data->gl) {
			elm_genlist_realized_items_update(ug_data->gl);
		}
	}
}

EMAIL_API email_module_t *email_module_alloc()
{
	debug_enter();

	EmailAccountModule *ug_data = MEM_ALLOC(ug_data, 1);
	if (!ug_data) {
		return NULL;
	}

	ug_data->base.create = _account_module_create;

	debug_leave();
	return &ug_data->base;
}

static void _refresh_account_ug(EmailAccountUGD *ug_data)
{
	debug_enter();
	RETURN_IF_FAIL(ug_data != NULL);

	debug_log("refresh folder view, folder_mode : %d", ug_data->folder_mode);

	if (ug_data->gl)
		elm_genlist_realized_items_update(ug_data->gl);
}

int _create_fullview(EmailAccountUGD *ug_data)
{
	debug_enter();

	/* push naviframe */
	ug_data->base.content = elm_layout_add(ug_data->base.module->win);
	elm_layout_theme_set(ug_data->base.content, "layout", "application", "default");
	evas_object_size_hint_weight_set(ug_data->base.content, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	/*elm_win_resize_object_add(ug_data->win, ug_data->navi_content);*/

	debug_log("folder_view_mode %d", ug_data->folder_view_mode);

	if (ug_data->folder_view_mode == ACC_FOLDER_MOVE_MAIL_VIEW_MODE) {
		email_module_view_push(&ug_data->base, NULL, 0);
		elm_object_item_domain_translatable_text_set(ug_data->base.navi_item, PACKAGE, "IDS_EMAIL_HEADER_SELECT_FOLDER_ABB2");
	} else if (ug_data->folder_view_mode == ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE) {
		email_module_view_push(&ug_data->base, NULL, 0);
		elm_object_item_domain_translatable_text_set(ug_data->base.navi_item, PACKAGE, "IDS_EMAIL_HEADER_MAILBOX_ABB");
		ug_data->more_btn = _create_title_btn(ug_data->base.module->navi, NULL, _account_more_clicked_cb, ug_data);
		elm_object_item_part_content_set(ug_data->base.navi_item, "toolbar_more_btn", ug_data->more_btn);
	} else {
		email_module_view_push(&ug_data->base, NULL, 0);
		ug_data->more_btn = _create_title_btn(ug_data->base.module->navi, NULL, _account_more_clicked_cb, ug_data);
		elm_object_item_part_content_set(ug_data->base.navi_item, "toolbar_more_btn", ug_data->more_btn);

		if (ug_data->folder_view_mode == ACC_FOLDER_SINGLE_VIEW_MODE
				|| ug_data->folder_view_mode == ACC_FOLDER_COMBINED_SINGLE_VIEW_MODE) {

			char *temp = account_get_user_email_address(ug_data->account_id);
			if (temp) {
				FREE(ug_data->user_email_address);
				ug_data->user_email_address = temp;
			}

			if (ug_data->folder_view_mode == ACC_FOLDER_COMBINED_SINGLE_VIEW_MODE) {
				elm_object_item_text_set(ug_data->base.navi_item, ug_data->user_email_address);
			} else {
				elm_object_item_domain_translatable_text_set(ug_data->base.navi_item, PACKAGE, "IDS_EMAIL_HEADER_MAILBOX_ABB");
			}
		} else {
			elm_object_item_part_text_set(ug_data->base.navi_item, "elm.text.title", _("IDS_EMAIL_HEADER_COMBINED_ACCOUNTS_ABB"));
		}
	}

	/* contents */
	_init_genlist_item_class(ug_data);
	account_create_list(ug_data);

	elm_object_part_content_set(ug_data->base.content, "elm.swallow.content", ug_data->gl);

	debug_log("account_id: [%d]", ug_data->account_id);

	return 0;
}

static Evas_Object *_create_title_btn(Evas_Object *parent, char *image_name, Evas_Smart_Cb func, void *data)
{
	Evas_Object *btn = elm_button_add(parent);
	if (!btn) return NULL;
	elm_object_style_set(btn, "naviframe/more/default");
	evas_object_smart_callback_add(btn, "clicked", func, data);
	return btn;
}

static void _account_on_back_key(email_view_t *self)
{
	debug_enter();
	RETURN_IF_FAIL(self != NULL);

	EmailAccountUGD *ug_data = (EmailAccountUGD *) self;
	debug_log("module: %p, folder_mode: %d, folder_view_mode: %d", &ug_data->base, ug_data->folder_mode, ug_data->folder_view_mode);

	if (ug_data->popup) {
		evas_object_del(ug_data->popup);
		ug_data->popup = NULL;
	}

	ug_data->it = NULL;
	ug_data->target_mailbox_id = -1;
	ug_data->editmode = false;

	if (ug_data->folder_view_mode == ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE
		|| ug_data->folder_view_mode == ACC_FOLDER_MOVE_MAIL_VIEW_MODE) {

		debug_log("module: %p, folder_mode: %d, folder_view_mode: %d", &ug_data->base, ug_data->folder_mode, ug_data->folder_view_mode);
		email_module_make_destroy_request(ug_data->base.module);
	} else {
		switch (ug_data->folder_mode) {
		case ACC_FOLDER_NONE:
#if 0
			ug_data->folder_view_mode = ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE;
			ug_data->folder_mode = ACC_FOLDER_INVALID;
			ug_data->account_id = 0;
			account_show_all_folder(ug_data);
#endif
			email_module_make_destroy_request(ug_data->base.module);
			break;

		case ACC_FOLDER_CREATE:
			account_create_folder_create_view(ug_data);
			break;
		case ACC_FOLDER_DELETE:
			account_delete_folder_view(ug_data);
			break;
		case ACC_FOLDER_RENAME:
			account_rename_folder_view(ug_data);
			break;

		default:
			debug_log("Warning...");
			break;
		}
	}
}

static void _folder_create_btn_click_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	RETURN_IF_FAIL(data != NULL);

	EmailAccountUGD *ug_data = (EmailAccountUGD *)data;
	DELETE_EVAS_OBJECT(ug_data->more_ctxpopup);

	ug_data->editmode = true;

	account_create_folder_create_view(ug_data);

	ug_data->it = ug_data->root_item;
	account_folder_newfolder(ug_data, obj, event_info);
	debug_leave();
}

static void _dbus_receiver_setup(EmailAccountUGD *ug_data)
{
	debug_enter();
	if (ug_data == NULL) {
		debug_log("data is NULL");
		return;
	}

	GError *error = NULL;
	if (ug_data->dbus_conn == NULL) {
		ug_data->dbus_conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
		if (error) {
			debug_error("g_bus_get_sync() failed (%s)", error->message);
			g_error_free(error);
			return;
		}

		ug_data->signal_handler_storage = g_dbus_connection_signal_subscribe(ug_data->dbus_conn, NULL, "User.Email.StorageChange", "email", "/User/Email/StorageChange",
													NULL, G_DBUS_SIGNAL_FLAGS_NONE, account_gdbus_event_account_receive, (void *)ug_data, NULL);

		if (ug_data->signal_handler_storage == GDBUS_SIGNAL_SUBSCRIBE_FAILURE) {
			debug_log("Failed to g_dbus_connection_signal_subscribe()");
			return;
		}
		ug_data->signal_handler_network = g_dbus_connection_signal_subscribe(ug_data->dbus_conn, NULL, "User.Email.NetworkStatus", "email", "/User/Email/NetworkStatus",
													NULL, G_DBUS_SIGNAL_FLAGS_NONE, account_gdbus_event_account_receive, (void *)ug_data, NULL);
		if (ug_data->signal_handler_network == GDBUS_SIGNAL_SUBSCRIBE_FAILURE) {
			debug_critical("Failed to g_dbus_connection_signal_subscribe()");
			return;
		}
	}
}

static void _remove_dbus_receiver(EmailAccountUGD *ug_data)
{
	debug_enter();
	if (ug_data == NULL) {
		debug_log("data is NULL");
		return;
	}

	g_dbus_connection_signal_unsubscribe(ug_data->dbus_conn, ug_data->signal_handler_storage);
	g_dbus_connection_signal_unsubscribe(ug_data->dbus_conn, ug_data->signal_handler_network);
	g_object_unref(ug_data->dbus_conn);
	ug_data->dbus_conn = NULL;
	ug_data->signal_handler_storage = 0;
	ug_data->signal_handler_network = 0;
}

static void _init_genlist_item_class(EmailAccountUGD *ug_data)
{
	debug_enter();
	RETURN_IF_FAIL(ug_data != NULL);

	if (ug_data->folder_view_mode == ACC_FOLDER_MOVE_MAIL_VIEW_MODE) {
		account_init_genlist_item_class_for_mail_move(ug_data);
	} else {
		account_init_genlist_item_class_for_account_view_list(ug_data);
		account_init_genlist_item_class_for_combined_folder_list(ug_data);
		account_init_genlist_item_class_for_single_folder_list(ug_data);
	}

}

static void _clear_all_genlist_item_class(EmailAccountUGD *ug_data)
{
	debug_enter();
	RETURN_IF_FAIL(ug_data != NULL);

	if (ug_data->folder_view_mode == ACC_FOLDER_MOVE_MAIL_VIEW_MODE) {
		EMAIL_GENLIST_ITC_FREE(ug_data->itc_group_move);
		EMAIL_GENLIST_ITC_FREE(ug_data->itc_move);
	} else {
		EMAIL_GENLIST_ITC_FREE(ug_data->itc_account_group);
		EMAIL_GENLIST_ITC_FREE(ug_data->itc_account_item);
		EMAIL_GENLIST_ITC_FREE(ug_data->itc_combined);
		EMAIL_GENLIST_ITC_FREE(ug_data->itc_group);
		EMAIL_GENLIST_ITC_FREE(ug_data->itc_root);
		EMAIL_GENLIST_ITC_FREE(ug_data->itc_single);
		EMAIL_GENLIST_ITC_FREE(ug_data->itc_account_name);
	}
}

int account_create_list(EmailAccountUGD *ug_data)
{
	debug_enter();
	Evas_Object *gl = NULL;
	int inserted_cnt = 0;

	account_destroy_view(ug_data);

	/* If one account only. Set as the account. */

	int err = _get_accounts_data(&ug_data->account_count, &ug_data->account_list);
	if (err != 0) {
		debug_log("get accounts data failed");
		return 0;
	}

	int i = 0;
	if (ug_data->account_list && (ug_data->account_count > 0)) {
		for (; i < ug_data->account_count; i++) {
			account_color_list_add(ug_data, ug_data->account_list[i].account_id, ug_data->account_list[i].color_label);
		}
	}

	debug_log("current account_id: %d", ug_data->account_id);

	gl = elm_genlist_add(ug_data->base.module->navi);
	elm_scroller_policy_set(gl, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
	elm_genlist_mode_set(gl, ELM_LIST_COMPRESS);
	elm_genlist_block_count_set(gl, EMAIL_GENLIST_MAX_BLOCK_ITEMS_COUNT);
	ug_data->gl = gl;

	if (ug_data->folder_view_mode == ACC_FOLDER_COMBINED_VIEW_MODE) {
		inserted_cnt = account_create_combined_folder_list(ug_data);
	} else if (ug_data->folder_view_mode == ACC_FOLDER_SINGLE_VIEW_MODE
			|| ug_data->folder_view_mode == ACC_FOLDER_COMBINED_SINGLE_VIEW_MODE) {

		inserted_cnt = account_create_single_account_folder_list(ug_data);

		if (inserted_cnt == -1) {
			err = notification_status_message_post(email_get_email_string("IDS_EMAIL_TPOP_SERVER_ERROR_OCCURRED_ABB"));
			if (err != NOTIFICATION_ERROR_NONE) {
				debug_log("fail to notification_status_message_post() : %d\n", err);
			}
			_account_on_back_key(&ug_data->base);
		}

	} else if (ug_data->folder_view_mode == ACC_FOLDER_MOVE_MAIL_VIEW_MODE) {
		inserted_cnt = account_create_folder_list_for_mail_move(ug_data);
	} else if (ug_data->folder_view_mode == ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE) {
		inserted_cnt = account_create_account_list_view(ug_data);
	}

	return inserted_cnt;
}

Evas_Object *account_add_empty_list(EmailAccountUGD *ug_data)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(ug_data != NULL, NULL);

	Evas_Object *list = elm_list_add(ug_data->base.module->win);
	elm_list_mode_set(list, ELM_LIST_COMPRESS);
	Elm_Object_Item *elm_item;

	elm_item = elm_list_item_append(list, "IDS_EMAIL_BODY_NO_ITEMS_TO_DISPLAY", NULL, NULL, NULL, ug_data);
	elm_object_item_domain_text_translatable_set(elm_item, PACKAGE, EINA_TRUE);
	elm_list_select_mode_set(list, ELM_OBJECT_SELECT_MODE_NONE);
	elm_list_go(list);

	return list;
}

static void _folder_delete_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	RETURN_IF_FAIL(data != NULL);

	EmailAccountUGD *ug_data = (EmailAccountUGD *)data;
	DELETE_EVAS_OBJECT(ug_data->more_ctxpopup);

	ug_data->editmode = true;

	account_delete_folder_view(ug_data);

	debug_leave();
}

static void _folder_rename_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	RETURN_IF_FAIL(data != NULL);

	EmailAccountUGD *ug_data = (EmailAccountUGD *)data;
	DELETE_EVAS_OBJECT(ug_data->more_ctxpopup);

	ug_data->editmode = true;

	account_rename_folder_view(ug_data);

	debug_leave();
}

static void _account_more_ctxpopup_dismissed_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailAccountUGD *ug_data = (EmailAccountUGD *) data;
	DELETE_EVAS_OBJECT(ug_data->more_ctxpopup);
}

static void _account_delete_more_ctxpopup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailAccountUGD *ug_data = (EmailAccountUGD *) data;
	evas_object_event_callback_del(ug_data->base.module->navi, EVAS_CALLBACK_RESIZE, _account_resize_more_ctxpopup_cb);
	eext_object_event_callback_del(ug_data->more_ctxpopup, EEXT_CALLBACK_BACK, _account_more_ctxpopup_dismissed_cb);
	eext_object_event_callback_del(ug_data->more_ctxpopup, EEXT_CALLBACK_MORE, _account_more_ctxpopup_dismissed_cb);
	evas_object_smart_callback_del(ug_data->more_ctxpopup, "dismissed", _account_more_ctxpopup_dismissed_cb);
	evas_object_event_callback_del(ug_data->more_ctxpopup, EVAS_CALLBACK_DEL, _account_delete_more_ctxpopup_cb);

}

static void _account_resize_more_ctxpopup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailAccountUGD *ug_data = (EmailAccountUGD *) data;

	Evas_Coord w, h;
	int pos = -1;

	elm_win_screen_size_get(ug_data->base.module->win, NULL, NULL, &w, &h);
	pos = elm_win_rotation_get(ug_data->base.module->win);
	debug_log("pos: [%d], w[%d], h[%d]", pos, w, h);

	if (ug_data->more_ctxpopup) {
		switch (pos) {
			case 0:
			case 180:
				evas_object_move(ug_data->more_ctxpopup, (w / 2), h);
				break;
			case 90:
				evas_object_move(ug_data->more_ctxpopup, (h / 2), w);
				break;
			case 270:
				evas_object_move(ug_data->more_ctxpopup, (h / 2), w);
				break;
			}
	}
}

static void _account_setting_module_destroy(void *priv, email_module_h module)
{
	debug_enter();
	retm_if(!module, "module is NULL");
	retm_if(!priv, "priv is NULL");

	EmailAccountUGD *ug_data = (EmailAccountUGD *)priv;

	if (module == ug_data->setting) {
		email_module_destroy(ug_data->setting);
		ug_data->setting = NULL;
	}
}

email_module_h account_setting_module_create(void *data, email_module_type_e module_type, app_control_h service)
{
	debug_enter();

	EmailAccountUGD *ug_data = (EmailAccountUGD *) data;

	email_module_listener_t listener = { 0 };
	listener.cb_data = ug_data;
	listener.destroy_request_cb = _account_setting_module_destroy;

	return email_module_create_child(ug_data->base.module, module_type, service, &listener);
}

static void _account_settings_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailAccountUGD *ug_data = (EmailAccountUGD *) data;
	app_control_h service;

	DELETE_EVAS_OBJECT(ug_data->more_ctxpopup);

	if (APP_CONTROL_ERROR_NONE != app_control_create(&service)) {
		debug_log("creating service handle failed");
		return;
	}

	app_control_add_extra_data(service, EMAIL_BUNDLE_KEY_VIEW_TYPE, EMAIL_BUNDLE_VAL_VIEW_SETTING);
	ug_data->setting = account_setting_module_create(ug_data, EMAIL_MODULE_SETTING, service);
	app_control_destroy(service);
}

static Elm_Object_Item *_add_ctx_menu_item(EmailAccountUGD *ug_data, const char *str, Evas_Object *icon, Evas_Smart_Cb cb)
{
	Elm_Object_Item *ctx_menu_item = NULL;

	ctx_menu_item = elm_ctxpopup_item_append(ug_data->more_ctxpopup, str, icon, cb, ug_data);
	elm_object_item_domain_text_translatable_set(ctx_menu_item, PACKAGE, EINA_TRUE);
	return ctx_menu_item;
}

static void _account_more_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailAccountUGD *ug_data = (EmailAccountUGD *) data;
	DELETE_EVAS_OBJECT(ug_data->more_ctxpopup);
	if (ug_data->popup) {
		debug_log("Popup already open!");
		return;
	}
	if (ug_data->editmode == false) {
		ug_data->more_ctxpopup = elm_ctxpopup_add(ug_data->base.module->win);
		elm_ctxpopup_auto_hide_disabled_set(ug_data->more_ctxpopup, EINA_TRUE);
		elm_object_style_set(ug_data->more_ctxpopup, "more/default");

		eext_object_event_callback_add(ug_data->more_ctxpopup, EEXT_CALLBACK_BACK, _account_more_ctxpopup_dismissed_cb, ug_data);
		eext_object_event_callback_add(ug_data->more_ctxpopup, EEXT_CALLBACK_MORE, _account_more_ctxpopup_dismissed_cb, ug_data);

		evas_object_smart_callback_add(ug_data->more_ctxpopup, "dismissed", _account_more_ctxpopup_dismissed_cb, ug_data);
		evas_object_event_callback_add(ug_data->more_ctxpopup, EVAS_CALLBACK_DEL, _account_delete_more_ctxpopup_cb, ug_data);
		evas_object_event_callback_add(ug_data->base.module->navi, EVAS_CALLBACK_RESIZE, _account_resize_more_ctxpopup_cb, ug_data);

		if (ug_data->folder_view_mode == ACC_FOLDER_SINGLE_VIEW_MODE
				|| ug_data->folder_view_mode == ACC_FOLDER_COMBINED_SINGLE_VIEW_MODE) {
			_add_ctx_menu_item(ug_data, "IDS_EMAIL_OPT_CREATE_FOLDER_ABB2", NULL, _folder_create_btn_click_cb);
			_add_ctx_menu_item(ug_data, "IDS_EMAIL_OPT_DELETE_FOLDER_ABB", NULL, _folder_delete_cb);
			_add_ctx_menu_item(ug_data, "IDS_EMAIL_OPT_RENAME_FOLDER", NULL, _folder_rename_cb);
		}
		_add_ctx_menu_item(ug_data, "IDS_EMAIL_OPT_SETTINGS", NULL, _account_settings_cb);

		_account_resize_more_ctxpopup_cb(ug_data, NULL, NULL, NULL);
		evas_object_show(ug_data->more_ctxpopup);
	}
}

void account_destroy_view(EmailAccountUGD *ug_data)
{
	debug_enter();

	RETURN_IF_FAIL(ug_data != NULL);

	int i = 0;
	int err = 0;

	if (ug_data->popup) {
		evas_object_del(ug_data->popup);
		ug_data->popup = NULL;
	}

	DELETE_EVAS_OBJECT(ug_data->more_ctxpopup);

	if (ug_data->gl) {
		elm_genlist_clear(ug_data->gl);
		evas_object_del(ug_data->gl);
		ug_data->gl = NULL;
	}

	if (ug_data->folder_view_mode == ACC_FOLDER_MOVE_MAIL_VIEW_MODE) {
		if (ug_data->account_list) {
			email_move_list *move_list = ug_data->move_list;
			for (i = 0; i < ug_data->account_count; i++) {
				if (move_list[i].mailbox_list) {
					err = email_free_mailbox(&(move_list[i].mailbox_list), move_list[i].mailbox_cnt);
					if (err != EMAIL_ERROR_NONE) {
						debug_critical("fail to free mailbox list - err(%d)", err);
					}
					move_list[i].mailbox_list = NULL;
				}
			}
			FREE(move_list);
		}
	} else if (ug_data->folder_view_mode == ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE) {
		G_LIST_FREE(ug_data->account_group_list);
	}

	if (ug_data->account_list) {
		debug_log("count of account: %d", ug_data->account_count);
		err = email_engine_free_account_list(&(ug_data->account_list), ug_data->account_count);
		if (err == 0) {
			debug_critical("fail to free account - err(%d)", err);
		}
		ug_data->account_list = NULL;
		ug_data->account_count = 0;
	}
}

void account_show_all_folder(EmailAccountUGD *ug_data)
{
	debug_enter();

	if (ug_data->folder_view_mode == ACC_FOLDER_COMBINED_SINGLE_VIEW_MODE) {

		char *temp = account_get_user_email_address(ug_data->account_id);
		if (temp) {
			FREE(ug_data->user_email_address);
			ug_data->user_email_address = temp;
		}
		elm_object_item_text_set(ug_data->base.navi_item, ug_data->user_email_address);

		/* DBUS */
		_dbus_receiver_setup(ug_data);

		ug_data->more_btn = _create_title_btn(ug_data->base.module->navi, NULL, _account_more_clicked_cb, ug_data);
		elm_object_item_part_content_set(ug_data->base.navi_item, "toolbar_more_btn", ug_data->more_btn);
	} else if (ug_data->folder_view_mode == ACC_FOLDER_COMBINED_VIEW_MODE) {
		elm_object_item_part_text_set(ug_data->base.navi_item, "elm.text.title", _("IDS_EMAIL_HEADER_COMBINED_ACCOUNTS_ABB"));
	} else if (ug_data->folder_view_mode == ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE) {
		elm_object_item_part_text_set(ug_data->base.navi_item, "elm.text.title", _("IDS_EMAIL_HEADER_MAILBOX_ABB"));
	} else {
		elm_object_item_part_text_set(ug_data->base.navi_item, "elm.text.title", _("IDS_ST_OPT_NONE"));
	}

	account_create_list(ug_data);
	evas_object_show(ug_data->gl);
	elm_object_part_content_set(ug_data->base.content, "elm.swallow.content", ug_data->gl);
}

static int _get_accounts_data(int *account_count, email_account_t **account_list)
{
	debug_enter();

	int err = email_engine_get_account_list(account_count, account_list);
	if (err == FALSE) {
		debug_critical("fail to get account list - err(%d)", err);
		if (*account_list) {
			email_engine_free_account_list(account_list, *account_count);
		}
		return -1;
	}

	if (*account_list == NULL || *account_count == 0) {
		debug_critical("account info is NULL");
		return -1;
	}

	return 0;
}

Evas_Object *account_create_account_color_bar(Evas_Object *parent, unsigned int color)
{
	Evas_Object *color_bar = evas_object_rectangle_add(evas_object_evas_get(parent));
	int r = R_MASKING(color);
	int g = G_MASKING(color);
	int b = B_MASKING(color);
	int a = A_MASKING(color);

	evas_object_size_hint_fill_set(color_bar, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_color_set(color_bar, r, g, b, a);
	return color_bar;
}

/* EOF */
