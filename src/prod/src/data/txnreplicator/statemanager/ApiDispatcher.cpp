// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::Utilities;
using namespace TxnReplicator;
using namespace Data::StateManager;

#define INVALID_ERROR_CODE 0

Common::WStringLiteral const ApiDispatcher::OpenAsync_FunctionName(L"OpenAsync");
Common::WStringLiteral const ApiDispatcher::CloseAsync_FunctionName(L"CloseAsync");
Common::WStringLiteral const ApiDispatcher::RecoverCheckpointAsync_FunctionName(L"RecoverCheckpointAsync");
Common::WStringLiteral const ApiDispatcher::PerformCheckpointAsync_FunctionName(L"PerformCheckpointAsync");
Common::WStringLiteral const ApiDispatcher::CompleteCheckpointAsync_FunctionName(L"CompleteCheckpointAsync");
Common::WStringLiteral const ApiDispatcher::RestoreCheckpointAsync_FunctionName(L"RestoreCheckpointAsync");
Common::WStringLiteral const ApiDispatcher::BeginSettingCurrentState_FunctionName(L"BeginSettingCurrentState");
Common::WStringLiteral const ApiDispatcher::EndSettingCurrentStateAsync_FunctionName(L"EndSettingCurrentStateAsync");
Common::WStringLiteral const ApiDispatcher::PrepareForRemoveAsync_FunctionName(L"PrepareForRemoveAsync");
Common::WStringLiteral const ApiDispatcher::RemoveStateAsync_FunctionName(L"RemoveStateAsync");

ApiDispatcher::SPtr ApiDispatcher::Create(
    __in PartitionedReplicaId const & traceId,
    __in IStateProvider2Factory & stateProviderFactory,
    __in KAllocator& allocator)
{
    ApiDispatcher* pointer = _new(API_DISPATCHER_TAG, allocator) ApiDispatcher(
        traceId,
        stateProviderFactory);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return ApiDispatcher::SPtr(pointer);
}

NTSTATUS ApiDispatcher::CreateStateProvider(
    __in KUri const & name,
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in KString const & typeString,
    __in_opt OperationData const * const initializationParameters,
    __out TxnReplicator::IStateProvider2::SPtr & outStateProvider) noexcept
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    IStateProvider2::SPtr stateProviderSPtr = nullptr;
    FactoryArguments factoryArguments(name, stateProviderId, typeString, PartitionId, ReplicaId, initializationParameters);

    status = stateProviderFactorySPtr_->Create(factoryArguments, stateProviderSPtr);

    if (NT_SUCCESS(status) == false)
    {
        StateManagerEventSource::Events->ISP2_Factory_ApiError(
            TracePartitionId,
            ReplicaId,
            stateProviderId,
            ToStringLiteral(name),
            ToStringLiteral(typeString),
            status);
        
        outStateProvider = nullptr;
        return status;
    }

    outStateProvider = stateProviderSPtr;
    return status;
}

NTSTATUS ApiDispatcher::Initialize(
    __in Metadata const & metadata,
    __in KWeakRef<TxnReplicator::ITransactionalReplicator> & transactionalReplicatorWRef,
    __in KStringView const & workFolder,
    __in_opt KSharedArray<TxnReplicator::IStateProvider2::SPtr> const * const children) noexcept
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_SUCCESS;

    try
    {
        metadata.StateProvider->Initialize(
            transactionalReplicatorWRef, 
            workFolder, 
            children);
    }
    catch (Exception const & exception)
    {
        ULONG32 numberOfChildren = children == nullptr ? 0 : children->Count();

        StateManagerEventSource::Events->ISP2_Initialize_ApiError(
            TracePartitionId,
            ReplicaId,
            metadata.StateProviderId,
            ToStringLiteral(workFolder),
            numberOfChildren,
            exception.GetStatus());
        
        status = exception.GetStatus();
    }

    return status;
}

