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

#include <string.h>
#include <Elementary.h>
#include <libexif/exif-data.h>
#include <image_util.h>

#include "email-debug.h"
#include "email-utils.h"

#include "email-composer.h"
#include "email-composer-types.h"
#include "email-composer-util.h"

#define RGB_BPP 3

typedef struct _rgba8888 {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
} rgba8888;

typedef struct _rgb888 {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} rgb888;

typedef struct _transform {
	int index;
	int coef;
} transform;

/*
 * Declaration for static functions
 */

#ifndef _TIZEN_2_4_BUILD_
static inline void __composer_util_make_image_resize_transform(transform *trans,
		int dst_length, int src_length);
static bool __composer_util_prepare_image_resize(const int dst_length, const int src_length,
		const int tmp_buff_mult, transform **trans, void **tmp_buff);
static bool __composer_util_resize_image_width_rgba(rgba8888 *const dst, const int dst_width,
		const rgba8888 *const src, const int src_width, const int height);
static bool __composer_util_resize_image_height_rgba(rgba8888 *const dst, const int dst_height,
		const rgba8888 *const src, const int src_height, const int width);

static int _composer_util_resize_image_rgba(
		unsigned char *const dst, const int dst_width , const int dst_height,
		const unsigned char *const src, const int src_width, const int src_height);
#endif

static int __composer_image_util_rotate(unsigned char *dest, int *dest_width, int *dest_height, const image_util_rotation_e dest_rotation,
		const unsigned char *src, const int src_w, const int src_h, const image_util_colorspace_e colorspace);

/*
 * Definitions for static & exported functions
 */

Eina_Bool composer_util_image_is_resizable_image_type(const char *mime_type)
{
	debug_enter();

	retvm_if(!mime_type, EINA_FALSE, "Invalid parameter: mime_type is NULL!");

#ifdef _TIZEN_2_4_BUILD_
	debug_leave();
	return EINA_FALSE;
#endif

	if (!g_strcmp0(mime_type, "image/png") || !g_strcmp0(mime_type, "image/jpeg")) {
		debug_leave();
		return EINA_TRUE;
	} else {
		debug_leave();
		return EINA_FALSE;
	}
}

Eina_Bool composer_util_image_get_size(void *data, const char *src_file, int *width, int *height)
{
	debug_enter();

	retvm_if(data == NULL, EINA_FALSE, "data is NULL!");
	retvm_if(src_file == NULL, EINA_FALSE, "src_file is NULL!");
	retvm_if(width == NULL, EINA_FALSE, "width is NULL!");
	retvm_if(height == NULL, EINA_FALSE, "height is NULL!");

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;
	int w, h;

	Evas_Object *img = evas_object_image_add(evas_object_evas_get(ugd->base.module->navi));
	if (!img) {
		debug_error("evas_object_image_add() failed!");
		return EINA_FALSE;
	}

	evas_object_image_file_set(img, src_file, NULL);
	evas_object_image_size_get(img, &w, &h);

	DELETE_EVAS_OBJECT(img);

	*width = w;
	*height = h;

	debug_secure("(%s) W:H = (%dx%d)", src_file, *width, *height);

	debug_leave();
	return EINA_TRUE;
}

/*
 * Example:
 *		Evas_Object *img;
 *		int w, h;
 *		composer_image_get_size(ugd, src_file_path, &w, &h);
 *		composer_image_set_image_with_size(ugd, &img, src_file_path, w / 2, h / 2);
 *		evas_object_image_save(img, dst_file_path, NULL, "quality=100");
 *		evas_object_del(img);
 */
