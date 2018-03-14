// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace FailoverManagerUnitTest
{
    using namespace Common;
    using namespace Client;
    using namespace std;
    using namespace Reliability;
    using namespace Reliability::FailoverManagerComponent;
    using namespace Federation;

    class TestFailoverUnitCache
    {
    protected:

        TestFailoverUnitCache() { BOOST_REQUIRE(MethodSetup()); }
        TEST_METHOD_SETUP(MethodSetup);

        ~TestFailoverUnitCache() { BOOST_REQUIRE(MethodCleanup()); }
        TEST_METHOD_CLEANUP(MethodCleanup);

        void CreateFailoverUnitsFromService(ServiceInfoSPtr const& serviceInfo, vector<FailoverUnitUPtr> & failoverUnits);

        ComponentRootSPtr root_;
        FailoverManagerSPtr fm_;

        vector<FailoverUnitUPtr> failoverUnits_;
        vector<ServiceInfoSPtr> services_;
    };

    BOOST_FIXTURE_TEST_SUITE(TestFailoverUnitCacheSuite,TestFailoverUnitCache)

    BOOST_AUTO_TEST_CASE(BasicTest)
    {
        FailoverUnitCache cache(*fm_, failoverUnits_, 0, *root_);

        VERIFY_ARE_EQUAL(30u, cache.Count);

        cache.ServiceLookupTable.Dispose();
    }

    BOOST_AUTO_TEST_CASE(VisitorTest)
    {
        FailoverUnitCache cache(*fm_, failoverUnits_, 0, *root_);

        FailoverUnitCache::VisitorSPtr visitor = cache.CreateVisitor(true);

        set<FailoverUnitId> fuSet;
        while (auto failoverUnit = visitor->MoveNext())
        {
            fuSet.insert(failoverUnit->Id);
        }

        VERIFY_ARE_EQUAL(30u, fuSet.size());

        cache.ServiceLookupTable.Dispose();
    }

    BOOST_AUTO_TEST_SUITE_END()

    void TestFailoverUnitCache::CreateFailoverUnitsFromService(ServiceInfoSPtr const& serviceInfo, vector<FailoverUnitUPtr> & failoverUnits)
    {
        for (int i=0; i < 10; i++)
        {
            // TODO: create a helper class for creating failover units in dev code
            ConsistencyUnitDescription consistencyUnitDescription;
            FailoverUnitId failoverUnitId(consistencyUnitDescription.ConsistencyUnitId.Guid);
            auto failoverUnit = new FailoverUnit(failoverUnitId, consistencyUnitDescription, true, true, serviceInfo->Name, serviceInfo);
            failoverUnit->PostUpdate(DateTime::Now());
            failoverUnit->PostCommit(failoverUnit->OperationLSN + 1);
            failoverUnits.push_back(FailoverUnitUPtr(failoverUnit));
        }
    }

    bool TestFailoverUnitCache::MethodSetup()
    {
        FailoverConfig::Test_Reset();
        FailoverConfig & config = FailoverConfig::GetConfig();
        config.IsTestMode = true;
        config.DummyPLBEnabled = true;
        
        LoadBalancingComponent::PLBConfig::Test_Reset();
        auto & config_plb = Reliability::LoadBalancingComponent::PLBConfig::GetConfig();
        config_plb.IsTestMode = true;
        config_plb.DummyPLBEnabled=true;
        config_plb.PLBRefreshInterval = (TimeSpan::MaxValue); //set so that PLB won't spawn threads to balance empty data structures in functional tests
        config_plb.ProcessPendingUpdatesInterval = (TimeSpan::MaxValue);//set so that PLB won't spawn threads to balance empty data structures in functional tests

        root_ = make_shared<ComponentRoot>();
        fm_ = TestHelper::CreateFauxFM(*root_);

        wstring placementConstraints = L"";
        int scaleoutCount = 0;

        ServiceModel::ApplicationIdentifier appId;
        ServiceModel::ApplicationIdentifier::FromString(L"TestApp_App0", appId);
        ApplicationInfoSPtr applicationInfo = make_shared<ApplicationInfo>(appId, NamingUri(L"fabric:/TestApp"), 1);
        ApplicationEntrySPtr applicationEntry = make_shared<CacheEntry<ApplicationInfo>>(move(applicationInfo));
        for (int i = 1; i <= 3; ++i)
        {
            wstring serviceName = L"App" + StringUtility::ToWString<int>(i);
            wstring serviceTypeName = L"Type" + StringUtility::ToWString<int>(i);
            ServiceModel::ServiceTypeIdentifier typeId(ServiceModel::ServicePackageIdentifier(appId, L"TestPackage"), serviceTypeName);

            auto serviceType = make_shared<ServiceType>(typeId, applicationEntry);
            auto serviceInfo = make_unique<ServiceInfo>(
                ServiceDescription(serviceName, 0, 0, 10, 3, 2, true, true, TimeSpan::FromSeconds(60.0), TimeSpan::MaxValue, TimeSpan::FromSeconds(300.0), typeId, vector<ServiceCorrelationDescription>(), placementConstraints, scaleoutCount, vector<ServiceLoadMetricDescription>(), 0, vector<byte>()),
                serviceType,
                FABRIC_INVALID_SEQUENCE_NUMBER,
                false);

            services_.push_back(move(serviceInfo));
        }

        for (ServiceInfoSPtr const& service : services_)
        {
            CreateFailoverUnitsFromService(service, failoverUnits_);
        }

        return true;
    }

    bool TestFailoverUnitCache::MethodCleanup()
    {
        failoverUnits_.clear();
        services_.clear();
        FailoverConfig::Test_Reset();

        return true;
    }

}
