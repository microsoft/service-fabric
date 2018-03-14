// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

StringLiteral const TraceType("RuntimeSharingHelper");

IRuntimeSharingHelperUPtr Hosting2::RuntimeSharingHelperFactory(RuntimeSharingHelperConstructorParameters & parameters)
{
    return make_unique<RuntimeSharingHelper>(parameters);
}


// ********************************************************************************************************************
// RuntimeSharingHelper::OpenAsyncOperation Implementation
//
class RuntimeSharingHelper::OpenAsyncOperation : 
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(OpenAsyncOperation)

public:
    OpenAsyncOperation(
        RuntimeSharingHelper & owner,
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
        WriteInfo(
            TraceType,
            owner_.Root.TraceId,
            "Opening RuntimeSharingHelper: Timeout={0}",
            timeoutHelper_.GetRemainingTime());

        CreateApplicationHost(thisSPtr);
    }

private:
    void CreateApplicationHost(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = NonActivatedApplicationHost::Create2(owner_.ktlSystem_, owner_.runtimeServiceAddress_, Constants::NativeSystemServiceHostId, owner_.host_);

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.Root.TraceId,
            "CreateApplicationHost: RuntimeServiceAddress={0}, ErrorCode={1}",
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
        WriteInfo(
            TraceType, 
            owner_.Root.TraceId, 
            "Begin(OpenApplicationHost): Timeout={0}", 
            timeoutHelper_.GetRemainingTime());
        auto operation = owner_.host_->BeginOpen(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation){ this->OnApplicationHostOpened(operation); },
            thisSPtr);
        if (operation->CompletedSynchronously)
        {
            FinishApplicationHostOpen(operation);
        }
    }

    void OnApplicationHostOpened(AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            FinishApplicationHostOpen(operation);
        }
    }

    void FinishApplicationHostOpen(AsyncOperationSPtr const & operation)
    {
        auto error = owner_.host_->EndOpen(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType, 
            owner_.Root.TraceId, 
            "End(OpenApplicationHost): ErrorCode={0}",
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
            owner_.Root.TraceId, 
            "Begin(CreateComFabricRuntime): Timeout={0}", 
            timeoutHelper_.GetRemainingTime());
        auto operation = owner_.host_->BeginCreateComFabricRuntime(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation){ this->OnComFabricCreated(operation); },
            thisSPtr);
        if (operation->CompletedSynchronously)
        {
            FinishCreateComFabricRuntime(operation);
        }
    }

    void OnComFabricCreated(AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            FinishCreateComFabricRuntime(operation);
        }
    }

    void FinishCreateComFabricRuntime(AsyncOperationSPtr const & operation)
    {
        auto error = owner_.host_->EndCreateComFabricRuntime(operation, owner_.runtime_);
        WriteTrace(
            error.ToLogLevel(),
            TraceType, 
            owner_.Root.TraceId, 
            "End(CreateComFabricRuntime): ErrorCode={0}",
            error);

        TryComplete(operation->Parent, error);
        return;
    }

private:
    RuntimeSharingHelper & owner_;
    TimeoutHelper timeoutHelper_;
};


// ********************************************************************************************************************
// RuntimeSharingHelper::CloseAsyncOperation Implementation
//
class RuntimeSharingHelper::CloseAsyncOperation : 
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(CloseAsyncOperation)

public:
    CloseAsyncOperation(
        RuntimeSharingHelper & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout)
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
            owner_.Root.TraceId,
            "Closing RuntimeSharingHelper: Timeout={0}",
            timeoutHelper_.GetRemainingTime());

       
        CloseApplicationHost(thisSPtr);
    }

private:
    void CloseApplicationHost(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
            TraceType, 
            owner_.Root.TraceId, 
            "Begin(CloseApplicationHost): Timeout={0}", 
            timeoutHelper_.GetRemainingTime());
        auto operation = owner_.host_->BeginClose(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation){ this->OnApplicationHostClosed(operation); },
            thisSPtr);
        if (operation->CompletedSynchronously)
        {
            FinishCloseApplicationHost(operation);
        }
    }

    void OnApplicationHostClosed(AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            FinishCloseApplicationHost(operation);
        }
    }

    void FinishCloseApplicationHost(AsyncOperationSPtr const & operation)
    {
        auto error = owner_.host_->EndClose(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType, 
            owner_.Root.TraceId, 
            "End(CloseApplicationHost): ErrorCode={0}",
            error);
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }

        CloseRuntime(operation->Parent);
    }

    void CloseRuntime(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
            TraceType, 
            owner_.Root.TraceId, 
            "Releasing runtime");
        owner_.runtime_.Release();
        
        WriteInfo(
            TraceType, 
            owner_.Root.TraceId, 
            "RuntimeSharingHelper closed.");
        TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
        return;
    }

private:
    RuntimeSharingHelper & owner_;
    TimeoutHelper timeoutHelper_;
};

// ********************************************************************************************************************
// RuntimeSharingHelper Implementation
//
RuntimeSharingHelper::RuntimeSharingHelper(RuntimeSharingHelperConstructorParameters & parameters)    
    : RootedObject(*parameters.Root),
    ktlSystem_(&parameters.KtlLogger->KtlSystemObject),
    runtimeServiceAddress_(),
    host_(),
    runtime_()
{
}

RuntimeSharingHelper::~RuntimeSharingHelper()
{
}

ComPointer<IFabricRuntime> RuntimeSharingHelper::GetRuntime()
{
    ComPointer<IFabricRuntime> fabricRuntime = ComPointer<IFabricRuntime>(runtime_, IID_IFabricRuntime);
    ASSERT_IF(!fabricRuntime, "QueryInterface(IID_IFabricRuntime) failed on ComFabricRuntime.");

    return fabricRuntime;
} 

Transport::IpcClient & RuntimeSharingHelper::GetIpcClient() { return runtime_->Runtime->Host.Client; }

AsyncOperationSPtr RuntimeSharingHelper::BeginOpen(
    std::wstring const & runtimeServiceAddress,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    this->runtimeServiceAddress_ = runtimeServiceAddress;
    return AsyncFabricComponent::BeginOpen(timeout, callback, parent);
}

ErrorCode RuntimeSharingHelper::EndOpen(AsyncOperationSPtr const & asyncOperation)
{
    return AsyncFabricComponent::EndOpen(asyncOperation);
}

AsyncOperationSPtr RuntimeSharingHelper::OnBeginOpen(
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

ErrorCode RuntimeSharingHelper::OnEndOpen(AsyncOperationSPtr const & asyncOperation)
{
    return OpenAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr RuntimeSharingHelper::OnBeginClose(
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

ErrorCode RuntimeSharingHelper::OnEndClose(AsyncOperationSPtr const & asyncOperation)
{
    return CloseAsyncOperation::End(asyncOperation);
}

void RuntimeSharingHelper::OnAbort()
{
    WriteInfo(
        TraceType,
        Root.TraceId,
        "Aborting RuntimeSharingHelper");
    
    if (host_)
    {
        host_->Abort();
    }
    if (runtime_)
    {
        runtime_.Release();
    }

    WriteInfo(
        TraceType,
        Root.TraceId,
        "RuntimeSharingHelper Aborted");
}
