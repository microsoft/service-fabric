// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;

void NodeInstance::WriteTo(TextWriter& w, FormatOptions const&) const
{
    w.Write("{0}:{1}", id_, instanceId_);
}

wstring NodeInstance::ToString() const
{
    return wformatString(*this);
}

string NodeInstance::AddField(Common::TraceEvent & traceEvent, string const & name)
{
    string format = "{0}:{1}";
    size_t index = 0;

    traceEvent.AddEventField<Federation::NodeId>(format, name + ".id", index);
    traceEvent.AddEventField<uint64>(format, name + ".instance", index);
            
    return format;
}

void NodeInstance::FillEventData(TraceEventContext & context) const
{
    context.Write<Federation::NodeId>(id_);
    context.Write<uint64>(instanceId_);
}

bool NodeInstance::TryParse(wstring const & data, __out NodeInstance & nodeInstance)
{
    size_t splitLoc = data.find(L":", 0);
    if (splitLoc == std::wstring::npos || splitLoc == 0 || splitLoc == data.size() - 1)
    {
        return false;
    }
            
    wstring nodeIdStr = data.substr(0, splitLoc);
    wstring iIdStr = data.substr(splitLoc + 1);

    uint64 instanceId;
    if (!StringUtility::TryFromWString<uint64>(iIdStr, instanceId))
    {
        return false; 
    }

    NodeId id;
    if (!NodeId::TryParse(nodeIdStr, id))
    {
        return false;
    }
            
    nodeInstance = NodeInstance(id, instanceId);
    return true;
}
