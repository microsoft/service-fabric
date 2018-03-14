// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;


FailoverUnitCountsContext::FailoverUnitCountsContext()
    : BackgroundThreadContext(L"FailoverUnitCountsContext"),
      statelessFailoverUnitCount_(0),
      volatileFailoverUnitCount_(0),
      persistedFailoverUnitCount_(0),
      quorumLossFailoverUnitCount_(0),
      unhealthyFailoverUnitCount_(0),
      deletingFailoverUnitCount_(0),
      deletedFailoverUnitCount_(0),
      replicaCount_(0),
      inBuildReplicaCount_(0),
      standByReplicaCount_(0),
      offlineReplicaCount_(0),
      droppedReplicaCount_(0)
{
}

BackgroundThreadContextUPtr FailoverUnitCountsContext::CreateNewContext() const
{
    return make_unique<FailoverUnitCountsContext>();
}

void FailoverUnitCountsContext::Merge(BackgroundThreadContext const & context)
{
    FailoverUnitCountsContext const & other = dynamic_cast<FailoverUnitCountsContext const &>(context);
    
    persistedFailoverUnitCount_ += other.persistedFailoverUnitCount_;
    volatileFailoverUnitCount_ += other.volatileFailoverUnitCount_;
    statelessFailoverUnitCount_ += other.statelessFailoverUnitCount_;
    quorumLossFailoverUnitCount_ += other.quorumLossFailoverUnitCount_;
    unhealthyFailoverUnitCount_ += other.unhealthyFailoverUnitCount_;
    deletingFailoverUnitCount_ += other.deletingFailoverUnitCount_;
    deletedFailoverUnitCount_ += other.deletedFailoverUnitCount_;

    replicaCount_ += other.replicaCount_;
    inBuildReplicaCount_ += other.inBuildReplicaCount_;
    standByReplicaCount_ += other.standByReplicaCount_;
    offlineReplicaCount_ += other.offlineReplicaCount_;
    droppedReplicaCount_ += other.droppedReplicaCount_;
}

void FailoverUnitCountsContext::Process(FailoverManager const& fm, FailoverUnit const& failoverUnit)
{
    UNREFERENCED_PARAMETER(fm);

    if (failoverUnit.IsStateful)
    {
        if (failoverUnit.HasPersistedState)
        {
            persistedFailoverUnitCount_++;
        }

        if (failoverUnit.IsVolatile)
        {
            volatileFailoverUnitCount_++;
        }
    }
    else
    {
        statelessFailoverUnitCount_++;
    }

    if (failoverUnit.IsQuorumLost())
    {
        quorumLossFailoverUnitCount_++;
    }

    if (failoverUnit.IsUnhealthy)
    {
        unhealthyFailoverUnitCount_++;
    }

    if (failoverUnit.IsOrphaned)
    {
        deletedFailoverUnitCount_++;
    }
    else if (failoverUnit.IsToBeDeleted)
    {
        deletingFailoverUnitCount_++;
    }

    replicaCount_ += failoverUnit.ReplicaCount;
    inBuildReplicaCount_ += failoverUnit.InBuildReplicaCount;
    standByReplicaCount_ += failoverUnit.StandByReplicaCount;
    offlineReplicaCount_ += failoverUnit.OfflineReplicaCount;
    droppedReplicaCount_ += failoverUnit.DroppedReplicaCount;
}

bool FailoverUnitCountsContext::ReadyToComplete()
{
    return true;
}

void FailoverUnitCountsContext::Complete(FailoverManager & fm, bool isContextCompleted, bool isEnumerationAborted)
{
    UNREFERENCED_PARAMETER(isEnumerationAborted);

    size_t failoverUnitCount = fm.FailoverUnitCacheObj.Count;
    size_t inBuildFailoverUnitCount = fm.InBuildFailoverUnitCacheObj.Count;
    
    TraceWriter traceWriter(
        *this,
        failoverUnitCount,
        inBuildFailoverUnitCount);

    fm.Events.FailoverUnitCounts(traceWriter, isContextCompleted);

    if (isContextCompleted)
    {
        fm.FailoverUnitCounters->NumberOfFailoverUnits.Value = static_cast<Common::PerformanceCounterValue>(failoverUnitCount);
        fm.FailoverUnitCounters->NumberOfInBuildFailoverUnits.Value = static_cast<Common::PerformanceCounterValue>(inBuildFailoverUnitCount);
        fm.FailoverUnitCounters->NumberOfUnhealthyFailoverUnits.Value = static_cast<Common::PerformanceCounterValue>(unhealthyFailoverUnitCount_);

        fm.FailoverUnitCounters->NumberOfReplicas.Value = static_cast<Common::PerformanceCounterValue>(replicaCount_);
        fm.FailoverUnitCounters->NumberOfInBuildReplicas.Value = static_cast<Common::PerformanceCounterValue>(inBuildReplicaCount_);
        fm.FailoverUnitCounters->NumberOfStandByReplicas.Value = static_cast<Common::PerformanceCounterValue>(standByReplicaCount_);
        fm.FailoverUnitCounters->NumberOfOfflineReplicas.Value = static_cast<Common::PerformanceCounterValue>(offlineReplicaCount_);
    }
}

