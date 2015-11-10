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
#define EMAIL_API __attribute__ ((visibility("default")))
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
#define EMAIL_ALL_FILTER_BOX	-1

/* define operation type */
#define EMAIL_OPERATION_OPEN_NEW_ACCOUNT			"http://tizen.org/email/operation/open_new_account"
#define EMAIL_ACCOUNT_SUPPORTS_CAPABILITY_EMAIL		"http://tizen.org/account/capability/email"

/* define bundle key */
#define EMAIL_BUNDLE_KEY_NEW_ACCOUNT_ID				"NEW_ACCOUNT_ID"
#define EMAIL_BUNDLE_KEY_ACCOUNT_TYPE				"ACCOUNT_TYPE"
#define EMAIL_BUNDLE_KEY_ACCOUNT_ID					"ACCOUNT_ID"
#define EMAIL_BUNDLE_KEY_MAILBOX					"MAILBOX_ID"
#define EMAIL_BUNDLE_KEY_IS_MAILBOX_MOVE_UG			"IS_MAILBOX_MOVE_UG"
#define EMAIL_BUNDLE_KEY_IS_MAILBOX_ACCOUNT_UG		"IS_MAILBOX_ACCOUNT_UG"
#define EMAIL_BUNDLE_KEY_MAILBOX_MOVE_MODE			"MAILBOX_MOVE_MODE"
#define EMAIL_BUNDLE_KEY_MAILBOX_MOVE_STATUS		"MAILBOX_MOVE_STATUS"
#define EMAIL_BUNDLE_KEY_MAILBOX_MOVED_MAILBOX_NAME	"MAILBOX_MOVED_MAILBOX_NAME"
#define EMAIL_BUNDLE_KEY_IS_MAILBOX_EDIT_MODE		"MAILBOX_EDIT_MODE"
#define EMAIL_BUNDLE_KEY_MOVE_SRC_MAILBOX_ID		"MOVE_SRC_MAILBOX_ID"
#define EMAIL_BUNDLE_KEY_ARRAY_SELECTED_MAIL_IDS	"SELECTED_MAIL_IDS"
#define EMAIL_BUNDLE_KEY_ACCOUNT_COLOR				"ACCOUNT_COLOR"
#define EMAIL_BUNDLE_KEY_ATTACHFILE				"ATTACHFILE"
#define EMAIL_BUNDLE_KEY_VIEW_TYPE				"VIEW_TYPE"
#define EMAIL_BUNDLE_KEY_RUN_TYPE				"RUN_TYPE"
#define EMAIL_BUNDLE_KEY_MAIL_ID				"MAIL_ID"
#define EMAIL_BUNDLE_KEY_FIRST_LANDSCAPE		"bFirstLandscape"
#define EMAIL_BUNDLE_KEY_MSG					"MSG"
#define EMAIL_BUNDLE_KEY_LAUNCHER				"LAUNCHER"
#define EMAIL_BUNDLE_KEY_TO						"TO"
#define EMAIL_BUNDLE_KEY_CC						"CC"
#define EMAIL_BUNDLE_KEY_BCC					"BCC"
#define EMAIL_BUNDLE_KEY_SUBJECT				"SUBJECT"
#define EMAIL_BUNDLE_KEY_BODY					"BODY"
#define EMAIL_BUNDLE_KEY_ATTACHMENT				"ATTACHMENT"
#define EMAIL_BUNDLE_KEY_ESP_NAME				"ESP_NAME"
#define EMAIL_BUNDLE_KEY_REFRESH_ACCOUNT		"REFRESH_ACCOUNT"
#define EMAIL_BUNDLE_KEY_MAILBOX_TYPE			"MAILBOX_TYPE"
#define EMAIL_BUNDLE_KEY_FILTER_ID				"FILTER_ID"
#define EMAIL_BUNDLE_KEY_CALENDAR_ID			"CALENDAR_ID"
#define EMAIL_BUNDLE_KEY_ONLY_THIS_OCCURENCE	"ONLY_THIS_OCCURRENCE"
#define EMAIL_BUNDLE_KEY_INSTANCE_ID_DATE		"INSTANCE_ID_DATE"
#define EMAIL_BUNDLE_KEY_MAIL_INDEX				"MAIL_INDEX"

