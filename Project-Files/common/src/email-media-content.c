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

#include <media_content.h>

#include "email-utils.h"
#include "email-media-content.h"

static int _media_content_destroy_filter(filter_h filter);
static int _media_content_create_filter(filter_h *filter, email_condition_t *condition);
static void _media_content_free_condition(email_condition_t **condition);
static email_condition_t *_media_content_condition_create(char *condition);

static email_condition_t *_media_content_condition_create(char *condition)
{
	debug_enter();

	email_condition_t *filter = NULL;

	filter = (email_condition_t *)calloc(1, sizeof(email_condition_t));
	retvm_if(filter == NULL, NULL, "filter is NULL");

	memset(filter, 0, sizeof(email_condition_t));

	debug_secure("condition [%s]", condition);
	filter->cond = condition;
	filter->collate_type = MEDIA_CONTENT_COLLATE_DEFAULT;
	filter->sort_type = MEDIA_CONTENT_ORDER_DESC;
	filter->sort_keyword = MEDIA_MODIFIED_TIME;
	filter->with_meta = true;

	debug_leave();
	return filter;
}

static void _media_content_free_condition(email_condition_t **condition)
{
	debug_enter();

	retm_if(condition == NULL, "condition is NULL");
	if (*condition) {
		FREE((*condition)->cond);
		FREE(*condition);
	}

	debug_leave();
}

static int _media_content_create_filter(filter_h *filter, email_condition_t *condition)
{
	debug_enter();

	retvm_if(filter == NULL, -1, "filter is NULL");
	retvm_if(condition == NULL, -1, "condition is NULL");

	int ret = MEDIA_CONTENT_ERROR_NONE;
	filter_h tmp_filter = NULL;
	ret = media_filter_create(&tmp_filter);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		debug_log("Fail to create filter");
		return ret;
	}
	if (condition->cond) {
		ret = media_filter_set_condition(tmp_filter, condition->cond, condition->collate_type);
		if (ret != MEDIA_CONTENT_ERROR_NONE) {
			debug_log("Fail to set condition");
			goto ERROR;
		}
	}

	if (condition->sort_keyword) {
		ret = media_filter_set_order(tmp_filter, condition->sort_type, condition->sort_keyword, condition->collate_type);
		if (ret != MEDIA_CONTENT_ERROR_NONE) {
			debug_log("Fail to set order");
			goto ERROR;
		}
	}

	debug_log("offset is %d, count is %d", condition->offset, condition->count);
	if (condition->offset != -1 && condition->count != -1 && condition->count > condition->offset) {
		ret = media_filter_set_offset(tmp_filter, condition->offset, condition->count);
		if (ret != MEDIA_CONTENT_ERROR_NONE) {
			debug_log("Fail to set offset");
			goto ERROR;
		}
	}
	*filter = tmp_filter;

	debug_leave();
	return ret;

ERROR:
	if (tmp_filter) {
		media_filter_destroy(tmp_filter);
		tmp_filter = NULL;
	}

	debug_leave();
	return ret;
}

static int _media_content_destroy_filter(filter_h filter)
{
	debug_enter();

	retvm_if(filter == NULL, -1, "filter is NULL");
	int ret = MEDIA_CONTENT_ERROR_NONE;
	ret = media_filter_destroy(filter);

	debug_leave();
	return ret;
}

int email_media_content_data_get(void *data, char *condition, bool (*func) (media_info_h media, void *data))
{
	debug_enter();

	email_condition_t *temp_condition = NULL;
	filter_h filter = NULL;
	temp_condition = _media_content_condition_create(condition);

	int ret = -1;
	ret = _media_content_create_filter(&filter, temp_condition);

	if (ret != 0) {
		debug_log("Create filter failed");
		_media_content_free_condition(&temp_condition);
		return ret;
	}

	if (ret != 0) {
		debug_log("media_info_foreach_media_from_db failed: %d", ret);
	} else {
		debug_log("media_info_foreach_media_from_db success!");
	}

	_media_content_destroy_filter(filter);
	_media_content_free_condition(&temp_condition);

	debug_leave();
	return ret;

}

static bool _email_get_media_thumbnail_cb(media_info_h media, void *data)
{
	debug_enter();

	retvm_if(data == NULL, -1, "data is NULL");
	email_media_data_t *tmp_data = (email_media_data_t *)data;

	media_info_clone(tmp_data->media, media);
	media_info_get_thumbnail_path(media, &(tmp_data->thumb_path));
	debug_leave();
	return false;
}

EMAIL_API int email_media_content_get_image_thumbnail(const char *file_path, char **thumbnail_path)
{
	debug_enter();

	retvm_if(file_path == NULL, 1, "file_path is NULL");
	char *icon_path = NULL;

	int error_code = 0;
	int err = 0;
	email_media_data_t tmp_data;
	memset(&tmp_data, 0x00, sizeof(email_media_data_t));
	tmp_data.file_path = file_path;
	media_info_h *media = (media_info_h *)calloc(1, sizeof(media_info_h));
	tmp_data.media = media;

	int mc_ret = 0;

	mc_ret = media_content_connect();
	if (mc_ret == MEDIA_CONTENT_ERROR_NONE) {
		char *condition = NULL;
		condition = g_strdup_printf("%s and MEDIA_PATH=\"%s\"", "(MEDIA_TYPE=0 OR MEDIA_TYPE=1)", tmp_data.file_path);
		err = email_media_content_data_get(&tmp_data, condition, _email_get_media_thumbnail_cb);
		debug_secure("thumb_path [%s], err [%d]", tmp_data.thumb_path, err);
		if (err == 0) {
			if (tmp_data.thumb_path && email_file_exists(tmp_data.thumb_path)) {
				icon_path = g_strdup(tmp_data.thumb_path);
			} else {
				error_code = 1;
			}
		} else {
				error_code = err;
		}
		free(tmp_data.thumb_path);

		*thumbnail_path = icon_path;

		mc_ret = media_content_disconnect();
		if (mc_ret != MEDIA_CONTENT_ERROR_NONE) {
			debug_log("media_content_disconnect() is failed!");
		}
	} else {
		debug_log("media_content_connect() is failed!");
		error_code = mc_ret;
	}
	free(tmp_data.media);

	debug_leave();
	return error_code;
}
