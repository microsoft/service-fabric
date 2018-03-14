set( LINUX_SOURCES
    ../FabricStream.cpp
    ../FabricSerializable.cpp
    ../FabricSerializableStream.cpp
    ../FabricSerializationTypes.cpp
    ../fabricserializationcommon.cpp
)

set( SOURCES
    ../sample.cpp
    ../FabricStream.cpp
    ../FabricSerializable.cpp
    ../FabricSerializableStream.cpp
    ../FabricSerializationTypes.cpp
    ../fabricserializationcommon.cpp
)

include_directories("../../inc")
include_directories("../../../../src/inc")
include_directories("../../../../src/inc/palrt")
include_directories("../../../../src/inc/palrt/missing")
include_directories("../../../../src/inc/clr")
include_directories("../../../../src/inc/clr/pal")

add_library(objects_serialization OBJECT ${LINUX_SOURCES})
target_compile_definitions(objects_serialization PUBLIC KTL_CORE_LIB)
add_library(${lib_Serialization} STATIC $<TARGET_OBJECTS:objects_serialization>)

target_link_libraries(${lib_Serialization} ${Ktlcore})
set_target_properties(${lib_Serialization} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})
