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


StringLiteral const TraceType("ApplicationHostProxy");

// ********************************************************************************************************************
// ApplicationHostProxy::UpdateCodePackageContextAsyncOperation Implementation
//
class ApplicationHostProxy::UpdateCodePackageContextAsyncOperation :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(UpdateCodePackageContextAsyncOperation)

public:
    UpdateCodePackageContextAsyncOperation(
        __in ApplicationHostProxy & owner,
        CodePackageContext const & codePackageContext,
        CodePackageActivationId const & codePackageActivationId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        codePackageContext_(codePackageContext),
        codePackageActivationId_(codePackageActivationId),
        timeoutHelper_(timeout)
    {
    }

    virtual ~UpdateCodePackageContextAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<UpdateCodePackageContextAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        RequestCodePackageContextUpdate(thisSPtr);
    }

private:
    void RequestCodePackageContextUpdate(AsyncOperationSPtr const & thisSPtr)
    {
        TimeSpan timeout = timeoutHelper_.GetRemainingTime();
        auto request = CreateUpdateCodePackageContextRequestMessage(timeout);
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(RequestCodePackageContextUpdate): HostId={0}, Request={1}, Timeout={2}",
            owner_.HostId,
            *request,
            timeout);
        auto operation = owner_.Hosting.IpcServerObj.BeginRequest(
            move(request),
            owner_.HostId,
            timeout,
            [this](AsyncOperationSPtr const & operation) { this->FinishCodePackageContextUpdate(operation, false); },
            thisSPtr);
        FinishCodePackageContextUpdate(operation, true);
    }

    void FinishCodePackageContextUpdate(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr reply;
        auto error = owner_.Hosting.IpcServerObj.EndRequest(operation, reply);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "End(RequestCodePackageContextUpdate): HostId={0}, ErrorCode={1}",
                owner_.HostId,
                error);

            TryComplete(operation->Parent, error);

            // abort the application host proxy, we do not know the status of the proxy
            owner_.Abort();
            return;
        }

        // process the reply
        UpdateCodePackageContextReply replyBody;
        if (!reply->GetBody<UpdateCodePackageContextReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "GetBody<UpdateCodePackageContextReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);
            TryComplete(operation->Parent, error);
            return;
        }

        error = replyBody.Error;
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "UpdateCodePackageContextReply: HostId={0}, Message={1}, Body={2}",
            owner_.HostId,
            *reply,
            replyBody);
        if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::HostingCodePackageNotHosted))
        {
            TryComplete(operation->Parent, error);
            return;
        }

        TryComplete(operation->Parent, ErrorCode(ErrorCodeValue::Success));
    }

    MessageUPtr CreateUpdateCodePackageContextRequestMessage(TimeSpan const timeout)
    {
        UpdateCodePackageContextRequest requestBody(codePackageContext_, codePackageActivationId_, timeout);

        MessageUPtr request = make_unique<Message>(requestBody);
        request->Headers.Add(Transport::ActorHeader(Actor::ApplicationHost));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::UpdateCodePackageContextRequest));

        WriteNoise(TraceType, owner_.TraceId, "CreateUpdateCodePackageContextRequest: Message={0}, Body={1}", *request, requestBody);

        return move(request);
    }

private:
    ApplicationHostProxy & owner_;
    CodePackageContext const codePackageContext_;
    CodePackageActivationId const codePackageActivationId_;
    TimeoutHelper const timeoutHelper_;
};

// ********************************************************************************************************************
// ApplicationHostProxy Implementation
//
AsyncOperationSPtr ApplicationHostProxy::BeginUpdateCodePackageContext(
    CodePackageContext const & codePackageContext,
    CodePackageActivationId const & codePackageActivationId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<UpdateCodePackageContextAsyncOperation>(
        *this,
        codePackageContext,
        codePackageActivationId,
        timeout,
        callback,
        parent);
}

