CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(email-common C)

SET(SRCS-common
	${PROJECT_COMMON_DIR}/src/email-engine.c
	${PROJECT_COMMON_DIR}/src/email-html-converter.c
	${PROJECT_COMMON_DIR}/src/email-utils.c
	${PROJECT_COMMON_DIR}/src/email-popup-utils.c
	${PROJECT_COMMON_DIR}/src/email-utils-contacts.c
	${PROJECT_COMMON_DIR}/src/email-media-content.c
	${PROJECT_COMMON_DIR}/src/email-request-queue.c
	${PROJECT_COMMON_DIR}/src/email-module-launcher.c
	${PROJECT_COMMON_DIR}/src/email-module-core.c
	${PROJECT_COMMON_DIR}/src/email-editfield-utils.c
	${PROJECT_COMMON_DIR}/src/email-color-box.c
)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpic")

IF(${BUILD_SHARED_LIBS})
	ADD_LIBRARY(${PROJECT_NAME} SHARED ${SRCS-common})
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} dl ${pkgs-email_LDFLAGS})
	INSTALL(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${LIBDIR})
ELSE(${BUILD_SHARED_LIBS})
	ADD_LIBRARY(${PROJECT_NAME} STATIC ${SRCS-common})
ENDIF(${BUILD_SHARED_LIBS})

ADD_CUSTOM_COMMAND(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/email-common-theme.edj
		COMMAND edje_cc -no-save
		-id ${PROJECT_EDC_IMAGES_DIR}/account_icon
		-id ${PROJECT_EDC_IMAGES_DIR}/composer_icon
		-id ${PROJECT_EDC_IMAGES_DIR}/list_icon
		-id ${PROJECT_EDC_IMAGES_DIR}/viewer_icon
		-id ${PROJECT_IMAGES_DIR}/composer_icon
		-id ${PROJECT_IMAGES_DIR}/list_icon
		${EDJE_CC_DEFINITIONS}
		${PROJECT_EDC_DIR}/common/email-common-theme.edc ${CMAKE_CURRENT_BINARY_DIR}/email-common-theme.edj
		DEPENDS ${PROJECT_EDC_DIR}/common/email-common-theme.edc
				${PROJECT_EDC_DIR}/common/email-title-search.edc
				${PROJECT_EDC_DIR}/common/email-colors.edc
				${PROJECT_COMMON_INCLUDE}/email-images.h
)
ADD_CUSTOM_TARGET(email-common-theme.edj ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/email-common-theme.edj)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/email-common-theme.edj DESTINATION ${EDJDIR})
