cmake_minimum_required (VERSION 2.6)
project (KtlUser)

EXECUTE_PROCESS( COMMAND ${CMAKE_CXX_COMPILER} --version OUTPUT_VARIABLE clang_ver_str )
string (REGEX REPLACE ".*clang version ([0-9]+\\.[0-9]+\\.[0-9]).*" "\\1" CLANG_VERSION_STRING ${clang_ver_str})

if (CLANG_VERSION_STRING VERSION_GREATER 5.0.0)
  add_compile_options(-fdeclspec)
  add_definitions(-DCLANG_5_0_1_PLUS)
endif()

set(WARNINGS)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

option(ARM_CROSS_COMPILE OFF)
if (ARM_CROSS_COMPILE)
    set(EXTERNAL_DEPS_DIR "${ARM_SYS_ROOT}/external")
else()
    set(EXTERNAL_DEPS_DIR "${PROJECT_SOURCE_DIR}/../../../../external/")
endif()

option(BUILD_THIRD_PARTY OFF)
if (BUILD_THIRD_PARTY)
    set(THIRD_PARTY_LIB_DIR "${PROJECT_SOURCE_DIR}/../../../../deps/third-party/bin")
else()
    set(THIRD_PARTY_LIB_DIR "${EXTERNAL_DEPS_DIR}/WinFab.Linux.Libs")
endif()

# Dependent libs
set (EXTERNAL_LIB_PATH ${EXTERNAL_DEPS_DIR}/WinFab.Linux.Libs)

# configure PAL
add_definitions(-DPLATFORM_UNIX=1)
add_definitions(-D__LINUX__=1)
add_definitions(-DUSE_STL=1)
add_definitions(-DUSE_STDLIB=1)
add_definitions(-DUNICODE=1)
add_definitions(-DUSE_PLATFORM_MATH=1)
# paulmey: still unsure what all these do, but ProjectK had them in the root CMakeLists.txt
add_definitions(-DLP64COMPATIBLE=1)
#add_definitions(-DFEATURE_PAL=1) // When developing a PAL feature. Enabling it undefs UNICODE string et al
add_definitions(-DCORECLR=1)
add_definitions(-DPIC=1)
#add_definitions(-DFEATURE_PAL_INLINABLE_TLS=1)
add_definitions(-DBIT64=1)
add_definitions(-D_WIN64=1)

# sriramsh Pre processor definitions used in ServiceModel.props when compiling Windows Fabric.
# These have effect on PAL
add_definitions(-D_STATIC_CPPLIB)
add_definitions(-D_USE_DECLSPECS_FOR_SAL=1)

# From MSDN: The declarations in the Winsock.h header file will conflict with the declarations
# in the Winsock2.h header file required by Windows Sockets 2. The WIN32_LEAN_AND_MEAN
# macro prevents the Winsock.h from being included by the Windows.h header.
add_definitions(-DWIN32_LEAN_AND_MEAN=1)
add_definitions(-DSTRSAFE_LIB)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDBG=1)
endif()

# if this is defined, asserts are voided. This was defined in Windows Fabric Compilation.
add_definitions(-DNDEBUG)

# Additional defines from Winfab compilation
add_definitions(-DDEVL=1)
#add_definitions(-D_DISABLE_DEPRECATE_STATIC_CPPLIB)
add_definitions(-DKTL_BUILD=1)
add_definitions(-DKTL_USER_MODE=1)
add_definitions(-DKTL_MIXED_STL_ATL)

add_definitions(-DTODO_KTL_INCLUDE=0)
add_definitions(-DLINUX_DISABLE)

add_definitions(-DWINFAB_REVIEW_PORTING_CHANGE=)

add_definitions(-D__SSE4_2__)
add_definitions(-D__SSE4_1__)
add_definitions(-D__SSSE3__)
add_definitions(-D__SSE3__)
add_definitions(-DRTCPAL_INCLUDED)
#add_definitions(-DCONSOLE_TEST)

