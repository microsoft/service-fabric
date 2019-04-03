include_directories("..")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_ImageStore.Test}
  # boost.test main
  ../../../../test/BoostUnitTest/btest.cpp

  # test code
  ../FileImageStore.Test.cpp
  ../ImageStoreUtility.Test.cpp
)

add_precompiled_header(${exe_ImageStore.Test} ../stdafx.h)

set_target_properties(${exe_ImageStore.Test} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}) 

target_link_libraries(${exe_ImageStore.Test}
  ${lib_ImageStore}
  ${lib_FileStoreService}
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
    FILES
    ./ImageStore.Test.exe.cfg
    ./ImageStore.Test.ApplicationManifest.xml
    ./ImageStore.Test.ServiceManifest.xml    

    DESTINATION ${TEST_OUTPUT_DIR}
)

