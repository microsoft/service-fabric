// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Cluster.ClusterQuery
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Threading;
    using System.Threading.Tasks;
    using ClusterAnalysis.AnalysisCore;
    using ClusterAnalysis.AnalysisCore.ClusterEntityDescription;

    /// <summary>
    /// Interface for querying Cluster
    /// </summary>
    public interface IClusterQuery
    {
        /// <summary>
        /// Get Node
        /// </summary>
        /// <param name="nodeName"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        Task<NodeEntity> GetNodeAsync(string nodeName, CancellationToken token);

        /// <summary>
        /// Get list of all Nodes in cluster
        /// </summary>
        /// <param name="token"></param>
        /// <returns></returns>
        Task<IEnumerable<NodeEntity>> GetNodeListAsync(CancellationToken token);

        /// <summary>
        /// Get Replicas belonging to a partition
        /// </summary>
        /// <param name="partitionIdGuid"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        Task<IEnumerable<ReplicaEntity>> GetReplicasInPartitionAsync(Guid partitionIdGuid, CancellationToken token);

        /// <summary>
        /// Get names of all Exes that belong to services in a single partition
        /// </summary>
        /// <param name="nodeName"></param>
        /// <param name="appName"></param>
        /// <param name="partitionGuid"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        Task<IEnumerable<string>> GetPartitionDeployedExecutableNamesAsync(string nodeName, Uri appName, Guid partitionGuid, CancellationToken token);

        /// <summary>
        /// Get all partition Ids from Cluster
        /// </summary>
        /// <param name="token"></param>
        /// <returns></returns>
        Task<IEnumerable<Guid>> GetAllPartitionIdsAsync(CancellationToken token);

        /// <summary>
        /// Get Partition
        /// </summary>
        /// <param name="partitionGuid"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        Task<PartitionEntity> GetPartitionAsync(Guid partitionGuid, CancellationToken token);

        /// <summary>
        /// Get Replica
        /// </summary>
        /// <param name="partitionIdGuid"></param>
        /// <param name="replicaId"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        Task<ReplicaEntity> GetReplicaAsync(Guid partitionIdGuid, long replicaId, CancellationToken token);

        /// <summary>
        /// Get Application matching the partition
        /// </summary>
        /// <param name="partitionGuid"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        Task<ApplicationEntity> GetPartitionApplicationAsync(Guid partitionGuid, CancellationToken token);

        /// <summary>
        /// Get Service
        /// </summary>
        /// <param name="serviceName"></param>
        /// <param name="token"></param>
        /// <param name="applicationName"></param>
        /// <returns></returns>
        Task<ServiceEntity> GetServiceAsync(Uri applicationName, Uri serviceName, CancellationToken token);

        /// <summary>
        /// Get Service Name mapping this partition
        /// </summary>
        /// <param name="partitionGuid"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        Task<ServiceEntity> GetPartitionServiceAsync(Guid partitionGuid, CancellationToken token);

        /// <summary>
        /// Get all applications
        /// </summary>
        /// <param name="token"></param>
        /// <returns></returns>
        Task<IEnumerable<ApplicationEntity>> GetApplicationsAsync(CancellationToken token);

        /// <summary>
        /// Get application
        /// </summary>
        /// <param name="applicationName"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        Task<ApplicationEntity> GetApplicationAsync(Uri applicationName, CancellationToken token);

        /// <summary>
        /// Get all applications deployed on a particular node.
        /// </summary>
        /// <param name="nodeName"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        Task<IEnumerable<DeployedApplicationEntity>> GetDeployedApplicationsAsync(string nodeName, CancellationToken token);

        /// <summary>
        /// Get all instances of a particular application's deployed on a given node.
        /// </summary>
        /// <param name="applicationName"></param>
        /// <param name="nodeName"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        Task<DeployedApplicationEntity> GetDeployedApplicationsAsync(Uri applicationName, string nodeName, CancellationToken token);

        /// <summary>
        /// Get all Code packages deployed on a particular node
        /// </summary>
        /// <param name="nodeName"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        Task<IEnumerable<DeployedCodePackageEntity>> GetDeployedCodePackageAsync(string nodeName, CancellationToken token);

        /// <summary>
        /// Get deployed code Package
        /// </summary>
        /// <param name="nodeName"></param>
        /// <param name="applicationName"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        Task<IEnumerable<DeployedCodePackageEntity>> GetDeployedCodePackageAsync(string nodeName, Uri applicationName, CancellationToken token);

        /// <summary>
        /// Get Error and warning health events along with the entity they are present on.
        /// </summary>
        /// <param name="token"></param>
        /// <returns></returns>
        Task<List<KeyValuePair<BaseEntity, IList<HealthEventEntity>>>> GetErrorAndWarningHealthEventsAsync(CancellationToken token);
    }
}