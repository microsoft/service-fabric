set (exe_KXMEventFileTest "KXMEventFileTest.exe" CACHE STRING "KXMEventFileTest.exe")

set( SOURCES
  ./EventFileTest.cpp
)

string(CONCAT KXM_LIB_HEADERS ${PROJECT_SOURCE_DIR} "/src/kxm/user/inc")
string(CONCAT KXM_COMMON_HEADERS ${PROJECT_SOURCE_DIR} "/src/kxm/common")
include_directories(${KXM_LIB_HEADERS})
include_directories(${KXM_COMMON_HEADERS})
include_directories(".")

add_executable(${exe_KXMEventFileTest} ${SOURCES})
#target_link_libraries(${exe_KXMEventFileTest} ${lib_KxmUser})
target_link_libraries(${exe_KXMEventFileTest}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${Cxx}
  ${CxxABI}
)

set_target_properties(${exe_KXMEventFileTest} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})
