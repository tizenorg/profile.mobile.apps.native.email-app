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

#include "email-popup-utils.h"
#include "email-composer-rich-text-toolbar.h"
#include "email-composer-util.h"

#define DEFAULT_ALFA_CHANNEL_VALUE 255
#define MAX_COLOR_VALUE 255

#define RICHTEXT_TOOLBAR_SIDE_PADDING_WIDTH ELM_SCALE_SIZE(32)

#define RICHTEXT_TOOLBAR_DATA_NAME "toolbar_data"
#define BUTTON_COLOR_LINE_PART_NAME "colored_line"

#define JS_EXEC_COMMAND_PROTOTYPE "ExecCommand('%s', '%s');"
#define FONT_COLOR_COMMAND_NAME "fontColor"
#define BG_COLOR_COMMAND_NAME "backgroudColor"
#define FONT_SIZE_COMMAND_NAME "fontSize"

#define MED_BUFF_SIZE 64

#define RICHTEXT_EDJ_NAME email_get_composer_theme_path()

static Elm_Genlist_Item_Class _font_size_itc;

typedef struct {
	int size_range_low;
	int size;
	int size_range_high;
	int index;
} FonSizesType;

static FonSizesType _rich_text_font_sizes[] = {
		{ 1, 10, 10, 1 },
		{ 11, 14, 14, 2 },
		{ 15, 16, 16, 3 },
		{ 17, 18, 19, 4 },
		{ 20, 24, 26, 5 },
		{ 27, 32, 38, 6 },
		{ 39, 48, 50, 7 }
};

static char *_rich_text_button_list[RICH_TEXT_TYPE_COUNT] = {
		"ec/toolbar/button/insert_image",
		"ec/toolbar/button/fontsize",
		"ec/toolbar/button/bold",
		"ec/toolbar/button/italic",
		"ec/toolbar/button/underline",
		"ec/toolbar/button/fontcolor",
		"ec/toolbar/button/fontbgcolor",
		"ec/toolbar/button/ordered_list",
		"ec/toolbar/button/unordered_list"
};

static email_rgba_t _default_font_colors[] = {
		{0, 0, 0, 255},
		{255, 255, 255, 255}
};

typedef enum {
	BOX_PADDING_OUTER,
	BOX_PADDING_INNER
} BoxItemPaddingType;

#define TO_STR(str) #str

const char *_restore_selection		= TO_STR(RestoreCurrentSelection(););
const char *_update_cur_selection	= TO_STR(BackupCurrentSelection(););
const char *_get_font_properties	= TO_STR(GetCurrentFontStyle(););

const char *_set_bold_state			= TO_STR(ExecCommand("bold", null););
const char *_set_italic_state		= TO_STR(ExecCommand("italic", null););
const char *_set_underline_state	= TO_STR(ExecCommand("underline", null););
const char *_set_ordered_list_state	= TO_STR(ExecCommand("orderedList", null););
const char *_set_unordered_list_state = TO_STR(ExecCommand("unorderedList", null););

typedef void (*button_click_cb) (void *data, Evas_Object *obj, void *event_info);

static RichTextTypes _get_pressed_button_type(EmailComposerView *view, Evas_Object *button);

