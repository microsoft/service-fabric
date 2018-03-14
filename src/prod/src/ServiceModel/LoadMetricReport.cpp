// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

LoadMetricReport::LoadMetricReport(
    std::wstring const & name,
    uint value,
    Common::DateTime lastReportedTime)
    : name_(name)
    , value_(value)
    , lastReportedTime_(lastReportedTime)
{
}

LoadMetricReport::LoadMetricReport()
    : lastReportedTime_(DateTime::Zero)
{
}

LoadMetricReport::LoadMetricReport(LoadMetricReport && other)
    : name_(move(other.name_)),
      value_(move(other.value_)),
      lastReportedTime_(move(other.lastReportedTime_))
{
}

LoadMetricReport & LoadMetricReport::operator=(LoadMetricReport && other)
{
    if (this != &other)
    {
        name_ = move(other.name_);
        value_ = move(other.value_);
        lastReportedTime_ = move(other.lastReportedTime_);
    }

    return *this;
}
        
ErrorCode LoadMetricReport::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_LOAD_METRIC_REPORT & publicResult) const 
{
    publicResult.Name = heap.AddString(name_);
    publicResult.Value = static_cast<ULONG>(value_);
    publicResult.LastReportedUtc = lastReportedTime_.AsFileTime;
    return ErrorCode::Success();
}

Common::ErrorCode LoadMetricReport::FromPublicApi(__in FABRIC_LOAD_METRIC_REPORT const& publicResult)
{
    name_ = wstring(publicResult.Name);
    value_ = static_cast<ULONG>(publicResult.Value);
    lastReportedTime_ = DateTime(publicResult.LastReportedUtc);

    return ErrorCode::Success();
}

void LoadMetricReport::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}

std::wstring LoadMetricReport::ToString() const
{
    return wformatString("Name = {0}, Value = {1}, ReportedAt = {2}", name_, value_, lastReportedTime_);
}
