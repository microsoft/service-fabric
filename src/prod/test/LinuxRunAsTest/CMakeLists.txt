include_directories("./")

add_compile_options(-rdynamic)

set (exe_LinuxRunAsTest "LinuxRunAsTest.exe" CACHE STRING "LinuxRunAsTest exe")

add_executable(${exe_LinuxRunAsTest}
  ./FabricAsyncOperationCallback.cpp
  ./FabricRuntimeHost.cpp
  ./FabricUnknownBase.cpp
  ./MyStatelessServiceInstance.cpp
  ./FabricCompletedAsyncOperationContext.cpp
  ./FabricString.cpp
  ./MyStatelessServiceFactory.cpp
)

set_target_properties(${exe_LinuxRunAsTest} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})
    
set_target_properties(${exe_LinuxRunAsTest} PROPERTIES 
    LINK_FLAGS "-L${LIBRARY_OUTPUT_DIR} -L${BOOST_LIBRARY_DIRS} -lFabricClient -lFabricRuntime -lFabricCommon -lFabricResources")

target_link_libraries( ${exe_LinuxRunAsTest}
   ${lib_Common}
   ${lib_Transport}
   ssl
   crypto
   ssh2
   dl
   FabricUUID
   minizip
   z
   pthread
   ${Cxx}
   ${CxxABI}
)

add_dependencies(${exe_LinuxRunAsTest}
  ${lib_FabricClient}
  ${lib_FabricCommon}
  ${lib_FabricRuntime}
)

install(
    FILES ./LinuxRunAsTestAppDeployer.sh
    DESTINATION ${TEST_OUTPUT_DIR}
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ GROUP_EXECUTE WORLD_EXECUTE
)

set(LinuxRunAsTestAppDir ${TEST_OUTPUT_DIR}/LinuxRunAsTestApplication)
install(CODE "FILE(MAKE_DIRECTORY ${LinuxRunAsTestAppDir})")
install(
    FILES ./ApplicationManifest.xml
    DESTINATION ${LinuxRunAsTestAppDir}
)

set(LinuxRunAsAcctDir ${LinuxRunAsTestAppDir}/RunAsAcctServicePackage)
install(CODE "FILE(MAKE_DIRECTORY ${LinuxRunAsAcctDir})")
install(CODE "FILE(MAKE_DIRECTORY ${LinuxRunAsAcctDir}/Code)")
install(CODE "FILE(MAKE_DIRECTORY ${LinuxRunAsAcctDir}/Data)")
install(CODE "FILE(MAKE_DIRECTORY ${LinuxRunAsAcctDir}/Config)")
install(
    FILES ./ServiceManifest.RunAsAcct.xml
    DESTINATION ${LinuxRunAsAcctDir}
    RENAME ServiceManifest.xml
)
install(
    FILES ./LinuxRunAsAcct.sh
    DESTINATION ${LinuxRunAsAcctDir}/Code/
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ GROUP_EXECUTE WORLD_EXECUTE
)
install(
    FILES ${TEST_OUTPUT_DIR}/${exe_LinuxRunAsTest}
    DESTINATION ${LinuxRunAsAcctDir}/Code/
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ GROUP_EXECUTE WORLD_EXECUTE
) 
install(
    FILES ./_readme.txt
    DESTINATION ${LinuxRunAsAcctDir}/Data/
)
install(
    FILES ./_readme.txt
    DESTINATION ${LinuxRunAsAcctDir}/Config/
)

set(LinuxRunAsRootDir ${LinuxRunAsTestAppDir}/RunAsRootServicePackage)
install(CODE "FILE(MAKE_DIRECTORY ${LinuxRunAsRootDir})")
install(CODE "FILE(MAKE_DIRECTORY ${LinuxRunAsRootDir}/Code)")
install(CODE "FILE(MAKE_DIRECTORY ${LinuxRunAsRootDir}/Data)")
install(CODE "FILE(MAKE_DIRECTORY ${LinuxRunAsRootDir}/Config)")
install(
    FILES ./ServiceManifest.RunAsRoot.xml
    DESTINATION ${LinuxRunAsRootDir}
    RENAME ServiceManifest.xml
)
install(
    FILES ./LinuxRunAsRoot.sh
    DESTINATION ${LinuxRunAsRootDir}/Code/
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ GROUP_EXECUTE WORLD_EXECUTE
)
install(
    FILES ${TEST_OUTPUT_DIR}/${exe_LinuxRunAsTest}
    DESTINATION ${LinuxRunAsRootDir}/Code/
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ GROUP_EXECUTE WORLD_EXECUTE
)
install(
    FILES ./_readme.txt
    DESTINATION ${LinuxRunAsRootDir}/Data/
)
install(
    FILES ./_readme.txt
    DESTINATION ${LinuxRunAsRootDir}/Config/
)
