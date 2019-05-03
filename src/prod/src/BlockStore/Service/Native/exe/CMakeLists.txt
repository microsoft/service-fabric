include_directories("..")
include_directories("../inc")

add_compile_options(-rdynamic)

#TODO: Figure out non test way to pull boost library.
add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)
add_definitions(-DETL_TRACING_ENABLED)
#add_definitions(-DRELIABLE_COLLECTION_TEST_MOCK_CLUSTER)

add_executable(${exe_BlockStore}
  ../inc/stdafx.cpp
  ../src/BlockStorePayload.cpp
  ../src/CBlockStoreRequest.cpp
  ../src/CDriverClient.cpp
  ../src/CNetworkStream.cpp
  ../src/ServiceMain.cpp
  ../src/CBlockStoreService.cpp
  #../dependencies/shared/src/mock.cpp
  )

add_dependencies(${exe_BlockStore} ${lib_ReliableCollectionService} ${lib_ReliableCollectionRuntime})
add_precompiled_header(${exe_BlockStore} ../inc/stdafx.h)

set_target_properties(${exe_BlockStore} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}) 

target_link_libraries(${exe_BlockStore}
  ${lib_EntreeService}
  ${lib_StoreService}
  ${lib_Store}
  ${lib_StoreRepairPolicy}
  ${lib_Replication}
  ${lib_Common}
  ${lib_ServiceModel}
  ${lib_Serialization}
  ${lib_FabricCommon}
  ${lib_FabricRuntime}
  ${lib_FabricTypeHost}
  ${lib_InternalFabricUUID}
  ${lib_FabricUUID}
  ${lib_FabricResources}
  ${lib_BlockStore}
  ${KtlUser}
  ${Cxx}
  ${CxxABI}
  z
  m
  rt
  pthread
  dl
  xml2
  uuid
  boost_system
  boost_thread
  curl
)
