// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

ServiceContextCounts::ServiceContextCounts()
    : ServiceToPartitionMapContexts(0),
	  UpdateServiceContexts(0),
	  DeleteServiceContexts(0)
{
}

string ServiceContextCounts::AddField(Common::TraceEvent & traceEvent, string const & name)
{
    string format = "ServiceToPartitionMapContexts={0}\r\nUpdateServiceContexts={1}\r\nDeleteServiceContexts={2}";
    size_t index = 0;

    traceEvent.AddEventField<size_t>(format, name + ".serviceToPartitionMapContexts", index);
    traceEvent.AddEventField<size_t>(format, name + ".updateServiceContexts", index);
    traceEvent.AddEventField<size_t>(format, name + ".deleteServiceContexts", index);

    return format;
}

void ServiceContextCounts::FillEventData(Common::TraceEventContext & context) const
{
    context.Write(ServiceToPartitionMapContexts);
    context.Write(UpdateServiceContexts);
    context.Write(DeleteServiceContexts);
}

void ServiceContextCounts::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.WriteLine(
        "ServiceToPartitionMapContexts={0}, UpdateServiceContexts={1}, DeleteServiceContexts={2}",
        ServiceToPartitionMapContexts,
        UpdateServiceContexts,
        DeleteServiceContexts);
}
