// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include <memory>
#include "Common/TimeSpan.h"
#include "Common/ExpiringSet.h"

using namespace std;
using namespace Common;

namespace Common
{
    struct ExpiringSetTestEntry
    {
        ExpiringSetTestEntry(bool completed)
            : Completed(completed)
        {
        }

        bool IsCompleted() const
        {
            return Completed;
        }

        bool Completed;
    };

    class TestExpiringSet
    {
    };

    BOOST_FIXTURE_TEST_SUITE(TestExpiringSetSuite,TestExpiringSet)

    BOOST_AUTO_TEST_CASE(BasicExpiringSet)
    {
        ExpiringSet<int, ExpiringSetTestEntry> set(TimeSpan::FromSeconds(5), 3);

        VERIFY_IS_TRUE(set.TryAdd(1, true));
        VERIFY_IS_TRUE(set.TryAdd(3, false));
        VERIFY_IS_FALSE(set.TryAdd(1, false));
        VERIFY_IS_TRUE(set.TryAdd(2, true));

        set.RemoveExpiredEntries();
        VERIFY_IS_TRUE(set.size() == 3u);

        Sleep(6000);
        set.RemoveExpiredEntries();
        VERIFY_IS_TRUE(set.TryAdd(1, false));
        VERIFY_IS_TRUE(set.TryAdd(2, true));

        auto it = set.find(3);
        VERIFY_IS_TRUE(it != set.end());
        it->second.Completed = true;
        set.RemoveExpiredEntries();
        VERIFY_IS_TRUE(set.find(3) == set.end());

        VERIFY_IS_TRUE(set.TryAdd(3, false));
        VERIFY_IS_TRUE(set.TryAdd(4, false));
        VERIFY_IS_TRUE(set.find(1) != set.end());
        VERIFY_IS_TRUE(set.find(2) == set.end());
        VERIFY_IS_TRUE(set.size() == 3u);
        VERIFY_IS_FALSE(set.TryAdd(5, false));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
