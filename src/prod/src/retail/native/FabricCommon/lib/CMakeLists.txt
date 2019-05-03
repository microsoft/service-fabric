set( LINUX_SOURCES
  ../FabricCommon.cpp
  ../ConfigLoader.cpp
  ../stdafx.cpp
  ../../../../pal/wc16.c
)

set( LINUX_SOURCES2
  ./TraceTemplates.Linux.c
  ../TraceWrapper.Linux.c
  ../TraceBinaryWrapper.Linux.c
)

include_directories("../pch")

add_library(objects_fabriccommonstatic OBJECT ${LINUX_SOURCES})
add_library(objects_fabriccommonstatic2 OBJECT ${LINUX_SOURCES2})
add_library(${lib_FabricCommonStatic} STATIC $<TARGET_OBJECTS:objects_fabriccommonstatic> $<TARGET_OBJECTS:objects_fabriccommonstatic2>)

set_target_properties(${lib_FabricCommonStatic} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})
