set (lib_ReliableCollectionServiceStandalone "ReliableCollectionServiceStandalone" CACHE STRING "ReliableCollectionRuntime implementation library to run without cluster")

set( LINUX_SOURCES
    ../stdafx.cpp
    ../ReliableCollectionServiceStandalone.cpp
)

include_directories("..")

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
    LINK_FLAGS "-Wl,-Bsymbolic -Wl,--version-script=${EXPORTS_FILE}")

target_link_libraries(
  ${lib_ReliableCollectionServiceStandalone}
  ${lib_data_integration_test}
  ${lib_ReliableCollectionRuntimeImpl}
  ${lib_ReliableCollectionRuntime}
  ${lib_TStore}
  ${lib_data_testcommon}
  ${lib_txnreplicator_testcommon}
  ${lib_Replication}
  ${lib_TransactionalReplicator}
  ${lib_ServiceModel}
  ${lib_Serialization}
  ${lib_FabricCommon}
  ${lib_InternalFabricUUID}
  ${lib_FabricUUID}
  ${lib_FabricResources}
  ${KtlUser}
  ${Cxx}
  ${CxxABI}
  ${lib_Transport}
  ${lib_ApiWrappers}
  ${lib_Common}
  minizip
  z
  rt
  pthread
)

install(TARGETS ${lib_ReliableCollectionServiceStandalone} DESTINATION ${TEST_OUTPUT_DIR})
