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
        wstring const & hostId,
        vector<wstring> const & typesToHost,
        CodePackageContext const & codePackageContext,
        vector<EndpointDescription> && endpointDescriptions,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , hostId_(hostId)
        , typesToHost_(typesToHost)
        , codePackageContext_(codePackageContext)
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
            "Start OpenGuestServiceTypeHostAsyncOperation: Id={0}, Timeout={1}",
            hostId_,
            timeoutHelper_.GetRemainingTime());

        CreateGuestServiceTypeHost(thisSPtr);
    }

    void CreateGuestServiceTypeHost(AsyncOperationSPtr const & thisSPtr)
    {
        auto typeHost = make_shared<GuestServiceTypeHost>(
            HostingSubsystemHolder(owner_.hosting_, owner_.hosting_.Root.CreateComponentRoot()),
            hostId_,
            typesToHost_,
            codePackageContext_,
            owner_.hosting_.RuntimeServiceAddress,
            move(endpointDescriptions_));

        typeHostSPtr_ = move(typeHost);

        auto error = owner_.typeHostMap_->Add(typeHostSPtr_);

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "Failed to add GuestServiceTypeHost to map: Id={0}, Timeout={2}",
                hostId_,
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
            "End OpenGuestServiceTypeHostAsyncOperation: Id={0}, Timeout={1}, Error={2}",
            hostId_,
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
    wstring hostId_;
    vector<wstring> typesToHost_;
    CodePackageContext codePackageContext_;
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
        wstring const & hostId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , hostId_(hostId)
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
            "Start CloseGuestServiceTypeHostAsyncOperation: Id={0}, Timeout={1}",
            hostId_,
            timeoutHelper_.GetRemainingTime());

        FindGuestServiceTypeHost(thisSPtr);
    }

    void FindGuestServiceTypeHost(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.typeHostMap_->Find(hostId_, typeHostToCloseSPtr_);

        if (error.IsError(ErrorCodeValue::NotFound))
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "Failed to find GuestServiceTypeHost for HostId={0}. Completing stop operation.",
                hostId_);

            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "Failed to find ApplicationHostProxy for HostId={0}. ErrorCode={1}.",
                hostId_,
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
            "End CloseGuestServiceTypeHostAsyncOperation: Id={0}, Timeout={1}, Error={2}",
            hostId_,
            timeoutHelper_.GetRemainingTime(),
            error);

        GuestServiceTypeHostSPtr removed;
        owner_.typeHostMap_->Remove(hostId_, removed).ReadValue();

        if (error.IsSuccess())
        {
            owner_.NotifyAppHostHostManager(hostId_, ProcessActivator::ProcessDeactivateExitCode);
        }

        TryComplete(operation->Parent, error);
        return;
    }

private:
    GuestServiceTypeHostManager & owner_;
    wstring hostId_;
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
    wstring const & hostId,
    vector<wstring> const & typesToHost,
    CodePackageContext const & codePackageContext,
    vector<EndpointDescription> && endpointDescriptions,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<OpenGuestServiceTypeHostAsyncOperation>(
        *this,
        hostId,
        typesToHost,
        codePackageContext,
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
    wstring const & hostId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseGuestServiceTypeHostAsyncOperation>(
        *this,
        hostId,
        timeout,
        callback,
        parent);
}

ErrorCode GuestServiceTypeHostManager::EndCloseGuestServiceTypeHost(AsyncOperationSPtr const & operation)
{
    return CloseGuestServiceTypeHostAsyncOperation::End(operation);
}

void GuestServiceTypeHostManager::AbortGuestServiceTypeHost(wstring const & hostId)
{
    GuestServiceTypeHostSPtr typeHost;
    auto error = typeHostMap_->Remove(hostId, typeHost);
   
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType,
            Root.TraceId,
            "Failed to find GuestServiceTypeHost for HostId={0}. Error={1}.",
            hostId,
            error);
        return;
    }

    typeHost->Abort();

    this->NotifyAppHostHostManager(hostId, ProcessActivator::ProcessAbortExitCode);
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
            this->hosting_.ApplicationHostManagerObj->OnApplicationHostTerminated(hostId, exitCode);
        });
}

ErrorCode GuestServiceTypeHostManager::Test_HasGuestServiceTypeHost(wstring const & hostId, __out bool & isPresent)
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

ErrorCode GuestServiceTypeHostManager::Test_GetRuntimeId(std::wstring const & hostId, __out std::wstring & runtimeId)
{
    GuestServiceTypeHostSPtr typeHost;
    auto error = typeHostMap_->Find(hostId, typeHost);

    if (error.IsSuccess())
    {
        runtimeId = typeHost->RuntimeId;
    }

    return error;
}

ErrorCode GuestServiceTypeHostManager::Test_GetTypeHostCount(__out size_t & typeHostCount)
{
    return typeHostMap_->Count(typeHostCount);
}
