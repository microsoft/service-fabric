// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace std;

using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace ReliabilityUnitTest;
using namespace ServiceModel;
using namespace Transport;
using namespace ReliabilityUnitTest::StateManagement;

void ReliabilityUnitTest::BusyWaitUntil(
    std::function<bool ()> processor, 
    DWORD pollTimeoutInMs, 
    DWORD waitTimeoutInMs)
{
    DateTime start = DateTime::Now();

    for(;;)
    {
        if (processor())
        {
            return;
        }
        else if ((Common::DateTime::Now() - start) > Common::TimeSpan::FromMilliseconds(waitTimeoutInMs))
        {
            VERIFY_FAIL(L"validation timed out");
        }
        else
        {
            Sleep(pollTimeoutInMs);
        }
    }
}

void ReliabilityUnitTest::VerifyWithinConfidenceInterval(
    double expected,
    double actual,
    double lowerBoundMultiplier,
    double upperBoundMultiplier
    )
{
    wstring text = L"VerifyWithinConfidenceInterval: " + wformatString(expected * lowerBoundMultiplier) + L" <= " + wformatString(actual) + L" <= " + wformatString(expected * upperBoundMultiplier);
    TestLog::WriteInfo(text);
    VERIFY_IS_TRUE(expected * lowerBoundMultiplier <= actual);
    VERIFY_IS_TRUE(actual <= expected * upperBoundMultiplier);
}

wstring ReliabilityUnitTest::GetRandomString(wstring const & prefix, Random& random)
{
    wchar_t charTable[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};

    // The simplest random string algorithm ever
    // Good enough for our tests
    int length = random.Next(1, 20);

    wstring rv;
    for(int i = 0; i < length; i++)
    {
        int index = random.Next(ARRAYSIZE(charTable));
        rv += charTable[index];
    }

    if (!prefix.empty())
    {
        return prefix + L"_" + rv;
    }
    else
    {
        return rv;
    }
}
