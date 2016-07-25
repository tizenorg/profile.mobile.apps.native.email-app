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

#ifndef _EMAIL_UTILS_H_
#define _EMAIL_UTILS_H_

#include <errno.h>
#include <system_settings.h>
#include <efl_extension.h>

#include "email-common-types.h"
#include "email-debug.h"
#include "email-editfield-utils.h"
#include "email-module-dev.h"
#include "email-params.h"

#define DEFAULT_CHARSET "UTF-8"
#define EMAIL_DATETIME_FORMAT_12 "yMdhm"
#define EMAIL_DATETIME_FORMAT_24 "yMdHm"
#define EMAIL_TIME_FORMAT_12 "hm"
#define EMAIL_TIME_FORMAT_24 "Hm"
#define EMAIL_YEAR_MONTH_DAY_FORMAT "yMd"
#define POPUP_EDITFIELD_LAYOUT_MINMAX_HEIGHT_INC 96

G_BEGIN_DECLS

/*
 * GList iterator.
 */
#define LIST_ITER_START(i, list) \
	for (i = 0; i < g_list_length(list); ++i)
#define LIST_ITER_START_REVERSE(i, list) \
	for (i = (g_list_length(list) - 1); i >= 0; --i)
#define LIST_ITER_GET(i, list) \
	g_list_nth(list, i)
#define LIST_ITER_GET_DATA(i, list) \
	g_list_nth_data(list, i)

#define G_LIST_FOREACH(list, ith, ith_data)	\
	for (ith = g_list_first(list), ith_data = (typeof(ith_data)) g_list_nth_data(ith, 0);\
		ith;\
		ith = g_list_next(ith), ith_data = (typeof(ith_data)) g_list_nth_data(ith, 0))

#define G_LIST_FREE(list) \
	do {\
		if (list) g_list_free(list);\
		list = NULL;\
	} while(0)

/*
 * String utility.
 */
#undef STR_VALID
#define STR_VALID(str) ((str) && *(str))
#undef STR_INVALID
#define STR_INVALID(str) (!(str) || !(*(str)))
#undef STR_LEN
#define STR_LEN(str) \
	({ const char *s = (const char *)str; (s) ? strlen(s) : 0; })
#undef STR_CMP
#define STR_CMP(s1, s2) \
	(((s1) == NULL || (s2) == NULL)? FALSE : (g_ascii_strcasecmp(s1, s2) == 0 ? TRUE : FALSE))
#undef STR_CMP0
#define STR_CMP0(s1, s2) \
	(g_strcmp0(s1, s2) == 0 ? TRUE : FALSE)
#undef STR_NCMP
#define STR_NCMP(s1, s2, n) \
	(g_ascii_strncasecmp(s1, s2, (gsize)n) == 0 ? TRUE : FALSE)
#undef STR_CPY
#define STR_CPY(dest, src) \
	g_stpcpy(dest, src)

#undef STR_NCPY
/* note that sizeof(dest) > n. n is not array size but max num of copied char */
/* dest requires NULL space only when src is not null terminated */
#define STR_NCPY(dest, src, n) \
	({\
		const char *_src = (const char *) src;\
		if ( n > 0 && dest && _src ) {\
			char *e = strncpy(dest, _src, n);\
			char err_buff[EMAIL_BUFF_SIZE_HUG] = { 0, };\
			if (!e) debug_critical("strncpy error: (%s)",\
					strerror_r(errno, err_buff, sizeof(err_buff)));\
			dest[n] = '\0';\
		}\
		dest;\
	})

#undef STRNDUP
/* return heap array of size n */
#define STRNDUP(src, n) \
	({\
		char *_ret = NULL;\
		char *_src = src;\
		if ( _src && n > 0 ) {\
			_ret = strndup(_src, n);\
			char err_buff[EMAIL_BUFF_SIZE_HUG] = { 0, };\
			if ( !_ret ) debug_critical("strndup error: (%s)",\
					strerror_r(errno, err_buff, sizeof(err_buff)));\
		}\
		_ret;\
	})

#undef STR_DUP
#define STR_DUP(src) g_strdup(src)

