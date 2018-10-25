include_directories("..")
include_directories("../../../ktllogger/sys/ktlshim")
include_directories("../../../ktllogger/sys/inc")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_ReplicationPerf.Test}
  # boost.test main
  ../../../../test/BoostUnitTest/btest.cpp

  # test code
    ../ComTestStatefulServicePartition.cpp
    ../ComTestStateProvider.cpp
    ../ComReplicatePerfTest.cpp
  )

add_precompiled_header(${exe_ReplicationPerf.Test} ../stdafx.h)

set_target_properties(${exe_ReplicationPerf.Test} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}) 

target_link_libraries(${exe_ReplicationPerf.Test}
  ${lib_KComAdapter}
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
    FILES ./ReplicationPerf.Test.exe.cfg
    DESTINATION ${TEST_OUTPUT_DIR}
)
