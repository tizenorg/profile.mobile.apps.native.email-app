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

#ifndef _EMAIL_DEBUG_H_
#define _EMAIL_DEBUG_H_

#include <string.h>
#include <glib.h>
#include <glib-object.h>

#ifndef LOG_TAG
#define LOG_TAG "EMAIL_COMM"
#endif

#ifdef _DEBUG
#ifndef TIZEN_DEBUG_ENABLE
#define TIZEN_DEBUG_ENABLE
#endif
#endif

#include <dlog.h>
#include "email-common-types.h"

G_BEGIN_DECLS

#undef debug_trace
#undef debug_log
#undef debug_warning
#undef debug_error
#undef debug_critical
#undef debug_fatal

#undef debug_trace_if
#undef debug_log_if
#undef debug_warning_if
#undef debug_error_if
#undef debug_fatal_if

#undef debug_secure_trace
#undef debug_secure
#undef debug_secure_warning
#undef debug_secure_error
#undef debug_secure_fatal

#undef debug_secure_trace_if
#undef debug_secure_log_if
#undef debug_secure_warning_if
#undef debug_secure_error_if
#undef debug_secure_fatal_if

#undef debug_enter
#undef debug_leave

#ifdef _DEBUG

// LOG Level: V < D < I < W < E < F
#define debug_trace(fmt, args...)		do { LOGD(fmt, ##args); } while (0)
#define debug_log(fmt, args...)			do { LOGI(fmt, ##args); } while (0)
#define debug_warning(fmt, args...)		do { LOGW(fmt, ##args); } while (0)
#define debug_error(fmt, args...)		do { LOGE(fmt, ##args); } while (0)
#define debug_critical(fmt, args...)		do { LOGE(fmt, ##args); } while (0)
#define debug_fatal(fmt, args...)		do { LOGF(fmt, ##args); } while (0)

#define COND(expr)	(__builtin_expect((expr) != 0, 0))

#define debug_trace_if(expr, fmt, args...)		do { if (COND(expr)) LOGD(fmt, ##args); } while (0)
#define debug_log_if(expr, fmt, args...)		do { if (COND(expr)) LOGI(fmt, ##args); } while (0)
#define debug_warning_if(expr, fmt, args...)	do { if (COND(expr)) LOGW(fmt, ##args); } while (0)
#define debug_error_if(expr, fmt, args...)		do { if (COND(expr)) LOGE(fmt, ##args); } while (0)
#define debug_fatal_if(expr, fmt, args...)		do { if (COND(expr)) LOGF(fmt, ##args); } while (0)

#ifdef TIZEN_DEBUG_ENABLE
#define debug_secure_trace(fmt, args...)		do { SECURE_LOGD(fmt, ##args); } while (0)
#define debug_secure(fmt, args...)		do { SECURE_LOGI(fmt, ##args); } while (0)
#define debug_secure_warning(fmt, args...)		do { SECURE_LOGW(fmt, ##args); } while (0)
#define debug_secure_error(fmt, args...)		do { SECURE_LOGE(fmt, ##args); } while (0)
#define debug_secure_fatal(fmt, args...)		do { SECURE_LOGF(fmt, ##args); } while (0)

#define debug_secure_trace_if(expr, fmt, args...)		do { if (COND(expr)) SECURE_LOGD(fmt, ##args); } while (0)
#define debug_secure_log_if(expr, fmt, args...)		do { if (COND(expr)) SECURE_LOGI(fmt, ##args); } while (0)
#define debug_secure_warning_if(expr, fmt, args...)	do { if (COND(expr)) SECURE_LOGW(fmt, ##args); } while (0)
#define debug_secure_error_if(expr, fmt, args...)		do { if (COND(expr)) SECURE_LOGE(fmt, ##args); } while (0)
#define debug_secure_fatal_if(expr, fmt, args...)		do { if (COND(expr)) SECURE_LOGF(fmt, ##args); } while (0)
#else
#define debug_secure_trace(fmt, args...)
#define debug_secure(fmt, args...)
#define debug_secure_warning(fmt, args...)
#define debug_secure_error(fmt, args...)
#define debug_secure_fatal(fmt, args...)

#define debug_secure_trace_if(expr, fmt, args...)
#define debug_secure_log_if(expr, fmt, args...)
#define debug_secure_warning_if(expr, fmt, args...)
#define debug_secure_error_if(expr, fmt, args...)
#define debug_secure_fatal_if(expr, fmt, args...)
#endif

#define debug_enter()					do { debug_trace(" * Enter *"); } while (0)
#define debug_leave()					do { debug_trace(" * Leave *"); } while (0)

#else	/* _DEBUG */

#define debug_trace(fmt, args...)
#define debug_log(fmt, args...)
#define debug_warning(fmt, args...)
#define debug_error(fmt, args...)
#define debug_critical(fmt, args...)
#define debug_fatal(fmt, args...)

#define debug_trace_if(fmt, args...)
#define debug_log_if(fmt, args...)
#define debug_warning_if(fmt, args...)
#define debug_error_if(fmt, args...)
#define debug_fatal_if(fmt, args...)

#define debug_secure_trace(fmt, args...)
#define debug_secure(fmt, args...)
#define debug_secure_warning(fmt, args...)
#define debug_secure_error(fmt, args...)
#define debug_secure_fatal(fmt, args...)

#define debug_secure_trace_if(fmt, args...)
#define debug_secure_log_if(fmt, args...)
#define debug_secure_warning_if(fmt, args...)
#define debug_secure_error_if(fmt, args...)
#define debug_secure_fatal_if(fmt, args...)

#define debug_enter()
#define debug_leave()

#endif	/* _DEBUG */

#define RETURN_IF_FAIL(expr) \
	do { \
		if (!(expr)) { \
			debug_error("expr : (%s) failed", #expr); \
			return; \
		} \
	} while (0)

#define RETURN_VAL_IF_FAIL(expr, val) \
	do { \
		if (!(expr)) { \
			debug_error("expr : (%s) failed", #expr); \
			return (val); \
		} \
	} while (0)

#define ret_if(expr) \
	do { \
		if (expr) { \
			return; \
		} \
	} while (0)

#define retv_if(expr, val) \
	do { \
		if (expr) { \
			return (val); \
		} \
	} while (0)

#define retm_if(expr, fmt, arg...) \
	do { \
		if (expr) { \
			debug_error(fmt, ##arg); \
			return; \
		} \
	} while (0)

#define warn_if(expr, fmt, arg...) \
	do { \
		if (expr) { \
			debug_warning(fmt, ##arg); \
		} \
	} while (0)

#define gotom_if(expr, label, fmt, arg...) \
	do { \
		if (expr) { \
			debug_error(fmt, ##arg); \
			goto label; \
		} \
	} while (0)

#define retvm_if(expr, val, fmt, arg...) \
	do { \
		if (expr) { \
			debug_error(fmt, ##arg); \
			return (val); \
		} \
	} while (0)

typedef enum {
	EMAIL_DEBUG_CRITICAL = 0,
	EMAIL_DEBUG_WARNING,
	EMAIL_DEBUG_EXPR,
	EMAIL_DEBUG_TIME,
	EMAIL_DEBUG_LOG,
	EMAIL_DEBUG_MAX
} EmailDebugType;

G_END_DECLS
#endif	/* _EMAIL_DEBUG_H_ */

/* EOF */
