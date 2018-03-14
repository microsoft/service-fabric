// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ServiceHealth
        : public EntityHealthBase
    {
        DENY_COPY(ServiceHealth)
    public:
        ServiceHealth();

        ServiceHealth(
            std::wstring const & serviceName,
            std::vector<HealthEvent> && events,
            FABRIC_HEALTH_STATE aggregatedHealthState,
            std::vector<HealthEvaluation> && unhealthyEvaluations,
            std::vector<PartitionAggregatedHealthState> && partitionsAggregatedHealthStates,
            Management::HealthManager::HealthStatisticsUPtr && healthStatistics);

        ServiceHealth(ServiceHealth && other) = default;
        ServiceHealth & operator = (ServiceHealth && other) = default;

        virtual ~ServiceHealth();

        __declspec(property(get=get_ServiceName)) std::wstring const& ServiceName;
        std::wstring const& get_ServiceName() const { return serviceName_; }

        __declspec(property(get=get_PartitionsAggregatedHealthStates)) std::vector<PartitionAggregatedHealthState> const & PartitionsAggregatedHealthStates;
        std::vector<PartitionAggregatedHealthState> const & get_PartitionsAggregatedHealthStates() const { return partitionsAggregatedHealthStates_; }

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_SERVICE_HEALTH & publicServiceHealth) const;

        Common::ErrorCode FromPublicApi(
            FABRIC_SERVICE_HEALTH const & publicServiceHealth);
        
        void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const override;
        void WriteToEtw(uint16 contextSequenceId) const override;

        FABRIC_FIELDS_06(serviceName_, events_, aggregatedHealthState_, partitionsAggregatedHealthStates_, unhealthyEvaluations_, healthStats_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(Constants::Name, serviceName_)
            SERIALIZABLE_PROPERTY(Constants::PartitionAggregatedHealthState, partitionsAggregatedHealthStates_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(serviceName_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(partitionsAggregatedHealthStates_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        std::wstring serviceName_;
        std::vector<PartitionAggregatedHealthState> partitionsAggregatedHealthStates_;
    };    
}
