include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_data_utilities_test}
  ${PROJECT_SOURCE_DIR}/test/BoostUnitTest/btest.cpp  
  ../AsyncLock.Test.cpp
  ../BinaryReaderWriter.Test.cpp
  ../ConcurrentDictionary.Test.cpp
  ../CRC64.cpp
  ../CRC64.Test.cpp
  ../FileFormat.Test.cpp
  ../KHashSet.Test.cpp
  ../KPath.Test.cpp
  ../LockManager.Test.cpp
  ../PartitionedReplicaTraceComponent.Test.cpp
  ../ReaderWriterAsyncLock.Test.cpp
  ../ConcurrentSkipList.Test.cpp
  ../Sort.Test.cpp
  ../StatusConverter.Test.cpp
  ../ConcurrentDictionaryTest.cpp
)

add_precompiled_header(${exe_data_utilities_test} ../stdafx.h)

set_target_properties(${exe_data_utilities_test} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR} 
    LINK_FLAGS "-Wl,--allow-multiple-definition")

target_link_libraries(${exe_data_utilities_test}
  ${lib_data_utilities}
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
    FILES ./data.utilities.test.exe.cfg
    DESTINATION ${TEST_OUTPUT_DIR}
    RENAME ${exe_data_utilities_test}.cfg
)
