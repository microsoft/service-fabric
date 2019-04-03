// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Common;
using namespace Data;
using namespace Data::Utilities;
using namespace TxnReplicator;

Common::StringLiteral const TraceComponent("ComTransactionalReplicator");

ComTransactionalReplicator::ComTransactionalReplicator(
    __in PartitionedReplicaId const & prId,
    __in FABRIC_REPLICA_ID replicaId,
    __in IFabricStatefulServicePartition & fabricPartition,
    __in ComPointer<IFabricPrimaryReplicator> & v1PrimaryReplicator,
    __in LoggingReplicator::IStateReplicator & v1StateReplicator,
    __in IFabricStateProvider2Factory & factory,
    __in IRuntimeFolders & runtimeFolders,
    __in TRInternalSettingsSPtr && transactionalReplicatorConfig,
    __in SLInternalSettingsSPtr && ktlLoggerSharedLogConfig,
    __in Log::LogManager & logManager,
    __in IFabricDataLossHandler & userDataLossHandler,
    __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr && healthClient,
    __in BOOLEAN hasPersistedState)
    : KObject()
    , KShared()
    , PartitionedReplicaTraceComponent(prId)
    , primaryReplicator_(v1PrimaryReplicator)
    , transactionalReplicator_(nullptr)
    , isFaulted_(false)
{
    KAllocator & allocator = GetThisAllocator();

    NTSTATUS status = STATUS_SUCCESS;

    TREventSource::Events->Ctor(
        TracePartitionId,
        ReplicaId,
        wformatString("ComTransactionalReplicator \r\n Settings = {0} {1}",
            transactionalReplicatorConfig->ToString(),
            ktlLoggerSharedLogConfig->ToString()),
        reinterpret_cast<uintptr_t>(this));

    HRESULT hr = v1PrimaryReplicator->QueryInterface(IID_IFabricStateReplicator, fabricStateReplicator_.VoidInitializationAddress());
    ASSERT_IFNOT(
        SUCCEEDED(hr),
        "{0}: ComTransactionalReplicator: V1 replicator com object must implement IFabricStateReplicator. HResult {1}",
        TraceId, 
        hr);

    hr = v1PrimaryReplicator->QueryInterface(IID_IFabricInternalReplicator, fabricInternalReplicator_.VoidInitializationAddress());
    ASSERT_IFNOT(
        SUCCEEDED(hr),
        "{0}: ComTransactionalReplicator: V1 replicator com object must implement IFabricInternalReplicator. HResult {1}",
        TraceId,
        hr);

    try
    {
        IStatefulPartition::SPtr partition;
        status = Partition::Create(
            fabricPartition,
            allocator,
            partition);

        THROW_ON_FAILURE(status);

        ComProxyStateProvider2Factory::SPtr proxyStateProvider2Factory;
        ComPointer<IFabricStateProvider2Factory> comFactoryPtr;

        comFactoryPtr.SetAndAddRef(&factory);
        status = ComProxyStateProvider2Factory::Create(
            comFactoryPtr,
            GetThisAllocator(),
            proxyStateProvider2Factory);

        THROW_ON_FAILURE(status);

        // Create the ComProxyDataLossHandler
        ComPointer<IFabricDataLossHandler> comDataLossHandler;
        comDataLossHandler.SetAndAddRef(&userDataLossHandler);
        ComProxyDataLossHandler::SPtr dataLossHandlerSPtr = nullptr;
        status = ComProxyDataLossHandler::Create(comDataLossHandler, allocator, dataLossHandlerSPtr);
        THROW_ON_FAILURE(status);

        transactionalReplicator_ = TransactionalReplicator::Create(
            prId,
            runtimeFolders,
            *partition,
            v1StateReplicator,
            *proxyStateProvider2Factory,
            move(transactionalReplicatorConfig),
            move(ktlLoggerSharedLogConfig),
            logManager,
            *dataLossHandlerSPtr,
            move(healthClient),
            hasPersistedState,
            allocator);
    }
    catch (Exception e)
    {
        ASSERT_IF(
            NT_SUCCESS(e.GetStatus()),
            "Exception during creation of com xact replicator object for replica id: {1}, status: {1}. status is expected to be non success",
            replicaId, status);

        SetConstructorStatus(e.GetStatus());
    }
}

