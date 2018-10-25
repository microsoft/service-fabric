set (exe_DiskPerfTool "DiskPerfTool.exe" CACHE STRING "DiskPerfTool.exe")

set ( SOURCES
    ./DiskPerfTool.cpp
)

add_compile_options(-rdynamic)

# console test
add_library(objects_DiskPerfTool_console OBJECT ${SOURCES})
target_compile_definitions(objects_DiskPerfTool_console PUBLIC "CONSOLE_TEST=1")
add_executable(${exe_DiskPerfTool} $<TARGET_OBJECTS:objects_DiskPerfTool_console>)

set_target_properties(${exe_DiskPerfTool} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_DiskPerfTool}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${Cxx}
  ${CxxABI}
)

