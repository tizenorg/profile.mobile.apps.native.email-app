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

#include "email-debug.h"
#include "email-viewer.h"
#include "email-viewer-util.h"
#include "email-viewer-more-menu.h"
#include "email-viewer-more-menu-callback.h"
#include "email-viewer-header.h"
#include "email-viewer-recipient.h"
#include "email-viewer-eml.h"
#include "email-viewer-contents.h"
#include "email-viewer-scroller.h"

#define RECIPIENT_ADD_WITHOUT_IDLER	40
#define RECIPIENT_ADD_WITH_IDLER	1

static Elm_Genlist_Item_Class recipient_popup_itc;
static Elm_Genlist_Item_Class add_to_contact_popup_itc;

static email_string_t EMAIL_VIEWER_BUTTON_CANCEL = { PACKAGE, "IDS_EMAIL_BUTTON_CANCEL" };
static email_string_t EMAIL_VIEWER_STRING_NULL = { NULL, NULL };

/* for the recipient popup list */
typedef struct _Item_Data {
	int index;
	int highlighted;
} Item_Data;

typedef struct _Mbe_item_data {
	EmailViewerUGD *ug_data;
	int recipient_count;
	int appended_recipient_count;
	GList *recipient_info_list;
	Evas_Object *recipient_mbe;
	Ecore_Idler **recipient_append_idler;
} Mbe_item_data;

/*
 * Declaration for static functions
 */

static char *_recipient_get_mbe_string(VIEWER_TO_CC_BCC_LY recipient);
static void _recipient_assign_mbe(void *data, Evas_Object *mbe, VIEWER_TO_CC_BCC_LY recipient);
static Evas_Object *_recipient_create_mbe(void *data, VIEWER_TO_CC_BCC_LY recipient);
static Eina_Bool _recipient_scroll_correction(void *data);
static void _recipient_resize_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _recipient_popup_response_cb(void *data, Evas_Object *obj, void *event_info);
static void _recipient_mbe_deleted_cb(void *data, Evas_Object *obj, void *event_info);
static void _recipient_mbe_added_cb(void *data, Evas_Object *obj, void *event_info);
static void _recipient_mbe_selected_cb(void *data, Evas_Object *obj, void *event_info);

static char *_recipient_popup_genlist_text_get(void *data, Evas_Object *obj, const char *part);
static void _recipient_popup_genlist_sel(void *data, Evas_Object *obj, void *event_info);

static char *_recipient_add_to_contact_popup_genlist_text_get(void *data, Evas_Object *obj, const char *part);
static void _recipient_add_to_contact_popup_genlist_sel(void *data, Evas_Object *obj, void *event_info);
static Eina_Bool _recipient_smart_mbe_append(void *data);
static void _recipient_append_mbe(void *data, VIEWER_TO_CC_BCC_LY recipient, email_address_info_list_t *addrs_info_list);
static void _recipient_append_mbe_items_without_idler(void *data);

/*
 * Definition for static functions
 */

static void _recipient_popup_response_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	DELETE_EVAS_OBJECT(ug_data->notify);
	FREE(ug_data->selected_address);
	FREE(ug_data->selected_name);
	email_delete_contacts_list(&ug_data->recipient_contact_list_item);
	debug_leave();
}

static void _recipient_mbe_deleted_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(event_info == NULL, "Invalid parameter: event_info[NULL]");
	email_address_info_t *addrs_info = (email_address_info_t *)elm_object_item_data_get((const Elm_Object_Item *)event_info);
	/* When item is moved from one to another field, addrs_info is moved to the item in destination field. In case of this, addrs_info can be NULL. */
	if (addrs_info) {
		debug_log("addrs_info:%p", addrs_info);
		viewer_free_address_info(addrs_info);
	}
	debug_leave();
}

static void _recipient_mbe_added_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	retm_if(event_info == NULL, "Invalid parameter: event_info[NULL]");

	debug_leave();
}

