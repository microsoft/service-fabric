// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Common;
using namespace Data::Utilities;
using namespace TxnReplicator;
using namespace Data::StateManager;

NTSTATUS MetadataManager::Create(
    __in PartitionedReplicaId const & traceType,
    __in KAllocator & allocator,
    __out SPtr & result) noexcept
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    ConcurrentSkipList<FABRIC_STATE_PROVIDER_ID, Metadata::SPtr>::SPtr activeMapSPtr;
    status = ConcurrentSkipList<FABRIC_STATE_PROVIDER_ID, Metadata::SPtr>::Create(allocator, activeMapSPtr);
    RETURN_ON_FAILURE(status);

    ConcurrentSkipList<FABRIC_STATE_PROVIDER_ID, Metadata::SPtr>::SPtr deletedMapSPtr;
    status = ConcurrentSkipList<FABRIC_STATE_PROVIDER_ID, Metadata::SPtr>::Create(allocator, deletedMapSPtr);
    RETURN_ON_FAILURE(status);

    ConcurrentDictionary<KUri::CSPtr, Metadata::SPtr>::SPtr inMemoryStateSPtr;
    status = ConcurrentDictionary<KUri::CSPtr, Metadata::SPtr>::Create(allocator, inMemoryStateSPtr);
    RETURN_ON_FAILURE(status);

    ConcurrentDictionary<KUri::CSPtr, StateManagerLockContext::SPtr>::SPtr keyLockSPtr;
    status = ConcurrentDictionary<KUri::CSPtr, StateManagerLockContext::SPtr>::Create(allocator, keyLockSPtr);
    RETURN_ON_FAILURE(status);

    ConcurrentDictionary<LONG64, KHashSet<KUri::CSPtr>::SPtr>::SPtr inflightTransactionMapSPtr;
    status = ConcurrentDictionary<LONG64, KHashSet<KUri::CSPtr>::SPtr>::Create(allocator, inflightTransactionMapSPtr);
    RETURN_ON_FAILURE(status);

    ConcurrentDictionary<LONG64, KSharedArray<Metadata::SPtr>::SPtr>::SPtr parentToChildrenStateProviderMapSPtr;
    status = ConcurrentDictionary<LONG64, KSharedArray<Metadata::SPtr>::SPtr>::Create(allocator, parentToChildrenStateProviderMapSPtr);
    RETURN_ON_FAILURE(status);

    result = _new(METADATA_MANAGER_TAG, allocator) MetadataManager(
        traceType, 
        *activeMapSPtr, 
        *deletedMapSPtr,
        *inMemoryStateSPtr,
        *keyLockSPtr,
        *inflightTransactionMapSPtr,
        *parentToChildrenStateProviderMapSPtr);

    if (result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(result->Status()))
    {
        return (SPtr(Ktl::Move(result)))->Status();
    }

    return STATUS_SUCCESS;
}

bool MetadataManager::TryRemoveInflightTransaction(
    __in LONG64 transactionId,
    __out KHashSet<KUri::CSPtr>::SPtr lockSet) noexcept
{
    return inflightTransactionsSPtr_->TryRemove(transactionId, lockSet);
}

ULONG32 MetadataManager::GetInflightTransactionCount() const noexcept
{
    return inflightTransactionsSPtr_->Count;
}

Awaitable<void> MetadataManager::AcquireLockAndAddAsync(
    __in KUri const & key, 
    __in Metadata & metadata, 
    __in TimeSpan const & timeout, 
    __in CancellationToken const & cancellationToken)
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    StateManagerLockContext::SPtr lockContextSPtr = nullptr;

    // This function will not be called concurrently, the caller must be sync
    bool lockExists = keyLocksSPtr_->TryGetValue(metadata.Name, lockContextSPtr);
    if (lockExists == false)
    {
        status = StateManagerLockContext::Create(key, metadata.StateProviderId, *this, GetThisAllocator(), lockContextSPtr);
        Helper::ThrowIfNecessary(
            status,
            TracePartitionId,
            ReplicaId,
            L"AcquireLockAndAddAsync: Create StateManagerLockContext.",
            Helper::MetadataManager);

        bool lockAdded = keyLocksSPtr_->TryAdd(metadata.Name, lockContextSPtr);
        ASSERT_IFNOT(
            lockAdded, 
            "{0}: key {1} add failed because the lock key is already present in the dictionary.",
            TraceId,
            ToStringLiteral(key));
    }

    status = co_await lockContextSPtr->AcquireWriteLockAsync(timeout, cancellationToken);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"AcquireLockAndAddAsync: Acquire write lock failed.",
        Helper::MetadataManager);
    ++(lockContextSPtr->GrantorCount);

    try
    {
        Metadata::SPtr metadataSPtr(&metadata);
        bool addResult = inMemoryStateSPtr_->TryAdd(KUri::CSPtr(&key), metadataSPtr);
        ASSERT_IFNOT(
            addResult,
            "{0}: add has failed because the key {1} is already present in memory-map.",
            TraceId,
            ToStringLiteral(key));

        addResult = stateProviderIdMapSPtr_->TryAdd(metadata.StateProviderId, metadataSPtr);
        ASSERT_IFNOT(
            addResult,
            "{0}: failed to add into id-map because the key {1} is already present.",
            TraceId,
            ToStringLiteral(key));
    }
    catch (Exception const & exception)
    {
        status = lockContextSPtr->ReleaseLock(Constants::InvalidTransactionId);
        if (NT_SUCCESS(status) == false)
        {
            TRACE_ERRORS(
                status,
                TracePartitionId,
                ReplicaId,
                Helper::MetadataManager,
                "AcquireLockAndAddAsync: TryAdd throw exception {0}, and ReleaseLock failed with {1}, SPName: {3}.",
                exception.GetStatus(),
                status,
                ToStringLiteral(key));
        }

        throw;
    }

    status = lockContextSPtr->ReleaseLock(Constants::InvalidTransactionId);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"AcquireLockAndAddAsync: Release lock failed.",
        Helper::MetadataManager);

    co_return;
}

