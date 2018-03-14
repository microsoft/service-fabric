// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

#define ALLOC_TAG 'ssTP'

namespace TStoreTests
{
    using namespace ktl;

    class StoreSweepTest : public TStoreTestBase<LONG64, KString::SPtr, LongComparer, TestStateSerializer<LONG64>, StringStateSerializer>
    {
    public:
        StoreSweepTest()
        {
            Setup(1);
            Store->EnableSweep = false;  // Sweep will be manually called
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;
        }

        ~StoreSweepTest()
        {
            Cleanup();
        }

        KString::SPtr CreateString(__in LPCWSTR value)
        {
            KString::SPtr result;
            auto status = KString::Create(result, GetAllocator(), value);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            return result;
        }

        KString::SPtr CreateString(__in ULONG seed)
        {
            KString::SPtr value;
            wstring str = wstring(L"test_value") + to_wstring(seed);
            auto status = KString::Create(value, GetAllocator(), str.c_str());
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            return value;
        }

        void TriggerSweep()
        {
            ktl::AwaitableCompletionSource<bool>::SPtr completionSourceSPtr = nullptr;
            NTSTATUS status = ktl::AwaitableCompletionSource<bool>::Create(GetAllocator(), ALLOC_TAG, completionSourceSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            Store->ConsolidationManagerSPtr->Sweep(ktl::CancellationToken::None, *completionSourceSPtr);
        }

        void CheckpointAndSweep()
        {
            Checkpoint();
            ktl::AwaitableCompletionSource<bool>::SPtr cachedCompletionSourceSPtr = Store->SweepTaskSourceSPtr;
            if (cachedCompletionSourceSPtr != nullptr)
            {
                SyncAwait(cachedCompletionSourceSPtr->GetAwaitable());
                Store->SweepTaskSourceSPtr = nullptr; // Reset so that cleanup doesn't re-await
            }
        }

        LONG64 GetSerializedSize(__in KString & value)
        {
           BinaryWriter writer(GetAllocator());
           KString::SPtr valueSPtr = &value;
           Store->ValueConverterSPtr->Write(valueSPtr, writer);
           return writer.GetBuffer(0)->QuerySize();
        }

        static bool EqualityFunction(KString::SPtr & one, KString::SPtr & two)
        {
            if (one == nullptr || two == nullptr)
            {
                return one == two;
            }

            return one->Compare(*two) == 0;
        }

        Common::CommonConfig config; // load the config object as its needed for the tracing to work
    };

    BOOST_FIXTURE_TEST_SUITE(StoreSweepTestSuite, StoreSweepTest)

#pragma region Consolidation Manager Sweep Unit Tests
    BOOST_AUTO_TEST_CASE(Sweep_ItemsInOldConsolidatedState_ShouldBeSwept)
    {
        // Add one item to the store
        LONG64 key = 1;
        KString::SPtr value = CreateString(L"value");

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        // Perform checkpoint twice

        // 1. Sweep does not touch this item since it just got moved from differential
        Checkpoint();
        TriggerSweep();

        // No readers
        VersionedItem<KString::SPtr>::SPtr versionedItem = Store->ConsolidationManagerSPtr->Read(key);
        CODING_ERROR_ASSERT(versionedItem->GetValue() != nullptr);
        CODING_ERROR_ASSERT(versionedItem->GetInUse() == false);
        
        // 2. Sweep again, item should be swept out
        TriggerSweep();

        // No readers, read from consolidation manager again
        versionedItem = Store->ConsolidationManagerSPtr->Read(key);
        CODING_ERROR_ASSERT(versionedItem->GetValue() == nullptr);
        CODING_ERROR_ASSERT(versionedItem->GetInUse() == false);

        LONG64 actualSize = Store->Size;
        CODING_ERROR_ASSERT(actualSize == 0);
    }

