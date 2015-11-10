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

static COMPOSER_ERROR_TYPE_E _attachment_check_errors_of_file_in_list(EmailComposerUGD *ugd, Eina_List *list, int to_be_moved_or_copied, Eina_Bool is_inline);
static void _attachment_separate_list_into_relavant_lists(EmailComposerUGD *ugd, Eina_List *org_list, Eina_List **normal_list, Eina_List **image_list);

static void _attachment_process_attachments_list(EmailComposerUGD *ugd, Eina_List *list, int to_be_moved_or_copied, Eina_Bool is_inline);
static void _attachment_process_inline_attachments(void *data, Evas_Object *obj, void *event_info);
static void _attachment_process_normal_attachments(void *data, Evas_Object *obj, void *event_info);

static void _attachment_insert_inline_image_to_webkit(EmailComposerUGD *ugd, const char *pszImgPath);
static void _attachment_insert_inline_image(EmailComposerUGD *ugd, const char *path);

static EmailCommonStringType EMAIL_COMPOSER_STRING_NULL = { NULL, NULL };
static EmailCommonStringType EMAIL_COMPOSER_STRING_BUTTON_OK = { PACKAGE, "IDS_EMAIL_BUTTON_OK" };
static EmailCommonStringType EMAIL_COMPOSER_STRING_BUTTON_CANCEL = { PACKAGE, "IDS_EMAIL_BUTTON_CANCEL" };
static EmailCommonStringType EMAIL_COMPOSER_STRING_HEADER_UNABLE_TO_ADD_ATTACHMENT_ABB = { PACKAGE, "IDS_EMAIL_HEADER_UNABLE_TO_ADD_ATTACHMENT_ABB" };
static EmailCommonStringType EMAIL_COMPOSER_STRING_HEADER_UNABLE_TO_SEND_EMAIL_ABB = { PACKAGE, "IDS_EMAIL_HEADER_UNABLE_TO_SEND_EMAIL_ABB" };
static EmailCommonStringType EMAIL_COMPOSER_STRING_HEADER_UNABLE_TO_DOWNLOAD_ATTACHMENT_ABB = { PACKAGE, "IDS_EMAIL_HEADER_UNABLE_TO_DOWNLOAD_ATTACHMENT_ABB" };
static EmailCommonStringType EMAIL_COMPOSER_STRING_HEADER_DOWNLOAD_ATTACHMENT_HEADER = { PACKAGE, "IDS_EMAIL_HEADER_DOWNLOAD_ATTACHMENT_HEADER" };
static EmailCommonStringType EMAIL_COMPOSER_STRING_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED = { PACKAGE, "IDS_EMAIL_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED" };
static EmailCommonStringType EMAIL_COMPOSER_STRING_POP_A_FILE_WITH_THIS_NAME_HAS_ALREADY_BEEN_ATTACHED = { PACKAGE, "IDS_EMAIL_POP_A_FILE_WITH_THIS_NAME_HAS_ALREADY_BEEN_ATTACHED" };
static EmailCommonStringType EMAIL_COMPOSER_STRING_POP_THE_MAXIMUM_TOTAL_SIZE_OF_ATTACHMENTS_HPS_MB_HAS_BEEN_EXCEEDED_REMOVE_SOME_FILES_AND_TRY_AGAIN = { PACKAGE, "IDS_EMAIL_POP_THE_MAXIMUM_TOTAL_SIZE_OF_ATTACHMENTS_HPS_MB_HAS_BEEN_EXCEEDED_REMOVE_SOME_FILES_AND_TRY_AGAIN" };
static EmailCommonStringType EMAIL_COMPOSER_STRING_POP_DOWNLOADING_ING = { PACKAGE, "IDS_EMAIL_POP_DOWNLOADING_ING" };
static EmailCommonStringType EMAIL_COMPOSER_STRING_POP_UNABLE_TO_ATTACH_MAXIMUM_NUMBER_OF_FILES_IS_PD = { PACKAGE, "IDS_EMAIL_POP_THE_MAXIMUM_NUMBER_OF_ATTACHMENTS_HPD_HAS_BEEN_REACHED" };


/*
 * Definition for static functions
 */