Awaitable<NTSTATUS> ApiDispatcher::OpenAsync(
    __in Metadata const & metadata,
    __in ktl::CancellationToken const & cancellationToken) noexcept
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_SUCCESS;

    try
    {
        co_await metadata.StateProvider->OpenAsync(cancellationToken);
    }
    catch (Exception const & exception)
    {
        StateManagerEventSource::Events->ISP2_ApiError(
            TracePartitionId,
            ReplicaId,
            metadata.StateProviderId,
            OpenAsync_FunctionName,
            exception.GetStatus());
        
        status = exception.GetStatus();
    }

    co_return status;
}

Awaitable<NTSTATUS> ApiDispatcher::ChangeRoleAsync(
    __in Metadata const & metadata,
    __in FABRIC_REPLICA_ROLE role,
    __in ktl::CancellationToken const & cancellationToken) noexcept
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_SUCCESS;

    try
    {
        co_await metadata.StateProvider->ChangeRoleAsync(role, cancellationToken);
    }
    catch (Exception const & exception)
    {
        StateManagerEventSource::Events->ISP2_ChangeRoleAsync_ApiError(
            TracePartitionId,
            ReplicaId,
            metadata.StateProviderId,
            role,
            exception.GetStatus());

        status = exception.GetStatus();
    }

    co_return status;
}

// Closes a given state
Awaitable<NTSTATUS> ApiDispatcher::CloseAsync(
    __in Metadata const & metadata,
    __in FailureAction failureAction,
    __in ktl::CancellationToken const & cancellationToken) noexcept
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_SUCCESS;
    try
    {
        co_await metadata.StateProvider->CloseAsync(cancellationToken);
    }
    catch (Exception const & exception)
    {
        StateManagerEventSource::Events->ISP2_ApiError(
            TracePartitionId,
            ReplicaId,
            metadata.StateProviderId,
            CloseAsync_FunctionName,
            exception.GetStatus());
        status = exception.GetStatus();
    }
    catch (...)
    {
        ASSERT_IFNOT(
            false, 
            "{0}: SPid: {1} threw non-ktl::Exception in CloseAsync.", 
            TraceId, 
            metadata.StateProviderId);
    }

    if (NT_SUCCESS(status) == false)
    {
        if (failureAction == FailureAction::AbortStateProvider)
        {
            Abort(metadata);
        }
        else
        {
            co_return status;
        }
    }

    co_return STATUS_SUCCESS;
}

void ApiDispatcher::Abort(
    __in Metadata const & metadata) noexcept
{
    try
    {
        metadata.StateProvider->Abort();
    }
    catch (Exception const & e)
    {
        ASSERT_IFNOT(
            false, 
            "{0}: ISP2_Abort threw ktl::Exception. SPid: {1}. Error Code: {2}", 
            TraceId, 
            metadata.StateProviderId,
            e.GetStatus());
    }

    return;
}

Awaitable<NTSTATUS> ApiDispatcher::RecoverCheckpointAsync(
    __in Metadata const & metadata,
    __in ktl::CancellationToken const & cancellationToken) noexcept
{
    KShared$ApiEntry();

    try
    {
        co_await metadata.StateProvider->RecoverCheckpointAsync(cancellationToken);
    }
    catch (Exception const & exception)
    {
        StateManagerEventSource::Events->ISP2_ApiError(
            TracePartitionId,
            ReplicaId,
            metadata.StateProviderId,
            RecoverCheckpointAsync_FunctionName,
            exception.GetStatus());
        co_return exception.GetStatus();
    }

    co_return STATUS_SUCCESS;
}

