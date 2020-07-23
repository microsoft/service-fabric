set (exe_KtlAwaitableUserTests "KtlAwaitableUserTests.exe" CACHE STRING "KtlAwaitableUserTests.exe")
set (lib_KtlAwaitableUserTests "KtlAwaitableUserTests" CACHE STRING "KtlAwaitableTests.so")

set ( SOURCES
    ../ktlAwaitabletests.cpp
    ../../kmuser.cpp
)

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

# console test
add_library(objects_ktlawaitableusertests_console OBJECT ${SOURCES})
target_compile_definitions(objects_ktlawaitableusertests_console PUBLIC "CONSOLE_TEST=1")
target_include_directories(objects_ktlawaitableusertests_console BEFORE PUBLIC "../")
add_executable(${exe_KtlAwaitableUserTests} $<TARGET_OBJECTS:objects_ktlawaitableusertests_console>)

set_target_properties(${exe_KtlAwaitableUserTests} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KtlAwaitableUserTests}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${lib_KmUnitUser}
  ${Cxx}
  ${CxxABI}
)

# share test lib
add_library(objects_ktlawaitableusertests OBJECT ${SOURCES})
target_include_directories(objects_ktlawaitableusertests BEFORE PUBLIC "../")
add_library(${lib_KtlAwaitableUserTests} SHARED $<TARGET_OBJECTS:objects_ktlawaitableusertests>)

set_target_properties(${lib_KtlAwaitableUserTests} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${lib_KtlAwaitableUserTests}
  ${lib_ktlfull}
  ${lib_KmUnitUser}
)

