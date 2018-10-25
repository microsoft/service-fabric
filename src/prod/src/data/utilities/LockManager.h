// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Utilities
    {
#define LockManagerApiEntry() \
        if (!this->TryAcquireServiceActivity()) \
        { \
        throw ktl:: Exception(SF_STATUS_OBJECT_CLOSED); \
        } \
        KFinally([&] \
        { \
        this->ReleaseServiceActivity(); \
        })

        class ILock;
        class LockManager : public KAsyncServiceBase
        {
            K_FORCE_SHARED(LockManager);

        public:
            static NTSTATUS
                Create(
                    __in KAllocator& allocator,
                    __out LockManager::SPtr& result);

            ktl::Awaitable<void> OpenAsync() noexcept;
            ktl::Awaitable<void> CloseAsync() noexcept;

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

        protected:
            void OnServiceOpen() override;
            void OnServiceClose() override;

        private:
            ktl::Task OnServiceOpenAsync() noexcept;
            ktl::Task OnServiceCloseAsync() noexcept;

            __declspec(property(get = get_IsOpen)) bool IsOpen;
            bool get_IsOpen()
            {
                return status_;
            }

        private:
            void Open();
            void Close();

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
            ktl::Task ClearLocksInBackground(__in ULONG32 lockHashTableIndex);

            void ValidateLockResourceControlBlock(
                __in LockResourceControlBlock & lockResourceControlBlock,
                __in LockControlBlock & releasedLock);

            NTSTATUS LoadCompatibilityMatrix();

            NTSTATUS LoadConversionMatrix();

            LONG32 GetIndex(
                __in KSharedArray<KSharedPtr<LockControlBlock>> const & array, 
                __in LockControlBlock const & item);

            bool ContainsKey(
                __in KSharedArray<KSharedPtr<LockControlBlock>> const & array, 
                __in LockControlBlock const & item);

            // todo: Make it a multiple of processor count.
            ULONG32 lockHashTableCount_ = 12;
            ReaderWriterAsyncLock::SPtr tableLockSPtr_;
            KArray<LONG64> lockReleasedCleanupInProgress_;
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
