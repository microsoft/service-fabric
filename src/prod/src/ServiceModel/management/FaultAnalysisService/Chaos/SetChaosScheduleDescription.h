//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class SetChaosScheduleDescription
            : public Serialization::FabricSerializable
        {
            DENY_COPY(SetChaosScheduleDescription)

        public:

            SetChaosScheduleDescription();
            SetChaosScheduleDescription(SetChaosScheduleDescription &&);
            SetChaosScheduleDescription & operator = (SetChaosScheduleDescription && other);

            explicit SetChaosScheduleDescription(std::unique_ptr<ChaosScheduleDescription> &&);

            __declspec(property(get = get_ChaosScheduleDescription)) std::unique_ptr<ChaosScheduleDescription> const & ChaosScheduleDescriptionUPtr;

            std::unique_ptr<ChaosScheduleDescription> const & get_ChaosScheduleDescription() const { return chaosScheduleDescriptionUPtr_; }

            Common::ErrorCode FromPublicApi(FABRIC_CHAOS_SERVICE_SCHEDULE_DESCRIPTION const &);
            void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_CHAOS_SERVICE_SCHEDULE_DESCRIPTION &) const;

            FABRIC_FIELDS_01(
                chaosScheduleDescriptionUPtr_);

        private:

            std::unique_ptr<ChaosScheduleDescription> chaosScheduleDescriptionUPtr_;
        };
    }
}
