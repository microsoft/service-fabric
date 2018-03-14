// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {4CC6E05E-E48F-4C28-B136-CB8C27A4B4BB}
    static const GUID CLSID_ComTransaction = 
        { 0x4cc6e05e, 0xe48f, 0x4c28, { 0xb1, 0x36, 0xcb, 0x8c, 0x27, 0xa4, 0xb4, 0xbb } };

    class ComTransaction :
        public IFabricTransaction,
        private ComTransactionBase
    {
        DENY_COPY(ComTransaction)

        BEGIN_COM_INTERFACE_LIST(ComTransaction)
            COM_INTERFACE_ITEM(IID_IFabricTransaction, IFabricTransaction)
            COM_INTERFACE_ITEM(CLSID_ComTransaction, ComTransaction)
            COM_DELEGATE_TO_BASE_ITEM(ComTransactionBase)
        END_COM_INTERFACE_LIST()

    public:
        ComTransaction(ITransactionPtr const & impl);
        virtual ~ComTransaction();

        ITransactionPtr const & get_Impl() const { return impl_; }

        // 
        // IFabricTransactionBase methods
        // 
        const FABRIC_TRANSACTION_ID *STDMETHODCALLTYPE get_Id( void);
        
        FABRIC_TRANSACTION_ISOLATION_LEVEL STDMETHODCALLTYPE get_IsolationLevel( void);

        // 
        // IFabricTransaction methods
        // 
        HRESULT STDMETHODCALLTYPE BeginCommit( 
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT STDMETHODCALLTYPE EndCommit( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ FABRIC_SEQUENCE_NUMBER *commitSequenceNumber);
        
        void STDMETHODCALLTYPE Rollback( void);

    private:
        class CommitOperationContext;

    private:
        ITransactionPtr impl_;
    };
}
