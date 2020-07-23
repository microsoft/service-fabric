// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace ServiceModel;
using namespace Hosting2;

StringLiteral const TraceType("ApplicationHostManager");

// ********************************************************************************************************************
// ApplicationHostManager::OpenAsyncOperation Implementation
//
class ApplicationHostManager::OpenAsyncOperation : 
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(OpenAsyncOperation)

public:
    OpenAsyncOperation(
        ApplicationHostManager & owner,
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
        WriteNoise(
            TraceType,
            owner_.Root.TraceId,
            "Opening ApplicationHostManager: Timeout={0}",
            timeoutHelper_.GetRemainingTime());

#if !defined(PLATFORM_UNIX)
        auto error = ProcessUtility::GetProcessSidString(owner_.processSid_);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "GetProcessSidString: ErrorCode={0}",
            error);
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }
#endif

        owner_.RegisterIpcRequestHandler();

        ApplicationHostManager & appHostManager = owner_;
        
        IFabricActivatorClientSPtr activatorClient = this->owner_.hosting_.FabricActivatorClientObj;

        //Register handler to handle process termination events.
        auto terminationHandler = owner_.hosting_.FabricActivatorClientObj->AddProcessTerminationHandler(
            [&appHostManager] (EventArgs const& eventArgs)
            { 
                appHostManager.OnActivatedApplicationHostTerminated(eventArgs); 
            });
        
        owner_.terminationNotificationHandler_ = make_unique<ResourceHolder<HHandler>>(
            move(terminationHandler),
            [activatorClient](ResourceHolder<HHandler> * thisPtr)
        {
            activatorClient->RemoveProcessTerminationHandler(thisPtr->Value);
        });

        //Register handler to handle container health change events.
        auto healthChangeHandler = owner_.hosting_.FabricActivatorClientObj->AddContainerHealthCheckStatusChangeHandler(
            [&appHostManager](EventArgs const& eventArgs)
            { 
                appHostManager.OnContainerHealthCheckStatusChanged(eventArgs); 
            });
        
        owner_.healthStatusChangeNotificationHandler_ = make_unique<ResourceHolder<HHandler>>(
            move(healthChangeHandler),
            [activatorClient](ResourceHolder<HHandler> * thisPtr)
            {
                activatorClient->RemoveContainerHealthCheckStatusChangeHandler(thisPtr->Value);
            });
       
        TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
    }

private:
    ApplicationHostManager & owner_;
    TimeoutHelper timeoutHelper_;
};

// ********************************************************************************************************************
// ApplicationHostManager::CloseAsyncOperation Implementation
//
class ApplicationHostManager::CloseAsyncOperation : 
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(CloseAsyncOperation)

public:
    CloseAsyncOperation(
        ApplicationHostManager & owner,
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
        WriteNoise(
            TraceType,
            owner_.Root.TraceId,
            "Closing ApplicationHostManager: Timeout={0}",
            timeoutHelper_.GetRemainingTime());
        owner_.UnregisterIpcRequestHandler();
        if(owner_.terminationNotificationHandler_)
        {
            owner_.terminationNotificationHandler_.reset();
        }
        InvalidateLeaseInNonActivatedApplicationHosts(thisSPtr);
    }

private:
    void InvalidateLeaseInNonActivatedApplicationHosts(AsyncOperationSPtr const & thisSPtr)
    {
        vector<ApplicationHostRegistrationSPtr> registeredApplicationHosts = owner_.registrationTable_->Close();
        vector<ApplicationHostRegistrationSPtr> nonActivatedApplicationHosts;

        for(auto iter = registeredApplicationHosts.begin(); iter != registeredApplicationHosts.end(); ++iter)
        {
            auto registration = *iter;
            if (registration->HostType == ApplicationHostType::NonActivated)
            {
                nonActivatedApplicationHosts.push_back(registration);
            }
        }

        leaseInvalidatedCount_.store(nonActivatedApplicationHosts.size());

        WriteNoise(
            TraceType,
            owner_.Root.TraceId,
            "Sending Invalidate Lease request to {0} NonActivated ApplicationHosts:",
            nonActivatedApplicationHosts.size());

        for(auto iter = nonActivatedApplicationHosts.begin(); iter != nonActivatedApplicationHosts.end(); ++iter)
        {
            auto registration = *iter;
            InvalidateLease(thisSPtr, registration);
        }

        CheckPendingLeaseInvalidatations(thisSPtr, nonActivatedApplicationHosts.size());
    }

    void InvalidateLease(AsyncOperationSPtr const & thisSPtr, ApplicationHostRegistrationSPtr const & registration)
    {
        auto timeout = timeoutHelper_.GetRemainingTime();
        WriteNoise(
            TraceType, 
            owner_.Root.TraceId, 
            "Begin(InvalidateLeaseRequest): HostId={0}, Timeout={1}", 
            registration->HostId,
            timeout);
        auto operation = owner_.hosting_.IpcServerObj.BeginRequest(
            CreateInvalidateLeaseRequestMessage(),
            registration->HostId,
            timeout,
            [this, registration](AsyncOperationSPtr const & operation) { this->FinishInvalidateLease(operation, registration, false); },
            thisSPtr);
        FinishInvalidateLease(operation, registration, true);
    }

    void FinishInvalidateLease(
        AsyncOperationSPtr const & operation, 
        ApplicationHostRegistrationSPtr const & registration, 
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr reply;
        auto error = owner_.hosting_.IpcServerObj.EndRequest(operation, reply);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "End(InvalidateLeaseRequest): ErrorCode={0}, HostId={1}",
            error,
            registration->HostId);
        if (error.IsSuccess())
        {
            InvalidateLeaseReplyBody replyBody;
            if (reply->GetBody<InvalidateLeaseReplyBody>(replyBody))
            {
                error = replyBody.Error;
                WriteTrace(
                    error.ToLogLevel(),
                    TraceType,
                    owner_.Root.TraceId,
                    "InvalidateLeaseReply: ErrorCode={0}, HostId={1}", 
                    error,
                    registration->HostId);
            }
            else
            {
                error = ErrorCode::FromNtStatus(reply->Status);
                WriteWarning(
                    TraceType,
                    owner_.Root.TraceId,
                    "GetBody<InvalidateLeaseReplyBody> failed: Message={0}, ErrorCode={1}, HostId={2}",
                    *reply,
                    error,
                    registration->HostId);
            }
        }

        CheckPendingLeaseInvalidatations(operation->Parent, --leaseInvalidatedCount_);
    }

    void CheckPendingLeaseInvalidatations(AsyncOperationSPtr const & thisSPtr, uint64 count)
    {
        if(count == 0)
        {
            WriteNoise(
                TraceType,
                owner_.Root.TraceId,
                "Invalidated leases on all registered application hosts.");

            CloseActivatedApplicationHosts(thisSPtr);
        }
    }

    void CloseActivatedApplicationHosts(AsyncOperationSPtr const & thisSPtr)
    {
        vector<ApplicationHostProxySPtr> activatedApplicationHosts = owner_.activationTable_->Close();
        activatedHostClosedCount_.store(activatedApplicationHosts.size());

        WriteNoise(
            TraceType,
            owner_.Root.TraceId,
            "Closing {0} Activated ApplicationHosts.",
            activatedApplicationHosts.size());

        if (activatedApplicationHosts.empty())
        {
            CheckIfAllActivatedHostsClosed(thisSPtr, activatedApplicationHosts.size());
        }
        else
        {
            while(!activatedApplicationHosts.empty())
            {
                auto hostProxy = activatedApplicationHosts.back();
                activatedApplicationHosts.pop_back();
                CloseActivatedApplicationHost(thisSPtr, move(hostProxy), timeoutHelper_.GetRemainingTime());
            }
        }
    }

    void CloseActivatedApplicationHost(AsyncOperationSPtr const & thisSPtr, ApplicationHostProxySPtr && hostProxy, TimeSpan const timeout)
    {
        auto operation = hostProxy->BeginClose(
            timeout, 
            [this, hostProxy](AsyncOperationSPtr const & operation) mutable 
        { 
            ApplicationHostProxySPtr moveHostProxy = hostProxy; 
            hostProxy.reset(); 
            this->FinishCloseActivatedApplicationHost(move(moveHostProxy), operation, false); 
        },
            thisSPtr);
        FinishCloseActivatedApplicationHost(move(hostProxy), operation, true);
    }

    void FinishCloseActivatedApplicationHost(
        ApplicationHostProxySPtr && hostProxy, 
        AsyncOperationSPtr const & operation, 
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto proxy = move(hostProxy);
        auto error = proxy->EndClose(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType, 
            owner_.Root.TraceId, 
            "End(CloseProxy): ErrorCode={0},  Id={1}.", 
            error,
            proxy->HostId);
        proxy.reset();

        CheckIfAllActivatedHostsClosed(operation->Parent, --activatedHostClosedCount_);
    }

    void CheckIfAllActivatedHostsClosed(AsyncOperationSPtr const & thisSPtr, uint64 count)
    {
        if(count == 0)
        {
            WriteNoise(
                TraceType,
                owner_.Root.TraceId,
                "Closed all activated Application Hosts.");

            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
        }
    }

    MessageUPtr CreateInvalidateLeaseRequestMessage()
    {
        MessageUPtr request = make_unique<Message>();
        request->Headers.Add(Transport::ActorHeader(Actor::ApplicationHost));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::InvalidateLeaseRequest));

        WriteNoise(TraceType, owner_.Root.TraceId, "CreateInvalidateLeaseRequestMessage: Message={0}", *request);

        return move(request);
    }

