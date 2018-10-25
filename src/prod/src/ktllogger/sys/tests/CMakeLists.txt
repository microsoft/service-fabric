# pull in InternalKtlLogger.h
include_directories("../../../../ktl/src/logger/src")
include_directories("../inc")

add_subdirectory(Bvt-UPassthrough)
add_subdirectory(Stress-UPassthrough)
add_subdirectory(Feature-UPassthrough)
