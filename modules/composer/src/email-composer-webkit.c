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

#include "email-composer.h"
#include "email-composer-util.h"
#include "email-composer-webkit.h"
#include "email-composer-recipient.h"
#include "email-composer-attachment.h"
#include "email-composer-predictive-search.h"
#include "email-composer-js.h"
#include "email-composer-initial-view.h"
#include "email-composer-initial-data.h"
#include "email-composer-launcher.h"
#include "email-composer-send-mail.h"
#include "email-composer-rich-text-toolbar.h"

typedef enum {
	CUSTOM_CONTEXT_MENU_ITEM_SMART_SEARCH = EWK_CONTEXT_MENU_ITEM_BASE_APPLICATION_TAG,
} custom_context_menu_item_tag;

#define _WEBKIT_CONSOLE_MESSAGE_LOG 0
#define _WEBKIT_TEXT_STYLE_STATE_CALLBACK_LOG_ON 0

/*
 * Declaration for static functions
 */

static void _ewk_view_mosue_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _ewk_view_focus_in_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _ewk_view_focus_out_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void _webkit_button_focused_cb(void *data, Evas_Object *obj, void *event_info);

static void _webkit_js_execute_set_focus_cb(Evas_Object *obj, const char *result, void *data);
static void _webkit_js_execute_insert_signature_cb(Evas_Object *obj, const char *result, void *data);
static void _webkit_js_execute_insert_original_mail_info_cb(Evas_Object *obj, const char *result, void *data);
static void _webkit_js_execute_init_composer_body_cb(Evas_Object *obj, const char *result, void *data);
static void _webkit_js_execute_is_checkbox_clicked_cb(Evas_Object *obj, const char *result, void *data);
static void _webkit_js_execute_check_body_layout_cb(Evas_Object *obj, const char *result, void *data);
static void _webkit_js_execute_get_initial_parent_content_cb(Evas_Object *obj, const char *result, void *data);
static void _webkit_js_execute_get_initial_new_message_content_cb(Evas_Object *obj, const char *result, void *data);
static void _webkit_js_execute_get_initial_body_content_cb(Evas_Object *o, const char *result, void *data);

static void _ewk_view_load_progress_cb(void *data, Evas_Object *obj, void *event_info);
static void _ewk_view_load_error_cb(void *data, Evas_Object *obj, void *event_info);
static void _ewk_view_load_nonemptylayout_finished_cb(void *data, Evas_Object *obj, void *event_info);
static void _ewk_view_policy_navigation_decide_cb(void *data, Evas_Object *obj, void *event_info);
static void _ewk_view_contextmenu_customize_cb(void *data, Evas_Object *webview, void *event_info);

/*static void _ewk_view_console_message(void *data, Evas_Object *obj, void *event_info);*/

static Evas_Object *_webkit_create_ewk_view(Evas_Object *parent, EmailComposerView *view);
static Eina_Bool _webkit_parse_text_style_changed_data(const char *res_string, FontStyleParams *params);

/*
 * Definition for static functions
 */

static void _webkit_button_focused_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	if (!evas_object_focus_get(view->ewk_view)) {
		evas_object_focus_set(view->ewk_view, EINA_TRUE);
		if (view->need_to_set_focus_with_js) {
			composer_webkit_set_focus_to_webview_with_js(view);
		}
	}
	view->need_to_set_focus_with_js = EINA_TRUE;

	debug_leave();
}

static void _webkit_js_execute_set_focus_cb(Evas_Object *obj, const char *result, void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");
	EmailComposerView *view = (EmailComposerView *)data;

	if (view->is_loading_popup) {
		DELETE_EVAS_OBJECT(view->composer_popup);
		view->is_loading_popup = EINA_FALSE;
	}

	elm_object_tree_focus_allow_set(view->composer_layout, EINA_TRUE);
	if (!elm_win_focus_highlight_enabled_get(view->base.module->win)) {
		elm_object_focus_allow_set(view->send_btn, EINA_FALSE);
	}

	if (!view->composer_popup && (obj == view->ewk_view)) { /* Removing focus in case of scheduled email*/
		composer_util_focus_set_focus(view, view->selected_widget);
	}

	view->is_ewk_ready = EINA_TRUE;

	if (!view->vcard_save_thread) {
		view->is_load_finished = EINA_TRUE;
		if (view->need_to_destroy_after_initializing) {
			view->is_save_in_drafts_clicked = EINA_TRUE; /* The action should be just like clicking save in drafts.*/
			composer_exit_composer_get_contents(view); /* Exit out of composer without any pop-up.*/
		}
	}

	debug_leave();
}