    BOOST_AUTO_TEST_CASE(Sweep_ItemsThatJustCameFromDifferential_ShouldNotBeSweptOnConsolidation)
    {
        // Add one item to the store
        LONG64 key = 1;
        KString::SPtr value = CreateString(L"value");

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        // Perform checkpoint twice
        Checkpoint();
        TriggerSweep();

        // No readers
        VersionedItem<KString::SPtr>::SPtr versionedItem = Store->ConsolidationManagerSPtr->Read(key);

        // Should not have been swept
        CODING_ERROR_ASSERT(versionedItem != nullptr);
        CODING_ERROR_ASSERT(versionedItem->GetValue()->Compare(*value) == 0);

        // This should be false because only the top level read api from consolidated state sets the InUse flag to true
        CODING_ERROR_ASSERT(versionedItem->GetInUse() == false);

        LONG64 expectedSize = GetSerializedSize(*value);
        LONG64 actualSize = Store->Size;

        CODING_ERROR_ASSERT(expectedSize == actualSize);
    }

    BOOST_AUTO_TEST_CASE(Sweep_WhenThereIsAReader_ItemShouldNotBeSwept)
    {
        // Add one item in the store
        LONG64 key = 1;
        KString::SPtr value = CreateString(L"value");

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        // Perform checkpoint twice
        Checkpoint();
        TriggerSweep();

        // No readers
        VersionedItem<KString::SPtr>::SPtr versionedItem = Store->ConsolidationManagerSPtr->Read(key);
        CODING_ERROR_ASSERT(versionedItem->GetValue() != nullptr);
        CODING_ERROR_ASSERT(versionedItem->GetInUse() == false);
        CODING_ERROR_ASSERT(versionedItem->GetValue()->Compare(*value) == 0);

        // Read data
        SyncAwait(VerifyKeyExistsAsync(*Store, key, nullptr, value, StoreSweepTest::EqualityFunction));
        CODING_ERROR_ASSERT(versionedItem->GetInUse() == true);

        // Perform checkpoint again
        TriggerSweep();

        // No readers
        versionedItem = Store->ConsolidationManagerSPtr->Read(key);

        CODING_ERROR_ASSERT(versionedItem->GetValue() != nullptr);
        CODING_ERROR_ASSERT(versionedItem->GetInUse() == false);
        CODING_ERROR_ASSERT(versionedItem->GetValue()->Compare(*value) == 0);

        LONG64 expectedSize = GetSerializedSize(*value);
        LONG64 actualSize = Store->Size;

        CODING_ERROR_ASSERT(expectedSize == actualSize);
    }

    BOOST_AUTO_TEST_CASE(Sweep_CheckpointAfterRead_ItemShouldBeSwept)
    {
        LONG64 key = 1;
        KString::SPtr value = CreateString(L"value");

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        Checkpoint();

        // Does not simulate read
        VersionedItem<KString::SPtr>::SPtr versionedItem = Store->ConsolidationManagerSPtr->Read(key);
        CODING_ERROR_ASSERT(versionedItem->GetValue() != nullptr);
        CODING_ERROR_ASSERT(versionedItem->GetInUse() == true);

        // Sweep, the should reset the inuse flag
        TriggerSweep();
        CODING_ERROR_ASSERT(versionedItem->GetValue() != nullptr);
        CODING_ERROR_ASSERT(versionedItem->GetInUse() == false);

        // Read data
        SyncAwait(VerifyKeyExistsAsync(*Store, key, nullptr, value, StoreSweepTest::EqualityFunction));
        CODING_ERROR_ASSERT(versionedItem->GetValue() != nullptr);
        CODING_ERROR_ASSERT(versionedItem->GetInUse() == true);

        // Sweep again and the item should not have been swept
        TriggerSweep();
        CODING_ERROR_ASSERT(versionedItem->GetValue() != nullptr);
        CODING_ERROR_ASSERT(versionedItem->GetInUse() == false);

        // Sweep again
        TriggerSweep();
        CODING_ERROR_ASSERT(versionedItem->GetValue() == nullptr);
        CODING_ERROR_ASSERT(versionedItem->GetInUse() == false);

        LONG64 actualSize = Store->Size;
        CODING_ERROR_ASSERT(actualSize == 0);
    }

