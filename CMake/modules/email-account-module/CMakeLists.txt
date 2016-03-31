CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(email-account-module C)

SET(CURRENT_SOURCE_DIR "${PROJECT_MODULES_DIR}/account")

SET(SRCS
	${CURRENT_SOURCE_DIR}/src/email-account.c
	${CURRENT_SOURCE_DIR}/src/email-account-folder.c
	${CURRENT_SOURCE_DIR}/src/email-account-folder-move.c
	${CURRENT_SOURCE_DIR}/src/email-account-util.c
	${CURRENT_SOURCE_DIR}/src/email-account-list-view.c
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
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/email-account.edj
		COMMAND edje_cc -no-save -id ${PROJECT_EDC_IMAGES_DIR} ${EDJE_CC_DEFINITIONS}
		${PROJECT_EDJE_DIR}/email-account.edc ${CMAKE_CURRENT_BINARY_DIR}/email-account.edj
		DEPENDS ${PROJECT_EDJE_DIR}/email-account.edc
				${PROJECT_EDC_DIR}/account/email-account-base-layout.edc
				${PROJECT_EDC_DIR}/common/email-parts.edc
				${PROJECT_EDC_DIR}/common/email-templates.edc
)
ADD_CUSTOM_TARGET(email-account.edj ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/email-account.edj)

INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/email-account.edj DESTINATION ${EDJDIR})
