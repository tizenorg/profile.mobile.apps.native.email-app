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

#include "email-engine.h"
#include "email-viewer.h"
#include "email-viewer-util.h"
#include "email-viewer-attachment.h"
#include "email-viewer-header.h"

#define ATTACHMENT_TIMER_UPDATE_INTERVAL 0.1

#define ELLIPSIS_FACTOR 0.5
#define FONT_TEXT_SIZE 40
#define FILE_SIZE_LIMIT (4 * 1024 * 1024)
#define FILE_RESOLUTION_LIMIT 1000000 /* 3 MP */

typedef struct {
	void *data;
	float percentage;
} email_save_thead_notify_data;

/*
 * Declaration for static functions
 */

static int _create_view(email_view_t *self);
static void _destroy_view(email_view_t *self);
static void _on_back_key(email_view_t *self);

static void _popup_response_cb(void *data, Evas_Object *obj, void *event_info);
static int _create_temp_preview_folder(EmailViewerView *view);
static char *_get_ellipsised_attachment_filename(EmailViewerView *view, const char *org_filename, const char *file_size);

static VIEWER_ERROR_TYPE_E _download_attachment(EV_attachment_data *aid);
static VIEWER_ERROR_TYPE_E _save_and_preview_attachment(EV_attachment_data *aid);

static VIEWER_ERROR_TYPE_E _ensure_save_thread(EmailViewerView *view);
static void _stop_save_thread(EmailViewerView *view);
static void *_attachment_save_thread_run(void *data);
static email_ext_save_err_type_e _viewer_save_attachment_on_disk(EV_attachment_data *aid);

static VIEWER_ERROR_TYPE_E _ensure_update_timer(EmailViewerView *view);
static void _stop_update_timer(EmailViewerView *view);
static Eina_Bool _attachment_update_timer_cb(void *data);

static void _invalidate_attachment_item(EV_attachment_data *aid);
static void _stop_update_job(EmailViewerView *view);
static void _attachment_update_job_cb(void *data);

static void _cancel_attachment_task(EV_attachment_data *aid);
static void _cancel_preview_task(EmailViewerView *view, bool keep_download);
static void _cancel_all_attachment_tasks(EmailViewerView *view);

static void _save_all_attachments(EmailViewerView *view);
static void _attachment_download_save_all_cb(void *data, Evas_Object *obj, void *event_info);

static void _viewer_attachment_cancel_cb(EV_attachment_data *aid);
static void _viewer_attachment_save_cb(EV_attachment_data *aid);

static Evas_Object *_gl_attachment_content_get(void *data, Evas_Object *obj, const char *part);
static char *_gl_attachment_group_text_get(void *data, Evas_Object *obj, const char *part);
static void _gl_attachment_content_del(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _gl_attachment_sel(void *data, Evas_Object *obj, void *event_info);
static void _gl_attachment_group_sel(void *data, Evas_Object *obj, void *event_info);

static Elm_Genlist_Item_Class *_viewer_attachment_create_item_class(void);
static Elm_Genlist_Item_Class *_viewer_attachment_create_group_item_class(void);
static Evas_Object *_viewer_attachment_create_genlist(Evas_Object *parent);

static Evas_Object *_viewer_create_gl_item_content_box(Evas_Object *parent, EV_attachment_data *attachment_data);
static Evas_Object *_viewer_create_gl_item_icon_file_type(Evas_Object *parent, const char* attachment_path);
static Evas_Object *_viewer_create_gl_item_download_button(Evas_Object *parent,  EV_attachment_data *attachment_data);
static Evas_Object *_viewer_create_gl_item_label_filename(Evas_Object *parent);

static void _update_attachment_save_cancel_all_buttons(EmailViewerView *view);

static void _unpack_object_from_box(Evas_Object *box, Evas_Object *object);
static void _pack_object_to_box(Evas_Object *box, Evas_Object *object);
static Eina_Bool _viewer_gl_attachment_item_pack_progress_info_items(EV_attachment_data *attachment_data);
static void _viewer_gl_attachment_item_unpack_progress_info_items(EV_attachment_data *attachment_data);
static void _viewer_gl_attachment_update_filename_label_text(EV_attachment_data *attachment_data);

static void _viewer_update_gl_attachment_item(EV_attachment_data *attachment_data);

static void _viewer_create_attachment_list(void *data);
static void _destroy_attachment_list(EmailViewerView *view);
static Evas_Object *_viewer_create_attachment_view_ly(void *data);
static gboolean _viewer_notify_attachment_process_copy_cb(void *data, float percentage);
Evas_Object *_viewer_attachment_create_save_cancel_toolbar_btn(Evas_Object *layout, EmailViewerView *view);

static email_string_t EMAIL_VIEWER_STRING_OK = { PACKAGE, "IDS_EMAIL_BUTTON_OK" };
static email_string_t EMAIL_VIEWER_UNABLE_TO_DOWNLOAD_ATTACH = { PACKAGE, "IDS_EMAIL_HEADER_UNABLE_TO_DOWNLOAD_ATTACHMENT_ABB" };
static email_string_t EMAIL_VIEWER_STRING_NULL = { NULL, NULL };
static email_string_t EMAIL_VIEWER_BODY_FIRST_DOWNLOAD_MESSAGE = { PACKAGE, "IDS_EMAIL_POP_YOU_MUST_DOWNLOAD_THE_MESSAGE_BEFORE_YOU_CAN_DOWNLOAD_THE_ATTACHMENT" };

/*
 * Definition for static functions
 */

static void _popup_response_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	DELETE_EVAS_OBJECT(view->notify);
	debug_leave();
}

static void _create_unable_to_save_attachment_popup(EmailViewerView *view)
{
	email_string_t title = { PACKAGE, "IDS_EMAIL_HEADER_FAILED_TO_SAVE_ATTACHMENT_ABB" };
	email_string_t content = { PACKAGE, "IDS_EMAIL_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED" };
	util_create_notify(view, title, content, 1, EMAIL_VIEWER_STRING_OK,
			_popup_response_cb, EMAIL_VIEWER_STRING_NULL, NULL, NULL);
}

