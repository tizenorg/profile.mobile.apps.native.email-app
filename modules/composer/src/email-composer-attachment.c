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

#include "email-debug.h"
#include "email-composer.h"
#include "email-composer-util.h"
#include "email-composer-types.h"
#include "email-composer-js.h"
#include "email-composer-attachment.h"


/*
 * Declaration for static functions
 */

static void _attachment_download_attachment_response_cb(void *data, Evas_Object *obj, void *event_info);

static COMPOSER_ERROR_TYPE_E _attachment_check_errors_of_file_in_list(EmailComposerView *view, Eina_List *list, int to_be_moved_or_copied);
static void _attachment_separate_list_into_relavant_lists(Eina_List *org_list, Eina_List **normal_list, Eina_List **resize_list, Eina_Bool is_inline);

static void _attachment_process_attachments_list(EmailComposerView *view, Eina_List *list, int to_be_moved_or_copied, Eina_Bool is_inline);
static void _attachment_process_attachments(EmailComposerView *view, Eina_Bool is_inline);
static void _attachment_process_inline_attachments(void *data, Evas_Object *obj, void *event_info);
static void _attachment_process_normal_attachments(void *data, Evas_Object *obj, void *event_info);

static void _attachment_insert_inline_image_to_webkit(EmailComposerView *view, const char *file_path);

static void _attachment_get_image_list_cb(Evas_Object *o, const char *result, void *data);

static email_string_t EMAIL_COMPOSER_STRING_NULL = { NULL, NULL };
static email_string_t EMAIL_COMPOSER_STRING_BUTTON_OK = { PACKAGE, "IDS_EMAIL_BUTTON_OK" };
static email_string_t EMAIL_COMPOSER_STRING_BUTTON_CANCEL = { PACKAGE, "IDS_EMAIL_BUTTON_CANCEL" };
static email_string_t EMAIL_COMPOSER_STRING_HEADER_UNABLE_TO_ADD_ATTACHMENT_ABB = { PACKAGE, "IDS_EMAIL_HEADER_UNABLE_TO_ADD_ATTACHMENT_ABB" };
static email_string_t EMAIL_COMPOSER_STRING_HEADER_UNABLE_TO_SEND_EMAIL_ABB = { PACKAGE, "IDS_EMAIL_HEADER_UNABLE_TO_SEND_EMAIL_ABB" };
static email_string_t EMAIL_COMPOSER_STRING_HEADER_UNABLE_TO_DOWNLOAD_ATTACHMENT_ABB = { PACKAGE, "IDS_EMAIL_HEADER_UNABLE_TO_DOWNLOAD_ATTACHMENT_ABB" };
static email_string_t EMAIL_COMPOSER_STRING_HEADER_DOWNLOAD_ATTACHMENT_HEADER = { PACKAGE, "IDS_EMAIL_HEADER_DOWNLOAD_ATTACHMENT_HEADER" };
static email_string_t EMAIL_COMPOSER_STRING_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED = { PACKAGE, "IDS_EMAIL_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED" };
static email_string_t EMAIL_COMPOSER_STRING_POP_A_FILE_WITH_THIS_NAME_HAS_ALREADY_BEEN_ATTACHED = { PACKAGE, "IDS_EMAIL_POP_A_FILE_WITH_THIS_NAME_HAS_ALREADY_BEEN_ATTACHED" };
static email_string_t EMAIL_COMPOSER_STRING_POP_THE_MAXIMUM_TOTAL_SIZE_OF_ATTACHMENTS_HPS_MB_HAS_BEEN_EXCEEDED_REMOVE_SOME_FILES_AND_TRY_AGAIN = { PACKAGE, "IDS_EMAIL_POP_THE_MAXIMUM_TOTAL_SIZE_OF_ATTACHMENTS_HPS_MB_HAS_BEEN_EXCEEDED_REMOVE_SOME_FILES_AND_TRY_AGAIN" };
static email_string_t EMAIL_COMPOSER_STRING_POP_DOWNLOADING_ING = { PACKAGE, "IDS_EMAIL_POP_DOWNLOADING_ING" };
static email_string_t EMAIL_COMPOSER_STRING_POP_UNABLE_TO_ATTACH_MAXIMUM_NUMBER_OF_FILES_IS_PD = { PACKAGE, "IDS_EMAIL_POP_THE_MAXIMUM_NUMBER_OF_ATTACHMENTS_HPD_HAS_BEEN_REACHED" };


