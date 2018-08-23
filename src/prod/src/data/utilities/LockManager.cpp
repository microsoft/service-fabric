// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::Utilities;

#define LOCKMANAGER_TAG 'rgML'

ULONG MyHashFunction(const LockMode::Enum& Val)
{
   return ~Val;
}

LockManager::LockManager() :
    tableLockSPtr_(nullptr),
    lockReleasedCleanupInProgress_(GetThisAllocator(), lockHashTableCount_),
    lockHashTables_(GetThisAllocator(), lockHashTableCount_),
    status_(false),
    lockCompatibilityTableSPtr_(nullptr),
    lockConversionTableSPtr_(nullptr),
    lockModeComparerSPtr_(nullptr)
{
    NTSTATUS status = LockModeComparer::Create(this->GetThisAllocator(), lockModeComparerSPtr_);
    if (!NT_SUCCESS(status))
    {
        this->SetConstructorStatus(status);
        return;
    }

    status = Dictionary<LockMode::Enum, KSharedPtr<Dictionary<LockMode::Enum, LockCompatibility::Enum>>>::Create(32, MyHashFunction, *lockModeComparerSPtr_, this->GetThisAllocator(), lockCompatibilityTableSPtr_);
    if (!NT_SUCCESS(status))
    {
        this->SetConstructorStatus(status);
        return;
    }

    status = Dictionary<LockMode::Enum, KSharedPtr<Dictionary<LockMode::Enum, LockMode::Enum>>>::Create(32, MyHashFunction, *lockModeComparerSPtr_, this->GetThisAllocator(), lockConversionTableSPtr_);
    if (!NT_SUCCESS(status))
    {
       this->SetConstructorStatus(status);
    }

    status = LoadCompatibilityMatrix();
    if (!NT_SUCCESS(status))
    {
       this->SetConstructorStatus(status);
    }

    status = LoadConversionMatrix();
    this->SetConstructorStatus(status);
}

LockManager::~LockManager()
{
}

NTSTATUS
LockManager::Create(
    __in KAllocator& allocator, 
    __out LockManager::SPtr& result)
{
    NTSTATUS status;
    LockManager::SPtr output = _new(LOCKMANAGER_TAG, allocator) LockManager();

    if (!output)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = output->Status();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    result = Ktl::Move(output);
    return STATUS_SUCCESS;
}

ktl::Awaitable<void> LockManager::OpenAsync() noexcept
{
    ktl::kservice::OpenAwaiter::SPtr openAwaiter;
    NTSTATUS status = ktl::kservice::OpenAwaiter::Create(GetThisAllocator(), GetThisAllocationTag(), *this, openAwaiter);
    THROW_ON_FAILURE(status);
    status = co_await *openAwaiter;
    ASSERT_IFNOT(NT_SUCCESS(status), "Unsucessful open. Status: {1}", status);
}

