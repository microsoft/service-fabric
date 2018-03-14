// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace TStore
    {
        class ILock;
        class LockManager : public KObject<LockManager>, public KShared<LockManager>
        {
            K_FORCE_SHARED(LockManager);

        public:
            static NTSTATUS
                Create(
                    __in KAllocator& allocator,
                    __out LockManager::SPtr& result);

            void Open();

            ktl::Awaitable<void> AcquirePrimeLockAsync(
                __in LockMode::Enum lockMode,
                __in Common::TimeSpan timeout
            /*, ktl::CancellationToken*/);

            void ReleasePrimeLock(
                __in LockMode::Enum lockMode);

            ktl::Awaitable<KSharedPtr<LockControlBlock>> AcquireLockAsync(
                __in LONG64 owner,
                __in ULONG64 resourceNameHash,
                __in LockMode::Enum mode,
                __in Common::TimeSpan timeout);

            UnlockStatus::Enum ReleaseLock(__in LockControlBlock& acquiredLock);

            bool ExpireLock(__in LockControlBlock& lockControlBlockSPtr);

            bool IsShared(__in LockMode::Enum mode);

            void Close();

            __declspec(property(get = get_IsOpen)) bool IsOpen;
            bool get_IsOpen()
            {
                return status_;
            }

        private:
           static void Add(
              __in Dictionary<LockMode::Enum, LockMode::Enum>& dictionary,
              __in LockMode::Enum key,
              __in LockMode::Enum value);

           static void Add(
              __in Dictionary<LockMode::Enum, LockCompatibility::Enum>& dictionary,
              __in LockMode::Enum key,
              __in LockCompatibility::Enum value);

           static void Add(
              __in Dictionary<LockMode::Enum, KSharedPtr<Dictionary<LockMode::Enum, LockCompatibility::Enum>>>& dictionary,
              __in LockMode::Enum key,
              __in KSharedPtr<Dictionary<LockMode::Enum, LockCompatibility::Enum>> value);

           static void Add(
              __in Dictionary<LockMode::Enum, KSharedPtr<Dictionary<LockMode::Enum, LockMode::Enum>>>& dictionary,
              __in LockMode::Enum key,
              __in KSharedPtr<Dictionary<LockMode::Enum, LockMode::Enum>> value);

           bool IsCompatible(
                __in LockMode::Enum modeRequested,
                __in LockMode::Enum modeGranted);

            LockMode::Enum ConvertToMaxLockMode(
                __in LockMode::Enum modeRequested,
                __in LockMode::Enum modeGranted);

            void RecomputeLockGrantees(
                __in LockHashValue& lockHashValue,
                __in LockControlBlock & releasedLockControlBlock,
                __in bool isExpired);

            void ClearLocks(__in ULONG32 lockHashTableIndex);

            void ValidateLockResourceControlBlock(
                __in LockResourceControlBlock & lockResourceControlBlock,
                __in LockControlBlock & releasedLock);

            NTSTATUS LoadCompatibilityMatrix();

            NTSTATUS LoadConversionMatrix();

            // todo: Make it a multiple of processor count.
            ULONG32 lockHashTableCount_ = 16;
            ReaderWriterAsyncLock::SPtr tableLockSPtr_;
            KArray<LONG32> lockReleasedCleanupInProgress_;
            KArray<LockHashTable::SPtr> lockHashTables_;
            bool status_;
            KSharedPtr<IComparer<LockMode::Enum>> lockModeComparerSPtr_;

            //
            // Minimum numbers of entries in the hash table whose locks can be cleared.
            //
            ULONG32 clearLocksThreshold_ = 128;

            Dictionary<LockMode::Enum, KSharedPtr<Dictionary<LockMode::Enum, LockCompatibility::Enum>>>::SPtr lockCompatibilityTableSPtr_;

            Dictionary<LockMode::Enum, KSharedPtr<Dictionary<LockMode::Enum, LockMode::Enum>>>::SPtr lockConversionTableSPtr_;
        };
    }
}
