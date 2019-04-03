SET (CMAKE_VERBOSE_MAKEFILE on)

if(EXISTS "/opt/rh/rh-dotnet20/root/usr/lib64/dotnet/")
  set(SF_DOTNET_TYPE RHDOTNET20)
elseif(EXISTS "/opt/rh/rh-dotnet21/root/usr/lib64/dotnet/")
  set(SF_DOTNET_TYPE RHDOTNET21)
elseif(EXISTS "/usr/share/dotnet/")
  set(SF_DOTNET_TYPE STANDARD)
endif()
message(STATUS "DOTNET TYPE: ${SF_DOTNET_TYPE}")

# Dotnet specific commands
SET (CORECLR_CLI_COMMAND dotnet)
SET (CORECLR_RESTORE_COMMAND restore)
SET (CORECLR_BUILD_COMMAND msbuild)
SET (EXTRA_FLAGS "/warnaserror")

# Build specific variables
# Add these variables in dotnet restore command in src/properties/ServiceFabric.CoreClr.targets
LIST (APPEND ManagedLinuxBuildEnv "SRCROOT=${PROJECT_SOURCE_DIR}/../")
LIST (APPEND ManagedLinuxBuildEnv "BINROOT=${PROJECT_BINARY_DIR}/")
LIST (APPEND ManagedLinuxBuildEnv "BUILD_VERSION=${BUILD_VERSION}")
LIST (APPEND ManagedLinuxBuildEnv "SF_DOTNET_TYPE=${SF_DOTNET_TYPE}")
LIST (APPEND ManagedLinuxBuildEnv "EnvironmentConfig=${PROJECT_SOURCE_DIR}/../properties/linux/environment.props")

# Project specific variables
LIST (APPEND ManagedLinuxBuildEnv "PkgSF_AspNetCore_Internal=${MANAGED_DEPS_DIR}/SF.AspNetCore.Internal")
LIST (APPEND ManagedLinuxBuildEnv "PkgSF_ActorsServices_Internal=${MANAGED_DEPS_DIR}/SF.ActorsServices.Internal")

# netcoreapp2.0 projects
LIST (APPEND CORECLR_PROJECTS_LIST "FabricFAS/CoreCLRDlls/exe/FabricFAS.csproj")
LIST (APPEND CORECLR_PROJECTS_LIST "DCA/product/host/CoreClrDll/FabricDCA/FabricDCA.csproj")
LIST (APPEND CORECLR_PROJECTS_LIST "FabricCAS/CoreCLR/FabricCAS.csproj")
LIST (APPEND CORECLR_PROJECTS_LIST "TokenValidationService/CoreCLR/FabricTVS/FabricTVS.csproj")
LIST (APPEND CORECLR_PROJECTS_LIST "FabricUS/CoreCLR/FabricUS/FabricUS.csproj")
LIST (APPEND CORECLR_PROJECTS_LIST "ImageBuilderExe/CoreCLR/ImageBuilder/ImageBuilder.csproj")
LIST (APPEND CORECLR_PROJECTS_LIST "azurecliproxy/CoreCLR/azurecliproxy/azurecliproxy.csproj")
LIST (APPEND CORECLR_PROJECTS_LIST "../../Setup/FabricDeployer/CoreCLR/FabricDeployer/FabricDeployer.csproj")
LIST (APPEND CORECLR_PROJECTS_LIST "../BlockStore/Service/VolumeDriver/SetupEntryPoint/Linux/BSDockerVolumePluginSetupL.csproj")
LIST (APPEND CORECLR_PROJECTS_LIST "../BlockStore/Service/VolumeDriver/Service/Linux/BSDockerVolumePluginL.csproj")
LIST (APPEND CORECLR_PROJECTS_LIST "Container/VolumePlugin/AzureFiles/SetupEntryPoint/Linux/AzureFilesVolumePluginSetupL.csproj")
LIST (APPEND CORECLR_PROJECTS_LIST "Container/VolumePlugin/AzureFiles/Service/Linux/AzureFilesVolumePluginL.csproj")
LIST (APPEND CORECLR_PROJECTS_LIST "FabricGRM/CoreCLR/FabricGRM/FabricGRM.csproj")

# /p with multiple properties isn't working for linux msbuild, hence passing them as separate params
foreach(property IN LISTS ManagedLinuxBuildEnv)
  LIST (APPEND PropertyList "/p:${property}")
endforeach(property)

foreach(coreclr_project_fullpath IN LISTS CORECLR_PROJECTS_LIST)
  get_filename_component(coreclr_project_abspath ${coreclr_project_fullpath} ABSOLUTE)
  get_filename_component(coreclr_project_filename ${coreclr_project_fullpath} NAME)
  get_filename_component(coreclr_project_dir ${coreclr_project_fullpath} DIRECTORY)

  # Use /v:d for more detailed logging & /bl for binary logging.
  LIST(APPEND CORECLR_PROJECTS_BUILD_COMMAND_LIST COMMAND ${CMAKE_COMMAND} -E env NugetMachineInstallRoot=${MANAGED_DEPS_DIR} ${CORECLR_CLI_COMMAND} ${CORECLR_BUILD_COMMAND} ${coreclr_project_abspath} ${PropertyList} ${EXTRA_FLAGS})
endforeach(coreclr_project_fullpath)

add_custom_target(
  ManagedCoreClrBuild
  ${CORECLR_PROJECTS_BUILD_COMMAND_LIST}
  COMMENT "Building CoreClr projects"
  VERBATIM
)
