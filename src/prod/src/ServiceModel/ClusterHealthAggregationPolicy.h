// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ClusterHealthAggregationPolicy 
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        ClusterHealthAggregationPolicy();
        ClusterHealthAggregationPolicy(
            ClusterHealthPolicy && clusterPolicy,
            BYTE && maxPercentNodesUnhealthyPerUpgradeDomain,
            ApplicationHealthAggregationPolicy && defaultApplicationAggregationPolicy,
            ApplicationHealthAggregationPolicyMap &&);
        virtual ~ClusterHealthAggregationPolicy();

        __declspec(property(get=get_ClusterPolicy)) ClusterHealthPolicy const & ClusterPolicy;
        inline ClusterHealthPolicy const & get_ClusterPolicy() const { return clusterPolicy_; };

        __declspec(property(get=get_MaxPercentNodesUnhealthyPerUD)) BYTE MaxPercentNodesUnhealthyPerUpgradeDomain;
        inline BYTE get_MaxPercentNodesUnhealthyPerUD() const { return maxPercentNodesUnhealthyPerUpgradeDomain_; };

        __declspec(property(get=get_DefaultApplicationAggregationPolicy)) ApplicationHealthAggregationPolicy const & DefaultApplicationAggregationPolicy;
        inline ApplicationHealthAggregationPolicy const & get_DefaultApplicationAggregationPolicy() const { return defaultApplicationAggregationPolicy_; };

        __declspec(property(get=get_ApplicationAggregationPolicies)) ApplicationHealthAggregationPolicyMap const & ApplicationAggregationPolicyOverrides;
        inline ApplicationHealthAggregationPolicyMap const & get_ApplicationAggregationPolicies() const { return applicationAggregationPolicies_; };

        bool operator == (ClusterHealthAggregationPolicy const & other) const;
        bool operator != (ClusterHealthAggregationPolicy const & other) const;

        ClusterHealthAggregationPolicy & operator = (ClusterHealthAggregationPolicy && other);

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        Common::ErrorCode FromPublicApi(FABRIC_CLUSTER_HEALTH_AGGREGATION_POLICY const &);
        void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_CLUSTER_HEALTH_AGGREGATION_POLICY &) const;

        std::wstring ToString() const;
        static Common::ErrorCode FromString(std::wstring const &, __out ClusterHealthAggregationPolicy &);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::ClusterHealthPolicy, clusterPolicy_)
            SERIALIZABLE_PROPERTY(Constants::MaxPercentNodesUnhealthyPerUpgradeDomain, maxPercentNodesUnhealthyPerUpgradeDomain_)
            SERIALIZABLE_PROPERTY(Constants::DefaultApplicationHealthAggregationPolicy, defaultApplicationAggregationPolicy_)
            SERIALIZABLE_PROPERTY(Constants::ApplicationHealthAggregationPolicyOverrides, applicationAggregationPolicies_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        FABRIC_FIELDS_04(clusterPolicy_, maxPercentNodesUnhealthyPerUpgradeDomain_, defaultApplicationAggregationPolicy_, applicationAggregationPolicies_);

    private:
        ClusterHealthPolicy clusterPolicy_;
        BYTE maxPercentNodesUnhealthyPerUpgradeDomain_;
        ApplicationHealthAggregationPolicy defaultApplicationAggregationPolicy_;
        // keyed by application name
        ApplicationHealthAggregationPolicyMap applicationAggregationPolicies_;
    };
}
