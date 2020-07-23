// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

StringLiteral const TraceType("ApplicationHostContainer");

// ********************************************************************************************************************
// ApplicationHostContainer::OpenApplicationHostLinkedAsyncOperation Implementation
//
class ApplicationHostContainer::OpenApplicationHostLinkedAsyncOperation
    : public LinkableAsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(OpenApplicationHostLinkedAsyncOperation)

public:
    OpenApplicationHostLinkedAsyncOperation(
        ApplicationHostContainer & owner,
        AsyncOperationSPtr const & primary, 
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : LinkableAsyncOperation(primary, callback, parent),
        owner_(owner),
        host_(),
        timeout_()
    {
    }

    OpenApplicationHostLinkedAsyncOperation(
        ApplicationHostContainer & owner,
        ApplicationHostSPtr const & host,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : LinkableAsyncOperation(callback, parent),
        owner_(owner),
        host_(host),
        timeout_(timeout)
    {
    }

    virtual ~OpenApplicationHostLinkedAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out ApplicationHostSPtr & host)
    {
        auto thisSPtr = AsyncOperation::End<OpenApplicationHostLinkedAsyncOperation>(operation);
        if (thisSPtr->Error.IsSuccess())
        {
            if (thisSPtr->IsSecondary)
            {
                host = thisSPtr->GetPrimary<OpenApplicationHostLinkedAsyncOperation>()->host_;
            }
            else
            {
                host = thisSPtr->host_;
            }
        }

        return thisSPtr->Error;
    }

protected:
    void OnResumeOutsideLockPrimary(AsyncOperationSPtr const & thisSPtr)
    {
        OpenApplicationHost(thisSPtr);
    }

    void OnCompleted()
    {
        if (!this->IsSecondary)
        {
            {
                AcquireWriteLock lock(owner_.lock_);
                ASSERT_IF(owner_.pendingHostCreateOperation_.get() != this, "Invalid pendingHostCreation operation.");
                ASSERT_IF(owner_.host_, "ApplicationHostContainer must not have a valid host.");

                if (this->Error.IsSuccess())
                {
                    owner_.host_ = host_;
                }
                owner_.pendingHostCreateOperation_.reset();
            }
        }

        LinkableAsyncOperation::OnCompleted();
    }

private:
    void OpenApplicationHost(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType,
            "Begin(OpenApplicationHost): Timeout={0}",
            timeout_);
        auto operation = host_->BeginOpen(
            timeout_,
            [this](AsyncOperationSPtr const & operation){ this->OnApplicationHostOpenCompleted(operation); },
            thisSPtr);
        if (operation->CompletedSynchronously)
        {
            FinishOpenApplicationHost(operation);
        }
    }

    void OnApplicationHostOpenCompleted(AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            FinishOpenApplicationHost(operation);
        }
    }

    void FinishOpenApplicationHost(AsyncOperationSPtr const & operation)
    {
        auto error = host_->EndOpen(operation);
         WriteTrace(
            error.ToLogLevel(),
            TraceType,
            "End(OpenApplicationHost): ErrorCode={0}",
            error);
        TryComplete(operation->Parent, error);
        return;
    }

private:
    ApplicationHostContainer & owner_;
    ApplicationHostSPtr const host_;
    TimeSpan timeout_;
};


// ********************************************************************************************************************
// ApplicationHostContainer::EnsureApplicationHostAsyncOperation Implementation
//
class ApplicationHostContainer::EnsureApplicationHostAsyncOperation
    : public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(EnsureApplicationHostAsyncOperation)

public:
    EnsureApplicationHostAsyncOperation(
        ApplicationHostContainer & owner,
        ApplicationHostContext const & hostContext,
        ComPointer<IFabricCodePackageHost> const & codePackageHost,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        hostContext_(hostContext),
        codePackageHost_(codePackageHost),
        timeoutHelper_(timeout),
        host_()
    {
    }

    virtual ~EnsureApplicationHostAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out ApplicationHostSPtr & host)
    {
        auto thisPtr = AsyncOperation::End<EnsureApplicationHostAsyncOperation>(operation);
        host = thisPtr->host_;
        return thisPtr->Error;
    } 
    
protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        host_ = owner_.GetApplicationHost();
        if (host_)
        {
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }
        else
        {
            CreateApplicationHost(thisSPtr);
        }
    }

private:
    void CreateApplicationHost(AsyncOperationSPtr const & thisSPtr)
    {
        // host does not exit, create host
        
        auto error = ApplicationHostContainer::CreateApplicationHost(hostContext_, codePackageHost_, host_);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            "CreateApplicationHost: ErrorCode={0}, HostContext={1}",
            error,
            hostContext_);
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        OpenAndSetApplicationHost(thisSPtr);
    }

    void OpenAndSetApplicationHost(AsyncOperationSPtr const & thisSPtr)
    {
        AsyncOperationSPtr linkedOpenOperation;
        {
            AcquireWriteLock lock(owner_.lock_);

            ASSERT_IF(
                (owner_.host_ && owner_.pendingHostCreateOperation_), 
                "There cannot be a pending operation to open the host, if the host is already available.");

            if (owner_.host_)
            {
                host_ = owner_.host_;
            }
            else
            {
                if (owner_.pendingHostCreateOperation_)
                {
                    // secondary
                    linkedOpenOperation = AsyncOperation::CreateAndStart<OpenApplicationHostLinkedAsyncOperation>(
                        owner_,
                        owner_.pendingHostCreateOperation_,
                        [this](AsyncOperationSPtr const & operation){ this->OnLinkedHostOpenCompleted(operation); },
                        thisSPtr);
                }
                else
                {
                    // primary
                    linkedOpenOperation = AsyncOperation::CreateAndStart<OpenApplicationHostLinkedAsyncOperation>(
                        owner_,
                        host_,
                        timeoutHelper_.GetRemainingTime(),
                        [this](AsyncOperationSPtr const & operation){ this->OnLinkedHostOpenCompleted(operation); },
                        thisSPtr);

                    owner_.pendingHostCreateOperation_ = linkedOpenOperation;
                }
            }
        }

        if (linkedOpenOperation)
        {
            auto casted = AsyncOperation::Get<OpenApplicationHostLinkedAsyncOperation>(linkedOpenOperation);
            casted->ResumeOutsideLock(linkedOpenOperation);
            if (linkedOpenOperation->CompletedSynchronously)
            {
                FinishLinkedHostOpen(linkedOpenOperation);
            }
        }
        else
        {
            ASSERT_IF(!host_, "Result must be set, if the linkable async operation is not set.");
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
        }
    }

    void OnLinkedHostOpenCompleted(AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            FinishLinkedHostOpen(operation);
        }
    }

    void FinishLinkedHostOpen(AsyncOperationSPtr const & operation)
    {
        auto error = OpenApplicationHostLinkedAsyncOperation::End(operation, host_);
        TryComplete(operation->Parent, error);
    }


private:
    ApplicationHostContainer & owner_;
    TimeoutHelper timeoutHelper_;
    ApplicationHostContext const hostContext_;
    ComPointer<IFabricCodePackageHost> const codePackageHost_;
    ApplicationHostSPtr host_;
};

// ********************************************************************************************************************
// ApplicationHostContainer::CreateFabricRuntimeComAsyncOperationContext Implementation
//
// {420E365C-1FD4-4BBE-B38F-C79904195CF7}
static const GUID CLSID_CreateFabricRuntimeComAsyncOperationContext = 
{ 0x420e365c, 0x1fd4, 0x4bbe, { 0xb3, 0x8f, 0xc7, 0x99, 0x4, 0x19, 0x5c, 0xf7 } };
  
class ApplicationHostContainer::CreateFabricRuntimeComAsyncOperationContext
    : public ComAsyncOperationContext,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
      DENY_COPY(CreateFabricRuntimeComAsyncOperationContext)

    COM_INTERFACE_LIST2(
        CreateFabricRuntimeComAsyncOperationContext,
        IID_IFabricAsyncOperationContext,
        IFabricAsyncOperationContext,
        CLSID_CreateFabricRuntimeComAsyncOperationContext,
        CreateFabricRuntimeComAsyncOperationContext)

