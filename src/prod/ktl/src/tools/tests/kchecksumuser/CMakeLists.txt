set (exe_KChecksumUserTests "KChecksumUserTests.exe" CACHE STRING "KChecksumUserTests.exe")

set ( SOURCES
    ../KChecksum.cpp
    ../kmuser.cpp
)

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

# console test
add_library(objects_kchecksumusertests_console OBJECT ${SOURCES})
target_compile_definitions(objects_kchecksumusertests_console PUBLIC CONSOLE_TEST=1)
target_include_directories(objects_kchecksumusertests_console BEFORE PUBLIC "../kchecksum")
add_executable(${exe_KChecksumUserTests} $<TARGET_OBJECTS:objects_kchecksumusertests_console>)

set_target_properties(${exe_KChecksumUserTests} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KChecksumUserTests}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${lib_KmUnitUser}
  ${Cxx}
  ${CxxABI}
)

