// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LoggingReplicator;
using namespace TxnReplicator;
using namespace Data::Utilities;

inline LONG32 Compare(LONG64 const & one, LONG64 const & two)
{
    if (one == two)
    {
        return 0;
    }

    if (one > two)
    {
        return 1;
    }

    return -1;
}

inline ULONG Hash(__in EnumerationCompletionResult::SPtr const & enumerationCompletionResult)
{
    return enumerationCompletionResult->GetHashCode();
}

inline BOOLEAN Equals(__in EnumerationCompletionResult::SPtr const & one, __in EnumerationCompletionResult::SPtr const & two)
{
    if (one == nullptr && two == nullptr)
    {
        return TRUE;
    }

    if ((one == nullptr && two != nullptr) || (one != nullptr && two == nullptr))
    {
        return FALSE;
    }

    return *one == *two;
}

VersionManager::VersionManager()
    : IInternalVersionManager()
    , KObject()
    , KShared()
    , readerRemovalNotifications_(ExpectedNumberOfInflightNotifications, K_DefaultHashFunction, GetThisAllocator())
    , versionList_(GetThisAllocator(), ExpectedNumberOfInflightVisibilitySequenceNumbers)
    , versionListLock_()
    , lastDispatchingBarrier_(nullptr)
{
    if (NT_SUCCESS(readerRemovalNotifications_.Status()) == false)
    {
        SetConstructorStatus(readerRemovalNotifications_.Status());
        return;
    }

    if (NT_SUCCESS(versionList_.Status()) == false)
    {
        SetConstructorStatus(versionList_.Status());
        return;
    }

    NotificationKeyComparer::SPtr comparerSPtr(nullptr);
    NTSTATUS status = NotificationKeyComparer::Create(GetThisAllocator(), comparerSPtr);
    if (NT_SUCCESS(status) == false)
    {
        SetConstructorStatus(status);
        return;
    }
    
    ULONG consistencyLevel = ConcurrentDictionary<NotificationKey::SPtr, byte>::DefaultConcurrencyLevel;
    ULONG intialCapacity = ConcurrentDictionary<NotificationKey::SPtr, byte>::DefaultInitialCapacity;
    status = ConcurrentDictionary<NotificationKey::SPtr, byte>::Create(
        consistencyLevel, 
        intialCapacity, 
        NotificationKey::GetHashCode, 
        comparerSPtr.RawPtr(),
        GetThisAllocator(), 
        registeredNotifications_);
    if (NT_SUCCESS(status) == false)
    {
        SetConstructorStatus(status);
        return;
    }

    SetConstructorStatus(status);
}

VersionManager::~VersionManager()
{
}

VersionManager::SPtr VersionManager::Create(__in KAllocator & allocator)
{
    VersionManager * pointer = _new(VERSION_MANAGER_TAG, allocator) VersionManager();
   
    THROW_ON_ALLOCATION_FAILURE(pointer);
    THROW_ON_FAILURE(pointer->Status())
    return SPtr(pointer);
}

void VersionManager::Initialize(__in IVersionProvider & versionProvider)
{
    NTSTATUS status = versionProvider.GetWeakIfRef(versionProviderSPtr_);
    THROW_ON_FAILURE(status);
}

void VersionManager::Reuse()
{
    versionListLock_.AcquireExclusive();
    {
        KFinally([&] {versionListLock_.ReleaseExclusive(); });

        readerRemovalNotifications_.Clear();
        versionList_.Clear();
    }

    registeredNotifications_->Clear();
    lastDispatchingBarrier_.Put(nullptr);
}

void VersionManager::UpdateDispatchingBarrierTask(__in CompletionTask & barrierTask)
{
    lastDispatchingBarrier_.Put(&barrierTask);
}

