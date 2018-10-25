// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Transport;
using namespace Hosting2;
using namespace ServiceModel;

StringLiteral const TraceType("MultiCodePackageApplicationHost");

// ********************************************************************************************************************
// MultiCodePackageApplicationHost::ComHostedCodePackageActivationContext Implementation
//
class MultiCodePackageApplicationHost::ComHostedCodePackageActivationContext : public ComCodePackageActivationContext
{
    DENY_COPY(ComHostedCodePackageActivationContext)

public:
    ComHostedCodePackageActivationContext(
        RootedObjectHolder<MultiCodePackageApplicationHost> const & mcpApplicationHostHolder,
        CodePackageActivationId const & activationId,
        CodePackageActivationContextSPtr const & codePackageActivationContextSPtr)
        : ComCodePackageActivationContext(codePackageActivationContextSPtr),
        activationId_(activationId),
        mcpApplicationHostHolder_(mcpApplicationHostHolder)
    {
    }

    virtual ~ComHostedCodePackageActivationContext()
    {
        mcpApplicationHostHolder_.RootedObject.OnHostedCodePackageTerminated(
            ActivationContext->CodePackageInstanceId,
            activationId_);
    }

private:
    CodePackageActivationId const activationId_;
    RootedObjectHolder<MultiCodePackageApplicationHost> const mcpApplicationHostHolder_;
};

// ********************************************************************************************************************
// MultiCodePackageApplicationHost::ActivateCodePackageRequestAsyncProcessor Implementation
//
class MultiCodePackageApplicationHost::ActivateCodePackageRequestAsyncProcessor :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(ActivateCodePackageRequestAsyncProcessor)

public:
    ActivateCodePackageRequestAsyncProcessor(
        __in MultiCodePackageApplicationHost & owner,
        __in ActivateCodePackageRequest && requestBody,
        __in IpcReceiverContextUPtr & receiverContext,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        requestBody_(move(requestBody)),
        receiverContext_(move(receiverContext)),
        timeoutHelper_(requestBody_.Timeout),
        activationContext_(),
        comFabricRuntime_()
    {
    }

    virtual ~ActivateCodePackageRequestAsyncProcessor()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ActivateCodePackageRequestAsyncProcessor>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Processing ActivateCodePackageRequest: HostId={0}, RequestBody={1}",
            owner_.Id,
            requestBody_);

        EnsureValidRequest(thisSPtr);
    }

