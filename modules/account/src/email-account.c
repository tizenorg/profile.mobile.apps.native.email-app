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

#include "email-account.h"
#include "email-account-folder.h"
#include "email-account-folder-move.h"
#include "email-account-util.h"
#include "email-account-list-view.h"

#define EMAIL_GENLIST_MAX_BLOCK_ITEMS_COUNT 36

static int _account_module_create(email_module_t *self, email_params_h params);

static int _account_create(email_view_t *self);
static void _account_destroy(email_view_t *self);
static void _account_activate(email_view_t *self, email_view_state prev_state);
static void _account_update(email_view_t *self, int flags);
static void _account_on_back_key(email_view_t *self);

int _create_fullview(EmailAccountView *view);
static Evas_Object *_create_title_btn(Evas_Object *parent, char *image_name, Evas_Smart_Cb func, void *data);

static void _folder_create_btn_click_cb(void *data, Evas_Object *obj, void *event_info);

static void _dbus_receiver_setup(EmailAccountView *view);
static void _remove_dbus_receiver(EmailAccountView *view);

static void _init_genlist_item_class(EmailAccountView *view);
static void _clear_all_genlist_item_class(EmailAccountView *view);

static void _refresh_account_list(EmailAccountView *view);

static void _account_more_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static void _account_delete_more_ctxpopup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _account_more_ctxpopup_dismissed_cb(void *data, Evas_Object *obj, void *event_info);
static void _account_resize_more_ctxpopup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _account_settings_cb(void *data, Evas_Object *obj, void *event_info);
static email_module_h account_setting_module_create(void *data, email_module_type_e module_type, email_params_h params);
static void _account_setting_module_destroy(void *priv, email_module_h module);
static void _folder_delete_cb(void *data, Evas_Object *obj, void *event_info);
static void _folder_rename_cb(void *data, Evas_Object *obj, void *event_info);
static int _get_accounts_data(int *account_count, email_account_t **account_list);

