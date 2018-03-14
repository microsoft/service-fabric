// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ServiceMetric.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

GlobalWString ServiceMetric::PrimaryCountName = make_global<wstring>(L"PrimaryCount");
GlobalWString ServiceMetric::SecondaryCountName = make_global<wstring>(L"SecondaryCount");
GlobalWString ServiceMetric::ReplicaCountName = make_global<wstring>(L"ReplicaCount");
GlobalWString ServiceMetric::InstanceCountName = make_global<wstring>(L"InstanceCount");
GlobalWString ServiceMetric::CountName = make_global<wstring>(L"Count");

ServiceMetric::ServiceMetric(wstring && name, double weight, uint primaryDefaultLoad, uint secondaryDefaultLoad, bool isRGMetric)
    : name_(move(name)), 
      builtInType_(ParseName(name_)), 
      weight_(weight), 
      primaryDefaultLoad_(primaryDefaultLoad), 
      secondaryDefaultLoad_(secondaryDefaultLoad),
      isRGMetric_(isRGMetric)
{
    ASSERT_IF(name_.empty(), "Service metric name should not be empty");
    ASSERT_IFNOT(weight_ >= 0.0, "Weight should not be less than 0");
    // TODO: assert that built in metrics don't have a default load and validate this in API
}

ServiceMetric::ServiceMetric(ServiceMetric const & other)
    : name_(other.name_), 
    builtInType_(other.builtInType_), 
    weight_(other.weight_), 
    primaryDefaultLoad_(other.primaryDefaultLoad_),
    secondaryDefaultLoad_(other.secondaryDefaultLoad_),
    isRGMetric_(other.isRGMetric_)
{
}

ServiceMetric::ServiceMetric(ServiceMetric && other)
    : name_(move(other.name_)), 
      builtInType_(other.builtInType_), 
      weight_(other.weight_), 
      primaryDefaultLoad_(other.primaryDefaultLoad_),
      secondaryDefaultLoad_(other.secondaryDefaultLoad_),
      isRGMetric_(other.isRGMetric_)
{
}

ServiceMetric & ServiceMetric::operator = (ServiceMetric && other)
{
    if (this != &other)
    {
        name_ = move(other.name_);
        builtInType_ = other.builtInType_;
        weight_ = other.weight_;
        primaryDefaultLoad_ = other.primaryDefaultLoad_;
        secondaryDefaultLoad_ = other.secondaryDefaultLoad_;
        isRGMetric_ = other.isRGMetric_;
    }

    return *this;
}

bool ServiceMetric::operator == (ServiceMetric const & other ) const
{
    return (
        name_ == other.name_ &&
        weight_ == other.weight_ &&
        primaryDefaultLoad_ == other.primaryDefaultLoad_ &&
        secondaryDefaultLoad_ == other.secondaryDefaultLoad_ &&
        isRGMetric_ == other.isRGMetric_);
}

bool ServiceMetric::operator != (ServiceMetric const & other ) const
{
    return !(*this == other);
}

ServiceMetric::BuiltInType ServiceMetric::ParseName(wstring const& name)
{
    if (name == PrimaryCountName)
    {
        return PrimaryCount;
    }
    else if (name == SecondaryCountName)
    {
        return SecondaryCount;
    }
    else if (name == ReplicaCountName)
    {
        return ReplicaCount;
    }
    else if (name == InstanceCountName)
    {
        return InstanceCount;
    }
    else if (name == CountName)
    {
        return Count;
    }
    else
    {
        return None;
    }
}

bool ServiceMetric::IsMetricOfDefaultServices() const
{
    for (auto serviceName : Constants::DefaultServicesNameList)
    {
        wstring const & temp = serviceName;
        if (name_.find(temp) != -1)
        {
            return true;
        }
    }
    return false;
}

void ServiceMetric::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    writer.Write("{0}/{1}/{2}/{3}", name_, weight_, primaryDefaultLoad_, secondaryDefaultLoad_);
}