# enable KXM
add_definitions(-DUSE_KXM=1)

if(EXISTS "/etc/debian_version")
  set(LINUX_DISTRIBUTION DEBIAN)
  add_definitions(-DLINUX_DISTRIB_DEBIAN)
elseif(EXISTS "/etc/redhat-release")
  set(LINUX_DISTRIBUTION REDHAT)
  add_definitions(-DLINUX_DISTRIB_REDHAT)
endif()
message(STATUS "Distribution: ${LINUX_DISTRIBUTION}")


include(CheckCCompilerFlag)
check_c_compiler_flag(-fcoroutines-ts HAS_COROUTINE_SUPPORT)
if (HAS_COROUTINE_SUPPORT)
    add_definitions(-DK_UseResumable)
    add_compile_options(-fcoroutines-ts)
endif()

# CPP Flags
add_compile_options(-stdlib=libc++)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
# Adding linker flags for static library
set (CMAKE_STATIC_LIBRARY_FLAGS "-pthread")
# Adding linker flags for shared library
set (CMAKE_SHARED_LINKER_FLAGS "-pthread")

set (CMAKE_EXE_LINKER_FLAGS "-pthread")

# ms extensions
add_compile_options(-fms-extensions)
add_compile_options(-fdelayed-template-parsing)

add_compile_options(-fno-limit-debug-info)

# 2-byte wchar
add_compile_options(-fshort-wchar)
# debugging support
add_compile_options(-gdwarf-3)
add_compile_options(-dD)
#add_compile_options(-dD -save-temps)
# generate unwind info
add_compile_options(-fexceptions)
# generate stack cookies
add_compile_options(-fstack-protector-all)
# disable special handling for built in functions
add_compile_options(-fno-builtin)

# optionally include this option for CLANG to continue on errors
add_compile_options(-ferror-limit=0)
add_compile_options(-Wno-multichar)

# Build position independent to be included in shared libraries
add_compile_options(-fPIC)
# better diagnostics
add_compile_options(-fdiagnostics-show-template-tree
                    -fno-elide-type)

# This should be suppressed explicitly because variadic macros are not portable
# across compilers, MSVC and Clang support GCC way of handling it, and we are using it.
add_compile_options(-Wno-gnu-zero-variadic-macro-arguments)

# enable diagnostics. 
#add_compile_options(-Weverything)
add_compile_options(-w)
add_compile_options(-Wmicrosoft-exists)
add_compile_options(-Wno-pedantic)
add_compile_options(-Wno-c++98-compat-pedantic)
add_compile_options(-Wno-c++98-compat)
add_compile_options(-Wno-dollar-in-identifier-extension)
add_compile_options(-Wno-language-extension-token)
add_compile_options(-Wno-unused-macros)
add_compile_options(-Wno-unused-const-variable)
add_compile_options(-Wno-deprecated)
add_compile_options(-Wno-undef)
add_compile_options(-Wno-padded)
add_compile_options(-Wno-cast-align)
add_compile_options(-Wno-old-style-cast)
add_compile_options(-Wno-ignored-attributes)
add_compile_options(-Wno-unused-value)
add_compile_options(-Wno-extra-semi)
add_compile_options(-Wno-microsoft)
add_compile_options(-Wno-shadow)
add_compile_options(-Wno-sign-conversion)
add_compile_options(-Wno-constant-conversion)
add_compile_options(-Wno-tautological-compare)
add_compile_options(-Wno-weak-vtables)
add_compile_options(-Wno-non-virtual-dtor)
add_compile_options(-Wno-reorder)
add_compile_options(-Wno-newline-eof)
add_compile_options(-Wno-exit-time-destructors)
add_compile_options(-Wno-ignored-qualifiers)
add_compile_options(-Wno-shift-sign-overflow)
add_compile_options(-Wno-global-constructors)
add_compile_options(-Wno-macro-redefined)
add_compile_options(-Wno-writable-strings)
add_compile_options(-Wno-missing-declarations)
add_compile_options(-Wno-reserved-id-macro)

