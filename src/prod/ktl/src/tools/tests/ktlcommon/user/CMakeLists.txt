set (exe_KtlCommonUserTests "KtlCommonUserTests.exe" CACHE STRING "KtlCommonUserTests.exe")

set ( SOURCES
    ../KtlCommonTests.cpp
    ../KTextTest.cpp
    ../../kmuser.cpp
)

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

# console test
add_library(objects_ktlcommonusertests_console OBJECT ${SOURCES})
target_compile_definitions(objects_ktlcommonusertests_console PUBLIC "CONSOLE_TEST=1")
target_include_directories(objects_ktlcommonusertests_console BEFORE PUBLIC "../inc")
target_include_directories(objects_ktlcommonusertests_console BEFORE PUBLIC ".")
add_executable(${exe_KtlCommonUserTests} $<TARGET_OBJECTS:objects_ktlcommonusertests_console>)

set_target_properties(${exe_KtlCommonUserTests} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KtlCommonUserTests}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${lib_KmUnitUser}
  ${Cxx}
  ${CxxABI}
)

