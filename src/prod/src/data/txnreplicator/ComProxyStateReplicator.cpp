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

ComProxyStateReplicator::ComProxyStateReplicator(
    __in Data::Utilities::PartitionedReplicaId const & traceId,
    __in ComPointer<IFabricStateReplicator> & v1StateReplicator)
    : KObject()
    , KShared()
    , PartitionedReplicaTraceComponent(traceId)
    , comStateReplicator_(v1StateReplicator)
{
    TREventSource::Events->Ctor(
        TracePartitionId,
        ReplicaId,
        L"ComProxyStateReplicator",
        reinterpret_cast<uintptr_t>(this));
}

ComProxyStateReplicator::~ComProxyStateReplicator()
{ 
    TREventSource::Events->Dtor(
        TracePartitionId,
        ReplicaId,
        L"ComProxyStateReplicator",
        reinterpret_cast<uintptr_t>(this));
}

NTSTATUS ComProxyStateReplicator::Create(
    __in Data::Utilities::PartitionedReplicaId const & traceId,
    __in ComPointer<IFabricStateReplicator> & v1StateReplicator,
    __in KAllocator & allocator,
    __out ComProxyStateReplicator::SPtr & comProxyStateReplicator)
{
    ComProxyStateReplicator * pointer = _new(TRANSACTIONALREPLICATOR_TAG, allocator) ComProxyStateReplicator(traceId, v1StateReplicator);

    if (pointer == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    comProxyStateReplicator = pointer;

    return STATUS_SUCCESS;
}
 
CompletionTask::SPtr ComProxyStateReplicator::ReplicateAsync(
    __in OperationData const & replicationData,
    __out LONG64 & logicalSequenceNumber)
{
    logicalSequenceNumber = FABRIC_INVALID_SEQUENCE_NUMBER;
    AsyncReplicateContext::SPtr context = _new(TRANSACTIONALREPLICATOR_TAG, GetThisAllocator()) AsyncReplicateContext();
    
    if (!context)
    {
        return nullptr;
    }

    context->parent_ = this;

    ComPointer<IFabricOperationData> comOperationData;
    NTSTATUS status = ComOperationData::Create(
        replicationData,
        GetThisAllocator(),
        comOperationData);
  
    if (!NT_SUCCESS(status))
    {
        return nullptr;
    }

    return context->ReplicateAsync(
        comOperationData,
        logicalSequenceNumber);
}

NTSTATUS ComProxyStateReplicator::GetCopyStream(__out IOperationStream::SPtr & result)
{
    return GetOperationStream(true, result);
}

NTSTATUS ComProxyStateReplicator::GetReplicationStream(__out IOperationStream::SPtr & result)
{
    return GetOperationStream(false, result);
}

int64 ComProxyStateReplicator::GetMaxReplicationMessageSize()
{
    ComPointer <IFabricStateReplicator2> comStateReplicator2;
    HRESULT hr = comStateReplicator_->QueryInterface(IID_IFabricStateReplicator2, comStateReplicator2.VoidInitializationAddress());
    ASSERT_IFNOT(
        SUCCEEDED(hr),
        "ComProxyStateReplicator:: V1 replicator com object must implement IFabricStateReplicator2");

    ComPointer<IFabricReplicatorSettingsResult> settingsResult;
    ReferencePointer<FABRIC_REPLICATOR_SETTINGS_EX1> ex1Settings;

    comStateReplicator2->GetReplicatorSettings(settingsResult.InitializationAddress());
    FABRIC_REPLICATOR_SETTINGS const * outputResult = settingsResult->get_ReplicatorSettings();

    bool ex1FlagsUsed = Reliability::ReplicationComponent::ReplicatorSettings::IsReplicatorEx1FlagsUsed(outputResult->Flags);
    ASSERT_IFNOT(
        ex1FlagsUsed,
        "ComProxyStateReplicator:: V1 replicator com object must include FABRIC_REPLICATOR_SETTINGS_EX1");
    ex1Settings = static_cast<FABRIC_REPLICATOR_SETTINGS_EX1*>(outputResult->Reserved);

    return ex1Settings->MaxReplicationMessageSize;
}

void ComProxyStateReplicator::Test_SetTestHookContext(TestHooks::TestHookContext const & testHookContext)
{
    testHookContext_ = testHookContext;
}
 
NTSTATUS ComProxyStateReplicator::GetOperationStream(
    __in bool isCopyStream,
    __out Data::LoggingReplicator::IOperationStream::SPtr & result)
{
    ComPointer<IFabricOperationStream> operationStream;
    HRESULT hr = S_OK;
    
    if (isCopyStream)
    {
        hr = comStateReplicator_->GetCopyStream(operationStream.InitializationAddress());
    }
    else
    {
        hr = comStateReplicator_->GetReplicationStream(operationStream.InitializationAddress());
    }

    if (!SUCCEEDED(hr))
    {
        NTSTATUS status = StatusConverter::Convert(hr);
        return status;
    }
    
    ComProxyOperationStream::SPtr comProxyOperationStream;

    NTSTATUS status = ComProxyOperationStream::Create(
        *PartitionedReplicaIdentifier,
        operationStream,
        GetThisAllocator(),
        comProxyOperationStream);
    
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    comProxyOperationStream->Test_SetTestHookContext(testHookContext_);

    result = comProxyOperationStream.RawPtr();
    return status;
}

//
// Callout adapter to invoke replicate API on v1 replicator
//
ComProxyStateReplicator::AsyncReplicateContext::AsyncReplicateContext()
    : resultSequenceNumber_(FABRIC_INVALID_SEQUENCE_NUMBER)
    , parent_()
    , task_(CompletionTask::Create(GetThisAllocator()))
{
}

ComProxyStateReplicator::AsyncReplicateContext::~AsyncReplicateContext()
{
}

CompletionTask::SPtr ComProxyStateReplicator::AsyncReplicateContext::ReplicateAsync(
    __in Common::ComPointer<IFabricOperationData> comOperationData,
    __out LONG64 & logicalSequenceNumber)
{
    logicalSequenceNumber = FABRIC_INVALID_SEQUENCE_NUMBER;
    NTSTATUS status = STATUS_SUCCESS;

    //
    // The following implementation ensures that when the replication operations fails synchronously with
    // an error, this error is returned immediatly, instead of starting the underlying callout adapter.
    //
    // - begin fails: returns error, async context is not started
    // - begin succeeds:
    //    - completed synchronously:
    //       - end fails: returns error, async context is not started
    //       - end succeeds: returns success, async context not started
    //    - not completed synchronously:
    //       - when the Invoke is seen and the OnStart is seen, call end and complete the async context
    //
    ComPointer<IFabricAsyncOperationContext> context;

    HRESULT hr = parent_->comStateReplicator_->BeginReplicate(
        comOperationData.GetRawPointer(),
        this,
        &logicalSequenceNumber,
        context.InitializationAddress());

    if (!SUCCEEDED(hr))
    {
        status = StatusConverter::Convert(hr);
        task_->CompleteAwaiters(status);
    }
    else if (context->CompletedSynchronously())
    {
        hr = parent_->comStateReplicator_->EndReplicate(
            context.GetRawPointer(),
            &resultSequenceNumber_); // Use the reference input variable only on beginreplicate

        status = StatusConverter::Convert(hr);
        task_->CompleteAwaiters(status);
    }
    else
    {
        OnComOperationStarted(
            *context.GetRawPointer(),
            nullptr,
            nullptr);
    }

    return task_;
}
            
HRESULT ComProxyStateReplicator::AsyncReplicateContext::OnEnd(__in IFabricAsyncOperationContext & context)
{
    ASSERT_IFNOT(
        context.CompletedSynchronously() == FALSE,
        "Replicate Operation callback must only be invoked when not completed synchronously");

    HRESULT hr = parent_->comStateReplicator_->EndReplicate(
        &context,
        &resultSequenceNumber_);

    NTSTATUS status = StatusConverter::Convert(hr);
    task_->CompleteAwaiters(status);
    return hr;
}

FABRIC_SEQUENCE_NUMBER ComProxyStateReplicator::AsyncReplicateContext::GetResultLsn() const
{
    return resultSequenceNumber_;
}
