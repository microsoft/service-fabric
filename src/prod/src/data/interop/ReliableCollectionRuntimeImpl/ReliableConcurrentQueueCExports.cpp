//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"

using namespace Data;
using namespace Data::TStore;
using namespace TxnReplicator;
using namespace Data::Interop;
using namespace Data::Utilities;
using namespace Data::Collections;

using Data::Utilities::IAsyncEnumerator;

ktl::Task ConcurrentQueueEnqueueAsyncInternal(
    IReliableConcurrentQueue<KBuffer::SPtr>* rcq, 
    Transaction* txn, 
    size_t objectHandle, 
    void* bytes, 
    uint32_t bytesLength, 
    int64 timeout, 
    ktl::CancellationTokenSource** cts,
    fnNotifyAsyncCompletion callback,
    void* ctx, 
    NTSTATUS& status,
    BOOL& synchronousComplete)
{
    ULONG kBufferLength;
    KString::SPtr kstringkey;
    KBuffer::SPtr bufferSptr;
    ktl::CancellationToken cancellationToken = ktl::CancellationToken::None;
    ktl::CancellationTokenSource::SPtr cancellationTokenSource;

    status = STATUS_SUCCESS;
    synchronousComplete = false;
    kBufferLength = bytesLength;

#ifdef FEATURE_CACHE_OBJHANDLE
    kBufferLength += sizeof(size_t);
#endif

    status = KBuffer::Create(kBufferLength, bufferSptr, txn->GetThisAllocator());
    CO_RETURN_VOID_ON_FAILURE(status);

    auto buffer = bufferSptr->GetBuffer();
#ifdef FEATURE_CACHE_OBJHANDLE
    *(size_t*)buffer = objectHandle;
    buffer = (byte*)buffer + sizeof(size_t);
#endif
    memcpy(buffer, bytes, bytesLength);

    if (cts != nullptr)
    {
        status = ktl::CancellationTokenSource::Create(txn->GetThisAllocator(), RELIABLECOLLECTIONRUNTIME_TAG, cancellationTokenSource);
        CO_RETURN_VOID_ON_FAILURE(status);
        cancellationToken = cancellationTokenSource->Token;
    }

    auto awaitable = rcq->EnqueueAsync(*txn, bufferSptr, Common::TimeSpan::FromTicks(timeout), cancellationToken);

    if (IsComplete(awaitable))
    {
        synchronousComplete = true;
        EXCEPTION_TO_STATUS(co_await awaitable, status);
        co_return;
    }

    if (cts != nullptr)
        *cts = cancellationTokenSource.Detach();
    NTSTATUS ntstatus = STATUS_SUCCESS;

    EXCEPTION_TO_STATUS(co_await awaitable, ntstatus);

    callback(ctx, StatusConverter::ToHResult(ntstatus));
}

extern "C" HRESULT ConcurrentQueue_EnqueueAsync(
    __in StateProviderHandle stateProviderHandle,
    __in TransactionHandle txn,
    __in size_t objectHandle,
    __in void* bytes,
    __in uint32_t bytesLength,
    __in int64_t timeout,
    __out CancellationTokenSourceHandle* cts,
    __in fnNotifyAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    NTSTATUS status;

    auto stateProvider = reinterpret_cast<IStateProvider2*>(stateProviderHandle);
    auto rcq = dynamic_cast<IReliableConcurrentQueue<KBuffer::SPtr>*>(stateProvider);

    if (rcq == nullptr)
        return E_INVALIDARG;

    ConcurrentQueueEnqueueAsyncInternal(
        rcq,
        (Transaction*)txn,
        objectHandle, bytes, bytesLength, timeout,
        (ktl::CancellationTokenSource**)cts, 
        callback, ctx, status, *synchronousComplete);

    return StatusConverter::ToHResult(status);
}