// Add Metadata to StateProviderMetadataManager.
bool MetadataManager::TryAdd(
    __in KUri const & key, 
    __in Metadata & metadata)
{
    Metadata::SPtr metadataSPtr(&metadata);
    bool addResult = inMemoryStateSPtr_->TryAdd(KUri::CSPtr(&key), metadataSPtr);

    if (metadataSPtr->TransientCreate == false)
    {
        stateProviderIdMapSPtr_->TryAdd(metadataSPtr->StateProviderId, metadataSPtr);
        ASSERT_IFNOT(
            addResult,
            "{0}: Failed to add into id-map. Name: {1}.",
            TraceId,
            ToStringLiteral(key));
    }

    return addResult;
}

bool MetadataManager::AddDeleted(
    __in FABRIC_STATE_PROVIDER_ID stateProviderId, 
    __in Metadata & metadata)
{
    ASSERT_IFNOT(
        metadata.MetadataMode == MetadataMode::DelayDelete || metadata.MetadataMode == MetadataMode::FalseProgress,
        "{0}: Metadata mode must be DelayDelete | FalseProgress. SPid: {1} Name: {2} Mode: {3}",
        TraceId,
        metadata.StateProviderId,
        ToStringLiteral(*metadata.Name),
        metadata.MetadataMode);
    return deletedStateProvidersSPtr_->TryAdd(stateProviderId, &metadata);
}

bool MetadataManager::TryRemoveDeleted(
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __out Metadata::SPtr & metadata)
{
    return deletedStateProvidersSPtr_->TryRemove(stateProviderId, metadata);
}

// Checks if there are any state providers.
// Use: Assert during recovery that there is no state provider already in in-memory state.
bool MetadataManager::IsEmpty() const
{
    return inMemoryStateSPtr_->Count == 0;
}

// Checks if the state provider is contained.
// Note that only active state providers are queried.
bool MetadataManager::ContainsKey(
    __in KUri const & key,
    __in bool allowTransientState) const
{
    Metadata::SPtr metadataSPtr = nullptr;
    bool stateProviderExists = inMemoryStateSPtr_->TryGetValue(Ktl::Move(KUri::CSPtr(&key)), metadataSPtr);
    if (stateProviderExists == false)
    {
        return false;
    }

    ASSERT_IFNOT(
        metadataSPtr != nullptr,
        "{0}: Metadata cannot be null. Name: {1}",
        TraceId, 
        ToStringLiteral(key));

    if (allowTransientState)
    {
        return true;
    }

    return metadataSPtr->TransientCreate == false;
}

void MetadataManager::Dispose()
{
    if (keyLocksSPtr_ != nullptr)
    {
        IEnumerator<KeyValuePair<KUri::CSPtr, StateManagerLockContext::SPtr>>::SPtr enumerator = keyLocksSPtr_->GetEnumerator();
        while (enumerator->MoveNext())
        {
            enumerator->Current().Value->Close();
        }
    }
}

ULONG MetadataManager::GetInMemoryStateProvidersCount() const
{
    return inMemoryStateSPtr_->Count;
}

ULONG MetadataManager::GetDeletedStateProvidersCount()const
{
    return deletedStateProvidersSPtr_->Count;
}

//
// Get the metadataSPtr if it is active only or active & transient
//
// Algorithm:
// 1. If allow TransientState and it is in the InMemoryState, return the metadataSPtr, otherwise return nullptr
// 2. If not allow TransientState, return metadataSPtr only when the state provider is active, otherwise return nullptr
//
bool MetadataManager::TryGetMetadata(
    __in KUri const & key, 
    __in bool allowTransientState,
    __out Metadata::SPtr & result) const
{
    bool stateProviderExists = inMemoryStateSPtr_->TryGetValue(KUri::CSPtr(&key), result);
    if (stateProviderExists == false)
    {
        result = nullptr;
        return false;
    }

    ASSERT_IFNOT(
        result != nullptr,
        "{0}: Metadata cannot be nullptr. SPName {1}",
        TraceId,
        ToStringLiteral(key));

    if (allowTransientState == false && result->TransientCreate)
    {
        result = nullptr;
        return false;
    }

    return true;
}

//
// Get the metadataSPtr only if it is active
//
// Algorithm:
// 1. Check whether the state providerId is in the stateProviderIdMap
// 2. Return the metadataSPtr if exist, otherwise return nullptr
//
bool MetadataManager::TryGetMetadata(
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __out Metadata::SPtr & result) const
{
    bool stateProviderExists = stateProviderIdMapSPtr_->TryGetValue(stateProviderId, result);
    if (stateProviderExists == false)
    {
        result = nullptr;
        return false;
    }

    ASSERT_IFNOT(
        result != nullptr,
        "{0}: Metadata cannot be nullptr. SPID {1}",
        TraceId,
        stateProviderId);

    return true;
}

// Return the Enumerator of InMemoryState, work with function GetMetadataMoveNext and Current to get the activate metadataCollection
Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, Metadata::SPtr>>::SPtr MetadataManager::GetMetadataCollection() const
{
    KSharedPtr<IEnumerator<KeyValuePair<KUri::CSPtr, Metadata::SPtr>>> enumerator(inMemoryStateSPtr_->GetEnumerator());

    ActiveStateProviderEnumerator::SPtr activeStateProviderEnumerator;
    NTSTATUS status = ActiveStateProviderEnumerator::Create(
        *enumerator,
        StateProviderFilterMode::FilterMode::Active,
        GetThisAllocator(),
        activeStateProviderEnumerator);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"GetMetadataCollection: Create ActiveStateProviderEnumerator.",
        Helper::MetadataManager);

    return activeStateProviderEnumerator.RawPtr();
}

bool MetadataManager::TryGetDeletedMetadata(
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __out Metadata::SPtr & metadata) const
{
    return deletedStateProvidersSPtr_->TryGetValue(stateProviderId, metadata);
}

