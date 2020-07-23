include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_Lease.Test}
  ../../../test/BoostUnitTest/btest.cpp

  # test code
  ../LeaseLayerTestCommon.cpp
  ../LeaseLayerApi.Test.cpp
  #../LeaseLayerIoctl.Test.cpp
  )

include_directories(".")
include_directories("../inc/private")
include_directories("../inc/public")

set_target_properties(${exe_Lease.Test} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}) 

target_link_libraries(${exe_Lease.Test}
  ${lib_Common}
  ${lib_Serialization}
  ${lib_Lease}
  ${lib_Federation}
  ${lib_Transport}
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
  jemalloc
  pthread
  dl
  xml2
  uuid
  ${BoostTest2}
)

