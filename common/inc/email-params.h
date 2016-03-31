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

#ifndef __EMAIL_PARAMS__
#define __EMAIL_PARAMS__

#include "email-common-types.h"

#include <bundle.h>
#include <app_control.h>

typedef bundle *email_params_h;

EMAIL_API bool email_params_create(email_params_h *params);
EMAIL_API void email_params_free(email_params_h *params);

EMAIL_API bool email_params_clone(email_params_h *dst, email_params_h src);
EMAIL_API bool email_params_from_app_control(email_params_h *params, app_control_h app_control);
EMAIL_API bool email_params_to_app_control(email_params_h params, app_control_h *app_control);

EMAIL_API bool email_params_set_operation(email_params_h params, const char *operation);
EMAIL_API bool email_params_set_uri(email_params_h params, const char *uri);
EMAIL_API bool email_params_set_mime(email_params_h params, const char *mime);

EMAIL_API bool email_params_get_operation(email_params_h params, const char **result);
EMAIL_API bool email_params_get_operation_opt(email_params_h params, const char **result);
EMAIL_API bool email_params_get_uri(email_params_h params, const char **result);
EMAIL_API bool email_params_get_uri_opt(email_params_h params, const char **result);
EMAIL_API bool email_params_get_mime(email_params_h params, const char **result);
EMAIL_API bool email_params_get_mime_opt(email_params_h params, const char **result);

EMAIL_API bool email_params_add_int(email_params_h params, const char *key, int value);
EMAIL_API bool email_params_add_str(email_params_h params, const char *key, const char *value);
EMAIL_API bool email_params_add_str_array(email_params_h params, const char *key,
		const char **value, int array_length);

EMAIL_API bool email_params_get_is_array(email_params_h params, const char *key, bool *is_array);

EMAIL_API bool email_params_get_int(email_params_h params, const char *key, int *result);
EMAIL_API bool email_params_get_int_opt(email_params_h params, const char *key, int *result);
EMAIL_API bool email_params_get_str(email_params_h params, const char *key, const char **result);
EMAIL_API bool email_params_get_str_opt(email_params_h params, const char *key, const char **result);
EMAIL_API bool email_params_get_str_array(email_params_h params, const char *key,
		const char ***result, int *array_length);

#endif /* __EMAIL_PARAMS__ */
