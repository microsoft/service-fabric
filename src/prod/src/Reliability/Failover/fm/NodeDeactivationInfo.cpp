// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace ServiceModel;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

NodeDeactivationInfo::NodeDeactivationInfo()
    : intent_(NodeDeactivationIntent::None)
    , status_(NodeDeactivationStatus::None)
    , batchIds_()
    , intents_()
    , sequenceNumber_(0)
    , pendingSafetyChecks_()
    , startTime_()
{
}

NodeDeactivationInfo::NodeDeactivationInfo(NodeDeactivationInfo const& other)
    : intent_(other.intent_)
    , status_(other.status_)
    , batchIds_(other.batchIds_)
    , intents_(other.intents_)
    , sequenceNumber_(other.sequenceNumber_)
    , pendingSafetyChecks_(other.pendingSafetyChecks_)
    , startTime_(other.startTime_)
{
}

NodeDeactivationInfo::NodeDeactivationInfo(NodeDeactivationInfo && other)
    : intent_(other.intent_)
    , status_(other.status_)
    , batchIds_(move(other.batchIds_))
    , intents_(move(other.intents_))
    , sequenceNumber_(other.sequenceNumber_)
    , pendingSafetyChecks_(move(other.pendingSafetyChecks_))
    , startTime_(move(other.startTime_))
{
}

NodeDeactivationInfo & NodeDeactivationInfo::operator=(NodeDeactivationInfo && other)
{
    if (this != &other)
    {
        intent_ = other.intent_;
        status_ = other.status_;
        batchIds_ = move(other.batchIds_);
        intents_ = move(other.intents_);
        sequenceNumber_ = other.sequenceNumber_;
        pendingSafetyChecks_ = move(other.pendingSafetyChecks_);
        startTime_ = move(other.startTime_);
    }

    return *this;
}

void NodeDeactivationInfo::set_Status(NodeDeactivationStatus::Enum value)
{
    TESTASSERT_IF(
        status_ == NodeDeactivationStatus::None && value == NodeDeactivationStatus::ActivationInProgress,
        "Cannot change node deactivation status to ActivationInProgress from status None on node {0}.");

    status_ = value;

    if (status_ == NodeDeactivationStatus::DeactivationSafetyCheckInProgress)
    {
        startTime_ = DateTime::Now();
    }

    if (!IsSafetyCheckInProgress)
    {
        pendingSafetyChecks_.clear();
    }
}

TimeSpan NodeDeactivationInfo::get_Duration() const
{
    if (status_ == NodeDeactivationStatus::None ||
        startTime_ == DateTime::Zero)
    {
        return TimeSpan::Zero;
    }

    return DateTime::Now() - startTime_;
}

// When upgrading from earlier versions, intents_
// is not initialized. Do the initialization here
// so that intents_ and bataches_ stays in sync.
void NodeDeactivationInfo::Initialize()
{
    if (intents_.size() != batchIds_.size())
    {
        intents_.clear();

        for (int i = 0; i < batchIds_.size(); i++)
        {
            intents_.push_back(intent_);
        }
    }
    else
    {
        for (int i = 0; i < intents_.size(); i++)
        {
            if (intents_[i] > intent_)
            {
                intent_ = intents_[i];
            }
        }
    }
}

void NodeDeactivationInfo::AddBatch(wstring const& batchId, NodeDeactivationIntent::Enum intent, int64 sequenceNumber)
{
    bool isFound = false;

    for (int i = 0; i < batchIds_.size(); i++)
    {
        if (batchIds_[i] == batchId)
        {
            if (intent > intents_[i])
            {
                intents_[i] = intent;
            }

            isFound = true;
            break;
        }
    }

    if (!isFound)
    {
        batchIds_.push_back(batchId);
        intents_.push_back(intent);
    }

    if (intent > intent_)
    {
        intent_ = intent;
        Status = NodeDeactivationStatus::DeactivationSafetyCheckInProgress;
    }

    sequenceNumber_ = sequenceNumber;
}

std::wstring NodeDeactivationInfo::GetBatchIdsWithIntent()
{
    wstring result;
    result.append(L"{");
    for (int i = 0; i < batchIds_.size(); i++)
    {
        result.append(wformatString("[{0}:{1}]", batchIds_[i], intents_[i]));
    }
    result.append(L"}");
    return result;
}

void NodeDeactivationInfo::RemoveBatch(wstring const& batchId, int64 sequenceNumber, bool isNodeUp)
{
    for (int i = 0; i < batchIds_.size(); i++)
    {
        if (batchIds_[i] == batchId)
        {
            batchIds_.erase(batchIds_.begin() + i);
            intents_.erase(intents_.begin() + i);

            intent_ = NodeDeactivationIntent::None;
            if (batchIds_.empty())
            {
                Status = (isNodeUp ? NodeDeactivationStatus::ActivationInProgress : NodeDeactivationStatus::None);
            }
            else
            {
                for (int j = 0; j < intents_.size(); j++)
                {
                    if (intents_[j] > intent_)
                    {
                        intent_ = intents_[j];
                    }
                }
            }

            break;
        }
    }

    sequenceNumber_ = sequenceNumber;
}

