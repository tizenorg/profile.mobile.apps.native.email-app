CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpic")

ADD_SUBDIRECTORY(email-mailbox-module)
ADD_SUBDIRECTORY(email-viewer-module)
ADD_SUBDIRECTORY(email-composer-module)
ADD_SUBDIRECTORY(email-account-module)
ADD_SUBDIRECTORY(email-setting-module)
ADD_SUBDIRECTORY(email-filter-module)