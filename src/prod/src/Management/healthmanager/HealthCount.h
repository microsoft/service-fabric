// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class HealthCount
        {
            DENY_COPY(HealthCount);

        public:
            HealthCount();
            ~HealthCount();

            HealthCount(HealthCount && other);
            HealthCount & operator = (HealthCount && other);
            
            __declspec(property(get=get_TotalCount)) ULONG TotalCount;
            ULONG get_TotalCount() const { return static_cast<ULONG>(totalCount_); }
            
            __declspec(property(get = get_ErrorCount)) ULONG ErrorCount;
            ULONG get_ErrorCount() const { return static_cast<ULONG>(errorCount_); }

            __declspec(property(get = get_OkCount)) ULONG OkCount;
            ULONG get_OkCount() const { return static_cast<ULONG>(okCount_); }

            __declspec(property(get = get_WarningCount)) ULONG WarningCount;
            ULONG get_WarningCount() const { return static_cast<ULONG>(warningCount_); }

            void AddResult(FABRIC_HEALTH_STATE result);

            bool IsHealthy(BYTE maxPercentUnhealthy) const;

            HealthStateCount GetHealthStateCount() const { return HealthStateCount(OkCount, WarningCount, ErrorCount); }

            FABRIC_HEALTH_STATE GetHealthState(BYTE maxPercentUnhealthy) const;

            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
            static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
            void FillEventData(Common::TraceEventContext & context) const;
            
            static bool IsRelevant(FABRIC_HEALTH_STATE aggregatedHealthState, FABRIC_HEALTH_STATE targetHealthState);

            static std::vector<ServiceModel::HealthEvaluation> FilterUnhealthy(
                std::vector<ServiceModel::HealthEvaluation> const & entries, 
                FABRIC_HEALTH_STATE aggregatedHealthState);

        protected:
            uint64 okCount_;
            uint64 warningCount_;
            uint64 errorCount_;
            uint64 totalCount_;
        };
    }
}
