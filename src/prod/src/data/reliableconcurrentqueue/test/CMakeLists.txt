set (exe_ReliableConcurrentQueue_Test "ReliableConcurrentQueue.Test.exe" CACHE STRING "ReliableConcurrentQueue.Test Exe")

include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_ReliableConcurrentQueue_Test}
  ${PROJECT_SOURCE_DIR}/test/BoostUnitTest/btest.cpp  
  ${PROJECT_SOURCE_DIR}/src/data/tstore/MockStateManager.cpp
  ${PROJECT_SOURCE_DIR}/src/data/tstore/TestTransactionContext.cpp
  ${PROJECT_SOURCE_DIR}/src/data/tstore/MockTransactionalReplicator.cpp
  ../ReliableConcurrentQueue.BasicOperations.Test.cpp
  ../ReliableConcurrentQueue.RecoveryApply.Test.cpp
  ../ReliableConcurrentQueue.SecondaryApply.Test.cpp
  ../ReliableConcurrentQueue.UndoFalseProgress.Test.cpp
  ../ReliableConcurrentQueue.Stress.Test.cpp
  ../ReliableConcurrentQueue.StateProvider.Test.cpp
)

add_precompiled_header(${exe_ReliableConcurrentQueue_Test} ../stdafx.h FORCEINCLUDE)

set_target_properties(${exe_ReliableConcurrentQueue_Test} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}
    LINK_FLAGS "-Wl,--allow-multiple-definition")

target_link_libraries(${exe_ReliableConcurrentQueue_Test}
  ${lib_ReliableConcurrentQueue}
  ${lib_TStore}
  ${lib_data_testcommon}
  ${lib_ServiceModel}
  ${lib_Common}
  ${lib_Serialization}
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
    FILES ./reliableconcurrentqueue.test.exe.cfg
    DESTINATION ${TEST_OUTPUT_DIR}
    RENAME ${exe_ReliableConcurrentQueue_Test}.cfg
)
