// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.WindowsFabricValidator
{
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Management.FabricDeployer;
    using System.Fabric.Common.ImageModel;
    using System.Globalization;
    using System.IO;
    using System.Fabric.Management.ServiceModel;
    using System.Linq;

    /// <summary>
    /// This class provide getter methods for various folders that might be needed during deployment.
    /// </summary>
    internal class DeploymentFolders
    {
        #region Constructors
        public DeploymentFolders(string fabricInstallationFolder, SettingsOverridesTypeSection[] fabricSettings, FabricDeploymentSpecification deploymentSpecification, string nodeName, Versions versions, bool isScaleMin)
        {
            this.nodeName = nodeName;
            this.versions = versions;
            this.installationFolder = fabricInstallationFolder;
            this.IsCodeDeploymentNeeded = isScaleMin;
            this.DataRoot = deploymentSpecification.GetDataRoot();
            this.deploymentSpecification = deploymentSpecification;
            this.DeploymentRoot = this.deploymentSpecification.GetNodeFolder(nodeName);
            this.DataDeploymentDirectory = this.deploymentSpecification.GetDataDeploymentFolder(nodeName);
            this.WorkFolder = this.deploymentSpecification.GetWorkFolder(nodeName);
            this.CurrentClusterManifestFile = this.deploymentSpecification.GetCurrentClusterManifestFile(nodeName);
            this.CurrentFabricPackageFile = this.deploymentSpecification.GetCurrentFabricPackageFile(nodeName);
            this.InfrastructureManfiestFile = this.deploymentSpecification.GetInfrastructureManfiestFile(nodeName);
            this.ConfigDeploymentDirectory = this.deploymentSpecification.GetConfigurationDeploymentFolder(nodeName, versions.ConfigVersion);
            this.ApplicationDeploymentFolder = GetApplicationDeploymentFolder(fabricSettings, isScaleMin);
        }
        #endregion   
        
        #region Public Properties
        public string ConfigDeploymentDirectory { get; private set; }
        public string DataDeploymentDirectory { get; private set; }
        public string DataRoot { get; private set; }
        public string WorkFolder { get; private set; }
        public Versions versions { get; private set; }
        public bool IsCodeDeploymentNeeded { get; private set; }
        public string DeploymentRoot { get; private set; }
        public string CurrentClusterManifestFile { get; private set; }
        public string CurrentFabricPackageFile { get; private set; }
        public string InfrastructureManfiestFile { get; private set; }
        public string ApplicationDeploymentFolder { get; private set; }
        #endregion

        #region Public Functions
        public string GetCodeDeploymentDirectory(string service)
        {
            return this.IsCodeDeploymentNeeded ?
                this.deploymentSpecification.GetCodeDeploymentFolder(this.nodeName, service) 
                : this.GetInstalledBinaryDirectory(service);
        }

        public string GetInstalledBinaryDirectory(string service)
        {
            return this.deploymentSpecification.GetInstalledBinaryFolder(this.installationFolder, service);
        }

        public static string GetInstalledBinaryDirectory(string installationFolder, string service)
        {
            return FabricDeploymentSpecification.Create().GetInstalledBinaryFolder(installationFolder, service);
        }

        public string GetVersionedClusterManifestFile(string version)
        {
            return this.deploymentSpecification.GetVersionedClusterManifestFile(this.nodeName, version);
        }
        
        public string GetVersionedFabricPackageFile(string version)
        {
            return this.deploymentSpecification.GetVersionedFabricPackageFile(this.nodeName, version);
        }
        #endregion

        private string GetApplicationDeploymentFolder(SettingsOverridesTypeSection[] fabricSettings, bool isScaleMin)
        {
            string deploymentFolder = "Applications";

            if (fabricSettings != null)
            {
                var managementSection = fabricSettings.FirstOrDefault(section => string.Equals(section.Name, FabricValidatorConstants.SectionNames.Management, StringComparison.OrdinalIgnoreCase));
                if (managementSection != null)
                {
                    var deploymentDirectoryParameter = managementSection.Parameter.FirstOrDefault(parameter => string.Equals(parameter.Name, FabricValidatorConstants.ParameterNames.DeploymentDirectory));
                    if (deploymentDirectoryParameter != null)
                    {
                        deploymentFolder = deploymentDirectoryParameter.Value;
                    }
                }
            }

            if (string.IsNullOrEmpty(deploymentFolder))
            {
                return this.WorkFolder;
            }
            else if (!Path.IsPathRooted(deploymentFolder))
            {
                return Path.Combine(this.WorkFolder, deploymentFolder);
            }
            else if (isScaleMin)
            {
                return Path.Combine(deploymentFolder, this.nodeName);
            }
            else
            {
                return deploymentFolder;
            }
        }

        #region Private Fields
        private string installationFolder;
        private FabricDeploymentSpecification deploymentSpecification;
        private string nodeName;
        #endregion
    }
}