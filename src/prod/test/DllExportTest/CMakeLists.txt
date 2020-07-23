include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_DllExportTest}
  # boost.test main
  ../../test/BoostUnitTest/btest.cpp

  # test code
  ./PublicApi.Test.cpp
  )

add_precompiled_header(${exe_DllExportTest} ./stdafx.h)

set_target_properties(${exe_DllExportTest} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}) 

target_link_libraries(${exe_DllExportTest}
  ${lib_Common}
  ${lib_Serialization}
  ${lib_FabricCommon}
  ${lib_ServiceModel}
  ${BoostTest2}
  ${lib_Pal}
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

install(
    FILES ./DllExportTest.exe.cfg
    DESTINATION ${TEST_OUTPUT_DIR}
)
