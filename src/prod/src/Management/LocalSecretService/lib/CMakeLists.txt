set( LINUX_SOURCES
    ../LocalSecretServiceConfig.cpp
    ../LocalSecretServiceInstance.cpp
    ../LocalSecretServiceFactory.cpp
)

include_directories("..")

add_library(objects_LocalSecretService OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_LocalSecretService ../stdafx.h)

add_library(${lib_LocalSecretService} STATIC $<TARGET_OBJECTS:objects_LocalSecretService>)

set_target_properties(${lib_LocalSecretService} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})
