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

#include <notification.h>
#include <media_content.h>

#include "email-debug.h"
#include "email-engine.h"
#include "email-html-converter.h"
#include "email-viewer.h"
#include "email-viewer-scroller.h"
#include "email-viewer-js.h"
#include "email-viewer-util.h"
#include "email-viewer-contents.h"
#include "email-viewer-more-menu.h"
#include "email-viewer-ext-gesture.h"
#include "email-viewer-recipient.h"

typedef enum {
	INPUT_FIELD = 0,
	TEXT_ONLY,
	TEXT_LINK,
	IMAGE_ONLY,
	IMAGE_LINK,
	UNKNOWN_MENU
} context_menu_type;

typedef enum {
	CUSTOM_CONTEXT_MENU_ITEM_SMART_SEARCH = EWK_CONTEXT_MENU_ITEM_BASE_APPLICATION_TAG,
	CUSTOM_CONTEXT_MENU_ITEM_SHARE,
} custom_context_menu_item_tag;


/*
 * Declaration for static functions
 */

#ifdef _WEBKIT_CONSOLE_MESSAGE_LOG
static void _webview_console_message(void *data, Evas_Object *obj, void *event_info);
#endif
static void _webview_load_started_cb(void *data, Evas_Object *obj, void *event_info);
static void _webview_load_finished_cb(void *data, Evas_Object *obj, void *event_info);
static void _webview_policy_navigation_decide_cb(void *data, Evas_Object *obj, void *event_info);
static void _webview_load_error_cb(void *data, Evas_Object *obj, void *event_info);
static void _webview_load_noemptylayout_finished_cb(void *data, Evas_Object *obj, void *event_info);
static void _webview_load_progress_cb(void *data, Evas_Object *obj, void *event_info);
static Eina_Bool _webview_scroll_region_bringin_idler(void *data);
static void _webview_contextmenu_willshow_cb(void *data, Evas_Object *obj, void *event_info);

static void _webview_edge_top_cb(void *data, Evas_Object *obj, void *event_info);
static void _webview_edge_bottom_cb(void *data, Evas_Object *obj, void *event_info);
static void _webview_mouse_down_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _webview_resize_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _webview_zoom_started_cb(void *data, Evas_Object *obj, void *event_info);
static void _webview_zoom_finished_cb(void *data, Evas_Object *obj, void *event_info);

static void _webview_contextmenu_customize_cb(void *data, Evas_Object *webview, void *event_info);
static void _webview_contextmenu_selected_cb(void *data, Evas_Object *webview, void *event_info);
static void _webview_magnifier_show_cb(void *data, Evas_Object *obj, void *event_info);
static void _webview_magnifier_hide_cb(void *data, Evas_Object *obj, void *event_info);

static void _webview_btn_focused(void *data, Evas_Object *obj, void *event_info);
static void _webview_btn_unfocused(void *data, Evas_Object *obj, void *event_info);
static void _viewer_launch_smart_search(EmailViewerView *view, char *link_url);
static void _viewer_share_text(EmailViewerView *view, char *share_text);
static char *_viewer_convert_plain_text_body(EmailViewerWebview *wvd);

static int _g_pos_x;
static int _g_pos_y;

/*
 * Definition for static functions
 */

static void _create_combined_scroller_and_resize_webview(EmailViewerView *view)
{
	/*create scroller*/
	viewer_create_combined_scroller(view);

	/*resize webview*/
	double scale = ewk_view_scale_get(view->webview);
	int content_height = 0;
	ewk_view_contents_size_get(view->webview, NULL, &content_height);
	if (content_height) {
		viewer_resize_webview(view, content_height * scale);
	}
}

#ifdef _WEBKIT_CONSOLE_MESSAGE_LOG
static void _webview_console_message(void *data, Evas_Object *obj, void *event_info)
{
	retm_if(data == NULL, "Invalid parameter: data is NULL!");
	retm_if(event_info == NULL, "Invalid parameter: event_info is NULL!");

	Ewk_Console_Message *msg = (Ewk_Console_Message *)event_info;

	const char *log = ewk_console_message_text_get(msg);
	unsigned line = ewk_console_message_line_get(msg);

	debug_secure("ConMsg:%d> %s", line, log);
}
#endif

static Eina_Bool _webview_show_page(void *data)
{
	debug_enter();
	retvm_if(data == NULL, ECORE_CALLBACK_CANCEL, "Invalid parameter: data is NULL!");
	EmailViewerView *view = (EmailViewerView *)data;

	debug_log("scale:%f", ewk_view_scale_get(view->webview));

	debug_leave();
	return ECORE_CALLBACK_CANCEL;
}

static void _show_image_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;

	if (email_get_network_state() != EMAIL_NETWORK_STATUS_AVAILABLE) {
		int ret = notification_status_message_post(email_get_email_string("IDS_EMAIL_TPOP_FAILED_TO_CONNECT_TO_NETWORK"));
		debug_warning_if(ret != NOTIFICATION_ERROR_NONE, "notification_status_message_post() failed! ret:(%d)", ret);
		return;
	}

	if (view->body_download_status != EMAIL_BODY_DOWNLOAD_STATUS_NONE) {
		email_mail_attribute_value_t image_dn_status;
		debug_log("show_image_set");
		image_dn_status.integer_type_value = view->body_download_status | EMAIL_BODY_DOWNLOAD_STATUS_IMAGES_DOWNLOADED;
		email_engine_update_mail_attribute(view->account_id, &view->mail_id, 1, EMAIL_MAIL_ATTRIBUTE_BODY_DOWNLOAD_STATUS, image_dn_status);
	}

	if (view->webview != NULL) {
		viewer_delete_body_button(view);
		view->b_show_remote_images = EINA_TRUE;

		create_body_progress(view);
		_create_body(view);
	}

	debug_leave();
}

static void _check_show_image_cb(Evas_Object *o, const char *result, void *data)
{
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	if (result) {
		debug_secure("Is there any image in body content? [%s]", result);
		if (!g_strcmp0(result, "true")) {
			debug_log("body content has one image at least");
			if (!view->b_show_remote_images && (view->viewer_type != EML_VIEWER) &&
					viewer_check_body_download_status(view->body_download_status, EMAIL_BODY_DOWNLOAD_STATUS_FULLY_DOWNLOADED)) {
				bool image_shown = false;
				if (viewer_check_body_download_status(view->body_download_status, EMAIL_BODY_DOWNLOAD_STATUS_IMAGES_DOWNLOADED)) {
					Ewk_Settings *ewk_settings = ewk_view_settings_get(view->webview);
					if (ewk_settings_loads_images_automatically_set(ewk_settings, EINA_TRUE) == EINA_FALSE) {
						debug_log("SET show images is FAILED!");
					} else {
						image_shown = true;
					}
				}
				if (!image_shown) {

					viewer_create_body_button(view, "IDS_EMAIL_MBODY_DISPLAY_IMAGES", _show_image_cb);
					Evas_Object *content = evas_object_smart_parent_get(view->base.content);

					if (content) {
						edje_object_message_signal_process(content);
					} else {
						edje_message_signal_process();
					}
				}
			} else {
				if (view->webview != NULL) {
					Ewk_Settings *ewk_settings = ewk_view_settings_get(view->webview);
					debug_log("b_show_remote_images is %d", view->b_show_remote_images);
					if (ewk_settings_loads_images_automatically_set(ewk_settings, (view->b_show_remote_images || (view->viewer_type == EML_VIEWER))) == EINA_FALSE) {
						debug_log("SET show images is FAILED!");
					}
				}
			}
		}
	}

	/* XXX: resize webview here instead of
	 * _webview_load_noemptylayout_finished_cb, because toolbar is created in
	 * this function and its height affects the webview height
	 */
	_create_combined_scroller_and_resize_webview(view);
}

