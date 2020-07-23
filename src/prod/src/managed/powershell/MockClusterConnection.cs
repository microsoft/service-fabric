// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using Microsoft.ServiceFabric.Preview.Client;
    using Microsoft.ServiceFabric.Preview.Client.Description;
    using Microsoft.ServiceFabric.Preview.Client.Query;
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Chaos.DataStructures;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Query;
    using System.Fabric.Repair;
    using System.Fabric.Result;
    using System.Fabric.SecretStore;
    using System.Fabric.Testability.Scenario;
    using System.Linq;
    using System.Numerics;
    using System.Threading;
    using System.Threading.Tasks;
    using WEX.TestExecution;

    public class MockClusterConnection : IClusterConnection
    {
        private int apiCalls;

        public MockClusterConnection()
        {
            this.apiCalls = 0;
            this.ApplicationTypes = new ApplicationTypeList();
            this.Applications = new ApplicationList();
            this.Services = new Dictionary<Uri, ServiceList>();
            this.Partitions = new Dictionary<Uri, ServicePartitionList>();
            this.Replicas = new Dictionary<Guid, ServiceReplicaList>();
            this.ServiceTypes = new Dictionary<ApplicationType, ServiceTypeList>();
            this.Nodes = new NodeList();
            this.SystemServices = new ServiceList();
            this.AdhocServices = new ServiceList();
            this.FabricClientSettings = new FabricClientSettings();
            this.ClusterCodeVersions = new ProvisionedFabricCodeVersionList();
            this.ClusterConfigVersions = new ProvisionedFabricConfigVersionList();
        }

        public void Reset()
        {
            this.apiCalls = 0;
        }

        public FabricClient FabricClient
        {
            get;
            set;
        }

        public FabricClientSettings FabricClientSettings
        {
            get;
            set;
        }

        public bool ExpectNotEqual
        {
            get;
            set;
        }

        public SecurityCredentials SecurityCredentials
        {
            get;
            set;
        }

        public string[] ConnectionEndpoint
        {
            get;
            set;
        }

        public string[] ApiNameArray
        {
            get;
            set;
        }

        public Exception Exception
        {
            get;
            set;
        }

        public Uri Name
        {
            get;
            set;
        }

        public Uri[] ApplicationNameArray
        {
            get;
            set;
        }

        public string[] ApplicationTypeNameArray
        {
            get;
            set;
        }

        public string[] ApplicationTypeVersionArray
        {
            get;
            set;
        }

        public Uri[] ServiceNameArray
        {
            get;
            set;
        }

        public ServicePackageActivationMode[] ServicePackageActivationModeArray
        {
            get;
            set;
        }

        public string[] ServicePackageActivationIdArray
        {
            get;
            set;
        }

        public string[] ServiceTypeNameArray
        {
            get;
            set;
        }

        public string[] NodeNameArray
        {
            get;
            set;
        }

        public Guid[] PartitionIdArray
        {
            get;
            set;
        }

        public long[] ReplicaIdArray
        {
            get;
            set;
        }

        public ServiceReplicaStatusFilter[] ReplicaStatusFilterArray
        {
            get;
            set;
        }

        public string[] CodePackageNameArray
        {
            get;
            set;
        }

        public string[] ServiceManifestNameArray
        {
            get;
            set;
        }

        public PartitionSelector[] PartitionSelectorArray
        {
            get;
            set;
        }

        public ReplicaSelector[] ReplicaSelectorArray
        {
            get;
            set;
        }

        public ApplicationList Applications
        {
            get;
            set;
        }

        public ComposeDeploymentStatusList ComposeDeploymentStatusList
        {
            get;
            set;
        }

        public DeployedApplication DeployedApplication
        {
            get;
            set;
        }

        public DeployedCodePackage DeployedCodePackage
        {
            get;
            set;
        }

        public DeployedServiceReplica DeployedServiceReplica
        {
            get;
            set;
        }

        public DeployedServiceReplicaDetail DeployedServiceReplicaDetail
        {
            get;
            set;
        }

        public DeployedServicePackage DeployedServicePackage
        {
            get;
            set;
        }

        public DeployedServiceType DeployedServiceType
        {
            get;
            set;
        }

        public ApplicationTypeList ApplicationTypes
        {
            get;
            set;
        }

        public Dictionary<Uri, ServiceList> Services
        {
            get;
            set;
        }

        public Dictionary<Uri, ServicePartitionList> Partitions
        {
            get;
            set;
        }

        public Dictionary<Guid, ServiceReplicaList> Replicas
        {
            get;
            set;
        }

        public Dictionary<ApplicationType, ServiceTypeList> ServiceTypes
        {
            get;
            set;
        }

        public ServiceList SystemServices
        {
            get;
            set;
        }

        public ServiceList AdhocServices
        {
            get;
            set;
        }

        public NodeList Nodes
        {
            get;
            set;
        }

        public string MspFilePath
        {
            get;
            set;
        }

        public string ClusterManifestFilePath
        {
            get;
            set;
        }

        public string CodeVersion
        {
            get;
            set;
        }

        public string ConfigVersion
        {
            get;
            set;
        }

        public ProvisionedFabricCodeVersionList ClusterCodeVersions
        {
            get;
            set;
        }

        public ProvisionedFabricConfigVersionList ClusterConfigVersions
        {
            get;
            set;
        }

        public FabricUpgradeDescription FabricUpgradeDescription
        {
            get;
            set;
        }

        public FabricUpgradeUpdateDescription FabricUpgradeUpdateDescription
        {
            get;
            set;
        }

        public FabricUpgradeProgress FabricUpgradeProgress
        {
            get;
            set;
        }

        public string ApplicationBuildPath
        {
            get;
            set;
        }

        public ApplicationUpgradeDescription ApplicationUpgradeDescription
        {
            get;
            set;
        }

        public ApplicationUpgradeUpdateDescription ApplicationUpgradeUpdateDescription
        {
            get;
            set;
        }

        public ApplicationUpgradeProgress ApplicationUpgradeProgress
        {
            get;
            set;
        }

        public ComposeDeploymentUpgradeProgress ComposeDeploymentUpgradeProgress
        {
            get;
            set;
        }

        public ApplicationDescription ApplicationDescription
        {
            get;
            set;
        }

        public ApplicationUpdateDescription ApplicationUpdateDescription
        {
            get;
            set;
        }

        public string ApplicationManifest
        {
            get;
            set;
        }

        public ComposeDeploymentDescription ComposeDeploymentDescription
        {
            get;
            set;
        }

        public NodeDeactivationIntent DeactivationIntent
        {
            get;
            set;
        }

        public string ClusterManifest
        {
            get;
            set;
        }

        public string ServiceManifest
        {
            get;
            set;
        }

        public ServiceDescription ServiceDescription
        {
            get;
            set;
        }

        public string[] PartitionKeyStringArray
        {
            get;
            set;
        }

        public long[] PartitionKeyLongArray
        {
            get;
            set;
        }

        public ResolvedServicePartition[] PreviousResultArray
        {
            get;
            set;
        }

        public ResolvedServicePartition CurrentResult
        {
            get;
            set;
        }

        public ServiceUpdateDescription ServiceUpdateDescription
        {
            get;
            set;
        }

        public ServiceGroupUpdateDescription ServiceGroupUpdateDescription
        {
            get;
            set;
        }

        public object HealthPolicy
        {
            get;
            set;
        }

        public ApplicationHealth ApplicationHealth
        {
            get;
            set;
        }

        public NodeHealth NodeHealth
        {
            get;
            set;
        }

        public PartitionHealth PartitionHealth
        {
            get;
            set;
        }

        public PartitionLoadInformation PartitionLoadInformation
        {
            get;
            set;
        }

        public ReplicaHealth ReplicaHealth
        {
            get;
            set;
        }

        public ServiceHealth ServiceHealth
        {
            get;
            set;
        }

        public DeployedApplicationHealth DeployedApplicationHealth
        {
            get;
            set;
        }

        public DeployedServicePackageHealth DeployedServicePackageHealth
        {
            get;
            set;
        }

        public ClusterHealth ClusterHealth
        {
            get;
            set;
        }

        public ClusterHealthChunk ClusterHealthChunk
        {
            get;
            set;
        }

        public HealthReport healthReport
        {
            get;
            set;
        }

        public ClusterLoadInformation ClusterLoadInformation
        {
            get;
            set;
        }

        public NodeLoadInformation NodeLoadInformation
        {
            get;
            set;
        }

        public ReplicaLoadInformation ReplicaLoadInformation
        {
            get;
            set;
        }

        public UnplacedReplicaInformation UnplacedReplicaInformation
        {
            get;
            set;
        }

        public ApplicationLoadInformation ApplicationLoadInformation
        {
            get;
            set;
        }

        public ServiceNameResult ServiceName
        {
            get;
            set;
        }

        public ApplicationNameResult ApplicationName
        {
            get;
            set;
        }

        public IList<NodeHealthStateFilter> NodeHealthStateFilters
        {
            get;
            set;
        }

        public IList<ApplicationHealthStateFilter> ApplicationHealthStateFilters
        {
            get;
            set;
        }

        public HealthEventsFilter EventsHealthFilter
        {
            get;
            set;
        }

        public DeployedApplicationHealthStatesFilter DeployedApplicationsHealthFilter
        {
            get;
            set;
        }

        public ServiceHealthStatesFilter ServicesHealthFilter
        {
            get;
            set;
        }

        public NodeHealthStatesFilter NodesHealthFilter
        {
            get;
            set;
        }

        public ApplicationHealthStatesFilter ApplicationsHealthFilter
        {
            get;
            set;
        }

        public PartitionHealthStatesFilter PartitionsHealthFilter
        {
            get;
            set;
        }

        public ReplicaHealthStatesFilter ReplicasHealthFilter
        {
            get;
            set;
        }

        public DeployedServicePackageHealthStatesFilter DeployedServicePackagesHealthFilter
        {
            get;
            set;
        }

        public RepairTaskList RepairTasks
        {
            get;
            set;
        }

        public Action<RepairTask> RepairTaskValidator
        {
            get;
            set;
        }

        public DateTime StartTime
        {
            get;
            set;
        }

        public DateTime EndTime
        {
            get;
            set;
        }

        public string HostName
        {
            get;
            set;
        }

        public bool ForceRemove
        {
            get;
            set;
        }

        public string ContinuationToken
        {
            get;
            set;
        }

        public InvokeDataLossResult InvokeDataLossResult
        {
            get;
            set;
        }

        public InvokeDataLossResult[] InvokeDataLossResultArray
        {
            get;
            set;
        }

        public InvokeQuorumLossResult InvokeQuorumLossResult
        {
            get;
            set;
        }

        public InvokeQuorumLossResult[] InvokeQuorumLossResultArray
        {
            get;
            set;
        }

        public MovePrimaryResult MovePrimaryResult
        {
            get;
            set;
        }

        public MovePrimaryResult[] MovePrimaryResultArray
        {
            get;
            set;
        }

        public MoveSecondaryResult MoveSecondaryResult
        {
            get;
            set;
        }

        public MoveSecondaryResult[] MoveSecondaryResultArray
        {
            get;
            set;
        }

        public StartNodeResult StartNodeResult
        {
            get;
            set;
        }

        public StopNodeResult StopNodeResult
        {
            get;
            set;
        }

        public RestartNodeResult RestartNodeResult
        {
            get;
            set;
        }

        public RemoveReplicaResult RemoveReplicaResult
        {
            get;
            set;
        }

        public RestartReplicaResult RestartReplicaResult
        {
            get;
            set;
        }

        public RestartReplicaResult[] RestartReplicaResultArray
        {
            get;
            set;
        }

        public RestartPartitionResult RestartPartitionResult
        {
            get;
            set;
        }

        public RestartPartitionResult[] RestartPartitionResultArray
        {
            get;
            set;
        }

        public RestartDeployedCodePackageResult RestartDeployedCodePackageResult
        {
            get;
            set;
        }

        public void InitializeClaimsMetadata(TimeSpan timeout)
        {
        }

        public Task<bool> NameExistsAsync(Uri name, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.NameExistsApiName);
                Verify.AreEqual(this.Name, name);
                this.apiCalls++;

                return false;
            });
        }

        public Task<ApplicationList> GetApplicationPagedListAsync(
            ApplicationQueryDescription applicationQueryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task<ApplicationList>.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetApplicationListApiName);
                if (applicationQueryDescription.ApplicationNameFilter == null)
                {
                    Verify.IsNull(this.ApplicationNameArray[this.apiCalls]);
                }
                else
                {
                    Verify.AreEqual(this.ApplicationNameArray[this.apiCalls], applicationQueryDescription.ApplicationNameFilter);
                }

                Verify.AreEqual(this.ContinuationToken, applicationQueryDescription.ContinuationToken);
                this.apiCalls++;

                if (applicationQueryDescription.ApplicationNameFilter != null)
                {
                    var result = new ApplicationList();
                    var item = this.Applications.SingleOrDefault(p => p.ApplicationName == applicationQueryDescription.ApplicationNameFilter);
                    if (item != null)
                    {
                        result.Add(item);
                    }

                    return result;
                }

                return this.Applications;
            });
        }

        public Task<ComposeDeploymentStatusList> GetComposeDeploymentStatusPagedListAsync(
            ComposeDeploymentStatusQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task<ComposeDeploymentStatusList>.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetComposeDeploymentPagedListApiName);
                if (queryDescription.DeploymentNameFilter == null)
                {
                    Verify.IsNull(this.ApplicationNameArray[this.apiCalls]);
                }
                else
                {
                    Verify.AreEqual(this.ApplicationNameArray[this.apiCalls], queryDescription.DeploymentNameFilter);
                }

                Verify.AreEqual(this.ContinuationToken, queryDescription.ContinuationToken);
                this.apiCalls++;

                if (queryDescription.DeploymentNameFilter != null)
                {
                    var result = new ComposeDeploymentStatusList();
                    var item = this.ComposeDeploymentStatusList.SingleOrDefault(p => p.DeploymentName == queryDescription.DeploymentNameFilter);
                    if (item != null)
                    {
                        result.Add(item);
                    }

                    return result;
                }

                return this.ComposeDeploymentStatusList;
            });
        }

        public Task<ApplicationTypeList> GetApplicationTypeListAsync(
           string applicationTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task<ApplicationTypeList>.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetApplicationTypeListApiName);
                Verify.AreEqual(this.ApplicationTypeNameArray[this.apiCalls], applicationTypeNameFilter);
                this.apiCalls++;

                if (applicationTypeNameFilter != null)
                {
                    var result = new ApplicationTypeList();
                    var item = this.ApplicationTypes.SingleOrDefault(p => p.ApplicationTypeName == applicationTypeNameFilter);
                    if (item != null)
                    {
                        result.Add(item);
                    }

                    return result;
                }

                return this.ApplicationTypes;
            });
        }

        public Task<ApplicationTypePagedList> GetApplicationTypePagedListAsync(
            PagedApplicationTypeQueryDescription queryDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return Task<ApplicationTypePagedList>.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetApplicationTypeListApiName);
                Verify.AreEqual(this.ApplicationTypeNameArray[this.apiCalls], queryDescription.ApplicationTypeNameFilter);
                this.apiCalls++;

                Verify.AreEqual(this.ContinuationToken, queryDescription.ContinuationToken);
                var result = new ApplicationTypePagedList();
                var matches = this.ApplicationTypes.AsEnumerable();

                if (queryDescription.ApplicationTypeNameFilter != null)
                {
                    matches = this.ApplicationTypes.Where(p => p.ApplicationTypeName == queryDescription.ApplicationTypeNameFilter);
                }

                foreach (var item in matches)
                {
                    if (item != null)
                    {
                        result.Add(item);
                    }
                }

                return result;
            });
        }

        public Task<DeployedApplicationList> GetDeployedApplicationListAsync(
            string nodeName, Uri applicationNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task<DeployedApplicationList>.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetDeployedApplicationListApiName);
                Verify.AreEqual(this.NodeNameArray[this.apiCalls], nodeName);
                Verify.AreEqual(this.ApplicationNameArray[this.apiCalls], applicationNameFilter);
                this.apiCalls++;

                var result = new DeployedApplicationList();
                if (this.DeployedApplication != null)
                {
                    result.Add(this.DeployedApplication);
                }

                return result;
            });
        }

        public Task<DeployedApplicationPagedList> GetDeployedApplicationPagedListAsync(
            PagedDeployedApplicationQueryDescription queryDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return Task<DeployedApplicationPagedList>.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetDeployedApplicationListApiName);
                Verify.AreEqual(this.NodeNameArray[this.apiCalls], queryDescription.NodeName);
                Verify.AreEqual(this.ApplicationNameArray[this.apiCalls], queryDescription.ApplicationNameFilter);
                Verify.AreEqual(this.ContinuationToken, queryDescription.ContinuationToken);
                this.apiCalls++;

                var result = new DeployedApplicationPagedList();
                if (this.DeployedApplication != null)
                {
                    result.Add(this.DeployedApplication);
                }

                return result;
            });
        }

        public Task<DeployedCodePackageList> GetDeployedCodePackageListAsync(
            string nodeName, Uri applicationName, string serviceManifestNameFilter, string codePackageNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task<DeployedCodePackageList>.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetDeployedCodePackageListApiName);
                Verify.AreEqual(this.NodeNameArray[this.apiCalls], nodeName);
                Verify.AreEqual(this.ApplicationNameArray[this.apiCalls], applicationName);
                Verify.AreEqual(this.ServiceManifestNameArray[this.apiCalls], serviceManifestNameFilter);
                Verify.AreEqual(this.CodePackageNameArray[this.apiCalls], codePackageNameFilter);
                this.apiCalls++;

                var result = new DeployedCodePackageList();
                if (this.DeployedCodePackage != null)
                {
                    result.Add(this.DeployedCodePackage);
                }

                return result;
            });
        }

        public Task<DeployedServiceReplicaList> GetDeployedReplicaListAsync(
            string nodeName, Uri applicationName, string serviceManifestNameFilter, Guid? partitionIdFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task<DeployedServiceReplicaList>.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetDeployedServiceReplicaListApiName);
                Verify.AreEqual(this.NodeNameArray[this.apiCalls], nodeName);
                Verify.AreEqual(this.ApplicationNameArray[this.apiCalls], applicationName);
                Verify.AreEqual(this.ServiceManifestNameArray[this.apiCalls], serviceManifestNameFilter);
                Verify.AreEqual(this.PartitionIdArray[this.apiCalls], partitionIdFilter.Value);
                this.apiCalls++;

                var result = new DeployedServiceReplicaList();
                if (this.DeployedServiceReplica != null)
                {
                    result.Add(this.DeployedServiceReplica);
                }

                return result;
            });
        }

        public Task<DeployedServiceReplicaDetail> GetDeployedReplicaDetailAsync(
            string nodeName, Guid partitionId, long replicaId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task<DeployedServiceReplicaDetail>.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetDeployedServiceReplicaDetailApiName);
                Verify.AreEqual(this.NodeNameArray[this.apiCalls], nodeName);
                Verify.AreEqual(this.PartitionIdArray[this.apiCalls], partitionId);
                Verify.AreEqual(this.ReplicaIdArray[this.apiCalls], replicaId);
                this.apiCalls++;

                return this.DeployedServiceReplicaDetail;
            });
        }

        public Task<DeployedServicePackageList> GetDeployedServicePackageListAsync(
            string nodeName, Uri applicationName, string serviceManifestNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task<DeployedServicePackageList>.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetDeployedServiceManifestListApiName);
                Verify.AreEqual(this.NodeNameArray[this.apiCalls], nodeName);
                Verify.AreEqual(this.ApplicationNameArray[this.apiCalls], applicationName);
                Verify.AreEqual(this.ServiceManifestNameArray[this.apiCalls], serviceManifestNameFilter);
                this.apiCalls++;

                var result = new DeployedServicePackageList();
                if (this.DeployedServicePackage != null)
                {
                    result.Add(this.DeployedServicePackage);
                }

                return result;
            });
        }

        public Task<DeployedServiceTypeList> GetDeployedServiceTypeListAsync(
            string nodeName, Uri applicationName, string serviceManifestNameFilter, string serviceTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task<DeployedServiceTypeList>.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetDeployedServiceTypeListApiName);
                Verify.AreEqual(this.NodeNameArray[this.apiCalls], nodeName);
                Verify.AreEqual(this.ApplicationNameArray[this.apiCalls], applicationName);
                Verify.AreEqual(this.ServiceManifestNameArray[this.apiCalls], serviceManifestNameFilter);
                Verify.AreEqual(this.ServiceTypeNameArray[this.apiCalls], serviceTypeNameFilter);
                this.apiCalls++;

                var result = new DeployedServiceTypeList();
                if (this.DeployedServiceType != null)
                {
                    result.Add(this.DeployedServiceType);
                }

                return result;
            });
        }

        public Task<NodeList> GetNodeListAsync(
            string nodeNameFilter, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task<NodeList>.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetNodeListApiName);
                if (string.IsNullOrWhiteSpace(nodeNameFilter))
                {
                    Verify.IsTrue(string.IsNullOrWhiteSpace(this.NodeNameArray[this.apiCalls]));
                }
                else
                {
                    Verify.AreEqual(this.NodeNameArray[this.apiCalls], nodeNameFilter);
                }

                Verify.AreEqual(this.ContinuationToken, continuationToken);

                this.apiCalls++;

                if (nodeNameFilter != null)
                {
                    var result = new NodeList();
                    var item = this.Nodes.SingleOrDefault(p => p.NodeName == nodeNameFilter);
                    if (item != null)
                    {
                        result.Add(item);
                    }

                    return result;
                }

                return this.Nodes;
            });
        }

        public Task<NodeList> GetNodeListAsync(
            string nodeNameFilter, NodeStatusFilter nodeStatusFilter, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task<NodeList>.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetNodeListApiName);
                if (string.IsNullOrWhiteSpace(nodeNameFilter))
                {
                    Verify.IsTrue(string.IsNullOrWhiteSpace(this.NodeNameArray[this.apiCalls]));
                }
                else
                {
                    Verify.AreEqual(this.NodeNameArray[this.apiCalls], nodeNameFilter);
                }

                Verify.AreEqual(this.ContinuationToken, continuationToken);

                this.apiCalls++;

                if (nodeNameFilter != null)
                {
                    var result = new NodeList();
                    var item = this.Nodes.SingleOrDefault(p => p.NodeName == nodeNameFilter);
                    if (item != null)
                    {
                        result.Add(item);
                    }

                    return result;
                }

                return this.Nodes;
            });
        }

        public Task<NodeList> GetNodeListAsync(NodeQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return GetNodeListAsync(queryDescription.NodeNameFilter, queryDescription.NodeStatusFilter, queryDescription.ContinuationToken, timeout, cancellationToken);
        }

        public Task<ServicePartitionList> GetPartitionListAsync(
            Uri serviceName, Guid? partitionIdFilter, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task<ServicePartitionList>.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetPartitionListApiName);
                Verify.AreEqual(this.ServiceNameArray[this.apiCalls], serviceName);
                Verify.AreEqual(this.ContinuationToken, continuationToken);

                this.apiCalls++;

                if (partitionIdFilter.HasValue)
                {
                    Console.WriteLine("partitionIdFilter.Value: " + partitionIdFilter.Value);
                    var result = new ServicePartitionList();
                    var item = this.Partitions[serviceName].SingleOrDefault(p => p.PartitionInformation.Id == partitionIdFilter.Value);
                    if (item != null)
                    {
                        result.Add(item);
                    }

                    return result;
                }

                return this.Partitions[serviceName];
            });
        }

        public Task<ServicePartitionList> GetPartitionAsync(
            Guid partitionIdFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task<ServicePartitionList>.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetPartitionListApiName);
                Verify.IsTrue(partitionIdFilter != Guid.Empty);
                this.apiCalls++;

                Console.WriteLine("partitionIdFilter: " + partitionIdFilter);
                var result = new ServicePartitionList();
                foreach (var spl in this.Partitions.Values)
                {
                    var item = spl.Single(p => p.PartitionInformation.Id == partitionIdFilter);
                    if (item != null)
                    {
                        result.Add(item);
                        return result;
                    }
                }

                return result;
            });
        }

        public Task<PartitionLoadInformation> GetPartitionLoadInformationAsync(
            Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetPartitionLoadInformationApiName);
                Verify.AreEqual(this.PartitionIdArray[this.apiCalls], partitionId);
                this.apiCalls++;
                return this.PartitionLoadInformation;
            });
        }

        public Task<ReplicaLoadInformation> GetReplicaLoadInformationAsync(
            Guid partitionId, long replicaOrInstanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetReplicaLoadInformationApiName);
                Verify.AreEqual(this.PartitionIdArray[this.apiCalls], partitionId);
                Verify.AreEqual(this.ReplicaIdArray[this.apiCalls], replicaOrInstanceId);
                this.apiCalls++;
                return this.ReplicaLoadInformation;
            });
        }

        public Task<UnplacedReplicaInformation> GetUnplacedReplicaInformationAsync(
            string serviceName, Guid partitionId, bool onlyQueryPrimaries, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetUnplacedReplicaInformationApiName);
                Verify.AreEqual(this.ServiceNameArray[this.apiCalls], new Uri(serviceName));
                Verify.AreEqual(this.PartitionIdArray[this.apiCalls], partitionId);
                this.apiCalls++;
                return this.UnplacedReplicaInformation;
            });
        }

        public Task<ServiceReplicaList> GetReplicaListAsync(
            Guid partitionId, long replicaIdOrInstanceIdFilter, ServiceReplicaStatusFilter replicaStatusFilter, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task<ServiceReplicaList>.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetReplicaListApiName);
                Verify.AreEqual(this.PartitionIdArray[this.apiCalls], partitionId);
                Verify.AreEqual(this.ReplicaIdArray[this.apiCalls], replicaIdOrInstanceIdFilter);
                Verify.AreEqual(this.ReplicaStatusFilterArray[this.apiCalls], replicaStatusFilter);
                Verify.AreEqual(this.ContinuationToken, continuationToken);
                this.apiCalls++;

                if (replicaIdOrInstanceIdFilter != 0)
                {
                    var result = new ServiceReplicaList();
                    var item = this.Replicas[partitionId].SingleOrDefault(p => p.Id == replicaIdOrInstanceIdFilter);
                    if (item != null)
                    {
                        result.Add(item);
                    }

                    return result;
                }

                return this.Replicas[partitionId];
            });
        }

        public Task<ServiceList> GetServicePagedListAsync(
            ServiceQueryDescription serviceQueryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task<ServiceList>.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetServiceListApiName);
                Verify.AreEqual(this.ApplicationNameArray[this.apiCalls], serviceQueryDescription.ApplicationName);
                if (serviceQueryDescription.ServiceNameFilter != null)
                {
                    Verify.IsNull(this.ServiceNameArray[this.apiCalls]);
                }
                else
                {
                    Verify.AreEqual(this.ServiceNameArray[this.apiCalls], serviceQueryDescription.ServiceNameFilter);
                }

                Verify.AreEqual(this.ContinuationToken, serviceQueryDescription.ContinuationToken);

                this.apiCalls++;

                var serviceList = (serviceQueryDescription.ApplicationName == null) ? this.AdhocServices : this.Services[serviceQueryDescription.ApplicationName];
                if (serviceQueryDescription.ServiceNameFilter != null)
                {
                    var result = new ServiceList();
                    var item = serviceList.SingleOrDefault(p => p.ServiceName == serviceQueryDescription.ServiceNameFilter);
                    if (item != null)
                    {
                        result.Add(item);
                    }

                    return result;
                }

                return serviceList;
            });
        }

        public Task<ServiceGroupMemberList> GetServiceGroupMemberListAsync(
            Uri applicationName, Uri serviceNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return null;
        }

        public Task<ServiceTypeList> GetServiceTypeListAsync(
            string applicationTypeName, string applicationTypeVersion, string serviceTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task<ServiceTypeList>.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetServiceTypeListApiName);
                Verify.AreEqual(this.ApplicationTypeNameArray[this.apiCalls], applicationTypeName);
                Verify.AreEqual(this.ApplicationTypeVersionArray[this.apiCalls], applicationTypeVersion);
                Verify.AreEqual(this.ServiceTypeNameArray[this.apiCalls], serviceTypeNameFilter);
                this.apiCalls++;

                var appType = new ApplicationType(applicationTypeName, applicationTypeVersion);

                if (serviceTypeNameFilter != null)
                {
                    var result = new ServiceTypeList();
                    var item = this.ServiceTypes[appType].SingleOrDefault(p => p.ServiceTypeDescription.ServiceTypeName == serviceTypeNameFilter);
                    if (item != null)
                    {
                        result.Add(item);
                    }

                    return result;
                }

                return this.ServiceTypes[appType];
            });
        }

        public Task CreateApplicationAsync(
            ApplicationDescription applicationDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.CreateApplicationApiName);
                TestUtility.AreEqual(this.ApplicationDescription, applicationDescription);
                this.apiCalls++;
            });
        }

        public Task UpdateApplicationAsync(ApplicationUpdateDescription updateDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.UpdateApplicationApiName);
            });
        }

        public Task DeleteApplicationAsync(DeleteApplicationDescription deleteDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.DeleteApplicationApiName);
                Verify.AreEqual(this.ApplicationNameArray[this.apiCalls], deleteDescription.ApplicationName);
                Verify.AreEqual(this.ForceRemove, deleteDescription.ForceDelete);
                this.apiCalls++;
            });
        }

        public Task<string> GetApplicationManifestAsync(
            string applicationTypeName, string applicationTypeVersion, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetApplicationManifestApiName);
                Verify.AreEqual(this.ApplicationTypeNameArray[this.apiCalls], applicationTypeName);
                Verify.AreEqual(this.ApplicationTypeVersionArray[this.apiCalls], applicationTypeVersion);
                this.apiCalls++;

                return this.ApplicationManifest;
            });
        }

        public Task<ApplicationUpgradeProgress> GetApplicationUpgradeProgressAsync(Uri applicationName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetApplicationUpgradeProgressApiName);
                Verify.AreEqual(this.ApplicationNameArray[this.apiCalls], applicationName);
                this.apiCalls++;
                return this.ApplicationUpgradeProgress;
            });
        }

        public Task<ComposeDeploymentUpgradeProgress> GetComposeDeploymentUpgradeProgressAsync(string deploymentName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetComposeDeploymentUpgradeProgressApiName);
                // TODO: verify deploymentName

                this.apiCalls++;
                return this.ComposeDeploymentUpgradeProgress;
            });
        }

        public Task MoveNextApplicationUpgradeDomainAsync(
            Uri applicationName, string nextUpgradeDomain, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.MoveNextApplicationUpgradeDomainApiName);

                Verify.AreEqual(this.ApplicationUpgradeProgress.ApplicationName, applicationName);
                if (this.ExpectNotEqual)
                {
                    Verify.AreNotEqual(this.ApplicationUpgradeProgress.NextUpgradeDomain, nextUpgradeDomain);
                }
                else
                {
                    Verify.AreEqual(this.ApplicationUpgradeProgress.NextUpgradeDomain, nextUpgradeDomain);
                }

                this.apiCalls++;
            });
        }

        public Task ProvisionApplicationAsync(ProvisionApplicationTypeDescriptionBase description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.ProvisionApplicationApiName);

                if (description is ProvisionApplicationTypeDescription)
                {
                    var imageStoreDescription = (ProvisionApplicationTypeDescription)description;
                    Verify.AreEqual(this.ApplicationBuildPath, imageStoreDescription.BuildPath);
                }

                this.apiCalls++;
            });
        }

        public Task UnprovisionApplicationAsync(
            UnprovisionApplicationTypeDescription description,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.UnprovisionApplicationApiName);
                Verify.AreEqual(this.ApplicationTypeNameArray[this.apiCalls], description.ApplicationTypeName);
                Verify.AreEqual(this.ApplicationTypeVersionArray[this.apiCalls], description.ApplicationTypeVersion);
                this.apiCalls++;
            });
        }

        public Task UpgradeApplicationAsync(
            ApplicationUpgradeDescription upgradeDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.UpgradeApplicationApiName);
                TestUtility.AreEqual(this.ApplicationUpgradeDescription, upgradeDescription);
                this.apiCalls++;
            });
        }

        public Task UpdateApplicationUpgradeAsync(
            ApplicationUpgradeUpdateDescription updateDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.UpdateApplicationUpgradeApiName);
                TestUtility.AreEqual(this.ApplicationUpgradeUpdateDescription, updateDescription);
                this.apiCalls++;
            });
        }

        public Task RollbackApplicationUpgradeAsync(
            Uri applicationName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.RollbackApplicationUpgradeApiName);
                this.apiCalls++;
            });
        }

        public Task CreateComposeDeploymentAsync(
            ComposeDeploymentDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.CreateComposeDeploymentName);
                TestUtility.AreEqual(this.ComposeDeploymentDescription, description);
                this.apiCalls++;
            });
        }

        public Task DeleteComposeDeploymentAsync(
             DeleteComposeDeploymentDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.DeleteComposeDeploymentApiName);
                Verify.AreEqual(this.ApplicationNameArray[this.apiCalls], description.DeploymentName);
                this.apiCalls++;
            });
        }

        public Task UpgradeComposeDeploymentAsync(
             ComposeDeploymentUpgradeDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.UpgradeComposeDeploymentApiName);
                this.apiCalls++;
            });
        }

        public Task RollbackComposeDeploymentUpgradeAsync(
            ComposeDeploymentRollbackDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.RollbackComposeDeploymentApiName);
                Verify.AreEqual(this.ApplicationNameArray[this.apiCalls], description.DeploymentName);
                this.apiCalls++;
            });
        }

        public Task ActivateNodeAsync(string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.ActivateNodeApiName);
                Verify.AreEqual(this.NodeNameArray[this.apiCalls], nodeName);
                this.apiCalls++;
            });
        }

        public Task DeactivateNodeAsync(
            string nodeName, NodeDeactivationIntent deactivationIntent, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.DeactivateNodeApiName);
                Verify.AreEqual(this.NodeNameArray[this.apiCalls], nodeName);
                Verify.AreEqual(this.DeactivationIntent, deactivationIntent);
                this.apiCalls++;
            });
        }

        // public Task<StartNodeResult> StartNodeAsync(string nodeName, BigInteger instanceId, string ipAddressOrFQDN, int clientConnectionPort, int? requestTimeoutSecs, CompletionMode completionMode, CancellationToken cancellationToken)
        // {
        //    return Task.Factory.StartNew(() =>
        //    {
        //        if (this.Exception != null)
        //        {
        //            throw this.Exception;
        //        }

        // Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.StartNodeApiName);
        //        Verify.AreEqual(this.NodeNameArray[this.apiCalls], nodeName);
        //        this.apiCalls++;
        //        return this.StartNodeResult;
        //    });
        // }

        public Task RecoverPartitionsAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.RecoverPartitionsApiName);
                this.apiCalls++;
            });
        }

        public Task RecoverPartitionAsync(Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.RecoverPartitionApiName);
                Verify.AreEqual(this.PartitionIdArray[this.apiCalls], partitionId);
                this.apiCalls++;
            });
        }

        public Task RecoverServicePartitionsAsync(Uri serviceName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.RecoverServicePartitionsApiName);
                Verify.AreEqual(this.ServiceNameArray[this.apiCalls], serviceName);
                this.apiCalls++;
            });
        }

        public Task RecoverSystemPartitionsAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.RecoverSystemPartitionsApiName);
                this.apiCalls++;
            });
        }

        public Task ResetPartitionLoadAsync(Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.ResetPartitionLoadApiName);
                Verify.AreEqual(this.PartitionIdArray[this.apiCalls], partitionId);
                this.apiCalls++;
            });
        }

        public Task ToggleVerboseServicePlacementHealthReportingAsync(bool enabled, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.ToggleVerboseServicePlacementHealthReportingApiName);
                this.apiCalls++;
            });
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
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.RemoveReplicaApiName);

                // Verify.AreEqual(this.NodeNameArray[this.apiCalls], nodeName);
                // Verify.AreEqual(this.PartitionIdArray[this.apiCalls], partitionId);
                // Verify.AreEqual(this.ReplicaIdArray[this.apiCalls], replicaId);
                // Verify.AreEqual(this.ForceRemove, forceRemove);
                this.apiCalls++;
                return this.RemoveReplicaResult;
            }, cancellationToken);
        }

        public Task<RemoveReplicaResult> RemoveReplicaAsync(
            ReplicaSelector replicaSelector,
            int? timeoutSec,
            CompletionMode completionMode,
            bool forceRemove,
            CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.RemoveReplicaApiName);
                Verify.AreEqual(this.ReplicaSelectorArray[this.apiCalls], replicaSelector);
                this.apiCalls++;
                return this.RemoveReplicaResult;
            }, cancellationToken);
        }

        public Task<RestartReplicaResult> RestartReplicaAsync(
            string nodeName,
            Guid partitionId,
            long replicaId,
            CompletionMode completionMode,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.RestartReplicaApiName);
                this.apiCalls++;
                return this.RestartReplicaResult;
            }, cancellationToken);
        }

        public Task<RestartReplicaResult> RestartReplicaAsync(
            ReplicaSelector replicaSelector,
            int? timeoutSec,
            CompletionMode completionMode,
            CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.RestartReplicaApiName);
                Verify.AreEqual(this.ReplicaSelectorArray[this.apiCalls], replicaSelector);
                this.apiCalls++;
                return this.RestartReplicaResult;
            }, cancellationToken);
        }

        public Task<string> GetClusterManifestAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetClusterManifestAsync(new ClusterManifestQueryDescription(), timeout, cancellationToken);
        }

        public Task<string> GetClusterManifestAsync(ClusterManifestQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetClusterManifestApiName);
                this.apiCalls++;

                return this.ClusterManifest;
            });
        }

        public Task<FabricUpgradeProgress> GetFabricUpgradeProgressAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetFabricUpgradeProgressApiName);
                this.apiCalls++;
                return this.FabricUpgradeProgress;
            });
        }

        public Task MoveNextFabricUpgradeDomainAsync(
            string nextUpgradeDomain, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.MoveNextFabricUpgradeDomainApiName);
                if (this.ExpectNotEqual)
                {
                    Verify.AreNotEqual(this.FabricUpgradeProgress.NextUpgradeDomain, nextUpgradeDomain);
                }
                else
                {
                    Verify.AreEqual(this.FabricUpgradeProgress.NextUpgradeDomain, nextUpgradeDomain);
                }

                this.apiCalls++;
            });
        }

        public Task ProvisionFabricAsync(
            string mspFilePath, string clusterManifestFilePath, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.ProvisionFabricApiName);
                Verify.AreEqual(this.MspFilePath, mspFilePath);
                Verify.AreEqual(this.ClusterManifestFilePath, clusterManifestFilePath);
                this.apiCalls++;
            });
        }

        public Task RemoveNodeStateAsync(string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.RemoveNodeStateApiName);
                Verify.AreEqual(this.NodeNameArray[this.apiCalls], nodeName);
                this.apiCalls++;
            });
        }

        public Task UnprovisionFabricAsync(
            string codeVersion, string configVersion, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.UnprovisionFabricApiName);
                Verify.AreEqual(this.CodeVersion, codeVersion);
                Verify.AreEqual(this.ConfigVersion, configVersion);
                this.apiCalls++;
            });
        }

        public Task UpgradeFabricAsync(
            FabricUpgradeDescription upgradeDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.UpgradeFabricApiName);
                TestUtility.AreEqual(this.FabricUpgradeDescription, upgradeDescription);
                this.apiCalls++;
            });
        }

        public Task UpdateFabricUpgradeAsync(
            FabricUpgradeUpdateDescription updateDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.UpdateFabricUpgradeApiName);
                TestUtility.AreEqual(this.FabricUpgradeUpdateDescription, updateDescription);
                this.apiCalls++;
            });
        }

        public Task RollbackFabricUpgradeAsync(
            TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.RollbackFabricUpgradeApiName);
                this.apiCalls++;
            });
        }

        public Task CreateServiceAsync(ServiceDescription serviceDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.CreateServiceApiName);
                TestUtility.AreEqual(this.ServiceDescription, serviceDescription);
                this.apiCalls++;
            });
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
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.CreateServiceFromTemplateApiName);
                Verify.AreEqual(this.ApplicationNameArray[this.apiCalls], applicationName);
                Verify.AreEqual(this.ServiceNameArray[this.apiCalls], serviceName);
                Verify.AreEqual(this.ServiceTypeNameArray[this.apiCalls], serviceTypeName);
                Verify.AreEqual(null, initializationData);
                Verify.AreEqual(this.ServicePackageActivationModeArray[this.apiCalls], servicePackageActivationMode);
                this.apiCalls++;
            });
        }

        public Task DeleteServiceAsync(DeleteServiceDescription deleteDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.DeleteServiceApiName);
                Verify.AreEqual(this.ServiceNameArray[this.apiCalls], deleteDescription.ServiceName);
                Verify.AreEqual(this.ForceRemove, deleteDescription.ForceDelete);
                this.apiCalls++;
            });
        }

        public Task UpdateServiceAsync(
            Uri name, ServiceUpdateDescription serviceUpdateDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.UpdateServiceApiName);
                Verify.AreEqual(this.ServiceNameArray[this.apiCalls], name);
                TestUtility.AreEqual(this.ServiceUpdateDescription, serviceUpdateDescription);
                this.apiCalls++;
            });
        }

        public Task<string> GetServiceManifestAsync(
            string applicationTypeName,
            string applicationTypeVersion,
            string serviceManifestName,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetServiceManifestApiName);
                Verify.AreEqual(this.ApplicationTypeNameArray[this.apiCalls], applicationTypeName);
                Verify.AreEqual(this.ApplicationTypeVersionArray[this.apiCalls], applicationTypeVersion);
                Verify.AreEqual(this.ServiceManifestNameArray[this.apiCalls], serviceManifestName);
                this.apiCalls++;

                return this.ServiceManifest;
            });
        }

        public Task<ServiceDescription> GetServiceDescriptionAsync(
            Uri serviceName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetServiceDescriptionApiName);
                Verify.AreEqual(this.ServiceNameArray[this.apiCalls], serviceName);
                this.apiCalls++;

                return this.ServiceDescription;
            });
        }

        public Task<ResolvedServicePartition> ResolveServicePartitionAsync(
            Uri serviceName, ResolvedServicePartition previousResult, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.ResolveServicePartitionApiName);
                Verify.AreEqual(this.ServiceNameArray[this.apiCalls], serviceName);
                Verify.AreEqual(this.PreviousResultArray[this.apiCalls], previousResult);
                this.apiCalls++;

                return this.CurrentResult;
            });
        }

        public Task<ResolvedServicePartition> ResolveServicePartitionAsync(
            Uri serviceName, long partitionKey, ResolvedServicePartition previousResult, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.ResolveServicePartitionApiName);
                Verify.AreEqual(this.ServiceNameArray[this.apiCalls], serviceName);
                Verify.AreEqual(this.PartitionKeyLongArray[this.apiCalls], partitionKey);
                Verify.AreEqual(this.PreviousResultArray[this.apiCalls], previousResult);
                this.apiCalls++;

                return this.CurrentResult;
            });
        }

        public Task<ResolvedServicePartition> ResolveServicePartitionAsync(
            Uri serviceName, string partitionKey, ResolvedServicePartition previousResult, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.ResolveServicePartitionApiName);
                Verify.AreEqual(this.ServiceNameArray[this.apiCalls], serviceName);
                Verify.AreEqual(this.PartitionKeyStringArray[this.apiCalls], partitionKey);
                Verify.AreEqual(this.PreviousResultArray[this.apiCalls], previousResult);
                this.apiCalls++;

                return this.CurrentResult;
            });
        }

        public Task CreateServiceGroupAsync(
            ServiceGroupDescription serviceGroupDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return null;
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
            return null;
        }

        public Task UpdateServiceGroupAsync(
            Uri name, ServiceGroupUpdateDescription updateDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.UpdateServiceGroupApiName);
                Verify.AreEqual(this.ServiceNameArray[this.apiCalls], name);
                TestUtility.AreEqual(this.ServiceGroupUpdateDescription, updateDescription);
                this.apiCalls++;
            });
        }

        public Task DeleteServiceGroupAsync(
            Uri name, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return null;
        }

        public Task<ServiceGroupDescription> GetServiceGroupDescriptionAsync(
            Uri name, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return null;
        }

        public Task<ApplicationHealth> GetApplicationHealthAsync(
            ApplicationHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetApplicationHealthApiName);
                Verify.AreEqual(this.ApplicationNameArray[this.apiCalls], queryDescription.ApplicationName);
                if (this.HealthPolicy == null)
                {
                    Verify.IsNull(queryDescription.HealthPolicy);
                }
                else
                {
                    TestUtility.AreEqual(this.HealthPolicy as ApplicationHealthPolicy, queryDescription.HealthPolicy);
                }

                TestUtility.AreEqual(this.EventsHealthFilter, queryDescription.EventsFilter);
                TestUtility.AreEqual(this.DeployedApplicationsHealthFilter, queryDescription.DeployedApplicationsFilter);
                TestUtility.AreEqual(this.ServicesHealthFilter, queryDescription.ServicesFilter);

                this.apiCalls++;
                return this.ApplicationHealth;
            });
        }

        public Task<NodeHealth> GetNodeHealthAsync(
            NodeHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetNodeHealthApiName);
                Verify.AreEqual(this.NodeNameArray[this.apiCalls], queryDescription.NodeName);
                TestUtility.AreEqual(this.HealthPolicy as ClusterHealthPolicy, queryDescription.HealthPolicy);
                TestUtility.AreEqual(this.EventsHealthFilter, queryDescription.EventsFilter);
                this.apiCalls++;
                return this.NodeHealth;
            });
        }

        public Task<PartitionHealth> GetPartitionHealthAsync(
            PartitionHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetPartitionHealthApiName);
                Verify.AreEqual(this.PartitionIdArray[this.apiCalls], queryDescription.PartitionId);
                TestUtility.AreEqual(this.HealthPolicy as ApplicationHealthPolicy, queryDescription.HealthPolicy);
                TestUtility.AreEqual(this.EventsHealthFilter, queryDescription.EventsFilter);
                TestUtility.AreEqual(this.ReplicasHealthFilter, queryDescription.ReplicasFilter);
                this.apiCalls++;
                return this.PartitionHealth;
            });
        }

        public Task<ReplicaHealth> GetReplicaHealthAsync(
            ReplicaHealthQueryDescription queryDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetReplicaHealthApiName);
                Verify.AreEqual(this.PartitionIdArray[this.apiCalls], queryDescription.PartitionId);
                Verify.AreEqual(this.ReplicaIdArray[this.apiCalls], queryDescription.ReplicaOrInstanceId);
                TestUtility.AreEqual(this.HealthPolicy as ApplicationHealthPolicy, queryDescription.HealthPolicy);
                TestUtility.AreEqual(this.EventsHealthFilter, queryDescription.EventsFilter);
                this.apiCalls++;
                return this.ReplicaHealth;
            });
        }

        public Task<ServiceHealth> GetServiceHealthAsync(
            ServiceHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetServiceHealthApiName);
                Verify.AreEqual(this.ServiceNameArray[this.apiCalls], queryDescription.ServiceName);
                TestUtility.AreEqual(this.HealthPolicy as ApplicationHealthPolicy, queryDescription.HealthPolicy);
                TestUtility.AreEqual(this.EventsHealthFilter, queryDescription.EventsFilter);
                TestUtility.AreEqual(this.PartitionsHealthFilter, queryDescription.PartitionsFilter);
                this.apiCalls++;
                return this.ServiceHealth;
            });
        }

        public Task<DeployedApplicationHealth> GetDeployedApplicationHealthAsync(
            DeployedApplicationHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetDeployedApplicationHealthApiName);
                Verify.AreEqual(this.ApplicationNameArray[this.apiCalls], queryDescription.ApplicationName);
                Verify.AreEqual(this.NodeNameArray[this.apiCalls], queryDescription.NodeName);
                TestUtility.AreEqual(this.HealthPolicy as ApplicationHealthPolicy, queryDescription.HealthPolicy);
                TestUtility.AreEqual(this.EventsHealthFilter, queryDescription.EventsFilter);
                TestUtility.AreEqual(this.DeployedServicePackagesHealthFilter, queryDescription.DeployedServicePackagesFilter);
                this.apiCalls++;
                return this.DeployedApplicationHealth;
            });
        }

        public Task<DeployedServicePackageHealth> GetDeployedServicePackageHealthAsync(
            DeployedServicePackageHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetDeployedServicePackageHealthApiName);
                Verify.AreEqual(this.ApplicationNameArray[this.apiCalls], queryDescription.ApplicationName);
                Verify.AreEqual(this.ServiceManifestNameArray[this.apiCalls], queryDescription.ServiceManifestName);
                Verify.AreEqual(this.NodeNameArray[this.apiCalls], queryDescription.NodeName);
                Verify.AreEqual(this.ServicePackageActivationIdArray[this.apiCalls], queryDescription.ServicePackageActivationId);

                TestUtility.AreEqual(this.HealthPolicy as ApplicationHealthPolicy, queryDescription.HealthPolicy);
                TestUtility.AreEqual(this.EventsHealthFilter, queryDescription.EventsFilter);
                this.apiCalls++;
                return this.DeployedServicePackageHealth;
            });
        }

        public Task<ClusterHealth> GetClusterHealthAsync(
            ClusterHealthQueryDescription queryDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetClusterHealthApiName);
                TestUtility.AreEqual(this.HealthPolicy as ClusterHealthPolicy, queryDescription.HealthPolicy);
                TestUtility.AreEqual(this.EventsHealthFilter, queryDescription.EventsFilter);
                TestUtility.AreEqual(this.NodesHealthFilter, queryDescription.NodesFilter);
                TestUtility.AreEqual(this.ApplicationsHealthFilter, queryDescription.ApplicationsFilter);
                this.apiCalls++;
                return this.ClusterHealth;
            });
        }

        public Task<ClusterHealthChunk> GetClusterHealthChunkAsync(
            ClusterHealthChunkQueryDescription queryDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetClusterHealthChunkApiName);
                TestUtility.AreEqual(this.HealthPolicy as ClusterHealthPolicy, queryDescription.ClusterHealthPolicy);
                TestUtility.AreEqual(this.NodeHealthStateFilters, queryDescription.NodeFilters);
                TestUtility.AreEqual(this.ApplicationHealthStateFilters, queryDescription.ApplicationFilters);
                this.apiCalls++;
                return this.ClusterHealthChunk;
            });
        }

        /// This will mock ReportHealthAsync functionality of IClusterConnection API.
        public void ReportHealth(HealthReport healthReport, HealthReportSendOptions sendOptions)
        {
            if (this.Exception != null)
            {
                throw this.Exception;
            }

            Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.SendHealthReportApiName);
            TestUtility.AreEqual(this.healthReport, healthReport);
            this.apiCalls++;
        }

        public Task<ClusterLoadInformation> GetClusterLoadInformationAsync(
            TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetClusterLoadInformationApiName);
                this.apiCalls++;
                return this.ClusterLoadInformation;
            });
        }

        public Task<NodeLoadInformation> GetNodeLoadInformationAsync(
            string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetNodeLoadInformationApiName);
                this.apiCalls++;
                return this.NodeLoadInformation;
            });
        }

        public Task<ApplicationLoadInformation> GetApplicationLoadInformationAsync(
            string applicationName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetApplicationLoadInformationApiName);
                this.apiCalls++;
                return this.ApplicationLoadInformation;
            });
        }

        public Task<ServiceNameResult> GetServiceNameAsync(
            Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Run(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetServiceNameApiName);
                this.apiCalls++;

                var result = new ServiceNameResult();

                foreach (var entry in this.Partitions)
                {
                    foreach (var _ in entry.Value.Where(partition => partition.PartitionInformation.Id == partitionId))
                    {
                        result.ServiceName = entry.Key;
                    }
                }

                return result;
            });
        }

        public Task<ApplicationNameResult> GetApplicationNameAsync(
            Uri serviceName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Run(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetApplicationNameApiName);
                this.apiCalls++;

                var result = new ApplicationNameResult();

                foreach (var entry in this.Services)
                {
                    foreach (var _ in entry.Value.Where(service => service.ServiceName == serviceName))
                    {
                        result.ApplicationName = entry.Key;
                    }
                }

                return result;
            });
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
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.DeployServicePackageToNodeApiName);
                this.apiCalls++;
            });
        }

        public Task<ProvisionedFabricCodeVersionList> GetProvisionedFabricCodeVersionListAsync(string codeVersionFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetFabricCodeVersionListApiName);
                this.apiCalls++;

                return this.ClusterCodeVersions;
            });
        }

        public Task<ProvisionedFabricConfigVersionList> GetProvisionedFabricConfigVersionListAsync(string configVersionFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetFabricConfigVersionListApiName);
                this.apiCalls++;

                return this.ClusterConfigVersions;
            });
        }

        public Task<string> InvokeInfrastructureCommandAsync(Uri serviceName, string command, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.InvokeInfrastructureAsyncCommon(
                       TestConstants.InvokeInfrastructureCommandAsyncApiName,
                       serviceName,
                       command,
                       timeout,
                       cancellationToken);
        }

        public Task<string> InvokeInfrastructureQueryAsync(Uri serviceName, string command, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.InvokeInfrastructureAsyncCommon(
                       TestConstants.InvokeInfrastructureQueryAsyncApiName,
                       serviceName,
                       command,
                       timeout,
                       cancellationToken);
        }

        private Task<string> InvokeInfrastructureAsyncCommon(string apiName, Uri serviceName, string command, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], apiName);
                this.apiCalls++;
                return string.Format("{{\"ApiName\":\"{0}\", \"ServiceName\":\"{1}\", \"Command\":\"{2}\"}}", apiName, serviceName, command);
            });
        }

        public Task<long> CreateRepairTaskAsync(RepairTask repairTask, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.CreateRepairTaskApiName);

                if (this.RepairTaskValidator != null)
                {
                    this.RepairTaskValidator(repairTask);
                }

                this.apiCalls++;

                return 42L; // Value doesn't matter
            });
        }

        public Task<long> CancelRepairTaskAsync(string repairTaskId, long version, bool requestAbort, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.CancelRepairTaskApiName);
                Verify.IsFalse(requestAbort, "requestAbort"); // switch not exposed by the cmdlet currently
                // TODO validate task params
                this.apiCalls++;

                return 42L; // Value doesn't matter
            });
        }

        public Task<long> ForceApproveRepairTaskAsync(string repairTaskId, long version, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.ForceApproveRepairTaskApiName);

                // TODO validate task params
                this.apiCalls++;

                return 42L; // Value doesn't matter
            });
        }

        public Task DeleteRepairTaskAsync(string repairTaskId, long version, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.DeleteRepairTaskApiName);

                // TODO validate task params
                this.apiCalls++;
            });
        }

        public Task<long> UpdateRepairExecutionStateAsync(RepairTask repairTask, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.UpdateRepairExecutionStateApiName);

                if (this.RepairTaskValidator != null)
                {
                    this.RepairTaskValidator(repairTask);
                }

                this.apiCalls++;

                return 42L; // Value doesn't matter
            });
        }

        public Task<RepairTaskList> GetRepairTaskListAsync(string taskIdFilter, RepairTaskStateFilter stateFilter, string executorFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.GetRepairTaskListApiName);

                // TODO validate params
                this.apiCalls++;

                return this.RepairTasks;
            });
        }

        public Task<long> UpdateRepairTaskHealthPolicyAsync(
            string repairTaskId,
            long version,
            bool? performPreparingHealthCheck,
            bool? performRestoringHealthCheck,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.UpdateRepairTaskHealthPolicyApiName);

                // TODO validate task params
                this.apiCalls++;

                return 42L; // Value doesn't matter
            });
        }

        public Task<ServiceGroupMemberTypeList> GetServiceGroupMemberTypeListAsync(string applicationTypeName, string applicationTypeVersion, string serviceGroupTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task CleanTestStateAsync(int? requestTimeoutSec, CancellationToken token)
        {
            throw new NotImplementedException();
        }

        public Task<RestartDeployedCodePackageResult> RestartDeployedCodePackageAsync(
            string nodeName,
            Uri applicationName,
            string serviceManifestName,
            string servicePackageActivationId,
            string codePackageName,
            long codePackageInstanceId,
            int? requestTimeoutSecs,
            CompletionMode completionMode,
            CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(
                () =>
                {
                    if (this.Exception != null)
                    {
                        throw this.Exception;
                    }

                    Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.RestartDeployedCodePackageApiName);
                    Verify.AreEqual(this.ApplicationNameArray[this.apiCalls], applicationName);
                    Verify.AreEqual(this.ServiceManifestNameArray[this.apiCalls], serviceManifestName);
                    Verify.AreEqual(this.ServicePackageActivationIdArray[this.apiCalls], servicePackageActivationId);
                    Verify.AreEqual(this.CodePackageNameArray[this.apiCalls], codePackageName);
                    this.apiCalls++;
                    return this.RestartDeployedCodePackageResult;
                },
                cancellationToken);

        }

        public Task<RestartDeployedCodePackageResult> RestartDeployedCodePackageAsync(
            ReplicaSelector replicaSelector,
            Uri applicationName,
            int? requestTimeout,
            CompletionMode completionMode,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        [Obsolete("Deprecated")]
        public Task InvokeChaosTestScenarioAsync(ChaosTestScenarioParameters chaosScenarioParameters, CancellationToken token)
        {
            throw new NotImplementedException();
        }

        public Task InvokeFailoverTestScenarioAsync(
            Uri serviceName,
            PartitionSelector partitionSelector,
            uint timeToRunMinute,
            uint maxServiceStabilizationTimeoutInSeconds,
            int? waitTimeBetweenFaultsInSeconds,
            int? requestTimeoutSec,
            CancellationToken token)
        {
            throw new NotImplementedException();
        }

        public Task<InvokeDataLossResult> InvokeDataLossAsync(
            PartitionSelector partitionSelector,
            DataLossMode datalossMode,
            int? requestTimeoutSec,
            CancellationToken token)
        {
            return Task.Factory.StartNew(
                       () =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.InvokeDataLossApiName);
                Verify.AreEqual(this.PartitionSelectorArray[this.apiCalls], partitionSelector);
                this.apiCalls++;
                return this.InvokeDataLossResult ?? this.InvokeDataLossResultArray[this.apiCalls - 1];
            },
            token);
        }

        public Task InvokeDataLossAsync(
            Guid operationId,
            PartitionSelector partitionSelector,
            DataLossMode dataLossMode,
            int? requestTimeoutSec,
            CancellationToken token)
        {
            throw new NotImplementedException();
        }

        public Task<PartitionDataLossProgress> GetInvokeDataLossProgressAsync(
            Guid operationId,
            TimeSpan requestTimeoutSec,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task InvokeQuorumLossAsync(
            Guid operationId,
            PartitionSelector partitionSelector,
            QuorumLossMode quorumlossMode,
            int quorumlossDuration,
            int? operationTimeout,
            CancellationToken token)
        {
            throw new NotImplementedException();
        }

        public Task<PartitionQuorumLossProgress> GetInvokeQuorumLossProgressAsync(
            Guid operationId,
            TimeSpan requestTimeoutSec,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task RestartPartitionAsync(
            Guid operationId,
            PartitionSelector partitionSelector,
            RestartPartitionMode restartPartitionMode,
            int? operationTimeout,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<PartitionRestartProgress> GetRestartPartitionProgressAsync(
            Guid operationId,
            TimeSpan requestTimeoutSec,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<TestCommandStatusList> GetTestCommandStatusListAsync(
            TestCommandStateFilter stateFilter,
            TestCommandTypeFilter typeFilter,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task CancelTestCommandAsync(
            Guid operationId,
            bool force,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<InvokeQuorumLossResult> InvokeQuorumLossAsync(
            PartitionSelector partitionSelector,
            QuorumLossMode quorumlossMode,
            int quorumlossDuration,
            int? operationTimeout,
            CancellationToken token)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.InvokeQuorumLossApiName);
                Verify.AreEqual(this.PartitionSelectorArray[this.apiCalls], partitionSelector);
                this.apiCalls++;
                return this.InvokeQuorumLossResult ?? this.InvokeQuorumLossResultArray[this.apiCalls - 1];
            }, token);
        }

        public Task<MovePrimaryResult> MovePrimaryReplicaAsync(
            string newPrimaryNodeName,
            PartitionSelector partitionSelector,
            int? requestTimeoutSec,
            CancellationToken token)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.MovePrimaryApiName);
                Verify.AreEqual(this.PartitionSelectorArray[this.apiCalls], partitionSelector);
                this.apiCalls++;
                return this.MovePrimaryResult ?? this.MovePrimaryResultArray[apiCalls - 1];
            }, token);
        }

        public Task<MovePrimaryResult> MovePrimaryReplicaAsync(
            string newPrimaryNodeName,
            PartitionSelector partitionSelector,
            bool ignoreConstraints,
            int? requestTimeoutSec,
            CancellationToken token)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.MovePrimaryApiName);
                Verify.AreEqual(this.PartitionSelectorArray[this.apiCalls], partitionSelector);
                this.apiCalls++;
                return this.MovePrimaryResult ?? this.MovePrimaryResultArray[apiCalls - 1];
            }, token);
        }

        public Task<MoveSecondaryResult> MoveSecondaryReplicaAsync(
            string currentSecondaryNodeName,
            string newSecondaryNodeName,
            PartitionSelector partitionSelector,
            int? requestTimeoutSec,
            CancellationToken token)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.MoveSecondaryApiName);
                Verify.AreEqual(this.PartitionSelectorArray[this.apiCalls], partitionSelector);
                this.apiCalls++;
                return this.MoveSecondaryResult ?? this.MoveSecondaryResultArray[apiCalls - 1];
            }, token);
        }

        public Task<MoveSecondaryResult> MoveSecondaryReplicaAsync(
            string currentSecondaryNodeName,
            string newSecondaryNodeName,
            PartitionSelector partitionSelector,
            bool ignoreConstraints,
            int? requestTimeoutSec,
            CancellationToken token)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.MoveSecondaryApiName);
                Verify.AreEqual(this.PartitionSelectorArray[this.apiCalls], partitionSelector);
                this.apiCalls++;
                return this.MoveSecondaryResult ?? this.MoveSecondaryResultArray[apiCalls - 1];
            }, token);
        }

        public Task<RestartNodeResult> RestartNodeAsync(ReplicaSelector replicaSelector, bool createFabricDump, int? requestTimeoutSecs, CompletionMode completionMode, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.RestartNodeApiName);
                Verify.AreEqual(this.ReplicaSelectorArray[this.apiCalls], replicaSelector);
                this.apiCalls++;
                return this.RestartNodeResult;
            }, cancellationToken);
        }

        public Task<RestartNodeResult> RestartNodeAsync(string nodeName, BigInteger nodeInstanceId, bool createFabricDump, int? requestTimeoutSecs, CompletionMode completionMode, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.RestartNodeApiName);
                this.apiCalls++;
                return this.RestartNodeResult;
            }, cancellationToken);
        }

        public Task<RestartNodeResult> RestartNodeAsync(string nodeName, BigInteger nodeInstanceId, int? requestTimeoutSecs, CompletionMode completionMode, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.RestartNodeApiName);
                this.apiCalls++;
                return this.RestartNodeResult;
            }, cancellationToken);
        }

        public Task<RestartPartitionResult> RestartPartitionAsync(
            PartitionSelector partitionSelector,
            RestartPartitionMode restartPartitionMode,
            int? requestTimeoutSec,
            CancellationToken token)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.RestartPartitionApiName);
                Verify.AreEqual(this.PartitionSelectorArray[this.apiCalls], partitionSelector);
                this.apiCalls++;
                return this.RestartPartitionResult ?? this.RestartPartitionResultArray[apiCalls - 1];
            }, token);
        }

        public Task<StartNodeResult> StartNodeAsync(
            string nodeName,
            BigInteger instanceId,
            string ipAddressOrFQDN,
            int clientConnectionPort,
            int? requestTimeoutSecs,
            CompletionMode completionMode,
            CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.StartNodeApiName);
                Verify.AreEqual(this.NodeNameArray[this.apiCalls], nodeName);
                this.apiCalls++;
                return this.StartNodeResult;
            }, cancellationToken);
        }

        public Task<StopNodeResult> StopNodeAsync(
            string nodeName,
            BigInteger nodeInstanceId,
            int? requestTimeoutSecs,
            CompletionMode completionMode,
            CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.StopNodeApiName);
                Verify.AreEqual(this.NodeNameArray[this.apiCalls], nodeName);
                this.apiCalls++;
                return this.StopNodeResult;
            }, cancellationToken);
        }

        public Task TestApplicationAsync(Uri applicationName, int maxStabilizationTimeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.TestApplicationApiName);
                Verify.AreEqual(this.ApplicationNameArray[this.apiCalls], applicationName);

                this.apiCalls++;
            }, cancellationToken);
        }

        public Task TestServiceAsync(Uri serviceName, int maxStabilizationTimeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() =>
            {
                if (this.Exception != null)
                {
                    throw this.Exception;
                }

                Verify.AreEqual(this.ApiNameArray[this.apiCalls], TestConstants.TestServiceApiName);
                Verify.AreEqual(this.ServiceNameArray[this.apiCalls], serviceName);
                this.apiCalls++;
            }, cancellationToken);
        }

        public Task StartChaosAsync(
            ChaosParameters parameters,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task StopChaosAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<ChaosReport> GetChaosReportAsync(
            ChaosReportFilter filter,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<ChaosEventsSegment> GetChaosEventsAsync(
            ChaosEventsSegmentFilter filter,
            long maxResults,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<ChaosDescription> GetChaosAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<ChaosScheduleDescription> GetChaosScheduleAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task SetChaosScheduleAsync(
            ChaosScheduleDescription schedule,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<ChaosReport> GetChaosReportAsync(
            string continuationToken,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<ChaosEventsSegment> GetChaosEventsAsync(
            string continuationToken,
            long maxResults,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task StartNodeTransitionAsync(
            NodeTransitionDescription description,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<NodeTransitionProgress> GetNodeTransitionProgressAsync(
            Guid operationId,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<Secret[]> GetSecretsAsync(
            SecretReference[] secretReferences,
            bool excludeValue,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<SecretReference[]> SetSecretsAsync(
            Secret[] secrets,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<SecretReference[]> RemoveSecretsAsync(
            SecretReference[] secretReferences,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task StartClusterConfigurationUpgradeAsync(
            ConfigurationUpgradeDescription description,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<FabricOrchestrationUpgradeProgress> GetClusterConfigurationUpgradeStatusAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<string> GetClusterConfigurationAsync(
            string apiVersion,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<string> GetUpgradeOrchestrationServiceStateAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<FabricUpgradeOrchestrationServiceState> SetUpgradeOrchestrationServiceStateAsync(
            string state,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task GetUpgradesPendingApprovalAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task StartApprovedUpgradesAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public class ApplicationType
        {
            public ApplicationType(string name, string version)
            {
                this.Name = name;
                this.Version = version;
            }

            public string Name
            {
                get;
                set;
            }

            public string Version
            {
                get;
                set;
            }

            public bool Equals(ApplicationType other)
            {
                return string.Equals(this.Name, other.Name) && string.Equals(this.Version, other.Version);
            }

            public override bool Equals(object obj)
            {
                if (ReferenceEquals(null, obj))
                {
                    return false;
                }

                if (ReferenceEquals(this, obj))
                {
                    return true;
                }

                return obj.GetType() == this.GetType() && this.Equals((ApplicationType)obj);
            }

            public override int GetHashCode()
            {
                unchecked
                {
                    return ((this.Name != null ? this.Name.GetHashCode() : 0) * 397) ^ (this.Version != null ? this.Version.GetHashCode() : 0);
                }
            }
        }
    }
}