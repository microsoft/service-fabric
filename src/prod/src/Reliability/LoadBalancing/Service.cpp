// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Constants.h"
#include "Service.h"
#include "FailoverUnit.h"
#include "PlacementAndLoadBalancing.h"
#include "ReplicaDescription.h"
#include "SearcherSettings.h"
#include "ServiceDomain.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

void Reliability::LoadBalancingComponent::WriteToTextWriter(Common::TextWriter & w, Reliability::LoadBalancingComponent::Service::Type::Enum const & val)
{
    switch (val)
    {
        case Service::Type::Packing: 
            w << "Packing"; return;
        case Service::Type::Nonpacking: 
            w << "Nonpacking"; return;
        case Service::Type::Ignore: 
            w << "Ignore"; return;
        default: 
            Common::Assert::CodingError("Unknown FDPolicy Type");
    }
}

Service::Service(ServiceDescription && desc)
    : serviceDesc_(move(desc)),
    actualPartitionCount_(0),
    failoverUnitId_(),
    failoverUnits_(),
    nonEmptyPartitionCount_(0),
    isSingleton_(true), // by default assume a service to be singleton
    metricTable_(),
    shouldDisappearMetricTable_(),
    serviceBlockList_(),
    overallBlockList_(),
    primaryReplicaBlockList_(),
    FDDistribution_(Type::Packing), // By default, FD disbution is packing
    partiallyPlace_(PLBConfig::GetConfig().PartiallyPlaceServices)
{
}

Service::Service(Service && other)
    : serviceDesc_(move(other.serviceDesc_)),
    actualPartitionCount_(other.actualPartitionCount_),
    failoverUnitId_(other.failoverUnitId_),
    failoverUnits_(other.failoverUnits_),
    nonEmptyPartitionCount_(other.nonEmptyPartitionCount_),
    isSingleton_(other.isSingleton_),
    metricTable_(move(other.metricTable_)),
    shouldDisappearMetricTable_(move(other.shouldDisappearMetricTable_)),
    serviceBlockList_(move(other.serviceBlockList_)),
    overallBlockList_(move(other.overallBlockList_)),
    primaryReplicaBlockList_(move(other.primaryReplicaBlockList_)),
    FDDistribution_(move(other.FDDistribution_)),
    partiallyPlace_(move(other.partiallyPlace_))
{
}

void Service::OnFailoverUnitAdded(FailoverUnit const& failoverUnit, GuidUnorderedMap<FailoverUnit> const& existingFUTable, SearcherSettings const & settings)
{
    ++actualPartitionCount_;

    if (actualPartitionCount_ == 1)
    {
        failoverUnitId_ = failoverUnit.FUId;
    }
    else if (actualPartitionCount_ == 2 && isSingleton_)
    {
        isSingleton_ = false;

        // we need to record the loads for its first partition
        auto itFuFirst = existingFUTable.find(failoverUnitId_);
        ASSERT_IF(itFuFirst == existingFUTable.end(), "The first FailoverUnit {0} must exist in existingFUTable", failoverUnitId_);
        AddNodeLoad(itFuFirst->second, itFuFirst->second.FuDescription.Replicas, settings);
    }

    failoverUnits_.insert(failoverUnit.FUId);

    AddNodeLoad(failoverUnit, failoverUnit.FuDescription.Replicas, settings);

    if (!failoverUnit.IsEmpty)
    {
        ++nonEmptyPartitionCount_;
    }
}

void Service::OnFailoverUnitChanged(FailoverUnit const& oldFU, FailoverUnitDescription const& newFU)
{
    bool newIsEmpty = newFU.Replicas.empty() && newFU.ReplicaDifference == 0;

    if (oldFU.IsEmpty != newIsEmpty)
    {
        if (oldFU.IsEmpty)
        {
            ++nonEmptyPartitionCount_;
        }
        else
        {
            ASSERT_IFNOT(nonEmptyPartitionCount_ > 0, "Service {0} nonEmptyPartitionCount is 0 when changing FailoverUnit {1}", serviceDesc_.Name, newFU.FUId);
            --nonEmptyPartitionCount_;
        }
    }

}

