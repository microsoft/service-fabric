include_directories("../Linux/user/inc/")
include_directories("../Linux/driver/inc/")

if (CLANG_VERSION_STRING VERSION_GREATER 5.0.0)
  return()
endif()

add_executable(${exe_BlkDeviceLinuxKernelValidation_Test}
  BlkDeviceLinuxKernelValidation.cpp
)

#add_precompiled_header(${exe_TStore_Test} ../stdafx.h)

set_target_properties(
  ${exe_BlkDeviceLinuxKernelValidation_Test}
  PROPERTIES 
  RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR}
)

target_link_libraries(${exe_BlkDeviceLinuxKernelValidation_Test}
  ${lib_ReliableCollectionRuntime}
  ${lib_BlockStore}
  boost_system
  boost_filesystem
  dl
)

#install(
#  FILES ./${exe_BlkDeviceLinuxKernelValidation_Test}.cfg
#  DESTINATION ${TEST_OUTPUT_DIR}
#  RENAME ${exe_BlkDeviceLinuxKernelValidation_Test}.cfg
#)
