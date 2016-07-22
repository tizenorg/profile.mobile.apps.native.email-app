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

#include <image_util.h>
#include <metadata_extractor.h>

#include "email-debug.h"
#include "email-composer.h"
#include "email-composer-types.h"
#include "email-composer-util.h"
#include "email-composer-attachment.h"
#include "email-composer-launcher.h"
#include "email-media-content.h"

#define THUMB_SIZE 80

/*
 * Declaration for static functions
 */

static char *_attachment_thumbnail_create_thumbnail_for_image(EmailComposerView *view, const char *source);
static char *_attachment_thumbnail_create_thumbnail_for_video(const char *source);
static char *_attachment_thumbnail_create_thumbnail_for_audio(const char *source);
static Evas_Object *_attachment_thumbnail_create_thumbnail(EmailComposerView *view, const char *filepath, const char *filename, Evas_Object *parent);

static void _attachment_ui_item_delete_button_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static void _attachment_ui_item_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _attachment_ui_item_update_fileinfo(email_attachment_data_t *attachment_data, Evas_Object *layout);
static void _attachment_ui_item_update_thumbnail(ComposerAttachmentItemData *attachment_item_data);
static void _attachment_ui_item_create_delete_button(ComposerAttachmentItemData *attachment_item_data, Evas_Object *parent);
static void _attachment_ui_item_create_layout(ComposerAttachmentItemData *attachment_item_data, Evas_Object *parent);

static void _attachment_ui_group_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _attachment_ui_group_update_icon(EmailComposerView *view);
static void _attachment_ui_group_create_layout(EmailComposerView *view, Evas_Object *parent);

static email_string_t EMAIL_COMPOSER_STRING_NULL = { NULL, NULL };
static email_string_t EMAIL_COMPOSER_STRING_BUTTON_OK = { PACKAGE, "IDS_EMAIL_BUTTON_OK" };

static email_string_t EMAIL_COMPOSER_STRING_UNABLE_TO_OPEN_FILE = { PACKAGE, "IDS_EMAIL_HEADER_UNABLE_TO_OPEN_FILE_ABB" };
static email_string_t EMAIL_COMPOSER_STRING_UNABLE_TO_DISPLAY_ATTACHMENT = { NULL, N_("Unable to display attachment.") };

/*
 * Definition for static functions
 */

static char *_attachment_thumbnail_create_thumbnail_for_image(EmailComposerView *view, const char *source)
{
	debug_enter();

	retvm_if(!source, NULL, "source is NULL!");

	char *thumb_path = NULL;

	if (view->is_load_finished || (view->composer_type == RUN_COMPOSER_EXTERNAL)) {
		/* for images, we will get thumbnail from media-content db */
		email_media_content_get_image_thumbnail(source, &thumb_path);
	}

	if (!thumb_path) {
		thumb_path = g_strdup(source);
	}

	debug_leave();
	return thumb_path;
}

