set (exe_data_integration_test "data.integration.test.exe" CACHE STRING "Data Integration Test")

include_directories("..")
include_directories("../../../ktllogger/sys/ktlshim")
include_directories("../../../ktllogger/sys/inc")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_data_integration_test}
  ${PROJECT_SOURCE_DIR}/test/BoostUnitTest/btest.cpp
  ../Backup.Test.cpp
  ../FactoryArgument.Test.cpp
  ../StateManagerMapInterface.Test.cpp
  ../Replica.cpp
  ../TestBackupCallbackHandler.cpp
)

add_precompiled_header(${exe_data_integration_test} ../stdafx.h)

set_target_properties(${exe_data_integration_test} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}
    LINK_FLAGS "-Wl,--allow-multiple-definition")

target_link_libraries(${exe_data_integration_test}
  ${lib_TStore}
  ${lib_TransactionalReplicator}
  ${lib_data_testcommon}
  ${lib_Replication}
  ${lib_Hosting2}
  ${lib_ServiceModel}
  ${lib_Serialization}
  ${lib_Transport}
  ${lib_Common}
  ${lib_ApiWrappers}
  ${lib_txnreplicator_testcommon}
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
    FILES ./data.integration.test.exe.cfg
    DESTINATION ${TEST_OUTPUT_DIR}
    RENAME ${exe_data_integration_test}.cfg
)
