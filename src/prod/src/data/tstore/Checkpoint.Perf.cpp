// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

#define ALLOC_TAG 'cpPT'

namespace TStoreTests
{
    using namespace ktl;
    using namespace Data::Utilities;
    
    class CheckpointPerfTest : public TStorePerfTestBase<KBuffer::SPtr, KBuffer::SPtr, KBufferComparer, KBufferSerializer, KBufferSerializer>
    {
    public:
        typedef KeyValuePair<KBuffer::SPtr, KBuffer::SPtr> BufferPair;

        CheckpointPerfTest()
        {
            Setup(1, KBufferComparer::Hash);
        }
        
        ~CheckpointPerfTest()
        {
            Cleanup();
        }

        void CheckpointSingleFilePerfTest(
            __in ULONG32 totalKeys,
            __in ULONG32 keySizeInBytes,
            __in ULONG32 valueSizeInBytes,
            __in ULONG32 numAddTasks, 
            __in ULONG32 numMemoryReadTasks,
            __in ULONG32 numCheckpointReadTasks)
        {
            TRACE_TEST();

            CODING_ERROR_ASSERT(totalKeys % numAddTasks == 0);
            CODING_ERROR_ASSERT(totalKeys % numMemoryReadTasks == 0);
            CODING_ERROR_ASSERT(totalKeys % numCheckpointReadTasks == 0);

            Trace.WriteInfo(
                BoostTestTrace,
                "CheckpointPerfTest_SingleFile_Unique: Total Keys: {0}; Key Size: {1}; Value Size: {2}; Add Tasks: {3}; Memory Read Tasks: {4}; Checkpoint Read Tasks: {5}",
                totalKeys,
                keySizeInBytes,
                valueSizeInBytes,
                numAddTasks,
                numMemoryReadTasks,
                numCheckpointReadTasks);

            // Create items ahead of time
            KSharedArray<BufferPair>::SPtr itemsSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<BufferPair>();
            for (ULONG32 i = 0; i < totalKeys; i++)
            {
                auto key = CreateBuffer(keySizeInBytes, i);
                auto value = CreateBuffer(valueSizeInBytes, i);
                BufferPair pair(key, value);
                itemsSPtr->Append(pair);
            }

            LONG64 addTime = SyncAwait(AddKeysAsync(*itemsSPtr, numAddTasks));

            Trace.WriteInfo(
                BoostTestTrace,
                "CheckpointPerfTest_SingleFile_Unique Add: {0} ms",
                addTime);

            CODING_ERROR_ASSERT(Store->Count == totalKeys);

            LONG64 memoryReadTime = SyncAwait(VerifyKeyValueAsync(*itemsSPtr, numMemoryReadTasks, KBufferComparer::Equals));

            Trace.WriteInfo(
                BoostTestTrace,
                "CheckpointPerfTest_SingleFile_Unique Read from Memory: {0} ms",
                memoryReadTime);

            Common::Stopwatch stopwatch;
            stopwatch.Start();
            Checkpoint();
            LONG64 checkpointTime = stopwatch.ElapsedMilliseconds;
            stopwatch.Reset();

            Trace.WriteInfo(
                BoostTestTrace,
                "CheckpointPerfTest_SingleFile_Unique Checkpointed: {0} ms",
                checkpointTime);

            LONG64 recoveryTime = CloseAndReOpenStore();
            Trace.WriteInfo(
                BoostTestTrace,
                "CheckpointPerfTest_SingleFile_Unique Recovered: {0} ms",
                recoveryTime);

            LONG64 checkpointReadTime = SyncAwait(VerifyKeyValueAsync(*itemsSPtr, numCheckpointReadTasks, KBufferComparer::Equals));

            Trace.WriteInfo(
                BoostTestTrace,
                "CheckpointPerfTest_SingleFile_Unique Checkpoint Read: {0} ms",
                checkpointReadTime);
        }

        void ConcurrentMultiCheckpointFileRead(
            __in ULONG32 totalKeys,
            __in ULONG32 keySizeInBytes,
            __in ULONG32 valueSizeInBytes,
            __in ULONG32 numTasks)
        {
            TRACE_TEST();

            CODING_ERROR_ASSERT(totalKeys % numTasks == 0);
            ULONG32 keysPerTask = totalKeys / numTasks;

            // Create items ahead of time
            KSharedArray<BufferPair>::SPtr itemsSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<BufferPair>();
            for (ULONG32 i = 0; i < totalKeys; i++)
            {
                auto key = CreateBuffer(keySizeInBytes, i);
                auto value = CreateBuffer(valueSizeInBytes, i);
                BufferPair pair(key, value);
                itemsSPtr->Append(pair);
            }

            Common::Stopwatch stopwatch;

            stopwatch.Start();

            ULONG32 offset = 0;
            for (ULONG32 i = 0; i < numTasks; i++)
            {
                SyncAwait(AddKeysAsync(*itemsSPtr, offset, keysPerTask));
                Checkpoint();
                offset += keysPerTask;
            }

            LONG64 addTime = stopwatch.ElapsedMilliseconds;
            stopwatch.Reset();

            Trace.WriteInfo(
                BoostTestTrace,
                "CheckpointPerfTest_MultiFile_Concurrent Add with {0} checkpoints: {1} ms",
                numTasks,
                addTime);

            LONG64 memoryReadTime = SyncAwait(GetKeyAsync(*itemsSPtr, numTasks));

            Trace.WriteInfo(
                BoostTestTrace,
                "CheckpointPerfTest_MultiFile_Concurrent Read from Memory: {0} ms",
                memoryReadTime);

            LONG64 recoveryTime = CloseAndReOpenStore();
            Trace.WriteInfo(
                BoostTestTrace,
                "CheckpointPerfTest_MultiFile_Concurrent Recovered: {0} ms",
                recoveryTime);

            //LONG64 checkpointReadTime = SyncAwait(GetKeyAsync(*itemsSPtr, numTasks));

            //Trace.WriteInfo(
            //    BoostTestTrace,
            //    "CheckpointPerfTest_MultiFile_Concurrent Checkpoint Read: {0} ms",
            //    checkpointReadTime);
        }

