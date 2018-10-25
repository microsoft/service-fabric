// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

#define ALLOC_TAG 'tsPC'

namespace TStoreTests
{
    using namespace ktl;
    using namespace Common;

    class MergeStressTest : public TStoreTestBase<KString::SPtr, KBuffer::SPtr, KStringComparer, StringStateSerializer, KBufferSerializer>
    {
    public:
        MergeStressTest()
        {
            Setup(1);
            Store->MergeHelperSPtr->CurrentMergePolicy = MergePolicy::InvalidEntries;
        }

        ~MergeStressTest()
        {
            Cleanup();
        }

        KString::SPtr CreateString(__in ULONG32 seed)
        {
            KString::SPtr key;
            wstring str = wstring(L"test_key") + to_wstring(seed);
            auto status = KString::Create(key, GetAllocator(), str.c_str());
            Diagnostics::Validate(status);
            return key;
        }

        KBuffer::SPtr CreateBuffer(__in byte fillValue, __in  ULONG32 size = 8)
        {
            KBuffer::SPtr bufferSptr;
            auto status = KBuffer::Create(size, bufferSptr, GetAllocator());
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            auto buffer = (byte *)bufferSptr->GetBuffer();
            memset(buffer, fillValue, size);

            return bufferSptr;
        }

        auto GetExpectedFileCount(ULONG32 numExpectedCheckpoints)
        {
            ULONG32 const numMetadataFiles = 1;
            ULONG32 const numFilesPerCheckpoint = 2;

            return numMetadataFiles + (numExpectedCheckpoints * numFilesPerCheckpoint);
        }

        
        bool IsMergePolicyEnabled(MergePolicy input, MergePolicy expected)
        {
            auto flagValue = static_cast<ULONG32>(input) & static_cast<ULONG32>(expected);
            return flagValue == static_cast<ULONG32>(expected);
        }

        Common::CommonConfig config; // load the config object as its needed for the tracing to work
    };

    BOOST_FIXTURE_TEST_SUITE(MergeStressTestSuite, MergeStressTest)

    BOOST_AUTO_TEST_CASE(Merge_DefaultSettings_SizeOnDiskPolicyNecessary_ShouldSucceed)
    {
        // Disable FileCount policy since SizeOnDisk is meant to replace it
        Store->MergeHelperSPtr->CurrentMergePolicy = static_cast<MergePolicy>(MergePolicy::All & ~MergePolicy::FileCount);

        // This test creates ~40 ~150MB checkpoints that will not pass any merge policy except the SizeOnDisk policy
        // NOTE: The count and value size were specifically chosen to get ~150MB checkpoint files. This test will break if there are changes to the serializer or file format
        ULONG32 countPerCheckpoint = 24000;
        auto originalValue = CreateBuffer('s', 5);
        auto updatedValue = CreateBuffer('t', 4800);

        ULONG32 intKey = 0;
        while (intKey != countPerCheckpoint)
        {
            auto key = CreateString(intKey);

            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, originalValue, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());

            intKey++;
        }

