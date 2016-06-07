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
	${CURRENT_SOURCE_DIR}/src/email-viewer-reply-toolbar.c
	${CURRENT_SOURCE_DIR}/src/email-viewer-util.c
	${CURRENT_SOURCE_DIR}/src/email-viewer-initial-data.c
	${CURRENT_SOURCE_DIR}/src/email-viewer-eml.c
	${CURRENT_SOURCE_DIR}/src/email-viewer-scroller.c
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
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/email-viewer.edj
		COMMAND edje_cc -no-save -id ${PROJECT_EDC_IMAGES_DIR} ${EDJE_CC_DEFINITIONS}
		${PROJECT_EDJE_DIR}/email-viewer.edc ${CMAKE_CURRENT_BINARY_DIR}/email-viewer.edj
		DEPENDS ${PROJECT_EDJE_DIR}/email-viewer.edc
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
