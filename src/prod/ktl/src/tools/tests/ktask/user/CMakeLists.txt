set (exe_KTaskUserTests "KTaskUserTests.exe" CACHE STRING "KTaskUserTests.exe")

set ( SOURCES
    ../KTaskTests.cpp
    ../../kmuser.cpp
)

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

# console test
add_library(objects_ktaskusertests_console OBJECT ${SOURCES})
target_compile_definitions(objects_ktaskusertests_console PUBLIC "CONSOLE_TEST=1")
target_include_directories(objects_ktaskusertests_console BEFORE PUBLIC "../inc")
target_include_directories(objects_ktaskusertests_console BEFORE PUBLIC ".")
add_executable(${exe_KTaskUserTests} $<TARGET_OBJECTS:objects_ktaskusertests_console>)

set_target_properties(${exe_KTaskUserTests} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KTaskUserTests}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${lib_KmUnitUser}
  ${Cxx}
  ${CxxABI}
)

