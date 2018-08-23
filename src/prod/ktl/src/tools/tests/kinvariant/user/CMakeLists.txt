set (exe_KInvariantUserTests "KInvariantUserTests.exe" CACHE STRING "KInvariantUserTests.exe")

set ( SOURCES
    ../KInvariantTests.cpp
    ../../kmuser.cpp
)

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

# console test
add_library(objects_kinvariantusertests_console OBJECT ${SOURCES})
target_compile_definitions(objects_kinvariantusertests_console PUBLIC "CONSOLE_TEST=1")
target_include_directories(objects_kinvariantusertests_console BEFORE PUBLIC "../inc")
target_include_directories(objects_kinvariantusertests_console BEFORE PUBLIC ".")
add_executable(${exe_KInvariantUserTests} $<TARGET_OBJECTS:objects_kinvariantusertests_console>)

set_target_properties(${exe_KInvariantUserTests} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KInvariantUserTests}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${lib_KmUnitUser}
  ${Cxx}
  ${CxxABI}
)

