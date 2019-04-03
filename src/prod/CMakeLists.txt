cmake_minimum_required (VERSION 2.6)

# build ktl first as it has its own "toplevel" CMakeLists.txt
add_subdirectory (ktl)

if(NOT (BUILD_KTL_ONLY STREQUAL "ON"))

# now build ServiceFabric
project (ServiceFabric)

set(WARNINGS)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

include(CMakeParseArguments)

EXECUTE_PROCESS( COMMAND ${CMAKE_CXX_COMPILER} --version OUTPUT_VARIABLE clang_ver_str )
string (REGEX REPLACE ".*clang version ([0-9]+\\.[0-9]+).*" "\\1" CLANG_VERSION_STRING ${clang_ver_str})

if(EXISTS "/etc/debian_version")
  set(LINUX_DISTRIBUTION DEBIAN)
  add_definitions(-DLINUX_DISTRIB_DEBIAN)
elseif(EXISTS "/etc/redhat-release")
  set(LINUX_DISTRIBUTION REDHAT)
  add_definitions(-DLINUX_DISTRIB_REDHAT)
endif()
message(STATUS "Distribution: ${LINUX_DISTRIBUTION}")

string(TOUPPER "${CMAKE_BUILD_TYPE}" BuildType)
if(BuildType STREQUAL "DEBUG")
set(OPT_LEVEL "")
else(BuildType STREQUAL "DEBUG")
set(OPT_LEVEL "-O2")
endif()

set (CXX_STD "-std=c++14")

EXECUTE_PROCESS( COMMAND ${CMAKE_CXX_COMPILER} --version OUTPUT_VARIABLE clang_ver_str )
string (REGEX REPLACE ".*clang version ([0-9]+\\.[0-9]+\\.[0-9]).*" "\\1" CLANG_VERSION_STRING ${clang_ver_str})

set(sf_compile_flags "")
MACRO(SF_ADD_COMPILE_OPTIONS _flag)
  add_compile_options(${_flag})
  set(sf_compile_flags "${sf_compile_flags} ${_flag}")
ENDMACRO(SF_ADD_COMPILE_OPTIONS)

if (CLANG_VERSION_STRING VERSION_GREATER 5.0.0)
  sf_add_compile_options(-fdeclspec)
  add_definitions(-DCLANG_5_0_1_PLUS)
endif()

MACRO(ADD_PRECOMPILED_HEADER _targetName _inputhdr)
if(NOT (SF_DISABLE_PRECOMPILE STREQUAL "ON"))
  set(_input "${CMAKE_CURRENT_SOURCE_DIR}/${_inputhdr}")

  GET_FILENAME_COMPONENT(_name ${_input} NAME)
  GET_FILENAME_COMPONENT(_path ${_input} PATH)
  SET(_output "${CMAKE_CURRENT_BINARY_DIR}/${_name}.pch")

  SET(_compile_flags ${sf_compile_flags})
    
  GET_DIRECTORY_PROPERTY(_directory_flags INCLUDE_DIRECTORIES)
  FOREACH(dir ${_directory_flags})
    LIST(APPEND _compile_flags "-I${dir}")
  ENDFOREACH(dir)

  GET_DIRECTORY_PROPERTY(_directory_flags COMPILE_DEFINITIONS)
  FOREACH(dir ${_directory_flags})
    LIST(APPEND _compile_flags "-D${dir}")
  ENDFOREACH(dir)

  LIST(APPEND _compile_flags ${CMAKE_CXX_FLAGS} )

  SEPARATE_ARGUMENTS(_compile_flags)
  
  ADD_CUSTOM_COMMAND(OUTPUT ${_output} 
    COMMAND ${CMAKE_CXX_COMPILER} ${_compile_flags} -x c++-header 
            ${OPT_LEVEL} ${CXX_STD} -emit-pch -o ${_output} ${_input}
    IMPLICIT_DEPENDS CXX ${_input}
    DEPENDS ${_input} 
  )
  ADD_CUSTOM_TARGET(${_targetName}_pch DEPENDS  ${_output})
  ADD_DEPENDENCIES(${_targetName} ${_targetName}_pch)
  SET_TARGET_PROPERTIES(${_targetName} PROPERTIES COMPILE_FLAGS "-include-pch ${_output} -Winvalid-pch")
endif()
ENDMACRO(ADD_PRECOMPILED_HEADER)

find_program(AWK awk)
if (AWK STREQUAL "AWK-NOTFOUND")
    message(FATAL_ERROR "AWK not found")
endif()

function(generate_exports_file tgt inFile outFile)
  set(AWK_SCRIPT symexports.awk)
  add_custom_command(
    TARGET ${tgt} PRE_LINK
    COMMAND ${AWK} -f ${PROJECT_SOURCE_DIR}/tools/linux/cmake/${AWK_SCRIPT} ${inFile} > ${outFile} 2>/dev/null
    DEPENDS ${inFile} ${PROJECT_SOURCE_DIR}/tools/linux/cmake/${AWK_SCRIPT}
    COMMENT "Generating exports file ${outFile}"
  )
  set_source_files_properties(${outFile}
                              PROPERTIES GENERATED TRUE)
endfunction()

# configure PAL
add_definitions(-DPLATFORM_UNIX=1)
add_definitions(-D__LINUX__=1)
add_definitions(-DUSE_STL=1)
add_definitions(-DUSE_STDLIB=1)
add_definitions(-DUNICODE=1)
add_definitions(-DUSE_PLATFORM_MATH=1)

# ProjectK had them in the root CMakeLists.txt
add_definitions(-DLP64COMPATIBLE=1)
add_definitions(-DCORECLR=1)
add_definitions(-DPIC=1)
add_definitions(-DBIT64=1)
add_definitions(-D_WIN64=1)
add_definitions(-DPAL_STDCPP_COMPAT=1)

