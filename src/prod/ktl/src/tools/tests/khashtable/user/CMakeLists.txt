set (exe_KHashTableUserTests "KHashTableUserTests.exe" CACHE STRING "KHashTableUserTests.exe")

set ( SOURCES
    ../KHashTableTests.cpp
    ../../kmuser.cpp
)

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

# console test
add_library(objects_khashtableusertests_console OBJECT ${SOURCES})
target_compile_definitions(objects_khashtableusertests_console PUBLIC "CONSOLE_TEST=1")
target_include_directories(objects_khashtableusertests_console BEFORE PUBLIC "../inc")
target_include_directories(objects_khashtableusertests_console BEFORE PUBLIC ".")
add_executable(${exe_KHashTableUserTests} $<TARGET_OBJECTS:objects_khashtableusertests_console>)

set_target_properties(${exe_KHashTableUserTests} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KHashTableUserTests}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${lib_KmUnitUser}
  ${Cxx}
  ${CxxABI}
)

