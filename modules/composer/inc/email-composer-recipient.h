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

#ifndef __EMAIL_COMPOSER_RECIPIENT_H__
#define __EMAIL_COMPOSER_RECIPIENT_H__

typedef enum {
	MBE_VALIDATION_ERROR_NONE = 0,
	MBE_VALIDATION_ERROR_QUOTATIONS = 1, /**< this value is used on mbe filer cb to check whether some quotation marks exist in the string entered. */
	MBE_VALIDATION_ERROR_NO_ADDRESS = 2, /**< this value is used to display error string when sending email without any recipients. */
	MBE_VALIDATION_ERROR_DUPLICATE_ADDRESS = 3,
	MBE_VALIDATION_ERROR_MAX_NUMBER_REACHED = 4,
	MBE_VALIDATION_ERROR_INVALID_ADDRESS = 5,
} MBE_VALIDATION_ERROR;

/* email-composer-recipient.c */

/**
 * @brief Creates layout for recipient field in composer
 *
 * @param[in]	data		User data (Email composer data)
 * @param[in]	parent		Evas_Object that used as parent for recipient layout
 * @return Evas_Object with suitable layout, otherwise NULL
 *
 */
Evas_Object *composer_recipient_create_layout(void *data, Evas_Object *parent);

/**
 * @brief Updates recipient field with appropriate data
 *
 * @param[in]	data		User data (Email composer data)
 * @param[in]	parent		Evas_Object that used as parent for recipient layout
 *
 */
void composer_recipient_update_to_detail(void *data, Evas_Object *parent);

/**
 * @brief Updates cc field with appropriate data
 *
 * @param[in]	data		User data (Email composer data)
 * @param[in]	parent		Evas_Object that used as parent for recipient layout
 *
 */
void composer_recipient_update_cc_detail(void *data, Evas_Object *parent);

/**
 * @brief Updates bcc field with appropriate data
 *
 * @param[in]	data		User data (Email composer data)
 * @param[in]	parent		Evas_Object that used as parent for recipient layout
 *
 */
void composer_recipient_update_bcc_detail(void *data, Evas_Object *parent);

/**
 * @brief Updates from field with appropriate data
 *
 * @param[in]	data		User data (Email composer data)
 * @param[in]	parent		Evas_Object that used as parent for recipient layout
 *
 */
void composer_recipient_update_from_detail(void *data, Evas_Object *parent);

/**
 * @brief Delete from ctxpopup
 *
 * @param[in]	view			Email composer data
 *
 */
void composer_recipient_from_ctxpopup_item_delete(EmailComposerView *view);

/**
 * @brief Set cc field visible/hide
 *
 * @param[in]	view					Email composer data
 * @param[in]	to_be_showed		The visible flag (EINA_TRUE = Visible, EINA_FALSE = hide)
 *
 */
void composer_recipient_show_hide_cc_field(EmailComposerView *view, Eina_Bool to_be_showed);

/**
 * @brief Show bcc field
 *
 * @param[in]	view			Email composer data
 *
 */
void composer_recipient_show_bcc_field(EmailComposerView *view);

/**
 * @brief Hide bcc field
 *
 * @param[in]	view			Email composer data
 *
 */
void composer_recipient_hide_bcc_field(EmailComposerView *view);

/**
 * @brief Set bcc field field visible/hide
 *
 * @param[in]	view					Email composer data
 * @param[in]	to_be_showed		The visible flag (EINA_TRUE = Visible, EINA_FALSE = hide)
 *
 */
void composer_recipient_show_hide_bcc_field(EmailComposerView *view, Eina_Bool to_be_showed);

/**
 * @brief Apply updates to recipient string field
 *
 * @param[in]	view					Email composer data
 * @param[in]	mbe					Multi button entry
 * @param[in]	entry				Entry
 * @param[in]	display_entry		Dispaly entry
 * @param[in]	count				Recipient count
 *
 */
void composer_recipient_update_display_string(EmailComposerView *view, Evas_Object *mbe, Evas_Object *entry, Evas_Object *display_entry, int count);

/**
 * @brief Attach button to recipient contact field
 *
 * @param[in]	data				User data (Email composer data)
 * @param[in]	recp_layout			Layout
 * @param[in]	btn					Button
 *
 */
