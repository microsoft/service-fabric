// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Hosting2;
using namespace Reliability::ReplicationComponent;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace ServiceModel;
using namespace ktl;
using namespace Management::BackupRestoreService;

StringLiteral const TraceType("ApplicationHost");

// ********************************************************************************************************************
// ApplicationHost::KtlSystemWrapper Implementation
//
class ApplicationHost::KtlSystemWrapper
{
    DENY_COPY(KtlSystemWrapper)

public:
    
    //
    // Ktl system is created per app host only for fabrictest - to help debug leaks
    // In production, we will use the common default ktl system that is never deactivated (hence no leak detection)
    //
    KtlSystemWrapper(
        __in ApplicationHost & owner,
        __in KtlSystem * ktlSystem,
        __in bool useGlobalKtlSystem)
        : owner_(owner)
        , isKtlSystemShared_(false)
        , useGlobalKtlSystem_(useGlobalKtlSystem)
        , ktlSystem_(ktlSystem)
    {
        isKtlSystemShared_ = (ktlSystem_ != nullptr);

        owner_.WriteInfo(
            TraceType,
            owner_.TraceId,
            "KtlSystemWrapper: IsktlSystemNull: {0} isKtlSystemShared: {1} useGlobalKtlSystem: {2}",
            (ktlSystem_ == nullptr),
            isKtlSystemShared_,
            useGlobalKtlSystem_);
    }

    virtual ~KtlSystemWrapper()
    {
        if (useGlobalKtlSystem_ ||
            isKtlSystemShared_)
        {
            return;
        }

        KtlSystemBase * casted = reinterpret_cast<KtlSystemBase *>(ktlSystem_);
        delete casted;
    }

    __declspec(property(get = get_KtlSystem)) KtlSystem & KtlSystemObject;
    KtlSystem & get_KtlSystem() const
    {
        if (useGlobalKtlSystem_)
        {
            return Common::GetSFDefaultKtlSystem();
        }

        ASSERT_IF(
            ktlSystem_ == nullptr,
            "{0}: Ktl system is null",
            owner_.TraceId);

        return *ktlSystem_;
    }

    void Activate(TimeSpan const & timeout)
    {
        if (useGlobalKtlSystem_ ||
            isKtlSystemShared_)
        {
            owner_.WriteInfo(
                TraceType,
                owner_.TraceId,
                "KtlSystemWrapper: Skipping KTL System Activate");

            return;
        }

        KGuid ktlSystemGuid;
        ktlSystemGuid.CreateNew();

        ktlSystem_ = new KtlSystemBase(ktlSystemGuid);
        ktlSystem_->Activate(static_cast<ULONG>(timeout.TotalMilliseconds()));

        NTSTATUS status = ktlSystem_->Status();

        owner_.WriteInfo(
            TraceType,
            owner_.TraceId,
            "KtlSystemWrapper: KTL System Activated with NTSTATUS code {0:x}",
            status);

        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}: Cannot active KTL System due to error {1:x}",
            owner_.TraceId,
            status);

        // NOTE: This is needed for high performance of v2 replicator stack as more threads in the threadpool
        // are used to perform completions of awaitables rather than using current thread's local work items queue
        ktlSystem_->SetDefaultSystemThreadPoolUsage(FALSE);

        return;
    }

    void Deactivate(Common::TimeSpan const & timeout)
    {
        if (useGlobalKtlSystem_ ||
            isKtlSystemShared_ ||
           (ktlSystem_ == nullptr))
        {
            owner_.WriteInfo(
                TraceType,
                owner_.TraceId,
                "Skipping KTL System Deactivate");

            return;
        }

        ktlSystem_->Deactivate(static_cast<ULONG>(timeout.TotalMilliseconds()));

        NTSTATUS status = ktlSystem_->Status();

        owner_.WriteInfo(
            TraceType,
            owner_.TraceId,
            "KTL System Deactivate completed with NTSTATUS code {0:x} for timeout {1}",
            status,
            timeout);

        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}: Cannot deactive KTL System due to error {1:x}",
            owner_.TraceId,
            status);

        return;
    }

private:

    ApplicationHost & owner_;
    bool const useGlobalKtlSystem_;
    bool isKtlSystemShared_;
    KtlSystem * ktlSystem_;
};

// ********************************************************************************************************************
// ApplicationHost::CodePackageTable Implementation
//
class ApplicationHost::CodePackageTable : public RootedObject
{
public:
    class Entry
    {
        DENY_COPY(Entry)
    public:

        Entry(
            CodePackageActivationId const & activationId, 
            CodePackageActivationContextSPtr const & activationContext)
            : activationId_(activationId), 
            activationContext_(activationContext),
            isValid_(false)
        {
        }

        __declspec (property(get=get_ActivationId)) CodePackageActivationId const & ActivationId;
        CodePackageActivationId const & get_ActivationId() const { return activationId_; }

        __declspec (property(get=get_ActivationContext)) CodePackageActivationContextSPtr const & ActivationContext;
        CodePackageActivationContextSPtr const & get_ActivationContext() const { return activationContext_; }

        __declspec (property(get=get_IsValid)) bool const & IsValid;
        bool const & get_IsValid() const { return isValid_; }

    private:
        void Validate()
        {
            isValid_ = true;
        }

    private:
        CodePackageActivationId const activationId_;
        CodePackageActivationContextSPtr const activationContext_;
        bool isValid_;
        friend class ApplicationHost::CodePackageTable;
    };
    typedef shared_ptr<Entry> EntrySPtr;

    typedef std::function<bool(EntrySPtr const &)> Visitor;

    DENY_COPY(CodePackageTable)

public:
    CodePackageTable(ComponentRoot const & root)
        : RootedObject(root),
        lock_(),
        map_(),
        isClosed_(false)
    {
    }

    ErrorCode Add(
        CodePackageActivationId const & activationId, 
        CodePackageActivationContextSPtr const & activationContext,
        bool validate)
    {
        {
            AcquireWriteLock lock(lock_);
            if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

            auto iter = map_.find(activationContext->CodePackageInstanceId);
            if (iter == map_.end())
            {
                EntrySPtr entry = make_shared<Entry>(activationId, activationContext);
                if(validate)
                {
                    entry->Validate();
                }

                map_.insert(
                    make_pair(
                    activationContext->CodePackageInstanceId,
                    entry));

                return ErrorCode(ErrorCodeValue::Success);
            }
            else
            {
                return ErrorCode(ErrorCodeValue::HostingCodePackageAlreadyHosted);
            }
        }
    }

    ErrorCode Find(
        CodePackageInstanceIdentifier const & codePackageInstanceId, 
        __out EntrySPtr & entry)
    {
        {
            AcquireReadLock lock(lock_);
            if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

            auto iter = map_.find(codePackageInstanceId);
            if (iter == map_.end())
            {
                return ErrorCode(ErrorCodeValue::HostingCodePackageNotHosted);
            }
            else
            {
                entry = iter->second;
                return ErrorCode(ErrorCodeValue::Success);
            }
        }
    }

    ErrorCode Validate(
        CodePackageInstanceIdentifier const & codePackageInstanceId,
        CodePackageActivationId const & activationId)
    {
        {
            AcquireWriteLock lock(lock_);
            if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

            auto iter = map_.find(codePackageInstanceId);
            if (iter == map_.end() || activationId != iter->second->ActivationId)
            {
                return ErrorCode(ErrorCodeValue::HostingCodePackageNotHosted);
            }
            else
            {
                iter->second->Validate();
                return ErrorCode(ErrorCodeValue::Success);
            }
        }
    }

    ErrorCode Remove(
        CodePackageInstanceIdentifier const & codePackageInstanceId, 
        CodePackageActivationId const & activationId,
        __out EntrySPtr & entry)
    {
        {
            AcquireWriteLock lock(lock_);
            if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

            auto iter = map_.find(codePackageInstanceId);
            if (iter == map_.end() || activationId != iter->second->activationId_)
            {
                return ErrorCode(ErrorCodeValue::HostingCodePackageNotHosted);
            }
            else
            {
                entry = iter->second;
                map_.erase(iter);
                return ErrorCode(ErrorCodeValue::Success);
            }
        }
    }

    ErrorCode VisitUnderLock(Visitor const & visitor)
    {
        {
            AcquireReadLock readlock(lock_);
            if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

            for(auto iter = map_.begin(); iter != map_.end(); ++iter)
            {
                if (!visitor(iter->second))
                {
                    break;
                }
            }
        }

        return ErrorCode(ErrorCodeValue::Success);
    }

    void Close()
    {
        {
            AcquireWriteLock lock(lock_);
            if (!isClosed_)
            {
                isClosed_ = true;
            }
            map_.clear();
        }
    }

private:
    RwLock lock_;
    map<CodePackageInstanceIdentifier, EntrySPtr> map_;
    bool isClosed_;
};

ErrorCode ApplicationHost::AddCodePackage(
    CodePackageActivationId const & activationId, 
    CodePackageActivationContextSPtr const & activationContext,
    bool validate)
{
    return codePackageTable_->Add(
        activationId,
        activationContext,
        validate);
}

ErrorCode ApplicationHost::FindCodePackage(
    CodePackageInstanceIdentifier const & codePackageInstanceId,
    __out CodePackageActivationId & activationId, 
    __out CodePackageActivationContextSPtr & activationContext,
    __out bool & isValid)
{
    CodePackageTable::EntrySPtr entry;
    auto error = codePackageTable_->Find(
        codePackageInstanceId,
        entry);
    if(error.IsSuccess())
    {
        activationId = entry->ActivationId;
        activationContext = entry->ActivationContext;
        isValid = entry->IsValid;
    }

    return error;
}

ErrorCode ApplicationHost::ValidateCodePackage(
    CodePackageInstanceIdentifier const & codePackageInstanceId,
    CodePackageActivationId const & activationId)
{
    return codePackageTable_->Validate(
        codePackageInstanceId,
        activationId);
}

ErrorCode ApplicationHost::RemoveCodePackage(
    CodePackageInstanceIdentifier const & codePackageInstanceId,
    CodePackageActivationId const & activationId,
    __out bool & isValid)
{
    CodePackageTable::EntrySPtr entry;
    auto error = codePackageTable_->Remove(
        codePackageInstanceId,
        activationId,
        entry);
    if(error.IsSuccess())
    {
        isValid = entry->IsValid;
    }

    return error;
}

// ********************************************************************************************************************
// ApplicationHost::RuntimeTable Implementation
//

class ApplicationHost::RuntimeTable :
    public RootedObject,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(RuntimeTable)

