// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "../Management/NetworkInventoryManager/common/NIMCommon.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace ServiceModel;
using namespace Federation;
using namespace Hosting2;
using namespace Reliability;
using namespace Management::NetworkInventoryManager;

StringLiteral const TraceType("NetworkInventoryAgent");

// ********************************************************************************************************************
// NetworkInventoryAgent::OpenAsyncOperation Implementation
//
class NetworkInventoryAgent::OpenAsyncOperation : 
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
DENY_COPY(OpenAsyncOperation)

public:
    OpenAsyncOperation(
        NetworkInventoryAgent & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout)
    {
    }

    virtual ~OpenAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<OpenAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(TraceType, owner_.Root.TraceId, "Opening NetworkInventoryAgent: Timeout={0}",
            timeoutHelper_.GetRemainingTime());

        owner_.RegisterRequestHandler();

        TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
    }

private:
    NetworkInventoryAgent & owner_;
    TimeoutHelper timeoutHelper_;
};

// ********************************************************************************************************************
// NetworkInventoryAgent::CloseAsyncOperation Implementation
//
class NetworkInventoryAgent::CloseAsyncOperation : 
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
DENY_COPY(CloseAsyncOperation)

public:
    CloseAsyncOperation(
        NetworkInventoryAgent & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        leaseInvalidatedCount_(0),
        activatedHostClosedCount_(0)
    {
    }

    virtual ~CloseAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CloseAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        UNREFERENCED_PARAMETER(thisSPtr);

        WriteNoise(TraceType, owner_.Root.TraceId, "Closing NetworkInventoryAgent: Timeout={0}",
            timeoutHelper_.GetRemainingTime());

        owner_.UnregisterRequestHandler();

        TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
    }

private:
    NetworkInventoryAgent & owner_;
    TimeoutHelper timeoutHelper_;
    atomic_uint64 leaseInvalidatedCount_;
    atomic_uint64 activatedHostClosedCount_;
};

// Send a TRequest message to NIM service and get the TReply.
template <class TRequest, class TReply>
class NetworkInventoryAgent::NISRequestAsyncOperation : public AsyncOperation
{
public:

    NISRequestAsyncOperation(
        __in NetworkInventoryAgent & owner,
        TRequest const & requestMessageBody,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        requestMessageBody_(requestMessageBody)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out TReply & reply)
    {
        auto thisPtr = AsyncOperation::End<NISRequestAsyncOperation>(operation);
        reply = move(thisPtr->reply_);
        return thisPtr->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(TraceType, owner_.Root.TraceId, "NetworkInventoryAgent: Sending message to NIM service. {0}",
            timeoutHelper_.GetRemainingTime());

        MessageUPtr request = CreateMessageRequest();

        auto operation = owner_.reliability_.FederationWrapperBase.BeginRequestToFM(move(request),
            timeoutHelper_.GetRemainingTime(),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { OnRequestCompleted(operation, false); },
            thisSPtr);

        OnRequestCompleted(operation, true);
    }

    void OnRequestCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr message;
        ErrorCode error(ErrorCodeValue::Success);

        error = owner_.reliability_.FederationWrapperBase.EndRequestToFM(operation, message);
        if (!error.IsSuccess())
        {
            WriteWarning(TraceType, owner_.Root.TraceId, "End(NISRequestAsyncOperation): ErrorCode={0}", error);

            TryComplete(operation->Parent, error);
            return;
        }

        if (!message->GetBody(reply_))
        {
            error = ErrorCodeValue::InvalidMessage;
            WriteWarning(TraceType, owner_.Root.TraceId, "GetBody<TReply> failed: Message={0}, ErrorCode={1}", 
                *message, error);

            TryComplete(operation->Parent, error);
            return;
        }

        TryComplete(operation->Parent, error);
    }

private:

    MessageUPtr CreateMessageRequest()
    {
        MessageUPtr message = make_unique<Transport::Message>(requestMessageBody_);

        message->Headers.Add(ActorHeader(Actor::NetworkInventoryService));
        message->Headers.Add(ActionHeader(TRequest::ActionName));

        WriteInfo(TraceType, owner_.Root.TraceId, "NetworkInventoryAgent:CreateMessageRequest: Message={0}, Body={1}",
            *message, requestMessageBody_);

        return move(message);
    }

    TimeoutHelper timeoutHelper_;
    NetworkInventoryAgent & owner_;
    TRequest const & requestMessageBody_;
    TReply reply_;
};

