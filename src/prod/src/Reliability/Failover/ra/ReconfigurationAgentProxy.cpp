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
using namespace ServiceModel;

wstring const MessageQueueNameSuffix(L"message");

IReconfigurationAgentProxyUPtr Reliability::ReconfigurationAgentComponent::ReconfigurationAgentProxyFactory(ReconfigurationAgentProxyConstructorParameters & parameters) 
{
    return make_unique<ReconfigurationAgentProxy>(parameters);
}

ReconfigurationAgentProxy::ReconfigurationAgentProxy(ReconfigurationAgentProxyConstructorParameters & parameters)
    : applicationHost_(*parameters.ApplicationHost),
      ipcClient_(*parameters.IpcClient),
      replicatorFactory_(*parameters.ReplicatorFactory),
      transactionalReplicatorFactory_(*parameters.TransactionalReplicatorFactory),
      Common::RootedObject(*parameters.Root),
      isCloseAlreadyCalled_(false),
      lfuLoadReportingComponent_(make_unique<LocalLoadReportingComponent>(*parameters.Root, *parameters.IpcClient)),
      lfuMessageSenderComponent_(make_unique<LocalMessageSenderComponent>(*parameters.Root, *parameters.IpcClient)),
      lfuHealthReportingComponent_(make_unique<LocalHealthReportingComponent>(*parameters.Root, move(parameters.IpcHealthReportingClient))),
      throttledHealthReportingClient_(parameters.ThrottledIpcHealthReportingClient),
      lfuProxyMap_(make_unique<LocalFailoverUnitProxyMap>(*parameters.Root))
{
}

ReconfigurationAgentProxy::~ReconfigurationAgentProxy()
{
    RAPEventSource::Events->LifeCycleDestructor(id_);
}

// Open the RA-proxy
AsyncOperationSPtr ReconfigurationAgentProxy::OnBeginOpen(
    TimeSpan timeout,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    UNREFERENCED_PARAMETER(timeout);

    wstring nodeIdString, hostIdString, nodeName;
    uint64 nodeInstanceId = 0;

    auto error = applicationHost_.GetHostInformation(nodeIdString, nodeInstanceId, hostIdString, nodeName);
    if(!error.IsSuccess())
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, parent);        
    }       

    Federation::NodeId nodeId;
    if (!Federation::NodeId::TryParse(nodeIdString, nodeId))
    {
        Assert::CodingError("Node Id {0} must be parseable", nodeIdString);
    }

    Guid hostId(hostIdString);
    id_ = ReconfigurationAgentProxyId(nodeId, hostId);

    LocalLoadReportingComponentObj.Open(id_);
    LocalMessageSenderComponentObj.Open(id_);
    LocalHealthReportingComponentObj.Open(id_, Federation::NodeInstance(nodeId, nodeInstanceId), nodeName);
    int threadCount = FailoverConfig::GetConfig().RAPMessageProcessingQueueThreadCount;
    std::wstring queueName = nodeIdString + L"_" + hostIdString + MessageQueueNameSuffix;
    messageQueue_ = make_shared<RAPCommonJobQueue>(queueName, *this, false /*forceEnqueue*/, threadCount);

    LocalFailoverUnitProxyMapObj.Open(id_);

    // Register message handler so it can receive messages from the RA
    auto root = Root.CreateComponentRoot();
    ipcClient_.RegisterMessageHandler( 
        RAMessage::Actor,
        [this, root] (MessageUPtr & message, IpcReceiverContextUPtr & request) 
        { 
            wstring const & action = message->Action;

            if (State.Value != FabricComponentState::Opened)
            {
                // Ignore message, not open
                RAPEventSource::Events->MessageDropNotOpen(id_, action);
                return;
            }

            auto messageClone = message->Clone();

            // RAP does not use message metadata
            MessageProcessingJobItem<ReconfigurationAgentProxy> context(nullptr, move(messageClone), move(request));
            if (!messageQueue_->Enqueue(move(context)))
            {
                RAPEventSource::Events->MessageDropEnqueueFailed(id_, action);
                return;
            }
        },
        true/*dispatchOnTransportThread*/);

    RAPEventSource::Events->LifeCycleOpened(id_);

    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(callback, parent);
}

ErrorCode ReconfigurationAgentProxy::OnEndOpen(AsyncOperationSPtr const & openAsyncOperation)
{
    return CompletedAsyncOperation::End(openAsyncOperation);
}

AsyncOperationSPtr ReconfigurationAgentProxy::OnBeginClose(
    TimeSpan timeout,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    UNREFERENCED_PARAMETER(timeout);

    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(*this, callback, parent);
}

ErrorCode ReconfigurationAgentProxy::OnEndClose(AsyncOperationSPtr const & closeAsyncOperation)
{
    return CloseAsyncOperation::End(closeAsyncOperation);
}

void ReconfigurationAgentProxy::OnAbort()
{
    // We need to ensure that all of the LFUPM entries get destructed when they fall out of scope
    // However, we are aborting so we will simply release services, replicators, etc. w/o closing

    RAPEventSource::Events->LifeCycleAborting(id_);

    ipcClient_.UnregisterMessageHandler(RAMessage::Actor);

    if (messageQueue_)
    {
        messageQueue_->Close();
    }

    LocalLoadReportingComponentObj.Close();
    LocalMessageSenderComponentObj.Close();
    LocalHealthReportingComponentObj.Close();

    map<FailoverUnitId, FailoverUnitProxySPtr> lfupmContents(LocalFailoverUnitProxyMapObj.PrivatizeLFUPM());

    Common::atomic_uint64 outstandingOperations(1);
    ManualResetEvent completedEvent(false);
    for(auto iter = lfupmContents.begin(); iter != lfupmContents.end(); ++iter)
    {
        FailoverUnitProxySPtr fup = iter->second;

        outstandingOperations++;
        fup->BeginMarkForCloseAndDrainOperations(
            true,
            [fup, &outstandingOperations, &completedEvent](AsyncOperationSPtr const & asyncOperation)
            {
                ErrorCode error = fup->EndMarkForCloseAndDrainOperations(asyncOperation);
                ASSERT_IFNOT(error.IsSuccess(), "MarkForCloseAndDrainOperations with abort should not fail");

                // We assume Abort is non-blocking so it is ok to call in Callback thread
                fup->Abort(false /* keepFUPopen */);
                
                if((--outstandingOperations) == 0)
                {
                    completedEvent.Set();
                }
            },
            this->Root.CreateAsyncOperationRoot());
    }

    if ((--outstandingOperations) != 0)
    {
        completedEvent.WaitOne();
    }
}

void ReconfigurationAgentProxy::ProcessIpcMessage(
    IMessageMetadata const *, 
    Transport::MessageUPtr && message, 
    Transport::IpcReceiverContextUPtr && receiverContext)
{  
    UNREFERENCED_PARAMETER(receiverContext);

    RAPEventSource::Events->ProxyMessageReceived(id_, message->Action, wformatString(message->MessageId));

    wstring const & action = message->Action;

    if (State.Value != FabricComponentState::Opened)
    {
        // Ignore message, not open
        RAPEventSource::Events->MessageDropNotOpen(id_, action);
        return;
    }
  
    auto context = FailoverUnitProxyContext<ProxyRequestMessageBody>::CreateContext(std::move(message), std::move(receiverContext), *this);
    
    if (context.IsInvalid)
    {
        return;
    }

    // Route message from RA to appropriate handler
    MessageUPtr outMsg;
    bool isRemoveFailoverUnitProxy = false;
    bool isRequestReplyMessage = false;

    if (action == RAMessage::GetReplicaOpen().Action)
    {
        ReplicaOpenMessageHandler(context, outMsg, isRemoveFailoverUnitProxy);
    }
    else if (action == RAMessage::GetReplicaClose().Action)
    {
        ReplicaCloseMessageHandler(context, outMsg, isRemoveFailoverUnitProxy);
    }
    else if (action == RAMessage::GetStatefulServiceReopen().Action)
    {
        StatefulServiceReopenMessageHandler(context, outMsg, isRemoveFailoverUnitProxy);
    }
    else if (action == RAMessage::GetUpdateConfiguration().Action)
    {
        UpdateConfigurationMessageHandler(context, outMsg);
    }
    else if (action == RAMessage::GetReplicatorBuildIdleReplica().Action)
    {
        ReplicatorBuildIdleReplicaMessageHandler(context, outMsg);
    }
    else if (action == RAMessage::GetReplicatorRemoveIdleReplica().Action)
    {
        ReplicatorRemoveIdleReplicaMessageHandler(context, outMsg);
    }
    else if (action == RAMessage::GetReplicatorGetStatus().Action)
    {
        ReplicatorGetStatusMessageHandler(context, outMsg);
    }
    else if (action == RAMessage::GetReplicatorUpdateEpochAndGetStatus().Action)
    {
        ReplicatorUpdateEpochAndGetStatusMessageHandler(context, outMsg);
    }
    else if (action == RAMessage::GetCancelCatchupReplicaSet().Action)
    {
        CancelCatchupReplicaSetMessageHandler(context, outMsg);
    }
    else if (action == RAMessage::GetReplicaEndpointUpdatedReply().Action)
    {
        ReplicaEndpointUpdatedReplyMessageHandler(context, outMsg);
    }
    else if (action == RAMessage::GetReadWriteStatusRevokedNotificationReply().Action)
    {
        ReadWriteStatusRevokedNotificationReplyMessageHandler(context, outMsg);
    }
    else if (action == RAMessage::GetRAPQuery().Action)
    {
        isRequestReplyMessage = true;
        QueryMessageHandler(context, outMsg);
    }
    else if (action == RAMessage::GetUpdateServiceDescription().Action)
    {
        UpdateServiceDescriptionMessageHandler(context, outMsg);
    }
    else
    {
        Assert::CodingError("Unknown action '{0}' for RA message into service host.", action);
    }

    if (isRemoveFailoverUnitProxy)
    {
        RemoveFailoverUnitProxy(context.FailoverUnitProxy);
    }

    if (isRequestReplyMessage)
    {
        SendReplyAndTrace(move(outMsg), context.ReceiverContext);
    }
    else
    {
        SendReplyAndTrace(move(outMsg));
    }
}

