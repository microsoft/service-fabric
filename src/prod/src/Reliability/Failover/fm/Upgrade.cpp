// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;
using namespace ServiceModel;
using namespace std;

template <typename TDescription>
Upgrade<TDescription>::Upgrade()
    : lastUpgradeMessageSendTime_(DateTime::Zero)
    , sortPolicy_(UpgradeDomainSortPolicy::Lexicographical)
    , isRollback_(false)
{
}

template <typename TDescription>
Upgrade<TDescription>::Upgrade(
    TDescription && description,
    TimeSpan upgradeReplicaSetCheckTimeout,
    UpgradeDomainsCSPtr const& upgradeDomains,
    int64 sequenceNumber,
    bool isRollback,
    bool isPLBSafetyCheckDone,
    TimeSpan plbSafetyCheckTimeout)
    : description_(move(description)),
      upgradeReplicaSetCheckTimeout_(upgradeReplicaSetCheckTimeout),
      upgradeDomains_(upgradeDomains),
      sequenceNumber_(sequenceNumber),
      isRollback_(isRollback),
      currentDomainIndex_(0),
      lastUpgradeMessageSendTime_(DateTime::Zero),
      isUpgradePerformedOnCurrentUD_(false),
      skippedUDs_(),
      sortPolicy_(FailoverConfig::GetConfig().SortUpgradeDomainNamesAsNumbers ? UpgradeDomainSortPolicy::DigitsAsNumbers : UpgradeDomainSortPolicy::Lexicographical),
      isPLBSafetyCheckDone_(isPLBSafetyCheckDone),
      plbSafetyCheckTimeout_(plbSafetyCheckTimeout)
{
    currentDomain_ = upgradeDomains_->GetUpgradeDomain(currentDomainIndex_);
    clusterStartedTime_ = currentDomainStartedTime_ = DateTime::Now();
}

template <typename TDescription>
Upgrade<TDescription>::Upgrade(Upgrade<TDescription> && other)
    : description_(move(other.description_)),
      currentDomain_(move(other.currentDomain_)),
      clusterStartedTime_(other.clusterStartedTime_),
      currentDomainStartedTime_(other.currentDomainStartedTime_),
    upgradeReplicaSetCheckTimeout_(other.upgradeReplicaSetCheckTimeout_),
    upgradeDomains_((other.upgradeDomains_)),
    sequenceNumber_(other.sequenceNumber_),
    isRollback_(other.isRollback_),
    currentDomainIndex_(other.currentDomainIndex_),
    completedNodes_(move(other.completedNodes_)),
    lastUpgradeMessageSendTime_(move(other.lastUpgradeMessageSendTime_)),
    nodeUpgradeProgressList_(move(other.nodeUpgradeProgressList_)),
    isUpgradePerformedOnCurrentUD_(other.isUpgradePerformedOnCurrentUD_),
    skippedUDs_(other.skippedUDs_),
    sortPolicy_(other.sortPolicy_),
      isPLBSafetyCheckDone_(other.isPLBSafetyCheckDone_),
      plbSafetyCheckTimeout_(other.plbSafetyCheckTimeout_)
{
}

template <typename TDescription>
Upgrade<TDescription> & Upgrade<TDescription>::operator = (Upgrade<TDescription> && other)
{
    if (this != &other)
    {
        description_ = move(other.description_);
        currentDomain_ = move(other.currentDomain_);
        clusterStartedTime_ = other.clusterStartedTime_;
        currentDomainStartedTime_ = other.currentDomainStartedTime_;
        upgradeReplicaSetCheckTimeout_ = other.upgradeReplicaSetCheckTimeout_;
        upgradeDomains_ = move(other.upgradeDomains_);
        sequenceNumber_ = other.sequenceNumber_;
        isRollback_ = other.isRollback_;
        currentDomainIndex_ = other.currentDomainIndex_;
        completedNodes_ = move(other.completedNodes_);
        lastUpgradeMessageSendTime_ = move(other.lastUpgradeMessageSendTime_);
        nodeUpgradeProgressList_ = move(other.nodeUpgradeProgressList_);
        isUpgradePerformedOnCurrentUD_ = other.isUpgradePerformedOnCurrentUD_;
        skippedUDs_ = move(other.skippedUDs_);
        sortPolicy_ = other.sortPolicy_;
        isPLBSafetyCheckDone_ = other.isPLBSafetyCheckDone_;
        plbSafetyCheckTimeout_ = other.plbSafetyCheckTimeout_;
    }

    return *this;
}