static int _account_module_create(email_module_t *self, email_params_h params)
{
	debug_enter();
	EmailAccountModule *module = (EmailAccountModule *)self;
	EmailAccountView *view = &module->view;
	email_account_t *account_list = NULL;
	int ret = APP_CONTROL_ERROR_NONE;

	if (!self) {
		return -1;
	}

	if (!module->base.win) {
		return -1;
	}

	if (!email_engine_initialize()) {
		debug_warning("Failed to initialize email engine");
	}

	int w, h;
	elm_win_screen_size_get(module->base.win, NULL, NULL, &w, &h);
	debug_log("WINDOW W[%d] H[%d]", w, h);
	view->main_w = w;
	view->main_h = h;
	int is_move_mail_mode = 0;
	int is_account_list_mode = 0;
	int changed_ang = elm_win_rotation_get(module->base.win);
	if (changed_ang == 270 || changed_ang == 90) {
		view->isRotate = true;
	} else {
		view->isRotate = false;
	}

	/* parsing params data */
	email_params_get_int_opt(params, EMAIL_BUNDLE_KEY_ACCOUNT_ID, &view->account_id);
	email_params_get_int_opt(params, EMAIL_BUNDLE_KEY_MAILBOX, &view->folder_id);
	email_params_get_int_opt(params, EMAIL_BUNDLE_KEY_IS_MAILBOX_MOVE_MODE, &is_move_mail_mode);

	if (is_move_mail_mode) {
		view->folder_view_mode = ACC_FOLDER_MOVE_MAIL_VIEW_MODE;
	}

	if (_get_accounts_data(&view->account_count, &view->account_list)) {
		debug_critical("get accounts data failed");
		return -1;
	}

	email_params_get_int_opt(params, EMAIL_BUNDLE_KEY_IS_MAILBOX_ACCOUNT_MODE, &is_account_list_mode);
	if (is_account_list_mode) {

		const char *account_type = NULL;
		email_params_get_str_opt(params, EMAIL_BUNDLE_KEY_ACCOUNT_TYPE, &account_type);
		if (account_type) {

			debug_log("mailbox_mode:%s", account_type);
			if (g_strcmp0(account_type, EMAIL_BUNDLE_VAL_PRIORITY_SENDER) == 0) {
				view->mailbox_mode = ACC_MAILBOX_TYPE_PRIORITY_INBOX;
				view->mailbox_type = EMAIL_MAILBOX_TYPE_PRIORITY_SENDERS;
				if (view->account_count == 1) {
					view->folder_view_mode = ACC_FOLDER_SINGLE_VIEW_MODE;
				} else {
					view->folder_view_mode = ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE;
				}
			} else if (g_strcmp0(account_type, EMAIL_BUNDLE_VAL_FILTER_INBOX) == 0) {
				view->mailbox_mode = ACC_MAILBOX_TYPE_FLAGGED;
				view->mailbox_type = EMAIL_MAILBOX_TYPE_FLAGGED;
				if (view->account_count == 1) {
					view->folder_view_mode = ACC_FOLDER_SINGLE_VIEW_MODE;
				} else {
					view->folder_view_mode = ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE;
				}
			} else if (g_strcmp0(account_type, EMAIL_BUNDLE_VAL_ALL_ACCOUNT) == 0 || view->account_count > 1) {
				view->mailbox_mode = ACC_MAILBOX_TYPE_ALL_ACCOUNT;
				view->folder_view_mode = ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE;
			} else {
				view->mailbox_mode = ACC_MAILBOX_TYPE_SINGLE_ACCOUNT;
				view->folder_view_mode = ACC_FOLDER_SINGLE_VIEW_MODE;
			}
		} else {
			view->mailbox_mode = ACC_MAILBOX_TYPE_ALL_ACCOUNT;
			view->folder_view_mode = ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE;
		}
	}

	debug_log("account_id [%d], folder_id [%d], is_move_mail_mode [%d], is_account_list_mode [%d]", view->account_id, view->folder_id, is_move_mail_mode, is_account_list_mode);
	debug_log("folder_view_mode [%d]", view->folder_view_mode);

	/* contents */
	if (view->folder_view_mode == ACC_FOLDER_MOVE_MAIL_VIEW_MODE) {
		const char **selected_mail_list = NULL;
		int selected_mail_list_len = 0;

		email_params_get_int_opt(params, EMAIL_BUNDLE_KEY_MAILBOX_MOVE_MODE, &view->move_mode);
		email_params_get_int_opt(params, EMAIL_BUNDLE_KEY_IS_MAILBOX_EDIT_MODE, &view->b_editmode);
		email_params_get_int_opt(params, EMAIL_BUNDLE_KEY_MOVE_SRC_MAILBOX_ID, &view->move_src_mailbox_id);

		email_params_get_str_array(params, EMAIL_BUNDLE_KEY_ARRAY_SELECTED_MAIL_IDS, &selected_mail_list, &selected_mail_list_len);

		int i;
		for (i = 0; i < selected_mail_list_len; i++) {
			debug_log("mail list to move %d", atoi(selected_mail_list[i]));
			view->selected_mail_list_to_move = g_list_prepend(view->selected_mail_list_to_move,
											GINT_TO_POINTER(atoi(selected_mail_list[i])));
		}

	} else if (view->folder_view_mode == ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE ||
			view->folder_view_mode == ACC_FOLDER_SINGLE_VIEW_MODE) {

		if (view->mailbox_type == 0) {
			email_params_get_int_opt(params, EMAIL_BUNDLE_KEY_MAILBOX_TYPE, &view->mailbox_type);
		}

		if (view->mailbox_id == 0) {
			email_params_get_int_opt(params, EMAIL_BUNDLE_KEY_MAILBOX, &view->mailbox_id);
		}

		if (view->folder_view_mode == ACC_FOLDER_SINGLE_VIEW_MODE) {
			view->folder_mode = ACC_FOLDER_NONE;
			_dbus_receiver_setup(view);
		} else {
			view->folder_mode = ACC_FOLDER_INVALID;
		}
		debug_log("mailbox_type:%d, mailbox_id: %d, mailbox_mode:%d", view->mailbox_type, view->mailbox_id, view->mailbox_mode);

	} else {
		/* DBUS */
		_dbus_receiver_setup(view);

		view->folder_mode = ACC_FOLDER_NONE;

		if (view->account_id > 0) {
			view->folder_view_mode = ACC_FOLDER_SINGLE_VIEW_MODE;
		} else if (view->account_id == 0) {
			if (view->account_count == 1) {
				view->folder_view_mode = ACC_FOLDER_SINGLE_VIEW_MODE;
				view->account_id = account_list[0].account_id;
			} else {
				view->folder_view_mode = ACC_FOLDER_COMBINED_VIEW_MODE;
			}
		}
	}

	module->view.base.create = _account_create;
	module->view.base.destroy = _account_destroy;
	module->view.base.activate = _account_activate;
	module->view.base.update = _account_update;
	module->view.base.on_back_key = _account_on_back_key;

	ret = email_module_create_view(&module->base, &module->view.base);
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

	EmailAccountView *view = (EmailAccountView *)self;

	int ret = _create_fullview(view);
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

	EmailAccountView *view = (EmailAccountView *)self;
	view->folder_mode = ACC_FOLDER_NONE;

	debug_leave();
}

