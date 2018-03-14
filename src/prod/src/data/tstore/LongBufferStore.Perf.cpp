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

    class LongBufferStorePerfTest : public TStorePerfTestBase<LONG64, KBuffer::SPtr, LongComparer, TestStateSerializer<LONG64>, KBufferSerializer>
    {
    public:
        Common::CommonConfig config; // load the config object as it's needed for the tracing to work

        LONG64 CreateKey(__in ULONG32 index) override
        {
            return static_cast<LONG64>(index);
        }

        KBuffer::SPtr CreateValue(__in ULONG32 index) override
        {
            return CreateBuffer(500, index);
        }

        LongBufferStorePerfTest()
        {
            Setup(1);
        }

        ~LongBufferStorePerfTest()
        {
            Cleanup();
        }
    };

    BOOST_FIXTURE_TEST_SUITE(LongBufferStorePerfSuite, LongBufferStorePerfTest, *boost::unit_test::label("perf-cit"))

        BOOST_AUTO_TEST_CASE(CRUD_UniqueKeys_Perf)
    {
        CRUDPerf_UniqueKeys(1'000'000, 200);
    }

    BOOST_AUTO_TEST_SUITE_END()
}