Eina_Bool composer_util_image_set_image_with_size(void *data, Evas_Object **src_obj, const char *src_file, int dest_width, int dest_height)
{
	debug_enter();

	retvm_if(data == NULL, EINA_FALSE, "data is NULL!");
	retvm_if(src_obj == NULL, EINA_FALSE, "src_obj is NULL!");
	retvm_if(src_file == NULL, EINA_FALSE, "src_file is NULL!");

	if (email_file_exists(src_file) == EINA_FALSE) {
		debug_secure("email_file_exists(%s) failed!", src_file);
		return EINA_FALSE;
	}

	EmailComposerUGD *ugd = (EmailComposerUGD *)data;

	Ecore_Evas *m_ee = ecore_evas_buffer_new(dest_width, dest_height);
	if (!m_ee) {
		debug_error("ecore_evas_buffer_new() failed!");
		return EINA_FALSE;
	}

	Evas *evas = ecore_evas_get(m_ee);
	if (!evas) {
		debug_error("ecore_evas_get() failed!");
		ecore_evas_free(m_ee);
		return EINA_FALSE;
	}

	Evas_Object *img = evas_object_image_filled_add(evas);
	if (!img) {
		debug_error("evas_object_image_filled_add() failed!");
		ecore_evas_free(m_ee);
		return EINA_FALSE;
	}
	evas_object_image_file_set(img, src_file, NULL);
	evas_object_resize(img, dest_width, dest_height);
	evas_object_show(img);

	const void *pixels = ecore_evas_buffer_pixels_get(m_ee);
	Evas_Object *result = evas_object_image_add(evas_object_evas_get(ugd->base.module->navi));
	if (!result) {
		debug_error("evas_object_image_add() failed!");
		ecore_evas_free(m_ee);
		DELETE_EVAS_OBJECT(img);
		return EINA_FALSE;
	}

	evas_object_image_size_set(result, dest_width, dest_height);
	evas_object_image_data_copy_set(result, (void *)pixels);

	DELETE_EVAS_OBJECT(img);
	ecore_evas_free(m_ee);

	*src_obj = result;

	debug_leave();
	return EINA_TRUE;
}