# Preprocessor definitions used in ServiceModel.props. These have effect on PAL
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

set(KTL_LIB_DIR "${CMAKE_BINARY_DIR}/ktl/src/lib")
set(KtlLoggerUser "KtlLoggerUser")
set(Ktlcore "ktlcore")
set(Ktlfull "ktlfull")

# if this is defined, asserts are voided. This was defined in Windows Fabric Compilation.
add_definitions(-DNDEBUG)

# Additional defines from Winfab compilation
add_definitions(-DDEVL=1)
#add_definitions(-D_DISABLE_DEPRECATE_STATIC_CPPLIB)
add_definitions(-DKTL_USER_MODE=1)
add_definitions(-DKTL_MIXED_STL_ATL)

add_definitions(-DTODO_KTL_INCLUDE=0)
add_definitions(-DLINUX_DISABLE)

add_definitions(-DWINFAB_REVIEW_PORTING_CHANGE=)

# ktl
if (CLANG_VERSION_STRING VERSION_GREATER 3.9)
  add_definitions(-DK_UseResumable)
  sf_add_compile_options(-fcoroutines-ts)
endif()

# CPP Flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_STD}")
sf_add_compile_options(-stdlib=libc++)

set (CMAKE_STATIC_LIBRARIES_FLAGS "-pthread")
set (CMAKE_STATIC_LIBRARIES_FLAGS "${CMAKE_STATIC_LIBRARY_FLAGS} -Wl,--build-id")
set (CMAKE_SHARED_LINKER_FLAGS "-pthread")
set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--build-id -Wl,--allow-multiple-definition")
set (CMAKE_EXE_LINKER_FLAGS "-pthread")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--build-id -Wl,--allow-multiple-definition")

# ms extensions
#sf_add_compile_options(-v)
sf_add_compile_options(-fms-extensions)
sf_add_compile_options(-fdelayed-template-parsing)
# 2-byte wchar
sf_add_compile_options(-fshort-wchar)
# debugging support
sf_add_compile_options(-g)
sf_add_compile_options(-dD)
#sf_add_compile_options(-save-temps)
# generate unwind info
sf_add_compile_options(-fexceptions)
# generate stack cookies
sf_add_compile_options(-fstack-protector-all)
# disable special handling for built in functions
sf_add_compile_options(-fno-builtin)

# optionally include this option for CLANG to continue on errors
sf_add_compile_options(-ferror-limit=0)
sf_add_compile_options(-Wno-multichar)

# Build position independent to be included in shared libraries
sf_add_compile_options(-fPIC)
# better diagnostics
sf_add_compile_options(-fdiagnostics-show-template-tree
                    -fno-elide-type)

# This should be suppressed explicitly because variadic macros are not portable
# across compilers, MSVC and Clang support GCC way of handling it, and we are using it.
sf_add_compile_options(-Wno-gnu-zero-variadic-macro-arguments)

# enable diagnostics.
#sf_add_compile_options(-Weverything)
sf_add_compile_options(-w)
sf_add_compile_options(-Wmicrosoft-exists)
sf_add_compile_options(-Wno-pedantic)
sf_add_compile_options(-Wno-c++98-compat-pedantic)
sf_add_compile_options(-Wno-c++98-compat)
sf_add_compile_options(-Wno-dollar-in-identifier-extension)
sf_add_compile_options(-Wno-language-extension-token)
sf_add_compile_options(-Wno-unused-macros)
sf_add_compile_options(-Wno-unused-const-variable)
sf_add_compile_options(-Wno-deprecated)
sf_add_compile_options(-Wno-undef)
sf_add_compile_options(-Wno-padded)
sf_add_compile_options(-Wno-cast-align)
sf_add_compile_options(-Wno-old-style-cast)
sf_add_compile_options(-Wno-ignored-attributes)
sf_add_compile_options(-Wno-unused-value)
sf_add_compile_options(-Wno-extra-semi)
sf_add_compile_options(-Wno-microsoft)
sf_add_compile_options(-Wno-shadow)
sf_add_compile_options(-Wno-sign-conversion)
sf_add_compile_options(-Wno-constant-conversion)
sf_add_compile_options(-Wno-tautological-compare)
sf_add_compile_options(-Wno-weak-vtables)
sf_add_compile_options(-Wno-non-virtual-dtor)
sf_add_compile_options(-Wno-reorder)
sf_add_compile_options(-Wno-newline-eof)
sf_add_compile_options(-Wno-exit-time-destructors)
sf_add_compile_options(-Wno-ignored-qualifiers)
sf_add_compile_options(-Wno-shift-sign-overflow)
sf_add_compile_options(-Wno-global-constructors)
sf_add_compile_options(-Wno-macro-redefined)
sf_add_compile_options(-Wno-writable-strings)
sf_add_compile_options(-Wno-missing-declarations)
sf_add_compile_options(-Wno-reserved-id-macro)

# Create a lib directory for output during config time
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/lib ${PROJECT_BINARY_DIR}/test)

option(ARM_CROSS_COMPILE OFF)
if (ARM_CROSS_COMPILE)
    set(EXTERNAL_DEPS_DIR "${ARM_SYS_ROOT}/external")
else()
    set(EXTERNAL_DEPS_DIR "${PROJECT_SOURCE_DIR}/../../external/")
endif()

set(MANAGED_DEPS_DIR "${PROJECT_SOURCE_DIR}/../../external/") 

