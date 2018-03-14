// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class NodeUpgradeProgress 
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
        DEFAULT_COPY_CONSTRUCTOR(NodeUpgradeProgress)
        DEFAULT_COPY_ASSIGNMENT(NodeUpgradeProgress)

    public:
        NodeUpgradeProgress();

        // Constructor when safety check is NodeUpgradePhase is Upgrading
        NodeUpgradeProgress(
            Federation::NodeId nodeId,
            std::wstring const& nodeName,
            NodeUpgradePhase::Enum upgradePhase);

        // Constructor when safety check is SeedNodeUpgradeSafetyCheck
        NodeUpgradeProgress(
            Federation::NodeId nodeId,
            std::wstring const& nodeName,
            NodeUpgradePhase::Enum upgradePhase,
            UpgradeSafetyCheckKind::Enum kind);

        // Constructor when safety check is PartitionUpgradeSafetyCheck
        NodeUpgradeProgress(
            Federation::NodeId nodeId,
            std::wstring const& nodeName,
            NodeUpgradePhase::Enum upgradePhase,
            UpgradeSafetyCheckKind::Enum kind,
            Common::Guid partitionId);

        NodeUpgradeProgress(NodeUpgradeProgress && other);
        NodeUpgradeProgress & operator=(NodeUpgradeProgress && other);

        bool operator==(NodeUpgradeProgress const & other) const;
        bool operator!=(NodeUpgradeProgress const & other) const;
        bool Equals(NodeUpgradeProgress const & other, bool ignoreDynamicContent) const;

        __declspec (property(get = get_NodeId)) Federation::NodeId NodeId;
        Federation::NodeId get_NodeId() const { return nodeId_; }

        __declspec (property(get = get_NodeName)) std::wstring NodeName;
        std::wstring get_NodeName() const { return nodeName_; }

        __declspec (property(get = get_UpgradePhase)) NodeUpgradePhase::Enum UpgradePhase;
        NodeUpgradePhase::Enum get_UpgradePhase() const { return upgradePhase_; }

        __declspec (property(get = get_PendingSafetyChecks)) std::vector<UpgradeSafetyCheckWrapper> PendingSafetyChecks;
        std::vector<UpgradeSafetyCheckWrapper> get_PendingSafetyChecks() const { return pendingSafetyChecks_; }

        virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const;

        void ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_NODE_UPGRADE_PROGRESS & nodeUpgradeProgress) const;
        Common::ErrorCode FromPublicApi(
            FABRIC_NODE_UPGRADE_PROGRESS const &);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::NodeName, nodeName_)
            SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::UpgradePhase, upgradePhase_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::PendingSafetyChecks, pendingSafetyChecks_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        FABRIC_FIELDS_04(nodeId_, nodeName_, upgradePhase_, pendingSafetyChecks_);

    protected:

        Federation::NodeId nodeId_;
        std::wstring nodeName_;
        NodeUpgradePhase::Enum upgradePhase_;
        std::vector<UpgradeSafetyCheckWrapper> pendingSafetyChecks_;
    };

    class UpgradeDomainProgress
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
        DEFAULT_COPY_CONSTRUCTOR(UpgradeDomainProgress)
        DEFAULT_COPY_ASSIGNMENT(UpgradeDomainProgress)

    public:
        UpgradeDomainProgress();

        UpgradeDomainProgress(
            std::wstring const& upgradeDomainName,
            std::vector<NodeUpgradeProgress> const& nodeUpgradeProgressList);

        UpgradeDomainProgress(UpgradeDomainProgress && other);

        UpgradeDomainProgress & operator=(UpgradeDomainProgress && other);

        bool operator==(UpgradeDomainProgress const & other) const;
        bool operator!=(UpgradeDomainProgress const & other) const;
        bool Equals(UpgradeDomainProgress const & other, bool ignoreDynamicContent) const;

        __declspec (property(get = get_IsEmpty)) bool IsEmpty;
        bool get_IsEmpty() const { return nodeUpgradeProgressList_.empty(); }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const;

        static Common::ErrorCode FromString(std::wstring const & json, __out UpgradeDomainProgress &);

        void ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_UPGRADE_DOMAIN_PROGRESS & upgradeDomainProgress) const;
        Common::ErrorCode FromPublicApi(
            FABRIC_UPGRADE_DOMAIN_PROGRESS const &);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::DomainName, upgradeDomainName_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::NodeUpgradeProgressList, nodeUpgradeProgressList_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        FABRIC_FIELDS_02(upgradeDomainName_, nodeUpgradeProgressList_);

    private:

        std::wstring upgradeDomainName_;
        std::vector<NodeUpgradeProgress> nodeUpgradeProgressList_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Reliability::NodeUpgradeProgress);
