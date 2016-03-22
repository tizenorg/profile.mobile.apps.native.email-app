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

#include <stdarg.h>
#include <account.h>
#include <account-types.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <utils_i18n.h>

#include "email-setting.h"
#include "email-setting-utils.h"
#include "email-setting-account-set.h"

#define RGB(R, G, B) ((R << 24) | (G << 16) | (B << 8) | 0xff)
const int EMAIL_ACCOUNT_COLOR[] = {
	RGB(174, 221, 30),
	RGB(85, 166, 250),
	RGB(0, 176, 137),
	RGB(53, 101, 228),
	RGB(3, 56, 135),
	RGB(170, 0, 143),
	RGB(252, 109, 135),
	RGB(243, 30, 102),
	RGB(32, 0, 174),
	RGB(0, 88, 87),
	RGB(237, 205, 0),
	RGB(202, 128, 0),
	RGB(115, 40, 3),
	RGB(129, 130, 134)
};

typedef struct _email_setting_op_cancel_s {
	EmailSettingModule *module;
	int *op_handle;
} email_setting_op_cancel_s;

static int g_icu_ref_count = 0;
static i18n_udatepg_h setting_pattern_generator = NULL;
static const char *setting_locale = NULL;
#define CUSTOM_I18N_UDATE_IGNORE -2 /* Used temporarily since there is no substitute of UDATE_IGNORE in base-utils */

static void _entry_maxlength_reached_cb(void *data, Evas_Object *obj, void *event_info);
static void _register_popup_back_callback(Evas_Object *popup, Eext_Event_Cb back_cb, void *data);
static void _register_entry_popup_rot_callback(Evas_Object *popup, EmailSettingModule *module, const email_string_t *header);
static void _entry_popup_keypad_down_cb(void *data, Evas_Object *obj, void *event_info);
static void _entry_popup_keypad_up_cb(void *data, Evas_Object *obj, void *event_info);
static void _entry_popup_rot_cb(void *data, Evas_Object *obj, void *event_info);
static void _entry_popup_del_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _keypad_down_cb(void *data, Evas_Object *obj, void *event_info);
static void _keypad_up_cb(void *data, Evas_Object *obj, void *event_info);
static void _keypad_rot_cb(void *data, Evas_Object *obj, void *event_info);
static void _popup_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _popup_content_remove(Evas_Object *popup);
static void _popup_back_cb(void *data, Evas_Object *obj, void *event_info);
static void _popup_back_with_cancel_cb(void *data, Evas_Object *obj, void *event_info);
static void _popup_back_with_query_cancel_cb(void *data, Evas_Object *obj, void *event_info);
static void _popup_noop_cb(void *data, Evas_Object *obj, void *event_info);
static void _generate_best_pattern(const char *locale, i18n_uchar *customSkeleton, char *formattedString, void *time);

static void _wifi_launch_cb(void *data, Evas_Object *obj, void *event_info);
static void _data_setting_launch_cb(void *data, Evas_Object *obj, void *event_info);
static void _network_popup_cancel_cb(void *data, Evas_Object *obj, void *event_info);

static void _remove_locale_postfix(char *locale);

static email_string_t EMAIL_SETTING_STRING_CANCEL_USER = {PACKAGE, "IDS_EMAIL_POP_CANCELLED"};
static email_string_t EMAIL_SETTING_STRING_NO_SERVER_RESPONSE = {PACKAGE, "IDS_EMAIL_POP_UNABLE_TO_CONNECT_TO_SERVER"};
static email_string_t EMAIL_SETTING_STRING_ACCOUNT_ALREADY_EXISTS = {PACKAGE, "IDS_ST_POP_THIS_ACCOUNT_HAS_ALREADY_BEEN_ADDED"};
static email_string_t EMAIL_SETTING_STRING_AUTHENTICATION_FAILED = {PACKAGE, "IDS_ST_POP_CHECK_THE_EMAIL_ADDRESS_AND_PASSWORD_YOU_HAVE_ENTERED"};
static email_string_t EMAIL_SETTING_STRING_INCORRECT_USER_NAME_OR_PASSWORD = {PACKAGE, "IDS_ST_POP_CHECK_THE_EMAIL_ADDRESS_AND_PASSWORD_YOU_HAVE_ENTERED"};
static email_string_t EMAIL_SETTING_STRING_UNKNOWN_ERROR = {PACKAGE, "IDS_EMAIL_POP_AN_UNKNOWN_ERROR_HAS_OCCURRED"};
static email_string_t EMAIL_SETTING_STRING_EMAIL = {PACKAGE, "IDS_ST_HEADER_EMAIL"};
static email_string_t EMAIL_SETTING_STRING_CANCEL = {PACKAGE, "IDS_EMAIL_BUTTON_CANCEL"};
static email_string_t EMAIL_SETTING_STRING_MOBILE_DATA_DISABLED = {PACKAGE, "IDS_EMAIL_POP_MOBILE_DATA_IS_DISABLED_CONNECT_TO_WI_FI_NETWORK_INSTEAD_OR_ENABLE_MOBILE_DATA_AND_TRY_AGAIN"};
static email_string_t EMAIL_SETTING_STRING_MAX_ACCOUNT = {PACKAGE, "IDS_ST_POP_THE_MAXIMUM_NUMBER_OF_EMAIL_ACCOUNTS_HPD_HAS_BEEN_REACHED"};
static email_string_t EMAIL_SETTING_STRING_WIFI_REQUIRED = {PACKAGE, "IDS_EMAIL_POP_WI_FI_CONNECTION_REQUIRED_CONNECT_TO_WI_FI_NETWORK_AND_TRY_AGAIN"};
static email_string_t EMAIL_SETTING_STRING_DEVICE_STORAGE_FULL = {PACKAGE, "IDS_EMAIL_POP_THERE_IS_NOT_ENOUGH_SPACE_IN_YOUR_DEVICE_STORAGE"};
static email_string_t EMAIL_SETTING_STRING_ONLY_LOG_15_MIN = {PACKAGE, "IDS_EMAIL_POP_YOU_CAN_ONLY_LOG_IN_ONCE_EVERY_15_MINUTES"};
static email_string_t EMAIL_SETTING_STRING_VALIDATE_ACCOUNT_FAIL = {PACKAGE, "IDS_ST_POP_CHECK_THE_EMAIL_ADDRESS_AND_PASSWORD_YOU_HAVE_ENTERED"};
static email_string_t EMAIL_SETTING_STRING_CONNECTION_FAIL = {PACKAGE, "IDS_EMAIL_POP_CONNECTION_FAILED"};
static email_string_t EMAIL_SETTING_STRING_EMAIL_DELETED_FROM_SERVER = {PACKAGE, "IDS_EMAIL_POP_EMAIL_DELETED_FROM_SERVER"};
static email_string_t EMAIL_SETTING_STRING_HOST_NOT_FOUND = {PACKAGE, "IDS_EMAIL_POP_HOST_NOT_FOUND"};
static email_string_t EMAIL_SETTING_STRING_MAX_CHAR_REACHED = {PACKAGE, "IDS_EMAIL_TPOP_MAXIMUM_NUMBER_OF_CHARACTERS_REACHED"};
static email_string_t EMAIL_SETTING_STRING_NETWORK_BUSY = {PACKAGE, "IDS_EMAIL_POP_NETWORK_BUSY"};
static email_string_t EMAIL_SETTING_STRING_NETWORK_UNAVAIL = {PACKAGE, "IDS_EMAIL_TPOP_FAILED_TO_CONNECT_TO_NETWORK"};
static email_string_t EMAIL_SETTING_STRING_SERVER_NOT_AVAILABLE = {PACKAGE, "IDS_EMAIL_POP_SERVER_NOT_AVAILABLE"};
static email_string_t EMAIL_SETTING_STRING_NO_NETWORK_CONNECTION = {PACKAGE, "IDS_EMAIL_TPOP_FAILED_TO_CONNECT_TO_NETWORK"};
static email_string_t EMAIL_SETTING_STRING_FLIGHT_ENABLE = {PACKAGE, "IDS_EMAIL_POP_FLIGHT_MODE_ENABLED_OR_NETWORK_NOT_AVAILABLE"};
static email_string_t EMAIL_SETTING_STRING_CERT_FAIL = {PACKAGE, "IDS_EMAIL_POP_INVALID_OR_INACCESSIBLE_CERTIFICATE"};
static email_string_t EMAIL_SETTING_STRING_FAIL_SECURITY_POLICY = {PACKAGE, "IDS_EMAIL_POP_SECURITY_POLICY_RESTRICTS_USE_OF_THIS_ACCOUNT"};
static email_string_t EMAIL_SETTING_STRING_TLS_NOT_SUPPORTED = {PACKAGE, "IDS_EMAIL_BODY_SERVER_DOES_NOT_SUPPORT_TLS"};
static email_string_t EMAIL_SETTING_STRING_TLS_SSL_FAIL = {PACKAGE, "IDS_EMAIL_BODY_UNABLE_TO_OPEN_CONNECTION_TO_SERVER_SECURITY_ERROR_OCCURRED"};
static email_string_t EMAIL_SETTING_STRING_WARNING = {PACKAGE, "IDS_ST_HEADER_WARNING"};
static email_string_t EMAIL_SETTING_STRING_UNABLE_TO_VALIDATE = {PACKAGE, "IDS_ST_HEADER_UNABLE_TO_VALIDATE_ACCOUNT_ABB"};
static email_string_t EMAIL_SETTING_STRING_VALIDATING_ACCOUNT_ING = {PACKAGE, "IDS_ST_TPOP_VALIDATING_ACCOUNT_ING_ABB"};

