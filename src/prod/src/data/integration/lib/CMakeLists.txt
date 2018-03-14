set (lib_data_integration_test "data.integration.test" CACHE STRING "Data Integration Test Library")

set( LINUX_SOURCES
    ../Replica.cpp
    ../UpgradeStateProviderFactory.cpp
)

include_directories("..")
include_directories("../../../ktllogger/sys/ktlshim")
include_directories("../../../ktllogger/sys/inc")

add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_library(objects_data_integration_test OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_data_integration_test ../stdafx.h)

add_library(${lib_data_integration_test} STATIC $<TARGET_OBJECTS:objects_data_integration_test>)

target_link_libraries(${lib_data_integration_test}
    ${Ktlfull})

set_target_properties(${lib_data_integration_test} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 


