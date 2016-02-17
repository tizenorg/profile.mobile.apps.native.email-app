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

#include "email-params.h"

#include "email-debug.h"

#define EMAIL_PARAMS_OPERATION	"__EMAIL_PARAMS_OPERATION__"
#define EMAIL_PARAMS_URI		"__EMAIL_PARAMS_URI__"
#define EMAIL_PARAMS_MIME		"__EMAIL_PARAMS_MIME__"

static inline bool _email_params_get_str_impl(email_params_h params, const char *key, const char **result, bool opt);
static inline bool _email_params_get_int_impl(email_params_h params, const char *key, int *result, bool opt);

static bool _app_control_extra_data_cb(app_control_h app_control, const char *key, void *user_data);
static void _bundle_extra_data_cb(const char *key, const int type,
		const bundle_keyval_t *kv, void *user_data);

static inline bool _email_params_get_str_impl(email_params_h params, const char *key, const char **result, bool opt)
{
	retvm_if(!params, false, "params is NULL");
	retvm_if(!key, false, "key is NULL");
	retvm_if(!result, false, "result is NULL");

	char *str = NULL;

	int ret = bundle_get_str(params, key, &str);
	if (ret != BUNDLE_ERROR_NONE) {
		if (ret == BUNDLE_ERROR_KEY_NOT_AVAILABLE) {
			if (opt) {
				return true;
			}
			debug_log("key(%s) not available.", key);
		} else {
			debug_error("bundle_get_str(%s) failed! ret = %d", key, ret);
		}
		return false;
	}
	retvm_if(!str, false, "buff is NULL");

	*result = str;

	return true;
}

static inline bool _email_params_get_int_impl(email_params_h params, const char *key, int *result, bool opt)
{
	const char *buff = NULL;

	if (!_email_params_get_str_impl(params, key, &buff, opt)) {
		return false;
	}

	if (buff) {
		*result = atoi(buff);
	}

	return true;
}

EMAIL_API bool email_params_create(email_params_h *params)
{
	retvm_if(!params, false, "params is NULL");
	retvm_if(*params, false, "*params is not NULL");

	bundle *bundle = bundle_create();
	if (!bundle) {
		debug_error("bundle_create() failed!");
		return false;
	}

	*params = bundle;

	return true;
}

EMAIL_API void email_params_free(email_params_h *params)
{
	retm_if(!params, "params is NULL");

	if (*params) {
		int ret = bundle_free(*params);
		warn_if(ret != BUNDLE_ERROR_NONE, "bundle_free() failed! ret = %d", ret);

		*params = NULL;
	}
}

EMAIL_API bool email_params_clone(email_params_h *dst, email_params_h src)
{
	retvm_if(!dst, false, "dst is NULL");
	retvm_if(*dst, false, "*dst is not NULL");
	retvm_if(!src, false, "src is NULL");

	*dst = bundle_dup(src);

	return (*dst != NULL);
}

static bool _app_control_extra_data_cb(app_control_h app_control, const char *key, void *user_data)
{
	email_params_h *params = user_data;

	bool is_array = false;

	int ret = app_control_is_extra_data_array(app_control, key, &is_array);
	if (ret != APP_CONTROL_ERROR_NONE) {
		debug_error("app_control_is_extra_data_array() failed! ret = %d", ret);
		email_params_free(params);
		return false;
	}

	bool result = false;

	if (is_array) {
		int i = 0;
		char **array = NULL;
		int array_length = 0;

		ret = app_control_get_extra_data_array(app_control, key, &array, &array_length);
		if (ret != APP_CONTROL_ERROR_NONE) {
			debug_error("app_control_get_extra_data_array() failed! ret = %d", ret);
		} else {
			result = email_params_add_str_array(*params, key, (const char **)array, array_length);
		}

		for (i = 0; i < array_length; ++ array_length) {
			free(array[i]);
		}
		free(array);

	} else {
		char *str = NULL;

		ret = app_control_get_extra_data(app_control, key, &str);
		if (ret != APP_CONTROL_ERROR_NONE) {
			debug_error("app_control_get_extra_data() failed! ret = %d", ret);
		} else {
			result = email_params_add_str(*params, key, str);
		}

		free(str);
	}

	if (!result) {
		email_params_free(params);
	}

	return result;
}

