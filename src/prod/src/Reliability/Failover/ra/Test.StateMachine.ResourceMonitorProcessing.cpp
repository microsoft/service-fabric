// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Diagnostics.ResourceUsageReportEventData.h"
#include "Management/ResourceMonitor/config/ResourceMonitorServiceConfig.h"


using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Infrastructure;
using namespace std;

using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateItemHelper;

class Test_ResourceMonitor : public StateMachineTestBase
{

};


BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(Test_ResourceMonitorSuite, Test_ResourceMonitor)

BOOST_AUTO_TEST_CASE(ResourceMonitor_LoadReportAllValid)
{
    auto & test = this->Test;

    Test.UTContext.Clock.SetManualMode();
    test.RestartRA();
    test.UTContext.Clock.AdvanceTime(::ResourceMonitor::ResourceMonitorServiceConfig::GetConfig().ResourceTracingInterval);

    test.AddFT(L"RMC1", L"O None 000/000/411 1:1 CM [N/N/I RD U N F 1:1]");
    test.AddFT(L"RMC2", L"O None 000/000/511 1:1 CM [N/N/I RD U N F 1:1]");

    wstring reportFT1 = wformatString(L"[{0} 100 1.0]",StateManagement::Default::singleton_->RMC1_FTContext.FUID);
    wstring reportFT2 = wformatString(L"[{0} 50 1.5]", StateManagement::Default::singleton_->RMC2_FTContext.FUID);
    wstring report = wformatString(L"{0} {1}", reportFT1, reportFT2);

    test.ProcessRMMessageAndDrain<MessageType::ResourceUsageReport>(report);

    test.ValidateFMMessage<MessageType::ReportLoad>(wformatString(
        L"[{0} {1} false S\\servicefabric:/_MemoryInMB\\100\\{2} S\\servicefabric:/_CpuCores\\1000000\\{3} ] [{4} {5} false S\\servicefabric:/_MemoryInMB\\50\\{6} S\\servicefabric:/_CpuCores\\1500000\\{7} ]",
        StateManagement::Default::singleton_->RMC1_FTContext.FUID,
        StateManagement::Default::singleton_->RMC_STContext.SD.Name,
        test.RA.NodeId,
        test.RA.NodeId,
        StateManagement::Default::singleton_->RMC2_FTContext.FUID,
        StateManagement::Default::singleton_->RMC_STContext.SD.Name,
        test.RA.NodeId,
        test.RA.NodeId));

    auto report1 = test.StateItemHelpers.EventWriterHelper.GetEvent<Diagnostics::ResourceUsageReportEventData>(0);
    auto report2 = test.StateItemHelpers.EventWriterHelper.GetEvent<Diagnostics::ResourceUsageReportEventData>(1);

    Verify::AreEqual(report1->FtId, StateManagement::Default::singleton_->RMC1_FTContext.FUID.Guid);
    Verify::AreEqual(report1->NodeInstance, test.RA.NodeInstance.Id.ToString());
    Verify::AreEqual(report1->ReplicaId, 1);
    Verify::AreEqual(report1->ReplicaRole, ReplicaRole::Enum::Idle);
    Verify::AreEqual(report1->CpuUsage, 1.0);
    Verify::AreEqual(report1->MemoryUsage, 100);
    Verify::AreEqual(report2->FtId, StateManagement::Default::singleton_->RMC2_FTContext.FUID.Guid);
    Verify::AreEqual(report2->CpuUsage, 1.5);
    Verify::AreEqual(report2->MemoryUsage, 50);

}

BOOST_AUTO_TEST_CASE(ResourceMonitor_TracingTimerCheck)
{
    auto & test = this->Test;

    test.UTContext.Clock.SetManualMode();
    test.RestartRA();
    test.UTContext.Clock.AdvanceTime(::ResourceMonitor::ResourceMonitorServiceConfig::GetConfig().ResourceTracingInterval);

    test.AddFT(L"RMC1", L"O None 000/000/411 1:1 CM [N/N/I RD U N F 1:1]");
    wstring reportFT1 = wformatString(L"[{0} 100 1.0]", StateManagement::Default::singleton_->RMC1_FTContext.FUID);
    test.ProcessRMMessageAndDrain<MessageType::ResourceUsageReport>(reportFT1);

    auto report1 = test.StateItemHelpers.EventWriterHelper.GetEvent<Diagnostics::ResourceUsageReportEventData>(0);
    Verify::AreEqual(report1->FtId, StateManagement::Default::singleton_->RMC1_FTContext.FUID.Guid);

    test.StateItemHelpers.EventWriterHelper.Reset();
    test.UTContext.Clock.AdvanceTime(::ResourceMonitor::ResourceMonitorServiceConfig::GetConfig().ResourceTracingInterval / 2);

    wstring reportFT2 = wformatString(L"[{0} 100 2.0]", StateManagement::Default::singleton_->RMC1_FTContext.FUID);
    test.ProcessRMMessageAndDrain<MessageType::ResourceUsageReport>(reportFT2);

    //we should not trace as the interval has not passed
    Verify::IsTrue(test.StateItemHelpers.EventWriterHelper.IsTracedEventsEmpty());
}

