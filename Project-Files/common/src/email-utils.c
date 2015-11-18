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

#define CUSTOM_I18N_UDATE_IGNORE -2 /* Used temporarily since there is no substitute of UDATE_IGNORE in base-utils */

#include <sys/time.h>
#include <utils_i18n.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <stdlib.h>
#include <regex.h>
#include <errno.h>
#include <pthread.h>
#include <Eina.h>
#include <Elementary.h>
#include <email-api.h>
#include <net_connection.h>
#include <mime_type.h>
#include <sys/sendfile.h>
#include <app.h>
#include <storage.h>
#include <app_preference.h>
#include <vconf.h>
#include <feedback.h>
#include <tzplatform_config.h>

#include "email-utils.h"
#include "email-locale.h"
#include "email-engine.h"

#define EMAIL_RES_PATH 			PKGNAME"/res"
#define EMAIL_DATA_PATH 		PKGNAME"/data"
#define EMAIL_SHARED_RES_PATH 	PKGNAME"/shared/res"

// TODO: temporary for application testing
//#define EMAIL_SHARED_DATA_PATH 	PKGNAME"/shared/data"
#define EMAIL_SHARED_DATA_PATH 	PKGNAME"/data"

#define EMAIL_FAIL_SAFE_RES_PATH 			tzplatform_mkpath(TZ_SYS_RO_APP, EMAIL_RES_PATH)
#define EMAIL_FAIL_SAFE_DATA_PATH 			tzplatform_mkpath(TZ_USER_APP, EMAIL_DATA_PATH)
#define EMAIL_FAIL_SAFE_SHARED_RES_PATH 	tzplatform_mkpath(TZ_SYS_RO_APP, EMAIL_SHARED_RES_PATH)
#define EMAIL_FAIL_SAFE_SHARED_DATA_PATH 	tzplatform_mkpath(TZ_USER_APP, EMAIL_SHARED_DATA_PATH)

// TODO: temporary for application testing (for public temp folder)
#define EMAIL_FAIL_SAFE_SHARE_USER_PATH 	tzplatform_getenv(TZ_USER_SHARE)

#define EMAIL_FAIL_SAFE_PHONE_STORAGE_PATH 	tzplatform_getenv(TZ_USER_CONTENT)
#define EMAIL_FAIL_SAFE_MMC_STORAGE_PATH	tzplatform_mkpath(TZ_SYS_STORAGE, "sdcard")
#define EMAIL_FAIL_SAFE_DOWNLOADS_DIR 		"Downloads"
#define EMAIL_FAIL_SAFE_SOUNDS_DIR 			"Sounds"

#define EMAIL_ETC_LOCALTIME_PATH 			tzplatform_mkpath(TZ_SYS_ETC, "localtime")

#define EMAIL_FILE_URL_FMT "file://%s"

#define VCONFKEY_EMAIL_IS_INBOX_ACTIVE "db/private/org.tizen.email/is_inbox_active"

#define EMAIL_MIME_ICON_SUB_DIR "file_type_icon"

#define EMAIL_ADDRESS_REGEX "^[[:alpha:]0-9._%+-]+@[[:alpha:]0-9.-]+\\.[[:alpha:]]{2,6}$"

/* for Gallery application to prohibit delete/rename image */
#define EMAIL_PREVIEW_VIEW_MODE_PARAM_NAME "View Mode"
#define EMAIL_PREVIEW_VIEW_MODE_PARAM_STR_VALUE "EMAIL"

/* for Calendar application application to not show details menu */
#define EMAIL_PREVIEW_MENU_STATE_PARAM_NAME "Menu State"
#define EMAIL_PREVIEW_MENU_STATE_PARAM_STR_VALUE "1"

#define EMAIL_GMT_NAME_SIZE 3
#define EMAIL_STR_SIZE 10
#define EMAIL_DELIMETER_SIZE 2

#define EMAIL_BUFF_SIZE_TIN 16
#define EMAIL_BUFF_SIZE_SML 32
#define EMAIL_BUFF_SIZE_MID 64
#define EMAIL_BUFF_SIZE_BIG 128
#define EMAIL_BUFF_SIZE_HUG 256
#define EMAIL_BUFF_SIZE_4K 4096
static bool b_hide_status[EMAIL_STR_SIZE] = {'\0'};

EMAIL_API const char *locale = NULL;
static bool is_24_hr_format = false;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static i18n_udatepg_h pattern_generator = NULL;
static i18n_udate_format_h formatter_hmm = NULL;
static i18n_udate_format_h formatter_Hmm = NULL;
static i18n_udate_format_h formatter_MMMyyyy = NULL;
static i18n_udate_format_h formatter_MMMd = NULL;
static i18n_udate_format_h formatter_EEEMMMd = NULL;

static Eina_List *_system_settings_key_list = NULL;

static Eina_Bool _email_is_inbox_active = EINA_FALSE;

typedef struct {
	system_settings_changed_cb callback;
	void *user_data;
} EmailSystemSettingsCallbackEntry;

typedef struct {
	system_settings_key_e key;
	Eina_List *callbacks_list;
} EmailSystemSettingsKeyEntry;

static EmailFileTypeMIME email_mime_type[] = {
	/* EMAIL_FILE_TYPE_IMAGE */
	{"image/png", EMAIL_FILE_TYPE_IMAGE},
	{"image/jpeg", EMAIL_FILE_TYPE_IMAGE},
	{"image/gif", EMAIL_FILE_TYPE_IMAGE},
	{"image/bmp", EMAIL_FILE_TYPE_IMAGE},
	{"image/vnd.wap.wbmp", EMAIL_FILE_TYPE_IMAGE},
	{"image/x-xpixmap", EMAIL_FILE_TYPE_UNSUPPORTED},
	{"image/x-tga", EMAIL_FILE_TYPE_UNSUPPORTED},

	/* EMAIL_FILE_TYPE_VIDEO */
	{"video/x-msvideo", EMAIL_FILE_TYPE_VIDEO},
	{"video/mp4", EMAIL_FILE_TYPE_VIDEO},
	{"video/3gpp", EMAIL_FILE_TYPE_VIDEO},
	{"video/x-ms-asf", EMAIL_FILE_TYPE_VIDEO},
	{"video/x-ms-wmv", EMAIL_FILE_TYPE_VIDEO},
	{"video/x-matroska", EMAIL_FILE_TYPE_VIDEO},

	/* EMAIL_FILE_TYPE_MUSIC */
	{"audio/mpeg", EMAIL_FILE_TYPE_MUSIC},
	{"audio/x-wav", EMAIL_FILE_TYPE_MUSIC},
	{"application/x-smaf", EMAIL_FILE_TYPE_MUSIC},
	{"audio/mxmf", EMAIL_FILE_TYPE_MUSIC},
	{"audio/midi", EMAIL_FILE_TYPE_MUSIC},
	{"audio/x-xmf", EMAIL_FILE_TYPE_MUSIC},
	{"audio/x-ms-wma", EMAIL_FILE_TYPE_MUSIC},
	{"audio/aac", EMAIL_FILE_TYPE_MUSIC},
	{"audio/ac3", EMAIL_FILE_TYPE_MUSIC},
	{"audio/ogg", EMAIL_FILE_TYPE_MUSIC},
	{"audio/x-vorbis+ogg", EMAIL_FILE_TYPE_MUSIC},
	{"audio/vorbis", EMAIL_FILE_TYPE_MUSIC},
	{"audio/imelody", EMAIL_FILE_TYPE_MUSIC},
	{"audio/iMelody", EMAIL_FILE_TYPE_MUSIC},
	{"audio/x-rmf", EMAIL_FILE_TYPE_MUSIC},
	{"application/vnd.smaf", EMAIL_FILE_TYPE_MUSIC},
	{"audio/mobile-xmf", EMAIL_FILE_TYPE_MUSIC},
	{"audio/mid", EMAIL_FILE_TYPE_MUSIC},
	{"audio/vnd.ms-playready.media.pya", EMAIL_FILE_TYPE_MUSIC},
	{"audio/imy", EMAIL_FILE_TYPE_MUSIC},
	{"audio/m4a", EMAIL_FILE_TYPE_VOICE},
	{"audio/melody", EMAIL_FILE_TYPE_MUSIC},
	{"audio/mmf", EMAIL_FILE_TYPE_MUSIC},
	{"audio/mp3", EMAIL_FILE_TYPE_MUSIC},
	{"audio/mp4", EMAIL_FILE_TYPE_MUSIC},
	{"audio/MP4A-LATM", EMAIL_FILE_TYPE_MUSIC},
	{"audio/mpeg3", EMAIL_FILE_TYPE_MUSIC},
	{"audio/mpeg4", EMAIL_FILE_TYPE_MUSIC},
	{"audio/mpg", EMAIL_FILE_TYPE_MUSIC},
	{"audio/mpg3", EMAIL_FILE_TYPE_MUSIC},
	{"audio/smaf", EMAIL_FILE_TYPE_MUSIC},
	{"audio/sp-midi", EMAIL_FILE_TYPE_MUSIC},
	{"audio/wav", EMAIL_FILE_TYPE_MUSIC},
	{"audio/wave", EMAIL_FILE_TYPE_MUSIC},
	{"audio/wma", EMAIL_FILE_TYPE_MUSIC},
	{"audio/xmf", EMAIL_FILE_TYPE_MUSIC},
	{"audio/x-mid", EMAIL_FILE_TYPE_MUSIC},
	{"audio/x-midi", EMAIL_FILE_TYPE_MUSIC},
	{"audio/x-mp3", EMAIL_FILE_TYPE_MUSIC},
	{"audio/-mpeg", EMAIL_FILE_TYPE_MUSIC},
	{"audio/x-mpeg", EMAIL_FILE_TYPE_MUSIC},
	{"audio/x-mpegaudio", EMAIL_FILE_TYPE_MUSIC},
	{"audio/x-mpg", EMAIL_FILE_TYPE_MUSIC},
	{"audio/x-ms-asf", EMAIL_FILE_TYPE_MUSIC},
	{"audio/x-wave", EMAIL_FILE_TYPE_MUSIC},
	{"audio/x-flac", EMAIL_FILE_TYPE_MUSIC},
	{"text/x-iMelody", EMAIL_FILE_TYPE_MUSIC},

	/* EMAIL_FILE_TYPE_PDF */
	{"application/pdf", EMAIL_FILE_TYPE_PDF},

	/* EMAIL_FILE_TYPE_DOC */
	{"application/msword", EMAIL_FILE_TYPE_DOC},
	{"application/rtf", EMAIL_FILE_TYPE_DOC},
	{"application/vnd.openxmlformats-officedocument.wordprocessingml.document", EMAIL_FILE_TYPE_DOC},

	/* EMAIL_FILE_TYPE_PPT */
	{"application/vnd.ms-powerpoint", EMAIL_FILE_TYPE_PPT},
	{"application/vnd.openxmlformats-officedocument.presentationml.presentation", EMAIL_FILE_TYPE_PPT},

	/* EMAIL_FILE_TYPE_EXCEL */
	{"application/vnd.ms-excel", EMAIL_FILE_TYPE_EXCEL},
	{"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet", EMAIL_FILE_TYPE_EXCEL},

	/* EMAIL_FILE_TYPE_VOICE */
	{"audio/AMR", EMAIL_FILE_TYPE_VOICE},
	{"audio/AMR-WB", EMAIL_FILE_TYPE_VOICE},
	{"audio/amr", EMAIL_FILE_TYPE_VOICE},
	{"audio/amr-wb", EMAIL_FILE_TYPE_VOICE},
	{"audio/x-amr", EMAIL_FILE_TYPE_VOICE},

	/* EMAIL_FILE_TYPE_HTML */
	{"text/html", EMAIL_FILE_TYPE_HTML},
	{"text/xml", EMAIL_FILE_TYPE_HTML},
	{"application/xml", EMAIL_FILE_TYPE_HTML},

	/* EMAIL_FILE_TYPE_FLASH */
	{"application/x-shockwave-flash", EMAIL_FILE_TYPE_FLASH},
	{"video/x-flv", EMAIL_FILE_TYPE_FLASH},

	/* EMAIL_FILE_TYPE_TXT */
	{"text/plain", EMAIL_FILE_TYPE_TXT},

	/* EMAIL_FILE_TYPE_TPK */
	{"application/vnd.tizen.package", EMAIL_FILE_TYPE_TPK},

	/* EMAIL_FILE_TYPE_RSS */
	{"text/x-opml+xml", EMAIL_FILE_TYPE_RSS},

	/* EMAIL_FILE_TYPE_JAVA */
	{"text/vnd.sun.j2me.app-descriptor", EMAIL_FILE_TYPE_JAVA},
	{"application/x-java-archive", EMAIL_FILE_TYPE_JAVA},

	/* EMAIL_FILE_TYPE_VCONTACT */
	{"text/directory", EMAIL_FILE_TYPE_VCONTACT},
	{"text/x-vcard", EMAIL_FILE_TYPE_VCONTACT},
	{"text/vcard", EMAIL_FILE_TYPE_VCONTACT},

	/* EMAIL_FILE_TYPE_VCALENDAR */
	{"text/calendar", EMAIL_FILE_TYPE_VCALENDAR},

	/* EMAIL_FILE_TYPE_SNB for S memo */
	{"application/snb", EMAIL_FILE_TYPE_SNB},

	/* EMAIL_FILE_TYPE_SCC for Story album */
	{"application/scc", EMAIL_FILE_TYPE_SCC},

	/* EMAIL_FILE_TYPE_HWP for Hangul */
	{"application/x-hwp", EMAIL_FILE_TYPE_HWP},

	/* EMAIL_FILE_TYPE_ZIP */
	{"application/zip", EMAIL_FILE_TYPE_ZIP},

	/* EMAIL_FILE_TYPE_EML */
	{"message/rfc822", EMAIL_FILE_TYPE_EML},

	/* EMAIL_FILE_TYPE_SVG */
	{"image/svg+xml", EMAIL_FILE_TYPE_SVG},

	/* EMAIL_FILE_TYPE_ETC */
	{NULL, EMAIL_FILE_TYPE_ETC},
};

static const char *email_mime_icon[EMAIL_FILE_TYPE_MAX] = {
	[EMAIL_FILE_TYPE_IMAGE]		= "email_icon_img.png",
	[EMAIL_FILE_TYPE_VIDEO]		= "email_icon_movie.png",
	[EMAIL_FILE_TYPE_MUSIC]		= "email_icon_music.png",
	[EMAIL_FILE_TYPE_PDF]		= "email_icon_pdf.png",
	[EMAIL_FILE_TYPE_DOC]		= "email_icon_word.png",
	[EMAIL_FILE_TYPE_PPT]		= "email_icon_ppt.png",
	[EMAIL_FILE_TYPE_EXCEL]		= "email_icon_xls.png",
	[EMAIL_FILE_TYPE_VOICE]		= "email_icon_amr.png",
	[EMAIL_FILE_TYPE_HTML]		= "email_icon_html.png",
	[EMAIL_FILE_TYPE_FLASH]		= "email_icon_swf.png",
	[EMAIL_FILE_TYPE_TXT]		= "email_icon_txt.png",
	[EMAIL_FILE_TYPE_TPK]		= "email_icon_tpk.png",
	[EMAIL_FILE_TYPE_RSS]		= "email_icon_rss.png",
	[EMAIL_FILE_TYPE_JAVA]		= "email_icon_etc.png",
	[EMAIL_FILE_TYPE_VCONTACT]	= "email_icon_vcf.png",
	[EMAIL_FILE_TYPE_VCALENDAR]	= "email_icon_date.png",
	[EMAIL_FILE_TYPE_SNB]		= "email_icon_snb.png",
	[EMAIL_FILE_TYPE_SCC]		= "email_icon_story.png",
	[EMAIL_FILE_TYPE_HWP]		= "email_icon_hwp.png",
	[EMAIL_FILE_TYPE_ZIP]		= "email_icon_zip.png",
	[EMAIL_FILE_TYPE_EML]		= "email_icon_eml.png",
	[EMAIL_FILE_TYPE_SVG]		= "email_icon_svg.png",
	[EMAIL_FILE_TYPE_ETC]		= "email_icon_etc.png",
	[EMAIL_FILE_TYPE_UNSUPPORTED]		= "email_icon_unknown.png",
};

