include_directories("../../driver/inc/")
include_directories("../inc/")

add_definitions(-DETL_TRACING_ENABLED)

set( SOURCES
  SFBDCtlLib.cpp
  SFBDCtlLibTrace.cpp
)

add_library(objects_BlockStore OBJECT ${SOURCES})
add_library(${lib_BlockStore} SHARED $<TARGET_OBJECTS:objects_BlockStore>)

set(EXPORTS_FILE ${CMAKE_CURRENT_BINARY_DIR}/${lib_BlockStore}.exports)
generate_exports_file(${lib_BlockStore} "${CMAKE_CURRENT_SOURCE_DIR}/SFBDCtlLib.def" ${EXPORTS_FILE})

set_target_properties(${lib_BlockStore} PROPERTIES
    LINK_FLAGS "-Wl,-Bsymbolic -Wl,--version-script=${EXPORTS_FILE}")

target_link_libraries(${lib_BlockStore}
    ${lib_FabricCommon}
)

#add_precompiled_header(objects_BlockStore ../stdafx.h)

set_target_properties( ${lib_BlockStore} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR} )
