// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

#define ALLOC_TAG 'lsTP'

namespace TStoreTests
{
    using namespace ktl;
    using namespace Data::Utilities;

    class LongStringStoreTest : public TStoreTestBase<LONG64, KString::SPtr, LongComparer, TestStateSerializer<LONG64>, StringStateSerializer>
    {
    public:
        LongStringStoreTest()
        {
            Setup(3);
        }

        ~LongStringStoreTest()
        {
            Cleanup();
        }

        KString::SPtr GenerateStringValue(const WCHAR* str)
        {
            KString::SPtr value;
            auto status = KString::Create(value, GetAllocator(), str);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            return value;
        }

        KString::SPtr GenerateStringValue(ULONG32 seed)
        {
            KString::SPtr value;
            wstring str = wstring(L"test_value") + to_wstring(seed);
            auto status = KString::Create(value, GetAllocator(), str.c_str());
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            return value;
        }

        static bool EqualityFunction(KString::SPtr & one, KString::SPtr & two)
        {
            if (one == nullptr || two == nullptr)
            {
                return one == two;
            }

            return one->Compare(*two) == 0;
        }

        Common::CommonConfig config; // load the config object as it's needed for the tracing to work
    };

    BOOST_FIXTURE_TEST_SUITE(LongStringStoreTestSuite, LongStringStoreTest)