EMAIL_API bool email_params_from_app_control(email_params_h *params, app_control_h app_control)
{
	retvm_if(!app_control, false, "app_control is NULL");

	char *operation = NULL;
	char *uri = NULL;
	char *mime = NULL;

	bool result = false;

	while (true) {
		int ret = APP_CONTROL_ERROR_NONE;

		ret = app_control_get_operation(app_control, &operation);
		if (ret != APP_CONTROL_ERROR_NONE) {
			debug_error("app_control_get_operation() failed! ret = %d", ret);
			break;
		}
		ret = app_control_get_uri(app_control, &uri);
		if (ret != APP_CONTROL_ERROR_NONE) {
			debug_error("app_control_get_uri() failed! ret = %d", ret);
			break;
		}
		ret = app_control_get_mime(app_control, &mime);
		if (ret != APP_CONTROL_ERROR_NONE) {
			debug_error("app_control_get_mime() failed! ret = %d", ret);
			break;
		}

		if (email_params_create(params) &&
			(!operation || email_params_set_operation(*params, operation)) &&
			(!uri || email_params_set_uri(*params, uri)) &&
			(!mime || email_params_set_mime(*params, mime))) {

			ret = app_control_foreach_extra_data(app_control, _app_control_extra_data_cb, params);
			if (ret != APP_CONTROL_ERROR_NONE) {
				debug_error("app_control_foreach_extra_data() failed! ret = %d", ret);
				break;
			}

			if (*params) {
				result = true;
			}
		}

		break;
	}

	if (!result) {
		email_params_free(params);
	}

	free(operation);
	free(uri);
	free(mime);

	return result;
}

static void _bundle_extra_data_cb(const char *key, const int type,
		const bundle_keyval_t *kv, void *user_data)
{
	app_control_h *app_control = user_data;
	if (!*app_control) {
		return;
	}

	bool ok = false;

	switch (type) {
	case BUNDLE_TYPE_STR:
	{
		const char *val = NULL;
		size_t size = 0;

		int ret = bundle_keyval_get_basic_val((bundle_keyval_t *)kv, (void **)&val, &size);
		if (ret != BUNDLE_ERROR_NONE) {
			debug_error("bundle_keyval_get_basic_val() failed! ret = %d", ret);
			break;
		}

		if (strcmp(key, EMAIL_PARAMS_OPERATION) == 0) {
			ret = app_control_set_operation(*app_control, val);
		} else if (strcmp(key, EMAIL_PARAMS_URI) == 0) {
			ret = app_control_set_uri(*app_control, val);
		} else if (strcmp(key, EMAIL_PARAMS_MIME) == 0) {
			ret = app_control_set_mime(*app_control, val);
		} else {
			ret = app_control_add_extra_data(*app_control, key, val);
		}

		if (ret != APP_CONTROL_ERROR_NONE) {
			debug_error("app_control_***() failed! ret = %d", ret);
			break;
		}

		ok = true;
		break;
	}
	case BUNDLE_TYPE_STR_ARRAY:
	{
		const char **array = NULL;
		unsigned int array_len = 0;
		size_t *array_element_size = 0;

		if ((strcmp(key, EMAIL_PARAMS_OPERATION) == 0) ||
			(strcmp(key, EMAIL_PARAMS_URI) == 0) ||
			(strcmp(key, EMAIL_PARAMS_MIME) == 0)) {
			debug_log("Key %s skipped.", key);
			ok = true;
			break;
		}

		int ret = bundle_keyval_get_array_val((bundle_keyval_t *)kv, (void ***)&array,
				&array_len, &array_element_size);
		if (ret != BUNDLE_ERROR_NONE) {
			debug_error("bundle_keyval_get_array_val() failed! ret = %d", ret);
			break;
		}

		ret = app_control_add_extra_data_array(*app_control, key, array, array_len);
		if (ret != APP_CONTROL_ERROR_NONE) {
			debug_error("app_control_add_extra_data_array() failed! ret = %d", ret);
			break;
		}

		ok = true;
		break;
	}
	default:
		debug_log("Key %s skipped.", key);
		ok = true;
		break;
	}

	if (!ok) {
		app_control_destroy(*app_control);
		*app_control = NULL;
	}
}

EMAIL_API bool email_params_to_app_control(email_params_h params, app_control_h *app_control)
{
	retvm_if(!params, false, "params is NULL");
	retvm_if(!app_control, false, "app_control is NULL");
	retvm_if(*app_control, false, "*app_control is not NULL");

	int ret = app_control_create(app_control);
	if (ret != APP_CONTROL_ERROR_NONE) {
		debug_error("app_control_create() failed! ret = %d", ret);
		return false;
	}

	bundle_foreach(params, _bundle_extra_data_cb, app_control);

	if (!*app_control) {
		return false;
	}

	return true;
}

EMAIL_API bool email_params_set_operation(email_params_h params, const char *operation)
{
	return email_params_add_str(params, EMAIL_PARAMS_OPERATION, operation);
}

