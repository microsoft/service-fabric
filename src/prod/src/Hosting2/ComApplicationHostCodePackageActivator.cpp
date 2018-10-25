// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

// {D067B0C9-0710-4A37-A28C-D5A974036771}
static const GUID CLSID_ActivateCodePackageComAsyncOperationContext =
{ 0xd067b0c9, 0x710, 0x4a37,{ 0xa2, 0x8c, 0xd5, 0xa9, 0x74, 0x3, 0x67, 0x71 } };

class ComApplicationHostCodePackageActivator::ActivateCodePackageComAsyncOperationContext
    : public ComAsyncOperationContext
{
    DENY_COPY(ActivateCodePackageComAsyncOperationContext)

    COM_INTERFACE_LIST2(
        ActivateCodePackageComAsyncOperationContext,
        IID_IFabricAsyncOperationContext,
        IFabricAsyncOperationContext,
        CLSID_ActivateCodePackageComAsyncOperationContext,
        ActivateCodePackageComAsyncOperationContext)

public:
    explicit ActivateCodePackageComAsyncOperationContext(_In_ ComApplicationHostCodePackageActivator & owner)
        : ComAsyncOperationContext()
        , owner_(owner)
        , codePackageNames_()
        , timeout_()
    {
    }

    virtual ~ActivateCodePackageComAsyncOperationContext()
    {
    }

    HRESULT STDMETHODCALLTYPE Initialize(
        __in ComponentRootSPtr const & rootSPtr,
        __in FABRIC_STRING_LIST *codePackageNames,
        __in FABRIC_STRING_MAP *environment,
        __in DWORD timeoutMilliseconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        auto hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
        if (FAILED(hr))
        { 
            return hr; 
        }

        if (codePackageNames == nullptr)
        { 
            return E_INVALIDARG; 
        }
        
        auto error = StringList::FromPublicApi(*codePackageNames, codePackageNames_);
        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        if (environment != nullptr)
        {
            error = PublicApiHelper::FromPublicApiStringMap(*environment, environment_);
            if (!error.IsSuccess())
            {
                return error.ToHResult();
            }
        }

        timeout_ = owner_.GetTimeSpan(timeoutMilliseconds);

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<ActivateCodePackageComAsyncOperationContext> thisOperation(
            context, CLSID_ActivateCodePackageComAsyncOperationContext);
        
        return thisOperation->Result;
    }

protected:
    void OnStart(_In_ AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = owner_.activatorImpl_->BeginActivateCodePackage(
            codePackageNames_,
            move(environment_),
            timeout_,
            [this](AsyncOperationSPtr const & operation) 
            { 
                this->FinishActivateCodePackage(operation, false);
            },
            proxySPtr);

        this->FinishActivateCodePackage(operation, true);
    }

private:

    void FinishActivateCodePackage(AsyncOperationSPtr const & operation, bool expectCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.activatorImpl_->EndActivateCodePackage(operation);
        
        this->TryComplete(operation->Parent, error.ToHResult());
    }

private:
    ComApplicationHostCodePackageActivator & owner_;
    TimeSpan timeout_;
    vector<wstring> codePackageNames_;
    EnvironmentMap environment_;
};

// {DF27D9A9-24FE-4B9B-B9A1-856AF7DFC178}
static const GUID CLSID_DeactivateCodePackageComAsyncOperationContext =
{ 0xdf27d9a9, 0x24fe, 0x4b9b,{ 0xb9, 0xa1, 0x85, 0x6a, 0xf7, 0xdf, 0xc1, 0x78 } };

class ComApplicationHostCodePackageActivator::DeactivateCodePackageComAsyncOperationContext
    : public ComAsyncOperationContext
{
    DENY_COPY(DeactivateCodePackageComAsyncOperationContext)

        COM_INTERFACE_LIST2(
            DeactivateCodePackageComAsyncOperationContext,
            IID_IFabricAsyncOperationContext,
            IFabricAsyncOperationContext,
            CLSID_DeactivateCodePackageComAsyncOperationContext,
            DeactivateCodePackageComAsyncOperationContext)

public:
    explicit DeactivateCodePackageComAsyncOperationContext(_In_ ComApplicationHostCodePackageActivator & owner)
        : ComAsyncOperationContext()
        , owner_(owner)
        , codePackageNames_()
        , timeout_()
    {
    }

    virtual ~DeactivateCodePackageComAsyncOperationContext()
    {
    }

    HRESULT STDMETHODCALLTYPE Initialize(
        __in ComponentRootSPtr const & rootSPtr,
        __in FABRIC_STRING_LIST *codePackageNames,
        __in DWORD timeoutMilliseconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        auto hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
        if (FAILED(hr))
        {
            return hr;
        }

        if (codePackageNames == nullptr)
        {
            return E_INVALIDARG;
        }

        auto error = StringList::FromPublicApi(*codePackageNames, codePackageNames_);
        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        timeout_ = owner_.GetTimeSpan(timeoutMilliseconds);

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<DeactivateCodePackageComAsyncOperationContext> thisOperation(
            context, CLSID_DeactivateCodePackageComAsyncOperationContext);

        return thisOperation->Result;
    }

protected:
    void OnStart(_In_ AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = owner_.activatorImpl_->BeginDeactivateCodePackage(
            codePackageNames_,
            timeout_,
            [this](AsyncOperationSPtr const & operation)
            {
                this->FinishDeactivateCodePackage(operation, false);
            },
            proxySPtr);

        this->FinishDeactivateCodePackage(operation, true);
    }

private:

    void FinishDeactivateCodePackage(AsyncOperationSPtr const & operation, bool expectCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.activatorImpl_->EndDeactivateCodePackage(operation);

        this->TryComplete(operation->Parent, error.ToHResult());
    }

private:
    ComApplicationHostCodePackageActivator & owner_;
    TimeSpan timeout_;
    vector<wstring> codePackageNames_;
};

