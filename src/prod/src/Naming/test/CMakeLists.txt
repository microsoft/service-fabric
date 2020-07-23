include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_Naming.Test}
  # boost.test main
  ../../../test/BoostUnitTest/btest.cpp

  # test code
  ../Caching.test.cpp
  ../ClientAndEntreeCommunication.Test.cpp
  ../ComFabricClient.Test.cpp
  ../EseStoreTestHelper.cpp
  ../FauxFM.cpp
  ../FauxStore.cpp
  ../NameRangeTuple.test.cpp
  ../PartitionedServiceDescriptor.Test.cpp
  ../RouterTestHelper.cpp
  ../ServiceGroupDescriptor.test.cpp
  ../ServiceGroupServiceDescription.test.cpp
  ../ServiceNotificationManager.test.cpp
  ../TestNodeWrapper.cpp
  )

add_precompiled_header(${exe_Naming.Test} ../stdafx.h)

set_target_properties(${exe_Naming.Test} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}) 

target_link_libraries(${exe_Naming.Test}
  ${lib_Communication}
  ${lib_ManagementSubsystem}
  ${lib_Client}
  ${lib_ManagementCommon}
  ${lib_ImageStore}
  ${lib_EntreeService}
  ${lib_StoreService}
  ${lib_FabricGateway}
  ${lib_httpgateway}
  ${lib_httptransport}
  ${lib_ManagementRepairManager}
  ${Casablanca_LIBRARIES}
  ${lib_EntreeService}
  ${lib_StoreService}
  ${lib_Hosting2}
  ${lib_Testability}
  ${lib_FileStoreService}
  ${lib_Store}
  ${lib_TestHooks}
  ${lib_KtlLoggerNode}
  ${lib_TStore}
  ${lib_StoreRepairPolicy}
  ${lib_Replication}
  ${lib_TransactionalReplicator}
  ${lib_Failover}
  ${lib_NetworkInventoryManager}
  ${lib_FailoverCommon}
  ${lib_SystemServices}
  ${lib_ImageModel}
  ${lib_Query}
  ${lib_Federation}
  ${lib_LeaseAgent}
  ${lib_Lease}
  ${lib_ClientServerTransport}
  ${lib_Transport}
  ${lib_AadWrapperServer}
  ${lib_Common}
  ${lib_ServiceModel}
  ${lib_ApiWrappers}
  ${lib_Serialization}
  ${lib_FabricCommon}
  ${lib_FabricUUID}
  ${lib_InternalFabricUUID}
  ${lib_FabricResources}
  ${KtlUser}
  ${BoostTest2}
  ${Cxx}
  ${CxxABI}
  ssh2
  z
  ssl
  crypto
  minizip
  z
  m
  rt
  jemalloc
  pthread
  dl
  xml2
  uuid
)

install(
    FILES ./Naming.Test.exe.cfg
    DESTINATION ${TEST_OUTPUT_DIR}
    RENAME ${exe_Naming.Test}.cfg
)
