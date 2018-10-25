set( SOURCES
  ./TraceTemplates.Linux.c
  ./TraceWrapper.Linux.c
)

include_directories("../../inc")
include_directories(".")

add_library(objects_ktllttng OBJECT ${SOURCES})
add_library(${lib_KtlLttng} STATIC $<TARGET_OBJECTS:objects_ktllttng>)

set_target_properties(${lib_KtlLttng} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR})

target_link_libraries(${lib_KtlLttng} lttng-ust dl)


#install(DIRECTORY ${PROJECT_SOURCE_DIR}/External/Ktl.Linux.Libs/lttng/
#    DESTINATION ${TEST_OUTPUT_DIR}
#    FILES_MATCHING
#    PATTERN "*"
#    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ OWNER_EXECUTE WORLD_READ OWNER_EXECUTE)