void ReconfigurationAgentProxy::ProcessRequestResponseRequest(
    IMessageMetadata const *,
    Transport::MessageUPtr &&, 
    Federation::RequestReceiverContextUPtr &&)
{
    Assert::CodingError("Unsuported message type!");
}

void ReconfigurationAgentProxy::ProcessRequest(
    IMessageMetadata const *,
    Transport::MessageUPtr &&, 
    Federation::OneWayReceiverContextUPtr && )
{
    Assert::CodingError("Unsuported message type!");
}

// Message handlers
void ReconfigurationAgentProxy::ReplicaOpenMessageHandler
    (FailoverUnitProxyContext<ProxyRequestMessageBody> & msgContext, __out MessageUPtr & reply, __out bool & isRemoveFailoverUnitProxy)
{
    reply = nullptr;
    isRemoveFailoverUnitProxy = false;

    ProxyActionsListTypes::Enum actionsList;

    if (msgContext.Body.ServiceDescription.IsStateful)
    {
        if (msgContext.Body.LocalReplicaDescription.CurrentConfigurationRole == ReplicaRole::Primary)
        {
            actionsList = ProxyActionsListTypes::StatefulServiceOpenPrimary;
        }
        else if (msgContext.Body.LocalReplicaDescription.CurrentConfigurationRole == ReplicaRole::Idle)
        {
            actionsList = ProxyActionsListTypes::StatefulServiceOpenIdle;
        }
        else
        {
            Assert::CodingError("unexpected replica role {0}", msgContext.Body);
        }
    }
    else
    {
        actionsList = ProxyActionsListTypes::StatelessServiceOpen;
    }

    ASSERT_IF(msgContext.Body.LocalReplicaDescription.InstanceId <= Constants::InvalidReplicaId, "Invalid instance id during open");

    {
        LockedFailoverUnitProxyPtr lockedFailoverUnitProxyPtr(msgContext.FailoverUnitProxy);

        if (lockedFailoverUnitProxyPtr->IsDeleted)
        {
            return;
        }

        if (lockedFailoverUnitProxyPtr->IsClosed)
        {
            if (lockedFailoverUnitProxyPtr->ReplicaDescription.InstanceId >= msgContext.Body.LocalReplicaDescription.InstanceId)
            {
                // Open retry for an earlier failed open, stale message
                FailoverUnitDescription replyFuDesc = msgContext.Body.FailoverUnitDescription;
                ReplicaDescription replyLocalReplica = msgContext.Body.LocalReplicaDescription;
                std::vector<ReplicaDescription> replyRemoteReplicas = msgContext.Body.RemoteReplicaDescriptions;

                // Include the service location and replication endpoint since they may be unknown to RA
                // INclude the flags because they are used by RA to determine whether to drop the replica or not
                replyLocalReplica.ServiceLocation = lockedFailoverUnitProxyPtr->ReplicaDescription.ServiceLocation;
                replyLocalReplica.ReplicationEndpoint = lockedFailoverUnitProxyPtr->ReplicaDescription.ReplicationEndpoint;
                ProxyReplyMessageBody replyBody(replyFuDesc, replyLocalReplica, move(replyRemoteReplicas), ProxyErrorCode::CreateRAPError(ErrorCodeValue::ObjectClosed), msgContext.Body.Flags);

                reply = RAMessage::GetReplicaOpenReply().CreateMessage<ProxyReplyMessageBody>(replyBody, replyBody.FailoverUnitDescription.FailoverUnitId.Guid);
    
                lockedFailoverUnitProxyPtr->Cleanup();
                isRemoveFailoverUnitProxy = true;
                return;
            }
        }
        else if (lockedFailoverUnitProxyPtr->IsOpened)
        {
            if (lockedFailoverUnitProxyPtr->ReplicaDescription.InstanceId == msgContext.Body.LocalReplicaDescription.InstanceId)
            {
                if (!lockedFailoverUnitProxyPtr->IsStatefulService ||
                    (lockedFailoverUnitProxyPtr->CurrentServiceRole == msgContext.Body.LocalReplicaDescription.CurrentConfigurationRole && 
                    lockedFailoverUnitProxyPtr->CurrentReplicatorRole == msgContext.Body.LocalReplicaDescription.CurrentConfigurationRole))
                {
                    // Duplicate open, ACK immediately
                    FailoverUnitDescription replyFuDesc = msgContext.Body.FailoverUnitDescription;
                    ReplicaDescription replyLocalReplica = msgContext.Body.LocalReplicaDescription;
                    std::vector<ReplicaDescription> replyRemoteReplicas = msgContext.Body.RemoteReplicaDescriptions;

                    // Include the service location and replication endpoint since they may be unknown to RA
                    replyLocalReplica.ServiceLocation = lockedFailoverUnitProxyPtr->ReplicaDescription.ServiceLocation;
                    replyLocalReplica.ReplicationEndpoint = lockedFailoverUnitProxyPtr->ReplicaDescription.ReplicationEndpoint;
                    ProxyReplyMessageBody replyBody(replyFuDesc, replyLocalReplica, move(replyRemoteReplicas), ProxyErrorCode::Success(), msgContext.Body.Flags);

                    reply = RAMessage::GetReplicaOpenReply().CreateMessage<ProxyReplyMessageBody>(replyBody, replyBody.FailoverUnitDescription.FailoverUnitId.Guid);
                    return;
                }
                else if((lockedFailoverUnitProxyPtr->CurrentServiceRole == ReplicaRole::Secondary ||
                         lockedFailoverUnitProxyPtr->CurrentServiceRole == ReplicaRole::Primary ||
                         lockedFailoverUnitProxyPtr->CurrentReplicatorRole == ReplicaRole::Secondary ||
                         lockedFailoverUnitProxyPtr->CurrentReplicatorRole == ReplicaRole::Primary) && 
                        msgContext.Body.LocalReplicaDescription.CurrentConfigurationRole == ReplicaRole::Idle)
                {
                    // Stale open message, ignore
                    return;
                }
            }
            else if (lockedFailoverUnitProxyPtr->ReplicaDescription.InstanceId < msgContext.Body.LocalReplicaDescription.InstanceId)
            {
                Assert::CodingError("Trying to open replica with higher instance while the fup is already open.");
            }
            else
            {
                // Stale message or request for re-open before close
                // Either way, drop
                return;
            }
        }

        // If this is the creation of the primary then service availability is impacted by slow API
        bool impactsServiceAvailability = msgContext.Body.LocalReplicaDescription.CurrentConfigurationRole == ReplicaRole::Primary;

        if (!lockedFailoverUnitProxyPtr->TryAddToCurrentlyExecutingActionsLists(actionsList, impactsServiceAvailability, msgContext))
        {
            // Incompatible work in progress, so drop the message (it will be re-sent by RA)
            RAPEventSource::Events->MessageDropInvalidWorkInProgress(id_, msgContext.Action);
            return;
        }

        if (!msgContext.Body.ServiceDescription.HasPersistedState)
        {
            // Open Mode will be New for volatile/stateless
            lockedFailoverUnitProxyPtr->ReplicaOpenMode = ReplicaOpenMode::New;
        }
        else if (lockedFailoverUnitProxyPtr->IsClosed && 
                 lockedFailoverUnitProxyPtr->ReplicaDescription.ReplicaId != msgContext.Body.LocalReplicaDescription.ReplicaId)
        {
            // For Persisted service if the ReplicaOpen is processed for the very first time then open mode is New
            // After that as soon as OpenAsync completes then replica will transition to OpenMode existing
            // so even if changerole fails the open mode will be managed correctly
            // If OpenAsync fails then the replica open mode will be New as it would not have been changed
            lockedFailoverUnitProxyPtr->ReplicaOpenMode = ReplicaOpenMode::New;
        }

        // It is important to update the service description here as it may have changed on RA
        lockedFailoverUnitProxyPtr->State = FailoverUnitProxyStates::Opening;
        lockedFailoverUnitProxyPtr->ServiceDescription = msgContext.Body.ServiceDescription;

        // Preserve the replication endpoint and service location already on the FUP since, if this is an open 
        // after stateful service restart, RAP will already have reopened the replicator/service whereas RA will not yet have the 
        // replication endpoint / service location
        ReplicaDescription replicaDesc = msgContext.Body.LocalReplicaDescription;
        replicaDesc.ReplicationEndpoint = lockedFailoverUnitProxyPtr->ReplicaDescription.ReplicationEndpoint;
        replicaDesc.ServiceLocation = lockedFailoverUnitProxyPtr->ReplicaDescription.ServiceLocation;
        lockedFailoverUnitProxyPtr->ReplicaDescription = replicaDesc;
        
        lockedFailoverUnitProxyPtr->FailoverUnitDescription.PreviousConfigurationEpoch = msgContext.Body.FailoverUnitDescription.PreviousConfigurationEpoch;
        lockedFailoverUnitProxyPtr->FailoverUnitDescription.CurrentConfigurationEpoch = msgContext.Body.FailoverUnitDescription.CurrentConfigurationEpoch;
    }

    ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::CreateAndStart(
        *this,
        std::move(msgContext),
        actionsList,
        [this](AsyncOperationSPtr const & operation) { FinishReplicaOpenMessageHandling(operation); });
}

