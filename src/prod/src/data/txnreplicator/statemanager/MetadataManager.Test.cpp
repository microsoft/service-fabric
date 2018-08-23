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
using namespace Data::StateManager;
using namespace Data::Utilities;

namespace StateManagerTests
{
    class MetadataManagerTest
    {
        // load the config object as its needed for the tracing to work
        Common::CommonConfig config;

    public:
        enum LockType
        {
            Read,
            Write
        };

        void static VerifyMetadata(__in Metadata const * const metadata1, __in Metadata const * const metadata2)
        {
            VERIFY_IS_NOT_NULL(metadata1);
            VERIFY_IS_NOT_NULL(metadata2);
            VERIFY_ARE_EQUAL(metadata1->CreateLSN, metadata2->CreateLSN);
            VERIFY_ARE_EQUAL(metadata1->DeleteLSN, metadata2->DeleteLSN);
            VERIFY_ARE_EQUAL(metadata1->MetadataMode, metadata2->MetadataMode);
            VERIFY_ARE_EQUAL(metadata1->Name, metadata2->Name);
            VERIFY_ARE_EQUAL(metadata1->ParentId, metadata2->ParentId);
            VERIFY_ARE_EQUAL(metadata1->StateProviderId, metadata2->StateProviderId);
            VERIFY_ARE_EQUAL(metadata1->TransactionId, metadata2->TransactionId);
            VERIFY_ARE_EQUAL(metadata1->MetadataMode, metadata2->MetadataMode);
            VERIFY_ARE_EQUAL(metadata1->TransientCreate, metadata2->TransientCreate);
            VERIFY_ARE_EQUAL(metadata1->TransientDelete, metadata2->TransientDelete);
        }

        static KUri::CSPtr CreateStateProviderName(__in ULONG32 i, __in KAllocator& allocator)
        {
            KString::SPtr stringName;

            NTSTATUS status = KString::Create(stringName, allocator, L"fabric:/sps/sp");
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            WCHAR concatChildrenID = static_cast<WCHAR>('0' + i);
            BOOLEAN concatSuccess = stringName->AppendChar(concatChildrenID);
            VERIFY_IS_TRUE(concatSuccess == TRUE);

            concatSuccess = stringName->SetNullTerminator();
            VERIFY_IS_TRUE(concatSuccess == TRUE);

            KUriView tempName(static_cast<LPCWSTR>(*stringName));

            KUri::CSPtr expectedName;

            status = KUri::Create(tempName, allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            return expectedName;
        }

        static Awaitable<void> AddMetadataTask(
            __in AwaitableCompletionSource<bool> & signalCompletion,
            __in ULONG32 taskNum,
            __in ULONG32 metadataNum,
            __in KAllocator& allocator,
            __in MetadataManager & metadataManager)
        {
            AwaitableCompletionSource<bool>::SPtr tempCompletion(&signalCompletion);
            co_await tempCompletion->GetAwaitable();

            for (ULONG32 i = taskNum * metadataNum; i < taskNum * metadataNum + metadataNum; i++)
            {
                KUri::CSPtr expectedName = CreateStateProviderName(i, allocator);

                // State provider id cannot be 0. So increment all by one.
                auto metadata = TestHelper::CreateMetadata(*expectedName, true, i + 1, allocator);

                co_await metadataManager.AcquireLockAndAddAsync(*expectedName, *metadata, TimeSpan::MaxValue, CancellationToken::None);
            }

            co_return;
        }

        static void ConcurrentAcquireLockAndAdd(
            __in ULONG32 taskNum,
            __in ULONG32 operationPerTask)
        {
            KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
            KFinally([&]() { KtlSystem::Shutdown(); });
            {
                KAllocator& allocator = underlyingSystem->NonPagedAllocator();

                // Setup
                auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);

                KArray<Awaitable<void>> taskArray(allocator);

                AwaitableCompletionSource<bool>::SPtr signalCompletion = nullptr;
                NTSTATUS status = AwaitableCompletionSource<bool>::Create(allocator, 0, signalCompletion);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                for (ULONG32 i = 0; i < taskNum; i++)
                {
                    auto lStatus = taskArray.Append(AddMetadataTask(*signalCompletion, i, operationPerTask, allocator, *metadataManagerSPtr));
                    VERIFY_IS_TRUE(NT_SUCCESS(lStatus));
                }

                signalCompletion->SetResult(true);

                SyncAwait(TaskUtilities<void>::WhenAll(taskArray));

                // Verify
                // StateProviderID can't be 0, so start from 1.
                for (ULONG32 i = 0; i < taskNum * operationPerTask; i++)
                {
                    StateManagerLockContext::SPtr lockContextSPtr = nullptr;

                    KUri::CSPtr name = CreateStateProviderName(i, allocator);

                    auto lockContextExists = metadataManagerSPtr->TryGetLockContext(*name, lockContextSPtr);
                    VERIFY_IS_TRUE(lockContextExists);
                    VERIFY_ARE_EQUAL(0, lockContextSPtr->GrantorCount);
                }

                VERIFY_ARE_EQUAL(0, metadataManagerSPtr->GetInflightTransactionCount());
            }
        }
    };

    static Awaitable<void> AddReadOrWriteLockTask(
        __in AwaitableCompletionSource<bool> & signalCompletion,
        __in ULONG32 taskNum,
        __in ULONG32 operationPerTask,
        __in Metadata const & metadata, 
        __in KtlSystem* underlyingSystem,
        __in ConcurrentDictionary<int, StateManagerLockContext::SPtr> & lockContextDict,
        __in MetadataManager & metadataManager,
        __in MetadataManagerTest::LockType lockType)
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        co_await ktl::CorHelper::ThreadPoolThread(underlyingSystem->DefaultThreadPool());
        AwaitableCompletionSource<bool>::SPtr tempCompletion(&signalCompletion);
        co_await tempCompletion->GetAwaitable();

        for (ULONG32 i = taskNum * operationPerTask; i < taskNum * operationPerTask + operationPerTask; i++)
        {
            TestTransaction::SPtr txnSPtr = TestTransaction::Create(i + 1, underlyingSystem->PagedAllocator());
            StateManagerLockContext::SPtr readLockSPtr = nullptr;

            if (lockType == MetadataManagerTest::LockType::Read)
            {
                status = co_await metadataManager.LockForReadAsync(
                    *metadata.Name,
                    metadata.StateProviderId,
                    *txnSPtr,
                    TimeSpan::MaxValue,
                    CancellationToken::None,
                    readLockSPtr);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }
            else
            {
                status = co_await metadataManager.LockForWriteAsync(
                    *metadata.Name,
                    metadata.StateProviderId,
                    *txnSPtr,
                    TimeSpan::FromSeconds(1),
                    CancellationToken::None,
                    readLockSPtr);
                VERIFY_IS_TRUE(NT_SUCCESS(status) || status == SF_STATUS_TIMEOUT);
            }

            if (NT_SUCCESS(status))
            {
                lockContextDict.Add(i + 1, readLockSPtr);
            }
        }

