// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;

StringLiteral const TraceType("SingleCodePackageApplicationHost");

// ********************************************************************************************************************
// SingleCodePackageApplicationHost Implementation
//
SingleCodePackageApplicationHost::SingleCodePackageApplicationHost(
    wstring const & hostId, 
    wstring const & runtimeServiceAddress,
    bool isContainerHost,
    Common::PCCertContext certContext,
    wstring const & serverThumbprint,
    CodePackageContext const & codeContext)
    : ApplicationHost(
        ApplicationHostContext(hostId, ApplicationHostType::Activated_SingleCodePackage, isContainerHost), 
        runtimeServiceAddress,
        certContext,
        serverThumbprint,
        SystemServiceApplicationNameHelper::IsSystemServiceApplicationName(codeContext.ApplicationName), // useSystemServiceSharedLogSettings
        nullptr), // KtlSystemBase
    codeContextLock_(),
    codeContext_(codeContext)
{
}

SingleCodePackageApplicationHost::~SingleCodePackageApplicationHost()
{
}

ErrorCode SingleCodePackageApplicationHost::Create(
    wstring const & hostId, 
    wstring const & runtimeServiceAddress,
    bool isContainerHost,
    PCCertContext certContext,
    wstring const & serverThumbprint,
    CodePackageContext const & codeContext,
    __out ApplicationHostSPtr & applicationHost)
{
    WriteNoise(
        TraceType,
        "Creating SingleCodePackageApplicationHost: HostId={0}, CodeContext={1}",
        hostId,        
        codeContext);

    applicationHost = make_shared<SingleCodePackageApplicationHost>(
        hostId,
        runtimeServiceAddress,
        isContainerHost,
        certContext,
        serverThumbprint,
        codeContext);

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode SingleCodePackageApplicationHost::OnCreateAndAddFabricRuntime(
    FabricRuntimeContextUPtr const & fabricRuntimeContextUPtr,    
    ComPointer<IFabricProcessExitHandler> const & fabricExitHandler,
    __out FabricRuntimeImplSPtr & fabricRuntime)
{
    if (fabricExitHandler.GetRawPointer() != NULL)
    {
        WriteNoise(
            TraceType,
            TraceId,
            "OnCreateFabricRuntime: ignoring non-NULL FabricProcessExitHandler for activated SingleCodePackageApplicationHost");
    }

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

    ErrorCode error = ErrorCodeValue::Success;    
    {
        AcquireReadLock grab(codeContextLock_);

        wstring runtimeId = Guid::NewGuid().ToString();
        FabricRuntimeContext fabricRuntimeContext(runtimeId, this->HostContext, codeContext_);
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
        "RuntimeTableObj.AddPending: ErrorCode={0}, FabricRuntime={1}", 
        error,
        static_cast<void*>(fabricRuntime.get())); 

    return error;
}

Common::ErrorCode SingleCodePackageApplicationHost::OnGetCodePackageActivationContext(
    CodePackageContext const & codeContext,
    __out CodePackageActivationContextSPtr & codePackageActivationContext)
{
    {
        AcquireReadLock grab(codeContextLock_);
        // Make sure the code package ids match. The versions do not need to match
        ASSERT_IFNOT(
            codeContext_.CodePackageInstanceId == codeContext.CodePackageInstanceId,
            "CodePackageId does not match. Expected {0}, Actual {1}",
            codeContext_,
            codeContext);
    }

    CodePackageActivationId activationId;
    bool isValid;
    auto error = FindCodePackage(
        codeContext.CodePackageInstanceId,
        activationId,
        codePackageActivationContext,
        isValid);

    ASSERT_IF(
        error.IsError(ErrorCodeValue::HostingCodePackageNotHosted), 
        "FindCodePackage in OnGetCodePackageActivationContext should not return HostingCodePackageNotHosted");

    ASSERT_IF(
        error.IsSuccess() && !isValid, 
        "FindCodePackage in OnGetCodePackageActivationContext should not return invalid codePackageActivationContext");

    return error;
}

AsyncOperationSPtr SingleCodePackageApplicationHost::OnBeginOpen(
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return ApplicationHost::OnBeginOpen(timeout, callback, parent);
}

ErrorCode SingleCodePackageApplicationHost::OnEndOpen(AsyncOperationSPtr const & asyncOperation)
{
    auto error = ApplicationHost::OnEndOpen(asyncOperation);

    {
        AcquireReadLock grab(codeContextLock_);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            TraceId,
            "ApplicationHost::OnEndOpen for SingleCodePackageApplicationHost({0}) completed with ErrorCode={1}",
            codeContext_,
            error);

        if(error.IsSuccess())
        {
            CodePackageActivationId activationId = CodePackageActivationId(this->HostContext.HostId);
            CodePackageActivationContextSPtr activationContext;

            FabricNodeContextSPtr nodeContext;
            error = GetFabricNodeContext(nodeContext);
            if(!error.IsSuccess())
            {
                WriteWarning(
                    TraceType,
                    TraceId,
                    "GetFabricNodeContext completed with ErrorCode={0}",
                    error);
                return error;
            }

            error = CodePackageActivationContext::CreateCodePackageActivationContext(
                codeContext_.CodePackageInstanceId,
                codeContext_.servicePackageVersionInstance.Version.ApplicationVersionValue.RolloutVersionValue,
                nodeContext->DeploymentDirectory,
                ipcHealthReportingClient_,
                nodeContext,
                this->HostContext.IsContainerHost,
                activationContext);

            WriteTrace(
                error.ToLogLevel(),
                TraceType,
                TraceId,
                "activationContext_->EnsureData for SingleCodePackageApplicationHost({0}) completed with ErrorCode={1}",
                codeContext_,
                error);

            if(error.IsSuccess())
            {
                error = AddCodePackage(
                    activationId,
                    activationContext,
                    true /*Validate*/);

                WriteTrace(
                    error.ToLogLevel(),
                    TraceType,
                    TraceId,
                    "AddCodePackage for SingleCodePackageApplicationHost({0}) completed with ErrorCode={1}",
                    codeContext_,
                    error);

                ASSERT_IF(
                    error.IsError(ErrorCodeValue::HostingCodePackageAlreadyHosted), 
                    "AddCodePackage in OnEndOpen should not return HostingCodePackageAlreadyHosted");
            }
        }
    }

    return error;
}

ErrorCode SingleCodePackageApplicationHost::OnUpdateCodePackageContext(
    CodePackageContext const & codeContext)
{
    AcquireWriteLock grab(codeContextLock_);
    codeContext_ = codeContext;
    return ErrorCode::Success();
}

