include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_StoreTest}
  # boost.test main
  ../../../test/BoostUnitTest/btest.cpp
  Common.Test.cpp
  ReplicatedStore.test.cpp
  HealthTracker.test.cpp
  )

set_target_properties(${exe_StoreTest} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}) 

target_link_libraries(${exe_StoreTest}
  ${lib_Store}
  ${lib_StoreRepairPolicy}
  ${lib_ApiWrappers}
  ${lib_Common}
  ${lib_ServiceModel}
  ${lib_Serialization}
  ${BoostTest2}
  ${Cxx}
  ${CxxABI}
  ${lib_FabricCommon}
  ${lib_FabricResources}
  ${lib_TestHooks}
  ssh2
  minizip
  z
  ssl
  crypto
  m
  rt
  jemalloc
  pthread
  dl
  xml2
  uuid
)

install(
    FILES ./Store.Test.exe.cfg
    DESTINATION ${TEST_OUTPUT_DIR}
    RENAME ${exe_StoreTest}.cfg
)

