// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace FailoverManagerUnitTest
{
    using namespace Reliability;
    using namespace Common;

    using Common::make_unique;
    using std::unique_ptr;
    using std::wstring;

    class TestRSConfig
    {
    };

    BOOST_FIXTURE_TEST_SUITE(TestRSConfigSuite,TestRSConfig)

    BOOST_AUTO_TEST_CASE(SmokeRSConfig)
    {
        // Test loading config from string
        wstring configData =    L"[FailoverManager]\r\n"
                                L"    MaxActionsPerIteration = 512";

        std::shared_ptr<FileConfigStore> fileConfigPtr = std::make_shared<FileConfigStore>(configData, true);
        Config::SetConfigStore(fileConfigPtr);
        FailoverConfig::Test_Reset();
        FailoverConfig & config1 = FailoverConfig::GetConfig();

        VERIFY_ARE_EQUAL(512, config1.MaxActionsPerIteration);

        config1.MaxActionsPerIteration = 233;
        VERIFY_ARE_EQUAL(233, config1.MaxActionsPerIteration);
    }

    BOOST_AUTO_TEST_SUITE_END()
}
