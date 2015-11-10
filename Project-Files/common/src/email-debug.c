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

#define _SYSTEM_DEBUG_		1
#define _USE_TIME_ELAPSED_	0

#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>

#if _SYSTEM_DEBUG_ == 1
#include <dlog.h>
#endif	/* _SYSTEM_DEBUG_ */

#include "email-debug.h"

#define FG_RED		31
#define FG_GREEN	32
#define FG_YELLOW	33
#define FG_MAGENTA	35
#define FG_CYAN		36
#define BG_BLACK	40

#define email_debug_print(msg) \
do { \
	printf("\x1b[%dm\x1b[%dm%s", BG_BLACK, FG_GREEN, msg); \
	printf("\x1b[0m\n"); \
} while (0)

#define email_assert_print(msg) \
do { \
	printf("\x1b[%dm\x1b[%dm%s", BG_BLACK, FG_MAGENTA, msg); \
	printf("\x1b[0m\n"); \
} while (0)

#define email_time_print(msg) \
do { \
	printf("\x1b[%dm\x1b[%dm%s", BG_BLACK, FG_CYAN, msg); \
	printf("\x1b[0m\n"); \
} while (0)

#define email_warning_print(msg) \
do { \
	printf("\x1b[%dm\x1b[%dm%s", BG_BLACK, FG_YELLOW, msg); \
	printf("\x1b[0m\n"); \
} while (0)

#define email_critical_print(msg) \
do { \
	printf("\x1b[%dm\x1b[%dm%s", BG_BLACK, FG_RED, msg); \
	printf("\x1b[0m\n"); \
} while (0)

#if _USE_TIME_ELAPSED_ == 1
static double _g_start_time = 0;
static double _g_old_time = 0;
#endif	/* _USE_TIME_ELAPSED_ */

#if _SYSTEM_DEBUG_ == 1
static void _debug_A(EmailDebugType type, const gchar *message)
{
#define MID_EMAIL	(MID_MESSAGING + SET_IDENT(0))

	switch (type) {
	case EMAIL_DEBUG_CRITICAL:
		LOGW("%s", message);
		break;
	case EMAIL_DEBUG_WARNING:
		LOGI("%s", message);
		break;
	case EMAIL_DEBUG_EXPR:
	case EMAIL_DEBUG_TIME:
	case EMAIL_DEBUG_LOG:
		LOGD("%s", message);
		break;
	default:
		break;
	}

}
#else	/* _SYSTEM_DEBUG_ */
static void _debug_B(EmailDebugType type, const gchar *message)
{
	switch (type) {
	case EMAIL_DEBUG_CRITICAL:
		email_critical_print(message);
		break;
	case EMAIL_DEBUG_WARNING:
		email_warning_print(message);
		break;
	case EMAIL_DEBUG_EXPR:
		email_assert_print(message);
		break;
	case EMAIL_DEBUG_TIME:
		email_time_print(message);
		break;
	case EMAIL_DEBUG_LOG:
		email_debug_print(message);
		break;
	default:
		break;
	}

}
#endif	/* _SYSTEM_DEBUG_ */

EMAIL_API void email_debug_message(EmailDebugType type, const gchar *file_name, const gchar *func, gint line_number, const gchar *format, ...)
{
	va_list args;
	gchar msg_buf[1024] = "\0", buffer[2048] = "\0";
	const gchar *dbg_prefix = NULL;

	switch (type) {
	case EMAIL_DEBUG_CRITICAL:
		dbg_prefix = " * Critical * ";
		break;
	case EMAIL_DEBUG_WARNING:
		dbg_prefix = " * Warning * ";
		break;
	case EMAIL_DEBUG_EXPR:
	case EMAIL_DEBUG_TIME:
	case EMAIL_DEBUG_LOG:
		dbg_prefix = " ";
		break;
	default:
		break;
	}

	va_start(args, format);
	g_vsnprintf(msg_buf, sizeof(msg_buf), format, args);
#if _SYSTEM_DEBUG_ == 1
	g_snprintf(buffer, sizeof(buffer), "[%s:%s:#%d]%s%s\n", file_name, func, line_number, dbg_prefix, msg_buf);
#else	/* _SYSTEM_DEBUG_ */
	g_snprintf(buffer, sizeof(buffer), "[%s:%s:#%d]%s%s", file_name, func, line_number, dbg_prefix, msg_buf);
#endif	/* _SYSTEM_DEBUG_ */
	va_end(args);

#if _SYSTEM_DEBUG_ == 1
	_debug_A(type, buffer);
#else	/* _SYSTEM_DEBUG_ */
	_debug_B(type, buffer);
#endif	/* _SYSTEM_DEBUG_ */
}

EMAIL_API void email_time_elapsed_message(const gchar *file_name, const gchar *func, gint line_number)
{
#if _USE_TIME_ELAPSED_ == 1
	gchar message[512] = "\0";
	struct timeval tv;
	double new_time = 0;
	double elapsed = 0;
	double interval = 0;

	memset(&tv, 0, sizeof(struct timeval));

	gettimeofday(&tv, NULL);

	new_time = (double)(tv.tv_sec * 1000000 + tv.tv_usec);

	if (_g_start_time == 0) {
		_g_start_time = new_time;
	}

	if (_g_old_time == 0) {
		_g_old_time = new_time;
	}

	elapsed = (double)(new_time - _g_start_time) / CLOCKS_PER_SEC;
	interval = (double)(new_time - _g_old_time) / CLOCKS_PER_SEC;

	if (_g_old_time != new_time) {
		_g_old_time = new_time;
	}

	g_snprintf(message, sizeof(message), "elapsed (%lf) interval (%lf)", elapsed, interval);

	email_debug_message(EMAIL_DEBUG_TIME, file_name, func, line_number, message);
#endif	/* _USE_TIME_ELAPSED_ */
}

EMAIL_API void email_time_elapsed_reset(void)
{
	debug_enter();

#if _USE_TIME_ELAPSED_ == 1
	_g_start_time = 0;
	_g_old_time = 0;
#endif	/* _USE_TIME_ELAPSED_ */
}

/* EOF */
