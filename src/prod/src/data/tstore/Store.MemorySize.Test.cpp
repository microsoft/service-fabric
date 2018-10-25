// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

#define ALLOC_TAG 'ssTP'

inline bool SingleElementBufferEquals(KBuffer::SPtr & one, KBuffer::SPtr & two)
{
   auto oneNum = (ULONG32 *)one->GetBuffer();
   auto twoNum = (ULONG32 *)two->GetBuffer();

   return *oneNum == *twoNum;
}

namespace TStoreTests
{
    class StoreMemorySizeTest : public TStoreTestBase<KBuffer::SPtr, KBuffer::SPtr, KBufferComparer, KBufferSerializer, KBufferSerializer>
    {
    public:
        StoreMemorySizeTest()
        {
            Setup(1);
            Store->EnableSweep = false;
        }

        ~StoreMemorySizeTest()
        {
            Cleanup();
        }

        KBuffer::SPtr CreateBuffer(__in LONG64 size, __in byte fillValue=0xb6)
        {
            CODING_ERROR_ASSERT(size >= 0);

            KBuffer::SPtr bufferSPtr = nullptr;
            NTSTATUS status = KBuffer::Create(static_cast<ULONG>(size), bufferSPtr, GetAllocator());
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            byte* buffer = static_cast<byte *>(bufferSPtr->GetBuffer());
            memset(buffer, fillValue, static_cast<ULONG>(size));

            return bufferSPtr;
        }

        LONG64 GetSerializedSize(__in KBuffer & buffer)
        {
            BinaryWriter writer(GetAllocator());
            KBuffer::SPtr bufferSPtr = &buffer;
            Store->ValueConverterSPtr->Write(bufferSPtr, writer);
            return writer.GetBuffer(0)->QuerySize();
        }
        
        LONG64 GetKeysAndMetadataSize(
            __in LONG64 estimatedKeySize,
            __in ULONG32 numDifferential, 
            __in ULONG32 numNonDifferential)
        {
            return numDifferential * (2 * Constants::ValueMetadataSizeBytes + estimatedKeySize) + numNonDifferential * (estimatedKeySize + Constants::ValueMetadataSizeBytes);
        }

        Awaitable<void> VariableKeySizes_MultipleInDifferential_TestAsync()
        {
            Store->EnableSweep = false;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;
            Store->MergeHelperSPtr->CurrentMergePolicy = MergePolicy::None;
               
            KBuffer::SPtr key1 = CreateBuffer(8);
            KBuffer::SPtr key2 = CreateBuffer(16);
            KBuffer::SPtr key3 = CreateBuffer(32);

            KBuffer::SPtr value1 = CreateBuffer(32);
            KBuffer::SPtr value2 = CreateBuffer(16);
            KBuffer::SPtr value3 = CreateBuffer(8);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key1, value1, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            LONG64 totalKeySize = GetSerializedSize(*key1);
            LONG64 expectedEstimate = totalKeySize / 1;
            CODING_ERROR_ASSERT(Store->GetEstimatedKeySize() == expectedEstimate);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key2, value2, DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key3, value3, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            totalKeySize += GetSerializedSize(*key2) + GetSerializedSize(*key3);
            expectedEstimate = totalKeySize / 3;
            CODING_ERROR_ASSERT(Store->GetEstimatedKeySize() == expectedEstimate);
            
            LONG64 expectedSize = GetKeysAndMetadataSize(expectedEstimate, 3, 0) + GetSerializedSize(*value1) + GetSerializedSize(*value2) + GetSerializedSize(*value3);
            CODING_ERROR_ASSERT(Store->GetMemorySize() == expectedSize);
        }

        Awaitable<void> VariableKeySizes_InDifferentialAndConsolidated_TestAsync()
        {
            Store->EnableSweep = false;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;
            Store->MergeHelperSPtr->CurrentMergePolicy = MergePolicy::None;
               
            KBuffer::SPtr key1 = CreateBuffer(8);
            KBuffer::SPtr key2 = CreateBuffer(16);
            KBuffer::SPtr key3 = CreateBuffer(32);

            KBuffer::SPtr value1 = CreateBuffer(32);
            KBuffer::SPtr value2 = CreateBuffer(16);
            KBuffer::SPtr value3 = CreateBuffer(8);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key1, value1, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            LONG64 totalKeySize = GetSerializedSize(*key1);
            LONG64 expectedEstimate = totalKeySize / 1;
            CODING_ERROR_ASSERT(Store->GetEstimatedKeySize() == expectedEstimate);

            co_await CheckpointAsync();

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key2, value2, DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key3, value3, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            totalKeySize = GetSerializedSize(*key2) + GetSerializedSize(*key3);
            expectedEstimate = totalKeySize / 2;
            CODING_ERROR_ASSERT(Store->GetEstimatedKeySize() == expectedEstimate);
            
            LONG64 expectedSize = GetKeysAndMetadataSize(expectedEstimate, 2, 1) + GetSerializedSize(*value1) + GetSerializedSize(*value2) + GetSerializedSize(*value3);
            CODING_ERROR_ASSERT(Store->GetMemorySize() == expectedSize);
        }

