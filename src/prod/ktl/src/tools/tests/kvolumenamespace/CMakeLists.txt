set (exe_KVolumeNamespaceUserTests "KVolumeNamespaceUserTests.exe" CACHE STRING "KVolumeNamespaceUserTests.exe")

set ( SOURCES
    ../KVolumeNamespace.cpp
    ../kmuser.cpp
    ../Common.cpp
)

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

# console test
add_library(objects_kvolumenamespaceusertests_console OBJECT ${SOURCES})
target_compile_definitions(objects_kvolumenamespaceusertests_console PUBLIC "CONSOLE_TEST=1")
target_include_directories(objects_kvolumenamespaceusertests_console BEFORE PUBLIC ".")
add_executable(${exe_KVolumeNamespaceUserTests} $<TARGET_OBJECTS:objects_kvolumenamespaceusertests_console>)

set_target_properties(${exe_KVolumeNamespaceUserTests} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KVolumeNamespaceUserTests}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${lib_KmUnitUser}
  ${Cxx}
  ${CxxABI}
)

install(
    FILES KVolumeNamespaceTestInput.xml
    DESTINATION ${TEST_OUTPUT_DIR}
)
