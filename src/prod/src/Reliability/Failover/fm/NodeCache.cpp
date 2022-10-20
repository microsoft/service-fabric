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

NodeCache::NodeCache(
    FailoverManager & fm,
    vector<NodeInfoSPtr> & nodes)
    : fm_(fm)
    , lastNodeUpTime_(DateTime::Zero)
    , isClusterStable_(false)
    , upCount_(0)
    , nodeCount_(0)
    , ackedSequence_(FABRIC_INVALID_SEQUENCE_NUMBER)
    , healthInitialized_(false)
    , healthSequence_(0)
    , initialSequence_(0)
    , invalidateSequence_(0)
{
    for (size_t i = 0; i < nodes.size(); i++)
    {
        auto nodeCopy = nodes[i];
        if (nodeCopy->HealthSequence >= healthSequence_)
        {
            healthSequence_ = nodeCopy->HealthSequence + 1;
        }

        InitializeNode(move(nodeCopy));
    }

    // Currently we only need health report from FM, not FMM.
    if (!fm.IsMaster)
    {
        initialSequence_ = healthSequence_;

        fm.HealthClient->HealthPreInitialize(*ServiceModel::Constants::HealthReportFMSource, FABRIC_HEALTH_REPORT_KIND_NODE, fm_.PreOpenFMServiceEpoch.DataLossVersion);

        fm.WriteInfo(Constants::NodeCacheSource, "Loaded health sequence: {0}", healthSequence_);
    }
}

void NodeCache::InitializeNode(NodeInfoSPtr && node)
{
    AddUpgradeDomain(*node);

    if (node->IsUp)
    {
        if (node->LastUpdated > lastNodeUpTime_)
        {
            lastNodeUpTime_ = node->LastUpdated;
        }

        upCount_++;
    }

    node->InitializeNodeName();

    node->DeactivationInfo.Initialize();

    DateTime stableTime = lastNodeUpTime_ + FailoverConfig::GetConfig().ClusterStableWaitDuration;
    isClusterStable_ = (stableTime < DateTime::Now());

    Federation::NodeId const& nodeId = node->NodeInstance.Id;
    nodes_.insert(make_pair(nodeId, make_shared<CacheEntry<NodeInfo>>(move(node))));

    nodeCount_++;
}

bool NodeCache::get_IsClusterStable() const
{
    AcquireReadLock grab(lock_);

    // Once a cluster is stable, it never becomes unstable again.
    if (!isClusterStable_)
    {
        DateTime stableTime = lastNodeUpTime_ + FailoverConfig::GetConfig().ClusterStableWaitDuration;
        isClusterStable_ = (stableTime < DateTime::Now());
    }

    return isClusterStable_;
}

bool NodeCache::get_IsClusterPaused() const
{
     return !fm_.IsMaster && UpCount < static_cast<size_t>(FailoverConfig::GetConfig().ClusterPauseThreshold);
}

void NodeCache::Initialize(vector<NodeInfo> && nodes)
{
    ASSERT_IF(
        nodes_.size() > 0u,
        "Initialize should only be called when the NodeCache is empty: {0}", nodes_.size());

    for (size_t i = 0; i < nodes.size(); ++i)
    {
        NodeInfoSPtr node = make_shared<NodeInfo>(move(nodes[i]));
        int64 plbDuration;
        fm_.PLB.UpdateNode(node->GetPLBNodeDescription(), plbDuration);
        InitializeNode(move(node));
    }
}

void NodeCache::StartPeriodicTask()
{
    fm_.Events.PeriodicTaskBeginNoise(fm_.Id, PeriodicTaskName::NodeCache);
    auto startTime = Stopwatch::Now();

    NodePeriodicTaskCounts taskCounts;

    ProcessNodes(taskCounts);

    ProcessHealth();

    auto duration = Stopwatch::Now() - startTime;
    fm_.Events.NodePeriodicTaskEnd(fm_.Id, duration, taskCounts);
}

void NodeCache::ProcessHealth()
{
    if (fm_.IsMaster)
    {
        healthInitialized_ = true;
        return;
    }

    vector<CacheEntrySPtr> nodesToUpdate;

    {
        AcquireWriteLock grab(lock_);

        FABRIC_SEQUENCE_NUMBER progress;
        ErrorCode error = fm_.HealthClient->HealthGetProgress(*ServiceModel::Constants::HealthReportFMSource, FABRIC_HEALTH_REPORT_KIND_NODE, progress);
        if (!error.IsSuccess())
        {
            return;
        }

        ackedSequence_ = progress;

        if (!healthInitialized_)
        {
            InitializeHealthState();
        }
        else if (invalidateSequence_ != FABRIC_INVALID_SEQUENCE_NUMBER)
        {
            for (auto it = nodes_.begin(); it != nodes_.end(); ++it)
            {
                NodeInfoSPtr const& nodeInfo = it->second->Get();
                if (!nodeInfo->IsUnknown &&
                    nodeInfo->HealthSequence < invalidateSequence_)
                {
                    nodesToUpdate.push_back(it->second);
                }
            }

            if (nodesToUpdate.size() == 0)
            {
                invalidateSequence_ = FABRIC_INVALID_SEQUENCE_NUMBER;
            }
        }
    }

    TimeoutHelper timeout(TimeSpan::FromSeconds(1));
    for (auto it = nodesToUpdate.begin(); it != nodesToUpdate.end() && !timeout.IsExpired; ++it)
    {
        LockedNodeInfo lockedNodeInfo;
        ErrorCode error = (*it)->Lock(*it, lockedNodeInfo, TimeSpan::Zero);
        if (error.IsSuccess() && lockedNodeInfo->HealthSequence < invalidateSequence_)
        {
            NodeInfoSPtr newNodeInfo = make_shared<NodeInfo>(*lockedNodeInfo);
            newNodeInfo->HealthSequence = InterlockedIncrement64(&healthSequence_) - 1;
            newNodeInfo->PersistenceState = PersistenceState::ToBeUpdated;

            auto lockedNodeInfoMover = MoveUPtr<LockedNodeInfo>(move(make_unique<LockedNodeInfo>(move(lockedNodeInfo))));

            fm_.Store.BeginUpdateData(
                *newNodeInfo,
                [this, newNodeInfo, lockedNodeInfoMover](AsyncOperationSPtr const& operation) mutable
                {
                    int64 commitDuration(0);
                    ErrorCode error = fm_.Store.EndUpdateData(*newNodeInfo, operation, commitDuration);

                    ReportHealthAfterNodeUpdate(newNodeInfo, error);

                    if (error.IsSuccess())
                    {
                        fm_.WriteInfo(Constants::NodeUpdateSource, newNodeInfo->IdString,
                            "Health sequence updated: {0}Commit Duration = {1} ms", *newNodeInfo, commitDuration);

                        auto lockedNodeInfo = lockedNodeInfoMover.TakeUPtr();
                        lockedNodeInfo->Commit(move(newNodeInfo));
                    }
                    else
                    {
                        fm_.WriteWarning(
                            Constants::NodeSource, newNodeInfo->IdString,
                            "Update health sequence for node {0} failed with {1}\r\nCommit Duration = {2} ms", newNodeInfo->Id, error, commitDuration);
                    }
                },
                fm_.CreateAsyncOperationRoot());
        }
    }
}

void NodeCache::InitializeHealthState()
{
    FABRIC_SEQUENCE_NUMBER progress = ackedSequence_;

    fm_.WriteInfo(Constants::NodeCacheSource, "HealthGetProgress: {0}", progress);

    if (progress > initialSequence_)
    {
        fm_.WriteWarning(
            Constants::NodeCacheSource,
            "Progress retrieved {0} is larger than FM knows {1}", progress, initialSequence_);

        invalidateSequence_ = initialSequence_ = progress;

        FABRIC_SEQUENCE_NUMBER currentSequence = healthSequence_;
        while (currentSequence < progress)
        {
            FABRIC_SEQUENCE_NUMBER last = InterlockedCompareExchange64(&healthSequence_, progress, currentSequence);
            if (last == currentSequence)
            {
                fm_.WriteWarning(
                    Constants::NodeCacheSource,
                    "Updating sequence from {0} to {1}", currentSequence, progress);
                break;
            }
            else
            {
                currentSequence = last;
            }
        }
    }

    size_t reportCount = 0;
    if (initialSequence_ > progress)
    {
        vector<HealthReport> reports;

        for (auto it = nodes_.begin(); it != nodes_.end(); ++it)
        {
            NodeInfoSPtr const& nodeInfo = it->second->Get();
            if (nodeInfo->HealthSequence >= progress && nodeInfo->HealthSequence < initialSequence_)
            {
                reports.push_back(
                    fm_.HealthReportFactoryObj.GenerateNodeInfoHealthReport(
                        nodeInfo,
                        fm_.Federation.IsSeedNode(nodeInfo->Id)));
            }
        }

        reportCount = reports.size();
        if (reportCount > 0)
        {
            fm_.HealthClient->AddHealthReports(move(reports));
        }
    }

    FABRIC_SEQUENCE_NUMBER invalidateSequence = (invalidateSequence_ == 0 ? FABRIC_INVALID_SEQUENCE_NUMBER : invalidateSequence_);
    fm_.HealthClient->HealthPostInitialize(*ServiceModel::Constants::HealthReportFMSource, FABRIC_HEALTH_REPORT_KIND_NODE, initialSequence_, invalidateSequence);
    fm_.WriteInfo(Constants::NodeCacheSource, "HealthPostInitialize {0}/{1} with {2} reports", initialSequence_, invalidateSequence, reportCount);

    healthInitialized_ = true;
}

void NodeCache::GetNodes(vector<NodeInfo> & nodes) const
{
    AcquireReadLock grab(lock_);

    for (auto it = nodes_.begin(); it != nodes_.end(); ++it)
    {
        NodeInfoSPtr const& nodeInfo = it->second->Get();
        nodes.push_back(*nodeInfo);
    }
}

ErrorCode NodeCache::GetNodesForGFUMTransfer(__out vector<NodeInfo> & nodeInfos) const
{
    ASSERT_IFNOT(fm_.IsMaster, "Only FMM request the nodes GFUM transfer");

    AcquireReadLock grab(lock_);

    for (auto it = nodes_.begin(); it != nodes_.end(); ++it)
    {
        LockedNodeInfo lockedNodeInfo;
        ErrorCode error = it->second->Lock(it->second, lockedNodeInfo);
        if (!error.IsSuccess())
        {
            return error;
        }

        nodeInfos.push_back(*lockedNodeInfo);
    }

    return ErrorCode::Success();
}

void NodeCache::GetNodes(vector<NodeInfoSPtr> & nodes) const
{
    AcquireReadLock grab(lock_);

    for (auto it = nodes_.begin(); it != nodes_.end(); it++)
    {
        NodeInfoSPtr const& nodeInfo = it->second->Get();
        nodes.push_back(nodeInfo);
    }
}