/*
 * Definition for static functions
 */

static void _attachment_download_attachment_response_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	if (view->downloading_attachment) {
		email_engine_stop_working(view->downloading_attachment->account_id, view->handle_for_downloading_attachment);
	}
	composer_attachment_download_attachment_clear_flags(view);
	composer_util_popup_response_cb(view, obj, event_info);

	debug_leave();
}

static COMPOSER_ERROR_TYPE_E _attachment_check_errors_of_file_in_list(EmailComposerView *view, Eina_List *list, int to_be_moved_or_copied)
{
	debug_enter();

	retvm_if(!view, COMPOSER_ERROR_INVALID_ARG, "view is NULL!");
	retvm_if(!list, COMPOSER_ERROR_INVALID_ARG, "list is NULL!");

	int total_attachment_count = eina_list_count(view->attachment_item_list) + eina_list_count(view->attachment_inline_item_list);

	COMPOSER_ERROR_TYPE_E err = COMPOSER_ERROR_NONE;
	Eina_Bool check_exist = EINA_FALSE;
	Eina_Bool check_max_attach = EINA_FALSE;

	char *p = NULL;
	Eina_List *l = NULL;
	Eina_List *l_next = NULL;

	EINA_LIST_FOREACH_SAFE(list, l, l_next, p) {
		Eina_Bool remove = check_max_attach;

		/* 0. Check if path is empty. */
		if (!remove && (!p || !p[0])) {
			remove = EINA_TRUE;
		}

		/* 1. Check the count of attachments. */
		if (!remove && (total_attachment_count >= MAX_ATTACHMENT_ITEM)) {
			check_max_attach = EINA_TRUE;
			remove = EINA_TRUE;
		}

		/* 2. Check the stat of file. */
		if (!remove && !email_file_exists(p)) {
			check_exist = EINA_TRUE;
			remove = EINA_TRUE;
		}

		/* If all check routines are ok, this attachment is added to attachment list to be processed. Otherwise it'll be removed from the list. */
		if (!remove) {
			if (to_be_moved_or_copied) {
				char tmp_file_path[EMAIL_FILEPATH_MAX] = { 0, };
				if (composer_util_file_get_temp_filename(p, tmp_file_path, EMAIL_FILEPATH_MAX, NULL)) {
					if (to_be_moved_or_copied == ATTACH_BY_MOVE_FILE && email_file_mv(p, tmp_file_path)) {
						l->data = g_strdup(tmp_file_path);
						g_free(p);
					} else if (to_be_moved_or_copied == ATTACH_BY_COPY_FILE && email_file_cp(p, tmp_file_path)) {
						l->data = g_strdup(tmp_file_path);
						g_free(p);
					} else if (to_be_moved_or_copied != ATTACH_BY_USE_ORG_FILE) {
						debug_warning("email_file_mv() / email_file_cp() failed!");
					}
				}
			}
			total_attachment_count++;
		} else {
			list = eina_list_remove_list(list, l);
			g_free(p);
		}
	}

	view->attachment_list_to_be_processed = list;

	if (check_max_attach) {
		err = COMPOSER_ERROR_ATTACHMENT_MAX_NUMBER_EXCEEDED;
	} else if (check_exist) {
		err = COMPOSER_ERROR_ATTACHMENT_NOT_EXIST;
	}

	debug_leave();
	return err;
}

