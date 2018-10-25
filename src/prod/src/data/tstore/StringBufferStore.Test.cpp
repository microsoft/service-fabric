// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

#define ALLOC_TAG 'sbTP'

namespace TStoreTests
{
    using namespace ktl;

    class StringBufferStoreTest : public TStoreTestBase<KString::SPtr, KBuffer::SPtr, KStringComparer, StringStateSerializer, KBufferSerializer>
    {
    public:
        StringBufferStoreTest()
        {
            Setup(3);
        }

        ~StringBufferStoreTest()
        {
            Cleanup();
        }

        KString::SPtr ToString(__in ULONG seed)
        {
            KString::SPtr value;
            wstring str = wstring(L"test_value") + to_wstring(seed);
            auto status = KString::Create(value, GetAllocator(), str.c_str());
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            return value;
        }

        KBuffer::SPtr ToBuffer(__in ULONG num)
        {
            KBuffer::SPtr bufferSptr;
            auto status = KBuffer::Create(sizeof(ULONG), bufferSptr, GetAllocator());
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            auto buffer = (ULONG *)bufferSptr->GetBuffer();
            buffer[0] = num;
            return bufferSptr;
        }

        KBuffer::SPtr MakeBuffer(ULONG numElements, ULONG multiplier = 1)
        {
            KBuffer::SPtr bufferSptr;
            auto status = KBuffer::Create(numElements * sizeof(ULONG), bufferSptr, GetAllocator());
            Diagnostics::Validate(status);

            auto buffer = (ULONG *)bufferSptr->GetBuffer();
            for (ULONG i = 0; i < numElements; i++)
            {
                buffer[i] = multiplier * (i + 1); // so that buffer[0] = multiplier
            }

            return bufferSptr;
        }

        static bool EqualityFunction(KBuffer::SPtr & one, KBuffer::SPtr & two)
        {
            return *one == *two;
        }

        Common::CommonConfig config; // load the config object as its needed for the tracing to work

#pragma region test functions
    public:
        ktl::Awaitable<void> Add_SingleTransaction_ShouldSucceed_Test()
        {
            KString::SPtr key;
            auto status = KString::Create(key, GetAllocator(), L"test_key");
            Diagnostics::Validate(status);
        
            KBuffer::SPtr value = MakeBuffer(100);

            {
                auto storeTxn = CreateWriteTransaction();
                co_await Store->AddAsync(*storeTxn->StoreTransactionSPtr, key, value, Common::TimeSpan::MaxValue, CancellationToken::None);
                co_await VerifyKeyExistsAsync(*Store, *storeTxn->StoreTransactionSPtr, key, nullptr, value);
                co_await storeTxn->CommitAsync();
            }

            co_await VerifyKeyExistsInStoresAsync(key, nullptr, value, EqualityFunction);
            co_return;
        }

        ktl::Awaitable<void> AddUpdate_SingleTransaction_ShouldSucceed_Test()
        {
            // Create same key twice with different SPtrs
            KString::SPtr key1;
            auto status = KString::Create(key1, GetAllocator(), L"test_key");
            Diagnostics::Validate(status);

            KString::SPtr key2;
            status = KString::Create(key2, GetAllocator(), L"test_key");
            Diagnostics::Validate(status);
        
            auto value = MakeBuffer(100, 1);
            auto updatedValue = MakeBuffer(100, 2);

            {
                auto storeTxn = CreateWriteTransaction();
                co_await Store->AddAsync(*storeTxn->StoreTransactionSPtr, key1, value, Common::TimeSpan::MaxValue, CancellationToken::None);
                co_await VerifyKeyExistsAsync(*Store, *storeTxn->StoreTransactionSPtr, key1, nullptr, value);
                co_await Store->ConditionalUpdateAsync(*storeTxn->StoreTransactionSPtr, key2, updatedValue, DefaultTimeout, CancellationToken::None);
                co_await VerifyKeyExistsAsync(*Store, *storeTxn->StoreTransactionSPtr, key1, nullptr, updatedValue);
                co_await storeTxn->CommitAsync();
            }

            co_await VerifyKeyExistsInStoresAsync(key1, nullptr, updatedValue, EqualityFunction);
            co_return;
        }

        ktl::Awaitable<void> AddDelete_SingleTransaction_ShouldSucceed_Test()
        {
            KString::SPtr key;
            auto status = KString::Create(key, GetAllocator(), L"test_key");
            Diagnostics::Validate(status);
        
            auto value = MakeBuffer(100);
        
            {
                auto storeTxn = CreateWriteTransaction();
                co_await Store->AddAsync(*storeTxn->StoreTransactionSPtr, key, value, Common::TimeSpan::MaxValue, CancellationToken::None);
                co_await Store->ConditionalRemoveAsync(*storeTxn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None);
                co_await storeTxn->CommitAsync();
            }

            co_await VerifyKeyDoesNotExistInStoresAsync(key);
            co_return;
        }

