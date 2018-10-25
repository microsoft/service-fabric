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

#pragma region test functions
    public:
        ktl::Awaitable<void> Add_SingleTransaction_ShouldSucceed_Test()
        {
            LONG64 key = 5;
            KString::SPtr value = GenerateStringValue(L"value");

            {
               WriteTransaction<LONG64, KString::SPtr>::SPtr tx = CreateWriteTransaction();
               co_await Store->AddAsync(*tx->StoreTransactionSPtr, key, value, Common::TimeSpan::MaxValue, CancellationToken::None);
               co_await VerifyKeyExistsAsync(*Store, *tx->StoreTransactionSPtr, key, nullptr, value);
               co_await tx->CommitAsync();
            }

            co_await VerifyKeyExistsInStoresAsync(key, nullptr, value, EqualityFunction);
            co_return;
        }

        ktl::Awaitable<void> AddUpdateAbort_DifferentTransaction_ShouldSucceed_Test()
        {
            LONG64 key = 5;

            KString::SPtr value = GenerateStringValue(L"value");
            KString::SPtr updatedValue = GenerateStringValue(L"updated");

            {
                WriteTransaction<LONG64, KString::SPtr>::SPtr addTx = CreateWriteTransaction();
                co_await Store->AddAsync(*addTx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await VerifyKeyExistsAsync(*Store, *addTx->StoreTransactionSPtr, key, nullptr, value);
                co_await addTx->CommitAsync();
            }

            {
                WriteTransaction<LONG64, KString::SPtr>::SPtr updateTx = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*updateTx->StoreTransactionSPtr, key, updatedValue, DefaultTimeout, CancellationToken::None);
                co_await VerifyKeyExistsAsync(*Store, *updateTx->StoreTransactionSPtr, key, nullptr, updatedValue);
                co_await updateTx->AbortAsync();
            }

            co_await VerifyKeyExistsInStoresAsync(key, nullptr, value, EqualityFunction);
            co_return;
        }

        ktl::Awaitable<void> DeleteAddUpdateReadDelete_DifferentTransaction_ShouldSucceed_Test()
        {
            // Repeat twice 
            for (ULONG32 i = 0; i < 2; i++)
            {
                LONG64 key = 17;
                KString::SPtr value = GenerateStringValue(L"value");
                KString::SPtr updatedValue = GenerateStringValue(L"updated");

                {
                    WriteTransaction<LONG64, KString::SPtr>::SPtr tx1 = CreateWriteTransaction();
                    bool result = co_await Store->ConditionalRemoveAsync(*tx1->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None);
                    CODING_ERROR_ASSERT(result == false);
                    co_await tx1->AbortAsync();
                }
            
                {
                    WriteTransaction<LONG64, KString::SPtr>::SPtr tx2 = CreateWriteTransaction();
                    co_await Store->AddAsync(*tx2->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                    co_await VerifyKeyExistsAsync(*Store, *tx2->StoreTransactionSPtr, key, nullptr, value);
                    co_await tx2->CommitAsync();
                }

                {
                    WriteTransaction<LONG64, KString::SPtr>::SPtr tx3 = CreateWriteTransaction();
                    co_await Store->ConditionalUpdateAsync(*tx3->StoreTransactionSPtr, key, updatedValue, DefaultTimeout, CancellationToken::None);
                    co_await VerifyKeyExistsAsync(*Store, *tx3->StoreTransactionSPtr, key, nullptr, updatedValue);
                    co_await tx3->CommitAsync();
                }

                co_await VerifyKeyExistsInStoresAsync(key, nullptr, updatedValue, EqualityFunction);

                {
                    WriteTransaction<LONG64, KString::SPtr>::SPtr tx4 = CreateWriteTransaction();
                    bool result = co_await Store->ConditionalRemoveAsync(*tx4->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None);
                    CODING_ERROR_ASSERT(result == true);
                    co_await tx4->CommitAsync();
                }
            }
            co_return;
        }

        ktl::Awaitable<void> DeleteAddDelete_SameTransaction_ShouldSucceed_Test()
        {
            LONG64 key = 17;
            KString::SPtr value = GenerateStringValue(L"value");

            {
                WriteTransaction<LONG64, KString::SPtr>::SPtr tx = CreateWriteTransaction();

                bool result = co_await Store->ConditionalRemoveAsync(*tx->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None);
                CODING_ERROR_ASSERT(result == false);

                co_await Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await VerifyKeyExistsAsync(*Store, *tx->StoreTransactionSPtr, key, nullptr, value);

                result = co_await Store->ConditionalRemoveAsync(*tx->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None);
                CODING_ERROR_ASSERT(result == true);
                co_await VerifyKeyDoesNotExistAsync(*Store, *tx->StoreTransactionSPtr, key);

                co_await tx->CommitAsync();
            }

            co_await VerifyKeyDoesNotExistInStoresAsync(key);
            co_return;
        }

        ktl::Awaitable<void> AddDeleteAbort_DifferentTransaction_ShouldSucceed_Test()
        {
            LONG64 key = 17;
            KString::SPtr value = GenerateStringValue(L"value");

            {
                WriteTransaction<LONG64, KString::SPtr>::SPtr tx1 = CreateWriteTransaction();
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await VerifyKeyExistsAsync(*Store, *tx1->StoreTransactionSPtr, key, nullptr, value);
                co_await tx1->CommitAsync();
            }

            {
                WriteTransaction<LONG64, KString::SPtr>::SPtr tx2 = CreateWriteTransaction();
                co_await Store->ConditionalRemoveAsync(*tx2->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None);
                co_await VerifyKeyDoesNotExistAsync(*Store, *tx2->StoreTransactionSPtr, key);
                co_await tx2->AbortAsync();
            }

            co_await VerifyKeyExistsInStoresAsync(key, nullptr, value, EqualityFunction);
            co_return;
        }

        ktl::Awaitable<void> AddMultipleUpdates_MultipleTransactions_ShouldSuceed_Test()
        {
            LONG64 key = 17;
            KString::SPtr initialValue = GenerateStringValue(1);

            {
                WriteTransaction<LONG64, KString::SPtr>::SPtr tx = CreateWriteTransaction();
                co_await Store->AddAsync(*tx->StoreTransactionSPtr, key, initialValue, DefaultTimeout, CancellationToken::None);
                co_await tx->CommitAsync();
            }

            KString::SPtr updateValue;
            for (ULONG32 i = 0; i < 10; i++)
            {
                updateValue = GenerateStringValue(i + 1);
                {
                    WriteTransaction<LONG64, KString::SPtr>::SPtr updateTx = CreateWriteTransaction();
                    bool result = co_await Store->ConditionalUpdateAsync(*updateTx->StoreTransactionSPtr, key, updateValue, DefaultTimeout, CancellationToken::None);
                    CODING_ERROR_ASSERT(result == true);
                    co_await updateTx->CommitAsync();
                }
            }

            co_await VerifyKeyExistsInStoresAsync(key, nullptr, updateValue, EqualityFunction);
            co_return;
        }

        ktl::Awaitable<void> NoAdd_Update_ShouldFail_Test()
        {
            LONG64 key = 17;
            KString::SPtr value = GenerateStringValue(L"value");

            {
                WriteTransaction<LONG64, KString::SPtr>::SPtr tx1 = CreateWriteTransaction();
                bool result = co_await Store->ConditionalUpdateAsync(*tx1->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                CODING_ERROR_ASSERT(result == false);
                co_await VerifyKeyDoesNotExistAsync(*Store, *tx1->StoreTransactionSPtr, key);
                co_await tx1->AbortAsync();
            }

            co_await VerifyKeyDoesNotExistInStoresAsync(key);
            co_return;
        }
    #pragma endregion
    };

    BOOST_FIXTURE_TEST_SUITE(LongStringStoreTestSuite, LongStringStoreTest)

    BOOST_AUTO_TEST_CASE(Add_SingleTransaction_ShouldSucceed)
    {
        SyncAwait(Add_SingleTransaction_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddUpdateAbort_DifferentTransaction_ShouldSucceed)
    {
        SyncAwait(AddUpdateAbort_DifferentTransaction_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(DeleteAddUpdateReadDelete_DifferentTransaction_ShouldSucceed)
    {
        SyncAwait(DeleteAddUpdateReadDelete_DifferentTransaction_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(DeleteAddDelete_SameTransaction_ShouldSucceed)
    {
        SyncAwait(DeleteAddDelete_SameTransaction_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddDeleteAbort_DifferentTransaction_ShouldSucceed)
    {
        SyncAwait(AddDeleteAbort_DifferentTransaction_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddMultipleUpdates_MultipleTransactions_ShouldSuceed)
    {
        SyncAwait(AddMultipleUpdates_MultipleTransactions_ShouldSuceed_Test());
    }

    BOOST_AUTO_TEST_CASE(NoAdd_Update_ShouldFail)
    {
        SyncAwait(NoAdd_Update_ShouldFail_Test());
    }

    BOOST_AUTO_TEST_SUITE_END()
}
