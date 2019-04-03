// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "Common/ApiMonitoring.MonitoringTesting.h"

using namespace Common;
using namespace std;
using namespace ApiMonitoring;
using namespace MonitoringTesting;

namespace
{
    class MyRoot : public ComponentRoot
    {
    public:
    };

    StopwatchTime GetExpiryTime(ApiCallDescriptionSPtr const & description)
    {
        return description->Data.StartTime + description->Parameters.ApiSlowDuration;
    }

    StopwatchTime GetExpiryTime(
        ApiCallDescriptionSPtr const & description1,
        ApiCallDescriptionSPtr const & description2,
        bool shouldExpire)
    {
        auto firstExpiry = GetExpiryTime(description1);
        auto secondExpiry = GetExpiryTime(description2);

        if (shouldExpire)
        {
            return max(firstExpiry, secondExpiry) + TimeSpan::FromSeconds(1);
        }
        else
        {
            return min(firstExpiry, secondExpiry) - TimeSpan::FromSeconds(1);
        }
    }

    namespace EventName
    {
        enum Enum
        {
            HealthSlow = 0,
            ClearHealthSlow = 1,
            TraceStart = 2,
            TraceEnd = 3,
            TraceSlow = 4,
        };
    }

    struct Event
    {
        EventName::Enum Name;
        shared_ptr<MonitoringData> Data;

        FABRIC_SEQUENCE_NUMBER SequenceNumber;
        TimeSpan Elapsed;
        ErrorCode Error; 
    };

    class EventList
    {
    public:
        void VerifyCount(int expected)
        {
            VERIFY_ARE_EQUAL(expected, events_.size(), L"Size");
        }

        void VerifySequenceNumberIsGreater(size_t earlierEventIndex, size_t laterEventIndex)
        {
            auto const & earlierEvent = GetEvent(earlierEventIndex);
            auto const & laterEvent = GetEvent(laterEventIndex);

            VERIFY_IS_TRUE(earlierEvent.SequenceNumber < laterEvent.SequenceNumber, L"Sequence Number");
        }

        void Clear()
        {
            events_.clear();
        }

        void HealthCallback(EventName::Enum name, MonitoringHealthEventList const & events)
        {
            VERIFY_IS_FALSE(events.empty(), L"Events cannot be empty");

            for (auto const & i : events)
            {
                Event e;
                e.Name = name;
                e.Data = make_shared<MonitoringData>(*i.first);
                e.SequenceNumber = i.second;
                events_.push_back(e);
            }
        }

        void ApiTrace(EventName::Enum name, MonitoringData const & monitoringData, TimeSpan elapsed, ErrorCode const & error)
        {
            Event e;
            e.Name = name;
            e.Data = make_shared<MonitoringData>(monitoringData);
            e.SequenceNumber = 0;
            e.Elapsed = elapsed;
			e.Error = error;
            events_.push_back(e);
        }

        void VerifyHealthSlow(size_t index, ApiCallDescriptionSPtr const & expected)
        {
            VerifyEvent(index, EventName::HealthSlow, expected);
        }

        void VerifyClearHealthSlow(size_t index, ApiCallDescriptionSPtr const & expected)
        {
            VerifyEvent(index, EventName::ClearHealthSlow, expected);
        }

        void VerifyApiStart(size_t index, ApiCallDescriptionSPtr const & expected)
        {
            VerifyEvent(index, EventName::TraceStart, expected);
        }

        void VerifyApiEnd(size_t index, ApiCallDescriptionSPtr const & expected)
        {
            VerifyEvent(index, EventName::TraceEnd, expected);
        }

        void VerifyApiSlow(size_t index, ApiCallDescriptionSPtr const & expected)
        {
            VerifyEvent(index, EventName::TraceSlow, expected);
        }

        void VerifyNoEvents()
        {
            VERIFY_ARE_EQUAL(0, events_.size(), L"EventSize");
        }

        Event const & GetEvent(size_t index) const
        {
            return events_[index];
        }

    private:

        void VerifyEvent(
            size_t index, 
            EventName::Enum expectedName, 
            ApiCallDescriptionSPtr const & expected)
        {
            VERIFY_IS_TRUE(index < events_.size(), L"Event not found");
            
            auto const & actual = GetEvent(index);
            VERIFY_ARE_EQUAL(expectedName, actual.Name, L"Event Type");
            VERIFY_ARE_EQUAL(expected->Data.ReplicaId, actual.Data->ReplicaId, L"ReplicaId");
        }