bool NodeDeactivationInfo::ContainsBatch(wstring const& batchId) const
{
    return (find(batchIds_.begin(), batchIds_.end(), batchId) != batchIds_.end());
}

void NodeDeactivationInfo::UpdateProgressDetails(
    NodeId nodeId,
    map<NodeId, NodeUpgradeProgress> && pending,
    map<NodeId, pair<NodeUpgradeProgress, NodeInfoSPtr>> && ready,
    map<NodeId, NodeUpgradeProgress> && waiting,
    bool isComplete)
{
    pendingSafetyChecks_.clear();

    if (!isComplete)
    {
        UpdateSafetyChecks(nodeId, move(ready));
        UpdateSafetyChecks(nodeId, move(pending));
        UpdateSafetyChecks(nodeId, move(waiting));
    }
}

void NodeDeactivationInfo::UpdateSafetyChecks(NodeId nodeId, map<NodeId, NodeUpgradeProgress> && progressDetails)
{
    for (auto const& upgradeProgress : progressDetails)
    {
        if (upgradeProgress.first == nodeId)
        {
            for (auto const& upgradeSafetyCheckWrapper : upgradeProgress.second.PendingSafetyChecks)
            {
                UpdateSafetyCheck(upgradeSafetyCheckWrapper);
            }
        }
    }
}

void NodeDeactivationInfo::UpdateSafetyChecks(NodeId nodeId, map<NodeId, pair<NodeUpgradeProgress, NodeInfoSPtr>> && progressDetails)
{
    for (auto const& upgradeProgress : progressDetails)
    {
        if (upgradeProgress.first == nodeId)
        {
            for (auto const& upgradeSafetyCheckWrapper : upgradeProgress.second.first.PendingSafetyChecks)
            {
                UpdateSafetyCheck(upgradeSafetyCheckWrapper);
            }
        }
    }
}

void NodeDeactivationInfo::UpdateSafetyCheck(UpgradeSafetyCheckWrapper const& upgradeSafetyCheckWrapper)
{
    UpgradeSafetyCheck const& upgradeSafetyCheck = *upgradeSafetyCheckWrapper.SafetyCheck;

    SafetyCheckKind::Enum kind = static_cast<SafetyCheckKind::Enum>(upgradeSafetyCheck.Kind);
    switch (kind)
    {
    case Reliability::SafetyCheckKind::EnsureSeedNodeQuorum:
        pendingSafetyChecks_.push_back(SafetyCheckWrapper(make_shared<SeedNodeSafetyCheck>(kind)));
        break;
    case Reliability::SafetyCheckKind::EnsurePartitionQuorum:
    case Reliability::SafetyCheckKind::WaitForPrimaryPlacement:
    case Reliability::SafetyCheckKind::WaitForPrimarySwap:
    case Reliability::SafetyCheckKind::WaitForReconfiguration:
    case Reliability::SafetyCheckKind::WaitForInBuildReplica:
    case Reliability::SafetyCheckKind::EnsureAvailability:
    {
        PartitionUpgradeSafetyCheck const& partitionUpgradeSafetyCheck = dynamic_cast<PartitionUpgradeSafetyCheck const&>(upgradeSafetyCheck);
        pendingSafetyChecks_.push_back(SafetyCheckWrapper(make_shared<PartitionSafetyCheck>(kind, partitionUpgradeSafetyCheck.PartitionId)));
        break;
    }
    default:
        pendingSafetyChecks_.push_back(SafetyCheckWrapper(make_shared<SafetyCheck>(kind)));
    }
}

NodeDeactivationQueryResult NodeDeactivationInfo::GetQueryResult() const
{
    vector<NodeDeactivationTask> tasks;
    for (int i = 0; i < batchIds_.size(); i++)
    {
        NodeDeactivationTask task(
            NodeDeactivationTaskId(batchIds_[i]),
            NodeDeactivationIntent::ToPublic(intents_[i]));

        tasks.push_back(move(task));
    }

    NodeDeactivationQueryResult nodeDeactivationResult(
        NodeDeactivationIntent::ToPublic(intent_),
        NodeDeactivationStatus::ToPublic(status_),
        move(tasks),
        pendingSafetyChecks_);

    return nodeDeactivationResult;
}

void NodeDeactivationInfo::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    if (IsDeactivated)
    {
        writer.Write("{0}:{1}:", intent_, status_);

        for (int i = 0; i < batchIds_.size(); i++)
        {
            writer.Write("[{0}:{1}]", batchIds_[i], intents_[i]);
        }

        writer.Write(":");
    }
    else if (status_ != NodeDeactivationStatus::None)
    {
        writer.Write("{0}:", status_);
    }

    writer.Write("{0}", sequenceNumber_);
}
