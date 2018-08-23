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

StringLiteral const TraceType("GuestServiceTypeHostManager");

// ********************************************************************************************************************
// GuestServiceTypeHostManager::GuestServiceTypeHostMap Implementation
//
class GuestServiceTypeHostManager::GuestServiceTypeHostMap
{
    DENY_COPY(GuestServiceTypeHostMap)

public:
    GuestServiceTypeHostMap()
        : lock_()
        , closed_(false)
        , map_()
    {
    }

    ~GuestServiceTypeHostMap()
    {
    }

    vector<GuestServiceTypeHostSPtr> Close()
    {
        vector<GuestServiceTypeHostSPtr> retval;

        {
            AcquireWriteLock writelock(lock_);
            if (!closed_)
            {
                for (auto const & entry : map_)
                {
                    retval.push_back(move(entry.second));
                }

                map_.clear();
                closed_ = true;
            }
        }

        return move(retval);
    }
    
    ErrorCode Add(GuestServiceTypeHostSPtr const & guestServiceTypeHost)
    {
        AcquireWriteLock writeLock(lock_);

        if (closed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(guestServiceTypeHost->HostId);
        if (iter != map_.end())
        {
            return ErrorCode(ErrorCodeValue::AlreadyExists);
        }

        map_.insert(make_pair(guestServiceTypeHost->HostId, guestServiceTypeHost));

        return ErrorCode(ErrorCodeValue::Success);
    }

    ErrorCode Remove(wstring const & hostId, __out GuestServiceTypeHostSPtr & guestServiceTypeHost)
    {
        AcquireWriteLock writelock(lock_);

        if (closed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(hostId);
        if (iter == map_.end())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        guestServiceTypeHost = iter->second;
        map_.erase(iter);

        return ErrorCode(ErrorCodeValue::Success);
    }

    ErrorCode Find(wstring const & hostId, __out GuestServiceTypeHostSPtr & guestServiceTypeHost) const
    {
        AcquireReadLock readlock(lock_);

        if (closed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(hostId);
        if (iter == map_.end())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        guestServiceTypeHost = iter->second;

        return ErrorCode(ErrorCodeValue::Success);
    }

    ErrorCode Count(__out size_t & count)
    {
        AcquireReadLock readlock(lock_);

        if (closed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        count = map_.size();

        return ErrorCode(ErrorCodeValue::Success);
    }

    vector<GuestServiceTypeHostSPtr> GetServiceTypeHosts()
    {
        vector<GuestServiceTypeHostSPtr> retval;

        {
            AcquireReadLock readlock(lock_);
            if (!closed_)
            {
                for (auto const & entry : map_)
                {
                    retval.push_back(move(entry.second));
                }
            }
        }

        return move(retval);
    }

private:
    mutable RwLock lock_;
    bool closed_;
    map<wstring, GuestServiceTypeHostSPtr, IsLessCaseInsensitiveComparer<wstring>> map_;
};

// ********************************************************************************************************************
// GuestServiceTypeHostManager::CloseAsyncOperation Implementation
//
class GuestServiceTypeHostManager::CloseAsyncOperation
    : public AsyncOperation
    , TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(CloseAsyncOperation)

public:
    CloseAsyncOperation(
        GuestServiceTypeHostManager & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , timeoutHelper_(timeout)
        , activatedTypeHostClosedCount_(0)
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
            "Closing GuestServiceTypeHostManager: Timeout={0}",
            timeoutHelper_.GetRemainingTime());

        CloseActivatedGuestServiceTypeHosts(thisSPtr);
    }

    void CloseActivatedGuestServiceTypeHosts(AsyncOperationSPtr const & thisSPtr)
    {
        auto activatedTypeHosts = owner_.typeHostMap_->Close();

        activatedTypeHostClosedCount_.store(activatedTypeHosts.size());

        WriteNoise(
            TraceType,
            owner_.Root.TraceId,
            "Closing {0} Activated GuestServiceTypeHosts.",
            activatedTypeHosts.size());

        if (activatedTypeHosts.empty())
        {
            CheckIfAllActivatedTypeHostClosed(thisSPtr, activatedTypeHosts.size());
        }
        else
        {
            for (auto & typeHost : activatedTypeHosts)
            {
                CloseActivatedGuestServiceTypeHost(thisSPtr, typeHost, timeoutHelper_.GetRemainingTime());
            }
        }
    }

    void CloseActivatedGuestServiceTypeHost(AsyncOperationSPtr const & thisSPtr, GuestServiceTypeHostSPtr & typeHost, TimeSpan const timeout)
    {
        auto operation = typeHost->BeginClose(
            timeout,
            [this, typeHost](AsyncOperationSPtr const & operation) mutable
            {
                this->FinishCloseActivatedApplicationHost(typeHost, operation, false);
            },
            thisSPtr);

        FinishCloseActivatedApplicationHost(typeHost, operation, true);
    }

    void FinishCloseActivatedApplicationHost(GuestServiceTypeHostSPtr & typeHost, AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = typeHost->EndClose(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "End(CloseGuestServiceTypeHostS): ErrorCode={0},  Id={1}.",
            error,
            typeHost->HostId);

        CheckIfAllActivatedTypeHostClosed(operation->Parent, --activatedTypeHostClosedCount_);
    }

    void CheckIfAllActivatedTypeHostClosed(AsyncOperationSPtr const & thisSPtr, uint64 count)
    {
        if (count == 0)
        {
            WriteNoise(
                TraceType,
                owner_.Root.TraceId,
                "Closed all activated GuestServiceTypeHost Hosts.");

            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
        }
    }

private:
    GuestServiceTypeHostManager & owner_;
    TimeoutHelper timeoutHelper_;
    atomic_uint64 activatedTypeHostClosedCount_;
};

// ********************************************************************************************************************
// GuestServiceTypeHostManager::OpenGuestServiceTypeHostAsyncOperation Implementation
//
class GuestServiceTypeHostManager::OpenGuestServiceTypeHostAsyncOperation
    : public AsyncOperation
    , TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(OpenGuestServiceTypeHostAsyncOperation)

public:
    OpenGuestServiceTypeHostAsyncOperation(
        GuestServiceTypeHostManager & owner,
        ApplicationHostContext const & appHostContext,
        vector<GuestServiceTypeInfo> const & typesToHost,
        CodePackageContext const & codePackageContext,
        vector<wstring> && depdendentCodePackages,
        vector<EndpointDescription> && endpointDescriptions,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , appHostContext_(appHostContext)
        , typesToHost_(typesToHost)
        , codePackageContext_(codePackageContext)
        , depdendentCodePackages_(move(depdendentCodePackages))
        , endpointDescriptions_(move(endpointDescriptions))
        , timeoutHelper_(timeout)
        , typeHostSPtr_()
    {
    }

    virtual ~OpenGuestServiceTypeHostAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<OpenGuestServiceTypeHostAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType,
            owner_.Root.TraceId,
            "Start OpenGuestServiceTypeHostAsyncOperation: HostContext={0}, CodePackageContext={1}, Timeout={2}",
            appHostContext_,
            codePackageContext_,
            timeoutHelper_.GetRemainingTime());

        CreateGuestServiceTypeHost(thisSPtr);
    }

