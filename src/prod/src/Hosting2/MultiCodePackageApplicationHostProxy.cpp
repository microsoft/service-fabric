// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Management::ImageModel;
using namespace Hosting2;
using namespace Transport;

StringLiteral const TraceType("MultiCodePackageApplicationHostProxy");

// ********************************************************************************************************************
// MultiCodePackageApplicationHostProxy::CodePackageTable Implementation
//
class MultiCodePackageApplicationHostProxy::CodePackageTable : public RootedObject
{
    DENY_COPY(CodePackageTable)

public:
    CodePackageTable(ComponentRoot const & root)
        : RootedObject(root),
        lock_(),
        map_(),
        isClosed_(false)
    {
    }

    ErrorCode Add(CodePackageActivationId const & activationId, CodePackageInstanceSPtr const & codePackageInstance)
    {
        {
            AcquireWriteLock lock(lock_);
            if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

            auto iter = map_.find(codePackageInstance->CodePackageInstanceId);
            if (iter == map_.end())
            {
                map_.insert(
                    make_pair(
                        codePackageInstance->CodePackageInstanceId,
                        make_shared<Entry>(activationId, codePackageInstance)));
                return ErrorCode(ErrorCodeValue::Success);
            }
            else
            {
                return ErrorCode(ErrorCodeValue::HostingCodePackageAlreadyHosted);
            }
        }
    }

    size_t Size()
    {
        {
            AcquireReadLock lock(lock_);
            return map_.size();
        }
    }

    vector<pair<CodePackageActivationId, CodePackageInstanceSPtr>> Items()
    {
        vector<pair<CodePackageActivationId, CodePackageInstanceSPtr>> items;

        {
            AcquireReadLock lock(lock_);
            for (auto iter = map_.cbegin(); iter != map_.cend(); ++iter)
            {
                items.push_back(make_pair(iter->second->activationId_, iter->second->codePackageInstance_));
            }
        }

        return items;
    }

    ErrorCode Remove(
        CodePackageInstanceIdentifier const & codePackageInstanceId,
        CodePackageActivationId const & activationId,
        __out CodePackageInstanceSPtr & codePackageInstance)
    {
        {
            AcquireWriteLock lock(lock_);
            if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

            auto iter = map_.find(codePackageInstanceId);
            if ((iter == map_.end()) || (iter->second->activationId_ != activationId))
            {
                return ErrorCode(ErrorCodeValue::HostingCodePackageNotHosted);
            }
            else
            {
                codePackageInstance = iter->second->codePackageInstance_;
                map_.erase(iter);

                return ErrorCode(ErrorCodeValue::Success);
            }
        }
    }

    vector<pair<CodePackageActivationId, CodePackageInstanceSPtr>> Close()
    {
        vector<pair<CodePackageActivationId, CodePackageInstanceSPtr>> removed;
        {
            AcquireWriteLock lock(lock_);
            if (!isClosed_)
            {
                isClosed_ = true;
                for (auto iter = map_.begin(); iter != map_.end(); ++iter)
                {
                    removed.push_back(make_pair(iter->second->activationId_, iter->second->codePackageInstance_));
                }
                map_.clear();
            }
        }

        return removed;
    }

private:
    struct Entry
    {
        DENY_COPY(Entry)
    public:
        Entry(CodePackageActivationId const & activationId, CodePackageInstanceSPtr const & codePackageInstance)
            : activationId_(activationId), codePackageInstance_(codePackageInstance)
        {
        }

        CodePackageActivationId const activationId_;
        CodePackageInstanceSPtr const codePackageInstance_;
    };
    typedef shared_ptr<Entry> EntrySPtr;

private:
    Common::RwLock lock_;
    map<CodePackageInstanceIdentifier, EntrySPtr> map_;
    bool isClosed_;
};

// ********************************************************************************************************************
// MultiCodePackageApplicationHostProxy::OpenAsyncOperation Implementation
//
class MultiCodePackageApplicationHostProxy::OpenAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(OpenAsyncOperation)

