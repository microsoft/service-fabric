// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "ServiceModel/ServiceModel.h"

using namespace Common;
using namespace ServiceModel;
using namespace Naming;
using namespace std;
using namespace Management::HealthManager;

namespace Common
{    
    class JSonSerializationTest
    {
    };

    BOOST_FIXTURE_TEST_SUITE(JSonSerializationTestSuite,JSonSerializationTest)

 
    BOOST_AUTO_TEST_CASE(PSDTest)
    {
        Reliability::ServiceCorrelationDescription correlation(L"temp", FABRIC_SERVICE_CORRELATION_SCHEME_AFFINITY);
        vector<Reliability::ServiceCorrelationDescription> correlations;

        correlations.push_back(correlation);
        correlations.push_back(correlation);

        Reliability::ServiceLoadMetricDescription slmd(L"three", FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH, 2, 3);
        vector<Reliability::ServiceLoadMetricDescription> slmVector;
        slmVector.push_back(slmd);
        slmVector.push_back(slmd);
        wstring appName = L"appName";
        wstring serviceName = L"serviceName";
        wstring typeName = L"serviceTypeName";
        vector<wstring> partitionNames;
        partitionNames.push_back(L"four");
        partitionNames.push_back(L"five");
        ByteBuffer data;
        data.push_back(0x10);
        data.push_back(0xff);

        auto averageLoadPolicy = make_shared<Reliability::AveragePartitionLoadScalingTrigger>(L"servicefabric:/_MemoryInMB", 0.5, 1.2, 50);
        auto instanceMechanism = make_shared<Reliability::InstanceCountScalingMechanism>(2, 5, 1);

        Reliability::ServiceScalingPolicyDescription scalingPolicy(averageLoadPolicy, instanceMechanism);
        std::vector<Reliability::ServiceScalingPolicyDescription> scalingPolicies;
        scalingPolicies.push_back(scalingPolicy);

        PartitionedServiceDescWrapper psdWrapper(
            FABRIC_SERVICE_KIND_STATEFUL,
            appName,
            serviceName,
            typeName,
            data,
            PartitionSchemeDescription::Named,
            2,
            3,
            4,
            partitionNames,
            5,
            6,
            7,
            true,
            L"constraints",
            correlations,
            slmVector,
            vector<ServiceModel::ServicePlacementPolicyDescription>(),
            static_cast<FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_FLAGS>(FABRIC_STATEFUL_SERVICE_SETTINGS_REPLICA_RESTART_WAIT_DURATION | FABRIC_STATEFUL_SERVICE_SETTINGS_QUORUM_LOSS_WAIT_DURATION | FABRIC_STATEFUL_SERVICE_SETTINGS_STANDBY_REPLICA_KEEP_DURATION),
            8,
            9,
            10,
            FABRIC_MOVE_COST_MEDIUM,
            ServicePackageActivationMode::SharedProcess,
            L"",
            scalingPolicies);

        PartitionedServiceDescriptor psd;
        PartitionedServiceDescriptor psd1;

        wstring serializedString;
        auto error = JsonHelper::Serialize(psdWrapper, serializedString);
        VERIFY_IS_TRUE(error.IsSuccess());

        PartitionedServiceDescWrapper psdWrapper1;
        error = JsonHelper::Deserialize(psdWrapper1, serializedString);
        VERIFY_IS_TRUE(error.IsSuccess());
        psd.FromWrapper(psdWrapper);
        psd1.FromWrapper(psdWrapper1);

        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(psd.ToString(), psd1.ToString()));
        VERIFY_IS_TRUE(psd.Service.ScalingPolicies == psd1.Service.ScalingPolicies);
    }


    BOOST_AUTO_TEST_CASE(NodesTest)
    {
        wstring name = L"Node1";
        wstring ip = L"localhost";
        wstring nodeType = L"TestNode";
        wstring codeVersion = L"1.0";
        wstring configVersion = L"1.0";
        int64 nodeUpTimeInSeconds = 3600;
        int64 nodeDownTimeInSeconds = 0;
        DateTime nodeUpAt = DateTime::Now();
        DateTime nodeDownAt = DateTime::Zero;
        bool isSeed = true;
        wstring upgradeDomain = L"TestUpgradeDomain";
        wstring faultDomain = L"TestFaultDomain";
        Federation::NodeId nodeId;

        NodeQueryResult nodeResult(
            name,
            ip,
            nodeType,
            codeVersion,
            configVersion,
            FABRIC_QUERY_NODE_STATUS_UP,
            nodeUpTimeInSeconds,
            nodeDownTimeInSeconds,
            nodeUpAt,
            nodeDownAt,
            isSeed,
            upgradeDomain,
            faultDomain,
            nodeId,
            0,
            NodeDeactivationQueryResult(),
            0);
        wstring data;

        //
        // Verify successful Serialization.
        //
        auto error = JsonHelper::Serialize(nodeResult, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        NodeQueryResult nodeResult2;
        error = JsonHelper::Deserialize(nodeResult2, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        //
        // Validate the deserialized data.
        //
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(nodeResult2.ToString(), nodeResult.ToString()));
    }

    BOOST_AUTO_TEST_CASE(ApplicationsTest)
    {
        Uri uri(L"fabric:" L"tempName", L"tempPath");
        wstring appTypeName = L"appType1";
        wstring appTypeVersion = L"1.0";

        std::map<std::wstring, std::wstring> applicationParameters;
        ApplicationQueryResult appQueryResult(uri, appTypeName, appTypeVersion, ApplicationStatus::Ready, applicationParameters, FABRIC_APPLICATION_DEFINITION_KIND_SERVICE_FABRIC_APPLICATION_DESCRIPTION, L"", map<wstring, wstring>());
        wstring data;

        //
        // Verify successful Serialization.
        //
        auto error = JsonHelper::Serialize(appQueryResult, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        ApplicationQueryResult appQueryResult2;

        //
        // Verify successful DeSerialization.
        //
        error = JsonHelper::Deserialize(appQueryResult2, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        //
        // Validate the deserialized data.
        //
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(appQueryResult2.ToString(), appQueryResult.ToString()));
    }

    BOOST_AUTO_TEST_CASE(StatefulServicesTest)
    {
        Uri uri(L"fabric:" L"tempName", L"tempPath");

        ServiceQueryResult statefulQueryResult =
            ServiceQueryResult::CreateStatefulServiceQueryResult(uri, L"Type1", L"1.0", true, FABRIC_QUERY_SERVICE_STATUS_ACTIVE);
        wstring data;

        //
        // Verify successful Serialization.
        //
        auto error = JsonHelper::Serialize(statefulQueryResult, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        ServiceQueryResult statefulQueryResult2;

        //
        // Verify successful DeSerialization.
        //
        error = JsonHelper::Deserialize(statefulQueryResult2, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        //
        // Validate the deserialized data.
        //
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(statefulQueryResult2.ToString(), statefulQueryResult.ToString()));
    }

    BOOST_AUTO_TEST_CASE(StatelessServicesTest)
    {
        Uri uri(L"fabric:" L"tempName", L"tempPath");

        ServiceQueryResult statelessQueryResult =
            ServiceQueryResult::CreateStatelessServiceQueryResult(uri, L"Type1", L"1.0", FABRIC_QUERY_SERVICE_STATUS_ACTIVE);
        wstring data;

        //
        // Verify successful Serialization.
        //
        auto error = JsonHelper::Serialize(statelessQueryResult, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        ServiceQueryResult statelessQueryResult2;

        //
        // Verify successful DeSerialization.
        //
        error = JsonHelper::Deserialize(statelessQueryResult2, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        //
        // Validate the deserialized data.
        //
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(statelessQueryResult2.ToString(), statelessQueryResult.ToString()));
    }

    BOOST_AUTO_TEST_CASE(StatelessServicePartitionTest)
    {
        Guid partitionId = Guid::NewGuid();
        ServicePartitionInformation int64Partition = ServicePartitionInformation(partitionId, -1, 1);
        ServicePartitionQueryResult statelessPartQueryResult = ServicePartitionQueryResult::CreateStatelessServicePartitionQueryResult(
            int64Partition, 2, FABRIC_QUERY_SERVICE_PARTITION_STATUS_READY);
        wstring data;

        //
        // Verify successful Serialization.
        //
        auto error = JsonHelper::Serialize(statelessPartQueryResult, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        ServicePartitionQueryResult statelessPartQueryResult2;

        //
        // Verify successful DeSerialization.
        //
        error = JsonHelper::Deserialize(statelessPartQueryResult2, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        //
        // Validate the deserialized data.
        //
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(statelessPartQueryResult2.ToString(), statelessPartQueryResult.ToString()));
    }

    BOOST_AUTO_TEST_CASE(StatefulServicePartitionTest)
    {
        Guid partitionId = Guid::NewGuid();
        Reliability::Epoch primaryEpoch(2, 2);
        ServicePartitionInformation int64Partition = ServicePartitionInformation(partitionId, -1, 1);
        ServicePartitionQueryResult statefulPartQueryResult = ServicePartitionQueryResult::CreateStatefulServicePartitionQueryResult(
            int64Partition, 4, 2, FABRIC_QUERY_SERVICE_PARTITION_STATUS_READY, 0, primaryEpoch);
        wstring data;

        //
        // Verify successful Serialization.
        //
        auto error = JsonHelper::Serialize(statefulPartQueryResult, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        ServicePartitionQueryResult statefulPartQueryResult2;

        //
        // Verify successful DeSerialization.
        //
        error = JsonHelper::Deserialize(statefulPartQueryResult2, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        //
        // Validate the deserialized data.
        //
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(statefulPartQueryResult2.ToString(), statefulPartQueryResult.ToString()));
    }

    BOOST_AUTO_TEST_CASE(StatefulReplicaTest)
    {
        ServiceReplicaQueryResult statefulReplicaResult = ServiceReplicaQueryResult::CreateStatefulServiceReplicaQueryResult(
            -1,
            FABRIC_REPLICA_ROLE::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY,
            FABRIC_QUERY_SERVICE_REPLICA_STATUS::FABRIC_QUERY_SERVICE_REPLICA_STATUS_READY,
            L"Address",
            L"Node",
            0);
        wstring data;

        //
        // Verify successful Serialization.
        //
        auto error = JsonHelper::Serialize(statefulReplicaResult, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        ServiceReplicaQueryResult statefulReplicaResult2;

        //
        // Verify successful DeSerialization.
        //
        error = JsonHelper::Deserialize(statefulReplicaResult2, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        //
        // Validate the deserialized data.
        //
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(statefulReplicaResult2.ToString(), statefulReplicaResult.ToString()));
    }

    BOOST_AUTO_TEST_CASE(StatelessInstanceTest)
    {
        ServiceReplicaQueryResult statelessInstanceResult = ServiceReplicaQueryResult::CreateStatelessServiceInstanceQueryResult(
            -1,
            FABRIC_QUERY_SERVICE_REPLICA_STATUS::FABRIC_QUERY_SERVICE_REPLICA_STATUS_READY,
            L"Address",
            L"Node",
            0);
        wstring data;

        //
        // Verify successful Serialization.
        //
        auto error = JsonHelper::Serialize(statelessInstanceResult, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        ServiceReplicaQueryResult statelessInstanceResult2;

        //
        // Verify successful DeSerialization.
        //
        error = JsonHelper::Deserialize(statelessInstanceResult2, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        //
        // Validate the deserialized data.
        //
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(statelessInstanceResult2.ToString(), statelessInstanceResult.ToString()));
    }

    BOOST_AUTO_TEST_CASE(HealthEventsTest)
    {
        wstring data;
        FILETIME time;
        time.dwHighDateTime = 1234;
        time.dwLowDateTime = 1234;
        DateTime dateTime(time);
        HealthEvent healthEvent(
            L"source1",
            L"property1",
            TimeSpan::MaxValue,
            FABRIC_HEALTH_STATE_INVALID,
            L"no description",
            123,
            dateTime,
            dateTime,
            false,
            false,
            dateTime,
            dateTime,
            dateTime);
        std::vector<HealthEvent> healthEvents;
        healthEvents.push_back(healthEvent);

        wstring appname = L"Name";

        ServiceAggregatedHealthState serviceAggregatedHealthState;
        std::vector<ServiceModel::ServiceAggregatedHealthState> serviceAggHealthStates;
        serviceAggHealthStates.push_back(serviceAggregatedHealthState);

        DeployedApplicationAggregatedHealthState deployedAppHealthState;
        std::vector<DeployedApplicationAggregatedHealthState> deployedAppHealthStates;
        deployedAppHealthStates.push_back(deployedAppHealthState);

        HealthEvent healthEventCopy = healthEvent;
        HealthEvaluation evaluationReason = HealthEvaluation(make_shared<EventHealthEvaluation>(FABRIC_HEALTH_STATE_WARNING, move(healthEventCopy), true));
        std::vector<HealthEvaluation> unhealthyEvaluations;
        unhealthyEvaluations.push_back(evaluationReason);

        auto healthStats = make_unique<HealthStatistics>();
        healthStats->Add(EntityKind::Replica, HealthStateCount(100, 10, 20));
        healthStats->Add(EntityKind::Partition, HealthStateCount(100, 10, 20));
        healthStats->Add(EntityKind::Service, HealthStateCount(100, 10, 20));
        healthStats->Add(EntityKind::DeployedApplication, HealthStateCount(100, 10, 20));
        healthStats->Add(EntityKind::DeployedServicePackage, HealthStateCount(100, 10, 20));

        ApplicationHealth obj1(
            appname,
            move(healthEvents),
            FABRIC_HEALTH_STATE_UNKNOWN,
            move(unhealthyEvaluations),
            move(serviceAggHealthStates),
            move(deployedAppHealthStates),
            move(healthStats));

        auto error = JsonHelper::Serialize(obj1, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        ApplicationHealth obj2;

        error = JsonHelper::Deserialize(obj2, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        wstring string1 = obj1.ToString();
        wstring string2 = obj2.ToString();
        VERIFY_IS_TRUE(Common::StringUtility::AreEqualCaseInsensitive(string1, string2));

        string1 = obj1.Events[0].ToString();
        string2 = obj2.Events[0].ToString();
        VERIFY_IS_TRUE(Common::StringUtility::AreEqualCaseInsensitive(string1, string2));
    }


    BOOST_AUTO_TEST_CASE(LoadInformationTest) 
    {
        LoadMetricInformation loadMetricInformation;
        loadMetricInformation.Name = L"Metric1";
        loadMetricInformation.IsBalancedBefore = false;
        loadMetricInformation.IsBalancedAfter = true;
        loadMetricInformation.DeviationBefore = 0.62;
        loadMetricInformation.DeviationAfter = 0.5;
        loadMetricInformation.BalancingThreshold = 100;
        loadMetricInformation.Action = L"Balancing";
        loadMetricInformation.ActivityThreshold = 1;
        loadMetricInformation.ClusterCapacity = 100;
        loadMetricInformation.ClusterLoad = 50;
        loadMetricInformation.IsClusterCapacityViolation = false;
        loadMetricInformation.RemainingUnbufferedCapacity = 50;
        loadMetricInformation.NodeBufferPercentage = 0.0;
        loadMetricInformation.BufferedCapacity = 100;
        loadMetricInformation.RemainingBufferedCapacity = 0;
        loadMetricInformation.MinNodeLoadValue = 5;
        loadMetricInformation.MinNodeLoadNodeId = Federation::NodeId(Common::LargeInteger(0, 1));
        loadMetricInformation.MaxNodeLoadValue = 30;
        loadMetricInformation.MaxNodeLoadNodeId = Federation::NodeId(Common::LargeInteger(0, 5));
        loadMetricInformation.CurrentClusterLoad = 50.0;
        loadMetricInformation.BufferedClusterCapacityRemaining = 0.0;
        loadMetricInformation.ClusterCapacityRemaining = 50.0;
        loadMetricInformation.MaximumNodeLoad = 29.1;
        loadMetricInformation.MinimumNodeLoad = 4.5;

        wstring data;

        //
        // Verify successful Serialization.
        //
        auto error = JsonHelper::Serialize(loadMetricInformation, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        LoadMetricInformation loadMetricInformationDeserialized;
        error = JsonHelper::Deserialize(loadMetricInformationDeserialized, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        //
        // Validate the deserialized data.
        //
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(loadMetricInformation.ToString(), loadMetricInformationDeserialized.ToString()));
    }

    BOOST_AUTO_TEST_CASE(NodeLoadInformationTest)
    {
        NodeLoadMetricInformation nodeLoadInformation;
        nodeLoadInformation.Name = L"Node1";
        nodeLoadInformation.NodeCapacity = 8;
        nodeLoadInformation.NodeLoad = 6;
        nodeLoadInformation.NodeRemainingCapacity = 2;
        nodeLoadInformation.IsCapacityViolation = false;
        nodeLoadInformation.NodeBufferedCapacity = 0;
        nodeLoadInformation.NodeRemainingBufferedCapacity = 0;
        nodeLoadInformation.CurrentNodeLoad = 6.0;
        nodeLoadInformation.NodeCapacityRemaining = 2.0;
        nodeLoadInformation.BufferedNodeCapacityRemaining = 0.0;

        wstring data;

        //
        // Verify successful Serialization.
        //
        auto error = JsonHelper::Serialize(nodeLoadInformation, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        NodeLoadMetricInformation nodeLoadInformationDeserialized;
        error = JsonHelper::Deserialize(nodeLoadInformationDeserialized, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        //
        // Validate the deserialized data.
        //
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(nodeLoadInformation.ToString(), nodeLoadInformationDeserialized.ToString()));

    }

    BOOST_AUTO_TEST_CASE(ControlCharactersEscapeTest)
    {
        wstring testData = L"\x1b""[40m""\x1b""[1m""\x1b""[33myellow""\x1b""[39m""\x1b""[22m""\x1b""[49m ""\x1b""[2J""\x1b""[H""\x1b""[37;40m ""\x01""\x02""\x03""\x04""\x05""\x06""\x07""\b\f\n\r\v\t""\x0e""\x0f""\x10""\x11""\x12""\x13""\x14""\x15""\x16""\x17""\x18""\x19""\x1a""\x1b""\x1c""\x1d""\x1e""\x1f"" abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890-=[]\\;',./`~!@#$%^&*()_+{}|:\"<>?""\x00";
        ContainerInfoData containerInfoData(move(testData));
        wstring data;

        //
        // Verify successful Serialization.
        //
        auto error = JsonHelper::Serialize(containerInfoData, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        ContainerInfoData containerInfoDataDeserialized;
        error = JsonHelper::Deserialize(containerInfoDataDeserialized, data);
        VERIFY_IS_TRUE(error.IsSuccess());

        //
        // Validate the deserialized data.
        //
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(containerInfoData.Content, containerInfoDataDeserialized.Content));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
