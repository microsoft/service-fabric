// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class StartedEvent : public ChaosEventBase
        {
            DENY_COPY(StartedEvent)
        public:
            StartedEvent();

            explicit StartedEvent(StartedEvent &&);
            StartedEvent & operator = (StartedEvent && other);

            __declspec(property(get = get_ChaosParameters)) std::unique_ptr<ChaosParameters> const & ChaosParametersUPtr;
            std::unique_ptr<ChaosParameters> const & get_ChaosParameters() const { return chaosParametersUPtr_; }

            Common::ErrorCode FromPublicApi(FABRIC_CHAOS_EVENT const &) override;
            Common::ErrorCode ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_CHAOS_EVENT &) const override;

            FABRIC_FIELDS_03(
                kind_,
                timeStampUtc_,
                chaosParametersUPtr_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_CHAIN()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ChaosParameters, chaosParametersUPtr_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:

            std::unique_ptr<ChaosParameters> chaosParametersUPtr_;
        };

        DEFINE_CHAOS_EVENT_ACTIVATOR(StartedEvent, FABRIC_CHAOS_EVENT_KIND_STARTED)
    }
}
