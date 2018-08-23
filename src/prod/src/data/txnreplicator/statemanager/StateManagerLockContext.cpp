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

NTSTATUS StateManagerLockContext::Create(
    __in KUri const & key, 
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in MetadataManager& metadataManager,
    __in KAllocator& allocator, 
    __out SPtr & result) noexcept
{
    result = _new(SM_TRANSACTIONCONTEXT_TAG, allocator) StateManagerLockContext(key, stateProviderId, metadataManager);
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

ktl::Awaitable<NTSTATUS> StateManagerLockContext::AcquireReadLockAsync(
    __in TimeSpan const& timeout, 
    __in CancellationToken const& cancellationToken) noexcept
{
    KCoShared$ApiEntry();

    InterlockedIncrement(&lockWaiterCount_);
    KFinally([&]() { InterlockedDecrement(&lockWaiterCount_); });

    // No exception handling since no need to convert ObjectClosed to ObjectDisposed
    bool lockAcquired = true;
    try
    {
        lockAcquired = co_await keyLockSPtr_->AcquireReadLockAsync(static_cast<int>(timeout.TotalMilliseconds()));
    }
    catch (Exception & e)
    {
        if ((e.GetStatus() == SF_STATUS_OBJECT_CLOSED) && isDisposed_)
        {
            // This is used to distinguish between lock disappearing underneath the transaction.
            // Example case, Transaction that created the lock being aborted, causes lock to close underneath a parallel read transaction 
            // waiting for the lock.
            co_return SF_STATUS_OBJECT_DISPOSED;
        }

        co_return e.GetStatus();
    }

    if (lockAcquired == false)
    {
        co_return SF_STATUS_TIMEOUT;
    }

    lockMode_ = StateManagerLockMode::Enum::Read;

    co_return STATUS_SUCCESS;
}

ktl::Awaitable<NTSTATUS> StateManagerLockContext::AcquireWriteLockAsync(
    __in TimeSpan const& timeout, 
    __in CancellationToken const& cancellationToken) noexcept
{
    KCoShared$ApiEntry();

    InterlockedIncrement(&lockWaiterCount_);
    KFinally([&]() { InterlockedDecrement(&lockWaiterCount_); });

    // No exception handling since no need to convert ObjectClosed to ObjectDisposed
    bool lockAcquired = true;
    try
    {
        lockAcquired = co_await keyLockSPtr_->AcquireWriteLockAsync(static_cast<int>(timeout.TotalMilliseconds()));
    }
    catch (Exception & e)
    {
        if ((e.GetStatus() == SF_STATUS_OBJECT_CLOSED) && isDisposed_)
        {
            // This is used to distinguish between lock disappearing underneath the transaction.
            // Example case, Transaction that created the lock being aborted, causes lock to close underneath a parallel write transaction 
            // waiting for the lock.
            co_return SF_STATUS_OBJECT_DISPOSED;
        }

        co_return e.GetStatus();
    }

    if (lockAcquired == false)
    {
        co_return SF_STATUS_TIMEOUT;
    }

    this->lockMode_ = StateManagerLockMode::Enum::Write;

    co_return STATUS_SUCCESS;
}

NTSTATUS StateManagerLockContext::ReleaseLock(__in LONG64 transactionId) noexcept
{
    MetadataManager::SPtr metadataManagerSPtr(metadataManagerSPtr_->TryGetTarget());

    ASSERT_IFNOT(metadataManagerSPtr != nullptr, "Null metadata manager during releasing locks in state manager lock context");

    if (lockMode_ == StateManagerLockMode::Enum::Write)
    {
        // PORT NOTE: This is interesting. 
        // This class does not know which transaction holds the lock, hence does not do re-entrency.
        // Instead SPMM keeps track of map of txn to held locks and SMLockContext exposes grantor count for SPMM to increment
        // On the release path (!Txn:Unlock), it decrements to clean up SPMM map if necessary.
        // #11908695: Consider moving the logic here so that SMLockContext knows who acquired the lock and can do re-entrancy fully.
        // 
        grantorCount_--;
        ASSERT_IFNOT(grantorCount_ >= 0, "Lock grantor count should not be negative during releasing locks in state manager lock context");

        if (grantorCount_ == 0)
        {
            try
            {
                keyLockSPtr_->ReleaseWriteLock();
            }
            catch (Exception & e)
            {
                return e.GetStatus();
            }

            // InvalidTransactionId is used for lock-only (transaction free) operations.
            if (transactionId != Constants::InvalidTransactionId)
            {
                KHashSet<KUri::CSPtr>::SPtr keyLockCollection = nullptr;
                metadataManagerSPtr->TryRemoveInflightTransaction(transactionId, keyLockCollection);
            }
        }
    }
    else
    {
        try
        {
            keyLockSPtr_->ReleaseReadLock();
        }
        catch (Exception & e)
        {
            return e.GetStatus();
        }

        // InvalidTransactionId is used for lock-only (transaction free) operations.
        if (transactionId != Constants::InvalidTransactionId)
        {
            KHashSet<KUri::CSPtr>::SPtr keyLockCollection = nullptr;
            metadataManagerSPtr->TryRemoveInflightTransaction(transactionId, keyLockCollection);
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS StateManagerLockContext::Unlock(__in StateManagerTransactionContext & transactionContext) noexcept
{
    MetadataManager::SPtr metadataManagerSPtr = nullptr;

    // Since KWeakRef::TryGetTarget will not throw exception, remove it from try-catch block.
    metadataManagerSPtr = metadataManagerSPtr_->TryGetTarget();
    ASSERT_IFNOT(metadataManagerSPtr != nullptr, "Null metadata manager during unlock");

    return metadataManagerSPtr->Unlock(transactionContext);
}

void StateManagerLockContext::Close()
{
    keyLockSPtr_->Close();
}

void StateManagerLockContext::Dispose()
{
    ASSERT_IFNOT(isDisposed_ == false, "Lock context object already disposed while disposing");

    isDisposed_ = true;
    keyLockSPtr_->Close();
}

NOFAIL StateManagerLockContext::StateManagerLockContext(
    __in KUri const & key,
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in MetadataManager & metadataManager)
    : key_(&key)
    , stateProviderId_(stateProviderId)
    , metadataManagerSPtr_(metadataManager.GetWeakRef())
    , grantorCount_(0)
    , lockMode_(StateManagerLockMode::Enum::Read)
    , isDisposed_(false)
{
    NTSTATUS status = ReaderWriterAsyncLock::Create(GetThisAllocator(), GetThisAllocationTag(), keyLockSPtr_);
    SetConstructorStatus(status);
}

StateManagerLockContext::~StateManagerLockContext()
{
    // We must not close or dispose here. Close and Dispose is an act of terminating the lock.
    // SPMM keeps is locks today.
}