static void _attachment_download_attachment_response_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	if (ugd->downloading_attachment) {
		email_engine_stop_working(ugd->downloading_attachment->account_id, ugd->handle_for_downloading_attachment);
	}
	composer_attachment_download_attachment_clear_flags(ugd);
	composer_util_popup_response_cb(ugd, obj, event_info);

	debug_leave();
}

static COMPOSER_ERROR_TYPE_E _attachment_check_errors_of_file_in_list(EmailComposerUGD *ugd, Eina_List *list, int to_be_moved_or_copied, Eina_Bool is_inline)
{
	debug_enter();

	retvm_if(!ugd, COMPOSER_ERROR_INVALID_ARG, "ugd is NULL!");
	retvm_if(!list, COMPOSER_ERROR_INVALID_ARG, "list is NULL!");

	int total_attachment_count = eina_list_count(ugd->attachment_item_list) + eina_list_count(ugd->attachment_inline_item_list);

	COMPOSER_ERROR_TYPE_E err = COMPOSER_ERROR_NONE;
	Eina_Bool check_exist = EINA_FALSE;
	Eina_Bool check_max_attach = EINA_FALSE;

	char *p = NULL;
	Eina_List *l = NULL;

	EINA_LIST_FOREACH(list, l, p) {
		Eina_Bool remove = check_max_attach;
		if (!p || (strlen(p) == 0)) {
			continue;
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
					} else {
						debug_warning("email_file_mv() / email_file_cp() failed!");
					}
				}
			}
			total_attachment_count++;
		} else {
			list = eina_list_remove(list, p);
			g_free(p);
		}
	}

	ugd->attachment_list_to_be_processed = list;

	if (check_max_attach) {
		err = COMPOSER_ERROR_ATTACHMENT_MAX_NUMBER_EXCEEDED;
	} else if (check_exist) {
		err = COMPOSER_ERROR_ATTACHMENT_NOT_EXIST;
	}

	debug_leave();
	return err;
}

static void _attachment_separate_list_into_relavant_lists(EmailComposerUGD *ugd, Eina_List *org_list, Eina_List **normal_list, Eina_List **image_list)
{
	debug_enter();

	retm_if(!ugd, "ugd is NULL!");
	retm_if(!org_list, "org_list is NULL!");
	retm_if(!normal_list, "normal_list is NULL!");
	retm_if(!image_list, "image_list is NULL!");

	char *p = NULL;
	Eina_List *l = NULL;

	EINA_LIST_FOREACH(org_list, l, p) {
		char *mime_type = email_get_mime_type_from_file(p);
		EMAIL_FILE_TYPE ftype = email_get_file_type_from_mime_type(mime_type);
		if ((ftype == EMAIL_FILE_TYPE_IMAGE) && composer_util_image_is_resizable_image_type(mime_type) && (email_get_file_size(p) <= MAX_RESIZABLE_IMAGE_FILE_SIZE)) {
			*image_list = eina_list_append(*image_list, g_strdup(p));
		} else {
			*normal_list = eina_list_append(*normal_list, g_strdup(p));
		}

		FREE(mime_type);
	}

	debug_leave();
}

static void _attachment_process_attachments_list(EmailComposerUGD *ugd, Eina_List *list, int to_be_moved_or_copied, Eina_Bool is_inline)
{
	debug_enter();

	retm_if(!ugd, "ugd is NULL!");
	retm_if(!list, "list is NULL!");

	COMPOSER_ERROR_TYPE_E err = _attachment_check_errors_of_file_in_list(ugd, list, to_be_moved_or_copied, is_inline);

	/* To process attachment list after showing error popup if it exists. */
	Evas_Smart_Cb response_cb;
	if (ugd->attachment_list_to_be_processed) {
		if (is_inline) {
			response_cb = _attachment_process_inline_attachments;
		} else {
			response_cb = _attachment_process_normal_attachments;
			evas_object_freeze_events_set(ugd->base.module->navi, EINA_TRUE);
			evas_object_freeze_events_set(ugd->ewk_view, EINA_TRUE);
		}
	} else {
		response_cb = composer_util_popup_response_cb; /* If there're already max attachments, list is NULL. */
	}

	if (err == COMPOSER_ERROR_NONE) {
		response_cb(ugd, NULL, NULL);
	} else {
		composer_attachment_launch_attachment_error_popup(err, response_cb, ugd);
	}

	debug_leave();
}

