//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class ChaosScheduleTimeRangeUtc
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
        {
            DENY_COPY(ChaosScheduleTimeRangeUtc)

        public:

            ChaosScheduleTimeRangeUtc();
            ChaosScheduleTimeRangeUtc(ChaosScheduleTimeRangeUtc && other) = default;
            ChaosScheduleTimeRangeUtc & operator = (ChaosScheduleTimeRangeUtc && other) = default;

            __declspec(property(get = get_StartTime)) ChaosScheduleTimeUtc const & StartTime;
            ChaosScheduleTimeUtc const & get_StartTime() const { return startTime_; }

            __declspec(property(get = get_EndTime)) ChaosScheduleTimeUtc const & EndTime;
            ChaosScheduleTimeUtc const & get_EndTime() const { return endTime_; }

            Common::ErrorCode FromPublicApi(FABRIC_CHAOS_SCHEDULE_TIME_RANGE_UTC const &);
            Common::ErrorCode ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_CHAOS_SCHEDULE_TIME_RANGE_UTC &) const;

            FABRIC_FIELDS_02(
                endTime_,
                startTime_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::StartTime, startTime_);
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::EndTime, endTime_);
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:

            ChaosScheduleTimeUtc startTime_;
            ChaosScheduleTimeUtc endTime_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(Management::FaultAnalysisService::ChaosScheduleTimeRangeUtc);