        ktl::Awaitable<void> AddDeleteAdd_SingleTransaction_ShouldSucceed_Test()
        {
            KString::SPtr key;
            auto status = KString::Create(key, GetAllocator(), L"test_key");
            Diagnostics::Validate(status);

            auto value = MakeBuffer(100);

            {
                auto storeTxn = CreateWriteTransaction();
                co_await Store->AddAsync(*storeTxn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await Store->ConditionalRemoveAsync(*storeTxn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*storeTxn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await VerifyKeyExistsAsync(*Store, *storeTxn->StoreTransactionSPtr, key, nullptr, value);
                co_await storeTxn->CommitAsync();
            }

            co_await VerifyKeyExistsInStoresAsync(key, nullptr, value, EqualityFunction);
            co_return;
        }

        ktl::Awaitable<void> MultipleAdds_SingleTransaction_ShouldSucceed_Test()
        {
            KString::SPtr key1;
            KString::SPtr key2;
            KString::SPtr key3;

            Diagnostics::Validate(KString::Create(key1, GetAllocator(), L"test_key1"));
            Diagnostics::Validate(KString::Create(key2, GetAllocator(), L"test_key2"));
            Diagnostics::Validate(KString::Create(key3, GetAllocator(), L"test_key3"));

            auto value1 = MakeBuffer(100, 1);
            auto value2 = MakeBuffer(100, 2);
            auto value3 = MakeBuffer(100, 3);

            {
                auto storeTxn = CreateWriteTransaction();

                co_await Store->AddAsync(*storeTxn->StoreTransactionSPtr, key1, value1, DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*storeTxn->StoreTransactionSPtr, key2, value2, DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*storeTxn->StoreTransactionSPtr, key3, value3, DefaultTimeout, CancellationToken::None);

                co_await storeTxn->CommitAsync();
            }
        
            co_await VerifyKeyExistsInStoresAsync(key1, nullptr, value1, EqualityFunction);
            co_await VerifyKeyExistsInStoresAsync(key2, nullptr, value2, EqualityFunction);
            co_await VerifyKeyExistsInStoresAsync(key3, nullptr, value3, EqualityFunction);
            co_return;
        }

        ktl::Awaitable<void> MultipleUpdates_SingleTransaction_ShouldSucceed_Test()
        {

            KString::SPtr key;
            Diagnostics::Validate(KString::Create(key, GetAllocator(), L"test_key"));

            KBuffer::SPtr expectedValue = nullptr;

            {
                auto storeTxn = CreateWriteTransaction();

                auto value = MakeBuffer(100, 1);
                co_await Store->AddAsync(*storeTxn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);

                for (int i = 0; i < 10; i++)
                {
                    expectedValue = MakeBuffer(100, i);
                    co_await Store->ConditionalUpdateAsync(*storeTxn->StoreTransactionSPtr, key, expectedValue, DefaultTimeout, CancellationToken::None);
                }

                co_await storeTxn->CommitAsync();
            }
        
            co_await VerifyKeyExistsInStoresAsync(key, nullptr, expectedValue, EqualityFunction);
            co_return;
        }

        ktl::Awaitable<void> AddUpdate_DifferentTransactions_ShouldSucceed_Test()
        {
            KString::SPtr key;
            Diagnostics::Validate(KString::Create(key, GetAllocator(), L"test_key"));

            auto value = MakeBuffer(100, 1);
            auto updatedValue = MakeBuffer(100, 2);

            {
                auto storeTxn1 = CreateWriteTransaction();
                co_await Store->AddAsync(*storeTxn1->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await VerifyKeyExistsAsync(*Store, *storeTxn1->StoreTransactionSPtr, key, nullptr, value);
                co_await storeTxn1->CommitAsync();
            }

            {
                auto storeTxn2 = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*storeTxn2->StoreTransactionSPtr, key, updatedValue, DefaultTimeout, CancellationToken::None);
                co_await storeTxn2->CommitAsync();
            }
        
            co_await VerifyKeyExistsInStoresAsync(key, nullptr, updatedValue, EqualityFunction);
            co_return;
        }

        ktl::Awaitable<void> AddDelete_DifferentTransactions_ShouldSucceed_Test()
        {
        
            KString::SPtr key;
            Diagnostics::Validate(KString::Create(key, GetAllocator(), L"test_key"));

            auto value = MakeBuffer(100);

            {
                auto storeTxn1 = CreateWriteTransaction();
                co_await Store->AddAsync(*storeTxn1->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await VerifyKeyExistsAsync(*Store, *storeTxn1->StoreTransactionSPtr, key, nullptr, value);
                co_await storeTxn1->CommitAsync();
            }

            {
                auto storeTxn2 = CreateWriteTransaction();
                bool result = co_await Store->ConditionalRemoveAsync(*storeTxn2->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None);
                CODING_ERROR_ASSERT(result);
                co_await storeTxn2->CommitAsync();
            }

            co_await VerifyKeyDoesNotExistInStoresAsync(key);
            co_return;
        }

        ktl::Awaitable<void> AddUpdateRead_DifferentTransactions_ShouldSucceed_Test()
        {
            KString::SPtr key;
            Diagnostics::Validate(KString::Create(key, GetAllocator(), L"test_key"));

            auto value = MakeBuffer(100, 1);
            auto updatedValue = MakeBuffer(100, 2);

            {
                auto storeTxn1 = CreateWriteTransaction();
                co_await Store->AddAsync(*storeTxn1->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await VerifyKeyExistsAsync(*Store, *storeTxn1->StoreTransactionSPtr, key, nullptr, value);
                co_await storeTxn1->CommitAsync();
            }

            {
                auto storeTxn2 = CreateWriteTransaction();
                bool result = co_await Store->ConditionalUpdateAsync(*storeTxn2->StoreTransactionSPtr, key, updatedValue, DefaultTimeout, CancellationToken::None);
                CODING_ERROR_ASSERT(result);
                co_await storeTxn2->CommitAsync();
            }

            co_await VerifyKeyExistsInStoresAsync(key, nullptr, updatedValue, EqualityFunction);
            co_return;
        }

        ktl::Awaitable<void> MultipleAdds_MultipleTransactions_ShouldSucceed_Test()
        {
            KString::SPtr key1;
            KString::SPtr key2;
            KString::SPtr key3;

            Diagnostics::Validate(KString::Create(key1, GetAllocator(), L"test_key1"));
            Diagnostics::Validate(KString::Create(key2, GetAllocator(), L"test_key2"));
            Diagnostics::Validate(KString::Create(key3, GetAllocator(), L"test_key3"));
        
            auto value1 = MakeBuffer(100, 1);
            auto value2 = MakeBuffer(100, 2);
            auto value3 = MakeBuffer(100, 3);

            {
                auto storeTxn1 = CreateWriteTransaction();
                co_await Store->AddAsync(*storeTxn1->StoreTransactionSPtr, key1, value1, DefaultTimeout, CancellationToken::None);
                co_await storeTxn1->CommitAsync();
            }

            {
                auto storeTxn2 = CreateWriteTransaction();
                co_await Store->AddAsync(*storeTxn2->StoreTransactionSPtr, key2, value2, DefaultTimeout, CancellationToken::None);
                co_await storeTxn2->CommitAsync();
            }

            {
                auto storeTxn3 = CreateWriteTransaction();
                co_await Store->AddAsync(*storeTxn3->StoreTransactionSPtr, key3, value3, DefaultTimeout, CancellationToken::None);
                co_await storeTxn3->CommitAsync();
            }


            for (ULONG m = 0; m < Stores->Count(); m++)
            {
                auto storeVerifyTxn = CreateWriteTransaction(*(*Stores)[m]);
                co_await VerifyKeyExistsAsync(*(*Stores)[m], *storeVerifyTxn->StoreTransactionSPtr, key1, nullptr, value1, EqualityFunction);
                co_await VerifyKeyExistsAsync(*(*Stores)[m], *storeVerifyTxn->StoreTransactionSPtr, key2, nullptr, value2, EqualityFunction);
                co_await VerifyKeyExistsAsync(*(*Stores)[m], *storeVerifyTxn->StoreTransactionSPtr, key3, nullptr, value3, EqualityFunction);
                co_await storeVerifyTxn->AbortAsync();
            }
            co_return;
        }

        ktl::Awaitable<void> MultipleAddsUpdates_MultipleTransactions_ShouldSucceed_Test()
        {
            KString::SPtr key1;
            KString::SPtr key2;
            KString::SPtr key3;

            Diagnostics::Validate(KString::Create(key1, GetAllocator(), L"test_key1"));
            Diagnostics::Validate(KString::Create(key2, GetAllocator(), L"test_key2"));
            Diagnostics::Validate(KString::Create(key3, GetAllocator(), L"test_key3"));

            auto value1 = MakeBuffer(100, 1);
            auto value1Updated = MakeBuffer(100, 10);

            auto value2 = MakeBuffer(100, 2);
            auto value2Updated = MakeBuffer(100, 20);

            auto value3 = MakeBuffer(100, 3);
            auto value3Updated = MakeBuffer(100, 30);
        
            // Add keys to the store

            {
                auto storeAddTxn1 = CreateWriteTransaction();
                co_await Store->AddAsync(*storeAddTxn1->StoreTransactionSPtr, key1, value1, DefaultTimeout, CancellationToken::None);
                co_await storeAddTxn1->CommitAsync();
            }

            {
                auto storeAddTxn2 = CreateWriteTransaction();
                co_await Store->AddAsync(*storeAddTxn2->StoreTransactionSPtr, key2, value2, DefaultTimeout, CancellationToken::None);
                co_await storeAddTxn2->CommitAsync();
            }

            {
                auto storeAddTxn3 = CreateWriteTransaction();
                co_await Store->AddAsync(*storeAddTxn3->StoreTransactionSPtr, key3, value3, DefaultTimeout, CancellationToken::None);
                co_await storeAddTxn3->CommitAsync();
            }

            // Update key1 values
            {
                auto storeUpdateTxn1 = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*storeUpdateTxn1->StoreTransactionSPtr, key1, value1Updated, DefaultTimeout, CancellationToken::None);
                co_await storeUpdateTxn1->CommitAsync();
            }


            {
                auto storeUpdateTxn2 = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*storeUpdateTxn2->StoreTransactionSPtr, key2, value2Updated, DefaultTimeout, CancellationToken::None);
                co_await storeUpdateTxn2->CommitAsync();
            }

            {
                auto storeUpdateTxn3 = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*storeUpdateTxn3->StoreTransactionSPtr, key3, value3Updated, DefaultTimeout, CancellationToken::None);
                co_await storeUpdateTxn3->CommitAsync();
            }

            // Verify Results
            for (ULONG m = 0; m < Stores->Count(); m++)
            {
                auto storeVerifyTxn = CreateWriteTransaction(*(*Stores)[m]);
                co_await VerifyKeyExistsAsync(*(*Stores)[m], *storeVerifyTxn->StoreTransactionSPtr, key1, nullptr, value1Updated, EqualityFunction);
                co_await VerifyKeyExistsAsync(*(*Stores)[m], *storeVerifyTxn->StoreTransactionSPtr, key2, nullptr, value2Updated, EqualityFunction);
                co_await VerifyKeyExistsAsync(*(*Stores)[m], *storeVerifyTxn->StoreTransactionSPtr, key3, nullptr, value3Updated, EqualityFunction);
                co_await storeVerifyTxn->AbortAsync();
            }
            co_return;
        }

        ktl::Awaitable<void> AddSameKey_MultipleTransactions_ShouldFail_Test()
        {
            KString::SPtr key;
            Diagnostics::Validate(KString::Create(key, GetAllocator(), L"test_key"));

            auto value1 = MakeBuffer(100, 1);
            auto value2 = MakeBuffer(100, 2);

            {
                auto storeTxn1 = CreateWriteTransaction();
                co_await Store->AddAsync(*storeTxn1->StoreTransactionSPtr, key, value1, DefaultTimeout, CancellationToken::None);
                co_await storeTxn1->CommitAsync();
            }

            bool hasEx = false;

            auto storeTxn2 = CreateWriteTransaction();
            try
            {
                co_await Store->AddAsync(*storeTxn2->StoreTransactionSPtr, key, value2, DefaultTimeout, CancellationToken::None);
                co_await storeTxn2->CommitAsync();
            }
            catch (Exception const & e)
            {
                CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_WRITE_CONFLICT);
                hasEx = true;
            }
            co_await storeTxn2->AbortAsync();

            CODING_ERROR_ASSERT(hasEx);
            co_return;
        }

        ktl::Awaitable<void> Add_Abort_ShouldSucceed_Test()
        {
            KString::SPtr key;
            Diagnostics::Validate(KString::Create(key, GetAllocator(), L"test_key"));

            auto value = MakeBuffer(100);

            {
                auto storeTxn = CreateWriteTransaction();
                co_await Store->AddAsync(*storeTxn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);

                KeyValuePair<LONG64, KBuffer::SPtr> kvpair(-1, nullptr);
                co_await Store->ConditionalGetAsync(*storeTxn->StoreTransactionSPtr, key, DefaultTimeout, kvpair, CancellationToken::None);
                CODING_ERROR_ASSERT(kvpair.Value == value);
                co_await storeTxn->AbortAsync();
            }
        
            co_await VerifyKeyDoesNotExistInStoresAsync(key);
            co_return;
        }

        ktl::Awaitable<void> UndoFalseProgress_UndoAdd_ShouldSucceed_Test()
        {
            ULONG32 count = 10;

            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx1 = CreateWriteTransaction();

                for (ULONG32 idx = 0; idx < count; idx++)
                {
                    co_await Store->AddAsync(*tx1->StoreTransactionSPtr, ToString(idx), ToBuffer(idx), DefaultTimeout, CancellationToken::None);
                }

                co_await Replicator->SimulateFalseProgressAsync(*tx1->TransactionSPtr);

                //Abort here to unlock the resources, same as managed use using block to dispose the resource.
                co_await tx1->AbortAsync();
            }

            for (ULONG32 idx = 0; idx < count; idx++)
            {
                co_await VerifyKeyDoesNotExistInStoresAsync(ToString(idx));
            }
            co_return;
        }

        ktl::Awaitable<void> UndoFalseProgress_UndoUpdate_ShouldSucceed_Test()
        {
            ULONG32 count = 10;

            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx1 = CreateWriteTransaction();

                for (ULONG32 idx = 0; idx < count; idx++)
                {
                    co_await Store->AddAsync(*tx1->StoreTransactionSPtr, ToString(idx), ToBuffer(idx), DefaultTimeout, CancellationToken::None);
                }

                co_await tx1->CommitAsync();
            }

            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx1 = CreateWriteTransaction();

                for (ULONG32 idx = 0; idx < count; idx++)
                {
                    bool success = co_await Store->ConditionalUpdateAsync(*tx1->StoreTransactionSPtr, ToString(idx), ToBuffer(idx + 1), DefaultTimeout, CancellationToken::None);
                    CODING_ERROR_ASSERT(success);
                }

