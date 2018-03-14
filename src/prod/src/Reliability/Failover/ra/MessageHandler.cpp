// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

#include "ServiceModel/management/ResourceMonitor/public.h"

using namespace std;

using namespace Common;
using namespace Hosting2;
using namespace Transport;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace ServiceModel;
using namespace ClientServerTransport;
using namespace Infrastructure;
using namespace ReconfigurationAgentComponent::Communication;

namespace
{
    StringLiteral const UnknownMessage("Unknown message");
    StringLiteral const InvalidNodeLifeCycleState("Invalid RA Lifecycle State");
    StringLiteral const QueueFull("Message Queue Full");
    StringLiteral const Deprecated("Message is deprecated");
    StringLiteral const InvalidMessage("Message deserialization failed");
    StringLiteral const EntryNotFound("Entry not found");
    StringLiteral const RANotReady("RA not ready");

    std::wstring GetActivityId(Transport::Message & message)
    {
        Transport::FabricActivityHeader header;
        if (!message.Headers.TryReadFirst(header))
        {
            return wformatString(message.MessageId);
        }

        return wformatString(header.ActivityId);
    }

    template <typename TBody, typename TEntity>
    StringLiteral const * TryCreateJobItem(
        ReconfigurationAgent & ra,
        MessageMetadata const * metadata,
        Transport::Message & message,
        GenerationHeader const & generationHeader,
        Federation::NodeInstance const & from,
        typename MessageContext<TBody, TEntity>::MessageProcessorFunctionPtr messageProcessor,
        __out EntityJobItemBaseSPtr & jobItem)
    {
        TBody body;
        if (!TryGetMessageBody(ra.NodeInstanceIdStr, message, body))
        {
            return &InvalidMessage;
        }

        auto entityId = EntityTraits<TEntity>::GetEntityIdFromMessage(body);
        if (!ra.IsReady(entityId))
        {
            return &RANotReady;
        }

        auto & entityMap = EntityTraits<TEntity>::GetEntityMap(ra);
        auto entry = entityMap.GetOrCreateEntityMapEntry(entityId, metadata->CreatesEntity);
        if (entry == nullptr)
        {
            return &EntryNotFound;
        }

        typedef MessageContext<TBody, TEntity> MessageContextType;
        MessageContextType context(metadata, message.Action, move(body), generationHeader, from, messageProcessor);

        typedef EntityJobItem<TEntity, MessageContextType> JobItemType;

        auto handler = [](typename JobItemType::HandlerParametersType & handlerParameters, MessageContextType & context)
        {
            return context.Process(handlerParameters);
        };

        typename JobItemType::Parameters parameters(
            entry,
            GetActivityId(message),
            handler,
            context.GetJobItemCheck(),
            *JobItemDescription::MessageProcessing,
            move(context));

        auto inner = std::make_shared<JobItemType>(move(parameters));

        jobItem = static_pointer_cast<EntityJobItemBase>(inner);

        return nullptr;
    }
}

MessageHandler::MessageHandler(ReconfigurationAgent & ra) : ra_(ra)
{
}

template<typename TContext>
void MessageHandler::Reject(TContext & context)
{
    UNREFERENCED_PARAMETER(context);
    static_assert(false, "use specialization");
}

template<>
void MessageHandler::Reject<Federation::OneWayReceiverContext>(Federation::OneWayReceiverContext & context)
{
    context.Reject(ErrorCodeValue::ObjectClosed);
}

template<>
void MessageHandler::Reject<Transport::IpcReceiverContext>(Transport::IpcReceiverContext & context)
{
    UNREFERENCED_PARAMETER(context);
}

template<>
void MessageHandler::Reject<Federation::RequestReceiverContext>(Federation::RequestReceiverContext & context)
{
    ra_.Federation.CompleteRequestReceiverContext(context, ErrorCodeValue::ObjectClosed);
}