static void _recipient_mbe_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	retm_if(event_info == NULL, "Invalid parameter: event_info[NULL]");

	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;
	email_address_info_t *addrs_info = NULL;
	Elm_Object_Item *item = (Elm_Object_Item *) event_info;
	debug_secure("selected item:%s", elm_object_item_text_get(item));
	addrs_info = elm_object_item_data_get(item);
	ug_data->recipient_address = addrs_info->address;
	ug_data->recipient_name = addrs_info->display_name;
	debug_secure("display_name:%s", ug_data->recipient_name);
	debug_secure("recipient_address:%s", ug_data->recipient_address);

	email_delete_contacts_list(&ug_data->recipient_contact_list_item);
	ug_data->recipient_contact_list_item = (email_contact_list_info_t *)email_contact_search_by_email(ug_data, ug_data->recipient_address);

	char *popup_title = NULL;
	if ((addrs_info->display_name) && (g_strcmp0(addrs_info->display_name, addrs_info->address))) {
		char *utf8_name = elm_entry_markup_to_utf8(addrs_info->display_name);
		char *utf8_str = g_strconcat(utf8_name, " <", addrs_info->address, ">", NULL);
		popup_title = elm_entry_utf8_to_markup(utf8_str);

		debug_secure("utf8_name:[%s]", utf8_name);
		debug_secure("utf8 string:[%s]", utf8_str);
		debug_secure("markup string:[%s]", popup_title);
		g_free(utf8_name);
		g_free(utf8_str);
	} else {
		popup_title = g_strdup(addrs_info->address);
	}

	email_string_t EMAIL_VIEWER_STR_NO_TRANSITION = { NULL, popup_title };
	ug_data->notify = util_create_notify_with_list(ug_data, EMAIL_VIEWER_STR_NO_TRANSITION, EMAIL_VIEWER_STRING_NULL, 1, EMAIL_VIEWER_BUTTON_CANCEL,
			_recipient_popup_response_cb, EMAIL_VIEWER_STRING_NULL, NULL, NULL);

	g_free(popup_title);

	recipient_popup_itc.item_style = "type1";
	recipient_popup_itc.func.text_get = _recipient_popup_genlist_text_get;
	recipient_popup_itc.func.content_get = NULL;
	recipient_popup_itc.func.state_get = NULL;
	recipient_popup_itc.func.del = NULL;

	Evas_Object *recipient_popup_genlist = util_notify_genlist_add(ug_data->notify);
	evas_object_data_set(recipient_popup_genlist, VIEWER_EVAS_DATA_NAME, ug_data);

	int item_cnt = 0;
	item_cnt = 3;

	int index = 0;
	Elm_Object_Item *gen_item[item_cnt];
	Item_Data *item_data = NULL;
	for (index = 0; index < item_cnt; index++) {
		item_data = MEM_ALLOC(item_data, 1);
		item_data->index = index;
		gen_item[index] = elm_genlist_item_append(recipient_popup_genlist, &recipient_popup_itc, (void *)item_data, NULL, ELM_GENLIST_ITEM_NONE, _recipient_popup_genlist_sel, (void *)ug_data);
		elm_object_item_data_set(gen_item[index], item_data);
	}

	elm_object_content_set(ug_data->notify, recipient_popup_genlist);

	debug_leave();
}

static char *_recipient_popup_genlist_text_get(void *data, Evas_Object *obj, const char *part)
{
	debug_enter();
	retvm_if(obj == NULL, NULL, "Invalid parameter: obj[NULL]");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)evas_object_data_get(obj, VIEWER_EVAS_DATA_NAME);
	retvm_if(!ug_data, NULL, "ug_data is NULL!");
	Item_Data *item_data = data;
	int index = item_data->index;
	char *menu_string = NULL;

	if (!strcmp(part, "elm.text")) {
		if (index == 0) {
			if (ug_data->recipient_contact_list_item) {
				menu_string = email_get_email_string("IDS_EMAIL_OPT_VIEW_CONTACT_DETAILS_ABB");
			} else {
				menu_string = email_get_email_string("IDS_EMAIL_OPT_ADD_TO_CONTACTS_ABB2");
			}
		} else if (index == 1) {
			menu_string = email_get_email_string("IDS_EMAIL_OPT_COMPOSE_EMAIL_ABB");
		} else if (index == 2) {
			if (recipient_is_priority_email_address(ug_data->recipient_address)) {
				menu_string = email_get_email_string("IDS_EMAIL_OPT_REMOVE_FROM_PRIORITY_SENDERS");
			} else {
				menu_string = email_get_email_string("IDS_EMAIL_OPT_ADD_TO_PRIORITY_SENDERS_ABB");
			}
		}
	}
	debug_secure("menu_string: %s", menu_string);
	debug_leave();
	return (menu_string != NULL) ? strdup(menu_string) : NULL;
}

