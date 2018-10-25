// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    // Used inside the message body for REST GetClusterHealthChunk commands
    class ClusterHealthChunkQueryDescription
        : public Common::IFabricJsonSerializable
    {
        DENY_COPY(ClusterHealthChunkQueryDescription)

    public:
        ClusterHealthChunkQueryDescription();

        ClusterHealthChunkQueryDescription(ClusterHealthChunkQueryDescription && other) = default;
        ClusterHealthChunkQueryDescription & operator = (ClusterHealthChunkQueryDescription && other) = default;

        virtual ~ClusterHealthChunkQueryDescription();

        __declspec(property(get=get_HealthPolicy, put=set_HealthPolicy)) ClusterHealthPolicyUPtr const & HealthPolicy;
        ClusterHealthPolicyUPtr const & get_HealthPolicy() const { return healthPolicy_; }
        void set_HealthPolicy(ClusterHealthPolicyUPtr && value) { healthPolicy_ = std::move(value); }

        __declspec(property(get = get_ApplicationHealthPolicies)) ApplicationHealthPolicyMap const & ApplicationHealthPolicies;
        ApplicationHealthPolicyMap const & get_ApplicationHealthPolicies() const { return applicationHealthPolicies_; }
        void SetApplicationHealthPolicies(std::map<std::wstring, ApplicationHealthPolicy> && applicationHealthPolicies) { applicationHealthPolicies_.SetMap(std::move(applicationHealthPolicies)); }
        
        __declspec(property(get=get_ApplicationFilters)) ApplicationHealthStateFilterList const & ApplicationFilters;
        ApplicationHealthStateFilterList const & get_ApplicationFilters() const { return applicationFilters_; }
        
        __declspec(property(get=get_NodeFilters)) NodeHealthStateFilterList const & NodeFilters;
        NodeHealthStateFilterList const & get_NodeFilters() const { return nodeFilters_; }
        
        std::wstring ToJsonString() const;
        static Common::ErrorCode FromJsonString(
            std::wstring const & str,
            __inout ClusterHealthChunkQueryDescription & queryDescription);

        Common::ErrorCode FromPublicApi(FABRIC_CLUSTER_HEALTH_CHUNK_QUERY_DESCRIPTION const & publicClusterHealthChunkQueryDescription);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::NodeFilters, nodeFilters_)
            SERIALIZABLE_PROPERTY(Constants::ApplicationFilters, applicationFilters_)
            SERIALIZABLE_PROPERTY(Constants::ClusterHealthPolicy, healthPolicy_)
            SERIALIZABLE_PROPERTY(Constants::ApplicationHealthPolicies, applicationHealthPolicies_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        void MoveToQueryArguments(__inout QueryArgumentMap & argMap);

        Common::ErrorCode ValidateFilters() const;

    private:
        static Common::ErrorCode ValidateNodeFilters(
            NodeHealthStateFilterList const & filters);
        static Common::ErrorCode ValidateApplicationFilters(
            ApplicationHealthStateFilterList const & filters);
        static Common::ErrorCode ValidateServiceFilters(
            ServiceHealthStateFilterList const & filters, 
            std::wstring const & applicationNameFilter);
        static Common::ErrorCode ValidatePartitionFilters(
            PartitionHealthStateFilterList const & filters,
            std::wstring const & applicationNameFilter,
            std::wstring const & serviceNameFilter);
        static Common::ErrorCode ValidateReplicaFilters(
            ReplicaHealthStateFilterList const & filters,
            std::wstring const & applicationNameFilter,
            std::wstring const & serviceNameFilter ,
            Common::Guid const & partitionIdFilter);
        static Common::ErrorCode ValidateDeployedApplicationFilters(
            DeployedApplicationHealthStateFilterList const & filters,
            std::wstring const & applicationNameFilter);
        static Common::ErrorCode ValidateDeployedServicePackageFilters(
            DeployedServicePackageHealthStateFilterList const & filters,
            std::wstring const & applicationNameFilter,
            std::wstring const & nodeNameFilter);

        ClusterHealthPolicyUPtr healthPolicy_;
        ApplicationHealthPolicyMap applicationHealthPolicies_;
        ApplicationHealthStateFilterList applicationFilters_;
        NodeHealthStateFilterList nodeFilters_;
    };
}

