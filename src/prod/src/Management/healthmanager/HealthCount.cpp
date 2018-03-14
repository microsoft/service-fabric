// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace ServiceModel;
using namespace Management::HealthManager;

HealthCount::HealthCount()
    : okCount_(0)
    , warningCount_(0)
    , errorCount_(0)
    , totalCount_(0)
{
}

HealthCount::~HealthCount()
{
}

HealthCount::HealthCount(HealthCount && other)
    : okCount_(move(other.okCount_))
    , warningCount_(move(other.warningCount_))
    , errorCount_(move(other.errorCount_))
    , totalCount_(move(other.totalCount_))
{
}
            
HealthCount & HealthCount::operator = (HealthCount && other)
{
    if (this != &other)
    {
        okCount_ = move(other.okCount_);
        warningCount_ = move(other.warningCount_);
        errorCount_ = move(other.errorCount_);
        totalCount_ = move(other.totalCount_);
    }

    return *this;
}

void HealthCount::AddResult(FABRIC_HEALTH_STATE result)
{
    switch (result)
    {
    case FABRIC_HEALTH_STATE_ERROR:
        ++errorCount_;
        break;
    case FABRIC_HEALTH_STATE_WARNING:
        ++warningCount_;
        break;
    case FABRIC_HEALTH_STATE_OK:
        ++okCount_;
        break;
    default:
        Assert::CodingError("unsupported health state {0}", result);
    }

    ++totalCount_;
}

bool HealthCount::IsHealthy(BYTE maxPercentUnhealthy) const
{
    float ratio = static_cast<float>(maxPercentUnhealthy) / 100.0f;
    return (static_cast<float>(errorCount_) <= ceil(totalCount_ * ratio));
}

bool HealthCount::IsRelevant(
    FABRIC_HEALTH_STATE aggregatedHealthState, 
    FABRIC_HEALTH_STATE targetHealthState)
{
    if (aggregatedHealthState == FABRIC_HEALTH_STATE_ERROR)
    {
        // Only errors are relevant
        return targetHealthState == FABRIC_HEALTH_STATE_ERROR;
    } 
    else if (aggregatedHealthState == FABRIC_HEALTH_STATE_WARNING)
    {
        return targetHealthState != FABRIC_HEALTH_STATE_OK;
    }

    return false;
}

FABRIC_HEALTH_STATE HealthCount::GetHealthState(BYTE maxPercentUnhealthy) const
{
    bool isHealthy = IsHealthy(maxPercentUnhealthy);

    if (isHealthy)
    {
        if (okCount_ == totalCount_)
        {
            return FABRIC_HEALTH_STATE_OK;
        }
        else
        {
            return FABRIC_HEALTH_STATE_WARNING;
        }
    }

    return FABRIC_HEALTH_STATE_ERROR;
}

void HealthCount::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("ok:{0}/w:{1}/e:{2}", okCount_, warningCount_, errorCount_);
}

std::string HealthCount::AddField(Common::TraceEvent & traceEvent, std::string const & name)
{
    traceEvent.AddField<uint64>(name + ".ok");
    traceEvent.AddField<uint64>(name + ".warning");
    traceEvent.AddField<uint64>(name + ".error");
            
    return "ok:{0}/w:{1}/e:{2}";
}

void HealthCount::FillEventData(TraceEventContext & context) const
{
    context.Write<uint64>(okCount_);
    context.Write<uint64>(warningCount_);
    context.Write<uint64>(errorCount_);
}

std::vector<HealthEvaluation> HealthCount::FilterUnhealthy(
    std::vector<HealthEvaluation> const & entries, 
    FABRIC_HEALTH_STATE aggregatedHealthState)
{
    std::vector<HealthEvaluation> unhealthy;
    for (auto const & entry : entries)
    {
        if (HealthCount::IsRelevant(aggregatedHealthState, entry.Evaluation->AggregatedHealthState))
        {
            unhealthy.push_back(entry);
        }
    }

    return unhealthy;
}
