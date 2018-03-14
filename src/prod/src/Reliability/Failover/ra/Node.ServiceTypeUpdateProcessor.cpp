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

namespace
{
    wstring const MessageWorkManagerName(L"Message");
}

ServiceTypeUpdateProcessor::ServiceTypeUpdateProcessor(
    ReconfigurationAgent & ra,
    TimeSpanConfigEntry const& cleanupInterval,
    TimeSpanConfigEntry const& stalenessEntryTtl)
    : ra_(ra)
    , stalenessChecker_(ra_.Clock, stalenessEntryTtl)
    , stalenessCleanupTimer_(
        L"RA.ServiceTypeUpdateStaleness",
        ra,
        cleanupInterval,
        L"_ServiceTypeUpdateStaleness",
        [this](wstring const &, ReconfigurationAgent &)
        {
            this->OnCleanupTimerCallback();
        })
    , pendingLists_()
{
    Open();
}

void ServiceTypeUpdateProcessor::Open()
{
    CreateMessageWorkManager();

    stalenessCleanupTimer_.Set();
}

void ServiceTypeUpdateProcessor::CreateMessageWorkManager()
{
    messageWorkManager_ = make_unique<BackgroundWorkManagerWithRetry>(
        ra_.NodeInstanceIdStr + L"_" + MessageWorkManagerName,
        MessageWorkManagerName,
        [this](wstring const &, ReconfigurationAgent&, BackgroundWorkManagerWithRetry& bgmr)
        {
            bool isRetryRequired = MessageSendCallback();
            bgmr.OnWorkComplete(isRetryRequired);
        },
        ra_.Config.PerNodeMinimumIntervalBetweenServiceTypeMessageToFMEntry,
        ra_.Config.MessageRetryIntervalEntry,
        ra_);
}

void ServiceTypeUpdateProcessor::ProcessServiceTypeDisabled(
    ServiceTypeIdentifier const& serviceTypeId,
    uint64 sequenceNumber,
    ServicePackageActivationContext const& activationContext)
{
    UNREFERENCED_PARAMETER(activationContext);

    ProcessServiceTypeUpdate(
        ServiceTypeUpdateKind::Disabled,
        serviceTypeId,
        sequenceNumber);
}

void ServiceTypeUpdateProcessor::ProcessServiceTypeEnabled(
    ServiceTypeIdentifier const& serviceTypeId,
    uint64 sequenceNumber,
    ServicePackageActivationContext const& activationContext)
{
    UNREFERENCED_PARAMETER(activationContext);

    ProcessServiceTypeUpdate(
        ServiceTypeUpdateKind::Enabled,
        serviceTypeId,
        sequenceNumber);
}

void ServiceTypeUpdateProcessor::ProcessServiceTypeUpdate(
    ServiceTypeUpdateKind::Enum updateEvent,
    ServiceTypeIdentifier const& serviceTypeId,
    uint64 sequenceNumber)
{
    bool shouldSendMessage = false;

    if (!ra_.StateObj.IsFMReady)
    {
        RAEventSource::Events->HostingProcessServiceTypeEvent(
            ra_.NodeInstanceIdStr,
            serviceTypeId,
            updateEvent,
            ServiceTypeUpdateResult::RANotOpen,
            sequenceNumber);

        return;
    }

    {
        AcquireWriteLock lock(lock_);

        // Check if stale and/or update the staleness map
        bool updated = stalenessChecker_.TryUpdateServiceTypeSequenceNumber(serviceTypeId, sequenceNumber);
        if (!updated)
        {
            RAEventSource::Events->HostingProcessServiceTypeEvent(
                ra_.NodeInstanceIdStr,
                serviceTypeId,
                updateEvent,
                ServiceTypeUpdateResult::Stale,
                sequenceNumber);

            return;
        }

        // Update pending update lists
        shouldSendMessage = pendingLists_.TryUpdateServiceTypeLists(updateEvent, serviceTypeId);
    }

    if (shouldSendMessage)
    {
        // Send message to FM indicate service type is disabled on this node
        messageWorkManager_->Request(DefaultActivityId);
    }

    RAEventSource::Events->HostingProcessServiceTypeEvent(
        ra_.NodeInstanceIdStr,
        serviceTypeId,
        updateEvent,
        ServiceTypeUpdateResult::Processed,
        sequenceNumber);
}

void ServiceTypeUpdateProcessor::ProcessPartitionUpdate(
    ServiceTypeUpdateKind::Enum updateEvent,
    PartitionId partitionId,
    uint64 sequenceNumber)
{
    bool shouldSendMessage = false;

    // If RA has been closed, then nothing to do
    if (!ra_.StateObj.IsFMReady)
    {
        RAEventSource::Events->HostingProcessPartitionEvent(
            ra_.NodeInstanceIdStr,
            partitionId,
            updateEvent,
            ServiceTypeUpdateResult::RANotOpen,
            sequenceNumber);

        return;
    }

    {
        AcquireWriteLock lock(lock_);

        // Try to update the staleness map
        bool updated = stalenessChecker_.TryUpdatePartitionSequenceNumber(partitionId, sequenceNumber);
        if (!updated)
        {
            RAEventSource::Events->HostingProcessPartitionEvent(
                ra_.NodeInstanceIdStr,
                partitionId,
                updateEvent,
                ServiceTypeUpdateResult::Stale,
                sequenceNumber);

            return;
        }
        
        // Update pending disabled lists
        shouldSendMessage = pendingLists_.TryUpdatePartitionLists(updateEvent, partitionId);
    }

    if (shouldSendMessage)
    {
        // Send message to FM to indicate this partition is disabled
        messageWorkManager_->Request(DefaultActivityId);
    }

    RAEventSource::Events->HostingProcessPartitionEvent(
        ra_.NodeInstanceIdStr,
        partitionId,
        updateEvent,
        ServiceTypeUpdateResult::Processed,
        sequenceNumber);
}

