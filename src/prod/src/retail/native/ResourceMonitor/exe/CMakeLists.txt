add_executable(${exe_ResourceMonitor}
  ../Main.cpp
  ../stdafx.cpp
)

add_precompiled_header(${exe_ResourceMonitor} ../stdafx.h FORCEINCLUDE)

set_target_properties(${exe_ResourceMonitor} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}) 

add_custom_target(CopyFabricResourceMonitorService ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${TEST_OUTPUT_DIR}/__FabricSystem_App4294967295/ResourceMonitorService.Code.Current
    COMMAND ${CMAKE_COMMAND} -E copy ${TEST_OUTPUT_DIR}/${exe_ResourceMonitor} ${TEST_OUTPUT_DIR}/__FabricSystem_App4294967295/ResourceMonitor.Code.Current/ResourceMonitor.exe
    COMMAND ${CMAKE_STRIP} ${TEST_OUTPUT_DIR}/__FabricSystem_App4294967295/ResourceMonitor.Code.Current/ResourceMonitor.exe
    DEPENDS ${exe_ResourceMonitor}
)

target_link_libraries(${exe_ResourceMonitor}
  ${lib_ResourceMonitor}
  ${lib_ResourceMonitorConfig}
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
  ${lib_TestHooks}
  ${lib_KtlLoggerNode}
  ${lib_TStore}
  ${lib_ApiWrappers}
  ${lib_StoreRepairPolicy}
  ${lib_Replication}
  ${lib_TransactionalReplicator}
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
  ${lib_FabricRuntime}
  ${lib_FabricCommon}
  ${Cxx}
  ${CxxABI}
  ssh2
  ssl
  crypto
  m
  rt
  jemalloc
  pthread
  dl
  xml2
  uuid
)