// 11908632: We could add support for differentiating between TransientCreate and TransientDelete.
NTSTATUS MetadataManager::GetInMemoryMetadataArray(
    __in StateProviderFilterMode::FilterMode filterMode,
    __out KSharedArray<Metadata::CSPtr>::SPtr & outputArray) const noexcept
{
    // Stage 1: Resource allocations.
    KSharedArray<Metadata::CSPtr>::SPtr result = _new(GetThisAllocationTag(), GetThisAllocator()) KSharedArray<Metadata::CSPtr>();
    if (result == nullptr )
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    else if (NT_SUCCESS(result->Status()) == false)
    {
        return result->Status();
    }

    NTSTATUS status = result->Reserve(inMemoryStateSPtr_->Count);
    if (NT_SUCCESS(status) == false)
    {
        return status;
    }

    // Stage 2: Create the enumerable.
    IEnumerator<KeyValuePair<KUri::CSPtr, Metadata::SPtr>>::SPtr enumerator(inMemoryStateSPtr_->GetEnumerator());
    if (enumerator == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    else if (NT_SUCCESS(enumerator->Status()) == false)
    {
        return enumerator->Status();
    }

    // Stage 3: Shallow copy.
    while (enumerator->MoveNext())
    {
        // Note: Copy constructor of CSPtr is NOFAIL. 
        Metadata::CSPtr metadata(enumerator->Current().Value.RawPtr());
        if ((filterMode == StateProviderFilterMode::Active) && (metadata->TransientCreate))
        {
            continue;
        }
        else if ((filterMode == StateProviderFilterMode::Transient) && (metadata->TransientCreate == false))
        {
            continue;
        }

        // Since the array is preallocated with the worst case size, the append must not fail.
        status = result->Append(Ktl::Move(metadata));
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}: Adding into preallocated array failed with status: {1}.",
            TraceId,
            status);
    }

    // Stage 4: Assign the result only on successful completion.
    outputArray = result;
    return STATUS_SUCCESS;
}

NTSTATUS MetadataManager::GetDeletedMetadataArray(
    __out KSharedArray<Metadata::SPtr>::SPtr & outputArray) const noexcept
{
    // Stage 1: Resource allocations.
    KSharedArray<Metadata::SPtr>::SPtr result = _new(GetThisAllocationTag(), GetThisAllocator()) KSharedArray<Metadata::SPtr>();
    if (result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RETURN_ON_FAILURE(result->Status());

    NTSTATUS status = result->Reserve(deletedStateProvidersSPtr_->Count);
    RETURN_ON_FAILURE(status);

    // Stage 2: Create the enumerable.
    IEnumerator<KeyValuePair<FABRIC_STATE_PROVIDER_ID, Metadata::SPtr>>::SPtr enumerator = nullptr;
    status = ConcurrentSkipList<FABRIC_STATE_PROVIDER_ID, Metadata::SPtr>::Enumerator::Create(deletedStateProvidersSPtr_, enumerator);
    RETURN_ON_FAILURE(status);

    // Stage 3: Shallow copy.
    while (enumerator->MoveNext())
    {
        // Note: Copy constructor of CSPtr is NOFAIL. 
        Metadata::SPtr metadata(enumerator->Current().Value);

        status = result->Append(Ktl::Move(metadata));
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}: Adding into preallocated array failed with status: {1}.",
            TraceId,
            status);
    }

    outputArray = result;
    return STATUS_SUCCESS;
}

NTSTATUS MetadataManager::GetDeletedConstMetadataArray(
    __out KSharedArray<Metadata::CSPtr>::SPtr & outputArray) const noexcept
{
    // Stage 1: Resource allocations.
    KSharedArray<Metadata::CSPtr>::SPtr result = _new(GetThisAllocationTag(), GetThisAllocator()) KSharedArray<Metadata::CSPtr>();
    if (result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RETURN_ON_FAILURE(result->Status());

    NTSTATUS status = result->Reserve(deletedStateProvidersSPtr_->Count);
    RETURN_ON_FAILURE(status);

    // Stage 2: Create the enumerable.
    IEnumerator<KeyValuePair<FABRIC_STATE_PROVIDER_ID, Metadata::SPtr>>::SPtr enumerator = nullptr;
    status = ConcurrentSkipList<FABRIC_STATE_PROVIDER_ID, Metadata::SPtr>::Enumerator::Create(deletedStateProvidersSPtr_, enumerator);
    RETURN_ON_FAILURE(status);

    // Stage 3: Shallow copy.
    while (enumerator->MoveNext())
    {
        // Note: Copy constructor of CSPtr is NOFAIL. 
        Metadata::CSPtr metadata(enumerator->Current().Value.RawPtr());

        status = result->Append(Ktl::Move(metadata));
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}: Adding into preallocated array failed with status: {1}.",
            TraceId,
            status);
    }

    outputArray = result;
    return STATUS_SUCCESS;
}

NTSTATUS MetadataManager::MarkAllDeletedStateProviders(
    __in MetadataMode::Enum metadataMode) noexcept
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    IEnumerator<KeyValuePair<FABRIC_STATE_PROVIDER_ID, Metadata::SPtr>>::SPtr enumerator = nullptr;
    status = ConcurrentSkipList<FABRIC_STATE_PROVIDER_ID, Metadata::SPtr>::Enumerator::Create(deletedStateProvidersSPtr_, enumerator);
    RETURN_ON_FAILURE(status);

    while (enumerator->MoveNext())
    {
        enumerator->Current().Value->MetadataMode = metadataMode;
    }

    return status;
}

