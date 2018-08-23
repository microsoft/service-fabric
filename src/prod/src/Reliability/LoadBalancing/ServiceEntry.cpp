// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "PartitionEntry.h"
#include "LoadBalancingDomainEntry.h"
#include "ServiceEntry.h"
#include "PlacementReplica.h"
#include "PlacementAndLoadBalancing.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

ServiceEntry::ServiceEntry(
    wstring && name,
    ServicePackageEntry const* servicePackage,
    bool isStateful,
    bool hasPersistedState,
    int targetReplicaSetSize,
    DynamicBitSet && blockList,
    DynamicBitSet && primaryBlockList,
    size_t serviceBlockListCount,
    vector<size_t> && globalMetricIndices,
    bool onEveryNode,
    Service::Type::Enum distribution,
    BalanceChecker::DomainTree && faultDomainStructure,
    BalanceChecker::DomainTree && upgradeDomainStructure,
    ServiceEntryDiagnosticsData && diagnosticsData,
    bool partiallyPlace,
    ApplicationEntry const* application,
    bool metricHasCapacity,
    bool autoSwitchToQuorumBasedLogic
    )
    : name_(move(name)),
    servicePackage_(servicePackage),
    isStateful_(isStateful),
    hasPersistedState_(hasPersistedState),
    targetReplicaSetSize_(targetReplicaSetSize),
    zero_(globalMetricIndices.size()),
    blockList_(move(blockList)),
    primaryReplicaBlockList_(move(primaryBlockList)),
    serviceBlockListCount_(serviceBlockListCount),
    replicaCount_(0),
    lbDomain_(nullptr),
    globalLBDomain_(nullptr),
    dependedService_(nullptr),
    alignedAffinity_(true),
    dependentServices_(),
    isBalanced_(false),
    globalMetricIndices_(move(globalMetricIndices)),
    onEveryNode_(onEveryNode),
    FDDistribution_(distribution),
    faultDomainStructure_(move(faultDomainStructure)),
    upgradeDomainStructure_(move(upgradeDomainStructure)),
    diagnosticsData_(move(diagnosticsData)),
    hasAffinity_(false),
    partiallyPlace_(partiallyPlace),
    application_(application),
    metricHasCapacity_(metricHasCapacity),
    hasInUpgradePartition_(false),
    alignedAffinityAssociatedPartitions_(),
    hasAffinityAssociatedSingletonReplicaInUpgrade_(false),
    autoSwitchToQuorumBasedLogic_(autoSwitchToQuorumBasedLogic)
{
}

ServiceEntry::ServiceEntry(ServiceEntry && other)
    : name_(move(other.name_)),
    servicePackage_(other.servicePackage_),
    isStateful_(other.isStateful_),
    hasPersistedState_(other.hasPersistedState_),
    targetReplicaSetSize_(other.targetReplicaSetSize_),
    zero_(move(other.zero_)),
    blockList_(move(other.blockList_)),
    primaryReplicaBlockList_(move(other.primaryReplicaBlockList_)),
    serviceBlockListCount_(other.serviceBlockListCount_),
    replicaCount_(other.replicaCount_),
    partitions_(move(other.partitions_)),
    lbDomain_(other.lbDomain_),
    globalLBDomain_(other.globalLBDomain_),
    dependedService_(other.dependedService_),
    alignedAffinity_(other.alignedAffinity_),
    dependentServices_(move(other.dependentServices_)),
    isBalanced_(other.isBalanced_),
    globalMetricIndices_(move(other.globalMetricIndices_)),
    onEveryNode_(other.onEveryNode_),
    FDDistribution_(other.FDDistribution_),
    faultDomainStructure_(move(other.faultDomainStructure_)),
    upgradeDomainStructure_(move(other.upgradeDomainStructure_)),
    diagnosticsData_(move(other.diagnosticsData_)),
    hasAffinity_(other.hasAffinity_),
    partiallyPlace_(other.partiallyPlace_),
    application_(other.application_),
    metricHasCapacity_(other.metricHasCapacity_),
    alignedAffinityAssociatedPartitions_(move(other.alignedAffinityAssociatedPartitions_)),
    hasInUpgradePartition_(other.hasInUpgradePartition_),
    hasAffinityAssociatedSingletonReplicaInUpgrade_(other.hasAffinityAssociatedSingletonReplicaInUpgrade_),
    autoSwitchToQuorumBasedLogic_(other.autoSwitchToQuorumBasedLogic_)
{
}

