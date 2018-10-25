// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        class VersionManager
            : public TxnReplicator::IInternalVersionManager
            , public KObject<VersionManager>
            , public KShared<VersionManager>
        {
            K_FORCE_SHARED(VersionManager)
            K_SHARED_INTERFACE_IMP(IInternalVersionManager)
            K_SHARED_INTERFACE_IMP(ITransactionVersionManager)
            K_SHARED_INTERFACE_IMP(ILoggingReplicatorToVersionManager)
            K_SHARED_INTERFACE_IMP(IVersionManager)

        public:
            static SPtr Create(__in KAllocator & allocator);

            void Initialize(__in TxnReplicator::IVersionProvider & versionProvider);

            void Reuse();

            void UpdateDispatchingBarrierTask(__in TxnReplicator::CompletionTask & barrierTask) override;

            ktl::Awaitable<NTSTATUS> TryRemoveCheckpointAsync(
                __in FABRIC_SEQUENCE_NUMBER checkpointLsnToBeRemoved,
                __in FABRIC_SEQUENCE_NUMBER nextCheckpointLsn) noexcept override;

            NTSTATUS TryRemoveVersion(
                __in LONG64 stateProviderId,
                __in FABRIC_SEQUENCE_NUMBER commitLsn,
                __in FABRIC_SEQUENCE_NUMBER nextCommitLsn,
                __out TxnReplicator::TryRemoveVersionResult::SPtr & result) noexcept override;

            ktl::Awaitable<NTSTATUS> RegisterAsync(__out FABRIC_SEQUENCE_NUMBER & vsn) noexcept override;

            NTSTATUS UnRegister(__in FABRIC_SEQUENCE_NUMBER visibilityVersionNumber) noexcept override;

        public: // Test only methods.
            bool Test_VerifyState(
                __in LONG32 versionCount,
                __in LONG32 notificationCount,
                __in LONG32 registeredNotificationsCount);

        private: // Private methods.
            bool CanVersionBeRemovedCallerHoldsLock(
                __in LONG64 toBeRemovedLSN,
                __in LONG64 newPreviousLastCommittedLSN,
                __out LONG32 & indexOfEqualOrImmediatelyHigher);

            Utilities::KHashSet<LONG64>::SPtr FindAllVisibilitySequenceNumbersKeepingVersionAliveCallerHoldsLock(
                __in LONG32 startIndex,
                __in LONG64 newPreviousLastCommittedLSN);

            ktl::Task ProcessNotificationTask(
                __in ktl::AwaitableCompletionSource<LONG64> & acs,
                __in NotificationKey & key);

        private: // Private variables
            const ULONG DefaultHashSetSize = 997;
            const ULONG ExpectedNumberOfInflightNotifications = 997;
            const ULONG ExpectedNumberOfInflightVisibilitySequenceNumbers = 31;

            KHashTable<LONG64, ktl::AwaitableCompletionSource<LONG64>::SPtr> readerRemovalNotifications_;
            KArray<FABRIC_SEQUENCE_NUMBER> versionList_;
            KReaderWriterSpinLock versionListLock_;
            Utilities::ThreadSafeSPtrCache<TxnReplicator::CompletionTask> lastDispatchingBarrier_;

            Utilities::ConcurrentDictionary<NotificationKey::SPtr, byte>::SPtr registeredNotifications_;

            KWeakRef<TxnReplicator::IVersionProvider>::SPtr versionProviderSPtr_ = nullptr;
        };
    }
}
