add_definitions(-DUDRIVER)
add_definitions(-DKTL_BUILD)


set( SOURCES
  ../BaseObjects.cpp

  ../KtlLogMarshal.cpp
  ../ManagerShimUser.cpp
  ../ContainerShimUser.cpp
  ../StreamShimUser.cpp
  ../KtlLogMarshalUser.cpp

  ../coalescerecords.cpp
  ../ManagerShimKernel.cpp
  ../ContainerShimKernel.cpp
  ../StreamShimKernel.cpp
  ../KtlLogMarshalKernel.cpp
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
  ./InProcDevIoCtl.cpp
)

add_library(objects_KtlLoggerShimUDriver OBJECT ${SOURCES})
add_library(${lib_KtlLoggerShimUDriver} STATIC $<TARGET_OBJECTS:objects_KtlLoggerShimUDriver>)

set_target_properties(${lib_KtlLoggerShimUDriver} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})

