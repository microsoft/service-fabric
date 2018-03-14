// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

FailoverUnitJob::FailoverUnitJob(FailoverUnitId const & failoverUnitId, Federation::NodeInstance from, bool isFromPLB)
    : failoverUnitId_(failoverUnitId), isFromPLB_(isFromPLB), from_(from)
{
}

bool FailoverUnitJob::NeedThrottle(FailoverManager & fm)
{
    return fm.Store.IsThrottleNeeded;
}

bool FailoverUnitJob::ProcessJob(FailoverManager & fm)
{
    bool result;

    LockedFailoverUnitPtr failoverUnit;
    fm.FailoverUnitCacheObj.TryGetLockedFailoverUnit(failoverUnitId_, failoverUnit, TimeSpan::Zero, true);
    if (failoverUnit)
    {
        result = fm.BackgroundManagerObj.ProcessFailoverUnit(move(failoverUnit), false);
    }
    else
    {
        result = true;
    }

    return result;
}

void FailoverUnitJob::OnQueueFull(FailoverManager & fm, size_t actualQueueSize)
{
    if (fm.QueueFullThrottle.IsThrottling())
    {
        fm.Events.OnQueueFull(QueueName::FailoverUnitQueue, actualQueueSize);

        if (!isFromPLB_)
        {
            Transport::MessageUPtr reply = RSMessage::GetServiceBusy().CreateMessage();
            GenerationHeader header(fm.Generation, fm.IsMaster);
            reply->Headers.Add(header);
            fm.SendToNodeAsync(move(reply), from_);
        }
        else
        {
            fm.PLB.OnFMBusy();
        }
    }
}

FailoverUnitJobQueue::FailoverUnitJobQueue(FailoverManager & fm)
    : JobQueue(
      L"FMFailoverUnit." + fm.Id,
      fm, // Root
      false, // ForceEnqueue
      FailoverConfig::GetConfig().ProcessingQueueThreadCount,
      JobQueuePerfCounters::CreateInstance(L"FMFailoverUnit", fm.Id),
      FailoverConfig::GetConfig().ProcessingQueueSize)
{
}
