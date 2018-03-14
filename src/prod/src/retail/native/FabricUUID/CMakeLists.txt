set( LINUX_SOURCES
    ../../../prebuilt/sdk/FabricClient_i.c
    ../../../prebuilt/sdk/FabricCommon_i.c
    ../../../prebuilt/sdk/FabricRuntime_i.c
)

add_library(objects_FabricUUID OBJECT ${LINUX_SOURCES})

add_library(${lib_FabricUUID} STATIC $<TARGET_OBJECTS:objects_FabricUUID>)

set_target_properties(${lib_FabricUUID} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