public:
    explicit CreateFabricRuntimeComAsyncOperationContext() 
        : ComAsyncOperationContext()
    {
    }

    virtual ~CreateFabricRuntimeComAsyncOperationContext()
    {
    }

    HRESULT STDMETHODCALLTYPE Initialize(
        __in REFIID riid,
        __in IFabricProcessExitHandler *exitHandler,
        __in DWORD inTimeoutMilliseconds,
        __in ComponentRootSPtr const & rootSPtr,
        __in_opt IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
        if (FAILED(hr)) { return hr; }

        iid_ = riid;

        if (exitHandler != NULL)
        {
            fabricExitHandler_.SetAndAddRef(exitHandler);
        }
            
        if (inTimeoutMilliseconds == INFINITE)
        {
            timeout_ = TimeSpan::MaxValue;
        }
        else if (inTimeoutMilliseconds == 0)
        {
            timeout_ = HostingConfig::GetConfig().CreateFabricRuntimeTimeout;
        }
        else
        {
            timeout_ = TimeSpan::FromMilliseconds(static_cast<double>(inTimeoutMilliseconds));
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __out void **fabricRuntime)
    {
        ComPointer<CreateFabricRuntimeComAsyncOperationContext> thisOperation(context, CLSID_CreateFabricRuntimeComAsyncOperationContext);
        auto hr = thisOperation->Result;
        if (FAILED(hr)) { return hr; }

        return thisOperation->fabricRuntime_->QueryInterface(thisOperation->iid_, fabricRuntime);    
    }

protected:
    void OnStart(__in AsyncOperationSPtr const & proxySPtr)
    {
        ApplicationHostContainer & appHostContainer = GetApplicationHostContainer();
        auto operation = appHostContainer.BeginCreateComFabricRuntime(
            fabricExitHandler_,
            timeout_,
            [this](AsyncOperationSPtr const & operation){ this->FinishCreateComFabricRuntimeCompleted(operation, false); },
            proxySPtr);
        FinishCreateComFabricRuntimeCompleted(operation, true);
    }

private:
    void FinishCreateComFabricRuntimeCompleted(AsyncOperationSPtr const & operation, bool expectCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectCompletedSynchronously)
        {
            return;
        }

        ApplicationHostContainer & appHostContainer = GetApplicationHostContainer();

        auto error = appHostContainer.EndCreateComFabricRuntime(operation, fabricRuntime_);
        TryComplete(operation->Parent, error.ToHResult());
    }

private:
    IID iid_;
    TimeSpan timeout_;
    ComPointer<IFabricProcessExitHandler> fabricExitHandler_;
    ComPointer<ComFabricRuntime> fabricRuntime_;
};

// ********************************************************************************************************************
// ApplicationHostContainer::CreateComFabricRuntimeAsyncOperation Implementation
//
class ApplicationHostContainer::CreateComFabricRuntimeAsyncOperation
    : public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(CreateComFabricRuntimeAsyncOperation)

public:
    CreateComFabricRuntimeAsyncOperation(
        ApplicationHostContainer & owner,
        ComPointer<IFabricProcessExitHandler> const & fabricExitHandler,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        host_(),
        fabricExitHandler_(fabricExitHandler),
        hostContext_(),
        result_()
    {
    }

    virtual ~CreateComFabricRuntimeAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out ComPointer<ComFabricRuntime> & comFabricRuntime)
    {
        auto thisPtr = AsyncOperation::End<CreateComFabricRuntimeAsyncOperation>(operation);
        comFabricRuntime = thisPtr->result_;
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = InitializeHostContext();
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            "InitializeHostContext: ErrorCode={0}",
            error);
        
        // this operation is not valid in the context of high density (multi code package) host
        if (hostContext_.HostType == ApplicationHostType::Activated_MultiCodePackage)
        {
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::InvalidState));
            return;
        }

        // get application host
        if (host_)
        {
            CreateComFabricRuntime(thisSPtr);
        }
        else
        {
            EnsureApplicationHost(thisSPtr);
        }
    }

private:
    ErrorCode InitializeHostContext()
    {
        host_ = owner_.GetApplicationHost();
        if (host_)
        {
            hostContext_ = host_->HostContext;
            return ErrorCode(ErrorCodeValue::Success);
        }
        else
        {
            return ApplicationHostContainer::GetApplicationHostContext(hostContext_);
        }
    }

    void EnsureApplicationHost(AsyncOperationSPtr const & thisSPtr)
    {
        TimeSpan timeout = timeoutHelper_.GetRemainingTime();

        WriteNoise(
            TraceType,
            "Begin(EnsureApplicationHost): Timeout={0}",
            timeout);
        auto operation = owner_.BeginEnsureApplicationHost(
            hostContext_,
            ComPointer<IFabricCodePackageHost>(),
            timeout,
            [this](AsyncOperationSPtr const & operation){ this->OnEnsureApplicationHostCompleted(operation); },
            thisSPtr);
        if (operation->CompletedSynchronously)
        {
            FinishEnsureApplicationHost(operation);
        }
    }

    void OnEnsureApplicationHostCompleted(AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            FinishEnsureApplicationHost(operation);
        }
    }

    void FinishEnsureApplicationHost(AsyncOperationSPtr const & operation)
    {
        auto error = owner_.EndEnsureApplicationHost(operation, host_);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            "End(EnsureApplicationHost): ErrorCode={0}",
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
        TimeSpan timeout = timeoutHelper_.GetRemainingTime();

        WriteNoise(
            TraceType,
            "Begin(CreateComFabricRuntime): Timeout={0}",
            timeout);
        auto createRuntimeOperation = owner_.host_->BeginCreateComFabricRuntime(
                fabricExitHandler_,
                timeout,
                [this](AsyncOperationSPtr const & operation){ this->OnComFabricRuntimeCreated(operation); },
                thisSPtr);
        if (createRuntimeOperation->CompletedSynchronously)
        {
            FinishCreateComFabricRuntime(createRuntimeOperation);
        }
    }

    void OnComFabricRuntimeCreated(AsyncOperationSPtr const & createRuntimeOperation)
    {
        if (!createRuntimeOperation->CompletedSynchronously)
        {
            FinishCreateComFabricRuntime(createRuntimeOperation);
        }
    }

    void FinishCreateComFabricRuntime(AsyncOperationSPtr const & createRuntimeOperation)
    {
        auto error = owner_.host_->EndCreateComFabricRuntime(createRuntimeOperation, result_);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            "End(CreateComFabricRuntime): ErrorCode={0}",
            error);
        
        TryComplete(createRuntimeOperation->Parent, error);
    }

private:
    ApplicationHostContainer & owner_;
    TimeoutHelper timeoutHelper_;
    ApplicationHostSPtr host_;
    ComPointer<IFabricProcessExitHandler> const & fabricExitHandler_;
    ApplicationHostContext hostContext_;
    ComPointer<ComFabricRuntime> result_;
};

// ********************************************************************************************************************
// ApplicationHostContainer::CreateFabricRuntimeComAsyncOperationContext Implementation
//
// {C9CBB47D-E207-430F-A81A-D5FC15AA19E1}
static const GUID CLSID_GetActivationContextComAsyncOperationContext = 
{ 0xc9cbb47d, 0xe207, 0x430f, { 0xa8, 0x1a, 0xd5, 0xfc, 0x15, 0xaa, 0x19, 0xe1 } };

class ApplicationHostContainer::GetActivationContextComAsyncOperationContext
    : public ComAsyncOperationContext,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
      DENY_COPY(GetActivationContextComAsyncOperationContext)

    COM_INTERFACE_LIST2(
        GetActivationContextComAsyncOperationContext,
        IID_IFabricAsyncOperationContext,
        IFabricAsyncOperationContext,
        CLSID_GetActivationContextComAsyncOperationContext,
        GetActivationContextComAsyncOperationContext)

