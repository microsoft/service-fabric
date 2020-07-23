include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_FailoverFM.Test}
  # boost.test main
  ../../../../../test/BoostUnitTest/btest.cpp

  # test code
  ../FailoverConfig.Test.cpp
  ../FailoverUnit.Test.cpp 
  ../StateMachineTask.Test.cpp
  ../FailoverManagerStore.Test.cpp
  ../Rebuild.Test.cpp
  ../TestHelper.Test.cpp
  ../FailoverUnitCache.Test.cpp
  ../ServiceLookupTable.Test.cpp
  ../ServiceCache.Test.cpp
  ../TestConstants.cpp
  ../../../../client/TestClientFactory.cpp
  ../../../../client/MockFabricClientImpl.cpp  
)

add_precompiled_header(${exe_FailoverFM.Test} ../stdafx.h)

set_target_properties(${exe_FailoverFM.Test} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}) 

target_link_libraries(${exe_FailoverFM.Test}
  ${lib_FailoverFM}
  ${lib_Federation}
  ${lib_LeaseAgent}
  ${lib_Lease}
  ${lib_Query}
  ${lib_Client}
  ${lib_ClientServerTransport}
  ${lib_Transport}
  ${lib_LoadBalancing}
  ${lib_FailoverCommon}
  ${lib_NetworkInventoryManager}
  ${lib_NetworkInventoryManager}
  ${lib_Store}
  ${lib_TestHooks}
  ${lib_KtlLoggerNode}
  ${lib_Replication}
  ${lib_TStore}
  ${lib_StoreRepairPolicy}
  ${lib_ManagementRepairManager}
  ${lib_Common}
  ${lib_ServiceModel}
  ${lib_ManagementRepairManager}
  ${lib_ServiceModel}
  ${lib_ApiWrappers}
  ${lib_Serialization}
  ${BoostTest2}
  ${Cxx}
  ${CxxABI}
  ${lib_FabricCommon}
  ${lib_FabricResources}
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
    FILES ./FM.Test.exe.cfg
    DESTINATION ${TEST_OUTPUT_DIR}
)