enum {
	EMAIL_ROTATE_0 = 0,
	EMAIL_ROTATE_90 = 90,
	EMAIL_ROTATE_180 = 180,
	EMAIL_ROTATE_270 = 270
};

enum {
	EMAIL_GROUP_UNKNOWN = -1,
	EMAIL_GROUP_TODAY,
	EMAIL_GROUP_YESTERDAY,
	EMAIL_GROUP_SUN,
	EMAIL_GROUP_MON,
	EMAIL_GROUP_TUE,
	EMAIL_GROUP_WED,
	EMAIL_GROUP_THU,
	EMAIL_GROUP_LASTWEEK,
	EMAIL_GROUP_TWOWEEKS_AGO,
	EMAIL_GROUP_THREEWEEKS_AGO,
	EMAIL_GROUP_FOURWEEKS_AGO,
	EMAIL_GROUP_LASTMONTH,
	EMAIL_GROUP_OLDER,
	EMAIL_GROUP_UNREAD,
	EMAIL_GROUP_READ,
	EMAIL_GROUP_FAVORITES,
	EMAIL_GROUP_ATTACHMENTS,
	EMAIL_GROUP_OTHER,
	EMAIL_GROUP_PRIORITY_HIGH,
	EMAIL_GROUP_PRIORITY_NORMAL,
	EMAIL_GROUP_PRIORITY_LOW,
	EMAIL_GROUP_MAX,
};


static gint _g_time_rev_set = 0;
static gint _g_time_rev = 0;
static bool _g_viewer_launched_on_mailbox = false;

static struct info {
	char res_dir[EMAIL_SMALL_PATH_MAX];
	char data_dir[EMAIL_SMALL_PATH_MAX];
	char shared_res_dir[EMAIL_SMALL_PATH_MAX];
	char shared_data_dir[EMAIL_SMALL_PATH_MAX];
	char shared_user_dir[EMAIL_SMALL_PATH_MAX];

	char phone_storage_dir[EMAIL_SMALL_PATH_MAX];
	char mmc_storage_dir[EMAIL_SMALL_PATH_MAX];

	int phone_storage_id;

	bool storages_info_valid;
	bool phone_storage_ok;

	bool need_restart_flag;

} s_info = {
	.res_dir = {'\0'},
	.data_dir = {'\0'},
	.shared_res_dir = {'\0'},
	.shared_data_dir = {'\0'},
	.shared_user_dir = {'\0'},

	.phone_storage_dir = {'\0'},
	.mmc_storage_dir = {'\0'},

	.phone_storage_id = -1,

	.storages_info_valid = false,
	.phone_storage_ok = false,

	.need_restart_flag = false
};

static void _validate_storage_info();
static bool _storage_device_supported_cb(int storage_id, storage_type_e type,
		storage_state_e state, const char *path, void *user_data);
static void _copy_path_skip_last_bs(char *dst, int dst_size, const char *src);

static void _update_lang_environment(const char *lang);
static void _generate_best_pattern(const char *locale, i18n_uchar *customSkeleton, char *formattedString, void *time);
static int _open_pattern_n_formatter(const char *locale, char *skeleton, i18n_udate_format_h *formatter);
static int _close_pattern_n_formatter(i18n_udate_format_h formatter);

#ifdef _EMAIL_GBS_BUILD_
#define EMAIL_DEFINE_GET_APP_ROOT_PATH(func_name, root_func_name, var_name, fail_safe_path) \
	const char *func_name() \
	{ \
		if (!var_name[0]) { \
			snprintf(var_name, sizeof(var_name), "%s", fail_safe_path); \
			debug_log("Directory is: %s", var_name); \
		} \
		return var_name; \
	}
#else
#define EMAIL_DEFINE_GET_APP_ROOT_PATH(func_name, root_func_name, var_name, fail_safe_path) \
	const char *func_name() \
	{ \
		if (!var_name[0]) { \
			bool free_str = true; \
			char *str = root_func_name(); \
			if (!str || !str[0]) { \
				debug_error(#root_func_name "(): failed! Using fail safe path..."); \
				str = fail_safe_path; \
				free_str = false; \
			} \
			\
			_copy_path_skip_last_bs(var_name, sizeof(var_name), str); \
			\
			if (free_str) { \
				free(str); \
			} \
			\
			debug_log("Directory is: %s", var_name); \
		} \
		return var_name; \
	}
#endif

#define EMAIL_DEFINE_GET_MEDIA_PATH(func_name, dir_type, fail_safe_path) \
	const char *func_name() \
	{ \
		static char result[EMAIL_SMALL_PATH_MAX] = {'\0'}; \
		if (!result[0]) { \
			char *path = NULL; \
			\
			_validate_storage_info(); \
			\
			if (s_info.phone_storage_ok) { \
				int r = storage_get_directory(s_info.phone_storage_id, dir_type, &path); \
				if (r != STORAGE_ERROR_NONE) { \
					debug_error("storage_get_directory(%d): failed (%d)", (int)dir_type, r); \
					path = NULL; \
				} \
			} \
			\
			if (!path) { \
				debug_log("Using fail safe path..."); \
				snprintf(result, sizeof(result), "%s/" fail_safe_path, email_get_phone_storage_dir()); \
			} else { \
				_copy_path_skip_last_bs(result, sizeof(result), path); \
				free(path); \
			} \
		} \
		return result; \
	}

/*
 * Function implementation
 */

void _copy_path_skip_last_bs(char *dst, int dst_size, const char *src)
{
	int n = snprintf(dst, dst_size, "%s", src);
	if (n >= dst_size) {
		debug_warning("Path was cut!");
	} else if (dst[n - 1] == '/') {
		dst[n - 1] = '\0';
	}
}

void _validate_storage_info()
{
	if (!s_info.storages_info_valid) {

		int r = storage_foreach_device_supported(_storage_device_supported_cb, NULL);
		if (r != STORAGE_ERROR_NONE) {
			debug_error("storage_foreach_device_supported(): failed (%d)", r);
		}

		if (!s_info.phone_storage_dir[0]) {
			debug_log("Using fail safe phone storage path...");
			snprintf(s_info.phone_storage_dir, sizeof(s_info.phone_storage_dir),
					"%s", EMAIL_FAIL_SAFE_PHONE_STORAGE_PATH);
		}

		s_info.storages_info_valid = true;
	}
}

static bool _storage_device_supported_cb(int storage_id, storage_type_e type,
		storage_state_e state, const char *path, void *user_data)
{
	if (path) {
		switch (type) {
		case STORAGE_TYPE_INTERNAL:
			_copy_path_skip_last_bs(s_info.phone_storage_dir,
					sizeof(s_info.phone_storage_dir), path);
			s_info.phone_storage_id = storage_id;
			s_info.phone_storage_ok = true;
			debug_log("Phone storage directory is: %s", s_info.phone_storage_dir);
			break;

		case STORAGE_TYPE_EXTERNAL:
			_copy_path_skip_last_bs(s_info.mmc_storage_dir,
					sizeof(s_info.mmc_storage_dir), path);
			debug_log("MMC storage directory is: %s", s_info.mmc_storage_dir);
			break;

		default:
			break;
		}
	} else {
		debug_warning("path is NULL for storage %d", (int)type);
	}
	return true;
}

const char *email_get_phone_storage_dir()
{
	_validate_storage_info();
	return s_info.phone_storage_dir;
}

const char *email_get_mmc_storage_dir()
{
	_validate_storage_info();
	return s_info.mmc_storage_dir[0] ? s_info.mmc_storage_dir : NULL;
}

EMAIL_DEFINE_GET_MEDIA_PATH(email_get_phone_downloads_dir,
		STORAGE_DIRECTORY_DOWNLOADS, EMAIL_FAIL_SAFE_DOWNLOADS_DIR)
EMAIL_DEFINE_GET_MEDIA_PATH(email_get_phone_sounds_dir,
		STORAGE_DIRECTORY_SOUNDS, EMAIL_FAIL_SAFE_SOUNDS_DIR)
EMAIL_DEFINE_GET_PHONE_PATH(email_get_phone_tmp_dir, "/tmp")

EMAIL_DEFINE_GET_APP_ROOT_PATH(email_get_res_dir, app_get_resource_path,
		s_info.res_dir, EMAIL_FAIL_SAFE_RES_PATH)

EMAIL_DEFINE_GET_APP_ROOT_PATH(email_get_data_dir, app_get_data_path,
		s_info.data_dir, EMAIL_FAIL_SAFE_DATA_PATH)

EMAIL_DEFINE_GET_APP_ROOT_PATH(email_get_shared_res_dir, app_get_shared_resource_path,
		s_info.shared_res_dir, EMAIL_FAIL_SAFE_SHARED_RES_PATH)

EMAIL_DEFINE_GET_APP_ROOT_PATH(email_get_shared_data_dir, app_get_shared_data_path,
		s_info.shared_data_dir, EMAIL_FAIL_SAFE_SHARED_DATA_PATH)

// TODO: temporary for testing
EMAIL_DEFINE_GET_APP_ROOT_PATH(email_get_shared_user_dir, NULL,
		s_info.shared_user_dir, EMAIL_FAIL_SAFE_SHARE_USER_PATH)

EMAIL_DEFINE_GET_IMG_PATH(email_get_img_dir, "");
EMAIL_DEFINE_GET_MISC_PATH(email_get_misc_dir, "")
EMAIL_DEFINE_GET_PATH(email_get_locale_dir, res, "/locale");

EMAIL_DEFINE_GET_EDJ_PATH(email_get_common_theme_path, "/email-common-theme.edj")

EMAIL_DEFINE_GET_MISC_PATH(email_get_default_html_path, "/_email_default.html")
EMAIL_DEFINE_GET_MISC_PATH(email_get_template_html_path, "/_email_template.html")
EMAIL_DEFINE_GET_MISC_PATH(email_get_template_html_text_path, "/_email_template_text.html")

EMAIL_DEFINE_GET_FMT_STR(email_get_res_url, EMAIL_FILE_URL_FMT, email_get_res_dir())
EMAIL_DEFINE_GET_FMT_STR(email_get_phone_storage_url, EMAIL_FILE_URL_FMT, email_get_phone_storage_dir())

const char *email_get_mmc_storage_url()
{
	static char result[EMAIL_SMALL_PATH_MAX] = {'\0'};
	if (!result[0]) {
		const char *path = email_get_mmc_storage_dir();
		if (!path) {
			return NULL;
		}
		snprintf(result, sizeof(result), EMAIL_FILE_URL_FMT, path);
	}
	return result;
}

EMAIL_API void email_set_group_order(int i, bool hide)
{
	b_hide_status[i] = hide;
}

EMAIL_API bool email_get_group_order(int i)
{
	return b_hide_status[i];
}

EMAIL_API EMAIL_FILE_TYPE email_get_file_type_from_mime_type(const char *mime_type)
{
	debug_enter();

	retvm_if(!mime_type, EMAIL_FILE_TYPE_ETC, "Invalid parameter: mime_type is NULL!");

	debug_secure("mime_type: (%s)", mime_type);

	EMAIL_FILE_TYPE ftype = EMAIL_FILE_TYPE_ETC;

	EmailFileTypeMIME *mtype = email_mime_type;
	while (mtype && mtype->mime) {
		if (!g_strcmp0(mime_type, mtype->mime)) {
			ftype = mtype->type;
			break;
		}
		mtype++;
	}

	debug_leave();
	return ftype;
}

EMAIL_API const char *email_get_icon_path_from_file_type(EMAIL_FILE_TYPE ftype)
{
	static char buff[EMAIL_SMALL_PATH_MAX] = {'\0'};
	snprintf(buff, sizeof(buff), "%s/" EMAIL_MIME_ICON_SUB_DIR "/%s",
			email_get_img_dir(), email_mime_icon[ftype]);
	return buff;
}

EMAIL_API char *email_get_current_theme_name(void)
{
	char *theme_name = NULL;
	char *save_ptr = NULL;

	/* Get current theme path. Return value is a full path of theme file. ex) blue-hd:default */
	const char *current_theme_path = elm_theme_get(NULL);
	debug_secure("current_theme_path [%s]", current_theme_path);

	if (current_theme_path == NULL) {
		debug_log("current_theme_path is NULL !!");
		return NULL;
	}

	theme_name = strdup(current_theme_path);
	if (theme_name == NULL) {
		debug_log("theme_name is NULL !!");
		return NULL;
	}

	theme_name = strtok_r(theme_name, ":", &save_ptr);
	if (theme_name == NULL) {
		debug_log("theme_name is NULL !!");
		return NULL;
	}

	debug_secure("theme_name [%s]", theme_name);

	return theme_name;
}

EMAIL_API gboolean email_check_file_exist(const gchar * path)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(STR_VALID(path), FALSE);

	if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
		return FALSE;
	}
	return TRUE;
}

EMAIL_API gboolean email_check_dir_exist(const gchar * path)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(STR_VALID(path), FALSE);

	if (!g_file_test(path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)) {
		return FALSE;
	}
	return TRUE;
}

EMAIL_API gboolean email_check_hidden(const gchar * file)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(STR_VALID(file), FALSE);

	if (file[0] == '.') {
		return TRUE;
	}
	return FALSE;
}

EMAIL_API gchar *email_parse_get_title_from_path(const gchar *path)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(STR_VALID(path), NULL);

	guint index = 0;

	gchar *file_path = g_strdup(path);
	file_path = g_strstrip(file_path);
	gchar **token_list = g_strsplit_set(file_path, "/", -1);
	g_free(file_path);	/* MUST BE. */

	RETURN_VAL_IF_FAIL(token_list != NULL, NULL);

	while (token_list[index] != NULL) {
		index++;
	}

	gchar *file_name = g_strdup(token_list[index - 1]);

	g_strfreev(token_list);	/* MUST BE. */
	token_list = NULL;

	if (file_name)
		token_list = g_strsplit_set(file_name, ".", -1);

	g_free(file_name);	/* MUST BE. */

	RETURN_VAL_IF_FAIL(token_list != NULL, NULL);

	gchar *title = g_strdup(token_list[0]);

	g_strfreev(token_list);	/* MUST BE. */

	debug_secure("title name: %s", title);

	return title;
}

EMAIL_API gchar *email_parse_get_title_from_filename(const gchar *filename)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(STR_VALID(filename), NULL);

	gchar *title = NULL;
	gint idx = 0;
	gint size = STR_LEN((gchar *)filename);

	for (idx = (size - 1); idx >= 0; --idx) {
		if (filename[idx] == '.') {
			title = g_strndup(filename, idx);
			break;
		}
	}

	if (title == NULL) {
		title = g_strdup(filename);
	}

	debug_secure("title name: %s", title);

	return title;
}

EMAIL_API gchar *email_parse_get_filename_from_path(const gchar *path)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(STR_VALID(path), NULL);

	guint index = 0;

	gchar *file_path = g_strdup(path);
	file_path = g_strstrip(file_path);
	gchar **token_list = g_strsplit_set(file_path, "/", -1);
	g_free(file_path);	/* MUST BE. */

	RETURN_VAL_IF_FAIL(token_list != NULL, NULL);

	while (token_list[index] != NULL) {
		index++;
	}

	gchar *file_name = g_strdup(token_list[index - 1]);

	g_strfreev(token_list);	/* MUST BE. */

	int len = 0;
	if (file_name)
		len = strlen(file_name);

	gchar *ext = g_strrstr(file_name, ".");
	if (ext)
		len = len - strlen(ext);

	gchar *file_name_without_ext = g_strndup(file_name, len);

	g_free(file_name);	/* MUST BE. */

	debug_secure("file name (%s)", file_name_without_ext);

	return file_name_without_ext;
}

