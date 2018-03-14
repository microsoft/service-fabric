// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    // Wrapper that keeps track of report and other health client specific metadata,
    // like the state (waiting to be sent, waiting for ack etc)
    // and the last modified time.
    class HealthReportWrapper
    {
        DENY_COPY(HealthReportWrapper)
    public:
        explicit HealthReportWrapper(ServiceModel::HealthReport && report);

        HealthReportWrapper(HealthReportWrapper && other);
        HealthReportWrapper & operator=(HealthReportWrapper && other);

        ~HealthReportWrapper();

        __declspec(property(get = get_Report)) ServiceModel::HealthReport const & Report;
        ServiceModel::HealthReport const & get_Report() const { return report_; }

        // If the report was never sent or was sent more than retryTime ago, add it to the list of reports
        // and update the metadata.
        bool AddReportForSend(
            __inout std::vector<ServiceModel::HealthReport> & reports,
            Common::StopwatchTime const & now,
            Common::TimeSpan const & retryTime);

        // Add report without checking last sent time.
        // Needed for sequence streaming reports, where is important to 
        // include all reports in order.
        void AddReportForSend(
            __inout std::vector<ServiceModel::HealthReport> & reports);

        int Compare(HealthReportWrapper const & other) const; 
        bool operator<(HealthReportWrapper const & other) const;

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
        void FillEventData(Common::TraceEventContext & context) const;

    private:
        ServiceModel::HealthReport report_;
        HealthReportClientState::Enum reportState_;
        Common::StopwatchTime lastModifiedTime_;
    };

    using HealthReportWrapperUPtr = std::unique_ptr<HealthReportWrapper>;
    using HealthReportWrapperPtr = HealthReportWrapper*;
}