option(BUILD_THIRD_PARTY OFF)
if (BUILD_THIRD_PARTY)
    set(THIRD_PARTY_LIB_DIR "${PROJECT_SOURCE_DIR}/../../deps/third-party/bin")
else()
    set(THIRD_PARTY_LIB_DIR "${EXTERNAL_DEPS_DIR}/WinFab.Linux.Libs")
endif()

# Dependent libs
set (EXTERNAL_LIB_PATH ${EXTERNAL_DEPS_DIR}/WinFab.Linux.Libs)
set (CLRPAL_LIB_PATH ${EXTERNAL_LIB_PATH}/ClrPal)
set (BOOST_TEST_PATH ${THIRD_PARTY_LIB_DIR}/Boost_1_61_0/lib)
set (CASABLANCA_LIB_PATH ${THIRD_PARTY_LIB_DIR}/casablanca_v_2/lib)
set (LIBSSH2_LIB_PATH ${EXTERNAL_DEPS_DIR}/SF.Linux.Libssh2.1.5.1/lib/x64)

# for WindowsFabric
include_directories (".")
include_directories ("src")
include_directories ("test")
include_directories ("shared")
include_directories ("shared/serialization/inc")
include_directories ("src/prebuilt/sdk")
include_directories ("src/prebuilt/internal")

# do not use from prebuilt that was checked-in. This might be old. Use the one out of a Windows build
include_directories ("src/prebuilt/idl/public")
include_directories ("src/prebuilt/idl/internal")

# for PAL
include_directories ("src/pal/src")

# for ktlwf 
include_directories ("shared/ktlwf/ktlwf/inc")
include_directories ("shared/ktlwf/com/inc")

# for boost (bug fixed)
include_directories("${THIRD_PARTY_LIB_DIR}/Boost_1_61_0")

# for Cxx 
set (CXX_INC_PATH "${THIRD_PARTY_LIB_DIR}/Cxx/include/c++/v1")
if (BUILD_THIRD_PARTY)
    set (CXX_LIB_PATH "${THIRD_PARTY_LIB_DIR}/Cxx/lib")
else()
    set (CXX_LIB_PATH "${THIRD_PARTY_LIB_DIR}/Cxx")
endif()
include_directories ("${CXX_INC_PATH}")
link_directories("${CXX_LIB_PATH}")

# for libxml2
include_directories("/usr/include/libxml2")

# ktl libs
include_directories("./ktl/src/inc")
include_directories("./ktl/src/logger/inc")
include_directories("./ktl/src/linuxinc")
include_directories("./ktl/src/inc/linux-coroutine")

# for zip/unzip
include_directories("${THIRD_PARTY_LIB_DIR}/minizip")

# casablanca include directories
set(Casablanca_INCLUDE_DIR "${THIRD_PARTY_LIB_DIR}/casablanca_v_2/include")
set(WebSocket_INCLUDE_DIR "${THIRD_PARTY_LIB_DIR}/casablanca_v_2/libs/websocketpp")
set(Boost_INCLUDE_DIR "${THIRD_PARTY_LIB_DIR}/Boost_1_61_0")


set(Casablanca_INCLUDE_DIRS ${Casablanca_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR} ${WebSocket_INCLUDE_DIR})
include_directories(${Casablanca_INCLUDE_DIRS})

link_directories("${THIRD_PARTY_LIB_DIR}/casablanca_v_2/Binaries")
link_directories("${KTL_LIB_DIR}")

find_library(KmUnitUser NAMES KmUnitUser PATHS "${KTL_LIB_DIR}" NO_DEFAULT_PATH)
find_library(KtlLttng NAMES ktllttng PATHS "${KTL_LIB_DIR}" NO_DEFAULT_PATH)

link_directories("${THIRD_PARTY_LIB_DIR}/minizip")

include_directories(${THIRD_PARTY_LIB_DIR}/grpc/include)
link_directories("${THIRD_PARTY_LIB_DIR}/grpc")

include_directories(${THIRD_PARTY_LIB_DIR}/grpc/include)
link_directories("${THIRD_PARTY_LIB_DIR}/grpc")


# environmet variable
set (ENV{BOOST_TEST_LOG_LEVEL} "test_suite")

# boost packages for casablanca
# set boost paths for casablanca
set(BOOST_ROOT "${THIRD_PARTY_LIB_DIR}/Boost_1_61_0")
set(BOOST_INCLUDEDIR "${BOOST_ROOT}/boost")
# not needed?
#set(BOOST_INCLUDE_DIRS "${BOOST_ROOT}/boost")
#set (BOOST_TEST_PATH ${THIRD_PARTY_LIB_DIR}/Boost_1_61_0/lib)

set(BOOST_LIBRARY_DIRS "${BOOST_ROOT}/lib")

include_directories(${BOOST_ROOT}) # Some things include headers like boost/xxxx.hpp, so need root dir as include path as well
include_directories(${BOOST_INCLUDEDIR})
link_directories(${BOOST_LIBRARY_DIRS})

find_package(Boost REQUIRED COMPONENTS random chrono system thread regex filesystem)
find_library(BoostTest2 boost_unit_test_framework PATHS ${BOOST_LIBRARY_DIRS} NO_DEFAULT_PATH)



if (BUILD_THIRD_PARTY)
    find_library(Cxx c++ PATHS ${CXX_LIB_PATH} NO_DEFAULT_PATH)
    find_library(CxxABI c++abi PATHS ${CXX_LIB_PATH} NO_DEFAULT_PATH)
else()
    find_library(Cxx cxx PATHS ${CXX_LIB_PATH} NO_DEFAULT_PATH)
    find_library(CxxABI cxxabi PATHS ${CXX_LIB_PATH} NO_DEFAULT_PATH)
