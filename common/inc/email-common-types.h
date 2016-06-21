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

#ifndef _EMAIL_COMMON_TYPES_H_
#define _EMAIL_COMMON_TYPES_H_

#include <email-types.h>
#include <Elementary.h>

#include "email-common-contact-defines.h"
#include "email-images.h"
#include "email-colors.h"

#ifndef EMAIL_API
#ifdef SHARED_MODULES_FEATURE
#define EMAIL_API __attribute__ ((visibility("default")))
#else
#define EMAIL_API
#endif
#endif


#define EMAIL_FILEPATH_MAX		4096 	// PATH_MAX in limits.h
#define EMAIL_FILENAME_MAX		255 	// NAME_MAX in limits.h
#define EMAIL_SMALL_PATH_MAX	128
#define MAX_STR_LEN				1024
#define MAX_PATH_LEN			1024
#define MAX_URL_LEN				1024
#define MAX_RECPT_LEN			(MAX_STR_LEN * 8 + 1)
#define MIN_FREE_SPACE			(5) /* MB */
#define MAX_ACCOUNT_COUNT		10

#define EMAIL_BUFF_SIZE_TIN 16
#define EMAIL_BUFF_SIZE_SML 32
#define EMAIL_BUFF_SIZE_MID 64
#define EMAIL_BUFF_SIZE_BIG 128
#define EMAIL_BUFF_SIZE_HUG 256
#define EMAIL_BUFF_SIZE_1K 1024
#define EMAIL_BUFF_SIZE_4K 4096

/* define bundle key */
#define EMAIL_BUNDLE_KEY_ACCOUNT_TYPE				"ACCOUNT_TYPE"
#define EMAIL_BUNDLE_KEY_ACCOUNT_ID					"ACCOUNT_ID"
#define EMAIL_BUNDLE_KEY_MAILBOX					"MAILBOX_ID"
#define EMAIL_BUNDLE_KEY_IS_MAILBOX_MOVE_MODE			"IS_MAILBOX_MOVE_MODE"
#define EMAIL_BUNDLE_KEY_IS_MAILBOX_ACCOUNT_MODE		"IS_MAILBOX_ACCOUNT_MODE"
#define EMAIL_BUNDLE_KEY_MAILBOX_MOVE_MODE			"MAILBOX_MOVE_MODE"
#define EMAIL_BUNDLE_KEY_MAILBOX_MOVE_STATUS		"MAILBOX_MOVE_STATUS"
#define EMAIL_BUNDLE_KEY_MAILBOX_MOVED_MAILBOX_NAME	"MAILBOX_MOVED_MAILBOX_NAME"
#define EMAIL_BUNDLE_KEY_IS_MAILBOX_EDIT_MODE		"MAILBOX_EDIT_MODE"
#define EMAIL_BUNDLE_KEY_MOVE_SRC_MAILBOX_ID		"MOVE_SRC_MAILBOX_ID"
#define EMAIL_BUNDLE_KEY_ARRAY_SELECTED_MAIL_IDS	"SELECTED_MAIL_IDS"
#define EMAIL_BUNDLE_KEY_VIEW_TYPE				"VIEW_TYPE"
#define EMAIL_BUNDLE_KEY_RUN_TYPE				"RUN_TYPE"
#define EMAIL_BUNDLE_KEY_MAIL_ID				"MAIL_ID"
//TODO This bundle gets in Viewer module but nobody set it. Can be removed after Viewer module refactoring
#define EMAIL_BUNDLE_KEY_FIRST_LANDSCAPE							"bFirstLandscape"
#define EMAIL_BUNDLE_KEY_MSG										"MSG"
#define EMAIL_BUNDLE_KEY_TO											"TO"
#define EMAIL_BUNDLE_KEY_CC											"CC"
#define EMAIL_BUNDLE_KEY_BCC										"BCC"
#define EMAIL_BUNDLE_KEY_SUBJECT									"SUBJECT"
#define EMAIL_BUNDLE_KEY_BODY										"BODY"
#define EMAIL_BUNDLE_KEY_ATTACHMENT									"ATTACHMENT"
#define EMAIL_BUNDLE_KEY_IS_ATTACHMENT_INCLUDE_TO_FORWARD				"IS_ATTACHMENT_INCLUDE_TO_REPLY"
#define EMAIL_BUNDLE_KEY_MAILBOX_TYPE								"MAILBOX_TYPE"
#define EMAIL_BUNDLE_KEY_MAIL_INDEX									"MAIL_INDEX"

#define EMAIL_BUNDLE_KEY_MYFILE_PATH			"path"

#define EMAIL_BUNDLE_KEY_FILTER_OPERATION		"filter_op"
#define EMAIL_BUNDLE_KEY_FILTER_MODE			"filter_mode"
//TODO This bundle gets in Filter module but it never set. It can be removed by Filter refactoring
#define EMAIL_BUNDLE_KEY_FILTER_ADDR			"filter_addr"