#define EMAIL_BUNDLE_KEY_CONTACT_TYPE				"type"
#define EMAIL_BUNDLE_KEY_MYFILE_PATH				"path"
#define EMAIL_BUNDLE_KEY_MYFILE_SELECT_TYPE			"select_type"
#define EMAIL_BUNDLE_KEY_MYFILE_FILE_TYPE			"file_type"
#define EMAIL_BUNDLE_KEY_GALLERY_LAUNCH_TYPE		"launch-type"
#define EMAIL_BUNDLE_KEY_GALLERY_FILE_TYPE			"file-type"
#define EMAIL_BUNDLE_KEY_VOICE_REC_CALLER			"CALLER"
#define EMAIL_BUNDLE_KEY_VOICE_REC_SIZE				"SIZE"
#define EMAIL_BUNDLE_KEY_VOICE_REC_QUALITY			"QUALITY"
#define EMAIL_BUNDLE_KEY_CAMERA_CALLER				"CALLER"
#define EMAIL_BUNDLE_KEY_CAMERA_RESOLUTION			"RESOLUTION"
#define EMAIL_BUNDLE_KEY_CALENDAR_MODE				"mode"
#define EMAIL_BUNDLE_KEY_CALENDAR_MAX				"max"
#define EMAIL_BUNDLE_KEY_IMAGE_VIEWER_SAVE_FILE		"SaveFile"
#define EMAIL_BUNDLE_KEY_IMAGE_VIEWER_VIEW_MODE		"View Mode"
#define EMAIL_BUNDLE_KEY_IMAGE_VIEWER_SETAS_TYPE	"Setas type"
#define EMAIL_BUNDLE_KEY_IMAGE_VIEWER_PATH			"Path"
#define EMAIL_BUNDLE_KEY_CALENDAR_EDIT_ACCOUNT_ID	"account_id"
#define EMAIL_BUNDLE_KEY_CALENDAR_EDIT_CALENDAR_ID	"calendar_id"
#define EMAIL_BUNDLE_KEY_CALENDAR_EDIT_CREATE_MODE	"create_mode"
#define EMAIL_BUNDLE_KEY_CALENDAR_EDIT_MAIL_ID		"mail_id"
#define EMAIL_BUNDLE_KEY_CALENDAR_EDIT_MAILBOX_ID	"mailbox_id"
#define EMAIL_BUNDLE_KEY_CALENDAR_EDIT_INDEX		"index"
#define EMAIL_BUNDLE_KEY_CALENDAR_EDIT_NOTE			"note"
#define EMAIL_BUNDLE_KEY_MESSAGE_TO					"TO"
#define EMAIL_BUNDLE_KEY_CAMERA_LIMIT				"LIMIT"
#define EMAIL_BUNDLE_KEY_VIDEO_PLAYER_LAUNCH_APP	"launching_application"
#define EMAIL_BUNDLE_KEY_SELECTED_CERT				"selected-cert"
#define EMAIL_BUNDLE_KEY_MISC_WORK_TYPE				"email_misc_work_type"
#define EMAIL_BUNDLE_KEY_TTS_ACCOUNT_ID				"tts_email_account_id"
#define EMAIL_BUNDLE_KEY_TTS_MAIL_ID				"tts_email_id"
#define EMAIL_BUNDLE_KEY_SCHEDULED_TIME				"scheduled_time"
#define EMAIL_BUNDLE_KEY_MISC_DELAY_SENDING_TYPE	"delay_sending_type"
#define EMAIL_BUNDLE_KEY_MISC_POPUP_STRING			"popup_string"
#define EMAIL_BUNDLE_KEY_FILTER_OPERATION			"filter_op"
#define EMAIL_BUNDLE_KEY_FILTER_MODE				"filter_mode"
#define EMAIL_BUNDLE_KEY_FILTER_SUBJECT				"filter_subject"
#define EMAIL_BUNDLE_KEY_FILTER_ADDR				"filter_addr"