template<typename TContext>
void MessageHandler::RejectAndTrace(
    TContext & context,
    Common::StringLiteral const & reason,
    wstring const & action,
    Transport::MessageId const & id)
{
    Reject(context);

    RAEventSource::Events->MessageIgnore(ra_.NodeInstanceIdStr, reason, action, id);
}

// All of these functions are executing on threads that need to be released ASAP
// They should check if the message needs to be dropped
// If not they simply enqueue it onto a message queue and allow transport thread to continue
template<typename TContext>
void MessageHandler::ProcessRequestHelper(
    bool assertOnUnknownMessage,
    Common::PerformanceCounterData* perfCounter,
    Transport::MessageUPtr & message,
    std::unique_ptr<TContext> && context)
{
    if (perfCounter != nullptr)
    {
        perfCounter->Increment();
    }

    auto metadata = ra_.MessageMetadataTable.LookupMetadata(message);
    if (metadata == nullptr)
    {
        ASSERT_IF(assertOnUnknownMessage, "Unknown message {0}", message->Action);
        RejectAndTrace(*context, UnknownMessage, message->Action, message->MessageId);
        return;
    }

    if (metadata->IsDeprecated)
    {
        RejectAndTrace(*context, Deprecated, message->Action, message->MessageId);
        return;
    }

    if (!ra_.MessageHandlerObj.CanProcessMessageBasedOnNodeLifeCycle(*metadata))
    {
        RejectAndTrace(*context, InvalidNodeLifeCycleState, message->Action, message->MessageId);
        return;
    }

    auto msgClone = message->Clone();
    auto contextPtr = context.get();
    MessageProcessingJobItem<ReconfigurationAgent> msgContext(metadata, move(msgClone), move(context));
    if (!ra_.Threadpool.EnqueueIntoMessageQueue(msgContext))
    {
        // Accessing context is safe here
        // The job queue guarantees that msgContext will be moved from
        // only if it is able to enqueue
        RejectAndTrace(*contextPtr, QueueFull, message->Action, message->MessageId);
        return;
    }
}

void MessageHandler::ProcessTransportRequest(Transport::MessageUPtr & message, Federation::OneWayReceiverContextUPtr && context)
{
    ProcessRequestHelper<Federation::OneWayReceiverContext>(
        false,
        &ra_.PerfCounters.FederationMessagesReceivedPerSecond,
        message,
        move(context));
}

void MessageHandler::ProcessTransportIpcRequest(Transport::MessageUPtr & message, Transport::IpcReceiverContextUPtr && context)
{
    ProcessRequestHelper<Transport::IpcReceiverContext>(
        true,
        &ra_.PerfCounters.IpcMessagesReceivedPerSecond,
        message,
        move(context));
}

void MessageHandler::ProcessTransportRequestResponseRequest(Transport::MessageUPtr & message, Federation::RequestReceiverContextUPtr && context)
{
    ProcessRequestHelper<Federation::RequestReceiverContext>(
        false,
        nullptr,
        message,
        move(context));
}