/* define bundle value */
#define EMAIL_BUNDLE_VAL_ALL_ACCOUNT				"ALL_ACCOUNT"
#define EMAIL_BUNDLE_VAL_SINGLE_ACCOUNT				"SINGLE_ACCOUNT"
#define EMAIL_BUNDLE_VAL_PRIORITY_SENDER			"PRIORITY_SENDER"
#define EMAIL_BUNDLE_VAL_FILTER_INBOX				"FILTER_INBOX"

#define EMAIL_BUNDLE_VAL_NEXT_MSG					"NEXT_MSG"
#define EMAIL_BUNDLE_VAL_PREV_MSG					"PREV_MSG"
#define EMAIL_BUNDLE_VAL_EMAIL_COMPOSER_SAVE_DRAFT	"SAVE_DRAFT"

#define EMAIL_BUNDLE_VAL_VIEW_SETTING				"VIEW_SETTING"
#define EMAIL_BUNDLE_VAL_VIEW_ACCOUNT_SETUP			"VIEW_ACCOUNT_SETUP"

#define EMAIL_BUNDLE_VAL_VIEWER_RESTORE_VIEW		"VIEWER_RESTORE_VIEW"
#define EMAIL_BUNDLE_VAL_VIEWER_DESTROY_VIEW		"VIEWER_DESTROY_VIEW"

//TODO This bundle checks in Filter module but it never set. It can be removed by Filter refactoring
#define EMAIL_BUNDLE_VAL_FILTER_OPERATION_BLOCK		"filter_op_block"
#define EMAIL_BUNDLE_VAL_FILTER_OPERATION_PS		"filter_op_priority_sender"

#define EMAIL_BUNDLE_VAL_FILTER_ADD		"filter_add"
#define EMAIL_BUNDLE_VAL_FILTER_LIST	"filter_list"

#define EMAIL_URI_NOTIFICATION_SETTING 	"tizen-email://org.tizen.email/email_setting_notification"
#define EMAIL_URI_SIGNATURE_SETTING 	"tizen-email://org.tizen.email/email_setting_signature"

/* preference key */
#define EMAIL_PREFERENCE_KEY_VIP_RULE_CHANGED			"vip_rule_changed"
#define EMAIL_PREFERENCE_KEY_RICHTEXT_TOOLBAR_ENABLED	"richtext_toolbar_enabled"

// defines for s/mime attachment mime type
#define EMAIL_MIME_TYPE_ENCRYPTED		"application/pkcs7-mime"
#define EMAIL_MIME_TYPE_SIGNED			"application/pkcs7-signature"
#define EMAIL_MIME_TYPE_ENCRYPTED_X		"application/x-pkcs7-mime"
#define EMAIL_MIME_TYPE_SIGNED_X		"application/x-pkcs7-signature"

// boundary value defines
#define EMAIL_LIMIT_ACCOUNT_NAME_LENGTH 256
#define EMAIL_LIMIT_DISPLAY_NAME_LENGTH 256
#define EMAIL_LIMIT_USER_NAME_LENGTH 320
#define EMAIL_LIMIT_EMAIL_ADDRESS_LOCAL_PART_LENGTH 64
#define EMAIL_LIMIT_EMAIL_ADDRESS_DOMAIN_PART_LENGTH 255
#define EMAIL_LIMIT_EMAIL_ADDRESS_LENGTH 320 // local-part@domain-part = 64 + 1 + 255 + 1(null) = 321 // https://tools.ietf.org/html/rfc5321#section-4.5.3.1.1
#define EMAIL_LIMIT_PASSWORD_LENGTH 256
#define EMAIL_LIMIT_INCOMING_SERVER_LENGTH 256
#define EMAIL_LIMIT_INCOMING_PORT_LENGTH 5
#define EMAIL_LIMIT_OUTGOING_SERVER_LENGTH 256
#define EMAIL_LIMIT_OUTGOING_PORT_LENGTH 5
#define EMAIL_LIMIT_SIGNATURE_LENGTH 4096
#define EMAIL_LIMIT_COMPOSER_SUBJECT_LENGTH 1024

#define GDBUS_SIGNAL_SUBSCRIBE_FAILURE -1

