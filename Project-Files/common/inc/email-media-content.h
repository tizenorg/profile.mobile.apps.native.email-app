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

#ifndef __EMAIL_MEDIA_CONTENT_H_DEF__
#define __EMAIL_MEDIA_CONTENT_H_DEF__
#include <media_content.h>

typedef struct _email_condition_s {
	char *cond;
	media_content_collation_e collate_type;
	media_content_order_e sort_type;
	char *sort_keyword;
	int offset;
	int count;
	bool with_meta;
} email_condition_s;

typedef struct _email_media_data {
	const char *file_path;
	char *thumb_path;
	media_info_h *media;
} email_media_data;

EMAIL_API int email_media_content_get_image_thumbnail(const char *file_path, char **thumbnail_path);

#endif

