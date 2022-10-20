// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

StringLiteral const TraceType("ComGuestServiceCodePackageActivationManager");

// {01FCD4A6-EE55-49DB-AECE-9C8963DDE0A7}
static const GUID CLSID_ActivateCodePackagesComAsyncOperationContext =
{ 0x1fcd4a6, 0xee55, 0x49db,{ 0xae, 0xce, 0x9c, 0x89, 0x63, 0xdd, 0xe0, 0xa7 } };

class ComGuestServiceCodePackageActivationManager::ActivateCodePackagesComAsyncOperationContext
    : public ComAsyncOperationContext
{
    DENY_COPY(ActivateCodePackagesComAsyncOperationContext)

        COM_INTERFACE_LIST2(
            ActivateCodePackagesComAsyncOperationContext,
            IID_IFabricAsyncOperationContext,
            IFabricAsyncOperationContext,
            CLSID_ActivateCodePackagesComAsyncOperationContext,
            ActivateCodePackagesComAsyncOperationContext)

public:
    explicit ActivateCodePackagesComAsyncOperationContext(
        _In_ ComGuestServiceCodePackageActivationManager & owner)
        : ComAsyncOperationContext(),
        owner_(owner),
        timeout_()
    {
    }

    virtual ~ActivateCodePackagesComAsyncOperationContext()
    {
    }

    HRESULT STDMETHODCALLTYPE Initialize(
        _In_ ComponentRootSPtr const & rootSPtr,
        _In_ FABRIC_STRING_MAP *environment,
        _In_ DWORD timeoutMilliseconds,
        _In_ IFabricAsyncOperationCallback * callback)
    {
        auto hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
        if (FAILED(hr)) { return hr; }

        if (environment != nullptr)
        {
            auto error = PublicApiHelper::FromPublicApiStringMap(*environment, environment_);
            if (!error.IsSuccess())
            {
                WriteNoise(
                    TraceType,
                    "ActivateCodePackagesAsyncOperation: FromPublicApiStringMap() failed. Error={0}, CPCtx={1}, AppHostCtx={2}.",
                    owner_.typeHost_.CodeContext,
                    owner_.typeHost_.HostContext);

                return error.ToHResult();
            }
        }

        timeout_ = owner_.GetTimeout(timeoutMilliseconds);

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<ActivateCodePackagesComAsyncOperationContext> thisOperation(
            context, CLSID_ActivateCodePackagesComAsyncOperationContext);
        return thisOperation->Result;
    }

protected:
    void OnStart(_In_ AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = owner_.BeginActivateCodePackagesAndWaitForEvent(
            move(environment_),
            timeout_,
            [this](AsyncOperationSPtr const & operation)
            {
                this->FinishActivateCodePackagesAndWaitForEvent(operation, false);
            },
            proxySPtr);

        this->FinishActivateCodePackagesAndWaitForEvent(operation, true);
    }

private:
    void FinishActivateCodePackagesAndWaitForEvent(
        AsyncOperationSPtr const & operation, bool expectCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.EndActivateCodePackagesAndWaitForEvent(operation);

        this->TryComplete(operation->Parent, error.ToHResult());
    }

private:
    ComGuestServiceCodePackageActivationManager & owner_;
    EnvironmentMap environment_;
    TimeSpan timeout_;
};

// {42994658-2D0D-410A-958C-99DF9C69DB8A}
static const GUID CLSID_DeactivateCodePackagesComAsyncOperationContext =
{ 0x42994658, 0x2d0d, 0x410a,{ 0x95, 0x8c, 0x99, 0xdf, 0x9c, 0x69, 0xdb, 0x8a } };