static void _attachment_separate_list_into_relavant_lists(Eina_List *org_list, Eina_List **normal_list, Eina_List **resize_list, Eina_Bool is_inline)
{
	debug_enter();

	retm_if(!org_list, "org_list is NULL!");
	retm_if(!normal_list, "normal_list is NULL!");
	retm_if(!resize_list, "resize_list is NULL!");

	char *p = NULL;

	EINA_LIST_FREE(org_list, p) {
		char *mime_type = email_get_mime_type_from_file(p);
		email_file_type_e ftype = email_get_file_type_from_mime_type(mime_type);
		Eina_Bool need_add_to_image_list = EINA_FALSE;
		if (ftype == EMAIL_FILE_TYPE_IMAGE) {
			if (is_inline) {
				int rotation = EXIF_NOT_AVAILABLE;
				need_add_to_image_list = ((g_strcmp0(mime_type, "image/jpeg") == 0) &&
						composer_util_image_get_rotation_for_jpeg_image(p, &rotation) &&
						(rotation != EXIF_NORMAL));
			} else {
				need_add_to_image_list = (composer_util_image_is_resizable_image_type(mime_type) &&
						(email_get_file_size(p) <= MAX_RESIZABLE_IMAGE_FILE_SIZE));
			}
		}
		if (need_add_to_image_list) {
			*resize_list = eina_list_append(*resize_list, p);
		} else {
			*normal_list = eina_list_append(*normal_list, p);
		}

		FREE(mime_type);
	}

	debug_leave();
}

static void _attachment_process_attachments_list(EmailComposerView *view, Eina_List *list, int to_be_moved_or_copied, Eina_Bool is_inline)
{
	debug_enter();

	retm_if(!view, "view is NULL!");
	retm_if(!list, "list is NULL!");

	COMPOSER_ERROR_TYPE_E err = _attachment_check_errors_of_file_in_list(view, list, to_be_moved_or_copied);

	/* To process attachment list after showing error popup if it exists. */
	Evas_Smart_Cb response_cb;
	if (view->attachment_list_to_be_processed) {
		if (is_inline) {
			response_cb = _attachment_process_inline_attachments;
		} else {
			response_cb = _attachment_process_normal_attachments;
		}
		evas_object_freeze_events_set(view->base.module->navi, EINA_TRUE);
		evas_object_freeze_events_set(view->ewk_view, EINA_TRUE);
	} else {
		response_cb = composer_util_popup_response_cb; /* If there're already max attachments, list is NULL. */
	}

	if (err == COMPOSER_ERROR_NONE) {
		response_cb(view, NULL, NULL);
	} else {
		composer_attachment_launch_attachment_error_popup(err, response_cb, view);
	}

	debug_leave();
}

static void _attachment_process_attachments(EmailComposerView *view, Eina_Bool is_inline)
{
	debug_enter();

	char *p = NULL;
	Eina_Bool ret = EINA_TRUE;
	Eina_List *normal_list = NULL;
	Eina_List *resize_list = NULL;

	_attachment_separate_list_into_relavant_lists(view->attachment_list_to_be_processed, &normal_list, &resize_list, is_inline);
	view->attachment_list_to_be_processed = NULL;

	if (normal_list) {
		/* When adding normal and image attachment together, if max attachment size is exceeded, error popup will be displayed after processing image files. */
		ret = composer_attachment_create_list(view, normal_list, is_inline, (resize_list ? EINA_TRUE : EINA_FALSE));
		EINA_LIST_FREE(normal_list, p) {
			g_free(p);
		}
	}

	if (resize_list) {
		if (ret) {
			if (is_inline) {
				if (composer_attachment_resize_image_on_thread(view, resize_list,
						RESIZE_IMAGE_ORIGINAL_SIZE, EINA_TRUE)) {
					return;
				}
			} else {
				if (composer_attachment_show_image_resize_popup(view, resize_list)) {
					return;
				}
			}
			ret = composer_attachment_create_list(view, resize_list, is_inline, EINA_FALSE);
		}
		EINA_LIST_FREE(resize_list, p) {
			g_free(p);
		}
	}

	if (ret) {
		composer_util_popup_response_cb(view, NULL, NULL);
	}

	debug_leave();
}