#undef MEM_ALLOC
#define MEM_ALLOC(ptr, size) \
	({\
		int _size = size;\
		typeof(ptr) mem = NULL;\
		if ( _size > 0 ) {\
			mem = (typeof(ptr)) calloc (_size, sizeof(typeof(*ptr)));\
			if (!mem) debug_critical("%s: memory allocation failed", #ptr);\
		}\
		else\
			debug_critical("PARAM error(%d): must be positive", _size);\
		mem;\
	})

#undef MEM_ALLOC_STR
#define MEM_ALLOC_STR(size) \
	({\
		int _size = size;\
		char *mem = NULL;\
		if ( size > 1 ) {\
			mem = (char *) calloc (1, size);\
			if (!mem) debug_critical("MALLOC error: char str (%d) memory allocation failed", _size);\
		}\
		else\
			debug_critical("MALLOC error: char str size(%d) must be greater than 1", _size);\
		mem;\
	})

#undef STR_LCPY
#define STR_LCPY(dest, src, n) \
	g_strlcpy(dest, src, n)
#undef STR_CAT
#define STR_CAT(dest, src) \
	strcat(dest, src)
#undef STR_NCAT
#define STR_NCAT(dest, src, n) \
	strncat(dest, src, n)
#undef STR_LCAT
#define STR_LCAT(dest, src, n) \
	g_strlcat(dest, src, n)

#undef FREE
#define FREE(ptr) \
	do {\
		free(ptr);\
		ptr = NULL;\
	} while (0)

#undef G_FREE
#define G_FREE(ptr) \
	do {\
		g_free(ptr);\
		ptr = NULL;\
	} while (0)

#undef DELETE_EVAS_OBJECT
#define DELETE_EVAS_OBJECT(x) \
	do { \
		evas_object_del(x); \
		x = NULL; \
	} while (0)

#undef DELETE_ELM_OBJECT_ITEM
#define DELETE_ELM_OBJECT_ITEM(x) \
	do { \
		if (x) elm_object_item_del(x); \
		x = NULL; \
	} while (0)

#undef DELETE_TIMER_OBJECT
#define DELETE_TIMER_OBJECT(x) \
	do { \
		if (x) ecore_timer_del(x); \
		x = NULL; \
	} while (0)

#undef DELETE_ANIMATOR_OBJECT
#define DELETE_ANIMATOR_OBJECT(x) \
	do { \
		if (x) ecore_animator_del(x); \
		x = NULL; \
	} while (0)

#undef DELETE_IDLER_OBJECT
#define DELETE_IDLER_OBJECT(x) \
	do { \
		if (x) ecore_idler_del(x); \
		x = NULL; \
	} while (0)

#undef DELETE_LIST_OBJECT
#define DELETE_LIST_OBJECT(x) \
	do { \
		if (x) eina_list_free(x); \
		x = NULL; \
	} while (0)

/*utf8 util*/
#define UTF8_VALIDATE(from, len) \
	({\
		char *end = NULL;\
		if (STR_VALID(from))\
			if (!g_utf8_validate(from, len, (const gchar **)&end) && end)\
				*end = '\0';\
		NULL;\
	})

#define EMAIL_GENLIST_ITC_FREE(x) \
	do { \
		if (x) { \
			x->item_style = NULL; \
			x->func.text_get = NULL; \
			x->func.content_get = NULL; \
			x->func.state_get = NULL; \
			x->func.del = NULL; \
			elm_genlist_item_class_free(x); \
		} \
	} while(0)

#define EMAIL_DEFINE_GET_FMT_STR(func_name, fmt, args...) \
	const char *func_name() \
	{ \
		static char result[EMAIL_SMALL_PATH_MAX] = {'\0'}; \
		if (!result[0]) { \
			snprintf(result, sizeof(result), fmt, ##args); \
		} \
		return result; \
	}

#define EMAIL_DEFINE_GET_PATH(func_name, root_dir_name, sub_path) \
	EMAIL_DEFINE_GET_FMT_STR(func_name, "%s" sub_path, email_get_ ## root_dir_name ## _dir())

#define EMAIL_DEFINE_GET_PHONE_PATH(func_name, sub_path) EMAIL_DEFINE_GET_PATH(func_name, phone_storage, sub_path)

#define EMAIL_DEFINE_GET_EDJ_PATH(func_name, sub_path) EMAIL_DEFINE_GET_PATH(func_name, res, "/edje" sub_path)
#define EMAIL_DEFINE_GET_IMG_PATH(func_name, sub_path) EMAIL_DEFINE_GET_PATH(func_name, res, "/images" sub_path)
#define EMAIL_DEFINE_GET_MISC_PATH(func_name, sub_path) EMAIL_DEFINE_GET_PATH(func_name, res, "/misc" sub_path)
#define EMAIL_DEFINE_GET_DATA_PATH(func_name, sub_path) EMAIL_DEFINE_GET_PATH(func_name, data, sub_path)
#define EMAIL_DEFINE_SHARED_RES_PATH(func_name, sub_path) EMAIL_DEFINE_GET_PATH(func_name, shared_res, sub_path)

#define IF_NULL_FREE_2GSTRA_AND_RETURN(i_ptr, str_arr1, str_arr2) \
	({\
		if (!i_ptr) {\
			g_strfreev(str_arr1);\
			g_strfreev(str_arr2);\
			debug_error("allocation memory failed");\
			return;\
		}\
	})

typedef enum {
	EMAIL_FILE_TYPE_IMAGE,
	EMAIL_FILE_TYPE_VIDEO,
	EMAIL_FILE_TYPE_MUSIC,
	EMAIL_FILE_TYPE_PDF,
	EMAIL_FILE_TYPE_DOC,
	EMAIL_FILE_TYPE_PPT,
	EMAIL_FILE_TYPE_EXCEL,
	EMAIL_FILE_TYPE_VOICE,
	EMAIL_FILE_TYPE_HTML,
	EMAIL_FILE_TYPE_FLASH,
	EMAIL_FILE_TYPE_TXT,
	EMAIL_FILE_TYPE_TPK,
	EMAIL_FILE_TYPE_RSS,
	EMAIL_FILE_TYPE_JAVA,
	EMAIL_FILE_TYPE_VCONTACT,
	EMAIL_FILE_TYPE_VCALENDAR,
	EMAIL_FILE_TYPE_SNB,
	EMAIL_FILE_TYPE_SCC,
	EMAIL_FILE_TYPE_HWP,
	EMAIL_FILE_TYPE_ZIP,
	EMAIL_FILE_TYPE_EML,
	EMAIL_FILE_TYPE_SVG,
	EMAIL_FILE_TYPE_ETC,
	EMAIL_FILE_TYPE_UNSUPPORTED,
	EMAIL_FILE_TYPE_MAX
} email_file_type_e;

typedef struct {
	const char *mime;
	email_file_type_e type;
} email_file_type_mime_t;

typedef enum {
	EMAIL_NETWORK_STATUS_AVAILABLE = 0,
	EMAIL_NETWORK_STATUS_NO_SIM_OR_OUT_OF_SERVICE = -1,
	EMAIL_NETWORK_STATUS_FLIGHT_MODE = -2,
	EMAIL_NETWORK_STATUS_MOBILE_DATA_DISABLED = -3,
	EMAIL_NETWORK_STATUS_ROAMING_OFF = -4,
	EMAIL_NETWORK_STATUS_MOBILE_DATA_LIMIT_EXCEED = -5
} email_network_status_e;

/*
 * Exported functions.
 */

typedef void (*viprule_changed_cb) (const char *key, void *user_data);

EMAIL_API const char *email_get_phone_storage_dir();
EMAIL_API const char *email_get_mmc_storage_dir();

EMAIL_API const char *email_get_phone_downloads_dir();

EMAIL_API const char *email_get_res_dir();
EMAIL_API const char *email_get_data_dir();
EMAIL_API const char *email_get_shared_res_dir();

EMAIL_API const char *email_get_img_dir();
EMAIL_API const char *email_get_misc_dir();
EMAIL_API const char *email_get_locale_dir();

EMAIL_API const char *email_get_common_theme_path();

EMAIL_API const char *email_get_template_html_path();
EMAIL_API const char *email_get_template_html_text_path();

EMAIL_API const char *email_get_res_url();
EMAIL_API const char *email_get_phone_storage_url();
EMAIL_API const char *email_get_mmc_storage_url();

EMAIL_API email_network_status_e email_get_network_state(void);

EMAIL_API email_file_type_e email_get_file_type_from_mime_type(const char *mime_type);
EMAIL_API const char *email_get_icon_path_from_file_type(email_file_type_e ftype);

EMAIL_API gboolean email_check_dir_exist(const gchar *path);
EMAIL_API gboolean email_check_file_exist(const gchar *path);

EMAIL_API gchar *email_parse_get_title_from_filename(const gchar *title);	/* g_free(). */
EMAIL_API gchar *email_parse_get_filepath_from_path(const gchar *path);	/* g_free(). */
EMAIL_API gchar *email_parse_get_ext_from_filename(const gchar *filename);	/* g_free(). */
EMAIL_API gchar *email_parse_get_filename_from_path(const gchar *path);	/* g_free(). */
EMAIL_API void email_parse_get_filename_n_ext_from_path(const gchar *path, gchar **ret_file_name, gchar **ret_ext);

EMAIL_API gboolean email_save_file(const gchar *path, const gchar *buf, gsize len);
EMAIL_API gchar *email_get_buff_from_file(const gchar *path, guint max_kbyte);	/* g_free(). */
EMAIL_API char *email_body_encoding_convert(char *text_content, char *from_charset, char *to_charset);
EMAIL_API gchar *email_get_file_size_string(guint64 size);	/* g_free(). */
EMAIL_API guint64 email_get_file_size(const gchar *path);

EMAIL_API gboolean email_get_address_validation(const char *address);
EMAIL_API gboolean email_get_recipient_info(const gchar *_recipient, gchar **_nick, gchar **_addr);

EMAIL_API gchar *email_cut_text_by_byte_len(const gchar *text, gint len);	/* g_free(). */

EMAIL_API char *email_get_str_datetime(const time_t time_val);
EMAIL_API char *email_get_date_text(const char *locale, char *skeleton, void *time);
EMAIL_API char *email_get_date_text_with_formatter(void *formatter, void *time);

EMAIL_API int email_create_folder(const char *path);
EMAIL_API gboolean email_copy_file_feedback(const char *src_full_path, const char *dest_full_path, gboolean(*copy_file_cb) (void *data, float percentage), void *cb_data);

EMAIL_API int email_open_pattern_generator(void);
EMAIL_API int email_close_pattern_generator(void);
EMAIL_API int email_generate_pattern_for_local(const char *local, const char *pattern_format, char *gen_pattern, int gen_pattern_size);

EMAIL_API void email_mutex_lock(void);
EMAIL_API void email_mutex_unlock(void);

EMAIL_API void email_feedback_init(void);
EMAIL_API void email_feedback_deinit(void);
EMAIL_API void email_feedback_play_tap_sound(void);

EMAIL_API int email_register_language_changed_callback(system_settings_changed_cb func, void *data);
EMAIL_API int email_unregister_language_changed_callback(system_settings_changed_cb func);

EMAIL_API int email_register_timezone_changed_callback(system_settings_changed_cb func, void *data);
EMAIL_API int email_unregister_timezone_changed_callback(system_settings_changed_cb func);

EMAIL_API int email_register_accessibility_font_size_changed_callback(system_settings_changed_cb func, void *data);
EMAIL_API int email_unregister_accessibility_font_size_changed_callback(system_settings_changed_cb func);

EMAIL_API char *email_util_strrtrim(char *s);
EMAIL_API char *email_util_strltrim(char *s);
EMAIL_API char *email_util_strtrim(char *s);

EMAIL_API int email_util_escape_string(const char *const src, char *const dst,
		const int dst_buff_size, const char *const escape_chars);
EMAIL_API char *email_util_escape_string_alloc(const char *const src,
		const char *const escape_chars);

EMAIL_API char *email_get_mime_type_from_file(const char *path);
EMAIL_API bool email_is_smime_cert_attachment(const char *mime_type);

EMAIL_API int email_get_deleted_flag_mail_count(int account_id);
EMAIL_API int email_get_favourite_mail_count(bool unread_only);
EMAIL_API bool email_get_favourite_mail_count_ex(int *total_mail_count, int *unread_mail_count);
EMAIL_API int email_get_priority_sender_mail_count(bool unread_only);
EMAIL_API int email_get_priority_sender_mail_count_ex(int *total_mail_count, int *unread_mail_count);
EMAIL_API int email_get_scheduled_outbox_mail_count_by_account_id(int account_id, bool unread_only);
EMAIL_API int email_get_scheduled_outbox_mail_count_by_account_id_ex(int account_id, int *total_mail_count, int *unread_mail_count);
EMAIL_API bool email_get_combined_mail_count_by_mailbox_type(int mailbox_type, int *total_mail_count, int *unread_mail_count);

EMAIL_API void email_set_is_inbox_active(bool is_active);

EMAIL_API char *email_get_datetime_format(void);

EMAIL_API char *email_get_system_string(const char *string_id);
EMAIL_API char *email_get_email_string(const char *string_id);

EMAIL_API Elm_Genlist_Item_Class* email_util_get_genlist_item_class(const char *style,
		Elm_Gen_Item_Text_Get_Cb text_get,
		Elm_Gen_Item_Content_Get_Cb content_get,
		Elm_Gen_Item_State_Get_Cb state_get,
		Elm_Gen_Item_Del_Cb del);
EMAIL_API Evas_Object *email_util_create_password_changed_popup(
		Evas_Object *parent, int account_id, Evas_Smart_Cb done_cb,
		Evas_Smart_Cb cancel_cb, void *data);
EMAIL_API char *email_util_get_login_failure_string(int account_id);
EMAIL_API gboolean email_get_is_pattern_matched(const char *pattern, const char *str);
EMAIL_API Eina_Bool email_file_cp(const char* source, const char* destination);
EMAIL_API char *email_file_dir_get(const char *path);
EMAIL_API const char *email_file_file_get(const char *file_path);
EMAIL_API const char *email_file_name_ext_get(const char *file_name);
EMAIL_API const char *email_file_path_ext_get(const char *file_path);
EMAIL_API Eina_Bool email_file_exists(const char *file);
EMAIL_API Eina_Bool email_file_is_dir(const char *file);
EMAIL_API Eina_Bool email_file_mv(const char *src, const char *dst);
EMAIL_API Eina_Bool email_file_remove(const char *file);
EMAIL_API Eina_Bool email_file_unlink(const char *file);
EMAIL_API char *email_file_strip_ext(const char *file_path, const char **out_ext);
EMAIL_API Eina_Bool email_file_recursive_rm (const char *dir);
EMAIL_API bool email_file_optimize_path(char *in_out_path);

EMAIL_API int email_update_vip_rule_value();
EMAIL_API int email_get_vip_rule_value(double *value);
EMAIL_API int email_set_vip_rule_change_callback(viprule_changed_cb callback, void *user_data);
EMAIL_API int email_unset_vip_rule_change_callback();
EMAIL_API int email_set_richtext_toolbar_status(Eina_Bool enable);
EMAIL_API int email_get_richtext_toolbar_status(Eina_Bool *enabled);
EMAIL_API Eina_Bool email_util_get_rgb_string(char *buf, int size, unsigned char r, unsigned char g, unsigned char b);

EMAIL_API void email_set_need_restart_flag(bool value);
EMAIL_API bool email_get_need_restart_flag();

EMAIL_API int email_preview_attachment_file(email_module_t *module, const char *path, email_launched_app_listener_t *listener);
EMAIL_API email_ext_save_err_type_e email_prepare_temp_file_path(const int index,const char *tmp_root_dir, const char *src_file_path, char **dst_file_path);
EMAIL_API void email_set_ellipsised_text(Evas_Object *text_obj, const char *text, int max_width);
/*
 * Memory trace utility.
 */
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

typedef struct {
	void *view_data;
	void *data;
}email_common_timer_data_t;

#define COMMON_GET_TIMER_DATA(tdata, type, name, data) \
	email_common_timer_data_t *tdata = (email_common_timer_data_t *)data; \
	type *name = tdata->view_data

G_END_DECLS
#endif	/* _EMAIL_UTILS_H_ */

/* EOF */