        Awaitable<void> VariableKeySizes_KeyRemoved_AffectsAverage_TestAsync()
        {
            Store->EnableSweep = false;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;
            Store->MergeHelperSPtr->CurrentMergePolicy = MergePolicy::None;

            KBuffer::SPtr key1 = CreateBuffer(256); // Even though this key will be removed, it will still affect the average
            KBuffer::SPtr key2 = CreateBuffer(16);
            KBuffer::SPtr key3 = CreateBuffer(32);

            KBuffer::SPtr value1 = CreateBuffer(32);
            KBuffer::SPtr value2 = CreateBuffer(16);
            KBuffer::SPtr value3 = CreateBuffer(8);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key1, value1, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            LONG64 totalKeySize = GetSerializedSize(*key1);
            LONG64 expectedEstimate = totalKeySize / 1;

            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key1,  DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            CODING_ERROR_ASSERT(Store->GetEstimatedKeySize() == expectedEstimate);

            co_await CheckpointAsync();

            CODING_ERROR_ASSERT(Store->GetEstimatedKeySize() == expectedEstimate);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key2, value2, DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key3, value3, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            totalKeySize = GetSerializedSize(*key2) + GetSerializedSize(*key3);
            expectedEstimate = totalKeySize / 2;
            CODING_ERROR_ASSERT(Store->GetEstimatedKeySize() == expectedEstimate);
            
            LONG64 expectedSize = GetKeysAndMetadataSize(expectedEstimate, 2, 0) + GetSerializedSize(*value2) + GetSerializedSize(*value3);
            CODING_ERROR_ASSERT(Store->GetMemorySize() == expectedSize);
        }

        Awaitable<void> VariableKeySizes_DeletedKeysInCheckpoint_AffectsAverage_AfterRecovery_TestAsync()
        {
            Store->EnableSweep = false;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;
            Store->MergeHelperSPtr->CurrentMergePolicy = MergePolicy::None;

            // These keys will be removed in checkpoints, but will still affect the average
            KBuffer::SPtr key1 = CreateBuffer(1024); 
            KBuffer::SPtr key2 = CreateBuffer(256);
            KBuffer::SPtr key3 = CreateBuffer(512);

            KBuffer::SPtr key4 = CreateBuffer(32);

            KBuffer::SPtr value1 = CreateBuffer(32);
            KBuffer::SPtr value2 = CreateBuffer(16);
            KBuffer::SPtr value3 = CreateBuffer(8);
            KBuffer::SPtr value4 = CreateBuffer(8);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key1, value1, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();

            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key1,  DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key2, value2, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();

            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key2,  DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key3, value3, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();

            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key3,  DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();
            co_await CloseAndReOpenStoreAsync();

            LONG64 totalKeySize = (2 * GetSerializedSize(*key1)) + (2 * GetSerializedSize(*key2)) + (2 * GetSerializedSize(*key3));
            LONG64 expectedEstimate = totalKeySize / 6;
            CODING_ERROR_ASSERT(Store->GetEstimatedKeySize() == expectedEstimate);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key4, value4, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            totalKeySize = GetSerializedSize(*key4);
            expectedEstimate = totalKeySize / 1;
            CODING_ERROR_ASSERT(Store->GetEstimatedKeySize() == expectedEstimate);
            
            LONG64 expectedSize = GetKeysAndMetadataSize(expectedEstimate, 1, 0) + GetSerializedSize(*value4);
            CODING_ERROR_ASSERT(Store->GetMemorySize() == expectedSize);
        }

        Common::CommonConfig config; // load the config object as its needed for the tracing to work

#pragma region test functions
    public:
        ktl::Awaitable<void> EmptyStore_ZeroSize_ShouldSucceed_Test()
        {
            LONG64 size = Store->Size;
            CODING_ERROR_ASSERT(size == 0);
            co_return;
        }

        ktl::Awaitable<void> Differential_1Item_ShouldSucceed_Test()
        {
            Store->EnableSweep = false;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            KBuffer::SPtr key = CreateBuffer(8);
            KBuffer::SPtr value = CreateBuffer(128);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }
        