ktl::Awaitable<void> LockManager::CloseAsync() noexcept
{
    //
    // Set the status to closed. New lock request will be timed-out immediately.
    //
    status_ = false;
    // Close the table lock to force all outstanding prime lock requests to finish (i.e. prime lock acquires with infinite timeouts)
    tableLockSPtr_->Close();
    ktl::kservice::CloseAwaiter::SPtr closeAwaiter;
    NTSTATUS status = ktl::kservice::CloseAwaiter::Create(GetThisAllocator(), GetThisAllocationTag(), *this, closeAwaiter);
    THROW_ON_FAILURE(status);
    status = co_await *closeAwaiter;
    ASSERT_IFNOT(NT_SUCCESS(status), "Unsuccessful close. Status: {0}", status);
}

 ktl::Awaitable<void> LockManager::AcquirePrimeLockAsync(
     __in LockMode::Enum lockMode, 
     __in Common::TimeSpan timeout/*, CancellationToekn*/)
{
    LockManagerApiEntry();
    ASSERT_IFNOT(timeout >= Common::TimeSpan::Zero, "Invalid timeout={0}", timeout.TotalMilliseconds());

    if (lockMode == LockMode::Shared)
    {
        bool lockAcquired = co_await tableLockSPtr_->AcquireReadLockAsync(static_cast<ULONG32>(timeout.TotalMilliseconds()));
        if (lockAcquired == false)
        {
            throw ktl::Exception(SF_STATUS_TIMEOUT);
        }
    }
    else if(lockMode == LockMode::Enum::Exclusive)
    {
        bool lockAcquired = co_await tableLockSPtr_->AcquireWriteLockAsync(static_cast<ULONG32>(timeout.TotalMilliseconds()));
        if (lockAcquired == false)
        {
            throw ktl::Exception(SF_STATUS_TIMEOUT);
        }
    }
    else
    {
        CODING_ERROR_ASSERT(false);
    }
}

 void LockManager::ReleasePrimeLock(__in LockMode::Enum lockMode)
 {
    LockManagerApiEntry();

    ASSERT_IFNOT(lockMode == LockMode::Enum::Shared || lockMode == LockMode::Enum::Exclusive, "Invalid lock mode={0}", static_cast<int>(lockMode));
    if (lockMode == LockMode::Enum::Shared)
    {
       tableLockSPtr_->ReleaseReadLock();
    }
    else
    {
       tableLockSPtr_->ReleaseWriteLock();
    }
 }

 ktl::Awaitable<KSharedPtr<LockControlBlock>> LockManager::AcquireLockAsync(
    __in LONG64 owner,
    __in ULONG64 resourceNameHash,
    __in LockMode::Enum mode,
    __in Common::TimeSpan timeout)
 {
    LockManagerApiEntry();

    //
    // Check arguments.
    //
     ASSERT_IFNOT(timeout >= Common::TimeSpan::Zero, "Invalid timeout {0}", timeout.TotalMilliseconds());

    if (mode == LockMode::Enum::Free)
    {
        throw ktl::Exception(STATUS_INVALID_PARAMETER_3);
    }

    AwaitableCompletionSource<KSharedPtr<LockControlBlock>>::SPtr waiterTcs = nullptr;
    NTSTATUS status = AwaitableCompletionSource<KSharedPtr<LockControlBlock>>::Create(GetThisAllocator(), 0, waiterTcs);
    THROW_ON_FAILURE(status);
    LockHashValue::SPtr lockHashValueSPtr = nullptr;
    ULONG32 lockHashTableIndex = static_cast<ULONG32>(resourceNameHash % lockHashTableCount_);
    auto lockHashTableSPtr = lockHashTables_[lockHashTableIndex];
    auto isGranted = false;
    auto isPending = false;
    auto duplicate = false;
    auto isUpgrade = false;

    // Acquire first level lock
    lockHashTableSPtr->EnterReadLock();

    if (!status_)
    {
       //
       // Release first level lock.
       //
       lockHashTableSPtr->ExitReadLock();

       //
       // Timeout lock request immediately.
       //
       LockControlBlock::SPtr lockControlBlockSPtr = nullptr;
       status = LockControlBlock::Create(*this, owner, resourceNameHash, mode, timeout, LockStatus::Invalid, false, GetThisAllocator(), lockControlBlockSPtr);
       THROW_ON_FAILURE(status);
       waiterTcs->SetResult(lockControlBlockSPtr);
       return waiterTcs->GetAwaitable();
    }

    LockMode::Enum tableLockMode = LockMode::Enum::Shared;
    bool lockHashFound = lockHashTableSPtr->LockEntries->TryGetValue(resourceNameHash, lockHashValueSPtr);
    if (!lockHashFound)
    {
        // Didn't find the lock hash, so try again in exclusive mode in case we need to insert
       lockHashTableSPtr->ExitReadLock();
       lockHashTableSPtr->EnterWriteLock();

       lockHashFound = lockHashTableSPtr->LockEntries->TryGetValue(resourceNameHash, lockHashValueSPtr);
       tableLockMode = LockMode::Enum::Exclusive;
    }

    //
    // Find the lock resource entry, if it exists.
    //
    if (lockHashFound)
    {
       //
       // Acquire second level lock.
       //
       lockHashValueSPtr->EnterWriteLock();

       //
       // Release first level lock.
       //
       switch (tableLockMode)
       {
       case LockMode::Shared:
           lockHashTableSPtr->ExitReadLock();
           break;
       case LockMode::Exclusive:
           lockHashTableSPtr->ExitWriteLock();
           break;
       default:
           ASSERT_IFNOT(false, "Unhandled lock mode={0}", static_cast<int>(tableLockMode));
       }

       //
       // Attempt to find the lock owner in the granted list.
       //
       auto lockControlBlockSPtr = lockHashValueSPtr->ResourceControlBlock->LocateLockClient(owner, false);
       if (lockControlBlockSPtr == nullptr)
       {
          //
          // Lock owner not found in the granted list. 
          //
          if (lockHashValueSPtr->ResourceControlBlock->WaitingQueue->Count() != 0)
          {
             //
             // There are already waiters for this lock. The request therefore cannot be granted in fairness.
             //
             isPending = true;
          }
          else
          {
             if (IsCompatible(mode, lockHashValueSPtr->ResourceControlBlock->LockModeGranted))
             {
                isGranted = true;
             }
             else
             {
                isPending = true;
             }
          }
       }
       else
       {
          //
          // Lock owner found in the granted list.
          //
          if (lockControlBlockSPtr->GetLockMode() == mode)
          {
             //
             // This is a duplicate request from the same lock owner with the same lock mode.
             // Grant it immediately, since it is already granted.
             //
             isGranted = true;
             duplicate = true;
          }
          else
          {
             //
             // The lock owner is requesting a new lock mode (upgrade or downgrade).
             // There are two cases for which the request will be granted.
             // 1. The lock mode requested is compatible with the existent granted mode.
             // 2. There is a single lock owner in the granted list.
             //
             if (IsCompatible(mode, lockHashValueSPtr->ResourceControlBlock->LockModeGranted) ||
                lockHashValueSPtr->ResourceControlBlock->IsSingleLockOwnerGranted(owner))
             {
                //
                // Fairness might be violated in this case, in case existent waiters exist.
                // The reason for violating fairness is that the lock is already granted to this lock owner.
                //
                isGranted = true;
             }
             else
             {
                isPending = true;
                isUpgrade = true;
             }
          }
       }

       //
       // Internal state checks.
       //
       ASSERT_IFNOT(isGranted || isPending, "Invalid state. isGranted={0} isPending={1}", isGranted, isPending);
       if (isGranted)
       {
          //
          // Check to see if the lock request is a duplicate request.
          //
          if (!duplicate)
          {
             //
             // The lock request is added to the granted list and the granted mode is re-computed.
             //
             status = LockControlBlock::Create(*this, owner, resourceNameHash, mode, timeout, LockStatus::Granted, false, GetThisAllocator(), lockControlBlockSPtr);
             THROW_ON_FAILURE(status);
             lockControlBlockSPtr->SetGrantedTime(KDateTime::Now());

             //
             // Grant the max lock mode.
             //
             bool wasEmpty = lockHashValueSPtr->ResourceControlBlock->HasClients() == false;
             status = lockHashValueSPtr->ResourceControlBlock->GrantedList->Append(lockControlBlockSPtr);

             if (wasEmpty)
             {
                 lockHashTableSPtr->DecrementEmptyLocksCount();
             }

             ASSERT_IFNOT(NT_SUCCESS(status), "Failed to append to granted list. status={0}", status);

             //
             // Set the lock resource granted lock status.
             //
             lockHashValueSPtr->ResourceControlBlock->LockModeGranted =
                ConvertToMaxLockMode(mode, lockHashValueSPtr->ResourceControlBlock->LockModeGranted);
          }
          else
          {
             //
             // Return existent lock control block.
             //
             ASSERT_IFNOT(lockControlBlockSPtr != nullptr, "existing lock control block is null");
             lockControlBlockSPtr->LockCount++;
          }

          //
          // Release second level lock.
          //
          lockHashValueSPtr->ExitWriteLock();

          //
          // Return immediately.
          //
          waiterTcs->SetResult(lockControlBlockSPtr);
          return waiterTcs->GetAwaitable();
       }
       else
       {
          //
          // Check to see if this is an instant lock.
          //
          if (timeout == Common::TimeSpan::Zero)
          {
             //
             // Release second level lock.
             //
             lockHashValueSPtr->ExitWriteLock();

             //
             // Timeout lock request immediately.
             //
             status = LockControlBlock::Create(*this, owner, resourceNameHash, mode, timeout, LockStatus::Timeout, false, GetThisAllocator(), lockControlBlockSPtr);
             THROW_ON_FAILURE(status);
             waiterTcs->SetResult(lockControlBlockSPtr);
             return waiterTcs->GetAwaitable();
          }

          //
          // The lock request is added to the waiting list.
          //
          status = LockControlBlock::Create(*this, owner, resourceNameHash, mode, timeout, LockStatus::Pending, isUpgrade, GetThisAllocator(), lockControlBlockSPtr);
          THROW_ON_FAILURE(status);
          if (!isUpgrade)
          {
             //
             // Add the new lock control block to the waiting queue.
             //
             status = lockHashValueSPtr->ResourceControlBlock->WaitingQueue->Append(lockControlBlockSPtr);
             ASSERT_IFNOT(NT_SUCCESS(status), "Failed to apend to waiting queue. status={0}", status);

             // If added to the waiting queue, that means the granted list is non-empty. No need to change the number of empty locks for the hash table
          }
          else
          {
             //
             // Add the new lock control block before any lock control block that is not upgraded, 
             // but after the last upgraded lock control block.
             //
             ULONG32 index = 0;
             for (index = 0; index < lockHashValueSPtr->ResourceControlBlock->WaitingQueue->Count(); index++)
             {
                auto waitingQueue = lockHashValueSPtr->ResourceControlBlock->WaitingQueue;
                auto waiter = (*waitingQueue)[index];
                if (!waiter->IsUgraded())
                {
                   break;
                }
             }

             auto waitingQueue = lockHashValueSPtr->ResourceControlBlock->WaitingQueue;
             status = waitingQueue->InsertAt(index, lockControlBlockSPtr);
             ASSERT_IFNOT(NT_SUCCESS(status), "Failed to insert into waiting queue at index={0}. status={1}", index, status);
          }

          //
          // Set the waiting completion task source on the lock control block.
          //
          AwaitableCompletionSource<KSharedPtr<LockControlBlock>>::SPtr lWaiterTcs = nullptr;
          status = AwaitableCompletionSource<KSharedPtr<LockControlBlock>>::Create(GetThisAllocator(), 0, lWaiterTcs);
          THROW_ON_FAILURE(status);
          lockControlBlockSPtr->Waiter = *lWaiterTcs;

          //
          // Start the expiration for this lock control block.
          //
          lockControlBlockSPtr->StartExpire();

          //
          // Release second level lock.
          //
          lockHashValueSPtr->ExitWriteLock();

          //
          // Done with this request. The task is pending.
          //
          return lWaiterTcs->GetAwaitable();
       }
    }
    else
    {
       //
       // New lock resource being created.
       //
       status = LockHashValue::Create(GetThisAllocator(), lockHashValueSPtr);
       THROW_ON_FAILURE(status);

       //
       // Store lock resource name with its lock control block.
       //
       lockHashTableSPtr->LockEntries->Add(resourceNameHash, lockHashValueSPtr);

       // Adding a new lock that currently has no granted/waiters. This is cancelled out below...
       // lockHashTableSPtr->IncrementEmptyLocksCount();

       //
       // Create new lock control block.
       //
       LockControlBlock::SPtr lockControlBlockSPtr = nullptr;
       status = LockControlBlock::Create(*this, owner, resourceNameHash, mode, timeout, LockStatus::Granted, false, GetThisAllocator(), lockControlBlockSPtr);
       THROW_ON_FAILURE(status);
       lockControlBlockSPtr->SetGrantedTime(KDateTime::Now());

       //
       // Add the new lock control block to the granted list.
       //
       status = lockHashValueSPtr->ResourceControlBlock->GrantedList->Append(lockControlBlockSPtr);
       KInvariant(NT_SUCCESS(status));

       // Lock is no longer empty, cancelling out increment above
       // lockHashTableSPtr->DecrementEmptyLocksCount();

       //
       // Set the lock resource granted lock status.
       //
       lockHashValueSPtr->ResourceControlBlock->LockModeGranted = ConvertToMaxLockMode(mode, LockMode::Enum::Free);

       //
       // Release first level lock.
       //
       switch (tableLockMode)
       {
       case LockMode::Shared:
           lockHashTableSPtr->ExitReadLock();
           break;
       case LockMode::Exclusive:
           lockHashTableSPtr->ExitWriteLock();
           break;
       default:
           ASSERT_IFNOT(false, "Unhandled lock mode={0}", static_cast<int>(tableLockMode));
       }

       //
       // Return immediately.
       //
       waiterTcs->SetResult(lockControlBlockSPtr);
       return waiterTcs->GetAwaitable();
    }
 }

 UnlockStatus::Enum LockManager::ReleaseLock(__in LockControlBlock& acquiredLock)
 {
    LockManagerApiEntry();
    LockHashValue::SPtr lockHashValueSPtr = nullptr;
    ULONG32 lockHashTableIndex = static_cast<ULONG32>(acquiredLock.LockResourceNameHash % lockHashTableCount_);
    auto lockHashTableSPtr = lockHashTables_[lockHashTableIndex];

    //
    // Acquire first level lock.
    //
    lockHashTableSPtr->EnterReadLock();

    //
    // Find the right lock resource, if it exists.
    //
    if (lockHashTableSPtr->LockEntries->TryGetValue(acquiredLock.LockResourceNameHash, lockHashValueSPtr))
    {
        LONG64 cleanupInProgress = lockReleasedCleanupInProgress_[lockHashTableIndex];
        if (cleanupInProgress == 0 && lockHashTableSPtr->EmptyLocksCount > clearLocksThreshold_)
        {
            lockReleasedCleanupInProgress_[lockHashTableIndex] = 1;
            ktl::Task task = ClearLocksInBackground(lockHashTableIndex);
            ASSERT_IFNOT(task.IsTaskStarted(), "Failed to start clear locks background task");
        }

       //
       // Acquire second level lock.
       //
       lockHashValueSPtr->EnterWriteLock();

       //
       // Release first level lock.
       //
       lockHashTableSPtr->ExitReadLock();

       if (!ContainsKey(*(lockHashValueSPtr->ResourceControlBlock->GrantedList), acquiredLock))
       {
          //
          // Release second level lock.
          //
          lockHashValueSPtr->ExitWriteLock();

          //
          // If no lock owner found, return immediately.
          //
          return UnlockStatus::Enum::NotGranted;
       }

       //
       // Decrement the lock count for this granted lock.
       //
       acquiredLock.LockCount--;

       //
       // Check if the lock can be removed.
       //
       if (acquiredLock.LockCount == 0)
       {
          //
          // Remove the lock control block for this lock owner.
          //
          auto index = GetIndex(*lockHashValueSPtr->ResourceControlBlock->GrantedList, acquiredLock);
          KInvariant(index >= 0);
          NTSTATUS status = lockHashValueSPtr->ResourceControlBlock->GrantedList->Remove(index);
          KInvariant(NT_SUCCESS(status));

          bool isEmpty = lockHashValueSPtr->ResourceControlBlock->HasClients() == false;
          if (isEmpty)
          {
              lockHashTableSPtr->IncrementEmptyLocksCount();
          }

          //
          // This lock control block cannot be reused after this call.
          //
          if (acquiredLock.StopExpire())
          {
             acquiredLock.Close();
          }

          //
          // Recompute the new grantees and lock granted mode.
          RecomputeLockGrantees(*lockHashValueSPtr, acquiredLock, false);
       }

       //
       // Release second level lock.
       //
       lockHashValueSPtr->ExitWriteLock();

       return UnlockStatus::Enum::Success;
    }
    else
    {
       //
       // Release first level lock.
       //
       lockHashTableSPtr->ExitReadLock();

       //
       // Return immediately.
       //
       return UnlockStatus::Enum::UnknownResource;
    }
 }

 void LockManager::RecomputeLockGrantees(
    __in LockHashValue & lockHashValue,
    __in LockControlBlock & releasedLockControlBlock,
    __in bool isExpired)
 {
    LockHashValue::SPtr lockHashValueSPtr;
    lockHashValueSPtr = &lockHashValue;

    LockControlBlock::SPtr releasedLockControlBlockSPtr = &releasedLockControlBlock;

    auto resourceNameHash = releasedLockControlBlockSPtr->LockResourceNameHash;
    ULONG32 lockHashTableIndex = static_cast<ULONG32>(resourceNameHash % lockHashTableCount_);


    //
    // Need to find waiters that can be woken up.
    //
    KArray<LockControlBlock::SPtr> waitersWokenUpSuccess(GetThisAllocator(), lockHashTableCount_);

    //
    // Check if there is only one lock owner in the granted list.
    //
    // tood: pass min value.
    auto isSingleLockOwner = lockHashValue.ResourceControlBlock->IsSingleLockOwnerGranted(LONG_MIN);
    LockMode::Enum currentGrantedMode = lockHashValueSPtr->ResourceControlBlock->LockModeGranted;

    //
    // Validate that the granted list is consistent
    //
    if (!isExpired && lockHashValueSPtr->ResourceControlBlock->GrantedList->Count() > 0)
    {
#ifdef DBG
        for (ULONG32 i = 0; i < lockHashValueSPtr->ResourceControlBlock->GrantedList->Count(); i++)
        {
            auto grantedLockControlBlock = (*lockHashValueSPtr->ResourceControlBlock->GrantedList)[i];
            ASSERT_IFNOT(grantedLockControlBlock->GetStatus() == LockStatus::Enum::Granted, "Lock in granted list should have granted status");
        }

        if (!isSingleLockOwner)
        {
            ValidateLockResourceControlBlock(*lockHashValueSPtr->ResourceControlBlock, *releasedLockControlBlockSPtr);
        }
#endif
        if (currentGrantedMode == LockMode::Enum::Update && releasedLockControlBlockSPtr->GetLockMode() == LockMode::Enum::Update)
        {
            // If releasing an updater and granted list is not empty, then new mode is shared unless there is another same owner updater
            lockHashValueSPtr->ResourceControlBlock->LockModeGranted = LockMode::Enum::Shared;
            for (ULONG32 i = 0; i < lockHashValueSPtr->ResourceControlBlock->GrantedList->Count(); i++)
            {
                auto grantedLockControlBlock = (*lockHashValueSPtr->ResourceControlBlock->GrantedList)[i];
                if (grantedLockControlBlock->GetLockMode() == LockMode::Enum::Update)
                {
                    ASSERT_IFNOT(grantedLockControlBlock->GetOwner() == releasedLockControlBlockSPtr->GetOwner(), "All update locks should belong to the same owner");
                    lockHashValueSPtr->ResourceControlBlock->LockModeGranted = LockMode::Enum::Update;
                    break;
                }
            }
        }

        if (isSingleLockOwner)
        {
            // Upgrade the lock to the grantee with the highest lock mode
            auto modeToGrant = LockMode::Enum::Free;
            for (ULONG32 i = 0; i < lockHashValueSPtr->ResourceControlBlock->GrantedList->Count(); i++)
            {
                auto grantedLockControlBlock = (*lockHashValueSPtr->ResourceControlBlock->GrantedList)[i];
                modeToGrant = ConvertToMaxLockMode(grantedLockControlBlock->GetLockMode(), modeToGrant);
                if (modeToGrant == LockMode::Enum::Exclusive)
                {
                    // Can't go higher than exclusive, so break out early
                    break;
                }
            }

            lockHashValueSPtr->ResourceControlBlock->LockModeGranted = modeToGrant;
        }
    }
    
    if (lockHashValueSPtr->ResourceControlBlock->GrantedList->Count() == 0)
    {
        lockHashValueSPtr->ResourceControlBlock->LockModeGranted = LockMode::Enum::Free;
    }

    //
    // Go over remaining waiting queue and determine if any new waiter can be granted the lock.
    //
    for (ULONG32 index = 0; index < lockHashValueSPtr->ResourceControlBlock->WaitingQueue->Count(); index++)
    {
       auto waitingQueue = lockHashValueSPtr->ResourceControlBlock->WaitingQueue;
       LockControlBlock::SPtr lockControlBlockSPtr = (*waitingQueue)[index];

       //
       // Internal state checks.
       //
       ASSERT_IFNOT(lockControlBlockSPtr->GetStatus() == LockStatus::Enum::Pending, "Lock in waiting queue should have pending status");

       //
       // Check lock compatibility.
       //
       if (IsCompatible(lockControlBlockSPtr->GetLockMode(), lockHashValueSPtr->ResourceControlBlock->LockModeGranted) ||
          lockHashValueSPtr->ResourceControlBlock->IsSingleLockOwnerGranted(lockControlBlockSPtr->GetOwner()))
       {
          //
          // Set lock status to success.
          //
          lockControlBlockSPtr->SetStatus(LockStatus::Enum::Granted);
          lockControlBlockSPtr->SetGrantedTime(KDateTime::Now());

          //
          // Upgrade the lock mode.
          //
          lockHashValueSPtr->ResourceControlBlock->LockModeGranted = ConvertToMaxLockMode(
             lockControlBlockSPtr->GetLockMode(),
             lockHashValueSPtr->ResourceControlBlock->LockModeGranted);

          //
          // This waiter has successfully waited for the lock.
          //
          NTSTATUS status = waitersWokenUpSuccess.Append(lockControlBlockSPtr);
          KInvariant(NT_SUCCESS(status));

          status = lockHashValueSPtr->ResourceControlBlock->GrantedList->Append(lockControlBlockSPtr);
          KInvariant(NT_SUCCESS(status));

          // Since there was a waiter that has moved to granted, the total number of lock clients remains unchanged

          //
          // Stop the expiration timer for this waiter.
          //
          lockControlBlockSPtr->StopExpire();
       }
       else
       {
          //
          // Cannot unblock the rest of the waiters since at least one was found 
          // that has an incompatible lock mode.
          //
          break;
       }
    }

    //
    // Remove all granted waiters from the waiting queue and complete them.
    //
    for (ULONG32 index = 0; index < waitersWokenUpSuccess.Count(); index++)
    {
       auto completedWaiter = waitersWokenUpSuccess[index];

       auto arrayIndex = GetIndex(*lockHashValueSPtr->ResourceControlBlock->WaitingQueue, *completedWaiter);
       KInvariant(arrayIndex >= 0);
       NTSTATUS status = lockHashValueSPtr->ResourceControlBlock->WaitingQueue->Remove(arrayIndex);
       KInvariant(NT_SUCCESS(status));

       // todo
       // var awakeWaiterTask = Task.Factory.StartNew(() = > { completedWaiter.Waiter.SetResult(completedWaiter); });
       //completedWaiter->Waiter->SetResult(completedWaiter);
       bool result =  completedWaiter->Waiter->TrySetResult(completedWaiter);
       KInvariant(result);

       //completedWaiter->ClearWaiter();
    }

    waitersWokenUpSuccess.Clear();

    //
    // If there are no granted or waiting lock owner, we can clean up this lock resource lazily.
    //
    if (lockHashValueSPtr->ResourceControlBlock->HasClients() == false)
    {
       if (!status_)
       {
          auto task = ClearLocksInBackground(lockHashTableIndex);
          ASSERT_IFNOT(task.IsTaskStarted(), "Failed to start clear locks background task");
       }
    }
 }

 void LockManager::ValidateLockResourceControlBlock(
     __in LockResourceControlBlock & lockResourceControlBlock,
     __in LockControlBlock & releasedLock)
 {
     auto currentGrantedMode = lockResourceControlBlock.LockModeGranted;
     auto releaseMode = releasedLock.GetLockMode();

     if (releaseMode == LockMode::Enum::Shared && currentGrantedMode == LockMode::Enum::Shared)
     {
         // Granted list must be all readers
         for (ULONG32 i = 0; i < lockResourceControlBlock.GrantedList->Count(); i++)
         {
             auto grantee = (*lockResourceControlBlock.GrantedList)[i];
             ASSERT_IFNOT(grantee->GetLockMode() == LockMode::Enum::Shared, "Expected a shared lock");
         }
     }
     else if (releaseMode == LockMode::Enum::Shared && currentGrantedMode == LockMode::Enum::Exclusive)
     {
         ASSERT_IFNOT(false, "There must not be any shared locks if an exclusive has been granted");
     }
     else if (releaseMode == LockMode::Enum::Shared && currentGrantedMode == LockMode::Enum::Update)
     {
         // Granted list should be many readers and same owner updaters
         LONG64 updatesOwner = -1;
         for (ULONG32 i = 0; i < lockResourceControlBlock.GrantedList->Count(); i++)
         {
             auto grantee = (*lockResourceControlBlock.GrantedList)[i];
             if (grantee->GetLockMode() == LockMode::Enum::Update)
             {
                 if (updatesOwner == -1)
                 {
                     updatesOwner = grantee->GetOwner();
                 }

                 ASSERT_IFNOT(grantee->GetOwner() == updatesOwner, "All updates should be owned by the same owner");
             }

             ASSERT_IFNOT(grantee->GetLockMode() == LockMode::Enum::Shared || grantee->GetLockMode() == LockMode::Enum::Update, "Lock resource with Update granted mode has invalid granted list");
         }
     }

     else if (releaseMode == LockMode::Enum::Exclusive && currentGrantedMode == LockMode::Enum::Shared)
     {
         ASSERT_IFNOT(false, "When releasing an Exclusive lock there should not be a Shared lock");
     }
     else if (releaseMode == LockMode::Enum::Exclusive && currentGrantedMode == LockMode::Enum::Exclusive)
     {
         ASSERT_IFNOT(false, "When releasing an Exclusive lock there should not be antoher Exclusive lock");
     }
     else if (releaseMode == LockMode::Enum::Exclusive && currentGrantedMode == LockMode::Enum::Update)
     {
         ASSERT_IFNOT(false, "When releasing an Exclusive lock there should not be antoher Exclusive lock");
     }

     else if (releaseMode == LockMode::Enum::Update && currentGrantedMode == LockMode::Enum::Shared)
     {
         ASSERT_IFNOT(false, "When releasing an Update lock the current granted mode should not be Shared");
     }
     else if (releaseMode == LockMode::Enum::Update && currentGrantedMode == LockMode::Enum::Exclusive)
     {
         ASSERT_IFNOT(false, "When releasing an Update lock the current granted mode should not be Exclusive");
     }
     else if (releaseMode == LockMode::Enum::Update && currentGrantedMode == LockMode::Enum::Update)
     {
         // Must be readers and updaters, all updaters from the same owner
         LONG64 updatesOwner = -1;
         for (ULONG32 i = 0; i < lockResourceControlBlock.GrantedList->Count(); i++)
         {
             auto grantee = (*lockResourceControlBlock.GrantedList)[i];
             if (grantee->GetLockMode() == LockMode::Enum::Update)
             {
                 if (updatesOwner == -1)
                 {
                     updatesOwner = grantee->GetOwner();
                 }

                 ASSERT_IFNOT(grantee->GetOwner() == updatesOwner, "All updates should be owned by the same owner");
             }

             ASSERT_IFNOT(grantee->GetLockMode() == LockMode::Enum::Shared || grantee->GetLockMode() == LockMode::Enum::Update, "Lock resource with Update granted mode has invalid granted list");
         }
     }
 }

 LONG32 LockManager::GetIndex(
     __in KSharedArray<KSharedPtr<LockControlBlock>> const & array, 
     __in LockControlBlock const & item)
 {
     LockControlBlock::CSPtr itemSPtr = &item;
     for (ULONG32 index = 0; index < array.Count(); index++)
     {
         if (array[index].RawPtr() == itemSPtr.RawPtr())
         {
             return index;
         }
     }

     return -1;
 }

 bool LockManager::ContainsKey(
     __in KSharedArray<KSharedPtr<LockControlBlock>> const & array, 
     __in LockControlBlock const & item)
 {
     LockControlBlock::CSPtr itemSPtr = &item;
     for (ULONG32 index = 0; index < array.Count(); index++)
     {
         if (array[index].RawPtr() == itemSPtr.RawPtr())
         {
             return true;
         }
     }

     return false;
 }

 bool LockManager::ExpireLock(__in LockControlBlock& lockControlBlock)
 {
    LockManagerApiEntry();

    LockControlBlock::SPtr lockControlBlockSPtr = &lockControlBlock;
    LockHashValue::SPtr lockHashValueSPtr = nullptr;
    ULONG32 lockHashTableIndex = static_cast<ULONG32>(lockControlBlockSPtr->LockResourceNameHash % lockHashTableCount_);
    auto lockHashTableSPtr = lockHashTables_[lockHashTableIndex];

    //
    // Acquire first level lock.
    //
    lockHashTableSPtr->EnterWriteLock();

    //
    // Find the right lock resource.
    //
    if (lockHashTableSPtr->LockEntries->TryGetValue(lockControlBlockSPtr->LockResourceNameHash, lockHashValueSPtr))
    {
       //
       // Acquire second level lock.
       //
       lockHashValueSPtr->EnterWriteLock();

       //
       // Release first level lock.
       //
       lockHashTableSPtr->ExitWriteLock();

       //
       // Find the lock control block for this lock owner in the waiting list.
       //
       auto idx = GetIndex(*lockHashValueSPtr->ResourceControlBlock->WaitingQueue, *lockControlBlockSPtr);
       if (idx == -1)
       {
          //
          // Release second level lock.
          //
          lockHashValueSPtr->ExitWriteLock();

          //
          // There must have been an unlock that granted it in the meantime or it has expired already before.
          //
          return false;
       }

       //
       // Internal state checks.
       //
       KInvariant(lockControlBlockSPtr->GetStatus() == LockStatus::Enum::Pending);

       //
       // Remove lock owner from waiting list.
       //
       NTSTATUS status = lockHashValueSPtr->ResourceControlBlock->WaitingQueue->Remove(idx);
       KInvariant(NT_SUCCESS(status));

       // Lock only expired because someone else was granted, so no need to update lock hash table's empty locks count

       //
       // Clear the lock timer.
       //
       lockControlBlockSPtr->StopExpire();

       //
       // Set the correct lock status.
       //
       lockControlBlockSPtr->SetStatus(LockStatus::Enum::Timeout);
       lockControlBlockSPtr->LockCount = 0;

       //
       // Recompute the new grantees and lock granted mode.
       //
       if (idx == 0)
       {
          RecomputeLockGrantees(*lockHashValueSPtr, *lockControlBlockSPtr, true);
       }

       //
       // Release second level lock.
       //
       lockHashValueSPtr->ExitWriteLock();

       //
       // Complete the waiter.
       //
       lockControlBlockSPtr->Waiter->SetResult(lockControlBlockSPtr);
       //lockControlBlockSPtr->ClearWaiter();
       return true;
    }
    else
    {
       //
       // Release first level lock.
       //
       lockHashTableSPtr->ExitWriteLock();
    }

    return false;
 }

 ktl::Task LockManager::ClearLocksInBackground(__in ULONG32 lockHashTableIndex)
 {
     co_await CorHelper::ThreadPoolThread(this->GetThisKtlSystem().DefaultThreadPool());

     bool isOpen = status_ == true && TryAcquireServiceActivity();
     if (isOpen)
     {
         KFinally([&] { ReleaseServiceActivity(); });
         ClearLocks(lockHashTableIndex);
     }
 }

 void LockManager::ClearLocks(__in ULONG32 lockHashTableIndex)
 {
    auto lockHashTableSPtr = lockHashTables_[lockHashTableIndex];
    KArray<KeyValuePair<ULONG64, LockHashValue::SPtr>> lockHashItemsToBeCleared(GetThisAllocator());

    //
    // Acquire first level lock.
    //
    lockHashTableSPtr->EnterWriteLock();

    KSharedPtr<DictionaryEnumerator<ULONG64, LockHashValue::SPtr>> enumerator = nullptr;
    NTSTATUS status = DictionaryEnumerator<ULONG64, LockHashValue::SPtr>::Create(*lockHashTableSPtr->LockEntries, GetThisAllocator(), enumerator);
    KInvariant(NT_SUCCESS(status));
    while (enumerator->MoveNext())
    {
       KeyValuePair<ULONG64, LockHashValue::SPtr> entry = enumerator->Current();
       ULONG64 key = entry.Key;
       LockHashValue::SPtr lockHashValueSPtr = entry.Value;

       //
       // Acquire second level lock.
       //
       lockHashValueSPtr->EnterWriteLock();

       //
       // If there are no granted or waiting lock owners, we can clean up this lock resource.
       //
       bool wasCleared = false;
       if (lockHashValueSPtr->ResourceControlBlock->HasClients() == false)
       {
          KeyValuePair<ULONG64, LockHashValue::SPtr> pair(key, lockHashValueSPtr);
          status = lockHashItemsToBeCleared.Append(pair);
          KInvariant(NT_SUCCESS(status));
          wasCleared = true;
       }

       if (!wasCleared && !status_)
       {
          auto grantedList = lockHashValueSPtr->ResourceControlBlock->GrantedList;
          for (ULONG32 index = 0; index < grantedList->Count(); index++)
          {
             auto item = (*grantedList)[index];
             KInvariant(item->GetStatus() == LockStatus::Enum::Granted);
          }
       }

       //
       // Release second level lock.
       //
       lockHashValueSPtr->ExitWriteLock();
    }

    for (ULONG32 index = 0; index < lockHashItemsToBeCleared.Count(); index++)
    {
       auto itemToBeRemoved = lockHashItemsToBeCleared[index];
       
       LockHashValue::SPtr outValue = nullptr;
       lockHashTableSPtr->LockEntries->Remove(itemToBeRemoved.Key, outValue);
       lockHashTableSPtr->DecrementEmptyLocksCount();
    }

    //
    // Indicate that the next clean up task can start if needed.
    //
    lockReleasedCleanupInProgress_[lockHashTableIndex] = 0;

    //
    // Release first level lock.
    //
    lockHashTableSPtr->ExitWriteLock();

    //
    // Dispose all cleared locks.(
    //
    for (ULONG32 index = 0; index < lockHashItemsToBeCleared.Count(); index++)
    {
       auto item = lockHashItemsToBeCleared[index];
       item.Value->Close();
    }
 }


 bool LockManager::IsCompatible(
    __in LockMode::Enum modeRequested,
    __in LockMode::Enum modeGranted)
 {
    Dictionary<LockMode::Enum, LockCompatibility::Enum>::SPtr  grantedModeSPtr = nullptr;
    bool result = lockCompatibilityTableSPtr_->TryGetValue(modeGranted, grantedModeSPtr);
    KInvariant(result == true);

    LockCompatibility::Enum compatibility;
    result = grantedModeSPtr->TryGetValue(modeRequested, compatibility);
    KInvariant(result == true);

    return compatibility == LockCompatibility::Enum::NoConflict;
 }

 LockMode::Enum LockManager::ConvertToMaxLockMode(
    __in LockMode::Enum modeRequested,
    __in LockMode::Enum modeGranted)
 {
    Dictionary<LockMode::Enum, LockMode::Enum>::SPtr  grantedModeSPtr = nullptr;
    bool result = lockConversionTableSPtr_->TryGetValue(modeGranted, grantedModeSPtr);
    KInvariant(result == true);

    LockMode::Enum newMode;
    result = grantedModeSPtr->TryGetValue(modeRequested, newMode);
    return newMode;
 }  

 NTSTATUS LockManager::LoadCompatibilityMatrix()
 {
    // Free mode
    Dictionary<LockMode::Enum, LockCompatibility::Enum>::SPtr freeTableSPtr;
    NTSTATUS status = Dictionary<LockMode::Enum, LockCompatibility::Enum>::Create(32, MyHashFunction, *lockModeComparerSPtr_, GetThisAllocator(), freeTableSPtr);
    if (!NT_SUCCESS(status))
    {
       return status;
    }

    Add(*lockCompatibilityTableSPtr_, LockMode::Enum::Free, freeTableSPtr);

    Add(*freeTableSPtr, LockMode::Enum::Free, LockCompatibility::Enum::NoConflict);
    Add(*freeTableSPtr, LockMode::Enum::Shared, LockCompatibility::Enum::NoConflict);
    Add(*freeTableSPtr, LockMode::Enum::Exclusive, LockCompatibility::Enum::NoConflict);
    Add(*freeTableSPtr, LockMode::Enum::Update, LockCompatibility::Enum::NoConflict);

    // Shared mode
    Dictionary<LockMode::Enum, LockCompatibility::Enum>::SPtr sharedTableSPtr;
    status = Dictionary<LockMode::Enum, LockCompatibility::Enum>::Create(32, MyHashFunction, *lockModeComparerSPtr_, GetThisAllocator(), sharedTableSPtr);
    if (!NT_SUCCESS(status))
    {
       return status;
    }

    Add(*lockCompatibilityTableSPtr_, LockMode::Enum::Shared, sharedTableSPtr);

    Add(*sharedTableSPtr, LockMode::Enum::Free, LockCompatibility::Enum::NoConflict);
    Add(*sharedTableSPtr, LockMode::Enum::Shared, LockCompatibility::Enum::NoConflict);
    Add(*sharedTableSPtr, LockMode::Enum::Exclusive, LockCompatibility::Enum::Conflict);
    Add(*sharedTableSPtr, LockMode::Enum::Update, LockCompatibility::Enum::NoConflict);

    // Exclusive mode
    Dictionary<LockMode::Enum, LockCompatibility::Enum>::SPtr exclusiveTableSPtr;
    status = Dictionary<LockMode::Enum, LockCompatibility::Enum>::Create(32, MyHashFunction, *lockModeComparerSPtr_, GetThisAllocator(), exclusiveTableSPtr);
    if (!NT_SUCCESS(status))
    {
       return status;
    }

    Add(*lockCompatibilityTableSPtr_, LockMode::Enum::Exclusive, exclusiveTableSPtr);

    Add(*exclusiveTableSPtr, LockMode::Enum::Free, LockCompatibility::Enum::NoConflict);
    Add(*exclusiveTableSPtr, LockMode::Enum::Shared, LockCompatibility::Enum::Conflict);
    Add(*exclusiveTableSPtr, LockMode::Enum::Exclusive, LockCompatibility::Enum::Conflict);
    Add(*exclusiveTableSPtr, LockMode::Enum::Update, LockCompatibility::Enum::Conflict);

    // Update mode
    Dictionary<LockMode::Enum, LockCompatibility::Enum>::SPtr updateTableSPtr;
    status = Dictionary<LockMode::Enum, LockCompatibility::Enum>::Create(32, MyHashFunction, *lockModeComparerSPtr_, GetThisAllocator(), updateTableSPtr);
    if (!NT_SUCCESS(status))
    {
       return status;
    }

    Add(*lockCompatibilityTableSPtr_, LockMode::Enum::Update, updateTableSPtr);

    Add(*updateTableSPtr, LockMode::Enum::Free, LockCompatibility::Enum::NoConflict);
    Add(*updateTableSPtr, LockMode::Enum::Shared, LockCompatibility::Enum::Conflict);
    Add(*updateTableSPtr, LockMode::Enum::Exclusive, LockCompatibility::Enum::Conflict);
    Add(*updateTableSPtr, LockMode::Enum::Update, LockCompatibility::Enum::Conflict);

    return status;
 }

 NTSTATUS LockManager::LoadConversionMatrix()
 {
    // Free mode
    Dictionary<LockMode::Enum, LockMode::Enum>::SPtr freeTableSPtr;
    NTSTATUS status = Dictionary<LockMode::Enum, LockMode::Enum>::Create(32, MyHashFunction, *lockModeComparerSPtr_, GetThisAllocator(), freeTableSPtr);
    if (!NT_SUCCESS(status))
    {
       return status;
    }

    Add(*lockConversionTableSPtr_, LockMode::Enum::Free, freeTableSPtr);

    Add(*freeTableSPtr, LockMode::Enum::Free, LockMode::Enum::Free);
    Add(*freeTableSPtr, LockMode::Enum::Shared, LockMode::Enum::Shared);
    Add(*freeTableSPtr, LockMode::Enum::Exclusive, LockMode::Enum::Exclusive);
    Add(*freeTableSPtr, LockMode::Enum::Update, LockMode::Enum::Update);

    // Shared mode
    Dictionary<LockMode::Enum, LockMode::Enum>::SPtr sharedTableSPtr;
    status = Dictionary<LockMode::Enum, LockMode::Enum>::Create(32, MyHashFunction, *lockModeComparerSPtr_, GetThisAllocator(), sharedTableSPtr);
    if (!NT_SUCCESS(status))
    {
       return status;
    }

    Add(*lockConversionTableSPtr_,LockMode::Enum::Shared, sharedTableSPtr);

    Add(*sharedTableSPtr, LockMode::Enum::Free, LockMode::Enum::Shared);
    Add(*sharedTableSPtr, LockMode::Enum::Shared, LockMode::Enum::Shared);
    Add(*sharedTableSPtr, LockMode::Enum::Exclusive, LockMode::Enum::Exclusive);
    Add(*sharedTableSPtr, LockMode::Enum::Update, LockMode::Enum::Update);

    // Exclusive mode
    Dictionary<LockMode::Enum, LockMode::Enum>::SPtr exclusiveTableSPtr;
    status = Dictionary<LockMode::Enum, LockMode::Enum>::Create(32, MyHashFunction, *lockModeComparerSPtr_, GetThisAllocator(), exclusiveTableSPtr);
    if (!NT_SUCCESS(status))
    {
       return status;
    }

    Add(*lockConversionTableSPtr_, LockMode::Enum::Exclusive, exclusiveTableSPtr);

    Add(*exclusiveTableSPtr, LockMode::Enum::Free, LockMode::Enum::Exclusive);
    Add(*exclusiveTableSPtr, LockMode::Enum::Shared, LockMode::Enum::Exclusive);
    Add(*exclusiveTableSPtr, LockMode::Enum::Exclusive, LockMode::Enum::Exclusive);
    Add(*exclusiveTableSPtr, LockMode::Enum::Update, LockMode::Enum::Exclusive);

    // Update mode
    Dictionary<LockMode::Enum, LockMode::Enum>::SPtr updateTableSPtr;
    status = Dictionary<LockMode::Enum, LockMode::Enum>::Create(32, MyHashFunction, *lockModeComparerSPtr_, GetThisAllocator(), updateTableSPtr);
    if (!NT_SUCCESS(status))
    {
       return status;
    }

    Add(*lockConversionTableSPtr_, LockMode::Enum::Update, updateTableSPtr);

    Add(*updateTableSPtr, LockMode::Enum::Free, LockMode::Enum::Update);
    Add(*updateTableSPtr, LockMode::Enum::Shared, LockMode::Enum::Update);
    Add(*updateTableSPtr, LockMode::Enum::Exclusive, LockMode::Enum::Exclusive);
    Add(*updateTableSPtr, LockMode::Enum::Update, LockMode::Enum::Update);

    return status;
 }

 bool LockManager::IsShared(__in LockMode::Enum mode)
 {
    // todo for now there is only one shared mode.
    return mode == LockMode::Enum::Shared;
 }

 void LockManager::OnServiceOpen()
 {
     OnServiceOpenAsync();
 }

 void LockManager::OnServiceClose()
 {
     OnServiceCloseAsync();
 }

 ktl::Task LockManager::OnServiceOpenAsync() noexcept
 {
     SetDeferredCloseBehavior(); // Close must wait for all APIs to finish

     try
     {
         Open();
     }
     catch (ktl::Exception const & e)
     {
         CompleteOpen(e.GetStatus());
     }

     CompleteOpen(STATUS_SUCCESS);
     co_return;
 }

 ktl::Task LockManager::OnServiceCloseAsync() noexcept
 {
     try
     {
         Close();
     }
     catch (ktl::Exception const & e)
     {
         CompleteClose(e.GetStatus());
     }

     CompleteClose(STATUS_SUCCESS);
     co_return;
 }

 void LockManager::Open()
 {
     //
     // Create lock hash tables and drain tasks.
     //
     for (ULONG32 index = 0; index < lockHashTableCount_; index++)
     {
         LockHashTable::SPtr lockHashTableSPtr = nullptr;
         NTSTATUS status = LockHashTable::Create(GetThisAllocator(), lockHashTableSPtr);
         KInvariant(NT_SUCCESS(status));

         lockHashTables_.Append(lockHashTableSPtr);
     }

     for (ULONG32 index = 0; index < lockHashTableCount_; index++)
     {
         lockReleasedCleanupInProgress_.InsertAt(index, 0);
     }

     NTSTATUS status = ReaderWriterAsyncLock::Create(GetThisAllocator(), KTL_TAG_TEST, tableLockSPtr_);
     KInvariant(NT_SUCCESS(status));

     //
     // Set the status of this lock manager to be open for business.
     //
     status_ = true;
 }

 void LockManager::Close()
 {
     for (ULONG32 index = 0; index < lockHashTableCount_; index++)
     {
         ClearLocks(static_cast<ULONG32>(index));
     }

     auto countGranted = 0;
     for (ULONG32 index = 0; index < lockHashTables_.Count(); index++)
     {
         auto lockHashTableSPtr = lockHashTables_[index];

         //
         // Acquire first level lock.
         //
         lockHashTableSPtr->EnterWriteLock();

         //
         // Enumerate all entries.
         //
         KSharedPtr<DictionaryEnumerator<ULONG64, LockHashValue::SPtr>> enumerator = nullptr;
         NTSTATUS status = DictionaryEnumerator<ULONG64, LockHashValue::SPtr>::Create(*lockHashTableSPtr->LockEntries, GetThisAllocator(), enumerator);
         KInvariant(NT_SUCCESS(status));
         while (enumerator->MoveNext())
         {
             KeyValuePair<ULONG64, LockHashValue::SPtr> entry = enumerator->Current();
             LockHashValue::SPtr lockHashValueSPtr = entry.Value;

             //
             // Lock each entry.
             //
             lockHashValueSPtr->EnterWriteLock();

             //
             // Fail out all waiters.
             //
             for (ULONG32 lIndex = 0; lIndex < lockHashValueSPtr->ResourceControlBlock->WaitingQueue->Count(); lIndex++)
             {
                 auto queue = lockHashValueSPtr->ResourceControlBlock->WaitingQueue;
                 auto lEntry = (*queue)[lIndex];

                 //
                 // Internal state checks.
                 //
                 KInvariant(lEntry->GetStatus() == LockStatus::Enum::Pending);

                 lEntry->Waiter->SetResult(lEntry);
                 //
                 // Set the timed out lock status.
                 //
                 lEntry->SetStatus(LockStatus::Enum::Invalid);

                 //
                 // Stop the expiration timer for this waiter.
                 //
                 if (lEntry->StopExpire())
                 {
                     // todo: dispose entry
                     lEntry->Close();
                 }
             }

             //
             // Clear waiting queue.
             //
             lockHashValueSPtr->ResourceControlBlock->WaitingQueue->Clear();

             //
             // Keep track of granted lock count.
             //
             countGranted += lockHashValueSPtr->ResourceControlBlock->GrantedList->Count();

             //
             // Fail out all waiters.
             //
             for (ULONG32 wIndex = 0; wIndex < lockHashValueSPtr->ResourceControlBlock->GrantedList->Count(); wIndex++)
             {
                 auto grantedList = lockHashValueSPtr->ResourceControlBlock->GrantedList;
                 auto wEntry = (*grantedList)[wIndex];
                 wEntry->Close();
             }

             //
             // Unlock each entry.
             //
             lockHashValueSPtr->ExitWriteLock();
         }

         //
         //  Close the first level lock table.
         //
         lockHashTableSPtr->Close();

         //
         // Release first level lock.
         //
         lockHashTableSPtr->ExitWriteLock();
     }
 }

 void LockManager::Add(
    __in Dictionary<LockMode::Enum, LockMode::Enum>& dictionary,
    __in LockMode::Enum key,
    __in LockMode::Enum value)
 {
    dictionary.Add(key, value);
 }

 void LockManager::Add(
    __in Dictionary<LockMode::Enum, LockCompatibility::Enum>& dictionary,
    __in LockMode::Enum key,
    __in LockCompatibility::Enum value)
 {
    dictionary.Add(key, value);
 }

 void LockManager::Add(
    __in Dictionary<LockMode::Enum, KSharedPtr<Dictionary<LockMode::Enum, LockCompatibility::Enum>>>& dictionary,
    __in LockMode::Enum key,
    __in KSharedPtr<Dictionary<LockMode::Enum, LockCompatibility::Enum>> value)
 {
    dictionary.Add(key, value);
 }

 void LockManager::Add(
    __in Dictionary<LockMode::Enum, KSharedPtr<Dictionary<LockMode::Enum, LockMode::Enum>>>& dictionary,
    __in LockMode::Enum key,
    __in KSharedPtr<Dictionary<LockMode::Enum, LockMode::Enum>> value)
 {
    dictionary.Add(key, value);
 }