static void _webview_load_started_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
}

static void _webview_load_finished_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

#ifdef _EWK_LOAD_NONEMPTY_LAYOUT_FINISHED_CB_UNAVAILABLE_
	_webview_load_noemptylayout_finished_cb(data, obj, NULL);
#endif

	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;
	retm_if(view->mail_info == NULL, "view->mail_info is NULL!");

	_webview_show_page(view);

	debug_leave();
}

static Eina_Bool _handle_link_scheme(void *data, const char *link_uri, bool newwindow)
{
	debug_enter();
	retvm_if(!data, EINA_FALSE, "Invalid parameter: data[NULL]");
	retvm_if(!link_uri, EINA_FALSE, "Invalid parameter: link_uri[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;
	if ((strncmp(link_uri, URI_SCHEME_HTTP, strlen(URI_SCHEME_HTTP)) == 0) ||
			(strncmp(link_uri, URI_SCHEME_HTTPS, strlen(URI_SCHEME_HTTPS)) == 0) ||
			(strncmp(link_uri, URI_SCHEME_FTP, strlen(URI_SCHEME_FTP)) == 0) ||
			(strncmp(link_uri, URI_SCHEME_MMS, strlen(URI_SCHEME_MMS)) == 0) ||
			(strncmp(link_uri, URI_SCHEME_WAP, strlen(URI_SCHEME_WAP)) == 0)) {
		debug_log("launch browser");
		viewer_launch_browser(view, link_uri);
		debug_leave();
		return EINA_TRUE;
	} else if (strncmp(link_uri, URI_SCHEME_MAILTO, strlen(URI_SCHEME_MAILTO)) == 0) {
		debug_log("launch email composer");
		viewer_create_email(view, link_uri);
		return EINA_TRUE;
	} else {
		debug_log("Unsupported link scheme");
		return EINA_FALSE;
	}

}

static void _webview_policy_cb(void *data, Evas_Object *obj, void *event_info, bool newwindow)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;
	Ewk_Policy_Decision *policy_decision = (Ewk_Policy_Decision *)event_info;
	Ewk_Policy_Navigation_Type pd_type = ewk_policy_decision_navigation_type_get(policy_decision);
	debug_log("Ewk_Policy_Navigation_Type:%d", pd_type);

	const char *uri = NULL;
	uri = ewk_view_url_get(view->webview);
	debug_secure("ewk_view_url_get(%s)", uri);

	if (view->b_load_finished == EINA_TRUE) {
		if (view->is_long_pressed == EINA_TRUE) {
			debug_log("Long pressed!!!");
			view->is_long_pressed = EINA_FALSE;
			ewk_policy_decision_ignore(policy_decision);
			return;
		}

		ewk_policy_decision_ignore(policy_decision);

		const char *url = ewk_policy_decision_url_get(policy_decision);
		debug_secure("url:%s", url);
		debug_secure("scheme:%s", ewk_policy_decision_scheme_get(policy_decision));
		debug_secure("cookie:%s", ewk_policy_decision_cookie_get(policy_decision));
		debug_secure("host:%s", ewk_policy_decision_host_get(policy_decision));
		debug_secure("response_mime:%s", ewk_policy_decision_response_mime_get(policy_decision));
		debug_secure("http_method:%s", ewk_policy_decision_http_method_get(policy_decision));
		debug_secure("response_status:%d", ewk_policy_decision_response_status_code_get(policy_decision));
		debug_secure("Ewk_Policy_Decision_Type:%d", ewk_policy_decision_type_get(policy_decision));
		debug_secure("Ewk_Policy_Navigation_Type:%d", ewk_policy_decision_navigation_type_get(policy_decision));

		Eina_Bool is_scheme_handled = _handle_link_scheme(data, url, newwindow);
		debug_log("scheme handled [%d]", is_scheme_handled);
	} else {
		debug_log("Loading...");

		if (pd_type == EWK_POLICY_NAVIGATION_TYPE_LINK_CLICKED) {
			debug_log("Link click is ignored.");
			ewk_policy_decision_ignore(policy_decision);
			return;
		}
	}

	debug_leave();
}

static void _webview_policy_navigation_decide_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	_webview_policy_cb(data, obj, event_info, false);
}

static void _webview_policy_newwindow_decide_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	_webview_policy_cb(data, obj, event_info, true);
}

static void _webview_load_error_cb(void *data, Evas_Object *obj, void *event_info)
{
	Ewk_Error *error = (Ewk_Error *)event_info;
	const char *url = ewk_error_url_get(error);
	if (g_strcmp0(url, email_get_res_url()) != 0) {
		int error_code = ewk_error_code_get(error);
		const char *description = ewk_error_description_get(error);

		char buf[EMAIL_BUFF_SIZE_4K] = { 0, };
		snprintf(buf, sizeof(buf), "Error occured!<br>Url: (%s)<br>Code: (%d)<br>Description: (%s)<br><br>Please report us", url, error_code, description);

		debug_secure_error("%s", buf);
	}
}

static void _webview_load_noemptylayout_finished_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	/* delay destroying viewer if message deleted from server */
	view->can_destroy_on_msg_delete = true;
	if (view->need_pending_destroy) {
		debug_log("email_module_make_destroy_request");
		email_module_make_destroy_request(view->base.module);
		return;
	}

	if (ewk_view_script_execute(view->webview, VIEWER_JS_HAVE_IMAGES, _check_show_image_cb, view) == EINA_FALSE) {
		_create_combined_scroller_and_resize_webview(view);
		debug_log("VIEWER_JS_HAVE_IMAGES failed.");
	}
	if (view->list_progressbar != NULL) {
		debug_log("destroy list_progressbar");
		elm_object_part_content_unset(view->base_ly, "ev.swallow.progress");
		DELETE_EVAS_OBJECT(view->list_progressbar);
	}

	if ((view->webview_ly) && (view->webview)) {
		elm_object_part_content_set(view->webview_ly, "elm.swallow.content", view->webview);
		evas_object_show(view->webview_ly);
		evas_object_show(view->webview);
	}

	debug_log("set b_load_finished");
	view->b_load_finished = EINA_TRUE;
	debug_leave();
}