static void _account_destroy(email_view_t *self)
{
	debug_enter();
	if (!self) {
		return;
	}

	EmailAccountView *view = (EmailAccountView *)self;

	if (view->emf_handle != EMAIL_HANDLE_INVALID) {
		account_stop_emf_job(view, view->emf_handle);
	}

	if (view->folder_view_mode != ACC_FOLDER_MOVE_MAIL_VIEW_MODE && view->folder_view_mode != ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE) {
		/* DBUS */
		_remove_dbus_receiver(view);
	}

	account_color_list_free(view);
	account_destroy_view(view);
	_clear_all_genlist_item_class(view);
	email_engine_finalize();

	FREE(view->original_folder_name);
	FREE(view->user_email_address);
	FREE(view->moved_mailbox_name);

	if (view->selected_mail_list_to_move) {
		g_list_free(view->selected_mail_list_to_move);
		view->selected_mail_list_to_move = NULL;
	}

	if (view->popup) {
		evas_object_del(view->popup);
		view->popup = NULL;
	}

	if (view->account_list) {
		debug_log("count of account: %d", view->account_count);
		int err = email_engine_free_account_list(&(view->account_list), view->account_count);
		if (err == 0) {
			debug_critical("fail to free account - err(%d)", err);
		}
		view->account_list = NULL;
		view->account_count = 0;
	}
}

static void _account_update(email_view_t *self, int flags)
{
	debug_enter();
	EmailAccountView *view = (EmailAccountView *)self;

	if (flags & EVUF_LANGUAGE_CHANGED) {
		_refresh_account_list(view);
	}

	if (flags & EVUF_ORIENTATION_CHANGED) {
		view->isRotate =
				((view->base.orientation == APP_DEVICE_ORIENTATION_90) ||
				 (view->base.orientation == APP_DEVICE_ORIENTATION_270));
		if (view->gl) {
			elm_genlist_realized_items_update(view->gl);
		}
	}
}

#ifdef SHARED_MODULES_FEATURE
EMAIL_API email_module_t *email_module_alloc()
#else
email_module_t *account_module_alloc()
#endif
{
	debug_enter();

	EmailAccountModule *view = MEM_ALLOC(view, 1);
	if (!view) {
		return NULL;
	}

	view->base.create = _account_module_create;

	debug_leave();
	return &view->base;
}