        co_return;
    }

    static void ConcurrentAcquireReadOrWriteLock(
        __in ULONG32 taskNum,
        __in ULONG32 operationPerTask,
        __in MetadataManagerTest::LockType lockType)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            // Expected
            KUri::CSPtr expectedName;
            NTSTATUS status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
            ASSERT_IFNOT(
                NT_SUCCESS(status),
                "ConcurrentAcquireReadOrWriteLock:Create KUri failed. Status: {0}", 
                status);
            Metadata::SPtr metadata = TestHelper::CreateMetadata(*expectedName, false, allocator);

            // Setup
            MetadataManager::SPtr metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);

            // Prepopulate
            bool isAdded = metadataManagerSPtr->TryAdd(*expectedName, *metadata);
            ASSERT_IFNOT(isAdded, "ConcurrentAcquireReadOrWriteLock: TryAdd failed");

            KArray<Awaitable<void>> taskArray(allocator);
            ASSERT_IFNOT(
                NT_SUCCESS(taskArray.Status()), 
                "ConcurrentAcquireReadOrWriteLock:Create taskArray failed. Status: {0}",
                taskArray.Status());
            ConcurrentDictionary<int, StateManagerLockContext::SPtr>::SPtr lockContextDict;
            status = ConcurrentDictionary<int, StateManagerLockContext::SPtr>::Create(
                allocator,
                lockContextDict);
            ASSERT_IFNOT(NT_SUCCESS(
                status),
                "ConcurrentAcquireReadOrWriteLock:Create lockContextArray failed. Status: {0}",
                status);
            KArray<StateManagerTransactionContext::SPtr> txnContextSPtrArray(allocator);
            ASSERT_IFNOT(
                NT_SUCCESS(txnContextSPtrArray.Status()), 
                "ConcurrentAcquireReadOrWriteLock:Create txnContextSPtrArray failed. Status: {0}",
                txnContextSPtrArray.Status());

            AwaitableCompletionSource<bool>::SPtr signalCompletion = nullptr;
            status = AwaitableCompletionSource<bool>::Create(allocator, 0, signalCompletion);
            ASSERT_IFNOT(
                NT_SUCCESS(status), 
                "ConcurrentAcquireReadOrWriteLock:Create AwaitableCompletionSource failed. Status: {0}",
                status);

            for (ULONG32 i = 0; i < taskNum; i++)
            {
                status = taskArray.Append(AddReadOrWriteLockTask(
                    *signalCompletion,
                    i, 
                    operationPerTask, 
                    *metadata,
                    underlyingSystem,
                    *lockContextDict,
                    *metadataManagerSPtr,
                    lockType));
                ASSERT_IFNOT(
                    NT_SUCCESS(status), 
                    "ConcurrentAcquireReadOrWriteLock:: Add task fail. Status: {0}",
                    status);
            }

            signalCompletion->SetResult(true);
            SyncAwait(TaskUtilities<void>::WhenAll(taskArray));

            // Verify
            ASSERT_IFNOT(
                lockContextDict->Count == (lockType == MetadataManagerTest::LockType::Read ? taskNum * operationPerTask : 1),
                "ConcurrentAcquireReadOrWriteLock: Count does not match: Cout: {0}",
                lockContextDict->Count);

            auto enumerator = lockContextDict->GetEnumerator();
            while (enumerator->MoveNext())
            {
                StateManagerTransactionContext::SPtr txnContextSPtr = nullptr;
                status = StateManagerTransactionContext::Create(enumerator->Current().Key, *enumerator->Current().Value, OperationType::Enum::Read, allocator, txnContextSPtr);
                ASSERT_IFNOT(
                    NT_SUCCESS(status),
                    "ConcurrentAcquireReadOrWriteLock: Create StateManagerTransactionContext failed. Status: {0}",
                    status);
                status = txnContextSPtrArray.Append(txnContextSPtr);
                ASSERT_IFNOT(
                    NT_SUCCESS(status),
                    "ConcurrentAcquireReadOrWriteLock: txnContextSPtrArray append failed. Status: {0}",
                    status);
            }

            ASSERT_IFNOT(
                metadataManagerSPtr->GetInflightTransactionCount() == (lockType == MetadataManagerTest::LockType::Read ? taskNum * operationPerTask : 1),
                "ConcurrentAcquireReadOrWriteLock: Count does not match: Cout: {0}",
                metadataManagerSPtr->GetInflightTransactionCount());

            // Unlock all the txns
            for (StateManagerTransactionContext::SPtr txnContextSPtr : txnContextSPtrArray)
            {
                txnContextSPtr->Unlock();
            }

            // Verification
            StateManagerLockContext::SPtr testLockContextSPtr = nullptr;
            bool lockContextExists = metadataManagerSPtr->TryGetLockContext(*metadata->Name, testLockContextSPtr);
            ASSERT_IFNOT(lockContextExists, "ConcurrentAcquireReadOrWriteLock: Lock check failed.");
            ASSERT_IFNOT(
                0 == testLockContextSPtr->GrantorCount, 
                "ConcurrentAcquireReadOrWriteLock: Count is not expected. Count: {0}", 
                testLockContextSPtr->GrantorCount);
            ASSERT_IFNOT(
                0 == metadataManagerSPtr->GetInflightTransactionCount(),
                "ConcurrentAcquireReadOrWriteLock: Count is not expected. Count: {0}",
                metadataManagerSPtr->GetInflightTransactionCount());
        }
    };

    BOOST_FIXTURE_TEST_SUITE(MetadataManagerTestSuite, MetadataManagerTest)

    // Acitve metadata will be added to both inMemoryState and stateProviderIdMap
    // Check both dictionary to verify the metadata is added
    BOOST_AUTO_TEST_CASE(Add_Active_Metadata_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Expected
            KUri::CSPtr expectedName;
            auto status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            auto metadata = TestHelper::CreateMetadata(*expectedName, false, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);
            bool isAdded = metadataManagerSPtr->TryAdd(*expectedName, *metadata);
            VERIFY_IS_TRUE(isAdded == true);

            // Test
            bool result = metadataManagerSPtr->IsEmpty();
            VERIFY_IS_TRUE(result == false);
            result = metadataManagerSPtr->ContainsKey(*expectedName);
            VERIFY_IS_TRUE(result == true);
        }
    }

    // Transient metadata will be only added to inMemoryState
    // So stateProviderIdMap returns false
    BOOST_AUTO_TEST_CASE(Add_Transient_Metadata_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Expected
            KUri::CSPtr expectedName;
            auto status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            auto metadata = TestHelper::CreateMetadata(*expectedName, true, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);
            bool isAdded = metadataManagerSPtr->TryAdd(*expectedName, *metadata);
            VERIFY_IS_TRUE(isAdded == true);

            // Test
            bool result = metadataManagerSPtr->IsEmpty();
            VERIFY_IS_TRUE(result == false);
            result = metadataManagerSPtr->ContainsKey(*expectedName);
            VERIFY_IS_TRUE(result == false);
        }
    }

    //  Can't add metadata which is added
    BOOST_AUTO_TEST_CASE(Add_MetadataAlreadyExists_Negative)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Expected
            KUri::CSPtr expectedName;
            auto status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            auto metadata1 = TestHelper::CreateMetadata(*expectedName, false, allocator);
            auto metadata2 = TestHelper::CreateMetadata(*expectedName, true, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);
            bool isAdded = metadataManagerSPtr->TryAdd(*expectedName, *metadata1);
            VERIFY_IS_TRUE(isAdded == true);

            // Test
            isAdded = metadataManagerSPtr->TryAdd(*expectedName, *metadata2);
            VERIFY_IS_TRUE(isAdded == false);
        }
    }

    // Any function is used to check inMemoryState dictionary, which checks both active and transient metadata
    BOOST_AUTO_TEST_CASE(IsEmpty_NoItems_Negative)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);

            // Test
            bool result = metadataManagerSPtr->IsEmpty();
            VERIFY_IS_TRUE(result == true);
        }
    }

    BOOST_AUTO_TEST_CASE(ContainsKey_NotExist_Negative)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // expected
            KUri::CSPtr expectedName;
            auto status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);

            // Test
            bool containsSP = metadataManagerSPtr->ContainsKey(*expectedName);
            VERIFY_IS_TRUE(containsSP == false);
        }
    }

    BOOST_AUTO_TEST_CASE(ContainsKey_MetadataIsTransient_Negative)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // expected
            KUri::CSPtr expectedName;
            auto status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            auto metadata = TestHelper::CreateMetadata(*expectedName, true, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);

            // Test
            bool isAdded = metadataManagerSPtr->TryAdd(*expectedName, *metadata);
            VERIFY_IS_TRUE(isAdded == true);

            bool containsSP = metadataManagerSPtr->ContainsKey(*expectedName);
            VERIFY_IS_TRUE(containsSP == false);
        }
    }

    BOOST_AUTO_TEST_CASE(ContainsKey_MetadataIsApplied_Success)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // expected
            KUri::CSPtr expectedName;
            auto status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            auto metadata = TestHelper::CreateMetadata(*expectedName, false, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);

            // Test
            bool isAdded = metadataManagerSPtr->TryAdd(*expectedName, *metadata);
            VERIFY_IS_TRUE(isAdded == true);

            // Test
            bool containsSP = metadataManagerSPtr->ContainsKey(*expectedName);
            VERIFY_IS_TRUE(containsSP == true);
        }
    }

    BOOST_AUTO_TEST_CASE(GetMetadataCollection_Success)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);
            KArray<Metadata::SPtr> expectedMetadatas(allocator);

            for (ULONG i = 0; i < 10; i++)
            {
                KUri::CSPtr expectedName;
                wchar_t number = static_cast<wchar_t>(i);

                auto status = KUri::Create(KUriView(&number), allocator, expectedName);
                THROW_ON_FAILURE(status)

                auto metadata = TestHelper::CreateMetadata(*expectedName, false, i+1, allocator);
                status = expectedMetadatas.Append(metadata);
                THROW_ON_FAILURE(status)

                bool isAdded = metadataManagerSPtr->TryAdd(*expectedName, *metadata);
                VERIFY_IS_TRUE(isAdded == true);
            }

            // Test
            auto metadataEnumerator = metadataManagerSPtr->GetMetadataCollection();
            ULONG count = 0;
            while (metadataEnumerator->MoveNext())
            {
                Metadata::SPtr current = (metadataEnumerator->Current()).Value;
                ULONG expectedMetadataNum = static_cast<ULONG>(current->StateProviderId - 1);
                VerifyMetadata(current.RawPtr(), expectedMetadatas[expectedMetadataNum].RawPtr());
                count++;
            }
            VERIFY_IS_TRUE(count == 10);
        }
    }

    // GetMetadataCollection will only return the metadata with TransientCreate = false,
    // This test added 10 metadatas to the metadataManager, half of them are TransientCreate,
    // So when we call GetMetadataCollection, it will only return the metadatas with TransientCreate = false
    BOOST_AUTO_TEST_CASE(GetMetadataCollection_HalfTransientCreate_Success)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            MetadataManager::SPtr metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);
            KArray<Metadata::SPtr> expectedMetadatas(allocator);

            for (ULONG i = 0; i < 10; i++)
            {
                KUri::CSPtr expectedName;
                wchar_t number = static_cast<wchar_t>(i);

                NTSTATUS status = KUri::Create(KUriView(&number), allocator, expectedName);
                THROW_ON_FAILURE(status)

                Metadata::SPtr metadata;
                if (i % 2 == 1)
                {
                    metadata = TestHelper::CreateMetadata(*expectedName, false, i + 1, allocator);
                }
                else
                {
                    metadata = TestHelper::CreateMetadata(*expectedName, true, i + 1, allocator);
                }

                status = expectedMetadatas.Append(metadata);
                THROW_ON_FAILURE(status)

                bool isAdded = metadataManagerSPtr->TryAdd(*expectedName, *metadata);
                VERIFY_IS_TRUE(isAdded == true);
            }

            // Test
            auto metadataEnumerator = metadataManagerSPtr->GetMetadataCollection();
            ULONG count = 0;
            while (metadataEnumerator->MoveNext())
            {
                Metadata::SPtr current = (metadataEnumerator->Current()).Value;
                ULONG expectedMetadataNum = static_cast<ULONG>(current->StateProviderId - 1);
                VERIFY_IS_TRUE((current->StateProviderId - 1) % 2 == 1);

                VerifyMetadata(current.RawPtr(), expectedMetadatas[expectedMetadataNum].RawPtr());
                count++;
            }

            VERIFY_IS_TRUE(count == 5);
        }
    }

    BOOST_AUTO_TEST_CASE(ResetTransientState_Success)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // expected
            KUri::CSPtr expectedName;
            auto status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            auto metadata = TestHelper::CreateMetadata(*expectedName, true, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);
            bool isAdded = metadataManagerSPtr->TryAdd(*expectedName, *metadata);
            VERIFY_IS_TRUE(isAdded == true);

            // transient metadata 
            bool result = metadataManagerSPtr->IsEmpty();
            VERIFY_IS_TRUE(result == false);
            result = metadataManagerSPtr->ContainsKey(*expectedName);
            VERIFY_IS_TRUE(result == false);

            metadataManagerSPtr->ResetTransientState(*expectedName);

            // active metadata
            result = metadataManagerSPtr->IsEmpty();
            VERIFY_IS_TRUE(result == false);
            result = metadataManagerSPtr->ContainsKey(*expectedName);
            VERIFY_IS_TRUE(result == true);
        }
    }

    // Add metadata to deleted and call GetDeletedStateProviders to verify
    BOOST_AUTO_TEST_CASE(AddDeleted_Success)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // expected
            KUri::CSPtr expectedName;
            auto status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            auto metadata = TestHelper::CreateMetadata(*expectedName, false, allocator);
            metadata->MetadataMode = MetadataMode::DelayDelete;

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);
            auto result = metadataManagerSPtr->AddDeleted(metadata->StateProviderId, *metadata);
            VERIFY_IS_TRUE(result == true);

            // Verify
            KSharedArray<Metadata::CSPtr>::SPtr softDeletedMetadataArray = nullptr;
            status = metadataManagerSPtr->GetDeletedConstMetadataArray(softDeletedMetadataArray);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            
            ULONG count = 0;
            for (Metadata::CSPtr current : *softDeletedMetadataArray)
            {
                VerifyMetadata(current.RawPtr(), metadata.RawPtr());
                count++;
            }

            VERIFY_ARE_EQUAL(count, 1);
        }
    }

    //
    // Scenario:        Add Metadata to deleted then remove from deleted
    // Expected Result: TryGetDeletedMetadata should return false, and result Metadata is nullptr
    // 
    BOOST_AUTO_TEST_CASE(TryRemoveDeleted_RemovedFromDeleted)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // expected
            KUri::CSPtr expectedName;
            auto status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            auto metadata = TestHelper::CreateMetadata(*expectedName, false, allocator);
            metadata->MetadataMode = MetadataMode::DelayDelete;

            // Added to the deleted
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);
            auto result = metadataManagerSPtr->AddDeleted(metadata->StateProviderId, *metadata);
            VERIFY_IS_TRUE(result);

            Metadata::SPtr returnedMetadata = nullptr;
            // Verify the metadata is in the deleted list
            result = metadataManagerSPtr->TryGetDeletedMetadata(metadata->StateProviderId, returnedMetadata);
            VERIFY_IS_TRUE(result);
            VerifyMetadata(metadata.RawPtr(), returnedMetadata.RawPtr());

            // Remove the metadata from the deleted list
            result = metadataManagerSPtr->TryRemoveDeleted(metadata->StateProviderId, returnedMetadata);
            VERIFY_IS_TRUE(result);
            VerifyMetadata(metadata.RawPtr(), returnedMetadata.RawPtr());

            // Remove again to test the false scenario
            returnedMetadata = nullptr;
            result = metadataManagerSPtr->TryRemoveDeleted(metadata->StateProviderId, returnedMetadata);
            VERIFY_IS_TRUE(!result);
            VERIFY_IS_NULL(returnedMetadata);

            // Verify its been removed
            returnedMetadata = nullptr;
            result = metadataManagerSPtr->TryGetDeletedMetadata(metadata->StateProviderId, returnedMetadata);
            VERIFY_IS_TRUE(result == false);
            VERIFY_IS_NULL(returnedMetadata);
        }
    }

    BOOST_AUTO_TEST_CASE(IsStateProviderDeleted_StateProviderDeleted_Success)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();
            LONG64 const stateProviderId = 1;

            // expected
            KUri::CSPtr expectedName;
            auto status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            auto metadata = TestHelper::CreateMetadata(*expectedName, false, stateProviderId, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);
            metadata->MetadataMode = MetadataMode::Enum::DelayDelete;
            auto result = metadataManagerSPtr->AddDeleted(stateProviderId, *metadata);
            VERIFY_IS_TRUE(result == true);

            // Verify
            result = metadataManagerSPtr->IsStateProviderDeletedOrStale(stateProviderId, MetadataMode::DelayDelete);
            VERIFY_IS_TRUE(result == true);
        }
    }

    BOOST_AUTO_TEST_CASE(IsStateProviderDeleted_FalseProgress_Negtive)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();
            LONG64 const stateProviderId = 1;

            // expected
            KUri::CSPtr expectedName;
            auto status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            auto metadata = TestHelper::CreateMetadata(*expectedName, false, stateProviderId, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);
            metadata->MetadataMode = MetadataMode::Enum::FalseProgress;
            auto result = metadataManagerSPtr->AddDeleted(stateProviderId, *metadata);
            VERIFY_IS_TRUE(result == true);

            // Verify
            result = metadataManagerSPtr->IsStateProviderDeletedOrStale(stateProviderId, MetadataMode::DelayDelete);
            VERIFY_IS_TRUE(result == false);
        }
    }

    BOOST_AUTO_TEST_CASE(IsStateProviderDeleted_NonExistStateProvider_Negtive)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();
            LONG64 const nonExistStateProviderId = 2;

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);

            // Verify
            auto result = metadataManagerSPtr->IsStateProviderDeletedOrStale(nonExistStateProviderId, MetadataMode::DelayDelete);
            VERIFY_IS_TRUE(result == false);
        }
    }

    BOOST_AUTO_TEST_CASE(IsStateProviderStale_Success)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();
            LONG64 const stateProviderId = 1;
            LONG64 const nonExistStateProviderId = 2;

            // expected
            KUri::CSPtr expectedName;
            auto status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            auto metadata = TestHelper::CreateMetadata(*expectedName, false, stateProviderId, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);
            metadata->MetadataMode = MetadataMode::Enum::DelayDelete;
            auto result = metadataManagerSPtr->AddDeleted(stateProviderId, *metadata);
            VERIFY_IS_TRUE(result == true);

            // Verify
            result = metadataManagerSPtr->IsStateProviderDeletedOrStale(nonExistStateProviderId, MetadataMode::FalseProgress);
            VERIFY_IS_TRUE(result == false);

            result = metadataManagerSPtr->IsStateProviderDeletedOrStale(stateProviderId, MetadataMode::FalseProgress);
            VERIFY_IS_TRUE(result == false);
        }
    }

    // Get Test where the Get fails because the metadata does not exist.
    // Test includes both ALL and applied only.
    BOOST_AUTO_TEST_CASE(Get_MetdataDoesNotExist_Negative)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Expected
            KUri::CSPtr expectedName;
            auto status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);

            // Test
            Metadata::SPtr outputMetadataSPtr = nullptr;
            bool isExist = metadataManagerSPtr->TryGetMetadata(*expectedName, false, outputMetadataSPtr);
            VERIFY_IS_TRUE(isExist == false);

            isExist = metadataManagerSPtr->TryGetMetadata(*expectedName, true, outputMetadataSPtr);
            VERIFY_IS_TRUE(isExist == false);
        }
    }

    // Get Test where the Get fails because the metadata exists but it is transient.
    BOOST_AUTO_TEST_CASE(Get_AppliedOnly_MetdataIsTransient_Negative)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // expected
            KUri::CSPtr expectedName;
            auto status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            auto metadata = TestHelper::CreateMetadata(*expectedName, true, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);

            // Test
            bool isAdded = metadataManagerSPtr->TryAdd(*expectedName, *metadata);
            VERIFY_IS_TRUE(isAdded == true);

            Metadata::SPtr outputMetadataSPtr = nullptr;
            bool isExist = metadataManagerSPtr->TryGetMetadata(*expectedName, false, outputMetadataSPtr);
            VERIFY_IS_TRUE(isExist == false);
        }
    }

    // Allow return Transient metadata
    BOOST_AUTO_TEST_CASE(Get_AllowTransient_MetdataIsTransient_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // expected
            KUri::CSPtr expectedName;
            auto status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            auto metadata = TestHelper::CreateMetadata(*expectedName, true, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);

            // Test
            bool isAdded = metadataManagerSPtr->TryAdd(*expectedName, *metadata);
            VERIFY_IS_TRUE(isAdded == true);

            Metadata::SPtr outputMetadataSPtr = nullptr;
            bool isExist = metadataManagerSPtr->TryGetMetadata(*expectedName, true, outputMetadataSPtr);
            VERIFY_IS_TRUE(isExist == true);

            VerifyMetadata(outputMetadataSPtr.RawPtr(), metadata.RawPtr());
        }
    }

    BOOST_AUTO_TEST_CASE(GetActiveStateProvider_Active_Metadata_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // expected
            KUri::CSPtr expectedName;
            auto status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            auto metadata = TestHelper::CreateMetadata(*expectedName, false, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);

            // Test
            bool isAdded = metadataManagerSPtr->TryAdd(*expectedName, *metadata);
            VERIFY_IS_TRUE(isAdded == true);

            Metadata::SPtr metadataSPtr = nullptr;
            bool isExist = metadataManagerSPtr->TryGetMetadata(metadata->StateProviderId, metadataSPtr);

            VERIFY_IS_TRUE(isExist);
        }
    }

    BOOST_AUTO_TEST_CASE(AcquireLockAndAdd_SUCCESS)
    {
        ULONG32 const taskNum = 1;
        ULONG32 const operationPerTask = 1;
        ConcurrentAcquireLockAndAdd(taskNum, operationPerTask);
    }

    BOOST_AUTO_TEST_CASE(Concurrent_AcquireLockAndAdd_SUCCESS)
    {
        ULONG32 const taskNum = 10;
        ULONG32 const operationPerTask = 1;
        ConcurrentAcquireLockAndAdd(taskNum, operationPerTask);
    }

    BOOST_AUTO_TEST_CASE(Concurrent_AcquireLockAndAdd_MultiOperationPerTask_SUCCESS)
    {
        ULONG32 const taskNum = 10;
        ULONG32 const operationPerTask = 5;
        ConcurrentAcquireLockAndAdd(taskNum, operationPerTask);
    }

    // Assert if item is not exist, so just test ExistItem.
    BOOST_AUTO_TEST_CASE(Unlock_Remove_Transient_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // expected
            KUri::CSPtr expectedName;
            auto status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            auto metadata = TestHelper::CreateMetadata(*expectedName, true, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);

            // Pre-populate the metadata manager with a state provider.
            bool isAdded = metadataManagerSPtr->TryAdd(*expectedName, *metadata);
            VERIFY_IS_TRUE(isAdded);

            TestTransaction::SPtr txnSPtr = TestTransaction::Create(1, allocator);
            StateManagerLockContext::SPtr lockContextSPtr = nullptr;
            status = SyncAwait(metadataManagerSPtr->LockForWriteAsync(
                *expectedName, 
                metadata->StateProviderId, 
                *txnSPtr, 
                TimeSpan::MaxValue, 
                CancellationToken::None,
                lockContextSPtr));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_ARE_EQUAL(1, lockContextSPtr->GrantorCount);

            StateManagerTransactionContext::SPtr txnContextSPtr = nullptr;
            status = StateManagerTransactionContext::Create(1, *lockContextSPtr, OperationType::Enum::Add, allocator, txnContextSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            txnContextSPtr->Unlock();

            bool lockContextExists = metadataManagerSPtr->TryGetLockContext(*metadata->Name, lockContextSPtr);
            VERIFY_IS_FALSE(lockContextExists);
        }
    }

    // Abort code path: User tried adding the same state provider name twice.
    BOOST_AUTO_TEST_CASE(Unlock_Add_SameName_Abort)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // expected
            KUri::CSPtr expectedName;
            auto status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            auto metadata = TestHelper::CreateMetadata(*expectedName, true, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);

            // Pre-populate the metadata manager with a state provider.
            bool isAdded = metadataManagerSPtr->TryAdd(*expectedName, *metadata);
            VERIFY_IS_TRUE(isAdded);

            TestTransaction::SPtr txnSPtr = TestTransaction::Create(1, allocator);
            StateManagerLockContext::SPtr lockContextSPtr = nullptr;
            status = SyncAwait(metadataManagerSPtr->LockForWriteAsync(
                *metadata->Name,
                metadata->StateProviderId + 1,
                *txnSPtr,
                TimeSpan::MaxValue,
                CancellationToken::None,
                lockContextSPtr));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_ARE_EQUAL(1, lockContextSPtr->GrantorCount);

            StateManagerTransactionContext::SPtr txnContextSPtr = nullptr;
            status = StateManagerTransactionContext::Create(1, *lockContextSPtr, OperationType::Enum::Add, allocator, txnContextSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            txnContextSPtr->Unlock();

            // Lock has been released
            VERIFY_ARE_EQUAL(0, lockContextSPtr->GrantorCount);
            VERIFY_ARE_EQUAL(0, metadataManagerSPtr->GetInflightTransactionCount());
        }
    }

    BOOST_AUTO_TEST_CASE(Unlock_ReadTxn_Success)
    {
        NTSTATUS status;

        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Expected
            KUri::CSPtr expectedName;
            status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            auto metadata = TestHelper::CreateMetadata(*expectedName, false, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);

            // Pre-populate the metadata manager with a state provider.
            bool isAdded = metadataManagerSPtr->TryAdd(*expectedName, *metadata);
            VERIFY_IS_TRUE(isAdded);

            TestTransaction::SPtr txnSPtr = TestTransaction::Create(1, allocator);
            StateManagerLockContext::SPtr lockContextSPtr = nullptr;
            status = SyncAwait(metadataManagerSPtr->LockForReadAsync(
                *expectedName, 
                metadata->StateProviderId, 
                *txnSPtr, 
                TimeSpan::MaxValue, 
                CancellationToken::None,
                lockContextSPtr));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            StateManagerTransactionContext::SPtr txnContextSPtr = nullptr;
            status = StateManagerTransactionContext::Create(1, *lockContextSPtr, OperationType::Enum::Read, allocator, txnContextSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            StateManagerLockContext::SPtr lockContextSPtr2 = nullptr;
            bool lockContextExists = metadataManagerSPtr->TryGetLockContext(*metadata->Name, lockContextSPtr2);
            VERIFY_IS_TRUE(lockContextExists);
            VERIFY_ARE_EQUAL(StateManagerLockMode::Enum::Read, lockContextSPtr2->LockMode);

            txnContextSPtr->Unlock();
            lockContextExists = metadataManagerSPtr->TryGetLockContext(*metadata->Name, lockContextSPtr2);
            VERIFY_IS_TRUE(lockContextExists);
            VERIFY_ARE_EQUAL(0, lockContextSPtr2->GrantorCount);
            VERIFY_ARE_EQUAL(0, metadataManagerSPtr->GetInflightTransactionCount());
        }
    }

    BOOST_AUTO_TEST_CASE(Unlock_WriteTxn_Success)
    {
        NTSTATUS status;

        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Expected
            KUri::CSPtr expectedName;
            status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            auto metadata = TestHelper::CreateMetadata(*expectedName, false, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);

            TestTransaction::SPtr txnSPtr = TestTransaction::Create(1, allocator);
            StateManagerLockContext::SPtr lockContextSPtr = nullptr;
            status = SyncAwait(metadataManagerSPtr->LockForWriteAsync(
                *expectedName,
                metadata->StateProviderId,
                *txnSPtr,
                TimeSpan::MaxValue,
                CancellationToken::None,
                lockContextSPtr));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_ARE_EQUAL(1, lockContextSPtr->GrantorCount);

            bool isAdded = metadataManagerSPtr->TryAdd(*expectedName, *metadata);
            VERIFY_IS_TRUE(isAdded);

            StateManagerTransactionContext::SPtr txnContextSPtr = nullptr;
            status = StateManagerTransactionContext::Create(1, *lockContextSPtr, OperationType::Enum::Add, allocator, txnContextSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            StateManagerLockContext::SPtr lockContextSPtr2 = nullptr;
            bool lockContextExists = metadataManagerSPtr->TryGetLockContext(*metadata->Name, lockContextSPtr2);
            VERIFY_IS_TRUE(lockContextExists);
            VERIFY_ARE_EQUAL(StateManagerLockMode::Enum::Write, lockContextSPtr2->LockMode);

            Metadata::SPtr metadataSPtr = nullptr;
            bool isExist = metadataManagerSPtr->TryGetMetadata(*expectedName, true, metadataSPtr);
            VERIFY_IS_TRUE(isExist);

            metadataSPtr->TransientCreate = false;

            txnContextSPtr->Unlock();
            lockContextExists = metadataManagerSPtr->TryGetLockContext(*metadata->Name, lockContextSPtr2);
            VERIFY_IS_TRUE(lockContextExists);
            VERIFY_ARE_EQUAL(0, lockContextSPtr2->GrantorCount);
            VERIFY_ARE_EQUAL(0, metadataManagerSPtr->GetInflightTransactionCount());
        }
    }

    // Multi ReadLock is fine
    BOOST_AUTO_TEST_CASE(Multi_ReadLock_Success)
    {
        NTSTATUS status;

        // TODO: Fix the cyclic reference. Investigate the KFinally problem.
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Expected
            KUri::CSPtr expectedName;
            status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            auto metadata = TestHelper::CreateMetadata(*expectedName, false, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);

            // Create the text transaction
            TestTransaction::SPtr txnSPtr1 = TestTransaction::Create(1, allocator);
            TestTransaction::SPtr txnSPtr2 = TestTransaction::Create(2, allocator);
            TestTransaction::SPtr txnSPtr3 = TestTransaction::Create(3, allocator);

            // Prepopulate
            bool isAdded = metadataManagerSPtr->TryAdd(*expectedName, *metadata);
            VERIFY_IS_TRUE(isAdded);

            // Take the read lock and release it.
            StateManagerLockContext::SPtr readLockSPtr = nullptr;
            status = SyncAwait(metadataManagerSPtr->LockForReadAsync(
                *expectedName, 
                metadata->StateProviderId, 
                *txnSPtr1, 
                TimeSpan::MaxValue, 
                CancellationToken::None,
                readLockSPtr));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            StateManagerTransactionContext::SPtr txnContextSPtr = nullptr;
            status = StateManagerTransactionContext::Create(1, *readLockSPtr, OperationType::Enum::Read, allocator, txnContextSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = SyncAwait(metadataManagerSPtr->LockForReadAsync(
                *expectedName, 
                metadata->StateProviderId, 
                *txnSPtr2, 
                TimeSpan::MaxValue, 
                CancellationToken::None,
                readLockSPtr));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            StateManagerTransactionContext::SPtr txnContextSPtr2 = nullptr;
            status = StateManagerTransactionContext::Create(2, *readLockSPtr, OperationType::Enum::Read, allocator, txnContextSPtr2);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = SyncAwait(metadataManagerSPtr->LockForReadAsync(
                *expectedName, 
                metadata->StateProviderId, 
                *txnSPtr3, 
                TimeSpan::MaxValue, 
                CancellationToken::None,
                readLockSPtr));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            StateManagerTransactionContext::SPtr txnContextSPtr3 = nullptr;
            status = StateManagerTransactionContext::Create(3, *readLockSPtr, OperationType::Enum::Read, allocator, txnContextSPtr3);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_ARE_EQUAL(0, readLockSPtr->GrantorCount);
            VERIFY_ARE_EQUAL(3, metadataManagerSPtr->GetInflightTransactionCount());

            txnContextSPtr->Unlock();
            txnContextSPtr2->Unlock();
            txnContextSPtr3->Unlock();

            // Verification
            StateManagerLockContext::SPtr testLockContextSPtr = nullptr;
            bool lockContextExists = metadataManagerSPtr->TryGetLockContext(*metadata->Name, testLockContextSPtr);
            VERIFY_IS_TRUE(lockContextExists);
            VERIFY_ARE_EQUAL(0, testLockContextSPtr->GrantorCount);
            VERIFY_ARE_EQUAL(0, metadataManagerSPtr->GetInflightTransactionCount());
        }
    }

    BOOST_AUTO_TEST_CASE(Concurrent_ReadLock_Success)
    {
        ULONG32 const taskNum = 8;
        ULONG32 const operationPerTask = 4;
        ConcurrentAcquireReadOrWriteLock(taskNum, operationPerTask, MetadataManagerTest::LockType::Read);
    }

    BOOST_AUTO_TEST_CASE(Concurrent_WriteLock_Timeout)
    {
        ULONG32 const taskNum = 8;
        ULONG32 const operationPerTask = 4;
        ConcurrentAcquireReadOrWriteLock(taskNum, operationPerTask, MetadataManagerTest::LockType::Write);
    }

    BOOST_AUTO_TEST_CASE(LockForReadAsync_WithExistingWriteLock_Success)
    {
        NTSTATUS status;

        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            const LONG64 transactionId = 1;

            // Expected
            KUri::CSPtr expectedName;
            status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            auto metadata = TestHelper::CreateMetadata(*expectedName, false, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);

            // Prepopulate
            bool isAdded = metadataManagerSPtr->TryAdd(*expectedName, *metadata);
            VERIFY_IS_TRUE(isAdded);

            // Create the text transaction
            TestTransaction::SPtr txnSPtr = TestTransaction::Create(transactionId, allocator);

            // Take the write lock and create a txnlockcontext for it.
            StateManagerLockContext::SPtr writeLockSPtr = nullptr;
            status = SyncAwait(metadataManagerSPtr->LockForWriteAsync(
                *expectedName,
                metadata->StateProviderId,
                *txnSPtr,
                TimeSpan::MaxValue,
                CancellationToken::None,
                writeLockSPtr));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            StateManagerTransactionContext::SPtr txnContextSPtr1 = nullptr;
            status = StateManagerTransactionContext::Create(transactionId, *writeLockSPtr, OperationType::Enum::Add, allocator, txnContextSPtr1);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            StateManagerLockContext::SPtr readLockSPtr = nullptr;
            status = SyncAwait(metadataManagerSPtr->LockForReadAsync(
                *expectedName, 
                metadata->StateProviderId, 
                *txnSPtr, 
                TimeSpan::MaxValue, 
                CancellationToken::None,
                readLockSPtr));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            StateManagerTransactionContext::SPtr txnContextSPtr2 = nullptr;
            status = StateManagerTransactionContext::Create(transactionId, *readLockSPtr, OperationType::Enum::Read, allocator, txnContextSPtr2);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            txnContextSPtr1->Unlock();
            txnContextSPtr2->Unlock();

            // Verification
            StateManagerLockContext::SPtr testLockContextSPtr = nullptr;
            bool lockContextExists = metadataManagerSPtr->TryGetLockContext(*metadata->Name, testLockContextSPtr);
            VERIFY_IS_TRUE(lockContextExists);
            VERIFY_ARE_EQUAL(0, testLockContextSPtr->GrantorCount);
            VERIFY_ARE_EQUAL(0, metadataManagerSPtr->GetInflightTransactionCount());
        }
    }

    BOOST_AUTO_TEST_CASE(LockForWriteAsync_WithExistingReadLock_Success)
    {
        NTSTATUS status;

        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            const LONG64 transactionId = 1;

            // Expected
            KUri::CSPtr expectedName;
            status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            auto metadata = TestHelper::CreateMetadata(*expectedName, false, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);

            // Prepopulate
            bool isAdded = metadataManagerSPtr->TryAdd(*expectedName, *metadata);
            VERIFY_IS_TRUE(isAdded);

            // Create the text transaction
            TestTransaction::SPtr txnSPtr = TestTransaction::Create(transactionId, allocator);

            StateManagerLockContext::SPtr readLockSPtr = nullptr;
            status = SyncAwait(metadataManagerSPtr->LockForReadAsync(
                *expectedName, 
                metadata->StateProviderId, 
                *txnSPtr, 
                TimeSpan::MaxValue, 
                CancellationToken::None,
                readLockSPtr));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            StateManagerTransactionContext::SPtr txnContextSPtr1 = nullptr;
            status = StateManagerTransactionContext::Create(transactionId, *readLockSPtr, OperationType::Enum::Read, allocator, txnContextSPtr1);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            StateManagerLockContext::SPtr writeLockSPtr = nullptr;
            status = SyncAwait(metadataManagerSPtr->LockForWriteAsync(
                *expectedName,
                metadata->StateProviderId,
                *txnSPtr,
                TimeSpan::MaxValue,
                CancellationToken::None,
                writeLockSPtr));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            StateManagerTransactionContext::SPtr txnContextSPtr2 = nullptr;
            status = StateManagerTransactionContext::Create(transactionId, *writeLockSPtr, OperationType::Enum::Add, allocator, txnContextSPtr2);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            txnContextSPtr1->Unlock();
            txnContextSPtr2->Unlock();

            // Verification
            StateManagerLockContext::SPtr testLockContextSPtr = nullptr;
            bool lockContextExists = metadataManagerSPtr->TryGetLockContext(*metadata->Name, testLockContextSPtr);
            VERIFY_IS_TRUE(lockContextExists);
            VERIFY_ARE_EQUAL(0, testLockContextSPtr->GrantorCount);
            VERIFY_ARE_EQUAL(0, metadataManagerSPtr->GetInflightTransactionCount());
        }
    }

    BOOST_AUTO_TEST_CASE(SoftDelete_Success)
    {
        NTSTATUS status;

        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            const LONG64 transactionId = 1;

            // Expected
            KUri::CSPtr expectedName;
            status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            auto metadata = TestHelper::CreateMetadata(*expectedName, false, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);
            bool isAdded = metadataManagerSPtr->TryAdd(*expectedName, *metadata);
            VERIFY_IS_TRUE(isAdded);

            // Create the text transaction
            TestTransaction::SPtr txnSPtr = TestTransaction::Create(transactionId, allocator);

            StateManagerLockContext::SPtr writeLockSPtr = nullptr;
            status = SyncAwait(metadataManagerSPtr->LockForWriteAsync(
                *expectedName,
                metadata->StateProviderId,
                *txnSPtr,
                TimeSpan::MaxValue,
                CancellationToken::None,
                writeLockSPtr));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            StateManagerTransactionContext::SPtr txnContextSPtr = nullptr;
            status = StateManagerTransactionContext::Create(transactionId, *writeLockSPtr, OperationType::Enum::Add, allocator, txnContextSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            metadataManagerSPtr->SoftDelete(*expectedName, MetadataMode::Enum::DelayDelete);

            txnContextSPtr->Unlock();

            // Verification
            StateManagerLockContext::SPtr testLockContextSPtr = nullptr;
            bool lockContextExists = metadataManagerSPtr->TryGetLockContext(*metadata->Name, testLockContextSPtr);
            VERIFY_IS_FALSE(lockContextExists);
        }
    }

    BOOST_AUTO_TEST_CASE(Abort_RemoveOperation_Success)
    {
        NTSTATUS status;

        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            const LONG64 transactionId = 1;

            // Expected
            KUri::CSPtr expectedName;
            status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            auto metadata = TestHelper::CreateMetadata(*expectedName, false, allocator);
            metadata->TransientDelete = true;

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);
            bool isAdded = metadataManagerSPtr->TryAdd(*expectedName, *metadata);
            VERIFY_IS_TRUE(isAdded);

            // Create the text transaction
            TestTransaction::SPtr txnSPtr = TestTransaction::Create(transactionId, allocator);

            StateManagerLockContext::SPtr writeLockSPtr = nullptr;
            status = SyncAwait(metadataManagerSPtr->LockForWriteAsync(
                *expectedName,
                metadata->StateProviderId,
                *txnSPtr,
                TimeSpan::MaxValue,
                CancellationToken::None,
                writeLockSPtr));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            StateManagerTransactionContext::SPtr txnContextSPtr = nullptr;
            status = StateManagerTransactionContext::Create(transactionId, *writeLockSPtr, OperationType::Enum::Remove, allocator, txnContextSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            txnContextSPtr->Unlock();

            // Verification
            StateManagerLockContext::SPtr testLockContextSPtr = nullptr;
            bool lockContextExists = metadataManagerSPtr->TryGetLockContext(*metadata->Name, testLockContextSPtr);
            VERIFY_IS_TRUE(lockContextExists);
            VERIFY_ARE_EQUAL(0, testLockContextSPtr->GrantorCount);
            VERIFY_ARE_EQUAL(0, metadataManagerSPtr->GetInflightTransactionCount());
        }
    }

    BOOST_AUTO_TEST_CASE(Commit_RemoveOperation_Success)
    {
        NTSTATUS status;

        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            const LONG64 transactionId = 1;

            // Expected
            KUri::CSPtr expectedName;
            status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            auto metadata = TestHelper::CreateMetadata(*expectedName, false, allocator);
            metadata->TransientDelete = true;
            metadata->MetadataMode = MetadataMode::DelayDelete;

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);
            auto result = metadataManagerSPtr->AddDeleted(metadata->StateProviderId, *metadata);
            VERIFY_IS_TRUE(result == true);

            // Create the text transaction
            TestTransaction::SPtr txnSPtr = TestTransaction::Create(transactionId, allocator);

            StateManagerLockContext::SPtr writeLockSPtr = nullptr;
            status = SyncAwait(metadataManagerSPtr->LockForWriteAsync(
                *expectedName,
                metadata->StateProviderId,
                *txnSPtr,
                TimeSpan::MaxValue,
                CancellationToken::None,
                writeLockSPtr));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            StateManagerTransactionContext::SPtr txnContextSPtr = nullptr;
            status = StateManagerTransactionContext::Create(transactionId, *writeLockSPtr, OperationType::Enum::Remove, allocator, txnContextSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            txnContextSPtr->Unlock();

            // Verification
            StateManagerLockContext::SPtr testLockContextSPtr = nullptr;
            bool lockContextExists = metadataManagerSPtr->TryGetLockContext(*metadata->Name, testLockContextSPtr);
            VERIFY_IS_FALSE(lockContextExists);
        }
    }

    BOOST_AUTO_TEST_CASE(Abort_ReadOperation_Success)
    {
        NTSTATUS status;

        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            const LONG64 transactionId = 1;

            // Expected
            KUri::CSPtr expectedName;
            status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            auto metadata = TestHelper::CreateMetadata(*expectedName, false, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);

            // Create the text transaction
            TestTransaction::SPtr txnSPtr = TestTransaction::Create(transactionId, allocator);

            StateManagerLockContext::SPtr readLockSPtr = nullptr;
            status = SyncAwait(metadataManagerSPtr->LockForReadAsync(
                *expectedName, 
                metadata->StateProviderId, 
                *txnSPtr, 
                TimeSpan::MaxValue, 
                CancellationToken::None,
                readLockSPtr));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            StateManagerTransactionContext::SPtr txnContextSPtr = nullptr;
            status = StateManagerTransactionContext::Create(transactionId, *readLockSPtr, OperationType::Enum::Read, allocator, txnContextSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            txnContextSPtr->Unlock();

            // Verification
            StateManagerLockContext::SPtr testLockContextSPtr = nullptr;
            bool lockContextExists = metadataManagerSPtr->TryGetLockContext(*metadata->Name, testLockContextSPtr);
            VERIFY_IS_TRUE(lockContextExists);
            VERIFY_ARE_EQUAL(0, testLockContextSPtr->GrantorCount);
            VERIFY_ARE_EQUAL(0, metadataManagerSPtr->GetInflightTransactionCount());
        }
    }

    BOOST_AUTO_TEST_CASE(LockForWriteAsync_WithExistingWriteLock_Success)
    {
        NTSTATUS status;

        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            const LONG64 transactionId = 1;

            // Expected
            KUri::CSPtr expectedName;
            status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            auto metadata = TestHelper::CreateMetadata(*expectedName, false, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);

            // Prepopulate
            bool isAdded = metadataManagerSPtr->TryAdd(*expectedName, *metadata);
            VERIFY_IS_TRUE(isAdded);

            // Create the text transaction
            TestTransaction::SPtr txnSPtr = TestTransaction::Create(transactionId, allocator);

            // Take the write lock and create a txnlockcontext for it.
            StateManagerLockContext::SPtr writeLockSPtr1 = nullptr;
            status = SyncAwait(metadataManagerSPtr->LockForWriteAsync(
                *expectedName,
                metadata->StateProviderId,
                *txnSPtr,
                TimeSpan::MaxValue,
                CancellationToken::None,
                writeLockSPtr1));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            StateManagerTransactionContext::SPtr txnContextSPtr1 = nullptr;
            status = StateManagerTransactionContext::Create(transactionId, *writeLockSPtr1, OperationType::Enum::Add, allocator, txnContextSPtr1);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            StateManagerLockContext::SPtr writeLockSPtr2 = nullptr;
            status = SyncAwait(metadataManagerSPtr->LockForWriteAsync(
                *expectedName,
                metadata->StateProviderId,
                *txnSPtr,
                TimeSpan::MaxValue,
                CancellationToken::None,
                writeLockSPtr2));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            StateManagerTransactionContext::SPtr txnContextSPtr2 = nullptr;
            status = StateManagerTransactionContext::Create(transactionId, *writeLockSPtr1, OperationType::Enum::Add, allocator, txnContextSPtr2);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            txnContextSPtr1->Unlock();
            txnContextSPtr2->Unlock();

            // Verification
            StateManagerLockContext::SPtr testLockContextSPtr = nullptr;
            bool lockContextExists = metadataManagerSPtr->TryGetLockContext(*metadata->Name, testLockContextSPtr);
            VERIFY_IS_TRUE(lockContextExists);
            VERIFY_ARE_EQUAL(0, testLockContextSPtr->GrantorCount);
            VERIFY_ARE_EQUAL(0, metadataManagerSPtr->GetInflightTransactionCount());
        }
    }

    // Add the transient metadata, and apply. The metadata is added
    BOOST_AUTO_TEST_CASE(AddScenario_WriteLock_Success)
    {
        NTSTATUS status;

        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Constants
            const long transactionId = 1;

            // Expected
            KUri::CSPtr expectedName;
            status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            auto metadata = TestHelper::CreateMetadata(*expectedName, true, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);

            // Create the text transaction
            TestTransaction::SPtr txnSPtr = TestTransaction::Create(transactionId, allocator);

            // Take the write lock and create a txnlockcontext for it.
            StateManagerLockContext::SPtr writeLockSPtr = nullptr;
            status = SyncAwait(metadataManagerSPtr->LockForWriteAsync(
                *expectedName,
                metadata->StateProviderId,
                *txnSPtr,
                TimeSpan::MaxValue,
                CancellationToken::None,
                writeLockSPtr));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            StateManagerTransactionContext::SPtr txnContextSPtr = nullptr;
            status = StateManagerTransactionContext::Create(transactionId, *writeLockSPtr, OperationType::Enum::Add, allocator, txnContextSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            // Logically apply the transaction.
            bool isAdded = metadataManagerSPtr->TryAdd(*expectedName, *metadata);
            VERIFY_IS_TRUE(isAdded);
            Metadata::SPtr metadataSPtr = nullptr;
            bool isExist = metadataManagerSPtr->TryGetMetadata(*expectedName, true, metadataSPtr);
            VERIFY_IS_TRUE(isExist);

            metadataSPtr->TransientCreate = false;

            txnContextSPtr->Unlock();

            bool result = metadataManagerSPtr->IsEmpty();
            VERIFY_IS_FALSE(result);

            // Verification
            StateManagerLockContext::SPtr testLockContextSPtr = nullptr;
            bool lockContextExists = metadataManagerSPtr->TryGetLockContext(*metadata->Name, testLockContextSPtr);
            VERIFY_IS_TRUE(lockContextExists);
            VERIFY_ARE_EQUAL(0, testLockContextSPtr->GrantorCount);
            VERIFY_ARE_EQUAL(0, metadataManagerSPtr->GetInflightTransactionCount());
        }
    }

    // Add the transient metadata, and abort. The metadata is not added
    BOOST_AUTO_TEST_CASE(AbortAddScenario_WriteLock_Success)
    {
        NTSTATUS status;

        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Constants
            const long transactionId = 1;

            // Expected
            KUri::CSPtr expectedName;
            status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            auto metadata = TestHelper::CreateMetadata(*expectedName, true, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);

            // Create the text transaction
            TestTransaction::SPtr txnSPtr = TestTransaction::Create(transactionId, allocator);

            // Take the write lock and create a txnlockcontext for it.
            StateManagerLockContext::SPtr writeLockSPtr = nullptr;
            status = SyncAwait(metadataManagerSPtr->LockForWriteAsync(
                *expectedName,
                metadata->StateProviderId,
                *txnSPtr,
                TimeSpan::MaxValue,
                CancellationToken::None,
                writeLockSPtr));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            StateManagerTransactionContext::SPtr txnContextSPtr = nullptr;
            status = StateManagerTransactionContext::Create(transactionId, *writeLockSPtr, OperationType::Enum::Add, allocator, txnContextSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            // Logically apply the transaction.
            bool isAdded = metadataManagerSPtr->TryAdd(*expectedName, *metadata);
            VERIFY_IS_TRUE(isAdded);

            txnContextSPtr->Unlock();

            // Verification
            bool result = metadataManagerSPtr->IsEmpty();
            VERIFY_IS_TRUE(result);
        }
    }

    BOOST_AUTO_TEST_CASE(GetOrAddScenario_Added_Success)
    {
        NTSTATUS status;

        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Constants
            const long transactionId = 1;

            // Expected
            KUri::CSPtr expectedName;
            status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            auto metadata = TestHelper::CreateMetadata(*expectedName, true, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);

            // Create the text transaction
            TestTransaction::SPtr txnSPtr = TestTransaction::Create(transactionId, allocator);

            // Take the read lock and release it.
            StateManagerLockContext::SPtr readLockSPtr = nullptr;
            status = SyncAwait(metadataManagerSPtr->LockForReadAsync(
                *expectedName, 
                metadata->StateProviderId, 
                *txnSPtr, 
                TimeSpan::MaxValue, 
                CancellationToken::None,
                readLockSPtr));
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            readLockSPtr->ReleaseLock(transactionId);

            // Take the write lock and create a txnlockcontext for it.
            StateManagerLockContext::SPtr writeLockSPtr = nullptr;
            status = SyncAwait(metadataManagerSPtr->LockForWriteAsync(
                *expectedName,
                metadata->StateProviderId,
                *txnSPtr,
                TimeSpan::MaxValue,
                CancellationToken::None,
                writeLockSPtr));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            StateManagerTransactionContext::SPtr txnContextSPtr = nullptr;
            status = StateManagerTransactionContext::Create(transactionId, *writeLockSPtr, OperationType::Enum::Add, allocator, txnContextSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            // Logically apply the transaction.
            bool isAdded = metadataManagerSPtr->TryAdd(*expectedName, *metadata);
            VERIFY_IS_TRUE(isAdded);
            Metadata::SPtr metadataSPtr = nullptr;
            bool isExist = metadataManagerSPtr->TryGetMetadata(*expectedName, true, metadataSPtr);
            VERIFY_IS_TRUE(isExist);

            metadataSPtr->TransientCreate = false;

            txnContextSPtr->Unlock();

            // Verification
            StateManagerLockContext::SPtr testLockContextSPtr = nullptr;
            bool lockContextExists = metadataManagerSPtr->TryGetLockContext(*metadata->Name, testLockContextSPtr);
            VERIFY_IS_TRUE(lockContextExists);
            VERIFY_ARE_EQUAL(0, testLockContextSPtr->GrantorCount);
            VERIFY_ARE_EQUAL(0, metadataManagerSPtr->GetInflightTransactionCount());
        }
    }

    BOOST_AUTO_TEST_CASE(GetOrAddScenario_Read_Success)
    {
        NTSTATUS status;

        // TODO: Fix the cyclic reference. Investigate the KFinally problem.
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Constants
            const long transactionId = 1;

            // Expected
            KUri::CSPtr expectedName;
            status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            auto metadata = TestHelper::CreateMetadata(*expectedName, false, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);

            // Create the text transaction
            TestTransaction::SPtr txnSPtr = TestTransaction::Create(transactionId, allocator);

            // Prepopulate
            bool isAdded = metadataManagerSPtr->TryAdd(*expectedName, *metadata);
            VERIFY_IS_TRUE(isAdded);

            // Take the read lock and release it.
            StateManagerLockContext::SPtr readLockSPtr = nullptr;
            status = SyncAwait(metadataManagerSPtr->LockForReadAsync(
                *expectedName, 
                metadata->StateProviderId, 
                *txnSPtr, 
                TimeSpan::MaxValue, 
                CancellationToken::None,
                readLockSPtr));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            StateManagerTransactionContext::SPtr txnContextSPtr = nullptr;
            status = StateManagerTransactionContext::Create(transactionId, *readLockSPtr, OperationType::Enum::Read, allocator, txnContextSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            txnContextSPtr->Unlock();

            // Verification
            StateManagerLockContext::SPtr testLockContextSPtr = nullptr;
            bool lockContextExists = metadataManagerSPtr->TryGetLockContext(*metadata->Name, testLockContextSPtr);
            VERIFY_IS_TRUE(lockContextExists);
            VERIFY_ARE_EQUAL(0, testLockContextSPtr->GrantorCount);
            VERIFY_ARE_EQUAL(0, metadataManagerSPtr->GetInflightTransactionCount());
        }
    }

    // Add a child when parent is exist, then GetChildrenMetadata
    BOOST_AUTO_TEST_CASE(AddAndTryGetMetadata_Parent_Exist)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Expected
            KUri::CSPtr expectedParentName;
            KUri::CSPtr expectedChildName;

            auto status = KUri::Create(KUriView(L"fabric:/sps/sp/parent"), allocator, (KUri::SPtr&)expectedParentName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = KUri::Create(KUriView(L"fabric:/sps/sp/child"), allocator, (KUri::SPtr&)expectedChildName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            auto metadata = TestHelper::CreateMetadata(*expectedParentName, false, allocator);
            auto childMetadata = TestHelper::CreateMetadata(*expectedChildName, false, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);
            bool isAdded = metadataManagerSPtr->TryAdd(*expectedParentName, *metadata);
            VERIFY_IS_TRUE(isAdded == true);
            metadataManagerSPtr->AddChild(metadata->StateProviderId, *childMetadata);

            // Test
            bool result = metadataManagerSPtr->ContainsKey(*expectedParentName);
            VERIFY_IS_TRUE(result == true);
            KSharedArray<Metadata::SPtr>::SPtr childrenMetadataArray = nullptr;
            auto childResult = metadataManagerSPtr->GetChildrenMetadata(metadata->StateProviderId, childrenMetadataArray);
            VERIFY_IS_TRUE(childResult == true);
            VERIFY_IS_TRUE(childrenMetadataArray->Count() == 1);

            VerifyMetadata((*childrenMetadataArray)[0].RawPtr(), childMetadata.RawPtr());
        }
    }

    BOOST_AUTO_TEST_CASE(AddAndGetMultiChildrenMetadata_Parent_Exist)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();
            KArray<Metadata::SPtr> expectedChildrenArray(allocator);

            // Expected
            KUri::CSPtr expectedParentName;
            KUri::CSPtr expectedChildName;
            ULONG32 const childrenNum = 10;

            auto status = KUri::Create(KUriView(L"fabric:/sps/sp/parent"), allocator, (KUri::SPtr&)expectedParentName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = KUri::Create(KUriView(L"fabric:/sps/sp/child"), allocator, (KUri::SPtr&)expectedChildName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            auto metadata = TestHelper::CreateMetadata(*expectedParentName, false, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);
            bool isAdded = metadataManagerSPtr->TryAdd(*expectedParentName, *metadata);
            VERIFY_IS_TRUE(isAdded == true);

            for (ULONG32 i = 0; i < childrenNum; i++)
            {
                auto childMetadata = TestHelper::CreateMetadata(*expectedChildName, false, allocator);
                status = expectedChildrenArray.Append(childMetadata);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                metadataManagerSPtr->AddChild(metadata->StateProviderId, *childMetadata);
            }

            // Test
            bool result = metadataManagerSPtr->ContainsKey(*expectedParentName);
            VERIFY_IS_TRUE(result == true);
            KSharedArray<Metadata::SPtr>::SPtr childrenMetadataArray = nullptr;
            auto childResult = metadataManagerSPtr->GetChildrenMetadata(metadata->StateProviderId, childrenMetadataArray);
            VERIFY_IS_TRUE(childResult == true);
            VERIFY_IS_TRUE(childrenMetadataArray->Count() == childrenNum);

            for (ULONG32 i = 0; i < childrenNum; i++)
            {
                VerifyMetadata(expectedChildrenArray[i].RawPtr(), (*childrenMetadataArray)[i].RawPtr());
            }
        }
    }

    // Add a child when parent is not exist, then GetChildrenMetadata
    BOOST_AUTO_TEST_CASE(AddAndTryGetMetadata_Parent_NotExist)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Expected
            KUri::CSPtr expectedChildName;
            const LONG64 stateProviderId = 0;

            auto status = KUri::Create(KUriView(L"fabric:/sps/sp/child"), allocator, (KUri::SPtr&)expectedChildName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            auto childMetadata = TestHelper::CreateMetadata(*expectedChildName, false, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);
            metadataManagerSPtr->AddChild(stateProviderId, *childMetadata);

            // Test
            auto result = metadataManagerSPtr->IsEmpty();
            VERIFY_IS_TRUE(result == true);
            KSharedArray<Metadata::SPtr>::SPtr childrenMetadataArray = nullptr;
            auto childResult = metadataManagerSPtr->GetChildrenMetadata(stateProviderId, childrenMetadataArray);
            VERIFY_IS_TRUE(childResult == true);
            VERIFY_IS_TRUE(childrenMetadataArray->Count() == 1);

            VerifyMetadata((*childrenMetadataArray)[0].RawPtr(), childMetadata.RawPtr());
        }
    }

    BOOST_AUTO_TEST_CASE(AddAndGetChildren_Children_NotExist)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Expected
            KUri::CSPtr expectedParentName;

            auto status = KUri::Create(KUriView(L"fabric:/sps/sp/parent"), allocator, (KUri::SPtr&)expectedParentName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            auto metadata = TestHelper::CreateMetadata(*expectedParentName, false, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);
            bool isAdded = metadataManagerSPtr->TryAdd(*expectedParentName, *metadata);
            VERIFY_IS_TRUE(isAdded == true);

            // Test
            bool result = metadataManagerSPtr->ContainsKey(*expectedParentName);
            VERIFY_IS_TRUE(result == true);
            KSharedArray<TxnReplicator::IStateProvider2::SPtr>::SPtr childrenStateProviderArray = nullptr;
            auto childResult = metadataManagerSPtr->GetChildren(metadata->StateProviderId, childrenStateProviderArray);
            VERIFY_IS_TRUE(childResult == false);
            VERIFY_IS_TRUE(childrenStateProviderArray == nullptr);
        }
    }

    BOOST_AUTO_TEST_CASE(AddAndGetChildren_Children_Exist)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Expected
            KUri::CSPtr expectedParentName;
            KUri::CSPtr expectedChildName;

            auto status = KUri::Create(KUriView(L"fabric:/sps/sp/parent"), allocator, (KUri::SPtr&)expectedParentName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = KUri::Create(KUriView(L"fabric:/sps/sp/child"), allocator, (KUri::SPtr&)expectedChildName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            auto metadata = TestHelper::CreateMetadata(*expectedParentName, false, allocator);
            auto childMetadata = TestHelper::CreateMetadata(*expectedChildName, false, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);
            bool isAdded = metadataManagerSPtr->TryAdd(*expectedParentName, *metadata);
            VERIFY_IS_TRUE(isAdded == true);
            metadataManagerSPtr->AddChild(metadata->StateProviderId, *childMetadata);

            // Test
            bool result = metadataManagerSPtr->ContainsKey(*expectedParentName);
            VERIFY_IS_TRUE(result == true);
            KSharedArray<TxnReplicator::IStateProvider2::SPtr>::SPtr childrenStateProviderArray = nullptr;
            auto childResult = metadataManagerSPtr->GetChildren(metadata->StateProviderId, childrenStateProviderArray);
            VERIFY_IS_TRUE(childResult == true);
            VERIFY_IS_TRUE(childrenStateProviderArray != nullptr);
            VERIFY_IS_TRUE(childrenStateProviderArray->Count() == 1);
            VERIFY_IS_TRUE((*childrenStateProviderArray)[0] != nullptr);
        }
    }

    BOOST_AUTO_TEST_CASE(AddAndGetMultiChildren_Children_Exist)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Expected
            KUri::CSPtr expectedParentName;
            KUri::CSPtr expectedChildName;
            ULONG32 const childrenNum = 10;

            auto status = KUri::Create(KUriView(L"fabric:/sps/sp/parent"), allocator, (KUri::SPtr&)expectedParentName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = KUri::Create(KUriView(L"fabric:/sps/sp/child"), allocator, (KUri::SPtr&)expectedChildName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            auto metadata = TestHelper::CreateMetadata(*expectedParentName, false, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);
            bool isAdded = metadataManagerSPtr->TryAdd(*expectedParentName, *metadata);
            VERIFY_IS_TRUE(isAdded == true);
            for (ULONG32 i = 0; i < childrenNum; i++)
            {
                auto childMetadata = TestHelper::CreateMetadata(*expectedChildName, false, allocator);
                metadataManagerSPtr->AddChild(metadata->StateProviderId, *childMetadata);
            }

            // Test
            bool result = metadataManagerSPtr->ContainsKey(*expectedParentName);
            VERIFY_IS_TRUE(result == true);
            KSharedArray<TxnReplicator::IStateProvider2::SPtr>::SPtr childrenStateProviderArray = nullptr;
            auto childResult = metadataManagerSPtr->GetChildren(metadata->StateProviderId, childrenStateProviderArray);
            VERIFY_IS_TRUE(childResult == true);
            VERIFY_IS_TRUE(childrenStateProviderArray != nullptr);
            VERIFY_IS_TRUE(childrenStateProviderArray->Count() == childrenNum);
        }
    }

    BOOST_AUTO_TEST_CASE(TryRemoveParent_NonExist)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Expected
            KUri::CSPtr expectedChildName;
            const LONG64 stateProviderId = 0;

            auto status = KUri::Create(KUriView(L"fabric:/sps/sp/child"), allocator, (KUri::SPtr&)expectedChildName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            auto childMetadata = TestHelper::CreateMetadata(*expectedChildName, false, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);

            // Test
            auto result = metadataManagerSPtr->IsEmpty();
            VERIFY_IS_TRUE(result == true);
            KSharedArray<Metadata::SPtr>::SPtr childrenMetadataArray = nullptr;
            result = metadataManagerSPtr->TryRemoveParent(stateProviderId, childrenMetadataArray);
            VERIFY_IS_TRUE(childrenMetadataArray == nullptr);
            VERIFY_IS_TRUE(result == false);
        }
    }

    BOOST_AUTO_TEST_CASE(TryRemoveParent_Exist)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Expected
            KUri::CSPtr expectedParentName;
            KUri::CSPtr expectedChildName;

            auto status = KUri::Create(KUriView(L"fabric:/sps/sp/parent"), allocator, (KUri::SPtr&)expectedParentName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = KUri::Create(KUriView(L"fabric:/sps/sp/child"), allocator, (KUri::SPtr&)expectedChildName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            auto metadata = TestHelper::CreateMetadata(*expectedParentName, false, allocator);
            auto childMetadata = TestHelper::CreateMetadata(*expectedChildName, false, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);
            bool isAdded = metadataManagerSPtr->TryAdd(*expectedParentName, *metadata);
            VERIFY_IS_TRUE(isAdded == true);
            metadataManagerSPtr->AddChild(metadata->StateProviderId, *childMetadata);

            // Test
            bool result = metadataManagerSPtr->ContainsKey(*expectedParentName);
            VERIFY_IS_TRUE(result == true);
            KSharedArray<Metadata::SPtr>::SPtr childrenMetadataArray = nullptr;
            auto childResult = metadataManagerSPtr->TryRemoveParent(metadata->StateProviderId, childrenMetadataArray);
            VERIFY_IS_TRUE(childResult == true);
            VERIFY_IS_TRUE(childrenMetadataArray->Count() == 1);
            VerifyMetadata((*childrenMetadataArray)[0].RawPtr(), childMetadata.RawPtr());
        }
    }

    BOOST_AUTO_TEST_CASE(TryRemoveParent_MultiChildren_Exist)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Expected
            KUri::CSPtr expectedParentName;
            KUri::CSPtr expectedChildName;
            ULONG32 const childrenNum = 10;

            NTSTATUS status = KUri::Create(KUriView(L"fabric:/sps/sp/parent"), allocator, (KUri::SPtr&)expectedParentName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = KUri::Create(KUriView(L"fabric:/sps/sp/child"), allocator, (KUri::SPtr&)expectedChildName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            auto metadata = TestHelper::CreateMetadata(*expectedParentName, false, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);
            bool isAdded = metadataManagerSPtr->TryAdd(*expectedParentName, *metadata);
            VERIFY_IS_TRUE(isAdded == true);
            KArray<Metadata::SPtr> expectedChildrenArray(allocator);

            for (ULONG32 i = 0; i < childrenNum; i++)
            {
                auto childMetadata = TestHelper::CreateMetadata(*expectedChildName, false, allocator);
                status = expectedChildrenArray.Append(childMetadata);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                metadataManagerSPtr->AddChild(metadata->StateProviderId, *childMetadata);
            }

            // Test
            bool result = metadataManagerSPtr->ContainsKey(*expectedParentName);
            VERIFY_IS_TRUE(result == true);
            KSharedArray<Metadata::SPtr>::SPtr childrenMetadataArray = nullptr;
            auto childResult = metadataManagerSPtr->TryRemoveParent(metadata->StateProviderId, childrenMetadataArray);
            VERIFY_IS_TRUE(childResult == true);
            VERIFY_IS_TRUE(childrenMetadataArray->Count() == childrenNum);

            for (ULONG32 i = 0; i < childrenNum; i++)
            {
                VerifyMetadata(expectedChildrenArray[i].RawPtr(), (*childrenMetadataArray)[i].RawPtr());
            }
        }
    }

    BOOST_AUTO_TEST_CASE(Create_ExistingSP_NoLockChange)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Expected
            KUri::CSPtr expectedName;

            auto status = KUri::Create(KUriView(L"fabric:/sps/expectedName"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            auto metadata1 = TestHelper::CreateMetadata(*expectedName, false, allocator);
            auto metadata2 = TestHelper::CreateMetadata(*expectedName, false, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);
            SyncAwait(metadataManagerSPtr->AcquireLockAndAddAsync(*expectedName, *metadata1, TimeSpan::MaxValue, CancellationToken::None));

            TestTransaction::SPtr txnSPtr = TestTransaction::Create(1, allocator);
            StateManagerLockContext::SPtr lockContextSPtr = nullptr;
            status = SyncAwait(metadataManagerSPtr->LockForWriteAsync(
                *expectedName,
                metadata2->StateProviderId,
                *txnSPtr,
                TimeSpan::MaxValue,
                CancellationToken::None,
                lockContextSPtr));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_ARE_EQUAL(1, lockContextSPtr->GrantorCount);
            VERIFY_IS_TRUE(lockContextSPtr != nullptr);

            // Test
            StateManagerTransactionContext::SPtr txnContextSPtr = nullptr;
            status = StateManagerTransactionContext::Create(1, *lockContextSPtr, OperationType::Enum::Remove, allocator, txnContextSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            txnContextSPtr->Unlock();

            bool lockContextExists = metadataManagerSPtr->TryGetLockContext(*metadata1->Name, lockContextSPtr);
            VERIFY_IS_TRUE(lockContextExists);
            VERIFY_ARE_EQUAL(0, lockContextSPtr->GrantorCount);
            VERIFY_ARE_EQUAL(0, metadataManagerSPtr->GetInflightTransactionCount());
        }
    }

    //
    // Scenario:        Test MoveStateProvidersToDeletedList moves all the active state providers to the deleted list
    // Expected Result: all moved to deleted list 
    // 
    BOOST_AUTO_TEST_CASE(MoveStateProvidersToDeletedList_ActiveAddedToDeleted)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Expected
            KUri::CSPtr expectedName1;
            KUri::CSPtr expectedName2;

            NTSTATUS status = KUri::Create(KUriView(L"fabric:/sps/expectedName1"), allocator, expectedName1);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = KUri::Create(KUriView(L"fabric:/sps/expectedName2"), allocator, expectedName2);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            Metadata::SPtr metadata1 = TestHelper::CreateMetadata(*expectedName1, false, allocator);
            Metadata::SPtr metadata2 = TestHelper::CreateMetadata(*expectedName2, false, allocator);

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);

            // Added active state provider 
            bool isAdded = metadataManagerSPtr->TryAdd(*expectedName1, *metadata1);
            VERIFY_IS_TRUE(isAdded == true);
            isAdded = metadataManagerSPtr->TryAdd(*expectedName2, *metadata2);
            VERIFY_IS_TRUE(isAdded == true);

            auto metadataEnumerator(metadataManagerSPtr->GetMetadataCollection());
            ULONG activeMetadataCount = 0;

            while (metadataEnumerator->MoveNext())
            {
                const Metadata::SPtr metadata(metadataEnumerator->Current().Value);
                if(*metadata->Name == *expectedName1)
                {
                    MetadataManagerTest::VerifyMetadata(metadata.RawPtr(), metadata1.RawPtr());
                }
                else if (*metadata->Name == *expectedName2)
                {
                    MetadataManagerTest::VerifyMetadata(metadata.RawPtr(), metadata2.RawPtr());
                }
                else
                {
                    VERIFY_IS_TRUE(false);
                }

                activeMetadataCount++;
            }

            VERIFY_IS_TRUE(activeMetadataCount == 2);
            metadataManagerSPtr->MoveStateProvidersToDeletedList();

            Metadata::SPtr returnedMetadata = nullptr;

            bool result = metadataManagerSPtr->TryGetDeletedMetadata(metadata1->StateProviderId, returnedMetadata);
            VERIFY_IS_TRUE(result);
            VerifyMetadata(metadata1.RawPtr(), returnedMetadata.RawPtr());

            result = metadataManagerSPtr->TryGetDeletedMetadata(metadata2->StateProviderId, returnedMetadata);
            VERIFY_IS_TRUE(result);
            VerifyMetadata(metadata2.RawPtr(), returnedMetadata.RawPtr());
        }
    }

    //
    // Scenario:        Test ResurrectStateProviderAsync added all the deleted state providers
    // Expected Result: all state providers in the deleted list are added
    // 
    BOOST_AUTO_TEST_CASE(ResurrectStateProviderAsync_AllAdded)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Expected
            KUri::CSPtr expectedName1;
            KUri::CSPtr expectedName2;

            NTSTATUS status = KUri::Create(KUriView(L"fabric:/sps/expectedName1"), allocator, expectedName1);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = KUri::Create(KUriView(L"fabric:/sps/expectedName2"), allocator, expectedName2);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            Metadata::SPtr metadata1 = TestHelper::CreateMetadata(*expectedName1, false, allocator);
            Metadata::SPtr metadata2 = TestHelper::CreateMetadata(*expectedName2, false, allocator);
            metadata1->MetadataMode = MetadataMode::FalseProgress;
            metadata2->MetadataMode = MetadataMode::FalseProgress;

            // Setup
            auto metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);

            // Added active state provider 
            bool isAdded = metadataManagerSPtr->AddDeleted(metadata1->StateProviderId, *metadata1);
            VERIFY_IS_TRUE(isAdded);
            isAdded = metadataManagerSPtr->AddDeleted(metadata2->StateProviderId, *metadata2);
            VERIFY_IS_TRUE(isAdded);

            // Should not have any active state provider or transient one
            bool result = metadataManagerSPtr->IsEmpty();
            VERIFY_IS_TRUE(result == true);

            auto resurrectMetadata1 = SyncAwait(metadataManagerSPtr->ResurrectStateProviderAsync(
                metadata1->StateProviderId, 
                Common::TimeSpan::MaxValue,
                CancellationToken::None));

            VerifyMetadata(metadata1.RawPtr(), resurrectMetadata1.RawPtr());

            auto resurrectMetadata2 = SyncAwait(metadataManagerSPtr->ResurrectStateProviderAsync(
                metadata2->StateProviderId,
                Common::TimeSpan::MaxValue,
                CancellationToken::None));

            VerifyMetadata(metadata2.RawPtr(), resurrectMetadata2.RawPtr());

            // Verify the Metadata is resurrected and the Metadata contents are correct.
            Metadata::SPtr returnedMetadata = nullptr;

            result = metadataManagerSPtr->ContainsKey(*expectedName1);
            VERIFY_IS_TRUE(result);
            bool isExist = metadataManagerSPtr->TryGetMetadata(*expectedName1, false, returnedMetadata);
            VERIFY_IS_TRUE(isExist);

            VerifyMetadata(metadata1.RawPtr(), returnedMetadata.RawPtr());

            result = metadataManagerSPtr->ContainsKey(*expectedName2);
            VERIFY_IS_TRUE(result);
            isExist = metadataManagerSPtr->TryGetMetadata(*expectedName2, false, returnedMetadata);
            VERIFY_IS_TRUE(isExist);

            VerifyMetadata(metadata2.RawPtr(), returnedMetadata.RawPtr());
        }
    }

    //
    // Scenario:        Dispose all LockContext in the keyLock dictionary.
    // Expected Result: all LockContext have been disposed
    // 
    BOOST_AUTO_TEST_CASE(Dispose_AllLockContext)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Expected
            KUri::CSPtr expectedName1;
            KUri::CSPtr expectedName2;

            NTSTATUS status = KUri::Create(KUriView(L"fabric:/sps/expectedName1"), allocator, expectedName1);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = KUri::Create(KUriView(L"fabric:/sps/expectedName2"), allocator, expectedName2);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            Metadata::SPtr metadata1 = TestHelper::CreateMetadata(*expectedName1, false, allocator);
            Metadata::SPtr metadata2 = TestHelper::CreateMetadata(*expectedName2, false, allocator);

            // Setup
            MetadataManager::SPtr metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);
            SyncAwait(metadataManagerSPtr->AcquireLockAndAddAsync(*expectedName1, *metadata1, TimeSpan::MaxValue, CancellationToken::None));
            SyncAwait(metadataManagerSPtr->AcquireLockAndAddAsync(*expectedName2, *metadata2, TimeSpan::MaxValue, CancellationToken::None));

            // Test
            metadataManagerSPtr->Dispose();

            status = STATUS_SUCCESS;
            StateManagerLockContext::SPtr lockContextSPtr1 = nullptr;
            metadataManagerSPtr->TryGetLockContext(*expectedName1, lockContextSPtr1);
            status = SyncAwait(lockContextSPtr1->AcquireWriteLockAsync(TimeSpan::MaxValue, CancellationToken::None));
            VERIFY_IS_TRUE(status == SF_STATUS_OBJECT_CLOSED);

            status = STATUS_SUCCESS;
            StateManagerLockContext::SPtr lockContextSPtr2 = nullptr;
            metadataManagerSPtr->TryGetLockContext(*expectedName2, lockContextSPtr2);
            status = SyncAwait(lockContextSPtr2->AcquireWriteLockAsync(TimeSpan::MaxValue, CancellationToken::None));
            VERIFY_IS_TRUE(status == SF_STATUS_OBJECT_CLOSED);
        }
    }

    //
    // Scenario:        Simulate the abort code path: Unlock got called before apply.
    //                  The LockContext is disposed.
    // Expected Result: The AcquireWriteLockAsync should throw object closed exception
    // 
    BOOST_AUTO_TEST_CASE(LockContext_Dispose_Throw)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Expected
            KUri::CSPtr expectedName;

            NTSTATUS status = KUri::Create(KUriView(L"fabric:/sps/expectedName"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            Metadata::SPtr metadata1 = TestHelper::CreateMetadata(*expectedName, true, allocator);

            // Setup
            MetadataManager::SPtr metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);
            SyncAwait(metadataManagerSPtr->AcquireLockAndAddAsync(*expectedName, *metadata1, TimeSpan::MaxValue, CancellationToken::None));

            TestTransaction::SPtr txnSPtr = TestTransaction::Create(1, allocator);
            StateManagerLockContext::SPtr lockContextSPtr = nullptr;
            status = SyncAwait(metadataManagerSPtr->LockForWriteAsync(
                *expectedName,
                metadata1->StateProviderId,
                *txnSPtr,
                TimeSpan::MaxValue,
                CancellationToken::None,
                lockContextSPtr));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_ARE_EQUAL(1, lockContextSPtr->GrantorCount);
            VERIFY_IS_TRUE(lockContextSPtr != nullptr);

            // Test
            StateManagerTransactionContext::SPtr txnContextSPtr = nullptr;
            status = StateManagerTransactionContext::Create(txnSPtr->TransactionId, *lockContextSPtr, OperationType::Enum::Add, allocator, txnContextSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = metadataManagerSPtr->Unlock(*txnContextSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = SyncAwait(lockContextSPtr->AcquireWriteLockAsync(TimeSpan::MaxValue, CancellationToken::None));
            VERIFY_IS_TRUE(status == SF_STATUS_OBJECT_DISPOSED);
        }
    }

    //
    // Scenario:        Create of an already existing state provider
    // Expected Result: No lock changes
    // 
    BOOST_AUTO_TEST_CASE(Create_ExistingStateProvider_NoLockChanges)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Expected
            KUri::CSPtr expectedName;

            NTSTATUS status = KUri::Create(KUriView(L"fabric:/sps/expectedName"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            Metadata::SPtr metadata1 = TestHelper::CreateMetadata(*expectedName, false, 7, allocator);
            Metadata::SPtr metadata2 = TestHelper::CreateMetadata(*expectedName, false, 8, allocator);

            // Setup
            MetadataManager::SPtr metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);
            SyncAwait(metadataManagerSPtr->AcquireLockAndAddAsync(*expectedName, *metadata1, TimeSpan::MaxValue, CancellationToken::None));

            TestTransaction::SPtr txnSPtr = TestTransaction::Create(1, allocator);
            StateManagerLockContext::SPtr lockContextSPtr = nullptr;
            status = SyncAwait(metadataManagerSPtr->LockForWriteAsync(
                *expectedName,
                metadata2->StateProviderId,
                *txnSPtr,
                TimeSpan::MaxValue,
                CancellationToken::None,
                lockContextSPtr));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            // assert lock key has been created
            VERIFY_IS_TRUE(lockContextSPtr != nullptr);
            VERIFY_ARE_EQUAL(1, lockContextSPtr->GrantorCount);

            // Test
            StateManagerTransactionContext::SPtr txnContextSPtr = nullptr;
            status = StateManagerTransactionContext::Create(txnSPtr->TransactionId, *lockContextSPtr, OperationType::Enum::Add, allocator, txnContextSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = metadataManagerSPtr->Unlock(*txnContextSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            // assert lock has not been been removed.
            bool lockContextExists = metadataManagerSPtr->TryGetLockContext(*metadata1->Name, lockContextSPtr);
            VERIFY_IS_TRUE(lockContextExists);
            VERIFY_IS_TRUE(lockContextSPtr != nullptr);
        }
    }

    //
    // Scenario:        Delete a state provider which has already been deleted
    // Expected Result: Remove lock on delete
    // 
    BOOST_AUTO_TEST_CASE(RemoveLock_Delete_DeletedStateProvider)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Expected
            KUri::CSPtr expectedName;

            NTSTATUS status = KUri::Create(KUriView(L"fabric:/sps/expectedName"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            Metadata::SPtr metadata1 = TestHelper::CreateMetadata(*expectedName, false, 7, allocator);
            metadata1->MetadataMode = MetadataMode::DelayDelete;

            // Setup
            MetadataManager::SPtr metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);

            metadataManagerSPtr->AddDeleted(metadata1->StateProviderId, *metadata1);

            TestTransaction::SPtr txnSPtr = TestTransaction::Create(1, allocator);
            StateManagerLockContext::SPtr lockContextSPtr = nullptr;
            status = SyncAwait(metadataManagerSPtr->LockForWriteAsync(
                *expectedName,
                metadata1->StateProviderId,
                *txnSPtr,
                TimeSpan::MaxValue,
                CancellationToken::None,
                lockContextSPtr));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            // assert lock key has been created
            VERIFY_IS_TRUE(lockContextSPtr != nullptr);
            VERIFY_ARE_EQUAL(1, lockContextSPtr->GrantorCount);

            // Test
            StateManagerTransactionContext::SPtr txnContextSPtr = nullptr;
            status = StateManagerTransactionContext::Create(txnSPtr->TransactionId, *lockContextSPtr, OperationType::Enum::Remove, allocator, txnContextSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = metadataManagerSPtr->Unlock(*txnContextSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            // assert lock has not been been removed.
            StateManagerLockContext::SPtr newLockContextSPtr;
            bool lockContextExists = metadataManagerSPtr->TryGetLockContext(*metadata1->Name, newLockContextSPtr);
            VERIFY_IS_TRUE(!lockContextExists);
            VERIFY_IS_TRUE(newLockContextSPtr == nullptr);
        }
    }

    //
    // Scenario:        Delete a state provider which does not  exist
    // Expected Result: Remove lock on delete
    // 
    BOOST_AUTO_TEST_CASE(RemoveLock_Delete_NonExistStateProvider)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // Expected
            KUri::CSPtr expectedName;

            NTSTATUS status = KUri::Create(KUriView(L"fabric:/sps/expectedName"), allocator, expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            Metadata::SPtr metadata = TestHelper::CreateMetadata(*expectedName, false, 7, allocator);
            metadata->MetadataMode = MetadataMode::DelayDelete;

            // Setup
            MetadataManager::SPtr metadataManagerSPtr = TestHelper::CreateMetadataManager(allocator);

            TestTransaction::SPtr txnSPtr = TestTransaction::Create(1, allocator);
            StateManagerLockContext::SPtr lockContextSPtr = nullptr;
            status = SyncAwait(metadataManagerSPtr->LockForWriteAsync(
                *expectedName,
                metadata->StateProviderId,
                *txnSPtr,
                TimeSpan::MaxValue,
                CancellationToken::None,
                lockContextSPtr));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            // assert lock key has been created
            VERIFY_IS_TRUE(lockContextSPtr != nullptr);
            VERIFY_ARE_EQUAL(1, lockContextSPtr->GrantorCount);

            // Test
            StateManagerTransactionContext::SPtr txnContextSPtr = nullptr;
            status = StateManagerTransactionContext::Create(txnSPtr->TransactionId, *lockContextSPtr, OperationType::Enum::Remove, allocator, txnContextSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = metadataManagerSPtr->Unlock(*txnContextSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            // assert lock has not been been removed.
            StateManagerLockContext::SPtr newLockContextSPtr;
            bool lockContextExists = metadataManagerSPtr->TryGetLockContext(*metadata->Name, newLockContextSPtr);
            VERIFY_IS_TRUE(!lockContextExists);
            VERIFY_IS_TRUE(newLockContextSPtr == nullptr);
        }
    }

    BOOST_AUTO_TEST_SUITE_END()

    ULONGLONG RandomNumber()
    {
        return rand();
    }
}