    BOOST_AUTO_TEST_CASE(Sweep_WithMerge_ShouldSucceed)
    {
        Store->EnableSweep = true;
        // TODO: FileCount policy doesn't work background consolidation
        Store->MergeHelperSPtr->CurrentMergePolicy = MergePolicy::InvalidEntries;
        Store->MergeHelperSPtr->NumberOfInvalidEntries = 1;

        LONG64 key1 = 1;
        LONG64 key2 = 2;

        KString::SPtr value = CreateString(L"value");

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key1, value, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        Checkpoint();

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key1, value, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key2, value, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        Checkpoint();

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key1, value, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        Checkpoint();
        Checkpoint();

        LONG64 actualSize = Store->Size;
        CODING_ERROR_ASSERT(actualSize == 0);
        
        SyncAwait(VerifyKeyExistsInStoresAsync(key1, nullptr, value, StoreSweepTest::EqualityFunction));
        SyncAwait(VerifyKeyExistsInStoresAsync(key2, nullptr, value, StoreSweepTest::EqualityFunction));

        LONG64 expectedSize = GetSerializedSize(*value);
        actualSize = Store->Size;

        CODING_ERROR_ASSERT(expectedSize * 2 == actualSize);
    }

    //BOOST_AUTO_TEST_CASE(Sweep_WithNullValues_ShouldSucceed, * boost::unit_test_framework::disabled(/* StringStateSerializer doesn't support null values */))
    //{
    //    LONG64 key = 1;

    //    KString::SPtr value1 = nullptr;
    //    KString::SPtr value2 = CreateString(L"value");

    //    {
    //        auto txn = CreateWriteTransaction();
    //        SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value1, DefaultTimeout, ktl::CancellationToken::None));
    //        SyncAwait(txn->CommitAsync());
    //    }
    //    
    //    Checkpoint();

    //    VersionedItem<KString::SPtr>::SPtr versionedItem = Store->ConsolidationManagerSPtr->Read(key);
    //    CODING_ERROR_ASSERT(versionedItem->GetValue() == nullptr);
    //    CODING_ERROR_ASSERT(versionedItem->GetInUse() == true);

    //    TriggerSweep();

    //    CODING_ERROR_ASSERT(versionedItem->GetValue() == nullptr);
    //    CODING_ERROR_ASSERT(versionedItem->GetInUse() == true);

    //    TriggerSweep();

    //    CODING_ERROR_ASSERT(versionedItem->GetValue() == nullptr);
    //    CODING_ERROR_ASSERT(versionedItem->GetInUse() == true);

    //    CloseAndReOpenStore();

    //    Store->EnableSweep = false;
    //    Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

    //    CODING_ERROR_ASSERT(versionedItem->GetValue() == nullptr);
    //    CODING_ERROR_ASSERT(versionedItem->GetInUse() == true);

    //    SyncAwait(VerifyKeyExistsAsync(*Store, key, nullptr, value1, StoreSweepTest::EqualityFunction));

    //    CODING_ERROR_ASSERT(versionedItem->GetValue() == nullptr);
    //    CODING_ERROR_ASSERT(versionedItem->GetInUse() == true);

    //    {
    //        auto txn = CreateWriteTransaction();
    //        SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value2, DefaultTimeout, ktl::CancellationToken::None));
    //        SyncAwait(txn->CommitAsync());
    //    }

    //    versionedItem = Store->DifferentialState->Read(key);
    //    CODING_ERROR_ASSERT(versionedItem->GetValue() != nullptr);
    //    CODING_ERROR_ASSERT(versionedItem->GetInUse() == true);

    //    Checkpoint();

    //    versionedItem = Store->ConsolidationManagerSPtr->Read(key);
    //    CODING_ERROR_ASSERT(versionedItem->GetValue() != nullptr);
    //    CODING_ERROR_ASSERT(versionedItem->GetInUse() == true);

    //    TriggerSweep();

    //    CODING_ERROR_ASSERT(versionedItem->GetValue() != nullptr);
    //    CODING_ERROR_ASSERT(versionedItem->GetInUse() == false);