void MessageHandler::ProcessRequest(
    IMessageMetadata const * baseMetadata,
    MessageUPtr && requestPtr,
    Federation::OneWayReceiverContextUPtr && context)
{
    ASSERT_IF(baseMetadata == nullptr, "Metadata cannot be null. Unknown message should be traced and dropped on receive when metadata is looked up");
    auto metadata = static_cast<Communication::MessageMetadata const *>(baseMetadata);

    // Always accept all federation one-way context
    // Higher layers have a retry mechanism built in
    // And Failover does not use routing layer retries
    context->Accept();

    Message& request = *requestPtr;

    wstring const & action = request.Action;

    if (!CanProcessMessageBasedOnNodeLifeCycle(*metadata))
    {
        RAEventSource::Events->MessageIgnore(ra_.NodeInstanceIdStr, InvalidNodeLifeCycleState, action, request.MessageId);
        return;
    }

    auto from = context->FromInstance;
    RAEventSource::Events->MessageReceive(ra_.NodeInstanceIdStr, action, from, request.MessageId);

    GenerationHeader generationHeader;
    if (!ReadAndCheckGenerationHeader(request, from, generationHeader))
    {
        return;
    }

    // Route the message to the appropriate handler
    // The message can be for the RA or for a specific entity
    // First handle messages for the node itself
    // And then handle per entity messages

    //NB: The generation message should be processed as soon as RA is open.
    //      It should not wait for RA to be ready since RA is ready only when the
    //      NodeUpAck has been received with the new versions of the replicas.
    //      However, NodeUp will be processed by the FM and it will send NodeUpAck
    //      only after any ongoing rebuilds have completed.
    if (metadata->Target == MessageTarget::RA)
    {
        if (action == RSMessage::GetGenerationUpdate().Action)
        {
            ra_.ProcessGenerationUpdate(request, from);
        }
        else if (action == RSMessage::GetReplicaUpReply().Action)
        {
            // This is received from FM for ReplicaUp processing
            ra_.ReplicaUpReplyMessageHandler(request);
        }
        else if (action == RSMessage::GetNodeActivateRequest().Action)
        {
            ra_.NodeDeactivationMessageProcessorObj.ProcessActivateMessage(request);
        }
        else if (action == RSMessage::GetNodeDeactivateRequest().Action)
        {
            ra_.NodeDeactivationMessageProcessorObj.ProcessDeactivateMessage(request);
        }

        // These are received from FM for service type enable/disable handling
        else if (action == RSMessage::GetServiceTypeEnabledReply().Action)
        {
            ra_.ServiceTypeUpdateProcessorObj.ServiceTypeEnabledReplyHandler(request);
        }
        else if (action == RSMessage::GetServiceTypeDisabledReply().Action)
        {
            ra_.ServiceTypeUpdateProcessorObj.ServiceTypeDisabledReplyHandler(request);
        }

        // Upgrade messages
        else if (action == RSMessage::GetNodeUpgradeRequest().Action)
        {
            ra_.ProcessNodeUpgradeHandler(request);
        }
        else if (action == RSMessage::GetCancelApplicationUpgradeRequest().Action)
        {
            ra_.ProcessCancelApplicationUpgradeHandler(request);
        }

        else if (action == RSMessage::GetNodeFabricUpgradeRequest().Action)
        {
            ra_.ProcessNodeFabricUpgradeHandler(request);
        }
        else if (action == RSMessage::GetCancelFabricUpgradeRequest().Action)
        {
            ra_.ProcessCancelFabricUpgradeHandler(request);
        }
        else if (action == RSMessage::GetNodeUpdateServiceRequest().Action)
        {
            ra_.ProcessNodeUpdateServiceRequest(request);
        }
        else if (action == RSMessage::GetServiceTypeNotificationReply().Action)
        {
            ra_.ServiceTypeNotificationReplyHandler(request);
        }
        else if (action == RSMessage::GetPartitionNotificationReply().Action)
        {
            ra_.ServiceTypeUpdateProcessorObj.PartitionNotificationReplyHandler(request);
        }
        else if (action == RSMessage::GetServiceBusy().Action)
        {
            // TODO: RA does not process this yet
        }
        else
        {
            Assert::CodingError("Unkonwn federation message for RA {0} {1}", request.MessageId, request.Action);
        }
    }
    else if (metadata->Target == MessageTarget::FT)
    {
        ProcessFTMessage(metadata, move(requestPtr), generationHeader, from);
    }
    else
    {
        Assert::CodingError("Unknown target for federation message {0} {1}", request.MessageId, request.Action);
    }
}

