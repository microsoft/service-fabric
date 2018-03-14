// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

StringLiteral const TraceType("GuestServiceTypeHost");

// ********************************************************************************************************************
// GuestServiceTypeHost::OpenAsyncOperation Implementation
//
class GuestServiceTypeHost::OpenAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(OpenAsyncOperation)

public:
    OpenAsyncOperation(
        GuestServiceTypeHost & owner,
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
            "Opening GuestServiceTypeHost: HostId={0}, ServicePackageActivationId={1}, Timeout={2}.",
            owner_.HostId,
            owner_.ServicePackageActivationId,
            timeoutHelper_.GetRemainingTime());

        CreateApplicationHost(thisSPtr);
    }

private:
    void CreateApplicationHost(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.InitializeApplicationHost();
        
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "InitializeApplicationHost: HostId={0}, ServicePackageActivationId={1}, RuntimeServiceAddress={2}, ErrorCode={3}.",
            owner_.HostId,
            owner_.ServicePackageActivationId,
            owner_.runtimeServiceAddress_,
            error);
        
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        OpenApplicationHost(thisSPtr);
    }

    void OpenApplicationHost(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(OpenApplicationHost): HostId={0}, ServicePackageActivationId={1}, Timeout={2}.",
            owner_.HostId,
            owner_.ServicePackageActivationId,
            timeoutHelper_.GetRemainingTime());
        
        auto operation = owner_.applicationhost_->BeginOpen(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->FinishApplicationHostOpen(operation, false); },
            thisSPtr);
        
        FinishApplicationHostOpen(operation, true);
    }

    void FinishApplicationHostOpen(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        
        auto error = owner_.applicationhost_->EndOpen(operation);
        
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "End(OpenApplicationHost): HostId={0}, ServicePackageActivationId={1}, ErrorCode={2}",
            owner_.HostId,
            owner_.ServicePackageActivationId,
            error);
        
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }

        CreateComFabricRuntime(operation->Parent);
    }

    void CreateComFabricRuntime(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
            TraceType,
            owner_.TraceId,
            "Begin(CreateComFabricRuntime): HostId={0}, ServicePackageActivationId={1}, Timeout={2}",
            owner_.HostId,
            owner_.ServicePackageActivationId,
            timeoutHelper_.GetRemainingTime());
        
        auto operation = owner_.applicationhost_->BeginCreateComFabricRuntime(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->FinishCreateComFabricRuntime(operation, false); },
            thisSPtr);
        
        FinishCreateComFabricRuntime(operation, true);
    }

    void FinishCreateComFabricRuntime(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.applicationhost_->EndCreateComFabricRuntime(operation, owner_.runtime_);
        
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "End(CreateComFabricRuntime): HostId={0}, ServicePackageActivationId={1}, ErrorCode={2}",
            owner_.HostId,
            owner_.ServicePackageActivationId,
            error);

        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }

        owner_.runtimeId_ = owner_.runtime_->Runtime->RuntimeId;

        RegisterServiceTypes(operation->Parent);
    }

    void RegisterServiceTypes(AsyncOperationSPtr const & thisSPtr)
    {
        auto serviceFactory = make_com<ComGuestServiceTypeFactory, IFabricStatelessServiceFactory>(owner_.endpointDescriptions_);

        for (auto const & guestType : owner_.serviceTypesToHost_)
        {
            auto hr = owner_.runtime_->RegisterStatelessServiceFactory(guestType.c_str(), serviceFactory.GetRawPointer());
            
            if (FAILED(hr))
            {
                WriteWarning(
                    TraceType,
                    owner_.TraceId,
                    "Failed to register ComGuestServiceTypeFactory for {0}. HostId={1}, ServicePackageActivationId={2}, HRESULT={3}",
                    guestType,
                    owner_.HostId,
                    owner_.ServicePackageActivationId,
                    hr);

                TryComplete(thisSPtr, ErrorCode::FromHResult(hr));
                return;
            }
        }

        TryComplete(thisSPtr, ErrorCode::Success());
        return;
    }

private:
    GuestServiceTypeHost & owner_;
    TimeoutHelper timeoutHelper_;
};

// ********************************************************************************************************************
// GuestServiceTypeHost::CloseAsyncOperation Implementation
//
class GuestServiceTypeHost::CloseAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(CloseAsyncOperation)