void NodeCache::GetNodes(NodeId continuationToken, vector<NodeInfoSPtr> & nodes) const
{
    AcquireReadLock grab(lock_);

    auto contTokenIt = nodes_.upper_bound(continuationToken);

    if (contTokenIt == nodes_.end())
    {
        fm_.WriteInfo(Constants::NodeCacheSource, "Get nodes with continuation token {0} returned no results", continuationToken);
        return;
    }

    for (auto it = contTokenIt; it != nodes_.end(); it++)
    {
        NodeInfoSPtr const& nodeInfo = it->second->Get();
        nodes.push_back(nodeInfo);
    }
}

NodeInfoSPtr NodeCache::GetNode(NodeId id) const
{
    AcquireReadLock grab(lock_);

    auto it = nodes_.find(id);
    if (it == nodes_.end())
    {
        return nullptr;
    }

    return it->second->Get();
}

ErrorCode NodeCache::DeactivateNodes(map<NodeId, NodeDeactivationIntent::Enum> const& nodesToDeactivate, wstring const& batchId, int64 sequenceNumber)
{
    vector<LockedNodeInfo> lockedNodeInfos;
    vector<NodeInfoSPtr> newNodeInfos;

    for (auto it = nodesToDeactivate.begin(); it != nodesToDeactivate.end(); ++it)
    {
        NodeId nodeId = it->first;
        NodeDeactivationIntent::Enum deactivationIntent = it->second;

        if (deactivationIntent == NodeDeactivationIntent::None)
        {
            fm_.WriteInfo(Constants::NodeSource, wformatString(nodeId),
                "DeactivateNodes processing for node {0} failed with Invalid Argument Exception: Deactivation intent 'None'",
                nodeId);
            return ErrorCodeValue::InvalidArgument;
        }

        bool isNewEntry;
        LockedNodeInfo lockedNodeInfo;
        ErrorCode error = GetLockedNode(it->first, lockedNodeInfo, true, FailoverConfig::GetConfig().LockAcquireTimeout, isNewEntry);
        if (!error.IsSuccess())
        {
            fm_.WriteInfo(
                Constants::NodeSource, wformatString(nodeId),
                "DeactivateNodes processing for node {0} failed with {1}", nodeId, error);

            return error;
        }

        if (lockedNodeInfo->DeactivationInfo.SequenceNumber >= sequenceNumber)
        {
            fm_.WriteInfo(
                Constants::NodeSource, lockedNodeInfo->IdString,
                "Ignoring stale/duplicate DeactivateNodes request for node {0}. SeqNum = {1}, Incoming = {2}",
                nodeId, lockedNodeInfo->DeactivationInfo.SequenceNumber, sequenceNumber);

            if (lockedNodeInfo->DeactivationInfo.SequenceNumber == sequenceNumber)
            {
                continue;
            }
            else
            {
                return ErrorCodeValue::StaleRequest;
            }
        }

        NodeInfoSPtr newNodeInfo = make_shared<NodeInfo>(*lockedNodeInfo);
        newNodeInfo->DeactivationInfo.AddBatch(batchId, deactivationIntent, sequenceNumber);

        if ((!newNodeInfo->IsUp &&
            deactivationIntent != NodeDeactivationIntent::RemoveData &&
            deactivationIntent != NodeDeactivationIntent::RemoveNode) ||
            (deactivationIntent == NodeDeactivationIntent::Pause &&
            !FailoverConfig::GetConfig().PerformSafetyChecksForNodeDeactivationIntentPause))
        {
            newNodeInfo->DeactivationInfo.Status = NodeDeactivationStatus::DeactivationComplete;
        }

        if (isNewEntry)
        {
            newNodeInfo->PersistenceState = PersistenceState::ToBeInserted;
        }

        lockedNodeInfos.push_back(move(lockedNodeInfo));
        newNodeInfos.push_back(move(newNodeInfo));
    }

    ErrorCode error = fm_.Store.UpdateNodes(newNodeInfos);
    if (error.IsSuccess())
    {
        for (size_t i = 0; i < newNodeInfos.size(); i++)
        {
            NodeInfoSPtr & newNodeInfo = newNodeInfos[i];
            
            int64 plbElapsedTime;
            fm_.PLB.UpdateNode(newNodeInfo->GetPLBNodeDescription(), plbElapsedTime);

            fm_.WriteInfo(
                Constants::NodeUpdateSource, newNodeInfo->IdString,
                "Node deactivation started for batch {0}: {1}, PLB Duration = {2} ms", batchId, *newNodeInfo, plbElapsedTime);

            // It implies that this is a new node which we are seeing for first time.
            if (newNodeInfo->PersistenceState == PersistenceState::ToBeInserted)
            {
                // Push data to Event store.
                fm_.NodeEvents.NodeAddedOperational(
                    Common::Guid::NewGuid(),
                    newNodeInfo->Description.NodeName,
                    newNodeInfo->IdString,
                    newNodeInfo->NodeInstance.InstanceId,
                    newNodeInfo->NodeType,
                    newNodeInfo->VersionInstance.ToString(),
                    newNodeInfo->IpAddressOrFQDN,
                    wformatString(newNodeInfo->NodeCapacities));

                nodeCount_++;
            }

            // Push the information that node Deactivation has started to Event Store.
            fm_.NodeEvents.DeactivateNodeStartOperational(
                Common::Guid::NewGuid(),
                newNodeInfo->Description.NodeName,
                newNodeInfo->NodeInstance.InstanceId,
                batchId,
                nodesToDeactivate.find(newNodeInfo->Id)->second); // we know the entry is there so no need to check existence.

            if (newNodeInfo->DeactivationInfo.Status == NodeDeactivationStatus::DeactivationComplete)
            {
                // Push this information to Event Store
                fm_.NodeEvents.DeactivateNodeCompletedOperational(
                    Common::Guid::NewGuid(),
                    newNodeInfo->Description.NodeName,
                    newNodeInfo->NodeInstance.InstanceId,
                    newNodeInfo->DeactivationInfo.Intent,
                    newNodeInfo->DeactivationInfo.GetBatchIdsWithIntent(),
                    newNodeInfo->DeactivationInfo.StartTime);
            }

            lockedNodeInfos[i].Commit(move(newNodeInfo));
        }
    }
    else
    {
        for (size_t i = 0; i < newNodeInfos.size(); i++)
        {
            NodeInfoSPtr & newNodeInfo = newNodeInfos[i];

            if (newNodeInfo->PersistenceState == PersistenceState::ToBeInserted)
            {
                AcquireWriteLock grab(lock_);

                lockedNodeInfos[i].IsDeleted = true;
                nodes_.erase(newNodeInfo->Id);
            }

            fm_.WriteInfo(
                Constants::NodeSource, newNodeInfo->IdString,
                "DeactivateNodes processing for batch {0} node {1} failed with {2}", batchId, newNodeInfo->Id, error);
        }
    }

    return error;
}

ErrorCode NodeCache::GetNodeDeactivationStatus(
    wstring const& batchId,
    __out NodeDeactivationStatus::Enum & nodeDeactivationStatus,
    __out vector<NodeProgress> & progressDetails) const
{
    nodeDeactivationStatus = NodeDeactivationStatus::None;
    bool isFound = false;

    AcquireReadLock grab(lock_);

    for (auto it = nodes_.begin(); it != nodes_.end(); it++)
    {
        NodeInfoSPtr const& nodeInfo = it->second->Get();

        if (nodeInfo->DeactivationInfo.ContainsBatch(batchId))
        {
            isFound = true;

            if (nodeInfo->DeactivationInfo.Status <= nodeDeactivationStatus)
            {
                nodeDeactivationStatus = nodeInfo->DeactivationInfo.Status;

                if (!nodeInfo->DeactivationInfo.PendingSafetyChecks.empty())
                {
                    NodeProgress nodeProgress(nodeInfo->Id, nodeInfo->NodeName, nodeInfo->DeactivationInfo.PendingSafetyChecks);
                    progressDetails.push_back(move(nodeProgress));
                }
            }
        }
    }

    if (!isFound)
    {
        nodeDeactivationStatus = NodeDeactivationStatus::DeactivationComplete;
    }

    return ErrorCodeValue::Success;
}

ErrorCode NodeCache::RemoveNodeDeactivation(wstring const& batchId, int64 sequenceNumber)
{
    vector<LockedNodeInfo> lockedNodeInfos;
    vector<NodeInfoSPtr> newNodeInfos;

    {
        AcquireReadLock grab(lock_);

        for (auto it = nodes_.begin(); it != nodes_.end(); ++it)
        {
            NodeInfoSPtr nodeInfo = it->second->Get();

            if (nodeInfo->DeactivationInfo.ContainsBatch(batchId))
            {
                LockedNodeInfo lockedNodeInfo;
                ErrorCode error = it->second->Lock(it->second, lockedNodeInfo);
                if (error.IsSuccess() && lockedNodeInfo->DeactivationInfo.ContainsBatch(batchId))
                {
                    NodeInfoSPtr newNodeInfo = make_shared<NodeInfo>(*lockedNodeInfo);
                    newNodeInfo->DeactivationInfo.RemoveBatch(batchId, sequenceNumber, newNodeInfo->IsUp);

                    lockedNodeInfos.push_back(move(lockedNodeInfo));
                    newNodeInfos.push_back(move(newNodeInfo));
                }
                else if (!error.IsSuccess())
                {
                    fm_.WriteInfo(
                        Constants::NodeSource, wformatString(nodeInfo->Id),
                        "RemoveNodeDeactivation processing for batch {0} node {1} failed with {2}", batchId, nodeInfo->Id, error);

                    return error;
                }
            }
        }
    }

    ErrorCode error = fm_.Store.UpdateNodes(newNodeInfos);
    if (error.IsSuccess())
    {
        for (size_t i = 0; i < newNodeInfos.size(); i++)
        {
            NodeInfoSPtr & newNodeInfo = newNodeInfos[i];

            int64 plbElapsedTime;
            fm_.PLB.UpdateNode(newNodeInfo->GetPLBNodeDescription(), plbElapsedTime);

            fm_.WriteInfo(
                Constants::NodeUpdateSource, newNodeInfo->IdString,
                "Node deactivation removed for batch {0}: {1}, PLB duration = {2} ms", batchId, *newNodeInfo, plbElapsedTime);

            lockedNodeInfos[i].Commit(move(newNodeInfo));
        }
    }
    else
    {
        for (size_t i = 0; i < newNodeInfos.size(); i++)
        {
            NodeInfoSPtr & newNodeInfo = newNodeInfos[i];

            fm_.WriteInfo(
                Constants::NodeSource, newNodeInfo->IdString,
                "RemoveNodeDeactivation processing for batch {0} node {1} failed with {2}", batchId, newNodeInfo->Id, error);
        }
    }

    return error;
}

