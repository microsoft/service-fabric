// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

#define ALLOC_TAG 'cfPT'

namespace TStoreTests
{
    using namespace ktl;
    using namespace Data::Utilities;

    class CheckpointFilePerfTest
    {
    public:
        Common::CommonConfig config; // load the config object as it's needed for the tracing to work

        CheckpointFilePerfTest()
        {
            NTSTATUS status;
            status = KtlSystem::Initialize(FALSE, &ktlSystem_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            ktlSystem_->SetStrictAllocationChecks(TRUE);
        }

        ~CheckpointFilePerfTest()
        {
            ktlSystem_->Shutdown();
        }

        KAllocator& GetAllocator()
        {
            return ktlSystem_->NonPagedAllocator();
        }

        KBuffer::SPtr CreateBuffer(__in ULONG32 sizeInBytes, __in ULONG32 fillValue)
        {
            CODING_ERROR_ASSERT(sizeInBytes % sizeof(ULONG32) == 0);
            KBuffer::SPtr resultSPtr = nullptr;
            KBuffer::Create(sizeInBytes, resultSPtr, GetAllocator());
            
            ULONG32* buffer = static_cast<ULONG32 *>(resultSPtr->GetBuffer());
            
            auto numElements = sizeInBytes / sizeof(ULONG32);
            for (ULONG32 i = 0; i < numElements; i++)
            {
                buffer[i] = fillValue;
            }

            return resultSPtr;
        }

        KString::SPtr CreateFileString(__in KStringView const & name)
        {
            KAllocator& allocator = GetAllocator();
            KString::SPtr fileName;

            WCHAR currentDirectoryPathCharArray[MAX_PATH];
            GetCurrentDirectory(MAX_PATH, currentDirectoryPathCharArray);

#if !defined(PLATFORM_UNIX)
            NTSTATUS status = KString::Create(fileName, allocator, L"\\??\\");
#else
            NTSTATUS status = KString::Create(fileName, allocator, L"");
#endif
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            BOOLEAN concatSuccess = fileName->Concat(currentDirectoryPathCharArray);
            CODING_ERROR_ASSERT(concatSuccess == TRUE);

            concatSuccess = fileName->Concat(Common::Path::GetPathSeparatorWstr().c_str());
            CODING_ERROR_ASSERT(concatSuccess == TRUE);

            concatSuccess = fileName->Concat(name);
            CODING_ERROR_ASSERT(concatSuccess == TRUE);

            return fileName.RawPtr();
        }

        KBufferSerializer::SPtr CreateBufferSerializer()
        {
            KBufferSerializer::SPtr valueSerializerSPtr;
            auto status = KBufferSerializer::Create(GetAllocator(), valueSerializerSPtr);
            KInvariant(NT_SUCCESS(status));

            return valueSerializerSPtr;
        }

        StoreTraceComponent::SPtr CreateTraceComponent()
        {
            KGuid guid;
            guid.CreateNew();
            ::FABRIC_REPLICA_ID replicaId = 1;
            int stateProviderId = 1;

            StoreTraceComponent::SPtr traceComponent = nullptr;
            NTSTATUS status = StoreTraceComponent::Create(guid, replicaId, stateProviderId, GetAllocator(), traceComponent);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            return traceComponent;
        }

        KSharedPtr<IEnumerator<KeyValuePair<KBuffer::SPtr, KSharedPtr<VersionedItem<KBuffer::SPtr>>>>> GetEnumerator(
            __in KSharedArray<KeyValuePair<KSharedPtr<KBuffer>, KSharedPtr<VersionedItem<KSharedPtr<KBuffer>>>>> const & items)
        {
            KSharedPtr<ArrayKeyVersionedItemEnumerator<KBuffer::SPtr, KBuffer::SPtr>> arrayEnumeratorSPtr = nullptr;
            NTSTATUS status = ArrayKeyVersionedItemEnumerator<KBuffer::SPtr, KBuffer::SPtr>::Create(items, GetAllocator(), arrayEnumeratorSPtr);
            KInvariant(NT_SUCCESS(status));

            KSharedPtr<IEnumerator<KeyValuePair<KBuffer::SPtr, KSharedPtr<VersionedItem<KBuffer::SPtr>>>>> enumeratorSPtr =
                static_cast<IEnumerator<KeyValuePair<KBuffer::SPtr, KSharedPtr<VersionedItem<KBuffer::SPtr>>>> *>(arrayEnumeratorSPtr.RawPtr());

            KInvariant(enumeratorSPtr != nullptr);
            return enumeratorSPtr;
        }

        ktl::Awaitable<CheckpointFile::SPtr> CreateCheckpointFileAsync(
            __in KStringView & filepath,
            __in KSharedArray<KeyValuePair<KBuffer::SPtr, VersionedItem<KBuffer::SPtr>::SPtr>>::SPtr const & items,
            __in ULONG32 fileId = 1)
        {
            auto bufferSerializerSPtr = CreateBufferSerializer();
            StorePerformanceCountersSPtr perfCounters = nullptr;
            return CheckpointFile::CreateAsync<KBuffer::SPtr, KBuffer::SPtr>(
                fileId,
                filepath,
                *GetEnumerator(*items),
                *bufferSerializerSPtr,
                *bufferSerializerSPtr,
                1,
                GetAllocator(),
                *CreateTraceComponent(),
                perfCounters,
                true);
        }


        ktl::Awaitable<CheckpointFile::SPtr> OpenCheckpointFile(__in KStringView const & filepath)
        {
            KStringView path = const_cast<KStringView &>(filepath);
            return CheckpointFile::OpenAsync(
                path,
                *CreateTraceComponent(),
                GetAllocator(),
                true);
        }

        void CleanupCheckpointFile(__in CheckpointFile & checkpointFile)
        {
            SyncAwait(checkpointFile.CloseAsync());
            auto keyCheckpointNameSPtr = checkpointFile.KeyCheckpointFileNameSPtr;
            auto valueCheckpointNameSPtr = checkpointFile.ValueCheckpointFileNameSPtr;
            Common::File::Delete(checkpointFile.KeyCheckpointFileNameSPtr->operator LPCWSTR());
            Common::File::Delete(checkpointFile.ValueCheckpointFileNameSPtr->operator LPCWSTR());
        }

        VersionedItem<KBuffer::SPtr>::SPtr CreateInsertedVersionedItem(
            __in KBuffer & buffer,
            __in LONG64 versionSequenceNumber)
        {
            InsertedVersionedItem<KBuffer::SPtr>::SPtr itemSPtr = nullptr;
            NTSTATUS status = InsertedVersionedItem<KBuffer::SPtr>::Create(GetAllocator(), itemSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            KBuffer::SPtr bufferSPtr = &buffer;
            itemSPtr->InitializeOnApply(versionSequenceNumber, bufferSPtr);
            itemSPtr->SetValueSize(bufferSPtr->QuerySize());

            return static_cast<VersionedItem<KBuffer::SPtr> *>(itemSPtr.RawPtr());
        }

        KeyValuePair<KBuffer::SPtr, VersionedItem<KBuffer::SPtr>::SPtr> MakeKeyValuePair(__in KBuffer & key, __in KBuffer & value, __in LONG64 sequenceNumber)
        {
            KBuffer::SPtr keySPtr = &key;
            auto valueSPtr = CreateInsertedVersionedItem(value, sequenceNumber);
            return KeyValuePair<KBuffer::SPtr, VersionedItem<KBuffer::SPtr>::SPtr>(keySPtr, valueSPtr);
        }

        KSharedArray<KeyValuePair<KBuffer::SPtr, VersionedItem<KBuffer::SPtr>::SPtr>>::SPtr CreateEmptyArray()
        {
            KSharedArray<KeyValuePair<KBuffer::SPtr, VersionedItem<KBuffer::SPtr>::SPtr>>::SPtr resultSPtr =
                _new(TEST_TAG, GetAllocator()) KSharedArray<KeyValuePair<KBuffer::SPtr, VersionedItem<KBuffer::SPtr>::SPtr>>();
            return resultSPtr;
        }

        ktl::Awaitable<void> ReadKeys(
            __in CheckpointFile & checkpointFile,
            __in ULONG32 keysPerCheckpoint)
        {
            CheckpointFile::SPtr checkpointFileSPtr = &checkpointFile;

            auto serializerSPtr = CreateBufferSerializer();

            Common::Stopwatch stopwatch;
            stopwatch.Start();

            ULONG32 count = 0;
            auto enumeratorSPtr = checkpointFileSPtr->GetAsyncEnumerator<KBuffer::SPtr, KBuffer::SPtr>(*serializerSPtr);

            while (co_await enumeratorSPtr->MoveNextAsync(CancellationToken::None))
            {
                count += 1;
            }

            co_await enumeratorSPtr->CloseAsync();

            //Trace.WriteInfo(
            //    BoostTestTrace,
            //    "Read {0} keys from checkpoint file: {1} ms",
            //    count,
            //    stopwatch.ElapsedMilliseconds);

            co_return;
        }

        ktl::Awaitable<void> ReadValues(
            __in KSharedArray<CheckpointFile::SPtr> & checkpointFiles,
            __in KSharedArray<KeyValuePair<KBuffer::SPtr, VersionedItem<KBuffer::SPtr>::SPtr>> & items,
            __in ULONG32 offset,
            __in ULONG32 count,
            __in ULONG32 keysPerCheckpoint)
        {
            KSharedArray<CheckpointFile::SPtr>::SPtr checkpointFilesSPtr = &checkpointFiles;
            KSharedArray<KeyValuePair<KBuffer::SPtr, VersionedItem<KBuffer::SPtr>::SPtr>>::SPtr itemsSPtr = &items;

            auto serializerSPtr = CreateBufferSerializer();

            // TODO: Move inside the loop to get checkpoint file for specific i
            ULONG32 checkpointIndex = offset / keysPerCheckpoint;
            auto checkpointFileSPtr = (*checkpointFilesSPtr)[checkpointIndex];

            Common::Stopwatch stopwatch;
            stopwatch.Start();

            for (ULONG32 i = offset; i < offset + count; i++)
            {
                auto item = (*itemsSPtr)[i];
                co_await checkpointFileSPtr->ReadValueAsync<KBuffer::SPtr>(*item.Value, *serializerSPtr);
            }

            //Trace.WriteInfo(
            //    BoostTestTrace,
            //    "Read {0} values from checkpoint file #{1}: {2} ms",
            //    count,
            //    offset/keysPerCheckpoint,
            //    stopwatch.ElapsedMilliseconds);

            co_return;
        }

        void CheckpointFilesAddReadKeys(
            __in KStringView & filenameBase,
            __in ULONG32 numCheckpointFiles,
            __in ULONG32 numItems, 
            __in ULONG32 keySize,
            __in ULONG32 valueSize)
        {
            TRACE_TEST();
            ULONG32 keysPerCheckpointFile = numItems / numCheckpointFiles;

            auto allItemsSPtr = CreateEmptyArray();
            KSharedArray<CheckpointFile::SPtr>::SPtr checkpointFilesSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<CheckpointFile::SPtr>();

            Common::Stopwatch globalStopwatch;

            for (ULONG32 c = 0; c < numCheckpointFiles; c++)
            {
                auto checkpointData = CreateEmptyArray();
                for (ULONG32 k = 0; k < keysPerCheckpointFile; k++)
                {
                    ULONG32 index = allItemsSPtr->Count();
                    auto keySPtr = CreateBuffer(keySize, index);
                    auto valueSPtr = CreateBuffer(valueSize, index);
                    auto pair = MakeKeyValuePair(*keySPtr, *valueSPtr, index);

                    allItemsSPtr->Append(pair);
                    checkpointData->Append(pair);
                }
                KString::SPtr filename = nullptr;
                KString::Create(filename, GetAllocator(), filenameBase);
                filename->Concat(to_wstring(c).c_str());

                Common::Stopwatch stopwatch;
                globalStopwatch.Start();
                stopwatch.Start();
                auto file = SyncAwait(CreateCheckpointFileAsync(*filename, checkpointData, c+1));
                globalStopwatch.Stop();

                //Trace.WriteInfo(
                //    BoostTestTrace,
                //    "Create checkpoint file #{0} with {1} items (Key Size: {2}, Value Size: {3}): {4} ms",
                //    c+1,
                //    keysPerCheckpointFile,
                //    keySize,
                //    valueSize,
                //    stopwatch.ElapsedMilliseconds);

                checkpointFilesSPtr->Append(file);
            }

            Trace.WriteInfo(
                BoostTestTrace,
                "Created {0} checkpoint files with {1} items each (Key Size: {2}, Value Size: {3}): {4} ms",
                numCheckpointFiles,
                keysPerCheckpointFile,
                keySize,
                valueSize,
                globalStopwatch.ElapsedMilliseconds);

            KSharedArray<ktl::Awaitable<void>>::SPtr readersSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<ktl::Awaitable<void>>();

            for (ULONG32 i = 0; i < numCheckpointFiles; i++)
            {
                readersSPtr->Append(ReadKeys(*(*checkpointFilesSPtr)[i], keysPerCheckpointFile));
            }

            Common::Stopwatch stopwatch;
            stopwatch.Start();
            SyncAwait(StoreUtilities::WhenAll<void>(*readersSPtr, GetAllocator()));

            Trace.WriteInfo(
                BoostTestTrace,
                "Read {0} keys from {1} checkpoint files (Key Size: {2}, Value Size: {3}): {4} ms",
                numItems,
                numCheckpointFiles,
                keySize,
                valueSize,
                stopwatch.ElapsedMilliseconds);

            for (ULONG32 i = 0; i < checkpointFilesSPtr->Count(); i++)
            {
                CleanupCheckpointFile(*(*checkpointFilesSPtr)[i]);
            }
        }

        void CheckpointFilesAddReadValues(
            __in KStringView & filenameBase,
            __in ULONG32 numCheckpointFiles,
            __in ULONG32 numItems, 
            __in ULONG32 keySize,
            __in ULONG32 valueSize,
            __in ULONG32 numReaders)
        {
            TRACE_TEST();
            ULONG32 keysPerCheckpointFile = numItems / numCheckpointFiles;

            auto allItemsSPtr = CreateEmptyArray();
            KSharedArray<CheckpointFile::SPtr>::SPtr checkpointFilesSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<CheckpointFile::SPtr>();

            Common::Stopwatch globalStopwatch;

            for (ULONG32 c = 0; c < numCheckpointFiles; c++)
            {
                auto checkpointData = CreateEmptyArray();
                for (ULONG32 k = 0; k < keysPerCheckpointFile; k++)
                {
                    ULONG32 index = allItemsSPtr->Count();
                    auto keySPtr = CreateBuffer(keySize, index);
                    auto valueSPtr = CreateBuffer(valueSize, index);
                    auto pair = MakeKeyValuePair(*keySPtr, *valueSPtr, index);

                    allItemsSPtr->Append(pair);
                    checkpointData->Append(pair);
                }
                KString::SPtr filename = nullptr;
                KString::Create(filename, GetAllocator(), filenameBase);
                filename->Concat(to_wstring(c).c_str());

                Common::Stopwatch stopwatch;
                globalStopwatch.Start();
                stopwatch.Start();
                auto file = SyncAwait(CreateCheckpointFileAsync(*filename, checkpointData, c+1));
                globalStopwatch.Stop();

                //Trace.WriteInfo(
                //    BoostTestTrace,
                //    "Create checkpoint file #{0} with {1} items (Key Size: {2}, Value Size: {3}): {4} ms",
                //    c+1,
                //    keysPerCheckpointFile,
                //    keySize,
                //    valueSize,
                //    stopwatch.ElapsedMilliseconds);

                checkpointFilesSPtr->Append(file);
            }

            Trace.WriteInfo(
                BoostTestTrace,
                "Created {0} checkpoint files with {1} items each (Key Size: {2}, Value Size: {3}): {4} ms",
                numCheckpointFiles,
                keysPerCheckpointFile,
                keySize,
                valueSize,
                globalStopwatch.ElapsedMilliseconds);

            KSharedArray<ktl::Awaitable<void>>::SPtr readersSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<ktl::Awaitable<void>>();
            ULONG32 offset = 0;
            const ULONG32 count = numItems / numReaders;

            for (ULONG32 i = 0; i < numReaders; i++)
            {
                readersSPtr->Append(ReadValues(*checkpointFilesSPtr, *allItemsSPtr, offset, count, keysPerCheckpointFile));
                offset += count;
            }

            Common::Stopwatch stopwatch;
            stopwatch.Start();
            SyncAwait(StoreUtilities::WhenAll<void>(*readersSPtr, GetAllocator()));

            Trace.WriteInfo(
                BoostTestTrace,
                "Read {0} values from {1} checkpoint files with {2} readers (Key Size: {3}, Value Size: {4}): {5} ms",
                numItems,
                numCheckpointFiles,
                numReaders,
                keySize,
                valueSize,
                stopwatch.ElapsedMilliseconds);

            for (ULONG32 i = 0; i < checkpointFilesSPtr->Count(); i++)
            {
                CleanupCheckpointFile(*(*checkpointFilesSPtr)[i]);
            }
        }

        private:
            KtlSystem* ktlSystem_;
    };

    BOOST_FIXTURE_TEST_SUITE(CheckpointFilePerfTestSuite, CheckpointFilePerfTest)

#pragma region Read only values from checkpoint file
    BOOST_AUTO_TEST_CASE(CheckpointFile_1File_500_1Reader)
    {
        KString::SPtr filename = CreateFileString(L"CheckpointFile_1File_500_1Reader");
        const ULONG32 numCheckpointFiles = 1;
        const ULONG32 numItems = 500;
        const ULONG32 keySize = 100;
        const ULONG32 valueSize = 100;
        const ULONG32 numReaders = 1;

        CheckpointFilesAddReadValues(*filename, numCheckpointFiles, numItems, keySize, valueSize, numReaders);
    }

    BOOST_AUTO_TEST_CASE(CheckpointFile_1File_5K_1Reader, *boost::unit_test::label("perf-cit"))
    {
        KString::SPtr filename = CreateFileString(L"CheckpointFile_1File_5K_1Reader");
        const ULONG32 numCheckpointFiles = 1;
        const ULONG32 numItems = 5000;
        const ULONG32 keySize = 100;
        const ULONG32 valueSize = 100;
        const ULONG32 numReaders = 1;

        CheckpointFilesAddReadValues(*filename, numCheckpointFiles, numItems, keySize, valueSize, numReaders);
    }

    BOOST_AUTO_TEST_CASE(CheckpointFile_1File_1M_1Reader, *boost::unit_test::label("perf-cit"))
    {
        KString::SPtr filename = CreateFileString(L"CheckpointFile_1File_1M_1Reader");
        const ULONG32 numCheckpointFiles = 1;
        const ULONG32 numItems = 1000000;
        const ULONG32 keySize = 100;
        const ULONG32 valueSize = 100;
        const ULONG32 numReaders = 1;

        CheckpointFilesAddReadValues(*filename, numCheckpointFiles, numItems, keySize, valueSize, numReaders);
    }

    BOOST_AUTO_TEST_CASE(CheckpointFile_200File_100K_200Readers, *boost::unit_test::label("perf-cit"))
    {
        KString::SPtr filename = CreateFileString(L"CheckpointFile_200File_100K_200Readers");
        const ULONG32 numCheckpointFiles = 200;
        const ULONG32 numItems = 100000;
        const ULONG32 keySize = 100;
        const ULONG32 valueSize = 100;
        const ULONG32 numReaders = 200;

        CheckpointFilesAddReadValues(*filename, numCheckpointFiles, numItems, keySize, valueSize, numReaders);
    }

    BOOST_AUTO_TEST_CASE(CheckpointFile_200File_1M_200Readers, *boost::unit_test::label("perf-cit"))
    {
        KString::SPtr filename = CreateFileString(L"CheckpointFile_200File_1M_200Readers");
        const ULONG32 numCheckpointFiles = 200;
        const ULONG32 numItems = 1000000;
        const ULONG32 keySize = 100;
        const ULONG32 valueSize = 100;
        const ULONG32 numReaders = 200;

        CheckpointFilesAddReadValues(*filename, numCheckpointFiles, numItems, keySize, valueSize, numReaders);
    }

    BOOST_AUTO_TEST_CASE(CheckpointFile_100File_500K_100Readers)
    {
        KString::SPtr filename = CreateFileString(L"CheckpointFile_100File_500K_100Readers");
        const ULONG32 numCheckpointFiles = 100;
        const ULONG32 numItems = 500000;
        const ULONG32 keySize = 100;
        const ULONG32 valueSize = 100;
        const ULONG32 numReaders = 100;

        CheckpointFilesAddReadValues(*filename, numCheckpointFiles, numItems, keySize, valueSize, numReaders);
    }

    BOOST_AUTO_TEST_CASE(CheckpointFile_50File_250K_50Readers)
    {
        KString::SPtr filename = CreateFileString(L"CheckpointFile_100File_500K_100Readers");
        const ULONG32 numCheckpointFiles = 50;
        const ULONG32 numItems = 250000;
        const ULONG32 keySize = 100;
        const ULONG32 valueSize = 100;
        const ULONG32 numReaders = 50;

        CheckpointFilesAddReadValues(*filename, numCheckpointFiles, numItems, keySize, valueSize, numReaders);
    }

    BOOST_AUTO_TEST_CASE(CheckpointFile_10File_50K_10Readers)
    {
        KString::SPtr filename = CreateFileString(L"CheckpointFile_100File_500K_100Readers");
        const ULONG32 numCheckpointFiles = 10;
        const ULONG32 numItems = 50000;
        const ULONG32 keySize = 100;
        const ULONG32 valueSize = 100;
        const ULONG32 numReaders = 10;

        CheckpointFilesAddReadValues(*filename, numCheckpointFiles, numItems, keySize, valueSize, numReaders);
    }

    BOOST_AUTO_TEST_CASE(CheckpointFile_2File_10K_2Readers)
    {
        KString::SPtr filename = CreateFileString(L"CheckpointFile_100File_500K_100Readers");
        const ULONG32 numCheckpointFiles = 2;
        const ULONG32 numItems = 10000;
        const ULONG32 keySize = 100;
        const ULONG32 valueSize = 100;
        const ULONG32 numReaders = 2;

        CheckpointFilesAddReadValues(*filename, numCheckpointFiles, numItems, keySize, valueSize, numReaders);
    }

    BOOST_AUTO_TEST_CASE(CheckpointFile_1File_10K_2Readers)
    {
        KString::SPtr filename = CreateFileString(L"CheckpointFile_100File_500K_100Readers");
        const ULONG32 numCheckpointFiles = 1;
        const ULONG32 numItems = 10000;
        const ULONG32 keySize = 100;
        const ULONG32 valueSize = 100;
        const ULONG32 numReaders = 2;

        CheckpointFilesAddReadValues(*filename, numCheckpointFiles, numItems, keySize, valueSize, numReaders);
    }

    BOOST_AUTO_TEST_CASE(CheckpointFile_1File_50K_10Readers)
    {
        KString::SPtr filename = CreateFileString(L"CheckpointFile_100File_500K_100Readers");
        const ULONG32 numCheckpointFiles = 1;
        const ULONG32 numItems = 50000;
        const ULONG32 keySize = 100;
        const ULONG32 valueSize = 100;
        const ULONG32 numReaders = 10;

        CheckpointFilesAddReadValues(*filename, numCheckpointFiles, numItems, keySize, valueSize, numReaders);
    }

    BOOST_AUTO_TEST_CASE(CheckpointFile_1File_250K_50Readers)
    {
        KString::SPtr filename = CreateFileString(L"CheckpointFile_100File_500K_100Readers");
        const ULONG32 numCheckpointFiles = 1;
        const ULONG32 numItems = 250000;
        const ULONG32 keySize = 100;
        const ULONG32 valueSize = 100;
        const ULONG32 numReaders = 50;

        CheckpointFilesAddReadValues(*filename, numCheckpointFiles, numItems, keySize, valueSize, numReaders);
    }

    BOOST_AUTO_TEST_CASE(CheckpointFile_1File_500K_100Readers)
    {
        KString::SPtr filename = CreateFileString(L"CheckpointFile_100File_500K_100Readers");
        const ULONG32 numCheckpointFiles = 1;
        const ULONG32 numItems = 500000;
        const ULONG32 keySize = 100;
        const ULONG32 valueSize = 100;
        const ULONG32 numReaders = 100;

        CheckpointFilesAddReadValues(*filename, numCheckpointFiles, numItems, keySize, valueSize, numReaders);
    }

    BOOST_AUTO_TEST_CASE(CheckpointFile_1File_1M_200Readers, *boost::unit_test::label("perf-cit"))
    {
        KString::SPtr filename = CreateFileString(L"CheckpointFile_100File_500K_100Readers");
        const ULONG32 numCheckpointFiles = 1;
        const ULONG32 numItems = 1000000;
        const ULONG32 keySize = 100;
        const ULONG32 valueSize = 100;
        const ULONG32 numReaders = 200;

        CheckpointFilesAddReadValues(*filename, numCheckpointFiles, numItems, keySize, valueSize, numReaders);
    }
#pragma endregion

#pragma region Read only keys from checkpoint file
    BOOST_AUTO_TEST_CASE(CheckpointFile_1File_5K_ReadKeys)
    {
        KString::SPtr filename = CreateFileString(L"1File_5K_keys");
        const ULONG32 numCheckpointFiles = 1;
        const ULONG32 numItems = 5000;
        const ULONG32 keySize = 100;
        const ULONG32 valueSize = 100;
        
        CheckpointFilesAddReadKeys(*filename, numCheckpointFiles, numItems, keySize, valueSize);
    }

    BOOST_AUTO_TEST_CASE(CheckpointFile_1File_100K_ReadKeys, *boost::unit_test::label("perf-cit"))
    {
        KString::SPtr filename = CreateFileString(L"CheckpointFile_1File_100K_ReadKeys");
        const ULONG32 numCheckpointFiles = 1;
        const ULONG32 numItems = 100000;
        const ULONG32 keySize = 100;
        const ULONG32 valueSize = 100;
        
        CheckpointFilesAddReadKeys(*filename, numCheckpointFiles, numItems, keySize, valueSize);
    }

    BOOST_AUTO_TEST_CASE(CheckpointFile_1File_1M_ReadKeys, *boost::unit_test::label("perf-cit"))
    {
        KString::SPtr filename = CreateFileString(L"CheckpointFile_1File_lM_ReadKeys");
        const ULONG32 numCheckpointFiles = 1;
        const ULONG32 numItems = 1000000;
        const ULONG32 keySize = 100;
        const ULONG32 valueSize = 100;
        
        CheckpointFilesAddReadKeys(*filename, numCheckpointFiles, numItems, keySize, valueSize);
    }

    BOOST_AUTO_TEST_CASE(CheckpointFile_2File_10K_ReadKeys)
    {
        KString::SPtr filename = CreateFileString(L"CheckpointFile_2File_10K_ReadKeys");
        const ULONG32 numCheckpointFiles = 2;
        const ULONG32 numItems = 10000;
        const ULONG32 keySize = 100;
        const ULONG32 valueSize = 100;
        
        CheckpointFilesAddReadKeys(*filename, numCheckpointFiles, numItems, keySize, valueSize);
    }

    BOOST_AUTO_TEST_CASE(CheckpointFile_10File_50K_ReadKeys)
    {
        KString::SPtr filename = CreateFileString(L"CheckpointFile_10File_50K_ReadKeys");
        const ULONG32 numCheckpointFiles = 10;
        const ULONG32 numItems = 50000;
        const ULONG32 keySize = 100;
        const ULONG32 valueSize = 100;
        
        CheckpointFilesAddReadKeys(*filename, numCheckpointFiles, numItems, keySize, valueSize);
    }

    BOOST_AUTO_TEST_CASE(CheckpointFile_100File_500K_ReadKeys)
    {
        KString::SPtr filename = CreateFileString(L"CheckpointFile_100File_500K_ReadKeys");
        const ULONG32 numCheckpointFiles = 100;
        const ULONG32 numItems = 500000;
        const ULONG32 keySize = 100;
        const ULONG32 valueSize = 100;
        
        CheckpointFilesAddReadKeys(*filename, numCheckpointFiles, numItems, keySize, valueSize);
    }

    BOOST_AUTO_TEST_CASE(CheckpointFile_200File_1M_ReadKeys, *boost::unit_test::label("perf-cit"))
    {
        KString::SPtr filename = CreateFileString(L"200File_1M_ReadKeys");
        const ULONG32 numCheckpointFiles = 200;
        const ULONG32 numItems = 1000000;
        const ULONG32 keySize = 100;
        const ULONG32 valueSize = 100;
        
        CheckpointFilesAddReadKeys(*filename, numCheckpointFiles, numItems, keySize, valueSize);
    }
#pragma endregion

    BOOST_AUTO_TEST_SUITE_END()
}