static void _webview_load_progress_cb(void *data, Evas_Object *obj, void *event_info)
{
	double *progress = (double *)event_info;
	if (progress) {
		debug_log("===> load,progress: [%f]", *progress);
		EmailViewerView *view = (EmailViewerView *)data;
		if (view) view->loading_progress = *progress;
	}
}

/* Double_Scroller */
static void _webview_edge_top_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	view->is_top_webview_reached = EINA_TRUE;

	viewer_stop_webkit_scroller_start_elm_scroller(view);

	debug_leave();
}

static void _webview_edge_bottom_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	view->is_bottom_webview_reached = EINA_TRUE;

	edje_object_part_drag_value_set(_EDJ(view->combined_scroller), "elm.dragable.vbar", 0.0, 1.0);

	debug_leave();
}

static void _webview_edge_left_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;
	edje_object_part_drag_value_set(_EDJ(view->combined_scroller), "elm.dragable.hbar", 0.0, 0.0);

	debug_leave();
}

static void _webview_edge_right_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerView *view = (EmailViewerView *)data;
	edje_object_part_drag_value_set(_EDJ(view->combined_scroller), "elm.dragable.hbar", 1.0, 0.0);

	debug_leave();
}

static void _webview_mouse_down_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;
	retm_if(view->webview == NULL, "view->webview is NULL.");

	Evas_Event_Mouse_Down *mouse_info = (Evas_Event_Mouse_Down *)event_info;
	/*debug_log("output position: [%d, %d]", mouse_info->output.x, mouse_info->output.y);*/
	_g_pos_x = mouse_info->output.x;
	_g_pos_y = mouse_info->output.y;
	view->is_long_pressed = EINA_FALSE;
}

static void _webview_resize_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	debug_enter();
}

static void _webview_zoom_started_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	if (view->scroller_locked == FALSE) {
		view->scroller_locked = TRUE;
		debug_log("<<lock scroller>>");
		debug_log("<<unlock webview>>");
	}

	debug_leave();
}

static void _webview_zoom_finished_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	retm_if(!view->b_load_finished, "loading is in progress");

	if (view->scroller_locked) {
		view->scroller_locked = FALSE;
		debug_log("<<unlock scroller>>");
		debug_log("<<lock webview>>");
	}

	if (view->webview && (view->is_main_scroller_scrolling == EINA_TRUE)) {
		Evas_Coord wx, wy;
		ewk_view_scroll_pos_get(view->webview, &wx, &wy);
		debug_log("webkit scroller x[%d] y[%d]", wx, wy);
		ewk_view_scroll_set(view->webview, wx, 0);
	}

	/*changing webview size based on scale change*/
	double scale = ewk_view_scale_get(view->webview);
	int content_height = 0;
	ewk_view_contents_size_get(view->webview, NULL, &content_height);
	if (content_height) {
		viewer_resize_webview(view, content_height * scale);
	}

	debug_log("scale:%f", scale);
	debug_log("content_height:%d", content_height);

	debug_leave();
}

static context_menu_type _get_menu_type(Ewk_Context_Menu *contextmenu)
{
	retvm_if(contextmenu == NULL, UNKNOWN_MENU, "Invalid parameter: contextmenu[NULL]");

	int count = ewk_context_menu_item_count(contextmenu);
	debug_log("context menu item count:%d", count);
	Eina_Bool text = EINA_FALSE;
	Eina_Bool link = EINA_FALSE;
	Eina_Bool image = EINA_FALSE;
	Eina_Bool num_email = EINA_FALSE;
	int i = 0;

	/* 2013. 11. 16. original context menu list
	 * -----> Normal text
	 * 83: EWK_CONTEXT_MENU_ITEM_TAG_SELECT_ALL
	 *  8: EWK_CONTEXT_MENU_ITEM_TAG_COPY
	 * 21: EWK_CONTEXT_MENU_ITEM_TAG_SEARCH_WEB
	 * 88: EWK_CONTEXT_MENU_ITEM_TAG_TRANSLATE
	 * -----> URL link
	 *  1: EWK_CONTEXT_MENU_ITEM_TAG_OPEN_LINK_IN_NEW_WINDOW
	 *  2: EWK_CONTEXT_MENU_ITEM_TAG_DOWNLOAD_LINK_TO_DISK
	 *  3: EWK_CONTEXT_MENU_ITEM_TAG_COPY_LINK_TO_CLIPBOARD
	 * 85: EWK_CONTEXT_MENU_ITEM_TAG_TEXT_SELECTION_MODE
	 * 87: EWK_CONTEXT_MENU_ITEM_TAG_DRAG
	 * -----> Number/Email link
	 * 89: EWK_CONTEXT_MENU_ITEM_TAG_COPY_LINK_DATA
	 * 85: EWK_CONTEXT_MENU_ITEM_TAG_TEXT_SELECTION_MODE
	 * 87: EWK_CONTEXT_MENU_ITEM_TAG_DRAG
	 * -----> Date/UNC/geo link
	 *  3: EWK_CONTEXT_MENU_ITEM_TAG_COPY_LINK_TO_CLIPBOARD
	 * 85: EWK_CONTEXT_MENU_ITEM_TAG_TEXT_SELECTION_MODE
	 * 87: EWK_CONTEXT_MENU_ITEM_TAG_DRAG
	 * -----> Local image
	 *  4: EWK_CONTEXT_MENU_ITEM_TAG_OPEN_IMAGE_IN_NEW_WINDOW
	 *  5: EWK_CONTEXT_MENU_ITEM_TAG_DOWNLOAD_IMAGE_TO_DISK
	 *  6: EWK_CONTEXT_MENU_ITEM_TAG_COPY_IMAGE_TO_CLIPBOARD
	 *  7: EWK_CONTEXT_MENU_ITEM_TAG_OPEN_FRAME_IN_NEW_WINDOW
	 * 85: EWK_CONTEXT_MENU_ITEM_TAG_TEXT_SELECTION_MODE
	 * 87: EWK_CONTEXT_MENU_ITEM_TAG_DRAG
	 * -----> Image link
	 *  1: EWK_CONTEXT_MENU_ITEM_TAG_OPEN_LINK_IN_NEW_WINDOW
	 *  2: EWK_CONTEXT_MENU_ITEM_TAG_DOWNLOAD_LINK_TO_DISK
	 *  3: EWK_CONTEXT_MENU_ITEM_TAG_COPY_LINK_TO_CLIPBOARD
	 *  4: EWK_CONTEXT_MENU_ITEM_TAG_OPEN_IMAGE_IN_NEW_WINDOW
	 *  5: EWK_CONTEXT_MENU_ITEM_TAG_DOWNLOAD_IMAGE_TO_DISK
	 *  6: EWK_CONTEXT_MENU_ITEM_TAG_COPY_IMAGE_TO_CLIPBOARD
	 *  7: EWK_CONTEXT_MENU_ITEM_TAG_OPEN_FRAME_IN_NEW_WINDOW
	 * 85: EWK_CONTEXT_MENU_ITEM_TAG_TEXT_SELECTION_MODE
	 * 87: EWK_CONTEXT_MENU_ITEM_TAG_DRAG
	 */

	for (i = 0; i < count; i++) {
		Ewk_Context_Menu_Item *item = ewk_context_menu_nth_item_get(contextmenu, i);
		Ewk_Context_Menu_Item_Tag tag = ewk_context_menu_item_tag_get(item);
		debug_log("menu item tag : %d", tag);

		if (tag == EWK_CONTEXT_MENU_ITEM_TAG_COPY_LINK_DATA) {
			debug_log("Copy link data tag: Number / Email");
			num_email = EINA_TRUE;
		} else if (tag == EWK_CONTEXT_MENU_ITEM_TAG_CLIPBOARD) {
			debug_log("Clipboard tag: Input field");
			return INPUT_FIELD;
		} else if (tag == EWK_CONTEXT_MENU_ITEM_TAG_COPY) {
			debug_log("Copy tag: Text");
			text = EINA_TRUE;
		} else if (tag == EWK_CONTEXT_MENU_ITEM_TAG_COPY_LINK_TO_CLIPBOARD) {
			debug_log("Copy link to clipboard: Link");
			link = EINA_TRUE;
		} else if (tag == EWK_CONTEXT_MENU_ITEM_TAG_OPEN_LINK_IN_NEW_WINDOW) {
			debug_log("Open link in new window tag: Link");
			link = EINA_TRUE;
		} else if (tag == EWK_CONTEXT_MENU_ITEM_TAG_DOWNLOAD_IMAGE_TO_DISK) {
			debug_log("Download image to disk: Image");
			image = EINA_TRUE;
		}
	}

	if (text && !link && !num_email) {
		debug_log("Text only");
		return TEXT_ONLY;
	}
	if ((link || num_email) && !image) {
		debug_log("Text link");
		return TEXT_LINK;
	}
	if (image && !link && !num_email) {
		debug_log("Image only");
		return IMAGE_ONLY;
	}
	if (image && link && !num_email) {
		debug_log("Image link");
		return IMAGE_LINK;
	}

	return UNKNOWN_MENU;
}