static void _refresh_account_list(EmailAccountView *view)
{
	debug_enter();
	RETURN_IF_FAIL(view != NULL);

	debug_log("refresh folder view, folder_mode : %d", view->folder_mode);

	if (view->gl)
		elm_genlist_realized_items_update(view->gl);
}

int _create_fullview(EmailAccountView *view)
{
	debug_enter();

	/* push naviframe */
	view->base.content = elm_layout_add(view->base.module->win);
	elm_layout_theme_set(view->base.content, "layout", "application", "default");
	evas_object_size_hint_weight_set(view->base.content, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	email_module_view_push(&view->base, NULL, 0);

	debug_log("folder_view_mode %d", view->folder_view_mode);
	if (view->folder_view_mode == ACC_FOLDER_MOVE_MAIL_VIEW_MODE) {
		elm_object_item_domain_translatable_text_set(view->base.navi_item, PACKAGE, "IDS_EMAIL_HEADER_SELECT_FOLDER_ABB2");
	} else if (view->folder_view_mode == ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE) {
		elm_object_item_domain_translatable_text_set(view->base.navi_item, PACKAGE, "IDS_EMAIL_HEADER_MAILBOX_ABB");
		view->more_btn = _create_title_btn(view->base.module->navi, NULL, _account_more_clicked_cb, view);
		elm_object_item_part_content_set(view->base.navi_item, "toolbar_more_btn", view->more_btn);
	} else {
		view->more_btn = _create_title_btn(view->base.module->navi, NULL, _account_more_clicked_cb, view);
		elm_object_item_part_content_set(view->base.navi_item, "toolbar_more_btn", view->more_btn);

		if (view->folder_view_mode == ACC_FOLDER_SINGLE_VIEW_MODE
				|| view->folder_view_mode == ACC_FOLDER_COMBINED_SINGLE_VIEW_MODE) {

			char *temp = account_get_user_email_address(view->account_id);
			if (temp) {
				FREE(view->user_email_address);
				view->user_email_address = temp;
			}

			if (view->folder_view_mode == ACC_FOLDER_COMBINED_SINGLE_VIEW_MODE) {
				elm_object_item_text_set(view->base.navi_item, view->user_email_address);
			} else {
				elm_object_item_domain_translatable_text_set(view->base.navi_item, PACKAGE, "IDS_EMAIL_HEADER_MAILBOX_ABB");
			}
		} else {
			elm_object_item_part_text_set(view->base.navi_item, "elm.text.title", _("IDS_EMAIL_HEADER_COMBINED_ACCOUNTS_ABB"));
		}
	}

	/* contents */
	_init_genlist_item_class(view);
	account_create_list(view);

	elm_object_part_content_set(view->base.content, "elm.swallow.content", view->gl);

	debug_log("account_id: [%d]", view->account_id);

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

	EmailAccountView *view = (EmailAccountView *) self;
	debug_log("module: %p, folder_mode: %d, folder_view_mode: %d", &view->base, view->folder_mode, view->folder_view_mode);

	if (view->popup) {
		evas_object_del(view->popup);
		view->popup = NULL;
	}

	view->it = NULL;
	view->target_mailbox_id = -1;
	view->editmode = false;

	if (view->folder_view_mode == ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE
		|| view->folder_view_mode == ACC_FOLDER_MOVE_MAIL_VIEW_MODE) {

		debug_log("module: %p, folder_mode: %d, folder_view_mode: %d", &view->base, view->folder_mode, view->folder_view_mode);
		email_module_make_destroy_request(view->base.module);
	} else {
		switch (view->folder_mode) {
		case ACC_FOLDER_NONE:
#if 0
			view->folder_view_mode = ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE;
			view->folder_mode = ACC_FOLDER_INVALID;
			view->account_id = 0;
			account_show_all_folder(view);
#endif
			email_module_make_destroy_request(view->base.module);
			break;

		case ACC_FOLDER_CREATE:
			account_create_folder_create_view(view);
			break;
		case ACC_FOLDER_DELETE:
			account_delete_folder_view(view);
			break;
		case ACC_FOLDER_RENAME:
			account_rename_folder_view(view);
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

	EmailAccountView *view = (EmailAccountView *)data;
	DELETE_EVAS_OBJECT(view->more_ctxpopup);

	view->editmode = true;
	account_create_folder_create_view(view);
	account_folder_newfolder(view, obj, event_info);
	debug_leave();
}

static void _dbus_receiver_setup(EmailAccountView *view)
{
	debug_enter();
	if (view == NULL) {
		debug_log("data is NULL");
		return;
	}

	GError *error = NULL;
	if (view->dbus_conn == NULL) {
		view->dbus_conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
		if (error) {
			debug_error("g_bus_get_sync() failed (%s)", error->message);
			g_error_free(error);
			return;
		}

		view->signal_handler_storage = g_dbus_connection_signal_subscribe(view->dbus_conn, NULL, "User.Email.StorageChange", "email", "/User/Email/StorageChange",
													NULL, G_DBUS_SIGNAL_FLAGS_NONE, account_gdbus_event_account_receive, (void *)view, NULL);

		if (view->signal_handler_storage == GDBUS_SIGNAL_SUBSCRIBE_FAILURE) {
			debug_log("Failed to g_dbus_connection_signal_subscribe()");
			return;
		}
		view->signal_handler_network = g_dbus_connection_signal_subscribe(view->dbus_conn, NULL, "User.Email.NetworkStatus", "email", "/User/Email/NetworkStatus",
													NULL, G_DBUS_SIGNAL_FLAGS_NONE, account_gdbus_event_account_receive, (void *)view, NULL);
		if (view->signal_handler_network == GDBUS_SIGNAL_SUBSCRIBE_FAILURE) {
			debug_critical("Failed to g_dbus_connection_signal_subscribe()");
			return;
		}
	}
}

static void _remove_dbus_receiver(EmailAccountView *view)
{
	debug_enter();
	if (view == NULL) {
		debug_log("data is NULL");
		return;
	}

	g_dbus_connection_signal_unsubscribe(view->dbus_conn, view->signal_handler_storage);
	g_dbus_connection_signal_unsubscribe(view->dbus_conn, view->signal_handler_network);
	g_object_unref(view->dbus_conn);
	view->dbus_conn = NULL;
	view->signal_handler_storage = 0;
	view->signal_handler_network = 0;
}

static void _init_genlist_item_class(EmailAccountView *view)
{
	debug_enter();
	RETURN_IF_FAIL(view != NULL);

	if (view->folder_view_mode == ACC_FOLDER_MOVE_MAIL_VIEW_MODE) {
		account_init_genlist_item_class_for_mail_move(view);
	} else {
		account_init_genlist_item_class_for_account_view_list(view);
		account_init_genlist_item_class_for_combined_folder_list(view);
		account_init_genlist_item_class_for_single_folder_list(view);
	}

}

static void _clear_all_genlist_item_class(EmailAccountView *view)
{
	debug_enter();
	RETURN_IF_FAIL(view != NULL);

	if (view->folder_view_mode == ACC_FOLDER_MOVE_MAIL_VIEW_MODE) {
		EMAIL_GENLIST_ITC_FREE(view->itc_group_move);
		EMAIL_GENLIST_ITC_FREE(view->itc_move);
	} else {
		EMAIL_GENLIST_ITC_FREE(view->itc_account_group);
		EMAIL_GENLIST_ITC_FREE(view->itc_account_item);
		EMAIL_GENLIST_ITC_FREE(view->itc_combined);
		EMAIL_GENLIST_ITC_FREE(view->itc_group);
		EMAIL_GENLIST_ITC_FREE(view->itc_single);
		EMAIL_GENLIST_ITC_FREE(view->itc_account_name);
	}
}

int account_create_list(EmailAccountView *view)
{
	debug_enter();
	Evas_Object *gl = NULL;
	int inserted_cnt = 0;
	int err = 0;

	DELETE_EVAS_OBJECT(view->popup);
	DELETE_EVAS_OBJECT(view->more_ctxpopup);

	if (view->gl) {
		DELETE_EVAS_OBJECT(view->gl);
	}

	gl = elm_genlist_add(view->base.module->navi);
	elm_scroller_policy_set(gl, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
	elm_genlist_homogeneous_set(gl, EINA_TRUE);
	elm_genlist_mode_set(gl, ELM_LIST_COMPRESS);
	elm_genlist_block_count_set(gl, EMAIL_GENLIST_MAX_BLOCK_ITEMS_COUNT);
	view->gl = gl;

	/* If one account only. Set as the account. */
	int i = 0;
	if (view->account_list && (view->account_count > 0)) {
		for (; i < view->account_count; i++) {
			account_color_list_add(view, view->account_list[i].account_id, view->account_list[i].color_label);
		}
	}

	debug_log("current account_id: %d", view->account_id);

	if (view->folder_view_mode == ACC_FOLDER_COMBINED_VIEW_MODE) {
		inserted_cnt = account_create_combined_folder_list(view);
	} else if (view->folder_view_mode == ACC_FOLDER_SINGLE_VIEW_MODE
			|| view->folder_view_mode == ACC_FOLDER_COMBINED_SINGLE_VIEW_MODE) {

		inserted_cnt = account_create_single_account_folder_list(view);

		if (inserted_cnt == -1) {
			err = notification_status_message_post(email_get_email_string("IDS_EMAIL_TPOP_SERVER_ERROR_OCCURRED_ABB"));
			if (err != NOTIFICATION_ERROR_NONE) {
				debug_log("fail to notification_status_message_post() : %d\n", err);
			}
			_account_on_back_key(&view->base);
		}

	} else if (view->folder_view_mode == ACC_FOLDER_MOVE_MAIL_VIEW_MODE) {
		inserted_cnt = account_create_folder_list_for_mail_move(view);
	} else if (view->folder_view_mode == ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE) {
		inserted_cnt = account_create_account_list_view(view);
	}

	return inserted_cnt;
}

Evas_Object *account_add_empty_list(EmailAccountView *view)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(view != NULL, NULL);

	Evas_Object *list = elm_list_add(view->base.module->win);
	elm_list_mode_set(list, ELM_LIST_COMPRESS);
	Elm_Object_Item *elm_item;

	elm_item = elm_list_item_append(list, "IDS_EMAIL_BODY_NO_ITEMS_TO_DISPLAY", NULL, NULL, NULL, view);
	elm_object_item_domain_text_translatable_set(elm_item, PACKAGE, EINA_TRUE);
	elm_list_select_mode_set(list, ELM_OBJECT_SELECT_MODE_NONE);
	elm_list_go(list);

	return list;
}

static void _folder_delete_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	RETURN_IF_FAIL(data != NULL);

	EmailAccountView *view = (EmailAccountView *)data;
	DELETE_EVAS_OBJECT(view->more_ctxpopup);

	view->editmode = true;

	account_delete_folder_view(view);

	debug_leave();
}

static void _folder_rename_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	RETURN_IF_FAIL(data != NULL);

	EmailAccountView *view = (EmailAccountView *)data;
	DELETE_EVAS_OBJECT(view->more_ctxpopup);

	view->editmode = true;

	account_rename_folder_view(view);

	debug_leave();
}

static void _account_more_ctxpopup_dismissed_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailAccountView *view = (EmailAccountView *) data;
	DELETE_EVAS_OBJECT(view->more_ctxpopup);
}