    void CreateGuestServiceTypeHost(AsyncOperationSPtr const & thisSPtr)
    {
        auto typeHost = make_shared<GuestServiceTypeHost>(
            HostingSubsystemHolder(
                owner_.hosting_, 
                owner_.hosting_.Root.CreateComponentRoot()),
            appHostContext_,
            typesToHost_,
            codePackageContext_,
            owner_.hosting_.RuntimeServiceAddress,
            move(depdendentCodePackages_),
            move(endpointDescriptions_));

        typeHostSPtr_ = move(typeHost);

        auto error = owner_.typeHostMap_->Add(typeHostSPtr_);

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "Failed to add GuestServiceTypeHost to map: HostContext={0}, CodePackageContext={1}, Timeout={2}",
                appHostContext_,
                codePackageContext_,
                timeoutHelper_.GetRemainingTime());

            TryComplete(thisSPtr, error);
            return;
        }

        // Since this is in-proc, post opening on different thread.
        Threadpool::Post([this, thisSPtr]() { this->OpenGuestServiceTypeHost(thisSPtr); });
    }

    void OpenGuestServiceTypeHost(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = typeHostSPtr_->BeginOpen(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnGuestServiceTypeHostOpened(operation, false); },
            thisSPtr);

        OnGuestServiceTypeHostOpened(operation, true);
    }

    void OnGuestServiceTypeHostOpened(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = typeHostSPtr_->EndOpen(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "End OpenGuestServiceTypeHostAsyncOperation: HostContext={0}, CodePackageContext={1}, Timeout={2}, Error={3}.",
            appHostContext_,
            codePackageContext_,
            timeoutHelper_.GetRemainingTime(),
            error);

        if (!error.IsSuccess())
        {
            GuestServiceTypeHostSPtr removed;
            owner_.typeHostMap_->Remove(typeHostSPtr_->HostId, removed).ReadValue();
        }

        TryComplete(operation->Parent, error);
        return;
    }