public:
    OpenAsyncOperation(
        __in MultiCodePackageApplicationHostProxy & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        processPath_()
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
        WriteInfo(
            TraceType,
            owner_.TraceId,
            "Opening MultiCodePackageApplicationHostProxy: Id={0}, Type={1}, Timeout={2}",
            owner_.HostId,
            owner_.HostType,
            timeoutHelper_.GetRemainingTime());

        ActivateProcess(thisSPtr);
    }

private:
    void ActivateProcess(AsyncOperationSPtr const & thisSPtr)
    {
        ProcessDescriptionUPtr processDescription;
        auto error = owner_.GetProcessDescription(processDescription);

        processPath_ = processDescription->ExePath;
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "GetProcessDescription: ErrorCode={0}",
                error);
            TryComplete(thisSPtr, error);
            return;
        }

        TimeSpan timeout = timeoutHelper_.GetRemainingTime();
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(ProcessActivate): HostId={0}, HostType={1}, Timeout={2}",
            owner_.HostId,
            owner_.HostType,
            timeout);

        auto operation = owner_.Hosting.ApplicationHostManagerObj->FabricActivator->BeginActivateProcess(
            owner_.ApplicationId.ToString(),
            owner_.HostId,
            processDescription,
            owner_.RunAsId,
            false,
            move(make_unique<ContainerDescription>()),
            timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnProcessActivated(operation); },
            thisSPtr);

        if (operation->CompletedSynchronously)
        {
            FinishActivateProcess(operation);
        }
    }

    void OnProcessActivated(AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            FinishActivateProcess(operation);
        }
    }

    void FinishActivateProcess(AsyncOperationSPtr const & operation)
    {
        DWORD processId;
        auto error = owner_.Hosting.ApplicationHostManagerObj->FabricActivator->EndActivateProcess(operation, processId);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "End(ProcessActivate): ErrorCode={0}, HostId={1}",
            error,
            owner_.HostId);

        if (error.IsSuccess())
        {
            owner_.codePackageRuntimeInformation_ = make_shared<CodePackageRuntimeInformation>(processPath_, processId);
#if !defined(PLATFORM_UNIX)
            error = SetupAppHostMonitor(processId);
#endif
        }

        TryComplete(operation->Parent, error);
        if (owner_.terminatedExternally_.load())
        {
            WriteInfo(
                TraceType,
                owner_.TraceId,
                "ApplicationHost {0} terminated externally during open with exit code {1}, aborting",
                owner_.HostId,
                owner_.exitCode_.load());
            
            //process terminated while applicationhost was being opened
            owner_.Abort();
        }
    }

    ErrorCode SetupAppHostMonitor(DWORD processId)
    {
        if (!isContainerHost_)
        {
            HandleUPtr appHostProcessHandle;
            DWORD desiredAccess = SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION;

            auto err = ProcessUtility::OpenProcess(
                desiredAccess,
                FALSE,
                processId,
                appHostProcessHandle);
            if (!err.IsSuccess())
            {
                WriteWarning(
                    TraceType,
                    "MultiCodePackageApplicationHostProxy: OpenProcess: ErrorCode={0}, ProcessId={1}, ProcessPath={2}, AppHostId={3}. This means that process likely crashsed before Fabric could monitor the AppHost. Filter by AppHostId to get the lifecycle.",
                    err,
                    processId,
                    processPath_,
                    owner_.HostId);
            }
            else
            {
                HostingSubsystem & hosting = owner_.Hosting;
                wstring hostId = owner_.HostId;
                owner_.procHandle_ = move(appHostProcessHandle);
                auto hostMonitor = ProcessWait::CreateAndStart(
                    Handle(owner_.procHandle_->Value, Handle::DUPLICATE()),
                    processId,
                    [&hosting, hostId](pid_t, ErrorCode const &, DWORD exitCode)
                {
                    //LINUXTODO, what happens waitResult indicates wait failed?
                    hosting.ApplicationHostManagerObj->OnApplicationHostTerminated(ActivityDescription(ActivityId(), ActivityType::Enum::ServicePackageEvent), hostId, exitCode);
                });

                owner_.apphostMonitor_ = move(hostMonitor);
            }
        }
        return ErrorCodeValue::Success;
    }

