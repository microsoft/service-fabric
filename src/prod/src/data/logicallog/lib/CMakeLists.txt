add_definitions(-DUPASSTHROUGH=1)

set( LINUX_SOURCES
  ../KtlLoggerMode.cpp
  ../FakeLogManager.cpp
  ../FakePhysicalLog.cpp
  ../FileLogicalLog.cpp
  ../FileLogicalLogReadStream.cpp
  ../LogicalLogBuffer.cpp
  ../LogicalLogKIoBufferStream.cpp
  ../LogicalLogReadTask.cpp
  ../LogicalLogStream.cpp
  ../PhysicalLog.cpp
  ../PhysicalLogHandle.cpp
  ../LogicalLog.cpp
  ../LogManager.cpp
  ../LogManagerHandle.cpp
  ../LogCreationFlags.cpp
)

include_directories("..")
include_directories("../../../ktllogger/sys/inc")
include_directories("../../../ktllogger/sys/ktlshim")


add_library(objects_LogicalLog OBJECT ${LINUX_SOURCES})

add_precompiled_header(objects_LogicalLog ../stdafx.h)

add_library(${lib_LogicalLog} STATIC $<TARGET_OBJECTS:objects_LogicalLog>)

target_link_libraries(${lib_LogicalLog}
  ${lib_KtlLoggerNode}
  ${lib_Common}
  ${lib_data_utilities}
  ${KtlLoggerUser}
  ${lib_KtlLoggerShimUnify}
  ${KtlLoggerUser}
  ${Ktlfull}
    )

set_target_properties(${lib_LogicalLog} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}) 

