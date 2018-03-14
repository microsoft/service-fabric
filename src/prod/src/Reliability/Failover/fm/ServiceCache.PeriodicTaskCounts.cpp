// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

ServicePeriodicTaskCounts::ServicePeriodicTaskCounts()
    : ServicesUpdateHealthSequenceStarted(0)
{
}

string ServicePeriodicTaskCounts::AddField(Common::TraceEvent & traceEvent, string const & name)
{
    string format = "ServicesUpdateHealthSequenceStarted={0}";
    size_t index = 0;

    traceEvent.AddEventField<size_t>(format, name + ".servicesUpdateHealthSequenceStarted", index);

    return format;
}

void ServicePeriodicTaskCounts::FillEventData(Common::TraceEventContext & context) const
{
    context.Write(ServicesUpdateHealthSequenceStarted);
}

void ServicePeriodicTaskCounts::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.WriteLine(
        "ServicesUpdateHealthSequenceStarted={0}",
        ServicesUpdateHealthSequenceStarted);
}
