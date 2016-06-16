CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(email-mailbox-module C)

SET(CURRENT_SOURCE_DIR "${PROJECT_MODULES_DIR}/mailbox")

SET(SRCS
	${CURRENT_SOURCE_DIR}/src/email-mailbox.c
	${CURRENT_SOURCE_DIR}/src/email-mailbox-item.c
	${CURRENT_SOURCE_DIR}/src/email-mailbox-list.c
	${CURRENT_SOURCE_DIR}/src/email-mailbox-list-extensions.c
	${CURRENT_SOURCE_DIR}/src/email-mailbox-more-menu.c
	${CURRENT_SOURCE_DIR}/src/email-mailbox-noti-mgr.c
	${CURRENT_SOURCE_DIR}/src/email-mailbox-request.c
	${CURRENT_SOURCE_DIR}/src/email-mailbox-search.c
	${CURRENT_SOURCE_DIR}/src/email-mailbox-sync.c
	${CURRENT_SOURCE_DIR}/src/email-mailbox-title.c
	${CURRENT_SOURCE_DIR}/src/email-mailbox-toolbar.c
	${CURRENT_SOURCE_DIR}/src/email-mailbox-module-util.c
	${CURRENT_SOURCE_DIR}/src/email-mailbox-util.c
)

INCLUDE_DIRECTORIES(${CURRENT_SOURCE_DIR}/inc)

IF(${BUILD_SHARED_LIBS})
	ADD_LIBRARY(${PROJECT_NAME} SHARED ${SRCS})
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} email-common ${pkgs-email_LDFLAGS})
	INSTALL(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${LIBDIR})
ELSE(${BUILD_SHARED_LIBS})
	ADD_LIBRARY(${PROJECT_NAME} STATIC ${SRCS})
ENDIF(${BUILD_SHARED_LIBS})

ADD_CUSTOM_COMMAND(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/email-mailbox.edj
		COMMAND edje_cc -no-save -id ${PROJECT_EDC_IMAGES_DIR} ${EDJE_CC_DEFINITIONS}
		${PROJECT_EDJE_DIR}/email-mailbox.edc ${CMAKE_CURRENT_BINARY_DIR}/email-mailbox.edj
		DEPENDS ${PROJECT_EDJE_DIR}/email-mailbox.edc
				${PROJECT_EDC_DIR}/mailbox/email-mailbox-theme.edc
				${PROJECT_EDC_DIR}/mailbox/email-mailbox-genlist-style.edc
				${PROJECT_EDC_DIR}/mailbox/email-mailbox-macros.edc
				${PROJECT_EDC_DIR}/common/email-colors.edc
				${PROJECT_EDC_DIR}/common/email-parts.edc
)
ADD_CUSTOM_TARGET(email-mailbox.edj ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/email-mailbox.edj)

INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/email-mailbox.edj DESTINATION ${EDJDIR})
