include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_Query.Test}
  # boost.test main
  ../../../test/BoostUnitTest/btest.cpp

  # test code
  ../QueryMessageHandler.Test.cpp
  ../QueryAddress.Test.cpp
  )

add_precompiled_header(${exe_Query.Test} ../stdafx.h)

set_target_properties(${exe_Query.Test} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}) 

target_link_libraries(${exe_Query.Test}
  ${lib_Query}
  ${lib_ServiceModel}
  ${lib_Serialization}
  ${lib_Transport}
  ${lib_Common}
  ${lib_ApiWrappers}
  ${lib_ClientServerTransport}
  ${lib_Failover}
  ${lib_NetworkInventoryManager}
  ${lib_ManagementRepairManager}
  ${lib_ServiceModel}
  ${lib_ImageModel}
  ${KtlUser}
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
    FILES ./Query.Test.exe.cfg
    DESTINATION ${TEST_OUTPUT_DIR}
)

install(
    FILES ./RunQueryTests.sh
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    DESTINATION ${TEST_OUTPUT_DIR}
)

