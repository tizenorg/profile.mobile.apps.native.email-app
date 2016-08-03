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

#ifndef __EMAIL_COMPOSER_JS_H__
#define __EMAIL_COMPOSER_JS_H__

#include "email-composer.h"

#define IMAGE_RESIZE_RATIO	0.95

#define EC_HTML_META_INFO_FMT \
	"<html>" \
	"<head>" \
	"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">" \
	"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, maximum-scale=1.0, minimum-scale=1.0, user-scalable=no\" />" \
	"<link rel=\"stylesheet\" type=\"text/css\" href=\"%s/email-composer.css\">" \
	"<script src=\"%s/email-composer.js\"></script>" \
	"</head>\n"

#define EC_HTML_META_INFO_SIZE (sizeof(EC_HTML_META_INFO_FMT) + EMAIL_SMALL_PATH_MAX * 2)

/*
 * Many names are from email-composer.js
 */
#define EC_NAME_NEW_MESSAGE "DIV_NewMessage"
#define EC_NAME_ORG_MESSAGE "DIV_OrgMessage"
#define EC_NAME_ORIGINAL_MESSAGE_BAR "DIV_OrgMessageBar"
#define EC_NAME_ORIGINAL_MESSAGE_BAR_ROW "DIV_OrgMessageBarRow"
#define EC_NAME_ORIGINAL_MESSAGE_BAR_CHECKBOX_CELL "DIV_OrgMessageBarCheckCell"
#define EC_NAME_ORIGINAL_MESSAGE_BAR_CHECKBOX "DIV_OrgMessageBarCheckbox"
#define EC_NAME_ORIGINAL_MESSAGE_BAR_CHECKBOX_ICON "DIV_OrgMessageBarCheckboxIcon"

#define EC_NAME_CSS_NEW_MESSAGE "css-newMessage"
#define EC_NAME_CSS_ORG_MESSAGE "css-originalMessage"
#define EC_NAME_CSS_ORG_MESSAGE_BAR_CHECKED "css-originalMessageBar-checked"
#define EC_NAME_CSS_ORG_MESSAGE_BAR_ROW "css-originalMessageBar-row"
#define EC_NAME_CSS_ORG_MESSAGE_BAR_CHECKBOX_CELL "css-originalMessageBarCheckbox-cell"
#define EC_NAME_CSS_ORG_MESSAGE_BAR_CHECKBOX_CHECKED "css-originalMessageBarCheckbox-checked"

#define EC_TAG_DIV_DIR_S "<div dir='auto'>"
#define EC_TAG_DIV_F "</div>"
#define EC_TAG_DIV_DIR EC_TAG_DIV_DIR_S"<br>"EC_TAG_DIV_F
#define EC_TAG_SIGNATURE_DEFAULT EC_TAG_DIV_DIR
#define EC_TAG_SIGNATURE_TEXT EC_TAG_DIV_DIR""EC_TAG_DIV_DIR""EC_TAG_DIV_DIR""EC_TAG_DIV_DIR_S"<span style='font-size: 14px; color: rgba(%d, %d, %d, %d);'>%s</span>"EC_TAG_DIV_F
#define EC_TAG_BODY_CSS_START "<body class='css-body'>\n"
#define EC_TAG_BODY_ORIGINAL_MESSAGE_START "<body class='css-body-IncludeOriginalMessage'>\n"
#define EC_TAG_BODY_END "</body>\n"
#define EC_TAG_HTML_END "</html>\n"

#define EC_TAG_NEW_MSG_START "<div id='"EC_NAME_NEW_MESSAGE"' class='"EC_NAME_CSS_NEW_MESSAGE"'>\n"
#define EC_TAG_NEW_MSG_END EC_TAG_DIV_F"\n"
#define EC_TAG_ORG_MSG_START "<div id='"EC_NAME_ORG_MESSAGE"' class='"EC_NAME_CSS_ORG_MESSAGE"'>\n"
#define EC_TAG_ORG_MSG_END EC_TAG_DIV_F"\n"
#define EC_TAG_ORIGINAL_MESSAGE_BAR \
	"<div id='"EC_NAME_ORIGINAL_MESSAGE_BAR"' class='"EC_NAME_CSS_ORG_MESSAGE_BAR_CHECKED"'>" \
	"<div id='"EC_NAME_ORIGINAL_MESSAGE_BAR_ROW"' class='"EC_NAME_CSS_ORG_MESSAGE_BAR_ROW"'>" \
	"<div id='"EC_NAME_ORIGINAL_MESSAGE_BAR_CHECKBOX_CELL"' class='"EC_NAME_CSS_ORG_MESSAGE_BAR_CHECKBOX_CELL"'>" \
	"<div id='"EC_NAME_ORIGINAL_MESSAGE_BAR_CHECKBOX"' class='"EC_NAME_CSS_ORG_MESSAGE_BAR_CHECKBOX_CHECKED"' tabindex=0>" \
	"<div id='"EC_NAME_ORIGINAL_MESSAGE_BAR_CHECKBOX_ICON"' class='checkicon'>" \
	EC_TAG_DIV_F""EC_TAG_DIV_F""EC_TAG_DIV_F""EC_TAG_DIV_F""EC_TAG_DIV_F

