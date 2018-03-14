// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;

// {468859c7-94fd-449a-b6d8-085dcb750c39}
static const GUID CLSID_OnDataLossAsyncOperation =
{ 0x468859c7, 0x94fd, 0x449a, { 0xb6, 0xd8, 0x08, 0x5d, 0xcb, 0x75, 0x0c, 0x39 } };

class ComDataLossHandler::OnDataLossAsyncOperation : public ComAsyncOperationContext
{
    DENY_COPY(OnDataLossAsyncOperation)

    COM_INTERFACE_AND_DELEGATE_LIST(
        OnDataLossAsyncOperation,
        CLSID_OnDataLossAsyncOperation,
        OnDataLossAsyncOperation,
        ComAsyncOperationContext)

public:

    OnDataLossAsyncOperation(ComDataLossHandler & owner) : owner_(owner)
    {
    }

    virtual ~OnDataLossAsyncOperation() {};

    HRESULT Initialize(__in IFabricAsyncOperationCallback * callback)
    {
        ComponentRootSPtr rootSPtr;
        auto hr = this->CreateComComponentRoot(rootSPtr);
        if (FAILED(hr)) { return hr; }

        hr = ComAsyncOperationContext::Initialize(rootSPtr, callback);
        if (FAILED(hr)) { return hr; }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, BOOLEAN * isStateChanged)
    {
        if (context == NULL || isStateChanged == NULL) { return E_POINTER; }

        ComPointer<OnDataLossAsyncOperation> thisOperation(context, CLSID_OnDataLossAsyncOperation);

        auto hr = thisOperation->ComAsyncOperationContextEnd();
        if (SUCCEEDED(hr))
        {
            *isStateChanged = thisOperation->isStateChanged_ ? TRUE : FALSE;
        }

        return hr;
   }

    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        owner_.impl_->BeginOnDataLoss(
            [this](AsyncOperationSPtr const & operation) { this->OnDataLossComplete(operation); },
            proxySPtr);
    }

private:
    void OnDataLossComplete(__in AsyncOperationSPtr const& operation)
    {
        auto error = owner_.impl_->EndOnDataLoss(operation, isStateChanged_);
        this->TryComplete(operation->Parent, error);
    }

    HRESULT ComAsyncOperationContextEnd()
    {
        return ComAsyncOperationContext::End();
    }

    HRESULT CreateComComponentRoot(__out ComponentRootSPtr & result)
    {
        ComPointer<IUnknown> rootCPtr;
        HRESULT hr = owner_.QueryInterface(IID_IUnknown, rootCPtr.VoidInitializationAddress());
        if (FAILED(hr)) { return hr; }

        result = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));

        return S_OK;
    }

    ComDataLossHandler & owner_;
    bool isStateChanged_;
};


// ********************************************************************************************************************
// ComDataLossHandler::ComDataLossHandler Implementation
//

ComDataLossHandler::ComDataLossHandler(IDataLossHandlerPtr const & impl)
    : IFabricDataLossHandler()
    , ComUnknownBase()
    , impl_(impl)
{
}

ComDataLossHandler::~ComDataLossHandler()
{
}

HRESULT ComDataLossHandler::BeginOnDataLoss(
    IFabricAsyncOperationCallback * callback,
    IFabricAsyncOperationContext ** context)
{
    if (context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<OnDataLossAsyncOperation> operation = make_com<OnDataLossAsyncOperation>(*this);

    auto hr = operation->Initialize(callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComDataLossHandler::EndOnDataLoss(
    IFabricAsyncOperationContext * context,
    BOOLEAN * isStateChanged)
{
    return ComUtility::OnPublicApiReturn(OnDataLossAsyncOperation::End(context, isStateChanged));
}