            LONG64 totalKeySize = GetSerializedSize(*key);
            LONG64 expectedEstimate = totalKeySize / 1;
            CODING_ERROR_ASSERT(Store->GetEstimatedKeySize() == expectedEstimate);

            // Differential: 1 key with 1 value
            LONG64 expectedSize = GetSerializedSize(*value) + GetKeysAndMetadataSize(expectedEstimate, 1, 0);
            LONG64 actualSize = Store->Size;
            CODING_ERROR_ASSERT(actualSize == expectedSize);
            co_return;
        }

        ktl::Awaitable<void> Differential_AddUniqueItem_ShouldSucceed_Test()
        {
            Store->EnableSweep = false;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            LONG64 value1Size = 128;
            LONG64 value2Size = 64;

            KBuffer::SPtr key1 = CreateBuffer(8, 8);
            KBuffer::SPtr key2 = CreateBuffer(8, 10);

            KBuffer::SPtr value1 = CreateBuffer(value1Size);
            KBuffer::SPtr value2 = CreateBuffer(value2Size);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key1, value1, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key2, value2, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            LONG64 totalKeySize = GetSerializedSize(*key1) + GetSerializedSize(*key2);
            LONG64 expectedEstimate = totalKeySize / 2;
            CODING_ERROR_ASSERT(Store->GetEstimatedKeySize() == expectedEstimate);

            // Differential: 2 keys, each with 1 value -> 2 keys, 2 values
            LONG64 expectedSize = GetSerializedSize(*value1) + GetSerializedSize(*value2) + GetKeysAndMetadataSize(expectedEstimate, 2, 0);
            LONG64 actualSize = Store->Size;
            CODING_ERROR_ASSERT(actualSize == expectedSize);
            co_return;
        }

        ktl::Awaitable<void> Differential_UpdateExisting_ShouldSucceed_Test()
        {
            Store->EnableSweep = false;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            LONG64 value1Size = 128;
            LONG64 value2Size = 64;

            KBuffer::SPtr key = CreateBuffer(8);

            KBuffer::SPtr value1 = CreateBuffer(value1Size);
            KBuffer::SPtr value2 = CreateBuffer(value2Size);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value1, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value2, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            LONG64 totalKeySize = GetSerializedSize(*key);
            LONG64 expectedEstimate = totalKeySize / 1;
            CODING_ERROR_ASSERT(Store->GetEstimatedKeySize() == expectedEstimate);
        
            // Differential: 1 key with 1 current value and 1 previous value -> 1 key and 2 values
            LONG64 expectedSize = GetSerializedSize(*value1) + GetSerializedSize(*value2) + GetKeysAndMetadataSize(expectedEstimate, 1, 0);
            LONG64 actualSize = Store->Size;
            CODING_ERROR_ASSERT(actualSize == expectedSize);
            co_return;
        }

        ktl::Awaitable<void> Differential_UpdateExisting_SameTransaction_ShouldSucceed_Test()
        {
            Store->EnableSweep = false;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            LONG64 value1Size = 128;
            LONG64 value2Size = 64;

            KBuffer::SPtr key = CreateBuffer(8);

            KBuffer::SPtr value1 = CreateBuffer(value1Size);
            KBuffer::SPtr value2 = CreateBuffer(value2Size);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value1, DefaultTimeout, ktl::CancellationToken::None);
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value2, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            LONG64 totalKeySize = GetSerializedSize(*key);
            LONG64 expectedEstimate = totalKeySize / 1;
            CODING_ERROR_ASSERT(Store->GetEstimatedKeySize() == expectedEstimate);

