// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ktl;
using namespace Data::LoggingReplicator;
using namespace Data::LogRecordLib;
using namespace Data::Utilities;
using namespace TxnReplicator;
using namespace TestHooks;

StateProvider::StateProvider(__in  LoggingReplicatorImpl & loggingReplicatorStateProvider)
    : IStateProvider()
    , KObject()
    , KShared()
    , PartitionedReplicaTraceComponent(*loggingReplicatorStateProvider.PartitionedReplicaIdentifier)
    , loggingReplicatorStateProvider_(&loggingReplicatorStateProvider)
    , testHookContext_()
{
    EventSource::Events->Ctor(
        TracePartitionId,
        ReplicaId,
        L"StateProvider",
        reinterpret_cast<uintptr_t>(this));
}

StateProvider::~StateProvider()
{
    EventSource::Events->Dtor(
        TracePartitionId,
        ReplicaId,
        L"StateProvider",
        reinterpret_cast<uintptr_t>(this));
}

IStateProvider::SPtr StateProvider::Create(
    __in LoggingReplicatorImpl & loggingReplicator,
    __in KAllocator & allocator)
{
    StateProvider * pointer = _new(V1REPLICATOR_TAG, allocator) StateProvider(loggingReplicator);

    if (pointer == nullptr)
    {
        throw Exception(STATUS_INSUFFICIENT_RESOURCES);
    }

    return IStateProvider::SPtr(pointer);
}

void StateProvider::Dispose()
{
    loggingReplicatorStateProvider_.Reset();
}

Awaitable<NTSTATUS> StateProvider::OnDataLossAsync(__out bool & result)
{
    NTSTATUS status = co_await loggingReplicatorStateProvider_->OnDataLossAsync(result);

    co_return status;
}

Awaitable<NTSTATUS> StateProvider::UpdateEpochAsync(
    __in FABRIC_EPOCH const * epoch,
    __in LONG64 lastLsnInPreviousEpoch)
{
    Epoch e(epoch->DataLossNumber, epoch->ConfigurationNumber);

    NTSTATUS status = co_await loggingReplicatorStateProvider_->UpdateEpochAsync(e, lastLsnInPreviousEpoch);

    co_return status;
}

HRESULT StateProvider::GetCopyContext(__out IOperationDataStream::SPtr & copyContextStream)
{
    HRESULT ret = S_OK;

    NTSTATUS status = loggingReplicatorStateProvider_->GetCopyContext(copyContextStream);
    ret = StatusConverter::ToHResult(status);

    return ret;
}

HRESULT StateProvider::GetCopyState(
    __in FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
    __in OperationDataStream & copyContextStream,
    __out IOperationDataStream::SPtr & copyStateStream)
{
    HRESULT ret = this->Test_GetInjectedFault(FaultInjector::FabricTest_GetCopyStateTag);
    if (!SUCCEEDED(ret)) { return ret; }

    NTSTATUS status = loggingReplicatorStateProvider_->GetCopyState(
        uptoSequenceNumber,
        copyContextStream,
        copyStateStream);

    ret = StatusConverter::ToHResult(status);

    return ret;
}

HRESULT StateProvider::GetLastCommittedSequenceNumber(FABRIC_SEQUENCE_NUMBER * lsn)
{
    HRESULT ret = S_OK;

    NTSTATUS status = loggingReplicatorStateProvider_->GetLastCommittedSequenceNumber(*lsn);
    ret = StatusConverter::ToHResult(status);

    return ret;
}

void StateProvider::Test_SetTestHookContext(TestHooks::TestHookContext const & testHookContext)
{
    testHookContext_ = testHookContext;

    if (loggingReplicatorStateProvider_)
    {
        loggingReplicatorStateProvider_->Test_SetTestHookContext(testHookContext);
    }
}

HRESULT StateProvider::CreateAsyncOnDataLossContext(__out AsyncOnDataLossContext::SPtr & asyncContext)
{
    AsyncOnDataLossContextImpl::SPtr context = _new(V1REPLICATOR_TAG, GetThisAllocator()) AsyncOnDataLossContextImpl();
    if (!context)
    {
        return StatusConverter::ToHResult(STATUS_INSUFFICIENT_RESOURCES);
    }

    NTSTATUS status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return StatusConverter::ToHResult(status);
    }

    context->parent_ = this;

    asyncContext.Attach(context.DownCast<AsyncOnDataLossContext>());
    context.Detach();

    return S_OK;
}

