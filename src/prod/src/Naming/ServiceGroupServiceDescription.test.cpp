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

    class ServiceGroupServiceDescriptionTest
    {
    protected:
        ServiceGroupServiceDescriptionTest() { BOOST_REQUIRE(ClassSetup()); }
        TEST_CLASS_SETUP(ClassSetup)

    };


    BOOST_FIXTURE_TEST_SUITE(ServiceGroupServiceDescriptionTestSuite,ServiceGroupServiceDescriptionTest)

    BOOST_AUTO_TEST_CASE(TestCreateDescription)
    {
        FABRIC_SERVICE_GROUP_DESCRIPTION groupDescription = { 0 };
        
        FABRIC_SERVICE_DESCRIPTION groupServiceDescription;
        FABRIC_STATEFUL_SERVICE_DESCRIPTION groupServiceDescriptionStateful = { 0 };

        FABRIC_SERVICE_GROUP_MEMBER_DESCRIPTION members[2];

        groupDescription.Description = &groupServiceDescription;
        groupDescription.MemberDescriptions = members;
        groupDescription.MemberCount = 2;

        groupServiceDescription.Kind = FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL;
        groupServiceDescription.Value = &groupServiceDescriptionStateful;

        groupServiceDescriptionStateful.ApplicationName = L"fabric:/application";
        
        groupServiceDescriptionStateful.ServiceTypeName = L"SGType";
        groupServiceDescriptionStateful.ServiceName = L"fabric:/application/servicegroup";

        members[0].ServiceName = L"fabric:/application/servicegroup#a";
        members[0].ServiceType = L"AType";
        members[0].InitializationData = NULL;
        members[0].InitializationDataSize = 0;
        members[0].Metrics = NULL;
        members[0].MetricCount = 0;
        members[0].Reserved = NULL;

        members[1].ServiceName = L"fabric:/application/servicegroup#b";
        members[1].ServiceType = L"BType";
        members[1].InitializationData = NULL;
        members[1].InitializationDataSize = 0;
        members[1].Metrics = NULL;
        members[1].MetricCount = 0;
        members[1].Reserved = NULL;

        ServiceGroupServiceDescription description;
        description.FromServiceGroupDescription(groupDescription);

        auto serviceDescription = description.GetRawPointer();

        VERIFY_IS_TRUE(serviceDescription != NULL);
        VERIFY_ARE_EQUAL(FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL, serviceDescription->Kind);
        VERIFY_IS_TRUE(serviceDescription->Value != NULL);

        auto serviceDescriptionStateful = static_cast<FABRIC_STATEFUL_SERVICE_DESCRIPTION*>(serviceDescription->Value);

        VERIFY_ARE_EQUAL(groupServiceDescriptionStateful.ServiceName, serviceDescriptionStateful->ServiceName);
        VERIFY_ARE_EQUAL(groupServiceDescriptionStateful.ServiceTypeName, serviceDescriptionStateful->ServiceTypeName);
        VERIFY_ARE_EQUAL(groupServiceDescriptionStateful.ApplicationName, serviceDescriptionStateful->ApplicationName);

        VERIFY_IS_TRUE(serviceDescriptionStateful->InitializationDataSize != 0);
        VERIFY_IS_TRUE(serviceDescriptionStateful->InitializationData != NULL);

        CServiceGroupDescription serviceGroupDescription;

        VERIFY_IS_TRUE(FabricSerializer::Deserialize(
            serviceGroupDescription, 
            serviceDescriptionStateful->InitializationDataSize, 
            serviceDescriptionStateful->InitializationData).IsSuccess());
        
        VERIFY_ARE_EQUAL(static_cast<size_t>(2), serviceGroupDescription.ServiceGroupMemberData.size());

        auto memberDescriptions = serviceGroupDescription.ServiceGroupMemberData;

        VERIFY_ARE_EQUAL(wstring(members[0].ServiceName), memberDescriptions[0].ServiceName);
        VERIFY_ARE_EQUAL(wstring(members[0].ServiceType), memberDescriptions[0].ServiceType);

        VERIFY_ARE_EQUAL(wstring(members[1].ServiceName), memberDescriptions[1].ServiceName);
        VERIFY_ARE_EQUAL(wstring(members[1].ServiceType), memberDescriptions[1].ServiceType);

        VERIFY_ARE_EQUAL(static_cast<size_t>(0), memberDescriptions[0].ServiceGroupMemberInitializationData.size());
        VERIFY_ARE_EQUAL(static_cast<size_t>(0), memberDescriptions[1].ServiceGroupMemberInitializationData.size());

        VERIFY_ARE_EQUAL(0ul, serviceDescriptionStateful->MetricCount);
        VERIFY_IS_TRUE(NULL == serviceDescriptionStateful->Metrics);
    }

    BOOST_AUTO_TEST_CASE(TestCreateDescriptionWithInitData)
    {
        FABRIC_SERVICE_GROUP_DESCRIPTION groupDescription = { 0 };
        
        FABRIC_SERVICE_DESCRIPTION groupServiceDescription;
        FABRIC_STATEFUL_SERVICE_DESCRIPTION groupServiceDescriptionStateful = { 0 };

        FABRIC_SERVICE_GROUP_MEMBER_DESCRIPTION members[2];

        groupDescription.Description = &groupServiceDescription;
        groupDescription.MemberDescriptions = members;
        groupDescription.MemberCount = 2;

        groupServiceDescription.Kind = FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL;
        groupServiceDescription.Value = &groupServiceDescriptionStateful;

        groupServiceDescriptionStateful.ApplicationName = L"fabric:/application";
        
        groupServiceDescriptionStateful.ServiceTypeName = L"SGType";
        groupServiceDescriptionStateful.ServiceName = L"fabric:/application/servicegroup";

        vector<byte> serviceInitData;
        serviceInitData.resize(47);
        for (size_t i = 0; i < serviceInitData.size(); ++i)
        {
            serviceInitData[i] = (byte)i;
        }

        groupServiceDescriptionStateful.InitializationDataSize = static_cast<ULONG>(serviceInitData.size());
        groupServiceDescriptionStateful.InitializationData = serviceInitData.data();

        members[0].ServiceName = L"fabric:/application/servicegroup#a";
        members[0].ServiceType = L"AType";
        members[0].InitializationData = NULL;
        members[0].InitializationDataSize = 0;
        members[0].Metrics = NULL;
        members[0].MetricCount = 0;
        members[0].Reserved = 0;

        vector<byte> memberInitData;
        memberInitData.resize(10);
        for (size_t i = 0; i < memberInitData.size(); ++i)
        {
            memberInitData[i] = (byte)i;
        }

        members[1].ServiceName = L"fabric:/application/servicegroup#b";
        members[1].ServiceType = L"BType";
        members[1].InitializationData = memberInitData.data();
        members[1].InitializationDataSize = static_cast<ULONG>(memberInitData.size());
        members[1].Metrics = NULL;
        members[1].MetricCount = 0;
        members[1].Reserved = 0;

        ServiceGroupServiceDescription description;
        description.FromServiceGroupDescription(groupDescription);

        auto serviceDescription = description.GetRawPointer();

        VERIFY_IS_TRUE(serviceDescription != NULL);
        VERIFY_ARE_EQUAL(FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL, serviceDescription->Kind);
        VERIFY_IS_TRUE(serviceDescription->Value != NULL);

        auto serviceDescriptionStateful = static_cast<FABRIC_STATEFUL_SERVICE_DESCRIPTION*>(serviceDescription->Value);

        VERIFY_ARE_EQUAL(groupServiceDescriptionStateful.ServiceName, serviceDescriptionStateful->ServiceName);
        VERIFY_ARE_EQUAL(groupServiceDescriptionStateful.ServiceTypeName, serviceDescriptionStateful->ServiceTypeName);
        VERIFY_ARE_EQUAL(groupServiceDescriptionStateful.ApplicationName, serviceDescriptionStateful->ApplicationName);

        VERIFY_IS_TRUE(serviceDescriptionStateful->InitializationDataSize != 0);
        VERIFY_IS_TRUE(serviceDescriptionStateful->InitializationData != NULL);

        CServiceGroupDescription serviceGroupDescription;

        VERIFY_IS_TRUE(FabricSerializer::Deserialize(
            serviceGroupDescription, 
            serviceDescriptionStateful->InitializationDataSize, 
            serviceDescriptionStateful->InitializationData).IsSuccess());
        
        VERIFY_ARE_EQUAL(serviceInitData.size(), serviceGroupDescription.ServiceGroupInitializationData.size());
        for (size_t i = 0; i < serviceGroupDescription.ServiceGroupInitializationData.size(); ++i)
        {
            VERIFY_ARE_EQUAL((byte)i, serviceGroupDescription.ServiceGroupInitializationData[i]);
        }

        VERIFY_ARE_EQUAL(static_cast<size_t>(2), serviceGroupDescription.ServiceGroupMemberData.size());

        auto memberDescriptions = serviceGroupDescription.ServiceGroupMemberData;

        VERIFY_ARE_EQUAL(static_cast<size_t>(0), memberDescriptions[0].ServiceGroupMemberInitializationData.size());
        VERIFY_ARE_EQUAL(memberInitData.size(), memberDescriptions[1].ServiceGroupMemberInitializationData.size());
        for (size_t i = 0; i < memberDescriptions[1].ServiceGroupMemberInitializationData.size(); ++i)
        {
            VERIFY_ARE_EQUAL((byte)i, memberDescriptions[1].ServiceGroupMemberInitializationData[i]);
        }
    }

    BOOST_AUTO_TEST_CASE(TestCreateDescriptionAggregateLoadMetrics)
    {
        FABRIC_SERVICE_GROUP_DESCRIPTION groupDescription = { 0 };
        
        FABRIC_SERVICE_DESCRIPTION groupServiceDescription;
        FABRIC_STATEFUL_SERVICE_DESCRIPTION groupServiceDescriptionStateful = { 0 };

        FABRIC_SERVICE_GROUP_MEMBER_DESCRIPTION members[2];

        groupDescription.Description = &groupServiceDescription;
        groupDescription.MemberDescriptions = members;
        groupDescription.MemberCount = 2;

        groupServiceDescription.Kind = FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL;
        groupServiceDescription.Value = &groupServiceDescriptionStateful;

        groupServiceDescriptionStateful.ApplicationName = L"fabric:/application";
        
        groupServiceDescriptionStateful.ServiceTypeName = L"SGType";
        groupServiceDescriptionStateful.ServiceName = L"fabric:/application/servicegroup";

        FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION metricsA[3];
        FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION metricsB[2];

        memset(metricsA, 0, sizeof(metricsA));
        memset(metricsB, 0, sizeof(metricsB));

        members[0].ServiceName = L"fabric:/application/servicegroup#a";
        members[0].ServiceType = L"AType";
        members[0].InitializationData = NULL;
        members[0].InitializationDataSize = 0;
        members[0].Metrics = metricsA;
        members[0].MetricCount = 3;
        members[0].Reserved = NULL;

        members[1].ServiceName = L"fabric:/application/servicegroup#b";
        members[1].ServiceType = L"BType";
        members[1].InitializationData = NULL;
        members[1].InitializationDataSize = 0;
        members[1].Metrics = metricsB;
        members[1].MetricCount = 2;
        members[1].Reserved = NULL;

        metricsA[0].Weight = FABRIC_SERVICE_LOAD_METRIC_WEIGHT_LOW;
        metricsA[0].Name = L"M1";
        metricsA[0].PrimaryDefaultLoad = 15;
        metricsA[0].SecondaryDefaultLoad = 14;

        metricsA[1].Weight = FABRIC_SERVICE_LOAD_METRIC_WEIGHT_MEDIUM;
        metricsA[1].Name = L"M2";
        metricsA[1].PrimaryDefaultLoad = 48;
        metricsA[1].SecondaryDefaultLoad = 47;

        metricsA[2].Weight = FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH;
        metricsA[2].Name = L"M3";
        metricsA[2].PrimaryDefaultLoad = 4444;
        metricsA[2].SecondaryDefaultLoad = 4443;

        metricsB[0].Weight = FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH;
        metricsB[0].Name = L"M1";
        metricsB[0].PrimaryDefaultLoad = 100;
        metricsB[0].SecondaryDefaultLoad = 99;

        metricsB[1].Weight = FABRIC_SERVICE_LOAD_METRIC_WEIGHT_LOW;
        metricsB[1].Name = L"M2";
        metricsB[1].PrimaryDefaultLoad = 100;
        metricsB[1].SecondaryDefaultLoad = 99;

        ServiceGroupServiceDescription description;

        auto hr = description.FromServiceGroupDescription(groupDescription);
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        auto serviceDescription = description.GetRawPointer();

        VERIFY_IS_TRUE(serviceDescription != NULL);
        VERIFY_ARE_EQUAL(FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL, serviceDescription->Kind);
        VERIFY_IS_TRUE(serviceDescription->Value != NULL);

        auto serviceDescriptionStateful = static_cast<FABRIC_STATEFUL_SERVICE_DESCRIPTION*>(serviceDescription->Value);

        VERIFY_ARE_EQUAL(3ul, serviceDescriptionStateful->MetricCount);
        VERIFY_IS_TRUE(NULL != serviceDescriptionStateful->Metrics);

        map<wstring, FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION> metrics;
        for (ULONG i = 0; i < serviceDescriptionStateful->MetricCount; ++i)
        {
            metrics[wstring(serviceDescriptionStateful->Metrics[i].Name)] = serviceDescriptionStateful->Metrics[i];
        }

        VERIFY_ARE_EQUAL(static_cast<size_t>(3), metrics.size());

        VERIFY_IS_TRUE(metrics.find(L"M1") != end(metrics));
        VERIFY_IS_TRUE(metrics.find(L"M2") != end(metrics));
        VERIFY_IS_TRUE(metrics.find(L"M3") != end(metrics));

        VERIFY_ARE_EQUAL(100u + 15u, metrics[L"M1"].PrimaryDefaultLoad);
        VERIFY_ARE_EQUAL(99u + 14u, metrics[L"M1"].SecondaryDefaultLoad);
        VERIFY_ARE_EQUAL(FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH, metrics[L"M1"].Weight);
        
        VERIFY_ARE_EQUAL(100u + 48u, metrics[L"M2"].PrimaryDefaultLoad);
        VERIFY_ARE_EQUAL(99u + 47u, metrics[L"M2"].SecondaryDefaultLoad);
        VERIFY_ARE_EQUAL(FABRIC_SERVICE_LOAD_METRIC_WEIGHT_MEDIUM, metrics[L"M2"].Weight);
        
        VERIFY_ARE_EQUAL(4444u, metrics[L"M3"].PrimaryDefaultLoad);
        VERIFY_ARE_EQUAL(4443u, metrics[L"M3"].SecondaryDefaultLoad);
        VERIFY_ARE_EQUAL(FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH, metrics[L"M3"].Weight);
    }

    BOOST_AUTO_TEST_CASE(TestCreateDescriptionAggregatesDontOverride)
    {
        FABRIC_SERVICE_GROUP_DESCRIPTION groupDescription = { 0 };
        
        FABRIC_SERVICE_DESCRIPTION groupServiceDescription;
        FABRIC_STATEFUL_SERVICE_DESCRIPTION groupServiceDescriptionStateful = { 0 };

        FABRIC_SERVICE_GROUP_MEMBER_DESCRIPTION members[2];

        groupDescription.Description = &groupServiceDescription;
        groupDescription.MemberDescriptions = members;
        groupDescription.MemberCount = 2;

        groupServiceDescription.Kind = FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL;
        groupServiceDescription.Value = &groupServiceDescriptionStateful;

        groupServiceDescriptionStateful.ApplicationName = L"fabric:/application";
        
        groupServiceDescriptionStateful.ServiceTypeName = L"SGType";
        groupServiceDescriptionStateful.ServiceName = L"fabric:/application/servicegroup";

        FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION metricsGroup[1];
        FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION metricsA[3];
        FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION metricsB[2];

        memset(metricsA, 0, sizeof(metricsA));
        memset(metricsB, 0, sizeof(metricsB));

        groupServiceDescriptionStateful.Metrics = metricsGroup;
        groupServiceDescriptionStateful.MetricCount = 1;

        members[0].ServiceName = L"fabric:/application/servicegroup#a";
        members[0].ServiceType = L"AType";
        members[0].InitializationData = NULL;
        members[0].InitializationDataSize = 0;
        members[0].Metrics = metricsA;
        members[0].MetricCount = 3;
        members[0].Reserved = NULL;

        members[1].ServiceName = L"fabric:/application/servicegroup#b";
        members[1].ServiceType = L"BType";
        members[1].InitializationData = NULL;
        members[1].InitializationDataSize = 0;
        members[1].Metrics = metricsB;
        members[1].MetricCount = 2;
        members[1].Reserved = NULL;

        metricsA[0].Weight = FABRIC_SERVICE_LOAD_METRIC_WEIGHT_LOW;
        metricsA[0].Name = L"M1";
        metricsA[0].PrimaryDefaultLoad = 15;
        metricsA[0].SecondaryDefaultLoad = 14;

        metricsA[1].Weight = FABRIC_SERVICE_LOAD_METRIC_WEIGHT_MEDIUM;
        metricsA[1].Name = L"M2";
        metricsA[1].PrimaryDefaultLoad = 48;
        metricsA[1].SecondaryDefaultLoad = 47;

        metricsA[2].Weight = FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH;
        metricsA[2].Name = L"M3";
        metricsA[2].PrimaryDefaultLoad = 4444;
        metricsA[2].SecondaryDefaultLoad = 4443;

        metricsB[0].Weight = FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH;
        metricsB[0].Name = L"M1";
        metricsB[0].PrimaryDefaultLoad = 100;
        metricsB[0].SecondaryDefaultLoad = 99;

        metricsB[1].Weight = FABRIC_SERVICE_LOAD_METRIC_WEIGHT_LOW;
        metricsB[1].Name = L"M2";
        metricsB[1].PrimaryDefaultLoad = 100;
        metricsB[1].SecondaryDefaultLoad = 99;

        metricsGroup[0].Weight = FABRIC_SERVICE_LOAD_METRIC_WEIGHT_ZERO;
        metricsGroup[0].Name = L"MG";
        metricsGroup[0].PrimaryDefaultLoad = 9999;
        metricsGroup[0].SecondaryDefaultLoad = 9998;

        ServiceGroupServiceDescription description;
        description.FromServiceGroupDescription(groupDescription);

        auto serviceDescription = description.GetRawPointer();

        VERIFY_IS_TRUE(serviceDescription != NULL);
        VERIFY_ARE_EQUAL(FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL, serviceDescription->Kind);
        VERIFY_IS_TRUE(serviceDescription->Value != NULL);

        auto serviceDescriptionStateful = static_cast<FABRIC_STATEFUL_SERVICE_DESCRIPTION*>(serviceDescription->Value);

        VERIFY_ARE_EQUAL(1ul, serviceDescriptionStateful->MetricCount);
        VERIFY_IS_TRUE(NULL != serviceDescriptionStateful->Metrics);

        map<wstring, FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION> metrics;
        for (ULONG i = 0; i < serviceDescriptionStateful->MetricCount; ++i)
        {
            metrics[wstring(serviceDescriptionStateful->Metrics[i].Name)] = serviceDescriptionStateful->Metrics[i];
        }

        VERIFY_ARE_EQUAL(static_cast<size_t>(1), metrics.size());
        VERIFY_IS_TRUE(metrics.find(L"MG") != end(metrics));

        VERIFY_ARE_EQUAL(9999u, metrics[L"MG"].PrimaryDefaultLoad);
        VERIFY_ARE_EQUAL(9998u, metrics[L"MG"].SecondaryDefaultLoad);
        VERIFY_ARE_EQUAL(FABRIC_SERVICE_LOAD_METRIC_WEIGHT_ZERO, metrics[L"MG"].Weight);
    }

    BOOST_AUTO_TEST_SUITE_END()

    bool ServiceGroupServiceDescriptionTest::ClassSetup()
    {
        return true;
    }

}
