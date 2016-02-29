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

#ifndef __EMAIL_COMPOSER_LAUNCHER_H__
#define __EMAIL_COMPOSER_LAUNCHER_H__

#include "email-composer.h"

/**
 * @brief Provides preview attachment
 *
 * @param[in]	view		Email composer data
 * @param[in]	uri		Physical path
 *
 */
void composer_launcher_preview_attachment(EmailComposerView *view, const char *uri);

/**
 * @brief Launch contact application and pick contacts
 *
 * @param[in]	view		Email composer data
 *
 */
void composer_launcher_pick_contacts(EmailComposerView *view);

/**
 * @brief Launch contact application and update contact
 *
 * @param[in]	view		Email composer data
 *
 */
void composer_launcher_update_contact(EmailComposerView *view);

/**
 * @brief Launch contact application and add contact
 *
 * @param[in]	view		Email composer data
 *
 */
void composer_launcher_add_contact(EmailComposerView *view);

/**
 * @brief Launch setting application to provide storage settings
 *
 * @param[in]	view		Email composer data
 *
 */
void composer_launcher_launch_storage_settings(EmailComposerView *view);

#endif /* __EMAIL_COMPOSER_LAUNCHER_H__ */