private:
    GuestServiceTypeHostManager & owner_;
    ApplicationHostContext const & appHostContext_;
    vector<GuestServiceTypeInfo> typesToHost_;
    CodePackageContext codePackageContext_;
    vector<wstring> depdendentCodePackages_;
    vector<EndpointDescription> endpointDescriptions_;
    TimeoutHelper timeoutHelper_;
    GuestServiceTypeHostSPtr typeHostSPtr_;
};

// ********************************************************************************************************************
// GuestServiceTypeHostManager::CloseGuestServiceTypeHostAsyncOperation Implementation
//
class GuestServiceTypeHostManager::CloseGuestServiceTypeHostAsyncOperation
    : public AsyncOperation
    , TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(CloseGuestServiceTypeHostAsyncOperation)

public:
    CloseGuestServiceTypeHostAsyncOperation(
        GuestServiceTypeHostManager & owner,
        ApplicationHostContext const & appHostContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , appHostContext_(appHostContext)
        , timeoutHelper_(timeout)
        , typeHostToCloseSPtr_()
    {
    }

    virtual ~CloseGuestServiceTypeHostAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CloseGuestServiceTypeHostAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType,
            owner_.Root.TraceId,
            "Start CloseGuestServiceTypeHostAsyncOperation: HostContext={0}, Timeout={2}",
            appHostContext_,
            timeoutHelper_.GetRemainingTime());

        FindGuestServiceTypeHost(thisSPtr);
    }

    void FindGuestServiceTypeHost(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.typeHostMap_->Find(appHostContext_.HostId, typeHostToCloseSPtr_);

        if (error.IsError(ErrorCodeValue::NotFound))
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "Failed to find GuestServiceTypeHost for HostContext={0}. Completing stop operation.",
                appHostContext_);

            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "Failed to find ApplicationHostProxy for HostContext={0}. ErrorCode={1}.",
                appHostContext_,
                error);

            TryComplete(thisSPtr, error);
            return;
        }

        // Since this is in-proc, post on different thread.
        Threadpool::Post([this, thisSPtr]() { this->CloseGuestServiceTypeHost(thisSPtr); });
    }

    void CloseGuestServiceTypeHost(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = typeHostToCloseSPtr_->BeginClose(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnGuestServiceTypeHostClosed(operation, false); },
            thisSPtr);

        OnGuestServiceTypeHostClosed(operation, true);
    }

    void OnGuestServiceTypeHostClosed(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = typeHostToCloseSPtr_->EndClose(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "End CloseGuestServiceTypeHostAsyncOperation: HostContext={0}, Timeout={1}, Error={2}.",
            appHostContext_,
            timeoutHelper_.GetRemainingTime(),
            error);

        GuestServiceTypeHostSPtr removed;
        owner_.typeHostMap_->Remove(appHostContext_.HostId, removed).ReadValue();

        if (error.IsSuccess())
        {
            owner_.NotifyAppHostHostManager(appHostContext_.HostId, ProcessActivator::ProcessDeactivateExitCode);
        }

        TryComplete(operation->Parent, error);
        return;
    }

