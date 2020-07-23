set( LINUX_SOURCES
  ../TestableServiceBase.cpp
)

include_directories("..")

add_library(objects_TestableService OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_TestableService ../stdafx.h)
add_library(${lib_TestableService} STATIC $<TARGET_OBJECTS:objects_TestableService>)

target_link_libraries(${lib_TestableService}
    ${Ktlfull}
    ${lib_httptransport}
)

set_target_properties(${lib_TestableService} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})