private:
    void EnsureValidRequest(AsyncOperationSPtr const & thisSPtr)
    {
        CodePackageActivationId activationId;
        CodePackageActivationContextSPtr activationContext;
        bool isValid;
        auto error = owner_.FindCodePackage(
            requestBody_.CodePackageInstanceId,
            activationId,
            activationContext,
            isValid);
        if (error.IsSuccess())
        {
            // entry already found for this code package
            WriteNoise(
                TraceType,
                owner_.TraceId,
                "Found entry in the CodePackageTable. CodePackageInstanceId={0}, ActivationId={1}, IsValid={0}",
                requestBody_.CodePackageInstanceId,
                activationId,
                isValid);

            if (activationId == requestBody_.ActivationId)
            {
                // duplicate request - check if the activation has already been completed
                if (isValid)
                {
                    // send success response
                    SendReplyAndComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
                    return;
                }
                else
                {
                    // drop the request
                    WriteWarning(
                        TraceType,
                        owner_.TraceId,
                        "Dropping duplicate ActivateCodePackageRequest {0} as another activation is in progress.",
                        requestBody_);
                    TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::OperationCanceled));
                    return;
                }
            }
            else if (activationId < requestBody_.ActivationId)
            {
                // assert as this should not happen, we should not get an activation request
                // without Fabric confirming previous deactivation
                Assert::CodingError(
                    "Received an activation request for CodePackage {0} with higher instance id. Current ActivationId={1}, Received ActivationId={2}",
                    requestBody_.CodePackageInstanceId,
                    activationId,
                    requestBody_.ActivationId);
            }
            else
            {
                // previous activation id, drop the message
                WriteWarning(
                    TraceType,
                    owner_.TraceId,
                    "Dropping stale ActivateCodePackageRequest {0}.",
                    requestBody_);
                TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::OperationCanceled));
                return;
            }
        }
        else
        {
            // valid request
            CreateCodePackageActivationContext(thisSPtr);
        }
    }

    void CreateCodePackageActivationContext(AsyncOperationSPtr const & thisSPtr)
    {
        FabricNodeContextSPtr nodeContext;
        auto error = owner_.GetFabricNodeContext(nodeContext);
        if (!error.IsSuccess())
        {
            SendReplyAndComplete(thisSPtr, error);
            return;
        }

        error = CodePackageActivationContext::CreateCodePackageActivationContext(
            requestBody_.CodePackageInstanceId,
            requestBody_.CodeContext.servicePackageVersionInstance.Version.ApplicationVersionValue.RolloutVersionValue,
            nodeContext->DeploymentDirectory,
            owner_.ipcHealthReportingClient_,
            nodeContext,
            false, /*Container host not supported for this mode*/
            activationContext_);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "CreateCodePackageActivationContext: ErrorCode={0}, CodePackageInstanceId={1}, ActivationId={2}",
            error,
            requestBody_.CodePackageInstanceId,
            requestBody_.ActivationId);
        if (!error.IsSuccess())
        {
            SendReplyAndComplete(thisSPtr, error);
            return;
        }

        comActivationContext_ = make_com<MultiCodePackageApplicationHost::ComHostedCodePackageActivationContext>(
            RootedObjectHolder<MultiCodePackageApplicationHost>(owner_, owner_.CreateComponentRoot()),
            requestBody_.ActivationId,
            activationContext_);

        AddTentativeEntry(thisSPtr);
    }

    void AddTentativeEntry(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.AddCodePackage(requestBody_.ActivationId, activationContext_);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "AddTenatativeEntry: ErrorCode={0}, CodePackageInstanceId={1}, ActivationId={2}",
            error,
            requestBody_.CodePackageInstanceId,
            requestBody_.ActivationId);
        if (!error.IsSuccess())
        {
            if (error.IsError(ErrorCodeValue::HostingCodePackageAlreadyHosted))
            {
                TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::OperationCanceled));
                return;
            }
            else
            {
                this->SendReplyAndComplete(thisSPtr, error);
                return;
            }
        }

        CreateRuntimeToHostCodePackage(thisSPtr);
    }

    void CreateRuntimeToHostCodePackage(AsyncOperationSPtr const & thisSPtr)
    {
        TimeSpan timeout = timeoutHelper_.GetRemainingTime();

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(CreateComFabricRuntime): Timeout={0}, CodeContext={1}, ActivationId={2}",
            timeout,
            requestBody_.CodeContext,
            requestBody_.ActivationId);
        auto operation = owner_.BeginCreateComFabricRuntime(
            make_unique<FabricRuntimeContext>(owner_.HostContext, requestBody_.CodeContext),
            timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnCreateRuntimeToHostCodePackageCompleted(operation); },
            thisSPtr);
        if (operation->CompletedSynchronously)
        {
            FinishRuntimeToHostCodePackage(operation);
        }
    }

    void OnCreateRuntimeToHostCodePackageCompleted(AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            FinishRuntimeToHostCodePackage(operation);
        }
    }

    void FinishRuntimeToHostCodePackage(AsyncOperationSPtr const & operation)
    {
        auto error = owner_.EndCreateComFabricRuntime(operation, comFabricRuntime_);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "End(CreateComFabricRuntime): ErrorCode={0}, CodeContext={1}, ActivationId={2}",
            error,
            requestBody_.CodeContext,
            requestBody_.ActivationId);
        if (!error.IsSuccess())
        {
            CleanEntry();
            SendReplyAndComplete(operation->Parent, error);
            return;
        }

        ActivateWithinCodePackageHost(operation->Parent);
    }

    void ActivateWithinCodePackageHost(AsyncOperationSPtr const & thisSPtr)
    {
        ComPointer<IFabricRuntime> fabricRuntime(comFabricRuntime_, IID_IFabricRuntime);
        ComPointer<IFabricCodePackageActivationContext> activationContext(comActivationContext_, IID_IFabricCodePackageActivationContext);

        ASSERT_IF(fabricRuntime.GetRawPointer() == nullptr, "QueryInterface(IID_IFabricRuntime) failed on comFabricRuntime object.");
        ASSERT_IF(activationContext.GetRawPointer() == nullptr, "QueryInterface(IID_IFabricCodePackageActivationContext) failed on comActivationContext object.");

        // TODO: pass timeout to CodePackageHost
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(ActivateWithinCodePackageHost): CodeContext={0}, ActivationId={1}",
            requestBody_.CodeContext,
            requestBody_.ActivationId);

        auto operation = owner_.codePackageHostProxy_.BeginActivate(
            requestBody_.CodePackageInstanceId.ToString(),
            activationContext,
            fabricRuntime,
            [this](AsyncOperationSPtr const & operation) { this->OnActivateWithinCodePackageHostCompleted(operation); },
            thisSPtr);

        if (operation->CompletedSynchronously)
        {
            FinishActivateWithinCodePackageHost(operation);
        }
    }

    void OnActivateWithinCodePackageHostCompleted(AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            FinishActivateWithinCodePackageHost(operation);
        }
    }

    void FinishActivateWithinCodePackageHost(AsyncOperationSPtr const & operation)
    {
        auto error = owner_.codePackageHostProxy_.EndActivate(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "End(ActivateWithinCodePackageHost): ErrorCode={0}, CodeContext={1}, ActivationId={2}",
            error,
            requestBody_.CodeContext,
            requestBody_.ActivationId);
        if (!error.IsSuccess())
        {
            CleanEntry();
            SendReplyAndComplete(operation->Parent, error);
            return;
        }

        ValidateEntry(operation->Parent);
    }

    void ValidateEntry(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.ValidateCodePackage(
            requestBody_.CodePackageInstanceId,
            requestBody_.ActivationId);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "ValidateEntry: ErrorCode={0}, CodePackageInstanceId={1}, ActivationId={2}",
            error,
            requestBody_.CodePackageInstanceId,
            requestBody_.ActivationId);
        if (error.IsError(ErrorCodeValue::HostingCodePackageNotHosted))
        {
            Assert::CodingError(
                "ValidateEntry failed with HostingCodePackageNotHosted for CodePackageInstanceId={0}, ActivationId={1}",
                requestBody_.CodePackageInstanceId,
                requestBody_.ActivationId);
        }
        else
        {
            if (!error.IsSuccess())
            {
                CleanEntry();
            }
            SendReplyAndComplete(thisSPtr, error);
            return;
        }
    }

    void SendReplyAndComplete(AsyncOperationSPtr const & thisSPtr, ErrorCode const error)
    {
        ActivateCodePackageReply replyBody(error);
        MessageUPtr reply = make_unique<Message>(replyBody);
        receiverContext_->Reply(move(reply));
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Sent ActivateCodePackageReply: RequestBody={0}, ReplyBody={1}",
            requestBody_,
            replyBody);

        TryComplete(thisSPtr, error);
    }

    void CleanEntry()
    {
        bool isValid;
        auto error = owner_.RemoveCodePackage(requestBody_.CodePackageInstanceId, requestBody_.ActivationId, isValid);
        WriteWarning(
            TraceType,
            owner_.TraceId,
            "CleanEntry: ErrorCode={0}, CodePackageInstanceId={1}, ActivationId={2}",
            error,
            requestBody_.CodePackageInstanceId,
            requestBody_.ActivationId);
    }