static void _attachment_process_inline_attachments(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	char *p = NULL;

	Eina_Bool ret = composer_attachment_create_list(ugd, ugd->attachment_list_to_be_processed, EINA_TRUE, EINA_FALSE);
	if (ret) {
		composer_util_popup_response_cb(ugd, NULL, NULL);
	}

	EINA_LIST_FREE(ugd->attachment_list_to_be_processed, p) {
		g_free(p);
	}
	ugd->attachment_list_to_be_processed = NULL;

	debug_leave();
}

static void _attachment_process_normal_attachments(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	char *p = NULL;
	Eina_Bool ret = EINA_TRUE;
	Eina_List *normal_list = NULL;
	Eina_List *image_list = NULL;

	_attachment_separate_list_into_relavant_lists(ugd, ugd->attachment_list_to_be_processed, &normal_list, &image_list);

	if (normal_list) {
		/* When adding normal and image attachment together, if max attachment size is exceeded, error popup will be displayed after processing image files. */
		ret = composer_attachment_create_list(ugd, normal_list, EINA_FALSE, (image_list ? EINA_TRUE : EINA_FALSE));
		if (!image_list && ret) {
			composer_util_popup_response_cb(ugd, NULL, NULL);
		}
		EINA_LIST_FREE(normal_list, p) {
			g_free(p);
		}
	}

	if (image_list) {
		if (ret) {
			composer_attachment_resize_image(ugd, image_list);
		}
		EINA_LIST_FREE(image_list, p) {
			g_free(p);
		}
	}

	EINA_LIST_FREE(ugd->attachment_list_to_be_processed, p) {
		g_free(p);
	}
	ugd->attachment_list_to_be_processed = NULL;

	debug_leave();
}

static void _attachment_insert_inline_image_to_webkit(EmailComposerUGD *ugd, const char *pszImgPath)
{
	debug_enter();

	int org_width = 0, org_height = 0, new_width = 0, new_height = 0;
	float nImgResizeRatio = 0.0;
	int limit = 0;
	double scale = 0.0;
	gchar *pszHtmlTag = NULL;
	Evas_Coord x = 0, y = 0, w = 0, h = 0;

	if (!composer_util_image_get_size(ugd, pszImgPath, &org_width, &org_height)) {
		debug_error("composer_image_get_size() failed!");
		return;
	}
	scale = ewk_view_scale_get(ugd->ewk_view);
	retm_if(scale <= 0.0, "ewk_view_scale_get() failed!");

	evas_object_geometry_get(ugd->base.module->win, &x, &y, &w, &h);
	if (ugd->is_horizontal) {
		limit = h / scale;
	} else {
		limit = w / scale;
	}

	if (org_width > limit * IMAGE_RESIZE_RATIO) {
		nImgResizeRatio = (double)limit * IMAGE_RESIZE_RATIO / (double)org_width;
	} else {
		nImgResizeRatio = 1.0;
	}

	new_width = org_width * nImgResizeRatio;
	new_height = org_height * nImgResizeRatio;

	debug_secure("Original image: [%4dx%4d], limit:[%d], ratio:[%f]", org_width, org_height, limit, nImgResizeRatio);
	debug_secure("Resized image: [%4dx%4d]", new_width, new_height);

	gchar *escaped_string = g_uri_escape_string(pszImgPath, G_URI_RESERVED_CHARS_ALLOWED_IN_PATH, true);
	debug_secure("image path is %s and escaped string is %s", pszImgPath, escaped_string);
	gchar *pszId = g_strdup_printf("%s%d", pszImgPath, eina_list_count(ugd->attachment_inline_item_list));
	pszHtmlTag = g_strdup_printf(EC_TAG_IMG_WITH_SIZE, escaped_string, new_width, new_height, pszId);

	debug_secure("pszHtmlTag : %s", pszHtmlTag);

	if (!ewk_view_command_execute(ugd->ewk_view, EC_EWK_COMMAND_INSERT_HTML, pszHtmlTag)) {
		debug_error("ewk_view_command_execute(%s) failed!", EC_EWK_COMMAND_INSERT_HTML);
	}

	/* XXX; Need to check the behaviour with OnNodeInserted() in email-composer.js if insert image feature is enabled. */
	gchar *handler = g_strdup_printf(EC_JS_CONNECT_EVENT_LISTENER, pszId);
	if (!ewk_view_script_execute(ugd->ewk_view, handler, NULL, NULL)) {
		debug_error("EC_JS_CONNECT_EVENT_LISTENER is failed!");
	}

	FREE(escaped_string);
	FREE(handler);
	FREE(pszId);
	FREE(pszHtmlTag);

	debug_leave();
}