        // Checkpoint 37 times
        for (ULONG32 checkpointNum = 1; checkpointNum <= 37; checkpointNum++)
        {
            Checkpoint();
            CODING_ERROR_ASSERT(checkpointNum == Store->CurrentMetadataTableSPtr->Table->Count);

            // Invalidate 12.5% of original entries
            // Delete 12.5% of original entries
            // Add additional entries (75% of original entry count)
            ULONG32 startIntKey = intKey - (countPerCheckpoint / 4);

            for (intKey = startIntKey; intKey != startIntKey + countPerCheckpoint; intKey++)
            {
                auto key = CreateString(intKey);
                auto txn = CreateWriteTransaction();

                if (SyncAwait(Store->ContainsKeyAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None)))
                {
                    if (intKey % 2 == 0)
                    {
                        SyncAwait(Store->ConditionalUpdateAsync(
                            *txn->StoreTransactionSPtr,
                            key,
                            updatedValue,
                            DefaultTimeout,
                            CancellationToken::None));
                    }
                    else
                    {
                        SyncAwait(Store->ConditionalRemoveAsync(
                            *txn->StoreTransactionSPtr,
                            key,
                            DefaultTimeout,
                            CancellationToken::None));
                    }
                }
                else
                {
                    SyncAwait(Store->AddAsync(
                        *txn->StoreTransactionSPtr,
                        key,
                        updatedValue,
                        DefaultTimeout,
                        CancellationToken::None));
                }

                SyncAwait(txn->CommitAsync());
            }
        }

        ULONG64 totalSizeBefore = 0;
        auto enumeratorSPtr = Store->CurrentMetadataTableSPtr->Table->GetEnumerator();
        while (enumeratorSPtr->MoveNext())
        {
            auto fileMetadata = enumeratorSPtr->Current().Value;
            totalSizeBefore += SyncAwait(fileMetadata->GetFileSizeAsync());
        }

        // Checkpoint 38th time (three deltas before consolidation), causing a massive merge to a single checkpoint
        Checkpoint();

        CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->Table->Count == 1);

        ULONG64 totalSizeAfter = 0;
        enumeratorSPtr = Store->CurrentMetadataTableSPtr->Table->GetEnumerator();
        while (enumeratorSPtr->MoveNext())
        {
            auto fileMetadata = enumeratorSPtr->Current().Value;
            totalSizeAfter += SyncAwait(fileMetadata->GetFileSizeAsync());
        }

        CODING_ERROR_ASSERT(totalSizeAfter < totalSizeBefore);

        // There should be only 1 checkpoint
        ULONG32 const numExpectedCheckpoints = 1;
        auto files = Common::Directory::GetFiles(Store->WorkingDirectoryCSPtr->operator LPCWSTR());
        CODING_ERROR_ASSERT(GetExpectedFileCount(numExpectedCheckpoints) == files.size());
    }

    BOOST_AUTO_TEST_CASE(Merge_DefaultSettings_3HugeFiles_NoInvalidOrDeletedEntries_ShouldNotMerge)
    {
        // Disable FileCount policy since SizeOnDisk is meant to replace it
        Store->MergeHelperSPtr->CurrentMergePolicy = static_cast<MergePolicy>(MergePolicy::All & ~MergePolicy::FileCount);

        // This test creates ~3 ~2GB checkpoints with no invalid or deleted entries
        // NOTE: The count and value size were specifically chosen to get ~2GB checkpoint files
        ULONG32 countPerCheckpoint = 160000;
        auto value = CreateBuffer('s', 12000);

        for (ULONG32 checkpointNum = 1; checkpointNum <= Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated; checkpointNum++)
        {
            for (ULONG keyOffset = 0; keyOffset < countPerCheckpoint; keyOffset++)
            {
                auto key = CreateString((checkpointNum - 1) * countPerCheckpoint + keyOffset);
                auto txn = CreateWriteTransaction();

                SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
                SyncAwait(txn->CommitAsync());
            }

            Checkpoint();
            CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->Table->Count == checkpointNum);
        }

        // There should be three checkpoints
        ULONG32 const numExpectedCheckpoints = 3;
        auto files = Common::Directory::GetFiles(Store->WorkingDirectoryCSPtr->operator LPCWSTR());
        CODING_ERROR_ASSERT(GetExpectedFileCount(numExpectedCheckpoints) == files.size());
    }

    BOOST_AUTO_TEST_CASE(Merge_DefaultSettings_3HugeFiles_JustUnderThreshold_ShouldNotMerge)
    {
        // Disable FileCount policy since SizeOnDisk is meant to replace it
        Store->MergeHelperSPtr->CurrentMergePolicy = static_cast<MergePolicy>(MergePolicy::All & ~MergePolicy::FileCount);

        // This test creates 3 <2GB checkpoints
        // NOTE: The count and value size were specifically chosen to get ~2GB checkpoint files
        ULONG32 countPerCheckpoint = 160000;
        auto value = CreateBuffer('s', 12125);

        ULONG32 intKey = 0;
        while (intKey != countPerCheckpoint)
        {
            auto key = CreateString(intKey);

            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());

            intKey++;
        }

        Checkpoint();
        CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->Table->Count == 1);

        // Checkpoint 37 times
        for (ULONG32 checkpointNum = 2; checkpointNum <= 3; checkpointNum++)
        {
            // Invalidate 12.5% of original entries
            // Delete 12.5% of original entries
            // Add additional entries (75% of original entry count)
            ULONG32 startIntKey = intKey - (countPerCheckpoint / 4);

            for (intKey = startIntKey; intKey != startIntKey + countPerCheckpoint; intKey++)
            {
                auto key = CreateString(intKey);
                auto txn = CreateWriteTransaction();

                if (SyncAwait(Store->ContainsKeyAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None)))
                {
                    if (intKey % 2 == 0)
                    {
                        SyncAwait(Store->ConditionalUpdateAsync(
                            *txn->StoreTransactionSPtr,
                            key,
                            value,
                            DefaultTimeout,
                            CancellationToken::None));
                    }
                    else
                    {
                        SyncAwait(Store->ConditionalRemoveAsync(
                            *txn->StoreTransactionSPtr,
                            key,
                            DefaultTimeout,
                            CancellationToken::None));
                    }
                }
                else
                {
                    SyncAwait(Store->AddAsync(
                        *txn->StoreTransactionSPtr,
                        key,
                        value,
                        DefaultTimeout,
                        CancellationToken::None));
                }

                SyncAwait(txn->CommitAsync());
            }

            Checkpoint();
            CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->Table->Count == checkpointNum);
        }

        // There should be three checkpoints
        ULONG32 const numExpectedCheckpoints = 3;
        auto files = Common::Directory::GetFiles(Store->WorkingDirectoryCSPtr->operator LPCWSTR());
        CODING_ERROR_ASSERT(GetExpectedFileCount(numExpectedCheckpoints) == files.size());
    }

    BOOST_AUTO_TEST_SUITE_END();
}