EMAIL_API gchar *email_parse_get_filenameext_from_path(const gchar *path)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(STR_VALID(path), NULL);

	guint index = 0;

	gchar *file_path = g_strdup(path);
	file_path = g_strstrip(file_path);
	gchar **token_list = g_strsplit_set(file_path, "/", -1);
	g_free(file_path);	/* MUST BE. */

	RETURN_VAL_IF_FAIL(token_list != NULL, NULL);

	while (token_list[index] != NULL) {
		index++;
	}

	gchar *file_name = g_strdup(token_list[index - 1]);

	g_strfreev(token_list);	/* MUST BE. */

	debug_secure("file name (%s)", file_name);

	return file_name;
}

EMAIL_API void email_parse_get_filename_n_ext_from_path(const gchar *path, gchar **ret_file_name, gchar **ret_ext)
{
	debug_enter();
	RETURN_IF_FAIL(STR_VALID(path));

	guint index = 0;

	gchar *file_path = g_strdup(path);
	file_path = g_strstrip(file_path);
	gchar **token_list = g_strsplit_set(file_path, "/", -1);
	g_free(file_path);	/* MUST BE. */

	RETURN_IF_FAIL(token_list != NULL);

	while (token_list[index] != NULL) {
		index++;
	}

	gchar *file_name = g_strdup(token_list[index - 1]);

	g_strfreev(token_list);	/* MUST BE. */

	int len = 0;
	if (file_name)
		len = strlen(file_name);

	gchar *ext = g_strrstr(file_name, ".");
	if (ext)
		len = len - strlen(ext);

	*ret_file_name = g_strndup(file_name, len);

	*ret_ext = g_strdup(ext);

	g_free(file_name);

	debug_secure("file name (%s), file ext (%s)", *ret_file_name, *ret_ext);
}

EMAIL_API gchar *email_parse_get_filepath_from_path(const gchar *path)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(STR_VALID(path), NULL);

	gchar *file_path = NULL;
	gint i = 0;
	gint size = STR_LEN((gchar *)path);

	for (i = (size - 1); i >= 0; --i) {
		if (path[i] == '/') {
			file_path = g_strndup(path, i + 1);
			break;
		}
	}

	debug_secure("file path (%s)", file_path);

	return file_path;
}

EMAIL_API gchar *email_parse_get_ext_from_filename(const gchar *filename)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(STR_VALID(filename), NULL);

	gchar *ext = NULL;
	gint i = 0;
	gint size = STR_LEN((gchar *)filename);

	for (i = (size - 1); i >= 0; --i) {
		if (filename[i] == '.') {
			ext = g_strndup(&(filename[i + 1]), size - i - 1);
			break;
		}
	}

	debug_secure("file ext (%s)", ext);

	return ext;
}

EMAIL_API gchar *email_get_temp_dirname(const gchar *tmp_folder_path)
{
	debug_enter();
	debug_secure("tmp_folder_path (%s)", tmp_folder_path);
	pid_t pid = getpid();
	char *dirname = g_strdup_printf("%s/%d", tmp_folder_path, pid);
	debug_leave();
	return dirname;
}

EMAIL_API gboolean email_save_file(const gchar *path, const gchar *buf, gsize len)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(STR_VALID(path), FALSE);

	debug_secure("path (%s)", path);

	gboolean success_flag = TRUE;

	if (STR_LEN((gchar *)buf) > 0 && len > 0) {
		int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
		if (fd != -1) {
			ssize_t nwrite = write(fd, (const void *)buf, (size_t) len);
			debug_log("nwrite(%d)", nwrite);
			if (nwrite == -1) {
				debug_error("write() failed! (%d): %s", errno, strerror(errno));
				success_flag = FALSE;
			}
			close(fd);
		} else {
			debug_error("open() failed! (%d): %s", errno, strerror(errno));
			success_flag = FALSE;
		}
	} else {
		debug_log("check the buf!!");
		success_flag = FALSE;
	}

	return success_flag;
}

EMAIL_API gchar *email_get_buff_from_file(const gchar *path, guint max_kbyte)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(STR_VALID(path), NULL);

	debug_secure("path (%s)", path);

	gsize length = 0;
	gchar *content = NULL;
	GError *error = NULL;

	if (!g_file_get_contents(path, &content, &length, &error)) {
		debug_log("read fail err[%d]", error ? error->code : 0);
		debug_secure("err[%s]", error ? error->message : NULL);
		if (error) g_error_free(error);
	}

	return content;
}

EMAIL_API void email_dump_buff(const gchar *buff, const gchar *name)
{
	debug_enter();
	RETURN_IF_FAIL(STR_VALID(buff));

	tzset();	/* MUST BE. */
	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	retm_if(!t, "localtime() - failed");

	gchar buff_path[MAX_PATH_LEN] = "\0";

	g_snprintf(buff_path, sizeof(buff_path), "%s/%04d.%02d.%02d[%02dH.%02dM.%02dS]_%s.html",
			email_get_phone_tmp_dir(),
			t->tm_year + 1900, t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, name);
	debug_secure("path [%s]", buff_path);

	gboolean result = email_save_file(buff_path, buff, STR_LEN((gchar *)buff));
	if (!result) {
		debug_log("email_save_file fail!");
	}
}

EMAIL_API char *email_body_encoding_convert(char *text_content, char *from_charset, char *to_charset)
{
	debug_enter();
	retvm_if(text_content == NULL, NULL, "Invalid parameter: text_content[NULL]");
	retvm_if(from_charset == NULL, NULL, "Invalid parameter: from_charset[NULL]");
	retvm_if(to_charset == NULL, NULL, "Invalid parameter: to_charset[NULL]");

	char *encoded_text = NULL;
	char *charset = NULL;
	charset = g_ascii_strup(from_charset, strlen(from_charset));

	if (charset) {
		debug_secure("the charset of parent mail: (%s)", charset);
		/* "ks_c_5601-1987" is not a encoding name. it's just charset. there's no code page on IANA for "ks_c_5601-1987". it may be a fault.
		 * So we should convert this to encoding name "EUC-KR"
		 * CP949 is super set of EUC-KR, so we use CP949 first */
		if (!g_ascii_strcasecmp(charset, "KS_C_5601-1987")) {
			debug_secure("change: KS_C_5601-1987 ===> CP949");
			g_free(charset);
			charset = g_strdup("CP949");
		} else if (!g_ascii_strcasecmp(charset, "ISO-2022-JP")) {
			/* iso-2022-jp-2 is a superset of iso-2022-jp. In some email,
			 * iso-2022-jp is not converted to utf8 correctly. So in this case,
			 * we use iso-2022-jp-2 instead.*/
			debug_secure("change: ISO-2022-JP ===> ISO-2022-JP-2");
			g_free(charset);
			charset = g_strdup("ISO-2022-JP-2");
		}

		/* If the charset of parent mail is not "UTF-8", it should be converted to "UTF-8" to display the contents properly.
		 * If the charset of parent mail is unknown or same as utf-8, do not convert encoding to utf-8. */
		if ((g_ascii_strcasecmp(charset, UNKNOWN_CHARSET_PLAIN_TEXT_FILE) != 0) && (g_ascii_strcasecmp(charset, to_charset) != 0)) {
			debug_log("convert encoding of parent mail to utf-8");
			GError *error = NULL;
			gsize read_size = 0;
			gsize write_size = 0;

			char *tocharset = g_ascii_strup(to_charset, strlen(to_charset));
			if (!strstr(tocharset, "//IGNORE")) {
				char *s = g_strconcat(tocharset, "//IGNORE", NULL);
				g_free(tocharset);
				tocharset = s;
			}

			if (tocharset) {
				encoded_text = g_convert(text_content, -1, tocharset, charset, &read_size, &write_size, &error);
				if (!encoded_text && strcmp(charset, "CP949") == 0) {
					debug_secure("CP949 -> EUC-KR");
					encoded_text = g_convert(text_content, -1, tocharset, "EUC-KR", &read_size, &write_size, &error);
				}
				g_free(tocharset);
			}
			if (!encoded_text) {
				debug_error("g_convert() failed! encoded_text is NULL!");
				if (error) {
					debug_error("error_code:[%d], msg:(%s)", error->code, error->message);
					g_error_free(error);
				}
			}
		}
		g_free(charset);
	}

	return encoded_text;
}

EMAIL_API gchar *email_get_file_size_string(guint64 size)
{
	debug_enter();
	gchar str_buf[EMAIL_BUFF_SIZE_TIN] = "\0";
	gchar size_str[EMAIL_STR_SIZE] = "\0";

	if (size < 1024) {
		g_snprintf(size_str, sizeof(size_str), "%d", (gint) size);
		g_snprintf(str_buf, sizeof(str_buf), email_get_email_string("IDS_EMAIL_BODY_PS_B"), size_str);
	} else {
		gdouble tmpsize = (gdouble) (size / 1024.);

		if (tmpsize < 1024) {
			g_snprintf(size_str, sizeof(size_str), "%.1f", tmpsize);
			g_snprintf(str_buf, sizeof(str_buf), email_get_email_string("IDS_EMAIL_BODY_PS_KB"), size_str);
		} else {
			tmpsize /= 1024.;

			if (tmpsize < 1024) {
				g_snprintf(size_str, sizeof(size_str), "%.1f", tmpsize);
				g_snprintf(str_buf, sizeof(str_buf), email_get_email_string("IDS_EMAIL_BODY_PS_MB"), size_str);
			} else {
				tmpsize /= 1024.;
				g_snprintf(size_str, sizeof(size_str), "%.1f", tmpsize);
				g_snprintf(str_buf, sizeof(str_buf), email_get_email_string("IDS_EMAIL_BODY_PS_GB"), size_str);
			}
		}
	}
	return STR_LEN(str_buf) > 0 ? g_strdup(str_buf) : NULL;
}

EMAIL_API guint64 email_get_file_size(const gchar *path)
{
	debug_enter();
	guint64 size = 0;
	struct stat st;
	memset(&st, 0, sizeof(struct stat));

	debug_secure("path:%s", path);
	if (stat(path, &st) == 0) {
		if (S_ISREG(st.st_mode)) {
			size = (guint64) st.st_size;
			debug_secure("size (%llu)", size);
		}
	} else {
		debug_error("stat() failed! (%d): %s", errno, strerror(errno));
		size = 0;
	}
	return size;
}

EMAIL_API gboolean email_get_address_validation(const char *address)
{
	debug_enter();

	gboolean ret = FALSE;

	RETURN_VAL_IF_FAIL(STR_VALID(address), FALSE);

	/* this check is done for avoiding processing very long text */
	if (strlen(address) > (EMAIL_LIMIT_EMAIL_ADDRESS_LOCAL_PART_LENGTH + EMAIL_LIMIT_EMAIL_ADDRESS_DOMAIN_PART_LENGTH + 1)) {
		debug_warning("email address exceeded in max length!");
		return FALSE;
	}

	gchar **strip_string = g_strsplit(address, "@", -1);
	if ((strip_string[0] && (strlen(strip_string[0]) > EMAIL_LIMIT_EMAIL_ADDRESS_LOCAL_PART_LENGTH)) ||
		(strip_string[1] && (strlen(strip_string[1]) > EMAIL_LIMIT_EMAIL_ADDRESS_DOMAIN_PART_LENGTH))) {
		debug_warning("email address exceeded in max length!");
		g_strfreev(strip_string);
		return FALSE;
	}
	g_strfreev(strip_string);

	ret = email_get_is_pattern_matched(EMAIL_ADDRESS_REGEX, address);

	debug_leave();
	return ret;
}

EMAIL_API gboolean email_get_is_pattern_matched(const char *pattern, const char *str)
{
	debug_enter();
	GError *error = NULL;
	GRegex *regex;
	GMatchInfo *match_info = NULL;

	regex = g_regex_new(pattern, G_REGEX_CASELESS, 0, &error);
	if (error != NULL) {
		g_clear_error(&error);
		return FALSE;
	}

	if (!g_regex_match(regex, str, 0, &match_info)) {
		g_clear_error(&error);
		g_match_info_free(match_info);
		g_regex_unref(regex);
		return FALSE;
	}

	if (!g_match_info_matches(match_info)) {
		g_clear_error(&error);
		g_match_info_free(match_info);
		g_regex_unref(regex);
		return FALSE;
	}

	g_clear_error(&error);
	g_match_info_free(match_info);
	g_regex_unref(regex);

	return TRUE;
}

static void _strip_character_from_string(gchar **string, char ch)
{
	gchar delim[EMAIL_DELIMETER_SIZE] = {'\0'};
	snprintf(delim, sizeof(delim), "%c", ch);

	if (strchr(*string, ch)) {
		gchar **strip_string = g_strsplit(*string, delim, -1);
		gchar *join_string = g_strjoinv(NULL, strip_string);

		g_strfreev(strip_string);
		g_free(*string);

		*string = join_string;
	}
}

EMAIL_API gboolean email_get_recipient_info(const gchar *_recipient, gchar **_nick, gchar **_addr)
{
	debug_enter();

	RETURN_VAL_IF_FAIL(STR_VALID(_recipient), FALSE);

	gchar *buff_nick = NULL;
	gchar *buff_addr = NULL;
	char *nick = NULL;
	char *addr = NULL;
	gboolean res = FALSE;
	gboolean found = FALSE;

	gchar *buff = g_strdup(_recipient);
	buff = g_strstrip(buff);

	/* debug_secure("buff:[%d:%s]\n", STR_LEN(buff), buff); */

	gint i = 0;
	gint j = 0;
	gint size = STR_VALID(buff) ? STR_LEN(buff) : 0;

	/* Separate whole string into display name and email address */
	for (i = size - 1; i >= 0; i--) {
		if (buff[i] == '>') {
			for (j = i - 1; j >= 0; j--) {
				if (buff[j] == '<') {
					buff_addr = g_strndup(&buff[j + 1], (i - (j + 1)));
					if (j > 0) {
						buff_nick = g_strndup(buff, j);
					}
					found = TRUE;
					break;
				}
			}

			if (!found) {
				buff_addr = g_strndup(buff_addr, i - 1);
			}

			/* debug_secure("buff_nick = [%d:%s]\n", STR_LEN(buff_nick), buff_nick?buff_nick:"@niL"); */
			/* debug_secure("buff_addr = [%d:%s]\n", STR_LEN(buff_addr), buff_addr?buff_addr:"@niL"); */

			if (STR_LEN(buff_nick)) {
				nick = g_strdup(buff_nick);
			}
			if (STR_LEN(buff_addr)) {
				addr = g_strdup(buff_addr);
			}

			res = TRUE;
			break;
		}
	}

	/* If there's no '<' character in string, use whole string as email address */
	if (!res && buff) {
		addr = g_strdup(buff);
		res = TRUE;
	}

	if (buff != NULL) {
		g_free(buff);
	}

	FREE(buff_addr);
	FREE(buff_nick);

	/* debug_secure("nick = [%d:%s]\n", STR_LEN(nick), nick?nick:"@niL"); */
	/* debug_secure("addr = [%d:%s]\n", STR_LEN(addr), addr?addr:"@niL"); */

	/* If there's ", ' character in display name, strip it. */
	if (STR_LEN(nick)) {
		_strip_character_from_string(&nick, '\"');
		_strip_character_from_string(&nick, '\'');
	}

	/* If there's " character in email address and there's no display name, strip it. */
	if (!nick && STR_LEN(addr)) {
		_strip_character_from_string(&addr, '\"');
	}

	if (nick)
		nick = g_strstrip(nick);
	if (addr)
		addr = g_strstrip(addr);

	*_nick = nick;
	*_addr = addr;

	debug_secure("nick = [%d:%s]\n", STR_LEN(*_nick), *_nick ? *_nick : "@niL");
	debug_secure("addr = [%d:%s]\n", STR_LEN(*_addr), *_addr ? *_addr : "@niL");

	debug_leave();
	return res;
}

