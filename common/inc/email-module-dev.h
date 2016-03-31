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

#ifndef _EMAIL_MODULE_DEV_H_
#define _EMAIL_MODULE_DEV_H_

#include <app_common.h>
#include <bundle.h>
#include <Elementary.h>

#include "email-module.h"
#include "email-module-core.h"
#include "email-module-launcher.h"

/**
 * Forward declaration of the attach panel handle
 */
typedef struct _attach_panel *attach_panel_h;

/**
 * Enumeration of the email module states
 */
typedef enum _email_module_state
{
	/* Email module is running */
	EM_STATE_RUNNING,
	/* Email module is paused */
	EM_STATE_PAUSED,
	/* Email module is destroying */
	EM_STATE_DESTROYING

} email_module_state_e;

/**
 * Base data structure for all email modules
 */
struct _email_module
{
	/**
	 * Public members
	 */

	/**
	 * Core
	 */

	/* Main widgets of the application */
	Evas_Object *win;
	Evas_Object *conform;
	Evas_Object *navi;

	/* Count of views of this module */
	int views_count;

	/**
	 * Protected members
	 */

	/**
	 * Core
	 */

	/**
	 * @brief Virtual function which is called from base implementation to create the concrete module
	 *
	 * @param[in]	self		pointer to this module
	 * @param[in]	params		create parameters
	 *
	 * @return 0 - on success,
	 * 		negative value - on error
	 */
	int (*create) (email_module_t *self, email_params_h params);

	/**
	 * @brief Virtual function which is called from base implementation to destroy the concrete module
	 *
	 * @param[in]	self		pointer to this module
	 */
	void (*destroy) (email_module_t *self);

	/**
	 * @brief Virtual function to handle module pause event
	 *
	 * @param[in]	self		pointer to this module
	 */
	void (*pause) (email_module_t *self);

	/**
	 * @brief Virtual function to handle module resume event
	 *
	 * @param[in]	self		pointer to this module
	 */
	void (*resume) (email_module_t *self);

	/**
	 * @brief Virtual function to handle messages send to this module
	 *
	 * @param[in]	self		pointer to this module
	 */
	void (*on_message) (email_module_t *self, email_params_h msg);

	/**
	 * @brief Virtual function to handle events from module manager
	 *
	 * @param[in]	self		pointer to this module
	 * @param[in]	self		event type to handle
	 */
	void (*on_event) (email_module_t *self, email_module_event_e event);

	/**
	 * Launcher
	 */

	/* Currently launched application type or
	 * EMAIL_LAUNCH_APP_NONE if no applications launched*/
	email_launch_app_type_e launched_app_type;

	/* Status of the attach panel */
	bool is_attach_panel_launched;
	/* Busy status of the launcher */
	bool is_launcher_busy;

	/**
	 * Private members
	 */

	/**
	 * Core
	 */

	/* Current state of the module */
	email_module_state_e _state;

	/* Parent of the module or NULL for root module */
	email_module_t *_parent;
	/* Array of the module children or NULL */
	Eina_Array *_children;
	/* Listener of the module */
	email_module_listener_t _listener;

	/* Naviframe item on top of which all module view will be created */
	Elm_Object_Item *_parent_navi_item;

	/**
	 * Launcher
	 */

	/* Request that was used to launch an application or NULL */
	app_control_h _launched_app;
	/* Handle of the attach panel if created, or NULL */
	attach_panel_h _attach_panel;
	/* Timer used to reset transparent application state */
	Ecore_Timer *_app_timer;
	/* Application launch listener */
	email_launched_app_listener_t _app_listener;
	/* Attach panel launch listener */
	email_attach_panel_listener_t _attach_panel_listener;
	/* true if the application was actually started by the system */
	bool _app_was_started;
	/* true if we need to recreate attach panel on next launch */
	bool _attach_panel_bundles_changed;

	/* Attach panel category extra data bundles
	 * to use when creating attach panel categories */
	bundle *_attach_panel_bundles[EMAIL_APCT_COUNT];
};

/**
 * Enumeration of the email view states
 */
typedef enum _email_view_state
{
	/* View was created but is not yet active */
	EV_STATE_CREATED,
	/* View is active - normal state of the view */
	EV_STATE_ACTIVE,
	/* View is not active */
	EV_STATE_NOT_ACTIVE,
	/* View is getting destroyed */
	EV_STATE_DESTROYING

} email_view_state;

/**
 * Collection of the view create flags
 */
typedef enum _email_view_update_flag
{
	/* View orientation was changed from the last update */
	EVUF_ORIENTATION_CHANGED = 1,
	/* System language was changed */
	EVUF_LANGUAGE_CHANGED = 2,
	/* System region format was changed */
	EVUF_REGION_FMT_CHANGED = 4,

	/* Application was in pause state after the last update */
	EVUF_WAS_PAUSED = 8,
	/* View become topmost after popping old topmost view  */
	EVUF_POPPING = 16

} email_view_update_flag;

/**
 * Collection of the view push flags
 */
typedef enum _email_view_push_flag
{
	/* View should be pushed without a transition if possible */
	EVPF_NO_TRANSITION = 1,
	/* View should be pushed without a software back button */
	EVPF_NO_BACK_BUTTON = 2,
	/* View should be pushed without title */
	EVPF_NO_TITLE = 4

} email_view_push_flag;

/**
 * Email view base data structure
 */
struct _email_view
{
	/* Owning module of this view */
	email_module_t *module;

	/* Content of the naviframe item */
	Evas_Object *content;
	/* Naviframe item of this view */
	Elm_Object_Item *navi_item;

	/* View state */
	email_view_state state;
	/* View visibility status */
	bool visible;

	/* Current orientation of the view */
	app_device_orientation_e orientation;

	/**
	 * @brief Virtual function which is called from base implementation to create the concrete view
	 *
	 * @param[in]	self		pointer to this view
	 *
	 * @return 0 - on success,
	 * 		negative value - on error
	 */
	int (*create)(email_view_t *self);

	/**
	 * @brief Virtual function which is called from base implementation to destroy the concrete view
	 *
	 * @param[in]	self		pointer to this view
	 */
	void (*destroy) (email_view_t *self);

	/**
	 * @brief Virtual function which is called from base implementation to activate the concrete view
	 *
	 * @param[in]	self			pointer to this view
	 * @param[in]	prev_state		previous state of the view before activation
	 */
	void (*activate) (email_view_t *self, email_view_state prev_state);

	/**
	 * @brief Virtual function which is called from base implementation to deactivate the concrete view
	 *
	 * @param[in]	self		pointer to this view
	 */
	void (*deactivate) (email_view_t *self);

	/**
	 * @brief Virtual function to handle show event
	 *
	 * @param[in]	self		pointer to this view
	 */
	void (*show) (email_view_t *self);

	/**
	 * @brief Virtual function to handle hide event
	 *
	 * @param[in]	self		pointer to this view
	 */
	void (*hide) (email_view_t *self);

	/**
	 * @brief Virtual function which is called from base implementation to update the concrete view
	 *
	 * @param[in]	self		pointer to this view
	 * @param[in]	flags		view update flags
	 */
	void (*update) (email_view_t *self, int flags);

	/**
	 * @brief Virtual function to back button press event (software and hardware)
	 *
	 * @param[in]	self		pointer to this view
	 */
	void (*on_back_key) (email_view_t *self);

	/* Update flags that will be used in the next update */
	int _update_flags;
	/* Push flags that was to create the view */
	int _push_flags;
	/* Blocks popping of the view item */
	bool _allow_pop;
};

#endif /* _EMAIL_MODULE_DEV_H_ */