public:
    explicit GetActivationContextComAsyncOperationContext() 
        : ComAsyncOperationContext()
    {
    }

    virtual ~GetActivationContextComAsyncOperationContext()
    {
    }

    HRESULT STDMETHODCALLTYPE Initialize(
        __in REFIID riid,
        __in DWORD inTimeoutMilliseconds,
        __in ComponentRootSPtr const & rootSPtr,
        __in_opt IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
        if (FAILED(hr)) { return hr; }

        iid_ = riid;

        if (inTimeoutMilliseconds == INFINITE)
        {
            timeout_ = TimeSpan::MaxValue;
        }
        else if (inTimeoutMilliseconds == 0)
        {
            timeout_ = HostingConfig::GetConfig().GetCodePackageActivationContextTimeout;
        }
        else
        {
            timeout_ = TimeSpan::FromMilliseconds(static_cast<double>(inTimeoutMilliseconds));
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __out void **activationContext)
    {
        ComPointer<GetActivationContextComAsyncOperationContext> thisOperation(context, CLSID_GetActivationContextComAsyncOperationContext);
        auto hr = thisOperation->Result;
        if (FAILED(hr)) { return hr; }

        return thisOperation->activationContext_->QueryInterface(thisOperation->iid_, activationContext);    
    }

protected:
    void OnStart(__in AsyncOperationSPtr const & proxySPtr)
    {
        ApplicationHostContainer & appHostContainer = GetApplicationHostContainer();
        auto operation = appHostContainer.BeginGetComCodePackageActivationContext(
            timeout_,
            [this](AsyncOperationSPtr const & operation){ this->FinishGetComCodePackageActivationContext(operation, false); },
            proxySPtr);
        FinishGetComCodePackageActivationContext(operation, true);
    }

private:
    void FinishGetComCodePackageActivationContext(AsyncOperationSPtr const & operation, bool expectCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectCompletedSynchronously)
        {
            return;
        }

        ApplicationHostContainer & appHostContainer = GetApplicationHostContainer();

        auto error = appHostContainer.EndGetComCodePackageActivationContext(operation, activationContext_);
        TryComplete(operation->Parent, error.ToHResult());
    }

private:
    IID iid_;
    TimeSpan timeout_;
    ComPointer<ComCodePackageActivationContext> activationContext_;
};

// ********************************************************************************************************************
// ApplicationHostContainer::CreateFabricRuntimeComAsyncOperationContext Implementation
//
// {C9CBB47D-E207-430F-A81A-D5FC15AA19E1}
static const GUID CLSID_GetCodePackageActivatorComAsyncOperationContext =
{ 0xbdca6bef, 0xc502, 0x4a91, { 0xa2, 0x3a, 0x14, 0xe0, 0x5d, 0xa6, 0xe, 0xa8 } };

class ApplicationHostContainer::GetCodePackageActivatorComAsyncOperationContext
    : public ComAsyncOperationContext,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(GetCodePackageActivatorComAsyncOperationContext)

        COM_INTERFACE_LIST2(
            GetCodePackageActivatorComAsyncOperationContext,
            IID_IFabricAsyncOperationContext,
            IFabricAsyncOperationContext,
            CLSID_GetCodePackageActivatorComAsyncOperationContext,
            GetCodePackageActivatorComAsyncOperationContext)

public:
    explicit GetCodePackageActivatorComAsyncOperationContext()
        : ComAsyncOperationContext()
    {
    }

    virtual ~GetCodePackageActivatorComAsyncOperationContext()
    {
    }

    HRESULT STDMETHODCALLTYPE Initialize(
        __in REFIID riid,
        __in DWORD inTimeoutMilliseconds,
        __in ComponentRootSPtr const & rootSPtr,
        __in_opt IFabricAsyncOperationCallback * callback)
    {
        auto hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
        if (FAILED(hr)) { return hr; }

        iid_ = riid;

        if (inTimeoutMilliseconds == INFINITE)
        {
            timeout_ = TimeSpan::MaxValue;
        }
        else if (inTimeoutMilliseconds == 0)
        {
            timeout_ = HostingConfig::GetConfig().GetCodePackageActivationContextTimeout;
        }
        else
        {
            timeout_ = TimeSpan::FromMilliseconds(static_cast<double>(inTimeoutMilliseconds));
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __out void **activator)
    {
        ComPointer<GetCodePackageActivatorComAsyncOperationContext> thisOperation(
            context, CLSID_GetCodePackageActivatorComAsyncOperationContext);

        auto hr = thisOperation->Result;
        if (FAILED(hr))
        { 
            return hr;
        }

        return thisOperation->activator_->QueryInterface(thisOperation->iid_, activator);
    }

protected:
    void OnStart(__in AsyncOperationSPtr const & proxySPtr)
    {
        auto & appHostContainer = ApplicationHostContainer::GetApplicationHostContainer();

        auto operation = appHostContainer.BeginGetComCodePackageActivator(
            timeout_,
            [this](AsyncOperationSPtr const & operation)
            { 
                this->FinishGetComCodePackageActivator(operation, false); 
            },
            proxySPtr);

        this->FinishGetComCodePackageActivator(operation, true);
    }

private:
    void FinishGetComCodePackageActivator(
        AsyncOperationSPtr const & operation,
        bool expectCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectCompletedSynchronously)
        {
            return;
        }

        auto & appHostContainer = ApplicationHostContainer::GetApplicationHostContainer();

        auto error = appHostContainer.EndGetComCodePackageActivator(operation, activator_);
        
        this->TryComplete(operation->Parent, error.ToHResult());
    }

private:
    IID iid_;
    TimeSpan timeout_;
    ComPointer<ComApplicationHostCodePackageActivator> activator_;
};

// ********************************************************************************************************************
// ApplicationHostContainer::GetComCodePackageActivationContextAsyncOperation Implementation
//
class ApplicationHostContainer::GetComCodePackageActivationContextAsyncOperation
    : public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(GetComCodePackageActivationContextAsyncOperation)

public:
    GetComCodePackageActivationContextAsyncOperation(
        ApplicationHostContainer & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        hostContext_(),
        codePackageContext_(),
        host_(),
        codePackageActivationContextSPtr_()
    {
    }

    virtual ~GetComCodePackageActivationContextAsyncOperation()
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation,
        __out ComPointer<ComCodePackageActivationContext> & comCodePackageActivationContextCPtr)
    {
        auto thisPtr = AsyncOperation::End<GetComCodePackageActivationContextAsyncOperation>(operation);
        if(thisPtr->Error.IsSuccess())
        {
            comCodePackageActivationContextCPtr = make_com<ComCodePackageActivationContext>(thisPtr->codePackageActivationContextSPtr_);
        }

        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = EnsureValidEnvironment();
        if(!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        EnsureSingleCodePackageApplicationHost(thisSPtr);
    }

private:

    ErrorCode EnsureValidEnvironment()
    {
        EnvironmentMap envMap;
        if (!Environment::GetEnvironmentMap(envMap))
        {
            WriteError(
                TraceType,
                "EnsureValidEnvironment Failed to obtain environment block.");
            return ErrorCode(ErrorCodeValue::OperationFailed);
        }

        auto error = ApplicationHostContainer::GetApplicationHostContext(envMap, hostContext_);
        if (!error.IsSuccess()) 
        {
            WriteError(
                TraceType,
                "GetApplicationHostContext failed because of error {0}.",
                error);
            return error;
        }

        if ((hostContext_.HostType == ApplicationHostType::NonActivated) ||
            (hostContext_.HostType == ApplicationHostType::Activated_MultiCodePackage))
        {
            WriteWarning(
                TraceType,
                "GetActivationContextAsyncOperation called for invalid HostType {0}",
                hostContext_.HostType);
            return ErrorCode(ErrorCodeValue::InvalidState);
        }

        error = GetCodePackageContext(envMap, codePackageContext_);
        if (!error.IsSuccess()) 
        {
            WriteError(
                TraceType,
                "GetCodePackageContext failed because of error {0}.",
                error);
            return error;
        }

        if (codePackageContext_.CodePackageInstanceId.ServicePackageInstanceId.ApplicationId.IsAdhoc())
        {
            WriteWarning(
                TraceType,
                "GetActivationContextAsyncOperation called for adhoc activated application host: CodePackageId={0}",
                codePackageContext_.CodePackageInstanceId);

            return ErrorCode(ErrorCodeValue::InvalidState);
        }

        return ErrorCode::Success();
    }

    void EnsureSingleCodePackageApplicationHost(AsyncOperationSPtr const & thisSPtr)
    {
        TimeSpan timeout = timeoutHelper_.GetRemainingTime();

        WriteNoise(
            TraceType,
            "Begin(EnsureApplicationHost): Timeout={0}",
            timeout);
        auto operation = owner_.BeginEnsureApplicationHost(
            hostContext_,
            ComPointer<IFabricCodePackageHost>(),
            timeout,
            [this](AsyncOperationSPtr const & operation){ this->OnEnsureApplicationHostCompleted(operation); },
            thisSPtr);
        if (operation->CompletedSynchronously)
        {
            FinishEnsureApplicationHost(operation);
        }
    }

    void OnEnsureApplicationHostCompleted(AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            FinishEnsureApplicationHost(operation);
        }
    }

    void FinishEnsureApplicationHost(AsyncOperationSPtr const & operation)
    {
        auto error = owner_.EndEnsureApplicationHost(operation, host_);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            "End(EnsureApplicationHost): ErrorCode={0}",
            error);

        if(error.IsSuccess())
        {
            error = host_->GetCodePackageActivationContext(codePackageContext_, codePackageActivationContextSPtr_);
        }

        TryComplete(operation->Parent, error);
        return;
    }