private:
    MultiCodePackageApplicationHostProxy & owner_;
    TimeoutHelper const timeoutHelper_;
    std::wstring processPath_;
    bool isContainerHost_;
};

// ********************************************************************************************************************
// MultiCodePackageApplicationHostProxy::CloseAsyncOperation Implementation
//
class MultiCodePackageApplicationHostProxy::CloseAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(CloseAsyncOperation)

public:
    CloseAsyncOperation(
        __in MultiCodePackageApplicationHostProxy & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout)
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
        TimeSpan timeout = timeoutHelper_.GetRemainingTime();

        WriteInfo(
            TraceType,
            owner_.TraceId,
            "Closing ApplicationHostProxy: Id={0}, Type={1}, Timeout={2}",
            owner_.HostId,
            owner_.HostType,
            timeout);

        if (owner_.apphostMonitor_ != nullptr)
        {
            owner_.apphostMonitor_->Cancel();
        }
        if (!owner_.terminatedExternally_.load())
        {
            owner_.exitCode_.store(ProcessActivator::ProcessDeactivateExitCode);

            auto operation = owner_.Hosting.ApplicationHostManagerObj->FabricActivator->BeginDeactivateProcess(
                owner_.HostId, 
                timeout,
                [this](AsyncOperationSPtr const & operation) { this->OnProcessTerminated(operation); },
                thisSPtr);

            if (operation->CompletedSynchronously)
            {
                FinishTerminateProcess(operation);
            }
        }
        else
        {
            WriteNoise(
                TraceType,
                owner_.TraceId,
                "terminatedExternally_ is true. Host process already closed.");

            ErrorCode error = ErrorCode(ErrorCodeValue::Success);
            NotifyTermination(error);
            TryComplete(thisSPtr, error);
        }
    }

private:
    void OnProcessTerminated(AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            FinishTerminateProcess(operation);
        }
    }

    void FinishTerminateProcess(AsyncOperationSPtr const & operation)
    {
        auto error = owner_.Hosting.ApplicationHostManagerObj->FabricActivator->EndDeactivateProcess(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "ApplicationHostProxy Closed. ErrorCode={0}",
            error);

        NotifyTermination(error);

        TryComplete(operation->Parent, error);
    }

    void NotifyTermination(ErrorCode error)
    {
        if (error.IsSuccess())
        {
            auto removed = owner_.hostedCodePackageTable_->Close();
            if (!removed.empty())
            {
                owner_.NotifyTermination(removed);
            }
        }
    }

private:
    MultiCodePackageApplicationHostProxy & owner_;
    TimeoutHelper const timeoutHelper_;
};

// ********************************************************************************************************************
// MultiCodePackageApplicationHostProxy::ActivateCodePackageAsyncOperation Implementation
//
class MultiCodePackageApplicationHostProxy::ActivateCodePackageAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(ActivateCodePackageAsyncOperation)