ktl::Task ConcurrentQueueTryDequeueAsyncInternal(
    IReliableConcurrentQueue<KBuffer::SPtr>* rcq,
    Transaction* txn,
    int64 timeout,
    size_t* objectHandle,
    Buffer* value,
    ktl::CancellationTokenSource** cts,
    BOOL* succeeded,
    fnNotifyTryDequeueAsyncCompletion callback,
    void* ctx,
    NTSTATUS& status,
    BOOL& synchronousComplete)
{
    ktl::CancellationToken cancellationToken = ktl::CancellationToken::None;
    ktl::CancellationTokenSource::SPtr cancellationTokenSource = nullptr;

    status = STATUS_SUCCESS;
    synchronousComplete = false;

    if (cts != nullptr)
    {
        status = ktl::CancellationTokenSource::Create(txn->GetThisAllocator(), RELIABLECOLLECTIONRUNTIME_TAG, cancellationTokenSource);
        CO_RETURN_VOID_ON_FAILURE(status);
        cancellationToken = cancellationTokenSource->Token;
    }

    KBuffer::SPtr bufferSPtr;
    auto awaitable = rcq->TryDequeueAsync(*txn, bufferSPtr, Common::TimeSpan::FromTicks(timeout), cancellationToken);
    if (IsComplete(awaitable))
    {
        synchronousComplete = true;
        EXCEPTION_TO_STATUS(*succeeded = co_await awaitable, status);
        CO_RETURN_VOID_ON_FAILURE(status);

        if (*succeeded)
        {
            char* buffer = (char*)bufferSPtr->GetBuffer();
            uint32_t bufferLength = bufferSPtr->QuerySize();
#ifdef FEATURE_CACHE_OBJHANDLE
            *objectHandle = *(size_t*)buffer;
            buffer += sizeof(size_t);
            bufferLength -= sizeof(size_t);
#else 
            *objectHandle = 0;
#endif
            value->Bytes = buffer;
            value->Length = bufferLength;
            value->Handle = bufferSPtr.Detach();
        }

        co_return;
    }

    if (cts != nullptr)
        *cts = cancellationTokenSource.Detach();

    BOOL result = false;
    NTSTATUS ntstatus = STATUS_SUCCESS;

    EXCEPTION_TO_STATUS(result = co_await awaitable, ntstatus);
    if (result)
    {
        size_t objHandle = 0;
        byte* buffer = (byte*)bufferSPtr->GetBuffer();
        ULONG bufferLength = bufferSPtr->QuerySize();
#ifdef FEATURE_CACHE_OBJHANDLE
        objHandle = *(size_t*)buffer;
        buffer += sizeof(size_t);
        bufferLength -= sizeof(size_t);
#endif
        callback(ctx, StatusConverter::ToHResult(ntstatus), result, objHandle, buffer, bufferLength);
    }
    else
    {
        callback(ctx, StatusConverter::ToHResult(ntstatus), result, 0, nullptr, 0);
    }
}

extern "C" HRESULT ConcurrentQueue_TryDequeueAsync(
    __in StateProviderHandle stateProviderHandle,
    __in TransactionHandle txn,
    __in int64_t timeout,
    __out size_t* objectHandle,
    __out Buffer* value,
    __out CancellationTokenSourceHandle* cts,
    __out BOOL* succeeded,
    __in fnNotifyTryDequeueAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    NTSTATUS status;

    auto stateProvider = reinterpret_cast<IStateProvider2*>(stateProviderHandle);
    auto rcq = dynamic_cast<IReliableConcurrentQueue<KBuffer::SPtr>*>(stateProvider);
    if (rcq == nullptr)
        return E_INVALIDARG;

    ConcurrentQueueTryDequeueAsyncInternal(
        rcq,
        (Transaction*)txn, 
        timeout,
        objectHandle,
        value,
        (ktl::CancellationTokenSource**)cts, 
        succeeded,
        callback,
        ctx, 
        status,
        *synchronousComplete);

    return StatusConverter::ToHResult(status);
}

extern "C" HRESULT ConcurrentQueue_GetCount(
    __in StateProviderHandle stateProviderHandle,
    __out int64_t* count)
{
    NTSTATUS status = STATUS_SUCCESS;

    auto stateProvider = reinterpret_cast<IStateProvider2*>(stateProviderHandle);
    auto rcq = dynamic_cast<IReliableConcurrentQueue<KBuffer::SPtr>*>(stateProvider);
    if (rcq == nullptr)
        return E_INVALIDARG;

    EXCEPTION_TO_STATUS(*count = rcq->GetQueueCount(), status);

    return StatusConverter::ToHResult(status);
}