ErrorCode ApplicationHostProxy::EndUpdateCodePackageContext(
    AsyncOperationSPtr const & operation)
{
    return UpdateCodePackageContextAsyncOperation::End(operation);
}

ErrorCode ApplicationHostProxy::Create(
    HostingSubsystemHolder const & hostingHolder,
    ApplicationHostIsolationContext const & isolationContext,
    CodePackageInstanceSPtr const & codePackageInstance,
    __out ApplicationHostProxySPtr & applicationHostProxy)
{
    ASSERT_IF(
        codePackageInstance->EntryPoint.EntryPointType == EntryPointType::None,
        "Cannot create application host for code package {0} with EntryPointType::None",
        codePackageInstance);

    auto hostId = Guid::NewGuid().ToString();

    if (codePackageInstance->CodePackageObj.IsImplicitTypeHost && 
        HostingConfig::GetConfig().HostGuestServiceTypeInProc)
    {
        applicationHostProxy = make_shared<InProcessApplicationHostProxy>(
            hostingHolder,
            hostId,
            isolationContext,
            codePackageInstance);

        WriteInfo(
            TraceType,
            hostingHolder.Root->TraceId,
            "ApplicationHostProxy::Create: Created InProcessApplicationHostProxy: HostId={0}, CodePackageInstanceId={1}.",
            hostId,
            codePackageInstance->CodePackageInstanceId);
    }
    else if (codePackageInstance->EntryPoint.EntryPointType == EntryPointType::Exe ||
             codePackageInstance->EntryPoint.EntryPointType == EntryPointType::ContainerHost)
    {
        applicationHostProxy = make_shared<SingleCodePackageApplicationHostProxy>(
            hostingHolder,
            hostId,
            isolationContext,
            codePackageInstance);

        WriteNoise(
            TraceType,
            hostingHolder.Root->TraceId,
            "ApplicationHostProxy::Create: Created SingleCodePackageApplicationHostProxy: HostId={0}, CodePackageInstanceId={1}.",
            hostId,
            codePackageInstance->CodePackageInstanceId);
    }
    else
    {
        applicationHostProxy = make_shared<MultiCodePackageApplicationHostProxy>(
            hostingHolder,
            hostId,
            isolationContext,
            codePackageInstance->RunAsId,
            codePackageInstance->CodePackageInstanceId.ServicePackageInstanceId);

        WriteNoise(
            TraceType,
            hostingHolder.Root->TraceId,
            "ApplicationHostProxy::Create: Created MultiCodePackageApplicationHostProxy: HostId={0}, ServicePackageInstanceId={1}.",
            hostId,
            codePackageInstance->CodePackageInstanceId.ServicePackageInstanceId);
    }

    return ErrorCode();
}

void ApplicationHostProxy::TerminateExternally()
{
    WriteNoise(
        TraceType,
        TraceId,
        "ApplicationHostProxy TerminateExternally for HostId {0}",
        this->HostId);

    Hosting.ApplicationHostManagerObj->FabricActivator->AbortProcess(this->HostId);
}

ApplicationHostProxy::ApplicationHostProxy(
    HostingSubsystemHolder const & hostingHolder,
    ApplicationHostContext const & context,
    ApplicationHostIsolationContext const & isolationContext,
    wstring const & runAsId,
    ServicePackageInstanceIdentifier const & servicePackageInstanceId,
    EntryPointType::Enum entryPointType)
    : ComponentRoot()
    , AsyncFabricComponent()
    , hostingHolder_(hostingHolder)
    , appHostContext_(context)
    , appHostIsolationContext_(isolationContext)
    , runAsId_(runAsId)
    , servicePackageInstanceId_(servicePackageInstanceId)
    , entryPointType_(entryPointType)
{
    this->SetTraceId(hostingHolder.Root->TraceId);
}

ApplicationHostProxy::~ApplicationHostProxy()
{
    WriteNoise(
        TraceType,
        TraceId,
        "ApplicationHostProxy.destructor: HostType={0}, HostId={1}, ServicePackageInstanceId={2}.",
        this->HostType,
        this->HostId,
        this->ServicePackageInstanceId);
}

