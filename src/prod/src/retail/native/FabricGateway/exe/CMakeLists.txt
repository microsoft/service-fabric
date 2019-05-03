include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_FabricGateway}
  ../FabricGatewayResource.cpp
  ../Main.cpp
  ../stdafx.cpp
  )

add_precompiled_header(${exe_FabricGateway} ../stdafx.h)

set_target_properties(${exe_FabricGateway} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}) 

set(Casablanca_LIBRARY ${LIB}cpprest)

target_link_libraries(${exe_FabricGateway}
  ${lib_Communication}
  ${lib_ManagementSubsystem}
  ${lib_FabricGateway}
  ${lib_httpgateway}
  ${lib_httptransport}
  ${Casablanca_LIBRARIES}  
  ${BoostFileSystem}
  ${lib_FabricGateway}
  ${lib_Hosting2}
  ${lib_StoreService}
  ${lib_EntreeService}
  ${lib_StoreService}
  ${lib_Client}
  ${lib_ManagementRepairManager}
  ${lib_ManagementCommon}
  ${lib_ImageStore}
  ${lib_FileStoreService}
  ${lib_Store}
  ${lib_KtlLoggerNode}
  ${lib_TStore}
  ${lib_Replication}
  ${lib_Query}
  ${lib_Failover}
  ${lib_NetworkInventoryManager}
  ${lib_SystemServices}
  ${lib_ImageModel}
  ${lib_Federation}
  ${lib_LeaseAgent}
  ${lib_Lease}  
  ${lib_Transport}
  ${lib_AadWrapperServer}
  ${lib_ClientServerTransport}
  ${lib_Common}
  ${lib_ServiceModel}
  ${lib_ApiWrappers}
  ${lib_Serialization}
  ${lib_FabricCommon}
  ${lib_InternalFabricUUID}
  ${lib_FabricUUID}
  ${lib_FabricResources}
  ${KtlUser}
  ${Cxx}
  ${CxxABI}
  ssh2
  minizip
  z
  ssl
  crypto
  m
  rt
  pthread
  dl
  xml2
  uuid
)