void composer_recipient_show_contact_button(void *data, Evas_Object *recp_layout, Evas_Object *btn);

/**
 * @brief Hide button in recipient contact field
 *
 * @param[in]	data				User data (Email composer data)
 * @param[in]	entry				Entry
 *
 */
void composer_recipient_hide_contact_button(void *data, Evas_Object *entry);

/**
 * @brief Unfocus entry in recipient contact field
 *
 * @param[in]	data				User data (Email composer data)
 * @param[in]	entry				Entry
 *
 */
void composer_recipient_unfocus_entry(void *data, Evas_Object *entry);

/**
 * @brief Change entry in recipient contact field
 *
 * @param[in]	to_editable			Eina_Bool
 * @param[in]	recp_box			Evas_Object recipient box
 * @param[in]	ent_lay				Evas_Object entry layout
 * @param[in]	disp_ent_lay		Evas_Object dispalay entry layout
 *
 */
void composer_recipient_change_entry(Eina_Bool to_editable, Evas_Object *recp_box, Evas_Object *ent_lay, Evas_Object *disp_ent_lay);

/**
 * @brief Reset entry with mbe in recipient contact field
 *
 * @param[in]	composer_box		Evas_Object composer box
 * @param[in]	mbe_layout			Evas_Object mbe layout
 * @param[in]	mbe					Evas_Object mbe
 * @param[in]	recp_layout			Evas_Object recipient layout
 * @param[in]	recp_box			Evas_Object recipient box
 * @param[in]	label				Evas_Object label
 *
 */
void composer_recipient_reset_entry_with_mbe(Evas_Object *composer_box, Evas_Object *mbe_layout, Evas_Object *mbe, Evas_Object *recp_layout, Evas_Object *recp_box, Evas_Object *label);

/**
 * @brief Reset entry without mbe in recipient contact field
 *
 * @param[in]	composer_box		Evas_Object composer box
 * @param[in]	mbe_layout			Evas_Object mbe layout
 * @param[in]	recp_layout			Evas_Object recipient layout
 * @param[in]	recp_box			Evas_Object recipient box
 * @param[in]	label				Evas_Object label
 *
 */
void composer_recipient_reset_entry_without_mbe(Evas_Object *composer_box, Evas_Object *mbe_layout, Evas_Object *recp_layout, Evas_Object *recp_box, Evas_Object *label);

/**
 * @brief Dispalay error string from recipient contact field
 *
 * @param[in]	data		User data (Email composer data)
 * @param[in]	err			Error code from MBE_VALIDATION_ERROR enum
 *
 */
void composer_recipient_display_error_string(void *data, MBE_VALIDATION_ERROR err);

/**
 * @brief Pass through email address list and validate email address
 *
 * @param[in]	data			User data (Email composer data)
 * @param[in]	obj				Error code MBE_VALIDATION_ERROR enum
 * @param[in]	address			Address name
 * @param[in]	invalid_list	Invalid list of address names
 * @param[in]	err			Error code from MBE_VALIDATION_ERROR enum
 *
 * @return EINA_TRUE on success or EINA_FALSE if none or an error occurred
 *
 */
Eina_Bool composer_recipient_mbe_validate_email_address_list(void *data, Evas_Object *obj, char *address, char **invalid_list, MBE_VALIDATION_ERROR *ret_err);

/**
 * @brief Attach new multibuttonentry to one of the recipients
 *
 * @param[in]	data		User data (Email composer data)
 * @param[in]	entry		Evas_Object entry
 *
 * @return EINA_TRUE on success or EINA_FALSE if none or an error occurred
 *
 */
Eina_Bool composer_recipient_commit_recipient_on_entry(void *data, Evas_Object *entry);

/**
 * @brief Checks if an entry one of recipient fields
 *
 * @param[in]	data		User data (Email composer data)
 * @param[in]	entry		Error code MBE_VALIDATION_ERROR enum
 *
 * @return EINA_TRUE if entry one from 1)to 2)cc 3)bcc or EINA_FALSE if no one from recipient field
 */
Eina_Bool composer_recipient_is_recipient_entry(void *data, Evas_Object *entry);

#ifdef _ALWAYS_CC_MYSELF

/**
 * @brief Append to recipients my address
 *
 * @param[in]	data		User data (Email composer data)
 * @param[in]	account		Email account data
 *
 */
