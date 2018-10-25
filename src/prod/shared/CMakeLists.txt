# Create a lib directory for output during config time
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/lib ${PROJECT_BINARY_DIR}/test)

#define Library names and output properties
set (LIBRARY_OUTPUT_DIR ${PROJECT_BINARY_DIR}/lib)
set (TEST_OUTPUT_DIR ${PROJECT_BINARY_DIR}/test)

add_subdirectory(serialization)
add_subdirectory(ktlwf)
