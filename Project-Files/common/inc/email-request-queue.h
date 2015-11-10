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

#ifndef _EMAIL_REQUEST_QUEUE_H_
#define _EMAIL_REQUEST_QUEUE_H_

#include "email-common-types.h"

typedef enum _email_request_type email_request_type;
enum _email_request_type {
	EMAIL_REQUEST_TYPE_ADD_REMAINING_MAIL = 0,
	EMAIL_REQUEST_TYPE_MOVE_MAIL,
	EMAIL_REQUEST_TYPE_DELETE_MAIL,
	EMAIL_REQUEST_TYPE_ADD_MAIL,
	EMAIL_REQUEST_TYPE_MAX
};

typedef struct {} *email_request_queue_h;
typedef struct {} *email_request_h;

typedef void (*heavy_req_cb)(email_request_h request);
typedef void (*notify_req_cb)(email_request_h request, void *msg_data);
typedef void (*end_req_cb)(email_request_h request);

EMAIL_API void email_register_request_cbs(email_request_type req_type, heavy_req_cb heavy_cb, notify_req_cb notify_cb, end_req_cb end_cb);
EMAIL_API void email_unregister_request_cbs(email_request_type req_type);

EMAIL_API email_request_queue_h email_request_queue_create();
EMAIL_API void email_request_queue_destroy(email_request_queue_h rqueue);
EMAIL_API Eina_Bool email_request_queue_add_request(email_request_queue_h rqueue, email_request_type req_type, void *user_data);
EMAIL_API void email_request_queue_cancel_all_requests(email_request_queue_h rqueue);

EMAIL_API void *email_request_get_data(email_request_h request);
EMAIL_API Eina_Bool email_request_check(email_request_h request);
EMAIL_API void email_request_send_feedback(email_request_h request, void *msg_data);

#endif /* _EMAIL_REQUEST_QUEUE_H_*/