public:
    ActivateCodePackageAsyncOperation(
        __in MultiCodePackageApplicationHostProxy & owner,
        CodePackageInstanceSPtr const & codePackageInstance,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        codePackageInstance_(codePackageInstance),
        timeoutHelper_(timeout),
        activationId_(owner.HostId)
    {
    }

    virtual ~ActivateCodePackageAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, CodePackageActivationId & activationId)
    {
        auto thisPtr = AsyncOperation::End<ActivateCodePackageAsyncOperation>(operation);
        if (thisPtr->Error.IsSuccess())
        {
            activationId = thisPtr->activationId_;
        }
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Activating CodePackage: CodePackageInstanceId={0}, HostId={1}, HostType={2}",
            codePackageInstance_->CodePackageInstanceId,
            owner_.HostId,
            owner_.HostType);

        WaitForRegistration(thisSPtr);
    }

    void WaitForRegistration(AsyncOperationSPtr const & thisSPtr)
    {
        TimeSpan timeout = timeoutHelper_.GetRemainingTime();

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(WaitForRegistration): HostId={0}",
            owner_.HostId);
        auto operation = owner_.hostRegisteredEvent_.BeginWaitOne(
            timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnApplicationHostRegistered(operation); },
            thisSPtr);
        if (operation->CompletedSynchronously)
        {
            FinishWaitForRegistration(operation);
        }
    }

    void OnApplicationHostRegistered(AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            FinishWaitForRegistration(operation);
        }
    }

    void FinishWaitForRegistration(AsyncOperationSPtr const & operation)
    {
        auto error = owner_.hostRegisteredEvent_.EndWaitOne(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "End(WaitForRegistration): HostId={0}",
            owner_.HostId);
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }

        RequestCodePackageActivation(operation->Parent);
    }

    void RequestCodePackageActivation(AsyncOperationSPtr const & thisSPtr)
    {
        TimeSpan timeout = timeoutHelper_.GetRemainingTime();
        auto request = CreateActivateCodePackageRequestMessage(timeout);
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(RequestCodePackageActivation): HostId={0}, Request={1}, Timeout={2}",
            owner_.HostId,
            *request,
            timeout);

        auto operation = owner_.Hosting.IpcServerObj.BeginRequest(
            move(request),
            owner_.HostId,
            timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnCodePackageActivationReplyReceived(operation); },
            thisSPtr);

        if (operation->CompletedSynchronously)
        {
            FinishCodePackageActivation(operation);
        }
    }

    void OnCodePackageActivationReplyReceived(AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            FinishCodePackageActivation(operation);
        }
    }

    void FinishCodePackageActivation(AsyncOperationSPtr const & operation)
    {
        MessageUPtr reply;
        auto error = owner_.Hosting.IpcServerObj.EndRequest(operation, reply);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "End(RequestCodePackageActivation): HostId={0}, CodePackageInstanceId={1} ErrorCode={2}",
                owner_.HostId,
                codePackageInstance_->CodePackageInstanceId,
                error);

            TryComplete(operation->Parent, error);

            // abort the application host proxy, we do not know the status of the proxy
            owner_.Abort();
            return;
        }

        // process the reply
        ActivateCodePackageReply replyBody;
        if (!reply->GetBody<ActivateCodePackageReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "GetBody<ActivateCodePackageReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);
            TryComplete(operation->Parent, error);
            return;
        }

        error = replyBody.Error;
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "ActivateCodePackageReply: HostId={0}, Message={1}, Body={2}",
            owner_.HostId,
            *reply,
            replyBody);
        if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::HostingCodePackageAlreadyHosted))
        {
            TryComplete(operation->Parent, error);
            return;
        }

        AddCodePackageToTable(operation->Parent);
    }

    void AddCodePackageToTable(AsyncOperationSPtr const & thisSPtr)
    {
        // add this code package to the list of code packages being hosted by this application host
        auto error = owner_.hostedCodePackageTable_->Add(activationId_, codePackageInstance_);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "Failed to add CodePackage to HostedCodePackageTable ErrorCode={0}, Host: HostId={1}, CodePackageId={2}",
                error,
                owner_.HostId,
                codePackageInstance_->CodePackageInstanceId);
            TryComplete(thisSPtr, error);
            owner_.Abort();
            return;
        }
        TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
    }

    MessageUPtr CreateActivateCodePackageRequestMessage(TimeSpan const timeout)
    {
        ActivateCodePackageRequest requestBody(codePackageInstance_->Context, activationId_, timeout);

        MessageUPtr request = make_unique<Message>(requestBody);
        request->Headers.Add(Transport::ActorHeader(Actor::ApplicationHost));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::ActivateCodePackageRequest));

        WriteNoise(TraceType, owner_.TraceId, "CreateActivateCodePackageRequest: Message={0}, Body={1}", *request, requestBody);

        return move(request);
    }
private:
    MultiCodePackageApplicationHostProxy & owner_;
    CodePackageInstanceSPtr const codePackageInstance_;
    TimeoutHelper const timeoutHelper_;
    CodePackageActivationId const activationId_;
};

// ********************************************************************************************************************
// MultiCodePackageApplicationHostProxy::DeactivateCodePackageAsyncOperation Implementation
//
class MultiCodePackageApplicationHostProxy::DeactivateCodePackageAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(DeactivateCodePackageAsyncOperation)