void ReconfigurationAgentProxy::ReplicaCloseMessageHandler(
    FailoverUnitProxyContext<ProxyRequestMessageBody> & msgContext, __out MessageUPtr & reply, __out bool & isRemoveFailoverUnitProxy)
{
    reply = nullptr;
    isRemoveFailoverUnitProxy = false;

    ProxyActionsListTypes::Enum actionsList;
    bool isAbort = (msgContext.Body.Flags & ProxyMessageFlags::Abort) != 0;
    bool isDrop = (msgContext.Body.Flags & ProxyMessageFlags::Drop) != 0;

    bool cancelNeeded = false;

    ProxyRequestMessageBody const & body = msgContext.Body;
    
    {
        LockedFailoverUnitProxyPtr lockedFailoverUnitProxyPtr(msgContext.FailoverUnitProxy);

        if (lockedFailoverUnitProxyPtr->IsDeleted)
        {
            return;
        }

        if (body.ServiceDescription.IsStateful)
        {
            // If the service and replicator roles are mismatch we execute Abort
            actionsList = isAbort ? ProxyActionsListTypes::StatefulServiceAbort : 
                            isDrop ? ProxyActionsListTypes::StatefulServiceDrop : ProxyActionsListTypes::StatefulServiceClose;
        }
        else
        {
            // Close stateless service
            actionsList = isAbort ? ProxyActionsListTypes::StatelessServiceAbort : ProxyActionsListTypes::StatelessServiceClose;
        }

        if (lockedFailoverUnitProxyPtr->IsClosed && 
            msgContext.WasFailoverUnitProxyCreated &&
            (isDrop || isAbort) &&
            body.ServiceDescription.HasPersistedState)
        {
            // This LockedFT was created as a result of processing the message
            // This code path handles the scenario where RA is trying to drop a SP replica that
            // was already in drop before the node or apphost went down
            lockedFailoverUnitProxyPtr->State = FailoverUnitProxyStates::Opening;

            // It is important to update the service description here as it may have changed on RA
            lockedFailoverUnitProxyPtr->ServiceDescription = body.ServiceDescription;
            lockedFailoverUnitProxyPtr->ReplicaDescription = body.LocalReplicaDescription;

            lockedFailoverUnitProxyPtr->FailoverUnitDescription.PreviousConfigurationEpoch = msgContext.Body.FailoverUnitDescription.PreviousConfigurationEpoch;
            lockedFailoverUnitProxyPtr->FailoverUnitDescription.CurrentConfigurationEpoch = msgContext.Body.FailoverUnitDescription.CurrentConfigurationEpoch;

            // Set open mode
            lockedFailoverUnitProxyPtr->ReplicaOpenMode = ReplicaOpenMode::Existing;

            // Set current replica state
            lockedFailoverUnitProxyPtr->CurrentReplicaState = ReplicaStates::FromReplicaDescriptionState(msgContext.Body.LocalReplicaDescription.State);
        }
        else if (lockedFailoverUnitProxyPtr->IsClosed)
        {
            // Duplicate close, ACK immediately
            std::vector<ReplicaDescription> replyRemoteReplicas = body.RemoteReplicaDescriptions;

            ProxyReplyMessageBody replyBody(
                body.FailoverUnitDescription, 
                body.LocalReplicaDescription,
                move(replyRemoteReplicas), 
                ProxyErrorCode::Success());

            reply = RAMessage::GetReplicaCloseReply().CreateMessage<ProxyReplyMessageBody>(replyBody, replyBody.FailoverUnitDescription.FailoverUnitId.Guid);

            if (body.LocalReplicaDescription.InstanceId > lockedFailoverUnitProxyPtr->ReplicaDescription.InstanceId)
            {
                // Make sure we have the right instance id to be able to do staleness check later
                ReplicaDescription replicaDesc = body.LocalReplicaDescription;
                lockedFailoverUnitProxyPtr->ReplicaDescription = replicaDesc;
            }

            lockedFailoverUnitProxyPtr->Cleanup();
            isRemoveFailoverUnitProxy = true;
            return;
        }

        if (!lockedFailoverUnitProxyPtr->TryAddToCurrentlyExecutingActionsLists(actionsList, true, msgContext, cancelNeeded))
        {
            if (!cancelNeeded)
            {
                // Incompatible work in progress, so drop the message (it will be re-sent by RA)
                RAPEventSource::Events->MessageDropInvalidWorkInProgress(id_, msgContext.Action);
                return;
            }
        }
        else
        {
            lockedFailoverUnitProxyPtr->State = FailoverUnitProxyStates::Closing;
            ReplicaDescription replicaDesc = body.LocalReplicaDescription;

            if (isDrop || isAbort)
            {
                replicaDesc.CurrentConfigurationRole = Reliability::ReplicaRole::None;
            }

            lockedFailoverUnitProxyPtr->ReplicaDescription = replicaDesc;
        }
    }

    if (cancelNeeded)
    {
        RAPEventSource::Events->CancelPendingOperations(id_, msgContext.Action);
        // Execute cancel outside lock
        msgContext.FailoverUnitProxy->CancelOperations();
        return;
    }

    ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::CreateAndStart(
        *this,
        std::move(msgContext),
        actionsList,
        [this](AsyncOperationSPtr const & operation) { FinishReplicaCloseMessageHandling(operation); },
        isAbort /* continueOnFailure */);
}

void ReconfigurationAgentProxy::StatefulServiceReopenMessageHandler(
    FailoverUnitProxyContext<ProxyRequestMessageBody> & msgContext, __out MessageUPtr & reply, __out bool & isRemoveFailoverUnitProxy)
{
    isRemoveFailoverUnitProxy = false;
    reply = nullptr;
    ProxyActionsListTypes::Enum actionsList = ProxyActionsListTypes::StatefulServiceReopen;
    bool cancelNeeded = false;

    ASSERT_IF(msgContext.Body.LocalReplicaDescription.InstanceId <= Constants::InvalidReplicaId, "Invalid instance id during reopen");

    {
        LockedFailoverUnitProxyPtr lockedFailoverUnitProxyPtr(msgContext.FailoverUnitProxy);

        if (lockedFailoverUnitProxyPtr->IsDeleted)
        {
            return;
        }

        if (lockedFailoverUnitProxyPtr->IsClosed)
        {
            if (lockedFailoverUnitProxyPtr->ReplicaDescription.InstanceId >= msgContext.Body.LocalReplicaDescription.InstanceId)
            {
                // Reopen retry for an earlier failed reopen
                FailoverUnitDescription replyFuDesc = msgContext.Body.FailoverUnitDescription;
                ReplicaDescription replyLocalReplica = msgContext.Body.LocalReplicaDescription;
                std::vector<ReplicaDescription> replyRemoteReplicas = msgContext.Body.RemoteReplicaDescriptions;

                // Include the service location and replication endpoint since they may be unknown to RA
                replyLocalReplica.ServiceLocation = lockedFailoverUnitProxyPtr->ReplicaDescription.ServiceLocation;
                replyLocalReplica.ReplicationEndpoint = lockedFailoverUnitProxyPtr->ReplicaDescription.ReplicationEndpoint;

                ProxyReplyMessageBody replyBody(replyFuDesc, replyLocalReplica, move(replyRemoteReplicas), ProxyErrorCode::CreateRAPError(ErrorCodeValue::ObjectClosed), msgContext.Body.Flags);

                reply = RAMessage::GetStatefulServiceReopenReply().CreateMessage<ProxyReplyMessageBody>(replyBody, replyBody.FailoverUnitDescription.FailoverUnitId.Guid);

                lockedFailoverUnitProxyPtr->Cleanup();
                isRemoveFailoverUnitProxy = true;
                return;
            }
        }
        else if (lockedFailoverUnitProxyPtr->IsOpened && lockedFailoverUnitProxyPtr->IsServiceCreated && lockedFailoverUnitProxyPtr->IsReplicatorCreated &&
            lockedFailoverUnitProxyPtr->ReplicaDescription.InstanceId == msgContext.Body.LocalReplicaDescription.InstanceId)
        {
            // Duplicate re-open, ACK immediately
            FailoverUnitDescription replyFuDesc = msgContext.Body.FailoverUnitDescription;
            ReplicaDescription replyLocalReplica = msgContext.Body.LocalReplicaDescription;
            std::vector<ReplicaDescription> replyRemoteReplicas = msgContext.Body.RemoteReplicaDescriptions;

            // Include the service location and replication endpoint since they may be unknown to RA
            replyLocalReplica.ServiceLocation = lockedFailoverUnitProxyPtr->ReplicaDescription.ServiceLocation;
            replyLocalReplica.ReplicationEndpoint = lockedFailoverUnitProxyPtr->ReplicaDescription.ReplicationEndpoint;
                
            ProxyReplyMessageBody replyBody(replyFuDesc, replyLocalReplica, move(replyRemoteReplicas), ProxyErrorCode::Success(), msgContext.Body.Flags);

            reply = RAMessage::GetStatefulServiceReopenReply().CreateMessage<ProxyReplyMessageBody>(replyBody, replyBody.FailoverUnitDescription.FailoverUnitId.Guid);
            return;
        }
        else if ((lockedFailoverUnitProxyPtr->IsOpening || lockedFailoverUnitProxyPtr->IsClosing) &&
            lockedFailoverUnitProxyPtr->IsRunningActions && 
            lockedFailoverUnitProxyPtr->ReplicaDescription.InstanceId == msgContext.Body.LocalReplicaDescription.InstanceId)
        {
            // Stale message, ignore
            return;
        }

        if (!lockedFailoverUnitProxyPtr->TryAddToCurrentlyExecutingActionsLists(actionsList, false, msgContext, cancelNeeded))
        {
            if (!cancelNeeded)
            {
                // Incompatible work in progress, so drop the message (it will be re-sent by RA)
                RAPEventSource::Events->MessageDropInvalidWorkInProgress(id_, msgContext.Action);
                return;
            }
        }
        else
        {
            lockedFailoverUnitProxyPtr->State = FailoverUnitProxyStates::Opening;

            // It is important to update the service description here as it may have changed on RA
            lockedFailoverUnitProxyPtr->ServiceDescription = msgContext.Body.ServiceDescription;

            // Preserve the replication endpoint and service location already on the FUP since, if this is a duplicate reopen call
            // RAP will already have reopened the replicator and service
            ReplicaDescription replicaDesc = msgContext.Body.LocalReplicaDescription;
            replicaDesc.ReplicationEndpoint = lockedFailoverUnitProxyPtr->ReplicaDescription.ReplicationEndpoint;
            replicaDesc.ServiceLocation = lockedFailoverUnitProxyPtr->ReplicaDescription.ServiceLocation;
            lockedFailoverUnitProxyPtr->ReplicaDescription = replicaDesc;

            lockedFailoverUnitProxyPtr->FailoverUnitDescription.PreviousConfigurationEpoch = msgContext.Body.FailoverUnitDescription.PreviousConfigurationEpoch;
            lockedFailoverUnitProxyPtr->FailoverUnitDescription.CurrentConfigurationEpoch = msgContext.Body.FailoverUnitDescription.CurrentConfigurationEpoch;
        
            ASSERT_IFNOT(lockedFailoverUnitProxyPtr->ServiceDescription.HasPersistedState, "Reopen is called for non-persisted service.");

            // Set open mode
            lockedFailoverUnitProxyPtr->ReplicaOpenMode = ReplicaOpenMode::Existing;
        
            // Set current replica state
            lockedFailoverUnitProxyPtr->CurrentReplicaState = ReplicaStates::FromReplicaDescriptionState(msgContext.Body.LocalReplicaDescription.State);
        }
    }

    if (cancelNeeded)
    {
        // Execute cancel outside lock
        RAPEventSource::Events->CancelPendingOperations(id_, msgContext.Action);
        msgContext.FailoverUnitProxy->CancelOperations();
        return;
    }

    ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::CreateAndStart(
        *this,
        std::move(msgContext),
        actionsList,
        [this](AsyncOperationSPtr const & operation) { FinishStatefulServiceReopenMessageHandling(operation); });
}