/* not defined in UX */
static email_string_t EMAIL_SETTING_STRING_MANY_LOGIN_FAIL = {PACKAGE, N_("Too many login failure")};
static email_string_t EMAIL_SETTING_STRING_DATA_NETWORK = {PACKAGE, N_("Data network")};
static email_string_t EMAIL_SETTING_STRING_WIFI = {PACKAGE, N_("Wi-Fi")};

EMAIL_DEFINE_GET_EDJ_PATH(email_get_setting_theme_path, "/email-setting-theme.edj")

int setting_load_provider_list(GSList **list, const char *path, const char *filename)
{
	debug_enter();
	GSList *p_list = NULL;
	int ret = FALSE;
	int err;
	char file_path[1024] = {0,};
	char *prop = NULL;
	xmlDocPtr doc = NULL;
	xmlNodePtr root_node = NULL;
	xmlNodePtr cur_node = NULL;
	xmlNodePtr tmp_node = NULL;
	xmlNodePtr server_node = NULL;
	setting_account_provider_info_t *info = NULL;
	setting_account_server_info_t *server_info = NULL;

	gotom_if(!filename, CATCH, "filename is NULL");

	if (path == NULL) {
		snprintf(file_path, 1024, "%s/%s", EMAIL_SETTING_PROVIDER_XML_DIR_PATH, filename);
	} else {
		snprintf(file_path, 1024, "%s/%s", path, filename);
	}

	err = access(file_path, F_OK);
	gotom_if(err != 0, CATCH, "access failed: %d (%s)", ret, file_path);

	doc = xmlReadFile(file_path, "utf-8", XML_PARSE_RECOVER);
	gotom_if(!doc, CATCH, "xmlReadFile fail");

	root_node = xmlDocGetRootElement(doc);
	gotom_if(!root_node, CATCH, "xmlDocGetRootElement failed");

	cur_node = root_node->xmlChildrenNode;
	gotom_if(!cur_node, CATCH, "root_node->xmlChildrenNode is NULL");

	while (cur_node) {
		if (!xmlStrcmp(cur_node->name, (const xmlChar *)"provider")) {
			tmp_node = cur_node->xmlChildrenNode;
			info = calloc(1, sizeof(setting_account_provider_info_t));
			gotom_if(!info, CATCH, "calloc fail!");

			prop = (char *)xmlGetProp(cur_node, (const xmlChar *)"id");
			info->provider_id = prop;

			prop = (char *)xmlGetProp(cur_node, (const xmlChar *)"label");
			info->provider_label = prop;

			prop = (char *)xmlGetProp(cur_node, (const xmlChar *)"domain");
			info->provider_domain = prop;

			while (tmp_node) {
				if (!xmlStrcmp(tmp_node->name, (const xmlChar *)"incoming") ||
						!xmlStrcmp(tmp_node->name, (const xmlChar *)"outgoing")) {
					server_node = tmp_node->xmlChildrenNode;
					while (server_node) {
						if (!xmlStrcmp(server_node->name, (const xmlChar *)"server")) {
							server_info = calloc(1, sizeof(setting_account_server_info_t));
							gotom_if(!server_info, CATCH, "calloc fail!");

							prop = (char *)xmlGetProp(server_node, (const xmlChar *)"addr");
							server_info->provider_server_addr = prop;

							prop = (char *)xmlGetProp(server_node, (const xmlChar *)"type");
							server_info->provider_server_type = atoi(prop);
							FREE(prop);

							prop = (char *)xmlGetProp(server_node, (const xmlChar *)"port");
							server_info->provider_server_port = atoi(prop);
							FREE(prop);

							prop = (char *)xmlGetProp(server_node, (const xmlChar *)"security");
							server_info->provider_server_security_type = atoi(prop);
							FREE(prop);

							if (!xmlStrcmp(tmp_node->name, (const xmlChar *)"incoming"))
								info->incoming_server_list = g_slist_append(info->incoming_server_list, server_info);
							else if (!xmlStrcmp(tmp_node->name, (const xmlChar *)"outgoing"))
								info->outgoing_server_list = g_slist_append(info->outgoing_server_list, server_info);
						}
						server_node = server_node->next;
					}
				}
				tmp_node = tmp_node->next;
			}
			p_list = g_slist_append(p_list, info);
		}
		cur_node = cur_node->next;
	}
	*list = p_list;
	ret = TRUE;
	info = NULL;
	server_info = NULL;

CATCH:
	if (info)
		free(info);
	if (server_info)
		free(server_info);
	if (doc)
		xmlFreeDoc(doc);
	doc = NULL;
	if (!ret)
		setting_free_provider_list(list);
	return ret;
}

void setting_free_provider_list(GSList **list)
{
	debug_enter();
	GSList *l = NULL;
	GSList *ll = NULL;

	l = *list;
	while (l) {
		setting_account_provider_info_t *info = l->data;
		FREE(info->provider_id);
		FREE(info->provider_label);
		FREE(info->provider_domain);

		ll = info->incoming_server_list;
		while (ll) {
			setting_account_server_info_t *server_info = ll->data;
			FREE(server_info->provider_server_addr);
			FREE(server_info);
			ll = g_slist_next(ll);
		}
		g_slist_free(ll);

		ll = info->outgoing_server_list;
		while (ll) {
			setting_account_server_info_t *server_info = ll->data;
			FREE(server_info->provider_server_addr);
			FREE(server_info);
			ll = g_slist_next(ll);
		}
		g_slist_free(ll);

		FREE(info);
		l = g_slist_next(l);
	}
	g_slist_free(*list);
	*list = NULL;
}