EMAIL_API gchar *email_get_sync_name_list(const gchar *recipient)
{
#define COMMA_S ","
#define COMMA_C ','
	debug_enter();
	RETURN_VAL_IF_FAIL(STR_VALID(recipient), NULL);

	gchar *addr_list = g_strdup(recipient);
	gchar *stx = g_strdup_printf("%c", 0x02);
	const gchar etx = 0x03;

	/* removes leading & trailing spaces. */
	addr_list = g_strstrip(addr_list);

	gchar **token_list = g_strsplit_set(addr_list, stx, -1);

	if (addr_list != NULL) {
		g_free(addr_list);
	}

	g_free(stx);

	RETURN_VAL_IF_FAIL(token_list != NULL, NULL);

	gchar *sync_list = NULL;
	guint index = 1;

	while (token_list[index] != NULL) {

		gchar *token = strchr(token_list[index], etx);

		if (token != NULL && STR_LEN(token) > 1) {

			gchar *temp = sync_list;

			if (index == 1) {
				sync_list = g_strdup_printf("%s", token + 1);
			} else {
				sync_list = g_strdup_printf("%s%s", temp, token + 1);
			}

			if (temp != NULL) {
				g_free(temp);	/* MUST BE. */
			}
		}

		index++;
	}

	g_strfreev(token_list);	/* MUST BE. */

	return sync_list;
}

EMAIL_API guint email_get_recipient_count(const gchar *recipients)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(STR_VALID(recipients), 0);

	guint index = 0;

	gchar *addr_list = g_strdup(recipients);

	/* removes leading & trailing spaces. */
	addr_list = g_strstrip(addr_list);

	gchar **token_list = g_strsplit_set(addr_list, ",", -1);

	if (addr_list != NULL) {
		g_free(addr_list);
	}

	RETURN_VAL_IF_FAIL(token_list != NULL, 0);

	while (token_list[index] != NULL) {
		index++;
	}

	g_strfreev(token_list);	/* MUST BE. */

	return index;
}

EMAIL_API gchar *email_cut_text_by_char_len(const gchar *text, gint len)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(STR_VALID(text), NULL);
	RETURN_VAL_IF_FAIL(len > 0, NULL);

	gchar *offset = g_utf8_offset_to_pointer(text, len);
	gchar *ret_text = (gchar *)g_malloc0(offset - text + 1);

	STR_NCPY(ret_text, (gchar *)text, offset - text);

	return ret_text;
}

EMAIL_API gchar *email_cut_text_by_byte_len(const gchar *text, gint len)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(STR_VALID(text), NULL);
	RETURN_VAL_IF_FAIL(len > 0, NULL);

	gint char_len2 = g_utf8_strlen(text, len);
	gchar *offset = g_utf8_offset_to_pointer(text, char_len2);

	gchar *ret_text = (gchar *)g_malloc0(offset - text + 1);

	STR_NCPY(ret_text, (gchar *)text, offset - text);

	return ret_text;
}

EMAIL_API int email_get_default_first_day_of_week()
{
	i18n_error_code_e status = I18N_ERROR_NONE;
	i18n_uchar utf16_timezone[EMAIL_BUFF_SIZE_MID] = {0};
	char timezone[EMAIL_BUFF_SIZE_SML] = {'\0'};
	int first_day_of_week = 0, len = 0;
	int default_first_day_of_week = -1;
	char *locale_tmp = NULL;

	int res = system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_COUNTRY, &locale_tmp);
	if (res != SYSTEM_SETTINGS_ERROR_NONE || locale_tmp == NULL) {
		debug_critical("system_settings_get_value_string() failed. res = %d, local = %s", res, locale_tmp);
		FREE(locale_tmp);
		return default_first_day_of_week;
	}

	status = i18n_ulocale_set_default(locale_tmp);
	if (status != I18N_ERROR_NONE) {
		debug_critical("i18n_ulocale_set_default() failed: %d", status);
		FREE(locale_tmp);
		return default_first_day_of_week;
	}

	FREE(locale_tmp);

	i18n_ustring_copy_ua_n(utf16_timezone, timezone, sizeof(timezone));
	len = i18n_ustring_get_length(utf16_timezone);

	i18n_ucalendar_h cal = NULL;
	i18n_ucalendar_create(utf16_timezone, len, locale, I18N_UCALENDAR_TRADITIONAL, &cal);
	retvm_if(!cal, -1, "cal is null");

	i18n_ucalendar_get_attribute(cal, I18N_UCALENDAR_FIRST_DAY_OF_WEEK, &first_day_of_week);

	i18n_ucalendar_destroy(cal);

	return first_day_of_week - 1;
}

EMAIL_API bool email_time_isequal(time_t first_time_t, time_t second_time_t)
{
	return first_time_t == second_time_t ? TRUE : FALSE;
}

EMAIL_API bool email_time_isbigger(time_t first_time_t, time_t second_time_t)
{
	return first_time_t > second_time_t ? TRUE : FALSE;
}

EMAIL_API time_t email_convert_datetime(const char *datetime_str/* YYYYMMDDHHMMSS */)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(STR_VALID(datetime_str), -1);

	char buf[EMAIL_BUFF_SIZE_TIN] = "\0";
	struct tm t;

	memset(&t, 0, sizeof(struct tm));

	/* Year. */
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "%.4s", datetime_str);
	t.tm_year = atoi(buf) - 1900;

	/* Month. */
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "%.2s", datetime_str + 4);
	t.tm_mon = atoi(buf) - 1;

	/* Day. */
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "%.2s", datetime_str + 6);
	t.tm_mday = atoi(buf);

	/* Hour. */
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "%.2s", datetime_str + 8);
	t.tm_hour = atoi(buf);

	/* Minute. */
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "%.2s", datetime_str + 10);
	t.tm_min = atoi(buf);

	/* Second. */
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "%.2s", datetime_str + 12);
	t.tm_sec = atoi(buf);

	return mktime(&t) + email_get_time_revision();
}

static int is_today(struct tm *req_tm, struct tm *now_tm)
{
	if (req_tm->tm_year == now_tm->tm_year && req_tm->tm_yday == now_tm->tm_yday)
		return 1;
	else
		return 0;
}

EMAIL_API char *email_get_str_datetime(const time_t req_time)
{
	/* debug_log("req_time(%d)", req_time); */

	tzset(); /* MUST BE. */

	time_t now_time = time(NULL);

	struct tm *dummy = localtime(&now_time);
	retvm_if(!dummy, NULL, "localtime() - failed");
	struct tm now_tm;
	memcpy(&now_tm, dummy, sizeof(struct tm));

	dummy = localtime(&req_time);
	retvm_if(!dummy, NULL, "localtime() - failed");
	struct tm req_tm;
	memcpy(&req_tm, dummy, sizeof(struct tm));

	/* debug_secure("*now : %d.%d.%d", now_tm.tm_year + 1900, now_tm.tm_mon, now_tm.tm_mday);
	 * debug_secure("*req : %d.%d.%d", req_tm.tm_year + 1900, req_tm.tm_mon, req_tm.tm_mday);*/
	email_mutex_lock();

	char *datetime = ({
			char *_ret_str = NULL;
			if (is_today(&req_tm, &now_tm)) {
				/* today or yesterday */
				_ret_str = (is_24_hr_format == false) ?
					email_get_date_text_with_formatter(formatter_hmm, (void *)&req_time) :
					email_get_date_text_with_formatter(formatter_Hmm, (void *)&req_time);
			} else if (req_tm.tm_year < now_tm.tm_year || req_tm.tm_year > now_tm.tm_year) /* previous year or future */
				_ret_str = email_get_date_text_with_formatter(formatter_MMMyyyy, (void *)&req_time);
			else /* days before yesterday in this year, sometimes, future mail arrives :) */
				_ret_str = email_get_date_text_with_formatter(formatter_MMMd, (void *)&req_time);
			_ret_str;
		});
	email_mutex_unlock();

	return datetime;
}

EMAIL_API char *email_get_date_text(const char *locale, char *skeleton, void *time)
{
	debug_enter();
	char formattedString[EMAIL_BUFF_SIZE_BIG] = {'\0'};
	i18n_uchar customSkeleton[EMAIL_BUFF_SIZE_MID] = {'\0'};
	int skeletonLength = strlen(skeleton);

	email_mutex_lock();

	i18n_ustring_copy_ua_n(customSkeleton, skeleton, skeletonLength);
	_generate_best_pattern(locale, customSkeleton, formattedString, time);

	email_mutex_unlock();
	return g_strdup(formattedString);
}

EMAIL_API char *email_get_date_text_with_formatter(i18n_udate_format_h formatter, void *time)
{
	char formattedString[EMAIL_BUFF_SIZE_BIG] = {'\0'};

	i18n_error_code_e status = I18N_ERROR_NONE;
	i18n_udate date = 0;
	i18n_uchar formatted[EMAIL_BUFF_SIZE_MID] = {'\0'};
	int32_t formattedCapacity;
	int32_t formattedLength;

	formattedCapacity = (int32_t) (sizeof(formatted) / sizeof(formatted[0]));

	if (time) {
		time_t msg_time = *(time_t *)time;
		date = (i18n_udate)msg_time * 1000; /* Equivalent to Date = ucal_getNow() in Milliseconds */
	}


	status = I18N_ERROR_NONE;
	status = i18n_udate_format_date(formatter, date, formatted, formattedCapacity, NULL, &formattedLength);
	/* debug_log("formattedLength: %d", formattedLength); */
	if (status != I18N_ERROR_NONE) {
		debug_critical("i18n_udate_create() failed: %d", status);
	}

	i18n_ustring_copy_au_n(formattedString, formatted, EMAIL_BUFF_SIZE_BIG);

	return g_strdup(formattedString);
}

EMAIL_API void email_feedback_init(void)
{
	debug_enter();
	feedback_initialize();
}

EMAIL_API void email_feedback_deinit(void)
{
	debug_enter();
	feedback_deinitialize();
}

EMAIL_API void email_feedback_play_tap_sound(void)
{
	debug_enter();
	feedback_play(FEEDBACK_PATTERN_TAP);
}

EMAIL_API void email_mutex_init(void)
{
	debug_enter();
	pthread_mutex_init(&mutex, NULL);
}

EMAIL_API void email_mutex_lock(void)
{
	pthread_mutex_lock(&mutex);
}

EMAIL_API void email_mutex_unlock(void)
{
	pthread_mutex_unlock(&mutex);
}

EMAIL_API void email_mutex_destroy(void)
{
	debug_enter();
	pthread_mutex_destroy(&mutex);
}

EMAIL_API int email_open_pattern_generator(void)
{
	debug_enter();
	if (pattern_generator) {
		debug_log("i18n_udatepg_create() has been already called");
		return 1;
	}

	i18n_error_code_e status = I18N_ERROR_NONE;
	/* API to set default locale */
	status = i18n_ulocale_set_default(getenv("LC_TIME"));
	if (status != I18N_ERROR_NONE) {
		debug_critical("i18n_ulocale_set_default() failed: %d", status);
		return -1;
	}

	/* API to get default locale */
	i18n_ulocale_get_default(&locale);
	debug_secure("i18n_ulocale_get_default: %s", locale);

	/* remove ".UTF-8" in locale */
	char locale_tmp[EMAIL_BUFF_SIZE_SML] = {'\0'};
	strncpy(locale_tmp, locale, sizeof(locale_tmp) - 1);
	char *p = g_strstr_len(locale_tmp, strlen(locale_tmp), ".UTF-8");
	if (p) {
		*p = 0;
	}
	debug_secure("locale_tmp: %s", locale_tmp);

	int retcode = system_settings_get_value_bool(SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR, &is_24_hr_format);
	if (retcode != SYSTEM_SETTINGS_ERROR_NONE) {
		debug_log("system_settings_get_value_bool failed");
	}

	status = I18N_ERROR_NONE;
	status = i18n_udatepg_create(locale_tmp, &pattern_generator);
	if (!pattern_generator || status != I18N_ERROR_NONE) {
		debug_secure("i18n_udatepg_create() failed: %d", status);
		return -1;
	}

	_open_pattern_n_formatter(locale_tmp, "hmm", &formatter_hmm);
	_open_pattern_n_formatter(locale_tmp, "Hmm", &formatter_Hmm);
	_open_pattern_n_formatter(locale_tmp, "MMMyyyy", &formatter_MMMyyyy);
	_open_pattern_n_formatter(locale_tmp, "MMMd", &formatter_MMMd);
	_open_pattern_n_formatter(locale_tmp, "EEEMMMd", &formatter_EEEMMMd);

	return 0;
}

EMAIL_API int email_close_pattern_generator(void)
{
	debug_enter();

	if (pattern_generator) {
		i18n_udatepg_destroy(pattern_generator);
		pattern_generator = NULL;
	}

	_close_pattern_n_formatter(formatter_hmm);
	_close_pattern_n_formatter(formatter_Hmm);
	_close_pattern_n_formatter(formatter_MMMyyyy);
	_close_pattern_n_formatter(formatter_MMMd);
	_close_pattern_n_formatter(formatter_EEEMMMd);

	return 0;
}

EMAIL_API int email_generate_pattern_for_local(const char *local, const char *pattern_format, char *gen_pattern, int gen_pattern_size)
{
	i18n_error_code_e status = I18N_ERROR_NONE;
	i18n_udatepg_h udatepg = NULL;
	i18n_uchar format[EMAIL_BUFF_SIZE_MID] = {'\0'};
	i18n_uchar best_pattern[EMAIL_BUFF_SIZE_MID] = {'\0'};
	int pattern_len = 0;

	status = i18n_udatepg_create(local, &udatepg);
	if (!udatepg || status != I18N_ERROR_NONE) {
		debug_critical("i18n_udatepg_create() failed: %d", status);
		return -1;
	}

	i18n_ustring_copy_ua_n(format, pattern_format, EMAIL_BUFF_SIZE_MID);

	status = i18n_udatepg_get_best_pattern(udatepg, format, EMAIL_BUFF_SIZE_MID,
			best_pattern, EMAIL_BUFF_SIZE_MID, &pattern_len);
	if (status != I18N_ERROR_NONE) {
		debug_critical("i18n_udatepg_get_best_pattern() failed: %d", status);
		i18n_udatepg_destroy(udatepg);
		return -1;
	}

	i18n_ustring_copy_au_n(gen_pattern, best_pattern, gen_pattern_size);
	i18n_udatepg_destroy(udatepg);

	return 0;
}

static int _open_pattern_n_formatter(const char *locale, char *skeleton, i18n_udate_format_h *formatter)
{
	debug_enter();
	i18n_error_code_e status = I18N_ERROR_NONE;
	i18n_uchar bestPattern[EMAIL_BUFF_SIZE_MID] = {'\0'};
	i18n_uchar customSkeleton[EMAIL_BUFF_SIZE_MID] = {'\0'};
	int32_t bestPatternCapacity;
	int32_t bestPatternLength, len;
	int skeletonLength = strlen(skeleton);

	i18n_ustring_copy_ua_n(customSkeleton, skeleton, skeletonLength);
	len = i18n_ustring_get_length(customSkeleton);

	bestPatternCapacity = (int32_t) (sizeof(bestPattern) / sizeof(bestPattern[0]));
	status = i18n_udatepg_get_best_pattern(pattern_generator, customSkeleton, len, bestPattern, bestPatternCapacity, &bestPatternLength);
	debug_log("bestPatternLength: %d", bestPatternLength);

	if (status != I18N_ERROR_NONE) {
		debug_critical("i18n_udatepg_get_best_pattern() failed: %d", status);
		return -1;
	}

	status = I18N_ERROR_NONE;
	status = i18n_udate_create(CUSTOM_I18N_UDATE_IGNORE, CUSTOM_I18N_UDATE_IGNORE, locale, NULL, -1, bestPattern, -1, formatter);
	if (status > I18N_ERROR_NONE) { /* from the definition of U_FAILURE */
		debug_critical("i18n_udate_create() failed: %d", status);
		return -1;
	}

	return 0;
}

static int _close_pattern_n_formatter(i18n_udate_format_h formatter)
{
	debug_enter();

	i18n_udate_destroy(formatter);

	return 0;
}

