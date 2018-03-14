// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace Transport;


static const GUID CLSID_ComFabricTransportListener_OpenOperationContext =
{ 0x1d6bdd3a, 0xf995, 0x4713, { 0x9a, 0xdb, 0x44, 0xff, 0xa3, 0x95, 0x1e, 0x82 } };

class ComFabricTransportListener::OpenOperationContext
    : public ComAsyncOperationContext
{
    DENY_COPY(OpenOperationContext);

    COM_INTERFACE_AND_DELEGATE_LIST(
        OpenOperationContext,
        CLSID_ComFabricTransportListener_OpenOperationContext,
        OpenOperationContext,
        ComAsyncOperationContext)

public:
    OpenOperationContext(__in ComFabricTransportListener & owner)
        : ComAsyncOperationContext(),
        owner_(owner)
    {
    }

    virtual ~OpenOperationContext()
    {
    }

    HRESULT Initialize(
        __in IFabricAsyncOperationCallback * callback)
    {
        ComPointer<IUnknown> rootCPtr;
        HRESULT hr = owner_.QueryInterface(IID_IUnknown, (void**)rootCPtr.InitializationAddress());
        
        if (FAILED(hr)) 
        {
            return hr;
        }

        auto root = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));
        hr = this->ComAsyncOperationContext::Initialize(root, callback);
        if (FAILED(hr)) 
        {
            return hr;
        }
        return hr;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, IFabricStringResult **serviceAddress)
    {
        if (context == NULL || serviceAddress == NULL)
        {
            return E_POINTER;
        }

        ComPointer<OpenOperationContext> thisOperation(context, CLSID_ComFabricTransportListener_OpenOperationContext);
        auto hr = thisOperation->Result;
        if (SUCCEEDED(hr))
        {
            ComPointer<IFabricStringResult> serviceAddressResult = make_com<ComStringResult, IFabricStringResult>(thisOperation->serviceAddress_);
            *serviceAddress = serviceAddressResult.DetachNoRelease();
        }

        return hr;
    }

protected:
    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = owner_.impl_->BeginOpen(
            [this](AsyncOperationSPtr const & operation)
        {
            this->FinishOpen(operation, false); 
        },
            proxySPtr);

        this->FinishOpen(operation, true);
    }

private:
    void FinishOpen(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.impl_->EndOpen(operation, serviceAddress_);
        TryComplete(operation->Parent, error.ToHResult());
    }

private:
    ComFabricTransportListener & owner_;
    wstring serviceAddress_;
};

static const GUID CLSID_ComFabricTransportListener_CloseOperationContext =
{ 0x194b007c, 0x6ad0, 0x41fc, { 0x97, 0x9f, 0x18, 0xf9, 0x81, 0xec, 0xed, 0xb3 } };

class ComFabricTransportListener::CloseOperationContext :
public ComAsyncOperationContext
{
    DENY_COPY(CloseOperationContext);

    COM_INTERFACE_AND_DELEGATE_LIST(
        CloseOperationContext,
        CLSID_ComFabricTransportListener_CloseOperationContext,
        CloseOperationContext,
        ComAsyncOperationContext)

public:
    CloseOperationContext(__in ComFabricTransportListener & owner)
        : ComAsyncOperationContext(),
        owner_(owner)
    {
    }

    virtual ~CloseOperationContext()
    {
    }

    HRESULT Initialize(
        __in IFabricAsyncOperationCallback * callback)
    {
        ComPointer<IUnknown> rootCPtr;
        HRESULT hr = owner_.QueryInterface(IID_IUnknown, (void**)rootCPtr.InitializationAddress());
        
        if (FAILED(hr))
        {
            return hr;
        }
        auto root = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));
        hr = this->ComAsyncOperationContext::Initialize(root, callback);
        
        if (FAILED(hr))
        {
            return hr;
        }
        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) 
        {
            return E_POINTER;
        }

        ComPointer<CloseOperationContext> thisOperation(context, CLSID_ComFabricTransportListener_CloseOperationContext);
        return thisOperation->Result;
    }

protected:
    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = owner_.impl_->BeginClose(
            [this](AsyncOperationSPtr const & operation)
        {
            this->FinishClose(operation, false); 
        },
            proxySPtr);

        this->FinishClose(operation, true);
    }

private:
    void FinishClose(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.impl_->EndClose(operation);
        TryComplete(operation->Parent, error.ToHResult());
    }

private:
    ComFabricTransportListener & owner_;
};


ComFabricTransportListener::ComFabricTransportListener(
    IServiceCommunicationListenerPtr const & impl)
    : IFabricTransportListener(),
    ComUnknownBase(),
    impl_(impl)
{

}


ComFabricTransportListener::~ComFabricTransportListener()
{
}

HRESULT ComFabricTransportListener::BeginOpen(
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    ComPointer<OpenOperationContext> operation
        = make_com<OpenOperationContext>(*this);
    auto hr = operation->Initialize(
        callback);

    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT  ComFabricTransportListener::EndOpen(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricStringResult **serviceAddress)
{
    return ComUtility::OnPublicApiReturn(OpenOperationContext::End(context, serviceAddress));
}

HRESULT  ComFabricTransportListener::BeginClose(
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    ComPointer<CloseOperationContext> operation
        = make_com<CloseOperationContext>(*this);
    auto hr = operation->Initialize(
        callback);

    if (FAILED(hr)) 
    {
        return ComUtility::OnPublicApiReturn(hr);
    }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT  ComFabricTransportListener::EndClose(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(CloseOperationContext::End(context));
}

void ComFabricTransportListener::Abort(void)
{
    impl_->AbortListener();
}