add_compile_options(-Wno-c++11-narrowing)

# Create a lib directory for output during config time
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/lib ${PROJECT_BINARY_DIR}/test)

include_directories ("./inc")
include_directories ("./linuxinc")
include_directories ("./boost/inc") # for boost-taef.h

# BUG!!!: These inc dirs are forming an upward source dependency on SF
include_directories ("${PROJECT_SOURCE_DIR}/../../src/pal/src")

set(BOOST_ROOT "${THIRD_PARTY_LIB_DIR}/Boost_1_61_0")
set(BOOST_INCLUDEDIR "${BOOST_ROOT}/boost")
set(BOOST_LIBRARY_DIRS "${BOOST_ROOT}/lib")

include_directories(${BOOST_ROOT}) # Some things include headers like boost/xxxx.hpp, so need root dir as include path as well
include_directories(${BOOST_INCLUDEDIR})
link_directories(${BOOST_LIBRARY_DIRS})

find_package(Boost REQUIRED COMPONENTS random chrono system thread regex filesystem)
find_library(BoostTest2 boost_unit_test_framework PATHS ${BOOST_LIBRARY_DIRS} NO_DEFAULT_PATH)

# libraries
# for Cxx 
set (CXX_INC_PATH "${THIRD_PARTY_LIB_DIR}/Cxx/include/c++/v1")
if (BUILD_THIRD_PARTY)
    set (CXX_LIB_PATH "${THIRD_PARTY_LIB_DIR}/Cxx/lib")
else()
    set (CXX_LIB_PATH "${THIRD_PARTY_LIB_DIR}/Cxx")
endif()
include_directories ("${CXX_INC_PATH}")
link_directories("${CXX_LIB_PATH}")

# Dependent libs
if (BUILD_THIRD_PARTY)
    find_library(Cxx c++ PATHS ${CXX_LIB_PATH} NO_DEFAULT_PATH)
    find_library(CxxABI c++abi PATHS ${CXX_LIB_PATH} NO_DEFAULT_PATH)
else()
    find_library(Cxx cxx PATHS ${CXX_LIB_PATH} NO_DEFAULT_PATH)
    find_library(CxxABI cxxabi PATHS ${CXX_LIB_PATH} NO_DEFAULT_PATH)
endif()
set (CMAKE_CXX_STANDARD_LIBRARIES "${Cxx} ${CxxABI}")

# Create a lib directory for output during config time
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

# define Library names and output properties
set (LIBRARY_OUTPUT_DIR ${PROJECT_BINARY_DIR}/lib)
set (TEST_OUTPUT_DIR    ${PROJECT_BINARY_DIR}/test)
set (lib_KtlLttng "ktllttng" CACHE STRING "Ktl Lttng library")
set (lib_KtlUser "ktluser" CACHE STRING "Ktl Userspace library")
set (lib_ktlcore "ktlcore" CACHE STRING "KTL Core Library")
set (lib_ktlfull "ktlfull" CACHE STRING "KTL Full Library")
set (lib_KtlLoggerUser "KtlLoggerUser" CACHE STRING "KTL Logger (User) Library")
set (lib_KtlLoggerShimUser "KtlLoggerShimUser" CACHE STRING "KTL Logger Shim Library")
set (lib_KtlLoggerShimUDriver "KtlLoggerShimUDriver" CACHE STRING "KTL Logger UDriver Library")
set (lib_KmUnitUser "KmUnitUser" CACHE STRING "KTL Unit Test User Mode Library")
set (lib_KxmUser "KxmUser" CACHE STRING "KXM User API Library")

#add_custom_target(postbuild
#    COMMAND ${CMAKE_MAKE_PROGRAM} install
#    )

add_subdirectory (src)
add_subdirectory (tools)
add_subdirectory (logger)
add_subdirectory (test)
add_subdirectory (linuxsetup)
