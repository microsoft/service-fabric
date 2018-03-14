// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

namespace TestCommon
{
    using namespace std;
    using namespace Common;

    StringLiteral const verifierSource = "verifier";

    void TestVerifier::Expect(std::wstring const & message)
    {
        wstring text = message;
        StringUtility::TrimWhitespaces(text);
        TestSession::WriteNoise(verifierSource, "Expecting {0}.", text);

        {
            AcquireExclusiveLock lock(lock_);
            expecting_.push_back(text);
        }

        expectationResetEvent_.Reset();
    }

    bool TestVerifier::Validate(std::wstring const & message, bool ignoreUnmatched)
    {
        wstring text = message;
        StringUtility::TrimWhitespaces(text);
        TestSession::WriteNoise(verifierSource, "Validating {0}.", text);

        AcquireExclusiveLock lock(lock_);
        std::vector<std::wstring>::iterator messagePosition = find(expecting_.begin(), expecting_.end(), text);
        if(messagePosition != expecting_.end())
        {
            expecting_.erase(messagePosition);
            TestSession::WriteNoise(verifierSource, "Event {0} is valid.", text);
            if (expecting_.size() == 0)
            {
                expectationResetEvent_.Set();
            }

            return true;
        }
        else if (!ignoreUnmatched)
        {
            TestSession::FailTestIf(expecting_.size() == 0, "Event {0} is not expected!", text);

            TestSession::WriteError(verifierSource, "Event {0} is not expected!", text);
            errors_ += (L"(" + text + L")");

            return false;
        }
        else
        {
            TestSession::WriteWarning(verifierSource, "Event {0} is not expected and ignored!", text);
            return true;
        }
    } 

    bool TestVerifier::CheckPendingEvents()
    {
        AcquireExclusiveLock lock(lock_);

        TestSession::FailTestIf(errors_.size() > 0, "Events Not Expected! {0}", errors_);

        if (expecting_.size() == 0)
        {
            return true;
        }

        wstring output;
        StringWriter writer(output);

        writer.Write("{0} events pending: ", expecting_.size());
        int count = 0;
        for (std::wstring const & eventString : expecting_)
        {
            if (count++ < 10)
            {
                writer.Write("({0})", eventString);
            }
            else
            {
                writer.Write(" ...");
                break;
            }
        }
        TestSession::WriteInfo(verifierSource, "{0}", output);

        return false;
    }

    bool TestVerifier::WaitForExpectationEquilibrium(Common::TimeSpan timeout)
    {
        StopwatchTime stopwatchTime(Stopwatch::Now() + timeout);

        while (!CheckPendingEvents())
        {
            TimeSpan remain = stopwatchTime.RemainingDuration();
            if (remain <= TimeSpan::Zero)
            {
                return false;
            }

            expectationResetEvent_.WaitOne(min<int>(static_cast<int>(remain.TotalMilliseconds()), 3000));
        }

        return true;
    }

    void TestVerifier::Clear(int number)
    {
        AcquireExclusiveLock lock(lock_);
        if(static_cast<size_t>(number) <= expecting_.size())
        {
            TestSession::WriteNoise(verifierSource, "Clearing last {0} events!", number);
            expecting_.erase(expecting_.end() - number, expecting_.end());
        }
        else
        {
            TestSession::FailTest("Clear command called with argument '{0}' which is greater than list size ", number);
        }
    }

    void TestVerifier::Clear()
    {
        AcquireExclusiveLock lock(lock_);
        TestSession::WriteNoise(verifierSource, "{0}", L"Clearing all expected events!");
        expecting_.clear();
    }
}
