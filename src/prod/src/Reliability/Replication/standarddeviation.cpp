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
double const MAX_DOUBLE = std::numeric_limits<double>::max();

// To ensure the square does not overflow
double const MAX_VALUE = sqrt(MAX_DOUBLE) - 1.0;

StandardDeviation::StandardDeviation()
    : count_(0)
    , milliSecondsSum_(0.0)
    , squaredMilliSecondsSum_(0.0)
{
}

TimeSpan StandardDeviation::get_StdDev() const
{
    if (count_ == 0)
        return TimeSpan::Zero;

    auto average = Average;
    
    double value = ((squaredMilliSecondsSum_ / count_) - (average.TotalMillisecondsAsDouble() * average.TotalMillisecondsAsDouble()));

    return value <= 0 ? TimeSpan::Zero : TimeSpan::FromMilliseconds(sqrt(value));
}

void StandardDeviation::Clear()
{
    count_ = 0;
    milliSecondsSum_ = 0.0;
    squaredMilliSecondsSum_ = 0.0;
}

void StandardDeviation::Add(TimeSpan const & value)
{ 
    double millisecondsValue;
    double squaredMillisecondsValue;

    // NOTE - Currently the max TimeSpan value does not exceed this check and hence this is moot
    // Keeping in the code in case that ever changes
    if (value.TotalMillisecondsAsDouble() > MAX_VALUE)
    {
        millisecondsValue = MAX_VALUE;
    }
    else
    {
        millisecondsValue = value.TotalMillisecondsAsDouble();
    }

    squaredMillisecondsValue = millisecondsValue * millisecondsValue;
    count_ += 1;
    milliSecondsSum_ += millisecondsValue;
    
    // Protect against overflow here
    if (MAX_DOUBLE - squaredMilliSecondsSum_ > squaredMillisecondsValue)
    {
        squaredMilliSecondsSum_ += squaredMillisecondsValue;
    }
    else
    {
        squaredMilliSecondsSum_ = MAX_DOUBLE;
    }
}

}
}
