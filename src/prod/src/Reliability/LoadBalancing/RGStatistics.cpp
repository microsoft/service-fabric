// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "RGStatistics.h"
#include "ServicePackageDescription.h"
#include "NodeDescription.h"
#include "ServiceDescription.h"
#include "Snapshot.h"
#include "PLBConfig.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

ResourceMetricStatistics::ResourceMetricStatistics()
    : resourceUsed_(0),
      resourceTotal_(0),
      minRequest_(0),
      maxRequest_(0)
{
}

void ResourceMetricStatistics::ProcessServicePackage(ServicePackageDescription const& sp, std::wstring const& metricName)
{
    auto const& itResource = sp.RequiredResources.find(metricName);
    if (itResource == sp.RequiredResources.end())
    {
        return;
    }
    // 0.0001 is minimum that we can govern for both CPU and memory.
    if (itResource->second > 0.0001)
    {
        if (minRequest_ < 0.0001 || itResource->second < minRequest_)
        {
            minRequest_ = itResource->second;
        }
        if (itResource->second > maxRequest_)
        {
            maxRequest_ = itResource->second;
        }
    }
}

void ResourceMetricStatistics::RemoveResourceTotal(uint64_t res)
{
    TESTASSERT_IF(resourceTotal_ < res,
        "RGStatistics: Trying to remove more resource ({1}) than available ({2})",
        res,
        resourceTotal_);
    resourceTotal_ -= res;
}

std::string ResourceMetricStatistics::AddField(Common::TraceEvent & traceEvent, std::string const & name)
{
    string format = "ResourceUsed={0}\r\nTotalResource={1}\r\nMinRequest={2}\r\nMaxRequest={3}";
    size_t index = 0;

    traceEvent.AddEventField<double>(format, name + ".resourceUsed", index);
    traceEvent.AddEventField<uint64_t>(format, name + ".totalResource", index);

    traceEvent.AddEventField<double>(format, name + ".minRequest", index);
    traceEvent.AddEventField<double>(format, name + ".maxRequest", index);

    return format;
}

void ResourceMetricStatistics::FillEventData(Common::TraceEventContext & context) const
{
    context.Write(resourceUsed_);
    context.Write(resourceTotal_);

    context.Write(minRequest_);
    context.Write(maxRequest_);
}

void ResourceMetricStatistics::WriteTo(Common::TextWriter & writer, Common::FormatOptions const &) const
{
    writer.WriteLine("ResourceUsed={0} TotalResource={1} MinRequest={2} MaxRequest={3}",
        resourceUsed_,
        resourceTotal_,
        minRequest_,
        maxRequest_);
}

RGStatistics::RGStatistics()
    : servicePackageCount_(0),
      governedServicePackageCount_(0),
      numSharedServices_(0),
      numExclusiveServices_(0),
      autoDetectEnabled_(true),
      cpuCoresStats_(),
      memoryInMBStats_()
{
}

void RGStatistics::AddServicePackage(ServicePackageDescription const& sp)
{
    ++servicePackageCount_;
    if (sp.IsGoverned)
    {
        ++governedServicePackageCount_;
        cpuCoresStats_.ProcessServicePackage(sp, *ServiceModel::Constants::SystemMetricNameCpuCores);
        memoryInMBStats_.ProcessServicePackage(sp, *ServiceModel::Constants::SystemMetricNameMemoryInMB);
    }
}

void RGStatistics::RemoveServicePackage(ServicePackageDescription const& sp)
{
    --servicePackageCount_;
    if (sp.IsGoverned)
    {
        --governedServicePackageCount_;
    }
}

void RGStatistics::UpdateServicePackage(ServicePackageDescription const& oldSp, ServicePackageDescription const& newSp)
{
    RemoveServicePackage(oldSp);
    AddServicePackage(newSp);
}

void RGStatistics::AddNode(NodeDescription const& node)
{
    if (node.IsUp)
    {
        for (auto const& capacity : node.Capacities)
        {
            if (capacity.first == *ServiceModel::Constants::SystemMetricNameCpuCores)
            {
                cpuCoresStats_.AddResourceTotal(capacity.second / ServiceModel::Constants::ResourceGovernanceCpuCorrectionFactor);
            }
            else if (capacity.first == *ServiceModel::Constants::SystemMetricNameMemoryInMB)
            {
                memoryInMBStats_.AddResourceTotal(capacity.second);
            }
        }
    }
}

void RGStatistics::RemoveNode(NodeDescription const& node)
{
    if (node.IsUp)
    {
        for (auto const& capacity : node.Capacities)
        {
            if (capacity.first == *ServiceModel::Constants::SystemMetricNameCpuCores)
            {
                cpuCoresStats_.RemoveResourceTotal(capacity.second / ServiceModel::Constants::ResourceGovernanceCpuCorrectionFactor);
            }
            else if (capacity.first == *ServiceModel::Constants::SystemMetricNameMemoryInMB)
            {
                memoryInMBStats_.RemoveResourceTotal(capacity.second);
            }
        }
    }
}