        Common::CommonConfig config; // load the config object as it's needed for the tracing to work
    };

    BOOST_FIXTURE_TEST_SUITE(CheckpointPerfTestSuite, CheckpointPerfTest)

    // Naming Convention: CheckpointPerfTest_SingleFile_Unique_{num keys}_{key size}_{num checkpoint read file streams}

    BOOST_AUTO_TEST_CASE(CheckpointPerfTest_SingleFile_Unique_1K_100bytes_1FileStream)
    {
        ULONG32 totalKeys = 1000;
        ULONG32 keySize = 100;
        ULONG32 valueSize = 8;
        ULONG32 numAddTasks = 200;
        ULONG32 numMemoryReadTasks = 200;
        ULONG32 numCheckpointReadTasks = 1;

        CheckpointSingleFilePerfTest(
            totalKeys,
            keySize,
            valueSize,
            numAddTasks,
            numMemoryReadTasks,
            numCheckpointReadTasks);

    }

    BOOST_AUTO_TEST_CASE(CheckpointPerfTest_SingleFile_Unique_10K_100bytes_1FileStream)
    {
        ULONG32 totalKeys = 10000;
        ULONG32 keySize = 100;
        ULONG32 valueSize = 8;
        ULONG32 numAddTasks = 200;
        ULONG32 numMemoryReadTasks = 200;
        ULONG32 numCheckpointReadTasks = 1;

        CheckpointSingleFilePerfTest(
            totalKeys,
            keySize,
            valueSize,
            numAddTasks,
            numMemoryReadTasks,
            numCheckpointReadTasks);
    }

    BOOST_AUTO_TEST_CASE(CheckpointPerfTest_SingleFile_Unique_100K_100bytes_1FileStream, *boost::unit_test::label("perf-cit"))
    {
        ULONG32 totalKeys = 100000;
        ULONG32 keySize = 100;
        ULONG32 valueSize = 8;
        ULONG32 numAddTasks = 200;
        ULONG32 numMemoryReadTasks = 200;
        ULONG32 numCheckpointReadTasks = 1;

        CheckpointSingleFilePerfTest(
            totalKeys,
            keySize,
            valueSize,
            numAddTasks,
            numMemoryReadTasks,
            numCheckpointReadTasks);
    }

    BOOST_AUTO_TEST_CASE(CheckpointPerfTest_SingleFile_Unique_5K_100bytes_1FileStream)
    {
        ULONG32 totalKeys = 5000;
        ULONG32 keySize = 100;
        ULONG32 valueSize = 8;
        ULONG32 numAddTasks = 200;
        ULONG32 numMemoryReadTasks = 200;
        ULONG32 numCheckpointReadTasks = 1;

        CheckpointSingleFilePerfTest(
            totalKeys,
            keySize,
            valueSize,
            numAddTasks,
            numMemoryReadTasks,
            numCheckpointReadTasks);
    }


    BOOST_AUTO_TEST_CASE(CheckpointPerfTest_MultiFile_Concurrent_1K_100bytes_200FileStream)
    {
        ULONG32 totalKeys = 1000;
        ULONG32 keySize = 100;
        ULONG32 valueSize = 8;
        ULONG32 numTasks = 200;
        
        ConcurrentMultiCheckpointFileRead(totalKeys, keySize, valueSize, numTasks);
    }

    BOOST_AUTO_TEST_CASE(CheckpointPerfTest_MultiFile_Concurrent_10K_100bytes_200FileStream)
    {
        ULONG32 totalKeys = 10000;
        ULONG32 keySize = 100;
        ULONG32 valueSize = 8;
        ULONG32 numTasks = 200;
        
        ConcurrentMultiCheckpointFileRead(totalKeys, keySize, valueSize, numTasks);
    }

    BOOST_AUTO_TEST_CASE(CheckpointPerfTest_MultiFile_Concurrent_100K_100bytes_200FileStream, *boost::unit_test::label("perf-cit"))
    {
        ULONG32 totalKeys = 100000;
        ULONG32 keySize = 100;
        ULONG32 valueSize = 8;
        ULONG32 numTasks = 200;
        
        ConcurrentMultiCheckpointFileRead(totalKeys, keySize, valueSize, numTasks);
    }

    BOOST_AUTO_TEST_CASE(CheckpointPerfTest_MultiFile_Concurrent_1MK_100bytes_200FileStream, *boost::unit_test::label("perf-cit"))
    {
        ULONG32 totalKeys = 1000000;
        ULONG32 keySize = 100;
        ULONG32 valueSize = 8;
        ULONG32 numTasks = 200;
        
        ConcurrentMultiCheckpointFileRead(totalKeys, keySize, valueSize, numTasks);
    }

    BOOST_AUTO_TEST_SUITE_END()

}