ServiceEntry & ServiceEntry::operator = (ServiceEntry && other)
{
    if (this != &other)
    {
        name_ = move(other.name_);
        servicePackage_ = other.servicePackage_;
        isStateful_ = other.isStateful_;
        hasPersistedState_ = other.hasPersistedState_;
        targetReplicaSetSize_ = other.targetReplicaSetSize_;
        zero_ = move(other.zero_);
        blockList_ = move(other.blockList_);
        primaryReplicaBlockList_ = move(other.primaryReplicaBlockList_);
        serviceBlockListCount_ = other.serviceBlockListCount_;
        replicaCount_ = other.replicaCount_;
        partitions_ = move(other.partitions_);
        lbDomain_ = other.lbDomain_;
        globalLBDomain_ = other.globalLBDomain_;
        dependedService_ = other.dependedService_;
        alignedAffinity_ = other.alignedAffinity_;
        dependentServices_ = move(other.dependentServices_);
        isBalanced_ = other.isBalanced_;
        globalMetricIndices_ = move(other.globalMetricIndices_);
        onEveryNode_ = other.onEveryNode_;
        FDDistribution_ = other.FDDistribution_;
        faultDomainStructure_ = move(other.faultDomainStructure_);
        upgradeDomainStructure_ = move(other.upgradeDomainStructure_);
        diagnosticsData_ = move(other.diagnosticsData_);
        hasAffinity_ = other.hasAffinity_;
        partiallyPlace_ = other.partiallyPlace_;
        application_ = other.application_;
        metricHasCapacity_ = other.metricHasCapacity_;
        alignedAffinityAssociatedPartitions_ = other.alignedAffinityAssociatedPartitions_;
        hasInUpgradePartition_ = other.hasInUpgradePartition_;
        hasAffinityAssociatedSingletonReplicaInUpgrade_ = other.hasAffinityAssociatedSingletonReplicaInUpgrade_;
        autoSwitchToQuorumBasedLogic_ = other.autoSwitchToQuorumBasedLogic_;
    }

    return *this;
}

void ServiceEntry::SetLBDomain(LoadBalancingDomainEntry const* lbDomain, LoadBalancingDomainEntry const* globalLBDomain)
{
    if (lbDomain != nullptr)
    {
        ASSERT_IF(globalMetricIndices_.size() != lbDomain->MetricCount,
            "globalMetricIndices_ size and lbDomain metric count does not match: {0} {1}",
            globalMetricIndices_.size(),
            lbDomain->MetricCount);
        lbDomain_ = lbDomain;
    }

    ASSERT_IF(globalLBDomain == nullptr, "Global LB domains should not be null");
    globalLBDomain_ = globalLBDomain;
}

void ServiceEntry::SetDependedService(ServiceEntry const* service, bool alignedAffinity)
{
    dependedService_ = service;
    alignedAffinity_ = alignedAffinity;

    hasAffinity_ = true;
}

void ServiceEntry::AddDependentService(ServiceEntry const* service)
{
    dependentServices_.push_back(service);
}

void ServiceEntry::AddPartition(PartitionEntry const* partition)
{
    partitions_.push_back(partition);
}

void ServiceEntry::RefreshIsBalanced()
{
    // determine whether the service's local metrics are balanced
    isBalanced_ = (lbDomain_ == nullptr ? true : lbDomain_->IsBalanced); // assume lbDomain_.IsBalanced is already refreshed

    // Determine whether all of the service's global metrics are balanced in the global domain.
    // Note: in certain cases even if all the global metrics are balanced, moving this service could result in a more optimal solution
    // (because we could have another unbalanced local domain which shares the same global metric, and in an effort to balance that local domain,
    // we affect the global domain and thus dictating moving this service).
    // However we consider this situation rare and even if it happens we just obtain a less optimal solution and we prefer not moving services.
    if (isBalanced_)
    {
        size_t globalStartIndex = globalLBDomain_->MetricStartIndex;
        for (size_t i = 0; i < MetricCount; i++)
        {
            size_t globalMetricIndex = globalMetricIndices_[i];
            ASSERT_IFNOT(globalMetricIndex >= globalStartIndex, "Global metric index invalid {0} {1}", globalMetricIndex, globalStartIndex);

            size_t metricIndex = globalMetricIndex - globalStartIndex;
            ASSERT_IFNOT(metricIndex < globalLBDomain_->Metrics.size(), "Metric index invalid {0} {1}", metricIndex, globalLBDomain_->Metrics.size());
            if (!globalLBDomain_->Metrics[metricIndex].IsBalanced)
            {
                isBalanced_ = false;
                break;
            }
        }
    }
}

