// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.ComponentModel;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric;
    using System.Fabric.Chaos.DataStructures;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Query;
    using System.Fabric.Repair;
    using System.Fabric.Result;
    using System.Fabric.SecretStore;
    using System.Fabric.Security;
    using System.Fabric.Strings;
    using System.Fabric.Testability.Scenario;
    using System.Globalization;
    using System.Numerics;
    using System.Security.Cryptography.X509Certificates;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Preview.Client;
    using Microsoft.ServiceFabric.Preview.Client.Description;
    using Microsoft.ServiceFabric.Preview.Client.Query;

    public class ClusterConnection : IClusterConnection
    {
        [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1202:ElementsMustBeOrderedByAccess", Justification = "Reviewed. Suppression is OK here for DefaultOperationTimeoutInSeconds")]
        private const double DefaultOperationTimeoutInSeconds = 300.0;
        internal static readonly TimeSpan DefaultOperationTimeout = TimeSpan.FromSeconds(DefaultOperationTimeoutInSeconds);

        private readonly ManualResetEvent claimsMetadataEvent = new ManualResetEvent(false);
        private Exception claimsMetadataException = null;

        public ClusterConnection(
            FabricClientBuilder fabricClientBuilder)
        : this(fabricClientBuilder, false)
        {
        }

        public ClusterConnection(
            FabricClientBuilder fabricClientBuilder,
            bool getMetadata)
        {
            this.FabricClient = fabricClientBuilder.Build();
            this.ConnectionEndpoint = fabricClientBuilder.GetConnectionEndPoint();
            this.SecurityCredentials = fabricClientBuilder.GetSecurityCredentials();

            this.FabricClient.ClientConnected += (o, e) => this.OnClientConnected(e);
            this.FabricClient.ClientDisconnected += (o, e) => this.OnClientDisconnected(e);

            if (getMetadata)
            {
                this.FabricClient.ClaimsRetrieval += (o, e) => this.OnClaimsRetrieval(e);
            }
        }

        public FabricClientSettings FabricClientSettings
        {
            get
            {
                if (FabricClient == null)
                {
                    return null;
                }

                return this.FabricClient.Settings;
            }

            private set
            {
                if (FabricClient == null)
                {
                    return;
                }

                this.FabricClient.UpdateSettings(value);
            }
        }

        public FabricClient FabricClient
        {
            get;
            private set;
        }

        public string[] ConnectionEndpoint
        {
            get;
            set;
        }

        public SecurityCredentials SecurityCredentials
        {
            get;
            private set;
        }

        public GatewayInformation GatewayInformation
        {
            get;
            private set;
        }

        public AzureActiveDirectoryMetadata AzureActiveDirectoryMetadata
        {
            get;
            private set;
        }

        // If the call returns with a failure, see if it's caused by a down node, or a node that doesn't exist, something else.
        // This gives users a more detailed error message.
        public static async Task<TResult> ValidateNodeNameOnFailure<TResult>(string nodeName, FabricClient fabricClient, Func<Task<TResult>> function)
        {
            Exception originalException = null;
            Exception newException = null;

            try
            {
                var result = await function();
                return result;
            }
            catch (FabricException exception)
            {
                if (exception.ErrorCode == FabricErrorCode.InvalidAddress)
                {
                    originalException = exception;
                }
                else
                {
                    throw;
                }
            }

            // Check if the node name passed in belongs to a valid node.
            try
            {
                var result = fabricClient.QueryManager.GetNodeListAsync(nodeName).Result;

                if (result.Count == 0)
                {
                    newException = new AggregateException(
                        string.Format(CultureInfo.InvariantCulture, StringResources.Error_PowerShell_InvalidNodeName, nodeName),
                        originalException);
                }
                else if (result[0].NodeStatus != NodeStatus.Up)
                {
                    newException = new AggregateException(
                        string.Format(CultureInfo.InvariantCulture, StringResources.Error_PowerShell_NodeNotUp, nodeName),
                        originalException);
                }
                else
                {
                    newException = originalException;
                }
            }
            catch (Exception)
            {
                throw originalException;
            }

            throw newException;
        }

        public void InitializeClaimsMetadata(TimeSpan timeout)
        {
            TimeSpan innerTimeout = TimeSpan.FromSeconds(timeout.TotalSeconds / 2);

            // Trigger connection asynchronously. Do not care
            // about result of operation.
            this.NameExistsAsync(
                new Uri(Constants.DummyUri),
                innerTimeout,
                CancellationToken.None).ContinueWith(t =>
                {
                    try
                    {
                        t.Wait();
                    }
                    catch (Exception ex)
                    {
                        this.claimsMetadataException = ex;
                        this.claimsMetadataEvent.Set();
                    }
                });

            // Return as soon as we have the metadata. Do not need to
            // wait for actual authorization result from cluster.
            this.claimsMetadataEvent.WaitOne(timeout);

            if (this.AzureActiveDirectoryMetadata == null && this.claimsMetadataException != null)
            {
                throw this.claimsMetadataException;
            }
        }

        public Task<bool> NameExistsAsync(Uri name, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.PropertyManager.NameExistsAsync(name, timeout, cancellationToken);
        }

        public Task<ApplicationList> GetApplicationPagedListAsync(
            ApplicationQueryDescription applicationQueryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.QueryManager.GetApplicationPagedListAsync(
                       applicationQueryDescription, timeout, cancellationToken);
        }

        public Task<ComposeDeploymentStatusList> GetComposeDeploymentStatusPagedListAsync(
            ComposeDeploymentStatusQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.QueryManager.GetComposeDeploymentStatusPagedListAsync(
                queryDescription, timeout, cancellationToken);
        }

        public Task<ApplicationTypeList> GetApplicationTypeListAsync(
           string applicationTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.QueryManager.GetApplicationTypeListAsync(
                       applicationTypeNameFilter, timeout, cancellationToken);
        }

        public Task<ApplicationTypePagedList> GetApplicationTypePagedListAsync(
            PagedApplicationTypeQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.QueryManager.GetApplicationTypePagedListAsync(
                       queryDescription, timeout, cancellationToken);
        }

        public Task<DeployedApplicationList> GetDeployedApplicationListAsync(
            string nodeName, Uri applicationNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return ValidateNodeNameOnFailure<DeployedApplicationList>(
                       nodeName,
                       this.FabricClient,
                       async () =>
            {
                return await this.FabricClient.QueryManager.GetDeployedApplicationListAsync(
                    nodeName,
                    applicationNameFilter,
                    timeout,
                    cancellationToken);
            });
        }

        public Task<DeployedApplicationPagedList> GetDeployedApplicationPagedListAsync(
            PagedDeployedApplicationQueryDescription queryDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return ValidateNodeNameOnFailure<DeployedApplicationPagedList>(
               queryDescription.NodeName,
               this.FabricClient,
               async () =>
               {
                    return await this.FabricClient.QueryManager.GetDeployedApplicationPagedListAsync(
                        queryDescription,
                        timeout,
                        cancellationToken);
               });
        }

        public Task<DeployedCodePackageList> GetDeployedCodePackageListAsync(
            string nodeName, Uri applicationName, string serviceManifestNameFilter, string codePackageNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return ValidateNodeNameOnFailure<DeployedCodePackageList>(
                       nodeName,
                       this.FabricClient,
                       async () =>
            {
                return await this.FabricClient.QueryManager.GetDeployedCodePackageListAsync(
                    nodeName,
                    applicationName,
                    serviceManifestNameFilter,
                    codePackageNameFilter,
                    timeout,
                    cancellationToken);
            });
        }

        public Task<DeployedServiceReplicaList> GetDeployedReplicaListAsync(
            string nodeName, Uri applicationName, string serviceManifestNameFilter, Guid? partitionIdFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return ValidateNodeNameOnFailure<DeployedServiceReplicaList>(
                       nodeName,
                       this.FabricClient,
                       async () =>
            {
                return await this.FabricClient.QueryManager.GetDeployedReplicaListAsync(
                    nodeName,
                    applicationName,
                    serviceManifestNameFilter,
                    partitionIdFilter,
                    timeout,
                    cancellationToken);
            });
        }

        public Task<DeployedServiceReplicaDetail> GetDeployedReplicaDetailAsync(
            string nodeName, Guid partitionId, long replicaId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return ValidateNodeNameOnFailure<DeployedServiceReplicaDetail>(
                       nodeName,
                       this.FabricClient,
                       async () =>
            {
                return await this.FabricClient.QueryManager.GetDeployedReplicaDetailAsync(
                    nodeName,
                    partitionId,
                    replicaId,
                    timeout,
                    cancellationToken);
            });
        }

        public Task<ClusterLoadInformation> GetClusterLoadInformationAsync(
            TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.QueryManager.GetClusterLoadInformationAsync(timeout, cancellationToken);
        }

        public Task<NodeLoadInformation> GetNodeLoadInformationAsync(
            string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.QueryManager.GetNodeLoadInformationAsync(nodeName, timeout, cancellationToken);
        }

        public Task<ApplicationLoadInformation> GetApplicationLoadInformationAsync(
            string applicationName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.QueryManager.GetApplicationLoadInformationAsync(applicationName, timeout, cancellationToken);
        }

        public Task<ServiceNameResult> GetServiceNameAsync(
            Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.QueryManager.GetServiceNameAsync(partitionId, timeout, cancellationToken);
        }

        public Task<ApplicationNameResult> GetApplicationNameAsync(
            Uri serviceName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.QueryManager.GetApplicationNameAsync(serviceName, timeout, cancellationToken);
        }

        public Task<DeployedServicePackageList> GetDeployedServicePackageListAsync(
            string nodeName, Uri applicationName, string serviceManifestNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return ValidateNodeNameOnFailure<DeployedServicePackageList>(
                       nodeName,
                       this.FabricClient,
                       async () =>
            {
                return await this.FabricClient.QueryManager.GetDeployedServicePackageListAsync(
                    nodeName,
                    applicationName,
                    serviceManifestNameFilter,
                    timeout,
                    cancellationToken);
            });
        }

        public Task<DeployedServiceTypeList> GetDeployedServiceTypeListAsync(
            string nodeName, Uri applicationName, string serviceManifestNameFilter, string serviceTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return ValidateNodeNameOnFailure<DeployedServiceTypeList>(
                       nodeName,
                       this.FabricClient,
                       async () =>
            {
                return await this.FabricClient.QueryManager.GetDeployedServiceTypeListAsync(
                    nodeName,
                    applicationName,
                    serviceManifestNameFilter,
                    serviceTypeNameFilter,
                    timeout,
                    cancellationToken);
            });
        }

        public Task<NodeList> GetNodeListAsync(
            NodeQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
                return this.FabricClient.QueryManager.GetNodePagedListAsync(
                    queryDescription,
                    timeout,
                    cancellationToken);
        }

        public Task<ServicePartitionList> GetPartitionListAsync(
            Uri serviceName, Guid? partitionIdFilter, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.QueryManager.GetPartitionListAsync(
                       serviceName, partitionIdFilter, continuationToken, timeout, cancellationToken);
        }

        public Task<ServicePartitionList> GetPartitionAsync(
            Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.QueryManager.GetPartitionAsync(
                       partitionId, timeout, cancellationToken);
        }

        public Task<PartitionLoadInformation> GetPartitionLoadInformationAsync(
            Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.QueryManager.GetPartitionLoadInformationAsync(
                       partitionId, timeout, cancellationToken);
        }

        public Task<ReplicaLoadInformation> GetReplicaLoadInformationAsync(
            Guid partitionId, long replicaOrInstanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.QueryManager.GetReplicaLoadInformationAsync(
                       partitionId, replicaOrInstanceId, timeout, cancellationToken);
        }

        public Task<UnplacedReplicaInformation> GetUnplacedReplicaInformationAsync(
            string serviceName, Guid partitionId, bool onlyQueryPrimaries, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.QueryManager.GetUnplacedReplicaInformationAsync(
                       serviceName, partitionId, onlyQueryPrimaries, timeout, cancellationToken);
        }

        public Task<ServiceReplicaList> GetReplicaListAsync(
            Guid partitionId, long replicaIdOrInstanceIdFilter, ServiceReplicaStatusFilter replicaStatusFilter, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.QueryManager.GetReplicaListAsync(
                       partitionId, replicaIdOrInstanceIdFilter, replicaStatusFilter, continuationToken, timeout, cancellationToken);
        }

        public Task<ServiceList> GetServicePagedListAsync(
            ServiceQueryDescription serviceQueryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.QueryManager.GetServicePagedListAsync(
                       serviceQueryDescription, timeout, cancellationToken);
        }

        public Task<ServiceGroupMemberList> GetServiceGroupMemberListAsync(Uri applicationName, Uri serviceNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.QueryManager.GetServiceGroupMemberListAsync(
                       applicationName, serviceNameFilter, timeout, cancellationToken);
        }

        public Task<ServiceTypeList> GetServiceTypeListAsync(
            string applicationTypeName, string applicationTypeVersion, string serviceTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.QueryManager.GetServiceTypeListAsync(
                       applicationTypeName, applicationTypeVersion, serviceTypeNameFilter, timeout, cancellationToken);
        }

        public Task<ServiceGroupMemberTypeList> GetServiceGroupMemberTypeListAsync(
            string applicationTypeName, string applicationTypeVersion, string serviceGroupTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.QueryManager.GetServiceGroupMemberTypeListAsync(
                       applicationTypeName, applicationTypeVersion, serviceGroupTypeNameFilter, timeout, cancellationToken);
        }

        public Task<ProvisionedFabricCodeVersionList> GetProvisionedFabricCodeVersionListAsync(string codeVersionFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.QueryManager.GetProvisionedFabricCodeVersionListAsync(
                       codeVersionFilter, timeout, cancellationToken);
        }

        public Task<ProvisionedFabricConfigVersionList> GetProvisionedFabricConfigVersionListAsync(string configVersionFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.QueryManager.GetProvisionedFabricConfigVersionListAsync(
                       configVersionFilter, timeout, cancellationToken);
        }

        public Task CreateApplicationAsync(
            ApplicationDescription applicationDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ApplicationManager.CreateApplicationAsync(
                       applicationDescription, timeout, cancellationToken);
        }

        public Task UpdateApplicationAsync(ApplicationUpdateDescription updateDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ApplicationManager.UpdateApplicationAsync(updateDescription, timeout, cancellationToken);
        }

        public Task DeleteApplicationAsync(DeleteApplicationDescription deleteDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ApplicationManager.DeleteApplicationAsync(
                       deleteDescription, timeout, cancellationToken);
        }

        public Task<string> GetApplicationManifestAsync(
            string applicationTypeName, string applicationTypeVersion, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ApplicationManager.GetApplicationManifestAsync(
                       applicationTypeName, applicationTypeVersion, timeout, cancellationToken);
        }

        public Task<ApplicationUpgradeProgress> GetApplicationUpgradeProgressAsync(Uri applicationName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ApplicationManager.GetApplicationUpgradeProgressAsync(
                       applicationName, timeout, cancellationToken);
        }

        public Task<ComposeDeploymentUpgradeProgress> GetComposeDeploymentUpgradeProgressAsync(string deploymentName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ComposeDeploymentManager.GetComposeDeploymentUpgradeProgressAsync(
                        deploymentName, timeout, cancellationToken);
        }

        public Task MoveNextApplicationUpgradeDomainAsync(
            Uri applicationName, string nextUpgradeDomain, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ApplicationManager.MoveNextApplicationUpgradeDomainAsync(
                       applicationName, nextUpgradeDomain, timeout, cancellationToken);
        }

        public Task ProvisionApplicationAsync(ProvisionApplicationTypeDescriptionBase description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ApplicationManager.ProvisionApplicationAsync(
                       description, timeout, cancellationToken);
        }

        public Task UnprovisionApplicationAsync(
            UnprovisionApplicationTypeDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ApplicationManager.UnprovisionApplicationAsync(
                       description, timeout, cancellationToken);
        }

        public Task UpgradeApplicationAsync(
            ApplicationUpgradeDescription upgradeDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ApplicationManager.UpgradeApplicationAsync(
                       upgradeDescription, timeout, cancellationToken);
        }

        public Task UpdateApplicationUpgradeAsync(
            ApplicationUpgradeUpdateDescription updateDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ApplicationManager.UpdateApplicationUpgradeAsync(
                       updateDescription, timeout, cancellationToken);
        }

        public Task RollbackApplicationUpgradeAsync(
            Uri applicationName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ApplicationManager.RollbackApplicationUpgradeAsync(
                       applicationName, timeout, cancellationToken);
        }

        public Task CreateComposeDeploymentAsync(
            ComposeDeploymentDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ComposeDeploymentManager.CreateComposeDeploymentAsync(description, timeout, cancellationToken);
        }

        public Task DeleteComposeDeploymentAsync(
            DeleteComposeDeploymentDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ComposeDeploymentManager.DeleteComposeDeploymentAsync(
                       description,
                       timeout,
                       cancellationToken);
        }

        public Task UpgradeComposeDeploymentAsync(
            ComposeDeploymentUpgradeDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ComposeDeploymentManager.UpgradeComposeDeploymentAsync(
                       description,
                       timeout,
                       cancellationToken);
        }

        public Task RollbackComposeDeploymentUpgradeAsync(
            ComposeDeploymentRollbackDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ComposeDeploymentManager.RollbackComposeDeploymentUpgradeAsync(
                       description,
                       timeout,
                       cancellationToken);
        }

        public Task ActivateNodeAsync(string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ClusterManager.ActivateNodeAsync(nodeName, timeout, cancellationToken);
        }

        public Task DeactivateNodeAsync(
            string nodeName, NodeDeactivationIntent deactivationIntent, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ClusterManager.DeactivateNodeAsync(
                       nodeName, deactivationIntent, timeout, cancellationToken);
        }

        public Task RecoverPartitionsAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ClusterManager.RecoverPartitionsAsync(timeout, cancellationToken);
        }

        public Task RecoverPartitionAsync(Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ClusterManager.RecoverPartitionAsync(partitionId, timeout, cancellationToken);
        }

        public Task RecoverServicePartitionsAsync(Uri serviceName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ClusterManager.RecoverServicePartitionsAsync(serviceName, timeout, cancellationToken);
        }

        public Task RecoverSystemPartitionsAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ClusterManager.RecoverSystemPartitionsAsync(timeout, cancellationToken);
        }

        public Task ResetPartitionLoadAsync(Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ClusterManager.ResetPartitionLoadAsync(partitionId, timeout, cancellationToken);
        }

        public Task ToggleVerboseServicePlacementHealthReportingAsync(bool enabled, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ClusterManager.ToggleVerboseServicePlacementHealthReportingAsync(enabled, timeout, cancellationToken);
        }

        public Task<RemoveReplicaResult> RemoveReplicaAsync(
            string nodeName,
            Guid partitionId,
            long replicaId,
            CompletionMode completionMode,
            bool forceRemove,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.FaultManager.RemoveReplicaAsync(
                       nodeName,
                       partitionId,
                       replicaId,
                       completionMode,
                       forceRemove,
                       timeout.TotalSeconds,
                       cancellationToken);
        }

        public Task<RemoveReplicaResult> RemoveReplicaAsync(
            ReplicaSelector replicaSelector,
            int? timeoutSec,
            CompletionMode completionMode,
            bool forceRemove,
            CancellationToken cancellationToken)
        {
            TimeSpan timeout = timeoutSec.HasValue ? TimeSpan.FromSeconds((double)timeoutSec.Value) : DefaultOperationTimeout;

            return this.FabricClient.FaultManager.RemoveReplicaAsync(
                       replicaSelector,
                       completionMode,
                       forceRemove,
                       timeout,
                       cancellationToken);
        }

        public Task<RestartReplicaResult> RestartReplicaAsync(
            string nodeName,
            Guid partitionId,
            long replicaId,
            CompletionMode completionMode,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.FaultManager.RestartReplicaAsync(
                       nodeName,
                       partitionId,
                       replicaId,
                       completionMode,
                       timeout.TotalSeconds,
                       cancellationToken);
        }

        public Task<RestartReplicaResult> RestartReplicaAsync(
            ReplicaSelector replicaSelector,
            int? timeoutSec,
            CompletionMode completionMode,
            CancellationToken cancellationToken)
        {
            TimeSpan timeout = timeoutSec.HasValue ? TimeSpan.FromSeconds((double)timeoutSec.Value) : DefaultOperationTimeout;

            return this.FabricClient.FaultManager.RestartReplicaAsync(
                       replicaSelector,
                       completionMode,
                       timeout,
                       cancellationToken);
        }

        public Task<string> GetClusterManifestAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetClusterManifestAsync(new ClusterManifestQueryDescription(), timeout, cancellationToken);
        }

        public Task<string> GetClusterManifestAsync(ClusterManifestQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ClusterManager.GetClusterManifestAsync(queryDescription, timeout, cancellationToken);
        }

        public Task<FabricUpgradeProgress> GetFabricUpgradeProgressAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ClusterManager.GetFabricUpgradeProgressAsync(timeout, cancellationToken);
        }

        public Task MoveNextFabricUpgradeDomainAsync(
            string nextUpgradeDomain, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ClusterManager.MoveNextFabricUpgradeDomainAsync(
                       nextUpgradeDomain, timeout, cancellationToken);
        }

        public Task ProvisionFabricAsync(
            string mspFilePath, string clusterManifestFilePath, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ClusterManager.ProvisionFabricAsync(
                       mspFilePath, clusterManifestFilePath, timeout, cancellationToken);
        }

        public Task RemoveNodeStateAsync(string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ClusterManager.RemoveNodeStateAsync(
                       nodeName, timeout, cancellationToken);
        }

        public Task UnprovisionFabricAsync(
            string codeVersion, string configVersion, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ClusterManager.UnprovisionFabricAsync(
                       codeVersion, configVersion, timeout, cancellationToken);
        }

        public Task UpgradeFabricAsync(
            FabricUpgradeDescription upgradeDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ClusterManager.UpgradeFabricAsync(
                       upgradeDescription, timeout, cancellationToken);
        }

        public Task UpdateFabricUpgradeAsync(
            FabricUpgradeUpdateDescription updateDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ClusterManager.UpdateFabricUpgradeAsync(
                       updateDescription, timeout, cancellationToken);
        }

        public Task RollbackFabricUpgradeAsync(
            TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ClusterManager.RollbackFabricUpgradeAsync(
                       timeout, cancellationToken);
        }

        public Task CreateServiceAsync(ServiceDescription serviceDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ServiceManager.CreateServiceAsync(
                       serviceDescription, timeout, cancellationToken);
        }

        public Task CreateServiceFromTemplateAsync(
            Uri applicationName,
            Uri serviceName,
            string serviceDnsName,
            string serviceTypeName,
            ServicePackageActivationMode servicePackageActivationMode,
            byte[] initializationData,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.ServiceManager.CreateServiceFromTemplateAsync(
                new ServiceFromTemplateDescription(
                    applicationName,
                    serviceName,
                    serviceDnsName,
                    serviceTypeName,
                    servicePackageActivationMode,
                    initializationData),
                timeout,
                cancellationToken);
        }

        public Task DeleteServiceAsync(DeleteServiceDescription deleteDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ServiceManager.DeleteServiceAsync(
                       deleteDescription, timeout, cancellationToken);
        }

        public Task UpdateServiceAsync(
            Uri name, ServiceUpdateDescription serviceUpdateDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ServiceManager.UpdateServiceAsync(
                       name, serviceUpdateDescription, timeout, cancellationToken);
        }

        public Task<string> GetServiceManifestAsync(
            string applicationTypeName,
            string applicationTypeVersion,
            string serviceManifestName,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.ServiceManager.GetServiceManifestAsync(
                       applicationTypeName, applicationTypeVersion, serviceManifestName, timeout, cancellationToken);
        }

        public Task<ServiceDescription> GetServiceDescriptionAsync(
            Uri serviceName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ServiceManager.GetServiceDescriptionAsync(
                       serviceName, timeout, cancellationToken);
        }

        public Task<ResolvedServicePartition> ResolveServicePartitionAsync(
            Uri serviceName,
            ResolvedServicePartition previousResult,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.ServiceManager.ResolveServicePartitionAsync(
                       serviceName, previousResult, timeout, cancellationToken);
        }

        public Task<ResolvedServicePartition> ResolveServicePartitionAsync(
            Uri serviceName,
            long partitionKey,
            ResolvedServicePartition previousResult,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.ServiceManager.ResolveServicePartitionAsync(
                       serviceName, partitionKey, previousResult, timeout, cancellationToken);
        }

        public Task<ResolvedServicePartition> ResolveServicePartitionAsync(
            Uri serviceName,
            string partitionKey,
            ResolvedServicePartition previousResult,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.ServiceManager.ResolveServicePartitionAsync(
                       serviceName, partitionKey, previousResult, timeout, cancellationToken);
        }

        public Task CreateServiceGroupAsync(
            ServiceGroupDescription serviceGroupDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ServiceGroupManager.CreateServiceGroupAsync(
                       serviceGroupDescription, timeout, cancellationToken);
        }

        public Task CreateServiceGroupFromTemplateAsync(
            Uri applicationName,
            Uri serviceName,
            string serviceTypeName,
            ServicePackageActivationMode servicePackageActivationMode,
            byte[] initializationData,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.ServiceGroupManager.CreateServiceGroupFromTemplateAsync(
                new ServiceGroupFromTemplateDescription(
                    applicationName,
                    serviceName,
                    serviceTypeName,
                    servicePackageActivationMode,
                    initializationData),
                timeout,
                cancellationToken);
        }

        public Task UpdateServiceGroupAsync(
            Uri name, ServiceGroupUpdateDescription updateDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ServiceGroupManager.UpdateServiceGroupAsync(
                       name, updateDescription, timeout, cancellationToken);
        }

        public Task DeleteServiceGroupAsync(
            Uri name, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ServiceGroupManager.DeleteServiceGroupAsync(
                       name, timeout, cancellationToken);
        }

        public Task<ServiceGroupDescription> GetServiceGroupDescriptionAsync(
            Uri name, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.ServiceGroupManager.GetServiceGroupDescriptionAsync(
                       name, timeout, cancellationToken);
        }

        public Task<NodeHealth> GetNodeHealthAsync(
            NodeHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.HealthManager.GetNodeHealthAsync(
                       queryDescription, timeout, cancellationToken);
        }

        public Task<ApplicationHealth> GetApplicationHealthAsync(
            ApplicationHealthQueryDescription queryDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.HealthManager.GetApplicationHealthAsync(
                       queryDescription, timeout, cancellationToken);
        }

        public Task<ServiceHealth> GetServiceHealthAsync(
            ServiceHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.HealthManager.GetServiceHealthAsync(
                       queryDescription, timeout, cancellationToken);
        }

        public Task<PartitionHealth> GetPartitionHealthAsync(
            PartitionHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.HealthManager.GetPartitionHealthAsync(
                       queryDescription,
                       timeout,
                       cancellationToken);
        }

        public Task<ReplicaHealth> GetReplicaHealthAsync(
            ReplicaHealthQueryDescription queryDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.HealthManager.GetReplicaHealthAsync(
                       queryDescription,
                       timeout,
                       cancellationToken);
        }

        public Task<DeployedApplicationHealth> GetDeployedApplicationHealthAsync(
            DeployedApplicationHealthQueryDescription queryDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return ValidateNodeNameOnFailure<DeployedApplicationHealth>(
                       queryDescription.NodeName,
                       this.FabricClient,
                       async () =>
            {
                return await this.FabricClient.HealthManager.GetDeployedApplicationHealthAsync(
                    queryDescription,
                    timeout,
                    cancellationToken);
            });
        }

        public Task<DeployedServicePackageHealth> GetDeployedServicePackageHealthAsync(
            DeployedServicePackageHealthQueryDescription queryDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return ValidateNodeNameOnFailure<DeployedServicePackageHealth>(
                       queryDescription.NodeName,
                       this.FabricClient,
                       async () =>
            {
                return await this.FabricClient.HealthManager.GetDeployedServicePackageHealthAsync(
                    queryDescription,
                    timeout,
                    cancellationToken);
            });
        }

        public Task<ClusterHealth> GetClusterHealthAsync(
            ClusterHealthQueryDescription queryDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.HealthManager.GetClusterHealthAsync(
                       queryDescription, timeout, cancellationToken);
        }

        public Task<ClusterHealthChunk> GetClusterHealthChunkAsync(
            ClusterHealthChunkQueryDescription queryDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.HealthManager.GetClusterHealthChunkAsync(
                       queryDescription, timeout, cancellationToken);
        }

        public void ReportHealth(HealthReport healthReport, HealthReportSendOptions sendOptions)
        {
            this.FabricClient.HealthManager.ReportHealth(healthReport, sendOptions);
        }

        public Task DeployServicePackageToNodeAsync(
            string applicationTypeName,
            string applicationTypeVersion,
            string serviceManifestName,
            PackageSharingPolicyList sharingPolicies,
            string nodeName,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.ApplicationManager.DeployServicePackageToNode(
                       applicationTypeName,
                       applicationTypeVersion,
                       serviceManifestName,
                       sharingPolicies,
                       nodeName,
                       timeout,
                       cancellationToken);
        }

        public Task<string> InvokeInfrastructureCommandAsync(Uri serviceName, string command, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.InfrastructureManager.InvokeInfrastructureCommandAsync(serviceName, command, timeout, cancellationToken);
        }

        public Task<string> InvokeInfrastructureQueryAsync(Uri serviceName, string command, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.InfrastructureManager.InvokeInfrastructureQueryAsync(serviceName, command, timeout, cancellationToken);
        }

        public Task<long> CreateRepairTaskAsync(RepairTask repairTask, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.RepairManager.CreateRepairTaskAsync(
                       repairTask,
                       timeout,
                       cancellationToken);
        }

        public Task<long> CancelRepairTaskAsync(string repairTaskId, long version, bool requestAbort, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.RepairManager.CancelRepairTaskAsync(
                       repairTaskId,
                       version,
                       requestAbort,
                       timeout,
                       cancellationToken);
        }

        public Task<long> ForceApproveRepairTaskAsync(string repairTaskId, long version, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.RepairManager.ForceApproveRepairTaskAsync(
                       repairTaskId,
                       version,
                       timeout,
                       cancellationToken);
        }

        public Task DeleteRepairTaskAsync(string repairTaskId, long version, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.RepairManager.DeleteRepairTaskAsync(
                       repairTaskId,
                       version,
                       timeout,
                       cancellationToken);
        }

        public Task<long> UpdateRepairExecutionStateAsync(RepairTask repairTask, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.FabricClient.RepairManager.UpdateRepairExecutionStateAsync(
                       repairTask,
                       timeout,
                       cancellationToken);
        }

        public Task<RepairTaskList> GetRepairTaskListAsync(
            string taskIdFilter,
            RepairTaskStateFilter stateFilter,
            string executorFilter,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.RepairManager.GetRepairTaskListAsync(
                       taskIdFilter,
                       stateFilter,
                       executorFilter,
                       timeout,
                       cancellationToken);
        }

        public Task<long> UpdateRepairTaskHealthPolicyAsync(
            string repairTaskId,
            long version,
            bool? performPreparingHealthCheck,
            bool? performRestoringHealthCheck,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.RepairManager.UpdateRepairTaskHealthPolicyAsync(
                       repairTaskId,
                       version,
                       performPreparingHealthCheck,
                       performRestoringHealthCheck,
                       timeout,
                       cancellationToken);
        }

        public Task CleanTestStateAsync(int? operationTimeoutSec, CancellationToken token)
        {
            TimeSpan timeout = operationTimeoutSec.HasValue ? TimeSpan.FromSeconds(operationTimeoutSec.Value) : DefaultOperationTimeout;

            return this.FabricClient.TestManager.CleanTestStateAsync(
                       timeout,
                       token);
        }

        public Task<RestartDeployedCodePackageResult> RestartDeployedCodePackageAsync(
            ReplicaSelector replicaSelector,
            Uri applicationName,
            int? timeout,
            CompletionMode completionMode,
            CancellationToken cancellationToken)
        {
            TimeSpan timeoutVal = timeout.HasValue ? TimeSpan.FromSeconds(timeout.Value) : DefaultOperationTimeout;

            return this.FabricClient.FaultManager.RestartDeployedCodePackageAsync(
                       applicationName,
                       replicaSelector,
                       completionMode,
                       timeoutVal,
                       cancellationToken);
        }

        public Task<RestartDeployedCodePackageResult> RestartDeployedCodePackageAsync(
            string nodeName,
            Uri applicationName,
            string serviceManifestName,
            string servicePackageActivationId,
            string codePackageName,
            long codePackageInstanceId,
            int? operationTimeout,
            CompletionMode completionMode,
            CancellationToken cancellationToken)
        {
            TimeSpan timeout = operationTimeout.HasValue ? TimeSpan.FromSeconds(operationTimeout.Value) : DefaultOperationTimeout;

            return this.FabricClient.FaultManager.RestartDeployedCodePackageAsync(
                       nodeName,
                       applicationName,
                       serviceManifestName,
                       servicePackageActivationId,
                       codePackageName,
                       codePackageInstanceId,
                       completionMode,
                       timeout,
                       cancellationToken);
        }

        [Obsolete("Deprecated")]
        public async Task InvokeChaosTestScenarioAsync(ChaosTestScenarioParameters chaosScenarioParameters, CancellationToken token)
        {
            ChaosTestScenario chaosTestScenario = new ChaosTestScenario(this.FabricClient, chaosScenarioParameters);

            chaosTestScenario.ProgressChanged += this.TestScenarioProgressChanged;

            await chaosTestScenario.ExecuteAsync(token);
        }

        [Obsolete("Deprecated.  Please use Chaos instead https://docs.microsoft.com/azure/service-fabric/service-fabric-controlled-chaos")]
        public async Task InvokeFailoverTestScenarioAsync(
            Uri serviceName,
            PartitionSelector partitionSelector,
            uint timeToRunInMinutes,
            uint maxServiceStabilizationTimeoutInSeconds,
            int? waitTimeBetweenFaultsInSeconds,
            int? operationTimeoutSec,
            CancellationToken token)
        {
            FailoverTestScenarioParameters parameters = new FailoverTestScenarioParameters(
                partitionSelector,
                TimeSpan.FromMinutes(timeToRunInMinutes),
                TimeSpan.FromSeconds(maxServiceStabilizationTimeoutInSeconds));

            if (waitTimeBetweenFaultsInSeconds.HasValue)
            {
                parameters.WaitTimeBetweenFaults = TimeSpan.FromSeconds(waitTimeBetweenFaultsInSeconds.Value);
            }

            if (operationTimeoutSec.HasValue)
            {
                parameters.OperationTimeout = TimeSpan.FromSeconds(operationTimeoutSec.Value);
            }

            FailoverTestScenario testScenario = new FailoverTestScenario(this.FabricClient, parameters);
            testScenario.ProgressChanged += this.TestScenarioProgressChanged;

            await testScenario.ExecuteAsync(token);
        }

        public Task<InvokeDataLossResult> InvokeDataLossAsync(
            PartitionSelector partitionSelector,
            DataLossMode datalossMode,
            int? operationTimeoutSec,
            CancellationToken token)
        {
            TimeSpan timeout = operationTimeoutSec.HasValue ? TimeSpan.FromSeconds(operationTimeoutSec.Value) : DefaultOperationTimeout;

#pragma warning disable 618
            return this.FabricClient.TestManager.InvokeDataLossAsync(
                       partitionSelector,
                       datalossMode,
                       timeout,
                       token);
#pragma warning restore 618
        }

        public Task InvokeDataLossAsync(
            Guid operationId,
            PartitionSelector partitionSelector,
            DataLossMode dataLossMode,
            int? operationTimeoutSec,
            CancellationToken cancellationToken)
        {
            TimeSpan timeout = operationTimeoutSec.HasValue ? TimeSpan.FromSeconds(operationTimeoutSec.Value) : DefaultOperationTimeout;

            return this.FabricClient.TestManager.StartPartitionDataLossAsync(
                       operationId,
                       partitionSelector,
                       dataLossMode,
                       timeout,
                       cancellationToken);
        }

        public Task<PartitionDataLossProgress> GetInvokeDataLossProgressAsync(
            Guid operationId,
            TimeSpan requestTimeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.TestManager.GetPartitionDataLossProgressAsync(
                       operationId,
                       requestTimeout,
                       cancellationToken);
        }

        public Task<InvokeQuorumLossResult> InvokeQuorumLossAsync(
            PartitionSelector partitionSelector,
            QuorumLossMode quorumlossMode,
            int quorumlossDuration,
            int? operationTimeout,
            CancellationToken token)
        {
            TimeSpan timeout = operationTimeout.HasValue ? TimeSpan.FromSeconds(operationTimeout.Value) : DefaultOperationTimeout;

#pragma warning disable 618
            return this.FabricClient.TestManager.InvokeQuorumLossAsync(
                       partitionSelector,
                       quorumlossMode,
                       TimeSpan.FromSeconds(quorumlossDuration),
                       timeout,
                       token);
#pragma warning restore 618
        }

        public Task InvokeQuorumLossAsync(
            Guid operationId,
            PartitionSelector partitionSelector,
            QuorumLossMode quorumlossMode,
            int quorumlossDuration,
            int? operationTimeout,
            CancellationToken cancellationToken)
        {
            TimeSpan timeout = operationTimeout.HasValue ? TimeSpan.FromSeconds(operationTimeout.Value) : DefaultOperationTimeout;

            return this.FabricClient.TestManager.StartPartitionQuorumLossAsync(
                       operationId,
                       partitionSelector,
                       quorumlossMode,
                       TimeSpan.FromSeconds(quorumlossDuration),
                       timeout,
                       cancellationToken);
        }

        public Task<PartitionQuorumLossProgress> GetInvokeQuorumLossProgressAsync(
            Guid operationId,
            TimeSpan requestTimeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.TestManager.GetPartitionQuorumLossProgressAsync(
                       operationId,
                       requestTimeout,
                       cancellationToken);
        }

        public Task<MovePrimaryResult> MovePrimaryReplicaAsync(
            PartitionSelector partitionSelector,
            int? timeoutSec,
            CancellationToken cancellationToken)
        {
            TimeSpan timeout = timeoutSec.HasValue ? TimeSpan.FromSeconds(timeoutSec.Value) : DefaultOperationTimeout;

            return this.FabricClient.FaultManager.MovePrimaryAsync(
                       partitionSelector,
                       timeout,
                       cancellationToken);
        }

        public Task<MovePrimaryResult> MovePrimaryReplicaAsync(
            string nodeName,
            PartitionSelector partitionSelector,
            int? timeoutSec,
            CancellationToken cancellationToken)
        {
            TimeSpan timeout = timeoutSec.HasValue ? TimeSpan.FromSeconds(timeoutSec.Value) : DefaultOperationTimeout;

            return this.FabricClient.FaultManager.MovePrimaryAsync(
                       nodeName,
                       partitionSelector,
                       timeout,
                       cancellationToken);
        }

        public Task<MovePrimaryResult> MovePrimaryReplicaAsync(
            PartitionSelector partitionSelector,
            bool ignoreConstraints,
            int? timeoutSec,
            CancellationToken cancellationToken)
        {
            TimeSpan timeout = timeoutSec.HasValue ? TimeSpan.FromSeconds(timeoutSec.Value) : DefaultOperationTimeout;

            return this.FabricClient.FaultManager.MovePrimaryAsync(
                       partitionSelector,
                       ignoreConstraints,
                       timeout,
                       cancellationToken);
        }

        public Task<MovePrimaryResult> MovePrimaryReplicaAsync(
            string nodeName,
            PartitionSelector partitionSelector,
            bool ignoreConstraints,
            int? timeoutSec,
            CancellationToken cancellationToken)
        {
            TimeSpan timeout = timeoutSec.HasValue ? TimeSpan.FromSeconds(timeoutSec.Value) : DefaultOperationTimeout;

            return this.FabricClient.FaultManager.MovePrimaryAsync(
                       nodeName,
                       partitionSelector,
                       ignoreConstraints,
                       timeout,
                       cancellationToken);
        }

        public Task<MoveSecondaryResult> MoveSecondaryReplicaAsync(
            PartitionSelector partitionSelector,
            int? timeoutSec,
            CancellationToken cancellationToken)
        {
            TimeSpan timeout = timeoutSec.HasValue ? TimeSpan.FromSeconds(timeoutSec.Value) : DefaultOperationTimeout;

            return this.FabricClient.FaultManager.MoveSecondaryAsync(
                       partitionSelector,
                       timeout,
                       cancellationToken);
        }

        public Task<MoveSecondaryResult> MoveSecondaryReplicaAsync(
            string currentSecondaryNodeName,
            PartitionSelector partitionSelector,
            int? timeoutSec,
            CancellationToken cancellationToken)
        {
            TimeSpan timeout = timeoutSec.HasValue ? TimeSpan.FromSeconds(timeoutSec.Value) : DefaultOperationTimeout;

            return this.FabricClient.FaultManager.MoveSecondaryAsync(
                       currentSecondaryNodeName,
                       partitionSelector,
                       timeout,
                       cancellationToken);
        }

        public Task<MoveSecondaryResult> MoveSecondaryReplicaAsync(
            string currentSecondaryNodeName,
            string newSecondaryNodeName,
            PartitionSelector partitionSelector,
            int? timeoutSec,
            CancellationToken cancellationToken)
        {
            TimeSpan timeout = timeoutSec.HasValue ? TimeSpan.FromSeconds(timeoutSec.Value) : DefaultOperationTimeout;

            return this.FabricClient.FaultManager.MoveSecondaryAsync(
                       currentSecondaryNodeName,
                       newSecondaryNodeName,
                       partitionSelector,
                       timeout,
                       cancellationToken);
        }

        public Task<MoveSecondaryResult> MoveSecondaryReplicaAsync(
            PartitionSelector partitionSelector,
            bool ignoreConstraints,
            int? timeoutSec,
            CancellationToken cancellationToken)
        {
            TimeSpan timeout = timeoutSec.HasValue ? TimeSpan.FromSeconds(timeoutSec.Value) : DefaultOperationTimeout;

            return this.FabricClient.FaultManager.MoveSecondaryAsync(
                       partitionSelector,
                       ignoreConstraints,
                       timeout,
                       cancellationToken);
        }

        public Task<MoveSecondaryResult> MoveSecondaryReplicaAsync(
            string currentSecondaryNodeName,
            PartitionSelector partitionSelector,
            bool ignoreConstraints,
            int? timeoutSec,
            CancellationToken cancellationToken)
        {
            TimeSpan timeout = timeoutSec.HasValue ? TimeSpan.FromSeconds(timeoutSec.Value) : DefaultOperationTimeout;

            return this.FabricClient.FaultManager.MoveSecondaryAsync(
                       currentSecondaryNodeName,
                       partitionSelector,
                       ignoreConstraints,
                       timeout,
                       cancellationToken);
        }

        public Task<MoveSecondaryResult> MoveSecondaryReplicaAsync(
            string currentSecondaryNodeName,
            string newSecondaryNodeName,
            PartitionSelector partitionSelector,
            bool ignoreConstraints,
            int? timeoutSec,
            CancellationToken cancellationToken)
        {
            TimeSpan timeout = timeoutSec.HasValue ? TimeSpan.FromSeconds(timeoutSec.Value) : DefaultOperationTimeout;

            return this.FabricClient.FaultManager.MoveSecondaryAsync(
                       currentSecondaryNodeName,
                       newSecondaryNodeName,
                       partitionSelector,
                       ignoreConstraints,
                       timeout,
                       cancellationToken);
        }

        public Task<RestartNodeResult> RestartNodeAsync(
            ReplicaSelector replicaSelector,
            bool createFabricDump,
            int? timeoutSecs,
            CompletionMode completionMode,
            CancellationToken cancellationToken)
        {
            TimeSpan timeout = timeoutSecs.HasValue ? TimeSpan.FromSeconds(timeoutSecs.Value) : DefaultOperationTimeout;

            return this.FabricClient.FaultManager.RestartNodeAsync(
                       replicaSelector,
                       createFabricDump,
                       completionMode,
                       timeout,
                       cancellationToken);
        }

        public Task<RestartNodeResult> RestartNodeAsync(
            string nodeName,
            BigInteger nodeInstanceId,
            bool createFabricDump,
            int? timeoutSecs,
            CompletionMode completionMode,
            CancellationToken cancellationToken)
        {
            TimeSpan timeout = timeoutSecs.HasValue ? TimeSpan.FromSeconds(timeoutSecs.Value) : DefaultOperationTimeout;

            return ClusterConnection.ValidateNodeNameOnFailure<RestartNodeResult>(
                       nodeName,
                       this.FabricClient,
                       async () =>
            {
                return await this.FabricClient.FaultManager.RestartNodeAsync(
                    nodeName,
                    nodeInstanceId,
                    createFabricDump,
                    completionMode,
                    timeout,
                    cancellationToken);
            });
        }

        public Task<RestartPartitionResult> RestartPartitionAsync(
            PartitionSelector partitionSelector,
            RestartPartitionMode restartPartitionMode,
            int? operationTimeoutSec,
            CancellationToken token)
        {
            TimeSpan timeout = operationTimeoutSec.HasValue ? TimeSpan.FromSeconds(operationTimeoutSec.Value) : DefaultOperationTimeout;

#pragma warning disable 618
            return this.FabricClient.TestManager.RestartPartitionAsync(
                       partitionSelector,
                       restartPartitionMode,
                       timeout,
                       token);
#pragma warning restore 618
        }

        public Task RestartPartitionAsync(
            Guid operationId,
            PartitionSelector partitionSelector,
            RestartPartitionMode restartPartitionMode,
            int? operationTimeoutSec,
            CancellationToken token)
        {
            TimeSpan timeout = operationTimeoutSec.HasValue ? TimeSpan.FromSeconds(operationTimeoutSec.Value) : DefaultOperationTimeout;

            return this.FabricClient.TestManager.StartPartitionRestartAsync(
                       operationId,
                       partitionSelector,
                       restartPartitionMode,
                       timeout,
                       token);
        }

        public Task<PartitionRestartProgress> GetRestartPartitionProgressAsync(
            Guid operationId,
            TimeSpan requestTimeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.TestManager.GetPartitionRestartProgressAsync(
                       operationId,
                       requestTimeout,
                       cancellationToken);
        }

        public Task<TestCommandStatusList> GetTestCommandStatusListAsync(
            TestCommandStateFilter stateFilter,
            TestCommandTypeFilter typeFilter,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.TestManager.GetTestCommandStatusListAsync(
                       stateFilter,
                       typeFilter,
                       timeout,
                       cancellationToken);
        }

        public Task CancelTestCommandAsync(
            Guid operationId,
            bool force,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.TestManager.CancelTestCommandAsync(
                       operationId,
                       force,
                       timeout,
                       cancellationToken);
        }

        #region Chaos

        public Task StartChaosAsync(
            ChaosParameters parameters,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.TestManager.StartChaosAsync(
                       parameters,
                       timeout,
                       cancellationToken);
        }

        public Task StopChaosAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.TestManager.StopChaosAsync(
                       timeout,
                       cancellationToken);
        }

        public Task<ChaosReport> GetChaosReportAsync(
            ChaosReportFilter filter,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.TestManager.GetChaosReportAsync(
                       filter,
                       timeout,
                       cancellationToken);
        }

        public Task<ChaosEventsSegment> GetChaosEventsAsync(
            ChaosEventsSegmentFilter filter,
            long maxResults,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.TestManager.GetChaosEventsAsync(
                filter,
                maxResults,
                timeout,
                cancellationToken);
        }

        public Task<ChaosDescription> GetChaosAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.TestManager.GetChaosAsync(
                timeout,
                cancellationToken);
        }

        public Task SetChaosScheduleAsync(
            ChaosScheduleDescription schedule,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.TestManager.SetChaosScheduleAsync(
                schedule,
                timeout,
                cancellationToken);
        }

        public Task<ChaosScheduleDescription> GetChaosScheduleAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.TestManager.GetChaosScheduleAsync(
                timeout,
                cancellationToken);
        }

        public Task<ChaosReport> GetChaosReportAsync(
            string continuationToken,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.TestManager.GetChaosReportAsync(
                       continuationToken,
                       timeout,
                       cancellationToken);
        }

        public Task<ChaosEventsSegment> GetChaosEventsAsync(
            string continuationToken,
            long maxResults,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.TestManager.GetChaosEventsAsync(
                continuationToken,
                maxResults,
                timeout,
                cancellationToken);
        }

        #endregion Chaos

        public Task StartNodeTransitionAsync(
            NodeTransitionDescription description,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.TestManager.StartNodeTransitionAsync(
                       description,
                       timeout,
                       cancellationToken);
        }

        public Task<NodeTransitionProgress> GetNodeTransitionProgressAsync(
            Guid operationId,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.TestManager.GetNodeTransitionProgressAsync(
                       operationId,
                       timeout,
                       cancellationToken);
        }

        public Task<Secret[]> GetSecretsAsync(
            SecretReference[] secretReferences,
            bool includeValue,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.SecretStore.GetSecretsAsync(
                secretReferences,
                includeValue,
                timeout, 
                cancellationToken);
        }

        public Task<SecretReference[]> SetSecretsAsync(
            Secret[] secrets,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.SecretStore.SetSecretsAsync(
                secrets,
                timeout,
                cancellationToken);
        }

        public Task<SecretReference[]> RemoveSecretsAsync(
            SecretReference[] secretReferences,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.SecretStore.RemoveSecretsAsync(
               secretReferences,
               timeout,
               cancellationToken);
        }

        public Task StartClusterConfigurationUpgradeAsync(
            ConfigurationUpgradeDescription description,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.ClusterManager.UpgradeConfigurationAsync(
                       description,
                       timeout,
                       cancellationToken);
        }

        public Task<FabricOrchestrationUpgradeProgress> GetClusterConfigurationUpgradeStatusAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.ClusterManager.GetClusterConfigurationUpgradeStatusAsync(
                       timeout,
                       cancellationToken);
        }

        public Task<string> GetClusterConfigurationAsync(
            string apiVersion,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.ClusterManager.GetClusterConfigurationAsync(
                       apiVersion,
                       timeout,
                       cancellationToken);
        }

        public Task<string> GetUpgradeOrchestrationServiceStateAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.ClusterManager.GetUpgradeOrchestrationServiceStateAsync(
                       timeout,
                       cancellationToken);
        }

        public Task<FabricUpgradeOrchestrationServiceState> SetUpgradeOrchestrationServiceStateAsync(
            string state,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.ClusterManager.SetUpgradeOrchestrationServiceStateAsync(
                state,
                timeout,
                cancellationToken);
        }

        public Task GetUpgradesPendingApprovalAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.ClusterManager.GetUpgradesPendingApprovalAsync(
                       timeout,
                       cancellationToken);
        }

        public Task StartApprovedUpgradesAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.ClusterManager.StartApprovedUpgradesAsync(
                       timeout,
                       cancellationToken);
        }

        [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1305:FieldNamesMustNotUseHungarianNotation", Justification = "Reviewed. Suppression is OK here for ipAddressOrFQDN.")]
        public Task<StartNodeResult> StartNodeAsync(
            string nodeName,
            BigInteger instanceId,
            string ipAddressOrFQDN,
            int clusterConnectionPort,
            int? timeoutSecs,
            CompletionMode completionMode,
            CancellationToken cancellationToken)
        {
            TimeSpan timeout = timeoutSecs.HasValue ? TimeSpan.FromSeconds(timeoutSecs.Value) : DefaultOperationTimeout;

#pragma warning disable 618
            return this.FabricClient.FaultManager.StartNodeAsync(
                nodeName,
                instanceId,
                ipAddressOrFQDN,
                clusterConnectionPort,
                completionMode,
                timeout,
                cancellationToken);
#pragma warning restore 618
        }

        public Task<StopNodeResult> StopNodeAsync(
            string nodeName,
            BigInteger nodeInstanceId,
            int? timeoutSecs,
            CompletionMode completionMode,
            CancellationToken cancellationToken)
        {
            TimeSpan timeout = timeoutSecs.HasValue ? TimeSpan.FromSeconds((double)timeoutSecs.Value) : DefaultOperationTimeout;

#pragma warning disable 618
            return ClusterConnection.ValidateNodeNameOnFailure<StopNodeResult>(
                       nodeName,
                       this.FabricClient,
                       async () =>
            {
                return await this.FabricClient.FaultManager.StopNodeAsync(
                    nodeName,
                    nodeInstanceId,
                    completionMode,
                    timeout,
                    cancellationToken);
            });
#pragma warning restore 618
        }

        public Task TestApplicationAsync(
            Uri applicationName,
            int maxStabilizationTimeoutSec,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.TestManager.ValidateApplicationAsync(
                       applicationName,
                       TimeSpan.FromSeconds(maxStabilizationTimeoutSec),
                       cancellationToken);
        }

        public Task TestServiceAsync(
            Uri serviceName,
            int maxStabilizationTimeoutSec,
            CancellationToken cancellationToken)
        {
            return this.FabricClient.TestManager.ValidateServiceAsync(
                       serviceName,
                       TimeSpan.FromSeconds(maxStabilizationTimeoutSec),
                       cancellationToken);
        }

        private void OnClientConnected(EventArgs args)
        {
            var casted = args as FabricClient.GatewayInformationEventArgs;
            if (casted == null)
            {
                return;
            }

            this.GatewayInformation = casted.GatewayInformation;
        }

        private void OnClientDisconnected(EventArgs args)
        {
            var casted = args as FabricClient.GatewayInformationEventArgs;
            if (casted == null)
            {
                return;
            }

            this.GatewayInformation = null;
        }

        private string OnClaimsRetrieval(FabricClient.ClaimsRetrievalEventArgs args)
        {
            this.AzureActiveDirectoryMetadata = args.AzureActiveDirectoryMetadata;

            this.claimsMetadataEvent.Set();

            // Provide a non-empty (invalid) token to avoid triggering the default claims retrieval
            // handler. We only needed access to the metadata.
            return "_Invalid_";
        }

        private void TestScenarioProgressChanged(object sender, ProgressChangedEventArgs e)
        {
            Console.WriteLine("Progress: {0}% - {1}", e.ProgressPercentage, e.UserState);
        }

        public class FabricClientBuilder
        {
            private readonly SecurityCredentialsBuilder securityCredentialsBuilder;

            private readonly FabricClientSettingsBuilder fabricClientSettingsBuilder;

            private readonly string[] connectionEndpoint;

            private readonly SecurityCredentials securityCredentials;

            public FabricClientBuilder(
                SecurityCredentialsBuilder securityCredentialsBuilder,
                FabricClientSettingsBuilder fabricClientSettingsBuilder,
                string[] connectionEndpoint)
            {
                this.securityCredentialsBuilder = securityCredentialsBuilder;
                this.fabricClientSettingsBuilder = fabricClientSettingsBuilder;
                this.connectionEndpoint = connectionEndpoint;

                if (securityCredentialsBuilder != null)
                {
                    this.securityCredentials = securityCredentialsBuilder.Build();
                }
            }

            public FabricClient Build()
            {
                if (this.securityCredentials != null && this.fabricClientSettingsBuilder != null && this.connectionEndpoint != null)
                {
                    return new FabricClient(this.securityCredentials, this.fabricClientSettingsBuilder.Build(), this.connectionEndpoint);
                }

                if (this.securityCredentials != null && this.connectionEndpoint != null)
                {
                    return new FabricClient(this.securityCredentials, this.connectionEndpoint);
                }

                if (this.fabricClientSettingsBuilder != null && this.connectionEndpoint != null)
                {
                    return new FabricClient(this.fabricClientSettingsBuilder.Build(), this.connectionEndpoint);
                }

                if (this.fabricClientSettingsBuilder != null)
                {
                    return new FabricClient(this.fabricClientSettingsBuilder.Build());
                }

                if (this.connectionEndpoint != null)
                {
                    return new FabricClient(this.connectionEndpoint);
                }

                return new FabricClient();
            }

            public string[] GetConnectionEndPoint()
            {
                return this.connectionEndpoint;
            }

            public SecurityCredentials GetSecurityCredentials()
            {
                return this.securityCredentials;
            }
        }

        public class FabricClientSettingsBuilder
        {
            private readonly double? connectionInitializationTimeoutInSec;
            private readonly double? healthOperationTimeoutInSec;
            private readonly double? healthReportSendIntervalInSec;
            private readonly double? keepAliveIntervalInSec;
            private readonly double? serviceChangePollIntervalInSec;
            private readonly long? partitionLocationCacheLimit;
            private readonly double? healthReportRetrySendIntervalInSec;
            private readonly long? authTokenBufferSize;

            // If argument(s) are null then they will be ignored in Build() method.
            public FabricClientSettingsBuilder(
                double? connectionInitializationTimeoutInSec,
                double? healthOperationTimeoutInSec,
                double? healthReportSendIntervalInSec,
                double? keepAliveIntervalInSec,
                double? serviceChangePollIntervalInSec,
                long? partitionLocationCacheLimit,
                double? healthReportRetrySendIntervalInSec,
                long? authTokenBufferSize)
            {
                // Set the value of the fields. Note that it could be null.
                this.connectionInitializationTimeoutInSec = connectionInitializationTimeoutInSec;
                this.healthOperationTimeoutInSec = healthOperationTimeoutInSec;
                this.healthReportSendIntervalInSec = healthReportSendIntervalInSec;
                this.keepAliveIntervalInSec = keepAliveIntervalInSec;
                this.serviceChangePollIntervalInSec = serviceChangePollIntervalInSec;
                this.partitionLocationCacheLimit = partitionLocationCacheLimit;
                this.healthReportRetrySendIntervalInSec = healthReportRetrySendIntervalInSec;
                this.authTokenBufferSize = authTokenBufferSize;
            }

            public virtual FabricClientSettings Build()
            {
                var fabricClientSettings = new FabricClientSettings();

                // ConnectionInitializationTimeoutInSec
                if (this.connectionInitializationTimeoutInSec.HasValue)
                {
                    fabricClientSettings.ConnectionInitializationTimeout = TimeSpan.FromSeconds(this.connectionInitializationTimeoutInSec.Value);
                }

                // HealthOperationTimeoutInSec
                if (this.healthOperationTimeoutInSec.HasValue)
                {
                    fabricClientSettings.HealthOperationTimeout = TimeSpan.FromSeconds(this.healthOperationTimeoutInSec.Value);
                }

                // healthReportSendIntervalInSec
                if (this.healthReportSendIntervalInSec.HasValue)
                {
                    fabricClientSettings.HealthReportSendInterval = TimeSpan.FromSeconds(this.healthReportSendIntervalInSec.Value);
                }

                // keepAliveIntervalInSec
                if (this.keepAliveIntervalInSec.HasValue)
                {
                    fabricClientSettings.KeepAliveInterval = TimeSpan.FromSeconds(this.keepAliveIntervalInSec.Value);
                }

                // ServiceChangePollIntervalInSec
                if (this.serviceChangePollIntervalInSec.HasValue)
                {
                    fabricClientSettings.ServiceChangePollInterval = TimeSpan.FromSeconds(this.serviceChangePollIntervalInSec.Value);
                }

                // PartitionLocationCacheLimit
                if (this.partitionLocationCacheLimit.HasValue)
                {
                    fabricClientSettings.PartitionLocationCacheLimit = this.partitionLocationCacheLimit.Value;
                }

                // healthReportRetrySendIntervalInSec
                if (this.healthReportRetrySendIntervalInSec.HasValue)
                {
                    fabricClientSettings.HealthReportRetrySendInterval = TimeSpan.FromSeconds(this.healthReportRetrySendIntervalInSec.Value);
                }

                if (this.authTokenBufferSize.HasValue)
                {
                    fabricClientSettings.AuthTokenBufferSize = this.authTokenBufferSize.Value;
                }

                fabricClientSettings.ClientFriendlyName = "PowerShell-" + Guid.NewGuid();

                return fabricClientSettings;
            }
        }

        public abstract class SecurityCredentialsBuilder
        {
            public abstract SecurityCredentials Build();
        }

        public class WindowsSecurityCredentialsBuilder : SecurityCredentialsBuilder
        {
            private readonly string remoteSpn;

            public WindowsSecurityCredentialsBuilder(string remoteSpn)
            {
                this.remoteSpn = remoteSpn ?? string.Empty;
            }

            public override SecurityCredentials Build()
            {
                return new WindowsCredentials { RemoteSpn = this.remoteSpn };
            }
        }

        public class X509SecurityCredentialsBuilder : SecurityCredentialsBuilder
        {
            private readonly string[] serverCertThumbprints;
            private readonly string[] serverCommonNames;
            private readonly X509FindType findType;
            private readonly string findValue;
            private readonly StoreLocation storeLocation;
            private readonly string storeName;

            public X509SecurityCredentialsBuilder(
                string[] serverCertThumbprints,
                string[] serverCommonNames,
                X509FindType findType,
                string findValue,
                StoreLocation storeLocation,
                string storeName)
            {
                if ((findType != X509FindType.FindByThumbprint) && (findType != X509FindType.FindBySubjectName))
                {
                    throw new ArgumentException(StringResources.Error_InvalidClusterConnectionFindType);
                }

                this.serverCertThumbprints = serverCertThumbprints;
                this.serverCommonNames = serverCommonNames;
                this.findType = findType;
                this.findValue = findValue;
                this.storeLocation = storeLocation == 0 ? StoreLocation.LocalMachine : storeLocation;
                this.storeName = storeName ?? Constants.DefaultStoreName;
            }

            public override SecurityCredentials Build()
            {
                var securityCredentials = new X509Credentials();

                if (this.serverCertThumbprints != null)
                {
                    foreach (var thumbprint in this.serverCertThumbprints)
                    {
                        securityCredentials.RemoteCertThumbprints.Add(thumbprint);
                    }
                }

                if (this.serverCommonNames != null)
                {
                    foreach (var commonName in this.serverCommonNames)
                    {
                        securityCredentials.RemoteCommonNames.Add(commonName);
                    }
                }

                securityCredentials.FindType = this.findType;
                securityCredentials.FindValue = this.findValue;
                securityCredentials.StoreLocation = this.storeLocation;
                securityCredentials.StoreName = this.storeName;

                return securityCredentials;
            }
        }

        public class DSTSCredentialsBuilder : SecurityCredentialsBuilder
        {
            private readonly dSTSClientProxy client;
            private readonly string[] serverCommonNames;
            private readonly string[] serverThumbprints;

            public DSTSCredentialsBuilder(
                string[] serverCommonNames,
                string[] serverThumbprints,
                string metadataUrl,
                bool interactive,
                string cloudServiceName,
                string[] cloudServiceDNSNames)
            {
                this.client = new dSTSClientProxy(metadataUrl, serverCommonNames, serverThumbprints, interactive, cloudServiceName, cloudServiceDNSNames);
                this.serverCommonNames = serverCommonNames;
                this.serverThumbprints = serverThumbprints;
            }

            public override SecurityCredentials Build()
            {
                var securityCredentials = new ClaimsCredentials();
                if (this.client != null)
                {
                    string token = this.client.GetSecurityToken();
                    securityCredentials.LocalClaims = token;

                    if (this.serverCommonNames != null)
                    {
                        foreach (var commonName in this.serverCommonNames)
                        {
                            securityCredentials.ServerCommonNames.Add(commonName);
                        }
                    }

                    if (this.serverThumbprints != null)
                    {
                        foreach (var thumbprint in this.serverThumbprints)
                        {
                            securityCredentials.ServerThumbprints.Add(thumbprint);
                        }
                    }
                }

                return securityCredentials;
            }
        }

        public class AadCredentialsBuilder : SecurityCredentialsBuilder
        {
            private readonly string[] serverCommonNames;
            private readonly string[] serverThumbprints;
            private readonly string token;

            public AadCredentialsBuilder(
                string[] serverCommonNames,
                string[] serverThumbprints,
                string token)
            {
                this.serverCommonNames = serverCommonNames;
                this.serverThumbprints = serverThumbprints;
                this.token = token;
            }

            public override SecurityCredentials Build()
            {
                var securityCredentials = new ClaimsCredentials();

                if (this.serverCommonNames != null)
                {
                    foreach (var commonName in this.serverCommonNames)
                    {
                        securityCredentials.ServerCommonNames.Add(commonName);
                    }
                }

                if (this.serverThumbprints != null)
                {
                    foreach (var thumbprint in this.serverThumbprints)
                    {
                        securityCredentials.ServerThumbprints.Add(thumbprint);
                    }
                }

                if (this.token != null)
                {
                    securityCredentials.LocalClaims = this.token;
                }

                return securityCredentials;
            }
        }
    }
}
