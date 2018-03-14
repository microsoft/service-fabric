// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Node;

std::string NodeDeactivationInfo::AddField(Common::TraceEvent& traceEvent, std::string const& name)
{
    string format = "IsActivated = {0}. SequenceNumber = {1}.";
    size_t index = 0;

    traceEvent.AddEventField<bool>(format, name + ".isActivated", index);
    traceEvent.AddEventField<int64>(format, name + ".sequenceNumber", index);

    return format;
}

void NodeDeactivationInfo::FillEventData(Common::TraceEventContext& context) const
{
    context.Write(isActivated_);
    context.Write(sequenceNumber_);
}

void NodeDeactivationInfo::WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
{
    w << wformatString("IsActivated = {0}. SequenceNumber = {1}.", isActivated_, sequenceNumber_);
}