typedef enum {
	RUN_TYPE_UNKNOWN = -1,
	RUN_COMPOSER_NEW,	/*< Specific new type. this type is used to create new email */
	RUN_COMPOSER_EDIT,	/*< Specific edit type. this type is used to open draft email */
	RUN_COMPOSER_REPLY,	/*< Specific reply type. this type is used to reply email */
	RUN_COMPOSER_REPLY_ALL,	/*< Specific reply all type. this type is used to replay email to all */
	RUN_COMPOSER_FORWARD,	/*< Specific forward type. this type is used to forward email */
	// DO NOT MODIFY ABOVE TYPES
	RUN_COMPOSER_EXTERNAL = 7,	/*< Specific external type. this type is used to create new email from external app except email app */
	RUN_VIEWER = 8,
	RUN_SETTING_ADD_ACCOUNT_FROM_COMPOSER = 9,
	RUN_SETTING_ACCOUNT_ADD = 10,
	RUN_COMPOSER_SCHEDULED_EDIT = 11,	/*< Specific edit type. this type is used to open scheduled email */
	RUN_VIEWER_FROM_NOTIFICATION = 12,
	RUN_MAILBOX_FROM_NOTIFICATION = 13,
	RUN_EML_REPLY = 14,
	RUN_EML_FORWARD = 15,
	RUN_EML_REPLY_ALL = 16,
	RUN_COMPOSER_FORWARD_ALL = 17,
	RUN_TYPE_MAX
} email_run_type_e;

typedef enum {
	EMAIL_STATUS_REPLY = (1 << 3),
	EMAIL_STATUS_FORWARD = (1 << 7)
} email_mail_status_type_e;

typedef enum {
	EMAIL_SORT_NONE,
	EMAIL_SORT_DATE_RECENT,
	EMAIL_SORT_DATE_OLDEST,
	EMAIL_SORT_SENDER_ATOZ,
	EMAIL_SORT_SENDER_ZTOA,
	EMAIL_SORT_RCPT_ATOZ,
	EMAIL_SORT_RCPT_ZTOA,
	EMAIL_SORT_UNREAD,
	EMAIL_SORT_IMPORTANT,
	EMAIL_SORT_PRIORITY,
	EMAIL_SORT_ATTACHMENTS,
	EMAIL_SORT_SUBJECT_ATOZ,
	EMAIL_SORT_SUBJECT_ZTOA,
	EMAIL_SORT_TO_CC_BCC,
	EMAIL_SORT_SIZE_SMALLEST,
	EMAIL_SORT_SIZE_LARGEST,
	EMAIL_SORT_MAX,
} email_sort_type_e;

typedef enum {
	EMAIL_GET_MAIL_THREAD = -1,
	EMAIL_GET_MAIL_NORMAL = 0,
} email_get_mail_type_e;

typedef enum {
	EMAIL_EXT_SAVE_ERR_NONE = 0,
	EMAIL_EXT_SAVE_ERR_ALREADY_EXIST,
	EMAIL_EXT_SAVE_ERR_NO_FREE_SPACE,
	EMAIL_EXT_SAVE_ERR_UNKNOWN
} email_ext_save_err_type_e;

typedef enum
{
	EMAIL_MOVE_VIEW_NORMAL = 0,
} email_move_view_mode_e;

typedef struct {
	gchar *name;
	gchar *alias;
	email_mailbox_type_e mailbox_type;
	int mailbox_id;
	int unread_count;
	int total_mail_count_on_local;
	int total_mail_count_on_server;
} email_mailbox_name_and_alias_t;

typedef struct {
	int show_images;
	int pop3_deleting_option;
} account_user_data_t;

typedef enum {
	EMAIL_SEARCH_NONE,
	EMAIL_SEARCH_IN_SINGLE_FOLDER,
	EMAIL_SEARCH_IN_ALL_FOLDERS,
	EMAIL_SEARCH_ON_SERVER,
} email_search_type_e;

typedef struct {
	char *subject;
	char *sender;
	char *recipient;
	char *body_text;
	char *attach_text;
	int priority;
	int read_status;
	int flag_type;
	time_t from_time;
	time_t to_time;
} email_search_data_t;

typedef enum {
	EMAIL_SEARCH_CONTACT_MIN = 0,
	EMAIL_SEARCH_CONTACT_BY_NAME = EMAIL_SEARCH_CONTACT_MIN,
	EMAIL_SEARCH_CONTACT_BY_EMAIL,
	EMAIL_SEARCH_CONTACT_MAX
} email_contact_search_type_e;

typedef enum {
	EMAIL_SEARCH_CONTACT_ORIGIN_MIN = 0,
	EMAIL_SEARCH_CONTACT_ORIGIN_CONTACTS = EMAIL_SEARCH_CONTACT_ORIGIN_MIN,
	EMAIL_SEARCH_CONTACT_ORIGIN_RECENT,
	EMAIL_SEARCH_CONTACT_ORIGIN_MAX
} email_contact_search_origin_type_e;

typedef enum {
	EF_MULTILINE = 1,
	EF_CLEAR_BTN = 2,
	EF_PASSWORD = 4,
	EF_TITLE_SEARCH = 8,
} email_editfield_type_e;

typedef struct {
	Evas_Object *layout;
	Evas_Object *entry;
} email_editfield_t;

typedef struct {
	const char *domain;
	const char *id;
} email_string_t;

typedef struct {
	int red;
	int green;
	int blue;
	int alpha;
} email_rgba_t;

#endif	/* _EMAIL_COMMON_TYPES_H_ */

/* EOF */
