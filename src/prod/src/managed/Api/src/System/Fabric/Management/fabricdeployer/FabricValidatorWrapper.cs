// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Collections.Generic;
using System.Fabric.Common;
using System.Fabric.Management.FabricDeployer;
using System.Fabric.Management.ServiceModel;
using System.Fabric.Management.WindowsFabricValidator;
using System.Fabric.Strings;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Security;

namespace System.Fabric.FabricDeployer
{
    internal class ClusterManifestValidationException : Exception
    {
        public ClusterManifestValidationException(string message)
            : base(message)
        {
        }
    }

    internal class FabricHostRestartRequiredException : Exception
    {
        public FabricHostRestartRequiredException(string message)
            : base(message)
        {
        }
    }

    internal class FabricHostRestartNotRequiredException : Exception
    {
        public FabricHostRestartNotRequiredException(string message)
            : base(message)
        {
        }
    }

    internal class FabricValidatorWrapper
    {
        internal bool IsHttpGatewayEnabled { get { return fabricValidator.IsHttpGatewayEnabled; } }

        internal bool IsTVSEnabled { get { return fabricValidator.IsTVSEnabled; } }

        internal bool IsDCAEnabled { get { return fabricValidator.IsDCAEnabled; } }

        internal bool ShouldRegisterSpnForMachineAccount { get { return fabricValidator.ShouldRegisterSpnForMachineAccount; } }

        private FabricValidator fabricValidator;

        private readonly DeploymentParameters parameters;

        private readonly ClusterManifestType manifest;

        private readonly Infrastructure infrastructure;

        internal FabricValidatorWrapper(DeploymentParameters parameters, ClusterManifestType manifest, Infrastructure infrastructure)
        {
            this.parameters = parameters;
            this.manifest = manifest;
            this.infrastructure = infrastructure;
        }

        internal void ValidateAndEnsureDefaultImageStore()
        {
            try
            {
                fabricValidator = FabricValidator.Create(manifest, infrastructure == null ? null : infrastructure.InfrastructureNodes, new WindowsFabricSettings(this.manifest), parameters.Operation);
                fabricValidator.Validate();
            }
            catch (Exception e)
            {
                throw new ClusterManifestValidationException(
                    string.Format(StringResources.Error_FabricDeployer_InvalidClusterManifest_Formatted, e));
            }

            if (this.fabricValidator.IsSingleMachineDeployment)
            {
                this.EnsureImageStoreForSingleMachineDeployment(Path.Combine(parameters.DeploymentSpecification.GetDataRoot(), Constants.DefaultImageStoreRootFolderName));
                this.EnsureCorrectDiagnosticStore(Path.Combine(parameters.DeploymentSpecification.GetDataRoot(), Constants.DefaultDiagnosticsStoreRootFolderName));
            }
            else
            {
                this.EnsureCorrectDiagnosticStore(null);
            }
        }

