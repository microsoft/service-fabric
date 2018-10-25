// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

#define ALLOC_TAG 'plSC'

namespace TStoreTests
{
    using namespace ktl;
    using namespace Data::Utilities;

    class DataStructuresLongBytePerfTest : public DataStructuresPerfBase<LONG64, byte>
    {
    public:
        Common::CommonConfig config; // load the config object as it's needed for the tracing to work

        DataStructuresLongBytePerfTest()
        {
            Setup();
        }

        ~DataStructuresLongBytePerfTest()
        {
            Cleanup();
        }
    };

    class DataStructuresLongBufferPerfTest : public DataStructuresPerfBase<LONG64, KBuffer::SPtr>
    {
    public:
        Common::CommonConfig config; // load the config object as it's needed for the tracing to work

        DataStructuresLongBufferPerfTest()
        {
            Setup();
        }

        ~DataStructuresLongBufferPerfTest()
        {
            Cleanup();
        }
    };

    class DataStructuresStringBufferPerfTest : public DataStructuresPerfBase<KString::SPtr, KBuffer::SPtr>
    {
    public:
        Common::CommonConfig config; // load the config object as it's needed for the tracing to work

        DataStructuresStringBufferPerfTest()
        {
            Setup();
        }

        ~DataStructuresStringBufferPerfTest()
        {
            Cleanup();
        }
    };

    class DataStructuresSPtrBufferPerfTest : public DataStructuresPerfBase<SharedLong::SPtr, KBuffer::SPtr>
    {
    public:
        Common::CommonConfig config; // load the config object as it's needed for the tracing to work

        DataStructuresSPtrBufferPerfTest()
        {
            Setup();
        }

        ~DataStructuresSPtrBufferPerfTest()
        {
            Cleanup();
        }
    };

#pragma region Long-Byte Tests
    BOOST_FIXTURE_TEST_SUITE(DataStructuresLongBytePerfSuite, DataStructuresLongBytePerfTest)

    BOOST_AUTO_TEST_CASE(ConcurrentSkipListPerf_LongByte_SequentialAddRead_1M)
    {
        TRACE_TEST();
        ConcurrentSkipListPerf_SequentialAddReadTest(1'000'000);
    }

    BOOST_AUTO_TEST_CASE(ConcurrentSkipList_LongByte_ConcurrentAddThenReadTest_1M_200Tasks)
    {
        TRACE_TEST();
        ConcurrentSkipList_ConcurrentAddThenReadTest(1'000'000, 200);
    }

    BOOST_AUTO_TEST_CASE(ConcurrentSkipList_LongByte_SingleKeyConcurrentRead_1M_200Task)
    {
        TRACE_TEST();
        ConcurrentSkipList_SingleKeyReadPerfTest(1'000'000, 200);
    }

    BOOST_AUTO_TEST_CASE(FastSkipListPerf_LongByte_SequentialAddRead_1M)
    {
        TRACE_TEST();
        FastSkipListPerf_SequentialAddReadTest(1'000'000);
    }

    BOOST_AUTO_TEST_CASE(FastSkipList_LongByte_ConcurrentAddThenReadTest_1M_200Tasks)
    {
        TRACE_TEST();
        FastSkipList_ConcurrentAddThenReadTest(1'000'000, 200);
    }

    BOOST_AUTO_TEST_CASE(FastSkipList_LongByte_SingleKeyConcurrentRead_1M_200Task)
    {
        TRACE_TEST();
        FastSkipList_SingleKeyReadPerfTest(1'000'000, 200);
    }

    BOOST_AUTO_TEST_CASE(PartitionSortedList_LongByte_SequentialAddRead_1M)
    {
        TRACE_TEST();
        PartitionSortedList_SequentialAddReadTest(1'000'000);
    }

    BOOST_AUTO_TEST_CASE(PartitionSortedList_LongByte_ConcurrentReadTest_1M_200Tasks)
    {
        TRACE_TEST();
        PartitionedSortedList_ConcurrentReadTest(1'000'000, 200);
    }

    BOOST_AUTO_TEST_CASE(PartitionSortedList_LongByte_ConcurrentSingleKeyReadTest_1M_200Tasks)
    {
        TRACE_TEST();
        PartitionedSortedList_SingleKeyReadPerfTest(1'000'000, 200);
    }

    BOOST_AUTO_TEST_CASE(ConcurrentHashTablePerf_LongByte_SequentialAddRead_1M)
    {
        TRACE_TEST();
        ConcurrentHashTablePerf_SequentialAddReadTest(1'000'000);
    }

    BOOST_AUTO_TEST_CASE(ConcurrentHashTable_LongByte_ConcurrentAddThenReadTest_1M_200Tasks)
    {
        TRACE_TEST();
        ConcurrentHashTable_ConcurrentAddThenReadTest(1'000'000, 200);
    }

