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
    class StateManagerFunctionalTest : public StateManagerTestBase
    {
        // load the config object as its needed for the tracing to work
        CommonConfig config;

    public:
        StateManagerFunctionalTest()
        {
        }

        ~StateManagerFunctionalTest()
        {
        }

    public:
        Awaitable<void> Test_ConflictCreate_On_Same_StateProvider(
            __in ULONG taskNum) noexcept;

        Awaitable<void> Test_ConflictDelete_On_Same_StateProvider(
            __in ULONG taskNum) noexcept;

        Awaitable<void> Test_Create_FollowedBy_CreateOrDelete_SameTxn(
            __in bool isCreate) noexcept;

        Awaitable<void> Test_Delete_FollowedBy_CreateOrDelete_SameTxn(
            __in bool isCreate) noexcept;

        Awaitable<void> Test_Create_Delete_Recreate_Delete_StateProvider() noexcept;

    private:
        Awaitable<void> CreateOrDeleteStateProviderTask(
            __in AwaitableCompletionSource<bool> & signalCompletion,
            __in KUriView const & stateProviderName,
            __in bool isCreate,
            __in ULONG taskId,
            __in ConcurrentDictionary<ULONG, bool> & dict) noexcept;
    };

    //
    // Goal: Concurrent Create State Provider tasks on the same name state provider
    //       The state provider is successfully added, the later call should return
    //       name already exists NTSTATUS error code
    // Algorithm:
    //    1. Bring up the primary replica, and verify the state provider does not exist
    //    2. Create the CreateStateProviderTask Task, run them concurrently 
    //    3. Verify the state provider is successfully added, the later call should return
    //       name already exists NTSTATUS error code
    //    4. Verify the NTSTATUS error code is returned and state provider is added
    //    5. Clean up and Shut down
    //
    Awaitable<void> StateManagerFunctionalTest::Test_ConflictCreate_On_Same_StateProvider(
        __in ULONG taskNum) noexcept
    {
        KUri::CSPtr stateProviderName = GetStateProviderName(NameType::Valid);

        // Setup: Bring up the primary replica
        co_await (testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None));
        co_await (testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None));
        IStateManager::SPtr stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Verify the state provider does not exist
        IStateProvider2::SPtr outStateProvider;
        NTSTATUS status = stateManagerSPtr->Get(*stateProviderName, outStateProvider);
        VERIFY_ARE_EQUAL(status, SF_STATUS_NAME_DOES_NOT_EXIST);
        VERIFY_IS_NULL(outStateProvider);

        ConcurrentDictionary<ULONG, bool>::SPtr resultDict;
        status = ConcurrentDictionary<ULONG, bool>::Create(
            GetAllocator(),
            resultDict);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        KArray<Awaitable<void>> taskArray = KArray<Awaitable<void>>(GetAllocator(), taskNum);

        AwaitableCompletionSource<bool>::SPtr signalCompletion = nullptr;
        status = AwaitableCompletionSource<bool>::Create(GetAllocator(), 0, signalCompletion);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        for (ULONG i = 0; i < taskNum; i++)
        {
            status = taskArray.Append(CreateOrDeleteStateProviderTask(*signalCompletion, *stateProviderName, true, i, *resultDict));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        // Run the tasks concurrently, and at the same time
        signalCompletion->SetResult(true);

        co_await (TaskUtilities<void>::WhenAll(taskArray));

        status = stateManagerSPtr->Get(*stateProviderName, outStateProvider);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_NOT_NULL(outStateProvider);
        VERIFY_ARE_EQUAL(*stateProviderName, outStateProvider->GetName());

        // Verify all task finished and only one task finished without NTSTATUS error code returned
        VERIFY_IS_TRUE(resultDict->Count == taskNum);

        ULONG falseCount = 0;
        auto enumerator = resultDict->GetEnumerator();
        while (enumerator->MoveNext())
        {
            if (enumerator->Current().Value == false)
            {
                falseCount++;
            }
        }

        VERIFY_IS_TRUE(falseCount == 1);


        // Clean up and Shutdown
        co_await (testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None));
        co_await (testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None));

        co_return;
    }

    //
    // Goal: Concurrent Delete State Provider tasks on the same name state provider
    //       The state provider is successfully deleted, the later call should return
    //       invalid parameter NTSTATUS error code
    // Algorithm:
    //    1. Bring up the primary replica, add a state provider and verify the it exists
    //    2. Create the DeleteStateProviderTask Task, run them concurrently 
    //    3. Verify the state provider is successfully deleted, the later call should return
    //       invalid parameter NTSTATUS error code
    //    4. Verify the NTSTATUS error code is returned and state provider is deleted
    //    5. Clean up and Shut down
    //
    Awaitable<void> StateManagerFunctionalTest::Test_ConflictDelete_On_Same_StateProvider(
        __in ULONG taskNum) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        KUri::CSPtr stateProviderName = GetStateProviderName(NameType::Valid);

        // Setup: Bring up the primary replica
        co_await (testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None));
        co_await (testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None));
        IStateManager::SPtr stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

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
        IStateProvider2::SPtr outStateProvider;
        status = stateManagerSPtr->Get(*stateProviderName, outStateProvider);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_NOT_NULL(outStateProvider);
        VERIFY_ARE_EQUAL(*stateProviderName, outStateProvider->GetName());

        ConcurrentDictionary<ULONG, bool>::SPtr resultDict;
        status = ConcurrentDictionary<ULONG, bool>::Create(
            GetAllocator(),
            resultDict);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        KArray<Awaitable<void>> taskArray = KArray<Awaitable<void>>(GetAllocator(), taskNum);

        AwaitableCompletionSource<bool>::SPtr signalCompletion = nullptr;
        status = AwaitableCompletionSource<bool>::Create(GetAllocator(), 0, signalCompletion);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        // Create the DeleteStateProviderTask Task, run them concurrently 
        for (ULONG i = 0; i < taskNum; i++)
        {
            status = taskArray.Append(CreateOrDeleteStateProviderTask(*signalCompletion, *stateProviderName, false, i, *resultDict));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        // Run the tasks concurrently, and at the same time
        signalCompletion->SetResult(true);

        co_await (TaskUtilities<void>::WhenAll(taskArray));

        // Verify the state provider is successfully deleted
        status = stateManagerSPtr->Get(*stateProviderName, outStateProvider);
        VERIFY_ARE_EQUAL(status, SF_STATUS_NAME_DOES_NOT_EXIST);
        VERIFY_IS_NULL(outStateProvider);

        // Verify all task finished and only one task finished without NTSTATUS error code returned
        VERIFY_IS_TRUE(resultDict->Count == taskNum);

        ULONG falseCount = 0;
        auto enumerator = resultDict->GetEnumerator();
        while (enumerator->MoveNext())
        {
            if (enumerator->Current().Value == false)
            {
                falseCount++;
            }
        }

        VERIFY_IS_TRUE(falseCount == 1);

        // Clean up and Shutdown
        co_await (testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None));
        co_await (testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None));

        co_return;
    }

    //
    // Goal: Create State Provider follow Create or Delete in the same txn
    //       The second operation call, create or delete, should fail and return NTSTATUS
    //       error code
    // Algorithm:
    //    1. Bring up the primary replica
    //    2. Add a state provider, in the same txn, add or delete the state provider
    //    3. Verify the second returned NTSTATUS error code
    //    5. Clean up and Shut down
    //
    Awaitable<void> StateManagerFunctionalTest::Test_Create_FollowedBy_CreateOrDelete_SameTxn(
        __in bool isCreate) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        KUri::CSPtr stateProviderName = GetStateProviderName(NameType::Valid);

        // Setup: Bring up the primary replica
        co_await (testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None));
        co_await (testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None));
        IStateManager::SPtr stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

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

            // In the same txn, add or remove the state provider
            if (isCreate)
            {
                status = co_await stateManagerSPtr->AddAsync(
                    *txnSPtr,
                    *stateProviderName,
                    TestStateProvider::TypeName);
                VERIFY_ARE_EQUAL(status, SF_STATUS_NAME_ALREADY_EXISTS);
            }
            else
            {
                status = co_await stateManagerSPtr->RemoveAsync(
                        *txnSPtr,
                        *stateProviderName);
                VERIFY_ARE_EQUAL(status, SF_STATUS_NAME_DOES_NOT_EXIST);
            }
        }

        IStateProvider2::SPtr outStateProvider;
        status = stateManagerSPtr->Get(*stateProviderName, outStateProvider);
        VERIFY_ARE_EQUAL(status, SF_STATUS_NAME_DOES_NOT_EXIST);
        VERIFY_IS_NULL(outStateProvider);

        // Clean up and Shutdown
        co_await (testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None));
        co_await (testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None));

        co_return;
    }

    //
    // Goal: Delete State Provider follow Create or Delete in the same txn
    //       The second operation call, create or delete, should fail and return NTSTATUS
    //       error code
    // Algorithm:
    //    1. Bring up the primary replica
    //    2. Add a state provider
    //    3. delete the state provider, in the same txn, add or delete the state provider
    //    4. Verify the second returned NTSTATUS error code and state provider still exists
    //    5. Clean up and Shut down
    //
    Awaitable<void> StateManagerFunctionalTest::Test_Delete_FollowedBy_CreateOrDelete_SameTxn(
        __in bool isCreate) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        KUri::CSPtr stateProviderName = GetStateProviderName(NameType::Valid);

        // Setup: Bring up the primary replica
        co_await (testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None));
        co_await (testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None));
        IStateManager::SPtr stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

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
        IStateProvider2::SPtr outStateProvider;
        status = stateManagerSPtr->Get(*stateProviderName, outStateProvider);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_NOT_NULL(outStateProvider);
        VERIFY_ARE_EQUAL(*stateProviderName, outStateProvider->GetName());

        // Delete a state provider
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });
            status = co_await stateManagerSPtr->RemoveAsync(
                *txnSPtr,
                *stateProviderName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            // In the same txn, add or remove the state provider
            if (isCreate)
            {
                status = co_await stateManagerSPtr->AddAsync(
                    *txnSPtr,
                    *stateProviderName,
                    TestStateProvider::TypeName);
                VERIFY_ARE_EQUAL(status, SF_STATUS_NAME_ALREADY_EXISTS);
            }
            else
            {
                status = co_await stateManagerSPtr->RemoveAsync(
                        *txnSPtr,
                        *stateProviderName);
                VERIFY_ARE_EQUAL(status, SF_STATUS_NAME_DOES_NOT_EXIST);
            }
        }

        // Verify the state provider is still there
        status = stateManagerSPtr->Get(*stateProviderName, outStateProvider);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_NOT_NULL(outStateProvider);
        VERIFY_ARE_EQUAL(*stateProviderName, outStateProvider->GetName());

        // Clean up and Shutdown
        co_await (testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None));
        co_await (testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None));

        co_return;
    }

    //
    // Goal: Create - Deleted -Recreate - Deleted state provider
    //       The state provider is successfully added or deleted for each operation.
    // Algorithm:
    //    1. Bring up the primary replica
    //    2. Add a state provider, verify the state provider is added
    //    3. Delete the state provider, verify the state provider is deleted
    //    4. Recreate a state provider, verify the state provider is added
    //    5. Delete the state provider, verify the state provider is deleted
    //    6. Clean up and Shut down
    //
    Awaitable<void> StateManagerFunctionalTest::Test_Create_Delete_Recreate_Delete_StateProvider() noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        KUri::CSPtr stateProviderName = GetStateProviderName(NameType::Valid);

        // Setup: Bring up the primary replica
        co_await (testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None));
        co_await (testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None));
        IStateManager::SPtr stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

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
        IStateProvider2::SPtr outStateProvider;
        status = stateManagerSPtr->Get(*stateProviderName, outStateProvider);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_NOT_NULL(outStateProvider);
        VERIFY_ARE_EQUAL(*stateProviderName, outStateProvider->GetName());

        // Delete a state provider
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });
            status = co_await stateManagerSPtr->RemoveAsync(
                *txnSPtr,
                *stateProviderName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            TEST_COMMIT_ASYNC(txnSPtr);
        }

        // Verify the state provider is deleted
        status = stateManagerSPtr->Get(*stateProviderName, outStateProvider);
        VERIFY_ARE_EQUAL(status, SF_STATUS_NAME_DOES_NOT_EXIST);
        VERIFY_IS_NULL(outStateProvider);

        // recreate the state provider
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
        status = stateManagerSPtr->Get(*stateProviderName, outStateProvider);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_NOT_NULL(outStateProvider);
        VERIFY_ARE_EQUAL(*stateProviderName, outStateProvider->GetName());

        // Delete the state provider
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });
            status = co_await stateManagerSPtr->RemoveAsync(
                *txnSPtr,
                *stateProviderName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            TEST_COMMIT_ASYNC(txnSPtr);
        }

        // Verify the state provider is deleted
        status = stateManagerSPtr->Get(*stateProviderName, outStateProvider);
        VERIFY_ARE_EQUAL(status, SF_STATUS_NAME_DOES_NOT_EXIST);
        VERIFY_IS_NULL(outStateProvider);

        // Clean up and Shutdown
        co_await (testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None));
        co_await (testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None));

        co_return;
    }

    // Task: Add state provider to the state manager or delete a state provider from state manager
    Awaitable<void> StateManagerFunctionalTest::CreateOrDeleteStateProviderTask(
        __in AwaitableCompletionSource<bool> & signalCompletion,
        __in KUriView const & stateProviderName,
        __in bool isCreate,
        __in ULONG taskId,
        __in ConcurrentDictionary<ULONG, bool> & dict) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        AwaitableCompletionSource<bool>::SPtr tempCompletion(&signalCompletion);
        co_await tempCompletion->GetAwaitable();

        bool isNTSTATUSErrorCodeReturned = false;
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });

            if (isCreate)
            {
                status = co_await stateManagerSPtr->AddAsync(
                    *txnSPtr,
                    stateProviderName,
                    TestStateProvider::TypeName);
            }
            else
            {
                status = co_await stateManagerSPtr->RemoveAsync(
                    *txnSPtr,
                    stateProviderName);
            }

            if (NT_SUCCESS(status))
            {
                TEST_COMMIT_ASYNC(txnSPtr);
            }
            else
            {
                isNTSTATUSErrorCodeReturned = true;
                VERIFY_ARE_EQUAL(status, (isCreate ? SF_STATUS_NAME_ALREADY_EXISTS : SF_STATUS_NAME_DOES_NOT_EXIST));
            }
        }

        dict.Add(taskId, isNTSTATUSErrorCodeReturned);

        co_return;
    }

    BOOST_FIXTURE_TEST_SUITE(StateManagerFunctionalTestSuite, StateManagerFunctionalTest)
    
    //
    // Scenario:        Concurrent Create State Provider tasks on the same name conflict
    // Expected Result: The state provider is successfully added, the later call should return
    //                  Name already exists NTSTATUS error code
    // 
    BOOST_AUTO_TEST_CASE(ConflictCreate_On_Same_StateProvider)
    {
        SyncAwait(this->Test_ConflictCreate_On_Same_StateProvider(8));
    }

    //
    // Scenario:        Concurrent delete State Provider tasks on the same name conflict
    // Expected Result: The state provider is successfully deleted, the later call should return
    //                  invalid parameter NTSTATUS error code
    // 
    BOOST_AUTO_TEST_CASE(ConflictDelete_On_Same_StateProvider)
    {
        SyncAwait(this->Test_ConflictDelete_On_Same_StateProvider(8));
    }

    //
    // Scenario:        Create State Provider follow by another create in the same txn
    // Expected Result: The later Create state provider will return NTSTATUS error code
    //                  SF_STATUS_NAME_ALREADY_EXISTS indicates the second call failed
    // 
    BOOST_AUTO_TEST_CASE(Create_FollowedBy_Create_SameTxn)
    {
        SyncAwait(this->Test_Create_FollowedBy_CreateOrDelete_SameTxn(true));
    }

    //
    // Scenario:        Create State Provider follow by delete in the same txn
    // Expected Result: The later Delete state provider will return NTSTATUS error code SF_STATUS_NAME_DOES_NOT_EXIST
    //                  indicates the second call failed, since the add operation has not been applied 
    // 
    BOOST_AUTO_TEST_CASE(Create_FollowedBy_Delete_SameTxn)
    {
        SyncAwait(this->Test_Create_FollowedBy_CreateOrDelete_SameTxn(false));
    }

    //
    // Scenario:        Delete State Provider follow by create in the same txn
    // Expected Result: The later create state provider will return NTSTATUS error code SF_STATUS_NAME_ALREADY_EXISTS
    //                  indicates the second call failed, since it is still there but TransientDelete
    // 
    BOOST_AUTO_TEST_CASE(Delete_FollowedBy_Delete_SameTxn)
    {
        SyncAwait(this->Test_Delete_FollowedBy_CreateOrDelete_SameTxn(true));
    }

    //
    // Scenario:        Delete State Provider follow by delete in the same txn
    // Expected Result: The later create state provider will return NTSTATUS error code SF_STATUS_NAME_DOES_NOT_EXIST
    //                  indicates the second call failed, since it is TransientDelete now
    // 
    BOOST_AUTO_TEST_CASE(Delete_FollowedBy_Create_SameTxn)
    {
        SyncAwait(this->Test_Delete_FollowedBy_CreateOrDelete_SameTxn(false));
    }

    //
    // Scenario:        Create - Deleted -Recreate - Deleted state provider
    // Expected Result: The state provider is successfully added or deleted for each
    //                  operation.
    // 
    BOOST_AUTO_TEST_CASE(Create_Delete_Recreate_Delete_StateProvider)
    {
        SyncAwait(this->Test_Create_Delete_Recreate_Delete_StateProvider());
    }

    BOOST_AUTO_TEST_SUITE_END()
}
