set( LINUX_SOURCES
  ./stdafx.cpp
  ./dllmain.cpp
  ../../../../pal/wc16.c
)

include_directories("../pch")

add_library(objects_fabricimagestore OBJECT ${LINUX_SOURCES})
add_library(${lib_FabricImageStore} SHARED $<TARGET_OBJECTS:objects_fabricimagestore>)

set_target_properties(${lib_FabricImageStore} PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})

set(EXPORTS_FILE ${CMAKE_CURRENT_BINARY_DIR}/${lib_FabricImageStore}.exports)
generate_exports_file(${lib_FabricImageStore} "${CMAKE_CURRENT_SOURCE_DIR}/FabricImageStore.Linux.def" ${EXPORTS_FILE})

set_target_properties(${lib_FabricImageStore} PROPERTIES 
    LINK_FLAGS "-Wl,-Bsymbolic -Wl,--version-script=${EXPORTS_FILE}")

target_link_libraries(${lib_FabricImageStore}
  ${lib_Federation}
  ${lib_FileStoreService}
  ${lib_ClientServerTransport}
  ${lib_Client}
  ${lib_Transport}
  ${lib_ApiWrappers}
  ${lib_ManagementCommon}
  ${lib_ManagementRepairManager}
  ${lib_SystemServices}
  ${lib_ImageStore}
  ${lib_Common}
  ${lib_ImageModel}
  ${lib_ServiceModel}
  ${lib_FabricResources}
  ${lib_Serialization}
  ${lib_Common}
  ${lib_InternalFabricUUID}
  ${lib_FabricUUID}
  ${lib_FabricCommon}
  ${lib_ClientServerTransport}
  ${lib_FileStoreService}
  lttng-ust
  ${Cxx}
  ${CxxABI}
  crypto
  rt
)

install(TARGETS ${lib_FabricImageStore} DESTINATION ${TEST_OUTPUT_DIR})

