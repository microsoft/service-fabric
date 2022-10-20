// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Utility.h"
#include "Node.h"

using namespace std;
using namespace Common;
using namespace LBSimulator;

Node::Node(
    int index,
    std::map<std::wstring, uint> && capacities,
    Uri && faultDomainId,
    wstring && upgradeDomainId,
    std::map<std::wstring, std::wstring> && nodeProperties,
    bool isUp)
    : index_(index),
      instance_(-1),
      isUp_(isUp),
      capacityRatios_(),
      capacities_(move(capacities)),
      faultDomainId_(move(faultDomainId)),
      upgradeDomainId_(move(upgradeDomainId)),
      nodeProperties_(move(nodeProperties))
{
}

Node::Node(Node && other)
    : index_(other.index_),
      instance_(other.instance_),
      isUp_(other.isUp_),
      capacityRatios_(move(other.capacityRatios_)),
      capacities_(move(other.capacities_)),
      faultDomainId_(move(other.faultDomainId_)),
      upgradeDomainId_(move(other.upgradeDomainId_)),
      nodeProperties_(move(other.nodeProperties_))
{
}

Node & Node::operator = (Node && other)
{
    if (this != &other)
    {
        index_ = other.index_;
        instance_ = other.instance_;
        isUp_ = other.isUp_;
        capacityRatios_ = move(other.capacityRatios_);
        capacities_ = move(other.capacities_);
        faultDomainId_ = move(other.faultDomainId_);
        upgradeDomainId_ = move(other.upgradeDomainId_);
        nodeProperties_ = move(other.nodeProperties_);
    }

    return *this;
}

Federation::NodeId Node::CreateNodeId(int id)
{
    return Federation::NodeId(Common::LargeInteger(0, id));
}

Federation::NodeInstance Node::CreateNodeInstance(int id, int instance)
{
    ASSERT_IF(instance < 0, "Invalid node instance {0}", instance);
    return Federation::NodeInstance(CreateNodeId(id), instance);
}

Reliability::LoadBalancingComponent::NodeDescription Node::GetPLBNodeDescription() const
{
    return Reliability::LoadBalancingComponent::NodeDescription(
        FederationNodeInstance,
        isUp_,
        Reliability::NodeDeactivationIntent::Enum::None,
        Reliability::NodeDeactivationStatus::Enum::None,
        map<wstring, wstring>(nodeProperties_),
        Reliability::LoadBalancingComponent::NodeDescription::DomainId(faultDomainId_.Segments),
        wstring(upgradeDomainId_),
        map<wstring, uint>(capacityRatios_),
        map<wstring, uint>(capacities_)
        );
}

void Node::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    wstring propertiesStr = L"";
    for each(auto nodeProperty in nodeProperties_)
    {
        propertiesStr += nodeProperty.first + L":" + nodeProperty.second + L",";
    }
    StringUtility::TrimTrailing(propertiesStr, StringUtility::ToWString(L","));

    writer.Write("{0} {1} {2} {3} {4} {5} {6} {7} {8}", index_,NodeName, instance_,
        isUp_, capacityRatios_, capacities_, faultDomainId_, upgradeDomainId_, propertiesStr);
}
