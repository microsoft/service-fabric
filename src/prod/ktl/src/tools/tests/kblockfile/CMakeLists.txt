set (exe_KBlockFileUserTests "KBlockFileUserTests.exe" CACHE STRING "KBlockFileUserTests.exe")

set ( SOURCES
    ../KBlockFile.cpp
    ../kmuser.cpp
    ../Common.cpp
)

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

# console test
add_library(objects_kblockfileusertests_console OBJECT ${SOURCES})
target_compile_definitions(objects_kblockfileusertests_console PUBLIC "CONSOLE_TEST=1")
target_include_directories(objects_kblockfileusertests_console BEFORE PUBLIC ".")
add_executable(${exe_KBlockFileUserTests} $<TARGET_OBJECTS:objects_kblockfileusertests_console>)

set_target_properties(${exe_KBlockFileUserTests} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KBlockFileUserTests}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${lib_KmUnitUser}
  ${Cxx}
  ${CxxABI}
)

install(
    FILES KBlockFileTestInput.xml
    DESTINATION ${TEST_OUTPUT_DIR}
)
