// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Api;
using namespace Common;
using namespace Federation;
using namespace Hosting2;
using namespace ServiceModel;
using namespace SystemServices;
using namespace Transport;
using namespace Management;
using namespace Management::ClusterManager;
using namespace Management::TokenValidationService;
using namespace ClientServerTransport;

StringLiteral const TraceComponent("TokenValidationServiceAgent");

GlobalWString TokenValidationServiceAgent::ExpirationClaim = make_global<wstring>(L"exp");

//------------------------------------------------------------------------------
// Message Dispatch Helpers
//

class TokenValidationServiceAgent::DispatchMessageAsyncOperationBase 
    : public TimedAsyncOperation
    , protected NodeActivityTraceComponent<TraceTaskCodes::TokenValidationService>
{
public:
    DispatchMessageAsyncOperationBase(
        Federation::NodeInstance const & nodeInstance,
        __in ITokenValidationService & service,
        MessageUPtr && message,
        IpcReceiverContextUPtr && receiverContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , NodeActivityTraceComponent(nodeInstance, FabricActivityHeader::FromMessage(*message).ActivityId)
        , service_(service)
        , request_(move(message))
        , receiverContext_(move(receiverContext))
    {
    }

    virtual ~DispatchMessageAsyncOperationBase() { }

    __declspec(property(get=get_Service)) ITokenValidationService & Service;
    ITokenValidationService & get_Service() { return service_; }

    __declspec(property(get=get_Request)) Message & Request;
    Message & get_Request() { return *request_; }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out MessageUPtr & reply, __out IpcReceiverContextUPtr & receiverContext)
    {
        auto casted = AsyncOperation::End<DispatchMessageAsyncOperationBase>(operation);

        if (casted->Error.IsSuccess())
        {
            reply = move(casted->reply_);
        }

        // Always return receiver context for caller to send IPC failure
        receiverContext = move(casted->receiverContext_);

        return casted->Error;
    }

protected:

    virtual ErrorCode EndDispatch(AsyncOperationSPtr const & operation) = 0;

    void OnDispatchComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }
        ClaimsCollection result;
        ErrorCode error = this->EndDispatch(operation);

        this->TryComplete(operation->Parent, error);
    }
protected:
    MessageUPtr reply_;

private:

    ITokenValidationService & service_;
    MessageUPtr request_; 
    IpcReceiverContextUPtr receiverContext_;

};

class TokenValidationServiceAgent::ValidateTokenAsyncOperation : public DispatchMessageAsyncOperationBase
{
public:
    ValidateTokenAsyncOperation(
        Federation::NodeInstance const & nodeInstance,
        __in ITokenValidationService & service,
        MessageUPtr && message,
        IpcReceiverContextUPtr && receiverContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : DispatchMessageAsyncOperationBase(nodeInstance, service, move(message), move(receiverContext), timeout, callback, parent) 
        , claimResult_()
    {
    }

    virtual ~ValidateTokenAsyncOperation() { }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        ValidateTokenMessageBody body;
        if (this->Request.GetBody(body))
        {
            auto operation = this->Service.BeginValidateToken(
                body.Token,
                this->RemainingTime,
                [this](AsyncOperationSPtr const & operation) { this->OnDispatchComplete(operation, false); },
                thisSPtr);
            this->OnDispatchComplete(operation, true);
        }
        else
        {
            WriteInfo(TraceComponent, "{0} failed to get process incoming request: {1}", this->TraceId, this->Request.MessageId);
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }
    }

    ErrorCode EndDispatch(AsyncOperationSPtr const & operation)
    {
        TimeSpan expiration = TimeSpan::MaxValue;
        vector<Claim> claims;

        auto error = this->Service.EndValidateToken(operation, &claimResult_);

        if (error.IsSuccess())
        {
            const FABRIC_TOKEN_CLAIM_RESULT_LIST* claimsList = claimResult_->get_Result();
            for(ULONG i = 0; i < claimsList->Count; i++)
            {
                FABRIC_TOKEN_CLAIM claim = claimsList->Items[i];
                Claim c(claim.ClaimType, claim.Issuer, claim.OriginalIssuer, claim.Subject, claim.Value, claim.ValueType);
                claims.push_back(c);

                if (c.ClaimType == *ExpirationClaim)
                {
                    expiration = TimeSpan::FromSeconds(Common::Double_Parse(c.Value));
                }
            }
        }
        else
        {
            WriteWarning(
                TraceComponent, 
                this->TraceId, 
                "ValidateToken: error={0} details={1}", 
                error,
                error.Message);
        }

        ClaimsCollection claimsCollection(move(claims));
        TokenValidationMessage response(claimsCollection, expiration, error);

        MessageUPtr reply = make_unique<Message>(response);
        reply->Headers.Add(Transport::ActorHeader(Actor::Tvs));
        reply->Headers.Add(Transport::ActionHeader(Constants::TokenValidationAction));
        reply_ = move(reply);
        WriteNoise(TraceComponent, this->TraceId, "TokenValidationMessage: Message={0}", *reply);

        return error;
    }

