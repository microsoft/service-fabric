set (exe_TestLoader "TestLoader" CACHE STRING "TestLoader exe")

include_directories(".")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

add_executable(${exe_TestLoader}
    TestLoader.Tests.cpp
)

set_target_properties(${exe_TestLoader} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_TestLoader}
    ${lib_ktlfull}
    ${lib_KtlLttng}
    ${BoostTest2}
    ${Cxx}
    ${CxxABI}
    dl
    uuid
)

