set (exe_CheckLinuxLog "CheckLinuxLog.exe" CACHE STRING "CheckLinuxLog.exe")
set (lib_CheckLinuxLog "CheckLinuxLog" CACHE STRING "CheckLinuxLog.so")


include_directories(".")
include_directories("../../src")
include_directories("../../ktlshim")
# pull in InternalKtlLogger.h
include_directories("../../../../../ktl/src/logger/src")

add_definitions(-DUPASSTHROUGH)
add_definitions(-DKTL_BUILD)

set ( SOURCES
    ./checklinuxlog.cpp
)

add_compile_options(-rdynamic)

# console test
add_library(objects_CheckLinuxLog_console OBJECT ${SOURCES})
target_compile_definitions(objects_CheckLinuxLog_console 
    PUBLIC "CONSOLE_TEST=1"
    PUBLIC "UPASSTHROUGH=1"
)
target_include_directories(objects_CheckLinuxLog_console BEFORE PUBLIC ".")
add_executable(${exe_CheckLinuxLog} $<TARGET_OBJECTS:objects_CheckLinuxLog_console>)

set_target_properties(${exe_CheckLinuxLog} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_CheckLinuxLog}
  ${lib_KtlLoggerUser}
  ${lib_Common}
  ${lib_FabricCommon}
  ${lib_KtlLoggerShimUnify}
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
add_library(objects_CheckLinuxLog OBJECT ${SOURCES})
target_include_directories(objects_CheckLinuxLog BEFORE PUBLIC ".")
add_library(${lib_CheckLinuxLog} SHARED $<TARGET_OBJECTS:objects_CheckLinuxLog>)

set_target_properties(${lib_CheckLinuxLog} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${lib_CheckLinuxLog}
  ${lib_ktlfull}
)