public:

    class Entry
    {
        DENY_COPY(Entry)
    public:

        Entry(FabricRuntimeImplSPtr const & runtimeImpl)
            : runtimeImplSPtr_(runtimeImpl),
            isValid_(false)
        {
        }

        __declspec (property(get=get_RuntimeImpl)) FabricRuntimeImplSPtr const& RuntimeImpl;
        FabricRuntimeImplSPtr const & get_RuntimeImpl() const { return runtimeImplSPtr_; }

        __declspec (property(get=get_IsValid)) bool const & IsValid;
        bool const & get_IsValid() const { return isValid_; }

    private:
        void Validate() { isValid_ = true; }

    private:
        FabricRuntimeImplSPtr runtimeImplSPtr_;
        bool isValid_;
        friend class ApplicationHost::RuntimeTable;
    };

    typedef shared_ptr<Entry> EntrySPtr;

    typedef std::function<bool(EntrySPtr const &)> Visitor;

    RuntimeTable(ComponentRoot const & root)
        : RootedObject(root),
        map_(),
        lock_(),
        isClosed_(false)
    {
    }

    ErrorCode AddPending(FabricRuntimeImplSPtr const & runtime)
    {
        {
            AcquireWriteLock writeLock(lock_);
            if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

            auto iter = map_.find(runtime->RuntimeId);
            if (iter != map_.end())
            {
                return ErrorCode(ErrorCodeValue::HostingFabricRuntimeAlreadyRegistered);
            }

            map_.insert(make_pair(runtime->RuntimeId, make_shared<Entry>(runtime)));
        }

        return ErrorCode(ErrorCodeValue::Success);
    }

    ErrorCode Validate(FabricRuntimeImplSPtr const & runtime)
    {
        {
            AcquireWriteLock writeLock(lock_);
            if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

            auto iter = map_.find(runtime->GetRuntimeContext().RuntimeId);
            if (iter != map_.end())
            {
                iter->second->Validate();
            }
            else
            {
                return ErrorCode(ErrorCodeValue::HostingFabricRuntimeNotRegistered);
            }
        }

        return ErrorCode(ErrorCodeValue::Success);
    }

    ErrorCode Remove(wstring const & runtimeId, __out FabricRuntimeImplSPtr & runtime)
    {
        {
            AcquireWriteLock writeLock(lock_);
            if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

            {
                auto iter = map_.find(runtimeId);
                if (iter != map_.end())
                {
                    runtime = iter->second->RuntimeImpl;
                    map_.erase(iter); 
                }
                else
                {
                    return ErrorCode(ErrorCodeValue::HostingFabricRuntimeNotRegistered);
                }
            }
        }

        return ErrorCode(ErrorCodeValue::Success);
    }

    ErrorCode Find(wstring const & runtimeId, __out FabricRuntimeImplSPtr & runtime)
    {
        {
            AcquireReadLock readLock(lock_);
            if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

            auto iter = map_.find(runtimeId);
            if (iter != map_.end())
            {
                runtime = iter->second->RuntimeImpl;
            }
            else
            {
                return ErrorCode(ErrorCodeValue::HostingFabricRuntimeNotRegistered);
            }
        }

        return ErrorCode(ErrorCodeValue::Success);
    }


    ErrorCode VisitUnderLock(Visitor const & visitor)
    {
        {
            AcquireReadLock readlock(lock_);
            if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

            for(auto iter = map_.begin(); iter != map_.end(); ++iter)
            {
                if (!visitor(iter->second))
                {
                    break;
                }
            }
        }

        return ErrorCode(ErrorCodeValue::Success);
    }

    vector<FabricRuntimeImplSPtr> Close()
    {
        vector<FabricRuntimeImplSPtr> removed;
        {
            AcquireWriteLock writeLock(lock_);
            if (!isClosed_)
            {
                for(auto iter = map_.begin(); iter != map_.end(); ++iter)
                {
                    if (iter->second->RuntimeImpl)
                    {
                        removed.push_back(iter->second->RuntimeImpl);
                    }
                }
                map_.clear();
                isClosed_ = true;
            }
        }

        return removed;
    }

private:
    map<wstring, EntrySPtr> map_;
    RwLock lock_;
    bool isClosed_;
};

ErrorCode ApplicationHost::AddFabricRuntime(FabricRuntimeImplSPtr const & fabricRuntime)
{
    return runtimeTable_->AddPending(fabricRuntime);
}

// ********************************************************************************************************************
// ApplicationHost::OpenAsyncOperation Implementation
//
// Start
// Step 1: Open IPC Client
// Step 1.x: for non activated host ensure sychronize access to fabric process user
// Step 2: Start Application Host Registration
// Step 3: Initialize Node Host Process Monitoring
// Step 4: Initialize Lease Monitoring
// Step 5: Finish Application Host Registration
// Step 6: Open Runtime Table
// Step 7: Open Replicator Factories
// Step 8: Open RAP
// Step 9: Initialize KTL System
// Step 10: Open LogManager
// Step 11: Initialize Transactional replicator factory using the ktl system above
// Step 12: Open BAP (conditional)
// Complete
class ApplicationHost::OpenAsyncOperation : 
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(OpenAsyncOperation)

public:
    OpenAsyncOperation(
        ApplicationHost & owner,
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
            owner_.TraceId,
            "Opening ApplicationHost: Id={0}, Type={1}, Timeout={2}",
            owner_.Id,
            owner_.Type,
            timeoutHelper_.GetRemainingTime());

        // Step 1: Open IPC Client
        OpenIPC(thisSPtr);
    }