template <class TRequest, class TReply, class TMessageBody>
class NetworkInventoryAgent::NISRequestAsyncOperationApi : public AsyncOperation
{
public:

    NISRequestAsyncOperationApi(
        __in NetworkInventoryAgent & owner,
        TMessageBody const & nimMessage,
        TRequest const & requestMessageBody,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        requestMessageBody_(requestMessageBody),
        nimMessage_(nimMessage)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation,
        __out TReply & reply)
    {
        auto thisPtr = AsyncOperation::End<NISRequestAsyncOperationApi>(operation);
        reply = move(thisPtr->reply_);
        return thisPtr->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(TraceType, owner_.Root.TraceId, "NetworkInventoryAgent: Sending message to NIM service. {0}",
            timeoutHelper_.GetRemainingTime());

        MessageUPtr request = CreateMessageRequest();

        auto operation = owner_.reliability_.FederationWrapperBase.BeginRequestToFM(move(request),
            timeoutHelper_.GetRemainingTime(),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { OnRequestCompleted(operation, false); },
            thisSPtr);

        OnRequestCompleted(operation, true);
    }

    void OnRequestCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr message;
        ErrorCode error(ErrorCodeValue::Success);

        error = owner_.reliability_.FederationWrapperBase.EndRequestToFM(operation, message);
        if (!error.IsSuccess())
        {
            WriteWarning(TraceType, owner_.Root.TraceId, "End(NISRequestAsyncOperationApi): ErrorCode={0}", error);

            TryComplete(operation->Parent, error);
            return;
        }

        if (!message->GetBody(reply_))
        {
            error = ErrorCodeValue::InvalidMessage;
            WriteWarning(TraceType, owner_.Root.TraceId, "GetBody<TReply> failed: Message={0}, ErrorCode={1}", 
                *message, error);

            TryComplete(operation->Parent, error);
            return;
        }

        TryComplete(operation->Parent, error);
    }

private:

    MessageUPtr CreateMessageRequest()
    {
        return nimMessage_.CreateMessage<TRequest>(requestMessageBody_);
    }

    TimeoutHelper timeoutHelper_;
    NetworkInventoryAgent & owner_;

    TRequest const & requestMessageBody_;
    TReply reply_;

    TMessageBody nimMessage_;
};

// ********************************************************************************************************************
// NetworkInventoryAgent Implementation
//
NetworkInventoryAgent::NetworkInventoryAgent(
    Common::ComponentRoot const & root,
    __in Federation::FederationSubsystem & federation,
    __in IReliabilitySubsystem & reliability,
    __in IHostingSubsystemSPtr hosting) :
    RootedObject(root),
    federation_(federation),
    reliability_(reliability),
    hosting_(hosting)
{
}

NetworkInventoryAgent::~NetworkInventoryAgent()
{
    WriteNoise(TraceType, Root.TraceId, "NetworkInventoryAgent.destructor");
}

