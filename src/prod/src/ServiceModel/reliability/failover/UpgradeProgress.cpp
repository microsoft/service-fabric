// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Reliability;
using namespace std;

NodeUpgradeProgress::NodeUpgradeProgress()
{
}

NodeUpgradeProgress::NodeUpgradeProgress(
    Federation::NodeId nodeId,
    wstring const& nodeName,
    NodeUpgradePhase::Enum upgradePhase)
    : nodeId_(nodeId),
    nodeName_(nodeName),
    upgradePhase_(upgradePhase),
    pendingSafetyChecks_()
{
}

NodeUpgradeProgress::NodeUpgradeProgress(
    Federation::NodeId nodeId,
    wstring const& nodeName,
    NodeUpgradePhase::Enum upgradePhase,
    UpgradeSafetyCheckKind::Enum kind)
    : nodeId_(nodeId),
      nodeName_(nodeName),
      upgradePhase_(upgradePhase),
      pendingSafetyChecks_()
{
    UpgradeSafetyCheckSPtr safetyCheck = make_shared<SeedNodeUpgradeSafetyCheck>(kind);
    pendingSafetyChecks_.push_back(move(UpgradeSafetyCheckWrapper(move(safetyCheck))));
}

NodeUpgradeProgress::NodeUpgradeProgress(
    Federation::NodeId nodeId,
    wstring const& nodeName,
    NodeUpgradePhase::Enum upgradePhase,
    UpgradeSafetyCheckKind::Enum kind,
    Guid partitionId)
    : nodeId_(nodeId),
      nodeName_(nodeName),
      upgradePhase_(upgradePhase),
      pendingSafetyChecks_()
{
    UpgradeSafetyCheckSPtr safetyCheck = make_shared<PartitionUpgradeSafetyCheck>(kind, partitionId);
    pendingSafetyChecks_.push_back(move(UpgradeSafetyCheckWrapper(move(safetyCheck))));
}

NodeUpgradeProgress::NodeUpgradeProgress(NodeUpgradeProgress && other)
    : nodeId_(other.nodeId_),
      nodeName_(move(other.nodeName_)),
      upgradePhase_(other.upgradePhase_),
      pendingSafetyChecks_(other.pendingSafetyChecks_)
{
}

NodeUpgradeProgress & NodeUpgradeProgress::operator=(NodeUpgradeProgress && other)
{
    if (this != &other)
    {
        nodeId_ = other.nodeId_;
        nodeName_ = other.nodeName_;
        upgradePhase_ = other.upgradePhase_;
        pendingSafetyChecks_ = other.pendingSafetyChecks_;
    }

    return *this;
}

bool NodeUpgradeProgress::operator==(NodeUpgradeProgress const & other) const
{
    return this->Equals(other, false);
}

bool NodeUpgradeProgress::operator!=(NodeUpgradeProgress const & other) const
{
    return !(*this == other);
}

bool NodeUpgradeProgress::Equals(NodeUpgradeProgress const & other, bool ignoreDynamicContent) const
{
    if (nodeId_ != other.nodeId_) { return false; }

    if (nodeName_ != other.nodeName_) { return false; }

    if (upgradePhase_ != other.upgradePhase_) { return false; }

    if (pendingSafetyChecks_.size() != other.pendingSafetyChecks_.size()) { return false; }

    for (auto ix=0; ix< pendingSafetyChecks_.size(); ++ix)
    {
        if (pendingSafetyChecks_[ix].Equals(other.pendingSafetyChecks_[ix], ignoreDynamicContent))
        {
            return false;
        }
    }

    return true;
}

void NodeUpgradeProgress::WriteTo(TextWriter & w, FormatOptions const&) const
{
    w.WriteLine("NodeId={0}, NodeName={1}, Phase={2}", nodeId_, nodeName_, upgradePhase_);

    for (auto const& safetyCheck : pendingSafetyChecks_)
    {
        w.WriteLine(*safetyCheck.SafetyCheck);
    }
}