private:
    ApplicationHostManager & owner_;
    TimeoutHelper timeoutHelper_;
    atomic_uint64 leaseInvalidatedCount_;
    atomic_uint64 activatedHostClosedCount_;
};

// ********************************************************************************************************************
// ApplicationHostManager::ActivateCodePackageAsyncOperation Implementation
//
class ApplicationHostManager::ActivateCodePackageAsyncOperation : 
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(ActivateCodePackageAsyncOperation)

public:
    ActivateCodePackageAsyncOperation(
        __in ApplicationHostManager & owner,
        CodePackageInstanceSPtr const & codePackageInstance,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        codePackageInstance_(codePackageInstance),
        timeoutHelper_(timeout),
        hostIsolationContext_(CreateHostIsolationContext(codePackageInstance)),
        hostProxy_(),
        activationId_(),
        codePackageRuntimeInformation_(0)
    {
    }

    virtual ~ActivateCodePackageAsyncOperation()
    {
        WriteNoise(TraceType, owner_.Root.TraceId, "ApplicationHostManager::ActivateCodePackageAsyncOperation.destructor");
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation, 
        __out CodePackageActivationIdUPtr & activationId, 
        __out CodePackageRuntimeInformationSPtr & codePackageRuntimeInformation)
    {
        auto thisPtr = AsyncOperation::End<ActivateCodePackageAsyncOperation>(operation);
        if (thisPtr->Error.IsSuccess())
        {
            activationId = move(make_unique<CodePackageActivationId>(thisPtr->activationId_));
            codePackageRuntimeInformation = thisPtr->codePackageRuntimeInformation_;
        }
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        bool created = false;
        auto error = CreateOrGetApplicationHostProxy(created);
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        if (created)
        {
            OpenApplicationHostProxy(thisSPtr);
        }
        else
        {
            ActivateCodePackage(thisSPtr);
        }
    }

private:
    ErrorCode CreateOrGetApplicationHostProxy(bool & created)
    {
        created = false;
        bool done = false;
        ErrorCode error = ErrorCode(ErrorCodeValue::Success);

        //
        // For multi code package application host, if ApplicationHostIsolationContext is at application level,
        // multiple thread can try to create the proxy for first time. While loop here handles this scenario.
        // TODO: Add a GetOrAdd() function to activation table itself as it is already thread safe.
        //
        while(!done)
        {
            error = owner_.activationTable_->Find(hostIsolationContext_, hostProxy_);
            if (error.IsSuccess() || !error.IsError(ErrorCodeValue::NotFound)) 
            { 
                return error;
            }

            error = ApplicationHostProxy::Create(
                HostingSubsystemHolder(
                    owner_.hosting_, 
                    owner_.hosting_.Root.CreateComponentRoot()),
                hostIsolationContext_,
                codePackageInstance_,
                hostProxy_);
            if (!error.IsSuccess()) 
            {
                return error;
            }

            error = owner_.activationTable_->Add(hostProxy_);
            if (error.IsSuccess())
            {
                created = true;
                return error;
            }
            if (!error.IsError(ErrorCodeValue::AlreadyExists))
            {
                return error;
            }
        }

        return error;
    }

    void OpenApplicationHostProxy(AsyncOperationSPtr const & thisSPtr)
    {
        auto timeout = timeoutHelper_.GetRemainingTime();
        
        auto operation = hostProxy_->BeginOpen(
            timeout, 
            [this](AsyncOperationSPtr const & operation)
            { 
                this->OnApplicationHostProxyOpened(operation); 
            },
            thisSPtr);

        if (operation->CompletedSynchronously)
        {
            FinishOpenApplicationHostProxy(operation);
        }
    }

    void OnApplicationHostProxyOpened(AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            FinishOpenApplicationHostProxy(operation);
        }
    }

    void FinishOpenApplicationHostProxy(AsyncOperationSPtr const & operation)
    {
        auto error = hostProxy_->EndOpen(operation);
        if (!error.IsSuccess())
        {
            ApplicationHostProxySPtr removed;
            owner_.activationTable_->Remove(hostProxy_->HostId, removed).ReadValue();

            TryComplete(operation->Parent, error);
            return;
        }

        ActivateCodePackage(operation->Parent);
    }

    void ActivateCodePackage(AsyncOperationSPtr const & thisSPtr)
    {
        auto timeout = timeoutHelper_.GetRemainingTime();
        
        auto operation = hostProxy_->BeginActivateCodePackage(
            codePackageInstance_,
            timeout,
            [this](AsyncOperationSPtr const & operation)
            { 
                this->OnCodePackageActivated(operation);
            },
            thisSPtr);
        
        if (operation->CompletedSynchronously)
        {
            FinishActivateCodePackage(operation);
        }
    }

    void OnCodePackageActivated(AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            FinishActivateCodePackage(operation);
        }
    }

    void FinishActivateCodePackage(AsyncOperationSPtr const & operation)
    {
        auto error = hostProxy_->EndActivateCodePackage(operation, activationId_, codePackageRuntimeInformation_);
        TryComplete(operation->Parent, error);
    }

    ApplicationHostIsolationContext CreateHostIsolationContext(CodePackageInstanceSPtr const & codePackageInstance)
    {
        auto codePackageIsolationPolicy = codePackageInstance->IsolationPolicyType;

        if ((codePackageInstance->EntryPoint.EntryPointType == EntryPointType::Exe ||
            codePackageInstance->EntryPoint.EntryPointType == EntryPointType::ContainerHost) &&
            (codePackageIsolationPolicy != CodePackageIsolationPolicyType::DedicatedProcess))
        {
            WriteNoise(
                TraceType,
                owner_.Root.TraceId,
                "CreateHostIsolationContext: Ignoring specified isolation policy {0} for EXE code package {1}:{2}:{3}",
                codePackageInstance->IsolationPolicyType,
                codePackageInstance->CodePackageInstanceId,
                codePackageInstance->Version,
                codePackageInstance->InstanceId);

            codePackageIsolationPolicy = CodePackageIsolationPolicyType::DedicatedProcess;
        }

        return ApplicationHostIsolationContext::Create(
            codePackageInstance->CodePackageInstanceId,
            codePackageInstance->RunAsId,
            codePackageInstance->InstanceId,
            codePackageIsolationPolicy);
    }

private:
    ApplicationHostManager & owner_;
    CodePackageInstanceSPtr const codePackageInstance_;
    TimeoutHelper timeoutHelper_;
    ApplicationHostIsolationContext hostIsolationContext_;
    ApplicationHostProxySPtr hostProxy_;
    CodePackageActivationId activationId_;
    CodePackageRuntimeInformationSPtr codePackageRuntimeInformation_;
};

// ********************************************************************************************************************
// ApplicationHostManager::DeactivateCodePackageAsyncOperation Implementation
//
class ApplicationHostManager::DeactivateCodePackageAsyncOperation : 
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(DeactivateCodePackageAsyncOperation)

public:
    DeactivateCodePackageAsyncOperation(
        __in ApplicationHostManager & owner,
        CodePackageInstanceIdentifier const & codePackageInstanceId,
        CodePackageActivationId const & activationId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        codePackageInstanceId_(codePackageInstanceId),
        activationId_(activationId),
        timeoutHelper_(timeout),
        hostProxy_()
    {
    }

    virtual ~DeactivateCodePackageAsyncOperation()
    {
        WriteNoise(TraceType, owner_.Root.TraceId, "ApplicationHostManager::DeactivateCodePackageAsyncOperation.destructor");
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DeactivateCodePackageAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.activationTable_->Find(activationId_.HostId, hostProxy_);
        if (error.IsError(ErrorCodeValue::NotFound))
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "Failed to find ApplicationHostProxy for CodePackageActivationId {0}, completing deactivation of the code package.",
                activationId_);

            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "Operation to find ApplicationHostProxy for CodePackageActivationId {0} failed. ErrorCode: {1}.",
                activationId_,
                error);

            TryComplete(thisSPtr, error);
            return;
        }

        DeactivateCodePackage(thisSPtr);
    }