static void _account_delete_more_ctxpopup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailAccountView *view = (EmailAccountView *) data;
	evas_object_event_callback_del(view->base.module->navi, EVAS_CALLBACK_RESIZE, _account_resize_more_ctxpopup_cb);
	eext_object_event_callback_del(view->more_ctxpopup, EEXT_CALLBACK_BACK, _account_more_ctxpopup_dismissed_cb);
	eext_object_event_callback_del(view->more_ctxpopup, EEXT_CALLBACK_MORE, _account_more_ctxpopup_dismissed_cb);
	evas_object_smart_callback_del(view->more_ctxpopup, "dismissed", _account_more_ctxpopup_dismissed_cb);
	evas_object_event_callback_del(view->more_ctxpopup, EVAS_CALLBACK_DEL, _account_delete_more_ctxpopup_cb);

}

static void _account_resize_more_ctxpopup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailAccountView *view = (EmailAccountView *) data;

	Evas_Coord w, h;
	int pos = -1;

	elm_win_screen_size_get(view->base.module->win, NULL, NULL, &w, &h);
	pos = elm_win_rotation_get(view->base.module->win);
	debug_log("pos: [%d], w[%d], h[%d]", pos, w, h);

	if (view->more_ctxpopup) {
		switch (pos) {
			case 0:
			case 180:
				evas_object_move(view->more_ctxpopup, (w / 2), h);
				break;
			case 90:
				evas_object_move(view->more_ctxpopup, (h / 2), w);
				break;
			case 270:
				evas_object_move(view->more_ctxpopup, (h / 2), w);
				break;
			}
	}
}

