set (exe_TStore_Stress "TStore.Stress.exe" CACHE STRING "TStore Stress Test")

include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_TStore_Stress}
  ${PROJECT_SOURCE_DIR}/test/BoostUnitTest/btest.cpp
  ../Checkpoint.Stress.cpp
  ../Close.Stress.cpp
  ../Enumeration.Stress.cpp
  ../KBufferComparer.cpp
  ../Merge.Stress.cpp
  ../MockStateManager.cpp
  ../MockTransactionalReplicator.cpp
  ../TestTransactionContext.cpp
)

add_precompiled_header(${exe_TStore_Stress} ../stdafx.h FORCEINCLUDE)

set_target_properties(${exe_TStore_Stress} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}
    LINK_FLAGS "-Wl,--allow-multiple-definition")

target_link_libraries(${exe_TStore_Stress}
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
    FILES ./tstore.stress.exe.cfg
    DESTINATION ${TEST_OUTPUT_DIR}
    RENAME ${exe_TStore_Stress}.cfg
)
