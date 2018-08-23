set (exe_KXMBlockFilePerfTest "KXMBlockFilePerfTest.exe" CACHE STRING "KXMBlockFilePerfTest.exe")
set (exe_KXMBlockFileTest "KXMBlockFileTest.exe" CACHE STRING "KXMBlockFileTest.exe")

set( SOURCES
   # This is a perf test and doesn't run as a part of ./RunTests.sh
  ./LinuxIoPerf.cpp
)

set( SOURCES1
  ./BlockFileTest.cpp
)

string(CONCAT KXM_USER_LIB_HEADERS ${PROJECT_SOURCE_DIR} "/src/kxm/user")
string(CONCAT KXM_LIB_HEADERS ${PROJECT_SOURCE_DIR} "/src/kxm/user/inc")
string(CONCAT KXM_COMMON_HEADERS ${PROJECT_SOURCE_DIR} "/src/kxm/common")
include_directories(${KXM_USER_LIB_HEADERS})
include_directories(${KXM_LIB_HEADERS})
include_directories(${KXM_COMMON_HEADERS})
include_directories(".")

add_executable(${exe_KXMBlockFilePerfTest} ${SOURCES})
add_executable(${exe_KXMBlockFileTest} ${SOURCES1})

target_link_libraries(${exe_KXMBlockFileTest}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${Cxx}
  ${CxxABI}
)
target_link_libraries(${exe_KXMBlockFilePerfTest}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${Cxx}
  ${CxxABI}
)
#target_link_libraries(${exe_KXMBlockFileTest} ${lib_KxmUser})
#target_link_libraries(${exe_KXMBlockFilePerfTest} ${lib_KxmUser})

set_target_properties(${exe_KXMBlockFileTest} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})
set_target_properties(${exe_KXMBlockFilePerfTest} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})