static void _recipient_popup_genlist_sel(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	retm_if(event_info == NULL, "Invalid parameter: event_info[NULL]");

	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;

	if (item != NULL) {
		Item_Data *item_data = elm_object_item_data_get(item);
		int index = item_data->index;
		debug_log("index = %d", index);
		if (index == 0) {
			if (ug_data->recipient_contact_list_item) {
				debug_log("Contact Item Exist: index[%d]", ug_data->recipient_contact_list_item->person_id);
				char index_str[BUF_LEN_T] = { 0, };
				snprintf(index_str, sizeof(index_str), "%d", ug_data->recipient_contact_list_item->person_id);
				viewer_view_contact_detail(ug_data, g_strdup(index_str));
				email_delete_contacts_list(&ug_data->recipient_contact_list_item);
			} else {
				ug_data->create_contact_arg = CONTACTUI_REQ_ADD_EMAIL;
				DELETE_EVAS_OBJECT(ug_data->notify);
				recipient_add_to_contact_selection_popup(ug_data, ug_data->recipient_address, EMAIL_RECIPIENT_NAME);
				return;
			}
		} else if (index == 1) {
			char *utf8_name = elm_entry_markup_to_utf8(ug_data->recipient_name);
			char *utf8_str = g_strconcat(utf8_name, " <", ug_data->recipient_address, ">", NULL);
			debug_secure("utf8_name:[%s]", utf8_name);
			debug_secure("utf8 string:[%s]", utf8_str);

			viewer_send_email(ug_data, utf8_str);

			g_free(utf8_name);
			g_free(utf8_str);
		} else if (index == 2) {
			if (recipient_is_priority_email_address(ug_data->recipient_address)) {
				viewer_ctxpopup_remove_vip_rule_cb(ug_data->recipient_address, NULL, NULL);
			} else {
				viewer_ctxpopup_add_vip_rule_cb(ug_data->recipient_address, NULL, NULL);
			}
		}
	}
	DELETE_EVAS_OBJECT(ug_data->notify);
	debug_leave();
}

static char *_recipient_add_to_contact_popup_genlist_text_get(void *data, Evas_Object *obj, const char *part)
{
	debug_enter();
	retvm_if(obj == NULL, NULL, "Invalid parameter: obj[NULL]");
	int index = (int)(ptrdiff_t)data;
	char *menu_string = NULL;
	debug_log("index:%d", index);

	if (!strcmp(part, "elm.text")) {
		if (index == 0) {
			menu_string = email_get_email_string("IDS_EMAIL_OPT_CREATE_CONTACT");
		} else if (index == 1) {
			menu_string = email_get_email_string("IDS_EMAIL_OPT_UPDATE_CONTACT");
		}
	}
	debug_secure("menu_string: %s", menu_string);
	debug_leave();
	return (menu_string != NULL) ? strdup(menu_string) : NULL;
}

static void _recipient_add_to_contact_popup_genlist_sel(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	retm_if(event_info == NULL, "Invalid parameter: event_info[NULL]");

	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;

	if (item != NULL) {
		int index = (int)(ptrdiff_t)elm_object_item_data_get(item);
		debug_log("index = %d", index);

		if (index == 0) {
			viewer_add_contact(ug_data, ug_data->selected_address, ug_data->selected_name);
		} else if (index == 1) {
			viewer_update_contact(ug_data, ug_data->selected_address);
		}
	}
	DELETE_EVAS_OBJECT(ug_data->notify);
	FREE(ug_data->selected_address);
	FREE(ug_data->selected_name);
	debug_leave();
}

