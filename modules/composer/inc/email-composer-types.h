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
#ifndef __EMAIL_COMPOSER_TYPES_H__
#define __EMAIL_COMPOSER_TYPES_H__

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <glib-object.h>
#include <Elementary.h>
#include <Evas.h>
#include <Edje.h>
#include <Eina.h>
#include <gio/gio.h>
#include <app_common.h>

#include "email-module-dev.h"
#include "email-debug.h"

/**
 * @brief TODO Investigate in the future
 */
#define _ALWAYS_CC_MYSELF
#define FEATRUE_PREDICTIVE_SEARCH

/**
 * @brief TODO Need to revise all the types of structure and enum for consistency
 */
#define COMPOSER_EVAS_DATA_NAME "ComposerModule"
#define COMPOSER_EVAS_DATA_NAME_POPUP_IS_GENGRID "popup_gengrid"
#define COMPOSER_EVAS_DATA_NAME_POPUP_ITEM_COUNT "popup_item_count"
#define COMPOSER_EVAS_DATA_NAME_POPUP_ITEM_HEIGHT "popup_item_height"

#define COMPOSER_PREDICTIVE_SEARCH_ITEM_DEFAULT_HEIGHT ELM_SCALE_SIZE(144)

#define COMPOSER_SCHEME_MMS "mms://"
#define COMPOSER_SCHEME_WAP "wap://"
#define COMPOSER_SCHEME_FTP "ftp://"
#define COMPOSER_SCHEME_HTTP "http://"
#define COMPOSER_SCHEME_HTTPS "https://"
#define COMPOSER_SCHEME_RTSP "rtsp://"
#define COMPOSER_SCHEME_FILE "file://"
#define COMPOSER_SCHEME_MAILTO "mailto:"

#define COMPOSER_SCHEME_GROUP_EVENT "group://events?"

#define COMPOSER_SCHEME_CLICK_IMAGE "clickimage://"
#define COMPOSER_SCHEME_IMAGE_CONTROL_LAYER "imagecontrollayer://"
#define COMPOSER_SCHEME_RESIZE_START "resizestart://"
#define COMPOSER_SCHEME_RESIZE_END "resizeend://"
#define COMPOSER_SCHEME_START_DRAG "startdrag://"
#define COMPOSER_SCHEME_STOP_DRAG "stopdrag://"
#define COMPOSER_SCHEME_EXCEENDED_MAX "exceededmax://"
#define COMPOSER_SCHEME_CLICK_CHECKBOX "clickcheckbox://"
#define COMPOSER_SCHEME_GET_FOCUS_NEW "getfocusnewmessagediv://"
#define COMPOSER_SCHEME_GET_FOCUS_ORG "getfocusorgmessagediv://"
#define COMPOSER_SCHEME_TEXT_STYLE_CHANGE "getchangedtextstyle://"
#define COMPOSER_SCHEME_CARET_POS_CHANGE "caretposchanged://"

#define COMPOSER_MIME_CONTACT_SHARE "application/vnd.tizen.contact"

/*#define COMPOSER_MAILTO_TO_PREFIX "to="
#define COMPOSER_MAILTO_CC_PREFIX "cc="
#define COMPOSER_MAILTO_BCC_PREFIX "bcc="
#define COMPOSER_MAILTO_SUBJECT_PREFIX "subject="
#define COMPOSER_MAILTO_BODY_PREFIX "body="
#define COMPOSER_MAILTO_ATTACHMENT_PREFIX "attachment="*/

#define COMPOSER_TEXT_STYLE_CHANGE_MESSAGE "getchangedtextstyle://" \
	"font_size=%dpx&" \
	"bold=%d&" \
	"underline=%d&" \
	"italic=%d&" \
	"font_color=rgb(%d, %d, %d)&" \
	"bg_color=rgb(%d, %d, %d)"

#define EMAIL_COMPOSER_TMP_FOLDER_PERMISSION 0775

#define MAX_MESSAGE_SIZE		100
#define MAX_RECIPIENT_COUNT		100
#define MAX_ATTACHMENT_ITEM		30
#define MAX_ATTACHMENT_SIZE		50 * 1024 * 1024
#define MAX_RESIZABLE_IMAGE_FILE_SIZE		10 * 1024 * 1024
#define ATTACHMENT_HEIGHT		96
#define ATTACHMENT_GROUP_HEIGHT		51