static void _webkit_js_execute_insert_signature_cb(Evas_Object *obj, const char *result, void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	if (((view->composer_type == RUN_COMPOSER_REPLY || view->composer_type == RUN_COMPOSER_REPLY_ALL) && view->account_info->original_account->options.reply_with_body) ||
		(view->composer_type == RUN_COMPOSER_FORWARD) || (view->composer_type == RUN_COMPOSER_FORWARD_ALL)) {
		char *original_mail_info = composer_initial_data_body_make_parent_mail_info(view);

		char **sp_info = g_strsplit(original_mail_info, "\"", -1);
		char *jo_info = g_strjoinv("\\\"", sp_info);

		char *to_be_inserted_mail_info = g_strdup_printf(EC_JS_INSERT_ORIGINAL_MAIL_INFO, jo_info);

		char to_be_inserted_original_message_bar[EMAIL_BUFF_SIZE_1K] = { 0, };

		snprintf(to_be_inserted_original_message_bar, sizeof(to_be_inserted_original_message_bar), EC_JS_INSERT_TEXT_TO_ORIGINAL_MESSAGE_BAR, COLOR_BLACK, email_get_email_string("IDS_EMAIL_BODY_INCLUDE_ORIGINAL_MESSAGE"));

		if (!ewk_view_script_execute(obj, to_be_inserted_original_message_bar, NULL, NULL)) {
			debug_error("EC_JS_INSERT_TEXT_TO_ORIGINAL_MESSAGE_BAR failed.");
		}

		if (!ewk_view_script_execute(obj, to_be_inserted_mail_info, _webkit_js_execute_insert_original_mail_info_cb, (void *)view)) {
			debug_error("EC_JS_INSERT_ORIGINAL_MAIL_INFO failed.");
		}

		g_free(to_be_inserted_mail_info);
		g_free(original_mail_info);
		g_free(jo_info);
		g_strfreev(sp_info);
	} else {
		if (!ewk_view_script_execute(obj, EC_JS_HAS_ORIGINAL_MESSAGE_BAR, _webkit_js_execute_check_body_layout_cb, (void *)view)) {
			debug_error("EC_JS_HAS_ORIGINAL_MESSAGE_BAR failed.");
		}
	}

	debug_leave();
}

static void _webkit_js_execute_insert_original_mail_info_cb(Evas_Object *obj, const char *result, void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	if (!ewk_view_script_execute(obj, EC_JS_HAS_ORIGINAL_MESSAGE_BAR, _webkit_js_execute_check_body_layout_cb, (void *)data)) {
		debug_error("EC_JS_HAS_ORIGINAL_MESSAGE_BAR failed.");
	}

	debug_leave();
}

static void _webkit_js_execute_init_composer_body_cb(Evas_Object *obj, const char *result, void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;
	Evas_Object *ewk_view = (Evas_Object *)obj;

	/* Create combined scroller */
	composer_initial_view_create_combined_scroller(view);

	elm_object_part_content_set(view->ewk_view_layout, "elm.swallow.content", view->ewk_view);

	composer_util_resize_webview_height(view);

	char set_image_src[EMAIL_BUFF_SIZE_1K] = { 0, };
	snprintf(set_image_src, sizeof(set_image_src), EC_JS_UPDATE_IMAGE_SOURCES, composer_util_file_get_temp_dirname());

	/* TODO: is this needed? the symbolic links for inline image were made on tmp_folder. */
	if (view->composer_type == RUN_COMPOSER_EDIT || view->composer_type == RUN_COMPOSER_REPLY ||
		view->composer_type == RUN_COMPOSER_REPLY_ALL || view->composer_type == RUN_COMPOSER_FORWARD || view->composer_type == RUN_COMPOSER_FORWARD_ALL) {
		if (!ewk_view_script_execute(ewk_view, set_image_src, NULL, NULL))
			debug_error("EC_JS_UPDATE_IMAGE_SOURCES failed.");
	}

	if (view->with_original_message) {
		if (!ewk_view_script_execute(ewk_view, EC_JS_GET_CONTENTS_FROM_ORG_MESSAGE, _webkit_js_execute_get_initial_parent_content_cb, (void *)view)) {
			debug_error("EC_JS_GET_CONTENTS_FROM_ORG_MESSAGE failed.");
		}
	} else {
		if (!ewk_view_script_execute(ewk_view, EC_JS_GET_CONTENTS_FROM_BODY, _webkit_js_execute_get_initial_body_content_cb, (void *)view)) {
			debug_error("EC_JS_GET_CONTENTS_FROM_BODY failed.");
		}
	}

	debug_leave();
}

static void _webkit_js_execute_is_checkbox_clicked_cb(Evas_Object *obj, const char *result, void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	if (!g_strcmp0(result, "true")) {
		view->is_checkbox_clicked = EINA_TRUE;
	} else {
		view->is_checkbox_clicked = EINA_FALSE;
	}

	debug_leave();
}

static void _webkit_js_execute_check_body_layout_cb(Evas_Object *obj, const char *result, void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	if (!g_strcmp0(result, "true")) {
		view->with_original_message = EINA_TRUE;
	}

	if (view->with_original_message) {
		if (!ewk_view_script_execute(obj, EC_JS_IS_CHECKBOX_CHECKED, _webkit_js_execute_is_checkbox_clicked_cb, (void *)view)) {
			debug_error("EC_JS_IS_CHECKBOX_CHECKED failed.");
		}

		char buf[EMAIL_BUFF_SIZE_1K] = { 0, };
		snprintf(buf, sizeof(buf), EC_JS_UPDATE_COLOR_OF_ORG_MESSAGE_BAR, COLOR_GRAY);
		if (!ewk_view_script_execute(view->ewk_view, buf, NULL, (void *)view)) {
			debug_error("EC_JS_UPDATE_COLOR_OF_ORG_MESSAGE_BAR failed.");
		}

		snprintf(buf, sizeof(buf), EC_JS_UPDATE_COLOR_OF_CHECKBOX, COLOR_BLUE);
		if (!ewk_view_script_execute(view->ewk_view, buf, NULL, (void *)view)) {
			debug_error("EC_JS_UPDATE_COLOR_OF_CHECKBOX failed.");
		}

		snprintf(buf, sizeof(buf), EC_JS_UPDATE_COLOR_OF_CHECKBOX_ICON, COLOR_WHITE);
		if (!ewk_view_script_execute(view->ewk_view, buf, NULL, (void *)view)) {
			debug_error("EC_JS_UPDATE_COLOR_OF_CHECKBOX_ICON failed.");
		}
	}

	if (!ewk_view_script_execute(obj, EC_JS_INITIALIZE_COMPOSER, _webkit_js_execute_init_composer_body_cb, (void *)view)) {
		debug_error("EC_JS_INITIALIZE_COMPOSER failed.");
	}

	debug_leave();
}

