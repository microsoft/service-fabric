// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Common
{
    class ComAsyncOperationWaiter::ComAsyncOperationCallback : 
        public IFabricAsyncOperationCallback,
        private ComUnknownBase
    {
        DENY_COPY(ComAsyncOperationCallback)

        COM_INTERFACE_LIST1(
            ComProxyAsyncOperation,
            IID_IFabricAsyncOperationCallback,
            IFabricAsyncOperationCallback)

    public:
        ComAsyncOperationCallback()
        {
        }

        virtual ~ComAsyncOperationCallback()
        {
        }

        void SetUserCallback(ComAsyncCallback const & userCallback)
        {
            userCallback_ = userCallback;
        }

        virtual void STDMETHODCALLTYPE ComAsyncOperationCallback::Invoke( 
            /* [in] */ IFabricAsyncOperationContext *context)
        {
            ASSERT_IF(userCallback_ == nullptr, "UserCallback must be valid.");
            userCallback_(context);
            userCallback_ = nullptr;
        }
        
    private:
        ComAsyncCallback userCallback_;
    };
    
    ComAsyncOperationWaiter::ComAsyncOperationWaiter()
        : ManualResetEvent(false),
        callback_()
    {
        callback_ = make_com<ComAsyncOperationCallback>();
    }

    ComAsyncOperationWaiter::~ComAsyncOperationWaiter()
    {
    }

    IFabricAsyncOperationCallback * ComAsyncOperationWaiter::GetAsyncOperationCallback(ComAsyncCallback const & userCallback)
    {
        callback_->SetUserCallback(userCallback);

#ifdef DBG
        return dynamic_cast<IFabricAsyncOperationCallback*>(callback_.GetRawPointer());
#else
        return static_cast<IFabricAsyncOperationCallback*>(callback_.GetRawPointer());
#endif
    }
}