static char *_get_ellipsised_attachment_filename(EmailViewerView *view, const char *org_filename, const char *file_size)
{
	debug_enter();

	char *markup_name = elm_entry_utf8_to_markup(org_filename);
	retvm_if(!markup_name, NULL, "markup_name is NULL elm_entry_utf8_to_markup() - failed");

	char buff[MAX_STR_LEN] = { 0 };
	snprintf(buff, MAX_STR_LEN, HEAD_TEXT_STYLE_ELLIPSISED_FONT_COLOR, ELLIPSIS_FACTOR, FONT_TEXT_SIZE, COLOR_BLACK, markup_name, file_size);

	char *filename = strdup(buff);
	free(markup_name);

	debug_leave();
	return filename;
}

EV_attachment_data *viewer_get_attachment_data(EmailViewerView *view, int info_index)
{
	debug_enter();
	retvm_if(view == NULL, NULL, "Invalid parameter: view[NULL]");

	EV_attachment_data *result = NULL;
	Eina_List *l = NULL;
	EV_attachment_data *attachment_item_data = NULL;

	EINA_LIST_FOREACH(view->attachment_data_list, l, attachment_item_data) {
		if (attachment_item_data) {
			if (attachment_item_data->attachment_info->index == info_index) {
				result = attachment_item_data;
				break;
			}
		}
	}

	debug_leave();
	return result;
}

static int _create_temp_preview_folder(EmailViewerView *view)
{
	debug_enter();
	retvm_if(view == NULL, -1, "Invalid parameter: view[NULL]");

	int nErr = email_create_folder(email_get_viewer_tmp_dir());
	if (nErr == -1) {
		debug_error("email_create_folder(EMAIL_VIEWER_TMP_SHARE_FOLDER) failed!");
		return -1;
	}

	pid_t pid = getpid();
	view->temp_preview_folder_path = g_strdup_printf("%s/preview_%d_%d_%d",
			email_get_viewer_tmp_dir(), pid, view->account_id, view->mail_id);
	nErr = email_create_folder(view->temp_preview_folder_path);
	if (nErr == -1) {
		debug_error("email_create_folder(temp_preview_folder_path) failed!");
		return -1;
	}
	debug_leave();
	return 0;
}

static void _update_attachment_save_cancel_all_buttons(EmailViewerView *view)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	if (!(view->show_cancel_all_btn)) {
		elm_object_domain_translatable_text_set(view->save_all_btn, PACKAGE, "IDS_EMAIL_BUTTON_DOWNLOAD_ALL");
	} else {
		elm_object_domain_translatable_text_set(view->save_all_btn, PACKAGE, "IDS_EMAIL_BUTTON_CANCEL");
	}
}

static VIEWER_ERROR_TYPE_E _download_attachment(EV_attachment_data *aid)
{
	debug_enter();
	retvm_if(aid == NULL, VIEWER_ERROR_NULL_POINTER, "Invalid parameter: aid[NULL]");

	EmailAttachmentType *info = aid->attachment_info;

	if (aid->download_handle != 0) {
		debug_log("Already getting downloaded...!");
		return VIEWER_ERROR_DOWNLOAD_FAIL;
	}

	EmailViewerView *view = aid->view;

	if ((view->account_type == EMAIL_SERVER_TYPE_POP3) &&
			!viewer_check_body_download_status(view->body_download_status, EMAIL_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED)) {
		return VIEWER_ERROR_BODY_NOT_DOWNLOADED;
	}

	int handle = 0;

	/* download attachment */
	gboolean res = email_engine_attachment_download(view->mail_id, info->index, &handle);
	if (!res) {
		int ret = notification_status_message_post(_("IDS_EMAIL_POP_DOWNLOAD_FAILED"));
		debug_warning_if(ret != NOTIFICATION_ERROR_NONE, "notification_status_message_post() failed! ret:(%d)", ret);
		return VIEWER_ERROR_DOWNLOAD_FAIL;
	}

	aid->download_handle = handle;

	debug_log("Assigning Handle[%d] info_index[%d]", handle, info->index);

	return VIEWER_ERROR_NONE;
}

VIEWER_ERROR_TYPE_E viewer_download_and_preview_save_attachment(EV_attachment_data *aid)
{
	debug_enter();
	retvm_if(aid == NULL, -1, "Invalid parameter: aid[NULL]");
	EmailAttachmentType *info = aid->attachment_info;

	VIEWER_ERROR_TYPE_E result = VIEWER_ERROR_NONE;

	if (!aid->is_busy) {
		if (!info->is_downloaded) {
			result = _download_attachment(aid);
		} else {
			result = _save_and_preview_attachment(aid);
		}
		aid->is_busy = (result == VIEWER_ERROR_NONE);
	}

	if (!aid->is_busy) {
		viewer_set_attachment_state(aid, EV_ATT_STATE_IDLE);

		switch (result) {
		case VIEWER_ERROR_ALREADY_SAVED:
			result = VIEWER_ERROR_NONE;
			break;
		case VIEWER_ERROR_BODY_NOT_DOWNLOADED:
			util_create_notify(aid->view, EMAIL_VIEWER_UNABLE_TO_DOWNLOAD_ATTACH,
					EMAIL_VIEWER_BODY_FIRST_DOWNLOAD_MESSAGE, 1,
					EMAIL_VIEWER_STRING_OK, _popup_response_cb, EMAIL_VIEWER_STRING_NULL, NULL, NULL);
			break;
		default:
			_create_unable_to_save_attachment_popup(aid->view);
			break;
		}

	} else if (aid->state == EV_ATT_STATE_IDLE) {
		viewer_set_attachment_state(aid, EV_ATT_STATE_PENDING);
	}

	debug_leave();
	return result;
}

