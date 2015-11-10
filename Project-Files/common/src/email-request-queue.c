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

#include "email-request-queue.h"
#include "email-debug.h"
#include "email-utils.h"

typedef enum _email_message_type email_message_type;
enum _email_message_type {
	EMAIL_MESSAGE_TYPE_FEEDBACK = 0,
	EMAIL_MESSAGE_TYPE_END
};

typedef struct _email_request_queue email_request_queue_t;
struct _email_request_queue
{
	Eina_Lock mutex;
	Eina_Condition condition;
	Ecore_Thread *process_thread;
	Eina_List *queue_req_list;
	Eina_List *all_req_list;
	Eina_Bool is_canceled;
};

typedef struct _email_request email_request_t;
struct _email_request {
	email_request_type type;
	void *user_data;
	Eina_Bool is_canceled;
	email_request_queue_t *rqueue;
};

typedef struct _email_request_cbs email_thread_cbs;
struct _email_request_cbs {
	heavy_req_cb heavy_cb;
	notify_req_cb notify_cb;
	end_req_cb end_cb;
};

typedef struct _email_message email_message_t;
struct _email_message {
	email_request_t *request;
	void *data;
	email_message_type type;
};

static email_thread_cbs _g_request_cbs[EMAIL_REQUEST_TYPE_MAX];

static void _email_request_queue_clear_data(email_request_queue_t *rqueue);
static void _email_request_send_end_feedback(email_request_t *request);
static void _email_request_heavy_cb(void *data, Ecore_Thread *thd);
static void _email_request_notify_cb(void *data, Ecore_Thread *thd, void* message);
static void _email_request_end_cb(void *data, Ecore_Thread *thd);

EMAIL_API email_request_queue_h email_request_queue_create()
{
	debug_enter();

	email_request_queue_t *req_queue = calloc(1, sizeof(email_request_queue_t));
	retvm_if(!req_queue, NULL, "memory allocation failed");

	Eina_Bool res = eina_lock_new(&req_queue->mutex);
	if (!res) {
		debug_error("eina_lock_new() failed");
		free(req_queue);
		return NULL;
	}

	res = eina_condition_new(&req_queue->condition, &req_queue->mutex);
	if (!res) {
		debug_error("eina_condition_new() failed");
		eina_lock_free(&req_queue->mutex);
		free(req_queue);
		return NULL;
	}

	req_queue->process_thread = ecore_thread_feedback_run(_email_request_heavy_cb,
			_email_request_notify_cb,
			_email_request_end_cb,
			_email_request_end_cb,
			req_queue,
			EINA_TRUE);

	if (!req_queue->process_thread) {
		debug_error("ecore_thread_feedback_run() failed");
		_email_request_queue_clear_data(req_queue);
		return NULL;
	}
	debug_leave();

	return (email_request_queue_h)req_queue;
}

EMAIL_API void email_request_queue_destroy(email_request_queue_h rqueue)
{
	debug_enter();

	retm_if(!rqueue, "rqueue is NULL");

	email_request_queue_t *req_queue = (email_request_queue_t *)rqueue;

	email_request_queue_cancel_all_requests(rqueue);

	eina_lock_take(&req_queue->mutex);
	req_queue->is_canceled = EINA_TRUE;
	eina_lock_release(&req_queue->mutex);

	eina_condition_signal(&req_queue->condition);

	debug_leave();
}

EMAIL_API Eina_Bool email_request_queue_add_request(email_request_queue_h rqueue, email_request_type req_type, void *user_data)
{
	debug_enter();

	retvm_if(!rqueue, EINA_FALSE, "rqueue is NULL");
	retvm_if((req_type < EMAIL_REQUEST_TYPE_ADD_REMAINING_MAIL
			|| req_type >= EMAIL_REQUEST_TYPE_MAX),
			EINA_FALSE,  "invalid request type");
	retvm_if(!_g_request_cbs[req_type].heavy_cb,
			EINA_FALSE,  "heavy_cb function is not register for this request type");

	email_request_queue_t *req_queue = (email_request_queue_t *)rqueue;

	email_request_t *new_req = calloc(1, sizeof(email_request_t));
	retvm_if(!new_req, EINA_FALSE, "memory allocation failed");

	new_req->type = req_type;
	new_req->user_data = user_data;
	new_req->rqueue = req_queue;
	req_queue->all_req_list = eina_list_append(req_queue->all_req_list, new_req);

	eina_lock_take(&req_queue->mutex);
	req_queue->queue_req_list = eina_list_append(req_queue->queue_req_list, new_req);
	eina_lock_release(&req_queue->mutex);

	eina_condition_signal(&req_queue->condition);

	debug_leave();

	return EINA_TRUE;
}

EMAIL_API void email_request_queue_cancel_all_requests(email_request_queue_h rqueue)
{
	debug_enter();
	retm_if(!rqueue, "rqueue is NULL");
	email_request_queue_t *req_queue = (email_request_queue_t *)rqueue;

	Eina_List *l_next = NULL;
	Eina_List *l = NULL;
	email_request_t *req = NULL;
	EINA_LIST_FOREACH_SAFE(req_queue->all_req_list, l, l_next, req) {
		req->is_canceled = EINA_TRUE;
		req_queue->all_req_list = eina_list_remove_list(req_queue->all_req_list, l);
	}

	eina_lock_take(&req_queue->mutex);

	EINA_LIST_FOREACH_SAFE(req_queue->queue_req_list, l, l_next, req) {
		if (_g_request_cbs[req->type].end_cb) {
			_g_request_cbs[req->type].end_cb((email_request_h)req);
		}
		req_queue->queue_req_list = eina_list_remove_list(req_queue->queue_req_list, l);
	}

	eina_lock_release(&req_queue->mutex);

	debug_leave();
}

