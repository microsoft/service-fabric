set (exe_KtlAwaitableTests "KtlAwaitableTests.exe" CACHE STRING "KtlAwaitableTests.exe")

set ( SOURCES
  ../ktlAwaitabletestcases.cpp
  ../ktlAwaitabletests.cpp
)

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

# console test
add_library(objects_ktlawaitabletests_console OBJECT ${SOURCES})
target_compile_definitions(objects_ktlawaitabletests_console PUBLIC CONSOLE_TEST=1)
target_include_directories(objects_ktlawaitabletests_console BEFORE PUBLIC "../inc")
add_executable(${exe_KtlAwaitableTests} $<TARGET_OBJECTS:objects_ktlawaitabletests_console>)

set_target_properties(${exe_KtlAwaitableTests} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KtlAwaitableTests}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${lib_KmUnitUser}
  ${Cxx}
  ${CxxABI}
)

install(
    FILES ../KtlAwaitableTestInput.xml
    DESTINATION ${TEST_OUTPUT_DIR}
)