Awaitable<NTSTATUS> VersionManager::TryRemoveCheckpointAsync(
    __in LONG64 checkpointLSNToBeRemoved,
    __in LONG64 nextCheckpointLSN) noexcept
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_SUCCESS;

    ASSERT_IFNOT(
        nextCheckpointLSN > (checkpointLSNToBeRemoved + 1), 
        "Checkpoint is preceded by Barrier Record. Hence numbers should at least differ by 2. NextCheckpointLSN: {0} CheckpointLSNToBeRemoved: {1}", 
        checkpointLSNToBeRemoved, 
        nextCheckpointLSN);

    KArray<Awaitable<LONG64>> taskList(GetThisAllocator());
    THROW_ON_FAILURE(taskList.Status());

    versionListLock_.AcquireShared();
    {
        KFinally([&] {versionListLock_.ReleaseShared(); });

        LONG32 indexVisibilitySequenceHigherThanVersionToBeRemoved;
        bool canBeRemoved = CanVersionBeRemovedCallerHoldsLock(
            checkpointLSNToBeRemoved,
            nextCheckpointLSN,
            indexVisibilitySequenceHigherThanVersionToBeRemoved);

        if (canBeRemoved == true)
        {
            co_await suspend_never{};
            co_return status;
        }

        KHashSet<LONG64>::SPtr versionsKeepingItemAlive = FindAllVisibilitySequenceNumbersKeepingVersionAliveCallerHoldsLock(
            indexVisibilitySequenceHigherThanVersionToBeRemoved,
            nextCheckpointLSN);

        IEnumerator<LONG64>::SPtr enumerator = versionsKeepingItemAlive->GetEnumerator();
        while (enumerator->MoveNext())
        {
            LONG64 visibility = enumerator->Current();
            AwaitableCompletionSource<LONG64>::SPtr acs = nullptr;
            status = readerRemovalNotifications_.Get(visibility, acs);
            ASSERT_IFNOT(NT_SUCCESS(status), "Reader removal notification must already exist for VSN: {0}", visibility);
            ASSERT_IFNOT(acs != nullptr, "Removed acs must not be nullptr for VSN: {0}", visibility);

            taskList.Append(Ktl::Move(acs->GetAwaitable()));
        }
    }

    co_await TaskUtilities<LONG64>::WhenAll(taskList);
    co_return status;
}

NTSTATUS VersionManager::TryRemoveVersion(
    __in LONG64 stateProviderId,
    __in LONG64 commitLSN,
    __in LONG64 nextCommitLSN,
    __out TryRemoveVersionResult::SPtr & result) noexcept
{
    result = nullptr;

    KShared$ApiEntry();

    NTSTATUS status = STATUS_SUCCESS;

    ASSERT_IFNOT(nextCommitLSN > commitLSN, "Next Commit LSN {0} > Commit LSN {1}", nextCommitLSN, commitLSN);

    KHashSet<LONG64>::SPtr versionsKeepingItemAlive = nullptr;
    KHashSet<EnumerationCompletionResult::SPtr>::SPtr enumerationCompletionResultSet = nullptr;

    versionListLock_.AcquireShared();
    {
        KFinally([&] {versionListLock_.ReleaseShared(); });

        LONG32 indexVisibilityLSNHigherThanVersionToBeRemoved;
        bool canBeRemoved = CanVersionBeRemovedCallerHoldsLock(
            commitLSN,
            nextCommitLSN,
            indexVisibilityLSNHigherThanVersionToBeRemoved);

        if (canBeRemoved == true)
        {
            status = TryRemoveVersionResult::Create(canBeRemoved, GetThisAllocator(), result);
            return status;
        }

        versionsKeepingItemAlive = FindAllVisibilitySequenceNumbersKeepingVersionAliveCallerHoldsLock(
            indexVisibilityLSNHigherThanVersionToBeRemoved,
            nextCommitLSN);

        IEnumerator<LONG64>::SPtr enumerator = versionsKeepingItemAlive->GetEnumerator();
        while (enumerator->MoveNext())
        {
            LONG64 visibility = enumerator->Current();
            NotificationKey::SPtr notificationKeySPtr;
            status = NotificationKey::Create(stateProviderId, visibility, GetThisAllocator(), notificationKeySPtr);

            RETURN_ON_FAILURE(status);

            bool isAdded = registeredNotifications_->TryAdd(notificationKeySPtr, BYTE_MAX);
            if (isAdded == false)
            {
                continue;
            }

            if (enumerationCompletionResultSet == nullptr)
            {
                status = KHashSet<EnumerationCompletionResult::SPtr>::Create(
                    DefaultHashSetSize,
                    Hash,
                    Equals,
                    GetThisAllocator(), enumerationCompletionResultSet);

                RETURN_ON_FAILURE(status);
            }

            AwaitableCompletionSource<LONG64>::SPtr acs = nullptr;
            status = readerRemovalNotifications_.Get(visibility, acs);
            ASSERT_IFNOT(NT_SUCCESS(status), "Reader removal notification must already exist for VSN: {0}", visibility);
            ASSERT_IFNOT(acs != nullptr, "Removed acs must not be nullptr for VSN: {0}", visibility);

            ProcessNotificationTask(*acs, *notificationKeySPtr);

            EnumerationCompletionResult::SPtr enumerationCompletionResult = nullptr;
            status = EnumerationCompletionResult::Create(visibility, *acs, GetThisAllocator(), enumerationCompletionResult);

            RETURN_ON_FAILURE(status);

            enumerationCompletionResultSet->TryAdd(enumerationCompletionResult);
        }
    }

    status = TryRemoveVersionResult::Create(
        false,
        versionsKeepingItemAlive.RawPtr(),
        enumerationCompletionResultSet.RawPtr(),
        GetThisAllocator(),
        result);

    return status;
}

