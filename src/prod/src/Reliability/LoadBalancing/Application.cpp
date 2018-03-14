// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Application.h" 

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

GlobalWString const Application::FormatHeader = make_global<wstring>(L"ApplicationName");

Application::Application(ApplicationDescription && desc)
: applicationDesc_(move(desc))
{
}

Application::Application(Application && other)
: applicationDesc_(move(other.applicationDesc_))
{
}

bool Application::UpdateDescription(ApplicationDescription && desc)
{
    if (applicationDesc_ != desc)
    {
        applicationDesc_ = move(desc);
        return true;
    }
    else
    {
        return false;
    }
}

void Application::AddService(std::wstring const& serviceName)
{
    services_.insert(serviceName);
}

size_t Application::RemoveService(std::wstring const& serviceName)
{
    return services_.erase(serviceName);
}

int64 Application::GetReservedCapacity(std::wstring const& metricName) const
{
    int64 totalReservedCapacity(0);
    auto capIt = applicationDesc_.AppCapacities.find(metricName);
    if (capIt != applicationDesc_.AppCapacities.end())
    {
        totalReservedCapacity = capIt->second.ReservationCapacity * applicationDesc_.MinimumNodes;
    }

    return totalReservedCapacity;
}

void Application::GetChangedServicePackages(ApplicationDescription const & newDescription, std::set<ServiceModel::ServicePackageIdentifier>& deletedSPs, std::set<ServiceModel::ServicePackageIdentifier>& updatedSPs)
{
    auto const& newSPDescriptions = newDescription.ServicePackages;
    auto const& oldSPDescriptions = ApplicationDesc.ServicePackages;

    for (auto & oldSp : oldSPDescriptions)
    {
        if (newSPDescriptions.find(oldSp.first) == newSPDescriptions.end())
        {
            //Package is removed
            deletedSPs.insert(oldSp.first);
        }
    }

    for (auto & newSp : newSPDescriptions)
    {
        auto const & matchingOldSp = oldSPDescriptions.find(newSp.first);
        if (matchingOldSp == oldSPDescriptions.end())
        {
            //This package is new
            updatedSPs.insert(newSp.first);
        }
        else if (newSp.second != matchingOldSp->second)
        {
            //RG policy has changed
            updatedSPs.insert(newSp.first);
        }
    }
}

void Application::AddServicePackage(ServicePackageDescription const & servicePackage)
{
    applicationDesc_.servicePackages_.insert(std::pair<ServiceModel::ServicePackageIdentifier, ServicePackageDescription>(servicePackage.ServicePackageIdentifier, servicePackage));
}

void Application::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    writer.Write("{0}", applicationDesc_.Name);
}
