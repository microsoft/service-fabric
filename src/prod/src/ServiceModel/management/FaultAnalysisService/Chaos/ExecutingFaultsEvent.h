// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class ExecutingFaultsEvent : public ChaosEventBase
        {
            DENY_COPY(ExecutingFaultsEvent)
        public:
            ExecutingFaultsEvent();
            ExecutingFaultsEvent(
                Common::DateTime timeStampUtc,
                std::vector<std::wstring> const & faults);

            explicit ExecutingFaultsEvent(ExecutingFaultsEvent &&);
            ExecutingFaultsEvent & operator = (ExecutingFaultsEvent && other);

            __declspec(property(get = get_Faults)) std::vector<std::wstring> & Faults;
            std::vector<std::wstring> & get_Faults() { return faults_; }

            Common::ErrorCode FromPublicApi(FABRIC_CHAOS_EVENT const &) override;
            Common::ErrorCode ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_CHAOS_EVENT &) const override;

            FABRIC_FIELDS_03(
                kind_,
                timeStampUtc_,
                faults_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_CHAIN()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Faults, faults_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::vector<std::wstring> faults_;
        };

        DEFINE_CHAOS_EVENT_ACTIVATOR(ExecutingFaultsEvent, FABRIC_CHAOS_EVENT_KIND_EXECUTING_FAULTS)
    }
}

DEFINE_USER_ARRAY_UTILITY(Management::FaultAnalysisService::ExecutingFaultsEvent);