bool ReconfigurationAgentProxy::DoesUpdateConfigurationMessageImpactServiceAvailability(ProxyRequestMessageBody const & body)
{
    // Determine if this message impacts service availability
    // This is used to decide whether actions that are performed by the FT while this action list is executing
    // use a smaller timeout for reporting a warning to HM 
    //
    // It is not enough to use the difference between the PC primary epoch and CC primary epoch here 
    // because there can be situations in which the UC message from RA contains the same epoch
    // 
    // Consider a reconfiguration (P/P r1 U), (S/S r2 U), (I/S r3 U) from epoch e11/e12
    // If the primary goes down the FM will do (P/I r1 D), (S/P r2 U), (I/S r3 U) from epoch e11/e23 
    // r2 will perform GetLSN which will cause r3 to set both PC and CC epoch to e23 
    // and the subsequent activate -> UC to RAP will contain pc and cc epoch e23
    //
    // Hence inspect the message body to see if there is a change in the primary
    if (body.LocalReplicaDescription.CurrentConfigurationRole != body.LocalReplicaDescription.PreviousConfigurationRole &&
        (body.LocalReplicaDescription.CurrentConfigurationRole == ReplicaRole::Primary || body.LocalReplicaDescription.PreviousConfigurationRole == ReplicaRole::Primary))
    {
        // local replica undergoing primary change
        return true;
    }

    for(auto it = body.RemoteReplicaDescriptions.cbegin(); it != body.RemoteReplicaDescriptions.cend(); ++it)
    {
        if (!it->IsUp)
        {
            continue;
        }

        if (it->PreviousConfigurationRole != it->CurrentConfigurationRole &&
            (it->PreviousConfigurationRole == ReplicaRole::Primary || it->CurrentConfigurationRole == ReplicaRole::Primary))
        {
            return true;
        }
    }

    return false;
}

namespace
{
    void CreateUpdateConfigurationReply(
        LockedFailoverUnitProxyPtr & lockedFailoverUnitProxyPtr, 
        ProxyRequestMessageBody const & body,
        ErrorCodeValue::Enum error,
        MessageUPtr & reply)
    {
        FailoverUnitDescription replyFuDesc = body.FailoverUnitDescription;
        ReplicaDescription replyLocalReplica = body.LocalReplicaDescription;
        std::vector<ReplicaDescription> replyRemoteReplicas = body.RemoteReplicaDescriptions;

        // Include the service location and replication endpoint since they may be unknown to RA
        replyLocalReplica.ServiceLocation = lockedFailoverUnitProxyPtr->ReplicaDescription.ServiceLocation;
        replyLocalReplica.ReplicationEndpoint = lockedFailoverUnitProxyPtr->ReplicaDescription.ReplicationEndpoint;

        // If the catchup result was state changed on data loss then include the lsn
        if (lockedFailoverUnitProxyPtr->CatchupResult == CatchupResult::DataLossReported)
        {
            replyLocalReplica.LastAcknowledgedLSN = lockedFailoverUnitProxyPtr->ReplicaDescription.LastAcknowledgedLSN;
        }

        ProxyReplyMessageBody replyBody(
            replyFuDesc,
            replyLocalReplica,
            move(replyRemoteReplicas),
            ProxyErrorCode::CreateRAPError(error),
            body.Flags);

        reply = RAMessage::GetUpdateConfigurationReply().CreateMessage<ProxyReplyMessageBody>(replyBody, body.FailoverUnitDescription.FailoverUnitId.Guid);
    }
}


