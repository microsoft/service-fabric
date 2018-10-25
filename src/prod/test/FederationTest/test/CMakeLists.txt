include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_FederationTest}
    ./testmain.cpp
    ../RandomFederationTestSession.cpp
    ../FederationTestConfig.cpp
    ../RandomFederationTestConfig.cpp
    ../FederationTestDispatcher.cpp
    ../FederationTestSession.cpp
    ../TestFederation.cpp
    ../TestNode.cpp
  )

set_target_properties(${exe_FederationTest} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}) 

target_link_libraries(${exe_FederationTest}
  ${lib_Federation}
  ${lib_LeaseAgent}
  ${lib_Lease}
  ${lib_Client}
  ${lib_ClientServerTransport}
  ${lib_Transport}
  ${lib_TestCommon}
  ${lib_FederationTestCommon}
  ${lib_Common}
  ${lib_ServiceModel}
  ${lib_ServiceModel_Federation}
  ${lib_ServiceModel_Naming}
  ${lib_ServiceModel_Reliability_Failover}
  ${lib_ServiceModel_Management_ClusterManager}
  ${lib_ServiceModel_Transport}
  ${lib_ServiceModel}
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
    FILES ${exe_FederationTest}.Linux.cfg 
    RENAME ${exe_FederationTest}.cfg
    DESTINATION ${TEST_OUTPUT_DIR}
)

install(
    CODE "file( GLOB FederationTestFiles \"${CMAKE_CURRENT_SOURCE_DIR}/*.test\" \"${CMAKE_CURRENT_SOURCE_DIR}/*.Test\" )"
    CODE "file( INSTALL \${FederationTestFiles} DESTINATION ${TEST_OUTPUT_DIR} )"
)
