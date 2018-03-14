// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Management::RepairManager;

NodeRepairImpactDescription::NodeRepairImpactDescription()
    : RepairImpactDescriptionBase(RepairImpactKind::Node)
    , nodeImpactList_()
{
}

NodeRepairImpactDescription::NodeRepairImpactDescription(NodeRepairImpactDescription const & other)
    : RepairImpactDescriptionBase(other)
    , nodeImpactList_(other.nodeImpactList_)
{
}

NodeRepairImpactDescription::NodeRepairImpactDescription(NodeRepairImpactDescription && other)
    : RepairImpactDescriptionBase(move(other))
    , nodeImpactList_(move(other.nodeImpactList_))
{
}

bool NodeRepairImpactDescription::operator == (RepairImpactDescriptionBase const & other) const
{
    if (!RepairImpactDescriptionBase::operator==(other))
    {
        return false;
    }

    auto & otherCasted = static_cast<NodeRepairImpactDescription const &>(other);
    return (nodeImpactList_ == otherCasted.nodeImpactList_);
}

ErrorCode NodeRepairImpactDescription::FromPublicApi(FABRIC_REPAIR_IMPACT_DESCRIPTION const & publicDescription)
{
    auto publicNodeImpactList = static_cast<FABRIC_REPAIR_NODE_IMPACT_LIST*>(publicDescription.Value);
    if (publicNodeImpactList == nullptr) { return ErrorCodeValue::InvalidArgument; }

    vector<NodeImpact> nodeImpactList;

    auto error = PublicApiHelper::FromPublicApiList<NodeImpact, FABRIC_REPAIR_NODE_IMPACT_LIST>(
        publicNodeImpactList,
        nodeImpactList);
    if (!error.IsSuccess()) { return error; }

    nodeImpactList_.swap(nodeImpactList);

    return ErrorCode::Success();
}

void NodeRepairImpactDescription::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_REPAIR_IMPACT_DESCRIPTION & result) const
{
    auto impactList = heap.AddItem<FABRIC_REPAIR_NODE_IMPACT_LIST>();
    auto impactCount = nodeImpactList_.size();
            
    auto items = heap.AddArray<FABRIC_REPAIR_NODE_IMPACT>(impactCount);
    for (auto ix = 0; ix < impactCount; ++ix)
    {
        nodeImpactList_[ix].ToPublicApi(heap, items[ix]);
    }

    impactList->Count = static_cast<ULONG>(impactCount);
    impactList->Items = items.GetRawArray();

    result.Kind = RepairImpactKind::ToPublicApi(kind_);
    result.Value = impactList.GetRawPointer();
}

void NodeRepairImpactDescription::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write("NodeRepairImpactDescription[nodes = {0}]", nodeImpactList_);
}

bool NodeRepairImpactDescription::IsValid() const
{
    // TODO validate node names, casing, duplicates, etc?

    for (auto it = nodeImpactList_.begin(); it != nodeImpactList_.end(); ++it)
    {
        if (!it->IsValid())
        {
            return false;
        }
    }

    return true;
}

bool NodeRepairImpactDescription::IsZeroImpact() const
{
    for (auto it = nodeImpactList_.begin(); it != nodeImpactList_.end(); ++it)
    {
        if (it->ImpactLevel != NodeImpactLevel::None)
        {
            return false;
        }
    }

    // Empty impact list, or all impacts are None
    return true;
}

RepairImpactDescriptionBaseSPtr NodeRepairImpactDescription::Clone() const
{
    return make_shared<NodeRepairImpactDescription>(*this);
}