static void _attachment_insert_inline_image(EmailComposerUGD *ugd, const char *path)
{
	debug_enter();

	/* XXX; The feature Inserting inline image to webkit is not supported in current UX.
	 * XXX; Uncomment these codes if rotating jpeg image with orientation is needed.
	 */
	/*char tmp_file_path[EMAIL_FILEPATH_MAX + 1] = { 0, };
	char *mime_type = email_get_mime_type_from_file(path);

	if (!g_strcmp0(mime_type, "image/jpeg")) {
		int rotation = 0;
		if (composer_util_image_get_rotation_for_jpeg_image(path, &rotation)) {
			if (rotation == EXIF_ROT_90 || rotation == EXIF_ROT_180 || rotation == EXIF_ROT_270) {
				if (composer_util_file_get_temp_filename(path, tmp_file_path, EMAIL_FILEPATH_MAX, NULL)) {
					if (composer_util_image_rotate_jpeg_image(path, tmp_file_path, rotation)) {
						path = tmp_file_path;
					} else {
						debug_warning("composer_util_image_rotate_jpeg_image() failed!");
					}
				} else {
					debug_warning("composer_util_file_get_temp_filename() failed!");
				}
			} else {
				debug_log("No need to rotate image (rotation = %d)", rotation);
			}
		} else {
			debug_log("composer_util_image_get_rotation_for_jpeg_image() failed!");
		}
	}
	FREE(mime_type);*/

	_attachment_insert_inline_image_to_webkit(ugd, path);

	debug_leave();
}


/*
 * Definition for exported functions
 */

