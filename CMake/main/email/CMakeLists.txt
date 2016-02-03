CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(email C)

SET(SRCS ${PROJECT_FILES_DIR}/main/email-main.c)

ADD_DEFINITIONS("-DAPP_USE_SIG")

ADD_EXECUTABLE(${PROJECT_NAME} ${SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME}
	email-common
	email-account-module
	email-composer-module
	email-filter-module
	email-mailbox-module
	email-setting-module
	email-viewer-module
	email-common
	${pkgs-email_LDFLAGS})

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${BINDIR})
