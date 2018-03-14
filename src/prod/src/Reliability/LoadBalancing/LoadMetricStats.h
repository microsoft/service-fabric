// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class LoadMetricStats : public Serialization::FabricSerializable
        {
        public:
            // empty constructor, used by serialization only
            LoadMetricStats();

            LoadMetricStats(std::wstring && name, uint value, Common::StopwatchTime timestamp);

            LoadMetricStats(LoadMetricStats && other);

            LoadMetricStats(LoadMetricStats const& other);

            LoadMetricStats & operator = (LoadMetricStats const & other);
            LoadMetricStats & operator = (LoadMetricStats && other);
            bool operator == (LoadMetricStats const & other) const;

            // properties
            __declspec (property(get=get_Name)) std::wstring const& Name;
            std::wstring const& get_Name() const { return name_; }

            __declspec (property(get=get_LastReportTime)) Common::StopwatchTime LastReportTime;
            Common::StopwatchTime get_LastReportTime() const { return lastReportTime_; }

            __declspec (property(get=get_Value)) uint Value;
            uint get_Value() const;

            void AdjustTimestamp(Common::TimeSpan diff);

            bool Update(uint value, Common::StopwatchTime timestamp);

            bool Update(LoadMetricStats && other);

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const;

            FABRIC_FIELDS_06(name_, lastReportValue_, count_, weightedSum_, sumOfWeight_, lastReportTime_);

        private:

            void ValidateValues() const;

            std::wstring name_;
            uint lastReportValue_;
            Common::StopwatchTime lastReportTime_;
            uint count_;
            double weightedSum_;
            double sumOfWeight_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(Reliability::LoadBalancingComponent::LoadMetricStats);