endif()
set (CMAKE_CXX_STANDARD_LIBRARIES "${Cxx} ${CxxABI}")

find_library(cpprest cpprest PATHS ${CASABLANCA_LIB_PATH})

set (BABELTRACE_LIB_PATH ${THIRD_PARTY_LIB_DIR}/babeltrace/lib)
find_library(Babeltrace babeltrace PATHS ${BABELTRACE_LIB_PATH} NO_DEFAULT_PATH)
find_library(BabeltraceCTF babeltrace-ctf PATHS ${BABELTRACE_LIB_PATH} NO_DEFAULT_PATH)

set (Babeltrace_INCLUDE_DIR "${THIRD_PARTY_LIB_DIR}/babeltrace/include")

include_directories(${Babeltrace_INCLUDE_DIR})

# casablanca libraries
set(Casablanca_LIBRARY cpprest)
set(Casablanca_LIBRARIES ${Casablanca_LIBRARY})

# Create a lib directory for output during config time
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/lib ${PROJECT_BINARY_DIR}/test)

set(JAVA_HOME ${EXTERNAL_DEPS_DIR}/Java.Linux.Libs/jdk CACHE STRING "JAVA_HOME")
set(GRADLE_HOME ${EXTERNAL_DEPS_DIR}/Java.Linux.Libs/gradle CACHE STRING "JAVA GRADLE_HOME")
set(ASM_LIBS ${EXTERNAL_DEPS_DIR}/Java.Linux.Libs/asm CACHE STRING "JAVA ASM_LIBS")

#define Library names and output properties
set (LIBRARY_OUTPUT_DIR ${PROJECT_BINARY_DIR}/lib CACHE STRING "Library Output directory")
set (TEST_OUTPUT_DIR ${PROJECT_BINARY_DIR}/test CACHE STRING "Test Output directory")

set (lib_KtlLoggerShimUDriver "KtlLoggerShimUDriver" CACHE STRING "KTL Logger UDriver Library")
set (lib_KtlLoggerShimUnify "KtlLoggerShimUnify" CACHE STRING "KTL Logger Unify Library")

set (CLUSTER_DEPLOYER_DIR ${PROJECT_BINARY_DIR}/ClusterDeployer CACHE STRING "Cluster Deployer tool directory")
set (FABRIC_DROP_DIR ${PROJECT_BINARY_DIR}/FabricDrop CACHE STRING "FabricDrop directory")
file(MAKE_DIRECTORY ${CLUSTER_DEPLOYER_DIR})

# Fabric Drop and packaging constants
set (Linux_Setup_Dir ${PROJECT_SOURCE_DIR}/../prod/linuxsetup CACHE STRING "Path to linuxsetup folder")
set (WinFab_Linux_Libs_Dir ${EXTERNAL_LIB_PATH}/ CACHE STRING "Path to WinFab.Linux.Libs")
set (Managed_Libs_Dir ${PROJECT_BINARY_DIR}/coreclr/ CACHE STRING "Path to local build managed components")
set (Managed_Publish_Libs_Dir ${Managed_Libs_Dir}publish/external/ CACHE STRING "Path to managed structured published bits")
set (WinFab_Babeltrace_Dir ${THIRD_PARTY_LIB_DIR}/babeltrace/ CACHE STRING "Path to build of modified and patched babeltrace lib")
set (WinFab_CoreCLR_Libs_Dir ${EXTERNAL_DEPS_DIR}/WinFab.CoreCLR.Libs CACHE STRING "Path to WinFab.CoreCLR.Libs")

file (STRINGS "linuxsetup/versions/linuxbuild.version" BUILD_VERSION)

set (Cluster_Version ${BUILD_VERSION} CACHE STRING "Current Cluster version" FORCE)

option (PACKAGE_LOCAL_BUILD_MANAGED "Option to package locally build managed binaries in deb file. If OFF, uses external/WinFab.CoreClr.Libs/*" OFF)
if (PACKAGE_LOCAL_BUILD_MANAGED)
   set (Package_Managed_Libs_Dir ${Managed_Publish_Libs_Dir} CACHE STRING "Package Managed binaries from local build.")
else()
   set (Package_Managed_Libs_Dir ${WinFab_CoreCLR_Libs_Dir} CACHE STRING "Package Managed binaries from winfab_coreclr_libs.")
endif()

file (STRINGS "linuxsetup/versions/linuxupgradebuild.version" UPGRADE_BUILD_VERSION)
set (Cluster_Version_UpgradeTest ${UPGRADE_BUILD_VERSION} CACHE STRING "New Cluster version for upgrade testing" FORCE)
file (STRINGS "linuxsetup/versions/sdkcommon.version" SDK_COMMON_VERSION)
set (SDKCommon_Version ${SDK_COMMON_VERSION} CACHE STRING "Current SDK Common version")

set (GuestBinary_Gen_Name generator-azuresfguest  CACHE STRING "Guest binary generator name")
set (Container_Gen_Name generator-azuresfcontainer  CACHE STRING "Container generator name")