ErrorCode NodeCache::UpdateNodeDeactivationStatus(
    NodeId nodeId,
    NodeDeactivationIntent::Enum deactivationIntent,
    NodeDeactivationStatus::Enum deactivationStatus,
    map<NodeId, NodeUpgradeProgress> && pending,
    map<NodeId, pair<NodeUpgradeProgress, NodeInfoSPtr>> && ready,
    map<NodeId, NodeUpgradeProgress> && waiting)
{
    LockedNodeInfo lockedNodeInfo;
    ErrorCode error = GetLockedNode(nodeId, lockedNodeInfo);
    if (!error.IsSuccess())
    {
        fm_.WriteInfo(
            Constants::NodeSource, wformatString(nodeId),
            "UpdateNodeDeactivationStatus processing for node {0} failed with {1}", nodeId, error);

        return error;
    }

    if (!lockedNodeInfo->IsUp &&
        lockedNodeInfo->DeactivationInfo.IsRemove &&
        deactivationStatus == NodeDeactivationStatus::DeactivationSafetyCheckComplete)
    {
        deactivationStatus = NodeDeactivationStatus::DeactivationComplete;
    }

    lockedNodeInfo->DeactivationInfo.UpdateProgressDetails(nodeId, move(pending), move(ready), move(waiting), deactivationStatus == NodeDeactivationStatus::DeactivationComplete);

    if (lockedNodeInfo->DeactivationInfo.Intent != deactivationIntent)
    {
        return error;
    }

    if (lockedNodeInfo->DeactivationInfo.Status == deactivationStatus)
    {
        return error;
    }

    NodeInfoSPtr newNodeInfo = make_shared<NodeInfo>(*lockedNodeInfo);
    newNodeInfo->DeactivationInfo.Status = deactivationStatus;

    if (deactivationStatus == NodeDeactivationStatus::DeactivationComplete)
    {
        newNodeInfo->IsPendingDeactivateNode = false;
    }
    else if (!newNodeInfo->DeactivationInfo.IsPause && deactivationStatus == NodeDeactivationStatus::DeactivationSafetyCheckComplete)
    {
        newNodeInfo->IsPendingDeactivateNode = true;
    }

    int64 commitDuration;
    error = fm_.Store.UpdateData(*newNodeInfo, commitDuration);
    if (error.IsSuccess())
    {
        int64 plbElapsedTime;
        fm_.PLB.UpdateNode(newNodeInfo->GetPLBNodeDescription(), plbElapsedTime);
        fm_.WriteInfo(
            Constants::NodeUpdateSource, newNodeInfo->IdString,
            "Node deactivation status updated: {0}Commit Duration = {1} ms, PLB Duration = {2} ms", *newNodeInfo, commitDuration, plbElapsedTime);

        if (newNodeInfo->DeactivationInfo.Status == NodeDeactivationStatus::DeactivationComplete)
        {
            // Push this information to Event Store
            fm_.NodeEvents.DeactivateNodeCompletedOperational(
                Common::Guid::NewGuid(),
                newNodeInfo->Description.NodeName,
                newNodeInfo->NodeInstance.InstanceId,
                newNodeInfo->DeactivationInfo.Intent,
                newNodeInfo->DeactivationInfo.GetBatchIdsWithIntent(),
                newNodeInfo->DeactivationInfo.StartTime);
        }

        lockedNodeInfo.Commit(move(newNodeInfo));
    }
    else
    {
        fm_.WriteInfo(
            Constants::NodeSource, wformatString(nodeId),
            "UpdateNodeDeactivationStatus processing for node {0} failed with {1}\r\nCommit Duration = {2} ms", nodeId, error, commitDuration);
    }

    return error;
}

void NodeCache::ProcessNodeDeactivateReplyAsync(NodeId const& nodeId, int64 sequenceNumber)
{
    LockedNodeInfo lockedNodeInfo;
    ErrorCode error = GetLockedNode(nodeId, lockedNodeInfo);
    if (!error.IsSuccess())
    {
        fm_.WriteInfo(
            Constants::NodeSource, wformatString(nodeId),
            "NodeDeactivateReply processing for node {0} failed with {1}", nodeId, error);

        return;
    }

    ASSERT_IF(
        sequenceNumber > lockedNodeInfo->DeactivationInfo.SequenceNumber,
        "Deactivation sequence number in reply from from node {0} cannot be greater than what FM has: SeqNum={1}, Incoming={2}",
        nodeId, lockedNodeInfo->DeactivationInfo.SequenceNumber, sequenceNumber);

    if (sequenceNumber < lockedNodeInfo->DeactivationInfo.SequenceNumber)
    {
        fm_.WriteInfo(
            Constants::NodeSource, lockedNodeInfo->IdString,
            "Ignoring stale NodeDeactivateReply message from node {0}: SeqNum={1}, Incoming={2}",
            nodeId, lockedNodeInfo->DeactivationInfo.SequenceNumber, sequenceNumber);

        return;
    }

    if (lockedNodeInfo->DeactivationInfo.IsSafetyCheckComplete)
    {
        NodeInfoSPtr newNodeInfo = make_shared<NodeInfo>(*lockedNodeInfo);
        newNodeInfo->DeactivationInfo.Status = NodeDeactivationStatus::DeactivationComplete;
        newNodeInfo->IsPendingDeactivateNode = false;

        auto lockedNodeInfoMover = MoveUPtr<LockedNodeInfo>(move(make_unique<LockedNodeInfo>(move(lockedNodeInfo))));

        fm_.Store.BeginUpdateData(
            *newNodeInfo,
            [this, newNodeInfo, lockedNodeInfoMover](AsyncOperationSPtr const& operation) mutable
            {
                int64 commitDuration(0);
                ErrorCode error = fm_.Store.EndUpdateData(*newNodeInfo, operation, commitDuration);
                if (error.IsSuccess())
                {
                    int64 plbElapsedTime;
                    fm_.PLB.UpdateNode(newNodeInfo->GetPLBNodeDescription(), plbElapsedTime);

                    fm_.WriteInfo(
                        Constants::NodeUpdateSource, newNodeInfo->IdString,
                        "Node deactivation complete: {0}Commit Duration = {1} ms, PLB Duration = {2} ms", *newNodeInfo, commitDuration, plbElapsedTime);

                    // Push data to Event store.
                    fm_.NodeEvents.DeactivateNodeCompletedOperational(
                        Common::Guid::NewGuid(),
                        newNodeInfo->Description.NodeName,
                        newNodeInfo->NodeInstance.InstanceId,
                        newNodeInfo->DeactivationInfo.Intent,
                        newNodeInfo->DeactivationInfo.GetBatchIdsWithIntent(),
                        newNodeInfo->DeactivationInfo.StartTime);

                    auto lockedNodeInfo = lockedNodeInfoMover.TakeUPtr();
                    lockedNodeInfo->Commit(move(newNodeInfo));
                }
                else
                {
                    fm_.WriteInfo(
                        Constants::NodeSource, newNodeInfo->IdString,
                        "NodeDeactivateReply processing for node {0} failed with {1}\r\nCommit Duration = {2} ms",
                        newNodeInfo->Id, error, commitDuration);
                }
            },
            fm_.CreateAsyncOperationRoot());
    }
}

void NodeCache::ProcessNodeActivateReplyAsync(NodeId const& nodeId, int64 sequenceNumber)
{
    LockedNodeInfo lockedNodeInfo;
    ErrorCode error = GetLockedNode(nodeId, lockedNodeInfo);
    if (!error.IsSuccess())
    {
        fm_.WriteInfo(
            Constants::NodeSource, wformatString(nodeId),
            "NodeActivateReply processing for node {0} failed with {1}", nodeId, error);

        return;
    }

    ASSERT_IF(
        sequenceNumber > lockedNodeInfo->DeactivationInfo.SequenceNumber,
        "Deactivation sequence number in reply from from node {0} cannot be greater than what FM has: SeqNum={1}, Incoming={2}",
        nodeId, lockedNodeInfo->DeactivationInfo.SequenceNumber, sequenceNumber);

    if (sequenceNumber < lockedNodeInfo->DeactivationInfo.SequenceNumber)
    {
        fm_.WriteInfo(
            Constants::NodeSource, lockedNodeInfo->IdString,
            "Ignoring stale NodeActivateReply message from node {0}: SeqNum={1}, Incoming={2}",
            nodeId, lockedNodeInfo->DeactivationInfo.SequenceNumber, sequenceNumber);

        return;
    }

    if (!lockedNodeInfo->DeactivationInfo.IsDeactivated && lockedNodeInfo->DeactivationInfo.IsActivationInProgress)
    {
        NodeInfoSPtr newNodeInfo = make_shared<NodeInfo>(*lockedNodeInfo);
        newNodeInfo->DeactivationInfo.Status = NodeDeactivationStatus::None;    
        newNodeInfo->IsPendingDeactivateNode = false;

        auto lockedNodeInfoMover = MoveUPtr<LockedNodeInfo>(move(make_unique<LockedNodeInfo>(move(lockedNodeInfo))));

        fm_.Store.BeginUpdateData(
            *newNodeInfo,
            [this, newNodeInfo, lockedNodeInfoMover](AsyncOperationSPtr const& operation) mutable
            {
                int64 commitDuration(0);
                ErrorCode error = fm_.Store.EndUpdateData(*newNodeInfo, operation, commitDuration);
                if (error.IsSuccess())
                {
                    int64 plbElapsedTime;
                    fm_.PLB.UpdateNode(newNodeInfo->GetPLBNodeDescription(), plbElapsedTime);

                    fm_.WriteInfo(
                        Constants::NodeUpdateSource, newNodeInfo->IdString,
                        "Node activation complete: {0}Commit Duration = {1} ms, PLB Duration = {2} ms", *newNodeInfo, commitDuration, plbElapsedTime);

                    auto lockedNodeInfo = lockedNodeInfoMover.TakeUPtr();
                    lockedNodeInfo->Commit(move(newNodeInfo));
                }
                else
                {
                    fm_.WriteInfo(
                        Constants::NodeSource, newNodeInfo->IdString,
                        "NodeActivateReply processing for node {0} failed with {1}\r\nCommit Duration = {2} ms",
                        newNodeInfo->Id, error, commitDuration);
                }
            },
            fm_.CreateAsyncOperationRoot());
    }
}

NodeDeactivationInfo NodeCache::GetNodeDeactivationInfo(Federation::NodeId const& nodeId) const
{
    AcquireReadLock grab(lock_);

    auto it = nodes_.find(nodeId);
    if (it != nodes_.end())
    {
        NodeInfoSPtr const& nodeInfo = it->second->Get();
        return nodeInfo->DeactivationInfo;
    }

    return NodeDeactivationInfo();
}

