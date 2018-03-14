// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

NodeInfo::NodeInfo()
    : StoreData(),
      nodeUpTime_(DateTime::Zero),
      nodeDownTime_(DateTime::Zero),
      deactivationInfo_(),
      isNodeStateRemoved_(false),
      isPendingDeactivateNode_(false),
      isPendingFabricUpgrade_(false),
      pendingApplicationUpgrades_(),
      sequenceNumber_(0),
      healthSequence_(FABRIC_INVALID_SEQUENCE_NUMBER),
      isNodeHealthReportedDuringUpgrade_(false),
      isNodeHealthReportedDuringDeactivate_(false),
      disabledPartitions_(),
      isStale_(false),
      idString_()
{
}

NodeInfo::NodeInfo(
    Federation::NodeInstance const& nodeInstance,
    NodeDescription && nodeDescription,
    bool isReplicaUploaded)
    : StoreData(PersistenceState::ToBeInserted),
      nodeInstance_(nodeInstance),
      lastUpdated_(DateTime::Now()),
      isUp_(true),
      isReplicaUploaded_(isReplicaUploaded),
      nodeUpTime_(DateTime::Zero),
      nodeDownTime_(DateTime::Zero),
      deactivationInfo_(),
      isNodeStateRemoved_(false),
      isPendingDeactivateNode_(false),
      isPendingFabricUpgrade_(false),
      pendingApplicationUpgrades_(),
      nodeDescription_(move(nodeDescription)),
      actualUpgradeDomainId_(GetActualUpgradeDomain(nodeInstance.Id, nodeDescription_.NodeUpgradeDomainId)),
      sequenceNumber_(0),
      healthSequence_(FABRIC_INVALID_SEQUENCE_NUMBER),
      isNodeHealthReportedDuringUpgrade_(false),
      isNodeHealthReportedDuringDeactivate_(false),
      disabledPartitions_(),
      isStale_(false),
      idString_()
{
}

NodeInfo::NodeInfo(
    Federation::NodeInstance const& nodeInstance,
    NodeDescription && nodeDescription,
    bool isUp,
    bool isReplicaUploaded,
    bool isNodeStateRemoved,
    DateTime lastUpdated)
    : StoreData(PersistenceState::ToBeInserted),
      nodeInstance_(nodeInstance),
      lastUpdated_(lastUpdated),
      isUp_(isUp),
      isReplicaUploaded_(isReplicaUploaded),
      nodeUpTime_(DateTime::Zero),
      nodeDownTime_(DateTime::Zero),
      deactivationInfo_(),
      isNodeStateRemoved_(isNodeStateRemoved),
      isPendingDeactivateNode_(false),
      isPendingFabricUpgrade_(false),
      pendingApplicationUpgrades_(),
      nodeDescription_(move(nodeDescription)),
      actualUpgradeDomainId_(GetActualUpgradeDomain(nodeInstance.Id, nodeDescription_.NodeUpgradeDomainId)),
      sequenceNumber_(0),
      healthSequence_(FABRIC_INVALID_SEQUENCE_NUMBER),
      isNodeHealthReportedDuringUpgrade_(false),
      isNodeHealthReportedDuringDeactivate_(false),
      disabledPartitions_(),
      isStale_(false),
      idString_()
{
}

