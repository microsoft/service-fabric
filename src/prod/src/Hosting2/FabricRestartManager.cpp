// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;
using namespace Transport;

const StringLiteral TraceType_FabricRestartManager("FabricRestartManager");

// ********************************************************************************************************************
// FabricRestartManager::OpenAsyncOperation Implementation
//
class FabricRestartManager::OpenAsyncOperation : public AsyncOperation
{
    DENY_COPY(OpenAsyncOperation)

public:
    OpenAsyncOperation(
        FabricRestartManager & restartManager,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & asyncOperationParent)
        : AsyncOperation(callback, asyncOperationParent),
        restartManager_(restartManager),
        timeoutHelper_(timeout)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const& operation)
    {
        auto thisPtr = AsyncOperation::End<OpenAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        restartManager_.StartPollingAsync();
        restartManager_.RegisterIpcRequestHandler();
        TryComplete(thisSPtr, ErrorCodeValue::Success);
    }

private:		
    FabricRestartManager & restartManager_;
    TimeoutHelper timeoutHelper_;
};

// ********************************************************************************************************************
// FabricRestartManager::CloseAsyncOperation Implementation
//
class FabricRestartManager::CloseAsyncOperation : public AsyncOperation
{
    DENY_COPY(CloseAsyncOperation)

public:
    CloseAsyncOperation(
        FabricRestartManager & restartManager,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & asyncOperationParent)
        : restartManager_(restartManager),
        timeoutHelper_(timeout),
        AsyncOperation(callback, asyncOperationParent)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const& operation)
    {
        auto thisPtr = AsyncOperation::End<CloseAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        restartManager_.StopPolling();
        restartManager_.UnregisterIpcRequestHandler();
        TryComplete(thisSPtr, ErrorCodeValue::Success);
    }

private:
    FabricRestartManager & restartManager_;
    TimeoutHelper timeoutHelper_;
};

// ********************************************************************************************************************
// FabricRestartManager::DisableNodeAsyncOperation Implementation
//
class FabricRestartManager::DisableNodeAsyncOperation : public AsyncOperation
{
    DENY_COPY(DisableNodeAsyncOperation)

public:
    DisableNodeAsyncOperation(
        FabricRestartManager & restartManager,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & asyncOperationParent)
        : AsyncOperation(callback, asyncOperationParent),
        restartManager_(restartManager),
        timeoutHelper_(timeout)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const& operation)
    {
        auto thisPtr = AsyncOperation::End<DisableNodeAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        restartManager_.externalRebootPending_ = true;
        TimeSpan timeout = timeoutHelper_.GetRemainingTime();
        WriteInfo(
            TraceType_FabricRestartManager,
            restartManager_.Root.TraceId,
            "FabricRestartManager: DisableNodeAsyncOperation waiting for node to disable {0}", 
            timeout);
        bool done = restartManager_.nodeDisabled_.WaitOne(timeout);
        if (!done)
        {
            WriteError(
                TraceType_FabricRestartManager,
                restartManager_.Root.TraceId,
                "DisableNodeAsyncOperation: Didn't complete within given time");
            TryComplete(thisSPtr, ErrorCodeValue::Timeout);
        }
        else
        {
            WriteInfo(
                TraceType_FabricRestartManager,
                restartManager_.Root.TraceId,
                "DisableNodeAsyncOperation: completed");
            TryComplete(thisSPtr, ErrorCodeValue::Success);
        }
    }

private:
    FabricRestartManager & restartManager_;
    TimeoutHelper timeoutHelper_;
};

FabricRestartManager::FabricRestartManager(
    Common::ComponentRoot const & root,
    FabricActivationManager const & fabricHost) :
    RootedObject(root),
    fabricHost_(fabricHost),
    runCompleted_(true),
    runShouldExit_(false),
    runStarted_(false),
    lock_(),
    callbackAddress_(),
    rebootPending_(false),
    externalRebootPending_(false)
{
}

FabricRestartManager::~FabricRestartManager()
{
    WriteInfo(TraceType_FabricRestartManager, Root.TraceId, "destructor called");
}

AsyncOperationSPtr FabricRestartManager::OnBeginOpen(
    Common::TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<OpenAsyncOperation>(*this, timeout, callback, parent);
}