        vector<Event> events_;
    };
}

class TestMonitoringComponent
{
protected:
    void ApiStartIsTracedIfEnabledTestHelper(EnabledTraceFlags::Enum flags);
    void ApiEndIsTracedIfEnabledTestHelper(EnabledTraceFlags::Enum flags);
    void ApiSlowIsTracedTestHelper(EnabledTraceFlags::Enum flags);
    ~TestMonitoringComponent() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    void TestSetup();
    void CreateComponent();

    ApiCallDescriptionSPtr Start(
        function<ApiCallDescriptionSPtr ()> factory);

    ApiCallDescriptionSPtr Start(
        ApiCallDescriptionSPtr const & description);

    void Stop(
        ApiCallDescriptionSPtr const & description, 
        ErrorCode const & error);

    void Stop(
        ApiCallDescriptionSPtr const & description);

    void OnTimer(
        StopwatchTime now);

    void Close();

    void VerifyTimerIsSet();
    void VerifyTimerIsNotSet();
    void VerifyTimerIsNull();

    void RunExpiryScenario(
        ApiCallDescriptionSPtr const & description,
        bool shouldExpire);

    void RunTwoExpiryTest(
        ApiCallDescriptionSPtr const & description1,
        ApiCallDescriptionSPtr const & description2,
        bool shouldExpire);

	void RunMultipleOnTimerScenario(
		ApiCallDescriptionSPtr const & description,
		TimeSpan duration,
		TimeSpan interval);

	void RunMultipleOnTimerScenario(
		ApiCallDescriptionSPtr const & description,
		TimeSpan start,
		TimeSpan duration,
		TimeSpan interval);

    shared_ptr<MyRoot> root_;
    EventList eventList_;
    MonitoringComponentUPtr component_;
};

void TestMonitoringComponent::RunTwoExpiryTest(
    ApiCallDescriptionSPtr const & description1,
    ApiCallDescriptionSPtr const & description2,
    bool shouldExpire)
{
    auto now = GetExpiryTime(description1, description2, shouldExpire);

    Start(description1);
    Start(description2);

    eventList_.Clear();
    OnTimer(now);
}

void TestMonitoringComponent::RunExpiryScenario(
    ApiCallDescriptionSPtr const & description,
    bool shouldExpire)
{
    Start(description);
    eventList_.Clear();

    auto expiry = GetExpiryTime(description);
    auto currentTime = expiry;
    if (!shouldExpire)
    {
        currentTime -= TimeSpan::FromSeconds(1);
    }

    OnTimer(currentTime);
}

void TestMonitoringComponent::RunMultipleOnTimerScenario(
	ApiCallDescriptionSPtr const & description,
	TimeSpan duration,
	TimeSpan interval)
{
	RunMultipleOnTimerScenario(description, TimeSpan::Zero, duration, interval);
}

void TestMonitoringComponent::RunMultipleOnTimerScenario(
	ApiCallDescriptionSPtr const & description,
	TimeSpan offset,
	TimeSpan duration,
	TimeSpan interval)
{
	Start(description);
	eventList_.Clear();

	auto start = description->Data.StartTime + offset;
	auto end = start + duration;

	auto currentTime = start;
	OnTimer(currentTime);

	while (currentTime < end)
	{
		currentTime += interval;
		OnTimer(currentTime);
	}
}

void TestMonitoringComponent::CreateComponent()
{
    root_ = make_shared<MyRoot>();

    MonitoringComponentConstructorParameters parameters;
    parameters.ClearSlowHealthReportCallback = [this](MonitoringHealthEventList const & events, MonitoringComponentMetadata const &) { eventList_.HealthCallback(EventName::ClearHealthSlow, events); };
    parameters.SlowHealthReportCallback = [this](MonitoringHealthEventList const & events, MonitoringComponentMetadata const &) { eventList_.HealthCallback(EventName::HealthSlow, events); };
    parameters.Root = root_.get();
    parameters.ScanInterval = TimeSpan::FromMinutes(20); // high value - dont want timer to actually fire in the test

    component_ = make_unique<MonitoringComponent>(
        parameters,
        [this](MonitoringData const & data, MonitoringComponentMetadata const &) { eventList_.ApiTrace(EventName::TraceStart, data, TimeSpan::Zero, ErrorCodeValue::Success); },
        [this](MonitoringData const & data, MonitoringComponentMetadata const &) { eventList_.ApiTrace(EventName::TraceSlow, data, TimeSpan::Zero, ErrorCodeValue::Success); },
        [this](MonitoringData const & data, TimeSpan elapsed, ErrorCode const & error, MonitoringComponentMetadata const &) { eventList_.ApiTrace(EventName::TraceEnd, data, elapsed, error); });
}

