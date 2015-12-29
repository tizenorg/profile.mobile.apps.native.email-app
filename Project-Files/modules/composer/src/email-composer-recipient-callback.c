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

#include <string.h>
#include <notification.h>

#include "email-popup-utils.h"
#include "email-composer.h"
#include "email-composer-recipient.h"
#include "email-composer-attachment.h"
#include "email-composer-webkit.h"
#include "email-composer-util.h"
#include "email-composer-launcher.h"
#include "email-composer-predictive-search.h"
#include "email-composer-subject.h"
#include "email-composer-js.h"
#include "email-composer-more-menu.h"
#include "email-composer-rich-text-toolbar.h"

/*
 * Declaration for static functions
 */

#ifdef FEATRUE_PREDICTIVE_SEARCH
static void _request_predictive_search(EmailComposerUGD *ugd);
#endif
static char *_select_address_gl_text_get(void *data, Evas_Object *obj, const char *part);
static void _recipient_mbe_popup_delete(EmailComposerUGD *ugd);
static void _recipient_mbe_popup_edit(EmailComposerUGD *ugd);
static Eina_Bool _recipient_mbe_move_recipient_destroy_popup_idler_cb(void *data);
static void _recipient_mbe_popup_move_recipient(EmailComposerUGD *ugd, Evas_Object *dest_mbe);
static void _recipient_mbe_popup_add_to_contact(EmailComposerUGD *ugd);
static void _recipient_mbe_popup_update_contact(EmailComposerUGD *ugd);

static char *_add_to_contact_popup_gl_text_get(void *data, Evas_Object *obj, const char *part);
static void _add_to_contact_popup_gl_sel(void *data, Evas_Object *obj, void *event_info);
static void _recipient_mbe_popup_add_to_contact_selection_popup(void *data);
static  Evas_Object *_recipient_mbe_get(const Evas_Object *object, const EmailComposerUGD *ugd);
static Eina_Bool _recipient_mbe_unselect_last_item(Evas_Object *object, EmailComposerUGD *ugd);
static Eina_Bool _recipient_mbe_modify_last_item(Evas_Object *object, EmailComposerUGD *ugd);

/*static Evas_Object *_recipient_mbe_dnd_icon_create_cb(void *data, Evas_Object *win, Evas_Coord *xoff, Evas_Coord *yoff);
static void _recipient_mbe_dnd_accept_cb(void *data, Evas_Object *obj, Eina_Bool doaccept);
static void _recipient_mbe_dnd_dragdone_cb(void *data, Evas_Object *obj);
static Eina_Bool _recipient_mbe_dnd_dragdone_reset_cb(void *data);*/

static email_string_t EMAIL_COMPOSER_STRING_NULL = { NULL, NULL };

static bool edit_mode = false;
static Elm_Genlist_Item_Class select_address_itc;
static Elm_Genlist_Item_Class add_to_contact_popup_itc;

enum {
	RECP_SELECT_MENU_REMOVE,
	RECP_SELECT_MENU_EDIT,
	RECP_SELECT_MENU_ADD_TO_CONTACTS,
	RECP_SELECT_MENU_MOVE_TO_TO,
	RECP_SELECT_MENU_MOVE_TO_CC,
	RECP_SELECT_MENU_MOVE_TO_BCC,
	RECP_SELECT_MENU_MAX,
} RECIPIENT_SELECT_MENU_TYPE;

static Eina_Bool recipient_select_menu[RECP_SELECT_MENU_MAX];


/*
 * Definitions for static functions
 */

#ifdef FEATRUE_PREDICTIVE_SEARCH
static void _request_predictive_search(EmailComposerUGD *ugd)
{
	debug_enter();

	/* To prevent displaying gal-search popup when email address is set on entry. (entry_changed_cb is called!) */
	if (edit_mode == true) {
		edit_mode = false;
		return;
	}

	if (strlen(ugd->ps_keyword) > 0) {
		composer_ps_start_search(ugd);
	} else {
		if (ugd->ps_is_runnig) {
			composer_ps_stop_search(ugd);
		}
	}

	debug_leave();
}
#endif

static char *_select_address_gl_text_get(void *data, Evas_Object *obj, const char *part)
{
	int index = (int)(ptrdiff_t)data;

	if (!strcmp(part, "elm.text")) {
		switch (index) {
		case 0:
			return strdup(email_get_email_string("IDS_EMAIL_OPT_REMOVE"));
		case 1:
			return strdup(email_get_email_string("IDS_EMAIL_OPT_EDIT"));
		case 2:
			return strdup(email_get_email_string("IDS_EMAIL_OPT_ADD_TO_CONTACTS_ABB2"));
		case 3:
			return strdup(email_get_email_string("IDS_EMAIL_OPT_MOVE_TO_MAIN_RECIPIENT_HTO_ABB"));
		case 4:
			return strdup(email_get_email_string("IDS_EMAIL_OPT_MOVE_TO_CC"));
		case 5:
			return strdup(email_get_email_string("IDS_EMAIL_OPT_MOVE_TO_BCC_ABB"));
		default:
			debug_error("MUST NOT REACH HERE!");
			break;
		}
	}

	return NULL;
}