class ComGuestServiceCodePackageActivationManager::DeactivateCodePackagesComAsyncOperationContext
    : public ComAsyncOperationContext
{
    DENY_COPY(DeactivateCodePackagesComAsyncOperationContext)

        COM_INTERFACE_LIST2(
            DeactivateCodePackagesComAsyncOperationContext,
            IID_IFabricAsyncOperationContext,
            IFabricAsyncOperationContext,
            CLSID_DeactivateCodePackagesComAsyncOperationContext,
            DeactivateCodePackagesComAsyncOperationContext)

public:
    explicit DeactivateCodePackagesComAsyncOperationContext(
        _In_ ComGuestServiceCodePackageActivationManager & owner)
        : ComAsyncOperationContext(),
        owner_(owner),
        timeout_()
    {
    }

    virtual ~DeactivateCodePackagesComAsyncOperationContext()
    {
    }

    HRESULT STDMETHODCALLTYPE Initialize(
        _In_ ComponentRootSPtr const & rootSPtr,
        _In_ DWORD timeoutMilliseconds,
        _In_ IFabricAsyncOperationCallback * callback)
    {
        auto hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
        if (FAILED(hr)) { return hr; }

        timeout_ = owner_.GetTimeout(timeoutMilliseconds);

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<DeactivateCodePackagesComAsyncOperationContext> thisOperation(
            context, CLSID_DeactivateCodePackagesComAsyncOperationContext);
        return thisOperation->Result;
    }

protected:
    void OnStart(_In_ AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = owner_.BeginDeactivateCodePackages(
            timeout_,
            [this](AsyncOperationSPtr const & operation)
            {
                this->FinishDeactivateCodePackagesAndWaitForEvent(operation, false);
            },
            proxySPtr);

        this->FinishDeactivateCodePackagesAndWaitForEvent(operation, true);
    }

private:
    void FinishDeactivateCodePackagesAndWaitForEvent(
        AsyncOperationSPtr const & operation, bool expectCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.EndDeactivateCodePackages(operation);

        this->TryComplete(operation->Parent, error.ToHResult());
    }

private:
    ComGuestServiceCodePackageActivationManager & owner_;
    TimeSpan timeout_;
};

