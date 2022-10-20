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

class TestCallDescription
{
protected:
	void VerifyActions(
		ApiCallDescriptionSPtr const & description,
		TimeSpan sinceStart,
		bool expectingTrace,
		bool expectingHealth);
};

void TestCallDescription::VerifyActions(
	ApiCallDescriptionSPtr const & description,
	TimeSpan sinceStart,
	bool expectingTrace,
	bool expectingHealth)
{
	auto now = description->Data.StartTime + sinceStart;

	MonitoringActionsNeeded actions = description->GetActions(now);

	VERIFY_ARE_EQUAL(expectingTrace, actions.first, L"Trace Needed");
	VERIFY_ARE_EQUAL(expectingHealth, actions.second, L"Health Needed");
}

BOOST_FIXTURE_TEST_SUITE(TestCallDescriptionSuite,TestCallDescription)

BOOST_AUTO_TEST_CASE(CallDescription_Actions_NeedsTraceAndHealth)
{
	auto desc = CallDescriptionTesting::Create(1, true, EnabledTraceFlags::All, TimeSpan::FromMilliseconds(1));

	VerifyActions(desc, TimeSpan::FromMilliseconds(4), true, true);
}

BOOST_AUTO_TEST_CASE(CallDescription_Actions_NoTraceOrHealth)
{
	auto desc = CallDescriptionTesting::Create(1, true, EnabledTraceFlags::All, TimeSpan::FromMilliseconds(5));

	VerifyActions(desc, TimeSpan::FromMilliseconds(1), false, false);
}

BOOST_AUTO_TEST_CASE(CallDescription_Actions_OnlyHealthEventOnce)
{
	auto desc = CallDescriptionTesting::Create(1, true, EnabledTraceFlags::All, TimeSpan::FromMilliseconds(1));

	VerifyActions(desc, TimeSpan::FromMilliseconds(4), true, true);
	VerifyActions(desc, TimeSpan::FromMilliseconds(4), false, false);
	VerifyActions(desc, TimeSpan::FromMilliseconds(8), false, false);
}

BOOST_AUTO_TEST_CASE(CallDescription_Actions_Periodic_NeedsTraceAndHealth)
{
	auto desc = CallDescriptionTesting::Create(1, true, EnabledTraceFlags::Slow, TimeSpan::FromMilliseconds(1), Stopwatch::Now(), TimeSpan::FromMilliseconds(3));

	VerifyActions(desc, TimeSpan::FromMilliseconds(4), true, true);
}

BOOST_AUTO_TEST_CASE(CallDescription_Actions_Periodic_NoTraceOrHealth)
{
	auto desc = CallDescriptionTesting::Create(1, true, EnabledTraceFlags::Slow, TimeSpan::FromMilliseconds(2), Stopwatch::Now(), TimeSpan::FromMilliseconds(3));

	VerifyActions(desc, TimeSpan::FromMilliseconds(1), false, false);
}

BOOST_AUTO_TEST_CASE(CallDescription_Actions_Periodic_OnlyHealthEventOnce)
{
	auto desc = CallDescriptionTesting::Create(1, true, EnabledTraceFlags::Slow, TimeSpan::FromMilliseconds(1), Stopwatch::Now(), TimeSpan::FromMilliseconds(3));

	VerifyActions(desc, TimeSpan::FromMilliseconds(3), true, true);
	VerifyActions(desc, TimeSpan::FromMilliseconds(5), false, false);
	VerifyActions(desc, TimeSpan::FromMilliseconds(8), true, false);
}

BOOST_AUTO_TEST_SUITE_END()