public:
    DeactivateCodePackageAsyncOperation(
        __in MultiCodePackageApplicationHostProxy & owner,
        CodePackageInstanceIdentifier const & codePackageInstanceId,
        CodePackageActivationId const & activationId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        codePackageInstanceId_(codePackageInstanceId),
        activationId_(activationId),
        timeoutHelper_(timeout)
    {
    }

    virtual ~DeactivateCodePackageAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DeactivateCodePackageAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        RequestCodePackageDeactivation(thisSPtr);
    }

private:
    void RequestCodePackageDeactivation(AsyncOperationSPtr const & thisSPtr)
    {
        TimeSpan timeout = timeoutHelper_.GetRemainingTime();
        auto request = CreateDeactivateCodePackageRequestMessage(timeout);
        
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(RequestCodePackageDeactivation): HostId={0}, Request={1}, Timeout={2}",
            owner_.HostId,
            *request,
            timeout);
        
        auto operation = owner_.Hosting.IpcServerObj.BeginRequest(
            move(request),
            owner_.HostId,
            timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnCodePackageDeactivationReplyReceived(operation); },
            thisSPtr);
        
        if (operation->CompletedSynchronously)
        {
            FinishCodePackageDeactivation(operation);
        }
    }

    void OnCodePackageDeactivationReplyReceived(AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            FinishCodePackageDeactivation(operation);
        }
    }

    void FinishCodePackageDeactivation(AsyncOperationSPtr const & operation)
    {
        MessageUPtr reply;
        auto error = owner_.Hosting.IpcServerObj.EndRequest(operation, reply);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "End(RequestCodePackageDeactivation): HostId={0}, CodePackageInstanceId={1} ErrorCode={2}",
                owner_.HostId,
                codePackageInstanceId_,
                error);

            TryComplete(operation->Parent, error);

            // abort the application host proxy, we do not know the status of the proxy
            owner_.Abort();
            return;
        }

        // process the reply
        DeactivateCodePackageReply replyBody;
        if (!reply->GetBody<DeactivateCodePackageReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "GetBody<DeactivateCodePackageReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);
            TryComplete(operation->Parent, error);
            return;
        }

        error = replyBody.Error;
        
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "DeactivateCodePackageReply: HostId={0}, Message={1}, Body={2}",
            owner_.HostId,
            *reply,
            replyBody);

        if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::HostingCodePackageNotHosted))
        {
            TryComplete(operation->Parent, error);
            return;
        }

        RemoveCodePackageFromTable(operation->Parent);
    }

    MessageUPtr CreateDeactivateCodePackageRequestMessage(TimeSpan const timeout)
    {
        DeactivateCodePackageRequest requestBody(codePackageInstanceId_, activationId_, timeout);

        MessageUPtr request = make_unique<Message>(requestBody);
        request->Headers.Add(Transport::ActorHeader(Actor::ApplicationHost));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::DeactivateCodePackageRequest));

        WriteNoise(TraceType, owner_.TraceId, "CreateDeactivateCodePackageRequest: Message={0}, Body={1}", *request, requestBody);

        return move(request);
    }

    void RemoveCodePackageFromTable(AsyncOperationSPtr const & thisSPtr)
    {
        CodePackageInstanceSPtr codePackageInstance;
        auto error = owner_.hostedCodePackageTable_->Remove(codePackageInstanceId_, activationId_, codePackageInstance);
        
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "RemoveCodePackageFromTable: HostId={0}, ErrorCode={1}, CodePackageInstanceId={2}",
            owner_.HostId,
            error,
            codePackageInstanceId_);

        // complete with success as the code package is either removed or was not present
        TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
    }

private:
    MultiCodePackageApplicationHostProxy & owner_;
    CodePackageInstanceIdentifier const codePackageInstanceId_;
    CodePackageActivationId const activationId_;
    TimeoutHelper const timeoutHelper_;
};

