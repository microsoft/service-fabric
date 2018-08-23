// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;

StringLiteral const TraceType("InProcessApplicationHost");

// ********************************************************************************************************************
// InProcessApplicationHost Implementation
//
InProcessApplicationHost::InProcessApplicationHost(
    GuestServiceTypeHost & guestServiceTypeHost,
    wstring const & runtimeServiceAddress,
    ApplicationHostContext const & hostContext,
    CodePackageContext const & codePackageContext)
    : ApplicationHost(
        hostContext,
        runtimeServiceAddress,
        nullptr,
        L"",
        false,
        nullptr),
    guestServiceTypeHost_(guestServiceTypeHost),
    codeContextLock_(),
    codePackageContext_(codePackageContext),
    codePackageActivator_()
{
    if (hostContext.IsCodePackageActivatorHost)
    {
        auto cpActivator = make_shared<InProcessApplicationHostCodePackageActivator>(*this, *this);
        codePackageActivator_ = move(cpActivator);
    }
}

InProcessApplicationHost::~InProcessApplicationHost()
{
}

ErrorCode InProcessApplicationHost::Create(
    GuestServiceTypeHost & guestServiceTypeHost,
    wstring const & runtimeServiceAddress,
    ApplicationHostContext const & hostContext,
    CodePackageContext const & codePackageContext,
    __out InProcessApplicationHostSPtr & applicationHost)
{

    applicationHost = make_shared<InProcessApplicationHost>(
        guestServiceTypeHost,
        runtimeServiceAddress,
        hostContext,
        codePackageContext);

    return ErrorCode(ErrorCodeValue::Success);
}

void InProcessApplicationHost::GetCodePackageContext(
    CodePackageContext & codeContext)
{
    AcquireWriteLock grab(codeContextLock_);
    codeContext = codePackageContext_;
}

ErrorCode InProcessApplicationHost::OnCreateAndAddFabricRuntime(
    FabricRuntimeContextUPtr const & fabricRuntimeContextUPtr,
    ComPointer<IFabricProcessExitHandler> const & fabricExitHandler,
    __out FabricRuntimeImplSPtr & fabricRuntime)
{
    if (fabricExitHandler.GetRawPointer() != NULL)
    {
        WriteNoise(
            TraceType,
            TraceId,
            "OnCreateAndAddFabricRuntime: ignoring non-NULL FabricProcessExitHandler for activated InProcessApplicationHost");
    }

    if (fabricRuntimeContextUPtr)
    {
        auto error = ErrorCode::FromHResult(E_INVALIDARG);
        WriteWarning(
            TraceType,
            TraceId,
            "OnCreateAndAddFabricRuntime: FabricRuntimeContext {0} must not be supplied. ErrorCode={1}",
            *fabricRuntimeContextUPtr,
            error);
        return error;
    }

    ErrorCode error = ErrorCodeValue::Success;
    {
        AcquireReadLock grab(codeContextLock_);

        wstring runtimeId = Guid::NewGuid().ToString();
        FabricRuntimeContext fabricRuntimeContext(runtimeId, this->HostContext, codePackageContext_);
        
        fabricRuntime = make_shared<FabricRuntimeImpl>(
            *this, /* ComponentRoot & */
            *this, /* ApplicationHost & */
            move(fabricRuntimeContext),
            ComPointer<IFabricProcessExitHandler>());

        error = this->AddFabricRuntime(fabricRuntime);
    }    

    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        TraceId,
        "OnCreateAndAddFabricRuntime: RuntimeTableObj.AddPending: ErrorCode={0}, FabricRuntime={1}", 
        error,
        static_cast<void*>(fabricRuntime.get())); 

    return error;
}

ErrorCode InProcessApplicationHost::OnGetCodePackageActivationContext(
    CodePackageContext const & codeContext,
    __out CodePackageActivationContextSPtr & codePackageActivationContext)
{
    UNREFERENCED_PARAMETER(codePackageActivationContext);

    Assert::CodingError(
        "InProcessApplicationHost::OnGetCodePackageActivationContext should never be called. CodePackage {0}",
        codeContext);
}

ErrorCode InProcessApplicationHost::OnGetCodePackageActivator(
    _Out_ ApplicationHostCodePackageActivatorSPtr & codePackageActivator)
{
    if (this->HostContext.IsCodePackageActivatorHost)
    {
        codePackageActivator = codePackageActivator_;
        return ErrorCode::Success();
    }

    return ErrorCode(ErrorCodeValue::OperationNotSupported);
}

ErrorCode InProcessApplicationHost::OnCodePackageEvent(
    CodePackageEventDescription const & eventDescription)
{
    ApplicationHostCodePackageActivatorSPtr codePackageActivator;
    auto error = this->OnGetCodePackageActivator(codePackageActivator);
    if (!error.IsSuccess())
    {
        return error;
    }

    codePackageActivator->NotifyEventHandlers(eventDescription);

    return ErrorCode::Success();
}

void InProcessApplicationHost::ProcessCodePackageEvent(CodePackageEventDescription eventDescription)
{
    if (this->State.Value > FabricComponentState::Opened)
    {
        // no need to notify if type host is closing down
        return;
    }

    this->OnCodePackageEvent(eventDescription);
}

ErrorCode InProcessApplicationHost::OnUpdateCodePackageContext(
    CodePackageContext const & codeContext)
{
    AcquireWriteLock lock(codeContextLock_);

    codePackageContext_ = codeContext;

    return ErrorCode::Success();
}

