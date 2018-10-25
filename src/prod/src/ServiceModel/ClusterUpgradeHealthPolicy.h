// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ClusterUpgradeHealthPolicy
        : public Common::IFabricJsonSerializable
        , public Serialization::FabricSerializable
    {
    public:
        ClusterUpgradeHealthPolicy();

        ClusterUpgradeHealthPolicy(
            BYTE maxPercentDeltaUnhealthyNodes,
            BYTE maxPercentUpgradeDomainDeltaUnhealthyNodes);

        ClusterUpgradeHealthPolicy(ClusterUpgradeHealthPolicy const & other) = default;
        ClusterUpgradeHealthPolicy & operator = (ClusterUpgradeHealthPolicy const & other) = default;

        ClusterUpgradeHealthPolicy(ClusterUpgradeHealthPolicy && other) = default;
        ClusterUpgradeHealthPolicy & operator = (ClusterUpgradeHealthPolicy && other) = default;

        virtual ~ClusterUpgradeHealthPolicy();

        __declspec(property(get=get_MaxPercentDeltaUnhealthyNodes)) BYTE MaxPercentDeltaUnhealthyNodes;
        BYTE get_MaxPercentDeltaUnhealthyNodes() const { return maxPercentDeltaUnhealthyNodes_; }

        __declspec(property(get=get_MaxPercentUpgradeDomainDeltaUnhealthyNodes)) BYTE MaxPercentUpgradeDomainDeltaUnhealthyNodes;
        BYTE get_MaxPercentUpgradeDomainDeltaUnhealthyNodes() const { return maxPercentUpgradeDomainDeltaUnhealthyNodes_; }

        bool operator == (ClusterUpgradeHealthPolicy const & other) const;
        bool operator != (ClusterUpgradeHealthPolicy const & other) const;

        std::wstring ToString() const;
        static Common::ErrorCode FromString(std::wstring const & clusterHealthPolicyStr, __inout ClusterUpgradeHealthPolicy & healthPolicy);

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;

        Common::ErrorCode FromPublicApi(FABRIC_CLUSTER_UPGRADE_HEALTH_POLICY const & publicClusterUpgradeHealthPolicy);

        Common::ErrorCode ToPublicApi(__in Common::ScopedHeap & heap, __inout FABRIC_CLUSTER_UPGRADE_HEALTH_POLICY & publicClusterUpgradeHealthPolicy) const;

        bool TryValidate(__inout std::wstring & validationErrorMessage) const;
                
        FABRIC_FIELDS_02(maxPercentDeltaUnhealthyNodes_, maxPercentUpgradeDomainDeltaUnhealthyNodes_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::MaxPercentDeltaUnhealthyNodes, maxPercentDeltaUnhealthyNodes_)
            SERIALIZABLE_PROPERTY(Constants::MaxPercentUpgradeDomainDeltaUnhealthyNodes, maxPercentUpgradeDomainDeltaUnhealthyNodes_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        BYTE maxPercentDeltaUnhealthyNodes_;
        BYTE maxPercentUpgradeDomainDeltaUnhealthyNodes_;
    };

    typedef std::shared_ptr<ClusterUpgradeHealthPolicy> ClusterUpgradeHealthPolicySPtr;
}

