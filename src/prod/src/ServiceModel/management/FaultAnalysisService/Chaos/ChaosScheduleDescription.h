// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class ChaosScheduleDescription
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
        {
            DENY_COPY(ChaosScheduleDescription)

        public:

            ChaosScheduleDescription();
            ChaosScheduleDescription(ChaosScheduleDescription &&) = default;
            ChaosScheduleDescription & operator = (ChaosScheduleDescription && other) = default;

            __declspec(property(get = get_Version)) ULONG const& Version;
            ULONG const& get_Version() const { return version_; }

            _declspec(property(get = get_Schedule)) std::unique_ptr<ChaosSchedule> const& Schedule;
            std::unique_ptr<ChaosSchedule> const& get_Schedule() const { return scheduleUPtr_; }

            Common::ErrorCode FromPublicApi(FABRIC_CHAOS_SCHEDULE_DESCRIPTION const &);
            Common::ErrorCode ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_CHAOS_SCHEDULE_DESCRIPTION &) const;

            FABRIC_FIELDS_02(
                version_,
                scheduleUPtr_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Version, version_);
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Schedule, scheduleUPtr_);
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:

            ULONG version_;
            std::unique_ptr<ChaosSchedule> scheduleUPtr_;
        };
    }
}
