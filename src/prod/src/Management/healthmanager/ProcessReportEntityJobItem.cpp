// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace std;
using namespace Store;
using namespace Management::HealthManager;

StringLiteral const TraceComponent("ProcessReportEntityJobItem");

ProcessReportEntityJobItem::ProcessReportEntityJobItem(
    std::shared_ptr<HealthEntity> const & entity,
    ReportRequestContext && context)
    : EntityJobItem(entity)
    , contexts_()
    , isDeleteRequest_(context.Report.IsForDelete)
{
    contexts_.push_back(move(context));
}

ProcessReportEntityJobItem::~ProcessReportEntityJobItem()
{
}

Store::ReplicaActivityId const & ProcessReportEntityJobItem::get_ReplicaActivityId() const 
{
    // Reports can have different activity id, use the first one
    ASSERT_IF(contexts_.empty(), "ProcessReportEntityJobItem: at least one context must exist");
    return contexts_[0].ReplicaActivityId; 
}

ServiceModel::Priority::Enum ProcessReportEntityJobItem::get_JobPriority() const
{
    // All reports have the same priority
    ASSERT_IF(contexts_.empty(), "ProcessReportEntityJobItem: at least one context must exist");
    return contexts_[0].Report.ReportPriority;
}

void ProcessReportEntityJobItem::ProcessInternal(IHealthJobItemSPtr const & thisSPtr)
{
    this->Entity->StartProcessReport(thisSPtr, contexts_, isDeleteRequest_);
}

// NOTE: OnComplete moves the contexts, so the job item properties like 
// activityId can't be used after that.
void ProcessReportEntityJobItem::OnComplete(Common::ErrorCode const & error)
{
    for (auto && context : contexts_)
    {
        // Notify caller of result
        auto copyError = error;
        this->Entity->HealthManagerReplicaObj.QueueReportForCompletion(move(context), move(copyError));
    }

    contexts_.clear();
}

bool ProcessReportEntityJobItem::CanCombine(IHealthJobItemSPtr const & other) const
{
    if (isDeleteRequest_)
    {
        // Can't combine delete requests, skip other checks
        return false;
    }

    if (this->JobPriority != other->JobPriority)
    {
        return false;
    }

    if (contexts_.size() >= ManagementConfig::GetConfig().MaxEntityHealthReportsAllowedPerTransaction)
    {
        // Prevent one transaction from becoming too big.
        return false;
    }

    if (other->Type != this->Type)
    {
        // Only report job items can be batched
        return false;
    }

#ifdef DBG
    auto otherAsReport = dynamic_cast<ProcessReportEntityJobItem const*>(other.get());
#else
    auto otherAsReport = static_cast<ProcessReportEntityJobItem const*>(other.get());
#endif

    if (otherAsReport->isDeleteRequest_)
    {
        // Delete reports must be treated separately.
        return false;
    }

    auto instance1 = contexts_[0].Report.EntityInformation->EntityInstance;
    auto instance2 = otherAsReport->contexts_[0].Report.EntityInformation->EntityInstance;

#ifdef DBG
    // All merged reports in a job item should be for the same instance
    for (size_t i = 1; i < contexts_.size(); ++i)
    {
        ASSERT_IF(contexts_[i].Report.EntityInformation->EntityInstance != instance1, "{0}: all merged reports in a job item should have the same instance", *this);
    }

    for (size_t i = 1; i < otherAsReport->contexts_.size(); ++i)
    {
        ASSERT_IF(otherAsReport->contexts_[i].Report.EntityInformation->EntityInstance != instance2, "{0}: all merged reports in a job item should have the same instance", *otherAsReport);
    }
#endif

    // The report must be for the same entity instance. Otherwise, the report with higher instance
    // will delete the reports for previous instance and create a new entry.
    if (instance1 != instance2)
    {
        return false;
    }

    return true;
}

void ProcessReportEntityJobItem::Append(IHealthJobItemSPtr && other)
{
    ASSERT_IFNOT(other->Type == this->Type, "{0}: can't append {1}", *this, *other);

#ifdef DBG
    auto otherAsReport = dynamic_cast<ProcessReportEntityJobItem*>(other.get());
#else
    auto otherAsReport = static_cast<ProcessReportEntityJobItem*>(other.get());
#endif

    auto newContexts = otherAsReport->TakeContexts();
    ASSERT_IF(newContexts.empty(), "{0}: append job item: can't have empty contexts", *this);
    for (auto && context : newContexts)
    {
        contexts_.push_back(move(context));
    }

    HealthManagerReplica::WriteNoise(
        TraceComponent,
        this->JobId,
        "{0}: JI {1}: append JI {2} ({3}), total {4} contexts",
        this->ReplicaActivityId.TraceId,
        this->SequenceNumber,
        other->SequenceNumber,
        contexts_.back().ActivityId,
        contexts_.size());
}

void ProcessReportEntityJobItem::FinishAsyncWork()
{
    this->Entity->EndProcessReport(*this, this->PendingAsyncWork, isDeleteRequest_);
}
