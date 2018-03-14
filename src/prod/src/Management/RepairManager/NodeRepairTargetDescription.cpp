// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Management::RepairManager;

NodeRepairTargetDescription::NodeRepairTargetDescription()
    : RepairTargetDescriptionBase(RepairTargetKind::Node)
    , nodeList_()
{
}

NodeRepairTargetDescription::NodeRepairTargetDescription(NodeRepairTargetDescription const & other)
    : RepairTargetDescriptionBase(other)
    , nodeList_(other.nodeList_)
{
}

NodeRepairTargetDescription::NodeRepairTargetDescription(NodeRepairTargetDescription && other)
    : RepairTargetDescriptionBase(move(other))
    , nodeList_(move(other.nodeList_))
{
}

bool NodeRepairTargetDescription::operator == (RepairTargetDescriptionBase const & other) const
{
    if (!RepairTargetDescriptionBase::operator==(other))
    {
        return false;
    }

    auto & otherCasted = static_cast<NodeRepairTargetDescription const &>(other);
    return (nodeList_ == otherCasted.nodeList_);
}

ErrorCode NodeRepairTargetDescription::FromPublicApi(FABRIC_REPAIR_TARGET_DESCRIPTION const & publicDescription)
{
    auto publicNodeList = static_cast<FABRIC_STRING_LIST*>(publicDescription.Value);
    if (publicNodeList == nullptr) { return ErrorCodeValue::InvalidArgument; }

    vector<wstring> nodeList;

    for (ULONG ix = 0; ix < publicNodeList->Count; ++ix)
    {
        wstring targetName;

        HRESULT hr = StringUtility::LpcwstrToWstring(publicNodeList->Items[ix], false, targetName);
        if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

        nodeList.push_back(move(targetName));
    }

    nodeList_.swap(nodeList);

    return ErrorCode::Success();
}

void NodeRepairTargetDescription::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_REPAIR_TARGET_DESCRIPTION & result) const
{
    auto publicNodeList = heap.AddItem<FABRIC_STRING_LIST>();
    StringList::ToPublicAPI(heap, nodeList_, publicNodeList);
    result.Kind = RepairTargetKind::ToPublicApi(kind_);
    result.Value = publicNodeList.GetRawPointer();
}

void NodeRepairTargetDescription::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write("NodeRepairTargetDescription[nodeList = {0}]", nodeList_);
}

bool NodeRepairTargetDescription::IsValid() const
{
    // List may be zero-length, but elements must be non-empty
    for (auto it = nodeList_.begin(); it != nodeList_.end(); ++it)
    {
        if (it->empty())
        {
            return false;
        }
    }

    return true;
}

RepairTargetDescriptionBaseSPtr NodeRepairTargetDescription::Clone() const
{
    return make_shared<NodeRepairTargetDescription>(*this);
}
