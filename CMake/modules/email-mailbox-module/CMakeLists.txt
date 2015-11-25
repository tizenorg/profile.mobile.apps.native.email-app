CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(email-mailbox-module C)

SET(CURRENT_SOURCE_DIR "${PROJECT_MODULES_DIR}/mailbox")

SET(SRCS
	${CURRENT_SOURCE_DIR}/src/email-mailbox.c
	${CURRENT_SOURCE_DIR}/src/email-mailbox-item.c
	${CURRENT_SOURCE_DIR}/src/email-mailbox-list.c
	${CURRENT_SOURCE_DIR}/src/email-mailbox-list-other-items.c
	${CURRENT_SOURCE_DIR}/src/email-mailbox-more-menu.c
	${CURRENT_SOURCE_DIR}/src/email-mailbox-noti-mgr.c
	${CURRENT_SOURCE_DIR}/src/email-mailbox-request.c
	${CURRENT_SOURCE_DIR}/src/email-mailbox-search.c
	${CURRENT_SOURCE_DIR}/src/email-mailbox-sync.c
	${CURRENT_SOURCE_DIR}/src/email-mailbox-title.c
	${CURRENT_SOURCE_DIR}/src/email-mailbox-toolbar.c
	${CURRENT_SOURCE_DIR}/src/email-mailbox-ug-util.c
	${CURRENT_SOURCE_DIR}/src/email-mailbox-util.c
)

INCLUDE_DIRECTORIES(${CURRENT_SOURCE_DIR}/inc)

SET(email-required-pkgs elementary email-service glib-2.0 gio-2.0 bundle dlog capi-base-utils-i18n
	capi-appfw-application contacts-service2 notification capi-system-system-settings efl-extension)

pkg_check_modules(pkgs-mailbox REQUIRED "${email-required-pkgs}")

FOREACH(flag ${pkgs-mailbox_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")

ADD_LIBRARY(${PROJECT_NAME} SHARED ${SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} email-common ${pkgs-mailbox_LDFLAGS})
INSTALL(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${LIBDIR})

ADD_CUSTOM_COMMAND(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/email-mailbox.edj
		COMMAND edje_cc -no-save -id ${PROJECT_EDC_IMAGES_DIR} ${EDJE_CC_DEFINITIONS}
		${PROJECT_EDC_DIR}/mailbox/email-mailbox.edc ${CMAKE_CURRENT_BINARY_DIR}/email-mailbox.edj
		DEPENDS ${PROJECT_EDC_DIR}/mailbox/email-mailbox.edc
				${PROJECT_EDC_DIR}/mailbox/email-mailbox-theme.edc
				${PROJECT_EDC_DIR}/mailbox/email-mailbox-genlist-style.edc
				${PROJECT_EDC_DIR}/mailbox/email-mailbox-macros.edc
				${PROJECT_EDC_DIR}/common/email-colors.edc
				${PROJECT_EDC_DIR}/common/email-parts.edc
)
ADD_CUSTOM_TARGET(email-mailbox.edj ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/email-mailbox.edj)

INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/email-mailbox.edj DESTINATION ${EDJDIR})