Eina_Bool composer_util_image_scale_down_with_quality(const char *src_path, const char *dst_path, int quality)
{
	debug_enter();
	retvm_if(!src_path, EINA_FALSE, "src is NULL!");
	retvm_if(!dst_path, EINA_FALSE, "dst is NULL!");

#ifdef _TIZEN_2_4_BUILD_
	debug_secure("Not supported on Tizen 2.4");
	return EINA_FALSE;
#else

	if (!email_file_exists(src_path)) {
		debug_secure("email_file_exists(%s) failed!", src_path);
		return EINA_FALSE;
	}

	image_util_colorspace_e colorspace = IMAGE_UTIL_COLORSPACE_RGBA8888;
	image_util_type_e image_type = IMAGE_UTIL_PNG;
	bool image_type_ok = false;

	char *mime_type = email_get_mime_type_from_file(src_path);
	retvm_if(!mime_type, EINA_FALSE, "email_get_mime_type_from_file() failed!");

	if (g_strcmp0(mime_type, "image/jpeg") == 0) {
		colorspace = IMAGE_UTIL_COLORSPACE_RGB888;
		image_type = IMAGE_UTIL_JPEG;
		image_type_ok = true;
	} else if (g_strcmp0(mime_type, "image/png") == 0) {
		colorspace = IMAGE_UTIL_COLORSPACE_RGBA8888;
		image_type = IMAGE_UTIL_PNG;
		image_type_ok = true;
	}

	FREE(mime_type);

	retvm_if(!image_type_ok, EINA_FALSE, "File type not supported: %s", mime_type);

	Eina_Bool result = EINA_FALSE;
	int r = 0;

	image_util_decode_h decoder = NULL;
	image_util_encode_h encoder = NULL;
	unsigned long long int decode_size = 0;
	unsigned long long int encode_size = 0;
	unsigned char *src_buff = NULL;
	unsigned char *dst_buff = NULL;
	unsigned long src_w = 0;
	unsigned long src_h = 0;
	int dst_w = 0;
	int dst_h = 0;
	bool free_dst_buff = false;

	r = image_util_decode_create(&decoder);
	gotom_if(r != 0, exit_func, "image_util_decode_create() failed!");
	r = image_util_decode_set_input_path(decoder, src_path);
	gotom_if(r != 0, exit_func, "image_util_decode_set_input_path() failed!");
	r = image_util_decode_set_colorspace(decoder, colorspace);
	gotom_if(r != 0, exit_func, "image_util_decode_set_colorspace() failed!");
	r = image_util_decode_set_output_buffer(decoder, &src_buff);
	gotom_if(r != 0, exit_func, "image_util_decode_set_output_buffer() failed!");
	r = image_util_decode_run(decoder, &src_w, &src_h, &decode_size);
	gotom_if(r != 0, exit_func, "image_util_decode_run() failed!");

	r = image_util_encode_create(image_type, &encoder);
	gotom_if(r != 0, exit_func, "image_util_encode_create() failed!");
	r = image_util_encode_set_colorspace(encoder, colorspace);
	gotom_if(r != 0, exit_func, "image_util_encode_set_colorspace() failed!");
	r =image_util_encode_set_output_path(encoder, dst_path);
	gotom_if(r != 0, exit_func, "image_util_encode_set_output_path() failed!");

	dst_buff = src_buff;
	dst_w = src_w;
	dst_h = src_h;

	if (image_type == IMAGE_UTIL_JPEG) {

		int orientation = 0;
		if (composer_util_image_get_rotation_for_jpeg_image(src_path, &orientation)) {
			debug_log("==> orientation: (%d)", orientation);

			unsigned char *rotated_buffer = NULL;
			int rotated_width = 0;
			int rotated_height = 0;
			if (composer_util_image_rotate_jpeg_image_from_memory(src_buff, src_w, src_h,
					decode_size, orientation, &rotated_buffer, &rotated_width, &rotated_height)) {
				dst_w = rotated_width;
				dst_h = rotated_height;
				dst_buff = rotated_buffer;
				free_dst_buff = true;
			}
		}

		r = image_util_encode_set_quality(encoder, quality);
		gotom_if(r != 0, exit_func, "image_util_encode_set_quality() failed!");

	} else if (quality < 100) {

		float q = 1.0f;
		if (quality <= RESIZE_IMAGE_SMALL_SIZE) {
			q = 0.25;
		} else if (quality <= RESIZE_IMAGE_MEDIUM_SIZE) {
			q = 0.5;
		}

		dst_w = (int)(src_w * q + 0.5f);
		dst_h = (int)(src_h * q + 0.5f);
		dst_buff = malloc(dst_w * dst_h * sizeof(rgba8888));
		free_dst_buff = true;

		r = _composer_util_resize_image_rgba(dst_buff, dst_w, dst_h, src_buff, src_w, src_h);
		gotom_if(r != 0, exit_func, "_composer_util_resize_image_rgba() failed!");
	}

	r = image_util_encode_set_input_buffer(encoder, dst_buff);
	gotom_if(r != 0, exit_func, "image_util_encode_set_input_buffer() failed!");
	r = image_util_encode_set_resolution(encoder, dst_w, dst_h);
	gotom_if(r != 0, exit_func, "image_util_encode_set_resolution() failed!");
	r = image_util_encode_run(encoder, &encode_size);
	gotom_if(r != 0, exit_func, "image_util_decode_run() failed!");

	if (!email_file_exists(dst_path)) {
		debug_secure_error("email_file_exists(%s) failed!", dst_path);
		goto exit_func;
	}

	result = EINA_TRUE;

exit_func:
	if (free_dst_buff) {
		FREE(dst_buff);
	}
	if (encoder) {
		image_util_encode_destroy(encoder);
	}
	if (decoder) {
		image_util_decode_destroy(decoder);
	}

	debug_leave();
	return result;
#endif
}

