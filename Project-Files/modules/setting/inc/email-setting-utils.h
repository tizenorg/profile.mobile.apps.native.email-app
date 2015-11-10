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

#ifndef __EMAIL_SETTING_UTILS_H__
#define __EMAIL_SETTING_UTILS_H__

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "email-setting.h"

#define TEXT_ITEM_DEFAULT_SIZE 40
#define SYNC_FAILED_TEXT_STYLE "<color=#e12222>%s</color>"
#define WIFI_AUTODOWNLOAD_TEXT_STYLE "<font_size=%d><color=#000000>%s</color></font_size>"

#define R_MASKING(val) (((val) & 0xff000000) >> 24)
#define G_MASKING(val) (((val) & 0x00ff0000) >> 16)
#define B_MASKING(val) (((val) & 0x0000ff00) >> 8)
#define A_MASKING(val) (((val) & 0x000000ff))

#define EMAIL_OP_HANDLE_INITIALIZER (0xcafebabe)

#ifndef APP_CONTROL_OPERATION_SETTING_WIFI
#define APP_CONTROL_OPERATION_SETTING_WIFI "http://tizen.org/appcontrol/operation/setting/wifi"
#endif

#define EMAIL_SETTING_DATETIME_FORMAT "yMMd"
#define EMAIL_SETTING_DATETIME_FORMAT_12 "hm"
#define EMAIL_SETTING_DATETIME_FORMAT_24 "Hm"

#define EMAIL_SETTING_POPUP_BACK_CB_KEY "popup_back_cb"
#define EMAIL_SETTING_POPUP_NET_ERROR_CODE_KEY "net_error_code"
#define EMAIL_SETTING_POPUP_BUTTON "popup_button"

#define EMAIL_SETTING_PROVIDER_XML_DIR_PATH email_get_misc_dir()
#define EMAIL_SETTING_DEFAULT_PROVIDER_XML_FILENAME "provider_list_default.xml"

#define EMAIL_SETTING_EMAIL_ADDRESS_REGEX "^[[:alpha:]0-9._%+-]+@[[:alpha:]0-9.-]+\\.[[:alpha:]]{2,6}$"

#define EMAIL_SETTING_PRINT_ACCOUNT_INFO(account) \
	do { \
		if (account) { \
			debug_secure("*************Account Details*************"); \
			debug_secure("1. Account Id              : %d", account->account_id); \
			debug_secure("2. Account Name            : %s", account->account_name); \
			debug_secure("3. Sync Disabled           : %d", account->sync_disabled); \
			debug_secure("4. User Email Address      : %s", account->user_email_address); \
			debug_secure("5. Incoming server addr    : %s", account->incoming_server_address); \
			debug_secure("6. Incoming server pwd     : %s", account->incoming_server_password); \
			debug_secure("7. Outgoing server addr    : %s", account->outgoing_server_address); \
			debug_secure("8. Outgoing server pwd     : %s", account->outgoing_server_password); \
			debug_secure("*************Sending Option Details*************"); \
			debug_secure("1. Priority                : %d", account->options.priority); \
			debug_secure("2. Keep a copy             : %d", account->options.keep_local_copy); \
			debug_secure("3. Reply with body         : %d", account->options.reply_with_body); \
			debug_secure("4. Forward with files      : %d", account->options.forward_with_files); \
			debug_secure("5. Get read report         : %d", account->options.req_read_receipt); \
			debug_secure("6. Get delivery report     : %d", account->options.req_delivery_receipt); \
			debug_secure("7. Add my namecard         : %d", account->options.add_myname_card); \
			debug_secure("8. Always BCC myself       : %d", account->options.add_my_address_to_bcc); \
			debug_secure("9. Add signature           : %d", account->options.add_signature); \
			debug_secure("10. Signature              : %s", account->options.signature); \
			debug_secure("*************Receiving Option Details*************"); \
			debug_secure("1. Check Interval          : %d", account->check_interval); \
			debug_secure("2. Email size              : %d", account->options.download_limit); \
			debug_secure("3. Downloadflag            : %d", account->auto_download_size); \
			debug_secure("4. Keep on server          : %d", account->keep_mails_on_pop_server_after_download); \
		} else { \
			debug_warning("account is NULL"); \
		} \
	} while (0)