private:
    GuestServiceTypeHostManager & owner_;
    ApplicationHostContext const & appHostContext_;
    TimeoutHelper timeoutHelper_;
    GuestServiceTypeHostSPtr typeHostToCloseSPtr_;
};

// ********************************************************************************************************************
// GuestServiceTypeHostManager Implementation
//
GuestServiceTypeHostManager::GuestServiceTypeHostManager(
    ComponentRoot const & root,
    __in HostingSubsystem & hosting)
    : RootedObject(root)
    , hosting_(hosting)
    , typeHostMap_()
{
    auto typeHostMap = make_unique<GuestServiceTypeHostMap>();
    typeHostMap_ = move(typeHostMap);
}

GuestServiceTypeHostManager::~GuestServiceTypeHostManager()
{
}

AsyncOperationSPtr GuestServiceTypeHostManager::BeginOpenGuestServiceTypeHost(
    ApplicationHostContext const & appHostContext,
    vector<GuestServiceTypeInfo> const & typesToHost,
    CodePackageContext const & codePackageContext,
    vector<wstring> && depdendentCodePackages,
    vector<EndpointDescription> && endpointDescriptions,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<OpenGuestServiceTypeHostAsyncOperation>(
        *this,
        appHostContext,
        typesToHost,
        codePackageContext,
        move(depdendentCodePackages),
        move(endpointDescriptions),
        timeout,
        callback,
        parent);
}

ErrorCode GuestServiceTypeHostManager::EndOpenGuestServiceTypeHost(AsyncOperationSPtr const & operation)
{
    return OpenGuestServiceTypeHostAsyncOperation::End(operation);
}

AsyncOperationSPtr GuestServiceTypeHostManager::BeginCloseGuestServiceTypeHost(
    ApplicationHostContext const & appHostContext,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseGuestServiceTypeHostAsyncOperation>(
        *this,
        appHostContext,
        timeout,
        callback,
        parent);
}

ErrorCode GuestServiceTypeHostManager::EndCloseGuestServiceTypeHost(AsyncOperationSPtr const & operation)
{
    return CloseGuestServiceTypeHostAsyncOperation::End(operation);
}

void GuestServiceTypeHostManager::AbortGuestServiceTypeHost(ApplicationHostContext const & appHostContext)
{
    GuestServiceTypeHostSPtr typeHost;
    auto error = typeHostMap_->Remove(appHostContext.HostId, typeHost);
   
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType,
            Root.TraceId,
            "Failed to find GuestServiceTypeHost for HostContext={0}. Error={1}.",
            appHostContext,
            error);
        return;
    }

    typeHost->Abort();

    this->NotifyAppHostHostManager(appHostContext.HostId, ProcessActivator::ProcessAbortExitCode);
}

void GuestServiceTypeHostManager::ProcessCodePackageEvent(
    ApplicationHostContext const & appHostContext,
    CodePackageEventDescription eventDescription)
{
    GuestServiceTypeHostSPtr typeHost;
    auto error = typeHostMap_->Find(appHostContext.HostId, typeHost);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType,
            Root.TraceId,
            "Failed to find GuestServiceTypeHost for HostContext={0}. Error={1}.",
            appHostContext,
            error);
        return;
    }

    auto componentRoot = this->Root.CreateComponentRoot();
    Threadpool::Post(
        [this, typeHost, eventDescription, componentRoot]()
        {
            typeHost->ProcessCodePackageEvent(eventDescription);
        });
}