            // Differential: 1 key with 1 value
            LONG64 expectedSize = GetSerializedSize(*value2) + GetKeysAndMetadataSize(expectedEstimate, 1, 0);
            LONG64 actualSize = Store->Size;
            CODING_ERROR_ASSERT(actualSize == expectedSize);
            co_return;
        }

        ktl::Awaitable<void> Differential_UpdateTwice_ShouldSucceed_Test()
        {
            Store->EnableSweep = false;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            LONG64 value1Size = 128;
            LONG64 value2Size = 64;
            LONG64 value3Size = 256;

            KBuffer::SPtr key = CreateBuffer(8);

            KBuffer::SPtr value1 = CreateBuffer(value1Size);
            KBuffer::SPtr value2 = CreateBuffer(value2Size);
            KBuffer::SPtr value3 = CreateBuffer(value3Size);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value1, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value2, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value3, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            LONG64 totalKeySize = GetSerializedSize(*key);
            LONG64 expectedEstimate = totalKeySize / 1;
            CODING_ERROR_ASSERT(Store->GetEstimatedKeySize() == expectedEstimate);
        
            // Differential: 1 key with 1 previous value and 1 current value -> 1 key, 2 values
            LONG64 expectedSize = GetSerializedSize(*value2) + GetSerializedSize(*value3) + GetKeysAndMetadataSize(expectedEstimate, 1, 0);
            LONG64 actualSize = Store->Size;
            CODING_ERROR_ASSERT(actualSize == expectedSize);
            co_return;
        }

        ktl::Awaitable<void> Consolidated_SingleItem_ShouldSucceed_Test()
        {
            Store->EnableSweep = false;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            KBuffer::SPtr key = CreateBuffer(8);
            KBuffer::SPtr value = CreateBuffer(128);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();

            LONG64 totalKeySize = GetSerializedSize(*key);
            LONG64 expectedEstimate = totalKeySize / 1;
            CODING_ERROR_ASSERT(Store->GetEstimatedKeySize() == expectedEstimate);

            // Consolidated: 1 key and 1 value
            LONG64 expectedSize = GetSerializedSize(*value) + GetKeysAndMetadataSize(expectedEstimate, 0, 1);
            LONG64 actualSize = Store->Size;
            CODING_ERROR_ASSERT(actualSize == expectedSize);
            co_return;
        }

        ktl::Awaitable<void> Consolidated_AddUniqueItem_ShouldSucceed_Test()
        {
            Store->EnableSweep = false;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            LONG64 value1Size = 128;
            LONG64 value2Size = 64;

            KBuffer::SPtr key1 = CreateBuffer(8, 8);
            KBuffer::SPtr key2 = CreateBuffer(8, 10);

            KBuffer::SPtr value1 = CreateBuffer(value1Size);
            KBuffer::SPtr value2 = CreateBuffer(value2Size);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key1, value1, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key2, value2, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();

            LONG64 totalKeySize = GetSerializedSize(*key1) + GetSerializedSize(*key2);
            LONG64 expectedEstimate = totalKeySize / 2;
            CODING_ERROR_ASSERT(Store->GetEstimatedKeySize() == expectedEstimate);

            // Consolidated: 2 keys, 1 value each -> 2 keys, 2 values
            LONG64 expectedSize = GetSerializedSize(*value1) + GetSerializedSize(*value2) + GetKeysAndMetadataSize(expectedEstimate, 0, 2);
            LONG64 actualSize = Store->Size;
            CODING_ERROR_ASSERT(actualSize == expectedSize);
            co_return;
        }

        ktl::Awaitable<void> Consolidated_UpdateExisting_ShouldSucceed_Test()
        {
            Store->EnableSweep = false;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            LONG64 value1Size = 128;
            LONG64 value2Size = 64;

            KBuffer::SPtr key = CreateBuffer(8);

            KBuffer::SPtr value1 = CreateBuffer(value1Size);
            KBuffer::SPtr value2 = CreateBuffer(value2Size);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value1, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();

            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value2, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();

            LONG64 totalKeySize = GetSerializedSize(*key);
            LONG64 expectedEstimate = totalKeySize / 1;
            CODING_ERROR_ASSERT(Store->GetEstimatedKeySize() == expectedEstimate);

            // Consolidated: 1 key with 1 value
            LONG64 expectedSize = GetSerializedSize(*value2) + GetKeysAndMetadataSize(expectedEstimate, 0, 1);
            LONG64 actualSize = Store->Size;
            CODING_ERROR_ASSERT(actualSize == expectedSize);
            co_return;
        }

        ktl::Awaitable<void> Consolidated_Recovery_SingleItem_ShouldSucceed_Test()
        {
            Store->EnableSweep = false;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            LONG64 keySize = sizeof(LONG64);
            UNREFERENCED_PARAMETER(keySize);

            KBuffer::SPtr key = CreateBuffer(8);
            KBuffer::SPtr value = CreateBuffer(128);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            LONG64 totalKeySize = GetSerializedSize(*key);
            LONG64 expectedEstimate = totalKeySize / 1;
            CODING_ERROR_ASSERT(Store->GetEstimatedKeySize() == expectedEstimate);

            co_await CheckpointAsync();

            co_await CloseAndReOpenStoreAsync();
            CODING_ERROR_ASSERT(Store->GetEstimatedKeySize() == expectedEstimate);

            // Consolidated: 1 key recovered, value not in memory
            LONG64 expectedSize = GetKeysAndMetadataSize(expectedEstimate, 0, 1);
            LONG64 actualSize = Store->Size;
            CODING_ERROR_ASSERT(actualSize == expectedSize);

            // Load value and validate again.
            co_await VerifyKeyExistsAsync(*Store, key, nullptr, value, SingleElementBufferEquals);

            // Consolidated: 1 key with 1 value
            expectedSize = GetSerializedSize(*value) + GetKeysAndMetadataSize(expectedEstimate, 0, 1);
            actualSize = Store->Size;
            CODING_ERROR_ASSERT(actualSize == expectedSize);
            co_return;
        }

        ktl::Awaitable<void> ConsolidatedDifferential_KeyExistsInBoth_ShouldSucceed_Test()
        {
            Store->EnableSweep = false;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            LONG64 value1Size = 128;
            LONG64 value2Size = 64;

            KBuffer::SPtr key = CreateBuffer(8);

            KBuffer::SPtr value1 = CreateBuffer(value1Size);
            KBuffer::SPtr value2 = CreateBuffer(value2Size);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value1, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();

            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value2, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            LONG64 totalKeySize = GetSerializedSize(*key);
            LONG64 expectedEstimate = totalKeySize / 1;
            CODING_ERROR_ASSERT(Store->GetEstimatedKeySize() == expectedEstimate);

            // Consolidated: 1 key with value, Differential: 1 key with value -> 2 keys, 2 values
            LONG64 expectedSize = GetSerializedSize(*value1) + GetSerializedSize(*value2) + GetKeysAndMetadataSize(expectedEstimate, 1, 1);
            LONG64 actualSize = Store->Size;
            CODING_ERROR_ASSERT(actualSize == expectedSize);
            co_return;
        }

        ktl::Awaitable<void> ConsolidatedDifferential_KeyExistsInBoth_WithPreviousVersion_ShouldSucceed_Test()
        {
            Store->EnableSweep = false;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            LONG64 value1Size = 128;
            LONG64 value2Size = 64;
            LONG64 value3Size = 256;

            KBuffer::SPtr key = CreateBuffer(8);

            KBuffer::SPtr value1 = CreateBuffer(value1Size);
            KBuffer::SPtr value2 = CreateBuffer(value2Size);
            KBuffer::SPtr value3 = CreateBuffer(value3Size);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value1, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();

            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value2, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value3, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            LONG64 totalKeySize = GetSerializedSize(*key);
            LONG64 expectedEstimate = totalKeySize / 1;
            CODING_ERROR_ASSERT(Store->GetEstimatedKeySize() == expectedEstimate);

            // Consolidated: 1 key with value, Differential: 1 key with current and previous -> 2 keys and 3 values total
            LONG64 expectedSize = GetKeysAndMetadataSize(expectedEstimate, 1, 1) + GetSerializedSize(*value1) + GetSerializedSize(*value2) + GetSerializedSize(*value3);
            LONG64 actualSize = Store->Size;
            CODING_ERROR_ASSERT(actualSize == expectedSize);
            co_return;
        }

        ktl::Awaitable<void> ConsolidatedDifferential_AfterRecovery_KeyExistsInBoth_WithPreviousVersion_ShouldSucceed_Test()
        {
            Store->EnableSweep = false;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            LONG64 value1Size = 128;
            LONG64 value2Size = 64;
            LONG64 value3Size = 256;
            LONG64 value4Size = 100;

            KBuffer::SPtr key = CreateBuffer(8);

            KBuffer::SPtr value1 = CreateBuffer(value1Size);  // In consolidated from recovery
            KBuffer::SPtr value2 = CreateBuffer(value2Size);  // In consolidated from differential
            KBuffer::SPtr value3 = CreateBuffer(value3Size);  // Differential previous version
            KBuffer::SPtr value4 = CreateBuffer(value4Size);  // Differential current version

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value1, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();
            co_await CloseAndReOpenStoreAsync();
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;
            Store->EnableSweep = false;

            LONG64 totalKeySize = GetSerializedSize(*key);
            LONG64 expectedEstimate = totalKeySize / 1;
            CODING_ERROR_ASSERT(Store->GetEstimatedKeySize() == expectedEstimate);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value2, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();

            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value3, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value4, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            CODING_ERROR_ASSERT(Store->GetEstimatedKeySize() == expectedEstimate);

            // Consolidated: 1 key with value; Differential: 1 key with current and previous -> 2 keys, 3 values
            LONG64 expectedSize = GetSerializedSize(*value2) + GetSerializedSize(*value3) + GetSerializedSize(*value4) + GetKeysAndMetadataSize(expectedEstimate, 1, 1);
            LONG64 actualSize = Store->Size;
            CODING_ERROR_ASSERT(actualSize == expectedSize);
            co_return;
        }

        ktl::Awaitable<void> Snapshot_FromDifferential_ShouldSucceed_Test()
        {
            Store->EnableSweep = false;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            KBuffer::SPtr key = CreateBuffer(5);
            KBuffer::SPtr value1 = CreateBuffer(96);
            KBuffer::SPtr value2 = CreateBuffer(48);
            KBuffer::SPtr value3 = CreateBuffer(120);

            // Add
            {
                auto tx1 = CreateWriteTransaction();
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key, value1, DefaultTimeout, CancellationToken::None);
                co_await tx1->CommitAsync();
            }

            // Start the snapshot transaction here

            auto tx = CreateWriteTransaction();
            tx->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
            co_await VerifyKeyExistsAsync(*Store, *tx->StoreTransactionSPtr, key, nullptr, value1);

            // Update causes entries to previous version.
            {
                auto tx2 = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*tx2->StoreTransactionSPtr, key, value2, DefaultTimeout, CancellationToken::None);
                co_await tx2->CommitAsync();
            }

            // Update again to move entries to snapshot container
            {
                auto tx2 = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*tx2->StoreTransactionSPtr, key, value3, DefaultTimeout, CancellationToken::None);
                co_await tx2->CommitAsync();
            }

            LONG64 totalKeySize = GetSerializedSize(*key);
            LONG64 expectedEstimate = totalKeySize / 1;
            CODING_ERROR_ASSERT(Store->GetEstimatedKeySize() == expectedEstimate);

            // Snapshot: 1 key with value; Differential: 1 key with current and previous -> 2 keys, 3 values
            LONG64 expectedSize = GetSerializedSize(*value1) + GetSerializedSize(*value2) + GetSerializedSize(*value3) + GetKeysAndMetadataSize(expectedEstimate, 1, 1);
            LONG64 actualSize = Store->Size;
            CODING_ERROR_ASSERT(actualSize == expectedSize);

            co_await tx->AbortAsync();
            co_return;
        }

        ktl::Awaitable<void> Snapshot_FromConsolidated_ShouldSucceed_Test()
        {
            Store->EnableSweep = false;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            byte count = 4;
            KBuffer::SPtr value1 = CreateBuffer(96);
            KBuffer::SPtr value2 = CreateBuffer(60);

            for (byte i = 0; i < count; i++)
            {
                KBuffer::SPtr key = CreateBuffer(8, i);
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value1, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            auto snapshotTxn = CreateWriteTransaction();
            snapshotTxn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
            for (byte i = 0; i < count; i++)
            {
                KBuffer::SPtr key = CreateBuffer(8, i);
                co_await VerifyKeyExistsAsync(*Store, *snapshotTxn->StoreTransactionSPtr, key, nullptr, value1);
            }

            co_await CheckpointAsync();

            for (byte i = 0; i < count; i++)
            {
                KBuffer::SPtr key = CreateBuffer(8, i);

                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value2, DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }
            }
        
            co_await CheckpointAsync();

            LONG64 totalKeySize = count * GetSerializedSize(*CreateBuffer(8));
            LONG64 expectedEstimate = totalKeySize / count;
            CODING_ERROR_ASSERT(Store->GetEstimatedKeySize() == expectedEstimate);

            LONG64 expectedSize = count * GetSerializedSize(*value1) + count * GetSerializedSize(*value2) + GetKeysAndMetadataSize(expectedEstimate, 0, count + count);
            LONG64 actualSize = Store->Size;
            CODING_ERROR_ASSERT(actualSize == expectedSize);

            co_await snapshotTxn->AbortAsync();
            co_return;
        }

        ktl::Awaitable<void> Snapshot_FromConsolidated_ReadModesTest_Test()
        {
            Store->EnableSweep = false;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;
            ULONG32 count = 4;
            ULONG32 keySize = 8;
            KBuffer::SPtr value1 = CreateBuffer(96);
            KBuffer::SPtr value2 = CreateBuffer(60);

            for (byte i = 0; i < count; i++)
            {
                KBuffer::SPtr key = CreateBuffer(keySize, i);
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value1, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            auto snapshotTxn = CreateWriteTransaction();
            snapshotTxn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
            for (byte i = 0; i < count; i++)
            {
                KBuffer::SPtr key = CreateBuffer(keySize, i);
                co_await VerifyKeyExistsAsync(*Store, *snapshotTxn->StoreTransactionSPtr, key, nullptr, value1, KBufferComparer::Equals);
            }

            co_await CheckpointAsync();
        
            Store->ConsolidationManagerSPtr->SweepConsolidatedState(CancellationToken::None);
            Store->ConsolidationManagerSPtr->SweepConsolidatedState(CancellationToken::None);
        
            for (byte i = 0; i < count; i++)
            {
                KBuffer::SPtr key = CreateBuffer(keySize, i);
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value2, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync(); // Trigger move to snapshot component

            CODING_ERROR_ASSERT(Store->GetEstimatedKeySize() == GetSerializedSize(*CreateBuffer(keySize, 1)));

            // count in consolidated and count in snapshot
            LONG64 expectedKeyAndMetadataSize = GetKeysAndMetadataSize(Store->GetEstimatedKeySize(), 0, 2 * count);
            LONG64 expectedSize = expectedKeyAndMetadataSize + count * GetSerializedSize(*value2);
            LONG64 actualSize = Store->Size;
            CODING_ERROR_ASSERT(actualSize == expectedSize);

            // Read from snapshot component with ReadMode::ReadValue - should not increase size
            for (byte i = 0; i < count; i++)
            {
                KBuffer::SPtr key = CreateBuffer(keySize, i);
                KeyValuePair<LONG64, KBuffer::SPtr> kvPair(-1, nullptr);
                co_await Store->TryGetValueAsync(*snapshotTxn->StoreTransactionSPtr, key, DefaultTimeout, kvPair, ReadMode::ReadValue, CancellationToken::None);
            }

            expectedSize = expectedKeyAndMetadataSize + count * GetSerializedSize(*value2);
            actualSize = Store->Size;
            CODING_ERROR_ASSERT(actualSize == expectedSize);

            // Read from snapshot component with ReadMode::CacheResult - should increase size
            for (byte i = 0; i < count; i++)
            {
                KBuffer::SPtr key = CreateBuffer(8, i);
                KeyValuePair<LONG64, KBuffer::SPtr> kvPair(-1, nullptr);
                co_await Store->TryGetValueAsync(*snapshotTxn->StoreTransactionSPtr, key, DefaultTimeout, kvPair, ReadMode::CacheResult, CancellationToken::None);
            }

            expectedSize = expectedKeyAndMetadataSize + count * GetSerializedSize(*value1) + count * GetSerializedSize(*value2);
            actualSize = Store->Size;
            CODING_ERROR_ASSERT(actualSize == expectedSize);

            co_await snapshotTxn->AbortAsync();
            co_return;
        }

        ktl::Awaitable<void> Snapshot_MovedFromDifferential_DuringConsolidation_ShouldSucceed_Test()
        {
            Store->EnableSweep = false;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            byte count = 4;
            KBuffer::SPtr value1 = CreateBuffer(96);
            KBuffer::SPtr value2 = CreateBuffer(60);
            KBuffer::SPtr value3 = CreateBuffer(124);

            for (byte i = 0; i < count; i++)
            {
                KBuffer::SPtr key = CreateBuffer(8, i);

                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value1, DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }
            }

            auto snapshotTxn = CreateWriteTransaction();
            snapshotTxn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
            for (byte i = 0; i < count; i++)
            {
                KBuffer::SPtr key = CreateBuffer(8, i);

                co_await VerifyKeyExistsAsync(*Store, *snapshotTxn->StoreTransactionSPtr, key, nullptr, value1);
            }

            for (byte i = 0; i < count; i++)
            {
                KBuffer::SPtr key = CreateBuffer(8, i);

                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value2, DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }
            }

            co_await CheckpointAsync();

            for (byte i = 0; i < count; i++)
            {
                KBuffer::SPtr key = CreateBuffer(8, i);

                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value3, DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }
            }

            LONG64 totalKeySize = count * GetSerializedSize(*CreateBuffer(8));
            LONG64 expectedEstimate = totalKeySize / count;
            CODING_ERROR_ASSERT(Store->GetEstimatedKeySize() == expectedEstimate);

            LONG64 expectedSize = count * GetSerializedSize(*value1) + count * GetSerializedSize(*value2) + count * GetSerializedSize(*value3) + GetKeysAndMetadataSize(expectedEstimate, count, count + count);
            LONG64 actualSize = Store->Size;
            CODING_ERROR_ASSERT(actualSize == expectedSize);

            co_await snapshotTxn->AbortAsync();
            co_return;
        }

        ktl::Awaitable<void> VariableKeySizes_MultipleInDifferential_ShouldSucceed_Test()
        {
            co_await VariableKeySizes_MultipleInDifferential_TestAsync();
            co_return;
        }

        ktl::Awaitable<void> VariableKeySizes_InDifferentialAndConsolidated_ShouldSucceed_Test()
        {
            co_await VariableKeySizes_InDifferentialAndConsolidated_TestAsync();
            co_return;
        }

        ktl::Awaitable<void> VariableKeySizes_KeyRemoved_AffectsAverage_Test()
        {
            co_await VariableKeySizes_KeyRemoved_AffectsAverage_TestAsync();
            co_return;
        }

        ktl::Awaitable<void> VariableKeySizes_DeletedKeysInCheckpoint_AfterRecovery_AffectsAverage_Test()
        {
            co_await VariableKeySizes_DeletedKeysInCheckpoint_AffectsAverage_AfterRecovery_TestAsync();
            co_return;
        }
    #pragma endregion
    };
    
    BOOST_FIXTURE_TEST_SUITE(StoreMemorySizeTestSuite, StoreMemorySizeTest)

    //// TODO: Add test case with sweep enabled
    //// TODO: Add test case with multiple delta differentials