static void _select_address_gl_sel(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;

	if (item != NULL) {
		char *label = (char *)elm_object_item_part_text_get(item, "elm.text");

		if (!strcmp(label, email_get_email_string("IDS_EMAIL_OPT_REMOVE"))) {
			_recipient_mbe_popup_delete(ugd);
		} else if (!strcmp(label, email_get_email_string("IDS_EMAIL_OPT_EDIT"))) {
			_recipient_mbe_popup_edit(ugd);
		} else if (!strcmp(label, email_get_email_string("IDS_EMAIL_OPT_MOVE_TO_MAIN_RECIPIENT_HTO_ABB"))) {
			_recipient_mbe_popup_move_recipient(data, ugd->recp_to_mbe);
		} else if (!strcmp(label, email_get_email_string("IDS_EMAIL_OPT_MOVE_TO_CC"))) {
			if (!ugd->bcc_added) {
				composer_recipient_show_hide_bcc_field(ugd, EINA_TRUE);
			}
			_recipient_mbe_popup_move_recipient(data, ugd->recp_cc_mbe);
		} else if (!strcmp(label, email_get_email_string("IDS_EMAIL_OPT_MOVE_TO_BCC_ABB"))) {
			if (!ugd->bcc_added) {
				composer_recipient_show_hide_bcc_field(ugd, EINA_TRUE);
			}
			_recipient_mbe_popup_move_recipient(data, ugd->recp_bcc_mbe);
		} else if (!strcmp(label, email_get_email_string("IDS_EMAIL_OPT_ADD_TO_CONTACTS_ABB2"))) {
			_recipient_mbe_popup_add_to_contact_selection_popup(ugd);
		}
	}

	debug_leave();
}

static void _recipient_mbe_popup_delete(EmailComposerUGD *ugd)
{
	debug_enter();

	retm_if(!ugd, "Invalid paramater: ugd is NULL!");

	elm_object_item_del(ugd->selected_mbe_item);

	composer_util_popup_response_cb(ugd, NULL, NULL);

	debug_leave();
}

static void _recipient_mbe_popup_edit(EmailComposerUGD *ugd)
{
	debug_enter();

	retm_if(!ugd, "Invalid paramater: ugd is NULL!");

	EmailRecpInfo *ri = (EmailRecpInfo *)elm_object_item_data_get(ugd->selected_mbe_item);
	retm_if(!ri, "ri is NULL!!!");

	EmailAddressInfo *ai = (EmailAddressInfo *)eina_list_nth(ri->email_list, ri->selected_email_idx);
	retm_if(!ai, "ai is NULL!");

	edit_mode = true; /* To prevent displaying gal-search popup when email address is set on entry. (entry_changed_cb is called!) */

	char *existing_text = g_strdup(elm_entry_entry_get(ugd->selected_entry));

	debug_secure("index = %d, email_addr = %s", ri->selected_email_idx, ai->address);
	elm_entry_entry_set(ugd->selected_entry, ai->address);
	elm_entry_cursor_end_set(ugd->selected_entry);
	elm_entry_input_panel_show(ugd->selected_entry);

	elm_object_item_del(ugd->selected_mbe_item);
	ugd->selected_mbe_item = NULL;

	if (email_get_address_validation(existing_text)) {
		Evas_Object *mbe = NULL;
		if (ugd->selected_entry == ugd->recp_to_entry.entry) {
			mbe = ugd->recp_to_mbe;
		} else if (ugd->selected_entry == ugd->recp_cc_entry.entry) {
			mbe = ugd->recp_cc_mbe;
		} else if (ugd->selected_entry == ugd->recp_bcc_entry.entry) {
			mbe = ugd->recp_bcc_mbe;
		}
		retm_if(!mbe, "Invalid entry is selected!");

		ri = composer_util_recp_make_recipient_info(ugd, existing_text);
		if (ri) {
			char *markup_name = elm_entry_utf8_to_markup(ri->display_name);
			elm_multibuttonentry_item_append(mbe, markup_name, NULL, ri);
			FREE(markup_name);
		} else {
			debug_error("ri is NULL!");
		}
	} else if (g_strcmp0(existing_text, "")) {
		composer_recipient_display_error_string(ugd, MBE_VALIDATION_ERROR_INVALID_ADDRESS);
	}
	FREE(existing_text);

	composer_util_popup_response_cb(ugd, NULL, NULL);

	debug_leave();
}

static Eina_Bool _recipient_mbe_move_recipient_destroy_popup_idler_cb(void *data)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	ugd->idler_move_recipient = NULL;

	composer_util_popup_response_cb(ugd, NULL, NULL);

	debug_leave();
	return ECORE_CALLBACK_CANCEL;
}