static void _generate_best_pattern(const char *locale, i18n_uchar *customSkeleton, char *formattedString, void *time)
{
	debug_enter();
	i18n_error_code_e status = I18N_ERROR_NONE;
	i18n_udate_format_h formatter;
	i18n_udate date = 0;
	i18n_uchar bestPattern[EMAIL_BUFF_SIZE_MID] = {'\0'};
	i18n_uchar formatted[EMAIL_BUFF_SIZE_MID] = {'\0'};
	int32_t bestPatternCapacity, formattedCapacity;
	int32_t bestPatternLength, formattedLength, len;

	bestPatternCapacity = (int32_t) (sizeof(bestPattern) / sizeof(bestPattern[0]));
	len = i18n_ustring_get_length(customSkeleton);

	status = i18n_udatepg_get_best_pattern(pattern_generator, customSkeleton, len, bestPattern, bestPatternCapacity, &bestPatternLength);
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

	i18n_ustring_copy_au_n(formattedString, formatted, EMAIL_BUFF_SIZE_BIG);
	i18n_udate_destroy(formatter);
}

EMAIL_API char *email_get_timezone_str(void)
{
	debug_enter();

	char buf[MAX_STR_LEN];
	ssize_t len = readlink(EMAIL_ETC_LOCALTIME_PATH, buf, sizeof(buf) - 1);

	if (len != -1) {
		buf[len] = '\0';
	} else {
		/* handle error condition */
		debug_critical("readlink() failed");
		return NULL;
	}
	return g_strdup(buf + 20); /* Asia/Seoul */
}

gint _email_get_default_clock_time_zone(void)
{
	guint time_zone = email_get_timezone_in_minutes("+09");
	debug_secure("time_zone: %d", time_zone);
	return time_zone;
}

EMAIL_API gint email_get_clock_time_zone(void)
{
	debug_enter();
	char *time_zone_str = NULL;
	char *display_name = NULL;
	guint time_zone = 0;
	i18n_timezone_h tzone = NULL;

	int res = system_settings_get_value_string(
			SYSTEM_SETTINGS_KEY_LOCALE_TIMEZONE,
			&time_zone_str);

	if (res != SYSTEM_SETTINGS_ERROR_NONE || time_zone_str == NULL) {
		debug_error("unable to get system timezone"
				"res = %d, timezone = %s", res, time_zone_str);

		FREE(time_zone_str);
		return _email_get_default_clock_time_zone();
	}

	debug_secure("timezone string: %s", time_zone_str);

	res = i18n_timezone_create(&tzone, time_zone_str);
	if (res != I18N_ERROR_NONE) {
		debug_error("failed to create timezone handler. res = %d", res);
		FREE(time_zone_str);
		return _email_get_default_clock_time_zone();
	}

	res = i18n_timezone_get_display_name_with_type(tzone,
			email_get_day_light_saving(),
			I18N_TIMEZONE_DISPLAY_TYPE_LONG_GMT,
			&display_name);

	if (res != I18N_ERROR_NONE || display_name == NULL) {
		debug_error("failed to get timezone display name. "
				"res = %d, display_name = %s", res, display_name);
		time_zone = _email_get_default_clock_time_zone();
	} else {
		char buf[MAX_STR_LEN] = {'\0'};
		snprintf(buf, MAX_STR_LEN, "%s", display_name + EMAIL_GMT_NAME_SIZE);
		time_zone = email_get_timezone_in_minutes(buf);
		debug_secure("time_zone: %d", time_zone);
	}

	i18n_timezone_destroy(tzone);
	FREE(display_name);
	FREE(time_zone_str);

	return time_zone;
}

EMAIL_API gboolean email_get_day_light_saving(void)
{
	debug_enter();
	int dst = 0;
	struct tm *ts;
	time_t ctime;

	ctime = time(NULL);
	ts = localtime(&ctime);

	if (!ts) {
		return FALSE;
	}

	dst = ts->tm_isdst;
	debug_secure("day light saving is %d", dst);

	return (dst == 1 ? TRUE : FALSE);
}

EMAIL_API int email_get_timezone_in_minutes(char *str)
{
	debug_enter();
	int sign = 1;
	int temp_int = 0;
	int i = 0;

	for (i = 0; i < strlen(str); ++i) {
		if (str[i] == ':') {
			str[i] = '.';
			break;
		}
	}

	double temp_float = atof(str);
	debug_log("temp_float: %f", temp_float);
	temp_float = temp_float * 10;
	temp_int = temp_float;

	if (temp_int > 0) {
		sign = 1;
	} else {
		sign = -1;
	}

	temp_int = temp_int * sign;

	if (temp_int % 10 != 0) {
		temp_int = temp_int / 10;
		return (sign * (60 * temp_int + 30));
	} else {
		temp_int = temp_int / 10;
		return (sign * 60 * temp_int);
	}
}

EMAIL_API gint email_get_time_revision(void)
{
	debug_enter();

	if (_g_time_rev_set)
		return _g_time_rev;

	guint tz_val = email_get_clock_time_zone() * 60;

	gboolean dst_enabled = email_get_day_light_saving();

	guint dst = dst_enabled ? 3600 : 0;

	_g_time_rev_set = 1;
	_g_time_rev = tz_val + dst;

	return (tz_val + dst);
}

EMAIL_API int email_create_folder(const char *path)
{
	debug_enter();
	if (!email_check_dir_exist(path)) {
		int nErr = -1;
		nErr = mkdir(path, 0755);
		debug_secure("errno: %d, filepath: (%s)", nErr, path);
		if (nErr == -1) {
			debug_error("mkdir() failed! (%d): %s", errno, strerror(errno));
			return -1;
		}
	} else {
		debug_secure("temp save folder already exists. filepath: (%s)", path);
		return -2;
	}
	return 0;
}

EMAIL_API gboolean email_copy_file_feedback(const char *src_full_path, const char *dest_full_path, gboolean(*copy_file_cb) (void *data, float percentage), void *cb_data)
{
	debug_enter();
	FILE *fs = NULL;
	FILE *fd = NULL;
	char buff[EMAIL_BUFF_SIZE_4K] = {'\0'};
	int n = 0;
	gboolean result = FALSE;
	gboolean stop_copying = FALSE;
	int m = 0;
	int cnt = 0;
	struct stat statbuf = { 0 };
	int ret = 0;
	int total_size = 0;
	int copied_size = 0;
	gboolean remove_dest = FALSE;
	clock_t begin;
	clock_t finish;		/* consumed time to copy whole file */
	double totaltime;
	float percentage = 0.0;

	fs = fopen(src_full_path, "rb");
	if (fs == NULL) {
		debug_error("fopen() failed! (%d): %s", errno, strerror(errno));
		return FALSE;
	}

	ret = fstat(fileno(fs), &statbuf);
	if (ret != 0) {
		debug_error("fstat() failed! (%d): %s", errno, strerror(errno));
		fclose(fs);
		return FALSE;
	}

	total_size = (int)statbuf.st_size;

	fseek(fs, 0, SEEK_SET);

	fd = fopen(dest_full_path, "wb");

	remove_dest = TRUE;

	if (fd == NULL) {
		debug_error("fopen() failed! (%d): %s", errno, strerror(errno));
		fclose(fs);
		return FALSE;
	}

	fseek(fd, 0, SEEK_SET);

	copied_size = 0;

	begin = clock();

	while (1) {
		result = feof(fs);
		if (!result) {
			n = fread(buff, sizeof(char), sizeof(buff), fs);
			if (n > 0) {
				m = fwrite(buff, sizeof(char), n, fd);

				if (m <= 0) {
					debug_error("fwrite() failed! (%d): %s", errno, strerror(errno));
					result = FALSE;
					goto CATCH;
				}

				cnt++;
				copied_size += m;

				if (cnt > 10) {
					percentage = ((float)(copied_size) / (float)total_size) * 100.0;
					if (copy_file_cb)
						stop_copying = copy_file_cb(cb_data, percentage);

					if (stop_copying) {
						result = FALSE;
						remove_dest = TRUE;
						goto CATCH;
					}

					cnt = 0;
				}
			} else {
				debug_error("fread() failed! (%d): %s", errno, strerror(errno));
				result = TRUE;
				goto CATCH;
			}
		} else {
			debug_log("End of file reached");
			result = TRUE;
			goto CATCH;
		}
	}

 CATCH:
	fflush(fd);
	fsync(fileno(fd));
	fclose(fd);
	fclose(fs);

	if (copy_file_cb && !stop_copying) {
		percentage = ((float)(copied_size) / (float)total_size) * 100.0;
		stop_copying = copy_file_cb(cb_data, percentage);
	}

	if (remove_dest && result == FALSE) {
		if (-1 == remove(dest_full_path)) {
			debug_error("remove() failed! (%d): %s", errno, strerror(errno));
		}
		sync();
	}

	if (result) {
		finish = clock();
		totaltime = (double)(finish - begin) / CLOCKS_PER_SEC;
		debug_secure("takes %f s to copy %s", totaltime, src_full_path);
	}

	return result;
}


static int termination_flag = 0;
static int pause_flag = 1;

EMAIL_API void set_app_terminated()
{
	termination_flag = 1;
}

EMAIL_API int get_app_terminated()
{
	return termination_flag;
}

EMAIL_API void set_app_paused()
{
	pause_flag = 1;
}

EMAIL_API void reset_app_paused()
{
	pause_flag = 0;
}

EMAIL_API int get_app_paused()
{
	return pause_flag;
}

EMAIL_API char *email_util_strrtrim(char *s)
{
	char *end = s + strlen(s);

	while (end != s && isspace(end[-1])) {
		--end;
	}
	*end = '\0';

	return s;
}

EMAIL_API char *email_util_strltrim(char *s)
{
	char *begin = s;

	while (*begin != '\0') {
		if (isspace(*begin)) {
			++begin;
		} else {
			s = begin;
			break;
		}
	}

	return s;
}

EMAIL_API char *email_util_strtrim(char *s)
{
	return email_util_strrtrim(email_util_strltrim(s));
}

static EmailSystemSettingsKeyEntry *_email_system_settings_key_entry_get(system_settings_key_e key)
{
	Eina_List *keys_list = _system_settings_key_list;

	Eina_List *temp_list = NULL;
	EmailSystemSettingsKeyEntry *key_entry = NULL;
	EmailSystemSettingsKeyEntry *result_key_entry = NULL;
	EINA_LIST_FOREACH(keys_list, temp_list, key_entry) {

		if (key_entry->key != key) {
			continue;
		}

		result_key_entry = key_entry;
		break;
	}
	return result_key_entry;
}

static void _email_system_settings_callback(system_settings_key_e key, void *user_data)
{
	EmailSystemSettingsKeyEntry *key_entry = _email_system_settings_key_entry_get(key);
	if (NULL == key_entry) {
		debug_error("key_entry is absent!");
		return;
	}
	if (NULL == key_entry->callbacks_list) {
		debug_error("key_entry's callback list is empty!");
		return;
	}
	Eina_List *callback_list = key_entry->callbacks_list;
	Eina_List *temp_list = NULL;
	EmailSystemSettingsCallbackEntry *callback_entry = NULL;
	EINA_LIST_FOREACH(callback_list, temp_list, callback_entry) {
		system_settings_changed_cb callback_func = callback_entry->callback;
		void *callback_data = callback_entry->user_data;

		if (NULL == callback_func) {
			continue;
		}
		callback_func(key, callback_data);
	}
}

static Eina_Bool _email_system_settings_callback_exist(system_settings_key_e key, system_settings_changed_cb callback)
{
	EmailSystemSettingsKeyEntry *key_entry = _email_system_settings_key_entry_get(key);
	if (NULL == key_entry) {
		return EINA_FALSE;
	}
	Eina_List *temp_list = NULL;
	EmailSystemSettingsCallbackEntry *callback_entry = NULL;
	Eina_Bool callback_found = EINA_FALSE;
	Eina_List *callbacks_list = key_entry->callbacks_list;
	EINA_LIST_FOREACH(callbacks_list, temp_list, callback_entry) {
		if (NULL == callback_entry) {
			continue;
		}

		if (callback_entry->callback != callback) {
			continue;
		}
		callback_found = EINA_TRUE;
		break;
	}
	return callback_found;
}

static int _email_system_settings_key_callbacks_count(system_settings_key_e key)
{
	EmailSystemSettingsKeyEntry *key_entry = _email_system_settings_key_entry_get(key);
	if (NULL == key_entry) {
		return 0;
	}
	int callback_count = eina_list_count(key_entry->callbacks_list);

	return callback_count;
}

static void _email_system_settings_key_entry_add(EmailSystemSettingsKeyEntry *key_entry)
{
	_system_settings_key_list = eina_list_append(_system_settings_key_list, key_entry);
}

static void _email_system_settings_key_entry_del(EmailSystemSettingsKeyEntry *key_entry)
{
	_system_settings_key_list = eina_list_remove(_system_settings_key_list, key_entry);
}

static void _email_system_settings_callback_entry_add(EmailSystemSettingsKeyEntry *key_entry, EmailSystemSettingsCallbackEntry *callback_entry)
{
	key_entry->callbacks_list = eina_list_append(key_entry->callbacks_list, callback_entry);
}

static void _email_system_settings_callback_entry_del(EmailSystemSettingsKeyEntry *key_entry, EmailSystemSettingsCallbackEntry *callback_entry)
{
	key_entry->callbacks_list = eina_list_remove(key_entry->callbacks_list, callback_entry);
}

static int _email_register_changed_callback(system_settings_key_e key, system_settings_changed_cb callback, void *data)
{
	if (NULL == callback) {
		return SYSTEM_SETTINGS_ERROR_INVALID_PARAMETER;
	}
	if (EINA_TRUE == _email_system_settings_callback_exist(key, callback)) {
		return SYSTEM_SETTINGS_ERROR_NONE;
	}

	EmailSystemSettingsCallbackEntry *callback_data = (EmailSystemSettingsCallbackEntry *)malloc(sizeof(EmailSystemSettingsCallbackEntry));
	if (NULL == callback_data) {
		debug_error("allocation system settings FAILED");
		return SYSTEM_SETTINGS_ERROR_INVALID_PARAMETER;
	}

	callback_data->callback = callback;
	callback_data->user_data = data;

	EmailSystemSettingsKeyEntry *key_entry = _email_system_settings_key_entry_get(key);
	if (NULL == key_entry) {
		system_settings_set_changed_cb(key, _email_system_settings_callback, NULL);

		EmailSystemSettingsKeyEntry *key_new_entry = (EmailSystemSettingsKeyEntry *)malloc(sizeof(EmailSystemSettingsKeyEntry));
		if (NULL == key_new_entry) {
			debug_error("allocation key new entry FAILED");
			free(callback_data);
			return SYSTEM_SETTINGS_ERROR_OUT_OF_MEMORY;
		}

		key_new_entry->key = key;
		key_new_entry->callbacks_list = NULL;
		_email_system_settings_callback_entry_add(key_new_entry, callback_data);
		_email_system_settings_key_entry_add(key_new_entry);
	} else {
		_email_system_settings_callback_entry_add(key_entry, callback_data);
	}

	return SYSTEM_SETTINGS_ERROR_NONE;
}

static int _email_unregister_changed_callback(system_settings_key_e key, system_settings_changed_cb callback)
{
	if (NULL == callback) {
		debug_error("callback is NULL!");
		return SYSTEM_SETTINGS_ERROR_INVALID_PARAMETER;
	}

	EmailSystemSettingsKeyEntry *key_entry = _email_system_settings_key_entry_get(key);

	if (NULL == key_entry || NULL == key_entry->callbacks_list) {
		debug_error("key_entry is absent!");
		return SYSTEM_SETTINGS_ERROR_IO_ERROR;
	}
	Eina_List *l = NULL;
	EmailSystemSettingsCallbackEntry *callback_entry = NULL;
	EINA_LIST_FOREACH(key_entry->callbacks_list, l, callback_entry) {
		void *callback_func = callback_entry->callback;
		if (callback_func == callback) {
			_email_system_settings_callback_entry_del(key_entry, callback_entry);

			free(callback_entry);
			break;
		}
	}
	if (_email_system_settings_key_callbacks_count(key) == 0) {
		system_settings_unset_changed_cb(key);
		_email_system_settings_key_entry_del(key_entry);

		free(key_entry);
	}

	return SYSTEM_SETTINGS_ERROR_NONE;
}

