// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestUtility.h"
#include "TestFM.h"
#include "PlacementAndLoadBalancing.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include <algorithm>

namespace PlacementAndLoadBalancingUnitTest
{
    using namespace std;
    using namespace Common;
    using namespace Reliability::LoadBalancingComponent;
    using namespace Federation;

    class TestServiceDomain
    {
    protected:
        TestServiceDomain() {
            BOOST_REQUIRE(ClassSetup());
            BOOST_REQUIRE(TestSetup());
        }

        ~TestServiceDomain()
        {
            BOOST_REQUIRE(ClassCleanup());
        }

        TEST_CLASS_SETUP(ClassSetup);
        TEST_CLASS_CLEANUP(ClassCleanup);
        TEST_METHOD_SETUP(TestSetup);

        shared_ptr<TestFM> fm_;
        void VerifyApplicationSumLoad(wstring const& appName, wstring const& metricName, int64 expectedLoad, int64 size);
    };

    BOOST_FIXTURE_TEST_SUITE(TestServiceDomainSuite,TestServiceDomain)

    BOOST_AUTO_TEST_CASE(BasicTest)
    {
        PlacementAndLoadBalancing & plb = fm_->PLB;

        VERIFY_ARE_EQUAL(0u, plb.GetServiceDomains().size());

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));
        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true));

        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());

        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true));
        plb.UpdateService(CreateServiceDescription(L"TestService3", L"TestType", true));
        plb.UpdateService(CreateServiceDescription(L"TestService4", L"TestType", true));

        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());
    }

    BOOST_AUTO_TEST_CASE(DomainMergeTest)
    {
        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(SplitDomainEnabled, bool, true);

        VERIFY_ARE_EQUAL(0u, plb.GetServiceDomains().size());

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(L"TestService1", L"TestType", true, CreateMetrics(L"Metric1/1.0/0/0,Metric2/1.0/0/0")));
        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(L"TestService2", L"TestType", true, CreateMetrics(L"Metric3/1.0/0/0,Metric4/1.0/0/0")));

        VERIFY_ARE_EQUAL(2u, plb.GetServiceDomains().size());

        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(L"TestService3", L"TestType", true, CreateMetrics(L"Metric1/1.0/0/0,Metric4/1.0/0/0")));

        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());

        plb.DeleteService(L"TestService3");
        //after delete, TestService1 and TestService2 will split
        VERIFY_ARE_EQUAL(2u, plb.GetServiceDomains().size());

        plb.DeleteService(L"TestService2");
        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());

        plb.DeleteService(L"TestService1");
        VERIFY_ARE_EQUAL(0u, plb.GetServiceDomains().size());

        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(L"TestService4", L"TestType", true, CreateMetrics(L"Metric1/1.0/0/0")));
        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());

        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(L"TestService5", L"TestType", true, CreateMetrics(L"Metric2/1.0/0/0")));
        VERIFY_ARE_EQUAL(2u, plb.GetServiceDomains().size());

        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(L"TestService6", L"TestType", true, CreateMetrics(L"Metric3/1.0/0/0")));
        VERIFY_ARE_EQUAL(3u, plb.GetServiceDomains().size());

        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(L"TestService7", L"TestType", true, CreateMetrics(L"Metric1/1.0/0/0,Metric2/1.0/0/0,Metric3/1.0/0/0,Metric4/1.0/0/0")));
        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());

        //test split after deletion
        plb.DeleteService(L"TestService7");
        VERIFY_ARE_EQUAL(3u, plb.GetServiceDomains().size());

        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(L"TestService8", L"TestType", true, CreateMetrics(L"Metric1/1.0/0/0,Metric2/1.0/0/0")));
        VERIFY_ARE_EQUAL(2u, plb.GetServiceDomains().size());

        plb.DeleteService(L"TestService8");
        VERIFY_ARE_EQUAL(3u, plb.GetServiceDomains().size());
    }

    BOOST_AUTO_TEST_CASE(AddSameServiceLaterTest)
    {
        PlacementAndLoadBalancing & plb = fm_->PLB;

        VERIFY_ARE_EQUAL(0u, plb.GetServiceDomains().size());

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(L"TestService1", L"TestType", true, CreateMetrics(L"Metric1/1.0/0/0,Metric2/1.0/0/0")));
        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());

        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(L"TestService2", L"TestType", true, CreateMetrics(L"Metric2/1.0/0/0,Metric3/1.0/0/0")));
        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());

        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(L"TestService3", L"TestType", true, CreateMetrics(L"Metric3/1.0/0/0,Metric4/1.0/0/0")));
        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());

        plb.DeleteService(L"TestService2");
        plb.DeleteService(L"TestService1");
        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());

        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(L"TestService1", L"TestType", true, CreateMetrics(L"Metric1/1.0/0/0,Metric2/1.0/0/0")));
        VERIFY_ARE_EQUAL(2u, plb.GetServiceDomains().size());
    }

    BOOST_AUTO_TEST_CASE(AffinitizedServiceTest1)
    {
        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(UseAppGroupsInBoost, bool, false);
        PLBConfigScopeChange(SplitDomainEnabled, bool, true);
        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        vector<ServiceMetric> metrics1 = CreateMetrics(L"Metric1/1.0/0/0");
        vector<ServiceMetric> metrics2 = CreateMetrics(L"Metric2/1.0/0/0");

        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestServiceN", vector<ServiceMetric>(metrics1)));
        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(L"TestService2", L"TestType", true, vector<ServiceMetric>(metrics1)));
        plb.DeleteService(L"TestService1");
        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(L"TestServiceN", L"TestType", true, vector<ServiceMetric>(metrics2)));
        VERIFY_ARE_EQUAL(2u, plb.GetServiceDomains().size());

        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestServiceN", vector<ServiceMetric>(metrics1)));
        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());

        plb.DeleteService(L"TestServiceN");

        ServiceDomain::DomainId remainingDomainName = plb.GetServiceDomains().begin()->domainId_;

        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService3", L"TestType", true, L"TestServiceN", vector<ServiceMetric>(metrics1)));
        plb.DeleteService(L"TestService1");
        plb.DeleteService(L"TestService3");
        plb.DeleteService(L"TestService2");

        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(L"TestServiceN", L"TestType", true, vector<ServiceMetric>(metrics2)));
        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());
        VERIFY_ARE_NOT_EQUAL(remainingDomainName, plb.GetServiceDomains().begin()->domainId_);
    }

    BOOST_AUTO_TEST_CASE(AffinitizedServiceTest2)
    {
        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(UseAppGroupsInBoost, bool, false);
        PLBConfigScopeChange(SplitDomainEnabled, bool, true);

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true, CreateMetrics(L"Metric1/1.0/0/0")));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService2", L"TestType", true, L"TestServiceN", CreateMetrics(L"Metric2/1.0/0/0")));
        // results in a merge of the two domains
        plb.UpdateService(CreateServiceDescription(L"TestService3", L"TestType", true, CreateMetrics(L"Metric1/1.0/0/0,Metric2/1.0/0/0")));

        plb.UpdateService(CreateServiceDescription(L"TestServiceN", L"TestType", true, CreateMetrics(L"Metric3/1.0/0/0")));
        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());
        VERIFY_ARE_EQUAL(4u, plb.GetServiceDomains().begin()->services_.size());
    }

    BOOST_AUTO_TEST_CASE(AffinitizedServiceTest3)
    {
        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(UseAppGroupsInBoost, bool, false);
        PLBConfigScopeChange(SplitDomainEnabled, bool, true);

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestServiceN", CreateMetrics(L"Metric1/1.0/0/0")));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService2", L"TestType", true, L"TestServiceN", CreateMetrics(L"Metric2/1.0/0/0")));
        plb.UpdateService(CreateServiceDescription(L"TestServiceN", L"TestType", true, CreateMetrics(L"Metric3/1.0/0/0")));

        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());
        VERIFY_ARE_EQUAL(3u, plb.GetServiceDomains().begin()->services_.size());
    }

    BOOST_AUTO_TEST_CASE(AffinitizedServiceTest4)
    {
        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(UseAppGroupsInBoost, bool, false);
        PLBConfigScopeChange(SplitDomainEnabled, bool, true);

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true, CreateMetrics(L"Metric1/1.0/0/0")));
        // results in a merge of the two domains
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService2", L"TestType", true, L"TestService1", CreateMetrics(L"Metric2/1.0/0/0")));
        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());
        //affinity removal should split services
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true, CreateMetrics(L"Metric2/1.0/0/0")));
        VERIFY_ARE_EQUAL(2u, plb.GetServiceDomains().size());
    }

    BOOST_AUTO_TEST_CASE(AffinitizedServiceTest5)
    {
        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(UseAppGroupsInBoost, bool, false);
        PLBConfigScopeChange(SplitDomainEnabled, bool, true);

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true, CreateMetrics(L"Metric1/1.0/0/0")));
        // results in a merge of the two domains
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService2", L"TestType", true, L"TestService1", CreateMetrics(L"Metric2/1.0/0/0")));
        //affinity removal should split services
        plb.UpdateService(CreateServiceDescription(L"TestService3", L"TestType", true, CreateMetrics(L"Metric1/1.0/0/0")));
        plb.UpdateService(CreateServiceDescription(L"TestService4", L"TestType", true, CreateMetrics(L"Metric2/1.0/0/0")));
        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());
        //this should split off testservice3 from the domain...
        plb.UpdateService(CreateServiceDescription(L"TestService1", L"TestType", true, CreateMetrics(L"Metric3/1.0/0/0")));
        VERIFY_ARE_EQUAL(2u, plb.GetServiceDomains().size());
        //this should split off testservice4 from the domain...
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService2", L"TestType", true, L"TestService1", CreateMetrics(L"Metric4/1.0/0/0")));
        VERIFY_ARE_EQUAL(3u, plb.GetServiceDomains().size());
    }

    BOOST_AUTO_TEST_CASE(AffinityChainDetectionTest)
    {
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        vector<ServiceMetric> metrics1 = CreateMetrics(L"Metric1/1.0/0/0");
        vector<ServiceMetric> metrics2 = CreateMetrics(L"Metric2/1.0/0/0");
        vector<ServiceMetric> metrics3 = CreateMetrics(L"Metric3/1.0/0/0");

        VERIFY_IS_TRUE(plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService2", vector<ServiceMetric>(metrics1))).IsSuccess());
        VERIFY_IS_FALSE(plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService2", L"TestType", true, L"TestService1", vector<ServiceMetric>(metrics2))).IsSuccess());
        VERIFY_IS_FALSE(plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService2", L"TestType", true, L"TestService3", vector<ServiceMetric>(metrics2))).IsSuccess());

        VERIFY_IS_TRUE(plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService3", L"TestType", true, L"TestServiceN", vector<ServiceMetric>(metrics3))).IsSuccess());
        VERIFY_IS_TRUE(plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService4", L"TestType", true, L"TestServiceN", vector<ServiceMetric>(metrics1))).IsSuccess());

        VERIFY_IS_FALSE(plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService5", L"TestType", true, L"TestService3", vector<ServiceMetric>(metrics2))).IsSuccess());
        VERIFY_IS_FALSE(plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService5", L"TestType", true, L"TestService4", vector<ServiceMetric>(metrics2))).IsSuccess());

        VERIFY_IS_TRUE(plb.UpdateService(CreateServiceDescription(L"TestServiceN", L"TestType", true, vector<ServiceMetric>(metrics2))).IsSuccess());
        VERIFY_IS_FALSE(plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService5", L"TestType", true, L"TestService3", vector<ServiceMetric>(metrics2))).IsSuccess());
        VERIFY_IS_FALSE(plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService5", L"TestType", true, L"TestService4", vector<ServiceMetric>(metrics2))).IsSuccess());

        auto domains = plb.GetServiceDomains();
        VERIFY_ARE_EQUAL(1u, domains.size());
        VERIFY_ARE_EQUAL(4u, domains.begin()->services_.size());
    }

    BOOST_AUTO_TEST_CASE(InsufficientResourceDetectionTest)
    {
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 2; i++)
        {
            plb.UpdateNode(CreateNodeDescriptionWithCapacity(i, L"Metric1/10, Metric2/10, Metric3/10"));
        }
        // node2 doesn't have Metric3 capacity defined
        plb.UpdateNode(CreateNodeDescriptionWithCapacity(2, L"Metric1/10, Metric2/10"));

        // Force processing of pending updates so that service can be created.
        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        VERIFY_IS_TRUE(plb.UpdateService(CreateServiceDescriptionWithPartitionAndReplicaCount(L"TestService1", L"TestType", true, 1, 3, CreateMetrics(L"Metric1/1.0/5/5"))).IsSuccess());
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));

        fm_->RefreshPLB(Stopwatch::Now());
        map<Guid, FailoverUnitMovement> const& actions = fm_->MoveActions;
        VERIFY_ARE_EQUAL(0u, actions.size());
        fm_->Clear();

        // stateful
        VERIFY_IS_FALSE(plb.UpdateService(CreateServiceDescriptionWithPartitionAndReplicaCount(L"TestService2", L"TestType", true, 1, 3, CreateMetrics(L"Metric1/1.0/6/6"))).IsSuccess());
        // stateless should also be rejected
        VERIFY_IS_FALSE(plb.UpdateService(CreateServiceDescriptionWithPartitionAndReplicaCount(L"TestService3", L"TestType", false, 1, 3, CreateMetrics(L"Metric1/1.0/6/6"))).IsSuccess());
        // No metric2 load yet
        VERIFY_IS_TRUE(plb.UpdateService(CreateServiceDescriptionWithPartitionAndReplicaCount(L"TestService4", L"TestType", true, 1, 3, CreateMetrics(L"Metric2/1.0/6/6"))).IsSuccess());
        // Node3 doesn't have capacity for Metrics3
        VERIFY_IS_TRUE(plb.UpdateService(CreateServiceDescriptionWithPartitionAndReplicaCount(L"TestService5", L"TestType", true, 1, 3, CreateMetrics(L"Metric3/1.0/11/11"))).IsSuccess());

    }

    BOOST_AUTO_TEST_CASE(UpdateServiceDynamicAffinityTest1)
    {
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        vector<ServiceMetric> metrics1 = CreateMetrics(L"Metric1/1.0/0/0");
        vector<ServiceMetric> metrics2 = CreateMetrics(L"Metric2/1.0/0/0");
        vector<ServiceMetric> metrics3 = CreateMetrics(L"Metric3/1.0/0/0");

        // s1 -> s2
        VERIFY_IS_TRUE(plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService1", L"TestType", true, L"TestService2", vector<ServiceMetric>(metrics1))).IsSuccess());
        VERIFY_IS_TRUE(plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService2", L"TestType", true, L"", vector<ServiceMetric>(metrics2))).IsSuccess());
        // DO not update with chain
        VERIFY_IS_FALSE(plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService2", L"TestType", true, L"TestService1", vector<ServiceMetric>(metrics2))).IsSuccess());

        // Do not allow chain when doing affinity dynamic update
        VERIFY_IS_TRUE(plb.UpdateService(CreateServiceDescription(L"TestService3", L"TestType", true, vector<ServiceMetric>(metrics3))).IsSuccess());
        // s2->s3, (s1->s2 existing)
        VERIFY_IS_TRUE(plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService2", L"TestType", true, L"TestService3", vector<ServiceMetric>(metrics2))).IsSuccess());
        // s3->s1 causing s3->s1->s2->s3 chain, so not allowed
        VERIFY_IS_FALSE(plb.UpdateService(CreateServiceDescriptionWithAffinity(L"TestService3", L"TestType", true, L"TestService1", vector<ServiceMetric>(metrics3))).IsSuccess());
    }

    BOOST_AUTO_TEST_CASE(UpdateServiceDynamicAffinityTest2)
    {
        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(SplitDomainEnabled, bool, true);
        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        vector<ServiceMetric> metrics1 = CreateMetrics(L"Metric1/1.0/0/0");
        vector<ServiceMetric> metrics2 = CreateMetrics(L"Metric2/1.0/0/0");
        vector<ServiceMetric> metrics3 = CreateMetrics(L"Metric3/1.0/0/0");

        plb.UpdateService(CreateServiceDescriptionWithAffinityWithEmptyApplication(L"UpdateServiceDynamicAffinityTest2TestService1", L"TestType", true, L"UpdateServiceDynamicAffinityTest2TestService2", vector<ServiceMetric>(metrics1)));
        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(L"UpdateServiceDynamicAffinityTest2TestService2", L"TestType", true, vector<ServiceMetric>(metrics2)));
        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());
        // Update existing service by changing its affinitized service to nothing
        plb.UpdateService(CreateServiceDescriptionWithAffinityWithEmptyApplication(L"UpdateServiceDynamicAffinityTest2TestService1", L"TestType", true, L"", vector<ServiceMetric>(metrics1)));
        // Service Domain split after updating affinity
        VERIFY_ARE_EQUAL(2u, plb.GetServiceDomains().size());
        // Change it back, domain should merge
        plb.UpdateService(CreateServiceDescriptionWithAffinityWithEmptyApplication(L"UpdateServiceDynamicAffinityTest2TestService1", L"TestType", true, L"UpdateServiceDynamicAffinityTest2TestService2", vector<ServiceMetric>(metrics1)));
        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());

        // Add a new service that affinitized some unknown service
        plb.UpdateService(CreateServiceDescriptionWithAffinityWithEmptyApplication(L"UpdateServiceDynamicAffinityTest2TestService3", L"TestType", true, L"Foo", vector<ServiceMetric>(metrics3)));
        VERIFY_ARE_EQUAL(2u, plb.GetServiceDomains().size());
        // Update affinity, all 3 services will be in 1 domain
        plb.UpdateService(CreateServiceDescriptionWithAffinityWithEmptyApplication(L"UpdateServiceDynamicAffinityTest2TestService3", L"TestType", true, L"UpdateServiceDynamicAffinityTest2TestService2", vector<ServiceMetric>(metrics3)));
        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());
    }

    BOOST_AUTO_TEST_CASE(UpdateServiceDynamicMetricTest1)
    {
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        vector<ServiceMetric> metrics1 = CreateMetrics(L"Metric1/1.0/0/0");
        vector<ServiceMetric> metrics2 = CreateMetrics(L"Metric2/1.0/0/0");
        vector<ServiceMetric> metrics3 = CreateMetrics(L"Metric3/1.0/0/0");
        vector<ServiceMetric> metrics12 = CreateMetrics(L"Metric1/1.0/0/0,Metric2/1.0/0/0");

        // Each service begin with their own metric, so total # of service domains should be 2
        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(L"TestService1", L"TestType", true, vector<ServiceMetric>(metrics1)));
        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(L"TestService2", L"TestType", true, vector<ServiceMetric>(metrics2)));
        VERIFY_ARE_EQUAL(2u, plb.GetServiceDomains().size());

        // Update service 1 to use metric2 and the service domain will be merged
        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(L"TestService1", L"TestType", true, vector<ServiceMetric>(metrics2)));
        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());

        // Change it back, domain should split
        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(L"TestService1", L"TestType", true, vector<ServiceMetric>(metrics1)));
        VERIFY_ARE_EQUAL(2u, plb.GetServiceDomains().size());

        // Add a new service3 with metric3. 3 service domains
        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(L"TestService3", L"TestType", true, vector<ServiceMetric>(metrics3)));
        VERIFY_ARE_EQUAL(3u, plb.GetServiceDomains().size());
        // Update service3 have metrics1 and metrics2. 3 services will be in one service domain
        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(L"TestService3", L"TestType", true, vector<ServiceMetric>(metrics12)));
        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());
    }

    BOOST_AUTO_TEST_CASE(UpdateServiceDynamicMetricTest2)
    {
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        vector<ServiceMetric> metrics1 = CreateMetrics(L"__MetricA__/1.0/1/0,__MetricB__/0.3/1/1");
        vector<ServiceMetric> metrics2 = CreateMetrics(L"__MetricC__/1.0/1/0,__MetricD__/0.3/1/1");

        plb.UpdateService(CreateServiceDescription(L"UpdateServiceDynamicMetricTest2_TestService1", L"TestType", true, vector<ServiceMetric>(metrics1)));
        plb.UpdateService(CreateServiceDescriptionWithAffinity(L"UpdateServiceDynamicMetricTest2_TestService2", L"TestType", false, L"UpdateServiceDynamicMetricTest2_TestService1", vector<ServiceMetric>(metrics2)));
        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"UpdateServiceDynamicMetricTest2_TestService1"), 0, CreateReplicas(L""), 4));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"UpdateServiceDynamicMetricTest2_TestService2"), 0, CreateReplicas(L""), 3));

        fm_->RefreshPLB(Stopwatch::Now());
        vector<wstring> actionList = GetActionListString(fm_->MoveActions);

        VERIFY_ARE_EQUAL(7u, actionList.size());

        plb.UpdateService(CreateServiceDescription(L"UpdateServiceDynamicMetricTest2_TestService1", L"TestType", true, vector<ServiceMetric>(metrics2)));

        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());
    }

    BOOST_AUTO_TEST_CASE(ApplicationGroupTest1)
    {
        wstring testName = L"ApplicationGroupTest1";

        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(SplitDomainEnabled, bool, true);

        VERIFY_ARE_EQUAL(0u, plb.GetServiceDomains().size());

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        wstring appName1 = wformatString("{0}Application1", testName);
        wstring serviceWithScaleoutCount1 = wformatString("{0}{1}ScaleoutCount", testName, L"1");
        wstring serviceWithScaleoutCount2 = wformatString("{0}{1}ScaleoutCount", testName, L"2");

        plb.UpdateService(CreateServiceDescriptionWithApplication(serviceWithScaleoutCount1, testType, appName1, true, CreateMetrics(L"MyMetric1/1.0/6/2")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(serviceWithScaleoutCount2, testType, appName1, true, CreateMetrics(L"MyMetric2/1.0/3/3")));
        // Without application, services are in different domain
        VERIFY_ARE_EQUAL(2u, plb.GetServiceDomains().size());

        int appScaleoutCount = 2;
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(wstring(appName1), appScaleoutCount, CreateApplicationCapacities(L"")));

        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());

        /*
        plb.DeleteService(L"TestService3");
        VERIFY_ARE_EQUAL(2u, plb.GetServiceDomains().size());
        */
    }

    BOOST_AUTO_TEST_CASE(ApplicationGroupTest2)
    {
        wstring testName = L"ApplicationGroupTest2";
        Trace.WriteInfo("PLBServiceDomainTestSource", "{0}", testName);
        PLBConfigScopeChange(SplitDomainEnabled, bool, true);
        PLBConfigScopeChange(UseAppGroupsInBoost, bool, false);
        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescription(0, L"Metric5/30"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("{0}_Application1", testName);
        // App capacity: [total, perNode, reserved] - total reserved capacity: 60
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // MinimumNodes - no reservation
            2,  // MaximumNodes
            CreateApplicationCapacities(L"AppMetric/100/50/10")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        //connected to metric2 via application...
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService",
            L"TestType",
            wstring(applicationName),
            true,
            CreateMetrics(L"ServiceMetric1/1.0/10/10")));
        //has metric 2, so connected to TestService...
        plb.UpdateService(CreateServiceDescription(L"TestService2", L"TestType", true, CreateMetrics(L"ServiceMetric2/1.0/0/0,AppMetric/1.0/0/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // we should have a single domain since all metrics connect somehow or another
        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // MinimumNodes - no reservation
            2,  // MaximumNodes
            CreateApplicationCapacities(L"")));
        // Now these should seperate...
        VERIFY_ARE_EQUAL(2u, plb.GetServiceDomains().size());


        //connected to metric2 via application...
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService",
            L"TestType",
            wstring(applicationName),
            true,
            CreateMetrics(L"ServiceMetric2/1.0/10/10")));
        // Now these should merge again...
        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());
    }

    BOOST_AUTO_TEST_CASE(ApplicationGroupTest3)
    {
        wstring testName = L"ApplicationGroupTest3";
        Trace.WriteInfo("PLBServiceDomainTestSource", "{0}", testName);
        PLBConfigScopeChange(SplitDomainEnabled, bool, true);
        PLBConfigScopeChange(UseAppGroupsInBoost, bool, false);
        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescription(0, L"Metric5/30"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("{0}_Application", testName);
        wstring applicationName2 = wformatString("{0}_Application2", testName);
        // App capacity: [total, perNode, reserved] - total reserved capacity: 60
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // MinimumNodes - no reservation
            2,  // MaximumNodes
            CreateApplicationCapacities(L"AppMetric/100/50/10")));
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName2,
            0,  // MinimumNodes - no reservation
            2,  // MaximumNodes
            CreateApplicationCapacities(L"AppMetric2/100/50/10")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService",
            L"TestType",
            wstring(applicationName),
            true,
            CreateMetrics(L"ServiceMetric1/1.0/10/10,ServiceMetric2/1.0/0/0")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService2",
            L"TestType",
            wstring(applicationName2),
            true,
            CreateMetrics(L"ServiceMetric3/1.0/10/10,ServiceMetric4/1.0/0/0")));
        //Force a domain merge....
        plb.UpdateService(CreateServiceDescription(L"TestService3", L"TestType", true, CreateMetrics(L"ServiceMetric1/1.0/10/10,ServiceMetric3/1.0/10/10")));
        //check if domains were merged fine...
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService4",
            L"TestType",
            wstring(applicationName),
            true,
            CreateMetrics(L"ServiceMetric/1.0/10/10")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService5",
            L"TestType",
            wstring(applicationName),
            true,
            CreateMetrics(L"ServiceMetric3/1.0/10/10,ServiceMetric4/1.0/0/0")));

        plb.UpdateService(CreateServiceDescription(L"TestService3", L"TestType", true, CreateMetrics(L"ServiceMetric2/1.0/0/0")));
        plb.UpdateService(CreateServiceDescription(L"TestService4", L"TestType", true, CreateMetrics(L"ServiceMetric3/1.0/0/0")));

        // we should have a single domain since all metrics connect somehow or another
        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService",
            L"TestType",
            wstring(applicationName),
            true,
            CreateMetrics(L"ServiceMetric1/1.0/10/10")));

        // TestService3 should become disconnected...
        VERIFY_ARE_EQUAL(2u, plb.GetServiceDomains().size());

        plb.DeleteService(wstring(L"TestService3"));
        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());

        plb.UpdateService(CreateServiceDescription(L"TestService3", L"TestType", true, CreateMetrics(L"ServiceMetric1/1.0/10/10,ServiceMetric3/1.0/10/10")));
        plb.DeleteService(wstring(L"TestService3"));
        //ServiceMetric1 & ServiceMetric3 are still connected via an application with scaleout...
        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());
    }

    BOOST_AUTO_TEST_CASE(ApplicationGroupTest4)
    {
        wstring testName = L"ApplicationGroupTest4";
        Trace.WriteInfo("PLBServiceDomainTestSource", "{0}", testName);
        PLBConfigScopeChange(SplitDomainEnabled, bool, true);
        PLBConfigScopeChange(UseAppGroupsInBoost, bool, false);
        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescription(0, L"Metric5/30"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("{0}_Application", testName);
        // App capacity: [total, perNode, reserved] - total reserved capacity: 60
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // MinimumNodes - no reservation
            2,  // MaximumNodes
            CreateApplicationCapacities(L"AppMetric/100/50/10")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService",
            L"TestType",
            wstring(applicationName),
            true,
            CreateMetrics(L"ServiceMetric1/1.0/10/10,ServiceMetric2/1.0/0/0,ServiceMetric7/1.0/10/10,ServiceMetric8/1.0/0/0")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService2",
            L"TestType",
            wstring(applicationName),
            true,
            CreateMetrics(L"ServiceMetric3/1.0/10/10,ServiceMetric4/1.0/0/0,ServiceMetric5/1.0/10/10,ServiceMetric6/1.0/0/0")));
        // we should have a single domain since all metrics connect somehow or another
        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());
    }

    BOOST_AUTO_TEST_CASE(ApplicationMinNodeCapacityReservationTest1)
    {
        // This scenario test capacity with min load reservation
        wstring testName = L"ApplicationMinNodeCapacityReservationTest1";

        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 5; i++)
        {
            // Total capacity is 50
            plb.UpdateNode(CreateNodeDescriptionWithFaultDomainAndCapacity(i, L"dc0", L"MyMetric/10"));
        }

        wstring testType = wformatString("{0}Type", testName);
        plb.UpdateServiceType(ServiceTypeDescription(wstring(testType), set<NodeId>()));

        int fuIndex(0);
        wstring dummyService1 = wformatString("{0}_dummy_1", testName);
        plb.UpdateService(CreateServiceDescription(dummyService1, testType, true, CreateMetrics(L"MyMetric/1.0/5/5")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex++), wstring(dummyService1), 0, CreateReplicas(L"P/1, S/2, S/3, S/4, S/5"), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring appName1 = wformatString("{0}Application1", testName);
        int minNodeCount = 3;
        int appScaleoutCount = 5;
        // App capacity: [total, perNode, reserved]
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(wstring(appName1),
            minNodeCount, appScaleoutCount, CreateApplicationCapacities(L"MyMetric/50/10/5")));

        // After the application creation, cluster remain resource is 50 - 25 - 3x5 = 10 and dummy service #2 should fail
        wstring dummyService2 = wformatString("{0}_dummy_2", testName);
        VERIFY_IS_FALSE(plb.UpdateService(ServiceDescription(wstring(dummyService2), wstring(testType), wstring(L""), true, wstring(L""),
            wstring(L""), true, CreateMetrics(L"MyMetric/1.0/5/5"), FABRIC_MOVE_COST_LOW, false, 1, 3)).IsSuccess());

        wstring serviceWithApp1 = wformatString("{0}_{1}_App", testName, L"1");
        // Application service creation should succeed
        VERIFY_IS_TRUE(plb.UpdateService(ServiceDescription(wstring(serviceWithApp1), wstring(testType), wstring(appName1), true, wstring(L""),
            wstring(L""), true, CreateMetrics(L"MyMetric/1.0/5/5"), FABRIC_MOVE_COST_LOW, false, 1, 5)).IsSuccess());

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(fuIndex++), wstring(serviceWithApp1), 0, CreateReplicas(L"P/1, S/2, S/3"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(dummyService1), 1, CreateReplicas(L""), 0));

        plb.ProcessPendingUpdatesPeriodicTask();

        // Now the remaining resource is 50 - 15 and there is no reserved load for application1

        wstring serviceWithApp2 = wformatString("{0}_{1}_App", testName, L"2");
        // Application service creation should succeed
        VERIFY_IS_TRUE(plb.UpdateService(ServiceDescription(wstring(serviceWithApp2), wstring(testType), wstring(appName1), true, wstring(L""),
            wstring(L""), true, CreateMetrics(L"MyMetric/1.0/5/5"), FABRIC_MOVE_COST_LOW, false, 1, 5)).IsSuccess());

        VERIFY_IS_TRUE(plb.UpdateService(ServiceDescription(wstring(dummyService2), wstring(testType), wstring(L""), true, wstring(L""),
            wstring(L""), true, CreateMetrics(L"MyMetric/1.0/5/5"), FABRIC_MOVE_COST_LOW, false, 1, 5)).IsSuccess());

    }

    BOOST_AUTO_TEST_CASE(AppGroupAndAffinityMetricConnectionTest)
    {
        wstring testName = L"AppGroupAndAffinityMetricConnectionTest";
        Trace.WriteInfo("PLBServiceDomainTestSource", "{0}", testName);
        PLBConfigScopeChange(SplitDomainEnabled, bool, true);
        PLBConfigScopeChange(UseAppGroupsInBoost, bool, false);
        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescription(0, L"Metric5/30"));

        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("{0}_Application", testName);
        // App capacity: [total, perNode, reserved] - total reserved capacity: 60
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // MinimumNodes - no reservation
            2,  // MaximumNodes
            CreateApplicationCapacities(L"AppMetric/100/50/10")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescription(L"TestService", L"TestType", true, CreateMetrics(L"ServiceMetric1/1.0/0/0")));
        //connected to 3 via app, and 1 via affinity
        plb.UpdateService(CreateServiceDescriptionWithAffinityAndApplication(L"TestService2", L"TestType", true, wstring(applicationName), L"TestService", CreateMetrics(L"ServiceMetric2/1.0/10/10")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService3",
            L"TestType",
            wstring(applicationName),
            true,
            CreateMetrics(L"ServiceMetric3/1.0/10/10")));


        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService3"), 0, CreateReplicas(L"P/0"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // we should have a single domain since all metrics connect somehow or another
        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // MinimumNodes - no reservation
            0,  // MaximumNodes - no scaleout...
            CreateApplicationCapacities(L"")));
        plb.ProcessPendingUpdatesPeriodicTask();
        //Now these should seperate...
        VERIFY_ARE_EQUAL(2u, plb.GetServiceDomains().size());
    }

    BOOST_AUTO_TEST_CASE(BasicRandomMetricConnectionTest)
    {
        wstring testName = L"BasicRandomMetricConnectionTest";
        Trace.WriteInfo("PLBServiceDomainTestSource", "{0}", testName);
        PLBConfigScopeChange(SplitDomainEnabled, bool, true);
        PLBConfigScopeChange(UseAppGroupsInBoost, bool, false);
        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescription(0, L"Metric5/30"));
        plb.ProcessPendingUpdatesPeriodicTask();

        int seed = PLBConfig::GetConfig().InitialRandomSeed;

        Trace.WriteInfo("PLBAppGroupsTestSource", "{0} with Seed:{1}", testName, seed);
        Common::Random random(seed);
        auto myFunc = [&random](size_t n) -> int
        {
            return random.Next(static_cast<int>(n));
        };

        int numberOfServices = 50;
        int metricPoolSize = 15;
        int maxMetrics = 5;
        vector<wstring> applicationPool;
        vector<wstring> metricPool;

        for (int i = 0; i < metricPoolSize; i++)
        {
            metricPool.push_back(wformatString("{0}_Metric_{1}", testName, i));
        }

        vector<set<wstring>> metricSets;

        for (int i = 0; i < numberOfServices; i++)
        {
            int numMetrics = (rand() % maxMetrics)+1;
            random_shuffle(metricPool.begin(), metricPool.end(), myFunc);

            wstring serviceType = wformatString("TestType_{0}",i);
            wstring serviceName = wformatString("TestService_{0}", i);
            plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));

            wstring metrics;
            set<wstring> metricSet;
            for (int j = 0; j < numMetrics; j++)
            {
                if (j == 0)
                {
                    metrics += wformatString("{0}/1.0/1/1", metricPool[j]);
                }
                else
                {
                    metrics += wformatString(",{0}/1.0/1/1", metricPool[j]);
                }
                metricSet.insert(wstring(metricPool[j]));
            }
            metricSets.push_back(metricSet);

            plb.UpdateService(CreateServiceDescription(wstring(serviceName),wstring(serviceType),true,CreateMetrics(metrics)));
        }

        plb.ProcessPendingUpdatesPeriodicTask();

        VERIFY_ARE_EQUAL(NumberOfDomains(metricSets), plb.GetServiceDomains().size());
    }

    BOOST_AUTO_TEST_CASE(AdvancedRandomMetricConnectionTest)
    {
        wstring testName = L"AdvancedRandomMetricConnectionTest";
        int seed = PLBConfig::GetConfig().InitialRandomSeed;
        Trace.WriteInfo("PLBAppGroupsTestSource", "{0} with Seed:{1}", testName, seed);
        PLBConfigScopeChange(SplitDomainEnabled, bool, true);
        PLBConfigScopeChange(UseAppGroupsInBoost, bool, false);
        PlacementAndLoadBalancing & plb = fm_->PLB;
        plb.UpdateNode(CreateNodeDescription(0, L"Metric5/30"));
        plb.ProcessPendingUpdatesPeriodicTask();
        Common::Random random(seed);
        auto myFunc = [&random](size_t n) -> int
        {
            return random.Next(static_cast<int>(n));
        };
        int numberOfServices = 50;
        // total number of applications is two times this, hald with scaleout/capacity, half without.
        int applicationPoolSize = 40;
        int metricPoolSize = 100;
        int maxMetrics = 4;
        static const int chanceList[] = { 0, 0, 1, 1 }; //0=simple,1=affinity
        //structures for defined applications...
        vector<wstring> applications;
        vector<wstring> applicationMetricStrings;
        vector<set<wstring>> applicationMetrics;
        //these are the pools we shuffle about to ensure random assignment of metrics/applications...
        std::vector<int> chancePool(chanceList, chanceList + sizeof(chanceList) / sizeof(chanceList[0]));
        vector<wstring> metricPool;
        for (int i = 0; i < metricPoolSize; i++)
        {
            metricPool.push_back(wformatString("M{1}", testName, i));
        }
        vector<int> applicationIdPool;
        //Adding applications with capacity...
        for (int i = 0; i < (applicationPoolSize); i++)
        {
            random_shuffle(metricPool.begin(), metricPool.end(), myFunc);
            //each application has just 1 metric for now.(Difficult to test applications with multiple metrics, and we already have other tests that do this.)
            set<wstring> metricSet;
            wstring metrics = wformatString("{0}/1.0/1/1", metricPool[0]);
            metricSet.insert(metricPool[0]);

            wstring appName = wformatString("A{0}", i);
            applications.push_back(appName);
            applicationIdPool.push_back(i);
            applicationMetrics.push_back(metricSet);
            applicationMetricStrings.push_back(metrics);

            plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
                wstring(appName),
                0,  // MinimumNodes - no reservation
                2,  // MaximumNodes
                CreateApplicationCapacities(wstring(metrics))));
        }
        //adding blank|noncapacity applications...
        for (int i = 0; i < (applicationPoolSize); i++)
        {
            applications.push_back(L"BlankApp");
            applicationIdPool.push_back(i);
        }

        //structure for storing service info...
        class ServiceInfo{
        public:
            wstring name;
            int applicationId;
            set<wstring> affinityMetrics;
            ServiceInfo(wstring name)
            {
                this->name = name;
                this->applicationId = -1;
                this->affinityMetrics = set<wstring>();
            }
        };
        //Storage structures...
        vector<ServiceInfo*> services;
        vector<set<wstring>> metricSets;
        vector<int> applicationServiceCount(2 * applicationPoolSize, 0);
        //initializers to ensure unique affinity and serviceIds...
        int nextServiceId = 0;
        int nextAffinityId = 0;
        wstring serviceType = L"TestType";
        //Common ServiceType for all services...
        plb.UpdateServiceType(ServiceTypeDescription(wstring(serviceType), set<NodeId>()));

        for (int i = 0; i < numberOfServices; i++)
        {
            //five-sixth, we spend adding services...
            if (random.Next() % 6 != 0)
            {
                random_shuffle(metricPool.begin(), metricPool.end(), myFunc);
                random_shuffle(applicationIdPool.begin(), applicationIdPool.end(), myFunc);
                random_shuffle(chancePool.begin(), chancePool.end(), myFunc);

                switch (chancePool[0])
                {
                case 0: //simple
                {
                    int numMetrics = random.Next(maxMetrics) + 1;
                    wstring metrics;
                    set<wstring> metricSet;
                    size_t place;
                    for (int j = 0; j < numMetrics; j++)
                    {
                        if (j == 0)
                        {
                            metrics += wformatString("{0}/1.0/1/1", metricPool[j]);
                        }
                        else
                        {
                            metrics += wformatString(",{0}/1.0/1/1", metricPool[j]);
                        }
                        metricSet.insert(metricPool[j]);
                    }
                    bool isNew = false;
                    //3/44 times, we make a new service....
                    if (rand() % 4 != 0 || services.empty())
                    {
                        isNew = true;
                        place = services.size();
                        services.push_back(new ServiceInfo(wformatString("{0}_TestService_{1}", random.Next() % 10, nextServiceId++)));
                        //add a dummy entry which will be replaced later :)
                        metricSets.push_back(set<wstring>());
                    }
                    else
                    {
                        place = random.Next() % services.size();
                    }

                    int appId = applicationIdPool[0];
                    auto service = services[place];
                    //keep the old app id
                    if (!isNew)
                    {
                        appId = service->applicationId;
                    }
                    plb.UpdateService(CreateServiceDescriptionWithApplication(wstring(service->name), wstring(serviceType), wstring(applications[appId]), true, CreateMetrics(metrics)));

                    if (applications[appId] != L"BlankApp")
                    {
                        metricSet.insert(applicationMetrics[appId].begin(), applicationMetrics[appId].end());
                        applicationServiceCount[appId]++;
                    }
                    metricSet.insert(service->affinityMetrics.begin(), service->affinityMetrics.end());
                    metricSets[place] = metricSet;
                    service->applicationId = appId;
                    break;
                }
                case 1: //affinity..
                {
                    int numMetrics1 = random.Next(maxMetrics) + 1;
                    int numMetrics2 = random.Next(maxMetrics) + 1;
                    wstring metrics1;
                    wstring metrics2;
                    set<wstring> metricSet1;
                    set<wstring> metricSet2;
                    size_t place;
                    for (int j = 0; j < numMetrics1; j++)
                    {
                        if (j == 0)
                        {
                            metrics1 += wformatString("{0}/1.0/1/1", metricPool[j]);
                        }
                        else
                        {
                            metrics1 += wformatString(",{0}/1.0/1/1", metricPool[j]);
                        }
                        metricSet1.insert(metricPool[j]);
                    }
                    random_shuffle(metricPool.begin(), metricPool.end(), myFunc);
                    for (int j = 0; j < numMetrics2; j++)
                    {
                        if (j == 0)
                        {
                            metrics2 += wformatString("{0}/1.0/1/1", metricPool[j]);
                        }
                        else
                        {
                            metrics2 += wformatString(",{0}/1.0/1/1", metricPool[j]);
                        }
                        metricSet2.insert(metricPool[j]);
                    }

                    //adding parent service...
                    //3/4 times, we make a new parent service....
                    if (rand() % 4 != 0 || services.empty())
                    {
                        place = services.size();
                        services.push_back(new ServiceInfo(wformatString("{0}_TestService_{1}", random.Next() % 10, nextServiceId++)));
                        //add a dummy entry which will be replaced later :)
                        metricSets.push_back(set<wstring>());
                    }
                    else
                    {
                        place = random.Next() % services.size();
                    }

                    auto service1 = services[place];
                    int appId1 = applicationIdPool[0];
                    plb.UpdateService(CreateServiceDescriptionWithApplication(wstring(service1->name), wstring(serviceType), wstring(applications[appId1]), true, CreateMetrics(metrics1)));
                    service1->applicationId = appId1;

                    //adding child service...
                    //the child service HAS to be new, in order to avoid affinity chains...
                    services.push_back(new ServiceInfo(wformatString("{0}_TestService_{1}", random.Next() % 10, nextServiceId++)));
                    auto service2 = services.back();
                    int appId2 = applicationIdPool[1];
                    plb.UpdateService(CreateServiceDescriptionWithAffinityAndApplication(wstring(service2->name), wstring(serviceType), true, wstring(applications[appId2]), wstring(service1->name), CreateMetrics(metrics2)));
                    service2->applicationId = appId2;

                    if (applications[appId1] != L"BlankApp")
                    {
                        metricSet1.insert(applicationMetrics[appId1].begin(), applicationMetrics[appId1].end());
                        applicationServiceCount[appId1]++;
                    }
                    if (applications[appId2] != L"BlankApp")
                    {
                        metricSet2.insert(applicationMetrics[appId2].begin(), applicationMetrics[appId2].end());
                        applicationServiceCount[appId2]++;
                    }

                    wstring affinityIdentifier = wformatString("Aff{0}", nextAffinityId++);
                    service1->affinityMetrics.insert(affinityIdentifier);
                    //parent gets it's new and old affinities...
                    metricSet1.insert(service1->affinityMetrics.begin(), service1->affinityMetrics.end());
                    //child gets only the current affinity...
                    metricSet2.insert(affinityIdentifier);

                    metricSets[place] = metricSet1;
                    metricSets.push_back(metricSet2);
                    break;
                }
                }
                if (NumberOfDomains(metricSets) != plb.GetServiceDomains().size())
                {
                    Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", metricSets);
                }
                VERIFY_ARE_EQUAL(NumberOfDomains(metricSets), plb.GetServiceDomains().size());
            }
            else
                //one-sixth we spend removing...
            {
                if (!services.empty())
                {
                    //randomly pick service to remove...
                    auto place = random.Next() % services.size();
                    auto service = services[place];
                    //delete the service
                    {
                        plb.DeleteService(service->name);
                        applicationServiceCount[service->applicationId]--;
                    }

                    //cleanup :)
                    delete service;
                    //removing the service info...
                    metricSets.erase(metricSets.begin() + place);
                    services.erase(services.begin() + place);

                    if (!services.empty())
                    {
                        if (NumberOfDomains(metricSets) != plb.GetServiceDomains().size())
                        {
                            Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", metricSets);
                        }
                        VERIFY_ARE_EQUAL(NumberOfDomains(metricSets), plb.GetServiceDomains().size());
                    }
                }
            }
        }
        //next, we remove the rest...
        while (!services.empty())
        {
            //randomly pick service to remove...
            auto place = random.Next() % services.size();
            auto service = services[place];
            {
                plb.DeleteService(service->name);

                applicationServiceCount[service->applicationId]--;
                if (applicationServiceCount[service->applicationId] == 0 && applications[service->applicationId] != L"BlankApp")
                {
                    //we may or may not decide to delete the application...
                    if (random.Next() % 2)
                    {
                        plb.DeleteApplication(applications[service->applicationId]);
                    }
                }
            }

            delete service;
            metricSets.erase(metricSets.begin() + place);
            services.erase(services.begin() + place);
            if (!services.empty())
            {
                if (NumberOfDomains(metricSets) != plb.GetServiceDomains().size())
                {
                    Trace.WriteInfo("PLBAppGroupsTestSource", "{0}", metricSets);
                }
                VERIFY_ARE_EQUAL(NumberOfDomains(metricSets), plb.GetServiceDomains().size());
            }
        }
    }
    
    BOOST_AUTO_TEST_CASE(ApplicationLoadTestAssertMergeDomainWithMetricsAndApplication)
    {
        // testing application load consistency when merging domains due to application update with certain metrics
        // verifying accumulated node load for certain materic and application
        wstring testName = L"ApplicationLoadTestAssertMergeDomainWithMetricsAndApplication";
        Trace.WriteInfo("PLBPlacementTestSource", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(UseAppGroupsInBoost, bool, false);
        // Enable split domain logic
        PLBConfigScopeChange(SplitDomainEnabled, bool, true);
        VERIFY_ARE_EQUAL(0u, plb.GetServiceDomains().size());

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }
        wstring applicationName1 = wformatString("{0}_Application1", testName);
        wstring applicationName2 = wformatString("{0}_Application2", testName);

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName1,
            0,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"Metric1/500/100/0")));

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName2,
            0,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"Metric3/500/100/0")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        // Create two services in two different applications
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestType", applicationName1, true, CreateMetrics(L"Metric1/1.0/25/10,Metric2/1.0/20/15")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"TestType", applicationName2, true, CreateMetrics(L"Metric3/1.0/30/20,Metric4/1.0/35/25")));

        // Update failover units of those created services and force AddNodeLoad in failover unit update
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Check if there the number of service domains is 2
        VERIFY_ARE_EQUAL(2u, plb.GetServiceDomains().size());

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService3", L"TestType", applicationName1, true, CreateMetrics(L"Metric1/1.0/40/35,Metric4/1.0/35/20")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService3"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Check if there the number of service domains is 1
        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 1, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 1, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService3"), 1, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // verifying accumulated node load for certain materic and application
        // also check the size of applicationLoadTable_ in ServiceDomain to see if merging was successful
        VerifyApplicationSumLoad(applicationName1, L"Metric1", 155, 2);
        VerifyApplicationSumLoad(applicationName1, L"Metric2", 50, 2);
        VerifyApplicationSumLoad(applicationName1, L"Metric4", 75, 2);


        VerifyApplicationSumLoad(applicationName2, L"Metric3", 70, 2);
        VerifyApplicationSumLoad(applicationName2, L"Metric4", 85, 2);
    }

    BOOST_AUTO_TEST_CASE(ApplicationLoadTestAssertMergeDomainWithAffinity)
    {
        //testing application load consistency when merging domains due to affinity between services
        wstring testName = L"ApplicationLoadTestAssertMergeDomainWithAffinity";
        Trace.WriteInfo("TestServiceDomain", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(UseAppGroupsInBoost, bool, false);
        // Enable split domain logic
        PLBConfigScopeChange(SplitDomainEnabled, bool, true);
        VERIFY_ARE_EQUAL(0u, plb.GetServiceDomains().size());

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }
        wstring applicationName1 = wformatString("{0}_Application1", testName);
        wstring applicationName2 = wformatString("{0}_Application2", testName);

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName1,
            0,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"Metric1/500/100/0")));

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName2,
            0,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"Metric3/500/100/0")));

        plb.ProcessPendingUpdatesPeriodicTask();

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        // Create two services in two different applications
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestType", applicationName1, true, CreateMetrics(L"Metric1/1.0/25/10,Metric2/1.0/20/15")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"TestType", applicationName2, true, CreateMetrics(L"Metric3/1.0/30/20,Metric4/1.0/35/25")));

        // Update failover units of those created services and force AddNodeLoad in failover unit update
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Check if there the number of service domains is 2
        VERIFY_ARE_EQUAL(2u, plb.GetServiceDomains().size());

        plb.UpdateService(CreateServiceDescriptionWithAffinityAndApplication(L"TestService3", L"TestType", true, L"", L"TestService1", CreateMetrics(L"Metric4/1.0/35/20,Metric5/1.0/20/15")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService3"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Check if there the number of service domains is 1
        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 1, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 1, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService3"), 1, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // verifying accumulated node load for certain materic and application
        // also check the size of applicationLoadTable_ in ServiceDomain to see if merging was successful
        VerifyApplicationSumLoad(applicationName1, L"Metric1", 45, 2);
        VerifyApplicationSumLoad(applicationName1, L"Metric2", 50, 2);

        VerifyApplicationSumLoad(applicationName2, L"Metric3", 70, 2);
        VerifyApplicationSumLoad(applicationName2, L"Metric4", 85, 2);
    }

    BOOST_AUTO_TEST_CASE(ApplicationLoadTestAssertMergeSplitDomain)
    {
        // testing application load consistency when merging and spliting domains
        wstring testName = L"ApplicationLoadTestAssertMergeSplitDomain";
        Trace.WriteInfo("TestServiceDomain", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(UseAppGroupsInBoost, bool, false);
        // Enable split domain logic
        PLBConfigScopeChange(SplitDomainEnabled, bool, true);
        VERIFY_ARE_EQUAL(0u, plb.GetServiceDomains().size());

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }
        wstring applicationName1 = wformatString("{0}_Application1", testName);
        wstring applicationName2 = wformatString("{0}_Application2", testName);
        wstring applicationName3 = wformatString("{0}_Application3", testName);

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName1,
            0,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"Metric1/500/100/0")));

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName2,
            0,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"Metric3/500/100/0")));

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName3,
            0,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"Metric5/500/100/0")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        // Create two services in two different applications
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestType", applicationName1, true, CreateMetrics(L"Metric1/1.0/25/10,Metric2/1.0/20/15")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"TestType", applicationName2, true, CreateMetrics(L"Metric3/1.0/30/20,Metric4/1.0/35/25")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService3", L"TestType", applicationName3, true, CreateMetrics(L"Metric1/1.0/40/35,Metric4/1.0/35/20")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService3"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Check if there the number of service domains is 1
        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());

        //verifying accumulated node load for certain materic and application
        // also check the size of applicationLoadTable_ in ServiceDomain to see if merging was successful
        VerifyApplicationSumLoad(applicationName1, L"Metric1", 45, 3);
        VerifyApplicationSumLoad(applicationName1, L"Metric2", 50, 3);

        VerifyApplicationSumLoad(applicationName2, L"Metric3", 70, 3);
        VerifyApplicationSumLoad(applicationName2, L"Metric4", 85, 3);

        VerifyApplicationSumLoad(applicationName3, L"Metric1", 110, 3);
        VerifyApplicationSumLoad(applicationName3, L"Metric4", 75, 3);

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService3", L"TestType", applicationName3, true, CreateMetrics(L"Metric5/1.0/40/35")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 1, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 1, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService3"), 1, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Check if there the number of service domains is 3
        VERIFY_ARE_EQUAL(3u, plb.GetServiceDomains().size());
        
        // verifying accumulated node load for certain materic and application
        // also check the size of applicationLoadTable_ in ServiceDomain to see if spliting was successful
        VerifyApplicationSumLoad(applicationName1, L"Metric1", 45, 1);
        VerifyApplicationSumLoad(applicationName1, L"Metric2", 50, 1);

        VerifyApplicationSumLoad(applicationName2, L"Metric3", 70, 1);
        VerifyApplicationSumLoad(applicationName2, L"Metric4", 85, 1);

        VerifyApplicationSumLoad(applicationName3, L"Metric1", -1, 1);
        VerifyApplicationSumLoad(applicationName3, L"Metric4", -1, 1);
        VerifyApplicationSumLoad(applicationName3, L"Metric5", 110, 1);
    }

    BOOST_AUTO_TEST_CASE(ApplicationLoadTestAssertMergeSplitDomainApplicationResevation)
    {
        // testing application load consistency when merging and spliting domains due to update of application reservation
        wstring testName = L"ApplicationLoadTestAssertMergeSplitDomainApplicationResevation";
        Trace.WriteInfo("TestServiceDomain", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(UseAppGroupsInBoost, bool, false);
        // Enable split domain logic
        PLBConfigScopeChange(SplitDomainEnabled, bool, true);
        VERIFY_ARE_EQUAL(0u, plb.GetServiceDomains().size());

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }
        wstring applicationName1 = wformatString("{0}_Application1", testName);
        wstring applicationName2 = wformatString("{0}_Application2", testName);
        wstring applicationName3 = wformatString("{0}_Application3", testName);

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName1,
            0,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"Metric1/500/100/0,Metric2/400/70/0")));

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName2,
            0,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"Metric3/500/100/0")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        // Create two services in two different applications
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestType", applicationName1, true, CreateMetrics(L"Metric1/1.0/25/10,Metric2/1.0/20/15")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"TestType", applicationName2, true, CreateMetrics(L"Metric3/1.0/30/20,Metric4/1.0/35/25")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        VERIFY_ARE_EQUAL(2u, plb.GetServiceDomains().size());

        VerifyApplicationSumLoad(applicationName1, L"Metric1", 45, 1);
        VerifyApplicationSumLoad(applicationName1, L"Metric2", 50, 1);

        VerifyApplicationSumLoad(applicationName2, L"Metric3", 70, 1);
        VerifyApplicationSumLoad(applicationName2, L"Metric4", 85, 1);

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName1,
            0,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"Metric1/500/100/0,Metric2/400/70/0,Metric3/500/50/2")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 1, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 1, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());

        VerifyApplicationSumLoad(applicationName1, L"Metric1", 45, 2);
        VerifyApplicationSumLoad(applicationName1, L"Metric2", 50, 2);

        VerifyApplicationSumLoad(applicationName2, L"Metric3", 70, 2);
        VerifyApplicationSumLoad(applicationName2, L"Metric4", 85, 2);

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName1,
            0,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"Metric1/500/100/0,Metric2/400/70/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 1, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 1, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();
        
        VERIFY_ARE_EQUAL(2u, plb.GetServiceDomains().size());

        VerifyApplicationSumLoad(applicationName1, L"Metric1", 45, 1);
        VerifyApplicationSumLoad(applicationName1, L"Metric2", 50, 1);

        VerifyApplicationSumLoad(applicationName2, L"Metric3", 70, 1);
        VerifyApplicationSumLoad(applicationName2, L"Metric4", 85, 1);
    }

    BOOST_AUTO_TEST_CASE(ApplicationLoadTestAssertMergeSplitDomainApplicationResevation2)
    {
        wstring testName = L"ApplicationLoadTestAssertMergeSplitDomainApplicationResevation2";
        Trace.WriteInfo("TestServiceDomain", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(UseAppGroupsInBoost, bool, false);
        // Enable split domain logic
        PLBConfigScopeChange(SplitDomainEnabled, bool, true);
        VERIFY_ARE_EQUAL(0u, plb.GetServiceDomains().size());

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }
        wstring applicationName1 = wformatString("{0}_Application1", testName);
        wstring applicationName2 = wformatString("{0}_Application2", testName);
        wstring applicationName3 = wformatString("{0}_Application3", testName);

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName1,
            0,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"")));

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName2,
            0,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"Metric3/500/100/0")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        // Create two services in two different applications
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestType", applicationName1, true, CreateMetrics(L"Metric1/1.0/25/10,Metric2/1.0/20/15")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"TestType", applicationName2, true, CreateMetrics(L"Metric3/1.0/30/20,Metric4/1.0/35/25")));

        // Update failover units of those created services and force AddNodeLoad in failover unit update
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        VERIFY_ARE_EQUAL(2u, plb.GetServiceDomains().size());

        VerifyApplicationSumLoad(applicationName1, L"Metric1", 45, 1);
        VerifyApplicationSumLoad(applicationName1, L"Metric2", 50, 1);

        VerifyApplicationSumLoad(applicationName2, L"Metric3", 70, 1);
        VerifyApplicationSumLoad(applicationName2, L"Metric4", 85, 1);

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName1,
            2,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"Metric3/500/50/2")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 1, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 1, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());

        VerifyApplicationSumLoad(applicationName1, L"Metric1", 45, 2);
        VerifyApplicationSumLoad(applicationName1, L"Metric2", 50, 2);

        VerifyApplicationSumLoad(applicationName2, L"Metric3", 70, 2);
        VerifyApplicationSumLoad(applicationName2, L"Metric4", 85, 2);

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName1,
            2,  // MinimumNodes
            2,  // MaximumNodes
            CreateApplicationCapacities(L"Metric1/500/100/0,Metric2/400/70/0,Metric3/500/50/0")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 1, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 1, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());

        VerifyApplicationSumLoad(applicationName1, L"Metric1", 45, 2);
        VerifyApplicationSumLoad(applicationName1, L"Metric2", 50, 2);

        VerifyApplicationSumLoad(applicationName2, L"Metric3", 70, 2);
        VerifyApplicationSumLoad(applicationName2, L"Metric4", 85, 2);
    }

    BOOST_AUTO_TEST_CASE(MergeDomainsWithoutAppGroups)
    {
        // testing the merging domains scenario
        // with application that doesn't have neither metrics nor scaleout => non-appgroup application
        // first one domain contains Memory metric and StatelessParent service 
        // the other domain contains CPU metric and VolatileParent service

        // Case1: Creation of StatelessChild that is connected with affinity with VolatileParent and has Memory metric
        // triggers merging these two domains => 1 domain

        wstring testName = L"MergeDomainsWithoutAppGroups";
        Trace.WriteInfo("TestServiceDomain", "{0}", testName);
        PLBConfigScopeChange(UseAppGroupsInBoost, bool, true);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        plb.UpdateNode(CreateNodeDescription(0, L"CPU/30"));
        plb.UpdateNode(CreateNodeDescription(1, L"CPU/80"));
        plb.UpdateNode(CreateNodeDescription(2, L"CPU/80"));
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("{0}_App", testName);
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            0,  // No Scaleout
            CreateApplicationCapacities(L"")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"StatelessServiceType"), set<NodeId>()));
        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"VolatileServiceType"), set<NodeId>()));

        // Create stateless parent service
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"StatelessParent",
            L"StatelessServiceType",
            applicationName,
            false,
            CreateMetrics(L"Memory/1.0/15/15")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"StatelessParent"), 0, CreateReplicas(L"I/0"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());
        VerifyApplicationSumLoad(applicationName, L"Memory", 15, 1);

        // Create volatile parent service with CPU metrics
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"VolatileParent",
            L"VolatileServiceType",
            applicationName,
            true,
            CreateMetrics(L"CPU/1.0/10/10")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"VolatileParent"), 0, CreateReplicas(L"P/0"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        VERIFY_ARE_EQUAL(2u, plb.GetServiceDomains().size());
        VerifyApplicationSumLoad(applicationName, L"CPU", 10, 1);

        plb.UpdateService(CreateServiceDescriptionWithAffinityAndApplication(
            L"StatelessChild",
            L"StatelessServiceType",
            false,
            applicationName,
            L"VolatileParent",
            CreateMetrics(L"Memory/1.0/15/15")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"StatelessChild"), 0, CreateReplicas(L"I/0"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());
        VerifyApplicationSumLoad(applicationName, L"Memory", 30, 1);
        VerifyApplicationSumLoad(applicationName, L"CPU", 10, 1);
    }

    BOOST_AUTO_TEST_CASE(MergeDomainsWithoutAppGroupsWithAffinityAndMetric)
    {
        // testing different scenarios of merging domains using affinity and metric connections
        // there are 3 non-appgroup applications(no metrics and no scaleout)
        // there are 3 service with 3 different metrics and they all belong to different application => 3 domains

        // Case1: Created new Service4 that is connected with affinity toward Service2 and has metric M3
        // this creation merged 3services(Service2, Service3 & Service4) => now we have 2 domains

        // Case2: Update application1 with M1 metric and this will merge all domains => 1 domain

        wstring testName = L"MergeDomainsWithoutAppGroupsWithAffinityAndMetric";
        Trace.WriteInfo("TestServiceDomain", "{0}", testName);
        PlacementAndLoadBalancing & plb = fm_->PLB;

        for (int i = 0; i < 3; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName1 = wformatString("{0}_App1", testName);
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName1,
            0,  // No Scaleout
            CreateApplicationCapacities(L"")));

        wstring applicationName2 = wformatString("{0}_App2", testName);
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName2,
            0,  // No Scaleout
            CreateApplicationCapacities(L"")));

        wstring applicationName3 = wformatString("{0}_App3", testName);
        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName3,
            0,  // No Scaleout
            CreateApplicationCapacities(L"")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"ServiceType"), set<NodeId>()));

        // Create service1
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"Service1",
            L"ServiceType",
            applicationName1,
            true,
            CreateMetrics(L"M1/1.0/50/30")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"Service1"), 0, CreateReplicas(L"P/0"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Create service2
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"Service2",
            L"ServiceType",
            applicationName2,
            false,
            CreateMetrics(L"M2/1.0/25/25")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"Service2"), 0, CreateReplicas(L"I/0"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        // Create service3
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"Service3",
            L"ServiceType",
            applicationName3,
            true,
            CreateMetrics(L"M3/1.0/45/45")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"Service3"), 0, CreateReplicas(L"P/0"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        VERIFY_ARE_EQUAL(3u, plb.GetServiceDomains().size());
        VerifyApplicationSumLoad(applicationName1, L"M1", 50, 1);
        VerifyApplicationSumLoad(applicationName2, L"M2", 25, 1);
        VerifyApplicationSumLoad(applicationName3, L"M3", 45, 1);

        // Create volatile parent service with CPU metrics
        plb.UpdateService(CreateServiceDescriptionWithAffinityAndApplication(
            L"Service4",
            L"ServiceType",
            false,
            applicationName1,
            L"Service2",
            CreateMetrics(L"M3/1.0/10/10")));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(3), wstring(L"Service4"), 0, CreateReplicas(L"I/0"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        VERIFY_ARE_EQUAL(2u, plb.GetServiceDomains().size());
        // domain1
        VerifyApplicationSumLoad(applicationName1, L"M1", 50, 1);

        // in domain2 -> load for metric M1,M2 & M3 => size = 3
        VerifyApplicationSumLoad(applicationName2, L"M2", 25, 3);
        VerifyApplicationSumLoad(applicationName3, L"M3", 45, 3);

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName1,
            0,  // No Scaleout
            CreateApplicationCapacities(L"M1/1.0/30/20")));

        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());
        // in domain -> load for metric M1,M2 & M3 => size = 3
        VerifyApplicationSumLoad(applicationName1, L"M1", 50, 3);
        VerifyApplicationSumLoad(applicationName1, L"M3", 10, 3);
        VerifyApplicationSumLoad(applicationName2, L"M2", 25, 3);
        VerifyApplicationSumLoad(applicationName3, L"M3", 45, 3);
    }

    BOOST_AUTO_TEST_CASE(MergeSplitDomainsCheckAppLoad)
    {
        // triggering merging and splitting through metric connections

        // first we have 2 domains:
        // Domain1: M1,M2 metric and application1(since App1 is appgroup)
        // Domain1: M3,M4 metric and application2(since App2 is appgroup)

        // Case1: Update App2 to have M1 metric => triggers merging domains => 1 domain

        // Case2: Update Service that contains only M2 metric => triggers splitting domains => 2 domains

        wstring testName = L"MergeSplitDomainsCheckAppLoad";
        Trace.WriteInfo("TestServiceDomain", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(UseAppGroupsInBoost, bool, false);
        // Enable split domain logic
        PLBConfigScopeChange(SplitDomainEnabled, bool, true);
        VERIFY_ARE_EQUAL(0u, plb.GetServiceDomains().size());

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }
        wstring applicationName1 = wformatString("{0}_Application1", testName);
        wstring applicationName2 = wformatString("{0}_Application2", testName);

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName1,
            2,  // MinimumNodes
            3,  // MaximumNodes
            CreateApplicationCapacities(L"")));

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName2,
            2,  // MinimumNodes
            3,  // MaximumNodes
            CreateApplicationCapacities(L"")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        // Create two services in two different applications
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestType", applicationName1, true, CreateMetrics(L"Metric1/1.0/25/10,Metric2/1.0/20/15")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"TestType", applicationName2, true, CreateMetrics(L"Metric3/1.0/30/20,Metric4/1.0/35/25")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        VERIFY_ARE_EQUAL(2u, plb.GetServiceDomains().size());

        VerifyApplicationSumLoad(applicationName1, L"Metric1", 45, 1);
        VerifyApplicationSumLoad(applicationName1, L"Metric2", 50, 1);

        VerifyApplicationSumLoad(applicationName2, L"Metric3", 70, 1);
        VerifyApplicationSumLoad(applicationName2, L"Metric4", 85, 1);

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName2,
            2,  // MinimumNodes
            4,  // MaximumNodes
            CreateApplicationCapacities(L"Metric1/500/50/4")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 1, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 1, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());

        VerifyApplicationSumLoad(applicationName1, L"Metric1", 45, 2);
        VerifyApplicationSumLoad(applicationName1, L"Metric2", 50, 2);

        VerifyApplicationSumLoad(applicationName2, L"Metric3", 70, 2);
        VerifyApplicationSumLoad(applicationName2, L"Metric4", 85, 2);

        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService1", L"TestType", applicationName1, true, CreateMetrics(L"Metric2/1.0/20/10")));
        plb.UpdateService(CreateServiceDescriptionWithApplication(L"TestService2", L"TestType", applicationName2, true, CreateMetrics(L"Metric3/1.0/30/20,Metric5/1.0/35/25")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 1, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 1, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        VERIFY_ARE_EQUAL(2u, plb.GetServiceDomains().size());

        VerifyApplicationSumLoad(applicationName1, L"Metric2", 40, 1);
        VerifyApplicationSumLoad(applicationName1, L"Metric1", -1, 1);

        VerifyApplicationSumLoad(applicationName2, L"Metric1", -1, 1);
        VerifyApplicationSumLoad(applicationName2, L"Metric3", 70, 1);
        VerifyApplicationSumLoad(applicationName2, L"Metric4", -1, 1);
        VerifyApplicationSumLoad(applicationName2, L"Metric5", 85, 1);
    }

    BOOST_AUTO_TEST_CASE(AppChangesDomainWithRemovingMetricsWithoutServices)
    {
        // case where application has some metrics and belongs to one domain
        // Application doesn't have any services
        // with removing one metric, application is changing domain

        // Application has metric M1 and M2
        // Domain1: this Application(M1 & M2) and Service1(M1)
        // Domain2: Service2(M3)

        // Case1: Update Application to have metrics M2 and M3 => app changes domain
        // Domai1: Service1(M1)
        // Domain2: Application(M2 & M3) and Service2(M3)

        // Case2: Update Application to have metrics M1, M2 and M3 => merge domains
        // 1 domain(Service1(M1), Service2(M3), App(M1,M2,M3))

        wstring testName = L"AppChangesDomainWithRemovingMetricsWithoutServices";
        Trace.WriteInfo("TestServiceDomain", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(UseAppGroupsInBoost, bool, false);
        // Enable split domain logic
        PLBConfigScopeChange(SplitDomainEnabled, bool, true);
        VERIFY_ARE_EQUAL(0u, plb.GetServiceDomains().size());

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("{0}_Application", testName);

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            2,  // MinimumNodes
            3,  // MaximumNodes
            CreateApplicationCapacities(L"M1/500/100/20,M2/400/50/30")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(
            L"TestService1", 
            L"TestType", 
            true,
            CreateMetrics(L"M1/1.0/25/10")));

        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(
            L"TestService2", 
            L"TestType", 
            true, 
            CreateMetrics(L"M3/1.0/25/10")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        VERIFY_ARE_EQUAL(2u, plb.GetServiceDomains().size());

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            2,  // MinimumNodes
            3,  // MaximumNodes
            CreateApplicationCapacities(L"M2/500/100/20,M3/300/50/30")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 1, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 1, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        VERIFY_ARE_EQUAL(2u, plb.GetServiceDomains().size());

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            2,  // MinimumNodes
            3,  // MaximumNodes
            CreateApplicationCapacities(L"M1/400/20/10,M2/500/100/20,M3/300/50/30")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 1, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 1, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        VERIFY_ARE_EQUAL(1u, plb.GetServiceDomains().size());
    }

    BOOST_AUTO_TEST_CASE(AppChangesDomainWithRemovingMetrics)
    {
        // case where application has some metrics and belongs to one domain
        // Application(M2) has a service with metric(M1)
        // with removing one metric, application is changing domain

        // Application has metric M2 and Service1 with metric M1
        // Another Service2 with metric M2 
        // => they are in Domain1

        // in Domain2 => Service3 with metric M3

        // Case: change application metric to M3 and this cause application to change domain
        // Domain1: Service2(M2)
        // Domain2: Application(M3) + Service1(M1, belongs to the app) + Servic3(M3)

        wstring testName = L"AppChangesDomainWithRemovingMetrics";
        Trace.WriteInfo("TestServiceDomain", "{0}", testName);

        PlacementAndLoadBalancing & plb = fm_->PLB;
        PLBConfigScopeChange(UseAppGroupsInBoost, bool, false);
        // Enable split domain logic
        PLBConfigScopeChange(SplitDomainEnabled, bool, true);
        VERIFY_ARE_EQUAL(0u, plb.GetServiceDomains().size());

        for (int i = 0; i < 5; i++)
        {
            plb.UpdateNode(CreateNodeDescription(i));
        }
        plb.ProcessPendingUpdatesPeriodicTask();

        wstring applicationName = wformatString("{0}_Application", testName);

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            2,  // MinimumNodes
            3,  // MaximumNodes
            CreateApplicationCapacities(L"M2/500/100/20")));

        plb.UpdateServiceType(ServiceTypeDescription(wstring(L"TestType"), set<NodeId>()));

        // Service1 that belongs to application and has M1 metric
        plb.UpdateService(CreateServiceDescriptionWithApplication(
            L"TestService1",
            L"TestType",
            applicationName,
            true,
            CreateMetrics(L"M1/1.0/25/10")));

        // Service2 with metric M2 - no app
        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(
            L"TestService2",
            L"TestType",
            true,
            CreateMetrics(L"M2/1.0/25/10")));

        // Service3 with metric M3 - no app
        plb.UpdateService(CreateServiceDescriptionWithEmptyApplication(
            L"TestService3",
            L"TestType",
            true,
            CreateMetrics(L"M3/1.0/25/10")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService3"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        VERIFY_ARE_EQUAL(2u, plb.GetServiceDomains().size());
        VerifyApplicationSumLoad(applicationName, L"M1", 45, 1);

        plb.UpdateApplication(CreateApplicationDescriptionWithCapacities(
            applicationName,
            2,  // MinimumNodes
            3,  // MaximumNodes
            CreateApplicationCapacities(L"M3/300/50/30")));

        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(0), wstring(L"TestService1"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(1), wstring(L"TestService2"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.UpdateFailoverUnit(FailoverUnitDescription(CreateGuid(2), wstring(L"TestService3"), 0, CreateReplicas(L"P/0, S/1, S/2"), 0));
        plb.ProcessPendingUpdatesPeriodicTask();

        VERIFY_ARE_EQUAL(2u, plb.GetServiceDomains().size());
        VerifyApplicationSumLoad(applicationName, L"M1", 45, 1);
        VerifyApplicationSumLoad(applicationName, L"M3", -1, 1);
    }

    BOOST_AUTO_TEST_SUITE_END()

    void TestServiceDomain::VerifyApplicationSumLoad(wstring const& appName, wstring const& metricName, int64 expectedLoad, int64 expectedSize)
    {
        PlacementAndLoadBalancingTestHelper const& plbTestHelper = fm_->PLBTestHelper;
        int64 sumAppLoad(0);
        int64 size(0);

        plbTestHelper.GetApplicationSumLoadAndCapacity(appName, metricName, sumAppLoad, size);

        Trace.WriteInfo("TestServiceDomain", "VerifyApplicationSumLoad: For application: {0} (expected sum load: {1}), sumLoad: {2}",
            appName,
            expectedLoad,
            sumAppLoad);

        VERIFY_ARE_EQUAL(sumAppLoad, expectedLoad);
        VERIFY_ARE_EQUAL(size, expectedSize);
    }

    bool TestServiceDomain::ClassSetup()
    {
        fm_ = make_shared<TestFM>();

        return TRUE;
    }

    bool TestServiceDomain::TestSetup()
    {
        fm_->Load();

        return TRUE;
    }

    bool TestServiceDomain::ClassCleanup()
    {
        Trace.WriteInfo("TestServiceDomain", "Cleaning up the class.");

        // Dispose PLB
        fm_->PLBTestHelper.Dispose();

        return TRUE;
    }

}
