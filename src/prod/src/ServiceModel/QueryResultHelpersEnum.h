// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

// NOTE: To add a new query result type
// - Create a enum in QueryResultHelpers.h
// - Create an entry for the enum in QueryResultHelpers.cpp
// - Create ServiceModel type.
//    - Use DEFINE_QUERY_RESULT_INTERNAL_LIST_ITEM by providing ServiceModel type, QueryResultHelpers enum.
//    - This type will be only available internally via QueryResult.
//    - Add public query API's and corresponding result and interfaces.

// - **TO BE REMOVED ONCE FABRICTEST IS FIXED**
// - If the type is public and returned as query result via Query API
//    - Create IDL type
//    - Create en entry in FABRIC_QUERY_RESULT_ITEM_KIND enum
//    - Use DEFINE_QUERY_RESULT_LIST_ITEM by providing ServiceModel type, QueryResultHelpers enum, IDL type, FABRIC_QUERY_RESULT_ITEM_KIND enum


namespace ServiceModel
{
    namespace QueryResultHelpers
    {
        enum Enum
        {
            Invalid = 0,
            Node = 1,
            Service = 2,
            ApplicationType = 3,
            Application = 4,
            ServicePartition = 5,
            ServiceReplica = 6,
            QueryMetadata = 7,
            String = 8,
            ServiceType = 9,
            DeployedApplication = 10,
            DeployedServiceManifest = 11,
            DeployedServiceReplica = 12,
            DeployedServiceType = 13,
            DeployedCodePackage = 14,
            ApplicationQueryResult = 15,
            NodeHealth = 16,
            ReplicaHealth = 17,
            PartitionHealth = 18,
            ServiceHealth = 19,
            ApplicationHealth = 20,
            ClusterHealth = 21,
            DeployedApplicationHealth = 22,
            DeployedServicePackageHealth = 23,
            NodeAggregatedHealthState = 24,
            ReplicaAggregatedHealthState = 25,
            PartitionAggregatedHealthState = 26,
            ServiceAggregatedHealthState = 27,
            ApplicationAggregatedHealthState = 28,
            DeployedApplicationAggregatedHealthState = 29,
            DeployedServicePackageAggregatedHealthState = 30,
            InfrastructureTask = 31,
            DeployedServiceReplicaDetail = 32,
            RepairTask = 33,
            ClusterLoadInformation = 34,
            PartitionLoadInformation = 35,
            ProvisionedFabricCodeVersion = 36,
            ProvisionedFabricConfigVersion = 37,
            InternalDeletedApplicationsQueryObject = 38,
            InternalProvisionedApplicationTypeQueryResult = 39,
            PackageSharingPolicyQueryObject = 40,
            PackageSharingPolicyList = 41,
            NodeLoadInformation = 42,
            ReplicaLoadInformation = 43,
            ServiceGroupMember = 44,
            ServiceGroupMemberType = 45,
            UnplacedReplicaInformation = 46,
            ApplicationLoadInformation = 47,
            ClusterHealthChunk = 48,
            TestCommandList = 49,
            ServiceName = 50,
            ApplicationName = 51,
            ComposeDeploymentStatus = 52,
            ComposeDeploymentUpgradeProgress = 53,
            ReplicasByService = 54,
            ApplicationResourceList = 55,
            ServiceResourceList = 56,
            ReplicaResource = 57,
            ApplicationUnhealthyEvaluation = 58,
            VolumeResourceList = 59,
            Network = 60,
            NetworkApplication = 61,
            NetworkNode = 62,
            ApplicationNetwork = 63,
            DeployedNetwork = 64,
            DeployedNetworkCodePackage = 65,
            NetworkResourceList = 66,
            GatewayResourceList = 67
        };

        Serialization::FabricSerializable* CreateNew(QueryResultHelpers::Enum resultKind);
    }
}
