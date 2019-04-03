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
            "InitializeApplicationHost: HostContext={0}, ServicePackageActivationId={1}, RuntimeServiceAddress={2}, ErrorCode={3}.",
            owner_.HostContext,
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
            "Begin(OpenApplicationHost): HostContext={0}, ServicePackageActivationId={1}, Timeout={2}.",
            owner_.HostContext,
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
            "End(OpenApplicationHost): HostContext={0}, ServicePackageActivationId={1}, ErrorCode={2}",
            owner_.HostContext,
            owner_.ServicePackageActivationId,
            error);
        
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }

        if (owner_.applicationhost_->HostContext.IsCodePackageActivatorHost)
        {
            owner_.activationManager_ = make_com<ComGuestServiceCodePackageActivationManager>(owner_);
            error = owner_.activationManager_->Open();
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceType,
                    owner_.TraceId,
                    "OpenGuestServiceCodePackageActivationManager: HostContext={0}, ServicePackageActivationId={1}, ErrorCode={2}",
                    owner_.HostContext,
                    owner_.ServicePackageActivationId,
                    error);

                this->TryComplete(operation->Parent, error);
                return;
            }
        }
        
        this->CreateComFabricRuntime(operation->Parent);
    }

    void CreateComFabricRuntime(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
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
        auto statelessServiceFactory = 
            make_com<ComStatelessGuestServiceTypeFactory, IFabricStatelessServiceFactory>(owner_);

        auto statefulServiceFactory =
            make_com<ComStatefulGuestServiceTypeFactory, IFabricStatefulServiceFactory>(owner_);

        for (auto const & guestType : owner_.serviceTypesToHost_)
        {
            auto hr = S_OK;
            
            if (guestType.IsStateful)
            {
                hr = owner_.runtime_->RegisterStatefulServiceFactory(
                    guestType.ServiceTypeName.c_str(), 
                    statefulServiceFactory.GetRawPointer());
            }
            else
            {
                hr = owner_.runtime_->RegisterStatelessServiceFactory(
                    guestType.ServiceTypeName.c_str(), 
                    statelessServiceFactory.GetRawPointer());
            }

            auto error = ErrorCode::FromHResult(hr);

            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceType,
                    owner_.TraceId,
                    "Failed to register GuestServiceTypeFactory for {0}. HostId={1}, ServicePackageActivationId={2}, IsStateful={3}, Error={4}.",
                    guestType.ServiceTypeName,
                    owner_.HostId,
                    owner_.ServicePackageActivationId,
                    guestType.IsStateful,
                    error);

                TryComplete(thisSPtr, error);
                return;
            }

            WriteNoise(
                TraceType,
                owner_.TraceId,
                "Succesfully registered GuestServiceTypeFactory for {0}. HostId={1}, ServicePackageActivationId={2}, IsStateful={3}.",
                guestType.ServiceTypeName,
                owner_.HostId,
                owner_.ServicePackageActivationId,
                guestType.IsStateful);
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

        this->CloseApplicationHost(thisSPtr);
    }

private:
    void CloseApplicationHost(AsyncOperationSPtr const & thisSPtr)
    {
        owner_.runtime_.Release();

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

        // Note that the close/release should be done after the close of application host.
        // If activationManager_ is released before applicationhost closes aborts it will result in an access violation.
        if (owner_.applicationhost_->HostContext.IsCodePackageActivatorHost)
        {
            owner_.activationManager_->Close();
            owner_.activationManager_.Release();
        }
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

        this->TryComplete(operation->Parent, lastError_);
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
    ApplicationHostContext const & hostContext,
    vector<GuestServiceTypeInfo> const & serviceTypesToHost,
    CodePackageContext const & codePackageContext,
    wstring const & runtimeServiceAddress,
    vector<wstring> && depdendentCodePackages,
    vector<EndpointDescription> && endpointDescriptions)
    : hostingHolder_(hostingHolder)
    , hostContext_(hostContext)
    , serviceTypesToHost_(serviceTypesToHost)
    , codePackageContext_(codePackageContext)
    , runtimeServiceAddress_(runtimeServiceAddress)
    , runtimeId_(Guid::NewGuid().ToString())
    , applicationhost_()
    , runtime_()
    , activationManager_()
    , depdendentCodePackages_(depdendentCodePackages)
    , endpointDescriptions_(endpointDescriptions)
{
}

GuestServiceTypeHost::~GuestServiceTypeHost()
{
}

ErrorCode GuestServiceTypeHost::InitializeApplicationHost()
{
    return InProcessApplicationHost::Create(
        *this,
        runtimeServiceAddress_,
        hostContext_,
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
    if (runtime_)
    {
        runtime_.Release();
    }

    // Note the order of abort/release here is important.
    // If activationManager_ is released before applicationhost aborts it will result in an access violation.
    // ComGuestServiceInstance has a reference to this class and on abort will attempt to access activationManager
    // to call UnregisterServiceReplicaOrInstance.
    if (applicationhost_)
    {
        applicationhost_->Abort();
    }

    if (hostContext_.IsCodePackageActivatorHost && activationManager_)
    {
        activationManager_->Close();
        activationManager_.Release();
    }
}

void GuestServiceTypeHost::ProcessCodePackageEvent(CodePackageEventDescription eventDescription)
{
    if (this->State.Value > FabricComponentState::Opened)
    {
        // no need to notify if type host is closing down
        return;
    }

    applicationhost_->ProcessCodePackageEvent(eventDescription);
}

ErrorCode GuestServiceTypeHost::GetCodePackageActivator(
    _Out_ Common::ComPointer<IFabricCodePackageActivator> & codePackageActivator)
{
    return applicationhost_->GetCodePackageActivator(codePackageActivator);
}
