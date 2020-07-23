set (lib_ReliableCollectionServiceStandalone "ReliableCollectionServiceStandalone" CACHE STRING "ReliableCollectionRuntime implementation library to run without cluster")

set( LINUX_SOURCES
    ../stdafx.cpp
    ../ReliableCollectionServiceStandalone.cpp
    ../TraceTemplates.Linux.c
)

include_directories("..")
include_directories("../..")

include_directories("../../../../retail/native/FabricCommon")
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_custom_target(
    rcstringtbl.inc 
    COMMAND perl ${CMAKE_CURRENT_SOURCE_DIR}/../../../../resources/dll/resgenl.pl
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../inc/resourceids.h
    ${CMAKE_CURRENT_BINARY_DIR}/rcstringtbl.inc
    ${CMAKE_CURRENT_BINARY_DIR}/rcstringtbl.inc.tmp
)

# Build position independent to be included in shared libraries
add_compile_options(-fPIC)

add_library(objects_ReliableCollectionServiceStandalone OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_ReliableCollectionServiceStandalone ../stdafx.h)

add_library(${lib_ReliableCollectionServiceStandalone} SHARED $<TARGET_OBJECTS:objects_ReliableCollectionServiceStandalone>)

set_target_properties(${lib_ReliableCollectionServiceStandalone} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

set(EXPORTS_FILE ${CMAKE_CURRENT_BINARY_DIR}/${lib_ReliableCollectionServiceStandalone}.exports)
generate_exports_file(${lib_ReliableCollectionServiceStandalone} "${CMAKE_CURRENT_SOURCE_DIR}/ReliableCollectionServiceStandalone.def" ${EXPORTS_FILE})

set_target_properties(${lib_ReliableCollectionServiceStandalone} PROPERTIES 
    LINK_FLAGS "-v -z defs -Wl,-Bsymbolic -Wl,--version-script=${EXPORTS_FILE}")

target_link_libraries(
  ${lib_ReliableCollectionServiceStandalone}
  ${lib_data_integration_test}
  ${lib_ReliableCollectionRuntimeImpl}
  ${lib_TStore}
  ${lib_data_testcommon}
  ${lib_txnreplicator_testcommon}
  ${lib_Replication}
  ${lib_TransactionalReplicator}
  ${lib_ServiceModel}
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
  ${lib_ApiWrappers}
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

add_dependencies(objects_ReliableCollectionServiceStandalone rcstringtbl.inc)

install(TARGETS ${lib_ReliableCollectionServiceStandalone} DESTINATION ${TEST_OUTPUT_DIR})