                co_await Replicator->SimulateFalseProgressAsync(*tx1->TransactionSPtr);

                //Abort here to unlock the resources, same as managed use using block to dispose the resource.
                co_await tx1->AbortAsync();
            }

            for (ULONG32 idx = 0; idx < count; idx++)
            {
                co_await VerifyKeyExistsInStoresAsync(ToString(idx), MakeBuffer(0), ToBuffer(idx), EqualityFunction);
            }
            co_return;
        }

        ktl::Awaitable<void> UndoFalseProgress_UndoRemove_ShouldSucceed_Test()
        {
            ULONG32 count = 10;

            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx1 = CreateWriteTransaction();

                for (ULONG32 idx = 0; idx < count; idx++)
                {
                    co_await Store->AddAsync(*tx1->StoreTransactionSPtr, ToString(idx), ToBuffer(idx), DefaultTimeout, CancellationToken::None);
                }

                co_await tx1->CommitAsync();
            }

            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx1 = CreateWriteTransaction();

                for (ULONG32 idx = 0; idx < count; idx++)
                {
                    bool success = co_await Store->ConditionalRemoveAsync(*tx1->StoreTransactionSPtr, ToString(idx), DefaultTimeout, CancellationToken::None);
                    CODING_ERROR_ASSERT(success);
                }

                co_await Replicator->SimulateFalseProgressAsync(*tx1->TransactionSPtr);

                //Abort here to unlock the resources, same as managed use using block to dispose the resource.
                co_await tx1->AbortAsync();
            }

            for (ULONG32 idx = 0; idx < count; idx++)
            {
                co_await VerifyKeyExistsInStoresAsync(ToString(idx), MakeBuffer(0), ToBuffer(idx), EqualityFunction);
            }
            co_return;
        }

        ktl::Awaitable<void> UndoFalseProgress_UndoAddUpdateRemove_ShouldSucceed_Test()
        {
            ULONG32 count = 10;

            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx1 = CreateWriteTransaction();

                for (ULONG32 idx = 0; idx < count; idx++)
                {
                    co_await Store->AddAsync(*tx1->StoreTransactionSPtr, ToString(idx), ToBuffer(idx), DefaultTimeout, CancellationToken::None);

                    if (idx % 3 == 0)
                    {
                        bool success = co_await Store->ConditionalUpdateAsync(*tx1->StoreTransactionSPtr, ToString(idx), ToBuffer(idx + 1), DefaultTimeout, CancellationToken::None);
                        CODING_ERROR_ASSERT(success);
                    }
                    else if (idx % 3 == 1)
                    {
                        bool success = co_await Store->ConditionalRemoveAsync(*tx1->StoreTransactionSPtr, ToString(idx), DefaultTimeout, CancellationToken::None);
                        CODING_ERROR_ASSERT(success);
                    }
                }

                co_await Replicator->SimulateFalseProgressAsync(*tx1->TransactionSPtr);

                //Abort here to unlock the resources, same as managed use using block to dispose the resource.
                co_await tx1->AbortAsync();
            }

            for (ULONG32 idx = 0; idx < count; idx++)
            {
                co_await VerifyKeyDoesNotExistInStoresAsync(ToString(idx));
            }
            co_return;
        }

        ktl::Awaitable<void> UndoFalseProgress_UndoCRUD_ShouldSucceed_Test()
        {
            KString::SPtr key1 = ToString(5);
            KString::SPtr key2 = ToString(6);
            KString::SPtr key3 = ToString(7);
            KString::SPtr key4 = ToString(8);
            KString::SPtr key5 = ToString(9);
            KString::SPtr key6 = ToString(10);
            KString::SPtr key7 = ToString(11);

            KBuffer::SPtr value1 = ToBuffer(32);
            KBuffer::SPtr value1_updated = ToBuffer(42);
            KBuffer::SPtr value2 = ToBuffer(33);
            KBuffer::SPtr value2_updated = ToBuffer(43);
            KBuffer::SPtr value3 = ToBuffer(34);
            KBuffer::SPtr value3_updated = ToBuffer(44);
            KBuffer::SPtr value4 = ToBuffer(35);
            KBuffer::SPtr value4_updated = ToBuffer(45);
            KBuffer::SPtr value5 = ToBuffer(36);
            KBuffer::SPtr value5_updated = ToBuffer(46);
            KBuffer::SPtr value6 = ToBuffer(37);
            KBuffer::SPtr value6_updated = ToBuffer(47);
            KBuffer::SPtr value7 = ToBuffer(38);

            // First add some keys to operate on
            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx1 = CreateWriteTransaction();
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key1, value1, DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key2, value2, DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key3, value3, DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key4, value4, DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key5, value5, DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key6, value6, DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key7, value7, DefaultTimeout, CancellationToken::None);
                co_await tx1->CommitAsync();
            }

            bool success = false;

            // Create another transaction updates the values.
            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx2 = CreateWriteTransaction();
                success = co_await Store->ConditionalRemoveAsync(*tx2->StoreTransactionSPtr, key5, DefaultTimeout, CancellationToken::None);
                success = co_await Store->ConditionalRemoveAsync(*tx2->StoreTransactionSPtr, key6, DefaultTimeout, CancellationToken::None);
                success = co_await Store->ConditionalRemoveAsync(*tx2->StoreTransactionSPtr, key7, DefaultTimeout, CancellationToken::None);
                co_await tx2->CommitAsync();
            }

            // Simulate false progress after transiently performing create/update/delete operations on the keys then undoing the operations
            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx3 = CreateWriteTransaction();
                success = co_await Store->ConditionalRemoveAsync(*tx3->StoreTransactionSPtr, key1, DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*tx3->StoreTransactionSPtr, key1, ToBuffer(55), DefaultTimeout, CancellationToken::None);

                success = co_await Store->ConditionalUpdateAsync(*tx3->StoreTransactionSPtr, key2, ToBuffer(56), DefaultTimeout, CancellationToken::None);
                success = co_await Store->ConditionalRemoveAsync(*tx3->StoreTransactionSPtr, key2, DefaultTimeout, CancellationToken::None);

                success = co_await Store->ConditionalRemoveAsync(*tx3->StoreTransactionSPtr, key3, DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*tx3->StoreTransactionSPtr, key3, value3, DefaultTimeout, CancellationToken::None);
                success = co_await Store->ConditionalUpdateAsync(*tx3->StoreTransactionSPtr, key3, ToBuffer(57), DefaultTimeout, CancellationToken::None);

                success = co_await Store->ConditionalUpdateAsync(*tx3->StoreTransactionSPtr, key4, ToBuffer(58), DefaultTimeout, CancellationToken::None);
                success = co_await Store->ConditionalUpdateAsync(*tx3->StoreTransactionSPtr, key4, ToBuffer(59), DefaultTimeout, CancellationToken::None);
                success = co_await Store->ConditionalUpdateAsync(*tx3->StoreTransactionSPtr, key4, ToBuffer(60), DefaultTimeout, CancellationToken::None);

                co_await Store->AddAsync(*tx3->StoreTransactionSPtr, key5, ToBuffer(61), DefaultTimeout, CancellationToken::None);

                co_await Store->AddAsync(*tx3->StoreTransactionSPtr, key6, value6, DefaultTimeout, CancellationToken::None);
                success = co_await Store->ConditionalUpdateAsync(*tx3->StoreTransactionSPtr, key6, ToBuffer(62), DefaultTimeout, CancellationToken::None);

                co_await Store->AddAsync(*tx3->StoreTransactionSPtr, key7, ToBuffer(63), DefaultTimeout, CancellationToken::None);
                success = co_await Store->ConditionalRemoveAsync(*tx3->StoreTransactionSPtr, key7, DefaultTimeout, CancellationToken::None);

                co_await Replicator->SimulateFalseProgressAsync(*tx3->TransactionSPtr);
                co_await tx3->AbortAsync();
            }

            // Verify the keys were not removed on any replicas
            co_await VerifyKeyExistsInStoresAsync(key1, MakeBuffer(0), value1, EqualityFunction);
            co_await VerifyKeyExistsInStoresAsync(key2, MakeBuffer(0), value2, EqualityFunction);
            co_await VerifyKeyExistsInStoresAsync(key3, MakeBuffer(0), value3, EqualityFunction);
            co_await VerifyKeyExistsInStoresAsync(key4, MakeBuffer(0), value4, EqualityFunction);
            co_await VerifyKeyDoesNotExistInStoresAsync(key5);
            co_await VerifyKeyDoesNotExistInStoresAsync(key6);
            co_await VerifyKeyDoesNotExistInStoresAsync(key7);
            co_return;
        }

        ktl::Awaitable<void> UndoFalseProgress_UndoUpdateWithCheckpoint_ShouldSucceed_Test()
        {
            ULONG32 count = 10;

            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx1 = CreateWriteTransaction();

                for (ULONG32 idx = 0; idx < count; idx++)
                {
                    co_await Store->AddAsync(*tx1->StoreTransactionSPtr, ToString(idx), ToBuffer(idx), DefaultTimeout, CancellationToken::None);
                }

                co_await tx1->CommitAsync();
            }

            co_await CheckpointAsync();

            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx1 = CreateWriteTransaction();

                for (ULONG32 idx = 0; idx < count; idx++)
                {
                    bool success = co_await Store->ConditionalUpdateAsync(*tx1->StoreTransactionSPtr, ToString(idx), ToBuffer(idx + 1), DefaultTimeout, CancellationToken::None);
                    CODING_ERROR_ASSERT(success);
                }

                co_await Replicator->SimulateFalseProgressAsync(*tx1->TransactionSPtr);

                //Abort here to unlock the resources, same as managed use using block to dispose the resource.
                co_await tx1->AbortAsync();
            }

            for (ULONG32 idx = 0; idx < count; idx++)
            {
                co_await VerifyKeyExistsInStoresAsync(ToString(idx), MakeBuffer(0), ToBuffer(idx), EqualityFunction);
            }
            co_return;
        }

        ktl::Awaitable<void> UndoFalseProgress_UndoRemoveWithCheckpoint_ShouldSucceed_Test()
        {
            ULONG32 count = 10;

            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx1 = CreateWriteTransaction();

                for (ULONG32 idx = 0; idx < count; idx++)
                {
                    co_await Store->AddAsync(*tx1->StoreTransactionSPtr, ToString(idx), ToBuffer(idx), DefaultTimeout, CancellationToken::None);
                }

                co_await tx1->CommitAsync();
            }

            co_await CheckpointAsync();

            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx1 = CreateWriteTransaction();

                for (ULONG32 idx = 0; idx < count; idx++)
                {
                    bool success = co_await Store->ConditionalRemoveAsync(*tx1->StoreTransactionSPtr, ToString(idx), DefaultTimeout, CancellationToken::None);
                    CODING_ERROR_ASSERT(success);
                }

                co_await Replicator->SimulateFalseProgressAsync(*tx1->TransactionSPtr);

                //Abort here to unlock the resources, same as managed use using block to dispose the resource.
                co_await tx1->AbortAsync();
            }

            for (ULONG32 idx = 0; idx < count; idx++)
            {
                co_await VerifyKeyExistsInStoresAsync(ToString(idx), MakeBuffer(0), ToBuffer(idx), EqualityFunction);
            }
            co_return;
        }

        ktl::Awaitable<void> UndoFalseProgress_UndoCRUDwithCheckpoint__ShouldSucceed_Test()
        {
            KString::SPtr key1 = ToString(5);
            KString::SPtr key2 = ToString(6);
            KString::SPtr key3 = ToString(7);
            KString::SPtr key4 = ToString(8);
            KString::SPtr key5 = ToString(9);
            KString::SPtr key6 = ToString(10);
            KString::SPtr key7 = ToString(11);

            KBuffer::SPtr value1 = ToBuffer(32);
            KBuffer::SPtr value1_updated = ToBuffer(42);
            KBuffer::SPtr value2 = ToBuffer(33);
            KBuffer::SPtr value2_updated = ToBuffer(43);
            KBuffer::SPtr value3 = ToBuffer(34);
            KBuffer::SPtr value3_updated = ToBuffer(44);
            KBuffer::SPtr value4 = ToBuffer(35);
            KBuffer::SPtr value4_updated = ToBuffer(45);
            KBuffer::SPtr value5 = ToBuffer(36);
            KBuffer::SPtr value5_updated = ToBuffer(46);
            KBuffer::SPtr value6 = ToBuffer(37);
            KBuffer::SPtr value6_updated = ToBuffer(47);
            KBuffer::SPtr value7 = ToBuffer(38);

            // First add some keys to operate on
            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx1 = CreateWriteTransaction();
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key1, value1, DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key2, value2, DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key3, value3, DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key4, value4, DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key5, value5, DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key6, value6, DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key7, value7, DefaultTimeout, CancellationToken::None);
                co_await tx1->CommitAsync();
            }

            bool success = false;

            // Create another transaction updates the values.
            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx2 = CreateWriteTransaction();
                success = co_await Store->ConditionalRemoveAsync(*tx2->StoreTransactionSPtr, key5, DefaultTimeout, CancellationToken::None);
                success = co_await Store->ConditionalRemoveAsync(*tx2->StoreTransactionSPtr, key6, DefaultTimeout, CancellationToken::None);
                success = co_await Store->ConditionalRemoveAsync(*tx2->StoreTransactionSPtr, key7, DefaultTimeout, CancellationToken::None);
                co_await tx2->CommitAsync();
            }

            // Perform a checkpoint on all replicas before proceeding
            co_await CheckpointAsync();

            // Simulate false progress after transiently performing create/update/delete operations on the keys then undoing the operations
            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx3 = CreateWriteTransaction();
                success = co_await Store->ConditionalRemoveAsync(*tx3->StoreTransactionSPtr, key1, DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*tx3->StoreTransactionSPtr, key1, ToBuffer(55), DefaultTimeout, CancellationToken::None);

                success = co_await Store->ConditionalUpdateAsync(*tx3->StoreTransactionSPtr, key2, ToBuffer(56), DefaultTimeout, CancellationToken::None);
                success = co_await Store->ConditionalRemoveAsync(*tx3->StoreTransactionSPtr, key2, DefaultTimeout, CancellationToken::None);

                success = co_await Store->ConditionalRemoveAsync(*tx3->StoreTransactionSPtr, key3, DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*tx3->StoreTransactionSPtr, key3, value3, DefaultTimeout, CancellationToken::None);
                success = co_await Store->ConditionalUpdateAsync(*tx3->StoreTransactionSPtr, key3, ToBuffer(57), DefaultTimeout, CancellationToken::None);

                success = co_await Store->ConditionalUpdateAsync(*tx3->StoreTransactionSPtr, key4, ToBuffer(58), DefaultTimeout, CancellationToken::None);
                success = co_await Store->ConditionalUpdateAsync(*tx3->StoreTransactionSPtr, key4, ToBuffer(59), DefaultTimeout, CancellationToken::None);
                success = co_await Store->ConditionalUpdateAsync(*tx3->StoreTransactionSPtr, key4, ToBuffer(60), DefaultTimeout, CancellationToken::None);

                co_await Store->AddAsync(*tx3->StoreTransactionSPtr, key5, ToBuffer(61), DefaultTimeout, CancellationToken::None);

                co_await Store->AddAsync(*tx3->StoreTransactionSPtr, key6, value6, DefaultTimeout, CancellationToken::None);
                success = co_await Store->ConditionalUpdateAsync(*tx3->StoreTransactionSPtr, key6, ToBuffer(62), DefaultTimeout, CancellationToken::None);

                co_await Store->AddAsync(*tx3->StoreTransactionSPtr, key7, ToBuffer(63), DefaultTimeout, CancellationToken::None);
                success = co_await Store->ConditionalRemoveAsync(*tx3->StoreTransactionSPtr, key7, DefaultTimeout, CancellationToken::None);

                co_await Replicator->SimulateFalseProgressAsync(*tx3->TransactionSPtr);
                co_await tx3->AbortAsync();
            }

            // Verify the keys were not removed on any replicas
            co_await VerifyKeyExistsInStoresAsync(key1, MakeBuffer(0), value1, EqualityFunction);
            co_await VerifyKeyExistsInStoresAsync(key2, MakeBuffer(0), value2, EqualityFunction);
            co_await VerifyKeyExistsInStoresAsync(key3, MakeBuffer(0), value3, EqualityFunction);
            co_await VerifyKeyExistsInStoresAsync(key4, MakeBuffer(0), value4, EqualityFunction);
            co_await VerifyKeyDoesNotExistInStoresAsync(key5);
            co_await VerifyKeyDoesNotExistInStoresAsync(key6);
            co_await VerifyKeyDoesNotExistInStoresAsync(key7);
            co_return;
        }

        ktl::Awaitable<void> UndoFalseProgress_SameKeyMutipleOperationsSingleTransaction_ShouldSucceed_Test()
        {
            KString::SPtr key = ToString(1);
            KBuffer::SPtr value = ToBuffer(32);
            ULONG32 updatedValue = 33;

            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx1 = CreateWriteTransaction();
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await tx1->CommitAsync();
            }

            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx1 = CreateWriteTransaction();
                for (ULONG i = 0; i < 10; i++)
                {
                    bool success = co_await Store->ConditionalUpdateAsync(*tx1->StoreTransactionSPtr, key, ToBuffer(++updatedValue), DefaultTimeout, CancellationToken::None);
                    CODING_ERROR_ASSERT(success);
                }

                co_await Replicator->SimulateFalseProgressAsync(*tx1->TransactionSPtr);

                //Abort here to unlock the resources, same as managed use using block to dispose the resource.
                co_await tx1->AbortAsync();
            }

            co_await VerifyKeyExistsInStoresAsync(key, MakeBuffer(0), value, EqualityFunction);
            co_return;
        }

        ktl::Awaitable<void> SnapshotRead_FromDifferentialStoreAfterOneUpdate_ShouldSucceed_Test()
        {
            KString::SPtr key = ToString(1);
            KBuffer::SPtr value = ToBuffer(32);
            KBuffer::SPtr updateValue = ToBuffer(33);

            // Add
            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx1 = CreateWriteTransaction();
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await tx1->CommitAsync();
            }

            // Start the snapshot transaction here
            KArray<WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr> storesTransactions(GetAllocator());

            for (ULONG i = 0; i < Stores->Count(); i++)
            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx = CreateWriteTransaction(*(*Stores)[i]);
                tx->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
                storesTransactions.Append(tx);
                co_await VerifyKeyExistsAsync(*(*Stores)[i], *tx->StoreTransactionSPtr, key, MakeBuffer(0), value, EqualityFunction);
            }

            // Update causes entries to previous version.
            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx2 = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*tx2->StoreTransactionSPtr, key, updateValue, DefaultTimeout, CancellationToken::None);
                co_await tx2->CommitAsync();
            }

            co_await VerifyKeyExistsInStoresAsync(key, MakeBuffer(0), updateValue, EqualityFunction);

            for (ULONG i = 0; i < storesTransactions.Count(); i++)
            {
                //This will read from differential previous version.
                co_await VerifyKeyExistsAsync(*storesTransactions[i]->StateProviderSPtr, *storesTransactions[i]->StoreTransactionSPtr, key, MakeBuffer(0), value, EqualityFunction);
                co_await storesTransactions[i]->AbortAsync();
            }

            storesTransactions.Clear();
            co_return;
        }

        ktl::Awaitable<void> SnapshotRead_DuringCheckpoint_ShouldSucceed_Test()
        {
            KString::SPtr key = ToString(1);
            KBuffer::SPtr value = ToBuffer(32);
            KBuffer::SPtr updateValue = ToBuffer(33);

            // Add
            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx1 = CreateWriteTransaction();
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await tx1->CommitAsync();
            }

            // Start the snapshot transaction here
            KArray<WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr> storesTransactions(GetAllocator());

            for (ULONG i = 0; i < Stores->Count(); i++)
            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx = CreateWriteTransaction(*(*Stores)[i]);
                tx->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
                storesTransactions.Append(tx);
                co_await VerifyKeyExistsAsync(*(*Stores)[i], *tx->StoreTransactionSPtr, key, MakeBuffer(0), value, EqualityFunction);
            }

            // Update causes entries to previous version.
            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx2 = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*tx2->StoreTransactionSPtr, key, updateValue, DefaultTimeout, CancellationToken::None);
                co_await tx2->CommitAsync();
            }

            StoresPrepareCheckpoint();

            for (ULONG i = 0; i < storesTransactions.Count(); i++)
            {
                //This will read from aggregatedstate highest index of differential store (previous version).
                co_await VerifyKeyExistsAsync(*storesTransactions[i]->StateProviderSPtr, *storesTransactions[i]->StoreTransactionSPtr, key, MakeBuffer(0), value, EqualityFunction);
            }

            co_await VerifyKeyExistsInStoresAsync(key, MakeBuffer(0), updateValue, EqualityFunction);

            co_await PerformCheckpointOnStoresAsync();
            co_await CompleteCheckpointOnStoresAsync();

            for (ULONG i = 0; i < storesTransactions.Count(); i++)
            {
                //This will read from snapshot container (lsn not in consolidated since the update).
                co_await VerifyKeyExistsAsync(*storesTransactions[i]->StateProviderSPtr, *storesTransactions[i]->StoreTransactionSPtr, key, MakeBuffer(0), value, EqualityFunction);
                co_await storesTransactions[i]->AbortAsync();
            }

            co_await VerifyKeyExistsInStoresAsync(key, MakeBuffer(0), updateValue, EqualityFunction);

            storesTransactions.Clear();
            co_return;
        }

        ktl::Awaitable<void> SnapshotRead_GetValueOnDisk_FromConsolidatedComponentBeforeCompleteCheckpoint_ShouldSucceed_Test()
        {
            KString::SPtr key = ToString(1);
            KBuffer::SPtr value = ToBuffer(32);
            KBuffer::SPtr updateValue = ToBuffer(33);

            // Add
            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx1 = CreateWriteTransaction();
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await tx1->CommitAsync();
            }

            co_await CheckpointAsync();
            co_await CloseAndReOpenStoreAsync();

            // Update causes entries to previous version.
            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx2 = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*tx2->StoreTransactionSPtr, key, updateValue, DefaultTimeout, CancellationToken::None);
                co_await tx2->CommitAsync();
            }

            StoresPrepareCheckpoint();
            co_await PerformCheckpointOnStoresAsync();

            co_await VerifyKeyExistsInStoresAsync(key, MakeBuffer(0), updateValue, EqualityFunction);

            co_await CompleteCheckpointOnStoresAsync();

            co_await VerifyKeyExistsInStoresAsync(key, MakeBuffer(0), updateValue, EqualityFunction);
            co_return;
        }

        ktl::Awaitable<void> Snapshot_GetValueOnDisk_FromConsolidatedAndSnapshotComponent_ShouldSucceed_Test()
        {
            KString::SPtr key = ToString(1);
            KBuffer::SPtr value = ToBuffer(32);
            KBuffer::SPtr updateValue = ToBuffer(33);
            KBuffer::SPtr updateValue2 = ToBuffer(44);

            // Add
            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx1 = CreateWriteTransaction();
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await tx1->CommitAsync();
            }

            co_await CheckpointAsync();
            co_await CloseAndReOpenStoreAsync();

            // Create the snapshot transaction to read from consolidated.
            KArray<WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr> storesTransactions(GetAllocator());

            for (ULONG i = 0; i < Stores->Count(); i++)
            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx = CreateWriteTransaction(*(*Stores)[i]);
                tx->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
                LONG64 vsn;
                Diagnostics::Validate(co_await tx->TransactionSPtr->GetVisibilitySequenceNumberAsync(vsn));
                storesTransactions.Append(tx);
            }

            // Update to differential
            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx2 = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*tx2->StoreTransactionSPtr, key, updateValue, DefaultTimeout, CancellationToken::None);
                co_await tx2->CommitAsync();
            }

            co_await VerifyKeyExistsInStoresAsync(key, MakeBuffer(0), updateValue, EqualityFunction);

            for (ULONG i = 0; i < Stores->Count(); i++)
            {
                // Snapshot read from consolidated.
                co_await VerifyKeyExistsAsync(*storesTransactions[i]->StateProviderSPtr, *storesTransactions[i]->StoreTransactionSPtr, key, MakeBuffer(0), value, EqualityFunction);
                co_await storesTransactions[i]->AbortAsync();
            }

            storesTransactions.Clear();

            // Create the snapshot transaction to read from snapshot component.
            for (ULONG i = 0; i < Stores->Count(); i++)
            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx = CreateWriteTransaction(*(*Stores)[i]);
                tx->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
                LONG64 vsn;
                Diagnostics::Validate(co_await tx->TransactionSPtr->GetVisibilitySequenceNumberAsync(vsn));
                storesTransactions.Append(tx);
            }

            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx2 = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*tx2->StoreTransactionSPtr, key, updateValue2, DefaultTimeout, CancellationToken::None);
                co_await tx2->CommitAsync();
            }

            co_await VerifyKeyExistsInStoresAsync(key, MakeBuffer(0), updateValue2, EqualityFunction);

            // Checkpoint to move the value from consolidated to snapshot component.
            co_await CheckpointAsync();

            for (ULONG i = 0; i < Stores->Count(); i++)
            {
                // Snapshot read from snapshot component.
                co_await VerifyKeyExistsAsync(*storesTransactions[i]->StateProviderSPtr, *storesTransactions[i]->StoreTransactionSPtr, key, MakeBuffer(0), updateValue, EqualityFunction);
                co_await storesTransactions[i]->AbortAsync();
            }

            storesTransactions.Clear();
            co_return;
        }

        ktl::Awaitable<void> SnapshotRead_WithMultipleAppliesAndCheckpoint_ShouldSucceed_Test()
        {
            KString::SPtr key = ToString(1);
            KBuffer::SPtr value = ToBuffer(32);
            int updateValue = 33;

            // Add
            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx1 = CreateWriteTransaction();
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await tx1->CommitAsync();
            }

            // Start the snapshot transaction here
            KArray<WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr> storesTransactions(GetAllocator());

            for (ULONG i = 0; i < Stores->Count(); i++)
            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx = CreateWriteTransaction(*(*Stores)[i]);
                tx->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
                storesTransactions.Append(tx);
                co_await VerifyKeyExistsAsync(*(*Stores)[i], *tx->StoreTransactionSPtr, key, MakeBuffer(0), value, EqualityFunction);
            }

            // Update values mutiple times and will move first version to snapshot container.
            for (int i = 0; i < 10; i++)
            {
                {
                    WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx2 = CreateWriteTransaction();
                    co_await Store->ConditionalUpdateAsync(*tx2->StoreTransactionSPtr, key, ToBuffer(++updateValue), DefaultTimeout, CancellationToken::None);
                    co_await tx2->CommitAsync();
                }
            }

            //Move latest to consolidated.
            co_await CheckpointAsync();

            co_await VerifyKeyExistsInStoresAsync(key, MakeBuffer(0), ToBuffer(updateValue), EqualityFunction);

            for (ULONG i = 0; i < storesTransactions.Count(); i++)
            {
                co_await VerifyKeyExistsAsync(*storesTransactions[i]->StateProviderSPtr, *storesTransactions[i]->StoreTransactionSPtr, key, MakeBuffer(0), value, EqualityFunction);
                co_await storesTransactions[i]->AbortAsync();
            }

            storesTransactions.Clear();
            co_return;
        }

        ktl::Awaitable<void> SnapshotRead_VerifyCloseReopenWithMergeAndSnapShotReads_ShouldSucceed_Test()
        {
            KString::SPtr key = ToString(1);
            KBuffer::SPtr value = ToBuffer(32);
            KBuffer::SPtr updatedValue = ToBuffer(33);

            for (ULONG i = 0; i < Stores->Count(); i++)
            {
               auto store = (*Stores)[i];
               store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;
            }

            // Add
            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx1 = CreateWriteTransaction();
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await tx1->CommitAsync();
            }

            co_await CheckpointAsync();
            co_await CloseAndReOpenStoreAsync();

            for (ULONG i = 0; i < Stores->Count(); i++)
            {
               auto store = (*Stores)[i];
               store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;
            }

            // Start the snapshot transaction here, snapshot read from consolidated.
            KArray<WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr> storesTransactions(GetAllocator());

            for (ULONG i = 0; i < Stores->Count(); i++)
            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx = CreateWriteTransaction(*(*Stores)[i]);
                tx->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
                storesTransactions.Append(tx);
                co_await VerifyKeyExistsAsync(*(*Stores)[i], *tx->StoreTransactionSPtr, key, MakeBuffer(0), value, EqualityFunction);
            }

            // Update the value in differential.
            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx2 = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*tx2->StoreTransactionSPtr, key, updatedValue, DefaultTimeout, CancellationToken::None);
                co_await tx2->CommitAsync();
            }

            //Consolidate and move from consolidated to snapshot container.
            co_await CheckpointAsync();

            co_await VerifyKeyExistsInStoresAsync(key, MakeBuffer(0), updatedValue, EqualityFunction);

            //Snapshot read from snapshot container.
            for (ULONG i = 0; i < storesTransactions.Count(); i++)
            {
                co_await VerifyKeyExistsAsync(*storesTransactions[i]->StateProviderSPtr, *storesTransactions[i]->StoreTransactionSPtr, key, MakeBuffer(0), value, EqualityFunction);
                int count = storesTransactions[i]->StateProviderSPtr->SnapshotContainerSPtr->GetCount();
                CODING_ERROR_ASSERT(count == 1);
                co_await storesTransactions[i]->AbortAsync();
            }

            storesTransactions.Clear();
            co_return;
        }

        ktl::Awaitable<void> SnapshotRead_VerifyCloseReopenWithMergeThenSnapShotAndConsolidatedReads_ShouldSucceed_Test()
        {
            KString::SPtr key = ToString(1);
            KBuffer::SPtr value = ToBuffer(32);
            KBuffer::SPtr updatedValue1 = ToBuffer(33);
            KBuffer::SPtr updatedValue2 = ToBuffer(34);

            // Add
            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx1 = CreateWriteTransaction();
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await tx1->CommitAsync();
            }

            co_await CheckpointAsync();
            co_await CloseAndReOpenStoreAsync();

            // Update the value in differential.
            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx2 = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*tx2->StoreTransactionSPtr, key, updatedValue1, DefaultTimeout, CancellationToken::None);
                co_await tx2->CommitAsync();
            }

            // Start the snapshot transaction here, snapshot read from consolidated.
            KArray<WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr> storesTransactions(GetAllocator());

            for (ULONG i = 0; i < Stores->Count(); i++)
            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx = CreateWriteTransaction(*(*Stores)[i]);
                tx->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
                storesTransactions.Append(tx);
                co_await VerifyKeyExistsAsync(*(*Stores)[i], *tx->StoreTransactionSPtr, key, MakeBuffer(0), updatedValue1, EqualityFunction);
            }

            // Update the value in differential (previous version, current version)
            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx2 = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*tx2->StoreTransactionSPtr, key, updatedValue2, DefaultTimeout, CancellationToken::None);
                co_await tx2->CommitAsync();
            }

            //Move previous in differential to snapshot container during consolidation, current version in consolidated.
            co_await CheckpointAsync();

            co_await VerifyKeyExistsInStoresAsync(key, MakeBuffer(0), updatedValue2, EqualityFunction);

            //Snapshot read from snapshot container.
            for (ULONG i = 0; i < storesTransactions.Count(); i++)
            {
                co_await VerifyKeyExistsAsync(*storesTransactions[i]->StateProviderSPtr, *storesTransactions[i]->StoreTransactionSPtr, key, MakeBuffer(0), updatedValue1, EqualityFunction);
                int count = storesTransactions[i]->StateProviderSPtr->SnapshotContainerSPtr->GetCount();
                CODING_ERROR_ASSERT(count == 1);
                co_await storesTransactions[i]->AbortAsync();
            }

            storesTransactions.Clear();
            co_return;
        }

        ktl::Awaitable<void> SnapshotRead_WithKeysDeletedAndCheckpointed_ShouldReadSuccessfully_Test()
        {
            KString::SPtr key1 = ToString(5);
            KString::SPtr key2 = ToString(6);
            KString::SPtr key3 = ToString(7);
            KString::SPtr key4 = ToString(8);
            KBuffer::SPtr value = ToBuffer(32);

            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            // Add key1 and key2
            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx1 = CreateWriteTransaction();
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key1, value, DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key2, value, DefaultTimeout, CancellationToken::None);
                co_await tx1->CommitAsync();
            }

            co_await CheckpointAsync();

            // Add key3 and key4
            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx1 = CreateWriteTransaction();
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key3, value, DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key4, value, DefaultTimeout, CancellationToken::None);
                co_await tx1->CommitAsync();
            }

            // Start the snapshot transaction here
            KArray<WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr> storesTransactions(GetAllocator());

            for (ULONG i = 0; i < Stores->Count(); i++)
            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx = CreateWriteTransaction(*(*Stores)[i]);
                tx->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
                storesTransactions.Append(tx);
                //Key1 and key 2 should be in consolidated and key3 key4 should be in differential.
                co_await VerifyKeyExistsAsync(*(*Stores)[i], *tx->StoreTransactionSPtr, key1, MakeBuffer(0), value, EqualityFunction);
                co_await VerifyKeyExistsAsync(*(*Stores)[i], *tx->StoreTransactionSPtr, key2, MakeBuffer(0), value, EqualityFunction);
                co_await VerifyKeyExistsAsync(*(*Stores)[i], *tx->StoreTransactionSPtr, key3, MakeBuffer(0), value, EqualityFunction);
                co_await VerifyKeyExistsAsync(*(*Stores)[i], *tx->StoreTransactionSPtr, key4, MakeBuffer(0), value, EqualityFunction);
            }

            // Remove key1 and key3.
            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx2 = CreateWriteTransaction();
                bool success = co_await Store->ConditionalRemoveAsync(*tx2->StoreTransactionSPtr, key1, DefaultTimeout, CancellationToken::None);
                CODING_ERROR_ASSERT(success);
                success = co_await Store->ConditionalRemoveAsync(*tx2->StoreTransactionSPtr, key3, DefaultTimeout, CancellationToken::None);
                CODING_ERROR_ASSERT(success);
                co_await tx2->CommitAsync();
            }

            //Consolidate and will move key1 and key3 original value to snapshot container. key2 key 4 will move from diff to consolidated
            co_await CheckpointAsync();

            co_await VerifyKeyDoesNotExistInStoresAsync(key1);
            co_await VerifyKeyDoesNotExistInStoresAsync(key3);
            co_await VerifyKeyExistsInStoresAsync(key2, MakeBuffer(0), value, EqualityFunction);
            co_await VerifyKeyExistsInStoresAsync(key4, MakeBuffer(0), value, EqualityFunction);

            //Snapshot read from snapshot container.
            for (ULONG i = 0; i < storesTransactions.Count(); i++)
            {
                co_await VerifyKeyExistsAsync(*storesTransactions[i]->StateProviderSPtr, *storesTransactions[i]->StoreTransactionSPtr, key1, MakeBuffer(0), value, EqualityFunction);
                co_await VerifyKeyExistsAsync(*storesTransactions[i]->StateProviderSPtr, *storesTransactions[i]->StoreTransactionSPtr, key2, MakeBuffer(0), value, EqualityFunction);
                co_await VerifyKeyExistsAsync(*storesTransactions[i]->StateProviderSPtr, *storesTransactions[i]->StoreTransactionSPtr, key3, MakeBuffer(0), value, EqualityFunction);
                co_await VerifyKeyExistsAsync(*storesTransactions[i]->StateProviderSPtr, *storesTransactions[i]->StoreTransactionSPtr, key4, MakeBuffer(0), value, EqualityFunction);
                co_await storesTransactions[i]->AbortAsync();
            }

            storesTransactions.Clear();
            co_return;
        }

        ktl::Awaitable<void> Enumeration_Snapshot_Test()
        {
            KString::SPtr key1 = ToString(5);
            KString::SPtr key2 = ToString(6);
            KString::SPtr key3 = ToString(7);
            KString::SPtr key4 = ToString(8);
            KString::SPtr key5 = ToString(9);

            KBuffer::SPtr value1 = ToBuffer(32);
            KBuffer::SPtr value2 = ToBuffer(33);
            KBuffer::SPtr value2_updated = ToBuffer(43);
            KBuffer::SPtr value3 = ToBuffer(34);
            KBuffer::SPtr value4 = ToBuffer(35);
            KBuffer::SPtr value5 = ToBuffer(36);

            KArray<KeyValuePair<KString::SPtr, KBuffer::SPtr>> pairs(GetAllocator());
            KeyValuePair<KString::SPtr, KBuffer::SPtr> pair1(key1, value1);
            KeyValuePair<KString::SPtr, KBuffer::SPtr> pair2(key2, value2);
            KeyValuePair<KString::SPtr, KBuffer::SPtr> pair3(key3, value3);
            KeyValuePair<KString::SPtr, KBuffer::SPtr> pair4(key4, value4);
            KeyValuePair<KString::SPtr, KBuffer::SPtr> pair5(key5, value5);
            pairs.Append(pair1);
            pairs.Append(pair2);
            pairs.Append(pair3);
            pairs.Append(pair4);

            // Add keys
            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx1 = CreateWriteTransaction();
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key1, value1, DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key2, value2, DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key3, value3, DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key4, value4, DefaultTimeout, CancellationToken::None);
                co_await tx1->CommitAsync();
            }

            {
                // Create snapshot enumeration reads.
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx1 = CreateWriteTransaction();
                tx1->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
                auto enumerator1 = co_await Store->CreateEnumeratorAsync(*tx1->StoreTransactionSPtr);
 
                for (ULONG i = 0; i < pairs.Count(); i++)
                {
                    bool success = co_await enumerator1->MoveNextAsync(CancellationToken::None);
                    CODING_ERROR_ASSERT(success);
                    CODING_ERROR_ASSERT(enumerator1->GetCurrent().Key == pairs[i].Key);
                    CODING_ERROR_ASSERT(enumerator1->GetCurrent().Value.Value == pairs[i].Value);
                }

                bool success = co_await enumerator1->MoveNextAsync(CancellationToken::None);
                CODING_ERROR_ASSERT(!success);

                // Create another transaction updates the values.
                {
                    WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx2 = CreateWriteTransaction();
                    success = co_await Store->ConditionalRemoveAsync(*tx2->StoreTransactionSPtr, key1, DefaultTimeout, CancellationToken::None);
                    CODING_ERROR_ASSERT(success);
                    CODING_ERROR_ASSERT(success);
                    success = co_await Store->ConditionalUpdateAsync(*tx2->StoreTransactionSPtr, key2, value2_updated, DefaultTimeout, CancellationToken::None);
                    CODING_ERROR_ASSERT(success);
                    co_await Store->AddAsync(*tx2->StoreTransactionSPtr, key5, value5, DefaultTimeout, CancellationToken::None);
                    co_await tx2->CommitAsync();
                }

                // Snapshot read should read the same values before the update.
                auto enumerator2 = co_await Store->CreateEnumeratorAsync(*tx1->StoreTransactionSPtr);
                for (ULONG i = 0; i < pairs.Count(); i++)
                {
                    success = co_await enumerator2->MoveNextAsync(CancellationToken::None);
                    CODING_ERROR_ASSERT(success);
                    CODING_ERROR_ASSERT(enumerator2->GetCurrent().Key == pairs[i].Key);
                    CODING_ERROR_ASSERT(enumerator2->GetCurrent().Value.Value == pairs[i].Value);
                }

                success = co_await enumerator2->MoveNextAsync(CancellationToken::None);
                CODING_ERROR_ASSERT(!success);

                co_await tx1->AbortAsync();
            }

            // Read the current value and verify the values are updated.
            pairs[1].Value = value2_updated;
            pairs.Append(pair5);
            pairs.Remove(0);

            {
                WriteTransaction<KString::SPtr, KBuffer::SPtr>::SPtr tx1 = CreateWriteTransaction();
                tx1->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
                auto enumerator3 = co_await Store->CreateEnumeratorAsync(*tx1->StoreTransactionSPtr);
                for (ULONG i = 0; i < pairs.Count(); i++)
                {
                    bool success = co_await enumerator3->MoveNextAsync(CancellationToken::None);
                    CODING_ERROR_ASSERT(success);
                    CODING_ERROR_ASSERT(enumerator3->GetCurrent().Key == pairs[i].Key);
                    CODING_ERROR_ASSERT(enumerator3->GetCurrent().Value.Value == pairs[i].Value);
                }

                bool success = co_await enumerator3->MoveNextAsync(CancellationToken::None);
                CODING_ERROR_ASSERT(!success);

                co_await tx1->AbortAsync();
            }
            co_return;
        }

        ktl::Awaitable<void> NotifyAll_CRUD_ShouldSucceed_Test()
        {
            const ULONG32 bufferSize = 100 * sizeof(ULONG);
            auto keyComparerSPtr = Store->KeyComparerSPtr;

            KBufferComparer::SPtr bufferComparerSPtr = nullptr;
            auto status = KBufferComparer::Create(GetAllocator(), bufferComparerSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            IComparer<KBuffer::SPtr>::SPtr valueComparerSPtr = static_cast<IComparer<KBuffer::SPtr> *>(bufferComparerSPtr.RawPtr());
            CODING_ERROR_ASSERT(valueComparerSPtr != nullptr);

            TestDictionaryChangeHandler<KString::SPtr, KBuffer::SPtr>::SPtr testHandlerSPtr = nullptr;
            TestDictionaryChangeHandler<KString::SPtr, KBuffer::SPtr>::Create(*keyComparerSPtr, *valueComparerSPtr, GetAllocator(), testHandlerSPtr);
            IDictionaryChangeHandler<KString::SPtr, KBuffer::SPtr>::SPtr handlerSPtr = static_cast<IDictionaryChangeHandler<KString::SPtr, KBuffer::SPtr> *>(testHandlerSPtr.RawPtr());

            Store->DictionaryChangeHandlerSPtr = handlerSPtr;

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, ToString(1), MakeBuffer(bufferSize, 1), DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, ToString(2), MakeBuffer(bufferSize, 2), DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, ToString(3), MakeBuffer(bufferSize, 3), DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();

                testHandlerSPtr->AddToExpected(StoreModificationType::Add, ToString(1), MakeBuffer(bufferSize, 1), txn->TransactionSPtr->CommitSequenceNumber);
                testHandlerSPtr->AddToExpected(StoreModificationType::Add, ToString(2), MakeBuffer(bufferSize, 2), txn->TransactionSPtr->CommitSequenceNumber);
                testHandlerSPtr->AddToExpected(StoreModificationType::Add, ToString(3), MakeBuffer(bufferSize, 3), txn->TransactionSPtr->CommitSequenceNumber);

                testHandlerSPtr->AddToExpectedRebuild(ToString(1), MakeBuffer(bufferSize, 1), txn->TransactionSPtr->CommitSequenceNumber);
            }

            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, ToString(2), MakeBuffer(bufferSize, 22), DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();

                testHandlerSPtr->AddToExpected(StoreModificationType::Update, ToString(2), MakeBuffer(bufferSize, 22), txn->TransactionSPtr->CommitSequenceNumber);
                testHandlerSPtr->AddToExpectedRebuild(ToString(2), MakeBuffer(bufferSize, 22), txn->TransactionSPtr->CommitSequenceNumber);
            }

            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, ToString(3), DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();

                testHandlerSPtr->AddToExpected(StoreModificationType::Remove, ToString(3), nullptr, txn->TransactionSPtr->CommitSequenceNumber);
            }


            testHandlerSPtr->Validate();

            co_await CheckpointAsync();
            co_await CloseAndReOpenStoreAsync(handlerSPtr);
            co_return;
        }

        ktl::Awaitable<void> VerifyCount_SingleAdd_ShouldSucceed_Test()
       {
           auto key = ToString(7);
           auto value = ToBuffer(12);

           {
               auto txn = CreateWriteTransaction();
               co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
               co_await txn->CommitAsync();
           }
        
           auto expectedCount = 1;
           VerifyCountInAllStores(expectedCount);
           co_return;
       }

        ktl::Awaitable<void> VerifyCount_SingleAddRemove_ShouldSucceed_Test()
       {
           auto key = ToString(7);
           auto value = ToBuffer(12);

           {
               auto txn = CreateWriteTransaction();
               co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
               co_await txn->CommitAsync();
           }

           {
               auto txn = CreateWriteTransaction();
               co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None);
               co_await txn->CommitAsync();
           }
        
           auto expectedCount = 0;
           VerifyCountInAllStores(expectedCount);
           co_return;
       }

        ktl::Awaitable<void> VerifyCount_SingleAddUpdate_ShouldSucceed_Test()
       {
           auto key = ToString(7);
           auto value = ToBuffer(12);
           auto update = ToBuffer(14);

           {
               auto txn = CreateWriteTransaction();
               co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
               co_await txn->CommitAsync();
           }

           {
               auto txn = CreateWriteTransaction();
               co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, update, DefaultTimeout, CancellationToken::None);
               co_await txn->CommitAsync();
           }

           auto expectedCount = 1;
           VerifyCountInAllStores(expectedCount);
           co_return;
       }

        ktl::Awaitable<void> VerifyCount_SingleAddUpdateRemove_ShouldSucceed_Test()
       {
           auto key = ToString(7);
           auto value = ToBuffer(12);

           {
               auto txn = CreateWriteTransaction();
               co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
               co_await txn->CommitAsync();
           }

           {
               auto txn = CreateWriteTransaction();
               co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
               co_await txn->CommitAsync();
           }

           {
               auto txn = CreateWriteTransaction();
               co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None);
               co_await txn->CommitAsync();
           }

           auto expectedCount = 0;
           VerifyCountInAllStores(expectedCount);
           co_return;
       }

        ktl::Awaitable<void> VerifyCount_AddUpdateRemoveMany_ShouldSucceed_Test()
       {
           const int numItems = 64;

           for (int i = 1; i <= numItems; i++)
           {
               auto key = i;
               auto value = i;

               {
                   auto txn = CreateWriteTransaction();
                   co_await Store->AddAsync(*txn->StoreTransactionSPtr, ToString(key), ToBuffer(value), DefaultTimeout, CancellationToken::None);
                   co_await txn->CommitAsync();
                   VerifyCountInAllStores(i);
               }

               {
                   auto txn = CreateWriteTransaction();
                   co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, ToString(key), ToBuffer(value), DefaultTimeout, CancellationToken::None);
                   co_await txn->CommitAsync();
                   VerifyCountInAllStores(i);
               }

               {
                   auto txn = CreateWriteTransaction();
                   co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, ToString(key), DefaultTimeout, CancellationToken::None);
                   co_await txn->CommitAsync();
                   VerifyCountInAllStores(i-1);
               }

               {
                   auto txn = CreateWriteTransaction();
                   co_await Store->AddAsync(*txn->StoreTransactionSPtr, ToString(key), ToBuffer(value), DefaultTimeout, CancellationToken::None);
                   co_await txn->CommitAsync();
                   VerifyCountInAllStores(i);
               }
           }
           co_return;
       }

        ktl::Awaitable<void> VerifyCount_CorrectAfterRecovery_ShouldSucceed_Test()
       {
           const int numItems = 64;

           LONG64 expectedCount = 0;

           for (int i = 1; i <= numItems; i++)
           {
               auto key = i;
               auto value = i;

               // Add - increases expected count
               {
                   auto txn = CreateWriteTransaction();
                   co_await Store->AddAsync(*txn->StoreTransactionSPtr, ToString(key), ToBuffer(value), DefaultTimeout, CancellationToken::None);
                   co_await txn->CommitAsync();
                   expectedCount++;
               }

               if (i % 3 == 1)
               {
                   // Update - expected count doesn't change
                   {
                       auto txn = CreateWriteTransaction();
                       co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, ToString(key), ToBuffer(value+1), DefaultTimeout, CancellationToken::None);
                       co_await txn->CommitAsync();
                   }
               }

               else if (i % 3 == 2)
               {
                   // Remove - expected count decreases
                   {
                       auto txn = CreateWriteTransaction();
                       co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, ToString(key), DefaultTimeout, CancellationToken::None);
                       co_await txn->CommitAsync();
                       expectedCount--;
                   }
               }

               VerifyCountInAllStores(expectedCount);
           }

           co_await CheckpointAsync();
           co_await CloseAndReOpenStoreAsync();

           VerifyCountInAllStores(expectedCount);
           co_return;
       }

        ktl::Awaitable<void> VerifyCount_AfterFalseProgressUndo_ShouldSucceed_Test()
       {
           const int numInitialItems = 40;

           for (int i = 0; i < numInitialItems; i++)
           {
               auto key = ToString(i);
               auto value = ToBuffer(i);

               {
                   auto txn = CreateWriteTransaction();
                   co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                   co_await txn->CommitAsync();
               }
           }

           VerifyCountInAllStores(numInitialItems);
        
           {
               auto txn = CreateWriteTransaction();

               // keys 0-7 untouched
               // keys 8-15 updated
               // keys 16-23 updated/removed
               // keys 24-31 removed
               // keys 32-39 removed/added
               // keys 40-48 added

               for (int i = 8; i < 24; i++)
               {
                   co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, ToString(i), ToBuffer(i + 1), DefaultTimeout, CancellationToken::None);
               }

               for (int i = 16; i < 40; i++)
               {
                   co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, ToString(i), DefaultTimeout, CancellationToken::None);
               }

               for (int i = 32; i < 48; i++)
               {
                   co_await Store->AddAsync(*txn->StoreTransactionSPtr, ToString(i), ToBuffer(i+2), DefaultTimeout, CancellationToken::None);
               }

               co_await Replicator->SimulateFalseProgressAsync(*txn->TransactionSPtr);

               VerifyCountInAllStores(numInitialItems);

               co_await txn->AbortAsync();
           }

           VerifyCountInAllStores(numInitialItems);
           co_return;
       }

        ktl::Awaitable<void> CloseStore_WithOutstandingTransaction_ShouldSucceed_Test()
       {
           {
               auto txn = CreateWriteTransaction();
               co_await Store->AddAsync(*txn->StoreTransactionSPtr, ToString(5), ToBuffer(5), DefaultTimeout, CancellationToken::None);

               co_await CloseAndReOpenStoreAsync();
           }
           co_return;
       }

        ktl::Awaitable<void> CommitTransaction_AfterLockManagerClosed_ShouldFail_Test()
       {
           auto txn = CreateWriteTransaction();

           try
           {
               co_await Store->AddAsync(*txn->StoreTransactionSPtr, ToString(2), ToBuffer(1), DefaultTimeout, CancellationToken::None);
           
               co_await Store->LockManagerSPtr->CloseAsync();
               Store->LockManagerSPtr = nullptr;
            
               txn->Dispose(); // Need to call dispose manually since destructors are not allowed to throw
           }
           catch (ktl::Exception const & e)
           {
               CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_OBJECT_CLOSED);
           }

           txn = nullptr;

           // Workaround so that store cleanup will work
           LockManager::SPtr lockManagerSPtr = nullptr;
           LockManager::Create(GetAllocator(), lockManagerSPtr);
           co_await lockManagerSPtr->OpenAsync();
           Store->LockManagerSPtr = lockManagerSPtr;

           lockManagerSPtr = nullptr;
           co_return;
       }
    #pragma endregion
    };

    BOOST_FIXTURE_TEST_SUITE(StringBufferStoreTestSuite, StringBufferStoreTest)

    BOOST_AUTO_TEST_CASE(Add_SingleTransaction_ShouldSucceed)
    {
        SyncAwait(Add_SingleTransaction_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddUpdate_SingleTransaction_ShouldSucceed)
    {
        SyncAwait(AddUpdate_SingleTransaction_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddDelete_SingleTransaction_ShouldSucceed)
    {
        SyncAwait(AddDelete_SingleTransaction_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddDeleteAdd_SingleTransaction_ShouldSucceed)
    {
        SyncAwait(AddDeleteAdd_SingleTransaction_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(MultipleAdds_SingleTransaction_ShouldSucceed)
    {
        SyncAwait(MultipleAdds_SingleTransaction_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(MultipleUpdates_SingleTransaction_ShouldSucceed)
    {
        SyncAwait(MultipleUpdates_SingleTransaction_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddUpdate_DifferentTransactions_ShouldSucceed)
    {
        SyncAwait(AddUpdate_DifferentTransactions_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddDelete_DifferentTransactions_ShouldSucceed)
    {
        SyncAwait(AddDelete_DifferentTransactions_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddUpdateRead_DifferentTransactions_ShouldSucceed)
    {
        SyncAwait(AddUpdateRead_DifferentTransactions_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(MultipleAdds_MultipleTransactions_ShouldSucceed)
    {
        SyncAwait(MultipleAdds_MultipleTransactions_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(MultipleAddsUpdates_MultipleTransactions_ShouldSucceed)
    {
        SyncAwait(MultipleAddsUpdates_MultipleTransactions_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddSameKey_MultipleTransactions_ShouldFail)
    {
        SyncAwait(AddSameKey_MultipleTransactions_ShouldFail_Test());
    }

    BOOST_AUTO_TEST_CASE(Add_Abort_ShouldSucceed)
    {
        SyncAwait(Add_Abort_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(UndoFalseProgress_UndoAdd_ShouldSucceed)
    {
        SyncAwait(UndoFalseProgress_UndoAdd_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(UndoFalseProgress_UndoUpdate_ShouldSucceed)
    {
        SyncAwait(UndoFalseProgress_UndoUpdate_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(UndoFalseProgress_UndoRemove_ShouldSucceed)
    {
        SyncAwait(UndoFalseProgress_UndoRemove_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(UndoFalseProgress_UndoAddUpdateRemove_ShouldSucceed)
    {
        SyncAwait(UndoFalseProgress_UndoAddUpdateRemove_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(UndoFalseProgress_UndoCRUD_ShouldSucceed)
    {
        SyncAwait(UndoFalseProgress_UndoCRUD_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(UndoFalseProgress_UndoUpdateWithCheckpoint_ShouldSucceed)
    {
        SyncAwait(UndoFalseProgress_UndoUpdateWithCheckpoint_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(UndoFalseProgress_UndoRemoveWithCheckpoint_ShouldSucceed)
    {
        SyncAwait(UndoFalseProgress_UndoRemoveWithCheckpoint_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(UndoFalseProgress_UndoCRUDwithCheckpoint__ShouldSucceed)
    {
        SyncAwait(UndoFalseProgress_UndoCRUDwithCheckpoint__ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(UndoFalseProgress_SameKeyMutipleOperationsSingleTransaction_ShouldSucceed)
    {
        SyncAwait(UndoFalseProgress_SameKeyMutipleOperationsSingleTransaction_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(SnapshotRead_FromDifferentialStoreAfterOneUpdate_ShouldSucceed)
    {
        SyncAwait(SnapshotRead_FromDifferentialStoreAfterOneUpdate_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(SnapshotRead_DuringCheckpoint_ShouldSucceed)
    {
        SyncAwait(SnapshotRead_DuringCheckpoint_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(SnapshotRead_GetValueOnDisk_FromConsolidatedComponentBeforeCompleteCheckpoint_ShouldSucceed)
    {
        SyncAwait(SnapshotRead_GetValueOnDisk_FromConsolidatedComponentBeforeCompleteCheckpoint_ShouldSucceed_Test());
    }
    
    BOOST_AUTO_TEST_CASE(Snapshot_GetValueOnDisk_FromConsolidatedAndSnapshotComponent_ShouldSucceed)
    {
        SyncAwait(Snapshot_GetValueOnDisk_FromConsolidatedAndSnapshotComponent_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(SnapshotRead_WithMultipleAppliesAndCheckpoint_ShouldSucceed)
    {
        SyncAwait(SnapshotRead_WithMultipleAppliesAndCheckpoint_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(SnapshotRead_VerifyCloseReopenWithMergeAndSnapShotReads_ShouldSucceed)
    {
        SyncAwait(SnapshotRead_VerifyCloseReopenWithMergeAndSnapShotReads_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(SnapshotRead_VerifyCloseReopenWithMergeThenSnapShotAndConsolidatedReads_ShouldSucceed)
    {
        SyncAwait(SnapshotRead_VerifyCloseReopenWithMergeThenSnapShotAndConsolidatedReads_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(SnapshotRead_WithKeysDeletedAndCheckpointed_ShouldReadSuccessfully)
    {
        SyncAwait(SnapshotRead_WithKeysDeletedAndCheckpointed_ShouldReadSuccessfully_Test());
    }

    BOOST_AUTO_TEST_CASE(Enumeration_Snapshot)
    {
        SyncAwait(Enumeration_Snapshot_Test());
    }

    BOOST_AUTO_TEST_CASE(NotifyAll_CRUD_ShouldSucceed)
    {
        SyncAwait(NotifyAll_CRUD_ShouldSucceed_Test());
    }

   BOOST_AUTO_TEST_CASE(VerifyCount_SingleAdd_ShouldSucceed)
   {
       SyncAwait(VerifyCount_SingleAdd_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(VerifyCount_SingleAddRemove_ShouldSucceed)
   {
       SyncAwait(VerifyCount_SingleAddRemove_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(VerifyCount_SingleAddUpdate_ShouldSucceed)
   {
       SyncAwait(VerifyCount_SingleAddUpdate_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(VerifyCount_SingleAddUpdateRemove_ShouldSucceed)
   {
       SyncAwait(VerifyCount_SingleAddUpdateRemove_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(VerifyCount_AddUpdateRemoveMany_ShouldSucceed)
   {
       SyncAwait(VerifyCount_AddUpdateRemoveMany_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(VerifyCount_CorrectAfterRecovery_ShouldSucceed)
   {
       SyncAwait(VerifyCount_CorrectAfterRecovery_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(VerifyCount_AfterFalseProgressUndo_ShouldSucceed)
   {
       SyncAwait(VerifyCount_AfterFalseProgressUndo_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(CloseStore_WithOutstandingTransaction_ShouldSucceed)
   {
       SyncAwait(CloseStore_WithOutstandingTransaction_ShouldSucceed_Test());
   }

   BOOST_AUTO_TEST_CASE(CommitTransaction_AfterLockManagerClosed_ShouldFail)
   {
       SyncAwait(CommitTransaction_AfterLockManagerClosed_ShouldFail_Test());
   }

    BOOST_AUTO_TEST_SUITE_END()
}
