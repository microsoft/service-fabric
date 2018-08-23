// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class ChaosReportFilter
            : public Serialization::FabricSerializable
        {
        public:
            ChaosReportFilter();
            explicit ChaosReportFilter(ChaosReportFilter const &);
            explicit ChaosReportFilter(ChaosReportFilter &&);

            ChaosReportFilter(
                Common::DateTime const & startTimeUtc,
                Common::DateTime const & endTimeUtc);

            __declspec(property(get = get_StartTimeUtc)) Common::DateTime const & StartTimeUtc;
            Common::DateTime const & get_StartTimeUtc() const { return startTimeUtc_; }

            __declspec(property(get = get_EndTimeUtc)) Common::DateTime const & EndTimeUtc;
            Common::DateTime const & get_EndTimeUtc() const { return endTimeUtc_; }

            Common::ErrorCode FromPublicApi(FABRIC_CHAOS_REPORT_FILTER const &);
            void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_CHAOS_REPORT_FILTER &) const;

            Common::ErrorCode FromPublicApi(FABRIC_CHAOS_EVENTS_SEGMENT_FILTER const &);
            void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_CHAOS_EVENTS_SEGMENT_FILTER &) const;

            FABRIC_FIELDS_02(
                startTimeUtc_,
                endTimeUtc_);

        private:
            Common::DateTime startTimeUtc_;
            Common::DateTime endTimeUtc_;
        };
    }
}