template <typename TDescription>
void Upgrade<TDescription>::UpdateDomains(UpgradeDomainsCSPtr const& upgradeDomains)
{
    upgradeDomains_ = upgradeDomains;

    currentDomainIndex_ = upgradeDomains->GetUpgradeDomainIndex(currentDomain_);
    currentDomain_ = upgradeDomains->GetUpgradeDomain(currentDomainIndex_);
}

template <typename TDescription>
bool Upgrade<TDescription>::IsReplicaSetWaitInEffect() const
{
    return (DateTime::Now() - currentDomainStartedTime_ < upgradeReplicaSetCheckTimeout_);
}

template <typename TDescription>
bool Upgrade<TDescription>::IsReplicaSetCheckNeeded() const
{
    return ((description_.UpgradeType == UpgradeType::Rolling ||
        description_.UpgradeType == UpgradeType::Rolling_ForceRestart) &&
        IsReplicaSetWaitInEffect());
}

template <typename TDescription>
bool Upgrade<TDescription>::IsCompleted() const
{
    return upgradeDomains_->IsUpgradeComplete(currentDomainIndex_);
}

template <typename TDescription>
bool Upgrade<TDescription>::IsCurrentDomainStarted() const
{
    return (currentDomainStartedTime_ != DateTime::Zero);
}

template <typename TDescription>
bool Upgrade<TDescription>::IsDomainStarted(wstring const& upgradeDomain) const
{
    return (currentDomain_ == upgradeDomain && IsCurrentDomainStarted());
}

template <typename TDescription>
bool Upgrade<TDescription>::IsUpgradeCompletedOnNode(NodeInfo const& nodeInfo) const
{
    return (IsCompleted() ||
        NodeCache::CompareUpgradeDomains(sortPolicy_, nodeInfo.ActualUpgradeDomainId, currentDomain_) ||
        completedNodes_.find(nodeInfo.Id) != completedNodes_.end());
}

template <typename TDescription>
bool Upgrade<TDescription>::CheckUpgradeDomain(wstring const & upgradeDomain) const
{
    bool result = (IsCompleted() ||
        NodeCache::CompareUpgradeDomains(sortPolicy_, upgradeDomain, currentDomain_) ||
        ((upgradeDomain == currentDomain_) && IsCurrentDomainStarted()));

    return result;
}

template <typename TDescription>
set<wstring> Upgrade<TDescription>::GetCompletedDomains() const
{
    return upgradeDomains_->GetCompletedUpgradeDomains(currentDomainIndex_);
}

template <typename TDescription>
void Upgrade<TDescription>::GetProgress(UpgradeReplyMessageBody & replyBody) const
{
    vector<wstring> completed;
    vector<wstring> pending;

    upgradeDomains_->GetProgress(currentDomainIndex_, completed, pending);

    replyBody.SetCompletedUpgradeDomains(move(completed));
    replyBody.SetPendingUpgradeDomain(move(pending));
    replyBody.SetSkippedUpgradeDomains(skippedUDs_);

    UpgradeDomainProgress currentUpgradeDomainProgress(currentDomain_, nodeUpgradeProgressList_);
    replyBody.SetUpgradeProgressDetails(move(currentUpgradeDomainProgress));
}

