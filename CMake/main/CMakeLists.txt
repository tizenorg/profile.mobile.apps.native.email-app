CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(email C)

SET(VENDOR "tizen")
SET(PACKAGE ${PROJECT_NAME})
SET(PKGNAME "org.${VENDOR}.${PACKAGE}")

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpie")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie")

IF(${BUILD_SHARED_LIBS})
	ADD_SUBDIRECTORY(email-single)
	ADD_SUBDIRECTORY(email-setting)
	ADD_SUBDIRECTORY(email-composer)
ELSE(${BUILD_SHARED_LIBS})
	ADD_SUBDIRECTORY(email)
ENDIF(${BUILD_SHARED_LIBS})

# install manifest file
INSTALL(FILES ${PROJECT_ROOT_DIR}/${PKGNAME}.xml DESTINATION ${PACKAGEDIR})

# install license file
INSTALL(FILES ${PROJECT_ROOT_DIR}/LICENSE DESTINATION ${LICENSEDIR} RENAME ${PKGNAME})

IF(${TIZEN_2_4})
# install rule, include file
INSTALL(FILES ${PROJECT_ROOT_DIR}/${PKGNAME}.efl DESTINATION ${SMACKDIR})
# install manifest
CONFIGURE_FILE(${PROJECT_ROOT_DIR}/${PKGNAME}-2.4.manifest ${CMAKE_BINARY_DIR}/${PKGNAME}.manifest)
ELSE(${TIZEN_2_4})
# install manifest
CONFIGURE_FILE(${PROJECT_ROOT_DIR}/${PKGNAME}-3.0.manifest ${CMAKE_BINARY_DIR}/${PKGNAME}.manifest)
ENDIF(${TIZEN_2_4})

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
