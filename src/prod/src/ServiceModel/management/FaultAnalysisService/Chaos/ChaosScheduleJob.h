//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class ChaosScheduleJob
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
        {
            DENY_COPY(ChaosScheduleJob)

        public:

            ChaosScheduleJob();
            ChaosScheduleJob(ChaosScheduleJob &&) = default;
            ChaosScheduleJob & operator = (ChaosScheduleJob && other) = default;

            __declspec(property(get = get_ChaosParameters)) std::wstring const& ChaosParameters;
            std::wstring const& get_ChaosParameters() const { return chaosParametersReferenceKey_; }

            __declspec(property(get = get_ActiveDays)) std::unique_ptr<ChaosScheduleJobActiveDays> const& Days;
            std::unique_ptr<ChaosScheduleJobActiveDays> const& get_ActiveDays() const { return daysUPtr_; }

            __declspec(property(get = get_ActiveTimes)) std::vector<ChaosScheduleTimeRangeUtc> const& Times;
            std::vector<ChaosScheduleTimeRangeUtc> const& get_ActiveTimes() const { return times_; }

            Common::ErrorCode FromPublicApi(FABRIC_CHAOS_SCHEDULE_JOB const &);
            Common::ErrorCode ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_CHAOS_SCHEDULE_JOB &) const;

            FABRIC_FIELDS_03(
                chaosParametersReferenceKey_,
                daysUPtr_,
                times_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ChaosParameters, chaosParametersReferenceKey_);
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Days, daysUPtr_);
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Times, times_);
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:

            std::wstring chaosParametersReferenceKey_;
            std::unique_ptr<ChaosScheduleJobActiveDays> daysUPtr_;
            std::vector<ChaosScheduleTimeRangeUtc> times_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(Management::FaultAnalysisService::ChaosScheduleJob);
