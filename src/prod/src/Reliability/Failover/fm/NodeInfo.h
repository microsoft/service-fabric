// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class NodeInfo : public StoreData
        {
        public:
            NodeInfo();

            NodeInfo(
                Federation::NodeInstance const& nodeInstance,
                NodeDescription && nodeDescription,
                bool isReplicaUploaded);

            NodeInfo(
                Federation::NodeInstance const& nodeInstance,
                NodeDescription && nodeDescription,
                bool isUp,
                bool isReplicaUploaded,
                bool isNodeStateRemoved,
                Common::DateTime lastUpdated);

            NodeInfo(NodeInfo const& other);

            NodeInfo(NodeInfo && other);

            void InitializeNodeName();

            NodeInfo & operator=(NodeInfo && other);

            static std::wstring const& GetStoreType();
            virtual std::wstring const& GetStoreKey() const;

            FABRIC_FIELDS_17(
                nodeInstance_,
                lastUpdated_,
                isUp_,
                isReplicaUploaded_,
                isNodeHealthReportedDuringDeactivate_,
                isNodeStateRemoved_,
                nodeDescription_,
                actualUpgradeDomainId_,
                sequenceNumber_,
                deactivationInfo_,
                healthSequence_,
                isNodeHealthReportedDuringUpgrade_,
                isPendingDeactivateNode_,
                isPendingFabricUpgrade_,
                pendingApplicationUpgrades_,
                nodeUpTime_,
                nodeDownTime_);

            __declspec (property(get=get_NodeId)) Federation::NodeId Id;
            Federation::NodeId get_NodeId() const { return nodeInstance_.Id; }

            __declspec (property(get=get_NodeInstance)) Federation::NodeInstance const& NodeInstance;
            Federation::NodeInstance const& get_NodeInstance() const { return nodeInstance_; }

            __declspec (property(get=get_Description, put=set_Description)) NodeDescription & Description;
            NodeDescription const& get_Description() const { return nodeDescription_; }
            void set_Description(NodeDescription const& value) { nodeDescription_ = value; }

            __declspec (property(get=get_NodeStatus)) FABRIC_QUERY_NODE_STATUS NodeStatus;
            FABRIC_QUERY_NODE_STATUS get_NodeStatus() const;

            __declspec (property(get=get_NodeUpTime, put=set_NodeUpTime)) Common::DateTime NodeUpTime;
            Common::DateTime get_NodeUpTime() const { return nodeUpTime_; }
            void set_NodeUpTime(Common::DateTime value) { nodeUpTime_ = value; }

            __declspec (property(get = get_NodeDownTime, put = set_NodeDownTime)) Common::DateTime NodeDownTime;
            Common::DateTime get_NodeDownTime() const { return nodeDownTime_; }
            void set_NodeDownTime(Common::DateTime value) { nodeDownTime_ = value; }

            __declspec (property(get=get_VersionInstance)) Common::FabricVersionInstance const& VersionInstance;
            Common::FabricVersionInstance const& get_VersionInstance() const { return nodeDescription_.VersionInstance; }

            __declspec (property(get=get_IdString)) std::wstring const& IdString;
            std::wstring const& get_IdString() const;

            __declspec (property(get=get_IsUp)) bool IsUp;
            bool get_IsUp() const { return isUp_; }

            __declspec (property(get=get_IsUnknown)) bool IsUnknown;
            bool get_IsUnknown() const { return VersionInstance == Common::FabricVersionInstance::Invalid; }

            __declspec (property(get=get_IsReplicaUploaded, put=set_IsReplicaUploaded)) bool IsReplicaUploaded;
            bool get_IsReplicaUploaded() const { return isReplicaUploaded_; }
            void set_IsReplicaUploaded(bool value) { isReplicaUploaded_ = value; }

            __declspec (property(get=get_DeactivationInfo, put=set_DeactivationInfo)) NodeDeactivationInfo & DeactivationInfo;
            NodeDeactivationInfo const& get_DeactivationInfo() const { return deactivationInfo_; }
            NodeDeactivationInfo & get_DeactivationInfo() { return deactivationInfo_; }
            void set_DeactivationInfo(NodeDeactivationInfo const& value) { deactivationInfo_ = value; }

            __declspec (property(get=get_IsNodeStateRemoved, put=set_IsNodeStateRemoved)) bool IsNodeStateRemoved;
            bool get_IsNodeStateRemoved() const { return isNodeStateRemoved_; }
            void set_IsNodeStateRemoved(bool value) { isNodeStateRemoved_ = value; }

            __declspec (property(get=get_IsStale, put=set_IsStale)) bool IsStale;
            bool get_IsStale() const { return isStale_; }
            void set_IsStale(bool value) { isStale_ = value; }

            __declspec (property(get=get_LastUpdated)) Common::DateTime LastUpdated;
            Common::DateTime get_LastUpdated() const { return lastUpdated_; }

            __declspec (property(get=get_SequenceNumber, put=set_SequenceNumber)) uint64 SequenceNumber;
            uint64 get_SequenceNumber() const { return sequenceNumber_; }
            void set_SequenceNumber(uint64 value) { sequenceNumber_ = value; }

            __declspec (property(get=get_HealthSequence, put=set_HealthSequence)) FABRIC_SEQUENCE_NUMBER HealthSequence;
            FABRIC_SEQUENCE_NUMBER get_HealthSequence() const { return healthSequence_; }
            void set_HealthSequence(FABRIC_SEQUENCE_NUMBER value) { healthSequence_ = value; }

            __declspec (property(get=get_IsNodeHealthReportedDuringUpgrade, put=set_IsNodeHealthReportedDuringUpgrade)) bool IsNodeHealthReportedDuringUpgrade;
            bool get_IsNodeHealthReportedDuringUpgrade() const { return isNodeHealthReportedDuringUpgrade_; }
            void set_IsNodeHealthReportedDuringUpgrade(bool value) { isNodeHealthReportedDuringUpgrade_ = value; }

            __declspec (property(get = get_IsNodeHealthReportedDuringDeactivate, put = set_IsNodeHealthReportedDuringDeactivate)) bool IsNodeHealthReportedDuringDeactivate;
            bool get_IsNodeHealthReportedDuringDeactivate() const { return isNodeHealthReportedDuringDeactivate_; }
            void set_IsNodeHealthReportedDuringDeactivate(bool value) { isNodeHealthReportedDuringDeactivate_ = value; }

            __declspec (property(get=get_NodeProperties)) std::map<std::wstring, std::wstring> const& NodeProperties;
            std::map<std::wstring, std::wstring> const& get_NodeProperties() const { return nodeDescription_.NodePropertyMap; }

            __declspec (property(get=get_NodeCapacityRatios)) std::map<std::wstring, uint> const& NodeCapacityRatios;
            std::map<std::wstring, uint> const& get_NodeCapacityRatios() const { return nodeDescription_.NodeCapacityRatioMap; }

            __declspec (property(get=get_NodeCapacities)) std::map<std::wstring, uint> const& NodeCapacities;
            std::map<std::wstring, uint> const& get_NodeCapacities() const { return nodeDescription_.NodeCapacityMap; }

            __declspec (property(get=get_UpgradeDomainId)) std::wstring const& UpgradeDomainId;
            std::wstring const& get_UpgradeDomainId() const { return nodeDescription_.NodeUpgradeDomainId; }

            __declspec (property(get=get_ActualUpgradeDomainId)) std::wstring const& ActualUpgradeDomainId;
            std::wstring const& get_ActualUpgradeDomainId() const { return actualUpgradeDomainId_; }

            __declspec (property(get=get_FaultDomainIds)) std::vector<Common::Uri> const & FaultDomainIds;
            std::vector<Common::Uri> const & get_FaultDomainIds() const { return nodeDescription_.NodeFaultDomainIds; }

            __declspec (property(get=get_FaultDomainId)) std::wstring FaultDomainId;
            std::wstring get_FaultDomainId() const;

            __declspec (property(get=get_NodeName)) std::wstring const& NodeName;
            std::wstring const& get_NodeName() const { return nodeDescription_.NodeName; }

            __declspec (property(get=get_NodeType)) std::wstring const& NodeType;
            std::wstring const& get_NodeType() const { return nodeDescription_.NodeType; }

            __declspec (property(get=get_IpAddressOrFQDN)) std::wstring const& IpAddressOrFQDN;
            std::wstring const& get_IpAddressOrFQDN() const { return nodeDescription_.IpAddressOrFQDN; }

            __declspec (property(get = get_DisabledPartitions, put = set_DisabledPartitions)) std::vector<PartitionId> DisabledPartitions;
            std::vector<PartitionId> get_DisabledPartitions() const { return disabledPartitions_; }
            void set_DisabledPartitions(std::vector<PartitionId> && value) { disabledPartitions_ = move(value); }

            __declspec (property(get=get_IsPendingDeactivateNode, put=set_IsPendingDeactivateNode)) bool IsPendingDeactivateNode;
            bool get_IsPendingDeactivateNode() const { return isPendingDeactivateNode_; }
            void set_IsPendingDeactivateNode(bool value) { isPendingDeactivateNode_ = value; }

            __declspec (property(get=get_IsPendingFabricUpgrade, put=set_IsPendingFabricUpgrade)) bool IsPendingFabricUpgrade;
            bool get_IsPendingFabricUpgrade() const { return isPendingFabricUpgrade_; }
            void set_IsPendingFabricUpgrade(bool value) { isPendingFabricUpgrade_ = value; }

            __declspec (property(get = get_IsPendingUpgrade)) bool IsPendingUpgrade;
            bool get_IsPendingUpgrade() const { return (isPendingFabricUpgrade_ || !pendingApplicationUpgrades_.empty()); }

            bool IsPendingApplicationUpgrade(ServiceModel::ApplicationIdentifier const& applicationId) const;
            void AddPendingApplicationUpgrade(ServiceModel::ApplicationIdentifier const& applicationId);
            void RemovePendingApplicationUpgrade(ServiceModel::ApplicationIdentifier const& applicationId);

            bool IsPendingUpgradeOrDeactivateNode() const;
            bool IsPendingNodeClose(FailoverManager const& fm) const;
            bool IsPendingClose(FailoverManager const& fm, ApplicationInfo const& applicationInfo) const;

            static std::wstring GetActualUpgradeDomain(Federation::NodeId nodeId, std::wstring const& upgradeDomain);

            LoadBalancingComponent::NodeDescription GetPLBNodeDescription() const;

            void PostUpdate(Common::DateTime updatedTime);

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        private:
            Federation::NodeInstance nodeInstance_;
            Common::DateTime lastUpdated_;
            bool isUp_;
            bool isReplicaUploaded_;
            bool isNodeStateRemoved_;
            NodeDescription nodeDescription_;

            Common::DateTime nodeUpTime_;
            Common::DateTime nodeDownTime_;

            std::wstring actualUpgradeDomainId_;

            // Used to detect stale messages
            uint64 sequenceNumber_;

            NodeDeactivationInfo deactivationInfo_;

            bool isPendingDeactivateNode_;
            bool isPendingFabricUpgrade_;
            std::vector<ServiceModel::ApplicationIdentifier> pendingApplicationUpgrades_;

            std::vector<PartitionId> disabledPartitions_;

            FABRIC_SEQUENCE_NUMBER healthSequence_;

            bool isNodeHealthReportedDuringUpgrade_;
            bool isNodeHealthReportedDuringDeactivate_;

            // this fields indicate that whether there is a newer
            // NodeInfo in the node cache for this node
            mutable bool isStale_;

            mutable std::wstring idString_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(Reliability::FailoverManagerComponent::NodeInfo);