private:
    void OpenIPC(AsyncOperationSPtr const & thisSPtr)
    {   
        FabricNodeConfigSPtr fabricNodeConfig = std::make_shared<FabricNodeConfig>(); 

        SecuritySettings ipcClientSecuritySettings;
        ErrorCode error;

        if (owner_.HostContext.IsContainerHost || owner_.ipcUseSsl_.load())
        {
            error = SecuritySettings::CreateSslClient(
                owner_.certContext_,
                owner_.serverThumbprint_,
                ipcClientSecuritySettings);
        }
        else
        {
            error = SecuritySettings::CreateNegotiateClient(
                SecurityConfig::GetConfig().ClusterSpn,
                ipcClientSecuritySettings);
        }

        owner_.Client.SecuritySettings = ipcClientSecuritySettings;

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "SecuritySettings.CreatClient: ErrorCode={0}",
            error);

        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);
            return;
        }

        owner_.RegisterIpcMessageHandler();

        error = owner_.Client.Open();
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "IPC Client Open: ErrorCode={0}",
            error);
        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);
            return;
        }

        error = owner_.IpcHealthReportingClientObj.Open();

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "IPC Health Reporting Client Open: ErrorCode={0}",
            error);

        error = owner_.ThrottledIpcHealthReportingClientObj.Open();

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "Throttled IPC Health Reporting Client Open: ErrorCode={0}",
            error);

        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);
            return;
        }

        if (owner_.Type == ApplicationHostType::NonActivated)
        {
            // Step 1.x: ensure synchronize access to fabric process user
            ProvideSynchronizeAccessToFabricProcess(thisSPtr);
        }
        else
        {
            // Step 2: Start Application Host Registration
            StartApplicationHostRegistration(thisSPtr);
        }
    }

    void ProvideSynchronizeAccessToFabricProcess(AsyncOperationSPtr const & thisSPtr)
    {
        TimeSpan timeout = HostingConfig::GetConfig().RequestTimeout;
        if (timeout > timeoutHelper_.GetRemainingTime())
        {
            timeout = timeoutHelper_.GetRemainingTime();
        }

        MessageUPtr request = CreateGetFabricProcessSidRequestMessage();
        WriteNoise(
            TraceType, 
            owner_.TraceId, 
            "Begin(GetFabricProcessSidRequest): Timeout={0}, Request={1}", 
            timeout,
            *request);
        auto operation = owner_.Client.BeginRequest(
            move(request),
            timeout,
            [this](AsyncOperationSPtr const & operation){ OnGetFabricProcessSidRequestCompleted(operation); },
            thisSPtr);
        if (operation->CompletedSynchronously)
        {
            FinishGetFabricProcessSidRequestCompleted(operation);
        }
    }

    void OnGetFabricProcessSidRequestCompleted(AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            FinishGetFabricProcessSidRequestCompleted(operation);
        }
    }

    void FinishGetFabricProcessSidRequestCompleted(AsyncOperationSPtr const & operation)
    {
        MessageUPtr reply;
        auto error = owner_.Client.EndRequest(operation, reply);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType, 
                owner_.TraceId, 
                "End(GetFabricProcessSidRequest): ErrorCode={0}", 
                error);
            TryComplete(operation->Parent, error);
            return;
        }

        GetFabricProcessSidReply replyBody;
        if (!reply->GetBody<GetFabricProcessSidReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "GetBody<GetFabricProcessSidReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);
            TryComplete(operation->Parent, error);
            return;
        }
        
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "GetFabricProcessSidReply: Message={0}, Body={1}",
            *reply,
            replyBody);

        if (replyBody.FabricProcessId != ::GetCurrentProcessId())
        {
            // provide synchronize access to the sid
            SidSPtr fabricProcessUserSid;
            error = BufferedSid::CreateSPtrFromStringSid(replyBody.FabricProcessSid, fabricProcessUserSid);
            WriteTrace(
                error.ToLogLevel(),
                TraceType,
                owner_.TraceId,
                "CreateSPtrFromStringSid: ErrorCode={0}, StringSid={1}",
                error,
                replyBody.FabricProcessSid);
            if (!error.IsSuccess())
            {
                TryComplete(operation->Parent, error);
                return;
            }

            error = SecurityUtility::UpdateProcessAcl(ProcessHandle(::GetCurrentProcess(), false), fabricProcessUserSid, SYNCHRONIZE);
            WriteTrace(
                error.ToLogLevel(),
                TraceType,
                owner_.TraceId,
                "UpdateProcessAcl: ErrorCode={0}",
                error);
            if (!error.IsSuccess())
            {
                TryComplete(operation->Parent, error);
                return;
            }
        }
        else
        {
            WriteNoise(
                TraceType,
                owner_.TraceId,
                "Skipping setting up ACL for process synchronization as ApplicationHost {0} is part of same process {1} as Fabric.",
                owner_.Id,
                replyBody.FabricProcessId);
        }

        // Step 2: Start Application Host Registration
        StartApplicationHostRegistration(operation->Parent);
    }

    void StartApplicationHostRegistration(AsyncOperationSPtr const & thisSPtr)
    {
        TimeSpan timeout = HostingConfig::GetConfig().RequestTimeout;
        if (timeout > timeoutHelper_.GetRemainingTime())
        {
            timeout = timeoutHelper_.GetRemainingTime();
        }

        MessageUPtr request = CreateStartRegisterApplicationHostRequestMessage();
        WriteNoise(
            TraceType, 
            owner_.TraceId, 
            "Begin(StartRegisterApplicationHostRequest): Timeout={0}, Request={1}", 
            timeout,
            *request);

        auto requestTime = Stopwatch::Now();
        auto startHostRegistrationOperation = owner_.Client.BeginRequest(
            move(request),
            timeout,
            [this, requestTime](AsyncOperationSPtr const & operation){ OnStartRegisterApplicationHostCompleted(operation, requestTime); },
            thisSPtr);
        if (startHostRegistrationOperation->CompletedSynchronously)
        {
            FinishStartRegisterApplicationHost(startHostRegistrationOperation, requestTime);
        }
    }

    void OnStartRegisterApplicationHostCompleted(AsyncOperationSPtr const & startHostRegistrationOperation, StopwatchTime requestTime)
    {
        if (!startHostRegistrationOperation->CompletedSynchronously)
        {
            FinishStartRegisterApplicationHost(startHostRegistrationOperation, requestTime);
        }
    }

    void FinishStartRegisterApplicationHost(AsyncOperationSPtr const & hostRegistrationOperation, StopwatchTime requestTime)
    {
        MessageUPtr reply;
        auto error = owner_.Client.EndRequest(hostRegistrationOperation, reply);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType, 
                owner_.TraceId, 
                "End(StartRegisterApplicationHostRequest): ErrorCode={0}", 
                error);
            TryComplete(hostRegistrationOperation->Parent, error);
            return;
        }

        // process the reply
        StartRegisterApplicationHostReply replyBody;
        if (!reply->GetBody<StartRegisterApplicationHostReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "GetBody<StartRegisterApplicationHostReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);
            TryComplete(hostRegistrationOperation->Parent, error);
            return;
        }

        error = replyBody.Error;
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "StartRegisterApplicationHostReply: Message={0}, Body={1}",
            *reply,
            replyBody);
        if (!error.IsSuccess())
        {
            TryComplete(hostRegistrationOperation->Parent, error);
            return;
        }        

        // mark the attachment of this application host to fabric node        
        if (!owner_.TryPutReservedTraceId(L"AppHost-" + owner_.Id + L"@Node-" + replyBody.NodeId))
        {
            WriteWarning(
                TraceType, 
                owner_.TraceId, 
                "Failed to update reserved TraceId with NodeId: {0}",
                replyBody.NodeId);
        }

        // Step 3: Initialize Node Host Process Monitoring
        error = owner_.InitializeNodeHostMonitoring(replyBody.NodeHostProcessId);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "InitializeNodeHostProcessMonitoring: ErrorCode={0}, NodeHostProcessId={1}",
            error,
            replyBody.NodeHostProcessId);
        if (!error.IsSuccess())
        {
            TryComplete(hostRegistrationOperation->Parent, error);
            return;
        }

        // Step 4: Initialize Lease Monitoring
        error = owner_.InitializeLeaseMonitoring(replyBody.NodeLeaseHandle, replyBody.InitialLeaseTTL().Ticks, replyBody.NodeLeaseDuration);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "InitializeLeaseMonitoring: ErrorCode={0}, NodeLeaseHandle={1}, InitialLeaseTTL={2}ms, requestTime={3}, initialLeaseExpiration={4}, NodeLeaseDuration={5}",
            error,
            (uint64)(replyBody.NodeLeaseHandle),
            replyBody.InitialLeaseTTL(),
            requestTime,
            replyBody.InitialLeaseTTL(),
            replyBody.NodeLeaseDuration);
        if (!error.IsSuccess())
        {
            TryComplete(hostRegistrationOperation->Parent, error);
            return;
        }

        owner_.applicationSharedLogSettings_ = replyBody.ApplicationSharedLogSettings;
        owner_.systemServicesSharedLogSettings_ = replyBody.SystemServicesSharedLogSettings;

        {
            AcquireWriteLock lock(owner_.nodeContextLock_);
            owner_.nodeContext_ = make_shared<FabricNodeContext>(
                replyBody.NodeName,
                replyBody.NodeId,
                replyBody.NodeInstanceId,
                replyBody.NodeType,
                replyBody.ClientConnectionAddress,
                owner_.runtimeServiceAddress_,
                replyBody.DeploymentDirectory,
                replyBody.IPAddressOrFQDN,
                replyBody.NodeWorkFolder,
                replyBody.LogicalApplicationDirectories,
                replyBody.LogicalNodeDirectories);
        }

        // Step 5: Finish Application Host Registration
        FinishApplicationHostRegistration(hostRegistrationOperation->Parent);

        // Starts monitoring UnreliableTransportSettings.ini for configuration changes

        // TODO: Bug#9728016 - Disable the bind until windows supports mounting file onto container
        if (!owner_.hostContext_.IsContainerHost)
        {
            UnreliableTransportConfig::GetConfig().StartMonitoring(owner_.nodeContext_->NodeWorkFolder, owner_.nodeContext_->NodeId);
        }
    }

    void FinishApplicationHostRegistration(AsyncOperationSPtr const & thisSPtr)
    {
        TimeSpan timeout = HostingConfig::GetConfig().RequestTimeout;
        if (timeout > timeoutHelper_.GetRemainingTime())
        {
            timeout = timeoutHelper_.GetRemainingTime();
        }

        MessageUPtr request = CreateFinishRegisterApplicationHostRequestMessage();
        WriteNoise(
            TraceType, 
            owner_.TraceId, 
            "Begin(FinishRegisterApplicationHostRequest): Timeout={0}, Request={1}", 
            timeout,
            *request);
        auto finishHostRegistrationOperation = owner_.Client.BeginRequest(
            move(request),
            timeout,
            [this](AsyncOperationSPtr const & operation){ OnFinishRegisterApplicationHostCompleted(operation); },
            thisSPtr);
        if (finishHostRegistrationOperation->CompletedSynchronously)
        {
            FinishFinishRegisterApplicationHost(finishHostRegistrationOperation);
        }
    }

    void OnFinishRegisterApplicationHostCompleted(AsyncOperationSPtr const & finishHostRegistrationOperation)
    {
        if (!finishHostRegistrationOperation->CompletedSynchronously)
        {
            FinishFinishRegisterApplicationHost(finishHostRegistrationOperation);
        }
    }

    void FinishFinishRegisterApplicationHost(AsyncOperationSPtr const & finishHostRegistrationOperation)
    {
        MessageUPtr reply;
        auto error = owner_.Client.EndRequest(finishHostRegistrationOperation, reply);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType, 
                owner_.TraceId, 
                "End(FinishRegisterApplicationHostRequest): ErrorCode={0}", 
                error);
            TryComplete(finishHostRegistrationOperation->Parent, error);
            return;
        }

        // process the reply
        FinishRegisterApplicationHostReply replyBody;
        if (!reply->GetBody<FinishRegisterApplicationHostReply>(replyBody))
        {
           error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "GetBody<FinishRegisterApplicationHostReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);
            TryComplete(finishHostRegistrationOperation->Parent, error);
            return;
        }

        error = replyBody.Error;
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "FinishRegisterApplicationHostReply: Message={0}, Body={1}",
            *reply,
            replyBody);
        if (!error.IsSuccess())
        {
            TryComplete(finishHostRegistrationOperation->Parent, error);
            return;
        }

        // Step 6: Open Runtime Table
        OpenRuntimeTable(finishHostRegistrationOperation->Parent);
    }

    void OpenRuntimeTable(AsyncOperationSPtr const & thisSPtr)
    {
        // nothing to open in RuntimeTable

        // Step 7: Open Replicator Factories
        OpenReplicatorFactory(thisSPtr);
    }

    void OpenReplicatorFactory(AsyncOperationSPtr const & thisSPtr)
    {
        wstring nodeId;
        {
            AcquireReadLock lock(owner_.nodeContextLock_);
            ASSERT_IFNOT(owner_.nodeContext_, "NodeContext should be available");
            nodeId = owner_.nodeContext_->NodeId;
        }

        auto error = owner_.ComReplicatorFactoryObj.Open(nodeId);

        WriteTrace(
            error.ToLogLevel(),
            TraceType, 
            owner_.TraceId, 
            "OpenReplicatorFactory: NodeId={0}, ErrorCode={1}", 
            nodeId, 
            error);
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        // Step 8: Open RAP
        OpenRAP(thisSPtr);
    }

    void OpenRAP(AsyncOperationSPtr const & thisSPtr)
    {
        TimeSpan timeout = HostingConfig::GetConfig().RequestTimeout;

        if (timeout > timeoutHelper_.GetRemainingTime())
        {
            timeout = timeoutHelper_.GetRemainingTime();
        }

        owner_.WriteNoise(
            TraceType, 
            owner_.TraceId, 
            "Begin(OpenRAP): Timeout={0}", 
            timeout);

        auto openRAPOperation =         
            owner_.ReconfigurationAgentProxyObj.BeginOpen(
            timeout,
            [this](AsyncOperationSPtr const & operation){ OnOpenRAPCompleted(operation); },
            thisSPtr);

        if (openRAPOperation->CompletedSynchronously)
        {
            FinishOpenRAP(openRAPOperation);
        }
    }

    void OnOpenRAPCompleted(AsyncOperationSPtr const & openRAPOperation)
    {
        if (!openRAPOperation->CompletedSynchronously)
        {
            FinishOpenRAP(openRAPOperation);
        }
    }

    void FinishOpenRAP(AsyncOperationSPtr const & openRAPOperation)
    {
        auto error = owner_.ReconfigurationAgentProxyObj.EndOpen(openRAPOperation);

        if (!error.IsSuccess())
        {
            owner_.WriteWarning(
                TraceType, 
                owner_.TraceId, 
                "End(OpenRAPRequest): ErrorCode={0}", 
                error);

            TryComplete(openRAPOperation->Parent, error);

            return;
        }

        owner_.WriteNoise(
            TraceType,
            owner_.TraceId,
            "RAP Opened.");

        InitializeAndActivateKtlSystem(openRAPOperation->Parent);
    }

    void InitializeAndActivateKtlSystem(AsyncOperationSPtr const & thisSPtr)
    {
        owner_.ktlSystemWrapper_->Activate(timeoutHelper_.GetRemainingTime());
        OpenLogManager(thisSPtr);
    }

    void OpenLogManager(AsyncOperationSPtr const & thisSPtr)
    {
        NTSTATUS status;

        owner_.WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(OpenLogManager)");
        
        status = Data::Log::LogManager::Create(owner_.ktlSystemWrapper_->KtlSystemObject.NonPagedAllocator(), owner_.logManager_);
        if (!NT_SUCCESS(status))
        {
            // todo: use ntstatus -> hresult converter
            ErrorCode error = ErrorCode(ErrorCodeValue::OperationFailed); // ErrorCode::FromNtStatus(status);

            Assert::TestAssert(
                "Data::Log::LogManager Create failed with {0}",
                status);

            WriteTrace(
                error.ToLogLevel(),
                TraceType,
                owner_.TraceId,
                "Data::Log::LogManager Create: ErrorCode={0} NTSTATUS={1}",
                error,
                status);

            TryComplete(thisSPtr, error);
            return;
        }

        auto openLogManagerOperation =
            owner_.logManager_->BeginOpen(
                [this](AsyncOperationSPtr const & operation) { OnOpenLogManagerCompleted(operation); },
                thisSPtr,
                CancellationToken::None,
                owner_.useSystemServiceSharedLogSettings_ ? owner_.systemServicesSharedLogSettings_ : owner_.applicationSharedLogSettings_);

        if (openLogManagerOperation->CompletedSynchronously)
        {
            FinishOpenLogManager(openLogManagerOperation);
        }
    }

    void OnOpenLogManagerCompleted(AsyncOperationSPtr const & openLogManagerOperation)
    {
        if (!openLogManagerOperation->CompletedSynchronously)
        {
            FinishOpenLogManager(openLogManagerOperation);
        }
    }

    void FinishOpenLogManager(AsyncOperationSPtr const & openLogManagerOperation)
    {
        auto error = owner_.logManager_->EndOpen(openLogManagerOperation);

        if (!error.IsSuccess())
        {
            owner_.WriteWarning(
                TraceType,
                owner_.TraceId,
                "End(OpenLogManager): ErrorCode={0}",
                error);

            owner_.logManager_ = nullptr;
            TryComplete(openLogManagerOperation->Parent, error);

            return;
        }

        auto selfRoot = openLogManagerOperation->Parent;

        Threadpool::Post([this, selfRoot] 
        {
            owner_.WriteNoise(
                TraceType,
                owner_.TraceId,
                "LogManager Opened.");

            // Open the Transactional Replicator Factory
            this->OpenTransactionalReplicatorFactory(selfRoot);
        });
    }

    void OpenTransactionalReplicatorFactory(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.ComTransactionalReplicatorFactoryObj.Open(owner_.ktlSystemWrapper_->KtlSystemObject, *owner_.logManager_);

        WriteTrace(
            error.ToLogLevel(),
            TraceType, 
            owner_.TraceId, 
            "OpenTransactionalReplicatorFactory: ErrorCode={0}", 
            error);
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }
        
#if !defined (PLATFORM_UNIX)
        if (BackupRestoreServiceConfig::IsBackupRestoreServiceConfigured())
        {
            // Step 12: Open BAP
            OpenBAP(thisSPtr);
        }
        else
#endif
        {
            // Application Host Open is Completed.
            TryComplete(thisSPtr, error);
        }
    }

#if !defined (PLATFORM_UNIX)
    void OpenBAP(AsyncOperationSPtr const & thisSPtr)
    {
        auto timeout = HostingConfig::GetConfig().RequestTimeout;

        if (timeout > timeoutHelper_.GetRemainingTime())
        {
            timeout = timeoutHelper_.GetRemainingTime();
        }
        
        owner_.WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(OpenBAP): Timeout={0}",
            timeout);

        auto openBAPOperation =
            owner_.BackupRestoreAgentProxyObj.BeginOpen(
                timeout,
                [this](AsyncOperationSPtr const & operation) { OnOpenBAPCompleted(operation); },
                thisSPtr);

        if (openBAPOperation->CompletedSynchronously)
        {
            FinishOpenBAP(openBAPOperation);
        }
    }

    void OnOpenBAPCompleted(AsyncOperationSPtr const & openBAPOperation)
    {
        if (!openBAPOperation->CompletedSynchronously)
        {
            FinishOpenBAP(openBAPOperation);
        }
    }

    void FinishOpenBAP(AsyncOperationSPtr const & openBAPOperation)
    {
        auto error = owner_.BackupRestoreAgentProxyObj.EndOpen(openBAPOperation);

        if (!error.IsSuccess())
        {
            owner_.WriteWarning(
                TraceType,
                owner_.TraceId,
                "End(OpenBAPRequest): ErrorCode={0}",
                error);

            TryComplete(openBAPOperation->Parent, error);

            return;
        }

        owner_.WriteNoise(
            TraceType,
            owner_.TraceId,
            "BAP Opened.");

        // Application Host Open is Completed.
        TryComplete(openBAPOperation->Parent, error);
    }
