// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
using namespace Data;
using namespace Data::Utilities;
using namespace Data::Interop;

// TODO remove this
KAllocator& GetAllocator()
{
    KtlSystemCore* ktlsystem = KtlSystemCoreImp::TryGetDefaultKtlSystemCore();
    return ktlsystem->PagedAllocator();
}

void StoreKeyValueEnumeratorCurrent(
    __in IAsyncEnumerator<KeyValuePair<KString::SPtr, KeyValuePair<LONG64, KBuffer::SPtr>>>* enumerator,
    __out LPCWSTR* key,
    __out size_t *objectHandle,
    __out Buffer* value,
    __out LONG64* versionSequenceNumber)
{
    char* buffer;
    ULONG bufferLength;
    auto result = enumerator->GetCurrent();
    *key = static_cast<LPCWSTR>(*(result.Key));
    *versionSequenceNumber = result.Value.Key;
    KBuffer::SPtr kBufferSptr = result.Value.Value;
    buffer = (char*)kBufferSptr->GetBuffer();
    bufferLength = kBufferSptr->QuerySize();

#ifdef FEATURE_CACHE_OBJHANDLE
    *objectHandle = *(size_t*)buffer;
    buffer += sizeof(size_t);
    bufferLength -= sizeof(size_t);
#else
    *objectHandle = 0;
#endif

    value->Bytes = buffer;
    value->Length = bufferLength;
    value->Handle = kBufferSptr.RawPtr();
    kBufferSptr.Detach();
}

extern "C" void StoreKeyValueEnumerator_Release(
    __in StoreKeyValueAsyncEnumeratorHandle enumerator)
{
    IAsyncEnumerator<KeyValuePair<KString::SPtr, KeyValuePair<LONG64, KBuffer::SPtr>>>::SPtr enumeratorSPtr;
    enumeratorSPtr.Attach((IAsyncEnumerator<KeyValuePair<KString::SPtr, KeyValuePair<LONG64, KBuffer::SPtr>>>*)enumerator);
    enumeratorSPtr->Dispose();
}

ktl::Task StoreKeyValueEnumeratorMoveNextAsyncInternal(
    __in IAsyncEnumerator<KeyValuePair<KString::SPtr, KeyValuePair<LONG64, KBuffer::SPtr>>>* enumerator,
    __out ktl::CancellationTokenSource** cts,
    __out BOOL* advanced,
    __out LPCWSTR* key,
    __out size_t *objectHandle,
    __out Buffer* value,
    __out LONG64* versionSequenceNumber,
    __in fnNotifyStoreKeyValueEnumeratorMoveNextAsyncCompletion callback,
    __in void* ctx,
    __out NTSTATUS& status,
    __out BOOL& synchronousComplete)
{
    ktl::CancellationToken cancellationToken = ktl::CancellationToken::None;
    ktl::CancellationTokenSource::SPtr cancellationTokenSource = nullptr;
    bool returnKeyValues = true;

    if (key == nullptr)
        returnKeyValues = false;

    status = STATUS_SUCCESS;
    synchronousComplete = false;

    if (cts != nullptr)
    {
        status = ktl::CancellationTokenSource::Create(GetAllocator(), RELIABLECOLLECTIONRUNTIME_TAG, cancellationTokenSource);
        CO_RETURN_VOID_ON_FAILURE(status);
        cancellationToken = cancellationTokenSource->Token;
    }

    auto awaitable = enumerator->MoveNextAsync(cancellationToken);
    if (IsComplete(awaitable))
    {
        synchronousComplete = true;
        EXCEPTION_TO_STATUS(*advanced = co_await awaitable, status);
        // if key is null then don't care about key values
        if (returnKeyValues && *advanced)
        {
            StoreKeyValueEnumeratorCurrent(enumerator, key, objectHandle, value, versionSequenceNumber);
        }
        co_return;
    }

    if (cts != nullptr)
        *cts = cancellationTokenSource.Detach();

    NTSTATUS ntstatus = STATUS_SUCCESS;
    BOOL _advanced = false;
    LPCWSTR _key = nullptr;
    size_t _objectHandle = 0;
    Buffer buffer = {};
    LONG64 _versionSequenceNumber = -1;
    EXCEPTION_TO_STATUS(_advanced = co_await awaitable, ntstatus);
    if (returnKeyValues && _advanced)
    {
        StoreKeyValueEnumeratorCurrent(enumerator, &_key, &_objectHandle, &buffer, &_versionSequenceNumber);
    }
    callback(ctx, StatusConverter::ToHResult(ntstatus), _advanced, _key, _objectHandle, buffer.Bytes, buffer.Length, _versionSequenceNumber);
    KBuffer::SPtr kBuffer;
    kBuffer.Attach((KBuffer*)buffer.Handle);
}

extern "C" HRESULT StoreKeyValueEnumerator_MoveNextAsync(
    __in StoreKeyValueAsyncEnumeratorHandle enumerator,
    __out CancellationTokenSourceHandle* cts,
    __out BOOL* advanced,
    __out LPCWSTR* key,
    __out size_t* objectHandle,
    __out Buffer* value,
    __out int64_t* versionSequenceNumber,
    __in fnNotifyStoreKeyValueEnumeratorMoveNextAsyncCompletion callback,
    __in void* ctx,
    __out BOOL* synchronousComplete)
{
    NTSTATUS status;
    StoreKeyValueEnumeratorMoveNextAsyncInternal(
        (IAsyncEnumerator<KeyValuePair<KString::SPtr, KeyValuePair<LONG64, KBuffer::SPtr>>>*)enumerator, 
        (ktl::CancellationTokenSource**)cts, 
        advanced, key, objectHandle, value, 
        versionSequenceNumber, callback, ctx, status, *synchronousComplete);
    return StatusConverter::ToHResult(status);
}
