set (exe_KStreamUserTests "KStreamUserTests.exe" CACHE STRING "KStreamUserTests.exe")

set ( SOURCES
    ../KStreamTests.cpp
    ../../kmuser.cpp
    ../../Common.cpp
)

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

# console test
add_library(objects_kstreamusertests_console OBJECT ${SOURCES})
target_compile_definitions(objects_kstreamusertests_console PUBLIC "CONSOLE_TEST=1")
target_include_directories(objects_kstreamusertests_console BEFORE PUBLIC "../inc")
target_include_directories(objects_kstreamusertests_console BEFORE PUBLIC ".")
add_executable(${exe_KStreamUserTests} $<TARGET_OBJECTS:objects_kstreamusertests_console>)

set_target_properties(${exe_KStreamUserTests} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KStreamUserTests}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${lib_KmUnitUser}
  ${Cxx}
  ${CxxABI}
)