void TestMonitoringComponent::TestSetup()
{
    TestCleanup();

    CreateComponent();

    component_->Open(MonitoringComponentMetadata(L"", Federation::NodeInstance()));
}

bool TestMonitoringComponent::TestCleanup()
{
    if (component_ != nullptr)
    {
        Close();
        component_.reset();
    }

    eventList_.Clear();
    root_.reset();

    return true;
}

void TestMonitoringComponent::Close()
{
    component_->Close();
}

void TestMonitoringComponent::VerifyTimerIsSet()
{
    VERIFY_IS_TRUE(component_->Test_GetTimer != nullptr, L"Timer: Not null");
    VERIFY_IS_TRUE(component_->Test_GetTimer->Test_IsSet(), L"Timer: IsSet");
}

void TestMonitoringComponent::VerifyTimerIsNotSet()
{
    VERIFY_IS_TRUE(component_->Test_GetTimer != nullptr, L"Timer: IsNotSet");
    VERIFY_IS_TRUE(!component_->Test_GetTimer->Test_IsSet(), L"Timer: IsNotSet");
}

void TestMonitoringComponent::VerifyTimerIsNull()
{
    VERIFY_IS_TRUE(component_->Test_GetTimer == nullptr, L"Timer: IsNull");
}

ApiCallDescriptionSPtr TestMonitoringComponent::Start(function<ApiCallDescriptionSPtr()> factory)
{
    return Start(factory());
}

ApiCallDescriptionSPtr TestMonitoringComponent::Start(ApiCallDescriptionSPtr const & description)
{
    component_->StartMonitoring(description);
    return description;
}

void TestMonitoringComponent::Stop(ApiCallDescriptionSPtr const & description)
{
    Stop(description, ErrorCodeValue::GlobalLeaseLost);
}

void TestMonitoringComponent::Stop(ApiCallDescriptionSPtr const & description, ErrorCode const & error)
{
    component_->StopMonitoring(description, error);
}

void TestMonitoringComponent::OnTimer(StopwatchTime now)
{
    component_->Test_OnTimer(now);
}

void TestMonitoringComponent::ApiStartIsTracedIfEnabledTestHelper(EnabledTraceFlags::Enum flags)
{
    TestSetup();

    auto desc = Start([&]() { return CallDescriptionTesting::Create(1, true, flags, TimeSpan::FromSeconds(1)); });

    eventList_.VerifyApiStart(0, desc);
}

void TestMonitoringComponent::ApiEndIsTracedIfEnabledTestHelper(EnabledTraceFlags::Enum flags)
{
    auto duration = TimeSpan::FromSeconds(5);
    auto startTime = Stopwatch::Now() - duration;
    auto error = ErrorCodeValue::GlobalLeaseLost;

    TestSetup();

    auto desc = Start([&]() { return CallDescriptionTesting::Create(1, true, flags, TimeSpan::FromSeconds(51), startTime); });

    eventList_.Clear();
    Stop(desc, error);

    eventList_.VerifyApiEnd(0, desc);
    VERIFY_ARE_EQUAL(error, eventList_.GetEvent(0).Error.ReadValue(), L"ErrorCode");
    VERIFY_IS_TRUE(eventList_.GetEvent(0).Elapsed >= duration, L"Duration");
}

void TestMonitoringComponent::ApiSlowIsTracedTestHelper(EnabledTraceFlags::Enum flags)
{
    TestSetup();

    auto desc = CallDescriptionTesting::Create(1, false, flags, TimeSpan::FromMilliseconds(1));

    RunExpiryScenario(desc, true);

    eventList_.VerifyApiSlow(0, desc);
}

BOOST_FIXTURE_TEST_SUITE(TestMonitoringComponentSuite,TestMonitoringComponent)

