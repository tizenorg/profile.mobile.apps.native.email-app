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

#include <sys/stat.h>

#include "email-popup-utils.h"
#include "email-debug.h"
#include "email-composer.h"
#include "email-composer-types.h"
#include "email-composer-util.h"
#include "email-composer-attachment.h"


/*
 * Declaration for static functions
 */

static void _attachment_resize_image_thread_cancel_cb(void *data, Evas_Object *obj, void *event_info);
static void _attachment_resize_image_close_response_cb(void *data, Evas_Object *obj, void *event_info);

static void _attachment_resize_image_release_list(void *data);
static void _attachment_resize_image_thread_worker(void *data, Ecore_Thread *th);
static void _attachment_resize_image_thread_notify(void *data, Ecore_Thread *th, void *msg_data);
static void _attachment_resize_image_thread_finish(void *data, Ecore_Thread *th);
static Eina_Bool _attachment_resize_image_do_resize_on_thread(void *data);

static char *_attachment_resize_image_gl_text_get(void *data, Evas_Object *obj, const char *part);
static void _attachment_resize_image_gl_sel(void *data, Evas_Object *obj, void *event_info);

static email_string_t EMAIL_COMPOSER_STRING_NULL = { NULL, NULL };
static email_string_t EMAIL_COMPOSER_STRING_BUTTON_CANCEL = { PACKAGE, "IDS_EMAIL_BUTTON_CANCEL" };
static email_string_t EMAIL_COMPOSER_STRING_HEADER_IMAGE_QUALITY_ABB = { PACKAGE, "IDS_EMAIL_HEADER_IMAGE_QUALITY_ABB" };
static email_string_t EMAIL_COMPOSER_STRING_OPT_ADD_ATTACHMENT_ABB = { PACKAGE, "IDS_EMAIL_OPT_ADD_ATTACHMENT_ABB" };
static email_string_t EMAIL_COMPOSER_STRING_POP_RESIZNG_ING = { PACKAGE, "IDS_EMAIL_TPOP_RESIZING_FILES_ING" };

static Elm_Genlist_Item_Class resize_image_itc;


/*
 * Definition for static functions
 */

static void _attachment_resize_image_thread_cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: datais NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	if (view->thread_resize_image) {
		ecore_thread_cancel(view->thread_resize_image);
		view->thread_resize_image = NULL;
	}
	composer_util_popup_response_cb(view, NULL, NULL);

	debug_leave();
}

static void _attachment_resize_image_close_response_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	if (view->need_to_display_max_size_popup) {
		composer_attachment_launch_attachment_error_popup(COMPOSER_ERROR_ATTACHMENT_MAX_SIZE_EXCEEDED, composer_util_popup_response_cb, view);
		view->need_to_display_max_size_popup = EINA_FALSE;
	} else {
		composer_util_popup_response_cb(view, NULL, NULL);
	}
	_attachment_resize_image_release_list(view);

	debug_leave();
}

static void _attachment_resize_image_release_list(void *data)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;
	Eina_List *l;
	char *recv;

	EINA_LIST_FOREACH(view->resize_image_list, l, recv) {
		g_free(recv);
	}
	DELETE_LIST_OBJECT(view->resize_image_list);

	debug_leave();
}

static void _attachment_resize_image_thread_worker(void *data, Ecore_Thread *th)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	Eina_List *l = NULL;
	char *recv = NULL;

	EINA_LIST_FOREACH(view->resize_image_list, l, recv) {
		if (ecore_thread_check(th)) {
			debug_log("resizing image has been cancelled!");
			break;
		}

		char dst_filepath[EMAIL_FILEPATH_MAX + 1] = { 0, };
		if (composer_util_file_get_temp_filename(recv, dst_filepath, EMAIL_FILEPATH_MAX, "_resized")) {
			if (composer_util_image_scale_down_with_quality(recv, dst_filepath, view->resize_image_quality)) {
				struct stat file_info;
				if (stat(dst_filepath, &file_info) == -1) {
					char err_buff[EMAIL_BUFF_SIZE_HUG] = { 0, };
					debug_error("stat() failed! (%d): %s", errno, strerror_r(errno, err_buff, sizeof(err_buff)));
				} else {
					debug_secure("image resized (%s) => (%s)", recv, dst_filepath);
					g_free(recv);
					l->data = g_strdup(dst_filepath);
				}
			} else {
				debug_warning("composer_util_image_scale_down_with_quality() failed!");
			}
		} else {
			debug_warning("composer_util_file_get_temp_filename() failed!");
		}
	}

	if (ecore_thread_check(th)) {
		ecore_thread_feedback(th, (void *)(ptrdiff_t)EINA_FALSE);
	} else {
		ecore_thread_feedback(th, (void *)(ptrdiff_t)EINA_TRUE);
	}

	debug_leave();
}

static void _attachment_resize_image_thread_notify(void *data, Ecore_Thread *th, void *msg_data)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;
	int err = (int)(ptrdiff_t)msg_data;
	Eina_Bool ret = EINA_TRUE;

	if (err) {
		ret = composer_attachment_create_list(view, view->resize_image_list, view->resize_image_inline, EINA_FALSE);
	}
	if (ret) {
		composer_util_popup_response_cb(view, NULL, NULL);
	}

	debug_leave();
}

static void _attachment_resize_image_thread_finish(void *data, Ecore_Thread *th)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	view->thread_resize_image = NULL;
	_attachment_resize_image_release_list(view);

	debug_leave();
}