template <typename TDescription>
bool Upgrade<TDescription>::UpdateProgress(
    FailoverManager & fm,
    map<NodeId, pair<NodeUpgradeProgress, NodeInfoSPtr>> const& readyNodes,
    bool isUpgradeNeededOnCurrentUD,
    bool isComplete)
{
    DateTime now = DateTime::Now();
    FailoverConfig const& config = FailoverConfig::GetConfig();
    TimeSpan waitInterval = (now - currentDomainStartedTime_ > config.MaxActionRetryIntervalPerReplica ? config.MaxActionRetryIntervalPerReplica : config.MinActionRetryIntervalPerReplica);

    bool sendUpgradeMessage = (lastUpgradeMessageSendTime_ < now - waitInterval);

    if (sendUpgradeMessage)
    {
        for (auto readyNode = readyNodes.begin(); readyNode != readyNodes.end(); ++readyNode)
        {
            NodeInfoSPtr const& nodeInfo = readyNode->second.second;
            Transport::MessageUPtr message = CreateUpgradeRequestMessage();
            auto messageMover = MoveUPtr<Transport::Message>(move(message));

            TrySetPendingTraits<TDescription>::BeginTrySetPending(
                fm,
                *nodeInfo,
                description_,
                [&fm, messageMover, nodeInfo](AsyncOperationSPtr const& operation) mutable
                {
                    bool result = TrySetPendingTraits<TDescription>::EndTrySetPending(operation, fm);
                    if (result)
                    {
                        Transport::MessageUPtr message = messageMover.TakeUPtr();

                        GenerationHeader header(fm.Generation, false);
                        message->Headers.Add(header);

                        fm.SendToNodeAsync(move(message), nodeInfo->NodeInstance);
                    }

                },
                fm.CreateAsyncOperationRoot());
        }

        lastUpgradeMessageSendTime_ = now;
    }

    bool isUpdated = false;

    if (isComplete)
    {
        completedNodes_.clear();
        currentDomainIndex_++;

        isUpgradePerformedOnCurrentUD_ |= isUpgradeNeededOnCurrentUD;

        if (description_.IsMonitored && (description_.IsManual || isUpgradePerformedOnCurrentUD_))
        {
            // Wait for UD verification
            currentDomainStartedTime_ = DateTime::Zero;
        }
        else
        {
            // Start the next UD
            currentDomainStartedTime_ = DateTime::Now();
        }

        if (!isUpgradePerformedOnCurrentUD_)
        {
            skippedUDs_.push_back(currentDomain_);
        }

        if (upgradeDomains_->IsUpgradeComplete(currentDomainIndex_))
        {
            fm.WriteInfo(
                TraceSource, TraceType, "{0} last domain {1}",
                isUpgradePerformedOnCurrentUD_ ? "Completed" : "Skipped",
                currentDomain_);

            currentDomain_ = L"";
        }
        else
        {
            fm.WriteInfo(
                TraceSource, TraceType, "{0} {1} and moving to {2}",
                isUpgradePerformedOnCurrentUD_ ? "Completed" : "Skipped",
                currentDomain_,
                upgradeDomains_->GetUpgradeDomain(currentDomainIndex_));

            currentDomain_ = upgradeDomains_->GetUpgradeDomain(currentDomainIndex_);
        }

        isUpgradePerformedOnCurrentUD_ = false;
        isUpdated = true;
    }
    else if (!isUpgradePerformedOnCurrentUD_ && isUpgradeNeededOnCurrentUD)
    {
        isUpgradePerformedOnCurrentUD_ = true;
        isUpdated = true;
    }

    return isUpdated;
}

template <typename TDescription>
bool Upgrade<TDescription>::UpdateUpgrade(
    FailoverManager & fm,
    TDescription && description,
    TimeSpan upgradeReplicaSetCheckTimeout,
    vector<wstring> && verifiedDomains,
    int64 sequenceNumber)
{
    bool result = false;

    if (sequenceNumber > sequenceNumber_)
    {
        description_ = move(description);
        upgradeReplicaSetCheckTimeout_ = upgradeReplicaSetCheckTimeout;
        sequenceNumber_ = sequenceNumber;

        if (!description_.IsMonitored && !IsCurrentDomainStarted())
        {
            currentDomainStartedTime_ = DateTime::Now();
        }

        result = true;
    }

    NodeCache::SortUpgradeDomains(sortPolicy_, verifiedDomains);

    result |= UpdateVerifiedDomains(fm, verifiedDomains);

    return result;
}

