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

#ifndef __EMAIL_COLOR_BOX__
#define __EMAIL_COLOR_BOX__

#include "email-common-types.h"

/*
 * In order to use this functionality first create background
 * under the scroller object: "core_theme_bg_01.png".
 * Size of the background should match the size of the scroller.
 *
 * Second, each item of the box should be a layout with "email.swallow.bg" part.
 * If the object is not a elm layout or has no such part it will be skipped.
 *
 */

EMAIL_API Evas_Object *email_color_box_add(Evas_Object *scroller);

#endif /* __EMAIL_COLOR_BOX__ */