static Eina_Bool _attachment_resize_image_do_resize_on_thread(void *data)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	/* Not to show IME before destroying is called. (The focus moves to entry..) */
	elm_object_tree_focus_allow_set(view->composer_layout, EINA_FALSE);

	view->composer_popup = composer_util_popup_create_with_progress_horizontal(view, EMAIL_COMPOSER_STRING_OPT_ADD_ATTACHMENT_ABB, EMAIL_COMPOSER_STRING_POP_RESIZNG_ING, _attachment_resize_image_thread_cancel_cb,
	                                                                         EMAIL_COMPOSER_STRING_BUTTON_CANCEL, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);

	/* Resizing images take long time and have big load. so we use another thread to resize images. */
	view->thread_resize_image = ecore_thread_feedback_run(_attachment_resize_image_thread_worker, _attachment_resize_image_thread_notify,
	                                                     _attachment_resize_image_thread_finish, _attachment_resize_image_thread_finish, view, EINA_TRUE);
	if (view->thread_resize_image) {
		return EINA_TRUE;
	} else {
		debug_error("ecore_thread_feedback_run() failed!");
	}

	debug_leave();
	return EINA_FALSE;
}

static char *_attachment_resize_image_gl_text_get(void *data, Evas_Object *obj, const char *part)
{
	int index = (int)(ptrdiff_t)data;

	if (!strcmp(part, "elm.text")) {
		if (index == 0)
			return strdup(email_get_email_string("IDS_EMAIL_OPT_ORIGINAL"));
		else if (index == 1)
			return strdup(email_get_email_string("IDS_EMAIL_OPT_MEDIUM_M_IMAGE_SIZE"));
		else if (index == 2) {
			return strdup(email_get_email_string("IDS_EMAIL_OPT_LOW_M_IMAGE_SIZE"));
		}
	}

	return NULL;
}

static void _attachment_resize_image_gl_sel(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	EmailComposerView *view = (EmailComposerView *)data;
	Eina_Bool ret = EINA_TRUE;

	if (item != NULL) {
		int index = (int)(ptrdiff_t)elm_object_item_data_get(item);
		elm_genlist_item_selected_set(item, EINA_FALSE);

		if (index == 1) {
			view->resize_image_quality = RESIZE_IMAGE_MEDIUM_SIZE;
		} else if (index == 2) {
			view->resize_image_quality = RESIZE_IMAGE_SMALL_SIZE;
		} else {
			view->resize_image_quality = RESIZE_IMAGE_ORIGINAL_SIZE;
		}

		if (view->resize_image_quality != RESIZE_IMAGE_ORIGINAL_SIZE) {
			ret = _attachment_resize_image_do_resize_on_thread(view);
			ret_if(ret);
		}

		ret = composer_attachment_create_list(view, view->resize_image_list, view->resize_image_inline, EINA_FALSE);
	}

	if (ret) {
		composer_util_popup_response_cb(view, NULL, NULL);
	}
	_attachment_resize_image_release_list(view);

	debug_leave();
}


/*
 * Definition for exported functions
 */

Eina_Bool composer_attachment_show_image_resize_popup(EmailComposerView *view, Eina_List *attachment_list)
{
	debug_enter();

	retvm_if(!view, EINA_FALSE, "view is NULL!");
	retvm_if(!attachment_list, EINA_FALSE, "attachment_list is NULL!");

	view->resize_image_list = attachment_list;
	view->resize_image_inline = EINA_FALSE;

	view->composer_popup = common_util_create_popup(view->base.module->win,
			EMAIL_COMPOSER_STRING_HEADER_IMAGE_QUALITY_ABB,
			NULL, EMAIL_COMPOSER_STRING_NULL,
			NULL, EMAIL_COMPOSER_STRING_NULL,
			NULL, EMAIL_COMPOSER_STRING_NULL,
			_attachment_resize_image_close_response_cb, EINA_TRUE, view);

	resize_image_itc.item_style = "type1";
	resize_image_itc.func.text_get = _attachment_resize_image_gl_text_get;
	resize_image_itc.func.content_get = NULL;
	resize_image_itc.func.state_get = NULL;
	resize_image_itc.func.del = NULL;

	Evas_Object *genlist = elm_genlist_add(view->composer_popup);
	evas_object_data_set(genlist, COMPOSER_EVAS_DATA_NAME, view);
	elm_genlist_homogeneous_set(genlist, EINA_TRUE);
	elm_scroller_policy_set(genlist, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
	elm_scroller_content_min_limit(genlist, EINA_FALSE, EINA_TRUE);

	int index = 0;
	for (index = 0; index < 3; index++) {
		elm_genlist_item_append(genlist, &resize_image_itc, (void *)(ptrdiff_t)index, NULL, ELM_GENLIST_ITEM_NONE, _attachment_resize_image_gl_sel, (void *)view);
	}

	elm_object_content_set(view->composer_popup, genlist);
	evas_object_show(genlist);

	debug_leave();
	return EINA_TRUE;
}

Eina_Bool composer_attachment_resize_image_on_thread(EmailComposerView *view, Eina_List *attachment_list,
		int quality, Eina_Bool is_inline)
{
	debug_enter();

	retvm_if(!view, EINA_FALSE, "view is NULL!");
	retvm_if(!attachment_list, EINA_FALSE, "attachment_list is NULL!");

	view->resize_image_list = attachment_list;
	view->resize_image_quality = quality;
	view->resize_image_inline = is_inline;

	if (_attachment_resize_image_do_resize_on_thread(view)) {
		return EINA_TRUE;
	}

	view->resize_image_list = NULL;

	debug_leave();
	return EINA_TRUE;
}
