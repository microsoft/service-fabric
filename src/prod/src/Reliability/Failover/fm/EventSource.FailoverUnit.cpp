// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace FailoverManagerComponent;

namespace
{
    bool HasDetailedTraceIntervalExpired(LockedFailoverUnitPtr & failoverUnit)
    {
        return DateTime::Now() - failoverUnit.Old->LastUpdated >= FailoverConfig::GetConfig().FTDetailedTraceInterval;
    }

    bool HasDcaTraceIntervalExpired(FailoverUnit const& failoverUnit)
    {
        return DateTime::Now() - failoverUnit.LastUpdated >= FailoverConfig::GetConfig().DcaTraceInterval;
    }
}

void FTEventSource::TraceFTUpdateBackground(
    LockedFailoverUnitPtr & failoverUnit,
    std::vector<StateMachineActionUPtr> const & actions,
    int replicaDifference, 
    int64 commitDuration,
    int64 plbDuration) const
{
    if (HasDetailedTraceIntervalExpired(failoverUnit))
    {
        FTUpdateBackgroundWithOld(failoverUnit->Id.Guid, *(failoverUnit.Current), *(failoverUnit.Old), actions, replicaDifference, commitDuration, plbDuration);
    }
    else if (failoverUnit->IsPersistencePending)
    {
        FTUpdateBackgroundNoise(failoverUnit->Id.Guid, *(failoverUnit.Current), actions, replicaDifference, commitDuration, plbDuration);
    }
    else
    {
        FTUpdateBackground(failoverUnit->Id.Guid, *(failoverUnit.Current), actions, replicaDifference, commitDuration, plbDuration);
    }
}

void FTEventSource::TraceFTUpdateNodeStateRemoved(LockedFailoverUnitPtr & failoverUnit) const
{
    if (HasDetailedTraceIntervalExpired(failoverUnit))
    {
        FTUpdateNodeStateRemovedWithOld(failoverUnit->Id.Guid, *(failoverUnit.Current), *(failoverUnit.Old));
    }
    else
    {
        FTUpdateNodeStateRemoved(failoverUnit->Id.Guid, *(failoverUnit.Current));
    }
}

void FTEventSource::TraceFTUpdateRecover(LockedFailoverUnitPtr & failoverUnit) const
{
    if (HasDetailedTraceIntervalExpired(failoverUnit))
    {
        FTUpdateRecoverWithOld(failoverUnit->Id.Guid, *(failoverUnit.Current), *(failoverUnit.Old));
    }
    else
    {
        FTUpdateRecover(failoverUnit->Id.Guid, *(failoverUnit.Current));
    }
}

void FTEventSource::TraceFTUpdateReplicaDown(LockedFailoverUnitPtr & failoverUnit) const
{
    if (HasDetailedTraceIntervalExpired(failoverUnit))
    {
        FTUpdateReplicaDownWithOld(failoverUnit->Id.Guid, *(failoverUnit.Current), *(failoverUnit.Old));
    }
    else
    {
        FTUpdateReplicaDown(failoverUnit->Id.Guid, *(failoverUnit.Current));
    }
}

void FTEventSource::TraceFTAction(FailoverUnit const& failoverUnit, vector<StateMachineActionUPtr> const& actions, int32 replicaDifference) const
{
    if (HasDcaTraceIntervalExpired(failoverUnit))
    {
        FTActionDca(failoverUnit.Id.Guid, failoverUnit, actions, replicaDifference);
    }
    else
    {
        FTAction(failoverUnit.Id.Guid, failoverUnit, actions, replicaDifference);
    }
}