static void _webkit_js_execute_get_initial_parent_content_cb(Evas_Object *obj, const char *result, void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;
	Evas_Object *ewk_view = (Evas_Object *)obj;

	if (result) {
		view->initial_parent_content = g_strdup(result);
	}

	if (!ewk_view_script_execute(ewk_view, EC_JS_GET_CONTENTS_FROM_NEW_MESSAGE, _webkit_js_execute_get_initial_new_message_content_cb, (void *)view)) {
		debug_error("EC_JS_GET_CONTENTS_FROM_NEW_MESSAGE failed.");
	}

	debug_leave();
}

static void _webkit_js_execute_get_initial_new_message_content_cb(Evas_Object *obj, const char *result, void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;
	Evas_Object *ewk_view = (Evas_Object *)obj;

	if (result) {
		view->initial_new_message_content = g_strdup(result);
	}

	if (!ewk_view_script_execute(ewk_view, EC_JS_GET_CONTENTS_FROM_BODY, _webkit_js_execute_get_initial_body_content_cb, (void *)view)) {
		debug_error("EC_JS_GET_CONTENTS_FROM_BODY failed.");
	}

	debug_leave();
}

static void _webkit_js_execute_get_initial_body_content_cb(Evas_Object *obj, const char *result, void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;
	Evas_Object *ewk_view = (Evas_Object *)obj;

	if (result) {
		view->initial_body_content = g_strdup(result);
	}

	/* Incase of forward/reply a mail, we don't make body contenteditable to keep original message bar non editable state. */
	if (view->with_original_message) {
		if (!ewk_view_script_execute(ewk_view, EC_JS_ENABLE_EDITABLE_DIVS, _webkit_js_execute_set_focus_cb, (void *)view)) {
			debug_error("EC_JS_ENABLE_EDITABLE_DIVS failed!");
		}
	} else {
		if (!ewk_view_script_execute(ewk_view, EC_JS_ENABLE_EDITABLE_BODY, _webkit_js_execute_set_focus_cb, (void *)view)) {
			debug_error("EC_JS_ENABLE_EDITABLE_BODY failed!");
		}
	}

	debug_leave();
}

static void _ewk_view_mosue_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	if (view->allow_click_events && !elm_object_focus_get(view->ewk_btn)) {
		view->ewk_accepts_focus = EINA_TRUE;
	}
}

static void _ewk_view_focus_in_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	/* Menu ctxpopup and keypad is shown at the same time.
	 * Cause: Press menu hard key then press webview quickly.
	 * Menu ctxpopup comes up and keypad is shown because webview gets the focus.
	 */
	DELETE_EVAS_OBJECT(view->context_popup);

	/* If there's no focus on the webkit button, it'll set the focus to webkit again. */
	if (!elm_object_focus_get(view->ewk_btn)) {
		/* Idler used because recursion may do a nasty sings! */
		if (view->ewk_accepts_focus) {
			view->need_to_set_focus_with_js = EINA_FALSE;
			debug_log("reset focus");
			composer_util_focus_set_focus_with_idler(view, view->ewk_view);
		} else {
			debug_log("cancel focus");
			composer_util_focus_set_focus_with_idler(view, NULL);
		}
		debug_log("return");
		return;
	}

	view->ewk_accepts_focus = EINA_FALSE;

	if (composer_recipient_is_recipient_entry(view, view->selected_widget)) {
		if (!composer_recipient_commit_recipient_on_entry(view, view->selected_widget)) {

			if (view->richtext_toolbar) {
				composer_rich_text_disable_set(view, EINA_FALSE);
			}

			return;
		}
	}
	composer_recipient_unfocus_entry(view, view->selected_widget);
	if (view->bcc_added && !view->cc_recipients_cnt && !view->bcc_recipients_cnt) {
		composer_recipient_show_hide_bcc_field(view, EINA_FALSE);
	}

	/* This line(for selected_entry) should be here. (after composer_recipient_hide_contact_button()) */
	if (view->selected_widget != view->ewk_view) {
		composer_attachment_ui_contract_attachment_list(view);
		view->selected_widget = view->ewk_view;
	}

	if (view->richtext_toolbar) {
		composer_rich_text_disable_set(view, EINA_FALSE);
	}

	debug_leave();
}

static void _ewk_view_focus_out_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	view->cs_in_selection_mode = false;

	debug_leave();
}

static void _ewk_view_load_progress_cb(void *data, Evas_Object *obj, void *event_info)
{
	retm_if(!data, "Invalid parameter: data is NULL!");

	double *progress = (double *)event_info;
	if (progress) {
		debug_log("=> load,progress: (%f)", *progress);

		EmailComposerView *view = (EmailComposerView *)data;
		view->webview_load_progress = *progress;
	}
}

static void _ewk_view_load_error_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	Ewk_Error *error = (Ewk_Error *)event_info;
	const char *url = ewk_error_url_get(error);

	if ((g_strcmp0(url, email_get_phone_storage_url()) != 0) &&
		(g_strcmp0(url, email_get_mmc_storage_url()) != 0) &&
		(g_strcmp0(url, email_get_res_url()) != 0)) {
		/* NEVER BE HERE!! if this error popup was occured, contact webkit team with this error! It mustn't be occurred! */
		int error_code = ewk_error_code_get(error);
		const char *description = ewk_error_description_get(error);

		char buf[EMAIL_BUFF_SIZE_4K] = { 0, };
		snprintf(buf, sizeof(buf), "Error occured!<br>Url: (%s)<br>Code: (%d)<br>Description: (%s)<br><br>Please report us", url, error_code, description);

		debug_secure_error("%s", buf);
	}

	debug_leave();
}