#define TEXT_STYLE_SUBJECT_GUIDE	"<font_size=%d><color=#%02x%02x%02x%02x>%s</color></font_size>"
#define TEXT_STYLE_ENTRY_STRING		"DEFAULT='font_size=%d color=#%02x%02x%02x%02x'"
#define TEXT_STYLE_ENTRY_FROM_ADDRESS   "<align=left><color=#%02x%02x%02x%02x>%s</color></align>"
#define FONT_SIZE_ENTRY 40

#define COMPOSER_TAG_MATCH_PREFIX	"<match>"
#define COMPOSER_TAG_MATCH_SUFFIX	"</match>"

#define COMPOSER_VCARD_SAVE_POPUP_SHOW_MIN_TIME_SEC 1.0
#define COMPOSER_VCARD_SAVE_STATUS_CHECK_INTERVAL_SEC 0.1

/**
 * @brief Email popup element type enum
 */
typedef enum {
	POPUP_ELEMENT_TYPE_NONE = 0,
	POPUP_ELEMENT_TYPE_TITLE = 1,
	POPUP_ELEMENT_TYPE_CONTENT,
	POPUP_ELEMENT_TYPE_BTN1,
	POPUP_ELEMENT_TYPE_BTN2,
	POPUP_ELEMENT_TYPE_BTN3,
} EmailPopupElementType;

/**
 * @brief Email popup string type enum
 */
typedef enum {
	PACKAGE_TYPE_NOT_AVAILABLE = 0,
	PACKAGE_TYPE_PACKAGE,
	PACKAGE_TYPE_SYS_STRING,
} EmailPopupStringType;

/**
 * @brief Composer CS events flags enum
 */
typedef enum {
	COMPOSER_CSEF_DRAG_START = 1,
	COMPOSER_CSEF_DRAGGING = 2,

	COMPOSER_CSEF_MAIN_SCROLLER_RESIZE = 4,
	COMPOSER_CSEF_MAIN_CONTENT_RESIZE = 8,

	COMPOSER_CSEF_EWK_CONTENT_RESIZE = 16,

	COMPOSER_CSEF_INITIALIZE = 32

} ComposerCSEventFlag;

/**
 * @brief Composer CS events masks enum
 */
typedef enum {
	COMPOSER_CSEF_DRAG_EVENTS =
			COMPOSER_CSEF_DRAG_START |
			COMPOSER_CSEF_DRAGGING,

	COMPOSER_CSEF_IMMEDIATE_EVENTS =
			COMPOSER_CSEF_DRAG_EVENTS |
			COMPOSER_CSEF_MAIN_SCROLLER_RESIZE |
			COMPOSER_CSEF_MAIN_CONTENT_RESIZE,

	COMPOSER_CSEF_PENDING_EVENTS =
			COMPOSER_CSEF_EWK_CONTENT_RESIZE |
			COMPOSER_CSEF_INITIALIZE

} ComposerCSEventMasks;

/**
 * @brief Email address info data
 */
typedef struct {
	int type;
	char address[EMAIL_LIMIT_EMAIL_ADDRESS_LENGTH + 1];
} EmailAddressInfo;

/**
 * @brief Email Recipient info data
 */
typedef struct {
	int selected_email_idx;
	int email_id;
	Eina_Bool is_always_bcc;
	gchar *display_name;	/* Specific display name of recipient */
	Eina_List *email_list;
} EmailRecpInfo;

/**
 * @brief Predictive search account bar item data
 */
typedef struct {
	int acc_svc_id;
	char *acc_name;
	Elm_Object_Item *gl_item;
	Eina_List *gal_search_list;
} PredictiveSearchAccountBarItem;

/**
 * @brief COMPOSER ERROR TYPE enum
 */
