set( LINUX_SOURCES
    ../stdafx.cpp
    ../ComFactory.cpp
    ../Factory.cpp
    ../Helpers.cpp
    ../StatefulServiceBase.cpp
    ../TestComProxyDataLossHandler.cpp
    ../EndpointsDescription.cpp
)

include_directories("..")

# Build position independent to be included in shared libraries
add_compile_options(-fPIC)

add_library(objects_ReliableCollectionService OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_ReliableCollectionService ../stdafx.h)

add_library(${lib_ReliableCollectionService} SHARED $<TARGET_OBJECTS:objects_ReliableCollectionService>)

set_target_properties(${lib_ReliableCollectionService} PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

set(EXPORTS_FILE ${CMAKE_CURRENT_BINARY_DIR}/${lib_ReliableCollectionService}.exports)
generate_exports_file(${lib_ReliableCollectionService} "${CMAKE_CURRENT_SOURCE_DIR}/ReliableCollectionService.def" ${EXPORTS_FILE})

set_target_properties(${lib_ReliableCollectionService} PROPERTIES 
    LINK_FLAGS "-Wl,-Bsymbolic -Wl,--version-script=${EXPORTS_FILE}")

target_link_libraries(
  ${lib_ReliableCollectionService}
  ${lib_FabricRuntime}
  ${lib_ReliableCollectionRuntime}
  ${lib_ImageModel}
  ${lib_Query}
  ${lib_LeaseAgent}
  ${lib_Lease}
  ${lib_Client}
  ${lib_Common}
  ${lib_ServiceModel}
  ${lib_Serialization}
  ${lib_FabricCommon}
  ${lib_InternalFabricUUID}
  ${lib_FabricUUID}
  ${lib_FabricResources}
  ${lib_DnsServiceConfig}
  ${KtlUser}
  ${Cxx}
  ${CxxABI}
  minizip
  z
  rt
  pthread
)

