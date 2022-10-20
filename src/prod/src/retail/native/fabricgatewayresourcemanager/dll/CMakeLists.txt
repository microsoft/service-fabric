set( LINUX_SOURCES
  ./dllmain.cpp
  ../../../../pal/wc16.c
)

include_directories(".")

add_library(objects_FabricGatewayResourceManager OBJECT ${LINUX_SOURCES})

add_library(${lib_FabricGatewayResourceManager} SHARED $<TARGET_OBJECTS:objects_FabricGatewayResourceManager>)

set_target_properties(${lib_FabricGatewayResourceManager} PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})

set(EXPORTS_FILE ${CMAKE_CURRENT_BINARY_DIR}/${lib_FabricGatewayResourceManager}.exports)
generate_exports_file(${lib_FabricGatewayResourceManager} "${CMAKE_CURRENT_SOURCE_DIR}/FabricGatewayResourceManager.Linux.def" ${EXPORTS_FILE})

set_target_properties(${lib_FabricGatewayResourceManager} PROPERTIES 
    LINK_FLAGS "-Wl,-Bsymbolic -Wl,--version-script=${EXPORTS_FILE}")

target_link_libraries(
  ${lib_FabricGatewayResourceManager}
  ${lib_GatewayResourceManager}
  ${lib_FabricRuntime}
  ${lib_ClusterManager}
  ${lib_ManagementRepairManager}
  ${lib_FileStoreService}
  ${lib_HealthManager}
  ${lib_InfrastructureService}
  ${lib_ManagementCommon}
  ${lib_StoreService}
  ${lib_Hosting2}
  ${lib_Replication}
  ${lib_TransactionalReplicator}
  ${lib_Failover}
  ${lib_LoadBalancing}
  ${lib_SystemServices}
  ${lib_ServiceGroup}
  ${lib_StoreRepairPolicy}
  ${lib_ImageStore}
  ${lib_ImageModel}
  ${lib_Query}
  ${lib_Federation}
  ${lib_LeaseAgent}
  ${lib_Lease}
  ${lib_Client}
  ${lib_ClientServerTransport}
  ${lib_Transport}
  ${lib_ApiDefinitions}
  ${lib_ApiWrappers}
  ${lib_Common}
  ${lib_ServiceModel}
  ${lib_Serialization}
  ${lib_FabricCommon}
  ${lib_FabricUUID}
  ${lib_InternalFabricUUID}
  ${lib_FabricResources}
  lttng-ust
  ${Cxx}
  ${CxxABI}
  crypto
  rt
)

install(TARGETS ${lib_FabricGatewayResourceManager} DESTINATION ${TEST_OUTPUT_DIR})