EMAIL_API int email_register_accessibility_font_size_changed_callback(system_settings_changed_cb func, void *data)
{
	debug_enter();
	if (_email_register_changed_callback(SYSTEM_SETTINGS_KEY_FONT_SIZE, func, data) != SYSTEM_SETTINGS_ERROR_NONE) {
		debug_error("system_setting callback registration is failed");
		return -1;
	}
	debug_leave();

	return 0;
}

EMAIL_API int email_unregister_accessibility_font_size_changed_callback(system_settings_changed_cb func)
{
	debug_enter();
	if (_email_unregister_changed_callback(SYSTEM_SETTINGS_KEY_FONT_SIZE, func) != SYSTEM_SETTINGS_ERROR_NONE) {
		debug_error("system_setting callback removal is failed");
		return -1;
	}
	debug_leave();

	return 0;
}

EMAIL_API int email_register_language_changed_callback(system_settings_changed_cb func, void *data)
{
	debug_enter();
	if (_email_register_changed_callback(SYSTEM_SETTINGS_KEY_LOCALE_LANGUAGE, func, data) != SYSTEM_SETTINGS_ERROR_NONE) {
		debug_error("system_setting callback registration is failed");
		return -1;
	}
	debug_leave();

	return 0;
}

EMAIL_API int email_unregister_language_changed_callback(system_settings_changed_cb func)
{
	debug_enter();
	if (_email_unregister_changed_callback(SYSTEM_SETTINGS_KEY_LOCALE_LANGUAGE, func) != SYSTEM_SETTINGS_ERROR_NONE) {
		debug_error("system_setting callback removal is failed");
		return -1;
	}
	debug_leave();

	return 0;
}

EMAIL_API int email_register_timezone_changed_callback(system_settings_changed_cb func, void *data)
{
	debug_enter();
	if (_email_register_changed_callback(SYSTEM_SETTINGS_KEY_TIME_CHANGED, func, data) != SYSTEM_SETTINGS_ERROR_NONE) {
		debug_error("system_setting callback registration is failed");
		return -1;
	}
	debug_leave();

	return 0;
}

EMAIL_API int email_unregister_timezone_changed_callback(system_settings_changed_cb func)
{
	debug_enter();
	if (_email_unregister_changed_callback(SYSTEM_SETTINGS_KEY_TIME_CHANGED, func) != SYSTEM_SETTINGS_ERROR_NONE) {
		debug_error("system_setting callback removal is failed");
		return -1;
	}
	debug_leave();

	return 0;
}

EMAIL_API char *email_get_mime_type_from_file(const char *path)
{
	debug_enter();

	retvm_if(!path, NULL, "path is NULL!");

	char *extension = strrchr(path, '.');
	extension = (extension && (extension + 1)) ? extension + 1 : NULL;

	char *mime_type = NULL;
	int ret = mime_type_get_mime_type(extension, &mime_type);
	retvm_if(ret != MIME_TYPE_ERROR_NONE, NULL, "mime_type_get_mime_type() failed!");

	debug_secure("mime_type:(%s)", mime_type);

	debug_leave();
	return mime_type;
}

EMAIL_API bool email_is_smime_cert_attachment(const char *mime_type)
{
	debug_enter();

	retvm_if(!mime_type, false, "Invalid parameter: mime_type is NULL!");

	bool ret = false;

	if (!g_ascii_strcasecmp(mime_type, EMAIL_MIME_TYPE_ENCRYPTED) ||
		!g_ascii_strcasecmp(mime_type, EMAIL_MIME_TYPE_SIGNED) ||
		!g_ascii_strcasecmp(mime_type, EMAIL_MIME_TYPE_ENCRYPTED_X) ||
		!g_ascii_strcasecmp(mime_type, EMAIL_MIME_TYPE_SIGNED_X)) {
		ret = true;
	}

	debug_leave();
	return ret;
}

EMAIL_API bool email_is_encryption_cert_attachment(const char *mime_type)
{
	debug_enter();

	retvm_if(!mime_type, false, "Invalid parameter: mime_type is NULL!");

	bool ret = false;

	if (!g_ascii_strcasecmp(mime_type, EMAIL_MIME_TYPE_ENCRYPTED) ||
		!g_ascii_strcasecmp(mime_type, EMAIL_MIME_TYPE_ENCRYPTED_X)) {
		ret = true;
	}

	debug_leave();
	return ret;
}

EMAIL_API bool email_is_signing_cert_attachment(const char *mime_type)
{
	debug_enter();

	retvm_if(!mime_type, false, "Invalid parameter: mime_type is NULL!");

	bool ret = false;

	if (!g_ascii_strcasecmp(mime_type, EMAIL_MIME_TYPE_SIGNED) ||
		!g_ascii_strcasecmp(mime_type, EMAIL_MIME_TYPE_SIGNED_X)) {
		ret = true;
	}

	debug_leave();
	return ret;
}

EMAIL_API int email_get_deleted_flag_mail_count(int account_id)
{
	debug_enter();

	int total_count = 0;
	int unread_count = 0;
	int i = 0;
	int err = 0;

	email_list_filter_t *filter_list = NULL;
	int filter_list_count = 0;

	if (account_id == 0) {
		filter_list_count = 5;
		filter_list = calloc(filter_list_count, sizeof(email_list_filter_t));
		if (!filter_list) {
			debug_error("fail to alloc filter_list");
			return 0;
		}
	} else {
		filter_list_count = 7;
		filter_list = calloc(filter_list_count, sizeof(email_list_filter_t));
		if (!filter_list) {
			debug_error("fail to alloc filter_list");
			return 0;
		}
	}

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_MAILBOX_TYPE;
	filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[i].list_filter_item.rule.key_value.integer_type_value = 1;
	i++;

	if (account_id != 0) {
		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
		filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
		i++;

		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
		filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_ACCOUNT_ID;
		filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_EQUAL;
		filter_list[i].list_filter_item.rule.key_value.integer_type_value = account_id;
		i++;
	}

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
	i++;

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_FLAGS_SEEN_FIELD;
	filter_list[i].list_filter_item.rule.key_value.integer_type_value = 0;
	filter_list[i].list_filter_item.rule.case_sensitivity = EMAIL_CASE_SENSITIVE;
	i++;

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
	i++;

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_SAVE_STATUS;
	filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[i].list_filter_item.rule.key_value.integer_type_value = EMAIL_MAIL_STATUS_SAVED_OFFLINE;

	err = email_count_mail(filter_list, filter_list_count, &total_count, &unread_count);
	if (err != EMAIL_ERROR_NONE) {
		debug_warning("email_count_mail - err (%d)", err);
	}

	free(filter_list);

	filter_list = NULL;
	debug_log("total:%d, unread:%d", total_count, unread_count);
	return (gint) unread_count;
}

EMAIL_API bool email_get_favourite_mail_count_ex(int *total_mail_count, int *unread_mail_count)
{
	email_list_filter_t *filter_list = NULL;
	int cnt_filter_list = 9;
	int err = 0;
	int i = 0;
	int total_count = 0, unread_count = 0;

	filter_list = calloc(cnt_filter_list, sizeof(email_list_filter_t));
	retvm_if(filter_list == NULL, false, "filter_list is NULL!");

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_LEFT_PARENTHESIS;
	i++;

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_FLAGS_FLAGGED_FIELD;
	filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[i].list_filter_item.rule.key_value.integer_type_value = EMAIL_FLAG_TASK_STATUS_ACTIVE;
	i++;

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_OR;
	i++;

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_FLAGS_FLAGGED_FIELD;
	filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[i].list_filter_item.rule.key_value.integer_type_value = EMAIL_FLAG_FLAGED;
	i++;

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_RIGHT_PARENTHESIS;
	i++;

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
	i++;

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_FLAGS_DELETED_FIELD;
	filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_NOT_EQUAL;
	filter_list[i].list_filter_item.rule.key_value.integer_type_value = 1;
	i++;

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
	i++;

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_MAILBOX_TYPE;
	filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_NOT_EQUAL;
	filter_list[i].list_filter_item.rule.key_value.integer_type_value = EMAIL_MAILBOX_TYPE_TRASH;
	i++;

	err = email_count_mail(filter_list, cnt_filter_list, &total_count, &unread_count);

	email_free_list_filter(&filter_list, cnt_filter_list);

	if (unread_mail_count)
		*unread_mail_count = unread_count;

	if (total_mail_count)
		*total_mail_count = total_count;

	debug_log("total_count(%d), unread_count(%d)", total_count, unread_count);

	if (err != EMAIL_ERROR_NONE) {
		debug_warning("email_count_mail - err (%d)", err);
		return false;
	}

	return true;
}

EMAIL_API int email_get_favourite_mail_count(bool unread_only)
{
	debug_log("unread_only : %d", unread_only);
	int total_count = 0, unread_count = 0;

	if (email_get_favourite_mail_count_ex(&total_count, &unread_count)) {
		if (unread_only)
			return unread_count;
		else
			return total_count;
	}
	return 0;
}

EMAIL_API int email_get_priority_sender_mail_count(bool unread_only)
{
	debug_log("unread_only : %d", unread_only);
	int total_count = 0, unread_count = 0;

	if (email_get_priority_sender_mail_count_ex(&total_count, &unread_count)) {
		if (unread_only)
			return unread_count;
		else
			return total_count;
	}

	return 0;
}

EMAIL_API int email_get_priority_sender_mail_count_ex(int *total_mail_count, int *unread_mail_count)
{
	int j = 0;
	int err;
	email_list_filter_t *filter_list = NULL;
	int total_count = 0, unread_count = 0;
	int cnt_filter_list = 5;

	filter_list = malloc(sizeof(email_list_filter_t) * cnt_filter_list);
	retvm_if(filter_list == NULL, false, "filter_list is NULL!");
	memset(filter_list, 0, sizeof(email_list_filter_t) * cnt_filter_list);

	filter_list[j].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[j].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_TAG_ID;
	filter_list[j].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[j].list_filter_item.rule.key_value.integer_type_value = PRIORITY_SENDER_TAG_ID;
	j++;

	filter_list[j].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[j].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
	j++;

	filter_list[j].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[j].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_MAILBOX_TYPE;
	filter_list[j].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[j].list_filter_item.rule.key_value.integer_type_value = EMAIL_MAILBOX_TYPE_INBOX;
	j++;

	filter_list[j].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[j].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
	j++;

	filter_list[j].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[j].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_FLAGS_DELETED_FIELD;
	filter_list[j].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_NOT_EQUAL;
	filter_list[j].list_filter_item.rule.key_value.integer_type_value = 1;
	j++;

	err = email_count_mail(filter_list, cnt_filter_list, &total_count, &unread_count);

	email_free_list_filter(&filter_list, cnt_filter_list);

	debug_log("total_count(%d), unread_count(%d)", total_count, unread_count);

	if (unread_mail_count)
		*unread_mail_count = unread_count;

	if (total_mail_count)
		*total_mail_count = total_count;

	if (err != EMAIL_ERROR_NONE) {
		debug_warning("email_count_mail - err (%d)", err);
		return false;
	}

	return true;

}

EMAIL_API int email_get_scheduled_outbox_mail_count_by_account_id(int account_id, bool unread_only)
{
	debug_log("unread_only : %d", unread_only);
	int total_count = 0, unread_count = 0;

	if (email_get_scheduled_outbox_mail_count_by_account_id_ex(account_id, &total_count, &unread_count)) {
		if (unread_only)
			return unread_count;
		else
			return total_count;
	}
	return 0;
}

EMAIL_API int email_get_scheduled_outbox_mail_count_by_account_id_ex(int account_id, int *total_mail_count, int *unread_mail_count)
{
	email_list_filter_t *filter_list = NULL;
	int cnt_filter_list = 0;
	int err = 0;
	int i = 0;
	int total_count = 0, unread_count = 0;

	if (account_id >= 0) {
		if (account_id == 0) { /* for all account */
			cnt_filter_list += 5;
		} else {
			cnt_filter_list += 7;
		}

		filter_list = malloc(sizeof(email_list_filter_t) * cnt_filter_list);
		retvm_if(filter_list == NULL, false, "filter_list is NULL!");
		memset(filter_list, 0, sizeof(email_list_filter_t) * cnt_filter_list);

		if (account_id != 0) {
			filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
			filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_ACCOUNT_ID;
			filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_EQUAL;
			filter_list[i].list_filter_item.rule.key_value.integer_type_value = account_id;
			i++;

			filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
			filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
			i++;
		}

		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
		filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_SAVE_STATUS;
		filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_EQUAL;
		filter_list[i].list_filter_item.rule.key_value.integer_type_value = EMAIL_MAIL_STATUS_SEND_SCHEDULED;
		i++;

		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
		filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
		i++;

		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
		filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_FLAGS_DELETED_FIELD;
		filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_NOT_EQUAL;
		filter_list[i].list_filter_item.rule.key_value.integer_type_value = 1;
		i++;

		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
		filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
		i++;

		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
		filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_MAILBOX_TYPE;
		filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_EQUAL;
		filter_list[i].list_filter_item.rule.key_value.integer_type_value = EMAIL_MAILBOX_TYPE_OUTBOX;
		i++;

		err = email_count_mail(filter_list, cnt_filter_list, &total_count, &unread_count);

		email_free_list_filter(&filter_list, cnt_filter_list);

		debug_log("account_id : %d, total_count(%d), unread_count(%d)", account_id, total_count, unread_count);

		if (unread_mail_count)
			*unread_mail_count = unread_count;

		if (total_mail_count)
			*total_mail_count = total_count;

		if (err != EMAIL_ERROR_NONE) {
			debug_warning("email_count_mail - err (%d)", err);
			return false;
		}
		return true;
	}

	return false;
}

EMAIL_API bool email_get_combined_mail_count_by_mailbox_type(int mailbox_type, int *total_mail_count, int *unread_mail_count)
{
	email_list_filter_t *filter_list = NULL;
	int cnt_filter_list = 3;
	int i = 0;
	int err = 0;
	int total_count = 0, unread_count = 0;

	if (mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX) {
		cnt_filter_list += 2;
	}

	filter_list = malloc(sizeof(email_list_filter_t) * cnt_filter_list);
	retvm_if(filter_list == NULL, false, "filter_list is NULL!");
	memset(filter_list, 0, sizeof(email_list_filter_t) * cnt_filter_list);

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_FLAGS_DELETED_FIELD;
	filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_NOT_EQUAL;
	filter_list[i].list_filter_item.rule.key_value.integer_type_value = 1;
	i++;

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
	filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
	i++;

	filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
	filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_MAILBOX_TYPE;
	filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_EQUAL;
	filter_list[i].list_filter_item.rule.key_value.integer_type_value = mailbox_type;
	i++;

	if (mailbox_type == EMAIL_MAILBOX_TYPE_OUTBOX) {
		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_OPERATOR;
		filter_list[i].list_filter_item.operator_type = EMAIL_LIST_FILTER_OPERATOR_AND;
		i++;

		filter_list[i].list_filter_item_type = EMAIL_LIST_FILTER_ITEM_RULE;
		filter_list[i].list_filter_item.rule.target_attribute = EMAIL_MAIL_ATTRIBUTE_SAVE_STATUS;
		filter_list[i].list_filter_item.rule.rule_type = EMAIL_LIST_FILTER_RULE_NOT_EQUAL;
		filter_list[i].list_filter_item.rule.key_value.integer_type_value = EMAIL_MAIL_STATUS_SEND_SCHEDULED;
		i++;
	}

	err = email_count_mail(filter_list, cnt_filter_list, &total_count, &unread_count);
	email_free_list_filter(&filter_list, cnt_filter_list);

	debug_log("mailbox_type : %d total_count(%d), unread_count(%d)", mailbox_type, total_count, unread_count);

	if (unread_mail_count)
		*unread_mail_count = unread_count;

	if (total_mail_count)
		*total_mail_count = total_count;

	if (err != EMAIL_ERROR_NONE) {
		debug_warning("email_count_mail - err (%d)", err);
		return false;
	}
	return true;
}

