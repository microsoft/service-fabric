set( LINUX_SOURCES
  ../StringResourceRepo.cpp
  ../../pal/wc16.c
)

include_directories("../..")

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_custom_target(
    stringtbl.inc 
    COMMAND perl ${CMAKE_CURRENT_SOURCE_DIR}/resgenl.pl 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../inc/resourceids.h
    ${CMAKE_CURRENT_BINARY_DIR}/stringtbl.inc
    ${CMAKE_CURRENT_BINARY_DIR}/stringtbl.inc.tmp
)

add_library(objects_fabricresources OBJECT ${LINUX_SOURCES})
add_library(${lib_FabricResources} SHARED $<TARGET_OBJECTS:objects_fabricresources>)

set_target_properties(${lib_FabricResources} PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

set(EXPORTS_FILE ${CMAKE_CURRENT_BINARY_DIR}/${lib_FabricResources}.exports)
generate_exports_file(${lib_FabricResources} "${CMAKE_CURRENT_SOURCE_DIR}/FabricResources.Linux.def" ${EXPORTS_FILE})

set_target_properties(${lib_FabricResources} PROPERTIES 
    LINK_FLAGS "-Wl,-Bsymbolic -Wl,--version-script=${EXPORTS_FILE}")

target_link_libraries(${lib_FabricResources}
  ${Cxx}
  ${CxxABI}
)

add_dependencies(objects_fabricresources stringtbl.inc)

install(TARGETS ${lib_FabricResources} DESTINATION ${TEST_OUTPUT_DIR})

