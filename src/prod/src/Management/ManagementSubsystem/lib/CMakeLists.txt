set( LINUX_SOURCES
  ../ManagementSubsystem.cpp
)

include_directories("..")

add_library(objects_ManagementSubsystem OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_ManagementSubsystem ../stdafx.h)
add_library(${lib_ManagementSubsystem} STATIC $<TARGET_OBJECTS:objects_ManagementSubsystem>)

set_target_properties(${lib_ManagementSubsystem} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

target_link_libraries(${lib_ManagementSubsystem}
  ${lib_CentralSecretService}
  ${lib_LocalSecretService}
)
