set( LINUX_SOURCES
    ../stdafx.cpp
    ../TestabilityConfig.cpp
    ../TestabilitySubsystem.cpp
    ../UnreliableLeaseHelper.cpp
    ../UnreliableTransportHelper.cpp
    ../NodeLevelHealthReportHelper.cpp
)

include_directories("..")

add_library(objects_Testability OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_Testability ../stdafx.h)

add_library(${lib_Testability} STATIC $<TARGET_OBJECTS:objects_Testability>)

set_target_properties(${lib_Testability} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

