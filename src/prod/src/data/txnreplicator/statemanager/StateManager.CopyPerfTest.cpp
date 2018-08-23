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
    class StateManagerCopyPerfTest : public StateManagerTestBase
    {
        // load the config object as its needed for the tracing to work
        CommonConfig config;

    public:
        StateManagerCopyPerfTest()
        {
        }

        ~StateManagerCopyPerfTest()
        {
        }

    public:
        // Copy Performance Test 
        Awaitable<void> Test_Copy_Performance_Test(
            __in ULONG itemsCount) noexcept;
    };

    //
    // Goal: Copy Performance Test, This test will focus on get copystream call GetCurrentState 
    //       and copy state call CopyAsync
    //       The time interval for GetCurrentState and  CopyAsync will be printed out 
    //
    // Algorithm:
    //    1. Populate the state providers name list
    //    2. Bring up the primary replica
    //    3. Populate state providers and take checkpoint, then GetCurrentState and print out the time interval for GetCurrentState
    //    4. Clean up and shut down the primary replica 
    //    5. Bring up the Secondary replica
    //    6. Copy the states in the copy stream
    //    7. Verify the state is as expected
    //    8. Clean up and shut down Secondary replica
    //
    Awaitable<void> StateManagerCopyPerfTest::Test_Copy_Performance_Test(
        __in ULONG itemsCount) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        Common::Stopwatch stopwatch;
        Common::Stopwatch stopwatchGetCopyStream;

        // Setup: Populate the state providers name list
        KArray<KUri::CSPtr> nameList(GetAllocator(), itemsCount);

        for (ULONG index = 0; index < itemsCount; index++)
        {
            auto name = GetStateProviderName(NameType::Random);
            status = nameList.Append(name);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        OperationDataStream::SPtr copyStream;

        // Setup: Bring up the primary replica
        {
            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

            // Populate state providers
            co_await this->PopulateAsync(nameList);

            // #10485130: Disable the Test StateProvider WriteFile.
            this->DisableStateProviderCheckpointing(nameList);

            co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);

            // Get the copy stream. The time interval for GetCurrentState should include the enumeration part.
            stopwatchGetCopyStream.Start();
            status = testTransactionalReplicatorSPtr_->StateManager->GetCurrentState(
                (partitionedReplicaIdCSPtr_->ReplicaId + 1),
                copyStream);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            stopwatchGetCopyStream.Stop();

            // This is to verify the test.
            VerifyExist(nameList, true);

            // Clean up and shut down the primary replica 
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
        }

        {
            // Bring up the Secondary replica
            LONG64 replicaId = partitionedReplicaIdCSPtr_->ReplicaId + 1;
            auto partitionedReplicaId = PartitionedReplicaId::Create(partitionedReplicaIdCSPtr_->PartitionId, replicaId, GetAllocator());

            auto runtimeFolders = TestHelper::CreateRuntimeFolders(GetAllocator());
            auto partition = TestHelper::CreateStatefulServicePartition(GetAllocator());
            TestTransactionManager::SPtr secondaryTransactionalReplicator = CreateReplica(*partitionedReplicaId, *runtimeFolders, *partition);

            co_await secondaryTransactionalReplicator->OpenAsync(CancellationToken::None);
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_IDLE_SECONDARY, CancellationToken::None);
            auto secondarystateManagerSPtr = secondaryTransactionalReplicator->StateManager;

            // Copy the states in the copy stream
            {
                stopwatch.Start();
                co_await secondarystateManagerSPtr->BeginSettingCurrentStateAsync();
                stopwatch.Stop();
                Trace.WriteInfo(
                    BoostTestTrace,
                    "Copy Performance Test CopyAsync:BeginSettingCurrentState call with {0} items completed in {1} ms.",
                    itemsCount,
                    stopwatch.ElapsedMilliseconds);

                LONG64 stateRecordNumber = 0;
                while (true)
                {
                    stopwatchGetCopyStream.Start();
                    OperationData::CSPtr operationData;
                    co_await copyStream->GetNextAsync(CancellationToken::None, operationData);
                    stopwatchGetCopyStream.Stop();

                    if (operationData == nullptr)
                    {
                        break;
                    }

                    if (stateRecordNumber == 0)
                    {
                        stopwatch.Restart();
                    }
                    else
                    {
                        stopwatch.Start();
                    }

                    co_await secondarystateManagerSPtr->SetCurrentStateAsync(stateRecordNumber++, *operationData);
                    stopwatch.Stop();
                }

                Trace.WriteInfo(
                    BoostTestTrace,
                    "Copy Performance Test CopyAsync:SetCurrentStateAsync call with {0} items completed in {1} ms.",
                    itemsCount,
                    stopwatch.ElapsedMilliseconds);

                stopwatch.Restart();
                co_await secondarystateManagerSPtr->EndSettingCurrentState();
                stopwatch.Stop();
                Trace.WriteInfo(
                    BoostTestTrace,
                    "Copy Performance Test CopyAsync:EndSettingCurrentState call with {0} items completed in {1} ms.",
                    itemsCount,
                    stopwatch.ElapsedMilliseconds);
            }

            Trace.WriteInfo(
                BoostTestTrace,
                "Copy Performance Test GetCurrentState call with {0} items completed in {1} ms.",
                itemsCount,
                stopwatchGetCopyStream.ElapsedMilliseconds);

            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, CancellationToken::None);

            // Verify the state is as expected
            VerifyExist(nameList, true, secondaryTransactionalReplicator->StateManager.RawPtr());

            // Clean up and shut down Secondary replica
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            co_await secondaryTransactionalReplicator->CloseAsync(CancellationToken::None);
        }
    }

    BOOST_FIXTURE_TEST_SUITE(StateManagerCopyPerfTestSuite, StateManagerCopyPerfTest)

    //
    // Scenario:        Copy Performance Test with all active state providers, test mainly focus on the time 
    //                  interval for GetCurrentState and each steps of CopyAsync (BeginSettingCurrentState, 
    //                  SetCurrentStateAsync and EndSettingCurrentState)
    // Expected Result: The time interval for each CopyAsync steps will be printed out 
    // 
    BOOST_AUTO_TEST_CASE(Copy_Performance_Test_TenThousandItems)
    {
        const ULONG itemsCount = 10000;
        SyncAwait(this->Test_Copy_Performance_Test(itemsCount));
    }

    //
    // Scenario:        Copy Performance Test with all active state providers, test mainly focus on the time 
    //                  interval for GetCurrentState and each steps of CopyAsync (BeginSettingCurrentState, 
    //                  SetCurrentStateAsync and EndSettingCurrentState)
    // Expected Result: The time interval for each CopyAsync steps will be printed out 
    // 
    BOOST_AUTO_TEST_CASE(Copy_Performance_Test_TwentyFiveThousandItems)
    {
        const ULONG itemsCount = 25000;
        SyncAwait(this->Test_Copy_Performance_Test(itemsCount));
    }

    //
    // Scenario:        Copy Performance Test with all active state providers, test mainly focus on the time 
    //                  interval for GetCurrentState and each steps of CopyAsync (BeginSettingCurrentState, 
    //                  SetCurrentStateAsync and EndSettingCurrentState)
    // Expected Result: The time interval for each CopyAsync steps will be printed out 
    // 
    BOOST_AUTO_TEST_CASE(Copy_Performance_Test_FiftyThousandItems)
    {
        const ULONG itemsCount = 50000;
        SyncAwait(this->Test_Copy_Performance_Test(itemsCount));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