ComApplicationHostCodePackageActivator::ComApplicationHostCodePackageActivator(
    ApplicationHostCodePackageActivatorSPtr const & activatorImpl)
    : ITentativeApplicationHostCodePackageActivator()
    , ComUnknownBase()
    , objectId_(Guid::NewGuid().ToString())
    , activatorImpl_(activatorImpl)
{
    activatorImpl_->RegisterComCodePackageActivator(objectId_, *this);
}

ComApplicationHostCodePackageActivator::~ComApplicationHostCodePackageActivator()
{
    activatorImpl_->UnregisterComCodePackageActivator(objectId_);
}

ULONG  ComApplicationHostCodePackageActivator::TryAddRef()
{
    return this->BaseTryAddRef();
}

HRESULT ComApplicationHostCodePackageActivator::BeginActivateCodePackage(
    /* [in] */ FABRIC_STRING_LIST *codePackageNames,
    /* [in] */ FABRIC_STRING_MAP *environment,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    ComPointer<IUnknown> rootCPtr;
    auto hr = this->QueryInterface(IID_IUnknown, (void**)rootCPtr.InitializationAddress());
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    auto root = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));

    auto operation = make_com<ActivateCodePackageComAsyncOperationContext>(*this);
    hr = operation->Initialize(
        root,
        codePackageNames,
        environment,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) 
    { 
        return ComUtility::OnPublicApiReturn(hr); 
    }

    return ComUtility::OnPublicApiReturn(
        ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComApplicationHostCodePackageActivator::EndActivateCodePackage(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(
        ActivateCodePackageComAsyncOperationContext::End(context));
}

HRESULT ComApplicationHostCodePackageActivator::BeginDeactivateCodePackage(
    /* [in] */ FABRIC_STRING_LIST *codePackageNames,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    ComPointer<IUnknown> rootCPtr;
    auto hr = this->QueryInterface(IID_IUnknown, (void**)rootCPtr.InitializationAddress());
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    auto root = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));

    auto operation = make_com<DeactivateCodePackageComAsyncOperationContext>(*this);
    hr = operation->Initialize(
        root,
        codePackageNames,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }

    return ComUtility::OnPublicApiReturn(
        ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComApplicationHostCodePackageActivator::EndDeactivateCodePackage(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(
        DeactivateCodePackageComAsyncOperationContext::End(context));
}

HRESULT ComApplicationHostCodePackageActivator::AbortCodePackage(
    /* [in] */ FABRIC_STRING_LIST *codePackageNames)
{
    if (codePackageNames == nullptr || codePackageNames->Count == 0)
    {
        return E_INVALIDARG;
    }

    vector<wstring> codePackageNameList;
    auto error = StringList::FromPublicApi(*codePackageNames, codePackageNameList);
    if (!error.IsSuccess())
    {
        return error.ToHResult();
    }

    activatorImpl_->AbortCodePackage(codePackageNameList);

    return S_OK;
}

HRESULT ComApplicationHostCodePackageActivator::RegisterCodePackageEventHandler(
    /* [in] */ IFabricCodePackageEventHandler *eventHandler,
    /* [retval][out] */ ULONGLONG *callbackHandle)
{
    if (eventHandler == NULL || callbackHandle == NULL)
    { 
        return ComUtility::OnPublicApiReturn(E_POINTER); 
    }

    ComPointer<IFabricCodePackageEventHandler> handlerCPtr;
    handlerCPtr.SetAndAddRef(eventHandler);

    auto handleId = activatorImpl_->RegisterCodePackageEventHandler(
        handlerCPtr,
        objectId_);

    *callbackHandle = handleId;

    return S_OK;
}

HRESULT ComApplicationHostCodePackageActivator::UnregisterCodePackageEventHandler(
    /* [in] */ ULONGLONG callbackHandle)
{
    activatorImpl_->UnregisterCodePackageEventHandler(callbackHandle);

    return S_OK;
}

TimeSpan ComApplicationHostCodePackageActivator::GetTimeSpan(DWORD timeMilliSec)
{
    if (timeMilliSec == INFINITE)
    {
        return TimeSpan::MaxValue;
    }

    if (timeMilliSec == 0)
    {
        return HostingConfig::GetConfig().ActivationTimeout;
    }
    
    return TimeSpan::FromMilliseconds(static_cast<double>(timeMilliSec));
}

