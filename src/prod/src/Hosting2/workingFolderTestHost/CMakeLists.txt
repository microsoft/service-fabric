include_directories("..")
include_directories(".")

add_compile_options(-rdynamic)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_WorkingFolderTestHost}
  # test code
  Main.cpp
  ComStatelessServiceFactory.cpp
  WorkingFolderTestHost.cpp
  )

add_precompiled_header(${exe_WorkingFolderTestHost} stdafx.h)
set_target_properties(${exe_WorkingFolderTestHost} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}) 

target_link_libraries(${exe_WorkingFolderTestHost}
  ${lib_Hosting2}
  ${lib_Common}
  ${lib_ServiceModel}
  ${lib_Serialization}
  ${lib_ImageModel}
  ${lib_ManagementSubsystem}
  ${lib_ManagementCommon}
  ${lib_ServiceGroup}
  ${lib_ApiWrappers}
  ${lib_FabricNode}
  ${lib_ImageStore}
  ${lib_StoreService}
  ${lib_SystemServices}
  ${lib_Federation}
  ${lib_LeaseAgent}
  ${lib_Replication}
  ${lib_ServiceGroup}
  ${lib_ApiWrappers}
  ${lib_Transport}
  ${lib_Client}
  ${lib_Common}
  ${lib_ServiceModel}
  ${lib_Serialization}
  ${lib_FabricUUID}
  ${lib_InternalFabricUUID}
  ${KtlUser}
  ${BoostTest2}
  ${Cxx}
  ${CxxABI}
  ${lib_Hosting2}
  ${lib_FabricRuntime}
  ${lib_FabricCommon}
  ${lib_FabricResources}
  ssh2
  m
  rt
  jemalloc
  pthread
  dl
  xml2
  uuid
)
