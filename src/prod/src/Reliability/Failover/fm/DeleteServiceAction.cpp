// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;
using namespace std;

DeleteServiceAction::DeleteServiceAction(ServiceInfoSPtr const& serviceInfo, FailoverUnitId const& failoverUnitId)
    : serviceInfo_(serviceInfo)
    , failoverUnitId_(failoverUnitId)
{
}

int DeleteServiceAction::OnPerformAction(FailoverManager & fm)
{
    if (serviceInfo_->FailoverUnitIds.size() != serviceInfo_->ServiceDescription.PartitionCount && !serviceInfo_->IsForceDelete)
    {
        return 0;
    }

    bool shouldMarkServiceAsDeleted = true;
    for (auto const& failoverUnitId : serviceInfo_->FailoverUnitIds)
    {
        if (failoverUnitId != failoverUnitId_)
        {
            // TODO: Do not lock the FailoverUnit
            LockedFailoverUnitPtr failoverUnit;
            fm.FailoverUnitCacheObj.TryGetLockedFailoverUnit(failoverUnitId, failoverUnit, TimeSpan::Zero);

            if (!failoverUnit || !failoverUnit->IsOrphaned)
            {
                shouldMarkServiceAsDeleted = false;
                break;
            }
        }
    }

    if (shouldMarkServiceAsDeleted)
    {
        MarkServiceAsDeletedCommitJobItemUPtr jobItem = make_unique<MarkServiceAsDeletedCommitJobItem>(serviceInfo_);
        fm.CommitQueue.Enqueue(move(jobItem));
    }

    return 0;
}

void DeleteServiceAction::WriteTo(TextWriter& w, FormatOptions const&) const
{
    w.Write("DeleteService");
}

void DeleteServiceAction::WriteToEtw(uint16 contextSequenceId) const
{
    FailoverManager::FMEventSource->StateMachineAction(contextSequenceId, L"DeleteService");
}