private:
    MultiCodePackageApplicationHost & owner_;
    ActivateCodePackageRequest const requestBody_;
    IpcReceiverContextUPtr const receiverContext_;
    TimeoutHelper timeoutHelper_;
    CodePackageActivationContextSPtr activationContext_;
    ComPointer<MultiCodePackageApplicationHost::ComHostedCodePackageActivationContext> comActivationContext_;
    ComPointer<ComFabricRuntime> comFabricRuntime_;
};

// ********************************************************************************************************************
// MultiCodePackageApplicationHost::DeactivateCodePackageRequestAsyncProcessor Implementation
//
class MultiCodePackageApplicationHost::DeactivateCodePackageRequestAsyncProcessor :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(DeactivateCodePackageRequestAsyncProcessor)

public:
    DeactivateCodePackageRequestAsyncProcessor(
        __in MultiCodePackageApplicationHost & owner,
        __in DeactivateCodePackageRequest && requestBody,
        __in IpcReceiverContextUPtr & receiverContext,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        requestBody_(move(requestBody)),
        receiverContext_(move(receiverContext)),
        timeoutHelper_(requestBody_.Timeout)
    {
    }

    virtual ~DeactivateCodePackageRequestAsyncProcessor()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DeactivateCodePackageRequestAsyncProcessor>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Processing DeactivateCodePackageRequest: HostId={0}, RequestBody={1}",
            owner_.Id,
            requestBody_);

        RemoveEntry(thisSPtr);
    }

