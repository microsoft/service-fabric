// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace ClientServerTransport;
using namespace Common;
using namespace Federation;
using namespace Naming;
using namespace Reliability;
using namespace ServiceModel;
using namespace Transport;
using namespace Query;
using namespace Store;
using namespace SystemServices;
using namespace Api;
using namespace Management;
using namespace Management::CentralSecretService;
using namespace Management::ResourceManager;

StringLiteral const TraceComponent("CentralSecretServiceReplica");

template <class TAsyncOperation>
AsyncOperationSPtr CreateHandler(
    __in CentralSecretServiceReplica & replica,
    __in MessageUPtr request,
    __in IpcReceiverContextUPtr requestContext,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<TAsyncOperation>(
        replica.SecretManagerAgent,
        replica.ResourceManager,
        move(request),
        move(requestContext),
        timeout,
        callback,
        parent);
}

ErrorCode CompleteHandler(
    AsyncOperationSPtr const & operationSPtr,
    __out MessageUPtr & replyMsgUPtr,
    __out IpcReceiverContextUPtr & receiverContextUPtr)
{
    return ClientRequestAsyncOperation::End(operationSPtr, replyMsgUPtr, receiverContextUPtr);
}

AsyncOperationSPtr CreateResourceManagerHandler(
    __in CentralSecretServiceReplica & replica,
    __in MessageUPtr request,
    __in IpcReceiverContextUPtr requestContext,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return replica.ResourceManager.Handle(
        move(request),
        move(requestContext),
        timeout,
        callback,
        parent);
}

ErrorCode CompleteResourceManagerHandler(
    AsyncOperationSPtr const & operationSPtr,
    __out MessageUPtr & replyMsgUPtr,
    __out IpcReceiverContextUPtr & receiverContextUPtr)
{
    return IpcReplicaActivityAsyncOperation::End(operationSPtr, replyMsgUPtr, receiverContextUPtr);
}

CentralSecretServiceReplica::RequestProcessor::RequestProcessor(
    StartRequestProcessHandler const & startHandler,
    CompleteRequestProcessHandler const & completeHandler)
    : StartHandler(startHandler)
    , CompleteHandler(completeHandler)
{
}

CentralSecretServiceReplica::CentralSecretServiceReplica(
    Guid const & partitionId,
    FABRIC_REPLICA_ID const & replicaId,
    wstring const & serviceName,
    __in ServiceRoutingAgentProxy & routingAgentProxy,
    Api::IClientFactoryPtr const & clientFactory,
    Federation::NodeInstance const & nodeInstance)
    : KeyValueStoreReplica(partitionId, replicaId)
    , serviceName_(serviceName)
    , nodeInstance_(nodeInstance)
    , serviceLocation_()
    , clientFactory_(clientFactory)
    , routingAgentProxy_(routingAgentProxy)
    , requestHandlers_()
    , secretManager_(nullptr)
    , resourceManagerSvc_(nullptr)
{
    this->SetTraceId(this->PartitionedReplicaId.TraceId);

    WriteInfo(
        TraceComponent,
        "{0} ctor: node = {1}, this = {2}",
        this->TraceId,
        this->NodeInstance,
        TraceThis);
}

CentralSecretServiceReplica::~CentralSecretServiceReplica()
{
    WriteInfo(
        TraceComponent,
        "{0} ~dtor: node = {1}, this = {2}",
        this->TraceId,
        this->NodeInstance,
        TraceThis);
}

void CentralSecretServiceReplica::AddHandler(
    wstring const & action,
    StartRequestProcessHandler const & startHandler,
    CompleteRequestProcessHandler const & completeHandler)
{
    this->requestHandlers_.insert(make_pair(action, CentralSecretServiceReplica::RequestProcessor(startHandler, completeHandler)));
}


// *******************
// StatefulServiceBase
// *******************

