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
    wstring const & hostId, 
    wstring const & runtimeServiceAddress,
    CodePackageContext const & codePackageContext)
    : ApplicationHost(
        ApplicationHostContext(hostId, ApplicationHostType::Activated_InProcess, false),
        runtimeServiceAddress,
        nullptr,
        L"",
        false,
        nullptr),
    codeContextLock_(),
    codePackageContext_(codePackageContext)
{
}

InProcessApplicationHost::~InProcessApplicationHost()
{
}

ErrorCode InProcessApplicationHost::Create(
    wstring const & hostId, 
    wstring const & runtimeServiceAddress,
    CodePackageContext const & codePackageContext,
    __out ApplicationHostSPtr & applicationHost)
{
    WriteNoise(
        TraceType,
        "Creating: HostId={0}, CodeContext={1}",
        hostId,        
        codePackageContext);

    applicationHost = make_shared<InProcessApplicationHost>(
        hostId,
        runtimeServiceAddress,
        codePackageContext);

    return ErrorCode(ErrorCodeValue::Success);
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

ErrorCode InProcessApplicationHost::OnUpdateCodePackageContext(
    CodePackageContext const & codeContext)
{
    AcquireWriteLock lock(codeContextLock_);

    codePackageContext_ = codeContext;

    return ErrorCode::Success();
}

