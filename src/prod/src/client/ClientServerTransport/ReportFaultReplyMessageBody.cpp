// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ServiceModel;

ReportFaultReplyMessageBody::ReportFaultReplyMessageBody()
{
}

ReportFaultReplyMessageBody::ReportFaultReplyMessageBody(ErrorCode error, std::wstring const & message)
    : error_(error),
      message_(message)
{
}

string ReportFaultReplyMessageBody::AddField(Common::TraceEvent & traceEvent, string const & name)
{
    string format = "Error = {0}, Message = {1}";
    size_t index = 0;

    traceEvent.AddEventField<ErrorCode>(format, name + ".error", index);
    traceEvent.AddEventField<std::wstring>(format, name + ".message", index);

    return format;
}

void ReportFaultReplyMessageBody::FillEventData(TraceEventContext & context) const
{
    context.Write(error_);
    context.Write(message_);
}

void ReportFaultReplyMessageBody::WriteTo(TextWriter& w, FormatOptions const &) const
{
    w.Write("Error = {0}, Message = {1}", error_, message_);
}
