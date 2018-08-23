// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;
using namespace Reliability::LoadBalancingComponent;


AutoScalingTask::AutoScalingTask(int targetReplicaSetSize) 
    :targetReplicaSetSize_(targetReplicaSetSize)
{
}

void AutoScalingTask::CheckFailoverUnit(LockedFailoverUnitPtr & failoverUnit, vector<StateMachineActionUPtr> & actions)
{
    if (failoverUnit->IsToBeDeleted)
    {
        return;
    }

    failoverUnit.EnableUpdate();

    failoverUnit->TargetReplicaSetSize = targetReplicaSetSize_;

    actions.push_back(make_unique<TraceAutoScaleAction>(targetReplicaSetSize_));
}
