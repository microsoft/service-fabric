// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Query
{
    namespace QueryNames
    {
        enum Enum
        {
            FIRST_QUERY_MINUS_ONE = 0,
            GetQueries = 1,
            GetNodeList = 2,
            GetApplicationList = 3,
            GetApplicationTypeList = 4,
            GetApplicationServiceList = 5,
            GetServiceTypeList = 6,
            GetSystemServicesList = 7,
            GetServicePartitionList = 8,
            GetServicePartitionReplicaList = 9,
            GetNodeHealth = 10,
            GetServicePartitionHealth = 11,
            GetServicePartitionReplicaHealth = 12,
            GetApplicationManifest = 13,
            GetClusterManifest = 14,
            GetApplicationListDeployedOnNode = 15,
            GetServiceManifest = 16,
            GetServiceManifestListDeployedOnNode = 17,
            GetServiceTypeListDeployedOnNode = 18,
            GetServiceHealth = 19,
            GetApplicationHealth = 20,
            GetClusterHealth = 21,
            GetAggregatedNodeHealthList = 22,
            GetAggregatedServicePartitionHealthList = 23,
            GetAggregatedServicePartitionReplicaHealthList = 24,
            GetAggregatedServiceHealthList = 25,
            GetCodePackageListDeployedOnNode = 26,
            GetServiceReplicaListDeployedOnNode = 27,
            GetAggregatedApplicationHealthList = 28,
            GetInfrastructureTask = 29,
            GetDeployedApplicationHealth = 30,
            GetDeployedServicePackageHealth = 31,
            GetAggregatedDeployedApplicationHealthList = 32,
            GetAggregatedDeployedServicePackageHealthList = 33,
            GetDeployedServiceReplicaDetail = 34,
            MovePrimary = 35,
            MoveSecondary = 36,
            StopNode = 37,
            RestartDeployedCodePackage = 38,
            GetRepairList = 39,
            GetClusterLoadInformation = 40,
            GetPartitionLoadInformation = 41,
            GetProvisionedFabricCodeVersionList = 42,
            GetProvisionedFabricConfigVersionList = 43,
            GetDeletedApplicationsList = 44,
            GetProvisionedApplicationTypePackages = 45,
            GetFMNodeList = 46,
            DeployServicePackageToNode = 47,
            GetNodeLoadInformation = 48,
            GetReplicaLoadInformation = 49,
            AddUnreliableTransportBehavior = 50,
            RemoveUnreliableTransportBehavior = 51,
            GetApplicationServiceGroupMemberList = 52,
            GetServiceGroupMemberTypeList = 53,
            GetUnplacedReplicaInformation = 54,
            GetApplicationLoadInformation = 55,
            GetClusterHealthChunk = 56,
            GetTestCommandList = 57,
            GetTransportBehaviors = 58,
            AddUnreliableLeaseBehavior = 59,
            RemoveUnreliableLeaseBehavior = 60,
            GetClusterConfiguration = 61,
            GetServiceName = 62,
            GetApplicationName = 63,
            GetApplicationTypePagedList = 64,
            GetApplicationPagedListDeployedOnNode = 65,
            GetAggregatedDeployedApplicationsOnNodeHealthPagedList = 66,
            GetComposeDeploymentStatusList = 67,
            GetComposeDeploymentUpgradeProgress = 68,
            GetContainerInfoDeployedOnNode = 69,
            GetDeployedCodePackageListByApplication = 70,
            GetReplicaListByServiceNames = 71,
            InvokeContainerApiOnNode = 72,
            GetApplicationResourceList = 73,
            GetServiceResourceList = 74,
            GetContainerCodePackageLogs = 75,
            GetReplicaResourceList = 76,
            GetApplicationUnhealthyEvaluation = 77,
            GetVolumeResourceList = 78,
            GetClusterVersion = 79,
            GetNetworkList = 80,
            GetNetworkApplicationList = 81,
            GetNetworkNodeList = 82,
            GetApplicationNetworkList = 83,
            GetDeployedNetworkList = 84,
            GetDeployedNetworkCodePackageList = 85,
            GetGatewayResourceList = 86,
            LAST_QUERY_PLUS_ONE = 87
        };

        std::wstring ToString(Enum const & val);

        void WriteToTextWriter(__in Common::TextWriter & w, Enum const & val);

        DECLARE_ENUM_STRUCTURED_TRACE( QueryNames )
    };
}