static char *_attachment_thumbnail_create_thumbnail_for_video(const char *source)
{
	debug_enter();

	retvm_if(!source, NULL, "source is NULL!");

	metadata_extractor_h metadata = NULL;
	char *thumb_path = NULL;
	unsigned char *dst = NULL;
	void *rawdata = NULL;
	char *value = NULL;
	int rawdata_len = 0;
	int width = 0;
	int height = 0;
	int orient = 0;
	int ret = METADATA_EXTRACTOR_ERROR_NONE;

	email_media_content_get_image_thumbnail(source, &thumb_path);
	if (!thumb_path) {
		ret = metadata_extractor_create(&metadata);
		retvm_if(ret != METADATA_EXTRACTOR_ERROR_NONE, NULL, "metadata_extractor_create() failed! ret:[%d]", ret);

		ret = metadata_extractor_set_path(metadata, source);
		gotom_if(ret != METADATA_EXTRACTOR_ERROR_NONE, EXIT_FUNC, "metadata_extractor_set_path() failed! ret:[%d]", ret);

		ret = metadata_extractor_get_metadata(metadata, METADATA_VIDEO_WIDTH, &value);
		gotom_if(ret != METADATA_EXTRACTOR_ERROR_NONE, EXIT_FUNC, "metadata_extractor_get_metadata() failed! ret:[%d]", ret);

		if (value) {
			width = atoi(value);
			free(value);
			value = NULL;
		}

		ret = metadata_extractor_get_metadata(metadata, METADATA_VIDEO_HEIGHT, &value);
		gotom_if(ret != METADATA_EXTRACTOR_ERROR_NONE, EXIT_FUNC, "metadata_extractor_get_metadata() failed! ret:[%d]", ret);

		if (value) {
			height = atoi(value);
			free(value);
			value = NULL;
		}

		ret = metadata_extractor_get_metadata(metadata, METADATA_ROTATE, &value);
		gotom_if(ret != METADATA_EXTRACTOR_ERROR_NONE, EXIT_FUNC, "metadata_extractor_get_metadata() failed! ret:[%d]", ret);

		if (value) {
			orient = atoi(value);
			free(value);
			value = NULL;
		}

		ret = metadata_extractor_get_frame(metadata, &rawdata, &rawdata_len);
		gotom_if(ret != METADATA_EXTRACTOR_ERROR_NONE, EXIT_FUNC, "metadata_extractor_get_frame() failed! ret:[%d]", ret);

		if (rawdata && rawdata_len > 0) {
			char dst_filepath[EMAIL_FILEPATH_MAX] = { 0, };
			char filepath[EMAIL_FILEPATH_MAX] = { 0, };
			composer_util_file_get_temp_filename(source, dst_filepath, EMAIL_FILEPATH_MAX, NULL);

			char *filename = (char *)email_file_file_get(dst_filepath);
			if (filename && (strlen(filename) >= (EMAIL_FILENAME_MAX - 5))) {
				filename[EMAIL_FILENAME_MAX - 5] = '\0'; /* To append extension */
			}
			snprintf(filepath, EMAIL_FILEPATH_MAX, "%s.jpg", dst_filepath);

			if (orient) {
				int dst_width = 0;
				int dst_height = 0;

				/*if (orient == 90) {
					rot = IMAGE_UTIL_ROTATION_90;
				} else if (orient == 180) {
					rot = IMAGE_UTIL_ROTATION_180;
				} else if (orient == 270) {
					rot = IMAGE_UTIL_ROTATION_270;
				}*/

				dst = (unsigned char *)calloc(1, rawdata_len);
				gotom_if(!dst, EXIT_FUNC, "Failed to allocate memory for dst!");
				ret = image_util_encode_jpeg(dst, dst_width, dst_height, IMAGE_UTIL_COLORSPACE_RGB888, 95, filepath);
				gotom_if(ret != IMAGE_UTIL_ERROR_NONE, EXIT_FUNC, "image_util_encode_jpeg() failed! ret:[%d]", ret);
			} else {
				ret = image_util_encode_jpeg((const unsigned char *)rawdata, width, height, IMAGE_UTIL_COLORSPACE_RGB888, 95, filepath);
				gotom_if(ret != IMAGE_UTIL_ERROR_NONE, EXIT_FUNC, "image_util_encode_jpeg() failed! ret:[%d]", ret);
			}

			if (email_file_exists(filepath)) {
				thumb_path = g_strdup(filepath);
			}
		}
	}

EXIT_FUNC:
	g_free(rawdata);
	g_free(dst);

	if (metadata) {
		ret = metadata_extractor_destroy(metadata);
		debug_warning_if(ret != METADATA_EXTRACTOR_ERROR_NONE, "metadata_extractor_destroy() failed! ret:[%d]", ret);
	}

	debug_leave();
	return thumb_path;
}