static void _ewk_view_load_nonemptylayout_finished_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	/* In this case, ewk_view_script_execute() should not be called because it causes crash sometimes. script executed callback is called after composer is destroyed. */
	retm_if(view->is_low_memory_handled, "Composer will be destroyed because of low memory!");

	if (view->composer_type == RUN_COMPOSER_EDIT) {
		if (!ewk_view_script_execute(obj, EC_JS_HAS_ORIGINAL_MESSAGE_BAR, _webkit_js_execute_check_body_layout_cb, (void *)view)) {
			debug_warning("EC_JS_HAS_ORIGINAL_MESSAGE_BAR failed.");
		}
	} else {
		char *to_be_inserted_signature = NULL;
		char *signature = composer_initial_data_body_make_signature_markup(view);

		if (((view->composer_type == RUN_COMPOSER_REPLY || view->composer_type == RUN_COMPOSER_REPLY_ALL) && view->account_info->original_account->options.reply_with_body) ||
			(view->composer_type == RUN_COMPOSER_FORWARD) || (view->composer_type == RUN_COMPOSER_FORWARD_ALL)) {
			to_be_inserted_signature = g_strdup_printf(EC_JS_INSERT_SIGNATURE_TO_NEW_MESSAGE, signature);
		} else {
			to_be_inserted_signature = g_strdup_printf(EC_JS_INSERT_SIGNATURE, signature);
		}

		if (!ewk_view_script_execute(obj, to_be_inserted_signature, _webkit_js_execute_insert_signature_cb, (void *)view)) {
			debug_warning("EC_JS_INSERT_SIGNATURE failed.");
		}

		g_free(signature);
		g_free(to_be_inserted_signature);
	}

	debug_leave();
}

static Eina_Bool _webkit_parse_text_style_changed_data(const char *res_string, FontStyleParams *params)
{
	retvm_if(!res_string, EINA_FALSE, "res_string is NULL");

	int is_bold = 0;
	int is_italic = 0;
	int is_underline = 0;
	int is_ordered_list = 0;
	int is_unordered_list = 0;

	int res = sscanf(res_string, COMPOSER_TEXT_STYLE_CHANGE_MESSAGE,
			&params->font_size,
			&is_bold,
			&is_underline,
			&is_italic,
			&params->font_color.red, &params->font_color.green, &params->font_color.blue,
			&params->bg_color.red, &params->bg_color.green, &params->bg_color.blue,
			&is_ordered_list, &is_unordered_list);

	retvm_if(res < 0, EINA_FALSE, "sscanf failed!");

	params->is_bold = is_bold;
	params->is_underline = is_underline;
	params->is_italic = is_italic;
	params->is_ordered_list = is_ordered_list;
	params->is_unordered_list = is_unordered_list;

	return EINA_TRUE;
}