Eina_Bool composer_attachment_create_list(EmailComposerUGD *ugd, Eina_List *attachment_list, Eina_Bool is_inline, Eina_Bool more_list_exists)
{
	debug_enter();

	retvm_if(!ugd, EINA_FALSE, "ugd is NULL!");

	Eina_Bool ret = EINA_TRUE;
	Eina_Bool pending_max_attach_popup = EINA_FALSE;
	int total_attachments_size = composer_util_get_total_attachments_size(ugd, EINA_TRUE);

	char *recv = NULL;
	Eina_List *l = NULL;
	EINA_LIST_FOREACH(attachment_list, l, recv) {
		struct stat file_info;
		if (stat(recv, &file_info) == -1) {
			debug_error("stat() failed! (%d): %s", errno, strerror(errno));
			composer_attachment_launch_attachment_error_popup(COMPOSER_ERROR_ATTACHMENT_NOT_EXIST, composer_util_popup_response_cb, ugd);
			ret = EINA_FALSE;
			break;
		}
		total_attachments_size += file_info.st_size;

		if (ret && !pending_max_attach_popup && (total_attachments_size > ugd->account_info->max_sending_size)) {
			/* If there's more lists to be processed, max size popup'll be launched after processing them */
			if (!more_list_exists) {
				debug_secure_warning("Total size is over! (%s) current:[%d Byte], max:[%d Byte]", recv, total_attachments_size, ugd->account_info->max_sending_size);
				composer_attachment_launch_attachment_error_popup(COMPOSER_ERROR_ATTACHMENT_MAX_SIZE_EXCEEDED, composer_util_popup_response_cb, ugd);
				ugd->need_to_display_max_size_popup = EINA_FALSE;
				ret = EINA_FALSE;
			} else {
				pending_max_attach_popup = EINA_TRUE;
			}
		}

		if (is_inline) {
			_attachment_insert_inline_image(ugd, recv);
		} else {
			email_attachment_data_t *attachment_data = (email_attachment_data_t *)calloc(1, sizeof(email_attachment_data_t));
			if (!attachment_data) {
				debug_error("Failed to allocate memory for attachment_data!");
				composer_attachment_launch_attachment_error_popup(COMPOSER_ERROR_OUT_OF_MEMORY, composer_util_popup_response_cb, ugd);
				ret = EINA_FALSE;
				break;
			}

			gchar **tokens = g_strsplit(recv, "/", -1);

			attachment_data->attachment_name = COMPOSER_STRDUP(tokens[g_strv_length(tokens) - 1]);
			attachment_data->attachment_path = COMPOSER_STRDUP(recv);
			attachment_data->save_status = 1;
			attachment_data->attachment_size = file_info.st_size;

			g_strfreev(tokens);

			Eina_Bool attachment_group_needed = eina_list_count(ugd->attachment_item_list) > 0 ? EINA_TRUE : EINA_FALSE;
			composer_attachment_ui_create_item_layout(ugd, attachment_data, attachment_group_needed, EINA_TRUE);
		}
	}

	if (ugd->need_to_display_max_size_popup) {
		if (!ugd->composer_popup) {
			composer_attachment_launch_attachment_error_popup(COMPOSER_ERROR_ATTACHMENT_MAX_SIZE_EXCEEDED, composer_util_popup_response_cb, ugd);
			ret = EINA_FALSE;
		}
		ugd->need_to_display_max_size_popup = EINA_FALSE;
	}

	if (pending_max_attach_popup) {
		ugd->need_to_display_max_size_popup = EINA_TRUE;
	}

	composer_util_modify_send_button(ugd);

	debug_leave();
	return ret;
}

void composer_attachment_process_attachments_with_string(EmailComposerUGD *ugd, const char *path_string, const char *delim, int to_be_moved_or_copied, Eina_Bool is_inline)
{
	debug_enter();

	retm_if(!ugd, "ugd is NULL!");
	retm_if(!path_string, "path_string is NULL!");
	retm_if(!delim, "delim is NULL!");

	/* NOTE: list is freed() after processing adding attachment. Don't remove the list from the caller! */
	Eina_List *list = composer_util_make_string_to_list(path_string, delim);
	_attachment_process_attachments_list(ugd, list, to_be_moved_or_copied, is_inline);

	debug_leave();
}

void composer_attachment_process_attachments_with_array(EmailComposerUGD *ugd, const char **path_array, int path_len, int to_be_moved_or_copied, Eina_Bool is_inline)
{
	debug_enter();

	retm_if(!ugd, "ugd is NULL!");
	retm_if(!path_array, "path_array is NULL!");
	retm_if(path_len <= 0, "Invalid parameter: len is [%d]!", path_len);

	/* NOTE: list is freed() after processing adding attachment. Don't remove the list from the caller! */
	Eina_List *list = composer_util_make_array_to_list(path_array, path_len);
	_attachment_process_attachments_list(ugd, list, to_be_moved_or_copied, is_inline);

	debug_leave();
}

/* NOTE: path_list is freed() after processing adding attachment. Don't remove the list from the caller! */
void composer_attachment_process_attachments_with_list(EmailComposerUGD *ugd, Eina_List *path_list, int to_be_moved_or_copied, Eina_Bool is_inline)
{
	debug_enter();

	retm_if(!ugd, "ugd is NULL!");
	retm_if(!path_list, "path_list is NULL!");

	_attachment_process_attachments_list(ugd, path_list, to_be_moved_or_copied, is_inline);

	debug_leave();
}

