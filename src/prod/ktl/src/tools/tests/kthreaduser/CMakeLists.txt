set (exe_KThreadUserTests "KThreadUserTests.exe" CACHE STRING "KThreadUserTests.exe")

set ( SOURCES
    ../KThread.cpp
    ../kmuser.cpp
)

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

# console test
add_library(objects_kthreadusertests_console OBJECT ${SOURCES})
target_compile_definitions(objects_kthreadusertests_console PUBLIC "CONSOLE_TEST=1")
target_include_directories(objects_kthreadusertests_console BEFORE PUBLIC "../kthread")
add_executable(${exe_KThreadUserTests} $<TARGET_OBJECTS:objects_kthreadusertests_console>)

set_target_properties(${exe_KThreadUserTests} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KThreadUserTests}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${lib_KmUnitUser}
  ${Cxx}
  ${CxxABI}
)

