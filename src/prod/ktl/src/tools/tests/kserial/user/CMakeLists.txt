set (exe_KSerialUserTests "KSerialUserTests.exe" CACHE STRING "KSerialUserTests.exe")

set ( SOURCES
    ../KSerialTests.cpp
    ../../kmuser.cpp
)

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

# console test
add_library(objects_kserialusertests_console OBJECT ${SOURCES})
target_compile_definitions(objects_kserialusertests_console PUBLIC "CONSOLE_TEST=1")
target_include_directories(objects_kserialusertests_console BEFORE PUBLIC "../inc")
target_include_directories(objects_kserialusertests_console BEFORE PUBLIC ".")
add_executable(${exe_KSerialUserTests} $<TARGET_OBJECTS:objects_kserialusertests_console>)

set_target_properties(${exe_KSerialUserTests} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KSerialUserTests}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${lib_KmUnitUser}
  ${Cxx}
  ${CxxABI}
)

