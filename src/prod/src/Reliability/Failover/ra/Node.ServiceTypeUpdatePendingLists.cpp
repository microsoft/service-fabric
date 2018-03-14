// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Node;
using namespace Infrastructure;
using namespace ServiceModel;

ServiceTypeUpdatePendingLists::ServiceTypeUpdatePendingLists()
    : serviceTypeUpdateMsgSeqNum_(0)
    , hasPendingPartitionUpdates_(false)
{
}

bool ServiceTypeUpdatePendingLists::TryUpdateServiceTypeLists(
    ServiceTypeUpdateKind::Enum updateEvent,
    ServiceModel::ServiceTypeIdentifier const& serviceTypeId)
{
    bool wasOtherUpdateCancelled = CancelPendingOtherServiceTypeUpdate(updateEvent, serviceTypeId);

    bool shouldIncrement = ShouldIncrementSeqNumForKind(updateEvent) || wasOtherUpdateCancelled;

    return AddPendingServiceTypeUpdate(updateEvent, serviceTypeId, shouldIncrement);
}

bool ServiceTypeUpdatePendingLists::TryUpdatePartitionLists(
    ServiceTypeUpdateKind::Enum updateEvent,
    PartitionId const& partitionId)
{
    bool shouldIncrement = ShouldIncrementSeqNumForKind(updateEvent);

    if (updateEvent == ServiceTypeUpdateKind::PartitionEnabled)
    {
        return RemoveDisabledPartition(partitionId, shouldIncrement);
    }
    
    if (updateEvent == ServiceTypeUpdateKind::PartitionDisabled)
    {
        return AddDisabledPartition(partitionId, shouldIncrement);
    }
    
    Assert::TestAssert("Invalid ServiceTypeUpdateKind passed to update pending partition lists.");

    return false;
}

bool ServiceTypeUpdatePendingLists::CancelPendingOtherServiceTypeUpdate(
    ServiceTypeUpdateKind::Enum updateEvent,
    const ServiceModel::ServiceTypeIdentifier& serviceTypeId)
{
    auto & pendingList = updateEvent == ServiceTypeUpdateKind::Enabled ? pendingDisabledServices_ : pendingEnabledServices_;

    // Cancel any other pending update for this service type
    return RemoveFromList<ServiceTypeIdentifier>(pendingList, serviceTypeId);
}

bool ServiceTypeUpdatePendingLists::AddPendingServiceTypeUpdate(
    ServiceTypeUpdateKind::Enum updateEvent,
    const ServiceModel::ServiceTypeIdentifier& serviceTypeId,
    bool shouldIncrement)
{
    auto & pendingList = updateEvent == ServiceTypeUpdateKind::Enabled ? pendingEnabledServices_ : pendingDisabledServices_;

    // Add the ServiceType to the pending list
    bool wasAdded = AddToListIfDoesNotExist(pendingList, serviceTypeId);
    if (shouldIncrement && wasAdded)
    {
        ++serviceTypeUpdateMsgSeqNum_;
        return true;
    }

    return false;
}

bool ServiceTypeUpdatePendingLists::AddDisabledPartition(
    PartitionId const& partitionId,
    bool shouldIncrement)
{
    // Add the partition to the disabled list
    bool wasAdded = AddToListIfDoesNotExist<PartitionId>(disabledPartitions_, partitionId);
    if (wasAdded)
    {
        hasPendingPartitionUpdates_ = true;

        if (shouldIncrement)
        {
            ++serviceTypeUpdateMsgSeqNum_;
            return true;
        }
    }

    return false;
}

bool ServiceTypeUpdatePendingLists::RemoveDisabledPartition(
    PartitionId const& partitionId,
    bool shouldIncrement)
{
    bool wasRemoved = RemoveFromList<PartitionId>(disabledPartitions_, partitionId);
    if (wasRemoved)
    {
        hasPendingPartitionUpdates_ = true;

        if (shouldIncrement)
        {
            ++serviceTypeUpdateMsgSeqNum_;
            return true;
        }
    }

    return false;
}

template<typename T>
bool ServiceTypeUpdatePendingLists::RemoveFromList(set<T> & list, T const& item)
{
    return list.erase(item) == 1;
}

template<typename T>
bool ServiceTypeUpdatePendingLists::AddToListIfDoesNotExist(set<T> & list, T const& item)
{
    return list.insert(item).second;
}

bool ServiceTypeUpdatePendingLists::HasPendingUpdates() const
{
    return HasPendingUpdates(ServiceTypeUpdateKind::Disabled) ||
           HasPendingUpdates(ServiceTypeUpdateKind::Enabled) ||
           HasPendingUpdates(ServiceTypeUpdateKind::PartitionDisabled);
}

