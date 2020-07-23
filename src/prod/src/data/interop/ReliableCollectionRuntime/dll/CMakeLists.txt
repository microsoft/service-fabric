set( LINUX_SOURCES
  ./ReliableCollectionRuntime.cpp
)

# Build position independent to be included in shared libraries
add_compile_options(-fPIC)

include_directories(".")

add_library(objects_ReliableCollectionRuntime OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_ReliableCollectionRuntime ./stdafx.h)

add_library(${lib_ReliableCollectionRuntime} SHARED $<TARGET_OBJECTS:objects_ReliableCollectionRuntime>)

set_target_properties(${lib_ReliableCollectionRuntime} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

set(EXPORTS_FILE ${CMAKE_CURRENT_BINARY_DIR}/${lib_ReliableCollectionRuntime}.exports)
generate_exports_file(${lib_ReliableCollectionRuntime} "${CMAKE_CURRENT_SOURCE_DIR}/ReliableCollectionRuntime.def" ${EXPORTS_FILE})

set_target_properties(${lib_ReliableCollectionRuntime} PROPERTIES 
    LINK_FLAGS "-Wl,-Bsymbolic -Wl,--version-script=${EXPORTS_FILE}")

# These options are used to force every object to be included even if it's unused.

target_link_libraries( ${lib_ReliableCollectionRuntime}
  ${lib_Common}
  ${Ktlfull}
  ${lib_Pal}
  ${lib_FabricCommonStatic}
  ${lib_Federation}
  ${lib_Transport}
  ${lib_ImageModel}
  ${lib_ServiceModel}
  ${lib_Common}
  ${lib_Serialization}
  ${lib_InternalFabricUUID}
  ${lib_FabricUUID}
  ${lib_FabricResourcesStatic}
  ${KtlUser}
  ${Cxx}
  ${CxxABI}
  lttng-ust
  ssh2
  ssl
  crypto
  minizip
  z
  xml2
  uuid
  rt
  dl
  pthread
)

install(TARGETS ${lib_ReliableCollectionRuntime} DESTINATION ${TEST_OUTPUT_DIR})

