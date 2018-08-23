set( LINUX_SOURCES
  ./dllmain.cpp
  ../../../../pal/wc16.c
)

include_directories(".")

add_library(objects_FabricClient OBJECT ${LINUX_SOURCES})

#add_precompiled_header(objects_FabricClient ./stdafx.h)

add_library(${lib_FabricClient} SHARED $<TARGET_OBJECTS:objects_FabricClient>)

set_target_properties(${lib_FabricClient} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

set(EXPORTS_FILE ${CMAKE_CURRENT_BINARY_DIR}/${lib_FabricClient}.exports)
generate_exports_file(${lib_FabricClient} "${CMAKE_CURRENT_SOURCE_DIR}/FabricClient.Linux.def" ${EXPORTS_FILE})

set_target_properties(${lib_FabricClient} PROPERTIES 
    LINK_FLAGS "-Wl,-Bsymbolic -Wl,--version-script=${EXPORTS_FILE}")

target_link_libraries(
  ${lib_FabricClient}
  ${lib_Client}
  ${lib_ApiWrappers}
  ${lib_ManagementCommon}
  ${lib_ManagementRepairManager}
  ${lib_SystemServices}
  ${lib_ImageStore}
  ${lib_ClientServerTransport}
  ${lib_FileStoreService}
  ${lib_Transport}
  ${lib_Common}
  ${lib_ServiceModel}
  ${lib_Serialization}
  ${lib_InternalFabricUUID}
  ${lib_FabricUUID}
  ${lib_FabricCommon}
  ${lib_FabricResources}
  ${KtlUser}
  ${Cxx}
  ${CxxABI}
  crypto
  rt
  pthread
)

install(TARGETS ${lib_FabricClient} DESTINATION ${TEST_OUTPUT_DIR})