static void _webview_contextmenu_customize_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	retm_if(event_info == NULL, "Invalid parameter: event_info[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	int i = 0, count = 0;
	Ewk_Context_Menu *contextmenu = (Ewk_Context_Menu *)event_info;
	Ewk_Context_Menu_Item *menu_item = NULL;
	Ewk_Context_Menu_Item_Tag menu_item_tag = EWK_CONTEXT_MENU_ITEM_TAG_NO_ACTION;
	view->is_long_pressed = EINA_TRUE;

	context_menu_type menu_type = _get_menu_type(contextmenu);
	debug_log("menu_type=%d", menu_type);

	if (menu_type == UNKNOWN_MENU || menu_type == INPUT_FIELD)
		return;

	count = ewk_context_menu_item_count(contextmenu);
	debug_log("context menu item count:%d", count);

	for (i = 0; i < count; i++) {
		menu_item = ewk_context_menu_nth_item_get(contextmenu, 0);
		menu_item_tag = ewk_context_menu_item_tag_get(menu_item);
		debug_log("menu_item_tag in for: %d", menu_item_tag);
		ewk_context_menu_item_remove(contextmenu, menu_item);
	}

	if (menu_type == TEXT_ONLY) {
		debug_log("TEXT_ONLY");
		ewk_context_menu_item_append_as_action(contextmenu, EWK_CONTEXT_MENU_ITEM_TAG_SELECT_ALL, email_get_email_string("IDS_EMAIL_OPT_SELECT_ALL"), EINA_TRUE);
		ewk_context_menu_item_append_as_action(contextmenu, EWK_CONTEXT_MENU_ITEM_TAG_COPY, email_get_email_string("IDS_EMAIL_OPT_COPY"), EINA_TRUE);
		ewk_context_menu_item_append(contextmenu, CUSTOM_CONTEXT_MENU_ITEM_SHARE, email_get_email_string("IDS_EMAIL_OPT_SHARE"), NULL, EINA_TRUE);
		ewk_context_menu_item_append(contextmenu, CUSTOM_CONTEXT_MENU_ITEM_SMART_SEARCH, email_get_email_string("IDS_BR_OPT_WEB_SEARCH"), NULL, EINA_TRUE);
	}
	debug_leave();
}