EMAIL_API void *email_request_get_data(email_request_h request)
{
	retvm_if(!request, NULL, "request is NULL");
	email_request_t *req = (email_request_t *)request;
	return req->user_data;
}

EMAIL_API Eina_Bool email_request_check(email_request_h request)
{
	retvm_if(!request, EINA_TRUE, "request is NULL");
	email_request_t *req = (email_request_t *)request;
	return req->is_canceled;
}

EMAIL_API void email_request_send_feedback(email_request_h request, void *msg_data)
{
	retm_if(!request, "request is NULL");
	retm_if(!msg_data, "msg_data is NULL");

	email_request_t *req = (email_request_t *)request;

	email_message_t *msg = calloc(1, sizeof(email_message_t));
	retm_if(!msg, "memory allocation failed");
	msg->request = req;
	msg->data = msg_data;
	msg->type = EMAIL_MESSAGE_TYPE_FEEDBACK;

	ecore_thread_feedback(req->rqueue->process_thread, msg);
}

EMAIL_API void email_register_request_cbs(email_request_type req_type,
		heavy_req_cb heavy_cb,
		notify_req_cb notify_cb,
		end_req_cb end_cb)
{
	debug_enter();

	retm_if(!heavy_cb, "heavy_cb is NULL");

	_g_request_cbs[req_type].heavy_cb = heavy_cb;
	_g_request_cbs[req_type].notify_cb = notify_cb;
	_g_request_cbs[req_type].end_cb = end_cb;

	debug_leave();
}

EMAIL_API void email_unregister_request_cbs(email_request_type req_type)
{
	debug_enter();

	_g_request_cbs[req_type].heavy_cb = NULL;
	_g_request_cbs[req_type].notify_cb = NULL;
	_g_request_cbs[req_type].end_cb = NULL;

	debug_leave();
}

static void _email_request_queue_clear_data(email_request_queue_t *rqueue)
{
	eina_condition_free(&rqueue->condition);
	eina_lock_free(&rqueue->mutex);
	FREE(rqueue);
}

static void _email_request_send_end_feedback(email_request_t *request)
{
	email_message_t *msg = calloc(1, sizeof(email_message_t));
	retm_if(!msg, "msg is NULL!");
	msg->request = request;
	msg->type = EMAIL_MESSAGE_TYPE_END;

	ecore_thread_feedback(request->rqueue->process_thread, msg);
}

static void _email_request_heavy_cb(void *data, Ecore_Thread *thd)
{
	debug_enter();

	email_request_queue_t *req_queue = data;
	do
	{
		debug_log("try get next request from queue");
		email_request_t *cur_request = NULL;
		eina_lock_take(&req_queue->mutex);

		int count = eina_list_count(req_queue->queue_req_list);
		debug_log("number of requests in queue [%d]", count);
		if (count != 0) {
			debug_log("next request from queue got");
			cur_request = (email_request_t *)eina_list_data_get(req_queue->queue_req_list);
			req_queue->queue_req_list = eina_list_remove_list(req_queue->queue_req_list, req_queue->queue_req_list);
		} else {
			if (req_queue->is_canceled) {
				debug_log("thread is cancelled");
				eina_lock_release(&req_queue->mutex);
				return;
			}
			debug_log("start waiting for new requests");
			eina_condition_wait(&req_queue->condition);
			debug_log("signal to continue process requests received");
			eina_lock_release(&req_queue->mutex);
			continue;
		}

		eina_lock_release(&req_queue->mutex);

		debug_log("start process request");
		if (_g_request_cbs[cur_request->type].heavy_cb) {
			_g_request_cbs[cur_request->type].heavy_cb((email_request_h)cur_request);
		} else {
			debug_error("heavy_cb for request type %d is NULL", cur_request->type);
		}
		_email_request_send_end_feedback(cur_request);

	} while(true);

	debug_leave();
}

static void _email_request_notify_cb(void *data, Ecore_Thread *thd, void* message)
{
	debug_enter();

	retm_if(!message, "message is NULL");

	email_message_t *msg = (email_message_t*)message;
	email_request_t *req = msg->request;
	email_request_type cur_req_type = req->type;


	switch(msg->type) {
	case EMAIL_MESSAGE_TYPE_FEEDBACK:
		if (_g_request_cbs[cur_req_type].notify_cb) {
			_g_request_cbs[cur_req_type].notify_cb((email_request_h)req, msg->data);
		}
		break;
	case EMAIL_MESSAGE_TYPE_END:
		if (_g_request_cbs[cur_req_type].end_cb) {
			_g_request_cbs[cur_req_type].end_cb((email_request_h)req);
		}
		req->rqueue->all_req_list = eina_list_remove(req->rqueue->all_req_list, req);
		break;
	default:
		debug_error("wrong message type");
		break;
	}

	free(msg);

	debug_leave();
}

static void _email_request_end_cb(void *data, Ecore_Thread *thd)
{
	debug_enter();

	retm_if(!data, "data is NULL");
	email_request_queue_t *req_queue = (email_request_queue_t *)data;

	req_queue->process_thread = NULL;
	_email_request_queue_clear_data(req_queue);

	debug_leave();
}