void ReconfigurationAgentProxy::UpdateConfigurationMessageHandler(FailoverUnitProxyContext<ProxyRequestMessageBody> & msgContext, __out MessageUPtr & reply)
{ 
    reply = nullptr;
    ProxyActionsListTypes::Enum actionsList = ProxyActionsListTypes::Empty;

    bool doesMessageImpactServiceAvailability = DoesUpdateConfigurationMessageImpactServiceAvailability(msgContext.Body);

    {
        LockedFailoverUnitProxyPtr lockedFailoverUnitProxyPtr(msgContext.FailoverUnitProxy);

        if (lockedFailoverUnitProxyPtr->IsDeleted)
        {
            return;
        }

        ReplicaRole::Enum currentConfigurationRole = lockedFailoverUnitProxyPtr->CurrentConfigurationRole;
        bool isReplicatorRoleCurrent = (lockedFailoverUnitProxyPtr->CurrentReplicatorRole == currentConfigurationRole);
        bool isServiceRoleCurrent =  (lockedFailoverUnitProxyPtr->CurrentServiceRole == currentConfigurationRole);

        if (lockedFailoverUnitProxyPtr->IsCatchupCancel)
        { 
            if (lockedFailoverUnitProxyPtr->CurrentConfigurationEpoch < msgContext.Body.FailoverUnitDescription.CurrentConfigurationEpoch)
            {
                lockedFailoverUnitProxyPtr->DoneCancelReplicatorCatchupReplicaSet();
            }
            else
            {
                return;
            }
        }

        if (currentConfigurationRole == ReplicaRole::None)
        {
            return;
        }

        if (lockedFailoverUnitProxyPtr->CurrentConfigurationEpoch > msgContext.Body.FailoverUnitDescription.CurrentConfigurationEpoch)
        {
            return;
        }

        bool isEndReconfiguration = (msgContext.Body.Flags & ProxyMessageFlags::EndReconfiguration) != 0;
        bool isCatchup = (msgContext.Body.Flags & ProxyMessageFlags::Catchup) != 0;

        if (lockedFailoverUnitProxyPtr->CurrentConfigurationEpoch == msgContext.Body.FailoverUnitDescription.CurrentConfigurationEpoch &&
            ((lockedFailoverUnitProxyPtr->ConfigurationStage == ProxyConfigurationStage::CurrentPending || 
              lockedFailoverUnitProxyPtr->ConfigurationStage == ProxyConfigurationStage::Current) && isCatchup))
        {
            return;
        }

        if (lockedFailoverUnitProxyPtr->CurrentConfigurationEpoch < msgContext.Body.FailoverUnitDescription.CurrentConfigurationEpoch)
        {
            if (msgContext.Body.LocalReplicaDescription.CurrentConfigurationRole == ReplicaRole::Secondary && lockedFailoverUnitProxyPtr->IsPreWriteStatusCatchupEnabled)
            {
                lockedFailoverUnitProxyPtr->ConfigurationStage = ProxyConfigurationStage::PreWriteStatusRevokeCatchupPending;
            }
            else
            {
                lockedFailoverUnitProxyPtr->ConfigurationStage = ProxyConfigurationStage::CatchupPending;
            }
        }

        if (lockedFailoverUnitProxyPtr->IsCatchupPending)
        {
            // Catchup action list already pending, so just update list of replicas
            actionsList = ProxyActionsListTypes::ReplicatorUpdateReplicas;
        } 
        // Are we already in the local replica role specified in the new configuration?
        else if (currentConfigurationRole == msgContext.Body.LocalReplicaDescription.CurrentConfigurationRole && isServiceRoleCurrent && isReplicatorRoleCurrent)
        {
            if (lockedFailoverUnitProxyPtr->CurrentReplicatorRole == ReplicaRole::Primary)
            {
                if(isEndReconfiguration)
                {
                    actionsList = ProxyActionsListTypes::StatefulServiceEndReconfiguration;
                }
                else if (isCatchup)
                {
                    if (lockedFailoverUnitProxyPtr->CurrentConfigurationEpoch == msgContext.Body.FailoverUnitDescription.CurrentConfigurationEpoch)
                    {
                        // Handle the case where catchup may already be complete
                        switch (lockedFailoverUnitProxyPtr->CatchupResult)
                        {
                        case CatchupResult::NotStarted:
                            // Catchup not started or the action list is queued and executing
                            // Execute the catchup action list and each individual action will perform the correct work
                            actionsList = ProxyActionsListTypes::ReplicatorUpdateAndCatchupQuorum;
                            break;

                        case CatchupResult::CatchupCompleted:
                            // Catchup is already complete for this epoch
                            // Check if the replicas need to be updated
                            if (lockedFailoverUnitProxyPtr->IsConfigurationMessageBodyStaleCallerHoldsLock(msgContext.Body.RemoteReplicaDescriptions))
                            {
                                return;
                            }

                            if (!lockedFailoverUnitProxyPtr->CheckConfigurationMessageBodyForUpdatesCallerHoldsLock(msgContext.Body.RemoteReplicaDescriptions, false /* Apply */))
                            {
                                // This catchup operation has been completed 
                                // The UC body does not have any additional changes
                                // Reply can be sent
                                CreateUpdateConfigurationReply(lockedFailoverUnitProxyPtr, msgContext.Body, ErrorCodeValue::Success, reply);
                                return;
                            }
                            else
                            {
                                // Catchup for this config is already complete
                                // Simply update replicas
                                actionsList = ProxyActionsListTypes::ReplicatorUpdateReplicas;
                            }
                            break;

                        case CatchupResult::DataLossReported:
                            // No need to check for config body staleness here
                            // If we have invoked data loss and state changed for this epoch simply reply
                            CreateUpdateConfigurationReply(lockedFailoverUnitProxyPtr, msgContext.Body, ErrorCodeValue::RAProxyStateChangedOnDataLoss, reply);
                            return;

                        default:
                            Assert::CodingError("unknown value {0}", static_cast<int>(lockedFailoverUnitProxyPtr->CatchupResult));
                        }
                    }
                    else
                    {
                        // Update list of replicas and perform a quorum catchup
                        actionsList = ProxyActionsListTypes::ReplicatorUpdateAndCatchupQuorum;
                    }
                }
                else
                {
                    actionsList = ProxyActionsListTypes::ReplicatorUpdateReplicas;
                }
            }
            else
            {
                if (lockedFailoverUnitProxyPtr->CurrentReplicatorRole == ReplicaRole::Secondary &&
                    isCatchup)
                {
                    // The roles are in the intended state however the last step in demote to secondary (ReplicatorGetStatus)
                    // could have failed, re-run
                    actionsList = ProxyActionsListTypes::ReplicatorGetStatus;
                }
                else
                {
                    // Intent is achieved, ACK immediately
                    CreateUpdateConfigurationReply(lockedFailoverUnitProxyPtr, msgContext.Body, ErrorCodeValue::Success, reply);
                    return;
                }
            }
        }
        else if (msgContext.Body.LocalReplicaDescription.CurrentConfigurationRole == ReplicaRole::Primary)
        {
            TESTASSERT_IF(isEndReconfiguration, "UC with ER cannot be received for primary {0}", msgContext.Body);
            actionsList = ProxyActionsListTypes::StatefulServicePromoteToPrimary;
        }
        else if (msgContext.Body.LocalReplicaDescription.CurrentConfigurationRole == ReplicaRole::Secondary)
        {
            if (currentConfigurationRole == ReplicaRole::Primary)
            {
                actionsList = ProxyActionsListTypes::StatefulServiceDemoteToSecondary;
            }
            else if (currentConfigurationRole == ReplicaRole::Idle)
            {
                actionsList = ProxyActionsListTypes::StatefulServiceChangeRole;
            }
            else if (currentConfigurationRole == ReplicaRole::Secondary)
            {
                if (lockedFailoverUnitProxyPtr->CurrentReplicatorRole == ReplicaRole::Primary || 
                    lockedFailoverUnitProxyPtr->CurrentServiceRole == ReplicaRole::Primary)
                {
                    // Previous attempt at demote to secondary must have failed, retry
                    if (isReplicatorRoleCurrent)
                    {
                        actionsList = ProxyActionsListTypes::StatefulServiceFinishDemoteToSecondary;
                    }
                    else
                    {
                        actionsList = ProxyActionsListTypes::StatefulServiceDemoteToSecondary;
                    }
                }
                else if (lockedFailoverUnitProxyPtr->CurrentReplicatorRole == ReplicaRole::Idle ||
                    lockedFailoverUnitProxyPtr->CurrentServiceRole == ReplicaRole::Idle)
                {
                    // Previous I->S transition failed, retry
                    actionsList = ProxyActionsListTypes::StatefulServiceChangeRole;
                }
                else
                {
                    return;
                }
            }
        }

        ASSERT_IF(actionsList == ProxyActionsListTypes::Empty, 
            "UpdateConfigurationMessageHandler failed to select a valid action list for execution - configuration role {0}, replicator role {1}, service role {2}.", 
            lockedFailoverUnitProxyPtr->CurrentConfigurationRole, lockedFailoverUnitProxyPtr->CurrentReplicatorRole, lockedFailoverUnitProxyPtr->CurrentServiceRole);

        if (!lockedFailoverUnitProxyPtr->TryAddToCurrentlyExecutingActionsLists(actionsList, doesMessageImpactServiceAvailability, msgContext))
        {
            // Incompatible work in progress, so drop the message (it will be re-sent by RA)
            RAPEventSource::Events->MessageDropInvalidWorkInProgress(id_, msgContext.Action);
            return;
        }        

        lockedFailoverUnitProxyPtr->ProcessUpdateConfigurationMessage(msgContext.Body);
    }

    ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::CreateAndStart(
        *this,
        std::move(msgContext),
        actionsList,
        [this](AsyncOperationSPtr const & operation) { FinishUpdateConfigurationMessageHandling(operation); });
}

void ReconfigurationAgentProxy::ReplicatorBuildIdleReplicaMessageHandler(FailoverUnitProxyContext<ProxyRequestMessageBody> & msgContext, __out MessageUPtr & reply)
{ 
    reply = nullptr;
    ProxyActionsListTypes::Enum actionsList = ProxyActionsListTypes::ReplicatorBuildIdleReplica;
    
    {
        LockedFailoverUnitProxyPtr lockedFailoverUnitProxyPtr(msgContext.FailoverUnitProxy);

        if (lockedFailoverUnitProxyPtr->IsDeleted)
        {
            return;
        }

        if (!lockedFailoverUnitProxyPtr->TryAddToCurrentlyExecutingActionsLists(actionsList, false, msgContext))
        {
            // Incompatible work in progress, so drop the message (it will be re-sent by RA)
            RAPEventSource::Events->MessageDropInvalidWorkInProgress(id_, msgContext.Action);
            return;
        }
    }

    ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::CreateAndStart(
        *this,
        std::move(msgContext),
        actionsList,
        [this](AsyncOperationSPtr const & operation) { FinishReplicatorBuildIdleReplicaMessageHandling(operation); });
}

void ReconfigurationAgentProxy::ReplicatorRemoveIdleReplicaMessageHandler(FailoverUnitProxyContext<ProxyRequestMessageBody> & msgContext, __out MessageUPtr & reply)
{ 
    reply = nullptr;
    ProxyActionsListTypes::Enum actionsList = ProxyActionsListTypes::ReplicatorRemoveIdleReplica;

    {
        LockedFailoverUnitProxyPtr lockedFailoverUnitProxyPtr(msgContext.FailoverUnitProxy);

        if (lockedFailoverUnitProxyPtr->IsDeleted)
        {
            return;
        }

        if (!lockedFailoverUnitProxyPtr->TryAddToCurrentlyExecutingActionsLists(actionsList, false, msgContext))
        {
            // Incompatible work in progress, so drop the message (it will be re-sent by RA)
            RAPEventSource::Events->MessageDropInvalidWorkInProgress(id_, msgContext.Action);
            return;
        }
    }

    ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::CreateAndStart(
        *this,
        std::move(msgContext),
        actionsList,
        [this](AsyncOperationSPtr const & operation) { FinishReplicatorRemoveIdleReplicaMessageHandling(operation); });
}

void ReconfigurationAgentProxy::ReplicatorGetStatusMessageHandler(FailoverUnitProxyContext<ProxyRequestMessageBody> & msgContext, __out MessageUPtr & reply)
{ 
    reply = nullptr;
    ProxyActionsListTypes::Enum actionsList = ProxyActionsListTypes::ReplicatorGetStatus;

    {
        LockedFailoverUnitProxyPtr lockedFailoverUnitProxyPtr(msgContext.FailoverUnitProxy);

        if (lockedFailoverUnitProxyPtr->IsDeleted)
        {
            return;
        }

        if (!lockedFailoverUnitProxyPtr->TryAddToCurrentlyExecutingActionsLists(actionsList, false, msgContext))
        {
            // Incompatible work in progress, so drop the message (it will be re-sent by RA)
            RAPEventSource::Events->MessageDropInvalidWorkInProgress(id_, msgContext.Action);
            return;
        }
    }

    ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::CreateAndStart(
        *this,
        std::move(msgContext),
        actionsList,
        [this](AsyncOperationSPtr const & operation) { FinishReplicatorGetStatusMessageHandling(operation); });
}

Transport::MessageUPtr CreateQueryFailedMessage(ErrorCodeValue::Enum error, ProxyRequestMessageBody const & req)
{
    DeployedServiceReplicaDetailQueryResult result;
    
    ProxyQueryReplyMessageBody<DeployedServiceReplicaDetailQueryResult> replyBody(
        move(ErrorCode(error)), 
        req.FailoverUnitDescription, 
        req.LocalReplicaDescription, 
        Stopwatch::Now(), 
        move(result));

    return RAMessage::GetRAPQueryReply().CreateMessage(replyBody, req.FailoverUnitDescription.FailoverUnitId.Guid);
}

Transport::MessageUPtr CreateQuerySuccessReplyMessage(ProxyRequestMessageBody const & req, DeployedServiceReplicaDetailQueryResult && result)
{
    Trace.WriteInfo("SendRA", "Sending query reply: {0}", result);

    ProxyQueryReplyMessageBody<DeployedServiceReplicaDetailQueryResult> replyBody(
        ErrorCodeValue::Success, 
        req.FailoverUnitDescription, 
        req.LocalReplicaDescription, 
        Stopwatch::Now(), 
        move(result));

    return RAMessage::GetRAPQueryReply().CreateMessage(replyBody, req.FailoverUnitDescription.FailoverUnitId.Guid);
}

