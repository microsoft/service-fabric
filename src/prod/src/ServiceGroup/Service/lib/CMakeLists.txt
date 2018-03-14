set( LINUX_SOURCES
  ../ServiceGroup.AsyncOperations.cpp
  ../ServiceGroup.Public.cpp
  ../ServiceGroup.ServiceBase.cpp
  ../ServiceGroup.ServiceBaseUnit.cpp
  ../ServiceGroup.StatefulService.cpp
  ../ServiceGroup.StatefulServiceUnit.cpp
  ../ServiceGroup.StatelessService.cpp
  ../ServiceGroup.StatelessServiceUnit.cpp
  ../ServiceGroup.AtomicGroup.cpp
  ../ServiceGroup.OperationStream.cpp
  ../ServiceGroup.OperationDataStream.cpp
  ../ServiceGroup.OperationExtendedBuffer.cpp
  ../ServiceGroup.Runtime.cpp
  ../ServiceGroup.ReplicatorSettings.cpp
  ../ServiceGroup.ServiceGroupCommonEventSource.cpp
  ../ServiceGroup.ServiceGroupReplicationEventSource.cpp
  ../ServiceGroup.ServiceGroupStatefulEventSource.cpp
  ../ServiceGroup.ServiceGroupStatefulMemberEventSource.cpp
  ../ServiceGroup.ServiceGroupStatelessEventSource.cpp
  ../ServiceGroup.ServiceGroupStatelessMemberEventSource.cpp
  ../ServiceGroup.PerformanceCounters.cpp
)

include_directories("..")

add_library(objects_ServiceGroup OBJECT ${LINUX_SOURCES})
add_precompiled_header(objects_ServiceGroup ../stdafx.h)
add_library(${lib_ServiceGroup} STATIC $<TARGET_OBJECTS:objects_ServiceGroup>)

set_target_properties(${lib_ServiceGroup} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