void recipient_add_to_contact_selection_popup(void *data, char *contact_data, VIEWER_DISPLAY_NAME name_type)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");

	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;
	char *popup_title = NULL;
	char *utf8_str = NULL;
	if (name_type == EMAIL_SENDER_NAME && ug_data->sender_display_name && ug_data->sender_address &&
			g_strcmp0(ug_data->sender_display_name, ug_data->sender_address)) {
		ug_data->selected_address = g_strdup(ug_data->sender_address);
		ug_data->selected_name = g_strdup(ug_data->sender_display_name);
		utf8_str = g_strconcat(ug_data->sender_display_name, " <", ug_data->sender_address, ">", NULL);
		popup_title = elm_entry_utf8_to_markup(utf8_str);
		debug_secure("sender_display_name:[%s]", ug_data->sender_display_name);
		debug_secure("utf8 string:[%s]", utf8_str);
	} else if (name_type == EMAIL_RECIPIENT_NAME && ug_data->recipient_name && ug_data->recipient_address &&
			g_strcmp0(ug_data->recipient_name, ug_data->recipient_address)) {
		ug_data->selected_address = g_strdup(ug_data->recipient_address);
		ug_data->selected_name = elm_entry_markup_to_utf8(ug_data->recipient_name);
		utf8_str = g_strconcat(ug_data->selected_name, " <", ug_data->recipient_address, ">", NULL);
		popup_title = elm_entry_utf8_to_markup(utf8_str);
		debug_secure("recipient_name:[%s]", ug_data->recipient_name);
		debug_secure("utf8 string:[%s]", utf8_str);
	} else {
		ug_data->selected_address = g_strdup(contact_data);
		gchar **str_tokens = g_strsplit(ug_data->selected_address, "@", -1);
		ug_data->selected_name = g_strdup(str_tokens[0]);
		popup_title = g_strdup(ug_data->selected_address);
		g_strfreev(str_tokens);
	}

	debug_secure("selected_address:[%s]", ug_data->selected_address);
	debug_secure("selected_name:[%s]", ug_data->selected_name);
	debug_secure("popup_title:[%s]", popup_title);

	email_string_t EMAIL_VIEWER_STR_NO_TRANSITION = { NULL, popup_title };
	ug_data->notify = util_create_notify_with_list(ug_data, EMAIL_VIEWER_STR_NO_TRANSITION, EMAIL_VIEWER_STRING_NULL, 1, EMAIL_VIEWER_BUTTON_CANCEL,
			_recipient_popup_response_cb, EMAIL_VIEWER_STRING_NULL, NULL, NULL);

	FREE(utf8_str);
	FREE(popup_title);

	add_to_contact_popup_itc.item_style = "type1";
	add_to_contact_popup_itc.func.text_get = _recipient_add_to_contact_popup_genlist_text_get;
	add_to_contact_popup_itc.func.content_get = NULL;
	add_to_contact_popup_itc.func.state_get = NULL;
	add_to_contact_popup_itc.func.del = NULL;

	Evas_Object *add_to_contact_popup_genlist = util_notify_genlist_add(ug_data->notify);
	evas_object_data_set(add_to_contact_popup_genlist, VIEWER_EVAS_DATA_NAME, ug_data);

	int index = 0;
	for (index = 0; index < 2; index++) {
		elm_genlist_item_append(add_to_contact_popup_genlist, &add_to_contact_popup_itc, (void *)(ptrdiff_t)index, NULL, ELM_GENLIST_ITEM_NONE, _recipient_add_to_contact_popup_genlist_sel, (void *)ug_data);
	}

	elm_object_content_set(ug_data->notify, add_to_contact_popup_genlist);
	debug_leave();
}

static void _viewer_append_mbe_item(
		EmailViewerUGD *ug_data, Evas_Object *mbe,
		const email_address_info_t *addrs_info)
{
	retm_if(!addrs_info, "invalid param");

	email_address_info_t *mbe_info = (email_address_info_t *)
		calloc(1, sizeof(email_address_info_t));
	retm_if(!mbe_info, "mbe_info is NULL");

	debug_secure("type:%d storage:%d contact:%d addr:%s name:%s",
			addrs_info->address_type, addrs_info->storage_type,
			addrs_info->contact_id, addrs_info->address,
			addrs_info->display_name);

	mbe_info->address_type = addrs_info->address_type;
	mbe_info->address = g_strdup(addrs_info->address);
	mbe_info->storage_type = addrs_info->storage_type;
	if (!STR_VALID(addrs_info->display_name)) {
		mbe_info->display_name = elm_entry_utf8_to_markup(addrs_info->display_name);
	} else {
		mbe_info->display_name = NULL;
	}

	if (!mbe_info->display_name || mbe_info->display_name[0] == '\0') {
		FREE(mbe_info->display_name);
		mbe_info->display_name = g_strdup(addrs_info->address);
	}

	debug_log("mbe_info:%p", mbe_info);

	bool added = false;
	if (mbe_info->display_name && *mbe_info->display_name) {
		added = elm_multibuttonentry_item_append(mbe, mbe_info->display_name, NULL, mbe_info) != NULL;
	}

	if (!added) {
		viewer_free_address_info(mbe_info);
	}
}

