// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

QueueCounts::QueueCounts(
    bool isUpgradingFabric,
    uint64 commonQueueLength,
    uint64 queryQueueLength,
    uint64 failoverUnitQueueLength,
    uint64 commitQueueLength,
    int commonQueueCompleted,
    int queryQueueCompleted,
    int failoverUnitQueueCompleted,
    int commitQueueCompleted)
    : commonQueueLength_(commonQueueLength),
      queryQueueLength_(queryQueueLength),
      failoverUnitQueueLength_(failoverUnitQueueLength),
      commitQueueLength_(commitQueueLength),
      commonQueueCompleted_(commonQueueCompleted),
      queryQueueCompleted_(queryQueueCompleted),
      failoverUnitQueueCompleted_(failoverUnitQueueCompleted),
      commitQueueCompleted_(commitQueueCompleted),
      isUpgradingFabric_(isUpgradingFabric)
{
}

string QueueCounts::AddField(Common::TraceEvent & traceEvent, string const & name)
{
    string format = "CommonQueue={0}/{1}\r\nQueryQueue={2}/{3}\r\nFailoverUnitQueueLength={4}/{5}\r\nCommitQueueLength={6}/{7}\r\nIsUpgradingFabric={8}";
    size_t index = 0;

    traceEvent.AddEventField<uint64>(format, name + ".commonQueueLength", index);
    traceEvent.AddEventField<int>(format, name + ".commonQueueCompleted", index);
    traceEvent.AddEventField<uint64>(format, name + ".queryQueueLength", index);
    traceEvent.AddEventField<int>(format, name + ".queryQueueCompleted", index);
    traceEvent.AddEventField<uint64>(format, name + ".failoverUnitQueueLength", index);
    traceEvent.AddEventField<int>(format, name + ".failoverUnitQueueCompleted", index);
    traceEvent.AddEventField<uint64>(format, name + ".commitQueueLength", index);
    traceEvent.AddEventField<int>(format, name + ".commitQueueCompleted", index);
    traceEvent.AddEventField<bool>(format, name + ".isUpgradingFabric", index);

    return format;
}

void QueueCounts::FillEventData(Common::TraceEventContext & context) const
{
    context.Write(commonQueueLength_);
    context.Write(commonQueueCompleted_);
    context.Write(queryQueueLength_);
    context.Write(queryQueueCompleted_);
    context.Write(failoverUnitQueueLength_);
    context.Write(failoverUnitQueueCompleted_);
    context.Write(commitQueueLength_);
    context.Write(commitQueueCompleted_);
    context.Write(isUpgradingFabric_);
}

void QueueCounts::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.WriteLine(
        "CommonQueue={0}/{1}, QueryQueue={2}/{3}, FailoverUnitQueue={4}/{5}, CommitQueue={6}/{7}",
        commonQueueLength_, commonQueueCompleted_,
        queryQueueLength_, queryQueueCompleted_,
        failoverUnitQueueLength_, failoverUnitQueueCompleted_,
        commitQueueLength_, commitQueueCompleted_);

    writer.Write(
        "IsUpgradingFabric={0}",
        isUpgradingFabric_);
}