public:
    CloseAsyncOperation(
        GuestServiceTypeHost & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
		lastError_()
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
            "Closing GuestServiceTypeHost: HostId={0}, ServicePackageActivationId={1}, Timeout={2}.",
            owner_.HostId,
            owner_.ServicePackageActivationId,
            timeoutHelper_.GetRemainingTime());

        CloseApplicationHost(thisSPtr);
    }

private:
    void CloseApplicationHost(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(CloseApplicationHost): HostId={0}, ServicePackageActivationId={1}, Timeout={2}.",
            owner_.HostId,
            owner_.ServicePackageActivationId,
            timeoutHelper_.GetRemainingTime());

        auto operation = owner_.applicationhost_->BeginClose(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->FinishCloseApplicationHost(operation, false); },
            thisSPtr);
        
        FinishCloseApplicationHost(operation, true);
    }

    void FinishCloseApplicationHost(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        lastError_ = owner_.applicationhost_->EndClose(operation);
        
        WriteTrace(
            lastError_.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "End(CloseApplicationHost): HostId={0}, ServicePackageActivationId={1}, ErrorCode={2}.",
            owner_.HostId,
            owner_.ServicePackageActivationId,
            lastError_);

        CloseRuntime(operation->Parent);
    }

    void CloseRuntime(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Releasing runtime: HostId={0}, ServicePackageActivationId={1}.",
            owner_.HostId,
            owner_.ServicePackageActivationId);
        
        owner_.runtime_.Release();
        
        TryComplete(thisSPtr, lastError_);
        
        return;
    }

private:
    GuestServiceTypeHost & owner_;
    TimeoutHelper timeoutHelper_;
	ErrorCode lastError_;
};

// ********************************************************************************************************************
// GuestServiceTypeHost Implementation
//
GuestServiceTypeHost::GuestServiceTypeHost(
    HostingSubsystemHolder const & hostingHolder,
    wstring const & hostId,
    vector<std::wstring> const & serviceTypesToHost,
    CodePackageContext const & codePackageContext,
    wstring const & runtimeServiceAddress,
    vector<EndpointDescription> && endpointDescriptions)
    : hostingHolder_(hostingHolder)
    , hostId_(hostId)
    , serviceTypesToHost_(serviceTypesToHost)
    , codePackageContext_(codePackageContext)
    , runtimeServiceAddress_(runtimeServiceAddress)
    , runtimeId_(Guid::NewGuid().ToString())
    , applicationhost_()
    , runtime_()
    , endpointDescriptions_(endpointDescriptions)
{
    WriteNoise(
        TraceType, 
        TraceId, 
        "ctor: HostId={0}, ServicePackageActivationId={1}.",
        hostId,
        codePackageContext_.CodePackageInstanceId.ServicePackageInstanceId.PublicActivationId);
}

GuestServiceTypeHost::~GuestServiceTypeHost()
{
    WriteNoise(
        TraceType, 
        TraceId, 
        "dtor: HostId={0}, ServicePackageActivationId={1}.",
        this->HostId,
        this->ServicePackageActivationId);
}

ErrorCode GuestServiceTypeHost::InitializeApplicationHost()
{
    return InProcessApplicationHost::Create(
        hostId_,
        runtimeServiceAddress_,
        codePackageContext_,
        applicationhost_);
}

AsyncOperationSPtr GuestServiceTypeHost::OnBeginOpen(
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

ErrorCode GuestServiceTypeHost::OnEndOpen(AsyncOperationSPtr const & asyncOperation)
{
    return OpenAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr GuestServiceTypeHost::OnBeginClose(
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

ErrorCode GuestServiceTypeHost::OnEndClose(AsyncOperationSPtr const & asyncOperation)
{
    return CloseAsyncOperation::End(asyncOperation);
}

void GuestServiceTypeHost::OnAbort()
{
    if (applicationhost_)
    {
        applicationhost_->Abort();
    }

    if (runtime_)
    {
        runtime_.Release();
    }

    WriteNoise(
        TraceType,
        this->TraceId,
        "Aborted: HostId={0}, ServicePackageActivationId={1}.",
        this->HostId,
        this->ServicePackageActivationId);
}
