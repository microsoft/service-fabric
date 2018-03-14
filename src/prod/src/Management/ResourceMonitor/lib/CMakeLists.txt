set( LINUX_SOURCES
  ../ResourceDescription.cpp
  ../ResourceMonitoringAgent.cpp
  ../ResourceMonitorServiceFactory.cpp
  ../ResourceMonitorServiceInstance.cpp
  ../ResourceUsage.cpp
  ../stdafx.cpp
  ../TelemetryClient.cpp
  ../TelemetryData.cpp
  ../TelemetryEvent.cpp
)

add_library(objects_ResourceMonitor OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_ResourceMonitor ../stdafx.h FORCEINCLUDE)
add_library(${lib_ResourceMonitor} STATIC $<TARGET_OBJECTS:objects_ResourceMonitor>)

set_target_properties(${lib_ResourceMonitor} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 
