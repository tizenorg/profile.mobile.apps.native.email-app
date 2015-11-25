CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(email-composer-module C)

SET(CURRENT_SOURCE_DIR "${PROJECT_MODULES_DIR}/composer")

SET(SRCS
	${CURRENT_SOURCE_DIR}/src/email-composer-webkit.c
	${CURRENT_SOURCE_DIR}/src/email-composer-more-menu.c
	${CURRENT_SOURCE_DIR}/src/email-composer-predictive-search.c
	${CURRENT_SOURCE_DIR}/src/email-composer-launcher.c
	${CURRENT_SOURCE_DIR}/src/email-composer-attachment.c
	${CURRENT_SOURCE_DIR}/src/email-composer-attachment-resize-image.c
	${CURRENT_SOURCE_DIR}/src/email-composer-attachment-ui.c
	${CURRENT_SOURCE_DIR}/src/email-composer-recipient.c
	${CURRENT_SOURCE_DIR}/src/email-composer-recipient-callback.c
	${CURRENT_SOURCE_DIR}/src/email-composer.c
	${CURRENT_SOURCE_DIR}/src/email-composer-initial-view.c
	${CURRENT_SOURCE_DIR}/src/email-composer-initial-data.c
	${CURRENT_SOURCE_DIR}/src/email-composer-subject.c
	${CURRENT_SOURCE_DIR}/src/email-composer-send-mail.c
	${CURRENT_SOURCE_DIR}/src/email-composer-util.c
	${CURRENT_SOURCE_DIR}/src/email-composer-util-image.c
	${CURRENT_SOURCE_DIR}/src/email-composer-util-popup.c
	${CURRENT_SOURCE_DIR}/src/email-composer-util-recipient.c
	${CURRENT_SOURCE_DIR}/src/email-composer-rich-text-toolbar.c
)

INCLUDE_DIRECTORIES(${CURRENT_SOURCE_DIR}/inc)

SET(email-required-pkgs ecore glib-2.0 gobject-2.0 gio-2.0 elementary capi-appfw-application
	capi-media-metadata-extractor capi-media-image-util notification evas dlog email-service contacts-service2
	libexif chromium-efl bundle efl-extension capi-system-system-settings
	capi-network-connection capi-network-wifi libpng storage capi-content-media-content)

pkg_check_modules(pkgs-composer REQUIRED "${email-required-pkgs}")

FOREACH(flag ${pkgs-composer_CFLAGS})
	SET(EXTRA_FLAGS "${EXTRA_FLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_FLAGS}")
#SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-z,nodelete")

ADD_LIBRARY(${PROJECT_NAME} SHARED ${SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} email-common rt ${pkgs-composer_LDFLAGS})
INSTALL(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${LIBDIR})

ADD_CUSTOM_COMMAND(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/email-composer-view.edj
		COMMAND edje_cc -no-save -id ${PROJECT_EDC_IMAGES_DIR} ${EDJE_CC_DEFINITIONS}
		${PROJECT_EDC_DIR}/composer/email-composer-view.edc ${CMAKE_CURRENT_BINARY_DIR}/email-composer-view.edj
		DEPENDS ${PROJECT_EDC_DIR}/composer/email-composer-view.edc
				${PROJECT_EDC_DIR}/composer/email-composer-edc-common.edc
				${PROJECT_EDC_DIR}/composer/email-composer-layouts.edc
				${PROJECT_EDC_DIR}/composer/email-composer-attachment.edc
				${PROJECT_EDC_DIR}/composer/email-composer-popup.edc
				${PROJECT_EDC_DIR}/composer/email-composer-predictive-search.edc
				${PROJECT_EDC_DIR}/composer/email-composer-recipients.edc
				${PROJECT_EDC_DIR}/composer/email-composer-scroller.edc
				${PROJECT_EDC_DIR}/composer/email-composer-subject.edc
				${PROJECT_EDC_DIR}/common/email-colors.edc
				${PROJECT_EDC_DIR}/common/email-parts.edc
				${PROJECT_EDC_DIR}/composer/email-composer-rich-text.edc
				${PROJECT_EDC_DIR}/composer/email-composer-edc-define.h
				${PROJECT_EDC_DIR}/composer/email-composer-toolbar-buttons_defines.edc
)
ADD_CUSTOM_TARGET(email-composer-view.edj ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/email-composer-view.edj)

INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/email-composer-view.edj DESTINATION ${EDJDIR})
INSTALL(FILES ${PROJECT_MISC_DIR}/email-composer.js DESTINATION ${MISCDIR})
INSTALL(FILES ${PROJECT_MISC_DIR}/email-composer.css DESTINATION ${MISCDIR})