static void _recipient_mbe_popup_move_recipient(EmailComposerUGD *ugd, Evas_Object *dest_mbe)
{
	debug_enter();

	retm_if(!ugd, "Invalid paramater: ugd is NULL!");

	EmailRecpInfo *ri = (EmailRecpInfo *)elm_object_item_data_get(ugd->selected_mbe_item);
	retm_if(!ri, "ri is NULL!");

	EmailAddressInfo *ai = (EmailAddressInfo *)eina_list_nth(ri->email_list, ri->selected_email_idx);
	retm_if(!ai, "ai is NULL!");

	/* Not to delete ri when item,deleted callback is called. */
	elm_object_item_data_set(ugd->selected_mbe_item, NULL);
	elm_object_item_del(ugd->selected_mbe_item);
	ugd->selected_mbe_item = NULL;

	/* If there's the same recipient on dest_mbe, we don't need to append it. */
	if (!composer_util_recp_is_duplicated(ugd, dest_mbe, ai->address)) {
		debug_secure("display name: [%s]", ri->display_name);
		if (ri->display_name) {
			char *markup_name = elm_entry_utf8_to_markup(ri->display_name);
			elm_multibuttonentry_item_append(dest_mbe, markup_name, NULL, ri);
			FREE(markup_name);
		} else {
			elm_multibuttonentry_item_append(dest_mbe, ai->address, NULL, ri);
		}
	} else {
		composer_recipient_display_error_string(ugd, MBE_VALIDATION_ERROR_DUPLICATE_ADDRESS);
	}

	if (!ugd->cc_added && ((dest_mbe == ugd->recp_cc_mbe) || dest_mbe == ugd->recp_bcc_mbe)) {
		composer_recipient_show_hide_cc_field(ugd, EINA_TRUE);
		composer_recipient_show_hide_bcc_field(ugd, EINA_TRUE);
	}

	/* We need to reset focus allow set which we set to false on mbe_select */
	elm_object_tree_focus_allow_set(ugd->composer_layout, EINA_TRUE);
	elm_object_focus_allow_set(ugd->selected_entry, EINA_TRUE);
	elm_object_focus_allow_set(ugd->subject_entry.entry, EINA_FALSE);
	elm_object_focus_allow_set(ugd->ewk_btn, EINA_FALSE); /* ewk_btn isn't a child of composer_layout. so we need to control the focus of it as well. */
	/* To reset recipient layout for the previous selected entry because selected_entry will be changed to dest mbe. */
	composer_recipient_unfocus_entry(ugd, ugd->selected_entry);

	/* To set the focus to destination entry */
	Evas_Object *entry_box = NULL;
	Evas_Object *entry_layout = NULL;
	Evas_Object *display_entry_layout = NULL;
	email_editfield_t display_entry;

	if (dest_mbe == ugd->recp_to_mbe) {
		ugd->selected_entry = ugd->recp_to_entry.entry;
		entry_box = ugd->recp_to_box;
		entry_layout = ugd->recp_to_entry_layout;
		display_entry = ugd->recp_to_display_entry;
		display_entry_layout = ugd->recp_to_display_entry_layout;
		composer_recipient_change_entry(EINA_TRUE, entry_box, &ugd->recp_to_entry, &display_entry, entry_layout, display_entry_layout);
	} else if (dest_mbe == ugd->recp_cc_mbe) {
		ugd->selected_entry = ugd->recp_cc_entry.entry;
		entry_box = ugd->recp_cc_box;
		entry_layout = ugd->recp_cc_entry_layout;
		display_entry = ugd->recp_cc_display_entry;
		display_entry_layout = ugd->recp_cc_display_entry_layout;
		composer_recipient_change_entry(EINA_TRUE, entry_box, &ugd->recp_cc_entry, &display_entry, entry_layout, display_entry_layout);
	} else if (dest_mbe == ugd->recp_bcc_mbe) {
		ugd->selected_entry = ugd->recp_bcc_entry.entry;
		entry_box = ugd->recp_bcc_box;
		entry_layout = ugd->recp_bcc_entry_layout;
		display_entry = ugd->recp_bcc_display_entry;
		display_entry_layout = ugd->recp_bcc_display_entry_layout;
		composer_recipient_change_entry(EINA_TRUE, entry_box, &ugd->recp_bcc_entry, &display_entry, entry_layout, display_entry_layout);
	}

	/* Destination entry is changed to shown status. But some status values in EFL side aren't updated immediately.
	 * They'll be updated on the next idle time.
	 * We have to use idler here to set the focus to selected_entry because the focus is moved to another entry if we destroy popup here.
	 */
	ugd->idler_move_recipient = ecore_idler_add(_recipient_mbe_move_recipient_destroy_popup_idler_cb, ugd);

	debug_leave();
}

static void _recipient_mbe_popup_add_to_contact(EmailComposerUGD *ugd)
{
	debug_enter();

	retm_if(!ugd, "Invalid paramater: ugd is NULL!");

	composer_launcher_add_contact(ugd);

	debug_leave();
}

static void _recipient_mbe_popup_update_contact(EmailComposerUGD *ugd)
{
	debug_enter();

	retm_if(!ugd, "Invalid paramater: ugd is NULL!");

	composer_launcher_update_contact(ugd);

	debug_leave();
}

static char *_add_to_contact_popup_gl_text_get(void *data, Evas_Object *obj, const char *part)
{
	debug_enter();
	retvm_if(obj == NULL, NULL, "Invalid parameter: obj[NULL]");

	int index = (int)(ptrdiff_t)data;

	if (!strcmp(part, "elm.text")) {
		if (index == 0) {
			return strdup(email_get_email_string("IDS_EMAIL_OPT_CREATE_CONTACT"));
		} else if (index == 1) {
			return strdup(email_get_email_string("IDS_EMAIL_OPT_UPDATE_CONTACT"));
		}
	}
	return NULL;
}

static void _add_to_contact_popup_gl_sel(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	retm_if(event_info == NULL, "Invalid parameter: event_info[NULL]");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;

	if (item != NULL) {
		int index = (int)(ptrdiff_t)elm_object_item_data_get(item);

		if (index == 0) {
			_recipient_mbe_popup_add_to_contact(ugd);
		} else if (index == 1) {
			_recipient_mbe_popup_update_contact(ugd);
		}
	}
	composer_util_popup_response_cb(data, obj, event_info);

	debug_leave();
}

