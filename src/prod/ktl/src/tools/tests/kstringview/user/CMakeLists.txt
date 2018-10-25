set (exe_KStringViewUserTests "KStringViewUserTests.exe" CACHE STRING "KStringViewUserTests.exe")

set ( SOURCES
    ../KStringViewTests.cpp
    ../../kmuser.cpp
)

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

# console test
add_library(objects_kstringviewusertests_console OBJECT ${SOURCES})
target_compile_definitions(objects_kstringviewusertests_console PUBLIC "CONSOLE_TEST=1")
target_include_directories(objects_kstringviewusertests_console BEFORE PUBLIC "../inc")
target_include_directories(objects_kstringviewusertests_console BEFORE PUBLIC ".")
add_executable(${exe_KStringViewUserTests} $<TARGET_OBJECTS:objects_kstringviewusertests_console>)

set_target_properties(${exe_KStringViewUserTests} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KStringViewUserTests}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${lib_KmUnitUser}
  ${Cxx}
  ${CxxABI}
)