Eina_Bool composer_util_image_get_rotation_for_jpeg_image(const char *src_file_path, int *rotation)
{
	debug_enter();

	retvm_if(email_file_exists(src_file_path) == EINA_FALSE, EINA_FALSE, "Invalid parameter: file not exist!");
	retvm_if(!rotation, EINA_FALSE, "Invalid parameter: rotation is NULL!");

	Eina_Bool ret = EINA_TRUE;

	ExifData *exif_data = exif_data_new_from_file(src_file_path);
	retvm_if(!exif_data, EINA_FALSE, "exif_data_new_from_file() failed! no exif data.");

	ExifEntry *exif_entry = exif_data_get_entry(exif_data, EXIF_TAG_ORIENTATION);
	if (exif_entry) {
		ExifByteOrder byte_order = exif_data_get_byte_order(exif_data);
		short exif_value = exif_get_short(exif_entry->data, byte_order);
		debug_log("value = %d", exif_value);

		if (exif_value >= EXIF_NOT_AVAILABLE && exif_value <= EXIF_ROT_270) {
			*rotation = exif_value;
		} else {
			*rotation = EXIF_NOT_AVAILABLE;
		}
	} else {
		debug_secure("exif_entry(for EXIF_TAG_ORIENTATION) is NULL!");
		ret = EINA_FALSE;
	}

	exif_data_unref(exif_data);

	debug_leave();
	return ret;
}

Eina_Bool composer_util_image_rotate_jpeg_image(const char *src_file_path, const char *dst_file_path, int rotation)
{
	debug_enter();
	retvm_if(src_file_path == NULL, EINA_FALSE, "src_file_path is NULL");
	retvm_if(dst_file_path == NULL, EINA_FALSE, "dst_file_path is NULL");

	int ret = IMAGE_UTIL_ERROR_NONE;
	unsigned char *dst = NULL;
	unsigned char *image_buffer = NULL;
	int width = 0;
	int height = 0;
	int dest_width = 0;
	int dest_height = 0;
	unsigned int size = 0;
	unsigned int dst_size = 0;
	image_util_rotation_e rot = IMAGE_UTIL_ROTATION_NONE;

	if (email_file_exists(src_file_path) == EINA_FALSE) {
		debug_secure("filepath:(%s)", src_file_path);
		debug_warning("email_file_exists() failed!");
		return EINA_FALSE;
	}

	if (rotation != EXIF_ROT_90 && rotation != EXIF_ROT_180 && rotation != EXIF_ROT_270) {
		debug_warning("Rotation is not required (rotation = %d)", rotation);
		return EINA_FALSE;
	}

	/*decode jpeg file*/
	ret = image_util_decode_jpeg(src_file_path, IMAGE_UTIL_COLORSPACE_RGB888, &image_buffer, &width, &height, &size);
	if (ret != IMAGE_UTIL_ERROR_NONE) {
		debug_warning("image_util_decode_jpeg is failed (ret = %d)", ret);
		return EINA_FALSE;
	}

	debug_log("width: %d, height: %d, size: %d, rotation: %d", width, height, size, rotation);
	if (rotation == EXIF_ROT_90 || rotation == EXIF_ROT_270) {
		dest_width = height;
		dest_height = width;
	} else { /* EXIF_ROT_180 */
		dest_width = width;
		dest_height = height;
	}

	/*get size about resize img*/
	dst_size = dest_width * dest_height * RGB_BPP;

	if (dst_size > 0) {
		debug_log("dst_size = %d", dst_size);
		dst = calloc(1, dst_size);
		if (dst == NULL) {
			debug_warning("fail of memory allocation size = %d", dst_size);
			goto err_return;
		}
	} else {
		debug_warning("no data size = %d", dst_size);
		goto err_return;
	}

	/* convert rotation value */
	if (rotation == EXIF_ROT_90)
		rot = IMAGE_UTIL_ROTATION_90;
	else if (rotation == EXIF_ROT_180)
		rot = IMAGE_UTIL_ROTATION_180;
	else /* EXIF_ROT_270 */
		rot = IMAGE_UTIL_ROTATION_270;

	dest_width = 0;
	dest_height = 0;

	debug_log("rotation : %d => rot : %d", rotation, rot);

	/* rotate image */
	ret = __composer_image_util_rotate(dst, &dest_width, &dest_height, rot, image_buffer, width, height, IMAGE_UTIL_COLORSPACE_RGB888);
	if (ret != IMAGE_UTIL_ERROR_NONE) {
		debug_warning("__composer_image_util_rotate is failed (ret = %d)", ret);
		goto err_return;
	}

	debug_log("[rotate image] (%d x %d) -> (%d x %d)", width, height, dest_width, dest_height);

	/*save image to dst file path*/
	ret = image_util_encode_jpeg(dst, dest_width, dest_height, IMAGE_UTIL_COLORSPACE_RGB888, 90, dst_file_path);
	if (ret != IMAGE_UTIL_ERROR_NONE) {
		debug_warning("image_util_encode_jpeg is failed (ret = %d)", ret);
		goto err_return;
	}

	if (image_buffer)
		free(image_buffer);

	if (dst)
		free(dst);

	debug_leave();
	return EINA_TRUE;

err_return:
	if (image_buffer)
		free(image_buffer);

	if (dst)
		free(dst);

	debug_leave();
	return EINA_FALSE;
}