#endif

    MessageUPtr CreateGetFabricProcessSidRequestMessage()
    {
        MessageUPtr request = make_unique<Message>();
        request->Headers.Add(Transport::ActorHeader(Actor::ApplicationHostManager));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::GetFabricProcessSidRequest));

        WriteNoise(TraceType, owner_.TraceId, "CreateGetFabricProcessSidRequestMessage: Message={0}", *request);

        return move(request);
    }

    MessageUPtr CreateStartRegisterApplicationHostRequestMessage()
    {
        StartRegisterApplicationHostRequest requestBody(
            owner_.Id,
            owner_.Type,
            ::GetCurrentProcessId(),
            owner_.hostContext_.IsContainerHost,
            owner_.hostContext_.IsCodePackageActivatorHost,
            timeoutHelper_.GetRemainingTime());

        MessageUPtr request = make_unique<Message>(requestBody);
        request->Headers.Add(Transport::ActorHeader(Actor::ApplicationHostManager));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::StartRegisterApplicationHostRequest));

        WriteNoise(TraceType, owner_.TraceId, "CreateStartRegisterApplicationHostRequest: Message={0}, Body={1}", *request, requestBody);

        return move(request);
    }

    MessageUPtr CreateFinishRegisterApplicationHostRequestMessage()
    {
        FinishRegisterApplicationHostRequest requestBody(owner_.Id);

        MessageUPtr request = make_unique<Message>(requestBody);
        request->Headers.Add(Transport::ActorHeader(Actor::ApplicationHostManager));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::FinishRegisterApplicationHostRequest));

        WriteNoise(TraceType, owner_.TraceId, "CreateFinishRegisterApplicationHostRequest: Message={0}, Body={1}", *request, requestBody);

        return move(request);
    }

private:
    ApplicationHost & owner_;
    TimeoutHelper timeoutHelper_;
};

// ********************************************************************************************************************
// ApplicationHost::CloseAsyncOperation Implementation
//
// Start
// Step 1: Close BAP (conditional)
// Step 2: Close RAP
// Step 3: Close Replicator Factories
// Step 4: Close Runtime Table
// Step 5: Unregister Application Host
// Step 6: Close Lease Monitoring
// Step 7: Close Node Host Monitoring
// Step 8: Close IPC Client
// Step 9: Close LogManager
// Step 10: Shutdown KTL System
// Complete
//
class ApplicationHost::CloseAsyncOperation : 
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(CloseAsyncOperation)

public:
    CloseAsyncOperation(
        ActivityDescription const & activityDescription,
        ApplicationHost & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        activityDescription_(activityDescription),
        owner_(owner),
        timeoutHelper_(timeout),
        removedRuntimes_()
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
            owner_.TraceId,
            "Closing ApplicationHost: Id={0}, Timeout={1} ActivityDescription={2}",
            owner_.Id, 
            timeoutHelper_.GetRemainingTime(),
            activityDescription_);

#if !defined(PLATFORM_UNIX)
        if (BackupRestoreServiceConfig::IsBackupRestoreServiceConfigured())
        {
            // Step 1: Close BAP
            CloseBAP(thisSPtr);
        }
        else
#endif
        {
            // Step 2: Close RAP
            CloseRAP(thisSPtr);
        }
    }

private:
#if !defined(PLATFORM_UNIX)
    void CloseBAP(AsyncOperationSPtr const & thisSPtr)
    {
        auto timeout = timeoutHelper_.GetRemainingTime();
        owner_.WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(CloseBAP): Timeout={0} ActivityDescription={1}",
            timeout,
            activityDescription_);

        auto CloseBAPOperation =
            owner_.BackupRestoreAgentProxyObj.BeginClose(
                timeout,
                [this](AsyncOperationSPtr const & operation) { OnCloseBAPCompleted(operation); },
                thisSPtr);

        if (CloseBAPOperation->CompletedSynchronously)
        {
            FinishCloseBAP(CloseBAPOperation);
        }
    }

    void OnCloseBAPCompleted(AsyncOperationSPtr const & CloseBAPOperation)
    {
        if (!CloseBAPOperation->CompletedSynchronously)
        {
            FinishCloseBAP(CloseBAPOperation);
        }
    }

    void FinishCloseBAP(AsyncOperationSPtr const & closeRAPOperation)
    {
        auto error = owner_.BackupRestoreAgentProxyObj.EndClose(closeRAPOperation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "BAP Close: ErrorCode={0} ActivityDescription={1}",
            error,
            activityDescription_);
        if (!error.IsSuccess())
        {
            TryComplete(closeRAPOperation->Parent, error);
            return;
        }

        // Step 2: Close RAP
        CloseRAP(closeRAPOperation->Parent);
    }
#endif

    void CloseRAP(AsyncOperationSPtr const & thisSPtr)
    {
        auto timeout = timeoutHelper_.GetRemainingTime();
        owner_.WriteNoise(
            TraceType, 
            owner_.TraceId, 
            "Begin(CloseRAP): Timeout={0} ActivityDescription={1}", 
            timeout,
            activityDescription_);

        auto CloseRAPOperation =         
            owner_.ReconfigurationAgentProxyObj.BeginClose(
            timeout,
            [this](AsyncOperationSPtr const & operation){ OnCloseRAPCompleted(operation); },
            thisSPtr);

        if (CloseRAPOperation->CompletedSynchronously)
        {
            FinishCloseRAP(CloseRAPOperation);
        }
    }

    void OnCloseRAPCompleted(AsyncOperationSPtr const & CloseRAPOperation)
    {
        if (!CloseRAPOperation->CompletedSynchronously)
        {
            FinishCloseRAP(CloseRAPOperation);
        }
    }

    void FinishCloseRAP(AsyncOperationSPtr const & closeRAPOperation)
    {
        auto error = owner_.ReconfigurationAgentProxyObj.EndClose(closeRAPOperation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "RAP Close: ErrorCode={0} ActivityDescription={1}",
            error,
            activityDescription_);
        if (!error.IsSuccess())
        {
            TryComplete(closeRAPOperation->Parent, error);
            return;
        }

        // Step 3: Close Replicator Factories
        CloseReplicatorFactories(closeRAPOperation->Parent);
    }

    void CloseReplicatorFactories(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.ComReplicatorFactoryObj.Close();
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "Replicator Factory Close: ErrorCode={0} ActivityDescription={1}",
            error,
            activityDescription_);
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        error = owner_.ComTransactionalReplicatorFactoryObj.Close();
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "TransactionalReplicator Factory Close: ErrorCode={0}",
            error);
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        // Step 4: Close Runtime Table
        CloseRuntimeTable(thisSPtr);
    }

    void CloseRuntimeTable(AsyncOperationSPtr const & thisSPtr)
    {
        removedRuntimes_ = owner_.runtimeTable_->Close();
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "RuntimeTable Closed: Removed {0} FabricRuntimeImpl entries. ActivityDescription={1}",
            removedRuntimes_.size(),
            activityDescription_);

        if (owner_.nodeHostProcessClosed_.load())
        {
            // Fabric is already down, no need to unregister
            // Step 6: Close Lease Monitoring
            CloseLeaseMonitoring(thisSPtr);
        }
        else
        {
            // Step 5: Unregister Application Host
            UnregisterApplicationHost(thisSPtr);
        }
    }

    void UnregisterApplicationHost(AsyncOperationSPtr const & thisSPtr)
    {
        TimeSpan timeout = HostingConfig::GetConfig().RequestTimeout;
        if (timeout > timeoutHelper_.GetRemainingTime())
        {
            timeout = timeoutHelper_.GetRemainingTime();
        }
        if (timeout <= TimeSpan::Zero)
        {
            WriteNoise(
                TraceType, 
                owner_.TraceId, 
                "Unregistration of application host was not completed in allocated time {0}.", 
                timeoutHelper_.OriginalTimeout);
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Timeout));
            return;
        }

        MessageUPtr request = CreateUnregisterApplicationHostRequestMessage(activityDescription_);
        WriteNoise(
            TraceType, 
            owner_.TraceId, 
            "Begin(UnegisterApplicationHostRequest): Timeout={0}, Request={1}", 
            timeout,
            *request);
        auto unregisterHostRequest = owner_.Client.BeginRequest(
            move(request),
            timeout,
            [this](AsyncOperationSPtr const & operation){ OnUnregisterApplicationHostReplyRecevied(operation); },
            thisSPtr);
        if (unregisterHostRequest->CompletedSynchronously)
        {
            FinishUnregisterApplicationHost(unregisterHostRequest);
        }
    }

    void OnUnregisterApplicationHostReplyRecevied(AsyncOperationSPtr const & unregisterHostRequest)
    {
        if (!unregisterHostRequest->CompletedSynchronously)
        {
            FinishUnregisterApplicationHost(unregisterHostRequest);
        }
    }

    void FinishUnregisterApplicationHost(AsyncOperationSPtr const & unregisterHostRequest)
    {
        MessageUPtr reply;
        auto error = owner_.Client.EndRequest(unregisterHostRequest, reply);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType, 
                owner_.TraceId, 
                "End(UnregisterApplicationHostRequest): ErrorCode={0}",
                error);

            if (!error.IsError(ErrorCodeValue::ObjectClosed))
            {
                // retry unregister
                WriteNoise(
                    TraceType, 
                    owner_.TraceId, 
                    "Retrying UnregisterApplicationHostRequest"); 
                UnregisterApplicationHost(unregisterHostRequest->Parent);
                return;
            }
            else
            {
                TryComplete(unregisterHostRequest->Parent, error);
                return;
            }
        }

        // process the reply
        UnregisterApplicationHostReply replyBody;
        if (!reply->GetBody<UnregisterApplicationHostReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "GetBody<UnregisterApplicationHostReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);
            TryComplete(unregisterHostRequest->Parent, error);
            return;
        }

        error = replyBody.Error;
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "UnregisterApplicationHostReply: Message={0}, Body={1}",
            *reply,
            replyBody);
        if (!error.IsSuccess())
        {
            TryComplete(unregisterHostRequest->Parent, error);
            return;
        }

        // Step 6: Close Lease Monitoring
        CloseLeaseMonitoring(unregisterHostRequest->Parent);
    }

    void CloseLeaseMonitoring(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.leaseMonitor_)
        {
            owner_.leaseMonitor_->Close();
        }

        // Step 7: Close Node Host Monitoring
        CloseNodeHostMonitoring(thisSPtr);
    }

    void CloseNodeHostMonitoring(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.nodeHostProcessWait_)
        {
            owner_.nodeHostProcessWait_->Cancel();
        }

        // Step 8: Close IPC Client
        CloseIpcClient(thisSPtr);
    }

    void CloseIpcClient(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.IpcHealthReportingClientObj.Close();

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "IPC Health Reporting Client Close: ErrorCode={0} ActivityDescription={1}",
            error,
            activityDescription_);

        error = owner_.ThrottledIpcHealthReportingClientObj.Close();

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "Throttled IPC Health Reporting Client Close: ErrorCode={0}",
            error);
        
        owner_.UnregisterIpcMessageHandler();

        error = owner_.Client.Close();
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "IPC Client Close: ErrorCode={0} ActivityDescription={1}",
            error,
            activityDescription_);

        {
            auto runtimes = move(removedRuntimes_);
            if (owner_.nodeHostProcessClosed_.load())
            {
                // raise nodehost closed event, notify the exit handlers 
                owner_.RaiseNodeHostProcessClosedEvent();
    
                for(auto iter = runtimes.cbegin(); iter != runtimes.cend(); ++iter)
                {
                    (*iter)->OnFabricProcessExited();
                }
            }
        }

        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        CloseLogManager(thisSPtr);
    }

    void CloseLogManager(AsyncOperationSPtr const & thisSPtr)
    {
        owner_.WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(CloseLogManager) ActivityDescription={0}", activityDescription_);

        auto closeLogManagerOperation =
            owner_.logManager_->BeginClose(
                [this](AsyncOperationSPtr const & operation) { OnCloseLogManagerCompleted(operation); },
                thisSPtr,
                CancellationToken::None);

        if (closeLogManagerOperation->CompletedSynchronously)
        {
            FinishCloseLogManager(closeLogManagerOperation);
        }
    }

    void OnCloseLogManagerCompleted(AsyncOperationSPtr const & closeLogManagerOperation)
    {
        if (!closeLogManagerOperation->CompletedSynchronously)
        {
            FinishCloseLogManager(closeLogManagerOperation);
        }
    }

    void FinishCloseLogManager(AsyncOperationSPtr const & closeLogManagerOperation)
    {
        auto error = owner_.logManager_->EndClose(closeLogManagerOperation);
        if (!error.IsSuccess())
        {
            owner_.WriteWarning(
                TraceType,
                owner_.TraceId,
                "End(CloseLogManager): ErrorCode={0}",
                error);

            TryComplete(closeLogManagerOperation->Parent, error);
            return;
        }

        owner_.WriteNoise(
            TraceType,
            owner_.TraceId,
            "LogManager Closed. ActivityDescription={0}", activityDescription_);

        owner_.logManager_ = nullptr;

        // Post this on the threadpool since we are currently on a ktl thread, and deactivating on a ktl
        // thread is not allowed.
        Threadpool::Post(
            [this, closeLogManagerOperation]()
        {
            ShutdownKtlSystem(closeLogManagerOperation->Parent);
        });
    }

    void ShutdownKtlSystem(AsyncOperationSPtr const & thisSPtr)
    {
        ErrorCode error = owner_.ShutdownKtlSystem(timeoutHelper_.GetRemainingTime());
        TryComplete(thisSPtr, error);
    }

    MessageUPtr CreateUnregisterApplicationHostRequestMessage(ActivityDescription const & activityDescription)
    {
        UnregisterApplicationHostRequest requestBody(activityDescription, owner_.Id);

        MessageUPtr request = make_unique<Message>(requestBody);
        request->Headers.Add(Transport::ActorHeader(Actor::ApplicationHostManager));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::UnregisterApplicationHostRequest));

        WriteNoise(TraceType, owner_.TraceId, "CreateUnregisterApplicationHostRequestMessage: Message={0}, Body={1}", *request, requestBody);

        return move(request);
    }

