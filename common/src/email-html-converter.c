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

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <glib.h>
#include <glib/gprintf.h>

#include "email-common-types.h"
#include "email-debug.h"
#include "email-utils.h"
#include "email-html-converter.h"


static const gchar *_get_html_tag(const gchar char_val)
{
	const gchar *tag = NULL;

	switch (char_val) {
	case '\n':
		tag = "<br>";
		break;
	case '\r':
		tag = "";	/* skip. */
		break;
	case '<':
		tag = "&lt;";
		break;
	case '>':
		tag = "&gt;";
		break;
	case '&':
		tag = "&amp;";
		break;
	default:
		break;
	}
	return tag;
}

/* Exported API.
 */
/* g_free(). */
EMAIL_API gchar *email_get_parse_plain_text(const gchar *text, gint size)
{
	debug_enter();
	RETURN_VAL_IF_FAIL(STR_VALID(text), NULL);
	RETURN_VAL_IF_FAIL(size > 0, NULL);

	gchar *html = NULL;
	gchar *buff = NULL;
	gchar *temp = NULL;
	guint i = 0;
	guint old_offset = 0, offset = 0;

	for (i = 0; i < size; ++i) {
		/* Get html tag. */
		const gchar *tag = _get_html_tag(text[i]);

		/* If need, convert text to html format. */
		if (tag) {
			temp = html;

			if (i > old_offset) {
				offset = i - old_offset;
				buff = g_strndup(&text[old_offset], offset);

				if (temp) {
					html = g_strconcat(temp, buff, tag, NULL);
				} else {
					html = g_strconcat(buff, tag, NULL);
				}

				if (buff) {
					g_free(buff);
					buff = NULL;
				}
			} else {
				if (temp) {
					html = g_strconcat(temp, tag, NULL);
				} else {
					html = g_strconcat(tag, NULL);
				}
			}

			if (temp) {
				g_free(temp);
				temp = NULL;
			}

			old_offset = i + 1;
		}
	}

	/* If not terminated. */
	if (old_offset < size) {
		if (html) {
			temp = html;
			buff = g_strndup(&text[old_offset], size - old_offset);
			html = g_strconcat(temp, buff, NULL);
			if (buff) {
				g_free(buff);
			}
			g_free(temp);
		}
	}

	debug_leave();
	return html ? html : g_strndup(text, size);
}

/* EOF */