/* define ug name */

#define UG_NAME_MESSAGE_COMPOSER	"msg-composer-efl"
#define UG_NAME_MAP					"maps-lite-ug"

#define APP_NAME_EMAIL	"org.tizen.email"

/* define bundle value */
#define EMAIL_BUNDLE_VAL_ALL_ACCOUNT			"ALL_ACCOUNT"
#define EMAIL_BUNDLE_VAL_SINGLE_ACCOUNT			"SINGLE_ACCOUNT"
#define EMAIL_BUNDLE_VAL_PRIORITY_SENDER		"PRIORITY_SENDER"
#define EMAIL_BUNDLE_VAL_SCHEDULED_OUTBOX		"SCHEDULED_OUTBOX"
#define EMAIL_BUNDLE_VAL_FILTER_INBOX			"FILTER_INBOX"
#define EMAIL_BUNDLE_VAL_LAUNCH_MAILBOX			"LAUNCH_MAILBOX"

#define EMAIL_BUNDLE_VAL_SORTBY_UNREAD			"SORTBY_UNREAD"
#define EMAIL_BUNDLE_VAL_SORTBY_FAVOURITES		"SORTBY_FAVOURITES"
#define EMAIL_BUNDLE_VAL_SORTBY_ATTACH			"SORTBY_ATTACH"
#define EMAIL_BUNDLE_VAL_SORTBY_PRIORITY		"SORTBY_PRIORITY"
#define EMAIL_BUNDLE_VAL_LAUNCH_VIEWER			"LAUNCH_VIEWER"

#define EMAIL_BUNDLE_VAL_NEXT_MSG				"NEXT_MSG"
#define EMAIL_BUNDLE_VAL_PREV_MSG				"PREV_MSG"
#define EMAIL_BUNDLE_VAL_INBOX					"INBOX"
#define EMAIL_BUNDLE_VAL_EMAIL_COMPOSER_SAVE_DRAFT	"SAVE_DRAFT"

#define EMAIL_BUNDLE_VAL_VIEW_SETTING			"VIEW_SETTING"
#define EMAIL_BUNDLE_VAL_VIEW_ACCOUNT_SETUP		"VIEW_ACCOUNT_SETUP"

#define EMAIL_BUNDLE_VAL_VIEWER_RESTORE_VIEW			"VIEWER_RESTORE_VIEW"
#define EMAIL_BUNDLE_VAL_VIEWER_DESTROY_VIEW			"VIEWER_DESTROY_VIEW"

#define EMAIL_BUNDLE_VAL_MYFILE_RESULT			"result"
#define EMAIL_BUNDLE_VAL_CONTACTS_PERSON_LIST	"person_id_list"
#define EMAIL_BUNDLE_VAL_CONTACTS_SHARING_OP	"contact_sharing_settings"

#define EMAIL_BUNDLE_VAL_FILTER_OPERATION_FILTER	"filter_op_filter"
#define EMAIL_BUNDLE_VAL_FILTER_OPERATION_BLOCK		"filter_op_block"
#define EMAIL_BUNDLE_VAL_FILTER_OPERATION_PS		"filter_op_priority_sender"

#define EMAIL_BUNDLE_VAL_FILTER_ADD					"filter_add"
#define EMAIL_BUNDLE_VAL_FILTER_LIST				"filter_list"

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
#define EMAIL_LIMIT_TEXT_TEMPLATE_LENGTH 256
#define EMAIL_LIMIT_FILTER_NAME_LENGTH 256