void viewer_set_attachment_state(EV_attachment_data *aid, EV_attachment_state new_state)
{
	debug_enter();
	retm_if(aid == NULL, "Invalid parameter: aid[NULL]");
	EmailViewerView *view = aid->view;

	if (new_state != aid->state) {

		switch (new_state) {
		case EV_ATT_STATE_IDLE:
			break;
		case EV_ATT_STATE_PENDING:
			retm_if(aid->state != EV_ATT_STATE_IDLE, "Can switch to pending only from idle");
			break;
		case EV_ATT_STATE_IN_PROGRESS:
			{
				VIEWER_ERROR_TYPE_E r = _ensure_update_timer(aid->view);
				if (r != VIEWER_ERROR_NONE) {
					debug_warning("_ensure_update_timer() failed: %d", r);
				}
			}
			break;
		default:
			debug_warning("Illegal state: %d", new_state);
			return;
		}

		aid->state = new_state;

		_invalidate_attachment_item(aid);

		switch (viewer_get_all_attachments_state(view)) {
		case EV_ALL_ATT_STATE_IDLE:
			view->show_cancel_all_btn = EINA_FALSE;
			break;
		case EV_ALL_ATT_STATE_BUSY:
			view->show_cancel_all_btn = EINA_TRUE;
			break;
		default:
			break;
		}

		if (view->save_all_btn) {
			_update_attachment_save_cancel_all_buttons(view);
		}
	}

	debug_leave();
}

EV_all_attachment_state viewer_get_all_attachments_state(EmailViewerView *view)
{
	debug_enter();
	retvm_if(view == NULL, EV_ALL_ATT_STATE_INVALID, "Invalid parameter: view[NULL]");

	EV_all_attachment_state result = EV_ALL_ATT_STATE_INVALID;
	Eina_List *l = NULL;
	EV_attachment_data *aid = NULL;

	EINA_LIST_FOREACH(view->attachment_data_list, l, aid) {
		if (aid) {
			EV_all_attachment_state all_state = (aid->state == EV_ATT_STATE_IDLE) ?
					EV_ALL_ATT_STATE_IDLE : EV_ALL_ATT_STATE_BUSY;
			if (result == EV_ALL_ATT_STATE_INVALID) {
				result = all_state;
			} else if (all_state != result) {
				result = EV_ALL_ATT_STATE_MIXED;
				break;
			}
		}
	}

	debug_leave();
	return result;
}

static VIEWER_ERROR_TYPE_E _save_and_preview_attachment(EV_attachment_data *aid)
{
	debug_enter();
	retvm_if(aid == NULL, VIEWER_ERROR_NULL_POINTER, "Invalid parameter: aid[NULL]");

	if (aid->is_saving != 0) {
		debug_log("Saving already in progress...!");
		return VIEWER_ERROR_FAIL;
	}

	EmailAttachmentType *info = aid->attachment_info;
	EmailViewerView *view = aid->view;

	if (!info || STR_INVALID(info->path)) {
		debug_leave();
		return VIEWER_ERROR_INVALID_ARG;
	}

	VIEWER_ERROR_TYPE_E r = _ensure_save_thread(view);
	if (r != VIEWER_ERROR_NONE) {
		debug_error("_ensure_save_thread() failed: %d", r);
		return r;
	}

	r = _ensure_update_timer(view);
	if (r != VIEWER_ERROR_NONE) {
		debug_error("_ensure_update_timer() failed: %d", r);
		return r;
	}

	if (aid == view->preview_aid) {
		email_ext_save_err_type_e r =  email_prepare_temp_file_path(info->index, view->temp_preview_folder_path, info->path, &aid->preview_path);
		if (r == EMAIL_EXT_SAVE_ERR_ALREADY_EXIST) {
			debug_log("Already saved. Showing the preview...");
			viewer_show_attachment_preview(aid);
			return VIEWER_ERROR_ALREADY_SAVED;
		}
		if (r != EMAIL_EXT_SAVE_ERR_NONE) {
			debug_error("viewer_check_attachment_preview_file() failed: %d", r);
			return VIEWER_ERROR_FAIL;
		}
		aid->save_for_preview = true;
	} else {
		aid->save_for_preview = false;
		aid->is_saved = false;
	}

	aid->is_saving = true;
	aid->saving_was_started = false;
	aid->saving_was_finished = false;
	aid->saving_was_canceled = false;

	aid->progress_val = 0;

	pthread_mutex_lock(&view->attachment_save_mutex);

	view->attachment_data_list_to_save = eina_list_append(view->attachment_data_list_to_save, aid);

	pthread_mutex_unlock(&view->attachment_save_mutex);
	pthread_cond_signal(&view->attachment_save_cond);

	debug_leave();
	return VIEWER_ERROR_NONE;
}

static VIEWER_ERROR_TYPE_E _ensure_save_thread(EmailViewerView *view)
{
	debug_enter();
	retvm_if(view == NULL, VIEWER_ERROR_NULL_POINTER, "Invalid parameter: view[NULL]");

	if (view->attachment_save_thread == 0) {
		int r = pthread_create(&view->attachment_save_thread, NULL,
				_attachment_save_thread_run, view);
		if (r != 0) {
			debug_error("Failed to create thread: %d", r);
			view->attachment_save_thread = 0;
			return VIEWER_ERROR_FAIL;
		}
	}

	debug_leave();
	return VIEWER_ERROR_NONE;
}

static void _stop_save_thread(EmailViewerView *view)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	if (view->attachment_save_thread != 0) {
		pthread_t t = view->attachment_save_thread;

		pthread_mutex_lock(&view->attachment_save_mutex);
		view->attachment_save_thread = 0;
		pthread_mutex_unlock(&view->attachment_save_mutex);
		pthread_cond_signal(&view->attachment_save_cond);
		pthread_join(t, NULL);
	}

	debug_leave();
}

