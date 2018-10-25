// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

StringLiteral const TraceType("InProcessApplicationHostProxy");

// ********************************************************************************************************************
// InProcessApplicationHostProxy::OpenAsyncOperation Implementation
//
class InProcessApplicationHostProxy::OpenAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(OpenAsyncOperation)

public:
    OpenAsyncOperation(
        __in InProcessApplicationHostProxy & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , timeoutHelper_(timeout)
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
            "Start OpenAsyncOperation: Id={0}, Type={1}, Timeout={2}",
            owner_.HostId,
            owner_.HostType,
            timeoutHelper_.GetRemainingTime());

        OpenGuestServiceTypeHost(thisSPtr);
    }

    void OpenGuestServiceTypeHost(AsyncOperationSPtr const & thisSPtr)
    {
        vector<EndpointDescription> endpointDescriptions;

        for (auto const & endpointResource : owner_.codePackageInstance_->EnvContext->Endpoints)
        {
            endpointDescriptions.push_back(endpointResource->EndpointDescriptionObj);
        }

        vector<wstring> depdendentCodePackages;
        if (owner_.Context.IsCodePackageActivatorHost)
        {
            for (auto const & dcpDesc : owner_.PackageDescription.DigestedCodePackages)
            {
                depdendentCodePackages.push_back(dcpDesc.Name);
            }
        }
        
        auto operation = owner_.TypeHostManager->BeginOpenGuestServiceTypeHost(
            owner_.Context,
            owner_.TypesToHost,
            owner_.codePackageInstance_->Context,
            move(depdendentCodePackages),
            move(endpointDescriptions),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) 
            { 
                this->FinishOpenGuestServiceTypeHost(operation, false); 
            },
            thisSPtr);

        FinishOpenGuestServiceTypeHost(operation, true);
    }

    void FinishOpenGuestServiceTypeHost(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.TypeHostManager->EndOpenGuestServiceTypeHost(operation);
        
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "End OpenAsyncOperation: Id={0}, Type={1}, Timeout={2}, Error={3}.",
            owner_.HostId,
            owner_.HostType,
            timeoutHelper_.GetRemainingTime(),
            error);

        TryComplete(operation->Parent, error);
    }

private:
    InProcessApplicationHostProxy & owner_;
    TimeoutHelper const timeoutHelper_;
};

// ********************************************************************************************************************
// InProcessApplicationHostProxy::CloseAsyncOperation Implementation
//
class InProcessApplicationHostProxy::CloseAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(CloseAsyncOperation)

public:
    CloseAsyncOperation(
        __in InProcessApplicationHostProxy & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , timeoutHelper_(timeout)
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
        WriteInfo(
            TraceType,
            owner_.TraceId,
            "Start CloseAsyncOperation: Id={0}, Type={1}, Timeout={2}",
            owner_.HostId,
            owner_.HostType,
            timeoutHelper_.GetRemainingTime());

        this->CloseGuestServiceTypeHost(thisSPtr);
    }

    void CloseGuestServiceTypeHost(AsyncOperationSPtr const & thisSPtr)
    {
        owner_.exitCode_.store(ProcessActivator::ProcessDeactivateExitCode);

        auto operation = owner_.TypeHostManager->BeginCloseGuestServiceTypeHost(
            owner_.Context,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->FinishCloseGuestServiceTypeHost(operation, false); },
            thisSPtr);

        FinishCloseGuestServiceTypeHost(operation, true);
    }

    void FinishCloseGuestServiceTypeHost(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.TypeHostManager->EndCloseGuestServiceTypeHost(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "Finish ClosingAsyncOperation: Id={0}, Type={1}, Timeout={2}, Error={3}.",
            owner_.HostId,
            owner_.HostType,
            timeoutHelper_.GetRemainingTime(),
            error);

        if (error.IsSuccess())
        {
            owner_.NotifyCodePackageInstance();
        }

        TryComplete(operation->Parent, error);
        return;
    }

private:
    InProcessApplicationHostProxy & owner_;
    TimeoutHelper const timeoutHelper_;
};

// ********************************************************************************************************************
// InProcessApplicationHostProxy::CloseAsyncOperation Implementation
//
class InProcessApplicationHostProxy::ApplicationHostCodePackageAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
public:
    ApplicationHostCodePackageAsyncOperation(
        __in InProcessApplicationHostProxy & owner,
        ApplicationHostCodePackageOperationRequest const & request,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , request_(request)
    {
    }

    static ErrorCode ApplicationHostCodePackageAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ApplicationHostCodePackageAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.Hosting.State.Value > FabricComponentState::Opened ||
            owner_.State.Value > FabricComponentState::Opened ||
            owner_.ShouldNotify() == false)
        {
            //
            // No need to proceed if Hosting or ApplicationHostProxy
            // is closing or ApplicationHost has terminated.
            //
            this->TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::ObjectClosed));
            return;
        }

        auto operation = owner_.codePackageInstance_->BeginApplicationHostCodePackageOperation(
            request_,
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnApplicationHostCodePackageOperationCompleted(operation, false);
            },
            thisSPtr);

        this->OnApplicationHostCodePackageOperationCompleted(operation, true);
    }

