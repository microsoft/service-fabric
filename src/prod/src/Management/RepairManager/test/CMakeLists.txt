include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_ManagementRepairManager.Test}
  # boost.test main
  ../../../../test/BoostUnitTest/btest.cpp
  # test code
  ../UpdateRepairTaskHealthPolicyMessageBody.Test.cpp
  )

add_precompiled_header(${exe_ManagementRepairManager.Test} ../stdafx.h)

set_target_properties(${exe_ManagementRepairManager.Test} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}) 

target_link_libraries(${exe_ManagementRepairManager.Test}
  ${lib_ManagementRepairManager}
  ${lib_ImageStore}
  ${lib_SystemServices}
  ${lib_Federation}
  ${lib_LeaseAgent}
  ${lib_Lease}
  ${lib_Transport}
  ${lib_Client}
  ${lib_ClientServerTransport}
  ${lib_Common}
  ${lib_ServiceModel}
  ${lib_FabricCommon}
  ${BoostTest2}
  ${Cxx}
  ${CxxABI}
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