BOOST_AUTO_TEST_CASE(Monitoring_Tracing_ApiStartIsTraced_IfEnabled)
{
    ApiStartIsTracedIfEnabledTestHelper(EnabledTraceFlags::All);
    ApiStartIsTracedIfEnabledTestHelper(EnabledTraceFlags::LifeCycle);
}

BOOST_AUTO_TEST_CASE(Monitoring_Tracing_ApiEndIsTraced_IfEnabled)
{
    ApiEndIsTracedIfEnabledTestHelper(EnabledTraceFlags::All);
    ApiEndIsTracedIfEnabledTestHelper(EnabledTraceFlags::LifeCycle);
}

BOOST_AUTO_TEST_CASE(Monitoring_Tracing_HappensEvenIfClosedIfIsEnabled)
{
    TestSetup();

    Close();

    auto desc = Start([&]() { return CallDescriptionTesting::Create(1, true, EnabledTraceFlags::All, TimeSpan::FromSeconds(1)); });

    eventList_.VerifyApiStart(0, desc);

    eventList_.Clear();
    Stop(desc, ErrorCode::Success());

    eventList_.VerifyApiEnd(0, desc);
}

BOOST_AUTO_TEST_CASE(Monitoring_Timer_IsNotArmedAtStart)
{
    CreateComponent();
    VerifyTimerIsNull();
}

BOOST_AUTO_TEST_CASE(Monitoring_Timer_IsArmedIfReportsArePresent)
{
    TestSetup();
    
    Start([&]() { return CallDescriptionTesting::Create(1); });

    VerifyTimerIsSet();
}

BOOST_AUTO_TEST_CASE(Monitoring_Timer_IsCancelledIfNoReportsArePresent)
{
    TestSetup();

    auto desc = Start([&]() { return CallDescriptionTesting::Create(1); });

    Stop(desc);

    VerifyTimerIsNotSet();
}

BOOST_AUTO_TEST_CASE(Monitoring_Timer_IsNotCancelledIfReportsArePresent)
{
    TestSetup();

    auto desc = Start([&]() { return CallDescriptionTesting::Create(1); });
    Start([&]() { return CallDescriptionTesting::Create(2); });

    Stop(desc);

    VerifyTimerIsSet();
}

BOOST_AUTO_TEST_CASE(Monitoring_Timer_IsCancelledOnClose)
{
    TestSetup();

    Start([&]() { return CallDescriptionTesting::Create(1); });

    Close();

    VerifyTimerIsNull();
}

BOOST_AUTO_TEST_CASE(Monitoring_Timer_IsArmedAfterCallback)
{
    TestSetup();

    auto desc = CallDescriptionTesting::Create(1, true, EnabledTraceFlags::All, TimeSpan::FromMilliseconds(2));

    RunExpiryScenario(desc, true);

    VerifyTimerIsSet();
}

BOOST_AUTO_TEST_CASE(Monitoring_Tracing_ApiSlowIsTraced)
{
    ApiSlowIsTracedTestHelper(EnabledTraceFlags::Slow);
    ApiSlowIsTracedTestHelper(EnabledTraceFlags::All);
}

BOOST_AUTO_TEST_CASE(Monitoring_Expiry_NoActionIfNotSlow)
{
    TestSetup();

    auto desc = CallDescriptionTesting::Create(1, false, EnabledTraceFlags::All, TimeSpan::FromMilliseconds(1));

    RunExpiryScenario(desc, false);

    eventList_.VerifyNoEvents();
}

BOOST_AUTO_TEST_CASE(Monitoring_Expiry_BothActionsIfBothEnabled)
{
    TestSetup();

    auto desc1 = CallDescriptionTesting::Create(1, true, EnabledTraceFlags::All, TimeSpan::FromSeconds(5));
    auto desc2 = CallDescriptionTesting::Create(2, true, EnabledTraceFlags::All, TimeSpan::FromSeconds(10));

    RunTwoExpiryTest(desc1, desc2, true);

    eventList_.VerifyCount(4);
}

BOOST_AUTO_TEST_CASE(Monitoring_Expiry_ActionsPerformedOnlyOnce)
{
    TestSetup();

    auto desc1 = CallDescriptionTesting::Create(1, true, EnabledTraceFlags::All, TimeSpan::FromSeconds(5));
    auto desc2 = CallDescriptionTesting::Create(2, true, EnabledTraceFlags::All, TimeSpan::FromSeconds(10));

    RunTwoExpiryTest(desc1, desc2, true);

    // Expire again
    eventList_.Clear();
    OnTimer(GetExpiryTime(desc1, desc2, true));

    eventList_.VerifyNoEvents();
}

