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

#include <png.h>
#include <string.h>
#include <Elementary.h>
#include <libexif/exif-data.h>
#include <image_util.h>

#include "email-debug.h"
#include "email-utils.h"

#include "email-composer.h"
#include "email-composer-types.h"
#include "email-composer-util.h"

#define Z_BEST_COMPRESSION 9


typedef unsigned char uchar;
typedef struct _rgb888 {
	uchar r;
	uchar g;
	uchar b;
} rgb888;
typedef struct _rgba8888 {
	uchar r;
	uchar g;
	uchar b;
	uchar a;
} rgba8888;
typedef struct _transform {
	int index;
	float coef;
} transform;

/*
 * Declarations for static variables
 */

static const int RGBA_BPP = 4;
static const int RGB_BPP = 3;

/*
 * Declaration for static functions
 */

static Eina_Bool __composer_util_image_read_png_image(const char *filename, png_bytep **row_pointers, png_uint_32 *width, png_uint_32 *height,
                                                      int *bit_depth, int *color_type, int *interlace_type, int quality, int *channels, image_util_colorspace_e *colorspace);
static Eina_Bool __composer_util_image_write_png_image(const char *filename, png_bytep *row_pointers, png_uint_32 width, png_uint_32 height, int bit_depth, int color_type, int interlace_type);

static int __composer_image_util_resize(unsigned char *dest, const int *dest_width , const int *dest_height, const unsigned char *src,
		const int src_w, const int src_h, const image_util_colorspace_e colorspace);
static int __composer_image_util_rotate(unsigned char *dest, int *dest_width, int *dest_height, const image_util_rotation_e dest_rotation,
		const unsigned char *src, const int src_w, const int src_h, const image_util_colorspace_e colorspace);

/*
 * Definitions for static & exported functions
 */