int setting_open_icu_pattern_generator(void)
{
	debug_enter();

	++g_icu_ref_count;
	if (g_icu_ref_count > 1) {
		debug_log("icu pattern generator already opend - ref count: %d", g_icu_ref_count);
		return 1;
	}

	i18n_error_code_e status = I18N_ERROR_NONE;
	/* API to set default locale */
	status = i18n_ulocale_set_default(getenv("LC_TIME"));
	if (status != I18N_ERROR_NONE) {
		debug_critical("i18n_ulocale_set_default() failed: %d", status);
		return -1;
	}

	status = I18N_ERROR_NONE;
	/* API to get default locale */
	i18n_ulocale_get_default(&setting_locale);
	_remove_locale_postfix((char *)setting_locale);

	debug_secure("i18n_ulocale_get_default: %s", setting_locale);
	status = i18n_udatepg_create(setting_locale, &setting_pattern_generator);
	if (!setting_locale || status != I18N_ERROR_NONE) {
		debug_secure("i18n_udatepg_create() failed: %d", (status));
		return -1;
	}

	return 0;
}

int setting_close_icu_pattern_generator(void)
{
	debug_enter();

	--g_icu_ref_count;
	if (g_icu_ref_count > 0) {
		debug_log("icu pattern generator already opend - ref count: %d", g_icu_ref_count);
		return 0;
	}

	if (setting_pattern_generator) {
		i18n_udatepg_destroy(setting_pattern_generator);
		setting_pattern_generator = NULL;
	}

	return 0;
}

char *setting_get_datetime_format_text(const char *skeleton, void *time)
{
	debug_enter();
	char formattedString[256] = { 0, };
	i18n_uchar customSkeleton[128] = { 0, };
	int skeletonLength = -1;

	retvm_if(!setting_locale, NULL, "icu_locale is NULL");
	retvm_if(!skeleton, NULL, "skeleton is NULL");
	retvm_if(!time, NULL, "time is NULL");

	skeletonLength = strlen(skeleton);
	i18n_ustring_copy_ua_n(customSkeleton, skeleton, skeletonLength);
	_generate_best_pattern(setting_locale, customSkeleton, formattedString, time);

	return g_strdup(formattedString);
}

Evas_Object *setting_get_notify(email_view_t *view, const email_string_t *header,
		const email_string_t *content, int btn_num, const email_string_t *btn1_lb, Evas_Smart_Cb resp_cb1,
		const email_string_t *btn2_lb, Evas_Smart_Cb resp_cb2)
{
	debug_enter();

	EmailSettingModule *module = (EmailSettingModule *)view->module;
	Evas_Object *notify = NULL;

	if (module->popup) {
		_popup_content_remove(module->popup);
		notify = module->popup;
	} else {
		notify = elm_popup_add(module->base.navi);
		if (!notify) {
			debug_error("elm_popup_add returns NULL");
			return NULL;
		}
		elm_popup_align_set(notify, ELM_NOTIFY_ALIGN_FILL, 1.0);
		evas_object_event_callback_add(notify, EVAS_CALLBACK_DEL, _popup_del_cb, module);
	}

	if (header) {
		elm_object_domain_translatable_part_text_set(notify, "title,text", header->domain, header->id);
	}

	if (content) {
		elm_object_domain_translatable_text_set(notify, content->domain, content->id);
	}

	if (btn_num == 1) {
		Evas_Object *btn1 = elm_button_add(notify);
		elm_object_style_set(btn1, "popup");
		if (btn1_lb)
			elm_object_domain_translatable_text_set(btn1, btn1_lb->domain, btn1_lb->id);
		elm_object_part_content_set(notify, "button1", btn1);
		evas_object_smart_callback_add(btn1, "clicked", resp_cb1, view);
	}
	if (btn_num == 2) {
		Evas_Object *btn1 = elm_button_add(notify);
		elm_object_style_set(btn1, "popup");
		if (btn1_lb)
			elm_object_domain_translatable_text_set(btn1, btn1_lb->domain, btn1_lb->id);
		elm_object_part_content_set(notify, "button1", btn1);
		evas_object_smart_callback_add(btn1, "clicked", resp_cb1, view);

		Evas_Object *btn2 = elm_button_add(notify);
		if (btn2_lb)
			elm_object_domain_translatable_text_set(btn2, btn2_lb->domain, btn2_lb->id);
		elm_object_style_set(btn2, "popup");
		elm_object_part_content_set(notify, "button2", btn2);
		evas_object_smart_callback_add(btn2, "clicked", resp_cb2, view);
	}

	evas_object_show(notify);
	_register_popup_back_callback(notify, _popup_back_cb, module);

	return notify;
}

Evas_Object *setting_get_pb_process_notify(email_view_t *view, const email_string_t *header,
		int btn_num, const email_string_t *btn1_lb, Evas_Smart_Cb resp_cb1,
		const email_string_t *btn2_lb, Evas_Smart_Cb resp_cb2, EMAIL_SETTING_POPUP_BACK_TYPE back_type, int *op_handle)
{
	debug_enter();

	Evas_Object *notify = NULL;
	Evas_Object *progressbar = NULL;
	Evas_Object *layout = NULL;
	EmailSettingModule *module = (EmailSettingModule *)view->module;

	if (module->popup) {
		_popup_content_remove(module->popup);
		notify = module->popup;
	} else {
		notify = elm_popup_add(module->base.navi);
		if (!notify) {
			debug_error("elm_popup_add returns NULL");
			return NULL;
		}
		evas_object_event_callback_add(notify, EVAS_CALLBACK_DEL, _popup_del_cb, module);
		elm_popup_align_set(notify, ELM_NOTIFY_ALIGN_FILL, 1.0);
	}

	layout = elm_layout_add(notify);
	elm_layout_file_set(layout, email_get_common_theme_path(), "processing_view");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);

	if (header) {
		elm_object_part_text_set(layout, "elm.text", email_setting_gettext(*header));
		elm_object_domain_translatable_part_text_set(layout, "elm.text", header->domain, header->id);
	}

	progressbar = elm_progressbar_add(notify);
	elm_object_style_set(progressbar, "process_medium");
	evas_object_size_hint_align_set(progressbar, EVAS_HINT_FILL, 0.5);
	evas_object_size_hint_weight_set(progressbar, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(progressbar);
	elm_progressbar_pulse(progressbar, EINA_TRUE);
	elm_object_part_content_set(layout, "processing", progressbar);

	elm_object_content_set(notify, layout);

	if (btn_num == 1) {
		Evas_Object *btn1 = elm_button_add(notify);
		elm_object_style_set(btn1, "popup");
		if (btn1_lb)
			elm_object_domain_translatable_text_set(btn1, btn1_lb->domain, btn1_lb->id);
		elm_object_part_content_set(notify, "button1", btn1);
		evas_object_smart_callback_add(btn1, "clicked", resp_cb1, view);
	}
	if (btn_num == 2) {
		Evas_Object *btn1 = elm_button_add(notify);
		elm_object_style_set(btn1, "popup");
		if (btn1_lb)
			elm_object_domain_translatable_text_set(btn1, btn1_lb->domain, btn1_lb->id);
		elm_object_part_content_set(notify, "button1", btn1);
		evas_object_smart_callback_add(btn1, "clicked", resp_cb1, view);

		Evas_Object *btn2 = elm_button_add(notify);
		elm_object_style_set(btn2, "popup");
		if (btn2_lb)
			elm_object_domain_translatable_text_set(btn2, btn2_lb->domain, btn2_lb->id);
		elm_object_part_content_set(notify, "button2", btn2);
		evas_object_smart_callback_add(btn2, "clicked", resp_cb2, view);
	}

	evas_object_show(notify);

	if (back_type == POPUP_BACK_TYPE_DESTROY) {
		_register_popup_back_callback(notify, _popup_back_cb, module);
	} else if (back_type == POPUP_BACK_TYPE_DESTROY_WITH_CANCEL_OP) {
		email_setting_op_cancel_s *cancel_op = NULL;
		cancel_op = calloc(1, sizeof(email_setting_op_cancel_s));
		if (cancel_op) {
			cancel_op->module = module;
			cancel_op->op_handle = op_handle;
			module->cancel_op_list = g_slist_append(module->cancel_op_list, cancel_op);
			_register_popup_back_callback(notify, _popup_back_with_cancel_cb, cancel_op);
		}
	} else if (back_type == POPUP_BACK_TYPE_DESTROY_WITH_QUERY_CANCEL_OP) {
		_register_popup_back_callback(notify, _popup_back_with_query_cancel_cb, module);
	} else {
		_register_popup_back_callback(notify, _popup_noop_cb, module);
	}

	return notify;
}

