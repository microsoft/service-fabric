// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace ServiceModel;
using namespace Management::RepairManager;

StringLiteral const TraceComponent("RepairTaskHistory");

RepairTaskHistory::RepairTaskHistory()
    : createdUtcTimestamp_()
    , claimedUtcTimestamp_()
    , preparingUtcTimestamp_()
    , approvedUtcTimestamp_()
    , executingUtcTimestamp_()
    , restoringUtcTimestamp_()
    , completedUtcTimestamp_()
    , preparingHealthCheckStartUtcTimestamp_()
    , preparingHealthCheckEndUtcTimestamp_()
    , restoringHealthCheckStartUtcTimestamp_()
    , restoringHealthCheckEndUtcTimestamp_()    
{
}

RepairTaskHistory::RepairTaskHistory(RepairTaskHistory && other)
    : createdUtcTimestamp_(move(other.createdUtcTimestamp_))
    , claimedUtcTimestamp_(move(other.claimedUtcTimestamp_))
    , preparingUtcTimestamp_(move(other.preparingUtcTimestamp_))
    , approvedUtcTimestamp_(move(other.approvedUtcTimestamp_))
    , executingUtcTimestamp_(move(other.executingUtcTimestamp_))
    , restoringUtcTimestamp_(move(other.restoringUtcTimestamp_))
    , completedUtcTimestamp_(move(other.completedUtcTimestamp_))
    , preparingHealthCheckStartUtcTimestamp_(move(other.preparingHealthCheckStartUtcTimestamp_))
    , preparingHealthCheckEndUtcTimestamp_(move(other.preparingHealthCheckEndUtcTimestamp_))
    , restoringHealthCheckStartUtcTimestamp_(move(other.restoringHealthCheckStartUtcTimestamp_))
    , restoringHealthCheckEndUtcTimestamp_(move(other.restoringHealthCheckEndUtcTimestamp_))    
{
}

ErrorCode RepairTaskHistory::FromPublicApi(FABRIC_REPAIR_TASK_HISTORY const & publicHistory)
{
    createdUtcTimestamp_ = DateTime(publicHistory.CreatedUtcTimestamp);
    claimedUtcTimestamp_ = DateTime(publicHistory.ClaimedUtcTimestamp);
    preparingUtcTimestamp_ = DateTime(publicHistory.PreparingUtcTimestamp);
    approvedUtcTimestamp_ = DateTime(publicHistory.ApprovedUtcTimestamp);
    executingUtcTimestamp_ = DateTime(publicHistory.ExecutingUtcTimestamp);
    restoringUtcTimestamp_ = DateTime(publicHistory.RestoringUtcTimestamp);
    completedUtcTimestamp_ = DateTime(publicHistory.CompletedUtcTimestamp);

    if (publicHistory.Reserved != NULL)
    {
        auto ex1 = static_cast<FABRIC_REPAIR_TASK_HISTORY_EX1*>(publicHistory.Reserved);

        preparingHealthCheckStartUtcTimestamp_ = DateTime(ex1->PreparingHealthCheckStartUtcTimestamp);
        preparingHealthCheckEndUtcTimestamp_ = DateTime(ex1->PreparingHealthCheckEndUtcTimestamp);
        restoringHealthCheckStartUtcTimestamp_ = DateTime(ex1->RestoringHealthCheckStartUtcTimestamp);
        restoringHealthCheckEndUtcTimestamp_ = DateTime(ex1->RestoringHealthCheckEndUtcTimestamp);
    }

    return ErrorCode::Success();
}

void RepairTaskHistory::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_REPAIR_TASK_HISTORY & result) const
{
    UNREFERENCED_PARAMETER(heap);

    result.CreatedUtcTimestamp = createdUtcTimestamp_.AsFileTime;
    result.ClaimedUtcTimestamp = claimedUtcTimestamp_.AsFileTime;
    result.PreparingUtcTimestamp = preparingUtcTimestamp_.AsFileTime;
    result.ApprovedUtcTimestamp = approvedUtcTimestamp_.AsFileTime;
    result.ExecutingUtcTimestamp = executingUtcTimestamp_.AsFileTime;
    result.RestoringUtcTimestamp = restoringUtcTimestamp_.AsFileTime;
    result.CompletedUtcTimestamp = completedUtcTimestamp_.AsFileTime;

    ReferencePointer<FABRIC_REPAIR_TASK_HISTORY_EX1> ex1 = heap.AddItem<FABRIC_REPAIR_TASK_HISTORY_EX1>();
    ex1->PreparingHealthCheckStartUtcTimestamp = preparingHealthCheckStartUtcTimestamp_.AsFileTime;
    ex1->PreparingHealthCheckEndUtcTimestamp = preparingHealthCheckEndUtcTimestamp_.AsFileTime;
    ex1->RestoringHealthCheckStartUtcTimestamp = restoringHealthCheckStartUtcTimestamp_.AsFileTime;
    ex1->RestoringHealthCheckEndUtcTimestamp = restoringHealthCheckEndUtcTimestamp_.AsFileTime;

    result.Reserved = ex1.GetRawPointer();    
}

