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
using namespace Query;
using namespace ServiceModel;
using namespace Store;
using namespace SystemServices;
using namespace Transport;
using namespace Management;
using namespace Management::UpgradeOrchestrationService;
using namespace ClientServerTransport;

StringLiteral const TraceComponent("UpgradeOrchestrationServiceAgent");

//------------------------------------------------------------------------------
// Message Dispatch Helpers
//

class UpgradeOrchestrationServiceAgent::DispatchMessageAsyncOperationBase
    : public TimedAsyncOperation
    , protected NodeActivityTraceComponent<TraceTaskCodes::UpgradeOrchestrationService>
{
public:
    DispatchMessageAsyncOperationBase(
        Federation::NodeInstance const & nodeInstance,
        __in IUpgradeOrchestrationService & service,
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

    __declspec(property(get = get_Service)) IUpgradeOrchestrationService & Service;
    IUpgradeOrchestrationService & get_Service() { return service_; }

    __declspec(property(get = get_Request)) Message & Request;
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

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        SystemServiceMessageBody requestBody;
        if (this->Request.SerializedBodySize() > 0)
        {
            if (!this->Request.GetBody(requestBody))
            {
                WriteWarning(TraceComponent, "{0} failed to get process incoming request: {1}", this->TraceId, this->Request.MessageId);
                this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
                return;
            }
        }

        auto operation = this->Service.BeginCallSystemService(
            std::move(this->Request.Action),
            std::move(requestBody.Content),
            this->RemainingTime,
            [this](AsyncOperationSPtr const & operation) { this->OnDispatchComplete(operation, false); },
            thisSPtr);
        this->OnDispatchComplete(operation, true);
    }

    virtual ErrorCode EndDispatch(AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
    {
        wstring result;

        ErrorCode error = this->Service.EndCallSystemService(operation, result);

        if (!error.IsSuccess())
        {
            Trace.WriteWarning(TraceComponent, "EndCallSystemServiceCall failed");
            return error;
        }

        SystemServiceReplyMessageBody replyBody(move(result));
        reply = UpgradeOrchestrationServiceMessage::GetClientOperationSuccess(replyBody);
        return error;
    }

    void OnDispatchComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        ErrorCode error = this->EndDispatch(operation, reply_);

        this->TryComplete(operation->Parent, error);
    }

private:

    IUpgradeOrchestrationService & service_;
    MessageUPtr request_;
    IpcReceiverContextUPtr receiverContext_;
    MessageUPtr reply_;
};

