// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

HealthReportSendOptions::HealthReportSendOptions()
    : immediate_(false)
{
}

HealthReportSendOptions::HealthReportSendOptions(bool immediate)
    : immediate_(immediate)
{
}

HealthReportSendOptions::~HealthReportSendOptions()
{
}

ErrorCode HealthReportSendOptions::ToPublicApi(
    __in ScopedHeap & heap,
    __inout FABRIC_HEALTH_REPORT_SEND_OPTIONS & sendOptions) const
{
    UNREFERENCED_PARAMETER(heap);

    sendOptions.Immediate = immediate_ ? TRUE : FALSE;
    return ErrorCode::Success();
}

ErrorCode HealthReportSendOptions::FromPublicApi(
    FABRIC_HEALTH_REPORT_SEND_OPTIONS const & sendOptions)
{
    immediate_ = (sendOptions.Immediate == TRUE) ? true : false;
    return ErrorCode::Success();
}

void HealthReportSendOptions::WriteTo(__in TextWriter& writer, FormatOptions const &) const
{
    writer.Write("immediate={0}", immediate_);
}

wstring HealthReportSendOptions::ToString() const
{
    wstring result;
    StringWriter(result).Write(*this);
    return result;
}

std::string HealthReportSendOptions::AddField(Common::TraceEvent & traceEvent, std::string const & name)
{
    traceEvent.AddField<bool>(name + ".immediate");
    return "immediate={0}";
}

void HealthReportSendOptions::FillEventData(Common::TraceEventContext & context) const
{
    context.Write<bool>(immediate_);
}