ktl::Awaitable<NTSTATUS> ApiDispatcher::BeginSettingCurrentStateAsync(
    __in Metadata const & metadata) noexcept
{
    try
    {
        co_await metadata.StateProvider->BeginSettingCurrentStateAsync();
    }
    catch (Exception const & exception)
    {
        StateManagerEventSource::Events->ISP2_ApiError(
            TracePartitionId,
            ReplicaId,
            metadata.StateProviderId,
            BeginSettingCurrentState_FunctionName,
            exception.GetStatus());
        co_return exception.GetStatus();
    }

    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> ApiDispatcher::SetCurrentStateAsync(
    __in Metadata const & metadata,
    __in LONG64 stateRecordNumber,
    __in OperationData const & data,
    __in ktl::CancellationToken const & cancellationToken) noexcept
{
    KShared$ApiEntry();

    try
    {
        co_await metadata.StateProvider->SetCurrentStateAsync(stateRecordNumber, data, cancellationToken);
    }
    catch (Exception const & exception)
    {
        StateManagerEventSource::Events->ISP2_SetCurrentStateAsync_ApiError(
            TracePartitionId,
            ReplicaId,
            metadata.StateProviderId,
            stateRecordNumber,
            data.BufferCount,
            exception.GetStatus());
        co_return exception.GetStatus();
    }

    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> ApiDispatcher::EndSettingCurrentStateAsync(
    __in Metadata const & metadata,
    __in ktl::CancellationToken const & cancellationToken) noexcept
{
    KShared$ApiEntry();

    try
    {
        co_await metadata.StateProvider->EndSettingCurrentStateAsync(cancellationToken);
    }
    catch (Exception const & exception)
    {
        StateManagerEventSource::Events->ISP2_ApiError(
            TracePartitionId,
            ReplicaId,
            metadata.StateProviderId,
            EndSettingCurrentStateAsync_FunctionName,
            exception.GetStatus());
        co_return exception.GetStatus();
    }

    co_return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> ApiDispatcher::OpenAsync(
    __in KArray<Metadata::CSPtr> const & metadataArray,
    __in ktl::CancellationToken const & cancellationToken) noexcept
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    KArray<Awaitable<NTSTATUS>> awaitableArray(GetThisAllocator(), metadataArray.Count());
    ASSERT_IFNOT(NT_SUCCESS(awaitableArray.Status()), "{0}: Failed to create KArray. Status: {1}", TraceId, status);

    SharedException::CSPtr sharedException = nullptr;

    for (Metadata::CSPtr metadata : metadataArray)
    {
        // Assumption: Below code assumes that if the exception is not a ktl::exception, the process will go down.
        status = awaitableArray.Append(OpenAsync(*metadata, cancellationToken));

        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}: Failed to append with code {1}. Array is correctly sized. This is not expected",
            TraceId,
            status);
    }

    // Even if one of the sync paths of OpenAsync failed, we need to ensure that all existing Awaitables are co_awaited.
    // We expect the KThreadPool and scheduler to ensure the optimal number of Awaitables execute asynchronously.
    status = co_await Utilities::TaskUtilities<NTSTATUS>::WhenAll_NoException(awaitableArray);
    if (NT_SUCCESS(status) == false)
    {
        ASSERT_IFNOT(
            awaitableArray.Count() <= metadataArray.Count(),
            "{0}: AwaitableArray count {1} <= metadataArray count {2}",
            TraceId,
            awaitableArray.Count(),
            metadataArray.Count());

        // Clean opened state providers.
        // #11908301: As an additional feature we can close the opened state providers.
        for (ULONG index = 0; index < awaitableArray.Count(); index++)
        {
            NTSTATUS awaitableStatus = co_await awaitableArray[index];
            if (NT_SUCCESS(awaitableStatus))
            {
                Abort(*metadataArray[index]);
            }
        }
    }

    co_return status;
}

Awaitable<NTSTATUS> ApiDispatcher::ChangeRoleAsync(
    __in KArray<Metadata::CSPtr> const & metadataArray,
    __in FABRIC_REPLICA_ROLE role,
    __in ktl::CancellationToken const & cancellationToken) noexcept
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    KArray<Awaitable<NTSTATUS>> awaitableArray(GetThisAllocator(), metadataArray.Count());
    ASSERT_IFNOT(NT_SUCCESS(awaitableArray.Status()), "{0}: Failed to create KArray. Status: {1}", TraceId, status);

    SharedException::CSPtr sharedException = nullptr;
    for (Metadata::CSPtr metadata : metadataArray)
    {
        // Assumption: Below code assumes that if the exception is not a ktl::exception, the process will go down.
        status = awaitableArray.Append(ChangeRoleAsync(*metadata, role, cancellationToken));

        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}: Failed to append with code {1}. Array is correctly sized. This is not expected",
            TraceId,
            status);
    }

    // Even if one of the sync paths of OpenAsync failed, we need to ensure that all existing Awaitables are co_awaited.
    // We expect the KThreadPool and scheduler to ensure the optimal number of Awaitables execute asynchronously.
    status = co_await Utilities::TaskUtilities<NTSTATUS>::WhenAll_NoException(awaitableArray);
    co_return status;
}