Eina_Bool composer_util_image_rotate_jpeg_image_from_memory(const unsigned char *src_buffer, int width, int height, unsigned int size, int orientation, unsigned char **dst_buffer, int *dst_width, int *dst_height)
{
	debug_enter();

	retvm_if(src_buffer == NULL, EINA_FALSE, "Invalid parameter: src_buffer is NULL!");
	retvm_if(dst_buffer == NULL, EINA_FALSE, "Invalid parameter: dst_buffer is NULL!");

	int ret = IMAGE_UTIL_ERROR_NONE;
	image_util_rotation_e rot = IMAGE_UTIL_ROTATION_NONE;
	unsigned char *rotated_buffer = NULL;
	unsigned int rotated_size = 0;
	int rotated_width = 0, rotated_height = 0;

	switch (orientation) {
		case EXIF_ROT_90:
		case EXIF_ROT_270:
			rotated_width = height;
			rotated_height = width;
			if (EXIF_ROT_90 == orientation) {
				rot = IMAGE_UTIL_ROTATION_90;
			} else {
				rot = IMAGE_UTIL_ROTATION_270;
			}
			break;
		case EXIF_ROT_180:
			rotated_width = width;
			rotated_height = height;
			rot = IMAGE_UTIL_ROTATION_180;
			break;
		default:
			debug_log("Rotation is not required (orientation = %d)", orientation);
			return EINA_FALSE;
	}

	debug_log("width: %d, height: %d, size: %d, orientation: %d", width, height, size, orientation);

	rotated_size = rotated_width * rotated_height * RGB_BPP;
	debug_log("rotated_size: (%d)", rotated_size);

	rotated_buffer = calloc(1, rotated_size);
	gotom_if(!rotated_buffer, err_return, "Failed to allocate memory for dst with size:(%u)", rotated_size);

	rotated_width = 0;
	rotated_height = 0;

	ret = __composer_image_util_rotate(rotated_buffer, &rotated_width, &rotated_height, rot, src_buffer, width, height, IMAGE_UTIL_COLORSPACE_RGB888);
	gotom_if(ret != IMAGE_UTIL_ERROR_NONE, err_return, "__composer_image_util_rotate() failed! (%d)", ret);

	debug_log("[rotate image] (%d x %d) -> (%d x %d)", width, height, rotated_width, rotated_height);

	*dst_width = rotated_width;
	*dst_height = rotated_height;
	*dst_buffer = rotated_buffer;

	debug_leave();
	return EINA_TRUE;

err_return:
	FREE(rotated_buffer);

	dst_width = 0;
	dst_height = 0;
	*dst_buffer = NULL;

	debug_leave();
	return EINA_FALSE;
}

#ifndef _TIZEN_2_4_BUILD_
static inline void __composer_util_make_image_resize_transform(transform *trans,
		int dst_length, int src_length)
{
	const int src_index_max = src_length - 2;

	const float scale = (float)(src_length) / (float)(dst_length);
	const float bias = 0.5f * scale - 0.5f;

	int dst_index = 0;
	for (; dst_index < dst_length; ++dst_index) {
		const float src_pos = (float)dst_index * scale + bias;
		const float src_floor_pos = floorf(src_pos);
		const int src_index = (int)src_floor_pos;
		if (src_index < 0) {
			trans[dst_index].index = 0;
			trans[dst_index].coef = 0;
		} else if (src_index > src_index_max) {
			trans[dst_index].index = src_index_max;
			trans[dst_index].coef = 0x10000;
		} else {
			trans[dst_index].index = src_index;
			trans[dst_index].coef = (int)((src_pos - src_floor_pos) * 65536.0f + 0.5f);
		}
	}
}

