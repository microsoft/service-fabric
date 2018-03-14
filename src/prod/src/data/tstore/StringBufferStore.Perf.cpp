// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

#define ALLOC_TAG 'psBS'

namespace TStoreTests
{
    using namespace ktl;
    using namespace Data::Utilities;

    class StringBufferStorePerfTest : public TStorePerfTestBase<KString::SPtr, KBuffer::SPtr, KStringComparer, StringStateSerializer, KBufferSerializer>
    {
    public:
        Common::CommonConfig config; // load the config object as it's needed for the tracing to work

        StringBufferStorePerfTest()
        {
            Setup(1);

            Store->EnableBackgroundConsolidation = false;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;
        }

        ~StringBufferStorePerfTest()
        {
            Cleanup();
        }

        KString::SPtr CreateKey(__in ULONG32 index) override
        {
            KString::SPtr key;

            wstring zeroString = wstring(L"0");
            wstring seedString = to_wstring(index);
            while (seedString.length() < 15) // zero pad the number to make it easier to compare
            {
                seedString = zeroString + seedString;
            }

            wstring str = wstring(L"test_key_") + seedString;
            auto status = KString::Create(key, GetAllocator(), str.c_str());
            Diagnostics::Validate(status);
            return key;
        }

        KBuffer::SPtr CreateValue(__in ULONG32 index) override
        {
            return CreateBuffer(500, index);
        }

        void CheckpointAndSweep()
        {
            bool wasSweepEnabled = Store->EnableSweep;
            Store->EnableSweep = false;

            Checkpoint();
            ktl::AwaitableCompletionSource<bool>::SPtr cachedCompletionSourceSPtr = Store->SweepTaskSourceSPtr;
            if (cachedCompletionSourceSPtr != nullptr)
            {
                SyncAwait(cachedCompletionSourceSPtr->GetAwaitable());
                Store->SweepTaskSourceSPtr = nullptr; // Reset so that cleanup doesn't re-await
            }

            Store->EnableSweep = wasSweepEnabled;
        }
    };

    BOOST_FIXTURE_TEST_SUITE(StringBufferStorePerfSuite, StringBufferStorePerfTest)

        BOOST_AUTO_TEST_CASE(CRUD_UniqueKeys_Perf_1MKeys_200Txns, *boost::unit_test::label("perf-cit"))
    {
        CRUDPerf_UniqueKeys(1'000'000, 200);
    }

    BOOST_AUTO_TEST_CASE(SingleKeyPerf_1MKeys_200Txns_Differential, *boost::unit_test::label("perf-cit"))
    {
        SingleKeyReadTest(1'000'000, 200, true, false);
    }

    BOOST_AUTO_TEST_CASE(SingleKeyPerf_1MKeys_1MTxns_200Tasks_Differential, *boost::unit_test::label("perf-cit"))
    {
        SingleKeyReadTest(1'000'000, 200, false, false);
    }

    BOOST_AUTO_TEST_CASE(SingleKeyPerf_1MKeys_200Txns_Consolidated, *boost::unit_test::label("perf-cit"))
    {
        SingleKeyReadTest(1'000'000, 200, true, true);
    }

    BOOST_AUTO_TEST_CASE(SingleKeyPerf_1MKeys_1MTxns_200Tasks_Consolidated, *boost::unit_test::label("perf-cit"))
    {
        SingleKeyReadTest(1'000'000, 200, false, true);
    }

    BOOST_AUTO_TEST_CASE(KeysFromDiskTest_1MKeys, *boost::unit_test::label("perf-cit"))
    {
        CheckpointAndReadKeysFromDiskTest(1'000'000);
    }

    BOOST_AUTO_TEST_CASE(ReadFromDiskTest_5KKeys_1Reader)
    {
        CheckpointAndReadFromDiskTest(5000, 1);
    }

    BOOST_AUTO_TEST_CASE(ReadFromDiskTest_1MKeys_1Reader, *boost::unit_test::label("perf-cit"))
    {
        CheckpointAndReadFromDiskTest(1'000'000, 1);
    }

    BOOST_AUTO_TEST_CASE(ReadFromDiskTest_1MKeys_200Readers, *boost::unit_test::label("perf-cit"))
    {
        CheckpointAndReadFromDiskTest(1'000'000, 200);
    }

	// Disabling since this is a very long test
    //BOOST_AUTO_TEST_CASE(CheckpointScaleTest)
    //{
    //    // 5'000'000'000 bytes total
    //    // 1'000 checkpoint files
    //    // 5'000'000 bytes per checkpoint file
    //    // 10'000 keys per checkpoint file
    //    // 500 bytes per key

    //    MultipleCheckpointTest(10'000'000, 1'000, 200);
    //}

    BOOST_AUTO_TEST_SUITE_END()
}