void NodeUpgradeProgress::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_NODE_UPGRADE_PROGRESS & nodeUpgradeProgress) const
{
    nodeUpgradeProgress.NodeName = heap.AddString(nodeName_);

    switch (upgradePhase_)
    {
    case NodeUpgradePhase::PreUpgradeSafetyCheck:
        nodeUpgradeProgress.UpgradePhase = FABRIC_NODE_UPGRADE_PHASE_PRE_UPGRADE_SAFETY_CHECK;
        break;
    case NodeUpgradePhase::Upgrading:
        nodeUpgradeProgress.UpgradePhase = FABRIC_NODE_UPGRADE_PHASE_UPGRADING;
        break;
    case NodeUpgradePhase::PostUpgradeSafetyCheck:
        nodeUpgradeProgress.UpgradePhase = FABRIC_NODE_UPGRADE_PHASE_POST_UPGRADE_SAFETY_CHECK;
        break;
    default:
        nodeUpgradeProgress.UpgradePhase = FABRIC_NODE_UPGRADE_PHASE_INVALID;
        break;
    }

    auto pendingSafetyCheckArray = heap.AddArray<FABRIC_UPGRADE_SAFETY_CHECK>(pendingSafetyChecks_.size());

    for (auto ix = 0; ix < pendingSafetyChecks_.size(); ++ix)
    {
        pendingSafetyChecks_[ix].ToPublicApi(heap, pendingSafetyCheckArray[ix]);
    }

    auto upgradeSafetyCheckList = heap.AddItem<FABRIC_UPGRADE_SAFETY_CHECK_LIST>();
    upgradeSafetyCheckList->Count = static_cast<ULONG>(pendingSafetyCheckArray.GetCount());
    upgradeSafetyCheckList->Items = pendingSafetyCheckArray.GetRawArray();

    nodeUpgradeProgress.PendingSafetyChecks = upgradeSafetyCheckList.GetRawPointer();

    nodeUpgradeProgress.Reserved = NULL;
}

ErrorCode NodeUpgradeProgress::FromPublicApi(
    FABRIC_NODE_UPGRADE_PROGRESS const & nodeUpgradeProgress)
{
    nodeId_ = Federation::NodeId::MinNodeId;
    pendingSafetyChecks_.clear();

    auto hr = StringUtility::LpcwstrToWstring(
        nodeUpgradeProgress.NodeName,
        false,
        nodeName_);
    if (!SUCCEEDED(hr)) 
    { 
        return ErrorCode::FromHResult(hr); 
    }

    switch (nodeUpgradeProgress.UpgradePhase)
    {
    case FABRIC_NODE_UPGRADE_PHASE_PRE_UPGRADE_SAFETY_CHECK:
        upgradePhase_ = NodeUpgradePhase::PreUpgradeSafetyCheck;
        break;

    case FABRIC_NODE_UPGRADE_PHASE_UPGRADING:
        upgradePhase_ = NodeUpgradePhase::Upgrading;
        break;

    case FABRIC_NODE_UPGRADE_PHASE_POST_UPGRADE_SAFETY_CHECK:
        upgradePhase_ = NodeUpgradePhase::PostUpgradeSafetyCheck;
        break;

    default:
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString("{0} {1}", GET_FM_RC( Invalid_Node_Upgrade_Phase ), static_cast<int>(nodeUpgradeProgress.UpgradePhase)));
    }

    if (nodeUpgradeProgress.PendingSafetyChecks == NULL)
    {
        return ErrorCodeValue::Success;
    }

    FABRIC_UPGRADE_SAFETY_CHECK_LIST const * list = nodeUpgradeProgress.PendingSafetyChecks;

    if (list->Count == 0)
    {
        return ErrorCodeValue::Success;
    }

    if (list->Items == NULL)
    {
        return ErrorCode(
            ErrorCodeValue::ArgumentNull,
            wformatString("{0} {1} ({2})", GET_COM_RC( Null_Items ), list->Count, "PendingSafetyChecks"));
    }

    for (ULONG ix=0; ix<list->Count; ++ix)
    {
        UpgradeSafetyCheckWrapper safetyCheck;
        auto error = safetyCheck.FromPublicApi(list->Items[ix]);
        if (!error.IsSuccess())
        {
            return error;
        }

        pendingSafetyChecks_.push_back(safetyCheck);
    }

    return ErrorCodeValue::Success;
}

UpgradeDomainProgress::UpgradeDomainProgress()
{
}

UpgradeDomainProgress::UpgradeDomainProgress(
    wstring const& upgradeDomainName,
    vector<NodeUpgradeProgress> const& nodeUpgradeProgressList)
    : upgradeDomainName_(upgradeDomainName),
      nodeUpgradeProgressList_(nodeUpgradeProgressList)
{
}

UpgradeDomainProgress::UpgradeDomainProgress(UpgradeDomainProgress && other)
    : upgradeDomainName_(move(other.upgradeDomainName_)),
      nodeUpgradeProgressList_(move(other.nodeUpgradeProgressList_))
{
}