NodeInfo::NodeInfo(NodeInfo const& other)
    : StoreData(other.OperationLSN, PersistenceState::ToBeUpdated),
      nodeInstance_(other.nodeInstance_),
      isUp_(other.isUp_),
      isReplicaUploaded_(other.isReplicaUploaded_),
      nodeUpTime_(other.nodeUpTime_),
      nodeDownTime_(other.nodeDownTime_),
      deactivationInfo_(other.deactivationInfo_),
      isNodeStateRemoved_(other.isNodeStateRemoved_),
      isPendingDeactivateNode_(other.isPendingDeactivateNode_),
      isPendingFabricUpgrade_(other.isPendingFabricUpgrade_),
      pendingApplicationUpgrades_(other.pendingApplicationUpgrades_),
      lastUpdated_(other.lastUpdated_),
      nodeDescription_(other.nodeDescription_),
      actualUpgradeDomainId_(other.actualUpgradeDomainId_),
      sequenceNumber_(other.sequenceNumber_),
      healthSequence_(other.healthSequence_),
      isNodeHealthReportedDuringUpgrade_(other.isNodeHealthReportedDuringUpgrade_),
      isNodeHealthReportedDuringDeactivate_(other.isNodeHealthReportedDuringDeactivate_),
      disabledPartitions_(other.disabledPartitions_),
      isStale_(other.isStale_),
      idString_(other.idString_)
{
}

NodeInfo::NodeInfo(NodeInfo && other)
    : StoreData(other),
      nodeInstance_(other.nodeInstance_),
      isUp_(other.isUp_),
      isReplicaUploaded_(other.isReplicaUploaded_),
      nodeUpTime_(other.nodeUpTime_),
      nodeDownTime_(other.nodeDownTime_),
      deactivationInfo_(move(other.deactivationInfo_)),
      isNodeStateRemoved_(other.isNodeStateRemoved_),
      isPendingDeactivateNode_(other.isPendingDeactivateNode_),
      isPendingFabricUpgrade_(other.isPendingFabricUpgrade_),
      pendingApplicationUpgrades_(move(other.pendingApplicationUpgrades_)),
      lastUpdated_(other.lastUpdated_),
      nodeDescription_(move(other.nodeDescription_)),
      actualUpgradeDomainId_(move(other.actualUpgradeDomainId_)),
      sequenceNumber_(other.sequenceNumber_),
      healthSequence_(other.healthSequence_),
      isNodeHealthReportedDuringUpgrade_(other.isNodeHealthReportedDuringUpgrade_),
      isNodeHealthReportedDuringDeactivate_(other.isNodeHealthReportedDuringDeactivate_),
      disabledPartitions_(move(other.disabledPartitions_)),
      isStale_(other.isStale_),
      idString_(move(other.idString_))
{
}

NodeInfo & NodeInfo::operator=(NodeInfo && other)
{
    if (this != &other)
    {
        PostRead(other.OperationLSN);
        PersistenceState = other.PersistenceState;

        nodeInstance_ = move(other.nodeInstance_);
        lastUpdated_ = other.lastUpdated_;
        isUp_ = other.isUp_;
        isReplicaUploaded_ = other.isReplicaUploaded_;
        nodeUpTime_ = other.nodeUpTime_;
        nodeDownTime_ = other.nodeDownTime_;
        deactivationInfo_ = move(other.deactivationInfo_);
        isNodeStateRemoved_ = other.isNodeStateRemoved_;
        isPendingDeactivateNode_ = other.isPendingDeactivateNode_;
        isPendingFabricUpgrade_ = other.isPendingFabricUpgrade_;
        pendingApplicationUpgrades_ = move(other.pendingApplicationUpgrades_);
        nodeDescription_ = move(other.nodeDescription_);
        actualUpgradeDomainId_ = move(other.actualUpgradeDomainId_);
        sequenceNumber_ = other.sequenceNumber_;
        healthSequence_ = other.healthSequence_;
        isNodeHealthReportedDuringUpgrade_ = other.isNodeHealthReportedDuringUpgrade_;
        isNodeHealthReportedDuringDeactivate_ = other.isNodeHealthReportedDuringDeactivate_;
        disabledPartitions_ = move(other.disabledPartitions_);
        isStale_ = other.isStale_;
        idString_ = move(other.idString_);
    }

    return *this;
}

void NodeInfo::InitializeNodeName()
{
    nodeDescription_.InitializeNodeName(nodeInstance_.Id);
}

wstring const& NodeInfo::GetStoreType()
{
    return FailoverManagerStore::NodeInfoType;
}

