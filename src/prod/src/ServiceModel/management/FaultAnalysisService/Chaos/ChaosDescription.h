//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class ChaosDescription
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
        {
            DENY_COPY(ChaosDescription)

        public:

            ChaosDescription();
            ChaosDescription(ChaosDescription &&) = default;
            ChaosDescription & operator=(ChaosDescription &&) = default;

            __declspec(property(get = get_ChaosParameters)) std::unique_ptr<ChaosParameters> const & ChaosParametersUPtr;
            std::unique_ptr<ChaosParameters> const & get_ChaosParameters() const { return chaosParametersUPtr_; }

            __declspec(property(get = get_Status)) ChaosStatus::Enum const & Status;
            ChaosStatus::Enum const & get_Status() const { return status_; }

            __declspec(property(get = get_ScheduleStatus)) ChaosScheduleStatus::Enum const & ScheduleStatus;
            ChaosScheduleStatus::Enum const & get_ScheduleStatus() const { return scheduleStatus_; }

            Common::ErrorCode FromPublicApi(FABRIC_CHAOS_DESCRIPTION const &);
            Common::ErrorCode ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_CHAOS_DESCRIPTION &) const;

            FABRIC_FIELDS_03(
                chaosParametersUPtr_,
                status_,
                scheduleStatus_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ChaosParameters, chaosParametersUPtr_);
            SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::ChaosStatus, status_)
            SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::ChaosScheduleStatus, scheduleStatus_);
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:

            std::unique_ptr<ChaosParameters> chaosParametersUPtr_;
            ChaosStatus::Enum status_;
            ChaosScheduleStatus::Enum scheduleStatus_;
        };
    }
}