private:
    void RemoveEntry(AsyncOperationSPtr const & thisSPtr)
    {
        bool isValid;
        auto error = owner_.RemoveCodePackage(requestBody_.CodePackageInstanceId, requestBody_.ActivationId, isValid);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "RemoveEntry: ErrorCode={0}, CodePackageInstanceId={1}, ActivationId={2}",
            error,
            requestBody_.CodePackageInstanceId,
            requestBody_.ActivationId);

        ASSERT_IFNOT(
            isValid,
            "Deactivation Request for invalid HostedCodePackage should not be received. CodePackageInstanceId={0}, ActivationId={0}",
            requestBody_.CodePackageInstanceId,
            requestBody_.ActivationId);

        if (!error.IsSuccess())
        {
            SendReplyAndComplete(thisSPtr, error);
            return;
        }

        DeactivateWithinCodePackageHost(thisSPtr);
    }

    void DeactivateWithinCodePackageHost(AsyncOperationSPtr const & thisSPtr)
    {
        // TODO: use timeout
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(DeactivateWithinCodePackageHost): CodePackageInstanceId={0}, ActivationId={1}",
            requestBody_.CodePackageInstanceId,
            requestBody_.ActivationId);
        auto operation = owner_.codePackageHostProxy_.BeginDeactivate(
            requestBody_.CodePackageInstanceId.ToString(),
            [this](AsyncOperationSPtr const & operation) { this->OnDeactivateWithinCodePackageHostCompleted(operation); },
            thisSPtr);
        if (operation->CompletedSynchronously)
        {
            FinishDeactivateWithinCodePackageHost(operation);
        }
    }

    void OnDeactivateWithinCodePackageHostCompleted(AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            FinishDeactivateWithinCodePackageHost(operation);
        }
    }

    void FinishDeactivateWithinCodePackageHost(AsyncOperationSPtr const & operation)
    {
        auto error = owner_.codePackageHostProxy_.EndDeactivate(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.TraceId,
            "End(DeactivateWithinCodePackageHost): ErrorCode={0}, CodePackageInstanceId={1}, ActivationId={2}",
            error,
            requestBody_.CodePackageInstanceId,
            requestBody_.ActivationId);
        SendReplyAndComplete(operation->Parent, error);
    }

    void SendReplyAndComplete(AsyncOperationSPtr const & thisSPtr, ErrorCode const error)
    {
        DeactivateCodePackageReply replyBody(error);
        MessageUPtr reply = make_unique<Message>(replyBody);
        receiverContext_->Reply(move(reply));
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Sent DeactivateCodePackageReply: RequestBody={0}, ReplyBody={1}",
            requestBody_,
            replyBody);

        TryComplete(thisSPtr, error);
    }

private:
    MultiCodePackageApplicationHost & owner_;
    DeactivateCodePackageRequest const requestBody_;
    IpcReceiverContextUPtr const receiverContext_;
    TimeoutHelper timeoutHelper_;
};