    IFabricTokenClaimResult* claimResult_;
};

//------------------------------------------------------------------------------
// TokenValidationServiceAgent
//

ErrorCode TokenValidationServiceAgent::Create(__out shared_ptr<TokenValidationServiceAgent> & result)
{
    ComPointer<IFabricRuntime> runtimeCPtr;

    auto hr = ::FabricCreateRuntime(
        IID_IFabricRuntime,
        runtimeCPtr.VoidInitializationAddress());
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    ComPointer<ComFabricRuntime> castedRuntimeCPtr;
    hr = runtimeCPtr->QueryInterface(CLSID_ComFabricRuntime, castedRuntimeCPtr.VoidInitializationAddress());
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    auto runtimeImpl = castedRuntimeCPtr->Runtime;

    FabricNodeContextSPtr fabricNodeContext;
    auto error = runtimeImpl->Host.GetFabricNodeContext(fabricNodeContext);
    if(!error.IsSuccess())
    {
        return error;
    }

    NodeId nodeId;
    if (!NodeId::TryParse(fabricNodeContext->NodeId, nodeId))
    {
        Assert::TestAssert("Could not parse NodeId {0}", fabricNodeContext->NodeId);
        return ErrorCodeValue::OperationFailed;
    }

    uint64 nodeInstanceId = fabricNodeContext->NodeInstanceId;
    Federation::NodeInstance nodeInstance(nodeId, nodeInstanceId);

    IpcClient & ipcClient = runtimeImpl->Host.Client;

    shared_ptr<TokenValidationServiceAgent> agentSPtr(new TokenValidationServiceAgent(nodeInstance, ipcClient, runtimeCPtr));

    error = agentSPtr->Initialize();

    if (error.IsSuccess())
    {
        result.swap(agentSPtr);
    }

    return error;
}

TokenValidationServiceAgent::TokenValidationServiceAgent(
    Federation::NodeInstance const & nodeInstance,
    __in IpcClient & ipcClient,
    ComPointer<IFabricRuntime> const & runtimeCPtr)
    : ComponentRoot()
    , NodeTraceComponent(nodeInstance)
    , routingAgentProxyUPtr_()
    , registeredServiceLocations_()
    , lock_()
    , runtimeCPtr_(runtimeCPtr)
{
    WriteInfo(TraceComponent, "{0} ctor", this->TraceId);

    auto temp = ServiceRoutingAgentProxy::Create(nodeInstance, ipcClient, *this);
    routingAgentProxyUPtr_.swap(temp);
}

TokenValidationServiceAgent::~TokenValidationServiceAgent()
{
    WriteInfo(TraceComponent, "{0} ~dtor", this->TraceId);
}

ErrorCode TokenValidationServiceAgent::Initialize()
{
    return routingAgentProxyUPtr_->Open();
}

void TokenValidationServiceAgent::Release()
{
    routingAgentProxyUPtr_->Close().ReadValue();
}

void TokenValidationServiceAgent::RegisterTokenValidationService(
    ::FABRIC_PARTITION_ID partitionId,
    ::FABRIC_REPLICA_ID replicaId,
    ITokenValidationServicePtr const & rootedService,
    __out wstring & serviceAddress)
{
    SystemServiceLocation serviceLocation(
        routingAgentProxyUPtr_->NodeInstance,
        Guid(partitionId),
        replicaId,
        DateTime::Now().Ticks);

    this->ReplaceServiceLocation(serviceLocation);

    routingAgentProxyUPtr_->RegisterMessageHandler(
        serviceLocation,
        [this, rootedService](MessageUPtr & message, IpcReceiverContextUPtr & receiverContext)
    {
        this->DispatchMessage(rootedService, message, receiverContext);
    });

    serviceAddress = wformatString("{0}", serviceLocation);
}

void TokenValidationServiceAgent::UnregisterTokenValidationService(
    ::FABRIC_PARTITION_ID partitionId,
    ::FABRIC_REPLICA_ID replicaId)
{
    SystemServiceLocation serviceLocation(
        routingAgentProxyUPtr_->NodeInstance,
        Guid(partitionId),
        replicaId,
        DateTime::Now().Ticks);

    {
        AcquireExclusiveLock lock(lock_);

        auto findIt = find_if(registeredServiceLocations_.begin(), registeredServiceLocations_.end(),
            [&serviceLocation](SystemServiceLocation const & it) { return serviceLocation.EqualsIgnoreInstances(it); });

        if (findIt != registeredServiceLocations_.end())
        {
            serviceLocation = *findIt;

            registeredServiceLocations_.erase(findIt);
        }
    }

    // Expect to find location in our own list, but calling UnregisterMessageHandler()
    // is harmless even if we did not find it
    //
    routingAgentProxyUPtr_->UnregisterMessageHandler(serviceLocation);
}

