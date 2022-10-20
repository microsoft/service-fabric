include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_LoadBalancing.Test}
  # test code
  ../PLBAutoscaling.Test.cpp
  ../PLBAuxiliaryStructures.Test.cpp
  ../PLBConstraintCheck.Test.cpp
  ../PLBScheduler.Test.cpp
  ../PLBBalancing.Test.cpp
  ../PLBPlacement.Test.cpp
  ../PLBUpgrade.Test.cpp
  ../PLBAppGroups.Test.cpp
  ../PLBAssert.Test.cpp
  ../PLBBalancingWithConstraintViolation.Test.cpp
  ../PLBQuery.Test.cpp
  ../RandomDistribution.Test.cpp
  ../ServiceDomain.Test.cpp
  ../TestFM.cpp
  ../TestUtility.cpp
  ../TimedCounter.Test.cpp
  ../PLBResourceGovernance.Test.cpp
  ../FailoverUnit.Test.cpp
  ../btest.cpp
  ../PLBThrottling.Test.cpp
)

add_precompiled_header(${exe_LoadBalancing.Test} ../stdafx.h)

set_target_properties(${exe_LoadBalancing.Test} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_LoadBalancing.Test}
  ${lib_LoadBalancing}
  ${lib_LoadBalancingCommon}
  ${lib_FailoverCommon}
  ${lib_Federation}
  ${lib_Client}
  ${lib_ClientServerTransport}
  ${lib_Transport}
  ${lib_Common}
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
    FILES ./LoadBalancing.Test.exe.cfg
    DESTINATION ${TEST_OUTPUT_DIR}
)

