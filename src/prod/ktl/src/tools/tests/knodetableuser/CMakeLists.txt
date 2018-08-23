set (exe_KNodeTableUserTests "KNodeTableUserTests.exe" CACHE STRING "KNodeTableUserTests.exe")

set ( SOURCES
    ../KNodeTable.cpp
    ../kmuser.cpp
)

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

# console test
add_library(objects_knodetableusertests_console OBJECT ${SOURCES})
target_compile_definitions(objects_knodetableusertests_console PUBLIC "CONSOLE_TEST=1")
target_include_directories(objects_knodetableusertests_console BEFORE PUBLIC "../knodetable")
add_executable(${exe_KNodeTableUserTests} $<TARGET_OBJECTS:objects_knodetableusertests_console>)

set_target_properties(${exe_KNodeTableUserTests} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KNodeTableUserTests}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${lib_KmUnitUser}
  ${Cxx}
  ${CxxABI}
)

