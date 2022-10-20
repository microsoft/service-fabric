// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using Management.ServiceModel;
    using Microsoft.Win32;
    using System.Collections.Generic;
    using System.Fabric.Strings;
    using System.IO;
    using System.Security;

    public static class ConfigurationDeployer
    {
        internal static readonly string DeleteLogTrue = "DeleteLogTrue";
        internal static readonly string DeleteLogFalse = "DeleteLogFalse";

        internal static void NewNodeConfiguration(
            string clusterManifestPath,
            string infrastructureManifestPath,
            string jsonClusterConfigPath, /* Value is null if it's not Standalone deployment */
            string fabricDataRoot,
            string fabricLogRoot,
            string fabricHostCredentialUser,
            SecureString fabricHostCredentialPassword,
            bool runFabricHostServiceAsManual,
            bool removeExistingConfiguration,
            FabricPackageType fabricPackageType,
            string fabricPackageRoot,
            string machineName,
            string bootstrapPackagePath)
        {
            try
            {
                NewNodeConfigurationInner(
                    clusterManifestPath,
                    infrastructureManifestPath,
                    jsonClusterConfigPath,
                    fabricDataRoot,
                    fabricLogRoot,
                    fabricHostCredentialUser,
                    fabricHostCredentialPassword,
                    runFabricHostServiceAsManual,
                    removeExistingConfiguration,
                    fabricPackageType,
                    fabricPackageRoot,
                    machineName,
                    bootstrapPackagePath);
            }
            catch (Exception exception)
            {
                DeployerTrace.WriteError(exception.ToString());
                throw;
            }
        }

        internal static void RemoveNodeConfiguration(bool deleteLog, FabricPackageType fabricPackageType, string machineName)
        {
            try
            {
                RemoveNodeConfigurationInner(deleteLog, fabricPackageType, machineName);
            }
            catch (Exception exception)
            {
                DeployerTrace.WriteError(exception.ToString());
                throw;
            }
        }

        private static void NewNodeConfigurationInner(
          string clusterManifestPath,
          string infrastructureManifestPath,
          string jsonClusterConfigPath, 
          string fabricDataRoot,
          string fabricLogRoot,
          string fabricHostCredentialUser,
          SecureString fabricHostCredentialPassword,
          bool runFabricHostServiceAsManual,
          bool removeExistingConfiguration,
          FabricPackageType fabricPackageType,
          string fabricPackageRoot,
          string machineName,
          string bootstrapPackagePath)
        {
            if (!string.IsNullOrEmpty(machineName))
            {
                if (!string.IsNullOrEmpty(fabricHostCredentialUser) || fabricHostCredentialPassword != null)
                {
                    throw new InvalidOperationException(StringResources.Error_RemoteNodeConfigNotSupportedWithCredential);
                }
            }

            // Delete the registry value used for RemoveNodeConfiguration if it is present
            if (fabricPackageType == FabricPackageType.MSI)
            {
                RemoveNodeConfigOperation.DeleteRemoveNodeConfigurationRegistryValue(machineName);
            }

            if (removeExistingConfiguration)
            {
                RemoveNodeConfigurationInner(true, fabricPackageType, machineName);
            }

            var parameters = new Dictionary<string, dynamic>
            {
                {DeploymentParameters.ClusterManifestString, clusterManifestPath},
                {DeploymentParameters.InfrastructureManifestString, infrastructureManifestPath},
                {DeploymentParameters.FabricDataRootString, fabricDataRoot},
                {DeploymentParameters.FabricLogRootString, fabricLogRoot}
            };

            if (fabricHostCredentialUser != null)
            {
                parameters.Add(DeploymentParameters.RunAsUserNameString, fabricHostCredentialUser);
            }

            if (jsonClusterConfigPath != null)
            {
                parameters.Add(DeploymentParameters.JsonClusterConfigLocationString, jsonClusterConfigPath);
            }

            if (fabricHostCredentialPassword != null)
            {
                parameters.Add(DeploymentParameters.RunAsPaswordString, fabricHostCredentialPassword);
            }

            if (machineName != null)
            {
                parameters.Add(DeploymentParameters.MachineNameString, machineName);
            }

            if (fabricPackageRoot != null)
            {
                parameters.Add(DeploymentParameters.FabricPackageRootString, fabricPackageRoot);
            }

            if (runFabricHostServiceAsManual)
            {
                parameters.Add(DeploymentParameters.ServiceStartupTypeString, FabricDeployerServiceController.ServiceStartupType.Manual.ToString());
            }

            if (bootstrapPackagePath != null)
            {
                parameters.Add(DeploymentParameters.BootstrapMSIPathString, bootstrapPackagePath);
            }

            // Create DeploymentParameters object with conditional retrieval of FabricBinRoot (to work around infinite recursion due to invalid reflection)
            bool setBinRoot = fabricPackageType != FabricPackageType.XCopyPackage;
            var deploymentParameters = new DeploymentParameters(setBinRoot);
            deploymentParameters.DeploymentPackageType = fabricPackageType;

            try
            {
                deploymentParameters.SetParameters(parameters, DeploymentOperations.Configure);

                DeploymentOperation.ExecuteOperation(deploymentParameters);
            }
            catch (Exception e)
            {
                DeployerTrace.WriteError("{0}", e.ToString());
                throw;
            }
        }

        private static void RemoveNodeConfigurationInner(bool deleteLog, FabricPackageType fabricPackageType, string machineName)
        {
            var parameters = new Dictionary<string, dynamic>
                {
                    {DeploymentParameters.MachineNameString, machineName}
                };

            // Create DeploymentParameters object with conditional retrieval of FabricBinRoot (to work around infinite recursion due to invalid reflection)
            bool setBinRoot = fabricPackageType != FabricPackageType.XCopyPackage;
            var deploymentParameters = new DeploymentParameters(setBinRoot);
            deploymentParameters.DeleteLog = deleteLog;
            deploymentParameters.DeploymentPackageType = fabricPackageType;
            deploymentParameters.SetParameters(parameters, DeploymentOperations.RemoveNodeConfig);

            try
            {
                DeploymentOperation.ExecuteOperation(deploymentParameters);
            }
            catch (Exception e)
            {
                DeployerTrace.WriteError("{0}", e.ToString());
                throw;
            }
        }
    }
}