void Service::OnFailoverUnitDeleted(FailoverUnit const& failoverUnit, SearcherSettings const & settings)
{
    ASSERT_IFNOT(actualPartitionCount_ > 0, "Service {0} actualPartitionCount is 0 when deleting FailoverUnit {1}", serviceDesc_.Name, failoverUnit.FUId);
    --actualPartitionCount_;

    // TODO: update the isSingleton_ here when we have the dynamic partitioning

    if (!failoverUnit.IsEmpty)
    {
        ASSERT_IFNOT(nonEmptyPartitionCount_ > 0, "Service {0} nonEmptyPartitionCount is 0 when deleting FailoverUnit {1}", serviceDesc_.Name, failoverUnit.FUId);
        --nonEmptyPartitionCount_;
    }

    failoverUnits_.erase(failoverUnit.FUId);

    DeleteNodeLoad(failoverUnit, failoverUnit.FuDescription.Replicas, settings);
}

vector<uint> Service::GetDefaultLoads(ReplicaRole::Enum role) const
{
    vector<uint> loads;
    loads.reserve(serviceDesc_.Metrics.size());
    ASSERT_IF(role == ReplicaRole::None, "Invalid role for getting loads");

    for (auto i = 0; i < serviceDesc_.Metrics.size(); ++i)
    {
        loads.push_back(GetDefaultMetricLoad(i, role));
    }

    return loads;
}

uint Service::GetDefaultMetricLoad(uint metricIndex, ReplicaRole::Enum role) const
{
    bool isPrimary = (role == ReplicaRole::Primary);
    bool isStateful = serviceDesc_.IsStateful;

    if (metricIndex >= serviceDesc_.Metrics.size())
    {
        TESTASSERT_IF(metricIndex >= serviceDesc_.Metrics.size(), "Metric index {0} is out of bounds", metricIndex);
        return 0;
    }

    ServiceMetric const& metric = serviceDesc_.Metrics[metricIndex];

    if (metric.IsPrimaryCount)
    {
        return (isPrimary ? (isStateful ? 1 : 0) : 0);
    }
    else if (metric.IsSecondaryCount)
    {
        return (isPrimary ? 0 : (isStateful ? 1 : 0));
    }
    else if (metric.IsReplicaCount)
    {
        return (isStateful ? 1 : 0);
    }
    else if (metric.IsInstanceCount)
    {
        return (isPrimary ? 0 : (isStateful ? 0 : 1));
    }
    else if (metric.IsCount)
    {
        return 1;
    }
    else
    {
        return (isPrimary ? (isStateful ? metric.PrimaryDefaultLoad : 0) : (isStateful ? metric.SecondaryDefaultLoad : metric.InstanceDefaultLoad));
    }
}

bool Service::IsLoadAvailableForPartitions(std::wstring const & metricName, ServiceDomain const & serviceDomain)
{
    for (auto ftId : failoverUnits_)
    {
        auto itFT = serviceDomain.FailoverUnits.find(ftId);
        if (itFT != serviceDomain.FailoverUnits.end())
        {
            if (itFT->second.IsLoadAvailable(metricName))
            {
                return true;
            }
        }
    }
    return false;
}

bool Service::GetAverageLoadPerPartitions(std::wstring const & metricName, ServiceDomain const & serviceDomain, bool useOnlyPrimaryLoad, double & averageLoad) const
{
    // Get the average load for partitions where load is available.
    bool isResource = metricName == *ServiceModel::Constants::SystemMetricNameCpuCores || metricName == *ServiceModel::Constants::SystemMetricNameMemoryInMB;
    int metricIndex = -1;
    double loadSum = 0.0;
    size_t count = 0;

    if (!isResource)
    {
        metricIndex = GetCustomLoadIndex(metricName);
        if (metricIndex < 0)
        {
            return false;
        }
    }

    for (auto ftId : failoverUnits_)
    {
        auto itFT = serviceDomain.FailoverUnits.find(ftId);
        if (itFT != serviceDomain.FailoverUnits.end() && itFT->second.IsLoadAvailable(metricName))
        {
            count++;
            if (serviceDesc_.IsStateful)
            {
                if (useOnlyPrimaryLoad) 
                {
                    if (isResource)
                    {
                        loadSum += itFT->second.GetResourcePrimaryLoad(metricName);
                    }
                    else
                    {
                        loadSum += itFT->second.PrimaryEntries[metricIndex];
                    }
                }
                else
                {
                    if (isResource)
                    {
                        loadSum += itFT->second.GetResourceAverageLoad(metricName);
                    }
                    else
                    {
                        //Assumption primary replica exist always
                        loadSum += itFT->second.GetAverageLoadForAutoScaling(metricIndex);
                    }
                }
            }
            else
            {
                if (isResource)
                {
                    loadSum += itFT->second.GetResourceAverageLoad(metricName);
                }
                else
                {
                    loadSum += itFT->second.GetSecondaryLoadForAutoScaling(metricIndex);
                }
            }
        }
    }

    if (count > 0)
    {
        averageLoad = loadSum / count;
    }
    else
    {
        averageLoad = 0.0;
    }
    return true;
}

