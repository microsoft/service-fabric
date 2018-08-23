include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_Federation.Test}
  # boost.test main
  ../../../test/BoostUnitTest/btest.cpp

  # test code
    ../FederationSubsystem.test.cpp
    ../NodeId.Test.cpp
    ../NodeIdRange.Test.cpp
    ../NodeIdRangeTable.Test.cpp
    ../RoutingTable.Test.cpp
    ../RoutingToken.Test.cpp
    ../FederationConfig.Test.cpp
    ../SiteNodeHelper.cpp
    ../SiteNode.Test.cpp
    ../StateMachineTestInfo.Test.cpp
)

add_precompiled_header(${exe_Federation.Test} ../stdafx.h)

set_target_properties(${exe_Federation.Test} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}) 

target_link_libraries(${exe_Federation.Test}
  ${lib_Federation}
  ${lib_LeaseAgent}
  ${lib_Lease}
  ${lib_Client}
  ${lib_ClientServerTransport}
  ${lib_Transport}
  ${lib_Common}
  ${lib_ServiceModel}
  ${lib_Serialization}
  ${lib_Pal}
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
    FILES ./Federation.Test.exe.cfg
    DESTINATION ${TEST_OUTPUT_DIR}
)