#define EC_JS_ENABLE_EDITABLE_BODY	"document.body.contentEditable='true'; void 0;"
#define EC_JS_ENABLE_EDITABLE_DIVS \
	"document.getElementById('"EC_NAME_NEW_MESSAGE"').contentEditable=\"true\";" \
	"document.getElementById('"EC_NAME_ORIGINAL_MESSAGE_BAR"').contentEditable=\"false\";" \
	"document.getElementById('"EC_NAME_ORG_MESSAGE"').contentEditable=\"true\"; void 0;"

#define EC_JS_SET_FOCUS					"document.body.focus(); void 0;"
#define EC_JS_SET_UNFOCUS				"document.body.blur(); void 0;"
#define EC_JS_SET_FOCUS_NEW_MESSAGE		"document.getElementById('"EC_NAME_NEW_MESSAGE"').focus(); void 0;"
#define EC_JS_SET_UNFOCUS_NEW_MESSAGE	"document.getElementById('"EC_NAME_NEW_MESSAGE"').blur(); void 0;"
#define EC_JS_SET_FOCUS_ORG_MESSAGE		"document.getElementById('"EC_NAME_ORG_MESSAGE"').focus(); void 0;"
#define EC_JS_SET_UNFOCUS_ORG_MESSAGE	"document.getElementById('"EC_NAME_ORG_MESSAGE"').blur(); void 0;"

/* These javascript functions are defined in email-composer.js */
#define EC_JS_INITIALIZE_COMPOSER "InitializeEmailComposer('%s', '%s');"

#define EC_JS_IS_CHECKBOX_CHECKED "IsCheckboxChecked();"
#define EC_JS_HAS_ORIGINAL_MESSAGE_BAR "HasOriginalMessageBar();"
#define EC_JS_INSERT_TEXT_TO_ORIGINAL_MESSAGE_BAR "InsertOriginalMessageText(%d, %d, %d, %d, \"%s\");"
#define EC_JS_UPDATE_TEXT_TO_ORIGINAL_MESSAGE_BAR "UpdateOriginalMessageText(\"%s\");"

#define EC_JS_INSERT_ORIGINAL_MAIL_INFO "InsertOriginalMailInfo(\"%s\");"
#define EC_JS_INSERT_SIGNATURE "InsertSignature(\"%s\");"
#define EC_JS_INSERT_SIGNATURE_TO_NEW_MESSAGE "InsertSignatureTo(\""EC_NAME_NEW_MESSAGE"\", \"%s\");"

#define EC_JS_UPDATE_COLOR_OF_ORG_MESSAGE_BAR "UpdateBGColorTo(\""EC_NAME_ORIGINAL_MESSAGE_BAR"\", %d, %d, %d, %d);"
#define EC_JS_UPDATE_COLOR_OF_CHECKBOX "UpdateBGColorTo(\""EC_NAME_ORIGINAL_MESSAGE_BAR_CHECKBOX"\", %d, %d, %d, %d);"
#define EC_JS_UPDATE_COLOR_OF_CHECKBOX_ICON "UpdateBGColorTo(\""EC_NAME_ORIGINAL_MESSAGE_BAR_CHECKBOX_ICON"\", %d, %d, %d, %d);"

#define EC_JS_INSERT_IMAGE "InsertImage(\"%s\");"
#define EC_JS_GET_IMAGE_LIST "GetImageSources();"
#define EC_JS_GET_IMAGE_LIST_FROM_NEW_MESSAGE "GetImageSourcesFrom(\""EC_NAME_NEW_MESSAGE"\");"

#define EC_JS_GET_INITIAL_CONTENTS "GetComposedHtmlContents('0');"
#define EC_JS_GET_COMPOSED_CONTENTS "GetComposedHtmlContents('%s');"

#define EC_JS_NOTIFY_CARET_POS_CHANGE "NotifyCaretPosChange(true);"

#define EC_JS_SET_SCROLL_TOP "SetScrollTop(%d);"

#endif /* __EMAIL_COMPOSER_JS_H__ */