private:

    void OnApplicationHostCodePackageOperationCompleted(
        AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.codePackageInstance_->EndApplicationHostCodePackageOperation(operation);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "OnApplicationHostCodePackageOperationCompleted: ErrorCode={0}",
            error);

        this->TryComplete(operation->Parent, error);
    }

private:
    InProcessApplicationHostProxy & owner_;
    ApplicationHostCodePackageOperationRequest request_;
};

AsyncOperationSPtr InProcessApplicationHostProxy::BeginActivateCodePackage(
    CodePackageInstanceSPtr const & codePackage,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    UNREFERENCED_PARAMETER(codePackage);
    UNREFERENCED_PARAMETER(timeout);
    
    if (isCodePackageActive_.load())
    {
        Assert::CodingError(
            "InProcessApplicationHostProxy::BeginActivateCodePackage should only be called once. CodePackage {0}",
            codePackageInstance_->Context);
    }

    isCodePackageActive_.store(true);

    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
        ErrorCode::Success(),
        callback,
        parent);
}

ErrorCode InProcessApplicationHostProxy::EndActivateCodePackage(
    Common::AsyncOperationSPtr const & operation,
    __out CodePackageActivationId & activationId,
    __out CodePackageRuntimeInformationSPtr & codePackageRuntimeInformation)
{
    activationId = codePackageActivationId_;
    codePackageRuntimeInformation = codePackageRuntimeInformation_;

    return CompletedAsyncOperation::End(operation);
}

AsyncOperationSPtr InProcessApplicationHostProxy::BeginDeactivateCodePackage(
    CodePackageInstanceIdentifier const & codePackageInstanceId,
    CodePackageActivationId const & activationId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    UNREFERENCED_PARAMETER(codePackageInstanceId);
    UNREFERENCED_PARAMETER(activationId);
    UNREFERENCED_PARAMETER(timeout);

    isCodePackageActive_.store(false);

    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
        ErrorCode::Success(),
        callback,
        parent);
}

ErrorCode InProcessApplicationHostProxy::EndDeactivateCodePackage(
    AsyncOperationSPtr const & operation)
{
    return CompletedAsyncOperation::End(operation);
}

AsyncOperationSPtr InProcessApplicationHostProxy::BeginUpdateCodePackageContext(
    CodePackageContext const & codePackageContext,
    CodePackageActivationId const & codePackageActivationId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    UNREFERENCED_PARAMETER(codePackageContext);
    UNREFERENCED_PARAMETER(codePackageActivationId);
    UNREFERENCED_PARAMETER(timeout);

    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
        ErrorCode::Success(),
        callback,
        parent);
}

ErrorCode InProcessApplicationHostProxy::EndUpdateCodePackageContext(
    AsyncOperationSPtr const & operation)
{
    return CompletedAsyncOperation::End(operation);
}

