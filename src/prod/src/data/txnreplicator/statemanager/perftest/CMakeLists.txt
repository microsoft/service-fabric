include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_statemanager_perftest}
  ${PROJECT_SOURCE_DIR}/test/BoostUnitTest/btest.cpp  
  ../MockLoggingReplicator.cpp
  ../StateManager.AddPerfTest.cpp
  ../StateManager.CheckpointPerfTest.cpp
  ../StateManager.CopyPerfTest.cpp
  ../StateManager.RecoverPerfTest.cpp
  ../StateManager.RemovePerfTest.cpp
  ../StateManagerTestBase.cpp
  ../TestHelper.cpp
  ../TestOperation.cpp
  ../TestLockContext.cpp
  ../TestOperationContext.cpp
  ../TestOperationDataStream.cpp
  ../TestReplicatedOperation.cpp
  ../TestStateManagerChangeHandler.cpp
  ../TestStateProvider.cpp
  ../TestStateProviderFactory.cpp
  ../TestStateProviderProperties.cpp
  ../TestTransaction.cpp
  ../TestTransactionManager.cpp
)

#add_precompiled_header(${exe_statemanager_perftest} ../stdafx.h)

set_target_properties(${exe_statemanager_perftest} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR} 
    LINK_FLAGS "-Wl,--allow-multiple-definition")

target_link_libraries(${exe_statemanager_perftest}
  ${lib_statemanager}
  ${lib_LoggingReplicator}
  ${lib_txnreplicator_common}
  ${lib_data_testcommon}
  ${lib_data_utilities}
  ${lib_ServiceModel}
  ${lib_Common}
  ${lib_Serialization}
  ${lib_KComAdapter}
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
  pthread
  c
  dl
  xml2
  uuid
)

install(
    FILES ./statemanager.perftest.exe.cfg
    DESTINATION ${TEST_OUTPUT_DIR}
    RENAME ${exe_statemanager_perftest}.cfg
)
