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
    class FaultStateProviderAPIsTest : public StateManagerTestBase
    {
        // load the config object as its needed for the tracing to work
        CommonConfig config;

    public:
        FaultStateProviderAPIsTest()
        {
        }

        ~FaultStateProviderAPIsTest()
        {
        }

    public:
        Awaitable<void> Test_StateProvider_FaultyConstructor() noexcept;

        Awaitable<void> Test_StateProvider_FaultyOpenAsync() noexcept;

        Awaitable<void> Test_StateProvider_FaultyChangeRoleAsync_AddPath() noexcept;

        Awaitable<void> Test_StateProvider_FaultyChangeRoleAsync() noexcept;

        Awaitable<void> Test_StateProvider_FaultyCheckpoint(__in FaultStateProviderType::FaultAPI faultAPI) noexcept;

        Awaitable<void> Test_StateProvider_FaultyRecoverCheckpointAsync() noexcept;

        Awaitable<void> Test_StateProvider_FaultyRecoverCheckpointAsync_RecoveryPath() noexcept;

        Awaitable<void> Test_StateProvider_FaultyRemoveStateAsync() noexcept;

        Awaitable<void> Test_StateProvider_FaultyRemoveStateAsync_RemoveAsync() noexcept;

        Awaitable<void> Test_StateProvider_FaultyGetCurrentState() noexcept;
        
        Awaitable<void> Test_StateProvider_FaultyCopy(__in FaultStateProviderType::FaultAPI faultAPI) noexcept;

        Awaitable<void> Test_StateProvider_FaultyPrepareForRemoveAsync() noexcept;

        Awaitable<void> Test_StateProvider_FaultyCloseAsync(
            __in bool synchronous) noexcept;
    };

    //
    // Goal: Inject fault in the Test State Provider Constructor API. 
    //       The NTSTATUS error code should be returned on AddAsync
    // Algorithm:
    //    1. Inject the fault in the test state provider Constructor
    //    2. Bring up the primary replica
    //    3. Add a state provider, Constructor will be called in this path
    //    4. Verify the returned NTSTATUS error code is the one we expected
    //    5. Clean up and shut down the replica
    //
    Awaitable<void> FaultStateProviderAPIsTest::Test_StateProvider_FaultyConstructor() noexcept
    {
        auto stateProviderName = GetStateProviderName(NameType::Valid);

        // Setup: Inject the fault in the test state provider Constructor
        stateProviderFactorySPtr_->FaultAPI = FaultStateProviderType::Constructor;

        // Setup: Bring up the primary replica
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Test: Add a state provider, Constructor will be called in this path
        // We expect NTSTATUS error code returned here
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });
            NTSTATUS status = co_await stateManagerSPtr->AddAsync(
                *txnSPtr,
                *stateProviderName,
                TestStateProvider::TypeName);
            // Verify the returned NTSTATUS error code is the one we expected
            VERIFY_ARE_EQUAL(status, SF_STATUS_COMMUNICATION_ERROR);
        }

        // Clean up and shut down the replica
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    //
    // Goal: Inject fault in the Test State Provider OpenAsync API. 
    //       The exception should be thrown out, State Manager should not eat the exception.
    // Algorithm:
    //    1. Inject the fault in the test state provider OpenAsync
    //    2. Bring up the primary replica
    //    3. Add a state provider, OpenAsync will be called in this path
    //    4. Verify the exception thrown and it's the one we expected 
    //    5. Clean up and shut down the replica
    //
    Awaitable<void> FaultStateProviderAPIsTest::Test_StateProvider_FaultyOpenAsync() noexcept
    {
        auto stateProviderName = GetStateProviderName(NameType::Valid);

        // Setup: Inject the fault in the test state provider OpenAsync
        stateProviderFactorySPtr_->FaultAPI = FaultStateProviderType::EndOpenAsync;

        // Setup: Bring up the primary replica
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Test: Add a state provider, OpenAsync will be called in this path
        // We expect exception thrown here
        Transaction::SPtr txnSPtr;
        testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
        KFinally([&] { txnSPtr->Dispose(); });
        NTSTATUS status = co_await stateManagerSPtr->AddAsync(
            *txnSPtr,
            *stateProviderName,
            TestStateProvider::TypeName);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        TEST_COMMIT_ASYNC_STATUS(txnSPtr, SF_STATUS_COMMUNICATION_ERROR);

        // Clean up and shut down the replica
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    //
    // Goal: Inject fault in the Test State Provider ChangeRoleAsync API. 
    //       The exception should be thrown out, State Manager should not eat the exception.
    // Algorithm:
    //    1. Inject the fault in the test state provider ChangeRoleAsync
    //    2. Bring up the primary replica
    //    3. Add a state provider, ChangeRoleAsync will be called in this path
    //    4. Verify the exception thrown and it's the one we expected 
    //    5. Clean up and shut down the replica
    //
    Awaitable<void> FaultStateProviderAPIsTest::Test_StateProvider_FaultyChangeRoleAsync_AddPath() noexcept
    {
        auto stateProviderName = GetStateProviderName(NameType::Valid);

        // Setup: Inject the fault in the test state provider ChangeRoleAsync
        stateProviderFactorySPtr_->FaultAPI = FaultStateProviderType::ChangeRoleAsync;

        // Setup: Bring up the primary replica
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Test: Add a state provider, ChangeRoleAsync will be called in this path
        Transaction::SPtr txnSPtr;
        testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
        KFinally([&] { txnSPtr->Dispose(); });
        NTSTATUS status = co_await stateManagerSPtr->AddAsync(
            *txnSPtr,
            *stateProviderName,
            TestStateProvider::TypeName);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        TEST_COMMIT_ASYNC_STATUS(txnSPtr, SF_STATUS_COMMUNICATION_ERROR);

        // Shutdown: We cannot clean up because the ChangeRole to None will throw in state provider. 
        // TODO: Verify that the partition has faulted.
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    //
    // Goal: Inject fault in the Test State Provider ChangeRoleAsync API. 
    //       The exception should be thrown out, State Manager should not eat the exception.
    // Algorithm:
    //    1. Bring up the primary replica
    //    2. Add a state provider
    //    3. Inject the fault in the test state provider ChangeRoleAsync
    //    4. Verify the exception thrown and it's the one we expected 
    //    5. Clean up and shut down the replica
    //
    Awaitable<void> FaultStateProviderAPIsTest::Test_StateProvider_FaultyChangeRoleAsync() noexcept
    {
        bool isThrow = false;
        auto stateProviderName = GetStateProviderName(NameType::Valid);

        // Setup: Bring up the primary replica
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Test: Add a state provider
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });
            NTSTATUS status = co_await stateManagerSPtr->AddAsync(
                *txnSPtr,
                *stateProviderName,
                TestStateProvider::TypeName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            TEST_COMMIT_ASYNC(txnSPtr);
        }

        IStateProvider2::SPtr stateProvider;
        NTSTATUS status = stateManagerSPtr->Get(*stateProviderName, stateProvider);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_NOT_NULL(stateProvider);
        VERIFY_ARE_EQUAL(*stateProviderName, stateProvider->GetName());

        TestStateProvider & testStateProvider = dynamic_cast<TestStateProvider &>(*stateProvider);
        testStateProvider.FaultyAPI = FaultStateProviderType::ChangeRoleAsync;

        try
        {
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        }
        catch (Exception & exception)
        {
            // Verify the exception thrown and it's the one we expected 
            isThrow = true;
            VERIFY_ARE_EQUAL(exception.GetStatus(), SF_STATUS_COMMUNICATION_ERROR);
        }

        VERIFY_IS_TRUE(isThrow);

        // Shut down
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }


    //
    // Goal: Inject fault in the Test State Provider Checkpoint API (PrepareCheckpoint, PerformCheckpoint, CompleteCheckpoint)
    //       The exception should be thrown out, State Manager should not eat the exception.
    // Algorithm:
    //    1. Inject the fault in the test state provider Checkpoint API
    //    2. Bring up the primary replica
    //    3. Add a state provider, Verify the state provider exists
    //    4. Take checkpoint, here the faulty Checkpoint API will be called
    //    5. Verify the exception thrown and it's the one we expected 
    //    6. Clean up and shut down the replica
    //
    Awaitable<void> FaultStateProviderAPIsTest::Test_StateProvider_FaultyCheckpoint(__in FaultStateProviderType::FaultAPI faultAPI) noexcept
    {
        bool isThrow = false;
        auto stateProviderName = GetStateProviderName(NameType::Valid);
        VERIFY_IS_TRUE(
            faultAPI == FaultStateProviderType::PrepareCheckpoint || 
            faultAPI == FaultStateProviderType::PerformCheckpointAsync || 
            faultAPI == FaultStateProviderType::CompleteCheckpointAsync);

        // Setup: Inject the fault in the test state provider Checkpoint API
        stateProviderFactorySPtr_->FaultAPI = faultAPI;

        // Setup: Bring up the primary replica
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Add a state provider
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });
            NTSTATUS status = co_await stateManagerSPtr->AddAsync(
                *txnSPtr,
                *stateProviderName,
                TestStateProvider::TypeName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            TEST_COMMIT_ASYNC(txnSPtr);
        }

        // Verify the state provider exists
        IStateProvider2::SPtr stateProvider;
        NTSTATUS status = testTransactionalReplicatorSPtr_->StateManager->Get(*stateProviderName, stateProvider);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_NOT_NULL(stateProvider);
        VERIFY_ARE_EQUAL(*stateProviderName, stateProvider->GetName());

        try
        {
            // Test: Take checkpoint, here the faulty Checkpoint API will be called
            co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);
        }
        catch (Exception & exception)
        {
            // Verify the exception thrown and it's the one we expected 
            isThrow = true;
            VERIFY_ARE_EQUAL(exception.GetStatus(), SF_STATUS_COMMUNICATION_ERROR);
        }

        VERIFY_IS_TRUE(isThrow);

        // Clean up and shut down the replica
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    //
    // Goal: Inject fault in the Test State Provider RecoverCheckpointAsync API
    //       The exception should be thrown out, State Manager should not eat the exception.
    // Algorithm:
    //    1. Inject the fault in the test state provider RecoverCheckpointAsync API
    //    2. Bring up the primary replica
    //    3. Add a state provider, RecoverCheckpointAsync will be called in this path
    //    4. Verify the exception thrown and it's the one we expected 
    //    5. Clean up and shut down the replica
    //
    Awaitable<void> FaultStateProviderAPIsTest::Test_StateProvider_FaultyRecoverCheckpointAsync() noexcept
    {
        auto stateProviderName = GetStateProviderName(NameType::Valid);

        // Setup: Inject the fault in the test state provider RecoverCheckpointAsync API
        stateProviderFactorySPtr_->FaultAPI = FaultStateProviderType::RecoverCheckpointAsync;

        // Setup: Bring up the primary replica
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Test: Add a state provider, RecoverCheckpointAsync will be called in this path
        Transaction::SPtr txnSPtr;
        testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
        KFinally([&] { txnSPtr->Dispose(); });
        NTSTATUS status = co_await stateManagerSPtr->AddAsync(
            *txnSPtr,
            *stateProviderName,
            TestStateProvider::TypeName);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        TEST_COMMIT_ASYNC_STATUS(txnSPtr, SF_STATUS_COMMUNICATION_ERROR);

        // Clean up and shut down the replica
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    //
    // Goal: Inject fault in the Test State Provider RemoveStateAsync API
    //       The exception should be thrown out, State Manager should not eat the exception.
    // Algorithm:
    //    1. Inject the fault in the test state provider RemoveStateAsync API
    //    2. Bring up the primary replica
    //    3. Add a state provider, Verify the state provider exists
    //    4. Change the replica role to none, the RemoveStateAsync will be called
    //    5. Verify the exception thrown and it's the one we expected 
    //    6. Shut down the replica
    //
    Awaitable<void> FaultStateProviderAPIsTest::Test_StateProvider_FaultyRemoveStateAsync() noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        bool isThrow = false;
        auto stateProviderName = GetStateProviderName(NameType::Valid);

        // Setup: Inject the fault in the test state provider RemoveStateAsync API
        stateProviderFactorySPtr_->FaultAPI = FaultStateProviderType::RemoveStateAsync;

        // Setup: Bring up the primary replica
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Add a state provider
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });
            status = co_await stateManagerSPtr->AddAsync(
                *txnSPtr,
                *stateProviderName,
                TestStateProvider::TypeName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            TEST_COMMIT_ASYNC(txnSPtr);
        }

        // Verify the state provider exists
        IStateProvider2::SPtr stateProvider;
        status = testTransactionalReplicatorSPtr_->StateManager->Get(*stateProviderName, stateProvider);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_NOT_NULL(stateProvider);
        VERIFY_ARE_EQUAL(*stateProviderName, stateProvider->GetName());

        try
        {
            // Test: Change the replica role to none, the RemoveStateAsync will be called
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        }
        catch (Exception & exception)
        {
            // Verify the exception thrown and it's the one we expected 
            isThrow = true;
            VERIFY_ARE_EQUAL(exception.GetStatus(), SF_STATUS_COMMUNICATION_ERROR);
        }

        VERIFY_IS_TRUE(isThrow);

        // Shut down the replica
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    Awaitable<void> FaultStateProviderAPIsTest::Test_StateProvider_FaultyRemoveStateAsync_RemoveAsync() noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        KUri::CSPtr faultyStateProviderName = GetStateProviderName(NameType::Random);

        // Setting up expectations.
        KArray<KUri::CSPtr> goodStateProviderList(GetAllocator(), 256);
        for (ULONG i = 0; i < 256; i++)
        {
            KUri::CSPtr name = GetStateProviderName(NameType::Random);
            status = goodStateProviderList.Append(name);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        // Setup: Bring up the primary replica
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Populate with good state providers
        co_await PopulateAsync(goodStateProviderList);

        // Add the faulty state provider.
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });

            status = co_await stateManagerSPtr->AddAsync(
                *txnSPtr,
                *faultyStateProviderName,
                TestStateProvider::TypeName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            TEST_COMMIT_ASYNC(txnSPtr);
        }

        // Verify the state provider exists. Set the faulty apis correctly.
        {
            IStateProvider2::SPtr faultyStateProvider;
            status = testTransactionalReplicatorSPtr_->StateManager->Get(*faultyStateProviderName, faultyStateProvider);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_NOT_NULL(faultyStateProvider);
            VERIFY_ARE_EQUAL(*faultyStateProviderName, faultyStateProvider->GetName());

            TestStateProvider & faultyTestStateProvider = dynamic_cast<TestStateProvider &>(*faultyStateProvider);
            faultyTestStateProvider.FaultyAPI = FaultStateProviderType::RemoveStateAsync;
        }

        // Test: Step 1: Remove the state provider.
        co_await DepopulateAsync(goodStateProviderList);

        // Test: Step 2: Remove the faulty state provider.
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });

            co_await stateManagerSPtr->RemoveAsync(
                *txnSPtr,
                *faultyStateProviderName);

            TEST_COMMIT_ASYNC(txnSPtr);
        }

        // Test: Step 2: Prepare and perform checkpoint
        FABRIC_SEQUENCE_NUMBER checkpointLSN = co_await testTransactionalReplicatorSPtr_->PrepareCheckpointAsync();
        co_await testTransactionalReplicatorSPtr_->PerformCheckpointAsync(CancellationToken::None);

        // Test: Step 3: Update safe lsn
        testTransactionalReplicatorSPtr_->SetSafeLSN(checkpointLSN);

        // Test: Step 4: Complete the checkpoint (ensure the remove is safe). RemoveStateAsync will throw in clean up.
        try
        {
            // Test: Change the replica role to none, the RemoveStateAsync will be called
            co_await testTransactionalReplicatorSPtr_->CompleteCheckpointAsync(CancellationToken::None);
            CODING_ASSERT("CompleteCheckpointAsync should have failed.");
        }
        catch (Exception & exception)
        {
            // Verify the exception thrown and it's the one we expected 
            CODING_ERROR_ASSERT(exception.GetStatus() == SF_STATUS_COMMUNICATION_ERROR);
        }

        // Shut down the replica. Note that this verifies that the 
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    //
    // Goal: Inject fault in the Test State Provider GetCurrentState API
    //       The exception should be thrown out, State Manager should not eat the exception.
    // Algorithm:
    //    1. Inject the fault in the test state provider GetCurrentState API
    //    2. Bring up the primary replica
    //    3. Add a state provider, Verify the state provider exists
    //    4. Take checkpoint and get copy stream, the GetCurrentState will be called in this path
    //    5. Verify the exception thrown and it's the one we expected 
    //    6. Clean up and shut down the replica
    //
    Awaitable<void> FaultStateProviderAPIsTest::Test_StateProvider_FaultyGetCurrentState() noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        auto stateProviderName = GetStateProviderName(NameType::Valid);

        // Setup: Inject the fault in the test state provider GetCurrentState API
        stateProviderFactorySPtr_->FaultAPI = FaultStateProviderType::GetCurrentState;

        // Setup: Bring up the primary replica
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Add a state provider
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });
            status = co_await stateManagerSPtr->AddAsync(
                *txnSPtr,
                *stateProviderName,
                TestStateProvider::TypeName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            TEST_COMMIT_ASYNC(txnSPtr);
        }

        // Verify the state provider exists
        IStateProvider2::SPtr stateProvider;
        status = testTransactionalReplicatorSPtr_->StateManager->Get(*stateProviderName, stateProvider);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_NOT_NULL(stateProvider);
        VERIFY_ARE_EQUAL(*stateProviderName, stateProvider->GetName());

        co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);

        // Get copy stream, the GetCurrentState will be called in this path
        OperationDataStream::SPtr copyStream = nullptr;
        status = testTransactionalReplicatorSPtr_->StateManager->GetCurrentState(1, copyStream);
        VERIFY_ARE_EQUAL(status, SF_STATUS_COMMUNICATION_ERROR);

        // Clean up and shut down the replica
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    //
    // Goal: Inject fault in the Test State Provider Copy API (BeginSettingCurrentState, SetCurrentStateAsync, EndSettingCurrentStateAsync)
    //       The exception should be thrown out, State Manager should not eat the exception.
    // Algorithm:
    //    1. Inject the fault in the test state provider Copy API
    //    2. Bring up the primary replica
    //    3. Add a state provider, Verify the state provider exists
    //    4. Take checkpoint and get copy stream
    //    5. Clean up and shut down the replica
    //    6. Bring up the secondary replica
    //    7. Clean up and shut down the replica
    //    8. CopyAsync, the Copy API will be called here
    //    9. Verify the exception thrown and it's the one we expected 
    //    10. Clean up and shut down the replica
    //
    Awaitable<void> FaultStateProviderAPIsTest::Test_StateProvider_FaultyCopy(__in FaultStateProviderType::FaultAPI faultAPI) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        bool isThrow = false;
        auto stateProviderName = GetStateProviderName(NameType::Valid);
        VERIFY_IS_TRUE(
            faultAPI == FaultStateProviderType::BeginSettingCurrentState ||
            faultAPI == FaultStateProviderType::SetCurrentStateAsync ||
            faultAPI == FaultStateProviderType::EndSettingCurrentStateAsync);

        // Setup: Inject the fault in the test state provider Copy API
        stateProviderFactorySPtr_->FaultAPI = faultAPI;

        // Setup: Bring up the primary replica
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Add a state provider
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });
            status = co_await stateManagerSPtr->AddAsync(
                *txnSPtr,
                *stateProviderName,
                TestStateProvider::TypeName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            TEST_COMMIT_ASYNC(txnSPtr);
        }

        // Verify the state provider exists
        IStateProvider2::SPtr stateProvider;
        status = testTransactionalReplicatorSPtr_->StateManager->Get(*stateProviderName, stateProvider);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_NOT_NULL(stateProvider);
        VERIFY_ARE_EQUAL(*stateProviderName, stateProvider->GetName());

        // Take checkpoint and get copy stream
        co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);
        OperationDataStream::SPtr copyStream = nullptr;
        status = testTransactionalReplicatorSPtr_->StateManager->GetCurrentState(partitionedReplicaIdCSPtr_->ReplicaId + 1, copyStream);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        // Clean up and shut down the replica
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        {
            // Bring up the secondary replica
            LONG64 replicaId = partitionedReplicaIdCSPtr_->ReplicaId + 1;
            auto partitionedReplicaId = PartitionedReplicaId::Create(partitionedReplicaIdCSPtr_->PartitionId, replicaId, GetAllocator());

            auto runtimeFolders = TestHelper::CreateRuntimeFolders(GetAllocator());
            auto partition = TestHelper::CreateStatefulServicePartition(GetAllocator());
            TestTransactionManager::SPtr secondaryTransactionalReplicator = CreateReplica(*partitionedReplicaId, *runtimeFolders, *partition);

            co_await secondaryTransactionalReplicator->OpenAsync(CancellationToken::None);
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_IDLE_SECONDARY, CancellationToken::None);

            try
            {
                // CopyAsync, the Copy API will be called here
                co_await secondaryTransactionalReplicator->CopyAsync(*copyStream, CancellationToken::None);
            }
            catch (Exception & exception)
            {
                // Verify the exception thrown and it's the one we expected 
                isThrow = true;
                VERIFY_ARE_EQUAL(exception.GetStatus(), SF_STATUS_COMMUNICATION_ERROR);
            }

            VERIFY_IS_TRUE(isThrow);

            // Clean up and shut down the replica
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            co_await secondaryTransactionalReplicator->CloseAsync(CancellationToken::None);
        }

        co_return;
    }

    //
    // Goal: Inject fault in the Test State Provider PrepareForRemoveAsync API
    //       The exception should be thrown out, State Manager should not eat the exception.
    // Algorithm:
    //    1. Inject the fault in the test state provider PrepareForRemoveAsync API
    //    2. Bring up the primary replica
    //    3. Add a state provider, Verify the state provider exists
    //    4. Remove the state provider, the PrepareForRemoveAsync will be called in this path
    //    5. Verify the exception thrown and it's the one we expected 
    //    6. Clean up and shut down the replica
    //
    Awaitable<void> FaultStateProviderAPIsTest::Test_StateProvider_FaultyPrepareForRemoveAsync() noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        auto stateProviderName = GetStateProviderName(NameType::Valid);

        // Setup: Inject the fault in the test state provider PrepareForRemoveAsync API
        stateProviderFactorySPtr_->FaultAPI = FaultStateProviderType::PrepareForRemoveAsync;

        // Setup: Bring up the primary replica
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Add a state provider
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });
            status = co_await stateManagerSPtr->AddAsync(
                *txnSPtr,
                *stateProviderName,
                TestStateProvider::TypeName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            TEST_COMMIT_ASYNC(txnSPtr);
        }

        // Verify the state provider exists
        IStateProvider2::SPtr stateProvider;
        status = testTransactionalReplicatorSPtr_->StateManager->Get(*stateProviderName, stateProvider);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_NOT_NULL(stateProvider);
        VERIFY_ARE_EQUAL(*stateProviderName, stateProvider->GetName());

        co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);

        {
            // Remove the state provider, the PrepareForRemoveAsync will be called in this path
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });
            status = co_await stateManagerSPtr->RemoveAsync(
                *txnSPtr,
                *stateProviderName);
            VERIFY_ARE_EQUAL(status, SF_STATUS_COMMUNICATION_ERROR);
        }

        // Clean up and shut down the replica
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    //
    // Goal: Inject fault in the Test State Provider RecoverCheckpointAsync API
    //       The exception should be thrown out, State Manager should not eat the exception.
    // Algorithm:
    //    1. Inject the fault in the test state provider RecoverCheckpointAsync API
    //    2. Bring up the primary replica
    //    3. On the recovery path, redo the insert of state provider
    //    4. Verify the exception thrown and it's the one we expected 
    //    5. Clean up and shut down the replica
    //
    Awaitable<void> FaultStateProviderAPIsTest::Test_StateProvider_FaultyRecoverCheckpointAsync_RecoveryPath() noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        auto stateProviderName = GetStateProviderName(NameType::Valid);

        // Setup: Inject the fault in the test state provider RecoverCheckpointAsync API
        stateProviderFactorySPtr_->FaultAPI = FaultStateProviderType::RecoverCheckpointAsync;

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);

        const LONG64 transactionId = 1;
        const LONG64 stateProviderId = 1;
        auto txn = TransactionBase::CreateTransaction(transactionId, false, GetAllocator());
        auto namedMetadataOperationData = GenerateNamedMetadataOperationData(stateProviderId, *stateProviderName, ApplyType::Insert);
        RedoOperationData::CSPtr redoOperationData = GenerateRedoOperationData(TestStateProvider::TypeName, nullptr);


        TxnReplicator::OperationContext::CSPtr operationContext = nullptr;
        status = co_await testTransactionalReplicatorSPtr_->StateManager->ApplyAsync(
            0,
            *txn,
            ApplyContext::RecoveryRedo,
            namedMetadataOperationData.RawPtr(),
            redoOperationData.RawPtr(),
            operationContext);
        VERIFY_ARE_EQUAL(status, SF_STATUS_COMMUNICATION_ERROR);

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    //
    // Goal: Inject fault in the Test State Provider CloseAsync API
    //       The exception should be handled and state provider aborted.
    //       State Manager's close must not fail.
    // Algorithm:
    //    1. Inject the fault in the test state provider PrepareForRemoveAsync API
    //    2. Bring up the primary replica
    //    3. Add a state provider, Verify the state provider exists
    //    4. Close the replicator (CloseAsync will be called on all state providers)
    //    5. Verify the close was successful
    //
    Awaitable<void> FaultStateProviderAPIsTest::Test_StateProvider_FaultyCloseAsync(
        __in bool synchronous) noexcept
    {
        auto stateProviderName = GetStateProviderName(NameType::Valid);

        // Setup: Inject the fault in the test state provider PrepareForRemoveAsync API
        stateProviderFactorySPtr_->FaultAPI = synchronous ? FaultStateProviderType::BeginCloseAsync : FaultStateProviderType::EndCloseAsync;

        // Setup: Bring up the primary replica
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Add a state provider
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });
            NTSTATUS status = co_await stateManagerSPtr->AddAsync(
                *txnSPtr,
                *stateProviderName,
                TestStateProvider::TypeName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            TEST_COMMIT_ASYNC(txnSPtr);
        }

        // Clean up and shut down the replica
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    BOOST_FIXTURE_TEST_SUITE(FaultStateProviderAPIsTestSuite, FaultStateProviderAPIsTest)

    //
    // Scenario:        Inject faulty in the Test State Provider Constructor API
    // Expected Result: The NTSTATUS error code should be returned on AddAsync
    // 
    BOOST_AUTO_TEST_CASE(StateProvider_FaultyConstructor)
    {
        SyncAwait(this->Test_StateProvider_FaultyConstructor());
    }

    //
    // Scenario:        Inject faulty in the Test State Provider OpenAsync API
    // Expected Result: The exception should be thrown out, State Manager should not eat the exception.
    // 
    BOOST_AUTO_TEST_CASE(StateProvider_FaultyOpenAsync_THROW)
    {
        SyncAwait(this->Test_StateProvider_FaultyOpenAsync());
    }

    //
    // Scenario:        Inject faulty in the Test State Provider ChangeRoleAsync API
    // Expected Result: The exception should be thrown out, State Manager should not eat the exception.
    // 
    BOOST_AUTO_TEST_CASE(StateProvider_FaultyChangeRoleAsync_AddPath_THROW)
    {
        SyncAwait(this->Test_StateProvider_FaultyChangeRoleAsync_AddPath());
    }

    //
    // Scenario:        Inject faulty in the Test State Provider ChangeRoleAsync API
    // Expected Result: The exception should be thrown out, State Manager should not eat the exception.
    // 
    BOOST_AUTO_TEST_CASE(StateProvider_FaultyChangeRoleAsync_THROW)
    {
        SyncAwait(this->Test_StateProvider_FaultyChangeRoleAsync());
    }

    //
    // Scenario:        Inject faulty in the Test State Provider PrepareCheckpoint API
    // Expected Result: The exception should be thrown out, State Manager should not eat the exception.
    // 
    BOOST_AUTO_TEST_CASE(StateProvider_FaultyPrepareCheckpoint_THROW)
    {
        SyncAwait(this->Test_StateProvider_FaultyCheckpoint(FaultStateProviderType::PrepareCheckpoint));
    }

    //
    // Scenario:        Inject faulty in the Test State Provider PerformCheckpointAsync API
    // Expected Result: The exception should be thrown out, State Manager should not eat the exception.
    // 
    BOOST_AUTO_TEST_CASE(StateProvider_FaultyPerformCheckpointAsync_THROW)
    {
        SyncAwait(this->Test_StateProvider_FaultyCheckpoint(FaultStateProviderType::PerformCheckpointAsync));
    }

    //
    // Scenario:        Inject faulty in the Test State Provider CompleteCheckpointAsync API
    // Expected Result: The exception should be thrown out, State Manager should not eat the exception.
    // 
    BOOST_AUTO_TEST_CASE(StateProvider_FaultyCompleteCheckpointAsync_THROW)
    {
        SyncAwait(this->Test_StateProvider_FaultyCheckpoint(FaultStateProviderType::CompleteCheckpointAsync));
    }

    //
    // Scenario:        Inject faulty in the Test State Provider RecoverCheckpointAsync API
    // Expected Result: The exception should be thrown out, State Manager should not eat the exception.
    // 
    BOOST_AUTO_TEST_CASE(StateProvider_FaultyRecoverCheckpointAsync_THROW)
    {
        SyncAwait(this->Test_StateProvider_FaultyRecoverCheckpointAsync());
    }

    //
    // Scenario:        Inject faulty in the Test State Provider RecoverCheckpointAsync API
    // Expected Result: The exception should be thrown out, State Manager should not eat the exception.
    // 
    BOOST_AUTO_TEST_CASE(StateProvider_FaultyRecoverCheckpointAsync_RecoveryPath_THROW)
    {
        SyncAwait(this->Test_StateProvider_FaultyRecoverCheckpointAsync_RecoveryPath());
    }

    //
    // Scenario:        Inject faulty in the Test State Provider RemoveStateAsync API
    // Expected Result: The exception should be thrown out, State Manager should not eat the exception.
    // 
    BOOST_AUTO_TEST_CASE(StateProvider_FaultyRemoveStateAsync_THROW)
    {
        SyncAwait(this->Test_StateProvider_FaultyRemoveStateAsync());
    }

    //
    // Scenario:        Inject faulty in the Test State Provider RemoveStateAsync API
    // Expected Result: The exception should be thrown out, State Manager should not eat the exception.
    // 
    BOOST_AUTO_TEST_CASE(StateProvider_FaultyRemoveStateAsync__RemoveAsync_THROW)
    {
        SyncAwait(this->Test_StateProvider_FaultyRemoveStateAsync_RemoveAsync());
    }

    //
    // Scenario:        Inject faulty in the Test State Provider GetCurrentState API
    // Expected Result: The exception should be thrown out, State Manager should not eat the exception.
    // 
    BOOST_AUTO_TEST_CASE(StateProvider_FaultyGetCurrentState_THROW)
    {
        SyncAwait(this->Test_StateProvider_FaultyGetCurrentState());
    }

    //
    // Scenario:        Inject faulty in the Test State Provider BeginSettingCurrentState API
    // Expected Result: The exception should be thrown out, State Manager should not eat the exception.
    // 
    BOOST_AUTO_TEST_CASE(StateProvider_FaultyBeginSettingCurrentState_THROW)
    {
        SyncAwait(this->Test_StateProvider_FaultyCopy(FaultStateProviderType::BeginSettingCurrentState));
    }

    //
    // Scenario:        Inject faulty in the Test State Provider SetCurrentStateAsync API
    // Expected Result: The exception should be thrown out, State Manager should not eat the exception.
    // 
    BOOST_AUTO_TEST_CASE(StateProvider_FaultySetCurrentStateAsync_THROW)
    {
        SyncAwait(this->Test_StateProvider_FaultyCopy(FaultStateProviderType::SetCurrentStateAsync));
    }

    //
    // Scenario:        Inject faulty in the Test State Provider EndSettingCurrentStateAsync API
    // Expected Result: The exception should be thrown out, State Manager should not eat the exception.
    // 
    BOOST_AUTO_TEST_CASE(StateProvider_FaultyEndSettingCurrentStateAsync_THROW)
    {
        SyncAwait(this->Test_StateProvider_FaultyCopy(FaultStateProviderType::EndSettingCurrentStateAsync));
    }

    //
    // Scenario:        Inject faulty in the Test State Provider PrepareForRemoveAsync API
    // Expected Result: The exception should be thrown out, State Manager should not eat the exception.
    // 
    BOOST_AUTO_TEST_CASE(StateProvider_FaultyPrepareForRemoveAsync_THROW)
    {
        SyncAwait(this->Test_StateProvider_FaultyPrepareForRemoveAsync());
    }

    //
    // Scenario:        Inject fault in the Test State Provider CloseAsync synchronous path
    // Expected Result: The exception should be thrown out, State Manager handle the exception and abort the state provider.
    // 
    BOOST_AUTO_TEST_CASE(Test_StateProvider_FaultyCloseAsync_Sync)
    {
        SyncAwait(this->Test_StateProvider_FaultyCloseAsync(true));
    }

    //
    // Scenario:        Inject fault in the Test State Provider CloseAsync asynchronous path
    // Expected Result: The exception should be thrown out, State Manager handle the exception and abort the state provider.
    // 
    BOOST_AUTO_TEST_CASE(Test_StateProvider_FaultyCloseAsync_Async)
    {
        SyncAwait(this->Test_StateProvider_FaultyCloseAsync(false));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