BOOST_AUTO_TEST_CASE(ResourceMonitor_LoadReportFTClosed)
{
    auto & test = this->Test;

    test.AddFT(L"RMC1", L"O None 000/000/411 1:1 CM [N/N/I RD U N F 1:1]");
    test.AddFT(L"RMC2", L"C None 000/000/411 1:1 -");

    wstring reportFT1 = wformatString(L"[{0} 100 1.0]", StateManagement::Default::singleton_->RMC1_FTContext.FUID);
    wstring reportFT2 = wformatString(L"[{0} 50 1.0]", StateManagement::Default::singleton_->RMC2_FTContext.FUID);
    wstring report = wformatString(L"{0} {1}", reportFT1, reportFT2);

    test.ProcessRMMessageAndDrain<MessageType::ResourceUsageReport>(report);

    test.ValidateFMMessage<MessageType::ReportLoad>(wformatString(
        L"[{0} {1} false S\\servicefabric:/_MemoryInMB\\100\\{2} S\\servicefabric:/_CpuCores\\1000000\\{3}]",
        StateManagement::Default::singleton_->RMC1_FTContext.FUID,
        StateManagement::Default::singleton_->RMC_STContext.SD.Name,
        test.RA.NodeId,
        test.RA.NodeId));
}

BOOST_AUTO_TEST_CASE(ResourceMonitor_LoadReportFTNotPresent)
{
    auto & test = this->Test;

    test.AddFT(L"RMC1", L"O None 000/000/411 1:1 CM [N/N/I RD U N F 1:1]");

    wstring reportFT1 = wformatString(L"[{0} 100 1.0]", StateManagement::Default::singleton_->RMC1_FTContext.FUID);
    wstring reportFT2 = wformatString(L"[{0} 50 1.0]", StateManagement::Default::singleton_->RMC2_FTContext.FUID);
    wstring report = wformatString(L"{0} {1}", reportFT1, reportFT2);

    test.ProcessRMMessageAndDrain<MessageType::ResourceUsageReport>(report);

    test.ValidateFMMessage<MessageType::ReportLoad>(wformatString(
        L"[{0} {1} false S\\servicefabric:/_MemoryInMB\\100\\{2} S\\servicefabric:/_CpuCores\\1000000\\{3}]",
        StateManagement::Default::singleton_->RMC1_FTContext.FUID,
        StateManagement::Default::singleton_->RMC_STContext.SD.Name,
        test.RA.NodeId,
        test.RA.NodeId));
}

BOOST_AUTO_TEST_CASE(ResourceMonitor_LoadReportWithShared)
{
    auto & test = this->Test;

    test.AddFT(L"RMC1", L"O None 000/000/411 1:1 CM [N/N/I RD U N F 1:1]");
    test.AddFT(L"SP1", L"O None 000/000/411 1:1 CM [N/N/S RD U N F 1:1]");

    wstring reportFT = wformatString(L"[{0} 50 0.1]", StateManagement::Default::singleton_->RMC1_FTContext.FUID);
    wstring report = wformatString(L"{0}", reportFT);

    test.ProcessRMMessageAndDrain<MessageType::ResourceUsageReport>(report);

    test.ValidateFMMessage<MessageType::ReportLoad>(wformatString(
        L"[{0} {1} false S\\servicefabric:/_MemoryInMB\\50\\{2} S\\servicefabric:/_CpuCores\\100000\\{3}]",
        StateManagement::Default::singleton_->RMC1_FTContext.FUID,
        StateManagement::Default::singleton_->RMC_STContext.SD.Name,
        test.RA.NodeId,
        test.RA.NodeId));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