EMAIL_API bool email_params_set_uri(email_params_h params, const char *uri)
{
	return email_params_add_str(params, EMAIL_PARAMS_URI, uri);
}

EMAIL_API bool email_params_set_mime(email_params_h params, const char *mime)
{
	return email_params_add_str(params, EMAIL_PARAMS_MIME, mime);
}

EMAIL_API bool email_params_get_operation(email_params_h params, const char **result)
{
	return _email_params_get_str_impl(params, EMAIL_PARAMS_OPERATION, result, false);
}

EMAIL_API bool email_params_get_operation_opt(email_params_h params, const char **result)
{
	return _email_params_get_str_impl(params, EMAIL_PARAMS_OPERATION, result, true);
}

EMAIL_API bool email_params_get_uri(email_params_h params, const char **result)
{
	return _email_params_get_str_impl(params, EMAIL_PARAMS_URI, result, false);
}

EMAIL_API bool email_params_get_uri_opt(email_params_h params, const char **result)
{
	return _email_params_get_str_impl(params, EMAIL_PARAMS_URI, result, true);
}

EMAIL_API bool email_params_get_mime(email_params_h params, const char **result)
{
	return _email_params_get_str_impl(params, EMAIL_PARAMS_MIME, result, false);
}

EMAIL_API bool email_params_get_mime_opt(email_params_h params, const char **result)
{
	return _email_params_get_str_impl(params, EMAIL_PARAMS_MIME, result, true);
}

EMAIL_API bool email_params_add_int(email_params_h params, const char *key, int value)
{
	retvm_if(!params, false, "params is NULL");
	retvm_if(!key, false, "key is NULL");

	char s_value[EMAIL_BUFF_SIZE_SML] = {'\0'};
	snprintf(s_value, sizeof(s_value), "%d", value);

	int ret = bundle_add_str(params, key, s_value);
	retvm_if(ret != BUNDLE_ERROR_NONE, false, "bundle_add_str() failed! ret = %d", ret);

	return true;
}

EMAIL_API bool email_params_add_str(email_params_h params, const char *key, const char *value)
{
	retvm_if(!params, false, "params is NULL");
	retvm_if(!key, false, "key is NULL");
	retvm_if(!value, false, "value is NULL");

	int ret = bundle_add_str(params, key, value);
	retvm_if(ret != BUNDLE_ERROR_NONE, false, "bundle_add_str() failed! ret = %d", ret);

	return true;
}

EMAIL_API bool email_params_add_str_array(email_params_h params, const char *key,
		const char **value, int array_length)
{
	retvm_if(!params, false, "params is NULL");
	retvm_if(!key, false, "key is NULL");
	retvm_if(!value, false, "value is NULL");
	retvm_if(array_length > 0, false, "array_length: %d", array_length);

	int ret = bundle_add_str_array(params, key, value, array_length);
	retvm_if(ret != BUNDLE_ERROR_NONE, false, "bundle_add_str_array() failed! ret = %d", ret);

	return true;
}

EMAIL_API bool email_params_get_is_array(email_params_h params, const char *key, bool *is_array)
{
	retvm_if(!params, false, "params is NULL");
	retvm_if(!key, false, "key is NULL");
	retvm_if(!is_array, false, "is_array is NULL");

	int type = bundle_get_type(params, key);

	if (type == BUNDLE_TYPE_STR_ARRAY) {
		*is_array = true;
		return true;
	}

	if (type != BUNDLE_TYPE_STR) {
		return false;
	}

	*is_array = false;

	return true;
}

EMAIL_API bool email_params_get_int(email_params_h params, const char *key, int *result)
{
	return _email_params_get_int_impl(params, key, result, false);
}

EMAIL_API bool email_params_get_int_opt(email_params_h params, const char *key, int *result)
{
	return _email_params_get_int_impl(params, key, result, true);
}

EMAIL_API bool email_params_get_str(email_params_h params, const char *key, const char **result)
{
	return _email_params_get_str_impl(params, key, result, false);
}

EMAIL_API bool email_params_get_str_opt(email_params_h params, const char *key, const char **result)
{
	return _email_params_get_str_impl(params, key, result, true);
}

EMAIL_API bool email_params_get_str_array(email_params_h params, const char *key,
		const char ***result, int *array_length)
{
	retvm_if(!params, false, "params is NULL");
	retvm_if(!key, false, "key is NULL");
	retvm_if(!result, false, "result is NULL");
	retvm_if(!array_length, false, "array_length is NULL");

	*result = bundle_get_str_array(params, key, array_length);
	if (!*result) {
		debug_error("bundle_get_str_array() failed!");
		return false;
	}

	return true;
}
