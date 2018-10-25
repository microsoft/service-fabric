include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_Client.Test}
  # boost.test main
  ../../../test/BoostUnitTest/btest.cpp

  # test code
  ../ComFabricClient.Test.cpp
  ../FabricClientImpl.test.cpp
  ../MockFabricClientImpl.cpp
  ../TestClientFactory.cpp
  )

add_precompiled_header(${exe_Client.Test} ../stdafx.h)

set_target_properties(${exe_Client.Test} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}) 

target_link_libraries(${exe_Client.Test}
  ${lib_Client}
  ${lib_Serialization}
  ${lib_ApiWrappers}
  ${lib_ClientServerTransport}
  ${lib_Transport}
  ${lib_ManagementCommon}
  ${lib_ManagementRepairManager}
  ${lib_ServiceModel}
  ${lib_ImageStore}
  ${lib_FileStoreService}
  ${lib_SystemServices}
  ${lib_Common}
  ${lib_FabricUUID}
  ${lib_FabricCommon}
  ${lib_FabricResources}
  ${lib_InternalFabricUUID}
  ${KtlUser}
  ${BoostTest2}
  ${Cxx}
  ${CxxABI}
  ssh2
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
    FILES ./Client.Test.exe.cfg
    DESTINATION ${TEST_OUTPUT_DIR}
	RENAME ${exe_Client.Test}.cfg
)