static void _webview_contextmenu_selected_cb(void *data, Evas_Object *webview, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	retm_if(event_info == NULL, "Invalid parameter: event_info[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	Ewk_Context_Menu_Item *menu_item = (Ewk_Context_Menu_Item *)event_info;
	Ewk_Context_Menu_Item_Tag menu_item_tag = EWK_CONTEXT_MENU_ITEM_TAG_NO_ACTION;
	debug_log("menu_item : %d", menu_item);

	char *link_url = NULL;
	link_url = (char *)ewk_context_menu_item_link_url_get(menu_item);
	debug_secure("link_url(%s)", link_url);
	menu_item_tag = ewk_context_menu_item_tag_get(menu_item);

	switch (menu_item_tag) {
	case CUSTOM_CONTEXT_MENU_ITEM_SMART_SEARCH:
		debug_log("SMART_SEARCH");
		_viewer_launch_smart_search(view, link_url);
	break;

	case CUSTOM_CONTEXT_MENU_ITEM_SHARE:
	{
		debug_log("SHARE");
		const char *selected_text = NULL;
		char *share_text = NULL;

		selected_text = ewk_view_text_selection_text_get(view->webview);
		debug_secure("selected_text [%s]", selected_text);
		debug_secure("link_url [%s]", link_url);
		if (STR_LEN(selected_text) != 0) {
			share_text = g_strdup(selected_text);
			debug_secure("share_text [%s]", share_text);
			_viewer_share_text(view, share_text);
			free(share_text);
		}
	}
	break;

	default:
		debug_log("Unsupported item");
	break;
	}

	debug_leave();
}

static void _viewer_launch_smart_search(EmailViewerView *view, char *link_url)
{
	debug_enter();
	retm_if(link_url == NULL, "Invalid parameter: link_url[NULL]");

	int ret = 0;
	const char *selected_text = NULL;
	char *keyword = NULL;
	selected_text = ewk_view_text_selection_text_get(view->webview);
	debug_secure("selected_text [%s]", selected_text);
	debug_secure("link_url [%s]", link_url);

	int text_len = STR_LEN(selected_text);
	debug_log("size of selected_text: %d", text_len);
	if (text_len == 0) {
		keyword = g_strdup(link_url);
	} else if (text_len > SHARE_TEXT_MAX_LEN) {
		keyword = g_strndup(selected_text, SHARE_TEXT_MAX_LEN);
	} else {
		keyword = g_strdup(selected_text);
	}
	debug_secure("keyword [%s]", keyword);

	app_control_h app_control = NULL;
	ret = app_control_create(&app_control);
	debug_log("app_control_create: %d", ret);
	retm_if(app_control == NULL, "app_control create failed: app_control[NULL]");
	ret = app_control_set_launch_mode(app_control, APP_CONTROL_LAUNCH_MODE_GROUP);
	debug_log("app_control_set_launch_mode: %d", ret);
	ret = app_control_set_operation(app_control, APP_CONTROL_OPERATION_SEARCH);
	debug_log("app_control_set_operation: %d", ret);
	ret = app_control_add_extra_data(app_control, "http://tizen.org/appcontrol/data/keyword", keyword);
	debug_log("app_control_add_extra_data: %d", ret);

	ret = app_control_send_launch_request(app_control, NULL, NULL);
	debug_log("app_control_send_launch_request: %d", ret);
	if (ret != APP_CONTROL_ERROR_NONE) {
		notification_status_message_post(_("IDS_EMAIL_HEADER_UNABLE_TO_PERFORM_ACTION_ABB"));
	}

	ret = app_control_destroy(app_control);
	debug_log("app_control_destroy: %d", ret);
	g_free(keyword);
}

static void _viewer_share_text(EmailViewerView *view, char *share_text)
{
	debug_enter();

	int ret = 0;
	app_control_h app_control = NULL;
	ret = app_control_create(&app_control);
	debug_log("app_control_create: %p, %d", app_control, ret);
	retm_if((app_control == NULL || ret < 0), "app_control create failed: app_control[NULL] or ret[< 0]");

	ret = app_control_set_launch_mode(app_control, APP_CONTROL_LAUNCH_MODE_GROUP);
	debug_log("app_control_set_launch_mode: %d", ret);

	ret = app_control_set_operation(app_control, APP_CONTROL_OPERATION_SHARE_TEXT);
	debug_log("app_control_set_operation: %d", ret);

	int text_len = STR_LEN(share_text);
	char *selected_text = NULL;
	debug_log("size of share_text: %d", text_len);
	if (text_len > SHARE_TEXT_MAX_LEN) {
		selected_text = g_strndup(share_text, SHARE_TEXT_MAX_LEN);
		char str[EMAIL_BUFF_SIZE_1K] = { 0, };
		snprintf(str, sizeof(str), email_get_email_string("IDS_EMAIL_TPOP_MAXIMUM_MESSAGE_SIZE_EXCEEDED_ONLY_FIRST_PD_KB_WILL_BE_SHARED"), SHARE_TEXT_MAX_LEN / 1024);
		notification_status_message_post(str);
	} else {
		selected_text = g_strdup(share_text);
	}
	ret = app_control_add_extra_data(app_control, APP_CONTROL_DATA_TEXT, selected_text);
	debug_log("app_control_add_extra_data: %d", ret);

	ret = app_control_send_launch_request(app_control, NULL, NULL);
	debug_log("app_control_send_launch_request: %d", ret);
	if (ret != APP_CONTROL_ERROR_NONE) {
		notification_status_message_post(_("IDS_EMAIL_HEADER_UNABLE_TO_PERFORM_ACTION_ABB"));
	}

	ret = app_control_destroy(app_control);
	debug_log("app_control_destroy: %d", ret);
	g_free(selected_text);
}

void viewer_launch_download_manager(EmailViewerView *view, const char *download_uri, const char *cookie)
{
	debug_enter();
	retm_if(download_uri == NULL, "Invalid parameter: download_uri[NULL]");
	debug_secure("uri = [%s], cookie = [%s]", download_uri, cookie);

	int ret = 0;
	app_control_h app_control = NULL;

	ret = app_control_create(&app_control);
	debug_log("app_control_create: %p, %d", app_control, ret);
	retm_if((app_control == NULL || ret < 0), "app_control create failed: app_control[NULL] or ret[< 0]");

	ret = app_control_set_launch_mode(app_control, APP_CONTROL_LAUNCH_MODE_GROUP);
	debug_log("app_control_set_launch_mode: %d", ret);

	ret = app_control_set_operation(app_control, APP_CONTROL_OPERATION_DOWNLOAD);
	debug_log("app_control_set_operation: %d", ret);

	ret = app_control_set_uri(app_control, download_uri);
	debug_log("app_control_set_uri: %d", ret);

	if (cookie && strlen(cookie)) {
		ret = app_control_add_extra_data(app_control, "req_http_header_field", "cookie");
		debug_log("app_control_add_extra_data: %d", ret);

		ret = app_control_add_extra_data(app_control, "req_http_header_value", cookie);
		debug_log("app_control_add_extra_data: %d", ret);
	}

	ret = app_control_add_extra_data(app_control, "mode", "silent");
	debug_log("app_control_add_extra_data: %d", ret);

	const char *storage_type = "0"; /* "0": phone memory / "1": memory card */
	ret = app_control_add_extra_data(app_control, "default_storage", storage_type);
	debug_log("app_control_add_extra_data: %d", ret);

	ret = app_control_add_extra_data(app_control, "network_bonding", "true");
	debug_log("app_control_add_extra_data: %d", ret);

	ret = app_control_send_launch_request(app_control, NULL, NULL);
	debug_log("app_control_send_launch_request: %d", ret);

	app_control_destroy(app_control);
	debug_log("app_control_destroy: %d", ret);

	int noti_ret = notification_status_message_post(email_get_email_string("IDS_EMAIL_POP_DOWNLOADING_ING"));
	debug_warning_if(noti_ret != NOTIFICATION_ERROR_NONE, "notification_status_message_post() failed! ret:(%d)", noti_ret);
}

static void _webview_magnifier_show_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;
	view->is_magnifier_opened = EINA_TRUE;

	debug_leave();
}

static void _webview_magnifier_hide_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;
	view->is_magnifier_opened = EINA_FALSE;
	debug_leave();
}


/*
 * Definition for exported functions
 */

static Eina_Bool _webview_scroll_region_bringin_idler(void *data)
{
	debug_enter();

	retvm_if(data == NULL, ECORE_CALLBACK_CANCEL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;
	Evas_Coord sc_width = 0, sc_height = 0;
	Evas_Coord web_y = 0;

	view->idler_regionbringin = NULL;
	evas_object_geometry_get(view->webview_ly, NULL, &web_y, NULL, NULL);

	elm_scroller_region_get(view->scroller, 0, NULL, &sc_width, &sc_height);
	elm_scroller_region_bring_in(view->scroller, 0, web_y, sc_width, sc_height);
	debug_log("size_to_be_scrolled :%d", web_y);

	debug_leave();
	return ECORE_CALLBACK_CANCEL;
}

static void _webview_contextmenu_willshow_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;
	Evas_Point *pt = (Evas_Point *)event_info;

	Evas_Coord web_y = 0, top_coordinate_y = 0;
	evas_object_geometry_get(view->webview, NULL, &web_y, NULL, NULL);
	evas_object_geometry_get(view->scroller, NULL, &top_coordinate_y, NULL, NULL);

	debug_log("==> will show at [%d, %d]", pt->x, pt->y);

	if ((top_coordinate_y != web_y) && view->is_webview_text_selected) {
		DELETE_IDLER_OBJECT(view->idler_regionbringin);
		view->idler_regionbringin = ecore_idler_add(_webview_scroll_region_bringin_idler, view);
	}
	view->is_webview_text_selected = EINA_FALSE;

	debug_leave();
}

