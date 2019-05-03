// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric;
    using System.Fabric.Chaos.DataStructures;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Query;
    using System.Fabric.Repair;
    using System.Fabric.Result;
    using System.Fabric.SecretStore;
    using System.Fabric.Testability.Scenario;
    using System.Numerics;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Preview.Client.Description;
    using Microsoft.ServiceFabric.Preview.Client.Query;

    public interface IClusterConnection
    {
        string[] ConnectionEndpoint
        {
            get;
            set;
        }

        SecurityCredentials SecurityCredentials
        {
            get;
        }

        FabricClientSettings FabricClientSettings
        {
            get;
        }

        FabricClient FabricClient
        {
            get;
        }

        void InitializeClaimsMetadata(TimeSpan timeout);

        // Supported methods from PropertyManagement
        Task<bool> NameExistsAsync(Uri name, TimeSpan timeout, CancellationToken cancellationToken);

        Task<ApplicationList> GetApplicationPagedListAsync(ApplicationQueryDescription applicationQueryDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task<ApplicationTypeList> GetApplicationTypeListAsync(string applicationTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken);

        Task<ApplicationTypePagedList> GetApplicationTypePagedListAsync(PagedApplicationTypeQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task<DeployedApplicationList> GetDeployedApplicationListAsync(string nodeName, Uri applicationNameFilter, TimeSpan timeout, CancellationToken cancellationToken);

        Task<DeployedApplicationPagedList> GetDeployedApplicationPagedListAsync(
            PagedDeployedApplicationQueryDescription queryDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task<DeployedCodePackageList> GetDeployedCodePackageListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, string codePackageNameFilter, TimeSpan timeout, CancellationToken cancellationToken);

        Task<DeployedServiceReplicaList> GetDeployedReplicaListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, Guid? partitionIdFilter, TimeSpan timeout, CancellationToken cancellationToken);

        Task<DeployedServicePackageList> GetDeployedServicePackageListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, TimeSpan timeout, CancellationToken cancellationToken);

        Task<DeployedServiceTypeList> GetDeployedServiceTypeListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, string serviceTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken);

        Task<NodeList> GetNodeListAsync(NodeQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task<ServicePartitionList> GetPartitionListAsync(Uri serviceName, Guid? partitionIdFilter, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken);

        Task<ServicePartitionList> GetPartitionAsync(Guid partitionIdFilter, TimeSpan timeout, CancellationToken cancellationToken);

        Task<PartitionLoadInformation> GetPartitionLoadInformationAsync(Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken);

        Task<ReplicaLoadInformation> GetReplicaLoadInformationAsync(Guid partitionId, long replicaOrInstanceId, TimeSpan timeout, CancellationToken cancellationToken);

        Task<UnplacedReplicaInformation> GetUnplacedReplicaInformationAsync(string serviceName, Guid partitionId, bool onlyQueryPrimaries, TimeSpan timeout, CancellationToken cancellationToken);

        Task<ServiceReplicaList> GetReplicaListAsync(Guid partitionId, long replicaIdOrInstanceIdFilter, ServiceReplicaStatusFilter replicaStatusFilter, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken);

        Task<ServiceList> GetServicePagedListAsync(ServiceQueryDescription serviceQueryDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task<ServiceGroupMemberList> GetServiceGroupMemberListAsync(Uri applicationName, Uri serviceNameFilter, TimeSpan timeout, CancellationToken cancellationToken);

        Task<ServiceTypeList> GetServiceTypeListAsync(string applicationTypeName, string applicationTypeVersion, string serviceTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken);

        Task<ServiceGroupMemberTypeList> GetServiceGroupMemberTypeListAsync(string applicationTypeName, string applicationTypeVersion, string serviceGroupTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken);

        Task<DeployedServiceReplicaDetail> GetDeployedReplicaDetailAsync(string nodeName, Guid partitionId, long replicaId, TimeSpan timeout, CancellationToken cancellationToken);

        Task<ClusterLoadInformation> GetClusterLoadInformationAsync(TimeSpan timeout, CancellationToken cancellationToken);

        Task<ProvisionedFabricCodeVersionList> GetProvisionedFabricCodeVersionListAsync(string codeVersionFilter, TimeSpan timeout, CancellationToken cancellationToken);

        Task<ProvisionedFabricConfigVersionList> GetProvisionedFabricConfigVersionListAsync(string configVersionFilter, TimeSpan timeout, CancellationToken cancellationToken);

        Task<NodeLoadInformation> GetNodeLoadInformationAsync(string nodeName, TimeSpan timeout, CancellationToken cancellationToken);

        Task<ApplicationLoadInformation> GetApplicationLoadInformationAsync(string applicationName, TimeSpan timeout, CancellationToken cancellationToken);

        Task<ServiceNameResult> GetServiceNameAsync(Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken);

        Task<ApplicationNameResult> GetApplicationNameAsync(Uri serviceName, TimeSpan timeout, CancellationToken cancellationToken);

        // Supported methods from ApplicationManager
        Task CreateApplicationAsync(ApplicationDescription applicationDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task UpdateApplicationAsync(ApplicationUpdateDescription updateDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task DeleteApplicationAsync(DeleteApplicationDescription deleteDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task<string> GetApplicationManifestAsync(string applicationTypeName, string applicationTypeVersion, TimeSpan timeout, CancellationToken cancellationToken);

        Task<ApplicationUpgradeProgress> GetApplicationUpgradeProgressAsync(Uri applicationName, TimeSpan timeout, CancellationToken cancellationToken);

        Task<ComposeDeploymentUpgradeProgress> GetComposeDeploymentUpgradeProgressAsync(string deploymentName, TimeSpan timeout, CancellationToken cancellationToken);

        Task MoveNextApplicationUpgradeDomainAsync(Uri applicationName, string nextUpgradeDomain, TimeSpan timeout, CancellationToken cancellationToken);

        Task ProvisionApplicationAsync(ProvisionApplicationTypeDescriptionBase description, TimeSpan timeout, CancellationToken cancellationToken);

        Task UnprovisionApplicationAsync(UnprovisionApplicationTypeDescription description, TimeSpan timeout, CancellationToken cancellationToken);

        Task UpgradeApplicationAsync(ApplicationUpgradeDescription upgradeDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task UpdateApplicationUpgradeAsync(ApplicationUpgradeUpdateDescription updateDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task RollbackApplicationUpgradeAsync(Uri applicationName, TimeSpan timeout, CancellationToken cancellationToken);

        Task DeployServicePackageToNodeAsync(string applicationTypeName, string applicationTypeVersion, string serviceManifestName, PackageSharingPolicyList sharingPolicies, string nodeName, TimeSpan timeout, CancellationToken cancellationToken);

        // Supported methods from Compose Application
        Task<ComposeDeploymentStatusList> GetComposeDeploymentStatusPagedListAsync(ComposeDeploymentStatusQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task CreateComposeDeploymentAsync(ComposeDeploymentDescription description, TimeSpan timeout, CancellationToken cancellationToken);

        Task DeleteComposeDeploymentAsync(DeleteComposeDeploymentDescription description, TimeSpan timeout, CancellationToken cancellationToken);

        Task UpgradeComposeDeploymentAsync(ComposeDeploymentUpgradeDescription description, TimeSpan timeout, CancellationToken cancellationToken);

        Task RollbackComposeDeploymentUpgradeAsync(ComposeDeploymentRollbackDescription description, TimeSpan timeout, CancellationToken cancellationToken);

        // Supported methods from ClusterManager
        Task ActivateNodeAsync(string nodeName, TimeSpan timeout, CancellationToken cancellationToken);

        Task DeactivateNodeAsync(string nodeName, NodeDeactivationIntent deactivationIntent, TimeSpan timeout, CancellationToken cancellationToken);

        // Task<StartNodeResult> StartNodeAsync(string nodeName, BigInteger instanceId, string ipAddressOrFQDN, int clientConnectionPort, int? requestTimeout, CompletionMode completionMode, CancellationToken cancellationToken);
        Task<string> GetClusterManifestAsync(TimeSpan timeout, CancellationToken cancellationToken);

        Task<string> GetClusterManifestAsync(ClusterManifestQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task<FabricUpgradeProgress> GetFabricUpgradeProgressAsync(TimeSpan timeout, CancellationToken cancellationToken);

        Task MoveNextFabricUpgradeDomainAsync(string nextUpgradeDomain, TimeSpan timeout, CancellationToken cancellationToken);

        Task ProvisionFabricAsync(string mspFilePath, string clusterManifestFilePath, TimeSpan timeout, CancellationToken cancellationToken);

        Task RemoveNodeStateAsync(string nodeName, TimeSpan timeout, CancellationToken cancellationToken);

        Task UnprovisionFabricAsync(string codeVersion, string configVersion, TimeSpan timeout, CancellationToken cancellationToken);

        Task UpgradeFabricAsync(FabricUpgradeDescription upgradeDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task UpdateFabricUpgradeAsync(FabricUpgradeUpdateDescription updateDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task RollbackFabricUpgradeAsync(TimeSpan timeout, CancellationToken cancellationToken);

        Task RecoverPartitionsAsync(TimeSpan timeout, CancellationToken cancellationToken);

        Task RecoverPartitionAsync(Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken);

        Task RecoverServicePartitionsAsync(Uri serviceName, TimeSpan timeout, CancellationToken cancellationToken);

        Task RecoverSystemPartitionsAsync(TimeSpan timeout, CancellationToken cancellationToken);

        Task ResetPartitionLoadAsync(Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken);

        Task ToggleVerboseServicePlacementHealthReportingAsync(bool enabled, TimeSpan timeout, CancellationToken cancellationToken);

        Task<RemoveReplicaResult> RemoveReplicaAsync(string nodeName, Guid partitionId, long replicaId, CompletionMode completionMode, bool forceRemove, TimeSpan timeout, CancellationToken cancellationToken);

        Task<RemoveReplicaResult> RemoveReplicaAsync(ReplicaSelector replicaSelector, int? timeoutSec, CompletionMode completionMode, bool forceRemove, CancellationToken cancellationToken);

        Task<RestartReplicaResult> RestartReplicaAsync(string nodeName, Guid partitionId, long replicaId, CompletionMode completionMode, TimeSpan timeout, CancellationToken cancellationToken);

        Task<RestartReplicaResult> RestartReplicaAsync(ReplicaSelector replicaSelector, int? timeoutSecSec, CompletionMode completionMode, CancellationToken cancellationToken);

        // Supported methods from ServiceManager
        Task CreateServiceAsync(ServiceDescription serviceDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task CreateServiceFromTemplateAsync(
            Uri applicationName,
            Uri serviceName,
            string serviceDnsName,
            string serviceTypeName,
            ServicePackageActivationMode servicePackageActivationMode,
            byte[] initializationData,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task DeleteServiceAsync(DeleteServiceDescription deleteDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task UpdateServiceAsync(Uri name, ServiceUpdateDescription serviceUpdateDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task<string> GetServiceManifestAsync(string applicationTypeName, string applicationTypeVersion, string serviceManifestName, TimeSpan timeout, CancellationToken cancellationToken);

        Task<ServiceDescription> GetServiceDescriptionAsync(Uri serviceName, TimeSpan timeout, CancellationToken cancellationToken);

        Task<ResolvedServicePartition> ResolveServicePartitionAsync(Uri serviceName, ResolvedServicePartition previousResult, TimeSpan timeout, CancellationToken cancellationToken);

        Task<ResolvedServicePartition> ResolveServicePartitionAsync(Uri serviceName, long partitionKey, ResolvedServicePartition previousResult, TimeSpan timeout, CancellationToken cancellationToken);

        Task<ResolvedServicePartition> ResolveServicePartitionAsync(Uri serviceName, string partitionKey, ResolvedServicePartition previousResult, TimeSpan timeout, CancellationToken cancellationToken);

        // Supported methods from ServiceGroupManager
        Task CreateServiceGroupAsync(ServiceGroupDescription serviceGroupDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task CreateServiceGroupFromTemplateAsync(
            Uri applicationName,
            Uri serviceName,
            string serviceTypeName,
            ServicePackageActivationMode servicePackageActivationMode,
            byte[] initializationData,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task UpdateServiceGroupAsync(Uri name, ServiceGroupUpdateDescription updateDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task DeleteServiceGroupAsync(Uri name, TimeSpan timeout, CancellationToken cancellationToken);

        Task<ServiceGroupDescription> GetServiceGroupDescriptionAsync(Uri serviceGroupName, TimeSpan timeout, CancellationToken cancellationToken);

        // Supported methods from HealthManager
        Task<ApplicationHealth> GetApplicationHealthAsync(ApplicationHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task<NodeHealth> GetNodeHealthAsync(NodeHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task<PartitionHealth> GetPartitionHealthAsync(PartitionHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task<ReplicaHealth> GetReplicaHealthAsync(ReplicaHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task<ServiceHealth> GetServiceHealthAsync(ServiceHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task<DeployedApplicationHealth> GetDeployedApplicationHealthAsync(DeployedApplicationHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task<DeployedServicePackageHealth> GetDeployedServicePackageHealthAsync(DeployedServicePackageHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task<ClusterHealth> GetClusterHealthAsync(ClusterHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task<ClusterHealthChunk> GetClusterHealthChunkAsync(ClusterHealthChunkQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken);

        void ReportHealth(HealthReport healthReport, HealthReportSendOptions sendOptions);

        Task<string> InvokeInfrastructureCommandAsync(Uri serviceName, string command, TimeSpan timeout, CancellationToken cancellationToken);

        Task<string> InvokeInfrastructureQueryAsync(Uri serviceName, string command, TimeSpan timeout, CancellationToken cancellationToken);

        Task<long> CreateRepairTaskAsync(RepairTask repairTask, TimeSpan timeout, CancellationToken cancellationToken);

        Task<long> CancelRepairTaskAsync(string repairTaskId, long version, bool requestAbort, TimeSpan timeout, CancellationToken cancellationToken);

        Task<long> ForceApproveRepairTaskAsync(string repairTaskId, long version, TimeSpan timeout, CancellationToken cancellationToken);

        Task DeleteRepairTaskAsync(string repairTaskId, long version, TimeSpan timeout, CancellationToken cancellationToken);

        Task<long> UpdateRepairExecutionStateAsync(RepairTask repairTask, TimeSpan timeout, CancellationToken cancellationToken);

        Task<RepairTaskList> GetRepairTaskListAsync(string taskIdFilter, RepairTaskStateFilter stateFilter, string executorFilter, TimeSpan timeout, CancellationToken cancellationToken);

        Task<long> UpdateRepairTaskHealthPolicyAsync(string repairTaskId, long version, bool? performPreparingHealthCheck, bool? performRestoringHealthCheck, TimeSpan timeout, CancellationToken cancellationToken);

        // Supported methods from FaultManager
        Task<RestartDeployedCodePackageResult> RestartDeployedCodePackageAsync(
            ReplicaSelector replicaSelector,
            Uri applicationName,
            int? requestTimeout,
            CompletionMode completionMode,
            CancellationToken cancellationToken);

        Task<RestartDeployedCodePackageResult> RestartDeployedCodePackageAsync(
            string nodeName,
            Uri applicationName,
            string serviceManifestName,
            string servicePackageActivationId,
            string codePackageName,
            long codePackageInstanceId,
            int? requestTimeout,
            CompletionMode completionMode,
            CancellationToken cancellationToken);

        Task<MovePrimaryResult> MovePrimaryReplicaAsync(
            string newPrimaryNodeName,
            PartitionSelector partitionSelector,
            bool ignoreConstraints,
            int? requestTimeoutSec,
            CancellationToken token);

        Task<MovePrimaryResult> MovePrimaryReplicaAsync(
            string newPrimaryNodeName,
            PartitionSelector partitionSelector,
            int? requestTimeoutSec,
            CancellationToken token);

        Task<MoveSecondaryResult> MoveSecondaryReplicaAsync(
            string currentSecondaryNodeName,
            string newSecondaryNodeName,
            PartitionSelector partitionSelector,
            bool ignoreConstraints,
            int? requestTimeoutSec,
            CancellationToken token);

        Task<MoveSecondaryResult> MoveSecondaryReplicaAsync(
            string currentSecondaryNodeName,
            string newSecondaryNodeName,
            PartitionSelector partitionSelector,
            int? requestTimeoutSec,
            CancellationToken token);

        Task<RestartNodeResult> RestartNodeAsync(
            ReplicaSelector replicaSelector,
            bool createFabricDump,
            int? requestTimeoutSecsSecs,
            CompletionMode completionMode,
            CancellationToken cancellationToken);

        Task<RestartNodeResult> RestartNodeAsync(
            string nodeName,
            BigInteger nodeInstanceId,
            bool createFabricDump,
            int? requestTimeoutSecs,
            CompletionMode completionMode,
            CancellationToken cancellationToken);

        [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1305:FieldNamesMustNotUseHungarianNotation", Justification = "Reviewed. Suppression is OK here for ipAddressOrFQDN.")]
        Task<StartNodeResult> StartNodeAsync(
            string nodeName,
            BigInteger instanceId,
            string ipAddressOrFQDN,
            int clientConnectionPort,
            int? requestTimeout,
            CompletionMode completionMode,
            CancellationToken cancellationToken);

        Task<StopNodeResult> StopNodeAsync(
            string nodeName,
            BigInteger nodeInstanceId,
            int? requestTimeout,
            CompletionMode completionMode,
            CancellationToken cancellationToken);

        // Supported methods from TestManager
        Task CleanTestStateAsync(
            int? requestTimeoutSec,
            CancellationToken token);

        Task<InvokeDataLossResult> InvokeDataLossAsync(
            PartitionSelector partitionSelector,
            DataLossMode datalossMode,
            int? requestTimeoutSec,
            CancellationToken token);

        Task InvokeDataLossAsync(
            Guid operationId,
            PartitionSelector partitionSelector,
            DataLossMode dataLossMode,
            int? requestTimeoutSec,
            CancellationToken token);

        Task<PartitionDataLossProgress> GetInvokeDataLossProgressAsync(
            Guid operationId,
            TimeSpan requestTimeoutSec,
            CancellationToken cancellationToken);

        Task<InvokeQuorumLossResult> InvokeQuorumLossAsync(
            PartitionSelector partitionSelector,
            QuorumLossMode quorumlossMode,
            int quorumlossDuration,
            int? operationTimeout,
            CancellationToken token);

        Task InvokeQuorumLossAsync(
            Guid operationId,
            PartitionSelector partitionSelector,
            QuorumLossMode quorumlossMode,
            int quorumlossDuration,
            int? operationTimeout,
            CancellationToken token);

        Task<PartitionQuorumLossProgress> GetInvokeQuorumLossProgressAsync(
            Guid operationId,
            TimeSpan requestTimeoutSec,
            CancellationToken cancellationToken);

        Task<RestartPartitionResult> RestartPartitionAsync(
            PartitionSelector partitionSelector,
            RestartPartitionMode restartPartitionMode,
            int? requestTimeoutSec,
            CancellationToken token);

        Task RestartPartitionAsync(
            Guid operationId,
            PartitionSelector partitionSelector,
            RestartPartitionMode restartPartitionMode,
            int? operationTimeout,
            CancellationToken cancellationToken);

        Task<PartitionRestartProgress> GetRestartPartitionProgressAsync(
            Guid operationId,
            TimeSpan requestTimeoutSec,
            CancellationToken cancellationToken);

        Task<TestCommandStatusList> GetTestCommandStatusListAsync(
            TestCommandStateFilter stateFilter,
            TestCommandTypeFilter typeFilter,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task CancelTestCommandAsync(
            Guid operationId,
            bool force,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task StartChaosAsync(
            ChaosParameters parameters,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task StopChaosAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task<ChaosDescription> GetChaosAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task SetChaosScheduleAsync(
            ChaosScheduleDescription schedule,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task<ChaosScheduleDescription> GetChaosScheduleAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task<ChaosReport> GetChaosReportAsync(
            ChaosReportFilter filter,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task<ChaosReport> GetChaosReportAsync(
            string continuationToken,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task<ChaosEventsSegment> GetChaosEventsAsync(
            ChaosEventsSegmentFilter filter,
            long maxResults,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task<ChaosEventsSegment> GetChaosEventsAsync(
            string continuationToken,
            long maxResults,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task TestApplicationAsync(
            Uri applicationName,
            int maxStabilizationTimeout,
            CancellationToken cancellationToken);

        Task TestServiceAsync(
            Uri serviceName,
            int maxStabilizationTimeout,
            CancellationToken cancellationToken);

        // Supported methods to invoke scenarios
        [Obsolete("Deprecated")]
        Task InvokeChaosTestScenarioAsync(
            ChaosTestScenarioParameters chaosScenarioParameters,
            CancellationToken token);

        Task InvokeFailoverTestScenarioAsync(
            Uri serviceName,
            PartitionSelector partitionSelector,
            uint timeToRunInMinutes,
            uint maxServiceStabilizationTimeoutInSeconds,
            int? waitTimeBetweenFaultsInSeconds,
            int? requestTimeoutSec,
            CancellationToken token);

        Task<Secret[]> GetSecretsAsync(
            SecretReference[] secretReferences,
            bool includeValue,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task<SecretReference[]> SetSecretsAsync(
            Secret[] secrets,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task<SecretReference[]> RemoveSecretsAsync(
            SecretReference[] secretReferences,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task StartClusterConfigurationUpgradeAsync(
            ConfigurationUpgradeDescription description,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task<FabricOrchestrationUpgradeProgress> GetClusterConfigurationUpgradeStatusAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task<string> GetClusterConfigurationAsync(
            string apiVersion,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task GetUpgradesPendingApprovalAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task StartApprovedUpgradesAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task<string> GetUpgradeOrchestrationServiceStateAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task<FabricUpgradeOrchestrationServiceState> SetUpgradeOrchestrationServiceStateAsync(
            string state,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task StartNodeTransitionAsync(
            NodeTransitionDescription description,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task<NodeTransitionProgress> GetNodeTransitionProgressAsync(
            Guid operationId,
            TimeSpan timeout,
            CancellationToken cancellationToken);
    }
}