private:
    ApplicationHostContainer & owner_;
    TimeoutHelper timeoutHelper_;
    ApplicationHostContext hostContext_;
    CodePackageContext codePackageContext_;
    ApplicationHostSPtr host_;
    CodePackageActivationContextSPtr codePackageActivationContextSPtr_;
};

// ********************************************************************************************************************
// ApplicationHostContainer::GetComCodePackageActivatorAsyncOperation Implementation
//
class ApplicationHostContainer::GetComCodePackageActivatorAsyncOperation
    : public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(GetComCodePackageActivatorAsyncOperation)

public:
    GetComCodePackageActivatorAsyncOperation(
        ApplicationHostContainer & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        hostContext_(),
        host_(),
        codePackageActivatorSPtr_()
    {
    }

    virtual ~GetComCodePackageActivatorAsyncOperation()
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation,
        __out ComPointer<ComApplicationHostCodePackageActivator> & comCodePackageActivatorCPtr)
    {
        auto thisPtr = AsyncOperation::End<GetComCodePackageActivatorAsyncOperation>(operation);
        if (thisPtr->Error.IsSuccess())
        {
            comCodePackageActivatorCPtr = make_com<ComApplicationHostCodePackageActivator>(
                thisPtr->codePackageActivatorSPtr_);
        }

        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = this->EnsureValidEnvironment();
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        this->EnsureSingleCodePackageApplicationHost(thisSPtr);
    }

private:
    ErrorCode EnsureValidEnvironment()
    {
        EnvironmentMap envMap;
        if (!Environment::GetEnvironmentMap(envMap))
        {
            WriteError(
                TraceType,
                "GetComCodePackageActivatorAsyncOperation: EnsureValidEnvironment failed to obtain environment block.");
            return ErrorCode(ErrorCodeValue::OperationFailed);
        }

        auto error = ApplicationHostContainer::GetApplicationHostContext(envMap, hostContext_);
        if (!error.IsSuccess())
        {
            WriteError(
                TraceType,
                "GetComCodePackageActivatorAsyncOperation: GetApplicationHostContext failed because of error {0}.",
                error);
            return error;
        }

        if ((hostContext_.HostType == ApplicationHostType::NonActivated) ||
            (hostContext_.HostType == ApplicationHostType::Activated_MultiCodePackage))
        {
            WriteWarning(
                TraceType,
                "GetComCodePackageActivatorAsyncOperation called for invalid HostType {0}",
                hostContext_.HostType);

            return ErrorCode(ErrorCodeValue::InvalidOperation);
        }

        CodePackageContext codePackageContext;
        error = ApplicationHostContainer::GetCodePackageContext(envMap, codePackageContext);
        if (!error.IsSuccess())
        {
            WriteError(
                TraceType,
                "GetCodePackageContext failed because of error {0}.",
                error);
            return error;
        }

        if (codePackageContext.CodePackageInstanceId.ServicePackageInstanceId.ApplicationId.IsAdhoc())
        {
            WriteWarning(
                TraceType,
                "GetComCodePackageActivatorAsyncOperation called for adhoc activated application host: CodePackageId={0}",
                codePackageContext.CodePackageInstanceId);

            return ErrorCode(ErrorCodeValue::InvalidOperation);
        }

        return ErrorCode::Success();
    }

    void EnsureSingleCodePackageApplicationHost(AsyncOperationSPtr const & thisSPtr)
    {
        auto timeout = timeoutHelper_.GetRemainingTime();

        WriteNoise(
            TraceType,
            "Begin(EnsureApplicationHost): Timeout={0}",
            timeout);

        auto operation = owner_.BeginEnsureApplicationHost(
            hostContext_,
            ComPointer<IFabricCodePackageHost>(),
            timeout,
            [this](AsyncOperationSPtr const & operation)
            { 
                this->FinishEnsureApplicationHost(operation, false);
            },
            thisSPtr);

        this->FinishEnsureApplicationHost(operation, true);
    }

    void FinishEnsureApplicationHost(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedAsynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedAsynchronously)
        {
            return;
        }

        auto error = owner_.EndEnsureApplicationHost(operation, host_);
        
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            "End(EnsureApplicationHost): ErrorCode={0}",
            error);

        if (error.IsSuccess())
        {
            error = host_->GetCodePackageActivator(codePackageActivatorSPtr_);
        }

        TryComplete(operation->Parent, error);
        return;
    }

private:
    ApplicationHostContainer & owner_;
    TimeoutHelper timeoutHelper_;
    ApplicationHostContext hostContext_;
    ApplicationHostSPtr host_;
    ApplicationHostCodePackageActivatorSPtr codePackageActivatorSPtr_;
};

// ********************************************************************************************************************
// ApplicationHostContainer::GetNodeContextComAsyncOperationContext Implementation
//
// {aff0fd05-f49c-4803-b287-4cf1c245c381}
static const GUID CLSID_GetNodeContextComAsyncOperationContext = 
{ 0xaff0fd05, 0xf49c, 0x4803, { 0xb2, 0x87, 0x4c, 0xf1, 0xc2, 0x45, 0xc3, 0x81 } };
  
class ApplicationHostContainer::GetNodeContextComAsyncOperationContext
    : public ComAsyncOperationContext,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
      DENY_COPY(GetNodeContextComAsyncOperationContext)

    COM_INTERFACE_LIST2(
        GetNodeContextComAsyncOperationContext,
        IID_IFabricAsyncOperationContext,
        IFabricAsyncOperationContext,
        CLSID_GetNodeContextComAsyncOperationContext,
        GetNodeContextComAsyncOperationContext)

public:
    explicit GetNodeContextComAsyncOperationContext() 
        : ComAsyncOperationContext(),
        nodeContextResult_()
    {
    }

    virtual ~GetNodeContextComAsyncOperationContext()
    {
    }

    HRESULT STDMETHODCALLTYPE Initialize(
        __in DWORD inTimeoutMilliseconds,
        __in ComponentRootSPtr const & rootSPtr,
        __in_opt IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
        if (FAILED(hr)) { return hr; }

        if (inTimeoutMilliseconds == INFINITE)
        {
            timeout_ = TimeSpan::MaxValue;
        }
        else if (inTimeoutMilliseconds == 0)
        {
            timeout_ = HostingConfig::GetConfig().GetCodePackageActivationContextTimeout;
        }
        else
        {
            timeout_ = TimeSpan::FromMilliseconds(static_cast<double>(inTimeoutMilliseconds));
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context,
        __out void ** nodeContext)
    {
        ComPointer<GetNodeContextComAsyncOperationContext> thisOperation(context, CLSID_GetNodeContextComAsyncOperationContext);
        auto hr = thisOperation->Result;
        if (FAILED(hr)) { return hr; }
        thisOperation->nodeContextResult_->QueryInterface(IID_IFabricNodeContextResult2, nodeContext);
        return hr;
    }

protected:
    void OnStart(__in AsyncOperationSPtr const & proxySPtr)
    {
        ApplicationHostContainer & appHostContainer = GetApplicationHostContainer();
        auto operation = appHostContainer.BeginGetFabricNodeContext(
            timeout_,
            [this](AsyncOperationSPtr const & operation){ this->FinishGetFabricNodeContext(operation, false); },
            proxySPtr);
        if(operation->CompletedSynchronously)
        {
            FinishGetFabricNodeContext(operation, true);
        }
    }

private:
    void FinishGetFabricNodeContext(AsyncOperationSPtr const & operation, bool expectCompletedSynchronously)
    {
         if (operation->CompletedSynchronously != expectCompletedSynchronously)
        {
            return;
        }

       ApplicationHostContainer & appHostContainer = GetApplicationHostContainer();

        auto error = appHostContainer.EndGetFabricNodeContext(operation, nodeContextResult_);
        TryComplete(operation->Parent, error.ToHResult());
    }

private:
    TimeSpan timeout_;
    ComPointer<ComFabricNodeContextResult> nodeContextResult_;
};