void FabricRestartManager::RegisterIpcRequestHandler()
{
    auto root = this->Root.CreateComponentRoot();
    this->fabricHost_.IpcServerObj.RegisterMessageHandler(
        Actor::RestartManager,
        [this, root](MessageUPtr & message, IpcReceiverContextUPtr & context) { this->ProcessIpcMessage(*message, context); },
        false/*dispatchOnTransportThread*/);
}

void FabricRestartManager::UnregisterIpcRequestHandler()
{
    this->fabricHost_.IpcServerObj.UnregisterMessageHandler(Actor::RestartManager);
}

void FabricRestartManager::ProcessIpcMessage(__in Message & message, __in IpcReceiverContextUPtr & context)
{
    wstring const & action = message.Action;
    WriteNoise(TraceType_FabricRestartManager,
        Root.TraceId,
        "Processing Ipc message with action {0}",
        action);

    if (action == Hosting2::Protocol::Actions::NodeDisabledNotification)
    {
        this->ProcessNodeDisabledNotification(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::NodeEnabledNotification)
    {
        this->ProcessNodeEnabledNotification(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::RegisterFabricActivatorClient || 
        action == Hosting2::Protocol::Actions::UnregisterFabricActivatorClient)
    {
        this->callbackAddress_ = action == Hosting2::Protocol::Actions::RegisterFabricActivatorClient ? context->From : L"";
        unique_ptr<RegisterFabricActivatorClientReply> replyBody;
        replyBody = make_unique<RegisterFabricActivatorClientReply>(ErrorCodeValue::Success);
        MessageUPtr reply = make_unique<Message>(*replyBody);
        WriteNoise(TraceType_FabricRestartManager,
            Root.TraceId,
            "Sending RegisterFabricActivatorClient: Error ={0}",
            ErrorCodeValue::Success);
        context->Reply(move(reply));
    }
    else
    {
        WriteWarning(
            TraceType_FabricRestartManager,
            Root.TraceId,
            "Dropping unsupported message: {0}",
            message);
    }
}

void FabricRestartManager::ProcessNodeEnabledNotification(__in Transport::Message & message, __in Transport::IpcReceiverContextUPtr & context)
{
    NodeEnabledNotification enableNotification;
    if (!message.GetBody<NodeEnabledNotification>(enableNotification))
    {
        WriteWarning(
            TraceType_FabricRestartManager,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }

    WriteInfo(
        TraceType_FabricRestartManager,
        Root.TraceId,
        "ProcessNodeEnabledNotification: {0}",
        message);
    this->nodeEnabled_.Set();

    //Just return message to ack reception
    MessageUPtr response = make_unique<Message>(enableNotification);
    context->Reply(move(response));
}

void FabricRestartManager::ProcessNodeDisabledNotification(__in Transport::Message & message, __in Transport::IpcReceiverContextUPtr & context)
{
    NodeDisabledNotification disableNotification;
    if (!message.GetBody<NodeDisabledNotification>(disableNotification))
    {
        WriteWarning(
            TraceType_FabricRestartManager,
            Root.TraceId,
            "Invalid message received: {0}, dropping",
            message);
        return;
    }

    WriteInfo(
        TraceType_FabricRestartManager,
        Root.TraceId,
        "ProcessNodeDisabledNotification: {0}",
        message);
    this->nodeDisabled_.Set();

    //Just return message to ack reception
    MessageUPtr response = make_unique<Message>(disableNotification);
    context->Reply(move(response));
}

ErrorCode FabricRestartManager::OnEndOpen(AsyncOperationSPtr const & asyncOperation)
{
    return OpenAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr FabricRestartManager::OnBeginClose(
    Common::TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(*this, timeout, callback, parent);
}

ErrorCode FabricRestartManager::OnEndClose(AsyncOperationSPtr const & asyncOperation)
{
    return CloseAsyncOperation::End(asyncOperation);
}

void FabricRestartManager::StartPollingAsync()
{
    AcquireExclusiveLock lock(lock_);

    if (!runCompleted_.IsSet())
    {
        return;
    }

    runCompleted_.Reset();
    runShouldExit_.Reset();
    runStarted_.Reset();

    Threadpool::Post([this]()
    {
        this->Run();
    });

    runStarted_.WaitOne(INFINITE);
}

AsyncOperationSPtr FabricRestartManager::BeginNodeDisable(TimeSpan timeout, AsyncCallback const & callback, AsyncOperationSPtr const & parent)
{
    WriteInfo(TraceType_FabricRestartManager, Root.TraceId, "FabricRestartManager: BeginNodeDisable called {0}", timeout);
    return AsyncOperation::CreateAndStart<DisableNodeAsyncOperation>(*this, timeout, callback, parent);
}

ErrorCode FabricRestartManager::EndNodeDisable(AsyncOperationSPtr const & asyncOperation)
{
    WriteInfo(TraceType_FabricRestartManager, Root.TraceId, "FabricRestartManager: EndNodeDisable");
    return DisableNodeAsyncOperation::End(asyncOperation);
}

void FabricRestartManager::StopPolling()
{
    AcquireExclusiveLock lock(lock_);

    if (!runCompleted_.IsSet())
    {
        runShouldExit_.Set();
        runCompleted_.WaitOne(INFINITE);
    }
}

void FabricRestartManager::Run()
{
#if !defined(PLATFORM_UNIX)
    runStarted_.Set();
    bool checkForRestartTriggeredDone = false;
    while (!runShouldExit_.IsSet())
    {
        if (!this->callbackAddress_.empty())
        {
            if (!checkForRestartTriggeredDone)
            {
                this->nodeEnabled_.Reset();
                this->SendNodeEnableRequest();

                if (this->nodeEnabled_.WaitOne(60000))
                {
                    WriteInfo(TraceType_FabricRestartManager, Root.TraceId, "FabricRestartManager: Node enabled");
                    checkForRestartTriggeredDone = true;
                }
            }

            if (checkForRestartTriggeredDone && this->IfRebootIsPending())
            {
                WriteInfo(TraceType_FabricRestartManager, Root.TraceId, "FabricRestartManager: Node disable");

                // Send Message to get deactivated and then wait for it.
                // This is done if node is not disabled
                if (!this->nodeDisabled_.IsSet())
                {
                    WriteInfo(TraceType_FabricRestartManager, Root.TraceId, "FabricRestartManager: Node disable Not set");
                    this->nodeDisabled_.Reset();
                    this->SendNodeDisableRequest();

                    // Disable node request sent, mark reboot pending as true now
                    this->rebootPending_ = true;
                }

                if (this->nodeDisabled_.WaitOne(120000))
                {
                    WriteInfo(TraceType_FabricRestartManager, Root.TraceId, "FabricRestartManager: Node disabled");
                    if (!this->externalRebootPending_)
                    {
                        HANDLE hToken;
                        TOKEN_PRIVILEGES tkp;

                        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
                        {
                            WriteError(TraceType_FabricRestartManager, Root.TraceId, "FabricRestartManager: Could not open process token");
                        }
                        else
                        {
                            LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
                            tkp.PrivilegeCount = 1;  // one privilege to set    
                            tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

                            // Get the shutdown privilege for this process. 
                            AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
                            DWORD error = GetLastError();
                            if (error != ERROR_SUCCESS)
                            {
                                WriteInfo(TraceType_FabricRestartManager, Root.TraceId, "FabricRestartManager: AdjustTokenPrivileges returned error {0}", error);
                            }
                            else
                            {
                                WriteInfo(TraceType_FabricRestartManager, Root.TraceId, "FabricRestartManager: Initiating shutdown");
                                error = InitiateSystemShutdownEx
                                    (
                                    NULL,
                                    L"Shuting down the machine",
                                    0,
                                    TRUE,
                                    TRUE,
                                    SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_UPGRADE | SHTDN_REASON_FLAG_PLANNED);
                            }
                        }
                    }
                }
            }
        }

        runShouldExit_.WaitOne(10000);
    }

    runCompleted_.Set();
#endif
}

void FabricRestartManager::SendNodeEnableRequest()
{
    EnableNodeRequest enableNodeRequest(L"Restart");
    MessageUPtr request = make_unique<Message>(enableNodeRequest);
    request->Headers.Add(Transport::ActorHeader(Actor::RestartManagerClient));
    request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::EnableNodeRequest));

    WriteNoise(TraceType_FabricRestartManager, Root.TraceId, "DisableNodeRequest: Message={0}, Body={1}", *request, enableNodeRequest);

    auto operation = this->fabricHost_.IpcServerObj.BeginRequest(
        move(request),
        this->callbackAddress_,
        HostingConfig::GetConfig().RequestTimeout,
        [this](AsyncOperationSPtr const & operation)
    {
        FinishSendEnableNodeRequestMessage(operation, false);
    },
        NULL);

    FinishSendEnableNodeRequestMessage(operation, true);
}

void FabricRestartManager::SendNodeDisableRequest()
{
    DisableNodeRequest disableNodeRequest(L"Restart");
    MessageUPtr request = make_unique<Message>(disableNodeRequest);
    request->Headers.Add(Transport::ActorHeader(Actor::RestartManagerClient));
    request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::DisableNodeRequest));

    WriteNoise(TraceType_FabricRestartManager, Root.TraceId, "DisableNodeRequest: Message={0}, Body={1}", *request, disableNodeRequest);

    auto operation = this->fabricHost_.IpcServerObj.BeginRequest(
        move(request),
        this->callbackAddress_,
        HostingConfig::GetConfig().RequestTimeout,
        [this](AsyncOperationSPtr const & operation)
    { 
        FinishSendDisableNodeRequestMessage(operation, false); 
    },
        NULL);

    FinishSendDisableNodeRequestMessage(operation, true);
}

