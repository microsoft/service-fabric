add_definitions(-DUPASSTHROUGH)
add_definitions(-DKTL_BUILD)


set( SOURCES
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
)

add_library(objects_KtlLoggerShimUPassthrough OBJECT ${SOURCES})
add_library(${lib_KtlLoggerShimUPassthrough} STATIC $<TARGET_OBJECTS:objects_KtlLoggerShimUPassthrough>)

set_target_properties(${lib_KtlLoggerShimUPassthrough} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})