static void *_attachment_save_thread_run(void *data)
{
	debug_enter();
	retvm_if(data == NULL, NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = data;

	bool need_exit = false;
	while (!need_exit) {
		EV_attachment_data *aid = NULL;

		debug_log("before lock");
		pthread_mutex_lock(&view->attachment_save_mutex);
		debug_log("LOCKED");

		if (view->attachment_data_list_to_save) {
			aid = eina_list_data_get(view->attachment_data_list_to_save);
			view->attachment_data_list_to_save = eina_list_remove_list(
					view->attachment_data_list_to_save, view->attachment_data_list_to_save);
		} else if (view->attachment_save_thread != 0) {
			debug_log("WAITING...");
			pthread_cond_wait(&view->attachment_save_cond,
					&view->attachment_save_mutex);
		} else {
			need_exit = true;
		}

		pthread_mutex_unlock(&view->attachment_save_mutex);
		debug_log("UNLOCKED");

		if (aid) {
			aid->saving_was_started = true;
			aid->save_result = _viewer_save_attachment_on_disk(aid);
			aid->saving_was_finished = true;
		}
	}

	debug_leave();
	return NULL;
}

static email_ext_save_err_type_e _viewer_save_attachment_on_disk(EV_attachment_data *aid)
{
	debug_enter();
	retvm_if(aid == NULL, EMAIL_EXT_SAVE_ERR_UNKNOWN, "Invalid parameter: aid[NULL]");

	email_ext_save_err_type_e ret = EMAIL_EXT_SAVE_ERR_NONE;

	if (aid->save_for_preview) {
		ret = viewer_save_attachment_for_preview(aid, _viewer_notify_attachment_process_copy_cb);
	} else {
		ret = viewer_save_attachment_in_downloads(aid, _viewer_notify_attachment_process_copy_cb);
	}

	if (ret != EMAIL_EXT_SAVE_ERR_NONE && ret != EMAIL_EXT_SAVE_ERR_ALREADY_EXIST) {
		debug_log("save attachment failed error=%d", ret);
		return ret;
	}

	return EMAIL_EXT_SAVE_ERR_NONE;
}

static VIEWER_ERROR_TYPE_E _ensure_update_timer(EmailViewerView *view)
{
	debug_enter();
	retvm_if(view == NULL, VIEWER_ERROR_NULL_POINTER, "Invalid parameter: view[NULL]");

	if (!view->attachment_update_timer) {
		view->attachment_update_timer = ecore_timer_add(
				ATTACHMENT_TIMER_UPDATE_INTERVAL, _attachment_update_timer_cb, view);
		if (!view->attachment_update_timer) {
			debug_error("Failed to create update timer");
			return VIEWER_ERROR_FAIL;
		}
	}

	debug_leave();
	return VIEWER_ERROR_NONE;
}

static void _stop_update_timer(EmailViewerView *view)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	if (view->attachment_update_timer) {
		ecore_timer_del(view->attachment_update_timer);
		view->attachment_update_timer = NULL;
	}

	debug_leave();
}

static Eina_Bool _attachment_update_timer_cb(void *data)
{
	debug_enter();
	retvm_if(data == NULL, ECORE_CALLBACK_CANCEL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = data;

	Eina_Bool result = ECORE_CALLBACK_CANCEL;

	Eina_List *l = NULL;
	EV_attachment_data *aid = NULL;
	EINA_LIST_FOREACH(view->attachment_data_list, l, aid) {
		if (!aid) {
			continue;
		}

		if (aid->is_saving) {

			if (aid->saving_was_finished) {
				aid->is_saving = false;
				aid->is_busy = false;

				debug_log("Saving finished!");

				if (aid->saving_was_canceled && (aid->state == EV_ATT_STATE_PENDING)) {
					debug_log("New download is pending. Start downloading...");
					viewer_download_and_preview_save_attachment(aid);
					result = ECORE_CALLBACK_RENEW;
					continue;
				}

				if (!aid->saving_was_canceled) {
					if (aid->save_result == EMAIL_EXT_SAVE_ERR_NONE) {
						if (aid == view->preview_aid) {
							viewer_show_attachment_preview(aid);
						} else {
							aid->is_saved = true;
							int ret = notification_status_message_post(email_get_email_string("IDS_EMAIL_TPOP_FILE_SAVED_TO_DOWNLOADS"));
							debug_warning_if(ret != NOTIFICATION_ERROR_NONE, "notification_status_message_post() failed! ret:(%d)", ret);
						}
					} else if (aid->save_result == EMAIL_EXT_SAVE_ERR_NO_FREE_SPACE) {
						viewer_show_storage_full_popup(view);
					}
				}

				viewer_set_attachment_state(aid, EV_ATT_STATE_IDLE);
				continue;
			}

			result = ECORE_CALLBACK_RENEW;

			if ((aid->state != EV_ATT_STATE_IN_PROGRESS) &&
					aid->saving_was_started && !aid->saving_was_canceled) {
				viewer_set_attachment_state(aid, EV_ATT_STATE_IN_PROGRESS);
				continue;
			}
		}

		if (aid->state == EV_ATT_STATE_IN_PROGRESS) {
			_invalidate_attachment_item(aid);
			result = ECORE_CALLBACK_RENEW;
		}
	}

	if (result == ECORE_CALLBACK_CANCEL) {
		view->attachment_update_timer = NULL;
	}

	debug_leave();
	return result;
}

static void _invalidate_attachment_item(EV_attachment_data *aid)
{
	debug_enter();
	retm_if(aid == NULL, "Invalid parameter: aid[NULL]");
	EmailViewerView *view = aid->view;

	aid->need_update = true;

	if (!view->attachment_update_job) {
		view->attachment_update_job = ecore_job_add(_attachment_update_job_cb, view);
	}

	debug_leave();
}

static void _stop_update_job(EmailViewerView *view)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	if (view->attachment_update_job) {
		ecore_job_del(view->attachment_update_job);
		view->attachment_update_job = NULL;
	}

	debug_leave();
}

static void _attachment_update_job_cb(void *data)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = data;

	view->attachment_update_job = NULL;

	Eina_List *l = NULL;
	EV_attachment_data *aid = NULL;
	EINA_LIST_FOREACH(view->attachment_data_list, l, aid) {
		if (aid && aid->need_update) {
			if (aid->content_box) {
				_viewer_update_gl_attachment_item(aid);
			}
			aid->need_update = false;
		}
	}

	debug_leave();
}

static void _cancel_attachment_task(EV_attachment_data *aid)
{
	debug_enter();
	retm_if(aid == NULL, "Invalid parameter: aid[NULL]");
	EmailViewerView *view = aid->view;

	if (aid->state == EV_ATT_STATE_IDLE) {
		debug_log("Already in idle state.");
		return;
	}

	if (aid->download_handle != 0) {
		email_engine_stop_working(view->account_id, aid->download_handle);
		aid->download_handle = 0;
	}

	if (aid->is_saving && !aid->saving_was_canceled) {

		bool was_removed = false;
		Eina_List *l = NULL;
		EV_attachment_data *aid_iter = NULL;

		pthread_mutex_lock(&view->attachment_save_mutex);

		EINA_LIST_FOREACH(view->attachment_data_list_to_save, l, aid_iter) {
			if (aid_iter == aid) {
				view->attachment_data_list_to_save = eina_list_remove_list(
						view->attachment_data_list_to_save, l);
				was_removed = true;
				break;
			}
		}

		pthread_mutex_unlock(&view->attachment_save_mutex);

		if (was_removed) {
			aid->is_saving = false;
			aid->is_busy = false;
		} else {
			aid->saving_was_canceled = true;
		}
	}

	if (aid == view->preview_aid) {
		view->preview_aid = NULL;
	}

	viewer_set_attachment_state(aid, EV_ATT_STATE_IDLE);

	debug_leave();
}