if(LINUX_DISTRIBUTION STREQUAL "DEBIAN")
    set (Runtime_Deb_Folder ${FABRIC_DROP_DIR}/deb/servicefabric_${Cluster_Version} CACHE STRING "Runtime Debian package name" FORCE)
    set (Runtime_Deb_Folder_UpgradeTest ${FABRIC_DROP_DIR}/deb/servicefabric_${Cluster_Version_UpgradeTest} CACHE STRING "Runtime Debian package name for deb upgrade testing" FORCE)
    set (Runtime_Deb_Install_Path opt/microsoft/servicefabric CACHE STRING "Runtime Debian package install path")

    set (SFSdk_Path opt/microsoft/sdk CACHE STRING "Path to SDK root")
    set (Sdk_Common_Deb_Folder ${FABRIC_DROP_DIR}/deb/servicefabric_sdkcommon_${SDKCommon_Version} CACHE STRING "SDK Common Debian package name")
    set (Sdk_Common_Deb_Install_Path ${SFSdk_Path}/servicefabric/common CACHE STRING "SDK Common Debian package install path")

    set (Yeoman_Guest_Dest_Path /${Sdk_Common_Deb_Install_Path}/yeoman-azuresfguest/${GuestBinary_Gen_Name} CACHE STRING "Yeoman guest binary generator full path in destination")
    set (Yeoman_Container_Dest_Path /${Sdk_Common_Deb_Install_Path}/yeoman-azuresfcontainer/${Container_Gen_Name} CACHE STRING "Yeoman container generator full path in destination")
elseif(LINUX_DISTRIBUTION STREQUAL "REDHAT")
    set (Base_Rpm_Folder ${FABRIC_DROP_DIR}/rpm CACHE STRING "RPM parent folder containing RPM build resources")
    set (Runtime_Rpm_DropFolder ${FABRIC_DROP_DIR}/rpm/servicefabric_runtime CACHE STRING "Runtime RPM package name")
    set (Runtime_Rpm_Folder ${FABRIC_DROP_DIR}/rpm/servicefabric_${Cluster_Version} CACHE STRING "Runtime RPM package name" FORCE)
    set (Runtime_Rpm_Folder_UpgradeTest ${FABRIC_DROP_DIR}/rpm/servicefabric_${Cluster_Version_UpgradeTest} CACHE STRING "Runtime RPM package name for rpm upgrade testing" FORCE)
    set (Runtime_Rpm_Install_Path opt/microsoft/servicefabric CACHE STRING "Runtime RPM package install path")

    set (Rpmbuild_Runtime_Path ${Base_Rpm_Folder}/rpmbuild_runtime CACHE STRING "rpmbuild path for Runtime package")
    set (Rpmbuild_Runtime_UpgradeTest_Path ${Base_Rpm_Folder}/rpmbuild_runtime_upgradetest CACHE STRING "rpmbuild path for Runtime package used for Upgrade Test")
    set (Rpmbuild_SdkCommon_Path ${Base_Rpm_Folder}/rpmbuild_sdkcommon CACHE STRING "rpmbuild path for SDK Common package")

    set (SFSdk_Path opt/microsoft/sdk CACHE STRING "Path to SDK root")
    set (Sdk_Common_Rpm_Folder ${FABRIC_DROP_DIR}/rpm/servicefabric_sdkcommon_${SDKCommon_Version} CACHE STRING "SDK Common RPM package name")
    set (Sdk_Common_Rpm_Install_Path ${SFSdk_Path}/servicefabric/common CACHE STRING "SDK Common RPM package install path")

    set (Yeoman_Guest_Dest_Path /${Sdk_Common_Rpm_Install_Path}/yeoman-azuresfguest/${GuestBinary_Gen_Name} CACHE STRING "Yeoman guest binary generator full path in destination")
    set (Yeoman_Container_Dest_Path /${Sdk_Common_Rpm_Install_Path}/yeoman-azuresfcontainer/${Container_Gen_Name} CACHE STRING "Yeoman container generator full path in destination")
endif()

#Java dirs
set(JAVA_OUT_DIR ${PROJECT_BINARY_DIR}/java CACHE STRING "Java OUTPUT dir")
set(JAVA_SRC_DIR ${PROJECT_SOURCE_DIR}/src/java CACHE STRING "Java SOURCE dir")
set(JAVA_SAMPLE_SRC_DIR ${JAVA_SRC_DIR}/sample/ActorSamples CACHE STRING "Java sample Source dir")
set(JAVA_SAMPLE_LIB_DIR ${JAVA_SRC_DIR}/sample/lib CACHE STRING "Lib folder with required jars")

set (lib_Serialization "Serialization" CACHE STRING "Serialization library")

set (exe_DllExportTest "DllExportTest.exe" CACHE STRING "DllExportTest.Exe")

set (exe_CommonTest "Common.Test.exe" CACHE STRING "Common.Test.Exe")
set (exe_KtlLogCoreTest "KtlLogCoreTest" CACHE STRING "Ktl Logger Core Logger Test")
set (exe_KtlLogBvtUPassthroughTest "KtlLogBvtUPassthroughTest" CACHE STRING "Ktl Logger UPassthrough BVT Test")
set (exe_KtlLogStressUPassthroughTest "KtlLogStressUPassthroughTest" CACHE STRING "Ktl Logger UPassthrough Stress Test")
set (exe_KtlLogFeatureUPassthroughTest "KtlLogFeatureUPassthroughTest" CACHE STRING "Ktl Logger UPassthrough Feature Test")

