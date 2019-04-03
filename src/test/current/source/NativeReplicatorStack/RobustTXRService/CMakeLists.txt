set (exe_robusttxr "RobustTXRService.exe" CACHE STRING "Native E2E Robust Test")

add_executable(${exe_robusttxr}
  ../NightWatchTXRService/PerfResult.cpp
  ../NightWatchTXRService/Service.cpp
  ../NightWatchTXRService/TestAction.cpp
  ../NightWatchTXRService/TestRunner.cpp
  ../NightWatchTXRService/TestStatus.cpp
  main.cpp
  RobustService.cpp
  RobustTestRunner.cpp
)

#add_precompiled_header(${exe_robusttxr} stdafx.h)

target_link_libraries(${exe_robusttxr}
  ${lib_TransactionalReplicator}
  ${lib_txnreplicator_testcommon}
  ${lib_data_utilities}
  ${lib_httptransport}
  ${lib_StatefulServiceBase}
  ${lib_Common}
  ${lib_ServiceModel}
  ${Ktlfull}
  ${lib_FabricRuntime}
  ${lib_FabricCommon}
  ${lib_InternalFabricUUID}
  ${lib_FabricUUID}
  ssh2
  ssl
  crypto
  minizip
  z
  m
  rt
  pthread
  c
  dl
  xml2
  uuid
) 

set_target_properties(${exe_robusttxr} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})