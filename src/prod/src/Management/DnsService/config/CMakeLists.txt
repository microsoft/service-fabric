set( LINUX_SOURCES
  DnsServiceConfig.cpp
  DnsValidateName.cpp
)

include_directories(".")
include_directories("precomp")

add_library(objects_DnsServiceConfig OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_DnsServiceConfig precomp/stdafx.h)
add_library(${lib_DnsServiceConfig} STATIC $<TARGET_OBJECTS:objects_DnsServiceConfig>)

set_target_properties(${lib_DnsServiceConfig} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