Awaitable<NTSTATUS> ApiDispatcher::CloseAsync(
    __in KArray<Metadata::CSPtr> const & metadataArray,
    __in FailureAction failureAction,
    __in ktl::CancellationToken const & cancellationToken) noexcept
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // Note: Since we only support one action, we could have chosen not to pass in a failure action.
    // I have it there to make it explicit and easily readable.
    ASSERT_IFNOT(
        failureAction == FailureAction::AbortStateProvider,
        "{0}: ApiDispatcher::CloseAsync only supports AbortStateProvider. FailureAction: {1}",
        TraceId,
        static_cast<int>(failureAction));

    KArray<Awaitable<NTSTATUS>> awaitableArray(GetThisAllocator(), metadataArray.Count());
    ASSERT_IFNOT(NT_SUCCESS(awaitableArray.Status()), "{0}: Failed to create KArray. Status: {1}", TraceId, status);

    for (Metadata::CSPtr metadata : metadataArray)
    {
        Awaitable<NTSTATUS> closeAwaitable = CloseAsync(*metadata, failureAction, cancellationToken);
        status = awaitableArray.Append(Ktl::Move(closeAwaitable));
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}: Failed to append with code {1}. Array is correctly sized. This is not expected",
            TraceId,
            status);
    }

    // Even if one of the sync paths of CloseAsync failed, we need to ensure that all existing awaitables are co_awaited.
    // We expect the KThreadPool and scheduler to ensure the optimal number of awaitables execute asynchronously.
    status = co_await Utilities::TaskUtilities<NTSTATUS>::WhenAll_NoException(awaitableArray);
    ASSERT_IFNOT(
        NT_SUCCESS(status),
        "{0} ApiDispatcher::CloseAsync failed even though FailureAction is AbortStateProvider. Status: {1}",
        TraceId,
        status);

#ifdef DBG
    for (ULONG index = 0; index < awaitableArray.Count(); index++)
    {
        NTSTATUS awaitableStatus = co_await awaitableArray[index];
        ASSERT_IFNOT(
            NT_SUCCESS(awaitableStatus),
            "{0} ApiDispatcher::CloseAsync failed even though FailureAction is AbortStateProvider. Status: {1}",
            TraceId,
            status);
    }
#endif

    co_return status;
}

void ApiDispatcher::Abort(
    __in KArray<Metadata::CSPtr> const & metadataArray,
    __in ULONG startingIndex,
    __in ULONG count) noexcept
{
    KShared$ApiEntry();

    for (ULONG index = startingIndex; index < startingIndex + count; index++)
    {
        Metadata::CSPtr current = metadataArray[index];

        try
        {
            // Rule: Abort cannot throw.
            Abort(*current);
        }
        catch (Exception & e)
        {
            ASSERT_IFNOT(false, 
                "{0}: State Provider {1} threw ktl::Exception from Abort. Error Code: {2}.", 
                TraceId,
                current->StateProviderId,
                e.GetStatus());
        }
        catch (...)
        {
            ASSERT_IFNOT(false, 
                "{0}: State Provider {1} threw non-ktl::Exception from Abort.", 
                TraceId,
                current->StateProviderId);
        }
    }

    return;
}

Awaitable<NTSTATUS> ApiDispatcher::EndSettingCurrentStateAsync(
    __in KArray<Metadata::CSPtr> const & metadataArray,
    __in ktl::CancellationToken const & cancellationToken) noexcept
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    KArray<Awaitable<NTSTATUS>> awaitableArray(GetThisAllocator(), metadataArray.Count());
    ASSERT_IFNOT(NT_SUCCESS(awaitableArray.Status()), "{0}: Failed to create KArray. Status: {1}", TraceId, status);

    for (Metadata::CSPtr metadata : metadataArray)
    {
        // Assumption: Below code assumes that if the exception is not a ktl::exception, the process will go down.
        status = awaitableArray.Append(EndSettingCurrentStateAsync(*metadata, cancellationToken));
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}: Failed to append with code {1}. Array is correctly sized. This is not expected",
            TraceId,
            status);
    }

    // We expect the KThreadPool and scheduler to ensure the optimal number of Awaitables execute asynchronously.
    status = co_await Utilities::TaskUtilities<void>::WhenAll_NoException(awaitableArray);
    co_return status;
}