uint Service::GetDefaultMoveCost(ReplicaRole::Enum role) const
{
    UNREFERENCED_PARAMETER(role);
    return serviceDesc_.DefaultMoveCost;
}

int Service::GetCustomLoadIndex(std::wstring const& name) const
{
    int ret = -1;

    for (size_t index = 0; index < serviceDesc_.Metrics.size(); ++index)
    {
        if (!serviceDesc_.Metrics[index].IsBuiltIn && serviceDesc_.Metrics[index].Name == name)
        {
            ret = static_cast<int>(index);
            break;
        }
    }

    return ret;
}

bool Service::ContainsMetric(std::wstring const& name) const
{
    bool ret = false;

    for (size_t index = 0; index < serviceDesc_.Metrics.size(); ++index)
    {
        if (serviceDesc_.Metrics[index].Name == name)
        {
            ret = true;
            break;
        }
    }

    return ret;
}

void Service::AddNodeLoad(FailoverUnit const& failoverUnit, vector<ReplicaDescription> const& replicas, SearcherSettings const & settings)
{
    if (replicas.empty() || isSingleton_)
    {
        return;
    }

    auto &metrics = serviceDesc_.Metrics;

    if (metricTable_.empty())
    {
        metricTable_.resize(metrics.size());
    }

    if (shouldDisappearMetricTable_.empty())
    {
        shouldDisappearMetricTable_.resize(metrics.size());
    }

    for (size_t metricIndex = 0; metricIndex < metrics.size(); ++metricIndex)
    {
        for (auto itReplica = replicas.begin(); itReplica != replicas.end(); ++itReplica)
        {
            if (itReplica->UsePrimaryLoad())
            {
                if (itReplica->ShouldDisappear)
                {
                    shouldDisappearMetricTable_[metricIndex].AddLoad(itReplica->NodeId, failoverUnit.PrimaryEntries[metricIndex]);
                }
                else
                {
                    metricTable_[metricIndex].AddLoad(itReplica->NodeId, failoverUnit.PrimaryEntries[metricIndex]);
                }
            }
            else if (itReplica->UseSecondaryLoad())
            {
                if (itReplica->ShouldDisappear)
                {
                    shouldDisappearMetricTable_[metricIndex].AddLoad(
                        itReplica->NodeId, failoverUnit.GetSecondaryLoad(metricIndex, itReplica->NodeId, settings));
                }
                else
                {
                    metricTable_[metricIndex].AddLoad(itReplica->NodeId, failoverUnit.GetSecondaryLoad(metricIndex, itReplica->NodeId, settings));
                }
            }
        }
    }
}

