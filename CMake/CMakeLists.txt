CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET(PKGNAME "org.tizen.email")
SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(BINDIR "${PREFIX}/bin")
SET(RESDIR "${PREFIX}/res")
SET(LIBDIR "${PREFIX}/lib")
SET(LOCALEDIR "${RESDIR}/locale")
SET(EDJDIR "${RESDIR}/edje")
SET(IMGDIR "${RESDIR}/images")
SET(MISCDIR "${RESDIR}/misc")
SET(LICENSEDIR ${SYS_LICENSE_DIR})
SET(PACKAGEDIR ${SYS_PACKAGES_DIR})
SET(ICONDIR ${SYS_ICONS_DIR})
SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

IF(${TIZEN_2_4})
	SET(SMACKDIR ${SYS_SMACK_DIR})
	ADD_DEFINITIONS("-D_TIZEN_2_4_BUILD_")
ELSE(${TIZEN_2_4})
	ADD_DEFINITIONS("-D_SAVE_IN_USER_SHARE_DIR_")
	ADD_DEFINITIONS("-D_EWK_LOAD_NONEMPTY_LAYOUT_FINISHED_CB_UNAVAILABLE_")
ENDIF(${TIZEN_2_4})

ADD_DEFINITIONS("-D_DEBUG")
ADD_DEFINITIONS("-D_EMAIL_GBS_BUILD_")

IF(${ENABLE_ATTACH_PANEL})
	ADD_DEFINITIONS("-DATTACH_PANEL_FEATURE")
ENDIF(${ENABLE_ATTACH_PANEL})

IF(${BUILD_SHARED_LIBS})
	ADD_DEFINITIONS("-DSHARED_MODULES_FEATURE")
ENDIF(${BUILD_SHARED_LIBS})

# Set definitions to be used to compile edcs
SET(EDJE_CC_DEFINITIONS "")

# Set definitinos to be used to compile source files
# TODO Remove use of this define from code
IF(FEATURE_PRIORITY_SENDER_DISABLE)
	ADD_DEFINITIONS("-D_FEATURE_PRIORITY_SENDER_DISABLE_")
ENDIF(FEATURE_PRIORITY_SENDER_DISABLE)

SET(CMAKE_C_FLAGS_DEBUG "-O0 -g")
FIND_PROGRAM(UNAME NAMES uname)
EXEC_PROGRAM("${UNAME}" ARGS "-m" OUTPUT_VARIABLE "ARCH")
IF("${ARCH}" STREQUAL "armv7l")
	SET(CMAKE_C_FLAGS_RELEASE "-mabi=aapcs-linux -O2")
ENDIF("${ARCH}" STREQUAL "armv7l")

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdata-sections -ffunction-sections -Wl,--gc-sections -Werror")

INCLUDE(FindPkgConfig)

get_directory_property(DEFS COMPILE_DEFINITIONS)
FOREACH(D ${DEFS})
	SET(EDJE_CC_DEFINITIONS ${EDJE_CC_DEFINITIONS} "-D${D}")
ENDFOREACH(D)

SET(CMAKE_SKIP_BUILD_RPATH TRUE)

IF("${CMAKE_BUILD_TYPE}" STREQUAL "")
	SET(CMAKE_BUILD_TYPE "Release")
ENDIF("${CMAKE_BUILD_TYPE}" STREQUAL "")
MESSAGE("Build type: ${CMAKE_BUILD_TYPE}")

SET(PROJECT_ROOT_DIR "${CMAKE_SOURCE_DIR}/..")
SET(PROJECT_FILES_DIR "${PROJECT_ROOT_DIR}/Project-Files")

SET(PROJECT_COMMON_DIR "${PROJECT_FILES_DIR}/common")
SET(PROJECT_MODULES_DIR "${PROJECT_FILES_DIR}/modules")
SET(PROJECT_RESOURCES_DIR "${PROJECT_FILES_DIR}/resources")

SET(PROJECT_EDC_DIR "${PROJECT_RESOURCES_DIR}/edc")
SET(PROJECT_EDC_IMAGES_DIR "${PROJECT_RESOURCES_DIR}/edc_images")
SET(PROJECT_IMAGES_DIR "${PROJECT_RESOURCES_DIR}/images")
SET(PROJECT_MISC_DIR "${PROJECT_RESOURCES_DIR}/misc")
SET(PROJECT_PO_DIR "${PROJECT_RESOURCES_DIR}/po")

SET(PROJECT_COMMON_INCLUDE "${PROJECT_COMMON_DIR}/inc")
INCLUDE_DIRECTORIES(${PROJECT_COMMON_INCLUDE})

SET(email-required-pkgs ecore glib-2.0 gobject-2.0 email-service storage
	contacts-service2 capi-content-media-content dlog elementary evas eina edje capi-base-utils-i18n
	accounts-svc capi-network-connection efl-extension capi-content-mime-type capi-system-system-settings
	capi-appfw-application capi-appfw-preference vconf feedback notification gio-2.0 capi-media-image-util
	libpng capi-media-metadata-extractor libexif libxml-2.0)

IF(${TIZEN_2_4})
	SET(email-required-pkgs ${email-required-pkgs} ewebkit2)
ELSE(${TIZEN_2_4})
	SET(email-required-pkgs ${email-required-pkgs} chromium-efl libtzplatform-config)
ENDIF(${TIZEN_2_4})

IF(${ENABLE_ATTACH_PANEL})
	SET(email-required-pkgs ${email-required-pkgs} attach-panel)
ENDIF(${ENABLE_ATTACH_PANEL})

pkg_check_modules(pkgs-email REQUIRED ${email-required-pkgs})

FOREACH(flag ${pkgs-email_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

# Add your submodule directory name

ADD_SUBDIRECTORY(po)

ADD_SUBDIRECTORY(email-common)

ADD_SUBDIRECTORY(modules)

ADD_SUBDIRECTORY(main)