Eina_Bool composer_util_image_is_resizable_image_type(const char *mime_type)
{
	debug_enter();

	retvm_if(!mime_type, EINA_FALSE, "Invalid parameter: mime_type is NULL!");

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

static Eina_Bool __composer_util_image_read_png_image(const char *filename, png_bytep **row_pointers, png_uint_32 *width, png_uint_32 *height,
                                                      int *bit_depth, int *color_type, int *interlace_type, int quality, int *channels, image_util_colorspace_e *colorspace)
{
	debug_enter();

	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;

	png_uint_32 w = 0;
	png_uint_32 h = 0;
	int t = 0, b = 0;
	image_util_colorspace_e img_colorspace = IMAGE_UTIL_COLORSPACE_RGB888;
	unsigned char header[8] = { 0, }; /* 8 is the maximum size that can be checked */

	FILE *fp = fopen(filename, "rb");
	retvm_if(!fp, EINA_FALSE, "fopen() failed!");

	size_t n = fread(header, 1, 8, fp);
	if (n <= 0) {
		debug_error("file size error ==> n:%d", n);
		fclose(fp);
		return EINA_FALSE;
	}
	if (png_sig_cmp(header, 0, 8) != 0) {
		debug_error("file is not recognized as a PNG file!");
		fclose(fp);
		return EINA_FALSE;
	}

	/* initialize stuff */
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) {
		debug_error("png_create_info_struct() failed!");
		fclose(fp);
		return EINA_FALSE;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		debug_error("png_create_info_struct() failed!");
		fclose(fp);
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		return EINA_FALSE;
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		debug_error("error occurred on libpng!");
		fclose(fp);
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		return EINA_FALSE;
	}

	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, 8);
	png_read_info(png_ptr, info_ptr);

	int intr_type = png_get_interlace_type(png_ptr, info_ptr);

	/* Currently interlaced PNG images are not supported */
	if (intr_type != PNG_INTERLACE_NONE) {
		debug_warning("interlace not supported");
		fclose(fp);
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		return EINA_FALSE;
	}

	int b_depth = png_get_bit_depth(png_ptr, info_ptr);
	int c_type = png_get_color_type(png_ptr, info_ptr);

	/* Convert palette color to RGB. */
	if (c_type == PNG_COLOR_TYPE_PALETTE) {
		png_set_palette_to_rgb(png_ptr);
	}

	/* Convert grayscale with less than 8 bpp to 8 bpp. */
	if (c_type == PNG_COLOR_TYPE_GRAY && b_depth < 8) {
		png_set_expand_gray_1_2_4_to_8(png_ptr);
	}

	/* Add full alpha channel if there's transparency. */
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
		png_set_tRNS_to_alpha(png_ptr);
	}

	/* PNGs support 16 bpp, but we don't. */
	if (b_depth == 16) {
		debug_warning("converting 16-bit channels to 8-bit");
		png_set_strip_16(png_ptr);
	}

	/* If there's more than one pixel per byte, expand to 1 pixel / byte. */
	if (b_depth < 8) {
		png_set_packing(png_ptr);
	}

	/* Update info after updating attributes. */
	png_read_update_info(png_ptr, info_ptr);

	/* Get info again after updating info. */
	png_get_IHDR(png_ptr, info_ptr, &w, &h, &b, &t, NULL, NULL, NULL);
	debug_log("(w:%d, h:%d, b:%d, t:%d)", w, h, b, t);

	/* read file */
	if (setjmp(png_jmpbuf(png_ptr))) {
		debug_error("error occurred on libpng!");
		fclose(fp);
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		return EINA_FALSE;
	}

	int i = 0;
	int chnls = png_get_channels(png_ptr, info_ptr);
	if (chnls == 4) {
		img_colorspace = IMAGE_UTIL_COLORSPACE_RGBA8888;
	} else {
		img_colorspace = IMAGE_UTIL_COLORSPACE_RGB888;
	}

	debug_log("channels: [%d]", chnls);

	png_bytep *row_ptrs = NULL;
	row_ptrs = calloc(h, sizeof(png_bytep));
	for (i = 0; i < h; ++i) {
		row_ptrs[i] = malloc(w * chnls);
	}
	png_read_image(png_ptr, row_ptrs);
	png_read_end(png_ptr, info_ptr);

	*width = w;
	*height = h;
	*bit_depth = b;
	*color_type = t;
	*row_pointers = row_ptrs;
	*channels = chnls;
	*colorspace = img_colorspace;

	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	fclose(fp);

	debug_leave();
	return EINA_TRUE;
}

static Eina_Bool __composer_util_image_write_png_image(const char *filename, png_bytep *row_pointers, png_uint_32 width, png_uint_32 height, int bit_depth, int color_type, int interlace_type)
{
	debug_enter();

	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;

	FILE *fp = fopen(filename, "wb");
	retvm_if(!fp, EINA_FALSE, "fopen() failed!");

	/* initialize stuff */
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) {
		debug_error("png_create_write_struct() failed!");
		fclose(fp);
		return EINA_FALSE;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		debug_error("png_create_info_struct() failed!");
		fclose(fp);
		png_destroy_write_struct(&png_ptr, NULL);
		return EINA_FALSE;
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		debug_error("error occurred on libpng!");
		fclose(fp);
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return EINA_FALSE;
	}

	debug_log("(w:%d, h:%d, bit_depth:%d, color_type:%d)", width, height, bit_depth, color_type);

	png_init_io(png_ptr, fp);
	png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);
	png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, color_type, interlace_type, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	png_write_info(png_ptr, info_ptr);

	/* write bytes */
	if (setjmp(png_jmpbuf(png_ptr))) {
		debug_error("error occurred on libpng!");
		fclose(fp);
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return EINA_FALSE;
	}

	png_write_image(png_ptr, row_pointers);
	png_write_end(png_ptr, NULL);

	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(fp);

	debug_leave();
	return EINA_TRUE;
}

