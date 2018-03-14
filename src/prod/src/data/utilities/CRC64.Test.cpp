// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace UtilitiesTests
{
    using namespace ktl;
    using namespace Data::Utilities;

    class CRC64Test
    {
    public:
        Common::CommonConfig config; // load the config object as its needed for the tracing to work
    };

    BOOST_GLOBAL_FIXTURE(CRC64Test);

    BOOST_AUTO_TEST_CASE(ToCRC64)
    {
        byte buffer[] = { 12, 1, 4, 0, 9, 127, 1 };

        LONG64 result = CRC64::ToCRC64(buffer, 1, 3);
        CODING_ERROR_ASSERT(result == 8700962087620482706);

        result = CRC64::ToCRC64(buffer, 0, 7);
        CODING_ERROR_ASSERT(result == 7867889297122755539);

        result = CRC64::ToCRC64(buffer, 2, 5);
        CODING_ERROR_ASSERT(result == 14226437255121905647);
    }
}