// Note: To check whether a state provider is Deleted, passing in MetadataMode::Enum::DelayDelete,
//       To check whether a state provider is Stale, passing in MetadataMode::Enum::FalseProgress,
bool MetadataManager::IsStateProviderDeletedOrStale(
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in MetadataMode::Enum filterMode) const
{
    Metadata::SPtr metadataSPtr = nullptr;
    bool doesExist = deletedStateProvidersSPtr_->TryGetValue(stateProviderId, metadataSPtr);
    if (doesExist == true)
    {
        ASSERT_IFNOT(
            metadataSPtr != nullptr,
            "{0}: IsStateProviderDeletedOrStale: Metadata cannot be nullptr. SPID: {1}, CheckMode: {2}",
            TraceId,
            stateProviderId,
            static_cast<int>(filterMode));

        if (metadataSPtr->MetadataMode == MetadataMode::Enum::DelayDelete)
        {
            ASSERT_IFNOT(
                metadataSPtr->DeleteLSN != FABRIC_INVALID_SEQUENCE_NUMBER,
                "{0}: IsStateProviderDeletedOrStale: DelayDeleted (soft deleted) state provider must have DeleteLSN. SPID {1}, CheckMode: {2}",
                TraceId,
                stateProviderId,
                static_cast<int>(filterMode));
        }

        ASSERT_IFNOT(
            (metadataSPtr->MetadataMode == MetadataMode::Enum::DelayDelete) || (metadataSPtr->MetadataMode == MetadataMode::Enum::FalseProgress),
            "{0}: Mode must be DelayDelete or False Progress. SPID {1}, CheckMode: {2}",
            TraceId,
            stateProviderId,
            static_cast<int>(filterMode));

        return metadataSPtr->MetadataMode == filterMode;
    }

    return false;
}

__checkReturn Awaitable<NTSTATUS> MetadataManager::LockForReadAsync(
    __in KUri const & key,
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in TransactionBase const & transaction,
    __in TimeSpan const & timeout,
    __in CancellationToken const & cancellationToken,
    __out StateManagerLockContext::SPtr & lockContextSPtr) noexcept
{
    KCoShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    ASSERT_IFNOT(
        stateProviderId != Constants::InvalidStateProviderId,
        "{0}: State Provider ID cannot be Invalid. Name: {1}",
        TraceId,
        ToStringLiteral(key));

    StateManagerEventSource::Events->MetadataManager_LockForRead(
        TracePartitionId,
        ReplicaId,
        ToStringLiteral(key), 
        stateProviderId, 
        transaction.TransactionId, 
        timeout);

    while (true)
    {
        bool lockExists = keyLocksSPtr_->TryGetValue(&key, lockContextSPtr);
        if (lockExists == false)
        {
            status = StateManagerLockContext::Create(key, stateProviderId, *this, GetThisAllocator(), lockContextSPtr);
            if (NT_SUCCESS(status) == false)
            {
                TRACE_ERRORS(
                    status,
                    TracePartitionId,
                    ReplicaId,
                    Helper::MetadataManager,
                    "LockForReadAsync: Create StateManagerLockContext. SPName: {0}, SPId: {1}, TxnId: {2}.",
                    ToStringLiteral(key),
                    stateProviderId,
                    transaction.TransactionId);
                co_return status;
            }

            bool lockAdded = keyLocksSPtr_->TryAdd(&key, lockContextSPtr);

            // Key might be added concurrently, so if the add failed, skip this round and check if the key exists again.
            if (!lockAdded)
            {
                continue;
            }
        }

        bool isReentrantTransaction = DoesTransactionContainKeyLock(transaction.TransactionId, key);
        if (lockContextSPtr->LockMode == StateManagerLockMode::Enum::Write && isReentrantTransaction)
        {
            // It is safe to do this grantor count increment outside the lock since this transaction already holds this lock.
            ++(lockContextSPtr->GrantorCount);
            break;
        }

        // With the removal of the global lock, when a lock is removed during an inflight transaction this exception can happen.
        // It can be solved by either ref counting or handling disposed exception, since this is a rare situation and ref counting
        // needs to be done every time. Gopalk has acked this recommendation.
        status = co_await lockContextSPtr->AcquireReadLockAsync(timeout, cancellationToken);
        if (NT_SUCCESS(status))
        {
            break;
        }

        if (status != SF_STATUS_OBJECT_DISPOSED)
        {
            TRACE_ERRORS(
                status,
                TracePartitionId,
                ReplicaId,
                Helper::MetadataManager,
                "LockForReadAsync: AcquireReadLockAsync failed. SPName: {0}, SPId: {1}, TxnId: {2}.",
                ToStringLiteral(key),
                stateProviderId,
                transaction.TransactionId);
            co_return status;
        }

        // If the exception is SF_STATUS_OBJECT_DISPOSED, it means the lock is disposed. 
        // So just ignore and continue, the next iteration will create the lock again and 
        // try to acquire lock.
    }

    KHashSet<KUri::CSPtr>::SPtr keyLocks = nullptr;

    // Use the while loop to check the key and add for now to avoid the race condition
    bool lockSetExists = false;
    while (!lockSetExists)
    {
        lockSetExists = inflightTransactionsSPtr_->TryGetValue(transaction.TransactionId, keyLocks);
        if (lockSetExists == false)
        {
            status = KHashSet<KUri::CSPtr>::Create(Constants::NumberOfBuckets, K_DefaultHashFunction, Comparer::Equals, GetThisAllocator(), keyLocks);
            if (NT_SUCCESS(status) == false)
            {
                TRACE_ERRORS(
                    status,
                    TracePartitionId,
                    ReplicaId,
                    Helper::MetadataManager,
                    "LockForReadAsync: Create KHashSet. SPName: {0}, SPId: {1}, TxnId: {2}.",
                    ToStringLiteral(key),
                    stateProviderId,
                    transaction.TransactionId);
                co_return status;
            }

            bool isAdded = inflightTransactionsSPtr_->TryAdd(transaction.TransactionId, keyLocks);
            if (isAdded)
            {
                break;
            }
        }
    }

    keyLocks->TryAdd(&key);

    co_return STATUS_SUCCESS;
}