private:
    void DeactivateCodePackage(AsyncOperationSPtr const & thisSPtr)
    {
        auto timeout = timeoutHelper_.GetRemainingTime();
        
        auto operation = hostProxy_->BeginDeactivateCodePackage(
            codePackageInstanceId_,
            activationId_,
            timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnCodePackageDeactivated(operation); },
            thisSPtr);
        
        if (operation->CompletedSynchronously)
        {
            FinishCodePackageDeactivate(operation);
        }
    }

    void OnCodePackageDeactivated(AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            FinishCodePackageDeactivate(operation);
        }
    }

    void FinishCodePackageDeactivate(AsyncOperationSPtr const & operation)
    {
        auto error = hostProxy_->EndDeactivateCodePackage(operation);
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }

        if (hostProxy_->HasHostedCodePackages())
        {      
            WriteNoise(
                TraceType,
                owner_.Root.TraceId,
                "ApplicationHostProxy with HostId:{0} has one or more CodePackageInstance hosted. Completing DeactivateCodePackageAsyncOperation with success.",
                hostProxy_->HostId);

            TryComplete(operation->Parent, ErrorCode(ErrorCodeValue::Success));
        }
        else
        {
            WriteNoise(
                TraceType,
                owner_.Root.TraceId,
                "ApplicationHostProxy with HostId:{0} has no CodePackageInstance hosted. Calling BeginClose on the ApplicationHostProxy instance.",
                hostProxy_->HostId);

            CloseApplicationHost(operation->Parent);
        }
    }

    void CloseApplicationHost(AsyncOperationSPtr const & thisSPtr)
    {
        TimeSpan timeout = timeoutHelper_.GetRemainingTime();
        auto operation = hostProxy_->BeginClose(
            timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnApplicationHostClosed(operation); },
            thisSPtr);
        if (operation->CompletedSynchronously)
        {
            FinishCloseApplicationHost(operation);
        }
    }

    void OnApplicationHostClosed(AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            FinishCloseApplicationHost(operation);
        }
    }

    void FinishCloseApplicationHost(AsyncOperationSPtr const & operation)
    {
        auto error = hostProxy_->EndClose(operation);

        ApplicationHostProxySPtr removed;
        owner_.activationTable_->Remove(hostProxy_->HostId, removed).ReadValue();

        TryComplete(operation->Parent, error);
    }

private:
    ApplicationHostManager & owner_;
    CodePackageInstanceIdentifier codePackageInstanceId_;
    CodePackageActivationId activationId_;
    TimeoutHelper timeoutHelper_;
    ApplicationHostProxySPtr hostProxy_;
};

// ********************************************************************************************************************
// ApplicationHostManager::GetContainerInfoAsyncOperation Implementation
//
class ApplicationHostManager::GetContainerInfoAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(GetContainerInfoAsyncOperation)

public:
    GetContainerInfoAsyncOperation(
        __in ApplicationHostManager & owner,
        CodePackageInstanceIdentifier const & codePackageInstanceId,
        CodePackageActivationId const & activationId,
        wstring const & containerInfoType,
        wstring const & containerInfoArgs,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        codePackageInstanceId_(codePackageInstanceId),
        activationId_(activationId),
        containerInfoType_(containerInfoType),
        containerInfoArgs_(containerInfoArgs),
        timeoutHelper_(timeout),
        hostProxy_()
    {
    }

    virtual ~GetContainerInfoAsyncOperation()
    {
        WriteNoise(TraceType, owner_.Root.TraceId, "ApplicationHostManager::GetContainerInfoAsyncOperation.destructor");
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out wstring & containerInfo)
    {
        auto thisPtr = AsyncOperation::End<GetContainerInfoAsyncOperation>(operation);
        containerInfo = thisPtr->containerInfo_;
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.activationTable_->Find(activationId_.HostId, hostProxy_);
        if (error.IsError(ErrorCodeValue::NotFound))
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "Failed to find ApplicationHostProxy for CodePackageActivationId {0}, completing Get Container Logs of the code package.",
                activationId_);

            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "Operation to find ApplicationHostProxy for CodePackageActivationId {0} failed. ErrorCode: {1}.",
                activationId_,
                error);

            TryComplete(thisSPtr, error);
            return;
        }

        GetContainerInfo(thisSPtr);
    }

private:
    void GetContainerInfo(AsyncOperationSPtr const & thisSPtr)
    {
        auto timeout = timeoutHelper_.GetRemainingTime();

        auto operation = hostProxy_->BeginGetContainerInfo(
            containerInfoType_,
            containerInfoArgs_,
            timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnGetContainerInfo(operation); },
            thisSPtr);

        if (operation->CompletedSynchronously)
        {
            FinishGetContainerInfo(operation);
        }
    }

    void OnGetContainerInfo(AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            FinishGetContainerInfo(operation);
        }
    }

    void FinishGetContainerInfo(AsyncOperationSPtr const & operation)
    {
        auto error = hostProxy_->EndGetContainerInfo(operation, containerInfo_);
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }

        TryComplete(operation->Parent, ErrorCode(ErrorCodeValue::Success));
    }

private:
    ApplicationHostManager & owner_;
    CodePackageInstanceIdentifier codePackageInstanceId_;
    CodePackageActivationId activationId_;
    wstring containerInfoType_;
    wstring containerInfoArgs_;
    TimeoutHelper timeoutHelper_;
    ApplicationHostProxySPtr hostProxy_;
    wstring containerInfo_;
};

// ********************************************************************************************************************
// ApplicationHostManager::UpdateCodePackageContextAsyncOperation Implementation
//
class ApplicationHostManager::UpdateCodePackageContextAsyncOperation : 
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(UpdateCodePackageContextAsyncOperation)

public:
    UpdateCodePackageContextAsyncOperation(
        __in ApplicationHostManager & owner,
        CodePackageContext const & codePackageContext,
        CodePackageActivationId const & codePackageActivationId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        codePackageContext_(codePackageContext),
        codePackageActivationId_(codePackageActivationId),
        timeout_(timeout),
        hostProxy_()
    {
    }

    virtual ~UpdateCodePackageContextAsyncOperation()
    {
        WriteNoise(TraceType, owner_.Root.TraceId, "ApplicationHostManager::UpdateCodePackageContextAsyncOperation.destructor");
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<UpdateCodePackageContextAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.activationTable_->Find(codePackageActivationId_.HostId, hostProxy_);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "Failed to find ApplicationHostProxy for CodePackageActivationId {0}, completing deactivation of the code package.",
                codePackageActivationId_);
            TryComplete(thisSPtr, error);
            return;
        }

        UpdateCodePackageContext(thisSPtr);
    }

private:
    void UpdateCodePackageContext(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = hostProxy_->BeginUpdateCodePackageContext(
            codePackageContext_,
            codePackageActivationId_,
            timeout_,
            [this](AsyncOperationSPtr const & operation) { this->FinishUpdateCodePackageContext(operation, false); },
            thisSPtr);
        FinishUpdateCodePackageContext(operation, true);
    }

    void FinishUpdateCodePackageContext(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = hostProxy_->EndUpdateCodePackageContext(operation);

        if (error.IsError(ErrorCodeValue::UpdateContextFailed))
        {
            // Do a lookup in the registrationTable if the AppHost is registered or not. If it is, then something is wrong and Abort the AppHostProxy.
            // If the AppHost is not registered then when the AppHost registers it will create the correct CodePackageActivationContext with correct ServiceType version
            // since it reads the Package xml file which will have correct contents.

            // Lookup in RegistrationTable and updating the flag should be under one lock. If you just protect flag by lock there will be a race where
            // you do a registration lookup and you don't find appHost. Then appHost registers you do a check for a flag and it's set to false.
            // Then you set the appHost flag for update from UpdateContext and never end up bringing down appHost for update.
            {
                AcquireWriteLock lock(owner_.updateContextlock_);
                ApplicationHostRegistrationSPtr registration;
                auto hostId = hostProxy_->HostId;
                auto result = owner_.registrationTable_->Find(hostId, registration);
                if (!result.IsSuccess())
                {
                    hostProxy_->IsUpdateContextPending = true;

                    WriteInfo(
                        TraceType,
                        owner_.Root.TraceId,
                        "ApplicationHostProxy needs an update {0}",
                        hostId);

                    TryComplete(operation->Parent, ErrorCodeValue::Success);
                    return;
                }

            }

            hostProxy_->Abort();
        }

        TryComplete(operation->Parent, error);
    }

