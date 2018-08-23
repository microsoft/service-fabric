include_directories("..")

add_compile_options(-rdynamic)

add_executable(${exe_Hosting2.TestProcess}
  # test code
  ../HostingTestProcess.Test.cpp
  )

set_target_properties(${exe_Hosting2.TestProcess} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}) 

target_link_libraries(${exe_Hosting2.TestProcess}
  ${lib_FabricCommon}
  ${lib_Common}
  ${lib_ServiceModel}
  ${lib_Serialization}
  ${lib_FabricResources}
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

