set (exe_ExpandSFLog "ExpandSFLog.exe" CACHE STRING "ExpandSFLog.exe")
set (lib_ExpandSFLog "ExpandSFLog" CACHE STRING "ExpandSFLog.so")


include_directories(".")
include_directories("../../src")
include_directories("../../ktlshim")

add_definitions(-DKTL_BUILD)

set ( SOURCES
    ./ExpandSFLog.cpp
)

add_compile_options(-rdynamic)

# console test
add_library(objects_ExpandSFLog_console OBJECT ${SOURCES})
target_compile_definitions(objects_ExpandSFLog_console 
    PUBLIC "CONSOLE_TEST=1"
    PUBLIC "UPASSTHROUGH=1"
)
target_include_directories(objects_ExpandSFLog_console BEFORE PUBLIC ".")
add_executable(${exe_ExpandSFLog} $<TARGET_OBJECTS:objects_ExpandSFLog_console>)

set_target_properties(${exe_ExpandSFLog} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_ExpandSFLog}
  ${lib_KtlLoggerUser}
  ${lib_Common}
  ${lib_FabricCommon}
  ${lib_ktlfull}
  ktllttng
  ${Cxx}
  ${CxxABI}
  lttng-ust
  uuid
   z
   m
   rt
   pthread
   c
   dl
   xml2
   uuid
)

# share test lib
add_library(objects_ExpandSFLog OBJECT ${SOURCES})
target_include_directories(objects_ExpandSFLog BEFORE PUBLIC ".")
add_library(${lib_ExpandSFLog} SHARED $<TARGET_OBJECTS:objects_ExpandSFLog>)

set_target_properties(${lib_ExpandSFLog} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${lib_ExpandSFLog}
  ${lib_ktlfull}
)

