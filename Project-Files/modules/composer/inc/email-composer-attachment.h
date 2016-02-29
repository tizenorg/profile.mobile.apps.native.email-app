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

#ifndef __EMAIL_COMPOSER_ATTACHMENT_H__
#define __EMAIL_COMPOSER_ATTACHMENT_H__

/**
 * @brief Specifyed to attached file to be moved or copied
 */
typedef enum {
	ATTACH_BY_USE_ORG_FILE = 0,		/**< attach with using original file */
	ATTACH_BY_MOVE_FILE = 1,		/**< attach with moving file */
	ATTACH_BY_COPY_FILE = 2,		/**< attach with copying file */
} FILE_TO_BE_ATTACHED;

/* email-composer-attachment.c */

/**
 * @brief Creates attachment list
 * @param[in]	view»				Email composer data
 * @param[in]	attachment_list»	Attachment List
 * @param[in]	is_inline	»		is inline (body content attachment), Eina_Bool value
 * @param[in]	more_list_exists»	is more list exist, Eina_Bool value
 * @return EINA_TRUE on success or EINA_FALSE if none or an error occurred
 */
Eina_Bool composer_attachment_create_list(EmailComposerView *view, Eina_List *attachment_list, Eina_Bool is_inline, Eina_Bool more_list_exists);

/**
 * @brief Provides making list process of attachment files through specifying path to files with delimiters
 * @remark Files in string have to be separated with delimiters
 * @param[in]	view»					Email composer data
 * @param[in]	path_string»			Path with files
 * @param[in]	delim	»				Delimiter
 * @param[in]	to_be_moved_or_copied»	Is file be moved or copied, enum
 * @param[in]	is_inline	»			is inline (body content attachment), Eina_Bool value
 */
void composer_attachment_process_attachments_with_string(EmailComposerView *view, const char *path_string, const char *delim, int to_be_moved_or_copied, Eina_Bool is_inline);

/**
 * @brief Provides making list process of attachment files through specifying path array to files with count of files
 * @param[in]	view»					Email composer data
 * @param[in]	path_array»				Path array
 * @param[in]	path_len	»			Count of files
 * @param[in]	to_be_moved_or_copied»	Is file be moved or copied, enum
 * @param[in]	is_inline	»			is inline (body content attachment), Eina_Bool value
 */
void composer_attachment_process_attachments_with_array(EmailComposerView *view, const char **path_array, int path_len, int to_be_moved_or_copied, Eina_Bool is_inline);

/**
 * @brief Provides making list process of attachment files through specifying list of files
 * @param[in]	view»					Email composer data
 * @param[in]	path_list»				List of pathes
 * @param[in]	to_be_moved_or_copied»	Is file be moved or copied, enum
 * @param[in]	is_inline	»			is inline (body content attachment), Eina_Bool value
 */
void composer_attachment_process_attachments_with_list(EmailComposerView *view, Eina_List *path_list, int to_be_moved_or_copied, Eina_Bool is_inline);

/**
 * @brief Download attachments
 * @param[in]	view»			Email composer data
 * @param[in]	attachment»		Email attachment data
 */
void composer_attachment_download_attachment(EmailComposerView *view, email_attachment_data_t *attachment);

/**
 * @brief Clear download attachments flags
 * @param[in]	data»			User data (Email composer data)
 */
void composer_attachment_download_attachment_clear_flags(void *data);

/**
 * @brief Reset attachment list
 * @param[in]	data»			User data (Email composer data)
 */
void composer_attachment_reset_attachment(EmailComposerView *view);

/**
 * @brief Reset attachment list
 * @param[in]	error_type»		Error type
 * @param[in]	response_cb»	Response callback function
 * @param[in]	data»			User data (Email composer data)
 */
void composer_attachment_launch_attachment_error_popup(COMPOSER_ERROR_TYPE_E error_type, Evas_Smart_Cb response_cb, void *data);

/* email-composer-attachment-ui.c */

/**
 * @brief Creates item layout for attachment
 * @param[in]	view»					Email composer data
 * @param[in]	attachment_data»		Email attachment data
 * @param[in]	is_group_needed»		Is group needed, Eina_Bool value
 * @param[in]	is_from_user»			Is from user, Eina_Bool value
 */
void composer_attachment_ui_create_item_layout(EmailComposerView *view, email_attachment_data_t *attachment_data, Eina_Bool is_group_needed, Eina_Bool is_from_user);

/**
 * @brief Updates attachment item element
 * @param[in]	attachment_item_data»		Composer attachment item data
 */
void composer_attachment_ui_update_item(ComposerAttachmentItemData *attachment_item_data);

/**
 * @brief Updates text for group attachment
 * @param[in]	view»			Email composer data
 */
void composer_attachment_ui_group_update_text(EmailComposerView *view);

/**
 * @brief Contract attachment list
 * @param[in]	view»			Email composer data
 */
void composer_attachment_ui_contract_attachment_list(EmailComposerView *view);

/**
 * @brief Expand attachment list
 * @param[in]	view»			Email composer data
 */
void composer_attachment_ui_expand_attachment_list(EmailComposerView *view);

/* email-composer-attachment-resize-image.c */

/**
 * @brief Creates popup for resize images selection
 * @param[in]	view»				Email composer data
 * @param[in]	attachment_list»	Attachment list of images
 */
void composer_attachment_resize_image(EmailComposerView *view, Eina_List *attachment_list);

#endif /* __EMAIL_COMPOSER_ATTACHMENT_H__ */