UpgradeDomainProgress & UpgradeDomainProgress::operator=(UpgradeDomainProgress && other)
{
    if (this != &other)
    {
        upgradeDomainName_ = move(other.upgradeDomainName_);
        nodeUpgradeProgressList_ = move(other.nodeUpgradeProgressList_);
    }

    return *this;
}

bool UpgradeDomainProgress::operator==(UpgradeDomainProgress const & other) const
{
    return this->Equals(other, false);
}

bool UpgradeDomainProgress::operator!=(UpgradeDomainProgress const & other) const
{
    return !(*this == other);
}

bool UpgradeDomainProgress::Equals(UpgradeDomainProgress const & other, bool ignoreDynamicContent) const
{
    if (upgradeDomainName_ != other.upgradeDomainName_) { return false; }

    if (nodeUpgradeProgressList_.size() != other.nodeUpgradeProgressList_.size()) { return false; }

    for (auto ix=0; ix<nodeUpgradeProgressList_.size(); ++ix)
    {
        if (nodeUpgradeProgressList_[ix].Equals(other.nodeUpgradeProgressList_[ix], ignoreDynamicContent))
        {
            return false;
        }
    }

    return true;
}


void UpgradeDomainProgress::WriteTo(TextWriter & w, FormatOptions const&) const
{
    w.WriteLine("CurrentUpgradeDomain={0}", upgradeDomainName_);

    for (size_t i = 0; i < nodeUpgradeProgressList_.size(); i++)
    {
        w.WriteLine(nodeUpgradeProgressList_[i]);
    }
}

ErrorCode UpgradeDomainProgress::FromString(
    std::wstring const & json, 
    __out UpgradeDomainProgress & result)
{
    return JsonHelper::Deserialize(result, json);
}

void UpgradeDomainProgress::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_UPGRADE_DOMAIN_PROGRESS & upgradeDomainProgress) const
{
    upgradeDomainProgress.UpgradeDomainName = heap.AddString(upgradeDomainName_);

    auto nodeUpgradeProgressArray = heap.AddArray<FABRIC_NODE_UPGRADE_PROGRESS>(nodeUpgradeProgressList_.size());

    for (auto ix = 0; ix < nodeUpgradeProgressList_.size(); ++ix)
    {
        auto & nodeUpgradeProgress = nodeUpgradeProgressArray.GetRawArray()[ix];

        nodeUpgradeProgressList_[ix].ToPublicApi(heap, nodeUpgradeProgress);
    }

    auto nodeUpgradeProgressList = heap.AddItem<FABRIC_NODE_UPGRADE_PROGRESS_LIST>();
    nodeUpgradeProgressList->Count = static_cast<ULONG>(nodeUpgradeProgressArray.GetCount());
    nodeUpgradeProgressList->Items = nodeUpgradeProgressArray.GetRawArray();

    upgradeDomainProgress.NodeProgressList = nodeUpgradeProgressList.GetRawPointer();

    upgradeDomainProgress.Reserved = NULL;
}

ErrorCode UpgradeDomainProgress::FromPublicApi(
    FABRIC_UPGRADE_DOMAIN_PROGRESS const & upgradeDomainProgress)
{
    nodeUpgradeProgressList_.clear();

    auto hr = StringUtility::LpcwstrToWstring(
        upgradeDomainProgress.UpgradeDomainName,
        false,
        upgradeDomainName_);
    if (!SUCCEEDED(hr)) 
    { 
        return ErrorCode::FromHResult(hr); 
    }

    if (upgradeDomainProgress.NodeProgressList == NULL) 
    { 
        return ErrorCodeValue::Success; 
    }

    FABRIC_NODE_UPGRADE_PROGRESS_LIST * list = upgradeDomainProgress.NodeProgressList;
    if (list->Count == 0)
    {
        return ErrorCodeValue::Success;
    }

    if (list->Items == NULL)
    {
        return ErrorCode(
            ErrorCodeValue::ArgumentNull,
            wformatString("{0} {1}", GET_COM_RC( Null_Items ), list->Count));
    }

    for (ULONG ix=0; ix<list->Count; ++ix)
    {
        NodeUpgradeProgress nodeProgress;
        auto error = nodeProgress.FromPublicApi(list->Items[ix]);
        if (!error.IsSuccess())
        {
            return error;
        }

        nodeUpgradeProgressList_.push_back(nodeProgress);
    }

    return ErrorCodeValue::Success;
}