static bool __composer_util_prepare_image_resize(const int dst_length, const int src_length,
		const int tmp_buff_mult, transform **trans, void **tmp_buff)
{
	int half_length = src_length;
	while ((half_length >> 1) >= dst_length) {
		half_length >>= 1;
	}

	if (half_length != src_length) {
		*tmp_buff = malloc((src_length >> 1) * tmp_buff_mult);
		if (!*tmp_buff) {
			debug_error("Memory allocation failed!");
			return false;
		}
	}

	if (half_length != dst_length) {
		*trans = malloc(dst_length * sizeof(transform));
		if (!*trans) {
			debug_error("Memory allocation failed!");
			FREE(*tmp_buff);
			return false;
		}
		__composer_util_make_image_resize_transform(*trans, dst_length, half_length);
	}

	return true;
}

static bool __composer_util_resize_image_width_rgba(rgba8888 *const dst, const int dst_width,
		const rgba8888 *const src, const int src_width, const int height)
{
	rgba8888 *tmp_row = NULL;
	transform *trans = NULL;

	if (!__composer_util_prepare_image_resize(dst_width, src_width,
			sizeof(rgba8888), &trans, (void **)&tmp_row)) {
		debug_error("Resize prepare failed!");
		return false;
	}

	const int half_src_width = (src_width >> 1);

	const rgba8888 *src_row = src;
	rgba8888 *dst_row = dst;

	int y = 0;
	for (; y < height; ++y) {
		const rgba8888 *src_row2 = src_row;

		int half_width = half_src_width;
		while (half_width >= dst_width) {
			rgba8888 *const dst_row2 = ((half_width == dst_width) ? dst_row : tmp_row);

			int x = 0;
			for (; x < half_width; ++x) {
				const rgba8888 src_pixel_l = src_row2[x * 2];
				const rgba8888 src_pixel_r = src_row2[x * 2 + 1];
				const rgba8888 p = {
						(src_pixel_l.r + src_pixel_r.r + 1) >> 1,
						(src_pixel_l.g + src_pixel_r.g + 1) >> 1,
						(src_pixel_l.b + src_pixel_r.b + 1) >> 1,
						(src_pixel_l.a + src_pixel_r.a + 1) >> 1};
				dst_row2[x] = p;
			}

			src_row2 = tmp_row;
			half_width >>= 1;
		}

		if (trans) {
			int x = 0;
			for (; x < dst_width; ++x) {
				const transform t = trans[x];
				const int inv_coef = (0x10000 - t.coef);
				const rgba8888 src_pixel_l = src_row2[t.index];
				const rgba8888 src_pixel_r = src_row2[t.index + 1];
				const rgba8888 p = {
						(src_pixel_l.r * inv_coef + src_pixel_r.r * t.coef + 0x8000) >> 16,
						(src_pixel_l.g * inv_coef + src_pixel_r.g * t.coef + 0x8000) >> 16,
						(src_pixel_l.b * inv_coef + src_pixel_r.b * t.coef + 0x8000) >> 16,
						(src_pixel_l.a * inv_coef + src_pixel_r.a * t.coef + 0x8000) >> 16};
				dst_row[x] = p;
			}
		}

		src_row += src_width;
		dst_row += dst_width;
	}

	free(tmp_row);
	free(trans);

	return true;
}

