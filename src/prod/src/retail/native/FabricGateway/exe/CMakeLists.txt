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
  ${lib_StoreService}
  ${lib_FabricGateway}
  ${lib_httpgateway}
  ${lib_httptransport}
  ${Casablanca_LIBRARIES}  
  ${BoostFileSystem}
  ${lib_EntreeService}
  ${lib_FabricGateway}
  ${lib_Client}
  ${lib_ManagementRepairManager}
  ${lib_ManagementCommon}
  ${lib_ImageStore}
  ${lib_ApiWrappers}
  ${lib_FileStoreService}
  ${lib_Store}
  ${lib_Query}
  ${lib_Failover}
  ${lib_SystemServices}
  ${lib_ImageModel}
  ${lib_ApiWrappers}
  ${lib_LeaseAgent}
  ${lib_Lease}
  ${lib_Federation}
  ${lib_ClientServerTransport}
  ${lib_Transport}
  ${lib_AadWrapperServer}
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
  unwind
  unwind-x86_64
)