Awaitable<NTSTATUS> VersionManager::RegisterAsync(__out FABRIC_SEQUENCE_NUMBER & vsn) noexcept
{
    KShared$ApiEntry();

    vsn = FABRIC_INVALID_SEQUENCE_NUMBER;
    NTSTATUS status = STATUS_SUCCESS;
    CompletionTask::SPtr snapLastDispatchingBarrier = lastDispatchingBarrier_.Get();

    if (snapLastDispatchingBarrier != nullptr)
    {
        co_await snapLastDispatchingBarrier->AwaitCompletion();
    }

    IVersionProvider::SPtr loggingReplicator = this->versionProviderSPtr_->TryGetTarget();
    ASSERT_IFNOT(loggingReplicator != nullptr, "LoggingReplicator could not have closed.");

    versionListLock_.AcquireExclusive();
    {
        KFinally([&] {versionListLock_.ReleaseExclusive(); });

        status = loggingReplicator->GetVersion(vsn);

        CO_RETURN_ON_FAILURE(status);

        ULONG currentSnapshotCount = versionList_.Count();
        ASSERT_IFNOT(
            currentSnapshotCount == 0 || vsn >= versionList_[currentSnapshotCount - 1],
            "Barriers must come in sorted order.");

        versionList_.Append(vsn);
        bool contains = readerRemovalNotifications_.ContainsKey(vsn);
        if (contains == false)
        {
            AwaitableCompletionSource<LONG64>::SPtr acs;
            status = AwaitableCompletionSource<LONG64>::Create(GetThisAllocator(), VERSION_MANAGER_TAG, acs);

            CO_RETURN_ON_FAILURE(status);

            readerRemovalNotifications_.Put(vsn, acs);
        }
    }

    co_return status;
}

NTSTATUS VersionManager::UnRegister(__in FABRIC_SEQUENCE_NUMBER visibilityVersionNumber) noexcept
{
    AwaitableCompletionSource<LONG64>::SPtr notification = nullptr;

    versionListLock_.AcquireExclusive();
    {
        KFinally([&] {versionListLock_.ReleaseExclusive(); });

        LONG32 index = Sort<LONG64>::BinarySearch(visibilityVersionNumber, true, Compare, versionList_);
        ASSERT_IFNOT(
            index >= 0,
            "An item that is not registered cannot be unregistered");

        LONG32 versionCount = static_cast<LONG32>(versionList_.Count());

        bool leftSideMatch = (index > 0) && (versionList_[index - 1] == visibilityVersionNumber);
        bool rightSideMatch = (index < (versionCount - 1)) && (versionList_[index + 1] == visibilityVersionNumber);

        if (leftSideMatch == false && rightSideMatch == false)
        {
            readerRemovalNotifications_.Remove(visibilityVersionNumber, &notification);
        }

        versionList_.Remove(index);
    }

    // Making sure that Tasks continuation runs outside the lock.
    if (notification != nullptr)
    {
        notification->SetResult(visibilityVersionNumber);
    }

    return STATUS_SUCCESS;
}

