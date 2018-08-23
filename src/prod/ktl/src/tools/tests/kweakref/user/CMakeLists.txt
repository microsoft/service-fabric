set (exe_KWeakRefUserTests "KWeakRefUserTests.exe" CACHE STRING "KWeakRefUserTests.exe")

set ( SOURCES
    ../KWeakRefTests.cpp
    ../../kmuser.cpp
)

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

# console test
add_library(objects_kweakrefusertests_console OBJECT ${SOURCES})
target_compile_definitions(objects_kweakrefusertests_console PUBLIC "CONSOLE_TEST=1")
target_include_directories(objects_kweakrefusertests_console BEFORE PUBLIC "../inc")
target_include_directories(objects_kweakrefusertests_console BEFORE PUBLIC ".")
add_executable(${exe_KWeakRefUserTests} $<TARGET_OBJECTS:objects_kweakrefusertests_console>)

set_target_properties(${exe_KWeakRefUserTests} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KWeakRefUserTests}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${lib_KmUnitUser}
  ${Cxx}
  ${CxxABI}
)