void RepairTaskHistory::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write("[created={0}, claimed={1}, preparing={2}, preparing-health-check-start={3}, preparing-health-check-end={4}, approved={5}, executing={6}, restoring={7}, restoring-health-check-start={8}, restoring-health-check-end={9}, completed={10}]",
        createdUtcTimestamp_,
        claimedUtcTimestamp_,
        preparingUtcTimestamp_,
        preparingHealthCheckStartUtcTimestamp_,
        preparingHealthCheckEndUtcTimestamp_,        
        approvedUtcTimestamp_,
        executingUtcTimestamp_,
        restoringUtcTimestamp_,
        restoringHealthCheckStartUtcTimestamp_,
        restoringHealthCheckEndUtcTimestamp_,        
        completedUtcTimestamp_);
}

void RepairTaskHistory::Initialize(RepairTaskState::Enum const & initialState)
{
    createdUtcTimestamp_ = DateTime();
    claimedUtcTimestamp_ = DateTime();
    preparingUtcTimestamp_ = DateTime();
    approvedUtcTimestamp_ = DateTime();
    executingUtcTimestamp_ = DateTime();
    restoringUtcTimestamp_ = DateTime();
    completedUtcTimestamp_ = DateTime();
    preparingHealthCheckStartUtcTimestamp_ = DateTime();
    preparingHealthCheckEndUtcTimestamp_ = DateTime();
    restoringHealthCheckStartUtcTimestamp_ = DateTime();
    restoringHealthCheckEndUtcTimestamp_ = DateTime();    

    RepairTaskState::Enum stateMask = RepairTaskState::Invalid;

    // Intentional fall-throughs for initializing in later states
    switch (initialState)
    {
    case RepairTaskState::Approved:
        stateMask |= RepairTaskState::Approved;
    case RepairTaskState::Preparing:
        stateMask |= RepairTaskState::Preparing;
    case RepairTaskState::Claimed:
        stateMask |= RepairTaskState::Claimed;
    case RepairTaskState::Created:
        stateMask |= RepairTaskState::Created;
    }

    SetTimestamps(stateMask);
}

void RepairTaskHistory::SetTimestamps(RepairTaskState::Enum const & stateMask)
{
    auto now = DateTime::Now();

    TrySetTimestamp(stateMask, RepairTaskState::Created, createdUtcTimestamp_, now);
    TrySetTimestamp(stateMask, RepairTaskState::Claimed, claimedUtcTimestamp_, now);
    TrySetTimestamp(stateMask, RepairTaskState::Preparing, preparingUtcTimestamp_, now);
    TrySetTimestamp(stateMask, RepairTaskState::Approved, approvedUtcTimestamp_, now);
    TrySetTimestamp(stateMask, RepairTaskState::Executing, executingUtcTimestamp_, now);
    TrySetTimestamp(stateMask, RepairTaskState::Restoring, restoringUtcTimestamp_, now);
    TrySetTimestamp(stateMask, RepairTaskState::Completed, completedUtcTimestamp_, now);
}

bool RepairTaskHistory::TrySetTimestamp(
    RepairTaskState::Enum const & stateMask,
    RepairTaskState::Enum const & matchState,
    DateTime & target,
    DateTime const & value)
{
    bool doUpdate =
        (stateMask & matchState) &&
        (target == DateTime::Zero);

    if (doUpdate)
    {
        target = value;
    }

    return doUpdate;
}

void RepairTaskHistory::SetPreparingHealthCheckStartTimestamp(DateTime const & value)
{
    TrySetHealthCheckTimestamp(preparingHealthCheckStartUtcTimestamp_, value);
}

void RepairTaskHistory::SetPreparingHealthCheckEndTimestamp(DateTime const & value)
{
    TrySetHealthCheckTimestamp(preparingHealthCheckEndUtcTimestamp_, value);
}

void RepairTaskHistory::SetRestoringHealthCheckStartTimestamp(DateTime const & value)
{
    TrySetHealthCheckTimestamp(restoringHealthCheckStartUtcTimestamp_, value);
}

void RepairTaskHistory::SetRestoringHealthCheckEndTimestamp(DateTime const & value)
{
    TrySetHealthCheckTimestamp(restoringHealthCheckEndUtcTimestamp_, value);
}

bool RepairTaskHistory::TrySetHealthCheckTimestamp(DateTime & target, DateTime const & value)
{
    bool doUpdate = (target == DateTime::Zero);

    if (doUpdate)
    {
        target = value;
    }

    return doUpdate;
}