static char *_attachment_thumbnail_create_thumbnail_for_audio(const char *source)
{
	debug_enter();

	retvm_if(!source, NULL, "source is NULL!");

	metadata_extractor_h metadata = NULL;
	char *thumb_path = NULL;
	void *artwork = NULL;
	char *artwork_mime = NULL;
	int artwork_size = 0;
	int ret = METADATA_EXTRACTOR_ERROR_NONE;

	email_media_content_get_image_thumbnail(source, &thumb_path);
	if (!thumb_path) {
		ret = metadata_extractor_create(&metadata);
		retvm_if(ret != METADATA_EXTRACTOR_ERROR_NONE, NULL, "metadata_extractor_create() failed! ret:[%d]", ret);

		ret = metadata_extractor_set_path(metadata, source);
		gotom_if(ret != METADATA_EXTRACTOR_ERROR_NONE, EXIT_FUNC, "metadata_extractor_set_path() failed! ret:[%d]", ret);

		ret = metadata_extractor_get_artwork(metadata, &artwork, &artwork_size, &artwork_mime);
		gotom_if(ret != METADATA_EXTRACTOR_ERROR_NONE, EXIT_FUNC, "metadata_extractor_get_artwork() failed! ret:[%d]", ret);

		debug_secure("artwork_mime: (%s), artwork_size: (%d)", artwork_mime, artwork_size);

		if (artwork && artwork_size > 0) {
			char dst_filepath[EMAIL_FILEPATH_MAX] = { 0, };
			char filepath[EMAIL_FILEPATH_MAX] = { 0, };
			composer_util_file_get_temp_filename(source, dst_filepath, EMAIL_FILEPATH_MAX, NULL);

			char *filename = (char *)email_file_file_get(dst_filepath);
			if (filename && (strlen(filename) >= (EMAIL_FILENAME_MAX - 5))) {
				filename[EMAIL_FILENAME_MAX - 5] = '\0'; /* To append extension */
			}
			snprintf(filepath, EMAIL_FILEPATH_MAX, "%s.jpg", dst_filepath);

			gboolean result = email_save_file(filepath, (char *)artwork, artwork_size);
			if (result && email_file_exists(filepath)) {
				thumb_path = g_strdup(filepath);
			}
		}
	}

EXIT_FUNC:
	g_free(artwork);
	g_free(artwork_mime);

	if (metadata) {
		ret = metadata_extractor_destroy(metadata);
		debug_warning_if(ret != METADATA_EXTRACTOR_ERROR_NONE, "metadata_extractor_destroy() failed! ret:[%d]", ret);
	}

	debug_leave();
	return thumb_path;
}

static Evas_Object *_attachment_thumbnail_create_thumbnail(EmailComposerView *view, const char *filepath, const char *filename, Evas_Object *parent)
{
	debug_enter();

	char *mime_type = NULL;
	char *thumb_path = NULL;
	email_file_type_e ftype = EMAIL_FILE_TYPE_ETC;
	Evas_Object *icon = evas_object_image_filled_add(evas_object_evas_get(parent));

	debug_secure("filename: (%s)", filename);

	mime_type = email_get_mime_type_from_file(filename);
	if (mime_type) {
		debug_secure("mime_type: (%s)", mime_type);
		ftype = email_get_file_type_from_mime_type(mime_type);

		free(mime_type);
	}

	if (filepath && email_file_exists(filepath)) {
		debug_secure("filepath: (%s)", filepath);

		if (ftype == EMAIL_FILE_TYPE_IMAGE) {
			thumb_path = _attachment_thumbnail_create_thumbnail_for_image(view, filepath);
		} else if (ftype == EMAIL_FILE_TYPE_VIDEO) {
			thumb_path = _attachment_thumbnail_create_thumbnail_for_video(filepath);
		} else if (ftype == EMAIL_FILE_TYPE_MUSIC) {
			thumb_path = _attachment_thumbnail_create_thumbnail_for_audio(filepath);
		}
	}

	if (thumb_path && strlen(thumb_path)) {
		evas_object_image_load_size_set(icon, THUMB_SIZE, THUMB_SIZE);
		evas_object_image_load_orientation_set(icon, EINA_TRUE);
		evas_object_image_file_set(icon, thumb_path, NULL);

		int err = evas_object_image_load_error_get(icon);
		if (err != EVAS_LOAD_ERROR_NONE) {
			debug_warning("evas_object_image_file_set() failed! error string is (%s)", evas_load_error_str(err));
			evas_object_image_file_set(icon, email_get_icon_path_from_file_type(ftype), NULL);
		} else {
			if (ftype == EMAIL_FILE_TYPE_VIDEO) {
				elm_layout_signal_emit(parent, "ec,state,video_play_image,show", "");
			}
		}
		g_free(thumb_path);
	} else {
		evas_object_image_file_set(icon, email_get_icon_path_from_file_type(ftype), NULL);
	}

	debug_leave();
	return icon;
}