private:
    ActivityDescription activityDescription_;
    ApplicationHost & owner_;
    TimeoutHelper timeoutHelper_;
    vector<FabricRuntimeImplSPtr> removedRuntimes_;
};

// ********************************************************************************************************************
// ApplicationHost::CreateComFabricRuntimeAsyncOperation Implementation
//
class ApplicationHost::CreateComFabricRuntimeAsyncOperation : 
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(CreateComFabricRuntimeAsyncOperation)

public:
    CreateComFabricRuntimeAsyncOperation(
        ApplicationHost & owner,
        FabricRuntimeContextUPtr && fabricRuntimeContextUPtr,
        ComPointer<IFabricProcessExitHandler> const & fabricExitHandler,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        fabricRuntimeContextUPtr_(move(fabricRuntimeContextUPtr)),
        fabricExitHandler_(fabricExitHandler),
        timeoutHelper_(timeout)
    {
    }

    virtual ~CreateComFabricRuntimeAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out ComPointer<ComFabricRuntime> & comFabricRuntime)
    {
        auto thisPtr = AsyncOperation::End<CreateComFabricRuntimeAsyncOperation>(operation);
        comFabricRuntime = thisPtr->result_;
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        // create FabricRuntime object
        auto error = owner_.OnCreateAndAddFabricRuntime(fabricRuntimeContextUPtr_, fabricExitHandler_, fabricRuntime_);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "OnCreateAndAddFabricRuntime: ErrorCode={0}, FabricRuntime={1}", 
            error,
            static_cast<void*>(fabricRuntime_.get()));        
        
        if (!error.IsSuccess()) 
        {
            TryComplete(thisSPtr, error);
            return;
        }

        // register the runtime
        RegisterFabricRuntime(thisSPtr);
    }

private:
    void RegisterFabricRuntime(AsyncOperationSPtr const & thisSPtr)
    {
        // Registers the Runtime in Fabric and validates the local runtime entry
        auto registerRuntimeOperation = owner_.BeginRegisterFabricRuntime(
            fabricRuntime_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation){ this->OnRegisterRuntimeCompleted(operation); },
            thisSPtr);
        if (registerRuntimeOperation->CompletedSynchronously)
        {
            FinishRegisterRuntime(registerRuntimeOperation);
        }
    }

    void OnRegisterRuntimeCompleted(AsyncOperationSPtr const & registerRuntimeOperation)
    {
        if (!registerRuntimeOperation->CompletedSynchronously)
        {
            FinishRegisterRuntime(registerRuntimeOperation);
        }
    }

    void FinishRegisterRuntime(AsyncOperationSPtr const & registerRuntimeOperation)
    {
        auto error = owner_.EndRegisterFabricRuntime(registerRuntimeOperation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "RegisterFabricRuntime: ErrorCode={0}", 
            error);
        if (!error.IsSuccess())
        {
            TryComplete(registerRuntimeOperation->Parent, error);
            return;
        }

        result_ = make_com<ComFabricRuntime>(fabricRuntime_);
        TryComplete(registerRuntimeOperation->Parent, ErrorCode(ErrorCodeValue::Success));
    }

private:
    ApplicationHost & owner_;
    TimeoutHelper timeoutHelper_;
    FabricRuntimeImplSPtr fabricRuntime_;
    ComPointer<ComFabricRuntime> result_;
    FabricRuntimeContextUPtr fabricRuntimeContextUPtr_;
    ComPointer<IFabricProcessExitHandler> fabricExitHandler_;
};

// ********************************************************************************************************************
// ApplicationHost::RegisterFabricRuntimeAsyncOperation Implementation
//
class ApplicationHost::RegisterFabricRuntimeAsyncOperation : 
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(RegisterFabricRuntimeAsyncOperation)

public:
    RegisterFabricRuntimeAsyncOperation(
        ApplicationHost & owner,
        FabricRuntimeImplSPtr const & runtime,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        runtime_(runtime),
        timeoutHelper_(timeout)
    {
    }

    virtual ~RegisterFabricRuntimeAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<RegisterFabricRuntimeAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Registering FabricRuntime. Id={0}, Context={1}", 
            runtime_->RuntimeId,
            runtime_->GetRuntimeContext());

        TimeSpan timeout = HostingConfig::GetConfig().RequestTimeout;
        if (timeout > timeoutHelper_.GetRemainingTime())
        {
            timeout = timeoutHelper_.GetRemainingTime();
        }

        MessageUPtr request = CreateRegisterFabricRuntimeRequestMessage();
        WriteNoise(
            TraceType, 
            owner_.TraceId, 
            "Begin(RegisterFabricRuntimeRequest): Timeout={0}, Request={1}", 
            timeout,
            *request);
        auto registerRuntimeRequest = owner_.Client.BeginRequest(
            move(request),
            timeout,
            [this](AsyncOperationSPtr const & operation){ OnRegisterFabricRuntimeReplyReceived(operation); },
            thisSPtr);
        if (registerRuntimeRequest->CompletedSynchronously)
        {
            FinishRegisterFabricRuntime(registerRuntimeRequest);
        }
    }

private:
    void OnRegisterFabricRuntimeReplyReceived(AsyncOperationSPtr const & registerRuntimeRequest)
    {
        if (!registerRuntimeRequest->CompletedSynchronously)
        {
            FinishRegisterFabricRuntime(registerRuntimeRequest);
        }
    }

    void FinishRegisterFabricRuntime(AsyncOperationSPtr const & registerRuntimeRequest)
    {
        MessageUPtr reply;
        auto error = owner_.Client.EndRequest(registerRuntimeRequest, reply);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType, 
                owner_.TraceId, 
                "End(RegisterFabricRuntimeRequest): ErrorCode={0}",
                error);
            owner_.UnregisterRuntimeAsync(runtime_->RuntimeId);
            TryComplete(registerRuntimeRequest->Parent, error);
            return;
        }

        RegisterFabricRuntimeReply replyBody;
        if (!reply->GetBody<RegisterFabricRuntimeReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "GetBody<RegisterFabricRuntimeReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);
            TryComplete(registerRuntimeRequest->Parent, error);
            return;
        }

        error = replyBody.Error;
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "RegisterFabricRuntimeReply: Message={0}, Body={1}",
            *reply,
            replyBody);
        if (!error.IsSuccess())
        {
            TryComplete(registerRuntimeRequest->Parent, error);
            return;
        }

        ValidateRuntimeTableEntry(registerRuntimeRequest->Parent);
    }

    void ValidateRuntimeTableEntry(AsyncOperationSPtr const & thisSPtr)
    {
        // validate runtime entry in the local registration table
        auto error = owner_.runtimeTable_->Validate(runtime_);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "RuntimeTable.Add: Error={0}, RuntimeId={1}",
            error,
            runtime_->RuntimeId);
        if (!error.IsSuccess())
        {
            owner_.UnregisterRuntimeAsync(runtime_->RuntimeId);
            TryComplete(thisSPtr, error);
            return;
        }

        TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
    }

    MessageUPtr CreateRegisterFabricRuntimeRequestMessage()
    {
        RegisterFabricRuntimeRequest requestBody(runtime_->GetRuntimeContext());

        MessageUPtr request = make_unique<Message>(requestBody);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricRuntimeManager));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::RegisterFabricRuntimeRequest));

        WriteNoise(TraceType, owner_.TraceId, "CreateRegisterFabricRuntimeRequestMessage: Message={0}, Body={1}", *request, requestBody);

        return move(request);
    }

private:
    ApplicationHost & owner_;
    FabricRuntimeImplSPtr runtime_;
    TimeoutHelper timeoutHelper_;
};

// ********************************************************************************************************************
// ApplicationHost::UnregisterRuntimeAsyncOperation Implementation
//
class ApplicationHost::UnregisterFabricRuntimeAsyncOperation : 
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(UnregisterFabricRuntimeAsyncOperation)

public:
    UnregisterFabricRuntimeAsyncOperation(
        ApplicationHost & owner,
        wstring const & runtimeId,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        runtimeId_(runtimeId)
    {
    }

    virtual ~UnregisterFabricRuntimeAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<UnregisterFabricRuntimeAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        UnregisterFabricRuntime(thisSPtr);
    }

private:
    void UnregisterFabricRuntime(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Unregistering FabricRuntime. Id={0}", 
            runtimeId_);

        MessageUPtr request = CreateUnregisterFabricRuntimeRequestMessage();
        TimeSpan timeout = HostingConfig::GetConfig().RequestTimeout;

        WriteNoise(
            TraceType, 
            owner_.TraceId, 
            "Begin(UnregisterFabricRuntimeRequest): Timeout={0}, Request={1}", 
            timeout,
            *request);
        auto unregisterRuntimeRequest = owner_.Client.BeginRequest(
            move(request),
            timeout,
            [this](AsyncOperationSPtr const & operation){ OnUnregisterFabricRuntimeReplyReceived(operation); },
            thisSPtr);
        if (unregisterRuntimeRequest->CompletedSynchronously)
        {
            FinishUnregisterFabricRuntime(unregisterRuntimeRequest);
        }
    }

    void OnUnregisterFabricRuntimeReplyReceived(AsyncOperationSPtr const & unregisterRuntimeRequest)
    {
        if (!unregisterRuntimeRequest->CompletedSynchronously)
        {
            FinishUnregisterFabricRuntime(unregisterRuntimeRequest);
        }
    }

    void FinishUnregisterFabricRuntime(AsyncOperationSPtr const & unregisterRuntimeRequest)
    {
        MessageUPtr reply;
        auto error = owner_.Client.EndRequest(unregisterRuntimeRequest, reply);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "UnregisterFabricRuntime: ErrorCode={0}",
                error);
            RetryIfNeeded(unregisterRuntimeRequest->Parent, error);
            return;
        }

        // process received response
        UnregisterFabricRuntimeReply replyBody;
        if (!reply->GetBody<UnregisterFabricRuntimeReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "GetBody<UnregisterFabricRuntimeReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);
            TryComplete(unregisterRuntimeRequest->Parent, error);
            return;
        }

        error = replyBody.Error;
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "UnregisterFabricRuntimeReply: Message={0}, Body={1}",
            *reply,
            replyBody);

        FabricRuntimeImplSPtr removed;
        owner_.runtimeTable_->Remove(runtimeId_, removed).ReadValue(); // ignore error
        TryComplete(unregisterRuntimeRequest->Parent, error);
    }

    void RetryIfNeeded(AsyncOperationSPtr const & thisSPtr, ErrorCode const error)
    {
        if (error.IsError(ErrorCodeValue::Timeout))
        {
            WriteNoise(
                TraceType,
                owner_.TraceId,
                "Retrying UnregisterFabricRuntime : IPC ErrorCode={0}, Id={1}",
                error,
                runtimeId_);
            UnregisterFabricRuntime(thisSPtr);
            return;
        }
        else
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "NotRetrying UnregisterFabricRuntime: IPC ErrorCode={0}, Id={1}",
                error,
                runtimeId_);
            FabricRuntimeImplSPtr removed;
            owner_.runtimeTable_->Remove(runtimeId_, removed).ReadValue(); // ignore
            TryComplete(thisSPtr, error);
            return;
        }
    }

    MessageUPtr CreateUnregisterFabricRuntimeRequestMessage()
    {
        UnregisterFabricRuntimeRequest requestBody(runtimeId_);

        MessageUPtr request = make_unique<Message>(requestBody);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricRuntimeManager));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::UnregisterFabricRuntimeRequest));

        WriteNoise(TraceType, owner_.TraceId, "CreateUnregisterFabricRuntimeRequestMessage: Message={0}, Body={1}", *request, requestBody);

        return move(request);
    }

