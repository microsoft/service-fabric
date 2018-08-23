set (exe_KMemChannelUserTests "KMemChannelUserTests.exe" CACHE STRING "KMemChannelUserTests.exe")

set ( SOURCES
    ../KMemChannelTests.cpp
    ../../kmuser.cpp
)

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

# console test
add_library(objects_kmemchannelusertests_console OBJECT ${SOURCES})
target_compile_definitions(objects_kmemchannelusertests_console PUBLIC "CONSOLE_TEST=1")
target_include_directories(objects_kmemchannelusertests_console BEFORE PUBLIC "../inc")
target_include_directories(objects_kmemchannelusertests_console BEFORE PUBLIC ".")
add_executable(${exe_KMemChannelUserTests} $<TARGET_OBJECTS:objects_kmemchannelusertests_console>)

set_target_properties(${exe_KMemChannelUserTests} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KMemChannelUserTests}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${lib_KmUnitUser}
  ${Cxx}
  ${CxxABI}
)