HRESULT StateProvider::CreateAsyncUpdateEpochContext(__out AsyncUpdateEpochContext::SPtr & asyncContext)
{
    ApiSignalHelper::WaitForSignalIfSet(testHookContext_, StateProviderBeginUpdateEpochBlock);

    auto hr = this->Test_GetInjectedFault(FaultInjector::FabricTest_BeginUpdateEpochTag);
    if (!SUCCEEDED(hr))
    {
        return hr;
    }

    AsyncUpdateEpochContextImpl::SPtr context = _new(V1REPLICATOR_TAG, GetThisAllocator()) AsyncUpdateEpochContextImpl();
    if (!context)
    {
        return StatusConverter::ToHResult(STATUS_INSUFFICIENT_RESOURCES);
    }

    NTSTATUS status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return StatusConverter::ToHResult(status);
    }

    context->parent_ = this;

    asyncContext.Attach(context.DownCast<AsyncUpdateEpochContext>());
    context.Detach();

    return S_OK;
}

HRESULT StateProvider::Test_GetInjectedFault(wstring const & tag)
{
    ErrorCodeValue::Enum injectedFault;
    if (FaultInjector::GetGlobalFaultInjector().TryReadError(
        tag,
        testHookContext_,
        injectedFault))
    {
        return ErrorCode(injectedFault).ToHResult();
    }

    return S_OK;
}

//
// OnDataLoss Operation
//
StateProvider::AsyncOnDataLossContextImpl::AsyncOnDataLossContextImpl() 
    : isStateChanged_(FALSE)
{
}

StateProvider::AsyncOnDataLossContextImpl::~AsyncOnDataLossContextImpl()
{
}

HRESULT StateProvider::AsyncOnDataLossContextImpl::StartOnDataLoss(
    __in_opt KAsyncContextBase * const parentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback callback)
{
    Start(parentAsyncContext, callback);

    return S_OK;
}

void StateProvider::AsyncOnDataLossContextImpl::OnStart()
{
    DoWork();
}

Task StateProvider::AsyncOnDataLossContextImpl::DoWork()
{
    KCoShared$ApiEntry();
    NTSTATUS status = STATUS_SUCCESS;

    bool result = false;
    status = co_await parent_->OnDataLossAsync(result);

    if (NT_SUCCESS(status))
    {
        isStateChanged_ =
            result ?
            TRUE :
            FALSE;
    }

    Complete(status);
}

HRESULT StateProvider::AsyncOnDataLossContextImpl::GetResult(
    __out BOOLEAN & isStateChanged)
{
    isStateChanged = isStateChanged_;
    return Result();
}

//
// UpdateEpoch Operation
//
StateProvider::AsyncUpdateEpochContextImpl::AsyncUpdateEpochContextImpl() 
    : previousEpochLastSequenceNumber_(Constants::InvalidLsn)
{
    epoch_.ConfigurationNumber = 0;
    epoch_.DataLossNumber = 0;
    epoch_.Reserved = nullptr;
}

StateProvider::AsyncUpdateEpochContextImpl::~AsyncUpdateEpochContextImpl()
{
}

HRESULT StateProvider::AsyncUpdateEpochContextImpl::StartUpdateEpoch(
    __in FABRIC_EPOCH const & epoch,
    __in FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
    __in_opt KAsyncContextBase * const parentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback callback)
{
    epoch_ = epoch;
    previousEpochLastSequenceNumber_ = previousEpochLastSequenceNumber;

    Start(parentAsyncContext, callback);

    return S_OK;
}

void StateProvider::AsyncUpdateEpochContextImpl::OnStart()
{
    DoWork();
}

Task StateProvider::AsyncUpdateEpochContextImpl::DoWork()
{
    KCoShared$ApiEntry();
    NTSTATUS status = STATUS_SUCCESS;

    auto hr = parent_->Test_GetInjectedFault(FaultInjector::FabricTest_EndUpdateEpochTag);
    if (!SUCCEEDED(hr))
    {
        status = ErrorCode::FromHResult(hr).ToNTStatus();

        Complete(status);

        co_return;
    }

    try
    {
        co_await parent_->UpdateEpochAsync(&epoch_, previousEpochLastSequenceNumber_);
    }
    catch (Exception e)
    {
        status = e.GetStatus();
    }

    Complete(status);
}
