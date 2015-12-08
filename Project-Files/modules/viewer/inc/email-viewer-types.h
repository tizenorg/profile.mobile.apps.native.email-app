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

#ifndef __EMAIL_VIEWER_TYPES_H__
#define __EMAIL_VIEWER_TYPES_H__

#define CONCAT_FORMAT(x, y, z) ((x<<16) | (y<<8) | z)

#define VIEWER_EVAS_DATA_NAME "ViewerUGD"

#define _EDJ(o) elm_layout_edje_get(o)

#define BUF_LEN_T	32		/* Tiny buffer */
#define BUF_LEN_S	128		/* Small buffer */
#define BUF_LEN_M	256		/* Medium buffer */
#define BUF_LEN_L	1024	/* Large buffer */
#define BUF_LEN_H	4096	/* Huge buffer */

#define SHARE_TEXT_MAX_LEN	16 * 1024	/* AUL buffer limit size */

#define URI_SCHEME_HTTP "http://"
#define URI_SCHEME_HTTPS "https://"
#define URI_SCHEME_FTP "ftp://"
#define URI_SCHEME_RTSP "rtsp://"
#define URI_SCHEME_FILE "file://"
#define URI_SCHEME_MMS "mms://"
#define URI_SCHEME_WAP "wap://"
#define URI_SCHEME_MAILTO "mailto:"
#define URI_SCHEME_TEL "tel:"
#define URI_SCHEME_DATE "date:"
#define URI_SCHEME_GEO "geo:"

#define CONTACTUI_REQ_ADD_PHONE_NUMBER 19
#define CONTACTUI_REQ_ADD_EMAIL 20
#define CONTACTUI_REQ_ADD_URL 21

#define EMAIL_VIEW_MAX_TO_COUNT 500

#define EMAIL_VIEWER_TMP_HTML_FILE "tmp_email.html"

#define HEAD_SUBJ_FONT_SIZE 32
#define HEAD_SUBJ_ICON_WIDTH 44
#define HEAD_SUBJ_ICON_HEIGHT HEAD_SUBJ_ICON_WIDTH

#define HEAD_SUBJ_TEXT_MAX_HEIGHT ELM_SCALE_SIZE(88)
#define HEAD_SUBJ_ICON_STYLE "<item size=%dx%d href=file://%s/list_icon/%s></item> "
#define HEAD_SUBJ_TEXT_STYLE "<font_size=%d><color=#%02x%02x%02x%02x>%s</color></font_size>"
#define HEAD_TEXT_STYLE_ELLIPSISED_FONT_COLOR "<ellipsis=%f><font_size=%d><color=#%02x%02x%02x%02x>%s (%s)</color></font_size></ellipsis>"
#define HEAD_SUBJ_TEXT_WITH_ICON_STYLE HEAD_SUBJ_ICON_STYLE HEAD_SUBJ_TEXT_STYLE

#define EMAIL_BODY_DOWNLOAD_STATUS_IMAGES_DOWNLOADED	32

typedef enum {
	EMAIL_VIEWER_PROGRESSBAR_ATT = 0,
	EMAIL_VIEWER_PROGRESSBAR_ATT_ALL,
	EMAIL_VIEWER_PROGRESSBAR_BODY,
	EMAIL_VIEWER_PROGRESSBAR_HTML,
	EMAIL_VIEWER_PROGRESSBAR_MAX
} EMAIL_VIEWER_PROGRESSBAR_TYPE;

typedef enum {
	NONE_LAUNCH_MODE = 0,
	CREATE_NEW_LAUNCH_MODE,
	REPLY_LAUNCH_MODE,
	REPLY_ALL_LAUNCH_MODE,
	FORWARD_LAUNCH_MODE
} MessageComposerLaunchMode;

typedef enum {
	EMAIL_VIEWER_TO_LAYOUT = 1,
	EMAIL_VIEWER_CC_LAYOUT = 2,
	EMAIL_VIEWER_BCC_LAYOUT = 3,
} VIEWER_TO_CC_BCC_LY;

typedef enum {
	EMAIL_NO_NAME = 1,
	EMAIL_SENDER_NAME = 2,
	EMAIL_RECIPIENT_NAME = 3,
} VIEWER_DISPLAY_NAME;

typedef enum {
	EMAIL_VIEWER = 1,
	EML_VIEWER = 2,
} VIEWER_TYPE_E;

typedef enum {
	VIEWER_ERROR_NONE = 0,

	VIEWER_ERROR_FAIL = -1,
	VIEWER_ERROR_DOWNLOAD_FAIL = -2,
	VIEWER_ERROR_NULL_POINTER = -3,
	VIEWER_ERROR_INVALID_ARG = -4,
	VIEWER_ERROR_SERVICE_INIT_FAIL = -5,

	VIEWER_ERROR_NO_DEFAULT_ACCOUNT = -6,
	VIEWER_ERROR_NO_ACCOUNT = -7,
	VIEWER_ERROR_NO_ACCOUNT_LIST = -8,
	VIEWER_ERROR_INVALID_FILE = -9,
	VIEWER_ERROR_INVALID_FILE_SIZE = -10,
	VIEWER_ERROR_ETHUMB_FAIL = -11,

	VIEWER_ERROR_GET_DATA_FAIL = -12,
	VIEWER_ERROR_UPDATE_DATA_FAIL = -13,
	VIEWER_ERROR_MAKE_MAIL_FAIL= -14,
	VIEWER_ERROR_UNKOWN_TYPE = -15,
	VIEWER_ERROR_DBUS_FAIL = -16,
	VIEWER_ERROR_RECIPIENT_DUPLICATED = -17,
	VIEWER_ERROR_ENGINE_INIT_FAILED = -18,
	VIEWER_ERROR_STORAGE_FULL = -19,
	VIEWER_ERROR_NOT_ALLOWED = -20,

	VIEWER_ERROR_GET_POLICY_FAIL = -21,
	VIEWER_ERROR_NOT_ALLOWED_BY_SECURITY_POLICY = -22,

	VIEWER_ERROR_FAILED_TO_LAUNCH_APPLICATION = -23,
	VIEWER_ERROR_OUT_OF_MEMORY = -24,

	VIEWER_ERROR_BODY_NOT_DOWNLOADED = -25,
	VIEWER_ERROR_ALREADY_SAVED = -26,
} VIEWER_ERROR_TYPE_E;

typedef enum {
	BODY_TYPE_TEXT = 0,
	BODY_TYPE_HTML = 1,
	BODY_TYPE_INVALID
} MESSAGE_BODY_TYPE;

typedef struct _EmailAttachmentType EmailAttachmentType;
struct _EmailAttachmentType {
	int attach_id;
	int index;
	char *name;
	char *path;
	char *content_id;
	gint64 size;
	gboolean is_downloaded;
	gboolean inline_content;
};

typedef struct _EmailViewerWebview EmailViewerWebview;
struct _EmailViewerWebview {
	Evas_Coord w, h;
	int content_w, content_h;
	float zoom_rate;

	MESSAGE_BODY_TYPE body_type;
	MESSAGE_BODY_TYPE body_type_prev;

	char *text_content;
	char *uri;
	char *charset;
};

typedef struct _EmailViewerIdlerData EmailViewerIdlerData;
struct _EmailViewerIdlerData {
	void *ug_data;
	void *data;
};

#endif /* __EMAIL_VIEWER_TYPES_H__ */
