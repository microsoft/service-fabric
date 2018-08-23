set (exe_KAsyncContextUserTests "KAsyncContextUserTests.exe" CACHE STRING "KAsyncContextUserTests.exe")

set ( SOURCES
    ../KAsyncContext.cpp
    ../kmuser.cpp
)

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

# console test
add_library(objects_kasynccontextusertests_console OBJECT ${SOURCES})
target_compile_definitions(objects_kasynccontextusertests_console PUBLIC CONSOLE_TEST=1)
target_include_directories(objects_kasynccontextusertests_console BEFORE PUBLIC "../kasynccontext")
add_executable(${exe_KAsyncContextUserTests} $<TARGET_OBJECTS:objects_kasynccontextusertests_console>)

set_target_properties(${exe_KAsyncContextUserTests} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KAsyncContextUserTests}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${lib_KmUnitUser}
  ${Cxx}
  ${CxxABI}
)

