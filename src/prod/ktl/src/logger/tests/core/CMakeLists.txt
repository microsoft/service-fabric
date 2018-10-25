set (exe_KtlLogCoreTest "KtlLogCoreUserTests.exe" CACHE STRING "KtlLogCoreUserTests.exe")

include_directories(".")
include_directories("../../src")
include_directories("../../ktlshim")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)
add_definitions(-DKTL_BUILD)


add_executable(${exe_KtlLogCoreTest}
  ../Core.Test.Linux.cpp
  ../RvdLogger.cpp
)

target_compile_definitions(${exe_KtlLogCoreTest}
    PUBLIC "ENABLE_TEST_CTOR=1"
    PUBLIC "CLUSTER_ENABLE_UNITTESTS=1"
    #PUBLIC "VERIFY_QUIET=1"
    PUBLIC "DISPLAY_ON_CONSOLE=1"
    PUBLIC "UDRIVER=1"
    PUBLIC "FEATURE_TEST=1"
    PUBLIC "TEST_CODE"
    )

set_target_properties(${exe_KtlLogCoreTest} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KtlLogCoreTest}
  ${lib_KtlLoggerUser}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${Cxx}
  ${CxxABI}
  ${BoostTest2}
)

install(CODE "EXECUTE_PROCESS(COMMAND cp -a ${EXTERNAL_DIR}/WinFab.Linux.Libs/Boost_1_61_0/rhel/lib/. ${LIBRARY_OUTPUT_DIR})")
