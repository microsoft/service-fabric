include_directories("..")

set( LINUX_SOURCES
../LoadMetricStats.cpp
../LoadMetric.cpp
../LoadOrMoveCostDescription.cpp
../PLBConfig.cpp
../ReplicaRole.cpp
../ReplicaDescription.cpp
../Constants.cpp
)

add_library(objects_LoadBalancingCommon OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_LoadBalancingCommon stdafx.h FORCEINCLUDE)

add_library(${lib_LoadBalancingCommon} STATIC $<TARGET_OBJECTS:objects_LoadBalancingCommon>)

set_target_properties(${lib_LoadBalancingCommon} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})

          