void ServiceEntry::AddAlignedAffinityPartitions()
{
    alignedAffinityAssociatedPartitions_.clear();
    // First add partition of parent service
    for (auto pIt = partitions_.begin(); pIt != partitions_.end(); ++pIt)
    {
        alignedAffinityAssociatedPartitions_.push_back(*pIt);
    }

    // Has children services
    for (auto serviceIt = DependentServices.begin(); serviceIt != DependentServices.end(); ++serviceIt)
    {
        auto childService = *serviceIt;

        if (childService->AlignedAffinity && childService->IsStateful)
        {
            for (auto cIt = childService->Partitions.begin(); cIt != childService->Partitions.end(); ++cIt)
            {
                alignedAffinityAssociatedPartitions_.push_back(*cIt);
            }
        }
    }

}

std::vector<PartitionEntry const*> const& ServiceEntry::AffinityAssociatedPartitions() const
{
    if (DependedService != nullptr)
    {
        // Has parent
        return DependedService->AffinityAssociatedPartitions();
    }
    else
    {
        return alignedAffinityAssociatedPartitions_;
    }
}

std::vector<PlacementReplica const*> ServiceEntry::AllAffinityAssociatedReplicasWithTarget1() const
{
    if (DependedService != nullptr)
    {
        // Return the ones from parent
        return DependedService->AllAffinityAssociatedReplicasWithTarget1();
    }

    std::vector<PlacementReplica const*> result;
    PlacementReplica const * replica = nullptr;

    // First insert partitions of the parent
    if (partitions_.size() > 1)
    {
        Common::Assert::TestAssert("Parent service {0} has more than one partition", name_);
        return result;
    }

    for (auto partition: partitions_)
    {
        // If partition has more replicas or it doesn't have any,
        // there are no candidates to be moved with the optimization
        if (partition->TargetReplicaSetSize != 1 || partition->ExistingReplicaCount == 0)
        {
            continue;
        }
        replica = partition->FindMovableReplicaForSingletonReplicaUpgrade();
        if (replica != nullptr)
        {
            result.push_back(replica);
        }
        else
        {
            result.clear();
            return result;
        }
    }

    if (0 == result.size())
    {
        return result;
    }

    // Has children services
    for (auto childService : DependentServices)
    {
        for (auto childPartition : childService->Partitions)
        {
            // If partition doesn't have any replicas,
            // there are no candidates to move (e.g. partition is new)
            if (childPartition->ExistingReplicaCount == 0 || childPartition->TargetReplicaSetSize != 1)
            {
                continue;
            }
            replica = childPartition->FindMovableReplicaForSingletonReplicaUpgrade();
            if (replica != nullptr)
            {
                result.push_back(replica);
            }
            else
            {
                result.clear();
                return result;
            }
        }
    }

    return result;
}

GlobalWString const ServiceEntry::TraceDescription = make_global<wstring>(L"[Services]\r\n#serviceName stateful persisted targetReplicaSetSize "
    "applicationName globalMetricIndices blockList primaryBlockList dependedService onEveryNode "
    "fdDistribution isBalanced partiallyPlace hasInUpgradePartition AutoSwitchToQuorumBasedLogic");

void ServiceEntry::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    wstring appName = L"";
    if (application_)
    {
        appName = application_->Name;
    }
    writer.Write("{0} {1} {2} {3} {4} {5} (",
        name_, isStateful_, hasPersistedState_, targetReplicaSetSize_, appName, globalMetricIndices_);

    bool first = true;
    blockList_.ForEach([&first, &writer](size_t nodeIndex) -> void
    {
        if (!first)
        {
            writer.Write(" ");
        }
        writer.Write("{0}", nodeIndex);
        first = false;
    });
    writer.Write(") (");

    first = true;
    primaryReplicaBlockList_.ForEach([&first, &writer](size_t nodeIndex) -> void
    {
        if (!first)
        {
            writer.Write(" ");
        }
        writer.Write("{0}", nodeIndex);
        first = false;
    });
    writer.Write(")");

    if (dependedService_ != nullptr)
    {
        writer.Write(" {0}", dependedService_->Name);
    }
    else
    {
        writer.Write(" -");
    }


    writer.Write(" {0} {1} {2} {3} {4} {5}", onEveryNode_, FDDistribution_, isBalanced_, partiallyPlace_, hasInUpgradePartition_, autoSwitchToQuorumBasedLogic_);
}

void ServiceEntry::WriteToEtw(uint16 contextSequenceId) const
{
    PlacementAndLoadBalancing::PLBTrace->PLBDump(contextSequenceId, wformatString(*this));
}