static void _attachment_ui_item_delete_button_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "data is NULL!");
	retm_if(!obj, "data is NULL!");

	ComposerAttachmentItemData *attachment_item_data = (ComposerAttachmentItemData *)data;
	EmailComposerView *view = (EmailComposerView *)attachment_item_data->view;
	Evas_Object *attachment_layout = attachment_item_data->layout;
	email_attachment_data_t *attachment_data = attachment_item_data->attachment_data;

	view->attachment_item_list = eina_list_remove(view->attachment_item_list, attachment_item_data);

	if (attachment_data) {
		debug_secure("attachment filename: (%s)", attachment_data->attachment_path);

		char *att_dirname = email_file_dir_get(attachment_data->attachment_path);
		if (!g_strcmp0(composer_util_file_get_temp_dirname(), att_dirname)) {
			debug_log("File is in temp directory! It should be removed!");
			Eina_Bool ret = email_file_unlink(attachment_data->attachment_path);
			if (!ret)
				debug_warning("email_file_unlink() failed!");
		}
		FREE(att_dirname);

		email_engine_free_attachment_data_list(&attachment_data, 1);
	}

	if (attachment_layout) {
		elm_box_unpack(view->composer_box, attachment_layout);
		evas_object_del(attachment_layout);
	}
	FREE(attachment_item_data);

	if (view->attachment_group_layout) {
		/* Group layout is removed when the number of attachment is reduced to 1 from 2. In other cases, update group layout text. */
		if (eina_list_count(view->attachment_item_list) == 1) {
			elm_box_unpack(view->composer_box, view->attachment_group_layout);
			DELETE_EVAS_OBJECT(view->attachment_group_layout);
			view->attachment_group_layout = NULL;
			view->attachment_group_icon = NULL;
			view->is_attachment_list_expanded = EINA_TRUE;
		} else {
			composer_attachment_ui_group_update_text(view);
		}
	}

	/* Reset need_to_download flag. Don't need to launch forward download popup if there's no attachment needed to download. */
	Eina_Bool is_needed_to_download = EINA_FALSE;
	Eina_List *list = NULL;
	ComposerAttachmentItemData *attach_item_data = NULL;
	EINA_LIST_FOREACH(view->attachment_item_list, list, attach_item_data) {
		email_attachment_data_t *att = attach_item_data->attachment_data;
		if ((att->attachment_id > 0) && (att->save_status == 0)) {
			is_needed_to_download = EINA_TRUE;
			break;
		}
	}
	view->need_download = is_needed_to_download;

	composer_util_modify_send_button(view);

	debug_leave();
}

static void _attachment_ui_item_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	ComposerAttachmentItemData *attach_item_data = (ComposerAttachmentItemData *)data;
	EmailComposerView *view = (EmailComposerView *)attach_item_data->view;
	email_attachment_data_t *attachment = attach_item_data->attachment_data;

	if (!view->allow_click_events) {
		debug_log("Click was blocked.");
		return;
	}

	/* When a user clicks attachment layout multiple times quickly, app launch requested is called multiple times. */
	ret_if(view->base.module->is_launcher_busy);

	email_feedback_play_tap_sound();

	if (!attachment->save_status) {
		if (view->account_info->account_type == EMAIL_SERVER_TYPE_POP3 && view->org_mail_info && view->org_mail_info->mail_data && (view->org_mail_info->mail_data->body_download_status != EMAIL_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED)) {
			view->composer_popup = composer_util_popup_create(view, EMAIL_COMPOSER_STRING_UNABLE_TO_OPEN_FILE, EMAIL_COMPOSER_STRING_UNABLE_TO_DISPLAY_ATTACHMENT,
									composer_util_popup_response_cb, EMAIL_COMPOSER_STRING_BUTTON_OK, EMAIL_COMPOSER_STRING_NULL, EMAIL_COMPOSER_STRING_NULL);
		} else {
			composer_attachment_download_attachment(view, attachment);
		}
	} else {
		composer_util_show_preview(attach_item_data);
	}


	debug_leave();
}

static void _attachment_ui_item_update_fileinfo(email_attachment_data_t *attachment_data, Evas_Object *layout)
{
	debug_enter();

	char file_size_info[EMAIL_BUFF_SIZE_SML] = { 0, };
	char *file_size = (char *)email_get_file_size_string((guint64)attachment_data->attachment_size);
	snprintf(file_size_info, sizeof(file_size_info), "(%s)", file_size);

	elm_object_part_text_set(layout, "ec.text.main.left", attachment_data->attachment_name);
	elm_object_part_text_set(layout, "ec.text.sub.right", file_size_info);

	g_free(file_size);

	debug_leave();
}

