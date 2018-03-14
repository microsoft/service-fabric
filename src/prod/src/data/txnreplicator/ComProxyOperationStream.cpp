// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Common;
using namespace Data::Utilities;
using namespace Data::LoggingReplicator;
using namespace TxnReplicator;

ComProxyOperationStream::ComProxyOperationStream(
    __in Data::Utilities::PartitionedReplicaId const & traceId,
    __in ComPointer<IFabricOperationStream> comOperationStream)
    : KObject()
    , KShared()
    , PartitionedReplicaTraceComponent(traceId)
    , comOperationStream_(comOperationStream)
{
    TREventSource::Events->Ctor(
        TracePartitionId,
        ReplicaId,
        L"ComProxyOperationStream",
        reinterpret_cast<uintptr_t>(this));
}

ComProxyOperationStream::~ComProxyOperationStream()
{
    TREventSource::Events->Dtor(
        TracePartitionId,
        ReplicaId,
        L"ComProxyOperationStream",
        reinterpret_cast<uintptr_t>(this));
}

NTSTATUS ComProxyOperationStream::Create(
    __in Data::Utilities::PartitionedReplicaId const & traceId,
    __in ComPointer<IFabricOperationStream> comOperationStream,
    __in KAllocator & allocator,
    __out ComProxyOperationStream::SPtr & comProxyOperationStream)
{
    ComProxyOperationStream * pointer = _new(TRANSACTIONALREPLICATOR_TAG, allocator) ComProxyOperationStream(traceId, comOperationStream);

    if (pointer == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    comProxyOperationStream = pointer;

    return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> ComProxyOperationStream::GetOperationAsync(__out IOperation::SPtr & result)
{
    TestHooks::ApiSignalHelper::WaitForSignalIfSet(testHookContext_, StateProviderSecondaryPumpBlock);

    result = nullptr;

    AsyncGetOperationContext::SPtr context = _new(TRANSACTIONALREPLICATOR_TAG, GetThisAllocator()) AsyncGetOperationContext();
    if (!context)
    {
        co_return STATUS_INSUFFICIENT_RESOURCES;
    }

    context->parent_ = this;
    
    ComProxyOperation::SPtr proxyOperation;
    NTSTATUS status = co_await context->GetNextOperationAsync(proxyOperation);

    if (NT_SUCCESS(status))
    {
        result = proxyOperation.RawPtr();
    }

    co_return status;
}

void ComProxyOperationStream::Test_SetTestHookContext(TestHooks::TestHookContext const & testHookContext)
{
    testHookContext_ = testHookContext;
}

//
// Callout adapter to invoke getnext API on v1 replicator
//
ComProxyOperationStream::AsyncGetOperationContext::AsyncGetOperationContext()
    : parent_()
    , result_()
    , tcs_()
{
}

ComProxyOperationStream::AsyncGetOperationContext::~AsyncGetOperationContext()
{
}

Awaitable<NTSTATUS> ComProxyOperationStream::AsyncGetOperationContext::GetNextOperationAsync(__out ComProxyOperation::SPtr & operation)
{
    operation = nullptr;
    NTSTATUS status = AwaitableCompletionSource<NTSTATUS>::Create(GetThisAllocator(), TRANSACTIONALREPLICATOR_TAG, tcs_);
    
    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    ComPointer<IFabricAsyncOperationContext> context;

    HRESULT hr = parent_->comOperationStream_->BeginGetOperation(
        this,
        context.InitializationAddress());

    if (!SUCCEEDED(hr))
    {
        status = StatusConverter::Convert(hr);
        co_return status;
    }

    OnComOperationStarted(
        *context.GetRawPointer(),
        nullptr,
        nullptr);

    status = co_await tcs_->GetAwaitable();

    if (NT_SUCCESS(status))
    {
        operation = GetResult();
    }

    co_return status;
}
            
HRESULT ComProxyOperationStream::AsyncGetOperationContext::OnEnd(__in IFabricAsyncOperationContext & context)
{
    ComPointer<IFabricOperation> comOperation;

    HRESULT hr = parent_->comOperationStream_->EndGetOperation(
        &context,
        comOperation.InitializationAddress());

    if (SUCCEEDED(hr))
    {
        if (comOperation.GetRawPointer() == nullptr)
        {
            result_ = nullptr;
        }
        else
        {
            NTSTATUS status = ComProxyOperation::Create(
                comOperation,
                GetThisAllocator(),
                result_);

            if (!NT_SUCCESS(status))
            {
                hr = StatusConverter::ToHResult(status);
            }
        }
    }

    NTSTATUS status = StatusConverter::Convert(hr);
    tcs_->SetResult(status);
    return hr;
}

ComProxyOperation::SPtr ComProxyOperationStream::AsyncGetOperationContext::GetResult()
{
    return result_;
}
