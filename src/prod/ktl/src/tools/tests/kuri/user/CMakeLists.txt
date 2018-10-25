set (exe_KUriUserTests "KUriUserTests.exe" CACHE STRING "KUriUserTests.exe")

set ( SOURCES
    ../KUriTests.cpp
    ../../kmuser.cpp
)

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

# console test
add_library(objects_kuriusertests_console OBJECT ${SOURCES})
target_compile_definitions(objects_kuriusertests_console PUBLIC "CONSOLE_TEST=1")
target_include_directories(objects_kuriusertests_console BEFORE PUBLIC "../inc")
target_include_directories(objects_kuriusertests_console BEFORE PUBLIC ".")
add_executable(${exe_KUriUserTests} $<TARGET_OBJECTS:objects_kuriusertests_console>)

set_target_properties(${exe_KUriUserTests} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KUriUserTests}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${lib_KmUnitUser}
  ${Cxx}
  ${CxxABI}
)