Evas_Object *setting_get_empty_content_notify(email_view_t *view, const email_string_t *header,
		int btn_num, const email_string_t *btn1_lb, Evas_Smart_Cb resp_cb1,
		const email_string_t *btn2_lb, Evas_Smart_Cb resp_cb2)
{
	debug_enter();
	Evas_Object *notify = NULL;
	EmailSettingModule *module = (EmailSettingModule *)view->module;

	if (module->popup) {
		_popup_content_remove(module->popup);
		notify = module->popup;
	} else {
		notify = elm_popup_add(module->base.navi);
		elm_popup_align_set(notify, ELM_NOTIFY_ALIGN_FILL, 1.0);
		evas_object_event_callback_add(notify, EVAS_CALLBACK_DEL, _popup_del_cb, module);
	}

	if (header) {
		elm_object_domain_translatable_part_text_set(notify, "title,text", header->domain, header->id);
	}

	if (btn_num == 1) {
		Evas_Object *btn1 = elm_button_add(notify);
		elm_object_style_set(btn1, "popup");
		if (btn1_lb)
			elm_object_domain_translatable_text_set(btn1, btn1_lb->domain, btn1_lb->id);
		elm_object_part_content_set(notify, "button1", btn1);
		evas_object_smart_callback_add(btn1, "clicked", resp_cb1, view);
	}
	if (btn_num == 2) {
		Evas_Object *btn1 = elm_button_add(notify);
		elm_object_style_set(btn1, "popup");
		if (btn1_lb)
			elm_object_domain_translatable_text_set(btn1, btn1_lb->domain, btn1_lb->id);
		elm_object_part_content_set(notify, "button1", btn1);
		evas_object_smart_callback_add(btn1, "clicked", resp_cb1, view);

		Evas_Object *btn2 = elm_button_add(notify);
		if (btn2_lb)
			elm_object_domain_translatable_text_set(btn2, btn2_lb->domain, btn2_lb->id);
		elm_object_style_set(btn2, "popup");
		elm_object_part_content_set(notify, "button2", btn2);
		evas_object_smart_callback_add(btn2, "clicked", resp_cb2, view);
	}

	evas_object_show(notify);

	_register_popup_back_callback(notify, _popup_back_cb, module);

	return notify;
}