private:
    ApplicationHostManager & owner_;
    CodePackageContext const codePackageContext_;
    CodePackageActivationId const codePackageActivationId_;
    TimeSpan const timeout_;
    ApplicationHostProxySPtr hostProxy_;
};

// ********************************************************************************************************************
// ApplicationHostManager::TerminateServiceHostAsyncOperation Implementation
//
class ApplicationHostManager::TerminateServiceHostAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(TerminateServiceHostAsyncOperation)

public:
    TerminateServiceHostAsyncOperation(
        __in ApplicationHostManager & owner,
        wstring const & hostId,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        hostId_(hostId),
        hostProxy_()
    {
    }

    virtual ~TerminateServiceHostAsyncOperation()
    {
        WriteNoise(TraceType, owner_.Root.TraceId, "ApplicationHostManager::TerminateServiceHostAsyncOperation.destructor");
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<TerminateServiceHostAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.activationTable_->Find(hostId_, hostProxy_);

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "Failed to find ApplicationHostProxy for hostId {0} while terminating with error {1}.",
                hostId_,
                error);

            thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::Success);
            return;
        }

        // If the AppHost is InProcessApplicationHost, abort it via GuestServiceTypeHostManager since
        // start/stop/update of InPrrocessApplicationHost is managed by GuestServiceTypeHostManager and 
        // FabricHost does not know about it.
        if (hostProxy_->HostType == ApplicationHostType::Activated_InProcess)
        {
            WriteInfo(
                TraceType,
                owner_.Root.TraceId,
                "Terminating InProcessApplicationHost hostId:{0}",
                hostId_);

            owner_.hosting_.GuestServiceTypeHostManagerObj->AbortGuestServiceTypeHost(
                hostProxy_->Context);

            thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::Success);
            return;
        }

        WriteNoise(
            TraceType,
            owner_.Root.TraceId,
            "Terminating ServiceHost hostId:{0}",
            hostId_);

        TerminateProcess(thisSPtr);
    }

private:
    void TerminateProcess(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_.FabricActivator->BeginTerminateProcess(
            hostId_,
            [this](AsyncOperationSPtr const & operation) { this->FinisTerminateProcess(operation, false); },
            thisSPtr
        );
        FinisTerminateProcess(operation, true);
    }

    void FinisTerminateProcess(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.FabricActivator->EndTerminateProcess(operation);

        WriteInfo(
            TraceType,
            owner_.Root.TraceId,
            "End Terminating ServiceHost hostId:{0} error:{1}",
            hostId_,
            error);

        TryComplete(operation->Parent, error);
    }

private:
    ApplicationHostManager & owner_;
    wstring const hostId_;
    ApplicationHostProxySPtr hostProxy_;
};

// ********************************************************************************************************************
// ApplicationHostManager::ApplicationHostCodePackageOperationCompleted Implementation
//
class ApplicationHostManager::ApplicationHostCodePackageAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(ApplicationHostCodePackageAsyncOperation)

public:
    ApplicationHostCodePackageAsyncOperation(
        _In_ ApplicationHostManager & owner,
        ApplicationHostCodePackageOperationRequest const & request,
        IpcReceiverContextUPtr && receiverContext,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , request_(request)
        , receiverContext_(move(receiverContext))
    {
    }

    virtual ~ApplicationHostCodePackageAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ApplicationHostCodePackageAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (!this->IsSupportedRequest())
        {
            //
            // This should not happen normally unless there is environment
            // corruption in ApplicationHost. Trace as error rather than asserting
            // to avoid bringing down the node completely.
            //
            WriteError(
                TraceType,
                owner_.Root.TraceId,
                "ApplicationHostCodePackageOperation: Unsuppoted request received: [{0}].",
                request_);

            this->SendOperationReplyAndComplete(ErrorCodeValue::OperationNotSupported, thisSPtr);
            return;
        }

        auto error = owner_.activationTable_->Find(request_.HostContext.HostId, hostProxy_);
        if (!error.IsSuccess())
        {
            WriteNoise(
                TraceType,
                owner_.Root.TraceId,
                "ApplicationHostCodePackageOperation: HostId not found for request={0}.",
                request_);

            this->SendOperationReplyAndComplete(error, thisSPtr);
            return;
        }

        if (hostProxy_->Context.IsCodePackageActivatorHost == false)
        {
            //
            // This should not happen normally unless there is environment
            // corruption in ApplicationHost. Trace as error rather than asserting
            // to avoid bringing down the node completely.
            //
            WriteError(
                TraceType,
                owner_.Root.TraceId,
                "ApplicationHostCodePackageOperation: Matching ApplicationHostProxy don't support requested operation. Request=[{0}].",
                request_);

            this->SendOperationReplyAndComplete(ErrorCodeValue::OperationNotSupported, thisSPtr);
            return;
        }

        this->StartApplicationHostCodePackageOperation(thisSPtr);
    }

    void StartApplicationHostCodePackageOperation(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = hostProxy_->BeginApplicationHostCodePackageOperation(
            request_,
            [this](AsyncOperationSPtr const & operation)
            {
                this->FinishApplicationHostCodePackageOperation(operation, false);
            },
            thisSPtr);

        this->FinishApplicationHostCodePackageOperation(operation, true);
    }

    void FinishApplicationHostCodePackageOperation(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedAsynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedAsynchronously)
        {
            return;
        }

        auto error = hostProxy_->EndApplicationHostCodePackageOperation(operation);
        
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "FinishApplicationHostCodePackageOperation: Request={0}, Error={1}.",
            request_,
            error);

        this->SendOperationReplyAndComplete(error, operation->Parent);
    }

private:
    bool IsSupportedRequest()
    {
        auto hostContext = request_.HostContext;

        return 
            ((hostContext.HostType == ApplicationHostType::Activated_InProcess ||
            hostContext.HostType == ApplicationHostType::Activated_SingleCodePackage) &&
            hostContext.IsCodePackageActivatorHost == true);
    }

    void SendOperationReplyAndComplete(
        ErrorCode const & error,
        AsyncOperationSPtr const & thisSPtr)
    {
        if (receiverContext_ != nullptr)
        {
            owner_.SendApplicationHostCodePackageOperationReply(error, move(receiverContext_));
        }

        this->TryComplete(thisSPtr, error);
    }

private:
    ApplicationHostManager & owner_;
    ApplicationHostCodePackageOperationRequest request_;
    IpcReceiverContextUPtr receiverContext_;
    ApplicationHostProxySPtr hostProxy_;
};

// ********************************************************************************************************************
// ApplicationHostManager Implementation
//
ApplicationHostManager::ApplicationHostManager(
    ComponentRoot const & root,
    __in HostingSubsystem & hosting) :
RootedObject(root),
    hosting_(hosting),
    registrationTable_(),
    activationTable_(),
    processId_(::GetCurrentProcessId()),
    processSid_(),
    updateContextlock_()
{
    auto registrationTable = make_unique<ApplicationHostRegistrationTable>(root);
    auto activationTable = make_unique<ApplicationHostActivationTable>(root, hosting_);
    auto activator = make_unique<ProcessActivator>(root);

    registrationTable_ = move(registrationTable);
    activationTable_ = move(activationTable);
}

ApplicationHostManager::~ApplicationHostManager()
{
    WriteNoise(TraceType, Root.TraceId, "ApplicationHostManager.destructor");
}

ErrorCode ApplicationHostManager::IsRegistered(wstring const & hostId, __out bool & isRegistered)
{
    ApplicationHostRegistrationSPtr registration;
    auto error = registrationTable_->Find(hostId, registration);
    if (!error.IsSuccess()) { return error; }

    isRegistered = (registration->GetStatus() == ApplicationHostRegistrationStatus::Completed);
    return ErrorCode(ErrorCodeValue::Success);
}

AsyncOperationSPtr ApplicationHostManager::BeginActivateCodePackage(
    CodePackageInstanceSPtr const & codePackageInstance,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ActivateCodePackageAsyncOperation>(
        *this,
        codePackageInstance,
        timeout,
        callback,
        parent);
}

