//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------
#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "ReliableConcurrentQueue.TestBase.h"

namespace ReliableConcurrentQueueTests
{
    using namespace ktl;
    using namespace Data;
    using namespace TxnReplicator;

    class RCQStateProviderTest
    {
      public:
        RCQStateProviderTest()
        {
            NTSTATUS status;
            status = KtlSystem::Initialize(FALSE, &ktlSystem_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            ktlSystem_->SetStrictAllocationChecks(TRUE);
        }

        ~RCQStateProviderTest()
        {
            ktlSystem_->Shutdown();
        }

        PartitionedReplicaId::SPtr GetPartitionedReplicaId()
        {
            KAllocator &allocator = GetAllocator();
            KGuid guid;
            guid.CreateNew();
            ::FABRIC_REPLICA_ID replicaId = 1;
            return PartitionedReplicaId::Create(guid, replicaId, allocator);
        }

        KSharedPtr<MockTransactionalReplicator> CreateMockReplicator(FABRIC_REPLICA_ROLE role) //TODO: IStatefulServicePartition& partition
        {
            KAllocator &allocator = GetAllocator();
            MockTransactionalReplicator::SPtr replicator = nullptr;
            NTSTATUS status = MockTransactionalReplicator::Create(allocator, /*hasPersistedState:*/true, replicator);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            replicator->Initialize(*GetPartitionedReplicaId());
            replicator->Role = role;
            return replicator;
        }

        void AddRCQ(__in MockStateManager &stateManager)
        {
            auto txn = stateManager.CreateTransaction();
            StoreInitializationParameters::SPtr paramSPtr = nullptr;
            NTSTATUS status = StoreInitializationParameters::Create(GetAllocator(), StoreBehavior::SingleVersion, true, paramSPtr);
            Diagnostics::Validate(status);

            Data::Utilities::OperationData::SPtr initParamData = paramSPtr->ToOperationData(GetAllocator()).RawPtr();
            SyncAwait(stateManager.AddAsync(*txn, L"fabric:/myRCQ1", L"RCQ", initParamData.RawPtr(), Common::TimeSpan::MaxValue, CancellationToken::None));
            SyncAwait(stateManager.CommitAddAsync(*txn));
        }

        void RemoveRCQ(__in MockStateManager &stateManager)
        {
            auto txn = stateManager.CreateTransaction();
            SyncAwait(stateManager.RemoveAsync(*txn, L"fabric:/myRCQ1", Common::TimeSpan::MaxValue, CancellationToken::None));
            SyncAwait(stateManager.CommitRemoveAsync(*txn));
        }

        MockStateManager::SPtr CreateStateManager(
            __in RCQStateProviderFactory::SPtr factorySPtr,
            __in TxnReplicator::ITransactionalReplicator &replicator)
        {
            MockStateManager::SPtr stateManagerSPtr = MockStateManager::Create(GetAllocator(), replicator, *factorySPtr);
            return stateManagerSPtr;
        }

        Common::CommonConfig config; // load the config object as its needed for the tracing to work

        KAllocator &GetAllocator()
        {
            return ktlSystem_->NonPagedAllocator();
        }

      private:
        KtlSystem *ktlSystem_;
    };

    BOOST_FIXTURE_TEST_SUITE(RCQStateProviderTestSuite, RCQStateProviderTest)

    BOOST_AUTO_TEST_CASE(AddRCQ_BufferVal_ShouldSucceed)
    {
        auto replicatorSPtr = CreateMockReplicator(FABRIC_REPLICA_ROLE_PRIMARY);
        RCQStateProviderFactory::SPtr factorySPtr = RCQStateProviderFactory::CreateBufferFactory(GetAllocator());
        MockStateManager::SPtr stateManagerSPtr = CreateStateManager(factorySPtr, *replicatorSPtr);
        AddRCQ(*stateManagerSPtr);
        SyncAwait(stateManagerSPtr->CloseAsync());
    }

    BOOST_AUTO_TEST_SUITE_END()
}