static bool __composer_util_resize_image_height_rgba(rgba8888 *const dst, const int dst_height,
		const rgba8888 *const src, const int src_height, const int width)
{
	rgba8888 *tmp_buff = NULL;
	transform *trans = NULL;

	if (!__composer_util_prepare_image_resize(dst_height, src_height,
			width * sizeof(rgba8888), &trans, (void **)&tmp_buff)) {
		debug_error("Resize prepare failed!");
		return false;
	}

	const rgba8888 *src2 = src;

	int half_height = (src_height >> 1);
	while (half_height >= dst_height) {
		rgba8888 *dst_row = ((half_height == dst_height) ? dst : tmp_buff);

		int y = 0;
		for (; y < half_height; ++y) {
			const rgba8888 *const src_row_t = src2 + y * 2 * width;
			const rgba8888 *const src_row_b = src_row_t + width;
			int x = 0;
			for (; x < width; ++x) {
				const rgba8888 src_pixel_t = src_row_t[x];
				const rgba8888 src_pixel_b = src_row_b[x];
				const rgba8888 p = {
						(src_pixel_t.r + src_pixel_b.r + 1) >> 1,
						(src_pixel_t.g + src_pixel_b.g + 1) >> 1,
						(src_pixel_t.b + src_pixel_b.b + 1) >> 1,
						(src_pixel_t.a + src_pixel_b.a + 1) >> 1};
				dst_row[x] = p;
			}
			dst_row += width;
		}

		src2 = tmp_buff;
		half_height >>= 1;
	}

	if (trans) {
		rgba8888 *dst_row = dst;

		int y = 0;
		for (y = 0; y < dst_height; ++y) {
			const transform t = trans[y];
			const int inv_coef = (0x10000 - t.coef);
			const rgba8888 *const src_row_t = src2 + t.index * width;
			const rgba8888 *const src_row_b = src_row_t + width;
			int x = 0;
			for (; x < width; ++x) {
				const rgba8888 src_pixel_t = src_row_t[x];
				const rgba8888 src_pixel_b = src_row_b[x];
				const rgba8888 p = {
						(src_pixel_t.r * inv_coef + src_pixel_b.r * t.coef + 0x8000) >> 16,
						(src_pixel_t.g * inv_coef + src_pixel_b.g * t.coef + 0x8000) >> 16,
						(src_pixel_t.b * inv_coef + src_pixel_b.b * t.coef + 0x8000) >> 16,
						(src_pixel_t.a * inv_coef + src_pixel_b.a * t.coef + 0x8000) >> 16};
				dst_row[x] = p;
			}
			dst_row += width;
		}
	}

	free(tmp_buff);
	free(trans);

	return true;
}

static int _composer_util_resize_image_rgba(
		unsigned char *const dst, const int dst_width , const int dst_height,
		const unsigned char *const src, const int src_width, const int src_height)
{
	debug_enter();
	retvm_if(!dst, IMAGE_UTIL_ERROR_INVALID_PARAMETER, "dst is NULL");
	retvm_if(!src, IMAGE_UTIL_ERROR_INVALID_PARAMETER, "src is NULL");

	if (dst_width <= 0 || dst_height <= 0 || dst_width > UINT16_MAX || dst_height > UINT16_MAX ||
		src_width <= 0 || src_height <= 0 || src_width > UINT16_MAX || src_height > UINT16_MAX) {
		debug_error("Invalid image size: dst(%d, %d); src(%d, %d)",
				dst_width, dst_height, src_width, src_height);
		return IMAGE_UTIL_ERROR_INVALID_PARAMETER;
	}

	bool ok = false;

	if ((dst_width == src_width) && (dst_height == src_height)) {

		memcpy(dst, src, dst_width * dst_height * sizeof(rgba8888));

	} else if (dst_width == src_width) {

		ok = __composer_util_resize_image_height_rgba((rgba8888 *)dst, dst_height,
				(rgba8888 *)dst, src_height, dst_width);

	} else if (dst_height == src_height) {

		ok = __composer_util_resize_image_width_rgba((rgba8888 *)dst, dst_width,
				(rgba8888 *)src, src_width, dst_height);

	} else {
		const int buff1_size = dst_width * src_height;
		const int buff2_size = dst_height * src_width;

		rgba8888 *const tmp_buff = malloc(MIN(buff1_size, buff2_size) * sizeof(*tmp_buff));
		if (!tmp_buff) {
			debug_error("Memory allocation failed!");
			return IMAGE_UTIL_ERROR_OUT_OF_MEMORY;
		}

		if (buff1_size < buff2_size) {
			ok = (__composer_util_resize_image_width_rgba(tmp_buff, dst_width,
						(rgba8888 *)src, src_width, src_height)) &&
				 (__composer_util_resize_image_height_rgba((rgba8888 *)dst, dst_height,
						tmp_buff, src_height, dst_width));
		} else {
			ok = (__composer_util_resize_image_height_rgba(tmp_buff, dst_height,
						(rgba8888 *)src, src_height, src_width)) &&
				 (__composer_util_resize_image_width_rgba((rgba8888 *)dst, dst_width,
						tmp_buff, src_width, dst_height));
		}

		free(tmp_buff);
	}

	if (!ok) {
		debug_error("Image resize failed!");
		return IMAGE_UTIL_ERROR_OUT_OF_MEMORY;
	}

	debug_leave();
	return IMAGE_UTIL_ERROR_NONE;
}
#endif