wstring const& NodeInfo::GetStoreKey() const
{
    return IdString;
}

wstring const& NodeInfo::get_IdString() const
{
    if (idString_.empty())
    {
        idString_ = Id.ToString();
    }

    return idString_;
}

bool NodeInfo::IsPendingApplicationUpgrade(ServiceModel::ApplicationIdentifier const& applicationId) const
{
    for (ServiceModel::ApplicationIdentifier const& appId : pendingApplicationUpgrades_)
    {
        if (appId == applicationId)
        {
            return true;
        }
    }

    return false;
}

void NodeInfo::AddPendingApplicationUpgrade(ServiceModel::ApplicationIdentifier const& applicationId)
{
    if (!IsPendingApplicationUpgrade(applicationId))
    {
        pendingApplicationUpgrades_.push_back(applicationId);
    }
}

void NodeInfo::RemovePendingApplicationUpgrade(ServiceModel::ApplicationIdentifier const& applicationId)
{
    for (auto it = pendingApplicationUpgrades_.begin(); it != pendingApplicationUpgrades_.end(); it++)
    {
        if (*it == applicationId)
        {
            pendingApplicationUpgrades_.erase(it);
            return;
        }
    }
}

bool NodeInfo::IsPendingNodeClose(FailoverManager const& fm) const
{
    bool isPendingClose = (
        deactivationInfo_.Status != NodeDeactivationStatus::None &&
        deactivationInfo_.Status > NodeDeactivationStatus::DeactivationSafetyCheckInProgress);
    
    isPendingClose |= (
        isPendingFabricUpgrade_ &&
        fm.FabricUpgradeManager.Upgrade &&
        fm.FabricUpgradeManager.Upgrade->Description.UpgradeType != UpgradeType::Rolling_NotificationOnly);

    return isPendingClose;
}

bool NodeInfo::IsPendingUpgradeOrDeactivateNode() const
{
    return isPendingDeactivateNode_ ||
           isPendingFabricUpgrade_ ||
           !pendingApplicationUpgrades_.empty();
}

bool NodeInfo::IsPendingClose(FailoverManager const& fm, ApplicationInfo const& applicationInfo) const
{
    bool isPendingClose = isPendingDeactivateNode_;

    isPendingClose |= (
        isPendingFabricUpgrade_ &&
        fm.FabricUpgradeManager.Upgrade &&
        fm.FabricUpgradeManager.Upgrade->Description.UpgradeType != UpgradeType::Rolling_NotificationOnly);

    isPendingClose |= (
        IsPendingApplicationUpgrade(applicationInfo.ApplicationId) &&
        applicationInfo.Upgrade &&
        applicationInfo.Upgrade->Description.UpgradeType != UpgradeType::Rolling_NotificationOnly);

    return isPendingClose;
}

wstring NodeInfo::GetActualUpgradeDomain(Federation::NodeId nodeId, wstring const& upgradeDomain)
{
    return (upgradeDomain.empty() ? nodeId.ToString() + L"_ud" : upgradeDomain);
}

wstring NodeInfo::get_FaultDomainId() const
{
    return (FaultDomainIds.size() > 0 ? FaultDomainIds[0].ToString() : L"");
}