typedef enum
{
	EMAIL_FONT_SIZE_DEFAULT = 0,/**<Global device font size */
	EMAIL_FONT_SIZE_SMALL, /**< A small size */
	EMAIL_FONT_SIZE_MEDIUM, /**< A medium size */
	EMAIL_FONT_SIZE_LARGE, /**< A large size */
	EMAIL_FONT_SIZE_EXTRA_LARGE, /**< A extra large size */
	EMAIL_FONT_SIZE_HUGE, /**< A huge size */
	EMAIL_FONT_SIZE_MAX
} EmailFontSizeType;

typedef enum {
	EMAIL_FONT_SIZE_VALUE_SMALL = 24,	//36,
	EMAIL_FONT_SIZE_VALUE_MEDIUM = 35,	//44,
	EMAIL_FONT_SIZE_VALUE_LARGE = 55,	//64,
	EMAIL_FONT_SIZE_VALUE_EXTRA_LARGE = 75,	//81,
	EMAIL_FONT_SIZE_VALUE_HUGE = 92,	//98,
	EMAIL_FONT_SIZE_VALUE_MAX
} EmailFontSizeValue;

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
	RUN_TYPE_MAX
} EmailRunType;

typedef enum {
	EMAIL_STATUS_REPLY = (1 << 3),
	EMAIL_STATUS_FORWARD = (1 << 7)
} EmailMailStatusType;

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
} EmailSortType;

typedef enum {
	EMAIL_GET_MAIL_THREAD = -1,
	EMAIL_GET_MAIL_NORMAL = 0,
} EmailGetMailType;

typedef enum {
	EMAIL_EXT_SAVE_ERR_NONE = 0,
	EMAIL_EXT_SAVE_ERR_ALREADY_EXIST,
	EMAIL_EXT_SAVE_ERR_NO_FREE_SPACE,
	EMAIL_EXT_SAVE_ERR_UNKNOWN
} EmailExtSaveErrType;

typedef enum
{
	EMAIL_MOVE_VIEW_NORMAL = 0,
} MoveViewMode;

typedef struct {
	gchar *name;
	gchar *alias;
	email_mailbox_type_e mailbox_type;
	int mailbox_id;
	int unread_count;
	int total_mail_count_on_local;
	int total_mail_count_on_server;
} EmailMailboxNameAndAlias;

typedef struct {
	int show_images;
	int wifi_auto_download;
	int send_read_report;
	int pop3_deleting_option;
	char service_provider_name[128];
} account_user_data_t;

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
	gboolean server_item;
} EmailSearchData;

typedef enum {
	EMAIL_SEARCH_CONTACT_MIN = 0,
	EMAIL_SEARCH_CONTACT_BY_NAME = EMAIL_SEARCH_CONTACT_MIN,
	EMAIL_SEARCH_CONTACT_BY_EMAIL,
	EMAIL_SEARCH_CONTACT_MAX
} EmailContactSearchType;

typedef enum {
	EMAIL_SEARCH_CONTACT_ORIGIN_MIN = 0,
	EMAIL_SEARCH_CONTACT_ORIGIN_CONTACTS = EMAIL_SEARCH_CONTACT_ORIGIN_MIN,
	EMAIL_SEARCH_CONTACT_ORIGIN_RECENT,
	EMAIL_SEARCH_CONTACT_ORIGIN_MAX
} EmailContactSearchOriginType;

typedef enum {
	EF_MULTILINE = 1,
	EF_CLEAR_BTN = 2,
	EF_PASSWORD = 4,
	EF_TITLE_SEARCH = 8,

} EmailEditfieldType;


typedef struct {
	Evas_Object *layout;
	Evas_Object *entry;
} email_editfield_t;

typedef struct _EmailCommonStringType {
	const char *domain;
	const char *id;
} EmailCommonStringType;

#define GDBUS_SIGNAL_SUBSCRIBE_FAILURE -1

typedef struct {
	int red;
	int green;
	int blue;
	int alpha;
} EmailRGBA;

#endif	/* _EMAIL_COMMON_TYPES_H_ */

/* EOF */
