set (exe_KDeferredCallUserTests "KDeferredCallUserTests.exe" CACHE STRING "KDeferredCallUserTests.exe")

set ( SOURCES
    ../KDeferredCallTests.cpp
    ../../kmuser.cpp
)

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

# console test
add_library(objects_kdeferredcallusertests_console OBJECT ${SOURCES})
target_compile_definitions(objects_kdeferredcallusertests_console PUBLIC "CONSOLE_TEST=1")
target_include_directories(objects_kdeferredcallusertests_console BEFORE PUBLIC "../inc")
target_include_directories(objects_kdeferredcallusertests_console BEFORE PUBLIC ".")
add_executable(${exe_KDeferredCallUserTests} $<TARGET_OBJECTS:objects_kdeferredcallusertests_console>)

set_target_properties(${exe_KDeferredCallUserTests} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KDeferredCallUserTests}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${lib_KmUnitUser}
  ${Cxx}
  ${CxxABI}
)