private:
    ApplicationHost & owner_;
    wstring const runtimeId_;
};

// ********************************************************************************************************************
// ApplicationHost::GetFactoryAndCreateInstanceAsyncOperation Implementation
//
class ApplicationHost::GetFactoryAndCreateInstanceAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(GetFactoryAndCreateInstanceAsyncOperation)

public:
    GetFactoryAndCreateInstanceAsyncOperation(
        ApplicationHost & owner,
        wstring const & runtimeId,
        ServiceTypeIdentifier const & serviceTypeId,
        wstring const & serviceName,
        vector<byte> const & initializationData,
        Guid const & partitionId,
        int64 & instanceId,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        runtimeId_(runtimeId),
        serviceTypeId_(serviceTypeId),
        serviceName_(serviceName),
        initializationData_(initializationData),
        partitionId_(partitionId),
        instanceId_(instanceId),
        statelessService_()
    {
    }

    __declspec (property(get = get_StatelessService)) Common::ComPointer<IFabricStatelessServiceInstance>& StatelessService;
    inline Common::ComPointer<IFabricStatelessServiceInstance> & get_StatelessService() { return statelessService_; }

    virtual ~GetFactoryAndCreateInstanceAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<GetFactoryAndCreateInstanceAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        FabricRuntimeImplSPtr runtime;
        ErrorCode retValue = owner_.GetFabricRuntime(runtimeId_, runtime);
        if (!retValue.IsSuccess())
        {
            TryComplete(thisSPtr, retValue);
            return;
        }

        AsyncOperationSPtr operation = runtime->FactoryManager->BeginGetStatelessServiceFactory(
            serviceTypeId_,
            [this](AsyncOperationSPtr const & operation) { OnFindCompleted(operation, false); },
            thisSPtr);

        OnFindCompleted(operation, true);
    }

private:
    void OnFindCompleted(Common::AsyncOperationSPtr const & findAsyncOperation, bool expectedCompletedSynchronously)
    {
        if (findAsyncOperation->CompletedSynchronously != expectedCompletedSynchronously)
            return;

        ErrorCode error = CreateInstance(findAsyncOperation);

        findAsyncOperation->Parent->TryComplete(findAsyncOperation->Parent, error);
    }

    ErrorCode CreateInstance(AsyncOperationSPtr const & findAsyncOperation)
    {
        FabricRuntimeImplSPtr runtime;
        ErrorCode retValue;

        retValue = owner_.GetFabricRuntime(runtimeId_, runtime);
        if (!retValue.IsSuccess())
        {
            return retValue;
        }

        std::wstring originalServiceTypeName;
        ComProxyStatelessServiceFactorySPtr statelessServiceFactory;

        // Get the service factory that will create the service instance
        retValue = runtime->FactoryManager->EndGetStatelessServiceFactory(
            findAsyncOperation,
            originalServiceTypeName,
            statelessServiceFactory);

        TESTASSERT_IF(
            retValue.IsError(ErrorCodeValue::HostingServiceTypeNotRegistered) &&
            runtime->FactoryManager->Test_IsServiceTypeInvalid(serviceTypeId_),
            "Service type {0} is invalid and validation should be in process", serviceTypeId_);

        if (!retValue.IsSuccess())
        {
            return retValue;
        }

        // Create the service instance from the factory
        return statelessServiceFactory->CreateInstance(
            originalServiceTypeName,
            serviceName_,
            initializationData_,
            partitionId_,
            instanceId_,
            statelessService_);
    }

private:
    ApplicationHost & owner_;
    std::wstring const runtimeId_;
    ServiceTypeIdentifier const serviceTypeId_;
    std::wstring const serviceName_;
    std::vector<byte> const initializationData_;
    Common::Guid const partitionId_;
    int64 instanceId_;
    Common::ComPointer<IFabricStatelessServiceInstance> statelessService_;
};

// ********************************************************************************************************************
// ApplicationHost::GetFactoryAndCreateReplicaAsyncOperation Implementation
//
class ApplicationHost::GetFactoryAndCreateReplicaAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(GetFactoryAndCreateReplicaAsyncOperation)

public:
    GetFactoryAndCreateReplicaAsyncOperation(
        ApplicationHost & owner,
        wstring const & runtimeId,
        ServiceTypeIdentifier const & serviceTypeId,
        wstring const & serviceName,
        vector<byte> const & initializationData,
        Guid const & partitionId,
        int64 & replicaId,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        runtimeId_(runtimeId),
        serviceTypeId_(serviceTypeId),
        serviceName_(serviceName),
        initializationData_(initializationData),
        partitionId_(partitionId),
        replicaId_(replicaId),
        statefulService_()
    {
    }

    __declspec (property(get = get_StatefulService)) Common::ComPointer<IFabricStatefulServiceReplica> & StatefulService;
    inline Common::ComPointer<IFabricStatefulServiceReplica> & get_StatefulService() { return statefulService_; }

    virtual ~GetFactoryAndCreateReplicaAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<GetFactoryAndCreateReplicaAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        FabricRuntimeImplSPtr runtime;
        ErrorCode retValue = owner_.GetFabricRuntime(runtimeId_, runtime);
        if (!retValue.IsSuccess())
        {
            TryComplete(thisSPtr, retValue);
            return;
        }

        AsyncOperationSPtr operation = runtime->FactoryManager->BeginGetStatefulServiceFactory(
            serviceTypeId_,
            [this](AsyncOperationSPtr const & operation) { OnFindCompleted(operation, false); },
            thisSPtr);

        OnFindCompleted(operation, true);
    }

private:
    void OnFindCompleted(Common::AsyncOperationSPtr const & findAsyncOperation, bool expectedCompletedSynchronously)
    {
        if (findAsyncOperation->CompletedSynchronously != expectedCompletedSynchronously)
            return;

        ErrorCode error = CreateReplica(findAsyncOperation);

        findAsyncOperation->Parent->TryComplete(findAsyncOperation->Parent, error);
    }

    ErrorCode CreateReplica(AsyncOperationSPtr const & findAsyncOperation)
    {
        FabricRuntimeImplSPtr runtime;
        ErrorCode retValue;

        retValue = owner_.GetFabricRuntime(runtimeId_, runtime);
        if (!retValue.IsSuccess())
        {
            return retValue;
        }

        std::wstring originalServiceTypeName;
        ComProxyStatefulServiceFactorySPtr statefulServiceFactory;

        // Get the service factory that will create the service replica
        retValue = runtime->FactoryManager->EndGetStatefulServiceFactory(
            findAsyncOperation,
            originalServiceTypeName,
            statefulServiceFactory);

        TESTASSERT_IF(
            retValue.IsError(ErrorCodeValue::HostingServiceTypeNotRegistered) &&
            runtime->FactoryManager->Test_IsServiceTypeInvalid(serviceTypeId_),
            "Service type {0} is invalid and validation should be in process", serviceTypeId_);

        if (!retValue.IsSuccess())
        {
            return retValue;
        }

        // Create the service replica from the factory
        return statefulServiceFactory->CreateReplica(
            originalServiceTypeName,
            serviceName_,
            initializationData_,
            partitionId_,
            replicaId_,
            statefulService_);
    }

private:
    ApplicationHost & owner_;
    std::wstring const runtimeId_;
    ServiceTypeIdentifier const serviceTypeId_;
    std::wstring const serviceName_;
    std::vector<byte> const initializationData_;
    Common::Guid const partitionId_;
    int64 replicaId_;
    Common::ComPointer<IFabricStatefulServiceReplica> statefulService_;
};

// ********************************************************************************************************************
// ApplicationHost Implementation
//
ApplicationHost::ApplicationHost(
    ApplicationHostContext const & hostContext,
    wstring const & runtimeServiceAddress,
    PCCertContext certContext,
    wstring const & serverThumbprint,
    bool useSystemServiceSharedLogSettings,
    KtlSystem * ktlSystem)
    : ComponentRoot(),
    AsyncFabricComponent(),
    hostContext_(hostContext),
    runtimeServiceAddress_(runtimeServiceAddress),
    ipcClient_(),
    ipcUseSsl_(certContext != nullptr),
    certContext_(certContext),
    serverThumbprint_(serverThumbprint),
    comReplicatorFactory_(),
    comTransactionalReplicatorFactory_(),
    raProxy_(),
    ipcHealthReportingClient_(),
    throttledIpcHealthReportingClient_(),
    runtimeTable_(),
    nodeHostProcessWait_(),
    leaseMonitor_(),
    nodeHostProcessClosedEvent_(),
    nodeHostProcessClosed_(false),
    codePackageTable_(),
    nodeContext_(),
    nodeContextLock_(),
    ktlSystemWrapper_(),
    useSystemServiceSharedLogSettings_(useSystemServiceSharedLogSettings),
    applicationSharedLogSettings_(),
    systemServicesSharedLogSettings_()
{
    ASSERT_IF(hostContext_.HostId.length() == 0, "HostId must not be empty.");
    hostContext_.ProcessId = ::GetCurrentProcessId();

    auto runtimeTable = make_unique<RuntimeTable>(*this);

    auto ipcClient = make_unique<IpcClient>(
        *this,
        Id,
        runtimeServiceAddress,
        true /* allow use of unreliable transport */,
        L"ApplicationHost");

    auto ktlSystemWrapper = make_unique<KtlSystemWrapper>(
        *this, 
        ktlSystem, 
        CommonConfig::GetConfig().UseGlobalKtlSystem);

    this->SetTraceId(L"AppHost-" + Id, Constants::ApplicationHostTraceIdMaxLength);
    throttledIpcHealthReportingClient_ = make_shared<Client::IpcHealthReportingClient>(*this, *ipcClient.get(), true, this->get_TraceId(),
        Transport::Actor::ApplicationHostManager, HostingConfig::GetConfig().IsApplicationHostHealthReportingEnabled);

    ipcHealthReportingClient_ = make_shared<Client::IpcHealthReportingClient>(*this, *ipcClient.get(), false, L"RAP", Transport::Actor::RA, true);
    
    // Invoke RAP via factory function
    Reliability::ReplicationComponent::ReplicatorFactoryConstructorParameters replicatorFactoryConstructorParameters;
    replicatorFactoryConstructorParameters.Root = this;
    auto comReplicatorFactory = Reliability::ReplicationComponent::ReplicatorFactoryFactory(replicatorFactoryConstructorParameters);

    TxnReplicator::TransactionalReplicatorFactoryConstructorParameters transactionalReplicatorFactoryConstructorParameters;
    transactionalReplicatorFactoryConstructorParameters.Root = this;
    auto comTransactionalReplicatorFactory = TxnReplicator::TransactionalReplicatorFactoryFactory(transactionalReplicatorFactoryConstructorParameters);

    Reliability::ReconfigurationAgentComponent::ReconfigurationAgentProxyConstructorParameters rapConstructorParameters;
    rapConstructorParameters.ApplicationHost = this;
    rapConstructorParameters.IpcClient = ipcClient.get();
    rapConstructorParameters.ReplicatorFactory = comReplicatorFactory.get();
    rapConstructorParameters.TransactionalReplicatorFactory = comTransactionalReplicatorFactory.get();
    rapConstructorParameters.Root = this;
    rapConstructorParameters.IpcHealthReportingClient = ipcHealthReportingClient_;
    rapConstructorParameters.ThrottledIpcHealthReportingClient = throttledIpcHealthReportingClient_;
    auto raProxy = Reliability::ReconfigurationAgentComponent::ReconfigurationAgentProxyFactory(rapConstructorParameters);
    
#if !defined(PLATFORM_UNIX)
    Management::BackupRestoreAgentComponent::IBackupRestoreAgentProxyUPtr baProxy = nullptr;
    auto isBackupRestoreServiceConfigured = BackupRestoreServiceConfig::IsBackupRestoreServiceConfigured();
    
    if (isBackupRestoreServiceConfigured)
    {
        // Construct BAP via factory function
        Management::BackupRestoreAgentComponent::BackupRestoreAgentProxyConstructorParameters bapConstructorParameters;
        bapConstructorParameters.ApplicationHost = this;
        bapConstructorParameters.IpcClient = ipcClient.get();
        bapConstructorParameters.Root = this;
        baProxy = Management::BackupRestoreAgentComponent::BackupRestoreAgentProxyFactory(bapConstructorParameters);
    }
#endif

    // Assignments
    runtimeTable_ = move(runtimeTable);
    ipcClient_ = move(ipcClient);
    comReplicatorFactory_ = move(comReplicatorFactory);
    comTransactionalReplicatorFactory_ = move(comTransactionalReplicatorFactory);
    raProxy_ = move(raProxy);
#if !defined(PLATFORM_UNIX)
    baProxy_ = isBackupRestoreServiceConfigured ? move(baProxy) : nullptr;
#endif
    auto codePackageTable = make_unique<CodePackageTable>(*this);
    codePackageTable_ = move(codePackageTable);
    ktlSystemWrapper_ = move(ktlSystemWrapper);

    WriteNoise(
        TraceType,
        TraceId,
        "Application Host Constructed. Id={0}, Type={1}, ProcessId={2}, RuntimeConnectionAddress={3}",
        Id,
        Type,
        hostContext_.ProcessId,
        runtimeServiceAddress);
}

