set (exe_KAllocationBitmapUserTests "KAllocationBitmapUserTests.exe" CACHE STRING "KAllocationBitmapUserTests.exe")

set ( SOURCES
    ../KAllocationBitmap.cpp
    ../kmuser.cpp
)

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

# console test
add_library(objects_kallocationbitmapusertests_console OBJECT ${SOURCES})
target_compile_definitions(objects_kallocationbitmapusertests_console PUBLIC CONSOLE_TEST=1)
target_include_directories(objects_kallocationbitmapusertests_console BEFORE PUBLIC "../kallocationbitmap")
add_executable(${exe_KAllocationBitmapUserTests} $<TARGET_OBJECTS:objects_kallocationbitmapusertests_console>)

set_target_properties(${exe_KAllocationBitmapUserTests} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KAllocationBitmapUserTests}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${lib_KmUnitUser}
  ${Cxx}
  ${CxxABI}
)