__checkReturn ktl::Awaitable<NTSTATUS> MetadataManager::LockForWriteAsync(
    __in KUri const & key,
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in TransactionBase const & transaction,
    __in TimeSpan const & timeout,
    __in CancellationToken const & cancellationToken,
    __out StateManagerLockContext::SPtr & lockContextSPtr) noexcept
{
    KCoShared$ApiEntry()

    ASSERT_IFNOT(
        stateProviderId != Constants::InvalidStateProviderId,
        "{0}: State Provider ID cannot be Invalid. Name: {1}",
        TraceId,
        ToStringLiteral(key));

    StateManagerEventSource::Events->MetadataManager_LockForWrite(
        TracePartitionId,
        ReplicaId,
        ToStringLiteral(key), 
        stateProviderId, 
        transaction.TransactionId, 
        timeout);

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    int incrementIndex = 0;
    while(true)
    {
        bool lockExists = keyLocksSPtr_->TryGetValue(&key, lockContextSPtr);
        if (lockExists == false)
        {
            status = StateManagerLockContext::Create(key, stateProviderId, *this, GetThisAllocator(), lockContextSPtr);
            if (NT_SUCCESS(status) == false)
            {
                TRACE_ERRORS(
                    status,
                    TracePartitionId,
                    ReplicaId,
                    Helper::MetadataManager,
                    "LockForWriteAsync: Create StateManagerLockContext. SPName: {0}, SPId: {1}, TxnId: {2}.",
                    ToStringLiteral(key),
                    stateProviderId,
                    transaction.TransactionId);
                co_return status;
            }

            bool lockAdded = keyLocksSPtr_->TryAdd(&key, lockContextSPtr);
            if (!lockAdded)
            {
                continue;
            }
        }

        bool isReentrantTransaction = DoesTransactionContainKeyLock(transaction.TransactionId, key);
        if (lockContextSPtr->LockMode == StateManagerLockMode::Enum::Write && isReentrantTransaction)
        {
            // It is safe to do this grantor count increment outside the lock since this transaction already holds this lock.
            ++(lockContextSPtr->GrantorCount);
            break;
        }
        else if (lockContextSPtr->LockMode == StateManagerLockMode::Enum::Read && isReentrantTransaction)
        {
            // It is safe to do this grantor count increment outside the lock since this transaction already holds this lock.
            status = lockContextSPtr->ReleaseLock(transaction.TransactionId);
            if (NT_SUCCESS(status) == false)
            {
                TRACE_ERRORS(
                    status,
                    TracePartitionId,
                    ReplicaId,
                    Helper::MetadataManager,
                    "LockForWriteAsync: Release StateManagerLockContext. SPName: {0}, SPId: {1}, TxnId: {2}.",
                    ToStringLiteral(key),
                    stateProviderId,
                    transaction.TransactionId);
                co_return status;
            }

            // Increment grantor count by 2 to account for both the operations, but after lock acquisition.
            incrementIndex = 2;
        }
        else
        {
            incrementIndex = 1;
        }

        // With the removal of the global lock, when a lock is removed during an in flight transaction this exception can happen.
        // It can be solved by either ref counting or handling disposed exception, since this is a rare situation and ref counting
        // needs to be done every time. Gopalk has acked this recommendation.
        status = co_await lockContextSPtr->AcquireWriteLockAsync(timeout, cancellationToken);
        if (NT_SUCCESS(status))
        {
            lockContextSPtr->GrantorCount = lockContextSPtr->GrantorCount + incrementIndex;
            break;
        }

        if (status != SF_STATUS_OBJECT_DISPOSED)
        {
            TRACE_ERRORS(
                status,
                TracePartitionId,
                ReplicaId,
                Helper::MetadataManager,
                "LockForWriteAsync: AcquireWriteLockAsync failed. SPName: {0}, SPId: {1}, TxnId: {2}.",
                ToStringLiteral(key),
                stateProviderId,
                transaction.TransactionId);
            co_return status;
        }

        // If the status code is SF_STATUS_OBJECT_DISPOSED, just Ignore and continue to create another lock context.
    }

    KHashSet<KUri::CSPtr>::SPtr keyLocks = nullptr;

    bool lockSetExists = false;
    while (!lockSetExists)
    {
        lockSetExists = inflightTransactionsSPtr_->TryGetValue(transaction.TransactionId, keyLocks);
        if (lockSetExists == false)
        {
            status = KHashSet<KUri::CSPtr>::Create(Constants::NumberOfBuckets, K_DefaultHashFunction, Comparer::Equals, GetThisAllocator(), keyLocks);
            if (!NT_SUCCESS(status))
            {
                TRACE_ERRORS(
                    status,
                    TracePartitionId,
                    ReplicaId,
                    Helper::MetadataManager,
                    "LockForWriteAsync: Create KHashSet. SPName: {0}, SPId: {1}, TxnId: {2}.",
                    ToStringLiteral(key),
                    stateProviderId,
                    transaction.TransactionId);
                co_return status;
            }

            bool isAdded = inflightTransactionsSPtr_->TryAdd(transaction.TransactionId, keyLocks);
            if (isAdded)
            {
                break;
            }
        }
    }

    keyLocks->TryAdd(&key);

    co_return STATUS_SUCCESS;
}

void MetadataManager::MoveStateProvidersToDeletedList()
{
    Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, Metadata::SPtr>>::SPtr metadataEnumerator(GetMetadataCollection());

    while (metadataEnumerator->MoveNext())
    {
        const Metadata::SPtr metadata (metadataEnumerator->Current().Value);
        metadata->MetadataMode = MetadataMode::FalseProgress;

        bool isAdded = deletedStateProvidersSPtr_->TryAdd(metadata->StateProviderId, metadata);
        ASSERT_IFNOT(
            isAdded,
            "{0}: Failed to add state provider to delete list. State Provider Id: {1}. State Provider Name: {2}",
            TraceId,
            metadata->StateProviderId,
            ToStringLiteral(*metadata->Name));

        StateManagerEventSource::Events->MetadataManager_MoveToDeleted(
            TracePartitionId,
            ReplicaId,
            metadata->StateProviderId,
            ToStringLiteral(*metadata->Name));
    }

    // Empty the stateprovider id look up.
    stateProviderIdMapSPtr_->Clear();
}