//StartClusterConfigurationUpgradeOperation
class UpgradeOrchestrationServiceAgent::StartClusterConfigurationUpgradeOperation : public DispatchMessageAsyncOperationBase
{
public:
    StartClusterConfigurationUpgradeOperation(
        Federation::NodeInstance const & nodeInstance,
        __in IUpgradeOrchestrationService & service,
        MessageUPtr && message,
        IpcReceiverContextUPtr && receiverContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : DispatchMessageAsyncOperationBase(nodeInstance, service, move(message), move(receiverContext), timeout, callback, parent)
    {
    }

    virtual ~StartClusterConfigurationUpgradeOperation() { }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        StartUpgradeMessageBody body;
        if (this->Request.GetBody(body))
        {
            ScopedHeap heap;
            FABRIC_START_UPGRADE_DESCRIPTION publicDescription = { 0 };
            body.Description.ToPublicApi(heap, publicDescription);

            auto operation = this->Service.BeginUpgradeConfiguration(
                &publicDescription,
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

    ErrorCode EndDispatch(AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
    {
        Trace.WriteInfo(TraceComponent, "Inside EndDispatch of StartUpgrade in Agent");
        auto error = this->Service.EndUpgradeConfiguration(operation);

        if (error.IsSuccess())
        {
            reply = UpgradeOrchestrationServiceMessage::GetClientOperationSuccess();
        }

        return error;
    }
};

//GetClusterConfigurationUpgradeStatus
class UpgradeOrchestrationServiceAgent::GetClusterConfigurationUpgradeStatusOperation : public DispatchMessageAsyncOperationBase
{
public:
    GetClusterConfigurationUpgradeStatusOperation(
        Federation::NodeInstance const & nodeInstance,
        __in IUpgradeOrchestrationService & service,
        MessageUPtr && message,
        IpcReceiverContextUPtr && receiverContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : DispatchMessageAsyncOperationBase(nodeInstance, service, move(message), move(receiverContext), timeout, callback, parent)
    {
    }

    virtual ~GetClusterConfigurationUpgradeStatusOperation() { }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = this->Service.BeginGetClusterConfigurationUpgradeStatus(
            this->RemainingTime,
            [this](AsyncOperationSPtr const & operation) { this->OnDispatchComplete(operation, false); },
            thisSPtr);
        this->OnDispatchComplete(operation, true);
    }

    ErrorCode EndDispatch(AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
    {
        ErrorCode error = this->Service.EndGetClusterConfigurationUpgradeStatus(operation, &progressResult_);

        if (!error.IsSuccess())
        {
            Trace.WriteWarning(TraceComponent, "GetClusterConfigurationUpgradeStatus in Agent/Service.EndGetClusterConfigurationUpgradeStatus failed");
            return error;
        }

        OrchestrationUpgradeProgress progress;

        const FABRIC_ORCHESTRATION_UPGRADE_PROGRESS * publicProgress = progressResult_->get_Progress();

        error = progress.FromPublicApi(*publicProgress);

        if (!error.IsSuccess())
        {
            return error;
        }
        else
        {
            OrchestrationUpgradeProgress getClusterConfigurationUpgradeStatusProgress(move(progress));
            GetUpgradeStatusReplyMessageBody replyBody(move(getClusterConfigurationUpgradeStatusProgress));
            reply = UpgradeOrchestrationServiceMessage::GetClientOperationSuccess(replyBody);
        }

        return error;
    }

    IFabricOrchestrationUpgradeStatusResult* progressResult_;
};
//GetClusterConfigurationOperation
class UpgradeOrchestrationServiceAgent::GetClusterConfigurationOperation : public DispatchMessageAsyncOperationBase
{
public:
    GetClusterConfigurationOperation(
        Federation::NodeInstance const & nodeInstance,
        __in IUpgradeOrchestrationService & service,
        MessageUPtr && message,
        IpcReceiverContextUPtr && receiverContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : DispatchMessageAsyncOperationBase(nodeInstance, service, move(message), move(receiverContext), timeout, callback, parent)
    {
    }

    virtual ~GetClusterConfigurationOperation() { }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        GetClusterConfigurationMessageBody body;
        wstring apiVersion;
        if (this->Request.GetBody(body))
        {
            apiVersion = move(body.ApiVersion);
        }
        else
        {
            // backward compatibility
            WriteInfo(TraceComponent, "{0} failed to get apiVersion from incoming message body: {1}", this->TraceId, this->Request.MessageId);
        }

        auto operation = this->Service.BeginGetClusterConfiguration(
            apiVersion,
            this->RemainingTime,
            [this](AsyncOperationSPtr const & operation) { this->OnDispatchComplete(operation, false); },
            thisSPtr);
        this->OnDispatchComplete(operation, true);
    }

    ErrorCode EndDispatch(AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
    {
        ErrorCode error = this->Service.EndGetClusterConfiguration(operation, &clusterConfiguration_);

        if (!error.IsSuccess())
        {
            Trace.WriteWarning(TraceComponent, "GetClusterConfiguration in Agent/Service.EndGetClusterConfiguration failed");
            return error;
        }

        GetClusterConfigurationReplyMessageBody replyBody(move(wstring(clusterConfiguration_->get_String())));
        reply = UpgradeOrchestrationServiceMessage::GetClientOperationSuccess(replyBody);
        return error;
    }

    IFabricStringResult* clusterConfiguration_;
};

//GetUpgradesPendingApproval
class UpgradeOrchestrationServiceAgent::GetUpgradesPendingApprovalOperation : public DispatchMessageAsyncOperationBase
{
public:
    GetUpgradesPendingApprovalOperation(
        Federation::NodeInstance const & nodeInstance,
        __in IUpgradeOrchestrationService & service,
        MessageUPtr && message,
        IpcReceiverContextUPtr && receiverContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : DispatchMessageAsyncOperationBase(nodeInstance, service, move(message), move(receiverContext), timeout, callback, parent)
    {
    }

    virtual ~GetUpgradesPendingApprovalOperation() { }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = this->Service.BeginGetUpgradesPendingApproval(
            this->RemainingTime,
            [this](AsyncOperationSPtr const & operation) { this->OnDispatchComplete(operation, false); },
            thisSPtr);
        this->OnDispatchComplete(operation, true);
    }

    ErrorCode EndDispatch(AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
    {
        Trace.WriteInfo(TraceComponent, "Inside EndDispatch of GetUpgradesPendingApproval in Agent");
        auto error = this->Service.EndGetUpgradesPendingApproval(operation);

        if (error.IsSuccess())
        {
            reply = UpgradeOrchestrationServiceMessage::GetClientOperationSuccess();
        }

        return error;
    }
};

//StartApprovedUpgrades
class UpgradeOrchestrationServiceAgent::StartApprovedUpgradesOperation : public DispatchMessageAsyncOperationBase
{
public:
    StartApprovedUpgradesOperation(
        Federation::NodeInstance const & nodeInstance,
        __in IUpgradeOrchestrationService & service,
        MessageUPtr && message,
        IpcReceiverContextUPtr && receiverContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : DispatchMessageAsyncOperationBase(nodeInstance, service, move(message), move(receiverContext), timeout, callback, parent)
    {
    }

    virtual ~StartApprovedUpgradesOperation() { }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = this->Service.BeginStartApprovedUpgrades(
            this->RemainingTime,
            [this](AsyncOperationSPtr const & operation) { this->OnDispatchComplete(operation, false); },
            thisSPtr);
        this->OnDispatchComplete(operation, true);
    }

    ErrorCode EndDispatch(AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
    {
        Trace.WriteInfo(TraceComponent, "Inside EndDispatch of StartApprovedUpgrades in Agent");
        auto error = this->Service.EndStartApprovedUpgrades(operation);

        if (error.IsSuccess())
        {
            reply = UpgradeOrchestrationServiceMessage::GetClientOperationSuccess();
        }

        return error;
    }
};


//------------------------------------------------------------------------------
// UpgradeOrchestrationServiceAgent
//

UpgradeOrchestrationServiceAgent::UpgradeOrchestrationServiceAgent(
    Federation::NodeInstance const & nodeInstance,
    __in IpcClient & ipcClient,
    ComPointer<IFabricRuntime> const & runtimeCPtr)
    : IUpgradeOrchestrationServiceAgent()
    , ComponentRoot()
    , NodeTraceComponent(nodeInstance)
    , routingAgentProxyUPtr_()
    , registeredServiceLocations_()
    , lock_()
    , runtimeCPtr_(runtimeCPtr)
    , servicePtr_()
{
    WriteInfo(TraceComponent, "{0} ctor", this->TraceId);

    auto temp = ServiceRoutingAgentProxy::Create(nodeInstance, ipcClient, *this);
    routingAgentProxyUPtr_.swap(temp);
}

UpgradeOrchestrationServiceAgent::~UpgradeOrchestrationServiceAgent()
{
    WriteInfo(TraceComponent, "{0} ~dtor", this->TraceId);
}

ErrorCode UpgradeOrchestrationServiceAgent::Create(__out shared_ptr<UpgradeOrchestrationServiceAgent> & result)
{
    ComPointer<IFabricRuntime> runtimeCPtr;

    HRESULT hr = ::FabricCreateRuntime(
        IID_IFabricRuntime,
        runtimeCPtr.VoidInitializationAddress());
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    ComFabricRuntime * castedRuntimePtr;
    try
    {
        castedRuntimePtr = dynamic_cast<ComFabricRuntime*>(runtimeCPtr.GetRawPointer());
    }
    catch (...)
    {
        Assert::TestAssert("UOSAgent unable to get ComfabricRuntime");
        return ErrorCodeValue::OperationFailed;
    }

    FabricNodeContextSPtr fabricNodeContext;
    auto error = castedRuntimePtr->Runtime->Host.GetFabricNodeContext(fabricNodeContext);
    if (!error.IsSuccess())
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

    IpcClient & ipcClient = castedRuntimePtr->Runtime->Host.Client;

    shared_ptr<UpgradeOrchestrationServiceAgent> agentSPtr(new UpgradeOrchestrationServiceAgent(nodeInstance, ipcClient, runtimeCPtr));

    error = agentSPtr->Initialize();

    if (error.IsSuccess())
    {
        Trace.WriteInfo(TraceComponent, "Successfully Created UpgradeOrchestrationServiceAgent");
        result.swap(agentSPtr);
    }

    return error;
}

ErrorCode UpgradeOrchestrationServiceAgent::Initialize()
{
    return routingAgentProxyUPtr_->Open();
}

void UpgradeOrchestrationServiceAgent::Release()
{
    routingAgentProxyUPtr_->Close().ReadValue();
}

void UpgradeOrchestrationServiceAgent::RegisterUpgradeOrchestrationService(
    ::FABRIC_PARTITION_ID partitionId,
    ::FABRIC_REPLICA_ID replicaId,
    IUpgradeOrchestrationServicePtr const & rootedService,
    __out wstring & serviceAddress)
{
    SystemServiceLocation serviceLocation(
        routingAgentProxyUPtr_->NodeInstance,
        Guid(partitionId),
        replicaId,
        DateTime::Now().Ticks);
    this->ReplaceServiceLocation(serviceLocation);

    
    auto selfRoot = this->CreateComponentRoot();

    servicePtr_ = rootedService;
    
    routingAgentProxyUPtr_->RegisterMessageHandler(
        serviceLocation,
        [this, rootedService](MessageUPtr & message, IpcReceiverContextUPtr & receiverContext)
    {
        this->DispatchMessage(rootedService, message, receiverContext);
    });

    serviceAddress = wformatString("{0}", serviceLocation);
}

void UpgradeOrchestrationServiceAgent::UnregisterUpgradeOrchestrationService(
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

void UpgradeOrchestrationServiceAgent::ReplaceServiceLocation(SystemServiceLocation const & serviceLocation)
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


TimeSpan UpgradeOrchestrationServiceAgent::GetRandomizedOperationRetryDelay(ErrorCode const & error) const
{
    return StoreTransaction::GetRandomizedOperationRetryDelay(
        error,
        ManagementConfig::GetConfig().MaxOperationRetryDelay);
}

void UpgradeOrchestrationServiceAgent::DispatchMessage(
    IUpgradeOrchestrationServicePtr const & servicePtr,
    __in Transport::MessageUPtr & message,
    __in Transport::IpcReceiverContextUPtr & receiverContext)
{
     if (message->Actor == Actor::UOS)
     {
        TimeoutHeader timeoutHeader;
        if (!message->Headers.TryReadFirst(timeoutHeader))
        {
            WriteInfo(TraceComponent, "{0} missing TimeoutHeader: message Id = {1}", this->TraceId, message->MessageId);
            routingAgentProxyUPtr_->OnIpcFailure(ErrorCodeValue::InvalidMessage, *receiverContext);
            return;
        }

        vector<ComponentRootSPtr> roots;
        roots.push_back(servicePtr.get_Root());
        roots.push_back(this->CreateComponentRoot());

        if (message->Action == UpgradeOrchestrationServiceTcpMessage::StartClusterConfigurationUpgradeAction)
        {
            auto operation = AsyncOperation::CreateAndStart<StartClusterConfigurationUpgradeOperation>(
                routingAgentProxyUPtr_->NodeInstance,
                *servicePtr.get(),
                move(message),
                move(receiverContext),
                timeoutHeader.Timeout,
                [this](AsyncOperationSPtr const & operation) { return this->OnDispatchMessageComplete(operation, false); },
                ComponentRoot::CreateAsyncOperationMultiRoot(move(roots)));
            this->OnDispatchMessageComplete(operation, true);
        }
        else if (message->Action == UpgradeOrchestrationServiceTcpMessage::GetUpgradesPendingApprovalAction)
        {
            auto operation = AsyncOperation::CreateAndStart<GetUpgradesPendingApprovalOperation>(
                routingAgentProxyUPtr_->NodeInstance,
                *servicePtr.get(),
                move(message),
                move(receiverContext),
                timeoutHeader.Timeout,
                [this](AsyncOperationSPtr const & operation) { return this->OnDispatchMessageComplete(operation, false); },
                ComponentRoot::CreateAsyncOperationMultiRoot(move(roots)));
            this->OnDispatchMessageComplete(operation, true);
        }
        else if (message->Action == UpgradeOrchestrationServiceTcpMessage::GetClusterConfigurationAction)
        {
            auto operation = AsyncOperation::CreateAndStart<GetClusterConfigurationOperation>(
                routingAgentProxyUPtr_->NodeInstance,
                *servicePtr.get(),
                move(message),
                move(receiverContext),
                timeoutHeader.Timeout,
                [this](AsyncOperationSPtr const & operation) { return this->OnDispatchMessageComplete(operation, false); },
                ComponentRoot::CreateAsyncOperationMultiRoot(move(roots)));
            this->OnDispatchMessageComplete(operation, true);
        }
        else if (message->Action == UpgradeOrchestrationServiceTcpMessage::GetClusterConfigurationUpgradeStatusAction)
        {
            auto operation = AsyncOperation::CreateAndStart<GetClusterConfigurationUpgradeStatusOperation>(
                routingAgentProxyUPtr_->NodeInstance,
                *servicePtr.get(),
                move(message),
                move(receiverContext),
                timeoutHeader.Timeout,
                [this](AsyncOperationSPtr const & operation) { return this->OnDispatchMessageComplete(operation, false); },
                ComponentRoot::CreateAsyncOperationMultiRoot(move(roots)));
            this->OnDispatchMessageComplete(operation, true);
        }
        else if (message->Action == UpgradeOrchestrationServiceTcpMessage::StartApprovedUpgradesAction)
        {
            auto operation = AsyncOperation::CreateAndStart<StartApprovedUpgradesOperation>(
                routingAgentProxyUPtr_->NodeInstance,
                *servicePtr.get(),
                move(message),
                move(receiverContext),
                timeoutHeader.Timeout,
                [this](AsyncOperationSPtr const & operation) { return this->OnDispatchMessageComplete(operation, false); },
                ComponentRoot::CreateAsyncOperationMultiRoot(move(roots)));
            this->OnDispatchMessageComplete(operation, true);
        }
        else
        {
            auto operation = AsyncOperation::CreateAndStart<DispatchMessageAsyncOperationBase>(
                routingAgentProxyUPtr_->NodeInstance,
                *servicePtr.get(),
                move(message),
                move(receiverContext),
                timeoutHeader.Timeout,
                [this](AsyncOperationSPtr const & operation) { return this->OnDispatchMessageComplete(operation, false); },
                ComponentRoot::CreateAsyncOperationMultiRoot(move(roots)));
            this->OnDispatchMessageComplete(operation, true);
        }
    }
    else
    {
        Trace.WriteWarning(TraceComponent, "{0} invalid Actor {1}: message Id = {2}", this->TraceId, message->Actor, message->MessageId);
        routingAgentProxyUPtr_->OnIpcFailure(ErrorCodeValue::InvalidMessage, *receiverContext);
    }
}

void UpgradeOrchestrationServiceAgent::OnDispatchMessageComplete(
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

