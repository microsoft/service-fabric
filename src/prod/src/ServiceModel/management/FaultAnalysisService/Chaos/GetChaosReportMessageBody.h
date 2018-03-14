// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class GetChaosReportMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            GetChaosReportMessageBody() : getChaosReportDescription_() {}

            explicit GetChaosReportMessageBody(GetChaosReportDescription const & getChaosReportDescription) : getChaosReportDescription_(getChaosReportDescription) { }

            __declspec(property(get = get_ReportDescription)) GetChaosReportDescription const & ReportDescription;

            GetChaosReportDescription const & get_ReportDescription() const { return getChaosReportDescription_; }

            FABRIC_FIELDS_01(getChaosReportDescription_);

        private:
            GetChaosReportDescription getChaosReportDescription_;
        };
    }
}
