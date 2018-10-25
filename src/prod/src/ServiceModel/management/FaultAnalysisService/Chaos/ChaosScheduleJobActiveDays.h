//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class ChaosScheduleJobActiveDays
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
        {
            DENY_COPY(ChaosScheduleJobActiveDays)

        public:

            ChaosScheduleJobActiveDays();
            ChaosScheduleJobActiveDays(ChaosScheduleJobActiveDays &&) = default;
            ChaosScheduleJobActiveDays & operator = (ChaosScheduleJobActiveDays && other) = default;

            __declspec(property(get = get_Sunday)) bool const& Sunday;
            bool const& get_Sunday() const { return sunday_; }

            __declspec(property(get = get_Monday)) bool const& Monday;
            bool const& get_Monday() const { return monday_; }

            __declspec(property(get = get_Tuesday)) bool const& Tuesday;
            bool const& get_Tuesday() const { return tuesday_; }

            __declspec(property(get = get_Wednesday)) bool const& Wednesday;
            bool const& get_Wednesday() const { return wednesday_; }

            __declspec(property(get = get_Thursday)) bool const& Thursday;
            bool const& get_Thursday() const { return thursday_; }

            __declspec(property(get = get_Friday)) bool const& Friday;
            bool const& get_Friday() const { return friday_; }

            __declspec(property(get = get_Saturday)) bool const& Saturday;
            bool const& get_Saturday() const { return saturday_; }

            Common::ErrorCode FromPublicApi(FABRIC_CHAOS_SCHEDULE_JOB_ACTIVE_DAYS const &);
            Common::ErrorCode ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_CHAOS_SCHEDULE_JOB_ACTIVE_DAYS &) const;

            FABRIC_FIELDS_07(
                sunday_,
                monday_,
                tuesday_,
                wednesday_,
                thursday_,
                friday_,
                saturday_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Sunday, sunday_);
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Monday, monday_);
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Tuesday, tuesday_);
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Wednesday, wednesday_);
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Thursday, thursday_);
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Friday, friday_);
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Saturday, saturday_);
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:

            bool sunday_;
            bool monday_;
            bool tuesday_;
            bool wednesday_;
            bool thursday_;
            bool friday_;
            bool saturday_;
        };
    }
}