static void _cancel_preview_task(EmailViewerView *view, bool keep_download)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	if (view->preview_aid) {
		if (!keep_download || view->preview_aid->is_saving) {
			_cancel_attachment_task(view->preview_aid);
		} else {
			view->preview_aid = NULL;
		}
	}

	debug_leave();
}

static void _cancel_all_attachment_tasks(EmailViewerView *view)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	Eina_List *l = NULL;
	EV_attachment_data *aid = NULL;
	EINA_LIST_FOREACH(view->attachment_data_list, l, aid) {
		_cancel_attachment_task(aid);
	}

	debug_leave();
}

static void _save_all_attachments(EmailViewerView *view)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");

	/* Cancel preview task only if already saving */
	_cancel_preview_task(view, true);

	Eina_List *l = NULL;
	EV_attachment_data *aid = NULL;
	EINA_LIST_FOREACH(view->attachment_data_list, l, aid) {
		if (aid && (aid->state == EV_ATT_STATE_IDLE)) {
			VIEWER_ERROR_TYPE_E r = viewer_download_and_preview_save_attachment(aid);
			if (r != VIEWER_ERROR_NONE) {
				debug_error("viewer_create_attachment_view() failed: %d", r);
				break;
			}
		}
	}

	debug_leave();
}

static void _attachment_download_save_all_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *) data;

	if (!(view->show_cancel_all_btn)) {
		_save_all_attachments(view);
	} else {
		_cancel_all_attachment_tasks(view);
	}

	debug_leave();
}

static void _viewer_attachment_cancel_cb(EV_attachment_data *aid)
{
	debug_enter();
	retm_if(!aid, "Invalid parameter: aid is NULL!");

	_cancel_attachment_task(aid);

	debug_leave();
}

static void _viewer_attachment_save_cb(EV_attachment_data *aid)
{
	debug_enter();
	retm_if(!aid, "Invalid parameter: aid is NULL!");

	debug_log("Selected item info_index = %d", aid->attachment_info->index);

	viewer_download_and_preview_save_attachment(aid);

	debug_leave();
}

static void _viewer_attachment_button_clicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	debug_enter();
	retm_if(!data, "data is NULL!");
	EV_attachment_data *aid = (EV_attachment_data *)data;

	email_feedback_play_tap_sound();

	if (aid->state != EV_ATT_STATE_IDLE) {
		_viewer_attachment_cancel_cb(aid);
	} else {
		_viewer_attachment_save_cb(aid);
	}
}

static char *_gl_attachment_group_text_get(void *data, Evas_Object *obj, const char *part)
{
	EmailViewerView *view = (EmailViewerView *)data;

	if ((strcmp(part, "elm.text")) == 0) {
		char item_count_info[BUF_LEN_S];
		if (view->normal_att_count == 1) {
			snprintf(item_count_info, sizeof(item_count_info), "%s", email_get_email_string("IDS_EMAIL_BODY_1_FILE_ABB"));
		} else {
			snprintf(item_count_info, sizeof(item_count_info), email_get_email_string("IDS_EMAIL_BODY_PD_FILES_ABB"), view->normal_att_count);
		}

		char *total_file_size = NULL;
		total_file_size = email_get_file_size_string(view->total_att_size);

		strcat(strcat(strcat(item_count_info, " ("), total_file_size), ")");
		g_free(total_file_size);

		return strdup(item_count_info);
	}
	return NULL;
}

static Evas_Object *_viewer_create_gl_item_progress_bar(Evas_Object *parent)
{
	Evas_Object *obj = elm_progressbar_add(parent);
	elm_progressbar_unit_format_set(obj, NULL);
	elm_progressbar_horizontal_set(obj, EINA_TRUE);
	evas_object_size_hint_weight_set(obj, EVAS_HINT_EXPAND, 0.0);
	evas_object_size_hint_align_set(obj, EVAS_HINT_FILL, 0.5);

	return obj;
}

