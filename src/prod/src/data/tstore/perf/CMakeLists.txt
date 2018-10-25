set (exe_TStore_Perf "TStore.Perf.exe" CACHE STRING "TStore Performance Test")

include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_TStore_Perf}
  ${PROJECT_SOURCE_DIR}/test/BoostUnitTest/btest.cpp  
  ../BufferBufferStore.Perf.cpp
  ../Checkpoint.Perf.cpp
  ../CheckpointFile.Perf.cpp
  ../DataStructures.Perf.cpp
  ../FileStream.Perf.cpp
  ../KBufferComparer.cpp
  ../LongBufferStore.Perf.cpp
  ../LongLongStore.Perf.cpp
  ../MockStateManager.cpp
  ../MockTransactionalReplicator.cpp
  ../SharedLong.cpp
  ../Store.Stress.Test.cpp
  ../StringBufferStore.Perf.cpp
  ../TestTransactionContext.cpp
)

add_precompiled_header(${exe_TStore_Perf} ../stdafx.h)

set_target_properties(${exe_TStore_Perf} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}
    LINK_FLAGS "-Wl,--allow-multiple-definition")

target_link_libraries(${exe_TStore_Perf}
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
    FILES ./tstore.perf.exe.cfg
    DESTINATION ${TEST_OUTPUT_DIR}
    RENAME ${exe_TStore_Perf}.cfg
)