static void _attachment_ui_item_update_thumbnail(ComposerAttachmentItemData *attachment_item_data)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)attachment_item_data->view;
	email_attachment_data_t *attachment_data = attachment_item_data->attachment_data;

	Evas_Object *thumbnail = _attachment_thumbnail_create_thumbnail(view, attachment_data->attachment_path, attachment_data->attachment_name, attachment_item_data->layout);
	elm_object_part_content_set(attachment_item_data->layout, "ec.swallow.icon", thumbnail);

	debug_leave();
}

static void _attachment_ui_item_create_delete_button(ComposerAttachmentItemData *attachment_item_data, Evas_Object *parent)
{
	debug_enter();

	Evas_Object *delete_button = elm_button_add(parent);
	elm_object_style_set(delete_button, "icon_expand_delete");
	elm_object_tree_focus_allow_set(delete_button, EINA_FALSE);
	evas_object_repeat_events_set(delete_button, EINA_FALSE);
	evas_object_smart_callback_add(delete_button, "clicked", _attachment_ui_item_delete_button_clicked_cb, attachment_item_data);
	evas_object_show(delete_button);

	elm_object_part_content_set(parent, "ec.swallow.button", delete_button);

	debug_leave();
}

static void _attachment_ui_item_create_layout(ComposerAttachmentItemData *attachment_item_data, Evas_Object *parent)
{
	debug_enter();

	Evas_Object *layout = elm_layout_add(parent);
	elm_layout_file_set(layout, email_get_composer_theme_path(), "ec/attachment/base");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(layout);
	elm_layout_signal_callback_add(layout, "ec,action,clicked", "", _attachment_ui_item_clicked_cb, attachment_item_data);
	EmailComposerView *view = (EmailComposerView *)attachment_item_data->view;
	if (view->richtext_toolbar) {
		elm_box_pack_before(parent, layout, view->richtext_toolbar);
	} else {
		elm_box_pack_end(parent, layout);
	}



	attachment_item_data->layout = layout;

	debug_leave();
}

static void _attachment_ui_group_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	email_feedback_play_tap_sound();

	EmailComposerView *view = (EmailComposerView *)data;

	if (view->is_attachment_list_expanded) {
		composer_attachment_ui_contract_attachment_list(view);
	} else {
		composer_attachment_ui_expand_attachment_list(view);
	}

	debug_leave();
}

static void _attachment_ui_group_update_icon(EmailComposerView *view)
{
	debug_enter();

	if (view->is_attachment_list_expanded) {
		elm_layout_file_set(view->attachment_group_icon, email_get_common_theme_path(), EMAIL_IMAGE_COMPOSER_EXPAND_OPEN);
	} else {
		elm_layout_file_set(view->attachment_group_icon, email_get_common_theme_path(), EMAIL_IMAGE_COMPOSER_EXPAND_CLOSE);
	}
	debug_leave();
}

static void _attachment_ui_group_create_layout(EmailComposerView *view, Evas_Object *parent)
{
	debug_enter();

	Evas_Object *layout = elm_layout_add(parent);
	elm_layout_file_set(layout, email_get_composer_theme_path(), "ec/attachment/group_item");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(layout);

	Evas_Object *group_icon = elm_layout_add(layout);
	elm_layout_file_set(group_icon, email_get_common_theme_path(), EMAIL_IMAGE_COMPOSER_EXPAND_OPEN);
	evas_object_show(group_icon);

	elm_layout_signal_callback_add(layout, "ec,action,clicked", "", _attachment_ui_group_clicked_cb, view);
	elm_object_part_content_set(layout, "ec.swallow.icon", group_icon);
	elm_object_tree_focus_allow_set(layout, EINA_FALSE);

	elm_box_pack_after(parent, layout, view->subject_layout);

	view->attachment_group_icon = group_icon;
	view->attachment_group_layout = layout;

	debug_leave();
}


/*
 * Definition for exported functions
 */