AsyncOperationSPtr GuestServiceTypeHostManager::OnBeginOpen(
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    UNREFERENCED_PARAMETER(timeout);

    WriteInfo(
        TraceType,
        Root.TraceId,
        "Opening GuestServiceTypeHostManager");

    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
        ErrorCode::Success(),
        callback,
        parent);
}

ErrorCode GuestServiceTypeHostManager::OnEndOpen(AsyncOperationSPtr const & asyncOperation)
{
    WriteInfo(
        TraceType,
        Root.TraceId,
        "Opened GuestServiceTypeHostManager");

    return CompletedAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr GuestServiceTypeHostManager::OnBeginClose(
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    WriteInfo(
        TraceType,
        Root.TraceId,
        "Closing GuestServiceTypeHostManager");

    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode GuestServiceTypeHostManager::OnEndClose(AsyncOperationSPtr const & asyncOperation)
{
    WriteInfo(
        TraceType,
        Root.TraceId,
        "Closed GuestServiceTypeHostManager");

    return CloseAsyncOperation::End(asyncOperation);
}

void GuestServiceTypeHostManager::OnAbort()
{
    WriteInfo(
        TraceType,
        Root.TraceId,
        "Aborting GuestServiceTypeHostManager");

    auto activatedTypeHosts = typeHostMap_->Close();

    for (auto & typeHost : activatedTypeHosts)
    {
        typeHost->Abort();
    }
}

void GuestServiceTypeHostManager::NotifyAppHostHostManager(std::wstring const & hostId, uint exitCode)
{
    auto componentRoot = this->Root.CreateComponentRoot();

    Threadpool::Post(
        [this, hostId, exitCode, componentRoot]()
        {
            this->hosting_.ApplicationHostManagerObj->OnApplicationHostTerminated(
                ActivityDescription(
                    ActivityId(),
                    ActivityType::Enum::ServicePackageEvent),
                hostId, 
                exitCode);
        });
}

ErrorCode GuestServiceTypeHostManager::Test_HasGuestServiceTypeHost(
    wstring const & hostId, __out bool & isPresent)
{
    GuestServiceTypeHostSPtr typeHost;
    auto error = typeHostMap_->Find(hostId, typeHost);

    if (error.IsSuccess() || error.IsError(ErrorCodeValue::NotFound))
    {
        isPresent = error.IsSuccess();

        return ErrorCode::Success();
    }

    return error;
}

ErrorCode GuestServiceTypeHostManager::Test_GetTypeHost(
    std::wstring const & hostId, 
    __out GuestServiceTypeHostSPtr & guestTypeHost)
{
    GuestServiceTypeHostSPtr typeHost;
    auto error = typeHostMap_->Find(hostId, typeHost);

    if (error.IsSuccess())
    {
        guestTypeHost = typeHost;
    }

    return error;
}

ErrorCode GuestServiceTypeHostManager::Test_GetTypeHostCount(__out size_t & typeHostCount)
{
    return typeHostMap_->Count(typeHostCount);
}

ErrorCode GuestServiceTypeHostManager::Test_GetTypeHost(
    ServicePackageInstanceIdentifier const & servicePackageInstanceId,
    _Out_ GuestServiceTypeHostSPtr & guestTypeHost)
{
    auto typeHosts = typeHostMap_->GetServiceTypeHosts();

    for (auto const & kvPair : typeHosts)
    {
        if (kvPair->CodeContext.CodePackageInstanceId.ServicePackageInstanceId == servicePackageInstanceId)
        {
            guestTypeHost = kvPair;
            return ErrorCode::Success();
        }
    }

    return ErrorCode(ErrorCodeValue::NotFound);
}