void ReconfigurationAgentProxy::QueryMessageHandler(FailoverUnitProxyContext<ProxyRequestMessageBody> & msgContext, __out Transport::MessageUPtr & reply)
{
    reply = nullptr;
    ErrorCode error;
    ProxyActionsListTypes::Enum actionsList = ProxyActionsListTypes::ReplicatorGetQuery;

    // Get the FUP Query Result
    ServiceModel::DeployedServiceReplicaDetailQueryResult fupQueryResult;

    {
        LockedFailoverUnitProxyPtr lockedFailoverUnitProxyPtr(msgContext.FailoverUnitProxy);

        if (lockedFailoverUnitProxyPtr->IsDeleted)
        {
            reply = CreateQueryFailedMessage(ErrorCodeValue::StaleRequest, msgContext.Body);
            return;
        }
        
        bool isReplicatorQueryRequired = false;
        error = lockedFailoverUnitProxyPtr->IsQueryAllowed(isReplicatorQueryRequired);

        if (!error.IsSuccess())
        {
            reply = CreateQueryFailedMessage(error.ReadValue(), msgContext.Body);
            return;
        }

        fupQueryResult = lockedFailoverUnitProxyPtr->GetQueryResult();

        if (!isReplicatorQueryRequired 
            ||
            (isReplicatorQueryRequired &&
            !lockedFailoverUnitProxyPtr->TryAddToCurrentlyExecutingActionsLists(actionsList, false, msgContext)))
        {
            // If replicator query is not needed OR if replicator query is required, but there is an incompatible action list running
            // we return back the rest of the RAP query result
            reply = CreateQuerySuccessReplyMessage(msgContext.Body, move(fupQueryResult));
            return;
        }
    }

    // Issue async op which will hold on to the FUP query result as well
    // In the complete it will use the FUP Query result to return the appropriate rpely based on the return value of the replicator
    AsyncOperationSPtr operation = AsyncOperation::CreateAndStart<ReconfigurationAgentProxy::ActionListExecutorReplicatorQueryAsyncOperation>(
        *this,
        std::move(msgContext),
        actionsList,
        [this](AsyncOperationSPtr const & operation) { this->ReplicatorGetQueryCompletionCallback(operation); },
        Root.CreateAsyncOperationRoot(),
        move(fupQueryResult));

    if (operation->CompletedSynchronously)
    {
        FinishReplicatorGetQueryMessageHandling(operation);
    }
}

void ReconfigurationAgentProxy::ReplicatorUpdateEpochAndGetStatusMessageHandler(FailoverUnitProxyContext<ProxyRequestMessageBody> & msgContext, __out MessageUPtr & reply)
{ 
    reply = nullptr;
    ProxyActionsListTypes::Enum actionsList = ProxyActionsListTypes::ReplicatorUpdateEpochAndGetStatus;

    {
        LockedFailoverUnitProxyPtr lockedFailoverUnitProxyPtr(msgContext.FailoverUnitProxy);

        if (lockedFailoverUnitProxyPtr->IsDeleted)
        {
            return;
        }

        if (!lockedFailoverUnitProxyPtr->TryAddToCurrentlyExecutingActionsLists(actionsList, false, msgContext))
        {
            // Incompatible work in progress, so drop the message (it will be re-sent by RA)
            RAPEventSource::Events->MessageDropInvalidWorkInProgress(id_, msgContext.Action);
            return;
        }

        if (lockedFailoverUnitProxyPtr->IsCatchupCancel)
        { 
            if (lockedFailoverUnitProxyPtr->CurrentConfigurationEpoch < msgContext.Body.FailoverUnitDescription.CurrentConfigurationEpoch)
            {
                lockedFailoverUnitProxyPtr->DoneCancelReplicatorCatchupReplicaSet();
            }
            else
            {
                return;
            }
        }

        if (lockedFailoverUnitProxyPtr->CurrentConfigurationEpoch < msgContext.Body.FailoverUnitDescription.CurrentConfigurationEpoch)
        {
            lockedFailoverUnitProxyPtr->ConfigurationStage = ProxyConfigurationStage::CatchupPending;
        }

        lockedFailoverUnitProxyPtr->FailoverUnitDescription.PreviousConfigurationEpoch = msgContext.Body.FailoverUnitDescription.PreviousConfigurationEpoch;
        lockedFailoverUnitProxyPtr->FailoverUnitDescription.CurrentConfigurationEpoch = msgContext.Body.FailoverUnitDescription.CurrentConfigurationEpoch;
    }

    ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::CreateAndStart(
        *this,
        std::move(msgContext),
        actionsList,
        [this](AsyncOperationSPtr const & operation) { FinishReplicatorUpdateEpochAndGetStatusMessageHandling(operation); });
}

void ReconfigurationAgentProxy::CancelCatchupReplicaSetMessageHandler(FailoverUnitProxyContext<ProxyRequestMessageBody> & msgContext, __out MessageUPtr & reply)
{
    reply = nullptr;
    ProxyActionsListTypes::Enum actionsList;

    {
        LockedFailoverUnitProxyPtr lockedFailoverUnitProxyPtr(msgContext.FailoverUnitProxy);
        
        if (lockedFailoverUnitProxyPtr->IsDeleted)
        {
            return;
        }

        if (lockedFailoverUnitProxyPtr->CatchupResult == CatchupResult::CatchupCompleted && !lockedFailoverUnitProxyPtr->AreServiceAndReplicatorRoleCurrent)
        {
            // Catchup completed for the P/S
            // This implies that not primary would have been observed
            // Ensure CR(S) completes
            actionsList = ProxyActionsListTypes::StatefulServiceFinalizeDemoteToSecondary;
        }
        else
        {
            actionsList = ProxyActionsListTypes::CancelCatchupReplicaSet;
        }

        if (!lockedFailoverUnitProxyPtr->TryAddToCurrentlyExecutingActionsLists(actionsList, false, msgContext))
        {
            // Incompatible work in progress, so drop the message (it will be re-sent by RA)
            RAPEventSource::Events->MessageDropInvalidWorkInProgress(id_, msgContext.Action);
            return;
        }
    }

    ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::CreateAndStart(
        *this,
        std::move(msgContext),
        actionsList,
        [this](AsyncOperationSPtr const & operation) { FinishCancelCatchupReplicaSetMessageHandling(operation); });
}

void ReconfigurationAgentProxy::UpdateServiceDescriptionMessageHandler(FailoverUnitProxyContext<ProxyRequestMessageBody> & msgContext, __out Transport::MessageUPtr & reply)
{
    auto const & body = msgContext.Body;
    auto actionsList = ProxyActionsListTypes::UpdateServiceDescription;
    {
        LockedFailoverUnitProxyPtr lockedFailoverUnitProxyPtr(msgContext.FailoverUnitProxy);

        if (lockedFailoverUnitProxyPtr->IsDeleted)
        {
            return;
        }

        if (lockedFailoverUnitProxyPtr->ReplicaDescription.ReplicaId != body.LocalReplicaDescription.ReplicaId ||
            lockedFailoverUnitProxyPtr->ReplicaDescription.InstanceId != body.LocalReplicaDescription.InstanceId)
        {
            return;
        }

        if (lockedFailoverUnitProxyPtr->ServiceDescription.UpdateVersion > body.ServiceDescription.UpdateVersion)
        {
            return;
        }

        if (lockedFailoverUnitProxyPtr->ServiceDescription.UpdateVersion == body.ServiceDescription.UpdateVersion)
        {
            ProxyUpdateServiceDescriptionReplyMessageBody replyBody(ErrorCode::Success(), body.FailoverUnitDescription, body.LocalReplicaDescription, body.ServiceDescription);
            reply = RAMessage::GetUpdateServiceDescriptionReply().CreateMessage<ProxyUpdateServiceDescriptionReplyMessageBody>(replyBody, replyBody.FailoverUnitDescription.FailoverUnitId.Guid);
            return;
        }

        if (!lockedFailoverUnitProxyPtr->TryAddToCurrentlyExecutingActionsLists(actionsList, false, msgContext))
        {
            // Incompatible work in progress, so drop the message (it will be re-sent by RA)
            RAPEventSource::Events->MessageDropInvalidWorkInProgress(id_, msgContext.Action);
            return;
        }
    }

    ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::CreateAndStart(
        *this,
        std::move(msgContext),
        actionsList,
        [this](AsyncOperationSPtr const & operation) { FinishUpdateServiceDescriptionMessageHandling(operation); });
}

void ReconfigurationAgentProxy::ReplicaEndpointUpdatedReplyMessageHandler(FailoverUnitProxyContext<ProxyRequestMessageBody> & msgContext, __out Transport::MessageUPtr & )
{
    LockedFailoverUnitProxyPtr lockedFailoverUnitProxyPtr(msgContext.FailoverUnitProxy);

    if (lockedFailoverUnitProxyPtr->IsDeleted)
    {
        return;
    }

    // Handle the endpoint updated reply
    lockedFailoverUnitProxyPtr->ProcessReplicaEndpointUpdatedReply(msgContext.Body.LocalReplicaDescription);
}

void ReconfigurationAgentProxy::ReadWriteStatusRevokedNotificationReplyMessageHandler(FailoverUnitProxyContext<ProxyRequestMessageBody> & msgContext, __out Transport::MessageUPtr & )
{
    LockedFailoverUnitProxyPtr lockedFailoverUnitProxyPtr(msgContext.FailoverUnitProxy);

    if (lockedFailoverUnitProxyPtr->IsDeleted)
    {
        return;
    }

    lockedFailoverUnitProxyPtr->ProcessReadWriteStatusRevokedNotificationReply(msgContext.Body.LocalReplicaDescription);
}

void ReconfigurationAgentProxy::ReplicatorGetQueryCompletionCallback(Common::AsyncOperationSPtr const & ActionListExecutorAsyncOperation)
{
    if (!ActionListExecutorAsyncOperation->CompletedSynchronously)
    {
        FinishReplicatorGetQueryMessageHandling(ActionListExecutorAsyncOperation);
    }
}

