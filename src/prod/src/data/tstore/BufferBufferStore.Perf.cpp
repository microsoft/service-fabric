// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

#define ALLOC_TAG 'psBL'

namespace TStoreTests
{
    using namespace ktl;
    using namespace Data::Utilities;

    class BufferBufferStorePerfTest : public TStorePerfTestBase<KBuffer::SPtr, KBuffer::SPtr, KBufferComparer, KBufferSerializer, KBufferSerializer>
    {
    public:
        Common::CommonConfig config; // load the config object as it's needed for the tracing to work

        KBuffer::SPtr CreateKey(__in ULONG32 index) override
        {
            wstring zeroString = wstring(L"0");
            wstring seedString = to_wstring(index);
            while (seedString.length() < 15) // zero pad the number to make it easier to compare
            {
                seedString = zeroString + seedString;
            }

            wstring str = wstring(L"test_key_") + seedString;

            KBuffer::SPtr result = nullptr;
            KBuffer::CreateOrCopy(result, str.data(), static_cast<ULONG>(str.length() * sizeof(wchar_t)), GetAllocator());
            return result;
        }

        KBuffer::SPtr CreateValue(__in ULONG32 index) override
        {
            return CreateBuffer(500, index);
        }

        BufferBufferStorePerfTest()
        {
            Setup(1);
        }

        ~BufferBufferStorePerfTest()
        {
            Cleanup();
        }
    };

    BOOST_FIXTURE_TEST_SUITE(BufferBufferStorePerfSuite, BufferBufferStorePerfTest)

        BOOST_AUTO_TEST_CASE(CRUD_UniqueKeys_Perf, *boost::unit_test::label("perf-cit"))
    {
        CRUDPerf_UniqueKeys(1'000'000, 200);
    }

    BOOST_AUTO_TEST_SUITE_END()
}