ErrorCode ApplicationHostManager::EndActivateCodePackage(
    AsyncOperationSPtr const & operation,
    __out CodePackageActivationIdUPtr & activationId,
    __out CodePackageRuntimeInformationSPtr & codePackageRuntimeInformation)

{
    return ActivateCodePackageAsyncOperation::End(operation, activationId, codePackageRuntimeInformation);
}

AsyncOperationSPtr ApplicationHostManager::BeginUpdateCodePackageContext(
    CodePackageContext const & codePackageContext,
    CodePackageActivationId const & activationId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<UpdateCodePackageContextAsyncOperation>(
        *this,
        codePackageContext,
        activationId,
        timeout,
        callback,
        parent);
}

ErrorCode ApplicationHostManager::EndUpdateCodePackageContext(
    AsyncOperationSPtr const & operation)
{
    return UpdateCodePackageContextAsyncOperation::End(operation);
}

AsyncOperationSPtr ApplicationHostManager::BeginDeactivateCodePackage(
    CodePackageInstanceIdentifier const & codePackageInstanceId,
    CodePackageActivationId const & activationId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<DeactivateCodePackageAsyncOperation>(
        *this,
        codePackageInstanceId,
        activationId,
        timeout,
        callback,
        parent);
}

ErrorCode ApplicationHostManager::EndDeactivateCodePackage(AsyncOperationSPtr const & operation)
{
    return DeactivateCodePackageAsyncOperation::End(operation);
}

AsyncOperationSPtr ApplicationHostManager::BeginGetContainerInfo(
    CodePackageInstanceIdentifier const & codePackageInstanceId,
    CodePackageActivationId const & activationId,
    wstring const & containerInfoType,
    wstring const & containerInfoArgs,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<GetContainerInfoAsyncOperation>(
        *this,
        codePackageInstanceId,
        activationId,
        containerInfoType,
        containerInfoArgs,
        timeout,
        callback,
        parent);
}

ErrorCode ApplicationHostManager::EndGetContainerInfo(
    AsyncOperationSPtr const & operation, 
    __out wstring & containerInfo)
{
    return GetContainerInfoAsyncOperation::End(operation, containerInfo);
}

void ApplicationHostManager::SendDependentCodePackageEvent(
    CodePackageEventDescription const & eventDescription,
    CodePackageActivationId const & codePackageActivationId)
{
    ApplicationHostProxySPtr hostProxy;
    auto error = activationTable_->Find(codePackageActivationId.HostId, hostProxy);
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceType,
            Root.TraceId,
            "SendDependentCodePackageEvent: ApplicationHostProxy not found for Event={0}, CodePackageActivationId={1}.",
            eventDescription,
            codePackageActivationId);
        return;
    }

    hostProxy->SendDependentCodePackageEvent(eventDescription);
}

void ApplicationHostManager::TerminateCodePackage(CodePackageActivationId const & activationId)
{
    ApplicationHostProxySPtr hostProxy;
    auto error = activationTable_->Remove(activationId.HostId, hostProxy);
    if (error.IsError(ErrorCodeValue::NotFound))
    {
        WriteWarning(
            TraceType,
            Root.TraceId,
            "Failed to find ApplicationHostProxy for CodePackageActivationId {0}, completing termination of the code package.",
            activationId);
        return;
    }
    if (error.IsSuccess())
    {
        hostProxy->Abort();
    }
}

ErrorCode ApplicationHostManager::TerminateCodePackageExternally(CodePackageActivationId const & activationId)
{
    ApplicationHostProxySPtr hostProxy;
    auto error = activationTable_->Find(activationId.HostId, hostProxy);

    if (error.IsError(ErrorCodeValue::NotFound))
    {
        WriteWarning(
            TraceType,
            Root.TraceId,
            "Failed to find ApplicationHostProxy for CodePackageActivationId {0} when trying to terminate externally.",
            activationId);
        return error;
    }

    if (error.IsSuccess())
    {
        hostProxy->TerminateExternally();
    }
    else
    {
        WriteWarning(
            TraceType,
            Root.TraceId,
            "Failed to find ApplicationHostProxy for CodePackageActivationId {0} when trying to terminate externally with Error {1}",
            activationId,
            error);
    }

    return ErrorCodeValue::Success;
}

AsyncOperationSPtr ApplicationHostManager::BeginTerminateServiceHost(
    wstring const & hostId,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<TerminateServiceHostAsyncOperation>(
        *this,
        hostId,
        callback,
        parent);
}

ErrorCode ApplicationHostManager::EndTerminateServiceHost(
    AsyncOperationSPtr const & operation)
{
    return TerminateServiceHostAsyncOperation::End(operation);
}

AsyncOperationSPtr ApplicationHostManager::BeginApplicationHostCodePackageOperation(
    ApplicationHostCodePackageOperationRequest const & request,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ApplicationHostCodePackageAsyncOperation>(
        *this,
        request,
        move(IpcReceiverContextUPtr()),
        callback,
        parent);
}

ErrorCode ApplicationHostManager::EndApplicationHostCodePackageOperation(
    AsyncOperationSPtr const & operation)
{
    return ApplicationHostCodePackageAsyncOperation::End(operation);
}

ErrorCode ApplicationHostManager::FindApplicationHost(std::wstring const & codePackageInstanceId, __out ApplicationHostProxySPtr & hostProxy)
{
    auto error = this->activationTable_->FindApplicationHostByCodePackageInstanceId(codePackageInstanceId, hostProxy);

    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        Root.TraceId,
        "FindApplicationHost for code package instance id {0}: ErrorCode={1}",
        codePackageInstanceId,
        error);

    return error;
}