template <typename TDescription>
void Upgrade<TDescription>::UpdateProgressDetails(
    map<NodeId, NodeUpgradeProgress> && pendingNodes,
    map<NodeId, pair<NodeUpgradeProgress, NodeInfoSPtr>> && readyNodes,
    map<NodeId, NodeUpgradeProgress> && waitingNodes,
    bool isComplete)
{
    nodeUpgradeProgressList_.clear();

    if (!isComplete)
    {
        for (auto const& upgradeProgress : readyNodes)
        {
            nodeUpgradeProgressList_.push_back(upgradeProgress.second.first);
        }

        for (auto const& upgradeProgress : pendingNodes)
        {
            nodeUpgradeProgressList_.push_back(upgradeProgress.second);
        }

        for (auto const& upgradeProgress : waitingNodes)
        {
            nodeUpgradeProgressList_.push_back(upgradeProgress.second);
        }
    }
}

template <typename TDescription>
bool Upgrade<TDescription>::UpdateVerifiedDomains(FailoverManager & fm, vector<wstring> const & verifiedDomains)
{
    if (verifiedDomains.size() == 0 || IsCompleted() || IsCurrentDomainStarted())
    {
        return false;
    }

    if (currentDomainIndex_ > 0)
    {
        wstring const & lastVerifiedDomain = *(verifiedDomains.rbegin());
        wstring const & lastCompletedDomain = upgradeDomains_->GetUpgradeDomain(currentDomainIndex_ - 1);
        if (NodeCache::CompareUpgradeDomains(sortPolicy_, lastVerifiedDomain, lastCompletedDomain))
        {
            return false;
        }
    }

    currentDomainStartedTime_ = DateTime::Now();

    fm.WriteInfo(TraceSource, TraceType, "Starting upgrade on {0}", currentDomain_);

    return true;
}

template <typename TDescription>
void Upgrade<TDescription>::AddCompletedNode(NodeId nodeId)
{
    completedNodes_.insert(nodeId);
}

template <typename TDescription>
void Upgrade<TDescription>::WriteTo(TextWriter& w, FormatOptions const&) const
{
    w.WriteLine(description_);
    w.WriteLine("IsRollback={0}", isRollback_);
    w.WriteLine("UDs={0}", upgradeDomains_);
    w.WriteLine("SkippedUDs={0}", skippedUDs_);
    w.WriteLine("CurrentUD={0}, {1}", currentDomain_, isUpgradePerformedOnCurrentUD_);
    w.WriteLine("SortPolicy={0}", sortPolicy_);
    w.Write("{0}/{1}/{2} {3}", clusterStartedTime_, currentDomainStartedTime_, upgradeReplicaSetCheckTimeout_, sequenceNumber_);
}

FabricUpgrade::FabricUpgrade()
    : persistenceState_(PersistenceState::NoChange)
{
}

FabricUpgrade::FabricUpgrade(
    FabricUpgradeSpecification && specification,
    TimeSpan upgradeReplicaSetCheckTimeout,
    UpgradeDomainsCSPtr const& upgradeDomains,
    int64 sequenceNumber,
    bool isRollback)
    : Upgrade(move(specification), upgradeReplicaSetCheckTimeout, upgradeDomains, sequenceNumber, isRollback),
      persistenceState_(PersistenceState::ToBeInserted)
{
}

FabricUpgrade::FabricUpgrade(FabricUpgrade && other)
    : Upgrade(move(other)),
      persistenceState_(other.PersistenceState)
{
}

StringLiteral const& FabricUpgrade::get_TraceSource() const
{
    return FailoverManager::TraceFabricUpgrade;
}

wstring FabricUpgrade::get_TraceType() const
{
    return wstring();
}

unique_ptr<FabricUpgradeContext> FabricUpgrade::CreateContext(wstring const& fmId) const
{
    if (!upgradeDomains_->IsUpgradeComplete(currentDomainIndex_) && !IsCurrentDomainStarted())
    {
        return nullptr;
    }

    return make_unique<FabricUpgradeContext>(fmId, *this, currentDomain_);
}

