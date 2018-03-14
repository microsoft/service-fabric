// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "LoadMetricStats.h"
#include "PLBConfig.h"
#include "Constants.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

LoadMetricStats::LoadMetricStats()
{
}

LoadMetricStats::LoadMetricStats(std::wstring && name, uint value, StopwatchTime timestamp)
    : name_(move(name)), 
    lastReportValue_(value), 
    lastReportTime_(timestamp),
    count_(1),
    weightedSum_(value),
    sumOfWeight_(1.0)
{
}

LoadMetricStats::LoadMetricStats(LoadMetricStats && other)
    : name_(move(other.name_)), 
    lastReportValue_(other.lastReportValue_), 
    lastReportTime_(other.lastReportTime_),
    count_(other.count_),
    weightedSum_(other.weightedSum_), 
    sumOfWeight_(other.sumOfWeight_)
{
}

LoadMetricStats::LoadMetricStats(LoadMetricStats const& other)
    : name_(other.name_), 
    lastReportValue_(other.lastReportValue_), 
    lastReportTime_(other.lastReportTime_),
    count_(other.count_),
    weightedSum_(other.weightedSum_), 
    sumOfWeight_(other.sumOfWeight_)
{
}

LoadMetricStats & LoadMetricStats::operator = (LoadMetricStats const & other)
{
    if (this != &other)
    {
        name_ = move(other.name_);
        lastReportValue_ = other.lastReportValue_;
        lastReportTime_ = other.lastReportTime_;
        count_ = other.count_;
        weightedSum_ = other.weightedSum_;
        sumOfWeight_ = other.sumOfWeight_;
    }

    return *this;
}

LoadMetricStats & LoadMetricStats::operator = (LoadMetricStats && other)
{
    if (this != &other) 
    {
        name_ = move(other.name_);
        lastReportValue_ = other.lastReportValue_;
        lastReportTime_ = other.lastReportTime_;
        count_ = other.count_;
        weightedSum_ = other.weightedSum_;
        sumOfWeight_ = other.sumOfWeight_;
    }

    return *this;
}

bool LoadMetricStats::operator == (LoadMetricStats const & other) const
{
    return Name == other.Name && Value == other.Value;
}

void LoadMetricStats::AdjustTimestamp(TimeSpan diff)
{
    lastReportTime_ += diff;
}

bool LoadMetricStats::Update(uint value, StopwatchTime timestamp)
{
    ValidateValues();

    bool updated = false;
    PLBConfig const& config = PLBConfig::GetConfig();

    if (config.LoadDecayFactor == 0.0)
    {
        if (timestamp > lastReportTime_)
        {
            if (count_ > 1 || lastReportValue_ != value)
            {
                lastReportValue_ = value;
                lastReportTime_ = timestamp;
                weightedSum_ = static_cast<double>(value);
                if (count_ > 1)
                {
                    count_ = 1;
                    sumOfWeight_ = 1.0;
                }
                updated = true;
            }
        }
        else if (count_ > 1)
        {
            count_ = 1;
            weightedSum_ = lastReportValue_;
            sumOfWeight_ = 1.0;
            updated = true;
        }
    }
    else
    {

        TimeSpan interval = timestamp - lastReportTime_;
        double power = interval / config.LoadDecayInterval;
        double coefficient = pow(config.LoadDecayFactor, power);

        if (coefficient > Constants::SmallValue)
        {
            // TODO: deal with overflow
            weightedSum_ = weightedSum_ * coefficient + static_cast<double>(value);
            sumOfWeight_ = sumOfWeight_ * coefficient + 1.0;
        }
        else
        {
            weightedSum_ = static_cast<double>(value);
            sumOfWeight_ = 1.0;
        }

        if (timestamp > lastReportTime_)
        {
            lastReportValue_ = value;
            lastReportTime_ = timestamp;
        }

        ++count_;
        updated = true;
    }

    return updated;
}

bool LoadMetricStats::Update(LoadMetricStats && other)
{
    ValidateValues();
    other.ValidateValues();

    bool updated = false;
    PLBConfig const& config = PLBConfig::GetConfig();

    if (config.LoadDecayFactor == 0.0)
    {
        if (other.lastReportTime_ > lastReportTime_)
        {
            if (count_ > 1 || lastReportValue_ != other.lastReportValue_)
            {
                lastReportValue_ = other.lastReportValue_;
                lastReportTime_ = other.lastReportTime_;
                weightedSum_ = static_cast<double>(other.lastReportValue_);
                if (count_ > 1)
                {
                    count_ = 1;
                    sumOfWeight_ = 1.0;
                }
                updated = true;
            }
        }
        else if (count_ > 1)
        {
            count_ = 1;
            weightedSum_ = lastReportValue_;
            sumOfWeight_ = 1.0;
            updated = true;
        }
    }
    else
    {

        TimeSpan interval = other.lastReportTime_ - lastReportTime_;
        double power = interval / config.LoadDecayInterval;
        double coefficient = pow(config.LoadDecayFactor, power);

        if (coefficient > Constants::SmallValue)
        {
            // TODO: deal with overflow
            weightedSum_ = weightedSum_ * coefficient + other.weightedSum_;
            sumOfWeight_ = sumOfWeight_ * coefficient + other.sumOfWeight_;
        }
        else
        {
            weightedSum_ = other.weightedSum_;
            sumOfWeight_ = other.sumOfWeight_;
        }

        if (other.lastReportTime_ > lastReportTime_)
        {
            lastReportValue_ = other.lastReportValue_;
            lastReportTime_ = other.lastReportTime_;
        }

        count_ += other.count_;
        updated = true;
    }

    return updated;
}

uint LoadMetricStats::get_Value() const
{
    ValidateValues();

    PLBConfig const& config = PLBConfig::GetConfig();

    if (config.LoadDecayFactor == 0.0 || count_ == 1)
    {
        return lastReportValue_;
    }
    else
    {
        if (weightedSum_ == 0.0)
        {
            return 0;
        }
        else
        {
            double value = weightedSum_ / sumOfWeight_;
            
            if (value > UINT_MAX)
            {
                return UINT_MAX;
            }
            else
            {
                return static_cast<uint>(value);
            }
        }
    }
}

void LoadMetricStats::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write("{0}/{1}/{2}/{3}/{4}/{5}", name_, lastReportValue_, lastReportTime_, count_, weightedSum_, sumOfWeight_);
}


void LoadMetricStats::ValidateValues() const
{
    ASSERT_IFNOT(count_ >= 1 && weightedSum_ >= 0.0 && sumOfWeight_ >= 1.0, "Invalid values: {0}", *this);

    if (count_ == 1)
    {
        ASSERT_IFNOT(sumOfWeight_ == 1.0 && weightedSum_ == lastReportValue_, "Invalid values {0}", *this);
    }
}
