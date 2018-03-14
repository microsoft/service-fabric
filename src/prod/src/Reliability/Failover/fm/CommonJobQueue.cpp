// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;
using namespace std;
using namespace Transport;

FailoverManager::OneWayMessageHandlerJobItem::OneWayMessageHandlerJobItem(
    MessageUPtr && message,
    OneWayReceiverContextUPtr && context,
    TimeSpan timeout)
    : CommonTimedJobItem(timeout)
    , message_(move(message))
    , context_(move(context))
{
}

void FailoverManager::OneWayMessageHandlerJobItem::Process(FailoverManager & fm)
{
    context_->Accept();

    NodeInstance const& from = context_->FromInstance;

    if (message_->Action == RSMessage::GetServiceTypeNotification().Action)
    {
        fm.ServiceTypeNotificationAsyncMessageHandler(*message_, from);
    }
    else if (message_->Action == RSMessage::GetReplicaUp().Action)
    {
        fm.ReplicaUpAsyncMessageHandler(*message_, from);
    }
    else if (message_->Action == RSMessage::GetReplicaDown().Action)
    {
        fm.ReplicaDownAsyncMessageHandler(*message_, from);
    }
    else if (message_->Action == RSMessage::GetServiceTypeEnabled().Action)
    {
        fm.ServiceTypeEnabledAsyncMessageHandler(*message_, from);
    }
    else if (message_->Action == RSMessage::GetServiceTypeDisabled().Action)
    {
        fm.ServiceTypeDisabledAsyncMessageHandler(*message_, from);
    }
    else if (message_->Action == RSMessage::GetNodeActivateReply().Action)
    {
        fm.NodeActivateReplyAsyncMessageHandler(*message_, from);
    }
    else if (message_->Action == RSMessage::GetNodeDeactivateReply().Action)
    {
        fm.NodeDeactivateReplyAsyncMessageHandler(*message_, from);
    }
    else if (message_->Action == RSMessage::GetNodeFabricUpgradeReply().Action)
    {
        fm.NodeFabricUpgradeReplyAsyncMessageHandler(*message_, from);
    }
    else
    {
        MessageUPtr reply = fm.ProcessOneWayMessage(*message_, from);
        if (reply)
        {
            fm.SendToNodeAsync(move(reply), from);
        }
    }
}

void FailoverManager::OneWayMessageHandlerJobItem::OnTimeout(FailoverManager & fm)
{
    context_->Reject(ErrorCode(ErrorCodeValue::Timeout));

    fm.Events.OnQueueTimeout(message_->MessageId.Guid, message_->Action);
}

