set( LINUX_SOURCES
    ../UpgradeServiceConfig.cpp
)

include_directories("..")

add_library(objects_UpgradeService OBJECT ${LINUX_SOURCES})
add_library(${lib_UpgradeService} STATIC $<TARGET_OBJECTS:objects_UpgradeService>)

set_target_properties(${lib_UpgradeService} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