static void _account_setting_module_destroy(void *priv, email_module_h module)
{
	debug_enter();
	retm_if(!module, "module is NULL");
	retm_if(!priv, "priv is NULL");

	EmailAccountView *view = (EmailAccountView *)priv;

	if (module == view->setting) {
		email_module_destroy(view->setting);
		view->setting = NULL;
	}
}

email_module_h account_setting_module_create(void *data, email_module_type_e module_type, email_params_h params)
{
	debug_enter();

	EmailAccountView *view = (EmailAccountView *) data;

	email_module_listener_t listener = { 0 };
	listener.cb_data = view;
	listener.destroy_request_cb = _account_setting_module_destroy;

	return email_module_create_child(view->base.module, module_type, params, &listener);
}

static void _account_settings_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailAccountView *view = (EmailAccountView *) data;
	email_params_h params = NULL;

	DELETE_EVAS_OBJECT(view->more_ctxpopup);

	if (email_params_create(&params) &&
		email_params_add_str(params, EMAIL_BUNDLE_KEY_VIEW_TYPE, EMAIL_BUNDLE_VAL_VIEW_SETTING)) {

		view->setting = account_setting_module_create(view, EMAIL_MODULE_SETTING, params);
	}

	email_params_free(&params);
}

static Elm_Object_Item *_add_ctx_menu_item(EmailAccountView *view, const char *str, Evas_Object *icon, Evas_Smart_Cb cb)
{
	Elm_Object_Item *ctx_menu_item = NULL;

	ctx_menu_item = elm_ctxpopup_item_append(view->more_ctxpopup, str, icon, cb, view);
	elm_object_item_domain_text_translatable_set(ctx_menu_item, PACKAGE, EINA_TRUE);
	return ctx_menu_item;
}

