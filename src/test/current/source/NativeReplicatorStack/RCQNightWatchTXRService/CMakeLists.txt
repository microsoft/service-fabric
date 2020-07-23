set (exe_rcqnightwatch "RCQNightWatchTXRService.exe" CACHE STRING "RCQ nightwatch Perf test")

add_executable(${exe_rcqnightwatch}
  ../NightWatchTXRService/PerfResult.cpp
  ../NightWatchTXRService/Service.cpp
  ../NightWatchTXRService/TestAction.cpp
  ../NightWatchTXRService/TestRunner.cpp
  ../NightWatchTXRService/TestStatus.cpp
  main.cpp
  TestComStateProvider2Factory.cpp
  RCQService.cpp
  RCQTestRunner.cpp
)

#add_precompiled_header(${exe_rcqnightwatch} stdafx.h)

target_link_libraries(${exe_rcqnightwatch}
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

set_target_properties(${exe_rcqnightwatch} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})