static int __composer_image_util_rotate(unsigned char *dest, int *dest_width, int *dest_height, const image_util_rotation_e dest_rotation,
										const unsigned char *src, const int src_w, const int src_h, const image_util_colorspace_e colorspace)
{
	debug_enter();
	if (!dest || !dest_width || !dest_height || !src) {
		debug_leave();
		return IMAGE_UTIL_ERROR_INVALID_PARAMETER;
	}

	if (IMAGE_UTIL_COLORSPACE_RGB888 != colorspace || src_w <= 0 || src_h <= 0
		|| dest_rotation <= IMAGE_UTIL_ROTATION_NONE || dest_rotation > IMAGE_UTIL_ROTATION_270) {
		debug_leave();
		return IMAGE_UTIL_ERROR_INVALID_PARAMETER;
	}

	const int dest_w = (IMAGE_UTIL_ROTATION_180 == dest_rotation ? src_w : src_h);
	const int dest_h = (IMAGE_UTIL_ROTATION_180 == dest_rotation ? src_h : src_w);
	const unsigned int src_stride = src_w * sizeof(rgb888);
	const unsigned int dest_stride = dest_w * sizeof(rgb888);
	int x = 0, y = 0;

	switch (dest_rotation) {
		case IMAGE_UTIL_ROTATION_90:
		{
			const rgb888 *const src_col_0 = (rgb888 *) (src + (src_h - 1) * src_stride);
			for (y = 0; y < dest_h; y++) {
				rgb888 *const dest_row = (rgb888 *) (dest + y * dest_stride);
				const rgb888 *const src_col = (src_col_0 + y);
				for (x = 0; x < dest_w; x++) {
					dest_row[x] = *(rgb888 *)((uint8_t *)src_col - x * src_stride);
				}
			}
			break;
		}
		case IMAGE_UTIL_ROTATION_180:
		{
			const rgb888 *const src_row_rev_0 = (rgb888 *) (src + (src_h - 1) * src_stride
					+ (src_w - 1) * sizeof(rgb888));
			for (y = 0; y < dest_h; y++) {
				rgb888 *const dest_row = (rgb888 *) (dest + y * dest_stride);
				const rgb888 *const src_row_rev = (rgb888 *) ((uint8_t *)src_row_rev_0 - y * src_stride);
				for (x = 0; x < dest_w; x++) {
					dest_row[x] = *(src_row_rev - x);
				}
			}
			break;
		}
		case IMAGE_UTIL_ROTATION_270:
		{
			const rgb888 *const src_col_rev_0 = (rgb888 *) (src + (src_w - 1) * sizeof(rgb888));
			for (y = 0; y < dest_h; y++) {
				rgb888 *const dest_row = (rgb888 *) (dest + y * dest_stride);
				const rgb888 *const src_col_rev = (src_col_rev_0 - y);
				for (x = 0; x < dest_w; x++) {
					dest_row[x] = *(rgb888 *)((uint8_t *)src_col_rev + x * src_stride);
				}
			}
			break;
		}
		default:
			debug_leave();
			return IMAGE_UTIL_ERROR_INVALID_PARAMETER;
	}

	*dest_width = dest_w;
	*dest_height = dest_h;
	debug_leave();
	return IMAGE_UTIL_ERROR_NONE;
}

