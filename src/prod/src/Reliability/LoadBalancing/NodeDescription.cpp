// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "NodeDescription.h"
#include "PlacementAndLoadBalancing.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

GlobalWString const NodeDescription::FormatHeader = make_global<wstring>(L"NodeId:NodeInstance (NodeProperties) FaultDomainId UpgradeDomainId State DeactivationIntent DeactivationStatus CapacityRatios Capacities");

NodeDescription::NodeDescription(
    Federation::NodeInstance nodeInstance, 
    bool isUp, 
    Reliability::NodeDeactivationIntent::Enum deactivationIntent,
    Reliability::NodeDeactivationStatus::Enum deactivationStatus,
    map<wstring, wstring> && nodeProperties,
    DomainId && faultDomainId,
    wstring && upgradeDomainId,
    map<wstring, uint> && capacityRatios,
    map<wstring, uint> && capacities)
    : nodeInstance_(nodeInstance), 
    isUp_(isUp), 
    deactivationIntent_(deactivationIntent),
    deactivationStatus_(deactivationStatus),
    nodeProperties_(move(nodeProperties)),
    faultDomainId_(move(faultDomainId)),
    upgradeDomainId_(move(upgradeDomainId)),
    capacityRatios_(move(capacityRatios)),
    capacities_(move(capacities)),
    nodeIndex_(UINT64_MAX)
{
    if (faultDomainId_.empty())
    {
        faultDomainId_.push_back(nodeInstance.Id.ToString() + L"_fd");
    }

    if (upgradeDomainId_.empty())
    {
        upgradeDomainId_ = nodeInstance.Id.ToString() + L"_ud";
    }
}

NodeDescription::NodeDescription(NodeDescription && other)
    : nodeInstance_(other.nodeInstance_), 
    isUp_(other.isUp_),
    deactivationIntent_(other.deactivationIntent_),
    deactivationStatus_(other.deactivationStatus_),
    nodeProperties_(move(other.nodeProperties_)),
    faultDomainId_(move(other.faultDomainId_)),
    upgradeDomainId_(move(other.upgradeDomainId_)),
    capacityRatios_(move(other.capacityRatios_)),
    capacities_(move(other.capacities_)),
    nodeIndex_(other.nodeIndex_)
{
}

NodeDescription::NodeDescription(NodeDescription const & other)
    : nodeInstance_(other.nodeInstance_), 
    isUp_(other.isUp_), 
    deactivationIntent_(other.deactivationIntent_),
    deactivationStatus_(other.deactivationStatus_),
    nodeProperties_(other.nodeProperties_),
    faultDomainId_(other.faultDomainId_),
    upgradeDomainId_(other.upgradeDomainId_),
    capacityRatios_(other.capacityRatios_),
    capacities_(other.capacities_),
    nodeIndex_(other.nodeIndex_)
{
}

NodeDescription & NodeDescription::operator = (NodeDescription && other)
{
    if (this != &other)
    {
        nodeInstance_ = other.nodeInstance_;
        isUp_ = other.isUp_;
        deactivationIntent_ = other.deactivationIntent_;
        deactivationStatus_ = other.deactivationStatus_;
        nodeProperties_ = move(other.nodeProperties_);
        faultDomainId_ = move(other.faultDomainId_);
        upgradeDomainId_ = move(other.upgradeDomainId_);
        capacityRatios_ = move(other.capacityRatios_);
        capacities_ = move(other.capacities_);
        nodeIndex_ = other.nodeIndex_;
    }

    return *this;
}

void NodeDescription::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    writer.Write("{0} ({1}) {2} {3} {4} {5} {6} {7} {8} {9}",
        nodeInstance_,
        nodeProperties_,
        faultDomainId_,
        upgradeDomainId_,
        isUp_,
        deactivationIntent_,
        deactivationStatus_,
        capacityRatios_,
        capacities_,
        nodeIndex_);
}

void NodeDescription::WriteToEtw(uint16 contextSequenceId) const
{
    PlacementAndLoadBalancing::PLBTrace->PLBDump(contextSequenceId, wformatString(*this));
}

void Reliability::LoadBalancingComponent::NodeDescription::CorrectCpuCapacity()
{
    for (auto capacityIter = capacities_.begin(); capacityIter != capacities_.end(); ++capacityIter)
    {
        if (capacityIter->first == ServiceModel::Constants::SystemMetricNameCpuCores)
        {
            capacityIter->second = capacityIter->second * ServiceModel::Constants::ResourceGovernanceCpuCorrectionFactor;
        }
    }
}
