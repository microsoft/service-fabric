// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ServicePackageDescription.h"
#include "PlacementAndLoadBalancing.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

GlobalWString const ServicePackageDescription::FormatHeader = make_global<wstring>(L"Service package descriptions");

ServicePackageDescription::ServicePackageDescription(
    ServiceModel::ServicePackageIdentifier  && servicePackageIdentifier,
    std::map<std::wstring, double> && requiredResources)
    : servicePackageIdentifier_(move(servicePackageIdentifier)),
    requiredResources_(move(requiredResources))
{
    CorrectCpuMetric();
}

ServicePackageDescription::ServicePackageDescription(ServicePackageDescription const& other)
    : servicePackageIdentifier_(other.servicePackageIdentifier_),
    requiredResources_(other.requiredResources_)
{
    CorrectCpuMetric();
}

ServicePackageDescription::ServicePackageDescription(ServicePackageDescription && other)
    : servicePackageIdentifier_(move(other.servicePackageIdentifier_)),
    requiredResources_(move(other.requiredResources_))
{
    CorrectCpuMetric();
}

Reliability::LoadBalancingComponent::ServicePackageDescription::ServicePackageDescription(ServicePackageDescription const & sp1, ServicePackageDescription const & sp2)
    : servicePackageIdentifier_(sp1.servicePackageIdentifier_),
    requiredResources_(sp1.requiredResources_)
{
    TESTASSERT_IFNOT(sp1.servicePackageIdentifier_ == sp2.servicePackageIdentifier_,
        "Illegal merge of two service packages {0} and {1}",
        sp1,
        sp2);
    // Used during upgrade: resulting SP will have maximum of resources.
    for (auto const& resource : sp2.requiredResources_)
    {
        auto sp1ResourceIt = requiredResources_.find(resource.first);
        if (sp1ResourceIt == requiredResources_.end())
        {
            requiredResources_.insert(make_pair(resource.first, resource.second));
        }
        else if (sp1ResourceIt->second < resource.second)
        {
            sp1ResourceIt->second = resource.second;
        }
    }
    CorrectCpuMetric();
}

ServicePackageDescription & ServicePackageDescription::operator = (ServicePackageDescription && other)
{
    if (this != &other)
    {
        servicePackageIdentifier_ = move(other.servicePackageIdentifier_);
        requiredResources_ = move(other.requiredResources_);
        CorrectCpuMetric();
    }
    return *this;
}

bool ServicePackageDescription::operator == (ServicePackageDescription const& other) const
{
    return servicePackageIdentifier_ == other.servicePackageIdentifier_ &&
        requiredResources_ == other.requiredResources_;
}

bool ServicePackageDescription::operator != (ServicePackageDescription const& other) const
{
    return !(*this == other);
}

uint ServicePackageDescription::GetMetricRgReservation(std::wstring const & metricName) const
{
    auto itResource = correctedRequiredResources_.find(metricName);
    if (itResource != correctedRequiredResources_.end())
    {
        return itResource->second;
    }
    return 0;
}

bool ServicePackageDescription::IsAnyResourceReservationIncreased(ServicePackageDescription const & other)
{
    TESTASSERT_IF(servicePackageIdentifier_ != other.servicePackageIdentifier_, "It is not allowed to compare resources for two different service packages.");

    for (auto const& upgradingResource : other.requiredResources_)
    {
        auto currentResource = requiredResources_.find(upgradingResource.first);
        if (currentResource != requiredResources_.end())
        {
            if (upgradingResource.second > currentResource->second)
            {
                return true;
            }
        }
        //this resource was not used before the upgrade at all
        else
        {
            return true;
        }
    }

    return false;
}

void ServicePackageDescription::WriteTo(TextWriter& writer, FormatOptions const &) const
{
    writer.Write("{0} {1}", servicePackageIdentifier_, requiredResources_);
}

void ServicePackageDescription::WriteToEtw(uint16 contextSequenceId) const
{
    PlacementAndLoadBalancing::PLBTrace->PLBDump(contextSequenceId, wformatString(*this));
}

void ServicePackageDescription::CorrectCpuMetric()
{
    correctedRequiredResources_.clear();
    for (auto reqResource : requiredResources_)
    {
        uint value;
        if (reqResource.first == ServiceModel::Constants::SystemMetricNameCpuCores)
        {
            value = static_cast<uint>(ceil(reqResource.second * ServiceModel::Constants::ResourceGovernanceCpuCorrectionFactor));
        }
        else
        {
            value = static_cast<uint>(reqResource.second);
        }
        correctedRequiredResources_.insert(make_pair(reqResource.first, value));
    }
}