void MessageHandler::ProcessIpcMessage(
    IMessageMetadata const * baseMetadata,
    MessageUPtr && messagePtr,
    IpcReceiverContextUPtr && context)
{
    ASSERT_IF(baseMetadata == nullptr, "Metadata cannot be null. Unknown message should be traced and dropped on receive when metadata is looked up");
    auto metadata = static_cast<Communication::MessageMetadata const *>(baseMetadata);

    MessageUPtr msgPtr = move(messagePtr);
    Message & message = *msgPtr;

    if (!CanProcessMessageBasedOnNodeLifeCycle(*metadata))
    {
        RAEventSource::Events->MessageIgnore(ra_.NodeInstanceIdStr, InvalidNodeLifeCycleState, message.Action, message.MessageId);
        return;
    }

    // Route the message to the appropriate handler
    wstring const& action = message.Action;

    RAEventSource::Events->MessageReceive(ra_.NodeInstanceIdStr, action, ReconfigurationAgent::InvalidNode, message.MessageId);

    if (metadata->Target == MessageTarget::RA)
    {
        if (action == RAMessage::GetReportLoad().Action)
        {
            // Since the message will contain multiple load for multiple FailoverUnits, it does
            // not fit the single-FailoverUnit context very well.  We could add verification logic in
            // other way, but it is important since load report is best effort. 
            ra_.ReportLoadMessageHandler(message);
        }
        else if (action == HealthManagerTcpMessage::ReportHealthAction)
        {
            // Report health handling
            ra_.ReportHealthReportMessageHandler(move(msgPtr), move(context));
        }
        else if (action == Management::ResourceMonitor::ResourceUsageReport::ResourceUsageReportAction)
        {
            //we need to convert these resource usage reports into actual load reports
            ra_.ReportResourceUsageHandler(message);
        }
        else
        {
            Assert::CodingError("Unkonwn IPC message for RA {0} {1}", message.MessageId, message.Action);
        }
    }
    else if (metadata->Target == MessageTarget::FT)
    {
        ProcessFTMessage(metadata, move(msgPtr), GenerationHeader(), ReconfigurationAgent::InvalidNode);
    }
    else
    {
        Assert::CodingError("Unknown target for IPC message {0} {1}", message.MessageId, message.Action);
    }
}

void MessageHandler::ProcessRequestResponseRequest(
    IMessageMetadata const * baseMetadata,
    MessageUPtr && requestPtr,
    Federation::RequestReceiverContextUPtr && context)
{
    ASSERT_IF(baseMetadata == nullptr, "Metadata cannot be null. Unknown message should be traced and dropped on receive when metadata is looked up");
    auto metadata = static_cast<Communication::MessageMetadata const *>(baseMetadata);

    Message & request = *requestPtr;
    wstring const & action = request.Action;

    if (!CanProcessMessageBasedOnNodeLifeCycle(*metadata))
    {
        RAEventSource::Events->MessageIgnore(ra_.NodeInstanceIdStr, InvalidNodeLifeCycleState, action, request.MessageId);
        ra_.Federation.CompleteRequestReceiverContext(*context, ErrorCode(ErrorCodeValue::RANotReadyForUse, StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_RA_NOT_READY_FOR_USE)));
        return;
    }

    if (metadata->Target == MessageTarget::RA)
    {
        if (action == RSMessage::GetQueryRequest().Action)
        {
            ra_.QueryHelperObj.ProcessMessage(*requestPtr, move(context));
        }
        else if (action == RSMessage::GetReportFaultRequest().Action)
        {
            ra_.ClientReportFaultRequestHandler(move(requestPtr), move(context));
        }
        else
        {
            Assert::CodingError("Unkonwn request reply message for RA {0} {1}", request.MessageId, request.Action);
        }
    }
    else if (metadata->Target == MessageTarget::FT)
    {
        Assert::TestAssert("Unknown request response message {0}", action);
    }
    else
    {
        Assert::TestAssert("Unknown request response message {0}", action);
    }
}

void MessageHandler::ProcessFTMessage(
    Reliability::ReconfigurationAgentComponent::Communication::MessageMetadata const * metadata,
    Transport::MessageUPtr && requestPtr,
    GenerationHeader const & generationHeader,
    Federation::NodeInstance const & from)
{
    auto jobItem = CreateJobItemForFTMessage(
        metadata,
        move(requestPtr),
        generationHeader,
        from);

    if (jobItem != nullptr)
    {
        ra_.JobQueueManager.ScheduleJobItem(move(jobItem));
    }
}