AsyncOperationSPtr ApplicationHostManager::OnBeginOpen(
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

ErrorCode ApplicationHostManager::OnEndOpen(AsyncOperationSPtr const & operation)
{
    return OpenAsyncOperation::End(operation);
}

AsyncOperationSPtr ApplicationHostManager::OnBeginClose(
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

ErrorCode ApplicationHostManager::OnEndClose(AsyncOperationSPtr const & operation)
{
    return CloseAsyncOperation::End(operation);
}

void ApplicationHostManager::OnAbort()
{
    WriteNoise(
        TraceType,
        Root.TraceId,
        "Aborting ApplicationHostManager");
    UnregisterIpcRequestHandler();

    vector<ApplicationHostProxySPtr> activatedApplicationHosts = activationTable_->Close();
    WriteNoise(
        TraceType,
        Root.TraceId,
        "Aborting {0} activated ApplicationHosts",
        activatedApplicationHosts.size());
    for(auto iter = activatedApplicationHosts.cbegin(); iter != activatedApplicationHosts.cend(); ++iter)
    {
        (*iter)->Abort();
    }

    // clear registration table
    registrationTable_->Close();
}

void ApplicationHostManager::RegisterIpcRequestHandler()
{
    auto root = Root.CreateComponentRoot();
    hosting_.IpcServerObj.RegisterMessageHandler(
        Actor::ApplicationHostManager,
        [this, root](MessageUPtr & message, IpcReceiverContextUPtr & context) { this->ProcessIpcMessage(*message, context); },
        false/*dispatchOnTransportThread*/);
}

void ApplicationHostManager::UnregisterIpcRequestHandler()
{
    hosting_.IpcServerObj.UnregisterMessageHandler(Actor::ApplicationHostManager);
}

void ApplicationHostManager::ProcessIpcMessage(
    __in Message & message, 
    __in IpcReceiverContextUPtr & context)
{
    wstring const & action = message.Action;
    if (action == Hosting2::Protocol::Actions::StartRegisterApplicationHostRequest)
    {
        this->ProcessStartRegisterApplicationHostRequest(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::FinishRegisterApplicationHostRequest)
    {
        this->ProcessFinishRegisterApplicationHostRequest(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::UnregisterApplicationHostRequest)
    {
        this->ProcessUnregisterApplicationHostRequest(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::CodePackageTerminationNotificationRequest)
    {
        this->ProcessCodePackageTerminationNotification(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::GetFabricProcessSidRequest)
    {
        this->ProcessGetFabricProcessSidRequest(message, context);
    }
    else if(action == ClientServerTransport::HealthManagerTcpMessage::ReportHealthAction)
    {
        ReportHealthReportMessageHandler(message, move(context));
    }
    else if (action == Management::ResourceMonitor::ResourceMonitorServiceRegistration::ResourceMonitorServiceRegistrationAction)
    {
        this->ProcessRegisterResourceMonitorService(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::ApplicationHostCodePackageOperationRequest)
    {
        this->ProcessApplicationHostCodePackageOperationRequest(message, move(context));
    }
    else
    {
        WriteWarning(
            TraceType,
            Root.TraceId,
            "Dropping unsupported message: {0}",
            message);
    }
}

void ApplicationHostManager::ReportHealthReportMessageHandler(Message & message, IpcReceiverContextUPtr && context)
{
    // Just send the report to HM
    MessageUPtr messagePtr = make_unique<Message>(message);
    hosting_.SendToHealthManager(move(messagePtr), move(context));
}

void ApplicationHostManager::ProcessApplicationHostCodePackageOperationRequest(
    __in Message & message,
    __in IpcReceiverContextUPtr && context)
{
    ApplicationHostCodePackageOperationRequest requestBody;
    if (!message.GetBody<ApplicationHostCodePackageOperationRequest>(requestBody))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteError(
            TraceType,
            Root.TraceId,
            "GetBody<ApplicationHostCodePackageOperationRequest> failed: Message={0}, ErrorCode={1}",
            message,
            error);

        this->SendApplicationHostCodePackageOperationReply(error, move(context));
        return;
    }

    auto operation = AsyncOperation::CreateAndStart<ApplicationHostCodePackageAsyncOperation>(
        *this,
        requestBody,
        move(context),
        [this](AsyncOperationSPtr const & operation)
        {
            this->EndApplicationHostCodePackageOperation(operation);
        },
        this->Root.CreateAsyncOperationRoot());
}

void ApplicationHostManager::SendApplicationHostCodePackageOperationReply(
    ErrorCode error,
    __in IpcReceiverContextUPtr && context)
{
    ApplicationHostCodePackageOperationReply replyBody(error);
    auto reply = make_unique<Message>(replyBody);
    context->Reply(move(reply));

    WriteNoise(
        TraceType,
        this->Root.TraceId,
        "SendApplicationHostCodePackageOperationReply: ReplyBody={1}",
        replyBody);
}

void ApplicationHostManager::ProcessGetFabricProcessSidRequest(
    __in Message & message,
    __in IpcReceiverContextUPtr & context)
{
    UNREFERENCED_PARAMETER(message);

    WriteNoise(
        TraceType,
        Root.TraceId,
        "Processing GetFabricProcessSidRequest");

    GetFabricProcessSidReply replyBody(processSid_, processId_);
    MessageUPtr reply = make_unique<Message>(replyBody);
    context->Reply(move(reply));
    WriteNoise(
        TraceType,
        Root.TraceId,
        "Sent GetFabricProcessSidReply: ReplyBody={0}",
        replyBody);
}

#pragma region StartRegisterApplicationHost Message Processing

// ********************************************************************************************************************
// StartRegisterApplicationHost Message Processing
//

void ApplicationHostManager::ProcessStartRegisterApplicationHostRequest(
    __in Message & message, 
    __in IpcReceiverContextUPtr & context)
{
    StartRegisterApplicationHostRequest requestBody;
    if (!message.GetBody<StartRegisterApplicationHostRequest>(requestBody))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType,
            Root.TraceId,
            "GetBody<StartRegisterApplicationHostRequest> failed: Message={0}, ErrorCode={1}",
            message,
            error);
        return;
    }

    WriteNoise(
        TraceType,
        Root.TraceId,
        "Processing StartRegisterApplicationHostRequest: HostId={0}, HostType={1}, HostProcessId={2}, Timeout={3}",
        requestBody.Id,
        requestBody.Type,
        requestBody.ProcessId,
        requestBody.Timeout);

    wstring hostId(requestBody.Id);

    // step 1: If the host is a container host then create a ContainerLogProcessMarkupFile so that DCA can monitor the container log directory.
    if (requestBody.IsContainerHost)
    {
        ContainerHelper::GetContainerHelper().MarkContainerLogFolder(hostId, true /*forProcessing*/);
    }

    ErrorCode error(ErrorCodeValue::Success);
    ApplicationHostRegistrationSPtr registration;
    ApplicationHostProxySPtr hostProxySPtr;

    //Todo: If we remove support for NonActivatedApplicationHosts we can return error for regsitrations if hostId is not present.
    auto result = activationTable_->Find(hostId, hostProxySPtr);

    // step 2: If pending Update then abort to update the versions.
    bool doesAppHostNeedsUpdate = false;
    {
        AcquireReadLock lock(updateContextlock_);
        // If no AppHost present then it already went down.
        if (result.IsSuccess() && hostProxySPtr->IsUpdateContextPending)
        {
            doesAppHostNeedsUpdate = true;
        }
        else
        {
            // step 3: create entry in the registration table
            registration = make_shared<ApplicationHostRegistration>(requestBody.Id, requestBody.Type);
            error = registrationTable_->Add(registration);
            WriteTrace(
                error.ToLogLevel(),
                TraceType,
                Root.TraceId,
                "StartRegisterApplicationHostRequest: Add Registration: ErrorCode={0}, HostId={1}",
                error,
                requestBody.Id);
        }
    }

    if (doesAppHostNeedsUpdate)
    {
        WriteInfo(
            TraceType,
            Root.TraceId,
            "StartRegisterApplicationHostRequest: Aborting AppHostProxy since update was pending for it. HostId={0}",
            hostId);
        hostProxySPtr->Abort();
        return;
    }

    if (!error.IsSuccess())
    {
        this->SendStartRegisterApplicationHostReply(requestBody, error, context);
        return;
    }

    // step 4: create a timer for the registration process to finish
    auto root = Root.CreateComponentRoot();
    TimeSpan timeout = requestBody.Timeout;
    TimerSPtr finishRegistrationTimer = Timer::Create(
        "Hosting.ApplicationHostRegistrationTimedout",
        [root, this, hostId, timeout] (TimerSPtr const & timer) 
    { this->OnApplicationHostRegistrationTimedout(hostId, timeout, timer); });
    finishRegistrationTimer->Change(timeout);
    error = registration->OnStartRegistration(move(finishRegistrationTimer));
    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        Root.TraceId,
        "StartRegisterApplicationHostRequest: OnStartRegistration: ErrorCode={0}, HostId={1}",
        error,
        requestBody.Id);
    if (!error.IsSuccess()) 
    {
        this->SendStartRegisterApplicationHostReply(requestBody, error, context);
        return;
    }

    if(requestBody.Type == ApplicationHostType::Enum::NonActivated)
    {
        // step 5: establish monitoring with the application host
        HandleUPtr appHostProcessHandle;
        error = ProcessUtility::OpenProcess(
            SYNCHRONIZE, 
            FALSE, 
            requestBody.ProcessId, 
            appHostProcessHandle);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            Root.TraceId,
            "StartRegisterNonActivatedApplicationHost: OpenProcess: ErrorCode={0}, HostId={1}",
            error,
            requestBody.Id);
        if (!error.IsSuccess()) 
        {
            this->SendStartRegisterApplicationHostReply(requestBody, error, context);
            return;
        }

        auto hostMonitor = ProcessWait::CreateAndStart(
            move(*appHostProcessHandle),
            requestBody.ProcessId,
            [root, this, hostId](pid_t, ErrorCode const & waitResult, DWORD)
        {
            this->OnRegisteredApplicationHostTerminated(ActivityDescription(ActivityId(), ActivityType::Enum::ServicePackageEvent), hostId, waitResult);
        });
        error = registration->OnMonitoringInitialized(move(hostMonitor));
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            Root.TraceId,
            "StartRegisterApplicationHostRequest: OnMonitoringInitialized: ErrorCode={0}, HostId={1}",
            error,
            requestBody.Id);
        if (!error.IsSuccess()) 
        {
            this->SendStartRegisterApplicationHostReply(requestBody, error, context);
            return;
        }
    }
    // step 6: send success response
    this->SendStartRegisterApplicationHostReply(requestBody, error, context);
    return;
}

void ApplicationHostManager::SendStartRegisterApplicationHostReply(
    StartRegisterApplicationHostRequest const & requestBody,
    ErrorCode const errorArg,
    __in Transport::IpcReceiverContextUPtr & context)
{
    unique_ptr<StartRegisterApplicationHostReply> replyBody;

    ErrorCode error(errorArg.ReadValue());

    // the lease handle can be null during lease agent close or restart
    if (hosting_.LeaseAgentObj.LeaseHandle == NULL)
    {
        error.Overwrite(ErrorCodeValue::ObjectClosed);
    }

#if defined(PLATFORM_UNIX)
    auto deploymentFolder = hosting_.DeploymentFolder;
#else
    // If the runtime is a container host the deployment folder changes accordingly since we do not mount exact path on windows
    auto deploymentFolder = requestBody.IsContainerHost ? HostingConfig::GetConfig().ContainerAppDeploymentRootFolder : hosting_.DeploymentFolder;
#endif

    if (error.IsSuccess())
    {
        TimeSpan initialLeaseTTL;

#ifdef PLATFORM_UNIX
        LONG leaseTTL_InMilliseconds = 0;
        LONGLONG currentTime = 0;
        if (hosting_.LeaseAgentObj.GetLeasingApplicationExpirationTimeFromIPC(&leaseTTL_InMilliseconds, &currentTime) == FALSE)
        {
            auto err = ::GetLastError();
            WriteWarning(TraceType, "SendStartRegisterApplicationHostReply: GetLeasingApplicationExpirationTimeFromIPC failed: 0x{0:x}", err);
        }

        if (MAXLONG == leaseTTL_InMilliseconds)
        {
            initialLeaseTTL = hosting_.LeaseAgentObj.LeaseDuration();
            WriteInfo(TraceType, Root.TraceId, "SendStartRegisterApplicationHostReply: initialLeaseTTL: {0}, GetLeasingApplicationExpirationTimeFromIPC returned MAXLONG TTL, LeaseDuration is used", initialLeaseTTL);
        }
        else
        {
            initialLeaseTTL = TimeSpan::FromMilliseconds(leaseTTL_InMilliseconds);
            WriteInfo(TraceType, Root.TraceId, "SendStartRegisterApplicationHostReply: initialLeaseTTL: {0}", initialLeaseTTL);
        }

#endif

        replyBody = make_unique<StartRegisterApplicationHostReply>(
            hosting_.NodeId,
            hosting_.NodeInstanceId,
            hosting_.NodeName,
            processId_,
            hosting_.NodeType,
            hosting_.FabricNodeConfigObj.IPAddressOrFQDN,
            hosting_.ClientConnectionAddress,
            deploymentFolder,
            hosting_.LeaseAgentObj.LeaseHandle,
            initialLeaseTTL,
            hosting_.LeaseAgentObj.LeaseDuration(),
            ErrorCode(ErrorCodeValue::Success),
            hosting_.NodeWorkFolder,
            hosting_.FabricNodeConfigObj.LogicalApplicationDirectories,
            hosting_.FabricNodeConfigObj.LogicalNodeDirectories,
            hosting_.ApplicationSharedLogSettings,
            hosting_.SystemServicesSharedLogSettings);
    }
    else
    {
        replyBody = make_unique<StartRegisterApplicationHostReply>(
            hosting_.NodeId,
            hosting_.NodeInstanceId,
            hosting_.NodeName,
            0,
            hosting_.NodeType,
            hosting_.FabricNodeConfigObj.IPAddressOrFQDN,
            hosting_.ClientConnectionAddress,
            deploymentFolder,
            HANDLE(),
            TimeSpan(),
            TimeSpan::Zero,
            error,
            hosting_.NodeWorkFolder,
            hosting_.FabricNodeConfigObj.LogicalApplicationDirectories,
            hosting_.FabricNodeConfigObj.LogicalNodeDirectories,
            hosting_.ApplicationSharedLogSettings,
            hosting_.SystemServicesSharedLogSettings);
    }

    MessageUPtr reply = make_unique<Message>(*replyBody);
    context->Reply(move(reply));
    WriteNoise(
        TraceType,
        Root.TraceId,
        "Sent StartRegisterApplicationHostReply: RequestBody={0}, ReplyBody={1}", 
        requestBody,
        *replyBody);
}

void ApplicationHostManager::OnApplicationHostRegistrationTimedout(
    wstring const & hostId, 
    TimeSpan timeout, 
    TimerSPtr const & timer)
{
    timer->Cancel();
    WriteWarning(
        TraceType,
        Root.TraceId,
        "ApplicationHostRegistrationTimedout: HostId={0}, Timeout={1}",
        hostId,
        timeout);

    ApplicationHostRegistrationSPtr registration;
    auto error = registrationTable_->Find(hostId, registration);
    WriteNoise(
        TraceType,
        Root.TraceId,
        "ApplicationHostRegistrationTimedout:Find Registration: ErrorCode={0}, HostId={1}",
        error,
        hostId);
    if (!error.IsSuccess())  { return; }

    error = registration->OnRegistrationTimedout();
    if (!error.IsSuccess())  { return; }

    error = registrationTable_->Remove(hostId, registration);
    WriteNoise(
        TraceType,
        Root.TraceId,
        "ApplicationHostRegistrationTimedout:Remove Registration: ErrorCode={0}, HostId={1}",
        error,
        hostId);
    if (!error.IsSuccess())  { return; }

    return;
}
#pragma endregion 

#pragma region FinishRegisterApplicationHost Message Processing

// ********************************************************************************************************************
// FinishRegisterApplicationHost Message Processing
//

void ApplicationHostManager::ProcessFinishRegisterApplicationHostRequest(
    __in Message & message, 
    __in IpcReceiverContextUPtr & context)
{
    FinishRegisterApplicationHostRequest requestBody;
    if (!message.GetBody<FinishRegisterApplicationHostRequest>(requestBody))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType,
            Root.TraceId,
            "GetBody<FinishRegisterApplicationHostRequest> failed: Message={0}, ErrorCode={1}",
            message,
            error);
        return;
    }

    WriteNoise(
        TraceType,
        Root.TraceId,
        "Processing FinishRegisterApplicationHostRequest: HostId={0}",
        requestBody.Id);

    // find the registration entry
    ApplicationHostRegistrationSPtr registration;
    auto error = registrationTable_->Find(requestBody.Id, registration);
    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        Root.TraceId,
        "FinishRegisterApplicationHost: Find Registration: ErrorCode={0}, HostId={1}",
        error,
        requestBody.Id);
    if (!error.IsSuccess()) 
    {
        this->SendFinishRegisterApplicationHostReply(requestBody, error, context);
        return;
    }

    // complete the registration process
    registration->OnFinishRegistration();
    this->SendFinishRegisterApplicationHostReply(requestBody, ErrorCode(ErrorCodeValue::Success), context);
    this->OnApplicationHostRegistered(registration);
}

