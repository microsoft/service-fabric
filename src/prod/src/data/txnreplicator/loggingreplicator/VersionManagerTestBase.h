// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LoggingReplicatorTests
{
    class VersionManagerTestBase
    {
    protected:
        __declspec(property(get = get_BeforeUnRegisterFlag)) LONG64 BeforeUnRegisterFlag;
        LONG64 get_BeforeUnRegisterFlag() const;

        __declspec(property(get = get_AfterUnRegisterFlag)) LONG64 AfterUnRegisterFlag;
        LONG64 get_AfterUnRegisterFlag() const;

    protected:
        TxnReplicator::TryRemoveVersionResult::SPtr CreateExpectedTryRemoveResult(
            __in std::vector<LONG64> & set,
            __in std::vector<LONG64> & tasks,
            __in KAllocator & allocator);

        ktl::Awaitable<void> RegisterMultipleSnapshotsAsync(
            __in Data::LoggingReplicator::VersionManager & versionManager,
            __in std::vector<LONG64> & barrierResponses);

        void UnRegisterMultipleSnapshots(
            __in Data::LoggingReplicator::VersionManager & versionManager,
            __in std::vector<LONG64> & barrierResponses,
            __in_opt bool withFlag = false);

        TxnReplicator::TryRemoveVersionResult::SPtr TestTryRemoveVersion(
            __in Data::LoggingReplicator::VersionManager & versionManager,
            __in LONG64 stateProviderId,
            __in LONG64 version,
            __in LONG64 versionNext,
            __in TxnReplicator::TryRemoveVersionResult & expectedResult);

        void VerifyTryRemoveResult(
            __in TxnReplicator::TryRemoveVersionResult & inputResult, 
            __in TxnReplicator::TryRemoveVersionResult & expectedResult);

        bool ContainsNotification(
            __in Data::Utilities::KHashSet<TxnReplicator::EnumerationCompletionResult::SPtr> const & set,
            __in LONG64 enumeration);

        void VerifyState(
            __in Data::LoggingReplicator::VersionManager & versionManager,
            __in LONG32 retryCount,
            __in LONG32 versionCount,
            __in LONG32 notificationCount,
            __in LONG32 registeredNotificationsCount);

        KAllocator& GetAllocator();
        LONG64 GetRandomLONG64();

    protected:
        VersionManagerTestBase();
        virtual ~VersionManagerTestBase();

    private:
        static ULONG VersionManagerTestBase::HashFunction(
            __in const TxnReplicator::EnumerationCompletionResult::SPtr & Key);

        static BOOLEAN Equals(
            __in TxnReplicator::EnumerationCompletionResult::SPtr const & one,
            __in TxnReplicator::EnumerationCompletionResult::SPtr const & two);

    protected:
        ULONG DefaultBackoff = 128;
        ULONG DefaultRetryCount = 8;

    private:
        KtlSystem* underlyingSystemPtr_ = nullptr;
        Common::Random random_;

        LONG64 beforeUnRegisterFlag_;
        LONG64 afterUnRegisterFlag_;
    };
}
