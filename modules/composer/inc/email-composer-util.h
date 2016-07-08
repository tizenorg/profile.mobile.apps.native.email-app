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

#ifndef __EMAIL_COMPOSER_UTIL_H__
#define __EMAIL_COMPOSER_UTIL_H__

#include "email-utils-contacts.h"

/**
 * Definition utils
 */

/**
 * @brief EXIF ORIENTATION enum
 */
enum EXIF_ORIENTATION {
	EXIF_NOT_AVAILABLE = 0,
	EXIF_NORMAL = 1,	/* Landscape (270) */
	EXIF_HFLIP = 2,
	EXIF_ROT_180 = 3,	/* Landscape upsidedown (90) */
	EXIF_VFLIP = 4,
	EXIF_TRANSPOSE = 5,
	EXIF_ROT_90 = 6,	/* Portrait (0) */
	EXIF_TRANSVERSE = 7,
	EXIF_ROT_270 = 8	/* Portrait upsidedown(180) */
};

enum {
	RESIZE_IMAGE_SMALL_SIZE = 30,
	RESIZE_IMAGE_MEDIUM_SIZE = 70,
	RESIZE_IMAGE_ORIGINAL_SIZE = 100
};

#ifdef _DEBUG
#define _USE_PROFILE_
#endif
#ifdef _USE_PROFILE_
#define email_profiling_begin(pfid) \
	struct timeval __prf_1_##pfid; \
	do { \
		gettimeofday(&__prf_1_##pfid, 0); \
		debug_trace("PROFILING [ BEGIN ] "#pfid" -> %u.%06u", (unsigned int)__prf_1_##pfid.tv_sec, (unsigned int)__prf_1_##pfid.tv_usec); \
	} while (0)

#define email_profiling_end(pfid) \
	struct timeval __prf_2_##pfid; \
	do { \
		gettimeofday(&__prf_2_##pfid, 0); \
		long __ds = __prf_2_##pfid.tv_sec - __prf_1_##pfid.tv_sec; \
		long __dm = __prf_2_##pfid.tv_usec - __prf_1_##pfid.tv_usec; \
		if ( __dm < 0 ) { __ds--; __dm = 1000000 + __dm; } \
		debug_trace("PROFILING [Elapsed] "#pfid" -> %u.%06u", (unsigned int)(__ds), (unsigned int)(__dm)); \
	} while (0)
#else
#define email_profiling_begin(pfid)
#define email_profiling_end(pfid)
#endif

/**
 * In email-composer-util-popup.c
 */

/**
 * @brief Create popup composers utility
 *
 * @param[in]	view				Email composer data
 * @param[in]	t_title			Email common string type data
 * @param[in]	t_content		Email common string type data
 * @param[in]	response_cb		This callback function set as a response for all buttons
 * @param[in]	t_btn1_lb		Email common string type data
 * @param[in]	t_btn2_lb		Email common string type data
 * @param[in]	t_btn3_lb		Email common string type data
 *
 * @return Evas_Object instance with suitable popup, otherwise NULL
 */
Evas_Object *composer_util_popup_create(EmailComposerView *view, email_string_t t_title, email_string_t t_content, Evas_Smart_Cb response_cb,
                                        email_string_t t_btn1_lb, email_string_t t_btn2_lb, email_string_t t_btn3_lb);

/**
 * @brief Create horizontal progress popup composers utility
 *
 * @param[in]	view				Email composer data
 * @param[in]	t_title			Email common string type data
 * @param[in]	t_content		Email common string type data
 * @param[in]	response_cb		This callback function set as a response for all buttons
 * @param[in]	t_btn1_lb		Email common string type data
 * @param[in]	t_btn2_lb		Email common string type data
 * @param[in]	t_btn3_lb		Email common string type data
 *
 * @return Evas_Object instance with suitable popup, otherwise NULL
 */
Evas_Object *composer_util_popup_create_with_progress_horizontal(EmailComposerView *view, email_string_t t_title, email_string_t t_content, Evas_Smart_Cb response_cb,
																email_string_t t_btn1_lb, email_string_t t_btn2_lb, email_string_t t_btn3_lb);

/**
 * @brief Callback popup response
 *
 * @param[in]	data			User data (Email composer data)
 * @param[in]	obj				Unused
 * @param[in]	event_info		Unused
 *
 */
void composer_util_popup_response_cb(void *data, Evas_Object *obj, void *event_info);

/**
 * @brief Creates account list popup
 *
 * @param[in]	data»			User data (Email composer data)
 * @param[in]	response_cb		This callback function set as a response for all buttons
 * @param[in]	selected_cb		Evas_Smart callback function data when item in genlist selected
 * @param[in]	style			Style name
 * @param[in]	t_title			Email common string type data for popup title
 * @param[in]	t_btn1_lb		Email common string type data for button lable #1
 * @param[in]	t_btn2_lb		Email common string type data for button lable #2
 *
 */
void composer_util_popup_create_account_list_popup(void *data, Evas_Smart_Cb response_cb, Evas_Smart_Cb selected_cb, const char *style,
													email_string_t t_title, email_string_t t_btn1_lb, email_string_t t_btn2_lb);

/**
 * @brief Provides translation for popup
 *
 * @param[in]	data		User data (Email composer data)
 *
 */
void composer_util_popup_translate_do(void *data);

/**
 * @brief Set translation usage variables
 *
 * @param[in]	data				User data (Email composer data)
 * @param[in]	element_type		Email popup element type enum
 * @param[in]	string_type			Email popup string type enum
 * @param[in]	string_format		String format
 * @param[in]	string_id			String ID name
 * @param[in]	data1				String data #1
 * @param[in]	data2				Integer data #2
 *
 */
void composer_util_popup_translate_set_variables(void *data, EmailPopupElementType element_type, EmailPopupStringType string_type, const char *string_format, const char *string_id, const char *data1, int data2);

/**
 * @brief Free translation usage variables
 *
 * @param[in]	data		User data (Email composer data)
 *
 */
void composer_util_popup_translate_free_variables(void *data);

/**
 * In email-composer-util-image.c
 */

/**
 * @brief Check by mime if image is resizable
 *
 * @param[in]	mime_type		Mime type name
 *
 * @return EINA_TRUE when resizable, otherwise EINA_FALSE or an error occurred
 */
Eina_Bool composer_util_image_is_resizable_image_type(const char *mime_type);

/**
 * @brief Get image size by file path
 *
 * @param[in]	data				User data (Email composer data)
 * @param[in]	src_file			Path to source file
 * @param[out]	width				Integer pointer to width
 * @param[out]	height				Integer pointer to height
 *
 * @return EINA_TRUE on success, otherwise EINA_FALSE or an error occurred
 */
Eina_Bool composer_util_image_get_size(void *data, const char *src_file, int *width, int *height);

/**
 * @brief Set image to Evas_Object from source file with destination width and height
 *
 * @param[in]		data»				User data (Email composer data)
 * @param[in/out]	src_obj»			Image object instance
 * @param[out]		width»				Integer pointer to width
 * @param[out]		height»				Integer pointer to height
 *
 * @return EINA_TRUE on success, otherwise EINA_FALSE or an error occurred
 */
Eina_Bool composer_util_image_set_image_with_size(void *data, Evas_Object **src_obj, const char *src_file, int dest_width, int dest_height);

/**
 * @brief Image scale down with quality
 *
 * @param[in]	src_file		Source file path
 * @param[in]	dst				Destination file path
 * @param[in]	quality			Integer parameter of quality 100, 70, 50, 30, 10
 *
 *@return EINA_TRUE on success, otherwise EINA_FALSE or an error occurred
 */
Eina_Bool composer_util_image_scale_down_with_quality(const char *src, const char *dst, int quality);

/**
 * @brief Get rotation for jpeg image
 *
 * @param[in]	src_file			Source file path
 * @param[out]	rotation			Integer pointer that specified rotation
 *
 * @return EINA_TRUE on success, otherwise EINA_FALSE or an error occurred
 */
Eina_Bool composer_util_image_get_rotation_for_jpeg_image(const char *src_file_path, int *rotation);

/**
 * @brief Rotate jpeg image
 *
 * @param[in]	src_file_path		Source file path
 * @param[in]	dst_file_path		Destination file path
 * @param[in]	rotation			Rotation
 *
 * @return EINA_TRUE on success, otherwise EINA_FALSE or an error occurred
 */
Eina_Bool composer_util_image_rotate_jpeg_image(const char *src_file_path, const char *dst_file_path, int rotation);

/**
 * @brief Rotate jpeg image from memory
 *
 * @param[in]	src_buffer			Source memory buffer
 * @param[in]	width				Width of source image
 * @param[in]	height				Height of source image
 * @param[in]	size				Size of jpeg image
 * @param[in]	orientation			Orientation of jpeg image
 * @param[out]	dst_buffer			Destination memory buffer
 * @param[out]	dst_width			Destination	width
 * @param[out]	height				Destination height
 *
 * @return EINA_TRUE on success, otherwise EINA_FALSE or an error occurred
 */
Eina_Bool composer_util_image_rotate_jpeg_image_from_memory(const unsigned char *src_buffer, int width, int height, unsigned int size, int orientation, unsigned char **dst_buffer, int *dst_width, int *dst_height);

/**
 * In email-composer-util-recipient.c
 */

/**
 * @brief Get ellipsised entry name
 *
 * @param[in]	view					Email composer data
 * @param[in]	display_entry		Entry object instance
 * @param[in]	entry_string		String to be ellipsised
 *
 * @return gchar string on success, otherwise NULL. It should be freed
 */
gchar *composer_util_get_ellipsised_entry_name(EmailComposerView *view, Evas_Object *display_entry, const char *entry_string);

/**
 * @brief Check if email address of recipient is already exist in multibuttonentry
 *
 * @param[in]	view					Email composer data
 * @param[in]	obj					Evas_Object multibuttonentry
 * @param[in]	email_address		Email address name
 *
 * @return EINA_TRUE if already exist, otherwise EINA_FALSE or an error occurred
 */
Eina_Bool composer_util_recp_is_duplicated(EmailComposerView *view, Evas_Object *obj, const char *email_address);

/**
 * @brief Check if multibuttonentry is empty
 *
 * @param[in]	data				User data (Email composer data)
 *
 * @return EINA_TRUE if it's empty, otherwise EINA_FALSE or an error occurred
 */
Eina_Bool composer_util_recp_is_mbe_empty(void *data);

/**
 * @brief Clear recipient multibuttonentry
 *
 * @param[in]	obj				Multibuttonentry instance
 *
 */
void composer_util_recp_clear_mbe(Evas_Object *obj);

/**
 * @brief Create Email recipient info of recipient
 *
 * @param[in]	email_address		Email address name
 *
 * @return Email recipient info data instance with suitable data, otherwise NULL
 */
EmailRecpInfo *composer_util_recp_make_recipient_info(const char *email_address);

/**
 * @brief Create Email recipient info of recipient with preset display name
 *
 * @param[in]	email_address		Email address name
 * @param[in]	display_name		Display name
 *
 * @return Email recipient info data instance with suitable data, otherwise NULL
 */
EmailRecpInfo *composer_util_recp_make_recipient_info_with_display_name(char *email_address, char *display_name);

/**
 * @brief Set recipient display address from multibuttonentry to destination string
 *
 * @param[in]	view		Email composer data
 * @param[in]	obj		Evas _Object multibuttonentry
 * @param[out]	dest	Pointer to destination string
 *
 * @return Composer error type enum
 */
COMPOSER_ERROR_TYPE_E composer_util_recp_retrieve_recp_string(EmailComposerView *view, Evas_Object *obj, char **dest);

/**
 * In email-composer-util.c
 */

/**
 * @brief elm util function to get theme path
 */
const char *email_get_composer_theme_path();

/**
 * @brief elm util function to get tmp dir
 */
const char *email_get_composer_tmp_dir();

/**
 * @brief Get string encoded to datetime format
 *
 * @return The datetime formated string on success, otherwise NULL. It should be freed
 */
char *composer_util_get_datetime_format();

/**
 * @brief Get error string from error type
 *
 * @param[in]	type		Error type that specify error
 *
 * @return The error string on success, otherwise NULL. It should be freed
 */
char *composer_util_get_error_string(int type);

/**
 * @brief Get total attachment size
 *
 * @param[in]	view							Email composer data
 * @param[in]	with_inline_contents		Flag that specify inline content is used
 *
 * @return Attachments files size
 */
int composer_util_get_total_attachments_size(EmailComposerView *view, Eina_Bool with_inline_contents);

/**
 * @brief Get body size
 *
 * @param[in]	mailinfo		Email composer mail data
 *
 * @return Body size
 */
int composer_util_get_body_size(EmailComposerMail *mailinfo);

/**
 * @brief Check if max sending size exceeded
 *
 * @param[in]	data		User data (Email composer data)
 *
 * @return EINA_TRUE if max sending size exceeded, otherwise EINA_FALSE or an error occurred
 */
Eina_Bool composer_util_is_max_sending_size_exceeded(void *data);

/**
 * @brief Modify send button acordinaly to send state
 *
 * @param[in]	view		Email composer data
 *
 */
void composer_util_modify_send_button(EmailComposerView *view);

/**
 * @brief Function to update inline image list
 *
 * @param[in]	view		Email composer data
 * @param[in]	img_srcs	Inline attachment list from webkit
 *
 */
void composer_util_update_inline_image_list(EmailComposerView *view, const char *img_srcs);

/**
 * @brief Composer create layout with noindicator
 *
 * @param[in]	parent		Evas_Object used as parent for creating layout
 *
 * @return Evas_Object instance with suitable nonindcator layout, otherwise NULL
 */
Evas_Object *composer_util_create_layout_with_noindicator(Evas_Object *parent);

/**
 * @brief Create scroller in composer
 *
 * @param[in]	parent		Evas_Object used as parent for creating scroller
 *
 * @return Evas_Object instance with suitable scroller, otherwise NULL
 */
Evas_Object *composer_util_create_scroller(Evas_Object *parent);

/**
 * @brief Composer create box
 *
 * @param[in]	parent		Evas_Object used as parent for creating box
 *
 * @return Evas_Object instance with suitable box, otherwise NULL
 */
Evas_Object *composer_util_create_box(Evas_Object *parent);

/**
 * @brief Composer create entry layout
 *
 * @param[in]	parent		Evas_Object used as parent for creating entry layout
 *
 * @return Evas_Object instance with suitable entry layout, otherwise NULL
 */
Evas_Object *composer_util_create_entry_layout(Evas_Object *parent);

/**
 * @brief Composer set entry layout text style
 *
 * @param[in]	entry		Entry object instance
 *
 */
void composer_util_set_entry_text_style(Evas_Object *entry);

/**
 * @brief Copy source file to precreated temp path
 *
 * @param[in]	src_file_path			Source file path
 * @param[out]	dst_file_path			Destination temp file path
 * @param[in]	size_dst_file_path		Size of destination temp path
 *
 * @return EINA_TRUE if operation succeeded, otherwise EINA_FALSE or an error occurred
 */
Eina_Bool composer_util_file_copy_temp_file(const char *src_file_path, char *dst_file_path, int size_dst_file_path);

/**
 * @brief Create temp filename based on source one and suffix if exist
 *
 * @param[in]	src_file_path			Source file path
 * @param[out]	dst_file_path			Destination temp file path
 * @param[in]	size_dst_file_path		Size of destination temp path
 * @param[in]	suffix					Suffix to be added to temp file
 *
 * @return EINA_TRUE if operation succeeded, otherwise EINA_FALSE or an error occurred
 */
Eina_Bool composer_util_file_get_temp_filename(const char *src_file_path, char *dst_file_path, int size_dst_file_path, const char *suffix);

/**
 * @brief Get default temp dirname
 *
 * @return The default temp dirname string on success, otherwise NULL. It should be freed.
 */
const char *composer_util_file_get_temp_dirname();

/**
 * @brief Converts string array to list
 *
 * @param[in]	path_array			String array
 * @param[out]	path_len			Path length
 *
 * @return Eina_List of strings when operation succeeded, otherwise EINA_FALSE or an error occurred
 */
Eina_List *composer_util_make_array_to_list(const char **path_array, int path_len);

/**
 * @brief Converts string with delimiters to list
 *
 * @param[in]	path_string		String of pathes
 * @param[out]	delim			Delimiter
 *
 * @return Eina_List of strings when operation succeeded, otherwise EINA_FALSE or an error occurred
 */
Eina_List *composer_util_make_string_to_list(const char *path_string, const char *delim);

/**
 * @brief Set focus to Evas_Object target in composer
 *
 * @param[in]	data			User data (Email composer data)
 * @param[out]	target			Evas_Object target to focused
 *
 */
void composer_util_focus_set_focus(void *data, Evas_Object *target);

/**
 * @brief Set focus to Evas_Object target with idler in composer
 *
 * @param[in]	data			User data (Email composer data)
 * @param[out]	target			Evas_Object target to focused
 *
 */
void composer_util_focus_set_focus_with_idler(void *data, Evas_Object *target);

/**
 * @brief Show scroller with selected widget position
 *
 * @param[in]	data			User data (Email composer data)
 *
 */
void composer_util_scroll_region_show(void *data);

/**
 * @brief Show scroller with selected widget position with idler
 *
 * @param[in]	data			User data (Email composer data)
 *
 * @return EINA_TRUE if operation succeeded, otherwise EINA_FALSE or an error occurred
 */
Eina_Bool composer_util_scroll_region_show_idler(void *data);

/**
 * @brief Show scroller with selected widget position with timer
 *
 * @param[in]	data			User data (Email composer data)
 *
 * @return EINA_TRUE if operation succeeded, otherwise EINA_FALSE or an error occurred
 */
Eina_Bool composer_util_scroll_region_show_timer(void *data);

/**
 * @brief Bring scroller with selected widget
 *
 * @param[in]	data			User data (Email composer data)
 *
 */
void composer_util_scroll_region_bringin(void *data);

/**
 * @brief Bring scroller with selected widget with idler
 *
 * @param[in]	data			User data (Email composer data)
 *
 * @return EINA_TRUE if operation succeeded, otherwise EINA_FALSE or an error occurred
 */
Eina_Bool composer_util_scroll_region_bringin_idler(void *data);

/**
 * @brief Bring scroller with selected widget with timer
 *
 * @param[in]	data			User data (Email composer data)
 *
 * @return EINA_TRUE if operation succeeded, otherwise EINA_FALSE or an error occurred
 */
Eina_Bool composer_util_scroll_region_bringin_timer(void *data);

/**
 * @brief Callback for popup that ends with module destroy operation
 *
 * @param[in]	data»			User data (Email composer data)
 * @param[in]	obj				Unused
 * @param[in]	event_info»		Unused
 *
 */
void composer_util_self_destroy_cb(void *data, Evas_Object *obj, void *event_info);

/**
 * @brief Check if module was modified
 *
 * @param[in]	data»			User data (Email composer data)
 *
 * @return EINA_TRUE if it was modified, otherwise EINA_FALSE or an error occurred
 *
 */
Eina_Bool composer_util_is_mail_modified(void *data);

/**
 * @brief Resize composer webview height
 *
 * @param[in]	view			Email composer data
 *
 */
void composer_util_resize_webview_height(EmailComposerView *view);

/**
 * @brief Display geometry for all elements in composer
 *
 * @param[in]	data»			User data (Email composer data)
 *
 */
void composer_util_display_position(void *data);

/**
 * @brief Return control to composer view
 *
 * @param[in]	data»			User data (Email composer data)
 *
 */
void composer_util_return_composer_view(void *data);

/**
 * @brief Sets the indicator mode of the window
 *
 * @param[in]	data»			User data (Email composer data)
 *
 */
void composer_util_indicator_show(void *data);

/**
 * @brief Sets the indicator opacity mode of the window
 *
 * @param[in]	data»			User data (Email composer data)
 *
 */
void composer_util_indicator_restore(void *data);

/**
 * @brief Check if an Evas_Object (second argument) is packed in Box
 *
 * @param[in]	box»			Box object where need to look for required object
 * @param[in]	obj»			The object that need to know if exists(packed) into Box object
 *
 * @return EINA_TRUE if it was packed, otherwise EINA_FALSE or an error occurred
 */
Eina_Bool composer_util_is_object_packed_in(Evas_Object *box, Evas_Object *obj);

/**
 * @brief Replace from email address with new one with markup
 *
 * @param[in]	email_address			Email address name
 *
 * @return The modified string on success, otherwise NULL. It should be freed.
 */
char *composer_util_strip_quotation_marks_in_email_address(const char *email_address);

/**
 * @brief Show attachment preview
 *
 * @param[in]	attach_item_data			Composer attachment item data
 *
 */
void composer_util_show_preview(ComposerAttachmentItemData *attach_item_data);

/**
 * @brief Network state notification
 *
 */
void composer_util_network_state_noti_post();

/**
 * @brief Update attach panel bundles
 *
 * @param[in]	view			Email composer data
 *
 */
void composer_util_update_attach_panel_bundles(EmailComposerView *view);

/**
 * @brief Creates popup for creating vcard
 *
 * @param[in]	view			Email composer data
 *
 */
void composer_create_vcard_create_popup(EmailComposerView *view);

#endif /* __EMAIL_COMPOSER_UTIL_H__ */
