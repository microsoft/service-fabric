// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Common.Serialization
{
    using Newtonsoft.Json;
    using System.Collections.Generic;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Query;

    /// Json properties of this class should match json properties of native class defined in file:
    /// %SDXROOT%\services\winfab\prod\test\System.Fabric\Common\NativeAndManagedSerializationInteropTest\QuerySerializationInteropTest.h
    internal class QuerySerializationInteropTest
    {
        [JsonIgnore]
        Random random = new Random((int)DateTime.Now.Ticks);

        // Application Type
        ApplicationType FABRIC_APPLICATION_TYPE_QUERY_RESULT_ITEM_ { get; set; }

        ApplicationTypeList FABRIC_APPLICATION_TYPE_QUERY_RESULT_ITEM_LIST_ { get; set; }

        ApplicationTypePagedList FABRIC_APPLICATION_TYPE_QUERY_RESULT_ITEM_PAGED_LIST_ { get; set; }

        // Application
        Application FABRIC_APPLICATION_QUERY_RESULT_ITEM_ { get; set; }

        ApplicationList FABRIC_APPLICATION_QUERY_RESULT_ITEM_LIST_ { get; set; }

        // Service Type
        ServiceTypeDescription FABRIC_SERVICE_TYPE_DESCRIPTION_ { get; set; }

        ServiceType FABRIC_SERVICE_TYPE_QUERY_RESULT_ITEM_ { get; set; }

        ServiceTypeList FABRIC_SERVICE_TYPE_QUERY_RESULT_LIST_ { get; set; }

        // Service
        Service FABRIC_SERVICE_QUERY_RESULT_ITEM_ { get; set; }

        ServiceList FABRIC_SERVICE_QUERY_RESULT_LIST_ { get; set; }

        // Replica
        Replica FABRIC_SERVICE_REPLICA_QUERY_RESULT_ITEM_ { get; set; }

        ServiceReplicaList FABRIC_SERVICE_REPLICA_LIST_RESULT_ { get; set; }

        // Partition
        ServicePartitionInformation FABRIC_SERVICE_PARTITION_INFORMATION_ { get; set; }

        ServicePartitionList FABRIC_SERVICE_PARTITION_LIST_RESULT_ { get; set; }

        Partition FABRIC_SERVICE_PARTITION_QUERY_RESULT_ITEM_ { get; set; }

        // Node
        NodeDeactivationResult FABRIC_NODE_DEACTIVATION_QUERY_RESULT_ITEM_ { get; set; }

        NodeDeactivationTask FABRIC_NODE_DEACTIVATION_TASK_ { get; set; }

        Node FABRIC_NODE_QUERY_RESULT_ITEM_ { get; set; }

        NodeList FABRIC_NODE_LIST_QUERY_RESULT_ { get; set; }

        DeployedStatefulServiceReplicaDetail FABRIC_DEPLOYED_SERVICE_REPLICA_DETAIL_QUERY_RESULT_ITEM_ { get; set; }
        DeployedStatelessServiceInstanceDetail FABRIC_DEPLOYED_SERVICE_REPLICA_DETAIL_QUERY_RESULT_ITEM2_ { get; set; }

        // Deployed Application
        [JsonProperty(DefaultValueHandling = DefaultValueHandling.Ignore)]
        DeployedApplication FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_ITEM_ { get; set; }

        DeployedApplicationList FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_ITEM_LIST_ { get; set; }

        DeployedApplicationPagedList FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_ITEM_PAGED_LIST_ { get; set; }

        // Deployed Replica
        DeployedStatefulServiceReplica FABRIC_DEPLOYED_SERVICE_REPLICA_QUERY_RESULT_ITEM_ { get; set; }

        DeployedStatelessServiceInstance FABRIC_DEPLOYED_SERVICE_REPLICA_QUERY_RESULT_ITEM2_ { get; set; }

        DeployedServiceReplica FABRIC_DEPLOYED_SERVICE_REPLICA_QUERY_RESULT_ITEM3_ { get; set; }

        // Code Package
        CodePackageEntryPointStatistics FABRIC_CODE_PACKAGE_ENTRY_POINT_STATISTICS_ { get; set; }

        CodePackageEntryPoint FABRIC_CODE_PACKAGE_ENTRY_POINT_ { get; set; }

        DeployedCodePackage FABRIC_DEPLOYED_CODE_PACKAGE_QUERY_RESULT_ITEM_ { get; set; }

        DeployedCodePackageList FABRIC_DEPLOYED_CODE_PACKAGE_QUERY_RESULT_ITEM_LIST_ { get; set; }

        DeployedServicePackage FABRIC_DEPLOYED_SERVICE_PACKAGE_QUERY_RESULT_ITEM_ { get; set; }

        // Replicator
        PrimaryReplicatorStatus FABRIC_PRIMARY_REPLICATOR_STATUS_QUERY_RESULT_ { get; set; }

        SecondaryReplicatorStatus FABRIC_SECONDARY_REPLICATOR_STATUS_QUERY_RESULT_ { get; set; }

        // Load
        LoadMetricReport FABRIC_LOAD_METRIC_REPORT_ { get; set; }

        LoadMetricInformation FABRIC_LOAD_METRIC_INFORMATION_ { get; set; }

        ReplicaLoadInformation FABRIC_REPLICA_LOAD_INFORMATION_ { get; set; }

        PartitionLoadInformation FABRIC_PARTITION_LOAD_INFORMATION_ { get; set; }

        NodeLoadInformation FABRIC_NODE_LOAD_INFORMATION_ { get; set; }

        NodeLoadMetricInformation FABRIC_NODE_LOAD_METRIC_INFORMATION_ { get; set; }

        // Service Group
        ServiceGroupMemberType FABRIC_SERVICE_GROUP_MEMBER_TYPE_QUERY_RESULT_ITEM_ { get; set; }

        //[JsonProperty(DefaultValueHandling = DefaultValueHandling.Ignore)] //TODO remove these
        ServiceGroupTypeMemberDescription FABRIC_SERVICE_GROUP_TYPE_MEMBER_DESCRIPTION_ { get; set; }

        public void Init()
        {
            // Application Type
            FABRIC_APPLICATION_TYPE_QUERY_RESULT_ITEM_ = this.random.CreateRandom<ApplicationType>();
            FABRIC_APPLICATION_TYPE_QUERY_RESULT_ITEM_LIST_ = new ApplicationTypeList() { this.random.CreateRandom<ApplicationType>() };

            FABRIC_APPLICATION_TYPE_QUERY_RESULT_ITEM_PAGED_LIST_ = new ApplicationTypePagedList();
            FABRIC_APPLICATION_TYPE_QUERY_RESULT_ITEM_PAGED_LIST_.ContinuationToken = "ContinuationToken342741";
            FABRIC_APPLICATION_TYPE_QUERY_RESULT_ITEM_PAGED_LIST_.Add(this.random.CreateRandom<ApplicationType>());
            FABRIC_APPLICATION_TYPE_QUERY_RESULT_ITEM_PAGED_LIST_.Add(this.random.CreateRandom<ApplicationType>());
            FABRIC_APPLICATION_TYPE_QUERY_RESULT_ITEM_PAGED_LIST_.Add(this.random.CreateRandom<ApplicationType>());

            // Application
            FABRIC_APPLICATION_QUERY_RESULT_ITEM_ = this.random.CreateRandom<Application>();
            FABRIC_APPLICATION_QUERY_RESULT_ITEM_LIST_ = new ApplicationList() { this.random.CreateRandom<Application>(), this.random.CreateRandom<Application>() };

            // Service Type
            FABRIC_SERVICE_TYPE_DESCRIPTION_ = this.random.CreateRandom<ServiceTypeDescription>();
            FABRIC_SERVICE_TYPE_QUERY_RESULT_ITEM_ = this.random.CreateRandom<ServiceType>();
            FABRIC_SERVICE_TYPE_QUERY_RESULT_LIST_ = new ServiceTypeList() { this.random.CreateRandom<ServiceType>() };

            //Replica
            FABRIC_SERVICE_REPLICA_QUERY_RESULT_ITEM_ = this.CreateReplica();

            FABRIC_SERVICE_REPLICA_LIST_RESULT_ = new ServiceReplicaList();
            FABRIC_SERVICE_REPLICA_LIST_RESULT_.ContinuationToken = "4387284";
            FABRIC_SERVICE_REPLICA_LIST_RESULT_.Add(this.CreateReplica());
            FABRIC_SERVICE_REPLICA_LIST_RESULT_.Add(this.CreateReplica());

            // Partition
            FABRIC_SERVICE_PARTITION_INFORMATION_ = this.random.CreateRandom<ServicePartitionInformation>();
            FABRIC_SERVICE_PARTITION_QUERY_RESULT_ITEM_ = CreatePartition();

            FABRIC_SERVICE_PARTITION_LIST_RESULT_ = new ServicePartitionList();
            FABRIC_SERVICE_PARTITION_LIST_RESULT_.ContinuationToken = Guid.NewGuid().ToString();
            FABRIC_SERVICE_PARTITION_LIST_RESULT_.Add(this.CreatePartition());
            FABRIC_SERVICE_PARTITION_LIST_RESULT_.Add(this.CreatePartition());

            //Service
            FABRIC_SERVICE_QUERY_RESULT_ITEM_ = this.CreateServiceQueryItem();
            FABRIC_SERVICE_QUERY_RESULT_LIST_ = new ServiceList() { this.CreateServiceQueryItem(), this.CreateServiceQueryItem() };

            // Node
            FABRIC_NODE_DEACTIVATION_QUERY_RESULT_ITEM_ = this.random.CreateRandom<NodeDeactivationResult>();
            FABRIC_NODE_DEACTIVATION_TASK_ = this.random.CreateRandom<NodeDeactivationTask>();
            FABRIC_NODE_QUERY_RESULT_ITEM_ = this.random.CreateRandom<Node>();

            // NodeList with continuation token
            FABRIC_NODE_LIST_QUERY_RESULT_ = new NodeList();
            FABRIC_NODE_LIST_QUERY_RESULT_.ContinuationToken = "ContinuationToken34274";
            FABRIC_NODE_LIST_QUERY_RESULT_.Add(this.random.CreateRandom<Node>());
            FABRIC_NODE_LIST_QUERY_RESULT_.Add(this.random.CreateRandom<Node>());
            FABRIC_NODE_LIST_QUERY_RESULT_.Add(this.random.CreateRandom<Node>());

            // Deployed Application
            FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_ITEM_ = this.random.CreateRandom<DeployedApplication>();
            FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_ITEM_LIST_ = new DeployedApplicationList() { FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_ITEM_ };

            FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_ITEM_PAGED_LIST_ = new DeployedApplicationPagedList();
            FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_ITEM_PAGED_LIST_.ContinuationToken = "ContinuationToken342741";
            FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_ITEM_PAGED_LIST_.Add(this.random.CreateRandom<DeployedApplication>());
            FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_ITEM_PAGED_LIST_.Add(this.random.CreateRandom<DeployedApplication>());
            FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_ITEM_PAGED_LIST_.Add(this.random.CreateRandom<DeployedApplication>());

            // Deployed Service
            FABRIC_DEPLOYED_SERVICE_REPLICA_QUERY_RESULT_ITEM_ = this.random.CreateRandom<DeployedStatefulServiceReplica>();
            FABRIC_DEPLOYED_SERVICE_REPLICA_QUERY_RESULT_ITEM2_ = this.random.CreateRandom<DeployedStatelessServiceInstance>();
            FABRIC_DEPLOYED_SERVICE_REPLICA_QUERY_RESULT_ITEM3_ = this.random.CreateRandom<DeployedServiceReplica>();
            FABRIC_DEPLOYED_SERVICE_REPLICA_DETAIL_QUERY_RESULT_ITEM_ = this.random.CreateRandom<DeployedStatefulServiceReplicaDetail>();
            FABRIC_DEPLOYED_SERVICE_REPLICA_DETAIL_QUERY_RESULT_ITEM2_ = this.random.CreateRandom<DeployedStatelessServiceInstanceDetail>();

            // Code Package
            FABRIC_CODE_PACKAGE_ENTRY_POINT_STATISTICS_ = this.random.CreateRandom<CodePackageEntryPointStatistics>();
            FABRIC_CODE_PACKAGE_ENTRY_POINT_ = this.random.CreateRandom<CodePackageEntryPoint>();
            FABRIC_DEPLOYED_CODE_PACKAGE_QUERY_RESULT_ITEM_ = this.random.CreateRandom<DeployedCodePackage>();
            FABRIC_DEPLOYED_CODE_PACKAGE_QUERY_RESULT_ITEM_LIST_ = new DeployedCodePackageList() { FABRIC_DEPLOYED_CODE_PACKAGE_QUERY_RESULT_ITEM_ };
            FABRIC_DEPLOYED_SERVICE_PACKAGE_QUERY_RESULT_ITEM_ = this.random.CreateRandom<DeployedServicePackage>();

            // Replicator
            FABRIC_PRIMARY_REPLICATOR_STATUS_QUERY_RESULT_ = this.random.CreateRandom<PrimaryReplicatorStatus>();
            FABRIC_SECONDARY_REPLICATOR_STATUS_QUERY_RESULT_ = this.random.CreateRandom<SecondaryReplicatorStatus>();

            // Load
            FABRIC_LOAD_METRIC_INFORMATION_ = this.random.CreateRandom<LoadMetricInformation>();
            FABRIC_LOAD_METRIC_REPORT_ = this.random.CreateRandom<LoadMetricReport>();
            FABRIC_NODE_LOAD_METRIC_INFORMATION_ = this.random.CreateRandom<NodeLoadMetricInformation>();
            FABRIC_NODE_LOAD_INFORMATION_ = this.random.CreateRandom<NodeLoadInformation>();
            FABRIC_PARTITION_LOAD_INFORMATION_ = this.random.CreateRandom<PartitionLoadInformation>();
            FABRIC_REPLICA_LOAD_INFORMATION_ = this.random.CreateRandom<ReplicaLoadInformation>();

            // Service Group
            FABRIC_SERVICE_GROUP_TYPE_MEMBER_DESCRIPTION_ = this.random.CreateRandom<ServiceGroupTypeMemberDescription>();
            FABRIC_SERVICE_GROUP_MEMBER_TYPE_QUERY_RESULT_ITEM_ = this.random.CreateRandom<ServiceGroupMemberType>();
        }

        // Service does not have default constructor so need a method.
        private Service CreateServiceQueryItem()
        {
            if (this.random.Next() % 2 == 0)
            {
                return new StatefulService(
                    this.random.CreateRandom<Uri>(),
                    this.random.CreateRandom<string>(),
                    this.random.CreateRandom<string>(),
                    this.random.CreateRandom<bool>(),
                    this.random.CreateRandom<HealthState>(),
                    this.random.CreateRandom<ServiceStatus>());
            }
            else
            {
                return new StatelessService(
                    this.random.CreateRandom<Uri>(),
                    this.random.CreateRandom<string>(),
                    this.random.CreateRandom<string>(),
                    this.random.CreateRandom<HealthState>(),
                    this.random.CreateRandom<ServiceStatus>());
            }
        }

        // Partition does not have default constructor so need a method.
        private Partition CreatePartition()
        {
            if (this.random.Next() % 2 == 0)
            {
                ServicePartitionStatus partitionStatus = this.random.CreateRandom<ServicePartitionStatus>();
                var quorumLossDuration = partitionStatus != ServicePartitionStatus.InQuorumLoss ? TimeSpan.FromSeconds(0) : this.random.CreateRandom<TimeSpan>();

                return new StatefulServicePartition(
                    this.random.CreateRandom<ServicePartitionInformation>(),
                    7,
                    4,
                    this.random.CreateRandom<HealthState>(),
                    partitionStatus,
                    quorumLossDuration,
                    new Epoch());
            }
            else
            {
                return new StatelessServicePartition(
                    this.random.CreateRandom<ServicePartitionInformation>(),
                    7,
                    this.random.CreateRandom<HealthState>(),
                    ServicePartitionStatus.Ready);
            }
        }

        // Replica does not have default constructor so need a method.
        private Replica CreateReplica()
        {
            Replica result;
            if (this.random.Next() % 2 == 0)
            {
                result = new StatefulServiceReplica(
                    this.random.CreateRandom<ServiceReplicaStatus>(),
                    this.random.CreateRandom<HealthState>(),
                    ReplicaRole.IdleSecondary,
                    this.random.CreateRandom<Uri>().ToString(),
                    this.random.CreateRandom<string>(),
                    this.random.CreateRandom<long>(),
                    TimeSpan.FromSeconds(2));
            }
            else
            {
                result = new StatelessServiceInstance(
                    this.random.CreateRandom<ServiceReplicaStatus>(),
                    this.random.CreateRandom<HealthState>(),
                    this.random.CreateRandom<Uri>().ToString(),
                    this.random.CreateRandom<string>(),
                    this.random.CreateRandom<long>(),
                    TimeSpan.FromSeconds(4));
            }

            return result;
        }

        //private NodeId CreateNodeId()
        //{
        //    var nodeId = new NodeId(this.random.CreateRandom<long>())
        //    nodeId.Low = this.random.CreateRandom<long>();
        //    nodeId.High = this.random.CreateRandom<long>();

        //    return nodeId;
        //}
    }
}