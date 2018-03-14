// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "ComTestOperation.h"
#include "TestHealthClient.h"

namespace ReplicationUnitTest
{
    using namespace Common;
    using namespace std;
    using namespace Reliability::ReplicationComponent;


    static Common::StringLiteral const Source("TESTBatchedHealthReporter");

    class TestBatchedHealthReporter
    {
    };

    BOOST_FIXTURE_TEST_SUITE(TestBatchedHealthReporterSuite,TestBatchedHealthReporter)

    BOOST_AUTO_TEST_CASE(TestDisabled)
    {
        ReplicationEndpointId endpoint(Common::Guid::NewGuid(), 0);
        auto healthClient = HealthClient::Create(false);

        ComTestOperation::WriteInfo(
            Source,
            "Start TestDisabled. Partition = {0}", endpoint.PartitionId);
        
        auto sender = BatchedHealthReporter::Create(
            endpoint.PartitionId,
            endpoint,
            HealthReportType::SecondaryReplicationQueueStatus,
            TimeSpan::Zero,
            healthClient);

        // Since timespan is zero, health reporting should be disalbed
        sender->ScheduleWarningReport(L"Temp");
        
        VERIFY_ARE_EQUAL(false, sender->Test_IsTimerRunning, L"Timer running check");
        VERIFY_ARE_EQUAL(::FABRIC_HEALTH_STATE_OK, sender->Test_LatestHealthState, L"Latest health state check");

        sender->Close();

        VERIFY_ARE_EQUAL(0, healthClient->NumberofReportsSent, L"Number of reports check");
    }
 
    BOOST_AUTO_TEST_CASE(TestOk)
    {
        ReplicationEndpointId endpoint(Common::Guid::NewGuid(), 0);
        auto healthClient = HealthClient::Create(true);

        ComTestOperation::WriteInfo(
            Source,
            "Start TestOk. Partition = {0}", endpoint.PartitionId);
        
        auto sender = BatchedHealthReporter::Create(
            endpoint.PartitionId,
            endpoint,
            HealthReportType::SecondaryReplicationQueueStatus,
            TimeSpan::FromMilliseconds(1),
            healthClient);

        sender->ScheduleOKReport();
        VERIFY_ARE_EQUAL(false, sender->Test_IsTimerRunning, L"Timer running check");

        sender->Close();

        VERIFY_ARE_EQUAL(::FABRIC_HEALTH_STATE_OK, sender->Test_LatestHealthState, L"Latest health state check");
        VERIFY_ARE_EQUAL(0, healthClient->NumberofReportsSent, L"Number of reports check");
    }

    BOOST_AUTO_TEST_CASE(TestSimpleWarningReport)
    {
        ReplicationEndpointId endpoint(Common::Guid::NewGuid(), 0);
        auto healthClient = HealthClient::Create(true);

        ComTestOperation::WriteInfo(
            Source,
            "Start TestSimpleWarningReport. Partition = {0}", endpoint.PartitionId);
        
        auto sender = BatchedHealthReporter::Create(
            endpoint.PartitionId,
            endpoint,
            HealthReportType::SecondaryReplicationQueueStatus,
            TimeSpan::FromMilliseconds(50),
            healthClient);

        sender->ScheduleWarningReport(L"Temp");
        VERIFY_ARE_EQUAL(true, sender->Test_IsTimerRunning, L"Timer running check");
        Sleep(1000); // wait for timer to finish running
        VERIFY_ARE_EQUAL(false, sender->Test_IsTimerRunning, L"Timer running check");

        sender->Close();

        VERIFY_ARE_EQUAL(::FABRIC_HEALTH_STATE_OK, sender->Test_LatestHealthState, L"Latest health state check");
        VERIFY_ARE_EQUAL(2, healthClient->NumberofReportsSent, L"Number of reports check");
    }
    
    BOOST_AUTO_TEST_CASE(TestWarningsWithinIntervalReport)
    {
        ReplicationEndpointId endpoint(Common::Guid::NewGuid(), 0);
        auto healthClient = HealthClient::Create(true);

        ComTestOperation::WriteInfo(
            Source,
            "Start TestWarningsWithinIntervalReport. Partition = {0}", endpoint.PartitionId);
        
        auto sender = BatchedHealthReporter::Create(
            endpoint.PartitionId,
            endpoint,
            HealthReportType::SecondaryReplicationQueueStatus,
            TimeSpan::FromMilliseconds(50),
            healthClient);

        sender->ScheduleWarningReport(L"Temp");
        VERIFY_ARE_EQUAL(true, sender->Test_IsTimerRunning, L"Timer running check");
        sender->ScheduleWarningReport(L"Temp4");
        sender->ScheduleWarningReport(L"Temp3");
        sender->ScheduleWarningReport(L"Temp5");
        sender->ScheduleWarningReport(L"Temp2");
        Sleep(1000); // wait for timer to finish running
        VERIFY_ARE_EQUAL(false, sender->Test_IsTimerRunning, L"Timer running check");
        sender->Close();

        VERIFY_ARE_EQUAL(::FABRIC_HEALTH_STATE_OK, sender->Test_LatestHealthState, L"Latest health state check");
        VERIFY_ARE_EQUAL(L"Temp2", sender->Test_LatestDescription, L"Description check");
        VERIFY_ARE_EQUAL(2, healthClient->NumberofReportsSent, L"Number of reports check");
    }
 
