// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class ChaosReportResult
        : public Api::IChaosReportResult
        , public Common::ComponentRoot
    {
        DENY_COPY(ChaosReportResult);

    public:
        ChaosReportResult(
            __in std::shared_ptr<Management::FaultAnalysisService::ChaosReport> && report);

        std::shared_ptr<Management::FaultAnalysisService::ChaosReport> const & GetChaosReport();

    private:
        std::shared_ptr<Management::FaultAnalysisService::ChaosReport> chaosReport_;
    };
}