ErrorCode ApplicationHostProxy::AddHostContextAndRuntimeConnection(EnvironmentMap & envMap)
{
    appHostContext_.ToEnvironmentMap(envMap);
	
	if (appHostContext_.IsContainerHost)
    {
        if (HostingConfig::GetConfig().FabricContainerAppsEnabled)
        {
            envMap[Constants::EnvironmentVariable::RuntimeConnectionAddress] = Hosting.RuntimeServiceAddressForContainers;
        }
        else
        {
            // Runtime address cannot be set since the container should not be allowed to connect back
            // if FabricContainerAppsEnabled is not set
            envMap[Constants::EnvironmentVariable::RuntimeConnectionAddress] = L"";
        }
    }
    else
    {
        envMap[Constants::EnvironmentVariable::RuntimeConnectionAddress] = Hosting.RuntimeServiceAddress;
    }

    // Temporary fix for FabricTest.
    // SQL Server: Defect 1461864: FabricTestHost should register IFabricCodePackageHost first to get NodeId
    envMap[Constants::EnvironmentVariable::NodeId] = Hosting.NodeId;
    envMap[Constants::EnvironmentVariable::NodeName] = Hosting.NodeName;

    ErrorCode error(ErrorCodeValue::Success);
    if (Hosting.IpcServerObj.IsTlsListenerEnabled())
    {
        error = AddTlsConnectionInformation(envMap);
    }

    return error;
}

ErrorCode ApplicationHostProxy::AddTlsConnectionInformation(__inout EnvironmentMap & envMap)
{
    envMap[Constants::EnvironmentVariable::RuntimeSslConnectionAddress] = Hosting.RuntimeServiceAddressForContainers;

#ifndef PLATFORM_UNIX
    SecureString certKey;
    CertContextUPtr cert;
    auto error = Hosting.IpcServerObj.SecuritySettingsTls.GenerateClientCert(certKey, cert);
    if (!error.IsSuccess()) { return error; }

    envMap[Constants::EnvironmentVariable::RuntimeSslConnectionCertKey] = certKey.GetPlaintext();
    envMap[Constants::EnvironmentVariable::RuntimeSslConnectionCertEncodedBytes] =  CryptoUtility::CertToBase64String(cert);

    wstring serverThumbprint;
    error = Hosting.IpcServerObj.SecuritySettingsTls.GetServerThumbprint(serverThumbprint);
    if (!error.IsSuccess()) { return error; }
    envMap[Constants::EnvironmentVariable::RuntimeSslConnectionCertThumbprint] = serverThumbprint;
#endif

    return  ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ApplicationHostProxy::GetCurrentProcessEnvironmentVariables(
    __in EnvironmentMap & envMap,
    wstring const & tempDirectory,
    wstring const & fabricBinFolder)
{
    if (this->runAsId_.empty())
    {
        if (!Environment::GetEnvironmentMap(envMap))
        {
            return ErrorCode(ErrorCodeValue::OperationFailed);
        }
    }
    // Set the temp folders
    envMap[L"TEMP"] = tempDirectory;
    envMap[L"TMP"] = tempDirectory;

    // Ensure PATH to fabric binaries
    auto pathIter = envMap.find(L"Path");
    wstring path = fabricBinFolder;
    if (pathIter != envMap.end())
    {
        path.append(L";");
        path.append(pathIter->second);
    }
    envMap[L"Path"] = path;

    wstring configStoreEnvVarName;
    wstring configStoreEnvVarValue;
    auto error = ComProxyConfigStore::FabricGetConfigStoreEnvironmentVariable(configStoreEnvVarName, configStoreEnvVarValue);
    if (!error.IsSuccess()) { return error; }

    if (!configStoreEnvVarName.empty())
    {
        envMap[configStoreEnvVarName] = configStoreEnvVarValue;
    }

    return ErrorCode(ErrorCodeValue::Success);
}
