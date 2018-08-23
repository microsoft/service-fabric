include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_httpgatewayTest}
  # boost.test main
  ../../../test/BoostUnitTest/btest.cpp

  # test code
  ../GatewayUri.Test.cpp
  ../UriArgumentParser.Test.cpp
  )

add_precompiled_header(${exe_httpgatewayTest} ../stdafx.h)

set_target_properties(${exe_httpgatewayTest} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}) 

target_link_libraries(${exe_httpgatewayTest}
  ${lib_httpgateway}
  ${lib_UpgradeOrchestrationService}
  ${lib_ClientServerTransport}
  ${lib_Client}
  ${lib_Transport}
  ${lib_AadWrapperServer}
  ${lib_Common}
  ${lib_ServiceModel}
  ${lib_Common}
  ${lib_Serialization}
  ${lib_FabricClient}
  ${BoostTest2}
  ${Cxx}
  ${CxxABI}
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
    FILES ./HttpGateway.Test.exe.cfg
    DESTINATION ${TEST_OUTPUT_DIR}
)

