// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace ServiceModel;
using namespace Management::HealthManager;

INITIALIZE_SIZE_ESTIMATION(HealthStateCount)

HealthStateCount::HealthStateCount()
    : okCount_(0)
    , warningCount_(0)
    , errorCount_(0)
{
}

HealthStateCount::HealthStateCount(ULONG okCount, ULONG warningCount, ULONG errorCount)
    : okCount_(okCount)
    , warningCount_(warningCount)
    , errorCount_(errorCount)
{
}

HealthStateCount::~HealthStateCount()
{
}

HealthStateCount::HealthStateCount(HealthStateCount const & other)
    : okCount_(other.okCount_)
    , warningCount_(other.warningCount_)
    , errorCount_(other.errorCount_)
{
}

HealthStateCount & HealthStateCount::operator = (HealthStateCount const & other)
{
    if (this != &other)
    {
        okCount_ = other.okCount_;
        warningCount_ = other.warningCount_;
        errorCount_ = other.errorCount_;
    }

    return *this;
}

HealthStateCount::HealthStateCount(HealthStateCount && other)
    : okCount_(move(other.okCount_))
    , warningCount_(move(other.warningCount_))
    , errorCount_(move(other.errorCount_))
{
}
            
HealthStateCount & HealthStateCount::operator = (HealthStateCount && other)
{
    if (this != &other)
    {
        okCount_ = move(other.okCount_);
        warningCount_ = move(other.warningCount_);
        errorCount_ = move(other.errorCount_);
    }

    return *this;
}

void HealthStateCount::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_HEALTH_STATE_COUNT & publicHealthCount) const
{
    UNREFERENCED_PARAMETER(heap);

    publicHealthCount.OkCount = okCount_;
    publicHealthCount.WarningCount = warningCount_;
    publicHealthCount.ErrorCount = errorCount_;
}

Common::ErrorCode HealthStateCount::FromPublicApi(
    FABRIC_HEALTH_STATE_COUNT const & publicHealthCount)
{
    okCount_ = publicHealthCount.OkCount;
    warningCount_ = publicHealthCount.WarningCount;
    errorCount_ = publicHealthCount.ErrorCount;
    return ErrorCode::Success();
}

void HealthStateCount::Add(FABRIC_HEALTH_STATE result)
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
}

void HealthStateCount::AppendCount(HealthStateCount const & other)
{
    okCount_ += other.OkCount;
    warningCount_ += other.WarningCount;
    errorCount_ += other.ErrorCount;
}

void HealthStateCount::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("ok:{0}/w:{1}/e:{2}", okCount_, warningCount_, errorCount_);
}

std::string HealthStateCount::AddField(Common::TraceEvent & traceEvent, std::string const & name)
{
    traceEvent.AddField<uint64>(name + ".ok");
    traceEvent.AddField<uint64>(name + ".warning");
    traceEvent.AddField<uint64>(name + ".error");
            
    return "ok:{0}/w:{1}/e:{2}";
}

void HealthStateCount::FillEventData(TraceEventContext & context) const
{
    context.Write<uint64>(okCount_);
    context.Write<uint64>(warningCount_);
    context.Write<uint64>(errorCount_);
}