static void _recipient_mbe_popup_add_to_contact_selection_popup(void *data)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	int max_index = 0;
	int index = 0;
	char *title_str = NULL;

	EmailRecpInfo *ri = (EmailRecpInfo *)elm_object_item_data_get(ugd->selected_mbe_item);
	retm_if(!ri, "ri is NULL!");

	EmailAddressInfo *ai = (EmailAddressInfo *)eina_list_nth(ri->email_list, ri->selected_email_idx);
	retm_if(!ai, "ai is NULL!");

	if (ri->display_name) {
		debug_secure("display_name:(%s), index:(%d)", ri->display_name, ri->selected_email_idx);

		char *utf8_str = g_strconcat(ri->display_name, " <", ai->address, ">", NULL);
		title_str = elm_entry_utf8_to_markup(utf8_str);

		debug_secure("utf8 string:[%s]", utf8_str);
		debug_secure("markup string:[%s]", title_str);
		g_free(utf8_str);
	} else {
		title_str = g_strdup(ai->address);
	}

	/* To prevent showing ime when the focus is changed. (when popup is deleted, the focus moves to the previous focused object) */
	elm_object_focus_allow_set(ugd->ewk_btn, EINA_FALSE);
	elm_object_tree_focus_allow_set(ugd->composer_layout, EINA_FALSE);

	email_string_t EMAIL_COMPOSER_STRING_NO_TRANSITION = { NULL, title_str };

	ugd->composer_popup = common_util_create_popup(ugd->base.module->win,
			EMAIL_COMPOSER_STRING_NO_TRANSITION,
			NULL, EMAIL_COMPOSER_STRING_NULL,
			NULL, EMAIL_COMPOSER_STRING_NULL,
			NULL, EMAIL_COMPOSER_STRING_NULL,
			composer_util_popup_response_cb, EINA_TRUE, ugd);

	g_free(title_str);

	add_to_contact_popup_itc.item_style = "type1";
	add_to_contact_popup_itc.func.text_get = _add_to_contact_popup_gl_text_get;
	add_to_contact_popup_itc.func.content_get = NULL;
	add_to_contact_popup_itc.func.state_get = NULL;
	add_to_contact_popup_itc.func.del = NULL;

	Evas_Object *genlist = elm_genlist_add(ugd->composer_popup);
	evas_object_data_set(genlist, COMPOSER_EVAS_DATA_NAME, ugd);
	elm_genlist_homogeneous_set(genlist, EINA_TRUE);
	elm_scroller_policy_set(genlist, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
	elm_scroller_content_min_limit(genlist, EINA_FALSE, EINA_TRUE);

	max_index = 2;

	for (index = 0; index < max_index; index++) {
		elm_genlist_item_append(genlist, &add_to_contact_popup_itc, (void *)(ptrdiff_t)index, NULL, ELM_GENLIST_ITEM_NONE, _add_to_contact_popup_gl_sel, (void *)ugd);
	}

	elm_object_content_set(ugd->composer_popup, genlist);
	evas_object_show(genlist);

	debug_leave();
}

/*
 * Definitions for exported functions
 */

void _recipient_contact_button_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(!data, "Invalid parameter: data is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	retm_if(ugd->base.module->is_launcher_busy, "is_launcher_busy = true");

	if (!ugd->allow_click_events) {
		debug_log("Click was blocked.");
		return;
	}

	email_feedback_play_tap_sound();

	if (((ugd->selected_entry == ugd->recp_to_entry.entry) && (ugd->to_recipients_cnt >= MAX_RECIPIENT_COUNT)) ||
		((ugd->selected_entry == ugd->recp_cc_entry.entry) && (ugd->cc_recipients_cnt >= MAX_RECIPIENT_COUNT)) ||
		((ugd->selected_entry == ugd->recp_bcc_entry.entry) && (ugd->bcc_recipients_cnt >= MAX_RECIPIENT_COUNT))) {
		char buf[BUF_LEN_L] = { 0, };
		snprintf(buf, sizeof(buf), email_get_email_string("IDS_EMAIL_TPOP_MAXIMUM_NUMBER_OF_RECIPIENTS_HPD_REACHED"), MAX_RECIPIENT_COUNT);

		int noti_ret = notification_status_message_post(buf);
		debug_warning_if(noti_ret != NOTIFICATION_ERROR_NONE, "notification_status_message_post() failed! ret:(%d)", noti_ret);
	} else {
		composer_launcher_pick_contacts(data);
	}

	debug_leave();
}

