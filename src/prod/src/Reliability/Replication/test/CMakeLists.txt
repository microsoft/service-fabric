include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_Replication.Test}
  # boost.test main
  ../../../../test/BoostUnitTest/btest.cpp

  # test code
    ../ComTestStatefulServicePartition.cpp
    ../ComTestStateProvider.cpp
    ../ComProxyTestReplicator.cpp
    ../CopyAsyncOperation.Test.cpp
    ../ChangeRole.Test.cpp
    ../DecayAverage.Test.cpp
    ../OperationQueue.Test.cpp
    ../PublicApi.Test.cpp
    ../ReliableOperationSender.Test.cpp
    ../ReplicaManager.Test.cpp
    ../ReplicatorSettings.Test.cpp
    ../ReplicationSecondary.Test.cpp
    ../ReplicateCopyOperations.Test.cpp
    ../ReplicationTransport.Test.cpp
    ../SecondaryTraceHandler.Test.cpp
    ../BatchedHealthReporter.Test.cpp
    ../testFabricAsyncCallbackHelper.cpp
  )

add_precompiled_header(${exe_Replication.Test} ../stdafx.h)

set_target_properties(${exe_Replication.Test} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}) 

target_link_libraries(${exe_Replication.Test}
  ${lib_Replication}
  ${lib_Hosting2}
  ${lib_Testability}
  ${lib_Common}
  ${lib_ServiceModel}
  ${lib_Serialization}
  ${lib_Transport}
  ${lib_ApiWrappers}
  ${KtlUser}
  ${BoostTest2}
  ${Cxx}
  ${CxxABI}
  ${lib_FabricCommon}
  ${lib_FabricResources}
  ssh2
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
    FILES ./Replication.Test.exe.cfg
    DESTINATION ${TEST_OUTPUT_DIR}
)

install(
    FILES ./RunReplicationTests.sh
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    DESTINATION ${TEST_OUTPUT_DIR}
)
