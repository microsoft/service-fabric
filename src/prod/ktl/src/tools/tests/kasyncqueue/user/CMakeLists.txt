set (exe_KAsyncQueueUserTests "KAsyncQueueUserTests.exe" CACHE STRING "KAsyncQueueUserTests.exe")

set ( SOURCES
    ../KAsyncQueueTests.cpp
    ../KAsyncEventTests.cpp
    ../KAsyncServiceTests.cpp
    ../KTimerTests.cpp
    ../KQuotaGateTests.cpp
    ../../kmuser.cpp
)

add_compile_options(-rdynamic)

add_definitions(-DBOOST_TEST_ENABLED)
add_definitions(-DNO_INLINE_EVENTDESCCREATE)

# console test
add_library(objects_kasyncqueueusertests_console OBJECT ${SOURCES})
target_compile_definitions(objects_kasyncqueueusertests_console PUBLIC "CONSOLE_TEST=1")
target_include_directories(objects_kasyncqueueusertests_console BEFORE PUBLIC "../inc")
target_include_directories(objects_kasyncqueueusertests_console BEFORE PUBLIC ".")
add_executable(${exe_KAsyncQueueUserTests} $<TARGET_OBJECTS:objects_kasyncqueueusertests_console>)

set_target_properties(${exe_KAsyncQueueUserTests} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})

target_link_libraries(${exe_KAsyncQueueUserTests}
  ${lib_ktlfull}
  ${lib_KtlLttng}
  ${lib_KmUnitUser}
  ${Cxx}
  ${CxxABI}
)