// ********************************************************************************************************************
// ApplicationHostContainer::GetNodeContextAsyncOperation Implementation
//
class ApplicationHostContainer::GetNodeContextAsyncOperation
    : public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(GetNodeContextAsyncOperation)

public:
    GetNodeContextAsyncOperation(
        ApplicationHostContainer & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        hostContext_(),
        host_()
    {
    }

    virtual ~GetNodeContextAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation,
        __out ComPointer<ComFabricNodeContextResult> & nodeContextResultCPtr)
    {
        auto thisPtr = AsyncOperation::End<GetNodeContextAsyncOperation>(operation);
        if(thisPtr->Error.IsSuccess())
        {
            nodeContextResultCPtr = make_com<ComFabricNodeContextResult>(thisPtr->result_);
        }

        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = EnsureValidEnvironment();
        if(!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        EnsureApplicationHost(thisSPtr);
    }

private:

    ErrorCode EnsureValidEnvironment()
    {
        auto error = ApplicationHostContainer::GetApplicationHostContext(hostContext_);
        if (!error.IsSuccess()) 
        {
            WriteError(
                TraceType,
                "GetApplicationHostContext failed because of error {0}.",
                error);
            return error;
        }

        return ErrorCode::Success();
    }

    void EnsureApplicationHost(AsyncOperationSPtr const & thisSPtr)
    {
        TimeSpan timeout = timeoutHelper_.GetRemainingTime();

        WriteNoise(
            TraceType,
            "Begin(EnsureApplicationHost): Timeout={0}",
            timeout);
        auto operation = owner_.BeginEnsureApplicationHost(
            hostContext_,
            ComPointer<IFabricCodePackageHost>(),
            timeout,
            [this](AsyncOperationSPtr const & operation){ this->OnEnsureApplicationHostCompleted(operation); },
            thisSPtr);
        if (operation->CompletedSynchronously)
        {
            FinishEnsureApplicationHost(operation);
        }
    }

    void OnEnsureApplicationHostCompleted(AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            FinishEnsureApplicationHost(operation);
        }
    }

    void FinishEnsureApplicationHost(AsyncOperationSPtr const & operation)
    {
        auto error = owner_.EndEnsureApplicationHost(operation, host_);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            "End(EnsureApplicationHost): ErrorCode={0}",
            error);
        owner_.GetNodeContext(result_);
        TryComplete(operation->Parent, error);
        return;
    }

private:
    ApplicationHostContainer & owner_;
    TimeoutHelper timeoutHelper_;
    ApplicationHostContext hostContext_;
    ApplicationHostSPtr host_;
    FabricNodeContextResultImplSPtr result_;
};


// ********************************************************************************************************************
// ApplicationHostContainer::RegisterCodePackageHostAsyncOperation Implementation
//
class ApplicationHostContainer::RegisterCodePackageHostAsyncOperation
    : public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(RegisterCodePackageHostAsyncOperation)

public:
    RegisterCodePackageHostAsyncOperation(
        ApplicationHostContainer & owner,
        ComPointer<IFabricCodePackageHost> const & codePackageHost,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        codePackageHost_(codePackageHost),
        hostContext_()
    {
    }

    virtual ~RegisterCodePackageHostAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<RegisterCodePackageHostAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = InitializeHostContext();
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            "InitializeHostContext: ErrorCode={0}",
            error);

        if (hostContext_.HostType != ApplicationHostType::Activated_MultiCodePackage)
        {
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::InvalidState));
            return;
        }

        if (host_)
        {
            // register code package can only be called once successfully
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::InvalidState));
            return;
        }
        else
        {
            EnsureMultiCodePackageApplicationHost(thisSPtr);
        }
    }

private:
    void EnsureMultiCodePackageApplicationHost(AsyncOperationSPtr const & thisSPtr)
    {
        TimeSpan timeout = timeoutHelper_.GetRemainingTime();

        WriteNoise(
            TraceType,
            "Begin(EnsureApplicationHost): Timeout={0}",
            timeout);
        auto operation = owner_.BeginEnsureApplicationHost(
            hostContext_,
            codePackageHost_,
            timeout,
            [this](AsyncOperationSPtr const & operation){ this->OnEnsureApplicationHostCompleted(operation); },
            thisSPtr);
        if (operation->CompletedSynchronously)
        {
            FinishEnsureApplicationHost(operation);
        }
    }

    void OnEnsureApplicationHostCompleted(AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            FinishEnsureApplicationHost(operation);
        }
    }

    void FinishEnsureApplicationHost(AsyncOperationSPtr const & operation)
    {
        auto error = owner_.EndEnsureApplicationHost(operation, host_);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            "End(EnsureApplicationHost): ErrorCode={0}",
            error);
        TryComplete(operation->Parent, error);
        return;
    }

    ErrorCode InitializeHostContext()
    {
        host_ = owner_.GetApplicationHost();
        if (host_)
        {
            hostContext_ = host_->HostContext;
            return ErrorCode(ErrorCodeValue::Success);
        }
        else
        {
            return ApplicationHostContainer::GetApplicationHostContext(hostContext_);
        }
    }

private:
    ApplicationHostContainer & owner_;
    TimeoutHelper timeoutHelper_;
    ComPointer<IFabricCodePackageHost> const codePackageHost_;
    ApplicationHostSPtr host_;
    ApplicationHostContext hostContext_;
};

// ********************************************************************************************************************
// ApplicationHostContainer Implementation
//

ApplicationHostContainer * ApplicationHostContainer::singleton_;
INIT_ONCE ApplicationHostContainer::initOnceAppHostContainer_;
     
ApplicationHostContainer::ApplicationHostContainer()
    : lock_(),
    host_(),
    pendingHostCreateOperation_()
{
}

ApplicationHostContainer::~ApplicationHostContainer()
{
}

ApplicationHostContainer & ApplicationHostContainer::GetApplicationHostContainer()
{
    PVOID lpContext = NULL;
    BOOL  bStatus = FALSE;

    bStatus = ::InitOnceExecuteOnce(
        &ApplicationHostContainer::initOnceAppHostContainer_,
        ApplicationHostContainer::InitAppHostContainerFunction,
        NULL,
        &lpContext);

    ASSERT_IF(!bStatus, "Failed to initialize ApplicationHostContainrt singleton");
    return *(ApplicationHostContainer::singleton_);
}

BOOL CALLBACK ApplicationHostContainer::InitAppHostContainerFunction(PINIT_ONCE, PVOID, PVOID *)
{
    singleton_ = new ApplicationHostContainer();
    return TRUE;
}

AsyncOperationSPtr ApplicationHostContainer::BeginCreateComFabricRuntime(
    ComPointer<IFabricProcessExitHandler> const & fabricExitHandler,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CreateComFabricRuntimeAsyncOperation>(
        *this,
        fabricExitHandler,
        timeout,
        callback,
        parent);
}

ErrorCode ApplicationHostContainer::EndCreateComFabricRuntime(
    AsyncOperationSPtr const & operation,
    __out ComPointer<ComFabricRuntime> & fabricRuntime)
{
    return CreateComFabricRuntimeAsyncOperation::End(operation, fabricRuntime);
}