typedef enum {
	FOLDER_TYPE_NONE = 0,
	FOLDER_TYPE_INBOX,
	FOLDER_TYPE_SENTBOX,
	FOLDER_TYPE_TRASH,
	FOLDER_TYPE_DRAFTBOX,
	FOLDER_TYPE_SPAMBOX,
	FOLDER_TYPE_OUTBOX,
	FOLDER_TYPE_ALL_EMAILS,
	FOLDER_TYPE_SEARCH_RESULT,
	FOLDER_TYPE_USER_DEFINED = 0xFF,
	FOLDER_TYPE_MAX
} SETTING_FOLDER_TYPE_ID;	/* TODO: This enum can be replaced with email_mailbox_type_e. */

typedef enum {
	POPUP_ENTRY_EDITFIELD,
	POPUP_ENRTY_PASSWORD
} SETTINGS_POPUP_ENTRY_TYPE;

typedef enum {
	POPUP_BACK_TYPE_NO_OP,
	POPUP_BACK_TYPE_DESTROY,
	POPUP_BACK_TYPE_DESTROY_WITH_CANCEL_OP,
	POPUP_BACK_TYPE_DESTROY_WITH_QUERY_CANCEL_OP
} EMAIL_SETTING_POPUP_BACK_TYPE;

/**
 * @brief Prototype of email setting get account info callback function
 */
typedef void (*setting_get_account_info_cb)(void *user_data, int result);

/**
 * @brief Email setting account info data
 */
typedef struct _setting_get_account_info_t {
	email_account_t *account;				/**< account structure pointer */
	email_protocol_config_t *imap_conf;		/**< IMAP protocol config structure pointer */
	email_protocol_config_t *pop_conf;		/**< POP3 protocol config structure pointer */
	email_protocol_config_t *smtp_conf;		/**< SMTP protocol config structure pointer */
	void *user_data;
	setting_get_account_info_cb cb;
} setting_get_account_info_t;

/**
 * @brief Email setting account server info
 */
typedef struct _setting_account_server_info_t {
	char *provider_server_addr;			/**< provider server address name*/
	int provider_server_type;			/**< provider server type */
	int provider_server_port;			/**< provider server port */
	int provider_server_security_type;	/**< provider server security type */
} setting_account_server_info_t;

/**
 * @brief Email setting account provider info
 */
typedef struct _setting_account_provider_info_t {
	char *provider_id;				/**< provider ID name */
	char *provider_label;			/**< provider label name */
	char *provider_domain;			/**< provider domain name */
	GSList *incoming_server_list;	/**< incoming server list */
	GSList *outgoing_server_list;	/**< outgoing server list */
} setting_account_provider_info_t;

/**
 * @brief elm util function
 */
const char *email_get_setting_theme_path();

/**
 * @brief Provides popup notification
 * @param[in]	vd			View data
 * @param[in]	header		Email setting string data for popup header
 * @param[in]	content		Email setting string data for popup content
 * @param[in]	btn_num		Count of buttons
 * @param[in]	btn1_lb		Email setting string data for button #1
 * @param[in]	resp_cb1	Evas_Smart callback function data for button #1
 * @param[in]	btn2_lb		Email setting string data for button #2
 * @param[in]	resp_cb2	Evas_Smart callback function data for button #2
 * @return Evas_Object with suitable popup notification, otherwise NULL
 */
Evas_Object *setting_get_notify(email_view_t *vd, const email_setting_string_t *header,
		const email_setting_string_t *content, int btn_num, const email_setting_string_t *btn1_lb, Evas_Smart_Cb resp_cb1,
		const email_setting_string_t *btn2_lb, Evas_Smart_Cb resp_cb2);

/**
 * @brief Provides process popup notification
 * @param[in]	vd			View data
 * @param[in]	header		Email setting string data for popup header
 * @param[in]	btn_num		Count of buttons
 * @param[in]	btn1_lb		Email setting string data for button #1
 * @param[in]	resp_cb1	Evas_Smart callback function data for button #1
 * @param[in]	btn2_lb		Email setting string data for button #2
 * @param[in]	resp_cb2	Evas_Smart callback function data for button #2
 * @param[in]	back_type	Popup back type
 * @param[in]	op_handle	The data pointer to be passed to cb func.
 * @return Evas_Object with suitable popup notification, otherwise NULL
 */
Evas_Object *setting_get_pb_process_notify(email_view_t *vd, const email_setting_string_t *header,
		int btn_num, const email_setting_string_t *btn1_lb, Evas_Smart_Cb resp_cb1,
		const email_setting_string_t *btn2_lb, Evas_Smart_Cb resp_cb2, EMAIL_SETTING_POPUP_BACK_TYPE back_type, int *op_handle);