static char *_font_size_gl_text_get(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_font_size_gl_content_get(void *data, Evas_Object *obj, const char *part);
static void _font_size_gl_sel(void *data, Evas_Object *obj, void *event_info);
static void _font_size_popup_cancel_cb(void *data, Evas_Object *obj, void *event_info);
static void _button_clicked_cb(void *data, Evas_Object *obj, void *event_info);

static Evas_Object *_create_toolbar_layout_base(Evas_Object *parent);
static void _delete_popup(EmailComposerView *view);

/* Color selector popup */
static void _create_colorselector_popup(EmailComposerView *view, RichTextTypes type);
static Evas_Object *_create_colorselector_popup_content(EmailComposerView *view, email_rgba_t rich_text_color, Evas_Object *parent, RichTextTypes type);
static Evas_Object *_create_colorselector(Evas_Object *layout, EmailComposerView *view, RichTextTypes type);
static void _font_color_response_cb(void *data, Evas_Object *obj, void *event_info);
static void _bg_color_response_cb(void *data, Evas_Object *obj, void *event_info);
static void _colorselector_popup_cancel_cb(void *data, Evas_Object *obj, void *event_info);

static void _save_current_selection(EmailComposerView *view);
static void _restore_saved_selection(EmailComposerView *view);
static void _update_button_state(EmailComposerView *view, RichTextTypes type);
static void _init_toolbar_buttons_state(EmailComposerView *view);

static void _create_font_size_popup(EmailComposerView *view);

static void _update_all_buttons_state(EmailComposerView *view);
static void _update_property_button_state(EmailComposerView *view, RichTextTypes type);
static void _update_color_button_state(EmailComposerView *view, RichTextTypes type);
static void _insert_inline_image(EmailComposerView *view);
static void _create_option_selection_popup(EmailComposerView *view, RichTextTypes type);
static void _update_color_button_after_color_pick(EmailComposerView *view, RichTextTypes type);
static Evas_Object *_create_button(EmailComposerView *view, Evas_Object *parent, RichTextTypes type);

static Evas_Object *_create_color_image_layout(EmailComposerView *view, RichTextTypes type, const char *group_name);
static void _insert_box_padding_item(Evas_Object *box, BoxItemPaddingType type);

static void _font_properties_script_exec_cb(Evas_Object *obj, const char *result_value, void *user_data);

static email_string_t EMAIL_COMPOSER_HEADER_TEXT_BACKGROUND_COLOUR = { PACKAGE, "IDS_EMAIL_HEADER_TEXT_BACKGROUND_COLOUR" };
static email_string_t EMAIL_COMPOSER_HEADER_TEXT_COLOUR = { PACKAGE, "IDS_EMAIL_BODY_FONT_COLOUR" };
static email_string_t EMAIL_COMPOSER_OPT_FONT_SIZE = { PACKAGE, "IDS_EMAIL_BODY_FONT_SIZE" };
static email_string_t EMAIL_COMPOSER_STRING_NULL = { NULL, NULL };
static email_string_t EMAIL_COMPOSER_STRING_CANCEL = { PACKAGE, "IDS_EMAIL_BUTTON_CANCEL" };
static email_string_t EMAIL_COMPOSER_SK_OK = { PACKAGE, "IDS_EMAIL_BUTTON_OK" };

static RichTextTypes _get_pressed_button_type(EmailComposerView *view, Evas_Object *button)
{
	debug_enter();

	int i = 0;
	for (; i < RICH_TEXT_TYPE_COUNT; i++) {
		if (view->richtext_button_list[i].button == button) {
			return i;
		}
	}

	debug_leave();

	return RICH_TEXT_TYPE_NONE;
}

static char *_font_size_gl_text_get(void *data, Evas_Object *obj, const char *part)
{
	debug_enter();

	retvm_if(data == NULL, NULL, "data is NULL!");
	retvm_if(part == NULL, NULL, "part is NULL!");

	FonSizesType *fontsize = (FonSizesType *)data;

	if (g_strcmp0(part, "elm.text")) {
		debug_leave();
		return NULL;
	}

	char buf[MED_BUFF_SIZE] = { 0 };
	snprintf(buf, sizeof(buf), "<font_size=%d>%dpx</font_size>", fontsize->size, fontsize->size);

	debug_leave();

	return strdup(buf);
}

static Evas_Object *_font_size_gl_content_get(void *data, Evas_Object *obj, const char *part)
{
	debug_enter();

	retvm_if(data == NULL, NULL, "data is NULL!");
	retvm_if(part == NULL, NULL, "part is NULL!");

	FonSizesType *fontSize = (FonSizesType *)data;
	EmailComposerView *view = (EmailComposerView *)evas_object_data_get(obj, RICHTEXT_TOOLBAR_DATA_NAME);
	retvm_if(view == NULL, NULL, "rt_data is NULL!");
	Evas_Object *radio = NULL;

	if (!g_strcmp0(part, "elm.swallow.icon")) {
		radio = elm_radio_add(obj);
		elm_radio_group_add(radio, view->richtext_font_size_radio_group);
		elm_radio_state_value_set(radio, fontSize->index);

		if ((view->richtext_font_size_pixels >= fontSize->size_range_low) &&
				(view->richtext_font_size_pixels <= fontSize->size_range_high)) {
			elm_radio_value_set(radio, fontSize->index);
		}
	}
	debug_leave();

	return radio;
}

static void _font_size_gl_sel(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(data == NULL, "data is NULL!");
	retm_if(event_info == NULL, "event_info is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;
	Elm_Object_Item *item = (Elm_Object_Item *)event_info;
	FonSizesType *fontsize = elm_object_item_data_get(item);

	char font_string[MED_BUFF_SIZE] = { 0 };
	snprintf(font_string, sizeof(font_string), "%d", fontsize->index);
	view->richtext_font_size_pixels = fontsize->size;

	_restore_saved_selection(view);

	char script_string[MED_BUFF_SIZE] = { 0 };
	snprintf(script_string, MED_BUFF_SIZE, JS_EXEC_COMMAND_PROTOTYPE, FONT_SIZE_COMMAND_NAME, font_string);

	if (ewk_view_script_execute(view->ewk_view, script_string, NULL, NULL) == EINA_FALSE) {
		debug_error("ewk_view_script_execute failed!");
	}

	_delete_popup(data);

	debug_leave();
}

static void _delete_popup(EmailComposerView *view)
{
	debug_enter();

	retm_if(!view, "Invalid parameter: rt_data is NULL!");

	DELETE_EVAS_OBJECT(view->composer_popup);
	view->composer_popup = NULL;
	view->richtext_font_size_radio_group = NULL;
	view->richtext_colorselector = NULL;

	debug_leave();
}

static void _font_size_popup_cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	EmailComposerView *view = (EmailComposerView *)data;

	_restore_saved_selection(view);

	_delete_popup(view);

	debug_leave();
}

static Evas_Object *_create_colorselector(Evas_Object *box, EmailComposerView *view, RichTextTypes type)
{
	debug_enter();

	retvm_if((type != RICH_TEXT_TYPE_FONT_COLOR) && (type != RICH_TEXT_TYPE_BACKGROUND_COLOR), NULL, "Invalid parameter: type is wrong! [%d]", type);

	const Eina_List *color_list = NULL, *cur_list = NULL;
	Elm_Object_Item *cur_it = NULL;
	email_rgba_t *rgb = NULL;
	email_rgba_t rgb_it;

	Evas_Object *colorselector = elm_colorselector_add(box);
	Elm_Colorselector_Mode mode = ELM_COLORSELECTOR_PALETTE;
	elm_colorselector_mode_set(colorselector, mode);
	evas_object_size_hint_fill_set(colorselector, 0.5, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(colorselector, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(colorselector);

	if (type == RICH_TEXT_TYPE_FONT_COLOR) {
		rgb = &view->richtext_font_color;
	} else if (type == RICH_TEXT_TYPE_BACKGROUND_COLOR) {
		rgb = &view->richtext_bg_color;
	}

	color_list = elm_colorselector_palette_items_get(colorselector);
	Elm_Object_Item *colorselector_it_default = eina_list_data_get(color_list);
	Elm_Object_Item *colorselector_it_sel = NULL;

	EINA_LIST_FOREACH(color_list, cur_list, cur_it) {
		elm_colorselector_palette_item_color_get(cur_it, &rgb_it.red, &rgb_it.green, &rgb_it.blue, &rgb_it.alpha);
		if ((rgb_it.red == rgb->red) &&
				(rgb_it.green == rgb->green) &&
				(rgb_it.blue == rgb->blue) &&
				(rgb_it.alpha == rgb->alpha)) {
			colorselector_it_sel = cur_it;
			break;
		}
	}

	if (!colorselector_it_sel) {
		colorselector_it_sel = colorselector_it_default;
	}
	elm_colorselector_palette_item_selected_set(colorselector_it_sel, EINA_TRUE);

	debug_leave();
	return colorselector;
}

static Evas_Object *_create_colorselector_popup_content(EmailComposerView *view, email_rgba_t rich_text_color, Evas_Object *parent, RichTextTypes type)
{
	debug_enter();

	retvm_if(view == NULL, NULL, "view is NULL!");
	retvm_if(parent == NULL, NULL, "parent is NULL!");

	Evas_Object *box = elm_box_add(parent);
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(box);

	view->richtext_colorselector = _create_colorselector(box, view, type);
	elm_box_pack_end(box, view->richtext_colorselector);

	debug_leave();
	return box;
}

static void _update_color_button_after_color_pick(EmailComposerView *view, RichTextTypes type)
{
	debug_enter();

	email_rgba_t *color_cur = NULL;
	char *operation = NULL;

	if (type == RICH_TEXT_TYPE_FONT_COLOR) {
		color_cur = &view->richtext_font_color;
		operation = FONT_COLOR_COMMAND_NAME;
	} else if (type == RICH_TEXT_TYPE_BACKGROUND_COLOR) {
		color_cur = &view->richtext_bg_color;
		operation = BG_COLOR_COMMAND_NAME;
	} else {
		debug_error("wrong type [%d]", type);
		return;
	}

	elm_colorselector_color_get(view->richtext_colorselector,
			&color_cur->red, &color_cur->green, &color_cur->blue, &color_cur->alpha);

	char szColor[MED_BUFF_SIZE] = { 0 };
	if (email_util_get_rgb_string(szColor, sizeof(szColor),
			color_cur->red, color_cur->green, color_cur->blue)) {

		char script_string[MED_BUFF_SIZE] = { 0 };
		snprintf(script_string, MED_BUFF_SIZE, JS_EXEC_COMMAND_PROTOTYPE, operation, szColor);

		if (ewk_view_script_execute(view->ewk_view, script_string, NULL, NULL) == EINA_FALSE) {
			debug_error("ewk_view_script_execute failed!");
		}

		_update_color_button_state(view, type);
	} else {
		debug_error("common_util_rgb_to_string() failed!");
	}

	debug_leave();
}

static void _font_color_response_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(data == NULL, "data is NULL!");
	retm_if(obj == NULL, "obj is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	_restore_saved_selection(view);

	_update_color_button_after_color_pick(view, RICH_TEXT_TYPE_FONT_COLOR);

	_delete_popup(view);

	debug_leave();
}

static void _colorselector_popup_cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();
	retm_if(data == NULL, "data is NULL!");
	retm_if(obj == NULL, "obj is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	_restore_saved_selection(view);

	_delete_popup(view);

	debug_leave();
}

static void _save_current_selection(EmailComposerView *view)
{
	if (ewk_view_script_execute(view->ewk_view, _update_cur_selection, NULL, NULL) == EINA_FALSE) {
		debug_error("ewk_view_script_execute failed!");
	}
}

static void _restore_saved_selection(EmailComposerView *view)
{
	if (ewk_view_script_execute(view->ewk_view, _restore_selection, NULL, NULL) == EINA_FALSE) {
		debug_error("ewk_view_script_execute failed!");
	}
}

static void _update_button_state(EmailComposerView *view, RichTextTypes type)
{
	const char *command = NULL;
	switch (type) {
	case RICH_TEXT_TYPE_BOLD:
		command = _set_bold_state;
		break;
	case RICH_TEXT_TYPE_ITALIC:
		command = _set_italic_state;
		break;
	case RICH_TEXT_TYPE_UNDERLINE:
		command = _set_underline_state;
		break;
	case RICH_TEXT_TYPE_ORDERED_LIST:
		command = _set_ordered_list_state;
		break;
	case RICH_TEXT_TYPE_UNORDERED_LIST:
		command = _set_unordered_list_state;
		break;
	default:
		debug_error("unsupported button type");
		return;
		break;
	}

	if (ewk_view_script_execute(view->ewk_view, command, NULL, NULL) == EINA_FALSE) {
		debug_error("ewk_view_script_execute failed!");
	}
}

static void _init_toolbar_buttons_state(EmailComposerView *view)
{
	if (ewk_view_script_execute(view->ewk_view,
			_get_font_properties,
			_font_properties_script_exec_cb,
			view) == EINA_FALSE) {
		debug_error("ewk_view_script_execute failed!");
	}
}

static void _create_font_size_popup(EmailComposerView *view)
{
	view->composer_popup = common_util_create_popup(view->base.module->win,
			EMAIL_COMPOSER_OPT_FONT_SIZE,
			NULL,
			EMAIL_COMPOSER_STRING_NULL,
			NULL,
			EMAIL_COMPOSER_STRING_NULL,
			NULL,
			EMAIL_COMPOSER_STRING_NULL,
			_font_size_popup_cancel_cb,
			EINA_TRUE,
			view);

	_font_size_itc.item_style = "type1";
	_font_size_itc.func.text_get = _font_size_gl_text_get;
	_font_size_itc.func.content_get = _font_size_gl_content_get;
	_font_size_itc.func.state_get = NULL;
	_font_size_itc.func.del = NULL;

	Evas_Object *genlist = elm_genlist_add(view->composer_popup);
	evas_object_data_set(genlist, RICHTEXT_TOOLBAR_DATA_NAME, view);
	evas_object_size_hint_weight_set(genlist, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(genlist, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_genlist_homogeneous_set(genlist, EINA_TRUE);

	int index = 0;
	for (; index < sizeof(_rich_text_font_sizes) / sizeof(_rich_text_font_sizes[0]); index++) {
		FonSizesType *fontSize = &_rich_text_font_sizes[index];
		Elm_Object_Item *it = elm_genlist_item_append(genlist, &_font_size_itc, (void *)fontSize, NULL, ELM_GENLIST_ITEM_NONE, _font_size_gl_sel, view);

		bool found_font = (view->richtext_font_size_pixels >= fontSize->size_range_low) && (view->richtext_font_size_pixels <= fontSize->size_range_high);
		if (found_font) {
			elm_genlist_item_show(it, ELM_GENLIST_ITEM_SCROLLTO_IN);
		}
	}

	Evas_Object *radio = elm_radio_add(genlist);
	elm_radio_state_value_set(radio, 0);
	view->richtext_font_size_radio_group = radio;

	common_util_popup_display_genlist(view->composer_popup, genlist, EINA_FALSE, COMMON_POPUP_ITEM_1_LINE_HEIGHT, index);

	debug_leave();
}

static void _create_colorselector_popup(EmailComposerView *view, RichTextTypes type)
{
	debug_enter();

	retm_if(view == NULL, "view is NULL!");

	Evas_Object *content = NULL;
	email_rgba_t *color_cur = NULL;
	button_click_cb button_ok_cb = NULL;
	email_string_t *string_type = NULL;

	if (type == RICH_TEXT_TYPE_FONT_COLOR) {
		color_cur = &view->richtext_font_color;
		button_ok_cb = _font_color_response_cb;
		string_type = &EMAIL_COMPOSER_HEADER_TEXT_COLOUR;
	} else if (type == RICH_TEXT_TYPE_BACKGROUND_COLOR) {
		color_cur = &view->richtext_bg_color;
		button_ok_cb = _bg_color_response_cb;
		string_type = &EMAIL_COMPOSER_HEADER_TEXT_BACKGROUND_COLOUR;
	} else {
		debug_error("invalid type");
		return;
	}

	view->composer_popup = common_util_create_popup(view->base.module->win,
			*string_type,
			_colorselector_popup_cancel_cb,
			EMAIL_COMPOSER_STRING_CANCEL,
			button_ok_cb,
			EMAIL_COMPOSER_SK_OK,
			NULL,
			EMAIL_COMPOSER_STRING_NULL,
			_colorselector_popup_cancel_cb,
			EINA_TRUE,
			view);

	content = _create_colorselector_popup_content(view, *color_cur, view->composer_popup, type);
	elm_object_content_set(view->composer_popup, content);

	debug_leave();
}

static void _bg_color_response_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(data == NULL, "data is NULL!");
	retm_if(obj == NULL, "obj is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;

	_restore_saved_selection(view);

	_update_color_button_after_color_pick(view, RICH_TEXT_TYPE_BACKGROUND_COLOR);

	_delete_popup(view);

	debug_leave();
}

static void _button_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	debug_enter();

	retm_if(data == NULL, "data is NULL!");
	retm_if(obj == NULL, "obj is NULL!");

	EmailComposerView *view = (EmailComposerView *)data;
	RichTextTypes type = _get_pressed_button_type(view, obj);

	switch (type) {
	case RICH_TEXT_TYPE_INSERT_IMAGE:
		_insert_inline_image(view);
		break;
	case RICH_TEXT_TYPE_FONT_SIZE:
	case RICH_TEXT_TYPE_FONT_COLOR:
	case RICH_TEXT_TYPE_BACKGROUND_COLOR:
		_create_option_selection_popup(view, type);
		break;
	case RICH_TEXT_TYPE_UNDERLINE:
	case RICH_TEXT_TYPE_BOLD:
	case RICH_TEXT_TYPE_ITALIC:
	case RICH_TEXT_TYPE_ORDERED_LIST:
	case RICH_TEXT_TYPE_UNORDERED_LIST:
		_update_button_state(view, type);
		break;
	default:
		debug_log("Unsupported operation: %d", type);
		break;
	}

	debug_leave();
}

static Evas_Object *_create_toolbar_layout_base(Evas_Object *parent)
{
	retvm_if(parent == NULL, NULL, "parent is NULL!");

	Evas_Object *ly = elm_layout_add(parent);

	elm_layout_file_set(ly, RICHTEXT_EDJ_NAME, "ec/rt/layout/base");
	evas_object_size_hint_weight_set(ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ly, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(ly);

	return ly;
}

static Evas_Object *_create_toolbar_layout_base_box(Evas_Object *parent)
{
	retvm_if(parent == NULL, NULL, "parent is NULL!");

	Evas_Object *container_box = elm_box_add(parent);
	evas_object_size_hint_align_set(container_box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(container_box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_box_horizontal_set(container_box, EINA_TRUE);
	elm_object_part_content_set(parent, "ec/rt/layout/base/toolbar_box", container_box);
	evas_object_show(container_box);

	return container_box;
}

static void _update_property_button_state(EmailComposerView *view, RichTextTypes type)
{
	Evas_Object *button = view->richtext_button_list[type].button;
	Eina_Bool state = view->richtext_button_list[type].state;

	if (state == EINA_TRUE) {
		elm_object_signal_emit(button, "button_press", "");
	} else {
		elm_object_signal_emit(button, "button_unpress", "");
	}
}

static void _update_color_button_state(EmailComposerView *view, RichTextTypes type)
{
	Evas_Object *button = NULL;
	email_rgba_t *color = NULL;

	if (type == RICH_TEXT_TYPE_FONT_COLOR) {
		button = view->richtext_button_list[RICH_TEXT_TYPE_FONT_COLOR].button;
		color = &view->richtext_font_color;
	} else if (type == RICH_TEXT_TYPE_BACKGROUND_COLOR) {
		button = view->richtext_button_list[RICH_TEXT_TYPE_BACKGROUND_COLOR].button;
		color = &view->richtext_bg_color;
	} else {
		debug_error("wrong type [%d]", type);
		return;
	}

	int r = 0, g = 0, b = 0;
	Evas_Object *select_color_img = elm_object_part_content_get(button, BUTTON_COLOR_LINE_PART_NAME);
	evas_object_color_get(select_color_img, &r, &g ,&b, NULL);

	if((r != MAX_COLOR_VALUE || g != MAX_COLOR_VALUE || b != MAX_COLOR_VALUE) &&
			(color->red == MAX_COLOR_VALUE && color->green == MAX_COLOR_VALUE  && color->blue == MAX_COLOR_VALUE)) {
		evas_object_del(elm_object_part_content_unset(button, BUTTON_COLOR_LINE_PART_NAME));
		select_color_img = _create_color_image_layout(view, type, EMAIL_IMAGE_COMPOSER_RTTB_BORDERED_COLOR_BAR);
	} else if ((r == MAX_COLOR_VALUE && g == MAX_COLOR_VALUE && b == MAX_COLOR_VALUE) &&
			(color->red != MAX_COLOR_VALUE || color->green != MAX_COLOR_VALUE || color->blue != MAX_COLOR_VALUE)) {
		evas_object_del(elm_object_part_content_unset(button, BUTTON_COLOR_LINE_PART_NAME));
		select_color_img = _create_color_image_layout(view, type, EMAIL_IMAGE_COMPOSER_RTTB_BORDERLESS_COLOR_BAR);
	}
	evas_object_color_set(select_color_img, color->red, color->green, color->blue, color->alpha);
}

static void _update_all_buttons_state(EmailComposerView *view)
{
	int i = RICH_TEXT_TYPE_BOLD;
	for (; i < RICH_TEXT_TYPE_COUNT; i++) {
		if (i == RICH_TEXT_TYPE_FONT_COLOR || i == RICH_TEXT_TYPE_BACKGROUND_COLOR) {
			_update_color_button_state(view, i);
		} else {
			_update_property_button_state(view, i);
		}
	}
}

static void _insert_inline_image(EmailComposerView *view)
{
	debug_enter();

	email_module_launch_attach_panel(view->base.module, EMAIL_APMT_IMAGES);
}

static void _create_option_selection_popup(EmailComposerView *view, RichTextTypes type)
{
	retm_if(!view, "view is NULL");
	_save_current_selection(view);

	switch (type) {
	case RICH_TEXT_TYPE_FONT_SIZE:
		_create_font_size_popup(view);
		break;
	case RICH_TEXT_TYPE_FONT_COLOR:
	case RICH_TEXT_TYPE_BACKGROUND_COLOR:
		_create_colorselector_popup(view, type);
		break;
	default:
		debug_error("not supported id");
		break;
	}
}

static Evas_Object *_create_button(EmailComposerView *view, Evas_Object *parent, RichTextTypes type)
{
	retvm_if(view == NULL, NULL, "view is NULL!");
	retvm_if(parent == NULL, NULL, "parent is NULL!");

	Evas_Object *btn = elm_button_add(parent);
	elm_object_style_set(btn, _rich_text_button_list[type]);
	elm_object_focus_allow_set(btn, EINA_FALSE);

	evas_object_smart_callback_add(btn, "clicked", _button_clicked_cb, view);
	evas_object_show(btn);

	view->richtext_button_list[type].button = btn;

	if (type == RICH_TEXT_TYPE_FONT_COLOR || type == RICH_TEXT_TYPE_BACKGROUND_COLOR) {

		email_rgba_t *color = NULL;
		switch (type) {
		case RICH_TEXT_TYPE_FONT_COLOR:
			color = &view->richtext_font_color;
			break;
		case RICH_TEXT_TYPE_BACKGROUND_COLOR:
			color = &view->richtext_bg_color;
			break;
		default:
			break;
		}

		const char *group_name = NULL;
		if (color->red == _default_font_colors[1].red &&
				color->green == _default_font_colors[1].green &&
				color->blue == _default_font_colors[1].blue) {
			group_name = EMAIL_IMAGE_COMPOSER_RTTB_BORDERED_COLOR_BAR;
		} else {
			group_name = EMAIL_IMAGE_COMPOSER_RTTB_BORDERLESS_COLOR_BAR;
		}
		Evas_Object *select_color_img = _create_color_image_layout(view, type, group_name);
		evas_object_color_set(select_color_img, color->red, color->green, color->blue, color->alpha);
	}

	return btn;
}

static Evas_Object *_create_color_image_layout(EmailComposerView *view, RichTextTypes type, const char *group_name)
{
	Evas_Object *layout = elm_layout_add(view->richtext_button_list[type].button);
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_fill_set(layout, 0.5, 0.5);
	elm_layout_file_set(layout, email_get_common_theme_path(), group_name);
	elm_object_part_content_set(view->richtext_button_list[type].button, BUTTON_COLOR_LINE_PART_NAME, layout);
	evas_object_show(layout);
	return layout;
}

static void _font_properties_script_exec_cb(Evas_Object *obj, const char *result_value, void *user_data)
{
	debug_enter();

	retm_if(user_data == NULL, "user_data is NULL!");
	retm_if(result_value == NULL, "javascript result is NULL");

	int res = 0;
	debug_log("result_value [%s]", result_value);

	sscanf(result_value, "%d", &res);
	if (res) {
		debug_log("javascript successfully executed");
	} else {
		debug_error("javascript failed");
	}

	debug_leave();
}

static void _insert_box_padding_item(Evas_Object *box, BoxItemPaddingType type)
{
	Evas_Object *rect = evas_object_rectangle_add(evas_object_evas_get(box));
	if (type == BOX_PADDING_OUTER) {
		evas_object_size_hint_min_set(rect, RICHTEXT_TOOLBAR_SIDE_PADDING_WIDTH, 0);
	} else {
		evas_object_size_hint_weight_set(rect, EVAS_HINT_EXPAND, 0.0);
	}
	evas_object_color_set(rect, TRANSPARENT);
	evas_object_show(rect);
	elm_box_pack_end(box, rect);
}

EMAIL_API Evas_Object *composer_rich_text_create_toolbar(EmailComposerView *view)
{
	debug_enter();

	retvm_if(view == NULL, NULL, "view is NULL!");

	Evas_Object *box = view->composer_box;

	Evas_Object *toolbar_layout_base = _create_toolbar_layout_base(box);
	Evas_Object *container_box = _create_toolbar_layout_base_box(toolbar_layout_base);

	view->richtext_is_disable = EINA_FALSE;
	view->richtext_font_color = _default_font_colors[0];
	view->richtext_bg_color = _default_font_colors[1];

	_insert_box_padding_item(container_box, BOX_PADDING_OUTER);

	int i = RICH_TEXT_TYPE_INSERT_IMAGE;
	for (; i < RICH_TEXT_TYPE_COUNT; i++) {
		Evas_Object *btn = _create_button(view, container_box, i);
		elm_box_pack_end(container_box, btn);

		if (i != RICH_TEXT_TYPE_COUNT - 1) {
			_insert_box_padding_item(container_box, BOX_PADDING_INNER);
		}
	}
	_insert_box_padding_item(container_box, BOX_PADDING_OUTER);

	_init_toolbar_buttons_state(view);

	elm_box_pack_end(box, toolbar_layout_base);
	evas_object_show(toolbar_layout_base);

	debug_leave();

	return toolbar_layout_base;
}

EMAIL_API void composer_rich_text_disable_set(EmailComposerView *view, Eina_Bool is_disable)
{
	debug_enter();
	retm_if(!view, "view is NULL");

	debug_log("CURRENT STATE [%s] <> INCOMING STATE [%s]",
			(view->richtext_is_disable) ? "TRUE" : "FALSE",
			(is_disable) ? "TRUE" : "FALSE");

	if (view->richtext_is_disable == is_disable) {
		debug_leave();
		return;
	}
	view->richtext_is_disable = is_disable;

	const char *button_signal = NULL;
	if (view->richtext_is_disable) {
		button_signal = "button_disable";
	} else {
		button_signal = "button_enable";
	}

	int i = RICH_TEXT_TYPE_INSERT_IMAGE;
	for (; i < RICH_TEXT_TYPE_COUNT; i++) {
		elm_object_disabled_set(view->richtext_button_list[i].button, is_disable);
		elm_object_signal_emit(view->richtext_button_list[i].button, button_signal, "");
	}

	if (!is_disable) {
		_update_all_buttons_state(view);
	}

	debug_leave();
}

EMAIL_API Eina_Bool composer_rich_text_disable_get(EmailComposerView *view)
{
	debug_enter();
	retvm_if(!view, EINA_FALSE, "view is NULL");

	debug_leave();
	return view->richtext_is_disable;
}

EMAIL_API void composer_rich_text_font_style_params_set(EmailComposerView *view, FontStyleParams *params)
{
	debug_enter();
	retm_if(!view, "view is NULL");
	retm_if(!params, "params is NULL");

	view->richtext_font_size_pixels = params->font_size;
	view->richtext_button_list[RICH_TEXT_TYPE_BOLD].state = params->is_bold;
	view->richtext_button_list[RICH_TEXT_TYPE_UNDERLINE].state = params->is_underline;
	view->richtext_button_list[RICH_TEXT_TYPE_ITALIC].state = params->is_italic;
	view->richtext_button_list[RICH_TEXT_TYPE_ORDERED_LIST].state = params->is_ordered_list;
	view->richtext_button_list[RICH_TEXT_TYPE_UNORDERED_LIST].state = params->is_unordered_list;

	view->richtext_font_color = params->font_color;
	view->richtext_bg_color = params->bg_color;

	view->richtext_font_color.alpha = DEFAULT_ALFA_CHANNEL_VALUE;
	view->richtext_bg_color.alpha = DEFAULT_ALFA_CHANNEL_VALUE;

	if (!view->richtext_is_disable && elm_object_focus_get(view->ewk_btn)) {
		_update_all_buttons_state(view);
	}

	debug_leave();
}
