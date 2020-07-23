set(sys_root ${ARM_SYS_ROOT} )
set(sys_root_bin ${sys_root}/usr/bin)
set(sys_root_inc ${sys_root}/usr/include)
set(sys_root_lib ${sys_root}/usr/lib)

set(CMAKE_SYSTEM_NAME Linux CACHE INTERNAL "system name")
set(CMAKE_SYSTEM_PROCESSOR arm CACHE INTERNAL "processor")

set(CMAKE_C_FLAGS "--target=${ARM_TARGET_TRIPLE} --sysroot=${sys_root}" CACHE INTERNAL "c compiler flags")
set(CMAKE_CXX_FLAGS "--target=${ARM_TARGET_TRIPLE} --sysroot=${sys_root}" CACHE INTERNAL "cxx compiler flags")

SET(CMAKE_STRIP /usr/bin/${ARM_TARGET_TRIPLE}-strip CACHE STRING "cross compile strip" FORCE)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH CACHE INTERNAL "")
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY CACHE INTERNAL "")
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY CACHE INTERNAL "")