static void _attachment_process_inline_attachments(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	_attachment_process_attachments(view, EINA_TRUE);

	debug_leave();
}

static void _attachment_process_normal_attachments(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	_attachment_process_attachments(view, EINA_FALSE);

	debug_leave();
}

static void _attachment_insert_inline_image_to_webkit(EmailComposerView *view, const char *file_path)
{
	debug_enter();

	gchar *const escaped_path = g_uri_escape_string(file_path, COMPOSER_URI_ALLOWED_JS_SAFE_CHARS, TRUE);
	gchar *const script = (escaped_path ? g_strdup_printf(EC_JS_INSERT_IMAGE, escaped_path) : NULL);
	g_free(escaped_path);
	if (!script) {
		debug_error("Script generation failed!");
		return;
	}

	if (!ewk_view_script_execute(view->ewk_view, script, NULL, NULL)) {
		debug_error("EC_JS_INSERT_IMAGE is failed!");
	}

	g_free(script);

	debug_leave();
}

static void _attachment_get_image_list_cb(Evas_Object *o, const char *result, void *data)
{
	debug_enter();
	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	composer_util_update_inline_image_list(view, result);
}

/*
 * Definition for exported functions
 */

Eina_Bool composer_attachment_create_list(EmailComposerView *view, Eina_List *attachment_list, Eina_Bool is_inline, Eina_Bool more_list_exists)
{
	debug_enter();

	retvm_if(!view, EINA_FALSE, "view is NULL!");

	Eina_Bool ret = EINA_TRUE;
	Eina_Bool pending_max_attach_popup = EINA_FALSE;
	int total_attachments_size = composer_util_get_total_attachments_size(view, EINA_TRUE);

	char *recv = NULL;
	Eina_List *l = NULL;
	EINA_LIST_FOREACH(attachment_list, l, recv) {
		struct stat file_info;
		if (stat(recv, &file_info) == -1) {
			char err_buff[EMAIL_BUFF_SIZE_HUG] = { 0, };
			debug_error("stat() failed! (%d): %s", errno, strerror_r(errno, err_buff, sizeof(err_buff)));
			composer_attachment_launch_attachment_error_popup(COMPOSER_ERROR_ATTACHMENT_NOT_EXIST, composer_util_popup_response_cb, view);
			ret = EINA_FALSE;
			break;
		}
		total_attachments_size += file_info.st_size;

		if (ret && !pending_max_attach_popup && (total_attachments_size > view->account_info->max_sending_size)) {
			/* If there's more lists to be processed, max size popup'll be launched after processing them */
			if (!more_list_exists) {
				debug_secure_warning("Total size is over! (%s) current:[%d Byte], max:[%d Byte]", recv, total_attachments_size, view->account_info->max_sending_size);
				composer_attachment_launch_attachment_error_popup(COMPOSER_ERROR_ATTACHMENT_MAX_SIZE_EXCEEDED, composer_util_popup_response_cb, view);
				view->need_to_display_max_size_popup = EINA_FALSE;
				ret = EINA_FALSE;
			} else {
				pending_max_attach_popup = EINA_TRUE;
			}
		}

		if (is_inline) {
			_attachment_insert_inline_image_to_webkit(view, recv);
		} else {
			email_attachment_data_t *attachment_data = (email_attachment_data_t *)calloc(1, sizeof(email_attachment_data_t));
			if (!attachment_data) {
				debug_error("Failed to allocate memory for attachment_data!");
				composer_attachment_launch_attachment_error_popup(COMPOSER_ERROR_OUT_OF_MEMORY, composer_util_popup_response_cb, view);
				ret = EINA_FALSE;
				break;
			}

			gchar **tokens = g_strsplit(recv, "/", -1);

			attachment_data->attachment_name = COMPOSER_STRDUP(tokens[g_strv_length(tokens) - 1]);
			attachment_data->attachment_path = COMPOSER_STRDUP(recv);
			attachment_data->save_status = 1;
			attachment_data->attachment_size = file_info.st_size;

			g_strfreev(tokens);

			Eina_Bool attachment_group_needed = eina_list_count(view->attachment_item_list) > 0 ? EINA_TRUE : EINA_FALSE;
			composer_attachment_ui_create_item_layout(view, attachment_data, attachment_group_needed, EINA_TRUE);
		}
	}

	if (view->need_to_display_max_size_popup) {
		if (!view->composer_popup) {
			composer_attachment_launch_attachment_error_popup(COMPOSER_ERROR_ATTACHMENT_MAX_SIZE_EXCEEDED, composer_util_popup_response_cb, view);
			ret = EINA_FALSE;
		}
		view->need_to_display_max_size_popup = EINA_FALSE;
	}

	if (pending_max_attach_popup) {
		view->need_to_display_max_size_popup = EINA_TRUE;
	}

	composer_util_modify_send_button(view);

	debug_leave();
	return ret;
}