/**
 * @brief Provides empty popup notification
 * @param[in]	vd			View data
 * @param[in]	header		Email setting string data for popup header
 * @param[in]	btn_num		Count of buttons
 * @param[in]	btn1_lb		Email setting string data for button #1
 * @param[in]	resp_cb1	Evas_Smart callback function data for button #1
 * @param[in]	btn2_lb		Email setting string data for button #2
 * @param[in]	resp_cb2	Evas_Smart callback function data for button #2
 * @return Evas_Object with suitable popup notification, otherwise NULL
 */
Evas_Object *setting_get_empty_content_notify(email_view_t *vd, const email_setting_string_t *header,
		int btn_num, const email_setting_string_t *btn1_lb, Evas_Smart_Cb resp_cb1,
		const email_setting_string_t *btn2_lb, Evas_Smart_Cb resp_cb2);

/**
 * @brief Provides entry popup notification
 * @param[in]	vd			View data
 * @param[in]	header		Email setting string data for popup header
 * @param[in]	btn_num		Count of buttons
 * @param[in]	btn1_lb		Email setting string data for button #1
 * @param[in]	resp_cb1	Evas_Smart callback function data for button #1
 * @param[in]	btn2_lb		Email setting string data for button #2
 * @param[in]	resp_cb2	Evas_Smart callback function data for button #2
 * @param[in]	popup_type	Popup entry type
 * @return Evas_Object with suitable popup notification, otherwise NULL
 */
Evas_Object *setting_get_entry_content_notify(email_view_t *vd, const email_setting_string_t *header,const char *entry_text,
		int btn_num, const email_setting_string_t *btn1_lb, Evas_Smart_Cb resp_cb1,
		const email_setting_string_t *btn2_lb, Evas_Smart_Cb resp_cb2, SETTINGS_POPUP_ENTRY_TYPE popup_type);

/**
 * @brief Provides network failure popup notification
 * @param[in]	vd		View data
 * @return EINA_TRUE when network is unavailable, otherwise EINA_FALSE
 */
Eina_Bool setting_get_network_failure_notify(email_view_t *vd);

/**
 * @brief Provides layout creation from Naviframe as parent
 * @remark Layout loaded with function elm_layout_theme_set(layout, "layout", "application", "noindicator")
 * @remark Size hint weight is setted with EVAS_HINT_EXPAND
 * @param[in]	vd		View data
 * @return Evas_Object layout or NULL if none or an error occurred
 */
Evas_Object *setting_add_inner_layout(email_view_t *vd);

/**
 * @brief Markup filter function for text inserted in the entry
 * @param[in]	entry				Entry object
 * @param[in]	max_char_count		limits by character count (disabled if 0)
 * @param[in]	max_char_count		limits by bytes count (disabled if 0)
 * @return Elm_Entry_Filter_Limit_Size structure that used as text filter
 */
Elm_Entry_Filter_Limit_Size *setting_set_input_entry_limit(Evas_Object *entry, int max_char_count, int max_byte_count);

/**
 * @brief Create genlist class item with appropriate settings
 * @param[in]	style			Defines the name of the item style
 * @param[in]	text_get		Represents function callback for the text fetching class function
 * @param[in]	content_get		Represents function callback for the content fetching class function
 * @param[in]	state_get		Represents the state fetching class function
 * @param[in]	del				Represents the deletion class function
 * @return Elm_Genlist_Item_Class item or NULL if none or an error occurred
 */
Elm_Genlist_Item_Class *setting_get_genlist_class_item(const char *style,
		Elm_Gen_Item_Text_Get_Cb text_get,
		Elm_Gen_Item_Content_Get_Cb content_get,
		Elm_Gen_Item_State_Get_Cb state_get,
		Elm_Gen_Item_Del_Cb del);

/**
 * @brief Set string to entry and convert a UTF-8 string into markup (HTML-like)
 * @param[in]	entry	Entry object
 * @param[in]	str		The string (in UTF-8) to be converted
 */
void setting_set_entry_str(Evas_Object *entry, const char *str);

/**
 * @brief Get from entry converted string from markup (HTML-like) to UTF-8
 * @param[in]	entry	Entry object
 * @return The converted string (in UTF-8). It should be freed.
 */
char *setting_get_entry_str(Evas_Object *entry);

/**
 * @brief Provides loading for supported providers list.
 * @param[in]	list		List of providers to process
 * @param[in]	path		Path to dir with providers XML
 * @param[in]	filename	XML filename with providers
 * @return TRUE on success or FALSE if none or an error occurred
 */