typedef enum {
	COMPOSER_ERROR_NONE = 0,

	COMPOSER_ERROR_FAIL = -1,
	COMPOSER_ERROR_SEND_FAIL = -2,
	COMPOSER_ERROR_NULL_POINTER = -3,
	COMPOSER_ERROR_INVALID_ARG = -4,
	COMPOSER_ERROR_SERVICE_INIT_FAIL = -5,

	COMPOSER_ERROR_NO_DEFAULT_ACCOUNT = -6,
	COMPOSER_ERROR_NO_ACCOUNT = -7,
	COMPOSER_ERROR_NO_ACCOUNT_LIST = -8,
	COMPOSER_ERROR_INVALID_FILE = -9,
	COMPOSER_ERROR_INVALID_FILE_SIZE = -10,
	COMPOSER_ERROR_ETHUMB_FAIL = -11,

	COMPOSER_ERROR_GET_DATA_FAIL = -12,
	COMPOSER_ERROR_UPDATE_DATA_FAIL = -13,
	COMPOSER_ERROR_MAKE_MAIL_FAIL= -14,
	COMPOSER_ERROR_UNKOWN_TYPE = -15,
	COMPOSER_ERROR_DBUS_FAIL = -16,
	COMPOSER_ERROR_RECIPIENT_DUPLICATED = -17,
	COMPOSER_ERROR_ENGINE_INIT_FAILED = -18,
	COMPOSER_ERROR_STORAGE_FULL = -19,
	COMPOSER_ERROR_NOT_ALLOWED = -20,

	COMPOSER_ERROR_GET_POLICY_FAIL = -21,
	COMPOSER_ERROR_NOT_ALLOWED_BY_SECURITY_POLICY = -22,

	COMPOSER_ERROR_FAILED_TO_LAUNCH_APPLICATION = -23,
	COMPOSER_ERROR_OUT_OF_MEMORY = -24,
	COMPOSER_ERROR_MAIL_SIZE_EXCEEDED = -25,
	COMPOSER_ERROR_MAIL_STORAGE_FULL = -26,

	COMPOSER_ERROR_VCARD_CREATE_FAILED = -27,

	COMPOSER_ERROR_ATTACHMENT_NOT_EXIST				= -801,
	COMPOSER_ERROR_ATTACHMENT_MAX_NUMBER_EXCEEDED	= -803,
	COMPOSER_ERROR_ATTACHMENT_MAX_SIZE_EXCEEDED		= -804,
	COMPOSER_ERROR_ATTACHMENT_DUPLICATE				= -805,
} COMPOSER_ERROR_TYPE_E;

typedef struct _EmailComposerAccount EmailComposerAccount;
typedef struct _EmailComposerMail EmailComposerMail;
typedef struct _ComposerAttachmentItemData ComposerAttachmentItemData;

/**
 * @brief Email composer account data
 */
struct _EmailComposerAccount {
	int original_account_id;
	int account_type;
	int max_sending_size;
	int account_count;
	Evas_Object *account_rdg;
	email_account_t *original_account;
	email_account_t *selected_account;
	email_account_t *account_list;
};

/**
 * @brief Email composer mail data
 */
struct _EmailComposerMail {
	email_mail_data_t *mail_data;
	email_attachment_data_t *attachment_list;
	int total_attachment_count;
};

/**
 * @brief Composer attachment item data
 */
struct _ComposerAttachmentItemData {
	void *view;
	email_attachment_data_t *attachment_data;
	char *preview_path;
	Evas_Object *layout;
	Eina_Bool from_user;
};

#define _EDJ(o) elm_layout_edje_get(o)
#define COMPOSER_STRDUP(src) (((src)) ? g_strdup((src)) : NULL)

typedef unsigned char		BYTE;
typedef unsigned short		WORD;
typedef unsigned long		DWORD;
typedef unsigned long		COLORREF;

#define RGB(r,g,b)	((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))

/**
 * @brief COMPOSER RECIPIENT TYPE enum
 */
typedef enum {
	COMPOSER_RECIPIENT_TYPE_NONE = 0,
	COMPOSER_RECIPIENT_TYPE_TO,
	COMPOSER_RECIPIENT_TYPE_CC,
	COMPOSER_RECIPIENT_TYPE_BCC,
	COMPOSER_RECIPIENT_TYPE_CC_BCC,
	COMPOSER_RECIPIENT_TYPE_FROM,
	COMPOSER_RECIPIENT_TYPE_MAX
} COMPOSER_RECIPIENT_TYPE_E;

#define GET_VALUE_WITH_RATE(total, rate) ((int)((float)(total) * rate / 100))

#define COMPOSER_GET_TIMER_DATA(tdata, name, data) \
        COMMON_GET_TIMER_DATA(tdata, EmailComposerView, name, data) \

/**
 * @brief Rich button state data
 */
typedef struct {
	Evas_Object *button;
	Eina_Bool state;
} RichButtonState;

/**
 * @brief Rich Text Types enum
 */
typedef enum {
	RICH_TEXT_TYPE_NONE = -1,
	RICH_TEXT_TYPE_FONT_SIZE,
	RICH_TEXT_TYPE_BOLD,
	RICH_TEXT_TYPE_ITALIC,
	RICH_TEXT_TYPE_UNDERLINE,
	RICH_TEXT_TYPE_FONT_COLOR,
	RICH_TEXT_TYPE_BACKGROUND_COLOR,
	RICH_TEXT_TYPE_COUNT
} RichTextTypes;

#endif /* __EMAIL_COMPOSER_TYPES_H__ */
