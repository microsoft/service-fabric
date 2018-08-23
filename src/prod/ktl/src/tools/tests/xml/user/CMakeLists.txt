set (exe_XmlUserTests "XmlUserTests.exe" CACHE STRING "XmlUserTests.exe")

set ( SOURCES
    ../DomTests.cpp
    ../XmlTestCases.cpp
    ../XmlTests.cpp
)

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

# console test
add_library(objects_xmlusertests_console OBJECT ${SOURCES})
target_compile_definitions(objects_xmlusertests_console PUBLIC "CONSOLE_TEST=1")
target_include_directories(objects_xmlusertests_console BEFORE PUBLIC "../inc")
add_executable(${exe_XmlUserTests} $<TARGET_OBJECTS:objects_xmlusertests_console>)

set_target_properties(${exe_XmlUserTests} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_XmlUserTests}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${lib_KmUnitUser}
  ${Cxx}
  ${CxxABI}
)