class ComGuestServiceCodePackageActivationManager::ActivateCodePackagesAsyncOperation
    : public ComProxyAsyncOperation
{
    DENY_COPY(ActivateCodePackagesAsyncOperation)

public:
    ActivateCodePackagesAsyncOperation(
        ComGuestServiceCodePackageActivationManager & owner,
        EnvironmentMap && environment,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , owner_(owner)
        , environment_(move(environment))
        , timeout_(timeout)
    {
    }

    virtual ~ActivateCodePackagesAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ActivateCodePackagesAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(
        IFabricAsyncOperationCallback * callback, 
        IFabricAsyncOperationContext ** context)
    {
        FABRIC_STRING_LIST codePackageNames = {};
        codePackageNames.Count = 0;
        codePackageNames.Items = nullptr;

        ScopedHeap heap;
        FABRIC_STRING_MAP environment = {};
        auto error = PublicApiHelper::ToPublicApiStringMap(heap, environment_, environment);
        if(!error.IsSuccess())
        {
            WriteNoise(
                TraceType,
                "ActivateCodePackagesAsyncOperation: ToPublicApiStringMap() failed. Error={0}, CPCtx={1}, AppHostCtx={2}.",
                owner_.typeHost_.CodeContext,
                owner_.typeHost_.HostContext);

            return error.ToHResult();
        }

        return owner_.activator_->BeginActivateCodePackage(
            &codePackageNames,
            &environment,
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return owner_.activator_->EndActivateCodePackage(context);
    }

private:
    ComGuestServiceCodePackageActivationManager & owner_;
    EnvironmentMap environment_;
    TimeSpan timeout_;
};

class ComGuestServiceCodePackageActivationManager::DeactivateCodePackagesAsyncOperation
    : public ComProxyAsyncOperation
{
    DENY_COPY(DeactivateCodePackagesAsyncOperation)

public:
    DeactivateCodePackagesAsyncOperation(
        ComGuestServiceCodePackageActivationManager & owner,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , owner_(owner)
        , timeout_(timeout)
    {
    }

    virtual ~DeactivateCodePackagesAsyncOperation() 
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DeactivateCodePackagesAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(
        IFabricAsyncOperationCallback * callback,
        IFabricAsyncOperationContext ** context)
    {
        FABRIC_STRING_LIST codePackageNames = {};
        codePackageNames.Count = 0;
        codePackageNames.Items = nullptr;

        return owner_.activator_->BeginDeactivateCodePackage(
            &codePackageNames,
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return owner_.activator_->EndDeactivateCodePackage(context);
    }

private:
    ComGuestServiceCodePackageActivationManager & owner_;
    TimeSpan timeout_;
};

class ComGuestServiceCodePackageActivationManager::EventWaitAsyncOperation 
    : public TimedAsyncOperation
{
    DENY_COPY(EventWaitAsyncOperation)

public:
    EventWaitAsyncOperation(
        __in ComGuestServiceCodePackageActivationManager & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , owner_(owner)
        , waiterId_(Guid::NewGuid().ToString())
    {
    }

    virtual ~EventWaitAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<EventWaitAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto shouldComplete = false;

        {
            AcquireWriteLock writeLock(owner_.eventLock_);

            if (owner_.eventReceivedCount_ == owner_.codePackageEventTracker_.size())
            {
                shouldComplete = true;
            }
            else
            {
                owner_.eventWaiters_.insert(make_pair(waiterId_, thisSPtr));
            }
        }

        //
        // Complete the operation outside the lock to avoid deadlock.
        //
        if (shouldComplete)
        {
            this->TryComplete(thisSPtr, ErrorCode::Success());
            return;
        }

        TimedAsyncOperation::OnStart(thisSPtr);
    }

    void OnCompleted()
    {
        TimedAsyncOperation::OnCompleted();

        {
            AcquireWriteLock lock(owner_.eventLock_);

            auto iter = owner_.eventWaiters_.find(waiterId_);
            if (iter != owner_.eventWaiters_.end())
            {
                owner_.eventWaiters_.erase(iter);
            }
        }
    }

private:
    ComGuestServiceCodePackageActivationManager & owner_;
    wstring waiterId_;
};

class ComGuestServiceCodePackageActivationManager::ActivateCodePackagesAndWaitForEventAsyncOperation
    : public AsyncOperation
{
    DENY_COPY(ActivateCodePackagesAndWaitForEventAsyncOperation)

public:
    ActivateCodePackagesAndWaitForEventAsyncOperation(
        __in ComGuestServiceCodePackageActivationManager & owner,
        EnvironmentMap && environment,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , environment_(move(environment))
        , timeoutHelper_(timeout)
    {
    }

    virtual ~ActivateCodePackagesAndWaitForEventAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ActivateCodePackagesAndWaitForEventAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (this->IsAlreadyActive())
        {
            WriteNoise(
                TraceType,
                "ActivateCodePackagesAndWaitForEventAsyncOperation: CPs already active. CPContext={0}, AppHostContext={1}.",
                owner_.typeHost_.CodeContext,
                owner_.typeHost_.HostContext);

            this->TryComplete(thisSPtr, ErrorCode::Success());
            return;
        }

        auto operation = AsyncOperation::CreateAndStart<ActivateCodePackagesAsyncOperation>(
            owner_,
            move(environment_),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
            {
                this->FinishActivateCodePackages(operation, false);
            },
            thisSPtr);

        this->FinishActivateCodePackages(operation, true);
    }

private:
    void FinishActivateCodePackages(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = ActivateCodePackagesAsyncOperation::End(operation);

        WriteTrace(
            error.ToLogLevel(LogLevel::Warning, LogLevel::Info),
            TraceType,
            "ActivateCodePackagesAndWaitForEventAsyncOperation(FinishActivateCodePackages): Error={0}, CPContext={1}, AppHostContext={2}.",
            error,
            owner_.typeHost_.CodeContext,
            owner_.typeHost_.HostContext);

        if (!error.IsSuccess())
        {
            this->TryComplete(operation->Parent, error);
            return;
        }

        auto eventWaitOperation = AsyncOperation::CreateAndStart<EventWaitAsyncOperation>(
            owner_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
            {
                this->FinishWaitForEvent(operation, false);
            },
            operation->Parent);

        this->FinishWaitForEvent(eventWaitOperation, true);
    }

    void FinishWaitForEvent(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = EventWaitAsyncOperation::End(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            "ActivateCodePackagesAndWaitForEventAsyncOperation(FinishWaitForEvent): Error={0}, CPContext={1}, AppHostContext={2}.",
            error,
            owner_.typeHost_.CodeContext,
            owner_.typeHost_.HostContext);

        this->TryComplete(operation->Parent, error);
    }

    bool IsAlreadyActive()
    {
        AcquireReadLock readLock(owner_.eventLock_);

        return (owner_.eventReceivedCount_ == owner_.codePackageEventTracker_.size());
    }

private:
    ComGuestServiceCodePackageActivationManager & owner_;
    EnvironmentMap environment_;
    TimeoutHelper timeoutHelper_;
};

ComGuestServiceCodePackageActivationManager::ComGuestServiceCodePackageActivationManager(
    IGuestServiceTypeHost & typeHost)
    : typeHost_(typeHost)
    , isClosed_(false)
    , eventHandlerHanlde_()
    , serviceRegistrationLock_()
    , serviceRegistrationIdTracker_()
    , registeredServiceInstanceMap_()
    , registeredServiceReplicaMap_()
    , eventReceivedCount_(0)
    , eventLock_()
    , codePackageEventTracker_()
{
    for (auto const & cpName : typeHost_.DependentCodePackages)
    {
        codePackageEventTracker_.insert(
            make_pair(
                cpName,
                make_pair(false, 0)));
    }
}

ComGuestServiceCodePackageActivationManager::~ComGuestServiceCodePackageActivationManager()
{
    activator_->UnregisterCodePackageEventHandler(eventHandlerHanlde_);
}

ErrorCode ComGuestServiceCodePackageActivationManager::Open()
{
    auto error = typeHost_.GetCodePackageActivator(activator_);

    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        "Open: GetCodePackageActivator() - CPContext={0}, AppHostContext={1}, Error={2}.",
        typeHost_.CodeContext,
        typeHost_.HostContext,
        error);

    ComPointer<IFabricCodePackageEventHandler> eventHandler;
    eventHandler.SetAndAddRef(this);

    auto hr = activator_->RegisterCodePackageEventHandler(eventHandler.GetRawPointer(), &eventHandlerHanlde_);
    error = ErrorCode::FromHResult(hr);

    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        "Open: RegisterCodePackageEventHandler() - CPContext={0}, AppHostContext={1}, Error={2}.",
        typeHost_.CodeContext,
        typeHost_.HostContext,
        error);

    return error;
}

void ComGuestServiceCodePackageActivationManager::Close()
{
    isClosed_.store(true);

    if (activator_)
    {
        activator_->UnregisterCodePackageEventHandler(eventHandlerHanlde_);
    }

    {
        AcquireReadLock writeLock(serviceRegistrationLock_);
        registeredServiceInstanceMap_.clear();
        registeredServiceReplicaMap_.clear();
    }
    
    vector<AsyncOperationSPtr> waitersToCancel;
    {
        AcquireReadLock readLock(eventLock_);

        for (auto waiter : eventWaiters_)
        {
            waitersToCancel.push_back(waiter.second);
        }
    }

    for (auto waiter : waitersToCancel)
    {
        waiter->TryComplete(waiter, ErrorCode(ErrorCodeValue::ObjectClosed));
    }
}

HRESULT ComGuestServiceCodePackageActivationManager::BeginActivateCodePackagesAndWaitForEvent(
    /* [in] */ FABRIC_STRING_MAP *environment,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    ComPointer<IUnknown> rootCPtr;
    auto hr = this->QueryInterface(IID_IUnknown, (void**)rootCPtr.InitializationAddress());
    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr); 
    }

    auto root = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));

    auto operation = make_com<ActivateCodePackagesComAsyncOperationContext>(*this);
    hr = operation->Initialize(root, environment, timeoutMilliseconds, callback);
    if (FAILED(hr))
    { 
        return ComUtility::OnPublicApiReturn(hr); 
    }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComGuestServiceCodePackageActivationManager::EndActivateCodePackagesAndWaitForEvent(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(ActivateCodePackagesComAsyncOperationContext::End(context));
}

