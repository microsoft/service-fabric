// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Common
{
    class KtlProxyAsyncOperation 
        : public AllocableAsyncOperation
    {
        DENY_COPY(KtlProxyAsyncOperation)

    public:

        virtual ~KtlProxyAsyncOperation();

    protected:

        KtlProxyAsyncOperation(
            KAsyncContextBase *ktlOperation,
            KAsyncContextBase* const ParentAsyncContext,
            AsyncCallback const& callback, 
            AsyncOperationSPtr const& parent)
            : AllocableAsyncOperation(callback, parent)
            , thisKtlOperation_(ktlOperation)
            , parentAsyncContext_(ParentAsyncContext)
        {
        }

        virtual NTSTATUS StartKtlAsyncOperation(
            __in KAsyncContextBase* const parentAsyncContext, 
            __in KAsyncContextBase::CompletionCallback callback) = 0;

        void OnStart(AsyncOperationSPtr const & thisSPtr) override;
        virtual void OnCancel() override;

        KAsyncContextBase::SPtr thisKtlOperation_;
    
    private:

        //
        // Callback for the KTL operation.
        //    
        void OnKtlOperationCompleted(__in KAsyncContextBase *const parent, __in KAsyncContextBase &context);
        
        //
        // The user registered callback is called from the system threadpool instead of on the thread
        // managed by KTL threadpool
        //
        void ExecuteCallback();
        AsyncOperationSPtr thisSPtr_;
        KAsyncContextBase* const parentAsyncContext_;
    };
}