MultiCodePackageApplicationHostProxy::MultiCodePackageApplicationHostProxy(
    HostingSubsystemHolder const & hostingHolder,
    wstring const & hostId,
    ApplicationHostIsolationContext const & isolationContext,
    wstring const & runAsId,
    ServicePackageInstanceIdentifier const & servicePackageInstanceId,
    bool removeServiceFabricRuntimeAccess)
    : ApplicationHostProxy(
        hostingHolder,
        ApplicationHostContext(
            hostId, 
            ApplicationHostType::Activated_MultiCodePackage,
            false,
            false),
        isolationContext,
        runAsId,
        servicePackageInstanceId,
        EntryPointType::Enum::DllHost,
        removeServiceFabricRuntimeAccess)
    , hostedCodePackageTable_()
    , terminatedExternally_(false)
    , exitCode_(1)
    , apphostMonitor_()
    , procHandle_()
{
    WriteNoise(
        TraceType,
        TraceId,
        "MultiCodePackageApplicationHostProxy.constructor: HostType={0}, HostId={1}, ServicePackageInstanceId={2}.",
        this->HostType,
        this->HostId,
        this->ServicePackageInstanceId);
}

MultiCodePackageApplicationHostProxy::~MultiCodePackageApplicationHostProxy()
{
}

AsyncOperationSPtr MultiCodePackageApplicationHostProxy::BeginActivateCodePackage(
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

ErrorCode MultiCodePackageApplicationHostProxy::EndActivateCodePackage(
    AsyncOperationSPtr const & operation,
    __out CodePackageActivationId & activationId,
    __out CodePackageRuntimeInformationSPtr & codePackageRuntimeInformation)
{
    if (codePackageRuntimeInformation_)
    {
        codePackageRuntimeInformation = codePackageRuntimeInformation_;
    }

    return ActivateCodePackageAsyncOperation::End(operation, activationId);
}

AsyncOperationSPtr MultiCodePackageApplicationHostProxy::BeginDeactivateCodePackage(
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

ErrorCode MultiCodePackageApplicationHostProxy::EndDeactivateCodePackage(
    AsyncOperationSPtr const & operation)
{
    return DeactivateCodePackageAsyncOperation::End(operation);
}

AsyncOperationSPtr MultiCodePackageApplicationHostProxy::BeginGetContainerInfo(
    wstring const & containerInfoType,
    wstring const & containerInfoArgs,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    UNREFERENCED_PARAMETER(containerInfoType);
    UNREFERENCED_PARAMETER(containerInfoArgs);
    UNREFERENCED_PARAMETER(timeout);

    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
        ErrorCode::Success(),
        callback,
        parent);
}

ErrorCode MultiCodePackageApplicationHostProxy::EndGetContainerInfo(
    AsyncOperationSPtr const & operation,
    __out wstring & containerInfo)
{
    containerInfo = L"Operation not Supported";
    return CompletedAsyncOperation::End(operation);
}

bool MultiCodePackageApplicationHostProxy::HasHostedCodePackages()
{
    return (hostedCodePackageTable_->Size() > 0);
}

void MultiCodePackageApplicationHostProxy::OnApplicationHostRegistered()
{
    auto error = this->hostRegisteredEvent_.Set();
   
    WriteNoise(
        TraceType,
        TraceId,
        "HostRegisteredEvent.Set: HostId={0}, ServicePackageInstanceId={1}, ErrorCode={2}",
        this->HostId,
        this->ServicePackageInstanceId,
        error);
}

void MultiCodePackageApplicationHostProxy::OnApplicationHostTerminated(DWORD exitCode)
{
    this->exitCode_.store(exitCode);
    this->terminatedExternally_.store(true);

    if (this->State.Value != FabricComponentState::Closing &&
        this->State.Value != FabricComponentState::Opening)
    {
        this->Abort();
    }

    WriteInfo(
        TraceType,
        TraceId,
        "OnApplicationHostTerminated called for HostId={0}, ServicePackageInstanceId={1} with ExitCode={2}. Current state is {3}.",
        this->HostId,
        this->ServicePackageInstanceId,
        exitCode,
        this->State);
}

void MultiCodePackageApplicationHostProxy::OnContainerHealthCheckStatusChanged(ContainerHealthStatusInfo const & healthStatusInfo)
{
    UNREFERENCED_PARAMETER(healthStatusInfo);

    // NO-OP
}

void MultiCodePackageApplicationHostProxy::OnCodePackageTerminated(
    CodePackageInstanceIdentifier const & codePackageInstanceId,
    CodePackageActivationId const & activationId)
{
    CodePackageInstanceSPtr codePackageInstance;
    auto error = hostedCodePackageTable_->Remove(codePackageInstanceId, activationId, codePackageInstance);
    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        TraceId,
        "OnCodePackageTerminated: RemoveFromHostedCodePackageTable: HostId={0}, ErrorCode={1}, CodePackageInstanceId={2}, ActivationId={3}",
        HostId,
        error,
        codePackageInstanceId,
        activationId);

    if (error.IsSuccess())
    {
        // notify the code package state machine
        codePackageInstance->OnEntryPointTerminated(activationId, 1, terminatedExternally_.load());
    }
}