// ********************************************************************************************************************
// MultiCodePackageApplicationHost::CodePackageTerminationAsyncHandler Implementation
//
class MultiCodePackageApplicationHost::CodePackageTerminationAsyncHandler :
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(CodePackageTerminationAsyncHandler)

public:
    CodePackageTerminationAsyncHandler(
        __in MultiCodePackageApplicationHost & owner,
        CodePackageInstanceIdentifier const & codePackageInstanceId,
        CodePackageActivationId const & activationId,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        codePackageInstanceId_(codePackageInstanceId),
        activationId_(activationId)
    {
    }

    virtual ~CodePackageTerminationAsyncHandler()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CodePackageTerminationAsyncHandler>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Handling Termination of CodePackage: HostId={0}, CodePackageInstanceId={1}, ActivationId={2}",
            owner_.Id,
            codePackageInstanceId_,
            activationId_);

        RemoveFromTable(thisSPtr);
    }

private:
    void RemoveFromTable(AsyncOperationSPtr const & thisSPtr)
    {
        bool isValid;
        auto error = owner_.RemoveCodePackage(codePackageInstanceId_, activationId_, isValid);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Id,
            "RemoveFromTable: ErrorCode={0}, CodePackageInstanceId={1}, ActivationId={2}",
            error,
            codePackageInstanceId_,
            activationId_);

        // ignore the error and send notification message to Fabric to remove
        SendTerminationNotification(thisSPtr);
    }

    void SendTerminationNotification(AsyncOperationSPtr const & thisSPtr)
    {
        MessageUPtr request = CreateCodePackageTerminationNotificationRequest();
        TimeSpan timeout = HostingConfig::GetConfig().RequestTimeout;

        WriteNoise(
            TraceType,
            owner_.TraceId,
            "Begin(CodePackageTerminationNotificationRequest): Timeout={0}, CodePackageId={1}, ActivationId={2}",
            timeout,
            codePackageInstanceId_,
            activationId_);
        auto operation = owner_.Client.BeginRequest(
            move(request),
            timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnSendTerminationNotificationReplyReceived(operation); },
            thisSPtr);
        if (operation->CompletedSynchronously)
        {
            FinishSendTerminationNotification(operation);
        }
    }

    void OnSendTerminationNotificationReplyReceived(AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            FinishSendTerminationNotification(operation);
        }
    }

    void FinishSendTerminationNotification(AsyncOperationSPtr const & operation)
    {
        MessageUPtr reply;
        auto error = owner_.Client.EndRequest(operation, reply);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Id,
            "End(CodePackageTerminationNotificationRequest): ErrorCode={0}, CodePackageId={1}, ActivationId={2}",
            error,
            codePackageInstanceId_,
            activationId_);
        if (!error.IsSuccess())
        {
            RetryIfNeeded(operation->Parent, error);
        }
        else
        {
            // no body for the reply, simple ack that Fabric has received and processed the notification
            TryComplete(operation->Parent, error);
        }
    }

    void RetryIfNeeded(AsyncOperationSPtr const & thisSPtr, ErrorCode const error)
    {
        if (error.IsError(ErrorCodeValue::Timeout))
        {
            WriteNoise(
                TraceType,
                owner_.TraceId,
                "Retrying CodePackageTerminationNotification : IPC ErrorCode={0}, CodePackageId={1}, ActivationId={2}",
                error,
                codePackageInstanceId_,
                activationId_);
            SendTerminationNotification(thisSPtr);
            return;
        }
        else
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "Not Retrying CodePackageTerminationNotification : IPC ErrorCode={0}, CodePackageId={1}, ActivationId={2}",
                error,
                codePackageInstanceId_,
                activationId_);
            TryComplete(thisSPtr, error);
            return;
        }
    }

    MessageUPtr CreateCodePackageTerminationNotificationRequest()
    {
        CodePackageTerminationNotificationRequest requestBody(codePackageInstanceId_, activationId_);

        MessageUPtr request = make_unique<Message>(requestBody);
        request->Headers.Add(Transport::ActorHeader(Actor::ApplicationHostManager));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::CodePackageTerminationNotificationRequest));

        WriteNoise(TraceType, owner_.TraceId, "CreateCodePackageTerminationNotificationRequest: Message={0}, Body={1}", *request, requestBody);
        return move(request);
    }