static void _webview_textselection_mode_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data || !event_info, "invalid data");

	EmailViewerView *view = (EmailViewerView *)data;
	bool *is_text_selection_mode = (bool *)event_info;
	debug_log("=> [%d]", *is_text_selection_mode);

	view->is_webview_text_selected = (Eina_Bool)*is_text_selection_mode;

	debug_leave();
}

void viewer_webkit_add_callbacks(void *data)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	if (view->webview) {
		evas_object_smart_callback_add(view->webview, "edge,top", _webview_edge_top_cb, view);
		evas_object_smart_callback_add(view->webview, "edge,bottom", _webview_edge_bottom_cb, view);
		evas_object_smart_callback_add(view->webview, "edge,left", _webview_edge_left_cb, view);
		evas_object_smart_callback_add(view->webview, "edge,right", _webview_edge_right_cb, view);

		evas_object_smart_callback_add(view->webview, "load,started", _webview_load_started_cb, view);
		evas_object_smart_callback_add(view->webview, "load,finished", _webview_load_finished_cb, view);
#ifndef _EWK_LOAD_NONEMPTY_LAYOUT_FINISHED_CB_UNAVAILABLE_
		evas_object_smart_callback_add(view->webview, "load,nonemptylayout,finished", _webview_load_noemptylayout_finished_cb, view);
#endif
		evas_object_smart_callback_add(view->webview, "load,progress", _webview_load_progress_cb, view);
		evas_object_smart_callback_add(view->webview, "load,error", _webview_load_error_cb, view);
		evas_object_smart_callback_add(view->webview, "policy,navigation,decide", _webview_policy_navigation_decide_cb, view);
		evas_object_smart_callback_add(view->webview, "policy,newwindow,decide", _webview_policy_newwindow_decide_cb, view);

		evas_object_smart_callback_add(view->webview, "contextmenu,customize", _webview_contextmenu_customize_cb, view);
		evas_object_smart_callback_add(view->webview, "contextmenu,selected", _webview_contextmenu_selected_cb, view);
		evas_object_smart_callback_add(view->webview, "contextmenu,willshow", _webview_contextmenu_willshow_cb, view);
		evas_object_smart_callback_add(view->webview, "textselection,mode", _webview_textselection_mode_cb, view);
		evas_object_smart_callback_add(view->webview, "magnifier,show", _webview_magnifier_show_cb, view);
		evas_object_smart_callback_add(view->webview, "magnifier,hide", _webview_magnifier_hide_cb, view);
		evas_object_smart_callback_add(view->webview, "zoom,started", _webview_zoom_started_cb, view);
		evas_object_smart_callback_add(view->webview, "zoom,finished", _webview_zoom_finished_cb, view);

		evas_object_event_callback_add(view->webview, EVAS_CALLBACK_MOUSE_DOWN, _webview_mouse_down_cb, view);
		evas_object_event_callback_add(view->webview, EVAS_CALLBACK_RESIZE, _webview_resize_cb, view);

		evas_object_smart_callback_add(view->webview_button, "focused", _webview_btn_focused, view);
		evas_object_smart_callback_add(view->webview_button, "unfocused", _webview_btn_unfocused, view);
#ifdef _WEBKIT_CONSOLE_MESSAGE_LOG
		evas_object_smart_callback_add(view->webview, "console,message", _webview_console_message, view);
#endif
	}
	debug_leave();
}

void viewer_webkit_del_callbacks(void *data)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	if (view->webview) {
		evas_object_smart_callback_del_full(view->webview, "edge,top", _webview_edge_top_cb, view);
		evas_object_smart_callback_del_full(view->webview, "edge,bottom", _webview_edge_bottom_cb, view);
		evas_object_smart_callback_del_full(view->webview, "edge,left", _webview_edge_left_cb, view);
		evas_object_smart_callback_del_full(view->webview, "edge,right", _webview_edge_right_cb, view);

		evas_object_smart_callback_del_full(view->webview, "load,started", _webview_load_started_cb, view);
		evas_object_smart_callback_del_full(view->webview, "load,finished", _webview_load_finished_cb, view);
#ifndef _EWK_LOAD_NONEMPTY_LAYOUT_FINISHED_CB_UNAVAILABLE_
		evas_object_smart_callback_del_full(view->webview, "load,nonemptylayout,finished", _webview_load_noemptylayout_finished_cb, view);
#endif
		evas_object_smart_callback_del_full(view->webview, "load,progress", _webview_load_progress_cb, view);
		evas_object_smart_callback_del_full(view->webview, "load,error", _webview_load_error_cb, view);
		evas_object_smart_callback_del_full(view->webview, "policy,navigation,decide", _webview_policy_navigation_decide_cb, view);
		evas_object_smart_callback_del_full(view->webview, "policy,newwindow,decide", _webview_policy_newwindow_decide_cb, view);

		evas_object_smart_callback_del_full(view->webview, "contextmenu,customize", _webview_contextmenu_customize_cb, view);
		evas_object_smart_callback_del_full(view->webview, "contextmenu,selected", _webview_contextmenu_selected_cb, view);
		evas_object_smart_callback_del_full(view->webview, "contextmenu,willshow", _webview_contextmenu_willshow_cb, view);
		evas_object_smart_callback_del_full(view->webview, "textselection,mode", _webview_textselection_mode_cb, view);
		evas_object_smart_callback_del_full(view->webview, "magnifier,show", _webview_magnifier_show_cb, view);
		evas_object_smart_callback_del_full(view->webview, "magnifier,hide", _webview_magnifier_hide_cb, view);
		evas_object_smart_callback_del_full(view->webview, "zoom,started", _webview_zoom_started_cb, view);
		evas_object_smart_callback_del_full(view->webview, "zoom,finished", _webview_zoom_finished_cb, view);

		evas_object_event_callback_del_full(view->webview, EVAS_CALLBACK_MOUSE_DOWN, _webview_mouse_down_cb, view);
		evas_object_event_callback_del_full(view->webview, EVAS_CALLBACK_RESIZE, _webview_resize_cb, view);

		evas_object_smart_callback_del_full(view->webview_button, "focused", _webview_btn_focused, view);
		evas_object_smart_callback_del_full(view->webview_button, "unfocused", _webview_btn_unfocused, view);
#ifdef _WEBKIT_CONSOLE_MESSAGE_LOG
		evas_object_smart_callback_del_full(view->webview, "console,message", _webview_console_message, view);
#endif
	}
	debug_leave();
}

