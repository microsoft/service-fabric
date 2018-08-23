set (exe_KBufferUserTests "KBufferUserTests.exe" CACHE STRING "KBufferUserTests.exe")

set ( SOURCES
    ../KBuffer.cpp
    ../kmuser.cpp
)

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

# console test
add_library(objects_kbufferusertests_console OBJECT ${SOURCES})
target_compile_definitions(objects_kbufferusertests_console PUBLIC "CONSOLE_TEST=1")
target_include_directories(objects_kbufferusertests_console BEFORE PUBLIC "../kbuffer")
add_executable(${exe_KBufferUserTests} $<TARGET_OBJECTS:objects_kbufferusertests_console>)

set_target_properties(${exe_KBufferUserTests} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KBufferUserTests}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${lib_KmUnitUser}
  ${Cxx}
  ${CxxABI}
)