void composer_attachment_process_attachments_with_string(EmailComposerView *view, const char *path_string, const char *delim, int to_be_moved_or_copied, Eina_Bool is_inline)
{
	debug_enter();

	retm_if(!view, "view is NULL!");
	retm_if(!path_string, "path_string is NULL!");
	retm_if(!delim, "delim is NULL!");

	/* NOTE: list is freed() after processing adding attachment. Don't remove the list from the caller! */
	Eina_List *list = composer_util_make_string_to_list(path_string, delim);
	_attachment_process_attachments_list(view, list, to_be_moved_or_copied, is_inline);

	debug_leave();
}

void composer_attachment_process_attachments_with_array(EmailComposerView *view, const char **path_array, int path_len, int to_be_moved_or_copied, Eina_Bool is_inline)
{
	debug_enter();

	retm_if(!view, "view is NULL!");
	retm_if(!path_array, "path_array is NULL!");
	retm_if(path_len <= 0, "Invalid parameter: len is [%d]!", path_len);

	/* NOTE: list is freed() after processing adding attachment. Don't remove the list from the caller! */
	Eina_List *list = composer_util_make_array_to_list(path_array, path_len);
	_attachment_process_attachments_list(view, list, to_be_moved_or_copied, is_inline);

	debug_leave();
}

/* NOTE: path_list is freed() after processing adding attachment. Don't remove the list from the caller! */
void composer_attachment_process_attachments_with_list(EmailComposerView *view, Eina_List *path_list, int to_be_moved_or_copied, Eina_Bool is_inline)
{
	debug_enter();

	retm_if(!view, "view is NULL!");
	retm_if(!path_list, "path_list is NULL!");

	_attachment_process_attachments_list(view, path_list, to_be_moved_or_copied, is_inline);

	debug_leave();
}

void composer_attachment_download_attachment(EmailComposerView *view, email_attachment_data_t *attachment)
{
	debug_enter();

	debug_secure("id:[%d], name:(%s), path:(%s), size:[%d], save:[%d], mail_id:[%d], account_id:[%d]", attachment->attachment_id, attachment->attachment_name, attachment->attachment_path,
			attachment->attachment_size, attachment->save_status, attachment->mail_id, attachment->account_id);

	int att_index = 0;
	int att_count = 0;
	email_attachment_data_t *att_info = NULL;

	/* To get attachment index. */
	if (email_engine_get_attachment_data_list(attachment->mail_id, &att_info, &att_count)) {
		int i = 0;
		for (i = 0; i < att_count; i++) {
			att_index++;
			if (attachment->attachment_id == att_info[i].attachment_id) {
				break;
			}
		}
		view->downloading_attachment_index = att_index;

		email_engine_free_attachment_data_list(&att_info, att_count);

		gboolean ret = email_engine_attachment_download(attachment->mail_id, att_index, &view->handle_for_downloading_attachment);
		if (ret) {
			view->downloading_attachment = attachment;
			view->composer_popup = composer_util_popup_create_with_progress_horizontal(view, EMAIL_COMPOSER_STRING_HEADER_DOWNLOAD_ATTACHMENT_HEADER, EMAIL_COMPOSER_STRING_POP_DOWNLOADING_ING,
									_attachment_download_attachment_response_cb, EMAIL_COMPOSER_STRING_BUTTON_CANCEL, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);
		} else {
			view->composer_popup = composer_util_popup_create(view, EMAIL_COMPOSER_STRING_HEADER_UNABLE_TO_DOWNLOAD_ATTACHMENT_ABB, EMAIL_COMPOSER_STRING_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED,
										composer_util_popup_response_cb, EMAIL_COMPOSER_STRING_BUTTON_OK, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);
		}
	} else {
		debug_error("email_engine_get_attachment_data_list() failed!");

		view->composer_popup = composer_util_popup_create(view, EMAIL_COMPOSER_STRING_HEADER_UNABLE_TO_DOWNLOAD_ATTACHMENT_ABB, EMAIL_COMPOSER_STRING_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED,
									composer_util_popup_response_cb, EMAIL_COMPOSER_STRING_BUTTON_OK, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);
	}

	debug_leave();
}

