set( LINUX_SOURCES
  ../StringResourceRepo.cpp
  ../../pal/wc16.c
)
include_directories("..")
include_directories("../..")

include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_definitions(-DFABRIC_RESOURCES_STATIC_LIB)
add_library(objects_fabricresourcesstatic OBJECT ${LINUX_SOURCES})
add_library(${lib_FabricResourcesStatic} STATIC $<TARGET_OBJECTS:objects_fabricresourcesstatic>)

set_target_properties(${lib_FabricResourcesStatic} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 