    BOOST_AUTO_TEST_CASE(TestWarningsWithinIntervalAndCloseImmediatelyReport)
    {
        ReplicationEndpointId endpoint(Common::Guid::NewGuid(), 0);
        auto healthClient = HealthClient::Create(true);

        ComTestOperation::WriteInfo(
            Source,
            "Start TestWarningsWithinIntervalAndCloseImmediatelyReport. Partition = {0}", endpoint.PartitionId);
        
        auto sender = BatchedHealthReporter::Create(
            endpoint.PartitionId,
            endpoint,
            HealthReportType::SecondaryReplicationQueueStatus,
            TimeSpan::FromMilliseconds(50),
            healthClient);

        sender->ScheduleWarningReport(L"Temp");
        sender->ScheduleWarningReport(L"Temp1");
        sender->ScheduleWarningReport(L"Temp9");
        sender->ScheduleWarningReport(L"Temp2");
        VERIFY_ARE_EQUAL(true, sender->Test_IsTimerRunning, L"Timer running check");
        sender->Close();

        VERIFY_ARE_EQUAL(false, sender->Test_IsTimerRunning, L"Timer running check");
        VERIFY_ARE_EQUAL(::FABRIC_HEALTH_STATE_OK, sender->Test_LatestHealthState, L"Latest health state check");
        VERIFY_ARE_EQUAL(L"Temp2", sender->Test_LatestDescription, L"Description check");
        VERIFY_ARE_EQUAL(0, healthClient->NumberofReportsSent, L"Number of reports check");
    }

    BOOST_AUTO_TEST_CASE(TestWarningsAndOkWithinIntervalReport)
    {
        ReplicationEndpointId endpoint(Common::Guid::NewGuid(), 0);
        auto healthClient = HealthClient::Create(true);

        ComTestOperation::WriteInfo(
            Source,
            "Start TestWarningsAndOkWithinIntervalReport. Partition = {0}", endpoint.PartitionId);
        
        auto sender = BatchedHealthReporter::Create(
            endpoint.PartitionId,
            endpoint,
            HealthReportType::SecondaryReplicationQueueStatus,
            TimeSpan::FromMilliseconds(50),
            healthClient);

        sender->ScheduleWarningReport(L"Temp");
        sender->ScheduleWarningReport(L"Temp3");
        sender->ScheduleWarningReport(L"Temp5");
        sender->ScheduleWarningReport(L"Temp2");
        VERIFY_ARE_EQUAL(true, sender->Test_IsTimerRunning, L"Timer running check");
        sender->ScheduleOKReport();

        VERIFY_ARE_EQUAL(false, sender->Test_IsTimerRunning, L"Timer running check");
        sender->Close();

        VERIFY_ARE_EQUAL(::FABRIC_HEALTH_STATE_OK, sender->Test_LatestHealthState, L"Latest health state check");
        VERIFY_ARE_EQUAL(L"", sender->Test_LatestDescription, L"Description check");
        VERIFY_ARE_EQUAL(0, healthClient->NumberofReportsSent, L"Number of reports check");
    }

    BOOST_AUTO_TEST_CASE(TestWarningAndOkin2IntervalsReport)
    {
        ReplicationEndpointId endpoint(Common::Guid::NewGuid(), 0);
        auto healthClient = HealthClient::Create(true);

        ComTestOperation::WriteInfo(
            Source,
            "Start TestWarningAndOkin2IntervalsReport. Partition = {0}", endpoint.PartitionId);
        
        auto sender = BatchedHealthReporter::Create(
            endpoint.PartitionId,
            endpoint,
            HealthReportType::SecondaryReplicationQueueStatus,
            TimeSpan::FromMilliseconds(50),
            healthClient);

        sender->ScheduleWarningReport(L"Temp");
        Sleep(1000);
        sender->ScheduleOKReport();

        VERIFY_ARE_EQUAL(true, sender->Test_IsTimerRunning, L"Timer running check");
        Sleep(1000);

        VERIFY_ARE_EQUAL(::FABRIC_HEALTH_STATE_OK, sender->Test_LatestHealthState, L"Latest health state check");
        VERIFY_ARE_EQUAL(L"", sender->Test_LatestDescription, L"Description check");
        VERIFY_ARE_EQUAL(2, healthClient->NumberofReportsSent, L"Number of reports check");
    }
    
    BOOST_AUTO_TEST_CASE(TestOksWithinIntervalReport)
    {
        ReplicationEndpointId endpoint(Common::Guid::NewGuid(), 0);
        auto healthClient = HealthClient::Create(true);

        ComTestOperation::WriteInfo(
            Source,
            "Start TestOksWithinIntervalReport. Partition = {0}", endpoint.PartitionId);
        
        auto sender = BatchedHealthReporter::Create(
            endpoint.PartitionId,
            endpoint,
            HealthReportType::SecondaryReplicationQueueStatus,
            TimeSpan::FromMilliseconds(50),
            healthClient);

        sender->ScheduleWarningReport(L"Temp");
        VERIFY_ARE_EQUAL(true, sender->Test_IsTimerRunning, L"Timer running check");
        Sleep(1000);
        VERIFY_ARE_EQUAL(false, sender->Test_IsTimerRunning, L"Timer running check");
        sender->ScheduleOKReport();
        sender->ScheduleOKReport();
        sender->ScheduleOKReport();

        Sleep(1000);
        VERIFY_ARE_EQUAL(false, sender->Test_IsTimerRunning, L"Timer running check");

        VERIFY_ARE_EQUAL(::FABRIC_HEALTH_STATE_OK, sender->Test_LatestHealthState, L"Latest health state check");
        VERIFY_ARE_EQUAL(L"", sender->Test_LatestDescription, L"Description check");
        VERIFY_ARE_EQUAL(2, healthClient->NumberofReportsSent, L"Number of reports check");
    }

    BOOST_AUTO_TEST_SUITE_END()
}