set (lib_Common "Common" CACHE STRING "Common library")
set (lib_Client "Client" CACHE STRING "Client library")
set (exe_Client.Test "Client.Test.exe" CACHE STRING "Client.Test.Exe")
set (lib_ClientServerTransport "ClientServerTransport" CACHE STRING "ClientServerTransport library")
set (lib_Query "Query" CACHE STRING "Query library")
set (exe_Query.Test "Query.Test.exe" CACHE STRING "Query.Test.Exe")
set (lib_ApiDefinitions "ApiDefinitions" CACHE STRING "Api Definitions library")
set (lib_ApiWrappers "ApiWrappers" CACHE STRING "Api Wrappers library")
set (lib_ServiceGroup "ServiceGroup" CACHE STRING "ServiceGroup library")
set (exe_ServiceGroup.Test "ServiceGroup.Test.exe" CACHE STRING "ServiceGroup.Test.Exe")
set (lib_ManagementCommon "ManagementCommon" CACHE STRING "ManagementCommon library")
set (lib_ManagementSubsystem "ManagementSubsystem" CACHE STRING "ManagementSubsystem library")
set (lib_ManagementRepairManager "ManagementRepairManager" CACHE STRING "ManagementRepairManager library")
set (exe_ManagementRepairManager.Test "FabricRM.Test.exe" CACHE STRING "FabricRM.Test.exe")
set (lib_httptransport "httptransport" CACHE STRING "HttpTransport library")
set (lib_UpgradeOrchestrationService "UpgradeOrchestrationService" CACHE STRING "lib_UpgradeOrchestrationService library")
set (lib_DnsServiceConfig "DnsServiceConfig" CACHE STRING "DnsServiceConfig library")
set (lib_Communication "Communication" CACHE STRING "Communication library")
set (lib_KtlLoggerNode "KtlLoggerNode" CACHE STRING "KtlLoggerNode library")
set (lib_TcpServiceCommunication "TcpServiceCommunication" CACHE STRING "TcpServiceCommunication library")
set (exe_TcpServiceCommunication.Test "TcpServiceCommunication.Test.exe" CACHE STRING "TcpServiceCommunication.Test.Exe")
set (lib_Lease "Lease" CACHE STRING "Lease library")
set (exe_Lease.Test "Lease.Test.exe" CACHE STRING "Lease.Test.Exe")
set (lib_LeaseAgent "LeaseAgent" CACHE STRING "LeaseAgent library")
set (lib_FabricCommon "FabricCommon" CACHE STRING "FabricCommon library")
set (lib_FabricCommonStatic "FabricCommonStatic" CACHE STRING "FabricCommon Static Library")
set (lib_FabricNode "FabricNode" CACHE STRING "FabricNode library")
set (exe_FabricNode.Test "FabricNode.Test.exe" CACHE STRING "FabricNode.Test.Exe")

set (lib_ResourceManager "ResourceManager" CACHE STRING "ResourceManager library")

set (lib_CentralSecretService "CentralSecretService" CACHE STRING "CentralSecretService library")
set (exe_FabricCSS "FabricCSS" CACHE STRING "FabricCSS")
set (lib_LocalSecretService "LocalSecretService" CACHE STRING "LocalSecretService library")
set (exe_FabricLSS "FabricLSS" CACHE STRING "FabricLSS")

set (lib_ServiceModel "ServiceModel" CACHE STRING "Service Model library")
set (exe_ServiceModelTest "ServiceModel.Test.exe" CACHE STRING "ServiceModel Boost Test Exe")

set (lib_FabricResources "FabricResources" CACHE STRING "Fabric Resources library")
set (lib_FabricResourcesStatic "FabricResourcesStatic" CACHE STRING "Fabric Resources Static Library")

set (lib_Transport "Transport" CACHE STRING "Transport library")
set (exe_Transport.Test "Transport.Test.exe" CACHE STRING "Transport.Test.Exe")
set (exe_Transport.Functional.Test "Transport.Functional.Test.exe" CACHE STRING "Transport.Functional.Test.Exe")
set (exe_Transport.Perf.Test "Transport.Perf.Test.exe" CACHE STRING "Transport.Perf.Test.Exe")
set (exe_Transport.PerfTest.Client "Transport.PerfTest.Client.exe" CACHE STRING "Transport.PerfTest.Client.Exe")

set (lib_Failover "Failover" CACHE STRING "Failover library")
set (lib_FailoverCommon "FailoverCommon" CACHE STRING "Failover Common library")
set (lib_FailoverFM "FailoverFM" CACHE STRING "FailoverFM library")
set (lib_NetworkInventoryManager "NetworkInventoryManager" CACHE STRING "NetworkInventoryManager library")
set (exe_FailoverFM.Test "FM.Test.exe" CACHE STRING "FM.Test.exe")
set (exe_FailoverRA.Test "RA.Test.exe" CACHE STRING "RA.Test.exe")

set (lib_ReliabilityCommon "ReliabilityCommon" CACHE STRING "ReliabilityCommon library")
set (exe_ReliabilityCommonTest "Reliability.Common.Test.exe" CACHE STRING "Reliability.Common.Test.Exe")

set (lib_Federation "Federation" CACHE STRING "Federation library")
set (exe_Federation.Test "Federation.Test.exe" CACHE STRING "Federation.Test")

set (lib_TestCommon "TestCommon" CACHE STRING "TestCommon library")
set (lib_TestHooks "TestHooks" CACHE STRING "TestHooks library")
set (lib_FederationTestCommon "FederationTestCommon" CACHE STRING "FederationTestCommon library")
set (exe_FederationTest "FederationTest.exe" CACHE STRING "FederationTest.Exe")

set (lib_Store "Store" CACHE STRING "Store library")
set (lib_StoreRepairPolicy "Store.RepairPolicy" CACHE STRING "Store Repair policy library")
set (exe_StoreTest "Store.Test.exe" CACHE STRING "Store Boost Test Exe")

set (lib_FabricRuntime "FabricRuntime" CACHE STRING "FabricRuntime library")
set (lib_FabricServiceCommunication "FabricServiceCommunication" CACHE STRING "FabricServiceCommunication library")
set (lib_FabricTransport "FabricTransport" CACHE STRING "FabricTransport library")

set (lib_Replication "Replication" CACHE STRING "Replication library")
set (exe_Replication.Test "Replication.Test.exe" CACHE STRING "Replication.Test.Exe")
set (exe_ReplicationPerf.Test "ReplicationPerf.Test.exe" CACHE STRING "ReplicationPerf.Test.Exe")

