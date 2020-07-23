set (exe_DumpContainer "DumpContainer.exe" CACHE STRING "DumpContainer.exe")
set (lib_DumpContainer "DumpContainer" CACHE STRING "DumpContainer.so")


include_directories(".")
include_directories("../../src")
include_directories("../../ktlshim")

add_definitions(-DUPASSTHROUGH)
add_definitions(-DKTL_BUILD)

set ( SOURCES
    ./dumpcontainer.cpp
)

add_compile_options(-rdynamic)

# console test
add_library(objects_DumpContainer_console OBJECT ${SOURCES})
target_compile_definitions(objects_DumpContainer_console 
    PUBLIC "CONSOLE_TEST=1"
    PUBLIC "UPASSTHROUGH=1"
)
target_include_directories(objects_DumpContainer_console BEFORE PUBLIC ".")
add_executable(${exe_DumpContainer} $<TARGET_OBJECTS:objects_DumpContainer_console>)

set_target_properties(${exe_DumpContainer} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_DumpContainer}
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
add_library(objects_DumpContainer OBJECT ${SOURCES})
target_include_directories(objects_DumpContainer BEFORE PUBLIC ".")
add_library(${lib_DumpContainer} SHARED $<TARGET_OBJECTS:objects_DumpContainer>)

set_target_properties(${lib_DumpContainer} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${lib_DumpContainer}
  ${lib_ktlfull}
)

