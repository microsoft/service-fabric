include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_ImageModel.Test}
  # boost.test main
  ../../../../test/BoostUnitTest/btest.cpp

  # test code
  ../FabricDeploymentSpecification.Test.cpp
  ../ImageModel.Test.cpp
)

add_precompiled_header(${exe_ImageModel.Test} ../stdafx.h)

set_target_properties(${exe_ImageModel.Test} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}) 

target_link_libraries(${exe_ImageModel.Test}
  ${lib_ImageModel}
  ${lib_Common}
  ${lib_ServiceModel}
  ${lib_Serialization}
  ${BoostTest2}
  ${Cxx}
  ${CxxABI}
  ${lib_FabricCommon}
  ${lib_FabricResources}
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
    FILES ./ImageModel.Test.exe.cfg
    DESTINATION ${TEST_OUTPUT_DIR}
)

