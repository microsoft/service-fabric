# Create a lib directory for output during config time
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/lib ${PROJECT_BINARY_DIR}/test)

#define Library names and output properties
set (LIBRARY_OUTPUT_DIR ${PROJECT_BINARY_DIR}/lib CACHE STRING "Library Output directory")
set (TEST_OUTPUT_DIR ${PROJECT_BINARY_DIR}/test CACHE STRING "Test Output directory")

add_subdirectory(serialization)
add_subdirectory(ktlwf)
