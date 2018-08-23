set (exe_wcarInproc "wcarInproc.exe" CACHE STRING "wcarInproc.exe")
set (lib_wcarInproc "wcarInproc" CACHE STRING "wcarInproc.so")


include_directories(".")
include_directories("../../../src")
include_directories("../../../ktlshim")
# pull in InternalKtlLogger.h
include_directories("../../../../../../ktl/src/logger/src")

add_definitions(-DUPASSTHROUGH)
add_definitions(-DKTL_BUILD)

set ( SOURCES
    ../wcar.cpp
)

add_compile_options(-rdynamic)

# console test
add_library(objects_wcarInproc_console OBJECT ${SOURCES})
target_compile_definitions(objects_wcarInproc_console 
    PUBLIC "CONSOLE_TEST=1"
    PUBLIC "UPASSTHROUGH=1"
)
target_include_directories(objects_wcarInproc_console BEFORE PUBLIC ".")
add_executable(${exe_wcarInproc} $<TARGET_OBJECTS:objects_wcarInproc_console>)

set_target_properties(${exe_wcarInproc} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_wcarInproc}
  ${lib_Common}
  ${lib_FabricCommon}
  ${lib_KtlLoggerShimUnify}
  ${lib_KtlLoggerUser}
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
add_library(objects_wcarInproc OBJECT ${SOURCES})
target_include_directories(objects_wcarInproc BEFORE PUBLIC ".")
add_library(${lib_wcarInproc} SHARED $<TARGET_OBJECTS:objects_wcarInproc>)

set_target_properties(${lib_wcarInproc} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${lib_wcarInproc}
  ${lib_ktlfull}
)