void NodeCache::AddDeactivateNodeContext(vector<BackgroundThreadContextUPtr> & contexts) const
{
    map<wstring, map<NodeId, NodeDeactivationIntent::Enum>> deactivateBatches;

    {
        AcquireReadLock grab(lock_);

        for (auto it = nodes_.begin(); it != nodes_.end(); ++it)
        {
            NodeInfoSPtr const& nodeInfo = it->second->Get();

            if (nodeInfo->DeactivationInfo.IsDeactivated &&
                !nodeInfo->DeactivationInfo.IsDeactivationComplete)
            {
                for (wstring const& batchId : nodeInfo->DeactivationInfo.BatchIds)
                {
                    auto it1 = deactivateBatches.find(batchId);

                    if (it1 == deactivateBatches.end())
                    {
                        map<NodeId, NodeDeactivationIntent::Enum> nodesToDeactivate;
                        nodesToDeactivate[nodeInfo->Id] = nodeInfo->DeactivationInfo.Intent;

                        deactivateBatches[batchId] = move(nodesToDeactivate);
                    }
                    else
                    {
                        deactivateBatches[batchId][nodeInfo->Id] = nodeInfo->DeactivationInfo.Intent;
                    }
                }
            }
        }
    }

    if (!deactivateBatches.empty())
    {
        // Randomly pick a batch
        auto it = deactivateBatches.begin();
        int r = Random().Next(static_cast<int>(deactivateBatches.size()));
        for (int i = 0; i < r; i++)
        {
            it++;
        }

        auto context = make_unique<DeactivateNodesContext>(it->second, it->first);
        context->Initialize(fm_);

        contexts.push_back(move(context));
    }
}

AsyncOperationSPtr NodeCache::BeginUpdateNodeSequenceNumber(
    NodeInstance const& nodeInstance,
    uint64 sequenceNumber,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& state)
{
    LockedNodeInfo lockedNodeInfo;
    ErrorCode error = GetLockedNode(nodeInstance.Id, lockedNodeInfo);
    if (!error.IsSuccess())
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, state);
    }

    if (lockedNodeInfo->NodeInstance != nodeInstance || lockedNodeInfo->SequenceNumber >= sequenceNumber)
    {
        fm_.NodeEvents.UpdateNodeSequenceNumberStale(nodeInstance.Id);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCodeValue::StaleRequest, callback, state);
    }

    NodeInfoSPtr newNodeInfo = make_shared<NodeInfo>(*lockedNodeInfo);
    newNodeInfo->SequenceNumber = sequenceNumber;

    auto newNodeInfoPtr = newNodeInfo.get();

    pair<LockedNodeInfo, NodeInfoSPtr> context(move(lockedNodeInfo), move(newNodeInfo));

    return fm_.Store.BeginUpdateData(
        *newNodeInfoPtr,
        OperationContextCallback<pair<LockedNodeInfo, NodeInfoSPtr>>(callback, move(context)),
        state);
}

ErrorCode NodeCache::EndUpdateNodeSequenceNumber(AsyncOperationSPtr const& operation)
{
    unique_ptr<OperationContext<pair<LockedNodeInfo, NodeInfoSPtr>>> context = operation->PopOperationContext<pair<LockedNodeInfo, NodeInfoSPtr>>();

    if (context)
    {
        LockedNodeInfo & lockedNodeInfo = context->Context.first;
        NodeInfoSPtr & newNodeInfo = context->Context.second;

        int64 commitDuration;
        ErrorCode error = fm_.Store.EndUpdateData(*newNodeInfo, operation, commitDuration);

        if (error.IsSuccess())
        {
            fm_.WriteInfo(
                Constants::NodeUpdateSource, newNodeInfo->IdString,
                "SequenceNumber updated: {0}Commit Duration = {1} ms", *newNodeInfo, commitDuration);

            lockedNodeInfo.Commit(move(newNodeInfo));
        }

        return error;
    }

    return CompletedAsyncOperation::End(operation);
}

void NodeCache::UpdateVersionInstanceAsync(NodeInstance const& nodeInstance, FabricVersionInstance const& versionInstance)
{
    LockedNodeInfo lockedNodeInfo;
    ErrorCode error = GetLockedNode(nodeInstance.Id, lockedNodeInfo);
    if (!error.IsSuccess())
    {
        fm_.WriteInfo(
            Constants::NodeSource, wformatString(nodeInstance.Id),
            "UpdateVersionInstance processing for node {0} failed with {1}",
            nodeInstance, error);

        return;
    }

    if (!lockedNodeInfo->IsPendingFabricUpgrade &&
        lockedNodeInfo->Description.VersionInstance == versionInstance)
    {
        return;
    }

    NodeDescription nodeDescription(
        versionInstance,
        lockedNodeInfo->UpgradeDomainId,
        lockedNodeInfo->FaultDomainIds,
        lockedNodeInfo->NodeProperties,
        lockedNodeInfo->NodeCapacityRatios,
        lockedNodeInfo->NodeCapacities,
        lockedNodeInfo->NodeName,
        lockedNodeInfo->NodeType,
        lockedNodeInfo->IpAddressOrFQDN,
        lockedNodeInfo->Description.ClusterConnectionPort,
        lockedNodeInfo->Description.HttpGatewayPort);

    NodeInfoSPtr newNodeInfo = make_shared<NodeInfo>(*lockedNodeInfo);
    newNodeInfo->Description = nodeDescription;
    newNodeInfo->IsPendingFabricUpgrade = false;
    newNodeInfo->PersistenceState = PersistenceState::ToBeUpdated;

    auto lockedNodeInfoMover = MoveUPtr<LockedNodeInfo>(move(make_unique<LockedNodeInfo>(move(lockedNodeInfo))));

    fm_.Store.BeginUpdateData(
        *newNodeInfo,
        [this, newNodeInfo, lockedNodeInfoMover](AsyncOperationSPtr const& operation) mutable
        {
            int64 commitDuration(0);
            ErrorCode error = fm_.Store.EndUpdateData(*newNodeInfo, operation, commitDuration);

            if (error.IsSuccess())
            {
                fm_.WriteInfo(
                    Constants::NodeUpdateSource, newNodeInfo->IdString,
                    "VersionInstance updated: {0}Commit Duration = {1} ms", *newNodeInfo, commitDuration);

                auto lockedNodeInfo = lockedNodeInfoMover.TakeUPtr();
                lockedNodeInfo->Commit(move(newNodeInfo));
            }
            else
            {
                fm_.WriteInfo(
                    Constants::NodeSource, newNodeInfo->IdString,
                    "UpdateVersionInstance update for node {0} failed with {1}", newNodeInfo->NodeInstance, error);
            }
        },
        fm_.CreateAsyncOperationRoot());
}

ErrorCode NodeCache::NodeUp(
    NodeInfoSPtr && node,
    bool isVersionGateKeepingNeeded,
    _Out_ FabricVersionInstance & targetVersionInstance)
{
    NodeInstance nodeInstance = node->NodeInstance;

    bool isNewEntry;
    LockedNodeInfo lockedNodeInfo;
    ErrorCode error = GetLockedNode(nodeInstance.Id, lockedNodeInfo, true, FailoverConfig::GetConfig().LockAcquireTimeout, isNewEntry);
    if (!error.IsSuccess())
    {
        return error;
    }

    if ((lockedNodeInfo->NodeInstance.InstanceId < node->NodeInstance.InstanceId) ||
        (lockedNodeInfo->NodeInstance.InstanceId == node->NodeInstance.InstanceId &&
         lockedNodeInfo->IsUp && !lockedNodeInfo->IsReplicaUploaded && node->IsReplicaUploaded))
    {
        if (isVersionGateKeepingNeeded &&
            fm_.FabricUpgradeManager.IsFabricUpgradeNeeded(node->VersionInstance, node->ActualUpgradeDomainId, targetVersionInstance))
        {
            fm_.WriteWarning(
                Constants::NodeUpSource, wformatString(nodeInstance.Id),
                "NodeUp from {0} ignored due to fabric version mismatch: Current={1}, Incoming={2}",
                nodeInstance.Id, targetVersionInstance, node->VersionInstance);

            if (isNewEntry)
            {
                AcquireWriteLock lock(lock_);

                lockedNodeInfo.IsDeleted = true;
                nodes_.erase(nodeInstance.Id);
            }

            return ErrorCodeValue::FMInvalidRolloutVersion;
        }

        if (lockedNodeInfo->IsUp)
        {
            node->NodeDownTime = DateTime::Now();
        }
        else
        {
            node->NodeDownTime = lockedNodeInfo->NodeDownTime;
        }

        node->NodeUpTime = DateTime::Now();
        node->DeactivationInfo = lockedNodeInfo->DeactivationInfo;

        if (!fm_.IsMaster)
        {
            node->HealthSequence = InterlockedIncrement64(&healthSequence_) - 1;
        }

        if (!isNewEntry)
        {
            node->PersistenceState = PersistenceState::ToBeUpdated;
        }

        int64 commitDuration;
        error = fm_.Store.UpdateData(*node, commitDuration);

        ReportHealthAfterNodeUpdate(node, error);

        if (error.IsSuccess())
        {
            lastNodeUpTime_ = DateTime::Now();

            if (isNewEntry)
            {
                // This is a new Node and we are pushing detailed information about it to Event Store.
                fm_.NodeEvents.NodeAddedOperational(
                    Common::Guid::NewGuid(),
                    node->Description.NodeName,
                    node->IdString,
                    node->NodeInstance.InstanceId,
                    node->NodeType,
                    node->VersionInstance.ToString(),
                    node->IpAddressOrFQDN,
                    wformatString(node->NodeCapacities));

                nodeCount_++;
            }

            if (!fm_.IsMaster)
            {
                fm_.NodeEvents.NodeUp(node->IdString, node->NodeInstance.InstanceId);

                // Push data to Event store.
                fm_.NodeEvents.NodeUpOperational(Common::Guid::NewGuid(), node->Description.NodeName, node->NodeInstance.InstanceId, node->NodeDownTime);
            }

            fm_.NodeEvents.NodeAddedToGFUM(node->Id, node->NodeInstance.InstanceId);

            int64 plbElapsedTime;
            fm_.PLB.UpdateNode(node->GetPLBNodeDescription(), plbElapsedTime);
            fm_.WriteInfo(Constants::NodeUpdateSource, wformatString(nodeInstance.Id), "NodeUp: {0}Commit Duration = {1} ms, PLB Duration = {2} ms", *node, commitDuration, plbElapsedTime);

            AddUpgradeDomain(*node);

            if (!lockedNodeInfo->IsUp)
            {
                uint64 oldUpCount = upCount_++;
                if (!fm_.IsMaster)
                {
                    if (oldUpCount == FailoverConfig::GetConfig().ClusterPauseThreshold - 1)
                    {
                        fm_.Events.ClusterEndPauseState(oldUpCount + 1);
                    }
                    else if (oldUpCount < FailoverConfig::GetConfig().ClusterPauseThreshold - 1)
                    {
                        fm_.Events.ClusterInPauseState(oldUpCount + 1);
                    }
                }
            }

            lockedNodeInfo.Commit(move(node));
        }
        else if (isNewEntry)
        {
            AcquireWriteLock lock(lock_);

            lockedNodeInfo.IsDeleted = true;
            nodes_.erase(nodeInstance.Id);
        }
    }

    if (error.IsSuccess())
    {
        error = fm_.ServiceCacheObj.RemoveFromDisabledNodes(0, nodeInstance.Id);
    }

    return error;
}
        
