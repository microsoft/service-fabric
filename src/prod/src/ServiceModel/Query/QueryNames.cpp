// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

namespace Query
{
    namespace QueryNames
    {
        std::wstring ToString(Enum const & val)
        {
            switch(val)
            {
                case QueryNames::GetQueries:
                    return L"GetQueries";
                case QueryNames::GetNodeList:
                    return L"GetNodeList";
                case QueryNames::GetApplicationList:
                    return L"GetApplicationList";
                case QueryNames::GetApplicationTypeList:
                    return L"GetApplicationTypeList";
                case QueryNames::GetApplicationServiceList:
                    return L"GetApplicationServiceList";
                case QueryNames::GetApplicationServiceGroupMemberList:
                    return L"GetApplicationServiceGroupMemberList";
                case QueryNames::GetServiceTypeList:
                    return L"GetServiceTypeList";
                case QueryNames::GetServiceGroupMemberTypeList:
                    return L"GetServiceGroupMemberTypeList";
                case QueryNames::GetSystemServicesList:
                    return L"GetSystemServicesList";
                case QueryNames::GetServicePartitionList:
                    return L"GetServicePartitionList";
                case QueryNames::GetServicePartitionReplicaList:
                    return L"GetServicePartitionReplicaList";
                case QueryNames::GetNodeHealth:
                    return L"GetNodeHealth";
                case QueryNames::GetServicePartitionHealth:
                    return L"GetServicePartitionHealth";
                case QueryNames::GetServicePartitionReplicaHealth:
                    return L"GetServicePartitionReplicaHealth";
                case QueryNames::GetApplicationManifest:
                    return L"GetApplicationManifest";
                case QueryNames::GetClusterManifest:
                    return L"GetClusterManifest";
                case QueryNames::GetApplicationListDeployedOnNode:
                    return L"GetApplicationListDeployedOnNode";
                case QueryNames::GetServiceManifest:
                    return L"GetServiceManifest";
                case QueryNames::GetServiceManifestListDeployedOnNode:
                    return L"GetServiceManifestListDeployedOnNode";
                case QueryNames::GetServiceTypeListDeployedOnNode:
                    return L"GetServiceTypeListDeployedOnNode";
                case QueryNames::GetServiceHealth:
                    return L"GetServiceHealth";
                case QueryNames::GetApplicationHealth:
                    return L"GetApplicationHealth";
                case QueryNames::GetClusterHealth:
                    return L"GetClusterHealth";
                case QueryNames::GetAggregatedNodeHealthList:
                    return L"GetAggregatedNodeHealthList";
                case QueryNames::GetAggregatedServicePartitionHealthList:
                    return L"GetAggregatedServicePartitionHealthList";
                case QueryNames::GetAggregatedServicePartitionReplicaHealthList:
                    return L"GetAggregatedServicePartitionReplicaHealthList";
                case QueryNames::GetAggregatedServiceHealthList:
                    return L"GetAggregatedServiceHealthList";
                case QueryNames::GetCodePackageListDeployedOnNode:
                    return L"GetCodePackageListDeployedOnNode";
                case QueryNames::GetServiceReplicaListDeployedOnNode:
                    return L"GetServiceReplicaListDeployedOnNode";
                case QueryNames::GetAggregatedApplicationHealthList:
                    return L"GetAggregatedApplicationHealthList";
                case QueryNames::GetInfrastructureTask:
                    return L"GetInfrastructureTask";
                case QueryNames::GetDeployedApplicationHealth:
                    return L"GetDeployedApplicationHealth";
                case QueryNames::GetDeployedServicePackageHealth:
                    return L"GetDeployedServicePackageHealth";
                case QueryNames::GetAggregatedDeployedApplicationHealthList:
                    return L"GetAggregatedDeployedApplicationHealthList";
                case QueryNames::GetAggregatedDeployedServicePackageHealthList:
                    return L"GetAggregatedDeployedServicePackageHealthList";
                case QueryNames::GetDeployedServiceReplicaDetail:
                    return L"GetDeployedServiceReplicaDetail";
                case QueryNames::MovePrimary:
                    return L"MovePrimary";
                case QueryNames::MoveSecondary:
                    return L"MoveSecondary";
                case QueryNames::StopNode:
                    return L"StopNode";
                case QueryNames::RestartDeployedCodePackage:
                    return L"RestartDeployedCodePackage";
                case QueryNames::GetContainerInfoDeployedOnNode:
                    return L"GetContainerInfoDeployedOnNode";
                case QueryNames::InvokeContainerApiOnNode:
                    return L"InvokeContainerApiOnNode";
                case QueryNames::GetTestCommandList:
                    return L"GetTestCommandList";
                case QueryNames::GetRepairList:
                    return L"GetRepairList";
                case QueryNames::GetClusterLoadInformation:
                    return L"GetClusterLoadInformation";
                case QueryNames::GetPartitionLoadInformation:
                    return L"GetPartitionLoadInformation";
                case QueryNames::GetProvisionedFabricCodeVersionList:
                    return L"GetProvisionedFabricCodeVersionList";
                case QueryNames::GetProvisionedFabricConfigVersionList:
                    return L"GetProvisionedFabricConfigVersionList";
                case QueryNames::GetDeletedApplicationsList:
                    return L"GetDeletedApplicationsList";
                case QueryNames::GetProvisionedApplicationTypePackages:
                    return L"GetProvisionedApplicationTypePackages";
                 // This is an internal only query to get node info from FM.
                 // The existing node query is left unchanged to handle requests from
                 // previous versions of code running during upgrade.
                case QueryNames::GetFMNodeList:
                    return L"GetFMNodeList";
                case QueryNames::DeployServicePackageToNode:
                    return L"DeployServicePackageToNode";
                case QueryNames::GetNodeLoadInformation:
                    return L"GetNodeLoadInformation";
                case QueryNames::GetReplicaLoadInformation:
                    return L"GetReplicaLoadInformation";
                case QueryNames::AddUnreliableTransportBehavior:
                    return L"AddUnreliableTransportBehavior";
                case QueryNames::RemoveUnreliableTransportBehavior:
                    return L"RemoveUnreliableTransportBehavior";
                case QueryNames::GetUnplacedReplicaInformation:
                    return L"GetUnplacedReplicaInformation";
                case QueryNames::GetApplicationLoadInformation:
                    return L"GetApplicationLoadInformation";
                case QueryNames::GetClusterHealthChunk:
                    return L"GetClusterHealthChunk";
                case QueryNames::GetTransportBehaviors:
                    return L"GetTransportBehaviors";
                case QueryNames::AddUnreliableLeaseBehavior:
                    return L"AddUnreliableLeaseBehavior";
                case QueryNames::RemoveUnreliableLeaseBehavior:
                    return L"RemoveUnreliableLeaseBehavior";
                case QueryNames::GetClusterConfiguration:
                    return L"GetClusterConfiguration";
                case QueryNames::GetServiceName:
                    return L"GetServiceName";
                case QueryNames::GetApplicationName:
                    return L"GetApplicationName";
                case QueryNames::GetApplicationTypePagedList:
                    return L"GetApplicationTypePagedList";
                case QueryNames::GetComposeDeploymentStatusList:
                    // Keep as old name for backward compatibility
                    return L"GetDockerComposeApplicationStatusList";
                case QueryNames::GetApplicationPagedListDeployedOnNode:
                    return L"GetApplicationPagedListDeployedOnNode";
                case QueryNames::GetAggregatedDeployedApplicationsOnNodeHealthPagedList:
                    return L"GetAggregatedDeployedApplicationsOnNodeHealthPagedList";
                case QueryNames::GetComposeDeploymentUpgradeProgress:
                    return L"GetComposeDeploymentUpgradeProgress";
                case QueryNames::GetDeployedCodePackageListByApplication:
                    return L"GetDeployedCodePackageListByApplication";
                case QueryNames::GetReplicaListByServiceNames:
                    return L"GetReplicaListByServiceNames";
                case QueryNames::GetApplicationResourceList:
                    return L"GetApplicationResourceList";
                case QueryNames::GetServiceResourceList:
                    return L"GetServiceResourceList";
                case QueryNames::GetContainerCodePackageLogs:
                    return L"GetContainerCodePackageLogs";
                case QueryNames::GetReplicaResourceList:
                    return L"GetReplicaResourceList";
                case QueryNames::GetApplicationUnhealthyEvaluation:
                    return L"GetApplicationUnhealthyEvaluation";
                case QueryNames::GetVolumeResourceList:
                    return L"GetVolumeResourceList";
                case QueryNames::GetClusterVersion:
                    return L"GetClusterVersion";
                case QueryNames::GetNetworkList:
                    return L"GetNetworkList";
                case QueryNames::GetNetworkApplicationList:
                    return L"GetNetworkApplicationList";
                case QueryNames::GetNetworkNodeList:
                    return L"GetNetworkNodeList";
                case QueryNames::GetApplicationNetworkList:
                    return L"GetApplicationNetworkList";
                case QueryNames::GetDeployedNetworkList:
                    return L"GetDeployedNetworkList";
                case QueryNames::GetDeployedNetworkCodePackageList:
                    return L"GetDeployedNetworkCodePackageList";
                case QueryNames::GetGatewayResourceList:
                    return L"GetGatewayResourceList";
                default:
                    return wformatString("Unknown QueryNames::Enum value {0}", static_cast<int>(val));
            }
        }

        void WriteToTextWriter(__in Common::TextWriter & w, Enum const & val)
        {
            w << QueryNames::ToString(val);
        }

        ENUM_STRUCTURED_TRACE(QueryNames, static_cast<QueryNames::Enum>(FIRST_QUERY_MINUS_ONE + 1), static_cast<QueryNames::Enum>(LAST_QUERY_PLUS_ONE - 1))
    }
}
