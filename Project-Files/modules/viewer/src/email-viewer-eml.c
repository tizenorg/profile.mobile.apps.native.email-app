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

#include "email-utils.h"
#include "email-viewer-eml.h"
#include "email-viewer-util.h"

static GList *_viewer_get_recipient_list(EmailViewerUGD *ug_data, email_address_type_t address_type)
{
	debug_enter();
	retvm_if(!ug_data, NULL, "Invalid parameter: ug_data[NULL]");

	debug_secure("addr type[%d]", address_type);
	const gchar *addr_list = NULL;

	switch (address_type) {
	case EMAIL_ADDRESS_TYPE_FROM:
		if (ug_data->mail_info) {
			addr_list = ug_data->mail_info->full_address_from;
		}
		break;
	case EMAIL_ADDRESS_TYPE_TO:
		if (ug_data->mail_info) {
			addr_list = ug_data->mail_info->full_address_to;
		}
		break;
	case EMAIL_ADDRESS_TYPE_CC:
		if (ug_data->mail_info) {
			addr_list = ug_data->mail_info->full_address_cc;
		}
		break;
	case EMAIL_ADDRESS_TYPE_BCC:
		if (ug_data->mail_info) {
			addr_list = ug_data->mail_info->full_address_bcc;
		}
		break;
	default:
		debug_log("Never come here");
		break;
	}
	retvm_if(!STR_VALID(addr_list), NULL, "Invalid parameter: addr_list[INVALID]");

	gchar *recipient = g_strdup(addr_list);
	recipient = g_strstrip(recipient);
	debug_secure("recipient (%s)", recipient);
	gchar **recipient_list = g_strsplit_set(recipient, ";", -1);
	G_FREE(recipient); /* MUST BE */

	guint index = 0;
	GList *list = NULL;
	while (STR_VALID(recipient_list[index])) {
		/*debug_secure("recipient_list[%s], index:%d", recipient_list[index], index);*/
		recipient_list[index] = g_strstrip(recipient_list[index]);
		/*debug_secure("recipient_list[%s]", recipient_list[index]);*/
		gchar *nick = NULL;
		gchar *addr = NULL;
		if (!email_get_recipient_info(recipient_list[index], &nick, &addr)) {
			debug_log("email_get_recipient_info failed");
			addr = g_strdup(recipient_list[index]);
		}
		/*debug_secure("nick[%s]", nick);*/
		/*debug_secure("addr[%s]", addr);*/
		if (!email_get_address_validation(addr)) {
			++index;
			G_FREE(nick);
			G_FREE(addr);
			continue;
		}

		email_address_info_t *addrs_info = (email_address_info_t *)calloc(1, sizeof(email_address_info_t));

		if (addrs_info == NULL) {
			G_FREE(nick);
			G_FREE(addr);
			debug_log("calloc failed");
			break;
		}

		addrs_info->address = g_strdup(addr);
		addrs_info->display_name = g_strdup(nick);
		addrs_info->address_type = address_type;
		addrs_info->contact_id = 0;
		addrs_info->storage_type = -1;

		list = g_list_append(list, addrs_info);
		++index;
		G_FREE(nick);
		G_FREE(addr);
	}

	g_strfreev(recipient_list);	/* MUST BE */
	debug_leave();
	return list;
}

email_address_info_list_t *viewer_create_address_info_list(EmailViewerUGD *ug_data)
{
	debug_enter();
	retvm_if(!ug_data, NULL, "Invalid parameter: ug_data[NULL]");

	email_mail_data_t *mail_info = ug_data->mail_info;
	retvm_if(mail_info == NULL, 0, "mail_info is NULL");
	email_address_info_list_t *address_info_list = NULL;

	address_info_list = (email_address_info_list_t *)calloc(1, sizeof(email_address_info_list_t));
	retvm_if(address_info_list == NULL, NULL, "calloc failed");

	address_info_list->from = _viewer_get_recipient_list(ug_data, EMAIL_ADDRESS_TYPE_FROM);
	address_info_list->to = _viewer_get_recipient_list(ug_data, EMAIL_ADDRESS_TYPE_TO);
	address_info_list->cc = _viewer_get_recipient_list(ug_data, EMAIL_ADDRESS_TYPE_CC);
	address_info_list->bcc = _viewer_get_recipient_list(ug_data, EMAIL_ADDRESS_TYPE_BCC);

	debug_leave();
	return address_info_list;
}

email_address_info_t *viewer_copy_address_info(const email_address_info_t *src)
{
	email_address_info_t *dst = (email_address_info_t *)
		calloc(1, sizeof(email_address_info_t));
	if (dst) {
		memcpy(dst, src, sizeof(email_address_info_t));
		dst->address = g_strdup(src->address);
		dst->display_name = g_strdup(src->display_name);
	}
	return dst;
}

void viewer_free_address_info(email_address_info_t *src)
{
	if (src) {
		FREE(src->address);
		FREE(src->display_name);
		FREE(src);
	}
}

GList *viewer_copy_recipient_list(GList *src)
{
	GList *dst = NULL;
	GList *cur = g_list_first(src);
	while (cur) {
		dst = g_list_append(dst, viewer_copy_address_info(cur->data));
		cur = g_list_next(cur);
	}
	return dst;
}

void viewer_free_recipient_list(GList *address_info)
{
	GList *cur = g_list_first(address_info);
	while (cur) {
		viewer_free_address_info(cur->data);
		cur = g_list_next(cur);
	}
	g_list_free(address_info);
}

void viewer_free_address_info_list(email_address_info_list_t *address_info_list)
{
	if (address_info_list) {
		viewer_free_recipient_list(address_info_list->from);
		viewer_free_recipient_list(address_info_list->to);
		viewer_free_recipient_list(address_info_list->cc);
		viewer_free_recipient_list(address_info_list->bcc);
		free(address_info_list);
	}
}