    //    TriggerSweep();

    //    CODING_ERROR_ASSERT(versionedItem->GetValue() == nullptr);
    //    CODING_ERROR_ASSERT(versionedItem->GetInUse() == false);
    //}

    BOOST_AUTO_TEST_CASE(AddSweepUpdate_ShouldSucceed)
    {
        LONG64 key = 17;
        KString::SPtr value = CreateString(L"value");
        KString::SPtr value2 = CreateString(L"update");

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        Checkpoint();
        TriggerSweep();
        TriggerSweep();
        TriggerSweep();

        {
            auto txn = CreateWriteTransaction();
            bool updated = SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value2, DefaultTimeout, ktl::CancellationToken::None));
            CODING_ERROR_ASSERT(updated);
            SyncAwait(txn->CommitAsync());
        }

        LONG64 expectedSize = GetSerializedSize(*value2);
        LONG64 actualSize = Store->Size;

        CODING_ERROR_ASSERT(expectedSize == actualSize)
    }

    BOOST_AUTO_TEST_CASE(Sweep_VerifyValueOnRead_ShouldSucceed)
    {
        LONG64 key = 17;
        KString::SPtr value = CreateString(L"value");

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        Checkpoint();
        TriggerSweep();
        TriggerSweep();

        VersionedItem<KString::SPtr>::SPtr versionedItem = Store->ConsolidationManagerSPtr->Read(key);
        CODING_ERROR_ASSERT(versionedItem->GetInUse() == false);
        CODING_ERROR_ASSERT(versionedItem->GetValue() == nullptr);

        LONG64 expectedSize = 0;
        LONG64 actualSize = Store->Size;

        CODING_ERROR_ASSERT(expectedSize == actualSize)

        SyncAwait(VerifyKeyExistsAsync(*Store, key, nullptr, value, StoreSweepTest::EqualityFunction));

        expectedSize = GetSerializedSize(*value);
        actualSize = Store->Size;

        CODING_ERROR_ASSERT(expectedSize == actualSize)
    }

    BOOST_AUTO_TEST_CASE(CheckpointRecoverSweepRead_ShouldSucceed)
    {
        LONG64 key = 17;
        KString::SPtr value = CreateString(L"value");

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        Checkpoint();
        CloseAndReOpenStore();
        TriggerSweep();
        TriggerSweep();
        TriggerSweep();

        VersionedItem<KString::SPtr>::SPtr versionedItem = Store->ConsolidationManagerSPtr->Read(key);
        CODING_ERROR_ASSERT(versionedItem->GetInUse() == false);
        CODING_ERROR_ASSERT(versionedItem->GetValue() == nullptr);

        LONG64 expectedSize = 0;
        LONG64 actualSize = Store->Size;

        CODING_ERROR_ASSERT(expectedSize == actualSize)

        SyncAwait(VerifyKeyExistsAsync(*Store, key, nullptr, value, StoreSweepTest::EqualityFunction));

        expectedSize = GetSerializedSize(*value);
        actualSize = Store->Size;

        CODING_ERROR_ASSERT(expectedSize == actualSize)
    }
#pragma endregion