AsyncOperationSPtr NodeCache::BeginReplicaUploaded(
    NodeInstance const& nodeInstance,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& state)
{
    LockedNodeInfo lockedNodeInfo;
    ErrorCode error = GetLockedNode(nodeInstance.Id, lockedNodeInfo);

    TESTASSERT_IF(error.IsError(ErrorCodeValue::NodeNotFound),
        "{0} not found for ReplicaUploaded", nodeInstance);

    if (!error.IsSuccess())
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, state);
    }

    if (lockedNodeInfo->NodeInstance.InstanceId > nodeInstance.InstanceId || lockedNodeInfo->IsReplicaUploaded)
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCodeValue::Success, callback, state);
    }

    TESTASSERT_IF(
        lockedNodeInfo->NodeInstance.InstanceId != nodeInstance.InstanceId,
        "{0} not match {1} for ReplicaUploaded", nodeInstance, *lockedNodeInfo);

    NodeInfoSPtr newNode = make_shared<NodeInfo>(*lockedNodeInfo);
    newNode->IsReplicaUploaded = true;

    auto newNodeInfoPtr = newNode.get();
    pair<LockedNodeInfo, NodeInfoSPtr> context(move(lockedNodeInfo), move(newNode));

    return fm_.Store.BeginUpdateData(
        *newNodeInfoPtr,
        OperationContextCallback<pair<LockedNodeInfo, NodeInfoSPtr>>(callback, move(context)),
        state);
}

ErrorCode NodeCache::EndReplicaUploaded(AsyncOperationSPtr const& operation)
{
    auto context = operation->PopOperationContext<pair<LockedNodeInfo, NodeInfoSPtr>>();

    if (context)
    {
        LockedNodeInfo & lockedNodeInfo = context->Context.first;
        NodeInfoSPtr & newNode = context->Context.second;

        int64 commitDuration;
        ErrorCode error = fm_.Store.EndUpdateData(*newNode, operation, commitDuration);

        if (error.IsSuccess())
        {
            fm_.WriteInfo(Constants::NodeUpdateSource, newNode->IdString,
                "Replicas uploaded: {0}Commit Duration = {1} ms", *newNode, commitDuration);

            lockedNodeInfo.Commit(move(newNode));
        }

        return error;
    }

    return CompletedAsyncOperation::End(operation);
}

ErrorCode NodeCache::NodeDown(NodeInstance const& nodeInstance)
{
    // First check if an update is needed without locking the NodeInfo.
    // NodeInfo can be stale, but if the incoming message is even staler
    // then we can safely ignore it.
    NodeInfoSPtr nodeInfo = GetNode(nodeInstance.Id);
    if (nodeInfo &&
        (nodeInfo->NodeInstance.InstanceId > nodeInstance.InstanceId ||
         (nodeInfo->NodeInstance.InstanceId == nodeInstance.InstanceId && !nodeInfo->IsUp)))
    {
        return ErrorCodeValue::Success;
    }

    //
    // An upate may be needed. Lock NodeInfo and check again if the update is needed.
    //

    bool isNewEntry;
    LockedNodeInfo lockedNodeInfo;
    ErrorCode error = GetLockedNode(nodeInstance.Id, lockedNodeInfo, true, FailoverConfig::GetConfig().LockAcquireTimeout, isNewEntry);
    if (!error.IsSuccess())
    {
        return error;
    }

    if ((lockedNodeInfo->NodeInstance.InstanceId > nodeInstance.InstanceId) ||
        (!isNewEntry && lockedNodeInfo->NodeInstance.InstanceId == nodeInstance.InstanceId && !lockedNodeInfo->IsUp))
    {
        return ErrorCodeValue::Success;
    }

    NodeInfoSPtr node = make_shared<NodeInfo>(
        nodeInstance,
        NodeDescription(lockedNodeInfo->Description),
        false, // IsUp
        false, // IsReplicaUploaded
        false, // IsNodeStateRemoved
        DateTime::Now()); // LastUpdated

    node->NodeUpTime = lockedNodeInfo->NodeUpTime;
    node->NodeDownTime = DateTime::Now();
    node->DeactivationInfo = lockedNodeInfo->DeactivationInfo;

    if (node->DeactivationInfo.IsActivated)
    {
        node->DeactivationInfo.Status = NodeDeactivationStatus::None;
    }
    else if (!node->DeactivationInfo.IsRemove)
    {
        node->DeactivationInfo.Status = NodeDeactivationStatus::DeactivationComplete;
    }

    if (!fm_.IsMaster)
    {
        node->HealthSequence = InterlockedIncrement64(&healthSequence_) - 1;
    }

    if (!isNewEntry)
    {
        node->PersistenceState = PersistenceState::ToBeUpdated;
    }

    int64 commitDuration;
    error = fm_.Store.UpdateData(*node, commitDuration);

    ReportHealthAfterNodeUpdate(node, error);

    if (error.IsSuccess())
    {
        if (isNewEntry)
        {
            // This is a new Node and we are pushing detailed information about it to Event Store.
            fm_.NodeEvents.NodeAddedOperational(
                Common::Guid::NewGuid(),
                node->Description.NodeName,
                node->IdString,
                node->NodeInstance.InstanceId,
                node->NodeType,
                node->VersionInstance.ToString(),
                node->IpAddressOrFQDN,
                wformatString(node->NodeCapacities));

            nodeCount_++;
        }

        if (!fm_.IsMaster)
        {
            fm_.NodeEvents.NodeDown(node->IdString, node->NodeInstance.InstanceId);

            // Push data to Event Store.
            fm_.NodeEvents.NodeDownOperational(Common::Guid::NewGuid(), node->Description.NodeName, node->NodeInstance.InstanceId, node->NodeUpTime);
        }

        fm_.NodeEvents.NodeRemovedFromGFUM(node->Id, node->NodeInstance.InstanceId);
        
        int64 plbElapsedTime;
        fm_.PLB.UpdateNode(node->GetPLBNodeDescription(), plbElapsedTime);

        fm_.WriteInfo(Constants::NodeUpdateSource, wformatString(nodeInstance.Id), "NodeDown: {0}Commit Duration = {1} ms PLB Duration = {2} ms", node, commitDuration, plbElapsedTime);

        if (lockedNodeInfo->IsUp)
        {
            uint64 oldUpCount = upCount_--;
            if (!fm_.IsMaster && oldUpCount <= FailoverConfig::GetConfig().ClusterPauseThreshold)
            {
                fm_.Events.ClusterInPauseState(oldUpCount - 1);
            }
        }

        lockedNodeInfo.Commit(move(node));
    }
    else if (isNewEntry)
    {
        AcquireWriteLock lock(lock_);

        lockedNodeInfo.IsDeleted = true;
        nodes_.erase(nodeInstance.Id);
    }

    if (error.IsSuccess())
    {
        error = fm_.ServiceCacheObj.RemoveFromDisabledNodes(0, nodeInstance.Id);

        if (fm_.IsReady)
        {
            fm_.BackgroundManagerObj.ScheduleRun();
        }
    }

    if (!error.IsSuccess())
    {
        fm_.WriteWarning(
            Constants::NodeSource, wformatString(nodeInstance.Id),
            "Node down for node {0} failed with {1}", nodeInstance, error);
    }

    return error;
}

AsyncOperationSPtr NodeCache::BeginNodeStateRemoved(
    NodeId const& nodeId,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& state)
{
    LockedNodeInfo lockedNodeInfo;
    ErrorCode error = GetLockedNode(nodeId, lockedNodeInfo);

    ASSERT_IF(
        error.IsError(ErrorCodeValue::NodeNotFound),
        "{0} not found for NodeStateRemoved.", nodeId);

    if (!error.IsSuccess())
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, state);
    }

    if (lockedNodeInfo->IsNodeStateRemoved)
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(callback, state);
    }

    NodeInfoSPtr newNode = make_shared<NodeInfo>(*lockedNodeInfo);
    newNode->IsNodeStateRemoved = true;
    if (!fm_.IsMaster)
    {
        newNode->HealthSequence = InterlockedIncrement64(&healthSequence_) - 1;
    }

    auto newNodePtr = newNode.get();
    pair<LockedNodeInfo, NodeInfoSPtr> context(move(lockedNodeInfo), newNode);

    return fm_.Store.BeginUpdateData(
        *newNodePtr,
        OperationContextCallback<pair<LockedNodeInfo, NodeInfoSPtr>>(callback, move(context)),
        state);
}

ErrorCode NodeCache::EndNodeStateRemoved(AsyncOperationSPtr const& operation)
{
    auto context = operation->PopOperationContext<pair<LockedNodeInfo, NodeInfoSPtr>>();

    if (context)
    {
        LockedNodeInfo & lockedNodeInfo = context->Context.first;
        NodeInfoSPtr & newNode = context->Context.second;

        int64 commitDuration(0);
        ErrorCode error = fm_.Store.EndUpdateData(*newNode, operation, commitDuration);

        ReportHealthAfterNodeUpdate(newNode, error);

        if (error.IsSuccess())
        {
            wstring upgradeDomain = newNode->ActualUpgradeDomainId;

            if (!fm_.IsMaster)
            {
                fm_.NodeEvents.NodeStateRemoved(newNode->IdString, newNode->NodeInstance.InstanceId);
            }

            fm_.WriteInfo(Constants::NodeUpdateSource, newNode->IdString,
                "NodeStateRemoved: {0}Commit Duration = {1} ms", *newNode, commitDuration);

            lockedNodeInfo.Commit(move(newNode));

            // Update the set of upgrade domains
            RemoveUpgradeDomain(upgradeDomain);
        }

        return error;
    }

    return CompletedAsyncOperation::End(operation);
}