ApplicationHost::~ApplicationHost()
{
    WriteNoise(TraceType, TraceId, "Application Host Destructed.");
}

AsyncOperationSPtr ApplicationHost::BeginCreateComFabricRuntime(
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return BeginCreateComFabricRuntime(
        move(FabricRuntimeContextUPtr()),
        ComPointer<IFabricProcessExitHandler>(),
        timeout,
        callback,
        parent);
}

AsyncOperationSPtr ApplicationHost::BeginCreateComFabricRuntime(
    ComPointer<IFabricProcessExitHandler> const & fabricExitHandler,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return BeginCreateComFabricRuntime(
        move(FabricRuntimeContextUPtr()),
        fabricExitHandler,
        timeout,
        callback,
        parent);
}

AsyncOperationSPtr ApplicationHost::BeginCreateComFabricRuntime(
    FabricRuntimeContextUPtr && fabricRuntimeContextUPtr,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return BeginCreateComFabricRuntime(
        move(fabricRuntimeContextUPtr),
        ComPointer<IFabricProcessExitHandler>(),
        timeout,
        callback,
        parent);
}

AsyncOperationSPtr ApplicationHost::BeginCreateComFabricRuntime(
    FabricRuntimeContextUPtr && fabricRuntimeContextUPtr,
    ComPointer<IFabricProcessExitHandler> const & fabricExitHandler,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CreateComFabricRuntimeAsyncOperation>(
        *this,
        move(fabricRuntimeContextUPtr),
        fabricExitHandler,
        timeout,
        callback,
        parent);
}

ErrorCode ApplicationHost::EndCreateComFabricRuntime(
    AsyncOperationSPtr const & operation,
    __out ComPointer<ComFabricRuntime> & comFabricRuntime)
{
    return CreateComFabricRuntimeAsyncOperation::End(operation, comFabricRuntime);
}

#if !defined(PLATFORM_UNIX)
ErrorCode ApplicationHost::CreateBackupRestoreAgent(__out ComPointer<ComFabricBackupRestoreAgent> & comBackupRestoreAgent)
{
    if (!BackupRestoreServiceConfig::IsBackupRestoreServiceConfigured())
    {
        return ErrorCodeValue::InvalidOperation;
    }

    comBackupRestoreAgent = make_com<ComFabricBackupRestoreAgent>(make_shared<FabricBackupRestoreAgentImpl>(*this, *this));
    return ErrorCode::Success();
}
#endif

ErrorCode ApplicationHost::GetFabricRuntime(wstring const & runtimeId, __out FabricRuntimeImplSPtr & runtime)
{
    return runtimeTable_->Find(runtimeId, runtime);
}

ErrorCode ApplicationHost::GetKtlSystem(__out KtlSystem ** ktlSystem)
{
    (*ktlSystem) = &ktlSystemWrapper_->KtlSystemObject;
    return ErrorCodeValue::Success;
}

ErrorCode ApplicationHost::GetCodePackageActivationContext(
    CodePackageContext const & codeContext,
    __out CodePackageActivationContextSPtr & codePackageActivationContext)
{
    return this->OnGetCodePackageActivationContext(codeContext, codePackageActivationContext);
}

ErrorCode ApplicationHost::GetCodePackageActivator(
    _Out_ ApplicationHostCodePackageActivatorSPtr & codePackageActivator)
{
    return this->OnGetCodePackageActivator(codePackageActivator);
}

bool ApplicationHost::IsLeaseExpired()
{
    if (leaseMonitor_)
    {
        return this->leaseMonitor_->IsLeaseExpired();
    }
    else
    {
        Assert::CodingError("Lease can be checked only after ApplicationHost is opened.");
    }
}

AsyncOperationSPtr ApplicationHost::BeginGetFactoryAndCreateInstance(
    wstring const & runtimeId,
    ServiceTypeIdentifier const & serviceTypeId,
    ServicePackageActivationContext const & activationContext,
    wstring const & serviceName,
    vector<byte> const & initializationData,
    Guid const & partitionId,
    int64 instanceId,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    UNREFERENCED_PARAMETER(activationContext);

    return AsyncOperation::CreateAndStart<GetFactoryAndCreateInstanceAsyncOperation>(
        *this,
        runtimeId,
        serviceTypeId,
        serviceName,
        initializationData,
        partitionId,
        instanceId,
        callback,
        parent);
}

ErrorCode ApplicationHost::EndGetFactoryAndCreateInstance(
    AsyncOperationSPtr const & operation,
    __out ComPointer<IFabricStatelessServiceInstance> & statelessService)
{
    statelessService = static_cast<GetFactoryAndCreateInstanceAsyncOperation*>(operation.get())->StatelessService;
    return GetFactoryAndCreateInstanceAsyncOperation::End(operation);
}

AsyncOperationSPtr ApplicationHost::BeginGetFactoryAndCreateReplica(
    wstring const & runtimeId,
    ServiceTypeIdentifier const & serviceTypeId,
    ServicePackageActivationContext const & activationContext,
    wstring const & serviceName,
    vector<byte> const & initializationData,
    Guid const & partitionId,
    int64 replicaId,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    UNREFERENCED_PARAMETER(activationContext);

    return AsyncOperation::CreateAndStart<GetFactoryAndCreateReplicaAsyncOperation>(
        *this,
        runtimeId,
        serviceTypeId,
        serviceName,
        initializationData,
        partitionId,
        replicaId,
        callback,
        parent);
}

ErrorCode ApplicationHost::EndGetFactoryAndCreateReplica(
    AsyncOperationSPtr const & operation,
    __out ComPointer<IFabricStatefulServiceReplica> & statefulService)
{
    statefulService = static_cast<GetFactoryAndCreateReplicaAsyncOperation*>(operation.get())->StatefulService;
    return GetFactoryAndCreateReplicaAsyncOperation::End(operation);
}

ErrorCode ApplicationHost::GetHostInformation(
    wstring & nodeIdOut,
    uint64 & nodeInstanceIdOut,
    wstring & hostIdOut,
    wstring & nodeNameOut)
{
    Hosting2::FabricNodeContextSPtr nodeContext;
    auto error = GetFabricNodeContext(nodeContext);
    if (!error.IsSuccess())
    {
        return error;
    }

    nodeInstanceIdOut = nodeContext->NodeInstanceId;
    nodeIdOut = nodeContext->NodeId;
    nodeNameOut = nodeContext->NodeName;
    hostIdOut = HostContext.HostId;
    return error;
}

ErrorCode ApplicationHost::OnGetCodePackageActivator(_Out_ ApplicationHostCodePackageActivatorSPtr & )
{
    //
    // ApplicationHost types that support code package activation 
    // should override this method.
    //
    return ErrorCode(ErrorCodeValue::OperationNotSupported);
}

Common::ErrorCode ApplicationHost::OnCodePackageEvent(CodePackageEventDescription const &)
{
    //
    // ApplicationHost types that support code package activation 
    // should override this method.
    //
    return ErrorCode(ErrorCodeValue::OperationNotSupported);
}

ErrorCode ApplicationHost::GetCodePackageActivationContext(
    __in wstring const & runtimeId,
    __out ComPointer<IFabricCodePackageActivationContext> & outCodePackageActivationContext)
{
    FabricRuntimeImplSPtr fabricRuntime;
    CodePackageActivationContextSPtr codePackageActivationContextSPtr;

    auto error = GetFabricRuntime(runtimeId, fabricRuntime);

    if (!error.IsSuccess())
    {
        return error;
    }

    error = OnGetCodePackageActivationContext(
        fabricRuntime->GetRuntimeContext().CodeContext, 
        codePackageActivationContextSPtr);

    if (!error.IsSuccess())
    {
        return error;
    }
    
    auto comCodePackageActivationContext = make_com<ComCodePackageActivationContext>(codePackageActivationContextSPtr);

    HRESULT hr = comCodePackageActivationContext->QueryInterface(
        IID_IFabricCodePackageActivationContext, 
        outCodePackageActivationContext.VoidInitializationAddress());

    ASSERT_IFNOT(SUCCEEDED(hr), "ComCodePackageActivationContext must implement IFabricCodePackageActivationContext");

    return ErrorCodeValue::Success;
}

ErrorCode ApplicationHost::GetCodePackageActivator(
    _Out_ ComPointer<IFabricCodePackageActivator> & codePackageActivator)
{
    ApplicationHostCodePackageActivatorSPtr codePackageActivatorSPtr;

    auto error = this->OnGetCodePackageActivator(codePackageActivatorSPtr);
    if (!error.IsSuccess())
    {
        return error;
    }

    auto comCodePackageActivator = make_com<ComApplicationHostCodePackageActivator>(codePackageActivatorSPtr);

    auto hr = comCodePackageActivator->QueryInterface(
        IID_IFabricCodePackageActivator,
        codePackageActivator.VoidInitializationAddress());

    ASSERT_IFNOT(SUCCEEDED(hr), "ComApplicationHostCodePackageActivator must implement IFabricCodePackageActivator");

    return ErrorCodeValue::Success;
}

