set (exe_KRegistryUserTests "KRegistryUserTests.exe" CACHE STRING "KRegistryUserTests.exe")
set (lib_KRegistryUserTests "KRegistryUserTests" CACHE STRING "KRegistryUserTests.so")

set ( SOURCES
    ../KRegistryTests.cpp
    ../../kmuser.cpp
)

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

# console test
add_library(objects_kregistryusertests_console OBJECT ${SOURCES})
target_compile_definitions(objects_kregistryusertests_console PUBLIC "CONSOLE_TEST=1")
target_include_directories(objects_kregistryusertests_console BEFORE PUBLIC "../")
add_executable(${exe_KRegistryUserTests} $<TARGET_OBJECTS:objects_kregistryusertests_console>)

set_target_properties(${exe_KRegistryUserTests} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KRegistryUserTests}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${lib_KmUnitUser}
  ${Cxx}
  ${CxxABI}
)

# share test lib
add_library(objects_kregistryusertests OBJECT ${SOURCES})
target_include_directories(objects_kregistryusertests BEFORE PUBLIC "../")
add_library(${lib_KRegistryUserTests} SHARED $<TARGET_OBJECTS:objects_kregistryusertests>)

set_target_properties(${lib_KRegistryUserTests} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${lib_KRegistryUserTests}
  ${lib_ktlfull}
  ${lib_KmUnitUser}
)

