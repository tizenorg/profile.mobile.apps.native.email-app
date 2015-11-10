CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(email-filter-module C)

SET(CURRENT_SOURCE_DIR "${PROJECT_MODULES_DIR}/filter")

SET(SRCS
	${CURRENT_SOURCE_DIR}/src/email-filter.c
	${CURRENT_SOURCE_DIR}/src/email-filter-list-view.c
	${CURRENT_SOURCE_DIR}/src/email-filter-add-view.c
	${CURRENT_SOURCE_DIR}/src/email-filter-edit-view.c
	${CURRENT_SOURCE_DIR}/src/email-filter-delete-view.c
)

INCLUDE_DIRECTORIES(${CURRENT_SOURCE_DIR}/inc)

pkg_check_modules(pkgs-filter REQUIRED
	ecore
	glib-2.0
	gobject-2.0
	email-service
	dlog
	elementary
	capi-appfw-application
	notification
	contacts-service2
	efl-extension
	capi-system-system-settings
)

FOREACH(flag ${pkgs-filter_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")

ADD_LIBRARY(${PROJECT_NAME} SHARED ${SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} email-common ${pkgs-filter_LDFLAGS})
INSTALL(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${LIBDIR})

ADD_CUSTOM_COMMAND(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/email-filter.edj
	COMMAND edje_cc -no-save -id ${PROJECT_EDC_IMAGES_DIR} ${EDJE_CC_DEFINITIONS}
	${PROJECT_EDC_DIR}/filter/email-filter.edc ${CMAKE_CURRENT_BINARY_DIR}/email-filter.edj
	DEPENDS ${PROJECT_EDC_DIR}/filter/email-filter.edc
	${PROJECT_EDC_DIR}/filter/email-filter-address-edit-view.edc)

ADD_CUSTOM_TARGET(email-filter.edj ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/email-filter.edj)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/email-filter.edj DESTINATION ${EDJDIR})
