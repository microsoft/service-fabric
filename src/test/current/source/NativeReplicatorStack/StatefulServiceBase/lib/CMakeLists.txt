set( LINUX_SOURCES
  ../ComAsyncOperationCallbackHelper.cpp
  ../ComFactory.cpp
  ../Factory.cpp
  ../Helpers.cpp
  ../StatefulServiceBase.cpp
  ../TestClientRequestHandler.cpp
)

include_directories("..")

add_library(objects_StatefulServiceBase OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_StatefulServiceBase ../stdafx.h)
add_library(${lib_StatefulServiceBase} STATIC $<TARGET_OBJECTS:objects_StatefulServiceBase>)

target_link_libraries(${lib_StatefulServiceBase}
    ${Ktlfull}
    ${lib_httptransport}
)

set_target_properties(${lib_StatefulServiceBase} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})