ComTransactionalReplicator::~ComTransactionalReplicator()
{
    TREventSource::Events->Dtor(
        TracePartitionId,
        ReplicaId,
        L"ComTransactionalReplicator",
        reinterpret_cast<uintptr_t>(this));
}

NTSTATUS ComTransactionalReplicator::Create(
    __in FABRIC_REPLICA_ID replicaId,
    __in Reliability::ReplicationComponent::IReplicatorFactory & replicatorFactory,
    __in IFabricStatefulServicePartition & partition,
    __in_opt FABRIC_REPLICATOR_SETTINGS const * replicatorSettings,
    __in IRuntimeFolders & runtimeFolders,
    __in BOOLEAN hasPersistedState,
    __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr && healthClient,
    __in IFabricStateProvider2Factory & factory,
    __in TRInternalSettingsSPtr && transactionalReplicatorConfig,
    __in SLInternalSettingsSPtr && ktlLoggerSharedLogConfig,
    __in Log::LogManager & logManager,
    __in IFabricDataLossHandler & userDataLossHandler,
    __in KAllocator & allocator,
    __out IFabricPrimaryReplicator ** replicator,
    __out ITransactionalReplicator::SPtr & transactionalReplicator)
{
    ComPointer<IFabricStateReplicator> v1StateReplicator;
    ComPointer<IFabricPrimaryReplicator> v1PrimaryReplicator;

    ComStateProvider::SPtr comStateProvider;
    ComProxyStateReplicator::SPtr comProxyStateReplicator;

    FABRIC_SERVICE_PARTITION_INFORMATION const * partitionInfo;
    HRESULT hr = partition.GetPartitionInfo(&partitionInfo);
    NTSTATUS status = StatusConverter::Convert(hr);

    if (!NT_SUCCESS(status))
    {
        TREventSource::Events->ExceptionError(
            Common::Guid::Empty(),
            0,
            L"Failed to GetPartitionInfo",
            status,
            L"");

        return status;
    }

    KGuid pId(ServiceInfoUtility::GetPartitionId(partitionInfo));

    PartitionedReplicaId::SPtr prId = PartitionedReplicaId::Create(
        pId,
        replicaId,
        allocator);

    if (prId == nullptr)
    {
        TREventSource::Events->ExceptionError(
            prId->TracePartitionId,
            prId->ReplicaId,
            L"Failed to Create PartitionedReplicaId due to insufficient memory in ComTransactionalReplicator",
            STATUS_INSUFFICIENT_RESOURCES,
            L"");

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Step 1: Create a ComStateProvider (which wraps the inner loggingreplicator state provider)
    status = ComStateProvider::Create(
        allocator,
        *prId,
        comStateProvider);

    if (!NT_SUCCESS(status))
    {
        TREventSource::Events->ExceptionError(
            prId->TracePartitionId,
            prId->ReplicaId,
            L"Failed to create ComStateProvider",
            status,
            L"");

        return status;
    }

    Reliability::ReplicationComponent::IReplicatorHealthClientSPtr v1HealthClient = std::move(healthClient);
    Reliability::ReplicationComponent::IReplicatorHealthClientSPtr v2HealthClient = v1HealthClient;

    // Step 2: Create the real v1 replicator using the above state provider
    hr = replicatorFactory.CreateReplicator(
        replicaId,
        &partition,
        comStateProvider.RawPtr(),
        replicatorSettings,
        hasPersistedState,
        std::move(v1HealthClient),
        v1StateReplicator.InitializationAddress());

    if (!SUCCEEDED(hr))
    {
        TREventSource::Events->ExceptionError(
            prId->TracePartitionId,
            prId->ReplicaId,
            L"Failed to create V1 replicator",
            hr,
            L"");

        status = StatusConverter::Convert(hr);

        return status;
    }

    // Step 3: Create a ComProxyStateReplicator (which wraps the v1 replicator)
    status = ComProxyStateReplicator::Create(
        *prId,
        v1StateReplicator,
        allocator,
        comProxyStateReplicator);

    if (!NT_SUCCESS(status))
    {
        TREventSource::Events->ExceptionError(
            prId->TracePartitionId,
            prId->ReplicaId,
            L"Failed to create ComProxyStateReplicator",
            hr,
            L"");

        return status;
    }

    // Verify that the V1 replicator com object implements the IFabricPrimaryReplicator interface as well
    hr = v1StateReplicator->QueryInterface(IID_IFabricPrimaryReplicator, v1PrimaryReplicator.VoidInitializationAddress());

    ASSERT_IFNOT(
        SUCCEEDED(hr), 
        "ComTransactionalReplicator:: V1 replicator com object must implement IFabricPrimaryReplicator");

    // Step 4: Create the com transactional replicator
    ComTransactionalReplicator::SPtr comTransactionalReplicator = _new(TRANSACTIONALREPLICATOR_TAG, allocator)
        ComTransactionalReplicator(
            *prId,
            replicaId,
            partition,
            v1PrimaryReplicator,
            *comProxyStateReplicator,
            factory,
            runtimeFolders,
            move(transactionalReplicatorConfig),
            move(ktlLoggerSharedLogConfig),
            logManager,
            userDataLossHandler,
            move(v2HealthClient),
            hasPersistedState);

    if (comTransactionalReplicator == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(comTransactionalReplicator->Status()))
    {
        return comTransactionalReplicator->Status();
    }

    // Lastly, get the inner state provider object and initialize the comStateProvider with it
    LoggingReplicator::IStateProvider::SPtr stateProvider = comTransactionalReplicator->GetStateProvider();

    ASSERT_IFNOT(
        stateProvider,
        "ComTransactionalReplicator:: StateProvider must not be null object must implement IFabricPrimaryReplicator");

    comStateProvider->Initialize(*stateProvider);

    // Verify that the comtransactionalreplicator com object implements the IFabricPrimaryReplicator interface as well
    hr = comTransactionalReplicator->QueryInterface(::IID_IFabricPrimaryReplicator, (void **)replicator);

    ASSERT_IFNOT(
        SUCCEEDED(hr), 
        "ComTransactionalReplicator:: ComTransactionalReplicator object must implement IFabricPrimaryReplicator");

    transactionalReplicator = comTransactionalReplicator->transactionalReplicator_.RawPtr();

    comProxyStateReplicator->Test_SetTestHookContext(transactionalReplicator->Test_GetTestHookContext());

    return STATUS_SUCCESS;
}

HRESULT ComTransactionalReplicator::BeginOpen(
    __in IFabricAsyncOperationCallback * callback,
    __out IFabricAsyncOperationContext ** context)
{
    if (!callback)
    {
        return E_POINTER;
    }

    if (!context)
    {
        return E_POINTER;
    }

    ComAsyncOpenContext::SPtr asyncOperation = _new(TRANSACTIONALREPLICATOR_TAG, GetThisAllocator()) ComAsyncOpenContext();
    if (!context)
    {
        return E_OUTOFMEMORY;
    }

    asyncOperation->parent_ = this;

    return Ktl::Com::AsyncCallInAdapter::CreateAndStart(
        GetThisAllocator(),
        Ktl::Move(asyncOperation),
        callback,
        context,
        [&](Ktl::Com::AsyncCallInAdapter&,
            ComAsyncOpenContext& operation,
            KAsyncContextBase::CompletionCallback completionCallback) -> HRESULT
    {
        return operation.StartOpen(
            nullptr,
            completionCallback);
    });
}

HRESULT ComTransactionalReplicator::EndOpen(
    __in IFabricAsyncOperationContext * context,
    __out IFabricStringResult ** replicationAddress)
{
    if (!context)
    {
        return E_POINTER;
    }

    ComAsyncOpenContext::SPtr asyncOperation;
    HRESULT hr = Ktl::Com::AsyncCallInAdapter::End(context, asyncOperation);

    if (SUCCEEDED(hr))
    {
        asyncOperation->GetResult(replicationAddress);
    }

    return hr;
}

HRESULT ComTransactionalReplicator::BeginChangeRole(
    __in FABRIC_EPOCH const * epoch,
    __in FABRIC_REPLICA_ROLE role,
    __in IFabricAsyncOperationCallback * callback,
    __out IFabricAsyncOperationContext ** context)
{ 
    if (!epoch)
    {
        return E_POINTER;
    }

    if (!callback)
    {
        return E_POINTER;
    }

    if (!context)
    {
        return E_POINTER;
    }

    if (isFaulted_.load())
    {
        return E_UNEXPECTED;
    }

    ComAsyncChangeRoleContext::SPtr asyncOperation = _new(TRANSACTIONALREPLICATOR_TAG, GetThisAllocator()) ComAsyncChangeRoleContext();
    if (!context)
    {
        return E_OUTOFMEMORY;
    }

    asyncOperation->parent_ = this;

    return Ktl::Com::AsyncCallInAdapter::CreateAndStart(
        GetThisAllocator(),
        Ktl::Move(asyncOperation),
        callback,
        context,
        [&](Ktl::Com::AsyncCallInAdapter&,
            ComAsyncChangeRoleContext& operation, 
            KAsyncContextBase::CompletionCallback completionCallback) -> HRESULT
    {
        return operation.StartChangeRole(
            epoch,
            role,
            nullptr, 
            completionCallback);
    });
}

HRESULT ComTransactionalReplicator::EndChangeRole(__in IFabricAsyncOperationContext * context)
{
    if (!context)
    {
        return E_POINTER;
    }

    ComAsyncChangeRoleContext::SPtr asyncOperation;
    HRESULT hr = Ktl::Com::AsyncCallInAdapter::End(context, asyncOperation);

    if (!SUCCEEDED(hr))
    {
        // Do not accept any further calls from failover if change role failed
        // Otherwise, we could end up invoking duplicate calls on V1 replicator which will lead to asserts
        isFaulted_.store(true);
    }

    return hr;
}

HRESULT ComTransactionalReplicator::BeginClose(
    __in IFabricAsyncOperationCallback * callback,
    __out IFabricAsyncOperationContext ** context)
{
    if (!callback)
    {
        return E_POINTER;
    }

    if (!context)
    {
        return E_POINTER;
    }

    ComAsyncCloseContext::SPtr asyncOperation = _new(TRANSACTIONALREPLICATOR_TAG, GetThisAllocator()) ComAsyncCloseContext();
    if (!context)
    {
        return E_OUTOFMEMORY;
    }

    asyncOperation->parent_ = this;

    return Ktl::Com::AsyncCallInAdapter::CreateAndStart(
        GetThisAllocator(),
        Ktl::Move(asyncOperation),
        callback,
        context,
        [&](Ktl::Com::AsyncCallInAdapter&,
            ComAsyncCloseContext& operation, 
            KAsyncContextBase::CompletionCallback completionCallback) -> HRESULT
    {
        return operation.StartClose(
            nullptr, 
            completionCallback);
    });
}

HRESULT ComTransactionalReplicator::EndClose(__in IFabricAsyncOperationContext * context)
{
    if (!context)
    {
        return E_POINTER;
    }

    ComAsyncCloseContext::SPtr asyncOperation;
    HRESULT hr = Ktl::Com::AsyncCallInAdapter::End(context, asyncOperation);

    return hr;
}

void ComTransactionalReplicator::Abort()
{
    primaryReplicator_->Abort();
    transactionalReplicator_->Abort();
}

//
// All the below methods are just a pass through to the V1 replicator as the transactional replicator has nothing to do here
//
HRESULT ComTransactionalReplicator::BeginUpdateEpoch(
    __in FABRIC_EPOCH const * epoch,
    __in IFabricAsyncOperationCallback * callback,
    __out IFabricAsyncOperationContext ** context)
{
    return primaryReplicator_->BeginUpdateEpoch(
        epoch,
        callback,
        context);
}

HRESULT ComTransactionalReplicator::EndUpdateEpoch(
    __in IFabricAsyncOperationContext * context)
{
    return primaryReplicator_->EndUpdateEpoch(context);
}

HRESULT ComTransactionalReplicator::GetCurrentProgress(
    __out FABRIC_SEQUENCE_NUMBER * lastSequenceNumber)
{
    return primaryReplicator_->GetCurrentProgress(lastSequenceNumber);
}

HRESULT ComTransactionalReplicator::GetCatchUpCapability(
    __out FABRIC_SEQUENCE_NUMBER * fromSequenceNumber)
{
    return primaryReplicator_->GetCatchUpCapability(fromSequenceNumber);
}

//
// IFabricPrimaryReplicator methods
//

HRESULT ComTransactionalReplicator::BeginOnDataLoss(
    __in IFabricAsyncOperationCallback * callback,
    __out IFabricAsyncOperationContext ** context)
{
    return primaryReplicator_->BeginOnDataLoss(
        callback,
        context);
}

HRESULT ComTransactionalReplicator::EndOnDataLoss(
    __in IFabricAsyncOperationContext * context,
    __out BOOLEAN * isStateChanged)
{
    return primaryReplicator_->EndOnDataLoss(
        context,
        isStateChanged);
}

HRESULT ComTransactionalReplicator::UpdateCatchUpReplicaSetConfiguration(
    __in const FABRIC_REPLICA_SET_CONFIGURATION * currentConfiguration,
    __in const FABRIC_REPLICA_SET_CONFIGURATION * previousConfiguration)
{
    return primaryReplicator_->UpdateCatchUpReplicaSetConfiguration(
        currentConfiguration,
        previousConfiguration);
}

HRESULT ComTransactionalReplicator::BeginWaitForCatchUpQuorum(
    __in FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode,
    __in IFabricAsyncOperationCallback * callback,
    __out IFabricAsyncOperationContext ** context)
{
    return primaryReplicator_->BeginWaitForCatchUpQuorum(
        catchUpMode,
        callback,
        context);
}

HRESULT ComTransactionalReplicator::EndWaitForCatchUpQuorum(
    __in IFabricAsyncOperationContext * context)
{
    return primaryReplicator_->EndWaitForCatchUpQuorum(context);
}

HRESULT ComTransactionalReplicator::UpdateCurrentReplicaSetConfiguration(
    __in const FABRIC_REPLICA_SET_CONFIGURATION * currentConfiguration)
{
    return primaryReplicator_->UpdateCurrentReplicaSetConfiguration(currentConfiguration);
}

HRESULT ComTransactionalReplicator::BeginBuildReplica(
    __in const FABRIC_REPLICA_INFORMATION * replica,
    __in IFabricAsyncOperationCallback * callback,
    __out IFabricAsyncOperationContext ** context)
{
    return primaryReplicator_->BeginBuildReplica(
        replica,
        callback,
        context);
}

HRESULT ComTransactionalReplicator::EndBuildReplica(
    __in IFabricAsyncOperationContext * context)
{
    return primaryReplicator_->EndBuildReplica(context);
}

HRESULT ComTransactionalReplicator::RemoveReplica(
    __in FABRIC_REPLICA_ID replicaId)
{
    return primaryReplicator_->RemoveReplica(replicaId);
}

HRESULT ComTransactionalReplicator::UpdateReplicatorSettings(
    __in FABRIC_REPLICATOR_SETTINGS const * replicatorSettings)
{
    return fabricStateReplicator_->UpdateReplicatorSettings(replicatorSettings);
}

HRESULT ComTransactionalReplicator::GetReplicatorStatus(
    __in IFabricGetReplicatorStatusResult ** replicatorStatus)
{
    return fabricInternalReplicator_->GetReplicatorStatus(replicatorStatus);
}

HRESULT ComTransactionalReplicator::GetReplicationQueueCounters(
    __in FABRIC_INTERNAL_REPLICATION_QUEUE_COUNTERS * counters)
{
    ComPointer<IFabricInternalStateReplicator> internalStateReplicator;
    HRESULT hr = fabricInternalReplicator_->QueryInterface(IID_IFabricInternalStateReplicator, internalStateReplicator.VoidInitializationAddress());

    if (SUCCEEDED(hr))
    {
        hr = (internalStateReplicator->GetReplicationQueueCounters(counters));
    }

    return hr;
}

//
// The following methods are those with interaction with both the v1 and transactional replicator
//

//
// Implementation of the open async operation
//
ComTransactionalReplicator::ComAsyncOpenContext::ComAsyncOpenContext()
    : replicatorAddress_()
    , parent_(nullptr)
{
}

ComTransactionalReplicator::ComAsyncOpenContext::~ComAsyncOpenContext()
{
}

void ComTransactionalReplicator::ComAsyncOpenContext::GetResult(__out IFabricStringResult ** replicatorAddress)
{
    *replicatorAddress = replicatorAddress_.DetachNoRelease();
}

HRESULT ComTransactionalReplicator::ComAsyncOpenContext::StartOpen(
    __in_opt KAsyncContextBase * const parentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback callback)
{
    Start(parentAsyncContext, callback);
    
    return S_OK;
}

void ComTransactionalReplicator::ComAsyncOpenContext::OnStart()
{
    // First step is to open the v1 replicator
    ComPointer<IFabricAsyncOperationContext> context;
    HRESULT hr = parent_->primaryReplicator_->BeginOpen(this, context.InitializationAddress());

    if (!SUCCEEDED(hr))
    {
        Complete(hr);
        return;
    }

    OnFinishV1ReplicatorOpen(context.GetRawPointer(), TRUE);
}

void ComTransactionalReplicator::ComAsyncOpenContext::Invoke(__in IFabricAsyncOperationContext *context)
{
    OnFinishV1ReplicatorOpen(context, FALSE);
}

void ComTransactionalReplicator::ComAsyncOpenContext::OnFinishV1ReplicatorOpen(__in IFabricAsyncOperationContext *context, __in BOOLEAN expectCompletedSynchronously)
{
    ASSERT_IFNOT(
        context != nullptr,
        "{0}: ComTransactionalReplicator FinishV1ReplicatorOpen context is null",
        parent_->TraceId);

    if (context->CompletedSynchronously() != expectCompletedSynchronously)
    {
        return;
    }

    HRESULT hr = parent_->primaryReplicator_->EndOpen(context, replicatorAddress_.InitializationAddress());

    if (!SUCCEEDED(hr))
    {
        Complete(hr);
        return;
    }

    // Open the transactional replicator
    OpenTransactionalReplicator();
}

Task ComTransactionalReplicator::ComAsyncOpenContext::OpenTransactionalReplicator()
{
    KCoShared$ApiEntry();

    NTSTATUS status = co_await parent_->transactionalReplicator_->OpenAsync();

    if (!NT_SUCCESS(status))
    {
        // Abort the V1 replicator to ensure it cleans up its resources if open of v2 replicator failed
        parent_->primaryReplicator_->Abort();
    }

    Complete(StatusConverter::ToHResult(status));
    co_return;
}

//
// Implementation of the close async operation
//
ComTransactionalReplicator::ComAsyncCloseContext::ComAsyncCloseContext()
    : parent_(nullptr)
{
}

ComTransactionalReplicator::ComAsyncCloseContext::~ComAsyncCloseContext()
{
}

HRESULT ComTransactionalReplicator::ComAsyncCloseContext::StartClose(
    __in_opt KAsyncContextBase * const parentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback callback)
{
    Start(parentAsyncContext, callback);

    return S_OK;
}

void ComTransactionalReplicator::ComAsyncCloseContext::OnStart()
{
    // First step is to close the v1 replicator
    ComPointer<IFabricAsyncOperationContext> context;
    HRESULT hr = parent_->primaryReplicator_->BeginClose(this, context.InitializationAddress());

    if (!SUCCEEDED(hr))
    {
        Complete(hr);
        return;
    }

    OnFinishV1ReplicatorClose(context.GetRawPointer(), TRUE);
}

void ComTransactionalReplicator::ComAsyncCloseContext::Invoke(__in IFabricAsyncOperationContext *context)
{
    OnFinishV1ReplicatorClose(context, FALSE);
}

void ComTransactionalReplicator::ComAsyncCloseContext::OnFinishV1ReplicatorClose(
    __in IFabricAsyncOperationContext *context, 
    __in BOOLEAN expectCompletedSynchronously)
{
    ASSERT_IFNOT(
        context != nullptr,
        "{0}: ComTransactionalReplicator FinishV1ReplicatorClose context is null",
        parent_->TraceId);

    if (context->CompletedSynchronously() != expectCompletedSynchronously)
    {
        return;
    }

    HRESULT hr = parent_->primaryReplicator_->EndClose(context);

    WriteNoise(
        TraceComponent,
        "{0}: OnFinishV1ReplicatorClose EndClose completed with error {1:x}",
        parent_->TraceId,
        hr);

    if (!SUCCEEDED(hr))
    {
        Complete(hr);
        return;
    }

    // Close the transactional replicator
    CloseTransactionalReplicator();
}

Task ComTransactionalReplicator::ComAsyncCloseContext::CloseTransactionalReplicator()
{
    KCoShared$ApiEntry();

    WriteNoise(
        TraceComponent,
        "{0}: CloseTransactionalReplicator starting",
        parent_->TraceId);

    NTSTATUS status = co_await parent_->transactionalReplicator_->CloseAsync();

    Complete(StatusConverter::ToHResult(status));
    co_return;
}

//
// Implementation of the change role async operation
//
ComTransactionalReplicator::ComAsyncChangeRoleContext::ComAsyncChangeRoleContext()
    : parent_(nullptr)
    , epoch_()
    , newRole_(FABRIC_REPLICA_ROLE_UNKNOWN)
{
}

ComTransactionalReplicator::ComAsyncChangeRoleContext::~ComAsyncChangeRoleContext()
{
}

HRESULT ComTransactionalReplicator::ComAsyncChangeRoleContext::StartChangeRole(
    __in FABRIC_EPOCH const * epoch,
    __in FABRIC_REPLICA_ROLE role,
    __in_opt KAsyncContextBase * const parentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback callback)
{
    newRole_ = role;
    epoch_ = *epoch;
    
    // Start after setting the input parameters as we might execute synchronously on the same thread
    Start(parentAsyncContext, callback);
    return S_OK;
}

void ComTransactionalReplicator::ComAsyncChangeRoleContext::OnStart()
{
    // First step is to change role the v1 replicator
    ComPointer<IFabricAsyncOperationContext> context;
    HRESULT hr = parent_->primaryReplicator_->BeginChangeRole(
        &epoch_,
        newRole_,
        this, 
        context.InitializationAddress());

    if (!SUCCEEDED(hr))
    {
        Complete(hr);
        return;
    }

    OnFinishV1ReplicatorChangeRole(context.GetRawPointer(), TRUE);
}

void ComTransactionalReplicator::ComAsyncChangeRoleContext::Invoke(__in IFabricAsyncOperationContext *context)
{
    OnFinishV1ReplicatorChangeRole(context, FALSE);
}

void ComTransactionalReplicator::ComAsyncChangeRoleContext::OnFinishV1ReplicatorChangeRole(
    __in IFabricAsyncOperationContext *context, 
    __in BOOLEAN expectCompletedSynchronously)
{
    ASSERT_IFNOT(
        context != nullptr,
        "{0}: ComTransactionalReplicator FinishV1ReplicatorChangeRole context is null",
        parent_->TraceId);

    if (context->CompletedSynchronously() != expectCompletedSynchronously)
    {
        return;
    }

    HRESULT hr = parent_->primaryReplicator_->EndChangeRole(context);

    if (!SUCCEEDED(hr))
    {
        Complete(hr);
        return;
    }

    // Change Role the transactional replicator
    ChangeRoleTransactionalReplicator();
}

Task ComTransactionalReplicator::ComAsyncChangeRoleContext::ChangeRoleTransactionalReplicator()
{
    KCoShared$ApiEntry();
    NTSTATUS status = co_await parent_->transactionalReplicator_->ChangeRoleAsync(newRole_);
    Complete(StatusConverter::ToHResult(status));
    co_return;
}

void ComTransactionalReplicator::Test_SetFaulted()
{
    isFaulted_.store(true);
}