// Removes data and the lock from the dictionary. 
// This method is always expected to be called within a lock so no lock is acquired here
// Port Note: Removing timeout for now. Timeout was here in case in future deleting a lock requires dereference and waiting.
NTSTATUS MetadataManager::Remove(
    __in KUri const & stateProviderName,
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in LONG64 transactionId) noexcept
{
    StateManagerLockContext::SPtr lockContextSPtr = nullptr;

    // Note: The only exception thrown from TryGetValue is STATUS_INSUFFICIENT_RESOURCES. Since we
    // can not do much for this exception, remove the try-catch block.
    bool keyExists = keyLocksSPtr_->TryGetValue(&stateProviderName, lockContextSPtr);
    ASSERT_IFNOT(
        keyExists == true,
        "{0}: Lock does not exist. Name: {1} SPID: {2} TxnID: {3}",
        TraceId,
        ToStringLiteral(stateProviderName),
        stateProviderId,
        transactionId);

    // remove metadata
    // Note: The only exception thrown from TryGetValue is STATUS_INSUFFICIENT_RESOURCES. Since we
    // can not do much for this exception, remove the try-catch block.
    Metadata::SPtr metadataToBeRemovedSPtr = nullptr;
    bool isRemoved = inMemoryStateSPtr_->TryRemove(KUri::CSPtr(&stateProviderName), metadataToBeRemovedSPtr);
    ASSERT_IFNOT(
        isRemoved == true,
        "{0}: State Provider does not exist in the in-memory. Name: {1} SPID: {2} TxnID: {3}",
        TraceId,
        ToStringLiteral(stateProviderName),
        stateProviderId,
        transactionId);

    // remove lock
    return RemoveLock(*lockContextSPtr, transactionId);
}

Awaitable<Metadata::SPtr> MetadataManager::ResurrectStateProviderAsync(
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in TimeSpan const & timeout,
    __in CancellationToken const & cancellationToken)
{
    KShared$ApiEntry();

    Metadata::SPtr metadataSPtr = nullptr;
    bool isExist = deletedStateProvidersSPtr_->TryGetValue(stateProviderId, metadataSPtr);
    ASSERT_IFNOT(
        isExist == true,
        "{0}: Key {1} does not exist in the deleted state provider list.",
        TraceId,
        stateProviderId);

    co_await AcquireLockAndAddAsync(
        *metadataSPtr->Name,
        *metadataSPtr,
        timeout,
        cancellationToken);

    // Reset delete lsn.
    // Reset mode to active.
    metadataSPtr->DeleteLSN = FABRIC_INVALID_SEQUENCE_NUMBER;
    metadataSPtr->MetadataMode = MetadataMode::Active;

    // soft delete does not close sp, so do not open.
    Metadata::SPtr removedMetadataSPtr = nullptr;
    bool isRemoved = deletedStateProvidersSPtr_->TryRemove(stateProviderId, removedMetadataSPtr);
    ASSERT_IFNOT(
        isRemoved == true,
        "{0}: Failed to remove SPID {1}.",
        TraceId,
        stateProviderId);

    co_return Ktl::Move(metadataSPtr);
}

void MetadataManager::SoftDelete(
    __in KUri const & key, 
    __in MetadataMode::Enum metadataMode)
{
    KUri::CSPtr stateProviderName(&key);

    Metadata::SPtr metadata = nullptr;
    inMemoryStateSPtr_->TryGetValue(stateProviderName, metadata);
    ASSERT_IFNOT(
        metadata != nullptr,
        "{0}: Metadata cannot be null for state provider {1}",
        TraceId,
        ToStringLiteral(*stateProviderName));

    ASSERT_IFNOT(
        metadata->StateProvider != nullptr,
        "{0}: State provider cannot be null for {1}",
        TraceId,
        ToStringLiteral(*stateProviderName));

    // Verify that the metadata is locked.
    StateManagerLockContext::SPtr lockContext;
    bool lockExists = keyLocksSPtr_->TryGetValue(metadata->Name, lockContext);
    ASSERT_IFNOT(lockExists, "{0}: Lock for {1} must exist", TraceId, metadata->StateProviderId);
    ASSERT_IFNOT(
        lockContext->LockMode == StateManagerLockMode::Write, 
        "{0}: LockMode must Write",
        TraceId);

    metadata->MetadataMode = metadataMode;
    bool isAdded = AddDeleted(metadata->StateProviderId, *metadata);
    ASSERT_IFNOT(
        isAdded,
        "{0}: failed to add state provider {1} to delete list",
        TraceId,
        metadata->StateProviderId);

    // only data is removed here, lock should not be removed here as unlock of this transaction needs the lock.
    Metadata::SPtr metadataToBeRemoved = nullptr;
    bool isRemoved = inMemoryStateSPtr_->TryRemove(stateProviderName, metadataToBeRemoved);
    ASSERT_IFNOT(
        isRemoved, 
        "{0}: failed to remove data for key {1}",
        TraceId,
        ToStringLiteral(*stateProviderName));

    // Remove from id-stateprovider provider map.
    Metadata::SPtr metadataSPtr = nullptr;
    isRemoved = stateProviderIdMapSPtr_->TryRemove(metadata->StateProviderId, metadataSPtr);
    ASSERT_IFNOT(
        isRemoved, 
        "{0}: failed to remove MetadataSPtr {1} from id map.",
        TraceId,
        metadata->StateProviderId);
}

