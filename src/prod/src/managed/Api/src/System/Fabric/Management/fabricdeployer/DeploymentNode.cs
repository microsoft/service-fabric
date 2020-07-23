// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.Common;
    using System.Fabric.Management.FabricDeployer;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.ImageStore;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Security;
    using System.Text;
    using System.Xml;

    /// <summary>
    /// This class represents the data structure for a single node.
    /// </summary>
    internal class DeploymentNode
    {
        #region Private Fields
        private NodeSettings nodeSettings;

        private ClusterSettings clusterSettings;
        
        private List<SettingsTypeSection> fabricHostSections;

        private string[] servicesToBeDeployed;

        private string[] servicesToBeEnabled;

        private Infrastructure infrastructure;

        private ClusterManifestType clusterManifest;
        #endregion

        #region Public Constructors
        public DeploymentNode(
            NodeSettings nodeSettings,
            ClusterSettings clusterSettings,
            Infrastructure infrastructure,
            string[] servicesToBeDeployed,
            string[] servicesToBeEnabled,
            ClusterManifestType clusterManifest)
        {
            this.nodeSettings = nodeSettings;
            this.clusterSettings = clusterSettings;
            this.infrastructure = infrastructure;
            this.servicesToBeDeployed = servicesToBeDeployed;
            this.servicesToBeEnabled = servicesToBeEnabled;
            this.clusterManifest = clusterManifest;
            this.fabricHostSections = new List<SettingsTypeSection>();
        }
        #endregion

        #region Public Functions
        public void CreateDeployment()
        {
            this.GenerateCodeDeployment();
            this.GenerateConfigDeployment();
            this.GenerateDataDeployment();
            this.GenerateWorkFolders();         
            RolloutVersion rolloutVersion = RolloutVersion.CreateRolloutVersion();
            this.DeployClusterManifest();
            this.DeploySeedInfoFile();
            this.GenerateAndDeployUnreliableTransportSettings();
            this.GenerateAndDeployFabricPackage(rolloutVersion.ToString());
        }

        public void UpgradeDeployment()
        {
            string currentPackagePath = this.nodeSettings.DeploymentFoldersInfo.CurrentFabricPackageFile;
            if (!File.Exists(currentPackagePath))
            {
                DeployerTrace.WriteError("Current package file {0} not found. Upgrade can't proceed", currentPackagePath);
                throw new InvalidDeploymentParameterException(StringResources.Error_FabricDeployer_CurrentPackageFileNotFound_Formatted);
            }

            this.GenerateCodeDeployment();
            this.GenerateConfigDeployment();
            this.GenerateDataDeployment();
            ServicePackageType currentServicePackage = XmlHelper.ReadXml<ServicePackageType>(currentPackagePath);

            RolloutVersion updatedRolloutVersion = RolloutVersion.CreateRolloutVersion(currentServicePackage.RolloutVersion).NextMinorRolloutVersion();
            DeployerTrace.WriteNoise("Creating Service Package RolloutVersion = {0}", updatedRolloutVersion);
            this.DeployClusterManifest();
            this.DeploySeedInfoFile();
            this.GenerateAndDeployUnreliableTransportSettings();         
            this.GenerateAndDeployFabricPackage(updatedRolloutVersion.ToString());
        }

        public List<SettingsTypeSection> GetHostedServices()
        {
            foreach (string service in this.servicesToBeEnabled)
            {
                if (this.servicesToBeDeployed.Contains(service))
                {
                    AddToHostedServiceSections(service);
                }
            }
            return this.fabricHostSections;
        }
        #endregion Public Functions

        #region Private Functions
        private int GenerateAndDeployUnreliableTransportSettings()
        {            
            string filePath = System.IO.Path.Combine(this.nodeSettings.DeploymentFoldersInfo.WorkFolder, UnreliableTransportSettings.SettingsFileName);         
            
            DeployerTrace.WriteNoise("Writing Unreliable Transport Settings file at {0}", filePath);
                        
            UnreliableTransportSettings unreliableTransportSettings = new UnreliableTransportSettings(this.clusterSettings);

            // acquiring lock to read and write to unreliable transport settings file.
            FileLock fileLock = new FileLock(filePath, false /* writing lock */ );

            if (!fileLock.Acquire(new TimeSpan(0, 0, 5)))
            {
                DeployerTrace.WriteWarning("Failed to acquire lock to load Unreliable Transport Settings File. Aborting loading at file: {0}", filePath);
                return Constants.ErrorCode_Failure;
            }
            
            // checks if it necessary to merge
            if (!File.Exists(filePath))
            {
                // writes the result to Unreliable Transport Settings File 
                unreliableTransportSettings.WriteUnreliableTransportSettingsFile(filePath);
            }
            else
            {
                // creates a new UnreliableTransportSettings from existing UnreliableTransportSettings file
                UnreliableTransportSettings unreliableTransportSettingsExisting = new UnreliableTransportSettings(filePath);
                // merge files giving precedence to settings coming from cluster manifest
                unreliableTransportSettingsExisting.Merge(unreliableTransportSettings);
                // writes the result to Unreliable Transport Settings File
                unreliableTransportSettingsExisting.WriteUnreliableTransportSettingsFile(filePath);                
            }
#if DotNetCoreClrLinux
            Helpers.UpdateFilePermission(filePath);
#endif
            // releasing lock.
            fileLock.Release();

            return Constants.ErrorCode_Success;
        }

        private int DeploySeedInfoFile()
        {
            Dictionary<string, string> seedMapping = infrastructure.VoteMappings;
            if (seedMapping == null)
            {
                return Constants.ErrorCode_Success;
            }
            string dataRoot = nodeSettings.DeploymentFoldersInfo.DataRoot;
            string seedInfoFileName = Path.Combine(dataRoot, Constants.FileNames.SeedInfo);
            try
            {
                using (FileStream fileStream = new FileStream(seedInfoFileName, FileMode.Create, FileAccess.Write, FileShare.Read))
                {
                    using (TextWriter writer = new StreamWriter(fileStream))
                    {
                        writer.WriteLine(Constants.SectionNames.SeedMapping);
                        seedMapping.ForEach(pair => writer.WriteLine("  {0} = {1}", pair.Key, pair.Value));
                    }
                }
                DeployerTrace.WriteInfo("Generated SeedInfoFile successfully");
            }
            catch (Exception e)
            {
                DeployerTrace.WriteError("Unable to generated SeedInfoFile because {0}", e);
                throw;
            }
            return Constants.ErrorCode_Success;
        }

        private int DeployClusterManifest()
        {
            ThrowIfUpdateRoot();
            WriteManifestFile<ClusterManifestType>(this.nodeSettings.DeploymentFoldersInfo.CurrentClusterManifestFile, this.clusterManifest);
            WriteManifestFile<ClusterManifestType>(
                this.nodeSettings.DeploymentFoldersInfo.GetVersionedClusterManifestFile(this.clusterManifest.Version),
                this.clusterManifest);
            return Constants.ErrorCode_Success;
        }

        /// <summary>
        /// This method makes sure FabricLogRoot and FabricDataRoot should not be updated through cluster config upgrade.
        /// </summary>
        private void ThrowIfUpdateRoot()
        {
            var currentClusterManifestFile = this.nodeSettings.DeploymentFoldersInfo.CurrentClusterManifestFile;
            if (File.Exists(currentClusterManifestFile))
            {
                // Check if cluster manifest is invalid. See RDBug 14040324.
                string fileContents = string.Empty;
                if (new FileInfo(currentClusterManifestFile).Length == 0
                        || new Func<bool>(() =>
                        {
                            fileContents = File.ReadAllText(currentClusterManifestFile);
                            return string.IsNullOrWhiteSpace(fileContents);
                        }).Invoke())
                {
                    string errorMsg = string.Format(StringResources.ClusterManifestInvalidDeleting_Formatted, currentClusterManifestFile, fileContents);
                    DeployerTrace.WriteError(errorMsg);
                    File.Delete(currentClusterManifestFile);
                    throw new ClusterManifestValidationException(errorMsg);
                }

                ClusterManifestType currentManifest = XmlHelper.ReadXml<ClusterManifestType>(currentClusterManifestFile, SchemaLocation.GetWindowsFabricSchemaLocation());
                SetupSettings currentSettings = new SetupSettings(currentManifest);
                SetupSettings targetSettings = new SetupSettings(this.clusterManifest);
                if (targetSettings.FabricDataRoot != null && targetSettings.FabricDataRoot != currentSettings.FabricDataRoot)
                {
                    string errorMsg = string.Format(StringResources.UpdateNotAllowed, Constants.ParameterNames.FabricDataRoot);
                    DeployerTrace.WriteError(errorMsg);
                    throw new ClusterManifestValidationException(errorMsg);
                }
                if (targetSettings.FabricLogRoot != null && targetSettings.FabricLogRoot != currentSettings.FabricLogRoot)
                {
                    string errorMsg = string.Format(StringResources.UpdateNotAllowed, Constants.ParameterNames.FabricLogRoot);
                    DeployerTrace.WriteError(errorMsg);
                    throw new ClusterManifestValidationException(errorMsg);
                }
            }
        }

        private void GenerateCodeDeployment()
        {
            if (!this.nodeSettings.DeploymentFoldersInfo.IsCodeDeploymentNeeded)
            {
                return;
            }

            foreach (string service in this.servicesToBeDeployed)
            {
                string destinationFolder = this.nodeSettings.DeploymentFoldersInfo.GetCodeDeploymentDirectory(service);
                string sourceFolder = this.nodeSettings.DeploymentFoldersInfo.GetInstalledBinaryDirectory(service);
                if (IsCodeDeploymentNeeded(service, sourceFolder, destinationFolder))
                {
                    try
                    {
                        FabricDirectory.Copy(sourceFolder, destinationFolder, true);
                    }
                    catch (Exception e)
                    {
                        DeployerTrace.WriteError("Code deployment failed because: {0}. Source folder: {1}. destinationFolder: {2}", e, sourceFolder, destinationFolder);
                        throw;
                    }
                }
            }
        }

        private bool IsCodeDeploymentNeeded(string service, string sourceFolder, string destinationFolder)
        {
            if (sourceFolder.Equals(destinationFolder, StringComparison.OrdinalIgnoreCase))
            {
                return false;
            }
            var sourceExeFile = Path.Combine(sourceFolder, Constants.ServiceExes[service]);
            var destinationExeFile = Path.Combine(destinationFolder, Constants.ServiceExes[service]);
            if (!File.Exists(destinationExeFile) || !File.Exists(sourceExeFile))
            {
                return true;
            }

            return FileVersionInfo.GetVersionInfo(sourceExeFile).FileVersion !=
                   FileVersionInfo.GetVersionInfo(destinationExeFile).FileVersion;
        }

        private void GenerateWorkFolders()
        {
            string workFolder = this.nodeSettings.DeploymentFoldersInfo.WorkFolder;
            DeployerTrace.WriteNoise("Deploying work folder {0}", workFolder);
#if DotNetCoreClrLinux
            FabricDirectory.CreateDirectory(workFolder);
#else
            Directory.CreateDirectory(workFolder);
#endif
        }

        private void GenerateDataDeployment()
        {
            string dataDeploymentPath = this.nodeSettings.DeploymentFoldersInfo.DataDeploymentDirectory;
#if DotNetCoreClrLinux
            FabricDirectory.CreateDirectory(dataDeploymentPath);
#else
            Directory.CreateDirectory(dataDeploymentPath);
#endif
            InfrastructureInformationType infrastructureManifest = this.infrastructure.GetInfrastructureManifest();
            string filePath = this.nodeSettings.DeploymentFoldersInfo.InfrastructureManfiestFile;
            DeployerTrace.WriteNoise("Writing infrastructure manifest file at {0}", filePath);
            WriteManifestFile<InfrastructureInformationType>(filePath, infrastructureManifest);
            CopyClusterSettingsToFabricData();
        }

        private void GenerateConfigDeployment()
        {
            SettingsType settings = this.GenerateConfig();
            this.DeployConfig(settings);
        }

        private SettingsType GenerateConfig()
        {
            List<SettingsTypeSection> sections = new List<SettingsTypeSection>();
            sections.AddRange(this.nodeSettings.Settings);
            sections.AddRange(this.clusterSettings.Settings.FindAll(setting => setting.Name != FabricValidatorConstants.SectionNames.UnreliableTransport));
            return new SettingsType() { Section = sections.ToArray() };
        }

        private void GenerateSfssRGPolicy(
            SettingsTypeSection sfssRgPolicy,
            List<SettingsTypeSectionParameter> parameters,
            string service)
        {
            string[] governedResources = {
                Constants.ParameterNames.ProcessCpusetCpus,
                Constants.ParameterNames.ProcessCpuShares,
                Constants.ParameterNames.ProcessMemoryInMB,
                Constants.ParameterNames.ProcessMemorySwapInMB
            };

            foreach(string resourceName in governedResources)
            {
                var value = sfssRgPolicy.Parameter.FirstOrDefault(
                    param =>
                        param.Name.Equals(string.Format(CultureInfo.InvariantCulture, "{0}{1}", service, resourceName), StringComparison.OrdinalIgnoreCase) &&
                        !param.Value.Equals("", StringComparison.OrdinalIgnoreCase));
                if (value != null)
                {
                    parameters.Add(new SettingsTypeSectionParameter() { Name = resourceName, Value = value.Value, MustOverride = false });
                }
            }
        }

        public void AddToHostedServiceSections(string service)
        {
            ReleaseAssert.AssertIfNot(Constants.RequiredServices.Contains(service, StringComparer.Ordinal), string.Format(CultureInfo.InvariantCulture, "Unknown service {0}", service));
            string serviceExePath = Path.Combine(this.nodeSettings.DeploymentFoldersInfo.GetCodeDeploymentDirectory(service), Constants.ServiceExes[service]);
            string ctrlCHelperPath = Path.Combine(this.nodeSettings.DeploymentFoldersInfo.GetCodeDeploymentDirectory(Constants.FabricService), Constants.CtrlCSenderExe);
            List<SettingsTypeSectionParameter> parameters = new List<SettingsTypeSectionParameter>();
            parameters.Add(new SettingsTypeSectionParameter() { Name = Constants.ParameterNames.ExePath, Value = serviceExePath, MustOverride = false });
            parameters.Add(new SettingsTypeSectionParameter() { Name = Constants.ParameterNames.CtrlCSenderPath, Value = ctrlCHelperPath, MustOverride = false });
            parameters.Add(new SettingsTypeSectionParameter()
            {
                Name = Constants.ParameterNames.EnvironmentMap,
                Value = string.Format(CultureInfo.InvariantCulture, "FabricPackageFileName={0}", this.nodeSettings.DeploymentFoldersInfo.CurrentFabricPackageFile),
                MustOverride = false
            });
            parameters.Add(new SettingsTypeSectionParameter() { Name = Constants.ParameterNames.ServiceNodeName, Value = this.nodeSettings.NodeName, MustOverride = false });

            // Generate SF system service resource governance policy
            var sfssRgPolicy = nodeSettings.Settings.FirstOrDefault(
                param =>
                    param.Name.Equals(Constants.SectionNames.NodeSfssRgPolicies, StringComparison.OrdinalIgnoreCase));
             if (sfssRgPolicy != null)
             {
                GenerateSfssRGPolicy(sfssRgPolicy, parameters, service);
             }

#if DotNetCoreClrLinux
            var serviceRunAsSectionName = GetServiceSpecificRunAsSectionName(service);

            // First check if the service specific run as section exists
            var runAsSection = this.clusterSettings.Settings.SingleOrDefault(section => section.Name.Equals(serviceRunAsSectionName, StringComparison.OrdinalIgnoreCase));
            if (runAsSection == null)
            {
                // If the service specific run as section does not exist read the generic one for all services
                runAsSection =
                    this.clusterSettings.Settings.SingleOrDefault(
                        section =>
                        section.Name.Equals(Constants.SectionNames.RunAs, StringComparison.OrdinalIgnoreCase));
            }
#else
            var runAsSection = this.clusterSettings.Settings.SingleOrDefault( section => section.Name.Equals(Constants.SectionNames.RunAs, StringComparison.OrdinalIgnoreCase));
#endif

            if (runAsSection != null)
            {
                var accountNameParameter = runAsSection.Parameter.SingleOrDefault(parameter => parameter.Name.Equals(Constants.ParameterNames.RunAsAccountName, StringComparison.OrdinalIgnoreCase));
                if (accountNameParameter != null)
                {
                    parameters.Add(new SettingsTypeSectionParameter() { Name = Constants.ParameterNames.RunAsAccountName, Value = accountNameParameter.Value, MustOverride = accountNameParameter.MustOverride });
                }

                var accountTypeParameter = runAsSection.Parameter.SingleOrDefault(parameter => parameter.Name.Equals(Constants.ParameterNames.RunAsAccountType, StringComparison.OrdinalIgnoreCase));
                if (accountTypeParameter != null)
                {
                    parameters.Add(new SettingsTypeSectionParameter() { Name = Constants.ParameterNames.RunAsAccountType, Value = accountTypeParameter.Value, MustOverride = accountTypeParameter.MustOverride });
                }
                
                var passwordParameter = runAsSection.Parameter.SingleOrDefault(parameter => parameter.Name.Equals(Constants.ParameterNames.RunAsPassword, StringComparison.OrdinalIgnoreCase));
                if (passwordParameter != null)
                {
                    parameters.Add(new SettingsTypeSectionParameter() { Name = Constants.ParameterNames.RunAsPassword, Value = passwordParameter.Value, MustOverride = passwordParameter.MustOverride, IsEncrypted = passwordParameter.IsEncrypted });
                }
            }
            else if (this.infrastructure.IsDefaultSystem &&
                !this.infrastructure.IsScaleMin)
            {
                parameters.Add(new SettingsTypeSectionParameter() { Name = Constants.ParameterNames.RunAsAccountType, Value = "LocalSystem" });
            }

            string sectionName = string.Format(CultureInfo.InvariantCulture, Constants.SectionNames.HostSettingsSectionPattern, this.nodeSettings.NodeName, service);
            this.fabricHostSections.Add(new SettingsTypeSection() { Name = sectionName, Parameter = parameters.ToArray() });

            DeployerTrace.WriteNoise("Created the {0} section for node {1}.", sectionName, this.nodeSettings.NodeName);
        }

#if DotNetCoreClrLinux
        private static string GetServiceSpecificRunAsSectionName(string service)
        {
            var serviceRunAsSectionName = string.Empty;
            switch (service)
            {
                case Constants.DCAService:
                    serviceRunAsSectionName = Constants.SectionNames.RunAs_DCA;
                    break;
                case Constants.FabricService:
                    serviceRunAsSectionName = Constants.SectionNames.RunAs_Fabric;
                    break;
                case Constants.HttpGatewayService:
                    serviceRunAsSectionName = Constants.SectionNames.RunAs_HttpGateway;
                    break;
                default:
                    ReleaseAssert.Failfast("Unknown service {0}", service);
                    break;
            }

            return serviceRunAsSectionName;
        }
#endif

        private void DeployConfig(SettingsType settings)
        {
            string configDeploymentPath = this.nodeSettings.DeploymentFoldersInfo.ConfigDeploymentDirectory;
#if DotNetCoreClrLinux
            FabricDirectory.CreateDirectory(configDeploymentPath);
#else
            Directory.CreateDirectory(configDeploymentPath);
#endif
            string configFilePath = Path.Combine(configDeploymentPath, Constants.FileNames.Settings);
            DeployerTrace.WriteNoise("Generating the settings file at {0}", configFilePath);
            XmlHelper.WriteXmlExclusive<SettingsType>(configFilePath, settings);
        }

        private void GenerateAndDeployFabricPackage(string rolloutVersion)
        {
            ServicePackageType package = new ServicePackageType();
            package.ManifestVersion = this.nodeSettings.DeploymentFoldersInfo.versions.ClusterManifestVersion;
            package.Name = "Fabric";
            package.RolloutVersion = rolloutVersion;
            package.DigestedConfigPackage = new ServicePackageTypeDigestedConfigPackage[1];
            package.DigestedDataPackage = new ServicePackageTypeDigestedDataPackage[1];
            package.DigestedCodePackage = new ServicePackageTypeDigestedCodePackage[this.servicesToBeDeployed.Length];
            package.DigestedServiceTypes = new ServicePackageTypeDigestedServiceTypes();
            package.DigestedServiceTypes.RolloutVersion = rolloutVersion;
            package.DigestedServiceTypes.ServiceTypes = new ServiceTypeType[this.servicesToBeDeployed.Length];
            package.DigestedResources = new ServicePackageTypeDigestedResources();
            package.DigestedResources.RolloutVersion = rolloutVersion;

            package.DigestedConfigPackage[0] = new ServicePackageTypeDigestedConfigPackage();
            package.DigestedConfigPackage[0].RolloutVersion = rolloutVersion;
            package.DigestedConfigPackage[0].ConfigPackage = new ConfigPackageType();
            package.DigestedConfigPackage[0].ConfigPackage.Name = Constants.ConfigName;
            package.DigestedConfigPackage[0].ConfigPackage.Version = this.nodeSettings.DeploymentFoldersInfo.versions.ConfigVersion;

            package.DigestedDataPackage[0] = new ServicePackageTypeDigestedDataPackage();
            package.DigestedDataPackage[0].RolloutVersion = rolloutVersion;
            package.DigestedDataPackage[0].DataPackage = new DataPackageType();
            package.DigestedDataPackage[0].DataPackage.Name = Constants.DataName;
            package.DigestedDataPackage[0].DataPackage.Version = string.Empty;

            int packageIndex = 0;
            foreach (string service in this.servicesToBeDeployed)
            {
                package.DigestedCodePackage[packageIndex] = new ServicePackageTypeDigestedCodePackage();
                package.DigestedCodePackage[packageIndex].RolloutVersion = rolloutVersion;
                package.DigestedCodePackage[packageIndex].CodePackage = new CodePackageType();
                package.DigestedCodePackage[packageIndex].CodePackage.Name = service;
                package.DigestedCodePackage[packageIndex].CodePackage.Version = string.Empty;
                package.DigestedCodePackage[packageIndex].CodePackage.EntryPoint =
                    new EntryPointDescriptionType
                    {
                        Item = new EntryPointDescriptionTypeExeHost
                        {
                            Program = Constants.ServiceExes[service],
                            Arguments = null,
                            WorkingFolder = ExeHostEntryPointTypeWorkingFolder.Work
                        }
                    };
                package.DigestedServiceTypes.ServiceTypes[packageIndex] = new StatefulServiceTypeType();
                package.DigestedServiceTypes.ServiceTypes[packageIndex].ServiceTypeName = service;
                packageIndex++;
            }

            string packageFilePath = this.nodeSettings.DeploymentFoldersInfo.GetVersionedFabricPackageFile(rolloutVersion);
            DeployerTrace.WriteNoise("Generating the fabric package file at {0}", packageFilePath);
            XmlHelper.WriteXmlExclusive<ServicePackageType>(packageFilePath, package);

            string currentPackageFilePath = this.nodeSettings.DeploymentFoldersInfo.CurrentFabricPackageFile;
            DeployerTrace.WriteInfo("Generating the fabric package file at {0}", currentPackageFilePath);
            XmlHelper.WriteXmlExclusive<ServicePackageType>(currentPackageFilePath, package);
        }

        private static void UpdateFileAttribute(string filePath, bool readOnly)
        {
            if (File.Exists(filePath))
            {
                FileAttributes attributes = File.GetAttributes(filePath);
                attributes = readOnly ? attributes | FileAttributes.ReadOnly : attributes & ~FileAttributes.ReadOnly;
                File.SetAttributes(filePath, attributes);
            }
        }

        /// <summary>
        /// This method writes the manifest file in exclusive mode and then marks the manifest file read only.
        /// </summary>
        /// <typeparam name="T">The type of the manifest</typeparam>
        /// <param name="fileName">The name of the file where to write the manifest.</param>
        /// <param name="manifest">The type of the manifest.</param>
        private void WriteManifestFile<T>(string fileName, T manifest)
        {
            UpdateFileAttribute(fileName, false);
            XmlHelper.WriteXmlExclusive<T>(fileName, manifest);
            UpdateFileAttribute(fileName, true);
        }

        /// <summary>
        /// This method copies the latest ClusterSettings File to Fabric Data.
        /// The latest copy is determined by the version number in the filename.
        /// If the version number is the same in fabric data root and fabric code path, the one in fabric data root is copied.
        /// </summary>
        private void CopyClusterSettingsToFabricData()
        {
            string sourceClusterSettingsPathInFabricDataRoot = Directory.EnumerateFiles(this.nodeSettings.DeploymentFoldersInfo.DataRoot, Constants.FileNames.ClusterSettings).SingleOrDefault();
            string sourceClusterSettingsPathInFabricCode = Directory.EnumerateFiles(FabricEnvironment.GetCodePath(), Constants.FileNames.ClusterSettings).SingleOrDefault();
            var destClusterSettingsPath = Path.Combine(this.nodeSettings.DeploymentFoldersInfo.DataDeploymentDirectory, Constants.FileNames.ClusterSettings);

            if (sourceClusterSettingsPathInFabricDataRoot == null && sourceClusterSettingsPathInFabricCode!=null)
            {

                try
                {
                    File.Copy(sourceClusterSettingsPathInFabricCode, destClusterSettingsPath, true);
                }
                catch (Exception e)
                {
                    DeployerTrace.WriteError("Code deployment failed because: {0}. Source Path: {1}. destinationFolder: {2}", e, sourceClusterSettingsPathInFabricCode, destClusterSettingsPath);
                    throw;
                }
            }
            else if (sourceClusterSettingsPathInFabricDataRoot != null && sourceClusterSettingsPathInFabricCode != null)
            {               
                try
                {
                    File.Copy(sourceClusterSettingsPathInFabricDataRoot, destClusterSettingsPath, true);
                }
                catch (Exception e)
                {
                     DeployerTrace.WriteError("Code deployment failed because: {0}. Source Path: {1}. destinationFolder: {2}", e, sourceClusterSettingsPathInFabricCode, destClusterSettingsPath);
                     throw;
                }
            }
        }

        #endregion
    }
}