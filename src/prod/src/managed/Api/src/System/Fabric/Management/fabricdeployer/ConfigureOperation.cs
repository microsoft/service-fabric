// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using System.Diagnostics;
    using System.Fabric.Common;
    using System.Fabric.Management.FabricDeployer;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Strings;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using System.Globalization;
    using Microsoft.Win32;
    using Collections.Generic;

    internal class ConfigureOperation : DeploymentOperation
    {        
        protected override void OnExecuteOperation(DeploymentParameters parameters, ClusterManifestType clusterManifest, Infrastructure infrastructure)
        {
#if !DotNetCoreClrLinux
            if (FabricDeployerServiceController.IsRunning(parameters.MachineName))
            {
                string message = string.Format(StringResources.Error_FabricDeployer_FabricHostStillRunning_Formatted, parameters.MachineName);
                DeployerTrace.WriteError(message);
                throw new InvalidOperationException(message);
            }
#endif
            DeployerTrace.WriteInfo("Creating FabricDataRoot {0}, if it doesn't exist on machine {1}", parameters.FabricDataRoot, parameters.MachineName);
            Helpers.CreateDirectoryIfNotExist(parameters.FabricDataRoot, parameters.MachineName);
            DeployerTrace.WriteInfo("Creating FabricLogRoot {0}, if it doesn't exist on machine {1}", parameters.FabricLogRoot, parameters.MachineName);
            Helpers.CreateDirectoryIfNotExist(Path.Combine(parameters.FabricLogRoot, Constants.TracesFolderName), parameters.MachineName);

            List <LogicalDirectoryType[]> logicalDirectorysSetForThisMachine = new List<LogicalDirectoryType[]>();

            // For single machine scale min, fabric deployer only runs once for all nodes. It may miss the nodeType that has the logicalApplicationDirectory section.
            // So here, we need to extract all the logicalApplicationDirectories sections from clusterManifest.
            if ((clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsServer &&
                ((ClusterManifestTypeInfrastructureWindowsServer)clusterManifest.Infrastructure.Item).IsScaleMin)
                || (clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureLinux &&
                ((ClusterManifestTypeInfrastructureLinux)clusterManifest.Infrastructure.Item).IsScaleMin))
            {
                FabricNodeType[] nodeList;
                nodeList = clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsServer ?
                    ((ClusterManifestTypeInfrastructureWindowsServer)clusterManifest.Infrastructure.Item).NodeList :
                    ((ClusterManifestTypeInfrastructureLinux)clusterManifest.Infrastructure.Item).NodeList;

                foreach (var node in nodeList)
                {
                    var logicalDirectories = clusterManifest.NodeTypes.Where(nodeType => nodeType.Name == node.NodeTypeRef).First().LogicalDirectories;
                    if (logicalDirectories != null)
                    {
                        logicalDirectorysSetForThisMachine.Add(logicalDirectories);
                    }
                }
            }
            else if (clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructurePaaS
                || clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsServer
                || clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureLinux) // PaaS doesn't support Scale Min.
            {
                if (!string.IsNullOrEmpty(parameters.MachineName)) //For cab deployment
                {
                    var logicalDirectories = clusterManifest.NodeTypes.Where(nt => nt.Name == Utility.GetNodeTypeFromMachineName(clusterManifest, infrastructure.GetInfrastructureManifest(), parameters.MachineName)).First().LogicalDirectories;
                    if (logicalDirectories != null)
                    {
                        logicalDirectorysSetForThisMachine.Add(logicalDirectories);
                    }
                }
                else //For msi deployment
                {
                    foreach (var infrastructureNode in infrastructure.InfrastructureNodes)
                    {
                        bool isNodeForThisMachine = NetworkApiHelper.IsNodeForThisMachine(infrastructureNode);
                        if (isNodeForThisMachine)
                        {
                            var logicalDirectories = clusterManifest.NodeTypes.Where(nt => nt.Name == infrastructureNode.NodeTypeRef).First().LogicalDirectories;
                            if (logicalDirectories != null)
                            {
                                logicalDirectorysSetForThisMachine.Add(logicalDirectories);
                            }
                        }
                    }
                }
            }

            if (logicalDirectorysSetForThisMachine.Any())
            {
                CreateLogicalDirectoriesForAllTheNodesOnThisMachineAndEnableRegKey(parameters.MachineName, logicalDirectorysSetForThisMachine);
            }

            FabricValidatorWrapper fabricValidator = new FabricValidatorWrapper(parameters, clusterManifest, infrastructure);
            fabricValidator.ValidateAndEnsureDefaultImageStore();

            if (!string.IsNullOrEmpty(parameters.BootstrapMSIPath)) // Mandatory for Standalone path
            {
                CopyBaselinePackageIfPathExists(parameters.BootstrapMSIPath, parameters.FabricDataRoot, parameters.DeploymentPackageType, parameters.MachineName);
            }

            Utility.SetFabricRegistrySettings(parameters);
            ConfigureFromManifest(parameters, clusterManifest, infrastructure);

            string destinationCabPath = Path.Combine(parameters.FabricDataRoot, Constants.FileNames.BaselineCab);
            WriteTargetInformationFile(
                parameters.ClusterManifestLocation, 
                parameters.InfrastructureManifestLocation, 
                parameters.FabricDataRoot, 
                parameters.MachineName,
                destinationCabPath,
                parameters.DeploymentPackageType,
                parameters.BootstrapMSIPath);

            WriteFabricHostSettingsFile(parameters.FabricDataRoot, new SettingsType(), parameters.MachineName);

#if !DotNetCoreClrLinux
            if (!string.IsNullOrEmpty(parameters.FabricPackageRoot))
            {
                FabricDeployerServiceController.InstallFabricInstallerService(parameters.FabricPackageRoot, parameters.MachineName);
            }
#endif

            if (!string.IsNullOrEmpty(parameters.JsonClusterConfigLocation))
            {
                string destinationConfigPath = Helpers.GetRemotePath(
                    Path.Combine(parameters.FabricDataRoot, Constants.FileNames.BaselineJsonClusterConfig), parameters.MachineName);
                File.Copy(parameters.JsonClusterConfigLocation, destinationConfigPath, true);
            }
        }

        internal void WriteTargetInformationFile(string clusterManifestLocation, string infrastructureManifestLocation, string fabricDataRoot, string machineName, string packageLocation, FabricPackageType fabricPackageType, string sourcePackageLocation)
        {
            ReleaseAssert.AssertIfNull(fabricDataRoot, "DataRoot Should not be null after configuration");
            TargetInformationType targetInformation = new TargetInformationType
            {
                TargetInstallation = new WindowsFabricDeploymentInformation
                {
                    TargetVersion = Utility.GetCurrentCodeVersion(sourcePackageLocation),
                    ClusterManifestLocation = clusterManifestLocation,
                    UpgradeEntryPointExe = "FabricSetup.exe",
                    UpgradeEntryPointExeParameters = "/operation:Install"
                }
            };

            if (fabricPackageType == FabricPackageType.XCopyPackage)
            {
                targetInformation.TargetInstallation.MSILocation = packageLocation;
            }

            if (!string.IsNullOrEmpty(infrastructureManifestLocation))
            {
                targetInformation.TargetInstallation.InfrastructureManifestLocation = infrastructureManifestLocation;
            }

            string targetInformationFileName = Path.Combine(fabricDataRoot, Constants.FileNames.TargetInformation);

            DeployerTrace.WriteInfo("TargetInformationFileName is {0}", targetInformationFileName);
            if (!string.IsNullOrEmpty(machineName) && !machineName.Equals(Helpers.GetMachine()))
            {
                targetInformation.TargetInstallation.ClusterManifestLocation = Path.Combine(fabricDataRoot, "clusterManifest.xml");
                targetInformationFileName = Helpers.GetRemotePath(targetInformationFileName, machineName);
            }

            XmlHelper.WriteXmlExclusive<TargetInformationType>(targetInformationFileName, targetInformation);
            DeployerTrace.WriteInfo("Target information file {0} written on machine: {1}", targetInformationFileName, machineName);
        }

        private void CreateLogicalDirectoriesForAllTheNodesOnThisMachineAndEnableRegKey(string machineName, List<LogicalDirectoryType[]> logicalDirectoriesSet)
        {
#if !DotNetCoreClrLinux
            try
            {
                RegistryKey hklm = Utility.GetHklm(machineName);

                //Enable all the symlink in registry key.
                using (RegistryKey key = hklm.OpenSubKey(@"System\CurrentControlSet\Control\FileSystem", true))
                {
                    using (RegistryKey regKey = key ?? hklm.CreateSubKey(@"System\CurrentControlSet\Control\FileSystem"))
                    {
                        regKey.SetValue("SymlinkLocalToLocalEvaluation", 1, RegistryValueKind.DWord);
                        regKey.SetValue("SymlinkLocalToRemoteEvaluation", 1, RegistryValueKind.DWord);
                        regKey.SetValue("SymlinkRemoteToLocalEvaluation", 1, RegistryValueKind.DWord);
                        regKey.SetValue("SymlinkRemoteToRemoteEvaluation", 1, RegistryValueKind.DWord);
                    }
                }
            }
            catch (Exception e)
            {
                DeployerTrace.WriteError(StringResources.Error_UnableToSetSymlinkRegistryKey, machineName, e);
                return;
            }
#endif

            foreach (LogicalDirectoryType[] logicalDirectories in logicalDirectoriesSet)
            {
                foreach (LogicalDirectoryType dir in logicalDirectories)
                {
                    try
                    {
#if !DotNetCoreClrLinux
                        Helpers.CreateDirectoryIfNotExist(dir.MappedTo, machineName);
#else
                        Helpers.CreateDirectoryIfNotExist(dir.MappedTo, "");
#endif

                    }
                    catch (Exception e)
                    {
                        string message = string.Format(StringResources.Error_FailedToCreateLogicalDirectory, dir.MappedTo, machineName, e);
                        DeployerTrace.WriteError(message);
                        throw new ArgumentException(message);
                    }
                }
            }
        }


        private static bool ArePathesEqual(string a, string b)
        {
            string sa = Path.GetFullPath(a);
            string sb = Path.GetFullPath(b);
        
            return string.Compare(sa, sb, StringComparison.OrdinalIgnoreCase) == 0;
        }
        
        private void ConfigureFromManifest(DeploymentParameters parameters, ClusterManifestType clusterManifest, Infrastructure infrastructure)
        {
            SetupSettings settings = new SetupSettings(clusterManifest);
#if !DotNetCoreClrLinux
            if (!string.IsNullOrEmpty(settings.ServiceRunAsAccountName))
            {
                FabricDeployerServiceController.SetServiceCredentials(settings.ServiceRunAsAccountName, settings.ServiceRunAsPassword, parameters.MachineName);
                DeployerTrace.WriteInfo("Set Service Fabric Host Service to run as {0}", settings.ServiceRunAsAccountName);
            }
#endif

            string clusterManifestTargetLocation = Helpers.GetRemotePath(Path.Combine(parameters.FabricDataRoot, "clusterManifest.xml"), parameters.MachineName);
            DeployerTrace.WriteInfo("Copying ClusterManifest to {0}", clusterManifestTargetLocation);
            
            if (!ArePathesEqual(parameters.ClusterManifestLocation, clusterManifestTargetLocation))
            {
                File.Copy(parameters.ClusterManifestLocation, clusterManifestTargetLocation, true);
            }

#if !DotNetCoreClrLinux
            string serviceStartupType = "DelayedAutoStart";
            if (!string.IsNullOrEmpty(parameters.ServiceStartupType))
            {
                FabricDeployerServiceController.SetStartupType(parameters.ServiceStartupType);
                serviceStartupType = parameters.ServiceStartupType;
            }
            else if (!string.IsNullOrEmpty(settings.ServiceStartupType))
            {
                FabricDeployerServiceController.SetStartupType(settings.ServiceStartupType);
                serviceStartupType = settings.ServiceStartupType;
            }
            else
            {
                if (infrastructure is AzureInfrastructure)
                {
                    FabricDeployerServiceController.SetServiceStartupType(FabricDeployerServiceController.ServiceStartupType.Manual, parameters.MachineName);
                    serviceStartupType = FabricDeployerServiceController.ServiceStartupType.Manual.ToString();
                }
                else
                {
                    FabricDeployerServiceController.SetStartupTypeDelayedAuto(parameters.MachineName);
                }

            }
            DeployerTrace.WriteInfo("Set Service Fabric Host Service to start up type to {0}", serviceStartupType);
#endif
        }

        private void CopyBaselinePackageIfPathExists(string bootstrapPackagePath, string fabricDataRoot, FabricPackageType fabricPackageType, string machineName)
        {
            string targetPath = Helpers.GetRemotePath(fabricDataRoot, machineName);
            DeployerTrace.WriteInfo("Copying {0} from {1} to {2}", fabricPackageType == FabricPackageType.MSI ? "BootstrapMSI" : "Standalone CAB", bootstrapPackagePath, fabricDataRoot);
            if (fabricPackageType == FabricPackageType.XCopyPackage)
            {
                targetPath = Path.Combine(targetPath, Constants.FileNames.BaselineCab);
            }
            else
            {
                targetPath = Path.Combine(targetPath, Path.GetFileName(bootstrapPackagePath));
            }

            using (FileStream sourceStream = File.Open(bootstrapPackagePath, FileMode.Open, FileAccess.Read, FileShare.Read))
            {
                using (FileStream destStream = File.Create(targetPath))
                {
                    sourceStream.CopyTo(destStream);
                }
            }
            if (!File.Exists(targetPath))
            {
                string message = string.Format(StringResources.Error_SFErrorCreatingCab, targetPath, machineName);
                DeployerTrace.WriteError(message);
                throw new FileNotFoundException(message);
            }
            string adminConfigPath = Environment.GetEnvironmentVariable(Constants.FabricTestAdminConfigPath);
            if (!string.IsNullOrEmpty(adminConfigPath))
            {
                CopyNewAdminConfigToFabricDataRoot(fabricDataRoot, adminConfigPath, machineName);
            }
        }

        private void CopyNewAdminConfigToFabricDataRoot(string fabricDataRoot, string newAdminConfigPath, string machineName)
        {
            string targetPath = Helpers.GetRemotePath(fabricDataRoot, machineName);
            DeployerTrace.WriteInfo("Copying New Admin Config from {0} to {1}", newAdminConfigPath, fabricDataRoot);
            targetPath = Path.Combine(targetPath, Constants.FileNames.ClusterSettings);

            using (FileStream sourceStream = File.Open(newAdminConfigPath, FileMode.Open, FileAccess.Read, FileShare.Read))
            {
                using (FileStream destStream = File.Create(targetPath))
                {
                    sourceStream.CopyTo(destStream);
                }
            }

            if (!File.Exists(targetPath))
            {
                string message = string.Format(StringResources.Error_SFErrorCreatingCab, targetPath, machineName);
                DeployerTrace.WriteError(message);
                throw new FileNotFoundException(message);
            }
        }
    }
}