bool ServiceTypeUpdatePendingLists::HasPendingUpdates(ServiceTypeUpdateKind::Enum updateEvent) const
{
    switch(updateEvent)
    {
    case ServiceTypeUpdateKind::Disabled:
        return !pendingDisabledServices_.empty();
    case ServiceTypeUpdateKind::Enabled:
        return !pendingEnabledServices_.empty();
    case ServiceTypeUpdateKind::PartitionDisabled:
        return hasPendingPartitionUpdates_;
    case ServiceTypeUpdateKind::PartitionEnabled:
        return hasPendingPartitionUpdates_;
    default:
        return false;
    }
}

bool ServiceTypeUpdatePendingLists::ShouldIncrementSeqNumForKind(ServiceTypeUpdateKind::Enum updateEvent) const
{
    ServiceTypeUpdateKind::Enum nextMessageKind;
    bool hasNext = TryGetNextMessageKind(nextMessageKind);
    if (!hasNext)
    {
        return true;
    }

    if (updateEvent == ServiceTypeUpdateKind::Enabled &&
        nextMessageKind == ServiceTypeUpdateKind::Disabled)
    {
        return false;
    }

    if ((updateEvent == ServiceTypeUpdateKind::PartitionDisabled || updateEvent == ServiceTypeUpdateKind::PartitionEnabled) &&
        (nextMessageKind == ServiceTypeUpdateKind::Disabled || nextMessageKind == ServiceTypeUpdateKind::Enabled))
    {
        return false;
    }

    return true;
}

bool ServiceTypeUpdatePendingLists::TryGetNextMessageKind(__out ServiceTypeUpdateKind::Enum & kind) const
{
    if (HasPendingUpdates(ServiceTypeUpdateKind::Disabled))
    {
        kind = ServiceTypeUpdateKind::Disabled;
        return true;
    }

    if (HasPendingUpdates(ServiceTypeUpdateKind::Enabled))
    {
        kind = ServiceTypeUpdateKind::Enabled;
        return true;
    }

    if (HasPendingUpdates(ServiceTypeUpdateKind::PartitionDisabled))
    {
        kind = ServiceTypeUpdateKind::PartitionDisabled;
        return true;
    }

    return false;
}

bool ServiceTypeUpdatePendingLists::TryGetMessage(__out MessageDescription & description)
{
    ServiceTypeUpdateKind::Enum updateEvent;
    bool hasNext = TryGetNextMessageKind(updateEvent);
    if (!hasNext)
    {
        return false;
    }

    description.UpdateEvent = updateEvent;

    if (updateEvent == ServiceTypeUpdateKind::Disabled ||
        updateEvent == ServiceTypeUpdateKind::Enabled)
    {
        GetPendingServiceTypes(
            updateEvent,
            description.ServiceTypes,
            description.SequenceNumber);
    }
    else if (updateEvent == ServiceTypeUpdateKind::PartitionDisabled)
    {
        GetPendingPartitions(
            description.Partitions,
            description.SequenceNumber);
    }
    else
    {
        Assert::TestAssert("Invalid update event kind received by try get message.");
    }

    return true;
}

void ServiceTypeUpdatePendingLists::GetPendingServiceTypes(
    ServiceTypeUpdateKind::Enum updateEvent,
    __out vector<ServiceTypeIdentifier> & serviceTypes,
    __out uint64 & sequenceNumber)
{
    auto & pendingUpdatesSet = updateEvent == ServiceTypeUpdateKind::Disabled ? pendingDisabledServices_ : pendingEnabledServices_;
    serviceTypes = vector<ServiceTypeIdentifier>(pendingUpdatesSet.cbegin(), pendingUpdatesSet.cend());
    sequenceNumber = ++serviceTypeUpdateMsgSeqNum_;
}

void ServiceTypeUpdatePendingLists::GetPendingPartitions(
    __out vector<PartitionId> & partitions,
    __out uint64 & sequenceNumber)
{
    partitions = vector<PartitionId>(disabledPartitions_.cbegin(), disabledPartitions_.cend());
    sequenceNumber = ++serviceTypeUpdateMsgSeqNum_;
}

ServiceTypeUpdatePendingLists::ProcessReplyResult ServiceTypeUpdatePendingLists::TryProcessReply(
    ServiceTypeUpdateKind::Enum updateEvent,
    uint64 sequenceNumber)
{
    // Only process reply if corresponds to the current list
    if (sequenceNumber != serviceTypeUpdateMsgSeqNum_.load())
    {
        return ProcessReplyResult::Stale;
    }

    if (updateEvent == ServiceTypeUpdateKind::Disabled)
    {
        pendingDisabledServices_.clear();
    }
    else if (updateEvent == ServiceTypeUpdateKind::Enabled)
    {
        pendingEnabledServices_.clear();
    }
    else if (updateEvent == ServiceTypeUpdateKind::PartitionDisabled)
    {
        hasPendingPartitionUpdates_ = false;
    }

    return HasPendingUpdates() ? ProcessReplyResult::HasPending : ProcessReplyResult::NoPending;
}

ServiceTypeUpdatePendingLists::MessageDescription::MessageDescription()
    : UpdateEvent(ServiceTypeUpdateKind::Disabled)
    , SequenceNumber(0)
    , ServiceTypes()
    , Partitions()
{
}
