set (exe_KNetworkEndpointUserTests "KNetworkEndpointUserTests.exe" CACHE STRING "KNetworkEndpointUserTests.exe")

set ( SOURCES
    ../KNetworkEndpointTests.cpp
    ../../kmuser.cpp
)

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

# console test
add_library(objects_knetworkendpointusertests_console OBJECT ${SOURCES})
target_compile_definitions(objects_knetworkendpointusertests_console PUBLIC "CONSOLE_TEST=1")
target_include_directories(objects_knetworkendpointusertests_console BEFORE PUBLIC "../inc")
target_include_directories(objects_knetworkendpointusertests_console BEFORE PUBLIC ".")
add_executable(${exe_KNetworkEndpointUserTests} $<TARGET_OBJECTS:objects_knetworkendpointusertests_console>)

set_target_properties(${exe_KNetworkEndpointUserTests} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KNetworkEndpointUserTests}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${lib_KmUnitUser}
  ${Cxx}
  ${CxxABI}
)

