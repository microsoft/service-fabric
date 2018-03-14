// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace ServiceModelTests
{
    using namespace std;
    using namespace Common;
    using namespace ServiceModel;
    using namespace Federation;
    using namespace Naming;
    using namespace Reliability;
    using namespace Management::HealthManager;

    StringLiteral const TestSource("SizeEstimatorTest");

    class SizeEstimatorTest
    {
    protected:
        template <class T>
        static void CheckSize(T const & obj, wstring const & objType);

        static void CheckSize(size_t expected, size_t actual, wstring const & objType);

        static vector<HealthEvent> GenerateRadomEvents(int count);
        static vector<HealthEvaluation> GenerateRadomEventUnhealthyEvaluation(int count);
    };

    class TestEstimator
        : public Serialization::FabricSerializable
        , public Common::ISizeEstimator
    {
    public:
        TestEstimator()
            : StringItem()
            , Vector()
            , Node()
        {
        }

        wstring StringItem;
        vector<wstring> Vector;
        NodeId Node;

        FABRIC_FIELDS_03(StringItem, Vector, Node)

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(StringItem)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(Vector)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(Node)
        END_DYNAMIC_SIZE_ESTIMATION()
    };

    INITIALIZE_SIZE_ESTIMATION(TestEstimator)

    BOOST_FIXTURE_TEST_SUITE(SizeEstimatorTestSuite,SizeEstimatorTest)

    BOOST_AUTO_TEST_CASE(TestEstimatorTest)
    {
        TestEstimator test;
        CheckSize(test, L"TestEstimator-Default");

        test.Node = NodeId::MaxNodeId;
        CheckSize(test, L"TestEstimator-MaxNodeId");

        test.Node = NodeId(LargeInteger(324325325, 4309240));
        CheckSize(test, L"TestEstimator-NodeId");

        test.StringItem = L"MyString";
        CheckSize(test, L"TestEstimator-WString");

        for (int i = 0; i < 1000; i++)
        {
            test.Vector.push_back(wformatString("Item-{0}", i));
        }

        CheckSize(test, L"TestEstimator-WithVectorItems");
    }

    BOOST_AUTO_TEST_CASE(UriTest)
    {
        NamingUri name(L"fabric:/MyApp/MyService/With/Long/Path/With/Many/Segments/To/Avoid/Issues/Due/To/Allowed/Overhead/Being/Too/Small");
        CheckSize(name, L"NamingUri");
    }

    BOOST_AUTO_TEST_CASE(PartitionKeyTest)
    {
        PartitionKey partitionKey2(L"partitionkeyinputstring");
        CheckSize(partitionKey2, L"PartitionKey-String");
    }

    BOOST_AUTO_TEST_CASE(PartitionInfoTest)
    {
        PartitionInfo pi;
        CheckSize(pi, L"PartitionInfo-Default");

        PartitionInfo pi3(L"partitionName-that-defines-the-name");
        CheckSize(pi3, L"PartitionInfo-string");
    }

    BOOST_AUTO_TEST_CASE(AddressDetectionFailureTest)
    {
        AddressDetectionFailure adf(
            NamingUri(L"fabric:/test/name/AddressDetectionFailureTest"),
            PartitionKey(L"KeyfortheAddressDetectionFailureTest"),
            ErrorCodeValue::AccessDenied,
            432542);
        CheckSize(adf, L"AddressDetectionFailure");
    }

    BOOST_AUTO_TEST_CASE(ServiceLocationNotificationRequestDataTest)
    {
        AddressDetectionFailureSPtr adf = make_shared<AddressDetectionFailure>(
            NamingUri(L"fabric:/test/name/ServiceLocationNotificationRequestDataTest"),
            PartitionKey(L"KeyforServiceLocationNotificationRequestDataTest"),
            ErrorCodeValue::AccessDenied,
            432542);

        map<Reliability::ConsistencyUnitId, ServiceLocationVersion> previousResolves;
        previousResolves[ConsistencyUnitId(Guid::NewGuid())] = ServiceLocationVersion(244, GenerationNumber(43535, NodeId(LargeInteger(23434, 5435))), 4325);
        previousResolves[ConsistencyUnitId(Guid::NewGuid())] = ServiceLocationVersion(242, GenerationNumber(43535, NodeId(LargeInteger(23434, 5435))), 4325);
        previousResolves[ConsistencyUnitId(Guid::NewGuid())] = ServiceLocationVersion(245, GenerationNumber(43535, NodeId(LargeInteger(23434, 5435))), 4325);
        ServiceLocationNotificationRequestData reqData(
            NamingUri(L"fabric:/Test/ServiceLocationNotificationRequestDataTest"),
            previousResolves,
            nullptr,
            PartitionKey(L"testServiceLocationNotificationRequestDataTest"));
        CheckSize(reqData, L"ServiceLocationNotificationRequestData");
    }

    BOOST_AUTO_TEST_CASE(ServiceReplicaSetTest)
    {
        ServiceReplicaSet srs1;
        CheckSize(srs1, L"ServiceReplicaSet-Default");

        vector<wstring> secondaries;
        for (int i = 0; i < 10; i++)
        {
            secondaries.push_back(wformatString("Secondary{0}", i));
        }

        wstring primary(L"Primary");
        ServiceReplicaSet srs2(
            true,
            true,
            move(primary),
            move(secondaries),
            43242);
        CheckSize(srs2, L"ServiceReplicaSet");
    }

    BOOST_AUTO_TEST_CASE(ConsistencyUnitIdTest)
    {
        ConsistencyUnitId cuid1;
        CheckSize(cuid1, L"ConsistencyUnitId-Default");

        ConsistencyUnitId cuid3(Guid::NewGuid());
        CheckSize(cuid3, L"ConsistencyUnitId-guid");
    }

    BOOST_AUTO_TEST_CASE(ServiceTableEntryTest)
    {
        ServiceTableEntry ste;
        CheckSize(ste, L"ServiceTableEntry-Default");

        ConsistencyUnitId cuid(Guid::NewGuid());

        wstring primary(L"Primary");
        vector<wstring> secondaries;
        for (int i = 0; i < 10; i++)
        {
            secondaries.push_back(wformatString("Secondary{0}", i));
        }
        ServiceReplicaSet srs(
            true,
            true,
            move(primary),
            move(secondaries),
            43242);
        ServiceTableEntry ste2(
            cuid,
            L"ServiceName",
            move(srs));
        CheckSize(ste2, L"ServiceTableEntry");
    }

    BOOST_AUTO_TEST_CASE(ResolvedServicePartitionTest)
    {
        ResolvedServicePartition rsp1;
        CheckSize(rsp1, L"ResolvedServicePartition-Default");

        ConsistencyUnitId cuid(Guid::NewGuid());
        wstring primary(L"Primary");
        vector<wstring> secondaries;
        for (int i = 0; i < 10; i++)
        {
            secondaries.push_back(wformatString("Secondary{0}", i));
        }
        ServiceReplicaSet srs(
            true,
            true,
            move(primary),
            move(secondaries),
            43242);
        ServiceTableEntry ste(
            cuid,
            L"ServiceName",
            move(srs));

        ResolvedServicePartition rsp2(
            true,
            ste,
            PartitionInfo(L"key"),
            GenerationNumber(4244, NodeId(LargeInteger(23, 34))),
            4325325,
            nullptr);
        CheckSize(rsp2, L"ResolvedServicePartition");
    }

    BOOST_AUTO_TEST_CASE(NamePropertyTest)
    {
        NamePropertyMetadata metadata(
            L"PropertyNameForNamePropertyTest",
            FABRIC_PROPERTY_TYPE_WSTRING,
            4325,
            L"CustomTypeId");
        auto metadataCopy = metadata;
        NamePropertyMetadataResult metadataResult(
            NamingUri(L"fabric:NamingUri/For/Metadata/Result/NamePropertyTest"),
            move(metadata),
            DateTime::MaxValue,
            32432,
            34u);
        CheckSize(metadataResult, L"NamePropertyMetadataResult");

        NamePropertyStoreData npsd1;
        CheckSize(npsd1, L"NamePropertyStoreData-Default");

        vector<byte> bytes(234, 100);
        NamePropertyStoreData npsd2(move(metadataCopy), move(bytes));
        CheckSize(npsd2, L"NamePropertyStoreData");

        vector<byte> bytes2(372, 132);
        NamePropertyResult npr(move(metadataResult), move(bytes2));
        CheckSize(npr, L"NamePropertyResult");
    }

    BOOST_AUTO_TEST_CASE(EnumeratePropertiesResultTest)
    {
        EnumeratePropertiesToken token1;
        CheckSize(token1, L"EnumeratePropertiesToken-Default");

        EnumeratePropertiesToken token2(L"LastEnumerated", 4324);
        CheckSize(token2, L"EnumeratePropertiesToken");

        EnumeratePropertiesResult result1;
        CheckSize(result1, L"EnumeratePropertiesResult-Default");

        vector<NamePropertyResult> properties;
        for (int i = 0; i < 10; ++i)
        {
            vector<byte> bytes(i, 132);
            NamePropertyMetadataResult mr(
                NamingUri(L"fabric:NamingUri/For/Metadata/Result/EnumeratePropertiesResultTest"),
                NamePropertyMetadata(
                    L"PropertyNameForEnumeratePropertiesResultTest",
                    FABRIC_PROPERTY_TYPE_WSTRING,
                    4325,
                    L"CustomTypeId"),
                DateTime::MaxValue,
                32432,
                34u);
            NamePropertyResult npr(move(mr), move(bytes));
            properties.push_back(move(npr));
        }

        EnumeratePropertiesResult result2(
            FABRIC_ENUMERATION_BEST_EFFORT_FINISHED,
            move(properties),
            token2);
        CheckSize(result2, L"EnumeratePropertiesResult");
    }

    BOOST_AUTO_TEST_CASE(AttributeListTest)
    {
        AttributeList attribs;
        CheckSize(attribs, L"AttributeList-Default");

        for (int i = 0; i < 1000; i++)
        {
            attribs.AddAttribute(wformatString("Key-{0}", i), wformatString("Value-{0}", i));
            if (i % 10 == 1)
            {
                CheckSize(attribs, wformatString("AttributeList-{0}", i + 1));
            }
        }
    }

    BOOST_AUTO_TEST_CASE(AggregatedHealthStateTest)
    {
        int i = 10;
        PartitionAggregatedHealthState partition(Guid::NewGuid(), FABRIC_HEALTH_STATE_ERROR);
        CheckSize(partition, L"PartitionAggregatedHealthState");

        ReplicaAggregatedHealthState replica(FABRIC_SERVICE_KIND_STATEFUL, Guid::NewGuid(), i, FABRIC_HEALTH_STATE_ERROR);
        CheckSize(replica, L"ReplicaAggregatedHealthState");

        NodeAggregatedHealthState node(
            wformatString("NodeAggregatedHealthStateTest{0}", i),
            NodeId(LargeInteger(10, 10)),
            FABRIC_HEALTH_STATE_WARNING);
        CheckSize(node, L"NodeAggregatedHealthState");

        ServiceAggregatedHealthState service(
            wformatString("fabric:/SizeEstimatorTest/ApplicationAggregatedHealthStateTest/ServiceAggregatedHealthStateTest{0}", i),
            FABRIC_HEALTH_STATE_ERROR);
        CheckSize(service, L"ServiceAggregatedHealthState");

        DeployedServicePackageAggregatedHealthState dSP(
            wformatString("fabric:/SizeEstimatorTest/ApplicationAggregatedHealthStateTest{0}", i),
            L"ServiceManifestName",
            L"0A69A3FE-F75D-498C-9BFE-C74AA286BEAF", // ServicePackageActivationId
            wformatString("NodeAggregatedHealthStateTest{0}", i),
            FABRIC_HEALTH_STATE_OK);
        CheckSize(dSP, L"DeployedServicePackageAggregatedHealthState");

        DeployedApplicationAggregatedHealthState dApp(
            wformatString("fabric:/SizeEstimatorTest/ApplicationAggregatedHealthStateTest{0}", i),
            wformatString("NodeAggregatedHealthStateTest{0}", i),
            FABRIC_HEALTH_STATE_OK);
        CheckSize(dApp, L"DeployedApplicationAggregatedHealthState");

        ApplicationAggregatedHealthState app(
            wformatString("fabric:/SizeEstimatorTest/ApplicationAggregatedHealthStateTest{0}", i),
            FABRIC_HEALTH_STATE_ERROR);
        CheckSize(app, L"ApplicationAggregatedHealthState");
    }

    BOOST_AUTO_TEST_CASE(HealthEvaluationTest)
    {
        wstring serviceName(L"fabric:/HealthEvaluationTestApp/Service");
        wstring appName(L"fabric:/HealthEvaluationTestApp");
        wstring nodeName1(L"HealthEvaluationTestNode1");

        // HealthEvent
        HealthEvent healthEvent(
            L"System.HealthEventTest",
            L"HealthEventTest",
            TimeSpan::FromSeconds(52),
            FABRIC_HEALTH_STATE_ERROR,
            L"Description for HealthEventTest",
            12,
            DateTime::Now(),
            DateTime::Now(),
            false,
            false,
            DateTime::Now(),
            DateTime::Now(),
            DateTime::Now());
        CheckSize(healthEvent, L"HealthEvent");

        HealthEvent healthEvent2(
            L"System.HealthEventTest",
            L"HealthEventTest",
            TimeSpan::FromSeconds(52),
            FABRIC_HEALTH_STATE_ERROR,
            L"Description for HealthEventTest2 with more details",
            235436,
            DateTime::Now(),
            DateTime::Now(),
            false,
            true,
            DateTime::Now(),
            DateTime::Now(),
            DateTime::Now());
        CheckSize(healthEvent2, L"HealthEvent2");

        // Event evaluation
        EventHealthEvaluation eventEval(FABRIC_HEALTH_STATE_WARNING, healthEvent, true);
        CheckSize(eventEval, L"EventHealthEvaluation");

        HealthEvaluation eval1(make_shared<EventHealthEvaluation>(move(eventEval)));
        CheckSize(eval1, L"HealthEvaluation-Event1");

        HealthEvaluation eval2(make_shared<EventHealthEvaluation>(FABRIC_HEALTH_STATE_WARNING, healthEvent2, false));
        CheckSize(eval2, L"HealthEvaluation-Event2");

        vector<HealthEvaluation> unhealthyEvals;

        // Node evaluation
        unhealthyEvals.push_back(eval1);
        unhealthyEvals.push_back(eval2);
        NodeHealthEvaluation nodeHEval1(L"HealthEvaluationTestNode1", FABRIC_HEALTH_STATE_ERROR, move(unhealthyEvals));
        CheckSize(nodeHEval1, L"NodeHealthEvaluation1");
        HealthEvaluation nodeEval1(make_shared<NodeHealthEvaluation>(move(nodeHEval1)));
        CheckSize(nodeEval1, L"HealthEvaluation-Node1");

        unhealthyEvals.push_back(eval1);
        HealthEvaluation nodeEval2(make_shared<NodeHealthEvaluation>(L"HealthEvaluationTestNode2", FABRIC_HEALTH_STATE_WARNING, move(unhealthyEvals)));
        CheckSize(nodeEval2, L"HealthEvaluation-Node2");

        unhealthyEvals.push_back(eval1);
        HealthEvaluation nodeEval3(make_shared<NodeHealthEvaluation>(L"HealthEvaluationTestNode3", FABRIC_HEALTH_STATE_OK, move(unhealthyEvals)));
        CheckSize(nodeEval3, L"HealthEvaluation-Node3");

        // Nodes evaluation
        vector<HealthEvaluation> unhealthyNodes;
        unhealthyNodes.push_back(nodeEval1);
        unhealthyNodes.push_back(nodeEval2);
        unhealthyNodes.push_back(nodeEval3);
        unhealthyNodes.push_back(nodeEval2);
        unhealthyNodes.push_back(nodeEval3);
        NodesHealthEvaluation nodesEval(FABRIC_HEALTH_STATE_ERROR, move(unhealthyNodes), 30, 10);
        CheckSize(nodesEval, L"NodesHealthEvaluation");

        // Replica evaluation
        unhealthyEvals.push_back(eval1);
        ReplicaHealthEvaluation replicaEval1(Guid::NewGuid(), 10, FABRIC_HEALTH_STATE_ERROR, move(unhealthyEvals));
        CheckSize(replicaEval1, L"ReplicaHealthEvaluation1");

        // Replicas evaluation
        unhealthyEvals.push_back(HealthEvaluation(make_shared<ReplicaHealthEvaluation>(move(replicaEval1))));
        ReplicasHealthEvaluation replicasEval(FABRIC_HEALTH_STATE_ERROR, move(unhealthyEvals), 33, 4);
        CheckSize(replicasEval, L"ReplicasHealthEvaluation");

        // Partition evaluation
        unhealthyEvals.push_back(eval1);
        unhealthyEvals.push_back(HealthEvaluation(make_shared<ReplicasHealthEvaluation>(move(replicasEval))));
        PartitionHealthEvaluation partitionEval1(Guid::NewGuid(), FABRIC_HEALTH_STATE_WARNING, move(unhealthyEvals));
        CheckSize(partitionEval1, L"PartitionHealthEvaluation1");

        // Partitions evaluation
        vector<HealthEvaluation> unhealthyPartitions;
        unhealthyPartitions.push_back(HealthEvaluation(make_shared<PartitionHealthEvaluation>(move(partitionEval1))));
        unhealthyPartitions.push_back(HealthEvaluation(make_shared<PartitionHealthEvaluation>(move(partitionEval1))));
        PartitionsHealthEvaluation partitionsEval(FABRIC_HEALTH_STATE_ERROR, move(unhealthyPartitions), 23, 7);
        CheckSize(partitionsEval, L"PartitionsHealthEvaluation");

        // Service evaluation
        unhealthyEvals.push_back(eval1);
        unhealthyEvals.push_back(HealthEvaluation(make_shared<PartitionsHealthEvaluation>(move(partitionsEval))));
        ServiceHealthEvaluation serviceEval1(serviceName, FABRIC_HEALTH_STATE_WARNING, move(unhealthyEvals));
        CheckSize(serviceEval1, L"ServiceHealthEvaluation1");

        // Services evaluation
        vector<HealthEvaluation> unhealthyServices;
        unhealthyServices.push_back(HealthEvaluation(make_shared<ServiceHealthEvaluation>(move(serviceEval1))));
        unhealthyServices.push_back(HealthEvaluation(make_shared<ServiceHealthEvaluation>(move(serviceEval1))));
        unhealthyServices.push_back(HealthEvaluation(make_shared<ServiceHealthEvaluation>(move(serviceEval1))));
        ServicesHealthEvaluation servicesEval(FABRIC_HEALTH_STATE_WARNING, L"ServiceTypeName", move(unhealthyServices), 53, 7);
        CheckSize(servicesEval, L"ServicesHealthEvaluation");

        // Deployed service package evaluation
        unhealthyEvals.push_back(eval1);
        DeployedServicePackageHealthEvaluation dspEval1(
            appName,
            L"serviceManifestName",
            L"0A69A3FE-F75D-498C-9BFE-C74AA286BEAF", // ServicePackageActivationId
            nodeName1,
            FABRIC_HEALTH_STATE_ERROR,
            move(unhealthyEvals));

        CheckSize(dspEval1, L"DSPEvaluation1");

        // DeployedServicePackages evaluation
        vector<HealthEvaluation> unhealthyDeployedServicePackages;
        unhealthyDeployedServicePackages.push_back(HealthEvaluation(make_shared<DeployedServicePackageHealthEvaluation>(move(dspEval1))));
        DeployedServicePackagesHealthEvaluation deployedServicePackagesEval(FABRIC_HEALTH_STATE_ERROR, move(unhealthyDeployedServicePackages), 100);
        CheckSize(deployedServicePackagesEval, L"DeployedServicePackagesHealthEvaluation");

        // DeployedApplication evaluation
        unhealthyEvals.push_back(eval1);
        unhealthyEvals.push_back(HealthEvaluation(make_shared<DeployedServicePackagesHealthEvaluation>(move(deployedServicePackagesEval))));
        DeployedApplicationHealthEvaluation deployedApplicationEval1(appName, nodeName1, FABRIC_HEALTH_STATE_WARNING, move(unhealthyEvals));
        CheckSize(deployedApplicationEval1, L"DeployedApplicationHealthEvaluation1");

        // DeployedApplications evaluation
        vector<HealthEvaluation> unhealthyDeployedApplications;
        unhealthyDeployedApplications.push_back(HealthEvaluation(make_shared<DeployedApplicationHealthEvaluation>(move(deployedApplicationEval1))));
        DeployedApplicationsHealthEvaluation deployedApplicationsEval(FABRIC_HEALTH_STATE_ERROR, move(unhealthyDeployedApplications), 100, 5);
        CheckSize(deployedApplicationsEval, L"DeployedApplicationsHealthEvaluation");

        // Application evaluation
        unhealthyEvals.push_back(eval1);
        unhealthyEvals.push_back(HealthEvaluation(make_shared<DeployedApplicationsHealthEvaluation>(move(deployedApplicationsEval))));
        ApplicationHealthEvaluation applicationEval1(appName, FABRIC_HEALTH_STATE_WARNING, move(unhealthyEvals));
        CheckSize(applicationEval1, L"ApplicationHealthEvaluation1");

        HealthEvaluationBaseSPtr appEval = make_shared<ApplicationHealthEvaluation>(move(applicationEval1));

        // Applications evaluation
        vector<HealthEvaluation> unhealthyApplications;
        unhealthyApplications.push_back(HealthEvaluation(appEval));
        unhealthyApplications.push_back(HealthEvaluation(appEval));
        ApplicationsHealthEvaluation applicationsEval(FABRIC_HEALTH_STATE_OK, move(unhealthyApplications), 80, 13);
        CheckSize(applicationsEval, L"ApplicationsHealthEvaluation");

        // ApplicationTypeApplications evaluation
        vector<HealthEvaluation> unhealthyApplicationTypeApplications;
        unhealthyApplicationTypeApplications.push_back(HealthEvaluation(appEval));
        unhealthyApplicationTypeApplications.push_back(HealthEvaluation(appEval));
        ApplicationTypeApplicationsHealthEvaluation appTypeApplicationsEval(L"AppTypeName", FABRIC_HEALTH_STATE_OK, move(unhealthyApplicationTypeApplications), 100, 23);
        CheckSize(appTypeApplicationsEval, L"ApplicationTypeApplicationsHealthEvaluation");

        // SystemApplication evaluation
        unhealthyEvals.push_back(eval1);
        SystemApplicationHealthEvaluation systemAppEval1(FABRIC_HEALTH_STATE_WARNING, move(unhealthyEvals));
        CheckSize(systemAppEval1, L"SystemApplicationHealthEvaluation1");
    }

    BOOST_AUTO_TEST_CASE(EntityHealthTest)
    {
        wstring serviceName(L"fabric:/HealthEvaluationTestApp/Service");
        wstring appName(L"fabric:/HealthEvaluationTestApp");
        wstring nodeName1(L"HealthEvaluationTestNode1");

        vector<NodeAggregatedHealthState> nodes;
        vector<ServiceAggregatedHealthState> services;
        vector<DeployedServicePackageAggregatedHealthState> dsps;
        vector<DeployedApplicationAggregatedHealthState> dApps;
        vector<ApplicationAggregatedHealthState> apps;
        vector<ReplicaAggregatedHealthState> replicas;
        vector<PartitionAggregatedHealthState> partitions;
        int childrenCount = 5;
        for (int i = 0; i < childrenCount; i++)
        {
            PartitionAggregatedHealthState partition(Guid::NewGuid(), FABRIC_HEALTH_STATE_ERROR);
            CheckSize(partition, L"PartitionAggregatedHealthState");
            partitions.push_back(move(partition));

            ReplicaAggregatedHealthState replica(FABRIC_SERVICE_KIND_STATEFUL, Guid::NewGuid(), i, FABRIC_HEALTH_STATE_ERROR);
            CheckSize(replica, L"ReplicaAggregatedHealthState");
            replicas.push_back(move(replica));

            NodeAggregatedHealthState node(wformatString("NodeAggregatedHealthStateTest{0}", i), NodeId(LargeInteger(i, i)), FABRIC_HEALTH_STATE_ERROR);
            CheckSize(node, L"NodeAggregatedHealthState");
            nodes.push_back(move(node));

            ServiceAggregatedHealthState service(wformatString("fabric:/SizeEstimatorTest/ApplicationAggregatedHealthStateTest/ServiceAggregatedHealthStateTest{0}", i), FABRIC_HEALTH_STATE_ERROR);
            CheckSize(service, L"ServiceAggregatedHealthState");
            services.push_back(move(service));

            DeployedServicePackageAggregatedHealthState dSP(
                wformatString("fabric:/SizeEstimatorTest/ApplicationAggregatedHealthStateTest{0}", i),
                L"ServiceManifestName",
                L"0A69A3FE-F75D-498C-9BFE-C74AA286BEAF", // ServicePackageActivationId
                wformatString("NodeAggregatedHealthStateTest{0}", i),
                FABRIC_HEALTH_STATE_OK);
            dsps.push_back(move(dSP));

            DeployedApplicationAggregatedHealthState dApp(
                wformatString("fabric:/SizeEstimatorTest/ApplicationAggregatedHealthStateTest{0}", i),
                wformatString("NodeAggregatedHealthStateTest{0}", i),
                FABRIC_HEALTH_STATE_OK);
            dApps.push_back(move(dApp));

            ApplicationAggregatedHealthState app(
                wformatString("fabric:/SizeEstimatorTest/ApplicationAggregatedHealthStateTest{0}", i),
                FABRIC_HEALTH_STATE_ERROR);
            apps.push_back(move(app));
        }

        HealthStatistics healthStats;
        healthStats.Add(EntityKind::Replica, HealthStateCount(10, 12, 13));
        healthStats.Add(EntityKind::Partition, HealthStateCount(10, 12, 13));
        healthStats.Add(EntityKind::Service, HealthStateCount(10, 12, 13));
        healthStats.Add(EntityKind::Application, HealthStateCount(1, 1, 1));

        NodeHealth nodeHealth(nodeName1, GenerateRadomEvents(4), FABRIC_HEALTH_STATE_ERROR, GenerateRadomEventUnhealthyEvaluation(2));
        CheckSize(nodeHealth, L"NodeHealth");

        ReplicaHealth replicaHealth(FABRIC_SERVICE_KIND_STATEFUL, Guid::NewGuid(), 3482395, GenerateRadomEvents(2), FABRIC_HEALTH_STATE_OK, GenerateRadomEventUnhealthyEvaluation(1));
        CheckSize(replicaHealth, L"ReplicaHealth");

        PartitionHealth partitionHealth(
            Guid::NewGuid(),
            GenerateRadomEvents(2),
            FABRIC_HEALTH_STATE_OK,
            GenerateRadomEventUnhealthyEvaluation(5),
            move(replicas),
            make_unique<HealthStatistics>(healthStats));
        CheckSize(partitionHealth, L"PartitionHealth");
        
        ServiceHealth serviceHealth(
            serviceName,
            GenerateRadomEvents(2),
            FABRIC_HEALTH_STATE_OK,
            GenerateRadomEventUnhealthyEvaluation(5),
            move(partitions),
            make_unique<HealthStatistics>(healthStats));

        CheckSize(serviceHealth, L"ServiceHealth");

        DeployedServicePackageHealth dspHealth(
            appName, 
            L"ServiceManifest",
            L"0A69A3FE-F75D-498C-9BFE-C74AA286BEAF", // ServicePackageActivationId
            nodeName1, 
            GenerateRadomEvents(4), 
            FABRIC_HEALTH_STATE_ERROR, 
            GenerateRadomEventUnhealthyEvaluation(2));
        
        CheckSize(dspHealth, L"DeployedServicePackageHealth");

        DeployedApplicationHealth deployedApplicationHealth(
            appName,
            nodeName1,
            GenerateRadomEvents(2),
            FABRIC_HEALTH_STATE_OK,
            GenerateRadomEventUnhealthyEvaluation(5),
            move(dsps),
            make_unique<HealthStatistics>(healthStats));
        CheckSize(deployedApplicationHealth, L"DeployedApplicationHealth");

        ApplicationHealth applicationHealth(
            appName,
            GenerateRadomEvents(2),
            FABRIC_HEALTH_STATE_OK,
            GenerateRadomEventUnhealthyEvaluation(5),
            move(services),
            move(dApps),
            make_unique<HealthStatistics>(healthStats));
        CheckSize(applicationHealth, L"ApplicationHealth");
    }

    BOOST_AUTO_TEST_CASE(EntityHealthInformationTest)
    {
        wstring nodeName(L"EntityHealthInformationTestNode");
        wstring serviceName(L"fabric:/EntityHealthInformationTestApp/Service");
        wstring appName(L"fabric:/EntityHealthInformationTestApp");

        ApplicationEntityHealthInformation appEntity(appName, 143535);
        CheckSize(appEntity, L"AppEntity");

        ClusterEntityHealthInformation clusterEntity;
        CheckSize(clusterEntity, L"ClusterEntity");

        DeployedApplicationEntityHealthInformation daEntity(appName, LargeInteger(104832, 10329183), nodeName, 432);
        CheckSize(daEntity, L"DeployedAppEntity");

        DeployedServicePackageEntityHealthInformation dspEntity(appName, L"ServiceManifest", L"0A69A3FE-F75D-498C-9BFE-C74AA286BEAF", LargeInteger(0, 0), nodeName, 4);
        CheckSize(dspEntity, L"DeployedServicePackageEntity");

        NodeEntityHealthInformation nodeEntity(LargeInteger(22384290890, 10329183), nodeName, 1);
        CheckSize(nodeEntity, L"NodeEntity");

        PartitionEntityHealthInformation partitionEntity(Guid::NewGuid());
        CheckSize(partitionEntity, L"PartitionEntity");

        StatefulReplicaEntityHealthInformation replicaEntity(Guid::NewGuid(), 1, 2);
        CheckSize(replicaEntity, L"ReplicaEntity");

        StatelessInstanceEntityHealthInformation instanceEntity(Guid::NewGuid(), 1);
        CheckSize(instanceEntity, L"InstanceEntity");
    }

    BOOST_AUTO_TEST_CASE(HealthReportTest)
    {
        wstring nodeName(L"HealthReportTestNode");
        AttributeList attribs;
        for (int i = 0; i < 5; i++)
        {
            attribs.AddAttribute(wformatString("Key-{0}", i), wformatString("Value-{0}", i));
        }

        HealthReport nodeReport = HealthReport::CreateSystemHealthReport(
            SystemHealthReportCode::FM_NodeDownDuringUpgrade,
            make_shared<NodeEntityHealthInformation>(LargeInteger(22384290890, 10329183), nodeName, 1),
            L"dynamic property",
            move(attribs));
        CheckSize(nodeReport, L"NodeHealthReport");
    }

    BOOST_AUTO_TEST_CASE(NodeQueryResultTest)
    {
        NodeDeactivationTaskId taskId1(L"taskId1", FABRIC_NODE_DEACTIVATION_TASK_TYPE_REPAIR);
        CheckSize(taskId1, L"NodeDeactivationTaskId");

        NodeDeactivationTask t1(taskId1, FABRIC_NODE_DEACTIVATION_INTENT_REMOVE_DATA);
        CheckSize(t1, L"NodeDeactivationTask");

        std::vector<NodeDeactivationTask> tasks;
        tasks.push_back(t1);
        tasks.push_back(t1);

        NodeDeactivationQueryResult nodeDeactivationInfo(
            FABRIC_NODE_DEACTIVATION_INTENT_PAUSE,
            FABRIC_NODE_DEACTIVATION_STATUS_COMPLETED,
            move(tasks),
            vector<SafetyCheckWrapper>());
        CheckSize(nodeDeactivationInfo, L"NodeDeactivationQueryResult");

        NodeQueryResult queryResult(
            L"HealthReportTestNode",
            L"127.0.0.1:3435",
            L"NodeType",
            L"1.0",
            L"2.3432532",
            FABRIC_QUERY_NODE_STATUS_UP,
            432758275,
            0,
            DateTime::Now(),
            DateTime::Zero,
            false,
            L"ud1",
            L"fd1",
            NodeId(LargeInteger(3431242, 43924392)),
            3435353,
            move(nodeDeactivationInfo),
            432);
        CheckSize(queryResult, L"NodeQueryResult");
    }

    BOOST_AUTO_TEST_CASE(ApplicationQueryResultTest)
    {
        map<wstring, wstring> params;
        for (int i = 0; i < 10; ++i)
        {
            params[wformatString("Key{0}", i)] = wformatString("Value{0}", i);
        }

        ApplicationQueryResult queryResult(
            NamingUri(L"fabric:/MyApp"),
            L"AppTypeName",
            L"AppVersion",
            ApplicationStatus::Ready,
            params,
            FABRIC_APPLICATION_DEFINITION_KIND_SERVICE_FABRIC_APPLICATION_DESCRIPTION,
            L"UpgradeTypeVersion",
            params);
        CheckSize(queryResult, L"ApplicationQueryResult");
    }

    BOOST_AUTO_TEST_CASE(ServiceQueryResultTest)
    {
        ServiceQueryResult queryResult = ServiceQueryResult::CreateStatefulServiceQueryResult(
            NamingUri(L"fabric:/MyApp/MyService"),
            L"ServiceTypeName",
            L"serviceManifestVersion",
            true,
            FABRIC_QUERY_SERVICE_STATUS_UPGRADING);
        CheckSize(queryResult, L"ServiceQueryResult");
    }

    BOOST_AUTO_TEST_CASE(ServicePartitionQueryResultTest)
    {
        ServicePartitionInformation partitionInfo(
            Guid::NewGuid(),
            L"PartitionNameForServicePartitionQueryResultTest");
        ServicePartitionQueryResult queryResult = ServicePartitionQueryResult::CreateStatefulServicePartitionQueryResult(
            partitionInfo,
            5,
            4,
            FABRIC_QUERY_SERVICE_PARTITION_STATUS_NOT_READY,
            45,
            Reliability::Epoch(4324, 43242));
        CheckSize(queryResult, L"ServicePartitionQueryResult");
    }

    BOOST_AUTO_TEST_CASE(ServiceReplicaQueryResultTest)
    {
        ServicePartitionInformation partitionInfo(
            Guid::NewGuid(),
            L"PartitionNameForServiceReplicaQueryResultTest");
        ServiceReplicaQueryResult queryResult = ServiceReplicaQueryResult::CreateStatefulServiceReplicaQueryResult(
            1,
            FABRIC_REPLICA_ROLE_PRIMARY,
            FABRIC_QUERY_SERVICE_REPLICA_STATUS_READY,
            L"fabric:/replica/address",
            L"NodeName",
            43);
        CheckSize(queryResult, L"ServiceReplicaQueryResult");
    }

    BOOST_AUTO_TEST_CASE(ServicePartitionInformationTest)
    {
        ServicePartitionInformation partitionInfo(
            Guid::NewGuid(),
            L"PartitionName");
        CheckSize(partitionInfo, L"ServicePartitionInformation");
    }

    BOOST_AUTO_TEST_SUITE_END()

    template <class T>
    void SizeEstimatorTest::CheckSize(T const & obj, wstring const & objType)
    {
        // Serialize the object and get the expected size
        size_t expectedSize;
        auto error = FabricSerializer::EstimateSize(obj, expectedSize);
        if (!error.IsSuccess())
        {
            Assert::CodingError("{0}: EstimateSize returned error {1}", objType, error);
        }

        size_t actualSize = obj.EstimateSize();
        if (obj.SerializationOverheadEstimate == 0)
        {
            Assert::CodingError("{0}: SerializationOverheadEstimate is not initialized", objType);
        }

        CheckSize(expectedSize, actualSize, objType);
    }

    void SizeEstimatorTest::CheckSize(size_t expectedSize, size_t actualSize, wstring const & objType)
    {
        if (actualSize != expectedSize)
        {
            int diffInt = static_cast<int>(expectedSize)-static_cast<int>(actualSize);
            //BOOST_WMESSAGE(wformatString("{0}: {1} (expected {2}): {3}", objType.c_str(), actualSize, expectedSize, diffInt).c_str());

            size_t diff = abs(diffInt);

            size_t maxThreshold = expectedSize / 5;
            const int minAllowedDiff = 20;
            if (maxThreshold < minAllowedDiff)
            {
                // Tolerate couple of bytes for smaller objects
                maxThreshold = minAllowedDiff;
            }

            if (diff > maxThreshold)
            {
                VERIFY_FAIL(wformatString("{0} diff: actual {1}, max tolerated {2}", objType.c_str(), diff, maxThreshold).c_str());
            }
        }
    }

    vector<HealthEvent> SizeEstimatorTest::GenerateRadomEvents(int count)
    {
        vector<HealthEvent> events;
        for (int i = 0; i < count; i++)
        {
            HealthEvent healthEvent(
                wformatString("System.HealthEventTest{0}", i),
                L"HealthEventTest",
                TimeSpan::FromSeconds(3 * i + 55),
                FABRIC_HEALTH_STATE_ERROR,
                L"Description for HealthEventTest",
                i,
                DateTime::Now(),
                DateTime::Now(),
                false,
                false,
                DateTime::Now(),
                DateTime::Now(),
                DateTime::Now());
            CheckSize(healthEvent, L"HealthEvent");
            events.push_back(move(healthEvent));
        }

        return events;
    }

    vector<HealthEvaluation> SizeEstimatorTest::GenerateRadomEventUnhealthyEvaluation(int count)
    {
        vector<HealthEvaluation> unhealthyEvals;
        vector<HealthEvent> events = GenerateRadomEvents(count);
        for (int i = 0; i < count; i++)
        {
            HealthEvaluation eval(make_shared<EventHealthEvaluation>(FABRIC_HEALTH_STATE_WARNING, events[i], true));
            CheckSize(eval, L"HealthEvaluation(Event)");
            unhealthyEvals.push_back(move(eval));
        }

        return unhealthyEvals;
    }
}
