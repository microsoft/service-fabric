// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class ChaosReport
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
        {
            DENY_COPY(ChaosReport)

        public:

            ChaosReport();
            ChaosReport(ChaosReport &&);

            __declspec(property(get = get_ChaosParameters)) std::unique_ptr<ChaosParameters> const & ChaosParametersUPtr;
            std::unique_ptr<ChaosParameters> const & get_ChaosParameters() const { return chaosParametersUPtr_; }

            __declspec(property(get = get_Status)) ChaosStatus::Enum const & Status;
            ChaosStatus::Enum const & get_Status() const { return status_; }

            __declspec(property(get = get_ContinuationToken)) std::wstring const& ContinuationToken;
            std::wstring const& get_ContinuationToken() const { return continuationToken_; }

            __declspec(property(get = get_History)) std::vector<ChaosEvent> const & History;
            std::vector<ChaosEvent> const & get_History() const { return history_; }

            Common::ErrorCode FromPublicApi(FABRIC_CHAOS_REPORT const &);
            Common::ErrorCode ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_CHAOS_REPORT &) const;

            FABRIC_FIELDS_04(
                chaosParametersUPtr_,
                status_,
                continuationToken_,
                history_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ChaosParameters, chaosParametersUPtr_);
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::Status, status_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ContinuationToken, continuationToken_);
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::History, history_);
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:

            std::unique_ptr<ChaosParameters> chaosParametersUPtr_;
            ChaosStatus::Enum status_;
            std::wstring continuationToken_;
            std::vector<ChaosEvent> history_;
        };
    }
}