bool VersionManager::Test_VerifyState(
    __in LONG32 versionCount,
    __in LONG32 notificationCount,
    __in LONG32 registeredNotificationsCount)
{
    LONG32 tmpCount = static_cast<LONG32>(versionList_.Count());

    if (tmpCount != versionCount)
    {
        return false;
    }

    tmpCount = static_cast<LONG32>(readerRemovalNotifications_.Count());
    if (tmpCount != notificationCount)
    {
        return false;
    }

    tmpCount = static_cast<LONG32>(readerRemovalNotifications_.Count());
    return tmpCount == registeredNotificationsCount;
}

bool VersionManager::CanVersionBeRemovedCallerHoldsLock(
    __in LONG64 toBeRemovedLSN,
    __in LONG64 newPreviousLastCommittedLSN,
    __out LONG32 & indexOfEqualOrImmediatelyHigher)
{
    indexOfEqualOrImmediatelyHigher = LONG_MIN;

    // List maintains _size
    if (versionList_.Count() == 0)
    {
        // It is removable.
        return true;
    }

    LONG32 index = Sort<LONG64>::BinarySearch(toBeRemovedLSN, true, Compare, versionList_);

    // version list could have perfect match due to isBarrier records like InformationLogRecord 
    // that is not preceded by a Barrier log record.
    indexOfEqualOrImmediatelyHigher = index < 0 ? ~index : index;

    LONG32 versionCount = static_cast<LONG32>(versionList_.Count());

    ASSERT_IFNOT(
        indexOfEqualOrImmediatelyHigher <= versionCount,
        "indexOfEqualOrImmediatelyHigher {0} <=  versionList_.Count() {1}",
        indexOfEqualOrImmediatelyHigher,
        versionCount);

    // if all versions are higher than versionLsnToBeRemoved
    if (indexOfEqualOrImmediatelyHigher == versionCount)
    {
        // It is removable.
        return true;
    }

    if (newPreviousLastCommittedLSN <= versionList_[indexOfEqualOrImmediatelyHigher])
    {
        // It is removable.
        return true;
    }

    return false;
}

KHashSet<LONG64>::SPtr VersionManager::FindAllVisibilitySequenceNumbersKeepingVersionAliveCallerHoldsLock(
    __in LONG32 startIndex,
    __in LONG64 newPreviousLastCommittedLSN)
{
    KHashSet<LONG64>::SPtr result = nullptr;
    NTSTATUS status = KHashSet<LONG64>::Create(DefaultHashSetSize, K_DefaultHashFunction, GetThisAllocator(), result);
    THROW_ON_FAILURE(status);

    LONG32 versionCount = versionList_.Count();

    for (LONG32 index = startIndex; index < versionCount; index++)
    {
        LONG64 visibilitySequenceNumber = versionList_[index];
        if (visibilitySequenceNumber > newPreviousLastCommittedLSN)
        {
            break;
        }

        // Idempotent add.
        result->TryAdd(visibilitySequenceNumber);
    }

    return result;
}

Task VersionManager::ProcessNotificationTask(
    __in AwaitableCompletionSource<LONG64> & acs,
    __in NotificationKey & key)
{
    KShared$ApiEntry();

    // Snap the inputs before we await the snapshot release since we need to keep them alive.
    NotificationKey::SPtr tmpKey(&key);
    AwaitableCompletionSource<LONG64>::SPtr tmpACS(&acs);

    co_await tmpACS->GetAwaitable();

    byte outputByte;
    bool isRemoved = registeredNotifications_->TryRemove(tmpKey, outputByte);
    ASSERT_IFNOT(
        isRemoved,
        "Only one can register and only one can unregister. VSN: {0}",
        key.VisibilitySeqeuenceNumber);
    ASSERT_IFNOT(
        outputByte == BYTE_MAX,
        "Byte is always set to MaxValue. VSN: {0} Byte: {1}",
        key.VisibilitySeqeuenceNumber,
        outputByte);

    co_return;
}
