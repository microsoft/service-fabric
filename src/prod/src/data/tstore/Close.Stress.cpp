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

    class CloseStressTest : public TStoreTestBase<LONG64, KString::SPtr, LongComparer, TestStateSerializer<LONG64>, StringStateSerializer>
    {
    public:
        CloseStressTest()
        {
            Setup(1);
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;
        }

        ~CloseStressTest()
        {
            Directory::Delete(Store->WorkingDirectoryCSPtr->operator LPCWSTR(), true);
            CleanupWithoutClose();
        }

        KString::SPtr CreateString(__in LONG64 index)
        {
            KString::SPtr key;

            wstring zeroString = wstring(L"0");
            wstring seedString = to_wstring(index);
            while (seedString.length() < 9) // zero pad the number to make it easier to compare
            {
                seedString = zeroString + seedString;
            }

            wstring str = wstring(L"test_key_") + seedString;
            auto status = KString::Create(key, GetAllocator(), str.c_str());
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            return key;
        }

        Random GetRandom()
        {
            auto seed = Stopwatch::Now().Ticks;
            Random random(static_cast<int>(seed));
            cout << "Random seed (use this seed to reproduce test failures): " << seed << endl;
            return random;
        }

        Awaitable<void> AddItemsAsync(__in LONG64 count)
        {
            for (LONG64 key = 0; key < count; key++)
            {
                auto value = CreateString(key);
                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }
            }
        }

        Awaitable<void> EnumerateAsync()
        {
            {
                auto txn = CreateWriteTransaction();
                txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
                auto enumerator = co_await Store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr);
                while (co_await enumerator->MoveNextAsync(CancellationToken::None))
                {
                    auto current = enumerator->GetCurrent();
                    auto expectedValue = CreateString(current.Key);
                    auto actualValue = current.Value.Value;
                    CODING_ERROR_ASSERT(StringEquals(expectedValue, actualValue));
                }
            }
        }

        void EnumerateStore(
            __in LONG64 count,
            __in KSharedArray<Awaitable<void>> & tasks)
        {
            for (LONG64 i = 0; i < count; i++)
            {
                tasks.Append(EnumerateAsync());
            }
        }
        
        Awaitable<void> GetItemsAsync(__in LONG64 count)
        {
            for (LONG64 i = 0; i < count; i++)
            {
                {
                    auto txn = CreateWriteTransaction();
                    KeyValuePair<LONG64, KString::SPtr> result(-1, nullptr);
                    co_await Store->ConditionalGetAsync(*txn->StoreTransactionSPtr, i, DefaultTimeout, result, CancellationToken::None);
                    co_await txn->AbortAsync();
                }
            }
        }

        void GetItems(
            __in LONG64 count,
            __in KSharedArray<Awaitable<void>> & tasks)
        {
            for (LONG64 j = 0; j < count; j++)
            {
                tasks.Append(GetItemsAsync(count));
            }
        }

        Awaitable<void> UpdateItemsAsync(
            __in LONG64 id,
            __in LONG64 count)
        {
            for (LONG64 i = 0; i < count; i++)
            {
                auto value = CreateString(i + id);
                {
                    auto txn = CreateWriteTransaction();
                    bool updated = co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, i, value, DefaultTimeout, CancellationToken::None);
                    CODING_ERROR_ASSERT(updated);
                }
            }
        }

        void UpdateItems(
            __in LONG64 count,
            __in KSharedArray<Awaitable<void>> & tasks)
        {
            for (LONG64 j = 0; j < count; j++)
            {
                tasks.Append(UpdateItemsAsync(j, count));
            }
        }

        Awaitable<void> RemoveItemsAsync(__in LONG64 count)
        {
            for (LONG64 i = 0; i < count; i++)
            {
                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, i, DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }
            }
        }

        void RemoveItems(
            __in LONG64 count,
            __in KSharedArray<Awaitable<void>> & tasks)
        {
            for (LONG64 j = 0; j < count; j++)
            {
                tasks.Append(RemoveItemsAsync(count));
            }
        }

        static bool StringEquals(KString::SPtr & one, KString::SPtr & two)
        {
            if (one == nullptr || two == nullptr)
            {
                return one == two;
            }

            return one->Compare(*two) == 0;
        }
    };

    BOOST_FIXTURE_TEST_SUITE(CloseStressTestSuite, CloseStressTest)

    BOOST_AUTO_TEST_CASE(Enumerate_DuringClose_ShouldSucceed)
    {
        LONG64 count = 1'000;
        SyncAwait(AddItemsAsync(count));
        Checkpoint();
        CloseAndReOpenStore();

        KSharedArray<Awaitable<void>>::SPtr tasks = _new(ALLOC_TAG, GetAllocator()) KSharedArray<Awaitable<void>>();

        EnumerateStore(count, *tasks);
        SyncAwait(KTimer::StartTimerAsync(GetAllocator(), ALLOC_TAG, 1000, nullptr, nullptr));
        Awaitable<void> closeTask = Store->CloseAsync(CancellationToken::None);

        try
        {
            for (ULONG i = 0; i < tasks->Count(); i++)
            {
                SyncAwait((*tasks)[i]);
            }
        }
        catch (ktl::Exception const & e)
        {
            bool isClosedException = e.GetStatus() == SF_STATUS_OBJECT_CLOSED;
            bool isTimeoutException = e.GetStatus() == SF_STATUS_TIMEOUT;
            CODING_ERROR_ASSERT(isClosedException || isTimeoutException);
        }
        
        SyncAwait(closeTask);
    }

    BOOST_AUTO_TEST_CASE(Get_DuringClose_ShouldSucceed)
    {
        LONG64 count = 1'000;
        SyncAwait(AddItemsAsync(count));
        Checkpoint();
        CloseAndReOpenStore();

        Random random = GetRandom();

        KSharedArray<Awaitable<void>>::SPtr tasks = _new(ALLOC_TAG, GetAllocator()) KSharedArray<Awaitable<void>>();
        GetItems(count, *tasks);

        ULONG waitTime = static_cast<ULONG>(random.Next(100, 10'000));
        cout << "Waiting " << waitTime << " ms" << endl;
        SyncAwait(KTimer::StartTimerAsync(GetAllocator(), ALLOC_TAG, waitTime, nullptr, nullptr));

        Awaitable<void> closeTask = Store->CloseAsync(CancellationToken::None);

        try
        {
            for (ULONG i = 0; i < tasks->Count(); i++)
            {
                SyncAwait((*tasks)[i]);
            }
        }
        catch (ktl::Exception const & e)
        {
            bool isClosedException = e.GetStatus() == SF_STATUS_OBJECT_CLOSED;
            bool isTimeoutException = e.GetStatus() == SF_STATUS_TIMEOUT;
            CODING_ERROR_ASSERT(isClosedException || isTimeoutException);
        }
        
        SyncAwait(closeTask);
    }

    BOOST_AUTO_TEST_CASE(Update_DuringClose_ShouldSucceed)
    {
        LONG64 count = 1'000;
        SyncAwait(AddItemsAsync(count));
        Checkpoint();
        CloseAndReOpenStore();

        Random random = GetRandom();

        KSharedArray<Awaitable<void>>::SPtr tasks = _new(ALLOC_TAG, GetAllocator()) KSharedArray<Awaitable<void>>();
        UpdateItems(count, *tasks);

        ULONG waitTime = static_cast<ULONG>(random.Next(100, 10'000));
        cout << "Waiting " << waitTime << " ms" << endl;
        SyncAwait(KTimer::StartTimerAsync(GetAllocator(), ALLOC_TAG, waitTime, nullptr, nullptr));

        Awaitable<void> closeTask = Store->CloseAsync(CancellationToken::None);

        try
        {
            for (ULONG i = 0; i < tasks->Count(); i++)
            {
                SyncAwait((*tasks)[i]);
            }
        }
        catch (ktl::Exception const & e)
        {
            bool isClosedException = e.GetStatus() == SF_STATUS_OBJECT_CLOSED;
            bool isTimeoutException = e.GetStatus() == SF_STATUS_TIMEOUT;
            CODING_ERROR_ASSERT(isClosedException || isTimeoutException);
        }
        
        SyncAwait(closeTask);
    }

    BOOST_AUTO_TEST_CASE(Remove_DuringClose_ShouldSucceed)
    {
        LONG64 count = 1'000;
        SyncAwait(AddItemsAsync(count));
        Checkpoint();
        CloseAndReOpenStore();

        Random random = GetRandom();

        KSharedArray<Awaitable<void>>::SPtr tasks = _new(ALLOC_TAG, GetAllocator()) KSharedArray<Awaitable<void>>();
        RemoveItems(count, *tasks);

        ULONG waitTime = static_cast<ULONG>(random.Next(100, 10'000));
        cout << "Waiting " << waitTime << " ms" << endl;
        SyncAwait(KTimer::StartTimerAsync(GetAllocator(), ALLOC_TAG, waitTime, nullptr, nullptr));

        Awaitable<void> closeTask = Store->CloseAsync(CancellationToken::None);

        try
        {
            for (ULONG i = 0; i < tasks->Count(); i++)
            {
                SyncAwait((*tasks)[i]);
            }
        }
        catch (ktl::Exception const & e)
        {
            bool isClosedException = e.GetStatus() == SF_STATUS_OBJECT_CLOSED;
            bool isTimeoutException = e.GetStatus() == SF_STATUS_TIMEOUT;
            CODING_ERROR_ASSERT(isClosedException || isTimeoutException);
        }
        
        SyncAwait(closeTask);
    }

    BOOST_AUTO_TEST_SUITE_END();
}
