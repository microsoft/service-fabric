// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

StringLiteral const TraceLookupTable("LookupTable");

FMServiceLookupTable::FMServiceLookupTable(
    FailoverManager& fm,
    vector<FailoverUnitUPtr>& failoverUnits,
    int64 savedLookupVersion,
    Common::ComponentRoot const & root)
    : ServiceLookupTable(root),
    fm_(fm),
    endVersion_(savedLookupVersion + 1),
    lastBroadcast_(DateTime::Zero)
{
    for (FailoverUnitUPtr const& failoverUnit : failoverUnits)
    {
        if (failoverUnit->LookupVersion > 0)
        {
            if (failoverUnit->LookupVersion >= endVersion_)
            {
                endVersion_ = failoverUnit->LookupVersion + 1;
            }

            UpdateEntryCallerHoldingLock(*failoverUnit);
        }
    }

    versionRangeCollection_ = VersionRangeCollection(1, endVersion_);

    broadcastVersionRangeCollections_ = vector<VersionRangeCollection>(FailoverConfig::GetConfig().ServiceLookupTableBroadcastAttemptCount, versionRangeCollection_);

    fm_.WriteInfo(TraceLookupTable, "{0} restored lookup version {1}", fm_.Id, endVersion_ - 1);

    auto componentRoot = Root.CreateComponentRoot();
    broadcastTimer_ = Timer::Create("FM.Broadcast", [this, componentRoot] (TimerSPtr const&) { BroadcastTimerCallback(); });
    StartBroadcastTimer();
}

void FMServiceLookupTable::set_EndVersion(int64 value)
{
	endVersion_ = value;

    // Set the VersionRangeCollection as this is called from GFUM Transfer
    versionRangeCollection_ = VersionRangeCollection(1, value);
}

void FMServiceLookupTable::Dispose()
{
    broadcastTimer_->Cancel();
}

void FMServiceLookupTable::UpdateLookupVersion(FailoverUnit & failoverUnit)
{
	failoverUnit.LookupVersion = InterlockedIncrement64(&endVersion_) - 1;
}

void FMServiceLookupTable::OnUpdateFailed(FailoverUnit const& failoverUnit)
{
    AcquireWriteLock grab(LockObject);
    UpdateVersionRangesCallerHoldingLock(failoverUnit);
}

void FMServiceLookupTable::Update(FailoverUnit const& failoverUnit)
{
    AcquireWriteLock grab(LockObject);

    UpdateEntryCallerHoldingLock(failoverUnit);
    UpdateVersionRangesCallerHoldingLock(failoverUnit);
}

void FMServiceLookupTable::UpdateEntryCallerHoldingLock(FailoverUnit const& failoverUnit)
{
    ServiceTableEntrySPtr serviceTableEntry = make_shared<ServiceTableEntry>(
        ConsistencyUnitId(failoverUnit.Id.Guid),
        failoverUnit.ServiceName,
        failoverUnit.CreateServiceReplicaSet(),
        !failoverUnit.IsOrphaned);

    ServiceLookupTable::TryUpdateEntryCallerHoldingLock(move(serviceTableEntry));
}

void FMServiceLookupTable::UpdateVersionRangesCallerHoldingLock(FailoverUnit const& failoverUnit)
{
    versionRangeCollection_.Add(VersionRange(failoverUnit.LookupVersion, failoverUnit.LookupVersion + 1));
}

void FMServiceLookupTable::RemoveEntry(FailoverUnit const& failoverUnit)
{
    AcquireWriteLock grab(LockObject);

    UpdateVersionRangesCallerHoldingLock(failoverUnit);
    TryRemoveEntryCallerHoldingLock(ConsistencyUnitId(failoverUnit.Id.Guid));
}

void FMServiceLookupTable::GetUpdatesCallerHoldingLock(
    size_t pageSizeLimit,
    VersionRangeCollection const& knownVersionRangeCollection,
    __out vector<ServiceTableEntry> & entriesToUpdate,
    __out VersionRangeCollection & versionRangesToUpdate) const
{
    versionRangesToUpdate = versionRangeCollection_;

    pageSizeLimit -= versionRangeCollection_.EstimateSize();

    int64 nextVersion = GetUpdatedEntriesCallerHoldingLock(
        pageSizeLimit,
        entriesToUpdate,
        knownVersionRangeCollection);

    if (nextVersion > 0)
    {
        versionRangesToUpdate.Remove(VersionRange(nextVersion, versionRangeCollection_.EndVersion));
    }
}