void FailoverManager::OneWayMessageHandlerJobItem::OnQueueFull(FailoverManager & fm, size_t actualQueueSize)
{
    context_->Reject(ErrorCode(ErrorCodeValue::ServiceTooBusy));

    if (fm.QueueFullThrottle.IsThrottling())
    {
        fm.Events.OnQueueFull(QueueName::CommonQueue, actualQueueSize);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

FailoverManager::RequestReplyMessageHandlerJobItem::RequestReplyMessageHandlerJobItem(
    MessageUPtr && message,
    TimedRequestReceiverContextUPtr && context,
    TimeSpan timeout)
    : CommonTimedJobItem(timeout)
    , message_(move(message))
    , context_(move(context))
{
}

void FailoverManager::RequestReplyMessageHandlerJobItem::Process(FailoverManager & fm)
{
    NodeInstance const & from = context_->Context->FromInstance;

    if (message_->Action == RSMessage::GetCreateService().Action)
    {
        fm.CreateServiceAsyncMessageHandler(*message_, move(context_));
    }
    else if (message_->Action == RSMessage::GetDeleteService().Action)
    {
        fm.DeleteServiceAsyncMessageHandler(*message_, move(context_));
    }
    else if (message_->Action == RSMessage::GetCreateApplicationRequest().Action)
    {
        fm.CreateApplicationAsyncMessageHandler(*message_, move(context_));
    }
    else if (message_->Action == RSMessage::GetDeleteApplicationRequest().Action)
    {
        fm.DeleteApplicationAsyncMessageHandler(*message_, move(context_));
    }
    else if (message_->Action == RSMessage::GetApplicationUpgradeRequest().Action)
    {
        fm.ApplicationUpgradeAsyncMessageHandler(*message_, move(context_));
    }
    else if (message_->Action == RSMessage::GetUpdateService().Action)
    {
        fm.UpdateServiceAsyncMessageHandler(*message_, move(context_));
    }
    else if (message_->Action == RSMessage::GetUpdateSystemService().Action)
    {
        fm.UpdateSystemServiceAsyncMessageHandler(*message_, move(context_));
    }
    else if (message_->Action == RSMessage::GetUpdateApplicationRequest().Action)
    {
        fm.UpdateApplicationAsyncMessageHandler(*message_, move(context_));
    }
    else if (message_->Action == RSMessage::GetNodeStateRemoved().Action)
    {
        fm.NodeStateRemovedAsyncMessageHandler(*message_, move(context_));
    }
    else
    {
        MessageUPtr reply = fm.ProcessRequestReplyMessage(*message_, from);
        if (reply)
        {
            context_->Reply(move(reply));
        }
        else
        {
            context_->Reject(ErrorCode(ErrorCodeValue::OperationFailed));
        }
    }
}

void FailoverManager::RequestReplyMessageHandlerJobItem::OnTimeout(FailoverManager & fm)
{
    fm.Events.OnQueueTimeout(message_->MessageId.Guid, message_->Action);
}

void FailoverManager::RequestReplyMessageHandlerJobItem::OnQueueFull(FailoverManager & fm, size_t actualQueueSize)
{
    if (fm.QueueFullThrottle.IsThrottling())
    {
        fm.Events.OnQueueFull(QueueName::CommonQueue, actualQueueSize);

        context_->Reject(ErrorCode(ErrorCodeValue::ServiceTooBusy));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

FailoverManager::NodeSequenceNumberCommitJobItem::NodeSequenceNumberCommitJobItem(
    ServiceTypeUpdateKind::Enum serviceTypeUpdateEvent,
    vector<ServiceModel::ServiceTypeIdentifier> && serviceTypeIds,
    uint64 sequenceNumber,
    MessageId const & messageId,
    Federation::NodeInstance const & from)
    : CommonTimedJobItem(FailoverConfig::GetConfig().MessageRetryInterval)
    , serviceTypeUpdateEvent_(serviceTypeUpdateEvent)
    , serviceTypeIds_(move(serviceTypeIds))
    , sequenceNumber_(sequenceNumber)
    , messageId_(messageId)
    , from_(from)
{
}

void FailoverManager::NodeSequenceNumberCommitJobItem::Process(FailoverManager & fm)
{
    fm.OnNodeSequenceNumberUpdated(
        serviceTypeUpdateEvent_,
        move(serviceTypeIds_),
        sequenceNumber_,
        messageId_,
        from_);
}

void FailoverManager::NodeSequenceNumberCommitJobItem::OnTimeout(FailoverManager & fm)
{
    if (serviceTypeUpdateEvent_ == ServiceTypeUpdateKind::Enabled)
    {
        fm.MessageEvents.ServiceTypeEnabledReply(messageId_, ErrorCodeValue::Timeout);
    }
    else if (serviceTypeUpdateEvent_ == ServiceTypeUpdateKind::Disabled)
    {
        fm.MessageEvents.ServiceTypeDisabledReply(messageId_, ErrorCodeValue::Timeout);
    }
}

void FailoverManager::NodeSequenceNumberCommitJobItem::OnQueueFull(FailoverManager & fm, size_t actualQueueSize)
{
    if (fm.QueueFullThrottle.IsThrottling())
    {
        fm.Events.OnQueueFull(QueueName::CommonQueue, actualQueueSize);
    }
}