void composer_attachment_download_attachment(EmailComposerUGD *ugd, email_attachment_data_t *attachment)
{
	debug_enter();

	debug_secure("id:[%d], name:(%s), path:(%s), size:[%d], save:[%d], mail_id:[%d], account_id:[%d]", attachment->attachment_id, attachment->attachment_name, attachment->attachment_path,
			attachment->attachment_size, attachment->save_status, attachment->mail_id, attachment->account_id);

	int att_index = 0;
	int att_count = 0;
	email_attachment_data_t *att_info = NULL;

	/* To get attachment index. */
	int ret = email_get_attachment_data_list(attachment->mail_id, &att_info, &att_count);
	if (ret == EMAIL_ERROR_NONE) {
		int i = 0;
		for (i = 0; i < att_count; i++) {
			att_index++;
			if (attachment->attachment_id == att_info[i].attachment_id) {
				break;
			}
		}
		ugd->downloading_attachment_index = att_index;

		ret = email_free_attachment_data(&att_info, att_count);
		debug_warning_if(ret != EMAIL_ERROR_NONE, "email_free_attachment_data() failed! It'll cause a memory leak!");

		gboolean ret = email_engine_attachment_download(attachment->mail_id, att_index, &ugd->handle_for_downloading_attachment);
		if (ret) {
			ugd->downloading_attachment = attachment;
			ugd->composer_popup = composer_util_popup_create_with_progress_horizontal(ugd, EMAIL_COMPOSER_STRING_HEADER_DOWNLOAD_ATTACHMENT_HEADER, EMAIL_COMPOSER_STRING_POP_DOWNLOADING_ING,
									_attachment_download_attachment_response_cb, EMAIL_COMPOSER_STRING_BUTTON_CANCEL, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);
		} else {
			ugd->composer_popup = composer_util_popup_create(ugd, EMAIL_COMPOSER_STRING_HEADER_UNABLE_TO_DOWNLOAD_ATTACHMENT_ABB, EMAIL_COMPOSER_STRING_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED,
										composer_util_popup_response_cb, EMAIL_COMPOSER_STRING_BUTTON_OK, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);
		}
	} else {
		debug_error("email_get_attachment_data_list() failed! ret:[%d]", ret);

		ugd->composer_popup = composer_util_popup_create(ugd, EMAIL_COMPOSER_STRING_HEADER_UNABLE_TO_DOWNLOAD_ATTACHMENT_ABB, EMAIL_COMPOSER_STRING_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED,
									composer_util_popup_response_cb, EMAIL_COMPOSER_STRING_BUTTON_OK, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);
	}

	debug_leave();
}

void composer_attachment_download_attachment_clear_flags(void *data)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	ugd->handle_for_downloading_attachment = 0;
	ugd->downloading_attachment_index = 0;
	ugd->downloading_attachment = NULL;

	debug_leave();
}

void composer_attachment_reset_attachment(EmailComposerUGD *ugd)
{
	debug_enter();

	Eina_List *list = NULL;
	ComposerAttachmentItemData *attachment_item_data = NULL;
	email_attachment_data_t *inline_att = NULL;

	if (ugd->attachment_group_layout) {
		elm_box_unpack(ugd->composer_box, ugd->attachment_group_layout);
		evas_object_del(ugd->attachment_group_layout);
		ugd->attachment_group_layout = NULL;
		ugd->attachment_group_icon = NULL;
	}

	EINA_LIST_FOREACH(ugd->attachment_item_list, list, attachment_item_data) {
		if (attachment_item_data) {
			Evas_Object *layout = attachment_item_data->layout;
			if (layout) {
				elm_box_unpack(ugd->composer_box, layout);
				evas_object_del(layout);
			}

			email_attachment_data_t *att = attachment_item_data->attachment_data;
			if (att) {
				debug_secure("attachment_data file name to be removed : %s", att->attachment_path);

				email_free_attachment_data(&att, 1);
			}

			if (attachment_item_data->preview_path) {
				FREE(attachment_item_data->preview_path);
			}
			free(attachment_item_data);
		}
	}

	EINA_LIST_FOREACH(ugd->attachment_inline_item_list, list, inline_att) {
		if (inline_att) {
			debug_secure("attachment_data file name to be removed : %s", inline_att->attachment_path);

			email_free_attachment_data(&inline_att, 1);
		}
	}

	DELETE_LIST_OBJECT(ugd->attachment_item_list);
	DELETE_LIST_OBJECT(ugd->attachment_inline_item_list);

	debug_leave();
}

