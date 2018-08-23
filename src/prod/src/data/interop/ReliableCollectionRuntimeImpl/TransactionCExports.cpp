// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace TxnReplicator;
using namespace Data::Interop;
using namespace Data::Utilities;

extern "C" void Transaction_Release(
    __in TransactionHandle txn)
{
    Transaction::SPtr txnSPtr;
    txnSPtr.Attach((Transaction*)txn);
}

extern "C" void Transaction_Dispose(
    __in TransactionHandle txn)
{
    ((Transaction*)txn)->Dispose();
}

extern "C" void Transaction_AddRef(
    __in TransactionHandle txn)
{
    Transaction::SPtr txnSPtr((Transaction*)txn);
    txnSPtr.Detach();
}

extern "C" HRESULT Transaction_Abort(
    __in TransactionHandle txn)
{
    NTSTATUS status = ((Transaction*)txn)->Abort();
    return StatusConverter::ToHResult(status);
}

ktl::Task TransactionCommitAsyncInternal(
    Transaction* txn, 
    fnNotifyAsyncCompletion callback,
    void* ctx,
    NTSTATUS& status,
    BOOL& synchronouscomplete)
{
    status = STATUS_SUCCESS;
    synchronouscomplete = false;

    auto awaitable = txn->CommitAsync();
    if (IsComplete(awaitable))
    {
        synchronouscomplete = true;
        status = co_await awaitable;
        co_return;
    }

    NTSTATUS ntstatus = co_await awaitable;
    callback(ctx, StatusConverter::ToHResult(ntstatus));
}

extern "C" HRESULT Transaction_CommitAsync(
    __in TransactionHandle txn,
    __in fnNotifyAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    NTSTATUS status;
    TransactionCommitAsyncInternal((Transaction*)txn, callback, ctx, status, *synchronousComplete);
    return StatusConverter::ToHResult(status);
}

struct Transaction_Info_v1
{
    uint32_t Size;
    int64_t CommitSequenceNumber;
    int64_t Id;
};

extern "C" HRESULT Transaction_GetInfo(__in TransactionHandle txnHandle, __out Transaction_Info *info)
{
    auto txn = reinterpret_cast<Transaction *>(txnHandle);

    if (info->Size >= sizeof(Transaction_Info_v1))
    {
        info->CommitSequenceNumber = txn->CommitSequenceNumber;
        info->Id = txn->TransactionId;
    }

    return S_OK;
}

ktl::Task TransactionGetVisibilitySequenceNumberAsyncInternal(
    Transaction* txn, 
    fnNotifyGetVisibilitySequenceNumberCompletion callback,
    void* ctx,
    NTSTATUS& status,
    int64_t &sequenceNumber,
    BOOL& synchronouscomplete)
{
    status = STATUS_SUCCESS;
    synchronouscomplete = false;

    FABRIC_SEQUENCE_NUMBER vsn;
    auto awaitable = txn->GetVisibilitySequenceNumberAsync(vsn);
    if (IsComplete(awaitable))
    {
        synchronouscomplete = true;
        sequenceNumber = vsn;
        status = co_await awaitable;
        co_return;
    }

    NTSTATUS ntstatus = co_await awaitable;
    callback(ctx, StatusConverter::ToHResult(ntstatus), vsn);
}

extern "C" HRESULT Transaction_GetVisibilitySequenceNumberAsync(
    __in TransactionHandle txnHandle,
    __out int64_t *sequenceNumber,
    __in fnNotifyGetVisibilitySequenceNumberCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    if (sequenceNumber == nullptr || synchronousComplete == nullptr)
        return E_INVALIDARG;

    NTSTATUS status;
    auto txn = reinterpret_cast<Transaction *>(txnHandle);    
    TransactionGetVisibilitySequenceNumberAsyncInternal(txn, callback, ctx, status, *sequenceNumber, *synchronousComplete);
    return StatusConverter::ToHResult(status);
}


 