#pragma region Fixed Key Size Tests
    BOOST_AUTO_TEST_CASE(EmptyStore_ZeroSize_ShouldSucceed)
    {
        SyncAwait(EmptyStore_ZeroSize_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Differential_1Item_ShouldSucceed)
    {
        SyncAwait(Differential_1Item_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Differential_AddUniqueItem_ShouldSucceed)
    {
        SyncAwait(Differential_AddUniqueItem_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Differential_UpdateExisting_ShouldSucceed)
    {
        SyncAwait(Differential_UpdateExisting_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Differential_UpdateExisting_SameTransaction_ShouldSucceed)
    {
        SyncAwait(Differential_UpdateExisting_SameTransaction_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Differential_UpdateTwice_ShouldSucceed)
    {
        SyncAwait(Differential_UpdateTwice_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Consolidated_SingleItem_ShouldSucceed)
    {
        SyncAwait(Consolidated_SingleItem_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Consolidated_AddUniqueItem_ShouldSucceed)
    {
        SyncAwait(Consolidated_AddUniqueItem_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Consolidated_UpdateExisting_ShouldSucceed)
    {
        SyncAwait(Consolidated_UpdateExisting_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Consolidated_Recovery_SingleItem_ShouldSucceed)
    {
        SyncAwait(Consolidated_Recovery_SingleItem_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(ConsolidatedDifferential_KeyExistsInBoth_ShouldSucceed)
    {
        SyncAwait(ConsolidatedDifferential_KeyExistsInBoth_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(ConsolidatedDifferential_KeyExistsInBoth_WithPreviousVersion_ShouldSucceed)
    {
        SyncAwait(ConsolidatedDifferential_KeyExistsInBoth_WithPreviousVersion_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(ConsolidatedDifferential_AfterRecovery_KeyExistsInBoth_WithPreviousVersion_ShouldSucceed)
    {
        SyncAwait(ConsolidatedDifferential_AfterRecovery_KeyExistsInBoth_WithPreviousVersion_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Snapshot_FromDifferential_ShouldSucceed)
    {
        SyncAwait(Snapshot_FromDifferential_ShouldSucceed_Test());
    }
    
    BOOST_AUTO_TEST_CASE(Snapshot_FromConsolidated_ShouldSucceed)
    {
        SyncAwait(Snapshot_FromConsolidated_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Snapshot_FromConsolidated_ReadModesTest)
    {
        SyncAwait(Snapshot_FromConsolidated_ReadModesTest_Test());
    }

    BOOST_AUTO_TEST_CASE(Snapshot_MovedFromDifferential_DuringConsolidation_ShouldSucceed)
    {
        SyncAwait(Snapshot_MovedFromDifferential_DuringConsolidation_ShouldSucceed_Test());
    }
#pragma endregion
   
#pragma region Variable Key Size Tests
    BOOST_AUTO_TEST_CASE(VariableKeySizes_MultipleInDifferential_ShouldSucceed)
    {
        SyncAwait(VariableKeySizes_MultipleInDifferential_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(VariableKeySizes_InDifferentialAndConsolidated_ShouldSucceed)
    {
        SyncAwait(VariableKeySizes_InDifferentialAndConsolidated_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(VariableKeySizes_KeyRemoved_AffectsAverage)
    {
        SyncAwait(VariableKeySizes_KeyRemoved_AffectsAverage_Test());
    }

    BOOST_AUTO_TEST_CASE(VariableKeySizes_DeletedKeysInCheckpoint_AfterRecovery_AffectsAverage)
    {
        SyncAwait(VariableKeySizes_DeletedKeysInCheckpoint_AfterRecovery_AffectsAverage_Test());
    }
#pragma endregion
    BOOST_AUTO_TEST_SUITE_END()
}