void composer_recipient_append_myaddress(void *data, email_account_t *account);

/**
 * @brief Remove from recipients my address
 *
 * @param[in]	mbe				Evas_Object mbe
 * @param[in]	myaddress		My address name
 *
 */
void composer_recipient_remove_myaddress(Evas_Object *mbe, const char *myaddress);
#endif

/* email-composer-recipient-callback.c */

/**
 * @brief Callback on click for recipient contact button
 *
 * @param[in]	data			User data (Email composer data)
 * @param[in]	obj				Evas_Object button
 * @param[in]	event_info		Event info
 *
 */
void _recipient_contact_button_clicked_cb(void *data, Evas_Object *obj, void *event_info);

/**
 * @brief Callback validate email address, mbe filter
 *
 * @param[in]	obj				Evas_Object
 * @param[in]	item_label		Item label name
 * @param[in]	item_data		User data (Email recipient info data)
 * @param[in]	data			User data (Email composer data)
 *
 * @return EINA_TRUE if entry one from 1)to 2)cc 3)bcc or EINA_FALSE if no one from recipient field
 */
Eina_Bool _recipient_mbe_filter_cb(Evas_Object *obj, const char *item_label, void *item_data, void *data);

/**
 * @brief Callback when mbe added
 *
 * @param[in]	data			User data (Email composer data)
 * @param[in]	obj				Evas_Object button
 * @param[in]	event_info		Event info
 *
 */
void _recipient_mbe_added_cb(void *data, Evas_Object *obj, void *event_info);

/**
 * @brief Callback when mbe deleted
 *
 * @param[in]	data			User data (Email composer data)
 * @param[in]	obj				Evas_Object button
 * @param[in]	event_info		Event info
 *
 */
void _recipient_mbe_deleted_cb(void *data, Evas_Object *obj, void *event_info);

/**
 * @brief Callback when mbe selected
 *
 * @param[in]	data			User data (Email composer data)
 * @param[in]	obj				Evas_Object button
 * @param[in]	event_info		Event info
 *
 */
void _recipient_mbe_selected_cb(void *data, Evas_Object *obj, void *event_info);

/**
 * @brief Callback when mbe focused
 *
 * @param[in]	data			User data (Email composer data)
 * @param[in]	obj				Evas_Object button
 * @param[in]	event_info		Event info
 *
 */
void _recipient_mbe_focused_cb(void *data, Evas_Object *obj, void *event_info);

/**
 * @brief Callback when entry changed
 *
 * @param[in]	data			User data (Email composer data)
 * @param[in]	obj				Evas_Object button
 * @param[in]	event_info		Event info
 *
 */
void _recipient_entry_changed_cb(void *data, Evas_Object *obj, void *event_info);

/**
 * @brief Callback when entry focused
 *
 * @param[in]	data			User data (Email composer data)
 * @param[in]	obj				Evas_Object button
 * @param[in]	event_info		Event info
 *
 */
void _recipient_entry_focused_cb(void *data, Evas_Object *obj, void *event_info);

/**
 * @brief Callback when entry unfocused
 *
 * @param[in]	data			User data (Email composer data)
 * @param[in]	obj				Evas_Object button
 * @param[in]	event_info		Event info
 *
 */
void _recipient_entry_unfocused_cb(void *data, Evas_Object *obj, void *event_info);

/**
 * @brief Callback when entry keydown
 *
 * @param[in]	data			User data (Email composer data)
 * @param[in]	obj				Evas_Object button
 * @param[in]	event_info		Event info
 *
 */
void _recipient_entry_keydown_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

/**
 * @brief Callback when entry keyup
 *
 * @param[in]	data			User data (Email composer data)
 * @param[in]	obj				Evas_Object button
 * @param[in]	event_info		Event info
 *
 */
void _recipient_entry_keyup_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

/**
 * @brief Set text into recipient label according to label type
 *
 * @param[in]	obj				Evas_Object label
 * @param[in]	recp_type		Label type: TO, CC, BCC, FROM, etc
 *
 */
void composer_recipient_label_text_set(Evas_Object *obj, COMPOSER_RECIPIENT_TYPE_E recp_type);

#endif /* __EMAIL_COMPOSER_RECIPIENT_H__ */
