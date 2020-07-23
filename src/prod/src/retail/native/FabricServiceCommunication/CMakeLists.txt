set( LINUX_SOURCES
  ./dllmain.cpp
  ../../../pal/wc16.c
)

include_directories(".")

add_library(objects_FabricServiceCommunication OBJECT ${LINUX_SOURCES})

#add_precompiled_header(objects_FabricServiceCommunication ./stdafx.h)

add_library(${lib_FabricServiceCommunication} SHARED $<TARGET_OBJECTS:objects_FabricServiceCommunication>)

set_target_properties(${lib_FabricServiceCommunication} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

set(EXPORTS_FILE ${CMAKE_CURRENT_BINARY_DIR}/${lib_FabricServiceCommunication}.exports)
generate_exports_file(${lib_FabricServiceCommunication} "${CMAKE_CURRENT_SOURCE_DIR}/FabricServiceCommunication.Linux.def" ${EXPORTS_FILE})

set_target_properties(${lib_FabricServiceCommunication} PROPERTIES 
    LINK_FLAGS "-Wl,-Bsymbolic -Wl,--version-script=${EXPORTS_FILE}")

target_link_libraries(
  ${lib_FabricServiceCommunication}
  ${lib_FabricNode}
  ${lib_Communication}
  ${lib_TcpServiceCommunication}
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
  ${lib_FileStoreService}
  ${lib_Store}
  ${lib_ApiWrappers}
  ${lib_StoreRepairPolicy}
  ${lib_Replication}
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
  z
  rt
  pthread
)

install(TARGETS ${lib_FabricServiceCommunication} DESTINATION ${TEST_OUTPUT_DIR})