//
// Release all the resources associated with the transaction.
//
NTSTATUS MetadataManager::Unlock(__in StateManagerTransactionContext const & transactionContext) noexcept
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    StateManagerLockContext::SPtr lockContextSPtr(transactionContext.LockContext);

    Metadata::SPtr metadataSPtr = nullptr;
    bool isExist = false;
    
    // Note: The only exception thrown from TryGetValue is STATUS_INSUFFICIENT_RESOURCES. Since we
    // can not do much for this exception, remove the try-catch block.
    isExist = TryGetMetadata(*(lockContextSPtr->Key), true, metadataSPtr);

    if (transactionContext.OperationType == OperationType::Enum::Add)
    {
        if (isExist)
        {
            ASSERT_IFNOT(
                metadataSPtr != nullptr,
                "{0}: metadataSPtr can not be nullptr if TryGetMetadata return true.",
                TraceId);

            if (metadataSPtr->StateProviderId == lockContextSPtr->StateProviderId)
            {
                if (metadataSPtr->TransientCreate)
                {
                    // Abort code path: Unlock got called before apply.
                    status = Remove(*(lockContextSPtr->Key), lockContextSPtr->StateProviderId, transactionContext.TransactionId);
                }
                else
                {
                    // Commit code path.
                    status = lockContextSPtr->ReleaseLock(transactionContext.TransactionId);
                }
            }
            else
            {
                // Abort code path: User tried adding the same state provider name twice.
                status = lockContextSPtr->ReleaseLock(transactionContext.TransactionId);
            }
        }
        else
        {
            // Future case: Add followed by a Remove.
            status = RemoveLock(*lockContextSPtr, transactionContext.TransactionId);
        }
    }
    else if (transactionContext.OperationType == OperationType::Enum::Remove)
    {
        if (isExist)
        {
            ASSERT_IFNOT(
                metadataSPtr != nullptr,
                "{0}: metadataSPtr can not be nullptr if TryGetMetadata return true.",
                TraceId);

            if (metadataSPtr->TransientDelete)
            {
                // Abort code path: Apply did not get called.
                metadataSPtr->TransientDelete = false;
                status = lockContextSPtr->ReleaseLock(transactionContext.TransactionId);
            }
            else
            {
                // Abort code path: Due to double delete.
                status = lockContextSPtr->ReleaseLock(transactionContext.TransactionId);
                StateManagerEventSource::Events->MetadataManager_RemoveAbort(
                    TracePartitionId,
                    ReplicaId,
                    ToStringLiteral(*lockContextSPtr->Key), 
                    lockContextSPtr->StateProviderId, 
                    transactionContext.TransactionId);
            }
        }
        else if (deletedStateProvidersSPtr_->ContainsKey(lockContextSPtr->StateProviderId))
        {
            StateManagerEventSource::Events->MetadataManager_RemoveCommit(
                TracePartitionId,
                ReplicaId,
                ToStringLiteral(*lockContextSPtr->Key), 
                lockContextSPtr->StateProviderId, 
                transactionContext.TransactionId);

            // Commit code path.
            status = RemoveLock(*lockContextSPtr, transactionContext.TransactionId);
        }
        else
        {
            // #11908702: What code path can cause this?
            status = RemoveLock(*lockContextSPtr, transactionContext.TransactionId);
        }
    }
    else
    {
        ASSERT_IFNOT(
            transactionContext.OperationType == OperationType::Enum::Read,
            "{0}: OperationType must be Read: {1}",
            TraceId,
            static_cast<LONG64>(transactionContext.OperationType));

        if (!isExist)
        {
            // Abort code path.
            status = RemoveLock(*lockContextSPtr, transactionContext.TransactionId);
        }
        else
        {
            // Commit code path.
            status = lockContextSPtr->ReleaseLock(transactionContext.TransactionId);
        }
    }

    return status;
}

// Test only method
bool MetadataManager::TryGetLockContext(
    __in KUri const & stateProviderName, 
    __out StateManagerLockContext::SPtr & lockContextSPtr) const
{
    return keyLocksSPtr_->TryGetValue(&stateProviderName, lockContextSPtr);
}

bool MetadataManager::DoesTransactionContainKeyLock(
    __in LONG64 transactionId, 
    __in KUri const & key) const
{
    if (transactionId == Constants::InvalidTransactionId)
    {
        return false;
    }

    KHashSet<KUri::CSPtr>::SPtr keySetSPtr = nullptr;
    bool transactionExists = inflightTransactionsSPtr_->TryGetValue(transactionId, keySetSPtr);
    if (transactionExists == false)
    {
        return false;
    }

    ASSERT_IFNOT(
        keySetSPtr != nullptr,
        "{0}: Locked state providers for this transaction cannot be nullptr. TxnID: {1}",
        TraceId,
        transactionId);

    return keySetSPtr->ContainsKey(&key);
}

// Requirement: Caller must keep lockContext alive for the duration of the call.
NTSTATUS MetadataManager::RemoveLock(
    __in StateManagerLockContext & lockContext, 
    __in LONG64 transactionId) noexcept
{
    if (lockContext.GrantorCount == 1)
    {
        try
        {
            if (transactionId != Constants::InvalidTransactionId)
            {
                KHashSet<KUri::CSPtr>::SPtr keyLockCollection;
                inflightTransactionsSPtr_->TryRemove(transactionId, keyLockCollection);
            }

            StateManagerLockContext::SPtr deletedLockContext = nullptr;
            bool isRemoved = keyLocksSPtr_->TryRemove(lockContext.Key, deletedLockContext);
            ASSERT_IFNOT(
                isRemoved,
                "{0}: Failed to remove lock for state provider. SPID: {1} TxnID: {2}",
                TraceId,
                lockContext.StateProviderId,
                transactionId);

            deletedLockContext->Dispose();
            StateManagerEventSource::Events->MetadataManager_RemoveLock(
                TracePartitionId,
                ReplicaId,
                ToStringLiteral(*deletedLockContext->Key),
                transactionId);
        }
        catch (Exception const & exception)
        {
            TRACE_ERRORS(
                exception.GetStatus(),
                TracePartitionId,
                ReplicaId,
                Helper::MetadataManager,
                "MetadataManager::RemoveLock throws. SPName: {0}, SPId: {1}, TxnId: {2}",
                ToStringLiteral(*lockContext.Key),
                lockContext.StateProviderId,
                transactionId);
            return exception.GetStatus();
        }

        return STATUS_SUCCESS;
    }
    
    return lockContext.ReleaseLock(transactionId);
}