NTSTATUS ApiDispatcher::PrepareCheckpoint(
    __in KArray<Metadata::CSPtr> const & metadataArray,
    __in FABRIC_SEQUENCE_NUMBER checkpointLSN) noexcept
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_SUCCESS;

    for (Metadata::CSPtr current : metadataArray)
    {
        // Rule: Abort cannot throw.
        status = PrepareCheckpoint(*current, checkpointLSN);
        if (NT_SUCCESS(status) == false)
        {
            return status;
        }
    }

    return status;
}

Awaitable<NTSTATUS> ApiDispatcher::PerformCheckpointAsync(
    __in KArray<Metadata::CSPtr> const & metadataArray,
    __in ktl::CancellationToken const & cancellationToken) noexcept
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    KArray<Awaitable<NTSTATUS>> awaitableArray(GetThisAllocator(), metadataArray.Count());
    ASSERT_IFNOT(NT_SUCCESS(awaitableArray.Status()), "{0}: Failed to create KArray. Status: {1}", TraceId, status);

    for (Metadata::CSPtr metadata : metadataArray)
    {
        Awaitable<NTSTATUS> performCheckpointAwaitable = PerformCheckpointAsync(*metadata, cancellationToken);
        status = awaitableArray.Append(Ktl::Move(performCheckpointAwaitable));
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}: Failed to append with code {1}. Array is correctly sized. This is not expected",
            TraceId,
            status);
    }

    // We expect the KThreadPool and scheduler to ensure the optimal number of awaitables execute asynchronously.
    status = co_await Utilities::TaskUtilities<NTSTATUS>::WhenAll_NoException(awaitableArray);
    co_return status;
}

Awaitable<NTSTATUS> ApiDispatcher::CompleteCheckpointAsync(
    __in KArray<Metadata::CSPtr> const & metadataArray,
    __in ktl::CancellationToken const & cancellationToken) noexcept
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    KArray<Awaitable<NTSTATUS>> awaitableArray(GetThisAllocator(), metadataArray.Count());
    ASSERT_IFNOT(NT_SUCCESS(awaitableArray.Status()), "{0}: Failed to create KArray. Status: {1}", TraceId, status);

    for (Metadata::CSPtr metadata : metadataArray)
    {
        Awaitable<NTSTATUS> completeAwaitable = CompleteCheckpointAsync(*metadata, cancellationToken);
        status = awaitableArray.Append(Ktl::Move(completeAwaitable));
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}: Failed to append with code {1}. Array is correctly sized. This is not expected",
            TraceId,
            status);
    }

    // We expect the KThreadPool and scheduler to ensure the optimal number of awaitables execute asynchronously.
    status = co_await Utilities::TaskUtilities<NTSTATUS>::WhenAll_NoException(awaitableArray);
    co_return status;
}

Awaitable<NTSTATUS> ApiDispatcher::RecoverCheckpointAsync(
    __in KArray<Metadata::CSPtr> const & metadataArray,
    __in ktl::CancellationToken const & cancellationToken) noexcept
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    KArray<Awaitable<NTSTATUS>> awaitableArray(GetThisAllocator(), metadataArray.Count());
    ASSERT_IFNOT(NT_SUCCESS(awaitableArray.Status()), "{0}: Failed to create KArray. Status: {1}", TraceId, status);

    for (Metadata::CSPtr metadata : metadataArray)
    {
        Awaitable<NTSTATUS> recoverAwaitable = RecoverCheckpointAsync(*metadata, cancellationToken);
        status = awaitableArray.Append(Ktl::Move(recoverAwaitable));
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}: Failed to append with code {1}. Array is correctly sized. This is not expected",
            TraceId,
            status);
    }

    // We expect the KThreadPool and scheduler to ensure the optimal number of awaitables execute asynchronously.
    status = co_await Utilities::TaskUtilities<NTSTATUS>::WhenAll_NoException(awaitableArray);
    co_return status;
}