void RGStatistics::UpdateNode(NodeDescription const& node1, NodeDescription const& node2)
{
    RemoveNode(node1);
    AddNode(node2);
}

void RGStatistics::AddService(ServiceDescription const& serviceDescription)
{
    if (serviceDescription.ServicePackageActivationMode == ServiceModel::ServicePackageActivationMode::Enum::ExclusiveProcess)
    {
        ++numExclusiveServices_;
    }
    else if (serviceDescription.ServicePackageActivationMode == ServiceModel::ServicePackageActivationMode::Enum::SharedProcess)
    {
        ++numSharedServices_;
    }
    else
    {
        Common::Assert::TestAssert("Unknown service package activation mode found in RGStatistics. Description={0}", serviceDescription);
    }
}

void RGStatistics::DeleteService(ServiceDescription const& serviceDescription)
{
    if (serviceDescription.ServicePackageActivationMode == ServiceModel::ServicePackageActivationMode::Enum::ExclusiveProcess)
    {
        --numExclusiveServices_;
    }
    else if (serviceDescription.ServicePackageActivationMode == ServiceModel::ServicePackageActivationMode::Enum::SharedProcess)
    {
        --numSharedServices_;
    }
    else
    {
        Common::Assert::TestAssert("Unknown service package activation mode found in RGStatistics. Description={0}", serviceDescription);
    }
}

void RGStatistics::UpdateService(ServiceDescription const & service1, ServiceDescription const & service2)
{
    DeleteService(service1);
    AddService(service2);
}

void RGStatistics::Update(Snapshot const & snapshot)
{
    autoDetectEnabled_ = PLBConfig::GetConfig().AutoDetectAvailableResources;

    ServiceDomain::DomainData const* rgDomainData = snapshot.GetRGDomainData();
    if (nullptr != rgDomainData)
    {
        Placement const& pl = *(rgDomainData->State.PlacementObj);

        if (pl.LBDomains.size() > 0)
        {
            // Use global load balancing domain,
            auto& globalLBDomain = pl.LBDomains.back();
            for (auto const& metric : globalLBDomain.Metrics)
            {
                if (metric.Name == *ServiceModel::Constants::SystemMetricNameCpuCores)
                {
                    double totalLoad = static_cast<double>(metric.ClusterLoad) / ServiceModel::Constants::ResourceGovernanceCpuCorrectionFactor;
                    cpuCoresStats_.ResourceUsed = totalLoad;
                }
                else if (metric.Name == *ServiceModel::Constants::SystemMetricNameMemoryInMB)
                {
                    double totalLoad = static_cast<double>(metric.ClusterLoad);
                    memoryInMBStats_.ResourceUsed = totalLoad;
                }
            }
        }
    }
    else
    {
        // There is no capacity and no load for RG metrics.
        memoryInMBStats_.ResourceUsed = 0.0;
        cpuCoresStats_.ResourceUsed = 0.0;
    }
}

string RGStatistics::AddField(Common::TraceEvent & traceEvent, string const & name)
{
    string format = "ServicePackages={0}\r\nGovernedServicePackages={1}\r\nExclusiveServiceCount={2}\r\nSharedServiceCount={3}\r\nAutoDetectResources=({4})\r\nCpuCores=({5})\r\nMemoryInMB=({6})";
    size_t index = 0;

    traceEvent.AddEventField<uint64_t>(format, name + ".servicePackageCount", index);
    traceEvent.AddEventField<uint64_t>(format, name + ".governedServicePackageCount", index);

    traceEvent.AddEventField<uint64_t>(format, name + ".exclusiveServiceCount", index);
    traceEvent.AddEventField<uint64_t>(format, name + ".sharedServiceCount", index);

    traceEvent.AddEventField<bool>(format, name + ".autoDetectResources", index);

    traceEvent.AddEventField<ResourceMetricStatistics>(format, name + ".cpuCores", index);
    traceEvent.AddEventField<ResourceMetricStatistics>(format, name + ".memoryInMB", index);

    return format;
}

void RGStatistics::FillEventData(Common::TraceEventContext & context) const
{
    context.Write(servicePackageCount_);
    context.Write(governedServicePackageCount_);

    context.Write(numExclusiveServices_);
    context.Write(numSharedServices_);

    context.Write(autoDetectEnabled_);

    context.Write(cpuCoresStats_);
    context.Write(memoryInMBStats_);
}

void RGStatistics::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.WriteLine("ServicePackages={0}  GovernedServicePackages={1} ExclusiveServices={2} SharedServices={3} AutoDetectEnabled={4} CpuCores=({5})  MemoryInMB=({6})",
        servicePackageCount_,
        governedServicePackageCount_,
        numExclusiveServices_,
        numSharedServices_,
        autoDetectEnabled_,
        cpuCoresStats_,
        memoryInMBStats_);
}