static void _account_more_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL");

	EmailAccountView *view = (EmailAccountView *) data;
	DELETE_EVAS_OBJECT(view->more_ctxpopup);
	if (view->popup) {
		debug_log("Popup already open!");
		return;
	}
	if (view->editmode == false) {
		view->more_ctxpopup = elm_ctxpopup_add(view->base.module->win);
		elm_ctxpopup_auto_hide_disabled_set(view->more_ctxpopup, EINA_TRUE);
		elm_object_style_set(view->more_ctxpopup, "more/default");

		eext_object_event_callback_add(view->more_ctxpopup, EEXT_CALLBACK_BACK, _account_more_ctxpopup_dismissed_cb, view);
		eext_object_event_callback_add(view->more_ctxpopup, EEXT_CALLBACK_MORE, _account_more_ctxpopup_dismissed_cb, view);

		evas_object_smart_callback_add(view->more_ctxpopup, "dismissed", _account_more_ctxpopup_dismissed_cb, view);
		evas_object_event_callback_add(view->more_ctxpopup, EVAS_CALLBACK_DEL, _account_delete_more_ctxpopup_cb, view);
		evas_object_event_callback_add(view->base.module->navi, EVAS_CALLBACK_RESIZE, _account_resize_more_ctxpopup_cb, view);

		if (view->folder_view_mode == ACC_FOLDER_SINGLE_VIEW_MODE
				|| view->folder_view_mode == ACC_FOLDER_COMBINED_SINGLE_VIEW_MODE) {
			_add_ctx_menu_item(view, "IDS_EMAIL_OPT_CREATE_FOLDER_ABB2", NULL, _folder_create_btn_click_cb);
			_add_ctx_menu_item(view, "IDS_EMAIL_OPT_DELETE_FOLDER_ABB", NULL, _folder_delete_cb);
			_add_ctx_menu_item(view, "IDS_EMAIL_OPT_RENAME_FOLDER", NULL, _folder_rename_cb);
		}
		_add_ctx_menu_item(view, "IDS_EMAIL_OPT_SETTINGS", NULL, _account_settings_cb);

		_account_resize_more_ctxpopup_cb(view, NULL, NULL, NULL);
		evas_object_show(view->more_ctxpopup);
	}
}

