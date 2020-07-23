// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Common.Serialization
{
    using Newtonsoft.Json;
    using System.Fabric.Description;
    using System.Fabric.Health;

    /// Json properties of this class should match json properties of native class defined in file:
    /// %SDXROOT%\services\winfab\prod\test\System.Fabric\Common\NativeAndManagedSerializationInteropTest\HealthSerializationInteropTest.h
    internal class HealthSerializationInteropTest
    {
        [JsonIgnore]
        Random random = new Random((int)DateTime.Now.Ticks);

        HealthState FABRIC_HEALTH_STATE_ { get; set; }

        HealthEvent FABRIC_HEALTH_EVENT_ { get; set; }

        HealthEvaluation FABRIC_EVENT_HEALTH_EVALUATION_ { get; set; }

        HealthEvaluation FABRIC_HEALTH_EVALUATION_ { get; set; }

        ReplicaHealth FABRIC_REPLICA_HEALTH_ { get; set; }

        PartitionHealth FABRIC_PARTITION_HEALTH_ { get; set; }

        ServiceHealth FABRIC_SERVICE_HEALTH_ { get; set; }

        ApplicationHealth FABRIC_APPLICATION_HEALTH_ { get; set; }

        NodeHealth FABRIC_NODE_HEALTH_ { get; set; }

        ClusterHealth FABRIC_CLUSTER_HEALTH_ { get; set; }

        DeployedApplicationHealth FABRIC_DEPLOYED_APPLICATION_HEALTH_ { get; set; }

        DeployedServicePackageHealth FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_ { get; set; }

        HealthEvaluation FABRIC_REPLICA_HEALTH_EVALUATION_ { get; set; }
        
        HealthEvaluation FABRIC_REPLICAS_HEALTH_EVALUATION_ { get; set; }

        HealthEvaluation FABRIC_PARTITION_HEALTH_EVALUATION_ { get; set; }
        
        HealthEvaluation FABRIC_PARTITIONS_HEALTH_EVALUATION_ { get; set; }
        
        HealthEvaluation FABRIC_SERVICE_HEALTH_EVALUATION_ { get; set; }
        
        HealthEvaluation FABRIC_SERVICES_HEALTH_EVALUATION_ { get; set; }
        
        HealthEvaluation FABRIC_APPLICATION_HEALTH_EVALUATION_ { get; set; }
        
        HealthEvaluation FABRIC_APPLICATIONS_HEALTH_EVALUATION_ { get; set; }

        HealthEvaluation FABRIC_APPLICATION_TYPE_APPLICATIONS_HEALTH_EVALUATION_ { get; set; }
        
        HealthEvaluation FABRIC_SYSTEM_APPLICATION_HEALTH_EVALUATION_ { get; set; }
        
        HealthEvaluation FABRIC_NODE_HEALTH_EVALUATION_ { get; set; }
        
        HealthEvaluation FABRIC_NODES_HEALTH_EVALUATION_ { get; set; }

        HealthEvaluation FABRIC_UPGRADE_DOMAIN_NODES_HEALTH_EVALUATION_ { get; set; }
        
        HealthEvaluation FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_EVALUATION_ { get; set; }
        
        HealthEvaluation FABRIC_DEPLOYED_SERVICE_PACKAGES_HEALTH_EVALUATION_ { get; set; }
        
        HealthEvaluation FABRIC_DEPLOYED_APPLICATION_HEALTH_EVALUATION_ { get; set; }
        
        HealthEvaluation FABRIC_DEPLOYED_APPLICATIONS_HEALTH_EVALUATION_ { get; set; }

        HealthEvaluation FABRIC_UPGRADE_DOMAIN_DEPLOYED_APPLICATIONS_HEALTH_EVALUATION_ { get; set; }

        HealthEvaluation FABRIC_DELTA_NODES_CHECK_HEALTH_EVALUATION_ { get; set; }
        
        HealthEvaluation FABRIC_UPGRADE_DOMAIN_DELTA_NODES_CHECK_HEALTH_EVALUATION_ { get; set; }

        ClusterHealthPolicy FABRIC_CLUSTER_HEALTH_POLICY_ { get; set; }

        ClusterHealthPolicy FABRIC_CLUSTER_HEALTH_POLICY_NO_APP_TYPE_ { get; set; }

        ApplicationHealthPolicy FABRIC_APPLICATION_HEALTH_POLICY_ { get; set; }

        ReplicaHealthStateChunk FABRIC_REPLICA_HEALTH_STATE_CHUNK_ { get; set; }

        PartitionHealthStateChunk FABRIC_PARTITION_HEALTH_STATE_CHUNK_ { get; set; }

        ServiceHealthStateChunk FABRIC_SERVICE_HEALTH_STATE_CHUNK_ { get; set; }

        DeployedServicePackageHealthStateChunk FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_CHUNK_ { get; set; }

        DeployedApplicationHealthStateChunk FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_CHUNK_ { get; set; }

        ApplicationHealthStateChunk FABRIC_APPLICATION_HEALTH_STATE_CHUNK_ { get; set; }

        NodeHealthStateChunk FABRIC_NODE_HEALTH_STATE_CHUNK_ { get; set; }

        ClusterHealthChunk FABRIC_CLUSTER_HEALTH_CHUNK_ { get; set; }

        ReplicaHealthStateFilter FABRIC_REPLICA_HEALTH_STATE_FILTER_ { get; set; }

        PartitionHealthStateFilter FABRIC_PARTITION_HEALTH_STATE_FILTER_ { get; set; }

        ServiceHealthStateFilter FABRIC_SERVICE_HEALTH_STATE_FILTER_ { get; set; }

        DeployedServicePackageHealthStateFilter FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_FILTER_ { get; set; }

        DeployedApplicationHealthStateFilter FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_FILTER_ { get; set; }

        NodeHealthStateFilter FABRIC_NODE_HEALTH_STATE_FILTER_ { get; set; }

        ApplicationHealthStateFilter FABRIC_APPLICATION_HEALTH_STATE_FILTER_ { get; set; }

        ClusterHealthChunkQueryDescription FABRIC_CLUSTER_HEALTH_CHUNK_QUERY_DESCRIPTION_ { get; set; }

        public void Init()
        {
            var random = new Random((int)DateTime.Now.Ticks);

            // Basic types
            FABRIC_HEALTH_STATE_ = random.CreateRandom<HealthState>();
            FABRIC_HEALTH_EVENT_ = random.CreateRandom<HealthEvent>();

            // HealthEvaluation
            FABRIC_EVENT_HEALTH_EVALUATION_ = random.CreateRandom<EventHealthEvaluation>();
            FABRIC_HEALTH_EVALUATION_ = random.CreateRandom<HealthEvaluation>();

            FABRIC_REPLICA_HEALTH_EVALUATION_ = random.CreateRandom<ReplicaHealthEvaluation>();
            FABRIC_REPLICAS_HEALTH_EVALUATION_ = random.CreateRandom<ReplicasHealthEvaluation>();

            FABRIC_PARTITION_HEALTH_EVALUATION_ = random.CreateRandom<PartitionHealthEvaluation>();
            FABRIC_PARTITIONS_HEALTH_EVALUATION_ = random.CreateRandom<PartitionsHealthEvaluation>();

            FABRIC_SERVICE_HEALTH_EVALUATION_ = random.CreateRandom<ServiceHealthEvaluation>();
            FABRIC_SERVICES_HEALTH_EVALUATION_ = random.CreateRandom<ServicesHealthEvaluation>();
            FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_EVALUATION_ = random.CreateRandom<DeployedServicePackageHealthEvaluation>();
            FABRIC_DEPLOYED_SERVICE_PACKAGES_HEALTH_EVALUATION_ = random.CreateRandom<NodeHealthEvaluation>();

            FABRIC_APPLICATION_HEALTH_EVALUATION_ = random.CreateRandom<ApplicationHealthEvaluation>();
            FABRIC_SYSTEM_APPLICATION_HEALTH_EVALUATION_ = random.CreateRandom<SystemApplicationHealthEvaluation>();
            FABRIC_APPLICATIONS_HEALTH_EVALUATION_ = random.CreateRandom<ApplicationsHealthEvaluation>();
            FABRIC_APPLICATION_TYPE_APPLICATIONS_HEALTH_EVALUATION_ = random.CreateRandom<ApplicationTypeApplicationsHealthEvaluation>();
            FABRIC_DEPLOYED_APPLICATION_HEALTH_EVALUATION_ = random.CreateRandom<DeployedApplicationHealthEvaluation>();
            FABRIC_DEPLOYED_APPLICATIONS_HEALTH_EVALUATION_ = random.CreateRandom<DeployedApplicationsHealthEvaluation>();
            FABRIC_UPGRADE_DOMAIN_DEPLOYED_APPLICATIONS_HEALTH_EVALUATION_ = random.CreateRandom<UpgradeDomainDeployedApplicationsHealthEvaluation>();

            FABRIC_NODE_HEALTH_EVALUATION_ = random.CreateRandom<NodeHealthEvaluation>();
            FABRIC_NODES_HEALTH_EVALUATION_ = random.CreateRandom<NodesHealthEvaluation>();
            FABRIC_DELTA_NODES_CHECK_HEALTH_EVALUATION_ = random.CreateRandom<DeltaNodesCheckHealthEvaluation>();
            FABRIC_UPGRADE_DOMAIN_NODES_HEALTH_EVALUATION_ = random.CreateRandom<UpgradeDomainNodesHealthEvaluation>();
            FABRIC_UPGRADE_DOMAIN_DELTA_NODES_CHECK_HEALTH_EVALUATION_ = random.CreateRandom<UpgradeDomainDeltaNodesCheckHealthEvaluation>();

            // Health
            FABRIC_REPLICA_HEALTH_ = random.CreateRandom<ReplicaHealth>();
            FABRIC_PARTITION_HEALTH_ = random.CreateRandom<PartitionHealth>();
            FABRIC_SERVICE_HEALTH_ = random.CreateRandom<ServiceHealth>();
            FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_ = random.CreateRandom<DeployedServicePackageHealth>();

            FABRIC_APPLICATION_HEALTH_ = random.CreateRandom<ApplicationHealth>();
            FABRIC_NODE_HEALTH_ = random.CreateRandom<NodeHealth>();
            FABRIC_CLUSTER_HEALTH_ = random.CreateRandom<ClusterHealth>();
            FABRIC_DEPLOYED_APPLICATION_HEALTH_ = random.CreateRandom<DeployedApplicationHealth>();

            FABRIC_CLUSTER_HEALTH_POLICY_ = random.CreateRandom<ClusterHealthPolicy>();

            FABRIC_CLUSTER_HEALTH_POLICY_NO_APP_TYPE_ = random.CreateRandom<ClusterHealthPolicy>();
            FABRIC_CLUSTER_HEALTH_POLICY_NO_APP_TYPE_.ApplicationTypeHealthPolicyMap.Clear();

            FABRIC_APPLICATION_HEALTH_POLICY_ = random.CreateRandom<ApplicationHealthPolicy>();

            FABRIC_REPLICA_HEALTH_STATE_CHUNK_ = this.random.CreateRandom<ReplicaHealthStateChunk>();

            FABRIC_PARTITION_HEALTH_STATE_CHUNK_ = this.random.CreateRandom<PartitionHealthStateChunk>();
            FABRIC_PARTITION_HEALTH_STATE_CHUNK_.ReplicaHealthStateChunks = new ReplicaHealthStateChunkList() { FABRIC_REPLICA_HEALTH_STATE_CHUNK_ };
            
            FABRIC_SERVICE_HEALTH_STATE_CHUNK_ = this.random.CreateRandom<ServiceHealthStateChunk>();
            FABRIC_SERVICE_HEALTH_STATE_CHUNK_.PartitionHealthStateChunks = new PartitionHealthStateChunkList() { FABRIC_PARTITION_HEALTH_STATE_CHUNK_ };
            
            FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_CHUNK_ = this.random.CreateRandom<DeployedServicePackageHealthStateChunk>();
            
            FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_CHUNK_ = this.random.CreateRandom<DeployedApplicationHealthStateChunk>();
            FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_CHUNK_.DeployedServicePackageHealthStateChunks = new DeployedServicePackageHealthStateChunkList() { FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_CHUNK_ };
            
            FABRIC_APPLICATION_HEALTH_STATE_CHUNK_ = this.random.CreateRandom<ApplicationHealthStateChunk>();
            FABRIC_APPLICATION_HEALTH_STATE_CHUNK_.ServiceHealthStateChunks = new ServiceHealthStateChunkList() { FABRIC_SERVICE_HEALTH_STATE_CHUNK_ };
            FABRIC_APPLICATION_HEALTH_STATE_CHUNK_.DeployedApplicationHealthStateChunks = new DeployedApplicationHealthStateChunkList() { FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_CHUNK_ };
            
            FABRIC_NODE_HEALTH_STATE_CHUNK_ = this.random.CreateRandom<NodeHealthStateChunk>();

            FABRIC_CLUSTER_HEALTH_CHUNK_ = this.random.CreateRandom<ClusterHealthChunk>();
            FABRIC_CLUSTER_HEALTH_CHUNK_.ApplicationHealthStateChunks = new ApplicationHealthStateChunkList() { FABRIC_APPLICATION_HEALTH_STATE_CHUNK_ };
            FABRIC_CLUSTER_HEALTH_CHUNK_.NodeHealthStateChunks = new NodeHealthStateChunkList() { FABRIC_NODE_HEALTH_STATE_CHUNK_ };

            FABRIC_CLUSTER_HEALTH_CHUNK_ = this.random.CreateRandom<ClusterHealthChunk>();

            FABRIC_REPLICA_HEALTH_STATE_FILTER_ = this.random.CreateRandom<ReplicaHealthStateFilter>();

            FABRIC_PARTITION_HEALTH_STATE_FILTER_ = this.random.CreateRandom<PartitionHealthStateFilter>();
            FABRIC_PARTITION_HEALTH_STATE_FILTER_.ReplicaFilters.Add(FABRIC_REPLICA_HEALTH_STATE_FILTER_);

            FABRIC_SERVICE_HEALTH_STATE_FILTER_ = this.random.CreateRandom<ServiceHealthStateFilter>();
            FABRIC_SERVICE_HEALTH_STATE_FILTER_.PartitionFilters.Add(FABRIC_PARTITION_HEALTH_STATE_FILTER_);

            FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_FILTER_ = this.random.CreateRandom<DeployedServicePackageHealthStateFilter>();

            FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_FILTER_ = this.random.CreateRandom<DeployedApplicationHealthStateFilter>();
            FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_FILTER_.DeployedServicePackageFilters.Add(FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_FILTER_);

            FABRIC_APPLICATION_HEALTH_STATE_FILTER_ = this.random.CreateRandom<ApplicationHealthStateFilter>();
            FABRIC_APPLICATION_HEALTH_STATE_FILTER_.ServiceFilters.Add(FABRIC_SERVICE_HEALTH_STATE_FILTER_);
            FABRIC_APPLICATION_HEALTH_STATE_FILTER_.DeployedApplicationFilters.Add(FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_FILTER_);

            FABRIC_NODE_HEALTH_STATE_FILTER_ = this.random.CreateRandom<NodeHealthStateFilter>();

            FABRIC_CLUSTER_HEALTH_CHUNK_QUERY_DESCRIPTION_ = this.random.CreateRandom<ClusterHealthChunkQueryDescription>();
            FABRIC_CLUSTER_HEALTH_CHUNK_QUERY_DESCRIPTION_.ApplicationFilters.Add(FABRIC_APPLICATION_HEALTH_STATE_FILTER_);
            FABRIC_CLUSTER_HEALTH_CHUNK_QUERY_DESCRIPTION_.NodeFilters.Add(FABRIC_NODE_HEALTH_STATE_FILTER_);
        }
    }
}