Eina_Bool _recipient_mbe_filter_cb(Evas_Object *obj, const char *item_label, void *item_data, void *data)
{
	debug_enter();

	retvm_if(!item_label, EINA_FALSE, "item_label is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	EmailRecpInfo *ri = (EmailRecpInfo *)item_data;
	gchar buff_addr[EMAIL_LIMIT_EMAIL_ADDRESS_LENGTH + 1] = { 0, };

	Eina_Bool ret = EINA_TRUE;
	char *whole_address = NULL;
	char *invalid_list = NULL;
	MBE_VALIDATION_ERROR err = MBE_VALIDATION_ERROR_NONE;

	Evas_Object *recp_entry = NULL;
	if (obj == ugd->recp_to_mbe) {
		recp_entry = ugd->recp_to_entry.entry;
	} else if (obj == ugd->recp_cc_mbe) {
		recp_entry = ugd->recp_cc_entry.entry;
	} else if (obj == ugd->recp_bcc_mbe) {
		recp_entry = ugd->recp_bcc_entry.entry;
	}

	if (ri) {
		EmailAddressInfo *ai = (EmailAddressInfo *)eina_list_nth(ri->email_list, ri->selected_email_idx);
		if (ai) {
			whole_address = ai->address;
		}
	}

	if (!whole_address) {
		char *utf8_item_label = elm_entry_markup_to_utf8(item_label);
		strncpy(buff_addr, (char *)utf8_item_label, EMAIL_LIMIT_EMAIL_ADDRESS_LENGTH); /* restricting the size to avoid very long string operations */
		whole_address = buff_addr;
		g_free(utf8_item_label);
	}

	ret = composer_recipient_mbe_validate_email_address_list(ugd, obj, whole_address, &invalid_list, &err);

	/* If mbe item is added by pressing ";" or "," key, entry isn't cleared because ret is false.
	 * In that case, we clear the entry manually.
	 */
	if ((err == MBE_VALIDATION_ERROR_NONE) && (ret == EINA_FALSE)) {
		elm_entry_entry_set(recp_entry, NULL);
	}

	if (err >= MBE_VALIDATION_ERROR_DUPLICATE_ADDRESS) {
		if (ri) {
			g_free(ri->display_name);
			g_free(ri);
		}
		if ((err != MBE_VALIDATION_ERROR_MAX_NUMBER_REACHED) && g_strcmp0(invalid_list, elm_entry_entry_get(recp_entry))) {
			invalid_list = g_strstrip(invalid_list);
			char *markup_invalid_list = elm_entry_utf8_to_markup(invalid_list);
			elm_entry_entry_set(recp_entry, markup_invalid_list);
			FREE(markup_invalid_list);
		}
		elm_entry_cursor_end_set(recp_entry);
		FREE(invalid_list);

		composer_recipient_display_error_string(ugd, err);
	} else {
		if (ret) {
			if (obj == ugd->recp_to_mbe) {
				ugd->to_recipients_cnt++;
			} else if (obj == ugd->recp_cc_mbe) {
				ugd->cc_recipients_cnt++;
			} else if (obj == ugd->recp_bcc_mbe) {
				ugd->bcc_recipients_cnt++;
			}
			composer_util_modify_send_button(ugd);
		}
	}
	debug_leave();
	return ret;
}

void _recipient_mbe_added_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;

	EmailRecpInfo *ri = (EmailRecpInfo *)elm_object_item_data_get(item);
	if (!ri) {
		char *email_address = (char *)elm_object_item_text_get(item);
		if (!email_address) {
			debug_error("email_adress is NULL!");
			elm_object_item_del(item);
			return;
		}

		ri = composer_util_recp_make_recipient_info(ugd, email_address);
		if (!ri) {
			debug_error("ri is NULL!");
			elm_object_item_del(item);
			return;
		}

		if (g_strcmp0(email_address, ri->display_name) != 0) {
			char *temp = elm_entry_utf8_to_markup(ri->display_name);
			elm_object_item_text_set(item, temp);
			free(temp);
		}
		elm_object_item_data_set(item, ri);
	}

	if ((obj == ugd->recp_to_mbe) && !composer_util_is_object_packed_in(ugd->composer_box, ugd->recp_to_mbe_layout)) {
		if (ugd->selected_entry == ugd->recp_to_entry.entry) {
			composer_recipient_reset_entry_with_mbe(ugd->composer_box, ugd->recp_to_mbe_layout, ugd->recp_to_mbe, ugd->recp_to_layout, ugd->recp_to_box, ugd->recp_to_label);
		} else {
			composer_recipient_change_entry(EINA_FALSE, ugd->recp_to_box, &ugd->recp_to_entry, &ugd->recp_to_display_entry, ugd->recp_to_entry_layout, ugd->recp_to_display_entry_layout);
			composer_recipient_update_display_string(ugd, ugd->recp_to_mbe, ugd->recp_to_entry.entry, ugd->recp_to_display_entry.entry, ugd->to_recipients_cnt);
		}
	} else if ((obj == ugd->recp_cc_mbe) && !composer_util_is_object_packed_in(ugd->composer_box, ugd->recp_cc_mbe_layout)) {
		if (ugd->selected_entry == ugd->recp_cc_entry.entry) {
			composer_recipient_reset_entry_with_mbe(ugd->composer_box, ugd->recp_cc_mbe_layout, ugd->recp_cc_mbe, ugd->recp_cc_layout, ugd->recp_cc_box, ugd->bcc_added ? ugd->recp_cc_label_cc : ugd->recp_cc_label_cc_bcc);
		} else {
			composer_recipient_change_entry(EINA_FALSE, ugd->recp_cc_box, &ugd->recp_cc_entry, &ugd->recp_cc_display_entry, ugd->recp_cc_entry_layout, ugd->recp_cc_display_entry_layout);
			composer_recipient_update_display_string(ugd, ugd->recp_cc_mbe, ugd->recp_cc_entry.entry, ugd->recp_cc_display_entry.entry, ugd->cc_recipients_cnt);
		}
	} else if ((obj == ugd->recp_bcc_mbe) && !composer_util_is_object_packed_in(ugd->composer_box, ugd->recp_bcc_mbe_layout)) {
		if (ugd->selected_entry == ugd->recp_bcc_entry.entry) {
			composer_recipient_reset_entry_with_mbe(ugd->composer_box, ugd->recp_bcc_mbe_layout, ugd->recp_bcc_mbe, ugd->recp_bcc_layout, ugd->recp_bcc_box, ugd->recp_bcc_label);
		} else {
			composer_recipient_change_entry(EINA_FALSE, ugd->recp_bcc_box, &ugd->recp_bcc_entry, &ugd->recp_bcc_display_entry, ugd->recp_bcc_entry_layout, ugd->recp_bcc_display_entry_layout);
			composer_recipient_update_display_string(ugd, ugd->recp_bcc_mbe, ugd->recp_bcc_entry.entry, ugd->recp_bcc_display_entry.entry, ugd->bcc_recipients_cnt);
		}
	}

	debug_leave();
}

