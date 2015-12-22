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

IF(${TIZEN_2_4})

IF(${ENABLE_ATTACH_PANEL})
	SET(email-required-pkgs ecore glib-2.0 gobject-2.0 email-service storage
		contacts-service2 capi-content-media-content dlog elementary evas eina edje capi-base-utils-i18n
		accounts-svc capi-network-connection efl-extension capi-content-mime-type capi-system-system-settings
		capi-appfw-application ewebkit2 capi-appfw-preference vconf attach-panel feedback)
ELSE(${ENABLE_ATTACH_PANEL})
	SET(email-required-pkgs ecore glib-2.0 gobject-2.0 email-service storage
		contacts-service2 capi-content-media-content dlog elementary evas eina edje capi-base-utils-i18n
		accounts-svc capi-network-connection efl-extension capi-content-mime-type capi-system-system-settings
		capi-appfw-application ewebkit2 capi-appfw-preference vconf feedback)
ENDIF(${ENABLE_ATTACH_PANEL})

ELSE(${TIZEN_2_4})

IF(${ENABLE_ATTACH_PANEL})
	SET(email-required-pkgs ecore glib-2.0 gobject-2.0 email-service storage
		contacts-service2 capi-content-media-content dlog elementary evas eina edje capi-base-utils-i18n
		accounts-svc capi-network-connection efl-extension capi-content-mime-type capi-system-system-settings
		capi-appfw-application chromium-efl capi-appfw-preference vconf attach-panel feedback libtzplatform-config)
ELSE(${ENABLE_ATTACH_PANEL})
	SET(email-required-pkgs ecore glib-2.0 gobject-2.0 email-service storage
		contacts-service2 capi-content-media-content dlog elementary evas eina edje capi-base-utils-i18n
		accounts-svc capi-network-connection efl-extension capi-content-mime-type capi-system-system-settings
		capi-appfw-application chromium-efl capi-appfw-preference vconf feedback libtzplatform-config)
ENDIF(${ENABLE_ATTACH_PANEL})

ENDIF(${TIZEN_2_4})


pkg_check_modules(pkgs-common REQUIRED "${email-required-pkgs}")

FOREACH(flag ${pkgs-common_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")

ADD_LIBRARY(${PROJECT_NAME} SHARED ${SRCS-common})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} dl ${pkgs-common_LDFLAGS})
INSTALL(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${LIBDIR})

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