AsyncOperationSPtr MultiCodePackageApplicationHostProxy::OnBeginOpen(
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<OpenAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode MultiCodePackageApplicationHostProxy::OnEndOpen(AsyncOperationSPtr const & asyncOperation)
{
    return OpenAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr MultiCodePackageApplicationHostProxy::OnBeginClose(
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode MultiCodePackageApplicationHostProxy::OnEndClose(AsyncOperationSPtr const & asyncOperation)
{
    return CloseAsyncOperation::End(asyncOperation);
}

void MultiCodePackageApplicationHostProxy::OnAbort()
{
    if (apphostMonitor_ != nullptr)
    {
        apphostMonitor_->Cancel();
    }
    if (!terminatedExternally_.load())
    {
        this->exitCode_.store(ProcessActivator::ProcessAbortExitCode);
        Hosting.ApplicationHostManagerObj->FabricActivator->AbortProcess(this->HostId);
    }
    auto removed = hostedCodePackageTable_->Close();
    if (!removed.empty())
    {
        this->NotifyTermination(removed);
    }
}

ErrorCode MultiCodePackageApplicationHostProxy::GetProcessDescription(
    __out ProcessDescriptionUPtr & processDescription)
{
    wstring appDirectory;
    wstring tempDirectory;
    wstring workDirectory;
    wstring logDirectory;
    wstring startInDirectory;

    RunLayoutSpecification runLayout(Hosting.DeploymentFolder);
    wstring applicationIdStr = this->ApplicationId.ToString();
    appDirectory = runLayout.GetApplicationFolder(applicationIdStr);

    tempDirectory = runLayout.GetApplicationTempFolder(applicationIdStr);
    logDirectory = runLayout.GetApplicationLogFolder(applicationIdStr);
    workDirectory = runLayout.GetApplicationWorkFolder(applicationIdStr);

    EnvironmentMap envVars;
    this->GetCurrentProcessEnvironmentVariables(envVars, tempDirectory, this->Hosting.FabricBinFolder);
    auto error = AddHostContextAndRuntimeConnection(envVars);
    if (!error.IsSuccess()) { return error; }

    startInDirectory = workDirectory;

    wstring dllHostPath;
    wstring dllHostArguments;

    error = Hosting.GetDllHostPathAndArguments(dllHostPath, dllHostArguments);
    if (!error.IsSuccess()) { return error; }

    processDescription = make_unique<ProcessDescription>(
        dllHostPath,
        dllHostArguments,
        startInDirectory,
        envVars,
        appDirectory,
        tempDirectory,
        workDirectory,
        logDirectory,
        HostingConfig::GetConfig().EnableActivateNoWindow,
        false,
        false,
        false);

    return ErrorCode(ErrorCodeValue::Success);
}

void MultiCodePackageApplicationHostProxy::NotifyTermination(
    vector<pair<CodePackageActivationId, CodePackageInstanceSPtr>> const & hostedCodePackages)
{
    if (this->Hosting.State.Value > FabricComponentState::Opened)
    {
        // no need to notify if the Subsystem is closing down
        return;
    }

    DWORD exitCode = (DWORD)exitCode_.load();
    bool terminatedExternally = terminatedExternally_.load();
    for (auto iter = hostedCodePackages.cbegin(); iter != hostedCodePackages.cend(); ++iter)
    {
        iter->second->OnEntryPointTerminated(iter->first, exitCode, !terminatedExternally);
    }
}