static Eina_Bool _recipient_smart_mbe_append(void *data)
{
	debug_enter();

	retvm_if(data == NULL, ECORE_CALLBACK_CANCEL, "data is NULL");
	Mbe_item_data *mbe_item_data = (Mbe_item_data *)data;
	EmailViewerUGD *ug_data = mbe_item_data->ug_data;
	int add_rcpnt_cnt = RECIPIENT_ADD_WITH_IDLER;

	if ((mbe_item_data->recipient_count - mbe_item_data->appended_recipient_count) < add_rcpnt_cnt) {
		add_rcpnt_cnt = mbe_item_data->recipient_count - mbe_item_data->appended_recipient_count;
	}

	int i = 0;
	for (i = 0; i < add_rcpnt_cnt; i++) {
		_viewer_append_mbe_item(ug_data, mbe_item_data->recipient_mbe,
				g_list_nth_data(mbe_item_data->recipient_info_list, mbe_item_data->appended_recipient_count + i));
	}

	mbe_item_data->appended_recipient_count = mbe_item_data->appended_recipient_count + add_rcpnt_cnt;
	if (mbe_item_data->appended_recipient_count < mbe_item_data->recipient_count) {
		return ECORE_CALLBACK_RENEW;
	}

	*(mbe_item_data->recipient_append_idler) = NULL;
	viewer_free_recipient_list(mbe_item_data->recipient_info_list);
	free(mbe_item_data);

	return ECORE_CALLBACK_CANCEL;
}

static void _recipient_append_mbe_items_without_idler(void *data)
{
	debug_enter();
	retm_if(data == NULL, "data is NULL");
	Mbe_item_data *mbe_item_data = (Mbe_item_data *)data;
	EmailViewerUGD *ug_data = mbe_item_data->ug_data;

	if (mbe_item_data->recipient_count < RECIPIENT_ADD_WITHOUT_IDLER) {
		mbe_item_data->appended_recipient_count = mbe_item_data->recipient_count;
	} else {
		mbe_item_data->appended_recipient_count = RECIPIENT_ADD_WITHOUT_IDLER;
	}

	int i = 0;
	for (i = 0; i < mbe_item_data->appended_recipient_count; i++) {
		_viewer_append_mbe_item(ug_data, mbe_item_data->recipient_mbe,
				g_list_nth_data(mbe_item_data->recipient_info_list, i));
	}
	debug_leave();
}

static void _recipient_append_mbe(void *data, VIEWER_TO_CC_BCC_LY recipient, email_address_info_list_t *addrs_info_list)
{
	debug_enter();
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;
	Evas_Object *recipient_mbe = NULL;
	GList *recipient_info_list = NULL;
	int n_recipient_list = 0;

	if (recipient == EMAIL_VIEWER_TO_LAYOUT) {
		n_recipient_list = ug_data->to_recipients_cnt;
		recipient_mbe = ug_data->to_mbe;
		recipient_info_list = viewer_copy_recipient_list(addrs_info_list->to);
		DELETE_IDLER_OBJECT(ug_data->to_recipient_idler);
	} else if (recipient == EMAIL_VIEWER_CC_LAYOUT) {
		n_recipient_list = ug_data->cc_recipients_cnt;
		recipient_mbe = ug_data->cc_mbe;
		recipient_info_list = viewer_copy_recipient_list(addrs_info_list->cc);
		DELETE_IDLER_OBJECT(ug_data->cc_recipient_idler);
	} else if (recipient == EMAIL_VIEWER_BCC_LAYOUT) {
		n_recipient_list = ug_data->bcc_recipients_cnt;
		recipient_mbe = ug_data->bcc_mbe;
		recipient_info_list = viewer_copy_recipient_list(addrs_info_list->bcc);
		DELETE_IDLER_OBJECT(ug_data->bcc_recipient_idler);
	}

	if (n_recipient_list > EMAIL_VIEW_MAX_TO_COUNT) {
		n_recipient_list = EMAIL_VIEW_MAX_TO_COUNT;
	}
	debug_log("n_recipient_list:%d", n_recipient_list);
	retm_if(n_recipient_list <= 0, "recipient count is 0");

	Mbe_item_data *mbe_item_data = (Mbe_item_data *)calloc(1, sizeof(Mbe_item_data));
	retm_if(mbe_item_data == NULL, "mbe_item_data is NULL");

	mbe_item_data->recipient_mbe = recipient_mbe;
	mbe_item_data->recipient_info_list = recipient_info_list;
	mbe_item_data->ug_data = ug_data;
	mbe_item_data->recipient_count = n_recipient_list;
	mbe_item_data->appended_recipient_count = 0;

	_recipient_append_mbe_items_without_idler(mbe_item_data);

	bool run = false;
	if (mbe_item_data->appended_recipient_count != mbe_item_data->recipient_count) {
		if (recipient == EMAIL_VIEWER_TO_LAYOUT) {
			ug_data->to_recipient_idler = ecore_idler_add(_recipient_smart_mbe_append, (void *)mbe_item_data);
			mbe_item_data->recipient_append_idler = &(ug_data->to_recipient_idler);
			run = true;
		} else if (recipient == EMAIL_VIEWER_CC_LAYOUT) {
			ug_data->cc_recipient_idler = ecore_idler_add(_recipient_smart_mbe_append, (void *)mbe_item_data);
			mbe_item_data->recipient_append_idler = &(ug_data->cc_recipient_idler);
			run = true;
		} else if (recipient == EMAIL_VIEWER_BCC_LAYOUT) {
			ug_data->bcc_recipient_idler = ecore_idler_add(_recipient_smart_mbe_append, (void *)mbe_item_data);
			mbe_item_data->recipient_append_idler = &(ug_data->bcc_recipient_idler);
			run = true;
		}
	}

	if (!run) {
		viewer_free_recipient_list(mbe_item_data->recipient_info_list);
		free(mbe_item_data);
	}

	debug_leave();
}