UpgradeDomainsCSPtr NodeCache::GetUpgradeDomains(UpgradeDomainSortPolicy::Enum sortPolicy)
{
    {
        AcquireReadLock lock(lock_);

        auto it = upgradeDomains_.find(sortPolicy);
        if (it != upgradeDomains_.end())
        {
            return it->second;
        }
    }

    AcquireWriteLock lock(lock_);

    auto it = upgradeDomains_.find(sortPolicy);
    if (it == upgradeDomains_.end())
    {
        UpgradeDomains const& upgradeDomains = *(upgradeDomains_.begin()->second);
        UpgradeDomainsSPtr newUpgradeDomains = make_shared<UpgradeDomains>(upgradeDomains, sortPolicy);
        
        upgradeDomains_[sortPolicy] = move(newUpgradeDomains);

        return upgradeDomains_[sortPolicy];
    }
    else
    {
        return it->second;
    }
}

void NodeCache::AddUpgradeDomain(NodeInfo const& nodeInfo)
{
    if (nodeInfo.IsUnknown || nodeInfo.IsNodeStateRemoved)
    {
        return;
    }

    AcquireWriteLock grab(lock_);

    if (upgradeDomains_.empty())
    {
        UpgradeDomainSortPolicy::Enum sortPolicy =
            (FailoverConfig::GetConfig().SortUpgradeDomainNamesAsNumbers ? UpgradeDomainSortPolicy::DigitsAsNumbers : UpgradeDomainSortPolicy::Lexicographical);

        UpgradeDomainsSPtr upgradeDomains = make_shared<UpgradeDomains>(sortPolicy);
        upgradeDomains->AddUpgradeDomain(nodeInfo.ActualUpgradeDomainId);

        upgradeDomains_[sortPolicy] = move(upgradeDomains);

        return;
    }

    for (auto it = upgradeDomains_.begin(); it != upgradeDomains_.end(); it++)
    {
        UpgradeDomains & upgradeDomains = *(it->second);
        if (!upgradeDomains.Exists(nodeInfo.ActualUpgradeDomainId))
        {
            UpgradeDomainsSPtr newUpgradeDomains = make_shared<UpgradeDomains>(upgradeDomains);
            newUpgradeDomains->AddUpgradeDomain(nodeInfo.ActualUpgradeDomainId);

            upgradeDomains.IsStale = true;

            it->second = move(newUpgradeDomains);
        }
    }
}

void NodeCache::RemoveUpgradeDomain(wstring const& upgradeDomain)
{
    AcquireWriteLock grab(lock_);

    for (auto const& node : nodes_)
    {
        NodeInfoSPtr const& n = node.second->Get();
        if (!n->IsNodeStateRemoved &&
            n->ActualUpgradeDomainId == upgradeDomain)
        {
            return;
        }
    }

    for (auto it = upgradeDomains_.begin(); it != upgradeDomains_.end(); it++)
    {
        UpgradeDomains & upgradeDomains = *(it->second);
        if (upgradeDomains.Exists(upgradeDomain))
        {
            UpgradeDomainsSPtr newUpgradeDomains = make_shared<UpgradeDomains>(upgradeDomains);
            newUpgradeDomains->RemoveUpgradeDomain(upgradeDomain);

            upgradeDomains.IsStale = true;

            it->second = move(newUpgradeDomains);
        }
    }
}

bool NodeCache::IsNodeUp(NodeInstance const& nodeInstance) const
{
    AcquireReadLock grab(lock_);

    auto it = nodes_.find(nodeInstance.Id);
    if (it != nodes_.end())
    {
        NodeInfoSPtr const& nodeInfo = it->second->Get();
        return (nodeInfo->NodeInstance == nodeInstance && nodeInfo->IsUp);
    }

    return false;
}

void NodeCache::ReportHealthAfterNodeUpdate(NodeInfoSPtr const & nodeInfo, ErrorCode error, bool isUpgrade)
{
    if (fm_.IsMaster)
    {
        return;
    }

    if (error.IsSuccess())
    {
        auto e1 = fm_.HealthClient->AddHealthReport(
            fm_.HealthReportFactoryObj.GenerateNodeInfoHealthReport(
                nodeInfo,
                fm_.Federation.IsSeedNode(nodeInfo->Id),
                isUpgrade));
        fm_.WriteInfo(Constants::NodeSource, wformatString(nodeInfo->Id), "AddHealthReport for node {0} completed with {1}", nodeInfo->Id, e1);
    }
    else
    {
        fm_.HealthClient->HealthSkipSequence(*ServiceModel::Constants::HealthReportFMSource, FABRIC_HEALTH_REPORT_KIND_NODE, nodeInfo->HealthSequence);
    }
}

void NodeCache::ReportHealthForNodeDeactivation(NodeInfoSPtr const & nodeInfo, ErrorCode error, bool isDeactivationComplete)
{
    if (error.IsSuccess())
    {
        auto e1 = fm_.HealthClient->AddHealthReport(
            fm_.HealthReportFactoryObj.GenerateNodeDeactivationHealthReport(
                nodeInfo,
                fm_.Federation.IsSeedNode(nodeInfo->Id),
                isDeactivationComplete));

        fm_.WriteInfo(
            Constants::NodeSource,
            wformatString(nodeInfo->Id),
            "AddHealthReport for node {0} completed with {1}",
            nodeInfo->Id,
            e1);
    }
    else
    {
        fm_.HealthClient->HealthSkipSequence(
            *ServiceModel::Constants::HealthReportFMSource,
            FABRIC_HEALTH_REPORT_KIND_NODE,
            nodeInfo->HealthSequence);
    }
}

ErrorCode NodeCache::SetPendingDeactivateNode(NodeInstance const& nodeInstance, bool value)
{
    LockedNodeInfo lockedNodeInfo;
    ErrorCode error = GetLockedNode(nodeInstance.Id, lockedNodeInfo);

    ASSERT_IF(error.IsError(ErrorCodeValue::NodeNotFound), "{0} not found for SetPendingDeactivateNode", nodeInstance);

    if (!error.IsSuccess())
    {
        return error;
    }

    if (lockedNodeInfo->NodeInstance.InstanceId > nodeInstance.InstanceId)
    {
        return ErrorCodeValue::StaleRequest;
    }

    ASSERT_IF(
        lockedNodeInfo->NodeInstance.InstanceId != nodeInstance.InstanceId,
        "{0} not match {1} for SetPendingDeactivateNode", nodeInstance, *lockedNodeInfo);

    if (lockedNodeInfo->IsPendingDeactivateNode == value)
    {
        return ErrorCodeValue::Success;
    }


    NodeInfoSPtr newNode = make_shared<NodeInfo>(*lockedNodeInfo);
    newNode->IsPendingDeactivateNode = value;

    int64 commitDuration;
    error = fm_.Store.UpdateData(*newNode, commitDuration);
    if (error.IsSuccess())
    {
        fm_.WriteInfo(Constants::NodeUpdateSource, wformatString(nodeInstance.Id), "NodeInfo updated: {0}Commit Duration = {1} ms", *newNode, commitDuration);

        lockedNodeInfo.Commit(move(newNode));
    }

    return error;
}

AsyncOperationSPtr NodeCache::BeginSetPendingFabricUpgrade(
    NodeInfo const& nodeInfo,
    bool value,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& state)
{
    if (nodeInfo.IsPendingFabricUpgrade == value)
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(callback, state);
    }

    LockedNodeInfo lockedNodeInfo;
    ErrorCode error = GetLockedNode(nodeInfo.Id, lockedNodeInfo);

    ASSERT_IF(error.IsError(ErrorCodeValue::NodeNotFound), "Node not found for SetPendingFabricUpgrade {0}: ", nodeInfo);

    if (!error.IsSuccess())
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, state);
    }

    if (lockedNodeInfo->NodeInstance.InstanceId > nodeInfo.NodeInstance.InstanceId)
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCodeValue::StaleRequest, callback, state);
    }

    ASSERT_IF(
        lockedNodeInfo->NodeInstance.InstanceId != nodeInfo.NodeInstance.InstanceId,
        "{0} not match {1} for SetPendingFabricUpgrade", nodeInfo.NodeInstance.InstanceId, *lockedNodeInfo);

    if (lockedNodeInfo->IsPendingFabricUpgrade == value)
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(callback, state);
    }

    NodeInfoSPtr newNodeInfo = make_shared<NodeInfo>(*lockedNodeInfo);
    newNodeInfo->IsPendingFabricUpgrade = value;

    auto newNodeInfoPtr = newNodeInfo.get();

    return fm_.Store.BeginUpdateData(
        *newNodeInfoPtr,
        OperationContextCallback<pair<NodeInfoSPtr, LockedNodeInfo>>(callback, move(make_pair(move(newNodeInfo), move(lockedNodeInfo)))),
        state);
}

ErrorCode NodeCache::EndSetPendingFabricUpgrade(AsyncOperationSPtr const& operation)
{
    unique_ptr<OperationContext<pair<NodeInfoSPtr, LockedNodeInfo>>> context = operation->PopOperationContext<pair<NodeInfoSPtr, LockedNodeInfo>>();

    if (context)
    {
        NodeInfoSPtr & newNodeInfo = context->Context.first;
        LockedNodeInfo & lockedNodeInfo = context->Context.second;

        int64 commitDuration;
        ErrorCode error = fm_.Store.EndUpdateData(*newNodeInfo, operation, commitDuration);
        if (error.IsSuccess())
        {
            fm_.WriteInfo(
                Constants::NodeUpdateSource, newNodeInfo->IdString,
                "NodeInfo updated: {0}Commit Duration = {1} ms", *newNodeInfo, commitDuration);

            lockedNodeInfo.Commit(move(newNodeInfo));
        }

        return error;
    }

    return CompletedAsyncOperation::End(operation);
}

AsyncOperationSPtr NodeCache::BeginAddPendingApplicationUpgrade(
    NodeInfo const& nodeInfo,
    ServiceModel::ApplicationIdentifier const& applicationId,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& state)
{
    if (nodeInfo.IsPendingApplicationUpgrade(applicationId))
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(callback, state);
    }

    LockedNodeInfo lockedNodeInfo;
    ErrorCode error = GetLockedNode(nodeInfo.Id, lockedNodeInfo);

    ASSERT_IF(error.IsError(ErrorCodeValue::NodeNotFound), "Node not found for AddPendingApplicationUpgrade: {0}", nodeInfo);

    if (!error.IsSuccess())
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, state);
    }

    if (lockedNodeInfo->NodeInstance.InstanceId > nodeInfo.NodeInstance.InstanceId)
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCodeValue::StaleRequest, callback, state);
    }

    ASSERT_IF(
        lockedNodeInfo->NodeInstance.InstanceId != nodeInfo.NodeInstance.InstanceId,
        "{0} not match {1} for AddPendingApplicationUpgrade", nodeInfo.NodeInstance.InstanceId, *lockedNodeInfo);

    if (lockedNodeInfo->IsPendingApplicationUpgrade(applicationId))
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(callback, state);
    }

    NodeInfoSPtr newNodeInfo = make_shared<NodeInfo>(*lockedNodeInfo);
    newNodeInfo->AddPendingApplicationUpgrade(applicationId);

    auto newNodeInfoPtr = newNodeInfo.get();

    return fm_.Store.BeginUpdateData(
        *newNodeInfoPtr,
        OperationContextCallback<pair<NodeInfoSPtr, LockedNodeInfo>>(callback, move(make_pair(move(newNodeInfo), move(lockedNodeInfo)))),
        state);
}