AsyncOperationSPtr ApplicationHostContainer::BeginGetComCodePackageActivationContext(
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<GetComCodePackageActivationContextAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode ApplicationHostContainer::EndGetComCodePackageActivationContext(
    AsyncOperationSPtr const & operation,
    __out ComPointer<ComCodePackageActivationContext> & comCodePackageActivationContextCPtr)
{
    return GetComCodePackageActivationContextAsyncOperation::End(operation, comCodePackageActivationContextCPtr);
}

AsyncOperationSPtr ApplicationHostContainer::BeginGetComCodePackageActivator(
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<GetComCodePackageActivatorAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode ApplicationHostContainer::EndGetComCodePackageActivator(
    AsyncOperationSPtr const & operation,
    _Out_ ComPointer<ComApplicationHostCodePackageActivator> & comCodePackageActivatorCPtr)
{
    return GetComCodePackageActivatorAsyncOperation::End(operation, comCodePackageActivatorCPtr);
}

AsyncOperationSPtr ApplicationHostContainer::BeginGetFabricNodeContext(
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<GetNodeContextAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode ApplicationHostContainer::EndGetFabricNodeContext(
    AsyncOperationSPtr const & operation,
    __out ComPointer<ComFabricNodeContextResult> & nodeContextResultCPtr)
{
    return GetNodeContextAsyncOperation::End(operation, nodeContextResultCPtr);
}

AsyncOperationSPtr ApplicationHostContainer::BeginRegisterCodePackageHost(
    ComPointer<IFabricCodePackageHost> const & codePackageHost,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<RegisterCodePackageHostAsyncOperation>(
        *this,
        codePackageHost,
        timeout,
        callback,
        parent);
}

ErrorCode ApplicationHostContainer::EndRegisterCodePackageHost(
    AsyncOperationSPtr const & operation)
{
    return RegisterCodePackageHostAsyncOperation::End(operation);
}

ApplicationHostSPtr ApplicationHostContainer::GetApplicationHost()
{
    {
        AcquireReadLock lock(lock_);
        return host_;
    }
}

bool ApplicationHostContainer::ClearApplicationHost(wstring const & hostId)
{
    bool success = false;
    {
        AcquireWriteLock lock(lock_);
        if (host_ && StringUtility::AreEqualCaseInsensitive(host_->Id, hostId))
        {
            auto hostState = host_->State.Value;
            if ((hostState != FabricComponentState::Aborted) && 
                (hostState != FabricComponentState::Closing) &&
                (hostState != FabricComponentState::Closed)) 
            {
                Assert::CodingError(
                    "The Application Host {0} must be in Aborted, Closing or Closed state, the current state is {1}.",
                    host_->Id,
                    hostState);
            }

            success = true;
            host_.reset();
            pendingHostCreateOperation_.reset();
        }
    }

    return success;
}

ErrorCode ApplicationHostContainer::GetRuntimeInformationFromEnv(
    EnvironmentMap const & envMap,
    ApplicationHostContext const & hostContext,
    wstring & runtimeServiceAddress,
    PCCertContext & certContextPtr,
    wstring & thumbprint)
{
    ErrorCode error = ErrorCodeValue::Success;
    if (hostContext.HostType == ApplicationHostType::NonActivated)
    {
        return error;
    }

    bool useSslService = false;

    if (hostContext.IsContainerHost)
    {
        useSslService = true;
    }
    else
    {
        auto iter = envMap.find(Constants::EnvironmentVariable::RuntimeConnectionUseSsl);
        if(iter != envMap.end())
        {
            wstring runtimeConnectionUseSsl = iter->second;
            if (StringUtility::AreEqualCaseInsensitive(runtimeConnectionUseSsl, L"true"))
            {
                useSslService = true;
            }
        }
    }

    auto iter = envMap.find(useSslService ? 
        Constants::EnvironmentVariable::RuntimeSslConnectionAddress :       
        Constants::EnvironmentVariable::RuntimeConnectionAddress);
    if(iter == envMap.end())
    {
        WriteError(
            TraceType,
            "Could not find the RuntimeConnectionAddress in the envMap of activated host.");

        return ErrorCode(ErrorCodeValue::OperationFailed);
    }

    runtimeServiceAddress = iter->second;

    if (useSslService)
    {
#ifndef PLATFORM_UNIX
        wstring certKey;
        iter = envMap.find(Constants::EnvironmentVariable::RuntimeSslConnectionCertKey);
        if(iter == envMap.end())
        {
            WriteError(
                TraceType,
                "Could not find the RuntimeSslConnectionCertKey in the envMap of activated host.");

            return ErrorCode(ErrorCodeValue::OperationFailed);
        }

        certKey = iter->second;

        ByteBuffer certEncodedBytes;
        iter = envMap.find(Constants::EnvironmentVariable::RuntimeSslConnectionCertEncodedBytes);
        if(iter == envMap.end())
        {
            WriteError(
                TraceType,
                "Could not find the RuntimeSslConnectionCertEncodedBytes in the envMap of activated host.");

            return ErrorCode(ErrorCodeValue::OperationFailed);
        }

        CryptoUtility::Base64StringToBytes(iter->second, certEncodedBytes);

        CertContextUPtr cert;
        error = CryptoUtility::CreateCertFromKey(
            certEncodedBytes,
            certKey,
            cert);
        certContextPtr = cert.release();

        if (!error.IsSuccess())
        {
            WriteError(
                TraceType,
                "CryptoUtility::CreateCertFromKey failed: {0}",
                error);
            return error; 
        }
#else
        iter = envMap.find(Constants::EnvironmentVariable::RuntimeSslConnectionCertFilePath);
        if(iter == envMap.end())
        {
            WriteError(
                TraceType,
                "Could not find the RuntimeSslConnectionCertFilePath in the envMap of activated host.");

            return ErrorCode(ErrorCodeValue::OperationFailed);
        }

        auto certFilePath = iter->second;
        
        CertContextUPtr cert;
        CryptoUtility::GetCertificate(certFilePath, cert);        
        certContextPtr = cert.release();
#endif
        iter = envMap.find(Constants::EnvironmentVariable::RuntimeSslConnectionCertThumbprint);
        if(iter == envMap.end())
        {
            WriteError(
                TraceType,
                "Could not find the RuntimeSslConnectionCertThumbprint in the envMap of activated host.");

            return ErrorCode(ErrorCodeValue::OperationFailed);
        }

        thumbprint = iter->second;
    }

#ifndef PLATFORM_UNIX
    if (hostContext.IsContainerHost)
    {
        ASSERT_IF(runtimeServiceAddress.empty(), "RuntimeServiceAddress should not be empty");

        if (!NetworkType::IsMultiNetwork(ContainerEnvironment::GetContainerNetworkingMode()))
        {
            map<wstring, vector<wstring>> gatewayAddressPerAdapter;
            error = IpUtility::GetGatewaysPerAdapter(gatewayAddressPerAdapter);

            ASSERT_IF(!error.IsSuccess(), "Getting HOST IP failed - {0}", error);
            ASSERT_IF(gatewayAddressPerAdapter.size() > 1, "Found more than one adapter in container");

            USHORT port = Transport::TcpTransportUtility::ParsePortString(runtimeServiceAddress);

            runtimeServiceAddress = Transport::TcpTransportUtility::ConstructAddressString(gatewayAddressPerAdapter.begin()->second[0], port);
        }
    }
#endif

    return error;
}

ErrorCode ApplicationHostContainer::CreateApplicationHost(
    ApplicationHostContext const & hostContext,
    ComPointer<IFabricCodePackageHost> const & codePackageHost,
    __out ApplicationHostSPtr & host)
{
    ErrorCode error = ErrorCodeValue::Success;

    EnvironmentMap envMap;
    if (!Environment::GetEnvironmentMap(envMap))
    {
        WriteError(
            TraceType,
            "CreateApplicationHost failed to obtain environment block.");
        return ErrorCode(ErrorCodeValue::OperationFailed);
    }        

    if (hostContext.IsContainerHost)
    {
        error = ApplicationHostContainer::StartTraceSessionInsideContainer();
        if (!error.IsSuccess())
        {
            WriteError(TraceType, "Unable to start trace sessions: ErrorCode={0}", error);
            return ErrorCode(ErrorCodeValue::OperationFailed);
        }
    }

    wstring runtimeServiceAddress;
    PCCertContext certContextPtr = nullptr;
    wstring thumbprint;

    error = GetRuntimeInformationFromEnv(envMap, hostContext, runtimeServiceAddress, certContextPtr, thumbprint);

    if (!error.IsSuccess()) { return error; }

    switch(hostContext.HostType)
    {
        case ApplicationHostType::NonActivated:
        {
            FabricNodeConfigSPtr config = make_shared<FabricNodeConfig>();

            error = NonActivatedApplicationHost::Create(nullptr, config->RuntimeServiceAddress, host);
            host->RegisterNodeHostProcessClosedEventHandler(
                [](NodeHostProcessClosedEventArgs const & eventArgs) { GetApplicationHostContainer().NodeHostProcessClosedEventHandler(eventArgs); });
            break;
        }

        case ApplicationHostType::Activated_SingleCodePackage:
        {
            ASSERT_IF(runtimeServiceAddress.empty(), "RuntimeServiceAddress should not be empty");

            CodePackageContext codeContext;
            error = GetCodePackageContext(envMap, codeContext);
            if (!error.IsSuccess())  { return error; }

            error = SingleCodePackageApplicationHost::Create(hostContext, runtimeServiceAddress, certContextPtr, thumbprint, codeContext, host);
            break;
        }

        case ApplicationHostType::Activated_MultiCodePackage:
        {
            ASSERT_IF(runtimeServiceAddress.empty(), "RuntimeServiceAddress should not be empty");

            ASSERT_IF(codePackageHost.GetRawPointer() == nullptr, "The CodePackageHost must not be null.");
            error = MultiCodePackageApplicationHost::Create(hostContext.HostId, runtimeServiceAddress, certContextPtr, thumbprint, codePackageHost, host);
            break;
        }

    default:
        Assert::CodingError("Invalid HostType value {0}.", hostContext.HostType);
    }

    return error;
}

AsyncOperationSPtr ApplicationHostContainer::BeginEnsureApplicationHost(
    ApplicationHostContext const & hostContext,
    ComPointer<IFabricCodePackageHost> const & codePackageHost,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<EnsureApplicationHostAsyncOperation>(
        *this,
        hostContext,
        codePackageHost,
        timeout,
        callback,
        parent);
}

ErrorCode ApplicationHostContainer::EndEnsureApplicationHost(
    AsyncOperationSPtr const & operation,
    __out ApplicationHostSPtr & host)
{
    return EnsureApplicationHostAsyncOperation::End(operation, host);
}

void ApplicationHostContainer::NodeHostProcessClosedEventHandler(NodeHostProcessClosedEventArgs const & eventArgs)
{
    WriteNoise(
        TraceType,
        "Processing NodeHostClosed Event {0}", 
        eventArgs);

    ClearApplicationHost(eventArgs.DetectedByHostId);
}

void ApplicationHostContainer::Test_CloseApplicationHost()
{
    ApplicationHostSPtr host;

    {
        AcquireReadLock lock(lock_);
        host = host_;
    }

    if (host)
    {
        host->Abort();
        ClearApplicationHost(host->Id);
    }
}

ErrorCode ApplicationHostContainer::GetApplicationHostContext(
    __out ApplicationHostContext & hostContext)
{
    EnvironmentMap envMap;

    if (!Environment::GetEnvironmentMap(envMap))
    {
        WriteError(
            TraceType,
            "GetApplicationHostContext failed to obtain environment block.");
        return ErrorCode(ErrorCodeValue::OperationFailed);
    }

    return GetApplicationHostContext(envMap, hostContext);
}

ErrorCode ApplicationHostContainer::GetApplicationHostContext(
    EnvironmentMap const & envMap,
    __out ApplicationHostContext & hostContext)
{

    auto error = ApplicationHostContext::FromEnvironmentMap(envMap, hostContext);
    if (error.IsError(ErrorCodeValue::NotFound))
    {
        //
        // This means this is a non-activatied application host.
        //

        hostContext = ApplicationHostContext(
            Guid::NewGuid().ToString(), 
            ApplicationHostType::NonActivated,
            false /* isContainerHost */,
            false /* isCodePackageActivatorHost */);

        error = ErrorCode(ErrorCodeValue::Success);
    }

    if (!error.IsSuccess())
    {
        WriteError(
            TraceType,
            "GetApplicationHostContext: Failed to obtain valid ApplicationHostContext from environment block. ErrorCode={0}, EnvironmentBlock={1}",
            error,
            Environment::ToString(envMap));

        return ErrorCode(ErrorCodeValue::OperationFailed);
    }

    WriteNoise(
        TraceType,
        "GetApplicationHostContext: HostContext={0}", 
        hostContext);
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ApplicationHostContainer::GetCodePackageContext(
    EnvironmentMap const & envMap,
    __out CodePackageContext & codeContext)
{
    auto error = CodePackageContext::FromEnvironmentMap(envMap, codeContext);
    if (!error.IsSuccess())
    {
        //Error is either NotFound or InvalidState. Either ways we expect the environment variable to be set for Activated application host
        WriteError(
            TraceType,
            "GetCodePackageContext: Failed to obtain valid CodePackageContext from environment block. ErrorCode={0}, EnvironmentBlock={1}",
            error,
            Environment::ToString(envMap));
        return ErrorCode(ErrorCodeValue::OperationFailed);
    }

    WriteNoise(
        TraceType,
        "GetCodePackageContext: CodeContext={0}", 
        codeContext);
    return ErrorCode(ErrorCodeValue::Success);
}

// ********************************************************************************************************************
// Implementation of Global Functions provided by ApplicationHostContainer
//

HRESULT ApplicationHostContainer::FabricBeginCreateRuntime(
    /* [in] */ __RPC__in REFIID riid,
    /* [in] */ __RPC__in_opt IFabricProcessExitHandler *exitHandler,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ __RPC__in_opt IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ __RPC__deref_out_opt IFabricAsyncOperationContext **context)
{
    ErrorCode errorMTA = ComUtility::CheckMTA();
    if (!errorMTA.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(errorMTA.ToHResult());
    }

    ComPointer<CreateFabricRuntimeComAsyncOperationContext> operation = make_com<CreateFabricRuntimeComAsyncOperationContext>();

    HRESULT hr = operation->Initialize(
        riid,
        exitHandler,
        timeoutMilliseconds,
        ComponentRootSPtr(),
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ApplicationHostContainer::FabricEndCreateRuntime( 
    /* [in] */ __RPC__in_opt IFabricAsyncOperationContext *context,
    /* [retval][out] */ __RPC__deref_out_opt void **fabricRuntime)
{
    if (context == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }
    if (fabricRuntime == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    return ComUtility::OnPublicApiReturn(CreateFabricRuntimeComAsyncOperationContext::End(context, fabricRuntime));
}   


HRESULT ApplicationHostContainer::FabricCreateRuntime( 
    /* [in] */ __RPC__in REFIID riid,
    /* [retval][out] */ __RPC__deref_out_opt void **fabricRuntime)
{
    if (fabricRuntime == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    auto waiter = make_shared<ComAsyncOperationWaiter>();
    ComPointer<IFabricAsyncOperationContext> context;

    HRESULT hr = ApplicationHostContainer::FabricBeginCreateRuntime(
        riid,
        NULL,
        0, // use default from the configuration
        waiter->GetAsyncOperationCallback([waiter,&fabricRuntime](IFabricAsyncOperationContext * context)
        {
            auto hr = ApplicationHostContainer::FabricEndCreateRuntime(context, fabricRuntime);
            waiter->SetHRESULT(hr);
            waiter->Set();
        }),
        context.InitializationAddress());
    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }
    else
    {
        waiter->WaitOne();
        return ComUtility::OnPublicApiReturn(waiter->GetHRESULT());
    }
}

HRESULT ApplicationHostContainer::FabricBeginGetActivationContext(
    /* [in] */ __RPC__in REFIID riid,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ __RPC__in_opt IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ __RPC__deref_out_opt IFabricAsyncOperationContext **context)
{
    ErrorCode errorMTA = ComUtility::CheckMTA();
    if (!errorMTA.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(errorMTA.ToHResult());
    }

    ComPointer<GetActivationContextComAsyncOperationContext> operation = make_com<GetActivationContextComAsyncOperationContext>();

    HRESULT hr = operation->Initialize(
        riid,
        timeoutMilliseconds,
        ComponentRootSPtr(),
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ApplicationHostContainer::FabricEndGetActivationContext(
    /* [in] */ __RPC__in_opt IFabricAsyncOperationContext *context,
    /* [retval][out] */ __RPC__deref_out_opt void **activationContext)
{
    if (context == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }
    if (activationContext == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    return ComUtility::OnPublicApiReturn(GetActivationContextComAsyncOperationContext::End(context, activationContext));
}

HRESULT ApplicationHostContainer::FabricGetActivationContext( 
    /* [in] */ __RPC__in REFIID riid,
    /* [retval][out] */ __RPC__deref_out_opt void **activationContext)
{
    if (activationContext == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    auto waiter = make_shared<ComAsyncOperationWaiter>();
    ComPointer<IFabricAsyncOperationContext> context;

    HRESULT hr = ApplicationHostContainer::FabricBeginGetActivationContext(
        riid,
        0, // load default from configuration
        waiter->GetAsyncOperationCallback([waiter,&activationContext](IFabricAsyncOperationContext * context)
        {
            auto hr = ApplicationHostContainer::FabricEndGetActivationContext(context, activationContext);
            waiter->SetHRESULT(hr);
            waiter->Set();
        }),
        context.InitializationAddress());
    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }
    else
    {
        waiter->WaitOne();
        return ComUtility::OnPublicApiReturn(waiter->GetHRESULT());
    }
}

HRESULT ApplicationHostContainer::FabricBeginGetCodePackageActivator(
    /* [in] */ __RPC__in REFIID riid,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ __RPC__in_opt IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ __RPC__deref_out_opt IFabricAsyncOperationContext **context)
{
    auto errorMTA = ComUtility::CheckMTA();
    if (!errorMTA.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(errorMTA.ToHResult());
    }

    auto operation = make_com<GetCodePackageActivatorComAsyncOperationContext>();

    auto hr = operation->Initialize(
        riid,
        timeoutMilliseconds,
        ComponentRootSPtr(),
        callback);

    if (FAILED(hr)) 
    { 
        return ComUtility::OnPublicApiReturn(hr); 
    }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ApplicationHostContainer::FabricEndGetCodePackageActivator(
    /* [in] */ __RPC__in_opt IFabricAsyncOperationContext *context,
    /* [retval][out] */ __RPC__deref_out_opt void **activator)
{
    if (context == NULL || activator == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    return ComUtility::OnPublicApiReturn(GetCodePackageActivatorComAsyncOperationContext::End(context, activator));
}

HRESULT ApplicationHostContainer::FabricGetCodePackageActivator(
    /* [in] */ __RPC__in REFIID riid,
    /* [retval][out] */ __RPC__deref_out_opt void **activator)
{
    if (activator == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    auto waiter = make_shared<ComAsyncOperationWaiter>();
    ComPointer<IFabricAsyncOperationContext> context;

    auto hr = ApplicationHostContainer::FabricBeginGetCodePackageActivator(
        riid,
        0, // load default from configuration
        waiter->GetAsyncOperationCallback([waiter, &activator](IFabricAsyncOperationContext * context)
        {
            auto hr = ApplicationHostContainer::FabricEndGetCodePackageActivator(context, activator);
            waiter->SetHRESULT(hr);
            waiter->Set();
        }),
        context.InitializationAddress());
    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }

    waiter->WaitOne();
    return ComUtility::OnPublicApiReturn(waiter->GetHRESULT());
}

HRESULT ApplicationHostContainer::FabricBeginGetNodeContext(
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ __RPC__in_opt IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ __RPC__deref_out_opt IFabricAsyncOperationContext **context)
{
    ErrorCode errorMTA = ComUtility::CheckMTA();
    if (!errorMTA.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(errorMTA.ToHResult());
    }

    ComPointer<GetNodeContextComAsyncOperationContext> operation = make_com<GetNodeContextComAsyncOperationContext>();

    HRESULT hr = operation->Initialize(
        timeoutMilliseconds,
        ComponentRootSPtr(),
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ApplicationHostContainer::FabricEndGetNodeContext(
    /* [in] */ __RPC__in_opt IFabricAsyncOperationContext *context,
    /* [retval][out] */ __RPC__deref_out_opt void ** nodeContext)
{
    if (context == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }
    if (nodeContext == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }
    auto hr = GetNodeContextComAsyncOperationContext::End(context, nodeContext);
    if(FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }
    return S_OK;
}

HRESULT ApplicationHostContainer::FabricGetNodeContext( 
    /* [retval][out] */ __RPC__deref_out_opt void ** nodeContext)
{
    if (nodeContext == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    auto waiter = make_shared<ComAsyncOperationWaiter>();
    ComPointer<IFabricAsyncOperationContext> context;

    HRESULT hr = ApplicationHostContainer::FabricBeginGetNodeContext(
        0, // use default from the configuration
        waiter->GetAsyncOperationCallback([waiter,&nodeContext](IFabricAsyncOperationContext * context)
        {
            auto hr = ApplicationHostContainer::FabricEndGetNodeContext(context, nodeContext);
            waiter->SetHRESULT(hr);
            waiter->Set();
        }),
        context.InitializationAddress());
    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }
    else
    {
        waiter->WaitOne();
        return ComUtility::OnPublicApiReturn(waiter->GetHRESULT());
    }
}
HRESULT ApplicationHostContainer::FabricRegisterCodePackageHost( 
    /* [in] */ __RPC__in_opt IUnknown *codePackageHost)
{
    if (codePackageHost == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_INVALIDARG);
    }

    ErrorCode errorMTA = ComUtility::CheckMTA();
    if (!errorMTA.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(errorMTA.ToHResult());
    }

    ComPointer<IFabricCodePackageHost> codePackageHostCPtr;
    auto hr = codePackageHost->QueryInterface(IID_IFabricCodePackageHost, codePackageHostCPtr.VoidInitializationAddress());
    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }

    ApplicationHostContainer & appHostContainer = ApplicationHostContainer::GetApplicationHostContainer();
    auto waiter = make_shared<AsyncOperationWaiter>();
    
    appHostContainer.BeginRegisterCodePackageHost(
        codePackageHostCPtr,
        HostingConfig::GetConfig().RegisterCodePackageHostTimeout, 
        [&appHostContainer, waiter] (AsyncOperationSPtr const & operation) 
        {
            auto error = appHostContainer.EndRegisterCodePackageHost(operation);
            waiter->SetError(error);
            waiter->Set();
        },
        AsyncOperationSPtr());

    waiter->WaitOne();
    auto error = waiter->GetError();
    return ComUtility::OnPublicApiReturn(error.ToHResult());
}

ErrorCode ApplicationHostContainer::GetNodeContext(__out FabricNodeContextResultImplSPtr & result)
{    
    ErrorCode error = ErrorCodeValue::NotFound;
    AcquireReadLock lock(lock_);
    if(host_)
    {
        FabricNodeContextSPtr nodeContext;
        error = host_->GetFabricNodeContext(nodeContext);

        if(error.IsSuccess())
        {
            result = make_shared<FabricNodeContextResult>(
                nodeContext->NodeId,
                nodeContext->NodeInstanceId,
                nodeContext->NodeName,
                nodeContext->NodeType,
                nodeContext->IPAddressOrFQDN,
                nodeContext->LogicalNodeDirectories);
        }
    }
    
    if(!error.IsSuccess())
    {
        error = ErrorCodeValue::HostingApplicationHostNotRegistered;
         WriteError(
            TraceType,
            "GetNodeContextFailed: Failed to get ApplicationHost {0}",
            error);
    }

    return error;
}

ErrorCode ApplicationHostContainer::StartTraceSessionInsideContainer()
{
    // Start will update only if necessary
    auto error = Common::TraceSession::Instance()->StartTraceSessions();
    if (!error.IsSuccess())
    {
        WriteError(TraceType, "Unable to start trace sessions: ErrorCode={0}", error);
        return error;
    }

    WriteNoise(TraceType, "Trace sessions successfully started");
    return error;
}


HRESULT ApplicationHostContainer::FabricCreateBackupRestoreAgent(
    /* [in] */ __RPC__in REFIID riid,
    /* [retval][out] */ __RPC__deref_out_opt void **backupRestoreAgent)
{
    if (backupRestoreAgent == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    auto & appHostContainer = GetApplicationHostContainer();
    auto result = appHostContainer.CreateBackupRestoreAgent(riid, backupRestoreAgent);
    return ComUtility::OnPublicApiReturn(result);
}

HRESULT ApplicationHostContainer::CreateBackupRestoreAgent(
    REFIID riid,
    void **backupRestoreAgent)
{
#if !defined(PLATFORM_UNIX)
    WriteInfo(TraceType, "Creating backup restore handler interface");
    ComPointer<ComFabricBackupRestoreAgent> comBackupRestoreAgent;
    this->host_->CreateBackupRestoreAgent(comBackupRestoreAgent);
    return comBackupRestoreAgent->QueryInterface(riid, backupRestoreAgent);
#else
    return E_NOTIMPL;
#endif
}
