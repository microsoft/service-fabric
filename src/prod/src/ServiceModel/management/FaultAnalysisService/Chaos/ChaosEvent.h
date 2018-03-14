// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class ChaosEvent 
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
        {
        public:
            ChaosEvent();
            explicit ChaosEvent(ChaosEventBaseSPtr && chaosEvent);
            explicit ChaosEvent(ChaosEventBaseSPtr const & chaosEvent);

            ChaosEvent(ChaosEvent const & other);
            ChaosEvent & operator = (ChaosEvent const & other);

            ChaosEvent(ChaosEvent && other);
            ChaosEvent & operator = (ChaosEvent && other);

            __declspec(property(get = get_ChaosEvent)) ChaosEventBaseSPtr const & Event;
            ChaosEventBaseSPtr const & get_ChaosEvent() const { return chaosEvent_; }

            Common::ErrorCode ToPublicApi(
                __in Common::ScopedHeap & heap,
                __out FABRIC_CHAOS_EVENT & publicChaosEVent) const;

            Common::ErrorCode FromPublicApi(
                FABRIC_CHAOS_EVENT const & publicChaosEVent);

            std::wstring ToString() const;

            FABRIC_FIELDS_01(
                chaosEvent_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ChaosEvent, chaosEvent_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            ChaosEventBaseSPtr chaosEvent_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(Management::FaultAnalysisService::ChaosEvent);
