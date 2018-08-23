// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
    
ProxyActionsList::ConcurrentExecutionCompatibilityLookupTable::ConcurrentExecutionCompatibilityLookupTable()
{
    // Disallow all combinations
    ::memset(&lookupTable_, (int)false, sizeof(lookupTable_));

    // Mark those combinations that are allowed as such
    for(int i = 0; i < ProxyActionsListTypes::ProxyActionsListTypesCount; i++)
    {
        // Empty is compatible with everything
        lookupTable_[ProxyActionsListTypes::Empty][i] = true;
        lookupTable_[i][ProxyActionsListTypes::Empty] = true;

        // Abort is compatible with any running action except for Abort
        if (i != ProxyActionsListTypes::StatefulServiceAbort)
        {
            lookupTable_[i][ProxyActionsListTypes::StatefulServiceAbort] = true;
        }
        
        if (i != ProxyActionsListTypes::StatelessServiceAbort)
        {
            lookupTable_[i][ProxyActionsListTypes::StatelessServiceAbort] = true;
        }

        // Close is compatible with any running Action except for Close or Abort
        if (i != ProxyActionsListTypes::StatefulServiceClose &&
            i != ProxyActionsListTypes::StatefulServiceDrop &&
            i != ProxyActionsListTypes::StatefulServiceAbort)
        {
            lookupTable_[i][ProxyActionsListTypes::StatefulServiceClose] = true;
            lookupTable_[i][ProxyActionsListTypes::StatefulServiceDrop] = true;
        }

        if (i != ProxyActionsListTypes::StatelessServiceClose && i != ProxyActionsListTypes::StatelessServiceAbort)
        {
            lookupTable_[i][ProxyActionsListTypes::StatelessServiceClose] = true;
        }

        // Reopen is compatible with any running Action except for Reopen, Close or Abort
        if (i != ProxyActionsListTypes::StatefulServiceReopen && 
            i != ProxyActionsListTypes::StatefulServiceClose &&
            i != ProxyActionsListTypes::StatefulServiceDrop && 
            i != ProxyActionsListTypes::StatefulServiceAbort)
        {
            lookupTable_[i][ProxyActionsListTypes::StatefulServiceReopen] = true;
        }
    }

    MakeCompatibleWithEverythingExceptLifecycleAndSelf(ProxyActionsListTypes::ReplicatorGetQuery);
    MakeCompatibleWithEverythingExceptLifecycleAndSelf(ProxyActionsListTypes::UpdateServiceDescription);

    lookupTable_[ProxyActionsListTypes::ReplicatorBuildIdleReplica][ProxyActionsListTypes::ReplicatorBuildIdleReplica] = true;
    lookupTable_[ProxyActionsListTypes::ReplicatorBuildIdleReplica][ProxyActionsListTypes::ReplicatorRemoveIdleReplica] = true;
    lookupTable_[ProxyActionsListTypes::ReplicatorBuildIdleReplica][ProxyActionsListTypes::ReplicatorUpdateAndCatchupQuorum] = true;
    lookupTable_[ProxyActionsListTypes::ReplicatorBuildIdleReplica][ProxyActionsListTypes::ReplicatorUpdateReplicas] = true;
    lookupTable_[ProxyActionsListTypes::ReplicatorBuildIdleReplica][ProxyActionsListTypes::StatefulServiceDemoteToSecondary] = true;
    lookupTable_[ProxyActionsListTypes::ReplicatorBuildIdleReplica][ProxyActionsListTypes::StatefulServiceFinishDemoteToSecondary] = true;
    lookupTable_[ProxyActionsListTypes::ReplicatorBuildIdleReplica][ProxyActionsListTypes::StatefulServiceEndReconfiguration] = true;
    lookupTable_[ProxyActionsListTypes::ReplicatorBuildIdleReplica][ProxyActionsListTypes::StatefulServicePromoteToPrimary] = true;
    lookupTable_[ProxyActionsListTypes::ReplicatorBuildIdleReplica][ProxyActionsListTypes::CancelCatchupReplicaSet] = true;

    lookupTable_[ProxyActionsListTypes::ReplicatorRemoveIdleReplica][ProxyActionsListTypes::ReplicatorBuildIdleReplica] = true;
    lookupTable_[ProxyActionsListTypes::ReplicatorRemoveIdleReplica][ProxyActionsListTypes::ReplicatorRemoveIdleReplica] = true;
    lookupTable_[ProxyActionsListTypes::ReplicatorRemoveIdleReplica][ProxyActionsListTypes::ReplicatorUpdateAndCatchupQuorum] = true;
    lookupTable_[ProxyActionsListTypes::ReplicatorRemoveIdleReplica][ProxyActionsListTypes::ReplicatorUpdateReplicas] = true;
    lookupTable_[ProxyActionsListTypes::ReplicatorRemoveIdleReplica][ProxyActionsListTypes::StatefulServiceDemoteToSecondary] = true;
    lookupTable_[ProxyActionsListTypes::ReplicatorRemoveIdleReplica][ProxyActionsListTypes::StatefulServiceFinishDemoteToSecondary] = true;
    lookupTable_[ProxyActionsListTypes::ReplicatorRemoveIdleReplica][ProxyActionsListTypes::StatefulServiceEndReconfiguration] = true;

    lookupTable_[ProxyActionsListTypes::ReplicatorUpdateAndCatchupQuorum][ProxyActionsListTypes::ReplicatorBuildIdleReplica] = true;
    lookupTable_[ProxyActionsListTypes::ReplicatorUpdateAndCatchupQuorum][ProxyActionsListTypes::ReplicatorRemoveIdleReplica] = true;
    lookupTable_[ProxyActionsListTypes::ReplicatorUpdateAndCatchupQuorum][ProxyActionsListTypes::ReplicatorUpdateReplicas] = true;
    lookupTable_[ProxyActionsListTypes::ReplicatorUpdateAndCatchupQuorum][ProxyActionsListTypes::StatefulServiceDemoteToSecondary] = true;
    lookupTable_[ProxyActionsListTypes::ReplicatorUpdateAndCatchupQuorum][ProxyActionsListTypes::StatefulServiceFinishDemoteToSecondary] = true;
    lookupTable_[ProxyActionsListTypes::ReplicatorUpdateAndCatchupQuorum][ProxyActionsListTypes::StatefulServiceEndReconfiguration] = true;

    lookupTable_[ProxyActionsListTypes::ReplicatorUpdateReplicas][ProxyActionsListTypes::ReplicatorBuildIdleReplica] = true;
    lookupTable_[ProxyActionsListTypes::ReplicatorUpdateReplicas][ProxyActionsListTypes::ReplicatorRemoveIdleReplica] = true;
    lookupTable_[ProxyActionsListTypes::ReplicatorUpdateReplicas][ProxyActionsListTypes::ReplicatorUpdateAndCatchupQuorum] = true;
    lookupTable_[ProxyActionsListTypes::ReplicatorUpdateReplicas][ProxyActionsListTypes::ReplicatorUpdateReplicas] = true;
    lookupTable_[ProxyActionsListTypes::ReplicatorUpdateReplicas][ProxyActionsListTypes::StatefulServiceDemoteToSecondary] = true;
    lookupTable_[ProxyActionsListTypes::ReplicatorUpdateReplicas][ProxyActionsListTypes::StatefulServiceFinishDemoteToSecondary] = true;
    lookupTable_[ProxyActionsListTypes::ReplicatorUpdateReplicas][ProxyActionsListTypes::StatefulServiceEndReconfiguration] = true;

    lookupTable_[ProxyActionsListTypes::StatefulServicePromoteToPrimary][ProxyActionsListTypes::ReplicatorRemoveIdleReplica] = true;
    lookupTable_[ProxyActionsListTypes::StatefulServicePromoteToPrimary][ProxyActionsListTypes::ReplicatorBuildIdleReplica] = true;
    lookupTable_[ProxyActionsListTypes::StatefulServicePromoteToPrimary][ProxyActionsListTypes::ReplicatorUpdateReplicas] = true;

    lookupTable_[ProxyActionsListTypes::StatefulServiceDemoteToSecondary][ProxyActionsListTypes::CancelCatchupReplicaSet] = true;
    lookupTable_[ProxyActionsListTypes::StatefulServiceDemoteToSecondary][ProxyActionsListTypes::ReplicatorRemoveIdleReplica] = true;
    lookupTable_[ProxyActionsListTypes::StatefulServiceDemoteToSecondary][ProxyActionsListTypes::ReplicatorBuildIdleReplica] = true;
    lookupTable_[ProxyActionsListTypes::StatefulServiceDemoteToSecondary][ProxyActionsListTypes::ReplicatorUpdateReplicas] = true;
    
    lookupTable_[ProxyActionsListTypes::StatefulServiceFinishDemoteToSecondary][ProxyActionsListTypes::CancelCatchupReplicaSet] = true;
    lookupTable_[ProxyActionsListTypes::StatefulServiceFinishDemoteToSecondary][ProxyActionsListTypes::ReplicatorRemoveIdleReplica] = true;
    lookupTable_[ProxyActionsListTypes::StatefulServiceFinishDemoteToSecondary][ProxyActionsListTypes::ReplicatorBuildIdleReplica] = true;
    lookupTable_[ProxyActionsListTypes::StatefulServiceFinishDemoteToSecondary][ProxyActionsListTypes::ReplicatorUpdateReplicas] = true;
}

bool ProxyActionsList::ConcurrentExecutionCompatibilityLookupTable::CanExecuteConcurrently(ProxyActionsListTypes::Enum const runningAction, ProxyActionsListTypes::Enum const actionToRun) const
{
    return lookupTable_[runningAction][actionToRun];
}

void ProxyActionsList::ConcurrentExecutionCompatibilityLookupTable::MakeCompatibleWithEverythingExceptLifecycleAndSelf(ProxyActionsListTypes::Enum action)
{
    for (int i = 0; i < ProxyActionsListTypes::ProxyActionsListTypesCount; i++)
    {
        // Do not allow this action to execute if a lifecycle action is executing
        if (i != ProxyActionsListTypes::StatefulServiceClose &&
            i != ProxyActionsListTypes::StatefulServiceDrop &&
            i != ProxyActionsListTypes::StatefulServiceAbort &&
            i != ProxyActionsListTypes::StatefulServiceReopen)
        {
            lookupTable_[i][action] = true;
        }

        // Allow all other actions to execute while this action is running except for this action in parallel
        lookupTable_[action][i] = true;
    }

    // Do not allow parallel invocations of the same action
    lookupTable_[action][action] = false;
}
