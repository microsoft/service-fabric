include_directories("./")

set( LINUX_SOURCES
  ../kmuser.cpp
  ../CommandLineParser.cpp
#  ./VHDUtil.cpp
)

add_definitions(-DKM_LIBRARY)

add_library(objects_KmUnitUser OBJECT ${LINUX_SOURCES})

add_library(${lib_KmUnitUser} STATIC $<TARGET_OBJECTS:objects_KmUnitUser>)

set_target_properties(${lib_KmUnitUser} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})

