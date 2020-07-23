// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace Naming
{
    using namespace std;
    using namespace Common;
    using namespace Reliability;
    using namespace ServiceModel;

    StringLiteral const TestSource("PartitionedServiceDescriptorTest");    

#define VERIFY(condition, ...) VERIFY_IS_TRUE(condition, wformatString(__VA_ARGS__).c_str());

#define GET_RC( ResourceName ) \
    Common::StringResource::Get( IDS_NAMING_##ResourceName ) \

    class PartitionedServiceDescriptorTest
    {
    protected:
        PartitionedServiceDescriptorTest() { BOOST_REQUIRE(ClassSetup()); }
        TEST_CLASS_SETUP(ClassSetup)

        ServiceDescription CreateServiceDescription(std::wstring const & name, int partitionCount);
        void PartitionedServiceDescriptorTest::CreateServiceGroupDescription(CServiceGroupDescription & description, bool hasPersistedState, size_t memberCount, size_t metricCount);

        wstring placementConstraints_;
        int scaleoutCount_;
        vector<Reliability::ServiceLoadMetricDescription> metrics_;
    };


    BOOST_FIXTURE_TEST_SUITE(PartitionedServiceDescriptorTestSuite,PartitionedServiceDescriptorTest)

    BOOST_AUTO_TEST_CASE(TestServiceContains_UniformInt64)
    {
        Trace.WriteInfo(TestSource, "*** TestServiceContains_UniformInt64");
        auto description = CreateServiceDescription(L"fabric:/Belltown", 4);

        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, 20, 100, psd).IsSuccess());
        VERIFY_IS_TRUE(psd.ServiceContains(PartitionKey(50)));
        VERIFY_IS_TRUE(psd.ServiceContains(PartitionKey(100)));
        VERIFY_IS_TRUE(psd.ServiceContains(PartitionKey(20)));
        VERIFY_IS_FALSE(psd.ServiceContains(PartitionKey(19)));
        VERIFY_IS_FALSE(psd.ServiceContains(PartitionKey(101)));

        PartitionedServiceDescriptor psd2;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, -100, -20, psd2).IsSuccess());
        VERIFY_IS_TRUE(psd2.ServiceContains(PartitionKey(-100)));
        VERIFY_IS_TRUE(psd2.ServiceContains(PartitionKey(-20)));
        VERIFY_IS_TRUE(psd2.ServiceContains(PartitionKey(-21)));
        VERIFY_IS_TRUE(psd2.ServiceContains(PartitionKey(-99)));
        VERIFY_IS_FALSE(psd2.ServiceContains(PartitionKey(-101)));
        VERIFY_IS_FALSE(psd2.ServiceContains(PartitionKey(-19)));

        // type mismatch
        VERIFY_IS_FALSE(psd.ServiceContains(PartitionKey()));
        VERIFY_IS_FALSE(psd.ServiceContains(PartitionKey(L"NamedPartition")));
    }

    BOOST_AUTO_TEST_CASE(TestServiceContains_Singleton)
    {
        Trace.WriteInfo(TestSource, "*** TestServiceContains_Singleton");
        auto description = CreateServiceDescription(L"fabric:/Georgetown", 1);

        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, psd).IsSuccess());
        VERIFY_IS_TRUE(psd.ServiceContains(PartitionKey()));

        // type mismatch
        VERIFY_IS_FALSE(psd.ServiceContains(PartitionKey(100)));
        VERIFY_IS_FALSE(psd.ServiceContains(PartitionKey(L"NamedPartition")));
    }

    BOOST_AUTO_TEST_CASE(TestServiceContains_Named)
    {
        Trace.WriteInfo(TestSource, "*** TestServiceContains_Named");
        auto description = CreateServiceDescription(L"fabric:/Eastlake", 3);
        vector<wstring> partitionNames;
        partitionNames.push_back(L"Roanoke");
        partitionNames.push_back(L"Yale");
        partitionNames.push_back(L"Fairview");

        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, partitionNames, psd).IsSuccess());
        VERIFY_IS_TRUE(psd.ServiceContains(PartitionKey(L"Roanoke")));
        VERIFY_IS_TRUE(psd.ServiceContains(PartitionKey(L"Yale")));
        VERIFY_IS_TRUE(psd.ServiceContains(PartitionKey(L"Fairview")));

        // type mismatch
        VERIFY_IS_FALSE(psd.ServiceContains(PartitionKey(100)));
        VERIFY_IS_FALSE(psd.ServiceContains(PartitionKey()));
    }
        
    BOOST_AUTO_TEST_CASE(TestGetCuid_UniformInt64)
    {
        auto description = CreateServiceDescription(L"fabric:/QueenAnne", 4);

        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, 20, 100, psd).IsSuccess());
        vector<ConsistencyUnitId> cuids = psd.Cuids;
        ConsistencyUnitId cuid;
        VERIFY_IS_TRUE(psd.TryGetCuid(PartitionKey(20), cuid) && cuids[0] == cuid);
        VERIFY_IS_TRUE(psd.TryGetCuid(PartitionKey(21), cuid) && cuids[0] == cuid);
        VERIFY_IS_TRUE(psd.TryGetCuid(PartitionKey(28), cuid) && cuids[0] == cuid);
        VERIFY_IS_TRUE(psd.TryGetCuid(PartitionKey(39), cuid) && cuids[0] == cuid);
        VERIFY_IS_TRUE(psd.TryGetCuid(PartitionKey(40), cuid) && cuids[0] == cuid);
        VERIFY_IS_TRUE(psd.TryGetCuid(PartitionKey(41), cuid) && cuids[1] == cuid);
        VERIFY_IS_TRUE(psd.TryGetCuid(PartitionKey(59), cuid) && cuids[1] == cuid);
        VERIFY_IS_TRUE(psd.TryGetCuid(PartitionKey(60), cuid) && cuids[1] == cuid);
        VERIFY_IS_TRUE(psd.TryGetCuid(PartitionKey(61), cuid) && cuids[2] == cuid);
        VERIFY_IS_TRUE(psd.TryGetCuid(PartitionKey(66), cuid) && cuids[2] == cuid);
        VERIFY_IS_TRUE(psd.TryGetCuid(PartitionKey(79), cuid) && cuids[2] == cuid);
        VERIFY_IS_TRUE(psd.TryGetCuid(PartitionKey(80), cuid) && cuids[2] == cuid);
        VERIFY_IS_TRUE(psd.TryGetCuid(PartitionKey(81), cuid) && cuids[3] == cuid);
        VERIFY_IS_TRUE(psd.TryGetCuid(PartitionKey(90), cuid) && cuids[3] == cuid);
        VERIFY_IS_TRUE(psd.TryGetCuid(PartitionKey(99), cuid) && cuids[3] == cuid);
        VERIFY_IS_TRUE(psd.TryGetCuid(PartitionKey(100), cuid) && cuids[3] == cuid);

        PartitionedServiceDescriptor psd2;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, -100, -20, psd2).IsSuccess());
        vector<ConsistencyUnitId> cuids2 = psd2.Cuids;
        VERIFY_IS_TRUE(psd2.TryGetCuid(PartitionKey(-100), cuid) && cuids2[0] == cuid);
        VERIFY_IS_TRUE(psd2.TryGetCuid(PartitionKey(-80), cuid) && cuids2[0] == cuid);
        VERIFY_IS_TRUE(psd2.TryGetCuid(PartitionKey(-79), cuid) && cuids2[1] == cuid);
        VERIFY_IS_TRUE(psd2.TryGetCuid(PartitionKey(-60), cuid) && cuids2[1] == cuid);
        VERIFY_IS_TRUE(psd2.TryGetCuid(PartitionKey(-59), cuid) && cuids2[2] == cuid);
        VERIFY_IS_TRUE(psd2.TryGetCuid(PartitionKey(-40), cuid) && cuids2[2] == cuid);
        VERIFY_IS_TRUE(psd2.TryGetCuid(PartitionKey(-39), cuid) && cuids2[3] == cuid);
        VERIFY_IS_TRUE(psd2.TryGetCuid(PartitionKey(-20), cuid) && cuids2[3] == cuid);
    }
    
    BOOST_AUTO_TEST_CASE(TestGetCuid_Singleton)
    {
        auto description = CreateServiceDescription(L"fabric:/Interbay", 1);

        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, psd).IsSuccess());
        vector<ConsistencyUnitId> cuids = psd.Cuids;
        ConsistencyUnitId cuid;
        VERIFY_ARE_EQUAL(1U, cuids.size());
        VERIFY_IS_TRUE(psd.TryGetCuid(PartitionKey(), cuid));
        VERIFY_ARE_EQUAL(cuids[0], cuid);
    }

    BOOST_AUTO_TEST_CASE(TestGetCuid_Named)
    {
        auto description = CreateServiceDescription(L"fabric:/Wallingford", 3);
        vector<wstring> partitionNames;
        partitionNames.push_back(L"Stone");
        partitionNames.push_back(L"45th");
        partitionNames.push_back(L"Meridian");

        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, partitionNames, psd).IsSuccess());
        vector<ConsistencyUnitId> cuids = psd.Cuids;
        VERIFY_ARE_EQUAL(3U, cuids.size());

        ConsistencyUnitId cuid;
        VERIFY_IS_TRUE(psd.TryGetCuid(PartitionKey(L"Stone"), cuid) && cuids[0] == cuid);
        VERIFY_IS_TRUE(psd.TryGetCuid(PartitionKey(L"45th"), cuid) && cuids[1] == cuid);
        VERIFY_IS_TRUE(psd.TryGetCuid(PartitionKey(L"Meridian"), cuid) && cuids[2] == cuid);
    }

    BOOST_AUTO_TEST_CASE(TestGetCuidUnevenPartitions)
    {
        auto description = CreateServiceDescription(L"fabric:/CapitolHill", 7);

        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, 10, 20, psd).IsSuccess());
        vector<ConsistencyUnitId> cuids = psd.Cuids;
        ConsistencyUnitId cuid;
        VERIFY_IS_TRUE(psd.TryGetCuid(PartitionKey(10), cuid) && cuids[0] == cuid);
        VERIFY_IS_TRUE(psd.TryGetCuid(PartitionKey(11), cuid) && cuids[0] == cuid);
        VERIFY_IS_TRUE(psd.TryGetCuid(PartitionKey(12), cuid) && cuids[1] == cuid);
        VERIFY_IS_TRUE(psd.TryGetCuid(PartitionKey(13), cuid) && cuids[1] == cuid);
        VERIFY_IS_TRUE(psd.TryGetCuid(PartitionKey(14), cuid) && cuids[2] == cuid);
        VERIFY_IS_TRUE(psd.TryGetCuid(PartitionKey(15), cuid) && cuids[2] == cuid);
        VERIFY_IS_TRUE(psd.TryGetCuid(PartitionKey(16), cuid) && cuids[3] == cuid);
        VERIFY_IS_TRUE(psd.TryGetCuid(PartitionKey(17), cuid) && cuids[3] == cuid);
        VERIFY_IS_TRUE(psd.TryGetCuid(PartitionKey(18), cuid) && cuids[4] == cuid);
        VERIFY_IS_TRUE(psd.TryGetCuid(PartitionKey(19), cuid) && cuids[5] == cuid);
        VERIFY_IS_TRUE(psd.TryGetCuid(PartitionKey(20), cuid) && cuids[6] == cuid);

        PartitionedServiceDescriptor psd2;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, -20, -10, psd2).IsSuccess());
        vector<ConsistencyUnitId> cuids2 = psd2.Cuids;
        VERIFY_IS_TRUE(psd2.TryGetCuid(PartitionKey(-20), cuid) && cuids2[0] == cuid);
        VERIFY_IS_TRUE(psd2.TryGetCuid(PartitionKey(-19), cuid) && cuids2[0] == cuid);
        VERIFY_IS_TRUE(psd2.TryGetCuid(PartitionKey(-18), cuid) && cuids2[1] == cuid);
        VERIFY_IS_TRUE(psd2.TryGetCuid(PartitionKey(-17), cuid) && cuids2[1] == cuid);
        VERIFY_IS_TRUE(psd2.TryGetCuid(PartitionKey(-16), cuid) && cuids2[2] == cuid);
        VERIFY_IS_TRUE(psd2.TryGetCuid(PartitionKey(-15), cuid) && cuids2[2] == cuid);
        VERIFY_IS_TRUE(psd2.TryGetCuid(PartitionKey(-14), cuid) && cuids2[3] == cuid);
        VERIFY_IS_TRUE(psd2.TryGetCuid(PartitionKey(-13), cuid) && cuids2[3] == cuid);
        VERIFY_IS_TRUE(psd2.TryGetCuid(PartitionKey(-12), cuid) && cuids2[4] == cuid);
        VERIFY_IS_TRUE(psd2.TryGetCuid(PartitionKey(-11), cuid) && cuids2[5] == cuid);
        VERIFY_IS_TRUE(psd2.TryGetCuid(PartitionKey(-10), cuid) && cuids2[6] == cuid);
    }

    BOOST_AUTO_TEST_CASE(TestGetCuidOneTinyPartition)
    {
        auto description = CreateServiceDescription(L"fabric:/Magnolia", 1);
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, 7, 7, psd).IsSuccess());
        vector<ConsistencyUnitId> cuids = psd.Cuids;
        ConsistencyUnitId cuid;
        VERIFY_IS_TRUE(psd.TryGetCuid(PartitionKey(7), cuid) && cuids[0] == cuid);
    }

    BOOST_AUTO_TEST_CASE(TestGetCuidError)
    {
        auto description = CreateServiceDescription(L"fabric:/Ballard", 4);
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, 20, 100, psd).IsSuccess());
        
        ConsistencyUnitId cuid;
        VERIFY_IS_FALSE(psd.TryGetCuid(PartitionKey(19), cuid));
        VERIFY_IS_FALSE(psd.TryGetCuid(PartitionKey(101), cuid));

        PartitionedServiceDescriptor psd2;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, -100, -20, psd2).IsSuccess());
        VERIFY_IS_FALSE(psd.TryGetCuid(PartitionKey(-19), cuid));
        VERIFY_IS_FALSE(psd.TryGetCuid(PartitionKey(-101), cuid));
    }    

    BOOST_AUTO_TEST_CASE(TestGetCuidTypeMismatch)
    {
        auto description = CreateServiceDescription(L"fabric:/Montlake", 4);
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, 0, 100, psd).IsSuccess());
        
        ConsistencyUnitId cuid;
        VERIFY_IS_FALSE(psd.TryGetCuid(PartitionKey(), cuid));
        VERIFY_IS_FALSE(psd.TryGetCuid(PartitionKey(L"Broadway"), cuid));
        
        auto description2 = CreateServiceDescription(L"fabric:/Madrona", 1);
        PartitionedServiceDescriptor psd2;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description2, psd2).IsSuccess());
        
        VERIFY_IS_FALSE(psd2.TryGetCuid(PartitionKey(0), cuid));
        VERIFY_IS_FALSE(psd2.TryGetCuid(PartitionKey(L"Denny"), cuid));
        
        auto description3 = CreateServiceDescription(L"fabric:/Broadview", 3);
        vector<wstring> partitionNames;
        partitionNames.push_back(L"Greenwood");
        partitionNames.push_back(L"120th");
        partitionNames.push_back(L"Westminster");
        PartitionedServiceDescriptor psd3;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description3, partitionNames, psd3).IsSuccess());
        
        VERIFY_IS_FALSE(psd3.TryGetCuid(PartitionKey(), cuid));
        VERIFY_IS_FALSE(psd3.TryGetCuid(PartitionKey(0), cuid));
    }    

    BOOST_AUTO_TEST_CASE(TestGetPartitionInfo_UniformInt64)
    {
        auto description = CreateServiceDescription(L"fabric:/SoDo", 4);

        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, 20, 100, psd).IsSuccess());
        vector<ConsistencyUnitId> cuids = psd.Cuids;
        VERIFY_ARE_EQUAL(PartitionInfo(20, 40), psd.GetPartitionInfo(cuids[0]));
        VERIFY_ARE_EQUAL(PartitionInfo(41, 60), psd.GetPartitionInfo(cuids[1]));
        VERIFY_ARE_EQUAL(PartitionInfo(61, 80), psd.GetPartitionInfo(cuids[2]));
        VERIFY_ARE_EQUAL(PartitionInfo(81, 100), psd.GetPartitionInfo(cuids[3]));

        PartitionedServiceDescriptor psd2;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, -100, -20, psd2).IsSuccess());
        vector<ConsistencyUnitId> cuids2 = psd2.Cuids;
        VERIFY_ARE_EQUAL(PartitionInfo(-100, -80), psd2.GetPartitionInfo(cuids2[0]));
        VERIFY_ARE_EQUAL(PartitionInfo(-79, -60), psd2.GetPartitionInfo(cuids2[1]));
        VERIFY_ARE_EQUAL(PartitionInfo(-59, -40), psd2.GetPartitionInfo(cuids2[2]));
        VERIFY_ARE_EQUAL(PartitionInfo(-39, -20), psd2.GetPartitionInfo(cuids2[3]));
    }

    BOOST_AUTO_TEST_CASE(TestGetPartitionInfo_Singleton)
    {
        auto description = CreateServiceDescription(L"fabric:/SewardPark", 1);

        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, psd).IsSuccess());
        vector<ConsistencyUnitId> cuids = psd.Cuids;
        VERIFY_ARE_EQUAL(PartitionInfo(), psd.GetPartitionInfo(cuids[0]));
        VERIFY_ARE_EQUAL(PartitionInfo(FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_SINGLETON), psd.GetPartitionInfo(cuids[0]));
    }

    BOOST_AUTO_TEST_CASE(TestGetPartitionInfo_Named)
    {
        auto description = CreateServiceDescription(L"fabric:/BeaconHill", 3);
        vector<wstring> partitionNames;
        partitionNames.push_back(L"McClellan");
        partitionNames.push_back(L"Spokane");
        partitionNames.push_back(L"Graham");
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, partitionNames, psd).IsSuccess());

        vector<ConsistencyUnitId> cuids = psd.Cuids;
        VERIFY_ARE_EQUAL(PartitionInfo(L"McClellan"), psd.GetPartitionInfo(cuids[0]));
        VERIFY_ARE_EQUAL(PartitionInfo(L"Spokane"), psd.GetPartitionInfo(cuids[1]));
        VERIFY_ARE_EQUAL(PartitionInfo(L"Graham"), psd.GetPartitionInfo(cuids[2]));
    }

    BOOST_AUTO_TEST_CASE(TestGetPartitionInfoUneven)
    {
        auto description = CreateServiceDescription(L"fabric:/UDistrict", 7);

        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, 10, 20, psd).IsSuccess());
        vector<ConsistencyUnitId> cuids = psd.Cuids;
        VERIFY_ARE_EQUAL(PartitionInfo(10, 11), psd.GetPartitionInfo(cuids[0]));
        VERIFY_ARE_EQUAL(PartitionInfo(12, 13), psd.GetPartitionInfo(cuids[1]));
        VERIFY_ARE_EQUAL(PartitionInfo(14, 15), psd.GetPartitionInfo(cuids[2]));
        VERIFY_ARE_EQUAL(PartitionInfo(16, 17), psd.GetPartitionInfo(cuids[3]));
        VERIFY_ARE_EQUAL(PartitionInfo(18, 18), psd.GetPartitionInfo(cuids[4]));
        VERIFY_ARE_EQUAL(PartitionInfo(19, 19), psd.GetPartitionInfo(cuids[5]));
        VERIFY_ARE_EQUAL(PartitionInfo(20, 20), psd.GetPartitionInfo(cuids[6]));

        PartitionedServiceDescriptor psd2;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, -20, -10, psd2).IsSuccess());
        vector<ConsistencyUnitId> cuids2 = psd2.Cuids;
        VERIFY_ARE_EQUAL(PartitionInfo(-20, -19), psd2.GetPartitionInfo(cuids2[0]));
        VERIFY_ARE_EQUAL(PartitionInfo(-18, -17), psd2.GetPartitionInfo(cuids2[1]));
        VERIFY_ARE_EQUAL(PartitionInfo(-16, -15), psd2.GetPartitionInfo(cuids2[2]));
        VERIFY_ARE_EQUAL(PartitionInfo(-14, -13), psd2.GetPartitionInfo(cuids2[3]));
        VERIFY_ARE_EQUAL(PartitionInfo(-12, -12), psd2.GetPartitionInfo(cuids2[4]));
        VERIFY_ARE_EQUAL(PartitionInfo(-11, -11), psd2.GetPartitionInfo(cuids2[5]));
        VERIFY_ARE_EQUAL(PartitionInfo(-10, -10), psd2.GetPartitionInfo(cuids2[6]));
    }

    BOOST_AUTO_TEST_CASE(TestGetVeryEvenPartitionInfo)
    {
        auto description = CreateServiceDescription(L"fabric:/Northgate", 5);
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, 50, 99, psd).IsSuccess());
        vector<ConsistencyUnitId> cuids = psd.Cuids;

        VERIFY_ARE_EQUAL(PartitionInfo(50, 59), psd.GetPartitionInfo(cuids[0]));
        VERIFY_ARE_EQUAL(PartitionInfo(60, 69), psd.GetPartitionInfo(cuids[1]));
        VERIFY_ARE_EQUAL(PartitionInfo(70, 79), psd.GetPartitionInfo(cuids[2]));
        VERIFY_ARE_EQUAL(PartitionInfo(80, 89), psd.GetPartitionInfo(cuids[3]));
        VERIFY_ARE_EQUAL(PartitionInfo(90, 99), psd.GetPartitionInfo(cuids[4]));
    }

    BOOST_AUTO_TEST_CASE(TestGetPartitionInfoOneTinyPartition)
    {
        auto description = CreateServiceDescription(L"fabric:/GreenLake", 1);
        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, 21, 21, psd).IsSuccess());
        VERIFY_ARE_EQUAL(PartitionInfo(21, 21), psd.GetPartitionInfo(psd.Cuids[0]));
    }

    BOOST_AUTO_TEST_CASE(TestCaseSensitivity)
    {
        auto description = CreateServiceDescription(L"fabric:/Interlaken", 1);
        vector<wstring> partitionNames;
        partitionNames.push_back(L"Boyer");

        PartitionedServiceDescriptor psd;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, partitionNames, psd).IsSuccess());
        vector<ConsistencyUnitId> cuids = psd.Cuids;
        VERIFY_ARE_EQUAL(1U, cuids.size());

        ConsistencyUnitId cuid;
        VERIFY_IS_TRUE(psd.TryGetCuid(PartitionKey(L"Boyer"), cuid) && cuids[0] == cuid);
        VERIFY_IS_FALSE(psd.TryGetCuid(PartitionKey(L"boyer"), cuid));
        VERIFY_IS_FALSE(psd.TryGetCuid(PartitionKey(L"BOYER"), cuid));
    }

    BOOST_AUTO_TEST_CASE(TestCreateFromDescriptions)
    {
        Trace.WriteInfo(TestSource, "*** TestCreateFromDescriptions");

        auto description = CreateServiceDescription(L"fabric:/TestCreateFromDescriptions", 20);

        PartitionedServiceDescriptor psd1;
        VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, 0, 199, psd1).IsSuccess());

        vector<ConsistencyUnitId> cuids = psd1.Cuids;

        vector<ConsistencyUnitDescription> cuDescriptions;
        for (auto iter = cuids.begin(); iter != cuids.end(); ++iter)
        {
            auto info = psd1.GetPartitionInfo(*iter);
            cuDescriptions.push_back(ConsistencyUnitDescription(*iter, info.LowKey, info.HighKey));
        }

        std::random_shuffle(cuDescriptions.begin(), cuDescriptions.end());

        Trace.WriteInfo(TestSource, "Original: {0}", cuids);
        Trace.WriteInfo(TestSource, "Shuffled: {0}", cuDescriptions);

        PartitionedServiceDescriptor psd2;
        ErrorCode error = PartitionedServiceDescriptor::Create(description, cuDescriptions, psd2);
        VERIFY_IS_TRUE(error.IsSuccess());

        Trace.WriteInfo(TestSource, "psd1: {0}", psd1);
        Trace.WriteInfo(TestSource, "psd2: {0}", psd2);

        VERIFY_IS_TRUE(psd1 == psd2);

        vector<ConsistencyUnitId> const & cuids2 = psd2.Cuids;

        bool cuidsMatched = true;
        for (size_t ix = 0; ix < cuids.size(); ++ix)
        {
            if (cuids[ix] != cuids2[ix])
            {
                Trace.WriteWarning(TestSource, "CUID mismatch at {0}: {1}, {2}", ix, cuids[ix], cuids2[ix]);
                cuidsMatched = false;
                break;
            }
        }
        VERIFY_IS_TRUE(cuidsMatched);
    }

    BOOST_AUTO_TEST_CASE(TestValidations)
    {
        
        Trace.WriteInfo(TestSource, "*** TestValidations");

        auto description = CreateServiceDescription(L"fabric:/TestValidations", 1);

        ErrorCode error;
        PartitionedServiceDescriptor psd;

        vector<Reliability::ServiceLoadMetricDescription> metrics;
        metrics.push_back(Reliability::ServiceLoadMetricDescription());
        description.Metrics = move(metrics);
        VERIFY((error = PartitionedServiceDescriptor::Create(description, 0, 0, psd)).IsError(ErrorCodeValue::InvalidArgument), "{0}", psd);
        VERIFY(StringUtility::StartsWith(error.Message, GET_RC( Invalid_Metric )), "{0}", error.Message);

        metrics.clear();
        metrics.push_back(Reliability::ServiceLoadMetricDescription(L"MyMetric", static_cast<FABRIC_SERVICE_LOAD_METRIC_WEIGHT>(FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH + 1), 0u, 0u));
        description.Metrics = move(metrics);
        VERIFY((error = PartitionedServiceDescriptor::Create(description, 0, 0, psd)).IsError(ErrorCodeValue::InvalidArgument), "{0}", psd);
        VERIFY(StringUtility::StartsWith(error.Message, GET_RC( Invalid_Metric_Range )), "{0}", error.Message);

        metrics.clear();
        metrics.push_back(Reliability::ServiceLoadMetricDescription(L"MyMetric", static_cast<FABRIC_SERVICE_LOAD_METRIC_WEIGHT>(FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH), 0u, 1u));
        description.Metrics = move(metrics);
        VERIFY((error = PartitionedServiceDescriptor::Create(description, 0, 0, psd)).IsError(ErrorCodeValue::InvalidArgument), "{0}", psd);
        VERIFY(StringUtility::StartsWith(error.Message, GET_RC( Invalid_Metric_Load )), "{0}", error.Message);

        description.Metrics = vector<Reliability::ServiceLoadMetricDescription>();

        vector<Reliability::ServiceCorrelationDescription> correlations;
        correlations.resize(2);
        description.Correlations = move(correlations);
        VERIFY((error = PartitionedServiceDescriptor::Create(description, 0, 0, psd)).IsError(ErrorCodeValue::InvalidArgument), "{0}", psd);
        VERIFY(StringUtility::StartsWith(error.Message, GET_RC( Invalid_Affinity_Count )), "{0}", error.Message);

        correlations.push_back(Reliability::ServiceCorrelationDescription(L"fabric:/OtherService", static_cast<FABRIC_SERVICE_CORRELATION_SCHEME>(FABRIC_SERVICE_CORRELATION_SCHEME_NONALIGNED_AFFINITY + 1)));
        description.Correlations = move(correlations);
        VERIFY((error = PartitionedServiceDescriptor::Create(description, 0, 0, psd)).IsError(ErrorCodeValue::InvalidArgument), "{0}", psd);
        VERIFY(StringUtility::StartsWith(error.Message, GET_RC( Invalid_Correlation_Scheme )), "{0}", error.Message);

        correlations.resize(1);
        description.Correlations = move(correlations);
        VERIFY((error = PartitionedServiceDescriptor::Create(description, 0, 0, psd)).IsError(ErrorCodeValue::InvalidArgument), "{0}", psd);
        VERIFY(StringUtility::StartsWith(error.Message, GET_RC( Invalid_Correlation_Service )), "{0}", error.Message);

        correlations.push_back(Reliability::ServiceCorrelationDescription(L"fabric:/OtherService", FABRIC_SERVICE_CORRELATION_SCHEME_AFFINITY));
        description.Correlations = move(correlations);
        description.TargetReplicaSetSize = -1;
        VERIFY((error = PartitionedServiceDescriptor::Create(description, 0, 0, psd)).IsError(ErrorCodeValue::InvalidArgument), "{0}", psd);
        VERIFY(StringUtility::StartsWith(error.Message, GET_RC( Invalid_Correlation )), "{0}", error.Message);

        description.Correlations = vector<Reliability::ServiceCorrelationDescription>();
        description.TargetReplicaSetSize = 1;

        description.ApplicationName = L"fabric:/app";
        description.Type = ServiceTypeIdentifier(ServicePackageIdentifier(), L"Type");
        VERIFY((error = PartitionedServiceDescriptor::Create(description, 0, 0, psd)).IsSuccess(), "{0}", psd);

        description.ApplicationName = L"";
        description.Type = ServiceTypeIdentifier(ServicePackageIdentifier(ApplicationIdentifier(L"AppType", 0), L"Package"), L"Type");
        VERIFY((error = PartitionedServiceDescriptor::Create(description, 0, 0, psd)).IsError(ErrorCodeValue::InvalidArgument), "{0}", psd);
        VERIFY(StringUtility::StartsWith(error.Message, GET_RC( Missing_Application_Name )), "{0}", error.Message);

        description.ApplicationName = L"";
        description.Type = ServiceTypeIdentifier(ServicePackageIdentifier(), L"Type");

        description.PlacementConstraints = L"&&||asdf";
        VERIFY((error = PartitionedServiceDescriptor::Create(description, 0, 0, psd)).IsError(ErrorCodeValue::InvalidArgument), "{0}", psd);
        VERIFY(StringUtility::StartsWith(error.Message, GET_RC( Invalid_Placement )), "{0}", error.Message);

        description.PlacementConstraints = L"";

        description.PartitionCount = 0;
        VERIFY((error = PartitionedServiceDescriptor::Create(description, 0, 0, psd)).IsError(ErrorCodeValue::InvalidArgument), "{0}", psd);
        VERIFY(StringUtility::StartsWith(error.Message, GET_RC( Invalid_Partition_Count )), "{0}", error.Message);

        description.PartitionCount = 1;

        description.TargetReplicaSetSize = 0;
        VERIFY((error = PartitionedServiceDescriptor::Create(description, 0, 0, psd)).IsError(ErrorCodeValue::InvalidArgument), "{0}", psd);
        VERIFY(StringUtility::StartsWith(error.Message, GET_RC( Invalid_Target_Replicas )), "{0}", error.Message);


        description.PartitionCount = 1;
        description.TargetReplicaSetSize = 1;
        VERIFY((error = PartitionedServiceDescriptor::Create(description, 0, 0, psd)).IsSuccess(), "{0}", psd);

        psd.Test_SetPartitionScheme(FABRIC_PARTITION_SCHEME_SINGLETON);
        psd.MutableService.PartitionCount = 2;
        VERIFY((error = psd.ValidateSkipName()).IsError(ErrorCodeValue::InvalidArgument), "{0}", psd);
        VERIFY(StringUtility::StartsWith(error.Message, GET_RC( Invalid_Singleton )), "{0}", error.Message);

        psd.Test_SetPartitionScheme(FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE);
        psd.Test_SetLowRange(1);
        psd.Test_SetHighRange(0);
        VERIFY((error = psd.ValidateSkipName()).IsError(ErrorCodeValue::InvalidArgument), "{0}", psd);
        VERIFY(StringUtility::StartsWith(error.Message, GET_RC( Invalid_Range )), "{0}", error.Message);

        psd.MutableService.PartitionCount = 3;
        psd.Test_SetLowRange(0);
        psd.Test_SetHighRange(1);
        VERIFY((error = psd.ValidateSkipName()).IsError(ErrorCodeValue::InvalidArgument), "{0}", psd);
        VERIFY(StringUtility::StartsWith(error.Message, GET_RC( Invalid_Range_Count )), "{0}", error.Message);

        psd.Test_SetPartitionScheme(FABRIC_PARTITION_SCHEME_NAMED);
        psd.MutableService.PartitionCount = 1;
        psd.Test_SetNames(vector<wstring>());
        VERIFY((error = psd.ValidateSkipName()).IsError(ErrorCodeValue::InvalidArgument), "{0}", psd);
        VERIFY(StringUtility::StartsWith(error.Message, GET_RC( Invalid_Names_Count )), "{0}", error.Message);

        vector<wstring> partitionNames;
        partitionNames.push_back(L"");
        auto expectedErrorMsg = wformatString(GET_RC( Invalid_Partition_Names ), partitionNames);
        psd.Test_SetNames(move(partitionNames));
        VERIFY((error = psd.ValidateSkipName()).IsError(ErrorCodeValue::InvalidArgument), "{0}", psd);
        VERIFY(error.Message == expectedErrorMsg, "{0}", error.Message);

        partitionNames.push_back(L"pName");
        partitionNames.push_back(L"pName");
        expectedErrorMsg = wformatString(GET_RC( Invalid_Partition_Names ), partitionNames);
        psd.MutableService.PartitionCount = 2;
        psd.Test_SetNames(move(partitionNames));
        VERIFY((error = psd.ValidateSkipName()).IsError(ErrorCodeValue::InvalidArgument), "{0}", psd);
        VERIFY(error.Message == expectedErrorMsg, "{0}", error.Message);

        psd.Test_SetPartitionScheme(FABRIC_PARTITION_SCHEME_SINGLETON);
        psd.MutableService.PartitionCount = 1;
        psd.Test_SetLowRange(0);
        psd.Test_SetHighRange(0);

        psd.MutableService.IsStateful = true;
        psd.MutableService.MinReplicaSetSize = 0;
        VERIFY((error = psd.ValidateSkipName()).IsError(ErrorCodeValue::InvalidArgument), "{0}", psd);
        VERIFY(StringUtility::StartsWith(error.Message, GET_RC( Invalid_Minimum_Replicas )), "{0}", error.Message);

        psd.MutableService.TargetReplicaSetSize = 3;
        psd.MutableService.MinReplicaSetSize = 4;
        VERIFY((error = psd.ValidateSkipName()).IsError(ErrorCodeValue::InvalidArgument), "{0}", psd);
        VERIFY(StringUtility::StartsWith(error.Message, GET_RC( Invalid_Minimum_Target )), "{0}", error.Message);
    }

    BOOST_AUTO_TEST_CASE(TestGetAndRangeInLoop)
    {
        // this test ensures the return values of GetCuid and
        // GetPartitionInfo are consistent with respect to each other
        
        for (int partitionCount = 1; partitionCount <= 100; ++partitionCount)
        {
            auto description = CreateServiceDescription(L"fabric:/Greenwood", partitionCount);
            for (__int64 low = 0; low <= 1; ++low)
            {
                for (__int64 high = low + partitionCount - 1; high <= partitionCount + 100U; ++high)
                {
                    PartitionedServiceDescriptor psd;
                    VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, low, high, psd).IsSuccess());
                    for (__int64 toTest = low; toTest <= high; ++toTest)
                    {
                        ConsistencyUnitId cuid;
                        bool success = psd.TryGetCuid(PartitionKey(toTest), cuid);

                        // Only print verification failures
                        if (!success)
                        {
                            VERIFY_IS_TRUE(psd.TryGetCuid(PartitionKey(toTest), cuid));
                        }

                        PartitionInfo range = psd.GetPartitionInfo(cuid);

                        // Only print verification failures
                        if (!range.RangeContains(PartitionKey(toTest)))
                        {
                            VERIFY_IS_TRUE(range.RangeContains(PartitionKey(toTest)));
                        }
                    }

                    __int64 negativeLow = low - 150;
                    __int64 negativeHigh = high - 150;

                    PartitionedServiceDescriptor psd2;
                    VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, negativeLow, negativeHigh, psd2).IsSuccess());
                    for (__int64 toTest = negativeLow; toTest <= negativeHigh; ++toTest)
                    {
                        ConsistencyUnitId cuid;
                        bool success = psd2.TryGetCuid(PartitionKey(toTest), cuid);
                        
                        // Only print verification failures
                        if (!success)
                        {
                            VERIFY_IS_TRUE(psd2.TryGetCuid(PartitionKey(toTest), cuid));
                        }
                        PartitionInfo range = psd2.GetPartitionInfo(cuid);

                        // Only print verification failures
                        if (!range.RangeContains(PartitionKey(toTest)))
                        {
                            VERIFY_IS_TRUE(range.RangeContains(PartitionKey(toTest)));
                        }
                    }
                }
            }
        }
    }

    BOOST_AUTO_TEST_CASE(TestCompareWithServiceGroup)
    {        
        // check equality
        for (size_t count = 1; count < 4; ++count)
        {
            CServiceGroupDescription first;
            CServiceGroupDescription second;

            CreateServiceGroupDescription(first, true, count, 2 * count);
            CreateServiceGroupDescription(second, true, count, 2 * count);

            VERIFY_IS_TRUE(CServiceGroupDescription::Equals(first, first, TRUE).IsSuccess());
            VERIFY_IS_TRUE(CServiceGroupDescription::Equals(second, second, TRUE).IsSuccess());

            VERIFY_IS_TRUE(CServiceGroupDescription::Equals(first, second, TRUE).IsSuccess());
            VERIFY_IS_TRUE(CServiceGroupDescription::Equals(second, first, TRUE).IsSuccess());
            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(first, second, FALSE).IsSuccess());
            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(second, first, FALSE).IsSuccess());

            vector<byte> serializedFirst;
            vector<byte> serializedSecond;

            VERIFY_IS_TRUE(FabricSerializer::Serialize(&first, serializedFirst).IsSuccess());
            VERIFY_IS_TRUE(FabricSerializer::Serialize(&second, serializedSecond).IsSuccess());

            VERIFY_IS_TRUE(CServiceGroupDescription::Equals(serializedFirst, serializedFirst, TRUE).IsSuccess());
            VERIFY_IS_TRUE(CServiceGroupDescription::Equals(serializedSecond, serializedSecond, TRUE).IsSuccess());

            VERIFY_IS_TRUE(CServiceGroupDescription::Equals(serializedFirst, serializedSecond, TRUE).IsSuccess());
            VERIFY_IS_TRUE(CServiceGroupDescription::Equals(serializedSecond, serializedFirst, TRUE).IsSuccess());
            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(serializedFirst, serializedSecond, FALSE).IsSuccess());
            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(serializedSecond, serializedFirst, FALSE).IsSuccess());

            ServiceDescription description = CreateServiceDescription(L"fabric:/application/group", 1);

            PartitionedServiceDescriptor firstPSD;
            VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, firstPSD).IsSuccess());
            PartitionedServiceDescriptor secondPSD;
            VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, secondPSD).IsSuccess());

            firstPSD = PartitionedServiceDescriptor(firstPSD, move(serializedFirst));
            secondPSD = PartitionedServiceDescriptor(secondPSD, move(serializedSecond));

            firstPSD.IsServiceGroup = true;
            secondPSD.IsServiceGroup = true;
            
            VERIFY_IS_TRUE(firstPSD == secondPSD);
            VERIFY_IS_TRUE(secondPSD == firstPSD);
        }

        // check inequality (member missing)
        for (size_t count = 1; count < 4; ++count)
        {
            CServiceGroupDescription first;
            CServiceGroupDescription second;

            CreateServiceGroupDescription(first, true, count, 2 * count);
            CreateServiceGroupDescription(second, true, count, 2 * count);

            second.ServiceGroupMemberData.erase(begin(second.ServiceGroupMemberData));

            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(first, second, TRUE).IsSuccess());
            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(second, first, TRUE).IsSuccess());
            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(first, second, FALSE).IsSuccess());
            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(second, first, FALSE).IsSuccess());

            vector<byte> serializedFirst;
            vector<byte> serializedSecond;

            VERIFY_IS_TRUE(FabricSerializer::Serialize(&first, serializedFirst).IsSuccess());
            VERIFY_IS_TRUE(FabricSerializer::Serialize(&second, serializedSecond).IsSuccess());

            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(serializedFirst, serializedSecond, TRUE).IsSuccess());
            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(serializedSecond, serializedFirst, TRUE).IsSuccess());
            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(serializedFirst, serializedSecond, FALSE).IsSuccess());
            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(serializedSecond, serializedFirst, FALSE).IsSuccess());

            ServiceDescription description = CreateServiceDescription(L"fabric:/application/group", 1);

            PartitionedServiceDescriptor firstPSD;
            VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, firstPSD).IsSuccess());
            PartitionedServiceDescriptor secondPSD;
            VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, secondPSD).IsSuccess());
            
            firstPSD = PartitionedServiceDescriptor(firstPSD, move(serializedFirst));
            secondPSD = PartitionedServiceDescriptor(secondPSD, move(serializedSecond));

            firstPSD.IsServiceGroup = true;
            secondPSD.IsServiceGroup = true;

            VERIFY_IS_FALSE(firstPSD == secondPSD);
            VERIFY_IS_FALSE(secondPSD == firstPSD);
        }

        // check inequality (member different)
        for (size_t count = 1; count < 4; ++count)
        {
            CServiceGroupDescription first;
            CServiceGroupDescription second;

            CreateServiceGroupDescription(first, true, count, 2 * count);
            CreateServiceGroupDescription(second, true, count, 2 * count);

            second.ServiceGroupMemberData[0].ServiceName = L"other";

            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(first, second, TRUE).IsSuccess());
            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(second, first, TRUE).IsSuccess());
            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(first, second, FALSE).IsSuccess());
            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(second, first, FALSE).IsSuccess());

            vector<byte> serializedFirst;
            vector<byte> serializedSecond;

            VERIFY_IS_TRUE(FabricSerializer::Serialize(&first, serializedFirst).IsSuccess());
            VERIFY_IS_TRUE(FabricSerializer::Serialize(&second, serializedSecond).IsSuccess());

            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(serializedFirst, serializedSecond, TRUE).IsSuccess());
            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(serializedSecond, serializedFirst, TRUE).IsSuccess());
            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(serializedFirst, serializedSecond, FALSE).IsSuccess());
            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(serializedSecond, serializedFirst, FALSE).IsSuccess());

            ServiceDescription description = CreateServiceDescription(L"fabric:/application/group", 1);

            PartitionedServiceDescriptor firstPSD;
            VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, firstPSD).IsSuccess());
            PartitionedServiceDescriptor secondPSD;
            VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, secondPSD).IsSuccess());

            firstPSD = PartitionedServiceDescriptor(firstPSD, move(serializedFirst));
            secondPSD = PartitionedServiceDescriptor(secondPSD, move(serializedSecond));

            firstPSD.IsServiceGroup = true;
            secondPSD.IsServiceGroup = true;
            
            VERIFY_IS_FALSE(firstPSD == secondPSD);
            VERIFY_IS_FALSE(secondPSD == firstPSD);
        }

        // check inequality (metric missing)
        for (size_t count = 1; count < 4; ++count)
        {
            CServiceGroupDescription first;
            CServiceGroupDescription second;

            CreateServiceGroupDescription(first, true, count, 2 * count);
            CreateServiceGroupDescription(second, true, count, 2 * count);

            second.ServiceGroupMemberData[0].Metrics.erase(begin(second.ServiceGroupMemberData[0].Metrics));

            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(first, second, TRUE).IsSuccess());
            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(second, first, TRUE).IsSuccess());
            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(first, second, FALSE).IsSuccess());
            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(second, first, FALSE).IsSuccess());

            vector<byte> serializedFirst;
            vector<byte> serializedSecond;

            VERIFY_IS_TRUE(FabricSerializer::Serialize(&first, serializedFirst).IsSuccess());
            VERIFY_IS_TRUE(FabricSerializer::Serialize(&second, serializedSecond).IsSuccess());

            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(serializedFirst, serializedSecond, TRUE).IsSuccess());
            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(serializedSecond, serializedFirst, TRUE).IsSuccess());
            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(serializedFirst, serializedSecond, FALSE).IsSuccess());
            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(serializedSecond, serializedFirst, FALSE).IsSuccess());

            ServiceDescription description = CreateServiceDescription(L"fabric:/application/group", 1);

            PartitionedServiceDescriptor firstPSD;
            VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, firstPSD).IsSuccess());
            PartitionedServiceDescriptor secondPSD;
            VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, secondPSD).IsSuccess());

            firstPSD = PartitionedServiceDescriptor(firstPSD, move(serializedFirst));
            secondPSD = PartitionedServiceDescriptor(secondPSD, move(serializedSecond));

            firstPSD.IsServiceGroup = true;
            secondPSD.IsServiceGroup = true;
            
            VERIFY_IS_FALSE(firstPSD == secondPSD);
            VERIFY_IS_FALSE(secondPSD == firstPSD);
        }

        // check inequality (metric different)
        for (size_t count = 1; count < 4; ++count)
        {
            CServiceGroupDescription first;
            CServiceGroupDescription second;

            CreateServiceGroupDescription(first, true, count, 2 * count);
            CreateServiceGroupDescription(second, true, count, 2 * count);

            second.ServiceGroupMemberData[0].Metrics[0].Name = L"other";

            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(first, second, TRUE).IsSuccess());
            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(second, first, TRUE).IsSuccess());
            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(first, second, FALSE).IsSuccess());
            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(second, first, FALSE).IsSuccess());

            vector<byte> serializedFirst;
            vector<byte> serializedSecond;

            VERIFY_IS_TRUE(FabricSerializer::Serialize(&first, serializedFirst).IsSuccess());
            VERIFY_IS_TRUE(FabricSerializer::Serialize(&second, serializedSecond).IsSuccess());

            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(serializedFirst, serializedSecond, TRUE).IsSuccess());
            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(serializedSecond, serializedFirst, TRUE).IsSuccess());
            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(serializedFirst, serializedSecond, FALSE).IsSuccess());
            VERIFY_IS_FALSE(CServiceGroupDescription::Equals(serializedSecond, serializedFirst, FALSE).IsSuccess());

            ServiceDescription description = CreateServiceDescription(L"fabric:/application/group", 1);

            PartitionedServiceDescriptor firstPSD;
            VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, firstPSD).IsSuccess());
            PartitionedServiceDescriptor secondPSD;
            VERIFY_IS_TRUE(PartitionedServiceDescriptor::Create(description, secondPSD).IsSuccess());

            firstPSD = PartitionedServiceDescriptor(firstPSD, move(serializedFirst));
            secondPSD = PartitionedServiceDescriptor(secondPSD, move(serializedSecond));

            firstPSD.IsServiceGroup = true;
            secondPSD.IsServiceGroup = true;
            
            VERIFY_IS_FALSE(firstPSD == secondPSD);
            VERIFY_IS_FALSE(secondPSD == firstPSD);
        }
    }

    BOOST_AUTO_TEST_SUITE_END()

    bool PartitionedServiceDescriptorTest::ClassSetup()
    {
        Config config;
        placementConstraints_ = L"";
        scaleoutCount_ = 0;
        metrics_ = vector<Reliability::ServiceLoadMetricDescription>();

        return true;
    }


    void PartitionedServiceDescriptorTest::CreateServiceGroupDescription(CServiceGroupDescription & description, bool hasPersistedState, size_t memberCount, size_t metricCount)
    {
        description.HasPersistedState = hasPersistedState;

        for (size_t i = 0; i < memberCount; ++i)
        {
            CServiceGroupMemberDescription member;
            
            wstring memberName;
            StringWriter name(memberName);
            name.Write("fabric:/application/group#member{0}", i);
            
            member.ServiceName = move(memberName);

            wstring typeName;
            StringWriter type(memberName);
            type.Write("MemberType{0}", i);

            member.ServiceType = move(typeName);

            member.ServiceDescriptionType = (i % 2) ? FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS : FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL;

            member.Identifier = Guid::NewGuid().AsGUID();

            for (size_t j = 0; j < i; ++j)
            {
                member.ServiceGroupMemberInitializationData.push_back((byte)(j % 255));
            }

            for (size_t j = 0; j < metricCount; j++)
            {
                CServiceGroupMemberLoadMetricDescription metric;

                wstring metricName;
                StringWriter temp(metricName);
                temp.Write("Metric{0}", i);

                metric.PrimaryDefaultLoad = (uint)(2 * i);
                metric.SecondaryDefaultLoad = (uint)i;
                metric.Weight = FABRIC_SERVICE_LOAD_METRIC_WEIGHT_LOW;

                member.Metrics.push_back(metric);
            }

            description.ServiceGroupMemberData.push_back(member);
        }
    }

    ServiceDescription PartitionedServiceDescriptorTest::CreateServiceDescription(
        wstring const & name,
        int partitionCount)
    {
        return ServiceDescription(
            name,
            0,
            0,
            partitionCount, 
            1,  // replica count 
            1,  // min write
            false,  // is stateful
            false,  // has persisted
            TimeSpan::Zero,
            TimeSpan::Zero,
            TimeSpan::Zero,
            ServiceModel::ServiceTypeIdentifier(ServiceModel::ServicePackageIdentifier(), L"PsdTestType"), 
            vector<Reliability::ServiceCorrelationDescription>(),
            L"",    // placement constraints
            0, 
            vector<Reliability::ServiceLoadMetricDescription>(), 
            0,
            vector<byte>());
    }

}
