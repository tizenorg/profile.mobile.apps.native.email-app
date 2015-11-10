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

#ifndef _EMAIL_HTML_CONVERTER_H_
#define _EMAIL_HTML_CONVERTER_H_

#include <glib.h>
#include "email-common-types.h"

G_BEGIN_DECLS

/* g_free(). */
EMAIL_API gchar *email_get_parse_plain_text(const gchar *text, gint size);

G_END_DECLS
#endif	/* _EMAIL_HTML_CONVERTER_H_ */

/* EOF */