Evas_Object *setting_get_entry_content_notify(email_view_t *view, const email_string_t *header, const char *entry_text,
		int btn_num, const email_string_t *btn1_lb, Evas_Smart_Cb resp_cb1,
		const email_string_t *btn2_lb, Evas_Smart_Cb resp_cb2, SETTINGS_POPUP_ENTRY_TYPE popup_type)
{
	debug_enter();
	Evas_Object *notify = NULL;
	EmailSettingModule *module = (EmailSettingModule *)view->module;
	Evas_Object *btn2 = NULL;
	Evas_Object *btn1 = NULL;
	email_editfield_t entryfield;

	if (module->popup) {
		_popup_content_remove(module->popup);
		notify = module->popup;
	} else {
		notify = elm_popup_add(module->base.navi);
		elm_popup_align_set(notify, ELM_NOTIFY_ALIGN_FILL, 1.0);
		evas_object_event_callback_add(notify, EVAS_CALLBACK_DEL, _popup_del_cb, module);
	}

	if (header) {
		elm_object_domain_translatable_part_text_set(notify, "title,text", header->domain, header->id);
	}

	if (btn_num == 1) {
		btn1 = elm_button_add(notify);
		elm_object_style_set(btn1, "popup");
		if (btn1_lb)
			elm_object_domain_translatable_text_set(btn1, btn1_lb->domain, btn1_lb->id);
		elm_object_part_content_set(notify, "button1", btn1);
		evas_object_smart_callback_add(btn1, "clicked", resp_cb1, view);
	}
	if (btn_num == 2) {
		btn1 = elm_button_add(notify);
		elm_object_style_set(btn1, "popup");
		if (btn1_lb)
			elm_object_domain_translatable_text_set(btn1, btn1_lb->domain, btn1_lb->id);
		elm_object_part_content_set(notify, "button1", btn1);
		evas_object_smart_callback_add(btn1, "clicked", resp_cb1, view);

		btn2 = elm_button_add(notify);
		if (btn2_lb)
			elm_object_domain_translatable_text_set(btn2, btn2_lb->domain, btn2_lb->id);
		elm_object_style_set(btn2, "popup");
		elm_object_part_content_set(notify, "button2", btn2);
		evas_object_smart_callback_add(btn2, "clicked", resp_cb2, view);
	}

	if (popup_type == POPUP_ENRTY_PASSWORD) {
		email_common_util_editfield_create(notify, EF_PASSWORD, &entryfield);
	} else if (popup_type == POPUP_ENTRY_EDITFIELD) {
		email_common_util_editfield_create(notify, 0, &entryfield);
	}

	elm_entry_input_panel_return_key_type_set(entryfield.entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
	setting_set_entry_str(entryfield.entry, entry_text);
	elm_entry_cursor_end_set(entryfield.entry);
	elm_object_content_set(notify, entryfield.layout);
	if (btn2)
		evas_object_data_set(entryfield.entry, EMAIL_SETTING_POPUP_BUTTON, btn2);

	if (entry_text == NULL || strlen(entry_text) <= 0) {
		elm_object_disabled_set(btn2, EINA_TRUE);
		elm_entry_input_panel_return_key_disabled_set(entryfield.entry, EINA_TRUE);
	}

	evas_object_show(notify);

	_register_popup_back_callback(notify, _popup_back_cb, module);
	_register_entry_popup_rot_callback(notify, module, header);

	return notify;
}

Eina_Bool setting_get_network_failure_notify(email_view_t *view)
{
	debug_enter();
	retvm_if(!view, EINA_FALSE, "view is NULL");

	EmailSettingModule *module = (EmailSettingModule *)view->module;
	Eina_Bool ret = EINA_TRUE;
	email_network_status_e status;

	status = email_get_network_state();
	if (status == EMAIL_NETWORK_STATUS_AVAILABLE) {
		debug_log("network is available. network failure nofity is unnessary");
		ret = EINA_FALSE;
	} else {
		debug_log("network is unavailable. network status: %d", status);
		ret = EINA_TRUE;
		switch (status) {
			case EMAIL_NETWORK_STATUS_FLIGHT_MODE:
			case EMAIL_NETWORK_STATUS_MOBILE_DATA_DISABLED:
			case EMAIL_NETWORK_STATUS_ROAMING_OFF:
			case EMAIL_NETWORK_STATUS_MOBILE_DATA_LIMIT_EXCEED:
				module->popup = setting_get_notify(view, &(EMAIL_SETTING_STRING_NO_NETWORK_CONNECTION),
						&(EMAIL_SETTING_STRING_MOBILE_DATA_DISABLED),
						2,
						&(EMAIL_SETTING_STRING_WIFI), _wifi_launch_cb,
						&(EMAIL_SETTING_STRING_DATA_NETWORK), _data_setting_launch_cb);
				evas_object_data_set(module->popup, EMAIL_SETTING_POPUP_NET_ERROR_CODE_KEY, (void *)status);
				break;
			case EMAIL_NETWORK_STATUS_NO_SIM_OR_OUT_OF_SERVICE:
				module->popup = setting_get_notify(view, &(EMAIL_SETTING_STRING_NO_NETWORK_CONNECTION),
						&(EMAIL_SETTING_STRING_WIFI_REQUIRED),
						2,
						&(EMAIL_SETTING_STRING_CANCEL), _network_popup_cancel_cb,
						&(EMAIL_SETTING_STRING_WIFI), _wifi_launch_cb);
				evas_object_data_set(module->popup, EMAIL_SETTING_POPUP_NET_ERROR_CODE_KEY, (void *)status);
				break;
			default:
				break;
		}
	}

	return ret;
}

Evas_Object *setting_add_inner_layout(email_view_t *view)
{
	debug_enter();

	if (view == NULL) {
		debug_error("view == NULL");
		return NULL;
	}

	Evas_Object *ly = NULL;
	ly = elm_layout_add(view->module->navi);
	elm_layout_theme_set(ly, "layout", "application", "noindicator");
	evas_object_size_hint_weight_set(ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	return ly;
}

gboolean setting_new_acct_init(email_view_t *view)
{
	debug_enter();
	retv_if(view == NULL, FALSE);
	EmailSettingModule *module = (EmailSettingModule *)view->module;

	module->new_account = NULL;
	module->new_account = calloc(1, sizeof(email_account_t));
	retvm_if(!module->new_account, FALSE, "calloc failed");

	return TRUE;
}

gboolean setting_new_acct_final(email_view_t *view)
{
	debug_enter();
	int r = 0;

	retv_if(view == NULL, FALSE);
	EmailSettingModule *module = (EmailSettingModule *)view->module;

	if (module->new_account != NULL) {
		r = email_engine_free_account_list(&(module->new_account), 1);
		retv_if(r == FALSE, FALSE);
	}
	module->new_account = NULL;
	return TRUE;
}

gboolean setting_update_acct_list(email_view_t *view)
{
	debug_enter();
	int r = 0;

	retv_if(view == NULL, FALSE);
	EmailSettingModule *module = (EmailSettingModule *)view->module;

	if (module->account_list != NULL) {
		r = email_engine_free_account_list(&(module->account_list), module->account_count);
		retv_if(r == FALSE, FALSE);
	}

	module->account_list = NULL;
	r = email_engine_get_account_list(&(module->account_count), &(module->account_list));
	retv_if(r == FALSE, FALSE);

	return TRUE;
}

gboolean setting_get_acct_full_data(int acctid, email_account_t **_account)
{
	debug_enter();
	int r = 0;

	if (*_account != NULL) {
		r = email_engine_free_account_list(_account, 1);
		retv_if(r == FALSE, FALSE);
	}
	*_account = NULL;
	r = email_engine_get_account_full_data(acctid, _account);
	retv_if(r == FALSE, FALSE);

	return TRUE;
}

void setting_delete_enter(char *string)
{
	debug_enter();
	int i = 0;

	for (i = 0; string[i]; i++) {
		if (string[i] == 10) {
			debug_log("find enter %d", i);
			string[i] = 32;
		}
	}
}

const email_string_t *setting_get_service_fail_type(int type)
{
	debug_enter();

	const email_string_t *ret = NULL;
	if (type == EMAIL_ERROR_NETWORK_TOO_BUSY) {
		ret = &(EMAIL_SETTING_STRING_NETWORK_BUSY);
	} else if (type == EMAIL_ERROR_LOGIN_ALLOWED_EVERY_15_MINS) {
		ret = &(EMAIL_SETTING_STRING_ONLY_LOG_15_MIN);
	} else if (type == EMAIL_ERROR_LOGIN_FAILURE) {
		ret = &(EMAIL_SETTING_STRING_INCORRECT_USER_NAME_OR_PASSWORD);
	} else if (type == EMAIL_ERROR_AUTH_NOT_SUPPORTED ||
			type == EMAIL_ERROR_AUTH_REQUIRED) {
		ret = &(EMAIL_SETTING_STRING_AUTHENTICATION_FAILED);
	} else if (type == EMAIL_ERROR_MAIL_NOT_FOUND_ON_SERVER) {
		ret = &(EMAIL_SETTING_STRING_EMAIL_DELETED_FROM_SERVER);
	} else if (type == EMAIL_ERROR_NO_SUCH_HOST) {
		ret = &(EMAIL_SETTING_STRING_HOST_NOT_FOUND);
	} else if (type == EMAIL_ERROR_INVALID_SERVER) {
		ret = &(EMAIL_SETTING_STRING_SERVER_NOT_AVAILABLE);
	} else if (type == EMAIL_ERROR_MAIL_MEMORY_FULL) {
		ret = &(EMAIL_SETTING_STRING_DEVICE_STORAGE_FULL);
	} else if (type == EMAIL_ERROR_NETWORK_NOT_AVAILABLE ||
			type == EMAIL_ERROR_NO_SIM_INSERTED) {
		ret = &(EMAIL_SETTING_STRING_NETWORK_UNAVAIL);
	} else if (type == EMAIL_ERROR_TOO_MANY_LOGIN_FAILURE) {
		ret = &(EMAIL_SETTING_STRING_MANY_LOGIN_FAIL);
	} else if (type == EMAIL_ERROR_FAILED_BY_SECURITY_POLICY) {
		ret = &(EMAIL_SETTING_STRING_FAIL_SECURITY_POLICY);
	} else if (type == EMAIL_ERROR_AUTHENTICATE ||
			type == EMAIL_ERROR_VALIDATE_ACCOUNT) {
		ret = &(EMAIL_SETTING_STRING_VALIDATE_ACCOUNT_FAIL);
	} else if (type == EMAIL_ERROR_FLIGHT_MODE_ENABLE) {
		ret = &(EMAIL_SETTING_STRING_FLIGHT_ENABLE);
	} else if (type == EMAIL_ERROR_CANCELLED) {
		ret = &(EMAIL_SETTING_STRING_CANCEL_USER);
	} else if (type == EMAIL_ERROR_CERTIFICATE_FAILURE) {
		ret = &(EMAIL_SETTING_STRING_CERT_FAIL);
	} else if (type == EMAIL_ERROR_NO_RESPONSE) {
		ret = &(EMAIL_SETTING_STRING_NO_SERVER_RESPONSE);
	} else if (type == EMAIL_ERROR_CONNECTION_BROKEN ||
			type == EMAIL_ERROR_CONNECTION_FAILURE) {
		ret = &(EMAIL_SETTING_STRING_CONNECTION_FAIL);
	} else if (type == EMAIL_ERROR_DB_FAILURE) {
		ret = &(EMAIL_SETTING_STRING_ACCOUNT_ALREADY_EXISTS);
	} else if (type == EMAIL_ERROR_TLS_NOT_SUPPORTED) {
		ret = &(EMAIL_SETTING_STRING_TLS_NOT_SUPPORTED);
	} else if (type == EMAIL_ERROR_TLS_SSL_FAILURE) {
		ret = &(EMAIL_SETTING_STRING_TLS_SSL_FAIL);
	} else if (type == EMAIL_ERROR_ACCOUNT_MAX_COUNT) {
		ret = &(EMAIL_SETTING_STRING_MAX_ACCOUNT);
	} else {
		ret = &(EMAIL_SETTING_STRING_UNKNOWN_ERROR);
	}

	return ret;
}

const email_string_t *setting_get_service_fail_type_header(int type)
{
	debug_enter();

	const email_string_t *ret = NULL;
	if (type == EMAIL_ERROR_VALIDATE_ACCOUNT ||
			type == EMAIL_ERROR_AUTHENTICATE ||
			type == EMAIL_ERROR_LOGIN_FAILURE) {
		ret = &(EMAIL_SETTING_STRING_UNABLE_TO_VALIDATE);
	} else {
		ret = &(EMAIL_SETTING_STRING_WARNING);
	}

	return ret;
}

int setting_add_account_to_account_svc(email_account_t *account)
{
	debug_enter();
	int ret = ACCOUNT_ERROR_NONE;
	int account_svc_id = -1;
	account_h account_handle = NULL;

	retvm_if(account == NULL, -1, "invalid parameter!");

	ret = account_create(&account_handle);
	if (ret != ACCOUNT_ERROR_NONE) {
		debug_error("account_create failed: %d", ret);
		goto CATCH;
	}

	ret = account_set_user_name(account_handle, account->incoming_server_user_name);
	debug_warning_if(ret != ACCOUNT_ERROR_NONE, "account_set_user_name failed: %d", ret);

	ret = account_set_domain_name(account_handle, account->account_name);
	debug_warning_if(ret != ACCOUNT_ERROR_NONE, "account_set_domain_name failed: %d", ret);

	ret = account_set_email_address(account_handle, account->user_email_address);
	debug_warning_if(ret != ACCOUNT_ERROR_NONE, "account_set_email_address failed: %d", ret);

	ret = account_set_source(account_handle, "SLP EMAIL");
	debug_warning_if(ret != ACCOUNT_ERROR_NONE, "account_set_source failed: %d", ret);

	ret = account_set_package_name(account_handle, "email-setting-efl");
	debug_warning_if(ret != ACCOUNT_ERROR_NONE, "account_set_package_name failed: %d", ret);

	ret = account_set_capability(account_handle, ACCOUNT_SUPPORTS_CAPABILITY_EMAIL, ACCOUNT_CAPABILITY_ENABLED);
	debug_warning_if(ret != ACCOUNT_ERROR_NONE, "account_set_capability failed: %d", ret);

	ret = account_set_sync_support(account_handle, ACCOUNT_SYNC_STATUS_IDLE);
	debug_warning_if(ret != ACCOUNT_ERROR_NONE, "account_set_sync_support failed: %d", ret);

	if (account->logo_icon_path) {
		ret = account_set_icon_path(account_handle, account->logo_icon_path);
		debug_warning_if(ret != ACCOUNT_ERROR_NONE, "account_set_icon_path failed: %d", ret);
	}

	ret = account_insert_to_db(account_handle, &account_svc_id);
	if (ret != ACCOUNT_ERROR_NONE) {
		debug_error("account_insert_to_db failed: %d", ret);
		goto CATCH;
	}

	account->account_svc_id = account_svc_id;

CATCH:
	if (account_handle)
		account_destroy(account_handle);

	return ret;
}

int setting_delete_account_from_account_svc(int account_svc_id)
{
	debug_enter();
	int ret = ACCOUNT_ERROR_NONE;

	retvm_if(account_svc_id <= 0, -1, "invalid parameter!");

	ret = account_delete_from_db_by_id(account_svc_id);
	if (ret != ACCOUNT_ERROR_NONE) {
		debug_error("account_delete_from_db_by_id failed: %d", ret);
	}

	return ret;
}

int setting_get_available_color(int account_id)
{
	debug_enter();

	int account_color = 0;
	int color_idx = (account_id - 1) % (sizeof(EMAIL_ACCOUNT_COLOR)/sizeof(int));
	account_color = EMAIL_ACCOUNT_COLOR[color_idx];

	return account_color;
}

int setting_is_duplicate_account(const char *email_addr)
{
	debug_enter();

	retvm_if(!email_addr, -1, "invalid parameter");

	int dup_ret = 0;
	int account_count = -1;
	email_account_t *account_list = NULL;
	int i = 0;
	char *addr = NULL;

	if (!email_engine_get_account_list(&account_count, &account_list)) {
		debug_error("email_engine_get_account_list failed");
		dup_ret = -1;
		goto CATCH;
	}

	if (account_count == 0) {
		debug_log("there is no account");
		return 0;
	}

	addr = g_strstrip(g_strdup(email_addr));
	debug_secure("check dup account for: %s", addr);
	for (i = 0; i < account_count; i++) {
		if (!g_ascii_strncasecmp(account_list[i].user_email_address, addr, EMAIL_LIMIT_EMAIL_ADDRESS_LENGTH)) {
			debug_log("there is same account in account list");
			dup_ret = -1;
			goto CATCH;
		}
	}

CATCH:
	if (account_list)
		email_engine_free_account_list(&account_list, account_count);
	g_free(addr);
	return dup_ret;
}

Elm_Entry_Filter_Limit_Size *setting_set_input_entry_limit(Evas_Object *entry, int max_char_count, int max_byte_count)
{
	Elm_Entry_Filter_Limit_Size *limit_filter_data = NULL;

	retvm_if(!entry, NULL, "entry is NULL");

	limit_filter_data = calloc(1, sizeof(Elm_Entry_Filter_Limit_Size));
	retvm_if(!limit_filter_data, NULL, "memory allocation fail");

	limit_filter_data->max_char_count = max_char_count;
	limit_filter_data->max_byte_count = max_byte_count;

	elm_entry_markup_filter_append(entry, elm_entry_filter_limit_size, limit_filter_data);
	evas_object_smart_callback_add(entry, "maxlength,reached", _entry_maxlength_reached_cb, NULL);

	return limit_filter_data;
}

void setting_cancel_job_by_account_id(int account_id)
{
	debug_enter();
	int task_info_count = 0;
	email_task_information_t *task_info_arr = NULL;
	int ret = EMAIL_ERROR_NONE;
	int i = 0;

	ret = email_engine_get_task_information(&task_info_arr, &task_info_count);
	retm_if(ret != EMAIL_ERROR_NONE, "email_engine_get_task_information failed: %d", ret);

	debug_log("account_id: %d, all job will be cancelled", account_id);
	for (i = 0; i < task_info_count; i++) {
		if (task_info_arr[i].account_id == account_id) {
			debug_log("cancel job - account_id: %d, event_type: %d", task_info_arr[i].account_id, task_info_arr[i].type);
			email_engine_stop_working(task_info_arr[i].account_id, task_info_arr[i].handle);
		}

		if (account_id <= 0 && task_info_arr[i].account_id <= 0) {
			debug_log("add account cancel job - account_id: %d, event_type: %d", task_info_arr[i].account_id, task_info_arr[i].type);
			email_engine_stop_working(0, task_info_arr[i].handle);
		}
	}

	free(task_info_arr);
}

static void _register_popup_back_callback(Evas_Object *popup, Eext_Event_Cb back_cb, void *data)
{
	debug_enter();
	retm_if(!popup, "invalid parameter");

	if (back_cb) {
		eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, back_cb, data);
		evas_object_data_set(popup, EMAIL_SETTING_POPUP_BACK_CB_KEY, back_cb);
	} else {
		eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, eext_popup_back_cb, NULL);
		evas_object_data_set(popup, EMAIL_SETTING_POPUP_BACK_CB_KEY, eext_popup_back_cb);
	}
}

