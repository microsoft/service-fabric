set( LINUX_SOURCES
  ./dllmain.cpp
  ../../../../pal/wc16.c
)

# Build position independent to be included in shared libraries
add_compile_options(-fPIC)

include_directories(".")

add_library(objects_FabricRuntime OBJECT ${LINUX_SOURCES})

#add_precompiled_header(objects_FabricRuntime ./stdafx.h)

add_library(${lib_FabricRuntime} SHARED $<TARGET_OBJECTS:objects_FabricRuntime>)

set_target_properties(${lib_FabricRuntime} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

set(EXPORTS_FILE ${CMAKE_CURRENT_BINARY_DIR}/${lib_FabricRuntime}.exports)
generate_exports_file(${lib_FabricRuntime} "${CMAKE_CURRENT_SOURCE_DIR}/FabricRuntime.Linux.def" ${EXPORTS_FILE})

set_target_properties(${lib_FabricRuntime} PROPERTIES 
    LINK_FLAGS "-Wl,-Bsymbolic -Wl,--version-script=${EXPORTS_FILE}")

target_link_libraries(
  ${lib_FabricRuntime}
  ${lib_ReliableCollectionRuntimeImpl}
  ${lib_FabricNode}
  ${lib_Communication}
  ${lib_ManagementSubsystem}
  ${lib_UpgradeService}
  ${lib_Client}
  ${lib_ManagementCommon}
  ${lib_ManagementRepairManager}
  ${lib_HealthManager}
  ${lib_ClusterManager}
  ${lib_ImageStore}
  ${lib_EntreeService}
  ${lib_StoreService}
  ${lib_FabricGateway}
  ${lib_Hosting2}
  ${lib_Testability}
  ${lib_FileStoreService}
  ${lib_Store}
  ${lib_ReliableConcurrentQueue}
  ${lib_TestHooks}
  ${lib_KtlLoggerNode}
  ${lib_TStore}
  ${lib_StoreRepairPolicy}
  ${lib_Replication}
  ${lib_TransactionalReplicator}
  ${lib_ApiWrappers}
  ${lib_Failover}
  ${lib_NetworkInventoryManager}
  ${lib_LoadBalancing}
  ${lib_SystemServices}
  ${lib_ServiceGroup}
  ${lib_ImageModel}
  ${lib_Query}
  ${lib_LeaseAgent}
  ${lib_Lease}
  ${lib_Federation}
  ${lib_ClientServerTransport}
  ${lib_Transport}
  ${lib_Client}
  ${lib_Common}
  ${lib_ServiceModel}
  ${lib_Serialization}
  ${lib_FabricCommon}
  ${lib_InternalFabricUUID}
  ${lib_FabricUUID}
  ${lib_FabricResources}
  ${KtlUser}
  ${Cxx}
  ${CxxABI}
  minizip
  z
  rt
  pthread
)

install(TARGETS ${lib_FabricRuntime} DESTINATION ${TEST_OUTPUT_DIR})