static void _ewk_view_policy_navigation_decide_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");
	retm_if(!event_info, "Invalid parameter: event_info is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;
	Evas_Object *ewk_view = (Evas_Object *)obj;

	if (view->is_load_finished) {
		Ewk_Policy_Decision *policy_decision = (Ewk_Policy_Decision *)event_info;

		debug_secure("url:%s", ewk_policy_decision_url_get(policy_decision));
		debug_secure("scheme:%s", ewk_policy_decision_scheme_get(policy_decision));
		debug_secure("cookie:%s", ewk_policy_decision_cookie_get(policy_decision));
		debug_secure("host:%s", ewk_policy_decision_host_get(policy_decision));
		debug_secure("response_mime:%s", ewk_policy_decision_response_mime_get(policy_decision));
		debug_secure("type: %d", ewk_policy_decision_type_get(policy_decision));

		ewk_policy_decision_ignore(policy_decision);

		const char *escaped_string = ewk_policy_decision_url_get(policy_decision);
		char *unescaped_string = g_uri_unescape_string(escaped_string, NULL);
		char *url = unescaped_string;
		char *save_ptr = NULL;

		if (g_str_has_prefix(url, COMPOSER_SCHEME_GROUP_EVENT)) {
			debug_secure("Got group event");
			url = strtok_r(url + strlen(COMPOSER_SCHEME_GROUP_EVENT), ";", &save_ptr);
		}

		while (url) {
			debug_secure("===> processed url: (%s)", url);
			if (g_str_has_prefix(url, COMPOSER_SCHEME_RESIZE_START)) {
				composer_initial_view_cs_freeze_push(view);
			} else if (g_str_has_prefix(url, COMPOSER_SCHEME_RESIZE_END)) {
				composer_initial_view_cs_freeze_pop(view);
			} else if (g_str_has_prefix(url, COMPOSER_SCHEME_START_DRAG)) {
				Ewk_Settings *ewkSetting = ewk_view_settings_get(ewk_view);
				ewk_settings_uses_keypad_without_user_action_set(ewkSetting, EINA_FALSE); /* To not show IME without user action */
			} else if (g_str_has_prefix(url, COMPOSER_SCHEME_STOP_DRAG)) {
				Ewk_Settings *ewkSetting = ewk_view_settings_get(ewk_view);
				ewk_settings_uses_keypad_without_user_action_set(ewkSetting, EINA_TRUE);
			} else if (g_str_has_prefix(url, COMPOSER_SCHEME_CLICK_CHECKBOX)) {
				char buf[EMAIL_BUFF_SIZE_1K] = { 0, };
				snprintf(buf, sizeof(buf), "%s", url + strlen(COMPOSER_SCHEME_CLICK_CHECKBOX));
				debug_log("click status: (%s)", buf);

				view->is_checkbox_clicked = atoi(buf);
				if (view->is_checkbox_clicked) {
					snprintf(buf, sizeof(buf), EC_JS_UPDATE_COLOR_OF_CHECKBOX, COLOR_BLUE);
				} else {
					snprintf(buf, sizeof(buf), EC_JS_UPDATE_COLOR_OF_CHECKBOX, COLOR_DARK_GRAY);
				}

				if (!ewk_view_script_execute(view->ewk_view, buf, NULL, (void *)view)) {
					debug_error("EC_JS_UPDATE_COLOR_OF_CHECKBOX failed.");
				}
				/* Not to move the focus to new meesage div when focused ui is enabled. */
				if (!elm_win_focus_highlight_enabled_get(view->base.module->win) && !view->is_focus_on_new_message_div) {
					if (ewk_view_script_execute(view->ewk_view, EC_JS_SET_FOCUS_NEW_MESSAGE, NULL, view) == EINA_FALSE) {
						debug_error("EC_JS_SET_FOCUS_NEW_MESSAGE is failed!");
					}
				}
				edje_object_signal_emit(_EDJ(view->composer_layout), "ec,action,clicked,layout", "");
			} else if (g_str_has_prefix(url, COMPOSER_SCHEME_GET_FOCUS_NEW)) {
				view->is_focus_on_new_message_div = EINA_TRUE;
			} else if (g_str_has_prefix(url, COMPOSER_SCHEME_GET_FOCUS_ORG)) {
				view->is_focus_on_new_message_div = EINA_FALSE;
			} else if (g_str_has_prefix(url, COMPOSER_SCHEME_EXCEENDED_MAX)) {
				char buf[EMAIL_BUFF_SIZE_1K] = { 0, };
				snprintf(buf, sizeof(buf), email_get_email_string("IDS_EMAIL_TPOP_MAXIMUM_MESSAGE_SIZE_HPD_KB_REACHED"), MAX_MESSAGE_SIZE);
				int noti_ret = notification_status_message_post(buf);
				debug_warning_if(noti_ret != NOTIFICATION_ERROR_NONE, "notification_status_message_post() failed! ret:(%d)", noti_ret);
			} else if (g_str_has_prefix(url, COMPOSER_SCHEME_TEXT_STYLE_CHANGE)) {
				if (view->richtext_toolbar) {
					FontStyleParams params = { 0 };
					if (_webkit_parse_text_style_changed_data(url, &params)) {
						composer_rich_text_font_style_params_set(view, &params);
					} else {
						debug_error("_webkit_parse_text_style_changed_data failed!");
					}
				}
			} else if (g_str_has_prefix(url, COMPOSER_SCHEME_CARET_POS_CHANGE)) {
				if (elm_object_focus_get(view->ewk_btn)) {
					int x = 0;
					int top = 0;
					int bottom = 0;
					int isCollapsed = 0;
					if (sscanf(url + strlen(COMPOSER_SCHEME_CARET_POS_CHANGE), "%d%d%d%d", &x, &top, &bottom, &isCollapsed) == 4) {
						composer_initial_view_caret_pos_changed_cb(view, top, bottom, (isCollapsed != 0));
					}
				} else {
					debug_log("Not in focus");
				}
			}
			url = (save_ptr ? strtok_r(NULL, ";", &save_ptr) : NULL);
		}

		g_free(unescaped_string);
	}

	debug_leave();
}