void composer_attachment_download_attachment_clear_flags(void *data)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	view->handle_for_downloading_attachment = 0;
	view->downloading_attachment_index = 0;
	view->downloading_attachment = NULL;

	debug_leave();
}

void composer_attachment_reset_attachment(EmailComposerView *view)
{
	debug_enter();

	Eina_List *list = NULL;
	ComposerAttachmentItemData *attachment_item_data = NULL;
	email_attachment_data_t *inline_att = NULL;

	if (view->attachment_group_layout) {
		elm_box_unpack(view->composer_box, view->attachment_group_layout);
		evas_object_del(view->attachment_group_layout);
		view->attachment_group_layout = NULL;
		view->attachment_group_icon = NULL;
	}

	EINA_LIST_FOREACH(view->attachment_item_list, list, attachment_item_data) {
		if (attachment_item_data) {
			Evas_Object *layout = attachment_item_data->layout;
			if (layout) {
				elm_box_unpack(view->composer_box, layout);
				evas_object_del(layout);
			}

			email_attachment_data_t *att = attachment_item_data->attachment_data;
			if (att) {
				debug_secure("attachment_data file name to be removed : %s", att->attachment_path);

				email_engine_free_attachment_data_list(&att, 1);
			}

			free(attachment_item_data->preview_path);
			free(attachment_item_data);
		}
	}

	EINA_LIST_FOREACH(view->attachment_inline_item_list, list, inline_att) {
		if (inline_att) {
			debug_secure("attachment_data file name to be removed : %s", inline_att->attachment_path);

			email_engine_free_attachment_data_list(&inline_att, 1);
		}
	}

	DELETE_LIST_OBJECT(view->attachment_item_list);
	DELETE_LIST_OBJECT(view->attachment_inline_item_list);

	g_free(view->attachment_inline_img_js_map);
	view->attachment_inline_img_js_map = NULL;

	debug_leave();
}

