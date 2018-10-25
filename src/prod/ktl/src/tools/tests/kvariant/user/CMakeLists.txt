set (exe_KVariantUserTests "KVariantUserTests.exe" CACHE STRING "KVariantUserTests.exe")

set ( SOURCES
    ../KVariantTests.cpp
    ../../kmuser.cpp
)

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

# console test
add_library(objects_kvariantusertests_console OBJECT ${SOURCES})
target_compile_definitions(objects_kvariantusertests_console PUBLIC "CONSOLE_TEST=1")
target_include_directories(objects_kvariantusertests_console BEFORE PUBLIC "../inc")
target_include_directories(objects_kvariantusertests_console BEFORE PUBLIC ".")
add_executable(${exe_KVariantUserTests} $<TARGET_OBJECTS:objects_kvariantusertests_console>)

set_target_properties(${exe_KVariantUserTests} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KVariantUserTests}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${lib_KmUnitUser}
  ${Cxx}
  ${CxxABI}
)