void _recipient_mbe_deleted_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	EmailRecpInfo *ri = (EmailRecpInfo *)elm_object_item_data_get((const Elm_Object_Item *)event_info);

	/* When item is moved from one to another field, ri is moved to the item in destination field. In case of this, ri is NULL. */
	if (ri) {
		g_free(ri->display_name);
		g_free(ri);
	}
	/*ugd->selected_mbe_item = NULL;*/
	ugd->is_last_item_selected = EINA_FALSE;

	if (obj == ugd->recp_to_mbe) {
		ugd->to_recipients_cnt--;
		debug_warning_if(ugd->to_recipients_cnt < 0, "Invalid state!! ugd->to_recipients_cnt:[%d]", ugd->to_recipients_cnt);
	} else if (obj == ugd->recp_cc_mbe) {
		ugd->cc_recipients_cnt--;
		debug_warning_if(ugd->cc_recipients_cnt < 0, "Invalid state!! ugd->cc_recipients_cnt:[%d]", ugd->cc_recipients_cnt);
	} else if (obj == ugd->recp_bcc_mbe) {
		ugd->bcc_recipients_cnt--;
		debug_warning_if(ugd->bcc_recipients_cnt < 0, "Invalid state!! ugd->bcc_recipients_cnt:[%d]", ugd->bcc_recipients_cnt);
	}

	if ((obj == ugd->recp_to_mbe) && (ugd->to_recipients_cnt == 0)) {
		composer_recipient_reset_entry_without_mbe(ugd->composer_box, ugd->recp_to_mbe_layout, ugd->recp_to_layout, ugd->recp_to_box, ugd->recp_to_label);
		composer_recipient_update_display_string(ugd, ugd->recp_to_mbe, ugd->recp_to_entry.entry, ugd->recp_to_display_entry.entry, ugd->to_recipients_cnt);
	} else if ((obj == ugd->recp_cc_mbe) && (ugd->cc_recipients_cnt == 0)) {
		composer_recipient_reset_entry_without_mbe(ugd->composer_box, ugd->recp_cc_mbe_layout, ugd->recp_cc_layout, ugd->recp_cc_box, ugd->bcc_added ? ugd->recp_cc_label_cc : ugd->recp_cc_label_cc_bcc);
		composer_recipient_update_display_string(ugd, ugd->recp_cc_mbe, ugd->recp_cc_entry.entry, ugd->recp_cc_display_entry.entry, ugd->cc_recipients_cnt);
	} else if ((obj == ugd->recp_bcc_mbe) && (ugd->bcc_recipients_cnt == 0)) {
		composer_recipient_reset_entry_without_mbe(ugd->composer_box, ugd->recp_bcc_mbe_layout, ugd->recp_bcc_layout, ugd->recp_bcc_box, ugd->recp_bcc_label);
		composer_recipient_update_display_string(ugd, ugd->recp_bcc_mbe, ugd->recp_bcc_entry.entry, ugd->recp_bcc_display_entry.entry, ugd->bcc_recipients_cnt);
	}

	ugd->selected_mbe_item = NULL;
	elm_entry_cursor_end_set(ugd->selected_entry);
	/* composer can also be destroyed when viewer ug gets launched from notification */
	if (!(ugd->is_back_btn_clicked || ugd->is_save_in_drafts_clicked || ugd->is_send_btn_clicked || ugd->is_composer_getting_destroyed)) {
		composer_util_focus_set_focus_with_idler(ugd, ugd->selected_entry);
		composer_util_modify_send_button(ugd);
	}
	debug_leave();
}

void _recipient_mbe_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	int index = 0;
	EmailRecpInfo *ri = NULL;
	char *title_str = NULL;

	ugd->selected_mbe_item = (Elm_Object_Item *)event_info;
	ri = (EmailRecpInfo *)elm_object_item_data_get(ugd->selected_mbe_item);
	retm_if(!ri, "ri is NULL!");

	if (!ugd->allow_click_events) {
		debug_log("Click was blocked.");
		return;
	}

	EmailAddressInfo *ai = (EmailAddressInfo *)eina_list_nth(ri->email_list, ri->selected_email_idx);
	retm_if(!ai, "ai is NULL!");

	if (ri->display_name) {
		if (!g_strcmp0(ri->display_name, ai->address)) {
			title_str = g_strdup(ai->address);
		} else {
			debug_secure("display_name:%s, index:%d", ri->display_name, ri->selected_email_idx);

			char *utf8_str = g_strconcat(ri->display_name, " <", ai->address, ">", NULL);
			title_str = elm_entry_utf8_to_markup(utf8_str);

			debug_secure("utf8 string:[%s]", utf8_str);
			debug_secure("markup string:[%s]", title_str);
			g_free(utf8_str);
		}
	} else {
		title_str = g_strdup(ai->address);
	}

	email_string_t EMAIL_COMPOSER_STRING_NO_TRANSITION = { NULL, title_str };

	ugd->composer_popup = common_util_create_popup(ugd->base.module->win,
			EMAIL_COMPOSER_STRING_NO_TRANSITION,
			NULL, EMAIL_COMPOSER_STRING_NULL,
			NULL, EMAIL_COMPOSER_STRING_NULL,
			NULL, EMAIL_COMPOSER_STRING_NULL,
			composer_util_popup_response_cb, EINA_TRUE, ugd);

	g_free(title_str);

	select_address_itc.item_style = "type1";
	select_address_itc.func.text_get = _select_address_gl_text_get;
	select_address_itc.func.content_get = NULL;
	select_address_itc.func.state_get = NULL;
	select_address_itc.func.del = NULL;

	Evas_Object *genlist = elm_genlist_add(ugd->composer_popup);
	evas_object_data_set(genlist, COMPOSER_EVAS_DATA_NAME, ugd);
	elm_genlist_homogeneous_set(genlist, EINA_TRUE);
	elm_scroller_policy_set(genlist, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
	elm_scroller_content_min_limit(genlist, EINA_FALSE, EINA_TRUE);

	memset(recipient_select_menu, 0, sizeof(recipient_select_menu));

	recipient_select_menu[RECP_SELECT_MENU_REMOVE] = EINA_TRUE;
	recipient_select_menu[RECP_SELECT_MENU_EDIT] = EINA_TRUE;
	if (ri->person_id <= 0) {
		recipient_select_menu[RECP_SELECT_MENU_ADD_TO_CONTACTS] = EINA_TRUE;
	}
	if (ugd->cc_added) {
		if (ugd->selected_entry == ugd->recp_to_entry.entry) {
			recipient_select_menu[RECP_SELECT_MENU_MOVE_TO_CC] = EINA_TRUE;
		} else {
			recipient_select_menu[RECP_SELECT_MENU_MOVE_TO_TO] = EINA_TRUE;
		}
		if (ugd->selected_entry == ugd->recp_bcc_entry.entry) {
			recipient_select_menu[RECP_SELECT_MENU_MOVE_TO_CC] = EINA_TRUE;
		} else {
			recipient_select_menu[RECP_SELECT_MENU_MOVE_TO_BCC] = EINA_TRUE;
		}
	}

	for (index = 0; index < RECP_SELECT_MENU_MAX; index++) {
		if (recipient_select_menu[index]) {
			elm_genlist_item_append(genlist, &select_address_itc, (void *)(ptrdiff_t)index, NULL, ELM_GENLIST_ITEM_NONE, _select_address_gl_sel, (void *)ugd);
		}
	}

	elm_object_content_set(ugd->composer_popup, genlist);
	evas_object_show(genlist);

	debug_leave();
}

