// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace FederationUnitTests
{
    using Common::TimeSpan;
    using namespace Federation;

    
    class FederationConfigTests
    {
    };

    BOOST_FIXTURE_TEST_SUITE(FederationConfigTestsSuite,FederationConfigTests)

    BOOST_AUTO_TEST_CASE(SmokeFederationConfig)
    {
        FederationConfig::Test_Reset();
        FederationConfig & federationConfig = FederationConfig::GetConfig();

        VERIFY_IS_TRUE(2 == federationConfig.NeighborhoodSize);
        VERIFY_IS_TRUE(TimeSpan::FromSeconds(15.3) == federationConfig.MessageTimeout);
            
        // Change some of the values in memory        
        federationConfig.NeighborhoodSize = 4;
            
        FederationConfig & federationConfig2 = FederationConfig::GetConfig();
        VERIFY_IS_TRUE(4 == federationConfig2.NeighborhoodSize);
        VERIFY_IS_TRUE(TimeSpan::FromSeconds(15.3) == federationConfig2.MessageTimeout);
    }

    BOOST_AUTO_TEST_CASE(ResetConfig)
    {
        FederationConfig::Test_Reset();
    }

    BOOST_AUTO_TEST_SUITE_END()
}