BOOST_AUTO_TEST_CASE(Monitoring_Expiry_OnlyExpiredActionsAreRaised)
{
    auto smallDuration = TimeSpan::FromMilliseconds(5);
    auto largeDuration = TimeSpan::FromMilliseconds(15);

    auto smallStartTime = Stopwatch::Now();
    auto largeStartTime = smallStartTime + TimeSpan::FromMilliseconds(10);

    TestSetup();

    auto desc1 = CallDescriptionTesting::Create(2, false, EnabledTraceFlags::All, largeDuration, largeStartTime);
    auto desc2 = CallDescriptionTesting::Create(11, false, EnabledTraceFlags::All, smallDuration, smallStartTime);

    Start(desc1);
    Start(desc2);

    eventList_.Clear();

    // First timer should invoke only small duration
    OnTimer(smallStartTime + TimeSpan::FromMilliseconds(12));
    eventList_.VerifyCount(1);
    eventList_.VerifyApiSlow(0, desc2);

    // Simulate spurious - no events generated
    eventList_.Clear();
    OnTimer(smallStartTime + TimeSpan::FromMilliseconds(3));
    eventList_.VerifyNoEvents();

    eventList_.Clear();
    OnTimer(smallStartTime + TimeSpan::FromMilliseconds(25));
    eventList_.VerifyApiSlow(0, desc1);
}

BOOST_AUTO_TEST_CASE(Monitoring_Expiry_NoOpIfClosed)
{
    auto duration = TimeSpan::FromSeconds(5);
    auto start = Stopwatch::Now();

    TestSetup();

    auto desc = Start([&]() { return CallDescriptionTesting::Create(1, true, EnabledTraceFlags::All, duration, start); });
    eventList_.Clear();

    auto currentTime = GetExpiryTime(desc);

    Close();

    OnTimer(currentTime);

    eventList_.VerifyNoEvents();    
}

BOOST_AUTO_TEST_CASE(Monitoring_Health_NoEventReportedIfCompletedWithoutExpiry)
{
    TestSetup();

    auto desc = Start([&]() { return CallDescriptionTesting::Create(1, true, EnabledTraceFlags::None, TimeSpan::FromMinutes(50), Stopwatch::Now()); });

    Stop(desc);

    eventList_.VerifyNoEvents();
}

BOOST_AUTO_TEST_CASE(Monitoring_Health_SlowEventReportedOnSlow)
{
    TestSetup();

    auto desc1 = CallDescriptionTesting::Create(1, true, EnabledTraceFlags::None, TimeSpan::FromSeconds(1));
    RunExpiryScenario(desc1, true);

    eventList_.VerifyHealthSlow(0, desc1);
}

BOOST_AUTO_TEST_CASE(Monitoring_Health_MultipleSlowAreCoalesced)
{
    TestSetup();

    auto desc1 = CallDescriptionTesting::Create(1, true, EnabledTraceFlags::None, TimeSpan::FromSeconds(1));
    auto desc2 = CallDescriptionTesting::Create(2, true, EnabledTraceFlags::None, TimeSpan::FromSeconds(2));

    RunTwoExpiryTest(desc1, desc2, true);

    eventList_.VerifyCount(2);
}

BOOST_AUTO_TEST_CASE(Monitoring_Health_ClearSlowEventReportedOnCompleteOfSlowApi)
{
    TestSetup();

    auto desc = CallDescriptionTesting::Create(1, true, EnabledTraceFlags::None, TimeSpan::FromMilliseconds(1));
    RunExpiryScenario(desc, true);

    eventList_.Clear();

    Stop(desc);

    eventList_.VerifyClearHealthSlow(0, desc);
}

BOOST_AUTO_TEST_CASE(Monitoring_Health_ClearSlowEventNotReportedOnCompleteOfSlowApi_HealthTracingDisabled)
{
    TestSetup();

    auto desc = CallDescriptionTesting::Create(1, false, EnabledTraceFlags::None, TimeSpan::FromMilliseconds(1));
    RunExpiryScenario(desc, true);

    eventList_.Clear();

    Stop(desc);

    eventList_.VerifyNoEvents();
}