EMAIL_API void email_set_is_inbox_active(bool is_active)
{
	debug_enter();

	if (is_active != _email_is_inbox_active) {
		debug_log("is_active = %s", is_active ? "true" : "false");

		int ret = vconf_set_int(VCONFKEY_EMAIL_IS_INBOX_ACTIVE, (int)is_active);
		debug_error_if(ret == -1, "vconf_set_int() failed!");

		_email_is_inbox_active = is_active;
	}
}

static void _update_lang_environment(const char *lang)
{
	if (lang && strcmp(lang, "")) {
		setenv("LANGUAGE", lang, 1);
	} else {
		unsetenv("LANGUAGE");
	}
}

EMAIL_API char *email_get_datetime_format(void)
{
	debug_enter();

	char *dt_fmt = NULL, *region_fmt = NULL, *lang = NULL;
	char buf[EMAIL_BUFF_SIZE_HUG] = {'\0'};
	int res = 0;
	bool is_hour24 = false;

	lang = getenv("LANGUAGE");
	_update_lang_environment("en_US");

	res = system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_COUNTRY, &region_fmt);
	if (res == SYSTEM_SETTINGS_ERROR_NONE && region_fmt != NULL) {
		res = system_settings_get_value_bool(
				SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR, &is_hour24);
		if (res != SYSTEM_SETTINGS_ERROR_NONE) {
			debug_error("failed to get system settings locale time format. "
					"res = %d", res);
			_update_lang_environment(lang);
			FREE(region_fmt);
			FREE(lang);
			return NULL;
		}
	} else {
		debug_error("failed to get system settings locale country. "
				"res = %d", res);
		_update_lang_environment(lang);
		FREE(lang);
		return NULL;
	}

	if (is_hour24) {
		snprintf(buf, sizeof(buf), "%s_DTFMT_24HR", region_fmt);
	} else {
		snprintf(buf, sizeof(buf), "%s_DTFMT_12HR", region_fmt);
	}

	dt_fmt = dgettext("dt_fmt", buf);

	_update_lang_environment(lang);
	FREE(region_fmt);
	FREE(lang);

	debug_leave();

	return strdup(dt_fmt);
}

EMAIL_API inline char *email_get_system_string(const char *string_id)
{
	return dgettext(SYSTEM_STRING, string_id);
}

EMAIL_API inline char *email_get_email_string(const char *string_id)
{
	return dgettext(PACKAGE, string_id);
}

EMAIL_API EMAIL_NETWORK_STATUS email_get_network_state(void)
{
	debug_enter();

	EMAIL_NETWORK_STATUS ret = EMAIL_NETWORK_STATUS_AVAILABLE;
	connection_type_e net_state = CONNECTION_TYPE_DISCONNECTED;
	connection_cellular_state_e cellular_state = CONNECTION_CELLULAR_STATE_OUT_OF_SERVICE;
	connection_h connection = NULL;

	int err = connection_create(&connection);
	gotom_if(err != CONNECTION_ERROR_NONE, CATCH, "connection_create failed: %d", err);

	err = connection_get_type(connection, &net_state);
	gotom_if(err != CONNECTION_ERROR_NONE, CATCH, "connection_get_type failed: %d", err);

	err = connection_get_cellular_state(connection, &cellular_state);
	gotom_if(err != CONNECTION_ERROR_NONE, CATCH, "connection_get_cellular_state failed: %d", err);

	if (net_state == CONNECTION_TYPE_DISCONNECTED) {
		if (cellular_state == CONNECTION_CELLULAR_STATE_FLIGHT_MODE)
			ret = EMAIL_NETWORK_STATUS_FLIGHT_MODE;
		else if (cellular_state == CONNECTION_CELLULAR_STATE_CALL_ONLY_AVAILABLE)
			ret = EMAIL_NETWORK_STATUS_MOBILE_DATA_DISABLED;
		else if (cellular_state == CONNECTION_CELLULAR_STATE_ROAMING_OFF)
			ret = EMAIL_NETWORK_STATUS_ROAMING_OFF;
		else
			ret = EMAIL_NETWORK_STATUS_NO_SIM_OR_OUT_OF_SERVICE;
	}

CATCH:
	if (connection)
		connection_destroy(connection);
	debug_log("network status result: %d", ret);
	return ret;
}

EMAIL_API void email_set_viewer_launched_on_mailbox_flag(bool value)
{
	_g_viewer_launched_on_mailbox = value;
}

EMAIL_API bool email_get_viewer_launched_on_mailbox_flag()
{
	return _g_viewer_launched_on_mailbox;
}

EMAIL_API Elm_Genlist_Item_Class *email_util_get_genlist_item_class(const char *style,
		Elm_Gen_Item_Text_Get_Cb text_get,
		Elm_Gen_Item_Content_Get_Cb content_get,
		Elm_Gen_Item_State_Get_Cb state_get,
		Elm_Gen_Item_Del_Cb del)
{
	Elm_Genlist_Item_Class *itc = NULL;
	itc = elm_genlist_item_class_new();
	retvm_if(itc == NULL, NULL, "itc is NULL!");
	itc->item_style = style;
	itc->func.text_get = text_get;
	itc->func.content_get = content_get;
	itc->func.state_get = state_get;
	itc->func.del = del;

	return itc;
}


typedef struct {
	Evas_Object *popup;
	Evas_Object *win_main;
	Evas_Object *conformant;
	email_editfield_t editfield;
	Ecore_Timer *timer;
	int account_id;
	bool is_sip_on;
	Evas_Smart_Cb done_cb;
	Evas_Smart_Cb cancel_cb;
	void *data;
} Passwd_Popup_Data;

static void _password_entry_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	if (!data || !obj) {
		debug_warning("data is NULL");
		return;
	}
	Passwd_Popup_Data *pdata = (Passwd_Popup_Data *)data;
	Evas_Object *done_button = elm_object_part_content_get(pdata->popup, "button2");
	retm_if(!done_button, "no done button");

	char *pass_string = g_strdup(elm_entry_entry_get(obj));
	if (pass_string == NULL) {
		elm_object_disabled_set(done_button, EINA_TRUE);
		elm_entry_input_panel_return_key_disabled_set(obj, EINA_TRUE);
		return;
	}

	if (g_strcmp0(pass_string, "") == 0) {
		elm_object_disabled_set(done_button, EINA_TRUE);
		elm_entry_input_panel_return_key_disabled_set(obj, EINA_TRUE);
	} else {
		elm_object_disabled_set(done_button, EINA_FALSE);
		elm_entry_input_panel_return_key_disabled_set(obj, EINA_FALSE);
	}
	FREE(pass_string);
}

static void _password_popup_ok_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	if (!data || !obj) {
		debug_warning("data is NULL");
		return;
	}

	Passwd_Popup_Data *pdata = (Passwd_Popup_Data *)data;

	if (!pdata->editfield.entry || elm_entry_is_empty(pdata->editfield.entry)) {
		debug_log("password is NULL");
		DELETE_EVAS_OBJECT(pdata->popup);
		return;
	}

	char *str_password = elm_entry_markup_to_utf8(elm_entry_entry_get(pdata->editfield.entry));
	debug_secure("str_password : %s", str_password);

	int err = EMAIL_ERROR_NONE;
	email_account_t *account_info = NULL;

	err = email_engine_get_account_full_data(pdata->account_id, &account_info);

	if (err != EMAIL_ERROR_NONE || !account_info) {
		debug_warning("fail to get account info - err(%d)", err);
	} else {
		FREE(account_info->incoming_server_password);
		account_info->incoming_server_password = strdup(str_password);

		if ((err = email_update_account(pdata->account_id, account_info)) != EMAIL_ERROR_NONE) {
			debug_warning("fail to update account - err(%d)", err);
		}
	}

	if (account_info) email_free_account(&account_info, 1);

	if (pdata->done_cb) {
		pdata->done_cb(pdata->data, NULL, (void *)(ptrdiff_t)pdata->account_id);
	}
	FREE(str_password);
}

static void _password_popup_cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	if (!data || !obj) {
		debug_warning("data is NULL");
		return;
	}

	Passwd_Popup_Data *pdata = (Passwd_Popup_Data *)data;

	if (pdata->cancel_cb) {
		pdata->cancel_cb(pdata->data, NULL, NULL);
	}
}

static Eina_Bool _password_popup_start_focus_cb(void *data)
{
	debug_enter();
	if (!data) {
		debug_warning("data is NULL");
		return ECORE_CALLBACK_CANCEL;
	}

	Passwd_Popup_Data *pdata = (Passwd_Popup_Data *)data;
	pdata->timer = NULL;

	if (pdata->editfield.entry) {
		elm_object_focus_set(pdata->editfield.entry, EINA_TRUE);
	}

	return ECORE_CALLBACK_CANCEL;
}

static void _passwd_popup_set_title(Evas_Object *popup, bool hide)
{
	if (hide) {
		debug_log("hide");
		elm_object_part_text_set(popup, "title,text", NULL);
	} else {
		debug_log("show");
		elm_object_domain_translatable_part_text_set(popup, "title,text",
				PACKAGE, "IDS_EMAIL_HEADER_ENTER_NEW_PASSWORD_ABB");
	}
}

static void _passwd_sip_off(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "invalid data");
	Passwd_Popup_Data *pdata = (Passwd_Popup_Data *)data;
	int rot = elm_win_rotation_get(pdata->win_main);
	if (rot == 90 || rot == 270) {
		_passwd_popup_set_title(pdata->popup, false);
	}
	pdata->is_sip_on = false;
}

static void _passwd_sip_on(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "invalid data");
	Passwd_Popup_Data *pdata = (Passwd_Popup_Data *)data;
	int rot = elm_win_rotation_get(pdata->win_main);
	if (rot == 90 || rot == 270) {
		_passwd_popup_set_title(pdata->popup, true);
	}
	pdata->is_sip_on = true;
}

static void _passwd_rotation_changed(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(!data, "invalid data");
	Passwd_Popup_Data *pdata = (Passwd_Popup_Data *)data;
	int rot = elm_win_rotation_get(pdata->win_main);
	debug_log("rot:%d, is_sip_on:%d", rot, pdata->is_sip_on);
	if (rot == 90 || rot == 270) {
		_passwd_popup_set_title(pdata->popup, pdata->is_sip_on);
	} else {
		_passwd_popup_set_title(pdata->popup, false);
	}
}

static void _password_popup_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	debug_enter();
	if (!data) {
		debug_warning("data is NULL");
		return;
	}

	Passwd_Popup_Data *pdata = (Passwd_Popup_Data *)data;
	if (pdata->timer) {
		ecore_timer_del(pdata->timer);
	}

	if (pdata->conformant) {
		evas_object_smart_callback_del_full(pdata->conformant,
				"virtualkeypad,state,on", _passwd_sip_on, pdata);
		evas_object_smart_callback_del_full(pdata->conformant,
				"virtualkeypad,state,off", _passwd_sip_off, pdata);
	}
	if (pdata->win_main) {
		evas_object_smart_callback_del_full(pdata->win_main,
				"wm,rotation,changed", _passwd_rotation_changed, pdata);
	}

	free(pdata);
}

EMAIL_API Evas_Object *email_util_get_conformant(Evas_Object *obj)
{
	Evas_Object *o = obj;
	while (o) {
		const char *type = elm_object_widget_type_get(o);
		if (type && strcmp(type, "Elm_Conformant") == 0) {
			break;
		}
		o = elm_object_parent_widget_get(o);
	}
	return o;
}

