include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_Transport.Functional.Test}
  ../../../test/BoostUnitTest/btest.cpp
  ../TestCommon.cpp

  # test code
  ../Security.functional.test.cpp
  ../SecureSessionExpiration.test.cpp
  )

add_precompiled_header(${exe_Transport.Functional.Test} ../stdafx.h)

set_target_properties(${exe_Transport.Functional.Test} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}) 

target_link_libraries(${exe_Transport.Functional.Test}
  ${lib_Serialization}
  ${lib_Transport}
  ${lib_Common}
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

install(
    FILES ./Transport.Functional.Test.exe.cfg
    DESTINATION ${TEST_OUTPUT_DIR}
    RENAME ${exe_Transport.Functional.Test}.cfg
)
