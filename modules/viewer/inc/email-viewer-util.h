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

#ifndef __EMAIL_VIEWER_UTIL_H__
#define __EMAIL_VIEWER_UTIL_H__

#include <glib.h>
#include <glib/gprintf.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <Elementary.h>

#include "email-viewer-types.h"

typedef void (*popup_cb) (void *data, Evas_Object *obj, void *event_info);

/**
 * @brief elm util function
 */
const char *email_get_viewer_theme_path();

/**
 * @brief elm util function
 */
const char *email_get_viewer_tmp_dir();

/**
 * @brief Create notification genlist
 *
 * @param[in]	parent				Parent object for genlist
 *
 * @return Evas_Object with suitable genlist, otherwise NULL
 */
Evas_Object *util_notify_genlist_add(Evas_Object *parent);

/**
 * @brief Creates notification popup list and returns object itself
 *
 * @param[in]	view				Email viewer data
 * @param[in]	t_header			Email viewer string data for header
 * @param[in]	t_content			Email viewer string data for content
 * @param[in]	btn_num				Count of buttons in list
 * @param[in]	t_btn1_lb			Email viewer string data for button label #1
 * @param[in]	resp_cb1			Response callback function for button #1
 * @param[in]	t_btn2_lb			Email viewer string data for button label #2
 * @param[in]	resp_cb2			Response callback function for button #2
 * @param[in]	resp_block_cb		Response callback function for Blocked Event area
 *
 * @return Evas_Object with suitable popup, otherwise NULL
 */
Evas_Object *util_create_notify_with_list(EmailViewerView *view, email_string_t t_header, email_string_t t_content,
						int btn_num, email_string_t t_btn1_lb, popup_cb resp_cb1,
						email_string_t t_btn2_lb, popup_cb resp_cb2, popup_cb resp_block_cb);

/**
 * @brief Creates and show notification popup list
 *
 * @param[in]	view				Email viewer data
 * @param[in]	t_header			Email viewer string data for header
 * @param[in]	t_content			Email viewer string data for content
 * @param[in]	btn_num				Count of buttons in list
 * @param[in]	t_btn1_lb			Email viewer string data for button label #1
 * @param[in]	resp_cb1			Response callback function for button #1
 * @param[in]	t_btn2_lb			Email viewer string data for button label #2
 * @param[in]	resp_cb2			Response callback function for button #2
 * @param[in]	resp_block_cb		Response callback function for Blocked Event area
 *
 */
void util_create_notify(EmailViewerView *view, email_string_t t_header, email_string_t t_content,
						int btn_num, email_string_t t_btn1_lb, popup_cb resp_cb1,
						email_string_t t_btn2_lb, popup_cb resp_cb2, popup_cb resp_block_cb);

/**
 * @brief Find folder ID using folder type
 *
 * @param[in]	view				Email viewer data
 * @param[in]	mailbox_type		Email mailbox type enum
 *
 * @return 0 on success, otherwise a negative error value
 */
int _find_folder_id_using_folder_type(EmailViewerView *view, email_mailbox_type_e mailbox_type);

/**
 * @brief Move email to destination folder that represented by folder ID
 *
 * @param[in]	view				Email viewer data
 * @param[in]	dest_folder_id		Folder ID
 * @param[in]	is_delete			To specify which popop to show in case of failure
 * @param[in]	is_block			To specify which popop to show in case of failure
 *
 * @return 0 on success, otherwise a negative error value
 */
int viewer_move_email(EmailViewerView *view, int dest_folder_id, gboolean is_delete, gboolean is_block);

/**
 * @brief Delete an email
 *
 * @param[in]	view				Email viewer data
 *
 * @return 0 on success, otherwise a negative error value
 */
int viewer_delete_email(EmailViewerView *view);

/**
 * @brief Create child module that defined by Email Module type
 *
 * @param[in]	data			User data (Email viewer data)
 * @param[in]	module_type		Email Module type
 * @param[in]	params			Emal params handle to create child module
 * @param[in]	hide			If set hide webview
 *
 * @return Handler with suitable Email module, otherwise NULL
 */
email_module_h viewer_create_module(void *data, email_module_type_e module_type, email_params_h params, bool hide);

/**
 * @brief Create Composer module
 *
 * @param[in]	data			User data (Email viewer data)
 * @param[in]	params			Emal params handle to create child module
 *
 * @return Handler with Composer Email module, otherwise NULL
 */
email_module_h viewer_create_scheduled_composer_module(void *data, email_params_h params);

/**
 * @brief Create Composer module
 *
 * @param[in]	parent			Parent object
 * @param[in]	file			Path to file
 * @param[in]	group			Group of collection to be used
 * @param[in]	hintx			Size hint weight for axis X
 * @param[in]	hinty			Size hint weight for axis Y
 *
 * @return Evas_Object with suitable layout, otherwise NULL
 */
Evas_Object *viewer_load_edj(Evas_Object *parent, const char *file, const char *group, double hintx, double hinty);

/**
 * @brief Launch browser by specifying URI operation
 *
 * @param[in]	view				Email viewer data
 * @param[in]	link_uri			URI to set operation
 *
 */
void viewer_launch_browser(EmailViewerView *view, const char *link_uri);

/**
 * @brief Create email
 *
 * @param[in]	view				Email viewer data
 * @param[in]	email_address		Email address name
 *
 */
void viewer_create_email(EmailViewerView *view, const char *email_address);