void composer_attachment_launch_attachment_error_popup(COMPOSER_ERROR_TYPE_E error_type, Evas_Smart_Cb response_cb, void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");
	retm_if(!response_cb, "Invalid parameter: response_cb is NULL!");
	retm_if(error_type == COMPOSER_ERROR_NONE, "Invalid parameter: error_type is (%d)", COMPOSER_ERROR_NONE);

	EmailComposerView *view = (EmailComposerView *)data;

	if (error_type == COMPOSER_ERROR_ATTACHMENT_MAX_NUMBER_EXCEEDED) {
		composer_util_popup_translate_set_variables(view, POPUP_ELEMENT_TYPE_CONTENT, PACKAGE_TYPE_PACKAGE, NULL, EMAIL_COMPOSER_STRING_POP_UNABLE_TO_ATTACH_MAXIMUM_NUMBER_OF_FILES_IS_PD.id, NULL, MAX_ATTACHMENT_ITEM);

		char buf[EMAIL_BUFF_SIZE_1K] = { 0, };
		snprintf(buf, sizeof(buf), email_get_email_string(EMAIL_COMPOSER_STRING_POP_UNABLE_TO_ATTACH_MAXIMUM_NUMBER_OF_FILES_IS_PD.id), MAX_ATTACHMENT_ITEM);
		email_string_t EMAIL_COMPOSER_NO_TRANSITION = { NULL, buf };
		view->composer_popup = composer_util_popup_create(view, EMAIL_COMPOSER_STRING_HEADER_UNABLE_TO_ADD_ATTACHMENT_ABB, EMAIL_COMPOSER_NO_TRANSITION,
		                                                response_cb, EMAIL_COMPOSER_STRING_BUTTON_OK, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);
	} else if (error_type == COMPOSER_ERROR_ATTACHMENT_MAX_SIZE_EXCEEDED) {
		char str[EMAIL_BUFF_SIZE_SML] = { 0, };
		snprintf(str, sizeof(str), "%.1f", view->account_info->max_sending_size / (float)(1024 * 1024));
		composer_util_popup_translate_set_variables(view, POPUP_ELEMENT_TYPE_CONTENT, PACKAGE_TYPE_PACKAGE, NULL, EMAIL_COMPOSER_STRING_POP_THE_MAXIMUM_TOTAL_SIZE_OF_ATTACHMENTS_HPS_MB_HAS_BEEN_EXCEEDED_REMOVE_SOME_FILES_AND_TRY_AGAIN.id, str, 0);

		char buf[EMAIL_BUFF_SIZE_1K] = { 0, };
		snprintf(buf, sizeof(buf), email_get_email_string(EMAIL_COMPOSER_STRING_POP_THE_MAXIMUM_TOTAL_SIZE_OF_ATTACHMENTS_HPS_MB_HAS_BEEN_EXCEEDED_REMOVE_SOME_FILES_AND_TRY_AGAIN.id), str);
		email_string_t EMAIL_COMPOSER_NO_TRANSITION = { NULL, buf };
		view->composer_popup = composer_util_popup_create(view, EMAIL_COMPOSER_STRING_HEADER_UNABLE_TO_SEND_EMAIL_ABB, EMAIL_COMPOSER_NO_TRANSITION,
		                                                 response_cb, EMAIL_COMPOSER_STRING_BUTTON_OK, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);
	} else if (error_type == COMPOSER_ERROR_ATTACHMENT_DUPLICATE) {
		view->composer_popup = composer_util_popup_create(view, EMAIL_COMPOSER_STRING_HEADER_UNABLE_TO_ADD_ATTACHMENT_ABB, EMAIL_COMPOSER_STRING_POP_A_FILE_WITH_THIS_NAME_HAS_ALREADY_BEEN_ATTACHED,
		                                                response_cb, EMAIL_COMPOSER_STRING_BUTTON_OK, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);
	} else {
		view->composer_popup = composer_util_popup_create(view, EMAIL_COMPOSER_STRING_HEADER_UNABLE_TO_ADD_ATTACHMENT_ABB, EMAIL_COMPOSER_STRING_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED,
		                                                response_cb, EMAIL_COMPOSER_STRING_BUTTON_OK, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);
	}

	debug_leave();
}

void composer_attachment_show_attach_panel(EmailComposerView *view, email_attach_panel_mode_type_e mode)
{
	debug_enter();

	if (email_module_launch_attach_panel(view->base.module, mode) == 0) {
		// To update inline image list to calculate total attachment size
		if (ewk_view_script_execute(view->ewk_view, EC_JS_GET_IMAGE_LIST,
				_attachment_get_image_list_cb, (void *)view) == EINA_FALSE) {
			debug_error("EC_JS_GET_IMAGE_LIST failed.");
		}
		elm_object_focus_set(view->ewk_btn, EINA_FALSE);
	}

	debug_leave();
}
