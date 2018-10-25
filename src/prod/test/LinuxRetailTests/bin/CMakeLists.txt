include_directories(
  ".."
  "../../../src/Common"
  "../../../shared/ktlwf/logger/com"
  "../../../src/ktllogger/sys/inc"
)

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_LinuxRetailTests}
  ../FabricClient.Test.cpp
  ../FabricCommon.Test.cpp
  ../FabricRuntime.Test.cpp
  ../FabricInfrastructureService.Test.cpp
  ../KPhysicalLog.Test.cpp
  ../main.cpp
)

add_precompiled_header(${exe_LinuxRetailTests} ../stdafx.h)

set_target_properties(${exe_LinuxRetailTests} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})
    
set_target_properties(${exe_LinuxRetailTests} PROPERTIES 
    LINK_FLAGS "-l FabricClient -l FabricCommon -l FabricRuntime -l FabricInfrastructureService -l KPhysicalLog -L ${LIBRARY_OUTPUT_DIR} -L ${BOOST_LIBRARY_DIRS}")

target_link_libraries(${exe_LinuxRetailTests}
  ${lib_FabricResources}
  ${lib_Common}
  ${lib_Transport}
  ssh2
  ssl
  crypto
  m
  rt
  jemalloc
  pthread
  dl
  xml2
  uuid
  minizip
  z
)

add_dependencies(${exe_LinuxRetailTests}
  ${lib_FabricResources}
  ${lib_Common}
  ${lib_Transport}
  ${lib_FabricClient}
  ${lib_FabricCommon}
  ${lib_FabricRuntime}
  ${lib_FabricInfrastructureService}
  ${lib_KPhysicalLog}
)
