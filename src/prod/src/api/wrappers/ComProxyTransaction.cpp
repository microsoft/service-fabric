// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;

// ********************************************************************************************************************
// ComProxyTransaction::CommitAsyncOperation Implementation
//
class ComProxyTransaction::CommitAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(CommitAsyncOperation)

public:
    CommitAsyncOperation(
        ComProxyTransaction & owner,
        TimeSpan const & timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent),
        owner_(owner),
        timeout_(timeout)
    {
    }

    virtual ~CommitAsyncOperation() 
    { 
    }

    FABRIC_SEQUENCE_NUMBER const get_CommitSequenceNumber() const
    {
        return sequenceNumber_;
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out ::FABRIC_SEQUENCE_NUMBER & commitSequenceNumber)
    {
        auto thisPtr = AsyncOperation::End<CommitAsyncOperation>(operation);
        commitSequenceNumber = thisPtr->sequenceNumber_;

        return thisPtr->Error;
    }

protected:
     HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
     {
         return owner_.comImpl_->BeginCommit(
             (DWORD)timeout_.TotalMilliseconds(),
             callback,
             context);
     }

     HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
     {
         return owner_.comImpl_->EndCommit(context, &sequenceNumber_);
     }

private:
    ComProxyTransaction & owner_;
    TimeSpan const timeout_;
    FABRIC_SEQUENCE_NUMBER sequenceNumber_;
};


// ********************************************************************************************************************
// ComProxyTransaction Implementation
//
ComProxyTransaction::ComProxyTransaction(
    ComPointer<IFabricTransaction> const & comImpl)
    : ComProxyTransactionBase(ComPointer<IFabricTransactionBase>(comImpl, IID_IFabricTransactionBase)),
    ITransaction(),
    comImpl_(comImpl)
{
}

ComProxyTransaction::~ComProxyTransaction()
{
}

Guid ComProxyTransaction::get_Id()
{
    return ComProxyTransactionBase::get_Id();
}

FABRIC_TRANSACTION_ISOLATION_LEVEL ComProxyTransaction::get_IsolationLevel()
{
    return ComProxyTransactionBase::get_IsolationLevel();
}

AsyncOperationSPtr ComProxyTransaction::BeginCommit(
    TimeSpan const & timeout,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<CommitAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyTransaction::EndCommit(
    AsyncOperationSPtr const & asyncOperation, 
    __out ::FABRIC_SEQUENCE_NUMBER & commitSequenceNumber)
{
    return CommitAsyncOperation::End(asyncOperation, commitSequenceNumber);
}

void ComProxyTransaction::Rollback()
{
    comImpl_->Rollback();
}