set (lib_FileStoreService "FileStoreSvc" CACHE STRING "FileStoreService library")

set (lib_ImageModel "ImageModel" CACHE STRING "ImageModel library")
set (exe_ImageModel.Test "ImageModel.Test.exe" CACHE STRING "ImageModel.Test")

set (lib_FaultAnalysisService "FaultAnalysisService" CACHE STRING "FaultAnalysisService library")
set (lib_FabricFaultAnalysisService "FabricFaultAnalysisService" CACHE STRING "FabricFaultAnalysisService library")


set (lib_ImageStore "ImageStore" CACHE STRING "ImageStore library")
set (exe_ImageStore.Test "ImageStore.Test.exe" CACHE STRING "ImageStore.Test")


set (lib_ClusterManager "ClusterManager" CACHE STRING "ClusterManager library")
set (lib_HealthManager "HealthManager" CACHE STRING "HealthManager library")
set (lib_UpgradeService "UpgradeService" CACHE STRING "UpgradeService library")
set (lib_SystemServices "SystemServices" CACHE STRING "SystemServices library")

set (lib_Hosting2 "Hosting2" CACHE STRING "Hosting2 library")
set (exe_Hosting2Test "Hosting2.Test.exe" CACHE STRING "Hosting2 Boost Test Exe")
set (exe_Hosting2NetworkingTest "Hosting2.Networking.Test.exe" CACHE STRING "Hosting2 Boost Networking Test Exe")
set (lib_httpgateway "HttpGateway" CACHE STRING "HttpGateway library")

set (lib_Testability "Testability" CACHE STRING "Testability library")

set (exe_httpgatewayTest "HttpGateway.Test.exe" CACHE STRING "HttpGateway Boost Test Exe")

set (lib_EntreeService "EntreeService" CACHE STRING "EntreeService library")
set (lib_StoreService "StoreService" CACHE STRING "StoreService library")
set (exe_Naming.Test "Naming.Test.exe" CACHE STRING "Naming.Test.exe")
set (lib_FabricGateway "Gateway" CACHE STRING "Gateway library")

set (lib_LoadBalancing "LoadBalancing" CACHE STRING "LoadBalancing library")
set (lib_LoadBalancingCommon "LoadBalancingCommon" CACHE STRING "LoadBalancingCommon library")
set (exe_LoadBalancing.Test "LoadBalancing.Test.exe" CACHE STRING "LoadBalancing.Test")

set (lib_ResourceMonitor "ResourceMonitor" CACHE STRING "ResourceMonitor library")
set (lib_ResourceMonitorConfig "ResourceMonitorConfig" CACHE STRING "ResourceMonitorConfig library")
set (exe_ResourceMonitor "ResourceMonitorService" CACHE STRING "ResourceMonitorService")

set (lib_FabricUUID "FabricUUID" CACHE STRING "FabricUUID Library")
set (lib_InternalFabricUUID "InternalFabricUUID" CACHE STRING "InternalFabricUUID Library")

set (lib_FabricDockerClient "FabricDockerClient")

set (exe_Hosting2.TestProcess "Hosting2.TestProcess.exe" CACHE STRING "Hosting2.TestProcess Exe")
set (exe_WorkingFolderTestHost "WorkingFolderTestHost.exe" CACHE STRING "WorkingFolderTestHost.exe")

set (lib_FabricTestCommon "FabricTestTestCommon" CACHE STRING "FabricTestCommon library")
set (exe_FabricTest "FabricTest.exe" CACHE STRING "FabricTest.Exe")
set (exe_FabricTestHost "FabricTestHost.exe" CACHE STRING "FabricTestHost.exe")
set (exe_FabricTypeHost "FabricTypeHost.exe" CACHE STRING "FabricTypeHost.exe")

set (exe_LinuxRetailTests "LinuxRetailTests.exe" CACHE STRING "LinuxRetailTests.exe")

set (exe_FabricHost "FabricHost" CACHE STRING "FabricHost")
set (exe_Fabric "Fabric" CACHE STRING "Fabric")
set (exe_FabricGateway "FabricGateway" CACHE STRING "FabricGateway")
set (exe_FabricTypeHost "FabricTypeHost" CACHE STRING "FabricTypeHost")
set (exe_FabricRM "FabricRM" CACHE STRING "FabricRM")

set (lib_jCommon "jCommon" CACHE STRING "jCommon library")
set (lib_jFabricClient "jFabricClient" CACHE STRING "jFabricClient library")
set (lib_jFabricRuntime "jFabricRuntime" CACHE STRING "jFabricRumtime library")
set (lib_FabricClient "FabricClient" CACHE STRING "FabricClient library")

set (exe_FileStoreService "FileStoreService" CACHE STRING "FileStoreService exe")
set (lib_FabricImageStore "FabricImageStore" CACHE STRING "FabricImageStore library")
set (lib_FabricUpgradeOrchestrationService "FabricUpgradeOrchestrationService" CACHE STRING "FabricUpgradeOrchestrationService library")

set (exe_ReplicatedStoreViewer "ReplicatedStoreViewer" CACHE STRING "ReplicatedStoreViewer exe")

set (lib_KtlLoggerUser "KtlLoggerUser" CACHE STRING "KTL Logger (User) Library")
set (lib_KtlLoggerShimUnify "KtlLoggerShimUnify" CACHE STRING "KTL Logger Unify Library")

set (lib_AadWrapperServer "AadWrapperServer" CACHE STRING "AadWrapperServer library")
set (lib_TokenValidationService "TokenValidationService" CACHE STRING "TokenValidationService library")
set (lib_FabricTokenValidationService "FabricTokenValidationService" CACHE STRING "FabricTokenValidationService library")
set (lib_FabricContainerActivatorService "FabricContainerActivatorService" CACHE STRING "FabricContainerActivatorService library")