AsyncOperationSPtr InProcessApplicationHostProxy::BeginGetContainerInfo(
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

ErrorCode InProcessApplicationHostProxy::EndGetContainerInfo(
    AsyncOperationSPtr const & operation,
    __out wstring & containerInfo)
{
    containerInfo = L"Operation not Supported";
    return CompletedAsyncOperation::End(operation);
}

bool InProcessApplicationHostProxy::HasHostedCodePackages()
{
    return isCodePackageActive_.load();
}

void InProcessApplicationHostProxy::OnApplicationHostRegistered()
{
    // NO-OP
}

void InProcessApplicationHostProxy::OnCodePackageTerminated(
    CodePackageInstanceIdentifier const & codePackageInstanceId,
    CodePackageActivationId const & activationId)
{
    UNREFERENCED_PARAMETER(codePackageInstanceId);
    UNREFERENCED_PARAMETER(activationId);

    // NO-OP
}

InProcessApplicationHostProxy::InProcessApplicationHostProxy(
    HostingSubsystemHolder const & hostingHolder,
    wstring const & hostId,
    ApplicationHostIsolationContext const & isolationContext,
    CodePackageInstanceSPtr const & codePackageInstance)
    : ApplicationHostProxy(
        hostingHolder,
        ApplicationHostContext(
            hostId, 
            ApplicationHostType::Activated_InProcess,
            false,
            codePackageInstance->IsActivator),
        isolationContext,
        codePackageInstance->RunAsId,
        codePackageInstance->CodePackageInstanceId.ServicePackageInstanceId,
        codePackageInstance->EntryPoint.EntryPointType,
        codePackageInstance->CodePackageObj.RemoveServiceFabricRuntimeAccess)
    , codePackageActivationId_(hostId)
    , codePackageInstance_(codePackageInstance)
    , exitCode_(1)
    , notificationLock_()
    , shouldNotify_(true)
    , terminatedExternally_(false)
    , isCodePackageActive_(false)
    , codePackageRuntimeInformation_(move(make_shared<CodePackageRuntimeInformation>(L"", ::GetCurrentProcessId())))
{
    ASSERT_IFNOT(
        codePackageInstance_->CodePackageObj.IsImplicitTypeHost == true,
        "InProcessApplicationHostProxy has CodePackageObj.IsGuestServiceTypeHost=true. CodePackage {0}",
        codePackageInstance_->Context);
}

InProcessApplicationHostProxy::~InProcessApplicationHostProxy()
{
}

AsyncOperationSPtr InProcessApplicationHostProxy::OnBeginOpen(
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

ErrorCode InProcessApplicationHostProxy::OnEndOpen(AsyncOperationSPtr const & asyncOperation)
{
    return OpenAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr InProcessApplicationHostProxy::OnBeginClose(
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

ErrorCode InProcessApplicationHostProxy::OnEndClose(AsyncOperationSPtr const & asyncOperation)
{
    return CloseAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr InProcessApplicationHostProxy::BeginApplicationHostCodePackageOperation(
    ApplicationHostCodePackageOperationRequest const & requestBody,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ApplicationHostCodePackageAsyncOperation>(
        *this,
        requestBody,
        callback,
        parent);
}

ErrorCode InProcessApplicationHostProxy::EndApplicationHostCodePackageOperation(
    AsyncOperationSPtr const & operation)
{
    return ApplicationHostCodePackageAsyncOperation::End(operation);
}

void InProcessApplicationHostProxy::SendDependentCodePackageEvent(
    CodePackageEventDescription const & eventDescription)
{
    WriteInfo(
        TraceType,
        TraceId,
        "SendDependentCodePackageEvent: HostContext={0}, CodePackageInstanceId={1}, Event={2}.",
        this->Context,
        this->CodePackageInstanceId,
        eventDescription);

    this->TypeHostManager->ProcessCodePackageEvent(this->Context, eventDescription);
}

void InProcessApplicationHostProxy::OnAbort()
{
    WriteWarning(
        TraceType,
        TraceId,
        "OnAbort called for HostId={0}, ServicePackageInstanceId={1}.",
        this->HostId,
        this->ServicePackageInstanceId);

    if (!terminatedExternally_.load())
    {
        this->exitCode_.store(ProcessActivator::ProcessAbortExitCode);
        this->TypeHostManager->AbortGuestServiceTypeHost(this->Context);
    }

    this->NotifyCodePackageInstance();
}

void InProcessApplicationHostProxy::OnApplicationHostTerminated(DWORD exitCode)
{
    this->exitCode_.store(exitCode);

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

void InProcessApplicationHostProxy::OnContainerHealthCheckStatusChanged(ContainerHealthStatusInfo const & healthStatusInfo)
{
    UNREFERENCED_PARAMETER(healthStatusInfo);

    // NO-OP
}

void InProcessApplicationHostProxy::TerminateExternally()
{
    WriteInfo(
        TraceType,
        TraceId,
        "TerminateExternally called for HostId={0}, ServicePackageInstanceId={1}.",
        this->HostId,
        this->ServicePackageInstanceId);

    terminatedExternally_.store(true);
    this->TypeHostManager->AbortGuestServiceTypeHost(this->Context);
}

bool InProcessApplicationHostProxy::ShouldNotify()
{
    bool shouldNotify;

    {
        AcquireWriteLock readLock(notificationLock_);
        shouldNotify = shouldNotify_;
    }

    return shouldNotify;
}

void InProcessApplicationHostProxy::NotifyCodePackageInstance()
{
    if (this->Hosting.State.Value > FabricComponentState::Opened)
    {
        // no need to notify if the Subsystem is closing down
        return;
    }

    {
        AcquireWriteLock writeLock(notificationLock_);
        if (!shouldNotify_)
        {
            return;
        }

        shouldNotify_ = false;
    }

    auto exitCode = (DWORD)exitCode_.load();

    //
    // InProc FabricTypeHost is implementation detail for guest app and is only
    // terminated as part of deactivation or when explicitly terminated to trigger
    // failover when one of the guest code package terminates. Skipping reporting
    // health will help not to clutter actual health report that comes from terminating
    // guest code package and also hides implementation detail.
    //
    codePackageInstance_->OnEntryPointTerminated(
        codePackageActivationId_,
        exitCode,
        true /* ignore health reporting */);
}