static void _webview_btn_focused(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;
	if (view->webview) {
		if (!evas_object_focus_get(view->webview)) {
			evas_object_focus_set(view->webview, EINA_TRUE);
		}
	}

	debug_leave();
}

static void _webview_btn_unfocused(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailViewerView *view = (EmailViewerView *)data;
	evas_object_focus_set(view->webview, EINA_FALSE);

	debug_leave();
}

void _viewer_download_start_cb(const char *download_uri, void *data)
{
	debug_enter();
	debug_secure("download_uri = [%s]", download_uri);
	retm_if(!download_uri, "Invalid parameter: download_uri[NULL]");
	retm_if(!data, "Invalid parameter: data[NULL]");
	EmailViewerView *view = (EmailViewerView *)data;

	if (strncmp(download_uri, URI_SCHEME_HTTP, strlen(URI_SCHEME_HTTP)) && strncmp(download_uri, URI_SCHEME_HTTPS, strlen(URI_SCHEME_HTTPS))) {
		debug_log("Only http or https URLs can be downloaded");
		int noti_ret = notification_status_message_post(email_get_email_string("IDS_BR_POP_ONLY_HTTP_OR_HTTPS_URLS_CAN_BE_DOWNLOADED"));
		debug_warning_if(noti_ret != NOTIFICATION_ERROR_NONE, "notification_status_message_post() failed! ret:(%d)", noti_ret);
		return;
	} else {
		char *cookie = NULL;
		/*cookie = ewk_view_cookies_get(view->webview, download_uri);*/ /* ewk_view_cookies_get_for_url */
		viewer_launch_download_manager(view, download_uri, cookie); /* cookie can be NULL. */
	}
}

Evas_Object *viewer_get_webview(EmailViewerView *view)
{
	debug_enter();
	retvm_if(view == NULL, NULL, "Invalid parameter: view[NULL]");

	Evas_Object *ewk_view = ewk_view_add(evas_object_evas_get(view->webview_ly));

	/**
	 * To modify background color of webkit, following parts should be modified as well.
	 * 1. here
	 * 2. background-color in css
	 * 3. color_class of webview_bg part in edc file
	 */

	evas_object_color_set(ewk_view, COLOR_WHITE);
	if (elm_win_focus_highlight_enabled_get(view->base.module->win)) {
		elm_object_focus_allow_set(ewk_view, EINA_TRUE);
	} else {
		elm_object_focus_allow_set(ewk_view, EINA_FALSE);
	}
	view->webview = ewk_view;
	Ewk_Settings *ewk_settings = ewk_view_settings_get(view->webview);
	ewk_settings_extra_feature_set(ewk_settings, "selection,magnifier", EINA_FALSE);
	ewk_settings_extra_feature_set(ewk_settings, "scrollbar,visibility", EINA_FALSE);
	ewk_settings_auto_fitting_set(ewk_settings, EINA_FALSE);
	if (ewk_settings_loads_images_automatically_set(ewk_settings, view->b_show_remote_images) == EINA_FALSE) {
		/* ewk_settings_load_remote_images_set for remote image */
		debug_log("SET show images is FAILED!");
	}

	/*ewk_view_unfocus_allow_callback_set(view->webview, _viewer_webkit_unfocused_cb, view);*/
	evas_object_propagate_events_set(ewk_view, EINA_FALSE);

	evas_object_repeat_events_set(view->webview, EINA_FALSE);

	Ewk_Context *context = ewk_view_context_get(view->webview);
	ewk_context_did_start_download_callback_set(context, _viewer_download_start_cb, view);

	/* Double_Scroller */
	viewer_webkit_add_callbacks(view);

	int angle = elm_win_rotation_get(view->base.module->win);
	switch (angle) {
	case APP_DEVICE_ORIENTATION_90:
		ewk_view_orientation_send(view->webview, -90);
		break;
	case APP_DEVICE_ORIENTATION_270:
		ewk_view_orientation_send(view->webview, 90);
		break;
	case APP_DEVICE_ORIENTATION_180:
		ewk_view_orientation_send(view->webview, 180);
		break;
	case APP_DEVICE_ORIENTATION_0: /* fall through */
	default:
		ewk_view_orientation_send(view->webview, 0);
		break;
	}
	evas_object_show(view->webview);
	view->loading_progress = 0.0;

	debug_leave();
	return view->webview;
}

static char *_viewer_apply_template(const char *tpl_file, GHashTable *table)
{
	retvm_if(tpl_file == NULL, NULL, "file name is null");

	char *filebuf = email_get_buff_from_file(tpl_file, 0);
	retvm_if(filebuf == NULL, NULL, "file name is null");

	GSList *list = NULL;
	char *p = filebuf;
	char *prev = p;

	for (; *p != '\0'; p++) {
		if (*p == '$' && *(p + 1) == '{') {
			p += 2;
			char *end = strchr(p, '}');
			if (end && end > p) {
				*(p - 2) = '\0';
				list = g_slist_prepend(list, prev);
				char *key = g_strndup(p, end - p);
				char *value = g_hash_table_lookup(table, key);
				if (value) list = g_slist_prepend(list, value);
				if (key) g_free(key);
				p = end;
				prev = p + 1;
			}
		}
	}

	list = g_slist_prepend(list, prev);
	list = g_slist_reverse(list);

	int i;
	GSList *l = list;
	char *html = NULL;
	char **str_arr = (char **)malloc((g_slist_length(list) + 1) * sizeof(char *));
	if (str_arr) {
		for (i = 0; l != NULL; i++, l = g_slist_next(l)) {
			str_arr[i] = (char *)l->data;
		}
		str_arr[i] = NULL;
		html = g_strjoinv(NULL, str_arr);
		free(str_arr);
	}
	g_slist_free(list);
	g_free(filebuf);
	return html;
}

static void _viewer_remove_trailing_open_tag(char *html)
{
	if (html) {
		char *p = strrchr(html, '>');
		if (p) *(p + 1) = '\0';
	}
}

