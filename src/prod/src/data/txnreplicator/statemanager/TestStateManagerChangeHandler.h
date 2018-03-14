// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace StateManagerTests
{
    /// <summary>
    /// Collection of named operation data
    /// </summary>
    class TestStateManagerChangeHandler final
        : public KObject<TestStateManagerChangeHandler>
        , public KShared<TestStateManagerChangeHandler>
        , public TxnReplicator::IStateManagerChangeHandler
    {
        K_FORCE_SHARED(TestStateManagerChangeHandler)
        K_SHARED_INTERFACE_IMP(IStateManagerChangeHandler)

    public:
        static SPtr Create(__in KAllocator & allocator);

    public: // Test APIs.
        __declspec(property(get = get_RebuildCount)) ULONG RebuildCount;
        ULONG get_RebuildCount() const { return static_cast<ULONG>(stateProviderRebuildNotificationCount_); }

        __declspec(property(get = get_AddCount)) ULONG AddCount;
        ULONG get_AddCount() const { return static_cast<ULONG>(stateProviderAddNotificationCount_); }

        __declspec(property(get = get_RemoveCount)) ULONG RemoveCount;
        ULONG get_RemoveCount() const { return static_cast<ULONG>(stateProviderRemoveNotificationCount_); }

        void VerifyExits(
            __in KArray<KUri::CSPtr> & stateProviderList,
            __in_opt bool expectedResult = true);

        void VerifyState(
            __in Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, TxnReplicator::IStateProvider2::SPtr>> & stateManagerState);

    public: // IStateManagerChangeHandler APIs.
        void OnRebuilt(
            __in TxnReplicator::ITransactionalReplicator & source,
            __in Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, KSharedPtr<TxnReplicator::IStateProvider2>>> & stateProviders) override;

        void OnAdded(
            __in TxnReplicator::ITransactionalReplicator & source,
            __in TxnReplicator::ITransaction const & transaction,
            __in KUri const & stateProviderName,
            __in TxnReplicator::IStateProvider2 & stateProvider) override;

        void OnRemoved(
            __in TxnReplicator::ITransactionalReplicator & source,
            __in TxnReplicator::ITransaction const & transaction,
            __in KUri const & stateProviderName,
            __in TxnReplicator::IStateProvider2 & stateProvider) override;

    private:
        Data::Utilities::ConcurrentDictionary<KUri::CSPtr, TxnReplicator::IStateProvider2::SPtr>::SPtr stateProviders_;

        volatile LONG stateProviderRebuildNotificationCount_ = 0;

        volatile LONG stateProviderAddNotificationCount_ = 0;

        volatile LONG stateProviderRemoveNotificationCount_ = 0;
    };
}
