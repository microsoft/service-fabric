set( LINUX_SOURCES
  ../FabricCommon.cpp
  ../ConfigLoader.cpp
  ../stdafx.cpp
  ./dllmain.cpp
  ../../../../pal/wc16.c
)

set( LINUX_SOURCES2
  ./TraceTemplates.Linux.c
  ../TraceWrapper.Linux.c
  ../TraceBinaryWrapper.Linux.c
)

add_precompiled_header(${lib_FabricCommon} ../stdafx.h)

include_directories("../pch")

add_library(objects_fabriccommon OBJECT ${LINUX_SOURCES})
add_library(objects_fabriccommon2 OBJECT ${LINUX_SOURCES2})
add_library(${lib_FabricCommon} SHARED $<TARGET_OBJECTS:objects_fabriccommon> $<TARGET_OBJECTS:objects_fabriccommon2>)

set_target_properties(${lib_FabricCommon} PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})

set(EXPORTS_FILE ${CMAKE_CURRENT_BINARY_DIR}/${lib_FabricCommon}.exports)
generate_exports_file(${lib_FabricCommon} "${CMAKE_CURRENT_SOURCE_DIR}/FabricCommon.Linux.def" ${EXPORTS_FILE})

set_target_properties(${lib_FabricCommon} PROPERTIES 
    LINK_FLAGS "-Wl,-Bsymbolic -Wl,--version-script=${EXPORTS_FILE}")

target_link_libraries(${lib_FabricCommon}
  ${lib_Federation}
  ${lib_Common}
  ${lib_Transport}
  ${lib_ImageModel}
  ${lib_ServiceModel}
  ${lib_Serialization}
  ${lib_Common}
  ${lib_InternalFabricUUID}
  ${lib_FabricUUID}
  ${lib_FabricResources}
  ${Ktlfull}
  lttng-ust
  ${Cxx}
  ${CxxABI}
  ssh2
  ssl
  crypto
  xml2
  uuid
  rt
  pthread
)

install(TARGETS ${lib_FabricCommon} DESTINATION ${TEST_OUTPUT_DIR})