static char *_recipient_get_mbe_string(VIEWER_TO_CC_BCC_LY recipient)
{
	char *recipient_string = NULL;
	if (recipient == EMAIL_VIEWER_TO_LAYOUT)
		recipient_string = g_strdup(email_get_email_string("IDS_EMAIL_BODY_TO_M_RECIPIENT"));
	else if (recipient == EMAIL_VIEWER_CC_LAYOUT)
		recipient_string = g_strdup(email_get_email_string("IDS_EMAIL_BODY_CC"));
	else
		recipient_string = g_strdup(email_get_email_string("IDS_EMAIL_BODY_BCC"));
	return recipient_string;
}

static void _recipient_assign_mbe(void *data, Evas_Object *mbe, VIEWER_TO_CC_BCC_LY recipient)
{
	retm_if(mbe == NULL, "Invalid parameter: data[NULL]");
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	if (recipient == EMAIL_VIEWER_TO_LAYOUT)
		ug_data->to_mbe = mbe;
	else if (recipient == EMAIL_VIEWER_CC_LAYOUT)
		ug_data->cc_mbe = mbe;
	else
		ug_data->bcc_mbe = mbe;
}

static Evas_Object *_recipient_create_mbe(void *data, VIEWER_TO_CC_BCC_LY recipient)
{
	debug_enter();
	retvm_if(data == NULL, NULL, "Invalid parameter: data[NULL]");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	Evas_Object *mbe = elm_multibuttonentry_add(ug_data->main_bx);
	elm_multibuttonentry_editable_set(mbe, EINA_FALSE);

	char *recipient_string = _recipient_get_mbe_string(recipient);
	elm_object_text_set(mbe, recipient_string);

	evas_object_smart_callback_add(mbe, "item,clicked", _recipient_mbe_selected_cb, ug_data);
	evas_object_smart_callback_add(mbe, "item,added", _recipient_mbe_added_cb, ug_data);
	evas_object_smart_callback_add(mbe, "item,deleted", _recipient_mbe_deleted_cb, ug_data);

	elm_object_focus_allow_set(mbe, EINA_FALSE);
	if (elm_win_focus_highlight_enabled_get(ug_data->base.module->win)) {
		elm_object_tree_focus_allow_set(mbe, EINA_TRUE);
	} else {
		elm_object_tree_focus_allow_set(mbe, EINA_FALSE);
	}
	_recipient_assign_mbe(ug_data, mbe, recipient);

	return mbe;
}

static Eina_Bool _recipient_scroll_correction(void *data)
{
	debug_enter();
	retvm_if(data == NULL, ECORE_CALLBACK_CANCEL, "Invalid parameter: data[NULL]");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;
	viewer_set_vertical_scroller_position(ug_data);
	ug_data->rcpt_scroll_corr = NULL;

	return ECORE_CALLBACK_CANCEL;
}