void Service::DeleteNodeLoad(FailoverUnit const& failoverUnit, vector<ReplicaDescription> const& replicas, SearcherSettings const & settings)
{
    if (replicas.empty() || isSingleton_)
    {
        return;
    }

    auto &metrics = serviceDesc_.Metrics;

    if (metricTable_.empty())
    {
        metricTable_.resize(metrics.size());
    }

    if (shouldDisappearMetricTable_.empty())
    {
        shouldDisappearMetricTable_.resize(metrics.size());
    }

    for (size_t metricIndex = 0; metricIndex < metrics.size(); ++metricIndex)
    {
        for (auto itReplica = replicas.begin(); itReplica != replicas.end(); ++itReplica)
        {
            if (itReplica->UsePrimaryLoad())
            {
                if (itReplica->ShouldDisappear)
                {
                    shouldDisappearMetricTable_[metricIndex].DeleteLoad(itReplica->NodeId, failoverUnit.PrimaryEntries[metricIndex]);
                }
                else
                {
                    metricTable_[metricIndex].DeleteLoad(itReplica->NodeId, failoverUnit.PrimaryEntries[metricIndex]);
                }
            }
            else if (itReplica->UseSecondaryLoad())
            {
                if (itReplica->ShouldDisappear)
                {
                    shouldDisappearMetricTable_[metricIndex].DeleteLoad(
                        itReplica->NodeId, failoverUnit.GetSecondaryLoad(metricIndex, itReplica->NodeId, settings));
                }
                else
                {
                    metricTable_[metricIndex].DeleteLoad(itReplica->NodeId, failoverUnit.GetSecondaryLoad(metricIndex, itReplica->NodeId, settings));
                }
            }
        }
    }
}

LoadEntry Service::GetNodeLoad(Federation::NodeId nodeId, bool shouldDisappear) const
{
    std::vector<ServiceDomainLocalMetric> const* metricTable = shouldDisappear ? &shouldDisappearMetricTable_ : &metricTable_;

    size_t metricCount = serviceDesc_.Metrics.size();
    LoadEntry loads(metricCount, 0);
    if (!metricTable->empty())
    {
        for (size_t metricIndex = 0; metricIndex < metricCount; ++metricIndex)
        {
            loads.Set(metricIndex, metricTable->at(metricIndex).GetLoad(nodeId));
        }
    }
    return loads;
}

void Service::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    writer.Write("{0} {1} {2} {3}", serviceDesc_, actualPartitionCount_, nonEmptyPartitionCount_, overallBlockList_);
}

void Service::VerifyMetricLoads(map<wstring, ServiceDomainLocalMetric> const& baseLoads, bool includeDisappearingLoad) const
{
    std::vector<ServiceDomainLocalMetric> const& table = includeDisappearingLoad ? shouldDisappearMetricTable_ : metricTable_;

    if (table.size() != baseLoads.size())
    {
        // this happens when this service hasn't received any loads
        ASSERT_IF(!table.empty(), "{0} should be empty when its size does not match that of baseLoads: {1} {2}",
            includeDisappearingLoad ? "shouldDisappearMetricTable_" : "metricTable_", table.size(), baseLoads.size());

        for (auto it = baseLoads.begin(); it != baseLoads.end(); ++it)
        {
            it->second.VerifyAreZero();
        }
        return;
    }

    auto &metrics = serviceDesc_.Metrics;
    for (size_t i = 0; i < metrics.size(); i++)
    {
        if (!metrics[i].IsRGMetric)
        {
            auto itMetric = baseLoads.find(metrics[i].Name);
            ASSERT_IF(itMetric == baseLoads.end(), "{0} for metric {1} does not exist in baseLoads during verification",
                includeDisappearingLoad ? "Disappear load" : "Load", metrics[i].Name);
            table[i].VerifyLoads(itMetric->second);
        }
    }
}

void Service::SetFDDistributionPolicy(wstring const& distPolicy)
{
    if (distPolicy.front() == L'P' || distPolicy.front() == L'p')
    {
        this->FDDistribution_ = Type::Packing;
    }
    else if (distPolicy.front() == L'N' || distPolicy.front() == L'n')
    {
        this->FDDistribution_ = Type::Nonpacking;
    }
    else if (distPolicy.front() == L'I' || distPolicy.front() == L'i')
    {
        this->FDDistribution_ = Type::Ignore;
    }
    else
    {
        //todo trace for invalid policy input
    }
}

void Service::SetPlaceDistributionPolicy(std::wstring const& distPolicy)
{
    if (distPolicy.front() == L'N' || distPolicy.front() == L'n')
    {
        this->partiallyPlace_ = false;
    }
    else
    {
        this->partiallyPlace_ = true;
    }
}
