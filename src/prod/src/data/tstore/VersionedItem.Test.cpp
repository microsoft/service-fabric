// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#define ALLOC_TAG 'tsTP'

namespace TStoreTests
{
    using namespace ktl;
    using namespace Data::TStore;


    class VersionedItemTest
    {
    public:
        VersionedItemTest()
        {
            NTSTATUS status;
            status = KtlSystem::Initialize(FALSE, &ktlSystem_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            ktlSystem_->SetStrictAllocationChecks(TRUE);
        }

        ~VersionedItemTest()
        {
            ktlSystem_->Shutdown();
        }

        KAllocator& GetAllocator()
        {
            return ktlSystem_->NonPagedAllocator();
        }

        ktl::Awaitable<void> AcquireLockAsync(__in InsertedVersionedItem<int> & versionedItem, __in KAllocator& allocator)
        {
            KSharedPtr<VersionedItem<int>> versionedItemSPtr = &versionedItem;

            // Introducing a timer just to make this routine async.
            NTSTATUS status = co_await KTimer::StartTimerAsync(
                allocator,
                ALLOC_TAG,
                5,
                nullptr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            versionedItemSPtr->AcquireLock();
            this->lockAcquired_ = true;
            co_return;
        }

        StoreTraceComponent::SPtr CreateTraceComponent()
        {
            KGuid guid;
            guid.CreateNew();
            ::FABRIC_REPLICA_ID replicaId = 1;
            int stateProviderId = 1;

            StoreTraceComponent::SPtr traceComponent = nullptr;
            NTSTATUS status = StoreTraceComponent::Create(guid, replicaId, stateProviderId, GetAllocator(), traceComponent);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            return traceComponent;
        }

        static void VerifyFlags(
            __in VersionedItem<int> & versionedItem,
            __in bool expectedInUseFlag,
            __in bool expectedIsInMemory,
            __in bool expectedLockBit,
            __in LONG64 expectedOffset)
        {
            VersionedItem<int>::SPtr outPtr(&versionedItem);

            outPtr->SetIsInMemory(expectedIsInMemory);
            outPtr->SetInUse(expectedInUseFlag);
            outPtr->SetLockBit(expectedLockBit);

            bool actualIsInMemory = outPtr->IsInMemory();
            bool actualInUse = outPtr->GetInUse();
            bool actualLockBit = outPtr->GetLockBit();
            LONG64 actualOffset = outPtr->GetOffset();

            CODING_ERROR_ASSERT(actualIsInMemory == expectedIsInMemory);
            CODING_ERROR_ASSERT(actualInUse == expectedInUseFlag);
            CODING_ERROR_ASSERT(actualOffset == expectedOffset);
            CODING_ERROR_ASSERT(actualLockBit == expectedLockBit);
        }

        static void VerifyVersionedItemFlags(
            __in bool expectedInUseFlag,
            __in bool expectedInMemory,
            __in bool expectedLockBit,
            __in LONG64 expectedOffset,
            __in RecordKind kind,
            __in KAllocator& allocator)
        {
            NTSTATUS status;
            if (kind == RecordKind::InsertedVersion)
            {
                InsertedVersionedItem<int>::SPtr outPtr = nullptr;
                status = InsertedVersionedItem<int>::Create(allocator, outPtr);
                CODING_ERROR_ASSERT(status == STATUS_SUCCESS);

                outPtr->InitializeOnRecovery(2, 2, expectedOffset, 2, 2);
                VerifyFlags(*outPtr, expectedInUseFlag, expectedInMemory, expectedLockBit, expectedOffset);
            }
            else
            {
                //TODO: The other type is now only Updated. Update the block once tests support more types.
                UpdatedVersionedItem<int>::SPtr outPtr = nullptr;
                status = UpdatedVersionedItem<int>::Create(allocator, outPtr);
                CODING_ERROR_ASSERT(status == STATUS_SUCCESS);

                outPtr->InitializeOnRecovery(2, 2, expectedOffset, 2, 2);
                VerifyFlags(*outPtr, expectedInUseFlag, expectedInMemory, expectedLockBit, expectedOffset);
            }
        }

        bool lockAcquired_ = false;

    private:
        KtlSystem* ktlSystem_;

#pragma region test functions
    public:
        ktl::Awaitable<void> Verify_LockAcquisition_Test()
        {
            KAllocator& allocator = VersionedItemTest::GetAllocator();
            InsertedVersionedItem<int>::SPtr versionedItemSPtr = nullptr;
            NTSTATUS status = InsertedVersionedItem<int>::Create(allocator, versionedItemSPtr);
            CODING_ERROR_ASSERT(status == STATUS_SUCCESS);

            // Assigning random values here, does not matter for this test.
            versionedItemSPtr->InitializeOnRecovery(2, 2, 1, 2, 2);

            this->lockAcquired_ = false;
            co_await AcquireLockAsync(*versionedItemSPtr, allocator);
            CODING_ERROR_ASSERT(this->lockAcquired_ == true);

            this->lockAcquired_ = false;
            ktl::Awaitable<void> lockTask = AcquireLockAsync(*versionedItemSPtr, allocator);
            CODING_ERROR_ASSERT(this->lockAcquired_ == false);

            status = co_await KTimer::StartTimerAsync(
                allocator,
                ALLOC_TAG,
                100,
                nullptr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            CODING_ERROR_ASSERT(this->lockAcquired_ == false);

            versionedItemSPtr->ReleaseLock(*CreateTraceComponent());

            co_await lockTask;
            CODING_ERROR_ASSERT(this->lockAcquired_ == true);
            co_return;
        }
#pragma endregion
   };

    BOOST_FIXTURE_TEST_SUITE(VersionedItemTestSuite, VersionedItemTest)

    BOOST_AUTO_TEST_CASE(InsertedVersion_VerifyInUseTrue_InMemoryTrue_LockBitTrue)
    {
        KAllocator& allocator = VersionedItemTest::GetAllocator();
        VerifyVersionedItemFlags(true, true, true, 500, RecordKind::InsertedVersion, allocator);
    }

    BOOST_AUTO_TEST_CASE(InsertedVersion_VerifyInUseTrue_InMemoryFalse_LockBitTrue)
    {
        KAllocator& allocator = VersionedItemTest::GetAllocator();
        VerifyVersionedItemFlags(true, false, true, 500, RecordKind::InsertedVersion, allocator);
    }

    BOOST_AUTO_TEST_CASE(InsertedVersion_VerifyInUseFalse_InMemoryTrue_LockBitTrue)
    {
        KAllocator& allocator = VersionedItemTest::GetAllocator();
        VerifyVersionedItemFlags(false, true, true, 500, RecordKind::InsertedVersion, allocator);
    }

    BOOST_AUTO_TEST_CASE(InsertedVersion_VerifyInUseFalse_InMemoryFalse_LockBitTrue)
    {
        KAllocator& allocator = VersionedItemTest::GetAllocator();
        VerifyVersionedItemFlags(false, false, true, 500, RecordKind::InsertedVersion, allocator);
    }

    BOOST_AUTO_TEST_CASE(InsertedVersion_VerifyInUseTrue_InMemoryTrue_LockBitFalse)
    {
        KAllocator& allocator = VersionedItemTest::GetAllocator();
        VerifyVersionedItemFlags(true, true, false, 500, RecordKind::InsertedVersion, allocator);
    }

    BOOST_AUTO_TEST_CASE(InsertedVersion_VerifyInUse_InMemoryFalse_LockBitFalse)
    {
        KAllocator& allocator = VersionedItemTest::GetAllocator();
        VerifyVersionedItemFlags(true, false, false, 500, RecordKind::InsertedVersion, allocator);
    }

    BOOST_AUTO_TEST_CASE(InsertedVersion_VerifyInUseFalse_InMemoryTrue_LockBitFalse)
    {
        KAllocator& allocator = VersionedItemTest::GetAllocator();
        VerifyVersionedItemFlags(false, true, false, 500, RecordKind::InsertedVersion, allocator);
    }

    BOOST_AUTO_TEST_CASE(InsertedVersion_VerifyInUseFalse_InMemoryFalse_LockBitFalse)
    {
        KAllocator& allocator = VersionedItemTest::GetAllocator();
        VerifyVersionedItemFlags(false, false, false, 500, RecordKind::InsertedVersion, allocator);
    }

    BOOST_AUTO_TEST_CASE(UpdatedVersion_VerifyInUseTrue_InMemoryTrue_LockBitTrue)
    {
        KAllocator& allocator = VersionedItemTest::GetAllocator();
        VerifyVersionedItemFlags(true, true, true, 500, RecordKind::UpdatedVersion, allocator);
    }

    BOOST_AUTO_TEST_CASE(UpdatedVersion_VerifyInUse_InMemoryFalse_LockBitTrue)
    {
        KAllocator& allocator = VersionedItemTest::GetAllocator();
        VerifyVersionedItemFlags(true, false, true, 500, RecordKind::UpdatedVersion, allocator);
    }

    BOOST_AUTO_TEST_CASE(UpdatedVersion_VerifyInUseFalse_InMemoryTrue_LockBitTrue)
    {
        KAllocator& allocator = VersionedItemTest::GetAllocator();
        VerifyVersionedItemFlags(false, true, true, 500, RecordKind::UpdatedVersion, allocator);
    }

    BOOST_AUTO_TEST_CASE(UpdatedVersion_VerifyInUseFalse_InMemoryFalse_LockBitTrue)
    {
        KAllocator& allocator = VersionedItemTest::GetAllocator();
        VerifyVersionedItemFlags(false, false, true, 500, RecordKind::UpdatedVersion, allocator);
    }
    
    BOOST_AUTO_TEST_CASE(UpdatedVersion_VerifyInUseTrue_InMemoryTrue_LockBitFalse)
    {
        KAllocator& allocator = VersionedItemTest::GetAllocator();
        VerifyVersionedItemFlags(true, true, false, 500, RecordKind::UpdatedVersion, allocator);
    }

    BOOST_AUTO_TEST_CASE(UpdatedVersion_VerifyInUse_InMemoryFalse_LockBitFalse)
    {
        KAllocator& allocator = VersionedItemTest::GetAllocator();
        VerifyVersionedItemFlags(true, false, false, 500, RecordKind::UpdatedVersion, allocator);
    }

    BOOST_AUTO_TEST_CASE(UpdatedVersion_VerifyInUseFalse_InMemoryTrue_LockBitFalse)
    {
        KAllocator& allocator = VersionedItemTest::GetAllocator();
        VerifyVersionedItemFlags(false, true, false, 500, RecordKind::UpdatedVersion, allocator);
    }

    BOOST_AUTO_TEST_CASE(UpdatedVersion_VerifyInUseFalse_InMemoryFalse_LockBitFalse)
    {
        KAllocator& allocator = VersionedItemTest::GetAllocator();
        VerifyVersionedItemFlags(false, false, false, 500, RecordKind::UpdatedVersion, allocator);
    }

    BOOST_AUTO_TEST_CASE(Verify_LockAcquisition)
    {
        SyncAwait(Verify_LockAcquisition_Test());
    }

    BOOST_AUTO_TEST_SUITE_END()
}