/**
 * @brief Save attached file to Downloads folder with progress notification
 *
 * @param[in]	aid				Viewer attachment data
 * @param[in]	copy_file_cb	Callback function to perform file copy
 *
 * @return Error code from EmailExtSaveErrType enum
 */
email_ext_save_err_type_e viewer_save_attachment_in_downloads(EV_attachment_data *aid, gboolean(*copy_file_cb) (void *data, float percentage));

/**
 * @brief Save attached file and preview with progress notofication
 *
 * @param[in]	aid				Viewer attachment data
 * @param[in]	copy_file_cb	Callback function to perform file copy
 *
 * @return Error code from EmailExtSaveErrType enum
 */
email_ext_save_err_type_e viewer_save_attachment_for_preview(EV_attachment_data *aid, gboolean(*copy_file_cb) (void *data, float percentage));

/**
 * @brief Preview attachment
 *
 * @param[in]	view				Email viewer data
 *
 */
void viewer_show_attachment_preview(EV_attachment_data *aid);

/**
 * @brief Check if storage is full
 *
 * @param[in]	mb_size				How much space is required in Mb
 *
 * @return True on success, otherwise False if an error or not enough memory on storage
 */
int viewer_check_storage_full(unsigned int mb_size);

/**
 * @brief Provides notification popup storage is full
 *
 * @param[in]	view				Email viewer data
 *
 */
void viewer_show_storage_full_popup(EmailViewerView *view);

/**
 * @brief Callback performed after full storage detected
 *
 * @param[in]	data			User data (Email viewer data)
 * @param[in]	obj				Unused
 * @param[in]	event_info		Unused
 *
 */
void viewer_storage_full_popup_response_cb(void *data, Evas_Object *obj, void *event_info);

/**
 * @brief Create popup downloading with progressbar
 *
 * @param[in]	data			User data (Email viewer data)
 * @param[in]	title			Email viewer string data
 * @param[in]	resp_cb			Callback function for back button response
 *
 */
void viewer_create_down_progress(void *data, email_string_t title, popup_cb resp_cb);

/**
 * @brief Destroy popup downloading callback
 *
 * @param[in]	data			User data (Email viewer data)
 * @param[in]	obj				Unused
 * @param[in]	event_info		Unused
 *
 */
void viewer_destroy_down_progress_cb(void *data, Evas_Object *obj, void *event_info);

/**
 * @brief Run Composer, called from mbe selection
 *
 * @param[in]	data			User data (Email viewer data)
 * @param[in]	email_address	Email addres name that will be passed as recipient addres in Composer
 *
 */
void viewer_send_email(void *data, char *email_address);

/**
 * @brief View contact detail by calling Contacts app with operation view
 *
 * @param[in]	data			User data (Email viewer data)
 * @param[in]	index			Index of contact that should be displayed
 *
 */
void viewer_view_contact_detail(void *data, int index);

/**
 * @brief Run Composer, called from mbe selection
 *
 * @param[in]	data		User data (Email viewer data)
 * @param[in]	type		Run type key
 *
 */
void viewer_launch_composer(void *data, int type);


/**
 * @brief Recursively remove folder
 *
 * @param[in]	path		Folder path that should be removed
 *
 */
void viewer_remove_folder(char *path);

/**
 * @brief Escaping special characters from given filename and replacing them with '_' character
 *
 * @param[in]	filename		The filename that should be converted
 *
 * @return The converted string. It should be freed.
 */
char *viewer_escape_filename(const char *filename);

/**
 * @brief Make string encoded to datetime format
 *
 * @return The datetime formated string. It should be freed.
 */
char *viewer_get_datetime_string(void);

/**
 * @brief Make file path performed from 1)dir name 2)filename 3)sufix
 *
 * @param[in]	dir			Dir name
 * @param[in]	name		Filename
 * @param[in]	suffix		Sufix for filename
 *
 * @return The file path string. It should be freed.
 */
char *viewer_make_filepath(const char *dir, const char *name, const char *suffix);

/**
 * @brief Make check for body download status by current status of downloading and that should be
 *
 * @param[in]	body_download_status		Current status of downloading
 * @param[in]	status						Desired status
 *
 * @return EINA_TRUE when desired status the same as current, otherwise EINA_FALSE or an error occurred
 */
Eina_Bool viewer_check_body_download_status(int body_download_status, int status);

/**
 * @brief Create body button
 *
 * @param[in]	view			Email viewer data
 * @param[in]	button_title	Button lable
 * @param[in]	cb				Callback function for button response
 *
 */
void viewer_create_body_button(EmailViewerView *view, const char *button_title, Evas_Smart_Cb cb);

/**
 * @brief Delete body button
 *
 * @param[in]	view			Email viewer data
 *
 */
void viewer_delete_body_button(EmailViewerView *view);

/**
 * @brief Temporary folder /tmp/email is created when viewer is launched
 *
 * @param[in]	view			Email viewer data
 *
 * @return 0 on success, otherwise a negative error value
 */
int viewer_create_temp_folder(EmailViewerView *view);

/**
 * @brief Temporary folder '/tmp/email' and its contents are deleted when composer is destroyed
 *
 * @param[in]	view			Email viewer data
 *
 */
void viewer_remove_temp_folders(EmailViewerView *view);

/**
 * @brief Delete files from temp viewer html file path
 *
 * @param[in]	view			Email viewer data
 *
 */
void viewer_remove_temp_files(EmailViewerView *view);

#endif	/* __EMAIL_VIEWER_UTIL_H__ */
/* EOF */
