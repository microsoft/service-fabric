IF(CMAKE_BUILD_TYPE MATCHES Debug)
  message("Debug build, linking against WinFab.Ktl.Linux")
  include_directories(BEFORE "${EXTERNAL_DEPS_DIR}/WinFab.Ktl.Linux.debug/inc")
ELSE()
  message("Optimized build, linking against WinFab.Ktl.Linux.retail")
  include_directories(BEFORE "${EXTERNAL_DEPS_DIR}/WinFab.Ktl.Linux.retail/inc")
ENDIF()
include_directories(BEFORE "${PROJECT_SOURCE_DIR}/src/Management/DnsService/linux_include")
include_directories("${PROJECT_SOURCE_DIR}/src/Management/DnsService/include")
include_directories("${PROJECT_SOURCE_DIR}/src/Management/DnsService/config")
include_directories("..")
include_directories("../fabric")
include_directories("../precomp")

add_compile_options(-rdynamic)

add_executable(${exe_FabricDnsService}
  ../FabricData.cpp
  ../FabricDnsService.cpp
  ../FabricDnsServiceFactory.cpp
  ../FabricTracer.cpp
  ../main.cpp
  ../fabric/FabricAsyncOperationCallback.cpp
  ../fabric/FabricAsyncOperationContext.cpp
  ../fabric/FabricStringResult.cpp
  ../precomp/stdafx.cpp
)

add_precompiled_header(${exe_FabricDnsService} ../precomp/stdafx.h)

set_target_properties(${exe_FabricDnsService} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}) 

add_custom_target(CopyFabricDnsService ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${TEST_OUTPUT_DIR}/__FabricSystem_App4294967295/DnsService.Code.Current
    COMMAND ${CMAKE_COMMAND} -E copy ${TEST_OUTPUT_DIR}/${exe_FabricDnsService} ${TEST_OUTPUT_DIR}/__FabricSystem_App4294967295/DnsService.Code.Current/FabricDnsService.exe
    COMMAND strip ${TEST_OUTPUT_DIR}/__FabricSystem_App4294967295/DnsService.Code.Current/FabricDnsService.exe
    DEPENDS ${exe_FabricDnsService}
)

find_library(KtlfullDbg NAMES ktlfull PATHS "${EXTERNAL_DEPS_DIR}/WinFab.Ktl.Linux.debug" NO_DEFAULT_PATH)
find_library(KtlfullOpt NAMES ktlfull PATHS "${EXTERNAL_DEPS_DIR}/WinFab.Ktl.Linux.retail" NO_DEFAULT_PATH)
    
target_link_libraries(${exe_FabricDnsService}
  ${lib_DnsService}
  ${lib_DnsServiceConfig}
  ${lib_Common}
  ${lib_ServiceModel}
  ${lib_Serialization}
  ${lib_FabricCommon}
  ${lib_FabricClient}
  ${lib_FabricResources}
  ${lib_FabricRuntime}
  debug ${KtlfullDbg} optimized ${KtlfullOpt}
  ${Cxx}
  ${CxxABI}
  ssh2
  ssl
  crypto
  m
  rt
  pthread
  c
  dl
  xml2
  uuid
  unwind
  unwind-x86_64
  icuuc
)
