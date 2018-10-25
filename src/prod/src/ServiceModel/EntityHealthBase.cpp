// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Management::HealthManager;

StringLiteral const TraceSource("EntityHealthBase");

// Do not initialize serialization overhead, as this is a virtual class;
// the initialization should be done in all derived classes.
INITIALIZE_BASE_DYNAMIC_SIZE_ESTIMATION(EntityHealthBase)

EntityHealthBase::EntityHealthBase()
    : events_()
    , aggregatedHealthState_(FABRIC_HEALTH_STATE_INVALID)
    , unhealthyEvaluations_()
    , healthStats_()
{
}

EntityHealthBase::EntityHealthBase(
    std::vector<HealthEvent> && events,
    FABRIC_HEALTH_STATE aggregatedHealthState,
    std::vector<HealthEvaluation> && unhealthyEvaluations,
    HealthStatisticsUPtr && healthStats)
    : events_(move(events))
    , aggregatedHealthState_(aggregatedHealthState)
    , unhealthyEvaluations_(move(unhealthyEvaluations))
    , healthStats_(move(healthStats))
{
}

EntityHealthBase::~EntityHealthBase()
{
}

Common::ErrorCode EntityHealthBase::UpdateUnhealthyEvalautionsPerMaxAllowedSize(size_t maxAllowedSize)
{
    // Take the unhealthy evaluations and check the size.
    auto unhealthyEvals = move(unhealthyEvaluations_);

    // Estimate size without unhealthy evaluations (if any originally)
    size_t estimatedSize = EstimateSize();

    if (unhealthyEvals.empty())
    {
        // If the object doesn't fit the max allowed size, return error.
        if (estimatedSize >= maxAllowedSize)
        {
            return ErrorCode(
                ErrorCodeValue::EntryTooLarge,
                wformatString(HMResource::GetResources().EntityTooLargeNoUnhealthyEvaluations, estimatedSize, maxAllowedSize));
        }

        return ErrorCode::Success();
    }

    // Try to add the unhealthy evaluations with generated description and trimmed if needed
    auto error = HealthEvaluation::GenerateDescriptionAndTrimIfNeeded(
        unhealthyEvals,
        maxAllowedSize,
        estimatedSize);
    
    if (!error.IsSuccess())
    {
        // Only consider error if none of the unhealthy evaluations could be added, not even trimmed
        return ErrorCode(
            ErrorCodeValue::EntryTooLarge,
            wformatString(HMResource::GetResources().EntityTooLargeWithUnhealthyEvaluations, maxAllowedSize, estimatedSize));
    }

    // Populate the unhealthy evaluations
    unhealthyEvaluations_ = move(unhealthyEvals);
    return ErrorCode::Success();
}

std::wstring EntityHealthBase::GetStatsString() const
{
    if (!healthStats_)
    {
        return *Constants::EmptyString;
    }

    wstring description;
    StringWriter writer(description);
    for (auto const & entry : healthStats_->HealthCounts)
    {
        if (!entry.StateCount.IsEmpty())
        {
            writer.Write("{0}: {1}; ", entry.EntityKind, entry.StateCount);
        }
    }

    return description;
}

void EntityHealthBase::WriteTo(__in Common::TextWriter& writer, Common::FormatOptions const &) const
{
    if (unhealthyEvaluations_.empty())
    {
        writer.Write(
            "{0}, {1} events.",
            aggregatedHealthState_,
            events_.size());
    }
    else
    {
        writer.Write(
            "{0}, {1} events. UnhealthyEvaluations: {2}",
            aggregatedHealthState_,
            events_.size(),
            *unhealthyEvaluations_[0].Evaluation);
    }
}

// Called by the entities that have no children, so stats are null.
void EntityHealthBase::WriteToEtw(uint16 contextSequenceId) const
{
    if (unhealthyEvaluations_.empty())
    {
        ServiceModel::ServiceModelEventSource::Trace->EntityHealthBaseTrace(
            contextSequenceId,
            wformatString(aggregatedHealthState_),
            static_cast<uint64>(events_.size()));
    }
    else
    {
        ServiceModel::ServiceModelEventSource::Trace->EntityHealthBaseWithEvaluationsTrace(
            contextSequenceId,
            wformatString(aggregatedHealthState_),
            static_cast<uint64>(events_.size()),
            *unhealthyEvaluations_[0].Evaluation);
    }
}

std::wstring EntityHealthBase::ToString() const
{
    return wformatString(*this);
}

wstring EntityHealthBase::GetUnhealthyEvaluationDescription() const
{
    wstring ret;
    StringWriter writer(ret);
    for (auto const & eval : unhealthyEvaluations_)
    {
        writer.Write(eval.Evaluation->GetUnhealthyEvaluationDescription());
        writer.WriteLine();
    }

    return ret;
}