Awaitable<NTSTATUS> ApiDispatcher::RestoreCheckpointAsync(
    __in KArray<Metadata::CSPtr> const & metadataArray,
    __in KArray<KString::CSPtr> const & backupDirectoryArray,
    __in ktl::CancellationToken const & cancellationToken) noexcept
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    ASSERT_IFNOT(
        metadataArray.Count() == backupDirectoryArray.Count(),
        "{0}: Input array counts do not match. Metadata Count: {1} Directory Count: {2}", 
        TraceId, 
        metadataArray.Count(),
        backupDirectoryArray.Count());

    KArray<Awaitable<NTSTATUS>> awaitableArray(GetThisAllocator(), metadataArray.Count());
    ASSERT_IFNOT(NT_SUCCESS(awaitableArray.Status()), "{0}: Failed to create KArray. Status: {1}", TraceId, status);

    for (ULONG i = 0; i < metadataArray.Count(); i++)
    {
        Awaitable<NTSTATUS> recoverAwaitable = RestoreCheckpointAsync(*metadataArray[i], *backupDirectoryArray[i], cancellationToken);
        status = awaitableArray.Append(Ktl::Move(recoverAwaitable));
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}: Failed to append with code {1}. Array is correctly sized. This is not expected",
            TraceId,
            status);
    }

    // We expect the KThreadPool and scheduler to ensure the optimal number of Awaitables execute asynchronously.
    status = co_await Utilities::TaskUtilities<NTSTATUS>::WhenAll_NoException(awaitableArray);
    co_return status;
}

Awaitable<NTSTATUS> ApiDispatcher::RemoveStateAsync(
    __in KArray<Metadata::CSPtr> const & metadataArray,
    __in ktl::CancellationToken const & cancellationToken) noexcept
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    KArray<Awaitable<NTSTATUS>> awaitableArray(GetThisAllocator(), metadataArray.Count());
    ASSERT_IFNOT(NT_SUCCESS(awaitableArray.Status()), "{0}: Failed to create KArray. Status: {1}", TraceId, status);

    for (Metadata::CSPtr metadata : metadataArray)
    {
        Awaitable<NTSTATUS> recoverAwaitable = RemoveStateAsync(*metadata, cancellationToken);
        status = awaitableArray.Append(Ktl::Move(recoverAwaitable));
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}: Failed to append with code {1}. Array is correctly sized. This is not expected",
            TraceId,
            status);
    }

    // We expect the KThreadPool and scheduler to ensure the optimal number of awaitables execute asynchronously.
    status = co_await Utilities::TaskUtilities<NTSTATUS>::WhenAll_NoException(awaitableArray);
    co_return status;
}

ktl::Awaitable<NTSTATUS> ApiDispatcher::PrepareForRemoveAsync(
    __in Metadata const & metadata,
    __in TxnReplicator::Transaction const & transaction,
    __in ktl::CancellationToken const & cancellationToken) noexcept
{
    KShared$ApiEntry();
    NTSTATUS status = STATUS_SUCCESS;

    try
    {
        co_await metadata.StateProvider->PrepareForRemoveAsync(transaction, cancellationToken);
    }
    catch (Exception const & exception)
    {
        StateManagerEventSource::Events->ISP2_PrepareForRemoveAsync_ApiError(
            TracePartitionId,
            ReplicaId,
            metadata.StateProviderId,
            transaction.TransactionId,
            exception.GetStatus());
        status = exception.GetStatus();
    }
    catch (...)
    {
        ASSERT_IFNOT(
            false,
            "{0}: SPid: {1} threw non-ktl::Exception in PrepareForRemoveAsync.",
            TraceId,
            metadata.StateProviderId);
    }

    co_return status;
}