void composer_attachment_launch_attachment_error_popup(COMPOSER_ERROR_TYPE_E error_type, Evas_Smart_Cb response_cb, void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");
	retm_if(!response_cb, "Invalid parameter: response_cb is NULL!");
	retm_if(error_type == COMPOSER_ERROR_NONE, "Invalid parameter: error_type is (%d)", COMPOSER_ERROR_NONE);

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	if (error_type == COMPOSER_ERROR_ATTACHMENT_MAX_NUMBER_EXCEEDED) {
		composer_util_popup_translate_set_variables(ugd, POPUP_ELEMENT_TYPE_CONTENT, PACKAGE_TYPE_PACKAGE, NULL, EMAIL_COMPOSER_STRING_POP_UNABLE_TO_ATTACH_MAXIMUM_NUMBER_OF_FILES_IS_PD.id, NULL, MAX_ATTACHMENT_ITEM);

		char buf[BUF_LEN_L] = { 0, };
		snprintf(buf, sizeof(buf), email_get_email_string(EMAIL_COMPOSER_STRING_POP_UNABLE_TO_ATTACH_MAXIMUM_NUMBER_OF_FILES_IS_PD.id), MAX_ATTACHMENT_ITEM);
		EmailCommonStringType EMAIL_COMPOSER_NO_TRANSITION = { NULL, buf };
		ugd->composer_popup = composer_util_popup_create(ugd, EMAIL_COMPOSER_STRING_HEADER_UNABLE_TO_ADD_ATTACHMENT_ABB, EMAIL_COMPOSER_NO_TRANSITION,
		                                                response_cb, EMAIL_COMPOSER_STRING_BUTTON_OK, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);
	} else if (error_type == COMPOSER_ERROR_ATTACHMENT_MAX_SIZE_EXCEEDED) {
		char str[BUF_LEN_T] = { 0, };
		snprintf(str, sizeof(str), "%.1f", ugd->account_info->max_sending_size / (float)(1024 * 1024));
		composer_util_popup_translate_set_variables(ugd, POPUP_ELEMENT_TYPE_CONTENT, PACKAGE_TYPE_PACKAGE, NULL, EMAIL_COMPOSER_STRING_POP_THE_MAXIMUM_TOTAL_SIZE_OF_ATTACHMENTS_HPS_MB_HAS_BEEN_EXCEEDED_REMOVE_SOME_FILES_AND_TRY_AGAIN.id, str, 0);

		char buf[BUF_LEN_L] = { 0, };
		snprintf(buf, sizeof(buf), email_get_email_string(EMAIL_COMPOSER_STRING_POP_THE_MAXIMUM_TOTAL_SIZE_OF_ATTACHMENTS_HPS_MB_HAS_BEEN_EXCEEDED_REMOVE_SOME_FILES_AND_TRY_AGAIN.id), str);
		EmailCommonStringType EMAIL_COMPOSER_NO_TRANSITION = { NULL, buf };
		ugd->composer_popup = composer_util_popup_create(ugd, EMAIL_COMPOSER_STRING_HEADER_UNABLE_TO_SEND_EMAIL_ABB, EMAIL_COMPOSER_NO_TRANSITION,
		                                                 response_cb, EMAIL_COMPOSER_STRING_BUTTON_OK, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);
	} else if (error_type == COMPOSER_ERROR_ATTACHMENT_DUPLICATE) {
		ugd->composer_popup = composer_util_popup_create(ugd, EMAIL_COMPOSER_STRING_HEADER_UNABLE_TO_ADD_ATTACHMENT_ABB, EMAIL_COMPOSER_STRING_POP_A_FILE_WITH_THIS_NAME_HAS_ALREADY_BEEN_ATTACHED,
		                                                response_cb, EMAIL_COMPOSER_STRING_BUTTON_OK, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);
	} else {
		ugd->composer_popup = composer_util_popup_create(ugd, EMAIL_COMPOSER_STRING_HEADER_UNABLE_TO_ADD_ATTACHMENT_ABB, EMAIL_COMPOSER_STRING_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED,
		                                                response_cb, EMAIL_COMPOSER_STRING_BUTTON_OK, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);
	}

	debug_leave();
}