static void _ewk_view_contextmenu_customize_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(data == NULL, "Invalid parameter: data is NULL!");
	retm_if(event_info == NULL, "Invalid parameter: event_info is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;
	Ewk_Context_Menu *contextmenu = (Ewk_Context_Menu *)event_info;

	if (view->selected_widget != view->ewk_view) {
		elm_object_focus_set(view->ewk_btn, EINA_TRUE);
	}

	int i = 0, count = 0;
	Eina_Bool has_copy = EINA_FALSE;
	Eina_Bool has_select = EINA_FALSE;
	Eina_Bool has_select_all = EINA_FALSE;
	Ewk_Context_Menu_Item *menu_item = NULL;
	Ewk_Context_Menu_Item_Tag menu_item_tag = EWK_CONTEXT_MENU_ITEM_TAG_NO_ACTION;

	/* 2013. 11. 15. original context menu list
	 * -----> No contents
	 * 14: EWK_CONTEXT_MENU_ITEM_TAG_PASTE
	 * 86: EWK_CONTEXT_MENU_ITEM_TAG_CLIPBOARD
	 * -----> No selection
	 * 84: EWK_CONTEXT_MENU_ITEM_TAG_SELECT_WORD
	 * 83: EWK_CONTEXT_MENU_ITEM_TAG_SELECT_ALL
	 * 14: EWK_CONTEXT_MENU_ITEM_TAG_PASTE
	 * 86: EWK_CONTEXT_MENU_ITEM_TAG_CLIPBOARD
	 * -----> Select/Select all
	 * 84: EWK_CONTEXT_MENU_ITEM_TAG_SELECT_WORD
	 * 83: EWK_CONTEXT_MENU_ITEM_TAG_SELECT_ALL
	 *  8: EWK_CONTEXT_MENU_ITEM_TAG_COPY
	 * 13: EWK_CONTEXT_MENU_ITEM_TAG_CUT
	 * 14: EWK_CONTEXT_MENU_ITEM_TAG_PASTE
	 * 86: EWK_CONTEXT_MENU_ITEM_TAG_CLIPBOARD
	 * 21: EWK_CONTEXT_MENU_ITEM_TAG_SEARCH_WEB
	 * 88: EWK_CONTEXT_MENU_ITEM_TAG_TRANSLATE
	 * -----> Select space
	 * 84: EWK_CONTEXT_MENU_ITEM_TAG_SELECT_WORD
	 * 83: EWK_CONTEXT_MENU_ITEM_TAG_SELECT_ALL
	 *  8: EWK_CONTEXT_MENU_ITEM_TAG_COPY
	 * 13: EWK_CONTEXT_MENU_ITEM_TAG_CUT
	 * 14: EWK_CONTEXT_MENU_ITEM_TAG_PASTE
	 * 86: EWK_CONTEXT_MENU_ITEM_TAG_CLIPBOARD
	 * 88: EWK_CONTEXT_MENU_ITEM_TAG_TRANSLATE
	 */

	count = ewk_context_menu_item_count(contextmenu);
	for (i = 0; i < count; i++) {
		menu_item = ewk_context_menu_nth_item_get(contextmenu, 0);
		menu_item_tag = ewk_context_menu_item_tag_get(menu_item);
		/*debug_log("menu_item_tag[%d] : %d", i, menu_item_tag);*/
		if (menu_item_tag == EWK_CONTEXT_MENU_ITEM_TAG_COPY) {
			has_copy = EINA_TRUE; /* It's selection mode. */
		} else if (menu_item_tag == EWK_CONTEXT_MENU_ITEM_TAG_SELECT_ALL) {
			has_select_all = EINA_TRUE;
		} else if (menu_item_tag == EWK_CONTEXT_MENU_ITEM_TAG_SELECT_WORD) {
			has_select = EINA_TRUE;
		}
		ewk_context_menu_item_remove(contextmenu, menu_item);
	}

	debug_log("has_copy:[%d], has_select_all:%d, has_select:%d", has_copy, has_select_all, has_select);
	if (has_copy) { /* It's selection mode. */
		if (has_select_all) {
			ewk_context_menu_item_append_as_action(contextmenu, EWK_CONTEXT_MENU_ITEM_TAG_SELECT_ALL, email_get_email_string("IDS_EMAIL_OPT_SELECT_ALL"), EINA_TRUE);
		}
		ewk_context_menu_item_append_as_action(contextmenu, EWK_CONTEXT_MENU_ITEM_TAG_COPY, email_get_email_string("IDS_EMAIL_OPT_COPY"), EINA_TRUE);
		ewk_context_menu_item_append_as_action(contextmenu, EWK_CONTEXT_MENU_ITEM_TAG_CUT, email_get_email_string("IDS_EMAIL_OPT_CUT"), EINA_TRUE);
		ewk_context_menu_item_append_as_action(contextmenu, EWK_CONTEXT_MENU_ITEM_TAG_PASTE, email_get_email_string("IDS_EMAIL_OPT_PASTE"), EINA_TRUE);
		ewk_context_menu_item_append_as_action(contextmenu, EWK_CONTEXT_MENU_ITEM_TAG_CLIPBOARD, email_get_email_string("IDS_EMAIL_OPT_CLIPBOARD"), EINA_TRUE);
	} else {
		if (has_select) {
			ewk_context_menu_item_append_as_action(contextmenu, EWK_CONTEXT_MENU_ITEM_TAG_SELECT_WORD, email_get_email_string("IDS_EMAIL_OPT_SELECT"), EINA_TRUE);
		}
		if (has_select_all) {
			ewk_context_menu_item_append_as_action(contextmenu, EWK_CONTEXT_MENU_ITEM_TAG_SELECT_ALL, email_get_email_string("IDS_EMAIL_OPT_SELECT_ALL"), EINA_TRUE);
		}
		ewk_context_menu_item_append_as_action(contextmenu, EWK_CONTEXT_MENU_ITEM_TAG_PASTE, email_get_email_string("IDS_EMAIL_OPT_PASTE"), EINA_TRUE);
		ewk_context_menu_item_append_as_action(contextmenu, EWK_CONTEXT_MENU_ITEM_TAG_CLIPBOARD, email_get_email_string("IDS_EMAIL_OPT_CLIPBOARD"), EINA_TRUE);
	}

	composer_initial_view_activate_selection_mode(view);

	debug_leave();
}

#if (_WEBKIT_CONSOLE_MESSAGE_LOG)
static void _ewk_view_console_message(void *data, Evas_Object *obj, void *event_info)
{
	retm_if(event_info == NULL, "Invalid parameter: event_info is NULL!");

	Ewk_Console_Message *msg = (Ewk_Console_Message *)event_info;

	const char *log = ewk_console_message_text_get(msg);
	unsigned line = ewk_console_message_line_get(msg);

	debug_log("ConMsg:%d> %s", line, log);
}
#endif

