include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_TcpServiceCommunication.Test}
  # boost.test main
  ../../../../test/BoostUnitTest/btest.cpp

  # test code
  ../tcpserviceommunication.test.cpp
  ../comfabricservicecommunication.test.cpp
  ../dummyservice.cpp
  ../comdummyservice.cpp
  ../comfabrictransport.test.cpp
  ../ComFabricTransportDummyHandler.cpp
  ../ComFabricTransportDummyCallbackHandler.cpp
)

add_precompiled_header(${exe_TcpServiceCommunication.Test} ../stdafx.h) 

set_target_properties(${exe_TcpServiceCommunication.Test} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}) 

target_link_libraries(${exe_TcpServiceCommunication.Test}
  ${lib_TcpServiceCommunication}
  ${lib_ApiWrappers}
  ${lib_Transport}
  ${lib_AadWrapperServer}
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
  jemalloc
  pthread
  dl
  xml2
  uuid
)

install(
    FILES ./TcpServiceCommunication.Test.exe.cfg
    DESTINATION ${TEST_OUTPUT_DIR}
)
