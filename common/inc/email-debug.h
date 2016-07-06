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

#if defined(_DEBUG)
#ifndef TIZEN_DEBUG_ENABLE
#define TIZEN_DEBUG_ENABLE
#endif
#endif

#if defined(_DEBUG) || defined(_RELEASE_DEBUG)
#ifndef EMAIL_DEBUG_ENABLE
#define EMAIL_DEBUG_ENABLE
#endif
#endif

#include <dlog.h>
#include "email-common-types.h"

G_BEGIN_DECLS

#undef _debug
#undef _debug_secure

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

#ifndef __MODULE__
#define __MODULE__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#define COND(expr)	(__builtin_expect((expr) != 0, 0))

#define _debug(prio, fmt, arg...) \
	dlog_print(prio, LOG_TAG, "%s: %s(%d) > " fmt, __MODULE__, __func__, __LINE__, ##arg)

#define _debug_secure(prio, fmt, arg...) \
	dlog_print(prio, LOG_TAG, "%s: %s(%d) > [SECURE_LOG] " fmt, __MODULE__, __func__, __LINE__, ##arg)

#define _debug_nop(fmt, args...) _debug_nop_impl("" fmt, ##args)

#define _debug_nop_expr(expr, fmt, args...) _debug_nop_expr_impl(expr, "" fmt, ##args)

static inline int _debug_nop_impl(const char *fmt __attribute__((unused)), ...) { return 0; }
static inline int _debug_nop_expr_impl(bool expr __attribute__((unused)),
		const char *fmt __attribute__((unused)), ...) { return 0; }

#define debug_warning(fmt, args...)		_debug(DLOG_WARN, fmt, ##args)
#define debug_error(fmt, args...)		_debug(DLOG_ERROR, fmt, ##args)
#define debug_fatal(fmt, args...)		_debug(DLOG_FATAL, fmt, ##args)
#define debug_critical(fmt, args...)	_debug(DLOG_ERROR, fmt, ##args)

#define debug_warning_if(expr, fmt, args...)	do { if (COND(expr)) debug_warning(fmt, ##args); } while (0)
#define debug_error_if(expr, fmt, args...)		do { if (COND(expr)) debug_error(fmt, ##args); } while (0)
#define debug_fatal_if(expr, fmt, args...)		do { if (COND(expr)) debug_fatal(fmt, ##args); } while (0)

#ifdef TIZEN_DEBUG_ENABLE
#define debug_secure_warning(fmt, args...)	_debug_secure(DLOG_WARN, fmt, ##args)
#define debug_secure_error(fmt, args...)	_debug_secure(DLOG_ERROR, fmt, ##args)
#define debug_secure_fatal(fmt, args...)	_debug_secure(DLOG_FATAL, fmt, ##args)

#define debug_secure_warning_if(expr, fmt, args...)	do { if (COND(expr)) debug_secure_warning(fmt, ##args); } while (0)
#define debug_secure_error_if(expr, fmt, args...)	do { if (COND(expr)) debug_secure_error(fmt, ##args); } while (0)
#define debug_secure_fatal_if(expr, fmt, args...)	do { if (COND(expr)) debug_secure_fatal(fmt, ##args); } while (0)
#else
#define debug_secure_warning(fmt, args...)	_debug_nop(fmt, ##args)
#define debug_secure_error(fmt, args...)	_debug_nop(fmt, ##args)
#define debug_secure_fatal(fmt, args...)	_debug_nop(fmt, ##args)

#define debug_secure_warning_if(expr, fmt, args...)	_debug_nop_expr(expr, fmt, ##args)
#define debug_secure_error_if(expr, fmt, args...)	_debug_nop_expr(expr, fmt, ##args)
#define debug_secure_fatal_if(expr, fmt, args...)	_debug_nop_expr(expr, fmt, ##args)
#endif /* TIZEN_DEBUG_ENABLE */

#ifdef EMAIL_DEBUG_ENABLE

#define debug_trace(fmt, args...)		_debug(DLOG_DEBUG, fmt, ##args)
#define debug_log(fmt, args...)			_debug(DLOG_INFO, fmt, ##args)

#define debug_trace_if(expr, fmt, args...)		do { if (COND(expr)) debug_trace(fmt, ##args); } while (0)
#define debug_log_if(expr, fmt, args...)		do { if (COND(expr)) debug_log(fmt, ##args); } while (0)

#ifdef TIZEN_DEBUG_ENABLE
#define debug_secure_trace(fmt, args...)	_debug_secure(DLOG_DEBUG, fmt, ##args)
#define debug_secure(fmt, args...)			_debug_secure(DLOG_INFO, fmt, ##args)

#define debug_secure_trace_if(expr, fmt, args...)	do { if (COND(expr)) debug_secure_trace(fmt, ##args); } while (0)
#define debug_secure_log_if(expr, fmt, args...)		do { if (COND(expr)) debug_secure(fmt, ##args); } while (0)
#else
#define debug_secure_trace(fmt, args...)	_debug_nop(fmt, ##args)
#define debug_secure(fmt, args...)			_debug_nop(fmt, ##args)

#define debug_secure_trace_if(expr, fmt, args...)	_debug_nop_expr(expr, fmt, ##args)
#define debug_secure_log_if(expr, fmt, args...)		_debug_nop_expr(expr, fmt, ##args)
#endif /* TIZEN_DEBUG_ENABLE */

#define debug_enter()	debug_trace(" * Enter *")
#define debug_leave()	debug_trace(" * Leave *")

#else	/* EMAIL_DEBUG_ENABLE */

#define debug_trace(fmt, args...)		_debug_nop(fmt, ##args)
#define debug_log(fmt, args...)			_debug_nop(fmt, ##args)

#define debug_trace_if(fmt, args...)	_debug_nop_expr(expr, fmt, ##args)
#define debug_log_if(fmt, args...)		_debug_nop_expr(expr, fmt, ##args)

#define debug_secure_trace(fmt, args...)	_debug_nop(fmt, ##args)
#define debug_secure(fmt, args...)			_debug_nop(fmt, ##args)

#define debug_secure_trace_if(fmt, args...)	_debug_nop_expr(expr, fmt, ##args)
#define debug_secure_log_if(fmt, args...)	_debug_nop_expr(expr, fmt, ##args)

#define debug_enter()	_debug_nop("")
#define debug_leave()	_debug_nop("")

#endif	/* EMAIL_DEBUG_ENABLE */

#define RETURN_IF_FAIL(expr) \
	do { \
		if (!COND(expr)) { \
			debug_error("expr : (%s) failed", #expr); \
			return; \
		} \
	} while (0)

#define RETURN_VAL_IF_FAIL(expr, val) \
	do { \
		if (!COND(expr)) { \
			debug_error("expr : (%s) failed", #expr); \
			return (val); \
		} \
	} while (0)

#define ret_if(expr) \
	do { \
		if (COND(expr)) { \
			return; \
		} \
	} while (0)

#define retv_if(expr, val) \
	do { \
		if (COND(expr)) { \
			return (val); \
		} \
	} while (0)

#define retm_if(expr, fmt, arg...) \
	do { \
		if (COND(expr)) { \
			debug_error(fmt, ##arg); \
			return; \
		} \
	} while (0)

#define retvm_if(expr, val, fmt, arg...) \
	do { \
		if (COND(expr)) { \
			debug_error(fmt, ##arg); \
			return (val); \
		} \
	} while (0)

#define gotom_if(expr, label, fmt, arg...) \
	do { \
		if (COND(expr)) { \
			debug_error(fmt, ##arg); \
			goto label; \
		} \
	} while (0)

#define break_if(expr, fmt, arg...) \
	do { \
		if (COND(expr)) { \
			debug_error(fmt, ##arg); \
			break; \
		} \
	} while (0)

G_END_DECLS
#endif	/* _EMAIL_DEBUG_H_ */

/* EOF */
