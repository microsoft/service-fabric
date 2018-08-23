// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class EntityHealthBase
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
        , public Common::ISizeEstimator
    {
        DENY_COPY(EntityHealthBase)
    public:
        EntityHealthBase();

        EntityHealthBase(
            std::vector<HealthEvent> && events,
            FABRIC_HEALTH_STATE aggregatedHealthState,
            std::vector<HealthEvaluation> && unhealthyEvaluations,
            Management::HealthManager::HealthStatisticsUPtr && healthStatistics = nullptr);
        
        EntityHealthBase(EntityHealthBase && other) = default;
        EntityHealthBase & operator = (EntityHealthBase && other) = default;

        virtual ~EntityHealthBase();

        __declspec(property(get=get_UnhealthyEvaluations)) std::vector<HealthEvaluation> const & UnhealthyEvaluations;
        std::vector<HealthEvaluation> const & get_UnhealthyEvaluations() const { return unhealthyEvaluations_; }
        std::vector<HealthEvaluation> && TakeUnhealthyEvaluations() { return std::move(unhealthyEvaluations_); }

         __declspec(property(get=get_Events)) std::vector<HealthEvent> const & Events;
        std::vector<HealthEvent> const & get_Events() const { return events_; }
    
        __declspec(property(get=get_AggregatedHealthState)) FABRIC_HEALTH_STATE AggregatedHealthState;
        FABRIC_HEALTH_STATE get_AggregatedHealthState() const { return aggregatedHealthState_; }

        __declspec(property(get=get_HealthStats)) Management::HealthManager::HealthStatisticsUPtr const & HealthStats;
        Management::HealthManager::HealthStatisticsUPtr const & get_HealthStats() const { return healthStats_; }

        Management::HealthManager::HealthStatisticsUPtr TakeHealthStats(){ return std::move(healthStats_); }
        
        // Updates the health evaluations with description.
        // If the object doesn't fit the max allowed size, trims the unhealthy evaluations if possible.
        // Otherwise, it returns error that max size is reached.
        Common::ErrorCode UpdateUnhealthyEvalautionsPerMaxAllowedSize(size_t maxAllowedSize);

        virtual void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;
        virtual void WriteToEtw(uint16 contextSequenceId) const;
        std::wstring ToString() const;
        std::wstring GetUnhealthyEvaluationDescription() const;

        std::wstring GetStatsString() const;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::HealthEvents, events_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::AggregatedHealthState, aggregatedHealthState_)
            SERIALIZABLE_PROPERTY(Constants::UnhealthyEvaluations, unhealthyEvaluations_)
            SERIALIZABLE_PROPERTY_IF(Constants::HealthStatistics, healthStats_, healthStats_ != nullptr)
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_BASE_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(events_)
            DYNAMIC_ENUM_ESTIMATION_MEMBER(aggregatedHealthState_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(unhealthyEvaluations_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(healthStats_)
        END_DYNAMIC_SIZE_ESTIMATION()

    protected:    
        std::vector<HealthEvent> events_;
        FABRIC_HEALTH_STATE aggregatedHealthState_;
        std::vector<HealthEvaluation> unhealthyEvaluations_;
        Management::HealthManager::HealthStatisticsUPtr healthStats_;
    };    
}