static void _register_entry_popup_rot_callback(Evas_Object *popup, EmailSettingModule *module, const email_string_t *header)
{
	debug_enter();

	evas_object_data_del(popup, "_header");
	evas_object_data_set(popup, "_header", header);

	evas_object_event_callback_add(popup, EVAS_CALLBACK_DEL, _entry_popup_del_cb, module);
	evas_object_smart_callback_add(module->base.conform, "virtualkeypad,state,on", _entry_popup_keypad_up_cb, module);
	evas_object_smart_callback_add(module->base.conform, "virtualkeypad,state,off", _entry_popup_keypad_down_cb, module);
	evas_object_smart_callback_add(module->base.win, "wm,rotation,changed", _entry_popup_rot_cb, module);
}

static void _entry_popup_keypad_down_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailSettingModule *module = data;
	int rot = -1;

	rot = elm_win_rotation_get(module->base.win);
	if (rot == 90 || rot == 270) {
		const email_string_t *header = (const email_string_t *)evas_object_data_get(module->popup, "_header");
		retm_if(!header, "header is NULL!");
		elm_object_domain_translatable_part_text_set(module->popup, "title,text", header->domain, header->id);
	}
	module->is_keypad = EINA_FALSE;
}

static void _entry_popup_keypad_up_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailSettingModule *module = data;
	int rot = -1;

	rot = elm_win_rotation_get(module->base.win);
	if (rot == 90 || rot == 270)
		elm_object_part_text_set(module->popup, "title,text", NULL);
	module->is_keypad = EINA_TRUE;
}

