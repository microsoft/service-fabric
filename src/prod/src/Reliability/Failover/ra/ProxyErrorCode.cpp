// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability::ReconfigurationAgentComponent;

void ProxyErrorCode::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("{0}:{1} '{2}'", errorCode_, api_, message_);
}

std::string ProxyErrorCode::AddField(Common::TraceEvent & traceEvent, std::string const & name)
{
    string format = "{0}:{1} '{2}'";
    size_t index = 0;

    traceEvent.AddEventField<Common::ErrorCode>(format, name + ".errorCode", index);
    traceEvent.AddEventField<ApiMonitoring::ApiNameDescription>(format, name + ".api", index);
    traceEvent.AddEventField<std::wstring>(format, name + ".message", index);

    return format;
}

void ProxyErrorCode::FillEventData(Common::TraceEventContext & context) const
{
    context.Write(errorCode_);
    context.Write(api_);
    context.Write(message_);
}
