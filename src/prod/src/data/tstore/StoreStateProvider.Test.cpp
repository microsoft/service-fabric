// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

namespace TStoreTests
{
using namespace ktl;
using namespace Data;
using namespace TxnReplicator;

class StoreStateProviderTest
{
  public:
    StoreStateProviderTest()
    {
        NTSTATUS status;
        status = KtlSystem::Initialize(FALSE, &ktlSystem_);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        ktlSystem_->SetStrictAllocationChecks(TRUE);
    }

    ~StoreStateProviderTest()
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
        NTSTATUS status = MockTransactionalReplicator::Create(allocator, /*hasPersistedState: */true, replicator);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        replicator->Initialize(*GetPartitionedReplicaId());
        replicator->Role = role;
        return replicator;
    }

    ktl::Awaitable<void> AddStoreAsync(__in MockStateManager &stateManager)
    {
        auto txn = stateManager.CreateTransaction();
        StoreInitializationParameters::SPtr paramSPtr = nullptr;
        NTSTATUS status = StoreInitializationParameters::Create(GetAllocator(), StoreBehavior::SingleVersion, true, paramSPtr);
        Diagnostics::Validate(status);

        Data::Utilities::OperationData::SPtr initParamData = paramSPtr->ToOperationData(GetAllocator()).RawPtr();
        co_await stateManager.AddAsync(*txn, L"fabric:/myTStore1", L"TStore", initParamData.RawPtr(), Common::TimeSpan::MaxValue, CancellationToken::None);
        co_await stateManager.CommitAddAsync(*txn);
        co_return;
    }

    ktl::Awaitable<void> RemoveStoreAsync(__in MockStateManager &stateManager)
    {
        auto txn = stateManager.CreateTransaction();
        co_await stateManager.RemoveAsync(*txn, L"fabric:/myTStore1", Common::TimeSpan::MaxValue, CancellationToken::None);
        co_await stateManager.CommitRemoveAsync(*txn);
        co_return;
    }

