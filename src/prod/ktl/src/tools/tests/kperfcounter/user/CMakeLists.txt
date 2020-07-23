set (exe_KPerfCounterUserTests "KPerfCounterUserTests.exe" CACHE STRING "KPerfCounterUserTests.exe")
set (lib_KPerfCounterUserTests "KPerfCounterUserTests" CACHE STRING "KPerfCounterUserTests.so")

set ( SOURCES
    ../KPerfCounterTests.cpp
    ../../kmuser.cpp
)

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

# console test
add_library(objects_kperfcounterusertests_console OBJECT ${SOURCES})
target_compile_definitions(objects_kperfcounterusertests_console PUBLIC "CONSOLE_TEST=1")
target_include_directories(objects_kperfcounterusertests_console BEFORE PUBLIC "../")
add_executable(${exe_KPerfCounterUserTests} $<TARGET_OBJECTS:objects_kperfcounterusertests_console>)

set_target_properties(${exe_KPerfCounterUserTests} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KPerfCounterUserTests}
  ${lib_ktlfull}
  ${lib_KmUnitUser}
)

# share test lib
add_library(objects_kperfcounterusertests OBJECT ${SOURCES})
target_include_directories(objects_kperfcounterusertests BEFORE PUBLIC "../")
add_library(${lib_KPerfCounterUserTests} SHARED $<TARGET_OBJECTS:objects_kperfcounterusertests>)

set_target_properties(${lib_KPerfCounterUserTests} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${lib_KPerfCounterUserTests}
  ${lib_ktlfull}
  ${lib_KmUnitUser}
)

