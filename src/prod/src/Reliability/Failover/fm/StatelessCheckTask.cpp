// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

StatelessCheckTask::StatelessCheckTask(FailoverManager const& fm, INodeCache const& nodeCache)
	: StateMachineTask(), fm_(fm), nodeCache_(nodeCache)
{
}

void StatelessCheckTask::CheckFailoverUnit(LockedFailoverUnitPtr & lockedFailoverUnit, vector<StateMachineActionUPtr> & actions)
{
    UNREFERENCED_PARAMETER(actions);

    FailoverUnit & failoverUnit = *lockedFailoverUnit;

    // ensure that all read operations will call the read only operators in FailoverUnitUPtrAccessor
    if (failoverUnit.IsToBeDeleted)
    {
        return;
    }

    if (failoverUnit.TargetReplicaSetSize == -1)
    {
        return;
    }

    int existingCount = 0;
    int inBuildCount = 0;
    int movingCount = 0;

    for (auto it = failoverUnit.BeginIterator; it != failoverUnit.EndIterator; ++it)
    {
        if (it->IsPreferredReplicaLocation && !it->IsReplicaToBePlaced)
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
            else if (!it->IsToBeDropped)
            {
                existingCount++;
            }
        }
    }

    existingCount += max(inBuildCount, movingCount);
    int desiredCount = failoverUnit.TargetReplicaSetSize;

    if (existingCount == desiredCount ||
		(failoverUnit.UpReplicaCount == 0 && !fm_.NodeCacheObj.IsClusterStable))
    {
        return;
    }

    failoverUnit.ReplicaDifference = desiredCount - existingCount;
}