int setting_load_provider_list(GSList **list, const char *path, const char *filename);

/**
 * @brief Deleting providers list.
 * @param[in]	list	List of providers to process
 */
void setting_free_provider_list(GSList **list);

/**
 * @brief Allocation of memory for new account.
 * @param[in]	vd	View data
 * @return TRUE on success or FALSE if none or an error occurred
 */
gboolean setting_new_acct_init(email_view_t *vd);

/**
 * @brief Freeing an account data.
 * @param[in]	vd	View data
 * @return TRUE on success or FALSE if none or an error occurred
 */
gboolean setting_new_acct_final(email_view_t *vd);

/**
 * @brief Update a list with account data.
 * @param[in]	vd	View data
 * @return TRUE on success or FALSE if none or an error occurred
 */
gboolean setting_update_acct_list(email_view_t *vd);

/**
 * @brief Get an account data
 * @param[in]	acctid		Account Id
 * @param[out]	_account	Email account data
 * @return TRUE on success or FALSE if none or an error occurred
 */
gboolean setting_get_acct_full_data(int acctid, email_account_t **_account);

/**
 * @brief Check the string and replace all new line character to space character
 * @param[in]	string		String to be processed
 */
void setting_delete_enter(char *string);

/**
 * @brief Get service failure type structure
 * @param[in]	type		Type that represent service fail
 * @return Structure with failure string on success or NULL if an error occurred
 */
const email_setting_string_t *setting_get_service_fail_type(int type);

/**
 * @brief Get service failure type structure for header
 * @param[in]	type		Type that represent service fail
 * @return Structure with failure string on success or NULL if an error occurred
 */
const email_setting_string_t *setting_get_service_fail_type_header(int type);

/**
 * @brief Inserts the account details to the account database.
 * @param[in]	account		Email account data
 * @return 0 on success, otherwise a negative error value
 */
int setting_add_account_to_account_svc(email_account_t *account);

/**
 * @brief Deletes an account from the account database by account SVC ID.
 * @param[in]	account_svc_id		The account ID
 * @return 0 on success, otherwise a negative error value
 */
int setting_delete_account_from_account_svc(int account_svc_id);

/**
 * @brief Get available color to account in RGB format
 * @param[in]	account_id		The account ID
 * @return The color in RGB format
 */
int setting_get_available_color(int account_id);

/**
 * @brief Check if email addres already exist in DB
 * @param[in]	email_addr		Email address name
 * @return 0 on success, otherwise a negative error value
 */
int setting_is_duplicate_account(const char *email_addr);

/**
 * @brief Cancel job by account ID
 * @param[in]	account_id		Account ID
 */
void setting_cancel_job_by_account_id(int account_id);

/**
 * @brief Prepare locale time pattern
 * @return 0 on success, otherwise a negative error value
 */
int setting_open_icu_pattern_generator(void);

/**
 * @brief Finalize I18N's locale pattern.
 * @return 0 on success, otherwise a negative error value
 */
int setting_close_icu_pattern_generator(void);

/**
 * @brief Copies a byte string encoded to datetime format.
 * @param[in]	skeleton	Base format for datetime format
 * @param[in]	time		Type represented time
 * @return The datetime formated string on success, otherwise NULL. It should be freed.
 */
char *setting_get_datetime_format_text(const char *skeleton, void *time);

/**
 * @brief Get provider name.
 * @param[in]	ugd		Email settings data
 * @return The provider formated string on success, otherwise NULL. It should be freed.
 */
char *setting_get_provider_name(EmailSettingUGD *ugd);

/**
 * @brief The function attempts to translate a text string into the user's native language, by looking up the translation in a message catalog.
 * @param[in]	t		Email string setting data
 * @return The translated text string on success, otherwise NULL. It should be freed.
 */
inline char *email_setting_gettext(email_setting_string_t t);

/**
 * @brief The function registers callbacks for: 1) "virtualkeypad,state,on" 2) "virtualkeypad,state,off"
 * @brief 3) "wm,rotation,changed".
 * @param[in]	ugd		Email settings data
 */
void setting_register_keypad_rot_cb(EmailSettingUGD *ugd);

/**
 * @brief The function unregisters callbacks for: 1) "virtualkeypad,state,on" 2) "virtualkeypad,state,off"
 * @brief 3) "wm,rotation,changed".
 * @param[in]	ugd		Email settings data
 */
void setting_deregister_keypad_rot_cb(EmailSettingUGD *ugd);

#endif				/* __EMAIL_SETTING_UTILS_H__ */

/* EOF */
