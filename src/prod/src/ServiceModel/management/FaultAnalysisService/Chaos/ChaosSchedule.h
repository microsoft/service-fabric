// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class ChaosSchedule
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
        {
            DENY_COPY(ChaosSchedule)

        public:

            ChaosSchedule();
            ChaosSchedule(ChaosSchedule &&) = default;
            ChaosSchedule & operator = (ChaosSchedule && other) = default;

            __declspec(property(get = get_StartDate)) Common::DateTime const& StartDate;
            Common::DateTime const& get_StartDate() const { return startDate_; }

            __declspec(property(get = get_ExpiryDate)) Common::DateTime const& ExpiryDate;
            Common::DateTime const& get_ExpiryDate() const { return expiryDate_; }

            __declspec(property(get = get_ChaosParametersMap)) std::map<std::wstring, ChaosParameters> const& ChaosParametersMap;
            std::map<std::wstring, ChaosParameters> const& get_ChaosParametersMap() const { return chaosParametersMap_; }

            __declspec(property(get = get_Jobs)) std::vector<ChaosScheduleJob> const & Jobs;
            std::vector<ChaosScheduleJob> const & get_Jobs() const { return jobs_; }

            Common::ErrorCode FromPublicApi(FABRIC_CHAOS_SCHEDULE const &);
            Common::ErrorCode ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_CHAOS_SCHEDULE &) const;

            FABRIC_FIELDS_04(
                startDate_,
                expiryDate_,
                chaosParametersMap_,
                jobs_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::StartDate, startDate_);
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ExpiryDate, expiryDate_);
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ChaosParametersMap, chaosParametersMap_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Jobs, jobs_);
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:

            Common::DateTime startDate_;
            Common::DateTime expiryDate_;
            std::map<std::wstring, ChaosParameters> chaosParametersMap_;
            std::vector<ChaosScheduleJob> jobs_;
        };
    }
}

