// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class GetChaosReportReplyMessageBody : public Serialization::FabricSerializable
        {
        public:
            GetChaosReportReplyMessageBody() : report_() { }

            GetChaosReportReplyMessageBody(ChaosReport && report) : report_(std::move(report)) { }

            __declspec(property(get=get_Report)) ChaosReport & Report;

            ChaosReport & get_Report() { return report_; }

            FABRIC_FIELDS_01(report_);

        private:
            ChaosReport report_;
        };
    }
}