private:
    MultiCodePackageApplicationHost & owner_;
    CodePackageInstanceIdentifier const codePackageInstanceId_;
    CodePackageActivationId const activationId_;
};
// ********************************************************************************************************************
// MultiCodePackageApplicationHost Implementation
//
MultiCodePackageApplicationHost::MultiCodePackageApplicationHost(
    wstring const & hostId,
    wstring const & runtimeServiceAddress,
    Common::PCCertContext certContext,
    wstring const & serverThumbprint,
    ComPointer<IFabricCodePackageHost> codePackageHost)
    : ApplicationHost(
        ApplicationHostContext(
            hostId, 
            ApplicationHostType::Activated_MultiCodePackage,
            false /* isContainerHost */,
            false /* isCodePackageActivatorHost */),
        runtimeServiceAddress,
        certContext,
        serverThumbprint,
        false, // useSystemServiceSharedLogSettings
        nullptr), // KtlSystemBase
    codePackageHostProxy_(codePackageHost)
{
}

MultiCodePackageApplicationHost::~MultiCodePackageApplicationHost()
{
}

ErrorCode MultiCodePackageApplicationHost::OnCreateAndAddFabricRuntime(
    FabricRuntimeContextUPtr const & fabricRuntimeContextUPtr,
    ComPointer<IFabricProcessExitHandler> const & fabricExitHandler,
    __out FabricRuntimeImplSPtr & fabricRuntime)
{
    if (fabricExitHandler.GetRawPointer() != NULL)
    {
        auto error = ErrorCode::FromHResult(E_INVALIDARG);
        WriteWarning(
            TraceType,
            TraceId,
            "OnCreateFabricRuntime: fabricExitHandler must be NULL. ErrorCode={0}",
            error);
        return error;
    }

    if (!fabricRuntimeContextUPtr)
    {
        auto error = ErrorCode::FromHResult(E_INVALIDARG);
        WriteWarning(
            TraceType,
            TraceId,
            "OnCreateFabricRuntime: FabricRuntimeContext must be supplied. ErrorCode={0}",
            error);
        return error;
    }

    fabricRuntime = make_shared<FabricRuntimeImpl>(
        *this, /* ComponentRoot & */
        *this, /* ApplicationHost & */
        move(*fabricRuntimeContextUPtr),
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

ErrorCode MultiCodePackageApplicationHost::Create(
    wstring const & hostId,
    wstring const & runtimeServiceAddress,
    PCCertContext certContext,
    wstring const & serverThumbprint,
    ComPointer<IFabricCodePackageHost> codePackageHost,
    __out ApplicationHostSPtr & host)
{
    host = make_shared<MultiCodePackageApplicationHost>(
        hostId,
        runtimeServiceAddress,
        certContext,
        serverThumbprint,
        codePackageHost);

    return ErrorCode(ErrorCodeValue::Success);
}

void MultiCodePackageApplicationHost::ProcessIpcMessage(
    __in Message & message,
    __in IpcReceiverContextUPtr & context)
{
    if (message.Action == Hosting2::Protocol::Actions::ActivateCodePackageRequest)
    {
        ProcessActivateCodePackageRequest(message, context);
        return;
    }
    else if (message.Action == Hosting2::Protocol::Actions::DeactivateCodePackageRequest)
    {
        ProcessDeactivateCodePackageRequest(message, context);
        return;
    }
    else
    {
        ApplicationHost::ProcessIpcMessage(message, context);
    }
}

void MultiCodePackageApplicationHost::ProcessActivateCodePackageRequest(
    Message & message,
    __in IpcReceiverContextUPtr & context)
{
    ActivateCodePackageRequest requestBody;
    if (!message.GetBody<ActivateCodePackageRequest>(requestBody))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType,
            TraceId,
            "GetBody<ActivateCodePackageRequest> failed: Message={0}, ErrorCode={1}",
            message,
            error);
        return;
    }

    auto operation = AsyncOperation::CreateAndStart<ActivateCodePackageRequestAsyncProcessor>(
        *this,
        move(requestBody),
        context,
        [this](AsyncOperationSPtr const & operation) { this->OnProcessActivateCodePackageRequestCompleted(operation); },
        this->CreateAsyncOperationRoot());
    if (operation->CompletedSynchronously)
    {
        FinishProcessActivateCodePackageRequest(operation);
    }
}

