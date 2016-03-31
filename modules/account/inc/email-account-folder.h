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

#ifndef __EMAIL_ACCOUNT_FOLDER_H_
#define __EMAIL_ACCOUNT_FOLDER_H_

#include "email-account.h"

void account_init_genlist_item_class_for_combined_folder_list(EmailAccountView *view);
void account_init_genlist_item_class_for_single_folder_list(EmailAccountView *view);
void account_init_genlist_item_class_for_account_view_list(EmailAccountView *view);

void account_create_folder_create_view(void *data);
void account_delete_folder_view(void *data);
void account_rename_folder_view(void *data);

int account_create_combined_folder_list(EmailAccountView *view);
int account_create_single_account_folder_list(EmailAccountView *view);

void account_folder_newfolder(void *data, Evas_Object *obj, void *event_info);

#endif	/* __EMAIL_ACCOUNT_FOLDER_H_ */

/* EOF */
