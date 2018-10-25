include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_ReliabilityCommonTest}
  # boost.test main
  ../../../../test/BoostUnitTest/btest.cpp

  # test code
  ../Sample.Test.cpp
  )

set_target_properties(${exe_ReliabilityCommonTest} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}) 

target_link_libraries(${exe_ReliabilityCommonTest}
  ${lib_Common}
  ${lib_Serialization}
  ${lib_ServiceModel} 
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
  ${BoostTest2}
)

install(
    FILES ./Reliability.Common.Test.exe.cfg
    DESTINATION ${TEST_OUTPUT_DIR}
)
