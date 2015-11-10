CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(email-viewer-module C)

SET(CURRENT_SOURCE_DIR "${PROJECT_MODULES_DIR}/viewer")

SET(SRCS
	${CURRENT_SOURCE_DIR}/src/email-viewer.c
	${CURRENT_SOURCE_DIR}/src/email-viewer-noti-mgr.c
	${CURRENT_SOURCE_DIR}/src/email-viewer-attachment.c
	${CURRENT_SOURCE_DIR}/src/email-viewer-contents.c
	${CURRENT_SOURCE_DIR}/src/email-viewer-ext-gesture.c
	${CURRENT_SOURCE_DIR}/src/email-viewer-logic.c
	${CURRENT_SOURCE_DIR}/src/email-viewer-more-menu.c
	${CURRENT_SOURCE_DIR}/src/email-viewer-more-menu-callback.c
	${CURRENT_SOURCE_DIR}/src/email-viewer-recipient.c
	${CURRENT_SOURCE_DIR}/src/email-viewer-header.c
	${CURRENT_SOURCE_DIR}/src/email-viewer-util.c
	${CURRENT_SOURCE_DIR}/src/email-viewer-initial-data.c
	${CURRENT_SOURCE_DIR}/src/email-viewer-eml.c
	${CURRENT_SOURCE_DIR}/src/email-viewer-scroller.c
)

INCLUDE_DIRECTORIES(${CURRENT_SOURCE_DIR}/inc)

SET(email-required-pkgs elementary ecore glib-2.0 gobject-2.0
	notification capi-media-metadata-extractor capi-media-image-util capi-content-media-content
	email-service gio-2.0 capi-appfw-application chromium-efl capi-system-system-settings
	contacts-service2 bundle dlog calendar-service2 efl-extension storage capi-base-utils-i18n)

pkg_check_modules(pkgs-viewer REQUIRED "${email-required-pkgs}")

FOREACH(flag ${pkgs-viewer_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")

#ADD_DEFINITIONS("-D_WEBKIT_CONSOLE_MESSAGE_LOG")

ADD_LIBRARY(${PROJECT_NAME} SHARED ${SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} email-common ${pkgs-viewer_LDFLAGS})
INSTALL(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${LIBDIR})

ADD_CUSTOM_COMMAND(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/email-viewer.edj
		COMMAND edje_cc -no-save -id ${PROJECT_EDC_IMAGES_DIR} ${EDJE_CC_DEFINITIONS}
		${PROJECT_EDC_DIR}/viewer/email-viewer.edc ${CMAKE_CURRENT_BINARY_DIR}/email-viewer.edj
		DEPENDS ${PROJECT_EDC_DIR}/viewer/email-viewer.edc
				${PROJECT_EDC_DIR}/viewer/email-viewer-common.edc
				${PROJECT_EDC_DIR}/viewer/email-viewer-base-layout.edc
				${PROJECT_EDC_DIR}/viewer/email-viewer-subject-layout.edc
				${PROJECT_EDC_DIR}/viewer/email-viewer-popup.edc
				${PROJECT_EDC_DIR}/viewer/email-viewer-scroller.edc
				${PROJECT_EDC_DIR}/common/email-colors.edc
				${PROJECT_EDC_DIR}/common/email-parts.edc
)
ADD_CUSTOM_TARGET(email-viewer.edj ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/email-viewer.edj)

INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/email-viewer.edj DESTINATION ${EDJDIR})
INSTALL(FILES ${PROJECT_MISC_DIR}/_email_default.html DESTINATION ${MISCDIR})
INSTALL(FILES ${PROJECT_MISC_DIR}/_email_template.html DESTINATION ${MISCDIR})
INSTALL(FILES ${PROJECT_MISC_DIR}/_email_template_text.html DESTINATION ${MISCDIR})
INSTALL(FILES ${PROJECT_MISC_DIR}/default.css DESTINATION ${MISCDIR})
