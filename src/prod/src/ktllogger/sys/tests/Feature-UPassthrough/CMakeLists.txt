
include_directories(".")
include_directories("../../src")
include_directories("../../ktlshim")

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)
add_definitions(-DKTL_BUILD)

add_executable(${exe_KtlLogFeatureUPassthroughTest}
  ../KtlLoggerTests.cpp
  ../MBInfoTests.cpp
  ../RWTStress.cpp
  ../LLWorkload.cpp
  ../Feature.Test.Linux.cpp
  ../RawCoreLoggerTests.cpp
  ../overlaytests.cpp
  ../misctests.cpp
  ../LimitsTests.cpp
  ../ktlloginternal.cpp
  ../MockRvdLogStream.cpp
)

target_compile_definitions(${exe_KtlLogFeatureUPassthroughTest}
    PUBLIC "ENABLE_TEST_CTOR=1"
    PUBLIC "CLUSTER_ENABLE_UNITTESTS=1"
    #PUBLIC "VERIFY_QUIET=1"
    PUBLIC "DISPLAY_ON_CONSOLE=1"
    PUBLIC "UPASSTHROUGH=1"
    PUBLIC "FEATURE_TEST=1"
    PUBLIC "TEST_CODE"
    )

set_target_properties(${exe_KtlLogFeatureUPassthroughTest} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KtlLogFeatureUPassthroughTest}
  ${KtlLoggerUser}
  ${lib_Common}
  ${lib_FabricCommon}
  ${lib_KtlLoggerShimUnify}
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