void viewer_set_webview_content(EmailViewerView *view)
{
	debug_enter();
	retm_if(view == NULL, "Invalid parameter: view[NULL]");
	retm_if(view->webview == NULL, "Invalid parameter: view->webview[NULL]");
	retm_if(view->webview_data == NULL, "Invalid parameter: view->webview_data[NULL]");

	debug_log("unset b_load_finished");
	view->b_load_finished = EINA_FALSE;

	EmailViewerWebview *wvd = view->webview_data;

	/* webview should be set to swallow part of webview_ly. If not, it will not
	 * display correctly when autofit off.
	 */
	elm_object_part_content_set(view->webview_ly, "elm.swallow.content", view->webview);

	/* ewk_settings_loads_images_automatically_set should be called before calling
	 * ewk_view_html_contents_set to show wide images correctly when autofit is on */
	Ewk_Settings *ewk_settings = ewk_view_settings_get(view->webview);

	bool show_image = view->b_show_remote_images ||
			viewer_check_body_download_status(view->body_download_status, EMAIL_BODY_DOWNLOAD_STATUS_IMAGES_DOWNLOADED);
	if (ewk_settings_loads_images_automatically_set(ewk_settings, show_image) == EINA_FALSE) { /* ewk_settings_load_remote_images_set for remote image */
		debug_log("SET show images is FAILED!");
	}

	char tmp_file_path[MAX_PATH_LEN] = { 0, };

	/* set content */
	if (wvd->body_type == BODY_TYPE_HTML) {
		retm_if(wvd->uri == NULL, "Invalid uri: wvd->uri [NULL]");
		/* Fix for issue - Sometimes html content of previous mail is shown
		 * Set the default html page if file size is 0 */
		struct stat statbuf = { 0 };
		int status = lstat(wvd->uri, &statbuf);
		if (!status) {
			debug_log("Total file size: %d", (int)statbuf.st_size);
			if ((int)statbuf.st_size == 0) {
				debug_log("Set URI with default html");
				char *file_stream = NULL;
				file_stream = email_get_buff_from_file(email_get_default_html_path(), 0);
				retm_if(!file_stream, "file_stream is NULL -allocation memory failed");

				snprintf(tmp_file_path, sizeof(tmp_file_path), "file://%s", email_get_default_html_path());
				debug_secure("tmp_file_path [%s]", tmp_file_path);
				/* If encoding(5th parameter) is missing, "UTF-8" is used to display contents. */
				ewk_view_contents_set(view->webview, file_stream, strlen(file_stream), NULL, NULL, tmp_file_path);
				/* Set ewk_view_vertical_panning_hold_set() here due to structural issue of Chromium */
				ewk_view_vertical_panning_hold_set(view->webview, EINA_TRUE);

				g_free(file_stream);
				return;
			}
		}
		snprintf(tmp_file_path, sizeof(tmp_file_path), "file://%s", wvd->uri);
		debug_secure("file://%s", wvd->uri);
	}

	GHashTable *hashtable = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	retm_if(hashtable == NULL, "hashtable is null");
	if (wvd->body_type == BODY_TYPE_TEXT) {
		/* generate temporary html file for viewing */
		retm_if(view->temp_viewer_html_file_path == NULL, "view->temp_viewer_html_file_path is NULL.");

		g_hash_table_insert(hashtable, g_strdup("BODY"), _viewer_convert_plain_text_body(wvd));
	}

	debug_log("body_type: %d", wvd->body_type);
	if (wvd->body_type == BODY_TYPE_HTML) {
		char *file_buffer = email_get_buff_from_file(wvd->uri, 0);
			_viewer_remove_trailing_open_tag(file_buffer);
		if (view->charset) {
			char *encoded_text = NULL;
			encoded_text = email_body_encoding_convert(file_buffer, view->charset, DEFAULT_CHARSET);
			if (encoded_text) {
				g_free(file_buffer);
				file_buffer = encoded_text;
			}
		}
		g_hash_table_insert(hashtable, g_strdup("BODY"), file_buffer);
	}

	char *file_stream = NULL;
	g_hash_table_insert(hashtable, g_strdup("CSSDIR"), g_strdup(email_get_misc_dir()));

	char basetime[100];
	if (view->mail_info)
		snprintf(basetime, sizeof(basetime), "%ld", view->mail_info->date_time);

	if (wvd->body_type == BODY_TYPE_TEXT) {
		file_stream = _viewer_apply_template(email_get_template_html_text_path(), hashtable);
	} else {
		file_stream = _viewer_apply_template(email_get_template_html_path(), hashtable);
	}
	g_hash_table_destroy(hashtable);
	hashtable = NULL;

	if (view->temp_viewer_html_file_path == NULL) {
		G_FREE(file_stream);
		retm_if(view->temp_viewer_html_file_path == NULL, "view->temp_viewer_html_file_path is NULL.");
	}

	if (email_file_exists(view->temp_viewer_html_file_path)) {
		int r = email_file_remove(view->temp_viewer_html_file_path);
		if (r == -1) {
			char err_buff[EMAIL_BUFF_SIZE_HUG] = { 0, };
			debug_error("remove(temp_viewer_html_file_path) failed! (%d): %s", r, strerror_r(errno, err_buff, sizeof(err_buff)));
		}
	}

	FILE *fp = fopen(view->temp_viewer_html_file_path, "w");
	if (fp != NULL) {
		if (file_stream != NULL)
			fputs(file_stream, fp);
		fclose(fp);
	}

	snprintf(tmp_file_path, sizeof(tmp_file_path), "file://%s", view->temp_viewer_html_file_path);
	debug_secure("tmp_file_path [%s]", tmp_file_path);
	size_t contents_size = 0;

	if (STR_VALID(file_stream)) {
		contents_size = STR_LEN(file_stream);
	}
	ewk_view_contents_set(view->webview, file_stream, contents_size, NULL, DEFAULT_CHARSET, tmp_file_path);
	/* Set ewk_view_vertical_panning_hold_set() here due to structural issue of Chromium */
	ewk_view_vertical_panning_hold_set(view->webview, EINA_TRUE);

	g_free(file_stream);
	file_stream = NULL;
	debug_leave();
}

static char *_viewer_convert_plain_text_body(EmailViewerWebview *wvd)
{
	debug_enter();
	retvm_if(wvd == NULL, NULL, "Invalid parameter: wvd[NULL]");

	char *html = NULL;

	char *email_content = wvd->text_content;
	char *charset = wvd->charset;

	char *encoded_text = NULL;
	encoded_text = email_body_encoding_convert(email_content, charset, DEFAULT_CHARSET);

	email_content = encoded_text ? encoded_text : email_content;

	html = email_get_parse_plain_text(email_content, STR_LEN(email_content));

	if (encoded_text) {
		g_free(encoded_text);
	}

	debug_leave();
	return html;
}

void viewer_webview_handle_mem_warning(EmailViewerView *view, bool hard)
{
	debug_enter();
	retm_if(!view || !view->webview, "no webview");

	Ewk_Context *context = ewk_view_context_get(view->webview);
	retm_if(!context, "no context");

	if (hard) {
		debug_log("hard warning");
		viewer_webview_handle_mem_warning(view, false);

		if (view->loading_progress > 0.5) {
			if (view->loading_progress < 1.0) {
				debug_log("stop page loading %.2f", view->loading_progress);
				ewk_view_stop(view->webview);
				_webview_load_finished_cb(view, NULL, NULL);
			}
		} else {
			debug_log("destroy me");
			notification_status_message_post(_("IDS_EMAIL_HEADER_UNABLE_TO_PERFORM_ACTION_ABB"));
			email_module_make_destroy_request(view->base.module);
		}
	} else {
		debug_log("soft warning");
		ewk_context_cache_clear(context);
		ewk_context_notify_low_memory(context);
	}
	debug_leave();
}
/* EOF */