bool ServiceTypeUpdateProcessor::MessageSendCallback()
{
    if (!ra_.IsOpen)
    {
        return false;
    }

    {
        AcquireWriteLock lock(lock_);

        if (!ra_.StateObj.IsFMReady)
        {
            return pendingLists_.HasPendingUpdates();
        }

        ServiceTypeUpdatePendingLists::MessageDescription description;
        bool hasNext = pendingLists_.TryGetMessage(description);
        if (!hasNext)
        {
            return false;
        }

        auto & updateEvent = description.UpdateEvent;
        if (updateEvent == ServiceTypeUpdateKind::Disabled ||
            updateEvent == ServiceTypeUpdateKind::Enabled)
        {
            SendServiceTypeUpdateMessage(move(description));
        }
        else if (updateEvent == ServiceTypeUpdateKind::PartitionDisabled)
        {
            SendPartitionNotificationMessage(move(description));
        }

        return true;
    }
}

void ServiceTypeUpdateProcessor::SendServiceTypeUpdateMessage(
    ServiceTypeUpdatePendingLists::MessageDescription && description)
{
    ServiceTypeUpdateMessageBody msgBody(
        move(description.ServiceTypes),
        description.SequenceNumber);

    RAEventSource::Events->HostingSendServiceTypeMessage(
        ra_.NodeInstanceIdStr,
        description.UpdateEvent,
        msgBody.SequenceNumber);

    MessageUPtr msg;
    if (description.UpdateEvent == ServiceTypeUpdateKind::Disabled)
    {
        msg = RSMessage::GetServiceTypeDisabled().CreateMessage<ServiceTypeUpdateMessageBody>(msgBody);
    }
    else
    {
        msg = RSMessage::GetServiceTypeEnabled().CreateMessage<ServiceTypeUpdateMessageBody>(msgBody);
    }

    ra_.Federation.SendToFM(move(msg));
}

void ServiceTypeUpdateProcessor::SendPartitionNotificationMessage(
    ServiceTypeUpdatePendingLists::MessageDescription && description)
{
    PartitionNotificationMessageBody msgBody(
        move(description.Partitions),
        description.SequenceNumber);

    RAEventSource::Events->HostingSendPartitionNotificationMessage(
        ra_.NodeInstanceIdStr,
        msgBody.SequenceNumber);

    MessageUPtr msg = RSMessage::GetPartitionNotification().CreateMessage<PartitionNotificationMessageBody>(msgBody);

    ra_.Federation.SendToFM(move(msg));
}

void ServiceTypeUpdateProcessor::ServiceTypeEnabledReplyHandler(Message & request)
{
    ServiceTypeUpdateReplyHandler(ServiceTypeUpdateKind::Enabled, request);
}

void ServiceTypeUpdateProcessor::ServiceTypeDisabledReplyHandler(Message & request)
{
    ServiceTypeUpdateReplyHandler(ServiceTypeUpdateKind::Disabled, request);
}

void ServiceTypeUpdateProcessor::PartitionNotificationReplyHandler(Message & request)
{
    ServiceTypeUpdateReplyHandler(ServiceTypeUpdateKind::PartitionDisabled, request);
}

void ServiceTypeUpdateProcessor::ServiceTypeUpdateReplyHandler(
    ServiceTypeUpdateKind::Enum updateEvent,
    Message & request)
{
    int64 retrySequenceNumber = messageWorkManager_->RetryTimerObj.SequenceNumber;

    {
        AcquireWriteLock lock(lock_);

        ServiceTypeUpdateReplyMessageBody body;
        if (!TryGetMessageBody(ra_.NodeInstanceIdStr, request, body))
        {
            return;
        }

        RAEventSource::Events->HostingProcessServiceTypeReplyMessage(
            ra_.NodeInstanceIdStr,
            updateEvent,
            request.Action,
            body.SequenceNumber);

        auto result = pendingLists_.TryProcessReply(updateEvent, body.SequenceNumber);

        if (result == ServiceTypeUpdatePendingLists::ProcessReplyResult::Stale)
        {
            RAEventSource::Events->HostingIgnoreStaleServiceTypeReplyMessage(
                ra_.NodeInstanceIdStr,
                updateEvent,
                request.Action);
        }
        else if (result == ServiceTypeUpdatePendingLists::ProcessReplyResult::NoPending)
        {
            CancelRetryTimer(retrySequenceNumber);
        }
    }
}

void ServiceTypeUpdateProcessor::CancelRetryTimer(int64 sequenceNumber)
{
    messageWorkManager_->RetryTimerObj.TryCancel(sequenceNumber);
}

void ServiceTypeUpdateProcessor::OnCleanupTimerCallback()
{
    {
        AcquireWriteLock lock(lock_);

        stalenessChecker_.PerformCleanup();
    }
}

void ServiceTypeUpdateProcessor::Close()
{
    stalenessCleanupTimer_.Close();
    messageWorkManager_->Close();
}
