// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

using std::wstring;

using Common::Assert;
using Common::Stopwatch;
using Common::StopwatchTime;
using Common::StringWriter;
using Common::TimeSpan;

double const MinCoefficient = 0.001;
uint const MAX_VALUE = std::numeric_limits<uint>::max();
double const MAX_DOUBLE = std::numeric_limits<double>::max();

DecayAverage::DecayAverage(
    double decayFactor,
    TimeSpan decayInterval)
    : decayFactor_(decayFactor)
    , decayInterval_(decayInterval)
    , lastUpdatedTime_(0)
    , lastValueMilliSeconds_(0)
    , weightedSumMilliSeconds_(0)
    , sumOfWeightMilliSeconds_(0)
{
    ASSERT_IF(decayFactor_ < 0 || decayFactor_ > 1, "Invalid Decay factor {0}. It must be greater than 0 and less than 1", decayFactor_);
}

DecayAverage::DecayAverage(DecayAverage && other)
    : decayFactor_(other.decayFactor_)
    , decayInterval_(move(other.decayInterval_))
    , lastUpdatedTime_(move(other.lastUpdatedTime_))
    , lastValueMilliSeconds_(other.lastValueMilliSeconds_)
    , weightedSumMilliSeconds_(other.weightedSumMilliSeconds_)
    , sumOfWeightMilliSeconds_(other.sumOfWeightMilliSeconds_)
{
}

TimeSpan DecayAverage::get_Value() const
{
    if (decayFactor_ == 0.0)
    {
        return TimeSpan::FromMilliseconds(static_cast<double>(lastValueMilliSeconds_));
    }

    if (weightedSumMilliSeconds_ == 0.0)
    {
        return TimeSpan::Zero;
    }

    double value = weightedSumMilliSeconds_ / sumOfWeightMilliSeconds_;
    
    return TimeSpan::FromMilliseconds(value);
}

void DecayAverage::Reset()
{
    lastUpdatedTime_ = Stopwatch::Now();
    lastValueMilliSeconds_ = 0;
    weightedSumMilliSeconds_ = 0;
    sumOfWeightMilliSeconds_ = 0;
}

void DecayAverage::UpdateDecayFactor(
    double newDecayFactor,
    TimeSpan const & decayInterval)
{
    decayFactor_ = newDecayFactor;
    decayInterval_ = decayInterval;
    ASSERT_IF(decayFactor_ < 0 || decayFactor_ > 1, "Invalid Decay factor {0}. It must be greater than 0 and less than 1", decayFactor_);
}

void DecayAverage::Update(TimeSpan const & value)
{
    uint millisecondsValue;
    StopwatchTime now = Stopwatch::Now();

    if (value.TotalMilliseconds() > MAX_VALUE)
    {
        millisecondsValue = MAX_VALUE;
    }
    else
    {
        millisecondsValue = static_cast<uint>(value.TotalMilliseconds());
    }

    if (decayFactor_ == 0.0)
    {
        weightedSumMilliSeconds_ = static_cast<double>(millisecondsValue);
        sumOfWeightMilliSeconds_ = 1.0;
    }
    else
    {
        TimeSpan interval = now - lastUpdatedTime_;
        double power = interval / decayInterval_;
        double coefficient = pow(decayFactor_, power);
        
        if (coefficient > MinCoefficient &&
            MAX_DOUBLE - weightedSumMilliSeconds_ > millisecondsValue)  // Protect against overflow here
        {
            weightedSumMilliSeconds_ = weightedSumMilliSeconds_ * coefficient + static_cast<double>(millisecondsValue);
            sumOfWeightMilliSeconds_ = sumOfWeightMilliSeconds_ * coefficient + 1.0;
        }
        else
        {
            weightedSumMilliSeconds_ = static_cast<double>(millisecondsValue);
            sumOfWeightMilliSeconds_ = 1.0;
        }
    }

    lastValueMilliSeconds_ = millisecondsValue;
    lastUpdatedTime_ = now;
}

wstring DecayAverage::ToString() const
{
    wstring content;
    StringWriter writer(content);
    WriteTo(writer, Common::FormatOptions(0, false, ""));
    return content;
}

void DecayAverage::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write("{0}ms", Value.TotalMilliseconds());
}

}
}