AsyncOperationSPtr ApplicationHost::OnBeginOpen(
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

ErrorCode ApplicationHost::OnEndOpen(AsyncOperationSPtr const & asyncOperation)
{
    return OpenAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr ApplicationHost::OnBeginClose(
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    this->codePackageTable_->Close();
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(
        ActivityDescription(ActivityId(), ActivityType::Enum::ServicePackageEvent),
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode ApplicationHost::OnEndClose(AsyncOperationSPtr const & asyncOperation)
{
    return CloseAsyncOperation::End(asyncOperation);
}

// Step 1: Abort BAP (conditional)
// Step 2: Abort RAP
// Step 3: Abort Replicator Factories
// Step 4: Abort Runtime Table
// Step 5: Abort Lease Monitoring
// Step 6: Abort Node Host Monitoring
// Step 7: Abort IPC Client
// Step 8: Abort LogManager
// Step 9: Shutdown the KTL System
void ApplicationHost::OnAbort()
{
    WriteNoise(
        TraceType,
        TraceId,
        "Aborting ApplicationHost");

    this->codePackageTable_->Close();

#if !defined(PLATFORM_UNIX)
    if (BackupRestoreServiceConfig::IsBackupRestoreServiceConfigured())
    {
        BackupRestoreAgentProxyObj.Abort();
    }
#endif

    ReconfigurationAgentProxyObj.Abort();

    ComReplicatorFactoryObj.Abort();
    ComTransactionalReplicatorFactoryObj.Abort();

    auto removedRuntimes = runtimeTable_->Close();

    if (leaseMonitor_)
    {
        leaseMonitor_->Close();
    }

    if (nodeHostProcessWait_)
    {
        nodeHostProcessWait_->Cancel();
    }

    if (ipcHealthReportingClient_)
    {
        IpcHealthReportingClientObj.Abort();
    }

    if (throttledIpcHealthReportingClient_)
    {
        ThrottledIpcHealthReportingClientObj.Abort();
    }

    Client.Abort();

    if (nodeHostProcessClosed_.load())
    {
        // raise nodehost closed event, notify the exit handlers 
        RaiseNodeHostProcessClosedEvent();

        for(auto iter = removedRuntimes.cbegin(); iter != removedRuntimes.cend(); ++iter)
        {
            (*iter)->OnFabricProcessExited();
        }
    }

    if (logManager_)
    {
        logManager_->Abort();
        logManager_ = nullptr;
    }

    ShutdownKtlSystem(TimeSpan::FromMilliseconds(Constants::Ktl::AbortTimeoutMilliseconds));

    WriteNoise(
        TraceType,
        TraceId,
        "ApplicationHost Aborted");
}

// must not be called on a ktl thread
ErrorCode ApplicationHost::ShutdownKtlSystem(TimeSpan const & timeout)
{
    if (ktlSystemWrapper_)
    {
        ktlSystemWrapper_->Deactivate(timeout);
        ktlSystemWrapper_.reset();
    }

    return ErrorCodeValue::Success;
}

AsyncOperationSPtr ApplicationHost::BeginRegisterFabricRuntime(
    FabricRuntimeImplSPtr const & runtime,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<RegisterFabricRuntimeAsyncOperation>(
        *this,
        runtime,
        timeout,
        callback,
        parent);
}

ErrorCode ApplicationHost::EndRegisterFabricRuntime(
    AsyncOperationSPtr const & operation)
{
    return RegisterFabricRuntimeAsyncOperation::End(operation);
}

void ApplicationHost::UnregisterRuntimeAsync(wstring const & runtimeId)
{
    auto operation = AsyncOperation::CreateAndStart<UnregisterFabricRuntimeAsyncOperation>(
        *this,
        runtimeId,
        [this](AsyncOperationSPtr const & operation){ this->OnUnregisterRuntimeAsyncCompleted(operation); },
        this->CreateAsyncOperationRoot());
    if (operation->CompletedSynchronously)
    {
        FinishUnregisterRuntimeAsync(operation);
    }
}

void ApplicationHost::OnUnregisterRuntimeAsyncCompleted(AsyncOperationSPtr const & operation)
{
    if (!operation->CompletedSynchronously)
    {
        FinishUnregisterRuntimeAsync(operation);
    }
}

void ApplicationHost::FinishUnregisterRuntimeAsync(AsyncOperationSPtr const & operation)
{
    auto error = UnregisterFabricRuntimeAsyncOperation::End(operation);
    error.ReadValue();
}

void ApplicationHost::RegisterIpcMessageHandler()
{
    auto root = this->CreateComponentRoot();
    ipcClient_->RegisterMessageHandler(
        Actor::ApplicationHost,
        [this, root] (MessageUPtr & message, IpcReceiverContextUPtr & context)
    {
        this->IpcMessageHandler(*message, context);
    },
        false/*dispatchOnTransportThread*/);
}

void ApplicationHost::UnregisterIpcMessageHandler()
{
    ipcClient_->UnregisterMessageHandler(Actor::ApplicationHost);
}

void ApplicationHost::IpcMessageHandler(Message & message, IpcReceiverContextUPtr & context)
{
    WriteNoise(TraceType, TraceId, "Received message: actor={0}, action={1}", message.Actor, message.Action);

    if (message.Actor == Actor::ApplicationHost)
    {
        ProcessIpcMessage(message, context);
    }
    else
    {
        WriteWarning(TraceType, TraceId, "Unexpected message: actor={0}, action={1}", message.Actor, message.Action);
    }
}

void ApplicationHost::ProcessIpcMessage(Message & message, IpcReceiverContextUPtr & context)
{
    if (message.Action == Hosting2::Protocol::Actions::InvalidateLeaseRequest)
    {
        this->ProcessInvalidateLeaseRequest(message, context);
    }
    else if (message.Action == Hosting2::Protocol::Actions::UpdateCodePackageContextRequest)
    {
        this->ProcessUpdateCodePackageContextRequest(message, context);
    }
    else if (message.Action == Hosting2::Protocol::Actions::CodePackageEventNotification)
    {
        this->ProcessCodePackageEventRequest(message, context);
    }
}

void ApplicationHost::ProcessInvalidateLeaseRequest(Message &, IpcReceiverContextUPtr & context)
{
    ErrorCode result;
    if (leaseMonitor_)
    {
        leaseMonitor_->Close();
        result = ErrorCodeValue::Success;
    }
    else
    {
        WriteWarning(TraceType, TraceId, "Received InvalidateLeaseRequest, leaseMonitor_ should not be null");
        result = ErrorCodeValue::OperationFailed;
    }

    InvalidateLeaseReplyBody replyBody(result);
    MessageUPtr reply = make_unique<Message>(replyBody);
    WriteNoise(TraceType, TraceId, "Sending InvalidateLeaseReply. Message={0}, Body={1}", *reply, replyBody);
    context->Reply(move(reply));
}

void ApplicationHost::ProcessUpdateCodePackageContextRequest(Message & message, IpcReceiverContextUPtr & context)
{
    UpdateCodePackageContextRequest requestBody;
    if (!message.GetBody<UpdateCodePackageContextRequest>(requestBody))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType,
            TraceId,
            "GetBody<UpdateCodePackageContextRequest> failed: Message={0}, ErrorCode={1}",
            message,
            error);
        return;
    }

    WriteInfo(
        TraceType,
        TraceId,
        "Processing UpdateCodePackageContextRequest: CodePackageContext={0}, ActivationId={1}, HostContext={2}",
        requestBody.CodeContext,
        requestBody.ActivationId,
        HostContext);

    auto error = OnUpdateCodePackageContext(requestBody.CodeContext);

    if(error.IsSuccess())
    {
        // Update all affected runtimes
        error = runtimeTable_->VisitUnderLock([&](RuntimeTable::EntrySPtr const& runtime) -> bool
        {
            FabricRuntimeContext oldContext = runtime->RuntimeImpl->GetRuntimeContext();
            if(oldContext.CodeContext.CodePackageInstanceId == requestBody.CodeContext.CodePackageInstanceId)
            {
                // IsValid on runtime is not checked here. This just updates the
                // FabricRuntimeContext with the new CodeContext
                WriteNoise(
                    TraceType,
                    TraceId,
                    "Updating runtime with Id {0}",
                    runtime->RuntimeImpl->RuntimeId);

                FabricRuntimeContext newContext(
                    oldContext.RuntimeId,
                    oldContext.HostContext,
                    requestBody.CodeContext);
                runtime->RuntimeImpl->UpdateFabricRuntimeContext(newContext);
            }

            return true;
        });

        if(error.IsSuccess())
        {
            // Trigger change handlers for CodePackageContexts
            error = codePackageTable_->VisitUnderLock([&](CodePackageTable::EntrySPtr const& codePackage) -> bool
            {
                if (codePackage->ActivationContext->CodePackageInstanceId == requestBody.CodeContext.CodePackageInstanceId)
                {
                    if(!codePackage->IsValid)
                    {
                        error = ErrorCode(ErrorCodeValue::InvalidState);
                        return false;
                    }

                    WriteNoise(
                        TraceType,
                        TraceId,
                        "Updating CodePackageActivationContext {0}",
                        codePackage->ActivationContext->CodePackageInstanceId);
                    codePackage->ActivationContext->OnServicePackageChanged();
                }

                return true;
            });
        }
    }

    UpdateCodePackageContextReply replyBody(error);

    MessageUPtr reply = make_unique<Message>(replyBody);
    WriteNoise(TraceType, TraceId, "Sending UpdateCodePackageContextReply. Message={0}, Body={1}", *reply, replyBody);
    context->Reply(move(reply));
}

void ApplicationHost::ProcessCodePackageEventRequest(Message & message, IpcReceiverContextUPtr & context)
{
    CodePackageEventNotificationRequest requestBody;
    if (!message.GetBody<CodePackageEventNotificationRequest>(requestBody))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType,
            TraceId,
            "GetBody<CodePackageEventNotificationRequest> failed: Message={0}, ErrorCode={1}",
            message,
            error);
        return;
    }

    WriteNoise(
        TraceType,
        TraceId,
        "Processing {0}.",
        requestBody.CodePackageEvent);

    auto error = this->OnCodePackageEvent(requestBody.CodePackageEvent);

    CodePackageEventNotificationReply replyBody(error);
    auto reply = make_unique<Message>(move(replyBody));
    
    WriteNoise(TraceType, TraceId, "Sending {0}.", replyBody);
    
    context->Reply(move(reply));
}

ErrorCode ApplicationHost::InitializeNodeHostMonitoring(DWORD nodeHostProcessId)
{
    // There is no need to monitor Fabric.exe from an activated process as it goes down 
    // automatically being part of the job object that gets closed when Fabric.exe dies.

    // Additionally when this code is run inside a sandboxed process it does not have 
    // privileges to open handle to another process and synchronize on that

    if ((Type == ApplicationHostType::NonActivated) && (nodeHostProcessId != ::GetCurrentProcessId()))
    {
        HandleUPtr nodeHostProcessHandle;
        auto error = ProcessUtility::OpenProcess(
            SYNCHRONIZE, 
            FALSE, 
            nodeHostProcessId, 
            nodeHostProcessHandle);
        if (!error.IsSuccess()) { return error; }


        auto rootSPtr = CreateComponentRoot();
        this->nodeHostProcessWait_ = ProcessWait::CreateAndStart(
            Handle(*nodeHostProcessHandle,Handle::DUPLICATE()),
            nodeHostProcessId, 
            [this, rootSPtr](pid_t, ErrorCode const & waitResult, DWORD) { OnNodeHostProcessDown(waitResult); });

        WriteNoise(TraceType, TraceId, "NodeHost Monitoring is initialized.");
    }
    else
    {
        WriteNoise(TraceType, TraceId, "Skipping NodeHost Monitoring for ApplicationHost {0}:{1}.", Type, Id);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ApplicationHost::InitializeLeaseMonitoring(HANDLE leaseHandle, int64 initialLeaseExpiration, TimeSpan const leaseDuration)
{
#ifdef PLATFORM_UNIX
    // On linux we always use IPC for lease instead of direct call
    bool useDirectLease = false;
#else
    // In case of container host use IPC for lease
    bool useDirectLease = hostContext_.IsContainerHost ? false : true;
#endif
    leaseMonitor_ = make_unique<LeaseMonitor>(*this, TraceId, *ipcClient_, leaseHandle, initialLeaseExpiration, leaseDuration, useDirectLease);

    WriteNoise(
        TraceType,
        TraceId,
        "Initial lease status checking: {0}", 
        leaseMonitor_->IsLeaseExpired());

    return ErrorCodeValue::Success;
}

void ApplicationHost::OnNodeHostProcessDown(ErrorCode const & waitResult)
{
    if (!waitResult.IsSuccess())
    {
        WriteError(
            TraceType,
            TraceId,
            "NodeHostMonitorWait failed with {0}, however continuing as if NodeHost was closed.",
            waitResult);
    }

    // mark nodehost being down
    nodeHostProcessClosed_.store(true);

    // Abort the current host
    this->Abort();
}

NodeHostProcessClosedEventHHandler ApplicationHost::RegisterNodeHostProcessClosedEventHandler(NodeHostProcessClosedEventHandler const & handler)
{
    return this->nodeHostProcessClosedEvent_.Add(handler);
}

bool ApplicationHost::UnregisterNodeHostProcessClosedEventHandler(NodeHostProcessClosedEventHHandler const & hHandler)
{
    return this->nodeHostProcessClosedEvent_.Remove(hHandler);
}

void ApplicationHost::RaiseNodeHostProcessClosedEvent()
{
    wstring nodeId;
    {
        AcquireReadLock lock(this->nodeContextLock_);
        if(!nodeContext_)
        {
            WriteWarning(
                TraceType,
                TraceId,
                "Not Firing NodeHostProcessClosedEvent. ApplicationHost has not completed registration with Fabric.");
            return;
        }

        nodeId = nodeContext_->NodeId;
    }

    NodeHostProcessClosedEventArgs args(nodeId, this->Id);

    WriteNoise(
        TraceType,
        TraceId,
        "Firing NodeHostProcessClosedEvent: EventArgs={0}",
        args);

    this->nodeHostProcessClosedEvent_.Fire(args, false);
}

ErrorCode ApplicationHost::GetFabricNodeContext(FabricNodeContextSPtr & nodeContext)
{
    AcquireReadLock lock(this->nodeContextLock_);
    if(nodeContext_)
    {
        nodeContext = nodeContext_;
        return ErrorCodeValue::Success;
    }
    else
    {
        WriteWarning(
            TraceType, 
            TraceId, 
            "GetFabricNodeContext is not intialized yet. Returning with HostingApplicationHostNotRegistered.");
        return ErrorCodeValue::HostingApplicationHostNotRegistered;
    }
}
