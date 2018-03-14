// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

NodeCounts::NodeCounts()
    : nodeCount_(0),
      upNodeCount_(0),
      downNodeCount_(0),
      deactivatedCount_(0),
      pauseIntentCount_(0),
      restartIntentCount_(0),
      removeDataIntentCount_(0),
      removeNodeIntentCount_(0),
      unknownCount_(0),
      removedNodeCount_(0),
      pendingDeactivateNode_(0),
      pendingFabricUpgrade_(0)
{
}

string NodeCounts::AddField(Common::TraceEvent & traceEvent, string const & name)
{
    string format = "Nodes={0}\r\nUp={1}\r\nDown={2}\r\nDeactivatedNodes={3}\r\n  Pause={4}\r\n  Restart={5}\r\n  RemoveData={6}\r\n  RemoveNode={7}\r\nUnknown={8}\r\nRemovedNodes={9}\r\nPendingDeactivateNode={10}\r\nPendingFabricUpgrade={11}";
    size_t index = 0;

    traceEvent.AddEventField<size_t>(format, name + ".nodes", index);
    traceEvent.AddEventField<size_t>(format, name + ".upNodeCount", index);
    traceEvent.AddEventField<size_t>(format, name + ".downNodeCount", index);
    traceEvent.AddEventField<size_t>(format, name + ".deactivatedNodes", index);
    traceEvent.AddEventField<size_t>(format, name + ".pauseIntentNodes", index);
    traceEvent.AddEventField<size_t>(format, name + ".restartIntentNodes", index);
    traceEvent.AddEventField<size_t>(format, name + ".removeDataIntentNodes", index);
    traceEvent.AddEventField<size_t>(format, name + ".removeNodeIntentNodes", index);
    traceEvent.AddEventField<size_t>(format, name + ".unknownCount", index);
    traceEvent.AddEventField<size_t>(format, name + ".removedNodeCount", index);
    traceEvent.AddEventField<size_t>(format, name + ".pendingDeactivateNode", index);
    traceEvent.AddEventField<size_t>(format, name + ".pendingFabricUpgrade", index);

    return format;
}

void NodeCounts::FillEventData(Common::TraceEventContext & context) const
{
    context.Write(nodeCount_);
    context.Write(upNodeCount_);
    context.Write(downNodeCount_);
    context.Write(deactivatedCount_);
    context.Write(pauseIntentCount_);
    context.Write(restartIntentCount_);
    context.Write(removeDataIntentCount_);
    context.Write(removeNodeIntentCount_);
    context.Write(unknownCount_);
    context.Write(removedNodeCount_);
    context.Write(pendingDeactivateNode_);
    context.Write(pendingFabricUpgrade_);
}

void NodeCounts::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.WriteLine(
        "Nodes={0}, Up={1}, Down={2}, DeactivatedNodes={3} (Pause={4}, Restart={5}, RemoveData={6}, RemoveNode={7}), Unknown={8}, RemovedNodes={9}, PendingDeactivateNode={10}, PendingFabricUpgrade={11}",
        nodeCount_,
        upNodeCount_,
        downNodeCount_,
        deactivatedCount_,
        pauseIntentCount_,
        restartIntentCount_,
        removeDataIntentCount_,
        removeNodeIntentCount_,
        unknownCount_,
        removedNodeCount_,
        pendingDeactivateNode_,
        pendingFabricUpgrade_);
}
