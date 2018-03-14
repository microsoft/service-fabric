// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;

INITIALIZE_SIZE_ESTIMATION(ServiceLoadMetricDescription)

ServiceLoadMetricDescription::ServiceLoadMetricDescription()
    : name_(), 
    weight_(FABRIC_SERVICE_LOAD_METRIC_WEIGHT_LOW), 
    primaryDefaultLoad_(0),
    secondaryDefaultLoad_(0)
{
}

ServiceLoadMetricDescription::ServiceLoadMetricDescription(
    std::wstring const& name, 
    FABRIC_SERVICE_LOAD_METRIC_WEIGHT weight, 
    uint primaryDefaultLoad,
    uint secondaryDefaultLoad)
    : name_(name), 
    weight_(weight),
    primaryDefaultLoad_(primaryDefaultLoad),
    secondaryDefaultLoad_(secondaryDefaultLoad)
{
}

ServiceLoadMetricDescription::ServiceLoadMetricDescription(ServiceLoadMetricDescription const& other)
    : name_(other.name_), 
    weight_(other.weight_),
    primaryDefaultLoad_(other.primaryDefaultLoad_),
    secondaryDefaultLoad_(other.secondaryDefaultLoad_)
{
}

ServiceLoadMetricDescription::ServiceLoadMetricDescription(ServiceLoadMetricDescription && other)
    : name_(std::move(other.name_)), 
    weight_(other.weight_),
    primaryDefaultLoad_(other.primaryDefaultLoad_),
    secondaryDefaultLoad_(other.secondaryDefaultLoad_)
{
}

ServiceLoadMetricDescription & ServiceLoadMetricDescription::operator = (ServiceLoadMetricDescription const& other)
{
    if (this != &other)
    {
        name_ = other.name_;
        weight_ = other.weight_;
        primaryDefaultLoad_ = other.primaryDefaultLoad_;
        secondaryDefaultLoad_ = other.secondaryDefaultLoad_;
    }

    return *this;
}

ServiceLoadMetricDescription & ServiceLoadMetricDescription::operator = (ServiceLoadMetricDescription && other)
{
    if (this != &other)
    {
        name_ = move(other.name_);
        weight_ = other.weight_;
        primaryDefaultLoad_ = other.primaryDefaultLoad_;
        secondaryDefaultLoad_ = other.secondaryDefaultLoad_;
    }

    return *this;
}

void ServiceLoadMetricDescription::WriteToEtw(uint16 contextSequenceId) const
{
    ServiceModel::ServiceModelEventSource::Trace->ServiceLoadMetricDescription(
        contextSequenceId,
        name_,
        static_cast<int>(weight_),
        primaryDefaultLoad_,
        secondaryDefaultLoad_);
}

void ServiceLoadMetricDescription::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    writer.Write("{0} {1} {2} {3}", name_, static_cast<uint>(weight_), primaryDefaultLoad_, secondaryDefaultLoad_);
}

bool ServiceLoadMetricDescription::operator == (ServiceLoadMetricDescription const & other) const
{
    return (name_ == other.name_ &&
            weight_ == other.weight_ &&
            primaryDefaultLoad_ == other.primaryDefaultLoad_ &&
            secondaryDefaultLoad_ == other.secondaryDefaultLoad_);
}

bool ServiceLoadMetricDescription::operator != (ServiceLoadMetricDescription const & other) const
{
    return !(*this == other);
}