// Finish handling async processing of various messages
void ReconfigurationAgentProxy::FinishReplicaOpenMessageHandling(Common::AsyncOperationSPtr const & ActionListExecutorAsyncOperation)
{
    ProxyReplyMessageBody replyBody;
    FailoverUnitProxyContext<ProxyRequestMessageBody>* context;
    ProxyActionsListTypes::Enum actionsList;

    ErrorCode errorCode = ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::End(ActionListExecutorAsyncOperation, context, actionsList, replyBody);
    ErrorCodeValue::Enum result = errorCode.ReadValue();

    // Compose reply mesage
    MessageUPtr outMsg = RAMessage::GetReplicaOpenReply().CreateMessage<ProxyReplyMessageBody>(replyBody, replyBody.FailoverUnitDescription.FailoverUnitId.Guid);
    bool sendReply = true;
    bool isAbortNeeded = false;
    bool isCleanupNeeded = false;

    {
        LockedFailoverUnitProxyPtr lockedFailoverUnitProxyPtr(context->FailoverUnitProxy);

        if (result == ErrorCodeValue::RAProxyOperationIncompatibleWithCurrentFupState)
        {
            // This can happen while fup is shutting down or aborting
            sendReply = false;
        }
        else if (result != ErrorCodeValue::Success && result != ErrorCodeValue::RAProxyStateChangedOnDataLoss)
        {
            ASSERT_IF(!lockedFailoverUnitProxyPtr->ServiceDescription.HasPersistedState && lockedFailoverUnitProxyPtr->CurrentReplicaState == ReplicaStates::StandBy, "CurrentReplicaState cannot be StandBy for non-persisted service.");

            // If it is a persisted replica with Standby state then we skip close step
            if (lockedFailoverUnitProxyPtr->CurrentReplicaState != ReplicaStates::StandBy)
            {
                if (lockedFailoverUnitProxyPtr->TryMarkForAbort())
                {
                    isAbortNeeded = true;
                }
                else
                {
                    sendReply = false;
                }
            }
            
            isCleanupNeeded = context->Body.Flags == ProxyMessageFlags::Abort || !lockedFailoverUnitProxyPtr->AreOperationManagersOpenForBusiness;
        }
        else
        {
            lockedFailoverUnitProxyPtr->State = FailoverUnitProxyStates::Opened;
        }
    }

    if (isAbortNeeded)
    {
        // Execute abort outside lock
        context->FailoverUnitProxy->Abort(!isCleanupNeeded);

        context->FailoverUnitProxy->DoneExecutingActionsList(actionsList);

        if (isCleanupNeeded)
        {
            RemoveFailoverUnitProxy(context->FailoverUnitProxy);

            {
                LockedFailoverUnitProxyPtr lockedFailoverUnitProxyPtr(context->FailoverUnitProxy);
                lockedFailoverUnitProxyPtr->Cleanup();
            }
        }
    }
    else
    {
        context->FailoverUnitProxy->DoneExecutingActionsList(actionsList);
    }

    FinishMessageProcessing(*context, result, sendReply ? std::move(outMsg) : nullptr);
}

void ReconfigurationAgentProxy::FinishReplicaCloseMessageHandling(Common::AsyncOperationSPtr const & ActionListExecutorAsyncOperation)
{
    ProxyReplyMessageBody replyBody;
    FailoverUnitProxyContext<ProxyRequestMessageBody>* context;
    ProxyActionsListTypes::Enum actionsList;

    ErrorCode errorCode = ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::End(ActionListExecutorAsyncOperation, context, actionsList, replyBody);
    ErrorCodeValue::Enum result = errorCode.ReadValue();

    MessageUPtr outMsg = RAMessage::GetReplicaCloseReply().CreateMessage<ProxyReplyMessageBody>(replyBody, replyBody.FailoverUnitDescription.FailoverUnitId.Guid);
    bool sendReply = true;
    bool isAbortNeeded = false;
    bool isRemoveNeeded = false;

    {
        LockedFailoverUnitProxyPtr lockedFailoverUnitProxyPtr(context->FailoverUnitProxy);
        
        if (result == ErrorCodeValue::Success)
        {
            isRemoveNeeded = true;
        }
        else if (result == ErrorCodeValue::RAProxyOperationIncompatibleWithCurrentFupState)
        {
            sendReply = false;
        }
        else
        {
            bool isAbort = (context->Body.Flags & ProxyMessageFlags::Abort) != 0;
            ASSERT_IF(isAbort, "FinishReplicaClose: Abort should not fail");

            if (lockedFailoverUnitProxyPtr->TryMarkForAbort())
            {
                isAbortNeeded = true;
            }
            else
            {
                sendReply = false;
            }
        }
    }

    if (isAbortNeeded)
    {
        context->FailoverUnitProxy->Abort(false /* keepFUPOpen */);

        context->FailoverUnitProxy->DoneExecutingActionsList(actionsList);
        RemoveFailoverUnitProxy(context->FailoverUnitProxy);

        {
            LockedFailoverUnitProxyPtr lockedFailoverUnitProxyPtr(context->FailoverUnitProxy);
            lockedFailoverUnitProxyPtr->Cleanup();
        }
    }
    else if (isRemoveNeeded)
    {
        context->FailoverUnitProxy->DoneExecutingActionsList(actionsList);
        RemoveFailoverUnitProxy(context->FailoverUnitProxy);

        {
            LockedFailoverUnitProxyPtr lockedFailoverUnitProxyPtr(context->FailoverUnitProxy);
            lockedFailoverUnitProxyPtr->State = FailoverUnitProxyStates::Closed;
            lockedFailoverUnitProxyPtr->Cleanup();
        }
    }
    else
    {
        context->FailoverUnitProxy->DoneExecutingActionsList(actionsList);
    }

    FinishMessageProcessing(*context, result, sendReply ? std::move(outMsg) : nullptr);
}

void ReconfigurationAgentProxy::FinishStatefulServiceReopenMessageHandling(Common::AsyncOperationSPtr const & ActionListExecutorAsyncOperation)
{
    ProxyReplyMessageBody replyBody;
    FailoverUnitProxyContext<ProxyRequestMessageBody>* context;
    ProxyActionsListTypes::Enum actionsList;

    ErrorCode errorCode = ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::End(ActionListExecutorAsyncOperation, context, actionsList, replyBody);
    ErrorCodeValue::Enum result = errorCode.ReadValue();

    MessageUPtr outMsg = RAMessage::GetStatefulServiceReopenReply().CreateMessage<ProxyReplyMessageBody>(replyBody, replyBody.FailoverUnitDescription.FailoverUnitId.Guid);
    bool sendReply = true;
    bool isAbortNeeded = false;
    bool isCleanupNeeded = false;

    {
        LockedFailoverUnitProxyPtr lockedFailoverUnitProxyPtr(context->FailoverUnitProxy);

        if (result == ErrorCodeValue::RAProxyOperationIncompatibleWithCurrentFupState)
        {
            sendReply = false;
        }
        else if (result != ErrorCodeValue::Success)
        {
            if (lockedFailoverUnitProxyPtr->TryMarkForAbort())
            {
                isAbortNeeded = true;
            }
            else
            {
                sendReply = false;
            }
            
            isCleanupNeeded = context->Body.Flags == ProxyMessageFlags::Abort || !lockedFailoverUnitProxyPtr->AreOperationManagersOpenForBusiness;
        }
        else
        {
            lockedFailoverUnitProxyPtr->State = FailoverUnitProxyStates::Opened;
        }
    }

    if (isAbortNeeded)
    {
        context->FailoverUnitProxy->Abort(!isCleanupNeeded);
        context->FailoverUnitProxy->DoneExecutingActionsList(actionsList);

        if (isCleanupNeeded)
        {
            RemoveFailoverUnitProxy(context->FailoverUnitProxy);

            {
                LockedFailoverUnitProxyPtr lockedFailoverUnitProxyPtr(context->FailoverUnitProxy);
                lockedFailoverUnitProxyPtr->Cleanup();
            }
        }
    }
    else
    {
        context->FailoverUnitProxy->DoneExecutingActionsList(actionsList);
    }

    FinishMessageProcessing(*context, result, sendReply ? std::move(outMsg) : nullptr);
}

void ReconfigurationAgentProxy::FinishUpdateConfigurationMessageHandling(Common::AsyncOperationSPtr const & actionListExecutorAsyncOperation)
{
    ProxyReplyMessageBody replyBody;
    FailoverUnitProxyContext<ProxyRequestMessageBody>* context;
    ProxyActionsListTypes::Enum actionsList;

    ErrorCode errorCode = ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::End(actionListExecutorAsyncOperation, context, actionsList, replyBody);

    context->FailoverUnitProxy->DoneExecutingActionsList(actionsList);

    ErrorCodeValue::Enum result = errorCode.ReadValue();
    MessageUPtr outMsg = RAMessage::GetUpdateConfigurationReply().CreateMessage<ProxyReplyMessageBody>(replyBody, replyBody.FailoverUnitDescription.FailoverUnitId.Guid);

    // We do not send a reply if:
    //  a) The operation was cancelled
    //  b) The operation was failed because of pending work (RA should retry)
    bool sendReply = result != ErrorCodeValue::OperationCanceled &&
        result != ErrorCodeValue::RAProxyOperationIncompatibleWithCurrentFupState;

    FinishMessageProcessing(*context, result, sendReply ? std::move(outMsg) : nullptr);
}

void ReconfigurationAgentProxy::FinishReplicatorBuildIdleReplicaMessageHandling(Common::AsyncOperationSPtr const & ActionListExecutorAsyncOperation)
{
    ProxyReplyMessageBody replyBody;
    FailoverUnitProxyContext<ProxyRequestMessageBody>* context;
    ProxyActionsListTypes::Enum actionsList;

    ErrorCode errorCode = ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::End(ActionListExecutorAsyncOperation, context, actionsList, replyBody);

    context->FailoverUnitProxy->DoneExecutingActionsList(actionsList);

    ErrorCodeValue::Enum result = errorCode.ReadValue();
    MessageUPtr outMsg = RAMessage::GetReplicatorBuildIdleReplicaReply().CreateMessage<ProxyReplyMessageBody>(replyBody, replyBody.FailoverUnitDescription.FailoverUnitId.Guid);

    bool sendReply = result != ErrorCodeValue::RAProxyBuildIdleReplicaInProgress &&
                     result != ErrorCodeValue::RAProxyOperationIncompatibleWithCurrentFupState;
    
    FinishMessageProcessing(*context, result, sendReply ? std::move(outMsg) : nullptr);
}

