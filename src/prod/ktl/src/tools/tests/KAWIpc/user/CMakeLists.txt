set (exe_KAWIpcUserTests "KAWIpcUserTests.exe" CACHE STRING "KAWIpcUserTests.exe")

#include_directories("../../../../src/KXm/user/inc")

set ( SOURCES
    ../KAWIpcTests.cpp
    ../../kmuser.cpp
)

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

# console test
add_library(objects_KAWIpcusertests_console OBJECT ${SOURCES})
target_compile_definitions(objects_KAWIpcusertests_console PUBLIC "CONSOLE_TEST=1")
target_include_directories(objects_KAWIpcusertests_console BEFORE PUBLIC "../inc")
target_include_directories(objects_KAWIpcusertests_console BEFORE PUBLIC ".")
add_executable(${exe_KAWIpcUserTests} $<TARGET_OBJECTS:objects_KAWIpcusertests_console>)

set_target_properties(${exe_KAWIpcUserTests} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KAWIpcUserTests}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${lib_KmUnitUser}
  ${Cxx}
  ${CxxABI}
)

