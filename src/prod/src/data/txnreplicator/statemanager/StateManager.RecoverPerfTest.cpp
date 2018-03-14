// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <stdlib.h>
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace Common;
using namespace ktl;
using namespace TxnReplicator;
using namespace Data::StateManager;
using namespace Data::Utilities;

namespace StateManagerTests
{
    class StateManagerRecoverPerfTest : public StateManagerTestBase
    {
        // load the config object as its needed for the tracing to work
        CommonConfig config;

    public:
        StateManagerRecoverPerfTest()
        {
        }

        ~StateManagerRecoverPerfTest()
        {
        }

    public:
        // Checkpoint Performance Test 
        Awaitable<void> Test_RecoverCheckpoint_Performance_Test(
            __in ULONG itemsCount) noexcept;
    };

    //
    // Goal: Recover Checkpoint Performance Test, This test will focus on Recover Checkpoint File
    //       The time interval for Recover will be printed out 
    //
    // Algorithm:
    //    1. Populate the state providers name list
    //    2. Bring up the primary replica
    //    3. Populate state providers and take checkpoint
    //    4. Close the replica 
    //    5. Recover the checkpoint file by open the replica 
    //    6. Clean up and shut down
    //
    Awaitable<void> StateManagerRecoverPerfTest::Test_RecoverCheckpoint_Performance_Test(
        __in ULONG itemsCount) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        Common::Stopwatch stopwatch;

        // Setup: Populate the state providers name list
        KArray<KUri::CSPtr> nameList(GetAllocator(), itemsCount);
        for (ULONG index = 0; index < itemsCount; index++)
        {
            status = nameList.Append(GetStateProviderName(NameType::Random));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        // Setup: Bring up the primary replica
        {
            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
            co_await this->PopulateAsync(nameList);
            VerifyExist(nameList, true);

            // #10485130: Disable the Test StateProvider WriteFile.
            this->DisableStateProviderCheckpointing(nameList);

            // Take checkpoint 
            co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);
          
            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
            testTransactionalReplicatorSPtr_.Reset();
        }

        // Recover from the checkpoint file
        {
            testTransactionalReplicatorSPtr_ = CreateReplica(
                *partitionedReplicaIdCSPtr_,
                *runtimeFolders_,
                *partitionSPtr_);

            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

            VerifyExist(nameList, true);

            // Clean up and shut down
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
        }
    }

    BOOST_FIXTURE_TEST_SUITE(StateManagerRecoverPerfTestSuite, StateManagerRecoverPerfTest)

    //
    // Scenario:        Recover Checkpoint Performance Test, This test will focus on Recover Checkpoint File
    // Expected Result: The time interval for Recover Checkpoint will be printed out 
    // 
    BOOST_AUTO_TEST_CASE(Recover_Checkpoint_Performance_Test_ZeroItem)
    {
        const ULONG itemsCount = 0;
        SyncAwait(this->Test_RecoverCheckpoint_Performance_Test(itemsCount));
    }

    //
    // Scenario:        Recover Checkpoint Performance Test, This test will focus on Recover Checkpoint File
    // Expected Result: The time interval for Recover Checkpoint will be printed out 
    // 
    BOOST_AUTO_TEST_CASE(Recover_Checkpoint_Performance_Test_ThousandItems)
    {
        const ULONG itemsCount = 1000;
        SyncAwait(this->Test_RecoverCheckpoint_Performance_Test(itemsCount));
    }

    //
    // Scenario:        Recover Checkpoint Performance Test, This test will focus on Recover Checkpoint File
    // Expected Result: The time interval for Recover Checkpoint will be printed out 
    // 
    BOOST_AUTO_TEST_CASE(Recover_Checkpoint_Performance_Test_TenThousandItems)
    {
        const ULONG itemsCount = 10000;
        SyncAwait(this->Test_RecoverCheckpoint_Performance_Test(itemsCount));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