ktl::Awaitable<NTSTATUS> ApiDispatcher::RemoveStateAsync(
    __in Metadata const & metadata,
    __in ktl::CancellationToken const & cancellationToken) noexcept
{
    KShared$ApiEntry();
    NTSTATUS status = STATUS_SUCCESS;

    try
    {
        co_await metadata.StateProvider->RemoveStateAsync(cancellationToken);
    }
    catch (Exception const & exception)
    {
        StateManagerEventSource::Events->ISP2_ApiError(
            TracePartitionId,
            ReplicaId,
            metadata.StateProviderId,
            RemoveStateAsync_FunctionName,
            exception.GetStatus());
        status = exception.GetStatus();
    }
    catch (...)
    {
        ASSERT_IFNOT(
            false,
            "{0}: SPid: {1} threw non-ktl::Exception in {2}.",
            TraceId,
            metadata.StateProviderId,
            RemoveStateAsync_FunctionName);
    }

    co_return status;
}

NTSTATUS ApiDispatcher::PrepareCheckpoint(
    __in Metadata const & metadata,
    __in FABRIC_SEQUENCE_NUMBER checkpointLSN) noexcept
{
    try
    {
        metadata.StateProvider->PrepareCheckpoint(checkpointLSN);
    }
    catch (Exception const & e)
    {
        StateManagerEventSource::Events->ISP2_PrepareCheckpoint_ApiError(
            TracePartitionId,
            ReplicaId,
            metadata.StateProviderId,
            checkpointLSN,
            e.GetStatus());
        return e.GetStatus();
    }

    return STATUS_SUCCESS;;
}

Awaitable<NTSTATUS> ApiDispatcher::PerformCheckpointAsync(
    __in Metadata const & metadata,
    __in ktl::CancellationToken const & cancellationToken) noexcept
{
    KShared$ApiEntry();

    try
    {
        co_await metadata.StateProvider->PerformCheckpointAsync(cancellationToken);
    }
    catch (Exception const & e)
    {
        StateManagerEventSource::Events->ISP2_ApiError(
            TracePartitionId,
            ReplicaId,
            metadata.StateProviderId,
            PerformCheckpointAsync_FunctionName,
            e.GetStatus());
        co_return e.GetStatus();
    }

    co_return STATUS_SUCCESS;;
}

Awaitable<NTSTATUS> ApiDispatcher::CompleteCheckpointAsync(
    __in Metadata const & metadata,
    __in ktl::CancellationToken const & cancellationToken) noexcept
{
    KShared$ApiEntry();

    try
    {
        co_await metadata.StateProvider->CompleteCheckpointAsync(cancellationToken);
    }
    catch (Exception & e)
    {
        StateManagerEventSource::Events->ISP2_ApiError(
            TracePartitionId,
            ReplicaId,
            metadata.StateProviderId,
            CompleteCheckpointAsync_FunctionName,
            e.GetStatus());
        co_return e.GetStatus();
    }

    co_return STATUS_SUCCESS;;
}

Awaitable<NTSTATUS> ApiDispatcher::RestoreCheckpointAsync(
    __in Metadata const & metadata,
    __in KString const & backupDirectory,
    __in ktl::CancellationToken const & cancellationToken) noexcept
{
    KShared$ApiEntry();

    try
    {
        co_await metadata.StateProvider->RestoreCheckpointAsync(backupDirectory, cancellationToken);
    }
    catch (Exception const & e)
    {
        StateManagerEventSource::Events->ISP2_ApiError(
            TracePartitionId,
            ReplicaId,
            metadata.StateProviderId,
            RestoreCheckpointAsync_FunctionName,
            e.GetStatus());
        co_return e.GetStatus();
    }

    co_return STATUS_SUCCESS;
}

NOFAIL ApiDispatcher::ApiDispatcher(
    __in PartitionedReplicaId const & traceId,
    __in IStateProvider2Factory & stateProviderFactory) noexcept
    : KObject()
    , KShared()
    , PartitionedReplicaTraceComponent(traceId)
    , stateProviderFactorySPtr_(&stateProviderFactory)
{
}

ApiDispatcher::~ApiDispatcher()
{
}