void _recipient_mbe_focused_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	/* 1. When you click an empty area in mbe, mbe gets the focus and entry lost its focus. so keypad is hided.
	 *    Re-set the focus to previous entry once mbe gets the focus to prevent this problem.
	 * This is handled since we are showing keyboard manually in that case
	 * 2. When a mbe item is selected by pressing BackSpace on the entry, mbe also gets the focus. (focus move: entry -> mbe -> mbe item)
	 *    In this case, we don't set the focus to previous entry.
	 * We need not set focus to the entry when mbe gets focused, because entry already has the focus.
	*/

	debug_leave();
}

void _recipient_entry_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	/* If there's invalid string on recipient entry, this callback is called while destroying composer.
	 * It makes predictive search layout and causes BS for MOUSE DOWN event. (P140918-05019)
	 */
	retm_if(ugd->is_back_btn_clicked || ugd->is_save_in_drafts_clicked || ugd->is_send_btn_clicked, "While destroying composer!");

	/* Because of elm_entry_entry_set(), this callback can be called even though the entry doesn't have the focus. */
	ret_if(ugd->selected_entry != obj);

	const char *entry_str = elm_entry_entry_get(obj);
	retm_if(!entry_str, "entry_str is [NULL]");
	if (elm_entry_is_empty(obj) || strcmp(entry_str, "<br/>") == 0) {
		debug_log("entry is empty");
		ugd->is_real_empty_entry = EINA_TRUE;
	} else {
		debug_log("entry is not empty");
		ugd->is_real_empty_entry = EINA_FALSE;
	}

	/* Note: keydown callback is called for only the keys in hareware keyboard even though we press keys in virtual keypad.
	 * So we need to handle is_real_empty_entry here instead in keydown callback.
	 */
	char *search_word = elm_entry_markup_to_utf8(entry_str);
	retm_if(!search_word, "elm_entry_markup_to_utf8 is failed");

	debug_secure("search_word: (%s)", search_word);

#ifdef FEATRUE_PREDICTIVE_SEARCH
	/* When the focused UI is enabled, we don't need to display the predictive list. */
	if (!elm_win_focus_highlight_enabled_get(ugd->base.module->win) &&
		!ugd->base.module->is_launcher_busy && (g_strcmp0(ugd->ps_keyword, search_word) != 0)) {
		snprintf(ugd->ps_keyword, sizeof(ugd->ps_keyword), "%s", search_word);
		_request_predictive_search(ugd);
	}
#endif
	free(search_word);

	debug_leave();
}

void _recipient_entry_focused_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	DELETE_EVAS_OBJECT(ugd->context_popup); /* XXX: check this */

	if (obj == ugd->recp_to_entry.entry) {
		if (ugd->to_recipients_cnt > 0) {
			composer_recipient_reset_entry_with_mbe(ugd->composer_box, ugd->recp_to_mbe_layout, ugd->recp_to_mbe, ugd->recp_to_layout, ugd->recp_to_box, ugd->recp_to_label);
		}
		composer_recipient_show_contact_button(ugd, ugd->recp_to_layout, ugd->recp_to_btn);

		if (ugd->bcc_added && !ugd->cc_recipients_cnt && !ugd->bcc_recipients_cnt) {
			composer_recipient_show_hide_bcc_field(ugd, EINA_FALSE);
		}
	} else if (obj == ugd->recp_cc_entry.entry) {
		if (ugd->cc_recipients_cnt > 0) {
			composer_recipient_reset_entry_with_mbe(ugd->composer_box, ugd->recp_cc_mbe_layout, ugd->recp_cc_mbe, ugd->recp_cc_layout, ugd->recp_cc_box, ugd->bcc_added ? ugd->recp_cc_label_cc : ugd->recp_cc_label_cc_bcc);
		}
		composer_recipient_show_contact_button(ugd, ugd->recp_cc_layout, ugd->recp_cc_btn);

		if (!ugd->bcc_added) {
			composer_recipient_show_hide_bcc_field(ugd, EINA_TRUE);
		}
	} else if (obj == ugd->recp_bcc_entry.entry) {
		if (ugd->bcc_recipients_cnt > 0) {
			composer_recipient_reset_entry_with_mbe(ugd->composer_box, ugd->recp_bcc_mbe_layout, ugd->recp_bcc_mbe, ugd->recp_bcc_layout, ugd->recp_bcc_box, ugd->recp_bcc_label);
		}
		composer_recipient_show_contact_button(ugd, ugd->recp_bcc_layout, ugd->recp_bcc_btn);
	}

	/* XXX; Do we need to set this variable here? */
	if (elm_entry_is_empty(obj)) {
		ugd->is_real_empty_entry = EINA_TRUE;
	} else {
		ugd->is_real_empty_entry = EINA_FALSE;
	}

	composer_webkit_blur_webkit_focus(ugd);
	if (ugd->richtext_toolbar) {
		composer_rich_text_disable_set(ugd, EINA_TRUE);
	}

	if (ugd->selected_entry != obj) {
		/* Below code should be here, since it returned without resetting the entry in case of error before */
		if (composer_recipient_is_recipient_entry(ugd, ugd->selected_entry)) {
			if (!composer_recipient_commit_recipient_on_entry(ugd, ugd->selected_entry)) {
				composer_recipient_unfocus_entry(ugd, obj); /* The current object should be unfocused if the function returns for composer_recipient_reset_entry_without_mbe to run */
				debug_log("composer_recipient_commit_recipient_on_entry returned false");
				return;
			}
		}
		composer_recipient_unfocus_entry(ugd, ugd->selected_entry);
		composer_attachment_ui_contract_attachment_list(ugd);
		ugd->selected_entry = obj;
	}

	/* To show the entry within the screen. refer the comments on focused callback for subject entry.*/
	evas_smart_objects_calculate(evas_object_evas_get(ugd->composer_layout));

	debug_leave();
}