void MultiCodePackageApplicationHost::OnProcessActivateCodePackageRequestCompleted(
    AsyncOperationSPtr const & operation)
{
    if (!operation->CompletedSynchronously)
    {
        FinishProcessActivateCodePackageRequest(operation);
    }
}

void MultiCodePackageApplicationHost::FinishProcessActivateCodePackageRequest(
    AsyncOperationSPtr const & operation)
{
    auto error = ActivateCodePackageRequestAsyncProcessor::End(operation);
    error.ReadValue();
}

void MultiCodePackageApplicationHost::ProcessDeactivateCodePackageRequest(
    Message & message,
    __in IpcReceiverContextUPtr & context)
{
    DeactivateCodePackageRequest requestBody;
    if (!message.GetBody<DeactivateCodePackageRequest>(requestBody))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType,
            TraceId,
            "GetBody<DeactivateCodePackageRequest> failed: Message={0}, ErrorCode={1}",
            message,
            error);
        return;
    }

    auto operation = AsyncOperation::CreateAndStart<DeactivateCodePackageRequestAsyncProcessor>(
        *this,
        move(requestBody),
        context,
        [this](AsyncOperationSPtr const & operation) { this->OnProcessDeactivateCodePackageRequestCompleted(operation); },
        this->CreateAsyncOperationRoot());
    if (operation->CompletedSynchronously)
    {
        FinishProcessDeactivateCodePackageRequest(operation);
    }
}

void MultiCodePackageApplicationHost::OnProcessDeactivateCodePackageRequestCompleted(
    AsyncOperationSPtr const & operation)
{
    if (!operation->CompletedSynchronously)
    {
        FinishProcessDeactivateCodePackageRequest(operation);
    }
}

void MultiCodePackageApplicationHost::FinishProcessDeactivateCodePackageRequest(
    AsyncOperationSPtr const & operation)
{
    auto error = DeactivateCodePackageRequestAsyncProcessor::End(operation);
    error.ReadValue();
}

void MultiCodePackageApplicationHost::OnHostedCodePackageTerminated(
    CodePackageInstanceIdentifier const & codePackageInstanceId,
    CodePackageActivationId const & activationId)
{
    auto operation = AsyncOperation::CreateAndStart<CodePackageTerminationAsyncHandler>(
        *this,
        codePackageInstanceId,
        activationId,
        [this](AsyncOperationSPtr const & operation) { this->OnCodePackageTerminationHandled(operation); },
        this->CreateAsyncOperationRoot());
    if (operation->CompletedSynchronously)
    {
        FinishCodePackageTerminationHandler(operation);
    }
}

void MultiCodePackageApplicationHost::OnCodePackageTerminationHandled(
    AsyncOperationSPtr const & operation)
{
    if (!operation->CompletedSynchronously)
    {
        FinishCodePackageTerminationHandler(operation);
    }
}

void MultiCodePackageApplicationHost::FinishCodePackageTerminationHandler(
    AsyncOperationSPtr const & operation)
{
    auto error = CodePackageTerminationAsyncHandler::End(operation);
    error.ReadValue();
}

Common::ErrorCode MultiCodePackageApplicationHost::OnGetCodePackageActivationContext(
    CodePackageContext const & codeContext,
    __out CodePackageActivationContextSPtr &)
{
    Assert::CodingError(
        "MultiCodePackageApplicationHost::OnGetCodePackageActivationContext should never be called. CodePackage {0}",
        codeContext);
}

Common::ErrorCode MultiCodePackageApplicationHost::OnUpdateCodePackageContext(
    CodePackageContext const & codeContext)
{
    UNREFERENCED_PARAMETER(codeContext);
    return ErrorCode::Success();
}