static void _entry_popup_rot_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailSettingModule *module = data;
	int rot = -1;
	const email_string_t *header = (const email_string_t *)evas_object_data_get(module->popup, "_header");
	retm_if(!header, "header is NULL!");

	rot = elm_win_rotation_get(obj);
	if (rot == 90 || rot == 270) {
		if (module->is_keypad)
			elm_object_domain_translatable_part_text_set(module->popup, "title,text", header->domain, header->id);
		else
			elm_object_part_text_set(module->popup, "title,text", NULL);
	} else {
		if (module->is_keypad)
			elm_object_domain_translatable_part_text_set(module->popup, "title,text", header->domain, header->id);
	}
}

static void _entry_popup_del_cb(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailSettingModule *module = data;

	evas_object_smart_callback_del_full(module->base.conform, "virtualkeypad,state,on", _entry_popup_keypad_up_cb, module);
	evas_object_smart_callback_del_full(module->base.conform, "virtualkeypad,state,off", _entry_popup_keypad_down_cb, module);
	evas_object_smart_callback_del_full(module->base.win, "wm,rotation,changed", _entry_popup_rot_cb, module);

	module->popup = NULL;
}
static void _entry_maxlength_reached_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	/* display warning popup */
	debug_log("entry length is max now");
	notification_status_message_post(email_setting_gettext(EMAIL_SETTING_STRING_MAX_CHAR_REACHED));
}

static void _popup_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailSettingModule *module = data;

	retm_if(!module, "module is NULL");

	module->popup = NULL;
}

static void _popup_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailSettingModule *module = data;

	retm_if(!module, "module is NULL");

	DELETE_EVAS_OBJECT(module->popup);
}

static void _popup_back_with_cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	email_setting_op_cancel_s *cancel_op = data;
	retm_if(!cancel_op, "cancel data is NULL");

	EmailSettingModule *module = cancel_op->module;
	int *op_handle = cancel_op->op_handle;

	retm_if(!module, "module is NULL");

	DELETE_EVAS_OBJECT(module->popup);

	if (op_handle) {
		debug_log("handle is initialized by cancel");
		*op_handle = EMAIL_OP_HANDLE_INITIALIZER;
	}

	module->cancel_op_list = g_slist_remove(module->cancel_op_list, cancel_op);
	free(cancel_op);

	setting_cancel_job_by_account_id(module->account_id);
}

static void _popup_back_with_query_cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailSettingModule *module = data;

	retm_if(!module, "module is NULL");

	DELETE_EVAS_OBJECT(module->popup);
}

static void _popup_noop_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailSettingModule *module = data;

	retm_if(!module, "module is NULL");

	debug_log("this popup cannot be destroyed by back key");
}

static void _generate_best_pattern(const char *locale, i18n_uchar *customSkeleton, char *formattedString, void *time)
{
	debug_enter();
	i18n_error_code_e status = I18N_ERROR_NONE;
	i18n_udate_format_h formatter;
	i18n_udate date = 0;
	i18n_uchar bestPattern[128] = { 0, };
	i18n_uchar formatted[128] = { 0, };
	int32_t bestPatternCapacity, formattedCapacity;
	int32_t bestPatternLength, formattedLength, len;

	bestPatternCapacity = (int32_t) (sizeof(bestPattern) / sizeof(bestPattern[0]));
	len = i18n_ustring_get_length(customSkeleton);
	status = i18n_udatepg_get_best_pattern(setting_pattern_generator, customSkeleton, len, bestPattern, bestPatternCapacity, &bestPatternLength);
	debug_log("bestPatternLength: %d", bestPatternLength);
	if (status != I18N_ERROR_NONE) {
		debug_critical("i18n_udatepg_get_best_pattern() failed: %d", status);
	}

	status = I18N_ERROR_NONE;
	status = i18n_udate_create(CUSTOM_I18N_UDATE_IGNORE, CUSTOM_I18N_UDATE_IGNORE, locale, NULL, -1, bestPattern, -1, &formatter);
	if (status > I18N_ERROR_NONE) { /* from the definition of U_FAILURE */
		debug_critical("i18n_udate_create() failed: %d", status);
	}

	formattedCapacity = (int32_t) (sizeof(formatted) / sizeof(formatted[0]));

	if (time) {
		time_t msg_time = *(time_t *)time;
		date = (i18n_udate)msg_time * 1000;	/* Equivalent to Date = ucal_getNow() in Milliseconds */
	}

	status = I18N_ERROR_NONE;
	status = i18n_udate_format_date(formatter, date, formatted, formattedCapacity, NULL, &formattedLength);
	debug_log("formattedLength: %d", formattedLength);
	if (status != I18N_ERROR_NONE) {
		debug_critical("i18n_udate_create() failed: %d", status);
	}
	i18n_ustring_copy_au_n(formattedString, formatted, 255);
	i18n_udate_destroy(formatter);
}

char *email_setting_gettext(email_string_t t)
{
	if (!strcmp(t.domain, SYSTEM_STRING))
		return dgettext(SYSTEM_STRING, t.id);
	else
		return dgettext(PACKAGE, t.id);
}

char *setting_get_provider_name(EmailSettingModule *module)
{
	debug_enter();

	return g_strdup(EMAIL_SETTING_STRING_EMAIL.id);
}

static void _popup_content_remove(Evas_Object *popup)
{
	debug_enter();
	Evas_Object *content = NULL;
	Evas_Object *btn1 = NULL;
	Evas_Object *btn2 = NULL;

	content = elm_object_content_get(popup);
	if (content) {
		elm_object_content_unset(popup);
		evas_object_del(content);
	}

	btn1 = elm_object_part_content_get(popup, "button1");
	if (btn1) {
		elm_object_part_content_unset(popup, "button1");
		evas_object_del(btn1);
	}

	btn2 = elm_object_part_content_get(popup, "button2");
	if (btn2) {
		elm_object_part_content_unset(popup, "button2");
		evas_object_del(btn2);
	}

	/*elm_object_part_text_set(popup, "title,text", "");
	elm_object_text_set(popup, "");*/

	Eext_Event_Cb back_cb = (Eext_Event_Cb)evas_object_data_get(popup, EMAIL_SETTING_POPUP_BACK_CB_KEY);
	if (back_cb) {
		eext_object_event_callback_del(popup, EEXT_CALLBACK_BACK, back_cb);
		evas_object_smart_callback_del(popup, "block,clicked", back_cb);
	}

}