ErrorCode NodeCache::EndAddPendingApplicationUpgrade(AsyncOperationSPtr const& operation)
{
    unique_ptr<OperationContext<pair<NodeInfoSPtr, LockedNodeInfo>>> context = operation->PopOperationContext<pair<NodeInfoSPtr, LockedNodeInfo>>();

    if (context)
    {
        NodeInfoSPtr & newNodeInfo = context->Context.first;
        LockedNodeInfo & lockedNodeInfo = context->Context.second;

        int64 commitDuration;
        ErrorCode error = fm_.Store.EndUpdateData(*newNodeInfo, operation, commitDuration);
        if (error.IsSuccess())
        {
            fm_.WriteInfo(
                Constants::NodeUpdateSource, newNodeInfo->IdString,
                "NodeInfo updated: {0}Commit Duration = {1} ms", *newNodeInfo, commitDuration);

            lockedNodeInfo.Commit(move(newNodeInfo));
        }

        return error;
    }

    return CompletedAsyncOperation::End(operation);
}

ErrorCode NodeCache::RemovePendingApplicationUpgrade(NodeInstance const& nodeInstance, ServiceModel::ApplicationIdentifier const& applicationId)
{
    LockedNodeInfo lockedNodeInfo;
    ErrorCode error = GetLockedNode(nodeInstance.Id, lockedNodeInfo);

    ASSERT_IF(error.IsError(ErrorCodeValue::NodeNotFound), "{0} not found for RemovePendingApplicationUpgrade", nodeInstance);

    if (!error.IsSuccess())
    {
        return error;
    }

    if (lockedNodeInfo->NodeInstance.InstanceId > nodeInstance.InstanceId)
    {
        return ErrorCodeValue::StaleRequest;
    }

    ASSERT_IF(
        lockedNodeInfo->NodeInstance.InstanceId != nodeInstance.InstanceId,
        "{0} not match {1} for RemovePendingApplicationUpgrade", nodeInstance, *lockedNodeInfo);

    if (!lockedNodeInfo->IsPendingApplicationUpgrade(applicationId))
    {
        return ErrorCodeValue::Success;
    }


    NodeInfoSPtr newNode = make_shared<NodeInfo>(*lockedNodeInfo);
    newNode->RemovePendingApplicationUpgrade(applicationId);

    int64 commitDuration;
    error = fm_.Store.UpdateData(*newNode, commitDuration);
    if (error.IsSuccess())
    {
        fm_.WriteInfo(Constants::NodeUpdateSource, wformatString(nodeInstance.Id), "NodeInfo updated: {0}Commit Duration = {1} ms", *newNode, commitDuration);

        lockedNodeInfo.Commit(move(newNode));
    }

    return error;
}

void NodeCache::ProcessNodes(NodePeriodicTaskCounts & taskCounts)
{
    vector<LockedNodeInfo> nodesToRemove;

    // Tracing
    vector<NodeInstance> upNodes;
    NodeCounts counts;
    counts.NodeCount = nodes_.size();

    // Health reporting
    DateTime upgradeStartTime;
    bool shouldReportFabricUpgradeHealth = !fm_.IsMaster && fm_.FabricUpgradeManager.GetUpgradeStartTime(upgradeStartTime);
    vector<CacheEntrySPtr> nodesToReportFabricUpgradeHealth;
    vector<CacheEntrySPtr> nodesToReportDeactivationStuck;
    vector<CacheEntrySPtr> nodesToReportDeactivationComplete;

    {
        AcquireReadLock grab(lock_);

        for (auto it = nodes_.begin(); it != nodes_.end(); ++it)
        {
            NodeInfoSPtr nodeInfo = it->second->Get();

            ProcessNodeForCleanup(
                taskCounts,
                nodeInfo,
                it->second,
                nodesToRemove);

            ProcessNodeForActivation(nodeInfo);

            ProcessNodeForCounts(nodeInfo, counts, upNodes);

            if (healthInitialized_)
            {
                ProcessNodeForHealthReporting(
                    nodeInfo,
                    it->second,
                    shouldReportFabricUpgradeHealth,
                    upgradeStartTime,
                    nodesToReportFabricUpgradeHealth,
                    nodesToReportDeactivationStuck,
                    nodesToReportDeactivationComplete);
            }
        }
    }

    RemoveNodesAsync(taskCounts, nodesToRemove);

    TraceNodeCounts(counts, upNodes);

    if (healthInitialized_)
    {
        ReportHealthForNodes(
            taskCounts,
            shouldReportFabricUpgradeHealth,
            nodesToReportFabricUpgradeHealth,
            nodesToReportDeactivationStuck,
            nodesToReportDeactivationComplete);
    }
}

void NodeCache::ProcessNodeForCleanup(
    NodePeriodicTaskCounts & taskCounts,
    NodeInfoSPtr const& nodeInfo,
    CacheEntrySPtr const& cacheEntry,
    vector<LockedNodeInfo> & nodesToRemove) const
{
    bool isDeleteNeeded = (nodeInfo->IsUnknown &&
        DateTime::Now() - nodeInfo->LastUpdated >= FailoverConfig::GetConfig().UnknownNodeKeepDuration);

    isDeleteNeeded |= (nodeInfo->IsNodeStateRemoved &&
        DateTime::Now() - nodeInfo->LastUpdated >= FailoverConfig::GetConfig().RemovedNodeKeepDuration);

    if (isDeleteNeeded && nodeInfo->HealthSequence + 1 < ackedSequence_)
    {
        LockedNodeInfo lockedNodeInfo;
        ErrorCode error = cacheEntry->Lock(cacheEntry, lockedNodeInfo, TimeSpan::Zero);
        if (error.IsSuccess() && lockedNodeInfo->OperationLSN == nodeInfo->OperationLSN)
        {
            ASSERT_IF(nodeInfo->IsUp, "An unknown/removed node should be down: {0}", *lockedNodeInfo);

            nodesToRemove.push_back(move(lockedNodeInfo));

            taskCounts.NodesToRemove++;
        }
    }
}

void NodeCache::ProcessNodeForActivation(NodeInfoSPtr const& nodeInfo) const
{
    if (nodeInfo->IsUp && nodeInfo->DeactivationInfo.IsActivationInProgress)
    {
        ASSERT_IF(nodeInfo->DeactivationInfo.IsDeactivated, "A node cannot be in a deactivated state while activation is in progress: {0}", *nodeInfo);

        // Send NodeActivateRequest message to RA
        NodeActivateRequestMessageBody body(nodeInfo->DeactivationInfo.SequenceNumber, fm_.IsMaster);
        Transport::MessageUPtr request = RSMessage::GetNodeActivateRequest().CreateMessage(move(body));
        GenerationHeader header(fm_.Generation, fm_.IsMaster);
        request->Headers.Add(header);

        fm_.SendToNodeAsync(move(request), nodeInfo->NodeInstance);
    }
}

void NodeCache::RemoveNodesAsync(
    NodePeriodicTaskCounts & taskCounts,
    vector<LockedNodeInfo> & nodesToRemove)
{
    for (auto it = nodesToRemove.begin(); it != nodesToRemove.end(); ++it)
    {
        LockedNodeInfo & lockedNodeInfo = *it;
        lockedNodeInfo->PersistenceState = PersistenceState::ToBeDeleted;

        auto mover = MoveUPtr<LockedNodeInfo>(move(make_unique<LockedNodeInfo>(move(lockedNodeInfo))));
        NodeInfo & nodeInfo = *mover->Get();

        fm_.Store.BeginUpdateData(
            nodeInfo,
            [this, mover](AsyncOperationSPtr const& updateOperation) mutable
            {
                int64 commitDuration;
                ErrorCode error = fm_.Store.EndUpdateData(*mover->Get(), updateOperation, commitDuration);
                if (error.IsSuccess())
                {
                    RemoveNodeCommitJobItemUPtr commitJobItem = make_unique<RemoveNodeCommitJobItem>(move(*mover.TakeUPtr()), commitDuration);
                    fm_.CommitQueue.Enqueue(move(commitJobItem));
                }
            },
            fm_.CreateAsyncOperationRoot());

        taskCounts.NodesRemovesStarted++;
    }
}

void NodeCache::ProcessNodeForCounts(
    NodeInfoSPtr const& nodeInfo,
    NodeCounts & counts,
    vector<NodeInstance> & upNodes) const
{
    if (nodeInfo->DeactivationInfo.IsDeactivated || nodeInfo->IsPendingUpgrade)
    {
        fm_.WriteInfo(Constants::NodeStateSource, wformatString(nodeInfo->Id), "{0}", *nodeInfo);
    }

    if (nodeInfo->IsUp)
    {
        counts.UpNodeCount++;

        if (fm_.BackgroundManagerObj.IsStateTraceEnabled)
        {
            upNodes.push_back(nodeInfo->NodeInstance);
        }
    }
    else
    {
        counts.DownNodeCount++;
    }

    if (nodeInfo->DeactivationInfo.IsDeactivated)
    {
        counts.DeactivatedCount++;

        if (nodeInfo->DeactivationInfo.Intent == NodeDeactivationIntent::Pause)
        {
            counts.PauseIntentCount++;
        }
        if (nodeInfo->DeactivationInfo.Intent == NodeDeactivationIntent::Restart)
        {
            counts.RestartIntentCount++;
        }
        if (nodeInfo->DeactivationInfo.Intent == NodeDeactivationIntent::RemoveData)
        {
            counts.RemoveDataIntentCount++;
        }
        if (nodeInfo->DeactivationInfo.Intent == NodeDeactivationIntent::RemoveNode)
        {
            counts.RemoveNodeIntentCount++;
        }
    }

    if (nodeInfo->IsUnknown)
    {
        counts.UnknownCount++;
    }

    if (nodeInfo->IsNodeStateRemoved)
    {
        counts.RemovedNodeCount++;
    }

    if (nodeInfo->IsPendingDeactivateNode)
    {
        counts.PendingDeactivateNode++;
    }

    if (nodeInfo->IsPendingFabricUpgrade)
    {
        counts.PendingFabricUpgrade++;
    }
}