static void _recipient_resize_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	DELETE_TIMER_OBJECT(ug_data->rcpt_scroll_corr);
	ug_data->rcpt_scroll_corr = ecore_timer_add(0.0, _recipient_scroll_correction, ug_data);
}


/*
 * Definition for exported functions
 */

Evas_Object *recipient_create_address(void *data, email_address_info_list_t *addrs_info_list, VIEWER_TO_CC_BCC_LY recipient)
{
	debug_enter();
	retvm_if(data == NULL, NULL, "Invalid parameter: data[NULL]");
	retvm_if(addrs_info_list == NULL, NULL, "Invalid parameter: addrs_info_list[NULL]");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	Evas_Object *layout = viewer_load_edj(ug_data->main_bx, email_get_viewer_theme_path(), "ev/recipient/base", EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(layout);

	Evas_Object *mbe = _recipient_create_mbe(ug_data, recipient);
	elm_object_part_content_set(layout, "ev.swallow.content", mbe);

	_recipient_append_mbe(ug_data, recipient, addrs_info_list);
	evas_object_event_callback_add(layout, EVAS_CALLBACK_RESIZE, _recipient_resize_cb, ug_data);

	debug_leave();
	return layout;
}

bool recipient_is_priority_email_address(char *email_address)
{
	debug_enter();
	retvm_if(email_address == NULL, FALSE, "Invalid parameter: email_address[NULL]");

	int count, i;
	email_rule_t *rule_list = NULL;
	bool is_vip = FALSE;

	/* get the rule list from service */
	if (email_get_rule_list(&rule_list, &count) < 0) {
		debug_log("email_get_rule_list failed");
	} else {
		if (count > 0) {
			for (i = 0; i < count; i++) {
				if (rule_list[i].type == EMAIL_PRIORITY_SENDER) {
					debug_secure("vip address %s", rule_list[i].value2);

					if (g_strcmp0(rule_list[i].value2, email_address) == 0) {
						debug_secure("[%s] already set", rule_list[i].value2);
						is_vip = TRUE;
						break;
					}
				}
			}
		}
		/* free email rule_list */
		email_free_rule(&rule_list, count);
	}
	debug_leave();
	return is_vip;
}

bool recipient_is_blocked_email_address(char *email_address)
{
	debug_enter();
	retvm_if(email_address == NULL, FALSE, "Invalid parameter: email_address[NULL]");

	int count, i;
	email_rule_t *rule_list = NULL;
	bool is_blocked = FALSE;

	/* get the rule list from service */
	if (email_get_rule_list(&rule_list, &count) < 0) {
		debug_log("email_get_rule_list failed");
	} else {
		if (count > 0) {
			for (i = 0; i < count; i++) {
				if (rule_list[i].type == EMAIL_FILTER_FROM) {
					debug_secure("block address %s", rule_list[i].value2);

					if (g_strcmp0(rule_list[i].value2, email_address) == 0) {
						debug_secure("[%s] already blocked", rule_list[i].value2);
						is_blocked = TRUE;
						break;
					}
				}
			}
		}
		/* free email rule_list */
		email_free_rule(&rule_list, count);
	}
	debug_leave();
	return is_blocked;
}

void recipient_clear_multibuttonentry_data(Evas_Object *obj)
{
	retm_if(obj == NULL, "Invalid parameter: obj[NULL]");

	Elm_Object_Item *mbe_item = elm_multibuttonentry_first_item_get(obj);
	while (mbe_item) {
		Elm_Object_Item *del_item = mbe_item;
		mbe_item = elm_multibuttonentry_item_next_get(mbe_item);
		elm_object_item_del(del_item);
		del_item = NULL;
	}
	elm_entry_entry_set(elm_multibuttonentry_entry_get(obj), NULL);
}

void recipient_delete_callbacks(void *data)
{
	retm_if(data == NULL, "Invalid parameter: data[NULL]");
	EmailViewerUGD *ug_data = (EmailViewerUGD *)data;

	if (ug_data->to_ly)
		evas_object_event_callback_del(ug_data->to_ly, EVAS_CALLBACK_RESIZE, _recipient_resize_cb);
	if (ug_data->cc_ly)
		evas_object_event_callback_del(ug_data->cc_ly, EVAS_CALLBACK_RESIZE, _recipient_resize_cb);
	if (ug_data->bcc_ly)
		evas_object_event_callback_del(ug_data->bcc_ly, EVAS_CALLBACK_RESIZE, _recipient_resize_cb);
}
