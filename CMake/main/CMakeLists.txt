CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(email C)

SET(VENDOR "tizen")
SET(PACKAGE ${PROJECT_NAME})
SET(PKGNAME "org.${VENDOR}.${PACKAGE}")

pkg_check_modules(pkgs-email REQUIRED elementary email-service accounts-svc
	bundle dlog ecore-input contacts-service2 efl-extension gio-2.0 capi-base-utils-i18n capi-system-system-settings
	capi-appfw-application)

FOREACH(flag ${pkgs-email_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -fpie")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie")

ADD_SUBDIRECTORY(email)
ADD_SUBDIRECTORY(email-setting)
ADD_SUBDIRECTORY(email-composer)

# install manifest file
INSTALL(FILES ${PROJECT_ROOT_DIR}/${PKGNAME}.xml DESTINATION ${PACKAGEDIR})

# install license file
INSTALL(FILES ${PROJECT_ROOT_DIR}/LICENSE DESTINATION ${LICENSEDIR} RENAME ${PKGNAME})

# install image files
INSTALL(FILES ${PROJECT_ROOT_DIR}/${PKGNAME}.png DESTINATION ${ICONDIR})
INSTALL(FILES ${PROJECT_IMAGES_DIR}/composer_icon/button_delete.png DESTINATION ${IMGDIR}/composer_icon)
INSTALL(FILES ${PROJECT_IMAGES_DIR}/composer_icon/core_check_bg_stroke.png DESTINATION ${IMGDIR}/composer_icon)
INSTALL(FILES ${PROJECT_IMAGES_DIR}/composer_icon/core_check_bg.png DESTINATION ${IMGDIR}/composer_icon)
INSTALL(FILES ${PROJECT_IMAGES_DIR}/composer_icon/core_check_icon.png DESTINATION ${IMGDIR}/composer_icon)
INSTALL(FILES ${PROJECT_IMAGES_DIR}/list_icon/index_icon_reply.png DESTINATION ${IMGDIR}/list_icon)
INSTALL(FILES ${PROJECT_IMAGES_DIR}/list_icon/index_icon_forward.png DESTINATION ${IMGDIR}/list_icon)
INSTALL(FILES ${PROJECT_IMAGES_DIR}/file_type_icon/email_icon_amr.png DESTINATION ${IMGDIR}/file_type_icon)
INSTALL(FILES ${PROJECT_IMAGES_DIR}/file_type_icon/email_icon_date.png DESTINATION ${IMGDIR}/file_type_icon)
INSTALL(FILES ${PROJECT_IMAGES_DIR}/file_type_icon/email_icon_eml.png DESTINATION ${IMGDIR}/file_type_icon)
INSTALL(FILES ${PROJECT_IMAGES_DIR}/file_type_icon/email_icon_etc.png DESTINATION ${IMGDIR}/file_type_icon)
INSTALL(FILES ${PROJECT_IMAGES_DIR}/file_type_icon/email_icon_html.png DESTINATION ${IMGDIR}/file_type_icon)
INSTALL(FILES ${PROJECT_IMAGES_DIR}/file_type_icon/email_icon_hwp.png DESTINATION ${IMGDIR}/file_type_icon)
INSTALL(FILES ${PROJECT_IMAGES_DIR}/file_type_icon/email_icon_img.png DESTINATION ${IMGDIR}/file_type_icon)
INSTALL(FILES ${PROJECT_IMAGES_DIR}/file_type_icon/email_icon_movie.png DESTINATION ${IMGDIR}/file_type_icon)
INSTALL(FILES ${PROJECT_IMAGES_DIR}/file_type_icon/email_icon_music.png DESTINATION ${IMGDIR}/file_type_icon)
INSTALL(FILES ${PROJECT_IMAGES_DIR}/file_type_icon/email_icon_pdf.png DESTINATION ${IMGDIR}/file_type_icon)
INSTALL(FILES ${PROJECT_IMAGES_DIR}/file_type_icon/email_icon_ppt.png DESTINATION ${IMGDIR}/file_type_icon)
INSTALL(FILES ${PROJECT_IMAGES_DIR}/file_type_icon/email_icon_rss.png DESTINATION ${IMGDIR}/file_type_icon)
INSTALL(FILES ${PROJECT_IMAGES_DIR}/file_type_icon/email_icon_snb.png DESTINATION ${IMGDIR}/file_type_icon)
INSTALL(FILES ${PROJECT_IMAGES_DIR}/file_type_icon/email_icon_story.png DESTINATION ${IMGDIR}/file_type_icon)
INSTALL(FILES ${PROJECT_IMAGES_DIR}/file_type_icon/email_icon_svg.png DESTINATION ${IMGDIR}/file_type_icon)
INSTALL(FILES ${PROJECT_IMAGES_DIR}/file_type_icon/email_icon_swf.png DESTINATION ${IMGDIR}/file_type_icon)
INSTALL(FILES ${PROJECT_IMAGES_DIR}/file_type_icon/email_icon_txt.png DESTINATION ${IMGDIR}/file_type_icon)
INSTALL(FILES ${PROJECT_IMAGES_DIR}/file_type_icon/email_icon_tpk.png DESTINATION ${IMGDIR}/file_type_icon)
INSTALL(FILES ${PROJECT_IMAGES_DIR}/file_type_icon/email_icon_unknown.png DESTINATION ${IMGDIR}/file_type_icon)
INSTALL(FILES ${PROJECT_IMAGES_DIR}/file_type_icon/email_icon_vcf.png DESTINATION ${IMGDIR}/file_type_icon)
INSTALL(FILES ${PROJECT_IMAGES_DIR}/file_type_icon/email_icon_word.png DESTINATION ${IMGDIR}/file_type_icon)
INSTALL(FILES ${PROJECT_IMAGES_DIR}/file_type_icon/email_icon_xls.png DESTINATION ${IMGDIR}/file_type_icon)
INSTALL(FILES ${PROJECT_IMAGES_DIR}/file_type_icon/email_icon_zip.png DESTINATION ${IMGDIR}/file_type_icon)