void ApplicationHostManager::SendFinishRegisterApplicationHostReply(
    FinishRegisterApplicationHostRequest const & requestBody,
    ErrorCode const error,
    __in Transport::IpcReceiverContextUPtr & context)
{
    FinishRegisterApplicationHostReply replyBody(error);
    MessageUPtr reply = make_unique<Message>(replyBody);
    context->Reply(move(reply));
    WriteNoise(
        TraceType,
        Root.TraceId,
        "Sent FinishRegisterApplicationHostReply: RequestBody={0}, ReplyBody={1}",
        requestBody,
        replyBody);
}

#pragma endregion 

#pragma region ApplicationHost Unregistration Processing

void ApplicationHostManager::ProcessUnregisterApplicationHostRequest(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    UnregisterApplicationHostRequest requestBody;
    if (!message.GetBody<UnregisterApplicationHostRequest>(requestBody))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType,
            Root.TraceId,
            "GetBody<UnregisterApplicationHostRequest> failed: Message={0}, ErrorCode={1}",
            message,
            error);
        return;
    }

    WriteNoise(
        TraceType,
        Root.TraceId,
        "Processing UnregisterApplicationHostRequest: HostId={0} ActivityDescription={1}",
        requestBody.Id,
        requestBody.ActivityDescription);

    // remove registration from registrationt table
    ApplicationHostRegistrationSPtr registration;
    auto error = registrationTable_->Remove(requestBody.Id, registration);
    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        Root.TraceId,
        "FinishRegisterApplicationHost: Remove Registration: ErrorCode={0}, HostId={1} ActivityDescription={2}",
        error,
        requestBody.Id,
        requestBody.ActivityDescription);

    // send the reponse
    this->SendUnregisterApplicationHostReply(requestBody, error, context);
    if (error.IsSuccess())
    {
        this->OnApplicationHostUnregistered(requestBody.ActivityDescription, registration->HostId);
    }

    hosting_.IpcServerObj.RemoveClient(requestBody.Id); 
}

void ApplicationHostManager::SendUnregisterApplicationHostReply(
    UnregisterApplicationHostRequest const & requestBody,
    ErrorCode const error,
    __in Transport::IpcReceiverContextUPtr & context)
{
    UnregisterApplicationHostReply replyBody(error);
    MessageUPtr reply = make_unique<Message>(replyBody);
    context->Reply(move(reply));
    WriteNoise(
        TraceType,
        Root.TraceId,
        "Sent UnregisterApplicationHostReply: RequestBody={0}, ReplyBody={1}",
        requestBody,
        replyBody);
}