Eina_Bool composer_util_image_scale_down_with_quality(void *data, const char *src, const char *dst, int quality)
{
	debug_enter();

	retvm_if(data == NULL, EINA_FALSE, "data is NULL!");
	retvm_if(src == NULL, EINA_FALSE, "src is NULL!");
	retvm_if(dst == NULL, EINA_FALSE, "dst is NULL!");

	if (email_file_exists(src) == EINA_FALSE) {
		debug_secure("email_file_exists(%s) failed!", src);
		return EINA_FALSE;
	}

	Eina_Bool err = EINA_FALSE;
	char *mime_type = email_get_mime_type_from_file(src);
	if (!g_strcmp0(mime_type, "image/jpeg")) {
		unsigned char *jpeg_image_buffer = NULL;
		int width = 0, height = 0;
		unsigned int size = 0;

		int ret = image_util_decode_jpeg(src, IMAGE_UTIL_COLORSPACE_RGB888, &jpeg_image_buffer, &width, &height, &size);
		if (ret != IMAGE_UTIL_ERROR_NONE) {
			debug_error("image_util_decode_jpeg() failed! ret:[%d]", ret);
			goto exit_func;
		}

		int orientation = 0;
		if (composer_util_image_get_rotation_for_jpeg_image(src, &orientation)) {
			debug_log("==> orientation: (%d)", orientation);

			unsigned char *rotated_buffer = NULL;
			int rotated_width = 0, rotated_height = 0;
			if (composer_util_image_rotate_jpeg_image_from_memory(jpeg_image_buffer, width, height, size, orientation, &rotated_buffer, &rotated_width, &rotated_height)) {
				FREE(jpeg_image_buffer);
				jpeg_image_buffer = rotated_buffer;
				width = rotated_width;
				height = rotated_height;
			}
		}

		ret = image_util_encode_jpeg(jpeg_image_buffer, width, height, IMAGE_UTIL_COLORSPACE_RGB888, quality, dst);
		if (ret != IMAGE_UTIL_ERROR_NONE) {
			debug_error("image_util_encode_jpeg() failed! ret:[%d]", ret);
			FREE(jpeg_image_buffer);
			goto exit_func;
		}
		FREE(jpeg_image_buffer);
	} else if (!g_strcmp0(mime_type, "image/png")) {
		int i = 0;
		int color_type = 0;
		int bit_depth = 0;
		int interlace_type = 0;
		int channels = 0;
		image_util_colorspace_e colorspace = IMAGE_UTIL_COLORSPACE_RGB888;
		png_uint_32 width = 0;
		png_uint_32 height = 0;
		png_bytep *src_row_ptrs = NULL;

		Eina_Bool ret = __composer_util_image_read_png_image(src, &src_row_ptrs, &width, &height, &bit_depth, &color_type, &interlace_type, quality, &channels, &colorspace);
		if (!ret) {
			debug_error("__composer_util_image_read_png_image() failed!");
			for (i = 0; i < height; i++) {
				free(src_row_ptrs[i]);
			}
			FREE(src_row_ptrs);
			goto exit_func;
		}

		unsigned char *srcbuf = (unsigned char *)malloc(height * width * channels);
		unsigned int ctr = 0;
		for (i = 0; i < height; ++i) {
			memcpy((srcbuf + ctr), src_row_ptrs[i], (width * channels));
			ctr += width * channels;
		}

		for (i = 0; i < height; i++) {
			free(src_row_ptrs[i]);
		}
		FREE(src_row_ptrs);

		double q = 1;
		if (quality < 100) {
			if (quality == 70) {
				q = 0.75;
			} else if ((quality == 30) || (quality == 50)) {
				q = 0.5;
			} else if (quality == 10) {
				q = 0.25;
			}
		}

		int dst_w = (int)(width * q);
		int dst_h = (int)(height * q);
		unsigned char *dstbuf = (unsigned char *)malloc(height * (width * channels));

		ret = __composer_image_util_resize(dstbuf, &dst_w, &dst_h, srcbuf, width, height, colorspace);
		if (ret != IMAGE_UTIL_ERROR_NONE) {
			debug_error("__composer_image_util_resize() failed! ret:[%d]", ret);
			FREE(srcbuf);
			FREE(dstbuf);
			goto exit_func;
		}
		ctr = 0;
		png_bytep *dst_row_ptrs = calloc(dst_h, sizeof(png_bytep));
		for (i = 0; i < dst_h; ++i) {
			dst_row_ptrs[i] = malloc(dst_w * channels);
			memcpy(dst_row_ptrs[i], (dstbuf + ctr), (dst_w * channels));
			ctr += (dst_w * channels);
		}

		FREE(srcbuf);
		FREE(dstbuf);

		ret = __composer_util_image_write_png_image(dst, dst_row_ptrs, dst_w, dst_h, bit_depth, color_type, interlace_type);
		if (!ret) {
			debug_error("__composer_util_image_write_png_image() failed!");

			for (i = 0; i < dst_h; i++) {
				free(dst_row_ptrs[i]);
			}
			FREE(dst_row_ptrs);

			goto exit_func;
		}

		for (i = 0; i < dst_h; i++) {
			free(dst_row_ptrs[i]);
		}
		FREE(dst_row_ptrs);
	}

	if (!email_file_exists(dst)) {
		debug_secure_error("email_file_exists(%s) failed!", dst);
		goto exit_func;
	}

	err = EINA_TRUE;

exit_func:
	FREE(mime_type);

	debug_leave();
	return err;
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

static int __composer_image_util_resize(unsigned char *dest, const int *dest_width , const int *dest_height, const unsigned char *src,
								const int src_w, const int src_h, const image_util_colorspace_e colorspace)
{
	debug_enter();

	if (!dest || !dest_width || !dest_height || !src) {
		debug_leave();
		return IMAGE_UTIL_ERROR_INVALID_PARAMETER;
	}

	int dest_w = *dest_width;
	int dest_h = *dest_height;

	if ((IMAGE_UTIL_COLORSPACE_RGB888 != colorspace && IMAGE_UTIL_COLORSPACE_RGBA8888 != colorspace)
			|| src_w <= 0 || src_h <= 0 || dest_w <= 0 || dest_h <= 0) {
		debug_leave();
		return IMAGE_UTIL_ERROR_INVALID_PARAMETER;
	}

	const unsigned int bpp = (IMAGE_UTIL_COLORSPACE_RGBA8888 == colorspace ? RGBA_BPP : RGB_BPP);
	const unsigned int src_stride = bpp * src_w;
	const unsigned int dest_stride = bpp * dest_w;
	float coef = 0.0f;
	float c1, c2, c3, c4;
	c1 = c2 = c3 = c4 = 0.0f;
	u_int32_t red, green, blue, alpha;
	red = green = blue = alpha = 0;
	int x = 0, y = 0;
	const float coef_x = (float) (src_w) / (float) (dest_w);
	const float coef_y = (float) (src_h) / (float) (dest_h);
	const float add_x = 0.5f * coef_x - 0.5f;
	const float add_y = 0.5f * coef_y - 0.5f;
	transform *transform_x = NULL, *transform_y = NULL;
	transform_x = calloc(dest_w, sizeof(transform));
	if (!transform_x) {
		debug_leave();
		return IMAGE_UTIL_ERROR_OUT_OF_MEMORY;
	}
	transform_y = calloc(dest_h, sizeof(transform));
	if (!transform_y) {
		free(transform_x);
		debug_leave();
		return IMAGE_UTIL_ERROR_OUT_OF_MEMORY;
	}

	for (x = 0; x < dest_w; ++x) {
		coef = x * coef_x + add_x;
		transform_x[x].index = (int)coef;
		transform_x[x].coef = 1 - coef + transform_x[x].index;
	}
	if (transform_x[0].index < 0) {
		transform_x[0].index = 0;
		transform_x[0].coef = 1.0f;
	}
	if (transform_x[dest_w - 1].index >= src_w - 2) {
		transform_x[dest_w - 1].index = src_w - 2;
		transform_x[dest_w - 1].coef = 0.0f;
	}

	for (y = 0; y < dest_h; ++y) {
		coef = y * coef_y + add_y;
		transform_y[y].index = (int)coef;
		transform_y[y].coef = 1 - coef + transform_y[y].index;
	}
	if (transform_y[0].index < 0) {
		transform_y[0].index = 0;
		transform_y[0].coef = 1.0f;
	}
	if (transform_y[dest_h - 1].index >= src_h - 2) {
		transform_y[dest_h - 1].index = src_h - 2;
		transform_y[dest_h - 1].coef = 0.0f;
	}

	if (colorspace == IMAGE_UTIL_COLORSPACE_RGBA8888) {
		for (y = 0; y < dest_h; ++y) {
			const transform t_y = transform_y[y];
			rgba8888 * const dest_row = (rgba8888 *)(dest + y * dest_stride);
			const rgba8888 * const src_row_1 = (rgba8888 *)(src + t_y.index * src_stride);
			const rgba8888 * const src_row_2 = (rgba8888 *)(src + (t_y.index + 1) * src_stride);

			for (x = 0; x < dest_w; ++x) {
				const transform t_x = transform_x[x];
				const rgba8888 pixel1 = src_row_1[t_x.index];
				const rgba8888 pixel2 = src_row_1[t_x.index + 1];
				const rgba8888 pixel3 = src_row_2[t_x.index];
				const rgba8888 pixel4 = src_row_2[t_x.index + 1];
				c1 = t_x.coef * t_y.coef;
				c2 = (1 - t_x.coef) * t_y.coef;
				c3 = t_x.coef * (1 - t_y.coef);
				c4 = (1 - t_x.coef) * (1 - t_y.coef);
				red = pixel1.r * c1 + pixel2.r * c2 + pixel3.r * c3
						+ pixel4.r * c4;
				green = pixel1.g * c1 + pixel2.g * c2 + pixel3.g * c3
						+ pixel4.g * c4;
				blue = pixel1.b * c1 + pixel2.b * c2 + pixel3.b * c3
						+ pixel4.b * c4;
				alpha = pixel1.a * c1 + pixel2.a * c2 + pixel3.a * c3
						+ pixel4.a * c4;
				dest_row[x].r = red;
				dest_row[x].g = green;
				dest_row[x].b = blue;
				dest_row[x].a = alpha;
			}
		}
	} else {
		for (y = 0; y < dest_h; ++y) {
			const transform t_y = transform_y[y];
			rgb888 * const dest_row = (rgb888 *)(dest + y * dest_stride);
			const rgb888 * const src_row_1 = (rgb888 *)(src + t_y.index * src_stride);
			const rgb888 * const src_row_2 = (rgb888 *)(src + (t_y.index + 1) * src_stride);

			for (x = 0; x < dest_w; ++x) {
				const transform t_x = transform_x[x];
				const rgb888 pixel1 = src_row_1[t_x.index];
				const rgb888 pixel2 = src_row_1[t_x.index + 1];
				const rgb888 pixel3 = src_row_2[t_x.index];
				const rgb888 pixel4 = src_row_2[t_x.index + 1];
				c1 = t_x.coef * t_y.coef;
				c2 = (1 - t_x.coef) * t_y.coef;
				c3 = t_x.coef * (1 - t_y.coef);
				c4 = (1 - t_x.coef) * (1 - t_y.coef);
				red = pixel1.r * c1 + pixel2.r * c2 + pixel3.r * c3
						+ pixel4.r * c4;
				green = pixel1.g * c1 + pixel2.g * c2 + pixel3.g * c3
						+ pixel4.g * c4;
				blue = pixel1.b * c1 + pixel2.b * c2 + pixel3.b * c3
						+ pixel4.b * c4;

				dest_row[x].r = red;
				dest_row[x].g = green;
				dest_row[x].b = blue;
			}
		}
	}

	free(transform_x);
	free(transform_y);
	debug_leave();

	return IMAGE_UTIL_ERROR_NONE;
}

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
					dest_row[x] = *(rgb888 *)((uchar *)src_col - x * src_stride);
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
				const rgb888 *const src_row_rev = (rgb888 *) ((uchar *)src_row_rev_0 - y * src_stride);
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
					dest_row[x] = *(rgb888 *)((uchar *)src_col_rev + x * src_stride);
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