void ReconfigurationAgentProxy::FinishReplicatorRemoveIdleReplicaMessageHandling(Common::AsyncOperationSPtr const & ActionListExecutorAsyncOperation)
{
    ProxyReplyMessageBody replyBody;
    FailoverUnitProxyContext<ProxyRequestMessageBody>* context;
    ProxyActionsListTypes::Enum actionsList;

    ErrorCode errorCode = ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::End(ActionListExecutorAsyncOperation, context, actionsList, replyBody);

    context->FailoverUnitProxy->DoneExecutingActionsList(actionsList);

    ErrorCodeValue::Enum result = errorCode.ReadValue();
    MessageUPtr outMsg = RAMessage::GetReplicatorRemoveIdleReplicaReply().CreateMessage<ProxyReplyMessageBody>(replyBody, replyBody.FailoverUnitDescription.FailoverUnitId.Guid);

    bool sendReply = result != ErrorCodeValue::RAProxyOperationIncompatibleWithCurrentFupState;
    FinishMessageProcessing(*context, result, sendReply ? std::move(outMsg) : nullptr);
}

void ReconfigurationAgentProxy::FinishReplicatorGetStatusMessageHandling(Common::AsyncOperationSPtr const & ActionListExecutorAsyncOperation)
{
    ProxyReplyMessageBody replyBody;
    FailoverUnitProxyContext<ProxyRequestMessageBody>* context;
    ProxyActionsListTypes::Enum actionsList;

    ErrorCode errorCode = ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::End(ActionListExecutorAsyncOperation, context, actionsList, replyBody);

    context->FailoverUnitProxy->DoneExecutingActionsList(actionsList);

    ErrorCodeValue::Enum result = errorCode.ReadValue();
    MessageUPtr outMsg = RAMessage::GetReplicatorGetStatusReply().CreateMessage<ProxyReplyMessageBody>(replyBody, replyBody.FailoverUnitDescription.FailoverUnitId.Guid);

    bool sendReply = result != ErrorCodeValue::RAProxyOperationIncompatibleWithCurrentFupState;
    FinishMessageProcessing(*context, result, sendReply ? std::move(outMsg) : nullptr);
}

void ReconfigurationAgentProxy::FinishUpdateServiceDescriptionMessageHandling(Common::AsyncOperationSPtr const & actionListExecutorAsyncOperation)
{
    ProxyReplyMessageBody dontCareReplyBody;
    FailoverUnitProxyContext<ProxyRequestMessageBody>* context;
    ProxyActionsListTypes::Enum actionsList;

    ErrorCode errorCode = ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::End(actionListExecutorAsyncOperation, context, actionsList, dontCareReplyBody);

    auto result = errorCode.ReadValue();

    context->FailoverUnitProxy->DoneExecutingActionsList(actionsList);

    ASSERT_IF(!errorCode.IsSuccess(), "Action list cannot fail");

    // For this generate the reply using the incoming request body as it is a different message body type
    auto const & incomingRequestBody = context->Body;
    ProxyUpdateServiceDescriptionReplyMessageBody replyBody(ErrorCode::Success(), incomingRequestBody.FailoverUnitDescription, incomingRequestBody.LocalReplicaDescription, incomingRequestBody.ServiceDescription);
    MessageUPtr outMsg = RAMessage::GetUpdateServiceDescriptionReply().CreateMessage<ProxyUpdateServiceDescriptionReplyMessageBody>(replyBody, replyBody.FailoverUnitDescription.FailoverUnitId.Guid);

    // We do not send a reply if:
    //  a) The operation was cancelled
    //  b) The operation was failed because of pending work (RA should retry)
    bool sendReply = result != ErrorCodeValue::OperationCanceled &&
        result != ErrorCodeValue::RAProxyOperationIncompatibleWithCurrentFupState;

    FinishMessageProcessing(*context, result, sendReply ? std::move(outMsg) : nullptr);
}

void ReconfigurationAgentProxy::FinishReplicatorGetQueryMessageHandling(Common::AsyncOperationSPtr const & ActionListExecutorAsyncOperation)
{
    ReplicatorStatusQueryResultSPtr replicatorResult;
    FailoverUnitProxyContext<ProxyRequestMessageBody>* context = nullptr;
    ProxyActionsListTypes::Enum actionsList;
    DeployedServiceReplicaDetailQueryResult fupResult;
    
    auto error = ReconfigurationAgentProxy::ActionListExecutorReplicatorQueryAsyncOperation::End(ActionListExecutorAsyncOperation, context, actionsList, replicatorResult, fupResult);
    context->FailoverUnitProxy->DoneExecutingActionsList(actionsList);

    Transport::MessageUPtr message;
    if (error.IsSuccess())
    {
        fupResult.SetReplicatorStatusQueryResult(move(replicatorResult));
    }
     
    message = CreateQuerySuccessReplyMessage(context->Body, move(fupResult));
    SendReplyAndTrace(move(message), context->ReceiverContext);
}

void ReconfigurationAgentProxy::FinishReplicatorUpdateEpochAndGetStatusMessageHandling(Common::AsyncOperationSPtr const & ActionListExecutorAsyncOperation)
{
    ProxyReplyMessageBody replyBody;
    FailoverUnitProxyContext<ProxyRequestMessageBody>* context;
    ProxyActionsListTypes::Enum actionsList;

    ErrorCode errorCode = ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::End(ActionListExecutorAsyncOperation, context, actionsList, replyBody);

    context->FailoverUnitProxy->DoneExecutingActionsList(actionsList);

    ErrorCodeValue::Enum result = errorCode.ReadValue();
    MessageUPtr outMsg = RAMessage::GetReplicatorUpdateEpochAndGetStatusReply().CreateMessage<ProxyReplyMessageBody>(replyBody, replyBody.FailoverUnitDescription.FailoverUnitId.Guid);

    bool sendReply = result != ErrorCodeValue::RAProxyOperationIncompatibleWithCurrentFupState;
    FinishMessageProcessing(*context, result, sendReply ? std::move(outMsg) : nullptr);
}

void ReconfigurationAgentProxy::FinishCancelCatchupReplicaSetMessageHandling(Common::AsyncOperationSPtr const & ActionListExecutorAsyncOperation)
{
    ProxyReplyMessageBody replyBody;
    FailoverUnitProxyContext<ProxyRequestMessageBody>* context;
    ProxyActionsListTypes::Enum actionsList;

    ErrorCode errorCode = ReconfigurationAgentProxy::ActionListExecutorAsyncOperation::End(ActionListExecutorAsyncOperation, context, actionsList, replyBody);

    context->FailoverUnitProxy->DoneExecutingActionsList(actionsList);

    ErrorCodeValue::Enum result = errorCode.ReadValue();
    MessageUPtr outMsg = RAMessage::GetCancelCatchupReplicaSetReply().CreateMessage<ProxyReplyMessageBody>(replyBody, replyBody.FailoverUnitDescription.FailoverUnitId.Guid);

    bool sendReply = result != ErrorCodeValue::RAProxyOperationIncompatibleWithCurrentFupState;
    FinishMessageProcessing(*context, result, sendReply ? std::move(outMsg) : nullptr);
}

void ReconfigurationAgentProxy::RemoveFailoverUnitProxy(FailoverUnitProxySPtr const & faiolverUnitProxySPtr)
{
    // Remove from the LFUPM
    LocalFailoverUnitProxyMapObj.RemoveFailoverUnitProxy(faiolverUnitProxySPtr->FailoverUnitId);
    LocalLoadReportingComponentObj.RemoveFailoverUnit(faiolverUnitProxySPtr->FailoverUnitId);
}

void ReconfigurationAgentProxy::FinishMessageProcessing(FailoverUnitProxyContext<ProxyRequestMessageBody> & context, Common::ErrorCode errorCode, Transport::MessageUPtr && reply)
{
    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(errorCode);

    SendReplyAndTrace(move(reply));
}

bool ReconfigurationAgentProxy::ReportLoad(
    FailoverUnitId const& fuId, 
    wstring && serviceName, 
    bool isStateful, 
    ReplicaRole::Enum replicaRole, 
    vector<LoadBalancingComponent::LoadMetric> && loadMetrics)
{
    if (State.Value == FabricComponentState::Opened)
    {
        LocalLoadReportingComponentObj.AddLoad(fuId, move(serviceName), isStateful, replicaRole, move(loadMetrics), NodeId);

        return true;
    }

    return false;
}

bool ReconfigurationAgentProxy::SendMessageToRA(ProxyOutgoingMessageUPtr && message)
{
    if (State.Value == FabricComponentState::Opened)
    {
        LocalMessageSenderComponentObj.AddMessage(std::move(message));
        return true;
    }

    return false;
}

void ReconfigurationAgentProxy::SendReplyAndTrace(Transport::MessageUPtr && msg, Transport::IpcReceiverContext * receiverContext)
{
    if (msg && receiverContext)
    {
        WriteNoise("SendRA", "Sending message {0} {1}", msg->Action, wformatString(msg->MessageId));
        receiverContext->Reply(move(msg));
    }
    else if (msg && !receiverContext)
    {
        WriteNoise("SendRA", "Dropping reply message {0} {1}", msg->Action, wformatString(msg->MessageId));
    }
}

void ReconfigurationAgentProxy::SendReplyAndTrace(Transport::MessageUPtr && msg)
{
    if (msg)
    {
        WriteNoise("SendRA", "Sending message {0} {1}", msg->Action, wformatString(msg->MessageId));
        ipcClient_.SendOneWay(move(msg));
    }
}
