// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

LoadValue::LoadValue(
    std::wstring const & name,
    uint value,
    Common::DateTime lastReportedTime)
    : name_(name)
    , value_(value)
    , lastReportedTime_(lastReportedTime)
{
}

LoadValue::LoadValue()
    : lastReportedTime_(DateTime::Zero)
{
}

LoadValue::LoadValue(LoadValue && other)
    : name_(move(other.name_)),
      value_(move(other.value_)),
      lastReportedTime_(move(other.lastReportedTime_))
{
}

LoadValue & LoadValue::operator=(LoadValue && other)
{
    if (this != &other)
    {
        name_ = move(other.name_);
        value_ = move(other.value_);
        lastReportedTime_ = move(other.lastReportedTime_);
    }

    return *this;
}
        
ErrorCode LoadValue::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_LOAD_VALUE & publicResult) const 
{
    publicResult.Name = heap.AddString(name_);
    publicResult.Value = static_cast<ULONG>(value_);
    publicResult.LastReportedUtc = lastReportedTime_.AsFileTime;
    return ErrorCode::Success();
}

Common::ErrorCode LoadValue::FromPublicApi(__in FABRIC_LOAD_VALUE const& publicResult)
{
    name_ = wstring(publicResult.Name);
    value_ = static_cast<ULONG>(publicResult.Value);
    lastReportedTime_ = DateTime(publicResult.LastReportedUtc);

    return ErrorCode::Success();
}

void LoadValue::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}

std::wstring LoadValue::ToString() const
{
    return wformatString("Name = {0}, Value = {1}, ReportedAt = {2}", name_, value_, lastReportedTime_);
}