AsyncOperationSPtr NetworkInventoryAgent::OnBeginOpen(
    TimeSpan const timeout,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<OpenAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode NetworkInventoryAgent::OnEndOpen(AsyncOperationSPtr const & operation)
{
    return OpenAsyncOperation::End(operation);
}

AsyncOperationSPtr NetworkInventoryAgent::OnBeginClose(
    TimeSpan const timeout,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode NetworkInventoryAgent::OnEndClose(AsyncOperationSPtr const & operation)
{
    return CloseAsyncOperation::End(operation);
}

void NetworkInventoryAgent::OnAbort()
{
    WriteInfo(TraceType, Root.TraceId, "Aborting NetworkInventoryAgent");
    UnregisterRequestHandler();
}

void NetworkInventoryAgent::RegisterRequestHandler()
{
    // Register the one way and request-reply message handlers
    federation_.RegisterMessageHandler(
        Actor::NetworkInventoryAgent,
        [this] (MessageUPtr & message, OneWayReceiverContextUPtr & oneWayReceiverContext)
        { 
            WriteError(TraceType, "{0} received a oneway message for NetworkInventoryAgent: {1}",
                Root.TraceId,
                *message);
            oneWayReceiverContext->Reject(ErrorCodeValue::InvalidMessage);
        },
        [this] (Transport::MessageUPtr & message, RequestReceiverContextUPtr & requestReceiverContext)
        { 
            this->ProcessMessage(message, requestReceiverContext); 
        },
        false /*dispatchOnTransportThread*/);

    WriteInfo(TraceType, Root.TraceId, "NetworkInventoryAgent::RegisterRequestHandler: registered handler for Actor::NetworkInventoryAgent: {0}",
        Actor::NetworkInventoryAgent);
}

void NetworkInventoryAgent::UnregisterRequestHandler()
{
    federation_.UnRegisterMessageHandler(Actor::NetworkInventoryAgent);
    WriteInfo(TraceType, Root.TraceId, "NetworkInventoryAgent::UnregisterRequestHandler: unregistered handler for Actor::NetworkInventoryAgent");
}

void NetworkInventoryAgent::ProcessMessage(
    __in Transport::MessageUPtr & message,
    __in Federation::RequestReceiverContextUPtr & context)
{
    wstring const & action = message->Action;
    if (action == PublishNetworkTablesRequestMessage::ActionName)
    {
        this->ProcessNetworkMappingTables(message, context);
    }
    else
    {
        WriteWarning(TraceType, Root.TraceId, "Dropping unsupported message: {0}", message);
    }
}

#pragma region Message Processing

// ********************************************************************************************************************
// NetworkInventoryAgent Message Processing
//
void NetworkInventoryAgent::ProcessNetworkMappingTables(
    __in Transport::MessageUPtr & message, 
    __in Federation::RequestReceiverContextUPtr & context)
{
    PublishNetworkTablesRequestMessage requestBody;
    if (!message->GetBody<PublishNetworkTablesRequestMessage>(requestBody))
    {
        auto error = ErrorCode::FromNtStatus(message->Status);
        WriteWarning(TraceType, Root.TraceId, "GetBody<PublishNetworkTablesRequestMessage> failed: Message={0}, ErrorCode={1}",
            message,
            error);
        return;
    }

    WriteNoise(TraceType, Root.TraceId, "Processing ProcessNetworkMappingTables: network={0} {1}, id={2}", 
        requestBody.NetworkName,
        requestBody.InstanceID,
        requestBody.SequenceNumber);

    auto operation = hosting_->FabricActivatorClientObj->BeginUpdateRoutes(
        requestBody,
        HostingConfig::GetConfig().RequestTimeout,
        [this, ctx = context.release()](AsyncOperationSPtr const & operation)
    {
        auto error = hosting_->FabricActivatorClientObj->EndUpdateRoutes(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            Root.TraceId,
            "End(ProcessNetworkMappingTablesCompleted): ErrorCode={0}",
            error);

        NetworkErrorCodeResponseMessage inReply(0, error);
        Transport::MessageUPtr replyMsg = Common::make_unique<Transport::Message>(inReply);
        ctx->Reply(move(replyMsg));
    },
        Root.CreateAsyncOperationRoot());
}

AsyncOperationSPtr NetworkInventoryAgent::BeginSendAllocationRequestMessage(
    NetworkAllocationRequestMessage const & params,
    TimeSpan const timeout,
    AsyncCallback const & callback)
{
    WriteNoise(TraceType, Root.TraceId, "NetworkInventoryAgent::BeginSendAllocationRequestMessage: {0}", params);
    return AsyncOperation::CreateAndStart<NetworkInventoryAgent::NISRequestAsyncOperation<NetworkAllocationRequestMessage, NetworkAllocationResponseMessage> >(
        *this, params, timeout, callback,
        Root.CreateAsyncOperationRoot());
}

ErrorCode NetworkInventoryAgent::EndSendAllocationRequestMessage(AsyncOperationSPtr const & operation,
    __out NetworkAllocationResponseMessage & reply)
{
    return NetworkInventoryAgent::NISRequestAsyncOperation<NetworkAllocationRequestMessage, NetworkAllocationResponseMessage>::End(
        operation,
        reply);
}

AsyncOperationSPtr NetworkInventoryAgent::BeginSendDeallocationRequestMessage(
    NetworkRemoveRequestMessage const & params,
    TimeSpan const timeout,
    AsyncCallback const & callback)
{
    WriteNoise(TraceType, Root.TraceId, "NetworkInventoryAgent::BeginSendDeallocationRequestMessage: {0}", params);
    return AsyncOperation::CreateAndStart<NetworkInventoryAgent::NISRequestAsyncOperation<NetworkRemoveRequestMessage, NetworkErrorCodeResponseMessage> >(
        *this, params, timeout, callback,
        Root.CreateAsyncOperationRoot());
}

Common::ErrorCode NetworkInventoryAgent::EndSendDeallocationRequestMessage(
    Common::AsyncOperationSPtr const & operation,
    __out NetworkErrorCodeResponseMessage & reply)
{
    return NetworkInventoryAgent::NISRequestAsyncOperation<NetworkRemoveRequestMessage, NetworkErrorCodeResponseMessage>::End(
        operation,
        reply);
}

Common::AsyncOperationSPtr NetworkInventoryAgent::BeginSendPublishNetworkTablesRequestMessage(
    PublishNetworkTablesMessageRequest const & params,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback)
{
    WriteNoise(TraceType, Root.TraceId, "NetworkInventoryAgent::BeginSendPublishNetworkTablesRequestMessage: {0}", params);
    return AsyncOperation::CreateAndStart<NetworkInventoryAgent::NISRequestAsyncOperation<PublishNetworkTablesMessageRequest, NetworkErrorCodeResponseMessage> >(
        *this, params, timeout, callback,
        Root.CreateAsyncOperationRoot());
}

Common::ErrorCode NetworkInventoryAgent::EndSendPublishNetworkTablesRequestMessage(
    Common::AsyncOperationSPtr const & operation,
    __out NetworkErrorCodeResponseMessage & reply)
{
    return NetworkInventoryAgent::NISRequestAsyncOperation<PublishNetworkTablesMessageRequest, NetworkErrorCodeResponseMessage>::End(
        operation,
        reply);
}

AsyncOperationSPtr NetworkInventoryAgent::BeginSendCreateRequestMessage(
    CreateNetworkMessageBody const & params,
    TimeSpan const timeout,
    AsyncCallback const & callback)
{
    WriteNoise(TraceType, Root.TraceId, "NetworkInventoryAgent::BeginSendCreateRequestMessage: {0}", params);
    return AsyncOperation::CreateAndStart<NetworkInventoryAgent::NISRequestAsyncOperationApi<CreateNetworkMessageBody, BasicFailoverReplyMessageBody, NIMMessage> >(
        *this, NIMMessage::GetCreateNetwork(),
        params, timeout, callback,
        Root.CreateAsyncOperationRoot());
}

ErrorCode NetworkInventoryAgent::EndSendCreateRequestMessage(AsyncOperationSPtr const & operation,
    __out BasicFailoverReplyMessageBody & reply)
{
    return NetworkInventoryAgent::NISRequestAsyncOperationApi<CreateNetworkMessageBody, BasicFailoverReplyMessageBody, NIMMessage>::End(
        operation,
        reply);
}

AsyncOperationSPtr NetworkInventoryAgent::BeginSendRemoveRequestMessage(
    NetworkRemoveRequestMessage const & params,
    TimeSpan const timeout,
    AsyncCallback const & callback)
{
    WriteNoise(TraceType, Root.TraceId, "NetworkInventoryAgent::BeginSendRemoveRequestMessage: {0}", params);
    return AsyncOperation::CreateAndStart<NetworkInventoryAgent::NISRequestAsyncOperation<NetworkRemoveRequestMessage, NetworkErrorCodeResponseMessage> >(
        *this, params, timeout, callback,
        Root.CreateAsyncOperationRoot());
}

ErrorCode NetworkInventoryAgent::EndSendRemoveRequestMessage(AsyncOperationSPtr const & operation,
    __out NetworkErrorCodeResponseMessage & reply)
{
    return NetworkInventoryAgent::NISRequestAsyncOperation<NetworkRemoveRequestMessage, NetworkErrorCodeResponseMessage>::End(
        operation,
        reply);
}

AsyncOperationSPtr NetworkInventoryAgent::BeginSendEnumerateRequestMessage(
    NetworkEnumerateRequestMessage const & params,
    TimeSpan const timeout,
    AsyncCallback const & callback)
{
    WriteNoise(TraceType, Root.TraceId, "NetworkInventoryAgent::BeginSendEnumerateRequestMessage: {0}", params);
    return AsyncOperation::CreateAndStart<NetworkInventoryAgent::NISRequestAsyncOperation<NetworkEnumerateRequestMessage, NetworkEnumerateResponseMessage> >(
        *this, params, timeout, callback,
        Root.CreateAsyncOperationRoot());
}

ErrorCode NetworkInventoryAgent::EndSendEnumerateRequestMessage(AsyncOperationSPtr const & operation,
    __out NetworkEnumerateResponseMessage & reply)
{
    return NetworkInventoryAgent::NISRequestAsyncOperation<NetworkEnumerateRequestMessage, NetworkEnumerateResponseMessage>::End(
        operation,
        reply);
}

#pragma endregion 

