CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(email-setting-module C)

SET(CURRENT_SOURCE_DIR "${PROJECT_MODULES_DIR}/setting")

SET(SRCS
	${CURRENT_SOURCE_DIR}/src/email-setting.c
	${CURRENT_SOURCE_DIR}/src/email-setting-account-set.c
	${CURRENT_SOURCE_DIR}/src/email-setting-utils.c
	${CURRENT_SOURCE_DIR}/src/email-setting-noti-mgr.c
	${CURRENT_SOURCE_DIR}/src/email-view-setting.c
	${CURRENT_SOURCE_DIR}/src/email-view-account-details.c
	${CURRENT_SOURCE_DIR}/src/email-view-account-details-setup.c
	${CURRENT_SOURCE_DIR}/src/email-view-account-setup.c
	${CURRENT_SOURCE_DIR}/src/email-view-manual-setup.c
	${CURRENT_SOURCE_DIR}/src/email-view-account-edit.c
	${CURRENT_SOURCE_DIR}/src/email-view-signature-edit.c
	${CURRENT_SOURCE_DIR}/src/email-view-signature-setting.c
	${CURRENT_SOURCE_DIR}/src/email-view-notification.c
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
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/email-setting-theme.edj
		COMMAND edje_cc -no-save ${EDJE_CC_DEFINITIONS}
		${PROJECT_EDJE_DIR}/email-setting-theme.edc ${CMAKE_CURRENT_BINARY_DIR}/email-setting-theme.edj
		DEPENDS ${PROJECT_EDJE_DIR}/email-setting-theme.edc
				${PROJECT_EDC_DIR}/setting/email-setting-two-bottom-btn-theme.edc
				${PROJECT_EDC_DIR}/setting/email-setting-genlist-item-custom-theme.edc
				${PROJECT_EDC_DIR}/common/email-colors.edc
				${PROJECT_EDC_DIR}/common/email-parts.edc
				${PROJECT_EDC_DIR}/common/email-templates.edc
)
ADD_CUSTOM_TARGET(email-setting-theme.edj ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/email-setting-theme.edj)

INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/email-setting-theme.edj DESTINATION ${EDJDIR})
INSTALL(FILES ${PROJECT_MISC_DIR}/provider_list_default.xml DESTINATION ${MISCDIR})