void NodeCache::TraceNodeCounts(
    NodeCounts const& counts,
    vector<NodeInstance> const& upNodes) const
{
    fm_.NodeEvents.NodeCounts(counts);

    if (fm_.BackgroundManagerObj.IsStateTraceEnabled)
    {
        int32 total = static_cast<int>(upNodes.size());
        const int32 nodesPerRecord = 50;
        int32 first = 0;

        while (first < total)
        {
            int32 last = min(first + nodesPerRecord, total);

            string result;
            StringWriterA writer(result);

            int32 index = first;
            writer.Write(upNodes[index]);

            for (++index; index < last; ++index)
            {
                writer.Write(", ");
                writer.Write(upNodes[index]);
            }

            fm_.Events.Nodes(result, first + 1, last, total);
            first = last;
        }
    }
}

void NodeCache::ProcessNodeForHealthReporting(
    NodeInfoSPtr const& nodeInfo,
    CacheEntrySPtr const& cacheEntry,
    bool shouldReportFabricUpgradeHealth,
    DateTime const& upgradeStartTime,
    vector<CacheEntrySPtr> & nodesToReportFabricUpgradeHealth,
    vector<CacheEntrySPtr> & nodesToReportDeactivationStuck,
    vector<CacheEntrySPtr> & nodesToReportDeactivationComplete) const
{
    if (shouldReportFabricUpgradeHealth &&
        !nodeInfo->IsUp &&
        !nodeInfo->IsNodeHealthReportedDuringUpgrade &&
        nodeInfo->LastUpdated > upgradeStartTime &&
        nodeInfo->LastUpdated + FailoverConfig::GetConfig().ExpectedNodeFabricUpgradeDuration < DateTime::Now())
    {
        nodesToReportFabricUpgradeHealth.push_back(cacheEntry);
    }

    if (nodeInfo->IsNodeHealthReportedDuringDeactivate)
    {
        if (nodeInfo->DeactivationInfo.Status == NodeDeactivationStatus::None ||
            nodeInfo->DeactivationInfo.Status == NodeDeactivationStatus::ActivationInProgress ||
            nodeInfo->DeactivationInfo.Status == NodeDeactivationStatus::DeactivationComplete)
        {
            nodesToReportDeactivationComplete.push_back(cacheEntry);
        }
    }
    else
    {
        if (nodeInfo->DeactivationInfo.Status != NodeDeactivationStatus::None &&
            nodeInfo->DeactivationInfo.Status != NodeDeactivationStatus::ActivationInProgress &&
            nodeInfo->DeactivationInfo.Status != NodeDeactivationStatus::DeactivationComplete &&
            nodeInfo->DeactivationInfo.StartTime + FailoverConfig::GetConfig().ExpectedNodeDeactivationDuration < DateTime::Now())
        {
            nodesToReportDeactivationStuck.push_back(cacheEntry);
        }
    }
}

void NodeCache::ReportHealthForNodes(
    NodePeriodicTaskCounts & taskCounts,
    bool shouldReportFabricUpgradeHealth,
    vector<CacheEntrySPtr> & nodesToReportFabricUpgradeHealth,
    vector<CacheEntrySPtr> & nodesToReportDeactivationStuck,
    vector<CacheEntrySPtr> & nodesToReportDeactivationComplete)
{

    if (shouldReportFabricUpgradeHealth)
    {
        for (auto it = nodesToReportFabricUpgradeHealth.begin(); it != nodesToReportFabricUpgradeHealth.end(); ++it)
        {
            LockedNodeInfo lockedNode;
            ErrorCode error = (*it)->Lock(*it, lockedNode, TimeSpan::Zero);
            if (error.IsSuccess() && !lockedNode->IsUp && !lockedNode->IsNodeHealthReportedDuringUpgrade)
            {
                NodeInfoSPtr node = make_shared<NodeInfo>(*lockedNode);
                node->HealthSequence = InterlockedIncrement64(&healthSequence_) - 1;
                node->IsNodeHealthReportedDuringUpgrade = true;
                node->PersistenceState = PersistenceState::ToBeUpdated;

                int64 commitDuration;
                error = fm_.Store.UpdateData(*node, commitDuration);

                ReportHealthAfterNodeUpdate(node, error, true);

                if (!error.IsSuccess())
                {
                    fm_.WriteWarning(
                        Constants::NodeSource, node->IdString,
                        "Health report during fabric upgrade for node {0} failed with {1}\r\nCommit Duration = {1} ms", node->Id, error, commitDuration);
                    return;
                }

                fm_.WriteInfo(Constants::NodeUpdateSource, node->IdString, "Health reported during fabric upgrade: {0}Commit Duration = {1} ms", *node, commitDuration);

                taskCounts.UpgradeHealthReports++;

                lockedNode.Commit(move(node));
            }
        }
    }

    // Health reporting for node deactivation
    for (auto it = nodesToReportDeactivationStuck.begin(); it != nodesToReportDeactivationStuck.end(); ++it)
    {
        ReportNodeDeactivationHealth(taskCounts, *it, false);
    }
    for (auto it = nodesToReportDeactivationComplete.begin(); it != nodesToReportDeactivationComplete.end(); ++it)
    {
        ReportNodeDeactivationHealth(taskCounts, *it, true);
    }
}

void NodeCache::ReportNodeDeactivationHealth(
    NodePeriodicTaskCounts & taskCounts,
    CacheEntrySPtr & cacheEntry,
    bool isDeactivationComplete)
{
    LockedNodeInfo lockedNode;
    ErrorCode error = cacheEntry->Lock(cacheEntry, lockedNode, TimeSpan::Zero);
    if (error.IsSuccess())
    {
        NodeInfoSPtr node = make_shared<NodeInfo>(*lockedNode);
        node->HealthSequence = InterlockedIncrement64(&healthSequence_) - 1;
        node->IsNodeHealthReportedDuringDeactivate = !isDeactivationComplete;
        node->PersistenceState = PersistenceState::ToBeUpdated;

        int64 commitDuration;
        error = fm_.Store.UpdateData(*node, commitDuration);

        ReportHealthForNodeDeactivation(node, error, isDeactivationComplete);

        if (!error.IsSuccess())
        {
            fm_.WriteWarning(
                Constants::NodeSource,
                node->IdString,
                "Health report during deactivation for node {0} failed with {1}\r\nCommit Duration = {1} ms",
                node->Id,
                error,
                commitDuration);

            return;
        }

        fm_.WriteInfo(
            Constants::NodeUpdateSource,
            node->IdString,
            "Health reported during node deactivation: {0}Commit Duration = {1} ms",
            *node,
            commitDuration);

        if (isDeactivationComplete)
        {
            taskCounts.DeactivationCompleteHealthReports++;
        }
        else
        {
            taskCounts.DeactivationStuckHealthReports++;
        }

        lockedNode.Commit(move(node));
    }
}

void NodeCache::FinishRemoveNode(LockedNodeInfo & lockedNodeInfo, int64 commitDuration)
{
    AcquireWriteLock grab(lock_);

    lockedNodeInfo.IsDeleted = true;
    nodes_.erase(lockedNodeInfo->Id);
    nodeCount_--;

    // Push to Operational store.
    fm_.NodeEvents.NodeRemovedOperational(
        Common::Guid::NewGuid(),
        lockedNodeInfo->Description.NodeName,
        lockedNodeInfo->IdString,
        lockedNodeInfo->NodeInstance.InstanceId,
        lockedNodeInfo->NodeType,
        lockedNodeInfo->VersionInstance.ToString(),
        lockedNodeInfo->IpAddressOrFQDN,
        wformatString(lockedNodeInfo->NodeCapacities));

    fm_.WriteInfo(Constants::NodeUpdateSource, lockedNodeInfo->IdString, "Node removed: {0}\r\nCommit Duration = {1} ms", *lockedNodeInfo, commitDuration);
}

void NodeCache::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    for (auto const& pair : nodes_)
    {
        writer.Write("{0} ", *pair.second->Get());
    }
}

ErrorCode NodeCache::GetLockedNode(NodeId const& nodeId, LockedNodeInfo & lockedNodeInfo, TimeSpan timeout)
{
    bool isNewEntry = false;
    return GetLockedNode(nodeId, lockedNodeInfo, false, timeout, isNewEntry);
}

ErrorCode NodeCache::GetLockedNode(
    NodeId const& nodeId,
    LockedNodeInfo & lockedNodeInfo,
    bool createNewEntry,
    TimeSpan timeout,
    __out bool & isNewEntry)
{
    isNewEntry = false;

    CacheEntrySPtr entry;
    {
        // No need to acquire write lock when the node already exists, or
        // when we are not adding a new node.
        AcquireReadLock grab(lock_);

        auto it = nodes_.find(nodeId);
        if (it != nodes_.end())
        {
            entry = it->second;
        }
        else if (!createNewEntry)
        {
            return ErrorCodeValue::NodeNotFound;
        }
    }

    if (entry)
    {
        ErrorCode error = entry->Lock(entry, lockedNodeInfo, timeout);
        if (!createNewEntry || !error.IsError(ErrorCodeValue::NodeNotFound))
        {
            return error;
        }
    }

    AcquireWriteLock grab(lock_);

    // We need to check again for the existence of the node because it may have been created by a different thread.
    auto it = nodes_.find(nodeId);
    if (it == nodes_.end())
    {
        TESTASSERT_IFNOT(createNewEntry, "createNewEntry is not true");

        NodeInfoSPtr nodeInfo = make_shared<NodeInfo>(NodeInstance(nodeId, 0), NodeDescription(nodeId), false, false, false, DateTime::Now());
        CacheEntrySPtr entry1 = make_shared<CacheEntry<NodeInfo>>(move(nodeInfo));
        it = nodes_.insert(make_pair(nodeId, move(entry1))).first;

        isNewEntry = true;
    }

    // We use timeout of zero trying to lock node entry under write lock.
    return it->second->Lock(it->second, lockedNodeInfo, TimeSpan::Zero);
}

void NodeCache::SortUpgradeDomains(UpgradeDomainSortPolicy::Enum sortPolicy, __inout std::vector<std::wstring> & upgradeDomains)
{
    sort(
        upgradeDomains.begin(),
        upgradeDomains.end(),
        [sortPolicy](wstring const& upgradeDomain1, wstring const& upgradeDomain2)
        {
            return CompareUpgradeDomains(sortPolicy, upgradeDomain1, upgradeDomain2);
        });
}

bool NodeCache::CompareUpgradeDomains(UpgradeDomainSortPolicy::Enum sortPolicy, wstring const& upgradeDomain1, wstring const& upgradeDomain2)
{
    switch (sortPolicy)
    {
    case UpgradeDomainSortPolicy::DigitsAsNumbers:
        return (StringUtility::CompareDigitsAsNumbers(upgradeDomain1.c_str(), upgradeDomain2.c_str()) < 0);

    case UpgradeDomainSortPolicy::Lexicographical:
        return (upgradeDomain1 < upgradeDomain2);

    default:
        Assert::TestAssert("Unknown UpgradeDomainSortPolicy: {0}", static_cast<int>(sortPolicy));
        return (upgradeDomain1 < upgradeDomain2);
    }
}

