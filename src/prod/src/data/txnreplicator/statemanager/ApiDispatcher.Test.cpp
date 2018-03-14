// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <stdlib.h>
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace ktl;
using namespace TxnReplicator;
using namespace Data::StateManager;
using namespace Data::Utilities;

namespace StateManagerTests
{
    class ApiDispatcherTests : StateManagerTestBase
    {
        // load the config object as its needed for the tracing to work
        Common::CommonConfig config;

    public:
        ApiDispatcherTests()
        {
        }

        ~ApiDispatcherTests()
        {
        }

    public:
        Awaitable<void> Test_OpenAsync_NoFaultyAPIs_SUCCESS(
            __in ULONG count,
            __in bool useClose);
        Awaitable<void> Test_OpenAsync_FaultyAPIs_SUCCESS(
            __in ULONG countPerType);

        Awaitable<void> Test_CloseAsync_FaultyAPIs_SUCCESS(
            __in ULONG countPerType);
    };

    Awaitable<void> ApiDispatcherTests::Test_OpenAsync_NoFaultyAPIs_SUCCESS(
        __in ULONG count,
        __in bool useClose)
    {
        // Setup
        KGuid partitionId; 
        partitionId.CreateNew();
        PartitionedReplicaId::SPtr prId = PartitionedReplicaId::Create(partitionId, 17, GetAllocator());

        TestStateProviderFactory::SPtr testStateProviderFactory = TestStateProviderFactory::Create(GetAllocator());

        ApiDispatcher::SPtr apiDispatcher = ApiDispatcher::Create(*prId, *testStateProviderFactory, GetAllocator());

        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        KWeakRef<ITransactionalReplicator>::SPtr txnReplicator;
        NTSTATUS status = testTransactionalReplicatorSPtr_->GetWeakIfRef(txnReplicator);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        wstring currentFolder = Common::Directory::GetCurrentDirectoryW();
        KStringView workFolder = KStringView(currentFolder.c_str());

        // Setup test.
        KArray<Metadata::CSPtr> metadataArray(GetAllocator(), count);
        ULONG lastStateProviderId = 0;
        for (ULONG i = 0; i < count; i++)
        {
            LONG64 stateProviderId = ++lastStateProviderId;

            KUri::CSPtr name = TestHelper::CreateStateProviderName(stateProviderId, GetAllocator());
            Metadata::SPtr metadata = TestHelper::CreateMetadata(*name, false, stateProviderId, GetAllocator());
            status = metadataArray.Append(metadata.RawPtr());
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            status = apiDispatcher->Initialize(*metadata, *txnReplicator, workFolder, nullptr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }
        
        co_await apiDispatcher->OpenAsync(metadataArray, CancellationToken::None);

        if (useClose)
        {
            status = co_await apiDispatcher->CloseAsync(
                metadataArray, 
                ApiDispatcher::FailureAction::AbortStateProvider,
                CancellationToken::None);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }
        else
        {
            apiDispatcher->Abort(metadataArray, 0, metadataArray.Count());
        }

        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
    }

    // [0               , countPerType):        Successful.
    // [countPerType    , 2 * countPerType):    Asynchronous failure.
    // [2 * countPerType, 3 * countPerType):    Successful.
    // [3 * countPerType, 4 * countPerType):    Synchronous failure.
    Awaitable<void> ApiDispatcherTests::Test_OpenAsync_FaultyAPIs_SUCCESS(
        __in ULONG countPerType)
    {
        // Setup
        KGuid partitionId;
        partitionId.CreateNew();
        PartitionedReplicaId::SPtr prId = PartitionedReplicaId::Create(partitionId, 17, GetAllocator());

        TestStateProviderFactory::SPtr testStateProviderFactory = TestStateProviderFactory::Create(GetAllocator());

        ApiDispatcher::SPtr apiDispatcher = ApiDispatcher::Create(*prId, *testStateProviderFactory, GetAllocator());

        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        KWeakRef<ITransactionalReplicator>::SPtr txnReplicator;
        NTSTATUS status = testTransactionalReplicatorSPtr_->GetWeakIfRef(txnReplicator);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        wstring currentFolder = Common::Directory::GetCurrentDirectoryW();
        KStringView workFolder = KStringView(currentFolder.c_str());

        // Setup test.
        KArray<Metadata::CSPtr> metadataArray(GetAllocator(), countPerType * 4);
        ULONG lastStateProviderId = 0;
        for (ULONG i = 0; i < countPerType * 4; i++)
        {
            LONG64 stateProviderId = ++lastStateProviderId;

            KUri::CSPtr name = TestHelper::CreateStateProviderName(stateProviderId, GetAllocator());
            Metadata::SPtr metadata = TestHelper::CreateMetadata(*name, false, stateProviderId, GetAllocator());
            status = metadataArray.Append(metadata.RawPtr());
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            status = apiDispatcher->Initialize(*metadata, *txnReplicator, workFolder, nullptr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            TestStateProvider & tsp = dynamic_cast<TestStateProvider &>(*metadata->StateProvider);

            if (i < countPerType)
            {
            }
            else if (i < countPerType * 2)
            {
                tsp.FaultyAPI = FaultStateProviderType::EndOpenAsync;
            }
            else if (i < countPerType * 3)
            {
            }
            else if (i < countPerType * 4)
            {
                tsp.FaultyAPI = FaultStateProviderType::BeginOpenAsync;
            }
        }

        status = co_await apiDispatcher->OpenAsync(metadataArray, CancellationToken::None);
        VERIFY_ARE_EQUAL(SF_STATUS_COMMUNICATION_ERROR, status);

        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
    }

    // [0               , countPerType):        Successful.
    // [countPerType    , 2 * countPerType):    Asynchronous failure.
    // [2 * countPerType, 3 * countPerType):    Successful.
    // [3 * countPerType, 4 * countPerType):    Synchronous failure.
    Awaitable<void> ApiDispatcherTests::Test_CloseAsync_FaultyAPIs_SUCCESS(
        __in ULONG countPerType)
    {
        // Setup
        KGuid partitionId;
        partitionId.CreateNew();
        PartitionedReplicaId::SPtr prId = PartitionedReplicaId::Create(partitionId, 17, GetAllocator());

        TestStateProviderFactory::SPtr testStateProviderFactory = TestStateProviderFactory::Create(GetAllocator());

        ApiDispatcher::SPtr apiDispatcher = ApiDispatcher::Create(*prId, *testStateProviderFactory, GetAllocator());

        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        KWeakRef<ITransactionalReplicator>::SPtr txnReplicator;
        NTSTATUS status = testTransactionalReplicatorSPtr_->GetWeakIfRef(txnReplicator);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        wstring currentFolder = Common::Directory::GetCurrentDirectoryW();
        KStringView workFolder = KStringView(currentFolder.c_str());

        // Setup test.
        KArray<Metadata::CSPtr> metadataArray(GetAllocator(), countPerType * 4);
        ULONG lastStateProviderId = 0;
        for (ULONG i = 0; i < countPerType * 4; i++)
        {
            LONG64 stateProviderId = ++lastStateProviderId;

            KUri::CSPtr name = TestHelper::CreateStateProviderName(stateProviderId, GetAllocator());
            Metadata::SPtr metadata = TestHelper::CreateMetadata(*name, false, stateProviderId, GetAllocator());
            status = metadataArray.Append(metadata.RawPtr());
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            status = apiDispatcher->Initialize(*metadata, *txnReplicator, workFolder, nullptr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            TestStateProvider & tsp = dynamic_cast<TestStateProvider &>(*metadata->StateProvider);

            if (i < countPerType)
            {
            }
            else if (i < countPerType * 2)
            {
                tsp.FaultyAPI = FaultStateProviderType::EndCloseAsync;
            }
            else if (i < countPerType * 3)
            {
            }
            else if (i < countPerType * 4)
            {
                tsp.FaultyAPI = FaultStateProviderType::BeginCloseAsync;
            }
        }
        
        // Prepare
        status = co_await apiDispatcher->OpenAsync(metadataArray, CancellationToken::None);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        // Verify close is successful with corrective action.
        status = co_await apiDispatcher->CloseAsync(metadataArray, ApiDispatcher::AbortStateProvider, CancellationToken::None);
        VERIFY_ARE_EQUAL(STATUS_SUCCESS, status);

        // Shutdown.
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
    }

    BOOST_FIXTURE_TEST_SUITE(ApiDispatcherTestSuite, ApiDispatcherTests);

    BOOST_AUTO_TEST_CASE(OpenAsync_NoFaultyAPIs_Abort_SUCCESS)
    {
        SyncAwait(this->Test_OpenAsync_NoFaultyAPIs_SUCCESS(4096, false));
    }

    BOOST_AUTO_TEST_CASE(OpenAsync_NoFaultyAPIs_Close_SUCCESS)
    {
        SyncAwait(this->Test_OpenAsync_NoFaultyAPIs_SUCCESS(4096, true));
    }

    BOOST_AUTO_TEST_CASE(OpenAsync_FaultyAPIs_SUCCESS)
    {
        SyncAwait(this->Test_OpenAsync_FaultyAPIs_SUCCESS(16));
    }

    BOOST_AUTO_TEST_CASE(CloseAsync_FaultyAPIs_SUCCESS)
    {
        SyncAwait(this->Test_CloseAsync_FaultyAPIs_SUCCESS(16));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