void _recipient_entry_unfocused_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	_recipient_mbe_unselect_last_item(obj, ugd);

	debug_leave();
}

void _recipient_entry_keydown_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	Evas_Event_Key_Up *ev = (Evas_Event_Key_Up *)event_info;

	if (!strcmp(ev->keyname, "BackSpace")) {
		if (ugd->is_real_empty_entry) {
			ugd->is_backspace_pressed = EINA_TRUE;
		}
	} else {
		if (!strcmp(ev->keyname, "KP_Enter") || !strcmp(ev->keyname, "Return") || !strcmp(ev->keyname, "semicolon") || !strcmp(ev->keyname, "comma")) {
			bool res_cmp = false;
			const char *entry_str = elm_entry_entry_get(obj);
			retm_if(!entry_str, "entry_str is [NULL]");
			if (elm_entry_is_empty(obj) || (res_cmp = (strcmp(entry_str, "<br/>") == 0))) {
				if (res_cmp) {
					elm_entry_entry_set(obj, NULL);
				}
				ugd->is_next_pressed = EINA_TRUE;
			} else {
				ugd->is_commit_pressed = EINA_TRUE;
			}
		}

		_recipient_mbe_unselect_last_item(obj, ugd);
	}

	/*debug_leave();*/
}

void _recipient_entry_keyup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	/* Evas_Event_Key_Up *ev = (Evas_Event_Key_Up *)event_info;
	 *debug_secure("keyname: (%s)", ev->keyname);
	 */

	if (ugd->is_commit_pressed) {
		Evas_Object *mbe = NULL;
		if (obj == ugd->recp_to_entry.entry) {
			mbe = ugd->recp_to_mbe;
		} else if (obj == ugd->recp_cc_entry.entry) {
			mbe = ugd->recp_cc_mbe;
		} else if (obj == ugd->recp_bcc_entry.entry) {
			mbe = ugd->recp_bcc_mbe;
		}

		char *enter_word = elm_entry_markup_to_utf8(elm_entry_entry_get(obj));
		EmailRecpInfo *ri = composer_util_recp_make_recipient_info(ugd, enter_word);
		if (ri) {
			char *markup_name = elm_entry_utf8_to_markup(ri->display_name);
			Elm_Object_Item *it = elm_multibuttonentry_item_append(mbe, markup_name, NULL, ri);
			FREE(markup_name);
			if (it) {
				elm_entry_entry_set(obj, "");
			}
		}
		FREE(enter_word);
	} else if (ugd->is_next_pressed) {
		Evas_Object *next_widget = NULL;
		if (obj == ugd->recp_to_entry.entry) {
			if (ugd->cc_added) {
				composer_recipient_change_entry(EINA_TRUE, ugd->recp_cc_box, &ugd->recp_cc_entry, &ugd->recp_cc_display_entry, ugd->recp_cc_entry_layout, ugd->recp_cc_display_entry_layout);
				next_widget = ugd->recp_cc_entry.entry;
			} else {
				next_widget = ugd->subject_entry.entry;
			}
		} else if (obj == ugd->recp_cc_entry.entry) {
			composer_recipient_change_entry(EINA_TRUE, ugd->recp_bcc_box, &ugd->recp_bcc_entry, &ugd->recp_bcc_display_entry, ugd->recp_bcc_entry_layout, ugd->recp_bcc_display_entry_layout);
			next_widget = ugd->recp_bcc_entry.entry;
		} else if (obj == ugd->recp_bcc_entry.entry) {
			next_widget = ugd->subject_entry.entry;
		}
		composer_util_focus_set_focus_with_idler(ugd, next_widget);
	} else if (ugd->is_backspace_pressed) {
		_recipient_mbe_modify_last_item(obj, ugd);
	}

	ugd->is_next_pressed = EINA_FALSE;
	ugd->is_commit_pressed = EINA_FALSE;
	ugd->is_backspace_pressed = EINA_FALSE;

	debug_leave();
}

static Evas_Object *_recipient_mbe_get(const Evas_Object *object, const EmailComposerUGD *ugd)
{
	Evas_Object *mbe = NULL;
	if (object == ugd->recp_to_entry.entry) {
		mbe = ugd->recp_to_mbe;
	} else if (object == ugd->recp_cc_entry.entry) {
		mbe = ugd->recp_cc_mbe;
	} else if (object == ugd->recp_bcc_entry.entry) {
		mbe = ugd->recp_bcc_mbe;
	}
	return mbe;
}

static Eina_Bool _recipient_mbe_unselect_last_item(Evas_Object *object, EmailComposerUGD *ugd)
{
	Evas_Object *mbe = _recipient_mbe_get(object, ugd);
	if (mbe && ugd->is_last_item_selected) {
		Elm_Object_Item *last_item = elm_multibuttonentry_last_item_get(mbe);
		if (last_item) {
			ugd->is_last_item_selected = EINA_FALSE;
			elm_multibuttonentry_item_selected_set(last_item, EINA_FALSE);
			return EINA_TRUE;
		}
	}
	return EINA_FALSE;
}

static Eina_Bool _recipient_mbe_modify_last_item(Evas_Object *object, EmailComposerUGD *ugd)
{
	Evas_Object *mbe = _recipient_mbe_get(object, ugd);
	if (mbe) {
		Elm_Object_Item *last_item = elm_multibuttonentry_last_item_get(mbe);
		if (last_item) {
			if (ugd->is_last_item_selected) {
				elm_object_item_del(last_item);
			} else {
				ugd->is_last_item_selected = EINA_TRUE;
				elm_multibuttonentry_item_selected_set(last_item, EINA_TRUE);
			}
			return EINA_TRUE;
		}
	}
	return EINA_FALSE;
}