static Evas_Object *_webkit_create_ewk_view(Evas_Object *parent, EmailComposerView *view)
{
	debug_enter();

	email_profiling_begin(_webkit_create_ewk_view);

	Evas_Object *ewk_view = ewk_view_add(evas_object_evas_get(parent));
	evas_object_size_hint_weight_set(ewk_view, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ewk_view, EVAS_HINT_FILL, EVAS_HINT_FILL);
	ewk_view_vertical_panning_hold_set(ewk_view, EINA_TRUE);
	/*evas_object_propagate_events_set(ewkview, EINA_FALSE);*/
	ewk_view_split_scroll_overflow_enabled_set(ewk_view, EINA_TRUE);

	/* To modify background color of webkit, following parts should be modified as well.
	 * 1. here / 2. background-color in css / 3. color_class of webview_bg part in edc file
	 */
	evas_object_color_set(ewk_view, COLOR_WHITE); /* This color is used to set the BG color for webview before webkit gets background color from js or html(setBackgroundColor in webkit). */
	ewk_view_draws_transparent_background_set(ewk_view, EINA_TRUE); /* The default color of webview tiles is white. This API makes the BG transparent. */

	Ewk_Settings *ewkSetting = ewk_view_settings_get(ewk_view);
	debug_warning_if(!ewkSetting, "ewk_view_settings_get() failed!");

	/* NOTE: for "touch,focus"
	 * Webview steal the focus even we just touch the area of webview to scroll the screen. (mouse down event)
	 * It's a concept of webview. "touch,focus" feature will disable this concept.
	 * In this case, we have to set focus to webview manually.
	 */
	ewk_settings_extra_feature_set(ewkSetting, "touch,focus", EINA_FALSE);
	/* NOTE: for "selection,magnifier"
	 * This feature will prevent weview loading magnifier when we long press webview.
	 * There's the same concept in the latest android modle. (Note4)
	 */
	ewk_settings_extra_feature_set(ewkSetting, "selection,magnifier", EINA_FALSE);
	/* NOTE: for "scrollbar,visibility"
	 * This feature will prevent weview loading scrollbar when we scroll it.
	 */
	ewk_settings_extra_feature_set(ewkSetting, "scrollbar,visibility", EINA_FALSE);
	ewk_settings_uses_keypad_without_user_action_set(ewkSetting, EINA_TRUE);
	ewk_settings_select_word_automatically_set(ewkSetting, EINA_TRUE);
	ewk_settings_auto_fitting_set(ewkSetting, EINA_FALSE);
	evas_object_show(ewk_view);

	email_profiling_end(_webkit_create_ewk_view);
	debug_leave();
	return ewk_view;
}

/*
 * Definition for exported functions
 */

void composer_webkit_create_body_field(Evas_Object *parent, EmailComposerView *view)
{
	email_profiling_begin(composer_webkit_create_body_field);
	debug_enter();

	retm_if(!parent, "parent is NULL!");
	retm_if(!view, "view is NULL!");

	Evas_Object *ewk_view_layout = composer_util_create_layout_with_noindicator(parent);
	Evas_Object *ewk_view = _webkit_create_ewk_view(ewk_view_layout, view);
	retm_if(!ewk_view, "_webkit_create_ewk_view() failed!");

	/* The webkit object will be set to its layout on the composer view in "load,nonemptylayout,finished" callback */
	elm_object_part_content_set(parent, "ec.swallow.webview", ewk_view_layout);
	view->ewk_view = ewk_view;
	view->ewk_view_layout = ewk_view_layout;

	composer_webkit_add_callbacks(view->ewk_view, view);

	Evas_Object *ewk_btn = elm_button_add(view->base.module->navi);
	elm_object_style_set(ewk_btn, "transparent");
	evas_object_freeze_events_set(ewk_btn, EINA_TRUE);
	evas_object_size_hint_weight_set(ewk_btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ewk_btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(ewk_btn);
	elm_object_part_content_set(view->base.content, "ec.swallow.webview.button", ewk_btn);

	view->ewk_btn = ewk_btn;

	evas_object_smart_callback_add(view->ewk_btn, "focused", _webkit_button_focused_cb, view);

	composer_webkit_update_orientation(view);

	debug_leave();
	email_profiling_end(composer_webkit_create_body_field);
}

void composer_webkit_add_callbacks(Evas_Object *ewk_view, void *data)
{
	debug_enter();

	retm_if(!ewk_view, "Invalid parameter: ewk_view is NULL!");
	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	evas_object_event_callback_add(ewk_view, EVAS_CALLBACK_FOCUS_IN, _ewk_view_focus_in_cb, view);
	evas_object_event_callback_add(ewk_view, EVAS_CALLBACK_FOCUS_OUT, _ewk_view_focus_out_cb, view);
	evas_object_event_callback_add(ewk_view, EVAS_CALLBACK_MOUSE_UP, _ewk_view_mosue_up_cb, view);

	evas_object_smart_callback_add(ewk_view, "load,progress", _ewk_view_load_progress_cb, view);
	evas_object_smart_callback_add(ewk_view, "load,error", _ewk_view_load_error_cb, view);

#ifndef _EWK_LOAD_NONEMPTY_LAYOUT_FINISHED_CB_UNAVAILABLE_
	evas_object_smart_callback_add(ewk_view, "load,nonemptylayout,finished", _ewk_view_load_nonemptylayout_finished_cb, view);
#else
	evas_object_smart_callback_add(ewk_view, "load,finished", _ewk_view_load_nonemptylayout_finished_cb, view);
#endif

	evas_object_smart_callback_add(ewk_view, "policy,navigation,decide", _ewk_view_policy_navigation_decide_cb, view);
	evas_object_smart_callback_add(ewk_view, "contextmenu,customize", _ewk_view_contextmenu_customize_cb, view);

#if (_WEBKIT_CONSOLE_MESSAGE_LOG)
	evas_object_smart_callback_add(ewk_view, "console,message", _ewk_view_console_message, view);
#endif

	debug_leave();
}

void composer_webkit_del_callbacks(Evas_Object *ewk_view, void *data)
{
	debug_enter();

	retm_if(!ewk_view, "Invalid parameter: ewk_view is NULL!");
	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	evas_object_event_callback_del_full(ewk_view, EVAS_CALLBACK_FOCUS_IN, _ewk_view_focus_in_cb, view);
	evas_object_event_callback_del_full(ewk_view, EVAS_CALLBACK_FOCUS_OUT, _ewk_view_focus_out_cb, view);
	evas_object_event_callback_del_full(ewk_view, EVAS_CALLBACK_MOUSE_UP, _ewk_view_mosue_up_cb, view);

	evas_object_smart_callback_del_full(ewk_view, "load,progress", _ewk_view_load_progress_cb, view);
	evas_object_smart_callback_del_full(ewk_view, "load,error", _ewk_view_load_error_cb, view);

#ifndef _EWK_LOAD_NONEMPTY_LAYOUT_FINISHED_CB_UNAVAILABLE_
	evas_object_smart_callback_del_full(ewk_view, "load,nonemptylayout,finished", _ewk_view_load_nonemptylayout_finished_cb, view);
#else
	evas_object_smart_callback_del_full(ewk_view, "load,finished", _ewk_view_load_nonemptylayout_finished_cb, view);
#endif

	evas_object_smart_callback_del_full(ewk_view, "policy,navigation,decide", _ewk_view_policy_navigation_decide_cb, view);
	evas_object_smart_callback_del_full(ewk_view, "contextmenu,customize", _ewk_view_contextmenu_customize_cb, view);

#if (_WEBKIT_CONSOLE_MESSAGE_LOG)
	evas_object_smart_callback_del_full(ewk_view, "console,message", _ewk_view_console_message, view);
#endif

	debug_leave();
}

void composer_webkit_blur_webkit_focus(void *data)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	if (view->with_original_message) {
		if (view->is_focus_on_new_message_div) {
			if (ewk_view_script_execute(view->ewk_view, EC_JS_SET_UNFOCUS_NEW_MESSAGE, NULL, view) == EINA_FALSE) {
				debug_error("EC_JS_SET_UNFOCUS_NEW_MESSAGE is failed!");
			}
		} else {
			if (ewk_view_script_execute(view->ewk_view, EC_JS_SET_UNFOCUS_ORG_MESSAGE, NULL, view) == EINA_FALSE) {
				debug_error("EC_JS_SET_UNFOCUS_ORG_MESSAGE is failed!");
			}
		}
	} else {
		if (ewk_view_script_execute(view->ewk_view, EC_JS_SET_UNFOCUS, NULL, view) == EINA_FALSE) {
			debug_error("EC_JS_SET_UNFOCUS is failed!");
		}
	}

	debug_leave();
}

