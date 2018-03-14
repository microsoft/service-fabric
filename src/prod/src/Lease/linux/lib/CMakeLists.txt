set( LINUX_SOURCES
  ../Api.cpp
  ../generictable.cpp
  ../isvalid.cpp
  ../LeaseLayerIo.cpp
  ../LeaseTransport.cpp
  ../LeasLayr.cpp
  ../Leases.cpp
  ../LeaseAgent.cpp
  ../leaseapp.cpp
  ../stdafx.cpp
)

set(headers
)

include_directories("..")
include_directories("../../inc/public")

add_library(objects_Lease OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_Lease ../stdafx.h)
add_library(${lib_Lease} STATIC $<TARGET_OBJECTS:objects_Lease>)

set_target_properties(${lib_Lease} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

