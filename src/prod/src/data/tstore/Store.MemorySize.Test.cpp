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
    class StoreMemorySizeTest : public TStoreTestBase<LONG64, KBuffer::SPtr, LongComparer, TestStateSerializer<LONG64>, KBufferSerializer>
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

        Common::CommonConfig config; // load the config object as its needed for the tracing to work
    };
    
    BOOST_FIXTURE_TEST_SUITE(StoreMemorySizeTestSuite, StoreMemorySizeTest)

    // TODO: Add test case with sweep enabled
    // TODO: Add test case with multiple delta differentials

    BOOST_AUTO_TEST_CASE(EmptyStore_ZeroSize_ShouldSucceed)
    {
        LONG64 size = Store->Size;
        CODING_ERROR_ASSERT(size == 0);
    }

    BOOST_AUTO_TEST_CASE(Differential_1Item_ShouldSucceed)
    {
        Store->EnableSweep = false;
        Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

        LONG64 keySize = sizeof(LONG64);
        UNREFERENCED_PARAMETER(keySize);

        LONG64 key = 8;
        KBuffer::SPtr value = CreateBuffer(128);

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        LONG64 expectedSize = GetSerializedSize(*value); //keySize + valueSize;
        LONG64 actualSize = Store->Size;
        CODING_ERROR_ASSERT(actualSize == expectedSize);
    }

    BOOST_AUTO_TEST_CASE(Differential_AddUniqueItem_ShouldSucceed)
    {
        Store->EnableSweep = false;
        Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

        LONG64 keySize = sizeof(LONG64);
        UNREFERENCED_PARAMETER(keySize);

        LONG64 value1Size = 128;
        LONG64 value2Size = 64;

        LONG64 key1 = 8;
        LONG64 key2 = 10;

        KBuffer::SPtr value1 = CreateBuffer(value1Size);
        KBuffer::SPtr value2 = CreateBuffer(value2Size);

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key1, value1, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key2, value2, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        LONG64 expectedSize = GetSerializedSize(*value1) + GetSerializedSize(*value2);
        LONG64 actualSize = Store->Size;
        CODING_ERROR_ASSERT(actualSize == expectedSize);
    }

    BOOST_AUTO_TEST_CASE(Differential_UpdateExisting_ShouldSucceed)
    {
        Store->EnableSweep = false;
        Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

        LONG64 keySize = sizeof(LONG64);
        UNREFERENCED_PARAMETER(keySize);

        LONG64 value1Size = 128;
        LONG64 value2Size = 64;

        LONG64 key = 8;

        KBuffer::SPtr value1 = CreateBuffer(value1Size);
        KBuffer::SPtr value2 = CreateBuffer(value2Size);

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value1, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value2, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        
        // Differential state keeps track of previous version and current version
        LONG64 expectedSize = GetSerializedSize(*value1) + GetSerializedSize(*value2);
        LONG64 actualSize = Store->Size;
        CODING_ERROR_ASSERT(actualSize == expectedSize);
    }

    BOOST_AUTO_TEST_CASE(Differential_UpdateExisting_SameTransaction_ShouldSucceed)
    {
        Store->EnableSweep = false;
        Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

        LONG64 keySize = sizeof(LONG64);
        UNREFERENCED_PARAMETER(keySize);

        LONG64 value1Size = 128;
        LONG64 value2Size = 64;

        LONG64 key = 8;

        KBuffer::SPtr value1 = CreateBuffer(value1Size);
        KBuffer::SPtr value2 = CreateBuffer(value2Size);

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value1, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value2, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        LONG64 expectedSize = GetSerializedSize(*value2);
        LONG64 actualSize = Store->Size;
        CODING_ERROR_ASSERT(actualSize == expectedSize);
    }

    BOOST_AUTO_TEST_CASE(Differential_UpdateTwice_ShouldSucceed)
    {
        Store->EnableSweep = false;
        Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

        LONG64 keySize = sizeof(LONG64);
        UNREFERENCED_PARAMETER(keySize);

        LONG64 value1Size = 128;
        LONG64 value2Size = 64;
        LONG64 value3Size = 256;

        LONG64 key = 8;

        KBuffer::SPtr value1 = CreateBuffer(value1Size);
        KBuffer::SPtr value2 = CreateBuffer(value2Size);
        KBuffer::SPtr value3 = CreateBuffer(value3Size);

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value1, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value2, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value3, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        
        // Differential state keeps track of previous version and current version
        LONG64 expectedSize = GetSerializedSize(*value2) + GetSerializedSize(*value3);
        LONG64 actualSize = Store->Size;
        CODING_ERROR_ASSERT(actualSize == expectedSize);
    }

    BOOST_AUTO_TEST_CASE(Consolidated_SingleItem_ShouldSucceed)
    {
        Store->EnableSweep = false;
        Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

        LONG64 keySize = sizeof(LONG64);
        UNREFERENCED_PARAMETER(keySize);

        LONG64 key = 8;
        KBuffer::SPtr value = CreateBuffer(128);

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        Checkpoint();

        LONG64 expectedSize = GetSerializedSize(*value); //keySize + valueSize;
        LONG64 actualSize = Store->Size;
        CODING_ERROR_ASSERT(actualSize == expectedSize);
    }

    BOOST_AUTO_TEST_CASE(Consolidated_AddUniqueItem_ShouldSucceed)
    {
        Store->EnableSweep = false;
        Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

        LONG64 keySize = sizeof(LONG64);
        UNREFERENCED_PARAMETER(keySize);

        LONG64 value1Size = 128;
        LONG64 value2Size = 64;

        LONG64 key1 = 8;
        LONG64 key2 = 10;

        KBuffer::SPtr value1 = CreateBuffer(value1Size);
        KBuffer::SPtr value2 = CreateBuffer(value2Size);

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key1, value1, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        Checkpoint();

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key2, value2, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        Checkpoint();

        LONG64 expectedSize = GetSerializedSize(*value1) + GetSerializedSize(*value2);
        LONG64 actualSize = Store->Size;
        CODING_ERROR_ASSERT(actualSize == expectedSize);
    }

    BOOST_AUTO_TEST_CASE(Consolidated_UpdateExisting_ShouldSucceed)
    {
        Store->EnableSweep = false;
        Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

        LONG64 keySize = sizeof(LONG64);
        UNREFERENCED_PARAMETER(keySize);

        LONG64 value1Size = 128;
        LONG64 value2Size = 64;

        LONG64 key = 8;

        KBuffer::SPtr value1 = CreateBuffer(value1Size);
        KBuffer::SPtr value2 = CreateBuffer(value2Size);

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value1, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        Checkpoint();

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value2, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        Checkpoint();

        LONG64 expectedSize = GetSerializedSize(*value2);
        LONG64 actualSize = Store->Size;
        CODING_ERROR_ASSERT(actualSize == expectedSize);
    }

    BOOST_AUTO_TEST_CASE(Consolidated_Recovery_SingleItem_ShouldSucceed)
    {
        Store->EnableSweep = false;
        Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

        LONG64 keySize = sizeof(LONG64);
        UNREFERENCED_PARAMETER(keySize);

        LONG64 key = 8;
        KBuffer::SPtr value = CreateBuffer(128);

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        Checkpoint();
        CloseAndReOpenStore();

        LONG64 actualSize = Store->Size;
        CODING_ERROR_ASSERT(actualSize == 0);

        // Load value and validate again.
        SyncAwait(VerifyKeyExistsAsync(*Store, key, nullptr, value, SingleElementBufferEquals));

        LONG64 expectedSize = GetSerializedSize(*value); //keySize + valueSize;
        actualSize = Store->Size;
        CODING_ERROR_ASSERT(actualSize == expectedSize);
    }

    BOOST_AUTO_TEST_CASE(ConsolidatedDifferential_KeyExistsInBoth_ShouldSucceed)
    {
        Store->EnableSweep = false;
        Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

        LONG64 keySize = sizeof(LONG64);
        UNREFERENCED_PARAMETER(keySize);

        LONG64 value1Size = 128;
        LONG64 value2Size = 64;

        LONG64 key = 8;

        KBuffer::SPtr value1 = CreateBuffer(value1Size);
        KBuffer::SPtr value2 = CreateBuffer(value2Size);

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value1, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        Checkpoint();

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value2, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        LONG64 expectedSize = GetSerializedSize(*value1) + GetSerializedSize(*value2);
        LONG64 actualSize = Store->Size;
        CODING_ERROR_ASSERT(actualSize == expectedSize);
    }

    BOOST_AUTO_TEST_CASE(ConsolidatedDifferential_KeyExistsInBoth_WithPreviousVersion_ShouldSucceed)
    {
        Store->EnableSweep = false;
        Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

        LONG64 value1Size = 128;
        LONG64 value2Size = 64;
        LONG64 value3Size = 256;

        LONG64 key = 8;

        KBuffer::SPtr value1 = CreateBuffer(value1Size);
        KBuffer::SPtr value2 = CreateBuffer(value2Size);
        KBuffer::SPtr value3 = CreateBuffer(value3Size);

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value1, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        Checkpoint();

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value2, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value3, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        LONG64 expectedSize = GetSerializedSize(*value1) + GetSerializedSize(*value2) + GetSerializedSize(*value3);
        LONG64 actualSize = Store->Size;
        CODING_ERROR_ASSERT(actualSize == expectedSize);
    }

    BOOST_AUTO_TEST_CASE(ConsolidatedDifferential_AfterRecovery_KeyExistsInBoth_WithPreviousVersion_ShouldSucceed)
    {
        Store->EnableSweep = false;
        Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

        LONG64 value1Size = 128;
        LONG64 value2Size = 64;
        LONG64 value3Size = 256;
        LONG64 value4Size = 100;

        LONG64 key = 8;

        KBuffer::SPtr value1 = CreateBuffer(value1Size);  // In consolidated from recovery
        KBuffer::SPtr value2 = CreateBuffer(value2Size);  // In consolidated from differential
        KBuffer::SPtr value3 = CreateBuffer(value3Size);  // Differential previous version
        KBuffer::SPtr value4 = CreateBuffer(value4Size);  // Differential current version

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value1, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        Checkpoint();
        CloseAndReOpenStore();
        Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;
        Store->EnableSweep = false;

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value2, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        Checkpoint();

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value3, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value4, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        LONG64 expectedSize = /* GetSerializedSize(*value1) + */ GetSerializedSize(*value2) + GetSerializedSize(*value3) + GetSerializedSize(*value4);
        LONG64 actualSize = Store->Size;
        CODING_ERROR_ASSERT(actualSize == expectedSize);
    }

    BOOST_AUTO_TEST_CASE(Snapshot_FromDifferential_ShouldSucceed)
    {
        Store->EnableSweep = false;
        Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

        LONG64 key = 5;
        KBuffer::SPtr value1 = CreateBuffer(96);
        KBuffer::SPtr value2 = CreateBuffer(48);
        KBuffer::SPtr value3 = CreateBuffer(120);

        // Add
        {
            auto tx1 = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*tx1->StoreTransactionSPtr, key, value1, DefaultTimeout, CancellationToken::None));
            SyncAwait(tx1->CommitAsync());
        }

        // Start the snapshot transaction here

        auto tx = CreateWriteTransaction();
        tx->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
        SyncAwait(VerifyKeyExistsAsync(*Store, *tx->StoreTransactionSPtr, key, nullptr, value1));

        // Update causes entries to previous version.
        {
            auto tx2 = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*tx2->StoreTransactionSPtr, key, value2, DefaultTimeout, CancellationToken::None));
            SyncAwait(tx2->CommitAsync());
        }

        // Update again to move entries to snapshot container
        {
            auto tx2 = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*tx2->StoreTransactionSPtr, key, value3, DefaultTimeout, CancellationToken::None));
            SyncAwait(tx2->CommitAsync());
        }

        LONG64 expectedSize = GetSerializedSize(*value1) + GetSerializedSize(*value2) + GetSerializedSize(*value3);
        LONG64 actualSize = Store->Size;
        CODING_ERROR_ASSERT(actualSize == expectedSize);

        SyncAwait(tx->AbortAsync());
    }
    
    BOOST_AUTO_TEST_CASE(Snapshot_FromConsolidated_ShouldSucceed)
    {
        Store->EnableSweep = false;
        Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

        ULONG32 count = 4;
        KBuffer::SPtr value1 = CreateBuffer(96);
        KBuffer::SPtr value2 = CreateBuffer(60);

        for (ULONG32 i = 0; i < count; i++)
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, i, value1, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        auto snapshotTxn = CreateWriteTransaction();
        snapshotTxn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
        for (ULONG32 i = 0; i < count; i++)
        {
            SyncAwait(VerifyKeyExistsAsync(*Store, *snapshotTxn->StoreTransactionSPtr, i, nullptr, value1));
        }

        Checkpoint();

        for (ULONG32 i = 0; i < count; i++)
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, i, value2, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        LONG64 expectedSize = count * GetSerializedSize(*value1) + count * GetSerializedSize(*value2);
        LONG64 actualSize = Store->Size;
        CODING_ERROR_ASSERT(actualSize == expectedSize);

        SyncAwait(snapshotTxn->AbortAsync());
    }

    BOOST_AUTO_TEST_CASE(Snapshot_MovedFromDifferential_DuringConsolidation_ShouldSucceed)
    {
        Store->EnableSweep = false;
        Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

        ULONG32 count = 4;
        KBuffer::SPtr value1 = CreateBuffer(96);
        KBuffer::SPtr value2 = CreateBuffer(60);
        KBuffer::SPtr value3 = CreateBuffer(124);

        for (ULONG32 i = 0; i < count; i++)
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, i, value1, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        auto snapshotTxn = CreateWriteTransaction();
        snapshotTxn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
        for (ULONG32 i = 0; i < count; i++)
        {
            SyncAwait(VerifyKeyExistsAsync(*Store, *snapshotTxn->StoreTransactionSPtr, i, nullptr, value1));
        }

        for (ULONG32 i = 0; i < count; i++)
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, i, value2, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        Checkpoint();

        for (ULONG32 i = 0; i < count; i++)
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, i, value3, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        LONG64 expectedSize = count * GetSerializedSize(*value1) + count * GetSerializedSize(*value2) + count * GetSerializedSize(*value3);
        LONG64 actualSize = Store->Size;
        CODING_ERROR_ASSERT(actualSize == expectedSize);

        SyncAwait(snapshotTxn->AbortAsync());
    }
    
    BOOST_AUTO_TEST_SUITE_END()
}
