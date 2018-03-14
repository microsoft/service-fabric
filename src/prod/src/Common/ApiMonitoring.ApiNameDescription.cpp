// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ApiMonitoring;

string ApiNameDescription::AddField(Common::TraceEvent & traceEvent, string const & name)
{
    string format = "{0}.{1}({2})";
    size_t index = 0;

    traceEvent.AddEventField<InterfaceName::Trace>(format, name + ".interface", index);
    traceEvent.AddEventField<ApiName::Trace>(format, name + ".api", index);
    traceEvent.AddEventField<std::wstring>(format, name + ".metadata", index);

    return format;
}

void ApiNameDescription::FillEventData(TraceEventContext & context) const
{
    context.WriteCopy<uint>(interfaceName_);
    context.WriteCopy<uint>(apiName_);
    context.Write(metadata_);
}

void ApiNameDescription::WriteTo(TextWriter & writer, FormatOptions const &) const
{
    writer.Write("{0}.{1}({2})", interfaceName_, apiName_, metadata_);
}

wstring ApiNameDescription::ToString() const
{
    wstring result;
    StringWriter(result).Write("{0}.{1}({2})", interfaceName_, apiName_, metadata_);
    return result;
}
