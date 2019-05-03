// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

namespace Naming
{
    DEFINE_SINGLETON_COMPONENT_CONFIG(ClientAccessConfig)

#define CLIENT_OPERATION_ACE_INIT(Operation) \
    { \
        Operation##Roles = GetRoles(Operation); \
        Operation##Entry.AddHandler( \
            [this] (Common::EventArgs const &) { Operation##Roles = ClientAccessConfig::GetRoles(Operation); }); \
    }

    Transport::RoleMask::Enum ClientAccessConfig::GetRoles(wstring const & roles)
    {
        vector<wstring> roleList;
        StringUtility::Split<wstring>(roles, roleList, L"||", true);
        ASSERT_IF(roleList.empty(), "role list cannot be empty");

        unsigned int roleMask = 0;
        for(auto const & roleName : roleList)
        {
            Transport::RoleMask::Enum role;
            auto error = Transport::RoleMask::TryParse(roleName, role);
            ASSERT_IFNOT(error.IsSuccess(), "Invalid role name: {0}", roleName);
            roleMask |= (unsigned int)role;
        }

        return (Transport::RoleMask::Enum)roleMask;
    }

    void ClientAccessConfig::Initialize()
    {
        CLIENT_OPERATION_ACE_INIT( CreateService )
        CLIENT_OPERATION_ACE_INIT( CreateServiceFromTemplate )
        CLIENT_OPERATION_ACE_INIT( UpdateService )
        CLIENT_OPERATION_ACE_INIT( DeleteService )
        CLIENT_OPERATION_ACE_INIT( GetServiceDescription )
        CLIENT_OPERATION_ACE_INIT( ActivateNode )
        CLIENT_OPERATION_ACE_INIT( DeactivateNode )
        CLIENT_OPERATION_ACE_INIT( DeactivateNodesBatch )
        CLIENT_OPERATION_ACE_INIT( RemoveNodeDeactivations )
        CLIENT_OPERATION_ACE_INIT( GetNodeDeactivationStatus )
        CLIENT_OPERATION_ACE_INIT( NodeStateRemoved )
        CLIENT_OPERATION_ACE_INIT( RecoverPartition )
        CLIENT_OPERATION_ACE_INIT( RecoverPartitions )
        CLIENT_OPERATION_ACE_INIT( RecoverServicePartitions )
        CLIENT_OPERATION_ACE_INIT( RecoverSystemPartitions )
        CLIENT_OPERATION_ACE_INIT( ResetPartitionLoad)
        CLIENT_OPERATION_ACE_INIT( ToggleVerboseServicePlacementHealthReporting)
        CLIENT_OPERATION_ACE_INIT( ReportFault )
        CLIENT_OPERATION_ACE_INIT( Query )
        CLIENT_OPERATION_ACE_INIT( Ping )
        CLIENT_OPERATION_ACE_INIT( CreateName )
        CLIENT_OPERATION_ACE_INIT( DeleteName )
        CLIENT_OPERATION_ACE_INIT( NameExists )
        CLIENT_OPERATION_ACE_INIT( EnumerateSubnames )
        CLIENT_OPERATION_ACE_INIT( EnumerateProperties )
        CLIENT_OPERATION_ACE_INIT( PropertyReadBatch )
        CLIENT_OPERATION_ACE_INIT( PropertyWriteBatch )
        CLIENT_OPERATION_ACE_INIT( ResolveService )
        CLIENT_OPERATION_ACE_INIT( ResolveNameOwner )
        CLIENT_OPERATION_ACE_INIT( ResolvePartition )
        CLIENT_OPERATION_ACE_INIT( NodeControl )
        CLIENT_OPERATION_ACE_INIT( CodePackageControl )
        CLIENT_OPERATION_ACE_INIT( UnreliableTransportControl )
        CLIENT_OPERATION_ACE_INIT( MoveReplicaControl)
        CLIENT_OPERATION_ACE_INIT( ProvisionApplicationType )
        CLIENT_OPERATION_ACE_INIT( CreateApplication )
        CLIENT_OPERATION_ACE_INIT( DeleteApplication )
        CLIENT_OPERATION_ACE_INIT( UpgradeApplication )
        CLIENT_OPERATION_ACE_INIT( RollbackApplicationUpgrade )
        CLIENT_OPERATION_ACE_INIT( UnprovisionApplicationType )
        CLIENT_OPERATION_ACE_INIT( GetUpgradeStatus )
        CLIENT_OPERATION_ACE_INIT( ReportHealth )
        CLIENT_OPERATION_ACE_INIT( ReportUpgradeHealth )
        CLIENT_OPERATION_ACE_INIT( MoveNextUpgradeDomain )
        CLIENT_OPERATION_ACE_INIT( ProvisionFabric )
        CLIENT_OPERATION_ACE_INIT( UpgradeFabric )
        CLIENT_OPERATION_ACE_INIT( RollbackFabricUpgrade )
        CLIENT_OPERATION_ACE_INIT( GetFabricUpgradeStatus )
        CLIENT_OPERATION_ACE_INIT( ReportFabricUpgradeHealth )
        CLIENT_OPERATION_ACE_INIT( MoveNextFabricUpgradeDomain )
        CLIENT_OPERATION_ACE_INIT( UnprovisionFabric )
        CLIENT_OPERATION_ACE_INIT( StartInfrastructureTask )
        CLIENT_OPERATION_ACE_INIT( FinishInfrastructureTask )

        CLIENT_OPERATION_ACE_INIT( InvokeInfrastructureCommand )
        CLIENT_OPERATION_ACE_INIT( InvokeInfrastructureQuery )

        CLIENT_OPERATION_ACE_INIT( ServiceNotifications )
        CLIENT_OPERATION_ACE_INIT( PrefixResolveService )
        CLIENT_OPERATION_ACE_INIT( ResolveSystemService )
        CLIENT_OPERATION_ACE_INIT(FileContent)
        CLIENT_OPERATION_ACE_INIT(FileDownload)

        CLIENT_OPERATION_ACE_INIT(GetStagingLocation)
        CLIENT_OPERATION_ACE_INIT(GetStoreLocation)
        CLIENT_OPERATION_ACE_INIT(InternalList)
        CLIENT_OPERATION_ACE_INIT(Upload)
        CLIENT_OPERATION_ACE_INIT(Delete)
        CLIENT_OPERATION_ACE_INIT(List)
        CLIENT_OPERATION_ACE_INIT( PredeployPackageToNode )

        CLIENT_OPERATION_ACE_INIT( StartPartitionDataLoss )
        CLIENT_OPERATION_ACE_INIT( StartPartitionQuorumLoss )
        CLIENT_OPERATION_ACE_INIT( StartPartitionRestart )
        CLIENT_OPERATION_ACE_INIT( GetPartitionDataLossProgress )
        CLIENT_OPERATION_ACE_INIT( GetPartitionQuorumLossProgress )
        CLIENT_OPERATION_ACE_INIT( GetPartitionRestartProgress )
        CLIENT_OPERATION_ACE_INIT( CancelTestCommand )

        CLIENT_OPERATION_ACE_INIT( StartChaos )
        CLIENT_OPERATION_ACE_INIT( StopChaos )
        CLIENT_OPERATION_ACE_INIT( GetChaosReport )

        CLIENT_OPERATION_ACE_INIT( StartNodeTransition )
        CLIENT_OPERATION_ACE_INIT( GetNodeTransitionProgress )

        CLIENT_OPERATION_ACE_INIT(StartClusterConfigurationUpgrade)
        CLIENT_OPERATION_ACE_INIT(GetClusterConfigurationUpgradeStatus)
        CLIENT_OPERATION_ACE_INIT(GetClusterConfiguration)
        CLIENT_OPERATION_ACE_INIT(GetUpgradesPendingApproval)
        CLIENT_OPERATION_ACE_INIT(StartApprovedUpgrades)
        CLIENT_OPERATION_ACE_INIT(GetUpgradeOrchestrationServiceState)
        CLIENT_OPERATION_ACE_INIT(SetUpgradeOrchestrationServiceState)
        CLIENT_OPERATION_ACE_INIT(CreateComposeDeployment)
        CLIENT_OPERATION_ACE_INIT(DeleteComposeDeployment)
        CLIENT_OPERATION_ACE_INIT(UpgradeComposeDeployment)
        CLIENT_OPERATION_ACE_INIT(InvokeContainerApi)
        CLIENT_OPERATION_ACE_INIT(CreateVolume)
        CLIENT_OPERATION_ACE_INIT(DeleteVolume)
        CLIENT_OPERATION_ACE_INIT(GetSecrets)
        CLIENT_OPERATION_ACE_INIT(CreateNetwork)
        CLIENT_OPERATION_ACE_INIT(DeleteNetwork)
        CLIENT_OPERATION_ACE_INIT(CreateGatewayResource)
        CLIENT_OPERATION_ACE_INIT(DeleteGatewayResource)
    }
}
