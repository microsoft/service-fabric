set (exe_KXMCompletionPortTest "KXMCompletionPortTest.exe" CACHE STRING "KXMCompletionPortTest.exe")

set( SOURCES
  ./ComPortTest.cpp
)

string(CONCAT KXM_LIB_HEADERS ${PROJECT_SOURCE_DIR} "/src/kxm/user/inc")
string(CONCAT KXM_USER_LIB_HEADERS ${PROJECT_SOURCE_DIR} "/src/kxm/user")
string(CONCAT KXM_COMMON_HEADERS ${PROJECT_SOURCE_DIR} "/src/kxm/common")
include_directories(${KXM_LIB_HEADERS})
include_directories(${KXM_USER_LIB_HEADERS})
include_directories(${KXM_COMMON_HEADERS})
include_directories(".")

add_executable(${exe_KXMCompletionPortTest} ${SOURCES})
#target_link_libraries(${exe_KXMCompletionPortTest} ${lib_KxmUser})
target_link_libraries(${exe_KXMCompletionPortTest}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${Cxx}
  ${CxxABI}
)

set_target_properties(${exe_KXMCompletionPortTest} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})
