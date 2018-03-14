// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        using HealthStateCountMap = std::map<EntityKind::Enum, HealthStateCount>;

        class HealthStatistics
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
            , public Common::ISizeEstimator
        {
        public:
            HealthStatistics();
            ~HealthStatistics();

            HealthStatistics(HealthStatistics const & other);
            HealthStatistics & operator=(HealthStatistics const & other);

            HealthStatistics(HealthStatistics && other);
            HealthStatistics & operator=(HealthStatistics && other);

            __declspec(property(get = get_HealthCounts)) std::vector<EntityKindHealthStateCount> const & HealthCounts;
            std::vector<EntityKindHealthStateCount> const & get_HealthCounts() const { return healthCounts_; }

            HealthStateCountMap GetHealthCountMap() const;

            void Add(EntityKind::Enum entityKind);
            void Add(EntityKind::Enum entityKind, HealthStateCount && count);
            void Add(EntityKind::Enum entityKind, FABRIC_HEALTH_STATE healthState);
            
            Common::ErrorCode ToPublicApi(
                __in Common::ScopedHeap & heap,
                __out FABRIC_HEALTH_STATISTICS & publicStats) const;

            Common::ErrorCode FromPublicApi(
                FABRIC_HEALTH_STATISTICS const * publicStats);

            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::HealthStateCountList, healthCounts_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            FABRIC_FIELDS_01(healthCounts_);

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(healthCounts_)
            END_DYNAMIC_SIZE_ESTIMATION()

        private:
            std::vector<EntityKindHealthStateCount> healthCounts_;
        };

        using HealthStatisticsUPtr = std::unique_ptr<HealthStatistics>;
        using HealthStatisticsSPtr = std::shared_ptr<HealthStatistics>;
    }
}