void composer_attachment_ui_create_item_layout(EmailComposerView *view, email_attachment_data_t *attachment_data, Eina_Bool is_group_needed, Eina_Bool is_from_user)
{
	debug_enter();

	retm_if(!view, "view is NULL!");
	retm_if(!attachment_data, "attachment_data is NULL!");
	retm_if(attachment_data->inline_content_status, "attachment_data->inline_content_status is TRUE!");

	if (is_group_needed && !view->attachment_group_layout) {
		_attachment_ui_group_create_layout(view, view->composer_box);
		_attachment_ui_group_update_icon(view);
	}

	if (!view->is_attachment_list_expanded) {
		composer_attachment_ui_expand_attachment_list(view);
	}

	ComposerAttachmentItemData *attachment_item_data = NULL;
	attachment_item_data = calloc(1, sizeof(ComposerAttachmentItemData));
	retm_if(!attachment_item_data, "attachment_item_data is NULL!");
	attachment_item_data->view = (void *)view;
	attachment_item_data->attachment_data = attachment_data;
	attachment_item_data->preview_path = NULL;
	attachment_item_data->from_user = is_from_user;

	_attachment_ui_item_create_layout(attachment_item_data, view->composer_box);
	_attachment_ui_item_create_delete_button(attachment_item_data, attachment_item_data->layout);
	composer_attachment_ui_update_item(attachment_item_data);
	view->attachment_item_list = eina_list_append(view->attachment_item_list, attachment_item_data);

	if (view->attachment_group_layout) {
		composer_attachment_ui_group_update_text(view);
	}

	debug_leave();
}

void composer_attachment_ui_update_item(ComposerAttachmentItemData *attachment_item_data)
{
	debug_enter();

	retm_if(!attachment_item_data, "attachment_item_data is NULL!");

	_attachment_ui_item_update_fileinfo(attachment_item_data->attachment_data, attachment_item_data->layout);
	_attachment_ui_item_update_thumbnail(attachment_item_data);

	debug_leave();
}

void composer_attachment_ui_group_update_text(EmailComposerView *view)
{
	debug_enter();

	char summary_string[EMAIL_BUFF_SIZE_BIG] = { 0, };
	int size = composer_util_get_total_attachments_size(view, EINA_FALSE);
	char *file_size = (char *)email_get_file_size_string((guint64)size);

	snprintf(summary_string, sizeof(summary_string), email_get_email_string("IDS_EMAIL_BODY_PD_FILES_ABB"), eina_list_count(view->attachment_item_list));

	if ((view->account_info->selected_account->outgoing_server_size_limit == 0) || (view->account_info->selected_account->outgoing_server_size_limit == -1)) {
		snprintf(summary_string + strlen(summary_string), sizeof(summary_string) - strlen(summary_string), " (%s)", file_size);
	} else {
		char *max_size = (char *)email_get_file_size_string((guint64)view->account_info->max_sending_size);
		if (max_size) {
			snprintf(summary_string + strlen(summary_string), sizeof(summary_string) - strlen(summary_string), " (%s/%s)", file_size, max_size);
			free(max_size);
		} else {
			snprintf(summary_string + strlen(summary_string), sizeof(summary_string) - strlen(summary_string), " (%s)", file_size);
		}
	}

	elm_object_part_text_set(view->attachment_group_layout, "ec.text.main", summary_string);

	FREE(file_size);

	debug_leave();
}

void composer_attachment_ui_contract_attachment_list(EmailComposerView *view)
{
	debug_enter();

	retm_if(!view, "view is NULL!");

	if (view->attachment_group_layout) {
		Eina_List *l = NULL;
		ComposerAttachmentItemData *attachment_item_data = NULL;
		EINA_LIST_FOREACH(view->attachment_item_list, l, attachment_item_data) {
			elm_box_unpack(view->composer_box, attachment_item_data->layout);
			evas_object_hide(attachment_item_data->layout);
		}
		view->is_attachment_list_expanded = EINA_FALSE;
		_attachment_ui_group_update_icon(view);
	}

	debug_leave();
}

void composer_attachment_ui_expand_attachment_list(EmailComposerView *view)
{
	debug_enter();

	retm_if(!view, "view is NULL!");

	Eina_List *l = NULL;
	ComposerAttachmentItemData *attachment_item_data = NULL;
	EINA_LIST_FOREACH(view->attachment_item_list, l, attachment_item_data) {
		evas_object_show(attachment_item_data->layout);
		if (view->richtext_toolbar) {
			elm_box_pack_before(view->composer_box, attachment_item_data->layout, view->richtext_toolbar);
		} else {
			elm_box_pack_end(view->composer_box, attachment_item_data->layout);
		}

	}
	view->is_attachment_list_expanded = EINA_TRUE;
	_attachment_ui_group_update_icon(view);

	debug_leave();
}
