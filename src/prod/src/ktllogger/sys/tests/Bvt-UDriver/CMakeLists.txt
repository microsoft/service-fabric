
include_directories(".")
include_directories("../../src")
include_directories("../../ktlshim")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)
add_definitions(-DKTL_BUILD)

add_executable(${exe_KtlLogBvtUDriverTest}
  ../KtlLoggerTests.cpp
  ../RWTStress.cpp
  ../LLWorkload.cpp
  ../Bvt.Test.Linux.cpp
  ../overlaytests.cpp
  ../LimitsTests.cpp
  ../MockRvdLogStream.cpp
)

target_compile_definitions(${exe_KtlLogBvtUDriverTest}
    PUBLIC "ENABLE_TEST_CTOR=1"
    PUBLIC "CLUSTER_ENABLE_UNITTESTS=1"
    #PUBLIC "VERIFY_QUIET=1"
    PUBLIC "DISPLAY_ON_CONSOLE=1"
    PUBLIC "UDRIVER=1"
    PUBLIC "FEATURE_TEST=1"
    PUBLIC "TEST_CODE"
    )

set_target_properties(${exe_KtlLogBvtUDriverTest} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KtlLogBvtUDriverTest}
  ${KtlLoggerUser}
  ${lib_Common}
  ${lib_FabricCommon}
  ${lib_KtlLoggerShimUDriver}
  ${Ktlfull}
  ktllttng
  KmUnitUser
  uuid
  ${BoostTest2}
  ${Cxx}
  ${CxxABI}
  lttng-ust
   z
   m
   rt
   pthread
   c
   dl
   xml2
   uuid
)