LoadBalancingComponent::NodeDescription NodeInfo::GetPLBNodeDescription() const
{
    map<wstring, wstring> nodePropertiesForPLB(NodeProperties);

    //insert node properties only if user did not explictly add them
    if (nodePropertiesForPLB.find(*LoadBalancingComponent::Constants::ImplicitNodeType) == nodePropertiesForPLB.end())
    {
        nodePropertiesForPLB.insert(make_pair(*LoadBalancingComponent::Constants::ImplicitNodeType, NodeType));
    }
    if (nodePropertiesForPLB.find(*LoadBalancingComponent::Constants::ImplicitNodeName) == nodePropertiesForPLB.end())
    {
        nodePropertiesForPLB.insert(make_pair(*LoadBalancingComponent::Constants::ImplicitNodeName, NodeName));
    }
    if (!UpgradeDomainId.empty() && nodePropertiesForPLB.find(*LoadBalancingComponent::Constants::ImplicitUpgradeDomainId) == nodePropertiesForPLB.end())
    {
        nodePropertiesForPLB.insert(make_pair(*LoadBalancingComponent::Constants::ImplicitUpgradeDomainId, UpgradeDomainId));
    }

    wstring faultDomainId = FaultDomainId;
    if (!faultDomainId.empty() && nodePropertiesForPLB.find(*LoadBalancingComponent::Constants::ImplicitFaultDomainId) == nodePropertiesForPLB.end())
    {
        nodePropertiesForPLB.insert(make_pair(*LoadBalancingComponent::Constants::ImplicitFaultDomainId, move(faultDomainId)));
    }

    return LoadBalancingComponent::NodeDescription(
        NodeInstance,
        IsUp,
        deactivationInfo_.Intent,
        deactivationInfo_.Status,
        move(nodePropertiesForPLB),
        FaultDomainIds.empty() ? LoadBalancingComponent::NodeDescription::DomainId() : LoadBalancingComponent::NodeDescription::DomainId(FaultDomainIds[0].Segments),
        wstring(UpgradeDomainId),
        map<wstring, uint>(NodeCapacityRatios),
        map<wstring, uint>(NodeCapacities));
}

void NodeInfo::PostUpdate(DateTime updatedTime)
{
    lastUpdated_ = updatedTime;
}

FABRIC_QUERY_NODE_STATUS NodeInfo::get_NodeStatus() const
{
    if (!isUp_)
    {
        if (IsNodeStateRemoved)
        {
            return FABRIC_QUERY_NODE_STATUS_REMOVED;
        }
        if (IsUnknown)
        {
            return FABRIC_QUERY_NODE_STATUS_UNKNOWN;
        }
        return FABRIC_QUERY_NODE_STATUS_DOWN;
    }
    else if (deactivationInfo_.IsDeactivated)
    {
        if (deactivationInfo_.IsDeactivationComplete)
        {
            return FABRIC_QUERY_NODE_STATUS_DISABLED;
        }
        else
        {
            return FABRIC_QUERY_NODE_STATUS_DISABLING;
        }
    }
    else if (deactivationInfo_.IsActivationInProgress)
    {
        return FABRIC_QUERY_NODE_STATUS_ENABLING;
    }
    else
    {
        return FABRIC_QUERY_NODE_STATUS_UP;
    }
}

void NodeInfo::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    writer.Write("{0} {1}", nodeInstance_, (isUp_ ? L"Up" : L"Down"));
    writer.Write(" {0} {1} {2} {3} {4}", deactivationInfo_, sequenceNumber_, healthSequence_, this->OperationLSN, actualUpgradeDomainId_);

    if (isUp_ && !isReplicaUploaded_)
    {
        writer.Write(" Uploading");
    }

    if (isNodeStateRemoved_)
    {
        writer.Write(" NodeStateRemoved");
    }

    if (isPendingDeactivateNode_)
    {
        writer.Write(" PendingDeactivateNode");
    }

    if (isPendingFabricUpgrade_)
    {
        writer.Write(" PendingFabricUpgrade");
    }

    if (!pendingApplicationUpgrades_.empty())
    {
        writer.Write(" {0}", pendingApplicationUpgrades_);
    }

    if (isStale_)
    {
        writer.Write(" Stale");
    }

    writer.WriteLine(" {0}/{1} {2}", NodeUpTime, NodeDownTime, lastUpdated_);

    if (isUp_)
    {
        writer.WriteLine("{0}", nodeDescription_);
    }
    else
    {
        writer.WriteLine("VersionInstance: {0}", nodeDescription_.VersionInstance);
        writer.WriteLine("NodeName: {0}", nodeDescription_.NodeName);
    }
}
