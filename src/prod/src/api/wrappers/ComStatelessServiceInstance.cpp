// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;

// ********************************************************************************************************************
// ComStatelessServiceInstance::OpenOperationContext Implementation
//

// {1D6BDD3A-F995-4713-9ADB-44FFA3951E82}
static const GUID CLSID_ComStatelessServiceInstance_OpenOperationContext = 
{ 0x1d6bdd3a, 0xf995, 0x4713, { 0x9a, 0xdb, 0x44, 0xff, 0xa3, 0x95, 0x1e, 0x82 } };

class ComStatelessServiceInstance::OpenOperationContext :
    public ComAsyncOperationContext
{
    DENY_COPY(OpenOperationContext);

    COM_INTERFACE_AND_DELEGATE_LIST(
        OpenOperationContext, 
        CLSID_ComStatelessServiceInstance_OpenOperationContext,
        OpenOperationContext,
        ComAsyncOperationContext)

    public:
        OpenOperationContext(__in ComStatelessServiceInstance & owner) 
            : ComAsyncOperationContext(),
            owner_(owner),
            partition_(),
            serviceAddress_()
        {
        }

        virtual ~OpenOperationContext()
        {
        }

        HRESULT Initialize(
            __in ComponentRootSPtr const & rootSPtr,
            __in IFabricStatelessServicePartition *partition,
            __in IFabricAsyncOperationCallback * callback)
        {
            HRESULT hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
            if (FAILED(hr)) { return hr; }

            if (partition == NULL) { return E_POINTER; }

            partition_.SetAndAddRef(partition);
            return S_OK;
        }

        static HRESULT End(__in IFabricAsyncOperationContext * context, IFabricStringResult **serviceAddress)
        {
            if (context == NULL || serviceAddress == NULL) { return E_POINTER; }

            ComPointer<OpenOperationContext> thisOperation(context, CLSID_ComStatelessServiceInstance_OpenOperationContext);
            auto hr = thisOperation->Result;
             if (SUCCEEDED(hr))
            {
                ComPointer<IFabricStringResult> serviceAddressResult = make_com<ComStringResult, IFabricStringResult>(thisOperation->serviceAddress_);
                hr = serviceAddressResult->QueryInterface(IID_IFabricStringResult, reinterpret_cast<void**>(serviceAddress));
            }

            return hr;
        }

    protected:
        virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
        {
            auto operation = owner_.impl_->BeginOpen(
                partition_,
                [this](AsyncOperationSPtr const & operation){ this->FinishOpen(operation, false); },
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
        ComStatelessServiceInstance & owner_;
        ComPointer<IFabricStatelessServicePartition> partition_;
        wstring serviceAddress_;
};


// ********************************************************************************************************************
// ComStatelessServiceInstance::CloseOperationContext Implementation
//

// {194B007C-6AD0-41FC-979F-18F981ECEDB3}
static const GUID CLSID_ComStatelessServiceInstance_CloseOperationContext = 
{ 0x194b007c, 0x6ad0, 0x41fc, { 0x97, 0x9f, 0x18, 0xf9, 0x81, 0xec, 0xed, 0xb3 } };

class ComStatelessServiceInstance::CloseOperationContext :
    public ComAsyncOperationContext
{
    DENY_COPY(CloseOperationContext);

    COM_INTERFACE_AND_DELEGATE_LIST(
        CloseOperationContext, 
        CLSID_ComStatelessServiceInstance_CloseOperationContext,
        CloseOperationContext,
        ComAsyncOperationContext)

    public:
        CloseOperationContext(__in ComStatelessServiceInstance & owner) 
            : ComAsyncOperationContext(),
            owner_(owner)
        {
        }

        virtual ~CloseOperationContext()
        {
        }

        HRESULT Initialize(
            __in ComponentRootSPtr const & rootSPtr,
            __in IFabricAsyncOperationCallback * callback)
        {
            HRESULT hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
            if (FAILED(hr)) { return hr; }

            return S_OK;
        }

        static HRESULT End(__in IFabricAsyncOperationContext * context)
        {
            if (context == NULL) { return E_POINTER; }

            ComPointer<CloseOperationContext> thisOperation(context, CLSID_ComStatelessServiceInstance_CloseOperationContext);
            return thisOperation->Result;
        }

    protected:
        virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
        {
            auto operation = owner_.impl_->BeginClose(
                [this](AsyncOperationSPtr const & operation){ this->FinishOpen(operation, false); },
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

            auto error = owner_.impl_->EndClose(operation);
            TryComplete(operation->Parent, error.ToHResult());
        }

    private:
        ComStatelessServiceInstance & owner_;
};


// ********************************************************************************************************************
// ComStatelessServiceInstance Implementation
//

ComStatelessServiceInstance::ComStatelessServiceInstance(IStatelessServiceInstancePtr const & impl)
    : IFabricStatelessServiceInstance(),
    ComUnknownBase(),
    impl_(impl)
{
}

ComStatelessServiceInstance::~ComStatelessServiceInstance()
{
}

HRESULT ComStatelessServiceInstance::BeginOpen( 
    /* [in] */ IFabricStatelessServicePartition *partition,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    ComPointer<IUnknown> rootCPtr;
    HRESULT hr = this->QueryInterface(IID_IUnknown, (void**)rootCPtr.InitializationAddress());
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    auto root = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));

    ComPointer<OpenOperationContext> operation 
        = make_com<OpenOperationContext>(*this);
    hr = operation->Initialize(
        root,
        partition,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));

}

HRESULT ComStatelessServiceInstance::EndOpen( 
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricStringResult **serviceAddress)
{
    return ComUtility::OnPublicApiReturn(OpenOperationContext::End(context, serviceAddress));
}

HRESULT ComStatelessServiceInstance::BeginClose( 
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    ComPointer<IUnknown> rootCPtr;
    HRESULT hr = this->QueryInterface(IID_IUnknown, (void**)rootCPtr.InitializationAddress());
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    auto root = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));

    ComPointer<CloseOperationContext> operation 
        = make_com<CloseOperationContext>(*this);
    hr = operation->Initialize(
        root,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComStatelessServiceInstance::EndClose( 
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(CloseOperationContext::End(context));
}

void ComStatelessServiceInstance::Abort(void)
{
    impl_->Abort();
}
