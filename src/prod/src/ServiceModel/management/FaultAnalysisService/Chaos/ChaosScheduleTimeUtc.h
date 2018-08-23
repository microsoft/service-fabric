//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class ChaosScheduleTimeUtc
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
        {
            DENY_COPY(ChaosScheduleTimeUtc)

        public:

            ChaosScheduleTimeUtc();
            ChaosScheduleTimeUtc(ChaosScheduleTimeUtc && other) = default;
            ChaosScheduleTimeUtc & operator = (ChaosScheduleTimeUtc && other) = default;

            __declspec(property(get = get_Hour)) ULONG const & Hour;
            ULONG const & get_Hour() const { return hour_; }

            __declspec(property(get = get_Minute)) ULONG const & Minute;
            ULONG const & get_Minute() const { return minute_; }

            Common::ErrorCode FromPublicApi(FABRIC_CHAOS_SCHEDULE_TIME_UTC const &);
            Common::ErrorCode ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_CHAOS_SCHEDULE_TIME_UTC &) const;

            FABRIC_FIELDS_02(
                hour_,
                minute_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Hour, hour_);
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Minute, minute_);
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:

            ULONG hour_;
            ULONG minute_;
        };
    }
}
