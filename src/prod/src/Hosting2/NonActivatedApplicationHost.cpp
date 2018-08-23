// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

StringLiteral const TraceType("NonActivatedApplicationHost");

// ********************************************************************************************************************
// NonActivatedApplicationHost Implementation
//

NonActivatedApplicationHost::NonActivatedApplicationHost(
    wstring const & hostId,
    KtlSystem * ktlSystem,
    wstring const & runtimeServiceAddress)
     : ApplicationHost(
        ApplicationHostContext(
            hostId, 
            ApplicationHostType::NonActivated, 
            false /* isContainerHost */,
            false /* isCodePackageActivatorHost */),
        runtimeServiceAddress,
        nullptr,
        L"",
        (ktlSystem != nullptr), // useSystemServiceSharedLogSettings
        ktlSystem)
{
}

NonActivatedApplicationHost::~NonActivatedApplicationHost()
{
}

ErrorCode NonActivatedApplicationHost::Create(
    KtlSystem * ktlSystem,
    wstring const & runtimeServiceAddress,
    __out ApplicationHostSPtr & applicationHost)
{
    wstring hostId = Guid::NewGuid().ToString();
    WriteNoise(
        TraceType,
        "Creating NonActivatedApplicationHost: HostId={0}",
        hostId);

    applicationHost = make_shared<NonActivatedApplicationHost>(hostId, ktlSystem, runtimeServiceAddress);
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode NonActivatedApplicationHost::Create2(
    KtlSystem * ktlSystem,
    wstring const & runtimeServiceAddress,
    wstring const & hostId,
    __out ApplicationHostSPtr & applicationHost)
{
    WriteNoise(
        TraceType,
        "Creating NonActivatedApplicationHost: HostId={0}",
        hostId);

    applicationHost = make_shared<NonActivatedApplicationHost>(hostId, ktlSystem, runtimeServiceAddress);
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode NonActivatedApplicationHost::OnCreateAndAddFabricRuntime(
    FabricRuntimeContextUPtr const & fabricRuntimeContextUPtr,    
    ComPointer<IFabricProcessExitHandler> const & fabricExitHandler,
    __out FabricRuntimeImplSPtr & fabricRuntime)
{
    if (fabricRuntimeContextUPtr)
    {
        auto error = ErrorCode::FromHResult(E_INVALIDARG);
        WriteWarning(
            TraceType,
            TraceId,
            "OnCreateFabricRuntime: FabricRuntimeContext {0} must not be supplied. ErrorCode={1}",
            *fabricRuntimeContextUPtr,
            error);
        return error;
    }

    wstring runtimeId = Guid::NewGuid().ToString();
    fabricRuntime = make_shared<FabricRuntimeImpl>(
        *this, /* ComponentRoot & */
        *this, /* ApplicationHost & */
        FabricRuntimeContext(runtimeId, this->HostContext, CodePackageContext()),
        fabricExitHandler);

    auto error = this->AddFabricRuntime(fabricRuntime);        

    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        TraceId,
        "RuntimeTableObj.AddPending: ErrorCode={0}, FabricRuntime={1}", 
        error,
        static_cast<void*>(fabricRuntime.get())); 

    return error;
}

Common::ErrorCode NonActivatedApplicationHost::OnGetCodePackageActivationContext(
    CodePackageContext const & codeContext,
    __out CodePackageActivationContextSPtr &)
{
    Assert::CodingError(
        "NonActivatedApplicationHost::OnGetCodePackageActivationContext should never be called. CodePackage {0}",
        codeContext);
}

Common::ErrorCode NonActivatedApplicationHost::OnUpdateCodePackageContext(
    CodePackageContext const & codeContext)
{
    Assert::CodingError(
        "NonActivatedApplicationHost::OnUpdateCodePackageContext should never be called. CodePackage {0}",
        codeContext);
}
