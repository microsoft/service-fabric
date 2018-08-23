set (exe_KIoBufferUserTests "KIoBufferUserTests.exe" CACHE STRING "KIoBufferUserTests.exe")

set ( SOURCES
    ../KIoBuffer.cpp
    ../kmuser.cpp
)

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

# console test
add_library(objects_kiobufferusertests_console OBJECT ${SOURCES})
target_compile_definitions(objects_kiobufferusertests_console PUBLIC "CONSOLE_TEST=1")
target_include_directories(objects_kiobufferusertests_console BEFORE PUBLIC "../kiobuffer")
add_executable(${exe_KIoBufferUserTests} $<TARGET_OBJECTS:objects_kiobufferusertests_console>)

set_target_properties(${exe_KIoBufferUserTests} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KIoBufferUserTests}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${lib_KmUnitUser}
  ${Cxx}
  ${CxxABI}
)

install(
    FILES ../kiobuffer/KIoBufferTestInput.xml
    DESTINATION ${TEST_OUTPUT_DIR}
)