#pragma region Store Sweep tests

    BOOST_AUTO_TEST_CASE(CheckpointWithSweep_ItemsInOldConsolidatedState_ShouldBeSwept)
    {
        Store->EnableSweep = true;
        Store->EnableBackgroundConsolidation = false;
        Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

        LONG64 key = 1;
        KString::SPtr value = CreateString(L"value");

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        
        // Sweep does not touch this item since it just got moved from differential
        CheckpointAndSweep();

        // Sweep again, item should be swept out
        CheckpointAndSweep();

        // Read from consolidated should succeed
        SyncAwait(VerifyKeyExistsAsync(*Store, key, nullptr, value, StoreSweepTest::EqualityFunction));
    }

    BOOST_AUTO_TEST_CASE(CheckpointWithSweep_ItemsThatJustCameFromDifferential_ShouldNotBeSweptOnConsolidation)
    {
        Store->EnableSweep = true;
        Store->EnableBackgroundConsolidation = false;
        Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

        // Add one item to the store
        LONG64 key = 1;
        KString::SPtr value = CreateString(L"value");

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        
        CheckpointAndSweep();

        SyncAwait(VerifyKeyExistsAsync(*Store, key, nullptr, value, StoreSweepTest::EqualityFunction));
    }

    BOOST_AUTO_TEST_CASE(CheckpointWithSweep_WhenThereIsAReader_ItemShouldNotBeSwept)
    {
        Store->EnableSweep = true;
        Store->EnableBackgroundConsolidation = false;
        Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

        // Add one item in the store
        LONG64 key = 1;
        KString::SPtr value = CreateString(L"value");

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        CheckpointAndSweep();

        // Read data
        SyncAwait(VerifyKeyExistsAsync(*Store, key, nullptr, value, StoreSweepTest::EqualityFunction));

        // Perform checkpoint again
        CheckpointAndSweep();

        // No readers
        VersionedItem<KString::SPtr>::SPtr versionedItem = Store->ConsolidationManagerSPtr->Read(key);

        CODING_ERROR_ASSERT(versionedItem->GetValue() != nullptr);
        CODING_ERROR_ASSERT(versionedItem->GetInUse() == false);
        CODING_ERROR_ASSERT(versionedItem->GetValue()->Compare(*value) == 0);

        SyncAwait(VerifyKeyExistsAsync(*Store, key, nullptr, value, StoreSweepTest::EqualityFunction));
    }

    BOOST_AUTO_TEST_CASE(CheckpointWithSweep_CheckpointAfterRead_ItemShouldBeSwept)
    {
        Store->EnableSweep = true;
        Store->EnableBackgroundConsolidation = false;
        Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

        LONG64 key = 1;
        KString::SPtr value = CreateString(L"value");

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        
        // Sweep and read
        CheckpointAndSweep();
        SyncAwait(VerifyKeyExistsAsync(*Store, key, nullptr, value, StoreSweepTest::EqualityFunction));

        // Sweep again and the item should not have been swept
        CheckpointAndSweep();
        SyncAwait(VerifyKeyExistsAsync(*Store, key, nullptr, value, StoreSweepTest::EqualityFunction));

        // Sweep again
        CheckpointAndSweep();
        SyncAwait(VerifyKeyExistsAsync(*Store, key, nullptr, value, StoreSweepTest::EqualityFunction));
    }

    BOOST_AUTO_TEST_CASE(CheckpointWithSweep_AddSweepUpdate_ShouldSucceed)
    {
        Store->EnableSweep = true;
        Store->EnableBackgroundConsolidation = false;
        Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

        LONG64 key = 17;
        KString::SPtr value = CreateString(L"value");
        KString::SPtr value2 = CreateString(L"update");

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        
        CheckpointAndSweep();
        CheckpointAndSweep();
        CheckpointAndSweep();

        {
            auto txn = CreateWriteTransaction();
            bool updated = SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value2, DefaultTimeout, ktl::CancellationToken::None));
            CODING_ERROR_ASSERT(updated);
            SyncAwait(txn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(CheckpointRecoverCheckpointSweepRead_ShouldSucceed)
    {
        Store->EnableSweep = true;
        Store->EnableBackgroundConsolidation = false;
        Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

        LONG64 key = 17;
        KString::SPtr value = CreateString(L"value");

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        CheckpointAndSweep();
        CloseAndReOpenStore();

        Store->EnableSweep = true;
        Store->EnableBackgroundConsolidation = false;
        Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

        CheckpointAndSweep();
        CheckpointAndSweep();
        CheckpointAndSweep();

        SyncAwait(VerifyKeyExistsAsync(*Store, key, nullptr, value, StoreSweepTest::EqualityFunction));
    }

    BOOST_AUTO_TEST_CASE(CheckpointWithSweep_AddUpdateRemoveMany_ShouldSucceed)
    {
        Store->EnableSweep = true;
        Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 3;

        LONG64 key = 17;
        KString::SPtr value = CreateString(L"value");

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }
        
        for (ULONG32 i = 0; i < 81; i++)
        {
            switch (i % 3)
            {
            case 0: 
            {
                auto txn = CreateWriteTransaction();
                SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None));
                SyncAwait(txn->CommitAsync());
            }
            break;
            case 1: 
            {
                auto txn = CreateWriteTransaction();
                SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, ktl::CancellationToken::None));
                SyncAwait(txn->CommitAsync());
            }
            break;
            case 2: 
            {
                auto txn = CreateWriteTransaction();
                SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None));
                SyncAwait(txn->CommitAsync());
            }
            break;
            }
            Checkpoint();
        }
        
        Checkpoint();
        CloseAndReOpenStore();

        Store->EnableSweep = true;
        Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 3;

        VerifyCountInAllStores(1);
        SyncAwait(VerifyKeyExistsInStoresAsync(key, nullptr, value, StoreSweepTest::EqualityFunction));
    }
    
    BOOST_AUTO_TEST_CASE(Sweep_ManyItems_ShouldSucceed)
    {
        Store->EnableSweep = true;
        Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 3;

        for (ULONG32 startKey = 0; startKey < 100; startKey += 10)
        {
            for (ULONG32 key = startKey; key < startKey + 20; key++)
            {
                {
                    auto txn = CreateWriteTransaction();
                    
                    KeyValuePair<LONG64, KString::SPtr> output;
                    bool found = SyncAwait(Store->ConditionalGetAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, output, ktl::CancellationToken::None));
                    if (!found)
                    {
                        SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, CreateString(key), DefaultTimeout, ktl::CancellationToken::None));
                    }
                    else
                    {
                        SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, CreateString(key), DefaultTimeout, ktl::CancellationToken::None));
                    }

                    SyncAwait(txn->CommitAsync());
                }
            }
            Checkpoint();
        }

        for (ULONG32 key = 0; key < 110; key++)
        {
            SyncAwait(VerifyKeyExistsInStoresAsync(key, nullptr, CreateString(key), StoreSweepTest::EqualityFunction));
        }

        CloseAndReOpenStore();
        Store->EnableSweep = true;
        Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 3;

        for (ULONG32 key = 0; key < 110; key++)
        {
            SyncAwait(VerifyKeyExistsInStoresAsync(key, nullptr, CreateString(key), StoreSweepTest::EqualityFunction));
        }
    }

