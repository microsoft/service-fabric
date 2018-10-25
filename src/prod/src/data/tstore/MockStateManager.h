// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#define MOCK_SP_TAG 'tpSM'

namespace TStoreTests
{
    class MockStateManager
        : public KObject<MockStateManager>
        , public KShared<MockStateManager>
    {
        K_FORCE_SHARED(MockStateManager)

    public:
        static SPtr Create(
            __in KAllocator & allocator,
            __in TxnReplicator::ITransactionalReplicator& replicator,
            __in Data::StateManager::IStateProvider2Factory& factory);

        TxnReplicator::Transaction::SPtr CreateTransaction();

        ktl::Awaitable<void> CommitAddAsync(__in TxnReplicator::Transaction& txn);
        ktl::Awaitable<void> CommitRemoveAsync(__in TxnReplicator::Transaction& txn);

        ktl::Awaitable<void> AddAsync(
            __in TxnReplicator::Transaction & transaction,
            __in KUriView const & stateProviderName,
            __in KStringView const & stateProvider,
            __in_opt Data::Utilities::OperationData const * const initializationParameters,
            __in Common::TimeSpan const & timeout,
            __in ktl::CancellationToken const & cancellationToken);

        ktl::Awaitable<void> RemoveAsync(
            __in TxnReplicator::Transaction & transaction,
            __in KUriView const & stateProviderName,
            __in Common::TimeSpan const & timeout,
            __in ktl::CancellationToken const & cancellationToken);

        ktl::Awaitable<void> CloseAsync();

    private:

        KString::CSPtr CreateFileString(__in KStringView const & name);

        MockStateManager(
            __in TxnReplicator::ITransactionalReplicator& replicator,
            __in Data::StateManager::IStateProvider2Factory& factory);

        KUriView stateProviderName_ = L"fabric:/TemporaryName";
        Data::Utilities::OperationData const * initializationParameters;
        TxnReplicator::IStateProvider2::SPtr stateProviderSPtr_;
        Data::StateManager::IStateProvider2Factory::SPtr spFactorySPtr_;
        TxnReplicator::ITransactionalReplicator::SPtr replicatorSPtr_;
    };
}
