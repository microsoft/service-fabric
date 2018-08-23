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
    class StateManagerCheckpointPerfTest : public StateManagerTestBase
    {
        // load the config object as its needed for the tracing to work
        CommonConfig config;

    public:
        StateManagerCheckpointPerfTest()
        {
        }

        ~StateManagerCheckpointPerfTest()
        {
        }

    public:
        // Checkpoint Performance Test 
        Awaitable<void> Test_Checkpoint_Performance_Test_CreateAsyncAndOpenAsync(
            __in ULONG itemsCount) noexcept;
        Awaitable<void> Test_Checkpoint_Performance_Test(
            __in ULONG itemsCount) noexcept;

    private:
        KUriView expectedNameView = L"fabric:/sps/sp";
        const LONG64 expectedParentId = 16;
        const LONG64 expectedCreateLSN = 19;
        const LONG64 expectedDeleteLSN = 87;
        const MetadataMode::Enum expectedMetadataMode = MetadataMode::Enum::Active;
    };


    //
    // Goal: Checkpoint Performance Test, This test will focus on CreateAsync And OpenAsync
    //       The time interval for CreateAsync And OpenAsync will be printed out 
    //
    // Algorithm:
    //    1. Populate the Serializable MetadataArray list
    //    2. Create checkpoint file call CreateAsync, and print out the time interval
    //    3. Open the checkpoint file call OpenAsync, and print out the time interval
    //    4. Verify the checkpoint file is the same as we wrote
    //    5. Clean up 
    //
    Awaitable<void> StateManagerCheckpointPerfTest::Test_Checkpoint_Performance_Test_CreateAsyncAndOpenAsync(
        __in ULONG itemsCount) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        SerializableMetadata::CSPtr serializableMetadataCSPtr = nullptr;
        KWString fileName = TestHelper::CreateFileName(L"Test_Checkpoint_Performance_Test_CreateAsyncAndOpenAsync.txt", GetAllocator());

        KGuid expectedPartitionId;
        expectedPartitionId.CreateNew();
        ULONG64 expectedReplicId = 8;
        FABRIC_SEQUENCE_NUMBER expectedPrepareCheckpointLSN = 64;
        PartitionedReplicaId::SPtr partitionedReplicaIdCSPtr = PartitionedReplicaId::Create(expectedPartitionId, expectedReplicId, GetAllocator());

        // verifier used to verify the state provider ids are the same
        LONG64 verifier = 0;

        KSharedArray<SerializableMetadata::CSPtr>::SPtr SerializableMetadataArray = TestHelper::CreateSerializableMetadataArray(GetAllocator());

        for (ULONG i = 0; i < itemsCount; i++)
        {
            LONG64 stateProviderId = KDateTime::Now();
            verifier = verifier ^ stateProviderId;
            SerializableMetadata::CSPtr serializablemetadataSPtr = TestHelper::CreateSerializableMetadata(
                stateProviderId,
                expectedNameView,
                expectedParentId,
                expectedCreateLSN,
                expectedDeleteLSN, GetAllocator());
            status = SerializableMetadataArray->Append(serializablemetadataSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        Common::Stopwatch stopwatch;
        stopwatch.Start();
        
        CheckpointFile::SPtr checkpointFileSPtr = nullptr;
        status = CheckpointFile::Create(
            *partitionedReplicaIdCSPtr,
            fileName, 
            GetAllocator(),
            checkpointFileSPtr);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        co_await checkpointFileSPtr->WriteAsync(
            *SerializableMetadataArray, 
            SerializationMode::Enum::Native, 
            false, 
            expectedPrepareCheckpointLSN, 
            CancellationToken::None);

        stopwatch.Stop();
        Trace.WriteInfo(
            BoostTestTrace, 
            "Test_Checkpoint_Performance_Test_CreateAsyncAndOpenAsync with {0} items, Checkpoint Performance Test WriteAsync call completed in {1} ms", 
            itemsCount, 
            stopwatch.ElapsedMilliseconds);

        stopwatch.Restart();

        // Open the checkpoint file call OpenAsync, and print out the time interval
        // The OpenAsync should iterate through the enumerator
        co_await checkpointFileSPtr->ReadAsync(CancellationToken::None);
        CheckpointFileAsyncEnumerator::SPtr enumerator = checkpointFileSPtr->GetAsyncEnumerator();

        while (true)
        {
            // Since we are testing the performance of read, ignore the failure status check.
            status = co_await enumerator->GetNextAsync(CancellationToken::None, serializableMetadataCSPtr);
            if (status == STATUS_NOT_FOUND || !NT_SUCCESS(status))
            {
                break;
            }
        }

        // Close the file stream and file 
        co_await enumerator->CloseAsync();

        stopwatch.Stop();
        Trace.WriteInfo(
            BoostTestTrace,
            "Test_Checkpoint_Performance_Test_CreateAsyncAndOpenAsync with {0} items, Checkpoint Performance Test ReadAsync call completed in {1} ms, Status: {2}",
            itemsCount,
            stopwatch.ElapsedMilliseconds,
            status);

        // Verify the checkpoint file is the same as we wrote
        CheckpointFileAsyncEnumerator::SPtr enumeratorVerify = checkpointFileSPtr->GetAsyncEnumerator();
        ULONG i = 0;

        while (true)
        {
            status = co_await enumeratorVerify->GetNextAsync(CancellationToken::None, serializableMetadataCSPtr);
            if (status == STATUS_NOT_FOUND)
            {
                break;
            }

            VERIFY_IS_TRUE(NT_SUCCESS(status));

            verifier = verifier ^ serializableMetadataCSPtr->StateProviderId;
            VERIFY_IS_TRUE(serializableMetadataCSPtr->Name->Get(KUriView::eRaw).Compare(expectedNameView) == 0);
            VERIFY_IS_TRUE(serializableMetadataCSPtr->ParentStateProviderId == expectedParentId);
            VERIFY_IS_TRUE(serializableMetadataCSPtr->CreateLSN == expectedCreateLSN);
            VERIFY_IS_TRUE(serializableMetadataCSPtr->DeleteLSN == expectedDeleteLSN);
            VERIFY_IS_TRUE(serializableMetadataCSPtr->MetadataMode == expectedMetadataMode);

            ++i;
        }

        co_await enumeratorVerify->CloseAsync();
        VERIFY_IS_TRUE(verifier == 0);
        VERIFY_IS_TRUE(i == itemsCount);

        // Clean up
        KString::CSPtr filePath = TestHelper::CreateFileString(GetAllocator(), L"Test_Checkpoint_Performance_Test_CreateAsyncAndOpenAsync.txt");
        Common::File::Delete(static_cast<LPCWSTR>(*filePath), true);
    }

    //
    // Goal: Checkpoint Performance Test, This test will focus on CheckpointAsync
    //       The time interval for CheckpointAsync will be printed out 
    //
    // Algorithm:
    //    1. Populate the state providers name list
    //    2. Bring up the primary replica
    //    3. Populate state providers and take checkpoint, print out the time interval for CheckpointAsync call
    //    4. Verify the checkpoint file is as expected
    //    5. Clean up and shut down
    //
    Awaitable<void> StateManagerCheckpointPerfTest::Test_Checkpoint_Performance_Test(
        __in ULONG itemsCount) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        Common::Stopwatch stopwatch;

        // Setup: Populate the state providers name list
        KArray<KUri::CSPtr> nameList(GetAllocator(), itemsCount);
        for (ULONG index = 0; index < itemsCount; index++)
        {
            status = nameList.Append(GetStateProviderName(NameType::Random));
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        // Setup: Bring up the primary replica
        {
            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
            co_await this->PopulateAsync(nameList);
            VerifyExist(nameList, true);

            // #10485130: Disable the Test StateProvider WriteFile.
            this->DisableStateProviderCheckpointing(nameList);

            stopwatch.Start();
            co_await testTransactionalReplicatorSPtr_->PrepareCheckpointAsync();
            stopwatch.Stop();
            Trace.WriteInfo(
                BoostTestTrace,
                "Test_Checkpoint_Performance_Test with {0} items, PrepareCheckpoint completed in {1} ms",
                itemsCount,
                stopwatch.ElapsedMilliseconds);

            stopwatch.Restart();
            co_await testTransactionalReplicatorSPtr_->PerformCheckpointAsync(CancellationToken::None);
            stopwatch.Stop();
            Trace.WriteInfo(
                BoostTestTrace,
                "Test_Checkpoint_Performance_Test with {0} items, PerformCheckpointAsync completed in {1} ms",
                itemsCount,
                stopwatch.ElapsedMilliseconds);

            stopwatch.Restart();
            co_await testTransactionalReplicatorSPtr_->CompleteCheckpointAsync(CancellationToken::None);
            stopwatch.Stop();
            Trace.WriteInfo(
                BoostTestTrace,
                "Test_Checkpoint_Performance_Test with {0} items, CompleteCheckpointAsync completed in {1} ms",
                itemsCount,
                stopwatch.ElapsedMilliseconds);

            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
            testTransactionalReplicatorSPtr_.Reset();
        }

        // Verify the checkpoint file is as expected
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

    BOOST_FIXTURE_TEST_SUITE(StateManagerCheckpointPerfTestSuite, StateManagerCheckpointPerfTest)

    //
    // Scenario:        Checkpoint Performance Test, test mainly focus on CreateAsync and OpenAsync
    // Expected Result: The time interval for CreateAsync and OpenAsync will be printed out 
    // 
    BOOST_AUTO_TEST_CASE(Checkpoint_Performance_Test_CreateAsyncAndOpenAsync_ZeroItems)
    {
        ULONG itemsCount = 0;
        SyncAwait(this->Test_Checkpoint_Performance_Test_CreateAsyncAndOpenAsync(itemsCount));
    }

    //
    // Scenario:        Checkpoint Performance Test, test mainly focus on CreateAsync and OpenAsync
    // Expected Result: The time interval for CreateAsync and OpenAsync will be printed out 
    // 
    BOOST_AUTO_TEST_CASE(Checkpoint_Performance_Test_CreateAsyncAndOpenAsync_ThousandItems)
    {
        const ULONG itemsCount = 1000;
        SyncAwait(this->Test_Checkpoint_Performance_Test_CreateAsyncAndOpenAsync(itemsCount));
    }

    //
    // Scenario:        Checkpoint Performance Test, test mainly focus on CreateAsync and OpenAsync
    // Expected Result: The time interval for CreateAsync and OpenAsync will be printed out 
    // 
    BOOST_AUTO_TEST_CASE(Checkpoint_Performance_Test_CreateAsyncAndOpenAsync_TenThousandItems)
    {
        const ULONG itemsCount = 10000;
        SyncAwait(this->Test_Checkpoint_Performance_Test_CreateAsyncAndOpenAsync(itemsCount));
    }

    //
    // Scenario:        Checkpoint Performance Test, test mainly focus on CreateAsync and OpenAsync
    // Expected Result: The time interval for CreateAsync and OpenAsync will be printed out 
    // 
    BOOST_AUTO_TEST_CASE(Checkpoint_Performance_Test_CreateAsyncAndOpenAsync_FiftyThousandItems)
    {
        const ULONG itemsCount = 50000;
        SyncAwait(this->Test_Checkpoint_Performance_Test_CreateAsyncAndOpenAsync(itemsCount));
    }

    //
    // Scenario:        Checkpoint Performance Test, test mainly focus on CreateAsync and OpenAsync
    // Expected Result: The time interval for CreateAsync and OpenAsync will be printed out 
    // 
    BOOST_AUTO_TEST_CASE(Checkpoint_Performance_Test_CreateAsyncAndOpenAsync_HundredThousandItems)
    {
        const ULONG itemsCount = 100000;
        SyncAwait(this->Test_Checkpoint_Performance_Test_CreateAsyncAndOpenAsync(itemsCount));
    }

    //
    // Scenario:        Checkpoint Performance Test, test mainly focus on CheckpointAsync function
    // Expected Result: The time interval for CheckpointAsync will be printed out 
    // 
    BOOST_AUTO_TEST_CASE(Checkpoint_Performance_Test_ZeroItem)
    {
        const ULONG itemsCount = 0;
        SyncAwait(this->Test_Checkpoint_Performance_Test(itemsCount));
    }

    //
    // Scenario:        Checkpoint Performance Test, test mainly focus on CheckpointAsync function
    // Expected Result: The time interval for CheckpointAsync will be printed out 
    // 
    BOOST_AUTO_TEST_CASE(Checkpoint_Performance_Test_ThousandItems)
    {
        const ULONG itemsCount = 1000;
        SyncAwait(this->Test_Checkpoint_Performance_Test(itemsCount));
    }

    //
    // Scenario:        Checkpoint Performance Test, test mainly focus on CheckpointAsync function
    // Expected Result: The time interval for CheckpointAsync will be printed out 
    // 
    BOOST_AUTO_TEST_CASE(Checkpoint_Performance_Test_TenThousandItems)
    {
        const ULONG itemsCount = 10000;
        SyncAwait(this->Test_Checkpoint_Performance_Test(itemsCount));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