void MetadataManager::AddChild(
    __in FABRIC_STATE_PROVIDER_ID parentStateProviderId,
    __in Metadata & childMetadata)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    KSharedArray<Metadata::SPtr>::SPtr currentChildren = nullptr;
    bool parentExists = parentToChildrenStateProviderMapSPtr_->TryGetValue(parentStateProviderId, currentChildren);

    if (parentExists == false)
    {
        currentChildren = _new(GetThisAllocationTag(), GetThisAllocator()) KSharedArray<Metadata::SPtr>();
        THROW_ON_ALLOCATION_FAILURE(currentChildren);
        Helper::ThrowIfNecessary(
            currentChildren->Status(),
            TracePartitionId,
            ReplicaId,
            L"AddChild: Create currentChildren array.",
            Helper::MetadataManager);

        status = currentChildren->Append(&childMetadata);
        Helper::ThrowIfNecessary(
            status,
            TracePartitionId,
            ReplicaId,
            L"AddChild: Append childMetadata to the currentChildren array if parent doesn't exist.",
            Helper::MetadataManager);

        bool addSuccess = parentToChildrenStateProviderMapSPtr_->TryAdd(parentStateProviderId, currentChildren);
        ASSERT_IFNOT(addSuccess, "{0}: could not add child.", TraceId);
        return;
    }

    status = currentChildren->Append(&childMetadata);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"AddChild: Append childMetadata to the currentChildren array if parent exists.",
        Helper::MetadataManager);
}

bool MetadataManager::GetChildrenMetadata(
    __in FABRIC_STATE_PROVIDER_ID parentId,
    __out KSharedArray<Metadata::SPtr>::SPtr & childrenMetadata)
{
    return parentToChildrenStateProviderMapSPtr_->TryGetValue(parentId, childrenMetadata);
}

bool MetadataManager::GetChildren(
    __in FABRIC_STATE_PROVIDER_ID parentId,
    __out KSharedArray<IStateProvider2::SPtr>::SPtr & childrenMetadata)
{
    KSharedArray<Metadata::SPtr>::SPtr childrenMetadataSPtr = nullptr;
    bool hasChildren = GetChildrenMetadata(parentId, childrenMetadataSPtr);

    if (hasChildren == false)
    {
        childrenMetadata = nullptr;
        return false;
    }

    childrenMetadata = _new(GetThisAllocationTag(), GetThisAllocator()) KSharedArray<IStateProvider2::SPtr>();
    THROW_ON_ALLOCATION_FAILURE(childrenMetadata);
    Helper::ThrowIfNecessary(
        childrenMetadata->Status(),
        TracePartitionId,
        ReplicaId,
        L"GetChildren: Create childrenMetadata array.",
        Helper::MetadataManager);

    for (ULONG32 i = 0; i < childrenMetadataSPtr->Count(); ++i)
    {
        NTSTATUS status = childrenMetadata->Append((*childrenMetadataSPtr)[i]->StateProvider);
        Helper::ThrowIfNecessary(
            status,
            TracePartitionId,
            ReplicaId,
            L"GetChildren: Append childrenMetadata to childrenMetadata array.",
            Helper::MetadataManager);
    }

    return true;
}

bool MetadataManager::TryRemoveParent(
    __in FABRIC_STATE_PROVIDER_ID parentStateProviderId,
    __out KSharedArray<Metadata::SPtr>::SPtr & childrenMetadata)
{
    return parentToChildrenStateProviderMapSPtr_->TryRemove(parentStateProviderId, childrenMetadata);
}

void MetadataManager::ResetTransientState(__in KUri const & stateProviderName)
{
    Metadata::SPtr metadataSPtr = nullptr;
    bool metadataExists = inMemoryStateSPtr_->TryGetValue(KUri::CSPtr(&stateProviderName), metadataSPtr);
    ASSERT_IFNOT(
        metadataExists, 
        "{0}: Could not find in-memory map. Name: {1}",
        TraceId, 
        ToStringLiteral(stateProviderName));

    metadataSPtr->TransientCreate = false;

    bool isAdded = stateProviderIdMapSPtr_->TryAdd(metadataSPtr->StateProviderId, metadataSPtr);
    ASSERT_IFNOT(isAdded, "{0}: Could not to active state providers. SPID: {1}", TraceId, metadataSPtr->StateProviderId);
}

MetadataManager::MetadataManager(
    __in const PartitionedReplicaId & partitionedReplicaId,
    __in ConcurrentSkipList<FABRIC_STATE_PROVIDER_ID, Metadata::SPtr> & stateProviderIdMap,
    __in ConcurrentSkipList<FABRIC_STATE_PROVIDER_ID, Metadata::SPtr> & deletedStateProviderIdMap,
    __in ConcurrentDictionary<KUri::CSPtr, Metadata::SPtr> & inMemoryStateMap,
    __in ConcurrentDictionary<KUri::CSPtr, StateManagerLockContext::SPtr> & keyLockMap,
    __in ConcurrentDictionary<LONG64, KHashSet<KUri::CSPtr>::SPtr> & inflightTransactionMap,
    __in ConcurrentDictionary<LONG64, KSharedArray<Metadata::SPtr>::SPtr> & parentChildrenMap) noexcept
    : KObject()
    , KShared()
    , KWeakRefType()
    , PartitionedReplicaTraceComponent(partitionedReplicaId)
    , stateProviderIdMapSPtr_(&stateProviderIdMap)
    , deletedStateProvidersSPtr_(&deletedStateProviderIdMap)
    , inMemoryStateSPtr_(&inMemoryStateMap)
    , keyLocksSPtr_(&keyLockMap)
    , inflightTransactionsSPtr_(&inflightTransactionMap)
    , parentToChildrenStateProviderMapSPtr_(&parentChildrenMap)
{
}

MetadataManager::~MetadataManager()
{
}
