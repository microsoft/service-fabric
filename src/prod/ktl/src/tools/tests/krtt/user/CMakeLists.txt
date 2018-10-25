set (exe_KRTTUserTests "KRTTUserTests.exe" CACHE STRING "KRTTUserTests.exe")

set ( SOURCES
    ../KRTTTests.cpp
    ../../kmuser.cpp
)

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

# console test
add_library(objects_krttusertests_console OBJECT ${SOURCES})
target_compile_definitions(objects_krttusertests_console PUBLIC "CONSOLE_TEST=1")
target_include_directories(objects_krttusertests_console BEFORE PUBLIC "../inc")
target_include_directories(objects_krttusertests_console BEFORE PUBLIC ".")
add_executable(${exe_KRTTUserTests} $<TARGET_OBJECTS:objects_krttusertests_console>)

set_target_properties(${exe_KRTTUserTests} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KRTTUserTests}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${lib_KmUnitUser}
  ${Cxx}
  ${CxxABI}
)

