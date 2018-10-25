add_definitions(-DUNIFY)
add_definitions(-DKTL_BUILD)


# pull in InternalKtlLogger.h
include_directories("../../../../../ktl/src/logger/src")
include_directories("../../inc")

set( SOURCES
  ../BaseObjects.cpp
  ../coalescerecords.cpp
  ../ManagerShimKernel.cpp
  ../ContainerShimKernel.cpp
  ../StreamShimKernel.cpp
  ../LLRecord.cpp
  ../MBInfoAccess.cpp
  ../globalobj.cpp
  ../fileio.cpp
  ../FileObjectTable.cpp
  ../OverlayManager.cpp
  ../OverlayContainer.cpp
  ../OverlayRvdLog.cpp
  ../overlaylog.cpp
  ../OverlayStream.cpp
  ../OverlayStreamFS.cpp
  ../servicewrapper.cpp
  ../alias.cpp
  ../ValidateLinuxLog.cpp
)

add_library(objects_KtlLoggerShimUnify OBJECT ${SOURCES})
add_library(${lib_KtlLoggerShimUnify} STATIC $<TARGET_OBJECTS:objects_KtlLoggerShimUnify>)

set_target_properties(${lib_KtlLoggerShimUnify} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})