    BOOST_AUTO_TEST_CASE(ConcurrentHashTable_LongByte_SingleKeyConcurrentRead_1M_200Task)
    {
        TRACE_TEST();
        ConcurrentHashTable_SingleKeyReadPerfTest(1'000'000, 200);
    }

    BOOST_AUTO_TEST_SUITE_END()
#pragma endregion

#pragma region Long-Buffer Tests
    BOOST_FIXTURE_TEST_SUITE(DataStructuresLongBufferPerfSuite, DataStructuresLongBufferPerfTest)

    BOOST_AUTO_TEST_CASE(ConcurrentSkipListPerf_LongBuffer_SequentialAddRead_1M)
    {
        ConcurrentSkipListPerf_SequentialAddReadTest(1'000'000);
    }

    BOOST_AUTO_TEST_CASE(ConcurrentSkipList_LongBuffer_ConcurrentAddThenReadTest_1M_200Tasks)
    {
        ConcurrentSkipList_ConcurrentAddThenReadTest(1'000'000, 200);
    }

    BOOST_AUTO_TEST_CASE(ConcurrentSkipList_LongBuffer_SingleKeyConcurrentRead_1M_200Task)
    {
        ConcurrentSkipList_SingleKeyReadPerfTest(1'000'000, 200);
    }

    BOOST_AUTO_TEST_CASE(FastSkipListPerf_LongBuffer_SequentialAddRead_1M)
    {
        FastSkipListPerf_SequentialAddReadTest(1'000'000);
    }

    BOOST_AUTO_TEST_CASE(FastSkipList_LongBuffer_ConcurrentAddThenReadTest_1M_200Tasks)
    {
        FastSkipList_ConcurrentAddThenReadTest(1'000'000, 200);
    }

    BOOST_AUTO_TEST_CASE(FastSkipList_LongBuffer_SingleKeyConcurrentRead_1M_200Task)
    {
        FastSkipList_SingleKeyReadPerfTest(1'000'000, 200);
    }

    BOOST_AUTO_TEST_CASE(PartitionSortedList_LongBuffer_SequentialAddRead_1M)
    {
        PartitionSortedList_SequentialAddReadTest(1'000'000);
    }

    BOOST_AUTO_TEST_CASE(PartitionSortedList_LongBuffer_ConcurrentReadTest_1M_200Tasks)
    {
        PartitionedSortedList_ConcurrentReadTest(1'000'000, 200);
    }

    BOOST_AUTO_TEST_CASE(PartitionSortedList_LongBuffer_ConcurrentSingleKeyReadTest_1M_200Tasks)
    {
        PartitionedSortedList_SingleKeyReadPerfTest(1'000'000, 200);
    }

    BOOST_AUTO_TEST_CASE(ConcurrentHashTablePerf_LongBuffer_SequentialAddRead_1M)
    {
        ConcurrentHashTablePerf_SequentialAddReadTest(1'000'000);
    }

    BOOST_AUTO_TEST_CASE(ConcurrentHashTable_LongBuffer_ConcurrentAddThenReadTest_1M_200Tasks)
    {
        ConcurrentHashTable_ConcurrentAddThenReadTest(1'000'000, 200);
    }

    BOOST_AUTO_TEST_CASE(ConcurrentHashTable_LongBuffer_SingleKeyConcurrentRead_1M_200Task)
    {
        ConcurrentHashTable_SingleKeyReadPerfTest(1'000'000, 200);
    }


    BOOST_AUTO_TEST_SUITE_END()
#pragma endregion

#pragma region String-Buffer Tests
    BOOST_FIXTURE_TEST_SUITE(DataStructuresStringBufferPerfSuite, DataStructuresStringBufferPerfTest)

    BOOST_AUTO_TEST_CASE(ConcurrentSkipListPerf_StringBuffer_SequentialAddRead_1M)
    {
        ConcurrentSkipListPerf_SequentialAddReadTest(1'000'000);
    }

    BOOST_AUTO_TEST_CASE(ConcurrentSkipList_StringBuffer_ConcurrentAddThenReadTest_1M_200Tasks)
    {
        ConcurrentSkipList_ConcurrentAddThenReadTest(1'000'000, 200);
    }

    BOOST_AUTO_TEST_CASE(ConcurrentSkipList_StringBuffer_SingleKeyConcurrentRead_1M_200Task)
    {
        ConcurrentSkipList_SingleKeyReadPerfTest(1'000'000, 200);
    }

    BOOST_AUTO_TEST_CASE(FastSkipListPerf_StringBuffer_SequentialAddRead_1M)
    {
        FastSkipListPerf_SequentialAddReadTest(1'000'000);
    }