static void _wifi_launch_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	email_view_t *view = data;
	EmailSettingModule *module = (EmailSettingModule *)view->module;
	app_control_h app_control;
	int ret;

	ret = app_control_create(&app_control);
	retm_if(ret != APP_CONTROL_ERROR_NONE, "app_control_create failed: %d", ret);

	app_control_set_operation(app_control, APP_CONTROL_OPERATION_SETTING_WIFI);
	ret = app_control_send_launch_request(app_control, NULL, NULL);
	retm_if(ret != APP_CONTROL_ERROR_NONE, "app_control_send_launch_request failed: %d", ret);

	app_control_destroy(app_control);

	DELETE_EVAS_OBJECT(module->popup);
}

static void _data_setting_launch_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	email_view_t *view = data;
	EmailSettingModule *module = (EmailSettingModule *)view->module;
	email_network_status_e status = (int)(ptrdiff_t)evas_object_data_get(module->popup, EMAIL_SETTING_POPUP_NET_ERROR_CODE_KEY);
	app_control_h app_control;
	int ret;

	ret = app_control_create(&app_control);
	retm_if(ret != APP_CONTROL_ERROR_NONE, "app_control_create failed: %d", ret);

	debug_log("network status in btn callback: %d", status);
	if (status == EMAIL_NETWORK_STATUS_FLIGHT_MODE) {
		app_control_set_app_id(app_control, "setting-flightmode-efl");
	} else if (status == EMAIL_NETWORK_STATUS_MOBILE_DATA_DISABLED ||
			status == EMAIL_NETWORK_STATUS_ROAMING_OFF) {
		app_control_set_app_id(app_control, "setting-network-efl");
		app_control_set_operation(app_control, "http://tizen.org/appcontrol/operation/setting/mobile_network");
	} else if (status == EMAIL_NETWORK_STATUS_MOBILE_DATA_LIMIT_EXCEED) {
		app_control_set_app_id(app_control, "setting-datausage-efl");
	}

	app_control_send_launch_request(app_control, NULL, NULL);

	app_control_destroy(app_control);

	DELETE_EVAS_OBJECT(module->popup);
}

static void _network_popup_cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	email_view_t *view = data;
	EmailSettingModule *module = (EmailSettingModule *)view->module;

	DELETE_EVAS_OBJECT(module->popup);
}

Elm_Genlist_Item_Class *setting_get_genlist_class_item(const char *style,
		Elm_Gen_Item_Text_Get_Cb text_get,
		Elm_Gen_Item_Content_Get_Cb content_get,
		Elm_Gen_Item_State_Get_Cb state_get,
		Elm_Gen_Item_Del_Cb del)
{
	Elm_Genlist_Item_Class *itc = NULL;
	itc = elm_genlist_item_class_new();
	retvm_if(!itc, NULL, "itc is NULL!");
	itc->item_style = style;
	itc->func.text_get = text_get;
	itc->func.content_get = content_get;
	itc->func.state_get = state_get;
	itc->func.del = del;

	return itc;
}

static void _remove_locale_postfix(char *locale)
{
	debug_enter();

	retm_if(!locale, "locale is NULL");

	if (g_str_has_suffix(locale, ".UTF-8")) {
		int i;
		for (i = strlen(locale) - 1; i >= 0; i--) {
			char tmp = locale[i];
			locale[i] = '\0';
			if (tmp == '.')
				break;
		}
	}
}

void setting_set_entry_str(Evas_Object *entry, const char *str)
{
	retm_if(!entry, "entry is NULL");

	char *markup_str = elm_entry_utf8_to_markup(str ? str : "");
	elm_entry_entry_set(entry, markup_str);
	FREE(markup_str);
}

char *setting_get_entry_str(Evas_Object *entry)
{
	retvm_if(!entry, strdup(""), "entry is NULL");

	char *entry_str = elm_entry_markup_to_utf8(elm_entry_entry_get(entry));

	if (!entry_str) {
		return strdup("");
	}

	return entry_str;
}

void setting_register_keypad_rot_cb(EmailSettingModule *module)
{
	debug_enter();
	evas_object_smart_callback_add(module->base.conform, "virtualkeypad,state,on", _keypad_up_cb, module);
	evas_object_smart_callback_add(module->base.conform, "virtualkeypad,state,off", _keypad_down_cb, module);
	evas_object_smart_callback_add(module->base.win, "wm,rotation,changed", _keypad_rot_cb, module);
}

void setting_deregister_keypad_rot_cb(EmailSettingModule *module)
{
	debug_enter();
	evas_object_smart_callback_del_full(module->base.conform, "virtualkeypad,state,on", _keypad_up_cb, module);
	evas_object_smart_callback_del_full(module->base.conform, "virtualkeypad,state,off", _keypad_down_cb, module);
	evas_object_smart_callback_del_full(module->base.win, "wm,rotation,changed", _keypad_rot_cb, module);
}

static void _keypad_down_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailSettingModule *module = data;
	int rot = -1;

	retm_if(module->popup, "popup is alive!");

	rot = elm_win_rotation_get(module->base.win);
	if (rot == 90 || rot == 270) {
		Elm_Object_Item *navi_it = elm_naviframe_top_item_get(module->base.navi);
		elm_naviframe_item_title_enabled_set(navi_it, EINA_TRUE, EINA_TRUE);
	}
	module->is_keypad = EINA_FALSE;
}

static void _keypad_up_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailSettingModule *module = data;
	int rot = -1;

	retm_if(module->popup, "popup is alive!");

	rot = elm_win_rotation_get(module->base.win);
	if (rot == 90 || rot == 270) {
		Elm_Object_Item *navi_it = elm_naviframe_top_item_get(module->base.navi);
		elm_naviframe_item_title_enabled_set(navi_it, EINA_FALSE, EINA_TRUE);
	}
	module->is_keypad = EINA_TRUE;
}

static void _keypad_rot_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	EmailSettingModule *module = data;
	int rot = -1;

	retm_if(module->popup, "popup is alive!");

	rot = elm_win_rotation_get(obj);
	if (rot == 90 || rot == 270) {
		if (module->is_keypad) {
			Elm_Object_Item *navi_it = elm_naviframe_top_item_get(module->base.navi);
			elm_naviframe_item_title_enabled_set(navi_it, EINA_FALSE, EINA_TRUE);
		} else {
			Elm_Object_Item *navi_it = elm_naviframe_top_item_get(module->base.navi);
			elm_naviframe_item_title_enabled_set(navi_it, EINA_TRUE, EINA_TRUE);
		}
	} else {
		if (module->is_keypad) {
			Elm_Object_Item *navi_it = elm_naviframe_top_item_get(module->base.navi);
			elm_naviframe_item_title_enabled_set(navi_it, EINA_TRUE, EINA_TRUE);
		}
	}
}

void setting_create_account_validation_popup(email_view_t *view, int *handle)
{
	debug_enter();

	retm_if(!view, "view is null");

	EmailSettingModule *module = (EmailSettingModule *)view->module;

#ifdef _DEBUG
	email_account_t *account = module->new_account;
	debug_secure("account name:%s", account->account_name);
	debug_secure("email address:%s", account->user_email_address);
#endif

	debug_log("Start Account Validation");
	module->popup = setting_get_pb_process_notify(view,
			&(EMAIL_SETTING_STRING_VALIDATING_ACCOUNT_ING), 0, NULL, NULL, NULL, NULL,
			POPUP_BACK_TYPE_DESTROY_WITH_CANCEL_OP, handle);
}

/* EOF */
