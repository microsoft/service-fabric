include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_ServiceGroup.Test}
  # boost.test main
  ../../../../test/BoostUnitTest/btest.cpp

  # test code
  ../ServiceGroup.ReplicatorSettings.Test.cpp
  ../ServiceGroup.ServiceGroupFactory.Test.cpp
  )

add_precompiled_header(${exe_ServiceGroup.Test} ../stdafx.h)

set_target_properties(${exe_ServiceGroup.Test} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}) 

target_link_libraries(${exe_ServiceGroup.Test}
  ${lib_ServiceGroup}
  ${lib_Hosting2}
  ${lib_Testability}
  ${lib_Replication}
  ${lib_TransactionalReplicator}
  ${lib_Federation}
  ${lib_LeaseAgent}
  ${lib_Lease}
  ${lib_Transport}
  ${lib_Failover}
  ${lib_NetworkInventoryManager}
  ${lib_Client}
  ${lib_ManagementRepairManager}
  ${lib_Query}
  ${lib_Common}
  ${lib_ServiceModel}
  ${lib_Serialization}
  ${lib_FabricRuntime}
  ${lib_FabricCommon}
  ${lib_FabricResources}
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
