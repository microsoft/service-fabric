include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_Transport.PerfTest.Client}
  # test code
  ../stdafx.cpp
  ../TestCommon.cpp
  ../PerfTestClient.cpp
  )

add_precompiled_header(${exe_Transport.PerfTest.Client} ../stdafx.h)

set_target_properties(${exe_Transport.PerfTest.Client} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}) 

target_link_libraries(${exe_Transport.PerfTest.Client}
  ${lib_Serialization}
  ${lib_Transport}
  ${lib_Common}
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
  pthread
  c
  dl
  xml2
  uuid
  ${BoostTest2}
)

install(
    FILES ./Transport.PerfTest.Client.exe.cfg
    DESTINATION ${TEST_OUTPUT_DIR}
    RENAME ${exe_Transport.PerfTest.Client}.cfg
)