void FabricRestartManager::FinishSendEnableNodeRequestMessage(AsyncOperationSPtr operation, bool expectedCompletedAsynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedAsynchronously)
    {
        return;
    }

    MessageUPtr reply;
    auto error = this->fabricHost_.IpcServerObj.EndRequest(operation, reply);
    if (!error.IsSuccess())
    {
        WriteInfo(TraceType_FabricRestartManager,
            Root.TraceId,
            "Send request for enabling node...Retrying");
        SendNodeEnableRequest();
    }
}

void FabricRestartManager::FinishSendDisableNodeRequestMessage(AsyncOperationSPtr operation, bool expectedCompletedAsynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedAsynchronously)
    {
        return;
    }

    MessageUPtr reply;
    auto error = this->fabricHost_.IpcServerObj.EndRequest(operation, reply);
    if (!error.IsSuccess())
    {
        WriteInfo(TraceType_FabricRestartManager,
            Root.TraceId,
            "Send request for disabling node...Retrying");
        SendNodeDisableRequest();
    }
}

bool FabricRestartManager::IfNodeNeedToBeEnabled()
{
    RegistryKey windowsfabricKey(L"SOFTWARE\\Microsoft\\Windows Fabric", true, true);
    if (windowsfabricKey.IsValid)
    {
        DWORD value;
        if (windowsfabricKey.GetValue(L"RestartTriggered", value))
        {
            return value == 1;
        }
    }

    return FALSE;
}

bool FabricRestartManager::IfRebootIsPending()
{
    if (this->rebootPending_ == TRUE || this->externalRebootPending_ == TRUE)
    {
        return true;
    }

    RegistryKey autoUpdateRebootPending(
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\Auto Update\\RebootRequired",
        true,
        true);

    if (autoUpdateRebootPending.IsValid)
    {
        Trace.WriteInfo(TraceType_FabricRestartManager,
            Root.TraceId,
            "autoUpdateRebootPending is valid");
        return TRUE;
    }

    RegistryKey cbsRebootPending(
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Component Based Servicing\\RebootPending",
        true,
        true);

    if (cbsRebootPending.IsValid)
    {
        Trace.WriteInfo(TraceType_FabricRestartManager,
            Root.TraceId,
            "cbsRebootPending is valid");
        return TRUE;
    }

    return FALSE;
}

void FabricRestartManager::OnAbort()
{
    Trace.WriteInfo(TraceType_FabricRestartManager,
        Root.TraceId,
        "Aborting FabricRestartManager");

    this->UnregisterIpcRequestHandler();
}
