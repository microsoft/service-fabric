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

ReportFaultReplyMessageBody::ReportFaultReplyMessageBody(ErrorCode error)
: error_(error)
{
}

string ReportFaultReplyMessageBody::AddField(Common::TraceEvent & traceEvent, string const & name)
{
    string format = "Error = {0}";
    size_t index = 0;

    traceEvent.AddEventField<ErrorCode>(format, name + ".error", index);

    return format;
}

void ReportFaultReplyMessageBody::FillEventData(TraceEventContext & context) const
{
    context.Write(error_);
}

void ReportFaultReplyMessageBody::WriteTo(TextWriter& w, FormatOptions const &) const
{
    w.Write("Error = {0}", error_);
}
