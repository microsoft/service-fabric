// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <stdlib.h>
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace Common;
using namespace std;
using namespace ktl;
using namespace TxnReplicator;
using namespace Data::StateManager;
using namespace Data::Utilities;

namespace StateManagerTests
{
    class StateManagerHierarchyTest : public StateManagerTestBase
    {
        // load the config object as its needed for the tracing to work
        CommonConfig config;

    public:
        Awaitable<void> Test_Add_StateProviders_WithHierarchy_Commit_Success(
            __in ULONG parentCount,
            __in ULONG childrenCount) noexcept;

        Awaitable<void> Test_Add_Remove_StateProviders_WithHierarchy_Commit_Success(
            __in ULONG parentCount,
            __in ULONG childrenCount) noexcept;

        Awaitable<void> Test_Remove_StateProvider_WithNonExistChild(__in ULONG childrenCount) noexcept;

        Awaitable<void> Test_Add_StateProvider_WithHierarchy_Abort_Success(
            __in bool useDispose, 
            __in ULONG childrenCount) noexcept;

        Awaitable<void> Test_GetOrAdd_ExistStateProviders_Commit_Success(
            __in bool isParent) noexcept;

        Awaitable<void> Test_GetOrAdd_NotExist_WithHierarchy_Commit_Success(
            __in ULONG childrenCount) noexcept;

        Awaitable<void> Test_Recover_Checkpoint_WithHierarchy_Success(
            __in ULONG parentCount, 
            __in ULONG childrenCount) noexcept;

        Awaitable<void> StateManagerHierarchyTest::Test_Copy_MultipleStateProvider_WithHierarchy_AllActiveCopied(
            __in ULONG parentCount,
            __in ULONG childrenCount,
            __in bool checkpointAfterAdd,
            __in bool recoverInPlace) noexcept;

        Awaitable<void> Test_RecoveryPath_Checkpointless_AllItemsAdded(
            __in ULONG parentCount,
            __in ULONG childrenCount) noexcept;

        Awaitable<void> Test_CreateEnumerable(
            __in bool parentOnly) noexcept;
    };

    Awaitable<void> StateManagerHierarchyTest::Test_Add_StateProvider_WithHierarchy_Abort_Success(
        __in bool useDispose, 
        __in ULONG childrenCount) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        // Input variables
        auto stateProviderName = GetStateProviderName(NameType::Hierarchy, 0, 0);
        OperationData::SPtr initParameters;

        try
        {
            initParameters = TestStateProvider::CreateInitializationParameters(childrenCount, GetAllocator());
        }
        catch (Exception & exception)
        {
            VERIFY_ARE_EQUAL(exception.GetStatus(), STATUS_UNSUCCESSFUL);
            VERIFY_IS_TRUE(false);
        }

        // Setup Test.
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Test
        Transaction::SPtr txnSPtr;
        testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
        status = co_await stateManagerSPtr->AddAsync(
            *txnSPtr,
            *stateProviderName,
            TestStateProvider::TypeName,
            initParameters.RawPtr());
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        if (!useDispose)
        {
            txnSPtr->Abort();
        }

        txnSPtr->Dispose();

        // Verification
        IStateProvider2::SPtr outStateProvider;
        status = stateManagerSPtr->Get(*stateProviderName, outStateProvider);
        VERIFY_ARE_EQUAL(status, SF_STATUS_NAME_DOES_NOT_EXIST);
        VERIFY_IS_NULL(outStateProvider);

        for (ULONG i = 1; i <= childrenCount; i++)
        {
            auto childrenStateProviderName = GetStateProviderName(NameType::Hierarchy, 0, i);

            status = stateManagerSPtr->Get(*childrenStateProviderName, outStateProvider);
            VERIFY_ARE_EQUAL(status, SF_STATUS_NAME_DOES_NOT_EXIST);
            VERIFY_IS_NULL(outStateProvider);
        }