ErrorCode CentralSecretServiceReplica::OnOpen(ComPointer<IFabricStatefulServicePartition> const & servicePartition)
{
    UNREFERENCED_PARAMETER(servicePartition);

    // Cache service location since it will not change after open
    serviceLocation_ = SystemServiceLocation(
        this->NodeInstance,
        this->PartitionId,
        this->ReplicaId,
        DateTime::Now().Ticks);

    this->secretManager_ =
        make_unique<SecretManager>(
            this->ReplicatedStore,
            this->PartitionedReplicaId);

    this->resourceManagerSvc_ =
        IpcResourceManagerService::Create(
            this->PartitionedReplicaId,
            this->ReplicatedStore,
            *ResourceTypes::Secret,
            std::bind(&CentralSecretServiceReplica::QuerySecretResourceMetadata, this, placeholders::_1, placeholders::_2, placeholders::_3),
            CentralSecretServiceConfig::GetConfig().MaxOperationRetryDelay);

    this->InitializeRequestHandlers();

    WriteInfo(
        TraceComponent,
        "ReplicaOpen: replicaId: {0}, node: {1}, serviceLocation: {2}",
        this->PartitionedReplicaId,
        this->NodeInstance,
        serviceLocation_);

    return ErrorCode::Success();
}

ErrorCode CentralSecretServiceReplica::OnChangeRole(::FABRIC_REPLICA_ROLE newRole, __out wstring & serviceLocation)
{
    WriteInfo(
        TraceComponent,
        "{0} OnChangeRole started: node = {1}, this = {2}, newRole = {3}",
        this->TraceId,
        this->NodeInstance,
        TraceThis,
        static_cast<int>(newRole));

    ErrorCode error(ErrorCodeValue::Success);
    if (newRole == ::FABRIC_REPLICA_ROLE_PRIMARY)
    {
        RegisterMessageHandler();

        serviceLocation = this->serviceLocation_.Location;
    }
    else
    {
        UnregisterMessageHandler();
    }

    WriteTrace(
        error.ToLogLevel(LogLevel::Warning, LogLevel::Info),
        TraceComponent,
        "{0} OnChangeRole completed: node = {1}, this = {2}, error = {3}",
        this->TraceId,
        this->NodeInstance,
        TraceThis,
        error);

    return error;
}

ErrorCode CentralSecretServiceReplica::OnClose()
{
    WriteInfo(
        TraceComponent,
        "{0} OnClose started: node = {1}",
        this->TraceId,
        this->NodeInstance);

    auto callback = this->OnCloseReplicaCallback;
    if (callback)
    {
        callback(this->PartitionId);
    }

    return ErrorCode::Success();
}

void CentralSecretServiceReplica::OnAbort()
{
    OnClose();
}

void CentralSecretServiceReplica::RegisterMessageHandler()
{
    auto selfRoot = this->CreateComponentRoot();

    routingAgentProxy_.RegisterMessageHandler(
        serviceLocation_,
        [this, selfRoot](MessageUPtr & message, IpcReceiverContextUPtr & receiverContext)
    {
        this->ProcessMessage(move(message), move(receiverContext));
    });
}

void CentralSecretServiceReplica::UnregisterMessageHandler()
{
    routingAgentProxy_.UnregisterMessageHandler(serviceLocation_);
}

TimeSpan CentralSecretServiceReplica::GetRandomizedOperationRetryDelay(ErrorCode const & error) const
{
    return StoreTransaction::GetRandomizedOperationRetryDelay(
        error,
        CentralSecretServiceConfig::GetConfig().MaxOperationRetryDelay);
}

void CentralSecretServiceReplica::InitializeRequestHandlers()
{
    AddHandler(CentralSecretServiceMessage::GetSecretsAction, CreateHandler<GetSecretsAsyncOperation>, CompleteHandler);
    AddHandler(CentralSecretServiceMessage::SetSecretsAction, CreateHandler<SetSecretsAsyncOperation>, CompleteHandler);
    AddHandler(CentralSecretServiceMessage::RemoveSecretsAction, CreateHandler<RemoveSecretsAsyncOperation>, CompleteHandler);
    AddHandler(CentralSecretServiceMessage::GetSecretVersionsAction, CreateHandler<GetSecretVersionsAsyncOperation>, CompleteHandler);

    /* ResourceManager Actions */
    vector<wstring> resourceManagerActions;

    this->ResourceManager.GetSupportedActions(resourceManagerActions);

    for (wstring const & action : resourceManagerActions)
    {
        AddHandler(action, CreateResourceManagerHandler, CompleteResourceManagerHandler);
    }
}

