set (exe_KBitmapUserTests "KBitmapUserTests.exe" CACHE STRING "KBitmapUserTests.exe")

set ( SOURCES
    ../KBitmap.cpp
    ../kmuser.cpp
)

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

# console test
add_library(objects_kbitmapusertests_console OBJECT ${SOURCES})
target_compile_definitions(objects_kbitmapusertests_console PUBLIC "CONSOLE_TEST=1")
target_include_directories(objects_kbitmapusertests_console BEFORE PUBLIC "../kbitmap")
add_executable(${exe_KBitmapUserTests} $<TARGET_OBJECTS:objects_kbitmapusertests_console>)

set_target_properties(${exe_KBitmapUserTests} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KBitmapUserTests}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${lib_KmUnitUser}
  ${Cxx}
  ${CxxABI}
)

