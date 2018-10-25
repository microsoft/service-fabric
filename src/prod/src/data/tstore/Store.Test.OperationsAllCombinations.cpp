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
    
    class OperationsAllCombinationsTest
        : public TStoreTestBase<LONG64, KString::SPtr, LongComparer, TestStateSerializer<LONG64>, StringStateSerializer>
    {
    public:
        OperationsAllCombinationsTest()
        {
            Setup(3);
        }

        ~OperationsAllCombinationsTest()
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

        Awaitable<LONG64> AddItemAsync(LONG64 key, KString::SPtr value)
        {
            WriteTransaction<LONG64, KString::SPtr>::SPtr tx = CreateWriteTransaction();
            co_await Store->AddAsync(
                *tx->StoreTransactionSPtr,
                key,
                value,
                DefaultTimeout,
                CancellationToken::None);
            auto version = co_await tx->CommitAsync();
            co_return version;
        }

        Awaitable<LONG64> AddItemShouldFailAsync(LONG64 key, KString::SPtr value)
        {
            bool failed = false;
            WriteTransaction<LONG64, KString::SPtr>::SPtr tx = CreateWriteTransaction();

            try
            {
                co_await Store->AddAsync(
                    *tx->StoreTransactionSPtr,
                    key,
                    value,
                    DefaultTimeout,
                    CancellationToken::None);
            }
            catch (ktl::Exception &)
            {
                failed = true;
            }

            CODING_ERROR_ASSERT(failed);
            auto version = co_await tx->AbortAsync();
            co_return version;
        }

        Awaitable<LONG64> UpdateItemAsync(LONG64 key, KString::SPtr value, LONG64 conditionalVersion = -1)
        {
            WriteTransaction<LONG64, KString::SPtr>::SPtr tx = CreateWriteTransaction();
            bool updated = co_await Store->ConditionalUpdateAsync(
                *tx->StoreTransactionSPtr,
                key,
                value,
                DefaultTimeout,
                CancellationToken::None,
                conditionalVersion);
            CODING_ERROR_ASSERT(updated);
            auto version = co_await tx->CommitAsync();
            co_return version;
        }

        Awaitable<LONG64> UpdateItemShouldFailAsync(LONG64 key, KString::SPtr value, LONG64 conditionalVersion = -1)
        {
            WriteTransaction<LONG64, KString::SPtr>::SPtr tx = CreateWriteTransaction();
            bool updated = co_await Store->ConditionalUpdateAsync(
                *tx->StoreTransactionSPtr,
                key,
                value,
                DefaultTimeout,
                CancellationToken::None,
                conditionalVersion);
            CODING_ERROR_ASSERT(!updated);
            auto version = co_await tx->AbortAsync();
            co_return version;
        }

        Awaitable<LONG64> RemoveItemAsync(LONG64 key, LONG64 conditionalVersion = -1)
        {
            WriteTransaction<LONG64, KString::SPtr>::SPtr tx = CreateWriteTransaction();
            bool removed = co_await Store->ConditionalRemoveAsync(
                *tx->StoreTransactionSPtr,
                key,
                DefaultTimeout,
                CancellationToken::None,
                conditionalVersion);
            CODING_ERROR_ASSERT(removed);
            auto version = co_await tx->CommitAsync();
            co_return version;
        }

        Awaitable<LONG64> RemoveItemShouldFailAsync(LONG64 key, LONG64 conditionalVersion = -1)
        {
            WriteTransaction<LONG64, KString::SPtr>::SPtr tx = CreateWriteTransaction();
            bool removed = co_await Store->ConditionalRemoveAsync(
                *tx->StoreTransactionSPtr,
                key,
                DefaultTimeout,
                CancellationToken::None,
                conditionalVersion);
            CODING_ERROR_ASSERT(!removed);
            auto version = co_await tx->AbortAsync();
            co_return version;
        }

        static bool EqualityFunction(KString::SPtr & one, KString::SPtr & two)
        {
            if (one == nullptr || two == nullptr)
            {
                return one == two;
            }

            return one->Compare(*two) == 0;
        }

        // Load the config object as it's needed for the tracing to work.
        Common::CommonConfig config;

#pragma region test functions
    public:
        ktl::Awaitable<void> Add_ShouldSucceed_Test()
        {
            LONG64 key = 1;
            KString::SPtr value = GenerateStringValue(L"value");

            co_await AddItemAsync(key, value);

            co_await VerifyKeyExistsInStoresAsync(key, nullptr, value, EqualityFunction);
            co_return;
        }

        ktl::Awaitable<void> Update_NonExistentKey_ShouldFail_Test()
        {
            LONG64 key = 1;
            KString::SPtr value = GenerateStringValue(L"value");

            co_await UpdateItemShouldFailAsync(key, value);

            co_await VerifyKeyDoesNotExistAsync(*Store, key);
            co_return;
        }

        ktl::Awaitable<void> Remove_NonExistentKey_ShouldFail_Test()
        {
            LONG64 key = 1;

            co_await RemoveItemShouldFailAsync(key);

            co_await VerifyKeyDoesNotExistAsync(*Store, key);
            co_return;
        }

        ktl::Awaitable<void> AddAdd_ShouldFail_Test()
        {
            LONG64 key = 1;
            KString::SPtr value = GenerateStringValue(L"value");
            KString::SPtr value1 = GenerateStringValue(L"value1");

            co_await AddItemAsync(key, value);
            co_await AddItemShouldFailAsync(key, value1);

            co_await VerifyKeyExistsInStoresAsync(key, nullptr, value, EqualityFunction);
            co_return;
        }

        ktl::Awaitable<void> AddUpdate_ShouldSucceed_Test()
        {
            LONG64 key = 1;
            KString::SPtr value = GenerateStringValue(L"value");
            KString::SPtr value1 = GenerateStringValue(L"value1");

            co_await AddItemAsync(key, value);
            co_await UpdateItemAsync(key, value1);

            co_await VerifyKeyExistsInStoresAsync(key, nullptr, value1, EqualityFunction);
            co_return;
        }

        ktl::Awaitable<void> AddRemove_ShouldSucceed_Test()
        {
            LONG64 key = 1;
            KString::SPtr value = GenerateStringValue(L"value");

            co_await AddItemAsync(key, value);
            co_await RemoveItemAsync(key);

            co_await VerifyKeyDoesNotExistAsync(*Store, key);
            co_return;
        }

        ktl::Awaitable<void> AddRemoveUpdate_ShouldFail_Test()
        {
            LONG64 key = 1;
            KString::SPtr value = GenerateStringValue(L"value");

            co_await AddItemAsync(key, value);
            co_await RemoveItemAsync(key);
            co_await UpdateItemShouldFailAsync(key, value);

            co_await VerifyKeyDoesNotExistAsync(*Store, key);
            co_return;
        }

        ktl::Awaitable<void> AddRemoveAdd_ShouldSucceed_Test()
        {
            LONG64 key = 1;
            KString::SPtr value = GenerateStringValue(L"value");
            KString::SPtr value1 = GenerateStringValue(L"value1");

            co_await AddItemAsync(key, value);
            co_await RemoveItemAsync(key);
            co_await AddItemAsync(key, value1);

            co_await VerifyKeyExistsInStoresAsync(key, nullptr, value1, EqualityFunction);
            co_return;
        }

        ktl::Awaitable<void> AddRemoveAddAdd_ShouldFail_Test()
        {
            LONG64 key = 1;
            KString::SPtr value = GenerateStringValue(L"value");
            KString::SPtr value1 = GenerateStringValue(L"value1");

            co_await AddItemAsync(key, value);
            co_await RemoveItemAsync(key);
            co_await AddItemAsync(key, value1);
            co_await AddItemShouldFailAsync(key, value);

            co_await VerifyKeyExistsInStoresAsync(key, nullptr, value1, EqualityFunction);
            co_return;
        }

        ktl::Awaitable<void> AddRemoveRemove_ShouldFail_Test()
        {
            LONG64 key = 1;
            KString::SPtr value = GenerateStringValue(L"value");

            co_await AddItemAsync(key, value);
            co_await RemoveItemAsync(key);
            co_await RemoveItemShouldFailAsync(key);

            co_await VerifyKeyDoesNotExistAsync(*Store, key);
            co_return;
        }

        ktl::Awaitable<void> AddUpdateRemoveAdd_ShouldSucceed_Test()
        {
            LONG64 key = 1;
            KString::SPtr value = GenerateStringValue(L"value");
            KString::SPtr value1 = GenerateStringValue(L"value1");

            co_await AddItemAsync(key, value);
            co_await UpdateItemAsync(key, value);
            co_await RemoveItemAsync(key);
            co_await AddItemAsync(key, value1);

            co_await VerifyKeyExistsInStoresAsync(key, nullptr, value1, EqualityFunction);
            co_return;
        }

        ktl::Awaitable<void> AddUpdateRemoveRemove_ShouldFail_Test()
        {
            LONG64 key = 1;
            KString::SPtr value = GenerateStringValue(L"value");

            co_await AddItemAsync(key, value);
            co_await UpdateItemAsync(key, value);
            co_await RemoveItemAsync(key);
            co_await RemoveItemShouldFailAsync(key);

            co_await VerifyKeyDoesNotExistAsync(*Store, key);
            co_return;
        }

        ktl::Awaitable<void> AddRemoveAddUpdate_ShouldSucceed_Test()
        {
            LONG64 key = 1;
            KString::SPtr value = GenerateStringValue(L"value");
            KString::SPtr value1 = GenerateStringValue(L"value1");

            co_await AddItemAsync(key, value);
            co_await RemoveItemAsync(key);
            co_await AddItemAsync(key, value);
            co_await UpdateItemAsync(key, value1);

            co_await VerifyKeyExistsInStoresAsync(key, nullptr, value1, EqualityFunction);
            co_return;
        }

        ktl::Awaitable<void> AddRemoveAddUpdateRemove_ShouldSucceed_Test()
        {
            LONG64 key = 1;
            KString::SPtr value = GenerateStringValue(L"value");
            KString::SPtr value1 = GenerateStringValue(L"value1");

            co_await AddItemAsync(key, value);
            co_await RemoveItemAsync(key);
            co_await AddItemAsync(key, value);
            co_await UpdateItemAsync(key, value1);
            co_await RemoveItemAsync(key);

            co_await VerifyKeyDoesNotExistAsync(*Store, key);
            co_return;
        }

        ktl::Awaitable<void> AddUpdateRemoveAddUpdateRemove_ShouldSucceed_Test()
        {
            LONG64 key = 1;
            KString::SPtr value = GenerateStringValue(L"value");
            KString::SPtr value1 = GenerateStringValue(L"value1");

            co_await AddItemAsync(key, value);
            co_await UpdateItemAsync(key, value1);
            co_await RemoveItemAsync(key);
            co_await AddItemAsync(key, value1);
            co_await UpdateItemAsync(key, value);
            co_await RemoveItemAsync(key);

            co_await VerifyKeyDoesNotExistAsync(*Store, key);
            co_return;
        }

        ktl::Awaitable<void> AddUpdateRemoveUpdate_ShouldFail_Test()
        {
            LONG64 key = 1;
            KString::SPtr value = GenerateStringValue(L"value");
            KString::SPtr value1 = GenerateStringValue(L"value1");

            co_await AddItemAsync(key, value);
            co_await UpdateItemAsync(key, value1);
            co_await RemoveItemAsync(key);
            co_await UpdateItemShouldFailAsync(key, value);

            co_await VerifyKeyDoesNotExistAsync(*Store, key);
            co_return;
        }

        ktl::Awaitable<void> Add_ConditionalUpdate_ConditionalRemove_ShouldSucceed_Test()
        {
            LONG64 key = 1;
            KString::SPtr value = GenerateStringValue(L"value");
            KString::SPtr value1 = GenerateStringValue(L"value1");

            auto version1 = co_await AddItemAsync(key, value);
            co_await VerifyKeyExistsInStoresAsync(key, nullptr, value, EqualityFunction);

            auto version2 = co_await UpdateItemAsync(key, value1, version1);
            CODING_ERROR_ASSERT(version1 != version2);
            co_await VerifyKeyExistsInStoresAsync(key, nullptr, value1, EqualityFunction);

            co_await UpdateItemShouldFailAsync(key, value, version1);
            co_await RemoveItemShouldFailAsync(key, version1);
            co_await VerifyKeyExistsInStoresAsync(key, nullptr, value1, EqualityFunction);

            auto version3 = co_await RemoveItemAsync(key, version2);
            CODING_ERROR_ASSERT(version2 != version3);
            co_await VerifyKeyDoesNotExistAsync(*Store, key);

            auto version4 = co_await AddItemAsync(key, value);
            CODING_ERROR_ASSERT(version3 != version4);
            co_await VerifyKeyExistsInStoresAsync(key, nullptr, value, EqualityFunction);
            co_return;
        }

        ktl::Awaitable<void> AddGetUpdateGetRemoveGet_ShouldFail_Test()
        {
            LONG64 version = -1;
            LONG64 key = 1;
            KString::SPtr value = GenerateStringValue(L"value");
            KString::SPtr value1 = GenerateStringValue(L"value1");
            KeyValuePair<LONG64, KString::SPtr> kvpair(-1, GenerateStringValue(L"-1"));

            version = co_await AddItemAsync(key, value);
            {
                WriteTransaction<LONG64, KString::SPtr>::SPtr tx = CreateWriteTransaction();

                bool exists = co_await Store->ConditionalGetAsync(
                    *tx->StoreTransactionSPtr,
                    key,
                    DefaultTimeout,
                    kvpair,
                    CancellationToken::None);
                CODING_ERROR_ASSERT(exists);
                CODING_ERROR_ASSERT(kvpair.Key == version);
                CODING_ERROR_ASSERT(kvpair.Value == value);

                co_await tx->CommitAsync();
            }

            version = co_await UpdateItemAsync(key, value1);
            {
                WriteTransaction<LONG64, KString::SPtr>::SPtr tx = CreateWriteTransaction();

                bool exists = co_await Store->ConditionalGetAsync(
                    *tx->StoreTransactionSPtr,
                    key,
                    DefaultTimeout,
                    kvpair,
                    CancellationToken::None);
                CODING_ERROR_ASSERT(exists);
                CODING_ERROR_ASSERT(kvpair.Key == version);
                CODING_ERROR_ASSERT(kvpair.Value == value1);

                co_await tx->CommitAsync();
            }

            co_await RemoveItemAsync(key);
            {
                WriteTransaction<LONG64, KString::SPtr>::SPtr tx = CreateWriteTransaction();

                bool exists = co_await Store->ConditionalGetAsync(
                    *tx->StoreTransactionSPtr,
                    key,
                    DefaultTimeout,
                    kvpair,
                    CancellationToken::None);
                CODING_ERROR_ASSERT(!exists);

                co_await tx->CommitAsync();
            }
            co_return;
        }
    #pragma endregion
    };

    BOOST_FIXTURE_TEST_SUITE(OperationsAllCombinationsTestSuite, OperationsAllCombinationsTest)

    BOOST_AUTO_TEST_CASE(Add_ShouldSucceed)
    {
        SyncAwait(Add_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Update_NonExistentKey_ShouldFail)
    {
        SyncAwait(Update_NonExistentKey_ShouldFail_Test());
    }

    BOOST_AUTO_TEST_CASE(Remove_NonExistentKey_ShouldFail)
    {
        SyncAwait(Remove_NonExistentKey_ShouldFail_Test());
    }

    BOOST_AUTO_TEST_CASE(AddAdd_ShouldFail)
    {
        SyncAwait(AddAdd_ShouldFail_Test());
    }

    BOOST_AUTO_TEST_CASE(AddUpdate_ShouldSucceed)
    {
        SyncAwait(AddUpdate_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddRemove_ShouldSucceed)
    {
        SyncAwait(AddRemove_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddRemoveUpdate_ShouldFail)
    {
        SyncAwait(AddRemoveUpdate_ShouldFail_Test());
    }

    BOOST_AUTO_TEST_CASE(AddRemoveAdd_ShouldSucceed)
    {
        SyncAwait(AddRemoveAdd_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddRemoveAddAdd_ShouldFail)
    {
        SyncAwait(AddRemoveAddAdd_ShouldFail_Test());
    }

    BOOST_AUTO_TEST_CASE(AddRemoveRemove_ShouldFail)
    {
        SyncAwait(AddRemoveRemove_ShouldFail_Test());
    }

    BOOST_AUTO_TEST_CASE(AddUpdateRemoveAdd_ShouldSucceed)
    {
        SyncAwait(AddUpdateRemoveAdd_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddUpdateRemoveRemove_ShouldFail)
    {
        SyncAwait(AddUpdateRemoveRemove_ShouldFail_Test());
    }

    BOOST_AUTO_TEST_CASE(AddRemoveAddUpdate_ShouldSucceed)
    {
        SyncAwait(AddRemoveAddUpdate_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddRemoveAddUpdateRemove_ShouldSucceed)
    {
        SyncAwait(AddRemoveAddUpdateRemove_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddUpdateRemoveAddUpdateRemove_ShouldSucceed)
    {
        SyncAwait(AddUpdateRemoveAddUpdateRemove_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddUpdateRemoveUpdate_ShouldFail)
    {
        SyncAwait(AddUpdateRemoveUpdate_ShouldFail_Test());
    }

    BOOST_AUTO_TEST_CASE(Add_ConditionalUpdate_ConditionalRemove_ShouldSucceed)
    {
        SyncAwait(Add_ConditionalUpdate_ConditionalRemove_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddGetUpdateGetRemoveGet_ShouldFail)
    {
        SyncAwait(AddGetUpdateGetRemoveGet_ShouldFail_Test());
    }

    BOOST_AUTO_TEST_SUITE_END()
}