EMAIL_API Evas_Object *email_util_create_password_changed_popup(
		Evas_Object *parent, int account_id, Evas_Smart_Cb done_cb,
		Evas_Smart_Cb cancel_cb, void *data)
{
	debug_enter();

	Evas_Object *layout = NULL;
	Evas_Object *btn = NULL;
	EmailAccountInfo *account_info = NULL;
	Passwd_Popup_Data *pdata = (Passwd_Popup_Data *)calloc(1, sizeof(Passwd_Popup_Data));
	retvm_if(!pdata, NULL, "calloc fail");

	pdata->win_main = elm_object_top_widget_get(parent);
	pdata->conformant = email_util_get_conformant(parent);

	if (pdata->conformant) {
		evas_object_smart_callback_add(pdata->conformant,
				"virtualkeypad,state,on", _passwd_sip_on, pdata);
		evas_object_smart_callback_add(pdata->conformant,
				"virtualkeypad,state,off", _passwd_sip_off, pdata);
	}
	if (pdata->win_main) {
		evas_object_smart_callback_add(pdata->win_main,
				"wm,rotation,changed", _passwd_rotation_changed, pdata);
	}
	pdata->done_cb = done_cb;
	pdata->cancel_cb = cancel_cb;
	pdata->data = data;
	pdata->account_id = account_id;

	pdata->popup = elm_popup_add(parent);
	evas_object_size_hint_weight_set(pdata->popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_popup_align_set(pdata->popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
	_passwd_popup_set_title(pdata->popup, false);

	layout = elm_layout_add(pdata->popup);
	elm_layout_file_set(layout, email_get_common_theme_path(), "popup_description_with_entry_style");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(layout);

	debug_log("account_id %d", account_id);
	if (email_engine_get_account_info(account_id, &account_info)) {
		if (account_info && account_info->email_address) {
			elm_object_part_text_set(layout, "elm.text", account_info->email_address);
		}
		if (account_info)
			email_engine_free_account_info(&account_info);
	}
	email_common_util_editfield_create(pdata->popup, EF_PASSWORD, &pdata->editfield);
	elm_entry_input_panel_return_key_disabled_set(pdata->editfield.entry, EINA_TRUE);
	elm_object_domain_translatable_part_text_set(pdata->editfield.entry, "elm.guide", PACKAGE, "IDS_ST_TMBODY_PASSWORD");
	evas_object_smart_callback_add(pdata->editfield.entry, "changed", _password_entry_changed_cb, pdata);
	evas_object_smart_callback_add(pdata->editfield.entry, "preedit,changed", _password_entry_changed_cb, pdata);
	evas_object_smart_callback_add(pdata->editfield.entry, "activated", _password_popup_ok_cb, pdata);
	eext_object_event_callback_add(pdata->popup, EEXT_CALLBACK_BACK, _password_popup_cancel_cb, pdata);
	evas_object_event_callback_add(pdata->popup, EVAS_CALLBACK_DEL, _password_popup_del_cb, pdata);
	evas_object_show(pdata->editfield.layout);
	elm_object_part_content_set(layout, "elm.swallow.entry", pdata->editfield.layout);
	elm_object_content_set(pdata->popup, layout);

	btn = elm_button_add(pdata->popup);
	elm_object_style_set(btn, "popup");
	elm_object_domain_translatable_text_set(btn, PACKAGE, "IDS_EMAIL_BUTTON_CANCEL");
	elm_object_part_content_set(pdata->popup, "button1", btn);
	evas_object_smart_callback_add(btn, "clicked", _password_popup_cancel_cb, pdata);

	btn = elm_button_add(pdata->popup);
	elm_object_style_set(btn, "popup");
	elm_object_domain_translatable_text_set(btn, PACKAGE, "IDS_EMAIL_BUTTON_DONE");
	elm_object_part_content_set(pdata->popup, "button2", btn);
	evas_object_smart_callback_add(btn, "clicked", _password_popup_ok_cb, pdata);
	elm_object_disabled_set(btn, EINA_TRUE);

	pdata->timer = ecore_timer_add(0.1, _password_popup_start_focus_cb, pdata);

	evas_object_show(pdata->popup);
	return pdata->popup;
}

/*
 * @return	login failure string. free after use
 */
EMAIL_API char *email_util_get_login_failure_string(int account_id)
{
	char buf[EMAIL_BUFF_SIZE_HUG] = {'\0'};
	email_account_t *account = NULL;
	int ret = email_get_account(account_id, EMAIL_ACC_GET_OPT_ACCOUNT_NAME, &account);
	if (ret == EMAIL_ERROR_NONE && account) {
		const char *fmt = _("IDS_EMAIL_TPOP_FAILED_TO_SIGN_IN_TO_PS");
		const char *name = account->account_name;
		if (!name) name = account->user_email_address;
		if (name) snprintf(buf, sizeof(buf), fmt, name);
	}
	if (buf[0] == '\0') {
		snprintf(buf, sizeof(buf), _("IDS_EMAIL_POP_AUTHENTICATION_FAILED"));
	}
	if (account) email_free_account(&account, 1);
	return strdup(buf);
}

EMAIL_API Eina_Bool email_file_cp(const char *source, const char *destination)
{
	int input = 0, output = 0;
	if ((input = open(source, O_RDONLY)) == -1) {
		return EINA_FALSE;
	}
	if ((output = open(destination, O_WRONLY | O_CREAT, 0777)) == -1) {
		close(input);
		return EINA_FALSE;
	}

	off_t copied = 0;
	struct stat fileinfo = {0};
	fstat(input, &fileinfo);
	int result = sendfile(output, input, &copied, fileinfo.st_size);

	close(input);
	close(output);

	if (-1 != result) {
		return EINA_TRUE;
	}

	return EINA_FALSE;
}

EMAIL_API char *email_file_dir_get(const char *path)
{
	return dirname(strdup(path));
}

EMAIL_API Eina_Bool email_file_exists(const char *file)
{
	struct stat sts = { 0 };
	if (stat(file, &sts) == -1 && errno == ENOENT) {
		return EINA_FALSE;
	}
	return EINA_TRUE;
}

EMAIL_API const char *email_file_file_get(const char *path)
{
	if (NULL == path) {
		return NULL;
	}
	char *file = NULL;
	file = strrchr(path, '/');
	if (NULL != file) {
		file++;
	} else {
		file = (char *)path;
	}
	return file;
}

EMAIL_API Eina_Bool email_file_is_dir(const char *file)
{
	struct stat sts = { 0 };
	if (stat(file, &sts) == -1 && errno == ENOENT) {
		return EINA_FALSE;
	}
	return S_ISDIR(sts.st_mode);
}

EMAIL_API Eina_Bool email_file_mv(const char *src, const char *dst)
{
	int result = 0;
	if (email_file_cp(src, dst) == EINA_FALSE) {
		result = unlink(dst);
		return EINA_FALSE;
	}

	result = unlink(src);
	if (0 != result) {
		return EINA_FALSE;
	}
	return EINA_TRUE;
}

EMAIL_API Eina_Bool email_file_remove(const char *file)
{
	int result = 0;
	result = remove(file);
	if (0 != result) {
		return EINA_FALSE;
	}
	return EINA_TRUE;
}

EMAIL_API Eina_Bool email_file_unlink(const char *file)
{
	int result = 0;
	result = unlink(file);
	if (0 != result) {
		return EINA_FALSE;
	}
	return EINA_TRUE;
}

EMAIL_API char *email_file_strip_ext(const char *file)
{
	char dot = '.';
	char slash = '/';
	char *dot_found_pos = strrchr(file, dot);
	char *slash_found_pos = strrchr(file, slash);
	if (NULL == dot_found_pos) {
		return NULL;
	}
	if (NULL != slash_found_pos && slash_found_pos > dot_found_pos) {
		return NULL;
	}

	int result_length = dot_found_pos - file + 1;
	char *file_dup = (char *)malloc(result_length * sizeof(char));
	if (NULL == file_dup) {
		return NULL;
	}
	strncpy(file_dup, file, result_length);
	file_dup[result_length - 1] = '\0';
	return file_dup;
}

EMAIL_API Eina_Bool email_file_recursive_rm(const char *dir)
{
	if (EINA_FALSE == email_file_is_dir(dir)) {
		return EINA_FALSE;
	}

	DIR *dir_handle = opendir(dir);
	if (NULL == dir_handle) {
		return EINA_FALSE;
	}
	struct dirent *dir_entry = NULL;
	Eina_Bool result = EINA_TRUE;
	do {
		dir_entry = readdir(dir_handle);

		if (NULL == dir_entry) {
			break;
		}

		if (0 == strcmp(dir_entry->d_name, ".") || 0 == strcmp(dir_entry->d_name, "..")) {
			continue;
		}

		char file_path[MAX_PATH_LEN] = {'\0'};
		sprintf(file_path, "%s/%s", dir, dir_entry->d_name);

		if (EINA_FALSE == email_file_is_dir(file_path)) {
			if (EINA_FALSE == email_file_unlink(file_path)) {
				result = EINA_FALSE;
				break;
			}
			continue;
		}

		if (EINA_FALSE == email_file_recursive_rm(file_path)) {
			result = EINA_FALSE;
			break;
		}

	} while (true);
	closedir(dir_handle);

	rmdir(dir);

	return result;
}

int email_update_vip_rule_value()
{
	debug_enter();

	time_t tm;
	time(&tm);
	int pref_ret = preference_set_double(EMAIL_PREFERENCE_KEY_VIP_RULE_CHANGED, tm);
	if (pref_ret != PREFERENCE_ERROR_NONE) {
		debug_warning("preference_set_double failed. err = %d", pref_ret);
		return -1;
	}
	return 0;
}

int email_get_vip_rule_value(double *value)
{
	debug_enter();

	int pref_ret = preference_get_double(EMAIL_PREFERENCE_KEY_VIP_RULE_CHANGED, value);
	if (pref_ret != PREFERENCE_ERROR_NONE) {
		debug_error("preference_get_double failed. err = %d", pref_ret);
		return -1;
	}
	return 0;
}

int email_set_vip_rule_change_callback(viprule_changed_cb callback, void *user_data)
{
	debug_enter();

	int pref_ret = preference_set_changed_cb(EMAIL_PREFERENCE_KEY_VIP_RULE_CHANGED, callback, user_data);
	if (pref_ret != PREFERENCE_ERROR_NONE) {
		debug_error("preference_set_changed_cb failed. err = %d", pref_ret);
		return -1;
	}
	return 0;
}

int email_unset_vip_rule_change_callback()
{
	debug_enter();

	int pref_ret = preference_unset_changed_cb(EMAIL_PREFERENCE_KEY_VIP_RULE_CHANGED);
	if (pref_ret != PREFERENCE_ERROR_NONE) {
		debug_error("preference_unset_changed_cb failed. err = %d", pref_ret);
		return -1;
	}
	return 0;
}

EMAIL_API int email_set_richtext_toolbar_status(Eina_Bool enable)
{
	debug_enter();

	int pref_ret = preference_set_boolean(EMAIL_PREFERENCE_KEY_RICHTEXT_TOOLBAR_ENABLED, enable == EINA_TRUE);
	if (pref_ret != PREFERENCE_ERROR_NONE) {
		debug_warning("preference_set_boolean failed. err = %d", pref_ret);
		return -1;
	}
	return 0;
}

EMAIL_API int email_get_richtext_toolbar_status(Eina_Bool *enabled)
{
	debug_enter();
	bool value;
	int pref_ret = preference_get_boolean(EMAIL_PREFERENCE_KEY_RICHTEXT_TOOLBAR_ENABLED, &value);
	if (pref_ret != PREFERENCE_ERROR_NONE) {
		debug_error("preference_get_double failed. err = %d", pref_ret);
		return -1;
	}
	*enabled = value == true;
	return 0;
}

Eina_Bool email_util_get_rgb_string(char *buf, int size, unsigned char r, unsigned char g, unsigned char b)
{
	retvm_if(!buf, EINA_FALSE, "buf is NULL!");
	retvm_if(size < 7, EINA_FALSE, "buf size is too small! size:[%d]", size);

	snprintf(buf, size, "%02x%02x%02x", r, g, b);
	return EINA_TRUE;
}

EMAIL_API void email_set_need_restart_flag(bool value)
{
	debug_enter();
	debug_log("value: %s", value ? "true" : "false");

	s_info.need_restart_flag = value;
}

EMAIL_API bool email_get_need_restart_flag()
{
	debug_enter();
	debug_log("s_info.need_restart_flag: %s", s_info.need_restart_flag ? "true" : "false");

	return s_info.need_restart_flag;
}

EMAIL_API bool email_params_create(app_control_h *params)
{
	retvm_if(!params, false, "params is NULL");
	retvm_if(*params, false, "*params is not NULL");

	int ret = APP_CONTROL_ERROR_NONE;

	ret = app_control_create(params);
	if (ret != APP_CONTROL_ERROR_NONE) {
		debug_error("app_control_create() failed! ret = %d", ret);
		*params = NULL;
		return false;
	}

	return true;
}

EMAIL_API void email_params_free(app_control_h *params)
{
	retm_if(!params, "params is NULL");

	if (*params) {
		int ret = APP_CONTROL_ERROR_NONE;

		ret = app_control_destroy(*params);
		warn_if(ret != APP_CONTROL_ERROR_NONE, "app_control_destroy() failed! ret = %d", ret);

		*params = NULL;
	}
}

EMAIL_API bool email_params_operation_set(app_control_h params, const char *operation)
{
	retvm_if(!params, false, "params is NULL");
	retvm_if(!operation, false, "operation is NULL");

	int ret = APP_CONTROL_ERROR_NONE;

	ret = app_control_set_operation(params, operation);
	retvm_if(ret != APP_CONTROL_ERROR_NONE, false, "app_control_set_operation failed: %d", ret);

	return true;
}

EMAIL_API bool email_params_uri_set(app_control_h params, const char *uri)
{
	retvm_if(!params, false, "params is NULL");
	retvm_if(!uri, false, "operation is NULL");

	int ret = APP_CONTROL_ERROR_NONE;

	ret = app_control_set_uri(params, uri);
	retvm_if(ret != APP_CONTROL_ERROR_NONE, false, "app_control_set_uri() failed: %d", ret);

	return true;
}

EMAIL_API bool email_params_add_int(app_control_h params, const char *key, int value)
{
	retvm_if(!params, false, "params is NULL");
	retvm_if(!key, false, "key is NULL");

	char s_value[EMAIL_BUFF_SIZE_SML] = {'\0'};
	int ret = APP_CONTROL_ERROR_NONE;

	snprintf(s_value, sizeof(s_value), "%d", value);

	ret = app_control_add_extra_data(params, key, s_value);
	retvm_if(ret != APP_CONTROL_ERROR_NONE, false, "app_control_add_extra_data() failed! ret = %d", ret);

	return true;
}

EMAIL_API bool email_params_add_str(app_control_h params, const char *key, const char *value)
{
	retvm_if(!params, false, "params is NULL");
	retvm_if(!key, false, "key is NULL");
	retvm_if(!value, false, "value is NULL");

	int ret = APP_CONTROL_ERROR_NONE;

	ret = app_control_add_extra_data(params, key, value);
	retvm_if(ret != APP_CONTROL_ERROR_NONE, false, "app_control_add_extra_data() failed! ret = %d", ret);

	return true;
}

static bool _email_params_get_value_impl(app_control_h params, const char *key, char **result, bool opt)
{
	retvm_if(!params, false, "params is NULL");
	retvm_if(!key, false, "key is NULL");
	retvm_if(!result, false, "result is NULL");
	retvm_if(*result, false, "*result is not NULL");

	int ret = APP_CONTROL_ERROR_NONE;

	ret = app_control_get_extra_data(params, key, result);
	if (ret != APP_CONTROL_ERROR_NONE) {
		if (opt && (ret == APP_CONTROL_ERROR_KEY_NOT_FOUND)) {
			*result = NULL;
			return true;
		}
		debug_error("app_control_get_extra_data() failed! ret = %d", ret);
		return false;
	}
	retvm_if(!result, false, "buff is NULL");

	return true;
}

static bool _email_params_get_int_impl(app_control_h params, const char *key, int *result, bool opt)
{
	char *buff = NULL;

	if (!_email_params_get_value_impl(params, key, &buff, opt)) {
		return false;
	}

	if (buff) {
		*result = atoi(buff);
		free(buff);
	}

	return true;
}

EMAIL_API bool email_params_get_int(app_control_h params, const char *key, int *result)
{
	return _email_params_get_int_impl(params, key, result, false);
}

EMAIL_API bool email_params_get_int_opt(app_control_h params, const char *key, int *result)
{
	return _email_params_get_int_impl(params, key, result, true);
}

static bool _email_params_get_str_impl(app_control_h params, const char *key, char *result, int result_size, bool opt)
{
	char *buff = NULL;
	int n = 0;

	if (!_email_params_get_value_impl(params, key, &buff, opt)) {
		return false;
	}

	if (buff) {
		n = snprintf(result, result_size, "%s", buff);
		free(buff);
		retvm_if(n >= result_size, false, "Result_size is too small. needed: %d bytes", n + 1);
	}

	return true;
}

EMAIL_API bool email_params_get_str(app_control_h params, const char *key, char *result, int result_size)
{
	return _email_params_get_str_impl(params, key, result, result_size, false);
}

EMAIL_API bool email_params_get_str_opt(app_control_h params, const char *key, char *result, int result_size)
{
	return _email_params_get_str_impl(params, key, result, result_size, true);
}

EMAIL_API int email_preview_attachment_file(email_module_t *module, const char *path, email_launched_app_listener_t *listener)
{
	retvm_if(!path, -1, "path is NULL");
	retvm_if(!module, -1, "module is NULL");

	int ret = 0;
	app_control_h params = NULL;

	if (email_params_create(&params) &&
		email_params_operation_set(params, APP_CONTROL_OPERATION_VIEW) &&
		email_params_uri_set(params, path) &&
		email_params_add_str(params,
				EMAIL_PREVIEW_VIEW_MODE_PARAM_NAME,
				EMAIL_PREVIEW_VIEW_MODE_PARAM_STR_VALUE) &&
		email_params_add_str(params,
				EMAIL_PREVIEW_MENU_STATE_PARAM_NAME,
				EMAIL_PREVIEW_MENU_STATE_PARAM_STR_VALUE)) {
		if ((email_module_launch_app(module, EMAIL_LAUNCH_APP_AUTO, params, listener) != 0)) {
			debug_log("email_module_launch_app() failed");
			ret = -1;
		}
	}

	email_params_free(&params);

	return ret;
}


EMAIL_API EmailExtSaveErrType email_prepare_temp_file_path(const int index, const char *tmp_root_dir, const char *src_file_path, char **dst_file_path)
{
	debug_enter();
	EmailExtSaveErrType ret_val = EMAIL_EXT_SAVE_ERR_NONE;
	bool skip_check_exist = false;

	if (!*dst_file_path) {
		char preview_path[MAX_PATH_LEN] = {'\0'};
		const char *file_name = email_file_file_get(src_file_path);
		if (!file_name) {
			debug_error("email_file_file_get() failed!");
			return EMAIL_EXT_SAVE_ERR_UNKNOWN;
		}

		snprintf(preview_path, sizeof(preview_path), "%s/%d_%s",
				tmp_root_dir, index, file_name);
		*dst_file_path = strdup(preview_path);
		if (!*dst_file_path) {
			debug_error("strdup() failed!");
			return EMAIL_EXT_SAVE_ERR_UNKNOWN;
		}
		skip_check_exist = true;
	}

	debug_secure("preview_path: %s", *dst_file_path);
	if (!skip_check_exist) {
		ret_val = (email_file_exists(*dst_file_path) ? EMAIL_EXT_SAVE_ERR_ALREADY_EXIST : EMAIL_EXT_SAVE_ERR_NONE);
	}

	debug_log("return value = %d", ret_val);
	return ret_val;
}

/* EOF */