    MockStateManager::SPtr CreateStateManager(
        __in StoreStateProviderFactory::SPtr factorySPtr,
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

#pragma region test functions
    public:
        ktl::Awaitable<void> AddStore_NumKeyStringVal_ShouldSucceed_Test()
    {
        auto replicatorSPtr = CreateMockReplicator(FABRIC_REPLICA_ROLE_PRIMARY);
        StoreStateProviderFactory::SPtr factorySPtr = StoreStateProviderFactory::CreateLongStringUTF8Factory(GetAllocator());
        MockStateManager::SPtr stateManagerSPtr = CreateStateManager(factorySPtr, *replicatorSPtr);
        co_await AddStoreAsync(*stateManagerSPtr);
        co_await stateManagerSPtr->CloseAsync();
        co_return;
    }

        ktl::Awaitable<void> AddStore_StringUTF8KeyBufferVal_ShouldSucceed_Test()
    {
        auto replicatorSPtr = CreateMockReplicator(FABRIC_REPLICA_ROLE_PRIMARY);
        StoreStateProviderFactory::SPtr factorySPtr = StoreStateProviderFactory::CreateStringUTF8BufferFactory(GetAllocator());
        MockStateManager::SPtr stateManagerSPtr = CreateStateManager(factorySPtr, *replicatorSPtr);
        co_await AddStoreAsync(*stateManagerSPtr);
        co_await stateManagerSPtr->CloseAsync();
        co_return;
    }

        ktl::Awaitable<void> AddStore_StringUTF16KeyBufferVal_ShouldSucceed_Test()
    {
        auto replicatorSPtr = CreateMockReplicator(FABRIC_REPLICA_ROLE_PRIMARY);
        StoreStateProviderFactory::SPtr factorySPtr = StoreStateProviderFactory::CreateStringUTF16BufferFactory(GetAllocator());
        MockStateManager::SPtr stateManagerSPtr = CreateStateManager(factorySPtr, *replicatorSPtr);
        co_await AddStoreAsync(*stateManagerSPtr);
        co_await stateManagerSPtr->CloseAsync();
        co_return;
    }

        ktl::Awaitable<void> RemoveStore_NumKeyStringVal_ShouldSucceed_Test()
    {
        auto replicatorSPtr = CreateMockReplicator(FABRIC_REPLICA_ROLE_PRIMARY);
        StoreStateProviderFactory::SPtr factorySPtr = StoreStateProviderFactory::CreateLongStringUTF8Factory(GetAllocator());
        MockStateManager::SPtr stateManagerSPtr = CreateStateManager(factorySPtr, *replicatorSPtr);
        co_await AddStoreAsync(*stateManagerSPtr);
        co_await RemoveStoreAsync(*stateManagerSPtr);
        co_await stateManagerSPtr->CloseAsync();
        co_return;
    }

        ktl::Awaitable<void> StoreInitializationParameters_Serialization_ShouldSucceed_Test()
    {
        StoreInitializationParameters::SPtr paramSPtr = nullptr;
        NTSTATUS status = StoreInitializationParameters::Create(GetAllocator(), StoreBehavior::Historical, false, paramSPtr);
        Diagnostics::Validate(status);

        OperationData::SPtr initParametersSPtr = paramSPtr->ToOperationData(GetAllocator());
        auto paramSPtr2 = StoreInitializationParameters::FromOperationData(GetAllocator(), *initParametersSPtr);
        CODING_ERROR_ASSERT(paramSPtr2->StoreBehaviorProperty == paramSPtr->StoreBehaviorProperty);
        CODING_ERROR_ASSERT(paramSPtr2->AllowReadableSecondary == paramSPtr->AllowReadableSecondary);
        CODING_ERROR_ASSERT(paramSPtr2->Version == paramSPtr->Version);
        co_return;
    }

        ktl::Awaitable<void> AddStore_BufferKeyBufferVal_ShouldSucceed_Test()
    {
        auto replicatorSPtr = CreateMockReplicator(FABRIC_REPLICA_ROLE_PRIMARY);
        StoreStateProviderFactory::SPtr factorySPtr = StoreStateProviderFactory::CreateBufferBufferFactory(GetAllocator());
        MockStateManager::SPtr stateManagerSPtr = CreateStateManager(factorySPtr, *replicatorSPtr);
        co_await AddStoreAsync(*stateManagerSPtr);
        co_await stateManagerSPtr->CloseAsync();
        co_return;
    }
    #pragma endregion
};

BOOST_FIXTURE_TEST_SUITE(StoreStateProviderTestSuite, StoreStateProviderTest)

BOOST_AUTO_TEST_CASE(AddStore_NumKeyStringVal_ShouldSucceed)
{
    SyncAwait(AddStore_NumKeyStringVal_ShouldSucceed_Test());
}

BOOST_AUTO_TEST_CASE(AddStore_StringUTF8KeyBufferVal_ShouldSucceed)
{
    SyncAwait(AddStore_StringUTF8KeyBufferVal_ShouldSucceed_Test());
}

BOOST_AUTO_TEST_CASE(AddStore_StringUTF16KeyBufferVal_ShouldSucceed)
{
    SyncAwait(AddStore_StringUTF16KeyBufferVal_ShouldSucceed_Test());
}

BOOST_AUTO_TEST_CASE(RemoveStore_NumKeyStringVal_ShouldSucceed)
{
    SyncAwait(RemoveStore_NumKeyStringVal_ShouldSucceed_Test());
}

BOOST_AUTO_TEST_CASE(StoreInitializationParameters_Serialization_ShouldSucceed)
{
    SyncAwait(StoreInitializationParameters_Serialization_ShouldSucceed_Test());
}

BOOST_AUTO_TEST_CASE(AddStore_BufferKeyBufferVal_ShouldSucceed)
{
    SyncAwait(AddStore_BufferKeyBufferVal_ShouldSucceed_Test());
}

BOOST_AUTO_TEST_SUITE_END()
}
