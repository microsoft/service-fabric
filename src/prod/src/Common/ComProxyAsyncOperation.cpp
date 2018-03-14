// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Common/ComProxyAsyncOperation.h"

namespace Common
{
    class ComProxyAsyncOperation::ComAsyncOperationCallback : 
        public IFabricAsyncOperationCallback,
        private ComUnknownBase
    {
        DENY_COPY(ComAsyncOperationCallback)

        COM_INTERFACE_LIST1(
            ComProxyAsyncOperation,
            IID_IFabricAsyncOperationCallback,
            IFabricAsyncOperationCallback)

    public:
        ComAsyncOperationCallback(
            ComProxyAsyncOperation & owner,
            AsyncOperationSPtr parent)
            : owner_(owner), parent_(parent), invoked_(false)
        {
        }

        virtual ~ComAsyncOperationCallback()
        {
            if (!invoked_.load())
            {
                parent_->Cancel();
            }
        }

        virtual void STDMETHODCALLTYPE ComAsyncOperationCallback::Invoke( 
            /* [in] */ IFabricAsyncOperationContext *context)
        {
            bool previous = invoked_.exchange(true);

            ASSERT_IF(CommonConfig::GetConfig().MultipleAsyncCallbackInvokeAssertEnabled && previous, "async callback has been invoked multiple times");
            owner_.OnComAsyncOperationCompleted(parent_, context);
        }
        
    private:
        ComProxyAsyncOperation & owner_;
        AsyncOperationSPtr parent_;
        atomic_bool invoked_;
    };
    
    ComProxyAsyncOperation::ComProxyAsyncOperation(
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent, 
        bool skipCompleteOnCancel)
        : AsyncOperation(callback, parent, skipCompleteOnCancel),
        lock_(),
        comAsyncOperationSPtr_()
    {
    }

    ComProxyAsyncOperation::~ComProxyAsyncOperation()
    {
    }

    void ComProxyAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        ComPointer<IFabricAsyncOperationCallback> callback = make_com<ComAsyncOperationCallback,IFabricAsyncOperationCallback>(
            *this, thisSPtr);

        ComPointer<IFabricAsyncOperationContext> cleanup;
        HRESULT hr = BeginComAsyncOperation(
            callback.GetRawPointer(), 
            cleanup.InitializationAddress());
        if (FAILED(hr)) 
        {
            TryComplete_HRESULT(thisSPtr, hr, true);
            return;
        }

        if (!cleanup)
        {
            TryComplete_HRESULT(thisSPtr, E_INVALIDARG, false);
            return;
        }
        
        if (cleanup->CompletedSynchronously())
        {
            FinishComAsyncOperation(thisSPtr, cleanup.GetRawPointer());
        }
        else
        {
            AcquireExclusiveLock grab(lock_);

            if (!this->InternalIsCompleted)
            {
                comAsyncOperationSPtr_ = std::make_shared<ComPointer<IFabricAsyncOperationContext>>(std::move(cleanup));
            }
        }
    }

    HRESULT ComProxyAsyncOperation::OnComAsyncOperationCompleted(
        AsyncOperationSPtr const & thisSPtr,
        IFabricAsyncOperationContext * context)
    {
        HRESULT hr = S_OK;

        if (context == NULL)
        {
            hr = E_INVALIDARG;
            TryComplete_HRESULT(thisSPtr, hr, false);
            return hr;
        }

        if (context->CompletedSynchronously())
        {
            return S_OK;
        }    
        else
        {
            return FinishComAsyncOperation(thisSPtr, context); 
        }
    }
    
    HRESULT ComProxyAsyncOperation::FinishComAsyncOperation(
        AsyncOperationSPtr const & thisSPtr, 
        IFabricAsyncOperationContext * context)
    {
        HRESULT hr = this->EndComAsyncOperation(context);
        
        if (FAILED(hr))
        {
            hr = TryComplete_HRESULT(thisSPtr, hr, true);
        }
        else
        {
            hr = TryComplete_HRESULT(thisSPtr, hr, false);
        }

        Cleanup();

        return hr;
    }

    HRESULT ComProxyAsyncOperation::TryComplete_HRESULT(
        AsyncOperationSPtr const & thisSPtr, 
        HRESULT hr,
        bool captureThreadErrorMessage)
    {
        TryComplete(thisSPtr, std::move(ErrorCode::FromHResult(hr, captureThreadErrorMessage)));

        return S_OK; // callback completed successfully
    }

    void ComProxyAsyncOperation::OnCancel()
    {
        std::shared_ptr<ComPointer<IFabricAsyncOperationContext>> comAsyncOperation;

        {
            AcquireExclusiveLock grab(lock_);

            comAsyncOperation = comAsyncOperationSPtr_;
        }

        if (comAsyncOperation)
        {
            comAsyncOperation->GetRawPointer()->Cancel();
        }
    }

    void ComProxyAsyncOperation::Cleanup()
    {
        AcquireExclusiveLock grab(lock_);

        comAsyncOperationSPtr_.reset();
    }
}
