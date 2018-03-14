// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ClusterHealth : public EntityHealthBase
    {
        DENY_COPY(ClusterHealth)
    public:
        ClusterHealth();
        ClusterHealth(
            FABRIC_HEALTH_STATE aggregatedHealthState,
            std::vector<HealthEvaluation> && unhealthyEvaluations,
            std::vector<NodeAggregatedHealthState> && nodesAggregatedHealthStates,
            std::vector<ApplicationAggregatedHealthState> && applicationsAggregatedHealthStates,
            std::vector<HealthEvent> && events,
            Management::HealthManager::HealthStatisticsUPtr && healthStatistics);
        
        ClusterHealth(ClusterHealth && other) = default;
        ClusterHealth & operator= (ClusterHealth && other) = default;

        virtual ~ClusterHealth();

        __declspec(property(get=get_NodesAggregatedHealthStates)) std::vector<NodeAggregatedHealthState> const & NodesAggregatedHealthStates;
        std::vector<NodeAggregatedHealthState> const & get_NodesAggregatedHealthStates() const { return nodesAggregatedHealthStates_; }

        __declspec(property(get=get_ApplicationsAggregatedHealthStates)) std::vector<ApplicationAggregatedHealthState> const & ApplicationsAggregatedHealthStates;
        std::vector<ApplicationAggregatedHealthState> const & get_ApplicationsAggregatedHealthStates() const { return applicationsAggregatedHealthStates_; }

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_CLUSTER_HEALTH & publicClusterHealth) const;

        Common::ErrorCode FromPublicApi(
            FABRIC_CLUSTER_HEALTH const & publicClusterHealth);
        
        void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const override;
        void WriteToEtw(uint16 contextSequenceId) const override;

        std::wstring ToString() const;
        static Common::ErrorCode FromString(std::wstring const & clusterHealthStr, __out ClusterHealth & clusterHealth);

        FABRIC_FIELDS_06(aggregatedHealthState_, nodesAggregatedHealthStates_, applicationsAggregatedHealthStates_, events_, unhealthyEvaluations_, healthStats_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(Constants::NodeAggregatedHealthStates, nodesAggregatedHealthStates_)
            SERIALIZABLE_PROPERTY(Constants::ApplicationAggregatedHealthStates, applicationsAggregatedHealthStates_)
        END_JSON_SERIALIZABLE_PROPERTIES()      

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(nodesAggregatedHealthStates_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(applicationsAggregatedHealthStates_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        std::vector<NodeAggregatedHealthState> nodesAggregatedHealthStates_;
        std::vector<ApplicationAggregatedHealthState> applicationsAggregatedHealthStates_;
    };

    using ClusterHealthSPtr = std::shared_ptr<ClusterHealth>;
}