        // Clean up.
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);

        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    Awaitable<void> StateManagerHierarchyTest::Test_Add_StateProviders_WithHierarchy_Commit_Success(
        __in ULONG parentCount,
        __in ULONG childrenCount) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        KArray<KUri::CSPtr> parentNames(GetAllocator());
        for(ULONG i = 0;i < parentCount; i++)
        {
            auto stateProviderName = GetStateProviderName(NameType::Hierarchy, i, 0);
            status = parentNames.Append(stateProviderName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        OperationData::SPtr initParameters;

        try
        {
            initParameters = TestStateProvider::CreateInitializationParameters(childrenCount, GetAllocator());
        }
        catch (Exception & exception)
        {
            VERIFY_ARE_EQUAL(exception.GetStatus(), STATUS_UNSUCCESSFUL);
            VERIFY_IS_TRUE(false);
        }

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Test
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });

            for (ULONG i = 0; i < parentNames.Count(); i++)
            {
                status = co_await stateManagerSPtr->AddAsync(
                    *txnSPtr,
                    *(parentNames[i]),
                    TestStateProvider::TypeName,
                    initParameters.RawPtr());
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            TEST_COMMIT_ASYNC(txnSPtr);
        }

        // Verification
        IStateProvider2::SPtr outStateProvider;
        for (ULONG i = 0; i < parentNames.Count(); i++)
        {
            status = stateManagerSPtr->Get(*(parentNames[i]), outStateProvider);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_NOT_NULL(outStateProvider);
            VERIFY_ARE_EQUAL(*(parentNames[i]), outStateProvider->GetName());

            for (ULONG j = 1; j <= childrenCount; j++)
            {
                auto childrenStateProviderName = GetStateProviderName(NameType::Hierarchy, i, j);

                status = stateManagerSPtr->Get(*childrenStateProviderName, outStateProvider);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                VERIFY_IS_NOT_NULL(outStateProvider);
                VERIFY_ARE_EQUAL(*childrenStateProviderName, outStateProvider->GetName());

                TestStateProvider & childStateProvider = dynamic_cast<TestStateProvider &>(*outStateProvider);
                VERIFY_IS_NULL(childStateProvider.GetInitializationParameters());
            }
        }

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    Awaitable<void> StateManagerHierarchyTest::Test_CreateEnumerable(
        __in bool parentOnly) noexcept
    {
        const ULONG parentCount = 8;
        const ULONG childrenCount = 4;
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        KArray<KUri::CSPtr> parentNames(GetAllocator());
        KArray<KUri::CSPtr> childrenNames(GetAllocator());

        for (ULONG i = 0; i < parentCount; i++)
        {
            auto stateProviderName = GetStateProviderName(NameType::Hierarchy, i, 0);
            status = parentNames.Append(stateProviderName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

                if (!parentOnly)
                {
                    for (ULONG j = 1; j <= childrenCount; j++)
                    {
                        auto childrenStateProviderName = GetStateProviderName(NameType::Hierarchy, i, j);
                        status = childrenNames.Append(childrenStateProviderName);
                        VERIFY_IS_TRUE(NT_SUCCESS(status));
                    }
                }
        }

        OperationData::SPtr initParameters;

        try
        {
            initParameters = TestStateProvider::CreateInitializationParameters(childrenCount, GetAllocator());
        }
        catch (Exception & exception)
        {
            VERIFY_ARE_EQUAL(exception.GetStatus(), STATUS_UNSUCCESSFUL);
            VERIFY_IS_TRUE(false);
        }

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Test
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] {txnSPtr->Dispose(); });
            
            for (ULONG i = 0; i < parentNames.Count(); i++)
            {
                status = co_await stateManagerSPtr->AddAsync(
                    *txnSPtr,
                    *(parentNames[i]),
                    TestStateProvider::TypeName,
                    initParameters.RawPtr());
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            TEST_COMMIT_ASYNC(txnSPtr);
        }

        // Verification
        KSharedPtr<Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, KSharedPtr<TxnReplicator::IStateProvider2>>>> enumerator;
        status = stateManagerSPtr->CreateEnumerator(parentOnly, enumerator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        ULONG count = 0;
        while (enumerator->MoveNext())
        {
            bool findTheStateProvider = false;
            for (ULONG i = 0; i < parentNames.Count(); i++)
            {
                if (*(parentNames[i]) == *enumerator->Current().Key)
                {
                    findTheStateProvider = true;
                    break;
                }
            }

            if (!parentOnly && findTheStateProvider == false)
            {
                for (ULONG i = 0; i < childrenNames.Count(); i++)
                {
                    if (*(childrenNames[i]) == *enumerator->Current().Key)
                    {
                        findTheStateProvider = true;
                        break;
                    }
                }
            }

            VERIFY_IS_TRUE(findTheStateProvider);
            count++;
        }

        // if parentOnly, the count should be the parentCount, otherwise, it should be parentCount plus childrenCount
        VERIFY_ARE_EQUAL(count, parentOnly ? parentCount : parentCount * (childrenCount + 1));

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    Awaitable<void> StateManagerHierarchyTest::Test_GetOrAdd_ExistStateProviders_Commit_Success(
        __in bool isParent) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        KUri::CSPtr parentStateProviderName = GetStateProviderName(NameType::Hierarchy, 0, 0);

        OperationData::SPtr initParameters;

        try
        {
            initParameters = TestStateProvider::CreateInitializationParameters(1, GetAllocator());
        }
        catch (Exception & exception)
        {
            VERIFY_ARE_EQUAL(exception.GetStatus(), STATUS_UNSUCCESSFUL)
                VERIFY_IS_TRUE(false)
        }

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] {txnSPtr->Dispose(); });

            status = co_await stateManagerSPtr->AddAsync(
                *txnSPtr,
                *(parentStateProviderName),
                TestStateProvider::TypeName,
                initParameters.RawPtr());
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            TEST_COMMIT_ASYNC(txnSPtr);
        }

        // Make sure the parent and children exist
        IStateProvider2::SPtr outStateProvider;
        status = stateManagerSPtr->Get(*(parentStateProviderName), outStateProvider);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_NOT_NULL(outStateProvider);
        VERIFY_ARE_EQUAL(*parentStateProviderName, outStateProvider->GetName());

        KUri::CSPtr childrenStateProviderName = GetStateProviderName(NameType::Hierarchy, 0, 1);
        status = stateManagerSPtr->Get(*childrenStateProviderName, outStateProvider);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_NOT_NULL(outStateProvider);
        VERIFY_ARE_EQUAL(*childrenStateProviderName, outStateProvider->GetName());

        // Test
        KUri::CSPtr testStateProvider = isParent ? parentStateProviderName : childrenStateProviderName;

        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] {txnSPtr->Dispose(); });
            bool alreadyExist = false;
            status = co_await stateManagerSPtr->GetOrAddAsync(
                *txnSPtr,
                *testStateProvider,
                TestStateProvider::TypeName,
                outStateProvider,
                alreadyExist);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(alreadyExist);
            VERIFY_IS_NOT_NULL(outStateProvider);
            VERIFY_ARE_EQUAL(*testStateProvider, outStateProvider->GetName());

            TEST_COMMIT_ASYNC(txnSPtr);
        }

        VERIFY_ARE_EQUAL(isParent ? *parentStateProviderName : *childrenStateProviderName, outStateProvider->GetName())

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    Awaitable<void> StateManagerHierarchyTest::Test_GetOrAdd_NotExist_WithHierarchy_Commit_Success(
        __in ULONG childrenCount) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        KUri::CSPtr stateProviderName = GetStateProviderName(NameType::Hierarchy, 0, 0);
        OperationData::SPtr initParameters;

        try
        {
            initParameters = TestStateProvider::CreateInitializationParameters(childrenCount, GetAllocator());
        }
        catch (Exception & exception)
        {
            VERIFY_ARE_EQUAL(exception.GetStatus(), STATUS_UNSUCCESSFUL)
            VERIFY_IS_TRUE(false)
        }

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        IStateProvider2::SPtr outStateProvider;

        // Test
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            bool alreadyExist = false;
            status = co_await stateManagerSPtr->GetOrAddAsync(
                *txnSPtr,
                *stateProviderName,
                TestStateProvider::TypeName,
                outStateProvider,
                alreadyExist,
                initParameters.RawPtr());
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_FALSE(alreadyExist);
            VERIFY_IS_NOT_NULL(outStateProvider);
            VERIFY_ARE_EQUAL(*stateProviderName, outStateProvider->GetName());

            TEST_COMMIT_ASYNC(txnSPtr);
            txnSPtr->Dispose();
        }

        // Verification
        status = stateManagerSPtr->Get(*stateProviderName, outStateProvider);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_NOT_NULL(outStateProvider);
        VERIFY_ARE_EQUAL(*stateProviderName, outStateProvider->GetName());

        for (ULONG j = 1; j <= childrenCount; j++)
        {
            auto childrenStateProviderName = GetStateProviderName(NameType::Hierarchy, 0, j);

            status = stateManagerSPtr->Get(*childrenStateProviderName, outStateProvider);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_NOT_NULL(outStateProvider);
            VERIFY_ARE_EQUAL(*childrenStateProviderName, outStateProvider->GetName());
        }

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    Awaitable<void> StateManagerHierarchyTest::Test_Add_Remove_StateProviders_WithHierarchy_Commit_Success(
        __in ULONG parentCount, 
        __in ULONG childrenCount) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        KArray<KUri::CSPtr> parentNames(GetAllocator());
        for (ULONG i = 0; i < parentCount; i++)
        {
            auto stateProviderName = GetStateProviderName(NameType::Hierarchy, i, 0);
            status = parentNames.Append(stateProviderName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        OperationData::SPtr initParameters;

        try
        {
            initParameters = TestStateProvider::CreateInitializationParameters(childrenCount, GetAllocator());
        }
        catch (Exception & exception)
        {
            VERIFY_ARE_EQUAL(exception.GetStatus(), STATUS_UNSUCCESSFUL);
            VERIFY_IS_TRUE(false);
        }

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Test
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);

            for (ULONG i = 0; i < parentNames.Count(); i++)
            {
                status = co_await stateManagerSPtr->AddAsync(
                    *txnSPtr,
                    *(parentNames[i]),
                    TestStateProvider::TypeName,
                    initParameters.RawPtr());
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            TEST_COMMIT_ASYNC(txnSPtr);
            txnSPtr->Dispose();
        }

        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);

            for (ULONG i = 0; i < parentNames.Count(); i++)
            {
                status = co_await stateManagerSPtr->RemoveAsync(
                    *txnSPtr,
                    *(parentNames[i]));
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            TEST_COMMIT_ASYNC(txnSPtr);
            txnSPtr->Dispose();
        }

        // Verification
        IStateProvider2::SPtr outStateProvider;
        for (ULONG i = 0; i < parentNames.Count(); i++)
        {
            status = stateManagerSPtr->Get(*(parentNames[i]), outStateProvider);
            VERIFY_ARE_EQUAL(status, SF_STATUS_NAME_DOES_NOT_EXIST);
            VERIFY_IS_NULL(outStateProvider);

            for (ULONG j = 1; j <= childrenCount; j++)
            {
                auto childrenStateProviderName = GetStateProviderName(NameType::Hierarchy, i, j);

                status = stateManagerSPtr->Get(*childrenStateProviderName, outStateProvider);
                VERIFY_ARE_EQUAL(status, SF_STATUS_NAME_DOES_NOT_EXIST);
                VERIFY_IS_NULL(outStateProvider);
            }
        }

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    //
    // Goal: Test Remove a state provider with one of its Child already been removed. Should return SF_STATUS_NAME_DOES_NOT_EXIST in this case.
    // Algorithm:
    //    1. Bring up the primary replica
    //    2. Add a state provider
    //    3. Random remove one of its children.
    //    4. Remove the state provider, verify the exception.
    //    5. Clean up and Shut down
    //
    Awaitable<void> StateManagerHierarchyTest::Test_Remove_StateProvider_WithNonExistChild(__in ULONG childrenCount) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        KUri::CSPtr stateProviderName = GetStateProviderName(NameType::Hierarchy, 0, 0);

        OperationData::SPtr initParameters = nullptr;

        try
        {
            initParameters = TestStateProvider::CreateInitializationParameters(childrenCount, GetAllocator());
        }
        catch (Exception & exception)
        {
            VERIFY_ARE_EQUAL(exception.GetStatus(), STATUS_UNSUCCESSFUL);
            VERIFY_IS_TRUE(false);
        }

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Add parent state provider and its children.
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });

            status = co_await stateManagerSPtr->AddAsync(
                *txnSPtr,
                *stateProviderName,
                TestStateProvider::TypeName,
                initParameters.RawPtr());
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            TEST_COMMIT_ASYNC(txnSPtr);
        }

        // Random pick a child to remove.
        ULONG randomNumber = random_.Next(1, childrenCount);
        KUri::CSPtr childName = GetStateProviderName(NameType::Hierarchy, 0, randomNumber);

        // Verify the child state provider must exist.
        IStateProvider2::SPtr outStateProvider = nullptr;
        status = stateManagerSPtr->Get(*childName, outStateProvider);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_NOT_NULL(outStateProvider);
        VERIFY_ARE_EQUAL(*childName, outStateProvider->GetName());

        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });

            status = co_await stateManagerSPtr->RemoveAsync(
                *txnSPtr,
                *childName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            TEST_COMMIT_ASYNC(txnSPtr);
        }

        // Since one of the children does not exist, the RemoveAsync call should return NTSTATUS error code.
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });

            status = co_await stateManagerSPtr->RemoveAsync(
                *txnSPtr,
                *stateProviderName);
            VERIFY_ARE_EQUAL(status, SF_STATUS_NAME_DOES_NOT_EXIST);
        }

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    Awaitable<void> StateManagerHierarchyTest::Test_Recover_Checkpoint_WithHierarchy_Success(
        __in ULONG parentCount,
        __in ULONG childrenCount) noexcept
    {
        NTSTATUS status;

        // Setting up expectations.
        KArray<KUri::CSPtr> parentNames(GetAllocator());
        for (ULONG i = 0; i < parentCount; i++)
        {
            auto stateProviderName = GetStateProviderName(NameType::Hierarchy, i, 0);
            status = parentNames.Append(stateProviderName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        OperationData::SPtr initParameters;

        try
        {
            initParameters = TestStateProvider::CreateInitializationParameters(childrenCount, GetAllocator());
        }
        catch (Exception & exception)
        {
            VERIFY_ARE_EQUAL(exception.GetStatus(), STATUS_UNSUCCESSFUL);
            VERIFY_IS_TRUE(false);
        }

        // Test .
        {
            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
            auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

            co_await PopulateAsync(parentNames, initParameters.RawPtr());

            LONG64 checkpointLSN = co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);

            IStateProvider2::SPtr stateProvider;
            for (ULONG i = 0; i < parentNames.Count(); i++)
            {
                status = stateManagerSPtr->Get(*(parentNames[i]), stateProvider);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                VERIFY_IS_NOT_NULL(stateProvider);
                VERIFY_ARE_EQUAL(*(parentNames[i]), stateProvider->GetName());

                TestStateProvider & testStateProvider = dynamic_cast<TestStateProvider &>(*stateProvider);
                testStateProvider.ExpectCheckpoint(checkpointLSN);
                testStateProvider.Verify(true);

                for (ULONG j = 1; j <= childrenCount; j++)
                {
                    auto childrenStateProviderName = GetStateProviderName(NameType::Hierarchy, i, j);

                    status = stateManagerSPtr->Get(*childrenStateProviderName, stateProvider);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));
                    VERIFY_IS_NOT_NULL(stateProvider);
                    VERIFY_ARE_EQUAL(*childrenStateProviderName, stateProvider->GetName());
                    TestStateProvider & childTestStateProvider = dynamic_cast<TestStateProvider &>(*stateProvider);
                    childTestStateProvider.ExpectCheckpoint(checkpointLSN);
                    childTestStateProvider.Verify(true);
                }
            }
      
            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
            testTransactionalReplicatorSPtr_.Reset();
        }

        // Test Recovery
        {
            testTransactionalReplicatorSPtr_ = CreateReplica(
                *partitionedReplicaIdCSPtr_,
                *runtimeFolders_,
                *partitionSPtr_);

            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
            auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

            IStateProvider2::SPtr stateProvider;
            for (ULONG i = 0; i < parentNames.Count(); i++)
            {
                status = stateManagerSPtr->Get(*(parentNames[i]), stateProvider);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                VERIFY_IS_NOT_NULL(stateProvider);
                VERIFY_ARE_EQUAL(*(parentNames[i]), stateProvider->GetName());

                TestStateProvider & testStateProvider = dynamic_cast<TestStateProvider &>(*stateProvider);
                testStateProvider.Verify(true);

                for (ULONG j = 1; j <= childrenCount; j++)
                {
                    auto childrenStateProviderName = GetStateProviderName(NameType::Hierarchy, i, j);

                    status = stateManagerSPtr->Get(*childrenStateProviderName, stateProvider);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));
                    VERIFY_IS_NOT_NULL(stateProvider);
                    VERIFY_ARE_EQUAL(*childrenStateProviderName, stateProvider->GetName());
                    TestStateProvider & childTestStateProvider = dynamic_cast<TestStateProvider &>(*stateProvider);
                    childTestStateProvider.Verify(true);
                }
            }

            // Shutdown
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
        }

        co_return;
    }

    Awaitable<void> StateManagerHierarchyTest::Test_Copy_MultipleStateProvider_WithHierarchy_AllActiveCopied(
        __in ULONG parentCount,
        __in ULONG childrenCount,
        __in bool checkpointAfterAdd,
        __in bool recoverInPlace) noexcept
    {
        NTSTATUS status;

        // Setting up expectations.
        KArray<KUri::CSPtr> parentNames(GetAllocator());
        for (ULONG i = 0; i < parentCount; i++)
        {
            auto stateProviderName = GetStateProviderName(NameType::Hierarchy, i, 0);
            status = parentNames.Append(stateProviderName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        OperationData::SPtr initParameters;

        try
        {
            initParameters = TestStateProvider::CreateInitializationParameters(childrenCount, GetAllocator());
        }
        catch (Exception & exception)
        {
            VERIFY_ARE_EQUAL(exception.GetStatus(), STATUS_UNSUCCESSFUL);
            VERIFY_IS_TRUE(false);
        }

        OperationDataStream::SPtr copyStream;

        // Setup
        {
            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
            auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

            // Populate
            co_await PopulateAsync(parentNames, initParameters.RawPtr());

            if (checkpointAfterAdd)
            {
                co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);
            }

            // Get the copy stream.
            FABRIC_REPLICA_ID targetReplicaID = recoverInPlace ? partitionedReplicaIdCSPtr_->ReplicaId : partitionedReplicaIdCSPtr_->ReplicaId + 1;
            status = testTransactionalReplicatorSPtr_->StateManager->GetCurrentState(targetReplicaID, copyStream);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            // This is to verify the test.
            IStateProvider2::SPtr stateProvider;
            for (ULONG i = 0; i < parentNames.Count(); i++)
            {
                status = stateManagerSPtr->Get(*(parentNames[i]), stateProvider);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                VERIFY_IS_NOT_NULL(stateProvider);
                VERIFY_ARE_EQUAL(*(parentNames[i]), stateProvider->GetName());

                for (ULONG j = 1; j <= childrenCount; j++)
                {
                    auto childrenStateProviderName = GetStateProviderName(NameType::Hierarchy, i, j);

                    status = stateManagerSPtr->Get(*childrenStateProviderName, stateProvider);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));
                    VERIFY_IS_NOT_NULL(stateProvider);
                    VERIFY_ARE_EQUAL(*childrenStateProviderName, stateProvider->GetName());
                }
            }

            // Clean up
            if (recoverInPlace == false)
            {
                co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            }

            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
        }

        {
            LONG64 replicaId = recoverInPlace ? partitionedReplicaIdCSPtr_->ReplicaId : partitionedReplicaIdCSPtr_->ReplicaId + 1;
            auto partitionedReplicaId = PartitionedReplicaId::Create(partitionedReplicaIdCSPtr_->PartitionId, replicaId, GetAllocator());

            auto runtimeFolders = TestHelper::CreateRuntimeFolders(GetAllocator());
            auto partition = TestHelper::CreateStatefulServicePartition(GetAllocator());
            TestTransactionManager::SPtr secondaryTransactionalReplicator = CreateReplica(*partitionedReplicaId, *runtimeFolders, *partition);

            co_await secondaryTransactionalReplicator->OpenAsync(CancellationToken::None);
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_IDLE_SECONDARY, CancellationToken::None);

            co_await secondaryTransactionalReplicator->CopyAsync(*copyStream, CancellationToken::None);

            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, CancellationToken::None);

            auto stateManagerSPtr = secondaryTransactionalReplicator->StateManager;

            // This is to verify the test.
            IStateProvider2::SPtr stateProvider;
            for (ULONG i = 0; i < parentNames.Count(); i++)
            {
                status = stateManagerSPtr->Get(*(parentNames[i]), stateProvider);
                if (checkpointAfterAdd)
                {
                    VERIFY_IS_TRUE(NT_SUCCESS(status));
                    VERIFY_IS_NOT_NULL(stateProvider);
                    VERIFY_ARE_EQUAL(*(parentNames[i]), stateProvider->GetName());
                }
                else
                {
                    VERIFY_ARE_EQUAL(status, SF_STATUS_NAME_DOES_NOT_EXIST);
                    VERIFY_IS_NULL(stateProvider);
                }

                for (ULONG j = 1; j <= childrenCount; j++)
                {
                    auto childrenStateProviderName = GetStateProviderName(NameType::Hierarchy, i, j);

                    status = stateManagerSPtr->Get(*childrenStateProviderName, stateProvider);
                    if (checkpointAfterAdd)
                    {
                        VERIFY_IS_TRUE(NT_SUCCESS(status));
                        VERIFY_IS_NOT_NULL(stateProvider);
                        VERIFY_ARE_EQUAL(*childrenStateProviderName, stateProvider->GetName());
                    }
                    else
                    {
                        VERIFY_ARE_EQUAL(status, SF_STATUS_NAME_DOES_NOT_EXIST);
                        VERIFY_IS_NULL(stateProvider);
                    }
                }
            }

            // Clean up
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            co_await secondaryTransactionalReplicator->CloseAsync(CancellationToken::None);
        }

        co_return;
    }

    ktl::Awaitable<void> StateManagerHierarchyTest::Test_RecoveryPath_Checkpointless_AllItemsAdded(
        __in ULONG parentCount,
        __in ULONG childrenCount) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        OperationData::SPtr initParameters;

        try
        {
            initParameters = TestStateProvider::CreateInitializationParameters(childrenCount, GetAllocator());
        }
        catch (Exception & exception)
        {
            VERIFY_ARE_EQUAL(exception.GetStatus(), STATUS_UNSUCCESSFUL);
            VERIFY_IS_TRUE(false);
        }

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);

        // Setting up expectations.
        LONG64 index = 1;

        // Apply on the children first then apply on the parents.
        for (ULONG i = 0; i < parentCount; i++)
        {
            LONG64 parentTransactionId = index;
            LONG64 parentStateProviderId = index;
            index++;

            for (ULONG j = 1; j <= childrenCount; j++)
            {
                auto stateProviderName = GetStateProviderName(NameType::Hierarchy, i, j);
                
                LONG64 transactionId = index;
                LONG64 stateProviderId = index;
                
                auto txn = TransactionBase::CreateTransaction(transactionId, false, GetAllocator());
                auto namedMetadataOperationData = GenerateNamedMetadataOperationData(stateProviderId, *stateProviderName, ApplyType::Insert);
                auto redoOperationData = GenerateRedoOperationData(TestStateProvider::TypeName, nullptr, parentStateProviderId);
               
                TxnReplicator::OperationContext::CSPtr operationContext = nullptr;
                status = co_await testTransactionalReplicatorSPtr_->StateManager->ApplyAsync(
                    index,
                    *txn,
                    ApplyContext::RecoveryRedo,
                    namedMetadataOperationData.RawPtr(),
                    redoOperationData.RawPtr(), 
                    operationContext);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                VERIFY_IS_TRUE(operationContext != nullptr);
               
                status = testTransactionalReplicatorSPtr_->StateManager->Unlock(*operationContext);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                index++;
            }

            auto stateProviderName = GetStateProviderName(NameType::Hierarchy, i, 0);

            auto txn = TransactionBase::CreateTransaction(parentTransactionId, false, GetAllocator());
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            auto namedMetadataOperationData = GenerateNamedMetadataOperationData(parentStateProviderId, *stateProviderName, ApplyType::Insert);
            auto redoOperationData = GenerateRedoOperationData(TestStateProvider::TypeName, initParameters.RawPtr());

            TxnReplicator::OperationContext::CSPtr operationContext = nullptr;
            status = co_await testTransactionalReplicatorSPtr_->StateManager->ApplyAsync(
                index,
                *txn,
                ApplyContext::RecoveryRedo,
                namedMetadataOperationData.RawPtr(),
                redoOperationData.RawPtr(), operationContext);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(operationContext != nullptr);

            status = testTransactionalReplicatorSPtr_->StateManager->Unlock(*operationContext);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // This is to verify the test.
        IStateProvider2::SPtr stateProvider;
        for (ULONG i = 0; i < parentCount; i++)
        {
            auto StateProviderName = GetStateProviderName(NameType::Hierarchy, i, 0);

            status = stateManagerSPtr->Get(*StateProviderName, stateProvider);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_NOT_NULL(stateProvider);
            VERIFY_ARE_EQUAL(*StateProviderName, stateProvider->GetName());

            TestStateProvider & parentStateProvider = dynamic_cast<TestStateProvider &>(*stateProvider);

            // Add the RecoverCheckpoint call check both on parent and children, to avoid the 
            // hierarchy bug that children added to the MetadataManager but missed the OpenAsync
            // and RecoverCheckpointAsync call in recover path.
            VERIFY_IS_TRUE(parentStateProvider.RecoverCheckpoint);

            for (ULONG j = 1; j <= childrenCount; j++)
            {
                auto childrenStateProviderName = GetStateProviderName(NameType::Hierarchy, i, j);

                status = stateManagerSPtr->Get(*childrenStateProviderName, stateProvider);
                TestStateProvider & childStateProvider = dynamic_cast<TestStateProvider &>(*stateProvider);

                VERIFY_IS_TRUE(NT_SUCCESS(status));
                VERIFY_IS_NOT_NULL(stateProvider);
                VERIFY_ARE_EQUAL(*childrenStateProviderName, stateProvider->GetName());
                VERIFY_IS_TRUE(childStateProvider.RecoverCheckpoint);
            }
        }

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    BOOST_FIXTURE_TEST_SUITE(StateManagerHierarchyTestSuite, StateManagerHierarchyTest)

    //
    // Scenario:        Add a StateProvider with one child and commit
    // Expected Result: Parent and child StateProviders are correctly added.
    //                  Children should get nullptr for initialization parameters
    // 
    BOOST_AUTO_TEST_CASE(Add_StateProvider_WithChild_Commit_Success)
    {
        SyncAwait(this->Test_Add_StateProviders_WithHierarchy_Commit_Success(1, 1));
    }

    //
    // Scenario:        Add a StateProvider with children count 0 and commit
    // Expected Result: Parent StateProvider are correctly added, no child added.
    // 
    BOOST_AUTO_TEST_CASE(Add_StateProvider_WithChildCountZero_Commit_Success)
    {
        SyncAwait(this->Test_Add_StateProviders_WithHierarchy_Commit_Success(1, 0));
    }

    //
    // Scenario:        Add a StateProvider with children and commit
    // Expected Result: Parent and children StateProviders are correctly added.
    //                  Children should get nullptr for initialization parameters
    // 
    BOOST_AUTO_TEST_CASE(Add_StateProvider_WithChildren_Commit_Success)
    {
        SyncAwait(this->Test_Add_StateProviders_WithHierarchy_Commit_Success(1, 4));
    }

    //
    // Scenario:        Add parent StateProviders with children and commit
    // Expected Result: Parents and children StateProviders are correctly added.
    //                  Children should get nullptr for initialization parameters
    // 
    BOOST_AUTO_TEST_CASE(Add_StateProviders_WithChildren_Commit_Success)
    {
        SyncAwait(this->Test_Add_StateProviders_WithHierarchy_Commit_Success(8, 4));
    }

    // Test CreateEnumerable return the metadata collection with parent only 
    BOOST_AUTO_TEST_CASE(CreateEnumerable_ParentOnly_Success)
    {
        SyncAwait(this->Test_CreateEnumerable(true));
    }

    // Test CreateEnumerable return the metadata collection with parent and children  
    BOOST_AUTO_TEST_CASE(CreateEnumerable_ParentAndChildren_Success)
    {
        SyncAwait(this->Test_CreateEnumerable(false));
    }

    //
    // Scenario:        Add parent StateProviders with one child and commit, 
    //                  Call GetOrAdd on the parent state provider
    // Expected Result: Call should return the parent state provider,
    //                  Parents and children StateProviders are correctly added.
    // 
    BOOST_AUTO_TEST_CASE(GetOrAdd_ExistParent_Commit_Success)
    {
        SyncAwait(this->Test_GetOrAdd_ExistStateProviders_Commit_Success(true));
    }

    //
    // Scenario:        Add parent StateProviders with one child and commit, 
    //                  Call GetOrAdd on the child state provider
    // Expected Result: Call should return the child state provider,
    //                  Parents and children StateProviders are correctly added.
    // 
    BOOST_AUTO_TEST_CASE(GetOrAdd_ExistChild_Commit_Success)
    {
        SyncAwait(this->Test_GetOrAdd_ExistStateProviders_Commit_Success(false));
    }

    //
    // Scenario:        Call GetOrAdd on the state provider does not exist, with no child 
    // Expected Result: Parents state provider should be correctly added.
    // 
    BOOST_AUTO_TEST_CASE(GetOrAdd_NotExist_WithNoChild_Commit_Success)
    {
        SyncAwait(this->Test_GetOrAdd_NotExist_WithHierarchy_Commit_Success(0));
    }

    //
    // Scenario:        Call GetOrAdd on the state provider does not exist, with children  
    // Expected Result: Parents and children state provider should be correctly added.
    // 
    BOOST_AUTO_TEST_CASE(GetOrAdd_NotExist_WithChildren_Commit_Success)
    {
        SyncAwait(this->Test_GetOrAdd_NotExist_WithHierarchy_Commit_Success(8));
    }
   
    //
    // Scenario:        Add parent StateProviders with one child, then removed them 
    // Expected Result: Parents and child StateProviders are all removed.
    // 
    BOOST_AUTO_TEST_CASE(Add_Remove_StateProvider_WithChild_Commit_Success)
    {
        SyncAwait(this->Test_Add_Remove_StateProviders_WithHierarchy_Commit_Success(1, 1));
    }

    //
    // Scenario:        Add parent StateProviders with children, then removed them 
    // Expected Result: Parents and children StateProviders are all removed.
    // 
    BOOST_AUTO_TEST_CASE(Add_Remove_StateProviders_WithChildren_Commit_Success)
    {
        SyncAwait(this->Test_Add_Remove_StateProviders_WithHierarchy_Commit_Success(8, 2));
    }

    //
    // Scenario:        Remove a state provider with one of its Child already been removed.
    // Expected Result: The parent RemoveAsync call return NTSTATUS error code.
    // 
    BOOST_AUTO_TEST_CASE(Remove_StateProvider_WithNonExistChild)
    {
        const ULONG childrenCount = 5;

        SyncAwait(this->Test_Remove_StateProvider_WithNonExistChild(childrenCount));
    }

    //
    // Scenario:        Add a StateProvider with one child and dispose
    // Expected Result: Parent and child StateProviders are not added.
    // 
    BOOST_AUTO_TEST_CASE(Add_StateProvider_WithChild_Dispose_Success)
    {
        SyncAwait(this->Test_Add_StateProvider_WithHierarchy_Abort_Success(true, 1));
    }

    //
    // Scenario:        Add a StateProvider with children and dispose
    // Expected Result: Parent and children StateProviders are not added.
    // 
    BOOST_AUTO_TEST_CASE(Add_StateProvider_WithChildren_Dispose_Success)
    {
        SyncAwait(this->Test_Add_StateProvider_WithHierarchy_Abort_Success(true, 4));
    }

    //
    // Scenario:        Add a StateProvider with one child and abort the transaction with operation
    // Expected Result: Parent and child StateProviders are not added.
    //
    BOOST_AUTO_TEST_CASE(Add_StateProvider_WithChild_Abort_Success)
    {
        SyncAwait(this->Test_Add_StateProvider_WithHierarchy_Abort_Success(false, 1));
    }

    //
    // Scenario:        Add a StateProvider with children and abort the transaction with operation
    // Expected Result: Parent and children StateProviders are not added.
    // 
    BOOST_AUTO_TEST_CASE(Add_StateProvider_WithChildren_Abort_Success)
    {
        SyncAwait(this->Test_Add_StateProvider_WithHierarchy_Abort_Success(false, 4));
    }

    //
    // Scenario:        Recovery Checkpoint with Hierarchy, one parent and one child
    //                  1. primary add state provider with hierarchy
    //                  2. take checkpoint
    //                  3. close primary and open a new one
    //                  4. new primary recover from the checkpoint
    // Expected Result: Parent and child StateProviders are recovered.
    // 
    BOOST_AUTO_TEST_CASE(Recovery_Checkpoint_Parent_Child_Success)
    {
        SyncAwait(this->Test_Recover_Checkpoint_WithHierarchy_Success(1, 1));
    }

    //
    // Scenario:        Recovery Checkpoint with Hierarchy, one parent and multiple children
    // Expected Result: Parent and children StateProviders are recovered.
    // 
    BOOST_AUTO_TEST_CASE(Recovery_Checkpoint_Parent_Children_Success)
    {
        SyncAwait(this->Test_Recover_Checkpoint_WithHierarchy_Success(1, 4));
    }

    //
    // Scenario:        Recovery Checkpoint with Hierarchy, parents and multiple children
    // Expected Result: Parent and children StateProviders are recovered.
    // 
    BOOST_AUTO_TEST_CASE(Recovery_Checkpoint_Parents_Children_Success)
    {
        SyncAwait(this->Test_Recover_Checkpoint_WithHierarchy_Success(32, 8));
    }

    //
    // Scenario:        Recovery path, recovery from the log.
    //                  First apply on the children, then apply on the parent.
    // Expected Result: Parent and children are correctly added
    // 
    BOOST_AUTO_TEST_CASE(Test_RecoveryPath_StateProviderWithHierarchy_ALLAdded)
    {
        SyncAwait(this->Test_RecoveryPath_Checkpointless_AllItemsAdded(1, 1));
    }

    //
    // Scenario:        Recovery path, recovery from the log.
    //                  First apply on the children, then apply on the parent.
    // Expected Result: Parent and children are correctly added
    // 
    BOOST_AUTO_TEST_CASE(Test_RecoveryPath_StateProvidersWithHierarchy_ALLAdded)
    {
        SyncAwait(this->Test_RecoveryPath_Checkpointless_AllItemsAdded(16, 8));
    }

    //
    // Scenario:        Copy an SM with one parent state provider with one child, all are checkpointed.
    //                  Idle secondary getting the copy is empty.
    // Expected Result: All state providers are copied.
    // 
    BOOST_AUTO_TEST_CASE(CopyPath_StateProviderWithHierachy_Checkpointed_AllCopied)
    {
        SyncAwait(this->Test_Copy_MultipleStateProvider_WithHierarchy_AllActiveCopied(1, 1, true, false));
    }

    //
    // Scenario:        Copy an SM with one parent state provider with one child, all are checkpointed.
    //                  Idle secondary getting the copy is empty.
    // Expected Result: All state providers are copied.
    // 
    BOOST_AUTO_TEST_CASE(CopyPath_StateProvidersWithHierachy_Checkpointed_AllCopied)
    {
        SyncAwait(this->Test_Copy_MultipleStateProvider_WithHierarchy_AllActiveCopied(16, 8, true, false));
    }

    //
    // Scenario:        Copy an SM with one parent state provider with one child and all are checkpointed.
    //                  Idle secondary getting the copy has all the state providers already.
    // Expected Result: All state providers are copied.
    // 
    BOOST_AUTO_TEST_CASE(CopyPath_StateProviderWithHierachyAlreadyExist_ReuseRecoveredStateProviders)
    {
        SyncAwait(this->Test_Copy_MultipleStateProvider_WithHierarchy_AllActiveCopied(1, 1, true, true));
    }

    //
    // Scenario:        Copy an SM with one parent state provider with one child and all are checkpointed.
    //                  Idle secondary getting the copy has all the state providers already.
    // Expected Result: All state providers are copied.
    // 
    BOOST_AUTO_TEST_CASE(CopyPath_StateProvidersWithHierachyAlreadyExist_ReuseRecoveredStateProviders)
    {
        SyncAwait(this->Test_Copy_MultipleStateProvider_WithHierarchy_AllActiveCopied(16, 8, true, true));
    }

    //
    // Scenario:        Copy an SM with one parent state provider with one child and all are not checkpointed.
    //                  Idle secondary getting the copy is empty.
    // Expected Result: No state provider should be copied.
    // 
    BOOST_AUTO_TEST_CASE(CopyPath_StateProviderWithHierachy_NoCheckpoint_NothingCopied)
    {
        SyncAwait(this->Test_Copy_MultipleStateProvider_WithHierarchy_AllActiveCopied(1, 1, false , false));
    }

    //
    // Scenario:        Copy an SM with one parent state provider with one child and are not checkpointed.
    //                  Idle secondary getting the copy is empty.
    // Expected Result: No state provider should be copied.
    // 
    BOOST_AUTO_TEST_CASE(CopyPath_StateProvidersWithHierachy_NoCheckpoint_NothingCopied)
    {
        SyncAwait(this->Test_Copy_MultipleStateProvider_WithHierarchy_AllActiveCopied(16, 8, false, false));
    }
    
    BOOST_AUTO_TEST_SUITE_END()
}
