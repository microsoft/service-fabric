set( LINUX_SOURCES
    ../BackupRestoreServiceConfig.cpp
)

include_directories("..")

add_library(objects_BackupRestoreService OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_BackupRestoreService ../stdafx.h)
add_library(${lib_BackupRestoreService} STATIC $<TARGET_OBJECTS:objects_BackupRestoreService>)

set_target_properties(${lib_BackupRestoreService} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

