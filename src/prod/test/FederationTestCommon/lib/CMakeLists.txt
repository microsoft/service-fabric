set( LINUX_SOURCES
    ../AddressHelper.cpp
    ../CommonDispatcher.cpp
    ../FederationTestCommonConfig.cpp
   )

include_directories("..")

add_library(objects_FederationTestCommon OBJECT ${LINUX_SOURCES})
add_library(${lib_FederationTestCommon} STATIC $<TARGET_OBJECTS:objects_FederationTestCommon>)

set_target_properties(${lib_FederationTestCommon} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})