void CentralSecretServiceReplica::ProcessMessage(
    MessageUPtr && request,
    IpcReceiverContextUPtr && receiverContext)
{
    TimeoutHeader timeoutHeader;
    if (!request->Headers.TryReadFirst(timeoutHeader))
    {
        WriteInfo(TraceComponent, "{0} missing TimeoutHeader: message Id = {1}", this->TraceId, request->MessageId);
        routingAgentProxy_.OnIpcFailure(ErrorCodeValue::InvalidMessage, *receiverContext);
        return;
    }

    if (request->Actor != Transport::Actor::CSS)
    {
        WriteWarning(TraceComponent, "MessageId {0} was sent to the wrong actor. The target actor of Action {0} is {1}. MessageId: {2}", request->MessageId, request->Action, request->Actor);
        routingAgentProxy_.OnIpcFailure(ErrorCodeValue::InvalidMessage, *receiverContext);
        return;
    }

    auto timeout = TimeoutHeader::FromMessage(*request).Timeout;
    auto activityId = FabricActivityHeader::FromMessage(*request).ActivityId;
    auto messageId = request->MessageId;

    auto handler = this->requestHandlers_.find(request->Action);

    if (handler != this->requestHandlers_.end())
    {
        auto requestProcessor = handler->second;

        auto operation = requestProcessor.StartHandler(
            *this,
            move(request),
            move(receiverContext),
            timeout,
            [this, activityId, messageId, requestProcessor](AsyncOperationSPtr const & asyncOperation)
        {
            this->OnProcessMessageComplete(requestProcessor, activityId, messageId, asyncOperation, false);
        },
            this->CreateAsyncOperationRoot());
        this->OnProcessMessageComplete(requestProcessor, activityId, messageId, operation, true);
    }
    else
    {
        WriteWarning(TraceComponent, "Action {0} is not supported by CentralSecretService. MessageId: {1}", request->Action, request->MessageId);
        routingAgentProxy_.OnIpcFailure(ErrorCodeValue::InvalidMessage, *receiverContext);
    }
}

void CentralSecretServiceReplica::OnProcessMessageComplete(
    CentralSecretServiceReplica::RequestProcessor const & requestProcessor,
    ActivityId const & activityId,
    MessageId const & messageId,
    AsyncOperationSPtr const & asyncOperation,
    bool expectedCompletedSynchronously)
{
    UNREFERENCED_PARAMETER(messageId);

    if (asyncOperation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    MessageUPtr reply;
    IpcReceiverContextUPtr receiverContext;
    auto error = requestProcessor.CompleteHandler(asyncOperation, reply, receiverContext);

    if (error.IsSuccess())
    {
        routingAgentProxy_.SendIpcReply(move(reply), *receiverContext);
    }
    else
    {
        routingAgentProxy_.OnIpcFailure(error, *receiverContext, activityId);
    }
}

ErrorCode CentralSecretServiceReplica::QuerySecretResourceMetadata(
    __in wstring const & resourceName,
    __in Common::ActivityId const & activityId,
    __out ResourceMetadata & metadata)
{
    UNREFERENCED_PARAMETER(activityId);

    vector<SecretReference> secretRefs;
    vector<Secret> secrets;

    secretRefs.push_back(SecretReference(resourceName)); 
    ErrorCode error;

    //ErrorCode error = this->SecretManagerAgent.BeginGetSecrets(secretRefs, false, TimeSpan::MaxValue, [this]() {}, AsyncOperation:: activityId);

    if (!error.IsSuccess())
    {
        return error;
    }

    if (secrets.size() <= 0)
    {
        return ErrorCode(ErrorCodeValue::StoreRecordNotFound, wformatString("Secret resource, {0}, is not found.", resourceName));
    }
    else if (secrets.size() > 1)
    {
        Assert::CodingError("Multiple secrets found for one resource name. Resource = {0}", resourceName);
    }

    metadata = SecretResourceMetadata(secrets.at(0));

    return ErrorCode::Success();
}