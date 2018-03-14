// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "RandomDistribution.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace PlacementAndLoadBalancingUnitTest
{
    using namespace std;
    using namespace Common;
    using namespace Reliability::LoadBalancingComponent;

    class TestRandomDistribution
    {
    protected:
        TestRandomDistribution()
        {
            BOOST_REQUIRE(ClassSetup());
        }
        TEST_CLASS_SETUP(ClassSetup);
        uint32_t seed_;
    };

    BOOST_FIXTURE_TEST_SUITE(RandomDistributionTestSuite, TestRandomDistribution)

    BOOST_AUTO_TEST_CASE(AllOnesDistributionTest)
    {
        RandomDistribution rd(seed_);
        DistributionMap distribution =
            rd.GenerateDistribution(10000, RandomDistribution::Enum::AllOnes);

        VERIFY_ARE_EQUAL(1u, distribution.size());
        VERIFY_ARE_EQUAL(10000u, rd.GetNumberOfSamples(distribution));

        VERIFY_ARE_EQUAL(10000u, distribution[1]);
    }

    BOOST_AUTO_TEST_CASE(UniformDistributionTest)
    {
        RandomDistribution rd(seed_);
        DistributionMap distribution =
            rd.GenerateDistribution(10000, RandomDistribution::Enum::Uniform, -50, 49);

        VERIFY_ARE_EQUAL(100u, distribution.size());
        VERIFY_ARE_EQUAL(10000u, rd.GetNumberOfSamples(distribution));

        for (int i = -50; i <= 49; i++)
        {
            VERIFY_ARE_EQUAL(100u, distribution[i]);
        }
    }

    BOOST_AUTO_TEST_CASE(ExponentialDistributionTest)
    {
        RandomDistribution rd(seed_);
        DistributionMap distribution =
            rd.GenerateDistribution(10000, RandomDistribution::Enum::Exponential, -50, 50);

        VERIFY_ARE_EQUAL(10000u, rd.GetNumberOfSamples(distribution));
    }

    BOOST_AUTO_TEST_CASE(GaussianDistributionTest)
    {
        RandomDistribution rd(seed_);
        DistributionMap distribution =
            rd.GenerateDistribution(10000, RandomDistribution::Enum::Gaussian, -50, 50);

        VERIFY_ARE_EQUAL(10000u, rd.GetNumberOfSamples(distribution));
    }

    BOOST_AUTO_TEST_CASE(RandomSeedTest)
    {
        RandomDistribution rd1(100);
        RandomDistribution rd2(200);
        RandomDistribution rd3(200);

        DistributionMap d1_g = rd1.GenerateDistribution(10000, RandomDistribution::Enum::Gaussian, -50, 49);
        DistributionMap d2_g = rd2.GenerateDistribution(10000, RandomDistribution::Enum::Gaussian, -50, 49);
        DistributionMap d3_g = rd3.GenerateDistribution(10000, RandomDistribution::Enum::Gaussian, -50, 49);

        VERIFY_ARE_EQUAL(d2_g, d3_g);
        VERIFY_IS_TRUE(d1_g != d2_g);
        VERIFY_IS_TRUE(d1_g != d3_g);

        DistributionMap d1_e = rd1.GenerateDistribution(10000, RandomDistribution::Enum::Exponential, -50, 49);
        DistributionMap d2_e = rd2.GenerateDistribution(10000, RandomDistribution::Enum::Exponential, -50, 49);
        DistributionMap d3_e = rd3.GenerateDistribution(10000, RandomDistribution::Enum::Exponential, -50, 49);

        VERIFY_ARE_EQUAL(d2_e, d3_e);
        VERIFY_IS_TRUE(d1_e != d2_e);
        VERIFY_IS_TRUE(d1_e != d3_e);
    }

    BOOST_AUTO_TEST_SUITE_END()

    bool TestRandomDistribution::ClassSetup()
    {
        seed_ = 500;
        return TRUE;
    }
}