#pragma endregion

#pragma region CodePackageTerminationNotification processing

void ApplicationHostManager::ProcessCodePackageTerminationNotification(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    CodePackageTerminationNotificationRequest requestBody;
    if (!message.GetBody<CodePackageTerminationNotificationRequest>(requestBody))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType,
            Root.TraceId,
            "GetBody<CodePackageTerminationNotificationRequest> failed: Message={0}, ErrorCode={1}",
            message,
            error);
        return;
    }

    WriteNoise(
        TraceType,
        Root.TraceId,
        "Processing CodePackageTerminationNotificationRequest: CodePackageId={0}, ActivationId={1}",
        requestBody.CodePackageInstanceId,
        requestBody.ActivationId);

    // find the application host from activated host table
    ApplicationHostProxySPtr hostProxy;
    auto error = this->activationTable_->Find(requestBody.ActivationId.HostId, hostProxy);
    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        Root.TraceId,
        "CodePackageTerminationNotificationRequest Processing: FindProxyFromActivationTable: ErrorCode={0}, HostId={1}",
        error,
        requestBody.ActivationId.HostId);
    if (!error.IsSuccess())
    {
        // no host entry found, maybe stale message, send reply
        SendCodePackageTerminationNotificationReply(requestBody, context);
        return;
    }

    if (hostProxy->HostType != ApplicationHostType::Activated_MultiCodePackage)
    {
        WriteError(
            TraceType,
            Root.TraceId,
            "CodePackageTerminationNotificationRequest Received from invalid HostType. Request={0}, HostId={1}, HostType={2}",
            requestBody,
            hostProxy->HostId,
            hostProxy->HostType);

        // discard the request, do not reply
        return;
    }

    hostProxy->OnCodePackageTerminated(requestBody.CodePackageInstanceId, requestBody.ActivationId);
    SendCodePackageTerminationNotificationReply(requestBody, context);
}

void ApplicationHostManager::SendCodePackageTerminationNotificationReply(
    CodePackageTerminationNotificationRequest const & requestBody,
    __in Transport::IpcReceiverContextUPtr & context)
{
    MessageUPtr reply = make_unique<Message>();
    context->Reply(move(reply));
    WriteNoise(
        TraceType,
        Root.TraceId,
        "Sent CodePackageTerminationNotificationReply: RequestBody={0}",
        requestBody);
}

void Hosting2::ApplicationHostManager::ProcessRegisterResourceMonitorService(Transport::Message & message, Transport::IpcReceiverContextUPtr & context)
{
    Management::ResourceMonitor::ResourceMonitorServiceRegistration requestBody;
    if (!message.GetBody<Management::ResourceMonitor::ResourceMonitorServiceRegistration>(requestBody))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType,
            Root.TraceId,
            "GetBody<ResourceMonitorServiceRegistration> failed: Message={0}, ErrorCode={1}",
            message,
            error);
        return;
    }

    WriteInfo(
        TraceType,
        Root.TraceId,
        "Processing RM service registration {0}",
        requestBody);

    activationTable_->StartReportingToRM(requestBody.Id);

    SendRegisterResourceMonitorServiceReply(context);
}

void Hosting2::ApplicationHostManager::SendRegisterResourceMonitorServiceReply(Transport::IpcReceiverContextUPtr & context)
{
    //return back the address where the RM can talk to FabricHost
    wstring activatorAddress;
    Environment::GetEnvironmentVariable(Constants::FabricActivatorAddressEnvVariable, activatorAddress, NOTHROW());

    Management::ResourceMonitor::ResourceMonitorServiceRegistrationResponse response(activatorAddress);
    MessageUPtr reply = make_unique<Message>(response);
    WriteInfo(
        TraceType,
        Root.TraceId,
        "Sent Registration reply to RM service {0}",
        response.FabricHostAddress);

    context->Reply(move(reply));

}

#pragma endregion

#pragma region ApplicationHost Notifications

// ********************************************************************************************************************
// ApplicationHost Notifications
//

void ApplicationHostManager::OnApplicationHostRegistered(ApplicationHostRegistrationSPtr const & registration)
{
    WriteNoise(
        TraceType,
        Root.TraceId,
        "OnApplicationHostRegistered: Registration={0}",
        *registration);

    ApplicationHostProxySPtr hostProxy;
    auto error = activationTable_->Find(registration->HostId, hostProxy);
    if (!error.IsSuccess())
    {
        return;
    }

    WriteNoise(
        TraceType,
        Root.TraceId,
        "ActivatedApplicationHost Registered: HostId={0}",
        registration->HostId);
    hostProxy->OnApplicationHostRegistered();
}

void ApplicationHostManager::OnApplicationHostUnregistered(ActivityDescription const & activityDescription, wstring const & hostId)
{
    WriteNoise(
        TraceType,
        Root.TraceId,
        "OnApplicationHostUnregistered: HostId={0} ActivityDescription={1}",
        hostId,
        activityDescription);

    // notify fabric runtime manager to take action on application host doing down
    hosting_.FabricRuntimeManagerObj->OnApplicationHostUnregistered(activityDescription, hostId);
}

void ApplicationHostManager::OnRegisteredApplicationHostTerminated(
    ActivityDescription const & activityDescritption,
    wstring const & hostId,
    ErrorCode const & waitResult)
{
    waitResult; // todo, check waitResult

    LONG state = this->State.Value;
    if (state > FabricComponentState::Opened)
    {
        WriteNoise(
            TraceType,
            Root.TraceId,
            "Ignoring termination of registered Application Host: HostId={0} as current state is {1}",
            hostId,
            state);
        return;
    }

    WriteNoise(
        TraceType,
        Root.TraceId,
        "Processing termination of registered Application Host: HostId={0} ActivityDescription={1}",
        hostId,
        activityDescritption);

    hosting_.IpcServerObj.RemoveClient(hostId); 

    ApplicationHostRegistrationSPtr registration;
    registrationTable_->Remove(hostId, registration).ReadValue();
    this->OnApplicationHostUnregistered(activityDescritption, hostId);
}

void ApplicationHostManager::OnActivatedApplicationHostTerminated(
    EventArgs const & eventArgs)
{
    ApplicationHostTerminatedEventArgs args = dynamic_cast<ApplicationHostTerminatedEventArgs const &>(eventArgs);

    OnApplicationHostTerminated(args.ActivityDescription, args.HostId, args.ExitCode);
}

void ApplicationHostManager::OnApplicationHostTerminated(
    ActivityDescription const & activityDescription,
    wstring const & hostId,
    DWORD exitCode)
{
    LONG state = this->State.Value;
    if (state > FabricComponentState::Opened)
    {
        WriteNoise(
            TraceType,
            Root.TraceId,
            "Ignoring termination of activated Application Host: HostId={0} as current state is {1}",
            hostId,
            state);
        return;
    }

    WriteTrace(
        (exitCode == 0 || exitCode == ProcessActivator::ProcessDeactivateExitCode || exitCode == STATUS_CONTROL_C_EXIT) ? LogLevel::Info : LogLevel::Warning,
        TraceType,
        Root.TraceId,
        "Processing termination of activated Application Host: HostId={0}, ExitCode={1} ActivityDescription={2}",
        hostId,
        exitCode,
        activityDescription);

    hostingTrace.ApplicationHostTerminated(hostId, exitCode, activityDescription);

    this->OnRegisteredApplicationHostTerminated(activityDescription, hostId, ErrorCode(ErrorCodeValue::InvalidArgument));

    ApplicationHostProxySPtr hostProxy;
    auto error = this->activationTable_->Remove(hostId, hostProxy);
    
    if (!error.IsSuccess()) { return; }

    hostProxy->OnApplicationHostTerminated(exitCode);
}

void ApplicationHostManager::OnContainerHealthCheckStatusChanged(EventArgs const & eventArgs)
{
    auto args = dynamic_cast<ContainerHealthCheckStatusChangedEventArgs const &>(eventArgs);

    for (auto const & healthInfo : args.HealthStatusInfoList)
    {
        ApplicationHostProxySPtr hostProxy;

        auto error = activationTable_->Find(healthInfo.HostId, hostProxy);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                Root.TraceId,
                "OnContainerHealthCheckStatusChanged: HostId not found for HealthInfo={0}",
                healthInfo);

            return;
        }

        hostProxy->OnContainerHealthCheckStatusChanged(healthInfo);
    }
}

#pragma endregion