        internal static void CompareAndAnalyze(ClusterManifestType currentClusterManifest, ClusterManifestType targetClusterManifest, Infrastructure infrastructure, DeploymentParameters parameters)
        {  
            // If the infrastructure elements are different, the update cannot continue
            if (targetClusterManifest.Infrastructure.Item.GetType() != currentClusterManifest.Infrastructure.Item.GetType())
            {
                DeployerTrace.WriteError("Invalid Operation. Cannot update from 1 infrastructure type to another");
                throw new ClusterManifestValidationException(StringResources.Error_FabricDeployer_InvalidClusterManifest_Formatted);
            }

            // Validate that the new cluster manifest and the node list are valid
            try
            {
                var currentWindowsFabricSettings = new WindowsFabricSettings(currentClusterManifest);
                var targetWindowsFabricSettings = new WindowsFabricSettings(targetClusterManifest);
                FabricValidator.Create(targetClusterManifest, infrastructure.InfrastructureNodes, targetWindowsFabricSettings, parameters.Operation).Validate();
                //Validate if the SeedNodeCount increases by one
                int currentSeedNodes = 0, targetSeedNodes = 0;
                List<String> currentSeedNodesList = new List<String>();
                List<String> targetSeedNodesList = new List<String>();
                if (currentClusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsAzure)
                {
                    ClusterManifestTypeInfrastructureWindowsAzure currentInfrastructure = currentClusterManifest.Infrastructure.Item as ClusterManifestTypeInfrastructureWindowsAzure;
                    ClusterManifestTypeInfrastructureWindowsAzure targetInfrastructure = targetClusterManifest.Infrastructure.Item as ClusterManifestTypeInfrastructureWindowsAzure;
                    for (int i = 0; i < currentInfrastructure.Roles.Length; i++)
                    {
                        currentSeedNodes += currentInfrastructure.Roles[i].SeedNodeCount;
                    }
                    for (int j = 0; j < targetInfrastructure.Roles.Length; j++)
                    {
                        targetSeedNodes += targetInfrastructure.Roles[j].SeedNodeCount;
                    }
                }
                else if (currentClusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsAzureStaticTopology)
                {
                    ClusterManifestTypeInfrastructureWindowsAzureStaticTopology currentInfrastructure = currentClusterManifest.Infrastructure.Item as ClusterManifestTypeInfrastructureWindowsAzureStaticTopology;
                    ClusterManifestTypeInfrastructureWindowsAzureStaticTopology targetInfrastructure = targetClusterManifest.Infrastructure.Item as ClusterManifestTypeInfrastructureWindowsAzureStaticTopology;
                    foreach (FabricNodeType nodelist in currentInfrastructure.NodeList)
                    {
                        if (nodelist.IsSeedNode)
                        {
                            currentSeedNodes++;
                        }
                    }
                    foreach (FabricNodeType nodelist in targetInfrastructure.NodeList)
                    {
                        if (nodelist.IsSeedNode)
                        {
                            targetSeedNodes++;
                        }
                    }
                }
#if DotNetCoreClrLinux
                else if (currentClusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureLinux)
                {
                    var currentInfrastructure = currentClusterManifest.Infrastructure.Item as ClusterManifestTypeInfrastructureLinux;
                    var targetInfrastructure = targetClusterManifest.Infrastructure.Item as ClusterManifestTypeInfrastructureLinux;
#else
                else if (currentClusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsServer)
                {
                    var currentInfrastructure = currentClusterManifest.Infrastructure.Item as ClusterManifestTypeInfrastructureWindowsServer;
                    var targetInfrastructure = targetClusterManifest.Infrastructure.Item as ClusterManifestTypeInfrastructureWindowsServer;
#endif
                    foreach (FabricNodeType nodelist in currentInfrastructure.NodeList)
                    {
                        if (nodelist.IsSeedNode)
                        {
                            currentSeedNodes++;
                            currentSeedNodesList.Add(nodelist.NodeName);
                        }
                    }
                    foreach (FabricNodeType nodelist in targetInfrastructure.NodeList)
                    {
                        if (nodelist.IsSeedNode)
                        {
                            targetSeedNodes++;
                            targetSeedNodesList.Add(nodelist.NodeName);
                        }
                    }
                }
                if (System.Math.Abs(currentSeedNodes - targetSeedNodes) > 1)
                {
                    DeployerTrace.WriteError(StringResources.Warning_SeedNodeCountCanOnlyChangeByOne);
                    throw new ClusterManifestValidationException(StringResources.Warning_SeedNodeCountCanOnlyChangeByOne);
                }
                else if (currentSeedNodesList.Except(targetSeedNodesList).Union(targetSeedNodesList.Except(currentSeedNodesList)).Count() > 1)
                {
                    DeployerTrace.WriteError(StringResources.Warning_SeedNodeSetCannotHaveMultipleChanges);
                    throw new ClusterManifestValidationException(StringResources.Warning_SeedNodeSetCannotHaveMultipleChanges);
                }

                var fabricValidator = FabricValidator.Create(currentClusterManifest, null, currentWindowsFabricSettings, parameters.Operation);
                // Finally, compare the settings and decide if fabric must restart or not
                IEnumerable<KeyValuePair<string, string>> modifiedStaticSettings = fabricValidator.CompareAndAnalyze(targetClusterManifest, parameters.NodeTypeName);
                if (modifiedStaticSettings.Count() > 0)
                {
                    WriteModifiedStaticSettingsToFile(modifiedStaticSettings, targetClusterManifest.FabricSettings, parameters.OutputFile);
                    throw new FabricHostRestartRequiredException("Fabric host restart is required");
                }
                else
                {
                    throw new FabricHostRestartNotRequiredException("Fabric host restart is not required");
                }
            }
            catch (Exception e)
            {
                if (e is FabricHostRestartRequiredException || e is FabricHostRestartNotRequiredException)
                {
                    throw e;
                }
                else
                {
                    throw new ClusterManifestValidationException(
                        string.Format(StringResources.Error_FabricDeployer_InvalidClusterManifest_Formatted, e));
                }
            }
        }

        private void EnsureImageStoreForSingleMachineDeployment(string defaultImageStoreRoot)
        {
            SettingsOverridesTypeSection managementSection = this.manifest.FabricSettings.FirstOrDefault(
                section => section.Name.Equals(Constants.SectionNames.Management, StringComparison.OrdinalIgnoreCase));

            SettingsOverridesTypeSectionParameter imageStoreParameter = null;
            if (managementSection != null)
            {
                imageStoreParameter = managementSection.Parameter.FirstOrDefault(
                    parameter => parameter.Name.Equals(Constants.ParameterNames.ImageStoreConnectionString, StringComparison.OrdinalIgnoreCase));
            }

            if (imageStoreParameter == null)
            {
                return;
            }

            SecureString imageStoreRoot = fabricValidator.ImageStoreConnectionString;
            char[] secureImageStoreChars = Utility.SecureStringToCharArray(imageStoreRoot);

            if ((secureImageStoreChars == null) || (new string(secureImageStoreChars)).Equals(Constants.DefaultTag, StringComparison.OrdinalIgnoreCase))
            {
                var imageStoreIncomingFolder = Path.Combine(defaultImageStoreRoot, Constants.DefaultImageStoreIncomingFolderName);
                if (!Directory.Exists(imageStoreIncomingFolder))
                {
                    Directory.CreateDirectory(imageStoreIncomingFolder);
                }
                DeployerTrace.WriteInfo("Default ImageStore incoming folder: {0}", imageStoreIncomingFolder);

                imageStoreParameter.Value = string.Format(CultureInfo.InvariantCulture, "{0}{1}", System.Fabric.FabricValidatorConstants.FileImageStoreConnectionStringPrefix, defaultImageStoreRoot);
                imageStoreParameter.IsEncrypted = false;
            }
        }

        private void EnsureCorrectDiagnosticStore(string defaultDiagnosticsStoreRoot)
        {
            var diagnosticsSection = this.manifest.FabricSettings.FirstOrDefault(
                section => section.Name.Equals(Constants.SectionNames.Diagnostics, StringComparison.OrdinalIgnoreCase));
            if (diagnosticsSection == null)
            {
                return;
            }

            var consumerInstances = diagnosticsSection.Parameter.FirstOrDefault(
                param => param.Name.Equals(Constants.ParameterNames.ConsumerInstances, StringComparison.OrdinalIgnoreCase));
            if (consumerInstances == null)
            {
                return;
            }

            foreach (var instance in consumerInstances.Value.Split(','))
            {
                var consumerInstance = instance.Trim();
                var consumerInstanceSection = this.manifest.FabricSettings.FirstOrDefault(section => section.Name.Equals(consumerInstance, StringComparison.OrdinalIgnoreCase));
                var storeConnectionString = consumerInstanceSection.Parameter.FirstOrDefault(
                    param => param.Name.Equals(Constants.ParameterNames.StoreConnectionString, StringComparison.OrdinalIgnoreCase));
                if (storeConnectionString == null)
                {
                    continue;
                }
                char[] secureStringName = Utility.SecureStringToCharArray(storeConnectionString.GetSecureValue(fabricValidator.StoreName));
                if ((secureStringName == null) || (new string(secureStringName)).Equals(Constants.DefaultTag, StringComparison.OrdinalIgnoreCase))
                {
                    if (defaultDiagnosticsStoreRoot == null)
                    {
                        DeployerTrace.WriteError(
                            "Section {0} parameter {1} cannot be {2} for multi-machine deployment",
                            consumerInstance,
                            Constants.ParameterNames.StoreConnectionString,
                            Constants.DefaultTag);
                        throw new ClusterManifestValidationException(StringResources.Error_FabricDeployer_InvalidClusterManifest_Formatted);
                    }
                    var storeFolder = Path.Combine(defaultDiagnosticsStoreRoot, consumerInstance);
                    if (!Directory.Exists(storeFolder))
                    {
                        Directory.CreateDirectory(storeFolder);
                    }

                    DeployerTrace.WriteInfo("Default Configuration incoming folder: {0}", storeFolder);
                    storeConnectionString.Value = string.Format(CultureInfo.InvariantCulture, "{0}{1}",
                        System.Fabric.FabricValidatorConstants.FileImageStoreConnectionStringPrefix, storeFolder);
                    storeConnectionString.IsEncrypted = false;
                }
            }
        }

        private static void WriteModifiedStaticSettingsToFile(
            IEnumerable<KeyValuePair<string, string>> modifiedStaticSettings,
            SettingsOverridesTypeSection[] newSettings,
            string outputFile)
        {
            if (string.IsNullOrEmpty(outputFile)) { return; }

            FileInfo fileInfo = new FileInfo(outputFile);
            if (!fileInfo.Directory.Exists)
            {
                fileInfo.Directory.Create();
            }

            using (StreamWriter writer = new StreamWriter(fileInfo.Open(FileMode.CreateNew), Text.Encoding.Unicode))
            {
                for (int i = 0; i < modifiedStaticSettings.Count(); i++)
                {                    
                    var staticSetting = modifiedStaticSettings.ElementAt(i);
                    string value = string.Empty;
                    bool isEncrypted = false;

                    if (newSettings != null)
                    {
                        var matchingSection = newSettings.FirstOrDefault(section => section.Name.Equals(staticSetting.Key));
                        if (matchingSection != null && matchingSection.Parameter != null)
                        {
                            var matchingParameter = matchingSection.Parameter.FirstOrDefault(parameter => parameter.Name.Equals(staticSetting.Value));
                            if (matchingParameter != null)
                            {
                                value = matchingParameter.Value;
                                isEncrypted = matchingParameter.IsEncrypted;
                            }
                        }
                    }

                    if (i > 0) { writer.WriteLine(); }

                    writer.WriteLine(staticSetting.Key /*sectionName*/);
                    writer.WriteLine(staticSetting.Value /*parameterName*/);
                    writer.WriteLine(value);
                    writer.Write(isEncrypted);
                }
            }
        }
    }
}