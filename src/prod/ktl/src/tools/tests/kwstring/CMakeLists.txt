set (exe_KWStringTests "KWStringTests.exe" CACHE STRING "KWStringTests.exe")

set( SOURCES
  ../KWString.cpp
  ../kmuser.cpp
)

include_directories(".")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

# console test
add_library(objects_kwstringtests_console OBJECT ${SOURCES})
target_compile_definitions(objects_kwstringtests_console PUBLIC CONSOLE_TEST=1)
add_executable(${exe_KWStringTests} $<TARGET_OBJECTS:objects_kwstringtests_console>)

set_target_properties(${exe_KWStringTests} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KWStringTests}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${lib_KmUnitUser}
  ${Cxx}
  ${CxxABI}
  pthread
  c
)

