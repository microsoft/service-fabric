// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

PlacementTask::PlacementTask(FailoverManager const& fm)
    : StateMachineTask(), fm_(fm)
{
}

// only for stateful services
void PlacementTask::CheckFailoverUnit(LockedFailoverUnitPtr & lockedFailoverUnit, vector<StateMachineActionUPtr> &)
{
    FailoverUnit & failoverUnit = *lockedFailoverUnit;
    ASSERT_IFNOT(failoverUnit.IsStateful, "Placement task should only process stateful FailoverUnit");

    // During reconfiguration don't create any new replica.
    if (failoverUnit.IsChangingConfiguration || 
        failoverUnit.IsToBeDeleted ||
        (failoverUnit.CurrentConfiguration.AvailableCount != failoverUnit.CurrentConfiguration.ReplicaCount &&
         failoverUnit.CurrentConfiguration.ReplicaCount > static_cast<size_t>(failoverUnit.MinReplicaSetSize)))
    {
        return;
    }

    int count = 0;
    if (failoverUnit.CurrentConfiguration.IsEmpty)
    {
        if (failoverUnit.ReplicaCount > 0 || fm_.NodeCacheObj.IsClusterStable)
        {
            if (failoverUnit.OfflineReplicaCount == 0 ||
                DateTime::Now() - failoverUnit.LastUpdated >= FailoverConfig::GetConfig().RecoverOnDataLossWaitDuration)
            {
                count = failoverUnit.GetInitialReplicaPlacementCount();
            }
        }
    }
    else
    {
        if (!failoverUnit.CurrentConfiguration.IsPrimaryAvailable)
        {
            return;
        }

        int existingCount = 0;
        int downWaitingCount = 0;
        int inBuildCount = 0;
        int movingCount = 0;

        for (auto it = failoverUnit.BeginIterator; it != failoverUnit.EndIterator; ++it)
        {
            if (!it->IsToBeDropped)
            {
                if ((it->IsPreferredPrimaryLocation && !it->IsPrimaryToBePlaced) ||
                    (it->IsPreferredReplicaLocation && !it->IsReplicaToBePlaced))
                {
                    existingCount++;
                }
                else if (it->IsUp)
                {
                    if (it->IsMoveInProgress)
                    {
                        movingCount++;
                    }
                    else if (it->IsInBuild)
                    {
                        inBuildCount++;
                    }
                    else if (it->IsInConfiguration || !it->IsStandBy)
                    {
                        existingCount++;
                    }
                }
                else if (!it->IsDropped)
                {
                    if (!it->NodeInfoObj->DeactivationInfo.IsRemove)
                    {
                        TimeSpan replicaRestartWaitDuration = failoverUnit.ServiceInfoObj->ServiceDescription.ReplicaRestartWaitDuration;

                        if (lockedFailoverUnit->ProcessingStartTime.ToDateTime() - it->LastDownTime <  replicaRestartWaitDuration)
                        {
                            downWaitingCount++;
                        }
                    }
                }
            }
        }

        int targetReplicaSetSize = failoverUnit.TargetReplicaSetSize;
        if (failoverUnit.TargetReplicaSetSize == 1 && 
            FailoverConfig::GetConfig().IsSingletonReplicaMoveAllowedDuringUpgrade &&
            (failoverUnit.PreferredPrimaryLocationExists || failoverUnit.Primary->IsPrimaryToBeSwappedOut))
        {
            targetReplicaSetSize++;
        }

        count = targetReplicaSetSize - existingCount - max(inBuildCount, movingCount);

        if (count > 0)
        {
            count = max(count - downWaitingCount, 0);
        }
    }

    failoverUnit.ReplicaDifference = count;
}