BOOST_AUTO_TEST_CASE(Monitoring_Health_SequenceNumber_ClearEventHasHigherSequenceNumber)
{
    TestSetup();

    auto desc = CallDescriptionTesting::Create(1, true, EnabledTraceFlags::None, TimeSpan::FromMilliseconds(1));
    RunExpiryScenario(desc, true);

    Stop(desc);

    eventList_.VerifySequenceNumberIsGreater(0, 1);
}

BOOST_AUTO_TEST_CASE(Monitoring_Health_SequenceNumber_SlowEventsHaveUniqueSequenceNumber)
{
    TestSetup();

    auto desc1 = CallDescriptionTesting::Create(1, true, EnabledTraceFlags::None, TimeSpan::FromMilliseconds(1));
    auto desc2 = CallDescriptionTesting::Create(2, true, EnabledTraceFlags::None, TimeSpan::FromMilliseconds(2));

    RunTwoExpiryTest(desc1, desc2, true);

    eventList_.VerifySequenceNumberIsGreater(0, 1);
}

BOOST_AUTO_TEST_CASE(Monitoring_PeriodicSlowTrace_MultipleTraces)
{
	TestSetup();

	auto desc = CallDescriptionTesting::Create(1, false, EnabledTraceFlags::All, TimeSpan::FromMilliseconds(1), Stopwatch::Now(), TimeSpan::FromMilliseconds(3));

	RunMultipleOnTimerScenario(desc, TimeSpan::FromMilliseconds(8), TimeSpan::FromMilliseconds(1));

	eventList_.VerifyCount(3);
	eventList_.VerifyApiSlow(0, desc);
	eventList_.VerifyApiSlow(1, desc);
	eventList_.VerifyApiSlow(2, desc);
}

BOOST_AUTO_TEST_CASE(Monitoring_PeriodicSlowTrace_TraceOnlyOnce)
{
	TestSetup();

	auto desc = CallDescriptionTesting::Create(1, false, EnabledTraceFlags::All, TimeSpan::FromMilliseconds(1), Stopwatch::Now(), TimeSpan::FromMilliseconds(3));

	RunMultipleOnTimerScenario(desc, TimeSpan::FromMilliseconds(2), TimeSpan::FromMilliseconds(1));

	eventList_.VerifyCount(1);
	eventList_.VerifyApiSlow(0, desc);
}

BOOST_AUTO_TEST_CASE(Monitoring_PeriodicSlowTrace_OnlyOneHealth)
{
	TestSetup();

	auto desc = CallDescriptionTesting::Create(1, true, EnabledTraceFlags::None, TimeSpan::FromMilliseconds(1), Stopwatch::Now(), TimeSpan::FromMilliseconds(3));

	RunMultipleOnTimerScenario(desc, TimeSpan::FromMilliseconds(8), TimeSpan::FromMilliseconds(1));

	eventList_.VerifyCount(1);
	eventList_.VerifyHealthSlow(0, desc);
}

BOOST_AUTO_TEST_CASE(Monitoring_PeriodicSlowTrace_NoDoubleTrace)
{
	TestSetup();

	auto desc = CallDescriptionTesting::Create(1, false, EnabledTraceFlags::All, TimeSpan::FromMilliseconds(1), Stopwatch::Now(), TimeSpan::FromMilliseconds(1));

	RunMultipleOnTimerScenario(desc, TimeSpan::FromMilliseconds(3), TimeSpan::FromMilliseconds(0), TimeSpan::FromMilliseconds(1));

	eventList_.VerifyCount(1);
	eventList_.VerifyApiSlow(0, desc);
}

BOOST_AUTO_TEST_CASE(Monitoring_PeriodicSlowTrace_ClearSlow)
{
	TestSetup();

	auto desc = CallDescriptionTesting::Create(1, true, EnabledTraceFlags::Slow, TimeSpan::FromMilliseconds(1), Stopwatch::Now(), TimeSpan::FromMilliseconds(3));

	RunMultipleOnTimerScenario(desc, TimeSpan::FromMilliseconds(8), TimeSpan::FromMilliseconds(1));

	eventList_.Clear();
	
	Stop(desc);

	eventList_.VerifyCount(1);
	eventList_.VerifyClearHealthSlow(0, desc);
}

BOOST_AUTO_TEST_SUITE_END()
