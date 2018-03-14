// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;

// ********************************************************************************************************************
// ComTransaction::CommitOperationContext Implementation
//

// {08DC65A4-0454-4797-88DC-CDF0BE12D895}
static const GUID CLSID_ComTransaction_CommitOperationContext = 
{ 0x8dc65a4, 0x454, 0x4797, { 0x88, 0xdc, 0xcd, 0xf0, 0xbe, 0x12, 0xd8, 0x95 } };


class ComTransaction::CommitOperationContext :
    public ComAsyncOperationContext
{
    DENY_COPY(CommitOperationContext);

    COM_INTERFACE_AND_DELEGATE_LIST(
        CommitOperationContext, 
        CLSID_ComTransaction_CommitOperationContext,
        CommitOperationContext,
        ComAsyncOperationContext)

    public:
        CommitOperationContext(__in ComTransaction & owner) 
            : ComAsyncOperationContext(),
            owner_(owner),
            commitSequenceNumber_(),
            timeoutMilliseconds_()
        {
        }

        virtual ~CommitOperationContext()
        {
        }

        HRESULT Initialize(
            __in ComponentRootSPtr const & rootSPtr,
            __in DWORD timeoutMilliseconds,
            __in IFabricAsyncOperationCallback * callback)
        {
            HRESULT hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
            if (FAILED(hr)) { return hr; }

            timeoutMilliseconds_ = timeoutMilliseconds;

            return S_OK;
        }

        static HRESULT End(
            __in IFabricAsyncOperationContext * context, 
            __out FABRIC_SEQUENCE_NUMBER *commitSequenceNumber)
        {
            if (context == NULL || commitSequenceNumber == NULL) { return E_POINTER; }

            ComPointer<CommitOperationContext> thisOperation(context, CLSID_ComTransaction_CommitOperationContext);
            auto hr = thisOperation->Result;
            if (SUCCEEDED(hr))
            {
                *commitSequenceNumber = thisOperation->commitSequenceNumber_;
            }

            return hr;
        }

        virtual HRESULT STDMETHODCALLTYPE Cancel() override
        {
            // Intentional no-op. 
            // We do not support cancelling a commit operation as it can can cause the underlying
            // ESE transaction to rollback after replication has already started. In some cases it 
            // can stuck GetNextCopyState (#6383384) resulting in stuck replica build and re-configuration.
            return ComUtility::OnPublicApiReturn(S_OK);
        }

    protected:
        virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
        {
            auto operation = owner_.impl_->BeginCommit(
                TimeSpan::FromMilliseconds(timeoutMilliseconds_),
                [this](AsyncOperationSPtr const & operation){ this->FinishCommit(operation, false); },
                proxySPtr);

            this->FinishCommit(operation, true);
        }

    private:
        void FinishCommit(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously)
            {
                return;
            }

            auto error = owner_.impl_->EndCommit(operation, commitSequenceNumber_);
            TryComplete(operation->Parent, error.ToHResult());
        }

    private:
        ComTransaction & owner_;
        FABRIC_SEQUENCE_NUMBER commitSequenceNumber_;
        DWORD timeoutMilliseconds_;
};

// ********************************************************************************************************************
// ComTransaction::ComTransaction Implementation
//

ComTransaction::ComTransaction(ITransactionPtr const & impl)
    : IFabricTransaction(),
    ComTransactionBase(ITransactionBasePtr((ITransactionBase*)impl.get(), impl.get_Root())),
    impl_(impl)
{
}

ComTransaction::~ComTransaction()
{
}

const FABRIC_TRANSACTION_ID * ComTransaction::get_Id( void)
{
    return ComTransactionBase::get_Id();
}
        
FABRIC_TRANSACTION_ISOLATION_LEVEL ComTransaction::get_IsolationLevel( void)
{
    return ComTransactionBase::get_IsolationLevel();
}

HRESULT ComTransaction::BeginCommit( 
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    ComPointer<IUnknown> rootCPtr;
    HRESULT hr = this->QueryInterface(IID_IUnknown, (void**)rootCPtr.InitializationAddress());
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    auto root = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));

    ComPointer<CommitOperationContext> operation 
        = make_com<CommitOperationContext>(*this);
    hr = operation->Initialize(
        root,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComTransaction::EndCommit( 
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ FABRIC_SEQUENCE_NUMBER *commitSequenceNumber)
{
    return ComUtility::OnPublicApiReturn(CommitOperationContext::End(context, commitSequenceNumber));
}

void ComTransaction::Rollback(void)
{
    impl_->Rollback();
}