bool MessageHandler::CanProcessMessageBasedOnNodeLifeCycle(MessageMetadata const & metadata) const
{
    if (metadata.ProcessDuringNodeClose)
    {
        return ra_.StateObj.IsOpenOrClosing;
    }
    else
    {
        return ra_.IsOpen;
    }
}

EntityJobItemBaseSPtr MessageHandler::CreateJobItemForFTMessage(
    MessageMetadata const * metadata,
    Transport::MessageUPtr && msgPtr,
    GenerationHeader const & generationHeader,
    Federation::NodeInstance const & from)
{
    wstring const & action = msgPtr->Action;
    EntityJobItemBaseSPtr jobItem;
    StringLiteral const * dropReason = nullptr;

    // These are received from FM
    if (action == RSMessage::GetAddInstance().Action)
    {
        dropReason = TryCreateJobItem<ReplicaMessageBody, FailoverUnit>(
            ra_,
            metadata,
            *msgPtr,
            generationHeader,
            from,
            &ReconfigurationAgent::AddInstanceMessageProcessor,
            jobItem);
    }
    else if (action == RSMessage::GetRemoveInstance().Action)
    {
        dropReason = TryCreateJobItem<DeleteReplicaMessageBody, FailoverUnit>(
            ra_,
            metadata,
            *msgPtr,
            generationHeader,
            from,
            &ReconfigurationAgent::RemoveInstanceMessageProcessor,
            jobItem);
    }
    else if (action == RSMessage::GetAddPrimary().Action)
    {
        dropReason = TryCreateJobItem<ReplicaMessageBody, FailoverUnit>(
            ra_,
            metadata,
            *msgPtr,
            generationHeader,
            from,
            &ReconfigurationAgent::AddPrimaryMessageProcessor,
            jobItem);
    }
    else if (action == RSMessage::GetAddReplica().Action)
    {
        dropReason = TryCreateJobItem<ReplicaMessageBody, FailoverUnit>(
            ra_,
            metadata,
            *msgPtr,
            generationHeader,
            from,
            &ReconfigurationAgent::AddReplicaMessageProcessor,
            jobItem);
    }
    else if (action == RSMessage::GetRemoveReplica().Action)
    {
        dropReason = TryCreateJobItem<ReplicaMessageBody, FailoverUnit>(
            ra_,
            metadata,
            *msgPtr,
            generationHeader,
            from,
            &ReconfigurationAgent::RemoveReplicaMessageProcessor,
            jobItem);
    }
    else if (action == RSMessage::GetDoReconfiguration().Action)
    {
        dropReason = TryCreateJobItem<DoReconfigurationMessageBody, FailoverUnit>(
            ra_,
            metadata,
            *msgPtr,
            generationHeader,
            from,
            &ReconfigurationAgent::DoReconfigurationMessageProcessor,
            jobItem);
    }
    else if (action == RSMessage::GetDeleteReplica().Action)
    {
        dropReason = TryCreateJobItem<DeleteReplicaMessageBody, FailoverUnit>(
            ra_,
            metadata,
            *msgPtr,
            generationHeader,
            from,
            &ReconfigurationAgent::DeleteReplicaMessageProcessor,
            jobItem);
    }
    else if (action == RSMessage::GetReplicaEndpointUpdatedReply().Action)
    {
        dropReason = TryCreateJobItem<ReplicaReplyMessageBody, FailoverUnit>(
            ra_,
            metadata,
            *msgPtr,
            generationHeader,
            from,
            &ReconfigurationAgent::ReplicaEndpointUpdatedReplyMessageProcessor,
            jobItem);
    }

    // These are received from Primary RA
    else if (action == RSMessage::GetCreateReplica().Action)
    {
        dropReason = TryCreateJobItem<RAReplicaMessageBody, FailoverUnit>(
            ra_,
            metadata,
            *msgPtr,
            generationHeader,
            from,
            &ReconfigurationAgent::CreateReplicaMessageProcessor,
            jobItem);
    }
    else if (action == RSMessage::GetGetLSN().Action)
    {
        dropReason = TryCreateJobItem<ReplicaMessageBody, FailoverUnit>(
            ra_,
            metadata,
            *msgPtr,
            generationHeader,
            from,
            &ReconfigurationAgent::GetLSNMessageProcessor,
            jobItem);
    }
    else if (action == RSMessage::GetDeactivate().Action)
    {
        dropReason = TryCreateJobItem<DeactivateMessageBody, FailoverUnit>(
            ra_,
            metadata,
            *msgPtr,
            generationHeader,
            from,
            &ReconfigurationAgent::DeactivateMessageProcessor,
            jobItem);
    }
    else if (action == RSMessage::GetActivate().Action)
    {
        dropReason = TryCreateJobItem<ActivateMessageBody, FailoverUnit>(
            ra_,
            metadata,
            *msgPtr,
            generationHeader,
            from,
            &ReconfigurationAgent::ActivateMessageProcessor,
            jobItem);
    }

    // These are received from Secondary/Idle RA
    else if (action == RSMessage::GetCreateReplicaReply().Action)
    {
        dropReason = TryCreateJobItem<ReplicaReplyMessageBody, FailoverUnit>(
            ra_,
            metadata,
            *msgPtr,
            generationHeader,
            from,
            &ReconfigurationAgent::CreateReplicaReplyMessageProcessor,
            jobItem);
    }
    else if (action == RSMessage::GetGetLSNReply().Action)
    {
        dropReason = TryCreateJobItem<GetLSNReplyMessageBody, FailoverUnit>(
            ra_,
            metadata,
            *msgPtr,
            generationHeader,
            from,
            &ReconfigurationAgent::GetLSNReplyMessageProcessor,
            jobItem);
    }
    else if (action == RSMessage::GetDeactivateReply().Action)
    {
        dropReason = TryCreateJobItem<ReplicaReplyMessageBody, FailoverUnit>(
            ra_,
            metadata,
            *msgPtr,
            generationHeader,
            from,
            &ReconfigurationAgent::DeactivateReplyMessageProcessor,
            jobItem);
    }
    else if (action == RSMessage::GetActivateReply().Action)
    {
        dropReason = TryCreateJobItem<ReplicaReplyMessageBody, FailoverUnit>(
            ra_,
            metadata,
            *msgPtr,
            generationHeader,
            from,
            &ReconfigurationAgent::ActivateReplyMessageProcessor,
            jobItem);
    }
    else if (action == RAMessage::GetReportFault().Action)
    {
        dropReason = TryCreateJobItem<ReportFaultMessageBody, FailoverUnit>(
            ra_,
            metadata,
            *msgPtr,
            generationHeader,
            from,
            &ReconfigurationAgent::ReportFaultMessageHandler,
            jobItem);
    }
    else if (action == RAMessage::GetReplicaOpenReply().Action)
    {
        dropReason = TryCreateJobItem<ProxyReplyMessageBody, FailoverUnit>(
            ra_,
            metadata,
            *msgPtr,
            generationHeader,
            from,
            &ReconfigurationAgent::ReplicaOpenReplyHandler,
            jobItem);
    }
    else if (action == RAMessage::GetReplicaCloseReply().Action)
    {
        dropReason = TryCreateJobItem<ProxyReplyMessageBody, FailoverUnit>(
            ra_,
            metadata,
            *msgPtr,
            generationHeader,
            from,
            &ReconfigurationAgent::ReplicaCloseReplyHandler,
            jobItem);
    }
    else if (action == RAMessage::GetStatefulServiceReopenReply().Action)
    {
        dropReason = TryCreateJobItem<ProxyReplyMessageBody, FailoverUnit>(
            ra_,
            metadata,
            *msgPtr,
            generationHeader,
            from,
            &ReconfigurationAgent::StatefulServiceReopenReplyHandler,
            jobItem);
    }
    else if (action == RAMessage::GetUpdateConfigurationReply().Action)
    {
        dropReason = TryCreateJobItem<ProxyReplyMessageBody, FailoverUnit>(
            ra_,
            metadata,
            *msgPtr,
            generationHeader,
            from,
            &ReconfigurationAgent::UpdateConfigurationReplyHandler,
            jobItem);
    }
    else if (action == RAMessage::GetReplicatorBuildIdleReplicaReply().Action)
    {
        dropReason = TryCreateJobItem<ProxyReplyMessageBody, FailoverUnit>(
            ra_,
            metadata,
            *msgPtr,
            generationHeader,
            from,
            &ReconfigurationAgent::ReplicatorBuildIdleReplicaReplyHandler,
            jobItem);
    }
    else if (action == RAMessage::GetReplicatorRemoveIdleReplicaReply().Action)
    {
        dropReason = TryCreateJobItem<ProxyReplyMessageBody, FailoverUnit>(
            ra_,
            metadata,
            *msgPtr,
            generationHeader,
            from,
            &ReconfigurationAgent::ReplicatorRemoveIdleReplicaReplyHandler,
            jobItem);
    }
    else if (action == RAMessage::GetReplicatorGetStatusReply().Action)
    {
        dropReason = TryCreateJobItem<ProxyReplyMessageBody, FailoverUnit>(
            ra_,
            metadata,
            *msgPtr,
            generationHeader,
            from,
            &ReconfigurationAgent::ReplicatorGetStatusMessageHandler,
            jobItem);
    }
    else if (action == RAMessage::GetReplicatorUpdateEpochAndGetStatusReply().Action)
    {
        dropReason = TryCreateJobItem<ProxyReplyMessageBody, FailoverUnit>(
            ra_,
            metadata,
            *msgPtr,
            generationHeader,
            from,
            &ReconfigurationAgent::ReplicatorGetStatusMessageHandler,
            jobItem);
    }
    else if (action == RAMessage::GetCancelCatchupReplicaSetReply().Action)
    {
        dropReason = TryCreateJobItem<ProxyReplyMessageBody, FailoverUnit>(
            ra_,
            metadata,
            *msgPtr,
            generationHeader,
            from,
            &ReconfigurationAgent::ReplicatorCancelCatchupReplicaSetReplyHandler,
            jobItem);
    }
    else if (action == RAMessage::GetUpdateServiceDescriptionReply().Action)
    {
        dropReason = TryCreateJobItem<ProxyUpdateServiceDescriptionReplyMessageBody, FailoverUnit>(
            ra_,
            metadata,
            *msgPtr,
            generationHeader,
            from,
            &ReconfigurationAgent::ProxyUpdateServiceDescriptionReplyMessageHandler,
            jobItem);
    }
    else if (action == RAMessage::GetReadWriteStatusRevokedNotification().Action)
    {
        dropReason = TryCreateJobItem<ReplicaMessageBody, FailoverUnit>(
            ra_,
            metadata,
            *msgPtr,
            generationHeader,
            from,
            &ReconfigurationAgent::ReadWriteStatusRevokedNotificationMessageHandler,
            jobItem);
    }
    else if (action == RAMessage::GetReplicaEndpointUpdated().Action)
    {
        dropReason = TryCreateJobItem<ReplicaMessageBody, FailoverUnit>(
            ra_,
            metadata,
            *msgPtr,
            generationHeader,
            from,
            &ReconfigurationAgent::ProxyReplicaEndpointUpdatedMessageHandler,
            jobItem);
    }
    else
    {
        Assert::CodingError("Unknown message at RA which was not caught by metadata checks {0}", action);
    }

    if (dropReason != nullptr)
    {
        RAEventSource::Events->MessageIgnore(
            ra_.NodeInstanceIdStr,
            *dropReason,
            msgPtr->Action,
            msgPtr->MessageId);
    }

    return jobItem;
}

bool MessageHandler::ReadAndCheckGenerationHeader(
    Message & message,
    Federation::NodeInstance const & from,
    __out GenerationHeader & generationHeader)
{
    generationHeader = GenerationHeader::ReadFromMessage(message);

    return ra_.GenerationStateManagerObj.CheckGenerationHeader(generationHeader, message.Action, from);
}
