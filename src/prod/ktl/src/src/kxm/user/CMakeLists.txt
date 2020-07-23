set( SOURCES
  ./KXMApi.cpp
  ./KXMApiUser.cpp
  ./KXMApiKernel.cpp
  ../../ktlpal.cpp
)

add_library(objects_kxmuser OBJECT ${SOURCES})
add_library(${lib_KxmUser} STATIC $<TARGET_OBJECTS:objects_kxmuser>)

install(TARGETS ${lib_KxmUser}
    DESTINATION ${TEST_OUTPUT_DIR}
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ OWNER_EXECUTE WORLD_READ OWNER_EXECUTE)

set_target_properties( ${lib_KxmUser} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR} )