Transport::MessageUPtr FabricUpgrade::CreateUpgradeRequestMessage() const
{
    return RSMessage::GetNodeFabricUpgradeRequest().CreateMessage(description_);
}

ApplicationUpgrade::ApplicationUpgrade()
{
}

ApplicationUpgrade::ApplicationUpgrade(
    UpgradeDescription && description,
    TimeSpan upgradeReplicaSetCheckTimeout,
    UpgradeDomainsCSPtr const& upgradeDomains,
    int64 sequenceNumber,
    bool isRollback,
    bool isPLBSafetyCheckDone,
    Common::TimeSpan plbSafetyCheckTimeout)
    : Upgrade(move(description), upgradeReplicaSetCheckTimeout, upgradeDomains, sequenceNumber, isRollback, isPLBSafetyCheckDone, plbSafetyCheckTimeout)
{
    isPLBSafetyCheckDone_ = isPLBSafetyCheckDone;
}

ApplicationUpgrade::ApplicationUpgrade(ApplicationUpgrade const & other, bool isPLBSafetyCheckDone)
    : Upgrade(other)
{
    isPLBSafetyCheckDone_ = isPLBSafetyCheckDone;
}

ApplicationUpgrade::ApplicationUpgrade(ApplicationUpgrade && other)
    : Upgrade(move(other))
{
}

StringLiteral const& ApplicationUpgrade::get_TraceSource() const
{
    return FailoverManager::TraceApplicationUpgrade;
}

wstring ApplicationUpgrade::get_TraceType() const
{
    return wformatString(description_.ApplicationId);
}

bool ApplicationUpgrade::get_IsPLBSafetyCheckDone() const
{
    return isPLBSafetyCheckDone_ || DateTime::Now() - clusterStartedTime_ >= plbSafetyCheckTimeout_;
}

unique_ptr<ApplicationUpgradeContext> ApplicationUpgrade::CreateContext(ApplicationInfoSPtr const & application) const
{
    if (!upgradeDomains_->IsUpgradeComplete(currentDomainIndex_) && !IsCurrentDomainStarted())
    {
        return nullptr;
    }

    return make_unique<ApplicationUpgradeContext>(application, currentDomain_);
}

Transport::MessageUPtr ApplicationUpgrade::CreateUpgradeRequestMessage() const
{
    return RSMessage::GetNodeUpgradeRequest().CreateMessage(description_);
}

AsyncOperationSPtr TrySetPendingTraits<FabricUpgradeSpecification>::BeginTrySetPending(
    FailoverManager const& fm,
    NodeInfo const& nodeInfo,
    FabricUpgradeSpecification const& description,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& state)
{
    UNREFERENCED_PARAMETER(description);

    return fm.NodeCacheObj.BeginSetPendingFabricUpgrade(
        nodeInfo,
        true,
        callback,
        state);
}

bool TrySetPendingTraits<FabricUpgradeSpecification>::EndTrySetPending(AsyncOperationSPtr const& operation, FailoverManager const& fm)
{
    ErrorCode error = fm.NodeCacheObj.EndSetPendingFabricUpgrade(operation);
    return error.IsSuccess();
}

AsyncOperationSPtr TrySetPendingTraits<UpgradeDescription>::BeginTrySetPending(
    FailoverManager const& fm,
    NodeInfo const& nodeInfo,
    UpgradeDescription const& description,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& state)
{
    return fm.NodeCacheObj.BeginAddPendingApplicationUpgrade(
        nodeInfo,
        description.ApplicationId,
        callback,
        state);
}

bool TrySetPendingTraits<UpgradeDescription>::EndTrySetPending(AsyncOperationSPtr const& operation, FailoverManager const& fm)
{
    ErrorCode error = fm.NodeCacheObj.EndAddPendingApplicationUpgrade(operation);
    return error.IsSuccess();
}


template class Reliability::FailoverManagerComponent::Upgrade<FabricUpgradeSpecification>;
template class Reliability::FailoverManagerComponent::Upgrade<UpgradeDescription>;