set (lib_InfrastructureService "InfrastructureService" CACHE STRING "InfrastructureService library")
set (lib_FabricInfrastructureService "FabricInfrastructureService" CACHE STRING "FabricInfrastructureService library")

set (lib_GatewayResourceManager "GatewayResourceManager" CACHE STRING "GatewayResourceManager library")
set (lib_FabricGatewayResourceManager "FabricGatewayResourceManager" CACHE STRING "FabricGatewayResourceManager library")

set (lib_KComAdapter "KComAdapter" CACHE STRING "KComAdapter library")
set (exe_KComAdapter_Test "KComAdapter.Test" CACHE STRING "KComAdapter.Test Exe")

set (lib_data_testcommon "data.testcommon" CACHE STRING "data.testcommon library")

set (lib_LogicalLog "LogicalLog" CACHE STRING "LogicalLog library")
set (exe_LogicalLog "LogicalLog.Test" CACHE STRING "LogicalLog library")

set (lib_TStore "TStore" CACHE STRING "TStore library")
set (exe_TStore_Test "TStore.Test.exe" CACHE STRING "TStore.Test Exe")
set (lib_ReliableConcurrentQueue "ReliableConcurrentQueue" CACHE STRING "ReliableConcurrentQueue library")
set (exe_ReliableConcurrentQueue_Test "ReliableConcurrentQueue.Test.exe" CACHE STRING "ReliableConcurrentQueue.Test Exe")

set (lib_txnreplicator_common "txnreplicator.common" CACHE STRING "TransactionalReplicator Common library")
set (lib_txnreplicator_testcommon "txnreplicator.testcommon" CACHE STRING "lib_TransactionalReplicator TestCommon library")
set (lib_TransactionalReplicator "TransactionalReplicator" CACHE STRING "TransactionalReplicator library")
set (lib_txnreplicator_testcommon "txnreplicator.testcommon" CACHE STRING "txnreplicator.testcommon library")
set (exe_txnreplicator_test "txnreplicator.test.exe" CACHE STRING "txnreplicator.test Exe")

set (lib_StatefulServiceBase "StatefulServiceBase" CACHE STRING "Stateful Service Base")
set (exe_nightwatch "NightWatchTXRService.exe" CACHE STRING "nightwatch.test Exe")
set (exe_nightwatch_volatile "NightWatchVolatileService.exe" CACHE STRING "nightwatch.volatile.test Exe")

set (lib_TestableService "TestableService" CACHE STRING "Testable Service")
set (exe_datalonghaul "DataLonghaul.exe" CACHE STRING "datalonghaul.test Exe")

set (lib_LogRecordLib "LogRecordLib" CACHE STRING "LogRecordLib library")
set (lib_LoggingReplicator "LoggingReplicator" CACHE STRING "LoggingReplicator library")
set (exe_loggingreplicator_test "loggingreplicator.test.exe" CACHE STRING "loggingreplicator.test Exe")
set (exe_logicallog_test "logicallog.test.exe" CACHE STRING "logicallog.test Exe")

set (lib_statemanager "statemanager" CACHE STRING "statemanager library")
set (exe_statemanager_test "statemanager.test.exe" CACHE STRING "statemanager.test Exe")

set (exe_statemanager_perftest "statemanager.perftest.exe" CACHE STRING "statemanager.perftest Exe")

set (lib_data_utilities "data.utilities" CACHE STRING "data.utilities library")
set (exe_data_utilities_test "data.utilities.test.exe" CACHE STRING "data.utilities.test Exe")

set (lib_DnsService "DnsService" CACHE STRING "DnsService")
set (exe_FabricDnsService "FabricDnsService" CACHE STRING "FabricDnsService")
set (exe_DnsServiceTest "DnsService.Test.exe" CACHE STRING "DnsService.Test.Exe")

set (lib_KPhysicalLog "KPhysicalLog" CACHE STRING "KPhysicalLog adapter")
set (lib_ReliableCollectionRuntimeImpl "ReliableCollectionRuntimeImpl" CACHE STRING "ReliableCollectionRuntime Implementation Library")
set (lib_ReliableCollectionRuntime "ReliableCollectionRuntime" CACHE STRING "ReliableCollectionRuntime library")

set (lib_BackupRestoreService "BackupRestoreService" CACHE STRING "BackupRestoreService library")

set (lib_BlockStore "SFBDCtlLib" CACHE STRING "SFBDCtlLib")
set (exe_BlockStore "SFBlockStoreService" CACHE STRING "SFBlockStoreService")
set (exe_BlockStoreCtl "SFBlockStoreCtl" CACHE STRING "SFBlockStoreCtl")
set (exe_BlkDeviceLinuxKernelValidation_Test "BlkDeviceLinuxKernelValidation" CACHE STRING "BlkDeviceLinuxKernelValidation")

set (lib_ReliableCollectionService "ReliableCollectionService" CACHE STRING "ReliableCollectionService")
set (lib_LttngReader "LttngReader" CACHE STRING "LttngReader library")

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;MinSizeRel" CACHE STRING "" FORCE)

add_custom_target(postbuild
    COMMAND ${CMAKE_MAKE_PROGRAM} install
    COMMAND ${CMAKE_MAKE_PROGRAM} fabricdrop
    COMMAND ${CMAKE_MAKE_PROGRAM} symsplit
    )

add_subdirectory (src)
add_subdirectory (shared)
add_subdirectory (test)
add_subdirectory (tools)
add_subdirectory (linuxsetup)

endif()
