// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Description;
    using System.Fabric;
    using System.Fabric.Chaos.DataStructures;
    using System.Fabric.Health;
    using System.Fabric.ImageStore;
    using System.Fabric.Query;
    using System.Fabric.Result;
    using System.Fabric.Testability.Client.Structures;
    using System.Fabric.Testability.Scenario;
    using System.Numerics;
    using System.Threading;
    using System.Threading.Tasks;
    using Repair;

    public interface IFabricClient
    {
        string Name { get; }

        OperationResult<UpgradeDomainStatus[]> GetChangedUpgradeDomains(ApplicationUpgradeProgress previousApplicationUpgradeProgress, ApplicationUpgradeProgress currentApplicationUpgradeProgress);

        OperationResult<UpgradeDomainStatus[]> GetChangedUpgradeDomains(FabricUpgradeProgress previousFabricUpgradeProgress, FabricUpgradeProgress currentFabricUpgradeProgress);

        ////
        //// Application Management Client APIs
        ////

        Task<OperationResult> CreateApplicationAsync(ApplicationDescription applicationDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> UpdateApplicationAsync(ApplicationUpdateDescription updateDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> DeleteApplicationAsync(DeleteApplicationDescription deleteApplicationDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<string>> GetApplicationManifestAsync(string applicationTypeName, string applicationTypeVersion, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<ApplicationUpgradeProgress>> GetApplicationUpgradeProgressAsync(Uri name, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> MoveNextApplicationUpgradeDomainAsync(ApplicationUpgradeProgress currentProgress, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> ProvisionApplicationAsync(string buildPath, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> ProvisionApplicationAsync(ProvisionApplicationTypeDescriptionBase description, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> PutApplicationResourceAsync(string deploymentName, string descriptionJson, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> DeleteApplicationResourceAsync(string deploymentName, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<ApplicationResourceList>> GetApplicationResourceListAsync(string name, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<ServiceResourceList>> GetServiceResourceListAsync(string applicationname, string servicename, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<ReplicaResourceList>> GetReplicaResourceListAsync(string applicationname, string servicename, string replicaName, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> UnprovisionApplicationAsync(string applicationTypeName, string applicationTypeVersion, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> UnprovisionApplicationAsync(UnprovisionApplicationTypeDescription description, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> UpgradeApplicationAsync(ApplicationUpgradeDescription description, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> UpdateApplicationUpgradeAsync(ApplicationUpgradeUpdateDescription description, TimeSpan timeout, CancellationToken cancellationToken);

        ////
        //// Cluster Management Client APIs
        ////

        Task<OperationResult> ActivateNodeAsync(string nodeName, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> DeactivateNodeAsync(string nodeName, NodeDeactivationIntent deactivationIntent, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<string>> GetClusterManifestAsync(TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<string>> GetClusterManifestAsync(ClusterManifestQueryDescription description, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<string>> GetClusterVersionAsync(TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<FabricOrchestrationUpgradeProgress>> GetFabricConfigUpgradeProgressAsync(TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<string>> GetFabricConfigurationAsync(string apiVersion, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<string>> GetUpgradeOrchestrationServiceStateAsync(TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<FabricUpgradeOrchestrationServiceState>> SetUpgradeOrchestrationServiceStateAsync(string state, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<FabricUpgradeProgress>> GetFabricUpgradeProgressAsync(TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> MoveNextFabricUpgradeDomainAsync(FabricUpgradeProgress currentProgress, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> ProvisionFabricAsync(string codePackagePath, string clusterManifestFilePath, TimeSpan timeout, CancellationToken cancellationToken);

        //// TODO, API, Missing RecoverPartitionAsync

        Task<OperationResult> RecoverPartitionsAsync(TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> RecoverServicePartitionsAsync(Uri serviceName, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> RecoverSystemPartitionsAsync(TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> RemoveNodeStateAsync(string nodeName, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> UnprovisionFabricAsync(string codeVersion, string configVersion, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> UpgradeFabricAsync(FabricUpgradeDescription description, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> UpgradeFabricStandaloneAsync(ConfigurationUpgradeDescription description, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> UpdateFabricUpgradeAsync(FabricUpgradeUpdateDescription updateDescription, TimeSpan timeout, CancellationToken cancellationToken);

        ////
        //// Health Client APIs
        ////

        Task<OperationResult<ApplicationHealth>> GetApplicationHealthAsync(ApplicationHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<ClusterHealth>> GetClusterHealthAsync(ClusterHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<ClusterHealthChunk>> GetClusterHealthChunkAsync(ClusterHealthChunkQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<NodeHealth>> GetNodeHealthAsync(NodeHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<PartitionHealth>> GetPartitionHealthAsync(PartitionHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<ReplicaHealth>> GetReplicaHealthAsync(ReplicaHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<ServiceHealth>> GetServiceHealthAsync(ServiceHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<DeployedServicePackageHealth>> GetDeployedServicePackageHealthAsync(DeployedServicePackageHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<DeployedApplicationHealth>> GetDeployedApplicationHealthAsync(DeployedApplicationHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken);

        OperationResult ReportHealth(HealthReport healthReport, HealthReportSendOptions sendOptions);


        ////
        //// Image Store Client APIs
        ////
        Task<OperationResult<ImageStoreContentResult>> ListImageStoreContentAsync(string remoteLocation, string imageStoreConnectionString, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<ImageStorePagedContentResult>> ListImageStorePagedContentAsync(ImageStoreListDescription listDescription, string imageStoreConnectionString, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> DeleteImageStoreContentAsync(string remoteLocation, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> CopyImageStoreContentAsync(WinFabricCopyImageStoreContentDescription description, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> UploadChunkAsync(string remoteLocation, string sessionId, UInt64 startPosition, UInt64 endPosition, string filePath,  UInt64 fileSize, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> DeleteUploadSessionAsync(Guid sessionId, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<UploadSession>> ListUploadSessionAsync(Guid SessionId, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> CommitUploadSessionAsync(Guid SessionId, TimeSpan tiemout, CancellationToken cancellationToken);

        ////
        //// Repair Management Client APIs
        ////

        Task<OperationResult<Int64>> CreateRepairTaskAsync(WinFabricRepairTask repairTask, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<Int64>> CancelRepairTaskAsync(string repairTaskId, Int64 version, bool requestAbort, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<Int64>> ForceApproveRepairTaskAsync(string repairTaskId, Int64 version, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> DeleteRepairTaskAsync(string repairTaskId, Int64 version, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<Int64>> UpdateRepairExecutionStateAsync(WinFabricRepairTask repairTask, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<WinFabricRepairTask[]>> GetRepairTaskListAsync(string taskIdFilter, WinFabricRepairTaskStateFilter stateFilter, string executorFilter, TimeSpan timeout, CancellationToken cancellationToken);

        ////
        //// Property Management Client APIs
        ////

        Task<OperationResult> CreateNameAsync(Uri name, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> DeleteNameAsync(Uri name, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> DeletePropertyAsync(Uri name, string propertyName, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<WinFabricPropertyEnumerationResult>> EnumeratePropertiesAsync(Uri name, WinFabricPropertyEnumerationResult previousResult, bool includeValue, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<WinFabricNameEnumerationResult>> EnumerateSubNamesAsync(Uri name, WinFabricNameEnumerationResult previousResult, bool recursive, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<NamedProperty>> GetPropertyAsync(Uri name, string propertyName, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<NamedPropertyMetadata>> GetPropertyMetadataAsync(Uri name, string propertyName, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<bool>> NameExistsAsync(Uri name, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> PutCustomPropertyOperationAsync(Uri name, PutCustomPropertyOperation putCustomPropertyOperation, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> PutPropertyAsync(Uri name, string propertyName, object data, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<WinFabricPropertyBatchResult>> SubmitPropertyBatchAsync(Uri name, ICollection<PropertyBatchOperation> operations, TimeSpan timeout, CancellationToken cancellationToken);

        ////
        //// Query Client APIs
        ////

        Task<OperationResult<Application>> GetApplicationAsync(Uri applicationName, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<NodeLoadInformation>> GetNodeLoadAsync(string nodeName, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<ClusterLoadInformation>> GetClusterLoadAsync(TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<ReplicaLoadInformation>> GetReplicaLoadAsync(Guid partitionId, long replicaId, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<PartitionLoadInformation>> GetPartitionLoadAsync(Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<ApplicationLoadInformation>> GetApplicationLoadAsync(string applicationName, TimeSpan timeout, CancellationToken cancellationToken);

        //// TODO, API, Missing GetApplicationServiceTypeListAsync
        Task<OperationResult<ApplicationType[]>> GetApplicationTypeListAsync(string applicationTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<ApplicationTypePagedList>> GetApplicationTypePagedListAsync(PagedApplicationTypeQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken);

        // This overload exists only to test the PowerShell specific parameters which do not exist elsewhere.
        Task<OperationResult<ApplicationTypePagedList>> GetApplicationTypePagedListAsync(PagedApplicationTypeQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken, bool getPagedResults = false);

        Task<OperationResult<DeployedApplication[]>> GetDeployedApplicationListAsync(string nodeName, Uri applicationNameFilter, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<DeployedApplicationPagedList>> GetDeployedApplicationPagedListAsync(
            PagedDeployedApplicationQueryDescription queryDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task<OperationResult<DeployedCodePackage[]>> GetDeployedCodePackageListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, string codePackageNameFilter, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<DeployedServiceReplica[]>> GetDeployedReplicaListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, Guid? partitionIdFilter, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<DeployedServicePackage[]>> GetDeployedServicePackageListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<DeployedServiceType[]>> GetDeployedServiceTypeListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, string serviceTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<NodeList>> GetNodeListAsync(NodeQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<ServicePartitionList>> GetPartitionListAsync(Uri serviceName, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<ServiceReplicaList>> GetReplicaListAsync(Guid partitionId, long replicaIdOrInstanceIdFilter, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<ServiceList>> GetServicePagedListAsync(ServiceQueryDescription serviceQueryDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<ServiceType[]>> GetServiceTypeListAsync(string applicationTypeName, string applicationTypeVersion, string serviceTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<ServiceNameResult>> GetServiceNameAsync(Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<ApplicationNameResult>> GetApplicationNameAsync(Uri serviceName, TimeSpan timeout, CancellationToken cancellationToken);

        ////
        //// ServiceGroup Management Client APIs
        ////

        Task<OperationResult> CreateServiceGroupAsync(ServiceGroupDescription winFabricServiceGroupDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> DeleteServiceGroupAsync(Uri name, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> UpdateServiceGroupAsync(ServiceKind serviceKind, Uri serviceGroupName, int targetReplicaSetSize, TimeSpan replicaRestartWaitDuration, TimeSpan quorumLossWaitDuration, int instanceCount, TimeSpan timeout, CancellationToken cancellationToken);

        // TODO, API, Missing GetServiceGroupDescriptionAsync
        ////
        //// Service Management Client APIs
        ////

        Task<OperationResult> CreateServiceAsync(ServiceDescription winFabricServiceDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> CreateServiceFromTemplateAsync(
            Uri applicationName,
            Uri serviceName,
            string serviceDnsName,
            string serviceTypeName,
            ServicePackageActivationMode servicePackageActivationMode,
            byte[] initializationData,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task<OperationResult> DeleteServiceAsync(DeleteServiceDescription deleteServiceDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<ServiceDescription>> GetServiceDescriptionAsync(Uri name, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<string>> GetServiceManifestAsync(string applicationTypeName, string applicationTypeVersion, string serviceManifestName, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<TestServicePartitionInfo[]>> ResolveServiceAsync(Uri name, object partitionKey, TestServicePartitionInfo previousResult, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> UpdateServiceAsync(Uri name, ServiceUpdateDescription description, TimeSpan timeout, CancellationToken cancellationToken);

        ////
        //// Infrastructure Apis
        ////

        Task<OperationResult<string>> InvokeInfrastructureCommandAsync(Uri serviceName, string command, TimeSpan timeout, CancellationToken cancellationToken);

        ////
        //// PowerShell Specific APIs
        ////

        Task<OperationResult<string>> InvokeEncryptSecretAsync(string certificateThumbprint, string certificateStoreLocation, string text, CancellationToken cancellationToken);

        Task<OperationResult> NewNodeConfigurationAsync(string clusterManifestPath, string computerName, string userName, string password, CancellationToken cancellationToken);

        Task<OperationResult> UpdateNodeConfigurationAsync(string clusterManifestPath, string computerName, string userName, string password, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> RemoveNodeConfigurationAsync(string clusterManifestPath, string computerName, string userName, string password, CancellationToken cancellationToken);

        Task<OperationResult<bool>> TestClusterConnectionAsync(CancellationToken cancellationToken);

        Task<OperationResult<bool>> TestClusterManifestAsync(string clusterManifestPath, CancellationToken cancellationToken);

        Task<OperationResult<bool>> TestApplicationPackageAsync(string applicationPackagePath, string imageStoreConnectionString, CancellationToken cancellationToken);

        Task<OperationResult> CopyApplicationPackageAndShowProgressAsync(string applicationPackagePath, string imageStoreConnectionString, string applicationPackagePathInImageStore, bool compress, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> CopyApplicationPackageAsync(string applicationPackagePath, string imageStoreConnectionString, string applicationPackagePathInImageStore, bool compress, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> CopyClusterPackageAsync(string codePackagePath, string clusterManifestPath, string imageStoreConnectionString, string codePackagePathInImageStore, string clusterManifestPathInImageStore, CancellationToken cancellationToken);

        Task<OperationResult> CopyClusterPackageAsync(string codePackagePath, string clusterManifestPath, string imageStoreConnectionString, string codePackagePathInImageStore, string clusterManifestPathInImageStore, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> CopyWindowsFabricServicePackageToNodeAsync(string serviceManifestName, string applicationTypeName, string applicationTypeVersion, string nodeName, PackageSharingPolicyDescription[] policies, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> StartRepairTaskAsync(string nodeName, SystemNodeRepairAction nodeAction, string[] nodeNames, string customAction, NodeImpactLevel nodeImpact, string taskId, string description, TimeSpan timeout, CancellationToken cancellationToken);

        ////
        //// REST Specific APIs
        ////
        Task<OperationResult<Application>> GetApplicationByIdRestAsync(string applicationId, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<Node>> GetNodeAsync(string nodeName, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<ServicePartitionList>> GetPartitionListRestAsync(string applicationName, string serviceName, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<ServiceReplicaList>> GetReplicaListRestAsync(Uri applicationName, Uri serviceName, Guid partitionId, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<Service>> GetServiceByIdRestAsync(string applicationId, string serviceId, TimeSpan timeout, CancellationToken cancellationToken);

        ////
        //// BRS commands
        ////
        Task<OperationResult> CreateBackupPolicyAsync(string policyJson);

        Task<OperationResult> EnableBackupProtectionAsync(string policyNameJson, string fabricUri, TimeSpan timeout);

        ////
        //// TODO, API, Remove or Rename?
        ////

        Task<OperationResult<Partition>> GetServicePartitionAsync(Uri serviceName, Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<Replica>> GetServicePartitionReplicaAsync(Guid partitionId, long id, bool isStateful, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<ApplicationType>> GetApplicationTypeAsync(string applicationTypeName, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<ApplicationList>> GetApplicationPagedListAsync(ApplicationQueryDescription applicationQueryDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> AddUnreliableTransportBehaviorAsync(string nodeName, string name, System.Fabric.Testability.UnreliableTransportBehavior behavior, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> RemoveUnreliableTransportBehaviorAsync(string nodeName, string name, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> AddUnreliableLeaseBehaviorAsync(string nodeName, string behaviorString, string alias, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> RemoveUnreliableLeaseBehaviorAsync(string nodeName, string alias, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> RemoveApplicationPackageAsync(string applicationPackagePathInImageStore, string imageStoreConnectionString, TimeSpan timeout, CancellationToken cancellationToken);

        event EventHandler<WinFabricServiceResolutionChangeEventArgs> ServiceResolutionChanged;

        OperationResult<long> RegisterServicePartitionResolutionChangeHandler(Uri name, object partitionKey);

        OperationResult UnregisterServicePartitionResolutionChangeHandler(long callbackId);

        Task<OperationResult> StartNodeNativeAsync(string nodeName, BigInteger instanceId, string ipAddressOrFQDN, int clusterConnectionPort, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> RegisterServiceNotificationFilterAsync(ServiceNotificationFilterDescription filterDescription, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> UnregisterServiceNotificationFilterAsync(long filterId, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> StopNodeNativeAsync(string nodeName, BigInteger instanceId, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> RestartNodeNativeAsync(string nodeName, BigInteger instanceId, bool createFabricDump, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<RestartNodeResult>> RestartNodeAsync(ReplicaSelector replicaSelector, bool createFabricDump, CompletionMode completionMode, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<RestartNodeResult>> RestartNodeAsync(string nodeName, BigInteger instanceId, bool createFabricDump, CompletionMode completionMode, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> RestartDeployedCodePackageNativeAsync(string nodeName, Uri applicationName, string serviceManifestName, string servicePackageActivationId, string codePackageName, long instanceId, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> RestartReplicaInternalAsync(string nodeName, Guid partitionId, long replicaOrInstanceId, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> RemoveReplicaInternalAsync(string nodeName, Guid partitionId, long replicaOrInstanceId, bool forceRemove, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> StartPartitionDataLossAsync(Guid operationId, PartitionSelector partitionSelector, DataLossMode dataLossMode, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<PartitionDataLossProgress>> GetPartitionDataLossProgressAsync(Guid operationId, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> StartPartitionQuorumLossAsync(Guid operationId, PartitionSelector partitionSelector, QuorumLossMode quorumlossMode, TimeSpan quorumlossDuration, TimeSpan operationTimeout, CancellationToken cancellationToken);

        Task<OperationResult<PartitionQuorumLossProgress>> GetPartitionQuorumLossProgressAsync(Guid operationId, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> StartPartitionRestartAsync(Guid operationId, PartitionSelector partitionSelector, RestartPartitionMode restartPartitionMode, TimeSpan operationTimeout, CancellationToken cancellationToken);

        Task<OperationResult<PartitionRestartProgress>> GetPartitionRestartProgressAsync(Guid operationId, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> CancelTestCommandAsync(Guid operationId, bool force, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult<TestCommandStatusList>> GetTestCommandStatusListAsync(TestCommandStateFilter stateFilter, TestCommandTypeFilter typeFilter, TimeSpan timeout, CancellationToken cancellationToken);

        Task<OperationResult> StartChaosAsync(
            ChaosParameters chaosTestScenarioParameters,
            TimeSpan operationTimeout,
            CancellationToken cancellationToken);

        Task<OperationResult<ChaosDescription>> GetChaosAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task<OperationResult<ChaosScheduleDescription>> GetChaosScheduleAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task<OperationResult> SetChaosScheduleAsync(
            string scheduleJson,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task<OperationResult<ChaosEventsSegment>> GetChaosEventsAsync(
            DateTime startTime,
            DateTime endTime,
            string continuationToken,
            long maxResults,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task<OperationResult<ChaosReport>> GetChaosReportAsync(
            DateTime startTime,
            DateTime endTime,
            string continuationToken,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task<OperationResult> StopChaosAsync(
            TimeSpan operationTimeout,
            CancellationToken cancellationToken);

        Task<OperationResult> StartNodeTransitionAsync(
            NodeTransitionDescription description,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task<OperationResult<NodeTransitionProgress>> GetNodeTransitionProgressAsync(
            Guid operationId,
            string nodeName,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task<OperationResult> StartPartitionDataLossRestAsync(
            Guid operationId,
            Uri serviceName,
            Guid partitionId,
            DataLossMode dataLossMode,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task<OperationResult<PartitionDataLossProgress>> GetPartitionDataLossProgressRestAsync(
            Guid operationId,
            Uri serviceName,
            Guid partitionId,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task<OperationResult> StartPartitionQuorumLossRestAsync(
            Guid operationId,
            Uri serviceName,
            Guid partitionId,
            QuorumLossMode quorumlossMode,
            TimeSpan quorumlossDuration,
            TimeSpan operationTimeout,
            CancellationToken cancellationToken);

        Task<OperationResult<PartitionQuorumLossProgress>> GetPartitionQuorumLossProgressRestAsync(
            Guid operationId,
            Uri serviceName,
            Guid partitionId,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task<OperationResult> StartPartitionRestartRestAsync(
            Guid operationId,
            Uri serviceName,
            Guid partitionId,
            RestartPartitionMode restartPartitionMode,
            TimeSpan operationTimeout,
            CancellationToken cancellationToken);

        Task<OperationResult<PartitionRestartProgress>> GetPartitionRestartProgressRestAsync(
            Guid operationId,
            Uri serviceName,
            Guid partitionId,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        Task<OperationResult<List<string>>> GetEventsStorePartitionEventsRestAsync(
            Guid partitionId,
            DateTime startTimeUtc,
            DateTime endTimeUtc,
            IList<string> eventsTypesFilter,
            bool excludeAnalysisEvent,
            bool skipCorrelationLookup);
    }
}


#pragma warning restore 1591