FailoverUnitCountsContext::TraceWriter::TraceWriter(
    FailoverUnitCountsContext const& performanceCounterContext,
    size_t failoverUnitCount,
    size_t inBuildFailoverUnitCount)
    : performanceCounterContext_(performanceCounterContext),
      failoverUnitCount_(failoverUnitCount),
      inBuildFailoverUnitCount_(inBuildFailoverUnitCount)
{
}

string FailoverUnitCountsContext::TraceWriter::AddField(Common::TraceEvent & traceEvent, string const & name)
{
    size_t index = 0;
    string format = "FailoverUnits={0}\r\n  Persisted={1}\r\n  Volatile={2}\r\n  Stateless={3}\r\n  QuorumLoss={4}\r\n  InBuild={5}\r\n  Unhealthy={6}\r\n  Deleting={7}\r\n  Deleted={8}\r\nReplicas={9}\r\n  InBuild={10}\r\n  Standby={11}\r\n  Offline={12}\r\n  Dropped={13}";

    traceEvent.AddEventField<size_t>(format, name + ".failoverUnits", index);
    traceEvent.AddEventField<int>(format, name + ".persistedFailoverUnits", index);
    traceEvent.AddEventField<int>(format, name + ".volatileFailoverUnits", index);
    traceEvent.AddEventField<int>(format, name + ".statelessFailoverUnits", index);

    traceEvent.AddEventField<int>(format, name + ".quorumLossFailoverUnits", index);
    traceEvent.AddEventField<size_t>(format, name + ".inBuildFailoverUnits", index);
    traceEvent.AddEventField<int>(format, name + ".unhealthyFailoverUnits", index);
    traceEvent.AddEventField<int>(format, name + ".deletingFailoverUnits", index);
    traceEvent.AddEventField<int>(format, name + ".deletedFailoverUnits", index);

    traceEvent.AddEventField<size_t>(format, name + ".replicas", index);
    traceEvent.AddEventField<size_t>(format, name + ".inBuildReplicas", index);
    traceEvent.AddEventField<size_t>(format, name + ".standByReplicas", index);
    traceEvent.AddEventField<size_t>(format, name + ".offlineReplicas", index);
    traceEvent.AddEventField<size_t>(format, name + ".droppedReplicas", index);

    return format;
}

void FailoverUnitCountsContext::TraceWriter::FillEventData(Common::TraceEventContext & context) const
{
    context.Write(failoverUnitCount_);
    context.Write(performanceCounterContext_.persistedFailoverUnitCount_);
    context.Write(performanceCounterContext_.volatileFailoverUnitCount_);
    context.Write(performanceCounterContext_.statelessFailoverUnitCount_);

    context.Write(performanceCounterContext_.quorumLossFailoverUnitCount_);
    context.Write(inBuildFailoverUnitCount_);
    context.Write(performanceCounterContext_.unhealthyFailoverUnitCount_);
    context.Write(performanceCounterContext_.deletingFailoverUnitCount_);
    context.Write(performanceCounterContext_.deletedFailoverUnitCount_);

    context.Write(performanceCounterContext_.replicaCount_);
    context.Write(performanceCounterContext_.inBuildReplicaCount_);
    context.Write(performanceCounterContext_.standByReplicaCount_);
    context.Write(performanceCounterContext_.offlineReplicaCount_);
    context.Write(performanceCounterContext_.droppedReplicaCount_);
}

void FailoverUnitCountsContext::TraceWriter::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.WriteLine(
        "FailoverUnits={0}, InBuildFailoverUnits={1}, PersistedFailoverUnits={2}, VolatileFailoverUnits={3}, StatelessFailoverUnits={4}, QuorumLossFailoverUnits={5}, UnhealthyFailoverUnits={6}, DeletingFailoverUnits={7}, DeletedFailoverUnits={8}",
        failoverUnitCount_,
        inBuildFailoverUnitCount_,
        performanceCounterContext_.persistedFailoverUnitCount_,
        performanceCounterContext_.volatileFailoverUnitCount_,
        performanceCounterContext_.statelessFailoverUnitCount_,
        performanceCounterContext_.quorumLossFailoverUnitCount_,
        performanceCounterContext_.unhealthyFailoverUnitCount_,
        performanceCounterContext_.deletingFailoverUnitCount_,
        performanceCounterContext_.deletedFailoverUnitCount_);

    writer.WriteLine(
        "Replicas={0}, InBuildReplicas={1}, StandByReplicas={2}, OfflineReplicas={3}, DroppedReplicas={4}",
        performanceCounterContext_.replicaCount_,
        performanceCounterContext_.inBuildReplicaCount_,
        performanceCounterContext_.standByReplicaCount_,
        performanceCounterContext_.offlineReplicaCount_,
        performanceCounterContext_.droppedReplicaCount_);
}