void account_destroy_view(EmailAccountView *view)
{
	debug_enter();

	RETURN_IF_FAIL(view != NULL);

	int i = 0;

	DELETE_EVAS_OBJECT(view->popup);
	DELETE_EVAS_OBJECT(view->more_ctxpopup);
	DELETE_EVAS_OBJECT(view->gl);

	if (view->folder_view_mode == ACC_FOLDER_MOVE_MAIL_VIEW_MODE) {
		if (view->account_list && view->move_list) {
			email_move_list *move_list = view->move_list;
			for (i = 0; i < view->account_count; i++) {
				email_engine_free_mailbox_list(&move_list[i].mailbox_list, move_list[i].mailbox_cnt);
			}
			FREE(move_list);
		}
	} else if (view->folder_view_mode == ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE) {
		G_LIST_FREE(view->account_group_list);
	}
}

void account_show_all_folder(EmailAccountView *view)
{
	debug_enter();

	if (view->folder_view_mode == ACC_FOLDER_COMBINED_SINGLE_VIEW_MODE) {

		char *temp = account_get_user_email_address(view->account_id);
		if (temp) {
			FREE(view->user_email_address);
			view->user_email_address = temp;
		}
		elm_object_item_text_set(view->base.navi_item, view->user_email_address);

		/* DBUS */
		_dbus_receiver_setup(view);

		view->more_btn = _create_title_btn(view->base.module->navi, NULL, _account_more_clicked_cb, view);
		elm_object_item_part_content_set(view->base.navi_item, "toolbar_more_btn", view->more_btn);
	} else if (view->folder_view_mode == ACC_FOLDER_COMBINED_VIEW_MODE) {
		elm_object_item_part_text_set(view->base.navi_item, "elm.text.title", _("IDS_EMAIL_HEADER_COMBINED_ACCOUNTS_ABB"));
	} else if (view->folder_view_mode == ACC_FOLDER_ACCOUNT_LIST_VIEW_MODE) {
		elm_object_item_part_text_set(view->base.navi_item, "elm.text.title", _("IDS_EMAIL_HEADER_MAILBOX_ABB"));
	} else {
		elm_object_item_part_text_set(view->base.navi_item, "elm.text.title", _("IDS_ST_OPT_NONE"));
	}

	account_create_list(view);
	evas_object_show(view->gl);
	elm_object_part_content_set(view->base.content, "elm.swallow.content", view->gl);
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
/* EOF */