bool FMServiceLookupTable::GetUpdates(
    size_t pageSizeLimit,
    GenerationNumber const& generation,
    vector<ConsistencyUnitId> const& consistencyUnitIds,
    VersionRangeCollection && knownVersionRangeCollection,
    __out vector<ServiceTableEntry> & entriesToUpdate,
    __out VersionRangeCollection & versionRangesToUpdate,
    __out int64 & endVersion) const
{
    AcquireReadLock grab(LockObject);

    if (generation > fm_.Generation)
    {
        return false;
    }

    VersionRangeCollection knownRange = (generation < fm_.Generation ? VersionRangeCollection() : knownVersionRangeCollection);
    VersionRangeCollection rangeToRemove = knownRange;

    for (ConsistencyUnitId const& consistencyUnitId : consistencyUnitIds)
    {
        ServiceTableEntry entry(consistencyUnitId);
        if (TryGetEntryCallerHoldsReadLock(consistencyUnitId, entry))
        {
            knownRange.AddRange(entry.Version, entry.Version + 1);
        }

        entriesToUpdate.push_back(move(entry));
        pageSizeLimit -= entry.EstimateSize();
    }

    GetUpdatesCallerHoldingLock(
        pageSizeLimit,
        knownRange,
        entriesToUpdate,
        versionRangesToUpdate);

    versionRangesToUpdate.Remove(rangeToRemove);

    endVersion = versionRangesToUpdate.EndVersion;

    return true;
}

void FMServiceLookupTable::StartBroadcastTimer()
{
    broadcastTimer_->Change(FailoverConfig::GetConfig().ServiceLocationBroadcastInterval);
}

bool FMServiceLookupTable::TryGetServiceTableUpdateMessageBody(__out ServiceTableUpdateMessageBody & body)
{
    AcquireWriteLock grab(LockObject);

    fm_.ServiceEvents.LookupTableBroadcastRanges(versionRangeCollection_, broadcastVersionRangeCollections_[0]);

    auto pageSizeLimit = static_cast<size_t>(
        Federation::FederationConfig::GetConfig().SendQueueSizeLimit * FailoverConfig::GetConfig().MessageContentBufferRatio);

    vector<ServiceTableEntry> entries;
    VersionRangeCollection versionRanges;
    GetUpdatesCallerHoldingLock(pageSizeLimit, broadcastVersionRangeCollections_[0], entries, versionRanges);

    VersionRangeCollection versionRangesToBroadcast(versionRanges);
    versionRangesToBroadcast.Remove(broadcastVersionRangeCollections_[0]);

    if (!versionRangesToBroadcast.IsEmpty)
    {
        for (size_t i = 0; i < broadcastVersionRangeCollections_.size() - 1; i++)
        {
            broadcastVersionRangeCollections_[i] = broadcastVersionRangeCollections_[i + 1];
        }
        broadcastVersionRangeCollections_[broadcastVersionRangeCollections_.size() - 1] = versionRanges;
    }
    else if (fm_.IsMaster ||
        endVersion_ == 1u ||
        DateTime::Now() < lastBroadcast_ + FailoverConfig::GetConfig().ServiceLookupTableEmptyBroadcastInterval)
    {
        return false;
    }

    int64 endVersion = (versionRangesToBroadcast.IsEmpty ? versionRangeCollection_.EndVersion : versionRangesToBroadcast.EndVersion);

    body = ServiceTableUpdateMessageBody(move(entries), fm_.Generation, move(versionRangesToBroadcast), endVersion, fm_.IsMaster);
    return true;
}

void FMServiceLookupTable::BroadcastTimerCallback()
{
    if (fm_.IsActive)
    {
        if (fm_.IsReady)
        {
            ServiceTableUpdateMessageBody body;
            if (TryGetServiceTableUpdateMessageBody(body))
            {
                fm_.ServiceEvents.LookupTableBroadcast(body);

                MessageUPtr serviceTableUpdateMessage = RSMessage::GetServiceTableUpdate().CreateMessage(body);

                lastBroadcast_ = DateTime::Now();
                fm_.Broadcast(move(serviceTableUpdateMessage));
            }
        }

        StartBroadcastTimer();
    }
}
