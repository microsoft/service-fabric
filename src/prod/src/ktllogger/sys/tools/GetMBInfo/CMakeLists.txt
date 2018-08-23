set (exe_GetMBInfo "GetMBInfo.exe" CACHE STRING "GetMBInfo.exe")
set (lib_GetMBInfo "GetMBInfo" CACHE STRING "GetMBInfo.so")


include_directories(".")
include_directories("../../src")
include_directories("../../ktlshim")

add_definitions(-DKTL_BUILD)

set ( SOURCES
    ./GetMBInfo.cpp
)

add_compile_options(-rdynamic)

# console test
add_library(objects_GetMBInfo_console OBJECT ${SOURCES})
target_compile_definitions(objects_GetMBInfo_console 
    PUBLIC "CONSOLE_TEST=1"
    PUBLIC "UPASSTHROUGH=1"
)
target_include_directories(objects_GetMBInfo_console BEFORE PUBLIC ".")
add_executable(${exe_GetMBInfo} $<TARGET_OBJECTS:objects_GetMBInfo_console>)

set_target_properties(${exe_GetMBInfo} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_GetMBInfo}
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
add_library(objects_GetMBInfo OBJECT ${SOURCES})
target_include_directories(objects_GetMBInfo BEFORE PUBLIC ".")
add_library(${lib_GetMBInfo} SHARED $<TARGET_OBJECTS:objects_GetMBInfo>)

set_target_properties(${lib_GetMBInfo} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${lib_GetMBInfo}
  ${lib_ktlfull}
)