    BOOST_AUTO_TEST_CASE(Add_SingleTransaction_ShouldSucceed)
    {
        LONG64 key = 5;
        KString::SPtr value = GenerateStringValue(L"value");

        {
           WriteTransaction<LONG64, KString::SPtr>::SPtr tx = CreateWriteTransaction();
           SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, key, value, Common::TimeSpan::MaxValue, CancellationToken::None));
           SyncAwait(VerifyKeyExistsAsync(*Store, *tx->StoreTransactionSPtr, key, nullptr, value));
           SyncAwait(tx->CommitAsync());
        }

        SyncAwait(VerifyKeyExistsInStoresAsync(key, nullptr, value, EqualityFunction));
    }

    BOOST_AUTO_TEST_CASE(AddUpdateAbort_DifferentTransaction_ShouldSucceed)
    {
        LONG64 key = 5;

        KString::SPtr value = GenerateStringValue(L"value");
        KString::SPtr updatedValue = GenerateStringValue(L"updated");

        {
            WriteTransaction<LONG64, KString::SPtr>::SPtr addTx = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*addTx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(VerifyKeyExistsAsync(*Store, *addTx->StoreTransactionSPtr, key, nullptr, value));
            SyncAwait(addTx->CommitAsync());
        }

        {
            WriteTransaction<LONG64, KString::SPtr>::SPtr updateTx = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*updateTx->StoreTransactionSPtr, key, updatedValue, DefaultTimeout, CancellationToken::None));
            SyncAwait(VerifyKeyExistsAsync(*Store, *updateTx->StoreTransactionSPtr, key, nullptr, updatedValue));
            SyncAwait(updateTx->AbortAsync());
        }

        SyncAwait(VerifyKeyExistsInStoresAsync(key, nullptr, value, EqualityFunction));
    }

    BOOST_AUTO_TEST_CASE(DeleteAddUpdateReadDelete_DifferentTransaction_ShouldSucceed)
    {
        // Repeat twice 
        for (ULONG32 i = 0; i < 2; i++)
        {
            LONG64 key = 17;
            KString::SPtr value = GenerateStringValue(L"value");
            KString::SPtr updatedValue = GenerateStringValue(L"updated");

            {
                WriteTransaction<LONG64, KString::SPtr>::SPtr tx1 = CreateWriteTransaction();
                bool result = SyncAwait(Store->ConditionalRemoveAsync(*tx1->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
                CODING_ERROR_ASSERT(result == false);
                SyncAwait(tx1->AbortAsync());
            }
            
            {
                WriteTransaction<LONG64, KString::SPtr>::SPtr tx2 = CreateWriteTransaction();
                SyncAwait(Store->AddAsync(*tx2->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
                SyncAwait(VerifyKeyExistsAsync(*Store, *tx2->StoreTransactionSPtr, key, nullptr, value));
                SyncAwait(tx2->CommitAsync());
            }

            {
                WriteTransaction<LONG64, KString::SPtr>::SPtr tx3 = CreateWriteTransaction();
                SyncAwait(Store->ConditionalUpdateAsync(*tx3->StoreTransactionSPtr, key, updatedValue, DefaultTimeout, CancellationToken::None));
                SyncAwait(VerifyKeyExistsAsync(*Store, *tx3->StoreTransactionSPtr, key, nullptr, updatedValue));
                SyncAwait(tx3->CommitAsync());
            }

            SyncAwait(VerifyKeyExistsInStoresAsync(key, nullptr, updatedValue, EqualityFunction));

            {
                WriteTransaction<LONG64, KString::SPtr>::SPtr tx4 = CreateWriteTransaction();
                bool result = SyncAwait(Store->ConditionalRemoveAsync(*tx4->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
                CODING_ERROR_ASSERT(result == true);
                SyncAwait(tx4->CommitAsync());
            }
        }
    }

    BOOST_AUTO_TEST_CASE(DeleteAddDelete_SameTransaction_ShouldSucceed)
    {
        LONG64 key = 17;
        KString::SPtr value = GenerateStringValue(L"value");

        {
            WriteTransaction<LONG64, KString::SPtr>::SPtr tx = CreateWriteTransaction();

            bool result = SyncAwait(Store->ConditionalRemoveAsync(*tx->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
            CODING_ERROR_ASSERT(result == false);

            SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(VerifyKeyExistsAsync(*Store, *tx->StoreTransactionSPtr, key, nullptr, value));

            result = SyncAwait(Store->ConditionalRemoveAsync(*tx->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
            CODING_ERROR_ASSERT(result == true);
            SyncAwait(VerifyKeyDoesNotExistAsync(*Store, *tx->StoreTransactionSPtr, key));

            SyncAwait(tx->CommitAsync());
        }

        SyncAwait(VerifyKeyDoesNotExistInStoresAsync(key));
    }

    BOOST_AUTO_TEST_CASE(AddDeleteAbort_DifferentTransaction_ShouldSucceed)
    {
        LONG64 key = 17;
        KString::SPtr value = GenerateStringValue(L"value");

        {
            WriteTransaction<LONG64, KString::SPtr>::SPtr tx1 = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*tx1->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(VerifyKeyExistsAsync(*Store, *tx1->StoreTransactionSPtr, key, nullptr, value));
            SyncAwait(tx1->CommitAsync());
        }

        {
            WriteTransaction<LONG64, KString::SPtr>::SPtr tx2 = CreateWriteTransaction();
            SyncAwait(Store->ConditionalRemoveAsync(*tx2->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
            SyncAwait(VerifyKeyDoesNotExistAsync(*Store, *tx2->StoreTransactionSPtr, key));
            SyncAwait(tx2->AbortAsync());
        }

        SyncAwait(VerifyKeyExistsInStoresAsync(key, nullptr, value, EqualityFunction));
    }

    BOOST_AUTO_TEST_CASE(AddMultipleUpdates_MultipleTransactions_ShouldSuceed)
    {
        LONG64 key = 17;
        KString::SPtr initialValue = GenerateStringValue(1);

        {
            WriteTransaction<LONG64, KString::SPtr>::SPtr tx = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*tx->StoreTransactionSPtr, key, initialValue, DefaultTimeout, CancellationToken::None));
            SyncAwait(tx->CommitAsync());
        }

        KString::SPtr updateValue;
        for (ULONG32 i = 0; i < 10; i++)
        {
            updateValue = GenerateStringValue(i + 1);
            {
                WriteTransaction<LONG64, KString::SPtr>::SPtr updateTx = CreateWriteTransaction();
                bool result = SyncAwait(Store->ConditionalUpdateAsync(*updateTx->StoreTransactionSPtr, key, updateValue, DefaultTimeout, CancellationToken::None));
                CODING_ERROR_ASSERT(result == true);
                SyncAwait(updateTx->CommitAsync());
            }
        }

        SyncAwait(VerifyKeyExistsInStoresAsync(key, nullptr, updateValue, EqualityFunction));
    }

    BOOST_AUTO_TEST_CASE(NoAdd_Update_ShouldFail)
    {
        LONG64 key = 17;
        KString::SPtr value = GenerateStringValue(L"value");

        {
            WriteTransaction<LONG64, KString::SPtr>::SPtr tx1 = CreateWriteTransaction();
            bool result = SyncAwait(Store->ConditionalUpdateAsync(*tx1->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            CODING_ERROR_ASSERT(result == false);
            SyncAwait(VerifyKeyDoesNotExistAsync(*Store, *tx1->StoreTransactionSPtr, key));
            SyncAwait(tx1->AbortAsync());
        }

        SyncAwait(VerifyKeyDoesNotExistInStoresAsync(key));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
