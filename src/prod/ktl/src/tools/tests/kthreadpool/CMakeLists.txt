set (exe_KThreadPoolTests "KThreadPoolTests.exe" CACHE STRING "KThreadPoolTests.exe")

set ( SOURCES
  ./KThreadPoolTestCases.cpp
  ./KThreadPoolTests.cpp
)

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

# console test
add_library(objects_kthreadpooltests_console OBJECT ${SOURCES})
target_compile_definitions(objects_kthreadpooltests_console PUBLIC "CONSOLE_TEST=1")
target_include_directories(objects_kthreadpooltests_console BEFORE PUBLIC "../")
add_executable(${exe_KThreadPoolTests} $<TARGET_OBJECTS:objects_kthreadpooltests_console>)

set_target_properties(${exe_KThreadPoolTests} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KThreadPoolTests}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${lib_KmUnitUser}
  ${Cxx}
  ${CxxABI}
)

install(
    FILES KThreadPoolTestInput.xml
    DESTINATION ${TEST_OUTPUT_DIR}
)