void composer_webkit_handle_mem_warning(void *data, Eina_Bool is_hard)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!);");

	EmailComposerView *view = (EmailComposerView *)data;
	retm_if(!view->ewk_view, "No webview!");

	if (is_hard) {
		composer_webkit_handle_mem_warning(view, EINA_FALSE); /* To do action for soft warning. */

		Eina_Bool ret = ewk_view_stop(view->ewk_view);
		debug_warning_if(!ret, "ewk_view_stop() failed!");

		if (view->webview_load_progress > 0.5) {
			if (view->webview_load_progress < 1.0) {
				_ewk_view_load_nonemptylayout_finished_cb(view, view->ewk_view, NULL);
			}
		} else {
			view->is_low_memory_handled = EINA_TRUE;

			int noti_ret = notification_status_message_post(email_get_email_string("IDS_EMAIL_HEADER_UNABLE_TO_PERFORM_ACTION_ABB"));
			debug_warning_if(noti_ret != NOTIFICATION_ERROR_NONE, "notification_status_message_post() failed! ret:(%d)", noti_ret);

			/* Restore the indicator mode. */
			composer_util_indicator_restore(view);

			email_module_make_destroy_request(view->base.module);
		}
	} else {
		Ewk_Context *context = ewk_view_context_get(view->ewk_view);
		retm_if(!context, "ewk_view_context_get() failed!");

		ewk_context_cache_clear(context);
		ewk_context_notify_low_memory(context);
	}

	debug_leave();
}

void composer_webkit_set_focus_to_webview_with_js(void *data)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	if (view->with_original_message) {
		if (view->is_focus_on_new_message_div) {
			if (ewk_view_script_execute(view->ewk_view, EC_JS_SET_FOCUS_NEW_MESSAGE, NULL, NULL) == EINA_FALSE) {
				debug_error("EC_JS_SET_FOCUS_NEW_MESSAGE is failed!");
			}
		} else {
			if (ewk_view_script_execute(view->ewk_view, EC_JS_SET_FOCUS_ORG_MESSAGE, NULL, NULL) == EINA_FALSE) {
				debug_error("EC_JS_SET_FOCUS_ORG_MESSAGE is failed!");
			}
		}
	} else {
		if (ewk_view_script_execute(view->ewk_view, EC_JS_SET_FOCUS, NULL, NULL) == EINA_FALSE) {
			debug_error("EC_JS_SET_FOCUS is failed!");
		}
	}

	debug_leave();
}

void composer_webkit_update_orientation(EmailComposerView *view)
{
	switch (view->base.orientation) {
	case APP_DEVICE_ORIENTATION_0:
		view->is_horizontal = EINA_FALSE;
		ewk_view_orientation_send(view->ewk_view, 0);
		break;
	case APP_DEVICE_ORIENTATION_90:
		view->is_horizontal = EINA_TRUE;
		ewk_view_orientation_send(view->ewk_view, -90);
		break;
	case APP_DEVICE_ORIENTATION_180:
		view->is_horizontal = EINA_FALSE;
		ewk_view_orientation_send(view->ewk_view, 180);
		break;
	case APP_DEVICE_ORIENTATION_270:
		view->is_horizontal = EINA_TRUE;
		ewk_view_orientation_send(view->ewk_view, 90);
		break;
	}
}
