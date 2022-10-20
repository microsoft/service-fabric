// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric;
    using System.Fabric.Description;
    using System.Fabric.FabricDeployer;
    using System.Fabric.Health;
    using System.Fabric.ImageStore;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.IO;
    using System.Management.Automation;
    using System.Management.Automation.Runspaces;
    using System.Threading;
    using Microsoft.ServiceFabric.DeploymentManager;
    using Microsoft.ServiceFabric.DeploymentManager.BPA;
    using Microsoft.ServiceFabric.DeploymentManager.Model;
    using CMCommon = Microsoft.ServiceFabric.ClusterManagementCommon;

    public abstract class ClusterCmdletBase : CommonCmdletBase
    {
        protected void NewNodeConfiguration(
            string clusterManifestPath,
            string infrastructureManifestPath,
            string fabricDataRoot,
            string fabricLogRoot,
            PSCredential fabricHostCredential,
            bool runFabricHostServiceAsManual,
            bool removeExistingConfiguration,
            bool usingFabricPackage,
            string fabricPackageRoot,
            string machineName,
            string bootstrapMsiPath)
        {
            try
            {
                string clusterManifestAbsolutePath = GetAbsolutePath(clusterManifestPath);
                string infrastructureManifestAbsolutePath = GetAbsolutePath(infrastructureManifestPath);

                if (string.IsNullOrEmpty(machineName) || usingFabricPackage)
                {
                    ConfigurationDeployer.NewNodeConfiguration(
                        clusterManifestAbsolutePath,
                        infrastructureManifestAbsolutePath,
                        null,
                        fabricDataRoot,
                        fabricLogRoot,
                        fabricHostCredential == null ? null : fabricHostCredential.UserName,
                        fabricHostCredential == null ? null : fabricHostCredential.Password,
                        runFabricHostServiceAsManual,
                        removeExistingConfiguration,
                        usingFabricPackage ? FabricPackageType.XCopyPackage : FabricPackageType.MSI,
                        fabricPackageRoot,
                        machineName,
                        bootstrapMsiPath);
                    this.WriteObject(StringResources.Info_NewNodeConfigurationSucceeded);
                }
                else
                {
                    WSManConnectionInfo connectionInfo = new WSManConnectionInfo();
                    connectionInfo.ComputerName = machineName;
                    Runspace runspace = RunspaceFactory.CreateRunspace(connectionInfo);
                    runspace.Open();
                    using (PowerShell ps = PowerShell.Create())
                    {
                        ps.Runspace = runspace;
                        if (!string.IsNullOrEmpty(clusterManifestAbsolutePath))
                        {
                            ps.AddCommand("New-ServiceFabricNodeConfiguration").AddParameter("ClusterManifestPath", clusterManifestAbsolutePath);
                        }

                        if (!string.IsNullOrEmpty(infrastructureManifestAbsolutePath))
                        {
                            ps.AddParameter("InfrastructureManifestAbsolutePath", infrastructureManifestAbsolutePath);
                        }

                        if (!string.IsNullOrEmpty(fabricDataRoot))
                        {
                            ps.AddParameter("FabricDataRoot", fabricDataRoot);
                        }

                        if (!string.IsNullOrEmpty(fabricLogRoot))
                        {
                            ps.AddParameter("FabricLogRoot", fabricLogRoot);
                        }

                        if (runFabricHostServiceAsManual)
                        {
                            ps.AddParameter("RunFabricHostServiceAsManual");
                        }

                        if (removeExistingConfiguration)
                        {
                            ps.AddParameter("RemoveExistingConfiguration");
                        }

                        if (!string.IsNullOrEmpty(bootstrapMsiPath))
                        {
                            ps.AddParameter("BootstrapMsiPath", bootstrapMsiPath);
                        }

                        if (fabricHostCredential != null)
                        {
                            ps.AddParameter("FabricHostCredential", fabricHostCredential);
                        }

                        var results = ps.Invoke();
                        foreach (PSObject r in results)
                        {
                            this.WriteObject(r);
                        }
                    }

                    runspace.Close();
                }
            }
            catch (Exception exception)
            {
                this.ThrowTerminatingError(
                    exception,
                    Constants.NewNodeConfigurationErrorId,
                    null);
            }
        }

        protected void UpdateNodeNodeConfiguration(string clusterManifestPath)
        {
            try
            {
                var parameters = new Dictionary<string, dynamic>
                {
                    { DeploymentParameters.ClusterManifestString, this.GetAbsolutePath(clusterManifestPath) },
                };

                var deploymentParameters = new DeploymentParameters();
                deploymentParameters.SetParameters(parameters, DeploymentOperations.Update);
                DeploymentOperation.ExecuteOperation(deploymentParameters);

                this.WriteObject(StringResources.Info_UpdateNodeConfigurationSucceeded);
            }
            catch (Exception exception)
            {
                this.ThrowTerminatingError(
                    exception,
                    Constants.UpdateNodeConfigurationErrorId,
                    null);
            }
        }

        protected void RemoveNodeConfiguration(bool deleteLog, bool usingFabricPackage, string machineName)
        {
            try
            {
                ConfigurationDeployer.RemoveNodeConfiguration(
                    deleteLog,
                    usingFabricPackage ? FabricPackageType.XCopyPackage : FabricPackageType.MSI,
                    machineName);
                this.WriteObject(StringResources.Info_RemoveNodeConfigurationSucceeded);
            }
            catch (Exception exception)
            {
                this.ThrowTerminatingError(
                    exception,
                    Constants.RemoveNodeConfigurationErrorId,
                    null);
            }
        }

        protected List<RuntimePackageDetails> GetRuntimeSupportedVersion(DownloadableRuntimeVersionReturnSet returnSet)
        {
            return DeploymentManager.GetDownloadableRuntimeVersions(returnSet);
        }

        protected List<RuntimePackageDetails> GetRuntimeUpgradeVersion(string baseVersion)
        {
            return DeploymentManager.GetUpgradeableRuntimeVersions(baseVersion);
        }

        protected void NewCluster(string clusterConfigurationFilePath, string fabricPackageSourcePath, bool noCleanupOnFailure, bool force, int maxPercentFailedNodes, uint timeoutInSeconds)
        {
            this.ChangeCurrentDirectoryToSession();
            clusterConfigurationFilePath = this.GetAbsolutePath(clusterConfigurationFilePath);
            fabricPackageSourcePath = this.GetAbsolutePath(fabricPackageSourcePath);

            ClusterCreationOptions creationOptions = ClusterCreationOptions.None;
            if (noCleanupOnFailure)
            {
                creationOptions |= ClusterCreationOptions.OptOutCleanupOnFailure;
            }

            if (force)
            {
                creationOptions |= ClusterCreationOptions.Force;
            }

            TimeSpan? timeout = null;
            if (timeoutInSeconds > 0)
            {
                timeout = TimeSpan.FromSeconds(timeoutInSeconds);
            }

            try
            {
                this.WriteObject(StringResources.Info_CreatingServiceFabricCluster);
                this.WriteObject(StringResources.Info_CreatingServiceFabricClusterDebugDetails);
                DeploymentManager.CreateClusterAsync(
                    clusterConfigurationFilePath,
                    fabricPackageSourcePath,
                    creationOptions,
                    maxPercentFailedNodes,
                    timeout).Wait();

                var standAloneModel = StandAloneInstallerJsonModelBase.GetJsonConfigFromFile(clusterConfigurationFilePath);
                var node = standAloneModel.Nodes[0];
                var nodeType = standAloneModel.GetUserConfig().NodeTypes.Find(type => type.Name == node.NodeTypeRef);
                this.WriteObject(string.Format(StringResources.Info_CreateServiceFabricClusterSucceeded, node.IPAddress, nodeType.ClientConnectionEndpointPort));
            }
            catch (Exception exception)
            {
                this.WriteObject(StringResources.Error_CreateServiceFabricClusterFailed);
                this.WriteObject(exception.InnerException != null ? exception.InnerException.Message : exception.Message);
                this.ThrowTerminatingError(
                    exception,
                    Constants.NewNodeConfigurationErrorId,
                    null);
            }
        }

        protected void RemoveCluster(string clusterConfigurationFilePath, bool deleteLog)
        {
            this.ChangeCurrentDirectoryToSession();
            clusterConfigurationFilePath = this.GetAbsolutePath(clusterConfigurationFilePath);

            try
            {
                this.WriteObject(StringResources.Info_RemovingServiceFabricCluster);
                DeploymentManager.RemoveClusterAsync(
                    clusterConfigurationFilePath,
                    deleteLog ? deleteLog : true).Wait();
                this.WriteObject(StringResources.Info_RemoveServiceFabricClusterSucceeded);
            }
            catch (Exception exception)
            {
                this.WriteObject(StringResources.Error_RemoveServiceFabricClusterFailed);
                this.ThrowTerminatingError(
                    exception,
                    Constants.RemoveNodeConfigurationErrorId,
                    null);
            }
        }

        [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1305:FieldNamesMustNotUseHungarianNotation", Justification = "Reviewed. Suppression is OK here for ipAddressOrFQDN.")]
        protected void AddNode(
            string nodeName,
            string nodeType,
            string ipAddressOrFQDN,
            string upgradeDomain,
            string faultDomain,
            string fabricPackageSourcePath)
        {
            this.ChangeCurrentDirectoryToSession();
            fabricPackageSourcePath = this.GetAbsolutePath(fabricPackageSourcePath);

            DeploymentManager.AddNodeAsync(
                nodeName,
                nodeType,
                ipAddressOrFQDN,
                upgradeDomain,
                faultDomain,
                fabricPackageSourcePath,
                GetClusterConnection().FabricClient).Wait();
        }

        protected void RemoveNode()
        {
            this.ChangeCurrentDirectoryToSession();
            DeploymentManagerInternal.RemoveNodeAsyncInternal(GetClusterConnection().FabricClient).Wait();
        }

        protected void TestConfig(string clusterConfigPath, string oldClusterConfigPath, string fabricPackagePath, int maxPercentFailedNodes)
        {
            this.ChangeCurrentDirectoryToSession();

            if (string.IsNullOrWhiteSpace(fabricPackagePath))
            {
                fabricPackagePath = null;
            }

            clusterConfigPath = this.GetAbsolutePath(clusterConfigPath);
            oldClusterConfigPath = this.GetAbsolutePath(oldClusterConfigPath);
            fabricPackagePath = this.GetAbsolutePath(fabricPackagePath);
            AnalysisSummary summary = DeploymentManagerInternal.BpaAnalyzeClusterSetup(clusterConfigPath, oldClusterConfigPath, fabricPackagePath, maxPercentFailedNodes);

            this.WriteObject(this.FormatOutput(summary));
            if (!summary.Passed)
            {
                this.ThrowTerminatingError(
                    new InvalidOperationException(StringResources.Error_BPABailing),
                    Constants.TestClusterConfigurationErrorId,
                    null);
            }
        }

        protected void CopyClusterPackage(string codePackagePath, string configPackagePath, string imageStoreConnectionString, string relativeCodeDestinationPath, string relativeConfigDestinationPath)
        {
            try
            {
                if (codePackagePath != null)
                {
                    this.UploadToImageStore(imageStoreConnectionString, codePackagePath, relativeCodeDestinationPath);
                    this.WriteObject(StringResources.Info_CopyCodePackageSucceeded);
                }

                if (configPackagePath != null)
                {
                    this.UploadToImageStore(imageStoreConnectionString, configPackagePath, relativeConfigDestinationPath);
                    this.WriteObject(StringResources.Info_CopyConfigPackageSucceeded);
                }
            }
            catch (Exception exception)
            {
                this.ThrowTerminatingError(
                    exception,
                    Constants.CopyClusterPackageErrorId,
                    null);
            }
        }

        protected void RemoveClusterPackage(string codePackagePath, string configPackagePath, string imageStoreConnectionString)
        {
            try
            {
                if (imageStoreConnectionString.StartsWith(Constants.ImageStoreConnectionFabricType, StringComparison.OrdinalIgnoreCase))
                {
                    var clusterConnection = this.SessionState.PSVariable.GetValue(Constants.ClusterConnectionVariableName, null) as IClusterConnection;
                    if (clusterConnection.SecurityCredentials != null && clusterConnection.SecurityCredentials.CredentialType == CredentialType.Claims)
                    {
                        if (codePackagePath != null)
                        {
                            clusterConnection.FabricClient.ImageStore.DeleteContent(imageStoreConnectionString, codePackagePath, this.GetTimeout());
                            this.WriteObject(StringResources.Info_RemoveCodePackageSucceeded);
                        }

                        if (configPackagePath != null)
                        {
                            clusterConnection.FabricClient.ImageStore.DeleteContent(imageStoreConnectionString, configPackagePath, this.GetTimeout());
                            this.WriteObject(StringResources.Info_RemoveConfigPackageSucceeded);
                        }

                        return;
                    }
                }

                IImageStore imageStore = this.CreateImageStore(imageStoreConnectionString, Path.GetTempPath());

                if (codePackagePath != null)
                {
                    imageStore.DeleteContent(codePackagePath, this.GetTimeout());

                    this.WriteObject(StringResources.Info_RemoveCodePackageSucceeded);
                }

                if (configPackagePath != null)
                {
                    imageStore.DeleteContent(configPackagePath, this.GetTimeout());

                    this.WriteObject(StringResources.Info_RemoveConfigPackageSucceeded);
                }
            }
            catch (Exception exception)
            {
                this.ThrowTerminatingError(
                    exception,
                    Constants.RemoveClusterPackageErrorId,
                    null);
            }
        }

        protected void RegisterClusterPackage(string codePackagePath, string configPackagePath)
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.ProvisionFabricAsync(
                    codePackagePath,
                    configPackagePath,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();

                this.WriteObject(StringResources.Info_RegisterClusterPackageSucceeded);
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.RegisterClusterPackageErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected void UnRegisterClusterPackage(string codePackageVersion, string configPackageVersion)
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.UnprovisionFabricAsync(
                    codePackageVersion,
                    configPackageVersion,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();

                this.WriteObject(StringResources.Info_UnregisterClusterPackageSucceeded);
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.UnRegisterClusterPackageErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected void UpgradeCluster(FabricUpgradeDescription upgradeDescription)
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.UpgradeFabricAsync(
                    upgradeDescription,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();

                var currentProgress = this.GetClusterUpgradeProgress();
                upgradeDescription.TargetCodeVersion = currentProgress.TargetCodeVersion;
                upgradeDescription.TargetConfigVersion = currentProgress.TargetConfigVersion;

                this.WriteObject(this.FormatOutput(upgradeDescription));
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.UpgradeClusterErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected void UpdateClusterUpgrade(FabricUpgradeUpdateDescription updateDescription)
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.UpdateFabricUpgradeAsync(
                    updateDescription,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();
                this.WriteObject(this.FormatOutput(updateDescription), true);
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.UpdateClusterUpgradeErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected void RollbackClusterUpgrade()
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.RollbackFabricUpgradeAsync(
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.RollbackClusterUpgradeErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected FabricUpgradeProgress GetClusterUpgradeProgress()
        {
            var clusterConnection = this.GetClusterConnection();

            FabricUpgradeProgress currentProgress = null;
            try
            {
                currentProgress =
                    clusterConnection.GetFabricUpgradeProgressAsync(
                        this.GetTimeout(),
                        this.GetCancellationToken()).Result;
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetClusterUpgradeProgressErrorId,
                        clusterConnection);
                    return true;
                });
            }

            return currentProgress;
        }

        protected void ResumeClusterUpgrade(string upgradeDomainName)
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.MoveNextFabricUpgradeDomainAsync(
                    upgradeDomainName,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();

                this.WriteObject(StringResources.Info_ResumeClusterUpgradeSucceeded);
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.ResumeClusterUpgradeDomainErrorId,
                        clusterConnection);
                    return true;
                });
            }
            catch (Exception exception)
            {
                this.ThrowTerminatingError(
                    exception,
                    Constants.ResumeClusterUpgradeDomainErrorId,
                    null);
            }
        }

        protected void RemoveNodeState(string nodeName)
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.RemoveNodeStateAsync(
                    nodeName,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();

                this.WriteObject(string.Format(CultureInfo.CurrentCulture, StringResources.Info_NodeStateRemoveSucceeded_Formatted, nodeName));
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.RemoveNodeStateErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected void InternalEnableNode(string nodeName)
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.ActivateNodeAsync(
                    nodeName,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();

                this.WriteObject(string.Format(CultureInfo.CurrentCulture, StringResources.Info_EnableNodeSucceeded, nodeName));
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.EnableNodeErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected void InternalDisableNode(string nodeName, NodeDeactivationIntent deactivationIntent)
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.DeactivateNodeAsync(
                    nodeName,
                    deactivationIntent,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();

                this.WriteObject(string.Format(CultureInfo.CurrentCulture, StringResources.Info_DisableNodeSucceeded, nodeName));
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.DisableNodeErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected void InternalRepairAllPartitions()
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.RecoverPartitionsAsync(
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();

                this.WriteObject(string.Format(CultureInfo.CurrentCulture, StringResources.Info_RepairPartitionsSucceeded));
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.RepairPartitionErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected void InternalRepairPartition(Guid partitionId)
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.RecoverPartitionAsync(
                    partitionId,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();

                this.WriteObject(string.Format(CultureInfo.CurrentCulture, StringResources.Info_RepairPartitionSucceeded, partitionId));
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.RepairPartitionErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected void InternalRepairServicePartitions(Uri serviceName)
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.RecoverServicePartitionsAsync(
                    serviceName,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();

                this.WriteObject(string.Format(CultureInfo.CurrentCulture, StringResources.Info_RepairServicePartitionsSucceeded, serviceName));
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.RepairPartitionErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected void InternalRepairSystemPartitions()
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.RecoverSystemPartitionsAsync(
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();

                this.WriteObject(string.Format(CultureInfo.CurrentCulture, StringResources.Info_RepairSystemPartitionsSucceeded));
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.RepairPartitionErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected void InternalResetPartitionLoad(Guid partitionId)
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.ResetPartitionLoadAsync(
                    partitionId,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();

                this.WriteObject(string.Format(CultureInfo.CurrentCulture, StringResources.Info_ResetPartitionLoadSucceeded, partitionId));
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.ResetPartitionLoadErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected void InternalToggleVerboseServicePlacementHealthReporting(bool enabled)
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.ToggleVerboseServicePlacementHealthReportingAsync(
                    enabled,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();

                this.WriteObject(string.Format(CultureInfo.CurrentCulture, StringResources.Info_ToggleVerboseServicePlacementHealthReportingSucceeded, enabled));
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.ToggleVerboseServicePlacementHealthReportingErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected override object FormatOutput(object output)
        {
            if (output is ClusterConnection)
            {
                return this.FormatClusterConnection(output as ClusterConnection);
            }

            return output;
        }       

        protected PSObject ToPSObject(FabricUpgradeDescription upgradeDescription)
        {
            var itemPSObj = new PSObject(upgradeDescription);

            if (upgradeDescription.UpgradePolicyDescription == null)
            {
                return itemPSObj;
            }

            var monitoredPolicy = upgradeDescription.UpgradePolicyDescription as MonitoredRollingFabricUpgradePolicyDescription;
            if (monitoredPolicy != null)
            {
                itemPSObj.Properties.Add(new PSNoteProperty(Constants.UpgradeKindPropertyName, monitoredPolicy.Kind));
                itemPSObj.Properties.Add(new PSNoteProperty(Constants.ForceRestartPropertyName, monitoredPolicy.ForceRestart));
                itemPSObj.Properties.Add(new PSNoteProperty(Constants.UpgradeModePropertyName, monitoredPolicy.UpgradeMode));
                itemPSObj.Properties.Add(new PSNoteProperty(Constants.UpgradeReplicaSetCheckTimeoutPropertyName, monitoredPolicy.UpgradeReplicaSetCheckTimeout));
                itemPSObj.Properties.Add(new PSNoteProperty(Constants.EnableDeltaHealthEvaluationPropertyName, monitoredPolicy.EnableDeltaHealthEvaluation));

                if (monitoredPolicy.MonitoringPolicy != null)
                {
                    itemPSObj.Properties.Add(new PSNoteProperty(Constants.FailureActionPropertyName, monitoredPolicy.MonitoringPolicy.FailureAction));
                    itemPSObj.Properties.Add(new PSNoteProperty(Constants.HealthCheckWaitDurationPropertyName, monitoredPolicy.MonitoringPolicy.HealthCheckWaitDuration));
                    itemPSObj.Properties.Add(new PSNoteProperty(Constants.HealthCheckStableDurationPropertyName, monitoredPolicy.MonitoringPolicy.HealthCheckStableDuration));
                    itemPSObj.Properties.Add(new PSNoteProperty(Constants.HealthCheckRetryTimeoutPropertyName, monitoredPolicy.MonitoringPolicy.HealthCheckRetryTimeout));
                    itemPSObj.Properties.Add(new PSNoteProperty(Constants.UpgradeDomainTimeoutPropertyName, monitoredPolicy.MonitoringPolicy.UpgradeDomainTimeout));
                    itemPSObj.Properties.Add(new PSNoteProperty(Constants.UpgradeTimeoutPropertyName, monitoredPolicy.MonitoringPolicy.UpgradeTimeout));
                }

                if (monitoredPolicy.HealthPolicy != null)
                {
                    this.AddToPSObject(itemPSObj, monitoredPolicy.HealthPolicy);
                }

                if (monitoredPolicy.UpgradeHealthPolicy != null)
                {
                    this.AddToPSObject(itemPSObj, monitoredPolicy.UpgradeHealthPolicy);
                }

                this.AddToPSObject(itemPSObj, monitoredPolicy.ApplicationHealthPolicyMap);
            }
            else
            {
                var policy = upgradeDescription.UpgradePolicyDescription as RollingUpgradePolicyDescription;
                if (policy != null)
                {
                    itemPSObj.Properties.Add(new PSNoteProperty(Constants.UpgradeKindPropertyName, policy.Kind));
                    itemPSObj.Properties.Add(new PSNoteProperty(Constants.ForceRestartPropertyName, policy.ForceRestart));
                    itemPSObj.Properties.Add(new PSNoteProperty(Constants.UpgradeModePropertyName, policy.UpgradeMode));
                    itemPSObj.Properties.Add(new PSNoteProperty(Constants.UpgradeReplicaSetCheckTimeoutPropertyName, policy.UpgradeReplicaSetCheckTimeout));
                }
            }

            return itemPSObj;
        }

        protected void AddToPSObject(PSObject itemPSObj, ClusterHealthPolicy healthPolicy)
        {
            if (healthPolicy == null)
            {
                return;
            }

            itemPSObj.Properties.Add(new PSNoteProperty(Constants.ConsiderWarningAsErrorPropertyName, healthPolicy.ConsiderWarningAsError));
            itemPSObj.Properties.Add(new PSNoteProperty(Constants.MaxPercentUnhealthyApplicationsPropertyName, healthPolicy.MaxPercentUnhealthyApplications));
            itemPSObj.Properties.Add(new PSNoteProperty(Constants.MaxPercentUnhealthyNodesPropertyName, healthPolicy.MaxPercentUnhealthyNodes));

            if (healthPolicy.ApplicationTypeHealthPolicyMap != null)
            {
                var appTypeHealthPolicyMapPSObj = new PSObject(healthPolicy.ApplicationTypeHealthPolicyMap);
                appTypeHealthPolicyMapPSObj.Members.Add(
                    new PSCodeMethod(
                        Constants.ToStringMethodName,
                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));
                itemPSObj.Properties.Add(new PSNoteProperty(Constants.ApplicationTypeHealthPolicyMapPropertyName, appTypeHealthPolicyMapPSObj));
            }
        }

        protected void AddToPSObject(PSObject itemPSObj, ClusterUpgradeHealthPolicy upgradeHealthPolicy)
        {
            if (upgradeHealthPolicy == null)
            {
                return;
            }

            itemPSObj.Properties.Add(new PSNoteProperty(Constants.MaxPercentDeltaUnhealthyNodesPropertyName, upgradeHealthPolicy.MaxPercentDeltaUnhealthyNodes));
            itemPSObj.Properties.Add(new PSNoteProperty(Constants.MaxPercentUpgradeDomainDeltaUnhealthyNodesPropertyName, upgradeHealthPolicy.MaxPercentUpgradeDomainDeltaUnhealthyNodes));
        }
       
        protected void AddToPSObject(PSObject itemPSObj, ApplicationHealthPolicyMap applicationHealthPolicies)
        {
            if (applicationHealthPolicies == null)
            {
                return;
            }

            var appHealthPolicyMapPSObj = new PSObject(applicationHealthPolicies);
            appHealthPolicyMapPSObj.Members.Add(
                new PSCodeMethod(
                    Constants.ToStringMethodName,
                    typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));
            itemPSObj.Properties.Add(new PSNoteProperty(Constants.ApplicationHealthPolicyMapPropertyName, appHealthPolicyMapPSObj));
        }

        private object FormatClusterConnection(ClusterConnection clusterConnection)
        {
            var resultPSObj = new PSObject(clusterConnection);

            var fabricClientSettingsPSObj = new PSObject(clusterConnection.FabricClientSettings);
            fabricClientSettingsPSObj.Members.Add(
                new PSCodeMethod(
                    Constants.ToStringMethodName,
                    typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

            resultPSObj.Properties.Add(
                new PSNoteProperty(
                    Constants.ClusterConnectionFabricClientSettingsPropertyName,
                    fabricClientSettingsPSObj));

            if (clusterConnection.GatewayInformation != null)
            {
                var gatewayInfoPSObj = new PSObject(clusterConnection.GatewayInformation);
                gatewayInfoPSObj.Members.Add(
                    new PSCodeMethod(
                        Constants.ToStringMethodName,
                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                resultPSObj.Properties.Add(
                    new PSNoteProperty(
                        Constants.GatewayInformationPropertyName,
                        gatewayInfoPSObj));
            }

            if (clusterConnection.AzureActiveDirectoryMetadata != null)
            {
                var metadataPSObj = new PSObject(clusterConnection.AzureActiveDirectoryMetadata);
                metadataPSObj.Members.Add(
                    new PSCodeMethod(
                        Constants.ToStringMethodName,
                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                resultPSObj.Properties.Add(
                    new PSNoteProperty(
                        Constants.AzureActiveDirectoryMetadataPropertyName,
                        metadataPSObj));
            }

            return resultPSObj;
        }
    }
}