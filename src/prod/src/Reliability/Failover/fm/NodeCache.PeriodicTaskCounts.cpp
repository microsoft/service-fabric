// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

NodePeriodicTaskCounts::NodePeriodicTaskCounts()
    : NodesToRemove(0),
      NodesRemovesStarted(0),
      UpgradeHealthReports(0),
      DeactivationStuckHealthReports(0),
      DeactivationCompleteHealthReports(0)
{
}

string NodePeriodicTaskCounts::AddField(Common::TraceEvent & traceEvent, string const & name)
{
    string format = "NodesToRemove={0}\r\nNodesRemovesStarted={1}\r\nUpgradeHealthReports={2}\r\nDeactivationStuckHealthReports={3}\r\nDeactivationCompleteHealthReports={4}";
    size_t index = 0;

    traceEvent.AddEventField<size_t>(format, name + ".nodesToRemove", index);
    traceEvent.AddEventField<size_t>(format, name + ".nodesRemovesStarted", index);
    traceEvent.AddEventField<size_t>(format, name + ".upgradeHealthReports", index);
    traceEvent.AddEventField<size_t>(format, name + ".deactivationStuckHealthReports", index);
    traceEvent.AddEventField<size_t>(format, name + ".deactivationCompleteHealthReports", index);

    return format;
}

void NodePeriodicTaskCounts::FillEventData(Common::TraceEventContext & context) const
{
    context.Write(NodesToRemove);
    context.Write(NodesRemovesStarted);
    context.Write(UpgradeHealthReports);
    context.Write(DeactivationStuckHealthReports);
    context.Write(DeactivationCompleteHealthReports);
}

void NodePeriodicTaskCounts::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.WriteLine(
        "NodesToRemove={0}, NodesRemovesStarted={1}, UpgradeHealthReports={2}, DeactivationStuckHealthReports={3}, DeactivationCompleteHealthReports={4}",
        NodesToRemove,
        NodesRemovesStarted,
        UpgradeHealthReports,
        DeactivationStuckHealthReports,
        DeactivationCompleteHealthReports);
}