void TokenValidationServiceAgent::ReplaceServiceLocation(SystemServiceLocation const & serviceLocation)
{
    // If there is already a handler registration for this partition+replica, make sure its handler is
    // unregistered from the routing agent proxy to be more robust to re-opened replicas
    // that may not have cleanly unregistered (e.g. rudely aborted).
    //
    bool foundExisting = false;
    SystemServiceLocation existingLocation;
    {
        AcquireExclusiveLock lock(lock_);

        auto findIt = find_if(registeredServiceLocations_.begin(), registeredServiceLocations_.end(),
            [&serviceLocation](SystemServiceLocation const & it) { return serviceLocation.EqualsIgnoreInstances(it); });

        if (findIt != registeredServiceLocations_.end())
        {
            foundExisting = true;
            existingLocation = *findIt;

            registeredServiceLocations_.erase(findIt);
        }

        registeredServiceLocations_.push_back(serviceLocation);
    }

    if (foundExisting)
    {
        routingAgentProxyUPtr_->UnregisterMessageHandler(existingLocation);
    }
}


void TokenValidationServiceAgent::DispatchMessage(
    ITokenValidationServicePtr const & servicePtr,
    __in Transport::MessageUPtr & message, 
    __in Transport::IpcReceiverContextUPtr & receiverContext)
{
    if (message->Actor == Actor::Tvs)
    {
        TimeoutHeader timeoutHeader;
        if (!message->Headers.TryReadFirst(timeoutHeader))
        {
            WriteInfo(TraceComponent, "{0} missing TimeoutHeader: message Id = {1}", this->TraceId, message->MessageId);
            routingAgentProxyUPtr_->OnIpcFailure(ErrorCodeValue::InvalidMessage, *receiverContext);
            return;
        }
        if(message->Action == TokenValidationServiceTcpMessage::ValidateTokenAction)
        {
            vector<ComponentRootSPtr> roots;
            roots.push_back(servicePtr.get_Root());
            roots.push_back(this->CreateComponentRoot());
            auto operation = AsyncOperation::CreateAndStart<ValidateTokenAsyncOperation>(
                routingAgentProxyUPtr_->NodeInstance,
                *servicePtr.get(),
                move(message),
                move(receiverContext),
                timeoutHeader.Timeout,
                [this](AsyncOperationSPtr const & operation) { return this->OnDispatchMessageComplete(operation, false); },
                ComponentRoot::CreateAsyncOperationMultiRoot(move(roots)));
            this->OnDispatchMessageComplete(operation, true);
        }
        else if(message->Action == TokenValidationServiceTcpMessage::GetMetadataAction)
        {
            this->GetTokenServiceMetadata(servicePtr, receiverContext);
        }
    }
    else
    {
        WriteInfo(TraceComponent, "{0} invalid Actor {1}: message Id = {2}", this->TraceId, message->Actor, message->MessageId);
        routingAgentProxyUPtr_->OnIpcFailure(ErrorCodeValue::InvalidMessage, *receiverContext);
    }
}

void TokenValidationServiceAgent::OnDispatchMessageComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    MessageUPtr reply;
    IpcReceiverContextUPtr receiverContext;
    ErrorCode error = DispatchMessageAsyncOperationBase::End(operation, reply, receiverContext);

    if (error.IsSuccess())
    {
        routingAgentProxyUPtr_->SendIpcReply(move(reply), *receiverContext);
    }
    else
    {
        routingAgentProxyUPtr_->OnIpcFailure(error, *receiverContext);
    }
}

void TokenValidationServiceAgent::GetTokenServiceMetadata(
    ITokenValidationServicePtr const & servicePtr,
    __in Transport::IpcReceiverContextUPtr & receiverContext)
{
    MessageUPtr reply;
    IFabricTokenServiceMetadataResult* metadataResult;
    auto error = servicePtr->GetTokenServiceMetadata(&metadataResult);
    if(error.IsSuccess())
    {
        const FABRIC_TOKEN_SERVICE_METADATA* metadata = metadataResult->get_Result();
        TokenValidationServiceMetadata returnData(metadata->Metadata, metadata->ServiceName, metadata->ServiceDnsName);
        reply = make_unique<Message>(returnData);
        reply->Headers.Add(Transport::ActorHeader(Actor::Tvs));
        reply->Headers.Add(Transport::ActionHeader(Constants::GetMetadataAction));

        routingAgentProxyUPtr_->SendIpcReply(move(reply), *receiverContext);
    }
    else
    {
        routingAgentProxyUPtr_->OnIpcFailure(error, *receiverContext);
    }
}
