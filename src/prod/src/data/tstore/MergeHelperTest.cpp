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
    using namespace Common;
    using namespace std;
    using namespace ktl;
    using namespace Data::TStore;

    struct MergeHelperTest
    {        
        MergeHelperTest() 
        {
            NTSTATUS status;
            status = KtlSystem::Initialize(FALSE, &ktlSystem_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status)==true);
            ktlSystem_->SetStrictAllocationChecks(TRUE);
        }

        ~MergeHelperTest()
        {
            ktlSystem_->Shutdown();
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

        KAllocator& GetAllocator()
        {
            return ktlSystem_->NonPagedAllocator();
        }

    private:
        KtlSystem* ktlSystem_;

#pragma region test functions
    public:
        ktl::Awaitable<void> VerifyDefaultValues_OnInitalization_Test()
        {
            KAllocator& allocator = MergeHelperTest::GetAllocator();
            MergeHelper::SPtr mh = nullptr;
            NTSTATUS mh_status = MergeHelper::Create(allocator, mh);
            CODING_ERROR_ASSERT(mh_status == STATUS_SUCCESS);
            CODING_ERROR_ASSERT(mh->CurrentMergePolicy == MergePolicy::All);
            CODING_ERROR_ASSERT(mh->MergeFilesCountThreshold == 3);
            CODING_ERROR_ASSERT(mh->PercentageOfInvalidEntriesPerFile == 33);
            co_return;
        }

        ktl::Awaitable<void> MergeList_OnInvalidentries_ShouldMatchMergeThreshold_Test()
        {
            KAllocator& allocator = GetAllocator();
            MergeHelper::SPtr mergeHelperSPtr = nullptr;
            auto status = MergeHelper::Create(allocator, mergeHelperSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            MetadataTable::SPtr metadataTableSPtr = nullptr;
            status = MetadataTable::Create(allocator, metadataTableSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            FileMetadata::SPtr fileMeta = nullptr;
            KString::SPtr fileNameSPtr = nullptr;
            KString::Create(fileNameSPtr, GetAllocator(), L"");
            status = FileMetadata::Create(1, *fileNameSPtr, 0, 0, 0, 0, false, allocator, *CreateTraceComponent(), fileMeta);
            mergeHelperSPtr->NumberOfInvalidEntries = 1;
            fileMeta->TotalNumberOfEntries = 2;
            fileMeta->NumberOfValidEntries = 0;
            ULONG32 fileId = 1;
            metadataTableSPtr->Table->Add(fileId, fileMeta);

            KSharedArray<ULONG32>::SPtr mergeList = mergeHelperSPtr->GetMergeList(*metadataTableSPtr);
            CODING_ERROR_ASSERT((*mergeList)[0] == 1);
            co_return;
        }

        ktl::Awaitable<void> GetMergeList_BasedOnPercentage_ShouldBeNonEmpty_Test()
        {
            KAllocator& allocator = GetAllocator();
            MergeHelper::SPtr mergeHelperSPtr = nullptr;
            auto status = MergeHelper::Create(allocator, mergeHelperSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            MetadataTable::SPtr metadataTableSPtr = nullptr;
            status = MetadataTable::Create(allocator, metadataTableSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            FileMetadata::SPtr fileMeta = nullptr;
            KString::SPtr fileNameSPtr = nullptr;
            KString::Create(fileNameSPtr, GetAllocator(), L"");
            status = FileMetadata::Create(1, *fileNameSPtr, 0, 0, 0, 0, false, allocator, *CreateTraceComponent(), fileMeta);
            fileMeta->TotalNumberOfEntries = 1;
            fileMeta->NumberOfValidEntries = 0;
            ULONG32 fileId = 1;
            metadataTableSPtr->Table->Add(fileId, fileMeta);

            KSharedArray<ULONG32>::SPtr mergeList = mergeHelperSPtr->GetMergeList(*metadataTableSPtr);
            CODING_ERROR_ASSERT((*mergeList)[0] == 1);
            co_return;
        }

        ktl::Awaitable<void> ShouldMerge_OneEmptyMetadataTable_ShouldReturnFalse_Test()
        {
            KAllocator& allocator = GetAllocator();
            MergeHelper::SPtr mergeHelperSPtr = nullptr;
            auto status = MergeHelper::Create(allocator, mergeHelperSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            MetadataTable::SPtr metadataTableSPtr = nullptr;
            status = MetadataTable::Create(allocator, metadataTableSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        
            KSharedArray<ULONG32>::SPtr mergeList = nullptr;
            bool res = co_await mergeHelperSPtr->ShouldMerge(*metadataTableSPtr, mergeList);
            CODING_ERROR_ASSERT(res == false);
            co_return;
        }

        ktl::Awaitable<void> InvalidEntriesPolicy_MergeListExceedsMergeFilesCountThreshold_ShouldMerge_Test()
        {
            KAllocator& allocator = GetAllocator();
            MergeHelper::SPtr mergeHelperSPtr = nullptr;
            auto status = MergeHelper::Create(allocator, mergeHelperSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            mergeHelperSPtr->MergeFilesCountThreshold = 1;

            MetadataTable::SPtr metadataTableSPtr = nullptr;
            status = MetadataTable::Create(allocator, metadataTableSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            FileMetadata::SPtr fileMeta = nullptr;
            KString::SPtr fileNameSPtr = nullptr;
            KString::Create(fileNameSPtr, GetAllocator(), L"");
            status = FileMetadata::Create(1, *fileNameSPtr, 0, 0, 0, 0, false, allocator, *CreateTraceComponent(), fileMeta);
            fileMeta->TotalNumberOfEntries = 1;
            fileMeta->NumberOfValidEntries = 0;
            ULONG32 fileId = 1;
            metadataTableSPtr->Table->Add(fileId, fileMeta);

            KSharedArray<ULONG32>::SPtr mergeList = nullptr;
            bool res = co_await mergeHelperSPtr->ShouldMerge(*metadataTableSPtr, mergeList);
            CODING_ERROR_ASSERT(res == true);
            CODING_ERROR_ASSERT((*mergeList)[0] == 1);
            co_return;
        }
    #pragma endregion
    };

    BOOST_FIXTURE_TEST_SUITE(MergeHelperTestSuite, MergeHelperTest)

    BOOST_AUTO_TEST_CASE(VerifyDefaultValues_OnInitalization)
    {
        SyncAwait(VerifyDefaultValues_OnInitalization_Test());
    }

    BOOST_AUTO_TEST_CASE(MergeList_OnInvalidentries_ShouldMatchMergeThreshold)
    {
        SyncAwait(MergeList_OnInvalidentries_ShouldMatchMergeThreshold_Test());
    }

    BOOST_AUTO_TEST_CASE(GetMergeList_BasedOnPercentage_ShouldBeNonEmpty)
    {
        SyncAwait(GetMergeList_BasedOnPercentage_ShouldBeNonEmpty_Test());
    }

    BOOST_AUTO_TEST_CASE(ShouldMerge_OneEmptyMetadataTable_ShouldReturnFalse)
    {
        SyncAwait(ShouldMerge_OneEmptyMetadataTable_ShouldReturnFalse_Test());
    }

    BOOST_AUTO_TEST_CASE(InvalidEntriesPolicy_MergeListExceedsMergeFilesCountThreshold_ShouldMerge)
    {
        SyncAwait(InvalidEntriesPolicy_MergeListExceedsMergeFilesCountThreshold_ShouldMerge_Test());
    }
    
    // This test requires additonal setup to work correctly
    //BOOST_AUTO_TEST_CASE(FileCountPolicy_OneFileTypeFileCountExceedsConfigThread_ShouldMerge)
    //{
    //    KAllocator& allocator = GetAllocator();
    //    MergeHelper::SPtr mergeHelperSPtr = nullptr;
    //    auto status = MergeHelper::Create(allocator, mergeHelperSPtr);
    //    CODING_ERROR_ASSERT(NT_SUCCESS(status));
    //    mergeHelperSPtr->MergeFilesCountThreshold = 1;

    //    MetadataTable::SPtr metadataTableSPtr = nullptr;
    //    status = MetadataTable::Create(allocator, metadataTableSPtr);
    //    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    //    FileMetadata::SPtr fileMeta = nullptr;
    //    KString::SPtr fileNameSPtr = nullptr;
    //    KString::Create(fileNameSPtr, GetAllocator(), L"");
    //    status = FileMetadata::Create(1, *fileNameSPtr, 0, 0, 0, 0, false, allocator, *CreateTraceComponent(), fileMeta);

    //    for (ULONG32 fileId = 0; fileId < 20; fileId++)
    //    {
    //        metadataTableSPtr->Table->Add(fileId, fileMeta);
    //    }

    //    KSharedArray<ULONG32>::SPtr mergeList = nullptr;
    //    bool res = SyncAwait(mergeHelperSPtr->ShouldMerge(*metadataTableSPtr, mergeList));
    //    CODING_ERROR_ASSERT(res == true);
    //    CODING_ERROR_ASSERT(mergeList->Count() == mergeHelperSPtr->FileCountMergeConfigurationSPtr->FileCountMergeThreshold);
    //}

    BOOST_AUTO_TEST_SUITE_END()
}

