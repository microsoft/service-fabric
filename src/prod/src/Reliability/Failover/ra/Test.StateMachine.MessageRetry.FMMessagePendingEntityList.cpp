// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "RATestHeaders.h"

using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Infrastructure;
using namespace MessageRetry;
using namespace std;

using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateItemHelper;

namespace
{
    class StubThrottle : public Infrastructure::IThrottle
    {
    public:
        StubThrottle() : ThrottleValue(2)
        {
        }

        int GetCount(Common::StopwatchTime now)
        {
            Time.push_back(now);
            return ThrottleValue;
        }

        // Tell the throttle that 'count' elements happened at a particular time 
        void Update(int count, Common::StopwatchTime now)
        {
            Time.push_back(now);
            Counts.push_back(count);
        }

        std::vector<int> Counts;
        std::vector<Common::StopwatchTime> Time;
        int ThrottleValue;        
    };
    
    wstring const FT1(L"SP1");
    wstring const FT2(L"SP2");
}

class TestFMMessagePendingEntityList : public StateMachineTestBase
{
protected:

    // Test the object directly
    TestFMMessagePendingEntityList() :
        perfData_(RA.ClockSPtr),
        pendingList_(RA.GetFMMessageRetryComponent(*FailoverManagerId::Fm).Test_GetPendingEntityList())
    {
        pendingList_.Test_UpdateThrottle(&throttle_);

        start_ = Test.UTContext.Clock.SetManualMode();
    }

    void AddPendingEntities(initializer_list<wstring> names)
    {
        wstring const initial = L"O None 000/000/411 1:1 1 I [N/N/P SB D N F 1:1]";

        for (auto const & it : names)
        {
            Test.AddFT(it, initial);
        }
    }

    void Enumerate()
    {
        perfData_ = Diagnostics::FMMessageRetryPerformanceData(RA.ClockSPtr);
        result_ = pendingList_.Enumerate(perfData_);
    }

    void AdvanceTime(int seconds)
    {
        Test.UTContext.Clock.AdvanceTimeBySeconds(seconds);
    }

    void VerifyEnumeratedEntitiesSize(size_t expected)
    {
        Verify::AreEqual(expected, result_.Count, L"size");
    }

    void VerifyEnumeratedEntities(initializer_list<wstring> shortNames)
    {
        Verify::AreEqual(result_.Count, result_.Messages.size(), L"Enumeration result count");

        EntityEntryBaseList expected = Test.GetFailoverUnitEntries(shortNames);

        EntityEntryBaseList actual;
        for (auto const & it : result_.Messages)
        {
            actual.push_back(it.Source);
        }

        Verify::Vector(expected, actual);
    }

    void SetThrottle(int count)
    {
        throttle_.ThrottleValue = count;
    }

    void InvokeOnRetry(wstring const & shortName)
    {
        auto & ft = Test.GetFT(shortName);

        ReconfigurationAgentComponent::Communication::FMMessageData data;
        auto result = ft.TryComposeFMMessage(RA.NodeInstance, RA.Clock.Now(), data);
        Verify::IsTrue(result, L"expected message to be generated");

        Test.EntityExecutionContextTestUtilityObj.ExecuteEntityProcessor(
            shortName, 
            [&](EntityExecutionContext & base)
            {
                auto & context = base.As<FailoverUnitEntityExecutionContext>();
                ft.OnFMMessageSent(RA.Clock.Now(), data.SequenceNumber, context);
            });
    }

    void VerifyRetryType(BackgroundWorkRetryType::Enum expected)
    {
        VERIFY_ARE_EQUAL(expected, result_.RetryType, L"Retrytype");
    }

    void UpdateAndDrain(int secondsToAdvanceTime)
    {
        Test.UTContext.Clock.AdvanceTimeBySeconds(secondsToAdvanceTime);

        auto op = pendingList_.BeginUpdateEntitiesAfterSend(
            L"a",
            perfData_,
            result_,
            [this](AsyncOperationSPtr const & inner) { pendingList_.EndUpdateEntitiesAfterSend(inner); },
            AsyncOperationSPtr());

        Test.DrainJobQueues();

        Verify::IsTrue(op->IsCompleted, L"Async op completion");
    }

    FMMessagePendingEntityList::EnumerationResult result_;

    StubThrottle throttle_;
    FMMessagePendingEntityList & pendingList_;
    Diagnostics::FMMessageRetryPerformanceData perfData_;

    StopwatchTime start_;
};

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestFMMessagePendingEntityListSuite, TestFMMessagePendingEntityList)

BOOST_AUTO_TEST_CASE(AllPendingEntitiesAreReturned)
{
    AddPendingEntities({ FT1, FT2 });

    Enumerate();

    VerifyEnumeratedEntities({ FT1, FT2 });
}

BOOST_AUTO_TEST_CASE(IfThrottledThenFewerEntitiesAreReturned)
{
    AddPendingEntities({ FT1, FT2 });

    SetThrottle(1);

    Enumerate();

    VerifyEnumeratedEntitiesSize(1);
}

BOOST_AUTO_TEST_CASE(EntitiesThatAreNotPendingDoNotReturnError)
{
    AddPendingEntities({ FT1, FT2 });

    SetThrottle(1);

    InvokeOnRetry(FT1);

    Enumerate();

    VerifyEnumeratedEntities({ FT2 });
}

BOOST_AUTO_TEST_CASE(WithNoPendingEntitiesRetryTypeIsNone)
{
    Enumerate();

    VerifyRetryType(BackgroundWorkRetryType::None);
}

BOOST_AUTO_TEST_CASE(WithAllEntitiesEnumeratedRetryTypeIsDeferred)
{
    AddPendingEntities({ FT1, FT2 });

    Enumerate();

    VerifyRetryType(BackgroundWorkRetryType::Deferred);
}

BOOST_AUTO_TEST_CASE(IfthrottledRetryTypeIsImmediate)
{
    AddPendingEntities({ FT1, FT2 });

    SetThrottle(1);

    Enumerate();

    VerifyRetryType(BackgroundWorkRetryType::Immediate);
}

BOOST_AUTO_TEST_CASE(UpdateInvokesOnRetry)
{
    AddPendingEntities({ FT1, FT2 });

    Enumerate();

    UpdateAndDrain(2);

    Enumerate();

    VerifyEnumeratedEntities({});
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
