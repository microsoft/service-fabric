// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#define SKIPLISTPERFTEST_TAG 'plSC'
#define PERF_TRACE "Perf"

namespace TStoreTests
{
    using namespace ktl;
    using namespace Data;
    using namespace Data::Utilities;

    template<typename TKey, typename TValue>
    class DataStructuresPerfBase
    {
    public:
        void Setup()
        {
            NTSTATUS status;
            status = KtlSystem::Initialize(FALSE, &ktlSystem_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            ktlSystem_->SetStrictAllocationChecks(TRUE);
        }

        void Cleanup()
        {
            ktlSystem_->Shutdown();
        }

        KAllocator& GetAllocator()
        {
            return ktlSystem_->NonPagedAllocator();
        }

        KString::SPtr CreateString(ULONG seed)
        {
            KString::SPtr key;

            wstring zeroString = wstring(L"0");
            wstring seedString = to_wstring(seed);
            while (seedString.length() < 9) // zero pad the number to make it easier to compare
            {
                seedString = zeroString + seedString;
            }

            wstring str = wstring(L"test_key_") + seedString;
            auto status = KString::Create(key, GetAllocator(), str.c_str());
            Diagnostics::Validate(status);
            return key;
        }

        KBuffer::SPtr CreateBuffer(__in ULONG32 sizeInBytes, __in ULONG fillValue)
        {
            CODING_ERROR_ASSERT(sizeInBytes % sizeof(ULONG) == 0);
            KBuffer::SPtr resultSPtr = nullptr;
            KBuffer::Create(sizeInBytes, resultSPtr, this->GetAllocator());

            ULONG* buffer = static_cast<ULONG *>(resultSPtr->GetBuffer());

            auto numElements = sizeInBytes / sizeof(ULONG);
            for (ULONG i = 0; i < numElements; i++)
            {
                buffer[i] = fillValue;
            }

            return resultSPtr;
        }

#pragma region CreateComparer overloads
        void CreateComparer(__out KSharedPtr<IComparer<LONG64>> & comparer)
        {
            LongComparer::SPtr longComparer = nullptr;
            LongComparer::Create(GetAllocator(), longComparer);
            comparer = static_cast<IComparer<LONG64> *>(longComparer.RawPtr());
        }

        void CreateComparer(__out KSharedPtr<IComparer<KString::SPtr>> & comparer)
        {
            KStringComparer::SPtr stringComparer = nullptr;
            KStringComparer::Create(GetAllocator(), stringComparer);
            comparer = static_cast<IComparer<KString::SPtr> *>(stringComparer.RawPtr());
        }

        void CreateComparer(__out KSharedPtr<IComparer<SharedLong::SPtr>> & comparer)
        {
            IComparer<SharedLong::SPtr>::SPtr sharedLongComparer = SharedLongComparer::Create(GetAllocator());
            comparer = sharedLongComparer;
        }

#pragma endregion

#pragma region CreateKey overloads
        void CreateKey(__in ULONG index, __out LONG64 & key)
        {
            key = static_cast<LONG64>(index);
        }

        void CreateKey(__in ULONG index, __out KString::SPtr & key)
        {
            key = CreateString(index);
        }

        void CreateKey(__in ULONG index, __out SharedLong::SPtr & key)
        {
            key = SharedLong::Create(static_cast<LONG64>(index), GetAllocator());
        }
#pragma endregion

#pragma region CreateValue overloads
        void CreateValue(__in ULONG index, __out byte & value)
        {
            value = static_cast<byte>(index & 0xff);
        }

        void CreateValue(ULONG index, __out KBuffer::SPtr & value)
        {
            value = CreateBuffer(500, index);
        }
#pragma endregion

#pragma region ConcurrentSkipList Tests
        ktl::Awaitable<void> AddKeysAsync(
            __in ConcurrentSkipList<TKey, TValue> & skipList,
            __in KSharedArray<TKey> & keys,
            __in KSharedArray<TValue> & values,
            __in ULONG offset,
            __in ULONG count)
        {
            co_await CorHelper::ThreadPoolThread(ktlSystem_->DefaultThreadPool()); // Switch to background thread

            for (ULONG i = offset; i < offset + count; i++)
            {
                TKey key = keys[i];
                TValue value = values[i];
                skipList.TryAdd(key, value);
            }
        }

        ktl::Awaitable<void> ReadKeyValuesAsync(
            __in ConcurrentSkipList<TKey, TValue> & skipList,
            __in KSharedArray<TKey> & keys,
            __in ULONG offset,
            __in ULONG count)
        {
            co_await CorHelper::ThreadPoolThread(ktlSystem_->DefaultThreadPool()); // Switch to background thread

            for (ULONG i = offset; i < offset + count; i++)
            {
                TKey key = keys[i];
                TValue value;
                skipList.TryGetValue(key, value);
            }
        }

        void ConcurrentSkipListPerf_SequentialAddReadTest(__in ULONG numKeys)
        {
            KSharedPtr<IComparer<TKey>> comparer = nullptr;
            CreateComparer(comparer);

            KSharedPtr<ConcurrentSkipList<TKey, TValue>> listSPtr = nullptr;
            ConcurrentSkipList<TKey, TValue>::Create(comparer, GetAllocator(), listSPtr);

            Common::Stopwatch stopwatch;

            for (ULONG i = 0; i < numKeys; i++)
            {
                TKey key;
                TValue value;
                CreateKey(i, key);
                CreateValue(i, value);

                stopwatch.Start();
                listSPtr->TryAdd(key, value);
                stopwatch.Stop();
            }

            Trace.WriteInfo(
                PERF_TRACE,
                "ConcurrentSkipList_SequentialTest Add {0} keys: {1} ms",
                numKeys, stopwatch.ElapsedMilliseconds);

            stopwatch.Reset();

            for (ULONG i = 0; i < numKeys; i++)
            {
                TKey key;
                TValue value;
                CreateKey(i, key);

                stopwatch.Start();
                listSPtr->TryGetValue(key, value);
                stopwatch.Stop();
            }

            Trace.WriteInfo(
                PERF_TRACE,
                "ConcurrentSkipList_SequentialTest Read {0} keys: {1} ms",
                numKeys, stopwatch.ElapsedMilliseconds);
        }

        void ConcurrentSkipList_ConcurrentAddThenReadTest(__in ULONG numKeys, __in ULONG numTasks)
        {
            KSharedPtr<KSharedArray<TKey>> keys = _new(SKIPLISTPERFTEST_TAG, GetAllocator()) KSharedArray<TKey>();
            KSharedPtr<KSharedArray<TValue>> values = _new(SKIPLISTPERFTEST_TAG, GetAllocator()) KSharedArray<TValue>();

            for (ULONG i = 0; i < numKeys; i++)
            {
                TKey key;
                TValue value;

                CreateKey(i, key);
                CreateValue(i, value);

                keys->Append(key);
                values->Append(value);
            }

            KSharedPtr<IComparer<TKey>> comparer = nullptr;
            CreateComparer(comparer);

            KSharedPtr<ConcurrentSkipList<TKey, TValue>> listSPtr = nullptr;
            ConcurrentSkipList<TKey, TValue>::Create(comparer, GetAllocator(), listSPtr);

            KSharedArray<ktl::Awaitable<void>>::SPtr tasks = _new(SKIPLISTPERFTEST_TAG, GetAllocator()) KSharedArray<ktl::Awaitable<void>>();
            ULONG numKeysPerTask = numKeys / numTasks;

            Common::Stopwatch stopwatch;

            stopwatch.Start();
            for (ULONG offset = 0; offset < numKeys; offset += numKeysPerTask)
            {
                tasks->Append(AddKeysAsync(*listSPtr, *keys, *values, offset, numKeysPerTask));
            }

            SyncAwait(StoreUtilities::WhenAll<void>(*tasks, GetAllocator()));
            stopwatch.Stop();

            Trace.WriteInfo(
                PERF_TRACE,
                "ConcurrentSkipList_LongByte_ConcurrentAddThenReadTest Add {0} keys with {1} tasks: {2} ms",
                numKeys, numTasks, stopwatch.ElapsedMilliseconds);

            tasks->Clear();
            stopwatch.Reset();

            stopwatch.Start();
            for (ULONG offset = 0; offset < numKeys; offset += numKeysPerTask)
            {
                tasks->Append(ReadKeyValuesAsync(*listSPtr, *keys, offset, numKeysPerTask));
            }

            SyncAwait(StoreUtilities::WhenAll<void>(*tasks, GetAllocator()));
            stopwatch.Stop();

            Trace.WriteInfo(
                PERF_TRACE,
                "ConcurrentSkipList_LongByte_ConcurrentAddThenReadTest Read {0} keys with {1} tasks: {2} ms",
                numKeys, numTasks, stopwatch.ElapsedMilliseconds);
        }

        ktl::Awaitable<void> ReadSingleKey(
            __in ConcurrentSkipList<TKey, TValue> & list,
            __in TKey key,
            __in ULONG numReads)
        {
            co_await ktl::CorHelper::ThreadPoolThread(ktlSystem_->DefaultThreadPool());

            TValue value;
            for (ULONG32 i = 0; i < numReads; i++)
            {
                list.TryGetValue(key, value);
            }
        }

        void ConcurrentSkipList_SingleKeyReadPerfTest(__in ULONG numReads, __in ULONG numTasks)
        {
            KSharedPtr<IComparer<TKey>> comparer = nullptr;
            CreateComparer(comparer);

            KSharedPtr<ConcurrentSkipList<TKey, TValue>> listSPtr = nullptr;
            ConcurrentSkipList<TKey, TValue>::Create(comparer, GetAllocator(), listSPtr);

            TKey key;
            TValue value;

            CreateKey(8, key);
            CreateValue(8, value);
            listSPtr->TryAdd(key, value);

            KSharedArray<ktl::Awaitable<void>>::SPtr tasks = _new(SKIPLISTPERFTEST_TAG, GetAllocator()) KSharedArray<ktl::Awaitable<void>>();
            ULONG numReadsPerTask = numReads / numTasks;

            Common::Stopwatch stopwatch;

            stopwatch.Start();
            for (ULONG i = 0; i < numTasks; i++)
            {
                tasks->Append(ReadSingleKey(*listSPtr, key, numReadsPerTask));
            }

            SyncAwait(StoreUtilities::WhenAll<void>(*tasks, GetAllocator()));
            stopwatch.Stop();

            Trace.WriteInfo(
                PERF_TRACE,
                "ConcurrentSkipList_SingleKeyReadTest Read single key {0} times with {1} tasks: {2} ms",
                numReads, numTasks, stopwatch.ElapsedMilliseconds);
        }

#pragma endregion

#pragma region FastSkipList Tests
        ktl::Awaitable<void> AddKeysAsync(
            __in FastSkipList<TKey, TValue> & skipList,
            __in KSharedArray<TKey> & keys,
            __in KSharedArray<TValue> & values,
            __in ULONG offset,
            __in ULONG count)
        {
            co_await CorHelper::ThreadPoolThread(ktlSystem_->DefaultThreadPool()); // Switch to background thread

            for (ULONG i = offset; i < offset + count; i++)
            {
                TKey key = keys[i];
                TValue value = values[i];
                skipList.TryAdd(key, value);
            }
        }

        ktl::Awaitable<void> ReadKeyValuesAsync(
            __in FastSkipList<TKey, TValue> & skipList,
            __in KSharedArray<TKey> & keys,
            __in ULONG offset,
            __in ULONG count)
        {
            co_await CorHelper::ThreadPoolThread(ktlSystem_->DefaultThreadPool()); // Switch to background thread

            for (ULONG i = offset; i < offset + count; i++)
            {
                TKey key = keys[i];
                TValue value;
                skipList.TryGetValue(key, value);
            }
        }

        void FastSkipListPerf_SequentialAddReadTest(__in ULONG numKeys)
        {
            KSharedPtr<IComparer<TKey>> comparer = nullptr;
            CreateComparer(comparer);

            KSharedPtr<FastSkipList<TKey, TValue>> listSPtr = nullptr;
            FastSkipList<TKey, TValue>::Create(comparer, GetAllocator(), listSPtr);

            Common::Stopwatch stopwatch;

            for (ULONG i = 0; i < numKeys; i++)
            {
                TKey key;
                TValue value;
                CreateKey(i, key);
                CreateValue(i, value);

                stopwatch.Start();
                listSPtr->TryAdd(key, value);
                stopwatch.Stop();
            }

            Trace.WriteInfo(
                PERF_TRACE,
                "FastSkipList_SequentialTest Add {0} keys: {1} ms",
                numKeys, stopwatch.ElapsedMilliseconds);

            stopwatch.Reset();

            for (ULONG i = 0; i < numKeys; i++)
            {
                TKey key;
                TValue value;
                CreateKey(i, key);

                stopwatch.Start();
                listSPtr->TryGetValue(key, value);
                stopwatch.Stop();
            }

            Trace.WriteInfo(
                PERF_TRACE,
                "FastSkipList_SequentialTest Read {0} keys: {1} ms",
                numKeys, stopwatch.ElapsedMilliseconds);
        }

        void FastSkipList_ConcurrentAddThenReadTest(__in ULONG numKeys, __in ULONG numTasks)
        {
            KSharedPtr<KSharedArray<TKey>> keys = _new(SKIPLISTPERFTEST_TAG, GetAllocator()) KSharedArray<TKey>();
            KSharedPtr<KSharedArray<TValue>> values = _new(SKIPLISTPERFTEST_TAG, GetAllocator()) KSharedArray<TValue>();

            for (ULONG i = 0; i < numKeys; i++)
            {
                TKey key;
                TValue value;

                CreateKey(i, key);
                CreateValue(i, value);

                keys->Append(key);
                values->Append(value);
            }

            KSharedPtr<IComparer<TKey>> comparer = nullptr;
            CreateComparer(comparer);

            KSharedPtr<FastSkipList<TKey, TValue>> listSPtr = nullptr;
            FastSkipList<TKey, TValue>::Create(comparer, GetAllocator(), listSPtr);

            KSharedArray<ktl::Awaitable<void>>::SPtr tasks = _new(SKIPLISTPERFTEST_TAG, GetAllocator()) KSharedArray<ktl::Awaitable<void>>();
            ULONG numKeysPerTask = numKeys / numTasks;

            Common::Stopwatch stopwatch;

            stopwatch.Start();
            for (ULONG offset = 0; offset < numKeys; offset += numKeysPerTask)
            {
                tasks->Append(AddKeysAsync(*listSPtr, *keys, *values, offset, numKeysPerTask));
            }

            SyncAwait(StoreUtilities::WhenAll<void>(*tasks, GetAllocator()));
            stopwatch.Stop();

            Trace.WriteInfo(
                PERF_TRACE,
                "FastSkipList_LongByte_ConcurrentAddThenReadTest Add {0} keys with {1} tasks: {2} ms",
                numKeys, numTasks, stopwatch.ElapsedMilliseconds);

            tasks->Clear();
            stopwatch.Reset();

            stopwatch.Start();
            for (ULONG offset = 0; offset < numKeys; offset += numKeysPerTask)
            {
                tasks->Append(ReadKeyValuesAsync(*listSPtr, *keys, offset, numKeysPerTask));
            }

            SyncAwait(StoreUtilities::WhenAll<void>(*tasks, GetAllocator()));
            stopwatch.Stop();

            Trace.WriteInfo(
                PERF_TRACE,
                "FastSkipList_LongByte_ConcurrentAddThenReadTest Read {0} keys with {1} tasks: {2} ms",
                numKeys, numTasks, stopwatch.ElapsedMilliseconds);
        }

        ktl::Awaitable<void> ReadSingleKey(
            __in FastSkipList<TKey, TValue> & list,
            __in TKey key,
            __in ULONG numReads)
        {
            co_await ktl::CorHelper::ThreadPoolThread(ktlSystem_->DefaultThreadPool());

            TValue value;
            for (ULONG32 i = 0; i < numReads; i++)
            {
                list.TryGetValue(key, value);
            }
        }

        void FastSkipList_SingleKeyReadPerfTest(__in ULONG numReads, __in ULONG numTasks)
        {
            KSharedPtr<IComparer<TKey>> comparer = nullptr;
            CreateComparer(comparer);

            KSharedPtr<FastSkipList<TKey, TValue>> listSPtr = nullptr;
            FastSkipList<TKey, TValue>::Create(comparer, GetAllocator(), listSPtr);

            TKey key;
            TValue value;

            CreateKey(8, key);
            CreateValue(8, value);
            listSPtr->TryAdd(key, value);

            KSharedArray<ktl::Awaitable<void>>::SPtr tasks = _new(SKIPLISTPERFTEST_TAG, GetAllocator()) KSharedArray<ktl::Awaitable<void>>();
            ULONG numReadsPerTask = numReads / numTasks;

            Common::Stopwatch stopwatch;

            stopwatch.Start();
            for (ULONG i = 0; i < numTasks; i++)
            {
                tasks->Append(ReadSingleKey(*listSPtr, key, numReadsPerTask));
            }

            SyncAwait(StoreUtilities::WhenAll<void>(*tasks, GetAllocator()));
            stopwatch.Stop();

            Trace.WriteInfo(
                PERF_TRACE,
                "FastSkipList_SingleKeyReadTest Read single key {0} times with {1} tasks: {2} ms",
                numReads, numTasks, stopwatch.ElapsedMilliseconds);
        }

#pragma endregion

#pragma region PartitionSortedList Test
        ktl::Awaitable<void> ReadKeyValuesAsync(
            __in PartitionedSortedList<TKey, TValue> & list,
            __in KSharedArray<TKey> & keys,
            __in ULONG offset,
            __in ULONG count)
        {
            co_await CorHelper::ThreadPoolThread(ktlSystem_->DefaultThreadPool()); // Switch to background thread

            for (ULONG i = offset; i < offset + count; i++)
            {
                TKey key = keys[i];
                TValue value;
                list.TryGetValue(key, value);
            }
        }

        ktl::Awaitable<void> ReadSingleKey(
            __in PartitionedSortedList<TKey, TValue> & list,
            __in TKey key,
            __in ULONG numReads)
        {
            co_await ktl::CorHelper::ThreadPoolThread(ktlSystem_->DefaultThreadPool());

            TValue value;
            for (ULONG32 i = 0; i < numReads; i++)
            {
                list.TryGetValue(key, value);
            }
        }

        void PartitionSortedList_SequentialAddReadTest(__in ULONG32 numKeys)
        {
            KSharedPtr<IComparer<TKey>> comparer = nullptr;
            CreateComparer(comparer);

            KSharedPtr<PartitionedSortedList<TKey, TValue>> listSPtr = nullptr;
            PartitionedSortedList<TKey, TValue>::Create(*comparer, GetAllocator(), listSPtr);

            Common::Stopwatch stopwatch;

            for (ULONG i = 0; i < numKeys; i++)
            {
                TKey key;
                TValue value;
                CreateKey(i, key);
                CreateValue(i, value);

                stopwatch.Start();
                listSPtr->Add(key, value);
                stopwatch.Stop();
            }

            Trace.WriteInfo(
                PERF_TRACE,
                "PartitionSortedList_SequentialAddReadTest Add {0} keys: {1} ms",
                numKeys, stopwatch.ElapsedMilliseconds);

            stopwatch.Reset();

            for (ULONG i = 0; i < numKeys; i++)
            {
                TKey key;
                TValue value;
                CreateKey(i, key);

                stopwatch.Start();
                listSPtr->TryGetValue(key, value);
                stopwatch.Stop();
            }

            Trace.WriteInfo(
                PERF_TRACE,
                "PartitionSortedList_SequentialAddReadTest Read {0} keys: {1} ms",
                numKeys, stopwatch.ElapsedMilliseconds);
        }

        void PartitionedSortedList_ConcurrentReadTest(__in ULONG numKeys, __in ULONG numTasks)
        {
            KSharedPtr<IComparer<TKey>> comparer = nullptr;
            CreateComparer(comparer);

            KSharedPtr<PartitionedSortedList<TKey, TValue>> listSPtr = nullptr;
            PartitionedSortedList<TKey, TValue>::Create(*comparer, GetAllocator(), listSPtr);

            KSharedPtr<KSharedArray<TKey>> keys = _new(SKIPLISTPERFTEST_TAG, GetAllocator()) KSharedArray<TKey>();
            KSharedPtr<KSharedArray<TValue>> values = _new(SKIPLISTPERFTEST_TAG, GetAllocator()) KSharedArray<TValue>();

            for (ULONG i = 0; i < numKeys; i++)
            {
                TKey key;
                TValue value;

                CreateKey(i, key);
                CreateValue(i, value);

                keys->Append(key);
                values->Append(value);

                listSPtr->Add(key, value);
            }

            KSharedArray<ktl::Awaitable<void>>::SPtr tasks = _new(SKIPLISTPERFTEST_TAG, GetAllocator()) KSharedArray<ktl::Awaitable<void>>();
            ULONG numKeysPerTask = numKeys / numTasks;

            Common::Stopwatch stopwatch;
            stopwatch.Start();

            for (ULONG offset = 0; offset < numKeys; offset += numKeysPerTask)
            {
                tasks->Append(ReadKeyValuesAsync(*listSPtr, *keys, offset, numKeysPerTask));
            }

            SyncAwait(StoreUtilities::WhenAll<void>(*tasks, GetAllocator()));
            stopwatch.Stop();

            Trace.WriteInfo(
                PERF_TRACE,
                "PartitionedSortedListTest_ConcurrentReadTest Read {0} keys with {1} tasks: {2} ms",
                numKeys, numTasks, stopwatch.ElapsedMilliseconds);
        }

        void PartitionedSortedList_SingleKeyReadPerfTest(__in ULONG numReads, __in ULONG numTasks)
        {
            KSharedPtr<IComparer<TKey>> comparer = nullptr;
            CreateComparer(comparer);

            KSharedPtr<PartitionedSortedList<TKey, TValue>> listSPtr = nullptr;
            PartitionedSortedList<TKey, TValue>::Create(*comparer, GetAllocator(), listSPtr);

            TKey key;
            TValue value;

            CreateKey(8, key);
            CreateValue(8, value);
            listSPtr->Add(key, value);

            KSharedArray<ktl::Awaitable<void>>::SPtr tasks = _new(SKIPLISTPERFTEST_TAG, GetAllocator()) KSharedArray<ktl::Awaitable<void>>();
            ULONG numReadsPerTask = numReads / numTasks;

            Common::Stopwatch stopwatch;

            stopwatch.Start();
            for (ULONG i = 0; i < numTasks; i++)
            {
                tasks->Append(ReadSingleKey(*listSPtr, key, numReadsPerTask));
            }

            SyncAwait(StoreUtilities::WhenAll<void>(*tasks, GetAllocator()));
            stopwatch.Stop();

            Trace.WriteInfo(
                PERF_TRACE,
                "PartitionedSortedList_SingleKeyReadTest Read single key {0} times with {1} tasks: {2} ms",
                numReads, numTasks, stopwatch.ElapsedMilliseconds);
        }

#pragma endregion

#pragma region ConcurrentHashTable Test
        ktl::Awaitable<void> AddKeysAsync(
            __in ConcurrentHashTable<TKey, TValue> & hashTable,
            __in KSharedArray<TKey> & keys,
            __in KSharedArray<TValue> & values,
            __in ULONG offset,
            __in ULONG count)
        {
            co_await CorHelper::ThreadPoolThread(ktlSystem_->DefaultThreadPool()); // Switch to background thread

            for (ULONG i = offset; i < offset + count; i++)
            {
                TKey key = keys[i];
                TValue value = values[i];
                hashTable.TryAdd(key, value);
            }
        }

        ktl::Awaitable<void> ReadKeyValuesAsync(
            __in ConcurrentHashTable<TKey, TValue> & hashTable,
            __in KSharedArray<TKey> & keys,
            __in ULONG offset,
            __in ULONG count)
        {
            co_await CorHelper::ThreadPoolThread(ktlSystem_->DefaultThreadPool()); // Switch to background thread

            for (ULONG i = offset; i < offset + count; i++)
            {
                TKey key = keys[i];
                TValue value;
                hashTable.TryGetValue(key, value);
            }
        }

        void ConcurrentHashTablePerf_SequentialAddReadTest(__in ULONG numKeys)
        {
            KSharedPtr<ConcurrentHashTable<TKey, TValue>> hashTableSPtr = nullptr;
            ConcurrentHashTable<TKey, TValue>::Create(K_DefaultHashFunction, 16, GetAllocator(), hashTableSPtr);

            Common::Stopwatch stopwatch;

            for (ULONG i = 0; i < numKeys; i++)
            {
                TKey key;
                TValue value;
                CreateKey(i, key);
                CreateValue(i, value);

                stopwatch.Start();
                hashTableSPtr->TryAdd(key, value);
                stopwatch.Stop();
            }

            Trace.WriteInfo(
                PERF_TRACE,
                "ConcurrentHashTable_SequentialTest Add {0} keys: {1} ms",
                numKeys, stopwatch.ElapsedMilliseconds);

            stopwatch.Reset();

            for (ULONG i = 0; i < numKeys; i++)
            {
                TKey key;
                TValue value;
                CreateKey(i, key);

                stopwatch.Start();
                hashTableSPtr->TryGetValue(key, value);
                stopwatch.Stop();
            }

            Trace.WriteInfo(
                PERF_TRACE,
                "ConcurrentHashTable_SequentialTest Read {0} keys: {1} ms",
                numKeys, stopwatch.ElapsedMilliseconds);
        }

        void ConcurrentHashTable_ConcurrentAddThenReadTest(__in ULONG numKeys, __in ULONG numTasks)
        {
            KSharedPtr<KSharedArray<TKey>> keys = _new(SKIPLISTPERFTEST_TAG, GetAllocator()) KSharedArray<TKey>();
            KSharedPtr<KSharedArray<TValue>> values = _new(SKIPLISTPERFTEST_TAG, GetAllocator()) KSharedArray<TValue>();

            for (ULONG i = 0; i < numKeys; i++)
            {
                TKey key;
                TValue value;

                CreateKey(i, key);
                CreateValue(i, value);

                keys->Append(key);
                values->Append(value);
            }

            KSharedPtr<ConcurrentHashTable<TKey, TValue>> hashTableSPtr = nullptr;
            ConcurrentHashTable<TKey, TValue>::Create(K_DefaultHashFunction, 16, GetAllocator(), hashTableSPtr);

            KSharedArray<ktl::Awaitable<void>>::SPtr tasks = _new(SKIPLISTPERFTEST_TAG, GetAllocator()) KSharedArray<ktl::Awaitable<void>>();
            ULONG numKeysPerTask = numKeys / numTasks;

            Common::Stopwatch stopwatch;

            stopwatch.Start();
            for (ULONG offset = 0; offset < numKeys; offset += numKeysPerTask)
            {
                tasks->Append(AddKeysAsync(*hashTableSPtr, *keys, *values, offset, numKeysPerTask));
            }

            SyncAwait(StoreUtilities::WhenAll<void>(*tasks, GetAllocator()));
            stopwatch.Stop();

            Trace.WriteInfo(
                PERF_TRACE,
                "ConcurrentHashTable_LongByte_ConcurrentAddThenReadTest Add {0} keys with {1} tasks: {2} ms",
                numKeys, numTasks, stopwatch.ElapsedMilliseconds);

            tasks->Clear();
            stopwatch.Reset();

            stopwatch.Start();
            for (ULONG offset = 0; offset < numKeys; offset += numKeysPerTask)
            {
                tasks->Append(ReadKeyValuesAsync(*hashTableSPtr, *keys, offset, numKeysPerTask));
            }

            SyncAwait(StoreUtilities::WhenAll<void>(*tasks, GetAllocator()));
            stopwatch.Stop();

            Trace.WriteInfo(
                PERF_TRACE,
                "ConcurrentHashTable_LongByte_ConcurrentAddThenReadTest Read {0} keys with {1} tasks: {2} ms",
                numKeys, numTasks, stopwatch.ElapsedMilliseconds);
        }

        ktl::Awaitable<void> ReadSingleKey(
            __in ConcurrentHashTable<TKey, TValue> & hashTable,
            __in TKey key,
            __in ULONG numReads)
        {
            co_await ktl::CorHelper::ThreadPoolThread(ktlSystem_->DefaultThreadPool());

            TValue value;
            for (ULONG32 i = 0; i < numReads; i++)
            {
                hashTable.TryGetValue(key, value);
            }
        }

        void ConcurrentHashTable_SingleKeyReadPerfTest(__in ULONG numReads, __in ULONG numTasks)
        {
            KSharedPtr<ConcurrentHashTable<TKey, TValue>> hashTableSPtr = nullptr;
            ConcurrentHashTable<TKey, TValue>::Create(K_DefaultHashFunction, 16, GetAllocator(), hashTableSPtr);

            TKey key;
            TValue value;

            CreateKey(8, key);
            CreateValue(8, value);
            hashTableSPtr->TryAdd(key, value);

            KSharedArray<ktl::Awaitable<void>>::SPtr tasks = _new(SKIPLISTPERFTEST_TAG, GetAllocator()) KSharedArray<ktl::Awaitable<void>>();
            ULONG numReadsPerTask = numReads / numTasks;

            Common::Stopwatch stopwatch;

            stopwatch.Start();
            for (ULONG i = 0; i < numTasks; i++)
            {
                tasks->Append(ReadSingleKey(*hashTableSPtr, key, numReadsPerTask));
            }

            SyncAwait(StoreUtilities::WhenAll<void>(*tasks, GetAllocator()));
            stopwatch.Stop();

            Trace.WriteInfo(
                PERF_TRACE,
                "ConcurrentHashTable_SingleKeyReadTest Read single key {0} times with {1} tasks: {2} ms",
                numReads, numTasks, stopwatch.ElapsedMilliseconds);
        }

#pragma endregion

        ktl::Awaitable<void> EmptyTask()
        {
            // An empty task that can be used to measure KTL overhead
            co_await ktl::CorHelper::ThreadPoolThread(ktlSystem_->DefaultThreadPool());
        }

        void ConcurrencyPerfForTasks(__in ULONG32 numTasks)
        {
            KSharedArray<ktl::Awaitable<void>>::SPtr tasks = _new(SKIPLISTPERFTEST_TAG, GetAllocator()) KSharedArray<ktl::Awaitable<void>>();

            Common::Stopwatch stopwatch;

            stopwatch.Start();
            for (ULONG32 i = 0; i < numTasks; i++)
            {
                tasks->Append(EmptyTask());
            }

            SyncAwait(StoreUtilities::WhenAll<void>(*tasks, GetAllocator()));
            stopwatch.Stop();

            Trace.WriteInfo(
                PERF_TRACE,
                "ConcurrencyPerfForTasks Time to create {0} tasks: {1} us",
                numTasks, stopwatch.ElapsedMicroseconds);
        }

    private:
        KtlSystem* ktlSystem_;

    };
}
