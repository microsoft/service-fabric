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
using namespace Management::InfrastructureService;
using namespace ClientServerTransport;

StringLiteral const TraceComponent("Agent");

//------------------------------------------------------------------------------
// Message Dispatch Helpers
//

class InfrastructureServiceAgent::DispatchMessageAsyncOperationBase 
    : public TimedAsyncOperation
    , protected NodeActivityTraceComponent<TraceTaskCodes::InfrastructureService>
{
public:
    DispatchMessageAsyncOperationBase(
        Federation::NodeInstance const & nodeInstance,
        __in IInfrastructureService & service,
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

    __declspec(property(get=get_Service)) IInfrastructureService & Service;
    IInfrastructureService & get_Service() { return service_; }

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

    virtual ErrorCode EndDispatch(AsyncOperationSPtr const & operation, __out MessageUPtr & reply) = 0;

    void OnDispatchComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        ErrorCode error = this->EndDispatch(operation, reply_);

        this->TryComplete(operation->Parent, error);
    }

private:

    IInfrastructureService & service_;
    MessageUPtr request_; 
    IpcReceiverContextUPtr receiverContext_;
    MessageUPtr reply_;
};

class InfrastructureServiceAgent::RunCommandAsyncOperation : public DispatchMessageAsyncOperationBase
{
public:
    RunCommandAsyncOperation(
        bool isAdminOperation,
        Federation::NodeInstance const & nodeInstance,
        __in IInfrastructureService & service,
        MessageUPtr && message,
        IpcReceiverContextUPtr && receiverContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : DispatchMessageAsyncOperationBase(nodeInstance, service, move(message), move(receiverContext), timeout, callback, parent)
        , isAdminOperation_(isAdminOperation)
    {
    }
    
    virtual ~RunCommandAsyncOperation() { }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        RunCommandMessageBody body;
        if (this->Request.GetBody(body))
        {
            auto operation = this->Service.BeginRunCommand(
                isAdminOperation_,
                body.Command,
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
        wstring replyText;
        ErrorCode error = this->Service.EndRunCommand(operation, replyText);

        if (error.IsSuccess())
        {
            RunCommandReplyMessageBody replyBody(move(replyText));
            reply = InfrastructureServiceMessage::GetClientOperationSuccess(replyBody);
        }

        return error;
    }

    bool isAdminOperation_;
};

class InfrastructureServiceAgent::ReportTaskAsyncOperationBase : public DispatchMessageAsyncOperationBase
{
public:
    ReportTaskAsyncOperationBase(
        Federation::NodeInstance const & nodeInstance,
        __in IInfrastructureService & service,
        MessageUPtr && message,
        IpcReceiverContextUPtr && receiverContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : DispatchMessageAsyncOperationBase(nodeInstance, service, move(message), move(receiverContext), timeout, callback, parent) 
    {
    }
    
    virtual ~ReportTaskAsyncOperationBase() { }

protected:
    virtual AsyncOperationSPtr BeginDispatch(
        wstring const & taskId, 
        uint64 instanceId, 
        TimeSpan const, 
        AsyncCallback const &, 
        AsyncOperationSPtr const & parent) = 0;

    virtual ErrorCode EndDispatch(AsyncOperationSPtr const & operation) = 0;

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        ReportTaskMessageBody body;
        if (this->Request.GetBody(body))
        {
            auto operation = this->BeginDispatch(
                body.TaskId,
                body.InstanceId,
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
        ErrorCode error = this->EndDispatch(operation);

        if (error.IsSuccess())
        {
            reply = InfrastructureServiceMessage::GetClientOperationSuccess();
        }

        return error;
    }
};

class InfrastructureServiceAgent::ReportStartTaskSuccessAsyncOperation : public ReportTaskAsyncOperationBase
{
public:
    ReportStartTaskSuccessAsyncOperation(
        Federation::NodeInstance const & nodeInstance,
        __in IInfrastructureService & service,
        MessageUPtr && message,
        IpcReceiverContextUPtr && receiverContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ReportTaskAsyncOperationBase(nodeInstance, service, move(message), move(receiverContext), timeout, callback, parent) 
    {
    }

    virtual ~ReportStartTaskSuccessAsyncOperation() { }

protected:
    AsyncOperationSPtr BeginDispatch(
        wstring const & taskId, 
        uint64 instanceId,
        TimeSpan const timeout, 
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
    {
        return this->Service.BeginReportStartTaskSuccess(
            taskId,
            instanceId,
            timeout,
            callback,
            parent);
    }

    ErrorCode EndDispatch(AsyncOperationSPtr const & operation)
    {
        return this->Service.EndReportStartTaskSuccess(operation);
    }
};

class InfrastructureServiceAgent::ReportFinishTaskSuccessAsyncOperation : public ReportTaskAsyncOperationBase
{
public:
    ReportFinishTaskSuccessAsyncOperation(
        Federation::NodeInstance const & nodeInstance,
        __in IInfrastructureService & service,
        MessageUPtr && message,
        IpcReceiverContextUPtr && receiverContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ReportTaskAsyncOperationBase(nodeInstance, service, move(message), move(receiverContext), timeout, callback, parent) 
    {
    }

    virtual ~ReportFinishTaskSuccessAsyncOperation() { }

protected:
    AsyncOperationSPtr BeginDispatch(
        wstring const & taskId, 
        uint64 instanceId,
        TimeSpan const timeout, 
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
    {
        return this->Service.BeginReportFinishTaskSuccess(
            taskId,
            instanceId,
            timeout,
            callback,
            parent);
    }

    ErrorCode EndDispatch(AsyncOperationSPtr const & operation)
    {
        return this->Service.EndReportFinishTaskSuccess(operation);
    }
};

class InfrastructureServiceAgent::ReportTaskFailureAsyncOperation : public ReportTaskAsyncOperationBase
{
public:
    ReportTaskFailureAsyncOperation(
        Federation::NodeInstance const & nodeInstance,
        __in IInfrastructureService & service,
        MessageUPtr && message,
        IpcReceiverContextUPtr && receiverContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ReportTaskAsyncOperationBase(nodeInstance, service, move(message), move(receiverContext), timeout, callback, parent) 
    {
    }

    virtual ~ReportTaskFailureAsyncOperation() { }

protected:
    AsyncOperationSPtr BeginDispatch(
        wstring const & taskId, 
        uint64 instanceId,
        TimeSpan const timeout, 
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
    {
        return this->Service.BeginReportTaskFailure(
            taskId,
            instanceId,
            timeout,
            callback,
            parent);
    }

    ErrorCode EndDispatch(AsyncOperationSPtr const & operation)
    {
        return this->Service.EndReportTaskFailure(operation);
    }
};

//------------------------------------------------------------------------------
// Message Forwarding Helpers
//

class InfrastructureServiceAgent::InfrastructureTaskAsyncOperationBase : public TimedAsyncOperation
{
public:
    InfrastructureTaskAsyncOperationBase(
        __in ServiceRoutingAgentProxy & proxy,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : TimedAsyncOperation(timeout, callback, root)
        , routingAgentProxy_(proxy)
    {
    }

    virtual ~InfrastructureTaskAsyncOperationBase() { }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<InfrastructureTaskAsyncOperationBase>(operation)->Error;
    }
    
protected:
    virtual ErrorCode GetRequest(__out MessageUPtr &) = 0;
    
    virtual ErrorCode OnReply(MessageUPtr const &)
    {
        // ignore reply by default
        return ErrorCodeValue::Success;
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        MessageUPtr request;
        ErrorCode error = this->GetRequest(request);

        if (error.IsSuccess())
        {
            auto operation = routingAgentProxy_.BeginSendRequest(
                move(request),
                this->RemainingTime,
                [this](AsyncOperationSPtr const & operation) { this->OnRequestComplete(operation, false); },
                thisSPtr);
            this->OnRequestComplete(operation, true);
        }
        else
        {
            this->TryComplete(thisSPtr, error);
        }
    }

private:
    void OnRequestComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        MessageUPtr reply;
        ErrorCode error = routingAgentProxy_.EndSendRequest(operation, reply);

        if (error.IsSuccess())
        {
            error = this->OnReply(reply);
        }
        
        this->TryComplete(operation->Parent, error);
    }

    ServiceRoutingAgentProxy & routingAgentProxy_;
};

class InfrastructureServiceAgent::StartInfrastructureTaskAsyncOperation : public InfrastructureTaskAsyncOperationBase
{
public:
    StartInfrastructureTaskAsyncOperation(
        __in ServiceRoutingAgentProxy & proxy,
        FABRIC_INFRASTRUCTURE_TASK_DESCRIPTION * publicDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : InfrastructureTaskAsyncOperationBase(proxy, timeout, callback, root)
        , publicDescription_(publicDescription) 
    {
    }

    virtual ~StartInfrastructureTaskAsyncOperation() { }

protected:
    ErrorCode GetRequest(__out MessageUPtr & request)
    {
        auto taskDescription = Common::make_unique<InfrastructureTaskDescription>();
        ErrorCode error = taskDescription->FromPublicApi(*publicDescription_);

        taskDescription->UpdateDoAsyncAck(true);

        if (error.IsSuccess())
        {
            auto messageUPtr = ClusterManagerTcpMessage::GetStartInfrastructureTask(std::move(taskDescription))->GetTcpMessage();
            ServiceRoutingAgentMessage::WrapForForwarding(*messageUPtr);
            request.swap(messageUPtr);
        }

        return error;
    }

private:
    FABRIC_INFRASTRUCTURE_TASK_DESCRIPTION * publicDescription_;
};

class InfrastructureServiceAgent::FinishInfrastructureTaskAsyncOperation : public InfrastructureTaskAsyncOperationBase
{
public:
    FinishInfrastructureTaskAsyncOperation(
        __in ServiceRoutingAgentProxy & proxy,
        wstring const & taskId,
        uint64 instanceId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : InfrastructureTaskAsyncOperationBase(proxy, timeout, callback, root)
        , taskInstance_(Common::make_unique<TaskInstance>(taskId, instanceId))
    {
    }

    virtual ~FinishInfrastructureTaskAsyncOperation() { }

protected:
    ErrorCode GetRequest(__out MessageUPtr & request)
    {
        MessageUPtr messageUPtr = ClusterManagerTcpMessage::GetFinishInfrastructureTask(std::move(taskInstance_))->GetTcpMessage();
        ServiceRoutingAgentMessage::WrapForForwarding(*messageUPtr);
        request.swap(messageUPtr);

        return ErrorCodeValue::Success;
    }

private:

    std::unique_ptr<TaskInstance> taskInstance_;
};

class InfrastructureServiceAgent::QueryInfrastructureTaskAsyncOperation : public InfrastructureTaskAsyncOperationBase
{
public:
    QueryInfrastructureTaskAsyncOperation(
        __in ServiceRoutingAgentProxy & proxy,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : InfrastructureTaskAsyncOperationBase(proxy, timeout, callback, root)
    {
    }

    virtual ~QueryInfrastructureTaskAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out QueryResult & queryResult)
    {
        auto casted = AsyncOperation::End<QueryInfrastructureTaskAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            queryResult = move(casted->queryResult_);
        }

        return casted->Error;
    }

protected:
    ErrorCode GetRequest(__out MessageUPtr & request)
    {
        request = QueryTcpMessage::GetQueryRequest(
                Common::make_unique<Query::QueryRequestMessageBody>(
                    wformatString("{0}", Query::QueryNames::GetInfrastructureTask),
                    QueryArgumentMap()))->GetTcpMessage();

        return ErrorCodeValue::Success;
    }

    ErrorCode OnReply(MessageUPtr const & reply)
    {
        if (reply->GetBody(queryResult_))
        {
            return queryResult_.QueryProcessingError;
        }
        else
        {
            return ErrorCode::FromNtStatus(reply->GetStatus());
        }
    }

private:
    QueryResult queryResult_;
};

//------------------------------------------------------------------------------
// InfrastructureServiceAgent
//

InfrastructureServiceAgent::InfrastructureServiceAgent(
    Federation::NodeInstance const & nodeInstance,
    __in IpcClient & ipcClient,
    ComPointer<IFabricRuntime> const & runtimeCPtr)
    : IInfrastructureServiceAgent()
    , ComponentRoot()
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

InfrastructureServiceAgent::~InfrastructureServiceAgent()
{
    WriteInfo(TraceComponent, "{0} ~dtor", this->TraceId);
}

ErrorCode InfrastructureServiceAgent::Create(__out shared_ptr<InfrastructureServiceAgent> & result)
{
    ComPointer<IFabricRuntime> runtimeCPtr;

    HRESULT hr = ::FabricCreateRuntime(
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

    shared_ptr<InfrastructureServiceAgent> agentSPtr(new InfrastructureServiceAgent(nodeInstance, ipcClient, runtimeCPtr));
    
    error = agentSPtr->Initialize();

    if (error.IsSuccess())
    {
        result.swap(agentSPtr);
    }

    return error;
}

ErrorCode InfrastructureServiceAgent::Initialize()
{
    return routingAgentProxyUPtr_->Open();
}

void InfrastructureServiceAgent::Release()
{
    routingAgentProxyUPtr_->Close().ReadValue();
}

ErrorCode InfrastructureServiceAgent::RegisterInfrastructureServiceFactory(
    IFabricStatefulServiceFactory * factory)
{
    HRESULT hr = runtimeCPtr_->RegisterStatefulServiceFactory(
        SystemServiceApplicationNameHelper::InfrastructureServiceType->c_str(),
        factory);

    return ErrorCode::FromHResult(hr);
}

void InfrastructureServiceAgent::RegisterInfrastructureService(
    ::FABRIC_PARTITION_ID partitionId,
    ::FABRIC_REPLICA_ID replicaId,
    IInfrastructureServicePtr const & rootedService,
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

void InfrastructureServiceAgent::UnregisterInfrastructureService(
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

void InfrastructureServiceAgent::ReplaceServiceLocation(SystemServiceLocation const & serviceLocation)
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

void InfrastructureServiceAgent::DispatchMessage(
    IInfrastructureServicePtr const & servicePtr,
    __in Transport::MessageUPtr & message, 
    __in Transport::IpcReceiverContextUPtr & receiverContext)
{
    if (message->Actor == Actor::IS)
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

        if ((message->Action == InfrastructureServiceTcpMessage::RunCommandAction) ||
            (message->Action == InfrastructureServiceTcpMessage::InvokeInfrastructureCommandAction))
        {
            auto operation = AsyncOperation::CreateAndStart<RunCommandAsyncOperation>(
                true, // isAdminOperation
                routingAgentProxyUPtr_->NodeInstance,
                *servicePtr.get(),
                move(message),
                move(receiverContext),
                timeoutHeader.Timeout,
                [this](AsyncOperationSPtr const & operation) { return this->OnDispatchMessageComplete(operation, false); },
                ComponentRoot::CreateAsyncOperationMultiRoot(move(roots)));
            this->OnDispatchMessageComplete(operation, true);
        }
        else if (message->Action == InfrastructureServiceTcpMessage::InvokeInfrastructureQueryAction)
        {
            auto operation = AsyncOperation::CreateAndStart<RunCommandAsyncOperation>(
                false, // isAdminOperation
                routingAgentProxyUPtr_->NodeInstance,
                *servicePtr.get(),
                move(message),
                move(receiverContext),
                timeoutHeader.Timeout,
                [this](AsyncOperationSPtr const & operation) { return this->OnDispatchMessageComplete(operation, false); },
                ComponentRoot::CreateAsyncOperationMultiRoot(move(roots)));
            this->OnDispatchMessageComplete(operation, true);
        }
        else if (message->Action == InfrastructureServiceTcpMessage::ReportStartTaskSuccessAction)
        {
            auto operation = AsyncOperation::CreateAndStart<ReportStartTaskSuccessAsyncOperation>(
                routingAgentProxyUPtr_->NodeInstance,
                *servicePtr.get(),
                move(message),
                move(receiverContext),
                timeoutHeader.Timeout,
                [this](AsyncOperationSPtr const & operation) { return this->OnDispatchMessageComplete(operation, false); },
                ComponentRoot::CreateAsyncOperationMultiRoot(move(roots)));
            this->OnDispatchMessageComplete(operation, true);
        }
        else if (message->Action == InfrastructureServiceTcpMessage::ReportFinishTaskSuccessAction)
        {
            auto operation = AsyncOperation::CreateAndStart<ReportFinishTaskSuccessAsyncOperation>(
                routingAgentProxyUPtr_->NodeInstance,
                *servicePtr.get(),
                move(message),
                move(receiverContext),
                timeoutHeader.Timeout,
                [this](AsyncOperationSPtr const & operation) { return this->OnDispatchMessageComplete(operation, false); },
                ComponentRoot::CreateAsyncOperationMultiRoot(move(roots)));
            this->OnDispatchMessageComplete(operation, true);
        }
        else if (message->Action == InfrastructureServiceTcpMessage::ReportTaskFailureAction)
        {
            auto operation = AsyncOperation::CreateAndStart<ReportTaskFailureAsyncOperation>(
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
            WriteInfo(TraceComponent, "{0} invalid Action {1}: message Id = {2}", this->TraceId, message->Action, message->MessageId);
            routingAgentProxyUPtr_->OnIpcFailure(ErrorCodeValue::InvalidMessage, *receiverContext);
        }
    }
    else
    {
        WriteInfo(TraceComponent, "{0} invalid Actor {1}: message Id = {2}", this->TraceId, message->Actor, message->MessageId);
        routingAgentProxyUPtr_->OnIpcFailure(ErrorCodeValue::InvalidMessage, *receiverContext);
    }
}

void InfrastructureServiceAgent::OnDispatchMessageComplete(
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

AsyncOperationSPtr InfrastructureServiceAgent::BeginStartInfrastructureTask(
    ::FABRIC_INFRASTRUCTURE_TASK_DESCRIPTION * publicDescription,
    TimeSpan const timeout,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<StartInfrastructureTaskAsyncOperation>(
        *routingAgentProxyUPtr_,
        publicDescription,
        timeout,
        callback,
        parent);
}

ErrorCode InfrastructureServiceAgent::EndStartInfrastructureTask(
    AsyncOperationSPtr const & operation)
{
    return InfrastructureTaskAsyncOperationBase::End(operation);
}

AsyncOperationSPtr InfrastructureServiceAgent::BeginFinishInfrastructureTask(
    wstring const & taskId,
    uint64 instance,
    TimeSpan const timeout,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<FinishInfrastructureTaskAsyncOperation>(
        *routingAgentProxyUPtr_,
        taskId,
        instance,
        timeout,
        callback,
        parent);
}

ErrorCode InfrastructureServiceAgent::EndFinishInfrastructureTask(
    AsyncOperationSPtr const & operation)
{
    return InfrastructureTaskAsyncOperationBase::End(operation);
}

AsyncOperationSPtr InfrastructureServiceAgent::BeginQueryInfrastructureTask(
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<QueryInfrastructureTaskAsyncOperation>(
        *routingAgentProxyUPtr_,
        timeout,
        callback,
        parent);
}

ErrorCode InfrastructureServiceAgent::EndQueryInfrastructureTask(
    AsyncOperationSPtr const & operation,
    __out QueryResult & queryResult)
{
    return QueryInfrastructureTaskAsyncOperation::End(operation, queryResult);
}
