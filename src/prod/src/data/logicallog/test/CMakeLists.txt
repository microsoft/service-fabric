include_directories("..")
include_directories("../../../ktllogger/sys/inc")
include_directories("../../../ktllogger/sys/ktlshim")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)
add_definitions(-DUPASSTHROUGH=1)

add_executable(${exe_logicallog_test}
  ${PROJECT_SOURCE_DIR}/test/BoostUnitTest/btest.cpp
  ../LogTestBase.cpp
  ../FileLogicalLog.Test.cpp
  ../LogicalLog.Test.cpp
)

add_precompiled_header(${exe_logicallog_test} ../stdafx.h)

target_compile_definitions(${exe_logicallog_test}
    PUBLIC "UPASSTHROUGH=1"
    )

set_target_properties(${exe_logicallog_test} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR} 
    LINK_FLAGS "-Wl,--allow-multiple-definition")

target_link_libraries(${exe_logicallog_test}
  ${lib_LoggingReplicator}
  ${lib_LogicalLog}
  ${lib_KtlLoggerShimUnify}
  ${lib_data_testcommon}
  ${lib_data_utilities}
  ${lib_ServiceModel}
  ${lib_Common}
  ${lib_Serialization}
  ${lib_TestCommon}
  ${BoostTest2}
  KtlLoggerUser
  ${Ktlfull}
  ${Cxx}
  ${CxxABI}
  ${lib_FabricCommon}
  ${lib_FabricResources}
  ssh2
  ssl
  crypto
  m
  rt
  pthread
  c
  dl
  xml2
  uuid
)

install(
    FILES ./logicallog.test.exe.cfg
    DESTINATION ${TEST_OUTPUT_DIR}
    RENAME ${exe_logicallog_test}.cfg
)