    BOOST_AUTO_TEST_CASE(FastSkipList_StringBuffer_ConcurrentAddThenReadTest_1M_200Tasks)
    {
        FastSkipList_ConcurrentAddThenReadTest(1'000'000, 200);
    }

    BOOST_AUTO_TEST_CASE(FastSkipList_StringBuffer_SingleKeyConcurrentRead_1M_200Task)
    {
        FastSkipList_SingleKeyReadPerfTest(1'000'000, 200);
    }

    BOOST_AUTO_TEST_CASE(PartitionSortedList_StringBuffer_SequentialAddRead_1M)
    {
        PartitionSortedList_SequentialAddReadTest(1'000'000);
    }

    BOOST_AUTO_TEST_CASE(PartitionSortedList_StringBuffer_ConcurrentReadTest_1M_200Tasks)
    {
        PartitionedSortedList_ConcurrentReadTest(1'000'000, 200);
    }

    BOOST_AUTO_TEST_CASE(PartitionSortedList_StringBuffer_ConcurrentSingleKeyReadTest_1M_200Tasks)
    {
        PartitionedSortedList_SingleKeyReadPerfTest(1'000'000, 200);
    }

    BOOST_AUTO_TEST_CASE(ConcurrentHashTablePerf_StringBuffer_SequentialAddRead_1M)
    {
        ConcurrentHashTablePerf_SequentialAddReadTest(1'000'000);
    }

    BOOST_AUTO_TEST_CASE(ConcurrentHashTable_StringBuffer_ConcurrentAddThenReadTest_1M_200Tasks)
    {
        ConcurrentHashTable_ConcurrentAddThenReadTest(1'000'000, 200);
    }

    BOOST_AUTO_TEST_CASE(ConcurrentHashTable_StringBuffer_SingleKeyConcurrentRead_1M_200Task)
    {
        ConcurrentHashTable_SingleKeyReadPerfTest(1'000'000, 200);
    }

    BOOST_AUTO_TEST_SUITE_END()
#pragma endregion

#pragma region SPtr-Buffer Tests
    BOOST_FIXTURE_TEST_SUITE(DataStructuresSPtrBufferPerfSuite, DataStructuresSPtrBufferPerfTest, *boost::unit_test::label("perf-cit"))

    BOOST_AUTO_TEST_CASE(ConcurrentSkipListPerf_SPtrBuffer_SequentialAddRead_1M)
    {
        ConcurrentSkipListPerf_SequentialAddReadTest(1'000'000);
    }

    BOOST_AUTO_TEST_CASE(ConcurrentSkipList_SPtrBuffer_ConcurrentAddThenReadTest_1M_200Tasks)
    {
        ConcurrentSkipList_ConcurrentAddThenReadTest(1'000'000, 200);
    }

    BOOST_AUTO_TEST_CASE(ConcurrentSkipList_SPtrBuffer_SingleKeyConcurrentRead_1M_200Task)
    {
        ConcurrentSkipList_SingleKeyReadPerfTest(1'000'000, 200);
    }

    BOOST_AUTO_TEST_CASE(FastSkipListPerf_SPtrBuffer_SequentialAddRead_1M)
    {
        FastSkipListPerf_SequentialAddReadTest(1'000'000);
    }

    BOOST_AUTO_TEST_CASE(FastSkipList_SPtrBuffer_ConcurrentAddThenReadTest_1M_200Tasks)
    {
        FastSkipList_ConcurrentAddThenReadTest(1'000'000, 200);
    }

    BOOST_AUTO_TEST_CASE(FastSkipList_SPtrBuffer_SingleKeyConcurrentRead_1M_200Task)
    {
        FastSkipList_SingleKeyReadPerfTest(1'000'000, 200);
    }

    BOOST_AUTO_TEST_CASE(PartitionSortedList_SPtrBuffer_SequentialAddRead_1M)
    {
        PartitionSortedList_SequentialAddReadTest(1'000'000);
    }

    BOOST_AUTO_TEST_CASE(PartitionSortedList_SPtrBuffer_ConcurrentReadTest_1M_200Tasks)
    {
        PartitionedSortedList_ConcurrentReadTest(1'000'000, 200);
    }

    BOOST_AUTO_TEST_CASE(PartitionSortedList_SPtrBuffer_ConcurrentSingleKeyReadTest_1M_200Tasks)
    {
        PartitionedSortedList_SingleKeyReadPerfTest(1'000'000, 200);
    }

    BOOST_AUTO_TEST_CASE(ConcurrencyPerfTest)
    {
        ConcurrencyPerfForTasks(200);
    }

    BOOST_AUTO_TEST_SUITE_END()
#pragma endregion
}