HRESULT ComGuestServiceCodePackageActivationManager::BeginDeactivateCodePackages(
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    ComPointer<IUnknown> rootCPtr;
    auto hr = this->QueryInterface(IID_IUnknown, (void**)rootCPtr.InitializationAddress());
    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }

    auto root = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));

    auto operation = make_com<DeactivateCodePackagesComAsyncOperationContext>(*this);
    hr = operation->Initialize(root, timeoutMilliseconds, callback);
    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComGuestServiceCodePackageActivationManager::EndDeactivateCodePackages(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(DeactivateCodePackagesComAsyncOperationContext::End(context));
}

AsyncOperationSPtr ComGuestServiceCodePackageActivationManager::BeginActivateCodePackagesAndWaitForEvent(
    EnvironmentMap && environment,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ActivateCodePackagesAndWaitForEventAsyncOperation>(
        *this,
        move(environment),
        timeout,
        callback,
        parent);
}

ErrorCode ComGuestServiceCodePackageActivationManager::EndActivateCodePackagesAndWaitForEvent(
    AsyncOperationSPtr const & operation)
{
    return ActivateCodePackagesAndWaitForEventAsyncOperation::End(operation);
}

AsyncOperationSPtr ComGuestServiceCodePackageActivationManager::BeginDeactivateCodePackages(
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<DeactivateCodePackagesAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode ComGuestServiceCodePackageActivationManager::EndDeactivateCodePackages(
    AsyncOperationSPtr const & operation)
{
    return DeactivateCodePackagesAsyncOperation::End(operation);
}

void ComGuestServiceCodePackageActivationManager::AbortCodePackages()
{
    FABRIC_STRING_LIST codePackageNames = {};
    activator_->AbortCodePackage(&codePackageNames);
}

ErrorCode ComGuestServiceCodePackageActivationManager::RegisterServiceInstance(
    ComPointer<IGuestServiceInstance> const & serviceInstance,
    _Out_ ULONGLONG & registrationHandle)
{
    AcquireWriteLock writeLock(serviceRegistrationLock_);

    if (isClosed_.load())
    {
        return ErrorCode(ErrorCodeValue::ObjectClosed);
    }

    registrationHandle = ++serviceRegistrationIdTracker_;
    registeredServiceInstanceMap_.insert(make_pair(registrationHandle, serviceInstance));

    return ErrorCode::Success();
}

ErrorCode ComGuestServiceCodePackageActivationManager::RegisterServiceReplica(
    ComPointer<IGuestServiceReplica> const & serviceReplica,
    _Out_ ULONGLONG & registrationHandle)
{
    AcquireWriteLock writeLock(serviceRegistrationLock_);

    if (isClosed_.load())
    {
        return ErrorCode(ErrorCodeValue::ObjectClosed);
    }

    registrationHandle = ++serviceRegistrationIdTracker_;
    registeredServiceReplicaMap_.insert(make_pair(registrationHandle, serviceReplica));

    return ErrorCode::Success();
}

void ComGuestServiceCodePackageActivationManager::UnregisterServiceReplicaOrInstance(
    ULONGLONG serviceRegistrationhandle)
{
    AcquireWriteLock writeLock(serviceRegistrationLock_);

    if (isClosed_.load())
    {
        return;
    }

    auto removed = registeredServiceInstanceMap_.erase(serviceRegistrationhandle);
    if (removed == 0)
    {
        registeredServiceReplicaMap_.erase(serviceRegistrationhandle);
    }
}

void ComGuestServiceCodePackageActivationManager::OnCodePackageEvent(
    /* [in] */ IFabricCodePackageActivator *source,
    /* [in] */ const FABRIC_CODE_PACKAGE_EVENT_DESCRIPTION *eventDesc)
{
    UNREFERENCED_PARAMETER(source);

    CodePackageEventDescription internalEventDesc;
    internalEventDesc.FromPublicApi(*eventDesc);

    WriteInfo(
        TraceType,
        "OnCodePackageEvent(): CPContext={0}, AppHostContext={1}, Event={2}.",
        typeHost_.CodeContext,
        typeHost_.HostContext,
        internalEventDesc);

    if (
        internalEventDesc.EventType == CodePackageEventType::Started ||
        internalEventDesc.EventType == CodePackageEventType::Terminated ||
        internalEventDesc.EventType == CodePackageEventType::Stopped)
    {
        this->TrackCodePackage(move(internalEventDesc));
    }
}

bool ComGuestServiceCodePackageActivationManager::IsFailureCountExceeded(
    CodePackageEventDescription const & eventDesc)
{
    auto iter = eventDesc.Properties.find(CodePackageEventProperties::FailureCount);
    if (iter != eventDesc.Properties.end())
    {
        ULONG failureCount;
        if (StringUtility::TryFromWString<ULONG>(iter->second, failureCount))
        {
            auto failureCountThreshold = (ULONG)HostingConfig::GetConfig().DeployedServiceFailoverContinuousFailureThreshold;
            return (failureCount >= failureCountThreshold);
        }

        TRACE_ERROR_AND_ASSERT(
            TraceType,
            "IsFailureCountExceeded() : Failed to parse failure count as ULONG. Event={0}.",
            eventDesc);
    }

    WriteWarning(
        TraceType,
        "IsFailureCountExceeded(): FailureCount property is missing. CPContext={0}, AppHostContext={1}, Event={2}.",
        typeHost_.CodeContext,
        typeHost_.HostContext,
        eventDesc);

    //
    // Ideally we should always have FailureCount property set. If it is not then
    // return true so that replica is transient faulted and failover is triggered.
    //
    return true;
}

void ComGuestServiceCodePackageActivationManager::NotifyServices(CodePackageEventDescription && eventDesc)
{
    vector<ComPointer<IGuestServiceInstance>> instances;
    vector<ComPointer<IGuestServiceReplica>> replicas;

    {
        AcquireWriteLock readLock(serviceRegistrationLock_);

        if (isClosed_.load())
        {
            return;
        }

        //
        // Notify replica to fault itself only when failure limit is exceeded.
        //
        if (this->IsFailureCountExceeded(eventDesc) == false)
        {
            WriteInfo(
                TraceType,
                "NotifyServices(): Skipping notifying replica. CPContext={0}, AppHostContext={1}, Event={2}, FailoverContinuousFailureThreshold={3}.",
                typeHost_.CodeContext,
                typeHost_.HostContext,
                eventDesc,
                HostingConfig::GetConfig().DeployedServiceFailoverContinuousFailureThreshold);

            return;
        }

        for (auto kvPair : registeredServiceInstanceMap_)
        {
            instances.push_back(kvPair.second);
        }

        for (auto kvPair : registeredServiceReplicaMap_)
        {
            replicas.push_back(kvPair.second);
        }
    }

    //
    // Invoke callback outside lock so that if replica/instance try
    // to unregister from inside the callback, there is no deadlock.
    //
    for (auto instance : instances)
    {
        instance->OnActivatedCodePackageTerminated(move(eventDesc));
    }

    for (auto replica : replicas)
    {
        replica->OnActivatedCodePackageTerminated(move(eventDesc));
    }
}

void ComGuestServiceCodePackageActivationManager::TrackCodePackage(CodePackageEventDescription && eventDesc)
{
    vector<AsyncOperationSPtr> waitersToComplete;

    {
        AcquireWriteLock writeLock(eventLock_);

        if (isClosed_.load())
        {
            return;
        }

        auto iter = codePackageEventTracker_.find(eventDesc.CodePackageName);

        ASSERT_IF(
            iter == codePackageEventTracker_.end(), 
            "TrackCodePackage: Received unexpected event {0}. HostContext={1}, CodePackageContext={2}.",
            eventDesc,
            typeHost_.HostContext,
            typeHost_.CodeContext);

        if (eventDesc.SquenceNumber < iter->second.second)
        {
            WriteNoise(
                TraceType,
                "TrackCodePackage: Received stale event {0}. CurrentSequenceNumber={1}, HostContext={2}, CodePackageContext={3}.",
                eventDesc,
                iter->second.second,
                typeHost_.HostContext,
                typeHost_.CodeContext);
        }

        if (eventDesc.EventType == CodePackageEventType::Stopped ||
            eventDesc.EventType == CodePackageEventType::Terminated)
        {
            if (iter->second.first == true)
            {
                iter->second.first = false;
                iter->second.second = eventDesc.SquenceNumber;
                --eventReceivedCount_;
            }

            if (eventDesc.EventType == CodePackageEventType::Terminated)
            {
                this->NotifyServices(move(eventDesc));
            }

            return;
        }

        // We are here means this is Started event.
        if (iter->second.first == false)
        {
            iter->second.first = true;
            iter->second.second = eventDesc.SquenceNumber;
            ++eventReceivedCount_;
        }

        if (eventReceivedCount_ == codePackageEventTracker_.size())
        {
            for (auto waiter : eventWaiters_)
            {
                waitersToComplete.push_back(waiter.second);
            }
        }
    }

    //
    // Complete the waiters outside the event lock and these operations
    // remove themselves from event waiter map on completion and need
    // to acquire the event lock.
    //
    for (auto waiter : waitersToComplete)
    {
        waiter->TryComplete(waiter, ErrorCode::Success());
    }
}

TimeSpan ComGuestServiceCodePackageActivationManager::GetTimeout(
    DWORD timeoutMilliseconds)
{
    if (timeoutMilliseconds == INFINITE)
    {
        return TimeSpan::MaxValue;
    }
    
    if (timeoutMilliseconds == 0)
    {
        return HostingConfig::GetConfig().ActivationTimeout;
    }

    return TimeSpan::FromMilliseconds(static_cast<double>(timeoutMilliseconds));
}

void ComGuestServiceCodePackageActivationManager::Test_GetCodePackageEventTracker(
    CodePackageEventTracker & eventTracker)
{
    AcquireWriteLock readLock(eventLock_);
    eventTracker = codePackageEventTracker_;
}

