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
    using System.Fabric;
    using System.Fabric.Chaos.DataStructures;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.ImageStore;
    using System.Fabric.Query;
    using System.Fabric.Result;
    using System.Fabric.Testability.Client.Structures;
    using System.Linq;
    using System.Numerics;
    using System.Threading;
    using System.Threading.Tasks;
    using Repair;

    public class FabricClientObject
    {
        protected FabricClientObject()
        {
            Type type = this.GetType();
            this.SupportedMethods =
                type.GetMethods().Where(m => m.GetBaseDefinition().DeclaringType != m.DeclaringType).Select(m => m.Name).ToList().AsReadOnly();
        }

        public IReadOnlyList<string> SupportedMethods { get; private set; }

        ////
        //// Application Management Client APIs
        ////

        public virtual Task<OperationResult> CreateApplicationAsync(ApplicationDescription applicationDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> UpdateApplicationAsync(ApplicationUpdateDescription updateDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> DeleteApplicationAsync(DeleteApplicationDescription deleteApplicationDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        //// TODO, API, Missing GetApplicationManifestAsync
        public virtual Task<OperationResult<ApplicationUpgradeProgress>> GetApplicationUpgradeProgressAsync(Uri name, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> MoveNextApplicationUpgradeDomainAsync(ApplicationUpgradeProgress realProgress, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> ProvisionApplicationAsync(string applicationBuildPath, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> ProvisionApplicationAsync(ProvisionApplicationTypeDescriptionBase description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> UnprovisionApplicationAsync(string applicationTypeName, string applicationTypeVersion, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> UnprovisionApplicationAsync(UnprovisionApplicationTypeDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> UpgradeApplicationAsync(ApplicationUpgradeDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> UpdateApplicationUpgradeAsync(ApplicationUpgradeUpdateDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> CopyApplicationPackageAsync(string applicationPackagePath, string imageStoreConnectionString, string applicationPackagePathInImageStore, bool compress, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public virtual Task<OperationResult> RemoveApplicationPackageAsync(string applicationPackagePathInImageStore, string imageStoreConnectionString, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public virtual Task<OperationResult> PutApplicationResourceAsync(string applicationName, string descriptionJson, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> DeleteApplicationResourceAsync(string applicationName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<ApplicationResourceList>> GetApplicationResourceListAsync(string applicationName, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<ServiceResourceList>> GetServiceResourceListAsync(string applicationName, string serviceName, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<ReplicaResourceList>> GetReplicaResourceListAsync(string applicationName, string serviceName, string replicaName, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        ////
        //// Image Store Client APIs
        ////
        public virtual Task<OperationResult<ImageStoreContentResult>> ListImageStoreContentAsync(string remoteLocation, string imageStoreConnectionString, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<ImageStorePagedContentResult>> ListImageStorePagedContentAsync(ImageStoreListDescription listDescription, string imageStoreConnectionString, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> DeleteImageStoreContentAsync(string remoteLocation, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> CopyImageStoreContentAsync(WinFabricCopyImageStoreContentDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> UploadChunkAsync(string remoteLocation, string sessionId, UInt64 startPosition, UInt64 endPosition, string filePath, UInt64 fileSize, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> DeleteUploadSessionAsync(Guid sessionId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<UploadSession>> ListUploadSessionAsync(Guid SessionId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> CommitUploadSessionAsync(Guid SessionId, TimeSpan tiemout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        ////
        //// Cluster Management Client APIs
        ////

        public virtual Task<OperationResult> ActivateNodeAsync(string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> DeactivateNodeAsync(string nodeName, NodeDeactivationIntent deactivationIntent, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<string>> GetClusterManifestAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<string>> GetClusterManifestAsync(ClusterManifestQueryDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<string>> GetClusterVersionAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<FabricOrchestrationUpgradeProgress>> GetFabricConfigUpgradeProgressAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<string>> GetFabricConfigurationAsync(string apiVersion, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<string>> GetUpgradeOrchestrationServiceStateAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<FabricUpgradeOrchestrationServiceState>> SetUpgradeOrchestrationServiceStateAsync(string state, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<FabricUpgradeProgress>> GetFabricUpgradeProgressAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> MoveNextFabricUpgradeDomainAsync(FabricUpgradeProgress realProgress, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> ProvisionFabricAsync(string codePackagePath, string clusterManifestFilePath, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> StartNodeInternalAsync(string nodeName, BigInteger instanceId, string ipAddressOrFQDN, int clusterConnectionPort, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> StopNodeInternalAsync(string nodeName, BigInteger instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<RestartNodeResult>> RestartNodeInternalAsync(string nodeName, BigInteger instanceId, bool collectDump, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> RestartDeployedCodePackageInternalAsync(string nodeName, Uri applicationName, string serviceManifestName, string codePackageName, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> CopyClusterPackageAsync(string codePackagePath, string clusterManifestPath, string imageStoreConnectionString, string codePackagePathInImageStore, string clusterManifestPathInImageStore, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public virtual Task<OperationResult> CopyClusterPackageAsync(string codePackagePath, string clusterManifestPath, string imageStoreConnectionString, string codePackagePathInImageStore, string clusterManifestPathInImageStore,
            TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        //// TODO, API, Missing RecoverPartitionAsync

        public virtual Task<OperationResult> RecoverPartitionsAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> RecoverServicePartitionsAsync(Uri serviceName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> RecoverSystemPartitionsAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> RemoveNodeStateAsync(string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> UnprovisionFabricAsync(string codeVersion, string configVersion, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> UpgradeFabricAsync(FabricUpgradeDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> UpgradeFabricStandaloneAsync(ConfigurationUpgradeDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> UpdateFabricUpgradeAsync(FabricUpgradeUpdateDescription updateDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        ////
        //// Health Client APIs
        ////

        public virtual Task<OperationResult<ApplicationHealth>> GetApplicationHealthAsync(ApplicationHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<ClusterHealth>> GetClusterHealthAsync(ClusterHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<ClusterHealthChunk>> GetClusterHealthChunkAsync(ClusterHealthChunkQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<NodeHealth>> GetNodeHealthAsync(NodeHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<PartitionHealth>> GetPartitionHealthAsync(PartitionHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<ReplicaHealth>> GetReplicaHealthAsync(ReplicaHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<ServiceHealth>> GetServiceHealthAsync(ServiceHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<DeployedServicePackageHealth>> GetDeployedServicePackageHealthAsync(DeployedServicePackageHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<DeployedApplicationHealth>> GetDeployedApplicationHealthAsync(DeployedApplicationHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual OperationResult ReportHealth(HealthReport healthReport, HealthReportSendOptions sendOptions)
        {
            throw new NotSupportedException();
        }

        ////
        //// Property Management Client APIs
        ////

        public virtual Task<OperationResult> CreateNameAsync(Uri name, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> DeleteNameAsync(Uri name, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> DeletePropertyAsync(Uri name, string propertyName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<PropertyEnumerationResult>> EnumeratePropertiesAsync(Uri name, PropertyEnumerationResult previousResult, bool includeValue, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<NameEnumerationResult>> EnumerateSubNamesAsync(Uri name, NameEnumerationResult previousResult, bool recursive, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<NamedProperty>> GetPropertyAsync(Uri name, string propertyName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<NamedPropertyMetadata>> GetPropertyMetadataAsync(Uri name, string propertyName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<bool>> NameExistsAsync(Uri name, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> PutCustomPropertyOperationAsync(Uri name, PutCustomPropertyOperation putCustomPropertyOperation, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> PutPropertyAsync(Uri name, string propertyName, object data, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<PropertyBatchResult>> SubmitPropertyBatchAsync(Uri name, ICollection<PropertyBatchOperation> operations, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        ////
        //// Query Client APIs
        ////

        public virtual Task<OperationResult<Application>> GetApplicationAsync(Uri applicationName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<string>> GetApplicationManifestAsync(string applicationTypeName, string applicationTypeVersion, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public virtual Task<OperationResult<NodeLoadInformation>> GetNodeLoadAsync(string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public virtual Task<OperationResult<ClusterLoadInformation>> GetClusterLoadAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public virtual Task<OperationResult<ReplicaLoadInformation>> GetReplicaLoadAsync(Guid partitionId, long replicaId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public virtual Task<OperationResult<PartitionLoadInformation>> GetPartitionLoadAsync(Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public virtual Task<OperationResult<ApplicationLoadInformation>> GetApplicationLoadAsync(string applicationName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        //// TODO, API, Missing GetApplicationServiceTypeListAsync
        public virtual Task<OperationResult<ApplicationType[]>> GetApplicationTypeListAsync(string applicationTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<ApplicationTypePagedList>> GetApplicationTypePagedListAsync(PagedApplicationTypeQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        // This overload exists only to test the PowerShell specific parameters which do not exist elsewhere.
        public virtual Task<OperationResult<ApplicationTypePagedList>> GetApplicationTypePagedListAsync(PagedApplicationTypeQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken, bool getPagedResults)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<DeployedApplication[]>> GetDeployedApplicationListAsync(string nodeName, Uri applicationNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<DeployedApplicationPagedList>> GetDeployedApplicationPagedListAsync(
            PagedDeployedApplicationQueryDescription queryDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<DeployedCodePackage[]>> GetDeployedCodePackageListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, string codePackageNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<DeployedServiceReplica[]>> GetDeployedReplicaListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, Guid? partitionIdFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<DeployedServicePackage[]>> GetDeployedServicePackageListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<DeployedServiceType[]>> GetDeployedServiceTypeListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, string serviceTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<NodeList>> GetNodeListAsync(NodeQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<ServicePartitionList>> GetPartitionListAsync(Uri serviceName, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<ServiceReplicaList>> GetReplicaListAsync(Guid partitionId, long replicaIdOrInstanceId, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<ServiceList>> GetServicePagedListAsync(ServiceQueryDescription serviceQueryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<ServiceType[]>> GetServiceTypeListAsync(string applicationTypeName, string applicationTypeVersion, string serviceTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<ServiceNameResult>> GetServiceNameAsync(Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<ApplicationNameResult>> GetApplicationNameAsync(Uri serviceName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        ////
        //// ServiceGroup Management Client APIs
        ////

        public virtual Task<OperationResult> RestartReplicaInternalAsync(string nodeName, Guid partitionId, long replicaOrInstanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> RemoveReplicaInternalAsync(string nodeName, Guid partitionId, long replicaOrInstanceId, bool forceRemove, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> CreateServiceGroupAsync(ServiceGroupDescription winFabricServiceGroupDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> DeleteServiceGroupAsync(Uri name, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        // TODO, API, Missing GetServiceGroupDescriptionAsync
        public virtual Task<OperationResult> UpdateServiceGroupAsync(ServiceKind serviceKind, Uri serviceGroupName, int targetReplicaSetSize, TimeSpan replicaRestartWaitDuration, TimeSpan quorumLossWaitDuration, int instanceCount, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        ////
        //// Service Management Client APIs
        ////

        public virtual Task<OperationResult> CreateServiceAsync(ServiceDescription winFabricServiceDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> CreateServiceFromTemplateAsync(
            Uri applicationName,
            Uri serviceName,
            string serviceDnsName,
            string serviceTypeName,
            ServicePackageActivationMode servicePackageActivationMode,
            byte[] initializationData,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> DeleteServiceAsync(DeleteServiceDescription deleteServiceDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<ServiceDescription>> GetServiceDescriptionAsync(Uri name, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<string>> GetServiceManifestAsync(string applicationTypeName, string applicationTypeVersion, string serviceManifestName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual OperationResult<long> RegisterServicePartitionResolutionChangeHandler(Uri name, object partitionKey, ServicePartitionResolutionChangeHandler callback)
        {
            throw new NotSupportedException();
        }

        public virtual Task<ResolvedServicePartition> ResolveServiceAsync(Uri name, object partitionKey, ResolvedServicePartition previousServicePartition, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual OperationResult UnregisterServicePartitionResolutionChangeHandler(long callbackId)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> RegisterServiceNotificationFilterAsync(ServiceNotificationFilterDescription filterDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> UnregisterServiceNotificationFilterAsync(long filterId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> UpdateServiceAsync(Uri name, ServiceUpdateDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        ////
        //// Repair Management Client APIs
        ////

        public virtual Task<OperationResult<Int64>> CreateRepairTaskAsync(WinFabricRepairTask repairTask, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<Int64>> CancelRepairTaskAsync(string repairTaskId, Int64 version, bool requestAbort, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<Int64>> ForceApproveRepairTaskAsync(string repairTaskId, Int64 version, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> DeleteRepairTaskAsync(string repairTaskId, Int64 version, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<Int64>> UpdateRepairExecutionStateAsync(WinFabricRepairTask repairTask, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<WinFabricRepairTask[]>> GetRepairTaskListAsync(string taskIdFilter, WinFabricRepairTaskStateFilter stateFilter, string executorFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        ////
        //// Infrastructure Apis
        ////

        public virtual Task<OperationResult<string>> InvokeInfrastructureCommandAsync(Uri serviceName, string command, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        ////
        //// PowerShell Specific APIs
        ////

        public virtual Task<OperationResult<string>> InvokeEncryptSecretAsync(string certificateThumbprint, string certificateStoreLocation, string text, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> NewNodeConfigurationAsync(string clusterManifestPath, string computerName, string userName, string password, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> UpdateNodeConfigurationAsync(string clusterManifestPath, string computerName, string userName, string password, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> RemoveNodeConfigurationAsync(string clusterManifestPath, string computerName, string userName, string password, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<bool>> TestClusterConnectionAsync(CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<bool>> TestClusterManifestAsync(string clusterManifestPath, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<bool>> TestApplicationPackageAsync(string applicationPackagePath, string imageStoreConnectionString, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public virtual Task<OperationResult> CopyApplicationPackageAndShowProgressAsync(string applicationPackagePath, string imageStoreConnectionString, string applicationPackagePathInImageStore, bool compress, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public virtual Task<OperationResult> CopyWindowsFabricServicePackageToNodeAsync(string serviceManifestName, string applicationTypeName, string applicationTypeVersion, string nodeName, PackageSharingPolicyDescription[] policies, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public virtual Task<OperationResult> StartRepairTaskAsync(string nodeName, SystemNodeRepairAction nodeAction, string[] nodeNames, string customAction, NodeImpactLevel nodeImpact, string taskId, string description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        ////
        //// BRS specific APIs
        ////

        public virtual Task<OperationResult> CreateBackupPolicyAsync(string policyJson)
        {
            throw new NotImplementedException();
        }

        public virtual Task<OperationResult> EnableBackupProtectionAsync(string policyNameJson, string fabricUri, TimeSpan tiemout)
        {
            throw new NotImplementedException();
        }

        ////
        //// REST Specific APIs
        ////

        public virtual Task<OperationResult<Application>> GetApplicationByIdRestAsync(string applicationId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<Service>> GetServiceByIdRestAsync(string applicationId, string serviceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<Node>> GetNodeAsync(string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<ServicePartitionList>> GetPartitionListRestAsync(string applicationName, string serviceName, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<ServiceReplicaList>> GetReplicaListRestAsync(Uri applicationName, Uri serviceName, Guid partitionId, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        ////
        //// TODO, API, Remove or Rename?
        ////

        public virtual Task<OperationResult<Partition>> GetServicePartitionAsync(Uri serviceName, Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<Replica>> GetServicePartitionReplicaAsync(Guid partitionId, long id, bool isStateful, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<ApplicationType>> GetApplicationTypeAsync(string applicationTypeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<ApplicationList>> GetApplicationPagedListAsync(ApplicationQueryDescription applicationQueryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> AddUnreliableTransportBehaviorAsync(string nodeName, string name, System.Fabric.Testability.UnreliableTransportBehavior behavior, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> RemoveUnreliableTransportBehaviorAsync(string nodeName, string name, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> AddUnreliableLeaseBehaviorAsync(string nodeName, string behaviorString, string alias, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> RemoveUnreliableLeaseBehaviorAsync(string nodeName, string alias, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> StartNodeAsync(string nodeName, BigInteger nodeInstance, string ipAddressOrFQDN, int clusterConnectionPort, CompletionMode completionMode, TimeSpan operationTimeout, CancellationToken token)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> StopNodeAsync(string nodeName, BigInteger nodeInstance, CompletionMode completionMode, TimeSpan operationTimeout, CancellationToken token)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<RestartNodeResult>> RestartNodeAsync(string nodeName, BigInteger nodeInstance, bool createFabricDump, CompletionMode completionMode, TimeSpan operationTimeout, CancellationToken token)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<RestartNodeResult>> RestartNodeAsync(ReplicaSelector replicaSelector, bool createFabricDump, CompletionMode completionMode, TimeSpan operationTimeout, CancellationToken token)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> RestartDeployedCodePackageAsync(Uri applicationName, ReplicaSelector replicaSelector, CompletionMode completionMode, TimeSpan operationTimeout, CancellationToken token)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> RestartDeployedCodePackageAsync(string nodeName, Uri applicationName, string serviceManifestName, string servicePackageActivationId, string codePackageName, long codePackageInstanceId, CompletionMode completionMode, TimeSpan operationTimeout, CancellationToken token)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> RestartReplicaAsync(ReplicaSelector replicaSelector, CompletionMode completionMode, TimeSpan operationTimeout, CancellationToken token)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> RestartReplicaAsync(string nodeName, Guid partitionId, long replicaId, CompletionMode completionMode, TimeSpan operationTimeout, CancellationToken token)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> RemoveReplicaAsync(ReplicaSelector replicaSelector, CompletionMode completionMode, bool forceRemove, TimeSpan operationTimeout, CancellationToken token)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> RemoveReplicaAsync(string nodeName, Guid partitionId, long replicaId, CompletionMode completionMode, bool forceRemove, TimeSpan operationTimeout, CancellationToken token)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> ValidateApplicationAsync(Uri applicationName, TimeSpan operationTimeout, CancellationToken token)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> ValidateServiceAsync(Uri serviceName, TimeSpan operationTimeout, CancellationToken token)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> InvokeDataLossAsync(PartitionSelector partitionSelector, DataLossMode datalossMode, TimeSpan operationTimeout, CancellationToken token)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> RestartPartitionAsync(PartitionSelector partitionSelector, RestartPartitionMode restartPartitionMode, TimeSpan operationTimeout, CancellationToken token)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> MovePrimaryReplicaAsync(string nodeName, PartitionSelector partitionSelector, bool ignoreConstraints, TimeSpan operationTimeout, CancellationToken token)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> MoveSecondaryReplicaAsync(string currentNodeName, string newNodeName, PartitionSelector partitionSelector, bool ignoreConstraints, TimeSpan operationTimeout, CancellationToken token)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> CleanTestStateAsync(TimeSpan operationTimeout, CancellationToken token)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> InvokeQuorumLossAsync(PartitionSelector partitionSelector, QuorumLossMode quorumlossMode, TimeSpan quorumlossDuration, TimeSpan operationTimeout, CancellationToken token)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> StartNodeNativeAsync(string nodeName, BigInteger instanceId, string ipAddressOrFQDN, int clusterConnectionPort, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> StopNodeNativeAsync(string nodeName, BigInteger instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> RestartNodeNativeAsync(string nodeName, BigInteger instanceId, bool createFabricDump, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> RestartDeployedCodePackageNativeAsync(string nodeName, Uri applicationName, string serviceManifestName, string servicePackageActivationId, string codePackageName, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> StartPartitionDataLossAsync(Guid operationId, PartitionSelector partitionSelector, DataLossMode dataLossMode, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<PartitionDataLossProgress>> GetPartitionDataLossProgressAsync(Guid operationId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> StartPartitionQuorumLossAsync(Guid operationId, PartitionSelector partitionSelector, QuorumLossMode quorumlossMode, TimeSpan quorumlossDuration, TimeSpan operationTimeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<PartitionQuorumLossProgress>> GetPartitionQuorumLossProgressAsync(Guid operationId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> StartPartitionRestartAsync(Guid operationId, PartitionSelector partitionSelector, RestartPartitionMode restartPartitionMode, TimeSpan operationTimeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<PartitionRestartProgress>> GetPartitionRestartProgressAsync(Guid operationId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> CancelTestCommandAsync(Guid operationId, bool force, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<TestCommandStatusList>> GetTestCommandStatusListAsync(TestCommandStateFilter stateFilter, TestCommandTypeFilter typeFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> StartChaosAsync(
            ChaosParameters chaosTestScenarioParameters,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<ChaosDescription>> GetChaosAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<ChaosEventsSegment>> GetChaosEventsAsync(
            DateTime startTime,
            DateTime endTime,
            string continuationToken,
            long maxResults,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<ChaosScheduleDescription>> GetChaosScheduleAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> SetChaosScheduleAsync(
            string schedule,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<ChaosReport>> GetChaosReportAsync(
            DateTime startTime,
            DateTime endTime,
            string continuationToken,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> StopChaosAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> StartNodeTransitionAsync(
            NodeTransitionDescription description,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<NodeTransitionProgress>> GetNodeTransitionProgressAsync(
            Guid operationId,
            string nodeName,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> StartPartitionDataLossRestAsync(
            Guid operationId,
            Uri serviceName,
            Guid partitionId,
            DataLossMode dataLossMode,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<PartitionDataLossProgress>> GetPartitionDataLossProgressRestAsync(
            Guid operationId,
            Uri serviceName,
            Guid partitionId,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> StartPartitionQuorumLossRestAsync(
            Guid operationId,
            Uri serviceName,
            Guid partitionId,
            QuorumLossMode quorumlossMode,
            TimeSpan quorumlossDuration,
            TimeSpan operationTimeout,
            CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<PartitionQuorumLossProgress>> GetPartitionQuorumLossProgressRestAsync(
            Guid operationId,
            Uri serviceName,
            Guid partitionId,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult> StartPartitionRestartRestAsync(
            Guid operationId,
            Uri serviceName,
            Guid partitionId,
            RestartPartitionMode restartPartitionMode,
            TimeSpan operationTimeout,
            CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<PartitionRestartProgress>> GetPartitionRestartProgressRestAsync(
            Guid operationId,
            Uri serviceName,
            Guid partitionId,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotSupportedException();
        }

        public virtual Task<OperationResult<List<string>>> GetEventsStorePartitionEventsRestAsync(
            Guid partitionId,
            DateTime startTimeUtc,
            DateTime endTimeUtc,
            IList<string> eventsTypesFilter,
            bool excludeAnalysisEvent,
            bool skipCorrelationLookup)
        {
            throw new NotSupportedException();
        }
    }
}


#pragma warning restore 1591