static Evas_Object *_viewer_create_gl_item_progress_text_box(Evas_Object *parent, EV_attachment_data *attachment_item_data)
{
	Evas_Object *obj = elm_box_add(parent);
	elm_box_horizontal_set(obj, EINA_TRUE);
	evas_object_size_hint_weight_set(obj, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(obj, EVAS_HINT_FILL, 0.0);

	Evas_Object *progress_text_left = elm_label_add(parent);
	evas_object_size_hint_weight_set(progress_text_left, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(progress_text_left, 0.0, EVAS_HINT_FILL);
	attachment_item_data->progress_label_left = progress_text_left;

	Evas_Object *progress_text_right = elm_label_add(parent);
	evas_object_size_hint_weight_set(progress_text_right, 0.0, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(progress_text_right, 1.0, EVAS_HINT_FILL);
	attachment_item_data->progress_label_right = progress_text_right;

	evas_object_show(progress_text_left);
	elm_box_pack_end(obj, progress_text_left);
	evas_object_show(progress_text_right);
	elm_box_pack_end(obj, progress_text_right);

	return obj;
}

static Evas_Object *_gl_attachment_content_get(void *data, Evas_Object *obj, const char *part)
{
	debug_enter();
	retvm_if(!data, NULL, "data is NULL!");

	EV_attachment_data *att_data = (EV_attachment_data *)data;
	EmailAttachmentType *info = att_data->attachment_info;

	Evas_Object *ly = elm_layout_add(obj);
	elm_layout_file_set(ly, email_get_viewer_theme_path(), "ev/layout/attachment_item");
	evas_object_size_hint_align_set(ly, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	att_data->content_box = _viewer_create_gl_item_content_box(ly, att_data);

	Evas_Object *attachment_type_icon = _viewer_create_gl_item_icon_file_type(ly, info->name);
	elm_layout_content_set(ly, "ev.swallow.mime_icon", attachment_type_icon);
	evas_object_show(attachment_type_icon);

	att_data->download_button = _viewer_create_gl_item_download_button(ly, att_data);
	elm_layout_content_set(ly, "ev.swallow.download_button", att_data->download_button);
	evas_object_show(att_data->download_button);

	att_data->download_button_icon = elm_layout_add(ly);
	elm_layout_content_set(ly, "ev.swallow.download_icon", att_data->download_button_icon);
	evas_object_show(att_data->download_button_icon);

	att_data->progressbar = _viewer_create_gl_item_progress_bar(ly);
	att_data->progress_info_box = _viewer_create_gl_item_progress_text_box(ly, att_data);
	att_data->filename_label = _viewer_create_gl_item_label_filename(ly);
	_viewer_gl_attachment_update_filename_label_text(att_data);

	evas_object_show(att_data->filename_label);
	elm_box_pack_end (att_data->content_box, att_data->filename_label);
	evas_object_show(att_data->content_box);

	_viewer_update_gl_attachment_item(att_data);

	elm_layout_content_set(ly, "ev.swallow.content", att_data->content_box);
	debug_leave();
	return ly;
}

static void _gl_attachment_content_del(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "data is NULL!");
	EV_attachment_data *aid = data;

	aid->content_box = NULL;
	aid->progressbar = NULL;
	aid->download_button = NULL;
	aid->download_button_icon = NULL;
	aid->filename_label = NULL;
	aid->progress_info_box = NULL;
	aid->progress_label_right = NULL;
	aid->progress_label_left = NULL;

	aid->is_progress_info_packed = false;
}

static void _gl_attachment_sel(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EV_attachment_data *attachment_item_data = (EV_attachment_data *)data;
	EmailViewerView *view = attachment_item_data->view;
	debug_log("Selected item info_index = %d", attachment_item_data->attachment_info->index);

	email_feedback_play_tap_sound();
	elm_genlist_item_selected_set(event_info, EINA_FALSE);

	if (attachment_item_data->state != EV_ATT_STATE_IDLE) {
		debug_log("Currently downloading attachments, no actions taken");
		return;
	}

	/* When a user clicks attachment layout multiple times quickly, app launch requested is called multiple times. */
	ret_if(view->base.module->is_launcher_busy);

	/* Cancel current (if any) preview task in any case */
	_cancel_preview_task(view, false);

	/* Start new preview task */
	view->preview_aid = attachment_item_data;
	if (viewer_download_and_preview_save_attachment(attachment_item_data) != VIEWER_ERROR_NONE) {
		view->preview_aid = NULL;
	}

	debug_leave();
}

static void _gl_attachment_group_sel(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "Invalid parameter: data is NULL!");

	elm_genlist_item_selected_set(event_info, EINA_FALSE);

	debug_leave();
}

static Elm_Genlist_Item_Class *_viewer_attachment_create_item_class(void)
{
	debug_enter();

	Elm_Genlist_Item_Class *itc = elm_genlist_item_class_new();
	retvm_if(!itc, NULL, "itc is NULL!");
	itc->item_style = "full";
	itc->func.text_get = NULL;
	itc->func.content_get = _gl_attachment_content_get;
	itc->func.state_get = NULL;
	itc->func.del = NULL;

	debug_leave();

	return itc;
}

static Elm_Genlist_Item_Class *_viewer_attachment_create_group_item_class(void)
{
	debug_enter();

	Elm_Genlist_Item_Class *itc = elm_genlist_item_class_new();
	retvm_if(!itc, NULL, "itc is NULL!");
	itc->item_style = "group_index";
	itc->func.content_get = NULL;
	itc->func.text_get = _gl_attachment_group_text_get;
	itc->func.state_get = NULL;
	itc->func.del = NULL;

	debug_leave();

	return itc;
}

static Evas_Object *_viewer_attachment_create_genlist(Evas_Object *parent)
{
	debug_enter();

	retvm_if(!parent, NULL, "parent is NULL!");

	Evas_Object *genlist = elm_genlist_add(parent);
	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
	elm_genlist_homogeneous_set(genlist, EINA_TRUE);
	evas_object_show(genlist);

	debug_leave();
	return genlist;
}

static Evas_Object *_viewer_create_gl_item_content_box(Evas_Object *parent, EV_attachment_data *attachment_data)
{
	Evas_Object *obj = elm_box_add(parent);
	evas_object_size_hint_weight_set(obj, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(obj, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_event_callback_add(obj, EVAS_CALLBACK_DEL, _gl_attachment_content_del, attachment_data);

	return obj;
}

static Evas_Object *_viewer_create_gl_item_icon_file_type(Evas_Object *parent, const char* attachment_path)
{
	Evas_Object *obj = elm_image_add(parent);
	evas_object_size_hint_align_set(obj, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(obj, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_propagate_events_set(obj, EINA_TRUE);
	email_file_type_mime_t mime_type;
	char *mime_str = email_get_mime_type_from_file(attachment_path);
	mime_type.mime = mime_str;
	mime_type.type = email_get_file_type_from_mime_type(mime_type.mime);
	elm_image_file_set(obj, email_get_icon_path_from_file_type(mime_type.type), NULL);
	free(mime_str);

	return obj;
}

static Evas_Object *_viewer_create_gl_item_download_button(Evas_Object *parent, EV_attachment_data *attachment_data)
{
	Evas_Object *obj = viewer_load_edj(parent, email_get_viewer_theme_path(), "ev/layout/attachment_button", EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(obj, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_propagate_events_set(obj, EINA_FALSE);
	evas_object_repeat_events_set(obj, EINA_FALSE);
	edje_object_signal_callback_add(_EDJ(obj), "attachment_button.clicked", "edje", _viewer_attachment_button_clicked, attachment_data);
	evas_object_show(obj);

	return obj;
}

static Evas_Object *_viewer_create_gl_item_label_filename(Evas_Object *parent)
{
	Evas_Object *obj = elm_label_add(parent);
	evas_object_size_hint_weight_set(obj, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(obj, EVAS_HINT_FILL, 0.5);

	return obj;
}

static void _unpack_object_from_box(Evas_Object *box, Evas_Object *object)
{
	elm_box_unpack(box, object);
	evas_object_hide(object);
}

static void _pack_object_to_box(Evas_Object *box, Evas_Object *object)
{
	elm_box_pack_end(box, object);
	evas_object_show(object);
}

static Eina_Bool _viewer_gl_attachment_item_pack_progress_info_items(EV_attachment_data *attachment_data)
{
	if (!attachment_data->is_progress_info_packed) {
		_pack_object_to_box(attachment_data->content_box, attachment_data->progressbar);
		_pack_object_to_box(attachment_data->content_box, attachment_data->progress_info_box);
		attachment_data->is_progress_info_packed = true;
		return EINA_TRUE;
	}
	return EINA_FALSE;
}

static void _viewer_gl_attachment_item_unpack_progress_info_items(EV_attachment_data *attachment_data)
{
	if (attachment_data->is_progress_info_packed) {
		_unpack_object_from_box(attachment_data->content_box, attachment_data->progressbar);
		_unpack_object_from_box(attachment_data->content_box, attachment_data->progress_info_box);
		attachment_data->is_progress_info_packed = false;
	}
}

static void _viewer_gl_attachment_update_filename_label_text(EV_attachment_data *attachment_data)
{
	const gint64 size = attachment_data->attachment_info->size;
	gchar *file_size = email_get_file_size_string(size);
	char *filename = _get_ellipsised_attachment_filename(attachment_data->view,
			attachment_data->attachment_info->name, file_size);
	elm_object_text_set(attachment_data->filename_label, filename);
	free(filename);
	g_free(file_size);
}

static void _viewer_update_gl_attachment_item(EV_attachment_data *attachment_data)
{
	debug_enter();

	if (attachment_data->state == EV_ATT_STATE_IDLE) {
		elm_layout_file_set(attachment_data->download_button_icon, email_get_common_theme_path(), EMAIL_IMAGE_ICON_DOWN);

		if (attachment_data->is_saved) {

			evas_object_size_hint_align_set(attachment_data->filename_label, EVAS_HINT_FILL, 1.0);
			_viewer_gl_attachment_item_pack_progress_info_items(attachment_data);

			evas_object_hide(attachment_data->progressbar);
			elm_object_text_set(attachment_data->progress_label_left, email_get_email_string("IDS_EMAIL_POP_SAVED"));
			elm_object_text_set(attachment_data->progress_label_right, "");

		} else {
			evas_object_size_hint_align_set(attachment_data->filename_label, EVAS_HINT_FILL, 0.5);
			_viewer_gl_attachment_item_unpack_progress_info_items(attachment_data);
		}
	} else {
		elm_layout_file_set(attachment_data->download_button_icon, email_get_common_theme_path(), EMAIL_IMAGE_ICON_CANCEL);

		bool pending = true;
		char size_str[MAX_STR_LEN] = { 0 };
		char percent_str[MAX_STR_LEN] = { 0 };

		if (!attachment_data->is_saving || !attachment_data->save_for_preview) {

			pending = (attachment_data->state == EV_ATT_STATE_PENDING);
			const double progress = (pending ? 0.0 : (attachment_data->progress_val / 10000.0));

			elm_progressbar_value_set(attachment_data->progressbar, progress);

			const gint64 size = attachment_data->attachment_info->size;
			gchar *file_size = email_get_file_size_string(size);
			gchar *downloaded_size = email_get_file_size_string(size * progress);

			snprintf(size_str, sizeof(size_str), "%s/%s", downloaded_size, file_size);
			snprintf(percent_str, sizeof(percent_str), "%d%%", (int)(progress * 100));

			g_free(downloaded_size);
			g_free(file_size);
		}

		if (pending) {
			elm_object_style_set(attachment_data->progressbar, "pending");
			elm_progressbar_pulse(attachment_data->progressbar, EINA_TRUE);
		} else {
			elm_object_style_set(attachment_data->progressbar, "default");
		}
		elm_object_text_set(attachment_data->progress_label_left, size_str);
		elm_object_text_set(attachment_data->progress_label_right, percent_str);

		evas_object_size_hint_align_set(attachment_data->filename_label, EVAS_HINT_FILL, 1.0);

		if (!_viewer_gl_attachment_item_pack_progress_info_items(attachment_data)) {
			evas_object_show(attachment_data->progressbar);
		}
	}

	debug_leave();
}

void viewer_update_attachment_item_info(void *data)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EV_attachment_data *attachment_item_data = (EV_attachment_data *)data;
	EmailViewerView *view = attachment_item_data->view;
	EmailAttachmentType *info = attachment_item_data->attachment_info;

	info->path = email_engine_get_attachment_path(info->attach_id);

	if (email_check_file_exist(info->path)) {
		gint64 copy_size = info->size;
		info->size = email_get_file_size(info->path);
		view->total_att_size += (guint64) info->size - copy_size;
		info->is_downloaded = TRUE;

		_viewer_gl_attachment_update_filename_label_text(attachment_item_data);
	}

	elm_genlist_item_update(view->attachment_group_item);

	debug_leave();
}

static void _viewer_create_attachment_list(void *data)
{
	debug_enter();
	retm_if(!data, "Invalid parameter: data is NULL!");
	EmailViewerView *view = (EmailViewerView *)data;

	int i = 0;
	GList *attach_info_list = view->attachment_info_list;
	LIST_ITER_START(i, attach_info_list) {
		EmailAttachmentType *info = (EmailAttachmentType *)LIST_ITER_GET_DATA(i, attach_info_list);
		if (viewer_is_normal_attachment(info) || (view->is_smil_mail)) {
			EV_attachment_data *attachment_item_data = NULL;
			attachment_item_data = calloc(1, sizeof(EV_attachment_data));
			retm_if(!attachment_item_data, "attachment_item_data is NULL!");
			attachment_item_data->view = view;
			attachment_item_data->state = EV_ATT_STATE_IDLE;
			attachment_item_data->download_handle = 0;
			attachment_item_data->is_saving = false;
			attachment_item_data->is_busy = false;
			attachment_item_data->attachment_info = info;
			attachment_item_data->progress_val = 0;
			attachment_item_data->need_update = false;
			attachment_item_data->is_saved = false;
			attachment_item_data->is_progress_info_packed = false;
			debug_log("attachment_item_data->attachment_info :%p", attachment_item_data->attachment_info);

			if (!view->attachment_group_item) {
				view->attachment_group_itc = _viewer_attachment_create_group_item_class();
				view->attachment_group_item = elm_genlist_item_append(view->attachment_genlist, view->attachment_group_itc, view, NULL,
										ELM_GENLIST_ITEM_NONE, _gl_attachment_group_sel, view);
			}
			attachment_item_data->it = elm_genlist_item_append(view->attachment_genlist, view->attachment_itc, attachment_item_data, view->attachment_group_item,
										ELM_GENLIST_ITEM_NONE, _gl_attachment_sel, attachment_item_data);
			elm_genlist_item_select_mode_set(attachment_item_data->it, ELM_OBJECT_SELECT_MODE_ALWAYS);

			view->attachment_data_list = eina_list_append(view->attachment_data_list, attachment_item_data);
		}
	}

	debug_leave();
}

static void _destroy_attachment_list(EmailViewerView *view)
{
	debug_enter();
	retm_if(!view, "Invalid parameter: view[NULL]");

	Eina_List *l = NULL;
	EV_attachment_data *aid = NULL;
	EINA_LIST_FOREACH(view->attachment_data_list, l, aid) {
		free(aid->preview_path);
		free(aid);
	}

	DELETE_LIST_OBJECT(view->attachment_data_list);
	DELETE_EVAS_OBJECT(view->save_all_btn);
	view->save_all_btn = NULL;
	debug_leave();
}

static Evas_Object *_viewer_create_attachment_view_ly(void *data)
{
	debug_enter();
	retvm_if(!data, NULL, "Invalid parameter: data is NULL!");
	EmailViewerView *view = (EmailViewerView *)data;

	Evas_Object *parent = view->base.module->navi;

	Evas_Object *ly = elm_layout_add(parent);
	elm_layout_theme_set(ly, "layout", "application", "noindicator");
	evas_object_size_hint_weight_set(ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ly, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(ly);

	debug_leave();
	return ly;
}

static void _on_back_key(email_view_t *self)
{
	debug_enter();
	EmailViewerView *view = &((EmailViewerModule *)self->module)->view;

	email_module_exit_view(&view->attachment_view);

	debug_leave();
}

static void _destroy_view(email_view_t *self)
{
	debug_enter();
	EmailViewerView *view = &((EmailViewerModule *)self->module)->view;

	DELETE_EVAS_OBJECT(view->attachment_genlist);

	EMAIL_GENLIST_ITC_FREE(view->attachment_itc);
	EMAIL_GENLIST_ITC_FREE(view->attachment_group_itc);

	_cancel_all_attachment_tasks(view);

	_stop_update_job(view);
	_stop_update_timer(view);
	_stop_save_thread(view);

	_destroy_attachment_list(view);

	view->attachment_group_item = NULL;
	view->preview_aid = NULL;

	pthread_mutex_destroy(&view->attachment_save_mutex);
	pthread_cond_destroy(&view->attachment_save_cond);

	memset(&view->attachment_view, 0, sizeof(view->attachment_view));

	debug_leave();
}

static gboolean _viewer_notify_attachment_process_copy_cb(void *data, float percentage)
{
	debug_enter();
	retvm_if(!data, true, "data is NULL");

	EV_attachment_data *aid = data;
	aid->progress_val = (int)(percentage * 100.0 + 0.5);

	if (aid->saving_was_canceled) {
		debug_log("saving_was_canceled is true");
		return true;
	}

	debug_leave();
	return false;
}

void viewer_create_attachment_view(EmailViewerView *view)
{
	debug_enter();

	view->attachment_view.create = _create_view;
	view->attachment_view.destroy = _destroy_view;
	view->attachment_view.on_back_key = _on_back_key;

	int ret = email_module_create_view(view->base.module, &view->attachment_view);
	if (ret != 0) {
		debug_error("email_module_create_view(): failed (%d)", ret);
	}

	debug_leave();
}

Evas_Object *_viewer_attachment_create_save_cancel_toolbar_btn(Evas_Object *layout, EmailViewerView *view)
{
	Evas_Object *btn = elm_button_add(layout);
	elm_object_style_set(btn, "bottom");
	evas_object_smart_callback_add(btn, "clicked", _attachment_download_save_all_cb, view);

	return btn;
}

static int _create_view(email_view_t *self)
{
	debug_enter();
	EmailViewerView *view = &((EmailViewerModule *)self->module)->view;

	pthread_mutex_init(&view->attachment_save_mutex, NULL);
	pthread_cond_init(&view->attachment_save_cond, NULL);

	self->content = _viewer_create_attachment_view_ly(view);
	view->attachment_genlist = _viewer_attachment_create_genlist(self->content);
	view->attachment_itc = _viewer_attachment_create_item_class();
	_viewer_create_attachment_list(view);
	elm_object_part_content_set(self->content, "elm.swallow.content", view->attachment_genlist);

	Elm_Object_Item *attachment_navi_it = email_module_view_push(self, "IDS_EMAIL_HEADER_ATTACHMENTS_ABB", 0);
	elm_object_item_domain_text_translatable_set(attachment_navi_it, PACKAGE, EINA_TRUE);

	int ret = _create_temp_preview_folder(view);
	retvm_if(ret == -1, -1, "_create_temp_preview_folder() is failed!");

	if (view->normal_att_count > 1) {
		Evas_Object *ly = elm_layout_add(view->base.content);
		retvm_if(!ly, -1, "ly is NULL");
		elm_layout_file_set(ly, email_get_viewer_theme_path(), "ev/layout/toolbar_single_button");
		evas_object_size_hint_weight_set(ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(ly, EVAS_HINT_FILL, EVAS_HINT_FILL);

		view->save_all_btn = _viewer_attachment_create_save_cancel_toolbar_btn(ly, view);
		retvm_if(!view->save_all_btn, -1, "view->save_all_btn is NULL");

		elm_layout_content_set(ly, "ev.swallow.toolbar_single_button", view->save_all_btn);

		evas_object_show(view->save_all_btn);
		evas_object_show(ly);
		elm_object_item_part_content_set(attachment_navi_it, "toolbar", ly);
		_update_attachment_save_cancel_all_buttons(view);
	}

	debug_leave();
	return 0;
}