#pragma endregion

    BOOST_AUTO_TEST_CASE(CompleteCheckpoint_WithConcurrentReads_ShouldSucceed)
    {
        Store->EnableSweep = true;
        Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

        for (LONG64 i = 0; i < 1000; i++)
        {
            auto key = i;
            auto value = CreateString(static_cast<ULONG>(i));
            {
                auto txn = CreateWriteTransaction();
                SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None));
                SyncAwait(txn->CommitAsync());
            }
        }

        // Checkpoint multiple times to sweep items
        Checkpoint();
        Checkpoint();
        Checkpoint();

        LONG64 checkpointLSN = Replicator->IncrementAndGetCommitSequenceNumber();
        Store->PrepareCheckpoint(checkpointLSN);
        SyncAwait(Store->PerformCheckpointAsync(ktl::CancellationToken::None));

        KSharedArray<ktl::Awaitable<bool>>::SPtr getTasks = _new(ALLOC_TAG, GetAllocator()) KSharedArray<ktl::Awaitable<bool>>();

        {
            auto txn = CreateWriteTransaction();
            for (LONG64 i = 0; i < 1000; i++)
            {
                KeyValuePair<LONG64, KString::SPtr> result(-1, nullptr);
                auto task = Store->ConditionalGetAsync(*txn->StoreTransactionSPtr, i, DefaultTimeout, result, ktl::CancellationToken::None);
                getTasks->Append(Ktl::Move(task));
            }

            auto whenAllTask = StoreUtilities::WhenAll<bool>(*getTasks, GetAllocator());

            SyncAwait(Store->CompleteCheckpointAsync(ktl::CancellationToken::None));
            SyncAwait(whenAllTask);

            SyncAwait(txn->AbortAsync());
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
